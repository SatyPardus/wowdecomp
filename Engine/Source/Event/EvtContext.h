#ifndef ENGINE_SOURCE_EVENT_EVTCONTEXT_H
#define ENGINE_SOURCE_EVENT_EVTCONTEXT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/TExtraInstanceRecycler.h"
#include "Engine/Source/Base/TInstanceIdTable.h"
#include "Engine/Source/Base/Types.h"
#include "Engine/Source/Event/CSBasePriorityQueue.h"
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SLock.h"

struct EvtHandler {
    /* 0000 */ TSLink<EvtHandler> m_link;
    /* 0008 */ EVENTHANDLERFUNC m_func;
    /* 000C */ void* m_param;
    /* 0010 */ float m_priority;
    /* 0014 */ int32_t m_marker;
};

struct EvtKeyDown {
    /* 0000 */ TSLink<EvtKeyDown> m_link;
    /* 0008 */ KEY m_key;
};

struct EvtMessage : public TExtraInstanceRecyclable {
    /* 0004 */ TSLink<EvtMessage> m_link;
    /* 000C */ EVENTID m_id;
    /* 0010 */ char m_data[4];
};

struct EvtTimer {
    /* 0000 */ uint32_t m_id;
    /* 0004 */ TSTimerPriority<uint32_t> m_targetTime;
    /* 0014 */ float m_timeout;
    /* 0018 */ EVENTHANDLERFUNC m_handler;
    /* 001C */ void* m_param;
    /* 0020 */ EVENTHANDLERGUIDFUNC m_guidHandler;
    /* 0024 */ uint32_t unused_0024;
    /* 0028 */ uint64_t m_guidParam;
    /* 0030 */ void* m_guidParam2;
    /* 0034 */ uint32_t unused_0034;
};

template <class T>
class EvtIdTable {
    public:
    /* 0000 */ TSGrowableArray<T> m_allocArray;
    /* 0010 */ TSGrowableArray<uint32_t> m_freeArray;

    uint32_t Alloc();
    void Free(uint32_t id);
};

template <class T>
class TInstanceId : public TSLinkedNode<T> {
    public:
    /* 000C */ uint32_t m_id;

    TInstanceId();
    virtual ~TInstanceId();
};

template <class T, int bucketCount>
class TSingletonInstanceId : public TInstanceId<T> {
    public:
    static TInstanceIdTable<T, bucketCount> s_idTable;
};

class EvtContext : public TSingletonInstanceId<EvtContext, 8> {
    public:
    /* 0010 */ SCritSect m_critsect;
    /* 0038 */ uint32_t m_currTime;
    /* 003C */ SCHEDSTATE m_schedState;
    /* 0040 */ TSTimerPriority<uint32_t> m_schedNextWakeTime;
    /* 0050 */ uint32_t m_schedLastIdle;
    /* 0054 */ uint32_t m_schedFlags;
    /* 0058 */ uint32_t m_schedIdleTime;
    /* 005C */ uint32_t m_schedInitialIdleTime;
    /* 0060 */ uint32_t m_schedWeight;
    /* 0064 */ uint32_t m_schedSmoothWeight;
    /* 0068 */ int32_t m_schedRebalance;
    /* 006C */ TSExplicitList<EvtHandler, offsetof(EvtHandler, m_link)> m_queueHandlerList[EVENTIDS];
    /* 021C */ TSExplicitList<EvtMessage, offsetof(EvtMessage, m_link)> m_queueMessageList;
    /* 0228 */ uint32_t m_queueSyncButtonState;
    /* 022C */ TSExplicitList<EvtKeyDown, offsetof(EvtKeyDown, m_link)> m_queueSyncKeyDownList;
    /* 0238 */ EvtIdTable<EvtTimer*> m_timerIdTable;
    /* 0258 */ TSPriorityQueue<EvtTimer> m_timerQueue;
    /* 026C */ HPROPCONTEXT m_propContext;
    /* 0270 */ void* m_callContext;
    /* 0274 */ int32_t m_startWatchdog;

    EvtContext(uint32_t flags, uint32_t idleTime, uint32_t schedWeight, void* callContext, int32_t startWatchdog);
    ~EvtContext();
};

// INLINED
template <class T>
TInstanceId<T>::TInstanceId() {
    this->m_id = 0;
}

// VERIFY: 3b380357 auto
// OFFSET: [0x0047E510<EvtContext>]
template <class T>
TInstanceId<T>::~TInstanceId() {
}

// STATIC: 0x00B41670
template <class T, int bucketCount>
TInstanceIdTable<T, bucketCount> TSingletonInstanceId<T, bucketCount>::s_idTable;

// INLINED
template <class T>
uint32_t EvtIdTable<T>::Alloc() {
    uint32_t id;

    if (this->m_freeArray.Count()) {
        id = this->m_freeArray.m_data[this->m_freeArray.Count() - 1];
        this->m_freeArray.SetCount(this->m_freeArray.Count() - 1);
    } else {
        id = this->m_allocArray.Count();

        if (id < 1) {
            id = 1;
        }

        this->m_allocArray.GrowToFit(id, 1);
    }

    return id;
}

// INLINED
template <class T>
void EvtIdTable<T>::Free(uint32_t id) {
    this->m_freeArray.New(id);
}

#endif
