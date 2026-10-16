#include "LFGInfo.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LfgDungeonsRec.h"
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/Rec/LfgDungeonExpansionRec.h"
#include "WoW/Source/DB/Rec/LfgDungeonGroupRec.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include <ctime>
#include "WoW/Source/Game/ValidateName.h"
#include "Engine/Source/Os/Time.h"
#include "WoW/Source/Ui/LFGHandler.h"

struct LFGJoinResultError {
    /* 0000 */ uint32_t m_result;
    /* 0004 */ UIErrors m_error;
};

// STATIC: 0x00BEAA14
int32_t CGLookingForGroup::m_searchJoinedID;

// STATIC: 0x00BEAA18
uint32_t CGLookingForGroup::m_lastProposalID;

// STATIC: 0x00BEAA24
uint32_t CGLookingForGroup::m_randomDungeonBestChoice;

// STATIC: 0x00BEAA70
TSFixedArray<uint32_t> CGLookingForGroup::m_randomDungeons;

// STATIC: 0x00BEAA7C
TSFixedArray<CGLookingForGroup::LFGEntryInfo> CGLookingForGroup::m_lfdEntries;

// STATIC: 0x00BEAA88
TSFixedArray<CGLookingForGroup::LFGEntryInfo> CGLookingForGroup::m_lfrEntries;

// STATIC: 0x00BEA820
CGLookingForGroup::LFGEntryInfo CGLookingForGroup::m_unkBEA820[7];

// STATIC: 0x00BEA858
LFGBootProposal CGLookingForGroup::m_bootProposal;

// STATIC: 0x00BEA980
LFGProposal CGLookingForGroup::m_proposal;

// STATIC: 0x00BEAA10
uint8_t CGLookingForGroup::m_unkBEAA10;

// STATIC: 0x00BEAA98
LFGLIST CGLookingForGroup::m_prevSearchResults;

// STATIC: 0x00ACD660
uint8_t CGLookingForGroup::m_unkACD660;

// STATIC: 0x00BEAB20
LFGLIST CGLookingForGroup::m_searchResults;

// STATIC: 0x00ACD880
LFGInfo CGLookingForGroup::m_localInfo;

// STATIC: 0x00ACD9A0
LFGInfo CGLookingForGroup::m_playerInfo;

// STATIC: 0x00ACDAC0
LFGInfo CGLookingForGroup::m_partyInfo;

// STATIC: 0x00ACDBE0
LFGLockInfo CGLookingForGroup::m_playerLockInfo;

// STATIC: 0x00ACDC08
LFGPartyLockInfo CGLookingForGroup::m_partyLockInfo;

// STATIC: 0x00ACDC18
LFGRewardInfo CGLookingForGroup::m_reward;

// STATIC: 0x00BEAA28
LFGQueueStatus CGLookingForGroup::m_queueStatus;

// STATIC: 0x00BEAA4C
int32_t CGLookingForGroup::m_roleCheckState;

// STATIC: 0x00BEAA50
TSGrowableArray<uint32_t> CGLookingForGroup::m_roleCheckSlots;

// STATIC: 0x00BEAA60
TSGrowableArray<LFGRoleCheckMember> CGLookingForGroup::m_roleCheckMembers;

// VERIFY: 0e4c2bfa manual
// OFFSET: 0x00557260
bool CGLookingForGroup::HasRaidDungeon(LFGInfo* info) {
    if (!info->m_joined) {
        return false;
    }

    for (LFGDungeonEntry* entry = info->m_dungeons.Head(); entry; entry = info->m_dungeons.Next(entry)) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(entry->m_slot & 0xFFFFFF);

        if (rec && rec->m_typeID == 2) {
            return true;
        }
    }

    return false;
}

// VERIFY: 9c153620 manual
// OFFSET: 0x005536A0
static int QSortLFGDungeons(const void* a, const void* b) {
    const CGLookingForGroup::LFGEntryInfo* left = static_cast<const CGLookingForGroup::LFGEntryInfo*>(a);
    const CGLookingForGroup::LFGEntryInfo* right = static_cast<const CGLookingForGroup::LFGEntryInfo*>(b);
    int32_t leftID = left->m_id;
    int32_t rightID = right->m_id;
    bool leftIsGroup = leftID < 0;
    bool rightIsGroup = rightID < 0;
    int32_t leftGroupID = leftIsGroup ? -leftID : left->m_order;
    int32_t rightGroupID = rightIsGroup ? -rightID : right->m_order;

    if (leftGroupID != rightGroupID) {
        int32_t leftOrder = -1;
        int32_t rightOrder = -1;

        if (leftGroupID) {
            LfgDungeonGroupRec* group = g_lfgDungeonGroupDB.GetRecord(leftGroupID);
            leftOrder = group ? group->m_orderIndex : 0;
        }

        if (rightGroupID) {
            LfgDungeonGroupRec* group = g_lfgDungeonGroupDB.GetRecord(rightGroupID);
            rightOrder = group ? group->m_orderIndex : 0;
        }

        if (leftOrder != rightOrder) {
            return leftOrder - rightOrder;
        }
    }

    if (leftIsGroup != rightIsGroup) {
        return 2 * !leftIsGroup - 1;
    }

    LfgDungeonsRec* leftRec = g_lfgDungeonsDB.GetRecord(leftID);
    LfgDungeonsRec* rightRec = g_lfgDungeonsDB.GetRecord(rightID);

    if (leftRec->m_orderIndex != rightRec->m_orderIndex) {
        return leftRec->m_orderIndex - rightRec->m_orderIndex;
    }

    LfgDungeonExpansionRec* leftExpansion = GetLfgDungeonExpansionRec(leftRec->m_id, ClientServices::Connection()->m_accountExpansion);
    LfgDungeonExpansionRec* rightExpansion = GetLfgDungeonExpansionRec(rightRec->m_id, ClientServices::Connection()->m_accountExpansion);
    int32_t leftLevel = leftExpansion ? leftExpansion->m_hardLevelMin + leftExpansion->m_hardLevelMax : leftRec->m_minLevel + leftRec->m_maxLevel;
    int32_t rightLevel = rightExpansion ? rightExpansion->m_hardLevelMin + rightExpansion->m_hardLevelMax : rightRec->m_minLevel + rightRec->m_maxLevel;

    if (leftLevel / 2 != rightLevel / 2) {
        return leftLevel / 2 - rightLevel / 2;
    }

    return SStrCmpUTF8I(leftRec->m_name, rightRec->m_name, 0x7FFFFFFF);
}

// VERIFY: 2c369f9d manual
// OFFSET: 0x005574C0
LFGRewardItem* CGLookingForGroup::GetDungeonRewardItem(uint32_t slot, int32_t index) {
    if (!CGLookingForGroup::m_playerLockInfo.m_valid) {
        return nullptr;
    }

    for (LFGDungeonEntry* dungeon = CGLookingForGroup::m_playerLockInfo.m_dungeons.Head(); dungeon; dungeon = CGLookingForGroup::m_playerLockInfo.m_dungeons.Next(dungeon)) {
        if (dungeon->m_slot != slot) {
            continue;
        }

        for (LFGRewardItem* item = dungeon->m_rewards.Head(); item; item = dungeon->m_rewards.Next(item)) {
            if (!index--) {
                return item;
            }
        }

        return nullptr;
    }

    return nullptr;
}

// VERIFY: ab53bd79 manual
// OFFSET: 0x00558350
void CGLookingForGroup::UpdateLFDEntries() {
    uint32_t count = 0;

    for (int32_t i = 0; i < g_lfgDungeonsDB.m_numRecords; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecordByIndex(i);

        if (rec && (rec->m_flags & 0x2) && rec->m_typeID != 6 && !(rec->m_flags & 0x4)) {
            count++;
        }
    }

    for (int32_t i = 0; i < g_lfgDungeonGroupDB.m_numRecords; i++) {
        LfgDungeonGroupRec* rec = g_lfgDungeonGroupDB.GetRecordByIndex(i);

        if (rec && rec->m_typeID) {
            count++;
        }
    }

    CGLookingForGroup::m_lfdEntries.SetCount(count);

    uint32_t index = 0;

    for (int32_t i = 0; i < g_lfgDungeonsDB.m_numRecords; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecordByIndex(i);

        if (rec && (rec->m_flags & 0x2) && rec->m_typeID != 6 && !(rec->m_flags & 0x4)) {
            CGLookingForGroup::m_lfdEntries.m_data[index].m_id = rec->m_id;
            CGLookingForGroup::m_lfdEntries.m_data[index].m_order = rec->m_groupID;
            index++;
        }
    }

    for (int32_t i = 0; i < g_lfgDungeonGroupDB.m_numRecords; i++) {
        LfgDungeonGroupRec* rec = g_lfgDungeonGroupDB.GetRecordByIndex(i);

        if (rec && rec->m_typeID) {
            CGLookingForGroup::m_lfdEntries.m_data[index].m_id = -rec->m_id;
            CGLookingForGroup::m_lfdEntries.m_data[index].m_order = rec->m_parentGroupID;
            index++;
        }
    }

    qsort(CGLookingForGroup::m_lfdEntries.m_data, count, sizeof(CGLookingForGroup::LFGEntryInfo), QSortLFGDungeons);
}

