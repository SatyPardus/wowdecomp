#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/FFX/PassBox4.h"
#include "Engine/Source/FFX/PassGauss4.h"
#include "Engine/Source/FFX/PassGlow.h"
#include "Engine/Source/FFX/PassGlowWave.h"
#include "Engine/Source/FFX/EffectGlow.h"
#include "WoW/Source/Game/GameClient/FFXEffects.h"
#include "WoW/Source/Game/GameClient/PassDeath.h"
#include "WoW/Source/Game/GameClient/PassNetherBlur.h"
#include "WoW/Source/Game/GameClient/PassPropagateFog.h"
#include "WoW/Source/Game/GameClient/PassFogCombine.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Base/Handle.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <memory>
#include <unistd.h>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
using Pixel = std::array<uint8_t, 4>;
struct Image { unsigned width, height; std::vector<Pixel> pixels; };
void Upload(EGxTexCommand command, uint32_t width, uint32_t, uint32_t, uint32_t, void* arg, uint32_t& stride, const void*& data) {
    if (command == GxTex_Latch) { stride = width * 4; data = static_cast<Image*>(arg)->pixels.data(); }
}
FFX::Tex Texture(unsigned width, unsigned height, unsigned logicalWidth, unsigned logicalHeight, Image* image = nullptr) {
    CGxTexFlags flags(GxTex_Linear); flags.m_renderTarget = !image;
    const auto target = (width & (width - 1)) || (height & (height - 1)) ? GxTex_NonPow2 : GxTex_2d;
    auto* texture = TextureCreate(target, width, height, 0, GxTex_Argb8888, GxTex_Argb8888, flags,
        image, image ? Upload : GxTexNullUserFunc, "FFX pass probe", 0);
    Require(texture && TextureGetGxTex(texture, 1, nullptr), "Cannot create original Services texture for FFX");
    return {texture, {int(width), int(height)}, {int(logicalWidth), int(logicalHeight)}, {1.0f / width, 1.0f / height}};
}
std::array<float, 4> Sample(const Image& image, float x, float y, bool wrap = false) {
    const int left = int(std::floor(x)), top = int(std::floor(y));
    const float fx = x - left, fy = y - top;
    std::array<float, 4> result{};
    for (int dy = 0; dy < 2; ++dy) { for (int dx = 0; dx < 2; ++dx) {
        auto address = [&](int i, int size) { return wrap ? (i % size + size) % size : std::clamp(i, 0, size - 1); };
        const int ix = address(left + dx, image.width), iy = address(top + dy, image.height);
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
Image UpscaleScene(const Image& source, unsigned width, unsigned height) {
    Image result{width, height, std::vector<Pixel>(width * height)};
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const auto sample = Sample(source, (x + .5f) * source.width / width - .5f, (y + .5f) * source.height / height - .5f);
        for (unsigned c = 0; c < 4; ++c) result.pixels[y * width + x][c] = std::lround(sample[c]);
    }
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
Image WaveData() {
    auto* texture = TextureGetGxTex(PassGlowWave::s_waveTex.m_texture, 1, nullptr);
    Image image{128, 128, std::vector<Pixel>(128 * 128)};
    uint32_t rowBytes = 0; const void* pixels = nullptr;
    texture->m_userFunc(GxTex_Lock, 128, 128, 0, 0, texture->m_userArg, rowBytes, pixels);
    texture->m_userFunc(GxTex_Latch, 128, 128, 0, 0, texture->m_userArg, rowBytes, pixels);
    Require(rowBytes == 128 * 4 && pixels, "Original wave texture producer returned invalid pixels");
    std::memcpy(image.pixels.data(), pixels, image.pixels.size() * 4);
    texture->m_userFunc(GxTex_Unlock, 128, 128, 0, 0, texture->m_userArg, rowBytes, pixels);
    return image;
}
Image DeathReference(const Image& scene, const Image& blur, uint8_t glow) {
    Image result = scene;
    for (unsigned y = 0; y < scene.height; ++y) for (unsigned x = 0; x < scene.width; ++x) {
        const auto filtered = Sample(blur, float(x * blur.width) / scene.width, float(y * blur.height) / scene.height);
        const double weights[] = {0.144, 0.587, 0.299};
        double intensity = 0;
        for (unsigned c = 0; c < 3; ++c)
            intensity += weights[c] * (scene.pixels[y * scene.width + x][c] / 255.0 +
                filtered[c] * filtered[c] * glow / (255.0 * 255.0 * 255.0));
        intensity = std::clamp(intensity, 0.0, 1.0);
        const double tint = std::clamp(intensity * (1 - intensity) * 4, 0.0, 1.0);
        const uint8_t color[] = {0xA8, 0x93, 0x53};
        for (unsigned c = 0; c < 3; ++c)
            result.pixels[y * scene.width + x][c] = std::clamp(std::lround(255 * intensity + color[c] * tint), 0l, 255l);
        result.pixels[y * scene.width + x][3] = 255;
    }
    return result;
}
struct DayNightGlowScope {
    float previous = DayNight::GetInfo()->m_bands.m_glow;
    ~DayNightGlowScope() { DayNight::GetInfo()->m_bands.m_glow = previous; }
};
Image WaveReference(const Image& scene, const Image& blur, const Image& wave, const FFX::VertexPCT3* vertices, CImVector color, bool directDisplacement) {
    Image result = scene;
    for (unsigned y = 0; y < scene.height; ++y) { for (unsigned x = 0; x < scene.width; ++x) {
        C2Vector uv[3];
        for (unsigned i = 0; i < 3; ++i) {
            const auto& topLeft = vertices[1].tc[i]; const auto& topRight = vertices[2].tc[i]; const auto& bottomLeft = vertices[0].tc[i];
            const float fx = float(x) / scene.width, fy = float(y) / scene.height;
            uv[i].x = topLeft.x + (topRight.x - topLeft.x) * fx + (bottomLeft.x - topLeft.x) * fy;
            uv[i].y = topLeft.y + (topRight.y - topLeft.y) * fx + (bottomLeft.y - topLeft.y) * fy;
        }
        const auto sample = Sample(wave, uv[0].x * wave.width - 0.5f, uv[0].y * wave.height - 0.5f, true);
        // Original 0x008C2CE0 creates BGRA8 for SM2/3, but selects the shader
        // from the UV88 capability. Preserve that original combination: BLS
        // 0/1 consumes samples directly, while 2/3 remaps UNORM into [-1,1].
        const float scale = directDisplacement ? 1.0f / 255 : 2.0f / 255;
        const float bias = directDisplacement ? 0 : -1;
        const float dx = sample[2] * scale + bias, dy = sample[1] * scale + bias;
        const auto original = Sample(scene, uv[1].x * scene.width - 0.5f + dx * 3, uv[1].y * scene.height - 0.5f + dy * 3);
        const auto filtered = Sample(blur, uv[2].x * blur.width - 0.5f + dx * 0.75f, uv[2].y * blur.height - 0.5f + dy * 0.75f);
        for (unsigned c = 0; c < 3; ++c) {
            const float base = original[c] * (1 - color.b / 255.0f) + filtered[c] * color.b / 255.0f;
            result.pixels[y * scene.width + x][c] = std::clamp(int(std::lround(base + filtered[c] * filtered[c] * color.a / (255.0f * 255.0f))), 0, 255);
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
Image ScreenCoverage(CGxDeviceMetal& device, Image shaded, const Image* background=nullptr) {
    // Original postprocess quads begin at integer pixel centers. In MSAA,
    // samples to the left/above the first center retain the previous contents.
    const unsigned count=device.m_format.m_multisampleCount;
    if(count<=1 || !device.m_appRenderStates[GxRs_Multisample].m_value.m_data.i[0]) return shaded;
    std::vector<MTLSamplePosition> positions(count);
    [device.GPU().device getDefaultSamplePositions:positions.data() count:count];
    for(unsigned y=0;y<shaded.height;++y) for(unsigned x=0;x<shaded.width;++x) {
        unsigned covered=0;
        for(auto p:positions) covered+=(x || p.x>=.5f) && (y || p.y>=.5f);
        const unsigned index=y*shaded.width+x;
        const Pixel base=background ? background->pixels[index] : Pixel{0,0,0,255};
        for(unsigned c=0;c<4;++c)
            shaded.pixels[index][c]=std::lround((double(shaded.pixels[index][c])*covered+double(base[c])*(count-covered))/count);
    }
    return shaded;
}
void Compare(const Image& actual, const Image& expected, unsigned tolerance) {
    static unsigned comparison = 0; ++comparison;
    for (unsigned y = 0; y < expected.height; ++y) { for (unsigned x = 0; x < expected.width; ++x) {
        for (unsigned c = 0; c < 4; ++c) {
            if (unsigned(std::abs(int(actual.pixels[y * actual.width + x][c]) - expected.pixels[y * expected.width + x][c])) > tolerance)
                std::fprintf(stderr,"FFX comparison %u at %u,%u channel %u: %u expected %u\n",comparison,x,y,c,
                    actual.pixels[y*actual.width+x][c],expected.pixels[y*expected.width+x][c]);
            Require(unsigned(std::abs(int(actual.pixels[y * actual.width + x][c]) - expected.pixels[y * expected.width + x][c])) <= tolerance,
                "Original FFX pass output differs from D3D9 sampling equations");
        }
    } }
}
}

void CheckGxFFXPasses(CGxDeviceMetal& device) {
    DayNightGlowScope dayNight;
    // The pass checks intentionally use translated caller matrices. Restore
    // them at the probe boundary too, before unrelated fixed-function UI draws.
    C44Matrix previousProjection; GxXformProjection(previousProjection);
    device.XformPush(GxXform_World); device.XformPush(GxXform_View);
    struct Matrices {
        CGxDeviceMetal& device; C44Matrix projection;
        ~Matrices() { GxXformPop(GxXform_View); GxXformPop(GxXform_World); device.XformSetProjection(projection); }
    } matrices{device, previousProjection};
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
        Compare(Read(device, screen), ScreenCoverage(device,GlowReference(source, expected, color)), first ? 3 : 1);
        if (first) {
            auto actual = Read(device, output); Compare(actual, expected, 2);
            expected = std::move(actual); first = false;
        }
        GxScenePresent();
    }
    PassDeath death(glowInputs, 2, &screen, GxBlend_Opaque, true);
    PassDeath cachedDeath(glowInputs, 2, &screen, GxBlend_Opaque, true);
    Require(death.Valid() && cachedDeath.Valid() && death.m_shaders[0] == cachedDeath.m_shaders[0] &&
        death.m_shaders[1] == cachedDeath.m_shaders[1], "Death effect shader cache failed");
    for (uint8_t glowAmount : {0, 173, 255}) {
        DayNight::GetInfo()->m_bands.m_glow = glowAmount / 255.0f;
        device.BeginFrame(frame); death.Execute(); checkRestore();
        Compare(Read(device, screen), ScreenCoverage(device,DeathReference(source, expected, glowAmount)), 1);
        GxScenePresent();
    }
    FFX::Tex* waveInputs[] = {&PassGlowWave::s_waveTex, &input, &output};
    PassGlowWave wave(waveInputs, 3, &screen, GxBlend_Opaque, true);
    Require(wave.Valid() && wave.m_shaderIndex == (device.Caps().m_texFmt[GxTex_Uv88] ? 0 : 2), "FFX wave did not select the original capability-dependent permutation");
    const Image waveData = WaveData();
    // Capture the real pass's time-dependent UVs from the existing CPU shadow,
    // then independently evaluate texture filtering, displacement and composition.
    for (CImVector color : {CImVector(0, 23, 101, 0), CImVector(83, 11, 197, 173)}) {
        device.BeginFrame(frame); wave.m_color = color; wave.Execute(); checkRestore();
        const auto* buffer = device.m_primVertexFormatBuf[GxVA_Position];
        Require(buffer && buffer->m_itemSize == sizeof(FFX::VertexPCT3) && buffer->m_itemCount == 4, "FFX wave did not submit PCT3 vertices");
        FFX::VertexPCT3 vertices[4];
        std::memcpy(vertices, static_cast<const uint8_t*>(buffer->m_pool->m_mem) + buffer->m_index, sizeof(vertices));
        const float* constants = device.ShaderConstantsLock(GxSh_Pixel);
        Require(constants[0] == 3.0f / 32 && constants[3] == 3.0f / 32 && constants[4] == 0.75f / 8 && constants[7] == 0.75f / 8,
            "Original FFX wave pixel constants changed");
        Compare(Read(device, screen), ScreenCoverage(device,WaveReference(source, expected, waveData, vertices, color, wave.m_shaderIndex < 2)), 2);
        GxScenePresent();
    }
    GxRsSet(GxRs_AlphaRef, 0);
    {
        const C2Vector taps[] = {{-1,1},{0,1},{1,1},{0,2}};
        PassPropagateFog propagate(inputs, 1, &output, taps, GxBlend_Opaque, false);
        Require(propagate.Valid(), "Original fog propagation shader unavailable");
        for (float decay : {0.0f, 0.2f, 1.0f}) {
            propagate.unk_005C = decay;
            device.BeginFrame(frame); propagate.Execute(); checkRestore();
            Image reference{8, 8, std::vector<Pixel>(64)};
            for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 8; ++x) {
                double channels[4]{};
                for (const auto& tap : taps) {
                    const auto value = Sample(source, x * 4 + tap.x, y * 4 + tap.y);
                    for (unsigned c = 0; c < 4; ++c) channels[c] += value[c] / 4;
                }
                channels[3] -= decay * 255;
                for (unsigned c = 0; c < 4; ++c) reference.pixels[y * 8 + x][c] = std::clamp(std::lround(channels[c]), 0l, 255l);
            }
            Compare(Read(device, output), reference, 1); GxScenePresent();
        }
        // Exercise the original persistent 33x33 polar grid and density fade.
        Image cloud{8,8,std::vector<Pixel>(64, Pixel{27,91,149,85})};
        Image scene{8,8,std::vector<Pixel>(64, Pixel{173,63,102,17})};
        auto cloudTex = Texture(8,8,8,8,&cloud), sceneTex = Texture(8,8,8,8,&scene);
        scope.textures.insert(scope.textures.end(), {cloudTex.m_texture,sceneTex.m_texture});
        FFX::Tex* fogInputs[] = {&cloudTex,&sceneTex};
        PassFogCombine fog(fogInputs,2,&screen,GxBlend_Opaque,true);
        Require(fog.Valid(), "Original polar fog composition unavailable");
        for (float fade : {3.0f, 1.5f, 0.0f}) {
            fog.m_density = 0.73f; fog.m_fadeTime = fade;
            device.BeginFrame(frame); fog.Execute(); checkRestore();
            const double amount = fade > 0 ? 1-fade/3 : 1, density = 0.73*amount, minimum = 0.1*amount;
            const double gray = (173+63+102)*0.33333298563957214;
            Image reference{32,32,std::vector<Pixel>(1024)};
            std::vector<MTLSamplePosition> samples(device.m_format.m_multisampleCount, MTLSamplePositionMake(.5f,.5f));
            if (samples.size()>1 && device.m_appRenderStates[GxRs_Multisample].m_value.m_data.i[0])
                [device.GPU().device getDefaultSamplePositions:samples.data() count:samples.size()];
            for (unsigned y=0;y<32;++y) for (unsigned x=0;x<32;++x) {
                auto& pixel=reference.pixels[y*32+x];
                // The original persistent grid starts on the D3D pixel center.
                // Its half-pixel Metal translation leaves partial sample
                // coverage along the top/left edges, over the black clear.
                unsigned covered=0;
                for (auto p:samples) covered += (x || p.x>=.5f) && (y || p.y>=.5f);
                for (unsigned c=0;c<3;++c) {
                    const double base = (scene.pixels[0][c]*(1-density)+gray*density)*(1-minimum)+255*minimum;
                    pixel[c] = std::lround((base*(1-85/255.0)+cloud.pixels[0][c]*85/255.0)*covered/samples.size());
                }
                pixel[3]=255;
            }
            Compare(Read(device,screen),reference,1); GxScenePresent();
        }
    }
    PassNetherBlur nether(inputs, 1, &output, GxBlend_Opaque, false);
    PassNetherCombine combine(glowInputs, 2, &screen, GxBlend_Opaque, true);
    Require(nether.Valid() && combine.Valid(), "Original nether shaders unavailable");
    const float oldTick = FrameTime::s_tickTimeSec;
    struct TickScope { float value; ~TickScope() { FrameTime::s_tickTimeSec = value; } } tick{oldTick};
    FrameTime::s_tickTimeSec = 0.137f;
    Image previous{};
    for (unsigned step = 0; step < 3; ++step) {
        device.BeginFrame(frame); nether.Execute(); checkRestore();
        const auto intermediate = Read(device, FFX::s_quart[1]);
        const auto blurred = Read(device, output);
        Require(blurred.pixels != intermediate.pixels, "Nether second pass did not filter the intermediate surface");
        if (step) Require(blurred.pixels != previous.pixels, "Original nether phase/noise did not advance");
        previous = blurred;
        combine.Execute(); checkRestore();
        const auto actual = Read(device, screen);
        Image reference = source;
        for (unsigned y = 0; y < 32; ++y) for (unsigned x = 0; x < 32; ++x) {
            const auto blur = Sample(blurred, x / 4.0f, y / 4.0f);
            double mixed[3], gray = 0;
            const auto& original = source.pixels[y * 32 + x];
            for (unsigned c = 0; c < 3; ++c) { mixed[c] = original[c] * 0.5 + blur[c]; gray += mixed[c] * 0.16666649281978607; }
            const double tint[] = {0.78, 0.6, 0.6};
            for (unsigned c = 0; c < 3; ++c)
                reference.pixels[y * 32 + x][c] = std::clamp(std::lround(original[c] * (1 - combine.m_fade) +
                    (mixed[c] * 0.5 + gray) * tint[c] * combine.m_fade), 0l, 255l);
            reference.pixels[y * 32 + x][3] = 255;
        }
        Compare(actual, ScreenCoverage(device,reference), 1); GxScenePresent();
    }
}



void CheckGxFFXEffect(CGxDeviceMetal& device) {
    DayNightGlowScope dayNight;
    std::string pattern = (std::filesystem::temp_directory_path() / "wow-ffx-XXXXXX").string();
    std::vector<char> directory(pattern.begin(), pattern.end()); directory.push_back(0);
    Require(mkdtemp(directory.data()) != nullptr, "Cannot create FFX config directory");
    struct Scope {
        CGxDeviceMetal& device;
        std::string directory;
        std::unique_ptr<EffectGlow> effect;
        std::unique_ptr<EffectDeath> death;
        CTexture* source = nullptr;
        CGxShader* vertex[2]{}, *pixel = nullptr;
        ~Scope() {
            FFX::SetEffect(nullptr); effect.reset(); death.reset(); FFX::Destroy();
            if (source) { HandleClose(reinterpret_cast<HOBJECT>(source)); }
            TextureGxCacheFlush();
            for (auto& shader : vertex) { device.ShaderDestroy(shader); } device.ShaderDestroy(pixel);
            CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager();
            std::error_code error; std::filesystem::remove_all(directory, error);
        }
    } scope{device, directory.data()};
    CVar::Initialize((scope.directory + "/Config.wtf").c_str());
    CVar::Register("gxMultisample", "probe sample count", 1, "1", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
    FFX::Init(); scope.effect = std::make_unique<EffectGlow>(); FFX::SetEffect(scope.effect.get());
    auto* renderScale = CVar::Lookup("gxRenderScale");
    for (const char* invalid : {"0", "0.49", "1.01", "nan", "inf", "0.75junk"}) {
        // Original CVar::Set returns true even when its callback rejects input.
        renderScale->Set(invalid, true, false, false, false);
        Require(renderScale->m_floatValue == 1 && !std::strcmp(renderScale->m_stringValue.m_str, "1"),
            "Scene render scale accepted an invalid persisted/input value");
    }
    scope.death = std::make_unique<EffectDeath>();
    device.ShaderCreate(scope.vertex, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    device.ShaderCreate(&scope.pixel, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
    const auto wave = WaveData();
    // The same effect instance survives enable/mode changes and a host resize.
    struct Case { unsigned size; int mode; bool global, glow; bool death = false; uint8_t glowAmount = 173; unsigned height = 0; float scale = 1; };
    for (const auto test : {Case{32, 0, true, true}, Case{32, 1, true, true}, Case{32, 0, false, true},
        Case{32, 0, true, false}, Case{64, 0, true, true},
        Case{32, 0, true, true, true, 0}, Case{32, 0, true, true, true, 255},
        Case{32, 0, false, true, true}, Case{32, 0, true, false, true},
        Case{64, 0, true, true, true}, Case{67, 0, true, true, false, 173, 35},
        Case{70, 0, true, true, false, 173, 46}, Case{70, 1, true, true, false, 173, 46},
        Case{67, 0, true, true, true, 173, 35}, Case{32, 0, true, true},
        Case{32, 0, false, true, false, 173, 40, .5f},
        Case{32, 0, true, true, false, 173, 40, .5f},
        Case{48, 1, true, true, false, 173, 48, .75f},
        Case{32, 0, true, true, true, 173, 40, .5f},
        Case{32, 0, true, true}}) {
        const unsigned height = test.height ? test.height : test.size;
        const unsigned outputWidth = std::lround(test.size / test.scale), outputHeight = std::lround(height / test.scale);
        const std::string scale = std::to_string(test.scale);
        Require(CVar::Lookup("gxRenderScale")->Set(scale.c_str(), true, false, false, false), "Cannot set scene scale");
        FFX::SetEffect(test.death ? static_cast<FFX::Effect*>(scope.death.get()) : scope.effect.get());
        DayNight::GetInfo()->m_bands.m_glow = test.glowAmount / 255.0f;
        ConsoleCommandExecute(test.global ? "set ffx 1" : "set ffx 0", 0);
        ConsoleCommandExecute(test.glow ? "set ffxGlow 1" : "set ffxGlow 0", 0);
        ConsoleCommandExecute(test.glow ? "set ffxDeath 1" : "set ffxDeath 0", 0);
        const int parameters[] = {test.mode, 173, 85}; scope.effect->SetParameters(3, parameters);
        const bool enabled = test.global && test.glow;
        auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:outputWidth height:outputHeight mipmapped:NO];
        descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
        auto frame = [MTLRenderPassDescriptor renderPassDescriptor];
        frame.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
        frame.colorAttachments[0].loadAction = MTLLoadActionClear;
        Image source{test.size, height, std::vector<Pixel>(test.size * height)};
        for (unsigned y = 0; y < height; ++y) { for (unsigned x = 0; x < test.size; ++x) {
            source.pixels[y * test.size + x] = {uint8_t((x * 53 + y * 29) % 256), uint8_t((x * 79 + y * 17) % 256),
                uint8_t((x * 23 + y * 61) % 256), uint8_t((x * 31 + y * 37) % 256)};
        } }
        if (scope.source) { HandleClose(reinterpret_cast<HOBJECT>(scope.source)); }
        scope.source = Texture(test.size, height, test.size, height, &source).m_texture;
        GxRsSet(GxRs_VertexShader, scope.vertex[0]); GxRsSet(GxRs_PixelShader, scope.pixel);
        GxRsSet(GxRs_Texture0, TextureGetGxTex(scope.source, 1, nullptr));
        GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_Culling, 0);
        GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque)); GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_ColorWrite, 15);
        // Prepare an exact scene image with the D3D9 texel/pixel alignment.
        C44Matrix identity(1); identity.a3=-1.0f/test.size; identity.b3=1.0f/height;
        GxShaderConstantsSet(GxSh_Vertex, 0, &identity.a0, 4);
        device.BeginFrame(frame); FFX::Start();
        const C3Vector positions[] = {{-1, 1, 0}, {-1, -1, 0}, {1, 1, 0}, {1, -1, 0}};
        const C2Vector coordinates[] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}}; const CImVector white(255, 255, 255, 255);
        GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, &white, 0, coordinates, sizeof(C2Vector), nullptr, 0);
        CGxBatch batch{GxPrim_TriangleStrip, 0, 4, 0, 3}; GxDraw(&batch, 0);
        GxRsSet(GxRs_VertexShader, static_cast<void*>(nullptr)); GxRsSet(GxRs_PixelShader, static_cast<void*>(nullptr));
        FFX::Finish();
        FFX::Tex screen{nullptr, {int(outputWidth), int(outputHeight)}, {int(outputWidth), int(outputHeight)}, {1.0f / outputWidth, 1.0f / outputHeight}};
        auto actual = Read(device, screen);
        auto compareOutput = [&](const Image& expected, unsigned tolerance) {
            if (test.scale == 1) Compare(actual, expected, tolerance);
            else Compare(actual, UpscaleScene(expected, outputWidth, outputHeight), tolerance + 1);
        };
        if (!enabled) { compareOutput(source, 0); }
        else {
            Require(FFX::s_full[0].m_size.x == int(test.size) && FFX::s_quart[0].m_size.x == int(test.size / 4), "FFX Start did not allocate/resize original targets");
            Require(FFX::s_full[0].m_texSize.x == int(test.size) && FFX::s_full[0].m_texSize.y == int(height) &&
                FFX::s_quart[0].m_texSize.x == int(test.size / 4) && FFX::s_quart[0].m_texSize.y == int(height / 4),
                "Unrestricted native NPOT capability still selected padded FFX targets");
            Compare(Read(device, FFX::s_full[0]), source, 0);
            auto filtered = Read(device, FFX::s_quart[0]);
            auto reference = Reference(source, test.size / 4, height / 4, test.size, height,
                {{{-1.5f,-1.5f},{0.5f,-1.5f},{0.5f,0.5f},{-1.5f,0.5f}}}, true);
            reference = Reference(reference, reference.width, reference.height, reference.width, reference.height,
                {{{-2.5f,0},{-0.5f,0},{0.5f,0},{2.5f,0}}}, false);
            reference = Reference(reference, reference.width, reference.height, reference.width, reference.height,
                {{{0,-2.5f},{0,-0.5f},{0,0.5f},{0,2.5f}}}, false);
            Compare(filtered, reference, 2);
            const CImVector color(85, 85, 85, 173);
            if (test.death) { compareOutput(ScreenCoverage(device,DeathReference(source, filtered, test.glowAmount),&source), 1); }
            else if (!test.mode) { compareOutput(ScreenCoverage(device,GlowReference(source, filtered, color),&source), 1); }
            else {
                const auto* buffer = device.m_primVertexFormatBuf[GxVA_Position];
                FFX::VertexPCT3 vertices[4];
                std::memcpy(vertices, static_cast<const uint8_t*>(buffer->m_pool->m_mem) + buffer->m_index, sizeof(vertices));
                compareOutput(ScreenCoverage(device,WaveReference(source, filtered, wave, vertices, color, device.Caps().m_texFmt[GxTex_Uv88] != 0),&source), 2);
            }
        }
        GxScenePresent();
    }
}
