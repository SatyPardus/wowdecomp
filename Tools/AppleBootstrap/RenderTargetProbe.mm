#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
void White(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t pixel = 0xFFFFFFFF;
    if (command == GxTex_Latch) { stride = 4; data = &pixel; }
}
void Pattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* arg, uint32_t& stride, const void*& data) {
    if (command == GxTex_Latch) { stride = 32; data = arg; }
}
std::vector<uint32_t> Read(CGxDeviceMetal& device, int width, int height) {
    CiRect rect(0, 0, height, width); TSGrowableArray<CImVector> pixels;
    device.DeviceReadPixels(rect, pixels);
    Require(pixels.Count() == uint32_t(width * height), "Gx render target readback extent changed");
    std::vector<uint32_t> result(pixels.Count());
    std::memcpy(result.data(), pixels.Ptr(), result.size() * 4);
    return result;
}
void Solid(const std::vector<uint32_t>& pixels, uint32_t color, const char* message) {
    for (auto pixel : pixels) { Require(pixel == color, message); }
}
void Draw(CGxDeviceMetal& device, CGxTex* texture, uint32_t color, float z) {
    struct Vertex { float x, y, z; uint32_t color; float u, v; };
    const Vertex data[] = {{-1, 1, z, color, 0, 0}, {-1, -1, z, color, 0, 1},
        {1, 1, z, color, 1, 0}, {1, -1, z, color, 1, 1}};
    auto* buffer = GxBufStream(GxPoolTarget_Vertex, sizeof(Vertex), 4);
    std::memcpy(device.BufLock(buffer), data, sizeof(data)); device.BufUnlock(buffer, 0);
    GxPrimVertexPtr(buffer, GxVBF_PCT); GxRsSet(GxRs_Texture0, texture);
    C44Matrix identity(1); GxShaderConstantsSet(GxSh_Vertex, 0, &identity.a0, 4);
    CGxBatch batch{GxPrim_TriangleStrip, 0, 4, 0, 3}; GxDraw(&batch, 0);
}
void CheckClears(CGxDeviceMetal& device, CGxTex* small, CGxTex* white) {
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 1, 1);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 0.75;
    CiRect whole(0, 0, 16, 16); TSGrowableArray<float> depth;
    GxRsSet(GxRs_ScissorTest, 0); device.IRsSync(0);
    GxRsSet(GxRs_ColorWrite, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_AlphaRef, 255);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Add)); GxRsSet(GxRs_DepthTest, 1);
    for (uint32_t mask : {0, 1, 2, 3}) {
        device.BeginFrame(pass); GxXformSetViewport(0.25f, 0.75f, 0.25f, 0.5f, 0.2f, 0.4f);
        device.SceneClear(mask, CImVector(17, 34, 51, 68));
        auto pixels = Read(device, 16, 16); device.DeviceReadDepths(whole, depth);
        for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
            const bool inside = x >= 4 && x < 12 && y >= 8 && y < 12;
            Require(pixels[y * 16 + x] == (inside && (mask & 1) ? 0x44332211 : 0xFF0000FF), "Gx viewport color clear changed the wrong region or obeyed draw state");
            Require(depth[y * 16 + x] == (inside && (mask & 2) ? 1.0f : 0.75f), "Gx viewport depth clear changed outside pixels or used the viewport depth range");
        } }
        GxScenePresent();
    }
    GxXformSetViewport(0, 1, 0, 1, 0, 1); GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_AlphaRef, 0);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque));
    device.BeginFrame(pass);
    device.SetScissor(CRect{0.25f, 0.125f, 0.5f, 0.5f}); GxRsSet(GxRs_ScissorTest, 1);
    Draw(device, white, 0xFFFFFFFF, 0); // No color/depth writes; applies scissor.
    device.SetScissor(CRect{0, 0, 1, 1}); GxRsSet(GxRs_ScissorTest, 0);
    device.SceneClear(1, CImVector(0, 255, 0, 255));
    auto pixels = Read(device, 16, 16);
    for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(pixels[y * 16 + x] == (x >= 2 && x < 9 && y >= 8 && y < 13 ? 0xFF00FF00 : 0xFF0000FF),
            "Gx clear did not preserve the last applied scissor state");
    } }
    device.IRsSync(0); device.SceneClear(2, CImVector()); device.DeviceReadDepths(whole, depth);
    for (uint32_t i = 0; i < depth.Count(); ++i) { Require(depth[i] == 1, "Gx clear retained a disabled scissor"); }
    GxScenePresent(); GxRsSet(GxRs_ColorWrite, 15); GxRsSet(GxRs_DepthWrite, 1);
    CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget = 1;
    CGxTex* wide = nullptr;
    device.TexCreate(GxTex_2d, 16, 16, 0, GxTex_Argb8888, GxTex_Unknown, flags, nullptr, nullptr, "Gx clear target", wide);
    device.BeginFrame(pass); GxRenderTargetSet(GxBuffers_Color, wide, 0);
    device.SceneClear(3, CImVector()); Draw(device, white, 0xFFFF0000, 0.25f);
    GxRenderTargetSet(GxBuffers_Color, small, 0); device.SceneClear(2, CImVector());
    GxRenderTargetSet(GxBuffers_Color, wide, 0); device.DeviceReadDepths(whole, depth);
    for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(depth[y * 16 + x] == (x < 8 && y < 6 ? 1.0f : 0.25f), "Gx small-target clear erased depth outside its bounds");
    } }
    device.SceneClear(1, CImVector(0, 255, 0, 255));
    GxXformSetViewport(0, 0.5f, 0.13f, 0.38f, 0, 1);
    device.SceneClear(1, CImVector(0, 0, 255, 255));
    pixels = Read(device, 16, 16);
    for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(pixels[y * 16 + x] == (x < 8 && y >= 2 && y < 6 ? 0xFFFF0000 : 0xFF00FF00), "Gx offscreen clear origin/rounding mismatch");
    } }
    Draw(device, white, 0xFF0000FF, 0.25f); pixels = Read(device, 16, 16);
    for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(pixels[y * 16 + x] == (x < 8 && y >= 2 && y < 6 ? 0xFF0000FF : 0xFF00FF00), "Gx offscreen draw and clear use different viewport origins");
    } }
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    device.SetScissor(CRect{0.125f, 0.25f, 0.375f, 0.75f}); GxRsSet(GxRs_ScissorTest, 1);
    GxRsSet(GxRs_ColorWrite, 0); GxRsSet(GxRs_DepthWrite, 0); Draw(device, white, 0xFFFFFFFF, 0);
    device.SceneClear(1, CImVector(255, 255, 255, 255));
    auto scissored = Read(device, 16, 16);
    for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(scissored[y * 16 + x] == (x >= 4 && x < 13 && y >= 2 && y < 7 ? 0xFFFFFFFF : pixels[y * 16 + x]), "Gx offscreen scissor origin mismatch");
    } }
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
    device.SceneClear(1, CImVector(0, 255, 255, 255));
    Solid(Read(device, 16, 16), 0xFFFFFF00, "Gx target switch did not reset the applied scissor rectangle");
    GxScenePresent(); GxTexDestroy(wide);
    GxRsSet(GxRs_ColorWrite, 15); device.IRsSync(0);
}
void CheckCopies(CGxDeviceMetal& device, MTLRenderPassDescriptor* pass, CGxTex* offscreen) {
    auto& gpu = device.GPU();
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:8 height:8 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    pass.colorAttachments[0].texture = [gpu.device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.texture = nil;
    std::vector<uint32_t> pattern(64);
    for (unsigned y = 0; y < 8; ++y) { for (unsigned x = 0; x < 8; ++x) {
        pattern[y * 8 + x] = (x * 28) | ((y * 28) << 8) | (((x + y) * 16) << 16) | (((x * 3 + y * 5) % 16 * 17) << 24);
    } }
    CGxTex* source = nullptr;
    device.TexCreate(GxTex_2d, 8, 8, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest),
        pattern.data(), Pattern, "Gx copy pattern", source);
    auto target = [&](uint32_t width, uint32_t height, EGxTexFilter filter = GxTex_Nearest) {
        CGxTexFlags flags(filter); flags.m_renderTarget = 1;
        CGxTex* texture = nullptr;
        device.TexCreate(GxTex_2d, width, height, 0, GxTex_Argb8888, GxTex_Unknown, flags,
            nullptr, nullptr, "Gx copy destination", texture);
        return texture;
    };
    CGxTex* copy = target(8, 8), *clone = target(8, 8), *scaled = target(4, 4), *offset = target(12, 10);
    CGxTex* mip = target(16, 16, GxTex_NearestMipNearest);
    GxRsSet(GxRs_DepthWrite, 0);
    device.BeginFrame(pass); Draw(device, source, 0xFFFFFFFF, 0);
    Require(Read(device, 8, 8) == pattern, "Gx framebuffer copy source pattern mismatch");
    GxRenderTargetSet(GxBuffers_Color, copy, 0); device.SceneClear(1, CImVector(255, 0, 255, 255));
    GxRenderTargetSet(GxBuffers_Color, offscreen, 0); device.SceneClear(1, CImVector(0, 255, 0, 255));
    Require(GxTexCopy(copy, {2, 3}, {1, 2}, {3, 2}, 0, 0), "Gx partial framebuffer copy failed");
    Solid(Read(device, 8, 6), 0xFF00FF00, "Gx framebuffer copy changed the active offscreen target");
    GxRenderTargetSet(GxBuffers_Color, copy, 0);
    auto expected = std::vector<uint32_t>(64, 0xFFFF00FF);
    for (unsigned y = 0; y < 2; ++y) { for (unsigned x = 0; x < 3; ++x) { expected[(y + 3) * 8 + x + 2] = pattern[(y + 2) * 8 + x + 1]; } }
    Require(Read(device, 8, 8) == expected, "Gx partial copy origin, extent, outside pixels or default source mismatch");
    Require(!GxTexCopy(copy, {-1, 0}, {0, 0}, {2, 2}, 0, 0) &&
        !GxTexCopy(copy, {0, 0}, {0, 0}, {std::numeric_limits<int32_t>::max(), 2}, 0, 0) &&
        !GxTexCopy(copy, {0, 0}, {0, 0}, {2, 2}, 99, 0), "Gx copy accepted invalid range or mip");
    CiRect invalid(-1, 0, 8, 8);
    Require(!GxTexStretch(copy, &invalid, nullptr, 0, 0), "Gx stretch accepted an invalid rectangle");
    Require(Read(device, 8, 8) == expected, "Rejected Gx copy modified the destination");
    device.DeviceCopyTex(copy, 0, clone, 0); GxRenderTargetSet(GxBuffers_Color, clone, 0);
    Require(Read(device, 8, 8) == expected, "Gx texture-to-texture copy read the default surface instead of its source");
    Require(GxTexStretch(copy, nullptr, nullptr, 0, 0), "Gx full framebuffer capture failed");
    GxRenderTargetSet(GxBuffers_Color, copy, 0);
    Require(Read(device, 8, 8) == pattern, "Gx equal-size stretch did not preserve RGBA bytes");
    device.DeviceCopyTex(copy, 0, scaled, 0); GxRenderTargetSet(GxBuffers_Color, scaled, 0);
    auto nearest = Read(device, 4, 4);
    for (unsigned y = 0; y < 4; ++y) { for (unsigned x = 0; x < 4; ++x) {
        Require(nearest[y * 4 + x] == pattern[(2 * y + 1) * 8 + 2 * x + 1], "Gx D3DTEXF_NONE stretch selected the wrong texel");
    } }
    // Deliberately poison application render state. StretchRect is a surface
    // operation and must ignore it without changing it for the next game draw.
    GxRsSet(GxRs_ColorWrite, 0); GxRsSet(GxRs_AlphaRef, 255); GxRsSet(GxRs_DepthTest, 1);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Add));
    Require(GxTexStretch(scaled, nullptr, nullptr, 0, 0), "Gx linear framebuffer stretch failed");
    auto linear = Read(device, 4, 4);
    for (unsigned y = 0; y < 4; ++y) { for (unsigned x = 0; x < 4; ++x) { for (unsigned c = 0; c < 4; ++c) {
        unsigned sum = 0;
        for (unsigned dy = 0; dy < 2; ++dy) { for (unsigned dx = 0; dx < 2; ++dx) { sum += (pattern[(2 * y + dy) * 8 + 2 * x + dx] >> (8 * c)) & 255; } }
        Require(std::abs(int((linear[y * 4 + x] >> (8 * c)) & 255) - int((sum + 2) / 4)) <= 1, "Gx linear stretch RGBA filter mismatch");
    } } }
    Require(device.m_appRenderStates[GxRs_ColorWrite].m_value.m_data.u[0] == 0 &&
        device.m_appRenderStates[GxRs_AlphaRef].m_value.m_data.u[0] == 255, "Gx copy mutated application render state");
    GxRsSet(GxRs_ColorWrite, 15); GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque));
    GxRenderTargetSet(GxBuffers_Color, offset, 0); device.SceneClear(1, CImVector(255, 0, 255, 255));
    CiRect destination(2, 3, 6, 7);
    Require(GxTexStretch(offset, &destination, nullptr, 0, 0), "Gx offset stretch failed");
    auto region = Read(device, 12, 10);
    for (unsigned y = 0; y < 10; ++y) { for (unsigned x = 0; x < 12; ++x) {
        const uint32_t value = x >= 3 && x < 7 && y >= 2 && y < 6 ? linear[(y - 2) * 4 + x - 3] : 0xFFFF00FF;
        Require(region[y * 12 + x] == value, "Gx stretch viewport or pixels outside destination mismatch");
    } }
    Require(GxTexCopy(mip, {0, 0}, {0, 0}, {8, 8}, 1, 0), "Gx copy to a nonzero mip failed");
    Require(GxTexStretch(mip, nullptr, nullptr, 2, 0), "Gx linear stretch to a nonzero mip failed");
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0); Draw(device, mip, 0xFFFFFFFF, 0);
    Require(Read(device, 8, 8) == pattern, "Gx copied mip did not reach same-frame texture sampling");
    GxXformSetViewport(0, 0.5f, 0, 0.5f, 0, 1); Draw(device, mip, 0xFFFFFFFF, 0);
    CiRect mipRegion(4, 0, 8, 4); TSGrowableArray<CImVector> mipPixels;
    device.DeviceReadPixels(mipRegion, mipPixels);
    Require(mipPixels.Count() == 16 && !std::memcmp(mipPixels.Ptr(), linear.data(), 64), "Gx stretched mip did not reach same-frame sampling");
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    GxScenePresent();

    // The default attachment can still have an unconsumed host clear when the
    // frame begins with a persistent offscreen binding. Copy must consume it.
    GxRenderTargetSet(GxBuffers_Color, offscreen, 0);
    device.BeginFrame(pass);
    Require(GxTexCopy(copy, {0, 0}, {0, 0}, {8, 8}, 0, 0), "Gx pending-default-clear capture failed");
    GxRenderTargetSet(GxBuffers_Color, copy, 0);
    Solid(Read(device, 8, 8), 0xFF0000FF, "Gx capture read stale default contents before its clear");
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0); GxScenePresent();

    // Drawable hosts with framebufferOnly=false permit direct shader sampling,
    // so exercise that path as well as the attachment snapshot used above.
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    pass.colorAttachments[0].texture = [gpu.device newTextureWithDescriptor:descriptor];
    device.BeginFrame(pass); Draw(device, source, 0xFFFFFFFF, 0);
    Require(GxTexStretch(scaled, nullptr, nullptr, 0, 0), "Gx directly sampleable framebuffer stretch failed");
    GxRenderTargetSet(GxBuffers_Color, scaled, 0);
    Require(Read(device, 4, 4) == linear, "Gx direct and snapshot framebuffer scaling differ");
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0); GxScenePresent();
    for (auto* texture : {source, copy, clone, scaled, offset, mip}) { GxTexDestroy(texture); }
}
}

