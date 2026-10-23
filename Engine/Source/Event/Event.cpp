#include <cstdint>
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Event/EvtContext.h"
#include "Engine/Source/Event/EvtInput.h"
#include "Engine/Source/Event/EvtQueue.h"
#include "Engine/Source/Event/EvtSched.h"
#include "Engine/Source/Event/EvtTimer.h"
#include "Engine/Source/Event/Observer.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 849b60d4 auto
// OFFSET: 0x0047CFF0
void EventInitialize(uint32_t threadCount, int32_t netServer) {
    ObserverInitialize();
    InputObserverInitialize();
    IEvtQueueInitialize();
    IEvtInputInitialize();

    if (threadCount < 1) {
        threadCount = 1;
    }

    IEvtSchedulerInitialize(threadCount, netServer);
    OsInputSetEventPollProc(EventForceIdleProcessing);
}

// VERIFY: 0930ea91 auto
// OFFSET: 0x0047D030
void EventDestroy() {
    OsInputSetEventPollProc(nullptr);
    IEvtSchedulerDestroy();
    IEvtInputDestroy();
    IEvtQueueDestroy();
    InputObserverDestroy();
    ObserverDestroy();
}

// VERIFY: c7e2732f auto
// OFFSET: 0x0047D060
void EventDoMessageLoop() {
    IEvtSchedulerProcess();
}

// VERIFY: 98e52828 auto
// OFFSET: 0x0047D070
void EventInitiateShutdown() {
    IEvtSchedulerShutdown();
}

// VERIFY: e30a266b auto
// OFFSET: 0x0047D080
HEVENTCONTEXT EventCreateContextEx(int32_t interactive, EVENTHANDLERFUNC initializeHandler, EVENTHANDLERFUNC destroyHandler, uint32_t idleTime, uint32_t debugFlags) {
    return IEvtSchedulerCreateContext(interactive, initializeHandler, destroyHandler, idleTime, debugFlags);
}

// VERIFY: 592fa8a9 auto
// OFFSET: 0x0047D090
void EventSetConfirmCloseCallback(int32_t (*callback)(void*), void* param) {
    IEvtInputSetConfirmCloseCallback(callback, param);
}

// VERIFY: 6747ebf5 auto
// OFFSET: 0x0047D0D0
HEVENTCONTEXT EventGetCurrentContext() {
    return static_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
}

// VERIFY: 627859ff auto
// OFFSET: 0x0047D1D0
int32_t EventIsButtonDown(MOUSEBUTTON button) {
    int32_t result = 0;
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        result = IEvtQueueCheckSyncMouseState(context, button);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }

    return result;
}

// VERIFY: d92bb7e9 auto
// OFFSET: 0x0047D230
int32_t EventIsKeyDown(KEY key) {
    int32_t result = 0;
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        result = IEvtQueueCheckSyncKeyState(context, key);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }

    return result;
}

// VERIFY: 21d82365 manual
// OFFSET: 0x0047D290
void EventPostCloseEx(HEVENTCONTEXT contextHandle) {
    if (!contextHandle) {
        contextHandle = static_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
    }

    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(contextHandle)), 0, &findMask, 0, 1);

    if (context) {
        CloseContext(context);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }
}

// VERIFY: 485045ba manual
// OFFSET: 0x0047D310
int32_t EventQueuePost(HEVENTCONTEXT contextHandle, EVENTID id, const void* data, uint32_t bytes) {
    if (!contextHandle) {
        contextHandle = static_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
    }

    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(contextHandle)), 0, &findMask, 0, 1);

    if (!context) {
        return 0;
    }

    context->m_critsect.Enter();
    int32_t destroyed = context->m_schedState == SCHEDSTATE_DESTROYED;
    context->m_critsect.Leave();

    int32_t result;

    if (destroyed) {
        result = 0;
    } else {
        IEvtQueuePost(context, id, data, bytes);
        result = 1;
    }

    if (findMask != -1) {
        TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
    }

    return result;
}

// VERIFY: cfabbd7f manual
// OFFSET: 0x0047D3C0
void EventRegisterEx(EVENTID id, EVENTHANDLERFUNC handler, void* param, float priority) {
    if (id < 0 || id >= EVENTIDS || !handler) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        IEvtQueueRegister(context, id, handler, param, priority);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }
}

// VERIFY: f02987f7 auto
// OFFSET: 0x0047D450
void EventUnregisterEx(EVENTID id, EVENTHANDLERFUNC handler, void* param, uint32_t flags) {
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        IEvtQueueUnregister(context, id, handler, param, flags);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }
}

// VERIFY: cd498935 auto
// OFFSET: 0x0047D4C0
int32_t EventInputProcess(HEVENTCONTEXT contextHandle) {
    int32_t result = 0;

    if (!contextHandle) {
        contextHandle = static_cast<HEVENTCONTEXT>(PropGet(PROP_EVENTCONTEXT));
    }

    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(contextHandle)), 0, &findMask, 0, 1);

    if (context) {
        int32_t shutdown;
        result = IEvtInputProcess(context, &shutdown);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }

    return result;
}

// VERIFY: 7b49de2a auto
// OFFSET: 0x0047D530
uint32_t EventSetTimer(uint32_t delay, EVENTHANDLERFUNC handler, void* param) {
    uint32_t result = 0;
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        result = IEvtTimerSet(context, delay, handler, param, nullptr, 0, nullptr);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }

    return result;
}

// VERIFY: fe0bc4a4 auto
// OFFSET: 0x0047D5A0
uint32_t EventSetTimer(uint32_t delay, EVENTHANDLERGUIDFUNC handler, uint64_t guid, void* param) {
    uint32_t result = 0;
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        result = IEvtTimerSet(context, delay, nullptr, nullptr, handler, guid, param);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }

    return result;
}

// VERIFY: 613a3b19 auto
// OFFSET: 0x0047D620
void EventKillTimer(uint32_t id, EVENTHANDLERFUNC handler, const char* name) {
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        IEvtTimerKill(context, id, handler, name);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }
}

// VERIFY: 0b8bef1a auto
// OFFSET: 0x0047D690
void EventSetMouseMode(MOUSEMODE mode, uint32_t holdButton) {
    if (mode >= MOUSE_MODES) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        IEvtInputSetMouseMode(context, mode, holdButton);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }
}

// VERIFY: d1d149d8 auto
// OFFSET: 0x0047D700
void EventInputGetMousePosition(float* x, float* y) {
    int32_t findMask;
    EvtContext* context = TSingletonInstanceId<EvtContext, 8>::s_idTable.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PropGet(PROP_EVENTCONTEXT))), 0, &findMask, 0, 1);

    if (context) {
        IEvtInputGetMousePosition(x, y);

        if (findMask != -1) {
            TSingletonInstanceId<EvtContext, 8>::s_idTable.Unlock(findMask & 7, findMask >= 8);
        }
    }
}

// VERIFY: 16a0da59 auto
// OFFSET: 0x0047D760
void EventPostClose() {
    EventPostCloseEx(nullptr);
}

// VERIFY: 8763f0a0 auto
// OFFSET: 0x0047D770
void EventRegister(EVENTID id, EVENTHANDLERFUNC handler) {
    EventRegisterEx(id, handler, nullptr, EVENT_PRIORITY_NORMAL);
}

// VERIFY: b58fec50 auto
// OFFSET: 0x0047D790
void EventUnregister(EVENTID id, EVENTHANDLERFUNC handler) {
    EventUnregisterEx(id, handler, nullptr, 0xFFFFFFFF);
}
