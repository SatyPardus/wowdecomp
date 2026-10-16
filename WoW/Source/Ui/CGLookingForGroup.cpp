#include "LFGInfo.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Os/Time.h"
#include "WoW/Source/DB/Rec/DungeonEncounterRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/Ui/LFGInfo.h"
#include <cstdlib>
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"

// STATIC: 0x00A394B0
static const uint8_t s_classRoles[11] = { 0x0B, 0x0F, 0x09, 0x09, 0x0D, 0x0B, 0x0D, 0x09, 0x09, 0x00, 0x0F };

// STATIC: 0x00ACDD6C
static int32_t s_joinAttempts;

// STATIC: 0x00BEABC4
static float s_lastJoinTime;

// VERIFY: 7da483d6 auto
// OFFSET: 0x00552940
uint8_t CGLookingForGroup::GetClassRoles(uint32_t classID) {
    return s_classRoles[classID - 1];
}

// VERIFY: 29999d79 manual
// OFFSET: 0x00555920
void CGLookingForGroup::RequestLFGStatus() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_DF_GET_JOIN_STATUS));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 4918a5f7 manual
// OFFSET: 0x00555990
void CGLookingForGroup::RequestLFGPlayerInfo() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_PLAYER_LOCK_INFO_REQUEST));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 40d9d329 auto
// OFFSET: 0x00557240
void CGLookingForGroup::EnterWorld() {
    CGLookingForGroup::m_lastProposalID = 0;
    CGLookingForGroup::RequestLFGStatus();
    CGLookingForGroup::RequestLFGPlayerInfo();
}

// VERIFY: 62f8faf7 manual
// OFFSET: 0x00553830
void DungeonEncounterCountMatchingRows(int32_t mapId, int32_t difficulty, uint32_t completedMask, uint32_t* total, uint32_t* completed) {
    *total = 0;
    *completed = 0;

    for (int32_t i = 0; i < g_dungeonEncounterDB.m_numRecords; i++) {
        DungeonEncounterRec* encounter = g_dungeonEncounterDB.GetRecordByIndex(i);

        if (encounter->m_mapID == mapId && encounter->m_difficulty == difficulty) {
            ++*total;

            if ((1 << encounter->m_bit) & completedMask) {
                ++*completed;
            }
        }
    }
}