// VERIFY: 76abef22 manual
// OFFSET: 0x00558520
void CGLookingForGroup::UpdateLFREntries() {
    uint32_t count = 0;

    for (int32_t i = 0; i < g_lfgDungeonsDB.m_numRecords; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecordByIndex(i);

        if (rec && rec->m_typeID == 2) {
            count++;
        }
    }

    for (int32_t i = 0; i < g_lfgDungeonGroupDB.m_numRecords; i++) {
        if (g_lfgDungeonGroupDB.GetRecordByIndex(i)) {
            count++;
        }
    }

    CGLookingForGroup::m_lfrEntries.SetCount(count);

    uint32_t index = 0;

    for (int32_t i = 0; i < g_lfgDungeonsDB.m_numRecords; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecordByIndex(i);

        if (rec && rec->m_typeID == 2) {
            CGLookingForGroup::m_lfrEntries.m_data[index].m_id = rec->m_id;
            CGLookingForGroup::m_lfrEntries.m_data[index].m_order = rec->m_groupID;
            index++;
        }
    }

    for (int32_t i = 0; i < g_lfgDungeonGroupDB.m_numRecords; i++) {
        LfgDungeonGroupRec* rec = g_lfgDungeonGroupDB.GetRecordByIndex(i);

        if (rec) {
            CGLookingForGroup::m_lfrEntries.m_data[index].m_id = -rec->m_id;
            CGLookingForGroup::m_lfrEntries.m_data[index].m_order = rec->m_parentGroupID;
            index++;
        }
    }

    qsort(CGLookingForGroup::m_lfrEntries.m_data, count, sizeof(CGLookingForGroup::LFGEntryInfo), QSortLFGDungeons);
}

// VERIFY: a1411244 manual
// OFFSET: 0x00558A70
void LFGRewardInfo::Reset() {
    this->m_randomSlot = 0;
    this->m_actualSlot = 0;
    this->m_first = 0;
    this->m_strangers = 0;
    this->m_baseMoney = 0;
    this->m_baseXP = 0;
    this->m_variableMoney = 0;
    this->m_variableXP = 0;

    for (LFGRewardItem* item = this->m_items.Head(); !this->m_items.IsEnd(item); ) {
        LFGRewardItem* next = item->RawNext();
        delete item;
        item = next;
    }
}

// VERIFY: 4c628326 manual
// OFFSET: 0x00558D70
LFGDungeonEntry::~LFGDungeonEntry() {
    for (LFGRewardItem* item = this->m_rewards.Head(); !this->m_rewards.IsEnd(item); ) {
        LFGRewardItem* next = item->RawNext();
        delete item;
        item = next;
    }
}

// VERIFY: 82f6daec manual
// OFFSET: 0x00558E60
void LFGInfo::Reset() {
    this->m_joined = 0;
    this->m_lfgJoined = 0;
    this->m_queued = 0;
    this->m_dirty = 0;
    this->m_noPartial = 0;
    this->m_achievements = 0;
    this->m_roles[0] = 0;
    this->m_roles[1] = 0;
    this->m_roles[2] = 0;

    for (LFGDungeonEntry* dungeon = this->m_dungeons.Head(); !this->m_dungeons.IsEnd(dungeon); ) {
        LFGDungeonEntry* next = dungeon->RawNext();
        delete dungeon;
        dungeon = next;
    }

    this->m_comment[0] = '\0';
    this->m_numDungeons = 0;
}

// VERIFY: f46f62a0 auto
// OFFSET: 0x00559310
void CGLookingForGroup::JoiningParty() {
    CGLookingForGroup::m_playerInfo.Reset();
    CGLookingForGroup::m_partyInfo.Reset();
    CGLookingForGroup::RequestLFGStatus();
    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE, "%b", 1);
}

// VERIFY: ced25f9e manual
// OFFSET: 0x00559340
void CGLookingForGroup::ClearDungeonList() {
    for (LFGDungeonEntry* dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Head(); !CGLookingForGroup::m_localInfo.m_dungeons.IsEnd(dungeon); ) {
        LFGDungeonEntry* next = dungeon->RawNext();
        delete dungeon;
        dungeon = next;
    }

    CGLookingForGroup::m_localInfo.m_numDungeons = 0;
    CGLookingForGroup::m_localInfo.m_dirty = 1;
}

// VERIFY: ff1fe1c8 manual
// OFFSET: 0x00559800
void LFGLockInfo::ClearDungeons() {
    for (LFGDungeonEntry* dungeon = this->m_dungeons.Head(); !this->m_dungeons.IsEnd(dungeon); ) {
        LFGDungeonEntry* next = dungeon->RawNext();
        delete dungeon;
        dungeon = next;
    }
}

// INLINED
LFGLockInfo::LFGLockInfo() {
    this->m_valid = 0;
    this->m_guid.m_guid = 0;
    this->ClearDungeons();
}

// VERIFY: 9ea636a6 manual
// OFFSET: 0x0055AD80
LFGLockInfo::~LFGLockInfo() {
    this->m_valid = 0;
    this->m_guid.m_guid = 0;
    this->ClearDungeons();
}

// VERIFY: 05875565 manual
// OFFSET: 0x0055AE20
void LFGPartyLockInfo::Clear() {
    this->m_valid = 0;

    for (LFGLockInfo* member = this->m_members.Head(); !this->m_members.IsEnd(member); ) {
        LFGLockInfo* next = member->RawNext();
        delete member;
        member = next;
    }
}

// VERIFY: 71940fb5 manual
// OFFSET: 0x0055D440
void CGLookingForGroup::InitializeGame() {
    CGLookingForGroup::m_prevSearchResults.Clear();
    CGLookingForGroup::m_searchResults.Clear();
    CGLookingForGroup::m_localInfo.Reset();
    CGLookingForGroup::m_playerInfo.Reset();
    CGLookingForGroup::m_partyInfo.Reset();
    CGLookingForGroup::m_playerLockInfo.m_valid = 0;
    CGLookingForGroup::m_playerLockInfo.m_guid.m_guid = 0;
    CGLookingForGroup::m_playerLockInfo.ClearDungeons();
    CGLookingForGroup::m_partyLockInfo.Clear();
    CGLookingForGroup::m_queueStatus.m_lfgID = 0;
    CGLookingForGroup::m_queueStatus.m_averageWait = 0;
    CGLookingForGroup::m_queueStatus.m_tankWait = 0;
    CGLookingForGroup::m_queueStatus.m_healerWait = 0;
    CGLookingForGroup::m_queueStatus.m_damageWait = 0;
    CGLookingForGroup::m_queueStatus.m_myWait = 0;
    CGLookingForGroup::m_queueStatus.m_tanksNeeded = 0;
    CGLookingForGroup::m_queueStatus.m_healersNeeded = 0;
    CGLookingForGroup::m_queueStatus.m_damageNeeded = 0;
    CGLookingForGroup::m_queueStatus.m_statusTime = 0;
    CGLookingForGroup::m_queueStatus.m_queuedTime = 0;
    CGLookingForGroup::m_reward.Reset();
    CGLookingForGroup::m_unkBEAA10 = 0;
    CGLookingForGroup::m_searchJoinedID = 0;
    CGLookingForGroup::m_randomDungeonBestChoice = 0;
    CGLookingForGroup::m_randomDungeons.Clear();
    CGLookingForGroup::UpdateLFDEntries();
    CGLookingForGroup::UpdateLFREntries();
    CGLookingForGroup::m_proposal.m_dungeonId = 0;
    CGLookingForGroup::m_proposal.m_state = 0;
    CGLookingForGroup::m_proposal.m_proposalId = 0;
    CGLookingForGroup::m_proposal.m_completedMask = 0;
    CGLookingForGroup::m_proposal.m_silent = 0;
    memset(CGLookingForGroup::m_proposal.m_members, 0, sizeof(CGLookingForGroup::m_proposal.m_members));
    CGLookingForGroup::m_roleCheckState = 0;
    CGLookingForGroup::m_roleCheckSlots.m_count = 0;
    CGLookingForGroup::m_roleCheckMembers.m_count = 0;

    for (uint32_t i = 0; i < 7; i++) {
        CGLookingForGroup::m_unkBEA820[i].m_id = i;
        CGLookingForGroup::m_unkBEA820[i].m_order = 0;
    }

    CGLookingForGroup::m_unkACD660 = 1;

    ClientServices::SetMessageHandler(SMSG_LFG_LFR_LIST, Packet_SMSG_LFG_SEARCH_RESULTS, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_UPDATE_PLAYER, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_UPDATE_PARTY, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_UPDATE_SEARCH, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_QUEUE_STATUS, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_DISABLED, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_PROPOSAL_UPDATE, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_ROLE_CHECK_UPDATE, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_ROLE_CHOSEN, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_JOIN_RESULT, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_BOOT_PROPOSAL_UPDATE, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_PLAYER_INFO, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_PARTY_INFO, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_PLAYER_REWARD, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_TELEPORT_DENIED, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_LFG_OFFER_CONTINUE, LFGPacketHandler, nullptr);
    ClientServices::SetMessageHandler(SMSG_OPEN_LFG_DUNGEON_FINDER, Packet_SMSG_LFG_OPEN_FROM_GOSSIP, nullptr);
}

