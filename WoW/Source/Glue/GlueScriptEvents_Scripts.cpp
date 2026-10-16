#include "GlueMgr.h"
#include <cstdint>
#include <cstdio>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Console/ConsoleClient.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/AddOns.h"
#include "WoW/Source/WowServices/Login.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/Event/Event.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Glue/CharacterSelection.h"
#include "WoW/Source/WowServices/BattlenetLogin.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"

int32_t Script_GetLocale(lua_State* L);
static int32_t Script_PlayCreditsMusic(lua_State* L);
static int32_t Script_PatchDownloadProgress(lua_State* L);
static int32_t Script_GetAddOnEnableState(lua_State* L);
static int32_t Script_ResetAddOns(lua_State* L);
static int32_t Script_GetBillingTimeRemaining(lua_State* L);
static int32_t Script_GetBillingPlan(lua_State* L);
int32_t Script_GetBillingTimeRested(lua_State* L);
int32_t Script_IsLinuxClient(lua_State* L);
int32_t Script_RestoreVideoResolutionDefaults(lua_State* L);
int32_t Script_RestoreVideoEffectsDefaults(lua_State* L);
int32_t Script_RestoreVideoStereoDefaults(lua_State* L);

// VERIFY: 51aeb88d auto
// OFFSET: 0x004DBE60
static int32_t Script_GetBuildInfo(lua_State* L) {
    lua_pushstring(L, FrameScript_GetText("VERSION", -1, GENDER_NOT_APPLICABLE));
    lua_pushstring(L, FrameScript_GetText("RELEASE_BUILD", -1, GENDER_NOT_APPLICABLE));
    lua_pushstring(L, "3.3.5");
    lua_pushstring(L, "12340");
    lua_pushstring(L, "Jun 24 2010");
    return 5;
}

// VERIFY: aa3b9fda auto
// OFFSET: 0x004DBEC0
static int32_t Script_SetSavedAccountName(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetSavedAccountName(\"accountName\")");
        return 0;
    }

    g_accountNameVar->Set(lua_tolstring(L, 1, nullptr), true, false, false, true);
    return 0;
}

// VERIFY: e4dc65d8 auto
// OFFSET: 0x004DBF10
static int32_t Script_GetUsesToken(lua_State* L) {
    lua_pushboolean(L, g_accountUsesTokenVar->m_intValue);
    return 1;
}

// VERIFY: 51e62777 auto
// OFFSET: 0x004DBF30
static int32_t Script_SetUsesToken(lua_State* L) {
    if (lua_type(L, 1) != LUA_TBOOLEAN) {
        luaL_error(L, "Usage: SetUsesToken( 0 | 1 )");
        return 0;
    }

    g_accountUsesTokenVar->Set(lua_toboolean(L, 1), true, false, false, true);
    return 0;
}

// VERIFY: 90f2304e auto
// OFFSET: 0x004DBF80
static int32_t Script_SetSavedAccountList(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetSavedAccountList(\"accountList\")");
        return 0;
    }

    g_accountListVar->Set(lua_tolstring(L, 1, nullptr), true, false, false, true);
    return 0;
}

// VERIFY: cfd5b3b3 auto
// OFFSET: 0x004DC000
static int32_t Script_SetCurrentScreen(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetCurrentScreen(\"screen\")");
        return 0;
    }

    CGlueMgr::UpdateCurrentScreen(lua_tolstring(L, 1, nullptr));

    if (!Blizzard::Streaming::IsStreamingMode()) {
        AsyncFileReadWaitAll();
    }

    return 0;
}

// VERIFY: 158992c3 auto
// OFFSET: 0x004DC050
static int32_t Script_QuitGame(lua_State* L) {
    CGlueMgr::QuitGame();
    return 0;
}

// VERIFY: 4fac469e auto
// OFFSET: 0x004DC070
static int32_t Script_PlayGlueMusic(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: PlayGlueMusic(\"filename\")");
        return 0;
    }

    SI2::StartGlueMusic(lua_tolstring(L, 1, nullptr));
    return 0;
}

// VERIFY: 5af4c7e5 auto
// OFFSET: 0x004DC0C0
static int32_t Script_PlayGlueAmbience(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: PlayGlueAmbience(\"sound name\", \"(optional)fade in time\")");
        return 0;
    }

    float fadeInTime = lua_isnumber(L, 2) ? static_cast<float>(lua_tonumber(L, 2)) : -1.0f;
    SI2::StartGlueAmbience(lua_tolstring(L, 1, nullptr), fadeInTime);
    return 0;
}

// VERIFY: c5881bc5 auto
// OFFSET: 0x004DC130
static int32_t Script_StopGlueAmbience(lua_State* L) {
    SI2::StopGlueAmbience(1.0f);
    return 0;
}

// VERIFY: 5e262dd5 auto
// OFFSET: 0x004DC1A0
static int32_t Script_StopGlueMusic(lua_State* L) {
    SI2::StopCreditsMusic();
    SI2::StopGlueMusic(-1.0f);
    return 0;
}

// VERIFY: e490bdfe auto
// OFFSET: 0x004DC210
static int32_t Script_AcceptTOS(lua_State* L) {
    CGlueMgr::AcceptTOS();
    return 0;
}

// VERIFY: f03e3adb auto
// OFFSET: 0x004DC220
static int32_t Script_AcceptEULA(lua_State* L) {
    CGlueMgr::AcceptEULA();
    return 0;
}

// VERIFY: 130bc16e auto
// OFFSET: 0x004DC230
static int32_t Script_AcceptTerminationWithoutNotice(lua_State* L) {
    CGlueMgr::AcceptTerminationWithoutNotice();
    return 0;
}

// VERIFY: 900c11ab auto
// OFFSET: 0x004DC240
static int32_t Script_AcceptScanning(lua_State* L) {
    CGlueMgr::AcceptScanning();
    return 0;
}

