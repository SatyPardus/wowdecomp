#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapChunkLiquid.h"
#include <vector>
#include <algorithm>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "WoW/Source/Client.h"
#include <cstring>
#include <ostream>
#include <stdexcept>

namespace {
void Require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class T> T Read(const uint8_t *data, size_t offset) {
    T value;
    std::memcpy(&value, data + offset, sizeof(value));
    return value;
}
void Write(std::vector<uint8_t> &data, size_t at, uint32_t value) { std::memcpy(data.data() + at, &value, sizeof(value)); }
} // namespace
namespace {
void CheckLegacyLiquids(const std::vector<uint8_t> &adt, std::ostream &report) {
    unsigned total = 0, tiles = 0;
    for (size_t at = 0; at + 8 <= adt.size();) {
        const auto token = Read<uint32_t>(adt.data(), at), size = Read<uint32_t>(adt.data(), at + 4);
        Require(size <= adt.size() - at - 8, "Truncated legacy liquid fixture");
        if (token == 0x4D434E4B && size >= 128) {
            const auto *header = adt.data() + at + 8;
            const auto flags = Read<uint32_t>(header, 0), offset = Read<uint32_t>(header, 0x60), length = Read<uint32_t>(header, 0x64);
            if (length > 8) {
                Require(offset <= size_t(size) + 8 && length <= size_t(size) + 8 - offset, "Invalid MCLQ fixture extent");
                const auto *source = adt.data() + at + offset + 8;
                std::vector<uint8_t> unaligned(length - 7);
                std::memcpy(unaligned.data() + 1, source, length - 8);
                const auto records = Liquid::ReadLegacyChunks(unaligned.data() + 1, length - 8, flags);
                Require(records.size() && !std::memcmp(records.data(), source, records.size() * sizeof(SMLiquidChunk)),
                        "MCLQ record bytes changed");
                std::fill(unaligned.begin(), unaligned.end(), 0);
                CMapChunk owner;
                owner.m_topLeftCoords = C3Vector(0, 0, 0);
                owner.m_cOffset = {0, 0};
                unsigned layer = 0;
                for (unsigned kind = 0; kind < 4; ++kind)
                    if (flags & (4 << kind)) {
                        const auto &record = records[layer++];
                        CChunkLiquid liquid;
                        liquid.Initialize(owner, kind == 2 ? 15 : kind + 1, kind == 1 ? 2 : kind >= 2 ? 1 : 0, record);
                        unsigned present = 0;
                        for (unsigned y = 0; y < 8; ++y)
                            for (unsigned x = 0; x < 8; ++x) {
                                const auto tag = source[(layer - 1) * 804 + 656 + y * 8 + x] & 15;
                                const bool exists = tag != 15 && (tag & 3) == kind;
                                Require(liquid.TileExists(x, y) == exists, "MCLQ tile filtering changed");
                                present += exists;
                            }
                        float height = 0;
                        Require(!liquid.GetHeight(C2Vector(0, 0), C2iVector{8, 7}, height) &&
                                    !liquid.GetHeight(C2Vector(0, 0), C2iVector{7, 8}, height),
                                "Liquid accepted the exclusive tile boundary");
                        for (unsigned i = 0; i < 81; ++i) {
                            const auto *raw = source + (layer - 1) * 804 + 8 + i * 8;
                            const auto expected = kind == 1 ? 0 : Read<float>(raw, 4);
                            Require(liquid.m_vertexData->GetHeight(i) == expected && liquid.m_verts[i].z == expected,
                                    "MCLQ authored height changed");
                            if (kind >= 2) {
                                const auto *uv = liquid.m_vertexData->GetTexCoord(i);
                                Require(uv[0] == Read<uint16_t>(raw, 0) && uv[1] == Read<uint16_t>(raw, 2), "MCLQ authored UV changed");
                            }
                        }
                        std::array<uint16_t, 384> indices{};
                        CGxBatch batch{};
                        liquid.BuildIndices(indices.data(), 0, &batch);
                        Require(batch.m_count <= indices.size(), "MCLQ strip exceeds its allocation");
                        unsigned triangles = 0;
                        for (unsigned i = 2; i < batch.m_count; ++i) {
                            const auto a = indices[i - 2], b = indices[i - 1], c = indices[i];
                            Require(a < 81 && b < 81 && c < 81, "MCLQ index out of range");
                            if (a == b || a == c || b == c)
                                continue;
                            const auto x = std::min({a % 9, b % 9, c % 9}), y = std::min({a / 9, b / 9, c / 9});
                            Require(x < 8 && y < 8 && liquid.TileExists(x, y), "MCLQ strip crossed an authored hole");
                            ++triangles;
                        }
                        Require(triangles == 2 * present, "MCLQ strip omitted visible tiles");
                        tiles += present;
                        ++total;
                    }
                bool rejected = false;
                try {
                    Liquid::ReadLegacyChunks(source, records.size() * 804 - 1, flags);
                } catch (const std::exception &) {
                    rejected = true;
                }
                Require(rejected, "MCLQ accepted truncated layers");
                std::vector<uint8_t> bad(source, source + length - 8);
                Write(bad, 720, 3);
                rejected = false;
                try {
                    Liquid::ReadLegacyChunks(bad.data(), bad.size(), flags);
                } catch (const std::exception &) {
                    rejected = true;
                }
                Require(rejected, "MCLQ accepted out-of-range flow count");
            }
        }
        at += size_t(size) + 8;
    }
    if (total)
        report << "  MCLQ: " << total << " owned aligned layers / " << tiles
               << " visible tiles; original heights, magma UV, zero chunk-size rule, tile filtering/strip holes and exclusive boundaries "
                  "passed\n";
}
} // namespace
void CheckTerrainLiquids(const std::vector<uint8_t> &adt, std::ostream &report) {
    CheckLegacyLiquids(adt, report);
    const uint8_t *source = nullptr;
    size_t length = 0;
    for (size_t at = 0; at + 8 <= adt.size();) {
        const auto size = Read<uint32_t>(adt.data(), at + 4);
        Require(size <= adt.size() - at - 8, "Truncated liquid fixture container");
        if (Read<uint32_t>(adt.data(), at) == 0x4D48324F) {
            source = adt.data() + at + 8;
            length = size;
            break;
        }
        at += 8 + size;
    }
    if (!source || !length)
        return;
    std::vector<uint8_t> unaligned(length + 1);
    std::memcpy(unaligned.data() + 1, source, length);
    Liquid::CAreaData area(nullptr);
    area.SetData(unaligned.data() + 1, length);
    unsigned layers = 0, vertices = 0, firstChunk = 256;
    for (unsigned i = 0; i < 256; ++i) {
        const auto original = Read<SMLiquidChunkDesc>(source, i * 12);
        const auto *chunk = area.GetChunkDesc(i % 16, i / 16);
        Require(reinterpret_cast<uintptr_t>(chunk) % alignof(SMLiquidChunkDesc) == 0 && !std::memcmp(chunk, &original, 12),
                "MH2O chunk descriptor changed or unaligned");
        for (unsigned b = 0; b < 8; ++b) {
            const auto fishable = original.m_layerCount ? original.m_offsetAttributes ? source[original.m_offsetAttributes + b] : 255 : 0;
            const auto deep = original.m_layerCount ? original.m_offsetAttributes ? source[original.m_offsetAttributes + 8 + b] : 255 : 0;
            Require(area.GetFishableTable(chunk)[b] == fishable && area.GetDeepTable(chunk)[b] == deep,
                    "MH2O fishable/deep semantics changed");
        }
        for (unsigned j = 0; j < chunk->m_layerCount; ++j) {
            if (firstChunk == 256)
                firstChunk = i;
            const auto expected = Read<SMLiquidInstance>(source, original.m_offsetInstances + j * 24);
            const auto *instance = area.GetInstanceDesc(chunk, j);
            Require(reinterpret_cast<uintptr_t>(instance) % alignof(SMLiquidInstance) == 0 && !std::memcmp(instance, &expected, 24),
                    "MH2O instance changed or unaligned");
            Liquid::CInstanceVertexData data(&area, instance);
            const unsigned count = (expected.m_width + 1) * (expected.m_height + 1);
            for (unsigned k = 0; k < count; ++k) {
                const float height = expected.m_vertexFormat < 2 ? Read<float>(source, expected.m_offsetVertexData + 4 * k) : 0;
                const auto depth = expected.m_vertexFormat == 0 ? source[expected.m_offsetVertexData + 4 * count + k]
                                   : expected.m_vertexFormat == 2
                                       ? expected.m_offsetVertexData ? source[expected.m_offsetVertexData + k] : 255
                                       : 0;
                const uint16_t u = expected.m_vertexFormat == 1 ? Read<uint16_t>(source, expected.m_offsetVertexData + 4 * (count + k)) : 0;
                const uint16_t v =
                    expected.m_vertexFormat == 1 ? Read<uint16_t>(source, expected.m_offsetVertexData + 4 * (count + k) + 2) : 0;
                const auto *uv = data.GetTexCoord(k);
                Require(data.GetHeight(k) == height && data.GetDepth(k) == depth &&
                            reinterpret_cast<uintptr_t>(uv) % alignof(uint16_t) == 0 && uv[0] == u && uv[1] == v,
                        "MH2O height/depth/UV data changed");
            }
            const unsigned bitmap = (expected.m_width * expected.m_height + 7) / 8;
            for (unsigned k = 0; k < bitmap; ++k)
                Require(area.GetExistsTable(instance)[k] ==
                            (expected.m_offsetExistsBitmap ? source[expected.m_offsetExistsBitmap + k] : 255),
                        "MH2O tile existence changed");
            bool rejected = false;
            try {
                data.GetHeight(count);
            } catch (const std::exception &) {
                rejected = true;
            }
            Require(rejected, "MH2O accepted an out-of-range vertex");
            vertices += count;
            ++layers;
        }
    }
    Require(!std::memcmp(source, unaligned.data() + 1, length), "MH2O source was modified");
    // Owned views remain valid after the source lifetime ends.
    std::fill(unaligned.begin(), unaligned.end(), 0);
    unaligned.clear();
    unaligned.shrink_to_fit();
    Require(firstChunk < 256, "Liquid fixture has no layers");
    Require(area.HasData() && area.GetChunkDesc(firstChunk % 16, firstChunk / 16)->m_layerCount, "MH2O retained borrowed input");
    const auto descriptor = Read<SMLiquidChunkDesc>(source, firstChunk * 12);
    for (unsigned mode = 0; mode < 7; ++mode) {
        std::vector<uint8_t> bad(source, source + length);
        if (mode == 0)
            bad.resize(3071);
        if (mode == 1)
            Write(bad, firstChunk * 12, 0xfffffff0);
        if (mode == 2)
            Write(bad, firstChunk * 12 + 4, 0xffffffff);
        if (mode == 3)
            bad[descriptor.m_offsetInstances + 14] = 9;
        if (mode == 4)
            Write(bad, descriptor.m_offsetInstances + 16, uint32_t(length));
        if (mode == 5)
            Write(bad, descriptor.m_offsetInstances + 20, uint32_t(length));
        if (mode == 6)
            Write(bad, descriptor.m_offsetInstances + 4, 0x7fc00000);
        bool rejected = false;
        try {
            area.SetData(bad.data(), bad.size());
        } catch (const std::exception &) {
            rejected = true;
        }
        Require(rejected && area.GetChunkDesc(firstChunk % 16, firstChunk / 16)->m_layerCount == descriptor.m_layerCount,
                "MH2O invalid replacement corrupted live data");
    }
    area.SetData(nullptr, 0);
    Require(!area.HasData(), "MH2O clear retained owned data");

    // A valid unaligned format-1 instance covers authored magma coordinates;
    // the real exterior-water fixtures use height/depth or implicit ocean data.
    std::vector<uint8_t> magma(3072 + 24 + 4 * 8 + 1, 0);
    auto *raw = magma.data() + 1;
    SMLiquidChunkDesc chunk{3072, 1, 0};
    SMLiquidInstance instance{3, 1, 2, 5, 0, 0, 1, 1, 0, 3096};
    std::memcpy(raw, &chunk, 12);
    std::memcpy(raw + 3072, &instance, 24);
    for (unsigned i = 0; i < 4; ++i) {
        float h = 2 + float(i);
        uint16_t uv[2] = {uint16_t(257 * i), uint16_t(129 * i)};
        std::memcpy(raw + 3096 + i * 4, &h, 4);
        std::memcpy(raw + 3096 + 16 + i * 4, uv, 4);
    }
    area.SetData(raw, magma.size() - 1);
    Liquid::CInstanceVertexData data(&area, area.GetInstanceDesc(area.GetChunkDesc(0, 0), 0));
    for (unsigned i = 0; i < 4; ++i) {
        const auto *uv = data.GetTexCoord(i);
        Require(data.GetHeight(i) == 2 + float(i) && uv[0] == 257 * i && uv[1] == 129 * i, "Aligned MH2O magma UV/height mismatch");
    }
    std::vector<uint8_t> legacy(81 * 8 + 1);
    for (unsigned i = 0; i < 81; ++i) {
        float h = 0.25f * i;
        uint16_t uv[2] = {uint16_t(7 * i), uint16_t(11 * i)};
        std::memcpy(legacy.data() + 1 + i * 8, uv, 4);
        std::memcpy(legacy.data() + 5 + i * 8, &h, 4);
    }
    Liquid::CChunkVertexData old(legacy.data() + 1, 1);
    for (unsigned i = 0; i < 81; ++i) {
        const auto *uv = old.GetTexCoord(i);
        Require(old.GetHeight(i) == 0.25f * i && uv[0] == 7 * i && uv[1] == 11 * i, "Unaligned MCLQ height/UV mismatch");
    }
    report << "  MH2O: " << layers << " original layers / " << vertices
           << " vertices, owned aligned descriptors, authored heights/depth/UV/bitmaps, implicit ocean defaults and invalid replacement "
              "checks passed\n";
}

