#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string>

void CreateShadowComparisonDepths(CGxDeviceMetal&, MTLRenderPassDescriptor*, CGxTex*, CGxTex**);

namespace {
using V = std::array<float, 4>;
float Dot(const V &a, const V &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
float Sat(float x) {
    return std::clamp(x, 0.0f, 1.0f);
}
V Unit(V v) {
    float length = std::sqrt(Dot(v, v));
    for (unsigned i = 0; i < 3; ++i)
        v[i] /= length;
    return v;
}
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void *p, uint32_t &stride, const void *&data) {
    if (command == GxTex_Latch) {
        stride = 8;
        data = p;
    }
}
void UploadDepth(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void *p, uint32_t &stride, const void *&data) {
    if (command == GxTex_Latch) { stride = 5*sizeof(uint32_t); data = p; }
}
V Color(uint32_t value) {
    return {((value >> 16) & 255) / 255.0f, ((value >> 8) & 255) / 255.0f, (value & 255) / 255.0f, (value >> 24) / 255.0f};
}
V Sample(const std::array<uint32_t, 4> &image, float u, float v, bool wrap) {
    if (wrap) {
        u -= std::floor(u);
        v -= std::floor(v);
    }
    return Color(image[std::clamp(int(std::floor(v * 2)), 0, 1) * 2 + std::clamp(int(std::floor(u * 2)), 0, 1)]);
}
} // namespace

