#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <TargetConditionals.h>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
void CheckGxQueries(CGxDeviceMetal& device) {
#if TARGET_OS_SIMULATOR
    Require(!device.Caps().m_occlusionQuery, "Simulator must not advertise unsupported counting queries");
    std::fputs("Metal counting queries: unavailable on Simulator; original point-occlusion fallback retained\n", stderr);
    return;
#endif
    Require(device.Caps().m_occlusionQuery, "Physical Metal device has no counting queries");
    device.RsPush();
    const auto projection = device.m_projNative;
    device.m_projNative = C44Matrix(1);
    device.XformPushAndSet(GxXform_View, C44Matrix(1)); device.XformPushAndSet(GxXform_World, C44Matrix(1));
    struct Cleanup {
        CGxDeviceMetal& device; C44Matrix projection; CGxQuery* a = nullptr; CGxQuery* b = nullptr;
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.QueryDestroy(a); device.QueryDestroy(b); device.RsPop();
            GxXformPop(GxXform_World); GxXformPop(GxXform_View); device.m_projNative = projection;
        }
    } cleanup{device, projection};
    for (unsigned i = 0; i < 4; ++i) GxRsSet(EGxRenderState(GxRs_Texture0+i), static_cast<CGxTex*>(nullptr));
    GxRsSet(GxRs_VertexShader, static_cast<CGxShader*>(nullptr)); GxRsSet(GxRs_PixelShader, static_cast<CGxShader*>(nullptr));
    for (auto state : {GxRs_Lighting,GxRs_Fog,GxRs_DepthTest,GxRs_DepthWrite,GxRs_Culling,GxRs_ScissorTest,
                      GxRs_ClipPlaneMask,GxRs_PolygonOffset,GxRs_PointSprite,GxRs_AlphaRef}) GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 0); GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxXformSetViewport(0,1,0,1,0,1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = .25;
    C3Vector positions[]{{-2,2,.5f},{2,2,.5f},{-2,-2,.5f},{2,-2,.5f}};
    CImVector color(255,255,255,128);
    CGxBatch batch; batch.m_primType = GxPrim_TriangleStrip; batch.m_count = 4; batch.m_start = batch.m_minIndex = 0; batch.m_maxIndex = 3;
    unsigned checks = 0;
    auto draw = [&] {
        GxPrimVertexPtr(4,positions,sizeof(C3Vector),nullptr,0,&color,0,nullptr,0,nullptr,0); device.Draw(&batch,0);
    };
    auto answer = [&](CGxQuery* query, uint32_t expected) {
        uint32_t result = 0xBAD;
        Require(device.QueryGetData(query,result), "Metal query answer unavailable after explicit readback");
        if (result != expected) throw std::runtime_error("Metal query count mismatch at check " + std::to_string(checks) + ": " + std::to_string(result) + " != " + std::to_string(expected));
        ++checks;
    };
    device.QueryCreate(cleanup.a,GxQueryType_Occlusion); device.QueryCreate(cleanup.b,GxQueryType_Occlusion);
    auto* a=cleanup.a; auto* b=cleanup.b; uint32_t param = 99;
    Require(device.QueryGetParam(a,GxQueryParam_Unk0,param) && param == 4, "Gx query must return a DWORD");
    Require(!device.QueryGetParam(a,GxQueryParam_Unk1,param) && param == 1, "New Gx query must be signaled with original wrapper return value");
    answer(a,0);
    device.BeginFrame(pass); device.QueryBegin(a);
    Require(!device.QueryGetData(a,param), "Building query cannot be read");
    draw(); draw(); device.QueryEnd(a);
    const auto beforePoll=device.EncodingStats();
    for (int i=0;i<20;++i) {
        Require(!device.QueryGetParam(a,GxQueryParam_Unk1,param) && !param, "Unsubmitted query unexpectedly signaled");
    }
    Require(device.EncodingStats().querySubmissions == beforePoll.querySubmissions && device.EncodingStats().queryWaits == beforePoll.queryWaits,
        "Query availability poll submitted or waited on GPU work");
    answer(a,512); // Explicit readback submits, then the same frame remains usable.
    device.QueryBegin(b); draw(); device.QueryEnd(b); device.ScenePresent(); answer(b,256);

    device.BeginFrame(pass); GxRsSet(GxRs_DepthTest,1); GxRsSet(GxRs_DepthFunc,3); device.QueryBegin(a);
    draw(); for (auto& p:positions) p.z=.125f; draw();
    GxRsSet(GxRs_AlphaRef,129); draw(); GxRsSet(GxRs_AlphaRef,128); draw();
    device.QueryEnd(a); answer(a,512); device.ScenePresent();
    GxRsSet(GxRs_DepthTest,0); GxRsSet(GxRs_AlphaRef,0);

    device.BeginFrame(pass); const auto beforeClear=device.EncodingStats(); device.QueryBegin(a); draw();
    GxXformSetViewport(0,.5,0,1,0,1); device.SceneClear(1,CImVector(0,0,0,255)); GxXformSetViewport(0,1,0,1,0,1);
    draw(); device.QueryEnd(a); answer(a,512);
    Require(device.EncodingStats().renderPasses-beforeClear.renderPasses == 1, "Query clear boundary unnecessarily split the render pass");
    device.ScenePresent();

    device.BeginFrame(pass); device.QueryBegin(a); draw(); device.QueryBegin(b); draw();
    device.QueryEnd(a); draw(); device.QueryEnd(b); answer(a,512); answer(b,512); device.ScenePresent();
    device.BeginFrame(pass); device.QueryBegin(a); draw(); device.QueryBegin(a); draw(); draw(); device.QueryEnd(a);
    answer(a,512); device.QueryEnd(a); answer(a,0); device.ScenePresent();

    device.BeginFrame(pass); device.QueryBegin(a); draw();
    CiRect rect(0,0,16,16); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect,pixels);
    draw(); device.QueryEnd(a); answer(a,512); device.ScenePresent();
    device.BeginFrame(pass); device.QueryBegin(a); draw(); device.ScenePresent();
    device.BeginFrame(pass); draw(); device.QueryEnd(a); device.ScenePresent(); answer(a,512);

    // Keep an old answer while other frames recycle streaming and result pages.
    for (unsigned i=0;i<18;++i) {
        device.BeginFrame(pass); device.QueryBegin(b); draw(); device.QueryEnd(b); device.ScenePresent(); answer(b,256);
    }
    answer(a,512);
    device.BeginFrame(pass); const auto beforeMany=device.EncodingStats();
    for (unsigned i=0;i<600;++i) { device.QueryBegin(b); draw(); device.QueryEnd(b); }
    answer(b,256);
    Require(device.EncodingStats().renderPasses-beforeMany.renderPasses <= 3, "Query brackets created excessive render passes");
    device.ScenePresent(); answer(a,512);
    const auto allocations=device.EncodingStats().visibilityPageAllocations;
    for (unsigned i=0;i<12;++i) {
        device.BeginFrame(pass); device.QueryBegin(b); draw(); device.QueryEnd(b); device.ScenePresent(); answer(b,256);
    }
    Require(device.EncodingStats().visibilityPageAllocations == allocations, "Completed query storage did not recycle");
    device.BeginFrame(pass); device.QueryBegin(a); device.QueryBegin(b); draw(); device.QueryDestroy(cleanup.b);
    draw(); device.QueryEnd(a); answer(a,512); device.ScenePresent();

    // Exercise the original consumer too: its projected planet area converts
    // counted samples to a fractional glare fade, not just a visible bit.
    DayNight::DNPlanet planet{}; planet.m_scale = 1; planet.m_color = CImVector(255,255,255,255);
    DayNight::DNSunGlare glare;
    glare.m_planet = &planet; glare.m_occlusionQuery = a; glare.m_occlusionPixels = 0;
    glare.m_position = DayNight::GetInfo()->m_cameraPos;
    const auto gameProjection = device.m_xforms[GxXform_Projection].m_mtx[device.m_xforms[GxXform_Projection].m_level];
    struct RestoreProjection {
        CGxDeviceMetal& device; C44Matrix value;
        ~RestoreProjection() { device.XformSetProjection(value); }
    } restoreProjection{device,gameProjection};
    device.XformSetProjection(C44Matrix(1));
    C44Matrix view(1); view.a0=view.b1=view.c2=0; view.a2=view.b0=view.c1=1;
    device.XformSetView(view); GxRsSet(GxRs_DepthTest,1); GxRsSet(GxRs_DepthFunc,3);
    for (int visible : {2,1,0}) {
        pass.depthAttachment.clearDepth = visible == 2 ? 1 : .25;
        device.BeginFrame(pass);
        if (visible == 1) {
            GxXformSetViewport(.5,1,0,1,0,1); device.SceneClear(2,CImVector());
            GxXformSetViewport(0,1,0,1,0,1);
        }
        glare.m_skipOcclusionOnce = 1;
        Require(glare.GetQueryOcclusion() == 0, "Original glare did not skip its first result");
        device.ScenePresent(); device.Drain(); answer(a,32*visible);
        device.BeginFrame(pass);
        Require(std::fabs(glare.GetQueryOcclusion() - visible*.5f) < .0001f,
            "Original glare lost its fully/partly/fully occluded fade");
        device.ScenePresent(); device.Drain();
    }
    // A live query left at device destruction must be released by its owner.
    CGxQuery* owned = nullptr; device.QueryCreate(owned,GxQueryType_Occlusion);
    std::fprintf(stderr,"Metal visibility queries: %u counts/lifetimes, depth/alpha rejection, overlapping/restarted brackets, clear/readback/frame splits, nonblocking polls, pooled storage and original planet glare fades passed\n",checks);
}
