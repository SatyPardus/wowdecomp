#pragma once

#include <cstdint>
#include <memory>
#include <pthread.h>
#include "Engine/Source/Storm/H/SCritSect.h"
#include "Engine/Source/Storm/H/storm.h"

namespace StormApple { struct SyncState; }

class SCritSect : public CCritSect {};

class SSyncObject {
public:
    static uint32_t STORMAPI WaitMultiplePtr(uint32_t count, SSyncObject** objects, int32_t waitAll, uint32_t timeout);
    std::shared_ptr<StormApple::SyncState> m_opaqueData;
    SSyncObject() = default;
    SSyncObject(const SSyncObject&) = delete;
    SSyncObject& operator=(const SSyncObject&) = delete;
    ~SSyncObject();
    void Close();
    int32_t Valid();
    uint32_t Wait(uint32_t timeout);
};

class SEvent : public SSyncObject {
public:
    SEvent(int32_t manualReset, int32_t initialState);
    int32_t Reset();
    int32_t Set();
};

class SMutex : public SSyncObject {
public:
    SMutex();
    void Create(int32_t initialOwner, const char* name);
    int32_t Release();
};

class SSemaphore : public SSyncObject {
public:
    SSemaphore(int32_t initialCount, int32_t maximumCount);
    int32_t Signal(uint32_t count);
};

class CSRWLock {
    pthread_rwlock_t m_lock;
public:
    CSRWLock();
    ~CSRWLock();
    CSRWLock(const CSRWLock&) = delete;
    CSRWLock& operator=(const CSRWLock&) = delete;
    void Enter(int32_t forwriting);
    void Leave(int32_t fromwriting);
};
