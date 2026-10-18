#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "WoW/Source/WorldClient/Map.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {
// Borrow the original world binding globals only for this isolated probe.
struct OriginalTerrainShadows {
    CGxShader* environment = CMap::pixelShader_Terrain0_env;
    std::array<CGxShader*, 3> shaders0;
    std::array<CGxShader*, 32> shaders2;
    std::array<CGxShader*, 96> shaders3;
    CShadowBand dynamic = CShadowCache::s_extDynShadow;
    std::array<CShadowBand, 3> bands;
    std::array<C4Vector, 12> matrices;
    std::array<C4Vector, 8> offsets;
    EShadowMode mode = CShadowCache::s_shadowMode;
    int32_t recreate = CShadowCache::s_recreateTextures;
    uint32_t pcf = CShaderEffect::s_usePcfFiltering, size = CShadowCache::s_shadowSize;
    std::array<float, 2> textureSize;
    OriginalTerrainShadows() {
        CMap::pixelShader_Terrain0_env = nullptr;
        std::copy_n(CMap::pixelShader_Terrain0, 3, shaders0.begin());
        std::fill_n(CMap::pixelShader_Terrain0, 3, nullptr);
        std::copy_n(CMap::pixelShader_Terrain2, 32, shaders2.begin());
        std::copy_n(CMap::pixelShader_Terrain3, 96, shaders3.begin());
        std::fill_n(CMap::pixelShader_Terrain2, 32, nullptr);
        std::fill_n(CMap::pixelShader_Terrain3, 96, nullptr);
        std::copy_n(CShadowCache::s_extShadowBands, 3, bands.begin());
        std::copy_n(CShadowCache::s_shadowMatrices, 12, matrices.begin());
        std::copy_n(CShadowCache::s_pcfOffsets, 8, offsets.begin());
        std::copy_n(CShadowCache::s_textureSize, 2, textureSize.begin());
    }
    ~OriginalTerrainShadows() {
        g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain0_env);
        CMap::pixelShader_Terrain0_env = environment;
        for (auto& shader : CMap::pixelShader_Terrain0)
            g_theGxDevicePtr->ShaderDestroy(shader);
        std::copy(shaders0.begin(), shaders0.end(), CMap::pixelShader_Terrain0);
        for (auto& shader : CMap::pixelShader_Terrain2)
            g_theGxDevicePtr->ShaderDestroy(shader);
        for (auto& shader : CMap::pixelShader_Terrain3)
            g_theGxDevicePtr->ShaderDestroy(shader);
        std::copy(shaders2.begin(), shaders2.end(), CMap::pixelShader_Terrain2);
        std::copy(shaders3.begin(), shaders3.end(), CMap::pixelShader_Terrain3);
        CShadowCache::s_extDynShadow = dynamic;
        std::copy(bands.begin(), bands.end(), CShadowCache::s_extShadowBands);
        std::copy(matrices.begin(), matrices.end(), CShadowCache::s_shadowMatrices);
        std::copy(offsets.begin(), offsets.end(), CShadowCache::s_pcfOffsets);
        std::copy(textureSize.begin(), textureSize.end(), CShadowCache::s_textureSize);
        CShadowCache::s_shadowMode = mode;
        CShadowCache::s_shadowSize = size;
        CShadowCache::s_recreateTextures = recreate;
        CShaderEffect::s_usePcfFiltering = pcf;
    }
};
using Vec = std::array<float, 4>;
struct Pattern {
    std::array<uint32_t, 4> texels;
    std::array<uint16_t, 4> packed;
    bool usePacked = false;
};
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    auto& p = *static_cast<Pattern*>(context);
    if (command == GxTex_Latch) {
        stride = p.usePacked ? 4 : 8;
        data = p.usePacked ? static_cast<const void*>(p.packed.data()) : p.texels.data();
    }
}
float Dot(const Vec& a, const Vec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
float Sat(float x) { return std::clamp(x, 0.0f, 1.0f); }
Vec Rotate(const Vec& point, const std::array<Vec, 49>& c) {
    Vec result{};
    for (unsigned j = 0; j < 3; ++j)
        for (unsigned i = 0; i < 3; ++i)
            result[j] += point[i] * c[i][j];
    return result;
}
Vec Sample(const Pattern& pattern, float u, float v, bool wrap) {
    if (wrap) {
        u -= std::floor(u);
        v -= std::floor(v);
    }
    const unsigned x = unsigned(std::clamp(int(std::floor(u * 2)), 0, 1)), y = unsigned(std::clamp(int(std::floor(v * 2)), 0, 1));
    const unsigned value = pattern.usePacked ? pattern.packed[y * 2 + x] : pattern.texels[y * 2 + x];
    const unsigned shifts[] = {16, 8, 0, 24}, packedShifts[] = {8, 4, 0, 12};
    Vec result{};
    for (unsigned i = 0; i < 4; ++i)
        result[i] = pattern.usePacked ? ((value >> packedShifts[i]) & 15) / 15.0f : ((value >> shifts[i]) & 255) / 255.0f;
    return result;
}
} // namespace

