#include "GuildInfo.h"
#include <cstdint>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/Object/ObjectClient/GuildStats_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include <ctime>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Object/Player.h"


// STATIC: 0x00AD07B8
static GuildRankFlag s_guildRankFlags[17] = {
    { 0x1, 0 },
    { 0x2, 0 },
    { 0x4, 0 },
    { 0x8, 0 },
    { 0x80, 0 },
    { 0x100, 0 },
    { 0x10, 0 },
    { 0x20, 0 },
    { 0x1000, 0 },
    { 0x2000, 0 },
    { 0x4000, 0 },
    { 0x8000, 0 },
    { 0x10000, 0 },
    { 0x20000, 0 },
    { 0x40000, 0 },
    { 0x80000, 0 },
    { 0x100000, 0 },
};

// STATIC: 0x00AD07AC
static uint32_t s_guildBankTabPermissionFlags[3] = { 1, 2, 4 };

// VERIFY: d783a8ca auto
// OFFSET: 0x005CA1B0
static int32_t Script_SetGuildRosterShowOffline(lua_State* L) {
    g_guildShowOfflineCVar->Set(StringToBOOL(L, 1, 1), true, false, false, true);
    return 0;
}

// VERIFY: d633c72c auto
// OFFSET: 0x005CA350
static int32_t Script_GetNumGuildEvents(lua_State* L) {
    lua_pushnumber(L, CGGuildInfo::m_numEventLogEntries);
    return 1;
}

// VERIFY: ca7226df auto
// OFFSET: 0x005CA1E0
static int32_t Script_GetGuildRosterShowOffline(lua_State* L) {
    if (g_guildShowOfflineCVar->m_intValue) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 84ec1a39 manual
// OFFSET: 0x005CA220
static int32_t Script_GuildControlGetNumRanks(lua_State* L) {
    lua_pushnumber(L, CGGuildInfo::m_numRanks);
    return 1;
}

// VERIFY: d2e670c0 manual
// OFFSET: 0x005CA2B0
static int32_t Script_GuildControlSetRankFlag(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GuildControlSetRankFlag(index, enabled)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 17) {
        s_guildRankFlags[index].m_enabled = StringToBOOL(L, 2, 0);
    }

    return 0;
}

// VERIFY: 45ed1e52 manual
// OFFSET: 0x005CB3F0
static int32_t Script_GuildControlDelRank(lua_State* L) {
    if (CGGuildInfo::m_numRanks > 5) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GUILD_DELETE_RANK));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: ca544327 manual
