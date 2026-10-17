#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/FFX/PassBox4.h"
#include "Engine/Source/FFX/PassGauss4.h"
#include "Engine/Source/FFX/PassGlow.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Base/Handle.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
using Pixel = std::array<uint8_t, 4>;
struct Image { unsigned width, height; std::vector<Pixel> pixels; };
void Upload(EGxTexCommand command, uint32_t width, uint32_t, uint32_t, uint32_t, void* arg, uint32_t& stride, const void*& data) {
    if (command == GxTex_Latch) { stride = width * 4; data = static_cast<Image*>(arg)->pixels.data(); }
}
FFX::Tex Texture(unsigned width, unsigned height, unsigned logicalWidth, unsigned logicalHeight, Image* image = nullptr) {
    CGxTexFlags flags(GxTex_Linear); flags.m_renderTarget = !image;
    auto* texture = TextureCreate(GxTex_2d, width, height, 0, GxTex_Argb8888, GxTex_Argb8888, flags,
        image, image ? Upload : GxTexNullUserFunc, "FFX pass probe", 0);
    Require(texture, "Cannot create original Services texture for FFX");
    return {texture, {int(width), int(height)}, {int(logicalWidth), int(logicalHeight)}, {1.0f / width, 1.0f / height}};
}
std::array<float, 4> Sample(const Image& image, float x, float y) {
    const int left = int(std::floor(x)), top = int(std::floor(y));
    const float fx = x - left, fy = y - top;
    std::array<float, 4> result{};
    for (int dy = 0; dy < 2; ++dy) { for (int dx = 0; dx < 2; ++dx) {
        const int ix = std::clamp(left + dx, 0, int(image.width) - 1), iy = std::clamp(top + dy, 0, int(image.height) - 1);
        const float weight = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
        for (unsigned c = 0; c < 4; ++c) { result[c] += image.pixels[iy * image.width + ix][c] * weight; }
    } }
    return result;
}
Image Reference(const Image& source, unsigned width, unsigned height, float sourceWidth, float sourceHeight,
    const std::array<C2Vector, 4>& offsets, bool box) {
    Image result{width, height, std::vector<Pixel>(width * height)};
    const float weights[] = {0.125f, 0.375f, 0.375f, 0.125f};
    for (unsigned y = 0; y < height; ++y) { for (unsigned x = 0; x < width; ++x) {
        std::array<float, 4> channels{};
        for (unsigned tap = 0; tap < 4; ++tap) {
            // Original D3D9 integer screen centers + original half-texel UV.
            auto sample = Sample(source, x * sourceWidth / width + offsets[tap].x, y * sourceHeight / height + offsets[tap].y);
            for (unsigned c = 0; c < 4; ++c) { channels[c] += sample[c] * (box ? 0.25f : weights[tap]); }
        }
        for (unsigned c = 0; c < 4; ++c) { result.pixels[y * width + x][c] = std::lround(channels[c]); }
        if (box) { result.pixels[y * width + x][3] = 255; }
    } }
    return result;
}
Image GlowReference(const Image& scene, const Image& blur, CImVector color) {
    Image result = scene;
    for (unsigned y = 0; y < scene.height; ++y) { for (unsigned x = 0; x < scene.width; ++x) {
        auto filtered = Sample(blur, float(x * blur.width) / scene.width, float(y * blur.height) / scene.height);
        for (unsigned c = 0; c < 3; ++c) {
            const float base = scene.pixels[y * scene.width + x][c] * (1 - color.b / 255.0f) + filtered[c] * color.b / 255.0f;
            const float glow = filtered[c] * filtered[c] * color.a / (255.0f * 255.0f);
            result.pixels[y * scene.width + x][c] = std::clamp(int(std::lround(base + glow)), 0, 255);
        }
        result.pixels[y * scene.width + x][3] = 255;
    } }
    return result;
}
Image Read(CGxDeviceMetal& device, FFX::Tex& texture) {
    GxRenderTargetSet(GxBuffers_Color, texture.m_texture ? TextureGetGxTex(texture.m_texture, 1, nullptr) : nullptr, 0);
    CiRect rect(0, 0, texture.m_texSize.y, texture.m_texSize.x); TSGrowableArray<CImVector> pixels;
    GxReadPixels(rect, pixels);
    Image image{unsigned(texture.m_texSize.x), unsigned(texture.m_texSize.y), std::vector<Pixel>(pixels.Count())};
    std::memcpy(image.pixels.data(), pixels.Ptr(), image.pixels.size() * 4);
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
    return image;
}
void Compare(const Image& actual, const Image& expected, unsigned tolerance) {
    for (unsigned y = 0; y < expected.height; ++y) { for (unsigned x = 0; x < expected.width; ++x) {
        for (unsigned c = 0; c < 4; ++c) {
            Require(unsigned(std::abs(int(actual.pixels[y * actual.width + x][c]) - expected.pixels[y * expected.width + x][c])) <= tolerance,
                "Original FFX pass output differs from D3D9 sampling equations");
        }
    } }
}
}

