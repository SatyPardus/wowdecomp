#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {
using Vec = std::array<float, 4>;
struct Pattern {
    std::array<uint32_t, 4> texels;
    std::array<uint16_t, 4> packed;
    bool usePacked = false;
};
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    auto& p = *static_cast<Pattern*>(context);
    if (command == GxTex_Latch) { stride = p.usePacked ? 4 : 8; data = p.usePacked ? static_cast<const void*>(p.packed.data()) : p.texels.data(); }
}
float Dot(const Vec& a, const Vec& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
float Sat(float x) { return std::clamp(x, 0.0f, 1.0f); }
Vec Rotate(const Vec& point, const std::array<Vec, 37>& c) {
    Vec result{}; for (unsigned j = 0; j < 3; ++j) for (unsigned i = 0; i < 3; ++i) result[j] += point[i] * c[i][j];
    return result;
}
Vec Sample(const Pattern& pattern, float u, float v, bool wrap) {
    if (wrap) { u -= std::floor(u); v -= std::floor(v); }
    const unsigned x = unsigned(std::clamp(int(std::floor(u*2)), 0, 1)), y = unsigned(std::clamp(int(std::floor(v*2)), 0, 1));
    const unsigned value = pattern.usePacked ? pattern.packed[y*2+x] : pattern.texels[y*2+x];
    const unsigned shifts[] = {16, 8, 0, 24}, packedShifts[] = {8, 4, 0, 12};
    Vec result{}; for (unsigned i = 0; i < 4; ++i) result[i] = pattern.usePacked ? ((value >> packedShifts[i]) & 15) / 15.0f : ((value >> shifts[i]) & 255) / 255.0f;
    return result;
}
}

