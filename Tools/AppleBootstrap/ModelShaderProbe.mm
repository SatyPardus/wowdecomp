#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Data.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
using Vec = std::array<float, 4>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Pattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texels[] = {0x00104488, 0x0060A0E0, 0x00C08040, 0x00FFFFFF};
    if (command == GxTex_Latch) { stride = 8; data = texels; }
}
float Dot(const Vec& a, const Vec& b, unsigned count = 4) {
    float result = 0; for (unsigned i = 0; i < count; ++i) result += a[i] * b[i]; return result;
}
float Sat(float value) { return std::clamp(value, 0.0f, 1.0f); }
}

void CheckModelShaders(CGxDeviceMetal& device) {
    CGxShader* vertex[90]{}; CGxShader* pixel[16]{};
    device.ShaderCreate(vertex, GxSh_Vertex, "Shaders\\Vertex", "Diffuse_T1", 90);
    device.ShaderCreate(pixel, GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque", 16);
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader** vertex; CGxShader** pixel; CGxTex* texture = nullptr;
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop(); device.TexDestroy(texture);
            for (unsigned i = 0; i < 90; ++i) device.ShaderDestroy(vertex[i]);
            for (unsigned i = 0; i < 16; ++i) device.ShaderDestroy(pixel[i]);
        }
    } cleanup{device, vertex, pixel};
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, Pattern, "Model material reference", cleanup.texture);
    device.RsSet(GxRs_Texture0, cleanup.texture);
    GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_ScissorTest, 0); GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PolygonOffset, 0); GxRsSet(GxRs_PointSprite, 0); GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque); GxRsSet(GxRs_AlphaRef, 0);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    std::array<Vec, 256> c{};
    c[2] = {0.8f, 0, 0.2f, 0}; c[3] = {0.1f, 0.9f, 0, 0}; c[4] = {0, 0, 1, 0}; c[5] = {0, 0, 0.25f, 1};
    c[6] = {1.3f, 0.2f, 0, 0.05f}; c[7] = {-0.3f, 0.6f, 0, 0.11f};
    c[10] = {0.6f, 0.5f, 0.7f, 0}; c[11] = {0.15f, 0.2f, 0.1f, 0}; c[12] = {0, 0, -1, 0};
    c[25] = {0.7f, 0.9f, 1.1f, 1.3f}; c[26] = {0.1f, 0.2f, 0.3f, 0.4f}; c[27] = {0.03f, 0.05f, 0.07f, 0.09f};
    c[28] = {0.9f, 0.7f, 0.6f, 0.7f}; c[29] = {0.05f, 0.1f, 0.15f, 0}; c[30] = {-0.65f, 0.95f, 1.7f, 0};
    for (unsigned i = 0; i < 4; ++i) {
        c[17 + i] = {0.03f * (i + 1), 0.02f * (4 - i), 0.015f * (i + 1), 0};
        c[21 + i] = {-0.6f + 0.3f * i, 0.7f - 0.2f * i, 1.2f + 0.1f * i, 0};
    }
    for (unsigned i = 0; i < 5; ++i) {
        c[31 + i * 3] = {0.7f + i * 0.1f, 0.05f * i, 0, 0.02f * i};
        c[32 + i * 3] = {-0.04f * i, 1, 0.1f * i, -0.03f * i};
        c[33 + i * 3] = {0, 0.07f * i, 0.8f + i * 0.1f, 0.04f * i};
    }
    M2Vertex source{};
    source.m_position = C3Vector(0.1f, -0.15f, 0.4f); source.m_normal = C3Vector(0.3f, 0.4f, 0.5f);
    source.m_texcoord[0] = C2Vector(0.25f, 0.7f);
    const uint8_t weights[] = {32, 64, 96, 63}, indices[] = {4, 2, 1, 3};
    std::memcpy(&source.m_weights, weights, 4); std::memcpy(&source.m_indices, indices, 4);
    auto* buffer = GxBufStream(GxPoolTarget_Vertex, sizeof(source), 1);
    std::memcpy(device.BufLock(buffer), &source, sizeof(source)); device.BufUnlock(buffer, 0);
    GxPrimVertexPtr(buffer, GxVBF_PBNT2);
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    const Vec fogColor{0.12f, 0.23f, 0.34f, 0.6f};
    // Independently evaluate the original SM3 vertex equations at one vertex.
    // A point checks position, UV matrix, palette indexing, light accumulation,
    // fog and opaque alpha without introducing triangle-interpolation error.
    for (unsigned permutation = 0; permutation < 30; ++permutation) {
        Vec rows[3]{};
        for (unsigned row = 0; row < 3; ++row) for (unsigned component = 0; component < 4; ++component) {
            if (permutation < 20) rows[row][component] = c[31 + (permutation < 10 ? 0 : indices[0] * 3) + row][component];
            else for (unsigned bone = 0; bone < 4; ++bone)
                rows[row][component] += c[31 + indices[bone] * 3 + row][component] * (weights[bone] / 255.0f);
        }
        const Vec position{source.m_position.x, source.m_position.y, source.m_position.z, 1};
        const Vec normal{source.m_normal.x, source.m_normal.y, source.m_normal.z, 0};
        Vec view{Dot(rows[0], position), Dot(rows[1], position), Dot(rows[2], position), 1};
        Vec transformedNormal{Dot(rows[0], normal), Dot(rows[1], normal), Dot(rows[2], normal), 0};
        const float length = std::sqrt(Dot(transformedNormal, transformedNormal, 3));
        for (unsigned i = 0; i < 3; ++i) transformedNormal[i] /= length;
        Vec color = c[28];
        for (unsigned channel = 0; channel < 3; ++channel) {
            float lighting = Sat(transformedNormal[2]) * c[10][channel] + c[11][channel];
            for (unsigned light = 0; light < (permutation / 2) % 5; ++light) {
                Vec delta{}; for (unsigned axis = 0; axis < 3; ++axis) delta[axis] = c[21 + light][axis] - view[axis];
                const float distance = std::sqrt(Dot(delta, delta, 3));
                lighting += c[17 + light][channel] * std::max(0.0f, Dot(delta, transformedNormal, 3) / distance) /
                    (c[25][light] + c[26][light] * distance + c[27][light] * distance * distance);
            }
            color[channel] = Sat(c[28][channel] * ((permutation & 1) ? Sat(lighting) : 1) + c[29][channel]);
        }
        const float fog = std::min(1.0f, std::pow(std::max(0.0f, view[2] * c[30][0] + c[30][1]), c[30][2]));
        const Vec texel{96.0f / 255, 160.0f / 255, 224.0f / 255, 0}; // UV matrix selects (1,0).
        for (unsigned channel = 0; channel < 3; ++channel)
            color[channel] = fog * (texel[channel] * color[channel] - fogColor[channel]) + fogColor[channel];
        const float w = Dot(c[5], view);
        const unsigned x = std::floor((Dot(c[2], view) / w + 1) * 16), y = std::floor((1 - Dot(c[3], view) / w) * 16);
        for (unsigned pixelPermutation : {0u, 4u, 8u, 12u}) {
            device.BeginFrame(pass);
            GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 256);
            GxShaderConstantsSet(GxSh_Pixel, 2, fogColor.data(), 1);
            device.RsSet(GxRs_VertexShader, vertex[permutation]); device.RsSet(GxRs_PixelShader, pixel[pixelPermutation]);
            GxDraw(&batch, 0);
            CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            const auto& sample = pixels[y * 32 + x]; const uint8_t channels[] = {sample.r, sample.g, sample.b, sample.a};
            for (unsigned channel = 0; channel < 4; ++channel)
                if (std::abs(int(channels[channel]) - int(std::lround(color[channel] * 255))) > 2)
                    throw std::runtime_error("Model shader mismatch: VS " + std::to_string(permutation) + " PS " + std::to_string(pixelPermutation) +
                        " channel " + std::to_string(channel) + " at " + std::to_string(x) + "," + std::to_string(y) +
                        " actual " + std::to_string(channels[channel]) + " expected " + std::to_string(color[channel] * 255));
            unsigned covered = 0; for (uint32_t i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            Require(covered == 1, "Model point position or raster coverage changed");
            device.ScenePresent();
        }
    }
    struct AlphaCase { unsigned pixel, stateRef; float shaderRef; bool visible; };
    const float alpha = 128.0f / 255;
    for (const auto& test : {AlphaCase{0, 128, 0.9f, true}, AlphaCase{0, 129, 0, false},
        AlphaCase{8, 0, alpha, true}, AlphaCase{8, 0, std::nextafter(alpha, 1.0f), false},
        AlphaCase{8, 128, alpha, true}, AlphaCase{8, 129, alpha, false}}) {
        device.BeginFrame(pass);
        c[28][3] = alpha; Vec fog = fogColor; fog[3] = test.shaderRef;
        GxShaderConstantsSet(GxSh_Vertex, 28, c[28].data(), 1); GxShaderConstantsSet(GxSh_Pixel, 2, fog.data(), 1);
        device.RsSet(GxRs_VertexShader, vertex[0]); device.RsSet(GxRs_PixelShader, pixel[test.pixel]);
        GxRsSet(GxRs_AlphaRef, int32_t(test.stateRef)); GxDraw(&batch, 0);
        CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
        unsigned covered = 0; for (uint32_t i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
        Require(covered == unsigned(test.visible), "Model shader/state alpha threshold or equality mismatch");
        device.ScenePresent();
    }
    device.BeginFrame(pass);
    device.RsSet(GxRs_VertexShader, vertex[30]); device.RsSet(GxRs_PixelShader, pixel[1]);
    bool rejected = false; try { GxDraw(&batch, 0); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected, "Unimplemented model shadow permutation was silently replaced");
    device.ScenePresent();
}
