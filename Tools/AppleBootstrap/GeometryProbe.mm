#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Metal/UIShader.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
constexpr uint32_t extent = 16;
constexpr uint8_t background[] = {96, 64, 32, 255};
struct Vertex { float x, y, z; uint32_t color; float u, v; };
static_assert(sizeof(Vertex) == 24, "Gx PCT test layout");

GxMetal::GeometryDraw Quad(GxMetal::TextureDevice& gpu, uint32_t color = 0xFFFFFFFF, float z = 0.25f) {
    const Vertex vertices[] = {{-1, 1, z, color, 0, 0}, {-1, -1, z, color, 0, 1}, {1, 1, z, color, 1, 0}, {1, -1, z, color, 1, 1}};
    const uint16_t indices[] = {65535, 65535, 0, 1, 2, 2, 1, 3};
    GxMetal::GeometryDraw draw;
    draw.vertices = [gpu.device newBufferWithLength:256 + sizeof(vertices) options:MTLResourceStorageModeShared];
    Require(draw.vertices != nil, "Geometry vertex allocation failed");
    std::memcpy(static_cast<uint8_t*>(draw.vertices.contents) + 256, vertices, sizeof(vertices));
    draw.vertexOffset = 256;
    draw.indices = [gpu.device newBufferWithBytes:indices length:sizeof(indices) options:MTLResourceStorageModeShared];
    draw.batch = {GxPrim_Triangles, 2, 6, 0, 3};
    return draw;
}

std::vector<uint8_t> Render(GxMetal::TextureDevice& gpu, id<MTLTexture> texture,
    const std::vector<GxMetal::GeometryDraw>& draws, double clearDepth = 1) {
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:extent height:extent mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    id<MTLTexture> color = [gpu.device newTextureWithDescriptor:descriptor];
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    id<MTLTexture> depth = [gpu.device newTextureWithDescriptor:descriptor];
    id<MTLBuffer> readback = [gpu.device newBufferWithLength:extent * 256 options:MTLResourceStorageModeShared];
    Require(color && depth && readback, "Geometry target allocation failed");
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = color;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(32.0 / 255, 64.0 / 255, 96.0 / 255, 1);
    pass.depthAttachment.texture = depth;
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.storeAction = MTLStoreActionStore;
    pass.depthAttachment.clearDepth = clearDepth;
    id<MTLCommandBuffer> command = [gpu.queue commandBuffer];
    GxMetal::TextureDrawState state;
    state.filter = GxTex_Nearest;
    for (const auto& draw : draws) {
        gpu.DrawUnlit(command, pass, texture, draw, {}, state);
        pass.colorAttachments[0].loadAction = pass.depthAttachment.loadAction = MTLLoadActionLoad;
    }
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    [blit copyFromTexture:color sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
        sourceSize:MTLSizeMake(extent, extent, 1) toBuffer:readback destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:extent * 256];
    [blit endEncoding];
    [command commit];
    [command waitUntilCompleted];
    if (command.error) { throw std::runtime_error(command.error.localizedDescription.UTF8String); }
    std::vector<uint8_t> pixels(extent * extent * 4);
    for (unsigned y = 0; y < extent; ++y) { std::memcpy(pixels.data() + y * extent * 4, static_cast<uint8_t*>(readback.contents) + y * 256, extent * 4); }
    return pixels;
}
void Solid(const std::vector<uint8_t>& pixels, const uint8_t* color, const char* error) {
    for (size_t i = 0; i < pixels.size(); i += 4) { Require(!std::memcmp(pixels.data() + i, color, 4), error); }
}
}

