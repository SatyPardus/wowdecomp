#include "LFGInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/DB/Rec/LfgDungeonsRec.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Os/Time.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/LfgDungeonGroupRec.h"
#include "WoW/Source/DB/Rec/LfgDungeonExpansionRec.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"

// STATIC: 0x00ACDD60
static int32_t s_playerLockRequests = 2;

// STATIC: 0x00ACDD64
static int32_t s_partyLockRequests = 2;

// STATIC: 0x00BEABB8
static float s_playerLockRequestTime;

// STATIC: 0x00BEABBC
static float s_partyLockRequestTime;

// STATIC: 0x00ACD644
static const char* s_lfgTypeTokens[7] = {
    "LFG_TYPE_NONE",
    "LFG_TYPE_DUNGEON",
    "LFG_TYPE_RAID",
    "LFG_TYPE_QUEST",
    "LFG_TYPE_ZONE",
    "LFG_TYPE_HEROIC_DUNGEON",
    "LFG_TYPE_RANDOM_DUNGEON",
};

// STATIC: 0x00BEACD0
static char s_rewardItemTexture[260];

// VERIFY: c406e473 auto
// OFFSET: 0x00552E10
static int32_t Script_GetLFGRoles(lua_State* L) {
    if (s_cvLfgSelectedRoles) {
        int32_t roles = s_cvLfgSelectedRoles->m_intValue;
        lua_pushboolean(L, roles & 1);
        lua_pushboolean(L, roles & 2);
        lua_pushboolean(L, roles & 4);
        lua_pushboolean(L, roles & 8);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: 8d22b555 manual
// OFFSET: 0x00558060
static int32_t Script_GetLFGRandomCooldownExpiration(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t index = player->GetAuraIndex(71328);

    if (index == -1) {
        return 0;
    }

    lua_pushnumber(L, player->GetAura(index)->m_endTime * 0.001);
    return 1;
}

// VERIFY: 56281bf7 manual
// OFFSET: 0x005580E0
static int32_t Script_GetLFGDeserterExpiration(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t index = player->GetAuraIndex(71041);

    if (index == -1) {
        return 0;
    }

    lua_pushnumber(L, player->GetAura(index)->m_endTime * 0.001);
    return 1;
}

// VERIFY: b4e04149 auto
// OFFSET: 0x005548F0
static int32_t Script_GetAvailableRoles(lua_State* L) {
    ChrClassesRec* rec = g_chrClassesDB.GetRecord(ClientServices::GetCharacterClass());
    uint8_t roles = CGLookingForGroup::GetClassRoles(rec->m_id);
    lua_pushboolean(L, roles & 0x2);
    lua_pushboolean(L, roles & 0x4);
    lua_pushboolean(L, roles & 0x8);
    return 3;
}

// VERIFY: c1cf44e1 auto
// OFFSET: 0x00553D60
static int32_t Script_SetLFGRoles(lua_State* L) {
    int32_t roles = StringToBOOL(L, 1, 0) != 0;

    if (StringToBOOL(L, 2, 0)) {
        roles |= 0x2;
    }

    if (StringToBOOL(L, 3, 0)) {
        roles |= 0x4;
    }

    if (StringToBOOL(L, 4, 0)) {
        roles |= 0x8;
    }

    ChrClassesRec* rec = g_chrClassesDB.GetRecord(ClientServices::GetCharacterClass());
    s_cvLfgSelectedRoles->Set(roles & CGLookingForGroup::GetClassRoles(rec->m_id), true, false, false, true);
    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_UPDATE, nullptr);
    return 0;
}

// VERIFY: 9e5c5761 auto
// OFFSET: 0x00552DD0
static int32_t Script_GetLFGTypes(lua_State* L) {
    lua_checkstack(L, 7);

    for (uint32_t i = 0; i < 7; i++) {
        lua_pushstring(L, FrameScript_GetText(s_lfgTypeTokens[i], -1, GENDER_NOT_APPLICABLE));
    }

    return 7;
}

// VERIFY: 364604e4 auto
// OFFSET: 0x00552950
static void RandomDungeonRewardItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = WOWGUID();

    if (g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE_RANDOM_INFO, nullptr);
    }
}

// VERIFY: cdaad151 manual
// OFFSET: 0x005529A0
static int32_t Script_SearchLFGGetJoinedID(lua_State* L) {
    if (!(CGLookingForGroup::m_searchJoinedID & 0xFFFFFF)) {
        return 0;
    }

    lua_pushnumber(L, CGLookingForGroup::m_searchJoinedID & 0xFFFFFF);
    return 1;
}

// VERIFY: 6853d33c auto
// OFFSET: 0x00553120
static int32_t Script_GetRandomDungeonBestChoice(lua_State* L) {
    if (CGLookingForGroup::m_randomDungeonBestChoice) {
        lua_pushnumber(L, CGLookingForGroup::m_randomDungeonBestChoice & 0xFFFFFF);
        return 1;
    }

    return 0;
}

// VERIFY: 98c59818 manual
// OFFSET: 0x00554E60
static void PushLFDChoiceInfo(lua_State* L, int32_t id) {
    if (id < 0) {
        LfgDungeonGroupRec* group = g_lfgDungeonGroupDB.GetRecord(-id);
        lua_pushnumber(L, 1.0);
        lua_pushstring(L, group->m_name);
        lua_settable(L, -3);
        lua_pushnumber(L, 2.0);
        lua_pushnumber(L, group->m_typeID);
        lua_settable(L, -3);
        return;
    }

    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(id);
    LfgDungeonExpansionRec* expansion = GetLfgDungeonExpansionRec(rec->m_id, ClientServices::Connection()->m_accountExpansion);
    MapDifficultyRec* difficulty = GetMapDifficultyEntryByMapIdAndDifficulty(rec->m_mapID, rec->m_difficulty, nullptr);
    lua_pushnumber(L, 1.0);
    lua_pushstring(L, rec->m_name);
    lua_settable(L, -3);
    lua_pushnumber(L, 2.0);
    lua_pushnumber(L, rec->m_typeID);
    lua_settable(L, -3);
    lua_pushnumber(L, 3.0);
    lua_pushnumber(L, expansion ? expansion->m_hardLevelMin : rec->m_minLevel);
    lua_settable(L, -3);
    lua_pushnumber(L, 4.0);
    lua_pushnumber(L, expansion ? expansion->m_hardLevelMax : rec->m_maxLevel);
    lua_settable(L, -3);
    lua_pushnumber(L, 5.0);
    lua_pushnumber(L, rec->m_targetLevel);
    lua_settable(L, -3);
    lua_pushnumber(L, 6.0);
    lua_pushnumber(L, expansion ? expansion->m_targetLevelMin : rec->m_targetLevelMin);
    lua_settable(L, -3);
    lua_pushnumber(L, 7.0);
    lua_pushnumber(L, expansion ? expansion->m_targetLevelMax : rec->m_targetLevelMax);
    lua_settable(L, -3);
    lua_pushnumber(L, 8.0);
    lua_pushnumber(L, rec->m_expansionLevel);
    lua_settable(L, -3);
    lua_pushnumber(L, 9.0);
    lua_pushnumber(L, -rec->m_groupID);
    lua_settable(L, -3);
    lua_pushnumber(L, 10.0);
    lua_pushstring(L, rec->m_textureFilename);
    lua_settable(L, -3);
    lua_pushnumber(L, 11.0);
    lua_pushnumber(L, rec->m_difficulty);
    lua_settable(L, -3);
    lua_pushnumber(L, 12.0);

    if (difficulty) {
        lua_pushnumber(L, difficulty->m_maxPlayers);
        lua_settable(L, -3);
    } else {
        lua_pushnil(L);
        lua_settable(L, -3);
    }

    lua_pushnumber(L, 13.0);
    lua_pushstring(L, rec->m_description);
    lua_settable(L, -3);
    lua_pushnumber(L, 14.0);
    lua_pushboolean(L, rec->m_flags & 0x4);
    lua_settable(L, -3);
}

