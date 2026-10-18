#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/UIUtil/CSimpleCamera.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
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
std::array<unsigned, 5> CheckLoadedMap() {
    auto& device = *g_theGxDevicePtr;
    const auto projection = device.m_projNative;
    device.XformPush(GxXform_View);
    struct Restore {
        CGxDevice& device;
        C44Matrix projection;
        ~Restore() {
            device.m_projNative = projection;
            GxXformPop(GxXform_View);
        }
    } restore{device, projection};
    C3Vector position(-9000, 0, 80);
    CSimpleCamera camera(0.2f, 500, 1);
    CRect viewport;
    viewport.minX = viewport.minY = 0;
    viewport.maxX = viewport.maxY = 1;
    camera.SetGxProjectionAndView(viewport);
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
        bool services = false, modelCache = false, world = false;
        ~Scope() {
            CWorldParam::SetPortraitRefreshCallback(nullptr);
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
    if (!g_mapDB.m_loaded)
        g_mapDB.Load(__FILE__, __LINE__);
    std::array<unsigned, 5> firstMap{};

    for (unsigned cycle = 0; cycle < 2; ++cycle) {
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
        CheckDayNightMap(cycle);
        const auto loadedMap = CheckLoadedMap();
        if (cycle)
            Require(loadedMap == firstMap, "Map recreation changed the loaded world population");
        else
            firstMap = loadedMap;

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
    CheckDayNightTerrain();
    std::fprintf(
        stderr,
        "World lifecycle: original map loading and destruction twice, live/detached weather callbacks and portrait notifications passed\n");
}
