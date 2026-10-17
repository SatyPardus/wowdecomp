#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/ShaderEffect/ShaderEffectParser.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
using Vec = std::array<float, 4>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Pattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    static const uint32_t texels[] = {0x00104488, 0x0060A0E0, 0x00C08040, 0x00FFFFFF};
    static const uint32_t modTexels[] = {0xFF104488, 0x8060A0E0, 0x40C08040, 0x00FFFFFF};
    if (command == GxTex_Latch) { stride = 8; data = context ? modTexels : texels; }
}
void EnvironmentPattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texels[] = {0x00102030, 0xFF183028, 0x00183010, 0xFF301820};
    if (command == GxTex_Latch) { stride = 8; data = texels; }
}
void LayerAlphaPattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texel = 0x804080C0;
    if (command == GxTex_Latch) { stride = 4; data = &texel; }
}
Vec LayerReference(float u, float v) {
    u = std::clamp(u * 2 - 0.5f, 0.0f, 1.0f); v = std::clamp(v * 2 - 0.5f, 0.0f, 1.0f);
    const Vec texels[] = {{16,32,48,0}, {24,48,40,255}, {24,48,16,0}, {48,24,32,255}};
    Vec result{};
    for (unsigned i = 0; i < 4; ++i)
        result[i] = ((texels[0][i]*(1-u) + texels[1][i]*u)*(1-v) + (texels[2][i]*(1-u) + texels[3][i]*u)*v) / 255;
    return result;
}
Vec EnvironmentReference(const Vec& view, const Vec& normal) {
    const float viewLength = std::sqrt(view[0]*view[0] + view[1]*view[1] + view[2]*view[2]);
    Vec incident{}, reflected{};
    float projection = 0;
    for (unsigned i = 0; i < 3; ++i) { incident[i] = -view[i] / viewLength; projection += incident[i] * normal[i]; }
    for (unsigned i = 0; i < 3; ++i) reflected[i] = incident[i] - 2 * projection * normal[i];
    const float sphereLength = std::sqrt(reflected[0]*reflected[0] + reflected[1]*reflected[1] + (reflected[2]+1)*(reflected[2]+1));
    return LayerReference(reflected[0] / sphereLength * 0.5f + 0.5f, reflected[1] / sphereLength * 0.5f + 0.5f);
}
float Dot(const Vec& a, const Vec& b, unsigned count = 4) {
    float result = 0; for (unsigned i = 0; i < count; ++i) result += a[i] * b[i]; return result;
}
float Sat(float value) { return std::clamp(value, 0.0f, 1.0f); }
}

