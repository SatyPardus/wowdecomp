#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include "WoW/Source/WorldClient/shadow.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "WoW/Source/WorldClient/Sand.h"
#include <cstdlib>
#include <cstring>
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/UIUtil/CSimpleCamera.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/Game/GameTime.h"
#include "Engine/Source/Tempest/CRect.h"
#include <array>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

void CheckDayNightData();
void CheckDayNightMap(unsigned cycle);
void CheckDayNightTerrain();

namespace {
void Require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
int32_t InitializeServices(const void*, void*) {
    AsyncFileReadInitialize(0, 20);
    TextureInitialize();
    return 1;
}
int32_t DestroyServices(const void*, void*) {
    TextureDestroy();
    AsyncFileReadDestroy();
    return 1;
}
unsigned portraitRefreshes;
void CheckMapTransfers() {
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    const auto projection = device.m_projection;
    device.XformPush(GxXform_View);
    device.RsPush();
    struct Restore {
        CGxDeviceMetal& device;
        C44Matrix projection;
        ~Restore() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop();
            device.XformSetProjection(projection);
            GxXformPop(GxXform_View);
        }
    } restore{device, projection};
    struct Destination { const char* name; int32_t id; C3Vector position; bool dungeon; };
    const Destination route[] = {
        {"Azeroth", 0, {-9465, 64, 85}, false},
        {"Northrend", 571, {5800, 650, 660}, false},
        {"Expansion01", 530, {3082, 3596, 147}, false},
        {"MoltenCore", 409, {1115.22f, -462.96f, -95.01f}, true},
        {"Azeroth", 0, {-9465, 64, 85}, false}
    };
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:128 height:128 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    auto* scene = World::s_m2Scene;
    unsigned tick = 0;
    for (const auto& destination : route) {
        // Match Client.cpp's world transfer order, without a player/server or
        // destroying the shared world, M2 cache, Services or Metal device.
        World::UnloadMap();
        World::s_weather->Reset();
        Require(!CMap::bActive && !CMap::bDungeon && !CWorldScene::s_viewerMapObjDef,
                "Map transfer retained active map/viewer state");
        for (auto* area : CMap::m_areaTable) Require(!area, "Map transfer retained an ADT area");
        CSimpleCamera camera(0.2f, 500, 1);
        camera.m_position = destination.position;
        C3Vector direction(1, .3f, -.2f); direction.Normalize(); camera.SetFacing(direction);
        CRect viewport{0, 0, 128, 128};
        camera.SetGxProjectionAndView(viewport);
        World::ViewCameraScope cameraScope(camera);
        CWorld::LoadMap(destination.name, &camera.m_position, destination.id, 1, camera);
        Require(CMap::bActive && bool(CMap::bDungeon) == destination.dungeon && World::s_m2Scene == scene &&
                World::QueryMapId() == destination.id && DayNight::s_mapId == destination.id,
                "Map transfer selected stale geometry, lighting or world ownership");
        const auto* record = g_mapDB.GetRecord(destination.id);
        unsigned scenePixels = 0, varied = 0;
        const auto first = device.EncodingStats();
        for (unsigned frame = 0; frame < 120; ++frame) {
            FrameTime::Update(1.0f/60, ++tick*17);
            auto& lighting = *DayNight::GetInfo();
            lighting.m_dayProgression = .5f; lighting.m_timeOfDay = 720; lighting.m_day = 0;
            if (record->m_timeOfDayOverride != -1) {
                lighting.m_timeOfDay = 0;
                lighting.m_dayProgression = record->m_timeOfDayOverride >= 0 && record->m_timeOfDayOverride <= 1440 ? record->m_timeOfDayOverride/1440.0f : .5f;
            }
            lighting.m_timeSec = tick/60.0f; lighting.m_deltaSec = 1.0f/60;
            lighting.m_farClip = camera.m_farZ; lighting.m_weatherIntensity = 0;
            lighting.m_playerPos = lighting.m_lightRefPos = lighting.m_cameraPos = camera.m_position;
            lighting.m_cameraDir = direction;
            g_clientGameTime.SetHourAndMinutes(720);
            camera.SetGxProjectionAndView(viewport);
            C3Vector target = camera.m_position + direction;
            CWorld::Update(&camera.m_position, &target, &camera.m_position, camera);
            if (frame < 2) AsyncFileReadWaitAll();
            device.BeginFrame(pass);
            GxXformSetViewport(0,1,0,1,0,CWorld::s_unk_ADEEE4);
            CWorldScene::Render(camera.m_position,1.0f/60,camera);
            CMap::RenderFootprints(); CWorldScene::RenderScenePasses(1.0f/60);
            if (frame == 119) {
                CiRect rect(0,0,128,128); TSGrowableArray<CImVector> pixels; TSGrowableArray<float> depths;
                device.DeviceReadPixels(rect,pixels); device.DeviceReadDepths(rect,depths);
                for (unsigned i=0;i<pixels.Count();++i) {
                    scenePixels += depths[i] < CWorld::s_unk_ADEEE4;
                    varied += pixels[i] != pixels[0];
                }
            }
            device.ScenePresent();
        }
        Require(scenePixels > 1024 && varied > 1024, "Transferred map did not produce real scene geometry/color");
        std::fprintf(stderr,"World transfer %s (map %d, dungeon %d): 120 frames, %u scene / %u varied pixels, %llu draws; original world/Metal ownership retained\n",
                     destination.name,destination.id,destination.dungeon,scenePixels,varied,
                     static_cast<unsigned long long>(device.EncodingStats().geometryDraws-first.geometryDraws));
    }
}
std::array<unsigned, 5> CheckLoadedMap() {
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    const auto projection = device.m_projection;
    device.XformPush(GxXform_View);
    device.RsPush();
    struct Restore {
        CGxDeviceMetal& device;
        C44Matrix projection;
        ~Restore() {
            if (device.FrameCommand()) device.ScenePresent();
            device.RsPop();
            device.XformSetProjection(projection);
            GxXformPop(GxXform_View);
        }
    } restore{device, projection};
    C3Vector position(-9000, 0, 180);
    CSimpleCamera camera(0.2f, 500, 1);
    CRect viewport;
    viewport.minX = viewport.minY = 0;
    viewport.maxX = viewport.maxY = 1;
    camera.m_position = position;
    C3Vector direction(1, 0.3f, -0.2f); direction.Normalize();
    camera.SetFacing(direction);
    camera.SetGxProjectionAndView(viewport);
    World::ViewCameraScope cameraScope(camera);
    CWorld::LoadMap("Azeroth", &position, 0, 0, camera);
    Require(CMap::bActive && !CMap::bPreload && CMap::s_mapLight, "Original map activation did not complete");
    std::array<unsigned, 5> counts{};
    for (auto* area : CMap::m_areaTable)
        if (area && area->m_nativeData) {
            ++counts[0];
            for (auto* chunk : area->m_mapChunks)
                counts[1] += chunk && chunk->m_nativeData;
        }
    for (auto* wmo = CMap::mapObjDefHash.m_fulllist.Head(); wmo; wmo = CMap::mapObjDefHash.Next(wmo))
        ++counts[2];
    for (auto* doodad = CMap::doodadDefHash.m_fulllist.Head(); doodad; doodad = CMap::doodadDefHash.Next(doodad))
        counts[3] += doodad->m_model && doodad->m_model->IsLoaded(0, 0);
    for (auto* horizon : CMap::s_lowDetail.m_areas)
        counts[4] += horizon != nullptr;
    for (auto count : counts)
        Require(count, "Original map load omitted terrain, placements, models or horizon data");
    std::fprintf(stderr,
                 "Original Azeroth map: %u areas, %u terrain chunks, %u WMO placements, %u loaded doodad models, %u horizon tiles\n",
                 counts[0], counts[1], counts[2], counts[3], counts[4]);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:128 height:128 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 1;
    g_clientGameTime.SetHourAndMinutes(720);
    for (unsigned frame = 0; frame < 3; ++frame) {
        FrameTime::Update(1.0f / 60, 1000 + frame * 17);
        auto& lighting = *DayNight::GetInfo();
        lighting.m_dayProgression = 0.5f;
        lighting.m_timeOfDay = 720;
        lighting.m_timeSec = frame / 60.0f;
        lighting.m_deltaSec = 1.0f / 60;
        lighting.m_farClip = camera.m_farZ;
        lighting.m_playerPos = lighting.m_lightRefPos = lighting.m_cameraPos = position;
        lighting.m_cameraDir = camera.Forward();
        C3Vector target = position + camera.Forward();
        camera.SetGxProjectionAndView(viewport);
        CWorld::Update(&position, &target, &position, camera);
        AsyncFileReadWaitAll();
        device.BeginFrame(pass);
        GxXformSetViewport(0, 1, 0, 1, 0, CWorld::s_unk_ADEEE4);
        CWorldScene::Render(position, 1.0f / 60, camera);
        CMap::RenderFootprints();
        CWorldScene::RenderScenePasses(1.0f / 60);
        CiRect rect(0, 0, 128, 128);
        TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        TSGrowableArray<float> depths;
        device.DeviceReadDepths(rect, depths);
        unsigned scenePixels = 0;
        for (uint32_t i = 0; i < depths.Count(); ++i) scenePixels += depths[i] < CWorld::s_unk_ADEEE4;
        Require(scenePixels > 1024, "Original scene geometry is occluded by the horizon/sky depth bands");
        unsigned varied = 0;
        for (uint32_t i = 0; i < pixels.Count(); ++i) varied += pixels[i].r != pixels[0].r || pixels[i].g != pixels[0].g || pixels[i].b != pixels[0].b;
        Require(varied > 1024 && CWorldScene::s_chunksRendered && CWorldScene::nDoodadsRendered,
                "Original world frame omitted terrain/models or produced a flat image");
        std::fprintf(stderr, "Original world frame %u: %u varied / %u main-scene pixels, %u chunks, %u doodads\n", frame, varied, scenePixels, CWorldScene::s_chunksRendered, CWorldScene::nDoodadsRendered);
        device.ScenePresent();
    }
    if (const char* focus=std::getenv("WOW_METAL_CHECK"); focus && !std::strcmp(focus,"weather-world")) {
        auto* weather=World::s_weather;
        auto* density=CVar::Lookup("weatherDensity"); const std::string oldDensity=density->m_stringValue.m_str;
        density->Set("2",true,false,false,false);
        for (unsigned type : {2u,3u,1u}) {
            weather->Reset(); weather->SetType(type,0.75f,nullptr,false,1.0f); weather->Update();
            Require((weather->m_snow && weather->m_snow->m_useWeatherShaders) || (weather->m_sand && weather->m_sand->m_useWeatherShaders) ||
                (weather->m_rain && weather->m_rain->m_useWeatherShaders), "Original weather selected a software fallback");
            if (weather->m_snow) Require(TextureGetGxTex(weather->m_snow->m_snowFlakeTex,1,nullptr),"Original snow texture missing");
            if (weather->m_rain) {
                Require(TextureGetGxTex(weather->m_rain->m_rainDropTex,1,nullptr),"Original rain texture missing");
                Require(TextureGetGxTex(weather->m_rain->m_rainDropSplashTex,1,nullptr),"Original splash texture missing");
            }
            unsigned peakPackets=0,peakPixels=0; uint64_t weatherDraws=0;
            for (unsigned frame=0;frame<120;++frame) {
                FrameTime::Update(1.0f/60,2000+type*3000+frame*17);
                camera.SetGxProjectionAndView(viewport); weather->Update();
                device.BeginFrame(pass); device.SceneClear(3,CImVector(128,128,128,255));
                GxXformSetViewport(0,1,0,1,0,1);
                device.RsPush();
                device.RsSet(GxRs_VertexShader,static_cast<CGxShader*>(nullptr)); device.RsSet(GxRs_PixelShader,static_cast<CGxShader*>(nullptr));
                for (unsigned i=0;i<4;++i) device.RsSet(EGxRenderState(GxRs_Texture0+i),static_cast<CGxTex*>(nullptr));
                const auto before=device.EncodingStats().geometryDraws;
                weather->Render(); weatherDraws+=device.EncodingStats().geometryDraws-before;
                device.RsPop();
                const unsigned packets=weather->m_snow ? weather->m_snow->m_snowFlakePacketList.m_activePacketsCount :
                    weather->m_sand ? weather->m_sand->m_sandGrainPacketList.m_activePacketsCount : weather->m_rain->m_rainDropPacketList.m_activePacketsCount;
                peakPackets=std::max(peakPackets,packets);
                if (frame>=60 && frame%15==0) {
                    CiRect rect(0,0,128,128); TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect,pixels);
                    unsigned changed=0;
                    for (unsigned i=0;i<pixels.Count();++i) changed+=pixels[i].r!=128 || pixels[i].g!=128 || pixels[i].b!=128;
                    peakPixels=std::max(peakPixels,changed);
                }
                device.ScenePresent();
            }
            Require(peakPackets && weatherDraws && peakPixels,"Original weather packets produced no visible pixels");
            std::fprintf(stderr,"Original weather type %u: %u active packets, %llu draws, %u changed pixels across 120 frames\n",type,peakPackets,(unsigned long long)weatherDraws,peakPixels);
            // Exercise the same live weather through the full original scene.
            device.BeginFrame(pass); GxXformSetViewport(0,1,0,1,0,CWorld::s_unk_ADEEE4);
            CWorldScene::Render(position,1.0f/60,camera); CWorldScene::RenderScenePasses(1.0f/60); device.ScenePresent();
        }
        weather->Reset(); density->Set(oldDensity.c_str(),true,false,false,false);
    }
    return counts;
}
} // namespace