// VERIFY: e291b497 auto
// OFFSET: 0x005538B0
int32_t PushDungeonEncounterInfo(lua_State* L, int32_t mapId, int32_t difficulty, int32_t index, uint32_t completedMask) {
    for (int32_t i = 0; i < g_dungeonEncounterDB.m_numRecords; i++) {
        DungeonEncounterRec* encounter = g_dungeonEncounterDB.GetRecordByIndex(i);

        if (encounter->m_mapID != mapId || encounter->m_difficulty != difficulty) {
            continue;
        }

        if (index) {
            index--;
            continue;
        }

        lua_pushstring(L, encounter->m_name);
        SpellIconRec* icon;

        if (encounter->m_spellIconID && (icon = g_spellIconDB.GetRecord(encounter->m_spellIconID)) && icon->m_textureFilename) {
            lua_pushstring(L, icon->m_textureFilename);
        } else {
            lua_pushnil(L);
        }

        lua_pushboolean(L, (completedMask & (1 << encounter->m_bit)) != 0);
        return 3;
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

// VERIFY: f5aceb4b auto
// OFFSET: 0x00553B50
uint32_t FilterRolesByClass(uint8_t roles) {
    ChrClassesRec* rec = g_chrClassesDB.GetRecord(ClientServices::GetCharacterClass());
    return roles & s_classRoles[rec->m_id - 1];
}

// VERIFY: 49ccd8f7 manual
// OFFSET: 0x00554610
int32_t CGLookingForGroup::GetLFGRoleUpdate(lua_State* L) {
    lua_pushboolean(L, CGLookingForGroup::m_roleCheckState == 2);
    lua_pushnumber(L, CGLookingForGroup::m_roleCheckSlots.m_count);
    lua_pushnumber(L, CGLookingForGroup::m_roleCheckMembers.m_count);
    return 3;
}

// VERIFY: 5c5209c5 auto
// OFFSET: 0x00554690
int32_t CGLookingForGroup::GetLFGRoleUpdateSlot(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGRoleUpdateSlot(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= static_cast<int32_t>(CGLookingForGroup::m_roleCheckSlots.m_count)) {
        return 0;
    }

    lua_pushnumber(L, CGLookingForGroup::m_roleCheckSlots.m_data[index] >> 24);
    lua_pushnumber(L, CGLookingForGroup::m_roleCheckSlots.m_data[index] & 0xFFFFFF);
    return 2;
}

// VERIFY: 49637f08 manual
// OFFSET: 0x005572D0
void CGLookingForGroup::JoinLFG() {
    if ((CGLookingForGroup::m_localInfo.m_joined && !CGLookingForGroup::m_localInfo.m_dirty) || !s_cvLfgSelectedRoles) {
        return;
    }

    if (++s_joinAttempts > 2) {
        float now = OsGetAsyncTimeSec();

        if (now - s_lastJoinTime < 10.0f) {
            if (s_joinAttempts > 2) {
                return;
            }
        } else {
            s_lastJoinTime = now;
            s_joinAttempts = 0;
        }
    }

    CDataStore msg;
    uint32_t roles = FilterRolesByClass(static_cast<uint8_t>(s_cvLfgSelectedRoles->m_intValue));

    if (!(roles & ~0x1)) {
        return;
    }

    msg.Put(static_cast<uint32_t>(CMSG_LFG_JOIN));
    msg.Put(roles);
    msg.Put(static_cast<uint8_t>(CGLookingForGroup::m_localInfo.m_noPartial != 0));
    msg.Put(static_cast<uint8_t>(CGLookingForGroup::m_localInfo.m_achievements != 0));
    msg.Put(static_cast<uint8_t>(CGLookingForGroup::m_localInfo.m_numDungeons));

    for (LFGDungeonEntry* dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Head(); dungeon; dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Next(dungeon)) {
        msg.Put(dungeon->m_slot);
    }

    msg.Put(static_cast<uint8_t>(3));

    for (uint32_t i = 0; i < 3; i++) {
        msg.Put(CGLookingForGroup::m_localInfo.m_roles[i]);
    }

    msg.PutString(CGLookingForGroup::m_localInfo.m_comment);
    msg.Finalize();
    CGLookingForGroup::m_localInfo.m_joined = 1;
    CGLookingForGroup::m_localInfo.m_dirty = 0;
    ClientServices::Send2(&msg);
}

// STATIC: 0x00AD87B0
static const char* s_roleTokens[3] = { "TANK", "HEALER", "DAMAGER" };

// STATIC: 0x00ACDD58
static int32_t s_proposalResponseCount = 2;

// STATIC: 0x00BEABB0
static float s_proposalResponseTime;

// STATIC: 0x00ACDD5C
static int32_t s_bootVoteCount = 2;

// STATIC: 0x00BEABB4
static float s_bootVoteTime;

// STATIC: 0x00BEAA0C
static uint32_t s_lastQueueSlot;

// VERIFY: f9311e78 manual
// OFFSET: 0x00553AB0
void CGLookingForGroup::LeaveLFG() {
    if (!CGLookingForGroup::m_localInfo.m_joined) {
        LFGInfo* info = (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers) ? &CGLookingForGroup::m_partyInfo : &CGLookingForGroup::m_playerInfo;

        if (!info->m_joined) {
            return;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_LEAVE));
    msg.Finalize();
    CGLookingForGroup::m_localInfo.m_joined = 0;
    CGLookingForGroup::m_localInfo.m_dirty = 0;
    ClientServices::Send2(&msg);
}

// VERIFY: 2d5334cf manual
// OFFSET: 0x00553000
int32_t CGLookingForGroup::GetLFGProposalMember(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGProposalMember(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (!CGLookingForGroup::m_proposal.m_dungeonId || index > 4) {
        return 0;
    }

    LFGProposalMember* member = &CGLookingForGroup::m_proposal.m_members[index];
    lua_pushboolean(L, member->m_roles & 0x1);

    if (member->m_roles & 0x2) {
        lua_pushstring(L, s_roleTokens[0]);
    } else if (member->m_roles & 0x4) {
        lua_pushstring(L, s_roleTokens[1]);
    } else if (member->m_roles & 0x8) {
        lua_pushstring(L, s_roleTokens[2]);
    } else {
        lua_pushstring(L, "UNKNOWN");
    }

    lua_pushnumber(L, 0.0);
    lua_pushboolean(L, member->m_answered);
    lua_pushboolean(L, member->m_isAccepted);
    lua_pushnil(L);
    lua_pushnil(L);
    return 7;
}

// VERIFY: 71caab52 auto
// OFFSET: 0x00554380
int32_t CGLookingForGroup::GetLFGProposalEncounter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGProposalEncounter(index)");
        return 0;
    }

    double index = lua_tonumber(L, 1);
    LfgDungeonsRec* rec;

    if (CGLookingForGroup::m_proposal.m_dungeonId && (rec = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_proposal.m_dungeonId & 0xFFFFFF))) {
        return PushDungeonEncounterInfo(L, rec->m_mapID, rec->m_difficulty, static_cast<int32_t>(index - 1.0), CGLookingForGroup::m_proposal.m_completedMask);
    }

    return PushDungeonEncounterInfo(L, 0, 0, 0, 0);
}

