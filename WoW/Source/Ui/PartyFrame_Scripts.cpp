#include "PartyFrame.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Os/Time.h"
#include <ctime>

// VERIFY: 4f5603dd manual
// OFFSET: 0x0052C110
static int32_t Script_GetNumPartyMembers(lua_State* L) {
    lua_pushnumber(L, CGPartyInfo::GetNumPartyMembers());
    return 1;
}

// VERIFY: b0e384e0 auto
// OFFSET: 0x0052C350
static int32_t Script_HasLFGRestrictions(lua_State* L) {
    lua_pushboolean(L, CGPartyInfo::m_lfgRestricted);
    return 1;
}

// VERIFY: 2405f986 auto
// OFFSET: 0x0052C270
static int32_t Script_GetPartyLeaderIndex(lua_State* L) {
    lua_pushnumber(L, CGPartyInfo::m_leaderIndex + 1);
    return 1;
}

// VERIFY: 29e1d6ef auto
// OFFSET: 0x0052C2A0
static int32_t Script_GetLootThreshold(lua_State* L) {
    lua_pushnumber(L, CGPartyInfo::m_lootThreshold);
    return 1;
}

// VERIFY: 49815710 manual
// OFFSET: 0x0052D990
static int32_t Script_LeaveParty(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::LeaveGroup();
    }

    return 0;
}

// VERIFY: 76589b81 manual
// OFFSET: 0x0052C190
static int32_t Script_GetRealNumPartyMembers(lua_State* L) {
    lua_pushnumber(L, CGPartyInfo::m_realNumMembers);
    return 1;
}

