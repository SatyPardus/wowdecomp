#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
void White(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t pixel = 0xFFFFFFFF;
    if (command == GxTex_Latch) { stride = 4; data = &pixel; }
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
    GxTexDestroy(large); GxTexDestroy(white);
}
