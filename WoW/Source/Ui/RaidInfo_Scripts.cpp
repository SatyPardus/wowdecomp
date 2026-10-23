#include "RaidInfo.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Unit.h"
#include "Engine/Source/FrameScript/Types.h"

// VERIFY: 3ff51581 manual
// OFFSET: 0x00572B40
static int32_t Script_GetNumRaidMembers(lua_State* L) {
    lua_pushnumber(L, CGRaidInfo::m_numMembers);
    return 1;
}

// VERIFY: fcb45632 manual
// OFFSET: 0x00572B80
static int32_t Script_GetRealNumRaidMembers(lua_State* L) {
    lua_pushnumber(L, CGRaidInfo::m_realNumMembers);
    return 1;
}

// VERIFY: 8d0b38de auto
// OFFSET: 0x00573A60
static int32_t Script_IsRaidLeader(lua_State* L) {
    if (ClntObjMgrGetActivePlayer() == CGPartyInfo::m_leader) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: ad4569ee auto
// OFFSET: 0x00573AB0
static int32_t Script_IsRealRaidLeader(lua_State* L) {
    if (ClntObjMgrGetActivePlayer() == CGPartyInfo::m_realLeader) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 3a66ce1f auto
// OFFSET: 0x00572BC0
static int32_t Script_SetRaidRosterSelection(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetRaidRosterSelection(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGRaidInfo::m_numMembers) {
        CGRaidInfo::m_selectedMember = WOWGUID();
        return 0;
    }

    CGRaidInfo::m_selectedMember = CGRaidInfo::m_members[index]->m_guid;
    return 0;
}

// VERIFY: 452e857c manual
// OFFSET: 0x00572C80
static int32_t Script_GetReadyCheckTimeLeft(lua_State* L) {
    if (CGRaidInfo::m_numMembers || CGPartyInfo::m_members[0]) {
        uint32_t now = OsGetAsyncTimeMs();
        uint32_t remaining;

        if (CGRaidInfo::m_readyCheckTime && static_cast<int32_t>(now - CGRaidInfo::m_readyCheckTime) < 0) {
            remaining = CGRaidInfo::m_readyCheckTime - now;
        } else {
            remaining = 0;
        }

        lua_pushnumber(L, remaining / 1000);
        return 1;
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: c4260696 manual
// OFFSET: 0x00574A00
static int32_t Script_ConvertToRaid(lua_State* L) {
    if (CGPartyInfo::m_members[0] && CGPartyInfo::m_leader == ClntObjMgrGetActivePlayer()) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && static_cast<int32_t>(player->m_unit->m_unitFieldLevel) >= 10) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_GROUP_RAID_CONVERT));
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    return 0;
}

// VERIFY: 2efa072d manual
// OFFSET: 0x005740C0
static int32_t Script_ConfirmReadyCheck(lua_State* L) {
    if (CGRaidInfo::m_numMembers || CGPartyInfo::m_members[0]) {
        uint32_t now = OsGetAsyncTimeMs();
        uint32_t remaining;

        if (CGRaidInfo::m_readyCheckTime && static_cast<int32_t>(now - CGRaidInfo::m_readyCheckTime) < 0) {
            remaining = CGRaidInfo::m_readyCheckTime - now;
        } else {
            remaining = 0;
        }

        if (remaining) {
            uint8_t ready = StringToBOOL(L, 1, 0);
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(MSG_RAID_READY_CHECK));
            msg.Put(ready);
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    return 0;
}

// VERIFY: a6457528 auto
// OFFSET: 0x00573B00
static int32_t Script_IsRaidOfficer(lua_State* L) {
    CGRaidMember* member = CGRaidInfo::FindMemberByGuid(ClntObjMgrGetActivePlayer());

    if (member && member->m_rank > 0) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9510fc76 manual
// OFFSET: 0x00574180
static int32_t Script_GetReadyCheckStatus(lua_State* L) {
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));
    const char* status = nullptr;

    if (!CGRaidInfo::m_numMembers) {
        if (!CGPartyInfo::m_members[0] || CGPartyInfo::m_leader != ClntObjMgrGetActivePlayer()) {
            lua_pushstring(L, status);
            return 1;
        }

        int32_t index = CGRaidInfo::GetPartyMemberIndex(guid);
        uint32_t now = OsGetAsyncTimeMs();

        if (!CGRaidInfo::m_readyCheckTime || static_cast<int32_t>(now - CGRaidInfo::m_readyCheckTime) >= 0 || CGRaidInfo::m_readyCheckTime == now) {
            lua_pushstring(L, status);
            return 1;
        }

        if (index == -1) {
            if (guid == CGRaidInfo::m_readyCheckInitiator) {
                status = "ready";
            }

            lua_pushstring(L, status);
            return 1;
        }

        if (CGRaidInfo::IsPartyMemberReadyCheckPending(index)) {
            lua_pushstring(L, "waiting");
            return 1;
        }

        if (CGRaidInfo::IsPartyMemberReadyCheckReady(index)) {
            lua_pushstring(L, "ready");
            return 1;
        }

        lua_pushstring(L, "notready");
        return 1;
    }

    CGRaidMember* self = CGRaidInfo::FindMemberByGuid(ClntObjMgrGetActivePlayer());

    if (!self || self->m_rank <= 0) {
        lua_pushstring(L, status);
        return 1;
    }

    CGRaidMember* member = CGRaidInfo::FindMemberByGuid(guid);
    uint32_t now = OsGetAsyncTimeMs();

    if (!CGRaidInfo::m_readyCheckTime || static_cast<int32_t>(now - CGRaidInfo::m_readyCheckTime) >= 0 || CGRaidInfo::m_readyCheckTime == now || !member) {
        lua_pushstring(L, status);
        return 1;
    }

    if (guid == CGRaidInfo::m_readyCheckInitiator) {
        lua_pushstring(L, "ready");
        return 1;
    }

    if (member->m_readyCheckPending) {
        lua_pushstring(L, "waiting");
        return 1;
    }

    if (member->m_readyCheckReady) {
        lua_pushstring(L, "ready");
        return 1;
    }

    lua_pushstring(L, "notready");
    return 1;
}

// VERIFY: 6d2243ec auto
// OFFSET: 0x00572AB0
static int32_t Script_GetRaidTargetIndex(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetRaidTargetIndex(unit)");
        return 0;
    }

    uint32_t index = GetRaidTargetIndexFromGuid(GetGuidByKeyword(lua_tolstring(L, 1, nullptr)));

    if (index == 8) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, index + 1);
    }

    return 1;
}

