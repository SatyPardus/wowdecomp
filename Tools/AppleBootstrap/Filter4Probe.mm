#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
std::vector<uint8_t> Asset(const char* path) {
    SFileImpl* file = nullptr;
    Require(SFileOpenFile(path, &file) && file, "Missing original FFX filter asset");
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    uint32_t high = 0, size = SFileGetFileSize(file, &high), read = 0;
    Require(!high && size && size <= 1024 * 1024, "Invalid FFX filter asset size");
    std::vector<uint8_t> bytes(size);
    Require(SFileReadFile(file, bytes.data(), size, &read, nullptr, 0) && read == size, "Incomplete FFX filter asset");
    return bytes;
}
struct Vertex { float x, y, z; uint32_t color; float uv[4][2]; };
static_assert(sizeof(Vertex) == 48, "Original FFX PCT4 layout");
id<MTLBuffer> Quad(GxMetal::TextureDevice& gpu, const std::array<simd_float2, 4>& offsets, uint32_t color, unsigned uvCount = 4) {
    Vertex vertices[4]{};
    for (unsigned i = 0; i < 4; ++i) {
        float u = i >> 1, v = i & 1;
        vertices[i].x = u * 2 - 1; vertices[i].y = 1 - v * 2; vertices[i].color = color;
        for (unsigned tap = 0; tap < 4; ++tap) { vertices[i].uv[tap][0] = u + offsets[tap].x; vertices[i].uv[tap][1] = v + offsets[tap].y; }
    }
    const size_t stride = 16 + uvCount * 8;
    std::vector<uint8_t> packed(stride * 4);
    for (unsigned i = 0; i < 4; ++i) { std::memcpy(packed.data() + i * stride, &vertices[i], stride); }
    return [gpu.device newBufferWithBytes:packed.data() length:packed.size() options:MTLResourceStorageModeShared];
}
MTLRenderPassDescriptor* Target(GxMetal::TextureDevice& gpu, unsigned width, unsigned height) {
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [gpu.device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear; pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0x22 / 255.0, 0x33 / 255.0, 0x44 / 255.0, 0x11 / 255.0);
    return pass;
}
std::vector<uint8_t> Read(GxMetal::TextureDevice& gpu, id<MTLCommandBuffer> command, id<MTLTexture> image) {
    const size_t stride = (image.width * 4 + 255) & ~size_t(255);
    auto buffer = [gpu.device newBufferWithLength:stride * image.height options:MTLResourceStorageModeShared];
    auto blit = [command blitCommandEncoder];
    [blit copyFromTexture:image sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(image.width, image.height, 1)
        toBuffer:buffer destinationOffset:0 destinationBytesPerRow:stride destinationBytesPerImage:stride * image.height];
    [blit endEncoding]; [command commit]; [command waitUntilCompleted];
    if (command.error) { throw std::runtime_error(command.error.localizedDescription.UTF8String); }
    std::vector<uint8_t> result(image.width * image.height * 4);
    for (size_t y = 0; y < image.height; ++y) { std::memcpy(result.data() + y * image.width * 4, static_cast<const uint8_t*>(buffer.contents) + y * stride, image.width * 4); }
    return result;
}
float Sample(const BLP::Mip& image, float u, float v, unsigned channel, const GxMetal::TextureDrawState& state) {
    auto index = [](int p, int size, EGxTexWrapMode mode) { return mode == GxTex_Wrap ? (p % size + size) % size : std::clamp(p, 0, size - 1); };
    auto texel = [&](int x, int y) { return float(image.bytes[index(y, image.height, state.wrapV) * image.rowBytes + index(x, image.width, state.wrapU) * 4 + channel]); };
    if (state.filter == GxTex_Nearest) { return texel(std::floor(u * image.width), std::floor(v * image.height)); }
    float x = u * image.width - 0.5f, y = v * image.height - 0.5f;
    int ix = std::floor(x), iy = std::floor(y); float fx = x - ix, fy = y - iy;
    return (texel(ix, iy) * (1 - fx) + texel(ix + 1, iy) * fx) * (1 - fy) +
        (texel(ix, iy + 1) * (1 - fx) + texel(ix + 1, iy + 1) * fx) * fy;
}
std::vector<uint8_t> Reference(const std::array<BLP::Mip, 4>& inputs, const std::array<GxMetal::TextureDrawState, 4>& states,
    const std::array<simd_float2, 4>& offsets, unsigned size, GxMetal::FragmentEffect effect, uint8_t alpha) {
    std::vector<uint8_t> pixels(size * size * 4);
    const float gaussian[] = {0.125f, 0.375f, 0.375f, 0.125f};
    for (unsigned y = 0; y < size; ++y) { for (unsigned x = 0; x < size; ++x) { for (unsigned c = 0; c < 4; ++c) {
        float result = 0;
        for (unsigned tap = 0; tap < 4; ++tap) {
            result += Sample(inputs[tap], (x + 0.5f) / size + offsets[tap].x, (y + 0.5f) / size + offsets[tap].y, c, states[tap]) *
                (effect == GxMetal::FragmentEffect::Box4 ? 0.25f : gaussian[tap]);
        }
        pixels[(y * size + x) * 4 + c] = effect == GxMetal::FragmentEffect::Box4 && c == 3 ? alpha : std::lround(result);
    } } }
    return pixels;
}
void Compare(const std::vector<uint8_t>& actual, const std::vector<uint8_t>& expected, int tolerance) {
    Require(actual.size() == expected.size(), "FFX filter extent mismatch");
    for (size_t i = 0; i < actual.size(); ++i) { Require(std::abs(int(actual[i]) - expected[i]) <= tolerance, "Native FFX filter output differs from its original sampling equation"); }
}
void CheckFog(GxMetal::TextureDevice& gpu, const std::array<BLP::Mip, 4>& inputs,
    const std::array<id<MTLTexture>, 4>& textures, const std::array<GxMetal::TextureDrawState, 4>& originalStates,
    const std::array<simd_float2, 4>& offsets) {
    using Effect = GxMetal::FragmentEffect;
    for (auto effect : {Effect::PropagateFog, Effect::FogCombine}) {
        auto code = Asset(effect == Effect::PropagateFog ? "Shaders\\Pixel\\ps_2_0\\FFXPropagateFog.bls" : "Shaders\\Pixel\\ps_2_0\\FFXFogCombine.bls");
        GxMetal::PostProcessShader::FromBls(code, effect); code.back() ^= 1;
        bool rejected = false;
        try { GxMetal::PostProcessShader::FromBls(code, effect); } catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "Modified fog shader accepted");
    }
    GxMetal::GeometryDraw geometry; geometry.batch = {GxPrim_TriangleStrip, 0, 4, 0, 3};
    geometry.effect = Effect::PropagateFog; geometry.format = GxVertexBufferFormats_Last;
    geometry.vertices = Quad(gpu, offsets, 0xAB123456);
    for (float decay : {-0.2f, 0.0f, 0.17f, 1.2f}) for (unsigned alpha : {0u, 191u}) {
        geometry.fogEffect.density = {decay, 3, 4, 5};
        auto states = originalStates; states[0].alphaRef = alpha;
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawFogEffect(command, pass, textures, geometry, states);
        std::vector<uint8_t> expected(256);
        for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 8; ++x) {
            double channels[4]{};
            for (unsigned c = 0; c < 4; ++c) for (unsigned tap = 0; tap < 4; ++tap)
                channels[c] += Sample(inputs[tap], (x + 0.5f) / 8 + offsets[tap].x, (y + 0.5f) / 8 + offsets[tap].y, c, states[tap]) / 4;
            channels[3] -= decay * 255;
            for (unsigned c = 0; c < 4; ++c)
                expected[(y * 8 + x) * 4 + c] = alpha && channels[3] < alpha ? (0x11223344 >> (c * 8)) & 255 : std::clamp(std::lround(channels[c]), 0l, 255l);
        }
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), expected, 1);
    }
    geometry.effect = Effect::FogCombine; geometry.format = GxVBF_PCT2;
    geometry.vertices = Quad(gpu, offsets, 0x179BCDEF, 2);
    auto states = originalStates; states[0].alphaRef = 255;
    for (float value : {-0.5f, 0.0f, 0.25f, 1.0f, 1.5f}) {
        geometry.fogEffect.density = {value, value * 0.5f, 1.0f - value, 99};
        geometry.fogEffect.minimum = {0.1f, value, -value, 98};
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawFogEffect(command, pass, textures, geometry, states);
        std::vector<uint8_t> expected(256);
        for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 8; ++x) {
            double overlay[4], scene[3], gray = 0;
            for (unsigned c = 0; c < 4; ++c)
                overlay[c] = Sample(inputs[0], x / 8.0f + offsets[0].x, y / 8.0f + offsets[0].y, c, states[0]);
            for (unsigned c = 0; c < 3; ++c) {
                scene[c] = Sample(inputs[1], x / 8.0f + offsets[1].x, y / 8.0f + offsets[1].y, c, states[1]);
                gray += scene[c] * 0.33333298563957214;
            }
            for (unsigned c = 0; c < 3; ++c) {
                const double density = std::clamp(double(geometry.fogEffect.density[2-c]), 0.0, 1.0);
                const double minimum = std::clamp(double(geometry.fogEffect.minimum[2-c]), 0.0, 1.0);
                const double base = ((1-density)*scene[c] + density*gray)*(1-minimum) + 255*minimum;
                expected[(y * 8 + x) * 4 + c] = std::clamp(std::lround(base*(1-overlay[3]/255) + overlay[c]*overlay[3]/255), 0l, 255l);
            }
            expected[(y * 8 + x) * 4 + 3] = 255;
        }
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), expected, 1);
    }
}
void CheckNether(GxMetal::TextureDevice& gpu, const std::array<BLP::Mip, 4>& images,
    const std::array<GxMetal::TextureDrawState, 4>& originalStates, const std::array<simd_float2, 4>& offsets) {
    auto vertex = Asset("Shaders\\Vertex\\vs_2_0\\FFXNetherBlur.bls");
    GxMetal::PostProcessShader::ValidateNetherVertex(vertex);
    vertex.back() ^= 1;
    bool rejected = false;
    try { GxMetal::PostProcessShader::ValidateNetherVertex(vertex); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected, "Modified nether vertex shader accepted");
    for (const auto effect : {GxMetal::FragmentEffect::NetherBlur, GxMetal::FragmentEffect::NetherCombine}) {
        auto bytes = Asset(effect == GxMetal::FragmentEffect::NetherBlur ? "Shaders\\Pixel\\ps_2_0\\FFXNetherBlur.bls" : "Shaders\\Pixel\\ps_2_0\\FFXNetherCombine.bls");
        GxMetal::PostProcessShader::FromBls(bytes, effect); bytes.back() ^= 1; rejected = false;
        try { GxMetal::PostProcessShader::FromBls(bytes, effect); } catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "Modified nether pixel shader accepted");
    }
    auto inputs = images;
    std::array<id<MTLTexture>, 4> textures;
    for (unsigned i = 0; i < 4; ++i) {
        for (size_t j = 3; j < inputs[i].bytes.size(); j += 4) inputs[i].bytes[j] = 85;
        BLP::Image image; image.format = BLP::Format::BGRA8; image.mips.push_back(inputs[i]); textures[i] = gpu.Upload(image);
    }
    GxMetal::GeometryDraw geometry;
    geometry.batch = {GxPrim_TriangleStrip, 0, 4, 0, 3};
    geometry.effect = GxMetal::FragmentEffect::NetherBlur;
    geometry.format = GxVBF_PCT;
    for (float angle : {-8.1f, -0.4f, 0.1f, 3.5f}) for (uint32_t noise : {17u, 128u, 246u}) for (unsigned alpha : {0u, 85u, 86u}) {
        geometry.vertices = Quad(gpu, offsets, (noise << 16) | 0x7F0011DD, 1);
        geometry.netherBlur.angle = angle; geometry.netherBlur.tapSpacing = 0.091f;
        geometry.netherBlur.convergence = noise == 17 ? 0.25f : noise == 128 ? 1.0f : 3.5f;
        auto states = originalStates; states[0].alphaRef = alpha;
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawNether(command, pass, textures, geometry, states);
        std::vector<uint8_t> expected(8 * 8 * 4);
        // Independent original angle, fixed UV bias, four equal RGBA taps.
        const double direction = (2 * (noise / 255.0) - 1) * 3 + angle;
        for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 8; ++x) {
            for (unsigned c = 0; c < 4; ++c) {
                double result = 0;
                for (unsigned tap = 0; tap < 4; ++tap) {
                    const float u = x / 8.0 + offsets[0].x - 1.0 / 512 + tap * 0.091 * std::cos(direction);
                    const float v = y / 8.0 + offsets[0].y - 1.0 / 384 + tap * 0.091 * std::sin(direction);
                    result += Sample(inputs[tap], u, v, c, states[tap]) / 4;
                }
                const uint32_t clear = 0x11223344;
                expected[(y * 8 + x) * 4 + c] = alpha > 85 ? (clear >> (c * 8)) & 255 : std::lround(result);
            }
        }
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), expected, 1);
    }
    geometry.effect = GxMetal::FragmentEffect::NetherCombine; geometry.format = GxVBF_PCT2;
    geometry.vertices = Quad(gpu, offsets, 0x1379BDAF, 2);
    auto states = originalStates; states[0].alphaRef = 255;
    for (float fade : {0.0f, 0.3f, 0.75f, 1.0f}) {
        geometry.netherTintFade = {0.6f, 0.33f, 0.78f, fade};
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawNether(command, pass, textures, geometry, states);
        std::vector<uint8_t> expected(8 * 8 * 4);
        for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 8; ++x) {
            double original[3], combined[3], gray = 0;
            for (unsigned c = 0; c < 3; ++c) {
                original[c] = Sample(inputs[0], (x + 0.5f) / 8 + offsets[0].x, (y + 0.5f) / 8 + offsets[0].y, c, states[0]);
                combined[c] = original[c] * 0.5 + Sample(inputs[1], (x + 0.5f) / 8 + offsets[1].x, (y + 0.5f) / 8 + offsets[1].y, c, states[1]);
                gray += combined[c] * 0.16666649281978607;
            }
            const double tint[] = {0.78, 0.33, 0.6};
            for (unsigned c = 0; c < 3; ++c)
                expected[(y * 8 + x) * 4 + c] = std::clamp(std::lround(original[c] * (1 - fade) + (combined[c] * 0.5 + gray) * tint[c] * fade), 0l, 255l);
            expected[(y * 8 + x) * 4 + 3] = 255;
        }
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), expected, 1);
    }
}
void CheckDeath(GxMetal::TextureDevice& gpu, const std::array<BLP::Mip, 4>& inputs,
    const std::array<id<MTLTexture>, 4>& textures, const std::array<GxMetal::TextureDrawState, 4>& states,
    const std::array<simd_float2, 4>& offsets) {
    GxMetal::GeometryDraw geometry;
    geometry.effect = GxMetal::FragmentEffect::Death; geometry.format = GxVBF_PCT2;
    geometry.batch = {GxPrim_TriangleStrip, 0, 4, 0, 3};
    // Deliberately distinct RGB tint and alpha, including zero tint and zero
    // glow. Alpha test at 1 must keep every pixel, regardless of either texture.
    for (uint32_t color : {0u, 0x005393A8u, 0x805393A8u, 0xFF5393A8u, 0xFF0011FFu}) {
        geometry.vertices = Quad(gpu, offsets, color, 2);
        auto firstState = states[0]; firstState.alphaRef = 255;
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawDeath(command, pass, {textures[0], textures[1]}, geometry, {firstState, states[1]});
        std::vector<uint8_t> expected(8 * 8 * 4);
        for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 8; ++x) {
            double luminance = 0;
            // Independent ps_2_0 DP3 reference in BGRA storage order.
            const double weights[] = {0.144, 0.587, 0.299};
            for (unsigned c = 0; c < 3; ++c) {
                const double scene = Sample(inputs[0], (x + 0.5f) / 8 + offsets[0].x, (y + 0.5f) / 8 + offsets[0].y, c, states[0]) / 255.0;
                const double blur = Sample(inputs[1], (x + 0.5f) / 8 + offsets[1].x, (y + 0.5f) / 8 + offsets[1].y, c, states[1]) / 255.0;
                luminance += weights[c] * (scene + blur * blur * (color >> 24) / 255.0);
            }
            luminance = std::clamp(luminance, 0.0, 1.0);
            const double tint = std::clamp(4 * luminance * (1 - luminance), 0.0, 1.0);
            for (unsigned c = 0; c < 3; ++c)
                expected[(y * 8 + x) * 4 + c] = std::clamp(std::lround(255 * luminance + ((color >> (c * 8)) & 255) * tint), 0l, 255l);
            expected[(y * 8 + x) * 4 + 3] = 255;
        }
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), expected, 1);
    }
}
}