void CheckGxGeometry(GxMetal::TextureDevice& gpu) {
    BLP::Image pattern;
    pattern.mips.push_back({2, 2, 8, {0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255, 0, 255, 255, 255}});
    auto draw = Quad(gpu);
    const auto pixels = Render(gpu, gpu.Upload(pattern), {draw});
    for (unsigned y = 0; y < extent; ++y) {
        for (unsigned x = 0; x < extent; ++x) {
            const size_t sample = ((y >= 8 ? 2 : 0) + (x >= 8 ? 1 : 0)) * 4;
            Require(!std::memcmp(pixels.data() + (y * extent + x) * 4, pattern.mips[0].bytes.data() + sample, 4),
                "Gx indexed PCT geometry/UV orientation mismatch");
        }
    }
    BLP::Image white;
    white.mips.push_back({1, 1, 4, {255, 255, 255, 255}});
    auto texture = gpu.Upload(white);
    draw = Quad(gpu, 0xFF123456);
    draw.clipFromModel.Scale(C3Vector(0.5f, 0.5f, 1));
    draw.clipFromModel.Translate(C3Vector(0.5f, 0, 0));
    draw.scissorEnabled = true;
    draw.scissor = {10, 5, 4, 6};
    const auto clipped = Render(gpu, texture, {draw});
    const uint8_t tint[] = {0x56, 0x34, 0x12, 255};
    for (unsigned y = 0; y < extent; ++y) {
        for (unsigned x = 0; x < extent; ++x) {
            const auto* expected = x >= 10 && x < 14 && y >= 5 && y < 11 ? tint : background;
            Require(!std::memcmp(clipped.data() + (y * extent + x) * 4, expected, 4), "Gx transform, BGRA vertex color or scissor mismatch");
        }
    }
    auto red = Quad(gpu, 0xFFFF0000, 0.25f), blue = Quad(gpu, 0xFF0000FF, 0.75f), green = Quad(gpu, 0xFF00FF00, 0.125f);
    red.depthTest = red.depthWrite = blue.depthTest = blue.depthWrite = green.depthTest = true;
    const uint8_t redPixel[] = {0, 0, 255, 255}, bluePixel[] = {255, 0, 0, 255}, greenPixel[] = {0, 255, 0, 255};
    Solid(Render(gpu, texture, {red, blue}), redPixel, "Gx depth ordering mismatch");
    auto nearerBlue = Quad(gpu, 0xFF0000FF, 0.2f);
    nearerBlue.depthTest = true;
    Solid(Render(gpu, texture, {red, green, nearerBlue}), bluePixel, "Gx disabled depth write changed depth");
    for (uint32_t function = 0; function < 4; ++function) {
        green.depthFunction = function;
        Solid(Render(gpu, texture, {green}, 0.125), function == 3 ? background : greenPixel, "Gx depth comparison equality mismatch");
    }
    red.depthTest = false;
    Solid(Render(gpu, texture, {red, blue}), redPixel, "Gx disabled depth comparison incorrectly disabled depth writes");
    // Both culling directions must select opposite winding, without altering
    // the no-culling image. Absolute D3D9 winding equivalence needs comparison.
    red.depthTest = red.depthWrite = false;
    red.culling = 1;
    const auto front = Render(gpu, texture, {red});
    red.culling = 2;
    const auto back = Render(gpu, texture, {red});
    for (size_t i = 0; i < front.size(); i += 4) {
        Require((!std::memcmp(front.data() + i, redPixel, 4) && !std::memcmp(back.data() + i, background, 4)) ||
            (!std::memcmp(back.data() + i, redPixel, 4) && !std::memcmp(front.data() + i, background, 4)), "Gx opposite cull modes failed");
    }
    // Exercise every unskinned packed stride/attribute layout with the same
    // bytes supplied at the engine's declared offsets, without CPU repacking in Draw.
    for (unsigned format = 0; format < GxVertexBufferFormats_Last; ++format) {
        if (format == GxVBF_PBNT2) { continue; }
        const auto gx = static_cast<EGxVertexBufferFormat>(format);
        const uint32_t stride = GxVertexSize(gx);
        std::vector<uint8_t> bytes(stride * 3);
        const float positions[3][3] = {{-1, -1, 0}, {3, -1, 0}, {-1, 3, 0}};
        for (unsigned i = 0; i < 3; ++i) {
            std::memcpy(bytes.data() + i * stride, positions[i], 12);
            if (GxVertexHasAttrib(gx, GxVA_Color0)) { std::memset(bytes.data() + i * stride + GxVertexAttribOffset(gx, GxVA_Color0), 255, 4); }
        }
        GxMetal::GeometryDraw triangle;
        triangle.format = gx;
        triangle.batch = {GxPrim_Triangles, 0, 3, 0, 2};
        triangle.vertices = [gpu.device newBufferWithBytes:bytes.data() length:bytes.size() options:MTLResourceStorageModeShared];
        const uint8_t expected[] = {255, 255, 255, 255};
        Solid(Render(gpu, texture, {triangle}), expected, "Gx unskinned vertex layout mismatch");
    }
}

void CheckUIShader(GxMetal::TextureDevice& gpu, const GxMetal::UIShader& ui) {
    BLP::Image white;
    white.mips.push_back({1, 1, 4, {255, 255, 255, 255}});
    auto texture = gpu.Upload(white);
    const float registers[16] = {0.5f, 0, 0, 0.25f, 0, 0.5f, 0, -0.25f, 0, 0, 0, 1, 0, 0, 0, 2};
    for (uint32_t permutation = 0; permutation < 2; ++permutation) {
        auto draw = Quad(gpu, 0xFF102030);
        auto transform = ui.VertexTransform(registers, 4, permutation);
        draw.clipFromModel = transform.clipFromModel;
        draw.screenAtDepth = transform.screenAtDepth;
        const auto pixels = Render(gpu, texture, {draw});
        // Original DP4 equations give regular NDC [-.125,.375] x [-.375,.125].
        // Permutation 1 multiplies clip.xy by w before perspective divide,
        // giving [-.25,.75] x [-.75,.25] while retaining the same depth.
        const uint32_t start = permutation ? 6 : 7, end = permutation ? 14 : 11;
        const uint8_t tint[] = {0x30, 0x20, 0x10, 255};
        for (unsigned y = 0; y < extent; ++y) {
            for (unsigned x = 0; x < extent; ++x) {
                const auto* expected = x >= start && x < end && y >= start && y < end ? tint : background;
                Require(!std::memcmp(pixels.data() + (y * extent + x) * 4, expected, 4), "Native UI BLS permutation or register layout mismatch");
            }
        }
    }
}