// VERIFY: 24f2f0b5 auto
// OFFSET: 0x00572C50
static int32_t Script_GetRaidRosterSelection(lua_State* L) {
    lua_pushnumber(L, CGRaidInfo::GetSelectedMemberIndex() + 1);
    return 1;
}

// VERIFY: 41c24d2f manual
// OFFSET: 0x00573690
static int32_t Script_GetRaidRosterInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetRaidRosterInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    CGRaidMember* member;
    NameCache* name;
    WOWGUID none = WOWGUID();

    if (index >= CGRaidInfo::m_numMembers || !(member = CGRaidInfo::m_members[index]) || !(name = g_nameDBCache.GetRecord(member->m_guid.m_guid, none, nullptr, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, 1.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 14;
    }

    char fullName[308];
    lua_pushstring(L, name->FullName(fullName, 305));
    lua_pushnumber(L, member->m_rank);
    lua_pushnumber(L, member->m_subgroup + 1);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(member->m_guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        lua_pushnumber(L, static_cast<int32_t>(unit->m_unit->m_unitFieldLevel));
    } else if (member->m_stats.m_flags & 1) {
        lua_pushnumber(L, member->m_stats.m_level);
    } else {
        lua_pushnumber(L, 0.0);
    }

    ChrClassesRec* chrClass = g_chrClassesDB.GetRecord(name->m_class);
    const char* className = CGUnit_C::GetDisplayClassName(nullptr, name);

    if (chrClass && className) {
        lua_pushstring(L, className);
        lua_pushstring(L, chrClass->m_filename);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    if (unit || (member->m_stats.m_flags & 1)) {
        int32_t zoneID = unit ? CGGameUI::m_areaID : member->m_stats.m_zoneID;
        AreaTableRec* area = g_areaTableDB.GetRecord(zoneID);

        if (area) {
            lua_pushstring(L, area->m_areaName);
        } else {
            lua_pushnil(L);
        }

        lua_pushnumber(L, 1.0);
    } else {
        lua_pushstring(L, FrameScript_GetText("PLAYER_OFFLINE", -1, GENDER_NOT_APPLICABLE));
        lua_pushnil(L);
    }

    bool dead;

    if (unit) {
        dead = unit->m_unit->m_unitFieldHealth <= 0;
    } else {
        dead = (member->m_stats.m_flags & 4) && (member->m_stats.m_flags & 1);
    }

    if (dead) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (member->m_assignments & 2) {
        lua_pushstring(L, "MAINTANK");
    } else if (member->m_assignments & 4) {
        lua_pushstring(L, "MAINASSIST");
    } else {
        lua_pushnil(L);
    }

    if (CGPartyInfo::m_lootMethod == LOOT_METHOD_MASTERLOOTER && CGPartyInfo::m_lootMaster == member->m_guid) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (member->m_groupRole & 2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (member->m_groupRole & 4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (member->m_groupRole & 8) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 14;
}

// VERIFY: 5dd0653b manual
// OFFSET: 0x00573B50
static int32_t Script_SetRaidSubgroup(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetRaidSubgroup(index, subgroup)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(11)) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t subgroup = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (subgroup >= 8) {
        luaL_error(L, "Usage: SetRaidSubgroup(index, subgroup)");
        return 0;
    }

    CGRaidMember* member = CGRaidInfo::GetMember(index);

    if (!member || member->m_subgroup == subgroup) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(member->m_guid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GROUP_CHANGE_SUB_GROUP));
    msg.PutString(name->m_name);
    msg.Put(static_cast<uint8_t>(subgroup));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: c1bdf078 manual
// OFFSET: 0x00573C90
static int32_t Script_SwapRaidSubgroup(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SwapRaidSubgroup(index1, index2)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(11)) {
        return 0;
    }

    uint32_t index1 = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index2 = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    CGRaidMember* member1 = index1 < CGRaidInfo::m_numMembers ? CGRaidInfo::m_members[index1] : nullptr;
    CGRaidMember* member2 = index2 < CGRaidInfo::m_numMembers ? CGRaidInfo::m_members[index2] : nullptr;

    if (!member1 || !member2 || member1->m_subgroup == member2->m_subgroup) {
        return 0;
    }

    WOWGUID none1 = WOWGUID();
    NameCache* name1 = g_nameDBCache.GetRecord(member1->m_guid.m_guid, none1, nullptr, nullptr, false);
    WOWGUID none2 = WOWGUID();
    NameCache* name2 = g_nameDBCache.GetRecord(member2->m_guid.m_guid, none2, nullptr, nullptr, false);

    if (name1 && name2) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GROUP_SWAP_SUB_GROUP));
        msg.PutString(name1->m_name);
        msg.PutString(name2->m_name);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 9cf2e897 manual
// OFFSET: 0x00573E10
static int32_t Script_PromoteToLeader(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: PromoteToLeader(name)");
        return 0;
    }

    int32_t exact = StringToBOOL(L, 2, 0);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(lua_tolstring(L, 1, nullptr), TYPE_PLAYER, 6, exact, 3.4028235e38f, 0);

    if (guid) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GROUP_SET_LEADER));
        msg.Put(guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 6fd6d06a manual
// OFFSET: 0x00573EF0
static int32_t Script_PromoteToAssistant(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: PromoteToAssistant(name)");
        return 0;
    }

    int32_t exact = StringToBOOL(L, 2, 0);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(lua_tolstring(L, 1, nullptr), TYPE_PLAYER, 6, exact, 3.4028235e38f, 0);

    if (guid) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_ASSISTANT_LEADER));
        msg.Put(guid.m_guid);
        msg.Put(static_cast<uint8_t>(1));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 0848eb14 manual
