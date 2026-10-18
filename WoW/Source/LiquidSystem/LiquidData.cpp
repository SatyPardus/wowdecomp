#include "Liquid.h"
#include <array>
#include <vector>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <string>

static_assert(sizeof(SMLiquidChunkDesc) == 12);
static_assert(sizeof(SMLiquidInstance) == 24);

namespace {
void RequireLiquid(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Invalid MH2O liquid data: ") + message);
}
void VertexIndex(uint32_t index, uint32_t count) { RequireLiquid(index < count, "vertex index out of bounds"); }
} // namespace
struct Liquid::CAreaData::Storage {
    std::vector<uint8_t> bytes;
    std::array<SMLiquidChunkDesc, 256> chunks{};
    std::array<size_t, 256> first{};
    std::vector<SMLiquidInstance> instances;
};
Liquid::CAreaData::~CAreaData() = default;
size_t Liquid::CAreaData::ChunkIndex(const SMLiquidChunkDesc *chunk) const {
    const uintptr_t base = reinterpret_cast<uintptr_t>(m_storage->chunks.data()), address = reinterpret_cast<uintptr_t>(chunk);
    RequireLiquid(address >= base && address - base < sizeof(m_storage->chunks) && (address - base) % sizeof(*chunk) == 0,
                  "foreign chunk descriptor");
    return (address - base) / sizeof(*chunk);
}
void Liquid::CAreaData::SetData(const void *data, size_t size) {
    if (!data && !size) {
        SetData(nullptr);
        return;
    }
    RequireLiquid(data && size >= 256 * sizeof(SMLiquidChunkDesc) && size <= 64 * 1024 * 1024, "invalid payload size");
    const auto *input = static_cast<const uint8_t *>(data);
    auto range = [&](size_t offset, size_t length) {
        RequireLiquid(offset <= size && length <= size - offset, "payload range out of bounds");
        return input + offset;
    };
    auto owned = std::make_unique<Storage>();
    std::memcpy(owned->chunks.data(), input, sizeof(owned->chunks));
    size_t total = 0;
    for (const auto &chunk : owned->chunks) {
        RequireLiquid(chunk.m_layerCount <= (size - std::min(size, size_t(chunk.m_offsetInstances))) / sizeof(SMLiquidInstance),
                      "instance count exceeds payload");
        // Limit cumulative allocations, including intentionally aliased ranges.
        RequireLiquid(chunk.m_layerCount <= 65536 - total, "too many liquid layers");
        total += chunk.m_layerCount;
    }
    owned->instances.reserve(total);
    for (unsigned i = 0; i < 256; ++i) {
        const auto &chunk = owned->chunks[i];
        owned->first[i] = owned->instances.size();
        if (!chunk.m_layerCount)
            continue;
        RequireLiquid(chunk.m_offsetInstances != 0, "missing instance descriptors");
        if (chunk.m_offsetAttributes)
            range(chunk.m_offsetAttributes, 16);
        for (unsigned j = 0; j < chunk.m_layerCount; ++j) {
            SMLiquidInstance instance;
            std::memcpy(&instance, range(size_t(chunk.m_offsetInstances) + j * sizeof(instance), sizeof(instance)), sizeof(instance));
            RequireLiquid(instance.m_vertexFormat <= 2, "unsupported vertex format");
            RequireLiquid(instance.m_width && instance.m_height && unsigned(instance.m_xOffset) + instance.m_width <= 8 &&
                              unsigned(instance.m_yOffset) + instance.m_height <= 8,
                          "invalid liquid tile rectangle");
            RequireLiquid(std::isfinite(instance.m_minHeight) && std::isfinite(instance.m_maxHeight), "non-finite liquid bounds");
            const size_t vertices = (instance.m_width + 1) * (instance.m_height + 1), tiles = instance.m_width * instance.m_height;
            if (instance.m_offsetExistsBitmap)
                range(instance.m_offsetExistsBitmap, (tiles + 7) / 8);
            if (instance.m_vertexFormat < 2) {
                RequireLiquid(instance.m_offsetVertexData != 0, "missing height data");
                const auto *values = range(instance.m_offsetVertexData, vertices * (instance.m_vertexFormat == 0 ? 5 : 8));
                for (size_t k = 0; k < vertices; ++k) {
                    float height;
                    std::memcpy(&height, values + 4 * k, 4);
                    RequireLiquid(std::isfinite(height), "non-finite liquid height");
                }
            } else if (instance.m_offsetVertexData)
                range(instance.m_offsetVertexData, vertices);
            owned->instances.push_back(instance);
        }
    }
    owned->bytes.assign(input, input + size);
    m_storage = std::move(owned);
    m_data = m_storage->bytes.data();
}

