#ifndef WOWDECOMP_PLATFORM_COMPAT_RECURSIVEMUTEX_H
#define WOWDECOMP_PLATFORM_COMPAT_RECURSIVEMUTEX_H

#include <pthread.h>

// Stand-in for the Win32 CRITICAL_SECTION. Like CRITICAL_SECTION it has no destructor and is only torn down
// explicitly (MutexDestroy / DeleteCriticalSection paths), so global instances stay usable by threads that
// are still running while exit() runs static destructors. Constant-initialized, so it is valid before any
// dynamic initializer runs.
struct WowRecursiveMutex {
    pthread_mutex_t m_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

    void lock() {
        pthread_mutex_lock(&this->m_mutex);
    }

    bool try_lock() {
        return pthread_mutex_trylock(&this->m_mutex) == 0;
    }

    void unlock() {
        pthread_mutex_unlock(&this->m_mutex);
    }
};

#endif