// VERIFY: 28164d37 manual
// OFFSET: 0x005551E0
static int32_t Script_GetLFDChoiceInfo(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFDChoiceInfo([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    uint32_t count = CGLookingForGroup::m_lfdEntries.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (i < CGLookingForGroup::m_lfdEntries.Count()) {
            lua_pushnumber(L, CGLookingForGroup::m_lfdEntries.m_data[i].m_id);
            lua_createtable(L, 0, 0);
            PushLFDChoiceInfo(L, CGLookingForGroup::m_lfdEntries.m_data[i].m_id);
            lua_settable(L, -3);
        }
    }

    count = CGLookingForGroup::m_lfrEntries.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (i >= CGLookingForGroup::m_lfrEntries.Count()) {
            continue;
        }

        double id = CGLookingForGroup::m_lfrEntries.m_data[i].m_id;
        lua_pushnumber(L, id);
        lua_gettable(L, -2);

        if (lua_type(L, -1) > LUA_TNIL) {
            lua_settop(L, -2);
        } else {
            lua_settop(L, -2);
            lua_pushnumber(L, id);
            lua_createtable(L, 0, 0);
            PushLFDChoiceInfo(L, CGLookingForGroup::m_lfrEntries.m_data[i].m_id);
            lua_settable(L, -3);
        }
    }

    return 1;
}

// VERIFY: e200eb4e manual
// OFFSET: 0x005553B0
static int32_t Script_GetNumRandomDungeons(lua_State* L) {
    lua_pushnumber(L, CGLookingForGroup::m_randomDungeons.Count());
    return 1;
}

// VERIFY: 8bcc206c auto
// OFFSET: 0x00553170
static int32_t Script_CanPartyLFGBackfill(lua_State* L) {
    if (CGPartyInfo::m_realNumMembers && CGPartyInfo::m_realNumMembers < 4 && CGPartyInfo::m_lfgSlot && !(CGPartyInfo::m_playerLFGFlags & 0x2)) {
        lua_pushboolean(L, 1);
        return 1;
    }

    lua_pushboolean(L, 0);
    return 1;
}

// VERIFY: 430d119c auto
// OFFSET: 0x005556D0
static int32_t Script_GetPartyLFGBackfillInfo(lua_State* L) {
    if (!CGPartyInfo::m_realNumMembers || CGPartyInfo::m_realNumMembers >= 4 || !CGPartyInfo::m_lfgSlot || (CGPartyInfo::m_playerLFGFlags & 0x2)) {
        return 0;
    }

    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGPartyInfo::m_lfgSlot & 0xFFFFFF);
    lua_pushstring(L, rec->m_name);
    lua_pushnumber(L, rec->m_id);
    lua_pushnumber(L, rec->m_typeID);
    return 3;
}

// VERIFY: 582b07ce manual
// OFFSET: 0x00553BA0
static int32_t Script_SearchLFGGetNumResults(lua_State* L) {
    double numPlayers;

    if (CGLookingForGroup::m_searchResults.m_lfgID) {
        lua_pushnumber(L, CGLookingForGroup::m_searchResults.m_playerList.m_count + CGLookingForGroup::m_searchResults.m_partyList.m_count);
        numPlayers = CGLookingForGroup::m_searchResults.m_totalParties + CGLookingForGroup::m_searchResults.m_totalPlayers;
    } else {
        lua_pushnumber(L, 0.0);
        numPlayers = 0.0;
    }

    lua_pushnumber(L, numPlayers);
    return 2;
}

// VERIFY: c0039394 manual
// OFFSET: 0x00554BE0
static int32_t Script_GetLFDChoiceOrder(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFDChoiceOrder([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    uint32_t count = CGLookingForGroup::m_lfdEntries.Count();
    uint32_t key = 1;

    for (uint32_t i = 0; i < count; i++) {
        if (i < CGLookingForGroup::m_lfdEntries.Count()) {
            CGLookingForGroup::LFGEntryInfo entry = CGLookingForGroup::m_lfdEntries.m_data[i];
            lua_pushnumber(L, key++);
            lua_pushnumber(L, entry.m_id);
            lua_settable(L, -3);
        }
    }

    return 1;
}

// VERIFY: 49aafc6d manual
// OFFSET: 0x00554D20
static int32_t Script_GetLFRChoiceOrder(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFRChoiceOrder([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    uint32_t count = CGLookingForGroup::m_lfrEntries.Count();
    uint32_t key = 1;

    for (uint32_t i = 0; i < count; i++) {
        if (i < CGLookingForGroup::m_lfrEntries.Count()) {
            CGLookingForGroup::LFGEntryInfo entry = CGLookingForGroup::m_lfrEntries.m_data[i];
            lua_pushnumber(L, key++);
            lua_pushnumber(L, entry.m_id);
            lua_settable(L, -3);
        }
    }

    return 1;
}

// VERIFY: 66873f4c auto
// OFFSET: 0x00552E90
static int32_t Script_GetLFGInfoLocal(lua_State* L) {
    lua_pushboolean(L, CGLookingForGroup::m_localInfo.m_joined);
    lua_pushboolean(L, CGLookingForGroup::m_localInfo.m_noPartial);
    lua_pushboolean(L, CGLookingForGroup::m_localInfo.m_achievements);
    lua_pushstring(L, CGLookingForGroup::m_localInfo.m_comment);
    lua_pushnumber(L, CGLookingForGroup::m_localInfo.m_numDungeons);

    for (uint32_t i = 0; i < 3; i++) {
        lua_pushnumber(L, CGLookingForGroup::m_localInfo.m_roles[i]);
    }

    return 8;
}

// VERIFY: e6f7d5e1 auto
// OFFSET: 0x00552F30
static int32_t Script_GetLFGInfoServer(lua_State* L) {
    LFGInfo* info;
    int32_t inParty;

    if (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers) {
        info = &CGLookingForGroup::m_partyInfo;
        inParty = 1;
    } else {
        info = &CGLookingForGroup::m_playerInfo;
        inParty = 0;
    }

    lua_pushboolean(L, inParty);
    lua_pushboolean(L, info->m_joined);
    lua_pushboolean(L, info->m_queued);
    lua_pushboolean(L, info->m_noPartial);
    lua_pushboolean(L, info->m_achievements);
    lua_pushstring(L, info->m_comment);
    lua_pushnumber(L, info->m_numDungeons);

    for (uint32_t i = 0; i < 3; i++) {
        lua_pushnumber(L, info->m_roles[i]);
    }

    return 10;
}

// VERIFY: 1276fa18 manual
// OFFSET: 0x00553F90
static int32_t Script_GetLFGQueueStats(lua_State* L) {
    LFGInfo* info = CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers ? &CGLookingForGroup::m_partyInfo : &CGLookingForGroup::m_playerInfo;

    if (!info->m_joined || !info->m_queued || !CGLookingForGroup::m_queueStatus.m_lfgID) {
        return 0;
    }

    lua_pushboolean(L, 1);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_tanksNeeded);
    lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_healersNeeded);
    lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_damageNeeded);
    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_queueStatus.m_lfgID);

    if (rec) {
        lua_pushnumber(L, rec->m_typeID);
        lua_pushstring(L, rec->m_name);
        lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_tankWait);
        lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_healerWait);
        lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_damageWait);
        lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_myWait);
        lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_averageWait);
        lua_pushnumber(L, CGLookingForGroup::m_queueStatus.m_queuedTime * 0.001);
        return 13;
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 13;
}

