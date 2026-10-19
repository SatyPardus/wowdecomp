#pragma once
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/WorldClient/World.h"
#include <cstdio>
#include <cstdlib>

// Main game thread only. Native notifications can arrive inside a nested host
// loop; the host services this request after original Event dispatch returns.
class NativeClientMemory {
    bool pending = false;
    unsigned checkFrames = 0, checkRequests = 0;
public:
    void Request() { pending = true; }
    void Process(CGxDeviceMetal& device) {
        if (!pending || device.FrameCommand()) return;
        pending = false;
        // These original routines free only zero-reference models and detached
        // reusable textures. Live world/UI resources and async loads keep owners.
        if (World::s_m2Scene) World::s_m2Scene->PurgeModelCache();
        TextureGxCacheFlush();
        const auto before = device.EncodingStats();
        device.DeviceEvictResources();
        std::fprintf(stderr, "Native memory pressure: unused model/texture caches trimmed; %llu stream bytes retired without GPU wait\n",
            static_cast<unsigned long long>(device.EncodingStats().discardedStreamBytes - before.discardedStreamBytes));
    }
    void Frame(CGxDeviceMetal& device, bool inWorld) {
        // Explicit, bounded lifecycle exercise; it sends no game input and
        // allocates no artificial pressure that could affect other applications.
        if (inWorld && checkRequests < 3 && std::getenv("WOW_CLIENT_MEMORY_CHECK") && ++checkFrames == 120) {
            checkFrames = 0; ++checkRequests; Request();
            std::fprintf(stderr, "Native memory pressure check: request %u/3 after 120 world frames\n", checkRequests);
        }
        Process(device);
    }
};