void CheckGxFFXPasses(CGxDeviceMetal& device) {
    struct Scope {
        std::vector<CTexture*> textures;
        ~Scope() { for (auto* texture : textures) { HandleClose(reinterpret_cast<HOBJECT>(texture)); } FFX::s_quart[1] = {}; TextureGxCacheFlush(); }
    } scope;
    // Only bootstrap the quad data normally initialized by FFX::Init. Console,
    // effect selection and the full game loop remain outside this probe.
    const uint16_t indices[] = {0, 3, 1, 2};
    std::copy(std::begin(indices), std::end(indices), FFX::s_quadIndices); FFX::s_useRECT = false;
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto frame = [MTLRenderPassDescriptor renderPassDescriptor];
    frame.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    frame.colorAttachments[0].loadAction = MTLLoadActionClear;
    frame.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    GxRsSet(GxRs_VertexShader, static_cast<void*>(nullptr)); GxRsSet(GxRs_PixelShader, static_cast<void*>(nullptr));
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque)); GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_ScissorTest, 0);
    C44Matrix projection; GxuXformCreateOrtho(-3, 5, -4, 6, -2, 3, projection); GxXformSetProjection(projection);
    C44Matrix view(1); view.d0 = 7; GxXformSetView(view);
    C44Matrix world(1); world.d1 = 11; GxXformSet(GxXform_World, world);
    auto checkRestore = [&] {
        C44Matrix restored; GxXformProjection(restored); Require(!std::memcmp(&restored, &projection, sizeof(restored)), "FFX lost caller projection");
        GxXformView(restored); Require(!std::memcmp(&restored, &view, sizeof(restored)), "FFX lost caller view");
        const auto& stack = device.m_xforms[GxXform_World];
        Require(!std::memcmp(&stack.m_mtx[stack.m_level], &world, sizeof(world)), "FFX lost caller world transform");
        Require(!device.m_appRenderStates[GxRs_PixelShader].m_value.m_data.p &&
            device.m_textureTarget[GxBuffers_Color].m_texture == nullptr, "FFX lost caller shader/default target");
    };
    auto pattern = [](unsigned width, unsigned height) {
        Image image{width, height, std::vector<Pixel>(width * height)};
        for (unsigned y = 0; y < height; ++y) { for (unsigned x = 0; x < width; ++x) {
            image.pixels[y * width + x] = {uint8_t((x * 83 + y * 19) % 256), uint8_t((x * 31 + y * 71) % 256),
                uint8_t((x * 13 + y * 43) % 256), uint8_t((x * 67 + y * 29) % 256)};
        } }
        return image;
    };
    Image source = pattern(16, 8);
    auto input = Texture(16, 8, 13, 7, &source), output = Texture(8, 8, 7, 3);
    scope.textures = {input.m_texture, output.m_texture};
    FFX::Tex* inputs[] = {&input};
    {
        PassBox4 box(inputs, 1, &output, GxBlend_Opaque, false);
        PassBox4 cached(inputs, 1, &output, GxBlend_Opaque, false);
        Require(box.Valid() && cached.Valid() && box.m_shaders[0] == cached.m_shaders[0] && box.m_shaders[1] == cached.m_shaders[1],
            "FFX native shader factory or permutation cache failed");
        device.BeginFrame(frame);
        GxRenderTargetSet(GxBuffers_Color, TextureGetGxTex(output.m_texture, 1, nullptr), 0);
        GxXformSetViewport(0, 1, 0, 1, 0, 1); device.SceneClear(1, CImVector(17, 34, 51, 68));
        box.Execute(); checkRestore();
        auto actual = Read(device, output);
        Compare(actual, Reference(source, 7, 3, 13, 7, {{{-1.5f,-1.5f},{0.5f,-1.5f},{0.5f,0.5f},{-1.5f,0.5f}}}, true), 1);
        for (unsigned y = 0; y < 8; ++y) { for (unsigned x = 0; x < 8; ++x) {
            if (y >= 3 || x >= 7) { Require(actual.pixels[y * 8 + x] == Pixel{17, 34, 51, 68}, "FFX downsample overwrote padded target pixels"); }
        } }
        GxScenePresent();
    }
    source = pattern(8, 8);
    input = Texture(8, 8, 8, 8, &source); output = Texture(8, 8, 8, 8); FFX::s_quart[1] = Texture(8, 8, 8, 8);
    scope.textures.insert(scope.textures.end(), {input.m_texture, output.m_texture, FFX::s_quart[1].m_texture});
    {
        PassGauss4 gauss(inputs, 1, &output, GxBlend_Opaque, false); Require(gauss.Valid(), "FFX Gauss shader is invalid");
        device.BeginFrame(frame); gauss.Execute(); checkRestore();
        auto horizontal = Reference(source, 8, 8, 8, 8, {{{-2.5f,0},{-0.5f,0},{0.5f,0},{2.5f,0}}}, false);
        Compare(Read(device, FFX::s_quart[1]), horizontal, 1);
        Compare(Read(device, output), Reference(horizontal, 8, 8, 8, 8, {{{0,-2.5f},{0,-0.5f},{0,0.5f},{0,2.5f}}}, false), 2);
        GxScenePresent();
    }
    // Run the original scene -> Box4 -> Gauss4 -> Glow sequence with no
    // intermediate CPU readback, including the original in-place blur output.
    source = pattern(32, 32); input = Texture(32, 32, 32, 32, &source);
    scope.textures.push_back(input.m_texture);
    descriptor.width = descriptor.height = 32;
    frame.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    FFX::Tex screen{nullptr, {32, 32}, {32, 32}, {1.0f / 32, 1.0f / 32}};
    FFX::Tex* blurredInput[] = {&output}; FFX::Tex* glowInputs[] = {&input, &output};
    PassBox4 box(inputs, 1, &output, GxBlend_Opaque, false);
    PassGauss4 gauss(blurredInput, 1, &output, GxBlend_Opaque, false);
    PassGlow glow(glowInputs, 2, &screen, GxBlend_Opaque, true);
    Require(glow.Valid(), "Original FFX Glow shader is invalid");
    Image expected = Reference(source, 8, 8, 32, 32, {{{-1.5f,-1.5f},{0.5f,-1.5f},{0.5f,0.5f},{-1.5f,0.5f}}}, true);
    expected = Reference(expected, 8, 8, 8, 8, {{{-2.5f,0},{-0.5f,0},{0.5f,0},{2.5f,0}}}, false);
    expected = Reference(expected, 8, 8, 8, 8, {{{0,-2.5f},{0,-0.5f},{0,0.5f},{0,2.5f}}}, false);
    bool first = true;
    for (const CImVector color : {CImVector(85, 11, 219, 173), CImVector(0, 37, 93, 0),
        CImVector(0, 197, 3, 255), CImVector(255, 71, 17, 0), CImVector(255, 13, 201, 255)}) {
        device.BeginFrame(frame);
        if (first) { box.Execute(); gauss.Execute(); }
        glow.m_color = color; GxRsSet(GxRs_AlphaRef, 255); glow.Execute(); checkRestore();
        Compare(Read(device, screen), GlowReference(source, expected, color), first ? 3 : 1);
        if (first) {
            auto actual = Read(device, output); Compare(actual, expected, 2);
            expected = std::move(actual); first = false;
        }
        GxScenePresent();
    }
    GxRsSet(GxRs_AlphaRef, 0);

}