// VERIFY: c766a268 manual
// OFFSET: 0x00557520
static int32_t Script_GetLFGQueuedList(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFDQueuedList([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    LFGInfo* info = CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers ? &CGLookingForGroup::m_partyInfo : &CGLookingForGroup::m_playerInfo;

    for (LFGDungeonEntry* entry = info->m_dungeons.Head(); entry; entry = info->m_dungeons.Next(entry)) {
        lua_pushnumber(L, entry->m_slot & 0xFFFFFF);
        lua_pushboolean(L, 1);
        lua_settable(L, -3);
    }

    return 1;
}

// VERIFY: 3fc37ebb manual
// OFFSET: 0x00557890
static int32_t Script_GetLFDLockPlayerCount(lua_State* L) {
    if (!CGLookingForGroup::m_playerLockInfo.m_valid) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    if (!CGLookingForGroup::m_partyLockInfo.m_valid) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    int32_t count = 0;

    for (LFGLockInfo* member = CGLookingForGroup::m_partyLockInfo.m_members.Head(); member; member = CGLookingForGroup::m_partyLockInfo.m_members.Next(member)) {
        count++;
    }

    lua_pushnumber(L, count + 1);
    return 1;
}

// VERIFY: 51dce08f auto
// OFFSET: 0x00557BE0
static int32_t Script_GetLFGDungeonRewardInfo(lua_State* L) {
    // STATIC: 0x00BEABC8
    static char texture[260];

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetLFGDungeonRewardInfo(dungeonID, lootIndex)");
        return 0;
    }

    int32_t dungeonID = lua_tointeger(L, 1);
    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(dungeonID);

    if (!rec || (rec->m_typeID != 6 && !(rec->m_flags & 0x4))) {
        return 0;
    }

    uint32_t slot = (rec->m_typeID << 24) | (dungeonID & 0xFFFFFF);
    LFGRewardItem* item = CGLookingForGroup::GetDungeonRewardItem(slot, lua_tointeger(L, 2) - 1);

    if (!item) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    const char* name = "";
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_itemID, none, RandomDungeonRewardItemStatsCallback, nullptr, false);

    if (stats) {
        name = stats->GetName(0);
    }

    const char* path = ClientDBStringLookup(3);
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(item->m_displayID));
    lua_pushstring(L, name);
    lua_pushstring(L, texture);
    lua_pushnumber(L, item->m_quantity);
    return 3;
}

// VERIFY: ea869eb8 manual
// OFFSET: 0x00557E40
static int32_t Script_GetLFGCompletionReward(lua_State* L) {
    if (!CGLookingForGroup::m_reward.m_actualSlot) {
        return 0;
    }

    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_reward.m_actualSlot & 0xFFFFFF);

    if (!rec) {
        return 0;
    }

    lua_pushstring(L, rec->m_name);
    lua_pushnumber(L, rec->m_typeID);
    lua_pushstring(L, rec->m_textureFilename);
    lua_pushnumber(L, CGLookingForGroup::m_reward.m_baseMoney);
    lua_pushnumber(L, CGLookingForGroup::m_reward.m_variableMoney);
    lua_pushnumber(L, CGLookingForGroup::m_reward.m_baseXP);
    lua_pushnumber(L, CGLookingForGroup::m_reward.m_variableXP);
    lua_pushnumber(L, CGLookingForGroup::m_reward.m_strangers);
    int32_t count = 0;

    for (LFGRewardItem* item = CGLookingForGroup::m_reward.m_items.Head(); item; item = CGLookingForGroup::m_reward.m_items.Next(item)) {
        count++;
    }

    lua_pushnumber(L, count);
    return 9;
}

// VERIFY: ae8df1b4 manual
// OFFSET: 0x00557F70
static int32_t Script_GetLFGCompletionRewardItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGCompletionRewardItem(index)");
        return 0;
    }

    int32_t index = lua_tointeger(L, 1) - 1;
    LFGRewardItem* item = CGLookingForGroup::m_reward.m_items.Head();

    while (true) {
        if (!item) {
            return 0;
        }

        if (!index--) {
            break;
        }

        item = CGLookingForGroup::m_reward.m_items.Next(item);
    }

    const char* path = ClientDBStringLookup(3);
    SStrPrintf(s_rewardItemTexture, sizeof(s_rewardItemTexture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(item->m_displayID));
    lua_pushstring(L, s_rewardItemTexture);
    lua_pushnumber(L, item->m_quantity);
    return 2;
}

// VERIFY: ddc1f0e5 auto
// OFFSET: 0x00557660
static int32_t Script_JoinLFG(lua_State* L) {
    if (!CGLookingForGroup::m_localInfo.m_numDungeons) {
        CGGameUI::DisplayError(ERR_LFG_NO_SLOTS_SELECTED);
        return 0;
    }

    if (!s_cvLfgSelectedRoles || !(s_cvLfgSelectedRoles->m_intValue & ~0x1)) {
        CGGameUI::DisplayError(ERR_LFG_NO_ROLES_SELECTED);
        return 0;
    }

    CGLookingForGroup::JoinLFG();
    return 0;
}

// VERIFY: dcf03527 manual
// OFFSET: 0x005576B0
static int32_t Script_GetLFDChoiceLockedState(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFDChoiceLockedState([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    if (CGLookingForGroup::m_partyLockInfo.m_valid) {
        for (LFGLockInfo* member = CGLookingForGroup::m_partyLockInfo.m_members.Head(); member; member = CGLookingForGroup::m_partyLockInfo.m_members.Next(member)) {
            for (LFGDungeonEntry* entry = member->m_dungeons.Head(); entry; entry = member->m_dungeons.Next(entry)) {
                if ((entry->m_slot >> 24) != 2 && entry->m_locked) {
                    lua_pushnumber(L, entry->m_slot & 0xFFFFFF);
                    lua_pushnumber(L, entry->m_lockReason);
                    lua_settable(L, -3);
                }
            }
        }
    }

    if (CGLookingForGroup::m_playerLockInfo.m_valid) {
        for (LFGDungeonEntry* entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Head(); entry; entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Next(entry)) {
            if (entry->m_locked) {
                lua_pushnumber(L, entry->m_slot & 0xFFFFFF);
                lua_pushnumber(L, entry->m_lockReason);
                lua_settable(L, -3);
            }
        }
    }

    return 1;
}

// VERIFY: c82601c8 auto
// OFFSET: 0x005586D0
static int32_t Script_IsListedInLFR(lua_State* L) {
    LFGInfo* info = CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers ? &CGLookingForGroup::m_partyInfo : &CGLookingForGroup::m_playerInfo;
    lua_pushboolean(L, CGLookingForGroup::HasRaidDungeon(info));
    return 1;
}

// VERIFY: 31a496e4 auto
// OFFSET: 0x00554680
static int32_t Script_GetLFGRoleUpdate(lua_State* L) {
    return CGLookingForGroup::GetLFGRoleUpdate(L);
}

// VERIFY: bd40434c auto
// OFFSET: 0x00554740
static int32_t Script_GetLFGRoleUpdateSlot(lua_State* L) {
    return CGLookingForGroup::GetLFGRoleUpdateSlot(L);
}

// VERIFY: 667e80c2 auto
// OFFSET: 0x00554170
static int32_t PushLFGProposalInfo(lua_State* L) {
    if (!CGLookingForGroup::m_proposal.m_dungeonId) {
        return 0;
    }

    LfgDungeonsRec* dungeon = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_proposal.m_dungeonId & 0xFFFFFF);

    if (!dungeon) {
        return 0;
    }

    lua_pushboolean(L, 1);
    lua_pushnumber(L, CGLookingForGroup::m_proposal.m_dungeonId >> 24);
    lua_pushnumber(L, CGLookingForGroup::m_proposal.m_dungeonId & 0xFFFFFF);
    lua_pushstring(L, dungeon->m_name);
    lua_pushstring(L, dungeon->m_textureFilename);

    uint32_t roles = 0;
    uint32_t accepted = 0;

    for (uint32_t i = 0; i < 5; i++) {
        if (CGLookingForGroup::m_proposal.m_members[i].m_isLocal) {
            roles = CGLookingForGroup::m_proposal.m_members[i].m_roles;
            accepted = CGLookingForGroup::m_proposal.m_members[i].m_answered;
            break;
        }
    }

    if (roles & 0x2) {
        lua_pushstring(L, "TANK");
    } else if (roles & 0x4) {
        lua_pushstring(L, "HEALER");
    } else if (roles & 0x8) {
        lua_pushstring(L, "DAMAGER");
    } else {
        lua_pushstring(L, "UNKNOWN");
    }

    lua_pushboolean(L, accepted);

    uint32_t total = 0;
    uint32_t completed = 0;

    if (dungeon->m_typeID == 6) {
        total = 0;
        completed = CGLookingForGroup::m_proposal.m_completedMask != 0;
    } else if (dungeon->m_flags & 0x4) {
        total = 1;
        completed = CGLookingForGroup::m_proposal.m_completedMask != 0;
    } else {
        DungeonEncounterCountMatchingRows(dungeon->m_mapID, dungeon->m_difficulty, CGLookingForGroup::m_proposal.m_completedMask, &total, &completed);
    }

    lua_pushnumber(L, total);
    lua_pushnumber(L, completed);
    lua_pushnumber(L, 5.0);
    lua_pushboolean(L, roles & 0x1);
    lua_pushboolean(L, (dungeon->m_flags >> 2) & 0x1);
    return 12;
}

// VERIFY: a41527a2 auto
// OFFSET: 0x00554370
static int32_t Script_GetLFGProposal(lua_State* L) {
    return PushLFGProposalInfo(L);
}

// VERIFY: 3e7ae729 manual
// OFFSET: 0x005553F0
static int32_t PushDungeonInfo(lua_State* L, int32_t dungeonId) {
    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(dungeonId);

    if (!rec) {
        return 0;
    }

    LfgDungeonExpansionRec* expansion = GetLfgDungeonExpansionRec(rec->m_id, ClientServices::Connection()->m_accountExpansion);
    MapDifficultyRec* difficulty = GetMapDifficultyEntryByMapIdAndDifficulty(rec->m_mapID, rec->m_difficulty, nullptr);
    lua_pushstring(L, rec->m_name);
    lua_pushnumber(L, rec->m_typeID);
    lua_pushnumber(L, expansion ? expansion->m_hardLevelMin : rec->m_minLevel);
    lua_pushnumber(L, expansion ? expansion->m_hardLevelMax : rec->m_maxLevel);
    lua_pushnumber(L, rec->m_targetLevel);
    lua_pushnumber(L, expansion ? expansion->m_targetLevelMin : rec->m_targetLevelMin);
    lua_pushnumber(L, expansion ? expansion->m_targetLevelMax : rec->m_targetLevelMax);
    lua_pushnumber(L, rec->m_expansionLevel);
    lua_pushnumber(L, rec->m_groupID);
    lua_pushstring(L, rec->m_textureFilename);
    lua_pushnumber(L, rec->m_difficulty);

    if (difficulty) {
        lua_pushnumber(L, difficulty->m_maxPlayers);
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, rec->m_description);
    lua_pushboolean(L, rec->m_flags & 0x4);
    return 14;
}

// VERIFY: aa8994fc auto
// OFFSET: 0x00555590
static int32_t Script_GetLFGDungeonInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGDungeonInfo(dungeonID)");
        return 0;
    }

    return PushDungeonInfo(L, lua_tointeger(L, 1));
}

