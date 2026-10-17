#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
// Diffuse_T1 / Combiners_Opaque, inspected from the original D3D9 SM3 assets.
// Shadow permutations retain handles but are rejected when selected for drawing.
void ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target);
bool SupportsModelPermutation(EGxShTarget target, uint32_t permutation);
}
