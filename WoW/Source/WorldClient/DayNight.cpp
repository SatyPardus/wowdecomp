#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include <cmath>
#include <new>
#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LightFloatBandRec.h"
#include "WoW/Source/DB/Rec/LightParamsRec.h"
#include "WoW/Source/DB/Rec/LightRec.h"
#include "WoW/Source/DB/Rec/LightSkyboxRec.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "Engine/Source/ShaderEffect/Types.h"

static const float SUNRISE = 0.22916667f;
static const float NOON = 0.5f;
static const float SUNSET = 0.89583331f;
static const float MOONRISE = 0.91666669f;
static const float DAYEND = 1.0f;
static const float MIDNIGHT = 0.0f;
static const float MOONSET = 0.16666667f;
static const float VALUE_SCALE = 1.25f;
static const float SATURATION_SCALE = 0.33f;
static const float DEFAULT_FOG_SCALER = 0.5f;

// STATIC: 0x00AF4C20
static C2Vector s_starFadeTable[4] = { C2Vector(0.125f, 1.0f), C2Vector(0.1875f, 0.0f), C2Vector(0.9375f, 0.0f), C2Vector(1.0f, 1.0f) };

// STATIC: 0x00AF4C80
static C2Vector s_curveTable0[4] = { C2Vector(0.25f, 1.0f), C2Vector(0.29166666f, 0.0f), C2Vector(0.85416663f, 0.0f), C2Vector(0.89583331f, 1.0f) };

// STATIC: 0x00AF4CA0
static C2Vector s_curveTable1[2] = { C2Vector(0.083333336f, 0.25f), C2Vector(0.5f, 1.0f) };

// STATIC: 0x00D38180
int32_t DayNight::GlareBase::m_masterEnable;

// STATIC: 0x00D38184
int32_t DayNight::s_lightFlags;

// STATIC: 0x00D38A88
float DayNight::s_glowBlend;

// STATIC: 0x00D38A8C
static float s_eclipseAmount;

// STATIC: 0x00D38AC4
int32_t DayNight::s_mapId;

// STATIC: 0x00D38AC8
static CVar* s_cvSkyCloudLOD;

// STATIC: 0x00D38ADC
int32_t DayNight::g_forceFullUpdate;

// STATIC: 0x00D38AE4
DayNight::DNStars DayNight::g_stars;

// STATIC: 0x00D38D44
static CImVector s_glowColor;

// STATIC: 0x00D38D4C
DayNight::DNSky DayNight::g_sky;

// STATIC: 0x00D38D90
DayNight::DNClouds DayNight::g_clouds;

// STATIC: 0x00D38E28
DayNight::DNPlanets DayNight::g_planets;

// STATIC: 0x00D38EA8
DayNight::DNSunGlare DayNight::g_sunGlare;

// STATIC: 0x00D38F58
DayNight::DNMoonGlare DayNight::g_moonGlare;

// STATIC: 0x00AF4A68
static int32_t s_useZoneLights = 1;

// STATIC: 0x00AF4A6C
static int32_t s_useAreaLights = 1;

// STATIC: 0x00AF4DA8
static float s_lightMergeDistance = 1.0f / 3.0f;

// STATIC: 0x00D38AD4
static CM2Scene* s_skyScene;

// STATIC: 0x00D38AD8
static uint32_t s_skySceneTimeMs;

// STATIC: 0x00D38AE0
int32_t DayNight::LightQE::s_localSort;

// STATIC: 0x00D38D1C
TSHashTable<DayNight::DNOverrideSky, HASHKEY_STRI> DayNight::g_overrideSky;

// STATIC: 0x00D38D7C
static CDataAllocator s_lightAllocator(sizeof(LightRec), 16);

// STATIC: 0x00D39004
TSGrowableArray<const LightRec*> DayNight::g_areaLights;

// STATIC: 0x00D39014
static TSGrowableArray<DayNight::AreaLightOverride> s_areaLightOverrides;

// STATIC: 0x00D38A90
static float s_glowDuration;

// STATIC: 0x00D38A98
static float s_glowEndTime;

// STATIC: 0x00D38AA0
static float s_glowStartTime;

// STATIC: 0x00D38AA8
static float s_overrideFogRate;

// STATIC: 0x00D38AAC
static float s_overrideFogStart;

// STATIC: 0x00D38AB0
static float s_overrideFogEnd;

// STATIC: 0x00D38AB4
static int32_t s_savedShowSky;

// STATIC: 0x00D38AB8
static float s_savedFogRate;

// STATIC: 0x00D38ABC
static float s_savedFogStartMul;

// STATIC: 0x00D38AC0
static float s_savedFogEnd;

// STATIC: 0x00D38ACC
static int32_t s_fogMode;

// STATIC: 0x00D38AD0
static int32_t s_fogOverride;

// STATIC: 0x00D38D14
static CImVector s_savedSkyFog;

// STATIC: 0x00D38D18
static CImVector s_overrideFogColor;

// STATIC: 0x00D38B00
DayNight::DNInfo DayNight::g_dnInfo;

// STATIC: 0x00AF4B7C
static C2Vector s_highlightCurve[6] = { C2Vector(0.125f, 0.0f), C2Vector(0.27083334f, 1.0f), C2Vector(0.29166669f, 0.0f), C2Vector(0.85416663f, 0.0f), C2Vector(0.89583331f, 1.0f), C2Vector(0.99930555f, 0.0f) };

// STATIC: 0x00AF4BAC
static C2Vector s_skyFaceCurve[6] = { C2Vector(0.125f, 1.0f), C2Vector(0.375f, 0.0f), C2Vector(0.5f, -0.5f), C2Vector(0.625f, -0.69999999f), C2Vector(0.75f, -0.5f), C2Vector(0.875f, 0.0f) };

enum SKY_CONSTANTS : int32_t {
    SKY_NUMBANDS = 7,
};

// STATIC: 0x00A41A90
static const float s_skyThetaTable[SKY_NUMBANDS] = { 0.0f, 0.17f, 0.2f, 0.23f, 0.24f, 0.25f, 1.0f };

// STATIC: 0x00AF4DAC
static const uint16_t s_planetIndices[4] = { 0, 1, 2, 3 };

// STATIC: 0x00AF4DB4
static const uint16_t s_planetFadeIndices[6] = { 0, 1, 4, 5, 2, 3 };

// STATIC: 0x00ADEEF0
static float s_skyMinZ = 0.9990234375f;

// STATIC: 0x00ADEEF4
static float s_skyMaxZ = 1.0f;

// STATIC: 0x00AF4B70
static uint16_t s_glareIndices[4] = { 0, 2, 1, 3 };

// STATIC: 0x00AF4CB0
static C3Vector s_glareVertices[4] = { C3Vector(0.0f, -0.5f, 0.5f), C3Vector(0.0f, 0.5f, 0.5f), C3Vector(0.0f, -0.5f, -0.5f), C3Vector(0.0f, 0.5f, -0.5f) };

// STATIC: 0x00AF4CE0
static C2Vector s_glareTexCoords[4] = { C2Vector(0.0f, 0.0f), C2Vector(1.0f, 0.0f), C2Vector(0.0f, 1.0f), C2Vector(1.0f, 1.0f) };

// STATIC: 0x00AF4DC0
static float s_glarePower = 10.0f;

// STATIC: 0x00D38188
static float s_cloudFade[256];

// STATIC: 0x00D38588
static uint8_t s_cloudSharpness[256];

// STATIC: 0x00D38688
static float s_cloudNoise[256];

// STATIC: 0x00AF4A70
static const uint8_t s_cloudPerm[256] = {
    225, 155, 210, 108, 175, 199, 221, 144, 203, 116, 70, 213, 69, 158, 33, 252,
    5, 82, 173, 133, 222, 139, 174, 27, 9, 71, 90, 246, 75, 130, 91, 191,
    169, 138, 2, 151, 194, 235, 81, 7, 25, 113, 228, 159, 205, 253, 134, 142,
    248, 65, 224, 217, 22, 121, 229, 63, 89, 103, 96, 104, 156, 17, 201, 129,
    36, 8, 165, 110, 237, 117, 231, 56, 132, 211, 152, 20, 181, 111, 239, 218,
    170, 163, 51, 172, 157, 47, 80, 212, 176, 250, 87, 49, 99, 242, 136, 189,
    162, 115, 44, 43, 124, 94, 150, 16, 141, 247, 32, 10, 198, 223, 255, 72,
    53, 131, 84, 57, 220, 197, 58, 50, 208, 11, 241, 28, 3, 192, 62, 202,
    18, 215, 153, 24, 76, 41, 15, 179, 39, 46, 55, 6, 128, 167, 23, 188,
    106, 34, 187, 140, 164, 73, 112, 182, 244, 195, 227, 13, 35, 77, 196, 185,
    26, 200, 226, 119, 31, 123, 168, 125, 249, 68, 183, 230, 177, 135, 160, 180,
    12, 1, 243, 148, 102, 166, 38, 238, 251, 37, 240, 126, 64, 74, 161, 40,
    184, 149, 171, 178, 101, 66, 29, 59, 146, 61, 254, 107, 42, 86, 154, 4,
    236, 232, 120, 21, 233, 209, 45, 98, 193, 114, 78, 19, 206, 14, 118, 127,
    48, 79, 147, 85, 30, 207, 219, 54, 88, 234, 190, 122, 95, 67, 143, 109,
    137, 214, 145, 93, 92, 100, 245, 0, 216, 186, 60, 83, 105, 97, 204, 52
};

// STATIC: 0x00AF4DC4
static const uint16_t s_cloudOctaveFreq[5][5] = { { 16, 32, 64, 128, 256 }, { 8, 16, 32, 64, 128 }, { 4, 8, 16, 32, 64 }, { 2, 4, 8, 16, 32 }, { 1, 2, 4, 8, 16 } };

// STATIC: 0x00A41AAC
static const uint32_t s_cloudLodSize[5] = { 128, 256, 512, 1024, 2048 };

// STATIC: 0x00A41AC0
static const uint32_t s_cloudLodShift[5] = { 7, 8, 9, 10, 11 };

// STATIC: 0x00A41AD4
static const float s_cloudThetaTable[12] = { 0.0f, 0.025f, 0.050000001f, 0.075000003f, 0.1f, 0.125f, 0.15000001f, 0.175f, 0.205f, 0.23f, 0.245f, 0.25f };

// STATIC: 0x00A41B04
static const uint8_t s_cloudAlphaTable[12] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 128, 0, 0 };

// STATIC: 0x00AF4BE0
static C2Vector s_cloudLightCurve[8] = { C2Vector(0.16666667f, 1.0f), C2Vector(0.19444445f, 1.0f), C2Vector(0.2013889f, 1.0f), C2Vector(0.22916667f, 1.0f), C2Vector(0.89583331f, 1.0f), C2Vector(0.9236111f, 1.0f), C2Vector(0.88888890f, 1.0f), C2Vector(0.91666669f, 1.0f) };

// VERIFY: 5ec68a40 auto
// OFFSET: 0x007A0C70
void DayNight::LightQueue::Erase(uint32_t index, uint32_t count) {
    if (index >= this->m_count) {
        return;
    }

    if (index + count > this->m_count) {
        count = this->m_count - index;
    }

    uint32_t tail = this->m_count - index - count;

    if (tail) {
        memmove(this->m_data + sizeof(LightQE) * index, this->m_data + sizeof(LightQE) * (index + count), sizeof(LightQE) * tail);
    }

    this->m_count -= count;
}

// VERIFY: bd10d666 manual
// OFFSET: 0x007EB160
static C3Vector GetLightPosition(const LightRec* const* light) {
    return C3Vector((*light)->m_gameCoords[0], (*light)->m_gameCoords[1], (*light)->m_gameCoords[2]);
}

// VERIFY: 28940a59 auto
// OFFSET: 0x007EB180
const LightParamsRec* DayNight::GetLightParams(const LightRec* light, int32_t param) {
    return g_lightParamsDB.GetRecord(light->m_lightParamsID[param]);
}

// VERIFY: 731bfb93 auto
// OFFSET: 0x007EBF90
float DayNight::CalcIndividualColorFromDBRec(int32_t time, const LightParamsRec* params, int32_t band) {
    LightFloatBandRec rec;
    g_lightFloatBandDB.GetRecord(band + 6 * params->m_id - 5, &rec);
    return DayNight::InterpFloatBand(&rec, time, band);
}

// VERIFY: bab367f3 auto
// OFFSET: 0x007EBFF0
void DayNight::LoadLightingBands(int32_t time, CurrentLight* light, const LightParamsRec* params) {
    light->m_diffuse = DayNight::LoadLightIntBand(time, params, 0);
    light->m_ambient = DayNight::LoadLightIntBand(time, params, 1);
    light->m_skyColors[0] = DayNight::LoadLightIntBand(time, params, 2);
    light->m_skyColors[1] = DayNight::LoadLightIntBand(time, params, 3);
    light->m_skyColors[2] = DayNight::LoadLightIntBand(time, params, 4);
    light->m_skyColors[3] = DayNight::LoadLightIntBand(time, params, 5);
    light->m_skyColors[4] = DayNight::LoadLightIntBand(time, params, 6);
    light->m_skyColors[5] = DayNight::LoadLightIntBand(time, params, 7);
    light->m_shadowOpacity = DayNight::LoadLightIntBand(time, params, 8);
    light->m_sunCloudColors[0] = DayNight::LoadLightIntBand(time, params, 9);
    light->m_sunCloudColors[1] = DayNight::LoadLightIntBand(time, params, 10);
    light->m_sunCloudColors[2] = DayNight::LoadLightIntBand(time, params, 11);
    light->m_sunCloudColors[3] = DayNight::LoadLightIntBand(time, params, 12);
    light->m_sunCloudColors[4] = DayNight::LoadLightIntBand(time, params, 13);
    light->m_waterColors[0] = DayNight::LoadLightIntBand(time, params, 14);
    light->m_waterColors[1] = DayNight::LoadLightIntBand(time, params, 15);
    light->m_waterColors[2] = DayNight::LoadLightIntBand(time, params, 16);
    light->m_waterColors[3] = DayNight::LoadLightIntBand(time, params, 17);
    light->m_fogEnd = DayNight::CalcIndividualColorFromDBRec(time, params, 0);
    light->m_fogStartMul = DayNight::CalcIndividualColorFromDBRec(time, params, 1);

    if (light->m_fogStartMul < -1.0f) {
        light->m_fogStartMul = -1.0f;
    } else if (light->m_fogStartMul > 1.0f) {
        light->m_fogStartMul = 1.0f;
    }

    light->m_fogRate = 1.0f;
    light->m_floatBand2 = DayNight::CalcIndividualColorFromDBRec(time, params, 2);
    light->m_cloudDensity = DayNight::CalcIndividualColorFromDBRec(time, params, 3);
    light->m_floatBand4 = DayNight::CalcIndividualColorFromDBRec(time, params, 4);
    light->m_floatBand5 = DayNight::CalcIndividualColorFromDBRec(time, params, 5);
    light->m_highlightSky = static_cast<float>(params->m_highlightSky);
    light->m_glow = params->m_glow;
    light->m_waterShallowAlpha = params->m_waterShallowAlpha;
    light->m_waterDeepAlpha = params->m_waterDeepAlpha;
    light->m_oceanShallowAlpha = params->m_oceanShallowAlpha;
    light->m_oceanDeepAlpha = params->m_oceanDeepAlpha;
    light->m_skyboxes[0].m_id = params->m_lightSkyboxID;
    light->m_skyboxes[0].m_weight = 1.0f;
    light->m_cloudTypeId = params->m_cloudTypeID;
    light->m_cloudWeight = 1.0f;
}

// VERIFY: dc45f41c manual
// OFFSET: 0x007EC220
void DayNight::BlendColors(CurrentLight* dst, const CurrentLight* src, float t) {
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t >= 1.0f) {
        t = 1.0f;
    }

    dst->m_diffuse = DayNight::InterpColor(dst->m_diffuse, src->m_diffuse, t);
    dst->m_ambient = DayNight::InterpColor(dst->m_ambient, src->m_ambient, t);
    dst->m_skyColors[0] = DayNight::InterpColor(dst->m_skyColors[0], src->m_skyColors[0], t);
    dst->m_skyColors[1] = DayNight::InterpColor(dst->m_skyColors[1], src->m_skyColors[1], t);
    dst->m_skyColors[2] = DayNight::InterpColor(dst->m_skyColors[2], src->m_skyColors[2], t);
    dst->m_skyColors[3] = DayNight::InterpColor(dst->m_skyColors[3], src->m_skyColors[3], t);
    dst->m_skyColors[4] = DayNight::InterpColor(dst->m_skyColors[4], src->m_skyColors[4], t);
    dst->m_skyColors[5] = DayNight::InterpColor(dst->m_skyColors[5], src->m_skyColors[5], t);
    dst->m_sunCloudColors[0] = DayNight::InterpColor(dst->m_sunCloudColors[0], src->m_sunCloudColors[0], t);
    dst->m_sunCloudColors[1] = DayNight::InterpColor(dst->m_sunCloudColors[1], src->m_sunCloudColors[1], t);
    dst->m_sunCloudColors[2] = DayNight::InterpColor(dst->m_sunCloudColors[2], src->m_sunCloudColors[2], t);
    dst->m_sunCloudColors[3] = DayNight::InterpColor(dst->m_sunCloudColors[3], src->m_sunCloudColors[3], t);
    dst->m_sunCloudColors[4] = DayNight::InterpColor(dst->m_sunCloudColors[4], src->m_sunCloudColors[4], t);
    dst->m_waterColors[0] = DayNight::InterpColor(dst->m_waterColors[0], src->m_waterColors[0], t);
    dst->m_waterColors[1] = DayNight::InterpColor(dst->m_waterColors[1], src->m_waterColors[1], t);
    dst->m_waterColors[2] = DayNight::InterpColor(dst->m_waterColors[2], src->m_waterColors[2], t);
    dst->m_waterColors[3] = DayNight::InterpColor(dst->m_waterColors[3], src->m_waterColors[3], t);
    dst->m_fogEnd = (src->m_fogEnd - dst->m_fogEnd) * t + dst->m_fogEnd;
    dst->m_fogStartMul = (src->m_fogStartMul - dst->m_fogStartMul) * t + dst->m_fogStartMul;
    dst->m_fogRate = (src->m_fogRate - dst->m_fogRate) * t + dst->m_fogRate;
    dst->m_shadowOpacity = DayNight::InterpColor(dst->m_shadowOpacity, src->m_shadowOpacity, t);
    dst->m_cloudDensity = dst->m_cloudDensity + (src->m_cloudDensity - dst->m_cloudDensity) * t;
    dst->m_glow = dst->m_glow + (src->m_glow - dst->m_glow) * t;
    dst->m_waterShallowAlpha = dst->m_waterShallowAlpha + (src->m_waterShallowAlpha - dst->m_waterShallowAlpha) * t;
    dst->m_waterDeepAlpha = dst->m_waterDeepAlpha + (src->m_waterDeepAlpha - dst->m_waterDeepAlpha) * t;
    dst->m_oceanShallowAlpha = dst->m_oceanShallowAlpha + (src->m_oceanShallowAlpha - dst->m_oceanShallowAlpha) * t;
    dst->m_oceanDeepAlpha = t * (src->m_oceanDeepAlpha - dst->m_oceanDeepAlpha) + dst->m_oceanDeepAlpha;
}