// OFFSET: 0x005CC360
static int32_t Script_CanEditGuildEvent(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x100000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 468bb570 manual
// OFFSET: 0x005CBCF0
static int32_t Script_CanGuildPromote(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x80)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 29e69846 manual
// OFFSET: 0x005CBD80
static int32_t Script_CanGuildDemote(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x100)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 383050bb manual
// OFFSET: 0x005CBE10
static int32_t Script_CanGuildInvite(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x10)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 27f3ef3a manual
// OFFSET: 0x005CBEA0
static int32_t Script_CanGuildRemove(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x20)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 41636616 manual
// OFFSET: 0x005CBF30
static int32_t Script_CanEditMOTD(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x1000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: d8bb8cb1 manual
// OFFSET: 0x005CBFC0
static int32_t Script_CanEditPublicNote(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x2000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 43a3c0dd manual
// OFFSET: 0x005CC050
static int32_t Script_CanEditOfficerNote(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x8000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: dee3d37b manual
// OFFSET: 0x005CC0E0
static int32_t Script_CanViewOfficerNote(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x4000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1491f340 manual
// OFFSET: 0x005CC170
static int32_t Script_CanEditGuildInfo(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x10000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: d13b103b manual
// OFFSET: 0x005CC200
static int32_t Script_CanGuildBankRepair(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid && (CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_rights & 0x40000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9a060c1e manual
// OFFSET: 0x005CC3E0
static int32_t Script_GuildControlGetRankFlags(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t rights = CGGuildInfo::m_rankInfo[s_guildControlRank].m_rights;

    for (uint32_t i = 0; i < 17; i++) {
        if (player && player->m_player->m_playerGuildid && (rights & s_guildRankFlags[i].m_mask)) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }
    }

    return 17;
}

// VERIFY: 40d5d07a auto
// OFFSET: 0x005CB6A0
static int32_t Script_SetGuildBankWithdrawLimit(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: Script_SetGuildBankWithdrawLimit(amount)");
        return 0;
    }

    uint32_t amount = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (amount > 1000000000) {
        amount = 1000000000;
    }

    CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankMoneyPerDay = amount;
    return 0;
}

// VERIFY: c31fa032 manual
// OFFSET: 0x005CB700
static int32_t Script_GetGuildBankWithdrawLimit(lua_State* L) {
    int32_t limit = CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankMoneyPerDay;

    if (limit >= 0) {
        limit /= 10000;
    }

    lua_pushnumber(L, limit);
    return 1;
}

// VERIFY: 627eac9b manual
// OFFSET: 0x005CB760
static int32_t Script_SetGuildBankTabWithdraw(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetGuildBankTabWithdraw(tab, amount)");
        return 0;
    }

    uint32_t tab = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t amount = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (amount > 100000) {
        amount = 100000;
    }

    CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabStacksPerDay[tab] = amount;
    return 0;
}

// VERIFY: 7598cb74 manual
// OFFSET: 0x005CC470
static int32_t Script_QueryGuildEventLog(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_GUILD_EVENT_LOG_QUERY));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: ab016c69 manual
// OFFSET: 0x005CC290
static int32_t Script_CanEditGuildTabInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CanEditGuildTabInfo(tab)");
        return 0;
    }

    uint32_t tab = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (tab < 6 ? CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank].m_bankTabRights[tab] : 0) & 4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: d0e6889f manual
// OFFSET: 0x005CB470
static int32_t Script_SetGuildBankTabPermissions(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetGuildTabPermissions(tab, index, enabled)");
        return 0;
    }

    uint8_t tab = static_cast<uint8_t>(lua_tonumber(L, 1)) - 1;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    int32_t enabled = StringToBOOL(L, 3, 0);
    uint32_t rights = tab < 6 ? CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabRights[tab] : 0;
    uint32_t flag = s_guildBankTabPermissionFlags[index];

    if (enabled) {
        rights |= flag;
    } else {
        rights &= ~flag;
    }

    CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabRights[tab] = rights;
    return 0;
}

// VERIFY: 5e321988 auto
// OFFSET: 0x005CAF40
static int32_t Script_SetGuildRosterSelection(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetGuildRosterSelection(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < CGGuildInfo::m_numMembers) {
        CGGuildInfo::m_selectedMember = CGGuildInfo::m_roster.m_data[index]->m_guid;
        return 0;
    }

    CGGuildInfo::m_selectedMember.m_guid = 0;
    return 0;
}

// VERIFY: c3aaa605 auto
// OFFSET: 0x005CA190
static int32_t Script_GetGuildRosterMOTD(lua_State* L) {
    lua_pushstring(L, CGGuildInfo::m_motd);
    return 1;
}

// VERIFY: 9075d012 auto
// OFFSET: 0x005CB810
int32_t Script_GuildRoster(lua_State* L) {
    CGGuildInfo::QueryRosterAndRanks();

    return 0;
}

// VERIFY: c8a5453a manual
// OFFSET: 0x005CA130
static int32_t Script_GetNumGuildMembers(lua_State* L) {
    int32_t all = StringToBOOL(L, 1, 0);
    lua_pushnumber(L, CGGuildInfo::m_showOffline || all ? CGGuildInfo::m_numMembers : CGGuildInfo::m_numOnlineMembers);
    return 1;
}

// VERIFY: 59f8e8c8 auto
// OFFSET: 0x005CAFD0
static int32_t Script_GetGuildRosterSelection(lua_State* L) {
    lua_pushnumber(L, CGGuildInfo::GetSelectedMemberIndex() + 1);
    return 1;
}

// VERIFY: 50073971 manual
// OFFSET: 0x005CC9C0
static int32_t Script_GetGuildRosterInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildRosterInfo(index)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    GuildStats_C* stats;
    GuildMemberInfo* member;
    uint32_t index;

    if (!player || !(stats = g_guildInfoCache.GetRecord(player->m_player->m_playerGuildid, player->m_obj->m_guid, GuildInfoCacheCallback, nullptr, false)) || (index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1) >= CGGuildInfo::m_numMembers || !(member = CGGuildInfo::m_roster.m_data[index])) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "");
        lua_pushnil(L);
        return 11;
    }

    lua_pushstring(L, member->m_name);
    char rankName[64];
    SStrCopy(rankName, stats->m_rankNames[member->m_rank], STORM_MAX_STR);
    lua_pushstring(L, rankName);
    lua_pushnumber(L, member->m_rank);
    lua_pushnumber(L, member->m_level);
    ChrClassesRec* chrClass = g_chrClassesDB.GetRecord(member->m_class);
    const char* className = CGUnit_C::GetDisplayClassNameFromRecord(chrClass, member->m_gender, nullptr);

    if (className) {
        lua_pushstring(L, className);
    } else {
        lua_pushnil(L);
    }

    AreaTableRec* area = g_areaTableDB.GetRecord(member->m_zoneID);

    if (area) {
        lua_pushstring(L, area->m_areaName);
    } else {
        lua_pushnil(L);
    }

    if (member->unk_0051) {
        member->unk_0051 = 0;
    }

    lua_pushstring(L, member->m_note);

    if (member->unk_0051) {
        member->unk_0051 = 0;
    }

    lua_pushstring(L, member->m_officerNote);

    if (member->m_online) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (member->m_status & 0x4) {
        lua_pushstring(L, FrameScript_GetText("CHAT_FLAG_DND", -1, GENDER_NOT_APPLICABLE));
    } else if (member->m_status & 0x2) {
        lua_pushstring(L, FrameScript_GetText("CHAT_FLAG_AFK", -1, GENDER_NOT_APPLICABLE));
    } else {
        lua_pushstring(L, "");
    }

    if (chrClass) {
        lua_pushstring(L, chrClass->m_filename);
    } else {
        lua_pushnil(L);
    }

    return 11;
}

