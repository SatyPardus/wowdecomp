#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
constexpr bool SupportsDetailDoodadPermutation(EGxShTarget target, uint32_t permutation) {
    return target == GxSh_Vertex ? permutation < 6 && permutation % 3 == 0 : permutation < 8 && permutation % 4 == 0;
}
// Sun-only and three-point-light geometry; shadow receivers remain explicit
// unsupported handles until their sampling and band behavior are implemented.
void ValidateDetailDoodadShader(const std::vector<uint8_t>& bytes, EGxShTarget target);
} // namespace GxMetal
