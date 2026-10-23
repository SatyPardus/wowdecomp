#include "GossipInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/QuestLog.h"

// STATIC: 0x00ACEF20
static const char* s_gossipTypes[22] = {
    "gossip",
    "vendor",
    "taxi",
    "trainer",
    "healer",
    "binder",
    "banker",
    "petition",
    "tabard",
    "battlemaster",
    "auctioneer",
    "gossip",
    "gossip",
    "gossip",
    nullptr,
    nullptr,
    "gossip",
    "gossip",
    "gossip",
    "gossip",
    "gossip",
    nullptr,
};

// VERIFY: e97b7e00 manual
// OFFSET: 0x0058A920
static int32_t Script_GetNumGossipOptions(lua_State* L) {
    lua_pushnumber(L, CGGossipInfo::m_gossipOptionsCount);
    return 1;
}

// VERIFY: 2aa162f8 manual
// OFFSET: 0x0058AB50
static int32_t Script_ForceGossip(lua_State* L) {
    bool force = false;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGossipInfo::m_gossip, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        force = unit->IsForceGossip();
    }

    lua_pushboolean(L, force);
    return 1;
}

// VERIFY: bd0c95ec auto
// OFFSET: 0x0058A900
static int32_t Script_GetGossipText(lua_State* L) {
    lua_pushstring(L, CGGossipInfo::m_gossipText);
    return 1;
}

// VERIFY: 3429fd49 auto
// OFFSET: 0x0058A9E0
static int32_t Script_GetGossipOptions(lua_State* L) {
    int32_t count = 0;

    for (GossipOption* option = CGGossipInfo::m_options; option < reinterpret_cast<GossipOption*>(CGGossipInfo::m_gossipText); option++) {
        if (option && option->m_index >= 0) {
            lua_checkstack(L, 2);
            lua_pushstring(L, option->m_text);
            lua_pushstring(L, s_gossipTypes[option->m_type]);
            count++;
        }
    }

    return 2 * count;
}

// VERIFY: 2c77e3ed manual
// OFFSET: 0x0058A960
static int32_t Script_GetNumGossipAvailableQuests(lua_State* L) {
    lua_pushnumber(L, CGGossipInfo::GetNumAvailableQuests());
    return 1;
}

// VERIFY: 9166f232 manual
// OFFSET: 0x0058A9A0
static int32_t Script_GetNumGossipActiveQuests(lua_State* L) {
    lua_pushnumber(L, CGGossipInfo::GetNumActiveQuests());
    return 1;
}

// VERIFY: 9e58002d auto
// OFFSET: 0x0058AA40
static int32_t Script_CloseGossip(lua_State* L) {
    WOWGUID none = { 0 };
    CGGossipInfo::SetGossip(none);
    return 0;
}

// VERIFY: 4fc9b751 auto
// OFFSET: 0x0058B3A0
static int32_t Script_GetGossipAvailableQuests(lua_State* L) {
    uint32_t total = static_cast<uint32_t>(CGGossipInfo::GetNumAvailableQuests());
    int32_t count = 0;

    for (uint32_t i = 0; i < total; i++) {
        GossipQuestInfo* quest = CGGossipInfo::GetAvailableQuest(i);

        if (!quest) {
            continue;
        }

        lua_checkstack(L, 5);
        lua_pushstring(L, quest->m_title);
        lua_pushnumber(L, quest->m_level);

        if (CGQuestInfo::IsLowLevel(quest->m_level)) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        if (quest->m_flags & 0x1000) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        if (quest->m_repeatable) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        count++;
    }

    return 5 * count;
}

// VERIFY: 79e4bd10 manual
// OFFSET: 0x0058B490
static int32_t Script_GetGossipActiveQuests(lua_State* L) {
    uint32_t total = static_cast<uint32_t>(CGGossipInfo::GetNumActiveQuests());
    int32_t count = 0;

    for (uint32_t i = 0; i < total; i++) {
        GossipQuestInfo* quest = CGGossipInfo::GetActiveQuest(i);

        if (!quest) {
            continue;
        }

        lua_checkstack(L, 4);
        lua_pushstring(L, quest->m_title);
        lua_pushnumber(L, quest->m_level);

        if (CGQuestInfo::IsLowLevel(quest->m_level)) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        int32_t index = CGQuestLog::GetQuestIndex(quest->m_questID);

        if (index > 0 && (CGQuestLog::IsQuestCompleteEx(index - 1, 1) || CGQuestLog::HasNoObjectives(index - 1))) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        count++;
    }

    return 4 * count;
}

// VERIFY: e59d4d4e manual
// OFFSET: 0x0058B590
static int32_t Script_SelectGossipOption(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: SelectGossipOption(index)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t confirmed = StringToBOOL(L, 3, 0);
    CGGossipInfo::SelectGossipOption(index, lua_tolstring(L, 2, nullptr), confirmed);
    return 0;
}

// VERIFY: 89ac0b6a manual
// OFFSET: 0x0058B600
static int32_t Script_SelectGossipAvailableQuest(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: SelectGossipAvailableQuest(index)");
    }

    CGGossipInfo::SelectGossipAvailableQuest(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: a699660e manual
// OFFSET: 0x0058B670
static int32_t Script_SelectGossipActiveQuest(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: SelectGossipActiveQuest(index)");
    }

    CGGossipInfo::SelectGossipActiveQuest(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// STATIC: 0x00ACEF78
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetGossipText", &Script_GetGossipText },
    { "GetNumGossipOptions", &Script_GetNumGossipOptions },
    { "GetNumGossipAvailableQuests", &Script_GetNumGossipAvailableQuests },
    { "GetNumGossipActiveQuests", &Script_GetNumGossipActiveQuests },
    { "GetGossipOptions", &Script_GetGossipOptions },
    { "GetGossipAvailableQuests", &Script_GetGossipAvailableQuests },
    { "GetGossipActiveQuests", &Script_GetGossipActiveQuests },
    { "SelectGossipOption", &Script_SelectGossipOption },
    { "SelectGossipAvailableQuest", &Script_SelectGossipAvailableQuest },
    { "SelectGossipActiveQuest", &Script_SelectGossipActiveQuest },
    { "CloseGossip", &Script_CloseGossip },
    { "ForceGossip", &Script_ForceGossip },
};

// VERIFY: 872f7321 auto
// OFFSET: 0x0058AA60
void GossipInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: f7d24819 auto
// OFFSET: 0x0058AA90
void GossipInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
