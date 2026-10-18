#include "Tools/AppleBootstrap/DayNightPreview.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
template <class T> void Load(T& table) {
    if (!table.m_loaded)
        table.Load(__FILE__, __LINE__);
    if (!table.m_loaded || !table.m_numRecords)
        throw std::runtime_error("Cannot load original DayNight database");
}
} // namespace

DayNightPreview::DayNightPreview(const std::string& mapName) {
    if (!CVar::m_initialized || g_modelPool)
        throw std::logic_error("Terrain DayNight requires configuration and exclusive model ownership");
    try {
        Load(g_mapDB);
        Load(g_animationDataDB);
        Load(g_liquidTypeDB);
        Load(g_lightParamsDB);
        Load(g_lightSkyboxDB);
        Load(g_lightIntBandDB);
        Load(g_lightFloatBandDB);
        if (!g_lightDB.m_loaded) {
            Load(g_lightDB);
            // Same one-time coordinate conversion as ClientDBInitialize.
            InitLights(true);
        }
        int32_t mapId = -1;
        for (int32_t i = 0; i < g_mapDB.m_numRecords; ++i) {
            const auto* record = g_mapDB.GetRecordByIndex(i);
            if (!SStrCmpI(record->m_directory, mapName.c_str(), STORM_MAX_STR)) {
                mapId = record->m_id;
                break;
            }
        }
        if (mapId < 0)
            throw std::runtime_error("Terrain map is absent from original Map database");
        m_mapId = mapId;
        m_blob = ModelBlobLoad("World\\model.blob");
        if (!m_blob)
            throw std::runtime_error("Cannot load DayNight model bounds index");
        CShaderEffect::InitShaderSystem(1, 0);
        m_effects = true;
        CShaderEffectManager::AddEffectFile("Model2.wfx");
        CShaderEffectManager::AddEffectFile("Particle.wfx");
        M2Initialize(8, 16);
        m_models = true;
        m_map = true;
        DayNight::LoadMap(mapId);
    } catch (...) {
        Shutdown();
        throw;
    }
}
DayNightPreview::~DayNightPreview() { Shutdown(); }
void DayNightPreview::Shutdown() {
    if (m_map) {
        DayNight::UnloadMap();
        m_map = false;
    }
    if (m_models) {
        M2Destroy();
        m_models = false;
    }
    if (m_effects) {
        CShaderEffectManager::DestroyAllEffects();
        m_effects = false;
    }
    if (m_blob) {
        ModelBlobDestroy();
        m_blob = false;
    }
}
void DayNightPreview::SetTime(float minutes) {
    if (!std::isfinite(minutes) || minutes < 0 || minutes >= 1440)
        throw std::runtime_error("Terrain time must be in minutes from 0 to less than 1440");
    m_daySeconds = minutes * 60.0;
}
void DayNightPreview::Update(float elapsedSeconds, const C3Vector& position, const C3Vector& camera, const C3Vector& direction,
                             float farClip) {
    m_daySeconds = std::fmod(m_daySeconds + elapsedSeconds, 86400.0);
    m_elapsedSeconds += elapsedSeconds;
    auto& info = *DayNight::GetInfo();
    info.m_dayProgression = float(m_daySeconds / 86400.0);
    info.m_timeOfDay = uint32_t(m_daySeconds / 60);
    info.m_timeSec = float(m_elapsedSeconds);
    info.m_deltaSec = elapsedSeconds;
    info.m_farClip = farClip;
    info.m_playerPos = info.m_lightRefPos = position;
    info.m_cameraPos = camera;
    info.m_cameraDir = direction;
    // Original DayNight::Update selects exponential fog on expansion maps.
    DayNight::SetFogMode(m_mapId >= 530 && uint32_t(g_theGxDevicePtr->Caps().m_shaderTargets[0]) > 1);
    info.ClearZoneLights();
    DayNight::UpdateLighting();
    DayNight::UpdateFog({}, 0); // This ADT-only host has no WMO interior or submerged camera.
    std::array<uint32_t, 8> water;
    std::memcpy(water.data(), info.m_bands.m_waterColors, 16);
    std::memcpy(water.data() + 4, &info.m_bands.m_waterShallowAlpha, 16);
    if (water != m_waterState) {
        Liquid::InvalidateProceduralTextures();
        m_waterState = water;
    }
}