// VERIFY: ee3ac040 manual
// OFFSET: 0x005CA090
static void SetGuildControlRank(uint32_t rank) {
    if (rank > 16) {
        s_guildControlRank = -1;
        return;
    }

    uint32_t rights = CGGuildInfo::m_rankInfo[rank].m_rights;
    s_guildControlRank = rank;

    for (uint32_t i = 0; i < 17; i++) {
        s_guildRankFlags[i].m_enabled = rights & s_guildRankFlags[i].m_mask;
    }
}

// VERIFY: 723ba7a7 auto
// OFFSET: 0x005CA260
static int32_t Script_GuildControlSetRank(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GuildControlSetRank(rank)");
        return 0;
    }

    SetGuildControlRank(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: b9372943 auto
// OFFSET: 0x005CA330
static int32_t Script_GetGuildInfoText(lua_State* L) {
    lua_pushstring(L, CGGuildInfo::m_infoText);
    return 1;
}

// VERIFY: 4d6f79ce auto
// OFFSET: 0x005CABB0
static int32_t Script_GetGuildRosterLastOnline(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildRosterLastOnline(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    GuildMemberInfo* member;

    if (index >= CGGuildInfo::m_numMembers || !(member = CGGuildInfo::m_roster.m_data[index]) || member->m_online) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 4;
    }

    float time = member->m_lastOnline;
    int32_t years = static_cast<int32_t>(time * 0.0027397261f);
    time -= years * 365.0f;
    int32_t months = static_cast<int32_t>(time * 0.033333335f);
    time -= months * 30.0f;
    int32_t days = static_cast<int32_t>(time);
    lua_pushnumber(L, years);
    lua_pushnumber(L, months);
    lua_pushnumber(L, days);
    lua_pushnumber(L, static_cast<int32_t>((time - days) * 24.0f));
    return 4;
}

// VERIFY: 8f2f2c14 auto
// OFFSET: 0x005CAD00
static int32_t Script_GuildRosterSetPublicNote(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: GuildRosterSetPublicNote(index, note)");
        return 0;
    }

    const char* note = lua_tolstring(L, 2, nullptr);
    char scrubbed[128];
    ScrubString(note, scrubbed, 32, sizeof(scrubbed), 1);
    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGGuildInfo::m_numMembers) {
        return 0;
    }

    GuildMemberInfo* member = CGGuildInfo::m_roster.m_data[index];

    if (!member || !note) {
        return 0;
    }

    if (member->unk_0051) {
        member->unk_0051 = 0;
    }

    if (SStrCmp(member->m_note, note, STORM_MAX_STR)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GUILD_SET_PUBLIC_NOTE));
        msg.PutString(member->m_name);
        msg.PutString(scrubbed);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: e9f385cb auto
