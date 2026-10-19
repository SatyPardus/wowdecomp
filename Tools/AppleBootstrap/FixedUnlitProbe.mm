#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {
using Color = std::array<float, 4>;
Color Unpack(uint32_t p) { return {float((p >> 16) & 255) / 255, float((p >> 8) & 255) / 255, float(p & 255) / 255, float(p >> 24) / 255}; }
void Upload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    if (command == GxTex_Latch) { stride = 8; data = context; }
}
float Operation(unsigned op, float texture, float current, float previousAlpha, float diffuseAlpha) {
    switch (op) {
        case 0: return texture * current;
        case 1: return texture * current * 2;
        case 2: return texture + current;
        case 3: return current;
        case 4: return previousAlpha * current + (1 - previousAlpha) * texture;
        case 5: return diffuseAlpha * texture + (1 - diffuseAlpha) * current;
    }
    throw std::runtime_error("Invalid fixed-function reference operation");
}
Color Stage(Color current, Color texture, unsigned rgb, unsigned alpha, float diffuseAlpha) {
    Color result;
    for (unsigned c = 0; c < 4; ++c)
        result[c] = std::clamp(Operation(c == 3 ? alpha : rgb, texture[c], current[c], current[3], diffuseAlpha), 0.0f, 1.0f);
    return result;
}
}
void CheckFixedUnlit(CGxDeviceMetal& device) {
    device.RsPush();
    const auto projection = device.m_projNative;
    for (auto transform : {GxXform_View, GxXform_World}) device.XformPushAndSet(transform, C44Matrix(1));
    for (unsigned i = 0; i < 4; ++i) device.XformPushAndSet(EGxXform(i), C44Matrix(1));
    struct Cleanup {
        CGxDeviceMetal& device; C44Matrix projection; CGxTex* textures[6]{};
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop();
            for (auto& texture : textures) device.TexDestroy(texture);
            for (unsigned i = 0; i < 4; ++i) GxXformPop(EGxXform(i));
            GxXformPop(GxXform_World); GxXformPop(GxXform_View);
            device.m_projNative = projection;
        }
    } cleanup{device, projection};
    std::array<std::array<uint32_t, 4>, 4> texels{{{0x806090C0,0x40D030A0,0xC030E020,0xFF809050},
        {0xB0D08040,0x70603090,0x5090B060,0xFF304080}, {0x609030B0,0xFFA09020,0xFF204060,0xFF104080},
        {0xE0B06020,0xFF10A0F0,0xFF809010,0xFF708040}}};
    for (unsigned i = 0; i < 4; ++i) {
        device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest),
                         texels[i].data(), Upload, "Fixed-function reference", cleanup.textures[i]);
        device.RsSet(EGxRenderState(GxRs_Texture0 + i), static_cast<CGxTex*>(nullptr));
        GxRsSet(EGxRenderState(GxRs_TexGen0 + i), 0);
        GxRsSet(EGxRenderState(GxRs_TextureShader0 + i), 0);
        GxRsSet(EGxRenderState(GxRs_TextureCoord0 + i), int(i & 1));
    }
    device.RsSet(GxRs_VertexShader, static_cast<CGxShader*>(nullptr));
    device.RsSet(GxRs_PixelShader, static_cast<CGxShader*>(nullptr));
    for (auto state : {GxRs_Lighting, GxRs_Fog, GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_ScissorTest,
                       GxRs_ClipPlaneMask, GxRs_PolygonOffset, GxRs_PointSprite, GxRs_AlphaRef}) GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15); GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxRsSet(GxRs_MatDiffuse, -1); GxRsSet(GxRs_MatEmissive, 0);
    device.m_projNative = C44Matrix(1);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 1;
    C3Vector positions[4]{{-1,1,0.5f},{1,1,0.5f},{-1,-1,0.5f},{1,-1,0.5f}};
    CImVector color(160, 80, 112, 128);
    const Color diffuse = Unpack(0x807050A0);
    const C2Vector uv0(0.25f, 0.25f), uv1(0.75f, 0.25f);
    CGxBatch batch; batch.m_primType = GxPrim_TriangleStrip; batch.m_count = 4; batch.m_start = batch.m_minIndex = 0; batch.m_maxIndex = 3;
    unsigned draws = 0;
    auto draw = [&](Color expected, bool vertexColor = true, float expectedDepth = -1) {
        device.BeginFrame(pass);
        GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, vertexColor ? &color : nullptr, 0, &uv0, 0, &uv1, 0);
        device.Draw(&batch, 0);
        CiRect rect(0, 0, 16, 16); TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        if (expectedDepth >= 0) {
            TSGrowableArray<float> depth; device.DeviceReadDepths(rect, depth);
            if (std::abs(depth[8 * 16 + 8] - expectedDepth) > 1e-6f)
                throw std::runtime_error("Fixed-function normalized depth mismatch at draw " + std::to_string(draws));
        }
        device.ScenePresent();
        const auto& actual = pixels[8 * 16 + 8];
        const unsigned channels[]{actual.r, actual.g, actual.b, actual.a};
        for (unsigned c = 0; c < 4; ++c) if (std::abs(int(channels[c]) - int(std::lround(expected[c] * 255))) > 1)
            throw std::runtime_error("Fixed-function numerical mismatch at draw " + std::to_string(draws) + " channel " + std::to_string(c));
        ++draws;
    };
    draw(diffuse);
    // Stage one must remain disabled when stage zero is absent.
    device.RsSet(GxRs_Texture1, cleanup.textures[1]); draw(diffuse);
    device.RsSet(GxRs_Texture0, cleanup.textures[0]);
    for (unsigned rgb = 0; rgb < 6; ++rgb) for (unsigned alpha = 0; alpha < 6; ++alpha) {
        GxRsSet(GxRs_ColorOp0, int(rgb)); GxRsSet(GxRs_AlphaOp0, int(alpha));
        GxRsSet(GxRs_ColorOp1, int(alpha)); GxRsSet(GxRs_AlphaOp1, int(rgb));
        draw(Stage(Stage(diffuse, Unpack(texels[0][0]), rgb, alpha, diffuse[3]), Unpack(texels[1][1]), alpha, rgb, diffuse[3]));
    }
    Color expected = diffuse;
    const unsigned operations[4]{0,2,4,5};
    for (unsigned i = 0; i < 4; ++i) {
        device.RsSet(EGxRenderState(GxRs_Texture0+i), cleanup.textures[i]);
        GxRsSet(EGxRenderState(GxRs_ColorOp0+i), int(operations[i])); GxRsSet(EGxRenderState(GxRs_AlphaOp0+i), int(operations[3-i]));
        expected = Stage(expected, Unpack(texels[i][i&1]), operations[i], operations[3-i], diffuse[3]);
    }
    draw(expected);
    GxRsSet(GxRs_PolygonOffset, 0.125f); draw(expected);
    GxRsSet(GxRs_PolygonOffset, 0.0f); draw(expected);
    for (unsigned i = 1; i < 4; ++i) device.RsSet(EGxRenderState(GxRs_Texture0+i), static_cast<CGxTex*>(nullptr));
    GxRsSet(GxRs_ColorOp0, 0); GxRsSet(GxRs_AlphaOp0, 0);
    // Affine translation must use Gx's homogeneous coordinate placement.
    GxRsSet(GxRs_TextureShader0, 1);
    device.m_xforms[0].m_mtx[device.m_xforms[0].m_level].d0 = 0.5f;
    draw(Stage(diffuse, Unpack(texels[0][1]), 0, 0, diffuse[3]));
    // Project after raster interpolation, as D3DTTFF_COUNT3|PROJECTED does.
    GxRsSet(GxRs_TextureShader0, 2);
    device.m_xforms[0].m_mtx[device.m_xforms[0].m_level] = C44Matrix(1);
    device.m_xforms[0].m_mtx[device.m_xforms[0].m_level].c2 = 0.5f;
    draw(Stage(diffuse, Unpack(texels[0][3]), 0, 0, diffuse[3]));
    device.RsSet(GxRs_Texture0, static_cast<CGxTex*>(nullptr));
    GxRsSet(GxRs_MatDiffuse, int32_t(0x70503020)); GxRsSet(GxRs_MatEmissive, int32_t(0x00301008));
    draw(Unpack(0x70804028), false);
    // Fog changes RGB only; alpha reference equality still passes.
    GxRsSet(GxRs_Fog, 1); GxRsSet(GxRs_FogStart, 0.25f); GxRsSet(GxRs_FogEnd, 0.75f);
    GxRsSet(GxRs_FogColor, int32_t(0xFF20C040));
    expected = diffuse; const auto fog = Unpack(0xFF20C040);
    for (unsigned c = 0; c < 3; ++c) expected[c] = (diffuse[c]+fog[c])*0.5f;
    GxRsSet(GxRs_AlphaRef, 128); draw(expected);
    GxRsSet(GxRs_AlphaRef, 129); draw({0,0,0,0});
    GxRsSet(GxRs_AlphaRef, 0);
    for (auto& position : positions) position.z = 0.9f;
    expected = fog; expected[3] = diffuse[3]; draw(expected);
    for (auto& position : positions) position.z = 0.1f;
    draw(diffuse);
    positions[1].z = positions[3].z = 0.9f;
    const float factor = 1 - 8.5f / 16;
    expected = diffuse;
    for (unsigned c = 0; c < 3; ++c) expected[c] = factor * diffuse[c] + (1 - factor) * fog[c];
    draw(expected); // Clamp at vertices before interpolation, not at the pixel.
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_DepthWrite, 1);
    for (float z : {0.05f, 0.5f, 0.95f}) for (float offset : {-2.0f, -0.05f, 0.0f, 0.05f, 2.0f}) {
        for (auto& position : positions) position.z = z;
        GxRsSet(GxRs_PolygonOffset, offset);
        draw(diffuse, true, std::clamp(z - offset, 0.0f, 1.0f));
    }
    // The normalized bias is applied after viewport depth mapping.
    GxXformSetViewport(0, 1, 0, 1, 0.2f, 0.8f);
    for (auto& position : positions) position.z = 0.25f;
    GxRsSet(GxRs_PolygonOffset, 0.125f); draw(diffuse, true, 0.225f);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    // It must not move primitives across the original near/far clip planes.
    for (float z : {-0.1f, 1.1f}) {
        for (auto& position : positions) position.z = z;
        GxRsSet(GxRs_PolygonOffset, z < 0 ? -0.5f : 0.5f);
        draw({0,0,0,0}, true, 1);
    }
    for (auto& position : positions) position.z = 0.5f;
    pass.depthAttachment.clearDepth = 0.5;
    GxRsSet(GxRs_DepthTest, 1); GxRsSet(GxRs_DepthFunc, 3); GxRsSet(GxRs_DepthWrite, 0);
    for (float offset : {-0.05f, 0.0f, 0.05f}) {
        GxRsSet(GxRs_PolygonOffset, offset);
        draw(offset > 0 ? diffuse : Color{0,0,0,0}, true, 0.5f);
    }
    GxRsSet(GxRs_DepthWrite, 1); GxRsSet(GxRs_AlphaRef, 128); draw(diffuse, true, 0.45f);
    GxRsSet(GxRs_AlphaRef, 129); draw({0,0,0,0}, true, 0.5f);
    GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_DepthTest, 0); pass.depthAttachment.clearDepth = 1;
    // A slope must not scale a constant offset, unlike slope-scaled bias.
    positions[0].z = positions[2].z = 0.1f; positions[1].z = positions[3].z = 0.9f;
    draw(diffuse, true, 0.1f + (8.5f / 16) * 0.8f - 0.05f);
    for (auto& position : positions) position.z = 0.5f;
    device.BeginFrame(pass);
    for (float offset : {0.0f, 0.25f, 0.0f}) {
        GxRsSet(GxRs_PolygonOffset, offset);
        GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, &color, 0, &uv0, 0, &uv1, 0);
        device.Draw(&batch, 0); ++draws;
    }
    CiRect rect(0, 0, 16, 16); TSGrowableArray<float> depth;
    device.DeviceReadDepths(rect, depth); device.ScenePresent();
    if (std::abs(depth[8 * 16 + 8] - 0.5f) > 1e-6f) throw std::runtime_error("Fixed-function depth bias leaked across consecutive draws");
    GxRsSet(GxRs_PolygonOffset, 0.0f);
    // Generated coordinates must use position even when unrelated UVs exist.
    // Keep clip geometry unchanged while varying camera/model transforms.
    for (unsigned i = 0; i < 4; ++i) device.TexSetFlags(cleanup.textures[i], CGxTexFlags(GxTex_Linear));
    GxRsSet(GxRs_DepthWrite, 0);
    auto transform = [](const std::array<double,4>& v, const C44Matrix& m) {
        std::array<double,4> out{}; const float* a = &m.a0;
        for (unsigned c=0;c<4;++c) for (unsigned r=0;r<4;++r) out[c] += v[r] * a[r*4+c];
        return out;
    };
    auto sample = [&](unsigned image, double u, double v) {
        u = std::clamp(u*2-0.5, 0.0, 1.0); v = std::clamp(v*2-0.5, 0.0, 1.0);
        const auto a=Unpack(texels[image][0]), b=Unpack(texels[image][1]), c=Unpack(texels[image][2]), d=Unpack(texels[image][3]);
        Color out{};
        for (unsigned k=0;k<4;++k) out[k]=(1-v)*((1-u)*a[k]+u*b[k])+v*((1-u)*c[k]+u*d[k]);
        return out;
    };
    C44Matrix world(1), view(1), textureMatrix(1);
    world.a0=0.8f; world.a1=0.6f; world.b0=-0.6f; world.b1=0.8f; world.d0=0.37f; world.d1=-0.21f;
    view.a0=0; view.a1=-1; view.b0=1; view.b1=0; view.d0=0.11f; view.d1=0.43f;
    textureMatrix.a0=0.45f; textureMatrix.b0=0.12f; textureMatrix.c0=0.19f; textureMatrix.d0=0.42f;
    textureMatrix.a1=-0.17f; textureMatrix.b1=0.38f; textureMatrix.c1=0.09f; textureMatrix.d1=0.51f;
    textureMatrix.a2=0.13f; textureMatrix.b2=-0.07f; textureMatrix.c2=0.21f; textureMatrix.d2=0.86f;
    GxXformSet(GxXform_World, world);
    unsigned generatedSamples=0;
    for (unsigned camera=0;camera<2;++camera) {
        view.d0 += 0.19f; view.d1 -= 0.23f;
        device.XformSetView(view);
        device.m_projNative=(world*view).Inverse();
        for (unsigned gen=1;gen<=3;++gen) for (unsigned mode=0;mode<=2;++mode) for (bool withUV : {false,true}) {
            device.RsSet(GxRs_Texture0, cleanup.textures[0]);
            GxRsSet(GxRs_TexGen0, int(gen)); GxRsSet(GxRs_TextureShader0, int(mode));
            GxRsSet(GxRs_ColorOp0, 0); GxRsSet(GxRs_AlphaOp0, 0);
            GxXformSet(GxXform_Tex0, textureMatrix);
            device.BeginFrame(pass);
            GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, &color, 0, withUV ? &uv0 : nullptr, 0, nullptr, 0);
            device.Draw(&batch, 0);
            TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect,pixels); device.ScenePresent(); ++draws;
            for (unsigned y : {4u,8u,12u}) for (unsigned x : {4u,8u,12u}) {
                std::array<double,4> coord{(x+0.5)/8-1, 1-(y+0.5)/8, 0.5, 1};
                if (gen>=2) coord=transform(coord,world);
                if (gen==3) coord=transform(coord,view);
                if (mode) coord=transform(coord,textureMatrix);
                const double divisor=mode==2 ? coord[2] : 1;
                const auto want=Stage(diffuse,sample(0,coord[0]/divisor,coord[1]/divisor),0,0,diffuse[3]);
                const auto& p=pixels[y*16+x]; const unsigned got[]{p.r,p.g,p.b,p.a};
                for (unsigned c=0;c<4;++c) if (std::abs(int(got[c])-int(std::lround(want[c]*255)))>1)
                    throw std::runtime_error("Generated fixed-function coordinate mismatch: mode " + std::to_string(gen) + "/" + std::to_string(mode) + " sample " + std::to_string(generatedSamples));
                ++generatedSamples;
            }
        }
    }
    // Original render-state synchronization retains the inverse-model matrix
    // until TexGen is resent; moving only the world matrix must not refresh it.
    GxRsSet(GxRs_TexGen0, GxTexGen_ObjectLinear); device.IRsSync(0);
    const auto savedInverse = device.m_texGen[0].m_mtx[device.m_texGen[0].m_level];
    world.d0+=0.6f; GxXformSet(GxXform_World, world); device.IRsSync(0);
    const auto& retained = device.m_texGen[0].m_mtx[device.m_texGen[0].m_level];
    if (std::memcmp(&savedInverse,&retained,sizeof(C44Matrix))) throw std::runtime_error("Generated coordinates did not preserve original state timing");
    GxRsSet(GxRs_TexGen0, GxTexGen_Disabled);
    GxRsSet(GxRs_TextureShader0, 0); GxXformSet(GxXform_Tex0,C44Matrix(1));
    device.XformSetView(C44Matrix(1)); GxXformSet(GxXform_World,C44Matrix(1)); device.m_projNative=C44Matrix(1);
    draw(Stage(diffuse,Unpack(texels[0][0]),0,0,diffuse[3]));
    // Exercise the existing Gx light/material setters, independently evaluate
    // vertex lighting on the CPU, then interpolate before texture/specular/fog.
    std::array<CGxApiLight,4> savedLights;
    std::memcpy(savedLights.data(),device.m_lights,sizeof(device.m_lights));
    const bool savedMasterLighting=device.MasterEnable(GxMasterEnable_Lighting);
    struct RestoreLights {
        CGxDeviceMetal& device; std::array<CGxApiLight,4>& lights; bool master;
        ~RestoreLights() {
            std::memcpy(device.m_lights,lights.data(),sizeof(device.m_lights));
            device.MasterEnableSet(GxMasterEnable_Lighting,master);
        }
    } restoreLights{device,savedLights,savedMasterLighting};
    device.MasterEnableSet(GxMasterEnable_Lighting,1);
    CGxLight lights[4];
    for (unsigned i=0;i<4;++i) {
        auto& light=lights[i];
        light.m_flags=1 | ((i&1) ? 2 : 0);
        light.m_dir={.3f+float(i)*.1f,-.2f,.6f};
        if (!(i&1)) light.m_dir.z=-.7f;
        light.m_ambientColor={.06f,.04f,.08f};
        light.m_dirColor={.4f,.3f,.5f};
        light.m_specularColor={.3f,.5f,.2f};
        light.m_constantAttenuation=.4f; light.m_linearAttenuation=.2f; light.m_quadraticAttenuation=.03f;
        device.LightSet(i,light,C3Vector(0,0,0));
    }
    C3Vector normals[4]{{.25f,.1f,1.2f},{.6f,-.2f,1},{-.5f,.1f,.8f},{.2f,.3f,-.8f}};
    const auto material=Unpack(0xA0709060), emission=Unpack(0xFF080407), specularMaterial=Unpack(0xFF5A3651), fogColor=Unpack(0xFF182838);
    GxRsSet(GxRs_MatDiffuse,int32_t(0xA0709060)); GxRsSet(GxRs_MatEmissive,int32_t(0xFF080407));
    GxRsSet(GxRs_MatSpecular,int32_t(0xFF5A3651)); GxRsSet(GxRs_FogColor,int32_t(0xFF182838));
    GxRsSet(GxRs_FogStart,0.0f); GxRsSet(GxRs_FogEnd,8.0f);
    GxRsSet(GxRs_Lighting,1);
    auto unit=[](std::array<double,4> v) {
        const double length=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
        for(unsigned c=0;c<3;++c) v[c]=length>0 ? v[c]/length : 0;
        return v;
    };
    auto dot=[](const auto& a,const auto& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; };
    unsigned lightingSamples=0;
    for (unsigned transformed=0;transformed<2;++transformed) {
        world=C44Matrix(1); view=C44Matrix(1); world.d2=-3;
        if(transformed) {
            world.a0=2; world.b1=.5f; world.c2=1.5f; world.d0=.2f; world.d1=-.1f;
            view.a0=0; view.a1=-1; view.b0=1; view.b1=0; view.d0=.3f; view.d1=.2f; view.d2=-1;
        }
        device.XformSetView(view); GxXformSet(GxXform_World,world);
        device.m_projNative=(world*view).Inverse();
        const auto inverse=(world*view).Inverse();
        GxRsSet(GxRs_Fog,int(transformed));
        for(unsigned source=0;source<4;++source) for(bool normalize : {false,true})
        for(bool specular : {false,true}) for(unsigned mask : {0u,1u,10u,15u}) {
            const bool vertexColor=source<3;
            GxRsSet(GxRs_ColorMaterial,int(source%3)); GxRsSet(GxRs_NormalizeNormals,int(normalize));
            GxRsSet(GxRs_MatSpecularExp,specular ? 8.0f : 0.0f);
            for(unsigned i=0;i<4;++i) device.LightEnable(i,(mask>>i)&1);
            std::array<Color,4> vertexDiffuse{},vertexSpecular{};
            std::array<double,4> vertexFog{};
            for(unsigned vertex=0;vertex<4;++vertex) {
                const auto camera=transform(transform({positions[vertex].x,positions[vertex].y,positions[vertex].z,1},world),view);
                std::array<double,4> normal{};
                const double authored[]{normals[vertex].x,normals[vertex].y,normals[vertex].z};
                const float* inv=&inverse.a0;
                for(unsigned c=0;c<3;++c) for(unsigned k=0;k<3;++k) normal[c]+=authored[k]*inv[c*4+k];
                if(normalize) normal=unit(normal);
                const auto selected=vertexColor && source==0 ? diffuse : material;
                auto& d=vertexDiffuse[vertex]; d=vertexColor && source==2 ? diffuse : emission; d[3]=selected[3];
                auto& s=vertexSpecular[vertex];
                for(unsigned i=0;i<4;++i) if(mask&(1u<<i)) {
                    const auto& light=lights[i];
                    auto direction=transform({light.m_dir.x,light.m_dir.y,light.m_dir.z,double(i&1)},view);
                    double attenuation=1;
                    if(i&1) {
                        for(unsigned c=0;c<3;++c) direction[c]-=camera[c];
                        const double distance=std::sqrt(dot(direction,direction));
                        attenuation=distance>10000 ? 0 : 1/(light.m_constantAttenuation+light.m_linearAttenuation*distance+light.m_quadraticAttenuation*distance*distance);
                    } else for(unsigned c=0;c<3;++c) direction[c]=-direction[c];
                    direction=unit(direction);
                    auto halfway=unit({-camera[0],-camera[1],-camera[2],0});
                    for(unsigned c=0;c<3;++c) halfway[c]+=direction[c];
                    halfway=unit(halfway);
                    const double ndl=std::max(dot(normal,direction),0.0), highlight=specular ? std::pow(std::max(dot(normal,halfway),0.0),8) : 0;
                    const float ambient[]{light.m_ambientColor.x,light.m_ambientColor.y,light.m_ambientColor.z};
                    const float direct[]{light.m_dirColor.x,light.m_dirColor.y,light.m_dirColor.z};
                    const float spec[]{light.m_specularColor.x,light.m_specularColor.y,light.m_specularColor.z};
                    for(unsigned c=0;c<3;++c) {
                        d[c]+=selected[c]*(ambient[c]+direct[c]*ndl)*attenuation;
                        s[c]+=spec[c]*highlight*attenuation;
                    }
                }
                for(unsigned c=0;c<3;++c) {
                    d[c]=std::clamp(d[c],0.0f,1.0f);
                    s[c]=std::clamp(s[c]*(vertexColor && source==1 ? diffuse[c] : specularMaterial[c]),0.0f,1.0f);
                }
                vertexFog[vertex]=transformed ? std::clamp((8-std::abs(camera[2]))/8,0.0,1.0) : 1;
            }
            device.BeginFrame(pass);
            GxPrimVertexPtr(4,positions,sizeof(C3Vector),normals,sizeof(C3Vector),vertexColor ? &color : nullptr,0,&uv0,0,nullptr,0);
            device.Draw(&batch,0);
            TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect,pixels); device.ScenePresent(); ++draws;
            for(unsigned y : {3u,8u,12u}) for(unsigned x : {3u,8u,12u}) {
                const double u=(x+.5)/16,v=(y+.5)/16;
                const std::array<double,4> weights=u+v<=1 ? std::array<double,4>{1-u-v,u,v,0} : std::array<double,4>{0,1-v,1-u,u+v-1};
                Color d{},s{}; double fog=0;
                for(unsigned vertex=0;vertex<4;++vertex) {
                    for(unsigned c=0;c<4;++c) { d[c]+=weights[vertex]*vertexDiffuse[vertex][c]; s[c]+=weights[vertex]*vertexSpecular[vertex][c]; }
                    fog+=weights[vertex]*vertexFog[vertex];
                }
                auto want=Stage(d,Unpack(texels[0][0]),0,0,d[3]);
                for(unsigned c=0;c<3;++c) want[c]=std::clamp(want[c]+s[c],0.0f,1.0f)*fog+fogColor[c]*(1-fog);
                const auto& actual=pixels[y*16+x]; const unsigned got[]{actual.r,actual.g,actual.b,actual.a};
                for(unsigned c=0;c<4;++c) if(std::abs(int(got[c])-int(std::lround(want[c]*255)))>1)
                    throw std::runtime_error("Fixed lighting mismatch at sample "+std::to_string(lightingSamples)+", channel "+std::to_string(c)+": got "+std::to_string(got[c])+", expected "+std::to_string(std::lround(want[c]*255)));
                ++lightingSamples;
            }
        }
    }
    // Master disable bypasses materials; restoring lighting must not reuse its pipeline.
    device.MasterEnableSet(GxMasterEnable_Lighting,0); GxRsSet(GxRs_Fog,0);
    device.XformSetView(C44Matrix(1)); GxXformSet(GxXform_World,C44Matrix(1)); device.m_projNative=C44Matrix(1);
    draw(Stage(diffuse,Unpack(texels[0][0]),0,0,diffuse[3]));
    draw(Unpack(texels[0][0]),false);
    GxRsSet(GxRs_Lighting,0); GxRsSet(GxRs_MatDiffuse,-1); GxRsSet(GxRs_MatEmissive,0);
    draw(Stage(diffuse,Unpack(texels[0][0]),0,0,diffuse[3]));
    std::fprintf(stderr,"Fixed-function lighting: %u pixel references, four directional/point lights, material sources, local-viewer specular after texturing, fog, inverse-transpose normals, normalization and master disable passed\n",lightingSamples);
    using CubePixels=std::array<std::array<uint32_t,64>,6>;
    CubePixels gradient;
    for(unsigned face=0;face<6;++face) for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x)
        gradient[face][y*8+x]=((128+face*16)<<24)|((x*27+face*6)<<16)|((y*25+face*3)<<8)|(32+face*29+x+y);
    auto uploadGradient=[](EGxTexCommand cmd,uint32_t,uint32_t,uint32_t face,uint32_t,void* arg,uint32_t& stride,const void*& bytes) {
        if(cmd==GxTex_Latch) { stride=32; bytes=(*static_cast<CubePixels*>(arg))[face].data(); }
    };
    CGxTexFlags coordinateFlags(GxTex_Nearest); coordinateFlags.m_wrapU=coordinateFlags.m_wrapV=GxTex_Clamp;
    for(unsigned cube=0;cube<2;++cube)
        device.TexCreate(cube ? GxTex_CubeMap : GxTex_2d,8,8,0,GxTex_Argb8888,GxTex_Argb8888,coordinateFlags,&gradient,uploadGradient,"Generated normal/reflection reference",cleanup.textures[4+cube]);
    auto sampleGradient=[&](unsigned face,double u,double v,bool linear) {
        auto pixel=[&](int x,int y) { return Unpack(gradient[face][std::clamp(y,0,7)*8+std::clamp(x,0,7)]); };
        if(!linear) return pixel(int(std::floor(u*8)),int(std::floor(v*8)));
        const double sx=u*8-.5,sy=v*8-.5,fx=sx-std::floor(sx),fy=sy-std::floor(sy);
        Color result{};
        for(unsigned y=0;y<2;++y) for(unsigned x=0;x<2;++x) {
            const auto value=pixel(int(std::floor(sx))+x,int(std::floor(sy))+y);
            for(unsigned c=0;c<4;++c) result[c]+=value[c]*(x ? fx : 1-fx)*(y ? fy : 1-fy);
        }
        return result;
    };
    auto cubeGradient=[&](const std::array<double,4>& coord,bool linear) {
        const double x=coord[0],y=coord[1],z=coord[2],ax=std::abs(x),ay=std::abs(y),az=std::abs(z);
        unsigned face; double u,v,m;
        if(ax>=ay && ax>=az) { face=x>=0 ? 0 : 1; u=x>=0 ? -z : z; v=-y; m=ax; }
        else if(ay>=az) { face=y>=0 ? 2 : 3; u=x; v=y>=0 ? z : -z; m=ay; }
        else { face=z>=0 ? 4 : 5; u=z>=0 ? x : -x; v=-y; m=az; }
        return sampleGradient(face,m ? .5+.5*u/m : .5,m ? .5+.5*v/m : .5,linear);
    };
    auto setFilter=[](CGxTex* image,bool linear) {
        auto flags=image->m_flags; flags.m_filter=linear ? GxTex_Linear : GxTex_Nearest; GxTexSetFlags(image,flags);
    };
    unsigned normalSamples=0;
    auto drawCoordinates=[&](const std::array<std::array<double,4>,4>& reference,bool cube,bool linear,bool projected) {
        device.BeginFrame(pass);
        GxPrimVertexPtr(4,positions,sizeof(C3Vector),normals,sizeof(C3Vector),&color,0,nullptr,0,nullptr,0);
        device.Draw(&batch,0);
        TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect,pixels); device.ScenePresent(); ++draws;
        for(unsigned y : {2u,7u,11u}) for(unsigned x : {2u,7u,11u}) {
            const double u=(x+.5)/16,v=(y+.5)/16;
            const std::array<double,4> weights=u+v<=1 ? std::array<double,4>{1-u-v,u,v,0} : std::array<double,4>{0,1-v,1-u,u+v-1};
            std::array<double,4> coordinate{};
            for(unsigned vertex=0;vertex<4;++vertex) for(unsigned c=0;c<3;++c) coordinate[c]+=weights[vertex]*reference[vertex][c];
            const double divisor=projected ? coordinate[2] : 1;
            const auto texel=cube ? cubeGradient(coordinate,linear) : sampleGradient(0,coordinate[0]/divisor,coordinate[1]/divisor,linear);
            const auto want=Stage(diffuse,texel,0,0,diffuse[3]);
            const auto& p=pixels[y*16+x]; const unsigned got[]{p.r,p.g,p.b,p.a};
            for(unsigned c=0;c<4;++c) if(std::abs(int(got[c])-int(std::lround(want[c]*255)))>1)
                throw std::runtime_error("Generated normal/reflection mismatch at sample "+std::to_string(normalSamples)+", channel "+std::to_string(c)+": got "+std::to_string(got[c])+", expected "+std::to_string(std::lround(want[c]*255)));
            ++normalSamples;
        }
    };
    for(unsigned cameraIndex=0;cameraIndex<2;++cameraIndex) {
        world=C44Matrix(1); view=C44Matrix(1); world.d2=-3;
        if(cameraIndex) {
            world.a0=1.7f; world.b1=.6f; world.c2=1.3f; world.d0=.2f;
            view.a0=0; view.a1=-1; view.b0=1; view.b1=0; view.d1=.3f; view.d2=-1;
        }
        device.XformSetView(view); GxXformSet(GxXform_World,world); device.m_projNative=(world*view).Inverse();
        const auto inverse=(world*view).Inverse();
        for(auto gen : {GxTexGen_Normal,GxTexGen_ReflectionMap,GxTexGen_SphereMap}) for(bool normalize : {false,true})
        for(unsigned mode=0;mode<3;++mode) for(bool cube : {false,true}) for(bool linear : {false,true}) {
            if(cube && mode==2) continue; // D3D's projected COUNT3 supplies only two coordinates.
            GxRsSet(GxRs_TexGen0,gen); GxRsSet(GxRs_TextureShader0,int(mode)); GxRsSet(GxRs_NormalizeNormals,int(normalize));
            GxXformSet(GxXform_Tex0,textureMatrix);
            auto* image=cleanup.textures[cube ? 5 : 4]; setFilter(image,linear);
            device.RsSet(GxRs_Texture0,image);
            std::array<std::array<double,4>,4> reference{};
            for(unsigned vertex=0;vertex<4;++vertex) {
                auto& n=reference[vertex]; const float* inv=&inverse.a0;
                const double authored[]{normals[vertex].x,normals[vertex].y,normals[vertex].z};
                for(unsigned c=0;c<3;++c) for(unsigned k=0;k<3;++k) n[c]+=authored[k]*inv[c*4+k];
                if(normalize) n=unit(n);
                if(gen!=GxTexGen_Normal) {
                    auto eye=transform(transform({positions[vertex].x,positions[vertex].y,positions[vertex].z,1},world),view);
                    for(unsigned c=0;c<3;++c) eye[c]=-eye[c]; eye=unit(eye);
                    const double twice=2*dot(eye,n);
                    for(unsigned c=0;c<3;++c) n[c]=twice*n[c]-eye[c];
                }
                n[3]=1;
                if(gen==GxTexGen_SphereMap) { n[0]=n[0]*.5+.5; n[1]=n[1]*.5+.5; }
                if(mode) n=transform(n,textureMatrix);
            }
            drawCoordinates(reference,cube,linear,mode==2);
        }
    }
    // Exercise all six cube faces and each binding slot, mixed with 2D stages.
    const C3Vector directions[]{{1,.2f,-.3f},{-1,.2f,.3f},{.2f,1,.3f},{.2f,-1,-.3f},{.2f,.3f,1},{-.2f,.3f,-1}};
    device.XformSetView(C44Matrix(1)); GxXformSet(GxXform_World,C44Matrix(1)); device.m_projNative=C44Matrix(1);
    GxRsSet(GxRs_NormalizeNormals,0);
    for(unsigned slot=0;slot<4;++slot) for(const auto& direction : directions) for(bool linear : {false,true}) {
        std::array<std::array<double,4>,4> reference;
        for(unsigned i=0;i<4;++i) {
            normals[i]=direction; reference[i]={direction.x,direction.y,direction.z,1};
            device.RsSet(EGxRenderState(GxRs_Texture0+i),i>slot ? nullptr : cleanup.textures[i==slot ? 5 : 4]);
            GxRsSet(EGxRenderState(GxRs_TexGen0+i),i==slot ? GxTexGen_Normal : GxTexGen_Disabled);
            GxRsSet(EGxRenderState(GxRs_TextureShader0+i),0);
            GxRsSet(EGxRenderState(GxRs_ColorOp0+i),i==slot ? 0 : 3); GxRsSet(EGxRenderState(GxRs_AlphaOp0+i),i==slot ? 0 : 3);
        }
        setFilter(cleanup.textures[5],linear);
        drawCoordinates(reference,true,linear,false);
    }
    std::fprintf(stderr,"Generated normal/reflection coordinates: %u pixel references, camera/normal transforms, local-viewer reflection, original sphere matrix, normalization, projective 2D, six cube faces and all four mixed texture slots passed\n",normalSamples);
    std::fprintf(stderr,"Generated linear texture coordinates: %u independent pixel references, object/world/camera source, disabled/affine/projected transforms, missing UVs and view/state changes passed\n",generatedSamples);
    std::fprintf(stderr, "Native fixed-function: %u numerical draws, six independent RGB/alpha operations, four stages, disabled cascade, UV sets/transforms, material color, vertex fog, alpha boundary and normalized post-raster depth offset passed\n", draws);
}
