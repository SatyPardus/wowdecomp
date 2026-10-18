#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/DetailDoodad.h"
#include "WoW/Source/WorldClient/CDetailDoodadData.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Services/Texture.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {
using Vec = std::array<float, 4>;
Vec Transform(const std::array<Vec, 23>& c, unsigned first, Vec v) {
    Vec result{};
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned column = 0; column < 4; ++column)
            result[row] += c[first + column][row] * v[column];
    return result;
}
float Dot(Vec a, Vec b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
float Sat(float x) { return std::clamp(x, 0.0f, 1.0f); }
void Pattern(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texel = 0x806090C0;
    if (command == GxTex_Latch) {
        stride = 4;
        data = &texel;
    }
}
} // namespace

void CheckDetailDoodadShaders(CGxDeviceMetal& device) {
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device;
        CGxShader* vertex[6]{};
        CGxShader* pixel[8]{};
        CGxTex* texture = nullptr;
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            device.TexDestroy(texture);
            for (auto& shader : vertex)
                device.ShaderDestroy(shader);
            for (auto& shader : pixel)
                device.ShaderDestroy(shader);
        }
    } cleanup{device};
    device.ShaderCreate(cleanup.vertex, GxSh_Vertex, "Shaders\\Vertex", "DetailDoodad", 6);
    device.ShaderCreate(cleanup.pixel, GxSh_Pixel, "Shaders\\Pixel", "DetailDoodad", 8);
    device.TexCreate(GxTex_2d, 1, 1, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, Pattern,
                     "Ground decoration reference", cleanup.texture);
    device.RsSet(GxRs_Texture0, cleanup.texture);
    for (auto state : {GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_Fog, GxRs_ScissorTest, GxRs_ClipPlaneMask, GxRs_PolygonOffset,
                       GxRs_PointSprite})
        GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    std::array<Vec, 23> c{};
    c[0] = {0.7f, 0.1f, 0.05f, 0};
    c[1] = {0.05f, 1.2f, -0.07f, 0};
    c[2] = {-0.1f, 0.2f, 0.9f, 0};
    c[3] = {0.11f, -0.13f, 0.1f, 1};
    c[4] = {0.8f, 0.1f, 0, 0};
    c[5] = {-0.1f, 0.9f, 0, 0};
    c[6] = {0.1f, 0, 0.6f, 0.3f};
    c[7] = {0, 0, 0.2f, 1};
    c[8] = {-0.3f, 0.9f, 1.3f, 0};
    c[10] = {0.2f, 0.1f, 0.9f, 1};
    c[11] = {0.1f, 0.2f, 0.3f, 0};
    c[12] = {0.7f, 0.5f, 0.3f, 0};
    for (unsigned i = 0; i < 3; ++i) {
        c[14 + i * 3] = {-0.2f + 0.2f * i, 0.6f - 0.1f * i, 1.2f + 0.2f * i, 1};
        c[15 + i * 3] = {0.2f, 0.1f + 0.1f * i, 0.3f, 0};
        c[16 + i * 3] = {0.7f + 0.2f * i, 0.2f, 0.05f, 0};
    }
    const Vec texel{96.0f / 255, 144.0f / 255, 192.0f / 255, 128.0f / 255}, fogColor{0.12f, 0.22f, 0.32f, 0.9f};
    CGxBatch batch{GxPrim_Points, 0, 1, 0, 0};
    struct FadeCase {
        float fade;
        unsigned alphaRef;
    };
    unsigned draws = 0;
    for (unsigned lights : {0u, 1u})
        for (unsigned pixel : {0u, 4u})
            for (unsigned tint : {0u, 128u, 255u})
                for (const auto test :
                     {FadeCase{-0.5f, 0}, FadeCase{0.5f, 64}, FadeCase{0.5f, 65}, FadeCase{1.5f, 128}, FadeCase{1.5f, 129}}) {
                    CGxVertexPNCT source{};
                    source.m_position = C3Vector(0.1f, -0.15f, 0.4f);
                    source.m_normal = C3Vector(0.4f, -0.2f, tint == 255 ? 1.6f : -0.6f);
                    source.m_color = CImVector(192, 128, 160, tint);
                    source.m_texture = C2Vector(0.3f, 0.7f);
                    auto* buffer = GxBufStream(GxPoolTarget_Vertex, sizeof(source), 1);
                    std::memcpy(device.BufLock(buffer), &source, sizeof(source));
                    device.BufUnlock(buffer, 0);
                    GxPrimVertexPtr(buffer, GxVBF_PNCT);
                    const auto view = Transform(c, 0, {source.m_position.x, source.m_position.y, source.m_position.z, 1});
                    const auto clip = Transform(c, 4, view);
                    const auto normal = Transform(c, 0, {source.m_normal.x, source.m_normal.y, source.m_normal.z, 0});
                    Vec light{};
                    for (unsigned j = 0; j < 3; ++j)
                        light[j] = c[11][j] + c[12][j] * Sat(Dot(c[10], normal));
                    if (lights)
                        for (unsigned i = 0; i < 3; ++i) {
                            auto position = c[14 + i * 3];
                            position[3] = 0;
                            auto delta = Transform(c, 0, position);
                            for (unsigned j = 0; j < 3; ++j)
                                delta[j] -= view[j];
                            const float distance = std::sqrt(Dot(delta, delta));
                            const float diffuse = Sat(Dot(delta, normal) / distance);
                            const auto attenuation = c[16 + i * 3];
                            for (unsigned j = 0; j < 3; ++j)
                                light[j] += c[15 + i * 3][j] * diffuse /
                                            (attenuation[0] + attenuation[1] * distance + attenuation[2] * distance * distance);
                        }
                    // Use a depth-dependent expression, with this point at the chosen
                    // boundary, to distinguish distance fade from texture/vertex alpha.
                    c[9] = {-0.25f, test.fade + 0.25f * view[2], 0, 0};
                    const float fade = Sat(std::fma(view[2], c[9][0], c[9][1]));
                    const float fog = std::min(std::pow(std::max(std::fma(view[2], c[8][0], c[8][1]), 0.0f), c[8][2]), 1.0f);
                    const Vec color{160.0f / 255, 128.0f / 255, 192.0f / 255, float(tint) / 255};
                    Vec expected{};
                    expected[3] = texel[3] * fade;
                    for (unsigned j = 0; j < 3; ++j)
                        expected[j] = Sat(fog * (texel[j] * (color[3] * 0.3f + 0.7f) * std::min(light[j], 1.0f) * color[j] - fogColor[j]) +
                                          fogColor[j]);
                    const bool visible = expected[3] >= test.alphaRef / 255.0f;
                    if (!visible)
                        expected = {};
                    device.BeginFrame(pass);
                    device.ShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), c.size());
                    device.ShaderConstantsSet(GxSh_Pixel, 2, fogColor.data(), 1);
                    device.RsSet(GxRs_VertexShader, cleanup.vertex[lights * 3]);
                    device.RsSet(GxRs_PixelShader, cleanup.pixel[pixel]);
                    GxRsSet(GxRs_AlphaRef, int32_t(test.alphaRef));
                    device.Draw(&batch, 0);
                    CiRect rect(0, 0, 32, 32);
                    TSGrowableArray<CImVector> image;
                    device.DeviceReadPixels(rect, image);
                    const unsigned x = std::floor((clip[0] / clip[3] + 1) * 16), y = std::floor((1 - clip[1] / clip[3]) * 16);
                    const auto& sample = image[y * 32 + x];
                    const unsigned channels[] = {sample.r, sample.g, sample.b, sample.a};
                    for (unsigned j = 0; j < 4; ++j)
                        if (std::abs(int(channels[j]) - int(std::lround(expected[j] * 255))) > 2)
                            throw std::runtime_error("Detail-doodad light/tint/fog/fade/alpha reference mismatch");
                    unsigned covered = 0;
                    for (unsigned i = 0; i < image.Count(); ++i)
                        covered += image[i].r || image[i].g || image[i].b || image[i].a;
                    if (covered != unsigned(visible))
                        throw std::runtime_error("Detail-doodad point position or coverage changed");
                    device.ScenePresent();
                    ++draws;
                }
    std::fprintf(stderr, "Detail-doodad sun/three-light paths: %u independent tint/normal/fade/fog/alpha reference draws\n", draws);
}

