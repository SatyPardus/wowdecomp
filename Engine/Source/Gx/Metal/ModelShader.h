#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/Metal/PostProcessShader.h"

namespace GxMetal {
// Diffuse_T1 / Combiners_Opaque / Combiners_Mod, inspected from original SM3 assets.
// Shadow permutations retain handles but are rejected when selected for drawing.
void ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect);
bool SupportsModelPermutation(EGxShTarget target, uint32_t permutation);
}