// VERIFY: 14c1a79f manual
// OFFSET: 0x007ECB30
void DayNight::LightLoad(TSGrowableArray<const LightRec*>& lights, int32_t mapId) {
    uint32_t count = 1;

    for (int32_t i = 0; i < g_lightDB.m_numRecords; i++) {
        const LightRec* light = i >= 0 && i < g_lightDB.m_numRecords ? &g_lightDB.m_records[i] : nullptr;

        if (light->m_continentID == mapId && (light->m_gameCoords[0] != 0.0f || light->m_gameCoords[1] != 0.0f || light->m_gameCoords[2] != 0.0f)) {
            count++;
        }
    }

    int32_t hasDefault = 0;
    lights.SetCount(count);
    uint32_t index = 1;

    for (int32_t i = 0; i < g_lightDB.m_numRecords; i++) {
        const LightRec* light = i >= 0 && i < g_lightDB.m_numRecords ? &g_lightDB.m_records[i] : nullptr;

        if (light->m_continentID != mapId) {
            continue;
        }

        if (light->m_gameCoords[0] == 0.0f && light->m_gameCoords[1] == 0.0f && light->m_gameCoords[2] == 0.0f) {
            hasDefault = 1;
            lights.m_data[0] = light;
        } else {
            lights.m_data[index++] = light;
        }
    }

    if (!hasDefault) {
        lights.m_data[0] = g_lightDB.GetRecord(1);
    }
}

// VERIFY: c96361d7 auto
// OFFSET: 0x007ECC90
void DayNight::SetFogMode(int32_t mode) {
    if (mode < 2) {
        s_fogMode = mode;
    }
}

// VERIFY: f887589a auto
// OFFSET: 0x007ECCB0
void DayNight::DNInfo::ClearZoneLights() {
    for (int32_t i = 0; i < 5; i++) {
        this->m_nearLightBlend[i] = 0.0f;
        this->m_nearLight[i] = nullptr;
    }

    this->m_nearLightCount = 0;
}

// VERIFY: 8e2d6417 manual
// OFFSET: 0x007ECD00
float DayNight::CalcFogRate(float fogStart, float fogEnd) {
    static float s_rateRange = 7.0f - 1.5f;

    float farClip = DayNight::g_dnInfo.m_farClip;

    if (farClip >= 700.0f) {
        farClip = 700.0f;
    }

    float range = farClip - 200.0f;
    float distance = fogEnd - fogStart;

    if (distance <= range) {
        return (1.0f - distance / range) * s_rateRange + 1.5f;
    }

    return 1.5f;
}

// VERIFY: 71473e14 auto
// OFFSET: 0x007ECD80
void DayNight::CalcColors(int32_t time, CurrentLight* light, const LightParamsRec* params) {
    DayNight::LoadLightingBands(time, light, params);

    if (light->m_fogEnd < 10.0f) {
        light->m_fogEnd = 10.0f;
    }

    if (s_fogMode == 1) {
        if (light->m_fogEnd >= 27.777779f) {
            light->m_fogRate = DayNight::CalcFogRate(light->m_fogStartMul * light->m_fogEnd, light->m_fogEnd);
            light->m_fogEnd = DayNight::g_dnInfo.m_farClip;
        }

        if (light->m_fogStartMul < 0.0f) {
            light->m_fogStartMul = 0.0f;
        }
    }
}

// VERIFY: a958f705 auto
// OFFSET: 0x007ECE10
void DayNight::DNClouds::Callback_GxTex(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command == GxTex_Latch && !level) {
        texelStrideInBytes = 4 * width;
        texels = userArg;
    }
}

// VERIFY: 59b153f5 auto
// OFFSET: 0x007ECE40
static int32_t SkySunGlare(const char* command, const char* arguments) {
    int32_t enable;

    if (sscanf(arguments, "%d", &enable) && enable) {
        ConsoleWrite("SunGlare enabled.  Don't look directly at it.", DEFAULT_COLOR);
        DayNight::g_sunGlare.m_enable = 1;
        DayNight::g_moonGlare.m_enable = 1;
        return 1;
    }

    ConsoleWrite("SunGlare disabled", DEFAULT_COLOR);
    DayNight::g_sunGlare.m_enable = 0;
    DayNight::g_moonGlare.m_enable = 0;
    return 1;
}

// VERIFY: f0fe4721 auto
// OFFSET: 0x007ECEC0
void DayNight::SetOverrideLightParamsID(uint32_t id) {
    DayNight::g_dnInfo.m_overrideLightParamsId = id;

    if (id > 7) {
        DayNight::g_dnInfo.m_overrideLightParamsId = -1;
    }
}

// VERIFY: c1570062 auto
// OFFSET: 0x007ECEE0
void DayNight::ClearOverrideLightParamsID() {
    DayNight::g_dnInfo.m_overrideLightParamsId = -1;
}

// VERIFY: 900bfd47 auto
// OFFSET: 0x007ECEF0
DayNight::DNInfo* DayNight::GetInfo() {
    return &DayNight::g_dnInfo;
}

// VERIFY: 4628ebb7 auto
// OFFSET: 0x007ECF00
void DayNight::ClearLightFlag(int32_t bit) {
    DayNight::s_lightFlags &= ~(1 << bit);
}

// VERIFY: 0aa04f25 manual
// OFFSET: 0x007ECF20
void DayNight::SyncSkyAnimWithTime(DNOverrideSky* sky, int32_t minutes) {
    if (!sky || !sky->m_model || !sky->m_model->IsDrawable(0, 0)) {
        return;
    }

    if (!sky->m_animLengthMs) {
        M2BoneSequenceInfo info;
        sky->m_model->GetBoneSequenceInfo(0xFFFFFFFF, info);
        sky->m_animLengthMs = info.m_endTime - info.m_startTime;
    }

    uint32_t length = sky->m_animLengthMs;

    if (!length) {
        return;
    }

    if (abs(minutes - static_cast<int32_t>(sky->m_lastTimeOfDay)) >= 2 && (sky->m_flags & 1)) {
        float speed = 0.000000011574074f * length;
        uint32_t time = static_cast<uint32_t>(length * (minutes * 0.00069444446f));
        sky->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, time, speed, 1, 1);
    }

    sky->m_lastTimeOfDay = minutes;
}

// VERIFY: 083574c7 manual
// OFFSET: 0x007ED0A0
bool DayNight::LightQE::HasHigherPriority(const LightQE& a, const LightQE& b) {
    if (DayNight::LightQE::s_localSort) {
        return b.m_key >= a.m_key;
    }

    C3Vector positionB = GetLightPosition(b.m_light);
    C3Vector positionA = GetLightPosition(a.m_light);
    float dx = positionA.x - positionB.x;
    float dy = positionA.y - positionB.y;
    float dz = positionA.z - positionB.z;

    if (sqrt(dx * dx + dz * dz + dy * dy) > s_lightMergeDistance) {
        return b.m_key <= a.m_key;
    }

    return (*b.m_light)->m_gameFalloffStart <= (*a.m_light)->m_gameFalloffStart;
}

// VERIFY: 464db582 auto
// OFFSET: 0x007ED150
void DayNight::DNInfo::AddZoneLight(int32_t lightId, float blend) {
    if (this->m_nearLightCount >= 5) {
        return;
    }

    LightRec* light = g_lightDB.GetRecord(lightId);

    if (!light) {
        return;
    }

    this->m_nearLight[this->m_nearLightCount] = light;
    this->m_nearLightBlend[this->m_nearLightCount++] = blend;
}

// VERIFY: 11554f6f manual
// OFFSET: 0x007ED1B0
void DayNight::ApplyFogSettings(const SMOFog::Fog* fog) {
    float end = DayNight::g_dnInfo.m_farClip;

    if (end > fog->m_end) {
        end = fog->m_end;
    }

    DayNight::g_dnInfo.m_fogGroup.m_end = end;
    float start = fog->m_startScalar * end;
    DayNight::g_dnInfo.m_fogGroup.m_start = start;
    DayNight::g_dnInfo.m_fogGroup.m_color = fog->m_color;
    DayNight::g_dnInfo.m_fogGroup.m_density = 1.0f;

    if (end < 30.0f) {
        end = 30.0f;
        DayNight::g_dnInfo.m_fogGroup.m_end = end;
    }

    if (s_fogMode == 1) {
        DayNight::g_dnInfo.m_fogGroup.m_density = DayNight::CalcFogRate(start, end);
        DayNight::g_dnInfo.m_fogGroup.m_end = DayNight::g_dnInfo.m_farClip;

        if (DayNight::g_dnInfo.m_fogGroup.m_start < 0.0f) {
            DayNight::g_dnInfo.m_fogGroup.m_start = 0.0f;
        }
    }
}

// VERIFY: 1903e937 manual
// OFFSET: 0x007ED250
static void InitCloudNoise() {
    for (uint32_t i = 0; i < 256; i++) {
        int32_t value = rand();
        s_cloudNoise[i] = 1.0f - (value * 0.000030518509f + value * 0.000030518509f);
    }

    for (uint32_t i = 0; i < 256; i++) {
        s_cloudFade[i] = (1.0f - cos(i * 0.012271847)) * 0.5f;
    }
}

// VERIFY: 8e99f7a5 manual
// OFFSET: 0x007ED3B0
float DayNight::InterpTable(const C2Vector* table, uint32_t count, float t) {
    float value;

    if (t >= 0.0f) {
        value = t < 1.0f ? t : 1.0f;
    } else {
        value = 0.0f;
    }

    uint32_t index = 0;

    while (index < count && value > table[index].x) {
        index++;
    }

    uint32_t prev;

    if (index == count) {
        index = 0;
        prev = count - 1;
    } else if (index) {
        prev = index - 1;
    } else {
        prev = count - 1;
    }

    float range = table[index].x - table[prev].x;

    if (fabs(range) < 0.001f) {
        return table[prev].y;
    }

    if (range < 0.0f) {
        range += 1.0f;
    }

    float offset = value - table[prev].x;

    if (offset < 0.0f) {
        offset += 1.0f;
    }

    float fraction = offset / range;
    float from = table[prev].y;
    float to = table[index].y;

    if (to < from) {
        return from - fraction * (from - to);
    }

    return fraction * (to - from) + from;
}

// VERIFY: 467e66cc manual
// OFFSET: 0x007ED4C0
void DayNight::BlendOutputs(CurrentLight* dst, const CurrentLight* src, float t) {
    CImVector color;

    DayNight::ColorLerpBytes(&color, &dst->m_ambient, &src->m_ambient, t);
    dst->m_ambient = color;
    DayNight::ColorLerpBytes(&color, &dst->m_diffuse, &src->m_diffuse, t);
    dst->m_diffuse = color;
    DayNight::ColorLerpBytes(&color, &dst->m_shadowOpacity, &src->m_shadowOpacity, t);
    dst->m_shadowOpacity = color;

    for (int32_t i = 0; i < 6; i++) {
        DayNight::ColorLerpBytes(&color, &dst->m_skyColors[i], &src->m_skyColors[i], t);
        dst->m_skyColors[i] = color;
    }

    dst->m_fogEnd = src->m_fogEnd < dst->m_fogEnd ? dst->m_fogEnd - (dst->m_fogEnd - src->m_fogEnd) * t : dst->m_fogEnd + (src->m_fogEnd - dst->m_fogEnd) * t;
    dst->m_fogStartMul = src->m_fogStartMul < dst->m_fogStartMul ? dst->m_fogStartMul - (dst->m_fogStartMul - src->m_fogStartMul) * t : dst->m_fogStartMul + (src->m_fogStartMul - dst->m_fogStartMul) * t;
    dst->m_fogRate = src->m_fogRate < dst->m_fogRate ? dst->m_fogRate - (dst->m_fogRate - src->m_fogRate) * t : dst->m_fogRate + (src->m_fogRate - dst->m_fogRate) * t;
    dst->m_highlightSky = src->m_highlightSky < dst->m_highlightSky ? dst->m_highlightSky - (dst->m_highlightSky - src->m_highlightSky) * t : dst->m_highlightSky + (src->m_highlightSky - dst->m_highlightSky) * t;

    for (int32_t i = 0; i < 5; i++) {
        DayNight::ColorLerpBytes(&color, &dst->m_sunCloudColors[i], &src->m_sunCloudColors[i], t);
        dst->m_sunCloudColors[i] = color;
    }

    for (int32_t i = 0; i < 4; i++) {
        DayNight::ColorLerpBytes(&color, &dst->m_waterColors[i], &src->m_waterColors[i], t);
        dst->m_waterColors[i] = color;
    }

    dst->m_cloudDensity = src->m_cloudDensity < dst->m_cloudDensity ? dst->m_cloudDensity - (dst->m_cloudDensity - src->m_cloudDensity) * t : dst->m_cloudDensity + (src->m_cloudDensity - dst->m_cloudDensity) * t;
    dst->m_glow = src->m_glow < dst->m_glow ? dst->m_glow - (dst->m_glow - src->m_glow) * t : dst->m_glow + (src->m_glow - dst->m_glow) * t;
    dst->m_waterShallowAlpha = src->m_waterShallowAlpha < dst->m_waterShallowAlpha ? dst->m_waterShallowAlpha - (dst->m_waterShallowAlpha - src->m_waterShallowAlpha) * t : dst->m_waterShallowAlpha + (src->m_waterShallowAlpha - dst->m_waterShallowAlpha) * t;
    dst->m_waterDeepAlpha = src->m_waterDeepAlpha < dst->m_waterDeepAlpha ? dst->m_waterDeepAlpha - (dst->m_waterDeepAlpha - src->m_waterDeepAlpha) * t : dst->m_waterDeepAlpha + (src->m_waterDeepAlpha - dst->m_waterDeepAlpha) * t;
    dst->m_oceanShallowAlpha = src->m_oceanShallowAlpha < dst->m_oceanShallowAlpha ? dst->m_oceanShallowAlpha - (dst->m_oceanShallowAlpha - src->m_oceanShallowAlpha) * t : dst->m_oceanShallowAlpha + (src->m_oceanShallowAlpha - dst->m_oceanShallowAlpha) * t;
    dst->m_oceanDeepAlpha = src->m_oceanDeepAlpha < dst->m_oceanDeepAlpha ? dst->m_oceanDeepAlpha - (dst->m_oceanDeepAlpha - src->m_oceanDeepAlpha) * t : dst->m_oceanDeepAlpha + (src->m_oceanDeepAlpha - dst->m_oceanDeepAlpha) * t;

    uint32_t skyboxId = src->m_skyboxes[0].m_id;

    if (skyboxId && t > 0.0f) {
        for (int32_t i = 0; i < 3; i++) {
            CurrentLight::SkyboxBlend& skybox = dst->m_skyboxes[i];

            if (skybox.m_id == skyboxId) {
                skybox.m_weight += t;

                if (skybox.m_weight > 1.0f) {
                    skybox.m_weight = 1.0f;
                }

                break;
            }

            if (!skybox.m_id) {
                skybox.m_weight = t;
                skybox.m_id = skyboxId;
                break;
            }
        }
    }

    dst->m_cloudWeight = t;
    dst->m_cloudTypeId = src->m_cloudTypeId;
}

// VERIFY: 41c99b68 auto
// OFFSET: 0x007ED790
CImVector DayNight::DarkenColor(CImVector color, float scale) {
    C3Vector rgb(color.r * (1.0f / 255.0f), color.g * (1.0f / 255.0f), (1.0f / 255.0f) * color.b);
    C3Vector hsv(0.0f, 0.0f, 0.0f);
    RGBtoHSV(rgb, hsv);
    hsv.z = hsv.z * scale;
    HSVtoRGB(hsv, rgb);

    CImVector result;
    result = rgb;
    return result;
}

