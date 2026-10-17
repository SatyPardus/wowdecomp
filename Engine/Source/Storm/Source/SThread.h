#ifndef ENGINE_SOURCE_STORM_SOURCE_STHREAD_H
#define ENGINE_SOURCE_STORM_SOURCE_STHREAD_H

#include <cstddef>
#include <cstdint>
#if defined(__APPLE__)
#include "Engine/Source/Storm/Source/Apple/SThread.h"
#else
#include <windows.h>
#include "Engine/Source/Storm/H/SCritSect.h"
#include "Engine/Source/Storm/Source/SLock.h"

class SThread : public SSyncObject {
    public:
    static int32_t Create(uint32_t (STORMAPI* threadProc)(void*), void* param, SThread& thread, char* threadName, uint32_t stackSize);

    int32_t SetPriority(int32_t priority);
};

class CInitCritSect {
    public:
    /* 0000 */ LONG m_spinLock;
    /* 0004 */ CCritSect* m_critsect;
    /* 0008 */ uint8_t m_critsectData[sizeof(CCritSect)];

    int32_t Enter();
    void Leave();
};

namespace S_Thread {
    struct SThreadTrack {
        /* 0000 */ int32_t suspended;
        /* 0004 */ int32_t live;
        /* 0008 */ uint32_t threadId;
        /* 000C */ HANDLE threadH;
        /* 0010 */ char name[16];
    };

    const int32_t s_maxthreads = 128;

    extern SThreadTrack s_threads[s_maxthreads];
    extern int32_t s_numthreads;
    extern CInitCritSect s_threadCrit;

    uint32_t STORMAPI s_SLaunchThread(void* threadParam);
}

#endif // __APPLE__
#endif
