#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
void ValidateWaterRippleShader(const std::vector<uint8_t>& bytes, EGxShTarget target);
}
