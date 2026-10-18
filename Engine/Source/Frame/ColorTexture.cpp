#include "ColorTexture.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CImVector.h"

// VERIFY: a1ef70c7 auto
// OFFSET: 0x00599160
void EmblemTextureUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel,
                         void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command == GxTex_Latch) {
        texelStrideInBytes = 4 * width;
        texels = static_cast<TSGrowableArray<CImVector>*>(userArg)->m_data;
    }
}
