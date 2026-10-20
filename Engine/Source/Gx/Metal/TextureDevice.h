#ifndef ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#define ENGINE_SOURCE_GX_METAL_TEXTUREDEVICE_H
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <array>
#include <unordered_map>
#include "Engine/Source/Gx/Metal/ModelShader.h"
#include "Engine/Source/Gx/Metal/TerrainShader.h"
#include "Engine/Source/Gx/Metal/WmoShader.h"
#include "Engine/Source/Gx/Metal/LiquidShader.h"
#include "Engine/Source/Gx/Metal/ShadowMapShader.h"
#include "Engine/Source/Gx/Metal/DetailDoodadShader.h"
#include "Engine/Source/Gx/Metal/WaterRippleShader.h"
#include "Engine/Source/Gx/Metal/WeatherShader.h"
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C44Matrix.h"
class CGxGammaRamp;

namespace GxMetal {
// Typed native parameters for the original unlit texture-stage pipeline.
struct FixedUnlitVertexParameters {
    simd_float4x4 clipFromModel, viewFromModel;
    simd_float4 viewDepth, materialColor, fog;
    simd_float4 textureRows[4][3];
    simd_uint4 uvSet{}, projected{};
};
struct FixedUnlitFragmentParameters {
    simd_float4 fogColor, decodeScale[4];
    float alphaRef, depthOffset = 0, padding[2]{};
};
static_assert(sizeof(FixedUnlitVertexParameters) == 400 && sizeof(FixedUnlitFragmentParameters) == 96,
              "Native fixed-function constant layout");
struct FixedLight {
    simd_float4 positionOrDirection, ambient, diffuse, specular, attenuation;
};
// Kept separate so sky/weather/unlit draws do not upload lighting constants.
struct FixedNormalParameters { simd_float3x3 normalFromModel; };
struct FixedLightingParameters {
    simd_float4 emissive, specularPower;
    simd_uint4 materialSource{};
    FixedLight lights[4];
};
static_assert(sizeof(FixedLight) == 80 && sizeof(FixedLightingParameters) == 368 && sizeof(FixedNormalParameters) == 48,
              "Native fixed-function lighting layout");
struct WeatherParameters {
    simd_float4x4 clipFromCamera;
    simd_float4 camera, color, sizeFalloff, fog;
    simd_float4 corners[3], uvCorners[3], widths[3], lengths[3], blur[3];
    float age, tailTime, minimumSize, maximumSize;
    simd_float2 pixelToClip, padding;
};
static_assert(sizeof(WeatherParameters) == 400, "Native weather layout");
struct FixedUnlitDrawParameters {
    FixedUnlitVertexParameters vertex{};
    FixedUnlitFragmentParameters fragment{};
    uint32_t stages = 0, colorOps = 0, alphaOps = 0, coordinateSources = 0, cubeMask = 0;
    bool fog = false, pointSprite = false;
    bool lighting = false, normalizeNormals = false, specular = false;
    uint32_t lightMask = 0;
    FixedNormalParameters normals{};
    FixedLightingParameters lightingParameters{};
    WeatherVertex weather = WeatherVertex::None;
    WeatherParameters weatherParameters{};
};

struct QuadParameters {
    simd_float2 scale = {1, 1};
    float mipLevel = 0;
    uint32_t explicitMip = 0;
    simd_float4 tint = {1, 1, 1, 1};
    simd_float2 uvScale = {1, 1};
    simd_float2 uvOffset = {0, 0};
    float alphaRef = 0;
    float padding[3]{};
    simd_float4 decodeScale[4] = {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
};
static_assert(sizeof(QuadParameters) == 128, "Metal quad constant layout");

struct TextureDrawState {
    simd_float4 decodeScale = {1, 1, 1, 1};
    EGxBlend blend = GxBlend_Opaque;
    EGxTexFilter filter = GxTex_LinearMipLinear;
    EGxTexWrapMode wrapU = GxTex_Clamp, wrapV = GxTex_Clamp;
    uint32_t anisotropy = 1;
    uint32_t alphaRef = 0; // GxRs_AlphaRef, independent of blending mode
    uint32_t colorWrite = 15; // original Gx bit order: R=1, B=2, G=4, A=8
    float blendFactor = 1; // Gx render state Unk84, quantized to UNORM8
};

// View-space palette: three affine rows per bone, already composed with the
// original model/view transforms by Model2. Native typed constants replace the
// legacy register bank at the Gx boundary.
struct ModelAffine { simd_float4 x, y, z; };
struct ModelVertexParameters {
    simd_float4x4 projection;
    simd_float4 uv[4];
    simd_float4 sunDiffuse, sunAmbient, sunDirection;
    simd_float4 diffuse, emissive, fog;
    simd_float4 lightColor[4], lightPosition[4];
    simd_float4 attenuationConstant, attenuationLinear, attenuationQuadratic;
};
struct ModelFragmentParameters {
    simd_float4 fogColorAlphaRef;
    simd_float4 decodeScale[2];
    float stateAlphaRef, depthOffset = 0;
    float padding[2]{};
};
static_assert(sizeof(ModelAffine) == 48 && sizeof(ModelVertexParameters) == 400 && sizeof(ModelFragmentParameters) == 64,
    "Native model constant layout");
struct ShadowReceiverParameters {
    ModelAffine projection[4];
    simd_float4 distanceFade, lightDirection, offsets[8], decodeScale{1,1,1,1};
};
static_assert(sizeof(ShadowReceiverParameters) == 368, "Native shadow receiver layout");
// Gx/D3D9 programmable planes are already in homogeneous clip space.
struct ModelClipParameters {
    simd_float4 planes[6]{};
    uint32_t mask = 0, padding[3]{};
};
static_assert(sizeof(ModelClipParameters) == 112, "Native model clip-plane layout");
struct ModelDrawParameters {
    ModelVertexParameters vertex{};
    ModelClipParameters clip{};
    ModelFragmentParameters fragment{};
    std::array<ModelAffine, 75> bones{};
    uint32_t permutation = 0, boneCount = 1, shadowMode = 0;
    ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse;
    bool shaderAlphaTest = false, hardwareShadow = false;
    ShadowReceiverParameters shadow{};
};

struct ShadowMapVertexParameters {
    simd_float4x4 projection;
    ModelAffine lightFromView;
    simd_float4 uv[2];
};
struct ShadowMapFragmentParameters {
    float depthScale = 0, alphaRef = 0, stateAlphaRef = 0, padding = 0;
    simd_float4 decodeScale = {1, 1, 1, 1};
};
static_assert(sizeof(ShadowMapVertexParameters) == 144 && sizeof(ShadowMapFragmentParameters) == 32, "Native shadow-map layout");
struct ShadowMapDrawParameters {
    ShadowMapVertexParameters vertex{};
    ShadowMapFragmentParameters fragment{};
    std::array<ModelAffine, 75> bones{};
    uint32_t skinning = 0, boneCount = 1;
    bool cutout = false;
};

struct TerrainPointLight { simd_float4 position, color, attenuation; };
struct TerrainVertexParameters {
    simd_float4x4 worldView, projection;
    simd_float4 fog, texOffset[5], texScale[5];
    simd_float4 position, sunDirection, ambient, diffuse, specular;
    TerrainPointLight lights[3];
    simd_float4 shadowRows[12], environmentRotation[3];
};
struct TerrainFragmentParameters {
    simd_float4 fogColor, layerLighting, decodeScale[5], pcfOffsets[8];
    simd_float4 shadowDecodeScale = {1, 1, 1, 1};
    simd_float4 layerChannel{};
    float stateAlphaRef, shadowDistanceScale;
    float padding[2]{};
};
static_assert(sizeof(TerrainVertexParameters) == 768 && sizeof(TerrainFragmentParameters) == 288,
    "Native terrain constant layout");
struct TerrainDrawParameters {
    TerrainVertexParameters vertex{};
    TerrainFragmentParameters fragment{};
    uint32_t vertexPermutation = 0, pixelPermutation = 0, shadowMode = 0;
    TerrainLayerPass layerPass = TerrainLayerPass::Combined;
    bool hardwareShadow = false;
};

struct DetailDoodadVertexParameters {
    simd_float4x4 worldView, projection;
    simd_float4 fog, fade, sunDirection, ambient, diffuse;
    TerrainPointLight lights[3];
};
struct DetailDoodadFragmentParameters {
    simd_float4 fogColor, decodeScale;
    float stateAlphaRef;
    float padding[3]{};
};
static_assert(sizeof(DetailDoodadVertexParameters) == 352 && sizeof(DetailDoodadFragmentParameters) == 48, "Native detail-doodad layout");
struct DetailDoodadDrawParameters {
    DetailDoodadVertexParameters vertex{};
    DetailDoodadFragmentParameters fragment{};
    bool pointLights = false, hardwareShadow = false;
    uint32_t shadowMode = 0;
    ShadowReceiverParameters shadow{};
};

struct WmoVertexParameters {
    simd_float4x4 projection;
    ModelAffine worldView;
    simd_float4 ambient, sunDiffuse, sunDirection, specular, diffuse, emissive, fog;
    simd_float4 lightColor[4], lightPosition[4];
    simd_float4 attenuationConstant, attenuationLinear, attenuationQuadratic;
};
static_assert(sizeof(WmoVertexParameters) == 400, "Native WMO constant layout");
struct WmoDrawParameters {
    WmoVertexParameters vertex{};
    ModelFragmentParameters fragment{};
    uint32_t permutation = 0, shadowMode = 0;
    WmoVertexEffect vertexEffect = WmoVertexEffect::Diffuse;
    bool shaderAlphaTest = false, hardwareShadow = false;
    ShadowReceiverParameters shadow{};
};

struct LiquidUVTransform { simd_float4 x, y, translation; };
struct LiquidPointLight { simd_float4 position, color, attenuation; };
struct LiquidVertexParameters {
    simd_float4x4 projection, worldView;
    LiquidUVTransform textureTransform[2];
    simd_float4 fog, sunDirection, ambient, diffuse, specular;
    LiquidPointLight lights[3];
};
struct LiquidFragmentParameters {
    simd_float4 fogColor, decodeScale[2];
    float stateAlphaRef;
    float padding[3]{};
};
static_assert(sizeof(LiquidVertexParameters) == 448 && sizeof(LiquidFragmentParameters) == 64, "Native liquid constant layout");
// ProcWater's mounted programs use waves only to construct the shading basis;
// they neither displace geometry nor consume the four local-light variants.
struct ProceduralWaterVertexParameters {
    simd_float4x4 projection, worldView;
    LiquidUVTransform textureTransform[6];
    simd_float4 fog, radialPhase, planarPhase;
    simd_float4 radialWaves[3], radialFalloff, planarFalloff;
    simd_float4 planarWaves[3], planarFrequency, planarAmplitude;
};
struct ProceduralWaterFragmentParameters {
    simd_float4 fogColor, cameraPosition, sunDirection, specular;
    simd_float4 optical, fresnel, decodeScale[6];
    float stateAlphaRef;
    float padding[3]{};
};
static_assert(sizeof(ProceduralWaterVertexParameters) == 624 && sizeof(ProceduralWaterFragmentParameters) == 208,
              "Native procedural water constant layout");
struct LiquidDrawParameters {
    LiquidVertexParameters vertex{};
    LiquidFragmentParameters fragment{};
    ProceduralWaterVertexParameters proceduralVertex{};
    ProceduralWaterFragmentParameters proceduralFragment{};
    uint32_t lightCount = 0;
};

// Unlit geometry consumes the original packed Gx vertex layout directly. The
// caller supplies a transform to Metal clip space (z in [0,w]); it is not a
// replacement for lighting, skinning, fog or the game's programmable effects.
struct FogEffectParameters { simd_float4 density{}, minimum{}; };
static_assert(sizeof(FogEffectParameters) == 32, "Native fog effect constant layout");
struct NetherBlurParameters {
    float angle = 0, tapSpacing = 0, convergence = 1, padding = 0;
    simd_float2 pixelCenterOffset{};
    float padding2[2]{};
};
static_assert(sizeof(NetherBlurParameters) == 32, "Native nether blur constant layout");
struct GeometryDraw {
    id<MTLBuffer> vertices = nil, indices = nil;
    size_t vertexOffset = 0, indexOffset = 0;
    EGxVertexBufferFormat format = GxVBF_PCT;
    FragmentEffect effect = FragmentEffect::Unlit;
    CGxBatch batch{};
    const FixedUnlitDrawParameters* fixed = nullptr;
    const ModelDrawParameters* model = nullptr;
    const ShadowMapDrawParameters* shadow = nullptr;
    const DetailDoodadDrawParameters* detail = nullptr;
    const TerrainDrawParameters* terrain = nullptr;
    const WmoDrawParameters* wmo = nullptr;
    const LiquidDrawParameters* liquid = nullptr;
    C44Matrix clipFromModel{1.0f};
    simd_float2 waveSceneScale{}, waveBlurScale{};
    NetherBlurParameters netherBlur{};
    simd_float4 netherTintFade{};
    FogEffectParameters fogEffect{};
    bool signedWave = false; // BLS 0/1: direct sample; 2/3: UNORM remap (SM2/3 may bind BGRA8 to either)
    bool screenAtDepth = false; // UI vertex permutation 1: clip.xy *= clip.w
    float depthOffset = 0; // normalized post-raster depth offset (fixed unlit / water ripples / M2)
    bool depthTest = false, depthWrite = false;
    uint32_t depthFunction = 0; // Gx: <=, ==, >=, <
    uint32_t culling = 0; // Gx: none, clockwise, counterclockwise
    bool wireframe = false;
    bool integerPixelCenters = false; // Gx/D3D9 screen grid; raw Metal draws keep half-integer centers
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
    id<MTLTexture> UploadCube(const std::array<BLP::Image, 6>& faces, bool forceExpansion = false);
    void Draw(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const QuadParameters& parameters, const TextureDrawState& state, const MTLViewport* viewport = nullptr);
    // A supplied encoder remains open and must belong to the given pass/command.
    // Without one, these helpers encode an independent complete pass.
    void DrawFixedUnlit(id<MTLCommandBuffer>, MTLRenderPassDescriptor*, const std::array<id<MTLTexture>, 9>&,
        const GeometryDraw&, const std::array<TextureDrawState, 9>&, id<MTLRenderCommandEncoder> = nil);
    void DrawUnlit(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const GeometryDraw& geometry, const QuadParameters& parameters, const TextureDrawState& state, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawDetailDoodad(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawWaterRipple(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const GeometryDraw& geometry, const TextureDrawState& state, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawShadowMap(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
        const GeometryDraw& geometry, const TextureDrawState& state, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawModel(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawLiquid(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawWmo(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawTerrain(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawFilter4(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 4>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 4>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawGlow(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 2>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 2>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawDeath(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 2>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 2>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawFogEffect(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 4>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 4>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawNether(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 4>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 4>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void DrawGlowWave(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 3>& textures,
        const GeometryDraw& geometry, const std::array<TextureDrawState, 3>& states, id<MTLRenderCommandEncoder> encoder = nil);
    void ClearRegion(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const MTLScissorRect& region,
        uint32_t mask, simd_float4 color, id<MTLRenderCommandEncoder> encoder = nil);
    // Presentation-only lookup: the original BGRA8 game backbuffer is retained
    // for screenshots, copies and subsequent loads. No desktop gamma changes.
    void PresentGamma(id<MTLCommandBuffer>, id<MTLTexture> source, id<MTLTexture> destination, const CGxGammaRamp&);
    bool SupportsBC() const;
    bool SupportsPacked16() const;
    simd_float4 DecodeScale(BLP::Format format) const;

private:
    id<MTLTexture> UploadFaces(const BLP::Image* faces, size_t count, bool forceExpansion);
    id<MTLLibrary> m_library;
    id<MTLFunction> m_rippleVertex;
    id<MTLFunction> m_rippleFragment[2][2][3]{};
    id<MTLFunction> m_detailVertex[2][3]{};
    id<MTLFunction> m_detailFragment[2][2][4][2]{};
    id<MTLFunction> m_shadowVertex[3][2]{};
    id<MTLFunction> m_shadowFragment[2][2][2]{};
    id<MTLFunction> m_liquidVertex[4][4]{};
    id<MTLFunction> m_liquidFragment[4][2][2]{};
    id<MTLFunction> m_wmoVertex[7][10][3]{};
    id<MTLFunction> m_wmoFragment[7][2][2][2][4][2]{};
    id<MTLFunction> m_terrainVertex[128]{};
    id<MTLFunction> m_terrainShadowFragment;
    id<MTLFunction> m_terrainFragment[4][4][32][2][2][2]{};
    id<MTLFunction> m_modelVertex[unsigned(ModelVertexEffect::Count)][90][2]{};
    id<MTLFunction> m_modelFragment[23][2][2][2][4][2][3]{};
    id<MTLFunction> m_decodedFragments[5][2][2]{};
    id<MTLFunction> m_vertex;
    id<MTLFunction> m_fragment[2];
    id<MTLFunction> m_desaturateFragment[2][2]{};
    id<MTLFunction> m_deathFragment[2][2]{};
    id<MTLFunction> m_fogEffectFragment[2][2][2]{};
    id<MTLFunction> m_netherVertex;
    id<MTLFunction> m_netherFragment[2][2][2]{};
    id<MTLFunction> m_geometryVertex[2][4];
    id<MTLFunction> m_filterVertex, m_filterFragment[2][2];
    id<MTLFunction> m_glowVertex, m_glowFragment[2];
    id<MTLFunction> m_waveVertex, m_waveFragment[2][2];
    id<MTLFunction> m_clearVertex, m_clearFragment;
    id<MTLRenderPipelineState> m_gammaPipeline;
    std::unordered_map<uint32_t, id<MTLRenderPipelineState>> m_clearPipelines;
    id<MTLDepthStencilState> m_clearDepth[2]{};
    // Independent fields avoid collisions as material families and variants grow.
    using PipelineKey = std::array<uint32_t, 11>;
    struct PipelineHash {
        size_t operator()(const PipelineKey& key) const {
            size_t result = 0;
            for (auto field : key) result ^= std::hash<uint32_t>{}(field) + size_t(0x9e3779b9) + (result << 6) + (result >> 2);
            return result;
        }
    };
    std::unordered_map<PipelineKey, id<MTLRenderPipelineState>, PipelineHash> m_pipelines;
    std::unordered_map<uint32_t, id<MTLSamplerState>> m_samplers;
    std::unordered_map<uint32_t, id<MTLDepthStencilState>> m_depthStates;
    id<MTLRenderPipelineState> Pipeline(const TextureDrawState& state, const GeometryDraw& geometry,
        bool depthAttachment = false, bool decodePacked = false, MTLPixelFormat colorFormat = MTLPixelFormatBGRA8Unorm,
        uint32_t sampleCount = 1);
    id<MTLSamplerState> Sampler(const TextureDrawState& state, bool compareDepth = false);
    void DrawGeometry(id<MTLCommandBuffer>, MTLRenderPassDescriptor*, const std::array<id<MTLTexture>, 9>&,
        const GeometryDraw&, const QuadParameters&, const std::array<TextureDrawState, 9>&, id<MTLRenderCommandEncoder>);
};
}
#endif
