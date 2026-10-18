#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
using Color = std::array<float, 4>;
Color Unpack(uint32_t p) { return {float((p >> 16) & 255) / 255, float((p >> 8) & 255) / 255, float(p & 255) / 255, float(p >> 24) / 255}; }
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    if (command == GxTex_Latch) { stride = 8; data = context; }
}
float Operation(unsigned op, float texture, float current, float previousAlpha, float diffuseAlpha) {
    switch (op) {
        case 0: return texture * current;
        case 1: return texture * current * 2;
        case 2: return texture + current;
        case 3: return current;
        case 4: return previousAlpha * current + (1 - previousAlpha) * texture;
        case 5: return diffuseAlpha * texture + (1 - diffuseAlpha) * current;
    }
    throw std::runtime_error("Invalid fixed-function reference operation");
}
Color Stage(Color current, Color texture, unsigned rgb, unsigned alpha, float diffuseAlpha) {
    Color result;
    for (unsigned c = 0; c < 4; ++c)
        result[c] = std::clamp(Operation(c == 3 ? alpha : rgb, texture[c], current[c], current[3], diffuseAlpha), 0.0f, 1.0f);
    return result;
}
}
void CheckFixedUnlit(CGxDeviceMetal& device) {
    device.RsPush();
    const auto projection = device.m_projNative;
    for (auto transform : {GxXform_View, GxXform_World}) device.XformPushAndSet(transform, C44Matrix(1));
    for (unsigned i = 0; i < 4; ++i) device.XformPushAndSet(EGxXform(i), C44Matrix(1));
    struct Cleanup {
        CGxDeviceMetal& device; C44Matrix projection; CGxTex* textures[4]{};
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop();
            for (auto& texture : textures) device.TexDestroy(texture);
            for (unsigned i = 0; i < 4; ++i) GxXformPop(EGxXform(i));
            GxXformPop(GxXform_World); GxXformPop(GxXform_View);
            device.m_projNative = projection;
        }
    } cleanup{device, projection};
    std::array<std::array<uint32_t, 4>, 4> texels{{{0x806090C0,0x40D030A0,0xC030E020,0xFF809050},
        {0xB0D08040,0x70603090,0x5090B060,0xFF304080}, {0x609030B0,0xFFA09020,0xFF204060,0xFF104080},
        {0xE0B06020,0xFF10A0F0,0xFF809010,0xFF708040}}};
    for (unsigned i = 0; i < 4; ++i) {
        device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest),
                         texels[i].data(), Upload, "Fixed-function reference", cleanup.textures[i]);
        device.RsSet(EGxRenderState(GxRs_Texture0 + i), static_cast<CGxTex*>(nullptr));
        GxRsSet(EGxRenderState(GxRs_TexGen0 + i), 0);
        GxRsSet(EGxRenderState(GxRs_TextureShader0 + i), 0);
        GxRsSet(EGxRenderState(GxRs_TextureCoord0 + i), int(i & 1));
    }
    device.RsSet(GxRs_VertexShader, static_cast<CGxShader*>(nullptr));
    device.RsSet(GxRs_PixelShader, static_cast<CGxShader*>(nullptr));
    for (auto state : {GxRs_Lighting, GxRs_Fog, GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_ScissorTest,
                       GxRs_ClipPlaneMask, GxRs_PolygonOffset, GxRs_PointSprite, GxRs_AlphaRef}) GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15); GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxRsSet(GxRs_MatDiffuse, -1); GxRsSet(GxRs_MatEmissive, 0);
    device.m_projNative = C44Matrix(1);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 1;
    C3Vector positions[4]{{-1,1,0.5f},{1,1,0.5f},{-1,-1,0.5f},{1,-1,0.5f}};
    CImVector color(160, 80, 112, 128);
    const Color diffuse = Unpack(0x807050A0);
    const C2Vector uv0(0.25f, 0.25f), uv1(0.75f, 0.25f);
    CGxBatch batch; batch.m_primType = GxPrim_TriangleStrip; batch.m_count = 4; batch.m_start = batch.m_minIndex = 0; batch.m_maxIndex = 3;
    unsigned draws = 0;
    auto draw = [&](Color expected, bool vertexColor = true, float expectedDepth = -1) {
        device.BeginFrame(pass);
        GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, vertexColor ? &color : nullptr, 0, &uv0, 0, &uv1, 0);
        device.Draw(&batch, 0);
        CiRect rect(0, 0, 16, 16); TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        if (expectedDepth >= 0) {
            TSGrowableArray<float> depth; device.DeviceReadDepths(rect, depth);
            if (std::abs(depth[8 * 16 + 8] - expectedDepth) > 1e-6f)
                throw std::runtime_error("Fixed-function normalized depth mismatch at draw " + std::to_string(draws));
        }
        device.ScenePresent();
        const auto& actual = pixels[8 * 16 + 8];
        const unsigned channels[]{actual.r, actual.g, actual.b, actual.a};
        for (unsigned c = 0; c < 4; ++c) if (std::abs(int(channels[c]) - int(std::lround(expected[c] * 255))) > 1)
            throw std::runtime_error("Fixed-function numerical mismatch at draw " + std::to_string(draws) + " channel " + std::to_string(c));
        ++draws;
    };
    draw(diffuse);
    // Stage one must remain disabled when stage zero is absent.
    device.RsSet(GxRs_Texture1, cleanup.textures[1]); draw(diffuse);
    device.RsSet(GxRs_Texture0, cleanup.textures[0]);
    for (unsigned rgb = 0; rgb < 6; ++rgb) for (unsigned alpha = 0; alpha < 6; ++alpha) {
        GxRsSet(GxRs_ColorOp0, int(rgb)); GxRsSet(GxRs_AlphaOp0, int(alpha));
        GxRsSet(GxRs_ColorOp1, int(alpha)); GxRsSet(GxRs_AlphaOp1, int(rgb));
        draw(Stage(Stage(diffuse, Unpack(texels[0][0]), rgb, alpha, diffuse[3]), Unpack(texels[1][1]), alpha, rgb, diffuse[3]));
    }
    Color expected = diffuse;
    const unsigned operations[4]{0,2,4,5};
    for (unsigned i = 0; i < 4; ++i) {
        device.RsSet(EGxRenderState(GxRs_Texture0+i), cleanup.textures[i]);
        GxRsSet(EGxRenderState(GxRs_ColorOp0+i), int(operations[i])); GxRsSet(EGxRenderState(GxRs_AlphaOp0+i), int(operations[3-i]));
        expected = Stage(expected, Unpack(texels[i][i&1]), operations[i], operations[3-i], diffuse[3]);
    }
    draw(expected);
    GxRsSet(GxRs_PolygonOffset, 0.125f); draw(expected);
    GxRsSet(GxRs_PolygonOffset, 0.0f); draw(expected);
    for (unsigned i = 1; i < 4; ++i) device.RsSet(EGxRenderState(GxRs_Texture0+i), static_cast<CGxTex*>(nullptr));
    GxRsSet(GxRs_ColorOp0, 0); GxRsSet(GxRs_AlphaOp0, 0);
    // Affine translation must use Gx's homogeneous coordinate placement.
    GxRsSet(GxRs_TextureShader0, 1);
    device.m_xforms[0].m_mtx[device.m_xforms[0].m_level].d0 = 0.5f;
    draw(Stage(diffuse, Unpack(texels[0][1]), 0, 0, diffuse[3]));
    // Project after raster interpolation, as D3DTTFF_COUNT3|PROJECTED does.
    GxRsSet(GxRs_TextureShader0, 2);
    device.m_xforms[0].m_mtx[device.m_xforms[0].m_level] = C44Matrix(1);
    device.m_xforms[0].m_mtx[device.m_xforms[0].m_level].c2 = 0.5f;
    draw(Stage(diffuse, Unpack(texels[0][3]), 0, 0, diffuse[3]));
    device.RsSet(GxRs_Texture0, static_cast<CGxTex*>(nullptr));
    GxRsSet(GxRs_MatDiffuse, int32_t(0x70503020)); GxRsSet(GxRs_MatEmissive, int32_t(0x00301008));
    draw(Unpack(0x70804028), false);
    // Fog changes RGB only; alpha reference equality still passes.
    GxRsSet(GxRs_Fog, 1); GxRsSet(GxRs_FogStart, 0.25f); GxRsSet(GxRs_FogEnd, 0.75f);
    GxRsSet(GxRs_FogColor, int32_t(0xFF20C040));
    expected = diffuse; const auto fog = Unpack(0xFF20C040);
    for (unsigned c = 0; c < 3; ++c) expected[c] = (diffuse[c]+fog[c])*0.5f;
    GxRsSet(GxRs_AlphaRef, 128); draw(expected);
    GxRsSet(GxRs_AlphaRef, 129); draw({0,0,0,0});
    GxRsSet(GxRs_AlphaRef, 0);
    for (auto& position : positions) position.z = 0.9f;
    expected = fog; expected[3] = diffuse[3]; draw(expected);
    for (auto& position : positions) position.z = 0.1f;
    draw(diffuse);
    positions[1].z = positions[3].z = 0.9f;
    const float factor = 1 - 8.5f / 16;
    expected = diffuse;
    for (unsigned c = 0; c < 3; ++c) expected[c] = factor * diffuse[c] + (1 - factor) * fog[c];
    draw(expected); // Clamp at vertices before interpolation, not at the pixel.
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_DepthWrite, 1);
    for (float z : {0.05f, 0.5f, 0.95f}) for (float offset : {-2.0f, -0.05f, 0.0f, 0.05f, 2.0f}) {
        for (auto& position : positions) position.z = z;
        GxRsSet(GxRs_PolygonOffset, offset);
        draw(diffuse, true, std::clamp(z - offset, 0.0f, 1.0f));
    }
    // The normalized bias is applied after viewport depth mapping.
    GxXformSetViewport(0, 1, 0, 1, 0.2f, 0.8f);
    for (auto& position : positions) position.z = 0.25f;
    GxRsSet(GxRs_PolygonOffset, 0.125f); draw(diffuse, true, 0.225f);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    // It must not move primitives across the original near/far clip planes.
    for (float z : {-0.1f, 1.1f}) {
        for (auto& position : positions) position.z = z;
        GxRsSet(GxRs_PolygonOffset, z < 0 ? -0.5f : 0.5f);
        draw({0,0,0,0}, true, 1);
    }
    for (auto& position : positions) position.z = 0.5f;
    pass.depthAttachment.clearDepth = 0.5;
    GxRsSet(GxRs_DepthTest, 1); GxRsSet(GxRs_DepthFunc, 3); GxRsSet(GxRs_DepthWrite, 0);
    for (float offset : {-0.05f, 0.0f, 0.05f}) {
        GxRsSet(GxRs_PolygonOffset, offset);
        draw(offset > 0 ? diffuse : Color{0,0,0,0}, true, 0.5f);
    }
    GxRsSet(GxRs_DepthWrite, 1); GxRsSet(GxRs_AlphaRef, 128); draw(diffuse, true, 0.45f);
    GxRsSet(GxRs_AlphaRef, 129); draw({0,0,0,0}, true, 0.5f);
    GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_DepthTest, 0); pass.depthAttachment.clearDepth = 1;
    // A slope must not scale a constant offset, unlike slope-scaled bias.
    positions[0].z = positions[2].z = 0.1f; positions[1].z = positions[3].z = 0.9f;
    draw(diffuse, true, 0.1f + (8.5f / 16) * 0.8f - 0.05f);
    for (auto& position : positions) position.z = 0.5f;
    device.BeginFrame(pass);
    for (float offset : {0.0f, 0.25f, 0.0f}) {
        GxRsSet(GxRs_PolygonOffset, offset);
        GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, &color, 0, &uv0, 0, &uv1, 0);
        device.Draw(&batch, 0); ++draws;
    }
    CiRect rect(0, 0, 16, 16); TSGrowableArray<float> depth;
    device.DeviceReadDepths(rect, depth); device.ScenePresent();
    if (std::abs(depth[8 * 16 + 8] - 0.5f) > 1e-6f) throw std::runtime_error("Fixed-function depth bias leaked across consecutive draws");
    GxRsSet(GxRs_PolygonOffset, 0.0f);
    std::fprintf(stderr, "Native fixed-function: %u numerical draws, six independent RGB/alpha operations, four stages, disabled cascade, UV sets/transforms, material color, vertex fog, alpha boundary and normalized post-raster depth offset passed\n", draws);
}