// OFFSET: 0x005CAE20
static int32_t Script_GuildRosterSetOfficerNote(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: GuildRosterSetOfficerNote(index, note)");
        return 0;
    }

    const char* note = lua_tolstring(L, 2, nullptr);
    char scrubbed[128];
    ScrubString(note, scrubbed, 32, sizeof(scrubbed), 1);
    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGGuildInfo::m_numMembers) {
        return 0;
    }

    GuildMemberInfo* member = CGGuildInfo::m_roster.m_data[index];

    if (!member || !note) {
        return 0;
    }

    if (member->unk_0051) {
        member->unk_0051 = 0;
    }

    if (SStrCmp(member->m_officerNote, note, STORM_MAX_STR)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GUILD_SET_OFFICER_NOTE));
        msg.PutString(member->m_name);
        msg.PutString(scrubbed);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: c8bffae2 auto
// OFFSET: 0x005CB000
static int32_t Script_SortGuildRoster(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SortGuildRoster(\"type\")");
        return 0;
    }

    int32_t column = 2;
    const char* type = lua_tolstring(L, 1, nullptr);

    if (SStrCmpI(type, "name", STORM_MAX_STR)) {
        if (!SStrCmpI(type, "level", STORM_MAX_STR)) {
            CGGuildInfo::SortRoster(1);
            return 0;
        }

        if (!SStrCmpI(type, "class", STORM_MAX_STR)) {
            CGGuildInfo::SortRoster(4);
            return 0;
        }

        if (!SStrCmpI(type, "group", STORM_MAX_STR)) {
            CGGuildInfo::SortRoster(5);
            return 0;
        }

        if (!SStrCmpI(type, "rank", STORM_MAX_STR)) {
            CGGuildInfo::SortRoster(0);
            return 0;
        }

        if (!SStrCmpI(type, "zone", STORM_MAX_STR)) {
            CGGuildInfo::SortRoster(3);
            return 0;
        }

        if (!SStrCmpI(type, "online", STORM_MAX_STR)) {
            CGGuildInfo::SortRoster(6);
            return 0;
        }

        if (!SStrCmpI(type, "note", STORM_MAX_STR)) {
            column = 7;
        }
    }

    CGGuildInfo::SortRoster(column);
    return 0;
}

// VERIFY: 2a14de91 manual
// OFFSET: 0x005CB170
static int32_t Script_GuildControlSaveRank(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GuildControlSaveRank(name)");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (!name || !*name) {
        return 0;
    }

    char rankName[64];
    ScrubString(name, rankName, 16, sizeof(rankName), 1);
    int32_t rank = s_guildControlRank;

    if (static_cast<uint32_t>(rank) >= CGGuildInfo::m_numRanks) {
        return 0;
    }

    uint32_t rights = CGGuildInfo::m_rankInfo[rank].m_rights;

    for (uint32_t i = 0; i < 17; i++) {
        if (s_guildRankFlags[i].m_enabled) {
            rights |= s_guildRankFlags[i].m_mask;
        } else {
            rights &= ~s_guildRankFlags[i].m_mask;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_SET_RANK_PERMISSIONS));
    msg.Put(static_cast<uint32_t>(rank));
    msg.Put(rights);
    msg.PutString(rankName);
    msg.Put(static_cast<uint32_t>(10000 * CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankMoneyPerDay));

    for (uint32_t i = 0; i < 6; i++) {
        msg.Put(CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabRights[i]);
        msg.Put(static_cast<uint32_t>(CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabStacksPerDay[i]));
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 530716a0 auto
// OFFSET: 0x005CB330
static int32_t Script_GuildControlAddRank(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GuildControlAddRank(name)");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (!name || !*name) {
        return 0;
    }

    char rankName[64];
    ScrubString(name, rankName, 16, sizeof(rankName), 1);

    if (CGGuildInfo::m_numRanks < 10) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GUILD_ADD_RANK));
        msg.PutString(rankName);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: e5f9ce69 manual
