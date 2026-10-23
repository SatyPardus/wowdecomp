#include "ReputationInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/FactionRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// VERIFY: 1e40863f auto
// OFFSET: 0x005D1FC0
static int32_t Script_CollapseAllFactionHeaders(lua_State* L) {
    CGReputationInfo::CollapseHeader(CGReputationInfo::m_numFactions, 1);
    return 0;
}

// VERIFY: 6b33faf0 auto
// OFFSET: 0x005D1FE0
static int32_t Script_ExpandAllFactionHeaders(lua_State* L) {
    CGReputationInfo::CollapseHeader(CGReputationInfo::m_numFactions, 0);
    return 0;
}

// VERIFY: 26793d47 manual
// OFFSET: 0x005CFF20
static int32_t Script_GetNumFactions(lua_State* L) {
    lua_pushnumber(L, CGReputationInfo::m_numShownFactions);
    return 1;
}

// VERIFY: 6deeea03 auto
// OFFSET: 0x005D1F70
static int32_t Script_CollapseFactionHeader(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CollapseFactionHeader(index)");
        return 0;
    }

    CGReputationInfo::CollapseHeader(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1, 1);
    return 0;
}

// VERIFY: ceb13be5 auto
// OFFSET: 0x005D2000
static int32_t Script_ExpandFactionHeader(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ExpandFactionHeader(index)");
        return 0;
    }

    CGReputationInfo::CollapseHeader(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1, 0);
    return 0;
}

// VERIFY: 7c94cd2f auto
// OFFSET: 0x005D0740
static int32_t Script_IsFactionInactive(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsFactionInactive(index)");
        return 0;
    }

    if (CGReputationInfo::IsFactionInactive(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7d15ad95 auto
// OFFSET: 0x005D07B0
static int32_t Script_SetSelectedFaction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetSelectedFaction(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index <= CGReputationInfo::m_numFactions) {
        CGReputationInfo::m_selectedFaction = CGReputationInfo::m_factionList.m_data[index]->m_factionId;
    } else {
        CGReputationInfo::m_selectedFaction = 0;
    }

    return 0;
}

// VERIFY: 08160303 auto
// OFFSET: 0x005D0820
static int32_t Script_GetSelectedFaction(lua_State* L) {
    lua_pushnumber(L, CGReputationInfo::GetSelectedFactionIndex() + 1);
    return 1;
}

// VERIFY: d98fde42 manual
// OFFSET: 0x005D1150
static int32_t Script_GetFactionInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetFactionInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t faction = index <= CGReputationInfo::m_numFactions ? CGReputationInfo::m_factionList.m_data[index]->m_factionId : 0;
    return GetFactionInfoHelper(L, faction);
}

// VERIFY: 5a6cb739 manual
// OFFSET: 0x005D11E0
static int32_t Script_GetFactionInfoByID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetFactionInfoByID(id)");
        return 0;
    }

    return GetFactionInfoHelper(L, static_cast<int32_t>(lua_tonumber(L, 1)));
}

// VERIFY: 941cdb0b manual
// OFFSET: 0x005D1240
static int32_t Script_GetWatchedFactionInfo(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    FactionRec* rec = nullptr;
    int32_t faction = 0;

    if (player) {
        uint32_t index = player->m_player->m_playerFieldWatchedFactionIndex;
        faction = index > 127 ? 0 : CGReputationInfo::m_factionStandings[index].m_factionId;
        rec = g_factionDB.GetRecord(faction);
    }

    if (!rec) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 5;
    }

    lua_pushstring(L, rec->m_name);
    UNIT_REACTION reaction = CGReputationInfo::GetFactionStandingReaction(faction);
    lua_pushnumber(L, reaction + 1);
    int32_t max = CGReputationInfo::m_reactionThresholds[reaction + 1];
    int32_t value = GetRepListRepValue(faction);
    lua_pushnumber(L, CGReputationInfo::m_reactionThresholds[reaction]);
    lua_pushnumber(L, max);
    lua_pushnumber(L, value);
    return 5;
}

// VERIFY: 115d0d85 auto
// OFFSET: 0x005D1390
static int32_t Script_FactionToggleAtWar(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: FactionToggleAtWar(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index <= CGReputationInfo::m_numFactions && CGReputationInfo::m_factionList.m_data[index]->m_factionId) {
        int32_t faction = CGReputationInfo::m_factionList.m_data[index]->m_factionId;
        CGReputationInfo::SetAtWar(faction, !CGReputationInfo::IsAtWar(faction));
    }

    return 0;
}

// VERIFY: ca638192 auto
// OFFSET: 0x005D1420
static int32_t Script_SetWatchedFactionIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetWatchedFactionIndex(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index <= CGReputationInfo::m_numFactions) {
        CGReputationInfo::SetWatchedFaction(CGReputationInfo::m_factionList.m_data[index]->m_factionId);
    } else {
        CGReputationInfo::SetWatchedFaction(0);
    }

    return 0;
}

// VERIFY: 4581b042 auto
// OFFSET: 0x005D1E50
static int32_t Script_SetFactionInactive(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetFactionInactive(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index <= CGReputationInfo::m_numFactions) {
        int32_t faction = CGReputationInfo::m_factionList.m_data[index]->m_factionId;

        if (faction) {
            CGReputationInfo::SetFactionInactive(faction, true);
        }
    }

    return 0;
}

// VERIFY: 5b890263 auto
// OFFSET: 0x005D1EE0
static int32_t Script_SetFactionActive(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetFactionActive(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index <= CGReputationInfo::m_numFactions) {
        int32_t faction = CGReputationInfo::m_factionList.m_data[index]->m_factionId;

        if (faction) {
            CGReputationInfo::SetFactionInactive(faction, false);
        }
    }

    return 0;
}

// STATIC: 0x00AD0AE8
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumFactions", &Script_GetNumFactions },
    { "GetFactionInfo", &Script_GetFactionInfo },
    { "GetFactionInfoByID", &Script_GetFactionInfoByID },
    { "GetWatchedFactionInfo", &Script_GetWatchedFactionInfo },
    { "SetWatchedFactionIndex", &Script_SetWatchedFactionIndex },
    { "FactionToggleAtWar", &Script_FactionToggleAtWar },
    { "CollapseFactionHeader", &Script_CollapseFactionHeader },
    { "CollapseAllFactionHeaders", &Script_CollapseAllFactionHeaders },
    { "SetFactionInactive", &Script_SetFactionInactive },
    { "SetFactionActive", &Script_SetFactionActive },
    { "IsFactionInactive", &Script_IsFactionInactive },
    { "ExpandFactionHeader", &Script_ExpandFactionHeader },
    { "ExpandAllFactionHeaders", &Script_ExpandAllFactionHeaders },
    { "SetSelectedFaction", &Script_SetSelectedFaction },
    { "GetSelectedFaction", &Script_GetSelectedFaction },
};

// VERIFY: 5ad3f6ee auto
// OFFSET: 0x005CFF60
void ReputationInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 86abcaf1 auto
// OFFSET: 0x005CFF90
void ReputationInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
