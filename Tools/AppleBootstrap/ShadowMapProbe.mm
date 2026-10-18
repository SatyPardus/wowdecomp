#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Data.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {
using Vec = std::array<float, 4>;
float Dot(const Vec& a, const Vec& b) {
    float result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result += a[i] * b[i];
    return result;
}
void AlphaPattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texels[] = {0x40102030, 0xC0302010};
    if (command == GxTex_Latch) {
        stride = 8;
        data = texels;
    }
}
} // namespace

void CheckShadowMapShaders(CGxDeviceMetal& device) {
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device;
        CGxShader* vertex[90]{};
        CGxShader* pixel[16]{};
        CGxTex* texture = nullptr;
        CGxTex* target = nullptr;
        CGxTex* copy = nullptr;
        CGxShader* uiVertex[2]{};
        CGxShader* uiPixel[1]{};
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            device.TexDestroy(texture);
            device.TexDestroy(target);
            device.TexDestroy(copy);
            for (auto& shader : uiVertex)
                device.ShaderDestroy(shader);
            for (auto& shader : uiPixel)
                device.ShaderDestroy(shader);
            for (auto& shader : vertex)
                device.ShaderDestroy(shader);
            for (auto& shader : pixel)
                device.ShaderDestroy(shader);
        }
    } cleanup{device};
    device.ShaderCreate(cleanup.vertex, GxSh_Vertex, "Shaders\\Vertex", "ShadowMap", 90);
    device.ShaderCreate(cleanup.pixel, GxSh_Pixel, "Shaders\\Pixel", "ShadowMapSL", 16);
    device.TexCreate(GxTex_2d, 2, 1, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, AlphaPattern,
                     "Shadow cutout reference", cleanup.texture);
    CGxTexFlags targetFlags(GxTex_Nearest);
    targetFlags.m_renderTarget = 1;
    device.TexCreate(GxTex_2d, 32, 32, 0, GxTex_R32F, GxTex_R32F, targetFlags, nullptr, nullptr, "Linear shadow depth", cleanup.target);
    device.TexCreate(GxTex_2d, 32, 32, 0, GxTex_R32F, GxTex_R32F, targetFlags, nullptr, nullptr, "Copied shadow depth", cleanup.copy);
    device.ShaderCreate(cleanup.uiVertex, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    device.ShaderCreate(cleanup.uiPixel, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
    for (auto state : {GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_Fog, GxRs_ScissorTest, GxRs_ClipPlaneMask, GxRs_PolygonOffset,
                       GxRs_PointSprite})
        GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    std::array<Vec, 256> c{};
    c[2] = {0.8f, 0.1f, 0.2f, 0};
    c[3] = {-0.1f, 0.9f, 0, 0};
    c[4] = {0, 0, 0.5f, 0.25f};
    c[5] = {0, 0, 0.25f, 1};
    c[6] = {0.7f, 0.2f, 1.3f, 0.31f};
    c[7] = {0, 0.7f, -0.4f, 0.1f};
    c[14] = {0.9f, 0.1f, -0.2f, 0.12f};
    c[15] = {-0.1f, 0.8f, 0.1f, -0.08f};
    c[16] = {0.1f, -0.1f, 1.2f, 0.2f};
    for (unsigned i = 0; i < 5; ++i) {
        c[31 + i * 3] = {0.7f + i * 0.1f, 0.05f * i, 0, 0.02f * i};
        c[32 + i * 3] = {-0.04f * i, 1, 0.1f * i, -0.03f * i};
        c[33 + i * 3] = {0, 0.07f * i, 0.8f + i * 0.1f, 0.04f * i};
    }
    M2Vertex source{};
    source.m_position = C3Vector(0.1f, -0.15f, 0.4f);
    source.m_texcoord[0] = C2Vector(0.2f, 0.6f);
    // Deliberately non-normalized weights detect unwanted normalization.
    const uint8_t weights[] = {51, 76, 64, 38}, indices[] = {4, 2, 1, 3};
    std::memcpy(&source.m_weights, weights, 4);
    std::memcpy(&source.m_indices, indices, 4);
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    unsigned draws = 0;
    for (unsigned permutation = 0; permutation < 90; ++permutation) {
        const bool floating = permutation >= 30;
        const unsigned skinning = (permutation % 30) / 10;
        Vec rows[3]{};
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned component = 0; component < 4; ++component) {
                if (skinning < 2)
                    rows[row][component] = c[31 + (skinning ? indices[0] * 3 : 0) + row][component];
                else
                    for (unsigned bone = 0; bone < 4; ++bone)
                        rows[row][component] += c[31 + indices[bone] * 3 + row][component] * (weights[bone] / 255.0f);
            }
        const Vec position{source.m_position.x, source.m_position.y, source.m_position.z, 1};
        Vec light{Dot(rows[0], position), Dot(rows[1], position), Dot(rows[2], position), 1};
        if (skinning)
            light = {Dot(c[14], light), Dot(c[15], light), Dot(c[16], light), 1};
        const float w = Dot(c[5], light);
        const unsigned x = std::floor((Dot(c[2], light) / w + 1) * 16), y = std::floor((1 - Dot(c[3], light) / w) * 16);
        for (unsigned cutout = 0; cutout < 2; ++cutout)
            for (unsigned boundary = 0; boundary < 2; ++boundary) {
                const auto format = skinning ? GxVBF_PBNT2 : cutout ? GxVBF_PT : GxVBF_P;
                auto* buffer = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
                auto* data = reinterpret_cast<uint8_t*>(device.BufLock(buffer));
                if (skinning)
                    std::memcpy(data, &source, sizeof(source));
                else {
                    std::memcpy(data, &source.m_position, 12);
                    if (cutout)
                        std::memcpy(data + GxVertexAttribOffset(format, GxVA_TexCoord0), &source.m_texcoord[0], 8);
                }
                device.BufUnlock(buffer, 0);
                GxPrimVertexPtr(buffer, format);
                c[6][3] = permutation & 1 ? -0.15f : 0.31f;
                const float alpha = (permutation & 1 ? 64.0f : 192.0f) / 255;
                Vec scale{0, 0, 0, permutation & 2 ? -0.6f : floating ? 2.12345f : 0.6f};
                Vec alphaRef{0, 0, 0, boundary ? std::nextafter(alpha, 1.0f) : alpha};
                device.BeginFrame(pass);
                if (floating) {
                    device.DeviceSetRenderTarget(GxBuffers_Color, cleanup.target, 0);
                    device.SceneClear(3, CImVector(255, 255, 255, 255));
                }
                GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 256);
                GxShaderConstantsSet(GxSh_Pixel, 0, scale.data(), 1);
                GxShaderConstantsSet(GxSh_Pixel, 2, alphaRef.data(), 1);
                device.RsSet(GxRs_VertexShader, cleanup.vertex[permutation]);
                device.RsSet(GxRs_PixelShader, cleanup.pixel[cutout * 8 + permutation % 8]);
                device.RsSet(GxRs_Texture0, cutout ? cleanup.texture : nullptr);
                GxRsSet(GxRs_AlphaRef, 255); // Tests output alpha, not sampled alpha.
                GxDraw(&batch, 0);
                CiRect rect(0, 0, 32, 32);
                const bool visible = !cutout || !boundary;
                unsigned covered = 0;
                if (floating) {
                    TSGrowableArray<float> pixels;
                    device.ReadFloatColor(rect, pixels);
                    const float expected = visible ? std::max(light[2] * scale[3], 0.0f) : 1.0f;
                    if (std::abs(pixels[y * 32 + x] - expected) > 0.000001f)
                        throw std::runtime_error("R32F shadow depth lost precision or cutout semantics at vertex " +
                                                 std::to_string(permutation));
                    for (unsigned i = 0; i < pixels.Count(); ++i)
                        covered += pixels[i] != 1.0f;
                    device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
                } else {
                    TSGrowableArray<CImVector> pixels;
                    device.DeviceReadPixels(rect, pixels);
                    const int expected = visible ? std::lround(std::clamp(light[2] * scale[3], 0.0f, 1.0f) * 255) : 0;
                    const auto& sample = pixels[y * 32 + x];
                    if (std::abs(int(sample.r) - expected) > 1 || std::abs(int(sample.g) - expected) > 1 || sample.b ||
                        sample.a != (visible ? 255 : 0))
                        throw std::runtime_error("Shadow-map position, linear depth or cutout boundary mismatch at vertex " +
                                                 std::to_string(permutation));
                    for (unsigned i = 0; i < pixels.Count(); ++i)
                        covered += pixels[i].a != 0;
                }
                if (covered != unsigned(visible))
                    throw std::runtime_error("Shadow-map coverage changed");
                device.ScenePresent();
                ++draws;
            }
    }
    device.BeginFrame(pass);
    device.DeviceSetRenderTarget(GxBuffers_Color, cleanup.target, 0);
    device.SceneClear(3, CImVector(255, 255, 255, 255));
    GxXformSetViewport(0, 0.5f, 0, 1, 0, 1);
    device.SceneClear(1, CImVector(0, 0, 128, 255));
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    CiRect full(0, 0, 32, 32);
    TSGrowableArray<float> original, copied;
    device.ReadFloatColor(full, original);
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 32; ++x)
            if (original[y * 32 + x] != (x < 16 ? 128.0f / 255 : 1.0f))
                throw std::runtime_error("Regional R32F clear changed the untouched half");
    device.DeviceCopyTex(cleanup.target, 0, cleanup.copy, 0);
    device.DeviceSetRenderTarget(GxBuffers_Color, cleanup.copy, 0);
    device.ReadFloatColor(full, copied);
    if (std::memcmp(original.Ptr(), copied.Ptr(), 32 * 32 * sizeof(float)))
        throw std::runtime_error("R32F copy changed float bits");
    device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
    device.RsSet(GxRs_VertexShader, cleanup.uiVertex[0]);
    device.RsSet(GxRs_PixelShader, cleanup.uiPixel[0]);
    device.RsSet(GxRs_Texture0, cleanup.copy);
    C44Matrix identity(1);
    device.ShaderConstantsSet(GxSh_Vertex, 0, &identity.a0, 4);
    const float quad[][5] = {{-1, 1, 0, 0, 0}, {-1, -1, 0, 0, 1}, {1, 1, 0, 1, 0}, {1, -1, 0, 1, 1}};
    auto* buffer = GxBufStream(GxPoolTarget_Vertex, sizeof(quad[0]), 4);
    std::memcpy(device.BufLock(buffer), quad, sizeof(quad));
    device.BufUnlock(buffer, 0);
    GxPrimVertexPtr(buffer, GxVBF_PT);
    CGxBatch quadBatch{GxPrim_TriangleStrip, 0, 4, 0, 3};
    device.Draw(&quadBatch, 0);
    TSGrowableArray<CImVector> sampled;
    device.DeviceReadPixels(full, sampled);
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 32; ++x) {
            const auto& pixel = sampled[y * 32 + x];
            if (pixel.r != (x < 16 ? 128 : 255) || pixel.g != 255 || pixel.b != 255 || pixel.a != 255)
                throw std::runtime_error("R32F sampling lost red or D3D9's absent-channel values");
        }
    device.ScenePresent();
    std::fprintf(stderr,
                 "Shadow-map generator: 90 vertex / 16 pixel handles, %u depth/skin/UV/alpha reference draws, R32F "
                 "precision/copy/clear/channel semantics\n",
                 draws);
}