void CheckTerrainShaders(CGxDeviceMetal& device) {
    CGxShader* vs[128]{};
    CGxShader* ps[32]{};
    CGxShader* ps2[32]{};
    CGxShader* ps3[96]{};
    device.ShaderCreate(vs, GxSh_Vertex, "Shaders\\Vertex", "Terrain", 128);
    device.ShaderCreate(ps, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 32);
    device.ShaderCreate(ps2, GxSh_Pixel, "Shaders\\Pixel", "Terrain2", 32);
    device.ShaderCreate(ps3, GxSh_Pixel, "Shaders\\Pixel", "Terrain3", 96);
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device;
        CGxShader** vs;
        CGxShader** ps;
        CGxShader** ps2;
        CGxShader** ps3;
        CGxTex* textures[7]{};
        CTexture* shadows[4]{};
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
            device.RsPop();
            for (auto& texture : textures)
                device.TexDestroy(texture);
            for (auto* texture : shadows)
                if (texture)
                    HandleClose(reinterpret_cast<HOBJECT>(texture));
            for (unsigned i = 0; i < 128; ++i)
                device.ShaderDestroy(vs[i]);
            for (unsigned i = 0; i < 32; ++i)
                device.ShaderDestroy(ps[i]);
            for (unsigned i = 0; i < 32; ++i)
                device.ShaderDestroy(ps2[i]);
            for (unsigned i = 0; i < 96; ++i)
                device.ShaderDestroy(ps3[i]);
        }
    } cleanup{device, vs, ps, ps2, ps3};
    OriginalTerrainShadows original;
    device.ShaderCreate(CMap::pixelShader_Terrain0, GxSh_Pixel, "Shaders\\Pixel", "Terrain0", 3);
    device.ShaderCreate(&CMap::pixelShader_Terrain0_env, GxSh_Pixel, "Shaders\\Pixel", "Terrain0_env", 1);
    CShaderEffect::s_usePcfFiltering = 0;
    CMap::InitializePCFShaders();
    for (unsigned i = 0; i < 32; ++i)
        if (ps2[i] != CMap::pixelShader_Terrain2[i])
            throw std::runtime_error("Terrain2 shader cache mismatch");
    for (unsigned i = 0; i < 96; ++i)
        if (ps3[i] != CMap::pixelShader_Terrain3[i])
            throw std::runtime_error("Terrain3 shader cache mismatch");
    CShadowCache::s_recreateTextures = 0;
    CShadowCache::s_shadowSize = 8;
    CShadowCache::InitPcfOffsets();
    std::array<Pattern, 6> patterns{};
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < 4; ++j)
            patterns[i].texels[j] = (uint32_t(50 + i * 40 + j * 7) << 24) | (uint32_t(30 + i * 25 + j * 15) << 16) |
                                    (uint32_t(110 - i * 15 + j * 5) << 8) | uint32_t(20 + i * 20 + j * 12);
    patterns[4].texels = {0xC0203050, 0x4080A0C0, 0x80301020, 0xFF502010};
    patterns[5].packed = {0xC235, 0x48AC, 0x8312, 0xF521};
    patterns[5].usePacked = true;
    for (unsigned i = 0; i < 6; ++i) {
        CGxTexFlags flags(GxTex_Nearest);
        flags.m_wrapU = flags.m_wrapV = i < 4 ? GxTex_Wrap : GxTex_Clamp;
        const auto format = i == 5 ? GxTex_Argb4444 : GxTex_Argb8888;
        device.TexCreate(GxTex_2d, 2, 2, 0, format, format, flags, &patterns[i], Upload, "Terrain shader numeric input",
                         cleanup.textures[i]);
    }
    std::array<Pattern, 6> faces;
    for (unsigned face = 0; face < 6; ++face) {
        faces[face].texels = {0x80304050u + face * 0x00100805u, 0xA0207030u + face * 0x00100510u, 0xC0702020u + face * 0x00051010u,
                              0xE0303040u + face * 0x00101008u};
    }
    CGxTexFlags cubeFlags(GxTex_Nearest);
    cubeFlags.m_wrapU = cubeFlags.m_wrapV = GxTex_Clamp;
    device.TexCreate(
        GxTex_CubeMap, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, cubeFlags, faces.data(),
        [](EGxTexCommand command, uint32_t w, uint32_t h, uint32_t face, uint32_t mip, void* arg, uint32_t& stride, const void*& data) {
            Upload(command, w, h, face, mip, static_cast<Pattern*>(arg) + face, stride, data);
        },
        "Terrain cube reference", cleanup.textures[6]);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_ScissorTest, 0);
    GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PolygonOffset, 0);
    GxRsSet(GxRs_PointSprite, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxRsSet(GxRs_AlphaRef, 255);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    // Generate real float shadow targets through Gx regional clears. Distinct
    // bands and asymmetric depths make kernel selection and orientation visible.
    const uint8_t depths[] = {0, 64, 128, 192, 255};
    std::array<std::array<float, 64>, 4> shadowDepths{};
    device.BeginFrame(pass);
    for (unsigned band = 0; band < 4; ++band) {
        CGxTexFlags flags(GxTex_Nearest);
        flags.m_renderTarget = 1;
        flags.m_wrapU = flags.m_wrapV = GxTex_Clamp;
        cleanup.shadows[band] = TextureCreate(
            GxTex_2d, 8, 8, 0, GxTex_R32F, GxTex_R32F, flags, nullptr,
            [](EGxTexCommand, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t&, const void*&) {}, "Terrain shadow reference", 0);
        if (!cleanup.shadows[band])
            throw std::runtime_error("Services terrain shadow target creation failed");
        auto* target = TextureGetGxTex(cleanup.shadows[band], 1, nullptr);
        if (!target)
            throw std::runtime_error("Services shadow target has no Gx resource");
        device.DeviceSetRenderTarget(GxBuffers_Color, target, 0);
        if (!band)
            CShadowCache::s_extDynShadow.m_textures[0] = cleanup.shadows[band];
        else {
            CShadowCache::s_extShadowBands[band - 1].m_current = 0;
            CShadowCache::s_extShadowBands[band - 1].m_textures[0] = cleanup.shadows[band];
        }
        for (unsigned y = 0; y < 8; ++y)
            for (unsigned x = 0; x < 8; ++x) {
                const auto depth = depths[(x * 3 + y * 2 + band) % 5];
                shadowDepths[band][y * 8 + x] = depth / 255.0f;
                GxXformSetViewport(x / 8.0f, (x + 1) / 8.0f, y / 8.0f, (y + 1) / 8.0f, 0, 1);
                device.SceneClear(1, CImVector(0, 0, depth, 255));
            }
        CiRect rect(0, 0, 8, 8);
        TSGrowableArray<float> actual;
        device.ReadFloatColor(rect, actual);
        for (unsigned i = 0; i < 64; ++i)
            if (actual[i] != shadowDepths[band][i])
                throw std::runtime_error("Terrain shadow reference target initialization mismatch");
    }
    device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    device.ScenePresent();
    const std::array<Vec, 8> offsets{{{0.8f / 8, -1.0f / 8, 0, 0},
                                      {-0.2f / 8, -0.8f / 8, 0, 0},
                                      {0.2f / 8, -0.6f / 8, 0, 0},
                                      {1.0f / 8, -0.4f / 8, 0, 0},
                                      {-0.6f / 8, -0.2f / 8, 0, 0},
                                      {0.6f / 8, 0.2f / 8, 0, 0},
                                      {-1.0f / 8, -0.4f / 8, 0, 0},
                                      {-0.4f / 8, -0.6f / 8, 0, 0}}};
    auto shadowSamples = [&](unsigned band, const Vec& coord, bool nine) {
        unsigned visible = 0;
        for (unsigned tap = 0; tap < (nine ? 9u : 5u); ++tap) {
            const float u = coord[0] * 0.5f + 0.5f + (tap ? offsets[(tap - 1) * (nine ? 1 : 2)][0] : 0);
            const float v = coord[1] * 0.5f + 0.5f + (tap ? offsets[(tap - 1) * (nine ? 1 : 2)][1] : 0);
            const int x = std::clamp(int(std::floor(u * 8)), 0, 7), y = std::clamp(int(std::floor(v * 8)), 0, 7);
            visible += shadowDepths[band][y * 8 + x] >= coord[2];
        }
        return visible / (nine ? 9.0f : 5.0f);
    };
    std::array<Vec, 49> c{};
    c[0] = {0.8f, 0.1f, 0.05f, 0};
    c[1] = {-0.2f, 1, 0.1f, 0};
    c[2] = {0, 0.2f, 0.9f, 0};
    c[3] = {0.03f, -0.02f, -0.7f, 1};
    c[4] = {0.9f, 0, 0, 0};
    c[5] = {0, 0.8f, 0, 0};
    c[6] = {0.1f, 0, 1, 0};
    c[7] = {0, 0.02f, 0.8f, 1};
    c[8] = {0.8f, -0.3f, 0.1f, 0};
    c[9] = {0.2f, 0.9f, -0.1f, 0};
    c[10] = {-0.05f, 0.1f, 0.95f, 0};
    c[12] = {-0.4f, 0.55f, 1.3f, 0};
    c[23] = {-0.2f, 0.1f, 0, 0};
    for (unsigned i = 0; i < 5; ++i) {
        c[13 + i] = {-0.6f + i * 0.4f, 0.2f - i * 0.17f, 0, 0};
        c[18 + i] = {0.8f + i * 0.4f, -1.1f + i * 0.25f, 0, 0};
    }
    c[24] = {0.2f, 0.3f, 0.8f, 0};
    c[25] = {0.15f, 0.2f, 0.1f, 0};
    c[26] = {0.5f, 0.4f, 0.6f, 0};
    c[27] = {0.3f, 0.2f, 0.25f, 3};
    for (unsigned i = 0; i < 3; ++i) {
        c[28 + 3 * i] = {-0.3f + i * 0.3f, 0.2f - i * 0.1f, 0.5f + i * 0.2f, 0};
        c[29 + 3 * i] = {0.08f + i * 0.03f, 0.1f - i * 0.02f, 0.05f + i * 0.04f, 0};
        c[30 + 3 * i] = {0.6f, 0.15f, 0.1f, 0};
    }
    const Vec point{0.1f, -0.15f, 0.4f, 1}, normal{0.3f, 0.4f, 0.8f, 0}, color{128.0f / 255, 160.0f / 255, 192.0f / 255, 1};
    const Vec fogColor{0.12f, 0.23f, 0.34f, 0}, layerLighting{1, 0, -2, -0.0f};
    Vec view = Rotate(point, c);
    for (unsigned i = 0; i < 3; ++i)
        view[i] += c[3][i];
    view[3] = 1;
    const auto direction = Rotate(normal, c);
    Vec clip{};
    for (unsigned j = 0; j < 4; ++j)
        for (unsigned i = 0; i < 4; ++i)
            clip[j] += view[i] * c[4 + i][j];
    const unsigned x = unsigned((clip[0] / clip[3] + 1) * 16), y = unsigned((1 - clip[1] / clip[3]) * 16);
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    unsigned draws = 0;
    for (unsigned vp = 0; vp < 128; vp += 2) {
        const bool environment = (vp & 2) != 0;
        Vec diffuse{}, specular{};
        for (unsigned j = 0; j < 3; ++j)
            diffuse[j] = c[25][j] + c[26][j] * Sat(Dot(c[24], direction));
        if (vp & 64)
            for (unsigned i = 0; i < 3; ++i) {
                Vec delta = Rotate(c[28 + 3 * i], c);
                for (unsigned j = 0; j < 3; ++j)
                    delta[j] -= view[j];
                const float distance = std::sqrt(Dot(delta, delta)), angle = Sat(Dot(delta, direction) / distance);
                const auto& a = c[30 + 3 * i];
                const float attenuation = a[0] + a[1] * distance + a[2] * distance * distance;
                for (unsigned j = 0; j < 3; ++j)
                    diffuse[j] += c[29 + 3 * i][j] * angle / attenuation;
            }
        for (unsigned j = 0; j < 3; ++j)
            diffuse[j] = std::min(diffuse[j], 1.0f) * ((vp & 4) ? color[j] : 0.5f);
        if (vp & 8) {
            Vec halfway{};
            const float distance = std::sqrt(Dot(view, view));
            for (unsigned j = 0; j < 3; ++j)
                halfway[j] = -view[j] / distance + c[24][j];
            const float length = std::sqrt(Dot(halfway, halfway));
            for (unsigned j = 0; j < 3; ++j)
                specular[j] = std::pow(std::max(Dot(halfway, direction) / length, 0.0f), c[27][3]) * c[27][j];
        }
        const auto format = (vp & 4) ? GxVBF_PNC : GxVBF_PN;
        auto* vertices = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
        auto* bytes = reinterpret_cast<uint8_t*>(device.BufLock(vertices));
        std::memcpy(bytes, point.data(), 12);
        std::memcpy(bytes + 12, normal.data(), 12);
        const uint32_t packedColor = 0xFF80A0C0;
        if (vp & 4)
            std::memcpy(bytes + 24, &packedColor, 4);
        device.BufUnlock(vertices, 0);
        GxPrimVertexPtr(vertices, format);
        const unsigned layers = GxMetal::TerrainLayerCount(GxSh_Vertex, vp);
        for (unsigned mode = 0; mode < 4; ++mode)
            for (unsigned packed = 0; packed < 2; ++packed)
                for (unsigned receiver = 0; receiver < 8; ++receiver) {
                    const bool multipass = receiver >= 5;
                    if (multipass && (layers != 1 || (environment && receiver != 6)))
                        continue;
                    const unsigned shadowMode = receiver && !multipass ? receiver == 1 ? 1 : receiver - 1 : 0;
                    const unsigned activeVp = vp | unsigned(multipass ? packed : receiver != 0);
                    const unsigned pp = (multipass ? 0 : 2 * (layers - 1) + 8 * mode) + unsigned(environment);
                    Vec channel{};
                    channel[mode ? mode - 1 : 3] = 1;
                    std::array<Vec, 5> texels{};
                    for (unsigned i = 0; i <= layers; ++i) {
                        const unsigned pattern = i == layers ? 4 + packed : i;
                        const float u = (point[1] - c[23][1]) * c[18 + i][1] + c[13 + i][1];
                        const float v = (point[0] - c[23][0]) * c[18 + i][0] + c[13 + i][0];
                        texels[i] = Sample(patterns[pattern], u, v, i < layers);
                        device.RsSet(EGxRenderState(GxRs_Texture0 + i), cleanup.textures[pattern]);
                    }
                    if (environment) {
                        Vec reflection{}, direction{};
                        const auto normalView = Rotate(normal, c);
                        for (unsigned i = 0; i < 3; ++i)
                            reflection[i] = view[i] - 2 * Dot(view, normalView) * normalView[i];
                        for (unsigned j = 0; j < 3; ++j)
                            for (unsigned i = 0; i < 3; ++i)
                                direction[j] += reflection[i] * c[8 + i][j == 1 ? 2 : j == 2 ? 1 : 0];
                        unsigned axis = 0;
                        for (unsigned i = 1; i < 3; ++i)
                            if (std::abs(direction[i]) > std::abs(direction[axis]))
                                axis = i;
                        const bool negative = direction[axis] < 0;
                        const unsigned face = axis * 2 + negative;
                        const float major = std::abs(direction[axis]);
                        float u = axis == 0   ? (negative ? direction[2] : -direction[2])
                                  : axis == 1 ? direction[0]
                                              : (negative ? -direction[0] : direction[0]);
                        float v = axis == 1 ? (negative ? -direction[2] : direction[2]) : -direction[1];
                        texels[0] = Sample(faces[face], u / major * 0.5f + 0.5f, v / major * 0.5f + 0.5f, false);
                        device.RsSet(GxRs_Texture0, cleanup.textures[6]);
                    }
                    const Vec blend = texels[layers];
                    if (pp & 16)
                        for (unsigned i = 0; i < layers; ++i)
                            if (layerLighting[i] != 0)
                                for (unsigned j = 0; j < 3; ++j)
                                    texels[i][j] *= 2 * diffuse[j];
                    Vec mixed = texels[0];
                    if (pp & 8) {
                        const float base = 1 - Sat(blend[0] + blend[1] + blend[2]);
                        for (unsigned j = 0; j < 4; ++j) {
                            mixed[j] *= base;
                            for (unsigned i = 1; i < layers; ++i)
                                mixed[j] += texels[i][j] * blend[i - 1];
                        }
                    } else
                        for (unsigned i = 1; i < layers; ++i)
                            for (unsigned j = 0; j < 4; ++j)
                                mixed[j] = mixed[j] * (1 - blend[i - 1]) + texels[i][j] * blend[i - 1];
                    const float fog = std::min(std::pow(std::max(view[2] * c[12][0] + c[12][1], 0.0f), c[12][2]), 1.0f);
                    float visibility = blend[3];
                    if (receiver && !multipass) {
                        const unsigned scenario = (vp / 4 + mode * 2 + packed) % 8;
                        // 0.99 is numerically ambiguous: the authored edge coefficients
                        // yield 0 with separate float operations but 7.65e-9 with
                        // fma. Raster interpolation can choose either side across
                        // GPUs. Test stable positions on both sides; retain the
                        // original shader threshold and the exact band boundary 1.
                        const float dynamicX[] = {-0.4f, 0.8f, 0.995f, 1.1f, 0.0f, 0.95f, -0.9f, 1.0f};
                        const float z = scenario < 4 ? 128.0f / 255 : scenario == 4 ? 0 : scenario == 5 ? 1 : scenario == 6 ? -0.1f : 1.1f;
                        std::array<Vec, 4> coordinates{{{dynamicX[scenario], -0.2f, z, 1},
                                                        {scenario % 4 == 0 ? 0.3f : 1.0f, -0.1f, z, 1},
                                                        {scenario % 4 == 1 ? -0.3f : -1.0f, 0.2f, z, 1},
                                                        {scenario % 2 ? 0.95f : 1.1f, -0.3f, z, 1}}};
                        // Nontrivial rows exercise the view-space affine mapping. Keep
                        // boundary components exact so <1 and depth equality are tested.
                        for (unsigned band = 0; band < 4; ++band)
                            for (unsigned j = 0; j < 3; ++j) {
                                auto& row = c[37 + band * 3 + j];
                                row = {0, 0, 0, coordinates[band][j]};
                                if (j == 1) {
                                    row[0] = 0.2f + band * 0.1f;
                                    row[1] = -0.3f;
                                    row[2] = 0.15f;
                                    row[3] -= Dot(row, view);
                                }
                            }
                        const float border = Sat(std::max(std::abs(coordinates[0][0]), std::abs(coordinates[0][1])) * -3.4482758045196533f +
                                                 3.4137930870056152f);
                        float bandVisibility = 1;
                        if (shadowMode >= 2) {
                            unsigned band = 1;
                            while (band < 3 && std::max(std::abs(coordinates[band][0]), std::abs(coordinates[band][1])) >= 1)
                                ++band;
                            bandVisibility = shadowSamples(band, coordinates[band], false);
                            if (band == 3) {
                                const float fade =
                                    Sat(std::max(std::abs(coordinates[3][0]), std::abs(coordinates[3][1])) * -11.11111068725586f + 11.0f);
                                bandVisibility = 1 - fade * (1 - bandVisibility);
                            }
                        }
                        if (shadowMode == 3)
                            visibility = border > 0 ? shadowSamples(0, coordinates[0], true) : bandVisibility;
                        else
                            visibility = std::min(1 - border * (1 - shadowSamples(0, coordinates[0], false)),
                                                  shadowMode == 1 ? blend[3] : bandVisibility);
                        const auto mask = GxMetal::TerrainTextureMask(pp, shadowMode);
                        for (unsigned slot = 0; slot < 9; ++slot) {
                            if (!(mask & (1u << slot)))
                                device.RsSet(EGxRenderState(GxRs_Texture0 + slot), static_cast<CGxTex*>(nullptr));
                            else if (slot >= 5)
                                device.RsSet(EGxRenderState(GxRs_Texture0 + slot), static_cast<CGxTex*>(nullptr));
                        }
                    }
                    const float layerAlpha = receiver == 5 ? Dot(blend, channel) : 1;
                    const float layerWeight = receiver == 6   ? 1 - Sat(blend[0] + blend[1] + blend[2])
                                              : receiver == 7 ? Dot(blend, channel)
                                                              : 1;
                    const unsigned alphaRef = receiver == 5 ? mode ? unsigned(std::lround(layerAlpha * 255)) + packed : 0 : 255;
                    const bool discard = alphaRef && layerAlpha < alphaRef / 255.0f;
                    GxRsSet(GxRs_AlphaRef, int32_t(alphaRef));
                    Vec expected{0, 0, 0, layerAlpha};
                    for (unsigned j = 0; j < 3; ++j) {
                        const float lit =
                            mixed[j] * (visibility * 0.3f + 0.7f) * ((pp & 16) ? 1 : 2 * diffuse[j]) + mixed[3] * specular[j] * visibility;
                        expected[j] = Sat((lit * layerWeight - fogColor[j]) * fog + fogColor[j]);
                    }
                    if (discard)
                        expected = {};
                    device.BeginFrame(pass);
                    GxShaderConstantsSet(GxSh_Pixel, 0, channel.data(), 1);
                    GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 49);
                    GxShaderConstantsSet(GxSh_Pixel, 3, offsets[0].data(), 8);
                    GxShaderConstantsSet(GxSh_Pixel, 1, layerLighting.data(), 1);
                    GxShaderConstantsSet(GxSh_Pixel, 2, fogColor.data(), 1);
                    device.RsSet(GxRs_VertexShader, vs[activeVp]);
                    const unsigned shadowPp = 48 * (mode / 2) + 24 * (mode % 2) + 8 * (shadowMode ? shadowMode - 1 : 0) + 2 * (layers - 1) +
                                              unsigned(environment);
                    auto* pixelShader = multipass       ? CMap::GetPixelShader(receiver != 5, receiver != 7, environment)
                                        : receiver == 0 ? ps[pp]
                                        : receiver == 1 ? ps2[pp]
                                                        : ps3[shadowPp];
                    if (receiver && !multipass) {
                        std::memcpy(CShadowCache::s_shadowMatrices, c[37].data(), sizeof(CShadowCache::s_shadowMatrices));
                        CShadowCache::s_shadowMode = shadowMode >= 2 ? SHADOW_HIGH : SHADOW_MEDIUM;
                        CShadowCache::SetShadowMapTerrain();
                        if (receiver != 1) {
                            auto* selected = CMap::GetTerrainPixelShader(mode % 2, layers, shadowMode, environment, mode / 2);
                            if (selected != pixelShader)
                                throw std::runtime_error("Original CMap terrain receiver selection mismatch");
                            pixelShader = selected;
                        }
                    }
                    device.RsSet(GxRs_PixelShader, pixelShader);
                    GxDraw(&batch, 0);
                    ++draws;
                    CiRect rect(0, 0, 32, 32);
                    TSGrowableArray<CImVector> pixels;
                    device.DeviceReadPixels(rect, pixels);
                    const auto& sample = pixels[y * 32 + x];
                    const uint8_t actual[] = {sample.r, sample.g, sample.b, sample.a};
                    for (unsigned j = 0; j < 4; ++j)
                        if (std::abs(int(actual[j]) - int(std::lround(expected[j] * 255))) > 2)
                            throw std::runtime_error("Terrain numeric mismatch receiver " + std::to_string(receiver) + " VS " +
                                                     std::to_string(activeVp) + " PS " + std::to_string(pp) + " packed " +
                                                     std::to_string(packed) + " channel " + std::to_string(j) + " actual " +
                                                     std::to_string(actual[j]) + " expected " + std::to_string(expected[j] * 255));
                    unsigned covered = 0;
                    for (unsigned i = 0; i < pixels.Count(); ++i)
                        covered += pixels[i].a || pixels[i].r || pixels[i].g || pixels[i].b;
                    if (covered != unsigned(!discard))
                        throw std::runtime_error("Terrain point transform or coverage mismatch");
                    device.ScenePresent();
                }
    }
    for (auto pair : {std::pair<unsigned, unsigned>{1, 0}, {2, 0}, {0, 2}}) {
        device.BeginFrame(pass);
        device.RsSet(GxRs_VertexShader, vs[pair.first]);
        device.RsSet(GxRs_PixelShader, ps[pair.second]);
        bool rejected = false;
        try {
            GxDraw(&batch, 0);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        if (!rejected)
            throw std::runtime_error("Unsupported/mismatched terrain permutation silently accepted");
        device.ScenePresent();
    }
    std::fprintf(stderr,
                 "Terrain: 128 vertex / 164 pixel variants, %u independent UV/reflection/light/fog/layer/shadow draws; original "
                 "CMap/ShadowCache selection verified\n",
                 draws);
}