// VERIFY: 0ad38c35 manual
// OFFSET: 0x00554450
void CGLookingForGroup::ProposalResponse(bool accept) {
    if (!CGLookingForGroup::m_proposal.m_dungeonId) {
        return;
    }

    if (++s_proposalResponseCount > 2) {
        float now = OsGetAsyncTimeSec();

        if (now - s_proposalResponseTime < 10.0) {
            if (s_proposalResponseCount > 2) {
                return;
            }
        } else {
            s_proposalResponseTime = now;
            s_proposalResponseCount = 0;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_PROPOSAL_RESULT));
    msg.Put(CGLookingForGroup::m_proposal.m_proposalId);
    msg.Put(static_cast<uint8_t>(accept));
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (accept) {
        for (uint32_t i = 0; i < 5; i++) {
            if (CGLookingForGroup::m_proposal.m_members[i].m_isLocal) {
                CGLookingForGroup::m_proposal.m_members[i].m_answered = 1;
                CGLookingForGroup::m_proposal.m_members[i].m_isAccepted = 1;
            }
        }
    } else {
        CGLookingForGroup::m_proposal.Clear();
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_UPDATE, nullptr);
}

// VERIFY: cb5ab203 auto
// OFFSET: 0x00554750
int32_t CGLookingForGroup::GetLFGRoleUpdateMember(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLFGRoleUpdateMember(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= static_cast<int32_t>(CGLookingForGroup::m_roleCheckMembers.m_count)) {
        return 0;
    }

    LFGRoleCheckMember* member = &CGLookingForGroup::m_roleCheckMembers.m_data[index];
    lua_pushboolean(L, member->m_confirmed);
    lua_pushnumber(L, member->m_level);
    NameCache* name = g_nameDBCache.GetRecord(member->m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);

    if (name) {
        lua_pushstring(L, name->m_name);
        const char* className = CGUnit_C::GetDisplayClassName(nullptr, name);
        lua_pushstring(L, className ? className : "");
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    if (member->m_confirmed) {
        lua_pushboolean(L, member->m_roles & 0x1);
        lua_pushboolean(L, member->m_roles & 0x2);
        lua_pushboolean(L, member->m_roles & 0x4);
        lua_pushboolean(L, member->m_roles & 0x8);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 8;
}

// VERIFY: 0386d671 manual
// OFFSET: 0x00554960
int32_t CGLookingForGroup::SetLFGBootVote(lua_State* L) {
    bool vote = StringToBOOL(L, 1, 0) != 0;

    if (!CGLookingForGroup::m_bootProposal.m_inProgress) {
        return 0;
    }

    if (++s_bootVoteCount > 2) {
        float now = OsGetAsyncTimeSec();

        if (now - s_bootVoteTime < 10.0) {
            if (s_bootVoteCount > 2) {
                return 0;
            }
        } else {
            s_bootVoteTime = now;
            s_bootVoteCount = 0;
        }
    }

    CGLookingForGroup::m_bootProposal.m_myVote = vote;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_SET_BOOT_VOTE));
    msg.Put(static_cast<uint8_t>(vote));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 142d94de manual
// OFFSET: 0x00554A60
int32_t CGLookingForGroup::GetLFGBootProposal(lua_State* L) {
    lua_pushboolean(L, CGLookingForGroup::m_bootProposal.m_inProgress);

    if (!CGLookingForGroup::m_bootProposal.m_inProgress) {
        return 1;
    }

    lua_pushboolean(L, CGLookingForGroup::m_bootProposal.m_didVote);
    lua_pushboolean(L, CGLookingForGroup::m_bootProposal.m_myVote);
    NameCache* name = g_nameDBCache.GetRecord(CGLookingForGroup::m_bootProposal.m_target.m_guid, WOWGUID(), nullptr, nullptr, false);

    if (name) {
        char fullName[256];

        if (name->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
        } else {
            SStrCopy(fullName, name->m_name, sizeof(fullName));
        }

        lua_pushstring(L, fullName);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, CGLookingForGroup::m_bootProposal.m_totalVotes);
    lua_pushnumber(L, CGLookingForGroup::m_bootProposal.m_bootVotes);
    lua_pushnumber(L, CGLookingForGroup::m_bootProposal.m_timeLeft);
    ProfanityFilter(CGLookingForGroup::m_bootProposal.m_reason, false, false);
    lua_pushstring(L, CGLookingForGroup::m_bootProposal.m_reason);
    return 8;
}

// VERIFY: c482e6b9 auto
// OFFSET: 0x00555F30
bool CGLookingForGroup::SetHeaderCollapsed(uint32_t header, bool collapsed) {
    if (!s_cvLfdCollapsedHeaders) {
        return false;
    }

    CVarArrayWrapper wrapper(s_cvLfdCollapsedHeaders);
    return wrapper.RawSetValue(header, collapsed != wrapper.m_defaultValue);
}

// VERIFY: 4c9a2a42 auto
// OFFSET: 0x00555F80
bool CGLookingForGroup::SetDungeonEnabled(uint32_t dungeon, bool enabled) {
    if (!s_cvLfdSelectedDungeons) {
        return false;
    }

    CVarArrayWrapper wrapper(s_cvLfdSelectedDungeons);
    return wrapper.RawSetValue(dungeon, enabled != wrapper.m_defaultValue);
}

// VERIFY: 60bc3b0b manual
// OFFSET: 0x00558ED0
void CGLookingForGroup::JoinLFGSearch(int32_t lfgID) {
    if (CGLookingForGroup::m_searchJoinedID == lfgID) {
        return;
    }

    CGLookingForGroup::m_prevSearchResults.Clear();
    CGLookingForGroup::m_prevSearchResults.m_time = OsGetAsyncTimeMs();
    CGLookingForGroup::m_searchJoinedID = lfgID;
    CGLookingForGroup::m_searchResults.Clear();
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_LFR_JOIN));
    msg.Put(static_cast<uint32_t>(lfgID));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: b3b222bd manual