// VERIFY: a68ec378 auto
// OFFSET: 0x0052C2D0
static int32_t Script_GetOptOutOfLoot(lua_State* L) {
    if (CGPartyInfo::m_optOutOfLoot) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 34fc62cc auto
// OFFSET: 0x0052C310
static int32_t Script_IsPartyLFG(lua_State* L) {
    if (CGPartyInfo::m_lfgSlot) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5fab219c auto
// OFFSET: 0x0052CCD0
static int32_t Script_IsPartyLeader(lua_State* L) {
    if (CGPartyInfo::m_leader && CGPartyInfo::m_leader == ClntObjMgrGetActivePlayer()) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3dcc0814 auto
// OFFSET: 0x0052CD30
static int32_t Script_IsRealPartyLeader(lua_State* L) {
    if (CGPartyInfo::m_realLeader && CGPartyInfo::m_realLeader == ClntObjMgrGetActivePlayer()) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: de2d49bb auto
// OFFSET: 0x0052C1D0
static int32_t Script_GetPartyMember(lua_State* L) {
    uint32_t index;

    if (!lua_isnumber(L, 1) || (index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1) >= 4) {
        luaL_error(L, "Usage: GetPartyMember(1-4)");
        return 0;
    }

    if (CGPartyInfo::m_members[index]) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 28997e8a manual
// OFFSET: 0x0052DE60
static int32_t Script_SetLootThreshold(lua_State* L) {
    if (!CGPartyInfo::m_members[0] && !CGRaidInfo::m_numMembers) {
        CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
        return 0;
    }

    if (CGPartyInfo::m_leader != ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_NOT_LEADER);
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetLooThreshold(threshold)");
        return 0;
    }

    uint32_t threshold = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (threshold - 2 > 4) {
        luaL_error(L, "SetLootThreshold(): threshold must be between %d and %d", 2, 6);
        return 0;
    }

    if (ClntObjMgrGetActivePlayerObj()) {
        CGPlayer_C::SetLootMethod(CGPartyInfo::m_lootMethod, CGPartyInfo::m_lootMaster, threshold);
    }

    return 0;
}

// VERIFY: 0b0b0638 manual
// OFFSET: 0x0052CD90
static int32_t Script_GetLootMethod(lua_State* L) {
    const char* method;

    switch (CGPartyInfo::m_lootMethod) {
        case LOOT_METHOD_FREEFORALL:
            method = "freeforall";
            break;
        case LOOT_METHOD_ROUNDROBIN:
            method = "roundrobin";
            break;
        case LOOT_METHOD_MASTERLOOTER:
            method = "master";
            break;
        case LOOT_METHOD_GROUP:
            method = "group";
            break;
        case LOOT_METHOD_NEEDBEFOREGREED:
            method = "needbeforegreed";
            break;
        default:
            method = "ERROR!";
            break;
    }

    lua_pushstring(L, method);
    int32_t partyIndex = -1;
    int32_t raidIndex = -1;
    WOWGUID master = CGPartyInfo::m_lootMaster;

    if (master) {
        if (master == ClntObjMgrGetActivePlayer()) {
            partyIndex = 0;
        } else {
            for (int32_t i = 0; i < 4; i++) {
                if (master == CGPartyInfo::m_members[i]) {
                    partyIndex = i + 1;
                    break;
                }
            }
        }

        for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
            CGRaidMember* member = CGRaidInfo::m_members[i];

            if (member && member->m_guid == master) {
                raidIndex = i + 1;
                break;
            }
        }
    }

    if (partyIndex >= 0) {
        lua_pushnumber(L, partyIndex);
    } else {
        lua_pushnil(L);
    }

    if (raidIndex < 0) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, raidIndex);
    }

    return 3;
}

// VERIFY: 1eec9288 auto
// OFFSET: 0x0052CF00
static int32_t Script_SetOptOutOfLoot(lua_State* L) {
    int32_t optOut = lua_toboolean(L, 1);
    CGPartyInfo::SetOptOutOfLoot(optOut);
    CGChat::AddChatMessage(FrameScript_GetText(optOut ? "OPT_OUT_LOOT_TOGGLE_ON" : "OPT_OUT_LOOT_TOGGLE_OFF", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 0;
}

// VERIFY: 7839f724 auto
// OFFSET: 0x0052CF60
static int32_t Script_GetPartyAssignment(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetPartyAssignment(\"assignment\" [,\"raidmember\"] [,exactMatch])");
        return 0;
    }

    const char* assignment = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(assignment, "MAINTANK", STORM_MAX_STR)) {
        return CGPartyInfo::GetPartyAssignments(L, 0);
    }

    if (SStrCmpI(assignment, "MAINASSIST", STORM_MAX_STR)) {
        luaL_error(L, "GetPartyAssignment(): Invalid party assignment");
        return 0;
    }

    return CGPartyInfo::GetPartyAssignments(L, 1);
}

// VERIFY: 11c07222 manual
// OFFSET: 0x0052D000
static int32_t Script_CanChangePlayerDifficulty(lua_State* L) {
    lua_pushboolean(L, CGGameUI::CanChangePlayerDifficulty());
    bool cooldown = CGPartyInfo::m_nextDifficultyChange && static_cast<int32_t>(static_cast<uint32_t>(::time(nullptr)) - CGPartyInfo::m_nextDifficultyChange) < 0;
    lua_pushboolean(L, !cooldown);
    return 2;
}

// VERIFY: bce11cea manual
// OFFSET: 0x0052D9C0
static int32_t Script_SilenceMember(lua_State* L) {
    if ((!CGRaidInfo::m_numMembers && !CGPartyInfo::m_members[0]) || !lua_isstring(L, 1)) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, (CGRaidInfo::m_numMembers != 0) + 5, 0, 3.4028235e38f, 0);

    if (!guid) {
        CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (CGRaidInfo::m_numMembers) {
        WOWGUID playerGuid = player->m_obj->m_guid;

        if (CGRaidInfo::IsPlayerRaidOfficer(playerGuid)) {
            CGPlayer_C::SilencePlayer(guid, CGRaidInfo::IsPVPRaid());
            return 0;
        }
    } else if (CGPartyInfo::m_leader == player->m_obj->m_guid) {
        CGPlayer_C::SilencePlayer(guid, CGRaidInfo::IsPVPRaid());
    }

    return 0;
}

// VERIFY: 7f79bf87 manual
// OFFSET: 0x0052DAF0
static int32_t Script_UnSilenceMember(lua_State* L) {
    if ((!CGRaidInfo::m_numMembers && !CGPartyInfo::m_members[0]) || !lua_isstring(L, 1)) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, (CGRaidInfo::m_numMembers != 0) + 5, 0, 3.4028235e38f, 0);

    if (!guid) {
        CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (CGRaidInfo::m_numMembers) {
        WOWGUID playerGuid = player->m_obj->m_guid;

        if (CGRaidInfo::IsPlayerRaidOfficer(playerGuid)) {
            CGPlayer_C::UnSilencePlayer(guid, CGRaidInfo::IsPVPRaid());
            return 0;
        }
    } else if (CGPartyInfo::m_leader == player->m_obj->m_guid) {
        CGPlayer_C::UnSilencePlayer(guid, CGRaidInfo::IsPVPRaid());
    }

    return 0;
}

// VERIFY: 82549b17 manual
// OFFSET: 0x0052DC20
static int32_t Script_SetLootMethod(lua_State* L) {
    if (!CGPartyInfo::m_members[0] && !CGRaidInfo::m_numMembers) {
        CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
        return 0;
    }

    if (CGPartyInfo::m_leader != ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_NOT_LEADER);
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetLootMethod(\"method\" [,master])");
        return 0;
    }

    const char* method = lua_tolstring(L, 1, nullptr);
    LOOT_METHOD lootMethod;

    if (!SStrCmpI(method, "freeforall", STORM_MAX_STR)) {
        lootMethod = LOOT_METHOD_FREEFORALL;
    } else if (!SStrCmpI(method, "roundrobin", STORM_MAX_STR)) {
        lootMethod = LOOT_METHOD_ROUNDROBIN;
    } else if (!SStrCmpI(method, "master", STORM_MAX_STR)) {
        lootMethod = LOOT_METHOD_MASTERLOOTER;
    } else if (!SStrCmpI(method, "group", STORM_MAX_STR)) {
        lootMethod = LOOT_METHOD_GROUP;
    } else {
        if (SStrCmpI(method, "needbeforegreed", STORM_MAX_STR)) {
            luaL_error(L, "Invalid loot method");
            return 0;
        }

        lootMethod = LOOT_METHOD_NEEDBEFOREGREED;
    }

    WOWGUID master = WOWGUID();

    if (lootMethod == LOOT_METHOD_MASTERLOOTER) {
        const char* name;

        if (!lua_isstring(L, 2) || !(name = lua_tolstring(L, 2, nullptr)) || !*name) {
            CGGameUI::DisplayError(ERR_SPECIFY_MASTER_LOOTER);
            return 0;
        }

        if (CGRaidInfo::m_numMembers) {
            master = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, 6, StringToBOOL(L, 3, 0), 3.4028235e38f, 0);
        } else {
            master = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, 5, StringToBOOL(L, 3, 0), 3.4028235e38f, 0);
        }

        if (!master || (!CGPartyInfo::IsMember(master) && !IsGuidInMyRaid(master))) {
            CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
            return 0;
        }
    }

    if (ClntObjMgrGetActivePlayerObj()) {
        CGPlayer_C::SetLootMethod(lootMethod, master, CGPartyInfo::m_lootThreshold);
    }

    return 0;
}

