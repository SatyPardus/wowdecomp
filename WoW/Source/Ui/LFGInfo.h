#ifndef WOW_SOURCE_UI_LFGINFO_H
#define WOW_SOURCE_UI_LFGINFO_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Ui/LFGLIST.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
class LfgDungeonExpansionRec;

struct lua_State;

struct LFGRewardItem : public TSLinkedNode<LFGRewardItem> {
    /* 0008 */ int32_t m_itemID;
    /* 000C */ int32_t m_displayID;
    /* 0010 */ uint32_t m_quantity;
};

struct LFGDungeonEntry : public TSLinkedNode<LFGDungeonEntry> {
    /* 0008 */ uint32_t m_slot;
    /* 000C */ uint8_t m_rewardDone;
    /* 000D */ uint8_t m_locked;
    /* 000E */ uint8_t unused_000E[2];
    /* 0010 */ int32_t m_lockReason;
    /* 0014 */ int32_t m_baseMoney;
    /* 0018 */ int32_t m_baseXP;
    /* 001C */ int32_t m_variableMoney;
    /* 0020 */ int32_t m_variableXP;
    /* 0024 */ TSList<LFGRewardItem, TSGetLink<LFGRewardItem>> m_rewards;

    LFGDungeonEntry();
    ~LFGDungeonEntry();
};

struct LFGInfo {
    /* 0000 */ uint8_t m_joined;
    /* 0001 */ uint8_t m_lfgJoined;
    /* 0002 */ uint8_t m_queued;
    /* 0003 */ uint8_t m_dirty;
    /* 0004 */ uint8_t m_noPartial;
    /* 0005 */ uint8_t m_achievements;
    /* 0006 */ uint8_t m_roles[3];
    /* 0009 */ uint8_t unused_0009[3];
    /* 000C */ TSList<LFGDungeonEntry, TSGetLink<LFGDungeonEntry>> m_dungeons;
    /* 0018 */ uint32_t m_numDungeons;
    /* 001C */ char m_comment[256];
    /* 011C */ uint32_t unused_011C;

    LFGInfo();
    ~LFGInfo();
    void Reset();
    LFGDungeonEntry* AddDungeon(uint32_t slot);
    LFGDungeonEntry* FindDungeon(uint32_t slot) const;
    bool Equals(const LFGInfo& other) const;
    void CopyFrom(const LFGInfo& other);
};

struct LFGLockInfo : public TSLinkedNode<LFGLockInfo> {
    /* 0008 */ uint8_t m_valid;
    /* 0009 */ uint8_t unused_0009[3];
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ WOWGUID m_guid;
    /* 0018 */ TSList<LFGDungeonEntry, TSGetLink<LFGDungeonEntry>> m_dungeons;

    LFGLockInfo();
    ~LFGLockInfo();
    void ClearDungeons();
    LFGDungeonEntry* AddDungeon(uint32_t slot);
};

struct LFGPartyLockInfo {
    /* 0000 */ uint8_t m_valid;
    /* 0001 */ uint8_t unused_0001[3];
    /* 0004 */ TSList<LFGLockInfo, TSGetLink<LFGLockInfo>> m_members;

    void Clear();
    LFGLockInfo* FindMember(WOWGUID guid);
    LFGLockInfo* AddMember(WOWGUID guid);
};

struct LFGRewardInfo {
    /* 0000 */ uint32_t m_randomSlot;
    /* 0004 */ uint32_t m_actualSlot;
    /* 0008 */ uint32_t m_first;
    /* 000C */ int32_t m_strangers;
    /* 0010 */ int32_t m_baseMoney;
    /* 0014 */ int32_t m_baseXP;
    /* 0018 */ int32_t m_variableMoney;
    /* 001C */ int32_t m_variableXP;
    /* 0020 */ TSList<LFGRewardItem, TSGetLink<LFGRewardItem>> m_items;

    void Reset();
};

struct LFGProposalMember {
    /* 0000 */ uint32_t m_roles;
    /* 0004 */ uint32_t m_isLocal;
    /* 0008 */ uint32_t m_inDungeon;
    /* 000C */ uint32_t m_inSameGroup;
    /* 0010 */ uint32_t m_answered;
    /* 0014 */ uint32_t m_isAccepted;
};

struct LFGProposal {
    /* 0000 */ uint32_t m_dungeonId;
    /* 0004 */ uint8_t m_state;
    /* 0005 */ uint8_t unused_0005[3];
    /* 0008 */ uint32_t m_proposalId;
    /* 000C */ uint32_t m_completedMask;
    /* 0010 */ uint32_t m_silent;
    /* 0014 */ LFGProposalMember m_members[5];

    void Clear();
};

struct LFGQueueStatus {
    /* 0000 */ int32_t m_lfgID;
    /* 0004 */ int32_t m_averageWait;
    /* 0008 */ int32_t m_tankWait;
    /* 000C */ int32_t m_healerWait;
    /* 0010 */ int32_t m_damageWait;
    /* 0014 */ int32_t m_myWait;
    /* 0018 */ uint8_t m_tanksNeeded;
    /* 0019 */ uint8_t m_healersNeeded;
    /* 001A */ uint8_t m_damageNeeded;
    /* 001B */ uint8_t unused_001B;
    /* 001C */ uint32_t m_statusTime;
    /* 0020 */ int32_t m_queuedTime;

