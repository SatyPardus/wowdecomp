#ifndef WOW_SOURCE_UI_INSTANCEENCOUNTER_C_H
#define WOW_SOURCE_UI_INSTANCEENCOUNTER_C_H

#include <cstdint>
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"

class CDataStore;

struct ENCOUNTER_UNIT {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint8_t m_priority;
    /* 0009 */ uint8_t m_index;
    /* 000A */ uint8_t unused_000A[6];

    bool operator<(const ENCOUNTER_UNIT& other) const;
    static int32_t Compare(const void* a, const void* b);
};

struct ENCOUNTER_TIMER : public TSLinkedNode<ENCOUNTER_TIMER> {
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t m_expireTime;
};

struct ENCOUNTER_OBJECTIVE : public TSLinkedNode<ENCOUNTER_OBJECTIVE> {
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint8_t m_id;
    /* 0010 */ int32_t m_progress;
};

class CGInstanceEncounter_C {
    public:
    static void EnterWorld();
    static void LeaveWorld();
    static WOWGUID GetBoss(uint32_t index);
    static uint32_t GetNumBossSlots();
    static void EngageUnit(const WOWGUID& guid, uint8_t priority);
    static void DisengageUnit(const WOWGUID& guid);
    static void ClearExpiredTimers();
    static void AddTimer(uint32_t duration);
    static void EnableObjective(int32_t id);
    static void DisableObjective(int32_t id);
    static TSFixedArray<ENCOUNTER_UNIT> m_units;
    static TSList<ENCOUNTER_TIMER> m_timers;
    static TSList<ENCOUNTER_OBJECTIVE> m_objectives;
};

int32_t Packet_SMSG_UPDATE_INSTANCE_ENCOUNTER_UNIT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

#endif