// VERIFY: 70fc345d manual
// OFFSET: 0x00557480
static LFGDungeonEntry* FindPlayerDungeon(uint32_t slot) {
    if (!CGLookingForGroup::m_playerLockInfo.m_valid) {
        return nullptr;
    }

    for (LFGDungeonEntry* entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Head(); entry; entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Next(entry)) {
        if (entry->m_slot == slot) {
            return entry;
        }
    }

    return nullptr;
}

// VERIFY: 570d24db manual
// OFFSET: 0x00557930
static int32_t Script_GetLFDLockInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetLFDLockInfo(dungeonID, playerIndex)");
        return 0;
    }

    int32_t dungeonID = lua_tointeger(L, 1);

    if (dungeonID < 0) {
        return 0;
    }

    int32_t playerIndex = lua_tointeger(L, 2);
    LFGLockInfo* info = nullptr;

    if (playerIndex == 1) {
        if (!CGLookingForGroup::m_playerLockInfo.m_valid) {
            return 0;
        }

        info = &CGLookingForGroup::m_playerLockInfo;
    } else {
        int32_t remaining = playerIndex - 1;

        if (!CGLookingForGroup::m_partyLockInfo.m_valid) {
            return 0;
        }

        for (LFGLockInfo* member = CGLookingForGroup::m_partyLockInfo.m_members.Head(); member; member = CGLookingForGroup::m_partyLockInfo.m_members.Next(member)) {
            if (!--remaining) {
                info = member;
            }
        }

        if (!info) {
            return 0;
        }
    }

    WOWGUID none = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(info->m_guid.m_guid, none, nullptr, nullptr, false);
    const char* name = record ? record->m_name : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
    lua_pushstring(L, name);

    for (LFGDungeonEntry* entry = info->m_dungeons.Head(); entry; entry = info->m_dungeons.Next(entry)) {
        if ((entry->m_slot & 0xFFFFFF) == static_cast<uint32_t>(dungeonID)) {
            lua_pushnumber(L, entry->m_locked ? entry->m_lockReason : 0);
            return 2;
        }
    }

    lua_pushnumber(L, 0.0);
    return 2;
}

// VERIFY: 0c2da627 manual
// OFFSET: 0x00557AA0
static int32_t Script_GetLFGDungeonRewards(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGDungeonRewards(dungeonID)");
        return 0;
    }

    int32_t dungeonId = lua_tointeger(L, 1);
    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(dungeonId);

    if (!rec) {
        return 0;
    }

    LFGDungeonEntry* entry = FindPlayerDungeon(dungeonId & 0xFFFFFF | (rec->m_typeID << 24));

    if (entry) {
        int32_t items = 0;

        for (LFGRewardItem* item = entry->m_rewards.Head(); item; item = entry->m_rewards.Next(item)) {
            items++;
        }

        lua_pushboolean(L, entry->m_rewardDone);
        lua_pushnumber(L, entry->m_baseMoney);
        lua_pushnumber(L, entry->m_variableMoney);
        lua_pushnumber(L, entry->m_baseXP);
        lua_pushnumber(L, entry->m_variableXP);
        lua_pushnumber(L, items);
    } else {
        lua_pushboolean(L, 0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 6;
}

// VERIFY: 399640cb manual
// OFFSET: 0x005555D0
static int32_t Script_GetLFGRandomDungeonInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGRandomDungeonInfo(index)");
        return 0;
    }

    uint32_t index = lua_tointeger(L, 1) - 1;

    if (index >= CGLookingForGroup::m_randomDungeons.m_count) {
        return 0;
    }

    int32_t dungeonId = CGLookingForGroup::m_randomDungeons.m_data[index] & 0xFFFFFF;
    lua_pushnumber(L, dungeonId);
    return PushDungeonInfo(L, dungeonId) + 1;
}

// VERIFY: f9d18845 auto
// OFFSET: 0x005595D0
static int32_t Script_ClearAllLFGDungeons(lua_State* L) {
    CGLookingForGroup::ClearDungeonList();
    return 0;
}

// VERIFY: 002364d9 auto
// OFFSET: 0x00559F50
static int32_t Script_SetLFGDungeon(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetLFGDungeon(type)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(id);

    if (rec) {
        CGLookingForGroup::SetLFGDungeon((id & 0xFFFFFF) | (rec->m_typeID << 24));
    }

    return 0;
}

// VERIFY: 3307daf9 auto
// OFFSET: 0x0055A040
static int32_t Script_IsLFGDungeonJoinable(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsLFGDungeonJoinable(dungeonID)");
        return 0;
    }

    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(lua_tointeger(L, 1));

    if (!rec) {
        return 0;
    }

    lua_pushboolean(L, CGLookingForGroup::IsSlotValidForAll(rec->m_id & 0xFFFFFF | (rec->m_typeID << 24)));
    return 1;
}