void CheckGameWorldLifecycle() {
    const auto pattern = (std::filesystem::temp_directory_path() / "wow-world-XXXXXX").string();
    std::vector<char> directory(pattern.begin(), pattern.end());
    directory.push_back(0);
    Require(mkdtemp(directory.data()), "Cannot create world probe configuration directory");
    struct Scope {
        std::string directory;
        bool services = false, modelCache = false, world = false, shadows = false;
        ~Scope() {
            CWorldParam::SetPortraitRefreshCallback(nullptr);
            if (shadows) ShadowDestroy();
            if (world)
                CWorld::Destroy();
            if (modelCache)
                M2Destroy();
            CShaderEffectManager::DestroyAllEffects();
            ModelBlobDestroy();
            if (services) {
                EventDestroy();
                PropDestroy();
            }
            CVar::Destroy();
            ConsoleCommandDestroy();
            CStringManager::DestroyManager();
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    } scope{directory.data()};
    CVar::Initialize((scope.directory + "/Config.wtf").c_str());
    PropInitialize();
    EventInitialize(1, 0);
    scope.services = true;
    EventCreateContextEx(1, InitializeServices, DestroyServices, 1, 0);
    EventProcessPending();

    if (!g_footprintTexturesDB.m_loaded)
        g_footprintTexturesDB.Load(__FILE__, __LINE__);
    if (!g_groundEffectDoodadDB.m_loaded)
        g_groundEffectDoodadDB.Load(__FILE__, __LINE__);
    if (!g_liquidTypeDB.m_loaded)
        g_liquidTypeDB.Load(__FILE__, __LINE__);
    if (!g_liquidMaterialDB.m_loaded)
        g_liquidMaterialDB.Load(__FILE__, __LINE__);
    if (!g_animationDataDB.m_loaded)
        g_animationDataDB.Load(__FILE__, __LINE__);
    Require(ModelBlobLoad("World\\model.blob"), "World model bounds index did not load");
    CheckDayNightData();
    if (!g_areaTableDB.m_loaded) g_areaTableDB.Load(__FILE__, __LINE__);
    if (!g_wmoAreaTableDB.m_loaded) g_wmoAreaTableDB.Load(__FILE__, __LINE__);
    if (!g_groundEffectTextureDB.m_loaded) g_groundEffectTextureDB.Load(__FILE__, __LINE__);
    if (!g_mapDB.m_loaded)
        g_mapDB.Load(__FILE__, __LINE__);
    std::array<unsigned, 5> firstMap{};

    const char* focus = std::getenv("WOW_METAL_CHECK");
    const bool transfers = focus && !std::strcmp(focus,"world-transfers");
    for (unsigned cycle = 0; cycle < (transfers ? 1u : 2u); ++cycle) {
        // CWorld teardown destroys shader effects. The application-owned M2
        // cache must finish that lifetime too, before another world is started.
        M2Initialize(8, 16);
        scope.modelCache = true;
        CWorldParam::Initialize();
        portraitRefreshes = 0;
        CWorldParam::SetPortraitRefreshCallback([] { ++portraitRefreshes; });
        CWorldParam::BaseMipCallback(nullptr, "0", "1", nullptr);
        CWorldParam::BaseMipCallback(nullptr, "1", "1", nullptr);
        Require(portraitRefreshes == 1, "World texture change notification did not preserve unchanged values");
        CWorldParam::SetPortraitRefreshCallback(nullptr);
        CWorldParam::BaseMipCallback(nullptr, "1", "0", nullptr);
        Require(portraitRefreshes == 1, "World retained a detached portrait subscriber");

        CWorld::Initialize();
        scope.world = true;
        Require(World::s_m2Scene && World::s_weather && CWorld::particulate, "Original world initialization omitted owned systems");
        auto* weatherDensity = CVar::Lookup("weatherDensity");
        Require(weatherDensity, "World weather configuration was not registered");
        World::s_weather->m_densityChanged = 0;
        Require(weatherDensity->Set(cycle ? "3" : "0", true, false, false, false) && World::s_weather->m_densityChanged,
                "Weather configuration did not notify the current world");
        if (transfers) {
            scope.shadows = true; ShadowInit();
            Require(s_cvExtShadowQuality && s_cvExtShadowQuality->Set("5",true,false,false,false),"Cannot select original PCF shadow quality");
            Require(CShaderEffect::s_usePcfFiltering, "Map transfer check requires native comparison shadows");
            CheckMapTransfers();
            ShadowDestroy(); scope.shadows = false;
        } else {
            CheckDayNightMap(cycle);
            const auto loadedMap = CheckLoadedMap();
            if (cycle)
                Require(loadedMap == firstMap, "Map recreation changed the loaded world population");
            else
                firstMap = loadedMap;
        }

        CWorld::Destroy();
        scope.world = false;
        Require(!CMap::bActive && !CMap::mapObjDefHash.m_fulllist.Head() && !CMap::doodadDefHash.m_fulllist.Head(),
                "World teardown retained map placements");
        for (auto* area : CMap::m_areaTable)
            Require(!area, "World teardown retained an area");
        for (auto* horizon : CMap::s_lowDetail.m_areas)
            Require(!horizon, "World teardown retained horizon data");
        M2Destroy();
        scope.modelCache = false;
        Require(!World::s_m2Scene && !World::s_weather && !CWorld::particulate, "World destruction retained owned global references");
        // CVars survive a world lifetime. This callback and registration during
        // the next constructor must not touch the destroyed Weather instance.
        Require(weatherDensity->Set("2", true, false, false, false), "Weather configuration failed between world lifetimes");
    }
    if (transfers) {
        std::fputs("World transfer lifecycle: five map loads in one world/device lifetime and final destruction passed\n",stderr);
        return;
    }
    CheckDayNightTerrain();
    std::fprintf(
        stderr,
        "World lifecycle: original map loading and destruction twice, live/detached weather callbacks and portrait notifications passed\n");
}
