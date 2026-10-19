#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

void CheckGxFFXPasses(CGxDeviceMetal&);
void CheckGxFFXEffect(CGxDeviceMetal&);
void CheckGxRenderTargets(CGxDeviceMetal&);
void CheckModelDepthOffset(CGxDeviceMetal&);
namespace {
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
}
void CheckMultisample(CGxDeviceMetal& device) {
    const auto original = device.m_format;
    const auto projection = device.m_projNative;
    device.RsPush(); device.m_projNative = C44Matrix(1);
    device.XformPushAndSet(GxXform_View, C44Matrix(1)); device.XformPushAndSet(GxXform_World, C44Matrix(1));
    struct Cleanup {
        CGxDeviceMetal& d; CGxFormat format; C44Matrix projection; CGxQuery* query = nullptr;
        ~Cleanup() {
            if (d.FrameCommand()) d.ScenePresent();
            d.QueryDestroy(query); d.RsPop(); d.DeviceSetFormat(format);
            GxXformPop(GxXform_World); GxXformPop(GxXform_View); d.m_projNative = projection;
        }
    } cleanup{device, original, projection};
    for (unsigned i=0;i<4;++i) GxRsSet(EGxRenderState(GxRs_Texture0+i), static_cast<CGxTex*>(nullptr));
    GxRsSet(GxRs_VertexShader, static_cast<CGxShader*>(nullptr)); GxRsSet(GxRs_PixelShader, static_cast<CGxShader*>(nullptr));
    for (auto state : {GxRs_Lighting,GxRs_Fog,GxRs_Culling,GxRs_ScissorTest,GxRs_ClipPlaneMask,GxRs_PolygonOffset,GxRs_PointSprite,GxRs_AlphaRef}) GxRsSet(state,0);
    GxRsSet(GxRs_DepthTest,1); GxRsSet(GxRs_DepthWrite,1); GxRsSet(GxRs_DepthFunc,0);
    GxRsSet(GxRs_ColorWrite,15); GxRsSet(GxRs_BlendingMode,GxBlend_Opaque);
    GxXformSetViewport(0,1,0,1,0,1);
    auto gpu = device.GPU().device;
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [gpu newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [gpu newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    if (device.Caps().m_occlusionQuery) device.QueryCreate(cleanup.query,GxQueryType_Occlusion);
    auto draw = [&](bool diagonal, CImVector color, bool sloped = false) {
        // Screen coordinates give an independent coverage/depth reference.
        C2Vector points[]{{-2,-2},{diagonal ? 18.125f : 40,-2},{-2,diagonal ? 18.125f : 40}};
        C3Vector positions[3];
        for (unsigned i=0;i<3;++i) positions[i] = {points[i].x/8-1,1-points[i].y/8,
            sloped ? .2f+.01f*points[i].x+.02f*points[i].y : .8f};
        GxPrimVertexPtr(3,positions,sizeof(C3Vector),nullptr,0,&color,0,nullptr,0,nullptr,0);
        CGxBatch batch{GxPrim_Triangles,0,3,0,2}; device.Draw(&batch,0);
    };
    auto read = [&] {
        CiRect rect(0,0,16,16); TSGrowableArray<CImVector> image; device.DeviceReadPixels(rect,image);
        std::vector<CImVector> result(image.Ptr(),image.Ptr()+image.Count()); return result;
    };
    unsigned checks = 0;
    for (uint32_t samples : {1u,2u,4u,8u,16u,4u,1u}) {
        auto format = original; format.m_multisampleCount = samples;
        const bool supported = [gpu supportsTextureSampleCount:samples] && (samples == 1 || gpu.areProgrammableSamplePositionsSupported);
        Require(bool(device.DeviceSetFormat(format)) == supported, "Native MSAA format capability mismatch");
        if (!supported) continue;
        std::vector<MTLSamplePosition> positions(samples, MTLSamplePositionMake(.5f,.5f));
        if (gpu.areProgrammableSamplePositionsSupported) [gpu getDefaultSamplePositions:positions.data() count:samples];
        for (bool enabled : {true,false}) for (float bias : {0.0f,.000024414435f,.03f}) {
            GxRsSet(GxRs_Multisample,enabled); GxRsSet(GxRs_DepthTest,1); GxRsSet(GxRs_DepthWrite,1);
            device.BeginFrame(pass);
            if (cleanup.query) device.QueryBegin(cleanup.query);
            GxRsSet(GxRs_PolygonOffset,bias);
            draw(true,CImVector(255,255,255,255),true);
            GxRsSet(GxRs_PolygonOffset,0.0f);
            if (cleanup.query) device.QueryEnd(cleanup.query);
            const auto first = read();
            CiRect rect(0,0,16,16); TSGrowableArray<float> depths; device.DeviceReadDepths(rect,depths);
            uint32_t total = 0;
            for (unsigned y=0;y<16;++y) for (unsigned x=0;x<16;++x) {
                unsigned covered = 0;
                for (auto p:positions) covered += x+y+(enabled?p.x+p.y-1:0) < 16.125f;
                total += covered;
                const auto pixel = first[y*16+x];
                const int expected = int(std::lround(255.0*covered/samples));
                if (std::abs(int(pixel.r)-expected)>1 || pixel.r!=pixel.g || pixel.r!=pixel.b || pixel.r!=pixel.a)
                    std::fprintf(stderr,"MSAA mismatch %ux enabled %d at %u,%u: BGRA %u,%u,%u,%u expected %d\n",samples,enabled,x,y,pixel.b,pixel.g,pixel.r,pixel.a,expected);
                Require(std::abs(int(pixel.r)-expected)<=1 && pixel.r==pixel.g && pixel.r==pixel.b && pixel.r==pixel.a,
                    "MSAA coverage resolve differs from native sample positions");
                const auto p = enabled ? positions[0] : MTLSamplePositionMake(.5f,.5f);
                const float depth = x+y+p.x+p.y-1 < 16.125f ? .2f+.01f*(x+p.x-.5f)+.02f*(y+p.y-.5f)-bias : 1;
                Require(std::abs(depths[y*16+x]-depth)<.000002f,"MSAA sample-zero depth resolve mismatch");
            }
            if (cleanup.query) {
                uint32_t count=0; Require(device.QueryGetData(cleanup.query,count) && count==total,"MSAA occlusion sample count mismatch");
            }
            // A pixel-center draw must test against every preserved depth sample,
            // even after the synchronous reads and a sample-pattern transition.
            GxRsSet(GxRs_Multisample,0); draw(false,CImVector(0,0,255,255));
            const auto second = read();
            for (unsigned i=0;i<256;++i) Require(second[i].r==255 && second[i].g==first[i].g && second[i].b==first[i].b && second[i].a==255,
                "MSAA samples/depth lost across readback or disabled-AA drawing");
            GxXformSetViewport(.25f,.75f,.25f,.75f,0,1); device.SceneClear(3,CImVector(0,255,0,255));
            GxXformSetViewport(0,1,0,1,0,1); const auto cleared=read();
            for (unsigned y=0;y<16;++y) for (unsigned x=0;x<16;++x) {
                CImVector expected = x>=4&&x<12&&y>=4&&y<12 ? CImVector(0,255,0,255) : second[y*16+x];
                Require(!std::memcmp(&cleared[y*16+x],&expected,4),"MSAA regional clear damaged preserved samples");
            }
            device.ScenePresent(); ++checks;
            // Retained attachments are also used by the host's capture frame.
            MTLRenderPassDescriptor* retained=[pass copy];
            retained.colorAttachments[0].loadAction=retained.depthAttachment.loadAction=MTLLoadActionLoad;
            device.BeginFrame(retained); const auto repeat=read(); device.ScenePresent();
            Require(!std::memcmp(repeat.data(),cleared.data(),repeat.size()*4),"MSAA contents lost between frames");
        }
        std::fprintf(stderr,"Metal samples %ux: coverage, sample-zero depth, disabled-AA, regional clear and frame preservation passed; counting queries %s\n",
            samples,cleanup.query ? "passed":"unavailable");
    }
    // Existing copy/resize/offscreen and real original FFX checks now run with
    // multisampled default attachments and single-sample intermediate targets.
    auto format=original; format.m_multisampleCount=4;
    if (device.DeviceSetFormat(format)) {
        GxRsSet(GxRs_Multisample,1);
        CheckModelDepthOffset(device); CheckGxRenderTargets(device); CheckGxFFXPasses(device); CheckGxFFXEffect(device);
        std::fprintf(stderr,"Metal MSAA: %u coverage/depth sequences plus original model/copy/target/FFX paths passed\n",checks);
    } else {
        std::fprintf(stderr,"Metal MSAA unavailable: programmable positions %d, native 4x textures %d; format rejection and %u single-sample sequences passed. Multisampled model/copy/FFX checks skipped.\n",
            gpu.areProgrammableSamplePositionsSupported,[gpu supportsTextureSampleCount:4],checks);
    }
}
