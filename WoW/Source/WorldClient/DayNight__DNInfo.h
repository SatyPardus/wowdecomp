#ifndef WOW_SOURCE_WORLDCLIENT_DAYNIGHT__DNINFO_H
#define WOW_SOURCE_WORLDCLIENT_DAYNIGHT__DNINFO_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/WorldClient/DayNight.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/stpl.h"

class CMapObjDef;
class LightParamsRec;
class LightRec;

struct DNFogInfo {
    /* 0000 */ CImVector m_color;
    /* 0004 */ float m_start;
    /* 0008 */ float m_end;
    /* 000C */ float m_density;
};

struct DNLightState {
    /* 0000 */ C3Vector m_dir;
    /* 000C */ CImVector m_diffuse;
    /* 0010 */ CImVector m_ambient;
    /* 0014 */ CImVector m_mid0;
    /* 0018 */ CImVector m_mid1;
    /* 001C */ float m_desatR;
    /* 0020 */ float m_desatG;
    /* 0024 */ float m_desatB;
    /* 0028 */ float m_desatA;

    DNLightState();
};

namespace DayNight {
struct DNInfo {
    /* 0000 */ uint32_t m_timeOfDay;
    /* 0004 */ float m_dayProgression;
    /* 0008 */ float m_day;
    /* 000C */ C3Vector m_playerPos;
    /* 0018 */ C3Vector m_cameraPos;
    /* 0024 */ C3Vector m_lightRefPos;
    /* 0030 */ C3Vector m_cameraDir;
    /* 003C */ float m_faceAngle;
    /* 0040 */ float m_farClip;
    /* 0044 */ float m_timeSec;
    /* 0048 */ float m_deltaSec;
    /* 004C */ float m_weatherIntensity;
    /* 0050 */ uint8_t m_flashBlend;
    /* 0051 */ CImVector m_flashColor;
    /* 0055 */ uint8_t unused_0055 [3];
    /* 0058 */ uint32_t m_overrideLightParamsId;
    /* 005C */ DNOverrideSky* m_skybox;
    /* 0060 */ float m_skyboxWeight;
    /* 0064 */ DNOverrideSky* m_blendSkybox [3];
    /* 0070 */ float m_blendSkyboxWeight [3];
    /* 007C */ uint32_t m_blendSkyboxFlags [3];
    /* 0088 */ float m_stormBlend;
    /* 008C */ DNFogInfo m_fog;
    /* 009C */ float m_interiorFogBlend;
    /* 00A0 */ DNFogInfo m_fogInterior;
    /* 00B0 */ DNFogInfo m_fogGroup;
    /* 00C0 */ CMapObjDef* m_interiorMapObjDef;
    /* 00C4 */ TSGrowableArray<uint32_t> m_interiorGroups;
    /* 00D4 */ CurrentLight m_bands;
    /* 0170 */ DNLightState m_light0;
    /* 019C */ DNLightState m_light1;
    /* 01C8 */ CImVector m_shadowColor;
    /* 01CC */ uint32_t m_showSky;
    /* 01D0 */ C2Vector m_cloudSun;
    /* 01D8 */ float m_sunMoonPath;
    /* 01DC */ float m_curve0;
    /* 01E0 */ float m_curve1;
    /* 01E4 */ uint32_t m_nearLightCount;
    /* 01E8 */ uint32_t unused_01E8;
    /* 01EC */ LightRec* m_nearLight [5];
    /* 0200 */ float m_nearLightBlend [5];

    DNInfo();
    void AddZoneLight(int32_t lightId, float blend);
    void ClearZoneLights();
};

extern DNInfo g_dnInfo;

DNInfo* GetInfo();
CImVector LoadLightIntBand(int32_t time, const LightParamsRec* params, int32_t band);
void SetFlashColor(CImVector color, float blend);
}

// INLINED
inline DNLightState::DNLightState() {
    this->m_desatR = 0.0f;
    this->m_desatG = 0.0f;
    this->m_desatB = 0.0f;
    this->m_desatA = 0.0f;
}

#endif
