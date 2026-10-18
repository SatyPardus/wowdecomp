#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/Metal/PostProcessShader.h"

namespace GxMetal {
constexpr bool IsTerrainEffect(FragmentEffect effect) {
    return effect == FragmentEffect::Terrain || effect == FragmentEffect::Terrain2 || effect == FragmentEffect::Terrain3;
}
constexpr unsigned TerrainPermutationCount(EGxShTarget target, FragmentEffect effect) {
    return target == GxSh_Vertex ? 128 : effect == FragmentEffect::Terrain3 ? 96 : 32;
}
constexpr unsigned TerrainLayerCount(EGxShTarget target, uint32_t permutation) {
    return 1 + ((permutation >> (target == GxSh_Vertex ? 4 : 1)) & 3);
}
// Terrain3 interleaves three shadow modes inside each alpha/lighting group.
// Keep the common surface specialization separate from shadow selection.
constexpr uint32_t TerrainSurfacePermutation(FragmentEffect effect, uint32_t permutation) {
    return effect == FragmentEffect::Terrain3 ? (permutation % 8) + ((permutation / 24) % 2) * 8 + (permutation / 48) * 16 : permutation;
}
constexpr uint32_t TerrainShadowMode(FragmentEffect effect, uint32_t permutation) {
    return effect == FragmentEffect::Terrain ? 0 : effect == FragmentEffect::Terrain2 ? 1 : 1 + (permutation / 8) % 3;
}
constexpr bool SupportsTerrainPermutation(EGxShTarget target, uint32_t permutation, FragmentEffect effect = FragmentEffect::Terrain) {
    return permutation < TerrainPermutationCount(target, effect) && !(permutation & (target == GxSh_Vertex ? 2 : 1));
}
constexpr uint32_t TerrainTextureMask(uint32_t surfacePermutation, uint32_t shadowMode) {
    const auto layers = TerrainLayerCount(GxSh_Pixel, surfacePermutation);
    uint32_t mask = (1u << (layers + 1)) - 1;
    // Banded shadows replace baked shadow alpha. With one old-alpha layer the
    // blend map is entirely unused; original Terrain3 does not bind/sample it.
    if (shadowMode >= 2 && layers == 1 && !(surfacePermutation & 8))
        mask &= ~2u;
    if (shadowMode)
        mask |= 1u << 5;
    if (shadowMode >= 2)
        mask |= 7u << 6;
    return mask;
}
void ValidateTerrainShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect = FragmentEffect::Terrain);
} // namespace GxMetal
