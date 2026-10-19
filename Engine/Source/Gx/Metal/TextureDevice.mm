#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "MetalShaderSource.h"
#include "Engine/Source/Gx/Gx.h"
#include <TargetConditionals.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}
size_t Align(size_t value) { return (value + 255) & ~size_t(255); }
MTLPixelFormat PixelFormat(BLP::Format format) {
    if (format == BLP::Format::BGRA8) { return MTLPixelFormatBGRA8Unorm; }
    if (format == BLP::Format::RGB565) { return MTLPixelFormatB5G6R5Unorm; }
    if (format == BLP::Format::ARGB1555) { return MTLPixelFormatBGR5A1Unorm; }
    if (format == BLP::Format::ARGB4444) { return MTLPixelFormatABGR4Unorm; }
    if (@available(macOS 11.0, iOS 16.4, *)) {
        switch (format) {
            case BLP::Format::BC1: return MTLPixelFormatBC1_RGBA;
            case BLP::Format::BC2: return MTLPixelFormatBC2_RGBA;
            case BLP::Format::BC3: return MTLPixelFormatBC3_RGBA;
            default: break;
        }
    }
    throw std::runtime_error("Unsupported native texture format");
}
}

GxMetal::TextureDevice::TextureDevice(id<MTLDevice> selected) : device(selected) {
    Require(device != nil, "No Metal device available");
    queue = [device newCommandQueue];
    Require(queue != nil, "Cannot create Metal command queue");
    queue.label = @"WoW texture upload and draw";
    MTLCompileOptions* options = [MTLCompileOptions new];
    options.languageVersion = MTLLanguageVersion2_3;
    options.fastMathEnabled = NO;
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:@(TextureQuadSource) options:options error:&error];
    if (!library) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal shader compilation failed"); }
    m_library = library;
    m_vertex = [library newFunctionWithName:@"textureQuadVertex"];
    m_filterVertex = [library newFunctionWithName:@"filter4Vertex"];
    Require(m_filterVertex != nil, "Missing native FFX vertex function");
    m_glowVertex = [library newFunctionWithName:@"glowVertex"];
    Require(m_glowVertex != nil, "Missing native FFX glow vertex function");
    m_waveVertex = [library newFunctionWithName:@"glowWaveVertex"];
    Require(m_waveVertex != nil, "Missing native FFX wave vertex function");
    m_clearVertex = [library newFunctionWithName:@"clearVertex"];
    m_clearFragment = [library newFunctionWithName:@"clearFragment"];
    Require(m_clearVertex && m_clearFragment, "Missing native attachment clear functions");
    NSString* geometryFunctions[] = {@"unlitP", @"unlitPC", @"unlitPT", @"unlitPCT"};
    for (bool screenAtDepth : {false, true}) {
        MTLFunctionConstantValues* constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&screenAtDepth type:MTLDataTypeBool atIndex:1];
        for (unsigned i = 0; i < 4; ++i) {
            m_geometryVertex[screenAtDepth][i] = [library newFunctionWithName:geometryFunctions[i] constantValues:constants error:&error];
            if (!m_geometryVertex[screenAtDepth][i]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal geometry specialization failed"); }
        }
    }
    for (bool alphaTest : {false, true}) {
        MTLFunctionConstantValues* constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alphaTest type:MTLDataTypeBool atIndex:0];
        const bool decodePacked = false;
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        m_fragment[alphaTest] = [library newFunctionWithName:@"textureQuadFragment" constantValues:constants error:&error];
        if (!m_fragment[alphaTest]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal specialization failed"); }
        m_glowFragment[alphaTest] = [library newFunctionWithName:@"glowFragment" constantValues:constants error:&error];
        if (!m_glowFragment[alphaTest]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal glow specialization failed"); }
        for (bool signedWave : {false, true}) {
            [constants setConstantValue:&signedWave type:MTLDataTypeBool atIndex:3];
            m_waveFragment[signedWave][alphaTest] = [library newFunctionWithName:@"glowWaveFragment" constantValues:constants error:&error];
            if (!m_waveFragment[signedWave][alphaTest]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal wave specialization failed"); }
        }
        for (uint32_t filter = 1; filter <= 2; ++filter) {
            [constants setConstantValue:&filter type:MTLDataTypeUInt atIndex:2];
            m_filterFragment[filter - 1][alphaTest] = [library newFunctionWithName:@"filter4Fragment" constantValues:constants error:&error];
            if (!m_filterFragment[filter - 1][alphaTest]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal filter specialization failed"); }
        }
    }
    // Prepare common preview states at initialization. Other masks/states are
    // cached on demand; the full game will prewarm its material permutations.
    TextureDrawState state;
    GeometryDraw quad; quad.format = GxVertexBufferFormats_Last;
    Pipeline(state, quad); Sampler(state);
    state.blend = GxBlend_Alpha; state.alphaRef = 1;
    Pipeline(state, quad);
    quad.format = GxVBF_PCT; Pipeline(state, quad);
}

id<MTLRenderPipelineState> GxMetal::TextureDevice::Pipeline(const TextureDrawState& state, const GeometryDraw& geometry, bool depthAttachment, bool decodePacked, MTLPixelFormat colorFormat, uint32_t sampleCount) {
    const auto format = geometry.format; const auto effect = geometry.effect;
    const bool screenAtDepth = geometry.screenAtDepth, signedWave = geometry.signedWave, depthOffset = geometry.depthOffset != 0;
    const bool sampleDepthOffset = depthOffset && sampleCount > 1;
    const unsigned depthMode = depthOffset ? (sampleDepthOffset ? 2 : 1) : 0;
    const auto* fixed = geometry.fixed;
    const auto* model = geometry.model; const auto* terrain = geometry.terrain; const auto* wmo = geometry.wmo;
    const auto* liquid = geometry.liquid; const auto* shadow = geometry.shadow; const auto* detail = geometry.detail;
    const uint32_t modelPermutation = model ? model->permutation : 0;
    const uint32_t modelShadowMode = model ? model->shadowMode : 0;
    const bool shaderAlphaTest = model && model->shaderAlphaTest;
    const auto vertexEffect = model ? model->vertexEffect : ModelVertexEffect::Diffuse;
    Require(state.blend >= GxBlend_Opaque && state.blend < GxBlends_Last && state.colorWrite <= 15 && state.alphaRef <= 255,
        "Invalid Gx blend, color mask or alpha reference");
    Require(colorFormat == MTLPixelFormatBGRA8Unorm || colorFormat == MTLPixelFormatR32Float, "Unsupported pipeline color format");
    Require(colorFormat != MTLPixelFormatR32Float || state.blend < GxBlend_Alpha, "R32F blending is not implemented");
    Require(format >= GxVBF_P && format <= GxVertexBufferFormats_Last, "Invalid Gx vertex format");
    Require(!IsModelEffect(effect) || (SupportsModelPermutation(GxSh_Vertex, modelPermutation, vertexEffect) &&
        modelShadowMode < 4 && modelPermutation / 30 == std::min(modelShadowMode, 2u)),
        "Unsupported native model vertex permutation");
    const bool terrainShadow = effect == FragmentEffect::TerrainShadowMap;
    Require(!IsTerrainEffect(effect) ||
                (terrain && SupportsTerrainPermutation(GxSh_Vertex, terrain->vertexPermutation) &&
                 (terrainShadow ? bool(terrain->vertexPermutation & 1)
                                : SupportsTerrainPermutation(GxSh_Pixel, terrain->pixelPermutation) &&
                                      TerrainLayerCount(GxSh_Vertex, terrain->vertexPermutation) ==
                                          TerrainLayerCount(GxSh_Pixel, terrain->pixelPermutation) &&
                                      bool(terrain->vertexPermutation & 2) == bool(terrain->pixelPermutation & 1) &&
                                      terrain->shadowMode < 4 && uint32_t(terrain->layerPass) < 4 &&
                                      ((effect == FragmentEffect::Terrain0 || effect == FragmentEffect::Terrain0Environment) ==
                                       (terrain->layerPass != TerrainLayerPass::Combined)) &&
                                      (terrain->layerPass != TerrainLayerPass::Combined ||
                                       bool(terrain->vertexPermutation & 1) == bool(terrain->shadowMode)))),
            "Unsupported native terrain permutation or mismatched outputs");
    Require(!IsWmoEffect(effect) || (wmo && SupportsWmoPermutation(GxSh_Vertex, wmo->permutation) &&
        wmo->shadowMode < 4 && wmo->permutation / 30 == std::min(wmo->shadowMode,2u)), "Unsupported native WMO permutation");
    Require(!IsLiquidEffect(effect) || (liquid && liquid->lightCount < LiquidPermutationCount(effect, GxSh_Vertex)),
            "Unsupported native liquid permutation");
    Require(effect != FragmentEffect::ShadowMap || (shadow && shadow->skinning < 3), "Invalid shadow-map specialization");
    Require(effect != FragmentEffect::DetailDoodad || (detail && detail->shadowMode < 4), "Missing detail-doodad parameters");
    const bool modelClipping = model && model->clip.mask != 0;
    const uint32_t flags = uint32_t(state.alphaRef != 0) | (uint32_t(depthAttachment) << 1) | (uint32_t(screenAtDepth) << 2) |
                           (uint32_t(signedWave) << 3) | (uint32_t(decodePacked) << 4) | (uint32_t(shaderAlphaTest) << 5) |
                           (uint32_t(wmo && wmo->shaderAlphaTest) << 6) | (uint32_t(shadow && shadow->cutout) << 7) |
                           (uint32_t(depthOffset) << 8) | (uint32_t(modelClipping) << 9);
    const PipelineKey key{uint32_t(state.blend) | (state.colorWrite << 4), uint32_t(format), uint32_t(colorFormat), uint32_t(effect), flags,
        fixed ? fixed->positionCoords : wmo ? uint32_t(wmo->vertexEffect) : uint32_t(vertexEffect),
        terrain ? terrain->vertexPermutation : wmo ? wmo->permutation % 10 : liquid ? liquid->lightCount : shadow ? shadow->skinning : detail ? uint32_t(detail->pointLights) : modelPermutation,
        fixed ? fixed->colorOps : terrain ? terrain->pixelPermutation : 0,
        fixed ? fixed->alphaOps : terrain ? terrain->shadowMode : wmo ? wmo->shadowMode : detail ? detail->shadowMode : modelShadowMode,
        fixed ? fixed->stages | (uint32_t(fixed->fog) << 4) | (uint32_t(fixed->pointSprite) << 5) | (uint32_t(fixed->weather) << 6) : terrain ? uint32_t(terrain->layerPass) : 0, sampleCount};
    const auto found = m_pipelines.find(key);
    if (found != m_pipelines.end()) { return found->second; }
    // CGxDeviceD3d::s_srcBlend/s_dstBlend, preserving all 12 Gx modes.
    static const MTLBlendFactor source[] = {MTLBlendFactorOne, MTLBlendFactorOne, MTLBlendFactorSourceAlpha,
        MTLBlendFactorSourceAlpha, MTLBlendFactorDestinationColor, MTLBlendFactorDestinationColor,
        MTLBlendFactorDestinationColor, MTLBlendFactorOneMinusSourceAlpha, MTLBlendFactorOneMinusSourceAlpha,
        MTLBlendFactorSourceAlpha, MTLBlendFactorOne, MTLBlendFactorBlendAlpha};
    static const MTLBlendFactor destination[] = {MTLBlendFactorZero, MTLBlendFactorZero, MTLBlendFactorOneMinusSourceAlpha,
        MTLBlendFactorOne, MTLBlendFactorZero, MTLBlendFactorSourceColor, MTLBlendFactorOne, MTLBlendFactorOne,
        MTLBlendFactorZero, MTLBlendFactorZero, MTLBlendFactorOne, MTLBlendFactorOneMinusBlendAlpha};
    static_assert(sizeof(source) / sizeof(source[0]) == GxBlends_Last &&
        sizeof(destination) / sizeof(destination[0]) == GxBlends_Last, "Every Gx blend mode needs a native mapping");
    auto alphaFactor = [](MTLBlendFactor factor) {
        if (factor == MTLBlendFactorSourceColor) { return MTLBlendFactorSourceAlpha; }
        if (factor == MTLBlendFactorDestinationColor) { return MTLBlendFactorDestinationAlpha; }
        return factor;
    };
    MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.rasterSampleCount = sampleCount;
    descriptor.label = [NSString stringWithFormat:@"WoW Gx blend %u mask %u alpha %d", state.blend, state.colorWrite, state.alphaRef != 0];
    descriptor.vertexFunction = m_vertex;
    if (format != GxVertexBufferFormats_Last) {
        const bool color = GxVertexHasAttrib(format, GxVA_Color0), uv = GxVertexHasAttrib(format, GxVA_TexCoord0);
        descriptor.vertexFunction = m_geometryVertex[screenAtDepth][unsigned(color) | (unsigned(uv) << 1)];
        MTLVertexDescriptor* layout = [MTLVertexDescriptor vertexDescriptor];
        auto attribute = [&](unsigned index, EGxVertexAttrib gx, MTLVertexFormat type) {
            layout.attributes[index].bufferIndex = 1;
            layout.attributes[index].offset = GxVertexAttribOffset(format, gx);
            layout.attributes[index].format = type;
        };
        attribute(0, GxVA_Position, MTLVertexFormatFloat3);
        if (color) { attribute(1, GxVA_Color0, MTLVertexFormatUChar4Normalized_BGRA); }
        if (uv) { attribute(2, GxVA_TexCoord0, MTLVertexFormatFloat2); }
        layout.layouts[1].stride = GxVertexSize(format);
        descriptor.vertexDescriptor = layout;
    }
    if (depthAttachment) { descriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float; }
    descriptor.fragmentFunction = m_fragment[state.alphaRef != 0];
    if (effect == FragmentEffect::Box4 || effect == FragmentEffect::Gauss4 || effect == FragmentEffect::PropagateFog) {
        Require(!screenAtDepth,
            "Invalid native FFX filter pipeline");
        descriptor.vertexFunction = m_filterVertex;
        if (effect != FragmentEffect::PropagateFog) descriptor.fragmentFunction = m_filterFragment[uint32_t(effect) - 1][state.alphaRef != 0];
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        const MTLVertexFormat formats[] = {MTLVertexFormatFloat3, MTLVertexFormatUChar4Normalized_BGRA,
            MTLVertexFormatFloat2, MTLVertexFormatFloat2, MTLVertexFormatFloat2, MTLVertexFormatFloat2};
        const size_t offsets[] = {0, 12, 16, 24, 32, 40};
        for (unsigned i = 0; i < 6; ++i) {
            layout.attributes[i].bufferIndex = 1; layout.attributes[i].offset = offsets[i]; layout.attributes[i].format = formats[i];
        }
        layout.layouts[1].stride = 48; descriptor.vertexDescriptor = layout;
    }
    if (effect == FragmentEffect::Glow || effect == FragmentEffect::Death) {
        Require(format == GxVBF_PCT2 && !screenAtDepth, "Invalid native FFX scene/blur pipeline");
        descriptor.vertexFunction = m_glowVertex;
        descriptor.fragmentFunction = m_glowFragment[state.alphaRef != 0];
        descriptor.vertexDescriptor.attributes[3].bufferIndex = 1;
        descriptor.vertexDescriptor.attributes[3].offset = GxVertexAttribOffset(format, GxVA_TexCoord1);
        descriptor.vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
        if (effect == FragmentEffect::Death) {
            const bool alpha = state.alphaRef != 0;
            auto& fragment = m_deathFragment[alpha][decodePacked];
            if (!fragment) {
                auto constants = [MTLFunctionConstantValues new];
                [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
                [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
                NSError* error = nil;
                fragment = [m_library newFunctionWithName:@"deathFragment" constantValues:constants error:&error];
                if (!fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Death effect specialization failed");
            }
            descriptor.fragmentFunction = fragment;
        }
    }
    if (effect == FragmentEffect::GlowWave) {
        Require(format == GxVertexBufferFormats_Last && !screenAtDepth, "Invalid native FFX wave pipeline");
        descriptor.vertexFunction = m_waveVertex;
        descriptor.fragmentFunction = m_waveFragment[signedWave][state.alphaRef != 0];
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        const MTLVertexFormat formats[] = {MTLVertexFormatFloat3, MTLVertexFormatUChar4Normalized_BGRA,
            MTLVertexFormatFloat2, MTLVertexFormatFloat2, MTLVertexFormatFloat2};
        const size_t offsets[] = {0, 12, 16, 24, 32};
        for (unsigned i = 0; i < 5; ++i) {
            layout.attributes[i].bufferIndex = 1; layout.attributes[i].offset = offsets[i]; layout.attributes[i].format = formats[i];
        }
        layout.layouts[1].stride = 40; descriptor.vertexDescriptor = layout;
    }
    if (effect == FragmentEffect::PropagateFog || effect == FragmentEffect::FogCombine) {
        const bool propagate = effect == FragmentEffect::PropagateFog, alpha = state.alphaRef != 0;
        Require(!screenAtDepth && format == (propagate ? GxVertexBufferFormats_Last : GxVBF_PCT2), "Invalid fog effect vertex format");
        auto& fragment = m_fogEffectFragment[propagate][alpha][decodePacked];
        if (!fragment) {
            auto constants = [MTLFunctionConstantValues new];
            [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
            [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
            NSError* error = nil;
            fragment = [m_library newFunctionWithName:propagate ? @"propagateFogFragment" : @"fogCombineFragment" constantValues:constants error:&error];
            if (!fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Fog effect specialization failed");
        }
        descriptor.fragmentFunction = fragment;
        if (!propagate) {
            descriptor.vertexFunction = m_glowVertex;
            descriptor.vertexDescriptor.attributes[3].bufferIndex = 1;
            descriptor.vertexDescriptor.attributes[3].offset = GxVertexAttribOffset(format, GxVA_TexCoord1);
            descriptor.vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
        }
    }
    if (effect == FragmentEffect::NetherBlur || effect == FragmentEffect::NetherCombine) {
        const bool blur = effect == FragmentEffect::NetherBlur;
        Require(!screenAtDepth && format == (blur ? GxVBF_PCT : GxVBF_PCT2), "Invalid native nether vertex format");
        const bool alpha = state.alphaRef != 0;
        auto& fragment = m_netherFragment[blur][alpha][decodePacked];
        if (!fragment) {
            auto constants = [MTLFunctionConstantValues new];
            const unsigned kernel = 3;
            [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
            [constants setConstantValue:&kernel type:MTLDataTypeUInt atIndex:2];
            [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
            NSError* error = nil;
            fragment = [m_library newFunctionWithName:blur ? @"filter4Fragment" : @"netherCombineFragment" constantValues:constants error:&error];
            if (!fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Nether effect specialization failed");
        }
        descriptor.fragmentFunction = fragment;
        if (blur) {
            if (!m_netherVertex) m_netherVertex = [m_library newFunctionWithName:@"netherBlurVertex"];
            Require(m_netherVertex != nil, "Missing nether blur vertex function");
            descriptor.vertexFunction = m_netherVertex;
        } else {
            descriptor.vertexFunction = m_glowVertex;
            descriptor.vertexDescriptor.attributes[3].bufferIndex = 1;
            descriptor.vertexDescriptor.attributes[3].offset = GxVertexAttribOffset(format, GxVA_TexCoord1);
            descriptor.vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
        }
    }
    if (effect == FragmentEffect::Desaturate) {
        const bool alpha = state.alphaRef != 0;
        auto& fragment = m_desaturateFragment[alpha][decodePacked];
        if (!fragment) {
            auto constants = [MTLFunctionConstantValues new];
            [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
            [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
            NSError* error = nil;
            fragment = [m_library newFunctionWithName:@"desaturateFragment" constantValues:constants error:&error];
            if (!fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Desaturate specialization failed");
        }
        descriptor.fragmentFunction = fragment;
    }
    if (decodePacked && effect != FragmentEffect::PropagateFog && effect != FragmentEffect::FogCombine && effect != FragmentEffect::NetherBlur && effect != FragmentEffect::NetherCombine && effect != FragmentEffect::Death && effect != FragmentEffect::Desaturate && effect != FragmentEffect::FixedUnlit && effect != FragmentEffect::WaterRipple && effect != FragmentEffect::DetailDoodad && effect != FragmentEffect::ShadowMap && !IsModelEffect(effect) && !IsWmoEffect(effect) && !IsLiquidEffect(effect) && !IsTerrainEffect(effect)) {
        auto& fragment = m_decodedFragments[unsigned(effect)][state.alphaRef != 0][signedWave];
        if (!fragment) {
            const bool alpha = state.alphaRef != 0;
            const unsigned kernel = unsigned(effect);
            auto constants = [MTLFunctionConstantValues new];
            [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
            [constants setConstantValue:&kernel type:MTLDataTypeUInt atIndex:2];
            [constants setConstantValue:&signedWave type:MTLDataTypeBool atIndex:3];
            [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
            NSString* name = effect == FragmentEffect::Unlit ? @"textureQuadFragment" :
                effect == FragmentEffect::Glow ? @"glowFragment" : effect == FragmentEffect::GlowWave ? @"glowWaveFragment" : @"filter4Fragment";
            NSError* error = nil;
            fragment = [m_library newFunctionWithName:name constantValues:constants error:&error];
            if (!fragment) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Packed texture specialization failed"); }
        }
        descriptor.fragmentFunction = fragment;
    }
    if (effect == FragmentEffect::WaterRipple) {
        Require(!screenAtDepth && format == GxVBF_PCT, "Water ripples require original PCT vertices");
        const bool alpha = state.alphaRef != 0;
        auto constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&sampleDepthOffset type:MTLDataTypeBool atIndex:40];
        auto& fragment = m_rippleFragment[alpha][decodePacked][depthMode];
        NSError* error = nil;
        if (!m_rippleVertex) m_rippleVertex = [m_library newFunctionWithName:@"waterRippleVertex"];
        if (!fragment) fragment = [m_library newFunctionWithName:depthOffset ? @"waterRippleBiasedFragment" : @"waterRippleFragment"
            constantValues:constants error:&error];
        if (!m_rippleVertex || !fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Water-ripple specialization failed");
        descriptor.vertexFunction = m_rippleVertex; descriptor.fragmentFunction = fragment;
    }
    if (effect == FragmentEffect::DetailDoodad) {
        Require(!screenAtDepth && format == GxVBF_PNCT, "Detail-doodad rendering requires original PNCT vertices");
        const bool alpha = state.alphaRef != 0;
        auto constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&detail->pointLights type:MTLDataTypeBool atIndex:23];
        const uint32_t vertexShadow = std::min(detail->shadowMode, 2u);
        [constants setConstantValue:&vertexShadow type:MTLDataTypeUInt atIndex:37];
        auto& vertex = m_detailVertex[detail->pointLights][vertexShadow];
        auto& fragment = m_detailFragment[alpha][decodePacked][detail->shadowMode];
        NSError* error = nil;
        if (!vertex) vertex = [m_library newFunctionWithName:@"detailDoodadVertex" constantValues:constants error:&error];
        [constants setConstantValue:&detail->shadowMode type:MTLDataTypeUInt atIndex:37];
        if (!fragment) fragment = [m_library newFunctionWithName:@"detailDoodadFragment" constantValues:constants error:&error];
        if (!vertex || !fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Detail-doodad specialization failed");
        descriptor.vertexFunction = vertex; descriptor.fragmentFunction = fragment;
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        const EGxVertexAttrib attrs[] = {GxVA_Position, GxVA_Normal, GxVA_Color0, GxVA_TexCoord0};
        const MTLVertexFormat formats[] = {MTLVertexFormatFloat3, MTLVertexFormatFloat3, MTLVertexFormatUChar4Normalized_BGRA, MTLVertexFormatFloat2};
        for (unsigned i=0;i<4;++i) {
            layout.attributes[i].bufferIndex = 1; layout.attributes[i].offset = GxVertexAttribOffset(format, attrs[i]);
            layout.attributes[i].format = formats[i];
        }
        layout.layouts[1].stride = GxVertexSize(format); descriptor.vertexDescriptor = layout;
    }
    if (effect == FragmentEffect::ShadowMap) {
        Require(!screenAtDepth && (!shadow->cutout || GxVertexHasAttrib(format, GxVA_TexCoord0)) &&
            (!shadow->skinning || format == GxVBF_PBNT2), "Unsupported shadow-map vertex layout");
        const bool alpha = state.alphaRef != 0;
        auto constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&shadow->skinning type:MTLDataTypeUInt atIndex:21];
        [constants setConstantValue:&shadow->cutout type:MTLDataTypeBool atIndex:22];
        auto& vertex = m_shadowVertex[shadow->skinning][shadow->cutout];
        auto& fragment = m_shadowFragment[shadow->cutout][alpha][decodePacked];
        NSError* error = nil;
        if (!vertex) vertex = [m_library newFunctionWithName:shadow->skinning ? @"shadowMapSkinVertex" : @"shadowMapRigidVertex" constantValues:constants error:&error];
        if (!fragment) fragment = [m_library newFunctionWithName:@"shadowMapFragment" constantValues:constants error:&error];
        if (!vertex || !fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Shadow-map specialization failed");
        descriptor.vertexFunction = vertex; descriptor.fragmentFunction = fragment;
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        auto attribute = [&](unsigned index, EGxVertexAttrib gx, MTLVertexFormat type) {
            layout.attributes[index].bufferIndex = 1;
            layout.attributes[index].offset = GxVertexAttribOffset(format, gx);
            layout.attributes[index].format = type;
        };
        attribute(0, GxVA_Position, MTLVertexFormatFloat3);
        if (shadow->cutout) attribute(2, GxVA_TexCoord0, MTLVertexFormatFloat2);
        if (shadow->skinning) {
            attribute(3, GxVA_BlendWeight, MTLVertexFormatUChar4Normalized);
            attribute(4, GxVA_BlendIndices, MTLVertexFormatUChar4);
        }
        layout.layouts[1].stride = GxVertexSize(format); descriptor.vertexDescriptor = layout;
    }
    if (IsModelEffect(effect)) {
        const uint32_t basePermutation = modelPermutation % 30;
        Require(!modelShadowMode || ModelShadowMaterial(effect), "Unsupported model shadow material");
        const bool material = UsesModelMaterial(vertexEffect);
        const bool reflectionOnly = vertexEffect == ModelVertexEffect::DiffuseReflection;
        const bool environment = vertexEffect == ModelVertexEffect::DiffuseEnvironment || reflectionOnly;
        const bool dualUV = vertexEffect == ModelVertexEffect::DiffuseDualUV;
        const bool projected = UsesModelProjection(vertexEffect);
        const uint32_t colorMode = ModelColorMode(vertexEffect);
        const bool secondTexture = ModelTextureCount(effect) == 2;
        const uint32_t layerOperation = unsigned(ModelLayer(effect));
        const bool normal = (material && (!projected || modelShadowMode || (modelPermutation & 1))) ||
            (colorMode == 2 && (modelShadowMode || (modelPermutation & 1)));
        const bool color = !material && !(colorMode == 1 && (modelPermutation & 1));
        Require(SupportsModelPermutation(GxSh_Vertex, modelPermutation, vertexEffect) && !screenAtDepth && (!secondTexture || (!reflectionOnly && (environment || dualUV || projected))) &&
            (basePermutation < 10 || format == GxVBF_PBNT2) &&
            (!normal || GxVertexHasAttrib(format, GxVA_Normal)) &&
            (!color || GxVertexHasAttrib(format, GxVA_Color0)) && (projected || reflectionOnly || GxVertexHasAttrib(format, GxVA_TexCoord0)) &&
            (!dualUV || GxVertexHasAttrib(format, GxVA_TexCoord1)),
            "Unsupported native model vertex layout or permutation");
        auto& vertex = m_modelVertex[unsigned(vertexEffect)][modelPermutation][modelClipping];
        const bool textureAlpha = effect == FragmentEffect::ModelMod || effect == FragmentEffect::ModelModAddNA || effect == FragmentEffect::ModelModMod || effect == FragmentEffect::ModelModAdd || effect == FragmentEffect::ModelModMod2x;
        auto& fragment = m_modelFragment[unsigned(effect) - unsigned(FragmentEffect::ModelOpaque)][shaderAlphaTest][state.alphaRef != 0][decodePacked][modelShadowMode][depthMode];
        auto constants = [MTLFunctionConstantValues new];
        const bool alpha = state.alphaRef != 0;
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&basePermutation type:MTLDataTypeUInt atIndex:5];
        [constants setConstantValue:&shaderAlphaTest type:MTLDataTypeBool atIndex:6];
        [constants setConstantValue:&textureAlpha type:MTLDataTypeBool atIndex:7];
        [constants setConstantValue:&colorMode type:MTLDataTypeUInt atIndex:8];
        [constants setConstantValue:&environment type:MTLDataTypeBool atIndex:9];
        [constants setConstantValue:&layerOperation type:MTLDataTypeUInt atIndex:10];
        [constants setConstantValue:&dualUV type:MTLDataTypeBool atIndex:11];
        [constants setConstantValue:&projected type:MTLDataTypeBool atIndex:20];
        [constants setConstantValue:&reflectionOnly type:MTLDataTypeBool atIndex:33];
        [constants setConstantValue:&modelShadowMode type:MTLDataTypeUInt atIndex:35];
        [constants setConstantValue:&modelClipping type:MTLDataTypeBool atIndex:38];
        [constants setConstantValue:&depthOffset type:MTLDataTypeBool atIndex:39];
        [constants setConstantValue:&sampleDepthOffset type:MTLDataTypeBool atIndex:40];
        NSError* error = nil;
        NSString* vertexName = material ? (basePermutation < 10 ? @"modelRigidVertex" : @"modelSkinVertex") :
            normal ? @"modelColorLitVertex" : color ? @"modelColorVertex" : @"modelWhiteVertex";
        if (projected) vertexName = basePermutation >= 10 ? @"modelProjectedSkinVertex" :
            normal ? @"modelProjectedNormalVertex" : color ? @"modelProjectedColorVertex" : @"modelProjectedPositionVertex";
        if (reflectionOnly) vertexName = basePermutation < 10 ? @"modelProjectedNormalVertex" : @"modelProjectedSkinVertex";
        if (!vertex) vertex = [m_library newFunctionWithName:vertexName constantValues:constants error:&error];
        if (!fragment) fragment = [m_library newFunctionWithName:@"modelDiffuseFragment" constantValues:constants error:&error];
        if (!vertex || !fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Model specialization failed");
        descriptor.vertexFunction = vertex; descriptor.fragmentFunction = fragment;
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        auto attribute = [&](unsigned index, EGxVertexAttrib gx, MTLVertexFormat type) {
            layout.attributes[index].bufferIndex = 1;
            layout.attributes[index].offset = GxVertexAttribOffset(format, gx);
            layout.attributes[index].format = type;
        };
        attribute(0, GxVA_Position, MTLVertexFormatFloat3);
        if (normal) attribute(1, GxVA_Normal, MTLVertexFormatFloat3);
        if (color) attribute(5, GxVA_Color0, MTLVertexFormatUChar4Normalized_BGRA);
        if (!projected && !reflectionOnly) attribute(2, GxVA_TexCoord0, MTLVertexFormatFloat2);
        if (dualUV) attribute(6, GxVA_TexCoord1, MTLVertexFormatFloat2);
        if (basePermutation >= 10) {
            attribute(3, GxVA_BlendWeight, MTLVertexFormatUChar4Normalized);
            attribute(4, GxVA_BlendIndices, MTLVertexFormatUChar4);
        }
        layout.layouts[1].stride = GxVertexSize(format); descriptor.vertexDescriptor = layout;
    }
    if (IsTerrainEffect(effect)) {
        const uint32_t vertexPermutation = terrain->vertexPermutation, pixelPermutation = terrain->pixelPermutation;
        const bool color = (vertexPermutation & 4) != 0;
        Require(!screenAtDepth && GxVertexHasAttrib(format, GxVA_Normal) && (!color || GxVertexHasAttrib(format, GxVA_Color0)),
                "Unsupported native terrain vertex layout");
        auto& vertex = m_terrainVertex[vertexPermutation];
        auto& fragment =
            terrainShadow
                ? m_terrainShadowFragment
                : m_terrainFragment[uint32_t(terrain->layerPass)][terrain->shadowMode][pixelPermutation][state.alphaRef != 0][decodePacked];
        auto constants = [MTLFunctionConstantValues new];
        const bool alpha = state.alphaRef != 0;
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&vertexPermutation type:MTLDataTypeUInt atIndex:12];
        [constants setConstantValue:&pixelPermutation type:MTLDataTypeUInt atIndex:13];
        [constants setConstantValue:&terrain->shadowMode type:MTLDataTypeUInt atIndex:24];
        const uint32_t layerPass = uint32_t(terrain->layerPass);
        [constants setConstantValue:&layerPass type:MTLDataTypeUInt atIndex:25];
        NSError* error = nil;
        if (!vertex)
            vertex = [m_library newFunctionWithName:color ? @"terrainColorVertex" : @"terrainVertex" constantValues:constants error:&error];
        if (!fragment)
            fragment = [m_library newFunctionWithName:terrainShadow            ? @"terrainShadowMapFragment"
                                                      : (pixelPermutation & 1) ? @"terrainEnvironmentFragment"
                                                                               : @"terrainFragment"
                                       constantValues:constants
                                                error:&error];
        if (!vertex || !fragment)
            throw std::runtime_error(error.localizedDescription.UTF8String ?: "Terrain specialization failed");
        descriptor.vertexFunction = vertex;
        descriptor.fragmentFunction = fragment;
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        const EGxVertexAttrib attributes[] = {GxVA_Position, GxVA_Normal, GxVA_Color0};
        for (unsigned i = 0; i < (color ? 3u : 2u); ++i) {
            layout.attributes[i].bufferIndex = 1;
            layout.attributes[i].offset = GxVertexAttribOffset(format, attributes[i]);
            layout.attributes[i].format = i == 2 ? MTLVertexFormatUChar4Normalized_BGRA : MTLVertexFormatFloat3;
        }
        layout.layouts[1].stride = GxVertexSize(format); descriptor.vertexDescriptor = layout;
    }
    if (IsWmoEffect(effect)) {
        const uint32_t shadowMode = wmo->shadowMode;
        const uint32_t permutation = wmo->permutation % 10, family = unsigned(wmo->vertexEffect);
        const uint32_t material = unsigned(effect) - unsigned(FragmentEffect::WmoDiffuse);
        const bool composite = wmo->vertexEffect == WmoVertexEffect::Composite;
        const bool normal = shadowMode || (permutation & 1) || WmoEnvironment(wmo->vertexEffect) || composite;
        Require(family < 7 && !screenAtDepth && GxVertexHasAttrib(format, GxVA_Color0) && GxVertexHasAttrib(format, GxVA_TexCoord0) &&
            (!normal || GxVertexHasAttrib(format, GxVA_Normal)) &&
            (!composite || (GxVertexHasAttrib(format, GxVA_Color1) && GxVertexHasAttrib(format, GxVA_TexCoord1))) &&
            (WmoTextureCount(effect) == 1 || (effect == FragmentEffect::WmoComposite ? composite : WmoEnvironment(wmo->vertexEffect))),
            "Unsupported WMO vertex layout or shader pairing");
        auto& vertex = m_wmoVertex[family][permutation][std::min(shadowMode,2u)];
        auto& fragment = m_wmoFragment[material][wmo->shaderAlphaTest][state.alphaRef != 0][decodePacked][shadowMode];
        const bool alpha = state.alphaRef != 0;
        auto constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&permutation type:MTLDataTypeUInt atIndex:14];
        [constants setConstantValue:&family type:MTLDataTypeUInt atIndex:15];
        [constants setConstantValue:&material type:MTLDataTypeUInt atIndex:16];
        [constants setConstantValue:&wmo->shaderAlphaTest type:MTLDataTypeBool atIndex:17];
        [constants setConstantValue:&shadowMode type:MTLDataTypeUInt atIndex:36];
        NSError* error = nil;
        if (!vertex) vertex = [m_library newFunctionWithName:composite ? @"wmoCompositeVertex" : normal ? @"wmoLitVertex" : @"wmoVertex" constantValues:constants error:&error];
        if (!fragment) fragment = [m_library newFunctionWithName:@"wmoFragment" constantValues:constants error:&error];
        if (!vertex || !fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "WMO specialization failed");
        descriptor.vertexFunction = vertex; descriptor.fragmentFunction = fragment;
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        auto attribute = [&](unsigned index, EGxVertexAttrib gx, MTLVertexFormat type) {
            layout.attributes[index].bufferIndex = 1; layout.attributes[index].offset = GxVertexAttribOffset(format, gx); layout.attributes[index].format = type;
        };
        attribute(0, GxVA_Position, MTLVertexFormatFloat3);
        if (normal) attribute(1, GxVA_Normal, MTLVertexFormatFloat3);
        attribute(2, GxVA_Color0, MTLVertexFormatUChar4Normalized_BGRA);
        attribute(3, GxVA_TexCoord0, MTLVertexFormatFloat2);
        if (composite) { attribute(4, GxVA_Color1, MTLVertexFormatUChar4Normalized_BGRA); attribute(5, GxVA_TexCoord1, MTLVertexFormatFloat2); }
        layout.layouts[1].stride = GxVertexSize(format); descriptor.vertexDescriptor = layout;
    }
    if (IsLiquidEffect(effect)) {
        const uint32_t material = unsigned(effect) - unsigned(FragmentEffect::LiquidWater), lights = liquid->lightCount;
        const bool magma = effect == FragmentEffect::LiquidMagma;
        Require(!screenAtDepth && GxVertexHasAttrib(format, GxVA_Color0) && GxVertexHasAttrib(format, GxVA_TexCoord0) &&
            (magma || (GxVertexHasAttrib(format, GxVA_Normal) && GxVertexHasAttrib(format, GxVA_TexCoord1))), "Unsupported liquid vertex layout");
        auto& vertex = m_liquidVertex[material][lights];
        auto& fragment = m_liquidFragment[material][state.alphaRef != 0][decodePacked];
        const bool alpha = state.alphaRef != 0;
        auto constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&material type:MTLDataTypeUInt atIndex:18];
        [constants setConstantValue:&lights type:MTLDataTypeUInt atIndex:19];
        NSError* error = nil;
        if (!vertex) vertex = [m_library newFunctionWithName:magma ? @"liquidMagmaVertex" : @"liquidWaterVertex" constantValues:constants error:&error];
        if (!fragment) fragment = [m_library newFunctionWithName:@"liquidFragment" constantValues:constants error:&error];
        if (!vertex || !fragment) throw std::runtime_error(error.localizedDescription.UTF8String ?: "Liquid specialization failed");
        descriptor.vertexFunction = vertex; descriptor.fragmentFunction = fragment;
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        auto attribute = [&](unsigned index, EGxVertexAttrib gx, MTLVertexFormat type) {
            layout.attributes[index].bufferIndex = 1; layout.attributes[index].offset = GxVertexAttribOffset(format, gx); layout.attributes[index].format = type;
        };
        attribute(0, GxVA_Position, MTLVertexFormatFloat3);
        attribute(2, GxVA_Color0, MTLVertexFormatUChar4Normalized_BGRA);
        attribute(3, GxVA_TexCoord0, MTLVertexFormatFloat2);
        if (!magma) { attribute(1, GxVA_Normal, MTLVertexFormatFloat3); attribute(4, GxVA_TexCoord1, MTLVertexFormatFloat2); }
        layout.layouts[1].stride = GxVertexSize(format); descriptor.vertexDescriptor = layout;
    }
    if (effect == FragmentEffect::FixedUnlit) {
        Require(fixed && fixed->stages <= 4 && !screenAtDepth, "Invalid unlit fixed-function pipeline");
        const bool alpha = state.alphaRef != 0, color = GxVertexHasAttrib(format, GxVA_Color0),
                   uv0 = GxVertexHasAttrib(format, GxVA_TexCoord0), uv1 = GxVertexHasAttrib(format, GxVA_TexCoord1);
        auto constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alpha type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&decodePacked type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&fixed->stages type:MTLDataTypeUInt atIndex:26];
        [constants setConstantValue:&fixed->colorOps type:MTLDataTypeUInt atIndex:27];
        [constants setConstantValue:&fixed->alphaOps type:MTLDataTypeUInt atIndex:28];
        [constants setConstantValue:&fixed->fog type:MTLDataTypeBool atIndex:29];
        [constants setConstantValue:&fixed->pointSprite type:MTLDataTypeBool atIndex:41];
        const uint32_t weatherKind = uint32_t(fixed->weather);
        [constants setConstantValue:&weatherKind type:MTLDataTypeUInt atIndex:42];
        [constants setConstantValue:&color type:MTLDataTypeBool atIndex:30];
        [constants setConstantValue:&uv0 type:MTLDataTypeBool atIndex:31];
        [constants setConstantValue:&uv1 type:MTLDataTypeBool atIndex:32];
        [constants setConstantValue:&fixed->positionCoords type:MTLDataTypeUInt atIndex:34];
        [constants setConstantValue:&sampleDepthOffset type:MTLDataTypeBool atIndex:40];
        NSError* error = nil;
        descriptor.vertexFunction = [m_library newFunctionWithName:weatherKind ? @"weatherVertex" : @"fixedUnlitVertex" constantValues:constants error:&error];
        descriptor.fragmentFunction = [m_library newFunctionWithName:depthOffset ? @"fixedUnlitBiasedFragment" : @"fixedUnlitFragment" constantValues:constants error:&error];
        if (!descriptor.vertexFunction || !descriptor.fragmentFunction)
            throw std::runtime_error(error.localizedDescription.UTF8String ?: "Unlit fixed-function specialization failed");
        if (fixed->weather == WeatherVertex::Snow || fixed->weather == WeatherVertex::Sand)
            descriptor.vertexDescriptor.layouts[1].stepFunction = MTLVertexStepFunctionPerInstance;
        if (weatherKind && fixed->weather != WeatherVertex::Splash) {
            descriptor.vertexDescriptor.attributes[4].bufferIndex = 1;
            descriptor.vertexDescriptor.attributes[4].offset = GxVertexAttribOffset(format, GxVA_Normal);
            descriptor.vertexDescriptor.attributes[4].format = MTLVertexFormatFloat3;
        }
        if (uv1) {
            descriptor.vertexDescriptor.attributes[3].bufferIndex = 1;
            descriptor.vertexDescriptor.attributes[3].offset = GxVertexAttribOffset(format, GxVA_TexCoord1);
            descriptor.vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
        }
    }
    auto attachment = descriptor.colorAttachments[0];
    attachment.pixelFormat = colorFormat;
    attachment.blendingEnabled = state.blend >= GxBlend_Alpha;
    attachment.sourceRGBBlendFactor = source[state.blend];
    attachment.destinationRGBBlendFactor = destination[state.blend];
    attachment.sourceAlphaBlendFactor = alphaFactor(source[state.blend]);
    attachment.destinationAlphaBlendFactor = alphaFactor(destination[state.blend]);
    attachment.writeMask = MTLColorWriteMaskNone;
    if (state.colorWrite & 1) { attachment.writeMask |= MTLColorWriteMaskRed; }
    if (state.colorWrite & 2) { attachment.writeMask |= MTLColorWriteMaskBlue; }
    if (state.colorWrite & 4) { attachment.writeMask |= MTLColorWriteMaskGreen; }
    if (state.colorWrite & 8) { attachment.writeMask |= MTLColorWriteMaskAlpha; }
    NSError* error = nil;
    id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
    if (!pipeline) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal pipeline creation failed"); }
    m_pipelines.emplace(key, pipeline);
    return pipeline;
}

id<MTLSamplerState> GxMetal::TextureDevice::Sampler(const TextureDrawState& state) {
    Require(state.filter >= GxTex_Nearest && state.filter < GxTexFilters_Last && state.wrapU >= GxTex_Clamp && state.wrapU <= GxTex_Wrap &&
        state.wrapV >= GxTex_Clamp && state.wrapV <= GxTex_Wrap && state.anisotropy >= 1 && state.anisotropy <= 16, "Invalid Gx sampler state");
    const uint32_t anisotropy = state.filter == GxTex_Anisotropic ? state.anisotropy : 1;
    const uint32_t key = uint32_t(state.filter) | (uint32_t(state.wrapU) << 3) | (uint32_t(state.wrapV) << 4) | (anisotropy << 5);
    const auto found = m_samplers.find(key);
    if (found != m_samplers.end()) { return found->second; }
    MTLSamplerDescriptor* descriptor = [MTLSamplerDescriptor new];
    const bool nearest = state.filter == GxTex_Nearest || state.filter == GxTex_NearestMipNearest;
    descriptor.minFilter = descriptor.magFilter = nearest ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
    if (state.filter <= GxTex_Linear) { descriptor.mipFilter = MTLSamplerMipFilterNotMipmapped; }
    else if (state.filter <= GxTex_LinearMipNearest) { descriptor.mipFilter = MTLSamplerMipFilterNearest; }
    else { descriptor.mipFilter = MTLSamplerMipFilterLinear; }
    descriptor.sAddressMode = state.wrapU == GxTex_Wrap ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
    descriptor.tAddressMode = state.wrapV == GxTex_Wrap ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
    descriptor.maxAnisotropy = anisotropy;
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:descriptor];
    Require(sampler != nil, "Cannot create Metal sampler");
    m_samplers.emplace(key, sampler);
    return sampler;
}

bool GxMetal::TextureDevice::SupportsBC() const {
    if (@available(macOS 11.0, iOS 16.4, *)) { return device.supportsBCTextureCompression; }
    return false;
}

bool GxMetal::TextureDevice::SupportsPacked16() const {
#if TARGET_OS_SIMULATOR
    // The iOS simulator framework rejects these formats before consulting the
    // host device's Apple GPU family, even on Apple silicon.
    return false;
#else
    return [device supportsFamily:MTLGPUFamilyApple2];
#endif
}

simd_float4 GxMetal::TextureDevice::DecodeScale(BLP::Format format) const {
    if (!SupportsPacked16()) {
        if (format == BLP::Format::RGB565) { return {255.0f / 31, 255.0f / 63, 255.0f / 31, 1}; }
        if (format == BLP::Format::ARGB1555) { return {255.0f / 31, 255.0f / 31, 255.0f / 31, 255}; }
    }
    return {1, 1, 1, 1};
}

id<MTLTexture> GxMetal::TextureDevice::Upload(const BLP::Image& image, bool forceExpansion) {
    return UploadFaces(&image, 1, forceExpansion);
}
id<MTLTexture> GxMetal::TextureDevice::UploadCube(const std::array<BLP::Image, 6>& faces, bool forceExpansion) {
    return UploadFaces(faces.data(), faces.size(), forceExpansion);
}
id<MTLTexture> GxMetal::TextureDevice::UploadFaces(const BLP::Image* faces, size_t count, bool forceExpansion) {
    Require(faces && (count == 1 || count == 6) && !faces[0].mips.empty(), "Invalid texture faces");
    const auto& image = faces[0];
    const auto width = image.mips[0].width, height = image.mips[0].height;
    Require(width && height && width <= 16384 && height <= 16384 && (count == 1 || (width == height && !(width & (width - 1)))),
            "Invalid texture extent");
    size_t levels = 1;
    for (auto size = std::max(width, height); size > 1; size >>= 1)
        ++levels;
    Require(image.mips.size() <= levels, "Texture has excess mip levels");
    for (size_t face = 0; face < count; ++face) {
        Require(faces[face].format == image.format && faces[face].mips.size() == image.mips.size(), "Mismatched cube faces");
        for (size_t level = 0; level < image.mips.size(); ++level) {
            const auto& mip = faces[face].mips[level];
            const bool bc = BLP::IsBlockCompressed(image.format);
            const size_t block = bc ? 4 : 1, bytes = bc                                   ? (image.format == BLP::Format::BC1 ? 8 : 16)
                                                     : image.format == BLP::Format::BGRA8 ? 4
                                                                                          : 2;
            const size_t rowBytes = ((mip.width + block - 1) / block) * bytes;
            Require(mip.width == std::max(1u, width >> level) && mip.height == std::max(1u, height >> level) && mip.rowBytes == rowBytes &&
                        mip.bytes.size() == rowBytes * ((mip.height + block - 1) / block),
                    "Invalid texture mip payload");
        }
    }
    const bool compressed = BLP::IsBlockCompressed(image.format);
    const bool packed16 = !compressed && image.format != BLP::Format::BGRA8;
    const bool packedFallback = packed16 && !SupportsPacked16();
    const bool codedFallback = packedFallback && image.format != BLP::Format::ARGB4444;
    Require(!packed16 || !forceExpansion, "Packed 16-bit GPU textures must retain native UNORM precision");
    // Store UNORM5/6 integer codes exactly in filterable UNORM8, then apply
    // 255/(2^bits-1) in the shader. Scaling commutes with texture filtering;
    // there is no additional quantization to an approximate 8-bit color.
    // UNORM4 values instead have an exact full-range BGRA8 representation.
    const bool expanded = (compressed && (forceExpansion || !SupportsBC())) || (packedFallback && !codedFallback);
    const auto format = PixelFormat(expanded || codedFallback ? BLP::Format::BGRA8 : image.format);
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
        format width:image.mips[0].width height:image.mips[0].height mipmapped:NO];
    descriptor.mipmapLevelCount = image.mips.size();
    if (count == 6)
        descriptor.textureType = MTLTextureTypeCube;
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageShaderRead;
    struct Layout { size_t offset, stride, rows; };
    std::vector<Layout> layouts;
    size_t total = 0;
    for (size_t face = 0; face < count; ++face)
        for (const auto& mip : faces[face].mips) {
            const size_t stride = Align(expanded || codedFallback ? mip.width * 4 : mip.rowBytes);
            const size_t rows = expanded || codedFallback ? mip.height : mip.bytes.size() / mip.rowBytes;
            Require(stride * rows <= 256 * 1024 * 1024 - total, "Native texture staging exceeds memory budget");
            layouts.push_back({total, stride, rows});
            total += stride * rows;
        }
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    Require(texture != nil, "Metal texture allocation failed");
    texture.label = @"WoW authored texture mip chain";
    id<MTLBuffer> staging = [device newBufferWithLength:total options:MTLResourceStorageModeShared];
    Require(staging != nil, "Metal texture staging allocation failed");
    for (size_t face = 0; face < count; ++face)
        for (size_t level = 0; level < image.mips.size(); ++level) {
            BLP::Mip decoded;
            if (expanded) {
                decoded = faces[face].DecodeBGRA(level);
            }
            const BLP::Mip& mip = expanded ? decoded : faces[face].mips[level];
            const auto& layout = layouts[face * image.mips.size() + level];
            for (size_t row = 0; row < layout.rows; ++row) {
                auto* dest = static_cast<uint8_t*>(staging.contents) + layout.offset + row * layout.stride;
                const auto* source = mip.bytes.data() + row * mip.rowBytes;
                if (codedFallback) {
                    for (uint32_t x = 0; x < mip.width; ++x) {
                        uint16_t word;
                        std::memcpy(&word, source + x * 2, 2);
                        const bool rgb565 = image.format == BLP::Format::RGB565;
                        const unsigned greenBits = rgb565 ? 6 : 5;
                        dest[x * 4] = word & 31;
                        dest[x * 4 + 1] = (word >> 5) & ((1u << greenBits) - 1);
                        dest[x * 4 + 2] = (word >> (5 + greenBits)) & 31;
                        dest[x * 4 + 3] = rgb565 ? 255 : word >> 15;
                    }
                } else if (!expanded && image.format == BLP::Format::ARGB4444) {
                    // D3D A:R:G:B -> Metal R:G:B:A in the 16-bit word. This is
                    // only a rotation of channel bits, with no precision loss.
                    for (uint32_t x = 0; x < mip.width; ++x) {
                        uint16_t word;
                        std::memcpy(&word, source + x * 2, 2);
                        word = uint16_t((word << 4) | (word >> 12));
                        std::memcpy(dest + x * 2, &word, 2);
                    }
                } else {
                    std::memcpy(dest, source, mip.rowBytes);
                }
            }
        }
    id<MTLCommandBuffer> command = [queue commandBuffer];
    command.label = @"Upload BLP mip chain";
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    Require(blit != nil, "Cannot create Metal upload encoder");
    for (size_t face = 0; face < count; ++face)
        for (size_t level = 0; level < image.mips.size(); ++level) {
            const auto& mip = faces[face].mips[level];
            const auto& layout = layouts[face * image.mips.size() + level];
            [blit copyFromBuffer:staging
                       sourceOffset:layout.offset
                  sourceBytesPerRow:layout.stride
                sourceBytesPerImage:layout.stride * layout.rows
                         sourceSize:MTLSizeMake(mip.width, mip.height, 1)
                          toTexture:texture
                   destinationSlice:face
                   destinationLevel:level
                  destinationOrigin:MTLOriginMake(0, 0, 0)];
        }
    [blit endEncoding];
    [command addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        if (completed.error) { NSLog(@"BLP upload failed: %@", completed.error); }
    }];
    [command commit];
    return texture;
}

void GxMetal::TextureDevice::Draw(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
    const QuadParameters& parameters, const TextureDrawState& state, const MTLViewport* viewport) {
    // Resolve/validate before opening an encoder so failures cannot leave a pass open.
    GeometryDraw quad; quad.format = GxVertexBufferFormats_Last;
    id<MTLRenderPipelineState> pipeline = Pipeline(state, quad, false, simd_any(state.decodeScale != simd_make_float4(1)),
        pass.colorAttachments[0].texture.pixelFormat, uint32_t(pass.colorAttachments[0].texture.sampleCount));
    id<MTLSamplerState> sampler = Sampler(state);
    Require(std::isfinite(state.blendFactor) && state.blendFactor >= 0 && state.blendFactor <= 1, "Invalid Gx blend factor");
    const float factor = uint32_t(state.blendFactor * 255.0f) / 255.0f;
    QuadParameters constants = parameters;
    constants.alphaRef = state.alphaRef / 255.0f;
    constants.decodeScale[0] = state.decodeScale;
    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot create Metal render encoder");
    [encoder setRenderPipelineState:pipeline];
    [encoder setBlendColorRed:factor green:factor blue:factor alpha:factor];
    [encoder setCullMode:MTLCullModeNone];
    if (viewport) { [encoder setViewport:*viewport]; }
    [encoder setVertexBytes:&constants length:sizeof(constants) atIndex:0];
    [encoder setFragmentBytes:&constants length:sizeof(constants) atIndex:0];
    [encoder setFragmentTexture:texture atIndex:0];
    [encoder setFragmentSamplerState:sampler atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
}

void GxMetal::TextureDevice::ClearRegion(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass,
    const MTLScissorRect& region, uint32_t mask, simd_float4 color, id<MTLRenderCommandEncoder> encoder) {
    const bool hasDepth = pass.depthAttachment.texture != nil, writeColor = (mask & 1) != 0, writeDepth = (mask & 2) != 0;
    auto target = pass.colorAttachments[0].texture;
    Require(command && target && (target.pixelFormat == MTLPixelFormatBGRA8Unorm || target.pixelFormat == MTLPixelFormatR32Float) &&
        (!hasDepth || (pass.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float &&
                      pass.depthAttachment.texture.sampleCount == target.sampleCount)) && (!writeDepth || hasDepth),
        "Unsupported Metal regional clear attachments");
    Require(mask && mask <= 3 && region.width && region.height && region.x < target.width && region.y < target.height &&
        region.width <= target.width - region.x && region.height <= target.height - region.y, "Invalid Metal clear region");
    const uint32_t key = uint32_t(target.pixelFormat == MTLPixelFormatR32Float) | (uint32_t(hasDepth) << 1) |
        (uint32_t(writeColor) << 2) | (uint32_t(target.sampleCount) << 3);
    auto& pipeline = m_clearPipelines[key];
    if (!pipeline) {
        auto descriptor = [MTLRenderPipelineDescriptor new];
        descriptor.label = @"Gx regional attachment clear";
        descriptor.rasterSampleCount = target.sampleCount;
        descriptor.vertexFunction = m_clearVertex; descriptor.fragmentFunction = m_clearFragment;
        descriptor.colorAttachments[0].pixelFormat = target.pixelFormat;
        descriptor.colorAttachments[0].writeMask = writeColor ? MTLColorWriteMaskAll : MTLColorWriteMaskNone;
        if (hasDepth) { descriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float; }
        NSError* error = nil;
        pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!pipeline) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Cannot create regional clear pipeline"); }
    }
    auto& depth = m_clearDepth[writeDepth];
    if (!depth) {
        auto descriptor = [MTLDepthStencilDescriptor new];
        descriptor.depthCompareFunction = MTLCompareFunctionAlways; descriptor.depthWriteEnabled = writeDepth;
        depth = [device newDepthStencilStateWithDescriptor:descriptor];
        Require(depth != nil, "Cannot create clear depth state");
    }
    const bool ownsEncoder = encoder == nil;
    if (ownsEncoder) encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot encode regional Gx clear");
    [encoder setRenderPipelineState:pipeline]; [encoder setDepthStencilState:depth];
    [encoder setCullMode:MTLCullModeNone];
    [encoder setViewport:MTLViewport{double(region.x), double(region.y), double(region.width), double(region.height), 0, 1}];
    [encoder setScissorRect:region];
    [encoder setFragmentBytes:&color length:sizeof(color) atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    if (ownsEncoder) [encoder endEncoding];
}
