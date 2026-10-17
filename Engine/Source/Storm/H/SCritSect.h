#ifndef ENGINE_SOURCE_STORM_H_SCRITSECT_H
#define ENGINE_SOURCE_STORM_H_SCRITSECT_H

#include <cstdint>
#if defined(__APPLE__)
#include <pthread.h>
#else
#include <windows.h>
#endif

class CCritSect {
    public:
#if defined(__APPLE__)
    pthread_mutex_t m_critsect;
#else
    /* 0000 */ CRITICAL_SECTION m_critsect;
#endif

    CCritSect();
    ~CCritSect();
    void Enter();
    void Enter(int32_t forwriting);
    void Leave();
    void Leave(int32_t fromwriting);
};

#endif
