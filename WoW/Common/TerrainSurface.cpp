#include "TerrainSurface.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(std::string("Invalid terrain surface: ") + message);
}
struct Bytes {
    const uint8_t* data = nullptr;
    size_t size = 0;
    Bytes Slice(size_t offset, size_t count) const {
        Require(offset <= size && count <= size - offset, "chunk exceeds its container");
        return {data + offset, count};
    }
    template<class T> T Value(size_t offset) const {
        T result;
        const auto bytes = Slice(offset, sizeof(T));
        std::memcpy(&result, bytes.data, sizeof(T));
        return result;
    }
    Bytes Block(size_t offset, uint32_t token, size_t required) const {
        Require(Value<uint32_t>(offset) == token, "unexpected chunk at header offset");
        const auto count = Value<uint32_t>(offset + 4);
        Require(count >= required, "short chunk payload");
        return Slice(offset + 8, count);
    }
};
void CheckAlpha(const Terrain::SurfaceChunk& chunk, bool bigAlpha, unsigned side) {
    for (const auto& layer : chunk.layers) {
        if (!(layer.flags & 0x100)) continue;
        Require(layer.alphaOffset <= chunk.alpha.size(), "alpha offset exceeds MCAL");
        size_t cursor = layer.alphaOffset;
        if (layer.flags & 0x200) {
            size_t decoded = 0;
            while (decoded < side * side) {
                Require(cursor < chunk.alpha.size(), "truncated compressed alpha");
                const auto control = chunk.alpha[cursor++];
                const size_t count = control & 0x7F;
                // Original decompression consumes one authored row at a time.
                Require(count <= side - decoded % side, "invalid compressed alpha row");
                const size_t input = control & 0x80 ? 1 : count;
                Require(input <= chunk.alpha.size() - cursor, "truncated compressed alpha run");
                cursor += input; decoded += count;
            }
        } else {
            Require((bigAlpha ? side * side : (side * side + 1) / 2) <= chunk.alpha.size() - cursor, "truncated alpha map");
        }
    }
}
Terrain::SurfaceChunk ReadChunk(Bytes bytes, bool bigAlpha, unsigned side) {
    const auto header = bytes.Slice(8, 128);
    Terrain::SurfaceChunk result;
    result.flags = header.Value<uint32_t>(0);
    result.x = header.Value<uint32_t>(4); result.y = header.Value<uint32_t>(8);
    Require(result.x < 16 && result.y < 16, "chunk coordinates out of range");
    result.areaID = header.Value<uint32_t>(0x34);
    result.holes = header.Value<uint16_t>(0x3C);
    result.origin = header.Value<C3Vector>(0x68);
    Require(std::isfinite(result.origin.x) && std::isfinite(result.origin.y) && std::isfinite(result.origin.z), "non-finite origin");
    const auto heights = bytes.Block(header.Value<uint32_t>(0x14), 0x4D435654, sizeof(result.heights));
    const auto normals = bytes.Block(header.Value<uint32_t>(0x18), 0x4D434E52, sizeof(result.normals));
    std::memcpy(result.heights.data(), heights.data, sizeof(result.heights));
    std::memcpy(result.normals.data(), normals.data, sizeof(result.normals));
    for (float height : result.heights) Require(std::isfinite(height), "non-finite height");
    const auto colorOffset = header.Value<uint32_t>(0x74);
    if (colorOffset) {
        const auto colors = bytes.Block(colorOffset, 0x4D434356, sizeof(result.colors));
        std::memcpy(result.colors.data(), colors.data, sizeof(result.colors));
        result.hasColors = true;
    }
    const auto layerCount = header.Value<uint32_t>(0xC);
    Require(layerCount <= 4, "more than four material layers");
    if (layerCount) {
        const auto layers = bytes.Block(header.Value<uint32_t>(0x1C), 0x4D434C59, layerCount * 16);
        for (uint32_t i = 0; i < layerCount; ++i) result.layers.push_back(layers.Value<Terrain::Layer>(i * 16));
    }
    const auto alphaSize = header.Value<uint32_t>(0x28);
    if (alphaSize) {
        Require(alphaSize >= 8, "short MCAL size in header");
        const auto offset = header.Value<uint32_t>(0x24);
        Require(bytes.Value<uint32_t>(offset) == 0x4D43414C, "missing MCAL");
        // The original loader uses this header size to repair the MCAL size.
        const auto alpha = bytes.Slice(offset + 8, alphaSize - 8);
        result.alpha.assign(alpha.data, alpha.data + alpha.size);
    }
    if (result.flags & 1) {
        const auto shadow = bytes.Block(header.Value<uint32_t>(0x2C), 0x4D435348, (side + 7) / 8 * side);
        result.shadow.assign(shadow.data, shadow.data + shadow.size);
    }
    CheckAlpha(result, bigAlpha, side);
    return result;
}
}

