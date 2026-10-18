#include "Tools/AppleBootstrap/TerrainPreview.h"
#include "WoW/Source/WorldClient/MapAreaData.h"
#include "WoW/Source/WorldClient/MapChunkData.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstdio>
#include "Engine/Source/Os/TimeManager.h"
#include <cstring>
#include <stdexcept>

void CheckGameWaterRipples(CMapArea& area);

namespace {
void CheckOriginalAreaLoad() {
    const auto specular = CMap::enableSpecularTerrain;
    const auto flags = CMap::header.m_flags;
    struct Restore { decltype(specular) value; decltype(flags) flags; ~Restore() { CMap::enableSpecularTerrain = value; CMap::header.m_flags = flags; } } restore{specular, flags};
    CMap::enableSpecularTerrain = 1;
    for (const char* name : {"World\\Maps\\Azeroth\\Azeroth_32_48.adt", "World\\Maps\\Northrend\\Northrend_32_32.adt"}) {
        CMapArea area;
        struct Purge { CMapArea& area; ~Purge() { area.Purge(); } } purge{area};
        area.Load(name);
        AsyncFileReadWait(area.m_asyncObject);
        if (area.m_asyncObject || !area.m_nativeData || area.m_header != &area.m_nativeData->header ||
            area.m_chunkInfo != area.m_nativeData->chunks.data() || area.m_doodadDef != area.m_nativeData->doodads.data() ||
            area.m_mapObjDef != area.m_nativeData->objects.data() || area.m_liquid != area.m_nativeData->liquid.get())
            throw std::runtime_error("Original asynchronous ADT loader did not publish its owned tables");
        for (uint32_t i = 0; i < area.m_textures.Count(); ++i)
            if (!area.m_textures[i].m_texture || !TextureGetGxTex(area.m_textures[i].m_texture, 1, nullptr))
                throw std::runtime_error("Original ADT texture table did not load");
        const auto* source = static_cast<const uint8_t*>(area.m_fileBuffer)+20+area.m_header->m_mcin+8;
        SMChunkInfo first; std::memcpy(&first, source, sizeof(first));
        area.m_chunkInfo[0].m_flags ^= 1;
        if (std::memcmp(&first, source, sizeof(first))) throw std::runtime_error("ADT runtime flags modified source bytes");
        // These fixtures use original 4-bit Elwynn and 8-bit Northrend alpha.
        CMap::header.m_flags = std::strstr(name, "Northrend") ? 4 : 0;
        for (unsigned slot : {0u, 127u, 255u}) {
            CMapChunk chunk;
            CMapBaseObjLink parent{};
            parent.m_ref = &area; parent.m_owner = &chunk;
            chunk.m_parentLinkList.LinkToTail(&parent);
            chunk.m_chunkInfoBeginPtr = static_cast<uint8_t*>(area.m_fileBuffer)+area.m_chunkInfo[slot].m_offset;
            chunk.ProcessIffChunks(true);
            if (!chunk.m_nativeData || chunk.m_header != &chunk.m_nativeData->header || !chunk.m_height ||
                chunk.m_header->m_index.x != int32_t(slot%16) || chunk.m_header->m_index.y != int32_t(slot/16))
                throw std::runtime_error("Original area-to-chunk load did not publish aligned runtime fields");
        }
        std::fprintf(stderr, "Original ADT area %s: %d doodads, %d WMOs, %u resident textures; owned table lifecycle passed\n",
            name, area.m_doodadDefCount, area.m_mapObjDefCount, area.m_textures.Count());
        if (std::strstr(name,"Azeroth")) CheckGameWaterRipples(area);
    }
}
}

void CheckGameTerrain() {
    CheckOriginalAreaLoad();
    auto &device = *static_cast<CGxDeviceMetal *>(g_theGxDevicePtr);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:256 height:256 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 1;
    for (const char *name : {"Azeroth:32:48", "Northrend:32:32", "Expansion01:19:31", "Expansion01:28:37"}) {
        TerrainPreview terrain(name);
        if (!terrain.Ready(true) || terrain.ChunkCount() != 256)
            throw std::runtime_error("Real terrain surface did not become ready");
        if (terrain.LiquidLayerCount() != (name[0] == 'A' ? 42 : name[0] == 'N' ? 256 : std::strstr(name, "19:31") ? 208 : 31))
            throw std::runtime_error("Terrain liquid layers omitted");
        TSGrowableArray<CImVector> first;
        unsigned changed = 0;
        for (unsigned frame = 0; frame < 6; ++frame) {
            device.BeginFrame(pass);
            struct End {
                CGxDeviceMetal &device;
                ~End() {
                    if (device.FrameCommand())
                        device.ScenePresent();
                }
            } end{device};
            const auto before = device.EncodingStats();
            if (!terrain.Draw(0, 256, 256, true, frame >= 2))
                throw std::runtime_error("Ready terrain failed to draw");
            const auto after = device.EncodingStats();
            if (after.geometryDraws - before.geometryDraws != (frame >= 2 ? 0 : 256) + terrain.LiquidBatchCount())
                throw std::runtime_error("Terrain surface omitted chunks");
            if (after.renderPasses - before.renderPasses != 1)
                throw std::runtime_error("Resident terrain split its render pass");
            if (!frame && after.bufferUploadPasses-before.bufferUploadPasses > 3) throw std::runtime_error("Initial terrain/liquid buffer copies were not batched");
            CiRect rect(0, 0, 256, 256);
            TSGrowableArray<CImVector> pixels;
            device.DeviceReadPixels(rect, pixels);
            unsigned covered = 0, varied = 0;
            const CImVector *sample = nullptr;
            for (unsigned i = 0; i < pixels.Count(); ++i)
                if (pixels[i].a) {
                    ++covered;
                    if (!sample)
                        sample = &pixels[i];
                    varied += pixels[i].r != sample->r || pixels[i].g != sample->g || pixels[i].b != sample->b;
                }
            if (covered < (frame >= 2 ? 50 : 10000) || varied < covered / 2)
                throw std::runtime_error("Real terrain coverage or material variation missing");
            if (frame == 2) {
                first.SetCount(pixels.Count());
                std::memcpy(first.m_data, pixels.m_data, pixels.Count() * sizeof(CImVector));
            }
            if (frame > 2) {
                for (unsigned i = 0; i < pixels.Count(); ++i)
                    changed += std::memcmp(&pixels[i], &first[i], sizeof(CImVector)) != 0;
            }
            if (frame && after.bufferUploadPasses != before.bufferUploadPasses)
                throw std::runtime_error("Resident terrain/liquid geometry reuploaded");
            std::fprintf(stderr, "Terrain GPU %s frame %u: %u triangles, %u covered pixels, %llu draws / %llu passes / %llu buffer uploads\n", name, frame,
                         terrain.TriangleCount(), covered, static_cast<unsigned long long>(after.geometryDraws - before.geometryDraws),
                         static_cast<unsigned long long>(after.renderPasses - before.renderPasses),
                         static_cast<unsigned long long>(after.bufferUploadPasses-before.bufferUploadPasses));
            device.ScenePresent();
            if (frame >= 2 && frame < 5)
                OsSleep(250);
        }
        if (!changed)
            throw std::runtime_error("Terrain liquid animation did not change visible pixels");
    }
}
