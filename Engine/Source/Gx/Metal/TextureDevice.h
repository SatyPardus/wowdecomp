#ifndef ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#define ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <unordered_map>
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
struct QuadParameters {
    simd_float2 scale = {1, 1};
    float mipLevel = 0;
    uint32_t explicitMip = 0;
    simd_float4 tint = {1, 1, 1, 1};
    simd_float2 uvScale = {1, 1};
    simd_float2 uvOffset = {0, 0};
    float alphaRef = 0;
    float padding[3]{};
};
static_assert(sizeof(QuadParameters) == 64, "Metal quad constant layout");

struct TextureDrawState {
    EGxBlend blend = GxBlend_Opaque;
    EGxTexFilter filter = GxTex_LinearMipLinear;
    EGxTexWrapMode wrapU = GxTex_Clamp, wrapV = GxTex_Clamp;
    uint32_t anisotropy = 1;
    uint32_t alphaRef = 0; // GxRs_AlphaRef, independent of blending mode
    uint32_t colorWrite = 15; // original Gx bit order: R=1, B=2, G=4, A=8
    float blendFactor = 1; // Gx render state Unk84, quantized to UNORM8
};

// First native texture/render primitive. This is not yet a CGxDevice backend.
// Uploads and draws on this queue are ordered; command buffers retain staging
// resources until completion. Uploads do not block the CPU or mutate live images.
// Draw and its pipeline/sampler caches belong to the owning render thread.
class TextureDevice {
public:
    explicit TextureDevice(id<MTLDevice> device);
    TextureDevice(const TextureDevice&) = delete;
    TextureDevice& operator=(const TextureDevice&) = delete;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLTexture> Upload(const BLP::Image& image, bool forceExpansion = false);
    void Draw(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const QuadParameters& parameters, const TextureDrawState& state);
    bool SupportsBC() const;

private:
    id<MTLFunction> m_vertex;
    id<MTLFunction> m_fragment[2];
    std::unordered_map<uint32_t, id<MTLRenderPipelineState>> m_pipelines;
    std::unordered_map<uint32_t, id<MTLSamplerState>> m_samplers;
    id<MTLRenderPipelineState> Pipeline(const TextureDrawState& state);
    id<MTLSamplerState> Sampler(const TextureDrawState& state);
};
}
#endif