void CheckGxFilter4(GxMetal::TextureDevice& gpu) {
    std::array<BLP::Mip, 4> inputs;
    std::array<id<MTLTexture>, 4> textures;
    std::array<GxMetal::TextureDrawState, 4> states;
    const std::array<simd_float2, 4> offsets{{{0, 0}, {0.0625f, 0.25f}, {-0.25f, 0.125f}, {0.3125f, -0.1875f}}};
    for (unsigned tap = 0; tap < 4; ++tap) {
        inputs[tap] = {8, 8, 32, std::vector<uint8_t>(256)};
        for (unsigned y = 0; y < 8; ++y) { for (unsigned x = 0; x < 8; ++x) { for (unsigned c = 0; c < 4; ++c) {
            inputs[tap].bytes[(y * 8 + x) * 4 + c] = (x * 37 + y * 19 + tap * 53 + c * 31) & 255;
        } } }
        BLP::Image image; image.format = BLP::Format::BGRA8; image.mips.push_back(inputs[tap]); textures[tap] = gpu.Upload(image);
        states[tap].filter = tap & 1 ? GxTex_Linear : GxTex_Nearest;
        states[tap].wrapU = tap & 1 ? GxTex_Wrap : GxTex_Clamp;
        states[tap].wrapV = tap & 2 ? GxTex_Wrap : GxTex_Clamp;
    }
    auto deathBytes = Asset("Shaders\\Pixel\\ps_2_0\\FFXDeath.bls");
    GxMetal::PostProcessShader::FromBls(deathBytes, GxMetal::FragmentEffect::Death);
    deathBytes.back() ^= 1;
    bool rejectedDeath = false;
    try { GxMetal::PostProcessShader::FromBls(deathBytes, GxMetal::FragmentEffect::Death); }
    catch (const std::runtime_error&) { rejectedDeath = true; }
    Require(rejectedDeath, "Modified FFXDeath silently selected the original effect");
    CheckDeath(gpu, inputs, textures, states, offsets);
    CheckNether(gpu, inputs, states, offsets);
    CheckFog(gpu, inputs, textures, states, offsets);
    GxMetal::GeometryDraw geometry;
    geometry.format = GxVertexBufferFormats_Last;
    geometry.batch = {GxPrim_TriangleStrip, 0, 4, 0, 3};
    geometry.vertices = Quad(gpu, offsets, 0x55112233);
    for (auto effect : {GxMetal::FragmentEffect::Box4, GxMetal::FragmentEffect::Gauss4}) {
        auto bytes = Asset(effect == GxMetal::FragmentEffect::Box4 ? "Shaders\\Pixel\\ps_2_0\\FFXBox4.bls" : "Shaders\\Pixel\\ps_2_0\\FFXGauss4.bls");
        auto shader = GxMetal::PostProcessShader::FromBls(bytes, effect);
        geometry.effect = shader.Effect();
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawFilter4(command, pass, textures, geometry, states);
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), Reference(inputs, states, offsets, 8, effect, 0x55), 1);
        bool rejected = false; bytes.back() ^= 1;
        try { GxMetal::PostProcessShader::FromBls(bytes, effect); } catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "Modified FFX code silently selected the original effect");
    }
    geometry.effect = GxMetal::FragmentEffect::Box4;
    for (unsigned alphaRef : {85u, 86u}) {
        states[0].alphaRef = alphaRef;
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawFilter4(command, pass, textures, geometry, states);
        auto result = Read(gpu, command, pass.colorAttachments[0].texture);
        auto expected = Reference(inputs, states, offsets, 8, geometry.effect, 85);
        if (alphaRef == 86) { for (size_t i = 0; i < expected.size(); i += 4) { const uint32_t clear = 0x11223344; std::memcpy(expected.data() + i, &clear, 4); } }
        Compare(result, expected, 1);
    }
    for (auto& state : states) { state = {}; state.filter = GxTex_Linear; }
    geometry.effect = GxMetal::FragmentEffect::Gauss4;
    const std::array<simd_float2, 4> horizontal{{{-2.5f / 8, 0}, {-0.5f / 8, 0}, {0.5f / 8, 0}, {2.5f / 8, 0}}};
    const std::array<simd_float2, 4> vertical{{{0, -2.5f / 8}, {0, -0.5f / 8}, {0, 0.5f / 8}, {0, 2.5f / 8}}};
    auto first = Target(gpu, 8, 8), second = Target(gpu, 8, 8);
    auto command = [gpu.queue commandBuffer];
    geometry.vertices = Quad(gpu, horizontal, 0);
    gpu.DrawFilter4(command, first, {textures[0], textures[0], textures[0], textures[0]}, geometry, states);
    geometry.vertices = Quad(gpu, vertical, 0);
    auto intermediate = first.colorAttachments[0].texture;
    gpu.DrawFilter4(command, second, {intermediate, intermediate, intermediate, intermediate}, geometry, states);
    auto row = Reference({inputs[0], inputs[0], inputs[0], inputs[0]}, states, horizontal, 8, geometry.effect, 0);
    BLP::Mip filtered{8, 8, 32, std::move(row)};
    auto expected = Reference({filtered, filtered, filtered, filtered}, states, vertical, 8, geometry.effect, 0);
    Compare(Read(gpu, command, second.colorAttachments[0].texture), expected, 2);

    // Mixed packed inputs also exercise the simulator's per-texture decoding
    // in every FFX fragment family, including independent binding indices.
    const BLP::Format formats[] = {BLP::Format::RGB565, BLP::Format::ARGB1555, BLP::Format::ARGB4444, BLP::Format::RGB565};
    const uint16_t colors[] = {0xF800, 0x03E0, 0xF00F, 0xFFFF}; // red, green, blue, white
    for (unsigned tap = 0; tap < 4; ++tap) {
        BLP::Image packed; packed.format = formats[tap];
        packed.mips.push_back({1, 1, 2, std::vector<uint8_t>(2)});
        std::memcpy(packed.mips[0].bytes.data(), &colors[tap], 2);
        inputs[tap] = packed.DecodeBGRA(0); textures[tap] = gpu.Upload(packed);
        states[tap] = {}; states[tap].decodeScale = gpu.DecodeScale(packed.format);
    }
    CheckDeath(gpu, inputs, textures, states, {});
    geometry.vertices = Quad(gpu, {}, 0x80000040);
    for (auto effect : {GxMetal::FragmentEffect::Box4, GxMetal::FragmentEffect::Gauss4}) {
        geometry.effect = effect;
        auto pass = Target(gpu, 4, 4); auto command = [gpu.queue commandBuffer];
        gpu.DrawFilter4(command, pass, textures, geometry, states);
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), Reference(inputs, states, {}, 4, effect, 128), 1);
    }
    for (bool wave : {false, true}) {
        geometry.effect = wave ? GxMetal::FragmentEffect::GlowWave : GxMetal::FragmentEffect::Glow;
        geometry.format = wave ? GxVertexBufferFormats_Last : GxVBF_PCT2;
        geometry.vertices = Quad(gpu, {}, 0x80000040, wave ? 3 : 2);
        auto pass = Target(gpu, 4, 4); auto command = [gpu.queue commandBuffer];
        if (wave) { gpu.DrawGlowWave(command, pass, {textures[0], textures[1], textures[2]}, geometry, {states[0], states[1], states[2]}); }
        else { gpu.DrawGlow(command, pass, {textures[0], textures[1]}, geometry, {states[0], states[1]}); }
        std::vector<uint8_t> expected(4 * 4 * 4);
        for (unsigned i = 0; i < 16; ++i) {
            expected[i * 4 + (wave ? 1 : 2)] = 191;
            expected[i * 4 + (wave ? 0 : 1)] = 192;
            expected[i * 4 + 3] = 255;
        }
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), expected, 1);
    }
    // Black, white and each primary isolate both saturation boundaries and the
    // original nonstandard blue coefficient from the tint and blur terms.
    for (uint32_t color : {0u, 0x00FFFFFFu, 0x000000FFu, 0x0000FF00u, 0x00FF0000u}) {
        BLP::Image image; image.format = BLP::Format::BGRA8;
        image.mips.push_back({1, 1, 4, std::vector<uint8_t>(4)});
        std::memcpy(image.mips[0].bytes.data(), &color, 4);
        inputs[0] = inputs[1] = image.mips[0]; textures[0] = textures[1] = gpu.Upload(image);
        states[0] = {}; states[1] = {};
        CheckDeath(gpu, inputs, textures, states, {});
    }
}
