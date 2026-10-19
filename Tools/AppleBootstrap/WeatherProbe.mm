#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    if (command == GxTex_Latch) { stride = 8; data = context; }
}
float Clamp(float x) { return std::clamp(x, 0.0f, 1.0f); }
CImVector Pack(simd_float4 c) { return CImVector(std::lround(Clamp(c.z)*255), std::lround(Clamp(c.y)*255), std::lround(Clamp(c.x)*255), std::lround(Clamp(c.w)*255)); }
struct WeatherPoint { C3Vector position, velocity; C2Vector lifetime; };
struct RainVertex { C3Vector position, velocity; CImVector corner; C2Vector lifetime; };
struct SplashVertex { C3Vector position; CImVector corner; C2Vector lifetime; };
struct Reference { C3Vector position; CImVector color; C2Vector uv; };
}

void CheckWeatherShaders(CGxDeviceMetal& device, bool wireframePointsOnly) {
    device.RsPush();
    const auto projection = device.m_projNative;
    for (auto transform : {GxXform_View, GxXform_World}) device.XformPushAndSet(transform, C44Matrix(1));
    for (unsigned i=0;i<4;++i) device.XformPushAndSet(EGxXform(i), C44Matrix(1));
    struct Cleanup {
        CGxDeviceMetal& device; C44Matrix projection; CGxShader* shaders[4]{}; CGxTex* texture{};
        int filled = device.MasterEnable(GxMasterEnable_PolygonFill);
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.MasterEnableSet(GxMasterEnable_PolygonFill,filled);
            device.RsPop(); device.TexDestroy(texture);
            for (auto& shader : shaders) device.ShaderDestroy(shader);
            for (unsigned i=0;i<4;++i) GxXformPop(EGxXform(i));
            GxXformPop(GxXform_World); GxXformPop(GxXform_View); device.m_projNative=projection;
        }
    } cleanup{device,projection};
    if (wireframePointsOnly) device.MasterEnableSet(GxMasterEnable_PolygonFill,0);
    const char* names[] = {"snowpoint", "sand", "rain", "patter"};
    for (unsigned i=0;i<(wireframePointsOnly?2u:4u);++i) {
        device.ShaderCreate(&cleanup.shaders[i],GxSh_Vertex,"Shaders\\Vertex",names[i],1);
        CGxShader* cached{}; device.ShaderCreate(&cached,GxSh_Vertex,"Shaders\\Vertex",names[i],1);
        Require(cached==cleanup.shaders[i] && cached->Valid(),"Weather shader cache failed"); device.ShaderDestroy(cached);
    }
    uint32_t texels[]={0xff204080,0xff80c040,0xffc040a0,0xff60a0e0};
    device.TexCreate(GxTex_2d,2,2,0,GxTex_Argb8888,GxTex_Argb8888,CGxTexFlags(GxTex_Nearest),texels,Upload,"Weather coordinate reference",cleanup.texture);
    for (auto state : {GxRs_Lighting,GxRs_Fog,GxRs_DepthTest,GxRs_DepthWrite,GxRs_Culling,GxRs_ScissorTest,GxRs_ClipPlaneMask,GxRs_PolygonOffset,GxRs_AlphaRef}) GxRsSet(state,0);
    GxRsSet(GxRs_ColorWrite,15); GxRsSet(GxRs_BlendingMode,GxBlend_Opaque); GxRsSet(GxRs_MatDiffuse,-1);
    device.RsSet(GxRs_PixelShader,static_cast<CGxShader*>(nullptr));
    for (unsigned i=0;i<4;++i) {
        device.RsSet(EGxRenderState(GxRs_Texture0+i),static_cast<CGxTex*>(nullptr));
        GxRsSet(EGxRenderState(GxRs_TextureCoord0+i),0); GxRsSet(EGxRenderState(GxRs_TextureShader0+i),0);
        GxRsSet(EGxRenderState(GxRs_TexGen0+i),0); GxRsSet(EGxRenderState(GxRs_ColorOp0+i),0); GxRsSet(EGxRenderState(GxRs_AlphaOp0+i),0);
    }
    GxXformSetViewport(0,1,0,1,0,1);
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:64 height:64 mipmapped:NO];
    descriptor.usage=MTLTextureUsageRenderTarget; descriptor.storageMode=MTLStorageModePrivate;
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=[device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction=MTLLoadActionClear; pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
    simd_float4 c[21]{};
    auto matrix=matrix_identity_float4x4; matrix.columns[0].x=matrix.columns[1].y=0.05f; matrix.columns[2].z=0.02f; matrix.columns[3].z=0.5f;
    const auto rows=simd_transpose(matrix);
    auto draw=[&](const void* data, EGxVertexBufferFormat format, unsigned count, EGxPrim primitive) {
        device.ShaderConstantsSet(GxSh_Vertex,0,reinterpret_cast<const float*>(c),21);
        device.BeginFrame(pass);
        auto* buffer=device.BufStream(GxPoolTarget_Vertex,GxVertexSize(format),count);
        std::memcpy(device.BufLock(buffer),data,count*GxVertexSize(format)); device.BufUnlock(buffer,0);
        GxPrimVertexPtr(buffer,format); CGxBatch batch; batch.m_primType=primitive; batch.m_count=count; batch.m_start=batch.m_minIndex=0; batch.m_maxIndex=count-1;
        device.Draw(&batch,0);
        TSGrowableArray<CImVector> pixels; CiRect rect(0,0,64,64); device.DeviceReadPixels(rect,pixels);
        std::vector<CImVector> result(pixels.Ptr(),pixels.Ptr()+pixels.Count()); device.ScenePresent(); return result;
    };
    unsigned sequences=0;
    for (unsigned kind=0;kind<2;++kind) for (float age : {0.5f,1.0f,1.1f,1.75f,2.9f,3.0f,3.125f,3.3f}) for (bool textured : {false,true}) for (float offset : {0.0f,20.5f,-20.5f}) {
        std::memset(c,0,sizeof(c));
        const unsigned camera=kind ? 3 : 1, projectionIndex=kind ? 4 : 2;
        c[camera]={100,-50,7,1}; std::memcpy(c+projectionIndex,&rows,sizeof(rows));
        c[kind ? 8 : 6]={age,0,0,0}; c[kind ? 9 : 7]={-0.02f,1,14,0}; c[kind ? 0 : 8]={0.25f,0.5f,0.75f,0.625f};
        WeatherPoint point{{100.4125f+offset,-50.2125f,9},{1.25f,-0.75f,-0.2f},{1,3}};
        device.RsSet(GxRs_VertexShader,cleanup.shaders[kind]); device.RsSet(GxRs_Texture0,textured ? cleanup.texture : nullptr);
        GxRsSet(GxRs_PointSprite,1); GxRsSet(GxRs_PointScaleMin,1.0f); GxRsSet(GxRs_PointScaleMax,textured ? 24.0f : 10.0f);
        const unsigned alphaRef=textured && age==1.75f ? 192 : 0; GxRsSet(GxRs_AlphaRef,int(alphaRef));
        const auto pixels=draw(&point,GxVBF_PNT,1,GxPrim_Points);
        const float t=kind ? std::clamp(age-1,0.0f,2.0f) : std::min(age,3.0f)-1;
        simd_float3 position={point.position.x+point.velocity.x*t-100,point.position.y+point.velocity.y*t+50,point.position.z+point.velocity.z*t-7};
        const float size=std::clamp(Clamp(1-0.02f*simd_length(position))*14,1.0f,textured ? 24.0f : 10.0f);
        const float x=32+1.6f*position.x,y=32-1.6f*position.y;
        float alpha=kind ? Clamp(5*t)*Clamp((2-t)/(2-(2-0.2f)))*0.625f : age>3 ? Clamp(1-4*(age-3)) : Clamp(t);
        unsigned covered=0;
        for (unsigned py=0;py<64;++py) for (unsigned px=0;px<64;++px) {
            const float dx=px-x,dy=py-y;
            if (std::abs(std::abs(dx)-size/2)<0.01f || std::abs(std::abs(dy)-size/2)<0.01f) continue;
            const bool inside=std::abs(dx)<size/2 && std::abs(dy)<size/2;
            simd_float4 expected={0,0,0,0};
            if (inside) {
                ++covered; expected={0.25f,0.5f,0.75f,alpha};
                if (textured) { const uint32_t p=texels[(dy>=0 ? 2:0)+(dx>=0 ? 1:0)]; expected*=simd_make_float4((p>>16)&255,(p>>8)&255,p&255,255)/255.0f; }
            }
            if (alphaRef && alpha<alphaRef/255.0f) expected={0,0,0,0};
            const auto want=Pack(expected),actual=pixels[py*64+px];
            if (std::abs(int(want.r)-actual.r)>1 || std::abs(int(want.g)-actual.g)>1 || std::abs(int(want.b)-actual.b)>1 || std::abs(int(want.a)-actual.a)>1)
                throw std::runtime_error(std::string(names[kind])+" lifetime/size/point coordinates mismatch at "+std::to_string(px)+","+std::to_string(py)+" age "+std::to_string(age)+" expected "+std::to_string(want.r)+","+std::to_string(want.g)+","+std::to_string(want.b)+","+std::to_string(want.a)+" got "+std::to_string(actual.r)+","+std::to_string(actual.g)+","+std::to_string(actual.b)+","+std::to_string(actual.a));
        }
        if (!offset) Require(covered>40,"Weather reference did not cover enough pixels"); ++sequences;
    }
    if (wireframePointsOnly) {
        std::fprintf(stderr,"Native wireframe: %u snow/sand point-sprite cases retained filled coverage, lifetime, texture coordinates, alpha and viewport-edge clipping\n",sequences);
        return;
    }
    // Compare original packed triangle input against independently constructed
    // camera-facing geometry. UVs remain unquantized; vertex colors allow one
    // UNORM8 rounding step in the reference path.
    GxRsSet(GxRs_AlphaRef,0); GxRsSet(GxRs_PointSprite,0); device.RsSet(GxRs_Texture0,cleanup.texture);
    matrix.columns[2].y=0.04f; const auto triangleRows=simd_transpose(matrix);
    for (unsigned kind=2;kind<4;++kind) for (float age : {0.5f,1.0f,1.125f,1.4f,2.75f,3.0f,3.5f}) for (bool fog : {false,true}) {
        std::memset(c,0,sizeof(c)); std::memcpy(c+2,&triangleRows,sizeof(triangleRows));
        c[0]=fog ? simd_make_float4(0,0.5f,2,0):simd_make_float4(0,1,1,0); c[1]={100,-50,7,1}; c[6]={age,0,0,0}; c[7]={0.25f,0.5f,0.75f,1};
        Reference reference[3]; RainVertex rain[3]; SplashVertex splash[3];
        const simd_float3 origin={102,-49,5},velocity={0,0,-2};
        for (unsigned i=0;i<3;++i) {
            rain[i]={{origin.x,origin.y,origin.z},{velocity.x,velocity.y,velocity.z},CImVector(0,0,0,i),{1,3}};
            splash[i]={{origin.x,origin.y,origin.z},CImVector(0,0,0,i),{1,3}};
        }
        if (kind==2) {
            c[8]={1,0,0,0}; c[9]={0,1,0,0}; c[10]={0,0,1,0}; c[11]={2,0,0,0};
            c[12]={0,1,0,0}; c[13]={1,1,0,0}; c[14]={0.5f,0,0,0};
            c[15]={-3,-3,-3,0}; c[16]={3,3,3,0}; c[17]={0,0,0,0}; c[20]={8,8,8,0};
            for (unsigned i=0;i<3;++i) {
                simd_float3 p=origin+velocity*(std::min(age,i==2 ? 4.0f:3.0f)-1);
                const auto side=simd_normalize(simd_cross(simd_make_float3(100,-50,7)-p,simd_make_float3(0,0,1)));
                p+=side*(i==0 ? -3.0f:i==1 ? 3.0f:0)+simd_make_float3(0,0,i==2 ? 8:0);
                const auto clip=simd_mul(matrix,simd_make_float4(p-simd_make_float3(100,-50,7),1));
                reference[i]={{clip.x,clip.y,clip.z},Pack(simd_make_float4(0.25f,0.5f,0.75f,float(age>1 && age<4))),{c[12+i].x,c[12+i].y}};
            }
        } else {
            c[8]={-3,0,0,0}; c[9]={0,5,0,0}; c[10]={3,0,0,0}; c[11]={0,0.25f,0,0}; c[12]={0.125f,0.04296875f,0,0}; c[13]={0.25f,0.25f,0,0};
            for (unsigned i=0;i<3;++i) {
                auto p=origin+simd_make_float3(c[8+i])-simd_make_float3(100,-50,7);
                const float f=std::max(0.0f,-p.z/simd_length(p));
                p-=simd_make_float3(c[9])*(f*0.5f); p.z+=f/12;
                const auto clip=simd_mul(matrix,simd_make_float4(p,1));
                reference[i]={{clip.x,clip.y,clip.z},Pack(simd_make_float4(0.25f,0.5f,0.75f,float(age>1 && age<3))),
                    {std::trunc((age-1)*15.96f)*0.25f+c[11+i].x,std::trunc((1-f)*3.99f)*0.25f+c[11+i].y}};
            }
        }
        GxRsSet(GxRs_Fog,int(fog)); GxRsSet(GxRs_FogColor,int(0xff406080u)); device.RsSet(GxRs_Texture0,fog ? nullptr : cleanup.texture);
        device.RsSet(GxRs_VertexShader,cleanup.shaders[kind]);
        const auto actual=draw(kind==2 ? static_cast<const void*>(rain):splash,kind==2 ? GxVBF_PNCT:GxVBF_PCT,3,GxPrim_Triangles);
        GxRsSet(GxRs_Fog,0);
        if (fog) for (auto& v : reference) {
            auto color=Pack(simd_make_float4(0.25f,0.5f,0.75f,0)*0.25f+simd_make_float4(64/255.0f,96/255.0f,128/255.0f,0)*0.75f);
            color.a=v.color.a; v.color=color;
        }
        device.RsSet(GxRs_VertexShader,static_cast<CGxShader*>(nullptr)); device.m_projNative=C44Matrix(1);
        const auto expected=draw(reference,GxVBF_PCT,3,GxPrim_Triangles);
        unsigned covered=0;
        for (unsigned i=0;i<actual.size();++i) {
            const auto a=actual[i],e=expected[i]; covered+=e.r!=0;
            if (std::abs(int(a.r)-e.r)>1 || std::abs(int(a.g)-e.g)>1 || std::abs(int(a.b)-e.b)>1 || std::abs(int(a.a)-e.a)>1)
                throw std::runtime_error(std::string(names[kind])+" triangle motion/atlas mismatch at "+std::to_string(i)+" age "+std::to_string(age));
        }
        if (covered<=8) throw std::runtime_error(std::string(names[kind])+" reference coverage "+std::to_string(covered)); ++sequences;
    }
    std::fprintf(stderr,"Native weather: %u snow/sand lifetime, point-size/viewport-edge/UV/alpha and rain/splash motion/atlas/fog sequences passed\n",sequences);
}
