#if defined(__APPLE__)
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/Storm/H/SCritSect.h"

namespace Blizzard::Lock {
struct EventState {
    std::mutex mutex;
    std::condition_variable changed;
    bool signaled = false;
};

static void Check(int result) {
    if (result != 0) {
        std::abort();
    }
}

int32_t Atomic::Increment() { return __atomic_add_fetch(&m_value, 1, __ATOMIC_SEQ_CST); }
int32_t Atomic::Decrement() { return __atomic_sub_fetch(&m_value, 1, __ATOMIC_SEQ_CST); }

void MutexCreate(Mutex& mutex) {
    pthread_mutexattr_t attributes;
    Check(pthread_mutexattr_init(&attributes));
    // Win32 critical sections are recursive, including CCritSect's read mode.
    Check(pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE));
    Check(pthread_mutex_init(&mutex, &attributes));
    Check(pthread_mutexattr_destroy(&attributes));
}
void MutexDestroy(Mutex& mutex) { Check(pthread_mutex_destroy(&mutex)); }
void MutexEnter(Mutex& mutex) { Check(pthread_mutex_lock(&mutex)); }
void MutexLeave(Mutex& mutex) { Check(pthread_mutex_unlock(&mutex)); }
ScopedMutex::ScopedMutex(Mutex& mutex) : m_mutex(&mutex) { MutexEnter(mutex); }
ScopedMutex::~ScopedMutex() { MutexLeave(*m_mutex); }
RMutex::RMutex() { MutexCreate(m_mutex); }
RMutex::~RMutex() { MutexDestroy(m_mutex); }

void EventCreate(Event& event) { event = new EventState; }
void EventDestroy(Event& event) { delete event; event = nullptr; }
void EventSet(Event& event) {
    std::lock_guard<std::mutex> lock(event->mutex);
    event->signaled = true;
    event->changed.notify_one();
}
Event* EventWait(uint32_t timeout, Event& event) {
    std::unique_lock<std::mutex> lock(event->mutex);
    if (timeout == UINT32_MAX) {
        event->changed.wait(lock, [&] { return event->signaled; });
    } else if (!event->changed.wait_for(lock, std::chrono::milliseconds(timeout), [&] { return event->signaled; })) {
        return nullptr;
    }
    event->signaled = false; // auto-reset, with repeated Set calls coalesced
    return &event;
}
void EventWaitForever(Event& event) { EventWait(UINT32_MAX, event); }
EventObject::EventObject() { EventCreate(m_event); }
EventObject::~EventObject() { EventDestroy(m_event); }
}

void Blizzard::Process::Sleep(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

CCritSect::CCritSect() { Blizzard::Lock::MutexCreate(m_critsect); }
CCritSect::~CCritSect() { Blizzard::Lock::MutexDestroy(m_critsect); }
void CCritSect::Enter() { Blizzard::Lock::MutexEnter(m_critsect); }
void CCritSect::Enter(int32_t) { Enter(); }
void CCritSect::Leave() { Blizzard::Lock::MutexLeave(m_critsect); }
void CCritSect::Leave(int32_t) { Leave(); }
#endif