// VERIFY: 25b43275 auto
// OFFSET: 0x004DC250
static int32_t Script_AcceptContest(lua_State* L) {
    CGlueMgr::AcceptContest();
    return 0;
}

// VERIFY: 37439067 auto
// OFFSET: 0x004DC260
static int32_t Script_DefaultServerLogin(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: DefaultServerLogin(\"accountName\", \"password\")");
        return 0;
    }

    CGlueMgr::LoginServerLogin(lua_tolstring(L, 1, nullptr), const_cast<char*>(lua_tolstring(L, 2, nullptr)));
    return 0;
}

// VERIFY: 43cc0a55 auto
// OFFSET: 0x004DC500
static int32_t Script_StatusDialogClick(lua_State* L) {
    CGlueMgr::StatusDialogClick();
    return 0;
}

// VERIFY: 932722c3 auto
// OFFSET: 0x004DCA70
static int32_t Script_DisableAllAddOns(lua_State* L) {
    const char* character = lua_tolstring(L, 1, nullptr);

    for (uint32_t i = 0; i < GetNumAddOns(); i++) {
        SetAddOnEnableState(GetAddOnByIndex(i), character, 0);
    }

    return 0;
}

// VERIFY: 3e217cb6 auto
// OFFSET: 0x004DCB60
static int32_t Script_GetCursorPosition(lua_State* L) {
    float x = 0.0f;
    float y = 0.0f;
    NDCToDDC(CSimpleTop::s_instance->m_mousePosition.m_x, CSimpleTop::s_instance->m_mousePosition.m_y, &x, &y);

    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * x));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * y));
    return 2;
}

// VERIFY: d28d2800 auto
// OFFSET: 0x004DCBF0
static int32_t Script_GetClientExpansionLevel(lua_State* L) {
    lua_pushnumber(L, GetClientExpansionLevel() + 1);
    return 1;
}

// VERIFY: 27579bbd auto
// OFFSET: 0x004DCC20
static int32_t Script_GetScreenWidth(lua_State* L) {
    float width = NDCToDDCWidth(1.0f);
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * width));
    return 1;
}

// VERIFY: b457abf4 auto
// OFFSET: 0x004DCC70
static int32_t Script_GetScreenHeight(lua_State* L) {
    float height = NDCToDDCHeight(1.0f);
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * height));
    return 1;
}

