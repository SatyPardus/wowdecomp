#ifndef ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#define ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <unordered_map>
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C44Matrix.h"

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

// Unlit geometry consumes the original packed Gx vertex layout directly. The
// caller supplies a transform to Metal clip space (z in [0,w]); it is not a
// replacement for lighting, skinning, fog or the game's programmable effects.
struct GeometryDraw {
    id<MTLBuffer> vertices = nil, indices = nil;
    size_t vertexOffset = 0, indexOffset = 0;
    EGxVertexBufferFormat format = GxVBF_PCT;
    CGxBatch batch{};
    C44Matrix clipFromModel{1.0f};
    bool screenAtDepth = false; // UI vertex permutation 1: clip.xy *= clip.w
    bool depthTest = false, depthWrite = false;
    uint32_t depthFunction = 0; // Gx: <=, ==, >=, <
    uint32_t culling = 0; // Gx: none, clockwise, counterclockwise
    bool scissorEnabled = false;
    MTLScissorRect scissor{};
    bool viewportEnabled = false;
    MTLViewport viewport{};
};

// Native texture/unlit geometry primitives. This is not yet a CGxDevice backend.
// Uploads and draws on this queue are ordered; command buffers retain staging
// resources until completion. Uploads do not block the CPU or mutate live images.
// Draw and its pipeline/sampler caches belong to the owning render thread.
// Geometry buffers must remain unchanged until the submitted command completes.
class TextureDevice {
public:
    explicit TextureDevice(id<MTLDevice> device);
    TextureDevice(const TextureDevice&) = delete;
    TextureDevice& operator=(const TextureDevice&) = delete;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLTexture> Upload(const BLP::Image& image, bool forceExpansion = false);
    void Draw(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const QuadParameters& parameters, const TextureDrawState& state, const MTLViewport* viewport = nullptr);
    void DrawUnlit(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const GeometryDraw& geometry, const QuadParameters& parameters, const TextureDrawState& state);
    bool SupportsBC() const;

private:
    id<MTLFunction> m_vertex;
    id<MTLFunction> m_fragment[2];
    id<MTLFunction> m_geometryVertex[2][4];
    std::unordered_map<uint32_t, id<MTLRenderPipelineState>> m_pipelines;
    std::unordered_map<uint32_t, id<MTLSamplerState>> m_samplers;
    std::unordered_map<uint32_t, id<MTLDepthStencilState>> m_depthStates;
    id<MTLRenderPipelineState> Pipeline(const TextureDrawState& state, EGxVertexBufferFormat format = GxVertexBufferFormats_Last,
        bool depthAttachment = false, bool screenAtDepth = false);
    id<MTLSamplerState> Sampler(const TextureDrawState& state);
};
}
#endif
