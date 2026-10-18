#include "Tools/AppleBootstrap/WmoPreview.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstdio>
#include "Engine/Source/Os/TimeManager.h"
#include <cstring>
#include <stdexcept>

void CheckGameWmoScene() {
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:256 height:256 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear; pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    for (const char* name : {"World\\wmo\\Azeroth\\Buildings\\AltarOfStorms\\AltarOfStorms.wmo",
        "World\\wmo\\Azeroth\\Buildings\\Stormwind\\Stormwind.wmo",
        "World\\wmo\\Northrend\\Dalaran\\ND_Dalaran.wmo"}) {
        WmoPreview wmo(name);
        if (!wmo.Ready(true)) throw std::runtime_error("Real WMO materials did not become ready");
        TSGrowableArray<CImVector> first;
        for (unsigned frame = 0; frame < 2; ++frame) {
            device.BeginFrame(pass);
            struct End { CGxDeviceMetal& device; ~End() { if (device.FrameCommand()) device.ScenePresent(); } } end{device};
            const auto before = device.EncodingStats();
            if (!wmo.Draw(0, 256, 256, true)) throw std::runtime_error("Ready WMO failed to draw");
            const auto after = device.EncodingStats();
            if (after.geometryDraws - before.geometryDraws != wmo.BatchCount()) throw std::runtime_error("Original WMO draw omitted batches or transition passes");
            if (frame && after.renderPasses-before.renderPasses != 1) throw std::runtime_error("Resident WMO split its render pass");
            CiRect rect(0, 0, 256, 256); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            unsigned covered = 0, varied = 0;
            const CImVector* sample = nullptr;
            for (unsigned i = 0; i < pixels.Count(); ++i) if (pixels[i].a) {
                ++covered; if (!sample) sample = &pixels[i];
                varied += pixels[i].r != sample->r || pixels[i].g != sample->g || pixels[i].b != sample->b;
            }
            if (covered < 100 || varied < covered/3) throw std::runtime_error("Real WMO coverage or material variation missing");
            if (frame) {
                if (pixels.Count() != first.Count() || std::memcmp(pixels.m_data, first.m_data, pixels.Count()*sizeof(CImVector)))
                    throw std::runtime_error("Unchanged resident WMO did not reproduce its pixels");
            } else { first.SetCount(pixels.Count()); std::memcpy(first.m_data, pixels.m_data, pixels.Count()*sizeof(CImVector)); }
            std::fprintf(stderr, "WMO GPU %s frame %u: %u covered pixels, %llu original draws / %llu passes\n", name, frame, covered,
                static_cast<unsigned long long>(after.geometryDraws-before.geometryDraws), static_cast<unsigned long long>(after.renderPasses-before.renderPasses));
            device.ScenePresent();
        }
    }
    for (const char* name : {"World\\wmo\\Azeroth\\Buildings\\Stormwind\\Stormwind.wmo:224",
        "World\\wmo\\Northrend\\Dalaran\\ND_Dalaran.wmo:71",
        "World\\wmo\\Dungeon\\AZ_Blackrock\\Blackrock.wmo:43"}) {
        WmoPreview wmo(name);
        if (!wmo.Ready(true)) throw std::runtime_error("Real WMO liquid materials did not become ready");
        TSGrowableArray<CImVector> first;
        unsigned changed = 0;
        for (unsigned frame = 0; frame < 4; ++frame) {
            device.BeginFrame(pass);
            struct End { CGxDeviceMetal& device; ~End() { if (device.FrameCommand()) device.ScenePresent(); } } end{device};
            const auto before = device.EncodingStats();
            if (!wmo.Draw(0, 256, 256, true, true)) throw std::runtime_error("Ready WMO liquid failed to draw");
            const auto after = device.EncodingStats();
            if (after.geometryDraws-before.geometryDraws != 1) throw std::runtime_error("Original WMO liquid material omitted its draw");
            CiRect rect(0, 0, 256, 256); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            unsigned covered = 0;
            for (unsigned i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            if (covered < 16) throw std::runtime_error("Real WMO liquid coverage missing");
            if (frame) {
                for (unsigned i = 0; i < pixels.Count(); ++i) changed += std::memcmp(&pixels[i], &first[i], sizeof(CImVector)) != 0;
                if (after.bufferUploadPasses != before.bufferUploadPasses) throw std::runtime_error("Resident liquid reuploaded unchanged geometry");
            } else { first.SetCount(pixels.Count()); std::memcpy(first.m_data,pixels.m_data,pixels.Count()*sizeof(CImVector)); }
            std::fprintf(stderr,"WMO liquid GPU %s frame %u: %u covered pixels, %u changed pixels, %llu passes\n",name,frame,covered,changed,
                static_cast<unsigned long long>(after.renderPasses-before.renderPasses));
            device.ScenePresent();
            if (frame < 3) OsSleep(250);
        }
        if (!changed) throw std::runtime_error("Original WMO texture animation did not change visible pixels");
    }

}
