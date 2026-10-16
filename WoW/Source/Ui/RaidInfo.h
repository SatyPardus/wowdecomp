#ifndef WOW_SOURCE_UI_RAIDINFO_H
#define WOW_SOURCE_UI_RAIDINFO_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Ui/PartyFrame.h"

class CDataStore;
class CGPlayer_C;

uint32_t GetRaidTargetIndexFromGuid(WOWGUID guid);
struct lua_State;

class CGRaidMember {
    public:
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint32_t m_subgroup;
    /* 000C */ int32_t m_rank;
    /* 0010 */ CGPartyMemberState m_stats;
    /* 0320 */ int32_t m_readyCheckPending;
    /* 0324 */ int32_t m_readyCheckReady;
    /* 0328 */ uint8_t m_assignments;
    /* 0329 */ uint8_t unused_0329[3];
    /* 032C */ uint32_t m_groupRole;
};

class CGRaidInfo {
    public:
    static void LeaveWorld();
    static CGRaidMember* m_members[40];
    static uint32_t m_numMembers;
    static WOWGUID m_raidTargets[8];
    static uint32_t m_realNumMembers;
    static WOWGUID m_selectedMember;

    static int32_t IsPlayerOrPetInRaid(const WOWGUID& guid);
    static uint32_t m_readyCheckTime;
    static WOWGUID m_readyCheckInitiator;
    static uint32_t m_readyCheckTimer;
    static uint8_t m_rosterCounter;
    static int32_t m_partyReadyCheckReady[4];
    static int32_t m_partyReadyCheckPending[4];

    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static int32_t ProcessReadyCheckTimer(const void* data, void* param);
    static int32_t GetPartyMemberIndex(WOWGUID guid);
    static int32_t IsPartyMemberReadyCheckPending(uint32_t index);
    static int32_t IsPartyMemberReadyCheckReady(uint32_t index);
    static CGPartyMemberState* GetRemoteStats(const WOWGUID& guid);
    static void ClearRecruitAFriend(WOWGUID guid);
    static CGPartyMemberState* GetRemoteStatsFromPet(const WOWGUID& petGuid);
    static CGRaidMember* FindMemberByGuid(WOWGUID guid);
    static CGPartyPetState* GetRemotePetStats(const WOWGUID& guid);

    static void EnableMember(CGPlayer_C* player, int32_t enable);
    static void UpdateRealStats(uint32_t count, WOWGUID leader);
    static void ClearRoster();
    static void CheckReadyCheckTime();
    static void InitiateReadyCheck(const char* name);
    static void SetRaidTarget(uint32_t index, const WOWGUID& guid);
    static void RosterUpdateRefreshReadyCheck(uint32_t removed, WOWGUID leader);
    static void UpdateRoster(const WOWGUID* guids, const uint32_t* subgroups, const uint8_t* flags, const uint32_t* roles, const uint8_t* online, uint32_t count, uint8_t battleground);
    static int32_t IsPetInRaid(const WOWGUID& guid, int32_t* index);
    static int32_t IsPlayerRaidOfficer(const WOWGUID& guid);
    static int32_t IsPVPRaid();
    static int32_t GetMemberIndexByGuid(WOWGUID guid);
    static CGRaidMember* GetMember(uint32_t index);
    static int32_t GetSelectedMemberIndex();
    static void SendReadyCheck();
    static uint8_t GetPartyAssignments(const WOWGUID& guid);
    static int32_t GetPartyAssignments(lua_State* L, int32_t assignment);
    static void GetMapNameWithDifficulty(char* buffer, uint32_t size, int32_t mapId, int32_t difficulty);
};

int32_t Script_DoReadyCheck(lua_State* L);
void RaidInfoRegisterScriptFunctions();
void RaidInfoUnregisterScriptFunctions();

void RaidRosterNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
int32_t Packet_MSG_RAID_TARGET_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_RAID_READY_CHECK(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_RAID_READY_CHECK_CONFIRM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_RAID_READY_CHECK_FINISHED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_RAID_READY_CHECK_ERROR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_NOTIFY_PARTY_SQUELCH(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_ECHO_PARTY_SQUELCH(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

#endif