// VERIFY: 40f0cb62 manual
// OFFSET: 0x006349F0
LfgDungeonExpansionRec* GetLfgDungeonExpansionRec(int32_t dungeonID, int32_t expansionLevel) {
    for (int32_t i = 0; i < g_lfgDungeonExpansionDB.m_numRecords; i++) {
        if (g_lfgDungeonExpansionDB.GetRecordByIndex(i)->m_lfgID != dungeonID) {
            continue;
        }

        for (int32_t j = i; j < g_lfgDungeonExpansionDB.m_numRecords; j++) {
            LfgDungeonExpansionRec* rec = g_lfgDungeonExpansionDB.GetRecordByIndex(j);

            if (rec->m_lfgID != dungeonID) {
                return nullptr;
            }

            if (rec->m_expansionLevel == expansionLevel) {
                return rec;
            }
        }
    }

    return nullptr;
}

// VERIFY: 9f6b8622 auto
// OFFSET: 0x00552850
void LFGProposal::Clear() {
    this->m_dungeonId = 0;
    this->m_state = 0;
    this->m_proposalId = 0;
    this->m_completedMask = 0;
    this->m_silent = 0;
    memset(this->m_members, 0, sizeof(this->m_members));
}

// VERIFY: 8daf3271 manual
// OFFSET: 0x00552870
void LFGQueueStatus::Clear() {
    this->m_lfgID = 0;
    this->m_averageWait = 0;
    this->m_tankWait = 0;
    this->m_healerWait = 0;
    this->m_damageWait = 0;
    this->m_myWait = 0;
    this->m_tanksNeeded = 0;
    this->m_healersNeeded = 0;
    this->m_damageNeeded = 0;
    this->m_statusTime = 0;
    this->m_queuedTime = 0;
}

// VERIFY: 9624669a auto
// OFFSET: 0x00552900
void CGLookingForGroup::ShowProposal(int32_t silent) {
    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_UPDATE, nullptr);

    if (CGLookingForGroup::m_lastProposalID != CGLookingForGroup::m_proposal.m_proposalId) {
        CGLookingForGroup::m_lastProposalID = CGLookingForGroup::m_proposal.m_proposalId;

        if (!silent) {
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_SHOW, nullptr);
        }
    }
}

// VERIFY: b8ab3e63 auto
// OFFSET: 0x005570C0
LFGDungeonEntry* LFGInfo::FindDungeon(uint32_t slot) const {
    for (LFGDungeonEntry* dungeon = const_cast<LFGInfo*>(this)->m_dungeons.Head(); !this->m_dungeons.IsEnd(dungeon); dungeon = dungeon->RawNext()) {
        if (dungeon->m_slot == slot) {
            return dungeon;
        }
    }

    return nullptr;
}

// VERIFY: dd19911d manual
// OFFSET: 0x005570F0
bool LFGInfo::Equals(const LFGInfo& other) const {
    if ((this->m_joined != 0) != (other.m_joined != 0) || (this->m_lfgJoined != 0) != (other.m_lfgJoined != 0) || (this->m_queued != 0) != (other.m_queued != 0) || (this->m_noPartial != 0) != (other.m_noPartial != 0) || (this->m_achievements != 0) != (other.m_achievements != 0) || this->m_numDungeons != other.m_numDungeons) {
        return false;
    }

    if (memcmp(this->m_roles, other.m_roles, sizeof(this->m_roles)) || SStrCmp(this->m_comment, other.m_comment, 0x7FFFFFFF)) {
        return false;
    }

    LFGInfo* self = const_cast<LFGInfo*>(this);

    for (LFGDungeonEntry* dungeon = self->m_dungeons.Head(); !self->m_dungeons.IsEnd(dungeon); dungeon = self->m_dungeons.Next(dungeon)) {
        if (!other.FindDungeon(dungeon->m_slot)) {
            return false;
        }
    }

    return true;
}

// INLINED
LFGDungeonEntry::LFGDungeonEntry() {
    this->m_slot = 0;
    this->m_rewardDone = 0;
    this->m_locked = 0;
    this->m_lockReason = 0;
    this->m_baseMoney = 0;
    this->m_baseXP = 0;
    this->m_variableMoney = 0;
    this->m_variableXP = 0;
}

// VERIFY: 0df5071f manual
// OFFSET: 0x00559840
LFGDungeonEntry* LFGLockInfo::AddDungeon(uint32_t slot) {
    for (LFGDungeonEntry* dungeon = this->m_dungeons.Head(); !this->m_dungeons.IsEnd(dungeon); dungeon = dungeon->RawNext()) {
        if (dungeon->m_slot == slot) {
            return dungeon;
        }
    }

    LFGDungeonEntry* dungeon = new (__FILE__, __LINE__) LFGDungeonEntry();
    dungeon->m_slot = slot;
    this->m_dungeons.LinkToTail(dungeon);
    return dungeon;
}

// VERIFY: e83e8eb7 auto
// OFFSET: 0x005598D0
LFGInfo::LFGInfo() {
    this->Reset();
}

// VERIFY: 1a192e21 manual
// OFFSET: 0x00559900
LFGInfo::~LFGInfo() {
    this->Reset();
}

// VERIFY: 3eab0733 manual
// OFFSET: 0x00559950
LFGDungeonEntry* LFGInfo::AddDungeon(uint32_t slot) {
    for (LFGDungeonEntry* dungeon = this->m_dungeons.Head(); !this->m_dungeons.IsEnd(dungeon); dungeon = dungeon->RawNext()) {
        if (dungeon->m_slot == slot) {
            return dungeon;
        }
    }

    LFGDungeonEntry* dungeon = new (__FILE__, __LINE__) LFGDungeonEntry();
    dungeon->m_slot = slot;
    this->m_dungeons.LinkToTail(dungeon);
    this->m_numDungeons++;
    return dungeon;
}

// VERIFY: bb433006 manual
// OFFSET: 0x005599F0
void LFGInfo::CopyFrom(const LFGInfo& other) {
    this->m_joined = other.m_joined;
    this->m_lfgJoined = other.m_lfgJoined;
    this->m_queued = other.m_queued;
    this->m_noPartial = other.m_noPartial;
    this->m_achievements = other.m_achievements;
    memcpy(this->m_roles, other.m_roles, sizeof(this->m_roles));
    SStrCopy(this->m_comment, other.m_comment, sizeof(this->m_comment));

    for (LFGDungeonEntry* dungeon = this->m_dungeons.Head(); !this->m_dungeons.IsEnd(dungeon); ) {
        LFGDungeonEntry* next = dungeon->RawNext();

        if (!other.FindDungeon(dungeon->m_slot)) {
            delete dungeon;
            this->m_numDungeons--;
        }

        dungeon = next;
    }

    LFGInfo& source = const_cast<LFGInfo&>(other);

    for (LFGDungeonEntry* dungeon = source.m_dungeons.Head(); !source.m_dungeons.IsEnd(dungeon); dungeon = dungeon->RawNext()) {
        if (!this->FindDungeon(dungeon->m_slot)) {
            this->AddDungeon(dungeon->m_slot);
        }
    }
}

// VERIFY: 704dfe19 auto
// OFFSET: 0x00558C30
static void ClearSearchResults() {
    CGLookingForGroup::m_searchResults.Clear();
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
}