void CheckTerrainShaders(CGxDeviceMetal& device) {
    CGxShader* vs[128]{}; CGxShader* ps[32]{};
    device.ShaderCreate(vs, GxSh_Vertex, "Shaders\\Vertex", "Terrain", 128);
    device.ShaderCreate(ps, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 32);
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader** vs; CGxShader** ps; CGxTex* textures[6]{};
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop(); for (auto& texture : textures) device.TexDestroy(texture);
            for (unsigned i = 0; i < 128; ++i) device.ShaderDestroy(vs[i]);
            for (unsigned i = 0; i < 32; ++i) device.ShaderDestroy(ps[i]);
        }
    } cleanup{device, vs, ps};
    std::array<Pattern, 6> patterns{};
    for (unsigned i = 0; i < 4; ++i) for (unsigned j = 0; j < 4; ++j)
        patterns[i].texels[j] = (uint32_t(50 + i*40 + j*7) << 24) | (uint32_t(30 + i*25 + j*15) << 16) | (uint32_t(110 - i*15 + j*5) << 8) | uint32_t(20 + i*20 + j*12);
    patterns[4].texels = {0xC0203050, 0x4080A0C0, 0x80301020, 0xFF502010};
    patterns[5].packed = {0xC235, 0x48AC, 0x8312, 0xF521}; patterns[5].usePacked = true;
    for (unsigned i = 0; i < 6; ++i) {
        CGxTexFlags flags(GxTex_Nearest); flags.m_wrapU = flags.m_wrapV = i < 4 ? GxTex_Wrap : GxTex_Clamp;
        const auto format = i == 5 ? GxTex_Argb4444 : GxTex_Argb8888;
        device.TexCreate(GxTex_2d, 2, 2, 0, format, format, flags, &patterns[i], Upload, "Terrain shader numeric input", cleanup.textures[i]);
    }
    GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_ScissorTest, 0); GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PolygonOffset, 0); GxRsSet(GxRs_PointSprite, 0); GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque); GxRsSet(GxRs_AlphaRef, 255);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    std::array<Vec, 37> c{};
    c[0] = {0.8f, 0.1f, 0.05f, 0}; c[1] = {-0.2f, 1, 0.1f, 0}; c[2] = {0, 0.2f, 0.9f, 0}; c[3] = {0.03f, -0.02f, -0.7f, 1};
    c[4] = {0.9f, 0, 0, 0}; c[5] = {0, 0.8f, 0, 0}; c[6] = {0.1f, 0, 1, 0}; c[7] = {0, 0.02f, 0.8f, 1};
    c[12] = {-0.4f, 0.55f, 1.3f, 0}; c[23] = {-0.2f, 0.1f, 0, 0};
    for (unsigned i = 0; i < 5; ++i) { c[13+i] = {-0.6f + i*0.4f, 0.2f - i*0.17f, 0, 0}; c[18+i] = {0.8f + i*0.4f, -1.1f + i*0.25f, 0, 0}; }
    c[24] = {0.2f, 0.3f, 0.8f, 0}; c[25] = {0.15f, 0.2f, 0.1f, 0}; c[26] = {0.5f, 0.4f, 0.6f, 0}; c[27] = {0.3f, 0.2f, 0.25f, 3};
    for (unsigned i = 0; i < 3; ++i) {
        c[28+3*i] = {-0.3f + i*0.3f, 0.2f - i*0.1f, 0.5f + i*0.2f, 0};
        c[29+3*i] = {0.08f + i*0.03f, 0.1f - i*0.02f, 0.05f + i*0.04f, 0};
        c[30+3*i] = {0.6f, 0.15f, 0.1f, 0};
    }
    const Vec point{0.1f, -0.15f, 0.4f, 1}, normal{0.3f, 0.4f, 0.8f, 0}, color{128.0f/255, 160.0f/255, 192.0f/255, 1};
    const Vec fogColor{0.12f, 0.23f, 0.34f, 0}, layerLighting{1, 0, -2, -0.0f};
    Vec view = Rotate(point, c); for (unsigned i = 0; i < 3; ++i) view[i] += c[3][i]; view[3] = 1;
    const auto direction = Rotate(normal, c);
    Vec clip{}; for (unsigned j = 0; j < 4; ++j) for (unsigned i = 0; i < 4; ++i) clip[j] += view[i] * c[4+i][j];
    const unsigned x = unsigned((clip[0]/clip[3]+1)*16), y = unsigned((1-clip[1]/clip[3])*16);
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    for (unsigned vp = 0; vp < 128; vp += 4) {
        Vec diffuse{}, specular{};
        for (unsigned j = 0; j < 3; ++j) diffuse[j] = c[25][j] + c[26][j] * Sat(Dot(c[24], direction));
        if (vp & 64) for (unsigned i = 0; i < 3; ++i) {
            Vec delta = Rotate(c[28+3*i], c); for (unsigned j = 0; j < 3; ++j) delta[j] -= view[j];
            const float distance = std::sqrt(Dot(delta, delta)), angle = Sat(Dot(delta, direction) / distance);
            const auto& a = c[30+3*i]; const float attenuation = a[0] + a[1]*distance + a[2]*distance*distance;
            for (unsigned j = 0; j < 3; ++j) diffuse[j] += c[29+3*i][j] * angle / attenuation;
        }
        for (unsigned j = 0; j < 3; ++j) diffuse[j] = std::min(diffuse[j], 1.0f) * ((vp & 4) ? color[j] : 0.5f);
        if (vp & 8) {
            Vec halfway{}; const float distance = std::sqrt(Dot(view, view));
            for (unsigned j = 0; j < 3; ++j) halfway[j] = -view[j]/distance + c[24][j];
            const float length = std::sqrt(Dot(halfway, halfway));
            for (unsigned j = 0; j < 3; ++j) specular[j] = std::pow(std::max(Dot(halfway, direction)/length, 0.0f), c[27][3]) * c[27][j];
        }
        const auto format = (vp & 4) ? GxVBF_PNC : GxVBF_PN;
        auto* vertices = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
        auto* bytes = reinterpret_cast<uint8_t*>(device.BufLock(vertices));
        std::memcpy(bytes, point.data(), 12); std::memcpy(bytes+12, normal.data(), 12);
        const uint32_t packedColor = 0xFF80A0C0; if (vp & 4) std::memcpy(bytes+24, &packedColor, 4);
        device.BufUnlock(vertices, 0); GxPrimVertexPtr(vertices, format);
        const unsigned layers = GxMetal::TerrainLayerCount(GxSh_Vertex, vp);
        for (unsigned mode = 0; mode < 4; ++mode) for (unsigned packed = 0; packed < 2; ++packed) {
            const unsigned pp = 2*(layers-1) + 8*mode;
            std::array<Vec, 5> texels{};
            for (unsigned i = 0; i <= layers; ++i) {
                const unsigned pattern = i == layers ? 4+packed : i;
                const float u = (point[1]-c[23][1])*c[18+i][1] + c[13+i][1];
                const float v = (point[0]-c[23][0])*c[18+i][0] + c[13+i][0];
                texels[i] = Sample(patterns[pattern], u, v, i < layers);
                device.RsSet(EGxRenderState(GxRs_Texture0+i), cleanup.textures[pattern]);
            }
            const Vec blend = texels[layers];
            if (pp & 16) for (unsigned i = 0; i < layers; ++i) if (layerLighting[i] != 0)
                for (unsigned j = 0; j < 3; ++j) texels[i][j] *= 2*diffuse[j];
            Vec mixed = texels[0];
            if (pp & 8) {
                const float base = 1-Sat(blend[0]+blend[1]+blend[2]);
                for (unsigned j = 0; j < 4; ++j) {
                    mixed[j] *= base;
                    for (unsigned i = 1; i < layers; ++i) mixed[j] += texels[i][j]*blend[i-1];
                }
            } else for (unsigned i = 1; i < layers; ++i) for (unsigned j = 0; j < 4; ++j)
                mixed[j] = mixed[j]*(1-blend[i-1]) + texels[i][j]*blend[i-1];
            const float fog = std::min(std::pow(std::max(view[2]*c[12][0]+c[12][1], 0.0f), c[12][2]), 1.0f);
            Vec expected{0, 0, 0, 1};
            for (unsigned j = 0; j < 3; ++j) {
                const float lit = mixed[j]*(blend[3]*0.3f+0.7f)*((pp & 16) ? 1 : 2*diffuse[j]) + mixed[3]*specular[j]*blend[3];
                expected[j] = Sat((lit-fogColor[j])*fog + fogColor[j]);
            }
            device.BeginFrame(pass);
            GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 37);
            GxShaderConstantsSet(GxSh_Pixel, 1, layerLighting.data(), 1); GxShaderConstantsSet(GxSh_Pixel, 2, fogColor.data(), 1);
            device.RsSet(GxRs_VertexShader, vs[vp]); device.RsSet(GxRs_PixelShader, ps[pp]); GxDraw(&batch, 0);
            CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            const auto& sample = pixels[y*32+x]; const uint8_t actual[] = {sample.r, sample.g, sample.b, sample.a};
            for (unsigned j = 0; j < 4; ++j) if (std::abs(int(actual[j])-int(std::lround(expected[j]*255))) > 2)
                throw std::runtime_error("Terrain numeric mismatch VS " + std::to_string(vp) + " PS " + std::to_string(pp) + " packed " + std::to_string(packed) + " channel " + std::to_string(j) + " actual " + std::to_string(actual[j]) + " expected " + std::to_string(expected[j]*255));
            unsigned covered = 0; for (unsigned i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            if (covered != 1) throw std::runtime_error("Terrain point transform or coverage mismatch");
            device.ScenePresent();
        }
    }
    for (auto pair : {std::pair<unsigned, unsigned>{1, 0}, {2, 1}, {0, 2}}) {
        device.BeginFrame(pass); device.RsSet(GxRs_VertexShader, vs[pair.first]); device.RsSet(GxRs_PixelShader, ps[pair.second]);
        bool rejected = false; try { GxDraw(&batch, 0); } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Unsupported/mismatched terrain permutation silently accepted");
        device.ScenePresent();
    }
    std::fprintf(stderr, "Terrain: 32 vertex and 16 pixel variants, 256 draws with independent UV/light/fog/blend references, packed and RGBA alpha, five textures verified\n");
}
