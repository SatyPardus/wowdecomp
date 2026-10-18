#include "WorldPreview.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Game/GameTime.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Event/Event.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/Tempest/CRect.h"
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace {
template <class T> void Load(T& table) {
    if (!table.m_loaded) table.Load(__FILE__, __LINE__);
    if (!table.m_loaded || !table.m_numRecords) throw std::runtime_error("Cannot load original world database");
}
}
WorldPreview::WorldPreview(const std::string& selection) {
    if (!CVar::m_initialized || g_modelPool || World::s_m2Scene)
        throw std::logic_error("World view requires configuration and exclusive world/model ownership");
    const auto first = selection.find(':');
    if (first == std::string::npos) throw std::runtime_error("WOW_WORLD must be Map:worldX:worldY:worldZ");
    m_map = selection.substr(0, first);
    std::istringstream coordinates(selection.substr(first + 1));
    char separator1, separator2;
    auto& p = m_camera.m_position;
    if (!(coordinates >> p.x >> separator1 >> p.y >> separator2 >> p.z) || separator1 != ':' || separator2 != ':' ||
        !(coordinates >> std::ws).eof() || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
        std::abs(p.x) > 17067 || std::abs(p.y) > 17067 || std::abs(p.z) > 100000)
        throw std::runtime_error("WOW_WORLD has invalid world coordinates");
    C3Vector direction(1, 0.3f, -0.2f); direction.Normalize();
    m_camera.SetFacing(direction);
    try {
        Load(g_mapDB);
        bool mapFound = false;
        for (int32_t i = 0; i < g_mapDB.m_numRecords; ++i) {
            const auto* record = g_mapDB.GetRecordByIndex(i);
            if (!SStrCmpI(record->m_directory, m_map.c_str(), STORM_MAX_STR)) {
                mapFound = true; m_timeOverride = record->m_timeOfDayOverride; break;
            }
        }
        if (!mapFound) throw std::runtime_error("WOW_WORLD map is absent from the original Map database");
        Load(g_animationDataDB); Load(g_footprintTexturesDB); Load(g_groundEffectDoodadDB); Load(g_groundEffectTextureDB);
        Load(g_areaTableDB); Load(g_wmoAreaTableDB); Load(g_liquidTypeDB); Load(g_liquidMaterialDB);
        Load(g_lightParamsDB); Load(g_lightSkyboxDB); Load(g_lightIntBandDB); Load(g_lightFloatBandDB);
        if (!g_lightDB.m_loaded) { Load(g_lightDB); InitLights(true); }
        m_blob = ModelBlobLoad("World\\model.blob");
        if (!m_blob) throw std::runtime_error("Cannot load world model bounds index");
        M2Initialize(8, 16); m_models = true;
        CWorldParam::Initialize();
        // Initialization may allocate several world-owned subsystems before an
        // unavailable effect fails. CWorld::Destroy also cleans partial state.
        m_world = true;
        CWorld::Initialize();
    } catch (...) { Shutdown(); throw; }
}
WorldPreview::~WorldPreview() { Shutdown(); }
void WorldPreview::Shutdown() {
    if (m_world) { CWorld::Destroy(); m_world = false; }
    if (m_models) { M2Destroy(); m_models = false; }
    CShaderEffectManager::DestroyAllEffects();
    if (m_blob) { ModelBlobDestroy(); m_blob = false; }
}
void WorldPreview::SetTime(float minutes) {
    if (!std::isfinite(minutes) || minutes < 0 || minutes >= 1440) throw std::runtime_error("World time must be in [0, 1440) minutes");
    m_daySeconds = double(minutes) * 60;
}
bool WorldPreview::Draw(float elapsedSeconds, uint32_t width, uint32_t height) {
    if (!width || !height) return false;
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0) throw std::runtime_error("Invalid world elapsed time");
    auto& device = *g_theGxDevicePtr;
    device.RsPush();
    device.XformPush(GxXform_View);
    struct Restore {
        CGxDevice& device; C44Matrix projection; float viewport[6]{};
        ~Restore() {
            GxXformSetViewport(viewport[0], viewport[1], viewport[2], viewport[3], viewport[4], viewport[5]);
            GxXformPop(GxXform_View); device.XformSetProjection(projection); device.RsPop();
        }
    } restore{device, device.m_projection};
    GxXformViewport(restore.viewport[0], restore.viewport[1], restore.viewport[2], restore.viewport[3], restore.viewport[4], restore.viewport[5]);
    CRect screen; screen.minX = screen.minY = 0; screen.maxX = float(width); screen.maxY = float(height);
    m_camera.SetGxProjectionAndView(screen);
    World::ViewCameraScope cameraScope(m_camera);
    if (!m_loaded) { CWorld::LoadMap(m_map.c_str(), &m_camera.m_position, 0, 0, m_camera); m_loaded = true; EventResetElapsedTime(); }
    m_elapsed += elapsedSeconds;
    m_daySeconds = std::fmod(m_daySeconds + elapsedSeconds, 86400.0);
    FrameTime::Update(elapsedSeconds, uint32_t(std::fmod(m_elapsed * 1000, 4294967296.0)));
    auto& lighting = *DayNight::GetInfo();
    lighting.m_dayProgression = float(m_daySeconds / 86400.0);
    lighting.m_timeOfDay = uint32_t(m_daySeconds / 60);
    lighting.m_day = 0;
    if (m_timeOverride != -1) {
        lighting.m_timeOfDay = 0;
        lighting.m_dayProgression = m_timeOverride >= 0 && m_timeOverride <= 1440 ? m_timeOverride / 1440.0f : 0.5f;
    }
    lighting.m_weatherIntensity = World::s_weather->m_densityBlend * World::s_weather->m_fogIntensity;
    lighting.m_timeSec = float(m_elapsed); lighting.m_deltaSec = elapsedSeconds;
    lighting.m_farClip = m_camera.m_farZ;
    lighting.m_playerPos = lighting.m_lightRefPos = lighting.m_cameraPos = m_camera.m_position;
    lighting.m_cameraDir = m_camera.Forward();
    g_clientGameTime.SetHourAndMinutes(uint32_t(m_daySeconds / 60));
    C3Vector target = m_camera.m_position + m_camera.Forward();
    CWorld::Update(&m_camera.m_position, &target, &m_camera.m_position, m_camera);
    // Match WorldFrame::OnWorldRender: main geometry ends before the separate
    // horizon and sky depth bands, or distant silhouettes occlude near terrain.
    GxXformSetViewport(0, 1, 0, 1, 0, CWorld::s_unk_ADEEE4);
    CWorldScene::Render(m_camera.m_position, elapsedSeconds, m_camera);
    CMap::RenderFootprints();
    CWorldScene::RenderScenePasses(elapsedSeconds);
    return true;
}
