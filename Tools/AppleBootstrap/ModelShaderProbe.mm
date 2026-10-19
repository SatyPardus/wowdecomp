#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/ShaderEffect/ShaderEffectParser.h"
#include "WoW/Common/TerrainGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {
using Vec = std::array<float, 4>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Pattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    static const uint32_t texels[] = {0x00104488, 0x0060A0E0, 0x00C08040, 0x00FFFFFF};
    static const uint32_t modTexels[] = {0xFF104488, 0x8060A0E0, 0x40C08040, 0x00FFFFFF};
    if (command == GxTex_Latch) { stride = 8; data = context ? modTexels : texels; }
}
void EnvironmentPattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texels[] = {0x00102030, 0xFF183028, 0x00183010, 0xFF301820};
    if (command == GxTex_Latch) { stride = 8; data = texels; }
}
void LayerAlphaPattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texel = 0x804080C0;
    if (command == GxTex_Latch) { stride = 4; data = &texel; }
}
Vec LayerReference(float u, float v) {
    u = std::clamp(u * 2 - 0.5f, 0.0f, 1.0f); v = std::clamp(v * 2 - 0.5f, 0.0f, 1.0f);
    const Vec texels[] = {{16,32,48,0}, {24,48,40,255}, {24,48,16,0}, {48,24,32,255}};
    Vec result{};
    for (unsigned i = 0; i < 4; ++i)
        result[i] = ((texels[0][i]*(1-u) + texels[1][i]*u)*(1-v) + (texels[2][i]*(1-u) + texels[3][i]*u)*v) / 255;
    return result;
}
Vec EnvironmentCoordinates(const Vec& view, const Vec& normal) {
    const float viewLength = std::sqrt(view[0]*view[0] + view[1]*view[1] + view[2]*view[2]);
    Vec incident{}, reflected{};
    float projection = 0;
    for (unsigned i = 0; i < 3; ++i) { incident[i] = -view[i] / viewLength; projection += incident[i] * normal[i]; }
    for (unsigned i = 0; i < 3; ++i) reflected[i] = incident[i] - 2 * projection * normal[i];
    const float sphereLength = std::sqrt(reflected[0]*reflected[0] + reflected[1]*reflected[1] + (reflected[2]+1)*(reflected[2]+1));
    return {reflected[0] / sphereLength * 0.5f + 0.5f, reflected[1] / sphereLength * 0.5f + 0.5f, 0, 1};
}
Vec EnvironmentReference(const Vec& view, const Vec& normal) {
    const auto uv = EnvironmentCoordinates(view, normal);
    return LayerReference(uv[0], uv[1]);
}
float Dot(const Vec& a, const Vec& b, unsigned count = 4) {
    float result = 0; for (unsigned i = 0; i < count; ++i) result += a[i] * b[i]; return result;
}
float Sat(float value) { return std::clamp(value, 0.0f, 1.0f); }
float ShadowDepth(unsigned x,unsigned y,unsigned band=0) { return float(64*(1+(x+2*y+band)%3))/255; }
void ShadowPattern(EGxTexCommand command,uint32_t,uint32_t,uint32_t,uint32_t,void* context,uint32_t& stride,const void*& data) {
    static std::array<std::array<uint32_t,25>,4> values=[] {
        std::array<std::array<uint32_t,25>,4> v;
        for(unsigned band=0;band<4;++band) for(unsigned y=0;y<5;++y) for(unsigned x=0;x<5;++x)
            v[band][y*5+x]=0xFF000000u | ((64*(1+(x+2*y+band)%3))<<16);
        return v;
    }();
    if (command == GxTex_Latch) { stride=5*sizeof(uint32_t); data=values[context ? *static_cast<const unsigned*>(context):0].data(); }
}
void CreateComparisonDepths(CGxDeviceMetal& device, MTLRenderPassDescriptor* pass, CGxTex* material, CGxTex** depths) {
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader* vertex[2]{}; CGxShader* pixel[1]{}; CGxTex* color=nullptr;
        ~Cleanup() {
            device.DeviceSetRenderTarget(GxBuffers_Depth,nullptr,0); device.DeviceSetRenderTarget(GxBuffers_Color,nullptr,0);
            if(device.FrameCommand()) device.ScenePresent(); device.RsPop();
            for(auto* shader:vertex) device.ShaderDestroy(shader);
            for(auto* shader:pixel) device.ShaderDestroy(shader);
            if(color) device.TexDestroy(color);
        }
    } cleanup{device};
    device.ShaderCreate(cleanup.vertex,GxSh_Vertex,"Shaders\\Vertex","UI",2);
    device.ShaderCreate(cleanup.pixel,GxSh_Pixel,"Shaders\\Pixel","UI",1);
    GxRsSet(GxRs_VertexShader,cleanup.vertex[0]); GxRsSet(GxRs_PixelShader,cleanup.pixel[0]); GxRsSet(GxRs_Texture0,material);
    GxRsSet(GxRs_DepthTest,0); GxRsSet(GxRs_DepthWrite,1); GxRsSet(GxRs_ColorWrite,0); GxRsSet(GxRs_AlphaRef,0);
    GxRsSet(GxRs_ScissorTest,0); GxRsSet(GxRs_ClipPlaneMask,0); GxRsSet(GxRs_Culling,0); GxRsSet(GxRs_PolygonOffset,0);
    GxRsSet(GxRs_Fog,0);
    CGxTexFlags flags(GxTex_Linear); flags.m_renderTarget=1;
    device.TexCreate(GxTex_NonPow2,5,5,0,GxTex_Argb8888,GxTex_Argb8888,flags,nullptr,nullptr,"PCF color attachment",cleanup.color);
    C44Matrix identity(1); GxShaderConstantsSet(GxSh_Vertex,0,&identity.a0,4);
    device.BeginFrame(pass); device.DeviceSetRenderTarget(GxBuffers_Color,cleanup.color,0);
    for(unsigned band=0;band<4;++band) {
        device.TexCreate(GxTex_NonPow2,5,5,0,GxTex_D24X8,GxTex_D24X8,flags,nullptr,nullptr,"PCF depth comparison reference",depths[band]);
        device.DeviceSetRenderTarget(GxBuffers_Depth,depths[band],0);
        GxXformSetViewport(0,1,0,1,0,1); device.SceneClear(3,CImVector(0,0,0,0));
        struct Vertex { float x,y,z; uint32_t color; float u,v; };
        for(unsigned y=0;y<5;++y) for(unsigned x=0;x<5;++x) {
            const float z=ShadowDepth(x,y,band);
            const Vertex data[]={{-1,-1,z,0xFFFFFFFF,0,0},{-1,1,z,0xFFFFFFFF,0,1},{1,-1,z,0xFFFFFFFF,1,0},{1,1,z,0xFFFFFFFF,1,1}};
            auto* buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(Vertex),4);
            std::memcpy(device.BufLock(buffer),data,sizeof(data)); device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,GxVBF_PCT);
            GxXformSetViewport(float(x)/5,float(x+1)/5,float(y)/5,float(y+1)/5,0,1);
            CGxBatch batch{GxPrim_TriangleStrip,0,4,0,3}; GxDraw(&batch,0);
        }
        GxXformSetViewport(0,1,0,1,0,1);
        CiRect rect(0,0,5,5); TSGrowableArray<float> values; device.DeviceReadDepths(rect,values);
        for(unsigned y=0;y<5;++y) for(unsigned x=0;x<5;++x)
            Require(values[y*5+x]==ShadowDepth(x,y,band),"PCF fixture depth rasterization mismatch");
    }
}
void CheckModelShadows(CGxDeviceMetal& device,CGxShader** vertex,CGxShader** opaque,CGxShader** mod,CGxShader** environmentVertex,CGxShader** environmentPixel,
    CGxShader** dualUVVertex,CGxShader** reflectionVertex,CGxShader* (*layerPixel)[16],
    MTLRenderPassDescriptor* pass,CGxTex* material,CGxTex* environment,CGxTex* const* shadows,bool projectedOnly=false,
    GxMetal::ModelVertexEffect vertexEffect=GxMetal::ModelVertexEffect::Diffuse, bool comparison=false) {
    const bool coordinateFamily=vertexEffect >= GxMetal::ModelVertexEffect::DiffuseSecondUV;
    const unsigned coordinateKind=unsigned(vertexEffect);
    const bool colored=!GxMetal::UsesModelMaterial(vertexEffect);
    const bool colorDiffuse=vertexEffect==GxMetal::ModelVertexEffect::ColorDiffuse;
    const Vec vertexTint{64.0f/255,128.0f/255,192.0f/255,128.0f/255};
    GxRsSet(GxRs_Texture0,material); GxRsSet(GxRs_Texture1,environment); GxRsSet(GxRs_AlphaRef,0);
    for(unsigned i=0;i<4;++i) GxRsSet(EGxRenderState(GxRs_Texture4+i),shadows[i]);
    std::array<Vec,256> c{}; std::array<Vec,13> p{};
    c[2]={1,0,0,0}; c[3]={0,1,0,0}; c[4]={0,0,0,.5}; c[5]={0,0,0,1};
    c[6]={1,0,0,0}; c[7]={0,1,0,0}; c[10]={.4,.5,.6,0}; c[11]={.1,.1,.1,0}; c[12]={0,0,-1,0};
    c[8]={.5,.25,.9,.125}; c[9]={.25,.5,.7,.1875};
    if(projectedOnly) { c[6]={4,0,.02f,.2f}; c[7]={0,4,-.01f,.3f}; }
    c[25]={1,1,1,1}; c[28]={.6,.7,.8,.5}; c[29]={.05,.1,.15,0}; c[30]={0,.6,1,0};
    for(unsigned i=0;i<4;++i) c[21+i]={-2,-1,-3,0};
    if(projectedOnly || colored || coordinateFamily) {
        c[25]={.7,.9,1.1,1.3}; c[26]={.1,.2,.3,.4}; c[27]={.03,.05,.07,.09};
        for(unsigned i=0;i<4;++i) {
            c[17+i]={.03f*(i+1),.02f*(4-i),.015f*(i+1),0};
            c[21+i]={-.6f+.3f*i,.7f-.2f*i,1.2f+.1f*i,0};
        }
    }
    if(colored) c[29][3]=.125f;
    p[2]={.12,.23,.34,0};
    const Vec offsets[]={{-.2,0,0,0},{-.2,-.2,0,0},{0,-.2,0,0},{.2,-.2,0,0},
        {.2,0,0,0},{.2,.2,0,0},{0,.2,0,0},{-.2,.2,0,0}};
    std::copy_n(offsets,8,p.begin()+5);
    M2Vertex source{}; source.m_position={.03125f,.03125f,0}; source.m_normal={0,0,1}; source.m_texcoord[0]={.5,.5}; source.m_texcoord[1]={.25,.5};
    if(projectedOnly || colored || coordinateFamily) source.m_normal={.3f,.4f,.5f};
    // Rotate the model so a wrongly transformed fixed +Z normal is observable.
    Vec lightingNormal=colored ? Vec{.54f,.4f,.22f,0}:(projectedOnly || coordinateFamily) ? Vec{.3f,.4f,.5f,0}:Vec{0,0,1,0};
    if(projectedOnly || colored || coordinateFamily) for(unsigned i=0;i<3;++i) lightingNormal[i]/=std::sqrt(.5f);
    const float normalZ=colored && !colorDiffuse ? 1:lightingNormal[2];
    const uint8_t weights[]={32,64,96,63}, indices[]={4,2,1,3};
    std::memcpy(&source.m_weights,weights,4); std::memcpy(&source.m_indices,indices,4);
    auto* vertices=GxBufStream(GxPoolTarget_Vertex,sizeof(source),1);
    std::memcpy(device.BufLock(vertices),&source,sizeof(source)); device.BufUnlock(vertices,0); GxPrimVertexPtr(vertices,GxVBF_PBNT2);
    CGxBatch batch{GxPrim_Points,0,1,0,0}; unsigned checks=0;
    struct Case {
        float z,x,y,depth,distance,normal;
        std::array<Vec,3> bands{Vec{2,0,0,0},Vec{2,0,0,0},Vec{2,0,0,0}};
        bool affine = false;
    };
    std::vector<Case> cases={{.5,0,0,128.0f/255,0,1},{9.99,0,0,128.0f/255,0,1},{10,0,0,128.0f/255,0,1},{10.01,0,0,128.0f/255,0,1},
        {.5,0,0,std::nextafter(128.0f/255,1.0f),0,1},{.5,0,0,0,0,1},{.5,0,0,1,0,1},
        {.5,.9,0,1,0,1},{.5,.98,0,1,0,1},{.5,1,0,1,0,1},{.5,0,-.98,1,0,1},
        {.5,0,0,1,.4,1},{.5,0,0,1,1,1},{.5,0,0,1,0,.7},{.5,0,0,1,0,0}};
    const size_t dynamicCases=cases.size();
    // Distinct maps reveal slot/coordinate swaps; exact +/-1 selects the next band.
    for(unsigned selected=0;selected<3;++selected) for(float edge:{0.0f,std::nextafter(1.0f,0.0f),1.0f,-1.0f}) {
        Case test{.5,0,0,0,0,1}; test.bands[selected]={edge,0,128.0f/255,0};
        if(selected<2) test.bands[selected+1]={0,0,1,0};
        cases.push_back(test);
    }
    Case farFade{.5,0,0,0,0,1}; farFade.bands[2]={0,-.95,1,0}; cases.push_back(farFade);
    Case minOrder{10,0,0,1,1,.7}; minOrder.bands[0]={.1,.2,128.0f/255,0}; minOrder.affine=true; cases.push_back(minOrder);
    for(unsigned mode=colored ? 0:1;mode<=3;++mode) for(unsigned permutation=0;permutation<(colored ? 10u:30u);++permutation)
        for(size_t caseIndex=0;caseIndex<(!mode ? 1:mode==1 ? dynamicCases:cases.size());++caseIndex) {
        const auto& test=cases[caseIndex];
        if(colored && !caseIndex) {
            const auto format=projectedOnly ? ((permutation&1) ? GxVBF_P:GxVBF_PC):
                colorDiffuse ? ((mode || (permutation&1)) ? GxVBF_PNCT:GxVBF_PCT):
                (permutation&1) ? GxVBF_PT:GxVBF_PCT;
            auto* buffer=GxBufStream(GxPoolTarget_Vertex,GxVertexSize(format),1);
            auto* bytes=reinterpret_cast<uint8_t*>(device.BufLock(buffer));
            std::memcpy(bytes+GxVertexAttribOffset(format,GxVA_Position),&source.m_position,12);
            if(GxVertexHasAttrib(format,GxVA_Normal)) std::memcpy(bytes+GxVertexAttribOffset(format,GxVA_Normal),&source.m_normal,12);
            if(GxVertexHasAttrib(format,GxVA_TexCoord0)) std::memcpy(bytes+GxVertexAttribOffset(format,GxVA_TexCoord0),&source.m_texcoord[0],8);
            const uint32_t packedColor=0x804080C0;
            if(GxVertexHasAttrib(format,GxVA_Color0)) std::memcpy(bytes+GxVertexAttribOffset(format,GxVA_Color0),&packedColor,4);
            device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,format);
        } else if((projectedOnly || coordinateKind==9) && !caseIndex) {
            if(permutation<10) {
                const CGxVertexPN value{source.m_position,source.m_normal};
                auto* buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(value),1);
                std::memcpy(device.BufLock(buffer),&value,sizeof(value)); device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,GxVBF_PN);
            } else {
                auto* buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(source),1);
                std::memcpy(device.BufLock(buffer),&source,sizeof(source)); device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,GxVBF_PBNT2);
            }
        }
        for(unsigned i=0;i<5;++i) {
            c[31+i*3]={1,0,0,.02f*i}; c[32+i*3]={0,1,0,-.03f*i}; c[33+i*3]={0,0,1,test.z};
        }
        if(colored) { c[31]={.8,0,.6,0}; c[32]={0,1,0,0}; c[33]={-.6,0,.8,test.z+.6f*source.m_position.x}; }
        float bone=0;
        if(permutation>=20) for(unsigned i=0;i<4;++i) bone+=indices[i]*(weights[i]/255.0f);
        else if(permutation>=10) bone=indices[0];
        const Vec view{colored ? .8f*.03125f:.03125f+bone*.02f,.03125f+bone*-.03f,test.z,1};
        c[224]={.2,.3,0,test.x}; c[225]={-.1,.2,0,test.y}; c[226]={0,0,0,test.depth};
        for(unsigned band=0;band<3;++band) {
            c[227+3*band]={test.affine ? .1f*(band+1):0,0,0,test.bands[band][0]};
            c[228+3*band]={0,test.affine ? -.2f*(band+1):0,0,test.bands[band][1]};
            c[229+3*band]={0,0,test.affine ? .01f*(band+1):0,test.bands[band][2]};
        }
        p[3]={.1,.2,0,test.distance}; p[4]={0,0,test.normal,0};
        const float sx=Dot(c[224],view),sy=Dot(c[225],view);
        const float border=Sat(std::max(std::abs(sx),std::abs(sy))*(-3.4482758045196533f)+3.4137930870056152f);
        auto sample=[&](const Vec& coordinate,unsigned band,float dx,float dy) {
            const float x=(coordinate[0]*.5f+.5f+dx)*5, y=(coordinate[1]*.5f+.5f+dy)*5;
            auto compare=[&](int px,int py) { return ShadowDepth(unsigned(std::clamp(px,0,4)),unsigned(std::clamp(py,0,4)),band)>=coordinate[2] ? 1.0f:0.0f; };
            if(!comparison) return compare(int(std::floor(x)),int(std::floor(y)));
            // Independently compare four texels, then bilinearly interpolate
            // comparison results; this distinguishes PCF from filtering depth.
            const int left=int(std::floor(x-.5f)), top=int(std::floor(y-.5f));
            const float fx=x-.5f-left, fy=y-.5f-top;
            return (compare(left,top)*(1-fx)+compare(left+1,top)*fx)*(1-fy)+
                (compare(left,top+1)*(1-fx)+compare(left+1,top+1)*fx)*fy;
        };
        auto average=[&](const Vec& coordinate,unsigned band,bool nine) {
            float count=sample(coordinate,band,0,0);
            for(unsigned i=0;i<8;i+=nine ? 1:2) count+=sample(coordinate,band,offsets[i][0],offsets[i][1]);
            return count/(nine ? 9:5);
        };
        float visibility=1;
        if(border>(mode==3 ? 0:.01f)) {
            const float count=average({sx,sy,test.depth,0},0,mode!=3 && test.z>=10);
            visibility=mode==3 ? count:std::min(1.0f,1+border*(count-1));
        }
        const float distance=Sat(Dot(p[3],view)); visibility+=distance*(1-visibility);
        if(mode>=2) {
            for(unsigned band=0;band<3;++band) {
                Vec coordinate{Dot(c[227+3*band],view),Dot(c[228+3*band],view),Dot(c[229+3*band],view),0};
                const float edge=std::max(std::abs(coordinate[0]),std::abs(coordinate[1]));
                if(edge>=1 && band<2) continue;
                float bandValue=average(coordinate,band+1,false);
                if(band==2) bandValue=1+Sat(edge*(-11.11111068725586f)+11)*(bandValue-1);
                visibility=std::min(visibility,bandValue); break;
            }
        }
        const float normalFade=Sat(std::pow(1.2f-std::abs(test.normal*normalZ),4)); visibility+=normalFade*(1-visibility);
        const float brightness=mode ? .7f+.3f*visibility:1;
        for(unsigned materialIndex=0;materialIndex<25;++materialIndex) for(unsigned alphaCase=0;alphaCase<3;++alphaCase) {
            if(comparison && materialIndex>1) continue;
            if(coordinateFamily && materialIndex > (coordinateKind==8 ? 1u:3u)) continue;
            if(projectedOnly && materialIndex!=3 && materialIndex!=9) continue;
            if(colored && !projectedOnly && materialIndex>1) continue;
            // Remaining fragment operators share the already exercised receiver
            // and UV transforms; cover lit/unlit rigid, one and four weights.
            if(!colored && ((materialIndex>=7 && materialIndex<=12) || materialIndex>=15) && permutation%10>1) continue;
            const bool reflectionOnly=materialIndex==13 || materialIndex==14;
            const bool reflectedLayer=materialIndex==2 || ((materialIndex==7 || materialIndex==8) && alphaCase==0);
            const Vec secondUV{source.m_texcoord[1].x,source.m_texcoord[1].y,0,1};
            const auto reflectedUV=EnvironmentCoordinates(view,coordinateFamily ? lightingNormal:Vec{0,0,1,0});
            const auto reflection=LayerReference(reflectedUV[0],reflectedUV[1]);
            const Vec firstUV{source.m_texcoord[0].x,source.m_texcoord[0].y,0,1};
            const auto layer=coordinateKind==9 ? reflection : coordinateKind==10 ? LayerReference(Dot(c[6],firstUV),Dot(c[7],firstUV)) : projectedOnly ? LayerReference(Dot(c[8],view),Dot(c[9],view)) :
                reflectedLayer ? reflection : LayerReference(Dot(c[8],secondUV),Dot(c[9],secondUV));
            Vec baseTexel=reflectionOnly && !alphaCase ? reflection : Vec{64.0f/255,128.0f/255,192.0f/255,128.0f/255};
            if(projectedOnly || coordinateFamily) {
                const Vec texels[]={{16,68,136,255},{96,160,224,128},{192,128,64,64},{255,255,255,0}};
                const float u=coordinateKind==8 ? Dot(c[8],secondUV):coordinateFamily ? reflectedUV[0]:Dot(c[6],view);
                const float v=coordinateKind==8 ? Dot(c[9],secondUV):coordinateFamily ? reflectedUV[1]:Dot(c[7],view);
                baseTexel=texels[unsigned(u>=.5f)+2*unsigned(v>=.5f)];
                for(auto& value:baseTexel) value/=255;
            }
            const bool textureAlpha=materialIndex==1 || materialIndex==3 || materialIndex==6 || materialIndex==8 || materialIndex==9 || materialIndex==14 || materialIndex==15 || materialIndex==18;
            // Binary-exact UV/matrix values give layer alpha 1/4. Equality
            // cases therefore isolate the shader's MUL/MAD/DP2ADD ordering.
            const float vertexAlpha=colored ? (colorDiffuse ? Sat(vertexTint[3]+((permutation&1) ? c[29][3]:0)):
                (permutation&1) ? 1:vertexTint[3]):.5f;
            float alpha=vertexAlpha*(textureAlpha ? baseTexel[3] : 1);
            if(materialIndex==3 || materialIndex==17) alpha*=layer[3];
            if(materialIndex==4 || materialIndex==6) alpha*=2*layer[3];
            if(materialIndex==9 || materialIndex==10) alpha+=layer[3];
            if(materialIndex==21) alpha=baseTexel[3]+vertexAlpha;
            if(materialIndex==22) alpha=2*baseTexel[3]*vertexAlpha;
            if(materialIndex==23) alpha=(baseTexel[3]+vertexAlpha)*layer[3];
            if(materialIndex==24) alpha=4*((baseTexel[3]*vertexAlpha)*layer[3]);
            p[2][3]=alphaCase==2 ? std::nextafter(alpha,alpha+1) : alpha;
            // Projected UVs move across a filtered texture. Keep this test
            // away from rounding ties; exact alpha equality is covered above.
            if(projectedOnly || coordinateFamily) p[2][3]=alpha+(alphaCase==2 ? .01f : -.01f);
            const bool visible=alphaCase!=2;
            device.BeginFrame(pass); GxShaderConstantsSet(GxSh_Vertex,0,c[0].data(),256); GxShaderConstantsSet(GxSh_Pixel,0,p[0].data(),13);
            GxRsSet(GxRs_Texture0,reflectionOnly && !alphaCase ? environment:material);
            CGxShader** pixelFamilies[]={opaque,mod,environmentPixel,layerPixel[2],layerPixel[4],layerPixel[3],layerPixel[6],
                layerPixel[0],layerPixel[1],layerPixel[5],layerPixel[7],layerPixel[9],layerPixel[10],opaque,mod,layerPixel[11],layerPixel[12],layerPixel[13],layerPixel[14],layerPixel[15],layerPixel[16],layerPixel[17],layerPixel[18],layerPixel[19],layerPixel[20]};
            CGxShader** vertexFamily=(projectedOnly || coordinateFamily) ? vertex:reflectionOnly ? reflectionVertex:reflectedLayer ? environmentVertex:materialIndex>=3 ? dualUVVertex:vertex;
            GxRsSet(GxRs_VertexShader,vertexFamily[30*std::min(mode,2u)+permutation]);
            GxRsSet(GxRs_PixelShader,pixelFamilies[materialIndex][mode+(comparison ? 4:0)+(alphaCase ? 8:0)]);
            GxDraw(&batch,0); CiRect rect(0,0,32,32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect,pixels);
            const unsigned x=unsigned(std::floor((view[0]+1)*16)),y=unsigned(std::floor((1-view[1])*16));
            const auto& pixel=pixels[y*32+x]; const unsigned channels[]={pixel.r,pixel.g,pixel.b,pixel.a};
            for(unsigned channel=0;channel<4;++channel) {
                float expected=alpha;
                if(channel<3) {
                    float lighting=c[10][channel]*normalZ+c[11][channel];
                    if(projectedOnly || colored || coordinateFamily) for(unsigned light=0;light<(permutation/2)%5;++light) {
                        const Vec ray{c[21+light][0]-view[0],c[21+light][1]-view[1],c[21+light][2]-view[2],0};
                        const float squared=Dot(ray,ray,3),distance=std::sqrt(squared);
                        const float diffuse=std::max(Dot(ray,lightingNormal,3)/distance,0.0f);
                        lighting+=c[17+light][channel]*diffuse/(c[25][light]+distance*c[26][light]+squared*c[27][light]);
                    }
                    const float lit=colored ? (colorDiffuse ? ((permutation&1) ? Sat(vertexTint[channel]*Sat(lighting)+c[29][channel]):vertexTint[channel]):
                        (permutation&1) ? 1:vertexTint[channel]):Sat(c[28][channel]*((permutation&1) ? Sat(lighting):1)+c[29][channel]);
                    const float texel=baseTexel[channel];
                    float base=texel;
                    if(materialIndex==3 || materialIndex==15 || materialIndex==16 || materialIndex==17) base*=layer[channel];
                    if(materialIndex==4 || materialIndex==6 || materialIndex==12 || materialIndex==18) base*=2*layer[channel];
                    if(materialIndex==5) base*=128.0f/255 + 2*layer[channel]*(1-128.0f/255);
                    if(materialIndex==11) base+=(1-baseTexel[3])*layer[channel]*layer[3];
                    const float added=materialIndex==2 ? layer[channel]*layer[3] : materialIndex>=7 && materialIndex<=10 ? layer[channel] : 0;
                    const float surface = materialIndex==19 ? vertexAlpha*lit*brightness+(1-vertexAlpha)*texel :
                        materialIndex==20 ? vertexAlpha*texel+(1-vertexAlpha)*lit*brightness :
                        materialIndex==21 ? texel+lit*brightness : materialIndex==22 ? 2*texel*lit*brightness :
                        materialIndex==23 ? (texel+lit*brightness)*layer[channel] :
                        materialIndex==24 ? 4*texel*layer[channel]*lit*brightness : base*lit*brightness+added;
                    expected=.6f*(surface-p[2][channel])+p[2][channel];
                }
                if(!visible) expected=0;
                expected=Sat(expected); // BGRA8 attachment conversion, after shader alpha rejection.
                if(std::abs(int(channels[channel])-int(std::lround(expected*255)))>2)
                    throw std::runtime_error("Model shadow mismatch: family "+std::string(GxMetal::ModelVertexFamily(vertexEffect))+" mode "+std::to_string(mode)+" material "+std::to_string(materialIndex)+" VS "+std::to_string(30*std::min(mode,2u)+permutation)+" case "+std::to_string(checks)+" channel "+std::to_string(channel)+" got "+std::to_string(channels[channel])+" expected "+std::to_string(expected*255));
            }
            device.ScenePresent(); ++checks;
        }
    }
    if(comparison) {
        std::fprintf(stderr,"Native PCF model shadows: %u independent outputs, original opaque/modulated receiver variants, comparison-before-filtering, five/nine taps, bands, fades and alpha boundaries passed\n",checks);
        return;
    }
    if(coordinateFamily) {
        std::fprintf(stderr,"%s: %u independent shadow/UV outputs, rigid/one/four weights, local lights, original coordinate declarations, R32F shadows and alpha/fog passed\n",GxMetal::ModelVertexFamily(vertexEffect),checks);
        return;
    }
    if(colored) {
        std::fprintf(stderr,"%s shadows: %u independent outputs, 20 shadow and 10 unshadowed rigid/light variants, rotated model, original fixed/authored normals, BGRA/white/lighting, three R32F modes and alpha/fog passed\n",GxMetal::ModelVertexFamily(vertexEffect),checks);
        return;
    }
    if(projectedOnly) {
        std::fprintf(stderr,"Projected model shadows: %u independent outputs, 60 rigid/skinned/light variants, two camera-space texture projections, R32F dynamic/banded shadows, RGB/alpha and fog passed\n",checks);
        return;
    }
    std::fprintf(stderr,"Model shadows: %u independent outputs, 240 diffuse/environment/dual-UV/reflection rigid/skinned/light variants, independent reflection and layered RGB/alpha, all R32F shadow modes, five/nine taps, band selection, depth equality, border/distance/normal fades and shader alpha boundaries passed\n",checks);
}
}