// OFFSET: 0x005CB560
static int32_t Script_GetGuildBankTabPermissions(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildBankTabPermissions(tab)");
        return 0;
    }

    uint8_t tab = static_cast<uint8_t>(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    uint32_t rights = tab < 6 ? CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabRights[tab] : 0;

    if (rights & 0x1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (rights & 0x2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (rights & 0x4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, tab < 6 ? CGGuildInfo::m_rankInfo[s_guildControlRank].m_bankTabStacksPerDay[tab] : 0);
    return 4;
}

// VERIFY: 8b3ec56c manual
// OFFSET: 0x005CB820
static int32_t Script_SetGuildInfoText(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetGuildInfoText(text)");
        return 0;
    }

    const char* text = lua_tolstring(L, 1, nullptr);
    char scrubbed[2004];
    scrubbed[0] = '\0';

    if (text && *text) {
        ScrubString(text, scrubbed, 501, sizeof(scrubbed), 0);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_INFO_TEXT));
    msg.PutString(scrubbed);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 1f5aef90 manual
// OFFSET: 0x005CCC90
static int32_t Script_GuildControlGetRankName(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GuildControlGetRankName(index)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    char name[64] = { 0 };
    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (player && index < CGGuildInfo::m_numRanks) {
        GuildStats_C* stats = g_guildInfoCache.GetRecord(player->m_player->m_playerGuildid, player->m_obj->m_guid, GuildInfoCacheCallback, nullptr, false);

        if (stats) {
            SStrCopy(name, stats->m_rankNames[index], STORM_MAX_STR);
            TruncateNewLines(name);
        }
    }

    lua_pushstring(L, name);
    return 1;
}

// VERIFY: 791632e5 manual
// OFFSET: 0x005CCD90
static int32_t Script_GetGuildEventInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildEventInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    GuildEventLogEntry* entry;

    if (index >= static_cast<uint32_t>(CGGuildInfo::m_numEventLogEntries) || !(entry = &CGGuildInfo::m_eventLog[index])) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 8;
    }

    char type[16];

    switch (entry->m_type) {
        case 1:
            SStrCopy(type, "invite", STORM_MAX_STR);
            break;

        case 2:
            SStrCopy(type, "join", STORM_MAX_STR);
            break;

        case 3:
            SStrCopy(type, "promote", STORM_MAX_STR);
            break;

        case 4:
            SStrCopy(type, "demote", STORM_MAX_STR);
            break;

        case 5:
            SStrCopy(type, "remove", STORM_MAX_STR);
            break;

        case 6:
            SStrCopy(type, "quit", STORM_MAX_STR);
            break;

        default:
            SStrCopy(type, "none", STORM_MAX_STR);
            break;
    }

    lua_pushstring(L, type);
    WOWGUID none = WOWGUID();
    NameCache* player1 = g_nameDBCache.GetRecord(entry->m_player1.m_guid, none, nullptr, nullptr, false);

    if (player1) {
        lua_pushstring(L, player1->m_name);
    } else {
        lua_pushnil(L);
    }

    none = WOWGUID();
    NameCache* player2 = g_nameDBCache.GetRecord(entry->m_player2.m_guid, none, nullptr, nullptr, false);

    if (player2) {
        lua_pushstring(L, player2->m_name);
    } else {
        lua_pushnil(L);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && entry->m_rank < CGGuildInfo::m_numRanks) {
        GuildStats_C* stats = g_guildInfoCache.GetRecord(player->m_player->m_playerGuildid, player->m_obj->m_guid, GuildInfoCacheCallback, nullptr, false);

        if (stats) {
            char rankName[64];
            SStrCopy(rankName, stats->m_rankNames[entry->m_rank], STORM_MAX_STR);
            lua_pushstring(L, rankName);
        } else {
            lua_pushnil(L);
        }
    }

    int32_t hours = (static_cast<int32_t>(time(nullptr)) - static_cast<int32_t>(entry->m_time)) / 3600;
    int32_t years = hours / 8760;
    int32_t remainder = hours % 8760;
    int32_t months = remainder / 720;
    remainder %= 720;
    lua_pushnumber(L, years);
    lua_pushnumber(L, months);
    lua_pushnumber(L, remainder / 24);
    lua_pushnumber(L, remainder % 24);
    return 8;
}

