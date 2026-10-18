#include "WoW/Common/TerrainSurface.h"
#include "WoW/Source/WorldClient/MapAreaData.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WowServices/ClientData.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
uint32_t Word(const std::vector<uint8_t>& bytes, size_t offset) {
    uint32_t word; std::memcpy(&word, bytes.data() + offset, 4); return word;
}
void Write(std::vector<uint8_t>& bytes, size_t offset, uint32_t word) { std::memcpy(bytes.data() + offset, &word, 4); }
void CheckAreaTables(const std::vector<uint8_t>& adt, std::ostream& report) {
    std::vector<uint8_t> unaligned(adt.size()+1);
    std::memcpy(unaligned.data()+1, adt.data(), adt.size());
    MapAreaData native(unaligned.data()+1, adt.size());
    Require(!std::memcmp(&native.header, adt.data()+20, 64), "ADT header bytes changed");
    auto compare = [&](const auto& records, uint32_t offset) {
        if (!offset) { Require(records.empty(), "Absent ADT table gained records"); return; }
        const auto size = Word(adt, size_t(offset)+24);
        Require(records.size()*sizeof(records[0]) == size, "ADT table record count changed");
        if (size) Require(!std::memcmp(records.data(), adt.data()+offset+28, size), "ADT table bytes changed");
    };
    compare(native.chunks, native.header.m_mcin);
    compare(native.textureNames, native.header.m_mtex);
    compare(native.modelNames, native.header.m_mmdx); compare(native.modelOffsets, native.header.m_mmid);
    compare(native.wmoNames, native.header.m_mwmo); compare(native.wmoOffsets, native.header.m_mwid);
    compare(native.doodads, native.header.m_mddf); compare(native.objects, native.header.m_modf);
    compare(native.textureFlags, native.header.m_mtxf);
    Require(reinterpret_cast<uintptr_t>(native.doodads.data()) % alignof(SMDoodadDef) == 0 &&
        reinterpret_cast<uintptr_t>(native.objects.data()) % alignof(SMMapObjDef) == 0, "ADT object tables are unaligned");
    std::fill(unaligned.begin(), unaligned.end(), 0);
    compare(native.doodads, native.header.m_mddf); compare(native.objects, native.header.m_modf);
    if (native.header.m_flags & 1)
        Require(!std::memcmp(native.flyingBounds.data(), adt.data()+native.header.m_mfbo+28, 36), "ADT flight bounds changed");
    auto reject = [&](std::vector<uint8_t> changed) {
        bool failed = false;
        try { MapAreaData invalid(changed.data(), changed.size()); } catch (const std::runtime_error&) { failed = true; }
        Require(failed, "Malformed ADT area table was accepted");
    };
    auto changed = adt; changed.pop_back(); reject(changed);
    changed = adt; Write(changed, 24, native.header.m_mcin+1); reject(changed);
    changed = adt; Write(changed, native.header.m_mcin+32, 0xFFFFFFFF); reject(changed);
    if (!native.modelOffsets.empty()) {
        changed = adt; Write(changed, native.header.m_mmid+28, uint32_t(native.modelNames.size())); reject(changed);
    }
    if (!native.doodads.empty()) {
        changed = adt; Write(changed, native.header.m_mddf+28, uint32_t(native.modelOffsets.size())); reject(changed);
    }
    if (!native.objects.empty()) {
        changed = adt; Write(changed, native.header.m_modf+28, uint32_t(native.wmoOffsets.size())); reject(changed);
    }
    for (const auto& chunk : native.chunks) if (Word(adt, chunk.m_offset+24)) {
        changed = adt;
        Write(changed, chunk.m_offset+Word(adt, chunk.m_offset+40)+8, uint32_t(native.doodads.size()));
        reject(changed); break;
    }
    report << "  ADT area: " << native.doodads.size() << " doodad placements, " << native.objects.size()
        << " WMO placements, " << native.modelOffsets.size()+native.wmoOffsets.size()
        << " filename references; aligned ownership and invalid-reference rejection passed\n";
}
CImVector BlendPixel(const Terrain::BlendTexture& image, unsigned x, unsigned y) {
    if (!image.rgba.empty()) return image.rgba[y * image.side + x];
    const auto value = image.packed[y * image.side + x];
    CImVector result;
    result.r = ((value >> 8) & 15) * 17; result.g = ((value >> 4) & 15) * 17;
    result.b = (value & 15) * 17; result.a = (value >> 12) * 17;
    return result;
}
void CheckBlend(const Terrain::Surface& surface, size_t index) {
    const auto& chunk = surface.chunks[index];
    const unsigned side = 64 >> surface.alphaMapShift;
    const auto full = Terrain::CreateBlendTexture(surface, index, false, true);
    const auto half = Terrain::CreateBlendTexture(surface, index, true, true);
    const auto noShadow = Terrain::CreateBlendTexture(surface, index, false, false);
    std::vector<uint8_t> alpha[3];
    for (auto& mask : alpha) mask.resize(side * side);
    for (size_t layer = 1; layer < chunk.layers.size(); ++layer) {
        const auto& entry = chunk.layers[layer];
        if (!(entry.flags & 0x100)) continue;
        auto& mask = alpha[layer - 1];
        size_t input = entry.alphaOffset, output = 0;
        if (entry.flags & 0x200) {
            while (output < mask.size()) {
                const auto control = chunk.alpha[input++];
                const size_t count = control & 127;
                if (control & 128) std::fill_n(mask.data() + output, count, chunk.alpha[input++]);
                else { std::copy_n(chunk.alpha.data() + input, count, mask.data() + output); input += count; }
                output += count;
            }
        } else for (size_t i = 0; i < mask.size(); ++i) {
            mask[i] = surface.bigAlpha ? chunk.alpha[input + i] : ((chunk.alpha[input + i / 2] >> ((i & 1) * 4)) & 15) * 17;
        }
    }
    for (unsigned y = 0; y < side; ++y) for (unsigned x = 0; x < side; ++x) {
        const unsigned sourceX = chunk.flags & 0x8000 ? x : std::min(x, side - 2);
        const unsigned sourceY = chunk.flags & 0x8000 ? y : std::min(y, side - 2);
        const unsigned i = sourceY * side + sourceX;
        const auto actual = BlendPixel(full, x, y);
        const uint8_t shadow = (chunk.flags & 1) && (chunk.shadow[i / 8] & (1 << (i & 7))) ? 0 : 255;
        Require(actual.r == alpha[0][i] && actual.g == alpha[1][i] && actual.b == alpha[2][i] && actual.a == shadow,
            "Original terrain blend differs from raw alpha/shadow data");
        const auto unshadowed = BlendPixel(noShadow, x, y);
        Require(unshadowed.r == actual.r && unshadowed.g == actual.g && unshadowed.b == actual.b && unshadowed.a == 255,
            "Terrain shadow switch changed material weights");
    }
    for (unsigned y = 0; y < half.side; ++y) for (unsigned x = 0; x < half.side; ++x) {
        const auto sourceX = (chunk.flags & 0x8000) && x == half.side - 1 ? side - 1 : x * 2;
        const auto sourceY = (chunk.flags & 0x8000) && y == half.side - 1 ? side - 1 : y * 2;
        const auto expected = BlendPixel(full, sourceX, sourceY), actual = BlendPixel(half, x, y);
        Require(!std::memcmp(&actual, &expected, 4), "Terrain half-resolution selection or edge changed");
    }
}
}

