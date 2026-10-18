#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include "WoW/Source/WorldClient/MapWeather.h"
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

        CWorld::Destroy();
        scope.world = false;
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
        "World lifecycle: original initialization/destruction twice, live/detached weather callbacks and portrait notifications passed\n");
}
