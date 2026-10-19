#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Gx/Blit.h"
#include "Engine/Source/BLPFile/BlpImage.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
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
struct RecreationData {
    uint32_t texels[4][64]{};
    unsigned notifications = 0, latches[4]{}, locks = 0, unlocks = 0;
    uint32_t previousBase = 0;
};
void RecreationTexture(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t, uint32_t level,
    void* arg, uint32_t& stride, const void*& data) {
    auto& state = *static_cast<RecreationData*>(arg);
    if (command == GxTex_3) { ++state.notifications; state.previousBase = g_theGxDevicePtr->DeviceBaseMipLevel(); }
    if (command == GxTex_Lock) { ++state.locks; }
    if (command == GxTex_Unlock) { ++state.unlocks; }
    if (command == GxTex_Latch) {
        Require(level < 4 && width == (8u >> level) && height == width, "Recreated Gx mip callback extent/index mismatch");
        ++state.latches[level]; stride = width * 4; data = state.texels[level];
    }
}
unsigned recreationNotifications = 0;
uint32_t recreationPreviousBase = 0;
void RecreationNotification() { ++recreationNotifications; recreationPreviousBase = g_theGxDevicePtr->DeviceBaseMipLevel(); }
struct RecreationScope {
    CGxDeviceMetal& device;
    explicit RecreationScope(CGxDeviceMetal& d) : device(d) { recreationNotifications = 0; device.AddTextureRecreationCallback(RecreationNotification); }
    ~RecreationScope() { device.RemoveTextureRecreationCallback(RecreationNotification); }
};
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
struct TargetProducer {
    CGxDeviceMetal* device;
    CGxTex* target = nullptr;
    CGxTex* white;
    unsigned locks = 0, latches = 0, unlocks = 0, invalidations = 0;
    bool fail = false;
};
void ProduceTarget(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level,
    void* arg, uint32_t&, const void*&) {
    auto& p = *static_cast<TargetProducer*>(arg);
    if (command == GxTex_3) { ++p.invalidations; return; }
    if (command == GxTex_Lock) { ++p.locks; return; }
    if (command == GxTex_Unlock) { ++p.unlocks; return; }
    Require(command == GxTex_Latch && width == 8 && height == 8 && face == 0 && level == 0,
        "Render target producer callback dimensions changed");
    ++p.latches;
    if (p.fail) throw std::runtime_error("Intentional render target producer failure");
    CGxTex* saved = nullptr; p.device->DeviceGetRenderTarget(GxBuffers_Color, saved);
    float x0,x1,y0,y1,z0,z1; GxXformViewport(x0,x1,y0,y1,z0,z1);
    p.device->RsPush();
    GxRenderTargetSet(GxBuffers_Color, p.target, 0);
    GxXformSetViewport(0,1,0,1,0,1); p.device->SceneClear(1,CImVector(0,255,0,255));
    GxXformSetViewport(0,0.5f,0,1,0,1);
    Draw(*p.device,p.white,p.latches & 1 ? 0xFFFF0000 : 0xFF0000FF,0);
    GxRenderTargetSet(GxBuffers_Color,saved,0);
    GxXformSetViewport(x0,x1,y0,y1,z0,z1);
    p.device->RsPop();
}
void CheckTargetProducer(CGxDeviceMetal& device, CGxTex* white) {
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(1,1,0,1);
    TargetProducer p{&device,nullptr,white};
    CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget=1;
    Require(device.TexCreate(GxTex_2d,8,8,0,GxTex_Argb8888,GxTex_Argb8888,flags,&p,ProduceTarget,"RTT producer",p.target), "Cannot create producer target");
    device.BeginFrame(pass); GxXformSetViewport(0,1,0,1,0,1);
    GxRsSet(GxRs_DepthTest,0); GxRsSet(GxRs_DepthWrite,0); GxRsSet(GxRs_ScissorTest,0);
    GxRenderTargetSet(GxBuffers_Color,p.target,0); GxRenderTargetSet(GxBuffers_Color,nullptr,0);
    Require(p.latches==0,"Writable target binding unexpectedly ran its producer");
    Require(device.TexCopy(p.target,{0,0},{0,0},{8,8},0,0) && p.latches==0,"Framebuffer copy unexpectedly ran its destination producer");
    for (unsigned generation=1;generation<=4;++generation) {
        device.SceneClear(1,CImVector(0,255,255,255));
        GxRsSet(GxRs_Texture0,white);
        if (generation==2) GxTexUpdate(p.target,0,0,0,0,0);
        if (generation==3) { device.DeviceSetBaseMipLevel(0); Require(p.invalidations==1,"Producer invalidation was not delivered"); }
        if (generation==4) {
            GxTexUpdate(p.target,0,0,0,0,0); p.fail=true;
            bool rejected=false; try { device.ITexMarkAsUpdated(p.target); } catch (const std::runtime_error&) { rejected=true; }
            Require(rejected && p.target->m_needsUpdate && p.unlocks==p.locks,"Failed producer did not remain retryable or unlock");
            p.fail=false;
        }
        GxRsSet(GxRs_Texture0,p.target);
        Require(p.latches==generation+(generation==4) && p.unlocks==p.locks,"Producer callback lifecycle or repeated binding changed");
        Solid(Read(device,16,16),0xFFFFFF00,"Producer changed the default surface");
        Draw(device,p.target,0xFFFFFFFF,0);
        const auto pixels=Read(device,16,16);
        for (unsigned y=0;y<16;++y) for (unsigned x=0;x<16;++x)
            Require(pixels[y*16+x]==(x<8 ? (p.latches & 1 ? 0xFFFF0000 : 0xFF0000FF) : 0xFF00FF00),"Callback rendering was not sampled correctly");
    }
    GxRsSet(GxRs_Texture0,static_cast<CGxTex*>(nullptr)); GxTexDestroy(p.target); GxScenePresent();
}
void CheckDepthTargets(CGxDeviceMetal& device, CGxTex* white) {
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device; CGxTex* target[4]{};
        ~Cleanup() {
            GxRenderTargetSet(GxBuffers_Depth,nullptr,0); GxRenderTargetSet(GxBuffers_Color,nullptr,0);
            if(device.FrameCommand()) device.ScenePresent();
            for(auto* texture:target) if(texture) device.TexDestroy(texture);
            device.RsPop();
        }
    } cleanup{device};
    CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget=1;
    for(unsigned i=0;i<4;++i) {
        const auto format=i==0 ? GxTex_Argb8888:GxTex_D24X8;
        const unsigned size=i==0 || i==3 ? 8:16;
        Require(device.TexCreate(GxTex_2d,size,size,0,format,format,flags,nullptr,nullptr,"Independent depth attachment",cleanup.target[i]),"Cannot create depth fixture");
    }
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode=MTLStorageModePrivate; descriptor.usage=MTLTextureUsageRenderTarget;
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=[device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction=MTLLoadActionClear; pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,1,1);
    descriptor.pixelFormat=MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture=[device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction=MTLLoadActionClear; pass.depthAttachment.clearDepth=.75;
    for(auto state:{GxRs_DepthTest,GxRs_ScissorTest,GxRs_AlphaRef,GxRs_Culling,GxRs_ClipPlaneMask,GxRs_PolygonOffset}) GxRsSet(state,0);
    GxRsSet(GxRs_DepthWrite,1); GxRsSet(GxRs_ColorWrite,15); GxRsSet(GxRs_BlendingMode,GxBlend_Opaque);
    GxXformSetViewport(0,1,0,1,0,1);
    auto depths=[&](unsigned size) {
        CiRect rect(0,0,size,size); TSGrowableArray<float> values; device.DeviceReadDepths(rect,values);
        return std::vector<float>(values.Ptr(),values.Ptr()+values.Count());
    };
    device.BeginFrame(pass);
    GxRenderTargetSet(GxBuffers_Depth,cleanup.target[1],0);
    device.SceneClear(2,CImVector(0,0,0,0)); Draw(device,white,0xFFFF0000,.25f);
    for(float value:depths(16)) Require(value==.25f,"Explicit depth did not receive original UI geometry");
    GxRenderTargetSet(GxBuffers_Color,cleanup.target[0],0);
    device.SceneClear(3,CImVector(0,255,0,255));
    for(float value:depths(8)) Require(value==1,"Small color viewport did not clear explicit depth");
    GxRenderTargetSet(GxBuffers_Color,nullptr,0);
    const auto retained=depths(16);
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x)
        Require(retained[y*16+x]==(x<8 && y<8 ? 1:.25f),"Small clear changed depth outside its color extent");
    device.DeviceCopyTex(cleanup.target[1],0,cleanup.target[2],0);
    GxRenderTargetSet(GxBuffers_Depth,cleanup.target[2],0);
    Require(depths(16)==retained,"Depth blit changed samples or command order");
    Solid(Read(device,16,16),0xFFFF0000,"Depth target switch repeated the host color clear");
    device.ScenePresent();
    device.BeginFrame(pass);
    Require(depths(16)==retained,"Explicit depth contents did not persist across frames");
    GxXformSetViewport(0,.5f,0,1,0,1); Draw(device,white,0xFF00FF00,.5f);
    GxXformSetViewport(0,1,0,1,0,1);
    const auto beforeDelete=Read(device,16,16);
    device.TexDestroy(cleanup.target[2]); cleanup.target[2]=nullptr;
    for(float value:depths(16)) Require(value==.75f,"Destroying bound depth did not restore independent host depth");
    Require(Read(device,16,16)==beforeDelete,"Destroying explicit depth changed host color");
    GxRenderTargetSet(GxBuffers_Color,cleanup.target[0],0);
    GxRenderTargetSet(GxBuffers_Depth,cleanup.target[3],0);
    device.SceneClear(3,CImVector(0,0,0,0)); Draw(device,white,0xFFFFFFFF,.625f);
    for(float value:depths(8)) Require(value==.625f,"Small explicit depth mismatch");
    bool rejected=false;
    try { device.DeviceCopyTex(cleanup.target[1],0,cleanup.target[3],0); } catch(const std::runtime_error&) { rejected=true; }
    Require(rejected,"Depth copy accepted unequal extents");
    GxRenderTargetSet(GxBuffers_Color,nullptr,0);
    rejected=false; try { Draw(device,white,0xFFFFFFFF,0); } catch(const std::runtime_error&) { rejected=true; }
    Require(rejected,"Undersized explicit depth was submitted to Metal");
    GxRenderTargetSet(GxBuffers_Depth,nullptr,0);
    rejected=false; try { GxRenderTargetSet(GxBuffers_Color,cleanup.target[1],0); } catch(const std::runtime_error&) { rejected=true; }
    Require(rejected,"Depth texture was accepted as a color attachment");
    std::fprintf(stderr,"Explicit depth targets: independent host/scratch/depth lifetimes, regional clear, ordered exact copy, cross-frame binding, destruction and invalid format/extent rejection passed; native Depth32Float precision\n");
}
void CheckRecreation(CGxDeviceMetal& device) {
    RecreationScope scope(device);
    RecreationData data;
    const uint32_t colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF};
    for (unsigned level = 0; level < 4; ++level) { std::fill(std::begin(data.texels[level]), std::end(data.texels[level]), colors[level]); }
    CGxTex* texture = nullptr, *unused = nullptr, *target = nullptr;
    CGxTexFlags flags(GxTex_NearestMipNearest);
    device.TexCreate(GxTex_2d, 8, 8, 0, GxTex_Argb8888, GxTex_Argb8888, flags, &data, RecreationTexture, "Gx mip recreation", texture);
    RecreationData unusedData;
    device.TexCreate(GxTex_2d, 8, 8, 0, GxTex_Argb8888, GxTex_Argb8888, flags, &unusedData, RecreationTexture, "Gx nonresident texture", unused);
    flags = CGxTexFlags(GxTex_Nearest); flags.m_renderTarget = 1;
    device.TexCreate(GxTex_2d, 8, 8, 0, GxTex_Argb8888, GxTex_Unknown, flags, nullptr, nullptr, "Gx recreated target", target);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:8 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    device.BeginFrame(pass); GxRenderTargetSet(GxBuffers_Color, target, 0); device.SceneClear(1, CImVector(255, 0, 0, 255));
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
    GxXformSetViewport(0, 0.5f, 0, 1, 0, 1); Draw(device, texture, 0xFFFFFFFF, 0);
    device.DeviceSetBaseMipLevel(1);
    Require(data.notifications == 1 && data.previousBase == 0 && recreationNotifications == 1 && recreationPreviousBase == 0 &&
        unusedData.notifications == 0 && target->m_needsCreation && target->m_needsUpdate && !target->m_apiSpecificData,
        "Gx resource invalidation or producer/cache notification order mismatch");
    GxXformSetViewport(0.5f, 1, 0, 1, 0, 1); Draw(device, texture, 0xFFFFFFFF, 0);
    auto pixels = Read(device, 16, 8);
    for (unsigned y = 0; y < 8; ++y) { for (unsigned x = 0; x < 16; ++x) {
        Require(pixels[y * 16 + x] == colors[x < 8 ? 0 : 1], "Gx mip recreation changed an earlier draw or selected the wrong authored mip");
    } }
    Require(data.latches[0] == 1 && data.latches[1] == 2 && data.latches[2] == 2 && data.latches[3] == 2 && data.locks == data.unlocks,
        "Gx recreation did not upload the retained authored mip chain");
    GxXformSetViewport(0, 1, 0, 1, 0, 1); GxRenderTargetSet(GxBuffers_Color, target, 0);
    device.SceneClear(1, CImVector(0, 255, 255, 255));
    Solid(Read(device, 8, 8), 0xFFFFFF00, "Gx render target did not recreate after mip invalidation");
    device.DeviceSetBaseMipLevel(0);
    Require(data.notifications == 2 && data.previousBase == 1 && recreationNotifications == 2 && recreationPreviousBase == 1,
        "Gx restored base-mip notifications were missing");
    // An attachment already bound by the frame retains its resource, just as a
    // D3D surface reference outlives invalidation of its parent texture object.
    Solid(Read(device, 8, 8), 0xFFFFFF00, "Gx invalidation discarded the active attachment");
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0); GxScenePresent();
    device.BeginFrame(pass); Draw(device, texture, 0xFFFFFFFF, 0);
    Solid(Read(device, 16, 8), colors[0], "Gx base mip restore failed"); GxScenePresent();
    device.DeviceSetBaseMipLevel(0);
    Require(data.notifications == 3 && recreationNotifications == 3 && unusedData.notifications == 0,
        "Explicit Gx recreation at unchanged base mip lost its notifications");
    for (auto* item : {texture, unused, target}) { GxTexDestroy(item); }
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
    CheckRecreation(device);
    CheckTargetProducer(device, white);
    CheckDepthTargets(device, white);
    GxTexDestroy(large); GxTexDestroy(white);
}