// VERIFY: 2d4d0672 auto
// OFFSET: 0x007ED820
void DayNight::ClearOverrideFog() {
    if (s_fogOverride) {
        DayNight::g_dnInfo.m_bands.m_fogEnd = s_savedFogEnd;
        DayNight::g_dnInfo.m_bands.m_skyColors[5] = s_savedSkyFog;
        DayNight::g_dnInfo.m_bands.m_fogStartMul = s_savedFogStartMul;
        DayNight::g_dnInfo.m_showSky = s_savedShowSky;
        s_fogOverride = 0;
        DayNight::g_dnInfo.m_bands.m_fogRate = s_savedFogRate;
    }
}

// VERIFY: fd35bd95 auto
// OFFSET: 0x007ED870
void DayNight::SetOverrideFog(float startScalar, float end, CImVector color, int32_t showSky) {
    if (!s_fogOverride) {
        s_savedFogEnd = DayNight::g_dnInfo.m_bands.m_fogEnd;
        s_savedSkyFog = DayNight::g_dnInfo.m_bands.m_skyColors[5];
        s_savedFogStartMul = DayNight::g_dnInfo.m_bands.m_fogStartMul;
        s_savedShowSky = DayNight::g_dnInfo.m_showSky;
        s_savedFogRate = DayNight::g_dnInfo.m_bands.m_fogRate;
    }

    s_overrideFogEnd = end;
    s_overrideFogStart = startScalar;
    s_overrideFogRate = DayNight::CalcFogRate(startScalar * end, end);
    s_overrideFogColor = color;
    DayNight::g_dnInfo.m_showSky = showSky;
    s_fogOverride = 1;
}

// VERIFY: 57002cbf auto
// OFFSET: 0x007ED910
DayNight::CurrentLight& DayNight::CurrentLight::operator=(const CurrentLight& source) {
    this->m_ambient = source.m_ambient;
    this->m_diffuse = source.m_diffuse;
    this->m_shadowOpacity = source.m_shadowOpacity;
    this->m_skyColors[0] = source.m_skyColors[0];
    this->m_skyColors[1] = source.m_skyColors[1];
    this->m_skyColors[2] = source.m_skyColors[2];
    this->m_skyColors[3] = source.m_skyColors[3];
    this->m_skyColors[4] = source.m_skyColors[4];
    this->m_skyColors[5] = source.m_skyColors[5];
    this->m_sunCloudColors[0] = source.m_sunCloudColors[0];
    this->m_sunCloudColors[1] = source.m_sunCloudColors[1];
    this->m_sunCloudColors[2] = source.m_sunCloudColors[2];
    this->m_sunCloudColors[3] = source.m_sunCloudColors[3];
    this->m_sunCloudColors[4] = source.m_sunCloudColors[4];
    this->m_waterColors[0] = source.m_waterColors[0];
    this->m_waterColors[1] = source.m_waterColors[1];
    this->m_waterColors[2] = source.m_waterColors[2];
    this->m_waterColors[3] = source.m_waterColors[3];
    this->m_fogEnd = source.m_fogEnd;
    this->m_fogStartMul = source.m_fogStartMul;
    this->m_fogRate = source.m_fogRate;
    this->m_highlightSky = source.m_highlightSky;
    this->m_glow = source.m_glow;
    this->m_floatBand2 = source.m_floatBand2;
    this->m_cloudDensity = source.m_cloudDensity;
    this->m_floatBand4 = source.m_floatBand4;
    this->m_floatBand5 = source.m_floatBand5;
    this->m_waterShallowAlpha = source.m_waterShallowAlpha;
    this->m_waterDeepAlpha = source.m_waterDeepAlpha;
    this->m_oceanShallowAlpha = source.m_oceanShallowAlpha;
    this->m_oceanDeepAlpha = source.m_oceanDeepAlpha;
    this->m_skyboxes[0].m_id = source.m_skyboxes[0].m_id;
    this->m_skyboxes[0].m_weight = source.m_skyboxes[0].m_weight;
    this->m_skyboxes[1].m_id = source.m_skyboxes[1].m_id;
    this->m_skyboxes[1].m_weight = source.m_skyboxes[1].m_weight;
    this->m_skyboxes[2].m_id = source.m_skyboxes[2].m_id;
    this->m_skyboxes[2].m_weight = source.m_skyboxes[2].m_weight;
    this->m_cloudTypeId = source.m_cloudTypeId;
    this->m_cloudWeight = source.m_cloudWeight;
    return *this;
}

// VERIFY: 9e94a499 manual
// OFFSET: 0x007EDA90
void __stdcall DayNight::DNClouds::Collide(const C3Vector* origin, const C3Vector* dir, C3Vector* out) {
    float offset = -cos(0.7853981852531433);
    float r0;
    float r1;
    CMath::solvequad_(dir->x * dir->x + dir->y * dir->y + dir->z * dir->z, -offset * dir->z + -offset * dir->z, offset * offset - 1.0f, r0, r1);

    float x = dir->x * r1 + origin->x + DayNight::g_dnInfo.m_cameraPos.x;
    float y = dir->y * r1 + origin->y + DayNight::g_dnInfo.m_cameraPos.y;
    float z = r1 * dir->z + origin->z + DayNight::g_dnInfo.m_cameraPos.z;
    out->x = x;
    out->y = y;
    out->z = z;
}

// VERIFY: b394fc50 manual
// OFFSET: 0x007EDB50
void DayNight::DNClouds::SetSharpness(float sharpness) {
    this->m_sharpness = sharpness;

    float exponent = 0.0f;
    float step = (255 - this->m_density) * 0.00390625f;

    for (uint32_t i = 0; i < 256; i++) {
        s_cloudSharpness[i] = static_cast<uint8_t>(255.0 - pow(static_cast<double>(this->m_sharpness), static_cast<double>(exponent)) * 255.0);
        exponent += step;
    }
}

// VERIFY: 50e48cfa manual
// OFFSET: 0x007EDBE0
void DayNight::DNPlanet::GenGeometry(C3Vector* verts, C2Vector* texCoords, CImVector* colors, uint16_t* indices, uint32_t* vertCount, uint32_t* indexCount) {
    static C3Vector s_verts[6] = { C3Vector(0.0f, -0.5f, 0.5f), C3Vector(0.0f, 0.5f, 0.5f), C3Vector(0.0f, -0.5f, -0.5f), C3Vector(0.0f, 0.5f, -0.5f), C3Vector(0.0f, -0.5f, 99.0f), C3Vector(0.0f, 0.5f, 99.0f) };
    static C2Vector s_texCoords[6] = { C2Vector(0.0f, 0.0f), C2Vector(1.0f, 0.0f), C2Vector(0.0f, 1.0f), C2Vector(1.0f, 1.0f), C2Vector(0.0f, 99.0f), C2Vector(1.0f, 99.0f) };

    for (uint32_t i = 0; i < 6; i++) {
        float scale = this->m_scale;
        verts[i] = C3Vector(s_verts[i].x * scale, s_verts[i].y * scale, scale * s_verts[i].z);
        texCoords[i] = s_texCoords[i];
        colors[i] = this->m_color;
    }

    *vertCount = 4;
    memcpy(indices, s_planetIndices, sizeof(s_planetIndices));
    *indexCount = 4;
}

// VERIFY: 67289f76 auto
// OFFSET: 0x007EDEE0
void DayNight::DNPlanet::ClipGeometry(C3Vector* verts, C2Vector* texCoords, CImVector* colors, uint16_t* indices, uint32_t* vertCount, uint32_t* indexCount) {
    *vertCount = 0;

    float dz = this->m_position.z - DayNight::g_dnInfo.m_cameraPos.z;
    float top = verts[0].z + dz;
    float bottom = verts[2].z + dz;

    if (top > 0.0f && bottom > 0.0f) {
        *vertCount = 4;
    } else {
        if (top < 0.0f && bottom < 0.0f) {
            return;
        }

        *vertCount = 4;
        float t = top / (top - bottom);
        float z = (verts[2].z - verts[0].z) * t + verts[0].z;
        verts[2].z = z;
        verts[3].z = z;
        texCoords[2].y = t;
        texCoords[3].y = t;
    }

    float fadeTop = verts[0].z + dz - 0.40000001f;
    float fadeBottom = verts[2].z + dz - 0.40000001f;

    if (fadeTop > 0.001f && fadeBottom < 0.001f) {
        *vertCount = 6;
        *indexCount = 6;
        memcpy(indices, s_planetFadeIndices, sizeof(s_planetFadeIndices));

        float t = fadeTop / (fadeTop - fadeBottom);
        float z = (verts[2].z - verts[0].z) * t + verts[0].z;
        float v = t * (texCoords[2].y - texCoords[0].y) + texCoords[0].y;
        verts[4].z = z;
        verts[5].z = z;
        texCoords[4].y = v;
        texCoords[5].y = v;
    }

    for (uint32_t i = 0; i < *vertCount; i++) {
        float fade = dz + verts[i].z - 0.40000001f;

        if (fade < 0.001f) {
            float alpha = (0.40000001f + fade) * 2.5f;

            if (alpha < 0.0f) {
                alpha = 0.0f;
            } else if (alpha >= 1.0f) {
                alpha = 1.0f;
            }

            colors[i].a = static_cast<uint8_t>(alpha * 255.0f);
        }
    }
}

// VERIFY: 04178186 manual
// OFFSET: 0x007EE0D0
void DayNight::DNStars::Update() {
    this->m_position = DayNight::g_dnInfo.m_cameraPos;
    this->m_color.a = static_cast<uint8_t>(DayNight::InterpTable(s_starFadeTable, 4, DayNight::g_dnInfo.m_dayProgression) * 254.0f + 1.0f);
}

// VERIFY: ece2840b auto
// OFFSET: 0x007EE150
void DayNight::InitSunGlare() {
    DayNight::g_sunGlare.Initialize("Textures\\sunGlare.blp");
    DayNight::g_sunGlare.m_baseScale = 1.0f;
    DayNight::g_sunGlare.m_planet = &DayNight::g_planets.m_sun;
    DayNight::g_sunGlare.m_fadeOutRate = 1.5151515f;
    DayNight::g_sunGlare.m_enable = 1;
    DayNight::g_sunGlare.m_skipOcclusionOnce = 1;
    DayNight::g_sunGlare.m_fadeInRate = 4.0f;
    DayNight::g_sunGlare.m_alphaMin = 0.5f;
    DayNight::g_sunGlare.m_alphaMax = 1.0f;
    DayNight::g_sunGlare.m_scaleMin = 3.0f;
    DayNight::g_sunGlare.m_scaleMax = 20.0f;
    DayNight::g_sunGlare.m_minDot = 0.69999999f;
    DayNight::g_sunGlare.m_screenGlow = 0.0f;
    DayNight::g_sunGlare.m_alphaCurve[0].m_time = 0.27083334f;
    DayNight::g_sunGlare.m_alphaCurve[0].m_value = 0.0f;
    DayNight::g_sunGlare.m_alphaCurve[1].m_time = 0.3125f;
    DayNight::g_sunGlare.m_alphaCurve[1].m_value = 1.0f;
    DayNight::g_sunGlare.m_alphaCurve[2].m_time = 0.8125f;
    DayNight::g_sunGlare.m_alphaCurve[2].m_value = 1.0f;
    DayNight::g_sunGlare.m_alphaCurve[3].m_time = 0.875f;
    DayNight::g_sunGlare.m_alphaCurve[3].m_value = 0.0f;
}

// VERIFY: 1dc69a0b auto
// OFFSET: 0x007EE230
void DayNight::InitMoonGlare() {
    DayNight::g_moonGlare.Initialize("Textures\\moonGlare.blp");
    DayNight::g_moonGlare.m_baseScale = 2.0f;
    DayNight::g_moonGlare.m_planet = &DayNight::g_planets.m_moon;
    DayNight::g_moonGlare.m_fadeOutRate = 1.5151515f;
    DayNight::g_moonGlare.m_enable = 1;
    DayNight::g_moonGlare.m_skipOcclusionOnce = 1;
    DayNight::g_moonGlare.m_fadeInRate = 3.030303f;
    DayNight::g_moonGlare.m_alphaMin = 0.1f;
    DayNight::g_moonGlare.m_alphaMax = 1.0f;
    DayNight::g_moonGlare.m_scaleMin = 1.0f;
    DayNight::g_moonGlare.m_scaleMax = 1.0f;
    DayNight::g_moonGlare.m_minDot = 0.69999999f;
    DayNight::g_moonGlare.m_alphaCurve[0].m_time = 0.083333336f;
    DayNight::g_moonGlare.m_alphaCurve[0].m_value = 1.0f;
    DayNight::g_moonGlare.m_alphaCurve[1].m_time = 0.13541667f;
    DayNight::g_moonGlare.m_alphaCurve[1].m_value = 0.0f;
    DayNight::g_moonGlare.m_alphaCurve[2].m_time = 0.94791669f;
    DayNight::g_moonGlare.m_alphaCurve[2].m_value = 0.0f;
    DayNight::g_moonGlare.m_alphaCurve[3].m_time = 0.99930561f;
    DayNight::g_moonGlare.m_alphaCurve[3].m_value = 1.0f;
    DayNight::g_moonGlare.m_screenGlow = 0.0f;
}

// VERIFY: 38c6efb3 manual
// OFFSET: 0x007EE360
DayNight::CurrentLight::CurrentLight() {
    this->m_skyboxes[0].m_weight = 0.0f;
    this->m_skyboxes[1].m_weight = 0.0f;
    this->m_skyboxes[0].m_id = 0;
    this->m_skyboxes[2].m_weight = 0.0f;
    this->m_skyboxes[1].m_id = 0;
    this->m_skyboxes[2].m_id = 0;
    this->m_cloudWeight = 0.0f;
    this->m_cloudTypeId = 0;
    this->m_fogEnd = 0.0f;
    this->m_fogStartMul = 0.0f;
    this->m_fogRate = 0.0f;
    this->m_highlightSky = 0.0f;
    this->m_glow = 0.0f;
    this->m_floatBand2 = 0.0f;
    this->m_cloudDensity = 0.0f;
    this->m_floatBand4 = 0.0f;
    this->m_floatBand5 = 0.0f;
    this->m_waterShallowAlpha = 0.0f;
    this->m_waterDeepAlpha = 0.0f;
    this->m_oceanShallowAlpha = 0.0f;
    this->m_oceanDeepAlpha = 0.0f;
}

// VERIFY: bc27af6c manual
// OFFSET: 0x007EE510
void DayNight::BlendAreaLightColors(CurrentLight* dst, const LightRec* light, int32_t underwater) {
    float progression = DayNight::g_dnInfo.m_dayProgression * 2880.0f;
    int32_t time = CMath::fint_(progression - 0.5f);
    const LightParamsRec* params = DayNight::GetLightParams(light, underwater != 0);
    DayNight::CalcColors(time, dst, params);

    if (DayNight::g_dnInfo.m_stormBlend > 0.0f) {
        CurrentLight storm;
        progression = DayNight::g_dnInfo.m_dayProgression * 2880.0f;
        time = CMath::fint_(progression - 0.5f);
        params = DayNight::GetLightParams(light, (underwater != 0) + 2);
        DayNight::CalcColors(time, &storm, params);
        DayNight::BlendColors(dst, &storm, DayNight::g_dnInfo.m_stormBlend);
    }
}

// VERIFY: 5f29ba2d manual
// OFFSET: 0x007EE5D0
void DayNight::BlendAreaLight(CurrentLight* dst, const LightRec* light, int32_t underwater, float maxFade) {
    CurrentLight blend;
    DayNight::BlendAreaLightColors(&blend, light, underwater);

    float dx = DayNight::g_dnInfo.m_lightRefPos.x - light->m_gameCoords[0];
    float dy = DayNight::g_dnInfo.m_lightRefPos.y - light->m_gameCoords[1];
    float dz = DayNight::g_dnInfo.m_lightRefPos.z - light->m_gameCoords[2];
    float distance = sqrt(dx * dx + dz * dz + dy * dy);
    float weight = 1.0f;

    if (light->m_gameFalloffStart < distance) {
        float fade = 1.0f - (distance - light->m_gameFalloffStart) / (light->m_gameFalloffEnd - light->m_gameFalloffStart);
        weight = fade >= 0.0f ? fade : 0.0f;
    }

    DayNight::BlendOutputs(dst, &blend, maxFade <= weight ? maxFade : weight);
}

// VERIFY: f7e06820 manual
// OFFSET: 0x007EE6B0
void DayNight::BlendZoneLight(CurrentLight* dst, const LightRec* light, int32_t underwater, float maxFade, float distPct) {
    CurrentLight blend;
    DayNight::BlendAreaLightColors(&blend, light, underwater);

    float fade = (100.0f - distPct) * 0.0099999998f;
    float weight;

    if (fade <= 0.0f) {
        weight = 1.0f;
    } else {
        weight = 1.0f - fade;

        if (weight < 0.0f) {
            weight = 0.0f;
        }
    }

    DayNight::BlendOutputs(dst, &blend, maxFade <= weight ? maxFade : weight);
}

