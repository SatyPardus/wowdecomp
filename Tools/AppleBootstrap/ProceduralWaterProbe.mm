#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
using V = std::array<float, 4>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Image {
    uint32_t pixels[6][16]{};
    bool cube = false, linear = false, wrap = false;
    static void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t face, uint32_t, void* context,
                       uint32_t& stride, const void*& data) {
        if (command == GxTex_Latch) { stride = 16; data = static_cast<Image*>(context)->pixels[face]; }
    }
    V Sample(V p) const {
        unsigned face = 0;
        if (cube) {
            float x = p[0], y = p[1], z = p[2], m;
            if (std::abs(x) >= std::abs(y) && std::abs(x) >= std::abs(z)) {
                face = x >= 0 ? 0 : 1; m = std::abs(x); p[0] = x >= 0 ? -z : z; p[1] = -y;
            } else if (std::abs(y) >= std::abs(z)) {
                face = y >= 0 ? 2 : 3; m = std::abs(y); p[0] = x; p[1] = y >= 0 ? z : -z;
            } else { face = z >= 0 ? 4 : 5; m = std::abs(z); p[0] = z >= 0 ? x : -x; p[1] = -y; }
            p[0] = m ? (p[0] / m + 1) * .5f : .5f;
            p[1] = m ? (p[1] / m + 1) * .5f : .5f;
        }
        auto texel = [&](int x, int y) {
            auto address = [&](int i) { return wrap ? ((i % 4) + 4) % 4 : std::clamp(i, 0, 3); };
            const uint32_t c = pixels[face][address(y)*4+address(x)];
            return V{float((c>>16)&255)/255, float((c>>8)&255)/255, float(c&255)/255, float(c>>24)/255};
        };
        const float x = p[0]*4 - (linear ? .5f : 0), y = p[1]*4 - (linear ? .5f : 0);
        const int ix = int(std::floor(x)), iy = int(std::floor(y));
        V a = texel(ix, iy);
        if (!linear) return a;
        const V b = texel(ix+1, iy), c = texel(ix, iy+1), d = texel(ix+1, iy+1);
        for (unsigned j=0;j<4;++j) a[j] = (a[j]*(1-(x-ix))+b[j]*(x-ix))*(1-(y-iy)) + (c[j]*(1-(x-ix))+d[j]*(x-ix))*(y-iy);
        return a;
    }
};
std::vector<uint32_t> Program(const char* path) {
    SFileImpl* file = nullptr;
    Require(SFileOpenFile(path, &file) && file, "Cannot open ProcWater reference BLS");
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    const uint32_t size = SFileGetFileSize(file, nullptr);
    Require(size <= 1024*1024, "Invalid ProcWater reference BLS size");
    std::vector<uint8_t> bytes(size); uint32_t got = 0;
    Require(SFileReadFile(file, bytes.data(), size, &got, nullptr, 0) && got == size, "Cannot read ProcWater reference BLS");
    const auto programs = GxReadShaderBinary(bytes, 1024*1024);
    const auto& code = programs.at(0).code;
    Require(code.size()%4==0, "Unaligned ProcWater reference program length");
    std::vector<uint32_t> words(code.size()/4); std::memcpy(words.data(), code.data(), code.size()); return words;
}
// A deliberately small evaluator for the mounted ProcWater SM3 instructions.
// This reference executes the original bytecode, independently of the native
// wave/optics formulas. Unsupported instructions fail instead of approximating.
struct Reference {
    V constants[256]{}, input[16]{}, output[16]{}, temporary[32]{}, integers[16]{}, loop{};
    const std::array<Image,6>* images = nullptr;
    static unsigned Type(uint32_t token) { return ((token>>28)&7)|((token>>8)&24); }
    V& Register(unsigned type, unsigned index) {
        switch (type) {
            case 0: Require(index<32,"Reference temp index"); return temporary[index];
            case 1: Require(index<16,"Reference input index"); return input[index];
            case 2: Require(index<256,"Reference constant index"); return constants[index];
            case 6: case 8: Require(index<16,"Reference output index"); return output[index];
            case 7: Require(index<16,"Reference integer index"); return integers[index];
            case 15: return loop;
            default: throw std::runtime_error("Unexpected ProcWater reference register");
        }
    }
    V Source(const std::vector<uint32_t>& words, size_t& cursor) {
        const auto t=words.at(cursor++); unsigned index=t&2047;
        if (t&8192) { const V address=Source(words,cursor); index+=unsigned(address[0]); }
        const V v=Register(Type(t),index); V result{};
        Require(((t>>24)&15)<=1,"Unexpected reference source modifier");
        for(unsigned j=0;j<4;++j) result[j]=v[(t>>(16+2*j))&3] * (((t>>24)&15)==1 ? -1 : 1);
        return result;
    }
    void Run(const std::vector<uint32_t>& words) {
        size_t pc=1, loopStart=0; unsigned remaining=0; float step=0;
        for (unsigned instructions=0;pc<words.size() && instructions<4096;++instructions) {
            const auto token=words.at(pc++); const unsigned op=token&65535;
            if(op==65535) return;
            if(op==65534) { pc+=(token>>16)&32767; continue; }
            const auto end=pc+((token>>24)&15);
            if(op==31) { pc=end; continue; }
            if(op==48 || op==81) {
                const auto dst=words.at(pc++); V value{};
                for(unsigned j=0;j<4;++j) { auto t=words.at(pc++); if(op==48) value[j]=int32_t(t); else std::memcpy(&value[j],&t,4); }
                Register(Type(dst),dst&2047)=value; continue;
            }
            if(op==27) { ++pc; const auto info=Source(words,pc); remaining=unsigned(info[0]); loop[0]=info[1];step=info[2];loopStart=end;continue; }
            if(op==29) { if(--remaining) {loop[0]+=step;pc=loopStart;} continue; }
            const auto dst=words.at(pc++); V a=Source(words,pc), b{}, c{}, result{};
            if(pc<end && op!=66) b=Source(words,pc);
            if(pc<end && op!=66) c=Source(words,pc);
            if(op==66) { Require(images && pc+1==end,"Invalid reference texture instruction"); result=images->at(words.at(pc++)&2047).Sample(a); }
            else {
                float dot=0; if(op==8 || op==9 || op==36 || op==90) for(unsigned j=0;j<(op==9?4u:op==90?2u:3u);++j) dot+=a[j]*(op==36?a[j]:b[j]);
                for(unsigned j=0;j<4;++j) switch(op) {
                    case 1: result[j]=a[j];break;
                    case 2: result[j]=a[j]+b[j];break;
                    case 3: result[j]=a[j]-b[j];break;
                    case 4: result[j]=std::fma(a[j],b[j],c[j]);break;
                    case 5: result[j]=a[j]*b[j];break;
                    case 6: result[j]=1/a[0];break;
                    case 7: result[j]=1/std::sqrt(std::abs(a[0]));break;
                    case 8: case 9: result[j]=dot;break;
                    case 10: result[j]=std::min(a[j],b[j]);break;
                    case 11: result[j]=std::max(a[j],b[j]);break;
                    case 18: result[j]=a[j]*b[j]+(1-a[j])*c[j];break;
                    case 19: result[j]=a[j]-std::floor(a[j]);break;
                    case 32: result[j]=std::pow(std::abs(a[0]),b[0]);break;
                    case 36: result[j]=a[j]/std::sqrt(dot);break;
                    case 37: result[j]=j==0?std::cos(a[0]):std::sin(a[0]);break;
                    case 88: result[j]=a[j]>=0?b[j]:c[j];break;
                    case 90: result[j]=dot+c[0];break;
                    default: throw std::runtime_error("Unexpected ProcWater reference opcode " + std::to_string(op));
                }
            }
            Require(pc==end,"Invalid reference instruction length");
            auto& value=Register(Type(dst),dst&2047);
            for(unsigned j=0;j<4;++j) if(dst&(1u<<(16+j))) value[j]=(dst&(1u<<20))?std::clamp(result[j],0.0f,1.0f):result[j];
        }
        throw std::runtime_error("ProcWater reference did not terminate");
    }
};
} // namespace

