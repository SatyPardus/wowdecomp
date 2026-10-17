#include <cstdint>
#include <typeinfo>
#include "Engine/Source/Base/RefCount.h"
#include "Engine/Source/Event/Observer.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B4186C
static TRefCnt* s_inputObserver;

// STATIC: 0x00B41874
static LockedDataAllocator s_eventRegAllocator(sizeof(EventReg), 1024);

// STATIC: 0x00B418B0
static LockedDataAllocator s_callbackRegAllocator(sizeof(EventReg::EVENTCALLBACKREG), 256);

// STATIC: 0x00B418EC
static LockedDataAllocator s_dispatchRegAllocator(sizeof(EventReg::EVENTDISPATCHREG), 1024);

void InputObserverInitialize() {
}

// VERIFY: 16889398 manual
// OFFSET: 0x00480E70
void InputObserverDestroy() {
    if (s_inputObserver && --s_inputObserver->m_refcnt == 0) {
        s_inputObserver->DeleteSelf();
    }

    s_inputObserver = nullptr;
}

void ObserverInitialize() {
}

// VERIFY: 42d23e57 auto
// OFFSET: 0x00480EA0
void ObserverDestroy() {
    s_eventRegAllocator.m_allocator.Clear(STORM_TYPE_NAME(EventReg), SLOG_OBJECT, 1);
    s_callbackRegAllocator.m_allocator.Clear(STORM_TYPE_NAME(EventReg::EVENTCALLBACKREG), SLOG_OBJECT, 1);
    s_dispatchRegAllocator.m_allocator.Clear(STORM_TYPE_NAME(EventReg::EVENTDISPATCHREG), SLOG_OBJECT, 1);
}
