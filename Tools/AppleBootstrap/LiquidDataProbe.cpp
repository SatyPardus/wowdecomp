#include "WoW/Source/LiquidSystem/Liquid.h"
#include <vector>
#include <algorithm>
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
void CheckTerrainLiquids(const std::vector<uint8_t> &adt, std::ostream &report) {
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