void CheckTerrainShadowGenerator(CGxDeviceMetal& device) {
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device;
        CGxShader* vertex[128]{};
        CGxShader* pixel = nullptr;
        CGxTex* target = nullptr;
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
            device.RsPop();
            device.TexDestroy(target);
            for (auto& shader : vertex)
                device.ShaderDestroy(shader);
            device.ShaderDestroy(pixel);
        }
    } cleanup{device};
    device.ShaderCreate(cleanup.vertex, GxSh_Vertex, "Shaders\\Vertex", "Terrain", 128);
    device.ShaderCreate(&cleanup.pixel, GxSh_Pixel, "Shaders\\Pixel", "TerrainSM", 1);
    CGxTexFlags flags(GxTex_Nearest);
    flags.m_renderTarget = 1;
    device.TexCreate(GxTex_2d, 16, 16, 0, GxTex_R32F, GxTex_R32F, flags, nullptr, nullptr, "Terrain radial distance", cleanup.target);
    for (auto state : {GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_Fog, GxRs_ScissorTest, GxRs_ClipPlaneMask, GxRs_PolygonOffset,
                       GxRs_PointSprite})
        device.RsSet(state, 0);
    for (unsigned i = 0; i < 16; ++i)
        device.RsSet(EGxRenderState(GxRs_Texture0 + i), static_cast<CGxTex*>(nullptr));
    device.RsSet(GxRs_ColorWrite, 15);
    device.RsSet(GxRs_BlendingMode, GxBlend_Opaque);
    device.RsSet(GxRs_AlphaRef, 255);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    std::array<Vec, 49> c{};
    c[0] = {.8f, .1f, 0, 0};
    c[1] = {-.2f, .9f, .1f, 0};
    c[2] = {0, -.1f, .7f, 0};
    c[3] = {.04f, -.02f, .2f, 1};
    for (unsigned i = 0; i < 4; ++i)
        c[4 + i][i] = 1;
    c[40] = {.7f, -.4f, .2f, .25f};
    c[41] = {-.1f, .5f, .3f, -.15f};
    c[42] = {.2f, .1f, .8f, .1f};
    // Distinct unused projections expose an accidental TEXCOORD4/6 mapping.
    for (unsigned i = 37; i < 49; ++i)
        if (i < 40 || i > 42)
            c[i] = {3, 4, 5, 6};
    c[12] = {0, 1, 1, 0};
    const Vec point{.1f, -.2f, .3f, 1};
    Vec view = Rotate(point, c);
    for (unsigned i = 0; i < 3; ++i)
        view[i] += c[3][i];
    view[3] = 1;
    Vec projected{};
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned j = 0; j < 4; ++j)
            projected[row] += c[40 + row][j] * view[j];
    const unsigned x = unsigned((view[0] + 1) * 8), y = unsigned((1 - view[1]) * 8);
    unsigned draws = 0;
    for (unsigned vp = 1; vp < 128; vp += 2) {
        const auto format = (vp & 4) ? GxVBF_PNC : GxVBF_PN;
        auto* vertex = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
        auto* bytes = device.BufLock(vertex);
        std::memset(bytes, 0, GxVertexSize(format));
        std::memcpy(bytes, point.data(), 12);
        device.BufUnlock(vertex, 0);
        GxPrimVertexPtr(vertex, format);
        for (float scale : {-1.2f, 0.0f, .3f, 3.0f})
            for (unsigned floating = 0; floating < 2; ++floating) {
                const float squared = Dot(projected, projected) * scale * scale, distance = std::sqrt(squared);
                device.BeginFrame(pass);
                device.DeviceSetRenderTarget(GxBuffers_Color, floating ? cleanup.target : nullptr, 0);
                device.SceneClear(1, CImVector(0, 0, 0, 0));
                GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 49);
                const Vec depth{100, 200, 300, scale};
                GxShaderConstantsSet(GxSh_Pixel, 1, depth.data(), 1);
                device.RsSet(GxRs_VertexShader, cleanup.vertex[vp]);
                device.RsSet(GxRs_PixelShader, cleanup.pixel);
                CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
                device.Draw(&batch, 0);
                ++draws;
                CiRect rect(0, 0, 16, 16);
                if (floating) {
                    TSGrowableArray<float> actual;
                    device.ReadFloatColor(rect, actual);
                    if (!std::isfinite(actual[y * 16 + x]) || std::abs(actual[y * 16 + x] - distance) > 2e-6f)
                        throw std::runtime_error("TerrainSM float distance mismatch");
                } else {
                    TSGrowableArray<CImVector> actual;
                    device.DeviceReadPixels(rect, actual);
                    const auto p = actual[y * 16 + x];
                    if (std::abs(int(p.r) - int(std::lround(Sat(distance) * 255))) > 1 ||
                        std::abs(int(p.g) - int(std::lround(Sat(squared) * 255))) > 1 || p.b != 255 || p.a != 255)
                        throw std::runtime_error("TerrainSM RG distance/square or constant channels mismatch");
                }
                device.ScenePresent();
            }
    }
    // A varying quad distinguishes per-fragment radial distance from a length
    // computed at vertices and interpolated. Use the actual affine TEXCOORD5.
    c = {};
    for (unsigned i = 0; i < 4; ++i)
        c[i][i] = c[4 + i][i] = 1;
    for (unsigned i = 0; i < 3; ++i)
        c[40 + i][i] = 1;
    c[12] = {0, 1, 1, 0};
    struct Vertex {
        float x, y, z, nx, ny, nz;
    };
    const Vertex quad[] = {{-1, 1, .5f, 0, 0, 0}, {-1, -1, .5f, 0, 0, 0}, {1, 1, .5f, 0, 0, 0}, {1, -1, .5f, 0, 0, 0}};
    auto* buffer = GxBufStream(GxPoolTarget_Vertex, sizeof(Vertex), 4);
    std::memcpy(device.BufLock(buffer), quad, sizeof(quad));
    device.BufUnlock(buffer, 0);
    GxPrimVertexPtr(buffer, GxVBF_PN);
    device.BeginFrame(pass);
    device.DeviceSetRenderTarget(GxBuffers_Color, cleanup.target, 0);
    GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 49);
    const Vec depth{0, 0, 0, .5f};
    GxShaderConstantsSet(GxSh_Pixel, 1, depth.data(), 1);
    device.RsSet(GxRs_VertexShader, cleanup.vertex[1]);
    CGxBatch batch{GxPrim_TriangleStrip, 0, 4, 0, 3};
    device.Draw(&batch, 0);
    CiRect rect(0, 0, 16, 16);
    TSGrowableArray<float> pixels;
    device.ReadFloatColor(rect, pixels);
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x) {
            const float px = (x + .5f) / 8 - 1, py = 1 - (y + .5f) / 8;
            if (std::abs(pixels[y * 16 + x] - .5f * std::sqrt(px * px + py * py + .25f)) > 2e-6f)
                throw std::runtime_error("TerrainSM fragment interpolation mismatch");
        }
    device.ScenePresent();
    device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
    std::fprintf(stderr,
                 "TerrainSM: %u point references across 64 vertex variants, signed/zero scale, R32F and RGBA channels; interpolated radial "
                 "distance verified without texture inputs\n",
                 draws);
}
