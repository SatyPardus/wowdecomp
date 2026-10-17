#pragma once
#include "WoW/Common/TerrainGeometry.h"
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace Terrain {
struct Layer {
    uint32_t textureID, flags, alphaOffset, effectID;
};
struct SurfaceChunk {
    uint32_t flags = 0, x = 0, y = 0, areaID = 0;
    uint16_t holes = 0;
    C3Vector origin;
    std::array<float, 145> heights{};
    std::array<int8_t, 435> normals{};
    std::array<uint32_t, 145> colors{};
    bool hasColors = false;
    std::vector<Layer> layers;
    std::vector<uint8_t> alpha, shadow;
};
// The terrain surface only. WMO/doodad placement, liquids and sound emitters
// retain their own WorldClient loaders and are not represented by this view.
struct Surface {
    uint8_t alphaMapShift = 0; // Original MHDR MAMP value: dimensions are 64 >> shift.
    bool bigAlpha = false;
    std::vector<std::string> textures;
    std::vector<uint32_t> textureFlags;
    std::vector<SurfaceChunk> chunks; // 16*y+x, as in the original MCIN table.
};
// Does not modify the archive buffer. ADT subchunks may be unaligned; numeric
// arrays are copied into their naturally aligned destination representation.
Surface ReadSurface(const void* bytes, size_t size, bool bigAlpha);
struct BlendTexture {
    unsigned side = 0;
    std::vector<CImVector> rgba;
    std::vector<uint16_t> packed;
};
// Runs the original unpacking, including edge replication and optional authored
// half-resolution selection. Keep the 4-bit path packed until Gx uploads it.
BlendTexture CreateBlendTexture(const Surface& surface, size_t chunk, bool halfResolution, bool shadows);
}