// OFFSET: 0x005593A0
void CGLookingForGroup::ClearLFGDungeon(uint32_t slot) {
    for (LFGDungeonEntry* entry = CGLookingForGroup::m_localInfo.m_dungeons.Head(); entry; entry = CGLookingForGroup::m_localInfo.m_dungeons.Next(entry)) {
        if (entry->m_slot == slot) {
            delete entry;
            CGLookingForGroup::m_localInfo.m_numDungeons--;
            CGLookingForGroup::m_localInfo.m_dirty = 1;
            return;
        }
    }
}

// VERIFY: 35836964 manual
// OFFSET: 0x00559C40
int32_t CGLookingForGroup::GetQueueStatusIndex() {
    LFGInfo* info = (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers) ? &CGLookingForGroup::m_partyInfo : &CGLookingForGroup::m_playerInfo;
    int32_t index = 0;

    for (LFGDungeonEntry* entry = info->m_dungeons.Head(); entry; entry = info->m_dungeons.Next(entry)) {
        if (entry->m_slot == s_lastQueueSlot) {
            return index;
        }

        index++;
    }

    return -1;
}

// VERIFY: f4faa7f6 manual
// OFFSET: 0x0055B120
void CGLookingForGroup::ClearPartyInfo() {
    CGLookingForGroup::m_partyLockInfo.Clear();

    if (CGLookingForGroup::m_roleCheckState == 2) {
        CGLookingForGroup::m_roleCheckState = 0;
        CGLookingForGroup::m_roleCheckSlots.m_count = 0;
        CGLookingForGroup::m_roleCheckMembers.m_count = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_HIDE, nullptr);
    }

    if (CGLookingForGroup::m_proposal.m_dungeonId) {
        CGLookingForGroup::m_proposal.Clear();
        FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_FAILED, nullptr);
    }
}