void CheckTerrainLiquids(const std::vector<uint8_t>& adt, std::ostream& report);

void CheckMapDatabase(const ClientData& client, std::ostream& report);

void CheckTerrainResources(const ClientData& client, std::ostream& report) {
    CheckMapDatabase(client, report);
    struct Case { const char* map; uint32_t x, y; };
    for (const auto test : {Case{"AhnQiraj", 26, 46}, Case{"Azeroth", 32, 48}, Case{"Northrend", 32, 32}, Case{"Expansion01", 19, 31}, Case{"Expansion01", 28, 37}}) {
        const std::string base = std::string("World\\Maps\\") + test.map + "\\" + test.map;
        const auto map = client.Read(base + ".wdt", 1024 * 1024, true);
        Require(map.bytes.size() >= 52 && Word(map.bytes, 12) == 0x4D504844, "Terrain WDT header missing");
        const bool bigAlpha = (Word(map.bytes, 20) & 4) != 0;
        const auto asset = client.Read(base + "_" + std::to_string(test.x) + "_" + std::to_string(test.y) + ".adt", 64 * 1024 * 1024, true);
        CheckAreaTables(asset.bytes, report);
        CheckTerrainLiquids(asset.bytes, report);
        // Exercise even an unaligned file base, in addition to the real ADT's
        // naturally unaligned inner chunks. The source must remain untouched.
        std::vector<uint8_t> unaligned(asset.bytes.size() + 1);
        std::memcpy(unaligned.data() + 1, asset.bytes.data(), asset.bytes.size());
        const auto surface = Terrain::ReadSurface(unaligned.data() + 1, asset.bytes.size(), bigAlpha);
        Require(!std::memcmp(unaligned.data() + 1, asset.bytes.data(), asset.bytes.size()), "Terrain loader modified archive bytes");
        size_t triangles = 0, layers = 0, colored = 0, compressed = 0;
        for (const auto& chunk : surface.chunks) {
            CheckBlend(surface, chunk.y * 16 + chunk.x);
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
            << " material layers, " << colored << " vertex-colored chunks, " << compressed << " compressed alpha maps; original packed/RGBA blend and shadow paths passed\n";
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
