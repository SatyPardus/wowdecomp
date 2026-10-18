#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/WaterRipple.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {
void RippleTexel(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
    static const uint32_t texel = 0x806090C0;
    if (command == GxTex_Latch) {
        stride = 4;
        data = &texel;
    }
}

} // namespace

void CheckGameWaterRipples(CMapArea& area) {
    // A real Elwynn MH2O layer is temporarily registered in the original map
    // lookup. Ripple creation therefore queries the authored liquid triangles.
    unsigned slot = 256;
    const SMLiquidInstance* record = nullptr;
    for (unsigned i = 0; area.m_liquid && i < 256 && !record; ++i) {
        const auto* desc = area.m_liquid->GetChunkDesc(i % 16, i / 16);
        for (unsigned j = 0; desc && j < desc->m_layerCount; ++j) {
            const auto* candidate = area.m_liquid->GetInstanceDesc(desc, j);
            if (candidate->m_liquidType == 5 && candidate->m_width == 8 && candidate->m_height == 8) {
                slot = i;
                record = candidate;
                break;
            }
        }
    }
    if (!record)
        throw std::runtime_error("Elwynn liquid receiver fixture is missing");
    CMapChunk owner;
    owner.LoadChunkData(static_cast<uint8_t*>(area.m_fileBuffer) + area.m_chunkInfo[slot].m_offset, area.m_chunkInfo[slot].m_size, false,
                        0);
    owner.m_cOffset = {int32_t(32 * 16 + slot % 16), int32_t(48 * 16 + slot / 16)};
    owner.m_topLeftCoords = {17066.666f - owner.m_cOffset.y * 33.333332f, 17066.666f - owner.m_cOffset.x * 33.333332f,
                             owner.m_header->m_position.z};
    CChunkLiquid liquid;
    liquid.Initialize(owner, *area.m_liquid, *record);
    C3Vector center;
    bool found = false;
    for (unsigned y = 2; y < 6 && !found; ++y)
        for (unsigned x = 2; x < 6; ++x)
            if (liquid.TileExists(x, y)) {
                const auto index = y * 9 + x;
                center = (liquid.m_verts[index] + liquid.m_verts[index + 10]) * 0.5f + owner.m_topLeftCoords;
                found = true;
                break;
            }
    if (!found)
        throw std::runtime_error("Elwynn liquid receiver has no interior tile");
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    CVar footstep;
    footstep.m_floatValue = 0.125f;
    device.RsPush();
    C44Matrix view(1);
    view.Scale(0.1f);
    device.XformPushAndSet(GxXform_View, view);
    struct Restore {
        CGxDeviceMetal& device;
        CMapArea& area;
        CChunkLiquid& liquid;
        unsigned slot;
        CMapArea* previousArea = CMap::m_areaTable[48 * 64 + 32];
        CMapChunk* previousChunk = area.m_mapChunks[slot];
        CVar* previousBias = CWorldParam::cvar_footstepBias;
        C3Vector camera = CWorldScene::s_activeWorldView;
        C44Matrix projection = device.m_projection;
        float time = FrameTime::s_curTimeSec, tick = FrameTime::s_tickTimeSec;
        bool initialized = false;
        ~Restore() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            GxXformPop(GxXform_View);
            device.XformSetProjection(projection);
            if (initialized)
                WaterRipple::Destroy();
            liquid.m_chunkLink.Unlink();
            area.m_mapChunks[slot] = previousChunk;
            CMap::m_areaTable[48 * 64 + 32] = previousArea;
            CWorldParam::cvar_footstepBias = previousBias;
            CWorldScene::s_activeWorldView = camera;
            FrameTime::s_curTimeSec = time;
            FrameTime::s_tickTimeSec = tick;
        }
    } restore{device, area, liquid, slot};
    CMap::m_areaTable[48 * 64 + 32] = &area;
    area.m_mapChunks[slot] = &owner;
    owner.m_liquidChunkLinkList.LinkToTail(&liquid);
    CWorldParam::cvar_footstepBias = &footstep;
    CWorldScene::s_activeWorldView = center;
    device.XformSetProjection(C44Matrix(1));
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    for (auto state : {GxRs_ScissorTest, GxRs_ClipPlaneMask, GxRs_PointSprite, GxRs_PolygonOffset})
        GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_DepthTest, 1);
    GxRsSet(GxRs_DepthFunc, 0);
    WaterRipple::Initialize();
    restore.initialized = true;
    for (const char* name : {"XTextures\\splash\\splash.blp", "XTextures\\splash\\wake.blp"}) {
        CStatus status;
        auto* texture = TextureCreate(name, CGxTexFlags(), &status, 1);
        const bool loaded = texture && TextureGetGxTex(texture, 1, &status);
        if (!loaded) {
            char message[1024];
            std::fprintf(stderr, "Ripple texture %s: %s\n", name, status.GetErrorStr(message, sizeof(message), STATUS_INFO, "; "));
        }
        if (texture)
            HandleClose(reinterpret_cast<HOBJECT>(texture));
        if (!loaded)
            throw std::runtime_error("Original ripple texture did not become ready");
    }
    FrameTime::s_curTimeSec = 10;
    for (int type : {0, 1}) {
        WaterRipple::Create(center, 0.3f * type, 4, 1, 2, 0.8f, type, type);
        if (!World::TriData::nBatches)
            throw std::runtime_error("Original ripple creation found no real liquid triangles");
    }
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:128 height:128 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 0.5;
    TSGrowableArray<CImVector> first;
    unsigned frame = 0;
    for (float tick : {0.2f, 0.4f, 0.6f, 1.0f}) {
        FrameTime::s_tickTimeSec = tick;
        FrameTime::s_curTimeSec += tick;
        device.BeginFrame(pass);
        const auto before = device.EncodingStats();
        CMap::RenderWaterRipples();
        CiRect rect(0, 0, 128, 128);
        TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        unsigned covered = 0, changed = 0;
        for (unsigned i = 0; i < pixels.Count(); ++i) {
            covered += pixels[i].r || pixels[i].g || pixels[i].b;
            if (first.Count())
                changed += std::memcmp(&first[i], &pixels[i], sizeof(CImVector)) != 0;
        }
        const auto draws = device.EncodingStats().geometryDraws - before.geometryDraws;
        std::fprintf(stderr, "Original Elwynn ripple/wake frame %u: %llu draws, %u covered, %u changed pixels\n", frame,
                     static_cast<unsigned long long>(draws), covered, changed);
        if (frame < 3 ? (draws != 2 || covered < 32 || (frame && changed < 32)) : (draws != 0 || covered))
            throw std::runtime_error("Original ripple/wake batching, animation or expiry failed");
        if (!frame) {
            first.SetCount(pixels.Count());
            std::memcpy(first.m_data, pixels.m_data, pixels.Count() * sizeof(CImVector));
        }
        device.ScenePresent();
        ++frame;
    }
}