// VERIFY: dcbd648d manual
// OFFSET: 0x0052DF40
static int32_t SetOrClearAssignment(lua_State* L, bool set) {
    if (set && !CGGameUI::CanPerformAction(4)) {
        return 0;
    }

    if (!CGPartyInfo::m_members[0] && !CGRaidInfo::m_numMembers) {
        CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
        return 0;
    }

    if (CGPartyInfo::m_leader != ClntObjMgrGetActivePlayer()) {
        WOWGUID player = ClntObjMgrGetActivePlayer();

        if (!CGRaidInfo::IsPlayerRaidOfficer(player)) {
            CGGameUI::DisplayError(ERR_NOT_LEADER);
            return 0;
        }
    }

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: %s(\"assignment\" [,\"raidmember\"] [,exactMatch])", set ? "SetPartyAssignment" : "ClearPartyAssignment");
        return 0;
    }

    WOWGUID guid = WOWGUID();
    const char* assignmentName = lua_tolstring(L, 1, nullptr);
    uint8_t assignment;

    if (!SStrCmpI(assignmentName, "MAINTANK", STORM_MAX_STR)) {
        assignment = 0;
    } else {
        if (SStrCmpI(assignmentName, "MAINASSIST", STORM_MAX_STR)) {
            luaL_error(L, "Invalid Party assignment");
            return 0;
        }

        assignment = 1;
    }

    if (!CanSendPartyCommand()) {
        return 0;
    }

    if (lua_isstring(L, 2)) {
        const char* name = lua_tolstring(L, 2, nullptr);
        guid = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, 6, StringToBOOL(L, 3, 0), 3.4028235e38f, 0);

        if (!guid) {
            WOWGUID unit = CGGameUI::ClosestObjectMatch(name, TYPE_UNIT, 0, StringToBOOL(L, 3, 0), 3.4028235e38f, 0);

            if (ClntObjMgrObjectPtr(unit, TYPE_UNIT, __FILE__, __LINE__)) {
                CGGameUI::DisplayError(ERR_MEETING_STONE_TARGET_NOT_IN_PARTY);
            } else if (name && *name && SStrCmpI(name, "target", STORM_MAX_STR)) {
                CGGameUI::DisplayError(ERR_UNIT_NOT_FOUND);
            } else {
                CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
            }

            return 0;
        }
    }

    if (ClntObjMgrGetActivePlayerObj()) {
        CGPartyInfo::m_lastAssignmentTime = OsGetAsyncTimeMs();

        if (set) {
            CGPlayer_C::SetPartyAssignment(assignment, guid);
            return 0;
        }

        CGPlayer_C::ClearPartyAssignment(assignment, guid);
    }

    return 0;
}

