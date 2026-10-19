#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
constexpr bool SupportsDetailDoodadPermutation(EGxShTarget target, uint32_t permutation) {
    return target == GxSh_Vertex ? permutation < 6 : permutation < 8;
}
// Sun/three-light geometry and all original R32F/native PCF receivers.
void ValidateDetailDoodadShader(const std::vector<uint8_t>& bytes, EGxShTarget target);
} // namespace GxMetal
