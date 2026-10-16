#ifndef WOW_SOURCE_UI_REPUTATIONINFO_H
#define WOW_SOURCE_UI_REPUTATIONINFO_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;

struct lua_State;
class FactionRec;

struct FactionStanding {
    /* 0000 */ int32_t m_factionId;
    /* 0004 */ uint8_t m_flags;
    /* 0005 */ uint8_t unused_0005[3];
    /* 0008 */ int32_t m_base;
    /* 000C */ int32_t m_standing;
};

struct FactionListEntry {
    /* 0000 */ int32_t m_repListId;
    /* 0004 */ int32_t m_factionId;
    /* 0008 */ int32_t m_isHeader;
    /* 000C */ uint32_t m_headerIndex;
    /* 0010 */ int32_t m_parent;
};

struct FactionHeader {
    /* 0000 */ int32_t m_factionId;
    /* 0004 */ int32_t m_count;
    /* 0008 */ int32_t m_parent;
    /* 000C */ int32_t m_collapsed;
};

struct ForcedReaction {
    /* 0000 */ int32_t m_factionId;
    /* 0004 */ int32_t m_reaction;
};

struct FACTION : public TSHashObject<FACTION, HASHKEY_NONE> {
    /* 0018 */ TSGrowableArray<const FactionRec*> m_children;
};

class CGReputationInfo {
    public:
    static TSGrowableArray<const FactionRec*>* GetFactionChildren(int32_t faction);
    static FactionStanding m_factionStandings[128];
    static const int32_t m_reactionThresholds[9];
    static FactionHeader* m_headers[64];
    static uint32_t m_numHeaders;
    static uint32_t m_numFactions;
    static uint32_t m_numShownFactions;
    static int32_t m_selectedFaction;
    static TSFixedArray<FactionListEntry*> m_factionList;
    static TSGrowableArray<ForcedReaction> m_forcedReactions;
    static TSHashTable<FACTION, HASHKEY_NONE> m_factions;
    static uint32_t m_collapsedHeaders;
    static void EnterWorld();
    static void LeaveWorld();
    static uint32_t AddHeader(int32_t factionId, int32_t parent);
    static uint32_t AddFactionHeader(int32_t factionId);
    static int32_t AddVisibleFaction(uint32_t repListId, int32_t announce);
    static void CollapseHeader(uint32_t index, int32_t collapse);
    static void SortFactions();
    static bool IsFactionInactive(uint32_t index);
    static void InitializeGame();
    static void ShutdownGame();
    static bool GetForcedReaction(int32_t faction, int32_t* reaction);
    static int32_t GetFactionStandingReaction(int32_t faction);
    static bool IsAtWar(int32_t faction);
    static void SetAtWar(int32_t faction, bool atWar);
    static void SetWatchedFaction(int32_t faction);
    static void SetFactionInactive(int32_t faction, bool inactive);
    static int32_t GetSelectedFactionIndex();
};

int32_t GetFactionInfoHelper(lua_State* L, int32_t faction);
void ReputationInfoRegisterScriptFunctions();
void ReputationInfoUnregisterScriptFunctions();
int32_t Packet_SMSG_INITIALIZE_FACTIONS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_SET_FACTION_ATWAR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_SET_FACTION_VISIBLE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_SET_FACTION_STANDING(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_SET_FORCED_REACTIONS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

int32_t GetRepListRepValue(int32_t faction);

#endif