// OFFSET: 0x00573FD0
static int32_t Script_DemoteAssistant(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: DemoteAssistant(name)");
        return 0;
    }

    int32_t exact = StringToBOOL(L, 2, 0);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(lua_tolstring(L, 1, nullptr), TYPE_PLAYER, 6, exact, 3.4028235e38f, 0);

    if (guid) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_ASSISTANT_LEADER));
        msg.Put(guid.m_guid);
        msg.Put(static_cast<uint8_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 4c3de34f auto
// OFFSET: 0x005740B0
static int32_t Script_DoReadyCheck(lua_State* L) {
    CGRaidInfo::SendReadyCheck();
    return 0;
}

// VERIFY: bb0d26ff manual
// OFFSET: 0x00574AB0
static int32_t Script_SetRaidTarget(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetRaidTarget(unit, index)");
        return 0;
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (!guid) {
        return 0;
    }

    uint32_t index = static_cast<uint8_t>(lua_tonumber(L, 2)) - 1;

    if (index < 8) {
        if (guid && !CGRaidInfo::IsPlayerOrPetInRaid(guid) && !CGPartyInfo::IsPlayerOrPetInParty(guid)) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

            if (!unit) {
                CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
                return 0;
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player && unit != player && (unit->m_obj->m_type & TYPE_PLAYER) && !player->CanCooperate(unit)) {
                CGGameUI::DisplayError(ERR_INVALID_RAID_TARGET);
                return 0;
            }
        }
    } else {
        index = GetRaidTargetIndexFromGuid(guid);
        guid = WOWGUID();
    }

    if (index >= 8) {
        return 0;
    }

    if (CGRaidInfo::m_numMembers || CGPartyInfo::InParty()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_RAID_TARGET_UPDATE));
        msg.Put(static_cast<uint8_t>(index));
        msg.Put(guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
        return 0;
    }

    uint32_t current = GetRaidTargetIndexFromGuid(guid);

    if (current == index || current < 8) {
        WOWGUID none = WOWGUID();
        CGRaidInfo::SetRaidTarget(current, none);
    }

    if (current != index) {
        CGRaidInfo::SetRaidTarget(index, guid);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_RAID_TARGET_UPDATE, nullptr);
    return 0;
}