Terrain::Surface Terrain::ReadSurface(const void* input, size_t size, bool bigAlpha) {
    Require(input && size >= 12 && size <= 64 * 1024 * 1024, "invalid file size");
    static_assert(sizeof(Layer) == 16 && sizeof(C3Vector) == 12, "ADT scalar layout");
    Bytes file{static_cast<const uint8_t*>(input), size};
    Require(file.Block(0, 0x4D564552, 4).Value<uint32_t>(0) == 18, "unsupported ADT version");
    Bytes index, names, flags;
    std::array<bool, 256> seen{};
    std::array<size_t, 256> chunkOffsets{}, chunkSizes{};
    Surface result; result.chunks.resize(256);
    result.alphaMapShift = file.Block(12, 0x4D484452, 64).Value<uint8_t>(0x30);
    Require(result.alphaMapShift <= 6, "invalid alpha map dimension shift");
    const unsigned alphaSide = 64 >> result.alphaMapShift;
    for (size_t offset = 0; offset < size;) {
        const auto token = file.Value<uint32_t>(offset);
        const auto length = file.Value<uint32_t>(offset + 4);
        const auto payload = file.Slice(offset + 8, length);
        if (token == 0x4D43494E) { Require(!index.data, "duplicate MCIN"); index = payload; }
        if (token == 0x4D544558) { Require(!names.data, "duplicate MTEX"); names = payload; }
        if (token == 0x4D545846) { Require(!flags.data, "duplicate MTXF"); flags = payload; }
        if (token == 0x4D434E4B) {
            auto chunk = ReadChunk(file.Slice(offset, size_t(length) + 8), bigAlpha, alphaSide);
            const auto slot = chunk.y * 16 + chunk.x;
            Require(!seen[slot], "duplicate terrain coordinates");
            seen[slot] = true; chunkOffsets[slot] = offset; chunkSizes[slot] = size_t(length) + 8;
            result.chunks[slot] = std::move(chunk);
        }
        offset += size_t(length) + 8;
    }
    Require(index.size == 256 * 16 && names.data, "missing MCIN or MTEX");
    for (size_t cursor = 0; cursor < names.size;) {
        const auto* end = static_cast<const uint8_t*>(std::memchr(names.data + cursor, 0, names.size - cursor));
        Require(end && end != names.data + cursor, "invalid texture name");
        result.textures.emplace_back(reinterpret_cast<const char*>(names.data + cursor), size_t(end - names.data - cursor));
        cursor = size_t(end - names.data) + 1;
    }
    result.textureFlags.resize(result.textures.size());
    if (flags.data) {
        Require(flags.size == result.textures.size() * 4, "texture flags count mismatch");
        for (size_t i = 0; i < result.textureFlags.size(); ++i) result.textureFlags[i] = flags.Value<uint32_t>(i * 4);
    }
    for (size_t i = 0; i < 256; ++i) {
        Require(seen[i], "missing terrain chunk");
        const auto offset = index.Value<uint32_t>(i * 16);
        Require(offset == chunkOffsets[i] && index.Value<uint32_t>(i * 16 + 4) == chunkSizes[i], "MCIN extent or coordinate mismatch");
        for (const auto& layer : result.chunks[i].layers)
            Require(layer.textureID < result.textures.size(), "texture index out of range");
    }
    return result;
}
