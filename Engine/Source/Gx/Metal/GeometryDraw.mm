#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include <cmath>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
struct alignas(16) GeometryParameters {
    C44Matrix clipFromModel;
    GxMetal::QuadParameters texture;
};
struct alignas(16) WaveParameters {
    simd_float2 sceneScale, blurScale;
    float alphaRef, padding[3]{};
    simd_float4 decodeScale[3];
};
struct alignas(16) WaterRippleParameters {
    simd_float4 decodeScale;
    float alphaRef, depthOffset, padding[2]{};
};
static_assert(sizeof(WaterRippleParameters) == 32, "Metal ripple constant layout");
static_assert(sizeof(WaveParameters) == 80, "Metal wave constant layout");
static_assert(sizeof(GeometryParameters) == 192 && offsetof(GeometryParameters, texture) == 64, "Metal geometry constant layout");
}

void GxMetal::TextureDevice::DrawFixedUnlit(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass,
    const std::array<id<MTLTexture>, 9>& textures, const GeometryDraw& geometry,
    const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::FixedUnlit && geometry.fixed && geometry.fixed->stages <= 4,
            "Invalid unlit fixed-function draw");
    DrawGeometry(command, pass, textures, geometry, {}, states, encoder);
}
void GxMetal::TextureDevice::DrawUnlit(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
    const GeometryDraw& geometry, const QuadParameters& parameters, const TextureDrawState& state, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::Unlit || geometry.effect == FragmentEffect::Desaturate, "UI draw received a different native effect");
    DrawGeometry(command, pass, {texture, nil, nil, nil}, geometry, parameters, {state, state, state, state}, encoder);
}
void GxMetal::TextureDevice::DrawDetailDoodad(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::DetailDoodad && geometry.detail, "Invalid native detail-doodad effect");
    DrawGeometry(command, pass, textures, geometry, {}, states, encoder);
}
void GxMetal::TextureDevice::DrawWaterRipple(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
    const GeometryDraw& geometry, const TextureDrawState& state, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::WaterRipple && std::isfinite(geometry.depthOffset), "Invalid native water-ripple effect");
    DrawGeometry(command, pass, {texture, nil, nil, nil, nil}, geometry, {}, {state, {}, {}, {}, {}}, encoder);
}
void GxMetal::TextureDevice::DrawShadowMap(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
    const GeometryDraw& geometry, const TextureDrawState& state, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::ShadowMap && geometry.shadow, "Invalid native shadow-map effect");
    DrawGeometry(command, pass, {texture, nil, nil, nil, nil}, geometry, {}, {state, {}, {}, {}, {}}, encoder);
}
void GxMetal::TextureDevice::DrawModel(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(IsModelEffect(geometry.effect), "Invalid native model effect");
    DrawGeometry(command, pass, textures, geometry, {}, states, encoder);
}
void GxMetal::TextureDevice::DrawFilter4(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 4>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 4>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::Box4 || geometry.effect == FragmentEffect::Gauss4, "Invalid FFX filter effect");
    DrawGeometry(command, pass, {textures[0], textures[1], textures[2], textures[3], nil}, geometry, {}, {states[0], states[1], states[2], states[3], {}}, encoder);
}
void GxMetal::TextureDevice::DrawGlow(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 2>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 2>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::Glow, "Invalid FFX glow effect");
    DrawGeometry(command, pass, {textures[0], textures[1], nil, nil}, geometry, {}, {states[0], states[1], {}, {}}, encoder);
}
void GxMetal::TextureDevice::DrawFogEffect(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 4>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 4>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::PropagateFog || geometry.effect == FragmentEffect::FogCombine, "Invalid fog effect");
    DrawGeometry(command, pass, {textures[0], textures[1], textures[2], textures[3]}, geometry, {}, {states[0], states[1], states[2], states[3]}, encoder);
}
void GxMetal::TextureDevice::DrawNether(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 4>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 4>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::NetherBlur || geometry.effect == FragmentEffect::NetherCombine, "Invalid nether effect");
    DrawGeometry(command, pass, {textures[0], textures[1], textures[2], textures[3]}, geometry, {}, {states[0], states[1], states[2], states[3]}, encoder);
}
void GxMetal::TextureDevice::DrawGlowWave(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 3>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 3>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::GlowWave, "Invalid FFX wave effect");
    DrawGeometry(command, pass, {textures[0], textures[1], textures[2], nil}, geometry, {}, {states[0], states[1], states[2], {}}, encoder);
}
void GxMetal::TextureDevice::DrawDeath(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 2>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 2>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(geometry.effect == FragmentEffect::Death, "Invalid FFX death effect");
    DrawGeometry(command, pass, {textures[0], textures[1], nil, nil}, geometry, {}, {states[0], states[1], {}, {}}, encoder);
}
void GxMetal::TextureDevice::DrawLiquid(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 2>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 2>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(IsLiquidEffect(geometry.effect) && geometry.liquid, "Invalid native liquid effect");
    DrawGeometry(command, pass, {textures[0], textures[1], nil, nil, nil}, geometry, {}, {states[0], states[1], {}, {}, {}}, encoder);
}
void GxMetal::TextureDevice::DrawWmo(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(IsWmoEffect(geometry.effect) && geometry.wmo, "Invalid native WMO effect");
    DrawGeometry(command, pass, textures, geometry, {}, states, encoder);
}
void GxMetal::TextureDevice::DrawTerrain(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
    const GeometryDraw& geometry, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder) {
    Require(IsTerrainEffect(geometry.effect) && geometry.terrain, "Invalid native terrain effect");
    DrawGeometry(command, pass, textures, geometry, {}, states, encoder);
}
void GxMetal::TextureDevice::DrawGeometry(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, const std::array<id<MTLTexture>, 9>& textures,
    const GeometryDraw& geometry, const QuadParameters& parameters, const std::array<TextureDrawState, 9>& states, id<MTLRenderCommandEncoder> encoder) {
    const bool filter = geometry.effect == FragmentEffect::Box4 || geometry.effect == FragmentEffect::Gauss4 || geometry.effect == FragmentEffect::PropagateFog;
    const bool model = IsModelEffect(geometry.effect);
    const bool detail = geometry.effect == FragmentEffect::DetailDoodad;
    Require(!detail || geometry.detail, "Missing native detail-doodad parameters");
    const bool shadow = geometry.effect == FragmentEffect::ShadowMap;
    Require(!shadow || (geometry.shadow && geometry.shadow->boneCount && geometry.shadow->boneCount <= geometry.shadow->bones.size()), "Invalid shadow-map palette");
    const bool liquid = IsLiquidEffect(geometry.effect);
    Require(!liquid || geometry.liquid, "Missing native liquid parameters");
    const bool wmo = IsWmoEffect(geometry.effect);
    Require(!wmo || geometry.wmo, "Missing native WMO parameters");
    const bool terrain = IsTerrainEffect(geometry.effect);
    Require(!terrain || geometry.terrain, "Missing native terrain parameters");
    Require(!model || (geometry.model && geometry.model->boneCount && geometry.model->boneCount <= geometry.model->bones.size()), "Invalid native model palette");
    const bool wave = geometry.effect == FragmentEffect::GlowWave;
    const bool terrainShadow = geometry.effect == FragmentEffect::TerrainShadowMap;
    const unsigned inputs = geometry.fixed ? geometry.fixed->stages : terrainShadow ? 0 : shadow ? unsigned(geometry.shadow->cutout) : liquid ? LiquidTextureCount(geometry.effect) : wmo ? WmoTextureCount(geometry.effect) : terrain ? TerrainLayerCount(GxSh_Pixel, geometry.terrain->pixelPermutation) + 1 : model ? ModelTextureCount(geometry.effect) : (filter || geometry.effect == FragmentEffect::NetherBlur) ? 4 : wave ? 3 : (geometry.effect == FragmentEffect::Glow || geometry.effect == FragmentEffect::Death || geometry.effect == FragmentEffect::NetherCombine || geometry.effect == FragmentEffect::FogCombine) ? 2 : 1;
    const uint32_t inputMask = terrainShadow ? 0 : terrain ? TerrainTextureMask(geometry.terrain->pixelPermutation, geometry.terrain->shadowMode) : ((1u << inputs) - 1) | (model ? ShadowReceiverTextureMask(geometry.model->shadowMode) : wmo ? ShadowReceiverTextureMask(geometry.wmo->shadowMode) : detail ? ShadowReceiverTextureMask(geometry.detail->shadowMode) : 0);
    for (unsigned i = 0; i < textures.size(); ++i)
        if (inputMask & (1u << i)) {
            const bool cube = terrain && i == 0 && (geometry.terrain->pixelPermutation & 1);
            Require(textures[i] && textures[i].textureType == (cube ? MTLTextureType2DArray : MTLTextureType2D) &&
                        (!cube || textures[i].arrayLength == 6),
                    "Native shader texture type mismatch");
        }
    const auto& state = states[0];
    Require(command && pass && (!inputs || textures[0]) && geometry.vertices, "Missing Metal geometry resource");
    Require((filter || wave) ? geometry.format == GxVertexBufferFormats_Last : geometry.format >= GxVBF_P && geometry.format < GxVertexBufferFormats_Last,
        "Invalid Gx vertex format for native effect");
    Require(model || shadow || geometry.format != GxVBF_PBNT2, "Skinned Gx vertices require a skinning effect");
    const bool weatherPoints = geometry.fixed && (geometry.fixed->weather == WeatherVertex::Snow || geometry.fixed->weather == WeatherVertex::Sand);
    Require(!weatherPoints || !geometry.indices, "Weather point packets use non-indexed instances");
    const auto& batch = geometry.batch;
    Require(batch.m_primType >= GxPrim_Points && batch.m_primType < GxPrim_TriangleFan,
        "This Metal geometry path requires points, lines, line strips, triangles or triangle strips");
    const MTLPrimitiveType primitives[] = {MTLPrimitiveTypePoint, MTLPrimitiveTypeLine, MTLPrimitiveTypeLineStrip,
        MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangleStrip};
    size_t count = batch.m_count;
    if (batch.m_primType == GxPrim_Lines) { count -= count % 2; }
    if (batch.m_primType == GxPrim_Triangles) { count -= count % 3; }
    Require(count >= (batch.m_primType >= GxPrim_Triangles ? 3 : batch.m_primType >= GxPrim_Lines ? 2 : 1), "Empty Gx geometry batch");
    const size_t stride = filter ? 48 : wave ? 40 : GxVertexSize(geometry.format);
    Require(geometry.vertexOffset % 4 == 0 && geometry.vertexOffset <= geometry.vertices.length, "Invalid Gx vertex buffer offset");
    const size_t availableVertices = (geometry.vertices.length - geometry.vertexOffset) / stride;
    size_t indexOffset = geometry.indexOffset;
    if (geometry.indices) {
        Require(batch.m_minIndex <= batch.m_maxIndex && batch.m_maxIndex < availableVertices, "Gx vertex range exceeds buffer");
        Require(indexOffset <= geometry.indices.length && batch.m_start <= (geometry.indices.length - indexOffset) / sizeof(uint16_t),
            "Invalid Gx index buffer offset");
        indexOffset += size_t(batch.m_start) * sizeof(uint16_t);
        Require(indexOffset % 2 == 0 && indexOffset <= geometry.indices.length &&
            count <= (geometry.indices.length - indexOffset) / sizeof(uint16_t), "Gx index range exceeds buffer");
    } else {
        Require(count <= availableVertices, "Gx vertex count exceeds buffer");
    }
    id<MTLTexture> target = pass.colorAttachments[0].texture;
    Require(target && (target.pixelFormat == MTLPixelFormatBGRA8Unorm || target.pixelFormat == MTLPixelFormatR32Float), "Unsupported geometry color attachment");
    const bool hasDepth = pass.depthAttachment.texture != nil;
    Require(!hasDepth || (pass.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float &&
                         pass.depthAttachment.texture.sampleCount == target.sampleCount), "Unsupported geometry depth attachment");
    Require(hasDepth || (!geometry.depthTest && !geometry.depthWrite), "Gx depth state requires a depth attachment");
    Require(geometry.depthFunction < 4 && geometry.culling < 3, "Invalid Gx depth/culling state");
    if (geometry.scissorEnabled) {
        Require(geometry.scissor.x <= target.width && geometry.scissor.y <= target.height &&
            geometry.scissor.width <= target.width - geometry.scissor.x && geometry.scissor.height <= target.height - geometry.scissor.y,
            "Gx scissor exceeds target");
    }
    Require(std::isfinite(state.blendFactor) && state.blendFactor >= 0 && state.blendFactor <= 1, "Invalid Gx blend factor");
    if (geometry.viewportEnabled) {
        const auto& v = geometry.viewport;
        Require(std::isfinite(v.originX) && std::isfinite(v.originY) && std::isfinite(v.width) && std::isfinite(v.height) &&
            v.originX >= 0 && v.originY >= 0 && v.width > 0 && v.height > 0 &&
            v.originX + v.width <= target.width && v.originY + v.height <= target.height &&
            v.znear >= 0 && v.zfar <= 1 && v.znear <= v.zfar, "Invalid Metal geometry viewport");
    }
    bool decodePacked = false;
    for (unsigned i = 0; i < states.size(); ++i) if (inputMask & (1u << i)) { decodePacked |= simd_any(states[i].decodeScale != simd_make_float4(1)); }
    id<MTLRenderPipelineState> pipeline = Pipeline(state, geometry, hasDepth, decodePacked, target.pixelFormat, uint32_t(target.sampleCount));
    std::array<id<MTLSamplerState>, 9> samplers{};
    for (unsigned i = 0; i < textures.size(); ++i) if (inputMask & (1u << i)) {
        Require(textures[i] && textures[i] != target && textures[i] != pass.depthAttachment.texture, "Missing FFX input or render-target feedback");
        const bool comparison=i>=4 && ((model && geometry.model->hardwareShadow) || (wmo && geometry.wmo->hardwareShadow));
        Require((textures[i].pixelFormat==MTLPixelFormatDepth32Float)==comparison,"Shader depth comparison and texture format mismatch");
        samplers[i] = Sampler(states[i],comparison);
    }
    const uint32_t depthKey = geometry.depthFunction | (unsigned(geometry.depthTest) << 2) | (unsigned(geometry.depthWrite) << 3);
    auto depth = m_depthStates.find(depthKey);
    if (depth == m_depthStates.end()) {
        const MTLCompareFunction comparison[] = {MTLCompareFunctionLessEqual, MTLCompareFunctionEqual, MTLCompareFunctionGreaterEqual, MTLCompareFunctionLess};
        MTLDepthStencilDescriptor* descriptor = [MTLDepthStencilDescriptor new];
        descriptor.depthCompareFunction = geometry.depthTest ? comparison[geometry.depthFunction] : MTLCompareFunctionAlways;
        // Gx leaves D3D9 ZENABLE on: disabling its DepthTest selects ALWAYS,
        // while GxRs_DepthWrite remains independent.
        descriptor.depthWriteEnabled = geometry.depthWrite;
        id<MTLDepthStencilState> created = [device newDepthStencilStateWithDescriptor:descriptor];
        Require(created != nil, "Cannot create Metal depth state");
        depth = m_depthStates.emplace(depthKey, created).first;
    }
    GeometryParameters constants{geometry.clipFromModel, parameters};
    if (geometry.effect == FragmentEffect::FogCombine) {
        // The original persistent polar grid bypasses SetupGeo_Quad's Metal
        // coordinate conversion. Preserve its D3D9 integer pixel centers.
        const auto viewport = geometry.viewportEnabled ? geometry.viewport : MTLViewport{0, 0, double(target.width), double(target.height), 0, 1};
        Require(viewport.width > 0 && viewport.height > 0, "Empty fog composition viewport");
        float* matrix = &constants.clipFromModel.a0;
        for (unsigned row = 0; row < 4; ++row) {
            matrix[row * 4] += matrix[row * 4 + 3] / viewport.width;
            matrix[row * 4 + 1] -= matrix[row * 4 + 3] / viewport.height;
        }
    }
    constants.texture.alphaRef = state.alphaRef / 255.0f;
    for (unsigned i = 0; i < std::min(inputs, 4u); ++i) { constants.texture.decodeScale[i] = states[i].decodeScale; }
    // Tempest uses row vectors. Its contiguous rows are Metal's columns, so
    // column-vector multiplication reads the same transform without repacking.
    const bool ownsEncoder = encoder == nil;
    if (ownsEncoder) encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot create Metal geometry encoder");
    [encoder setRenderPipelineState:pipeline];
    [encoder setDepthStencilState:depth->second];
    [encoder setFrontFacingWinding:MTLWindingClockwise];
    [encoder setViewport:geometry.viewportEnabled ? geometry.viewport : MTLViewport{0, 0, double(target.width), double(target.height), 0, 1}];
    [encoder setCullMode:weatherPoints || geometry.culling == 0 ? MTLCullModeNone : geometry.culling == 1 ? MTLCullModeFront : MTLCullModeBack];
    [encoder setScissorRect:geometry.scissorEnabled ? geometry.scissor : MTLScissorRect{0, 0, target.width, target.height}];
    const float factor = uint32_t(state.blendFactor * 255.0f) / 255.0f;
    [encoder setBlendColorRed:factor green:factor blue:factor alpha:factor];
    if (geometry.effect == FragmentEffect::NetherBlur) {
        auto native = geometry.netherBlur;
        const auto viewport = geometry.viewportEnabled ? geometry.viewport : MTLViewport{0, 0, double(target.width), double(target.height), 0, 1};
        Require(viewport.width > 0 && viewport.height > 0, "Empty nether blur viewport");
        native.pixelCenterOffset = {float(1 / viewport.width), float(-1 / viewport.height)};
        [encoder setVertexBytes:&native length:sizeof(native) atIndex:0];
    } else if (geometry.fixed) {
        if (geometry.fixed->weather != WeatherVertex::None)
            [encoder setVertexBytes:&geometry.fixed->weatherParameters length:sizeof(WeatherParameters) atIndex:0];
        else [encoder setVertexBytes:&geometry.fixed->vertex length:sizeof(FixedUnlitVertexParameters) atIndex:0];
    } else if (geometry.effect == FragmentEffect::WaterRipple) {
        [encoder setVertexBytes:&geometry.clipFromModel length:sizeof(C44Matrix) atIndex:0];
    } else if (detail) {
        [encoder setVertexBytes:&geometry.detail->vertex length:sizeof(DetailDoodadVertexParameters) atIndex:0];
        if (geometry.detail->shadowMode) {
            auto native = geometry.detail->shadow;
            for (unsigned i=0;i<4;++i) native.decodeScale[i] = states[4+i].decodeScale[0];
            [encoder setVertexBytes:&native length:sizeof(native) atIndex:3];
            [encoder setFragmentBytes:&native length:sizeof(native) atIndex:3];
        }
    } else if (shadow) {
        [encoder setVertexBytes:&geometry.shadow->vertex length:sizeof(ShadowMapVertexParameters) atIndex:0];
        [encoder setVertexBytes:geometry.shadow->bones.data() length:geometry.shadow->boneCount * sizeof(ModelAffine) atIndex:2];
    } else if (model) {
        if (geometry.model->permutation >= 30) {
            auto native = geometry.model->shadow;
            for (unsigned i = 0; i < 4; ++i) native.decodeScale[i] = states[4+i].decodeScale[0];
            [encoder setVertexBytes:&native length:sizeof(native) atIndex:3];
            [encoder setFragmentBytes:&native length:sizeof(native) atIndex:3];
        }
        if (geometry.model->clip.mask)
            [encoder setVertexBytes:&geometry.model->clip length:sizeof(ModelClipParameters) atIndex:4];
        [encoder setVertexBytes:&geometry.model->vertex length:sizeof(ModelVertexParameters) atIndex:0];
        [encoder setVertexBytes:geometry.model->bones.data() length:geometry.model->boneCount * sizeof(ModelAffine) atIndex:2];
    } else if (liquid) { [encoder setVertexBytes:&geometry.liquid->vertex length:sizeof(LiquidVertexParameters) atIndex:0];
    } else if (wmo) {
        if (geometry.wmo->permutation >= 30) {
            auto native = geometry.wmo->shadow;
            for (unsigned i = 0; i < 4; ++i) native.decodeScale[i] = states[4+i].decodeScale[0];
            [encoder setVertexBytes:&native length:sizeof(native) atIndex:3];
            [encoder setFragmentBytes:&native length:sizeof(native) atIndex:3];
        }
        [encoder setVertexBytes:&geometry.wmo->vertex length:sizeof(WmoVertexParameters) atIndex:0];
    } else if (terrain) { [encoder setVertexBytes:&geometry.terrain->vertex length:sizeof(TerrainVertexParameters) atIndex:0];
    } else { [encoder setVertexBytes:&constants length:sizeof(constants) atIndex:0]; }
    if (geometry.effect == FragmentEffect::PropagateFog || geometry.effect == FragmentEffect::FogCombine)
        [encoder setFragmentBytes:&geometry.fogEffect length:sizeof(geometry.fogEffect) atIndex:1];
    if (geometry.effect == FragmentEffect::NetherCombine)
        [encoder setFragmentBytes:&geometry.netherTintFade length:sizeof(geometry.netherTintFade) atIndex:1];
    [encoder setVertexBuffer:geometry.vertices offset:geometry.vertexOffset atIndex:1];
    if (geometry.fixed) {
        FixedUnlitFragmentParameters material = geometry.fixed->fragment;
        material.alphaRef = state.alphaRef / 255.0f;
        material.depthOffset = geometry.depthOffset;
        for (unsigned i = 0; i < inputs; ++i) material.decodeScale[i] = states[i].decodeScale;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (geometry.effect == FragmentEffect::WaterRipple) {
        WaterRippleParameters material{state.decodeScale, state.alphaRef / 255.0f, geometry.depthOffset};
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (detail) {
        DetailDoodadFragmentParameters material = geometry.detail->fragment;
        material.stateAlphaRef = state.alphaRef / 255.0f; material.decodeScale = state.decodeScale;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (shadow) {
        ShadowMapFragmentParameters material = geometry.shadow->fragment;
        material.stateAlphaRef = state.alphaRef / 255.0f; material.decodeScale = state.decodeScale;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (model) {
        ModelFragmentParameters material = geometry.model->fragment;
        material.stateAlphaRef = state.alphaRef / 255.0f;
        material.depthOffset = geometry.depthOffset;
        for (unsigned i = 0; i < inputs; ++i) material.decodeScale[i] = states[i].decodeScale;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (liquid) {
        LiquidFragmentParameters material = geometry.liquid->fragment;
        material.stateAlphaRef = state.alphaRef / 255.0f;
        for (unsigned i = 0; i < inputs; ++i) material.decodeScale[i] = states[i].decodeScale;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (wmo) {
        ModelFragmentParameters material = geometry.wmo->fragment;
        material.stateAlphaRef = state.alphaRef / 255.0f;
        for (unsigned i = 0; i < inputs; ++i) material.decodeScale[i] = states[i].decodeScale;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (terrain) {
        TerrainFragmentParameters material = geometry.terrain->fragment;
        material.stateAlphaRef = state.alphaRef / 255.0f;
        for (unsigned i = 0; i < inputs; ++i) material.decodeScale[i] = states[i].decodeScale;
        for (unsigned i = 0; i < 4; ++i) material.shadowDecodeScale[i] = states[5 + i].decodeScale.x;
        [encoder setFragmentBytes:&material length:sizeof(material) atIndex:0];
    } else if (wave) {
        WaveParameters waveConstants{geometry.waveSceneScale, geometry.waveBlurScale, constants.texture.alphaRef};
        for (unsigned i = 0; i < 3; ++i) { waveConstants.decodeScale[i] = states[i].decodeScale; }
        [encoder setFragmentBytes:&waveConstants length:sizeof(waveConstants) atIndex:0];
    } else { [encoder setFragmentBytes:&constants.texture length:sizeof(constants.texture) atIndex:0]; }
    for (unsigned i = 0; i < textures.size(); ++i) if (inputMask & (1u << i)) {
        [encoder setFragmentTexture:textures[i] atIndex:i];
        [encoder setFragmentSamplerState:samplers[i] atIndex:i];
    }
    if (weatherPoints) {
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:count];
    } else if (geometry.indices) {
        [encoder drawIndexedPrimitives:primitives[batch.m_primType] indexCount:count indexType:MTLIndexTypeUInt16
            indexBuffer:geometry.indices indexBufferOffset:indexOffset];
    } else {
        [encoder drawPrimitives:primitives[batch.m_primType] vertexStart:0 vertexCount:count];
    }
    if (ownsEncoder) [encoder endEncoding];
}