// STATIC: 0x00ACDDA8
static const LFGJoinResultError s_joinResultErrors[17] = {
    { 1, ERR_LFG_ROLE_CHECK_FAILED },
    { 2, ERR_LFG_GROUP_FULL },
    { 4, ERR_LFG_NO_LFG_OBJECT },
    { 5, ERR_LFG_NO_SLOTS_PLAYER },
    { 6, ERR_LFG_NO_SLOTS_PARTY },
    { 7, ERR_LFG_MISMATCHED_SLOTS },
    { 8, ERR_LFG_PARTY_PLAYERS_FROM_DIFFERENT_REALMS },
    { 9, ERR_LFG_MEMBERS_NOT_PRESENT },
    { 10, ERR_LFG_GET_INFO_TIMEOUT },
    { 11, ERR_LFG_INVALID_SLOT },
    { 12, ERR_LFG_DESERTER_PLAYER },
    { 13, ERR_LFG_DESERTER_PARTY },
    { 14, ERR_LFG_RANDOM_COOLDOWN_PLAYER },
    { 15, ERR_LFG_RANDOM_COOLDOWN_PARTY },
    { 16, ERR_LFG_TOO_MANY_MEMBERS },
    { 17, ERR_LFG_CANT_USE_DUNGEONS },
    { 18, ERR_LFG_ROLE_CHECK_FAILED },
};

// VERIFY: 5418061a manual
// OFFSET: 0x0055B080
void CGLookingForGroup::ParsePartyInfo(CDataStore* msg) {
    uint8_t count;
    msg->Get(count);

    for (uint32_t i = 0; i < count; i++) {
        WOWGUID guid;
        msg->Get(guid.m_guid);
        LFGLockInfo* member = CGLookingForGroup::m_partyLockInfo.AddMember(guid);
        member->ClearDungeons();
        member->m_valid = 1;
        CGLookingForGroup::ParseBlackList(msg, member);
    }
}