void CheckGameDetailDoodads(CM2Scene* scene, CGxDeviceMetal& device, MTLRenderPassDescriptor* pass) {
    if (!g_groundEffectDoodadDB.m_loaded)
        g_groundEffectDoodadDB.Load(__FILE__, __LINE__);
    device.RsPush();
    struct Restore {
        CGxDeviceMetal& device;
        CM2Scene* scene = World::s_m2Scene;
        CMapLight* light = CMap::s_mapLight;
        uint32_t enables = CWorld::enables, enables2 = CWorld::enables2;
        int32_t alpha = CWorld::detailDoodadAlphaRef, fog = GxMasterEnable(GxMasterEnable_Fog);
        float distance = CWorld::detailDoodadDist;
        bool initialized = false;
        ~Restore() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            if (initialized)
                CDetailDoodad::Destroy();
            World::s_m2Scene = scene;
            CMap::s_mapLight = light;
            CWorld::enables = enables;
            CWorld::enables2 = enables2;
            CWorld::detailDoodadAlphaRef = alpha;
            CWorld::detailDoodadDist = distance;
            GxMasterEnableSet(GxMasterEnable_Fog, fog);
        }
    } restore{device};
    CMapLight light;
    light.m_light.m_type = M2LIGHT_DIRECT;
    light.m_light.m_visible = 1;
    light.m_light.m_dir = C3Vector(0, 0, -1);
    light.m_light.m_ambColor = C3Vector(0.4f, 0.4f, 0.4f);
    light.m_light.m_dirColor = C3Vector(0.6f, 0.6f, 0.6f);
    light.m_light.m_specColor = C3Vector(0, 0, 0);
    World::s_m2Scene = scene;
    CMap::s_mapLight = &light;
    CWorld::enables |= Enable_PixelShaders;
    CWorld::enables2 |= Enable_VertexShader;
    CWorld::detailDoodadAlphaRef = 64;
    CWorld::detailDoodadDist = 100;
    GxMasterEnableSet(GxMasterEnable_Fog, 0);
    CDetailDoodad::Initialize();
    restore.initialized = true;
    // Explicit preview placement and budget; generation and batching use the
    // original world routines, including both DBC orientation flag branches.
    CDetailDoodad::s_vtxBufItems = CDetailDoodad::s_idxBufItems = 4096;
    for (uint32_t id : {4u, 367u}) {
        CDetailDoodad::LoadModel(id);
        auto* data = CDetailDoodad::s_doodadList[id];
        if (!data || !data->m_model || !data->m_model->IsLoaded(1, 0) || !data->m_model->IsDrawable(1, 0) || !data->m_texture ||
            !TextureGetGxTex(data->m_texture, 1, nullptr))
            throw std::runtime_error("Original ground decoration or its texture did not load");
        const auto& bounds = data->m_model->m_shared->m_data->m_bounds;
        C44Matrix world(1);
        world.RotateAroundX(-0.65f);
        world.Scale(0.65f / bounds.m_radius);
        world.Translate((bounds.m_extent.b + bounds.m_extent.t) * -0.5f);
        CDetailDoodadInst instances;
        instances.AddDoodad(id, C3Vector(0, 0, 0), 0, 1, C3Vector(0, 0, 1), 0, CImVector(255, 255, 255, 255));
        instances.AddDoodad(id, C3Vector(bounds.m_radius * 0.3f, 0, 0), 0.4f, 0.8f, C3Vector(0.2f, 0.1f, 0.9746794f), 1,
                            CImVector(192, 224, 255, 64));
        unsigned batches = 0, additions = 0;
        for (auto& batch : instances.m_batches) {
            batches += batch.m_adds.Count() != 0;
            additions += batch.m_adds.Count();
        }
        if (batches != 1 || additions != 2)
            throw std::runtime_error("Original ground decoration texture batching changed");
        device.BeginFrame(pass);
        CDetailDoodad::InitializeRenderState();
        CDetailDoodad::SetupVertexShader(world, 0);
        const auto before = device.EncodingStats();
        instances.DrawBatches();
        CiRect rect(0, 0, 128, 128);
        TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        unsigned covered = 0;
        for (unsigned i = 0; i < pixels.Count(); ++i)
            covered += pixels[i].r || pixels[i].g || pixels[i].b;
        const auto draws = device.EncodingStats().geometryDraws - before.geometryDraws;
        std::fprintf(stderr, "Original ground decoration %u (%s, flags %u): %u instances, %llu draws, %u covered pixels\n", id,
                     data->m_record->m_doodadpath, data->m_record->m_flags, additions, static_cast<unsigned long long>(draws), covered);
        if (draws != batches || covered < 32)
            throw std::runtime_error("Original ground decoration produced no visible batch");
        device.ScenePresent();
    }
}