// VERIFY: bb3dc1d7 auto
// OFFSET: 0x004DCCF0
static int32_t Script_ScanDLLStart(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: ScanDLLStart(\"VersionURL\", \"DLLURL\")");
        return 0;
    }

    CGlueMgr::ScanDLLStart(lua_tolstring(L, 1, nullptr), lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: 6e20cc07 auto
// OFFSET: 0x004DCDF0
static int32_t Script_ScanDLLContinueAnyway(lua_State* L) {
    CGlueMgr::ScanDLLContinueAnyway();
    return 0;
}

// VERIFY: a5c4e4e7 auto
// OFFSET: 0x004DCE00
static int32_t Script_IsScanDLLFinished(lua_State* L) {
    if (CGlueMgr::IsScanDLLFinished()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e95a71be auto
// OFFSET: 0x004DCE40
int32_t Script_IsWindowsClient(lua_State* L) {
    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: 2416e552 manual
// OFFSET: 0x004DCE70
static int32_t Script_SetCVar(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetCVar(\"cvar\", value [, \"scriptCvar\")");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    CVar* var = CVar::LookupRegistered(name);

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", name);
        return 0;
    }

    if (var->m_flags & 0x4) {
        luaL_error(L, "\"%s\" is read-only", name);
        return 0;
    }

    const char* value = lua_tolstring(L, 2, nullptr);

    if (!value) {
        value = "0";
    }

    var->Set(value, true, false, false, true);
    return 0;
}

// VERIFY: f4464896 auto
// OFFSET: 0x004DCF30
static int32_t Script_ShowChangedOptionWarnings(lua_State* L) {
    if (CGlueMgr::ShowChangedOptionWarnings()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5b4fcbcb auto
// OFFSET: 0x004DD380
static int32_t Script_StopAllSFX(lua_State* L) {
    SI2::StopAllSounds(static_cast<float>(lua_tonumber(L, 1)));
    return 0;
}

// VERIFY: e1c1f234 auto
// OFFSET: 0x004DD400
int32_t Script_RestartGx(lua_State* L) {
    ConsoleCommandExecute("gxRestart", 1);
    return 0;
}

// VERIFY: 8043f47f auto
// OFFSET: 0x004DD440
static int32_t Script_IsStreamingTrial(lua_State* L) {
    return SFile::IsTrial();
}

// VERIFY: e53cb78a auto
// OFFSET: 0x004DD450
static int32_t Script_IsConsoleActive(lua_State* L) {
    if (ConsoleGetActive()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7284312f auto
// OFFSET: 0x004DD490
static int32_t Script_RunScript(lua_State* L) {
    if (lua_isstring(L, 1)) {
        const char* script = lua_tolstring(L, 1, nullptr);

        if (script && *script) {
            FrameScript_Execute(script, script, nullptr);
        }
    }

    return 0;
}

// VERIFY: be94c246 auto
// OFFSET: 0x004DD4D0
static int32_t Script_IsTrialAccount(lua_State* L) {
    CVar* converted = CVar::Lookup("converted");
    Login* login = ClientServices::LoginConnection();
    int32_t trial;

    if (login) {
        trial = login->IsTrialAccount();
    } else {
        if (!converted) {
            lua_pushnil(L);
            return 1;
        }

        trial = converted->m_intValue == 0;
    }

    if (trial) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c9c81de1 auto
// OFFSET: 0x004DD540
static int32_t Script_IsSystemSupported(lua_State* L) {
    if (OsGetProcessorFeatures() & 0x4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 0884af43 auto
// OFFSET: 0x004DD5D0
static int32_t Script_GetSavedAccountName(lua_State* L) {
    lua_pushstring(L, g_accountNameVar->m_stringValue.m_str);
    return 1;
}

// VERIFY: 2a52a3da auto
// OFFSET: 0x004DD5F0
static int32_t Script_GetSavedAccountList(lua_State* L) {
    lua_pushstring(L, g_accountListVar->m_stringValue.m_str);
    return 1;
}

// VERIFY: 03843444 auto
// OFFSET: 0x004DD680
static int32_t Script_ShowTOSNotice(lua_State* L) {
    if (CGlueMgr::m_rereadTOS) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1c86bd69 auto
// OFFSET: 0x004DD6C0
static int32_t Script_TOSAccepted(lua_State* L) {
    if (CGlueMgr::m_acceptedTOS) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 680dccbc auto
// OFFSET: 0x004DD700
static int32_t Script_ShowEULANotice(lua_State* L) {
    if (CGlueMgr::m_rereadEULA) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9b470de5 auto
// OFFSET: 0x004DD740
static int32_t Script_EULAAccepted(lua_State* L) {
    if (CGlueMgr::m_acceptedEULA) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: d7a5f107 auto
// OFFSET: 0x004DD780
static int32_t Script_ShowTerminationWithoutNoticeNotice(lua_State* L) {
    if (CGlueMgr::m_rereadTerminationWithoutNotice) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: b6c38c76 auto
// OFFSET: 0x004DD7C0
static int32_t Script_TerminationWithoutNoticeAccepted(lua_State* L) {
    if (CGlueMgr::m_acceptedTerminationWithoutNotice) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: ca45cd78 auto
// OFFSET: 0x004DD800
static int32_t Script_ShowScanningNotice(lua_State* L) {
    if (CGlueMgr::m_rereadScanning) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 4806aa2e auto
// OFFSET: 0x004DD840
static int32_t Script_ScanningAccepted(lua_State* L) {
    if (CGlueMgr::m_acceptedScanning) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 576d5e01 auto
// OFFSET: 0x004DD880
static int32_t Script_ShowContestNotice(lua_State* L) {
    if (CGlueMgr::m_rereadContest) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7c5f1ee5 auto
// OFFSET: 0x004DD8C0
static int32_t Script_ContestAccepted(lua_State* L) {
    if (CGlueMgr::m_acceptedContest) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: cff4ad26 auto
// OFFSET: 0x004DD900
static int32_t Script_GetServerName(lua_State* L) {
    lua_pushstring(L, ClientServices::GetSelectedRealmName());

    int32_t pvp = 0;
    int32_t rp = 0;
    int32_t down = 1;
    REALM_INFO* realm = ClientServices::GetSelectedRealm();

    if (realm) {
        for (int32_t i = 0; i < g_cfg_ConfigsDB.m_numRecords; i++) {
            Cfg_ConfigsRec* config = i >= 0 && i < g_cfg_ConfigsDB.m_numRecords ? &g_cfg_ConfigsDB.m_records[i] : nullptr;

            if (config->m_realmType == realm->m_type) {
                pvp = config->m_playerKillingAllowed != 0;
                rp = config->m_roleplaying != 0;
                down = realm->m_flags & 0x2;
                break;
            }
        }
    }

    if (pvp) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (rp) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: 3276d519 auto
// OFFSET: 0x004DDD10
static int32_t Script_GetCVar(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVar(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    lua_pushstring(L, var->m_stringValue.m_str);
    return 1;
}

// VERIFY: 41172ffb auto
// OFFSET: 0x004DDE10
static int32_t Script_GetCVarDefault(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarDefault(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    if (var->m_defaultValue.m_str) {
        lua_pushstring(L, var->m_defaultValue.m_str);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2f802eaf auto
// OFFSET: 0x004DDEA0
static int32_t Script_GetCVarMin(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarMin(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    const char* name = var->m_key.m_str;

    if (!SStrCmpI(name, "extShadowQuality", STORM_MAX_STR)) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    if (!SStrCmpI(name, "farclip", STORM_MAX_STR)) {
        lua_pushnumber(L, 177.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e71bc59f manual
// OFFSET: 0x004DDF80
static int32_t Script_GetCVarMax(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarMax(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    const char* name = var->m_key.m_str;

    if (!SStrCmpI(name, "extShadowQuality", STORM_MAX_STR)) {
        uint32_t mode = 0;
        uint32_t maxMode = CShadowCache::GetMaxShadowMode();

        do {
            if (!CShadowCache::ValidateShadowMode(static_cast<EShadowMode>(mode))) {
                break;
            }

            mode++;
        } while (mode <= maxMode);

        lua_pushnumber(L, mode - 1);
        return 1;
    }

    if (!SStrCmpI(name, "farclip", STORM_MAX_STR)) {
        lua_pushnumber(L, 1277.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: fec93d6e auto
// OFFSET: 0x004DE090
static int32_t Script_GetCVarAbsoluteMin(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarAbsoluteMin(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    if (SStrCmpI(var->m_key.m_str, "extShadowQuality", STORM_MAX_STR)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: e499875e auto
// OFFSET: 0x004DE130
static int32_t Script_GetCVarAbsoluteMax(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarAbsoluteMax(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    if (SStrCmpI(var->m_key.m_str, "extShadowQuality", STORM_MAX_STR)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, CShadowCache::GetMaxShadowMode());
    }

    return 1;
}

// VERIFY: 584a0e2b manual
// OFFSET: 0x004DE1F0
static int32_t Script_GetChangedOptionWarnings(lua_State* L) {
    uint32_t count = CGlueMgr::m_numChangedOptionWarnings;

    if (!count) {
        return 0;
    }

    lua_checkstack(L, count);

    for (uint32_t i = 0; i < count; i++) {
        const char* warning = CGlueMgr::m_changedOptionWarnings[i];
        lua_pushlstring(L, warning, SStrLen(warning));
    }

    return count;
}

// VERIFY: 02785aeb auto
// OFFSET: 0x00510B90
int32_t Script_IsMacClient(lua_State* L) {
    lua_pushnil(L);
    return 1;
}

// VERIFY: 31b85cc7 auto
// OFFSET: 0x00510B90
int32_t Script_IsLinuxClient(lua_State* L) {
    lua_pushnil(L);
    return 1;
}

// VERIFY: f53e291b auto
// OFFSET: 0x004DD610
int32_t Script_GetMovieResolution(lua_State* L) {
    CVar* resolution = CVar::Lookup("gxResolution");

    if (!resolution || !resolution->m_stringValue.m_str) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    int32_t width;
    int32_t height;
    sscanf(resolution->m_stringValue.m_str, "%dx%d", &width, &height);
    lua_pushnumber(L, width);

    return 1;
}

// VERIFY: 63dd1965 auto
// OFFSET: 0x004DDB90
static int32_t Script_ShowCursor(lua_State* L) {
    g_theGxDevicePtr->CursorSetVisible(1);
    return 0;
}

// VERIFY: 392c1d3f auto
// OFFSET: 0x004DDBB0
static int32_t Script_HideCursor(lua_State* L) {
    g_theGxDevicePtr->CursorSetVisible(0);

    return 0;
}

// VERIFY: bca364a3 auto
// OFFSET: 0x004DDC20
int32_t Script_GetAccountExpansionLevel(lua_State* L) {
    lua_pushnumber(L, ClientServices::Connection()->m_accountExpansion);

    return 1;
}

// VERIFY: 7a8c8b01 auto
// OFFSET: 0x004DDD90
static int32_t Script_GetCVarBool(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarBool(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (var && StringToBOOL(var->m_stringValue.m_str)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e563d98b auto
// OFFSET: 0x004DDA10
static int32_t Script_DisconnectFromServer(lua_State* L) {
    if (ClientServices::Connection()->m_connected) {
        CGlueMgr::m_disconnectPending = 1;
        ClientServices::Connection()->Disconnect();
    }

    ClientServices::LoginConnection()->Logoff();
    return 0;
}

// VERIFY: 4cbb98fd auto
// OFFSET: 0x004DC510
static int32_t Script_EnterWorld(lua_State* L) {
    CGlueMgr::EnterWorld();
    return 0;
}

// VERIFY: fd0c42a5 auto
// OFFSET: 0x004DDA50
static int32_t Script_IsConnectedToServer(lua_State* L) {
    if (ClientServices::Connection()->m_connected) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1e6a625a manual
// OFFSET: 0x004DE2E0
static int32_t Script_RequestRealmSplitInfo(lua_State* L) {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REALM_SPLIT));
    msg.Put(static_cast<uint32_t>(0xFFFFFFFF));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 4229109a manual
// OFFSET: 0x004DE390
static int32_t Script_ReadyForAccountDataTimes(lua_State* L) {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_READY_FOR_ACCOUNT_DATA_TIMES));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: da8be63e manual
// OFFSET: 0x00511200
int32_t Script_GetNumAddOns(lua_State* L) {
    lua_pushnumber(L, GetNumAddOns());
    return 1;
}

// VERIFY: 9ab1ae8b auto
// OFFSET: 0x004DCAD0
static int32_t Script_ResetAddOns(lua_State* L) {
    LoadAddOnEnableState(nullptr);
    return 0;
}

// VERIFY: 7a7ef6f2 auto
// OFFSET: 0x004DD420
int32_t Script_RestoreVideoStereoDefaults(lua_State* L) {
    ConsoleDeviceDefaults(OPTION_PAGE_2);
    return 0;
}

// VERIFY: 6f95e4dc auto
// OFFSET: 0x004DBFD0
int32_t Script_GetLocale(lua_State* L) {
    lua_pushstring(L, s_iniLocaleNames[s_locale]);
    return 1;
}

// VERIFY: ac7e3070 auto
// OFFSET: 0x004DDA90
static int32_t Script_PatchDownloadProgress(lua_State* L) {
    lua_pushnumber(L, CGlueMgr::m_patchDownloadProgress);
    return 1;
}

// VERIFY: e963f241 auto
// OFFSET: 0x004DC150
static int32_t Script_PlayCreditsMusic(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: PlayCreditsMusic( \"Sound kit name\" )");
        return 0;
    }

    SI2::StartCreditsMusic(lua_tolstring(L, 1, nullptr));
    return 0;
}

// VERIFY: dab7aa7c manual
// OFFSET: 0x004DDBD0
static int32_t Script_GetBillingTimeRemaining(lua_State* L) {
    uint32_t remaining = ClientServices::Connection()->m_billingTimeRemaining;

    if (ClientServices::Connection()->m_billingFlags & 0x2) {
        remaining = 0;
    }

    lua_pushnumber(L, remaining);
    return 1;
}

// VERIFY: 2dd7ddc5 auto
// OFFSET: 0x004DDC60
static int32_t Script_GetBillingPlan(lua_State* L) {
    uint32_t flags = ClientServices::Connection()->m_billingFlags;
    uint32_t plan = flags & 0x16;
    int32_t type = 0;

    if (plan == 0x2) {
        type = 1;
    } else if (plan == 0x4) {
        type = 2;
    } else if (!plan) {
        type = 3;
    } else if (plan == 0x10) {
        type = 4;
    }

    lua_pushnumber(L, type);
    lua_pushnumber(L, (flags & 0x20) != 0);
    lua_pushnumber(L, (flags & 0x8) != 0);
    return 3;
}

// VERIFY: 6d5a65a7 auto
// OFFSET: 0x004DC520
static int32_t Script_Screenshot(lua_State* L) {
    ScrnScreenshot(&CGlueMgr::HandleScreenshot, reinterpret_cast<uint8_t*>(ClientServices::GetClientStamp()), 88);
    return 0;
}

// VERIFY: 0aa14818 auto
// OFFSET: 0x004DC7C0
static int32_t Script_GetAddOnEnableState(lua_State* L) {
    if ((!lua_isstring(L, 1) && lua_type(L, 1)) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAddOnEnableState(\"character\", index)");
        return 0;
    }

    const char* character = lua_tolstring(L, 1, nullptr);
    const char* name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 2)) - 1);

    if (!name) {
        luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
        return 0;
    }

    lua_pushnumber(L, GetAddOnEnableState(name, character, true));
    return 1;
}

// VERIFY: 88319ee9 auto
// OFFSET: 0x004DC1C0
static int32_t Script_LaunchURL(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: LaunchURL(\"URL\")");
        return 0;
    }

    ClientServices::LaunchURL(lua_tolstring(L, 1, nullptr));
    return 0;
}

// VERIFY: ca947a95 auto
// OFFSET: 0x004DCD60
static int32_t Script_GetCreditsText(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: Script_GetCreditText(versionIndex)");
        return 0;
    }

    int32_t version = static_cast<int32_t>(lua_tonumber(L, 1));
    const char* filename;

    if (version == 1) {
        filename = "credits.html";
    } else if (version == 2) {
        filename = "credits_BC.html";
    } else {
        filename = "credits_LK.html";
    }

    void* text;

    if (SFile::ReadFile(nullptr, filename, &text, nullptr, 1, 1, nullptr)) {
        lua_pushstring(L, static_cast<const char*>(text));
        SFile::Unload(text);
    }

    return 1;
}

// VERIFY: a2877425 auto
// OFFSET: 0x004DC4D0
static int32_t Script_TokenEntered(lua_State* L) {
    if (lua_isstring(L, 1)) {
        CGlueMgr::LoginEnterToken(lua_tolstring(L, 1, nullptr));
    }

    return 0;
}

// VERIFY: 4fe1a04b auto
// OFFSET: 0x004DC560
static int32_t Script_GetAddOnInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAddOnInfo(index)");
        return 0;
    }

    const char* name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

    if (!name) {
        luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
        return 0;
    }

    lua_pushstring(L, name);
    const char* title = GetAddOnTitle(name);

    if (title) {
        lua_pushstring(L, title);
    } else {
        lua_pushnil(L);
    }

    const char* notes = GetAddOnNotes(name);

    if (notes) {
        lua_pushstring(L, notes);
    } else {
        lua_pushnil(L);
    }

    const char* url = GetAddOnURL(name);

    if (url) {
        lua_pushstring(L, url);
    } else {
        lua_pushnil(L);
    }

    UIADDON_LOADABLE_REASON reason = UIADDON_LOADABLE;
    UIADDON_LOADABLE_REASON dependencyReason = UIADDON_LOADABLE;

    if (IsAddOnLoadable(name, false, reason, dependencyReason, nullptr)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (reason || dependencyReason) {
        char buffer[128];
        lua_pushstring(L, AddOnUnloadableToken(reason, dependencyReason, buffer, sizeof(buffer)));
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, AddOnSecurityToken(GetAddOnSecurity(name)));

    if (IsAddOnUpdateAvailable(name)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 8;
}

// VERIFY: 21cbee05 auto
// OFFSET: 0x004DCAC0
static int32_t Script_SaveAddOns(lua_State* L) {
    SaveAddOnEnableState();
    return 0;
}

// VERIFY: fceeae16 auto
// OFFSET: 0x004DCAE0
static int32_t Script_IsAddonVersionCheckEnabled(lua_State* L) {
    if (IsAddonVersionCheckEnabled()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a66ac9ee auto
// OFFSET: 0x004DE250
static int32_t Script_SetRealmSplitState(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetRealmSplitState(1 or 2)");
        return 0;
    }

    uint32_t state = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (state <= 2) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REALM_SPLIT));
        msg.Put(state);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 042aa045 auto
// OFFSET: 0x004DBE10
static int32_t Script_IsShiftKeyDown(lua_State* L) {
    if (EventIsKeyDown(KEY_LSHIFT) || EventIsKeyDown(KEY_RSHIFT)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 24f12e8c manual
// OFFSET: 0x004DCB20
static int32_t Script_SetAddonVersionCheck(lua_State* L) {
    if (StringToBOOL(L, 1, 1)) {
        SetAddonVersionCheck(1);
    } else {
        SetAddonVersionCheck(0);
    }

    return 0;
}

// VERIFY: 438aa8da auto
// OFFSET: 0x004DCF70
static int32_t Script_GetNumDeclensionSets(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetNumDeclensionSets(\"name\", gender)");
        return 0;
    }

    int32_t sex = 2;
    const char* name = lua_tolstring(L, 1, nullptr);

    if (lua_isnumber(L, 2)) {
        int32_t value = lua_tointeger(L, 2);

        for (int32_t i = 0; i < 3; i++) {
            if (g_scriptSexIndex[i] == value) {
                sex = i;
                break;
            }
        }
    }

    lua_pushnumber(L, GetNumNameDeclensionSets(s_locale, name, sex));
    return 1;
}

// VERIFY: e08d08fe auto
// OFFSET: 0x004DD020
static int32_t Script_DeclineName(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: DeclineName(\"name\", gender, declensionSet)");
        return 0;
    }

    int32_t sex = 2;
    const char* name = lua_tolstring(L, 1, nullptr);

    if (lua_isnumber(L, 2)) {
        int32_t value = lua_tointeger(L, 2);

        for (int32_t i = 0; i < 3; i++) {
            if (g_scriptSexIndex[i] == value) {
                sex = i;
                break;
            }
        }
    }

    char declensions[5][1024];

    if (GetNameDeclensions(s_locale, declensions[0], name, sex, static_cast<int32_t>(lua_tonumber(L, 3)) - 1)) {
        for (uint32_t i = 0; i < 5; i++) {
            lua_pushstring(L, declensions[i]);
        }

        return 5;
    }

    for (uint32_t i = 0; i < 5; i++) {
        lua_pushnil(L);
    }

    return 5;
}

// VERIFY: 93189456 auto
// OFFSET: 0x004DD130
static int32_t Script_GetNumGameAccounts(lua_State* L) {
    Login* login = ClientServices::LoginConnection();

    if (!login) {
        luaL_error(L, "No login available");
        return 0;
    }

    if (login->GetLoginServerType() != 1) {
        luaL_error(L, "Wrong login type");
        return 0;
    }

    lua_pushnumber(L, static_cast<BattlenetLogin*>(login)->GetNumGameAccounts());
    return 1;
}

// VERIFY: fbabed65 manual
// OFFSET: 0x004DD1C0
static int32_t Script_GetGameAccountInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGameAccountInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    Login* login = ClientServices::LoginConnection();

    if (!login) {
        luaL_error(L, "No login available");
        return 0;
    }

    if (login->GetLoginServerType() != 1) {
        luaL_error(L, "Wrong login type");
        return 0;
    }

    BattlenetLogin* battlenet = static_cast<BattlenetLogin*>(login);
    const char* name = battlenet->GetGameAccountName(index);

    if (!name) {
        return 0;
    }

    lua_pushstring(L, name);
    lua_pushnumber(L, battlenet->GetGameAccountRegion(index));
    return 2;
}

// VERIFY: a295ea7c manual
// OFFSET: 0x004DD2B0
static int32_t Script_SetGameAccount(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetGameAccount(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    Login* login = ClientServices::LoginConnection();

    if (!login) {
        luaL_error(L, "No login available");
        return 0;
    }

    if (login->GetLoginServerType() != 1) {
        luaL_error(L, "Wrong login type");
        return 0;
    }

    lua_pushboolean(L, static_cast<BattlenetLogin*>(login)->SetGameAccount(index));
    return 1;
}

// VERIFY: d0d3a8b0 manual
// OFFSET: 0x004DD3B0
static int32_t Script_SetClearConfigData(lua_State* L) {
    if (lua_type(L, 1) == LUA_TBOOLEAN) {
        SetClearConfigData(lua_toboolean(L, 1));
    } else {
        SetClearConfigData(1);
    }

    return 0;
}

// VERIFY: 145f8072 auto
// OFFSET: 0x004DDAC0
static int32_t Script_GetAddOnDependencies(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAddOnDependencies(index)");
        return 0;
    }

    const char* name = GetAddOnByIndex(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);

    if (!name) {
        luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
        return 0;
    }

    TSGrowableArray<char*>* dependencies = GetAddonDeps(name);

    if (!dependencies) {
        return 0;
    }

    lua_checkstack(L, dependencies->m_count);

    for (uint32_t i = 0; i < dependencies->m_count; i++) {
        lua_pushstring(L, dependencies->m_data[i]);
    }

    return dependencies->m_count;
}

// VERIFY: 44b9454f auto
// OFFSET: 0x004DC060
static int32_t Script_QuitGameAndRunLauncher(lua_State* L) {
    CGlueMgr::QuitGameAndRunLauncher();
    return 0;
}

// VERIFY: 30f9beb4 manual
// OFFSET: 0x004DC2D0
static int32_t Script_PINEntered(lua_State* L) {
    uint8_t digits[10];
    int32_t count = 0;

    for (int32_t i = 1; i < 11; i++) {
        if (lua_type(L, i) <= LUA_TNIL) {
            break;
        }

        if (!lua_isnumber(L, i)) {
            luaL_error(L, "Usage: PINEntered(4-10 key positions) - non-numeric arg #%d", count + 1);
        }

        digits[i - 1] = static_cast<uint8_t>(lua_tonumber(L, i));
        count++;
    }

    if (static_cast<uint32_t>(count - 4) > 6) {
        luaL_error(L, "Usage: PINEntered(4-10 key positions) - bad number of args %d", count);
    }

    CGlueMgr::LoginEnterPIN(count, digits);
    return 0;
}

// VERIFY: 7ff32db5 manual
// OFFSET: 0x004DC3A0
static int32_t Script_MatrixEntered(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: MatrixEntered(number)");
    }

    double value = lua_tonumber(L, 1);

    if (value > 9) {
        luaL_error(L, "MatrixEntered: number must be 0 to 9");
    }

    CGlueMgr::LoginEnterMatrix(static_cast<uint8_t>(value));
    return 0;
}

// VERIFY: 587d557c auto
// OFFSET: 0x004DC420
static int32_t Script_MatrixRevert(lua_State* L) {
    CGlueMgr::LoginRevertMatrix();
    return 0;
}

// VERIFY: f7d16787 auto
// OFFSET: 0x004DC430
static int32_t Script_MatrixCommit(lua_State* L) {
    CGlueMgr::LoginCommitMatrix();
    return 0;
}

// VERIFY: dd9ba17d auto
// OFFSET: 0x004DC440
static int32_t Script_GetMatrixCoordinates(lua_State* L) {
    uint32_t x;
    uint32_t y;

    if (CGlueMgr::LoginGetMatrixCoordinate(&x, &y)) {
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: da79f55e auto
// OFFSET: 0x004DC540
static int32_t Script_PatchDownloadCancel(lua_State* L) {
    CGlueMgr::PatchDownloadCancel();
    return 0;
}

// VERIFY: d348a87d auto
// OFFSET: 0x004DC550
static int32_t Script_PatchDownloadApply(lua_State* L) {
    CGlueMgr::PatchDownloadApply();
    return 0;
}

// VERIFY: b3ad3e87 manual
// OFFSET: 0x004DC720
static int32_t Script_LaunchAddOnURL(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: LaunchAddOnURL(index)");
    }

    const char* name = GetAddOnByIndex(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1))) - 1);

    if (!name) {
        luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
    }

    const char* url = GetAddOnURL(name);

    if (url) {
        OsLaunchURL(url);
    }

    return 0;
}

// VERIFY: 8d79714e auto
// OFFSET: 0x004DC8A0
static int32_t Script_EnableAddOn(lua_State* L) {
    if ((!lua_isstring(L, 1) && lua_type(L, 1)) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: EnableAddOn(\"character\", index)");
    }

    const char* character = lua_tolstring(L, 1, nullptr);
    const char* name = GetAddOnByIndex(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 2))) - 1);

    if (!name) {
        luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
    }

    SetAddOnEnableState(name, character, 2);
    return 0;
}

// VERIFY: 42d43910 auto
// OFFSET: 0x004DC960
static int32_t Script_EnableAllAddOns(lua_State* L) {
    const char* character = lua_tolstring(L, 1, nullptr);

    for (uint32_t i = 0; i < GetNumAddOns(); i++) {
        SetAddOnEnableState(GetAddOnByIndex(i), character, 2);
    }

    return 0;
}

// VERIFY: 92fdab84 auto
// OFFSET: 0x004DC9B0
static int32_t Script_DisableAddOn(lua_State* L) {
    if ((!lua_isstring(L, 1) && lua_type(L, 1)) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: DisableAddOn(\"character\", index)");
    }

    const char* character = lua_tolstring(L, 1, nullptr);
    const char* name = GetAddOnByIndex(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 2))) - 1);

    if (!name) {
        luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
    }

    SetAddOnEnableState(name, character, 0);
    return 0;
}

// VERIFY: 2c62788c auto
// OFFSET: 0x004DCCC0
static int32_t Script_SurveyNotificationDone(lua_State* L) {
    if (StringToBOOL(L, 1, 0)) {
        CGlueMgr::SurveySendResults();
    }

    CGlueMgr::SurveyDownloadEnd();
    return 1;
}

// VERIFY: 38032ddb auto
// OFFSET: 0x004DCE60
static int32_t Script_CancelLogin(lua_State* L) {
    CGlueMgr::CancelLogin();
    return 0;
}

// VERIFY: 121c3b3a auto
// OFFSET: 0x004DCF20
static int32_t Script_AcceptChangedOptionWarnings(lua_State* L) {
    CGlueMgr::AcceptChangedOptionWarnings();
    return 0;
}

// VERIFY: a8e471a8 auto
// OFFSET: 0x004DD430
static int32_t Script_IsStreamingMode(lua_State* L) {
    return SFile::IsStreamingMode();
}

// STATIC: 0x00AC3E00
static FrameScript_Method s_scriptFunctions[] = {
    { "IsShiftKeyDown", &Script_IsShiftKeyDown },
    { "GetBuildInfo", &Script_GetBuildInfo },
    { "GetLocale", &Script_GetLocale },
    { "GetSavedAccountName", &Script_GetSavedAccountName },
    { "SetSavedAccountName", &Script_SetSavedAccountName },
    { "GetUsesToken", &Script_GetUsesToken },
    { "SetUsesToken", &Script_SetUsesToken },
    { "GetSavedAccountList", &Script_GetSavedAccountList },
    { "SetSavedAccountList", &Script_SetSavedAccountList },
    { "SetCurrentScreen", &Script_SetCurrentScreen },
    { "QuitGame", &Script_QuitGame },
    { "QuitGameAndRunLauncher", &Script_QuitGameAndRunLauncher },
    { "PlayGlueMusic", &Script_PlayGlueMusic },
    { "PlayCreditsMusic", &Script_PlayCreditsMusic },
    { "StopGlueMusic", &Script_StopGlueMusic },
    { "GetMovieResolution", &Script_GetMovieResolution },
    { "GetScreenWidth", &Script_GetScreenWidth },
    { "GetScreenHeight", &Script_GetScreenHeight },
    { "LaunchURL", &Script_LaunchURL },
    { "ShowTOSNotice", &Script_ShowTOSNotice },
    { "TOSAccepted", &Script_TOSAccepted },
    { "AcceptTOS", &Script_AcceptTOS },
    { "ShowEULANotice", &Script_ShowEULANotice },
    { "EULAAccepted", &Script_EULAAccepted },
    { "AcceptEULA", &Script_AcceptEULA },
    { "ShowTerminationWithoutNoticeNotice", &Script_ShowTerminationWithoutNoticeNotice },
    { "TerminationWithoutNoticeAccepted", &Script_TerminationWithoutNoticeAccepted },
    { "AcceptTerminationWithoutNotice", &Script_AcceptTerminationWithoutNotice },
    { "ShowScanningNotice", &Script_ShowScanningNotice },
    { "ScanningAccepted", &Script_ScanningAccepted },
    { "AcceptScanning", &Script_AcceptScanning },
    { "ShowContestNotice", &Script_ShowContestNotice },
    { "ContestAccepted", &Script_ContestAccepted },
    { "AcceptContest", &Script_AcceptContest },
    { "DefaultServerLogin", &Script_DefaultServerLogin },
    { "StatusDialogClick", &Script_StatusDialogClick },
    { "GetServerName", &Script_GetServerName },
    { "DisconnectFromServer", &Script_DisconnectFromServer },
    { "IsConnectedToServer", &Script_IsConnectedToServer },
    { "EnterWorld", &Script_EnterWorld },
    { "Screenshot", &Script_Screenshot },
    { "PatchDownloadProgress", &Script_PatchDownloadProgress },
    { "PatchDownloadCancel", &Script_PatchDownloadCancel },
    { "PatchDownloadApply", &Script_PatchDownloadApply },
    { "GetNumAddOns", &Script_GetNumAddOns },
    { "GetAddOnInfo", &Script_GetAddOnInfo },
    { "LaunchAddOnURL", &Script_LaunchAddOnURL },
    { "GetAddOnDependencies", &Script_GetAddOnDependencies },
    { "GetAddOnEnableState", &Script_GetAddOnEnableState },
    { "EnableAddOn", &Script_EnableAddOn },
    { "EnableAllAddOns", &Script_EnableAllAddOns },
    { "DisableAddOn", &Script_DisableAddOn },
    { "DisableAllAddOns", &Script_DisableAllAddOns },
    { "SaveAddOns", &Script_SaveAddOns },
    { "ResetAddOns", &Script_ResetAddOns },
    { "IsAddonVersionCheckEnabled", &Script_IsAddonVersionCheckEnabled },
    { "SetAddonVersionCheck", &Script_SetAddonVersionCheck },
    { "GetCursorPosition", &Script_GetCursorPosition },
    { "ShowCursor", &Script_ShowCursor },
    { "HideCursor", &Script_HideCursor },
    { "GetBillingTimeRemaining", &Script_GetBillingTimeRemaining },
    { "GetBillingPlan", &Script_GetBillingPlan },
    { "GetBillingTimeRested", &Script_GetBillingTimeRested },
    { "SurveyNotificationDone", &Script_SurveyNotificationDone },
    { "PINEntered", &Script_PINEntered },
    { "PlayGlueAmbience", &Script_PlayGlueAmbience },
    { "StopGlueAmbience", &Script_StopGlueAmbience },
    { "GetCreditsText", &Script_GetCreditsText },
    { "GetAccountExpansionLevel", &Script_GetAccountExpansionLevel },
    { "GetClientExpansionLevel", &Script_GetClientExpansionLevel },
    { "MatrixEntered", &Script_MatrixEntered },
    { "MatrixRevert", &Script_MatrixRevert },
    { "MatrixCommit", &Script_MatrixCommit },
    { "GetMatrixCoordinates", &Script_GetMatrixCoordinates },
    { "ScanDLLStart", &Script_ScanDLLStart },
    { "ScanDLLContinueAnyway", &Script_ScanDLLContinueAnyway },
    { "IsScanDLLFinished", &Script_IsScanDLLFinished },
    { "IsWindowsClient", &Script_IsWindowsClient },
    { "IsMacClient", &Script_IsMacClient },
    { "IsLinuxClient", &Script_IsLinuxClient },
    { "SetRealmSplitState", &Script_SetRealmSplitState },
    { "RequestRealmSplitInfo", &Script_RequestRealmSplitInfo },
    { "CancelLogin", &Script_CancelLogin },
    { "GetCVar", &Script_GetCVar },
    { "GetCVarBool", &Script_GetCVarBool },
    { "SetCVar", &Script_SetCVar },
    { "GetCVarDefault", &Script_GetCVarDefault },
    { "GetCVarMin", &Script_GetCVarMin },
    { "GetCVarMax", &Script_GetCVarMax },
    { "GetCVarAbsoluteMin", &Script_GetCVarAbsoluteMin },
    { "GetCVarAbsoluteMax", &Script_GetCVarAbsoluteMax },
    { "GetChangedOptionWarnings", &Script_GetChangedOptionWarnings },
    { "AcceptChangedOptionWarnings", &Script_AcceptChangedOptionWarnings },
    { "ShowChangedOptionWarnings", &Script_ShowChangedOptionWarnings },
    { "TokenEntered", &Script_TokenEntered },
    { "GetNumDeclensionSets", &Script_GetNumDeclensionSets },
    { "DeclineName", &Script_DeclineName },
    { "GetNumGameAccounts", &Script_GetNumGameAccounts },
    { "GetGameAccountInfo", &Script_GetGameAccountInfo },
    { "SetGameAccount", &Script_SetGameAccount },
    { "StopAllSFX", &Script_StopAllSFX },
    { "SetClearConfigData", &Script_SetClearConfigData },
    { "RestartGx", &Script_RestartGx },
    { "RestoreVideoResolutionDefaults", &Script_RestoreVideoResolutionDefaults },
    { "RestoreVideoEffectsDefaults", &Script_RestoreVideoEffectsDefaults },
    { "RestoreVideoStereoDefaults", &Script_RestoreVideoStereoDefaults },
    { "IsStreamingMode", &Script_IsStreamingMode },
    { "IsStreamingTrial", &Script_IsStreamingTrial },
    { "IsConsoleActive", &Script_IsConsoleActive },
    { "RunScript", &Script_RunScript },
    { "ReadyForAccountDataTimes", &Script_ReadyForAccountDataTimes },
    { "IsTrialAccount", &Script_IsTrialAccount },
    { "IsSystemSupported", &Script_IsSystemSupported },
};

// VERIFY: 56bca61d auto
// OFFSET: 0x004DD580
void GlueScriptEventsRegisterFunctions() {
    for (uint32_t i = 0; i < sizeof(s_scriptFunctions) / sizeof(s_scriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_scriptFunctions[i].m_name, s_scriptFunctions[i].m_method);
    }
}

// VERIFY: 80650abf auto
// OFFSET: 0x004DD5B0
void GlueScriptEventsUnregisterFunctions() {
    for (uint32_t i = 0; i < sizeof(s_scriptFunctions) / sizeof(s_scriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_scriptFunctions[i].m_name);
    }
}