// VERIFY: b597272d manual
// OFFSET: 0x0055BDC0
int32_t LFGPacketHandler(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    switch (msgId) {
        case SMSG_LFG_TELEPORT_DENIED: {
            uint32_t reason;
            msg->Get(reason);
            DebugLog("LFGMessage: LFG_TELEPORT_DENIED - Reason: %u", reason);

            switch (reason) {
                case 0:
                case 5:
                case 7:
                    break;

                case 1:
                    CGGameUI::DisplayError(ERR_PLAYER_DEAD);
                    break;

                case 2:
                    CGGameUI::DisplayError(ERR_NOT_WHILE_FALLING);
                    break;

                case 4:
                    CGGameUI::DisplayError(ERR_NOT_WHILE_FATIGUED);
                    break;

                case 6:
                    CGGameUI::DisplayError(ERR_INVALID_TELEPORT_LOCATION);
                    CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
                    break;

                default:
                    CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
                    break;
            }

            break;
        }

        case SMSG_LFG_PLAYER_REWARD: {
            LFGRewardInfo& reward = CGLookingForGroup::m_reward;
            reward.Reset();
            msg->Get(reward.m_randomSlot);
            msg->Get(reward.m_actualSlot);
            msg->GetInt8(reward.m_first);
            msg->Get(reinterpret_cast<uint32_t&>(reward.m_strangers));
            msg->Get(reinterpret_cast<uint32_t&>(reward.m_baseMoney));
            msg->Get(reinterpret_cast<uint32_t&>(reward.m_baseXP));
            msg->Get(reinterpret_cast<uint32_t&>(reward.m_variableMoney));
            msg->Get(reinterpret_cast<uint32_t&>(reward.m_variableXP));
            DebugLog("LFGMessage: LFG_PLAYER_REWARD - Random Slot: %u, Actual Slot: %u, First: %d, Strangers: %u, Base Money: %d, Base XP: %d, Var Money: %d, Var XP: %d", reward.m_randomSlot, reward.m_actualSlot, reward.m_first, reward.m_strangers, reward.m_baseMoney, reward.m_baseXP, reward.m_variableMoney, reward.m_variableXP);
            uint8_t itemCount = 0;
            msg->Get(itemCount);

            for (uint32_t i = 0; i < itemCount; i++) {
                LFGRewardItem* item = new (__FILE__, __LINE__) LFGRewardItem;
                reward.m_items.LinkToTail(item);
                msg->Get(reinterpret_cast<uint32_t&>(item->m_itemID));
                msg->Get(reinterpret_cast<uint32_t&>(item->m_displayID));
                msg->Get(item->m_quantity);
                DebugLog("LFG_PLAYER_REWARD: Receiving Item %u, Display %u, Quantity: %u", item->m_itemID, item->m_displayID, item->m_quantity);
            }

            int32_t variableMoney = reward.m_variableMoney * reward.m_strangers;
            int32_t variableXP = reward.m_variableXP * reward.m_strangers;

            if (reward.m_baseXP > 0) {
                CGGameUI::DisplayError(ERR_QUEST_REWARD_EXP_I, reward.m_baseXP);
            }

            if (variableXP > 0) {
                CGGameUI::DisplayError(ERR_QUEST_REWARD_EXP_I, variableXP);
            }

            char money[1024];

            if (reward.m_baseMoney > 0) {
                GetCurrencyText(reward.m_baseMoney, money, sizeof(money), ", ");
                CGGameUI::DisplayError(ERR_QUEST_REWARD_MONEY_S, money);
            }

            if (variableMoney > 0) {
                GetCurrencyText(variableMoney, money, sizeof(money), ", ");
                CGGameUI::DisplayError(ERR_QUEST_REWARD_MONEY_S, money);
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_COMPLETION_REWARD, nullptr);
            break;
        }

        case SMSG_LFG_OFFER_CONTINUE: {
            uint32_t slot;
            msg->Get(slot);
            DebugLog("LFGMessage: LFG_OFFER_CONTINUE - Slot: %u", slot);
            LfgDungeonsRec* dungeon = g_lfgDungeonsDB.GetRecord(slot & 0xFFFFFF);

            if (dungeon) {
                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_OFFER_CONTINUE, "%s%d%d", dungeon->m_name, dungeon->m_id, dungeon->m_typeID);
            }

            break;
        }

        case SMSG_LFG_ROLE_CHOSEN: {
            uint64_t guid;
            msg->Get(guid);
            uint32_t accepted;
            msg->GetInt8(accepted);
            uint32_t roles;
            msg->Get(roles);
            DebugLog("LFGMessage: ROLE_CHOSEN - GUID: %016I64X, Accepted: %d, Roles Desired: %x", guid, accepted, roles);
            NameCache* name = g_nameDBCache.GetRecord(guid, WOWGUID(), nullptr, nullptr, false);

            if (!name) {
                break;
            }

            if (accepted) {
                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_ROLE_CHOSEN, "%s%b%b%b", name->m_name, (roles >> 1) & 1, (roles >> 2) & 1, (roles >> 3) & 1);
            } else {
                CGGameUI::DisplayError(ERR_LFG_PLAYER_DECLINED_ROLE_CHECK, name->m_name);
            }

            break;
        }

        case SMSG_LFG_PROPOSAL_UPDATE: {
            LFGProposal& proposal = CGLookingForGroup::m_proposal;
            msg->Get(proposal.m_dungeonId);
            msg->Get(proposal.m_state);
            msg->Get(proposal.m_proposalId);
            msg->Get(proposal.m_completedMask);
            msg->GetInt8(proposal.m_silent);
            uint8_t memberCount;
            msg->Get(memberCount);
            uint8_t declinedSelf = 0;
            uint8_t declinedParty = 0;

            for (uint32_t i = 0; i < memberCount; i++) {
                if (i >= 5) {
                    break;
                }

                LFGProposalMember& member = proposal.m_members[i];
                msg->Get(member.m_roles);
                msg->GetInt8(member.m_isLocal);
                msg->GetInt8(member.m_inDungeon);
                msg->GetInt8(member.m_inSameGroup);
                msg->GetInt8(member.m_answered);
                msg->GetInt8(member.m_isAccepted);

                if (member.m_answered && !member.m_isAccepted) {
                    if (member.m_isLocal) {
                        declinedSelf = 1;
                    } else if (member.m_inSameGroup) {
                        declinedParty = 1;
                    }
                }
            }

            DebugLog("LFGMessage: LFG_PROPOSAL_UPDATE - Slot: %u, State: %d, ID: %u, Last ID: %u, Completed Mask: %X, silent %d, I Declined: %d, Party Declined: %d", proposal.m_dungeonId, proposal.m_state, proposal.m_proposalId, CGLookingForGroup::m_lastProposalID, proposal.m_completedMask, proposal.m_silent, declinedSelf, declinedParty);
            CGLookingForGroup::ShowProposal(proposal.m_silent);

            if (declinedParty) {
                SI2::PlayUISound(0x43BD);
                CGGameUI::DisplayError(ERR_LFG_PROPOSAL_DECLINED_PARTY);
                proposal.m_dungeonId = 0;
                proposal.m_state = 0;
                proposal.m_proposalId = 0;
                proposal.m_completedMask = 0;
                proposal.m_silent = 0;
                memset(proposal.m_members, 0, sizeof(proposal.m_members));
                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_FAILED, nullptr);
            } else if (declinedSelf) {
                SI2::PlayUISound(0x43BD);
                CGGameUI::DisplayError(ERR_LFG_PROPOSAL_DECLINED_SELF);
                proposal.Clear();
                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_FAILED, nullptr);
            }

            break;
        }

        case SMSG_LFG_ROLE_CHECK_UPDATE: {
            msg->Get(reinterpret_cast<uint32_t&>(CGLookingForGroup::m_roleCheckState));
            uint32_t beginning;
            msg->GetInt8(beginning);
            uint8_t slotCount;
            msg->Get(slotCount);
            CGLookingForGroup::m_roleCheckSlots.SetCount(slotCount);

            for (int32_t i = 0; i < slotCount; i++) {
                msg->Get(CGLookingForGroup::m_roleCheckSlots.m_data[i]);
            }

            uint8_t memberCount;
            msg->Get(memberCount);
            CGLookingForGroup::m_roleCheckMembers.SetCount(memberCount);
            uint32_t localIndex = 0;

            for (uint32_t i = 0; i < memberCount; i++) {
                LFGRoleCheckMember& member = CGLookingForGroup::m_roleCheckMembers.m_data[i];
                msg->Get(member.m_guid.m_guid);
                uint8_t confirmed = 0;
                msg->Get(confirmed);
                member.m_confirmed = confirmed;
                msg->Get(member.m_roles);
                msg->Get(member.m_level);

                if (ClntObjMgrGetActivePlayer() == member.m_guid) {
                    localIndex = i;
                }
            }

            DebugLog("LFGMessage: LFG_ROLE_CHECK_UPDATE - Status: %u, Is Beginning: %d, Count: %d", CGLookingForGroup::m_roleCheckState, beginning, memberCount);

            if (beginning) {
                CGGameUI::DisplayError(ERR_LFG_ROLE_CHECK_INITIATED);
                SI2::PlayUISound(0x43A6);

                for (int32_t i = 0; i < memberCount; i++) {
                    LFGRoleCheckMember& member = CGLookingForGroup::m_roleCheckMembers.m_data[i];

                    if (!member.m_confirmed) {
                        continue;
                    }

                    NameCache* name = g_nameDBCache.GetRecord(member.m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);

                    if (name) {
                        FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_ROLE_CHOSEN, "%s%b%b%b", name->m_name, (CGLookingForGroup::m_roleCheckMembers.m_data[i].m_roles >> 1) & 1, (CGLookingForGroup::m_roleCheckMembers.m_data[i].m_roles >> 2) & 1, (CGLookingForGroup::m_roleCheckMembers.m_data[i].m_roles >> 3) & 1);
                    }
                }
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_UPDATE, nullptr);

            if (!CGLookingForGroup::m_roleCheckMembers.m_data[localIndex].m_confirmed && CGLookingForGroup::m_roleCheckState == 2) {
                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_SHOW, nullptr);
            } else if (CGLookingForGroup::m_roleCheckState == 5) {
                SI2::PlayUISound(0x43BD);
                CGGameUI::DisplayError(ERR_LFG_ROLE_CHECK_ABORTED);
                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_HIDE, nullptr);
            } else {
                if (CGLookingForGroup::m_roleCheckState == 6) {
                    SI2::PlayUISound(0x43BD);
                }

                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_ROLE_CHECK_HIDE, nullptr);
            }

            break;
        }

        case SMSG_LFG_JOIN_RESULT: {
            uint32_t result;
            msg->Get(result);
            uint32_t reason;
            msg->Get(reason);
            DebugLog("LFGMessage: LFG_JOIN_RESULT - Result: %u, Reason: %u", result, reason);

            if (result == 6) {
                CGLookingForGroup::m_partyLockInfo.Clear();
                CGLookingForGroup::ParsePartyInfo(msg);
                CGLookingForGroup::m_partyLockInfo.m_valid = 1;
            }

            if (result == 1) {
                if (reason == 3) {
                    CGGameUI::DisplayError(ERR_LFG_ROLE_CHECK_FAILED_TIMEOUT);
                    break;
                }

                if (reason == 4) {
                    CGGameUI::DisplayError(ERR_LFG_ROLE_CHECK_FAILED_NOT_VIABLE);
                    break;
                }
            }

            for (uint32_t i = 0; i < 17; i++) {
                if (s_joinResultErrors[i].m_result == result) {
                    CGGameUI::DisplayError(s_joinResultErrors[i].m_error);
                    break;
                }
            }

            break;
        }

        case SMSG_LFG_QUEUE_STATUS: {
            LFGQueueStatus& status = CGLookingForGroup::m_queueStatus;
            msg->Get(reinterpret_cast<uint32_t&>(status.m_lfgID));
            msg->Get(reinterpret_cast<uint32_t&>(status.m_averageWait));
            msg->Get(reinterpret_cast<uint32_t&>(status.m_tankWait));
            msg->Get(reinterpret_cast<uint32_t&>(status.m_healerWait));
            msg->Get(reinterpret_cast<uint32_t&>(status.m_damageWait));
            msg->Get(reinterpret_cast<uint32_t&>(status.m_myWait));
            msg->Get(status.m_tanksNeeded);
            msg->Get(status.m_healersNeeded);
            msg->Get(status.m_damageNeeded);
            uint32_t queuedTime;
            msg->Get(queuedTime);
            status.m_queuedTime = OsGetAsyncTimeMs() - 1000 * queuedTime;

            if (!status.m_tanksNeeded && !status.m_healersNeeded && !status.m_damageNeeded) {
                status.m_damageNeeded = 1;
            }

            DebugLog("LFGMessage: LFG_QUEUE_STATUS - LFGID: %d, Wait: %d, Wait Tanks: %d, Wait Healer: %d, Wait DPS: %d, Tanks Needed: %d, Healers Needed: %d, DPS Needed: %d", status.m_lfgID, status.m_averageWait, status.m_tankWait, status.m_healerWait, status.m_damageWait, status.m_tanksNeeded, status.m_healersNeeded, status.m_damageNeeded);
            status.m_statusTime = static_cast<uint32_t>(::time(nullptr));
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_QUEUE_STATUS_UPDATE, nullptr);
            break;
        }

        case SMSG_LFG_UPDATE_PLAYER: {
            LFGInfo info;
            uint8_t queued = 0;
            uint8_t updateType;
            msg->Get(updateType);
            uint8_t joined;
            msg->Get(joined);
            info.m_joined = joined != 0;
            info.m_lfgJoined = joined != 0;

            if (joined) {
                msg->Get(queued);
                uint8_t flag4;
                msg->Get(flag4);
                uint8_t flag5;
                msg->Get(flag5);
                uint8_t dungeonCount = 0;
                msg->Get(dungeonCount);
                info.m_queued = queued != 0;
                info.m_noPartial = flag4 != 0;
                info.m_achievements = flag5 != 0;

                for (uint32_t i = 0; i < dungeonCount; i++) {
                    uint32_t slot;
                    msg->Get(slot);
                    info.AddDungeon(slot);
                }

                msg->GetString(info.m_comment, sizeof(info.m_comment));
            }

            DebugLog("LFGMessage: LFG_UPDATE_PLAYER - Joined: %d, Queued: %d, Slot Count: %u, Proposal is valid: %d, In Party: %d", joined, queued, info.m_numDungeons, CGLookingForGroup::m_proposal.m_dungeonId, CGRaidInfo::m_numMembers);

            if (CGLookingForGroup::m_proposal.m_dungeonId && (!joined || queued) && !CGPartyInfo::InParty() && !CGRaidInfo::m_numMembers) {
                CGLookingForGroup::m_proposal.Clear();

                if (joined) {
                    CGGameUI::DisplayError(ERR_LFG_PROPOSAL_FAILED);
                }

                if (updateType == 10) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_SUCCEEDED, nullptr);
                } else {
                    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_FAILED, nullptr);
                }
            }

            LFGInfo& current = CGLookingForGroup::m_playerInfo;

            if (info.Equals(current)) {
                break;
            }

            bool left = false;

            if (info.m_lfgJoined) {
                if (!current.m_lfgJoined) {
                    SI2::PlayUISound(0x210E);
                    CGGameUI::DisplayError(CGLookingForGroup::HasRaidDungeon(&info) ? ERR_LFG_JOINED_LIST : ERR_LFG_JOINED_QUEUE);
                } else if (!info.m_queued && current.m_queued) {
                    left = true;
                }
            } else if (current.m_lfgJoined || (!info.m_queued && current.m_queued)) {
                left = true;
            }

            if (left && (updateType == 7 || updateType == 15)) {
                SI2::PlayUISound(0x43BD);
                CGGameUI::DisplayError(CGLookingForGroup::HasRaidDungeon(&current) ? ERR_LFG_LEFT_LIST : ERR_LFG_LEFT_QUEUE);
            }

            current.CopyFrom(info);

            if (!CGPartyInfo::InParty() && !CGRaidInfo::m_numMembers) {
                if (!info.m_queued) {
                    CGLookingForGroup::m_queueStatus.Clear();
                }

                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE, "%b", 0);
            }

            break;
        }

        case SMSG_LFG_UPDATE_PARTY: {
            LFGInfo info;
            uint8_t lfgJoined = 0;
            uint8_t queued = 0;
            uint8_t updateType;
            msg->Get(updateType);
            uint8_t joined;
            msg->Get(joined);
            info.m_joined = joined != 0;
            uint32_t dungeonCount = 0;

            if (joined) {
                msg->Get(lfgJoined);
                msg->Get(queued);
                uint8_t flag4;
                msg->Get(flag4);
                uint8_t flag5;
                msg->Get(flag5);
                info.m_lfgJoined = lfgJoined != 0;
                info.m_queued = queued != 0;
                info.m_noPartial = flag4 != 0;
                info.m_achievements = flag5 != 0;

                for (uint32_t i = 0; i < 3; i++) {
                    msg->Get(info.m_roles[i]);
                }

                uint8_t count = 0;
                msg->Get(count);
                dungeonCount = count;

                for (uint32_t i = 0; i < dungeonCount; i++) {
                    uint32_t slot;
                    msg->Get(slot);
                    info.AddDungeon(slot);
                }

                msg->GetString(info.m_comment, sizeof(info.m_comment));
            }

            DebugLog("LFGMessage: LFG_UPDATE_PARTY - Joined: %d, LfgJoined: %d, Queued: %d, Slot Count: %u, Proposal is valid: %d, In Party: %d", joined, lfgJoined, queued, info.m_numDungeons, CGLookingForGroup::m_proposal.m_dungeonId, CGPartyInfo::InParty());

            if (CGLookingForGroup::m_proposal.m_dungeonId && (!joined || queued) && (CGPartyInfo::InParty() || CGRaidInfo::m_numMembers)) {
                LFGProposal& proposal = CGLookingForGroup::m_proposal;
                proposal.m_dungeonId = 0;
                proposal.m_state = 0;
                proposal.m_proposalId = 0;
                proposal.m_completedMask = 0;
                proposal.m_silent = 0;
                memset(proposal.m_members, 0, sizeof(proposal.m_members));

                if (joined) {
                    CGGameUI::DisplayError(ERR_LFG_PROPOSAL_FAILED);
                }

                if (updateType == 10 || updateType == 1 || updateType == 16) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_SUCCEEDED, nullptr);
                } else {
                    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_PROPOSAL_FAILED, nullptr);
                }
            }

            LFGInfo& current = CGLookingForGroup::m_partyInfo;

            if (info.Equals(current)) {
                break;
            }

            bool left = false;

            if (info.m_lfgJoined) {
                if (!current.m_lfgJoined && dungeonCount) {
                    SI2::PlayUISound(0x210E);
                    CGGameUI::DisplayError(CGLookingForGroup::HasRaidDungeon(&info) ? ERR_LFG_JOINED_LIST : ERR_LFG_JOINED_QUEUE);
                } else if (!info.m_queued && current.m_queued) {
                    left = true;
                }
            } else if (current.m_lfgJoined || (!info.m_queued && current.m_queued)) {
                left = true;
            }

            if (left && (updateType == 7 || updateType == 15)) {
                SI2::PlayUISound(0x43BD);
                CGGameUI::DisplayError(CGLookingForGroup::HasRaidDungeon(&current) ? ERR_LFG_LEFT_LIST : ERR_LFG_LEFT_QUEUE);
            }

            current.CopyFrom(info);

            if (CGPartyInfo::InParty() || CGRaidInfo::m_numMembers) {
                if (!info.m_queued) {
                    CGLookingForGroup::m_queueStatus.Clear();
                }

                FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE, "%b", 0);
            }

            break;
        }

        case SMSG_LFG_UPDATE_SEARCH: {
            uint8_t joined = 0;
            msg->Get(joined);
            DebugLog("LFGMessage: LFG_UPDATE_SEARCH");

            if (joined || !CGLookingForGroup::m_searchJoinedID) {
                break;
            }

            CGLookingForGroup::m_prevSearchResults.Clear();
            CGLookingForGroup::m_searchJoinedID = 0;
            ClearSearchResults();
            break;
        }

        case SMSG_LFG_BOOT_PROPOSAL_UPDATE: {
            LFGBootProposal& boot = CGLookingForGroup::m_bootProposal;
            msg->GetInt8(boot.m_inProgress);
            msg->GetInt8(boot.m_didVote);
            msg->GetInt8(boot.m_myVote);
            msg->Get(boot.m_target.m_guid);
            msg->Get(boot.m_totalVotes);
            msg->Get(boot.m_bootVotes);
            msg->Get(boot.m_timeLeft);
            msg->Get(boot.m_votesNeeded);
            msg->GetString(boot.m_reason, sizeof(boot.m_reason));
            DebugLog("LFGMessage: LFG_BOOT_PLAYER - In Progress: %d, I Voted: %d, My Vote: %d, Target: %016I64X, Total Votes: %u, Boot Votes: %u, Time Left: %d, Votes Needed: %u, Comment: %s", boot.m_inProgress, boot.m_didVote, boot.m_myVote, boot.m_target.m_guid, boot.m_totalVotes, boot.m_bootVotes, boot.m_timeLeft, boot.m_votesNeeded, boot.m_reason);

            if (!boot.m_inProgress) {
                NameCache* name = g_nameDBCache.GetRecord(boot.m_target.m_guid, WOWGUID(), nullptr, nullptr, false);

                if (name) {
                    CGGameUI::DisplayError(boot.m_bootVotes < boot.m_votesNeeded ? ERR_PARTY_LFG_BOOT_VOTE_FAILED : ERR_PARTY_LFG_BOOT_VOTE_SUCCEEDED, name->m_name);
                }
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_BOOT_PROPOSAL_UPDATE, nullptr);
            break;
        }

        case SMSG_LFG_PLAYER_INFO: {
            LFGLockInfo& lockInfo = CGLookingForGroup::m_playerLockInfo;
            lockInfo.m_valid = 0;
            lockInfo.m_guid = WOWGUID();
            lockInfo.ClearDungeons();
            uint8_t dungeonCount;
            msg->Get(dungeonCount);
            DebugLog("LFGMessage: MSG_LFG_PLAYER_INFO");

            for (uint32_t i = 0; i < dungeonCount; i++) {
                uint32_t slot;
                msg->Get(slot);
                LFGDungeonEntry* dungeon = lockInfo.AddDungeon(slot);
                uint8_t rewardDone = 0;
                msg->Get(rewardDone);
                dungeon->m_rewardDone = rewardDone != 0;
                msg->Get(reinterpret_cast<uint32_t&>(dungeon->m_baseMoney));
                msg->Get(reinterpret_cast<uint32_t&>(dungeon->m_baseXP));
                msg->Get(reinterpret_cast<uint32_t&>(dungeon->m_variableMoney));
                msg->Get(reinterpret_cast<uint32_t&>(dungeon->m_variableXP));
                uint8_t itemCount;
                msg->Get(itemCount);

                for (uint32_t j = 0; j < itemCount; j++) {
                    LFGRewardItem* item = new (__FILE__, __LINE__) LFGRewardItem;
                    dungeon->m_rewards.LinkToTail(item);
                    msg->Get(reinterpret_cast<uint32_t&>(item->m_itemID));
                    msg->Get(reinterpret_cast<uint32_t&>(item->m_displayID));
                    msg->Get(item->m_quantity);
                }
            }

            CGLookingForGroup::ParseBlackList(msg, &lockInfo);
            lockInfo.m_valid = 1;
            CGLookingForGroup::UpdateRandomDungeons();
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_LOCK_INFO_RECEIVED, nullptr);
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE_RANDOM_INFO, nullptr);
            break;
        }

        case SMSG_LFG_PARTY_INFO:
            DebugLog("LFGMessage: LFG_PARTY_INFO");
            CGLookingForGroup::m_partyLockInfo.Clear();
            CGLookingForGroup::ParsePartyInfo(msg);
            CGLookingForGroup::m_partyLockInfo.m_valid = 1;
            CGLookingForGroup::UpdateRandomDungeons();
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_LOCK_INFO_RECEIVED, nullptr);
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE_RANDOM_INFO, nullptr);
            break;

        case SMSG_LFG_DISABLED:
            CGLookingForGroup::m_localInfo.Reset();
            CGLookingForGroup::m_playerInfo.Reset();
            CGLookingForGroup::m_partyInfo.Reset();
            CGLookingForGroup::m_playerLockInfo.m_valid = 0;
            CGLookingForGroup::m_playerLockInfo.m_guid = WOWGUID();
            CGLookingForGroup::m_playerLockInfo.ClearDungeons();
            CGLookingForGroup::m_partyLockInfo.Clear();
            CGLookingForGroup::m_queueStatus.Clear();
            CGLookingForGroup::m_reward.Reset();
            DebugLog("LFGMessage: LFG_DISABLED");
            FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE, "%b", 0);
            CGGameUI::DisplayError(ERR_SYSTEM_DISABLED);
            break;
    }

    return 1;
}

