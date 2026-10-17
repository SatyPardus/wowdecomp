#include "Tools/AppleBootstrap/TerrainPreview.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

void CheckGameTerrain() {
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:256 height:256 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear; pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    for (const char* name : {"Azeroth:32:48", "Northrend:32:32"}) {
        TerrainPreview terrain(name);
        if (!terrain.Ready(true) || terrain.ChunkCount() != 256) throw std::runtime_error("Real terrain surface did not become ready");
        TSGrowableArray<CImVector> first;
        for (unsigned frame = 0; frame < 2; ++frame) {
            device.BeginFrame(pass);
            struct End { CGxDeviceMetal& device; ~End() { if (device.FrameCommand()) device.ScenePresent(); } } end{device};
            const auto before = device.EncodingStats();
            if (!terrain.Draw(0, 256, 256, true)) throw std::runtime_error("Ready terrain failed to draw");
            const auto after = device.EncodingStats();
            if (after.geometryDraws-before.geometryDraws != 256) throw std::runtime_error("Terrain surface omitted chunks");
            if (frame && after.renderPasses-before.renderPasses != 1) throw std::runtime_error("Resident terrain split its render pass");
            CiRect rect(0, 0, 256, 256); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            unsigned covered = 0, varied = 0;
            const CImVector* sample = nullptr;
            for (unsigned i = 0; i < pixels.Count(); ++i) if (pixels[i].a) {
                ++covered;
                if (!sample) sample = &pixels[i];
                varied += pixels[i].r != sample->r || pixels[i].g != sample->g || pixels[i].b != sample->b;
            }
            if (covered < 10000 || varied < covered/2) throw std::runtime_error("Real terrain coverage or material variation missing");
            if (frame) {
                if (pixels.Count() != first.Count() || std::memcmp(pixels.m_data, first.m_data, pixels.Count()*sizeof(CImVector)))
                    throw std::runtime_error("Unchanged resident terrain did not reproduce its pixels");
            } else { first.SetCount(pixels.Count()); std::memcpy(first.m_data, pixels.m_data, pixels.Count()*sizeof(CImVector)); }
            std::fprintf(stderr, "Terrain GPU %s frame %u: %u triangles, %u covered pixels, %llu draws / %llu passes\n", name, frame,
                terrain.TriangleCount(), covered, static_cast<unsigned long long>(after.geometryDraws-before.geometryDraws),
                static_cast<unsigned long long>(after.renderPasses-before.renderPasses));
            device.ScenePresent();
        }
    }
}
