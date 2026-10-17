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
id<MTLBuffer> Quad(GxMetal::TextureDevice& gpu, const std::array<simd_float2, 4>& offsets, uint32_t color) {
    Vertex vertices[4]{};
    for (unsigned i = 0; i < 4; ++i) {
        float u = i >> 1, v = i & 1;
        vertices[i].x = u * 2 - 1; vertices[i].y = 1 - v * 2; vertices[i].color = color;
        for (unsigned tap = 0; tap < 4; ++tap) { vertices[i].uv[tap][0] = u + offsets[tap].x; vertices[i].uv[tap][1] = v + offsets[tap].y; }
    }
    return [gpu.device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
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
    GxMetal::GeometryDraw geometry;
    geometry.format = GxVertexBufferFormats_Last;
    geometry.batch = {GxPrim_TriangleStrip, 0, 4, 0, 3};
    geometry.vertices = Quad(gpu, offsets, 0x55112233);
    for (auto effect : {GxMetal::FragmentEffect::Box4, GxMetal::FragmentEffect::Gauss4}) {
        auto bytes = Asset(effect == GxMetal::FragmentEffect::Box4 ? "Shaders\\Pixel\\ps_2_0\\FFXBox4.bls" : "Shaders\\Pixel\\ps_2_0\\FFXGauss4.bls");
        auto shader = GxMetal::Filter4Shader::FromBls(bytes, effect);
        geometry.effect = shader.Effect();
        auto pass = Target(gpu, 8, 8); auto command = [gpu.queue commandBuffer];
        gpu.DrawFilter4(command, pass, textures, geometry, states);
        Compare(Read(gpu, command, pass.colorAttachments[0].texture), Reference(inputs, states, offsets, 8, effect, 0x55), 1);
        bool rejected = false; bytes.back() ^= 1;
        try { GxMetal::Filter4Shader::FromBls(bytes, effect); } catch (const std::runtime_error&) { rejected = true; }
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
}