// VERIFY: b8de4a4e manual
// OFFSET: 0x007EE750
void DayNight::SetLightColors() {
    DNInfo& info = DayNight::g_dnInfo;
    const CurrentLight& bands = info.m_bands;

    info.m_light1.m_diffuse = bands.m_diffuse;

    CImVector toDiffuse;
    toDiffuse.b = static_cast<uint8_t>(bands.m_ambient.b + ((static_cast<uint32_t>(bands.m_diffuse.b - bands.m_ambient.b) << 7) >> 8));
    toDiffuse.g = static_cast<uint8_t>(bands.m_ambient.g + ((static_cast<uint32_t>(bands.m_diffuse.g - bands.m_ambient.g) << 7) >> 8));
    toDiffuse.r = static_cast<uint8_t>(bands.m_ambient.r + ((static_cast<uint32_t>(bands.m_diffuse.r - bands.m_ambient.r) << 7) >> 8));
    uint32_t mid = *reinterpret_cast<uint32_t*>(&toDiffuse);
    uint32_t sum = mid + 0xFF101010;
    uint32_t carry = (mid ^ 0xFF101010 ^ sum) & 0x01010100;
    uint32_t bright = (sum - carry) | (carry - (carry >> 8));

    CImVector toAmbient;
    toAmbient.b = static_cast<uint8_t>(bands.m_diffuse.b + ((static_cast<uint32_t>(bands.m_ambient.b - bands.m_diffuse.b) << 7) >> 8));
    toAmbient.g = static_cast<uint8_t>(bands.m_diffuse.g + ((static_cast<uint32_t>(bands.m_ambient.g - bands.m_diffuse.g) << 7) >> 8));
    toAmbient.r = static_cast<uint8_t>(bands.m_diffuse.r + ((static_cast<uint32_t>(bands.m_ambient.r - bands.m_diffuse.r) << 7) >> 8));

    info.m_light0.m_dir = info.m_light1.m_dir;
    info.m_light1.m_ambient = bands.m_ambient;
    *reinterpret_cast<uint32_t*>(&info.m_light1.m_mid1) = bright;
    info.m_light1.m_mid0 = toAmbient;
    info.m_light0.m_mid0 = info.m_light1.m_mid0;
    info.m_light0.m_desatR = info.m_light1.m_desatR;
    info.m_light0.m_desatG = info.m_light1.m_desatG;
    info.m_light0.m_desatB = info.m_light1.m_desatB;
    info.m_light0.m_desatA = info.m_light1.m_desatA;
    info.m_light0.m_diffuse = bands.m_diffuse;
    info.m_light0.m_ambient = bands.m_ambient;

    CImVector shadow;
    shadow.b = static_cast<uint8_t>((85 * (bands.m_ambient.b + 3)) >> 8);
    shadow.g = static_cast<uint8_t>((85 * (bands.m_ambient.g + 3)) >> 8);
    shadow.r = static_cast<uint8_t>((85 * (bands.m_ambient.r + 3)) >> 8);
    shadow.a = bands.m_ambient.a;
    info.m_shadowColor = shadow;
    info.m_shadowColor.a = bands.m_shadowOpacity.r;
    info.m_light0.m_mid1 = info.m_light1.m_mid1;

    C3Vector rgb(bands.m_ambient.r * (1.0f / 255.0f), bands.m_ambient.g * (1.0f / 255.0f), (1.0f / 255.0f) * bands.m_ambient.b);
    C3Vector hsv(0.0f, 0.0f, 0.0f);
    RGBtoHSV(rgb, hsv);
    hsv.y = hsv.y * SATURATION_SCALE;
    hsv.z = hsv.z * VALUE_SCALE;
    HSVtoRGB(hsv, rgb);
    info.m_light1.m_desatR = rgb.x;
    info.m_light1.m_desatG = rgb.y;
    info.m_light1.m_desatB = rgb.z;
    info.m_light1.m_desatA = 1.0f;
}

// VERIFY: 8ddbd15b manual
// OFFSET: 0x007EE9B0
static void ApplyFogColorBlend(DNFogInfo* fog) {
    fog->m_start = DayNight::s_glowBlend * fog->m_start;
    fog->m_end = fog->m_end * DayNight::s_glowBlend;
    fog->m_density = fog->m_density * DayNight::s_glowBlend;
    fog->m_color = DayNight::InterpColor(fog->m_color, s_glowColor, 1.0f - DayNight::s_glowBlend);
}

// VERIFY: 75393bb8 manual
// OFFSET: 0x007EEA10
static void ApplyFogEclipse(DNFogInfo* fog) {
    uint8_t scale = CMath::fint_((1.0f - s_eclipseAmount) * 255.0f);
    fog->m_color.r = static_cast<uint8_t>((fog->m_color.r * scale + 255) >> 8);
    fog->m_color.g = static_cast<uint8_t>((fog->m_color.g * scale + 255) >> 8);
    fog->m_color.b = static_cast<uint8_t>((fog->m_color.b * scale + 255) >> 8);
}

// VERIFY: 2bd27675 manual
// OFFSET: 0x007EEA90
void DayNight::SetDirection() {
    static C2Vector s_elevationTable[4] = { C2Vector(0.0f, 2.2165682f), C2Vector(0.25f, 1.9198623f), C2Vector(0.5f, 2.2165682f), C2Vector(0.75f, 1.9198623f) };
    static C2Vector s_azimuthTable[4] = { C2Vector(0.0f, 3.926991f), C2Vector(0.25f, 3.926991f), C2Vector(0.5f, 3.926991f), C2Vector(0.75f, 3.926991f) };

    float elevation = DayNight::InterpTable(s_elevationTable, 4, DayNight::g_dnInfo.m_dayProgression);
    float azimuth = DayNight::InterpTable(s_azimuthTable, 4, DayNight::g_dnInfo.m_dayProgression);
    float sinElevation = CMath::sin_(elevation);
    float cosElevation = CMath::cos_(elevation);
    float sinAzimuth = CMath::sin_(azimuth);
    float cosAzimuth = CMath::cos_(azimuth);

    DayNight::g_dnInfo.m_light1.m_dir.x = cosAzimuth * sinElevation;
    DayNight::g_dnInfo.m_light1.m_dir.y = sinElevation * sinAzimuth;
    DayNight::g_dnInfo.m_light1.m_dir.z = cosElevation;
}

// VERIFY: 3766e44c manual
// OFFSET: 0x007EECC0
void DayNight::SetPlanets() {
    static C2Vector s_sunTheta[5] = { C2Vector(SUNRISE, 1.7453293f), C2Vector(0.49652779f, 0.087266468f), C2Vector(NOON, 0.087266468f), C2Vector(0.50347221f, 0.087266468f), C2Vector(SUNSET, 1.7453293f) };
    static C2Vector s_sunPhi[3] = { C2Vector(SUNRISE, 0.78539819f), C2Vector(NOON, 0.78539819f), C2Vector(SUNSET, 0.78539819f) };
    static C2Vector s_moonTheta[5] = { C2Vector(MIDNIGHT, 0.61086524f), C2Vector(0.0034722222f, 0.61086524f), C2Vector(MOONSET, 1.7453293f), C2Vector(MOONRISE, 1.7453293f), C2Vector(0.99652779f, 0.61086524f) };
    static C2Vector s_moonPhi[3] = { C2Vector(MIDNIGHT, 0.78539819f), C2Vector(MOONSET, 0.78539819f), C2Vector(MOONRISE, 0.78539819f) };
    static C2Vector s_moon2Theta[5] = { C2Vector(MIDNIGHT, 0.61086524f), C2Vector(0.0034722222f, 0.61086524f), C2Vector(MOONSET, 1.7453293f), C2Vector(MOONRISE, 1.7453293f), C2Vector(0.99652779f, 0.61086524f) };
    static C2Vector s_moon2Phi[3] = { C2Vector(MIDNIGHT, 2.3561945f), C2Vector(MOONSET, 2.6179938f), C2Vector(MOONRISE, 2.8797934f) };
    static C2Vector s_sunScale[4] = { C2Vector(0.25f, 2.0f), C2Vector(0.28125f, 1.0f), C2Vector(0.84375f, 1.0f), C2Vector(0.875f, 2.0f) };
    static C2Vector s_moonScale[4] = { C2Vector(0.041666672f, 1.0f), C2Vector(MOONSET, 1.5f), C2Vector(MOONRISE, 1.5f), C2Vector(0.99930561f, 1.0f) };

    DNInfo& info = DayNight::g_dnInfo;
    DNPlanets& planets = DayNight::g_planets;

    float theta = DayNight::InterpTable(s_sunTheta, 5, info.m_dayProgression);
    float phi = DayNight::InterpTable(s_sunPhi, 3, info.m_dayProgression);
    float sinTheta = CMath::sin_(theta);
    float cosTheta = CMath::cos_(theta);
    float sinPhi = CMath::sin_(phi);
    float cosPhi = CMath::cos_(phi);
    float x = cosPhi * sinTheta;
    float y = sinTheta * sinPhi;
    float scale = 12.0f / sqrtf(x * x + cosTheta * cosTheta + y * y);
    planets.m_sun.m_position.x = x * scale + info.m_cameraPos.x;
    planets.m_sun.m_position.y = y * scale + info.m_cameraPos.y;
    planets.m_sun.m_position.z = cosTheta * scale + info.m_cameraPos.z;
    planets.m_sun.m_scale = DayNight::InterpTable(s_sunScale, 4, info.m_dayProgression) * planets.m_sun.m_baseScale;
    DayNight::g_sunGlare.m_position = planets.m_sun.m_position;

    theta = DayNight::InterpTable(s_moonTheta, 5, info.m_dayProgression);
    phi = DayNight::InterpTable(s_moonPhi, 3, info.m_dayProgression);
    sinTheta = CMath::sin_(theta);
    cosTheta = CMath::cos_(theta);
    sinPhi = CMath::sin_(phi);
    cosPhi = CMath::cos_(phi);
    x = cosPhi * sinTheta;
    y = sinTheta * sinPhi;
    scale = 12.0f / sqrtf(cosTheta * cosTheta + y * y + x * x);
    planets.m_moon.m_position.x = x * scale + info.m_cameraPos.x;
    planets.m_moon.m_position.y = y * scale + info.m_cameraPos.y;
    planets.m_moon.m_position.z = cosTheta * scale + info.m_cameraPos.z;
    planets.m_moon.m_scale = DayNight::InterpTable(s_moonScale, 4, info.m_dayProgression) * planets.m_moon.m_baseScale;
    DayNight::g_moonGlare.m_scaleMin = planets.m_moon.m_scale;
    DayNight::g_moonGlare.m_position = planets.m_moon.m_position;
    DayNight::g_moonGlare.m_scaleMax = planets.m_moon.m_scale;

    int32_t day = CMath::fint_(info.m_day * 65536.0f - 0.5f);
    int32_t progression = CMath::fint_(info.m_dayProgression * 65536.0f - 0.5f);
    int32_t total = progression + day;
    int32_t cycleStart = CMath::fint_(floorf((info.m_day + info.m_dayProgression) / planets.m_moon2.m_period) * planets.m_moon2.m_period * 65536.0f - 0.5f);

    if (cycleStart >= total) {
        cycleStart = total;
    }

    float cycle = (total - cycleStart) * 0.000015258789f / planets.m_moon2.m_period;
    theta = DayNight::InterpTable(s_moon2Theta, 5, cycle);
    phi = DayNight::InterpTable(s_moon2Phi, 3, cycle);
    sinTheta = CMath::sin_(theta);
    cosTheta = CMath::cos_(theta);
    sinPhi = CMath::sin_(phi);
    cosPhi = CMath::cos_(phi);
    x = cosPhi * sinTheta;
    y = sinTheta * sinPhi;
    scale = 12.0f / sqrtf(y * y + x * x + cosTheta * cosTheta);
    planets.m_moon2.m_position.x = x * scale + info.m_cameraPos.x;
    planets.m_moon2.m_position.y = y * scale + info.m_cameraPos.y;
    planets.m_moon2.m_position.z = cosTheta * scale + info.m_cameraPos.z;
    planets.m_moon2.m_scale = DayNight::InterpTable(s_moonScale, 4, cycle) * planets.m_moon2.m_baseScale;

    float time = info.m_dayProgression;

    if (time >= SUNRISE && time < NOON) {
        info.m_sunMoonPath = (time - SUNRISE) * 3.6923077f;
        return;
    }

    if (time >= NOON && time < SUNSET) {
        info.m_sunMoonPath = 1.0f - (time - NOON) * 2.5263159f;
    } else if (time >= MOONRISE && time < DAYEND) {
        info.m_sunMoonPath = (time - MOONRISE) * 12.000003f;
    } else if (time >= MIDNIGHT && time < MOONSET) {
        info.m_sunMoonPath = 1.0f - time * 6.0f;
    } else {
        info.m_sunMoonPath = 0.0f;
    }
}

// VERIFY: ecb3bec4 manual
// OFFSET: 0x007EF920
void DayNight::DNClouds::WorldToTexture(const C3Vector* world, C2Vector* uv) {
    C3Vector dir;
    dir.x = world->x - DayNight::g_dnInfo.m_cameraPos.x;
    dir.y = world->y - DayNight::g_dnInfo.m_cameraPos.y;
    dir.z = static_cast<float>(world->z - DayNight::g_dnInfo.m_cameraPos.z - -cos(0.7853981852531433));

    C3Vector up(0.0f, 0.0f, 1.0f);
    float angle = static_cast<float>(acos((dir.x * up.x + dir.y * up.y + dir.z * up.z) / (sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z) * sqrt(up.x * up.x + up.y * up.y + up.z * up.z))));

    if (angle > 0.78539819f) {
        angle = 0.78539819f;
    }

    float radius = angle * 1.2732395f * 0.5f;
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
    float nx;
    float ny;

    if (length <= 0.000009999999f) {
        ny = 0.0f;
        nx = 0.0f;
    } else {
        float inv = 1.0f / length;
        nx = dir.x * inv;
        ny = dir.y * inv;
    }

    uv->x = (nx * radius + 0.5f) * this->m_size;
    uv->y = (radius * ny + 0.5f) * this->m_size;
}

// VERIFY: 7f090860 manual
// OFFSET: 0x007EFA30
float DayNight::DNClouds::GetDensity(const C3Vector* position, float scale) {
    if (!this->m_enabled) {
        return 0.0f;
    }

    C2Vector uv;
    uv.x = 0.0f;
    uv.y = 0.0f;
    this->WorldToTexture(position, &uv);
    uint32_t max = this->m_size - 1;
    uint32_t x = static_cast<uint32_t>(static_cast<int64_t>(uv.x));
    uint32_t y = static_cast<uint32_t>(static_cast<int64_t>(uv.y));

    if (x > max) {
        x = ~(static_cast<int32_t>(x) >> 31) & max;
    }

    if (y > max) {
        y = ~(static_cast<int32_t>(y) >> 31) & max;
    }

    return this->m_densities.m_data[(y << this->m_shift) + x] * 0.0039215689f;
}

// VERIFY: e28145b2 manual
// OFFSET: 0x007EFAE0
void DayNight::DNClouds::SetupBumpMap(C3Vector* shadeColor, C3Vector* sunColor, C3Vector* baseColor, C3Vector* sunPos, float* intensity) {
    DNInfo& info = DayNight::g_dnInfo;
    C3Vector light;

    if (info.m_dayProgression < 0.2013889f || info.m_dayProgression > 0.9236111f) {
        light = DayNight::g_planets.m_moon.m_position;
    } else {
        light = DayNight::g_planets.m_sun.m_position;
    }

    C3Vector dir(light.x - info.m_cameraPos.x, light.y - info.m_cameraPos.y, light.z - info.m_cameraPos.z);
    C3Vector origin(0.0f, 0.0f, 0.0f);
    C3Vector hit(0.0f, 0.0f, 0.0f);
    C2Vector uv(0.0f, 0.0f);

    DayNight::DNClouds::Collide(&origin, &dir, &hit);
    this->WorldToTexture(&hit, &uv);

    info.m_cloudSun = uv;
    float inv = 1.0f / this->m_size;
    info.m_cloudSun.x = inv * uv.x;
    info.m_cloudSun.y = inv * info.m_cloudSun.y;

    sunPos->x = uv.x;
    sunPos->y = uv.y;
    sunPos->z = 64.0f;

    const CImVector& shade = info.m_bands.m_sunCloudColors[2];
    shadeColor->x = shade.r * 0.0039215689f;
    shadeColor->y = shade.g * 0.0039215689f;
    shadeColor->z = shade.b * 0.0039215689f;

    const CImVector& sun = info.m_bands.m_sunCloudColors[1];
    sunColor->x = sun.r * 0.0039215689f;
    sunColor->y = sun.g * 0.0039215689f;
    sunColor->z = sun.b * 0.0039215689f;

    const CImVector& base = info.m_bands.m_sunCloudColors[3];
    baseColor->x = base.r * 0.0039215689f;
    baseColor->y = base.g * 0.0039215689f;
    baseColor->z = 0.0039215689f * base.b;

    *intensity = DayNight::InterpTable(s_cloudLightCurve, 8, info.m_dayProgression) * *intensity;
    sunPos->z = info.m_stormBlend * 192.0f + sunPos->z;
    *intensity = (1.0f - info.m_stormBlend * 0.75f) * *intensity;
}