    void Clear();
};

struct LFGBootProposal {
    /* 0000 */ uint32_t m_inProgress;
    /* 0004 */ uint32_t m_didVote;
    /* 0008 */ uint32_t m_myVote;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ WOWGUID m_target;
    /* 0018 */ uint32_t m_totalVotes;
    /* 001C */ uint32_t m_bootVotes;
    /* 0020 */ uint32_t m_timeLeft;
    /* 0024 */ uint32_t m_votesNeeded;
    /* 0028 */ char m_reason[256];
};

struct LFGRoleCheckMember {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ int32_t m_confirmed;
    /* 000C */ uint32_t m_roles;
    /* 0010 */ uint8_t m_level;
    /* 0011 */ uint8_t unused_0011[3];
    /* 0014 */ uint32_t unused_0014;
};

class LfgDungeonsRec;
class CGLookingForGroup {
    public:
    struct LFGEntryInfo {
        /* 0000 */ int32_t m_id;
        /* 0004 */ int32_t m_order;
    };

    static int32_t m_searchJoinedID;
    static uint32_t m_lastProposalID;
    static uint32_t m_randomDungeonBestChoice;
    static TSFixedArray<uint32_t> m_randomDungeons;
    static TSFixedArray<CGLookingForGroup::LFGEntryInfo> m_lfdEntries;
    static TSFixedArray<CGLookingForGroup::LFGEntryInfo> m_lfrEntries;
    static LFGLIST m_searchResults;
    static LFGLIST m_prevSearchResults;
    static LFGProposal m_proposal;
    static CGLookingForGroup::LFGEntryInfo m_unkBEA820[7];
    static LFGBootProposal m_bootProposal;
    static uint8_t m_unkBEAA10;
    static uint8_t m_unkACD660;
    static LFGInfo m_localInfo;
    static LFGInfo m_playerInfo;
    static LFGInfo m_partyInfo;
    static LFGLockInfo m_playerLockInfo;
    static LFGPartyLockInfo m_partyLockInfo;
    static LFGRewardInfo m_reward;
    static LFGQueueStatus m_queueStatus;
    static int32_t m_roleCheckState;
    static TSGrowableArray<uint32_t> m_roleCheckSlots;
    static TSGrowableArray<LFGRoleCheckMember> m_roleCheckMembers;
    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static int32_t GetLFGRoleUpdate(lua_State* L);
    static int32_t GetLFGRoleUpdateSlot(lua_State* L);
    static void RequestLFGStatus();
    static void JoiningParty();
    static void ClearDungeonList();
    static void SetLFGDungeon(uint32_t slot);
    static void JoinLFG();
    static LFGRewardItem* GetDungeonRewardItem(uint32_t slot, int32_t index);
    static void RequestLFGPlayerInfo();
    static void ClearPartyInfo();
    static uint8_t GetClassRoles(uint32_t classID);
    static bool HasRaidDungeon(LFGInfo* info);
    static void UpdateLFDEntries();
    static void UpdateLFREntries();
    static void ShowProposal(int32_t silent);
    static void ParsePartyInfo(CDataStore* msg);
    static void ParseBlackList(CDataStore* msg, LFGLockInfo* lockInfo);
    static void UpdateRandomDungeons();
    static bool IsRandomDungeonUnlocked(const LfgDungeonsRec* rec);
    static bool UpdateRandomDungeonBestChoice();
    static bool IsSlotValidForAll(uint32_t slot);
    static void RequestLFGPartyInfo();
    static void SetLFGComment(const char* comment);
    static void LeaveLFGSearch();
    static void LeaveLFG();
    static int32_t GetLFGProposalMember(lua_State* L);
    static int32_t GetLFGProposalEncounter(lua_State* L);
    static void ProposalResponse(bool accept);
    static int32_t GetLFGRoleUpdateMember(lua_State* L);
    static int32_t SetLFGBootVote(lua_State* L);
    static int32_t GetLFGBootProposal(lua_State* L);
    static bool SetHeaderCollapsed(uint32_t header, bool collapsed);
    static bool SetDungeonEnabled(uint32_t dungeon, bool enabled);
    static void JoinLFGSearch(int32_t lfgID);
    static void ClearLFGDungeon(uint32_t slot);
    static int32_t GetQueueStatusIndex();
    static void ResortLFGLists();
};

LfgDungeonExpansionRec* GetLfgDungeonExpansionRec(int32_t dungeonID, int32_t expansionLevel);
int32_t LFGPacketHandler(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_LFG_OPEN_FROM_GOSSIP(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_LFG_SEARCH_RESULTS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

void LookingForGroupRegisterScriptFunctions();
void LookingForGroupUnregisterScriptFunctions();

void DungeonEncounterCountMatchingRows(int32_t mapId, int32_t difficulty, uint32_t completedMask, uint32_t* total, uint32_t* completed);
int32_t PushDungeonEncounterInfo(lua_State* L, int32_t mapId, int32_t difficulty, int32_t index, uint32_t completedMask);
uint32_t FilterRolesByClass(uint8_t roles);

#endif
