#pragma once
#include "MapChunk.h"
#include "Types.h"
#include "WoW/Common/TerrainSurface.h"
#include <vector>

// Native runtime views own all MCNK fields, including numeric records that
// follow packed/variable-length alpha data. No repair writes reach file bytes.
class MapChunkData {
  public:
    MapChunkData(const void *bytes, size_t size, bool bigAlpha, uint8_t alphaMapShift);
    SMChunk header{};
    Terrain::SurfaceChunk surface;
    std::vector<SMLayer> layers;
    std::vector<uint32_t> references;
    std::vector<CWSoundEmitter> sounds;
    std::vector<SMLiquidChunk> liquids;
};
