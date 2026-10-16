#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_LOCK_LOCK_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_LOCK_LOCK_H

#include <cstdint>
#include <new>
#if defined(_WIN32)
#include <windows.h>
#else
#include <mutex>
#include "Engine/Source/Platform/Compat/RecursiveMutex.h"
#endif
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"

namespace Blizzard {
namespace Lock {

#if defined(_WIN32)
typedef HANDLE Event;
typedef CRITICAL_SECTION Mutex;
#else
struct EventState;
typedef EventState* Event;
typedef WowRecursiveMutex Mutex;
#endif

class Atomic {
    public:
    /* 0000 */ volatile int32_t m_value;

    int32_t Decrement();
    int32_t Increment();
};

template <class T>
class ConstructOnce {
    public:
    static void Construct(void* param);

    /* 0000 */ bool m_constructed;
    /* 0001 */ uint8_t unused_0001[3];
    /* 0004 */ Atomic m_count;
    /* 0008 */ alignas(T) uint8_t m_storage[sizeof(T)];

    T* Get();
};

class EventObject {
    public:
    /* 0000 */ Event m_event;

    EventObject();
    ~EventObject();
};

class RMutex {
    public:
    /* 0000 */ Mutex m_mutex;

    RMutex();
    ~RMutex();
};

class ScopedMutex {
    public:
    /* 0000 */ Mutex* m_mutex;

    ScopedMutex(Mutex& mutex);
    ~ScopedMutex();
};

void EventCreate(Event& event);
void EventDestroy(Event& event);
void EventSet(Event& event);
Event* EventWait(uint32_t timeout, Event& event);
void EventWaitForever(Event& event);
void MutexCreate(Mutex& mutex);
void MutexDestroy(Mutex& mutex);
void MutexEnter(Mutex& mutex);
void MutexLeave(Mutex& mutex);

}
}

// VERIFY: ba79b1ff manual
// OFFSET: 0x004511B0
template <class T>
void Blizzard::Lock::ConstructOnce<T>::Construct(void* param) {
    new (static_cast<ConstructOnce<T>*>(param)->m_storage) T();
}

// INLINED
template <class T>
T* Blizzard::Lock::ConstructOnce<T>::Get() {
    if (!this->m_constructed) {
        if (this->m_count.Increment() == 1) {
            Construct(this);
            this->m_constructed = true;
        } else {
            while (!this->m_constructed) {
                Blizzard::Process::Sleep(1);
            }
        }
    }

    return reinterpret_cast<T*>(this->m_storage);
}

#endif
