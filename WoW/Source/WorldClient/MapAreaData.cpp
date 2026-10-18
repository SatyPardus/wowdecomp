#include "MapAreaData.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace {
void Require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Invalid ADT area: ") + message);
}
struct Bytes {
    const uint8_t *data = nullptr;
    size_t size = 0;
    Bytes Slice(size_t offset, size_t count) const {
        Require(offset <= size && count <= size - offset, "extent exceeds container");
        return {data + offset, count};
    }
    template <class T> T Value(size_t offset) const {
        T value;
        std::memcpy(&value, Slice(offset, sizeof(T)).data, sizeof(T));
        return value;
    }
    template <class T> std::vector<T> Records() const {
        Require(size % sizeof(T) == 0, "partial table record");
        std::vector<T> result(size / sizeof(T));
        if (size)
            std::memcpy(result.data(), data, size);
        return result;
    }
    Bytes Block(size_t offset, uint32_t token, size_t minimum = 0) const {
        Require(Value<uint32_t>(offset) == token, "unexpected chunk token");
        auto result = Slice(offset + 8, Value<uint32_t>(offset + 4));
        Require(result.size >= minimum, "short chunk payload");
        return result;
    }
};
uint32_t CheckNames(Bytes names) {
    uint32_t count = 0;
    for (size_t at = 0; at < names.size; ++count) {
        const auto *end = static_cast<const uint8_t *>(std::memchr(names.data + at, 0, names.size - at));
        Require(end && end != names.data + at, "unterminated or empty filename");
        at = size_t(end - names.data) + 1;
    }
    return count;
}
void CheckOffsets(const std::vector<uint32_t> &offsets, const std::vector<char> &names) {
    for (auto offset : offsets)
        Require(offset < names.size() && names[offset] && std::memchr(names.data() + offset, 0, names.size() - offset),
                "invalid filename offset or missing terminator");
}
bool Finite(const C3Vector &v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
} // namespace

