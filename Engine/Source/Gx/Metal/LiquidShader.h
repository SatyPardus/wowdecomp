#pragma once
#include "PostProcessShader.h"
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
constexpr bool IsLiquidEffect(FragmentEffect effect) {
    return effect >= FragmentEffect::LiquidWater && effect <= FragmentEffect::LiquidMagma;
}
constexpr unsigned LiquidTextureCount(FragmentEffect effect) { return effect == FragmentEffect::LiquidMagma ? 1 : 2; }
constexpr unsigned LiquidPermutationCount(FragmentEffect effect, EGxShTarget target) {
    return target == GxSh_Vertex && effect != FragmentEffect::LiquidMagma ? 4 : 1;
}
const char* LiquidShaderFamily(FragmentEffect effect, EGxShTarget target);
void ValidateLiquidShader(const std::vector<uint8_t>& bytes, FragmentEffect effect, EGxShTarget target);
}
