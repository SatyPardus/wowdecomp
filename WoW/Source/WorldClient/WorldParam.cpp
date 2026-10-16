#include "WorldParam.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Types.h"

// VERIFY: ee297f04 auto
// OFFSET: 0x0078DD40
void CWorldParam::SetDefaultParticleDensity() {
    switch (g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel]) {
        case 0:
        case 1:
        case 7:
        case 8:
        case 9:
        case 10: {
            CWorldParam::cvar_particleDensity->Set("0.1", true, false, false, true);
            break;
        }

        case 2: {
            CWorldParam::cvar_particleDensity->Set("0.4", true, false, false, true);
            break;
        }

        case 3: {
            CWorldParam::cvar_particleDensity->Set("0.7", true, false, false, true);
            break;
        }

        default: {
            CWorldParam::cvar_particleDensity->Set("1.0", true, false, false, true);
            break;
        }
    }
}

// VERIFY: 253389de auto
// OFFSET: 0x0078DDF0
void CWorldParam::SetDefaultProjectedTextures() {
    switch (g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel]) {
        case 0:
        case 1:
        case 2:
        case 7:
        case 8:
        case 9:
        case 10: {
            CWorldParam::cvar_projectedTextures->Set("0", true, false, false, true);
            break;
        }

        default: {
            CWorldParam::cvar_projectedTextures->Set("1", true, false, false, true);
            break;
        }
    }
}

// VERIFY: 92ae7057 auto
// OFFSET: 0x0078E950
void CWorldParam::Destroy() {
    RemoveConsoleDeviceDefaultCallback(CWorldParam::SetDefaults);
}

// VERIFY: 00239e3e auto
// OFFSET: 0x0078E1A0
void CWorldParam::SetDefaults(OPTION_PAGE page) {
    DefaultSettings* defaults = ConsoleDeviceGetDefaultSettings();

    if (page != OPTION_PAGE_1) {
        return;
    }

    char value[32];

    if (CWorldParam::cvar_farClip) {
        SStrPrintf(value, sizeof(value), "%f", defaults->m_farClip);
        CWorldParam::cvar_farClip->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_shadowLevel) {
        SStrPrintf(value, sizeof(value), "%d", defaults->m_terrainShadowLOD);
        CWorldParam::cvar_shadowLevel->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_maxLights) {
        SStrPrintf(value, sizeof(value), "%d", defaults->m_numLights);
        CWorldParam::cvar_maxLights->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_specular) {
        SStrPrintf(value, sizeof(value), defaults->m_specular ? "1" : "0");
        CWorldParam::cvar_specular->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_waterLOD) {
        SStrPrintf(value, sizeof(value), "%d", defaults->m_waterLOD);
        CWorldParam::cvar_waterLOD->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_particleDensity) {
        SStrPrintf(value, sizeof(value), "%f", defaults->m_particleDensity);
        CWorldParam::cvar_particleDensity->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_baseMip) {
        SStrPrintf(value, sizeof(value), "%d", defaults->m_baseMipLevel);
        CWorldParam::cvar_baseMip->Set(value, true, false, false, true);
    }

    if (CWorldParam::cvar_groundEffectDensity) {
        SStrPrintf(value, sizeof(value), "%d", defaults->m_detailDoodadDensity);
        CWorldParam::cvar_groundEffectDensity->Set(value, true, false, false, true);
    }

    CWorldParam::cvar_environmentDetail->Default();
    CWorldParam::cvar_groundEffectDist->Set("70.0", true, false, false, true);

    CVar* var = CVar::Lookup("weatherDensity");

    if (var) {
        var->Default();
    }

    var = CVar::Lookup("extShadowQuality");

    if (var) {
        var->Default();
    }

    var = CVar::Lookup("ffxDeath");

    if (var) {
        var->Default();
    }

    var = CVar::Lookup("ffxGlow");

    if (var) {
        var->Default();
    }

    CWorldParam::SetDefaultParticleDensity();
    CWorldParam::SetDefaultProjectedTextures();
}
