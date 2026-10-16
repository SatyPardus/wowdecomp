#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_LOOTROLL_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_LOOTROLL_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"

class CDataStore;

struct PendingLootRoll : public TSLinkedNode<PendingLootRoll> {
    /* 0008 */ int32_t m_id;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ WOWGUID m_lootGuid;
    /* 0018 */ uint32_t m_slot;
    /* 001C */ uint32_t m_itemId;
    /* 0020 */ uint32_t m_randomPropertyId;
    /* 0024 */ uint32_t m_randomSuffix;
    /* 0028 */ uint32_t m_count;
    /* 002C */ uint32_t m_expireTime;
    /* 0030 */ uint8_t unused_0030;
    /* 0031 */ uint8_t unused_0031 [3];
    /* 0034 */ uint32_t unused_0034;
    /* 0038 */ WOWGUID m_rollerGuid;
    /* 0040 */ int8_t m_rollNumber;
    /* 0041 */ uint8_t m_showPending;
    /* 0042 */ uint8_t m_finished;
    /* 0043 */ uint8_t unused_0043;
    /* 0044 */ uint32_t m_rollTime;
    /* 0048 */ uint8_t m_voteMask;
    /* 0049 */ uint8_t unused_0049[3];
    /* 004C */ uint32_t m_rollType;
    /* 0050 */ uint8_t m_autoPass;
    /* 0051 */ uint8_t unused_0051;
    /* 0052 */ uint8_t m_reasonNeed;
    /* 0053 */ uint8_t m_reasonGreed;
    /* 0054 */ uint8_t m_reasonDisenchant;
    /* 0055 */ uint8_t unused_0055 [3];
};

extern TSExplicitList<PendingLootRoll, offsetof(PendingLootRoll, m_link)> s_pendingLootRolls;
extern WOWGUID s_masterLootCandidates[40];
extern uint32_t s_numShownLootRolls;

PendingLootRoll* FindRoll(int32_t id);
WOWGUID GetMasterLootCandidate(uint32_t index);
void LootRollShowPending();
void LootRoll(int32_t id, int32_t rollType, bool confirmed);

void CloseAllLootRolls();
void LootRollDestroy();
void LootRollShutdown();
int32_t LootRollMasterList(CDataStore* msg);
int32_t LootRollRolled(CDataStore* msg);
int32_t LootRollStart(CDataStore* msg);
int32_t LootRollAllPassed(CDataStore* msg);
int32_t LootRollWon(CDataStore* msg);

#endif