// VERIFY: 45eb8fb4 manual
// OFFSET: 0x005564D0
static int32_t Script_GetLFDChoiceCollapseState(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFDChoiceCollapseState([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    uint32_t count;
    count = CGLookingForGroup::m_lfdEntries.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (i >= CGLookingForGroup::m_lfdEntries.Count()) {
            continue;
        }

        CGLookingForGroup::LFGEntryInfo entry = CGLookingForGroup::m_lfdEntries.m_data[i];

        if (entry.m_id >= 0) {
            LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(entry.m_id);
            lua_pushnumber(L, entry.m_id);
            uint32_t header = rec->m_groupID;
            bool collapsed = false;

            if (s_cvLfdCollapsedHeaders) {
                CVarArrayWrapper wrapper(s_cvLfdCollapsedHeaders);
                collapsed = wrapper.RawGetValue(header) != wrapper.m_defaultValue;
            }

            lua_pushboolean(L, collapsed);
        } else {
            lua_pushnumber(L, entry.m_id);
            uint32_t header = -entry.m_id;

            if (s_cvLfdCollapsedHeaders) {
                CVarArrayWrapper wrapper(s_cvLfdCollapsedHeaders);
                lua_pushboolean(L, wrapper.RawGetValue(header) != wrapper.m_defaultValue);
            } else {
                lua_pushboolean(L, 0);
            }
        }

        lua_settable(L, -3);
    }

    count = CGLookingForGroup::m_lfrEntries.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (i >= CGLookingForGroup::m_lfrEntries.Count()) {
            continue;
        }

        CGLookingForGroup::LFGEntryInfo entry = CGLookingForGroup::m_lfrEntries.m_data[i];

        if (entry.m_id >= 0) {
            LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(entry.m_id);
            lua_pushnumber(L, entry.m_id);
            uint32_t header = rec->m_groupID;
            bool collapsed = false;

            if (s_cvLfdCollapsedHeaders) {
                CVarArrayWrapper wrapper(s_cvLfdCollapsedHeaders);
                collapsed = wrapper.RawGetValue(header) != wrapper.m_defaultValue;
            }

            lua_pushboolean(L, collapsed);
        } else {
            lua_pushnumber(L, entry.m_id);
            uint32_t header = -entry.m_id;

            if (s_cvLfdCollapsedHeaders) {
                CVarArrayWrapper wrapper(s_cvLfdCollapsedHeaders);
                lua_pushboolean(L, wrapper.RawGetValue(header) != wrapper.m_defaultValue);
            } else {
                lua_pushboolean(L, 0);
            }
        }

        lua_settable(L, -3);
    }

    return 1;
}

// VERIFY: bf8c7985 auto
// OFFSET: 0x005567E0
static int32_t Script_GetLFDChoiceEnabledState(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE && lua_type(L, 1) > LUA_TNIL) {
        luaL_error(L, "Usage: GetLFDChoiceEnabledState([table])");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TTABLE) {
        luaL_checkstack(L, 3, "");
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_settop(L, -2);
            lua_pushvalue(L, -1);
            lua_pushnil(L);
            lua_rawset(L, 1);
        }

        lua_settop(L, 1);
    } else {
        lua_createtable(L, 0, 0);
    }

    uint32_t count;
    count = CGLookingForGroup::m_lfdEntries.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (i >= CGLookingForGroup::m_lfdEntries.Count()) {
            continue;
        }

        CGLookingForGroup::LFGEntryInfo entry = CGLookingForGroup::m_lfdEntries.m_data[i];

        if (entry.m_id <= 0 || !s_cvLfdSelectedDungeons) {
            continue;
        }

        CVarArrayWrapper wrapper(s_cvLfdSelectedDungeons);

        if (wrapper.RawGetValue(entry.m_id) != wrapper.m_defaultValue) {
            lua_pushnumber(L, entry.m_id);
            lua_pushboolean(L, 1);
            lua_settable(L, -3);
        }
    }

    count = CGLookingForGroup::m_lfrEntries.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (i >= CGLookingForGroup::m_lfrEntries.Count()) {
            continue;
        }

        CGLookingForGroup::LFGEntryInfo entry = CGLookingForGroup::m_lfrEntries.m_data[i];

        if (entry.m_id <= 0 || !s_cvLfdSelectedDungeons) {
            continue;
        }

        CVarArrayWrapper wrapper(s_cvLfdSelectedDungeons);

        if (wrapper.RawGetValue(entry.m_id) != wrapper.m_defaultValue) {
            lua_pushnumber(L, entry.m_id);
            lua_pushboolean(L, 1);
            lua_settable(L, -3);
        }
    }

    return 1;
}

// VERIFY: 8b0db2c1 auto
// OFFSET: 0x005569D0
static int32_t Script_RequestLFDPlayerLockInfo(lua_State* L) {
    if (++s_playerLockRequests > 2) {
        double now = OsGetAsyncTimeSec();

        if (now - s_playerLockRequestTime >= 10.0) {
            s_playerLockRequestTime = static_cast<float>(now);
            s_playerLockRequests = 0;
        } else if (s_playerLockRequests > 2) {
            lua_pushboolean(L, 0);
            return 1;
        }
    }

    CGLookingForGroup::RequestLFGPlayerInfo();
    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: 09eb253b auto
// OFFSET: 0x00556A50
static int32_t Script_RequestLFDPartyLockInfo(lua_State* L) {
    if (++s_partyLockRequests > 2) {
        double now = OsGetAsyncTimeSec();

        if (now - s_partyLockRequestTime >= 10.0) {
            s_partyLockRequestTime = static_cast<float>(now);
            s_partyLockRequests = 0;
        } else if (s_partyLockRequests > 2) {
            lua_pushboolean(L, 0);
            return 1;
        }
    }

    CGLookingForGroup::RequestLFGPartyInfo();
    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: eaa7e759 auto
// OFFSET: 0x00553F50
static int32_t Script_SetLFGComment(lua_State* L) {
    const char* comment = nullptr;

    if (lua_isstring(L, 1)) {
        comment = lua_tolstring(L, 1, nullptr);
    }

    CGLookingForGroup::SetLFGComment(comment);
    return 0;
}

// VERIFY: 44f6b8fc manual
// OFFSET: 0x00555760
static int32_t Script_UnitHasLFGRandomCooldown(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: UnitHasLFGRandomCooldown(\"unit\")");
        return 0;
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (!guid) {
        return 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        lua_pushboolean(L, unit->HasAuraBySpellId(71328));
        return 1;
    }

    CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(guid);

    if (!state) {
        return 0;
    }

    for (int32_t i = 0; i < 64; i++) {
        if (state->m_auras[i] == 71328) {
            lua_pushboolean(L, 1);
            return 1;
        }
    }

    lua_pushboolean(L, 0);
    return 1;
}

// VERIFY: d2f5b8c5 manual
// OFFSET: 0x00555840
static int32_t Script_UnitHasLFGDeserter(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: UnitHasLFGDeserter(\"unit\")");
        return 0;
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (!guid) {
        return 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        lua_pushboolean(L, unit->HasAuraBySpellId(71041));
        return 1;
    }

    CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(guid);

    if (!state) {
        return 0;
    }

    for (int32_t i = 0; i < 64; i++) {
        if (state->m_auras[i] == 71041) {
            lua_pushboolean(L, 1);
            return 1;
        }
    }

    lua_pushboolean(L, 0);
    return 1;
}

// VERIFY: a86675b0 auto
// OFFSET: 0x005594E0
static int32_t Script_SearchLFGLeave(lua_State* L) {
    if (CGGameUI::CanPerformAction(9)) {
        CGLookingForGroup::LeaveLFGSearch();
    }

    return 0;
}

// STATIC: 0x00ACDD54
static int32_t s_roleCheckCount = 2;

// STATIC: 0x00BEABAC
static float s_roleCheckTime;

// STATIC: 0x00ACDD68
static int32_t s_teleportCount = 2;

// STATIC: 0x00BEABC0
static float s_teleportTime;

// VERIFY: c5cfe210 manual
// OFFSET: 0x005529E0
static int32_t AddLFGPlayerInfo(lua_State* L, LFGListPlayer* player) {
    if (player) {
        lua_pushboolean(L, player->m_roles & 0x1);
        lua_pushboolean(L, player->m_roles & 0x2);
        lua_pushboolean(L, player->m_roles & 0x4);
        lua_pushboolean(L, player->m_roles & 0x8);
        lua_pushnumber(L, player->unk_0064);
        lua_pushnumber(L, player->m_talents[0]);
        lua_pushnumber(L, player->m_talents[1]);
        lua_pushnumber(L, player->m_talents[2]);
        lua_pushboolean(L, player->m_leader);
        lua_pushnumber(L, player->m_armor);
        lua_pushnumber(L, player->m_spellDamage);
        lua_pushnumber(L, player->m_spellHeal);
        lua_pushnumber(L, player->m_critMelee);
        lua_pushnumber(L, player->m_critRanged);
        lua_pushnumber(L, player->m_critSpell);
        lua_pushnumber(L, player->m_mp5);
        lua_pushnumber(L, player->m_mp5Combat);
        lua_pushnumber(L, player->m_attackPower);
        lua_pushnumber(L, player->m_agility);
        lua_pushnumber(L, player->m_maxHealth);
        lua_pushnumber(L, player->m_maxMana);
        lua_pushnumber(L, player->m_avgItemLevel);
        lua_pushnumber(L, player->m_defenseSkill);
        lua_pushnumber(L, player->m_dodge);
        lua_pushnumber(L, player->m_block);
        lua_pushnumber(L, player->m_parry);
        lua_pushnumber(L, player->m_haste);
        lua_pushnumber(L, player->m_expertise);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushboolean(L, 0);
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
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 28;
}

// VERIFY: 4b03e77f auto
// OFFSET: 0x00553110
static int32_t Script_GetLFGProposalMember(lua_State* L) {
    return CGLookingForGroup::GetLFGProposalMember(L);
}

// VERIFY: eec094ff manual
// OFFSET: 0x00553530
static bool GetLFGInfoByIndex(uint32_t index, LFGListParty** party, LFGListPlayer** player) {
    LFGLIST* list = &CGLookingForGroup::m_searchResults;

    if (CGLookingForGroup::m_unkACD660) {
        if (index < list->m_playerList.m_count) {
            *party = nullptr;
            *player = list->m_playerList.m_data[index];
            return true;
        }

        if (index - list->m_playerList.m_count < list->m_partyList.m_count) {
            LFGListParty* entry = list->m_partyList.m_data[index - list->m_playerList.m_count];
            *party = entry;
            *player = entry && entry->m_members.m_count ? entry->m_members.m_data[0] : nullptr;
            return true;
        }

        return false;
    }

    if (index >= list->m_partyList.m_count) {
        if (index - list->m_partyList.m_count < list->m_playerList.m_count) {
            *party = nullptr;
            *player = list->m_playerList.m_data[index - list->m_partyList.m_count];
            return true;
        }

        return false;
    }

    LFGListParty* entry = list->m_partyList.m_data[index];
    *party = entry;
    *player = entry && entry->m_members.m_count ? entry->m_members.m_data[0] : nullptr;
    return true;
}

// VERIFY: c52dfe5b manual
// OFFSET: 0x00553C30
static int32_t Script_SearchLFGGetEncounterResults(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SearchLFGGetEncounterResults(index, encounterIndex)");
        return 0;
    }

    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_searchResults.m_lfgID & 0xFFFFFF);

    if (!rec) {
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1));
    double encounter = lua_tonumber(L, 2);
    LFGListPlayer* player = nullptr;
    LFGListParty* party = nullptr;

    if (!GetLFGInfoByIndex(index - 1, &party, &player)) {
        return 0;
    }

    if (!player) {
        return PushDungeonEncounterInfo(L, 0, 0, 0, 0);
    }

    uint32_t mask = party ? party->m_encounterMask : player->m_encounterMask;
    return PushDungeonEncounterInfo(L, rec->m_mapID, rec->m_difficulty, static_cast<int32_t>(encounter - 1.0), mask);
}