// VERIFY: d69dd684 manual
// OFFSET: 0x0055AE60
LFGLockInfo* LFGPartyLockInfo::FindMember(WOWGUID guid) {
    for (LFGLockInfo* member = this->m_members.Head(); member; member = this->m_members.Next(member)) {
        if (member->m_guid == guid) {
            return member;
        }
    }

    return nullptr;
}

// VERIFY: d37b8c89 manual
// OFFSET: 0x0055AEA0
LFGLockInfo* LFGPartyLockInfo::AddMember(WOWGUID guid) {
    LFGLockInfo* member = this->FindMember(guid);

    if (member) {
        return member;
    }

    member = new (__FILE__, __LINE__) LFGLockInfo();
    member->m_guid = guid;
    this->m_members.LinkToTail(member);
    return member;
}

// VERIFY: 55669ffa auto
// OFFSET: 0x0055AF20
void CGLookingForGroup::ShutdownGame() {
    CGLookingForGroup::m_prevSearchResults.Clear();
    CGLookingForGroup::m_searchResults.Clear();
    CGLookingForGroup::m_localInfo.Reset();
    CGLookingForGroup::m_playerInfo.Reset();
    CGLookingForGroup::m_partyInfo.Reset();
    CGLookingForGroup::m_playerLockInfo.m_valid = 0;
    CGLookingForGroup::m_playerLockInfo.m_guid = WOWGUID();
    CGLookingForGroup::m_playerLockInfo.ClearDungeons();
    CGLookingForGroup::m_partyLockInfo.Clear();
    CGLookingForGroup::m_queueStatus.Clear();
    CGLookingForGroup::m_reward.Reset();
    ClientServices::ClearMessageHandler(SMSG_LFG_LFR_LIST);
    ClientServices::ClearMessageHandler(SMSG_LFG_UPDATE_PLAYER);
    ClientServices::ClearMessageHandler(SMSG_LFG_UPDATE_PARTY);
    ClientServices::ClearMessageHandler(SMSG_LFG_UPDATE_SEARCH);
    ClientServices::ClearMessageHandler(SMSG_LFG_QUEUE_STATUS);
    ClientServices::ClearMessageHandler(SMSG_LFG_DISABLED);
    ClientServices::ClearMessageHandler(SMSG_LFG_PROPOSAL_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_LFG_ROLE_CHECK_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_LFG_ROLE_CHOSEN);
    ClientServices::ClearMessageHandler(SMSG_LFG_JOIN_RESULT);
    ClientServices::ClearMessageHandler(SMSG_LFG_BOOT_PROPOSAL_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_LFG_PLAYER_INFO);
    ClientServices::ClearMessageHandler(SMSG_LFG_PARTY_INFO);
    ClientServices::ClearMessageHandler(SMSG_LFG_PLAYER_REWARD);
    ClientServices::ClearMessageHandler(SMSG_LFG_TELEPORT_DENIED);
    ClientServices::ClearMessageHandler(SMSG_LFG_OFFER_CONTINUE);
    ClientServices::ClearMessageHandler(SMSG_OPEN_LFG_DUNGEON_FINDER);
}