void CheckWaterRippleShaders(CGxDeviceMetal& device) {
    device.RsPush();
    struct Cleanup {
        CGxDeviceMetal& device;
        CGxShader *vertex = nullptr, *pixel = nullptr;
        CGxTex* texture = nullptr;
        ~Cleanup() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.RsPop();
            device.TexDestroy(texture);
            device.ShaderDestroy(vertex);
            device.ShaderDestroy(pixel);
        }
    } cleanup{device};
    device.ShaderCreate(&cleanup.vertex, GxSh_Vertex, "Shaders\\Vertex", "WaterRipples", 1);
    device.ShaderCreate(&cleanup.pixel, GxSh_Pixel, "Shaders\\Pixel", "WaterRipples", 1);
    device.TexCreate(GxTex_2d, 1, 1, 0, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Nearest), nullptr, RippleTexel,
                     "Water ripple reference", cleanup.texture);
    device.RsSet(GxRs_Texture0, cleanup.texture);
    device.RsSet(GxRs_VertexShader, cleanup.vertex);
    device.RsSet(GxRs_PixelShader, cleanup.pixel);
    for (auto state : {GxRs_DepthTest, GxRs_Culling, GxRs_Fog, GxRs_ScissorTest, GxRs_ClipPlaneMask, GxRs_PointSprite})
        GxRsSet(state, 0);
    GxRsSet(GxRs_DepthWrite, 1);
    GxRsSet(GxRs_ColorWrite, 15);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:32 height:32 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 1;
    C44Matrix identity(1);
    device.ShaderConstantsSet(GxSh_Vertex, 0, &identity.a0, 4);
    unsigned draws = 0;
    for (float z : {0.0625f, 0.5f, 0.9375f})
        for (float offset : {-0.125f, 0.0f, 0.125f})
            for (int alphaRef : {0, 128, 129}) {
                // Nonwhite vertex RGB must have no effect. Alpha equality passes.
                CGxVertexPCT vertices[] = {{{-1, -1, z}, CImVector(17, 49, 93, 255), {0, 0}},
                                           {{-1, 3, z}, CImVector(17, 49, 93, 255), {0, 1}},
                                           {{3, -1, z}, CImVector(17, 49, 93, 255), {1, 0}}};
                device.BeginFrame(pass);
                auto* buffer = device.BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), 3);
                device.BufData(buffer, vertices, sizeof(vertices), 0);
                GxPrimVertexPtr(buffer, GxVBF_PCT);
                GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
                GxRsSet(GxRs_AlphaRef, alphaRef);
                GxRsSet(GxRs_PolygonOffset, offset);
                CGxBatch batch{};
                batch.m_primType = GxPrim_Triangles;
                batch.m_count = 3;
                batch.m_maxIndex = 2;
                device.Draw(&batch, 0);
                CiRect rect(0, 0, 32, 32);
                TSGrowableArray<CImVector> pixels;
                device.DeviceReadPixels(rect, pixels);
                TSGrowableArray<float> depths;
                device.DeviceReadDepths(rect, depths);
                const bool visible = alphaRef <= 128;
                const auto& p = pixels[16 * 32 + 16];
                if (p.r != (visible ? 96 : 0) || p.g != (visible ? 144 : 0) || p.b != (visible ? 192 : 0) || p.a != (visible ? 128 : 0)) {
                    std::fprintf(stderr, "Ripple mismatch z=%g offset=%g ref=%d RGBA=%u,%u,%u,%u depth=%g\n", z, offset, alphaRef, p.r, p.g,
                                 p.b, p.a, depths[16 * 32 + 16]);
                    throw std::runtime_error("Water-ripple texture RGB or alpha boundary changed");
                }
                const float expected = visible ? std::clamp(z - offset, 0.0f, 1.0f) : 1;
                if (std::abs(depths[16 * 32 + 16] - expected) > 1e-6f)
                    throw std::runtime_error("Water-ripple normalized depth offset or clamping changed");
                device.ScenePresent();
                ++draws;
            }
    // Bias changes the comparison after clipping. The actual ripple path keeps
    // depth writes disabled, and must not pull a clipped primitive into view.
    pass.depthAttachment.clearDepth = 0.45;
    for (float z : {-0.1f, 0.5f, 1.1f})
        for (float offset : {-0.25f, 0.0f, 0.25f}) {
            device.BeginFrame(pass);
            GxRsSet(GxRs_DepthWrite, 0);
            GxRsSet(GxRs_DepthTest, 1);
            GxRsSet(GxRs_DepthFunc, 0);
            GxRsSet(GxRs_AlphaRef, 0);
            GxRsSet(GxRs_PolygonOffset, offset);
            CGxVertexPCT vertices[] = {{{-1, -1, z}, CImVector(255, 255, 255, 255), {0, 0}},
                                       {{-1, 3, z}, CImVector(255, 255, 255, 255), {0, 1}},
                                       {{3, -1, z}, CImVector(255, 255, 255, 255), {1, 0}}};
            auto* buffer = device.BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), 3);
            device.BufData(buffer, vertices, sizeof(vertices), 0);
            GxPrimVertexPtr(buffer, GxVBF_PCT);
            CGxBatch batch{};
            batch.m_primType = GxPrim_Triangles;
            batch.m_count = 3;
            batch.m_maxIndex = 2;
            device.Draw(&batch, 0);
            CiRect rect(0, 0, 32, 32);
            TSGrowableArray<CImVector> pixels;
            device.DeviceReadPixels(rect, pixels);
            TSGrowableArray<float> depths;
            device.DeviceReadDepths(rect, depths);
            const bool visible = z == 0.5f && offset == 0.25f;
            if (pixels[16 * 32 + 16].r != (visible ? 96 : 0) || std::abs(depths[16 * 32 + 16] - 0.45f) > 1e-6f)
                throw std::runtime_error("Water-ripple clipping/comparison or disabled depth write changed");
            device.ScenePresent();
            ++draws;
        }
    pass.depthAttachment.clearDepth = 1;
    GxRsSet(GxRs_DepthWrite, 1);
    GxRsSet(GxRs_DepthTest, 0);
    // A nontrivial transform and perspective divide distinguish the original
    // column constants from UI's dot-product rows. Points ignore polygon bias.
    C44Matrix transform(1);
    transform.a0 = 0.7f;
    transform.a1 = 0.2f;
    transform.b0 = -0.1f;
    transform.b1 = 0.9f;
    transform.c2 = 0.8f;
    transform.c3 = 0.3f;
    transform.d0 = 0.17f;
    transform.d1 = -0.11f;
    transform.d2 = 0.1f;
    device.ShaderConstantsSet(GxSh_Vertex, 0, &transform.a0, 4);
    for (uint8_t alpha : {uint8_t(0), uint8_t(64), uint8_t(255)}) {
        device.BeginFrame(pass);
        CGxVertexPCT vertex{{0.13f, -0.27f, 0.25f}, CImVector(0, 0, 0, alpha), {0.5f, 0.5f}};
        auto* buffer = device.BufStream(GxPoolTarget_Vertex, sizeof(vertex), 1);
        device.BufData(buffer, &vertex, sizeof(vertex), 0);
        GxPrimVertexPtr(buffer, GxVBF_PCT);
        GxRsSet(GxRs_BlendingMode, GxBlend_Add);
        GxRsSet(GxRs_AlphaRef, 0);
        GxRsSet(GxRs_PolygonOffset, 0.25f);
        CGxBatch batch{};
        batch.m_primType = GxPrim_Points;
        batch.m_count = 1;
        device.Draw(&batch, 0);
        const float w = 1 + vertex.p.z * 0.3f;
        const float x = 0.7f * vertex.p.x - 0.1f * vertex.p.y + 0.17f, y = 0.2f * vertex.p.x + 0.9f * vertex.p.y - 0.11f;
        const unsigned index = unsigned(std::floor((1 - y / w) * 16)) * 32 + unsigned(std::floor((x / w + 1) * 16));
        CiRect rect(0, 0, 32, 32);
        TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        TSGrowableArray<float> depths;
        device.DeviceReadDepths(rect, depths);
        const float sourceAlpha = (128.0f / 255) * (float(alpha) / 255);
        const unsigned actual[] = {pixels[index].r, pixels[index].g, pixels[index].b, pixels[index].a};
        const float expected[] = {96 * sourceAlpha, 144 * sourceAlpha, 192 * sourceAlpha, 255 * sourceAlpha * sourceAlpha};
        for (unsigned i = 0; i < 4; ++i)
            if (std::abs(int(actual[i]) - int(std::lround(expected[i]))) > 1)
                throw std::runtime_error("Water-ripple source-alpha additive blending changed");
        if (std::abs(depths[index] - (0.25f * 0.8f + 0.1f) / w) > 1e-6f)
            throw std::runtime_error("Water-ripple point transform or polygon-bias exclusion changed");
        device.ScenePresent();
        ++draws;
    }
    std::fprintf(stderr, "Water ripples: %u RGB/alpha/blend/transform/depth-offset reference draws passed\n", draws);
}
