#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include <cstdlib>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
using V = std::array<float, 4>;
float Dot(const V &a, const V &b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
float Sat(float a) { return std::clamp(a, 0.0f, 1.0f); }
V Unit(V a) {
    float length = std::sqrt(Dot(a, a));
    for (unsigned i = 0; i < 3; ++i)
        a[i] /= length;
    return a;
}
V Color(uint32_t a) { return {((a >> 16) & 255) / 255.0f, ((a >> 8) & 255) / 255.0f, (a & 255) / 255.0f, (a >> 24) / 255.0f}; }
V Transform(const std::array<V, 46> &c, unsigned at, const V &v) {
    V result{};
    for (unsigned j = 0; j < 4; ++j)
        for (unsigned i = 0; i < 4; ++i)
            result[j] += c[at + i][j] * v[i];
    return result;
}
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void *p, uint32_t &stride, const void *&data) {
    if (command == GxTex_Latch) {
        stride = 8;
        data = p;
    }
}
V Sample(const std::array<uint32_t, 4> &image, float u, float v, bool wrap) {
    if (wrap) {
        u -= std::floor(u);
        v -= std::floor(v);
    }
    return Color(image[std::clamp(int(std::floor(v * 2)), 0, 1) * 2 + std::clamp(int(std::floor(u * 2)), 0, 1)]);
}
} // namespace
void CheckLiquidShaders(CGxDeviceMetal &device) {
    // The original renderer queues pointers. Exercise both qsort adapters with
    // multiple instances; treating an array slot as an object reads past it.
    Liquid::CInstance instances[3];
    instances[0].m_transform.d3 = 3;
    instances[1].m_transform.d3 = 1;
    instances[2].m_transform.d3 = 2;
    for (unsigned pass = 0; pass < 2; ++pass) {
        Liquid::CInstance *order[] = {&instances[0], &instances[1], &instances[2]};
        std::qsort(order, 3, sizeof(order[0]), Liquid::CRenderer::s_drawListSorter[pass]);
        for (unsigned i = 0; i < 3; ++i)
            if (order[i]->m_transform.d3 != float(pass ? 3 - i : i + 1))
                throw std::runtime_error("Liquid queue ordering changed");
    }

    CGxShader *vs[3][4]{}, *ps[3]{};
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal &device;
        CGxShader *(*vs)[4];
        CGxShader **ps;
        CGxTex *textures[2]{};
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            for (auto &t : textures)
                device.TexDestroy(t);
            for (unsigned i = 0; i < 3; ++i) {
                for (auto &s : vs[i])
                    device.ShaderDestroy(s);
                device.ShaderDestroy(ps[i]);
            }
        }
    } cleanup{device, vs, ps};
    for (unsigned i = 0; i < 3; ++i) {
        auto effect = GxMetal::FragmentEffect(unsigned(GxMetal::FragmentEffect::LiquidWater) + i);
        device.ShaderCreate(vs[i], GxSh_Vertex, "Shaders\\Vertex", GxMetal::LiquidShaderFamily(effect, GxSh_Vertex),
                            GxMetal::LiquidPermutationCount(effect, GxSh_Vertex));
        device.ShaderCreate(&ps[i], GxSh_Pixel, "Shaders\\Pixel", GxMetal::LiquidShaderFamily(effect, GxSh_Pixel), 1);
    }
    std::array<uint32_t, 4> images[2] = {{{0x80402060, 0xA0608030, 0xC0306070, 0xE0703020}},
                                         {{0x70403020, 0x90604070, 0xB0307060, 0xD0502080}}};
    for (unsigned i = 0; i < 2; ++i) {
        CGxTexFlags flags(GxTex_Nearest);
        flags.m_wrapU = flags.m_wrapV = i ? GxTex_Wrap : GxTex_Clamp;
        device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, flags, images[i].data(), Upload, "Liquid material input",
                         cleanup.textures[i]);
        device.RsSet(EGxRenderState(GxRs_Texture0 + i), cleanup.textures[i]);
    }
    for (auto state : {GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_Fog, GxRs_ScissorTest, GxRs_ClipPlaneMask, GxRs_PolygonOffset,
                       GxRs_PointSprite, GxRs_AlphaRef})
        GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    std::array<V, 46> c{};
    c[0] = {0.8f, 0.1f, 0, 0};
    c[1] = {0, 0.9f, 0, 0};
    c[2] = {0, 0, 0.7f, 0};
    c[3] = {0.02f, -0.03f, 0, 1};
    c[4] = {-0.3f, 0.8f, 1.3f, 0};
    c[5] = {0.9f, 0.1f, 0, 0};
    c[6] = {-0.1f, 0.8f, 0.1f, 0};
    c[7] = {0, 0.1f, 1.1f, 0};
    c[8] = {0.05f, 0, 0.3f, 1};
    c[9] = {0.8f, 0.2f, 0, 0};
    c[10] = {0.1f, 1.2f, 0, 0};
    c[12] = {-0.8f, 1.1f, 0, 1};
    c[13] = {0.2f, 0.8f, 0, 0};
    c[14] = {1.1f, 0.1f, 0, 0};
    c[16] = {0.1f, -0.1f, 0, 1};
    c[33] = {-0.3f, 0.2f, -0.85f, 1};
    c[34] = {0.1f, 0.08f, 0.12f, 1};
    c[35] = {0.3f, 0.25f, 0.2f, 1};
    c[36] = {0.2f, 0.3f, 0.1f, 6};
    for (unsigned i = 0; i < 3; ++i) {
        c[37 + i * 3] = {0.4f - i * 0.2f, -0.1f, 1.3f + i * 0.1f, 1};
        c[38 + i * 3] = {0.2f, 0.3f, 0.1f, 1};
        c[39 + i * 3] = {0.8f, 0.1f, 0.05f, 0};
    }
    const V point{0.13f, -0.22f, 0.31f, 1}, fog{0.12f, 0.22f, 0.32f, 1}, color = Color(0xB0604050);
    const auto view = Transform(c, 5, point), clip = Transform(c, 0, view);
    const unsigned x = unsigned(std::ceil((clip[0] / clip[3] + 1) * 16 + .5f)-1), y = unsigned(std::ceil((1 - clip[1] / clip[3]) * 16 + .5f)-1);
    const float visibility = std::min(std::pow(std::max(view[2] * c[4][0] + c[4][1], 0.0f), c[4][2]), 1.0f);
    const float uv[2] = {0.7f, 0.2f}, uv2[2] = {0.2f, 0.7f};
    auto sample = [&](unsigned stage, unsigned matrix, const float *t) {
        return Sample(images[stage], t[0] * c[matrix][0] + t[1] * c[matrix + 1][0] + c[matrix + 3][0],
                      t[0] * c[matrix][1] + t[1] * c[matrix + 1][1] + c[matrix + 3][1], stage != 0);
    };
    for (unsigned material = 0; material < 3; ++material)
        for (unsigned lights = 0; lights < (material == 2 ? 1u : 4u); ++lights)
            for (unsigned orientation = 0; orientation < 2; ++orientation) {
                const V normal = orientation ? V{-0.2f, 0.1f, -1.2f, 0} : V{0.2f, 0.1f, 1.2f, 0};
                const auto direction = Transform(c, 5, normal);
                V lighting{};
                const float sun = Sat(-Dot(c[33], direction));
                for (unsigned j = 0; j < 3; ++j)
                    lighting[j] = c[34][j] + sun * c[35][j];
                for (unsigned i = 0; i < lights; ++i) {
                    V delta{};
                    for (unsigned j = 0; j < 3; ++j)
                        delta[j] = c[37 + i * 3][j] - view[j];
                    float distance = std::sqrt(Dot(delta, delta));
                    const auto &a = c[39 + i * 3];
                    const float weight = Sat(Dot(Unit(delta), direction)) / (a[0] + distance * a[1] + distance * distance * a[2]);
                    for (unsigned j = 0; j < 3; ++j)
                        lighting[j] += weight * c[38 + i * 3][j];
                }
                auto halfway = Unit(view);
                for (unsigned j = 0; j < 3; ++j)
                    halfway[j] += c[33][j];
                halfway = Unit(halfway);
                float specular = std::pow(std::max(-Dot(halfway, direction), 0.0f), c[36][3]);
                auto base = material == 2 ? sample(0, 9, uv) : sample(0, 13, uv2), layer = sample(1, 9, uv);
                V expected{};
                expected[3] = material == 2 ? 1 : base[3] * color[3];
                for (unsigned j = 0; j < 3; ++j) {
                    float shaded = base[j] * color[j];
                    if (material != 2)
                        shaded = shaded * lighting[j] + layer[j];
                    if (material == 0)
                        shaded += (specular * c[36][j] + 0.25f) * layer[3];
                    expected[j] = Sat(fog[j] + visibility * (shaded - fog[j]));
                }
                auto format = material == 2 ? GxVBF_PCT : GxVBF_PNCT2;
                auto *vertex = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
                auto *bytes = reinterpret_cast<uint8_t *>(device.BufLock(vertex));
                std::memset(bytes, 0, GxVertexSize(format));
                auto put = [&](EGxVertexAttrib a, const void *value, size_t size) {
                    std::memcpy(bytes + GxVertexAttribOffset(format, a), value, size);
                };
                const uint32_t packed = 0xB0604050;
                put(GxVA_Position, point.data(), 12);
                put(GxVA_Color0, &packed, 4);
                put(GxVA_TexCoord0, uv, 8);
                if (material != 2) {
                    put(GxVA_Normal, normal.data(), 12);
                    put(GxVA_TexCoord1, uv2, 8);
                }
                device.BufUnlock(vertex, 0);
                GxPrimVertexPtr(vertex, format);
                GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 46);
                GxShaderConstantsSet(GxSh_Pixel, 4, fog.data(), 1);
                device.RsSet(GxRs_VertexShader, vs[material][lights]);
                device.RsSet(GxRs_PixelShader, ps[material]);
                for (unsigned mode = 0; mode < 3; ++mode) {
                    const unsigned alpha = mode == 0 ? 0 : mode == 1 ? 64 : 255;
                    GxRsSet(GxRs_AlphaRef, int32_t(alpha));
                    device.BeginFrame(pass);
                    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
                    device.Draw(&batch, 0);
                    CiRect rect(0, 0, 32, 32);
                    TSGrowableArray<CImVector> pixels;
                    device.DeviceReadPixels(rect, pixels);
                    const bool discarded = expected[3] < alpha / 255.0f;
                    unsigned covered = 0;
                    for (unsigned i = 0; i < pixels.Count(); ++i)
                        covered += pixels[i].a != 0;
                    const auto got = pixels[y * 32 + x];
                    const unsigned channels[] = {got.r, got.g, got.b, got.a};
                    for (unsigned j = 0; j < 4; ++j)
                        if (std::abs(float(channels[j]) - (discarded ? 0 : expected[j] * 255)) > 2)
                            throw std::runtime_error("Liquid reference mismatch: material " + std::to_string(material) + ", lights " +
                                                     std::to_string(lights) + ", orientation " + std::to_string(orientation) +
                                                     ", channel " + std::to_string(j));
                    if (covered != (discarded ? 0u : 1u))
                        throw std::runtime_error("Liquid transform or point coverage mismatch");
                    device.ScenePresent();
                }
            }
}
