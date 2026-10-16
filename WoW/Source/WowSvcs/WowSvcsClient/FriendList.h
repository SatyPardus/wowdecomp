#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FRIENDLIST_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FRIENDLIST_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
struct lua_State;

struct FriendInfo {
    /* 0000 */ uint8_t m_connected;
    /* 0001 */ uint8_t m_flags;
    /* 0004 */ char* m_name;
    /* 0008 */ char m_notes[512];
    /* 0208 */ WOWGUID m_guid;
    /* 0210 */ uint32_t m_level;
    /* 0214 */ uint32_t m_class;
    /* 0218 */ uint32_t m_area;
    /* 021C */ uint8_t m_deleted;

    FriendInfo();
    ~FriendInfo();
};

struct IgnoreInfo {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint8_t m_flags;
};

class CGPlayer_C;

struct WhoResult {
    /* 0000 */ char m_name[48];
    /* 0030 */ char m_guild[96];
    /* 0090 */ int32_t m_level;
    /* 0094 */ int32_t m_race;
    /* 0098 */ int32_t m_class;
    /* 009C */ int32_t m_gender;
    /* 00A0 */ int32_t m_zone;
};

struct WhoSortField {
    /* 0000 */ int32_t m_field;
    /* 0004 */ int32_t m_reverse;
};

class FriendList {
    public:
    static void __stdcall AddFriend(const char* name, const char* note);
    static FriendList* s_friendList;
    static WhoSortField s_whoSort[7];
    static uint32_t s_numWhoResults;
    static uint32_t s_totalWhoResults;
    static WhoResult s_whoResults[50];
    static int32_t s_whoToUI;

    /* 0000 */ FriendInfo m_friends[100];
    /* D480 */ uint32_t m_pendingFriendNames;
    /* D484 */ uint32_t unused_D484;
    /* D488 */ WOWGUID m_selectedFriend;
    /* D490 */ IgnoreInfo m_ignores[50];
    /* D7B0 */ uint32_t m_pendingIgnoreNames;
    /* D7B4 */ uint32_t unused_D7B4;
    /* D7B8 */ WOWGUID m_selectedIgnore;
    /* D7C0 */ IgnoreInfo m_mutes[50];
    /* DAE0 */ uint32_t m_pendingMuteNames;
    /* DAE4 */ uint32_t unused_DAE4;
    /* DAE8 */ WOWGUID m_selectedMute;
    /* DAF0 */ uint64_t m_complained[32];
    /* DBF0 */ int32_t m_whoMinLevel;
    /* DBF4 */ int32_t m_whoMaxLevel;
    /* DBF8 */ char m_whoName[48];
    /* DC28 */ char m_whoGuild[96];
    /* DC88 */ int32_t m_whoRaceMask;
    /* DC8C */ int32_t m_whoClassMask;
    /* DC90 */ int32_t m_whoStringCount;
    /* DC94 */ char m_whoStrings[10][128];
    /* E194 */ uint8_t unused_E194[4];

    FriendList();
    ~FriendList();

    FriendInfo* GetFriend(WOWGUID guid);
    void SendWho(const char* filter);
    int32_t DoLocalWho();
    bool AllMatchFilter(CGPlayer_C* player);
    static bool __stdcall MatchesFilter(CGPlayer_C* player, const char* filter);
    FriendInfo* ClearRecruitAFriend(WOWGUID guid);
    uint32_t GetNumFriends();
    uint32_t GetNumIgnores();

    static void Destroy();
    static void Initialize();
    static void RegisterScriptFunctions();
    static void UnregisterScriptFunctions();

    void ReadPacket(CDataStore* msg);
    bool IsIgnored(WOWGUID guid, bool includeComplained);
    bool IsMuted(WOWGUID guid);
    bool IsIgnoredOrMuted(WOWGUID guid);
    bool IsComplained(WOWGUID guid);
    void AddComplained(WOWGUID guid);
    bool IsFriend(WOWGUID guid);
    static void __stdcall RemoveFriend(WOWGUID guid);
    static void __stdcall DelIgnore(WOWGUID guid);
    static void __stdcall DelVoiceIgnore(WOWGUID guid);
    bool GetFriendDeleted(WOWGUID guid);
    void SetFriendDeleted(WOWGUID guid, uint8_t deleted);
    void SortIgnoreList();
    void SortMuteList();
    void FriendRemoved(WOWGUID guid);
    uint32_t GetFriendIndexByGuid(WOWGUID guid);
    void SetName(WOWGUID guid, const char* name);
    void SetConnected(WOWGUID guid, uint8_t connected);
    void DecrementPendingFriendName();
    void DecrementPendingIgnoreName();
    void DecrementPendingMuteName();
    bool GetIgnoreDeleted(WOWGUID guid);
    void SetIgnoreDeleted(WOWGUID guid, uint8_t deleted);
    bool GetMuteDeleted(WOWGUID guid);
    void SetMuteDeleted(WOWGUID guid, uint8_t deleted);
    void IgnoreRemoved(WOWGUID guid);
    void MuteRemoved(WOWGUID guid);
    void IgnoreAdded(WOWGUID guid, int32_t lookupName);
    void MuteAdded(WOWGUID guid, int32_t lookupName);
    void HandleStatus(uint8_t status, WOWGUID guid, CDataStore* msg);
    FriendInfo* GetFriendInfoByName(const char* name);
    int32_t GetFriendSelectionIndex();
    uint32_t GetNumMutes();
    uint64_t GetIgnore(uint32_t index);
    int32_t GetIgnoreSelectionIndex();
    int32_t GetMuteSelectionIndex();
    void SetNotes(WOWGUID guid, const char* notes);
    void AddOrDelIgnore(const char* name);
    void AddIgnore(const char* name);
    void AddOrDelVoiceIgnore(const char* name);
    void AddVoiceIgnore(const char* name);
    void AddOrRemoveFriend(const char* name, const char* notes);
    void RemoveFriend(const char* name);
    void SetFriendNotes(const char* name, const char* notes);
    void DelIgnore(const char* name);
    void DelMute(const char* name);
};

int32_t QSortFriends(const void* a, const void* b);
int32_t QSortIgnore(const void* a, const void* b);
int32_t CompareWhoResults(const void* a, const void* b);
void FriendListNameCallbackWithSort(int32_t id, const WOWGUID& guid, void* param, bool success);
void FriendListIgnoreNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void FriendListMuteNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void FriendListNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void FriendListModuleLoaded();
void LoadEncryptedModule(const uint8_t* key, uint32_t length, void (*callback)());
int32_t Packet_SMSG_WHO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_WHOIS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_RWHOIS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_CONTACT_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_FRIEND_STATUS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t CCommand_Whois(const char* command, const char* arguments);

#endif
