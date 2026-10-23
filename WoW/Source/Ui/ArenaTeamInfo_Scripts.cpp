#include "ArenaTeamInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ArenaTeamCache.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

// VERIFY: 1f34a576 auto
// OFFSET: 0x005A2A00
static int32_t Script_GetArenaTeamRosterShowOffline(lua_State* L) {
    if (CGArenaTeamInfo::m_showOffline) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: cb1155eb manual
// OFFSET: 0x005A2930
static int32_t Script_GetNumArenaTeamMembers(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetNumArenaTeamMembers(team [,showOffline])");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t showOffline = StringToBOOL(L, 2, 0);
    uint32_t count;

    if (team >= 3) {
        count = 0;
    } else if (CGArenaTeamInfo::m_showOffline || showOffline) {
        count = CGArenaTeamInfo::m_arenaTeams[team].m_numMembers;
    } else {
        count = CGArenaTeamInfo::m_arenaTeams[team].m_numOnline;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 79e3ee3a manual
// OFFSET: 0x005A3260
static int32_t Script_GetArenaTeamGdfInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetArenaTeamGdfInfo(team, index)");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    ArenaTeamMember* member;

    if (team < 3 && index < CGArenaTeamInfo::m_arenaTeams[team].m_numMembers && (member = CGArenaTeamInfo::m_arenaTeams[team].m_members.m_data[index])) {
        lua_pushnumber(L, member->m_matchmakerRatingBefore);
        lua_pushnumber(L, member->m_matchmakerRatingAfter);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 8b9583ab auto
// OFFSET: 0x005A28C0
static void ArenaTeamCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_UPDATE, nullptr);
    }
}

// VERIFY: fab9558e manual
// OFFSET: 0x005A3860
static int32_t Script_GetArenaTeam(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetArenaTeam(team)");
        return 0;
    }

    lua_checkstack(L, 22);
    uint32_t index = static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t* info = player && index < 3 ? &player->m_player->m_playerFieldArenaTeamInfo11[index * 7] : nullptr;
    ArenaTeam* team = index < 3 ? &CGArenaTeamInfo::m_arenaTeams[index] : nullptr;
    ArenaTeamCache* cache = nullptr;

    if (player && info && info[0] && team) {
        cache = g_arenaTeamCache.GetRecord(info[0], player->m_obj->m_guid, ArenaTeamCacheCallback, nullptr, false);
    }

    if (!cache) {
        lua_pushnil(L);

        for (int32_t i = 0; i < 21; i++) {
            lua_pushnumber(L, 0.0);
        }

        return 22;
    }

    lua_pushstring(L, cache->m_name);
    lua_pushnumber(L, static_cast<int32_t>(cache->m_type));
    lua_pushnumber(L, team->m_rating);
    lua_pushnumber(L, team->m_weekGames);
    lua_pushnumber(L, team->m_weekWins);
    lua_pushnumber(L, team->m_seasonGames);
    lua_pushnumber(L, team->m_seasonWins);
    lua_pushnumber(L, info[3]);
    lua_pushnumber(L, info[4]);
    lua_pushnumber(L, team->m_rank);
    lua_pushnumber(L, info[6]);
    lua_pushnumber(L, static_cast<int32_t>((cache->m_backgroundColor >> 16) & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>((cache->m_backgroundColor >> 8) & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>(cache->m_backgroundColor & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>(cache->m_emblemStyle));
    lua_pushnumber(L, static_cast<int32_t>((cache->m_emblemColor >> 16) & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>((cache->m_emblemColor >> 8) & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>(cache->m_emblemColor & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>(cache->m_borderStyle));
    lua_pushnumber(L, static_cast<int32_t>((cache->m_borderColor >> 16) & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>((cache->m_borderColor >> 8) & 0xFF) * (1.0f / 255.0f));
    lua_pushnumber(L, static_cast<int32_t>(cache->m_borderColor & 0xFF) * (1.0f / 255.0f));
    return 22;
}

// VERIFY: 9361dbb0 auto
// OFFSET: 0x005A2A40
static int32_t Script_GetCurrentArenaSeason(lua_State* L) {
    lua_pushnumber(L, GetWorldState(3191));
    return 1;
}

// VERIFY: aaf2ec2e auto
// OFFSET: 0x005A2A70
static int32_t Script_GetPreviousArenaSeason(lua_State* L) {
    lua_pushnumber(L, GetWorldState(3901));
    return 1;
}

// VERIFY: e6e83a08 manual
// OFFSET: 0x005A2FC0
static int32_t Script_GetArenaTeamRosterInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetArenaTeamRosterInfo(team, index)");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    ArenaTeamMember* member;

    if (team >= 3 || index >= CGArenaTeamInfo::m_arenaTeams[team].m_numMembers || !(member = CGArenaTeamInfo::m_arenaTeams[team].m_members.m_data[index])) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 10;
    }

    lua_pushstring(L, member->m_name);
    lua_pushnumber(L, member->m_rank);
    lua_pushnumber(L, static_cast<int32_t>(member->m_level));
    ChrClassesRec* chrClass = g_chrClassesDB.GetRecord(member->m_class);

    if (chrClass) {
        WOWGUID none = WOWGUID();
        const char* className = CGUnit_C::GetDisplayClassName(nullptr, g_nameDBCache.GetRecord(member->m_guid.m_guid, none, nullptr, nullptr, false));

        if (className) {
            lua_pushstring(L, className);
        } else {
            lua_pushstring(L, chrClass->m_name);
        }
    } else {
        lua_pushnil(L);
    }

    if (member->m_online) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, member->m_weekGames);
    lua_pushnumber(L, member->m_weekWins);
    lua_pushnumber(L, member->m_seasonGames);
    lua_pushnumber(L, member->m_seasonWins);
    lua_pushnumber(L, member->m_personalRating);
    return 10;
}

// VERIFY: 93646446 manual
// OFFSET: 0x005A3370
static int32_t Script_SetArenaTeamRosterSelection(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetArenaTeamRosterSelection(team, index)");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    CGArenaTeamInfo::SelectMember(team, index);
    return 0;
}

// VERIFY: 35145502 auto
// OFFSET: 0x005A3410
static int32_t Script_GetArenaTeamRosterSelection(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetArenaTeamRosterSelection(team)");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    lua_pushnumber(L, CGArenaTeamInfo::GetSelectedMemberIndex(team) + 1);
    return 1;
}

// VERIFY: 2ae54b7c manual
// OFFSET: 0x005A3490
static int32_t Script_SortArenaTeamRoster(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usgae: SortArenaTeamRoster(\"type\")");
        return 0;
    }

    int32_t column = 0;
    const char* type = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(type, "name", STORM_MAX_STR)) {
        CGArenaTeamInfo::SortRoster(0);
        return 0;
    }

    if (!SStrCmpI(type, "class", STORM_MAX_STR)) {
        CGArenaTeamInfo::SortRoster(1);
        return 0;
    }

    if (!SStrCmpI(type, "played", STORM_MAX_STR)) {
        CGArenaTeamInfo::SortRoster(2);
        return 0;
    }

    if (!SStrCmpI(type, "won", STORM_MAX_STR)) {
        CGArenaTeamInfo::SortRoster(3);
        return 0;
    }

    if (!SStrCmpI(type, "seasonplayed", STORM_MAX_STR)) {
        CGArenaTeamInfo::SortRoster(4);
        return 0;
    }

    if (!SStrCmpI(type, "seasonwon", STORM_MAX_STR)) {
        CGArenaTeamInfo::SortRoster(5);
        return 0;
    }

    if (!SStrCmpI(type, "rating", STORM_MAX_STR)) {
        column = 6;
    }

    CGArenaTeamInfo::SortRoster(column);
    return 0;
}

// VERIFY: c5ef8996 auto
// OFFSET: 0x005A35E0
static int32_t Script_SetArenaTeamRosterShowOffline(lua_State* L) {
    CGArenaTeamInfo::SetRosterShowOffline(StringToBOOL(L, 1, 1));
    return 0;
}

// VERIFY: 5053c75c auto
// OFFSET: 0x008E5250
static int32_t Script_CloseArenaTeamRoster(lua_State* L) {
    return 0;
}

// VERIFY: 1e8f2585 auto
// OFFSET: 0x005A3CF0
static int32_t Script_ArenaTeamRoster(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ArenaTeamRoster(team)");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGArenaTeamInfo::QueryRoster(team);
    return 0;
}

// STATIC: 0x00ACF838
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetArenaTeam", &Script_GetArenaTeam },
    { "GetNumArenaTeamMembers", &Script_GetNumArenaTeamMembers },
    { "GetArenaTeamRosterInfo", &Script_GetArenaTeamRosterInfo },
    { "GetArenaTeamGdfInfo", &Script_GetArenaTeamGdfInfo },
    { "SetArenaTeamRosterSelection", &Script_SetArenaTeamRosterSelection },
    { "GetArenaTeamRosterSelection", &Script_GetArenaTeamRosterSelection },
    { "SortArenaTeamRoster", &Script_SortArenaTeamRoster },
    { "SetArenaTeamRosterShowOffline", &Script_SetArenaTeamRosterShowOffline },
    { "GetArenaTeamRosterShowOffline", &Script_GetArenaTeamRosterShowOffline },
    { "CloseArenaTeamRoster", &Script_CloseArenaTeamRoster },
    { "ArenaTeamRoster", &Script_ArenaTeamRoster },
    { "GetCurrentArenaSeason", &Script_GetCurrentArenaSeason },
    { "GetPreviousArenaSeason", &Script_GetPreviousArenaSeason },
};

// VERIFY: 2e34f30b auto
// OFFSET: 0x005A2AA0
void ArenaTeamInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 066ee713 auto
// OFFSET: 0x005A2AD0
void ArenaTeamInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