void CheckModelShaders(CGxDeviceMetal& device) {
    // Truncated scopes/arguments and over-capacity pass lists used to read or
    // write beyond the parser's stack records before any shader was created.
    for (std::string malformed : {std::string("Effect(Broken"), std::string("Effect(Test){Shader(){Pass(){VertexShader(" + std::string(80, 'x') + ");}}}"),
        std::string("Effect(Test){Shader(){Pass(){VertexShader(Color_T1);PixelShader(Combiners_Mod);}Pass(){}}}"),
        std::string("Effect(Test){FixedFunc(){Pass(3){ColorOp0(Mod);}}}")}) {
        bool rejected = false;
        try { CShaderEffectParser::ParseEffectFile(malformed.data(), uint32_t(malformed.size()), [](const auto&, void*) {}, nullptr); }
        catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Malformed shader effect escaped its bounds checks: " + malformed);
    }
    CGxShader* vertex[5][90]{}; CGxShader* pixel[16]{}; CGxShader* modPixel[16]{}; CGxShader* layerPixel[5][16]{};
    for (unsigned family = 0; family < 5; ++family)
        device.ShaderCreate(vertex[family], GxSh_Vertex, "Shaders\\Vertex", GxMetal::ModelVertexFamily(GxMetal::ModelVertexEffect(family)), 90);
    device.ShaderCreate(pixel, GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque", 16);
    device.ShaderCreate(modPixel, GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod", 16);
    device.ShaderCreate(layerPixel[0], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_AddNA", 16);
    device.ShaderCreate(layerPixel[1], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_AddNA", 16);
    device.ShaderCreate(layerPixel[2], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_Mod", 16);
    device.ShaderCreate(layerPixel[3], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Mod2xNA_Alpha", 16);
    device.ShaderCreate(layerPixel[4], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Mod2x", 16);
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader* (*vertex)[90]; CGxShader** pixel; CGxShader** modPixel; CGxShader* (*layerPixel)[16]; CGxTex* texture[4]{};
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop(); for (auto& image : texture) device.TexDestroy(image);
            for (unsigned family = 0; family < 5; ++family) for (unsigned i = 0; i < 90; ++i) device.ShaderDestroy(vertex[family][i]);
            for (unsigned i = 0; i < 16; ++i) { device.ShaderDestroy(pixel[i]); device.ShaderDestroy(modPixel[i]); device.ShaderDestroy(layerPixel[0][i]); device.ShaderDestroy(layerPixel[1][i]); device.ShaderDestroy(layerPixel[2][i]); device.ShaderDestroy(layerPixel[3][i]); device.ShaderDestroy(layerPixel[4][i]); }
        }
    } cleanup{device, vertex, pixel, modPixel, layerPixel};
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, Pattern, "Model material reference", cleanup.texture[0]);
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), &cleanup, Pattern, "Modulated model material reference", cleanup.texture[1]);
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Linear), nullptr, EnvironmentPattern, "Environment material reference", cleanup.texture[2]);
    device.TexCreate(GxTex_2d, 1, 1, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, LayerAlphaPattern, "Layer alpha boundary", cleanup.texture[3]);
    device.RsSet(GxRs_Texture1, cleanup.texture[2]);
    device.RsSet(GxRs_Texture0, cleanup.texture[0]);
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
    c[8] = {0.8f, -0.1f, 0, 0.3f}; c[9] = {0.4f, 1.2f, 0, -0.2f};
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
    source.m_texcoord[0] = C2Vector(0.25f, 0.7f); source.m_texcoord[1] = C2Vector(0.2f, 0.6f);
    const uint8_t weights[] = {32, 64, 96, 63}, indices[] = {4, 2, 1, 3};
    std::memcpy(&source.m_weights, weights, 4); std::memcpy(&source.m_indices, indices, 4);
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    const Vec fogColor{0.12f, 0.23f, 0.34f, 0.6f};
    // Independently evaluate the original SM3 vertex equations at one vertex.
    // A point checks position, UV matrix, palette indexing, light accumulation,
    // fog and opaque alpha without introducing triangle-interpolation error.
    for (unsigned family = 0; family < 5; ++family) for (unsigned permutation = 0; permutation < ((family == 0 || family >= 3) ? 30u : 10u); ++permutation) {
        const bool materialColor = family == 0 || family >= 3;
        source.m_normal.x = family == 3 && (permutation & 2) ? -0.3f : 0.3f;
        source.m_normal.y = family == 3 && (permutation & 4) ? -0.4f : 0.4f;
        const auto format = materialColor ? GxVBF_PBNT2 : family == 2 && (permutation & 1) ? GxVBF_PNCT :
            family == 1 && (permutation & 1) ? GxVBF_PT : GxVBF_PCT;
        auto* vertices = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
        auto* bytes = reinterpret_cast<uint8_t*>(device.BufLock(vertices));
        if (materialColor) std::memcpy(bytes, &source, sizeof(source));
        else {
            std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_Position), &source.m_position, 12);
            std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_TexCoord0), &source.m_texcoord[0], 8);
            if (GxVertexHasAttrib(format, GxVA_Normal)) std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_Normal), &source.m_normal, 12);
            const uint32_t packedColor = 0xB380A0C0;
            if (GxVertexHasAttrib(format, GxVA_Color0)) std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_Color0), &packedColor, 4);
        }
        device.BufUnlock(vertices, 0); GxPrimVertexPtr(vertices, format);
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
        const Vec vertexColor{128.0f / 255, 160.0f / 255, 192.0f / 255, 179.0f / 255};
        Vec color = materialColor ? c[28] : vertexColor;
        if (family == 1 && (permutation & 1)) color = {1, 1, 1, 1};
        for (unsigned channel = 0; channel < 3; ++channel) {
            float lighting = Sat(transformedNormal[2]) * c[10][channel] + c[11][channel];
            for (unsigned light = 0; light < (permutation / 2) % 5; ++light) {
                Vec delta{}; for (unsigned axis = 0; axis < 3; ++axis) delta[axis] = c[21 + light][axis] - view[axis];
                const float distance = std::sqrt(Dot(delta, delta, 3));
                lighting += c[17 + light][channel] * std::max(0.0f, Dot(delta, transformedNormal, 3) / distance) /
                    (c[25][light] + c[26][light] * distance + c[27][light] * distance * distance);
            }
            if (materialColor) color[channel] = Sat(c[28][channel] * ((permutation & 1) ? Sat(lighting) : 1) + c[29][channel]);
            else if (family == 2 && (permutation & 1)) color[channel] = Sat(vertexColor[channel] * Sat(lighting) + c[29][channel]);
        }
        const float fog = std::min(1.0f, std::pow(std::max(0.0f, view[2] * c[30][0] + c[30][1]), c[30][2]));
        const Vec texel{96.0f / 255, 160.0f / 255, 224.0f / 255, 0}; // UV matrix selects (1,0).
        const Vec secondUV{source.m_texcoord[1].x, source.m_texcoord[1].y, 0, 1};
        const Vec environment = family == 4 ? LayerReference(Dot(c[8], secondUV), Dot(c[9], secondUV)) : EnvironmentReference(view, transformedNormal);
        const float w = Dot(c[5], view);
        const unsigned x = std::floor((Dot(c[2], view) / w + 1) * 16), y = std::floor((1 - Dot(c[3], view) / w) * 16);
        for (unsigned material = 0; material < (family >= 3 ? 7u : 2u); ++material) for (unsigned pixelPermutation : {0u, 4u, 8u, 12u}) {
            const bool mod = material == 1 || material == 3 || material == 4, layered = material >= 2;
            device.RsSet(GxRs_Texture0, cleanup.texture[mod || material == 5]);
            Vec pixelFog = fogColor; if (material == 6) pixelFog[3] = 0; if (mod) pixelFog[3] = material == 4 ? 0.0f : 0.3f;
            Vec expected = color; if (mod) expected[3] *= 128.0f / 255;
            if (material == 4) expected[3] *= environment[3];
            if (material == 6) expected[3] = Sat(2 * color[3] * environment[3]);
            for (unsigned channel = 0; channel < 3; ++channel) {
                float shaded = texel[channel] * color[channel];
                if (material == 2 || material == 3) shaded += environment[channel];
                if (material == 4) shaded *= environment[channel];
                if (material == 6) shaded *= 2 * environment[channel];
                if (material == 5) shaded *= 128.0f / 255 + 2 * environment[channel] * (1 - 128.0f / 255);
                expected[channel] = Sat(fog * (shaded - fogColor[channel]) + fogColor[channel]);
            }
            device.BeginFrame(pass);
            GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 256);
            GxShaderConstantsSet(GxSh_Pixel, 2, pixelFog.data(), 1);
            device.RsSet(GxRs_VertexShader, vertex[family][permutation]); device.RsSet(GxRs_PixelShader, (layered ? layerPixel[material - 2] : mod ? modPixel : pixel)[pixelPermutation]);
            GxDraw(&batch, 0);
            CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            const auto& sample = pixels[y * 32 + x]; const uint8_t channels[] = {sample.r, sample.g, sample.b, sample.a};
            for (unsigned channel = 0; channel < 4; ++channel)
                if (std::abs(int(channels[channel]) - int(std::lround(expected[channel] * 255))) > 2)
                    throw std::runtime_error(std::to_string(material) + std::string(" model material mismatch: family ") + std::to_string(family) + " VS " + std::to_string(permutation) + " PS " + std::to_string(pixelPermutation) +
                        " channel " + std::to_string(channel) + " at " + std::to_string(x) + "," + std::to_string(y) +
                        " actual " + std::to_string(channels[channel]) + " expected " + std::to_string(expected[channel] * 255));
            unsigned covered = 0; for (uint32_t i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            Require(covered == unsigned(expected[3] >= 0.5f / 255), "Model point position or raster coverage changed");
            device.ScenePresent();
        }
    }
    struct AlphaCase { unsigned pixel, stateRef; float shaderRef; bool visible; };
    for (unsigned material = 0; material < 7; ++material) {
        const bool mod = material == 1 || material == 3 || material == 4, layered = material >= 2;
        device.RsSet(GxRs_Texture0, cleanup.texture[mod || material == 5]);
        device.RsSet(GxRs_Texture1, cleanup.texture[material == 4 || material == 6 ? 3 : 2]);
        const unsigned reference = mod ? 64 : 128;
        const float alpha = reference / 255.0f;
        for (const auto& test : {AlphaCase{0, reference, 0.9f, true}, AlphaCase{0, reference + 1, 0, false},
            AlphaCase{8, 0, alpha, true}, AlphaCase{8, 0, std::nextafter(alpha, 1.0f), false},
            AlphaCase{8, reference, alpha, true}, AlphaCase{8, reference + 1, alpha, false}}) {
            device.BeginFrame(pass);
            c[28][3] = material == 4 ? 255.0f / 256 : mod || material == 6 ? 0.5f : alpha; Vec fog = fogColor; fog[3] = test.shaderRef;
            GxShaderConstantsSet(GxSh_Vertex, 28, c[28].data(), 1); GxShaderConstantsSet(GxSh_Pixel, 2, fog.data(), 1);
            device.RsSet(GxRs_VertexShader, vertex[layered ? 3 : 0][0]); device.RsSet(GxRs_PixelShader, (layered ? layerPixel[material - 2] : mod ? modPixel : pixel)[test.pixel]);
            GxRsSet(GxRs_AlphaRef, int32_t(test.stateRef)); GxDraw(&batch, 0);
            CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            unsigned covered = 0; for (uint32_t i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            Require(covered == unsigned(test.visible), "Model shader/state alpha threshold or equality mismatch");
            device.ScenePresent();
        }
    }
    device.BeginFrame(pass);
    device.RsSet(GxRs_VertexShader, vertex[0][30]); device.RsSet(GxRs_PixelShader, pixel[1]);
    bool rejected = false; try { GxDraw(&batch, 0); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected, "Unimplemented model shadow permutation was silently replaced");
    device.ScenePresent();
}