// VERIFY: 7ce3f2fa manual
// OFFSET: 0x007EFD00
void DayNight::DNClouds::UpdateTextureAnimation() {
    struct Octave {
        struct Corner {
            float base;
            float delta;
        };

        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint16_t dx;
        uint16_t dy;
        uint16_t unk0A;
        uint16_t zInt0;
        uint16_t yInt0;
        uint16_t zInt1;
        uint16_t xEnd;
        uint16_t yInt1;
        uint16_t zInt2;
        float amplitude;
        uint32_t perm[4];
        Corner corner[4];
        uint32_t xInt;
        uint32_t cachedXInt;
    };

    DNInfo& info = DayNight::g_dnInfo;

    if (!this->m_enabled) {
        this->m_fullUpdate = 0;
        return;
    }

    if (info.m_skybox && info.m_skyboxWeight > 0.99000001f) {
        this->m_fullUpdate = 0;
        return;
    }

    for (int32_t i = 0; i < 3; i++) {
        if (info.m_blendSkybox[i] && info.m_blendSkyboxWeight[i] > 0.99000001f && !info.m_blendSkyboxFlags[i]) {
            this->m_fullUpdate = 0;
            return;
        }
    }

    uint32_t rowsPerUpdate = this->m_rowsPerUpdate;

    if (this->m_fullUpdate) {
        this->m_row = 0;
        this->m_rowsPerUpdate = this->m_size;
    }

    this->m_animTime += info.m_deltaSec;

    float density = info.m_bands.m_cloudDensity;

    if (this->m_densityOverride != 0.0f) {
        density = this->m_densityOverride;
    }

    this->m_density = static_cast<uint8_t>((1.0f - density) * 255.0f);

    uint16_t zInt = this->m_animFrame >> 8;
    uint32_t permZ0 = s_cloudPerm[zInt];
    uint32_t permZ1 = s_cloudPerm[(zInt + 1) & 0xFF];

    Octave octaves[5];
    const uint16_t* freq = s_cloudOctaveFreq[this->m_lod];

    for (uint32_t i = 0; i < this->m_octaves; i++) {
        Octave& octave = octaves[i];
        uint16_t f = *freq++;
        octave.dy = f;
        octave.dx = f;
        octave.y = static_cast<uint16_t>(f * static_cast<uint16_t>(this->m_row));
        octave.zInt0 = zInt;
        octave.xEnd = static_cast<uint16_t>((static_cast<int32_t>(f << this->m_shift) >> 8) + zInt + 2);
        octave.zInt2 = zInt + 1;
        octave.zInt1 = zInt;
        octave.z = this->m_animFrame;
        octave.amplitude = 1.0f / (1 << i);
    }

    C3Vector shadeColor(0.0f, 0.0f, 0.0f);
    C3Vector sunColor(0.0f, 0.0f, 0.0f);
    C3Vector baseColor(0.0f, 0.0f, 0.0f);
    C3Vector sunPos(0.0f, 0.0f, 0.0f);
    float intensity = 1.0f;
    this->SetupBumpMap(&shadeColor, &sunColor, &baseColor, &sunPos, &intensity);

    float gradX = 0.0f;
    float gradY = 0.0f;

    for (uint32_t r = 0; r < this->m_rowsPerUpdate; r++) {
        uint32_t row = this->m_row + r;
        uint8_t* densities = this->m_densities.m_data + (row << this->m_shift);
        CImVector* texels = this->m_texels.m_data + (row << this->m_shift);

        for (uint32_t i = 0; i < this->m_octaves; i++) {
            Octave& octave = octaves[i];
            uint16_t y0 = octave.y >> 8;
            octave.yInt0 = y0;
            uint16_t y1 = (y0 + 1) & 0xFF;
            octave.yInt1 = y1;
            octave.perm[0] = s_cloudPerm[(permZ0 + y0) & 0xFF];
            octave.perm[1] = s_cloudPerm[(permZ0 + y1) & 0xFF];
            octave.perm[2] = s_cloudPerm[(permZ1 + y0) & 0xFF];
            octave.perm[3] = s_cloudPerm[(permZ1 + y1) & 0xFF];
            octave.cachedXInt = 0xFFFFFFFF;
            octave.x = this->m_animFrame;
        }

        float prev = 0.0f;
        float sum = 0.0f;
        float rowPos = static_cast<float>(row);

        for (uint32_t col = 0; col < this->m_size; col++) {
            for (uint32_t i = 0; i < this->m_octaves; i++) {
                Octave& octave = octaves[i];
                uint32_t xInt = octave.x >> 8;
                uint32_t xFrac = octave.x & 0xFF;
                octave.xInt = xInt;

                if (xInt != octave.cachedXInt) {
                    octave.cachedXInt = xInt;

                    float v0 = s_cloudNoise[s_cloudPerm[(octave.perm[0] + xInt) & 0xFF]];
                    octave.corner[0].base = v0;
                    octave.corner[0].delta = s_cloudNoise[s_cloudPerm[(octave.perm[0] + xInt + 1) & 0xFF]] - v0;

                    float v1 = s_cloudNoise[s_cloudPerm[(octave.perm[1] + xInt) & 0xFF]];
                    octave.corner[1].base = v1;
                    octave.corner[1].delta = s_cloudNoise[s_cloudPerm[(octave.perm[1] + xInt + 1) & 0xFF]] - v1;

                    float v2 = s_cloudNoise[s_cloudPerm[(octave.perm[2] + xInt) & 0xFF]];
                    octave.corner[2].base = v2;
                    octave.corner[2].delta = s_cloudNoise[s_cloudPerm[(octave.perm[2] + xInt + 1) & 0xFF]] - v2;

                    float v3 = s_cloudNoise[s_cloudPerm[(octave.perm[3] + xInt) & 0xFF]];
                    octave.corner[3].base = v3;
                    octave.corner[3].delta = s_cloudNoise[s_cloudPerm[(octave.perm[3] + xInt + 1) & 0xFF]] - v3;
                }

                float fx = s_cloudFade[xFrac];
                float fy = s_cloudFade[octave.y & 0xFF];
                float a = octave.corner[0].delta * fx + octave.corner[0].base;
                float b = octave.corner[2].delta * fx + octave.corner[2].base;
                float c = octave.corner[1].delta * fx + octave.corner[1].base;
                a = a + (c - a) * fy;
                float d = octave.corner[3].delta * fx + octave.corner[3].base;
                b = b + (d - b) * fy;
                octave.x += octave.dx;
                sum += (a + (b - a) * s_cloudFade[octave.z & 0xFF]) * octave.amplitude;

                if (i == 2) {
                    float scale = static_cast<float>(1 << (this->m_shift - 7));
                    float* prevRow = &this->m_prevRow.m_data[col];
                    gradX = (prev - sum) * scale;
                    gradY = (*prevRow - sum) * scale;
                    *prevRow = sum;
                    prev = sum;
                }
            }

            uint8_t level = static_cast<uint8_t>(CMath::fint_(sum * 64.0f + 128.0f));
            int32_t index = level - this->m_density;
            uint8_t alpha = index < 0 ? 0 : s_cloudSharpness[index];
            densities[col] = alpha;

            if (!alpha) {
                if (col >= 1) {
                    texels[col] = texels[col - 1];
                    texels[col].a = alpha;
                }
            } else {
                float sunX = sunPos.x - static_cast<float>(col);
                float sunY = sunPos.y - rowPos;
                float sunZ = sunPos.z;

                uint8_t shadeLevel = ((static_cast<uint8_t>(0xFF - alpha)) >> 1) + 0x40;
                float shade = shadeLevel * 0.0039215689f;
                float red = shadeColor.x * shade + baseColor.x;
                float green = shadeColor.y * shade + baseColor.y;
                float blue = shadeColor.z * shade + baseColor.z;

                float sunLengthSq = sunZ * sunZ + sunY * sunY + sunX * sunX;
                uint32_t bits;
                memcpy(&bits, &sunLengthSq, sizeof(bits));
                bits = 0x5F3997BB - ((bits >> 1) & 0x3FFFFFFF);
                float sunInvLength;
                memcpy(&sunInvLength, &bits, sizeof(bits));

                float normalLengthSq = gradY * gradY + gradX * gradX + 1.0f;
                memcpy(&bits, &normalLengthSq, sizeof(bits));
                bits = 0x5F3997BB - ((bits >> 1) & 0x3FFFFFFF);
                float normalInvLength;
                memcpy(&normalInvLength, &bits, sizeof(bits));

                float dot = (sunZ + (gradX * sunX + gradY * sunY)) * (normalInvLength * sunInvLength);

                if (dot > 0.0f) {
                    dot *= intensity;
                    red += sunColor.x * dot;
                    green = sunColor.y * dot + green;
                    blue = dot * sunColor.z + blue;
                }

                if (red > 1.0f) {
                    red = 1.0f;
                }

                if (green > 1.0f) {
                    green = 1.0f;
                }

                if (blue > 1.0f) {
                    blue = 1.0f;
                }

                texels[col].r = static_cast<uint8_t>(CMath::fint_(red * 255.0f));
                texels[col].g = static_cast<uint8_t>(CMath::fint_(green * 255.0f));
                texels[col].b = static_cast<uint8_t>(CMath::fint_(blue * 255.0f));
                texels[col].a = densities[col];
            }

            sum = 0.0f;
        }

        for (uint32_t i = 0; i < this->m_octaves; i++) {
            octaves[i].y += octaves[i].dy;
        }
    }

    CGxTex* gxTex = TextureGetGxTex(this->m_textures[(this->m_texIndex - 1) & 1], 1, nullptr);

    if (gxTex) {
        GxTexUpdate(gxTex, 0, this->m_row, this->m_size, this->m_row + this->m_rowsPerUpdate, 1);
    }

    this->m_row += this->m_rowsPerUpdate;

    if (this->m_row >= this->m_size) {
        uint16_t frame = static_cast<uint16_t>(static_cast<int32_t>(this->m_animSpeed * this->m_animTime));

        if (frame != this->m_animFrame) {
            this->m_animFrame = frame;
            this->m_texIndex = (this->m_texIndex - 1) & 1;
        } else if (this->m_fullUpdate) {
            this->m_texIndex = (this->m_texIndex - 1) & 1;
        }

        this->m_row = 0;
    }

    if (this->m_fullUpdate) {
        this->m_rowsPerUpdate = rowsPerUpdate;
        this->m_fullUpdate = 0;
    }
}

// VERIFY: de50e0e5 manual
// OFFSET: 0x007F04B0
DayNight::DNClouds::DNClouds() {
    InitCloudNoise();
    this->m_animTime = 0.0f;
    this->m_texIndex = 0;
    this->m_fullUpdate = 0;
    this->m_animSpeed = 2.0f;
    this->m_octaves = 4;
    this->m_texFormat = GxTex_Argb8888;
    this->m_rowsPerUpdate = 8;
}

// VERIFY: 8ab405d3 manual
// OFFSET: 0x007F0530
void DayNight::DNSky::SetColors() {
    DNInfo& info = DayNight::g_dnInfo;
    CImVector* colors = this->m_clrVerts.m_data;
    CImVector ramp[6] = {};
    float highlight = DayNight::InterpTable(s_highlightCurve, 6, info.m_dayProgression) * info.m_bands.m_highlightSky;

    if (DayNight::s_lightFlags & 1) {
        for (int32_t i = 0; i < 6; i++) {
            uint8_t fraction = CMath::fint_((1.0f - DayNight::s_glowBlend) * 255.0f);
            CImVector glow = s_glowColor;

            if (fraction) {
                info.m_bands.m_skyColors[i].InterpColor(fraction, &glow);
            }
        }
    }

    for (int32_t i = 0; i < 5; i++) {
        CImVector color;
        DayNight::ColorLerpBytes(&color, &info.m_bands.m_skyColors[1 + i], &info.m_bands.m_skyColors[1], highlight);
        ramp[1 + i] = color;
    }

    *colors++ = DayNight::DarkenColor(info.m_bands.m_skyColors[0], 1.0f);

    float step = -1.0f / static_cast<int32_t>(this->m_sphThetaTess);

    for (int32_t row = 1; row <= 4; row++) {
        float angle = info.m_faceAngle * 0.15915494f + 0.25f;

        if (angle > 1.0f) {
            angle -= 1.0f;
        }

        for (int32_t i = 0; i < static_cast<int32_t>(this->m_sphThetaTess); i++) {
            if (angle < 0.0f) {
                angle += 1.0f;
            }

            float face = DayNight::InterpTable(s_skyFaceCurve, 6, angle);
            CImVector color;

            if (face >= 0.0f) {
                DayNight::ColorLerpBytes(&color, &info.m_bands.m_skyColors[row], &ramp[row], (1.0f - face) * highlight);
            } else {
                CImVector dark;
                DayNight::ColorLerpBytes(&dark, &ramp[row], &info.m_bands.m_skyColors[0], highlight * 0.69999999f);
                DayNight::ColorLerpBytes(&color, &ramp[row], &dark, -face * highlight);
            }

            if (info.m_flashBlend) {
                CImVector flash = info.m_flashColor;
                color.InterpColor(info.m_flashBlend, &flash);
            }

            *colors++ = color;
            angle += step;
        }
    }

    CImVector fog = info.m_bands.m_skyColors[5];

    if (info.m_flashBlend) {
        CImVector flash = info.m_flashColor;
        fog.InterpColor(info.m_flashBlend, &flash);
    }

    for (int32_t i = 0; i < static_cast<int32_t>(this->m_sphThetaTess); i++) {
        *colors++ = fog;
    }

    fog = info.m_bands.m_skyColors[5];

    if (info.m_flashBlend) {
        CImVector flash = info.m_flashColor;
        fog.InterpColor(info.m_flashBlend, &flash);
    }

    *colors = fog;
}

// VERIFY: 3e4634ac manual
// OFFSET: 0x007F08C0
void DayNight::DrawSky(DNOverrideSky* sky, float weight) {
    if (!sky || !sky->m_model || !sky->m_model->IsDrawable(0, 0) || weight <= 0.0f) {
        return;
    }

    sky->m_model->SetAnimating(1);

    CM2Model* model = sky->m_model;
    CM2Model* parent = model->m_attachParent;

    if (parent) {
        model->m_fFlags |= 0x80;
    } else {
        model->m_fFlags |= 0x8;
    }

    if (parent) {
        model->m_fFlags |= 0x20000;
    } else {
        model->m_fFlags |= 0x10000;
    }

    sky->m_model->m_fadeAlpha = weight;
    s_skyScene->Animate(C3Vector(0.0f, 0.0f, 0.0f));
    s_skyScene->Draw(M2PASS_0);
    s_skyScene->Draw(M2PASS_1);
    sky->m_model->SetAnimating(0);

    model = sky->m_model;
    parent = model->m_attachParent;

    if (parent) {
        model->m_fFlags &= ~0x80u;
    } else {
        model->m_fFlags &= ~0x8u;
    }

    if (parent) {
        model->m_fFlags &= ~0x20000u;
    } else {
        model->m_fFlags &= ~0x10000u;
    }
}

