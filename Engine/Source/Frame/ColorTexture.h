#pragma once
#include <cstdint>
#include "Engine/Source/Gx/Types.h"

// Shared by the original color selector and guild-tabard textures.
void EmblemTextureUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel,
                         void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
