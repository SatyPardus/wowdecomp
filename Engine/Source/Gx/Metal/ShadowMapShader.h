#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
// The original 90 vertex handles contain three distinct programs. Lighting
// and shadow-receiver bits do not change the shadow-map generation pass.
constexpr uint32_t ShadowMapSkinning(uint32_t permutation) { return (permutation % 30) / 10; }
void ValidateShadowMapShader(const std::vector<uint8_t>& bytes, EGxShTarget target);
} // namespace GxMetal