void CheckWmoShaders(CGxDeviceMetal &device) {
    CGxShader *vs[7][90]{};
    CGxShader *ps[7][16]{};
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal &device;
        CGxShader *(*vs)[90];
        CGxShader *(*ps)[16];
        CGxTex *textures[10]{};
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            for (auto &texture : textures)
                device.TexDestroy(texture);
            for (unsigned i = 0; i < 7; ++i) {
                for (auto &shader : vs[i])
                    device.ShaderDestroy(shader);
                for (auto &shader : ps[i])
                    device.ShaderDestroy(shader);
            }
        }
    } cleanup{device, vs, ps};
    for (unsigned i = 0; i < 7; ++i) {
        device.ShaderCreate(vs[i], GxSh_Vertex, "Shaders\\Vertex", GxMetal::WmoVertexFamily(GxMetal::WmoVertexEffect(i)), 90);
        device.ShaderCreate(ps[i], GxSh_Pixel, "Shaders\\Pixel",
                            GxMetal::WmoPixelFamily(GxMetal::FragmentEffect(unsigned(GxMetal::FragmentEffect::WmoDiffuse) + i)), 16);
    }
    std::array<uint32_t, 4> images[2] = {{{0x80402060, 0xA0608030, 0xC0306070, 0xE0703020}},
                                         {{0x70403020, 0x90604070, 0xB0307060, 0xD0502080}}};
    for (unsigned i = 0; i < 2; ++i) {
        CGxTexFlags flags(GxTex_Nearest);
        flags.m_wrapU = flags.m_wrapV = i ? GxTex_Wrap : GxTex_Clamp;
        device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, flags, images[i].data(), Upload, "WMO material input",
                         cleanup.textures[i]);
        device.RsSet(EGxRenderState(GxRs_Texture0 + i), cleanup.textures[i]);
    }
    std::array<std::array<uint32_t,25>,4> depth{};
    for(unsigned band=0;band<4;++band) {
        for(unsigned y=0;y<5;++y) for(unsigned x=0;x<5;++x)
            depth[band][y*5+x]=0xFF000000u | ((64*(1+(x+2*y+band)%3))<<16);
        device.TexCreate(GxTex_NonPow2,5,5,0,GxTex_Argb8888,GxTex_Argb8888,CGxTexFlags(GxTex_Nearest),depth[band].data(),UploadDepth,"WMO shadow reference",cleanup.textures[2+band]);
        GxRsSet(EGxRenderState(GxRs_Texture4+band),cleanup.textures[2+band]);
    }
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_ScissorTest, 0);
    GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PolygonOffset, 0);
    GxRsSet(GxRs_PointSprite, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxRsSet(GxRs_AlphaRef, 0);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    CreateShadowComparisonDepths(device,pass,cleanup.textures[0],cleanup.textures+6);
    std::array<V, 236> c{};
    c[2] = {0.85f, 0.1f, 0, 0.02f};
    c[3] = {0, 0.8f, 0, -0.01f};
    c[4] = {0, 0, 1, 0.6f};
    c[5] = {0, 0, 0, 1};
    c[31] = {0.9f, 0.1f, 0, 0.1f};
    c[32] = {-0.1f, 0.85f, 0.1f, 0};
    c[33] = {0, 0.1f, 0.8f, -0.4f};
    c[10] = {0.1f, 0.2f, 0.08f, 0};
    c[11] = {0.3f, 0.4f, 0.25f, 0};
    c[12] = {-0.1f, -0.2f, -0.85f, 0};
    c[13] = {0.4f, 0.2f, 0.1f, 4};
    c[28] = {0.4f, 0.3f, 0.5f, 1};
    c[29] = {0.03f, 0.01f, 0.05f, 0};
    c[30] = {-0.25f, 0.65f, 1.5f, 0};
    for (unsigned i = 0; i < 4; ++i) {
        c[17 + i] = {0.04f + i * 0.03f, 0.1f - i * 0.01f, 0.03f + i * 0.02f, 0};
        c[21 + i] = {-0.2f + i * 0.2f, 0.1f, 0.5f + i * 0.1f, 0};
        c[25][i] = 0.7f;
        c[26][i] = 0.1f;
        c[27][i] = 0.05f;
    }
    const V point{0.1f, -0.2f, 0.3f, 1}, normal{0.3f, 0.4f, 0.85f, 0}, color = Color(0xB0604050);
    V view{}, direction{};
    for (unsigned j = 0; j < 3; ++j) {
        view[j] = Dot(c[31 + j], point) + c[31 + j][3];
        direction[j] = Dot(c[31 + j], normal);
    }
    view[3] = 1;
    direction = Unit(direction);
    V clip{};
    for (unsigned j = 0; j < 4; ++j)
        clip[j] = Dot(c[2 + j], view) + c[2 + j][3];
    unsigned x = unsigned((clip[0] / clip[3] + 1) * 16), y = unsigned((1 - clip[1] / clip[3]) * 16);
    const V fog{0.12f, 0.22f, 0.32f, 0};
    const float visibility = std::min(std::pow(std::max(view[2] * c[30][0] + c[30][1], 0.0f), c[30][2]), 1.0f);
    unsigned draws = 0;
    for (unsigned family = 0; family < 7; ++family)
        for (unsigned vp = 0; vp < 10; ++vp) {
            const bool environment = family == 2 || family == 5, composite = family == 6;
            const unsigned materials[2] = {environment                  ? 4u
                                           : composite                  ? 6u
                                           : family == 1 || family == 4 ? 2u
                                                                        : 0u,
                                           environment                  ? 5u
                                           : composite                  ? 6u
                                           : family == 1 || family == 4 ? 3u
                                                                        : 1u};
            auto format = composite ? GxVBF_PNC2T2 : GxVBF_PNCT;
            auto *vertices = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 1);
            auto *bytes = reinterpret_cast<uint8_t *>(device.BufLock(vertices));
            std::memset(bytes, 0, GxVertexSize(format));
            auto put = [&](EGxVertexAttrib a, const void *data, size_t size) {
                std::memcpy(bytes + GxVertexAttribOffset(format, a), data, size);
            };
            const uint32_t packed = 0xB0604050, extra = 0x60305080;
            const float uv[2] = {0.7f, 0.2f}, uv2[2] = {-0.2f, 1.2f};
            put(GxVA_Position, point.data(), 12);
            put(GxVA_Normal, normal.data(), 12);
            put(GxVA_Color0, &packed, 4);
            put(GxVA_TexCoord0, uv, 8);
            if (composite) {
                put(GxVA_Color1, &extra, 4);
                put(GxVA_TexCoord1, uv2, 8);
            }
            device.BufUnlock(vertices, 0);
            GxPrimVertexPtr(vertices, format);
            V lighting = color;
            if (vp & 1) {
                V incident = c[12];
                for (unsigned j = 0; j < 3; ++j)
                    incident[j] = -incident[j];
                for (unsigned j = 0; j < 3; ++j)
                    lighting[j] = c[10][j] + Sat(Dot(incident, direction)) * c[11][j];
                for (unsigned i = 0; i < vp / 2; ++i) {
                    V delta{};
                    for (unsigned j = 0; j < 3; ++j)
                        delta[j] = c[21 + i][j] - view[j];
                    float length = std::sqrt(Dot(delta, delta));
                    float weight =
                        std::max(Dot(delta, direction) / length, 0.0f) / (c[25][i] + c[26][i] * length + c[27][i] * length * length);
                    for (unsigned j = 0; j < 3; ++j)
                        lighting[j] += c[17 + i][j] * weight;
                }
                for (unsigned j = 0; j < 3; ++j)
                    lighting[j] = Sat((family >= 3 ? c[28][j] * Sat(lighting[j]) + color[j] : color[j] * Sat(lighting[j])) + c[29][j]);
            }
            V second = Sample(images[1], uv2[0], uv2[1], true);
            if (environment) {
                auto directionToPoint = Unit(view);
                float projection = Dot(directionToPoint, direction);
                second = Sample(images[1], directionToPoint[0] - 2 * projection * direction[0],
                                directionToPoint[1] - 2 * projection * direction[1], true);
            }
            for (bool comparison : {false,true})
            for (unsigned shadowMode = comparison ? 1:0; shadowMode < 4; ++shadowMode)
            for (unsigned shadowCase = shadowMode ? 1:0; shadowCase < (shadowMode >= 2 ? 10u:shadowMode ? 6u:1u); ++shadowCase) {
                for(unsigned band=0;band<4;++band) GxRsSet(EGxRenderState(GxRs_Texture4+band),cleanup.textures[(comparison ? 6:2)+band]);
                c[224]={.2,.3,0,shadowCase==5 ? .96f:.1f}; c[225]={-.1,.2,0,-.2};
                c[226]={0,0,0,shadowCase>=6 ? 0:shadowCase==1 ? 128.0f/255:1};
                for(unsigned band=0;band<3;++band) {
                    const bool selected=shadowCase==6+band || (shadowCase==9 && band==2);
                    c[227+band*3]={selected && shadowCase==6 ? .1f:0,0,0,selected ? shadowCase==9 ? -.95f:0:1};
                    c[228+band*3]={0,selected && shadowCase==6 ? .2f:0,0,0};
                    c[229+band*3]={0,0,0,shadowCase==9 ? 1:128.0f/255};
                }
                std::array<V,10> shadowConstants{};
                shadowConstants[0]={0,0,0,shadowCase==3 ? 1.0f:0.0f};
                shadowConstants[1]=shadowCase==4 ? V{0,0,0,0}:direction;
                const V offsets[]={{-.2,0,0,0},{-.2,-.2,0,0},{0,-.2,0,0},{.2,-.2,0,0},
                    {.2,0,0,0},{.2,.2,0,0},{0,.2,0,0},{-.2,.2,0,0}};
                std::copy_n(offsets,8,shadowConstants.begin()+2);
                const float sx=Dot(c[224],view)+c[224][3],sy=Dot(c[225],view)+c[225][3];
                auto sample=[&](float cx,float cy,float cz,unsigned band,float dx,float dy) {
                    const float x=(cx*.5f+.5f+dx)*5, y=(cy*.5f+.5f+dy)*5;
                    auto compare=[&](int x,int y) { return Color(depth[band][std::clamp(y,0,4)*5+std::clamp(x,0,4)])[0]>=cz ? 1.0f:0.0f; };
                    if(!comparison) return compare(int(std::floor(x)),int(std::floor(y)));
                    const int left=int(std::floor(x-.5f)), top=int(std::floor(y-.5f));
                    const float fx=x-.5f-left, fy=y-.5f-top;
                    return (compare(left,top)*(1-fx)+compare(left+1,top)*fx)*(1-fy)+
                        (compare(left,top+1)*(1-fx)+compare(left+1,top+1)*fx)*fy;
                };
                const float border=Sat(std::max(std::abs(sx),std::abs(sy))*(-3.4482758045196533f)+3.4137930870056152f);
                float shadowVisibility=1;
                if(border>(shadowMode==3 ? 0:.01f)) {
                    float taps=sample(sx,sy,c[226][3],0,0,0); for(unsigned i=0;i<8;i+=2) taps+=sample(sx,sy,c[226][3],0,offsets[i][0],offsets[i][1]);
                    shadowVisibility=shadowMode==3 ? taps/5:std::min(1.0f,1+border*(taps/5-1));
                }
                shadowVisibility+=shadowConstants[0][3]*(1-shadowVisibility);
                if(shadowMode>=2) for(unsigned band=0;band<3;++band) {
                    const float bx=Dot(c[227+band*3],view)+c[227+band*3][3];
                    const float by=Dot(c[228+band*3],view)+c[228+band*3][3];
                    const float bz=Dot(c[229+band*3],view)+c[229+band*3][3];
                    const float edge=std::max(std::abs(bx),std::abs(by));
                    if(band<2 && edge>=1) continue;
                    float taps=sample(bx,by,bz,band+1,0,0);
                    for(unsigned i=0;i<8;i+=2) taps+=sample(bx,by,bz,band+1,offsets[i][0],offsets[i][1]);
                    const float distant=band==2 ? 1+Sat(edge*(-11.11111068725586f)+11)*(taps/5-1):taps/5;
                    shadowVisibility=std::min(shadowVisibility,distant); break;
                }
                const float normalFade=Sat(std::pow(1.2f-std::abs(Dot(direction,shadowConstants[1])),4));
                shadowVisibility+=normalFade*(1-shadowVisibility);
                const float modulation=shadowMode ? .7f+.3f*shadowVisibility:1;
                for (unsigned choice = 0; choice < (composite ? 1u : 2u); ++choice)
                    for (unsigned alpha = 0; alpha < 3; ++alpha) {
                        const unsigned material = materials[choice];
                        V texel = Sample(images[0], uv[0], uv[1], false), expected{};
                        if (composite)
                            for (unsigned j = 0; j < 4; ++j)
                                texel[j] = texel[j] * (96.0f / 255) + second[j] * (159.0f / 255);
                        expected[3] = (material == 0 || composite) ? texel[3] * lighting[3] : lighting[3];
                        for (unsigned j = 0; j < 3; ++j) {
                            float rgb = 2 * texel[j] * lighting[j] * modulation;
                            if (environment)
                                rgb += texel[3] * second[j] * (material == 5 ? texel[j] : 1);
                            expected[j] = Sat(fog[j] + visibility * (rgb - fog[j]));
                        }
                        V fogAlpha = fog;
                        fogAlpha[3] = alpha == 2 ? 0.9f : 0.05f;
                        if (alpha == 2)
                            expected = {0, 0, 0, 0};
                        device.BeginFrame(pass);
                        GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 236);
                        GxShaderConstantsSet(GxSh_Pixel, 2, fogAlpha.data(), 1);
                        GxShaderConstantsSet(GxSh_Pixel, 3, shadowConstants[0].data(), 10);
                        // 10/20 are identical rigid world aliases, not skinning variants.
                        device.RsSet(GxRs_VertexShader, vs[family][vp + 10 * alpha + 30*std::min(shadowMode,2u)]);
                        device.RsSet(GxRs_PixelShader, ps[material][(alpha ? 8:0) + shadowMode + (comparison ? 4:0)]);
                        CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
                        GxDraw(&batch, 0);
                        CiRect rect(0, 0, 32, 32);
                        TSGrowableArray<CImVector> pixels;
                        device.DeviceReadPixels(rect, pixels);
                        const auto &pixel = pixels[y * 32 + x];
                        const uint8_t actual[] = {pixel.r, pixel.g, pixel.b, pixel.a};
                        for (unsigned j = 0; j < 4; ++j)
                            if (std::abs(int(actual[j]) - int(std::lround(expected[j] * 255))) > 2)
                                throw std::runtime_error("WMO numeric mismatch family " + std::to_string(family) + " VS " + std::to_string(vp) +
                                                         " PCF " + std::to_string(comparison) + " material " + std::to_string(material) + " alpha " + std::to_string(alpha) + " shadow " + std::to_string(shadowMode) + "/" + std::to_string(shadowCase) +
                                                         " channel " + std::to_string(j) + " actual " + std::to_string(actual[j]) +
                                                         " expected " + std::to_string(expected[j] * 255));
                        unsigned covered = 0;
                        for (unsigned i = 0; i < pixels.Count(); ++i)
                            covered += pixels[i].a != 0;
                        if (covered != (alpha == 2 ? 0u : 1u))
                            throw std::runtime_error("WMO transform or alpha discard coverage mismatch");
                        device.ScenePresent();
                        ++draws;
                    }
            }
        }
    std::fprintf(
        stderr,
        "WMO native materials: 7 vertex families, 7 pixel families, %u numeric light/color/reflection/composite/fog/alpha/R32F-PCF-shadow draws passed\n",
        draws);
}