// VERIFY: f272c3fa manual
// OFFSET: 0x00553E20
static int32_t Script_CompleteLFGRoleCheck(lua_State* L) {
    if (++s_roleCheckCount > 2) {
        float now = OsGetAsyncTimeSec();

        if (now - s_roleCheckTime < 10.0) {
            if (s_roleCheckCount > 2) {
                return 0;
            }
        } else {
            s_roleCheckTime = now;
            s_roleCheckCount = 0;
        }
    }

    if (!s_cvLfgSelectedRoles) {
        return 0;
    }

    uint8_t roles = 0;

    if (StringToBOOL(L, 1, 0)) {
        roles = FilterRolesByClass(static_cast<uint8_t>(s_cvLfgSelectedRoles->m_intValue));

        if (!(roles & 0xE)) {
            CGGameUI::DisplayError(ERR_LFG_NO_ROLES_SELECTED);
            lua_pushboolean(L, 0);
            return 1;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_SET_ROLES));
    msg.Put(roles);
    msg.Finalize();
    ClientServices::Send2(&msg);
    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: c34393b3 auto
// OFFSET: 0x00554160
static int32_t Script_LeaveLFG(lua_State* L) {
    CGLookingForGroup::LeaveLFG();
    return 0;
}

// VERIFY: 3b50b521 auto
// OFFSET: 0x00554440
static int32_t Script_GetLFGProposalEncounter(lua_State* L) {
    return CGLookingForGroup::GetLFGProposalEncounter(L);
}

// VERIFY: e41907d8 auto
// OFFSET: 0x005545E0
static int32_t Script_AcceptProposal(lua_State* L) {
    if (CGGameUI::CanPerformAction(9)) {
        CGLookingForGroup::ProposalResponse(true);
    }

    return 0;
}

// VERIFY: e3b26d99 auto
// OFFSET: 0x00554600
static int32_t Script_RejectProposal(lua_State* L) {
    CGLookingForGroup::ProposalResponse(false);
    return 0;
}

// VERIFY: dc7cb7a4 auto
// OFFSET: 0x005548E0
static int32_t Script_GetLFGRoleUpdateMember(lua_State* L) {
    return CGLookingForGroup::GetLFGRoleUpdateMember(L);
}

// VERIFY: a316c04f auto
// OFFSET: 0x00554A50
static int32_t Script_SetLFGBootVote(lua_State* L) {
    return CGLookingForGroup::SetLFGBootVote(L);
}

// VERIFY: 4d375cba auto
// OFFSET: 0x00554BD0
static int32_t Script_GetLFGBootProposal(lua_State* L) {
    return CGLookingForGroup::GetLFGBootProposal(L);
}

// VERIFY: 679121ab auto
// OFFSET: 0x00555660
static int32_t Script_IsInLFGDungeon(lua_State* L) {
    if (!CGPartyInfo::m_lfgSlot) {
        return 0;
    }

    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGPartyInfo::m_lfgSlot & 0xFFFFFF);

    if (!rec) {
        return 0;
    }

    lua_pushboolean(L, rec->m_mapID == CGGameUI::m_iCurrentMapID && rec->m_difficulty == static_cast<int32_t>(CGGameUI::GetInstanceDifficulty()));
    return 1;
}