void CheckMapDatabase(const ClientData &client, std::ostream &report) {
    const auto asset = client.Read("DBFilesClient\\Map.dbc", 1024 * 1024);
    const auto *bytes = asset.bytes.data();
    const auto count = Read<uint32_t>(bytes, 4), stride = Read<uint32_t>(bytes, 12);
    Require(stride == 264 && 20 + size_t(count) * stride <= asset.bytes.size(), "Invalid map fixture");
    const auto *strings = reinterpret_cast<const char *>(bytes + 20 + count * stride);
    g_mapDB.Load(__FILE__, __LINE__);
    Require(g_mapDB.m_loaded, "Original Map database did not load");
    unsigned checked = 0;
    for (unsigned i = 0; i < count; ++i) {
        const auto *row = bytes + 20 + i * stride;
        const auto id = Read<uint32_t>(row, 0);
        if (id != 0 && id != 530 && id != 571)
            continue;
        const auto *record = g_mapDB.GetRecord(id);
        Require(record && !std::strcmp(record->m_directory, strings + Read<uint32_t>(row, 4)) &&
                    !std::strcmp(record->m_mapName, strings + Read<uint32_t>(row, 20 + 4 * s_locale)),
                "Map directory or localized name differs from original DBC");
        ++checked;
    }
    Require(checked == 3, "Expected world map records missing");
    report << "  Map database: original world IDs/directories and selected DBC locale slot " << s_locale << " passed\n";
}