// VERIFY: af1ffdeb manual
// OFFSET: 0x0052E1B0
static int32_t Script_SetPartyAssignment(lua_State* L) {
    return SetOrClearAssignment(L, true);
}

// VERIFY: dab7ea29 auto
// OFFSET: 0x0052E400
static int32_t Script_ClearPartyAssignment(lua_State* L) {
    return SetOrClearAssignment(L, false);
}

// VERIFY: c58ebc08 auto
// OFFSET: 0x0052E420
static int32_t Script_ChangePlayerDifficulty(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetPlayerDifficulty(\"difficulty\")");
        return 0;
    }

    if (CGGameUI::CanChangePlayerDifficulty()) {
        double difficulty = lua_tonumber(L, 1);

        if (difficulty <= 1.0) {
            CGPartyInfo::ChangeRaidDifficulty(static_cast<int32_t>(difficulty));
        }
    }

    return 0;
}

// STATIC: 0x00ACC6D0
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumPartyMembers", &Script_GetNumPartyMembers },
    { "GetRealNumPartyMembers", &Script_GetRealNumPartyMembers },
    { "GetPartyMember", &Script_GetPartyMember },
    { "GetPartyLeaderIndex", &Script_GetPartyLeaderIndex },
    { "IsPartyLeader", &Script_IsPartyLeader },
    { "IsRealPartyLeader", &Script_IsRealPartyLeader },
    { "LeaveParty", &Script_LeaveParty },
    { "GetLootMethod", &Script_GetLootMethod },
    { "SetLootMethod", &Script_SetLootMethod },
    { "GetLootThreshold", &Script_GetLootThreshold },
    { "SetLootThreshold", &Script_SetLootThreshold },
    { "SetPartyAssignment", &Script_SetPartyAssignment },
    { "ClearPartyAssignment", &Script_ClearPartyAssignment },
    { "GetPartyAssignment", &Script_GetPartyAssignment },
    { "SilenceMember", &Script_SilenceMember },
    { "UnSilenceMember", &Script_UnSilenceMember },
    { "SetOptOutOfLoot", &Script_SetOptOutOfLoot },
    { "GetOptOutOfLoot", &Script_GetOptOutOfLoot },
    { "CanChangePlayerDifficulty", &Script_CanChangePlayerDifficulty },
    { "ChangePlayerDifficulty", &Script_ChangePlayerDifficulty },
    { "IsPartyLFG", &Script_IsPartyLFG },
    { "HasLFGRestrictions", &Script_HasLFGRestrictions },
};

// VERIFY: bdeec586 auto
// OFFSET: 0x0052C370
void CGPartyInfo::RegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 4dbd4e0a auto
// OFFSET: 0x0052C3A0
void CGPartyInfo::UnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