// VERIFY: ef0b4eac auto
// OFFSET: 0x00555FD0
static int32_t Script_SearchLFGGetResults(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SearchLFGGetResults(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tointeger(L, 1)) - 1;
    LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_searchResults.m_lfgID & 0xFFFFFF);

    if (!rec) {
        return 0;
    }

    LFGListPlayer* player = nullptr;
    LFGListParty* party = nullptr;

    if (!GetLFGInfoByIndex(index, &party, &player) || !player) {
        return 0;
    }

    NameCache* name = g_nameDBCache.GetRecord(player->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
    lua_pushstring(L, name ? name->m_name : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    lua_pushnumber(L, player->m_level);
    AreaTableRec* area = g_areaTableDB.GetRecord(player->m_areaID);
    lua_pushstring(L, area ? area->m_areaName : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    const char* className;

    if (name && (className = CGUnit_C::GetDisplayClassName(nullptr, name))) {
        lua_pushstring(L, className);
    } else {
        lua_pushstring(L, FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    }

    char comment[256];

    if (party) {
        SStrCopy(comment, party->m_comment, STORM_MAX_STR);
    } else {
        SStrCopy(comment, player->m_comment, STORM_MAX_STR);
    }

    ProfanityFilter(comment, false, false);
    lua_pushstring(L, comment);
    lua_pushnumber(L, party ? party->GetNumMembers() : 0);
    lua_pushboolean(L, player->unk_0088 != 0);
    ChrClassesRec* classRec;

    if (name && (classRec = g_chrClassesDB.GetRecord(name->m_class)) && classRec->m_filename) {
        lua_pushstring(L, classRec->m_filename);
    } else {
        lua_pushnil(L);
    }

    uint32_t mask = party ? party->m_encounterMask : player->m_encounterMask;
    uint32_t total = 0;
    uint32_t completed = 0;
    DungeonEncounterCountMatchingRows(rec->m_mapID, rec->m_difficulty, mask, &total, &completed);
    lua_pushnumber(L, total);
    lua_pushnumber(L, completed);
    return AddLFGPlayerInfo(L, player) + 10;
}

// VERIFY: ae53c1d8 manual
// OFFSET: 0x00556280
static int32_t Script_SearchLFGGetPartyResults(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SearchLFGGetPartyResults(index, partyIndex)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t partyIndex = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    LFGListParty* party = nullptr;
    LFGListPlayer* leader;

    if (!GetLFGInfoByIndex(index, &party, &leader)) {
        return 0;
    }

    LFGListPlayer* player;
    NameCache* name;

    if (party && (player = party->GetMember(partyIndex)) && (name = g_nameDBCache.GetRecord(player->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false))) {
        lua_pushstring(L, name->m_name);
        lua_pushnumber(L, player->m_level);

        if (FriendList::s_friendList->IsIgnored(player->m_key.m_guid, true)) {
            lua_pushstring(L, "ignored");
        } else if (FriendList::s_friendList->IsFriend(player->m_key.m_guid)) {
            lua_pushstring(L, "friend");
        } else {
            lua_pushnil(L);
        }

        const char* className = CGUnit_C::GetDisplayClassName(nullptr, name);
        lua_pushstring(L, className ? className : "");
        AreaTableRec* area = g_areaTableDB.GetRecord(player->m_areaID);
        lua_pushstring(L, area ? area->m_areaName : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
        char comment[256];
        SStrCopy(comment, player->m_comment, STORM_MAX_STR);
        ProfanityFilter(comment, false, false);
        lua_pushstring(L, comment);
        return AddLFGPlayerInfo(L, player) + 6;
    }

    lua_pushnil(L);
    lua_pushstring(L, "");
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return AddLFGPlayerInfo(L, nullptr) + 5;
}

// VERIFY: 1994e414 auto
// OFFSET: 0x00556AD0
static int32_t Script_SetLFGHeaderCollapsed(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetLFGHeaderCollapsed(headerID, isCollapsed)");
        return 0;
    }

    int32_t header = static_cast<int32_t>(lua_tonumber(L, 1));
    int32_t collapsed = StringToBOOL(L, 2, 0);

    if (header < 0 && g_lfgDungeonGroupDB.GetRecord(-header)) {
        CGLookingForGroup::SetHeaderCollapsed(-header, collapsed != 0);
    }

    return 0;
}

// VERIFY: 28acc2e2 auto
// OFFSET: 0x00556B60
static int32_t Script_SetLFGDungeonEnabled(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetLFGDungeonEnabled(dungeonID, isEnabled)");
        return 0;
    }

    int32_t dungeon = static_cast<int32_t>(lua_tonumber(L, 1));
    int32_t enabled = StringToBOOL(L, 2, 1);

    if (dungeon > 0) {
        CGLookingForGroup::SetDungeonEnabled(dungeon, enabled != 0);
    }

    return 0;
}

// VERIFY: d43adb8c manual
// OFFSET: 0x00556BC0
static int32_t Script_LFGTeleport(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (++s_teleportCount > 2) {
        float now = OsGetAsyncTimeSec();

        if (now - s_teleportTime < 10.0) {
            if (s_teleportCount > 2) {
                lua_pushboolean(L, 0);
                return 1;
            }
        } else {
            s_teleportTime = now;
            s_teleportCount = 0;
        }
    }

    if (player->IsInCombat()) {
        CGGameUI::DisplayError(ERR_NOT_IN_COMBAT);
        lua_pushboolean(L, 0);
        return 1;
    }

    if (player->IsDeadOrGhost()) {
        CGGameUI::DisplayError(ERR_PLAYER_DEAD);
        lua_pushboolean(L, 0);
        return 1;
    }

    if (player->m_movement->m_flags & 0x1000) {
        CGGameUI::DisplayError(ERR_NOT_WHILE_FALLING);
        lua_pushboolean(L, 0);
        return 1;
    }

    if (IsOnTransport(player)) {
        CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
        lua_pushboolean(L, 0);
        return 1;
    }

    MirrorTimerInfo* timer = CGGameUI::MirrorTimerGetInfo(0);

    if (timer && timer->m_scale < 0) {
        CGGameUI::DisplayError(ERR_NOT_WHILE_FATIGUED);
        lua_pushboolean(L, 0);
        return 1;
    }

    int32_t out = StringToBOOL(L, 1, 0);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_TELEPORT));
    msg.Put(static_cast<uint8_t>(out != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: edcce8b9 auto
// OFFSET: 0x00557D60
static int32_t Script_GetLFGDungeonRewardLink(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetLFGDungeonRewardLink(dungeonID, lootIndex)");
        return 0;
    }

    uint32_t slot = (lua_tointeger(L, 1) & 0xFFFFFF) | 0x6000000;
    int32_t index = lua_tointeger(L, 2) - 1;
    LFGRewardItem* item = CGLookingForGroup::GetDungeonRewardItem(slot, index);

    if (!item) {
        return 0;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_itemID, WOWGUID(), nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item->m_itemID, stats->m_quality, 0, 0, 0));
    return 1;
}

// VERIFY: b739931e auto
// OFFSET: 0x00559400
static int32_t Script_SearchLFGJoin(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SearchLFGJoin(typeID, lfgID)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(9)) {
        return 0;
    }

    int32_t type = static_cast<int32_t>(lua_tonumber(L, 1));
    int32_t id = static_cast<int32_t>(lua_tonumber(L, 2));

    if (type < 7 && g_lfgDungeonsDB.GetRecord(id)) {
        CGLookingForGroup::JoinLFGSearch((type << 24) | (id & 0xFFFFFF));
    }

    return 0;
}

// VERIFY: 72f2ad5b auto
// OFFSET: 0x00559500
static int32_t Script_ClearLFGDungeon(lua_State* L) {
    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: ClearLFGDungeon(type, id)");
        return 0;
    }

    uint32_t type = static_cast<uint32_t>(lua_tonumber(L, 1));
    int32_t id = static_cast<int32_t>(lua_tonumber(L, 2));

    if (type < 7 && g_lfgDungeonsDB.GetRecord(id)) {
        CGLookingForGroup::ClearLFGDungeon((type << 24) | (id & 0xFFFFFF));
    }

    return 0;
}

// VERIFY: ce8b2ea6 manual
// OFFSET: 0x00559FF0
static int32_t Script_GetLastQueueStatusIndex(lua_State* L) {
    if (CGLookingForGroup::GetQueueStatusIndex() < 0) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, CGLookingForGroup::GetQueueStatusIndex() + 1);
    }

    return 1;
}