// STATIC: 0x00A59484
uint8_t Liquid::CAreaData::s_emptyTable[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// STATIC: 0x00A5948C
uint8_t Liquid::CAreaData::s_fullTable[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// STATIC: 0x00A59494
uint16_t Liquid::CAreaData::s_defaultTexCoord[2] = {0, 0};

// VERIFY: b3a3de07 auto
// OFFSET: 0x007D4F10
void Liquid::CAreaData::SetData(const void *data) {
    m_storage.reset();
    this->m_data = static_cast<const uint8_t *>(data);
}

// VERIFY: b03625ac auto
// OFFSET: 0x008A3050
Liquid::CAreaData::CAreaData(const void *data) { this->m_data = static_cast<const uint8_t *>(data); }

// VERIFY: 71cd1ec5 manual
// OFFSET: 0x008A3060
bool Liquid::CAreaData::HasData() const { return this->m_data != nullptr; }

// VERIFY: 6ee106da auto
// OFFSET: 0x008A3070
const SMLiquidChunkDesc *Liquid::CAreaData::GetChunkDesc(uint32_t x, uint32_t y) const {
    RequireLiquid(m_data && x < 16 && y < 16, "chunk coordinates out of bounds");
    if (m_storage)
        return &m_storage->chunks[x + 16 * y];
    return reinterpret_cast<const SMLiquidChunkDesc *>(this->m_data) + (x + 16 * y);
}

// VERIFY: f4b46567 auto
// OFFSET: 0x008A3090
const SMLiquidInstance *Liquid::CAreaData::GetInstanceDesc(const SMLiquidChunkDesc *chunk, uint32_t index) const {
    if (m_storage) {
        const auto slot = ChunkIndex(chunk);
        RequireLiquid(index < chunk->m_layerCount, "layer index out of bounds");
        return &m_storage->instances[m_storage->first[slot] + index];
    }
    return reinterpret_cast<const SMLiquidInstance *>(this->m_data + chunk->m_offsetInstances) + index;
}

// VERIFY: 6e52450a auto
// OFFSET: 0x008A30B0
const uint8_t *Liquid::CAreaData::GetFishableTable(const SMLiquidChunkDesc *chunk) const {
    if (m_storage)
        ChunkIndex(chunk);
    if (!chunk->m_layerCount) {
        return Liquid::CAreaData::s_emptyTable;
    }

    if (chunk->m_offsetAttributes) {
        return this->m_data + chunk->m_offsetAttributes;
    }

    return Liquid::CAreaData::s_fullTable;
}

// VERIFY: 3dff8387 auto
// OFFSET: 0x008A30E0
const uint8_t *Liquid::CAreaData::GetDeepTable(const SMLiquidChunkDesc *chunk) const {
    if (m_storage)
        ChunkIndex(chunk);
    if (!chunk->m_layerCount) {
        return Liquid::CAreaData::s_emptyTable;
    }

    if (chunk->m_offsetAttributes) {
        return this->m_data + chunk->m_offsetAttributes + 8;
    }

    return Liquid::CAreaData::s_fullTable;
}

// VERIFY: 6b0d6dd8 manual
// OFFSET: 0x008A3110
const uint8_t *Liquid::CAreaData::GetExistsTable(const SMLiquidInstance *instance) const {
    if (m_storage) {
        const uintptr_t base = reinterpret_cast<uintptr_t>(m_storage->instances.data()), address = reinterpret_cast<uintptr_t>(instance);
        RequireLiquid(address >= base && address - base < m_storage->instances.size() * sizeof(*instance) &&
                          (address - base) % sizeof(*instance) == 0,
                      "foreign instance descriptor");
    }
    if (instance->m_offsetExistsBitmap) {
        return this->m_data + instance->m_offsetExistsBitmap;
    }

    return Liquid::CAreaData::s_fullTable;
}

// VERIFY: 0ecbc03f manual
// OFFSET: 0x008A3130
float Liquid::CInstanceVertexData::GetHeight(uint32_t index) {
    VertexIndex(index, m_vertexCount);
    if (this->m_instance->m_vertexFormat > 1) {
        return 0.0f;
    }

    float value;
    std::memcpy(&value, m_data + m_instance->m_offsetVertexData + 4 * index, sizeof(value));
    return value;
}

// VERIFY: 43850342 manual
// OFFSET: 0x008A3160
uint8_t Liquid::CInstanceVertexData::GetDepth(uint32_t index) {
    VertexIndex(index, m_vertexCount);
    const SMLiquidInstance *instance = this->m_instance;

    if (instance->m_vertexFormat == 0) {
        return (this->m_data + instance->m_offsetVertexData + 4 * this->m_vertexCount)[index];
    }

    if (instance->m_vertexFormat != 2) {
        return 0;
    }

    if (instance->m_offsetVertexData) {
        return (this->m_data + instance->m_offsetVertexData)[index];
    }

    return 0xFF;
}

// VERIFY: 5fa5f4f8 manual
// OFFSET: 0x008A31B0
const uint16_t *Liquid::CInstanceVertexData::GetTexCoord(uint32_t index) {
    VertexIndex(index, m_vertexCount);
    if (this->m_instance->m_vertexFormat == 1) {
        std::memcpy(m_nativeTexCoord, m_data + m_instance->m_offsetVertexData + 4 * (index + m_vertexCount), sizeof(m_nativeTexCoord));
        return m_nativeTexCoord;
    }

    return Liquid::CAreaData::s_defaultTexCoord;
}

// VERIFY: 7b8fdcd4 auto
// OFFSET: 0x008A31E0
Liquid::CChunkVertexData::CChunkVertexData(const uint8_t *data, uint32_t format) {
    this->m_data = data;
    this->m_format = format;
}

// VERIFY: 9dcee70a manual
// OFFSET: 0x008A3200
float Liquid::CChunkVertexData::GetHeight(uint32_t index) {
    VertexIndex(index, 81);
    if (this->m_format > 1) {
        return 0.0f;
    }

    float value;
    std::memcpy(&value, m_data + 8 * index + 4, sizeof(value));
    return value;
}

// VERIFY: 0121d633 manual
// OFFSET: 0x008A3220
uint8_t Liquid::CChunkVertexData::GetDepth(uint32_t index) {
    VertexIndex(index, 81);
    uint32_t format = this->m_format;

    if (format == 0 || format == 2) {
        return this->m_data[8 * index];
    }

    return 0;
}

// VERIFY: 681ed984 manual
// OFFSET: 0x008A3250
const uint16_t *Liquid::CChunkVertexData::GetTexCoord(uint32_t index) {
    VertexIndex(index, 81);
    if (this->m_format == 1) {
        std::memcpy(m_nativeTexCoord, m_data + 8 * index, sizeof(m_nativeTexCoord));
        return m_nativeTexCoord;
    }

    return Liquid::CAreaData::s_defaultTexCoord;
}

// VERIFY: 3d77147d auto
// OFFSET: 0x008A3280
Liquid::CInstanceVertexData::CInstanceVertexData(const Liquid::CAreaData *areaData, const SMLiquidInstance *instance) {
    this->m_data = areaData->m_data;
    this->m_instance = instance;
    this->m_vertexCount = (instance->m_width + 1) * (instance->m_height + 1);
}