// VERIFY: 6874103e auto
// OFFSET: 0x008E5250
static int32_t Script_CloseGuildRoster(lua_State* L) {
    return 0;
}

// STATIC: 0x00AD0840
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumGuildMembers", &Script_GetNumGuildMembers },
    { "GetGuildRosterMOTD", &Script_GetGuildRosterMOTD },
    { "GetGuildRosterInfo", &Script_GetGuildRosterInfo },
    { "GetGuildRosterLastOnline", &Script_GetGuildRosterLastOnline },
    { "GuildRosterSetPublicNote", &Script_GuildRosterSetPublicNote },
    { "GuildRosterSetOfficerNote", &Script_GuildRosterSetOfficerNote },
    { "SetGuildRosterSelection", &Script_SetGuildRosterSelection },
    { "GetGuildRosterSelection", &Script_GetGuildRosterSelection },
    { "CanGuildPromote", &Script_CanGuildPromote },
    { "CanGuildDemote", &Script_CanGuildDemote },
    { "CanGuildInvite", &Script_CanGuildInvite },
    { "CanGuildRemove", &Script_CanGuildRemove },
    { "CanEditMOTD", &Script_CanEditMOTD },
    { "CanEditPublicNote", &Script_CanEditPublicNote },
    { "CanEditOfficerNote", &Script_CanEditOfficerNote },
    { "CanViewOfficerNote", &Script_CanViewOfficerNote },
    { "CanEditGuildInfo", &Script_CanEditGuildInfo },
    { "CanGuildBankRepair", &Script_CanGuildBankRepair },
    { "CanEditGuildTabInfo", &Script_CanEditGuildTabInfo },
    { "CanEditGuildEvent", &Script_CanEditGuildEvent },
    { "SortGuildRoster", &Script_SortGuildRoster },
    { "SetGuildRosterShowOffline", &Script_SetGuildRosterShowOffline },
    { "GetGuildRosterShowOffline", &Script_GetGuildRosterShowOffline },
    { "GuildControlGetNumRanks", &Script_GuildControlGetNumRanks },
    { "GuildControlGetRankName", &Script_GuildControlGetRankName },
    { "GuildControlSetRank", &Script_GuildControlSetRank },
    { "GuildControlGetRankFlags", &Script_GuildControlGetRankFlags },
    { "GuildControlSetRankFlag", &Script_GuildControlSetRankFlag },
    { "GuildControlSaveRank", &Script_GuildControlSaveRank },
    { "GuildControlAddRank", &Script_GuildControlAddRank },
    { "GuildControlDelRank", &Script_GuildControlDelRank },
    { "SetGuildBankTabPermissions", &Script_SetGuildBankTabPermissions },
    { "GetGuildBankTabPermissions", &Script_GetGuildBankTabPermissions },
    { "SetGuildBankWithdrawLimit", &Script_SetGuildBankWithdrawLimit },
    { "GetGuildBankWithdrawLimit", &Script_GetGuildBankWithdrawLimit },
    { "SetGuildBankTabWithdraw", &Script_SetGuildBankTabWithdraw },
    { "CloseGuildRoster", &Script_CloseGuildRoster },
    { "GuildRoster", &Script_GuildRoster },
    { "GetGuildInfoText", &Script_GetGuildInfoText },
    { "SetGuildInfoText", &Script_SetGuildInfoText },
    { "QueryGuildEventLog", &Script_QueryGuildEventLog },
    { "GetNumGuildEvents", &Script_GetNumGuildEvents },
    { "GetGuildEventInfo", &Script_GetGuildEventInfo },
};

// VERIFY: 361a345a auto
// OFFSET: 0x005CA380
void GuildInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 74512400 auto
// OFFSET: 0x005CA3B0
void GuildInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