// VERIFY: bf1376c8 auto
// OFFSET: 0x0055A0C0
static int32_t Script_PartyLFGStartBackfill(lua_State* L) {
    if (!s_cvLfgSelectedRoles || !(s_cvLfgSelectedRoles->m_intValue & 0xFFFFFFFE)) {
        CGGameUI::DisplayError(ERR_LFG_NO_ROLES_SELECTED);
        return 0;
    }

    if (!CGPartyInfo::m_realNumMembers || CGPartyInfo::m_realNumMembers >= 4 || !CGPartyInfo::m_lfgSlot || (CGPartyInfo::m_playerLFGFlags & 0x2)) {
        return 0;
    }

    CGLookingForGroup::ClearDungeonList();
    CGLookingForGroup::SetLFGDungeon(CGPartyInfo::m_lfgSlot);
    CGLookingForGroup::JoinLFG();
    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: 2f1b3eba auto
// OFFSET: 0x0055D280
static int32_t Script_RefreshLFGList(lua_State* L) {
    CGLookingForGroup::m_searchResults.Copy(CGLookingForGroup::m_prevSearchResults);
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
    return 0;
}

// VERIFY: b5d518ff manual
// OFFSET: 0x0055D2B0
static int32_t Script_SearchLFGSort(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SearchLFGSort(\"type\")");
        return 0;
    }

    int32_t type = 3;
    const char* name = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(name, "group", STORM_MAX_STR)) {
        CGLookingForGroup::m_unkACD660 = !CGLookingForGroup::m_unkACD660;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
        return 0;
    }

    if (!SStrCmpI(name, "name", STORM_MAX_STR)) {
        type = 3;
    } else if (!SStrCmpI(name, "level", STORM_MAX_STR)) {
        type = 1;
    } else if (!SStrCmpI(name, "class", STORM_MAX_STR)) {
        type = 2;
    } else if (!SStrCmpI(name, "zone", STORM_MAX_STR)) {
        type = 0;
    } else if (!SStrCmpI(name, "tank", STORM_MAX_STR)) {
        type = 4;
    } else if (!SStrCmpI(name, "healer", STORM_MAX_STR)) {
        type = 5;
    } else if (!SStrCmpI(name, "damage", STORM_MAX_STR)) {
        type = 6;
    }

    int32_t index = 0;

    while (CGLookingForGroup::m_unkBEA820[index].m_id != type) {
        if (++index >= 7) {
            CGLookingForGroup::ResortLFGLists();
            return 0;
        }
    }

    int32_t order = CGLookingForGroup::m_unkBEA820[index].m_order;

    if (!index) {
        order = !order;
    }

    for (int32_t i = index; i > 0; i--) {
        CGLookingForGroup::m_unkBEA820[i] = CGLookingForGroup::m_unkBEA820[i - 1];
    }

    CGLookingForGroup::m_unkBEA820[0].m_id = type;
    CGLookingForGroup::m_unkBEA820[0].m_order = order;
    CGLookingForGroup::ResortLFGLists();
    return 0;
}

// STATIC: 0x00ACD668
static FrameScript_Method s_ScriptFunctions[] = {
    { "SetLFGDungeon", &Script_SetLFGDungeon },
    { "ClearLFGDungeon", &Script_ClearLFGDungeon },
    { "ClearAllLFGDungeons", &Script_ClearAllLFGDungeons },
    { "GetLFGInfoLocal", &Script_GetLFGInfoLocal },
    { "GetLFGInfoServer", &Script_GetLFGInfoServer },
    { "GetLFGQueuedList", &Script_GetLFGQueuedList },
    { "SetLFGComment", &Script_SetLFGComment },
    { "LFGTeleport", &Script_LFGTeleport },
    { "JoinLFG", &Script_JoinLFG },
    { "LeaveLFG", &Script_LeaveLFG },
    { "RefreshLFGList", &Script_RefreshLFGList },
    { "SearchLFGJoin", &Script_SearchLFGJoin },
    { "SearchLFGGetJoinedID", &Script_SearchLFGGetJoinedID },
    { "SearchLFGLeave", &Script_SearchLFGLeave },
    { "SearchLFGGetNumResults", &Script_SearchLFGGetNumResults },
    { "SearchLFGGetResults", &Script_SearchLFGGetResults },
    { "SearchLFGGetEncounterResults", &Script_SearchLFGGetEncounterResults },
    { "SearchLFGGetPartyResults", &Script_SearchLFGGetPartyResults },
    { "SearchLFGSort", &Script_SearchLFGSort },
    { "GetLFGTypes", &Script_GetLFGTypes },
    { "GetLFGRoleUpdate", &Script_GetLFGRoleUpdate },
    { "GetLFGRoleUpdateSlot", &Script_GetLFGRoleUpdateSlot },
    { "GetLFGRoleUpdateMember", &Script_GetLFGRoleUpdateMember },
    { "GetLFGRoles", &Script_GetLFGRoles },
    { "SetLFGRoles", &Script_SetLFGRoles },
    { "CompleteLFGRoleCheck", &Script_CompleteLFGRoleCheck },
    { "GetLFGProposal", &Script_GetLFGProposal },
    { "GetLFGProposalMember", &Script_GetLFGProposalMember },
    { "GetLFGProposalEncounter", &Script_GetLFGProposalEncounter },
    { "AcceptProposal", &Script_AcceptProposal },
    { "RejectProposal", &Script_RejectProposal },
    { "GetLFGBootProposal", &Script_GetLFGBootProposal },
    { "SetLFGBootVote", &Script_SetLFGBootVote },
    { "GetLFGQueueStats", &Script_GetLFGQueueStats },
    { "GetLastQueueStatusIndex", &Script_GetLastQueueStatusIndex },
    { "GetLFGDungeonInfo", &Script_GetLFGDungeonInfo },
    { "GetLFGRandomDungeonInfo", &Script_GetLFGRandomDungeonInfo },
    { "IsLFGDungeonJoinable", &Script_IsLFGDungeonJoinable },
    { "SetLFGHeaderCollapsed", &Script_SetLFGHeaderCollapsed },
    { "SetLFGDungeonEnabled", &Script_SetLFGDungeonEnabled },
    { "GetLFGCompletionReward", &Script_GetLFGCompletionReward },
    { "GetLFGCompletionRewardItem", &Script_GetLFGCompletionRewardItem },
    { "IsInLFGDungeon", &Script_IsInLFGDungeon },
    { "CanPartyLFGBackfill", &Script_CanPartyLFGBackfill },
    { "GetPartyLFGBackfillInfo", &Script_GetPartyLFGBackfillInfo },
    { "PartyLFGStartBackfill", &Script_PartyLFGStartBackfill },
    { "GetLFGRandomCooldownExpiration", &Script_GetLFGRandomCooldownExpiration },
    { "UnitHasLFGRandomCooldown", &Script_UnitHasLFGRandomCooldown },
    { "GetLFGDeserterExpiration", &Script_GetLFGDeserterExpiration },
    { "UnitHasLFGDeserter", &Script_UnitHasLFGDeserter },
    { "GetAvailableRoles", &Script_GetAvailableRoles },
    { "GetLFDChoiceOrder", &Script_GetLFDChoiceOrder },
    { "GetLFDChoiceInfo", &Script_GetLFDChoiceInfo },
    { "GetLFDChoiceCollapseState", &Script_GetLFDChoiceCollapseState },
    { "GetLFDChoiceEnabledState", &Script_GetLFDChoiceEnabledState },
    { "GetLFDChoiceLockedState", &Script_GetLFDChoiceLockedState },
    { "GetLFDLockPlayerCount", &Script_GetLFDLockPlayerCount },
    { "GetLFDLockInfo", &Script_GetLFDLockInfo },
    { "RequestLFDPlayerLockInfo", &Script_RequestLFDPlayerLockInfo },
    { "RequestLFDPartyLockInfo", &Script_RequestLFDPartyLockInfo },
    { "GetNumRandomDungeons", &Script_GetNumRandomDungeons },
    { "GetLFGDungeonRewards", &Script_GetLFGDungeonRewards },
    { "GetLFGDungeonRewardInfo", &Script_GetLFGDungeonRewardInfo },
    { "GetLFGDungeonRewardLink", &Script_GetLFGDungeonRewardLink },
    { "GetRandomDungeonBestChoice", &Script_GetRandomDungeonBestChoice },
    { "GetLFRChoiceOrder", &Script_GetLFRChoiceOrder },
    { "IsListedInLFR", &Script_IsListedInLFR },
};

// VERIFY: cc0f9700 auto
// OFFSET: 0x005531D0
void LookingForGroupRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 12e270a0 auto
// OFFSET: 0x00553200
void LookingForGroupUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