MapAreaData::MapAreaData(const void *bytes, size_t size) {
    static_assert(sizeof(SMAreaHeader) == 64 && sizeof(SMChunkInfo) == 16 && sizeof(SMDoodadDef) == 36 && sizeof(SMMapObjDef) == 64,
                  "ADT disk table layout");
    Require(bytes && size >= 84 && size <= MaxBytes, "invalid file size or budget exceeded");
    Bytes file{static_cast<const uint8_t *>(bytes), size};
    Require(file.Block(0, 0x4D564552).size == 4 && file.Value<uint32_t>(8) == 18, "unsupported version");
    const auto mhdr = file.Block(12, 0x4D484452, sizeof(header));
    header = mhdr.Value<SMAreaHeader>(0);
    Require(header.m_mampValue <= 6, "invalid alpha dimension shift");
    // Header offsets must identify actual top-level chunk boundaries, not byte
    // patterns embedded in another payload. Unknown chunks remain untouched.
    std::unordered_map<size_t, uint32_t> tokens;
    size_t terrainCount = 0;
    for (size_t at = 0; at < size;) {
        Require(tokens.size() < 4096, "too many chunks");
        const auto token = file.Value<uint32_t>(at);
        const auto payload = file.Slice(at + 8, file.Value<uint32_t>(at + 4));
        tokens.emplace(at, token);
        terrainCount += token == 0x4D434E4B;
        at += payload.size + 8;
    }
    auto table = [&](uint32_t offset, uint32_t token, bool required = false) -> Bytes {
        if (!offset) {
            Require(!required, "missing required table");
            return {file.data, 0};
        }
        const size_t at = size_t(offset) + 20;
        const auto found = tokens.find(at);
        Require(found != tokens.end() && found->second == token, "table offset is not its chunk boundary");
        return file.Block(at, token);
    };
    chunks = table(header.m_mcin, 0x4D43494E, true).Records<SMChunkInfo>();
    Require(chunks.size() == 256 && terrainCount == 256, "incomplete terrain index");
    const auto textures = table(header.m_mtex, 0x4D544558, true);
    textureCount = CheckNames(textures);
    textureNames = textures.Records<char>();
    const auto models = table(header.m_mmdx, 0x4D4D4458), wmos = table(header.m_mwmo, 0x4D574D4F);
    modelNames = models.Records<char>();
    wmoNames = wmos.Records<char>();
    modelOffsets = table(header.m_mmid, 0x4D4D4944).Records<uint32_t>();
    wmoOffsets = table(header.m_mwid, 0x4D574944).Records<uint32_t>();
    CheckOffsets(modelOffsets, modelNames);
    CheckOffsets(wmoOffsets, wmoNames);
    doodads = table(header.m_mddf, 0x4D444446).Records<SMDoodadDef>();
    objects = table(header.m_modf, 0x4D4F4446).Records<SMMapObjDef>();
    for (const auto &object : doodads)
        Require(object.m_nameId < modelOffsets.size() && Finite(object.m_position) && Finite(object.m_rotation),
                "invalid doodad placement");
    for (const auto &object : objects)
        Require(object.m_nameId < wmoOffsets.size() && Finite(object.m_position) && Finite(object.m_rotation) &&
                    Finite(object.m_extents.b) && Finite(object.m_extents.t),
                "invalid WMO placement");
    if (header.m_flags & 1) {
        const auto bounds = table(header.m_mfbo, 0x4D46424F, true);
        Require(bounds.size == sizeof(flyingBounds), "invalid flying bounds");
        std::memcpy(flyingBounds.data(), bounds.data, bounds.size);
    }
    if (header.m_mtxf) {
        textureFlags = table(header.m_mtxf, 0x4D545846).Records<int32_t>();
        Require(textureFlags.size() == textureCount, "texture flags count mismatch");
    }
    if (header.m_mh2o) {
        const auto water = table(header.m_mh2o, 0x4D48324F);
        if (water.size) {
            liquid = std::make_unique<Liquid::CAreaData>(nullptr);
            liquid->SetData(water.data, water.size);
        }
    }
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto &entry = chunks[i];
        const auto found = tokens.find(entry.m_offset);
        Require(found != tokens.end() && found->second == 0x4D434E4B, "MCIN points outside terrain chunks");
        const auto body = file.Block(entry.m_offset, 0x4D434E4B, 128);
        Require(entry.m_size == body.size + 8 && body.Value<uint32_t>(4) == i % 16 && body.Value<uint32_t>(8) == i / 16,
                "MCIN size or coordinates mismatch");
        const auto chunk = file.Slice(entry.m_offset, entry.m_size);
        const uint32_t doodadCount = body.Value<uint32_t>(0x10), wmoCount = body.Value<uint32_t>(0x38);
        const uint64_t refCount = uint64_t(doodadCount) + wmoCount;
        if (refCount) {
            Require(refCount <= body.size / 4, "object reference count exceeds chunk");
            const auto refs = chunk.Block(body.Value<uint32_t>(0x20), 0x4D435246, size_t(refCount) * 4);
            for (uint32_t j = 0; j < doodadCount; ++j)
                Require(refs.Value<uint32_t>(j * 4) < doodads.size(), "doodad reference out of range");
            for (uint32_t j = 0; j < wmoCount; ++j)
                Require(refs.Value<uint32_t>((size_t(doodadCount) + j) * 4) < objects.size(), "WMO reference out of range");
        }
        const auto layerCount = body.Value<uint32_t>(0xC);
        Require(layerCount <= 4, "too many terrain layers");
        if (layerCount) {
            const auto layers = chunk.Block(body.Value<uint32_t>(0x1C), 0x4D434C59, layerCount * 16);
            for (uint32_t j = 0; j < layerCount; ++j)
                Require(layers.Value<uint32_t>(j * 16) < textureCount, "terrain texture reference out of range");
        }
    }
}