void CheckProceduralWater(CGxDeviceMetal& device) {
    const auto vertexProgram=Program("Shaders\\Vertex\\vs_3_0\\vsLiquidProcWater.bls");
    const auto pixelProgram=Program("Shaders\\Pixel\\ps_3_0\\psLiquidProcWater.bls");
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& d; CGxShader* vs[4]{}, *ps=nullptr; CGxTex* textures[6]{};
        ~Cleanup() { if(d.FrameCommand()) d.ScenePresent(); d.RsPop();for(auto& t:textures)d.TexDestroy(t);for(auto& s:vs)d.ShaderDestroy(s);d.ShaderDestroy(ps); }
    } cleanup{device};
    device.ShaderCreate(cleanup.vs,GxSh_Vertex,"Shaders\\Vertex","vsLiquidProcWater",4);
    device.ShaderCreate(&cleanup.ps,GxSh_Pixel,"Shaders\\Pixel","psLiquidProcWater",1);
    std::array<Image,6> images;
    for(unsigned i=0;i<6;++i) {
        auto& image=images[i]; image.cube=i<2;image.linear=(i%2)!=0;image.wrap=i>=3;
        for(unsigned face=0;face<6;++face)for(unsigned pixel=0;pixel<16;++pixel) {
            unsigned r=16+face*7+pixel*2+i*4,g=25+pixel*3+i*5,b=30+face*4+i*6,a=60+pixel*7;
            if(i>=4) {r=105+pixel*2;g=120+pixel;b=110+pixel;a=130+pixel;}
            image.pixels[face][pixel]=(a<<24)|(r<<16)|(g<<8)|b;
        }
        CGxTexFlags flags(image.linear?GxTex_Linear:GxTex_Nearest);
        flags.m_wrapU=flags.m_wrapV=image.wrap?GxTex_Wrap:GxTex_Clamp;
        device.TexCreate(image.cube?GxTex_CubeMap:GxTex_2d,4,4,0,GxTex_Argb8888,GxTex_Argb8888,flags,&image,Image::Upload,"ProcWater reference",cleanup.textures[i]);
        device.RsSet(EGxRenderState(GxRs_Texture0+i),cleanup.textures[i]);
    }
    for(auto state:{GxRs_DepthTest,GxRs_DepthWrite,GxRs_Culling,GxRs_Fog,GxRs_ScissorTest,GxRs_ClipPlaneMask,GxRs_PolygonOffset,GxRs_PointSprite,GxRs_AlphaRef})device.RsSet(state,0);
    device.RsSet(GxRs_ColorWrite,15);device.RsSet(GxRs_BlendingMode,GxBlend_Opaque);GxXformSetViewport(0,1,0,1,0,1);
    auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    desc.usage=MTLTextureUsageRenderTarget;desc.storageMode=MTLStorageModePrivate;
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=[device.GPU().device newTextureWithDescriptor:desc];
    pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
    unsigned checked=0;
    for(unsigned scenario=0;scenario<24;++scenario) {
        Reference vs,ps; ps.images=&images;
        auto& c=vs.constants;auto& p=ps.constants;
        for(unsigned j=0;j<4;++j)c[j][j]=c[5+j][j]=1;
        c[0][0]=.8f;c[1][0]=.1f;c[6][2]=.2f;c[8][2]=.05f;
        c[4]={-.4f,.9f,1.3f,0};
        for(unsigned i=0;i<6;++i) { c[9+i*4]={.7f+i*.11f,.1f,0,0};c[10+i*4]={.13f,.9f-i*.07f,0,0};c[12+i*4]={-.23f+i*.09f,.14f-i*.03f,0,1}; }
        for(unsigned i=0;i<3;++i) {
            c[46][i]=scenario*.27f+i*.3f;c[47][i]=scenario*.43f-i*.2f;
            c[48+i]={-.6f+i*.3f,.7f-i*.4f,.6f+i*.2f,scenario%4&1?.35f:0};
            c[51][i]=.13f+i*.1f;c[52][i]=.17f+i*.1f;
            c[53+i]={.5f-i*.3f,-.8f+i*.2f,.4f+i*.3f,.7f-i*.15f};
            c[56][i]=.5f+i*.3f;c[57][i]=scenario%4&2?.27f:0;
        }
        vs.input[0]={.13f+float(scenario%3)*.1f,-.22f,.31f,1};
        vs.input[1]={.71f,.24f,0,0};vs.input[2]={.21f,.73f,0,0};
        const V eyeDirections[]={{.7f,.2f,.3f,0},{-.7f,.2f,.3f,0},{.1f,.3f,-1,0},{-.1f,.3f,1,0},{.2f,.8f,-.1f,0},{.2f,-.8f,.1f,0}};
        for(unsigned j=0;j<3;++j)p[5][j]=vs.input[0][j]-eyeDirections[scenario/4][j];
        p[4]={.12f,.17f,.23f,0};p[6]={.2f,-.3f,.9f,0};p[9]={.11f,.08f,.12f,5};
        p[10]={scenario%3==0?-.9f:scenario%3==1?.8f:0,scenario%2?.7f:1.4f,.2f,.5f};p[11]={.12f,.6f,.7f,0};
        // Upload before the evaluator applies bytecode DEF overrides.
        GxShaderConstantsSet(GxSh_Vertex,0,c[0].data(),58);GxShaderConstantsSet(GxSh_Pixel,0,p[0].data(),12);
        struct Vertex {float position[3], uv[2], depthUV[2];} vertex;
        std::memcpy(vertex.position,vs.input[0].data(),12);std::memcpy(vertex.uv,vs.input[1].data(),8);std::memcpy(vertex.depthUV,vs.input[2].data(),8);
        auto* buffer=GxBufStream(GxPoolTarget_Vertex,sizeof(vertex),1);std::memcpy(device.BufLock(buffer),&vertex,sizeof(vertex));device.BufUnlock(buffer,0);GxPrimVertexPtr(buffer,GxVBF_PT2);
        vs.Run(vertexProgram);
        const unsigned mapping[]={5,6,7,1,2,3,4,8};for(unsigned i=0;i<8;++i)ps.input[i]=vs.output[mapping[i]];
        ps.Run(pixelProgram);const auto expected=ps.output[0];
        const auto& clip=vs.output[0];const unsigned x=unsigned(std::ceil((clip[0]/clip[3]+1)*16+.5f)-1),y=unsigned(std::ceil((1-clip[1]/clip[3])*16+.5f)-1);
        for(unsigned variant=0;variant<4;++variant)for(unsigned mode=0;mode<3;++mode) {
            const unsigned alpha=mode==0?0:mode==1?80:255;device.RsSet(GxRs_AlphaRef,int32_t(alpha));
            device.RsSet(GxRs_VertexShader,cleanup.vs[variant]);device.RsSet(GxRs_PixelShader,cleanup.ps);device.BeginFrame(pass);
            CGxBatch batch{GxPrim_Points,0,1,0,0};device.Draw(&batch,0);
            CiRect rect(0,0,32,32);TSGrowableArray<CImVector> pixels;device.DeviceReadPixels(rect,pixels);
            const auto got=pixels[y*32+x];const unsigned actual[]={got.r,got.g,got.b,got.a};const bool discard=expected[3]<alpha/255.0f;
            for(unsigned j=0;j<4;++j) {
                const float wanted=discard?0:std::clamp(expected[j],0.0f,1.0f)*255;
                if(!std::isfinite(wanted)||std::abs(actual[j]-wanted)>2) {
                    char message[256];std::snprintf(message,sizeof(message),"ProcWater BLS reference scenario %u variant %u alpha %u channel %u: got %u expected %.4f",scenario,variant,alpha,j,actual[j],wanted);throw std::runtime_error(message);
                }
            }
            unsigned covered=0;for(unsigned i=0;i<pixels.Count();++i)covered+=pixels[i].a!=0;
            Require(covered==(discard?0u:1u),"ProcWater geometry changed instead of its wave normal");
            device.ScenePresent();++checked;
        }
    }
    std::fprintf(stderr,"ProcWater: %u original-BLS color/alpha/position references; radial/planar waves, six texture slots, two cubes, near/far normal blend, reflection/refraction, fog and all four VS handles passed\n",checked);
}
