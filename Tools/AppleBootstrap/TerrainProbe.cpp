#include "WoW/Common/TerrainSurface.h"
#include "WoW/Source/WowServices/ClientData.h"
#include <cmath>
#include <cstring>
#include <ostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
uint32_t Word(const std::vector<uint8_t>& bytes, size_t offset) {
    uint32_t word; std::memcpy(&word, bytes.data() + offset, 4); return word;
}
void Write(std::vector<uint8_t>& bytes, size_t offset, uint32_t word) { std::memcpy(bytes.data() + offset, &word, 4); }
}

void CheckTerrainResources(const ClientData& client, std::ostream& report) {
    struct Case { const char* map; uint32_t x, y; };
    for (const auto test : {Case{"AhnQiraj", 26, 46}, Case{"Azeroth", 32, 48}, Case{"Northrend", 32, 32}}) {
        const std::string base = std::string("World\\Maps\\") + test.map + "\\" + test.map;
        const auto map = client.Read(base + ".wdt", 1024 * 1024, true);
        Require(map.bytes.size() >= 52 && Word(map.bytes, 12) == 0x4D504844, "Terrain WDT header missing");
        const bool bigAlpha = (Word(map.bytes, 20) & 4) != 0;
        const auto asset = client.Read(base + "_" + std::to_string(test.x) + "_" + std::to_string(test.y) + ".adt", 64 * 1024 * 1024, true);
        // Exercise even an unaligned file base, in addition to the real ADT's
        // naturally unaligned inner chunks. The source must remain untouched.
        std::vector<uint8_t> unaligned(asset.bytes.size() + 1);
        std::memcpy(unaligned.data() + 1, asset.bytes.data(), asset.bytes.size());
        const auto surface = Terrain::ReadSurface(unaligned.data() + 1, asset.bytes.size(), bigAlpha);
        Require(!std::memcmp(unaligned.data() + 1, asset.bytes.data(), asset.bytes.size()), "Terrain loader modified archive bytes");
        size_t triangles = 0, layers = 0, colored = 0, compressed = 0;
        for (const auto& chunk : surface.chunks) {
            Terrain::GeometryView view{{int32_t(test.x * 16 + chunk.x), int32_t(test.y * 16 + chunk.y)}, chunk.origin.z,
                chunk.heights.data(), chunk.normals.data(), chunk.hasColors ? chunk.colors.data() : nullptr, false};
            std::array<CGxVertexPN, 145> vertices;
            std::array<CGxVertexPNC, 145> colors;
            std::array<uint16_t, 768> indices;
            Terrain::CreateVerticesWorld(vertices.data(), view);
            Terrain::CreateVerticesWorld(colors.data(), view);
            for (unsigned i = 0; i < 145; ++i) {
                const auto& v = vertices[i];
                Require(std::isfinite(v.m_position.x) && std::isfinite(v.m_position.y) && std::isfinite(v.m_position.z), "Non-finite terrain geometry");
                Require(!std::memcmp(&v, &colors[i], sizeof(v)), "Terrain PN/PNC geometry differs");
                const uint32_t expected = chunk.hasColors ? chunk.colors[i] : 0xFF7F7F7F;
                Require(!std::memcmp(&colors[i].m_color, &expected, 4), "Terrain vertex color changed");
            }
            Require(std::abs(vertices[0].m_position.x - chunk.origin.x) <= 0.002f &&
                std::abs(vertices[0].m_position.y - chunk.origin.y) <= 0.002f, "Terrain tile coordinates disagree with original header");
            const auto count = Terrain::CreateIndices(indices.data(), chunk.holes);
            Require(count % 3 == 0, "Incomplete terrain triangle");
            for (unsigned i = 0; i < count; ++i) Require(indices[i] < 145, "Terrain index exceeds its chunk");
            triangles += count / 3; layers += chunk.layers.size(); colored += chunk.hasColors;
            for (const auto& layer : chunk.layers) compressed += (layer.flags & 0x200) != 0;
        }
        report << "  terrain " << test.map << ": 256 aligned chunks, " << triangles << " triangles, " << layers
            << " material layers, " << colored << " vertex-colored chunks, " << compressed << " compressed alpha maps\n";
        auto reject = [&](const std::vector<uint8_t>& bytes) {
            bool failed = false;
            try { Terrain::ReadSurface(bytes.data(), bytes.size(), bigAlpha); } catch (const std::runtime_error&) { failed = true; }
            Require(failed, "Malformed terrain surface was accepted");
        };
        auto changed = asset.bytes; changed.pop_back(); reject(changed);
        changed = asset.bytes;
        size_t index = 0;
        while (Word(changed, index) != 0x4D43494E) index += Word(changed, index + 4) + 8;
        const auto first = Word(changed, index + 8);
        Write(changed, first + 8 + 4, 16); reject(changed);
        changed = asset.bytes; Write(changed, first + 8 + 0x14, uint32_t(changed.size())); reject(changed);
        changed = asset.bytes;
        const auto height = first + Word(changed, first + 8 + 0x14) + 8;
        Write(changed, height, 0x7FC00000); reject(changed);
    }
    std::array<uint16_t, 768> indices;
    Require(Terrain::CreateIndices(indices.data(), 0xFFFF) == 0 && Terrain::CreateIndices(indices.data(), 1) == 720,
        "Terrain hole mask coverage changed");
    const auto count = Terrain::CreateIndices(indices.data(), 0, 145);
    Require(count == 768, "Terrain full chunk coverage changed");
    for (unsigned i = 0; i < count; ++i) Require(indices[i] >= 145 && indices[i] < 290, "Terrain batch base vertex changed");
}