// VERIFY: 3776dc9d auto
// OFFSET: 0x00559B10
void CGLookingForGroup::ParseBlackList(CDataStore* msg, LFGLockInfo* lockInfo) {
    uint32_t count;
    msg->Get(count);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot;
        msg->Get(slot);
        LFGDungeonEntry* entry = lockInfo->AddDungeon(slot);
        entry->m_locked = 1;
        uint32_t reason;
        msg->Get(reason);
        entry->m_lockReason = reason;
    }
}

// VERIFY: 72ac9659 manual
// OFFSET: 0x00559D40
bool CGLookingForGroup::IsRandomDungeonUnlocked(const LfgDungeonsRec* rec) {
    if (!(rec->m_flags & 0x4)) {
        return false;
    }

    uint32_t slot = (rec->m_id & 0xFFFFFF) | (rec->m_typeID << 24);

    for (LFGDungeonEntry* entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Head(); entry; entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Next(entry)) {
        if (entry->m_slot == slot) {
            return !entry->m_locked;
        }
    }

    return false;
}

// VERIFY: 589ed3a1 manual
// OFFSET: 0x0055AAF0
void CGLookingForGroup::UpdateRandomDungeons() {
    int32_t numRecords = g_lfgDungeonsDB.m_numRecords;
    uint32_t count = 0;

    for (int32_t i = 0; i < numRecords; i++) {
        LfgDungeonsRec* rec = i < 0 || i >= g_lfgDungeonsDB.m_numRecords ? nullptr : &g_lfgDungeonsDB.m_records[i];

        if (rec->m_typeID == 6 || CGLookingForGroup::IsRandomDungeonUnlocked(rec)) {
            count++;
        }
    }

    CGLookingForGroup::m_randomDungeons.SetCount(count);
    uint32_t index = 0;

    for (int32_t i = 0; i < numRecords; i++) {
        LfgDungeonsRec* rec = i < 0 || i >= g_lfgDungeonsDB.m_numRecords ? nullptr : &g_lfgDungeonsDB.m_records[i];
        uint32_t slot = (rec->m_id & 0xFFFFFF) | (rec->m_typeID << 24);

        if (rec->m_typeID == 6) {
            CGLookingForGroup::m_randomDungeons.m_data[index++] = slot;
        } else if (rec->m_flags & 0x4) {
            for (LFGDungeonEntry* entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Head(); entry; entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Next(entry)) {
                if (entry->m_slot == slot) {
                    if (!entry->m_locked) {
                        CGLookingForGroup::m_randomDungeons.m_data[index++] = slot;
                    }

                    break;
                }
            }
        }
    }

    CGLookingForGroup::UpdateRandomDungeonBestChoice();
}

// VERIFY: 15a7a01d manual
// OFFSET: 0x00559B70
void CGLookingForGroup::SetLFGDungeon(uint32_t slot) {
    uint32_t type = slot >> 24;
    bool clear = false;

    if (type == 1 || type == 5) {
        for (LFGDungeonEntry* dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Head(); dungeon; dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Next(dungeon)) {
            uint8_t dungeonType = static_cast<uint8_t>(dungeon->m_slot >> 24);

            if (dungeonType != 1 && dungeonType != 5) {
                clear = true;
                break;
            }
        }
    } else if (type != 2 || CGPartyInfo::GetNumPartyMembers()) {
        clear = true;
    } else {
        for (LFGDungeonEntry* dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Head(); dungeon; dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Next(dungeon)) {
            if (static_cast<uint8_t>(dungeon->m_slot >> 24) != 2) {
                clear = true;
                break;
            }
        }
    }

    if (clear) {
        CGLookingForGroup::ClearDungeonList();
    }

    for (LFGDungeonEntry* dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Head(); dungeon; dungeon = CGLookingForGroup::m_localInfo.m_dungeons.Next(dungeon)) {
        if (dungeon->m_slot == slot) {
            return;
        }
    }

    CGLookingForGroup::m_localInfo.AddDungeon(slot);
    CGLookingForGroup::m_localInfo.m_dirty = 1;
}

// VERIFY: 000b89b3 manual
// OFFSET: 0x00559C90
bool CGLookingForGroup::IsSlotValidForAll(uint32_t slot) {
    if (CGLookingForGroup::m_playerLockInfo.m_valid) {
        for (LFGDungeonEntry* entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Head(); entry; entry = CGLookingForGroup::m_playerLockInfo.m_dungeons.Next(entry)) {
            if (entry->m_slot == slot) {
                if (entry->m_locked) {
                    return false;
                }

                break;
            }
        }
    }

    if ((slot & 0xFF000000) != 0x2000000 && CGLookingForGroup::m_partyLockInfo.m_valid) {
        for (LFGLockInfo* member = CGLookingForGroup::m_partyLockInfo.m_members.Head(); member; member = CGLookingForGroup::m_partyLockInfo.m_members.Next(member)) {
            for (LFGDungeonEntry* entry = member->m_dungeons.Head(); entry; entry = member->m_dungeons.Next(entry)) {
                if (entry->m_slot == slot) {
                    if (entry->m_locked) {
                        return false;
                    }

                    break;
                }
            }
        }
    }

    return true;
}