// VERIFY: 77bb1be4 manual
// OFFSET: 0x007F09B0
void DayNight::RenderSky(CPortalView* view) {
    DNInfo& info = DayNight::g_dnInfo;

    if (!info.m_showSky) {
        return;
    }

    float minX = g_theGxDevicePtr->m_viewport.m_x.m_l;
    float maxX = g_theGxDevicePtr->m_viewport.m_x.m_h;
    float minY = g_theGxDevicePtr->m_viewport.m_y.m_l;
    float maxY = g_theGxDevicePtr->m_viewport.m_y.m_h;
    float minZ = g_theGxDevicePtr->m_viewport.m_z.m_l;
    float maxZ = g_theGxDevicePtr->m_viewport.m_z.m_h;

    CRect screen;
    screen.minY = minY;
    screen.minX = minX;
    screen.maxY = maxY;
    screen.maxX = maxX;

    CRect rect = view->m_rect & screen;

    if (rect.maxY <= rect.minY || rect.maxX <= rect.minX) {
        return;
    }

    GxXformSetViewport(minX, maxX, minY, maxY, s_skyMinZ, s_skyMaxZ);
    GxRsSet(GxRs_ScissorTest, 1);
    g_theGxDevicePtr->SetScissor(rect);

    bool drawObjects = true;

    if (info.m_skybox && info.m_skybox->m_model && info.m_skybox->m_model->IsDrawable(0, 0) && info.m_skyboxWeight > 0.99000001f) {
        drawObjects = false;
    }

    for (int32_t i = 0; i < 3; i++) {
        DNOverrideSky* sky = info.m_blendSkybox[i];

        if (sky && sky->m_model && sky->m_model->IsDrawable(0, 0) && info.m_blendSkyboxWeight[i] > 0.99000001f && !info.m_blendSkyboxFlags[i]) {
            drawObjects = false;
        }
    }

    if (drawObjects) {
        DayNight::g_stars.Render();
        DayNight::g_planets.m_sun.Render();
        DayNight::g_planets.m_moon.Render();
        DayNight::g_planets.m_moon2.Render();
        DayNight::g_sky.Render();
        DayNight::g_clouds.Render();
    }

    if (s_skyScene) {
        uint32_t elapsed = OsGetAsyncTimeMs() - s_skySceneTimeMs;
        s_skyScene->AdvanceTime(elapsed);
        s_skySceneTimeMs += elapsed;

        int32_t minutes = g_clientGameTime.GetHourAndMinutes();

        for (int32_t i = 0; i < 3; i++) {
            DayNight::SyncSkyAnimWithTime(info.m_blendSkybox[i], minutes);
        }

        DayNight::SyncSkyAnimWithTime(info.m_skybox, minutes);

        if (!info.m_skybox || info.m_skyboxWeight < 1.0f) {
            for (int32_t i = 0; i < 3; i++) {
                DayNight::DrawSky(info.m_blendSkybox[i], info.m_blendSkyboxWeight[i]);
            }
        }

        if (info.m_skybox && info.m_skyboxWeight > 0.0f) {
            DayNight::DrawSky(info.m_skybox, info.m_skyboxWeight);
        }
    }

    GxRsSet(GxRs_ScissorTest, 0);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

// VERIFY: 9ccd52e0 manual
// OFFSET: 0x007F0D40
DayNight::LightQueue::LightQueue(const uint32_t* params) : CMemBlock(sizeof(LightQE) * params[0], 0, typeid(LightQE).raw_name(), SLOG_OBJECT) {
    uint32_t count = params[0];
    uint32_t chunk = params[1];

    this->m_chunk = chunk;
    this->m_capacity = count;
    this->m_count = 0;

    if (count) {
        this->m_count++;
        return;
    }

    if (!chunk) {
        return;
    }

    uint32_t grow = chunk;

    if (grow <= 1 - count) {
        grow = 1 - count;
    }

    if (this->Resize(sizeof(LightQE) * (grow + count), true)) {
        this->m_capacity += grow;
        this->m_count++;
    }
}

// VERIFY: 2cb38e62 manual
// OFFSET: 0x007F0DC0
void DayNight::LightQueue::Push(LightQE entry) {
    if (this->m_count + 1 <= this->m_capacity) {
        this->m_count++;
    } else if (this->m_chunk) {
        uint32_t grow = this->m_chunk;

        if (grow <= this->m_count - this->m_capacity + 1) {
            grow = this->m_count - this->m_capacity + 1;
        }

        if (this->Resize(sizeof(LightQE) * (grow + this->m_capacity), true)) {
            this->m_capacity += grow;
            this->m_count++;
        }
    }

    uint32_t index = this->m_count - 1;

    while (index > 1) {
        uint32_t parent = index >> 1;
        LightQE* data = reinterpret_cast<LightQE*>(this->m_data);

        if (!LightQE::HasHigherPriority(entry, data[parent])) {
            break;
        }

        data[index] = data[parent];
        index = parent;
    }

    reinterpret_cast<LightQE*>(this->m_data)[index] = entry;
}

// VERIFY: e8928dc7 auto
// OFFSET: 0x007F0E70
DayNight::DNInfo::DNInfo() {
    this->m_timeOfDay = 0;
    this->m_dayProgression = 0.0f;
    this->m_day = 0.0f;
    this->m_faceAngle = 0.0f;
    this->m_farClip = 0.0f;
    this->m_timeSec = 0.0f;
    this->m_deltaSec = 0.0f;
    this->m_weatherIntensity = 0.0f;
    this->m_flashBlend = 0;
    this->m_overrideLightParamsId = 0;
    this->m_skybox = nullptr;
    this->m_skyboxWeight = 0.0f;

    for (int32_t i = 0; i < 3; i++) {
        this->m_blendSkybox[i] = nullptr;
        this->m_blendSkyboxWeight[i] = 0.0f;
        this->m_blendSkyboxFlags[i] = 0;
    }

    this->m_stormBlend = 0.0f;
    this->m_interiorFogBlend = 0.0f;
    this->m_interiorMapObjDef = nullptr;
    this->m_showSky = 0;
    this->m_sunMoonPath = 0.0f;
    this->m_curve0 = 0.0f;
    this->m_curve1 = 0.0f;
    this->m_nearLightCount = 0;

    for (int32_t i = 0; i < 5; i++) {
        this->m_nearLightBlend[i] = 0.0f;
    }
}

// VERIFY: c02cdeae auto
// OFFSET: 0x007F1010
void DayNight::DNClouds::Update() {
    DayNight::g_clouds.UpdateTextureAnimation();
}

// VERIFY: d35f80ed manual
// OFFSET: 0x007F1070
void DayNight::ResetFogAccumulators(bool updateClouds) {
    DayNight::g_dnInfo.m_interiorFogBlend = 0.0f;
    DayNight::g_dnInfo.m_interiorMapObjDef = nullptr;
    DayNight::g_dnInfo.m_interiorGroups.m_count = 0;

    if (updateClouds) {
        DayNight::g_clouds.m_fullUpdate = 1;
    }
}

// VERIFY: 75e3ef0e manual
// OFFSET: 0x007F1280
DayNight::LightQE DayNight::LightQueue::Pop() {
    LightQE* data = reinterpret_cast<LightQE*>(this->m_data);
    LightQE top = data[1];
    LightQE last = data[this->m_count - 1];

    if (this->m_count) {
        this->Erase(this->m_count - 1, 1);
    }

    if (this->m_count < 2) {
        return top;
    }

    uint32_t limit = this->m_count - 1;
    uint32_t half = limit >> 1;
    uint32_t index = 1;

    while (index <= half) {
        uint32_t child = 2 * index;
        data = reinterpret_cast<LightQE*>(this->m_data);

        if (child < limit && LightQE::HasHigherPriority(data[child + 1], data[child])) {
            child++;
        }

        if (LightQE::HasHigherPriority(last, data[child])) {
            break;
        }

        data[index] = data[child];
        index = child;
    }

    reinterpret_cast<LightQE*>(this->m_data)[index] = last;

    return top;
}

// VERIFY: 882785f0 manual
// OFFSET: 0x007F1360
void DayNight::DoAreaLights(CurrentLight* light, int32_t underwater) {
    uint32_t params[2] = { 32, 32 };
    LightQueue queue(params);

    if (DayNight::g_dnInfo.m_nearLightCount) {
        if (s_useZoneLights) {
            for (int32_t i = 0; i < static_cast<int32_t>(DayNight::g_dnInfo.m_nearLightCount); i++) {
                DayNight::BlendZoneLight(light, DayNight::g_dnInfo.m_nearLight[i], underwater, 1.0f, DayNight::g_dnInfo.m_nearLightBlend[i]);
            }
        }

        for (int32_t i = 0; i < 5; i++) {
            DayNight::g_dnInfo.m_nearLight[i] = nullptr;
            DayNight::g_dnInfo.m_nearLightBlend[i] = 0.0f;
        }

        DayNight::g_dnInfo.m_nearLightCount = 0;
    }

    if (s_useAreaLights) {
        for (uint32_t i = 1; i < DayNight::g_areaLights.m_count; i++) {
            const LightRec** areaLight = &DayNight::g_areaLights.m_data[i];

            if (s_useZoneLights && (*areaLight)->m_gameFalloffEnd < 3.0f) {
                continue;
            }

            C3Vector position = GetLightPosition(areaLight);
            float dx = DayNight::g_dnInfo.m_lightRefPos.x - position.x;
            float dy = DayNight::g_dnInfo.m_lightRefPos.y - position.y;
            float dz = DayNight::g_dnInfo.m_lightRefPos.z - position.z;
            float distanceSq = dx * dx + dz * dz + dy * dy;

            if ((*areaLight)->m_gameFalloffEnd * (*areaLight)->m_gameFalloffEnd > distanceSq) {
                LightQE entry;
                entry.m_key = distanceSq;
                entry.m_light = areaLight;
                queue.Push(entry);
            }
        }
    }

    while (queue.m_count > 1) {
        LightQE entry = queue.Pop();
        DayNight::BlendAreaLight(light, *entry.m_light, underwater, 1.0f);
    }

    uint32_t count = s_areaLightOverrides.m_count;

    for (uint32_t i = 0; i < count; i++) {
        AreaLightOverride* override = &s_areaLightOverrides.m_data[i];
        uint32_t now = OsGetAsyncTimeMs();
        uint32_t elapsed = now - override->m_lastTime;
        override->m_lastTime = now;

        if (override->m_state == 1) {
            float fade = static_cast<float>(elapsed) / static_cast<float>(override->m_duration) + override->m_fade;
            override->m_fade = fade > 1.0f ? 1.0f : fade;
        } else if (override->m_state == 2) {
            float fade = override->m_fade - static_cast<float>(elapsed) / static_cast<float>(override->m_duration);
            override->m_fade = fade < 0.0f ? 0.0f : fade;
        }

        const LightRec* overrideLight = override->m_light;

        if (override->m_fade > 0.0f) {
            float dx = DayNight::g_dnInfo.m_lightRefPos.x - overrideLight->m_gameCoords[0];
            float dy = DayNight::g_dnInfo.m_lightRefPos.y - overrideLight->m_gameCoords[1];
            float dz = DayNight::g_dnInfo.m_lightRefPos.z - overrideLight->m_gameCoords[2];

            if (overrideLight->m_gameFalloffEnd * overrideLight->m_gameFalloffEnd > dx * dx + dy * dy + dz * dz) {
                DayNight::BlendAreaLight(light, overrideLight, underwater, override->m_fade);
            }
        }

        if (override->m_state == 2 && override->m_fade == 0.0f) {
            LightRec* rec = override->m_light;
            count--;
            rec->~LightRec();
            s_lightAllocator.PutData(rec, nullptr, 0);
            s_areaLightOverrides.m_data[i] = s_areaLightOverrides.m_data[count];
            s_areaLightOverrides.SetCount(count);
            i--;
        }
    }
}

// VERIFY: 729edd43 manual
// OFFSET: 0x007F16F0
void DayNight::UpdateFog() {
    DNInfo& info = DayNight::g_dnInfo;
    float end = info.m_farClip;

    if (s_fogOverride) {
        if (s_overrideFogEnd < end) {
            end = s_overrideFogEnd;
        }

        info.m_fog.m_end = end;
        info.m_fog.m_color = s_overrideFogColor;
        info.m_fog.m_start = end * s_overrideFogStart;
        info.m_fog.m_density = s_overrideFogRate;
    } else {
        if (info.m_bands.m_fogEnd < end) {
            end = info.m_bands.m_fogEnd;
        }

        info.m_fog.m_end = end;
        info.m_fog.m_color = info.m_bands.m_skyColors[5];
        info.m_fog.m_start = end * info.m_bands.m_fogStartMul;
        info.m_fog.m_density = info.m_bands.m_fogRate;
    }

    SMOFog fog;
    CMapObjDef* mapObjDef;
    uint8_t found;
    TSGrowableArray<uint32_t>* ids;
    float distance;
    bool inside = World::QueryCameraFog(&fog, &mapObjDef, &found, &ids, &distance) == 1;

    if (!inside) {
        distance = 0.0f;
    }

    int32_t liquidStatus = World::SceneCamLiquidStatus(nullptr);
    float density;

    if (inside) {
        const LiquidTypeRec* liquid = liquidStatus ? g_liquidTypeDB.GetRecord(liquidStatus) : nullptr;
        int32_t underwater = 0;
        bool apply = true;

        if (liquidStatus) {
            int32_t flags = liquid->m_flags;
            underwater = 1;

            if ((flags & 0x20) && !(fog.m_flags & 0x100)) {
                underwater = 0;
            }

            if (((flags & 0x100) && !(fog.m_flags & 0x10)) || !underwater) {
                apply = false;
            }
        }

        if (apply) {
            DayNight::ApplyFogSettings(underwater ? &fog.m_underwaterFog : &fog.m_fog);

            if (underwater && liquid && (liquid->m_flags & 0x40)) {
                info.m_fog.m_end = info.m_fogGroup.m_end;
                info.m_fog.m_color = info.m_fogGroup.m_color;
                info.m_fog.m_start = info.m_fogGroup.m_start;
                info.m_fog.m_density = info.m_fogGroup.m_density;
            }
        } else {
            info.m_fogGroup.m_start = info.m_fog.m_start;
            info.m_fogGroup.m_color = info.m_fog.m_color;
            info.m_fogGroup.m_end = info.m_fog.m_end;
            info.m_fogGroup.m_density = info.m_fog.m_density;
        }

        density = info.m_fog.m_density;

        if (found) {
            info.m_interiorMapObjDef = mapObjDef;

            if (ids != &info.m_interiorGroups) {
                info.m_interiorGroups.Set(ids->m_count, ids->m_data);
            }

            info.m_interiorGroups.m_chunk = ids->m_chunk;
        }
    } else {
        density = info.m_fog.m_density;
    }

    float blend = distance * 0.039999999f;

    if (blend < 0.0f) {
        blend = 0.0f;
    } else if (blend >= 1.0f) {
        blend = 1.0f;
    }

    info.m_interiorFogBlend = blend;
    info.m_interiorMapObjDef = nullptr;
    info.m_interiorGroups.m_count = 0;

    float interiorDensity;

    if (inside && found) {
        info.m_fogInterior.m_end = info.m_fog.m_end + (info.m_fogGroup.m_end - info.m_fog.m_end) * blend;
        info.m_fogInterior.m_start = info.m_fog.m_start + (info.m_fogGroup.m_start - info.m_fog.m_start) * blend;
        info.m_fogInterior.m_density = density + (info.m_fogGroup.m_density - density) * blend;
        info.m_fogInterior.m_color = DayNight::InterpColor(info.m_fog.m_color, info.m_fogGroup.m_color, blend);
        info.m_fog.m_end = info.m_fogInterior.m_end;
        info.m_fog.m_start = info.m_fogInterior.m_start;
        density = info.m_fogInterior.m_density;
        info.m_fog.m_density = density;
        interiorDensity = density;
    } else {
        info.m_fogInterior.m_color = info.m_fog.m_color;
        info.m_fogInterior.m_start = info.m_fog.m_start;
        info.m_fogInterior.m_end = info.m_fog.m_end;
        info.m_fogInterior.m_density = density;
        interiorDensity = density;
    }

    if (s_fogMode == 1 && liquidStatus) {
        info.m_fog.m_density = density * 2.0f;
        info.m_fogInterior.m_density = 2.0f * interiorDensity;
    }

    if (DayNight::s_lightFlags & 2) {
        ApplyFogEclipse(&info.m_fog);
        ApplyFogEclipse(&info.m_fogInterior);
    }

    if (DayNight::s_lightFlags & 1) {
        if (info.m_timeSec <= s_glowEndTime) {
            float fade = 1.0f / static_cast<float>(CMath::exp2_((info.m_timeSec - s_glowStartTime) / s_glowDuration * 7.2134752f));

            if (fade < 0.0f) {
                fade = 0.0f;
            } else if (fade >= 1.0f) {
                fade = 1.0f;
            }

            DayNight::s_glowBlend = 1.0f - fade;
            ApplyFogColorBlend(&info.m_fog);
            ApplyFogColorBlend(&info.m_fogInterior);
        } else {
            DayNight::s_lightFlags &= ~1;
        }
    }
}

// VERIFY: cb3314a8 manual
// OFFSET: 0x007F1B10
void DayNight::DNClouds::SetLOD(uint32_t lod, uint32_t rowsPerUpdate) {
    if (this->m_textures[0] || this->m_textures[1]) {
        if (this->m_textures[0]) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[0]));
        }

        if (this->m_textures[1]) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[1]));
        }

        this->m_textures[0] = nullptr;
        this->m_textures[1] = nullptr;
        this->m_densities.Clear();
        this->m_texels.Clear();
        this->m_prevRow.Clear();
    }

    this->m_lod = static_cast<uint8_t>(lod);
    this->m_size = s_cloudLodSize[static_cast<uint8_t>(lod)];
    this->m_rowsPerUpdate = rowsPerUpdate ? rowsPerUpdate : 8;
    this->m_mask = this->m_size - 1;
    this->m_shift = s_cloudLodShift[static_cast<uint8_t>(lod)];

    this->m_texels.SetCount(this->m_size * this->m_size);
    memset(this->m_texels.m_data, 0xFF, sizeof(CImVector) * this->m_texels.m_count);
    this->m_densities.SetCount(this->m_size * this->m_size);
    this->m_prevRow.SetCount(this->m_size);
    memset(this->m_prevRow.m_data, 0, sizeof(float) * this->m_prevRow.m_count);

    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
    this->m_textures[0] = TextureCreate(this->m_size, this->m_size, this->m_texFormat, GxTex_Argb8888, flags, this->m_texels.m_data, DayNight::DNClouds::Callback_GxTex, "DNClouds0", 0);
    this->m_textures[1] = TextureCreate(this->m_size, this->m_size, this->m_texFormat, GxTex_Argb8888, flags, this->m_texels.m_data, DayNight::DNClouds::Callback_GxTex, "DNClouds1", 0);
    this->m_row = 0;
    this->m_fullUpdate = 1;
}

// VERIFY: 92e57f89 auto
// OFFSET: 0x007F1CD0
bool CWorldParam::CloudLODCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t lod = SStrToInt(value);

    if (lod < 0) {
        lod = 0;
    } else if (lod > 3) {
        lod = 3;
    }

    DayNight::g_clouds.SetLOD(lod, 0);

    if (oldValue) {
        char buffer[64];
        SStrPrintf(buffer, sizeof(buffer), "SkyCloudLOD set to %i", lod);
        ConsoleWrite(buffer, DEFAULT_COLOR);
    }

    return true;
}

// VERIFY: 3092a8fd manual
// OFFSET: 0x007F1D30
void DayNight::UnloadMap() {
    DNOverrideSky* sky = DayNight::g_overrideSky.m_fulllist.Head();

    while (sky) {
        DNOverrideSky* next = DayNight::g_overrideSky.m_fulllist.Next(sky);

        if (sky->m_model) {
            sky->m_model->Release();
        }

        DayNight::g_overrideSky.Delete(sky);
        sky = next;
    }

    if (s_skyScene) {
        s_skyScene->Release();
    }

    s_skyScene = nullptr;
    DayNight::g_dnInfo.m_skybox = nullptr;
    DayNight::g_dnInfo.m_blendSkybox[0] = nullptr;
    DayNight::g_dnInfo.m_blendSkybox[1] = nullptr;
    DayNight::g_dnInfo.m_blendSkybox[2] = nullptr;

    for (uint32_t i = 0; i < s_areaLightOverrides.m_count; i++) {
        LightRec* light = s_areaLightOverrides.m_data[i].m_light;
        light->~LightRec();
        s_lightAllocator.PutData(light, nullptr, 0);
    }

    s_areaLightOverrides.m_count = 0;
    s_lightAllocator.Clear(typeid(LightRec).raw_name(), SLOG_OBJECT, 1);

    if (DayNight::g_forceFullUpdate) {
        if (DayNight::g_clouds.m_textures[0]) {
            HandleClose(reinterpret_cast<HOBJECT>(DayNight::g_clouds.m_textures[0]));
        }

        if (DayNight::g_clouds.m_textures[1]) {
            HandleClose(reinterpret_cast<HOBJECT>(DayNight::g_clouds.m_textures[1]));
        }

        DayNight::g_clouds.m_textures[0] = nullptr;
        DayNight::g_clouds.m_textures[1] = nullptr;
        DayNight::g_planets.m_sun.Destroy();
        DayNight::g_planets.m_moon.Destroy();
        DayNight::g_planets.m_moon2.Destroy();
        DayNight::g_sunGlare.Destroy();
        DayNight::g_moonGlare.Destroy();
        DayNight::g_stars.Destroy();
        DayNight::g_forceFullUpdate = 0;
    }
}