// VERIFY: d3c6c263 manual
// OFFSET: 0x00555A70
static int QSortLFGParty(const void* a, const void* b) {
    LFGListParty* partyA = *static_cast<LFGListParty* const*>(a);
    LFGListParty* partyB = *static_cast<LFGListParty* const*>(b);
    LFGListPlayer* leaderA = partyA->m_members.m_count ? partyA->m_members.m_data[0] : nullptr;
    LFGListPlayer* leaderB = partyB->m_members.m_count ? partyB->m_members.m_data[0] : nullptr;

    if (!leaderA) {
        return leaderB ? -1 : 0;
    }

    if (!leaderB) {
        return 1;
    }

    for (uint32_t i = 0; i < 7; i++) {
        int32_t result = 0;

        switch (CGLookingForGroup::m_unkBEA820[i].m_id) {
            case 0: {
                AreaTableRec* areaA = g_areaTableDB.GetRecord(leaderA->m_areaID);
                AreaTableRec* areaB = g_areaTableDB.GetRecord(leaderB->m_areaID);

                if (areaA && areaB) {
                    result = SStrSortUTF8I(areaA->m_areaName, areaB->m_areaName, STORM_MAX_STR);
                }

                break;
            }

            case 1:
                if (leaderA->m_level != leaderB->m_level) {
                    result = leaderA->m_level < leaderB->m_level ? -1 : 1;
                }

                break;

            case 2: {
                NameCache* nameA = g_nameDBCache.GetRecord(leaderA->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
                NameCache* nameB = g_nameDBCache.GetRecord(leaderB->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
                ChrClassesRec* classA;
                ChrClassesRec* classB;

                if (nameA && nameB && (classA = g_chrClassesDB.GetRecord(nameA->m_class)) && (classB = g_chrClassesDB.GetRecord(nameB->m_class))) {
                    result = SStrSortUTF8I(classA->m_name, classB->m_name, STORM_MAX_STR);
                }

                break;
            }

            case 3: {
                NameCache* nameA = g_nameDBCache.GetRecord(leaderA->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
                NameCache* nameB = g_nameDBCache.GetRecord(leaderB->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);

                if (nameA && nameB) {
                    result = SStrCmp(nameA->m_name, nameB->m_name, STORM_MAX_STR);
                }

                break;
            }

            default:
                break;
        }

        if (result) {
            return CGLookingForGroup::m_unkBEA820[i].m_order ? -result : result;
        }
    }

    return 0;
}

// VERIFY: 484b867b manual
// OFFSET: 0x00555CD0
static int QSortLFGPlayer(const void* a, const void* b) {
    LFGListPlayer* playerA = *static_cast<LFGListPlayer* const*>(a);
    LFGListPlayer* playerB = *static_cast<LFGListPlayer* const*>(b);

    for (uint32_t i = 0; i < 7; i++) {
        int32_t result = 0;

        switch (CGLookingForGroup::m_unkBEA820[i].m_id) {
            case 0: {
                AreaTableRec* areaA = g_areaTableDB.GetRecord(playerA->m_areaID);
                AreaTableRec* areaB = g_areaTableDB.GetRecord(playerB->m_areaID);

                if (areaA && areaB) {
                    result = SStrSortUTF8I(areaA->m_areaName, areaB->m_areaName, STORM_MAX_STR);
                }

                break;
            }

            case 1:
                if (playerA->m_level != playerB->m_level) {
                    result = playerA->m_level < playerB->m_level ? -1 : 1;
                }

                break;

            case 2: {
                NameCache* nameA = g_nameDBCache.GetRecord(playerA->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
                NameCache* nameB = g_nameDBCache.GetRecord(playerB->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
                ChrClassesRec* classA;
                ChrClassesRec* classB;

                if (nameA && nameB && (classA = g_chrClassesDB.GetRecord(nameA->m_class)) && (classB = g_chrClassesDB.GetRecord(nameB->m_class))) {
                    result = SStrSortUTF8I(classA->m_name, classB->m_name, STORM_MAX_STR);
                }

                break;
            }

            case 3: {
                NameCache* nameA = g_nameDBCache.GetRecord(playerA->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
                NameCache* nameB = g_nameDBCache.GetRecord(playerB->m_key.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);

                if (nameA && nameB) {
                    result = SStrCmp(nameA->m_name, nameB->m_name, STORM_MAX_STR);
                }

                break;
            }

            case 4:
                result = (playerB->m_roles & 0x2) - (playerA->m_roles & 0x2);
                break;

            case 5:
                result = (playerB->m_roles & 0x4) - (playerA->m_roles & 0x4);
                break;

            case 6:
                result = (playerB->m_roles & 0x8) - (playerA->m_roles & 0x8);
                break;

            default:
                break;
        }

        if (result) {
            return CGLookingForGroup::m_unkBEA820[i].m_order ? -result : result;
        }
    }

    return 0;
}

// VERIFY: 334efcd1 auto
// OFFSET: 0x0055B700
void CGLookingForGroup::ResortLFGLists() {
    LFGLIST* list = &CGLookingForGroup::m_prevSearchResults;

    if (list->m_partyList.m_count) {
        qsort(list->m_partyList.m_data, list->m_partyList.m_count, sizeof(LFGListParty*), QSortLFGParty);
    }

    if (list->m_playerList.m_count) {
        qsort(list->m_playerList.m_data, list->m_playerList.m_count, sizeof(LFGListPlayer*), QSortLFGPlayer);
    }

    list->Reindex();
    CGLookingForGroup::m_searchResults.Copy(*list);
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
}
