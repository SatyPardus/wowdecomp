#ifndef ENGINE_SOURCE_EVENT_OBSERVER_H
#define ENGINE_SOURCE_EVENT_OBSERVER_H

#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Storm/Source/SLock.h"

struct EventReg {
    struct EVENTCALLBACKREG;
    struct EVENTDISPATCHREG;

    /* 0000 */ uint8_t unused_0000[0x34];
};

struct EventReg::EVENTCALLBACKREG {
    /* 0000 */ uint8_t unused_0000[0x10];
};

struct EventReg::EVENTDISPATCHREG {
    /* 0000 */ uint8_t unused_0000[0x10];
};

struct LockedDataAllocator {
    /* 0000 */ CDataAllocator m_allocator;
    /* 0014 */ SCritSect m_critsect;

    LockedDataAllocator(uint32_t bytesPerData, uint32_t dataPerBlock);
};

void InputObserverDestroy();
void InputObserverInitialize();
void ObserverDestroy();
void ObserverInitialize();

// INLINED
inline LockedDataAllocator::LockedDataAllocator(uint32_t bytesPerData, uint32_t dataPerBlock) : m_allocator(bytesPerData, dataPerBlock) {
}

#endif