// VERIFY: f46c563f manual
// OFFSET: 0x007F20E0
void DayNight::DNClouds::GenSphere(float radius) {
    float zOffset = -cos(0.7853981852531433);

    this->m_verts.SetCount(192);
    this->m_texCoords.SetCount(192);
    this->m_colors.SetCount(192);
    this->m_indices.SetCount(374);

    CImVector* colors = this->m_colors.m_data;
    C3Vector* verts = this->m_verts.m_data;
    C2Vector* texCoords = this->m_texCoords.m_data;
    uint16_t* indices = this->m_indices.m_data;
    uint16_t prevRowStart = 0;
    float prevTheta = 0.0f;

    CImVector color;
    color.b = 0xFF;
    color.g = 0xFF;
    color.r = 0xFF;

    for (uint32_t row = 0; row < 12; row++) {
        float theta = s_cloudThetaTable[row] * 3.1415927f;
        float sinTheta = sin(theta);
        float texRadius = 0.5f * (row * 0.090909094f);
        float z = radius * cos(theta) + zOffset;
        uint16_t rowStart = static_cast<uint16_t>(verts - this->m_verts.m_data);
        bool top = CMath::fequal(theta, 0.0f);
        float phi = 0.0f;

        for (uint32_t i = 0; ; ) {
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);
            verts->x = sinPhi * sinTheta * radius;
            verts->y = sinTheta * cosPhi * radius;
            verts->z = z;
            verts++;

            color.a = s_cloudAlphaTable[row];
            *colors++ = color;

            texCoords->x = sinPhi * texRadius + 0.5f;
            texCoords->y = cosPhi * texRadius + 0.5f;
            texCoords++;

            if (top || CMath::fequal(theta, 6.2831855f) || ++i >= 16) {
                break;
            }

            phi += 0.39269909f;
        }

        if (row) {
            bool prevPole = CMath::fequal(prevTheta, 0.0f);
            bool pole = CMath::fequal(theta, 6.2831855f);

            for (uint32_t i = 0; i < 17; i++) {
                *indices++ = (prevPole ? 0 : i & 0xF) + prevRowStart;
                *indices++ = (pole ? 0 : i & 0xF) + rowStart;
            }
        }

        prevTheta = theta;
        prevRowStart = rowStart;
    }

    this->m_nVerts = static_cast<uint16_t>(verts - this->m_verts.m_data);
    this->m_nIndices = 374;
}

// VERIFY: 73032c78 manual
// OFFSET: 0x007F2470
void DayNight::DNSky::GenSphere(float sphRadius) {
    this->m_sphThetaTess = 24;
    this->m_geoVerts.SetCount(168);
    this->m_clrVerts.SetCount(168);
    this->m_indices.SetCount(300);

    float zOffset = -cosf(0.78539819f);
    uint16_t* indices = this->m_indices.m_data;
    C3Vector* verts = this->m_geoVerts.m_data;
    uint16_t prevRowStart = 0;
    float prevTheta = 0.0f;

    for (int32_t row = 0; row < SKY_NUMBANDS; row++) {
        float theta = s_skyThetaTable[row] * 3.1415927f;
        float cosTheta = CMath::cos_(theta);
        float sinTheta = CMath::sin_(theta);
        float z = sphRadius * cosTheta + zOffset;
        uint16_t rowStart = static_cast<uint16_t>(verts - this->m_geoVerts.m_data);
        bool top = CMath::fequal(theta, 0.0f);

        for (int32_t i = 0; ; ) {
            float angle = i * 0.041666668f * 6.2831855f;
            verts->x = sinf(angle) * sinTheta * sphRadius;
            verts->y = sinTheta * cosf(angle) * sphRadius;
            verts->z = z;
            verts++;

            if (top || CMath::fequal(theta, 3.1415927f) || ++i >= 24) {
                break;
            }
        }

        if (row > 0) {
            bool prevPole = CMath::fequal(prevTheta, 0.0f);
            bool pole = CMath::fequal(theta, 3.1415927f);

            for (uint32_t i = 0; i < 25; i++) {
                *indices++ = (prevPole ? 0 : i % 24) + prevRowStart;
                *indices++ = rowStart + (pole ? 0 : i % 24);
            }
        }

        prevTheta = theta;
        prevRowStart = rowStart;
    }

    this->m_nVerts = static_cast<uint16_t>(verts - this->m_geoVerts.m_data);
    this->m_nIndices = 300;
}

// VERIFY: 2cd09dd6 manual
// OFFSET: 0x007F2790
void DayNight::LoadMap(int32_t mapId) {
    DayNight::s_mapId = mapId;
    DayNight::LightLoad(DayNight::g_areaLights, mapId);

    DNInfo& info = DayNight::g_dnInfo;
    info.m_dayProgression = 0.0f;
    info.m_day = 0.0f;
    info.m_timeOfDay = 0;
    info.m_playerPos = C3Vector();
    info.m_cameraPos = C3Vector();
    info.m_faceAngle = 0.0f;
    info.m_lightRefPos = C3Vector();
    info.m_farClip = 0.0f;
    info.m_timeSec = 0.0f;
    info.m_deltaSec = 0.0f;
    info.m_weatherIntensity = 0.0f;
    info.m_skyboxWeight = 0.0f;
    info.m_cameraDir = C3Vector();
    info.m_flashBlend = 0;
    info.m_flashColor = CImVector();
    info.m_overrideLightParamsId = -1;
    info.m_skybox = nullptr;

    for (int32_t i = 0; i < 3; i++) {
        info.m_blendSkybox[i] = nullptr;
        info.m_blendSkyboxWeight[i] = 0.0f;
        info.m_blendSkyboxFlags[i] = 0;
    }

    info.m_interiorFogBlend = 0.0f;
    info.m_interiorMapObjDef = nullptr;
    info.m_interiorGroups.m_count = 0;

    DayNight::g_sky.GenSphere(1.0f);
    DayNight::g_clouds.SetLOD(0, 0);
    DayNight::g_clouds.GenSphere(1.0f);

    float density = 0.6f;

    if (DayNight::g_clouds.m_densityOverride != 0.0f) {
        density = DayNight::g_clouds.m_densityOverride;
    }

    DayNight::g_clouds.m_density = static_cast<uint8_t>((1.0f - density) * 255.0f);
    DayNight::g_clouds.SetSharpness(0.95999998f);
    DayNight::g_clouds.m_enabled = 1;
    DayNight::GlareBase::m_masterEnable = 1;
    DayNight::InitSunGlare();
    DayNight::InitMoonGlare();
    DayNight::g_planets.m_sun.Initialize("Textures\\sunCenter.blp");
    DayNight::g_planets.m_sun.m_baseScale = 1.0f;
    DayNight::g_planets.m_sun.m_period = 1.0f;
    DayNight::g_planets.m_moon.Initialize("Textures\\moon.blp");
    DayNight::g_planets.m_moon.m_baseScale = 1.75f;
    DayNight::g_planets.m_moon.m_period = 1.0f;
    DayNight::g_planets.m_moon2.Initialize("Textures\\moon02.blp");
    DayNight::g_planets.m_moon2.m_baseScale = 1.0f;
    DayNight::g_planets.m_moon2.m_period = 1.7f;
    DayNight::g_stars.Initialize();
    info.m_showSky = 1;
    info.m_flashBlend = 0;
    DayNight::s_lightFlags = 0;
    s_cvSkyCloudLOD = CVar::Register("SkyCloudLOD", nullptr, 0, "0", CWorldParam::CloudLODCallback, 1, false, nullptr, false);
    ConsoleCommandRegister("SkySunGlare", SkySunGlare, CATEGORY_GRAPHICS, "0, 1");
    DayNight::g_forceFullUpdate = 1;
}

// INLINED
DayNight::DNOverrideSky::DNOverrideSky() : m_model(nullptr), m_animLengthMs(0), m_lastTimeOfDay(static_cast<uint32_t>(-2)), m_flags(0) {
}

// VERIFY: a23a774f manual
// OFFSET: 0x007F30C0
DayNight::DNOverrideSky* DayNight::GetOverrideSky(const char* path, uint32_t flags) {
    if (!path || !*path) {
        return nullptr;
    }

    DNOverrideSky* sky = DayNight::g_overrideSky.Ptr(path);

    if (sky) {
        return sky;
    }

    sky = DayNight::g_overrideSky.New(path, 0, 0);

    if (!s_skyScene) {
        s_skyScene = M2CreateScene();
        s_skySceneTimeMs = OsGetAsyncTimeMs();
    }

    sky->m_model = s_skyScene->CreateModel(path, 0);
    sky->m_flags = flags;

    return sky;
}

// VERIFY: 774391af manual
// OFFSET: 0x007F3230
void DayNight::SetColors() {
    DNInfo& info = DayNight::g_dnInfo;
    float liquidDepth = 0.0f;
    int32_t liquidStatus = World::SceneCamLiquidStatus(&liquidDepth);
    const LiquidTypeRec* liquid = liquidStatus ? g_liquidTypeDB.GetRecord(liquidStatus) : nullptr;
    bool liquidLight = liquid && liquid->m_lightID != 0;

    for (int32_t i = 0; i < 3; i++) {
        info.m_blendSkyboxWeight[i] = 0.0f;
        info.m_blendSkybox[i] = nullptr;
        info.m_blendSkyboxFlags[i] = 0;
    }

    info.m_skyboxWeight = 0.0f;
    info.m_skybox = nullptr;

    if (DayNight::g_areaLights.m_count) {
        if (liquidStatus && liquidLight) {
            if (liquid) {
                const LightParamsRec* params = g_lightParamsDB.GetRecord(liquid->m_lightID);
                float progression = info.m_dayProgression * 2880.0f;
                DayNight::CalcColors(CMath::fint_(progression - 0.5f), &info.m_bands, params);
            }
        } else {
            CurrentLight light;
            float progression = info.m_dayProgression * 2880.0f;
            int32_t time = CMath::fint_(progression - 0.5f);
            const LightParamsRec* params = DayNight::GetLightParams(DayNight::g_areaLights.m_data[0], liquidStatus != 0);
            DayNight::CalcColors(time, &light, params);

            if (info.m_stormBlend > 0.0f) {
                CurrentLight storm;
                progression = info.m_dayProgression * 2880.0f;
                time = CMath::fint_(progression - 0.5f);
                params = DayNight::GetLightParams(DayNight::g_areaLights.m_data[0], (liquidStatus != 0) + 2);
                DayNight::CalcColors(time, &storm, params);
                DayNight::BlendColors(&light, &storm, info.m_stormBlend);
            }

            DayNight::DoAreaLights(&light, liquidStatus);

            const LightParamsRec* override;

            if (info.m_overrideLightParamsId == -1 || !(override = DayNight::GetLightParams(DayNight::g_areaLights.m_data[0], info.m_overrideLightParamsId))) {
                info.m_bands = light;
            } else {
                progression = info.m_dayProgression * 2880.0f;
                DayNight::CalcColors(CMath::fint_(progression - 0.5f), &info.m_bands, override);
                info.m_bands.m_glow = light.m_glow;
                info.m_bands.m_waterShallowAlpha = light.m_waterShallowAlpha;
                info.m_bands.m_cloudTypeId = light.m_cloudTypeId;
                info.m_bands.m_waterDeepAlpha = light.m_waterDeepAlpha;
                info.m_bands.m_cloudWeight = light.m_cloudWeight;
                info.m_bands.m_oceanShallowAlpha = light.m_oceanShallowAlpha;
                info.m_bands.m_skyboxes[0].m_weight = light.m_skyboxes[0].m_weight;
                info.m_bands.m_oceanDeepAlpha = light.m_oceanDeepAlpha;
                info.m_bands.m_skyboxes[0].m_id = light.m_skyboxes[0].m_id;
                info.m_bands.m_skyboxes[1].m_id = light.m_skyboxes[1].m_id;
                info.m_bands.m_skyboxes[2].m_id = light.m_skyboxes[2].m_id;
                info.m_bands.m_skyboxes[1].m_weight = light.m_skyboxes[1].m_weight;
                info.m_bands.m_skyboxes[2].m_weight = light.m_skyboxes[2].m_weight;

                const LightSkyboxRec* skybox = g_lightSkyboxDB.GetRecord(override->m_lightSkyboxID);

                if (skybox) {
                    info.m_skybox = DayNight::GetOverrideSky(skybox->m_name, skybox->m_flags);
                    info.m_skyboxWeight = 1.0f;
                }
            }
        }
    } else {
        memset(&info.m_bands, 0xFF, sizeof(info.m_bands));
        info.m_bands.m_fogEnd = 1.0e10f;
        *reinterpret_cast<uint32_t*>(&info.m_bands.m_ambient) = 0xFF404040;
        info.m_bands.m_fogStartMul = DEFAULT_FOG_SCALER;
        info.m_bands.m_cloudTypeId = 0;
        info.m_bands.m_skyboxes[0].m_id = 0;
        info.m_bands.m_fogRate = 4.0f;
        info.m_bands.m_skyboxes[1].m_id = 0;
        info.m_bands.m_skyboxes[2].m_id = 0;
        info.m_bands.m_highlightSky = 0.0f;
        info.m_bands.m_floatBand2 = 0.0f;
        info.m_bands.m_cloudDensity = 0.0f;
        info.m_bands.m_floatBand4 = 0.0f;
        info.m_bands.m_floatBand5 = 0.0f;
        info.m_bands.m_glow = 0.5f;
        info.m_bands.m_oceanShallowAlpha = 0.75f;
        info.m_bands.m_oceanDeepAlpha = 1.0f;
        info.m_bands.m_waterDeepAlpha = 1.0f;
        info.m_bands.m_waterShallowAlpha = 0.5f;
    }

    DayNight::SetLightColors();

    if (liquidStatus && liquid && liquid->m_maxDarkenDepth > 0.0f) {
        float maxDepth = -liquid->m_maxDarkenDepth;
        float depth = liquidDepth <= 0.0f ? liquidDepth : 0.0f;

        if (depth <= maxDepth) {
            depth = maxDepth;
        }

        float fogScale = liquid->m_fogDarkenIntensity;
        float ambientScale = liquid->m_ambDarkenIntensity;
        float directScale = liquid->m_dirDarkenIntensity;
        float darken = 1.0f - depth / maxDepth - 1.0f;
        info.m_fog.m_color = DayNight::DarkenColor(info.m_fog.m_color, fogScale * darken + 1.0f);
        info.m_light1.m_ambient = DayNight::DarkenColor(info.m_light1.m_ambient, darken * ambientScale + 1.0f);
        info.m_light1.m_diffuse = DayNight::DarkenColor(info.m_light1.m_diffuse, darken * directScale + 1.0f);
    }

    DayNight::g_sky.SetColors();

    CImVector sunColor = info.m_bands.m_sunCloudColors[0];
    DayNight::g_planets.m_sun.m_color = sunColor;
    DayNight::g_sunGlare.m_color = sunColor;
    DayNight::g_planets.m_moon.m_color = sunColor;
    DayNight::g_moonGlare.m_color = sunColor;

    if (info.m_flashBlend != 0.0f) {
        CImVector flash = info.m_flashColor;

        if (info.m_flashBlend) {
            info.m_fog.m_color.InterpColor(info.m_flashBlend, &flash);
        }

        flash = info.m_flashColor;

        if (info.m_flashBlend) {
            info.m_light1.m_ambient.InterpColor(info.m_flashBlend, &flash);
        }

        flash = info.m_flashColor;

        if (info.m_flashBlend) {
            info.m_light1.m_diffuse.InterpColor(info.m_flashBlend, &flash);
        }

        flash = info.m_flashColor;
        uint8_t blend = info.m_flashBlend;

        if (blend) {
            DayNight::g_planets.m_sun.m_color.InterpColor(blend, &flash);
        }

        if (blend) {
            DayNight::g_planets.m_moon.m_color.InterpColor(blend, &flash);
        }
    }

    if (info.m_stormBlend != 0.0f) {
        uint8_t alpha = CMath::fint_((1.0f - info.m_stormBlend) * 255.0f);
        DayNight::g_planets.m_sun.m_color.a = alpha;
        DayNight::g_sunGlare.m_color.a = alpha;
        DayNight::g_planets.m_moon.m_color.a = alpha;
        DayNight::g_moonGlare.m_color.a = alpha;
        DayNight::g_planets.m_moon2.m_color.a = alpha;
    }

    uint32_t count = 0;

    for (CurrentLight::SkyboxBlend* blend = info.m_bands.m_skyboxes; blend < info.m_bands.m_skyboxes + 3; blend++) {
        if (count >= 3) {
            break;
        }

        if (!blend->m_id || blend->m_weight <= 0.0f) {
            continue;
        }

        const LightSkyboxRec* skybox = g_lightSkyboxDB.GetRecord(blend->m_id);

        if (!skybox) {
            continue;
        }

        DNOverrideSky* sky = DayNight::GetOverrideSky(skybox->m_name, skybox->m_flags);
        float weight = blend->m_weight;
        uint32_t flags = skybox->m_flags & 2;

        if (weight > 0.99000001f && count && !flags) {
            count = 0;
        }

        info.m_blendSkybox[count] = sky;
        info.m_blendSkyboxWeight[count] = weight;
        info.m_blendSkyboxFlags[count] = flags;
        count++;
    }

    if (count < 3) {
        info.m_blendSkyboxWeight[count] = 0.0f;
    }

    info.m_curve0 = DayNight::InterpTable(s_curveTable0, 4, info.m_dayProgression);
    info.m_curve1 = DayNight::InterpTable(s_curveTable1, 2, info.m_dayProgression);
}