// VERIFY: 0a9ddc1b manual
// OFFSET: 0x00559DA0
static bool IsBetterRandomDungeon(const LfgDungeonsRec* candidate, const LfgDungeonsRec* best) {
    if (!candidate || candidate->m_typeID != 6 || !CGLookingForGroup::IsSlotValidForAll(candidate->m_id & 0xFFFFFF | 0x6000000)) {
        return false;
    }

    if (!best) {
        return true;
    }

    if (candidate->m_expansionLevel != best->m_expansionLevel) {
        return candidate->m_expansionLevel > best->m_expansionLevel;
    }

    LfgDungeonExpansionRec* bestExpansion = GetLfgDungeonExpansionRec(best->m_id, ClientServices::Connection()->m_accountExpansion);
    LfgDungeonExpansionRec* candidateExpansion = GetLfgDungeonExpansionRec(candidate->m_id, ClientServices::Connection()->m_accountExpansion);
    int32_t bestLevel = (bestExpansion ? bestExpansion->m_targetLevelMin + bestExpansion->m_targetLevelMax : best->m_targetLevelMin + best->m_targetLevelMax) / 2;
    int32_t candidateLevel = (candidateExpansion ? candidateExpansion->m_targetLevelMin + candidateExpansion->m_targetLevelMax : candidate->m_targetLevelMin + candidate->m_targetLevelMax) / 2;

    if (candidateLevel != bestLevel) {
        return candidateLevel > bestLevel;
    }

    int32_t bestMin = bestExpansion ? bestExpansion->m_hardLevelMin : best->m_minLevel;
    int32_t candidateMin = candidateExpansion ? candidateExpansion->m_hardLevelMin : candidate->m_minLevel;
    return candidateMin > bestMin;
}

// VERIFY: fa41cf18 auto
// OFFSET: 0x00559EA0
bool CGLookingForGroup::UpdateRandomDungeonBestChoice() {
    LfgDungeonsRec* best = nullptr;

    for (uint32_t i = 0; i < CGLookingForGroup::m_randomDungeons.m_count; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecord(CGLookingForGroup::m_randomDungeons.m_data[i] & 0xFFFFFF);

        if (IsBetterRandomDungeon(rec, best)) {
            best = rec;
        }
    }

    uint32_t previous = CGLookingForGroup::m_randomDungeonBestChoice;

    if (best) {
        CGLookingForGroup::m_randomDungeonBestChoice = best->m_id & 0xFFFFFF | (best->m_typeID << 24);
        return previous != CGLookingForGroup::m_randomDungeonBestChoice;
    }

    CGLookingForGroup::m_randomDungeonBestChoice = 0;
    return previous != 0;
}

// VERIFY: 027a866a manual
// OFFSET: 0x00555A00
void CGLookingForGroup::RequestLFGPartyInfo() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_PARTY_LOCK_INFO_REQUEST));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// STATIC: 0x00ACDD50
static int32_t s_commentSendCount = 2;

// STATIC: 0x00BEABA8
static float s_commentSendTime;

// VERIFY: 7afbdf39 manual
// OFFSET: 0x005539A0
void CGLookingForGroup::SetLFGComment(const char* comment) {
    char scrubbed[256];
    ScrubString(comment, scrubbed, 64, sizeof(scrubbed), 1);
    SStrCopy(CGLookingForGroup::m_localInfo.m_comment, scrubbed, sizeof(CGLookingForGroup::m_localInfo.m_comment));

    if (!CGLookingForGroup::m_localInfo.m_joined) {
        CGLookingForGroup::m_localInfo.m_dirty = 1;
        return;
    }

    if (++s_commentSendCount > 2) {
        float now = OsGetAsyncTimeSec();

        if (now - s_commentSendTime < 2.0) {
            if (s_commentSendCount > 2) {
                return;
            }
        } else {
            s_commentSendTime = now;
            s_commentSendCount = 0;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_SET_COMMENT));
    msg.PutString(CGLookingForGroup::m_localInfo.m_comment);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 906cd50a manual
// OFFSET: 0x00558F90
void CGLookingForGroup::LeaveLFGSearch() {
    if (!CGLookingForGroup::m_searchJoinedID) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LFG_LFR_LEAVE));
    msg.Put(static_cast<uint32_t>(CGLookingForGroup::m_searchJoinedID));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGLookingForGroup::m_prevSearchResults.Clear();
    CGLookingForGroup::m_prevSearchResults.m_time = OsGetAsyncTimeMs();
    CGLookingForGroup::m_searchJoinedID = 0;
    CGLookingForGroup::m_searchResults.Clear();
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
}

// STATIC: 0x00BEAA1C
static uint32_t s_searchType;

// STATIC: 0x00BEAA20
static uint32_t s_searchID;

// VERIFY: 068db115 auto
// OFFSET: 0x005528A0
void LFGNameCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!CGLookingForGroup::m_prevSearchResults.m_pendingNames || !--CGLookingForGroup::m_prevSearchResults.m_pendingNames) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
    }
}

// VERIFY: 74fc5fd3 auto
// OFFSET: 0x005528D0
int32_t Packet_SMSG_LFG_OPEN_FROM_GOSSIP(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t slot;
    msg->Get(slot);
    FrameScript_SignalEvent(SCRIPT_EVENT_LFG_OPEN_FROM_GOSSIP, "%d", slot);
    return 1;
}

// VERIFY: d167d656 manual
// OFFSET: 0x0055B770
int32_t Packet_SMSG_LFG_SEARCH_RESULTS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    bool changed = false;
    msg->Get(s_searchType);
    msg->Get(s_searchID);
    int32_t lfgID = (s_searchID & 0xFFFFFF) | (s_searchType << 24);

    if (lfgID != CGLookingForGroup::m_searchJoinedID) {
        msg->m_read = msg->m_size;
        return 1;
    }

    LFGLIST* list = &CGLookingForGroup::m_prevSearchResults;

    if (static_cast<uint32_t>(lfgID) != list->m_lfgID) {
        changed = true;
    }

    uint8_t incremental = 0;
    msg->Get(incremental);

    if (incremental) {
        uint32_t count;
        msg->Get(count);

        for (uint32_t i = 0; i < count; i++) {
            uint64_t guid;
            msg->Get(guid);

            if ((guid & 0xF000000000000000ull) || !(guid & 0xF07FFFFFFFFFFFFFull)) {
                LFGListParty* party = list->m_parties.Ptr(static_cast<uint32_t>(guid), CHashKeyGUID(guid));

                if (party) {
                    list->DeleteParty(party);
                }
            } else {
                LFGListPlayer* player = list->m_players.Ptr(static_cast<uint32_t>(guid), CHashKeyGUID(guid));

                if (player) {
                    list->RemovePlayer(player);
                    delete player;
                }
            }
        }
    } else {
        list->Clear();
    }

    list->m_lfgID = lfgID;
    uint32_t count;
    msg->Get(count);
    msg->Get(list->m_totalParties);

    for (uint32_t i = 0; i < count; i++) {
        uint64_t guid;
        msg->Get(guid);
        LFGListParty* party = list->m_parties.Ptr(static_cast<uint32_t>(guid), CHashKeyGUID(guid));

        if (!party) {
            party = new (__FILE__, __LINE__) LFGListParty();
            list->m_parties.Insert(party, static_cast<uint32_t>(guid), CHashKeyGUID(guid));
        }

        LFGHandler::ReadGroupInfo(msg, party);

        if (party->m_index < 0) {
            uint32_t index = list->m_partyList.Count();
            list->m_partyList.SetCount(index + 1);
            list->m_partyList.m_data[index] = party;
            party->m_index = index;
        }
    }

    msg->Get(count);
    msg->Get(list->m_totalPlayers);

    for (uint32_t i = 0; i < count; i++) {
        WOWGUID guid;
        msg->Get(guid.m_guid);
        LFGListPlayer* player = list->m_players.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (!player) {
            player = new (__FILE__, __LINE__) LFGListPlayer();
            list->m_players.Insert(player, static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

            if (!g_nameDBCache.GetRecord(guid.m_guid, guid, LFGNameCacheCallback, nullptr, false)) {
                list->m_pendingNames++;
            }
        }

        LFGHandler::ReadPlayerInfo(msg, list, player);

        if (player->m_party) {
            player->m_party->AddPlayer(player);
        } else if (player->m_index < 0) {
            uint32_t index = list->m_playerList.Count();
            list->m_playerList.SetCount(index + 1);
            list->m_playerList.m_data[index] = player;
            player->m_index = index;
        }
    }

    if (changed) {
        CGLookingForGroup::m_searchResults.Copy(*list);
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_LFG_LIST, nullptr);
    }

    return 1;
}