// Called with the original UI BLS shaders bound by CheckGxDevice.
void CheckGxRenderTargets(CGxDeviceMetal& device) {
    auto& gpu = device.GPU();
    CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget = 1;
    auto target = [&](uint32_t width, uint32_t height) {
        CGxTex* texture = nullptr;
        Require(device.TexCreate(GxTex_2d, width, height, 0, GxTex_Argb8888, GxTex_Unknown, flags,
            nullptr, nullptr, "Gx color target check", texture), "Cannot create Gx color target");
        return texture;
    };
    CGxTex* large = target(8, 6), *small = target(4, 3), *white = nullptr;
    device.TexCreate(GxTex_2d, 1, 1, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest),
        nullptr, White, "Gx target check white", white);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [gpu.device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 1, 1);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [gpu.device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 0.75;
    GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 1); GxRsSet(GxRs_AlphaRef, 0);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque)); GxRsSet(GxRs_ColorWrite, 15);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    device.BeginFrame(pass);
    GxRenderTargetSet(GxBuffers_Color, large, 0);
    device.SceneClear(3, CImVector(0, 255, 0, 255));
    GxXformSetViewport(0, 0.5f, 0, 1, 0, 1); Draw(device, white, 0xFFFF0000, 0.25f);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    const auto rendered = Read(device, 8, 6);
    for (unsigned y = 0; y < 6; ++y) { for (unsigned x = 0; x < 8; ++x) {
        Require(rendered[y * 8 + x] == (x < 4 ? 0xFFFF0000 : 0xFF00FF00), "Gx offscreen viewport/render mismatch");
    } }
    GxRenderTargetSet(GxBuffers_Color, small, 0);
    device.SceneClear(1, CImVector(0, 255, 255, 255));
    Solid(Read(device, 4, 3), 0xFFFFFF00, "Gx smaller color target mismatch");
    CiRect rect(0, 0, 3, 4); TSGrowableArray<float> depths;
    device.DeviceReadDepths(rect, depths);
    for (uint32_t i = 0; i < depths.Count(); ++i) { Require(depths[i] == 0.25f, "Gx scratch depth did not survive smaller target binding"); }
    GxRenderTargetSet(GxBuffers_Color, large, 0);
    device.DeviceSetBaseMipLevel(1); device.DeviceSetBaseMipLevel(0);
    Require(Read(device, 8, 6) == rendered, "Gx color target contents lost across binding changes");
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
    Solid(Read(device, 16, 16), 0xFF0000FF, "Gx default pending clear lost or applied to custom target");
    device.DeviceReadDepths(rect, depths);
    for (uint32_t i = 0; i < depths.Count(); ++i) { Require(depths[i] == 0.75f, "Gx custom depth changed the default depth surface"); }
    GxRsSet(GxRs_DepthWrite, 0); Draw(device, large, 0xFFFFFFFF, 0);
    const auto sampled = Read(device, 16, 16);
    for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(sampled[y * 16 + x] == (x < 8 ? 0xFFFF0000 : 0xFF00FF00), "Gx render target sampling/order mismatch");
    } }
    GxRenderTargetSet(GxBuffers_Color, small, 0); GxTexDestroy(small);
    Require(Read(device, 16, 16) == sampled, "Destroying a bound Gx target did not restore the default surface");
    GxScenePresent();

    // Binding persists between frames, but the default surface and its extent
    // belong to the host's newest drawable, including a resize while offscreen.
    GxRenderTargetSet(GxBuffers_Color, large, 0);
    descriptor.pixelFormat = MTLPixelFormatBGRA8Unorm; descriptor.width = 20; descriptor.height = 10;
    pass.colorAttachments[0].texture = [gpu.device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.texture = nil;
    device.BeginFrame(pass);
    Require(Read(device, 8, 6) == rendered, "Gx target binding/content did not persist between frames");
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
    CRect window; device.CapsWindowSize(window);
    Require(window.maxX == 20 && window.maxY == 10, "Gx default target size was stale after host resize");
    Solid(Read(device, 20, 10), 0xFF0000FF, "Gx resized default clear mismatch");
    GxScenePresent();
    CheckCopies(device, pass, large);
    CheckClears(device, large, white);
    GxTexDestroy(large); GxTexDestroy(white);
}
