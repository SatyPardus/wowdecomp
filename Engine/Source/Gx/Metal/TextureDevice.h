#ifndef ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#define ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#import <Metal/Metal.h>
#include <simd/simd.h>
#include "Engine/Source/BLPFile/BlpImage.h"

namespace GxMetal {
struct QuadParameters {
    simd_float2 scale = {1, 1};
    float mipLevel = 0;
    uint32_t explicitMip = 0;
    simd_float4 tint = {1, 1, 1, 1};
};
static_assert(sizeof(QuadParameters) == 32, "Metal quad constant layout");

// First native texture/render primitive. This is not yet a CGxDevice backend.
// Uploads and draws on this queue are ordered; command buffers retain staging
// resources until completion. Uploads do not block the CPU or mutate live images.
class TextureDevice {
public:
    explicit TextureDevice(id<MTLDevice> device);
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLTexture> Upload(const BLP::Image& image, bool forceExpansion = false);
    void Draw(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const QuadParameters& parameters, bool blend, bool linear);
    bool SupportsBC() const;

private:
    id<MTLRenderPipelineState> m_opaque;
    id<MTLRenderPipelineState> m_alphaBlend;
    id<MTLSamplerState> m_nearest;
    id<MTLSamplerState> m_linear;
};
}
#endif
