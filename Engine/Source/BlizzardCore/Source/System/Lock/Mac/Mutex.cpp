#if defined(__APPLE__)

#include <new>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"

void Blizzard::Lock::MutexCreate(Mutex& mutex) {
    new (&mutex) Mutex();
}

void Blizzard::Lock::MutexEnter(Mutex& mutex) {
    mutex.lock();
}

void Blizzard::Lock::MutexLeave(Mutex& mutex) {
    mutex.unlock();
}

void Blizzard::Lock::MutexDestroy(Mutex& mutex) {
    pthread_mutex_destroy(&mutex.m_mutex);
}

Blizzard::Lock::ScopedMutex::ScopedMutex(Mutex& mutex) : m_mutex(&mutex) {
    MutexEnter(mutex);
}

Blizzard::Lock::ScopedMutex::~ScopedMutex() {
    MutexLeave(*this->m_mutex);
}

Blizzard::Lock::RMutex::RMutex() {
    MutexCreate(this->m_mutex);
}

Blizzard::Lock::RMutex::~RMutex() {
    MutexDestroy(this->m_mutex);
}

#endif