// Called with the original Services/Event lifecycle active. Recreate the same
// handle at each resident base mip, which exercises its synchronous reload too.
void CheckServicesTexturePixels(CGxTex* texture, const BLP::Image& reference, CTexture* atlasResource) {
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    CGxShader* vertex[2]{}; CGxShader* pixel = nullptr;
    device.ShaderCreate(vertex, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    device.ShaderCreate(&pixel, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
    GxRsSet(GxRs_VertexShader, vertex[0]); GxRsSet(GxRs_PixelShader, pixel);
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_Lighting, 0); GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_ScissorTest, 0);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque)); GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_ColorWrite, 15);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    for (uint32_t requested : {0u, 2u, 0u}) {
        device.DeviceSetBaseMipLevel(requested);
        if (atlasResource) {
            CStatus status;
            Require(TextureGetGxTex(atlasResource, 1, &status) == texture && status.IsEmpty(), "Atlas resource reload failed");
        }
        uint32_t width, height, base, end;
        device.ITexWHDStartEnd(texture, width, height, base, end);
        auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
        descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
        auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        device.BeginFrame(pass); Draw(device, texture, 0xFFFFFFFF, 0);
        const auto actual = Read(device, width, height);
        const auto expected = reference.DecodeBGRA(base);
        Require(expected.width == width && expected.height == height, "Services texture resident mip changed");
        const int tolerance = BLP::IsBlockCompressed(reference.format) && device.GPU().SupportsBC() ? 2 : 0;
        const auto* bytes = reinterpret_cast<const uint8_t*>(actual.data());
        for (size_t i = 0; i < expected.bytes.size(); ++i) {
            Require(std::abs(int(bytes[i]) - expected.bytes[i]) <= tolerance, "Services texture/reload differs from authored mip");
        }
        GxScenePresent();
    }
    GxRsSet(GxRs_Texture0, static_cast<CGxTex*>(nullptr));
    device.ShaderDestroy(vertex[0]); device.ShaderDestroy(vertex[1]); device.ShaderDestroy(pixel);
}

