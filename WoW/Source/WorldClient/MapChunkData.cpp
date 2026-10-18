#include "MapChunkData.h"
#include "MapChunkLiquid.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
void Require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Invalid ADT chunk: ") + message);
}
bool Finite(const C3Vector &v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
} // namespace

MapChunkData::MapChunkData(const void *data, size_t size, bool bigAlpha, uint8_t alphaMapShift)
    : surface(Terrain::ReadSurfaceChunk(data, size, bigAlpha, alphaMapShift)) {
    static_assert(sizeof(SMChunk) == 128 && sizeof(SMLayer) == 16 && sizeof(CWSoundEmitter) == 28, "MCNK disk layouts");
    const auto *bytes = static_cast<const uint8_t *>(data);
    std::memcpy(&header, bytes + 8, sizeof(header));
    auto word = [&](size_t offset) {
        Require(offset <= size && size - offset >= 4, "truncated scalar");
        uint32_t value;
        std::memcpy(&value, bytes + offset, 4);
        return value;
    };
    auto payload = [&](uint32_t offset, uint32_t token, size_t length, bool headerLength = false) {
        Require(offset >= 136 && offset <= size && size - offset >= 8, "subchunk offset out of range");
        Require(word(offset) == token, "unexpected subchunk token");
        const size_t count = headerLength ? length : word(size_t(offset) + 4);
        Require(count <= size - offset - 8 && length <= count, "subchunk record range exceeds container");
        return bytes + offset + 8;
    };
    layers.resize(header.m_nLayers);
    if (!layers.empty())
        std::memcpy(layers.data(), payload(header.m_ofsLayer, 0x4D434C59, layers.size() * sizeof(SMLayer)),
                    layers.size() * sizeof(SMLayer));
    const uint64_t count = uint64_t(header.m_nDoodadRefs) + header.m_nMapObjRefs;
    Require(count <= size / 4, "object reference count exceeds container");
    if (count) {
        const auto *source = payload(header.m_ofsRefs, 0x4D435246, size_t(count) * 4);
        references.resize(size_t(count));
        std::memcpy(references.data(), source, references.size() * 4);
    }
    Require(header.m_nSndEmitters <= size / sizeof(CWSoundEmitter), "sound emitter count exceeds container");
    if (header.m_nSndEmitters) {
        const size_t length = size_t(header.m_nSndEmitters) * sizeof(CWSoundEmitter);
        const auto *source = payload(header.m_ofsSndEmitters, 0x4D435345, length);
        sounds.resize(header.m_nSndEmitters);
        std::memcpy(sounds.data(), source, length);
        for (const auto &sound : sounds)
            Require(Finite(sound.m_position) && Finite(sound.m_size), "non-finite sound emitter");
    }
    if (header.m_sizeLiquid > 8) {
        // Original MCLQ records use the MCNK length even when their own size
        // word is zero. Keep that convention without rewriting either header.
        const size_t length = header.m_sizeLiquid - 8;
        liquids = Liquid::ReadLegacyChunks(payload(header.m_ofsLiquid, 0x4D434C51, length, true), length, header.m_flags);
    }
}