// STATIC: 0x00ACE790
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumRaidMembers", &Script_GetNumRaidMembers },
    { "GetRealNumRaidMembers", &Script_GetRealNumRaidMembers },
    { "GetRaidRosterInfo", &Script_GetRaidRosterInfo },
    { "SetRaidRosterSelection", &Script_SetRaidRosterSelection },
    { "GetRaidRosterSelection", &Script_GetRaidRosterSelection },
    { "IsRaidLeader", &Script_IsRaidLeader },
    { "IsRealRaidLeader", &Script_IsRealRaidLeader },
    { "IsRaidOfficer", &Script_IsRaidOfficer },
    { "SetRaidSubgroup", &Script_SetRaidSubgroup },
    { "SwapRaidSubgroup", &Script_SwapRaidSubgroup },
    { "ConvertToRaid", &Script_ConvertToRaid },
    { "PromoteToLeader", &Script_PromoteToLeader },
    { "PromoteToAssistant", &Script_PromoteToAssistant },
    { "DemoteAssistant", &Script_DemoteAssistant },
    { "SetRaidTarget", &Script_SetRaidTarget },
    { "GetRaidTargetIndex", &Script_GetRaidTargetIndex },
    { "DoReadyCheck", &Script_DoReadyCheck },
    { "ConfirmReadyCheck", &Script_ConfirmReadyCheck },
    { "GetReadyCheckTimeLeft", &Script_GetReadyCheckTimeLeft },
    { "GetReadyCheckStatus", &Script_GetReadyCheckStatus },
};

// VERIFY: 8b29d591 auto
// OFFSET: 0x00572D10
void RaidInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 04880dc6 auto
// OFFSET: 0x00572D40
void RaidInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