// VERIFY: d95f0d18 auto
// OFFSET: 0x007F3920
void DayNight::UpdateLighting() {
    DNInfo& info = DayNight::g_dnInfo;
    float x = info.m_cameraDir.x;
    float y = info.m_cameraDir.y;
    float angle;

    if (x * x + y * y <= 0.0001) {
        angle = atan2(info.m_cameraDir.z, x);
    } else {
        angle = atan2(y, x);
    }

    info.m_faceAngle = angle;

    if (angle < 0.0f) {
        info.m_faceAngle = angle + 6.2831855f;
    }

    float storm = info.m_weatherIntensity * 4.0f;

    if (storm > 1.0f) {
        storm = 1.0f;
    }

    info.m_stormBlend = storm;
    DayNight::SetColors();
    DayNight::SetDirection();
    DayNight::SetPlanets();
}

// VERIFY: dbc89d6c auto
// OFFSET: 0x007ECE00
int32_t DayNight::GetFogOverride() {
    return s_fogOverride;
}


// VERIFY: c61d066e auto
// OFFSET: 0x009ABAF0
void DayNight::DNPlanet::Destroy() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }
}

// VERIFY: 0e747bc2 auto
// OFFSET: 0x009ABAC0
void DayNight::DNGlare::Destroy() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    if (this->m_occlusionQuery) {
        GxQueryDestroy(this->m_occlusionQuery);
    }
}

// VERIFY: cdeab972 auto
// OFFSET: 0x009ABB30
void DayNight::DNStars::Destroy() {
    if (this->m_model) {
        this->m_model->Release();
        this->m_model = nullptr;
    }

    if (this->m_scene) {
        this->m_scene->Release();
        this->m_scene = nullptr;
    }
}

// VERIFY: cfbe1734 manual
// OFFSET: 0x007EE300
void DayNight::SetFlashColor(CImVector color, float blend) {
    DayNight::g_dnInfo.m_flashColor = color;
    DayNight::g_dnInfo.m_flashBlend = static_cast<uint8_t>(blend * 255.0f);
}

// VERIFY: 8f429e9d auto
// OFFSET: 0x007F0870
void DayNight::RenderGlares() {
    if (!DayNight::g_dnInfo.m_showSky) {
        return;
    }

    DayNight::g_sunGlare.Update(DayNight::g_dnInfo.m_deltaSec);
    DayNight::g_sunGlare.Render();
    DayNight::g_moonGlare.Update(DayNight::g_dnInfo.m_deltaSec);
    DayNight::g_moonGlare.Render();
}

// VERIFY: e14afd01 auto
// OFFSET: 0x007EE3F0
DayNight::DNGlare::DNGlare() {
    this->m_position.x = 0.0f;
    this->m_position.y = 0.0f;
    this->m_occlusionQuery = nullptr;
    this->m_position.z = 0.0f;
    this->m_color = CImVector();
    this->unused_003C[0] = 0.0f;
    this->unused_003C[1] = 0.0f;
    this->unused_003C[2] = 0.0f;
    this->unused_003C[3] = 0.0f;
    this->unused_003C[4] = 0.0f;
    this->unused_003C[5] = 0.0f;
    this->unused_003C[6] = 0.0f;
    this->unused_003C[7] = 0.0f;
    this->unused_005C[0] = 0;
    this->unused_005C[1] = 0;
    this->unused_005C[2] = 0;
    this->unused_005C[3] = 0;
    this->m_alphaCurve[0].m_time = 0.0f;
    this->m_alphaCurve[0].m_value = 0.0f;
    this->m_planet = nullptr;
    this->m_alphaCurve[1].m_time = 0.0f;
    this->m_alphaCurve[1].m_value = 0.0f;
    this->m_alphaCurve[2].m_time = 0.0f;
    this->m_alphaCurve[2].m_value = 0.0f;
    this->m_alphaCurve[3].m_time = 0.0f;
    this->m_alphaCurve[3].m_value = 0.0f;
}

// VERIFY: 32883be1 auto
// OFFSET: 0x007EF6E0
void DayNight::DNGlare::Update(float deltaSec) {
    this->m_fadeTarget = 1.0f;
    float dx = this->m_position.x - DayNight::g_dnInfo.m_cameraPos.x;
    float dy = this->m_position.y - DayNight::g_dnInfo.m_cameraPos.y;
    float dz = this->m_position.z - DayNight::g_dnInfo.m_cameraPos.z;
    float length = sqrt(dy * dy + dz * dz + dx * dx);
    float dirX = dx * (1.0f / length);
    float dirY = dy * (1.0f / length);
    float dirZ = (1.0f / length) * dz;
    this->m_fadeTarget = this->GetCloudDensityFade() * this->m_fadeTarget;
    this->m_fadeTarget = this->GetLiquidDepthFade() * this->m_fadeTarget;
    float skyboxWeight;

    if (DayNight::g_dnInfo.m_skybox && DayNight::g_dnInfo.m_skyboxWeight > 0.0f) {
        skyboxWeight = DayNight::g_dnInfo.m_skyboxWeight;
    } else {
        skyboxWeight = 0.0f;

        if (DayNight::g_dnInfo.m_blendSkybox[0] && DayNight::g_dnInfo.m_blendSkyboxWeight[0] > 0.0f) {
            skyboxWeight = DayNight::g_dnInfo.m_blendSkyboxWeight[0];
        }

        if (DayNight::g_dnInfo.m_blendSkybox[1] && DayNight::g_dnInfo.m_blendSkyboxWeight[1] > skyboxWeight) {
            skyboxWeight = DayNight::g_dnInfo.m_blendSkyboxWeight[1];
        }

        if (DayNight::g_dnInfo.m_blendSkybox[2] && DayNight::g_dnInfo.m_blendSkyboxWeight[2] > skyboxWeight) {
            skyboxWeight = DayNight::g_dnInfo.m_blendSkyboxWeight[2];
        }
    }

    float target = (1.0f - skyboxWeight) * this->m_fadeTarget;
    this->m_fadeTarget = target;
    this->m_fadeTarget = DayNight::InterpTable(reinterpret_cast<const C2Vector*>(this->m_alphaCurve), 4, DayNight::g_dnInfo.m_dayProgression) * target;

    if (this->m_fadeTarget != 0.0f) {
        this->m_fadeTarget = this->GetOcclusionFade() * this->m_fadeTarget;
    }

    if (this->m_fade < this->m_fadeTarget) {
        float fade = this->m_fadeInRate * deltaSec + this->m_fade;
        this->m_fade = fade >= this->m_fadeTarget ? this->m_fadeTarget : fade;
    } else if (this->m_fade > this->m_fadeTarget) {
        float fade = this->m_fade - this->m_fadeOutRate * deltaSec;
        this->m_fade = fade <= this->m_fadeTarget ? this->m_fadeTarget : fade;
    }

    float dot = DayNight::g_dnInfo.m_cameraDir.y * dirY + DayNight::g_dnInfo.m_cameraDir.z * dirZ + DayNight::g_dnInfo.m_cameraDir.x * dirX;

    if (dot < this->m_minDot) {
        dot = this->m_minDot;
    }

    float t = (dot - this->m_minDot) / (1.0f - this->m_minDot);
    this->m_scale = ((this->m_scaleMax - this->m_scaleMin) * t + this->m_scaleMin) * this->m_baseScale;
    this->m_color.a = static_cast<uint8_t>((t * (this->m_alphaMax - this->m_alphaMin) + this->m_alphaMin) * this->m_fade * (this->m_color.a * 0.0039215689f) * 255.0f);
    this->m_screenGlow = pow(dot, s_glarePower) * this->m_fade;
}

// VERIFY: 40579330 manual
// OFFSET: 0x009AC400
void DayNight::DNGlare::Render() {
    if (!DayNight::GlareBase::m_masterEnable || !this->m_enable || !this->m_color.a) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    C44Matrix view;
    GxXformView(view);
    C3Vector direction(view.a2, view.b2, view.c2);
    C44Matrix world;
    DayNight::Billboard(world, direction);
    world.d0 = this->m_position.x - info->m_cameraPos.x;
    world.d1 = this->m_position.y - info->m_cameraPos.y;
    world.d2 = this->m_position.z - info->m_cameraPos.z;
    world.Scale(C3Vector(this->m_scale, this->m_scale, this->m_scale));
    g_theGxDevicePtr->XformPushAndSet(GxXform_World, world);
    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minZ = 0.0f;
    float maxZ = 0.0f;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
    GxXformSetViewport(minX, maxX, minY, maxY, s_skyMinZ, s_skyMaxZ);
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 3);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_DepthTest, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxPrimVertexPtr(4, s_glareVertices, sizeof(C3Vector), nullptr, 0, &this->m_color, 0, s_glareTexCoords, sizeof(C2Vector), nullptr, 0);
    GxPrimIndexPtr(4, s_glareIndices);
    CGxBatch batch;
    batch.m_primType = GxPrim_TriangleStrip;
    batch.m_start = 0;
    batch.m_count = 4;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 3;
    g_theGxDevicePtr->Draw(&batch, 1);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
    GxXformPop(GxXform_World);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 32953b81 auto
// OFFSET: 0x009ABC60
float DayNight::DNGlare::GetPointOcclusion() {
    DayNight::DNInfo* info = DayNight::GetInfo();
    float dx = this->m_position.x - info->m_cameraPos.x;
    float dy = this->m_position.y - info->m_cameraPos.y;
    float dz = this->m_position.z - info->m_cameraPos.z;
    float inverse = 1.0f / sqrt(dz * dz + dy * dy + dx * dx);
    float nx = dx * inverse;
    float ny = dy * inverse;
    float nz = inverse * dz;
    C3Vector start;
    start.x = nx * 0.5f + info->m_cameraPos.x;
    start.y = ny * 0.5f + info->m_cameraPos.y;
    start.z = 0.5f * nz + info->m_cameraPos.z;
    C3Vector hit;
    C3Vector end;
    end.x = nx * info->m_farClip + info->m_cameraPos.x;
    end.y = ny * info->m_farClip + info->m_cameraPos.y;
    end.z = nz * info->m_farClip + info->m_cameraPos.z;
    float distance = 1.0f;

    if (World::Intersect(start, end, &hit, &distance, 0x200112, nullptr)) {
        return 0.0f;
    }

    return !CGWorldFrame::s_currentWorldFrame->HitTestModelList(start, end);
}

// VERIFY: 5f4e0628 manual
// OFFSET: 0x009ABE00
float DayNight::DNGlare::GetQueryOcclusion() {
    uint32_t pixels = 0;
    uint32_t available;
    GxQueryGetParam(this->m_occlusionQuery, GxQueryParam_Unk1, &available);

    if (!available || !GxQueryGetData(this->m_occlusionQuery, &pixels)) {
        pixels = this->m_occlusionPixels;
    }

    this->m_occlusionPixels = pixels;

    if (available) {
        GxQueryBegin(this->m_occlusionQuery);
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_ColorWrite, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_Lighting, 0);
    C3Vector verts[6];
    CImVector colors[6] = {};
    C3Vector screen[6];
    C2Vector texCoords[6];
    uint16_t indices[8];
    uint32_t vertCount;
    uint32_t indexCount;
    this->m_planet->GenGeometry(verts, texCoords, colors, indices, &vertCount, &indexCount);
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
    GxXformSetViewport(minX, maxX, minY, maxY, s_skyMinZ, s_skyMaxZ);
    C44Matrix view;
    GxXformView(view);
    DayNight::DNInfo* info = DayNight::GetInfo();
    C3Vector direction(view.a2, view.b2, view.c2);
    C44Matrix world;
    DayNight::Billboard(world, direction);
    world.d0 = this->m_position.x - info->m_cameraPos.x;
    world.d1 = this->m_position.y - info->m_cameraPos.y;
    world.d2 = this->m_position.z - info->m_cameraPos.z;
    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->m_xforms[GxXform_World].Identity();
    g_theGxDevicePtr->XformSetView(world * view);

    if (available) {
        GxPrimVertexPtr(vertCount, verts, sizeof(C3Vector), nullptr, 0, colors, sizeof(CImVector), nullptr, 0, nullptr, 0);
        GxPrimIndexPtr(indexCount, indices);
        CGxBatch batch;
        batch.m_primType = GxPrim_TriangleStrip;
        batch.m_start = 0;
        batch.m_count = indexCount;
        batch.m_minIndex = 0;
        batch.m_maxIndex = vertCount - 1;
        g_theGxDevicePtr->Draw(&batch, 1);
    }

    GxuXformCalc3dScreenCoords(vertCount, verts, screen);
    float area = fabs(screen[1].x - screen[0].x) * fabs(screen[2].y - screen[0].y);
    GxXformPop(GxXform_World);
    g_theGxDevicePtr->XformSetView(view);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
    g_theGxDevicePtr->RsPop();

    if (available) {
        GxQueryEnd(this->m_occlusionQuery);
    }

    float result = 0.0f;

    if (this->m_skipOcclusionOnce) {
        this->m_skipOcclusionOnce = 0;
    } else if (area > 1.0f) {
        result = pixels / area;

        if (result > 1.0f) {
            return 1.0f;
        }
    }

    return result;
}

// VERIFY: 736a9501 auto
// OFFSET: 0x009AC3C0
float DayNight::DNGlare::GetOcclusionFade() {
    if (!DayNight::GlareBase::m_masterEnable || !this->m_enable || !this->m_color.a) {
        return 0.0f;
    }

    if (this->m_occlusionQuery && CWorldParam::cvar_occlusion->m_intValue) {
        return this->GetQueryOcclusion();
    }

    return this->GetPointOcclusion();
}

// VERIFY: a694bf5f manual
// OFFSET: 0x007EDA30
float DayNight::DNGlare::GetLiquidDepthFade() {
    float fade = 1.0f;
    float depth;

    if (!World::SceneCamLiquidStatus(&depth)) {
        return fade;
    }

    float scaled = depth * 0.1f;

    if (scaled < 0.0f) {
        return 1.0f - 0.0f;
    }

    if (scaled >= 1.0f) {
        scaled = 1.0f;
    }

    return 1.0f - scaled;
}

// VERIFY: fead6e16 auto
// OFFSET: 0x007F1020
float DayNight::DNSunGlare::GetCloudDensityFade() {
    return 1.0f - DayNight::g_clouds.GetDensity(&this->m_position, 1.0f);
}

// VERIFY: 86e92cf3 auto
// OFFSET: 0x007F1040
float DayNight::DNMoonGlare::GetCloudDensityFade() {
    float density = DayNight::g_clouds.GetDensity(&this->m_position, 1.0f) - 0.5f;
    return 1.0f - fabs(density + density);
}

// VERIFY: fea873d6 auto
// OFFSET: 0x007EEA80
void DayNight::UpdateStars() {
    DayNight::g_stars.Update();
}

// VERIFY: aa435fb6 auto
// OFFSET: 0x007F10A0
void DayNight::AddLightOverride(int32_t lightId, int32_t overrideId, uint32_t duration) {
    LightRec* light = g_lightDB.GetRecord(lightId);
    LightRec* overrideLight = g_lightDB.GetRecord(overrideId);

    if (!light || !overrideLight) {
        return;
    }

    void* data = s_lightAllocator.GetData(0, typeid(LightRec).raw_name(), SLOG_OBJECT);

    if (!data) {
        return;
    }

    LightRec* rec = new (data) LightRec();
    *rec = *overrideLight;
    rec->m_gameCoords[0] = light->m_gameCoords[0];
    rec->m_gameCoords[1] = light->m_gameCoords[1];
    rec->m_gameCoords[2] = light->m_gameCoords[2];
    rec->m_gameFalloffStart = light->m_gameFalloffStart;
    rec->m_gameFalloffEnd = light->m_gameFalloffEnd;

    AreaLightOverride* entry = s_areaLightOverrides.New();

    if (!entry) {
        return;
    }

    entry->m_light = rec;
    entry->m_fade = 0.0f;
    entry->m_id = lightId;
    entry->m_state = 1;
    entry->m_lastTime = OsGetAsyncTimeMs();
    entry->m_duration = duration;
    DayNight::ResetFogAccumulators(true);
}

// VERIFY: 1e2f3fef auto
// OFFSET: 0x007F11A0
void DayNight::RemoveLightOverride(int32_t lightId, uint32_t duration) {
    for (uint32_t i = 0; i < s_areaLightOverrides.m_count; i++) {
        if (s_areaLightOverrides.m_data[i].m_id != static_cast<uint32_t>(lightId)) {
            continue;
        }

        AreaLightOverride* entry = &s_areaLightOverrides.m_data[i];
        entry->m_state = 2;
        entry->m_lastTime = OsGetAsyncTimeMs();
        entry->m_duration = duration;
        DayNight::ResetFogAccumulators(true);
        return;
    }
}

// VERIFY: 1d606d65 auto
// OFFSET: 0x007F31C0
void DayNight::SetBlendSky(uint32_t index, const char* sky, int32_t flags, float blend) {
    if (index > 3) {
        return;
    }

    if (sky) {
        DayNight::g_dnInfo.m_blendSkybox[index] = DayNight::GetOverrideSky(sky, flags);
        DayNight::g_dnInfo.m_blendSkyboxWeight[index] = blend;
        DayNight::g_dnInfo.m_blendSkyboxFlags[index] = 0;
    } else {
        DayNight::g_dnInfo.m_blendSkybox[index] = nullptr;
        DayNight::g_dnInfo.m_blendSkyboxWeight[index] = 0.0f;
        DayNight::g_dnInfo.m_blendSkyboxFlags[index] = 0;
    }
}