void CheckModelShaders(CGxDeviceMetal& device,bool projectedShadowsOnly,bool coloredShadowsOnly) {
    // Truncated scopes/arguments and over-capacity pass lists used to read or
    // write beyond the parser's stack records before any shader was created.
    for (std::string malformed : {std::string("Effect(Broken"), std::string("Effect(Test){Shader(){Pass(){VertexShader(" + std::string(80, 'x') + ");}}}"),
        std::string("Effect(Test){Shader(){Pass(){VertexShader(Color_T1);PixelShader(Combiners_Mod);}Pass(){}}}"),
        std::string("Effect(Test){FixedFunc(){Pass(3){ColorOp0(Mod);}}}")}) {
        bool rejected = false;
        try { CShaderEffectParser::ParseEffectFile(malformed.data(), uint32_t(malformed.size()), [](const auto&, void*) {}, nullptr); }
        catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Malformed shader effect escaped its bounds checks: " + malformed);
    }
    CGxShader* vertex[unsigned(GxMetal::ModelVertexEffect::Count)][90]{}; CGxShader* pixel[16]{}; CGxShader* modPixel[16]{}; CGxShader* layerPixel[21][16]{};
    for (unsigned family = 0; family < unsigned(GxMetal::ModelVertexEffect::Count); ++family)
        device.ShaderCreate(vertex[family], GxSh_Vertex, "Shaders\\Vertex", GxMetal::ModelVertexFamily(GxMetal::ModelVertexEffect(family)), 90);
    device.ShaderCreate(pixel, GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque", 16);
    device.ShaderCreate(modPixel, GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod", 16);
    device.ShaderCreate(layerPixel[0], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_AddNA", 16);
    device.ShaderCreate(layerPixel[1], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_AddNA", 16);
    device.ShaderCreate(layerPixel[2], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_Mod", 16);
    device.ShaderCreate(layerPixel[3], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Mod2xNA_Alpha", 16);
    device.ShaderCreate(layerPixel[4], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Mod2x", 16);
    device.ShaderCreate(layerPixel[5], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_Add", 16);
    device.ShaderCreate(layerPixel[6], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_Mod2x", 16);
    device.ShaderCreate(layerPixel[7], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Add", 16);
    device.ShaderCreate(layerPixel[8], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_AddAlpha", 16);
    device.ShaderCreate(layerPixel[9], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_AddAlpha_Alpha", 16);
    device.ShaderCreate(layerPixel[10], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Mod2xNA", 16);
    device.ShaderCreate(layerPixel[11], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_Opaque", 16);
    device.ShaderCreate(layerPixel[12], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Opaque", 16);
    device.ShaderCreate(layerPixel[13], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Opaque_Mod", 16);
    device.ShaderCreate(layerPixel[14], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod_Mod2xNA", 16);
    device.ShaderCreate(layerPixel[15], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Decal", 16);
    device.ShaderCreate(layerPixel[16], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Fade", 16);
    device.ShaderCreate(layerPixel[17], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Add", 16);
    device.ShaderCreate(layerPixel[18], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod2x", 16);
    device.ShaderCreate(layerPixel[19], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Add_Mod", 16);
    device.ShaderCreate(layerPixel[20], GxSh_Pixel, "Shaders\\Pixel", "Combiners_Mod2x_Mod2x", 16);
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader* (*vertex)[90]; CGxShader** pixel; CGxShader** modPixel; CGxShader* (*layerPixel)[16]; CGxTex* texture[12]{};
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop(); for (auto& image : texture) device.TexDestroy(image);
            for (unsigned family = 0; family < unsigned(GxMetal::ModelVertexEffect::Count); ++family) for (unsigned i = 0; i < 90; ++i) device.ShaderDestroy(vertex[family][i]);
            for (unsigned i = 0; i < 16; ++i) {
                device.ShaderDestroy(pixel[i]); device.ShaderDestroy(modPixel[i]);
                for (unsigned layer = 0; layer < 21; ++layer) device.ShaderDestroy(layerPixel[layer][i]);
            }
        }
    } cleanup{device, vertex, pixel, modPixel, layerPixel};
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, Pattern, "Model material reference", cleanup.texture[0]);
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), &cleanup, Pattern, "Modulated model material reference", cleanup.texture[1]);
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Linear), nullptr, EnvironmentPattern, "Environment material reference", cleanup.texture[2]);
    device.TexCreate(GxTex_2d, 1, 1, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, LayerAlphaPattern, "Layer alpha boundary", cleanup.texture[3]);
    device.TexCreate(GxTex_NonPow2, 5, 5, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, ShadowPattern, "Dynamic model depth reference", cleanup.texture[4]);
    static unsigned bandIndices[]={1,2,3};
    for(unsigned band=0;band<3;++band)
        device.TexCreate(GxTex_NonPow2,5,5,0,GxTex_Argb8888,GxTex_Argb8888,CGxTexFlags(GxTex_Nearest),&bandIndices[band],ShadowPattern,"Banded model depth reference",cleanup.texture[5+band]);
    device.RsSet(GxRs_Texture1, cleanup.texture[2]);
    device.RsSet(GxRs_Texture0, cleanup.texture[0]);
    GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Fog, 1); // SM3 must retain its shader fog without a second fixed-function fog stage.
    GxRsSet(GxRs_ScissorTest, 0); GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PolygonOffset, 0); GxRsSet(GxRs_PointSprite, 0); GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque); GxRsSet(GxRs_AlphaRef, 0);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    auto checkColoredShadows=[&] {
        for(unsigned family:{1u,2u,6u})
            CheckModelShadows(device,vertex[family],pixel,modPixel,vertex[3],layerPixel[8],vertex[4],vertex[7],layerPixel,pass,
                cleanup.texture[family==6 ? 1:3],cleanup.texture[2],cleanup.texture+4,family==6,GxMetal::ModelVertexEffect(family));
    };
    if(coloredShadowsOnly) { checkColoredShadows(); return; }
    if(projectedShadowsOnly) {
        CheckModelShadows(device,vertex[5],pixel,modPixel,vertex[3],layerPixel[8],vertex[4],vertex[7],layerPixel,pass,cleanup.texture[1],cleanup.texture[2],cleanup.texture+4,true);
        return;
    }
    std::array<Vec, 256> c{};
    c[2] = {0.8f, 0, 0.2f, 0}; c[3] = {0.1f, 0.9f, 0, 0}; c[4] = {0, 0, 1, 0}; c[5] = {0, 0, 0.25f, 1};
    c[6] = {1.3f, 0.2f, 0.2f, 0.05f}; c[7] = {-0.3f, 0.6f, -0.1f, 0.11f};
    c[8] = {0.8f, -0.1f, -0.2f, 0.3f}; c[9] = {0.4f, 1.2f, 0.3f, -0.2f};
    c[10] = {0.6f, 0.5f, 0.7f, 0}; c[11] = {0.15f, 0.2f, 0.1f, 0}; c[12] = {0, 0, -1, 0};
    c[25] = {0.7f, 0.9f, 1.1f, 1.3f}; c[26] = {0.1f, 0.2f, 0.3f, 0.4f}; c[27] = {0.03f, 0.05f, 0.07f, 0.09f};
    c[28] = {0.9f, 0.7f, 0.6f, 0.7f}; c[29] = {0.05f, 0.1f, 0.15f, 0}; c[30] = {-0.65f, 0.95f, 1.7f, 0};
    for (unsigned i = 0; i < 4; ++i) {
        c[17 + i] = {0.03f * (i + 1), 0.02f * (4 - i), 0.015f * (i + 1), 0};
        c[21 + i] = {-0.6f + 0.3f * i, 0.7f - 0.2f * i, 1.2f + 0.1f * i, 0};
    }
    for (unsigned i = 0; i < 5; ++i) {
        c[31 + i * 3] = {0.7f + i * 0.1f, 0.05f * i, 0, 0.02f * i};
        c[32 + i * 3] = {-0.04f * i, 1, 0.1f * i, -0.03f * i};
        c[33 + i * 3] = {0, 0.07f * i, 0.8f + i * 0.1f, 0.04f * i};
    }
    M2Vertex source{};
    source.m_position = C3Vector(0.1f, -0.15f, 0.4f); source.m_normal = C3Vector(0.3f, 0.4f, 0.5f);
    source.m_texcoord[0] = C2Vector(0.25f, 0.7f); source.m_texcoord[1] = C2Vector(0.2f, 0.6f);
    const uint8_t weights[] = {32, 64, 96, 63}, indices[] = {4, 2, 1, 3};
    std::memcpy(&source.m_weights, weights, 4); std::memcpy(&source.m_indices, indices, 4);
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    const Vec fogColor{0.12f, 0.23f, 0.34f, 0.6f};
    // Independently evaluate the original SM3 vertex equations at one vertex.
    // A point checks position, UV matrix, palette indexing, light accumulation,
    // fog and opaque alpha without introducing triangle-interpolation error.
    for (unsigned family = 0; family < unsigned(GxMetal::ModelVertexEffect::Count); ++family) for (unsigned permutation = 0; permutation < (GxMetal::UsesModelMaterial(GxMetal::ModelVertexEffect(family)) ? 30u : 10u); ++permutation) {
        const bool materialColor = GxMetal::UsesModelMaterial(GxMetal::ModelVertexEffect(family));
        const bool projected = family == 5 || family == 6;
        const bool reflectionOnly = family == 7, primaryReflection = reflectionOnly || family == 9 || family == 10;
        source.m_normal.x = (family == 3 || primaryReflection) && (permutation & 2) ? -0.3f : 0.3f;
        source.m_normal.y = (family == 3 || primaryReflection) && (permutation & 4) ? -0.4f : 0.4f;
        const auto format = family == 8 && permutation < 10 ? ((permutation & 1) ? GxVBF_PNT2 : GxVBF_PT2) :
            family == 10 && permutation < 10 ? GxVBF_PNT :
            (reflectionOnly || family == 9) && permutation < 10 ? GxVBF_PN : family == 5 && permutation < 10 ? ((permutation & 1) ? GxVBF_PN : GxVBF_P) :
            family == 6 ? ((permutation & 1) ? GxVBF_P : GxVBF_PC) : materialColor ? GxVBF_PBNT2 : family == 2 && (permutation & 1) ? GxVBF_PNCT :
            family == 1 && (permutation & 1) ? GxVBF_PT : GxVBF_PCT;
        auto* vertices = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
        auto* bytes = reinterpret_cast<uint8_t*>(device.BufLock(vertices));
        if (format == GxVBF_PBNT2) std::memcpy(bytes, &source, sizeof(source));
        else {
            std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_Position), &source.m_position, 12);
            if (GxVertexHasAttrib(format, GxVA_TexCoord0)) std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_TexCoord0), &source.m_texcoord[0], 8);
            if (GxVertexHasAttrib(format, GxVA_TexCoord1)) std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_TexCoord1), &source.m_texcoord[1], 8);
            if (GxVertexHasAttrib(format, GxVA_Normal)) std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_Normal), &source.m_normal, 12);
            const uint32_t packedColor = 0xB380A0C0;
            if (GxVertexHasAttrib(format, GxVA_Color0)) std::memcpy(bytes + GxVertexAttribOffset(format, GxVA_Color0), &packedColor, 4);
        }
        device.BufUnlock(vertices, 0); GxPrimVertexPtr(vertices, format);
        Vec rows[3]{};
        for (unsigned row = 0; row < 3; ++row) for (unsigned component = 0; component < 4; ++component) {
            if (permutation < 20) rows[row][component] = c[31 + (permutation < 10 ? 0 : indices[0] * 3) + row][component];
            else for (unsigned bone = 0; bone < 4; ++bone)
                rows[row][component] += c[31 + indices[bone] * 3 + row][component] * (weights[bone] / 255.0f);
        }
        const Vec position{source.m_position.x, source.m_position.y, source.m_position.z, 1};
        const Vec normal{source.m_normal.x, source.m_normal.y, source.m_normal.z, 0};
        Vec view{Dot(rows[0], position), Dot(rows[1], position), Dot(rows[2], position), 1};
        Vec transformedNormal{Dot(rows[0], normal), Dot(rows[1], normal), Dot(rows[2], normal), 0};
        const float length = std::sqrt(Dot(transformedNormal, transformedNormal, 3));
        for (unsigned i = 0; i < 3; ++i) transformedNormal[i] /= length;
        const Vec vertexColor{128.0f / 255, 160.0f / 255, 192.0f / 255, 179.0f / 255};
        Vec color = materialColor ? c[28] : vertexColor;
        if ((family == 1 || family == 6) && (permutation & 1)) color = {1, 1, 1, 1};
        for (unsigned channel = 0; channel < 3; ++channel) {
            float lighting = Sat(transformedNormal[2]) * c[10][channel] + c[11][channel];
            for (unsigned light = 0; light < (permutation / 2) % 5; ++light) {
                Vec delta{}; for (unsigned axis = 0; axis < 3; ++axis) delta[axis] = c[21 + light][axis] - view[axis];
                const float distance = std::sqrt(Dot(delta, delta, 3));
                lighting += c[17 + light][channel] * std::max(0.0f, Dot(delta, transformedNormal, 3) / distance) /
                    (c[25][light] + c[26][light] * distance + c[27][light] * distance * distance);
            }
            if (materialColor) color[channel] = Sat(c[28][channel] * ((permutation & 1) ? Sat(lighting) : 1) + c[29][channel]);
            else if (family == 2 && (permutation & 1)) color[channel] = Sat(vertexColor[channel] * Sat(lighting) + c[29][channel]);
        }
        const float fog = std::min(1.0f, std::pow(std::max(0.0f, view[2] * c[30][0] + c[30][1]), c[30][2]));
        const auto reflectedUV = EnvironmentCoordinates(view, transformedNormal);
        const Vec secondUV{source.m_texcoord[1].x, source.m_texcoord[1].y, 0, 1};
        const Vec firstUV{source.m_texcoord[0].x, source.m_texcoord[0].y, 0, 1};
        const unsigned texelIndex = primaryReflection ? unsigned(reflectedUV[0] >= 0.5f) + 2*unsigned(reflectedUV[1] >= 0.5f) : projected ? unsigned(Dot(c[6],view) >= 0.5f) + 2*unsigned(Dot(c[7],view) >= 0.5f) : family == 8 ? unsigned(Dot(c[8],secondUV) >= .5f)+2*unsigned(Dot(c[9],secondUV) >= .5f) : 1;
        const Vec baseTexels[] = {{16,68,136,255}, {96,160,224,128}, {192,128,64,64}, {255,255,255,0}};
        Vec texel = baseTexels[texelIndex]; for (auto& channel : texel) channel /= 255;
        const Vec environment = projected ? LayerReference(Dot(c[8],view), Dot(c[9],view)) : family == 10 ? LayerReference(Dot(c[6],firstUV),Dot(c[7],firstUV)) : family == 4 ? LayerReference(Dot(c[8], secondUV), Dot(c[9], secondUV)) : EnvironmentReference(view, transformedNormal);
        const float w = Dot(c[5], view);
        const unsigned x = std::floor((Dot(c[2], view) / w + 1) * 16), y = std::floor((1 - Dot(c[3], view) / w) * 16);
        for (unsigned slot = 0; slot < ((projected || reflectionOnly || family == 8) ? 2u : family >= 3 ? 22u : 2u); ++slot) for (unsigned pixelPermutation : {0u, 4u, 8u, 12u}) {
            const unsigned material = projected ? (slot ? 7 : 4) : slot >= 7 ? slot + 1 : slot;
            const bool mod = material == 1 || material == 3 || material == 4 || material == 7 || material == 8 || material == 13 || material == 16, layered = material >= 2;
            device.RsSet(GxRs_Texture0, cleanup.texture[mod || material == 5 || material == 11 || material >= 19]);
            Vec pixelFog = fogColor; if (material == 6 || material == 15) pixelFog[3] = 0; if (mod) pixelFog[3] = material == 4 || material == 7 || material == 8 ? 0.0f : 0.3f;
            Vec expected = color; if (mod) expected[3] *= texel[3];
            if (material == 7 || material == 9) expected[3] = Sat(expected[3] + environment[3]);
            if (material == 4 || material == 15) expected[3] *= environment[3];
            if (material == 8) expected[3] = Sat(2 * expected[3] * environment[3]);
            if (material == 6) expected[3] = Sat(2 * color[3] * environment[3]);
            if (material == 19) expected[3] = Sat(texel[3]+color[3]);
            if (material == 20) expected[3] = Sat(2*texel[3]*color[3]);
            if (material == 21) expected[3] = Sat((texel[3]+color[3])*environment[3]);
            if (material == 22) expected[3] = Sat(4*texel[3]*color[3]*environment[3]);
            if (material >= 19) pixelFog[3] = 0;
            for (unsigned channel = 0; channel < 3; ++channel) {
                float shaded = texel[channel] * color[channel];
                if (material == 2 || material == 3 || material == 7 || material == 9) shaded += environment[channel];
                if (material == 10) shaded += environment[channel] * environment[3];
                if (material == 11) shaded += (1 - texel[3]) * environment[channel] * environment[3] * color[channel];
                if (material == 4 || material == 13 || material == 14 || material == 15) shaded *= environment[channel];
                if (material == 6 || material == 8 || material == 12 || material == 16) shaded *= 2 * environment[channel];
                if (material == 5) shaded *= texel[3] + 2 * environment[channel] * (1 - texel[3]);
                if (material == 17) shaded = color[3]*color[channel] + (1-color[3])*texel[channel];
                if (material == 18) shaded = color[3]*texel[channel] + (1-color[3])*color[channel];
                if (material == 19) shaded = texel[channel]+color[channel];
                if (material == 20) shaded *= 2;
                if (material == 21) shaded = (texel[channel]+color[channel])*environment[channel];
                if (material == 22) shaded *= 4*environment[channel];
                expected[channel] = Sat(fog * (shaded - fogColor[channel]) + fogColor[channel]);
            }
            if ((pixelPermutation & 8) && expected[3] < pixelFog[3]) expected = {};
            device.BeginFrame(pass);
            GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 256);
            GxShaderConstantsSet(GxSh_Pixel, 2, pixelFog.data(), 1);
            device.RsSet(GxRs_VertexShader, vertex[family][permutation]); device.RsSet(GxRs_PixelShader, (layered ? layerPixel[material - 2] : mod ? modPixel : pixel)[pixelPermutation]);
            GxDraw(&batch, 0);
            CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            const auto& sample = pixels[y * 32 + x]; const uint8_t channels[] = {sample.r, sample.g, sample.b, sample.a};
            for (unsigned channel = 0; channel < 4; ++channel)
                if (std::abs(int(channels[channel]) - int(std::lround(expected[channel] * 255))) > 2)
                    throw std::runtime_error(std::to_string(material) + std::string(" model material mismatch: family ") + std::to_string(family) + " VS " + std::to_string(permutation) + " PS " + std::to_string(pixelPermutation) +
                        " channel " + std::to_string(channel) + " at " + std::to_string(x) + "," + std::to_string(y) +
                        " actual " + std::to_string(channels[channel]) + " expected " + std::to_string(expected[channel] * 255));
            unsigned covered = 0; for (uint32_t i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            Require(covered == unsigned(expected[3] >= 0.5f / 255), "Model point position or raster coverage changed");
            device.ScenePresent();
        }
    }
    // Restore a full model vertex after projected PC/P streams.
    auto* alphaVertex = GxBufStream(GxPoolTarget_Vertex, sizeof(source), 1);
    std::memcpy(device.BufLock(alphaVertex), &source, sizeof(source)); device.BufUnlock(alphaVertex, 0);
    GxPrimVertexPtr(alphaVertex, GxVBF_PBNT2);
    struct AlphaCase { unsigned pixel, stateRef; float shaderRef; bool visible; };
    for (unsigned material = 0; material < 23; ++material) {
        const bool mod = material == 1 || material == 3 || material == 4 || material == 7 || material == 8 || material == 13 || material == 16, layered = material >= 2;
        device.RsSet(GxRs_Texture0, cleanup.texture[mod || material == 5 || material == 11 || material >= 19]);
        device.RsSet(GxRs_Texture1, cleanup.texture[material == 4 || material == 6 || material == 7 || material == 8 || material == 9 || material == 15 || material == 21 || material == 22 ? 3 : 2]);
        const unsigned reference = mod && material != 7 ? 64 : 128;
        const float alpha = reference / 255.0f;
        for (const auto& test : {AlphaCase{0, reference, 0.9f, true}, AlphaCase{0, reference + 1, 0, false},
            AlphaCase{8, 0, alpha, true}, AlphaCase{8, 0, std::nextafter(alpha, 1.0f), false},
            AlphaCase{8, reference, alpha, true}, AlphaCase{8, reference + 1, alpha, false}}) {
            device.BeginFrame(pass);
            c[28][3] = (material == 7 || material == 9) ? 0 : material == 8 ? 255.0f / 512 : material == 4 ? 255.0f / 256 : mod || material == 6 ? 0.5f : material == 15 ? 1.0f : alpha; Vec fog = fogColor; fog[3] = test.shaderRef;
            if (material == 19) c[28][3] = 0;
            if (material == 20) c[28][3] = .5f;
            if (material == 22) c[28][3] = 255.0f / 512;
            if (material == 21) c[28][3] = 1-alpha;
            GxShaderConstantsSet(GxSh_Vertex, 28, c[28].data(), 1); GxShaderConstantsSet(GxSh_Pixel, 2, fog.data(), 1);
            device.RsSet(GxRs_VertexShader, vertex[material == 7 ? 5 : layered ? 3 : 0][0]); device.RsSet(GxRs_PixelShader, (layered ? layerPixel[material - 2] : mod ? modPixel : pixel)[test.pixel]);
            GxRsSet(GxRs_AlphaRef, int32_t(test.stateRef)); GxDraw(&batch, 0);
            CiRect rect(0, 0, 32, 32); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            unsigned covered = 0; for (uint32_t i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            Require(covered == unsigned(test.visible), "Model shader/state alpha threshold or equality mismatch");
            device.ScenePresent();
        }
    }
    device.BeginFrame(pass);
    device.RsSet(GxRs_VertexShader, vertex[0][30]); device.RsSet(GxRs_PixelShader, layerPixel[0][5]);
    bool rejected = false; try { GxDraw(&batch, 0); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected, "Unimplemented model shadow permutation was silently replaced");
    device.ScenePresent();
    CreateComparisonDepths(device,pass,cleanup.texture[3],cleanup.texture+8);
    CheckModelShadows(device,vertex[0],pixel,modPixel,vertex[3],layerPixel[8],vertex[4],vertex[7],layerPixel,pass,cleanup.texture[3],cleanup.texture[2],cleanup.texture+8,false,GxMetal::ModelVertexEffect::Diffuse,true);
    CheckModelShadows(device,vertex[0],pixel,modPixel,vertex[3],layerPixel[8],vertex[4],vertex[7],layerPixel,pass,cleanup.texture[3],cleanup.texture[2],cleanup.texture+4);
    CheckModelShadows(device,vertex[5],pixel,modPixel,vertex[3],layerPixel[8],vertex[4],vertex[7],layerPixel,pass,cleanup.texture[1],cleanup.texture[2],cleanup.texture+4,true);
    checkColoredShadows();
    for(unsigned family=8;family<unsigned(GxMetal::ModelVertexEffect::Count);++family)
        CheckModelShadows(device,vertex[family],pixel,modPixel,vertex[3],layerPixel[8],vertex[4],vertex[7],layerPixel,pass,cleanup.texture[1],cleanup.texture[2],cleanup.texture+4,false,GxMetal::ModelVertexEffect(family));
}

// Independent homogeneous-plane references: no shader equations are reused.
void CheckModelClipPlanes(CGxDeviceMetal& device) {
    CGxShader* vertices[4][90]{}; CGxShader* pixels[16]{}; CGxTex* texture=nullptr;
    const GxMetal::ModelVertexEffect families[]={GxMetal::ModelVertexEffect::Diffuse,GxMetal::ModelVertexEffect::DiffuseEnvironment,
        GxMetal::ModelVertexEffect::DiffuseDualUV,GxMetal::ModelVertexEffect::DiffuseReflection};
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader* (*vertices)[90]; CGxShader** pixels; CGxTex*& texture;
        ~Cleanup() {
            GxRsSet(GxRs_ClipPlaneMask,0); if(device.FrameCommand()) device.ScenePresent();
            for(unsigned family=0;family<4;++family) for(auto* shader:vertices[family]) device.ShaderDestroy(shader);
            for(unsigned i=0;i<16;++i) device.ShaderDestroy(pixels[i]);
            if(texture) device.TexDestroy(texture);
        }
    } cleanup{device,vertices,pixels,texture};
    for(unsigned family=0;family<4;++family)
        device.ShaderCreate(vertices[family],GxSh_Vertex,"Shaders\\Vertex",GxMetal::ModelVertexFamily(families[family]),90);
    device.ShaderCreate(pixels,GxSh_Pixel,"Shaders\\Pixel","Combiners_Opaque",16);
    device.TexCreate(GxTex_2d,2,2,0,GxTex_Argb8888,GxTex_Argb8888,CGxTexFlags(GxTex_Linear),nullptr,
        EnvironmentPattern,"Model clip reference",texture);
    GxRsSet(GxRs_Texture0,texture); GxRsSet(GxRs_PixelShader,pixels[0]);
    GxRsSet(GxRs_BlendingMode,GxBlend_Opaque); GxRsSet(GxRs_AlphaRef,0); GxRsSet(GxRs_ColorWrite,15);
    GxRsSet(GxRs_DepthTest,0); GxRsSet(GxRs_DepthWrite,0); GxRsSet(GxRs_Culling,0);
    GxRsSet(GxRs_Fog,0); GxRsSet(GxRs_ScissorTest,0); GxRsSet(GxRs_PolygonOffset,0); GxRsSet(GxRs_PointSprite,0);
    GxXformSetViewport(0,1,0,1,0,1);
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage=MTLTextureUsageRenderTarget; descriptor.storageMode=MTLStorageModePrivate;
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=[device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction=MTLLoadActionClear; pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
    std::array<Vec,256> c{};
    c[2]={1,0,0,0}; c[3]={0,1,0,0}; c[4]={0,0,1,0}; c[5]={0,0,0,1};
    c[6]={1,0,0,0}; c[7]={0,1,0,0}; c[8]=c[6]; c[9]=c[7];
    c[10]={1,1,1,0}; c[12]={0,0,-1,0}; c[25]={1,1,1,1}; c[28]={1,1,1,1}; c[30]={0,1,1,0};
    for(unsigned i=0;i<5;++i) { c[31+3*i]={1,0,0,i*.0625f}; c[32+3*i]={0,1,0,i*-.0625f}; c[33+3*i]={0,0,1,0}; }
    const Vec pixelConstants{}; GxShaderConstantsSet(GxSh_Pixel,2,pixelConstants.data(),1);
    M2Vertex source{}; source.m_position={.125f,-.125f,.5f}; source.m_normal={0,0,1};
    const uint8_t weights[]={255,0,0,0}, bones[]={4,3,2,1};
    std::memcpy(&source.m_weights,weights,4); std::memcpy(&source.m_indices,bones,4);
    auto* buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(source),1);
    std::memcpy(device.BufLock(buffer),&source,sizeof(source)); device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,GxVBF_PBNT2);
    CGxBatch point{GxPrim_Points,0,1,0,0}; unsigned checks=0;
    auto setPlane=[&](unsigned index,const Vec& coefficients) {
        C4Plane plane; plane.n={coefficients[0],coefficients[1],coefficients[2]}; plane.d=coefficients[3]; device.ClipPlaneSet(index,plane);
    };
    // Alternate enabled/disabled keys and repeatedly replace the same plane.
    for(unsigned family=0;family<4;++family) for(unsigned permutation:{0u,1u,10u,11u,20u,21u}) for(float w:{1.0f,2.0f}) {
        const float x=permutation<10 ? .125f:.375f;
        c[5][3]=w; GxShaderConstantsSet(GxSh_Vertex,0,c[0].data(),256);
        GxRsSet(GxRs_VertexShader,vertices[family][permutation]);
        for(unsigned plane=0;plane<6;++plane) for(unsigned side=0;side<4;++side) {
            const float boundary=side==2 ? std::nextafter(x/w,1.0f):x/w;
            setPlane(plane,{1,0,0,-boundary});
            GxRsSet(GxRs_ClipPlaneMask,side==0 || side==3 ? 0:int32_t(1u<<plane));
            device.BeginFrame(pass); GxDraw(&point,0); CiRect rect(0,0,32,32); TSGrowableArray<CImVector> image; device.DeviceReadPixels(rect,image);
            unsigned covered=0; for(unsigned i=0;i<image.Count();++i) covered+=image[i].a!=0;
            Require(covered==unsigned(side!=2),"Model clip plane equality, replacement or disabled state mismatch");
            device.ScenePresent(); ++checks;
        }
    }
    // Full triangles reveal intersection clipping and plane conjunction. All
    // boundaries lie between sample centers, independent of edge fill rules.
    std::array<M2Vertex,4> quad{};
    for(unsigned i=0;i<4;++i) { quad[i]=source; quad[i].m_position={i&1 ? 1.0f:-1.0f,i&2 ? 1.0f:-1.0f,.5f}; quad[i].m_texcoord[0]={i&1 ? 1.0f:0.0f,i&2 ? 1.0f:0.0f}; }
    buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(source),4);
    std::memcpy(device.BufLock(buffer),quad.data(),sizeof(quad)); device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,GxVBF_PBNT2);
    for(unsigned i=0;i<5;++i) { c[31+3*i][3]=0; c[32+3*i][3]=0; }
    c[5][3]=1; GxShaderConstantsSet(GxSh_Vertex,0,c[0].data(),256);
    const Vec planes[]={{1,0,0,.25},{0,-1,0,.5},{1,1,0,.03125},{-1,0,0,.75},{0,1,0,.75},{0,0,-1,.75}};
    CGxBatch triangles{GxPrim_TriangleStrip,0,4,0,0};
    for(unsigned i=0;i<6;++i) setPlane(i,planes[i]);
    std::vector<CImVector> reference;
    for(unsigned family=0;family<4;++family) for(unsigned permutation:{0u,11u,21u}) for(unsigned mask:{0u,1u,2u,4u,8u,16u,32u,3u,63u,0u}) {
        GxRsSet(GxRs_VertexShader,vertices[family][permutation]); GxRsSet(GxRs_ClipPlaneMask,int32_t(mask));
        device.BeginFrame(pass); GxDraw(&triangles,0); CiRect rect(0,0,32,32); TSGrowableArray<CImVector> image; device.DeviceReadPixels(rect,image);
        for(unsigned y=0;y<32;++y) for(unsigned x=0;x<32;++x) {
            const Vec position{(x+.5f)/16-1,1-(y+.5f)/16,.5,1}; bool visible=true;
            for(unsigned i=0;i<6;++i) if(mask&(1u<<i)) visible&=Dot(planes[i],position)>=0;
            Require(bool(image[y*32+x].a)==visible,"Model multiple-plane triangle coverage mismatch");
            if(mask && visible) {
                const auto* expected=reinterpret_cast<const uint8_t*>(&reference[y*32+x]);
                const auto* actual=reinterpret_cast<const uint8_t*>(&image[y*32+x]);
                for(unsigned channel=0;channel<4;++channel)
                    Require(std::abs(int(actual[channel])-int(expected[channel]))<=2,"Model clipping changed interpolated material RGBA");
            }
        }
        if(!mask) reference.assign(image.Ptr(),image.Ptr()+image.Count());
        device.ScenePresent(); ++checks;
    }
    std::fprintf(stderr,"Model clipping: %u draws, six independent homogeneous planes, rigid/one/four weights, plane equality, nonunit W, intersections and disabled-state restoration passed\n",checks);
}

void CheckModelDepthOffset(CGxDeviceMetal& device) {
    CGxShader* vertex[4][90]{}; CGxShader* pixel[2][16]{}; CGxTex* texture=nullptr;
    struct Cleanup {
        CGxDeviceMetal& device; CGxShader* (*vertex)[90]; CGxShader* (*pixel)[16]; CGxTex*& texture;
        ~Cleanup() {
            if(device.FrameCommand()) device.ScenePresent();
            GxRsSet(GxRs_PolygonOffset,0.0f); GxRsSet(GxRs_ClipPlaneMask,0);
            GxRsSet(GxRs_DepthTest,0); GxRsSet(GxRs_DepthWrite,0); GxXformSetViewport(0,1,0,1,0,1);
            for(unsigned family=0;family<4;++family) for(auto* shader:vertex[family]) device.ShaderDestroy(shader);
            for(unsigned family=0;family<2;++family) for(auto* shader:pixel[family]) device.ShaderDestroy(shader); if(texture) device.TexDestroy(texture);
        }
    } cleanup{device,vertex,pixel,texture};
    const char* families[]={"Diffuse_T1","Diffuse_T1_Env","Diffuse_T1_T2","Diffuse_Env"};
    for(unsigned i=0;i<4;++i) device.ShaderCreate(vertex[i],GxSh_Vertex,"Shaders\\Vertex",families[i],90);
    device.ShaderCreate(pixel[0],GxSh_Pixel,"Shaders\\Pixel","Combiners_Mod",16);
    device.ShaderCreate(pixel[1],GxSh_Pixel,"Shaders\\Pixel","Combiners_Mod_Mod",16);
    device.TexCreate(GxTex_2d,1,1,0,GxTex_Argb8888,GxTex_Argb8888,CGxTexFlags(GxTex_Nearest),nullptr,LayerAlphaPattern,"Model depth reference",texture);
    GxRsSet(GxRs_Texture0,texture); GxRsSet(GxRs_Texture1,texture);
    for(auto state:{GxRs_AlphaRef,GxRs_DepthTest,GxRs_Culling,GxRs_ScissorTest,GxRs_Fog,GxRs_ClipPlaneMask,GxRs_PointSprite}) GxRsSet(state,0);
    GxRsSet(GxRs_BlendingMode,GxBlend_Opaque); GxRsSet(GxRs_ColorWrite,15); GxRsSet(GxRs_DepthWrite,1); GxXformSetViewport(0,1,0,1,0,1);
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.usage=MTLTextureUsageRenderTarget; descriptor.storageMode=MTLStorageModePrivate;
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor]; pass.colorAttachments[0].texture=[device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction=MTLLoadActionClear; pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
    descriptor.pixelFormat=MTLPixelFormatDepth32Float; pass.depthAttachment.texture=[device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction=MTLLoadActionClear; pass.depthAttachment.storeAction=MTLStoreActionStore; pass.depthAttachment.clearDepth=1;
    std::array<Vec,256> c{}; c[2]={1,0,0,0}; c[3]={0,1,0,0}; c[4]={0,0,1,0}; c[5]={0,0,0,1};
    c[6]={1,0,0,0}; c[7]={0,1,0,0}; c[8]=c[6]; c[9]=c[7];
    c[10]={1,1,1,0}; c[12]={0,0,-1,0}; c[25]={1,1,1,1}; c[28]={1,1,1,1}; c[30]={0,1,1,0};
    c[31]={1,0,0,0}; c[32]={0,1,0,0}; c[33]={0,0,1,0}; GxShaderConstantsSet(GxSh_Vertex,0,c[0].data(),256);
    const Vec fog{}; GxShaderConstantsSet(GxSh_Pixel,2,fog.data(),1); unsigned draws=0; bool layered=false;
    auto encode=[&](float z,float offset,EGxPrim primitive=GxPrim_TriangleStrip,bool slope=false) {
        std::array<M2Vertex,4> source{};
        for(unsigned i=0;i<4;++i) {
            auto& v=source[i]; v.m_position={i&1 ? 1.0f:-1.0f,i&2 ? 1.0f:-1.0f,slope ? (i&1 ? .9f:.1f):z};
            v.m_normal={0,0,1}; v.m_texcoord[0]=v.m_texcoord[1]={.5f,.5f};
            const uint8_t weight[]={255,0,0,0}; std::memcpy(&v.m_weights,weight,4);
        }
        // Center one-pixel primitives on a pixel so the material/depth check
        // observes full coverage in both single-sample and MSAA attachments.
        if(primitive==GxPrim_Points) source[0].m_position={1.0f/16,-1.0f/16,z};
        if(primitive==GxPrim_Lines) { source[0].m_position={-.5f,-1.0f/16,z}; source[1].m_position={.5f,-1.0f/16,z}; }
        auto* buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(M2Vertex),4);
        std::memcpy(device.BufLock(buffer),source.data(),sizeof(source)); device.BufUnlock(buffer,0); GxPrimVertexPtr(buffer,GxVBF_PBNT2);
        GxRsSet(GxRs_PolygonOffset,offset); CGxBatch batch{primitive,0,primitive==GxPrim_Points ? 1u:primitive==GxPrim_Lines ? 2u:4u,0,0};
        GxDraw(&batch,0); ++draws;
    };
    auto check=[&](float expectedDepth,bool visible,EGxPrim primitive=GxPrim_TriangleStrip) {
        CiRect rect(0,0,16,16); TSGrowableArray<CImVector> color; TSGrowableArray<float> depth;
        device.DeviceReadPixels(rect,color); device.DeviceReadDepths(rect,depth); unsigned sample=8*16+8;
        if(primitive<GxPrim_Triangles) for(unsigned i=0;i<color.Count();++i) if(color[i].a) { sample=i; break; }
        Require(bool(color[sample].a)==visible,"Model depth offset changed expected depth/alpha rejection");
        if (std::abs(depth[sample]-expectedDepth)>=1e-6f)
            std::fprintf(stderr,"Model depth draw %u primitive %u pixel %u: %.9f expected %.9f\n",draws,unsigned(primitive),sample,depth[sample],expectedDepth);
        Require(std::abs(depth[sample]-expectedDepth)<1e-6f,"Model normalized depth offset mismatch");
        if(visible) Require(std::abs(int(color[sample].r)-(layered ? 16:64))<=1 && std::abs(int(color[sample].g)-(layered ? 64:128))<=1 &&
            std::abs(int(color[sample].b)-(layered ? 145:192))<=1 && std::abs(int(color[sample].a)-(layered ? 64:128))<=1,"Model depth offset changed material RGBA");
    };
    auto draw=[&](float z,float offset,float expectedDepth,bool visible=true,EGxPrim primitive=GxPrim_TriangleStrip,bool slope=false) {
        device.BeginFrame(pass); encode(z,offset,primitive,slope); check(expectedDepth,visible,primitive); device.ScenePresent();
    };
    for(unsigned family=0;family<4;++family) for(unsigned permutation:{0u,11u,21u}) {
        layered=family==1 || family==2; GxRsSet(GxRs_PixelShader,pixel[layered][0]);
        GxRsSet(GxRs_VertexShader,vertex[family][permutation]);
        for(float z:{.05f,.5f,.95f}) for(float offset:{-2.0f,-.05f,0.0f,.000024414435f,.05f,2.0f}) draw(z,offset,std::clamp(z-offset,0.0f,1.0f));
    }
    layered=false; GxRsSet(GxRs_VertexShader,vertex[0][0]); GxRsSet(GxRs_PixelShader,pixel[0][0]);
    GxXformSetViewport(0,1,0,1,.2f,.8f); draw(.25f,.125f,.225f); GxXformSetViewport(0,1,0,1,0,1);
    draw(-.1f,-.5f,1,false); draw(1.1f,.5f,1,false);
    pass.depthAttachment.clearDepth=.5; GxRsSet(GxRs_DepthTest,1); GxRsSet(GxRs_DepthFunc,3); GxRsSet(GxRs_DepthWrite,0);
    for(float offset:{-.05f,0.0f,.05f}) draw(.5f,offset,.5f,offset>0);
    GxRsSet(GxRs_DepthWrite,1); GxRsSet(GxRs_AlphaRef,128); draw(.5f,.05f,.45f);
    GxRsSet(GxRs_AlphaRef,129); draw(.5f,.05f,.5f,false); GxRsSet(GxRs_AlphaRef,0);
    GxRsSet(GxRs_DepthTest,0); pass.depthAttachment.clearDepth=1;
    MTLSamplePosition firstSample = MTLSamplePositionMake(.5f,.5f);
    if (device.m_format.m_multisampleCount > 1) {
        std::vector<MTLSamplePosition> positions(device.m_format.m_multisampleCount);
        [device.GPU().device getDefaultSamplePositions:positions.data() count:positions.size()]; firstSample=positions[0];
    }
    draw(.5f,.05f,.475f+(firstSample.x-.5f)*.8f/16,true,GxPrim_TriangleStrip,true);
    for(auto primitive:{GxPrim_Points,GxPrim_Lines}) draw(.5f,.25f,.5f,true,primitive);
    device.BeginFrame(pass); for(float offset:{0.0f,.25f,0.0f}) encode(.5f,offset); check(.5f,true); device.ScenePresent();
    std::fprintf(stderr,"Model depth offset: %u draws, four rigid/skinned vertex families, normalized sign/saturation, viewport, clip rejection, alpha/depth states, slopes, points/lines and same-frame restoration passed\n",draws);
}
