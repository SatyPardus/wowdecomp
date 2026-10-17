#pragma once

#include "Engine/Source/Storm/Source/SLock.h"

class SThread : public SSyncObject {
public:
    static int32_t Create(uint32_t (STORMAPI* threadProc)(void*), void* param, SThread& thread, char* threadName, uint32_t stackSize);
    int32_t SetPriority(int32_t priority);
};

class CInitCritSect {
    CCritSect m_critsect;
    bool m_initialized = false;
public:
    int32_t Enter();
    void Leave();
};
