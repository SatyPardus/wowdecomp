#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
constexpr unsigned TerrainLayerCount(EGxShTarget target, uint32_t permutation) {
    return 1 + ((permutation >> (target == GxSh_Vertex ? 4 : 1)) & 3);
}
constexpr bool SupportsTerrainPermutation(EGxShTarget target, uint32_t permutation) {
    return target == GxSh_Vertex ? permutation < 128 && !(permutation & 3) : permutation < 32 && !(permutation & 1);
}
// Terrain/Terrain1 without projected shadows or cube environment sampling.
// All other handles remain identifiable and fail explicitly at the draw boundary.
void ValidateTerrainShader(const std::vector<uint8_t>& bytes, EGxShTarget target);
}