void CheckServicesTexturePixels(CGxTex* texture, const BLP::Image& reference) {
    CheckServicesTexturePixels(texture, reference, nullptr);
}

void CheckPackedTextureFormats(CGxDeviceMetal& device) {
    struct Source {
        std::array<uint32_t, 256> bgra;
        std::array<uint16_t, 256> packed;
        bool converted = false, partial = false;
    } source;
    auto callback = [](EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* arg, uint32_t& stride, const void*& pixels) {
        if (command != GxTex_Latch) { return; }
        const auto& src = *static_cast<Source*>(arg);
        const unsigned offset = src.partial ? 127 : 0;
        stride = (src.partial ? 1 : 256) * (src.converted ? 4 : 2);
        pixels = src.converted ? static_cast<const void*>(src.bgra.data() + offset) : static_cast<const void*>(src.packed.data() + offset);
    };
    for (EGxTexFormat format : {GxTex_Rgb565, GxTex_Argb1555, GxTex_Argb4444}) {
        const BlitFormat blit = format == GxTex_Rgb565 ? BlitFormat_Rgb565 : format == GxTex_Argb1555 ? BlitFormat_Argb1555 : BlitFormat_Argb4444;
        for (bool converted : {false, true}) {
            source.converted = converted; source.partial = false;
            for (unsigned x = 0; x < 256; ++x) { source.bgra[x] = x << 24 | ((x * 7) & 255) << 16 | ((255 - x) << 8) | ((x * 13) & 255); }
            Require(Blit(C2iVector{256, 1}, BlitAlpha_0, source.bgra.data(), 1024, BlitFormat_Argb8888,
                source.packed.data(), 512, blit), "Original 32-to-16 bit blit failed");
            CGxTex* texture = nullptr;
            CGxTexFlags flags(GxTex_Nearest);
            device.TexCreate(GxTex_2d, 256, 1, 0, format, converted ? GxTex_Argb8888 : format, flags, &source, callback, "packed UNORM reference", texture);
            for (bool partial : {false, true}) {
                if (partial) {
                    source.partial = true; source.bgra[127] = 0x80F97C03;
                    Require(Blit(C2iVector{256, 1}, BlitAlpha_0, source.bgra.data(), 1024, BlitFormat_Argb8888,
                        source.packed.data(), 512, blit), "Original partial reference blit failed");
                    flags.m_bit15 = 1; GxTexSetFlags(texture, flags);
                    GxTexUpdate(texture, 127, 0, 128, 1, 1);
                }
                for (bool linear : {false, true}) {
                    const unsigned width = linear ? 512 : 256;
                    flags.m_filter = linear ? GxTex_Linear : GxTex_Nearest; GxTexSetFlags(texture, flags);
                    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:1 mipmapped:NO];
                    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
                    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
                    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
                    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
                    device.BeginFrame(pass); Draw(device, texture, 0xFFFFFFFF, 0);
                    const auto actual = Read(device, width, 1); GxScenePresent();
                    auto channel = [&](int x, unsigned c) -> double {
                        x = std::clamp(x, 0, 255);
                        const uint32_t value = (source.bgra[x] >> (8 * c)) & 255;
                        unsigned bits = format == GxTex_Argb4444 ? 4 : c == 3 ? (format == GxTex_Argb1555 ? 1 : 0) : c == 1 && format == GxTex_Rgb565 ? 6 : 5;
                        return bits ? double(value >> (8 - bits)) / ((1u << bits) - 1) : 1.0;
                    };
                    for (unsigned x = 0; x < width; ++x) {
                        const double position = (x + 0.5) * 256 / width - 0.5;
                        const int left = int(std::floor(position)); const double weight = position - left;
                        for (unsigned c = 0; c < 4; ++c) {
                            const double expected = linear ? channel(left, c) * (1 - weight) + channel(left + 1, c) * weight : channel(int(x), c);
                            const int value = (actual[x] >> (8 * c)) & 255;
                            Require(std::abs(value - int(std::lround(expected * 255))) <= (linear ? 1 : 0),
                                "Native packed UNORM upload/quantization/filter/partial update mismatch");
                        }
                    }
                }
            }
            if (format == GxTex_Argb1555) {
                flags.m_filter = GxTex_Nearest; GxTexSetFlags(texture, flags); GxRsSet(GxRs_AlphaRef, 255);
                auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:256 height:1 mipmapped:NO];
                descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
                auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
                pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
                pass.colorAttachments[0].loadAction = MTLLoadActionClear;
                pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
                device.BeginFrame(pass); Draw(device, texture, 0xFFFFFFFF, 0);
                const auto actual = Read(device, 256, 1); GxScenePresent();
                for (unsigned x = 0; x < 256; ++x) { Require((actual[x] >> 24) == (source.bgra[x] & 0x80000000 ? 255 : 0), "Packed alpha failed exact GREATEREQUAL at one"); }
                GxRsSet(GxRs_AlphaRef, 0);
            }
            GxTexDestroy(texture);
        }
    }
    // R32F preserves negative values through the existing UI shader. This
    // checks all signed byte codes before/after filtering, rather than hiding
    // negative displacement by clamping it into a color readback.
    source.converted = source.partial = false;
    for (unsigned x = 0; x < 256; ++x) source.packed[x] = uint8_t(int(x) - 128) | (uint16_t(uint8_t(127 - int(x))) << 8);
    CGxTex* signedTexture = nullptr;
    CGxTexFlags signedFlags(GxTex_Nearest);
    device.TexCreate(GxTex_2d, 256, 1, 0, GxTex_Uv88, GxTex_Uv88, signedFlags, &source, callback, "signed displacement reference", signedTexture);
    for (bool partial : {false, true}) {
        if (partial) {
            source.partial = true; source.packed[127] = 0x7f80;
            signedFlags.m_bit15 = 1; GxTexSetFlags(signedTexture, signedFlags);
            GxTexUpdate(signedTexture, 127, 0, 128, 1, 1);
        }
        for (bool linear : {false, true}) {
            const unsigned width = linear ? 512 : 256;
            signedFlags.m_filter = linear ? GxTex_Linear : GxTex_Nearest; GxTexSetFlags(signedTexture, signedFlags);
            auto channel = [&](int x, unsigned component) {
                const int code = (source.packed[std::clamp(x, 0, 255)] >> (8 * component)) & 255;
                return std::max(-1.0, double(code < 128 ? code : code - 256) / 127);
            };
            auto sample = [&](unsigned x, unsigned component) {
                const double position = (x + 0.5) * 256 / width - 0.5;
                const int left = int(std::floor(position)); const double weight = position - left;
                return linear ? channel(left, component) * (1 - weight) + channel(left + 1, component) * weight : channel(x, component);
            };
            for (bool floating : {true, false}) {
                CGxTex* floatTarget = nullptr;
                if (floating) {
                    CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget = 1;
                    device.TexCreate(GxTex_2d, width, 1, 0, GxTex_R32F, GxTex_R32F, flags, nullptr, nullptr, "Signed float readback", floatTarget);
                }
                auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                    width:width height:1 mipmapped:NO];
                descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
                auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
                pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
                pass.colorAttachments[0].loadAction = MTLLoadActionClear;
                device.BeginFrame(pass);
                if (floating) GxRenderTargetSet(GxBuffers_Color, floatTarget, 0);
                Draw(device, signedTexture, 0xffffffff, 0);
                if (floating) {
                    CiRect rect(0, 0, 1, width); TSGrowableArray<float> values; device.ReadFloatColor(rect, values);
                    Require(values.Count() == width, "Signed displacement float readback extent changed");
                    for (unsigned x = 0; x < width; ++x)
                        Require(std::abs(values[x] - sample(x, 0)) < 0.00001, "UV88 signed normalization/filter/partial update mismatch");
                } else {
                    const auto values = Read(device, width, 1);
                    for (unsigned x = 0; x < width; ++x) {
                        const int red = (values[x] >> 16) & 255, green = (values[x] >> 8) & 255;
                        Require(std::abs(red - std::lround(std::clamp(sample(x, 0), 0.0, 1.0) * 255)) <= 1 &&
                            std::abs(green - std::lround(std::clamp(sample(x, 1), 0.0, 1.0) * 255)) <= 1 &&
                            (values[x] & 0xff0000ff) == 0xff000000, "UV88 channel order or missing-component defaults changed");
                    }
                }
                if (floating) GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
                GxScenePresent();
                if (floatTarget) GxTexDestroy(floatTarget);
            }
        }
    }
    GxTexDestroy(signedTexture);
    std::fputs("Native UV88: all 256 signed codes, point/linear sampling, partial updates and RG channel defaults passed\n", stderr);
}
