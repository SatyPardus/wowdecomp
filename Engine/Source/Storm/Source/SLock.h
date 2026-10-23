#ifndef ENGINE_SOURCE_STORM_SOURCE_SLOCK_H
#define ENGINE_SOURCE_STORM_SOURCE_SLOCK_H

#include <cstddef>
#include <cstdint>
#include <windows.h>
#include "Engine/Source/Storm/H/storm.h"

class CInitCritSect;

class SCritSect {
    public:
    /* 0000 */ CRITICAL_SECTION m_critsect;
    /* 0018 */ uint8_t unused_0018[16];

    SCritSect();
    ~SCritSect();
    void Enter();
    void Leave();
};

class SSyncObject {
    public:
    static uint32_t STORMAPI WaitMultiplePtr(uint32_t count, SSyncObject** objects, int32_t waitAll, uint32_t timeout);

    /* 0000 */ HANDLE m_opaqueData;

    SSyncObject();
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

struct CDebugLockEntry {
    /* 0000 */ uint32_t m_time;
    /* 0004 */ uint32_t m_threadId;
    /* 0008 */ uint32_t m_next;
    /* 000C */ const char* m_file;
    /* 0010 */ uint32_t m_line;
};

class CDebugLock {
    public:
    enum : uint32_t {
        MAX_ENTRIES = 256,
    };

    /* 0000 */ uint32_t m_entry;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ CDebugLock* m_next;
};

template <class T>
class TDebugLock : public CDebugLock {
    public:
    static CDebugLockEntry s_entries[CDebugLock::MAX_ENTRIES];
    static uint32_t s_freeEntry;
    static CInitCritSect s_critsect;
    static CDebugLock* s_lockList;

    static void LockEntries();
    static void DumpEntries(CDebugLock* lock);
};

class CDebugSCritSect : public TDebugLock<CDebugSCritSect> {
    public:
    static void DumpAllEntries();
};

class CDebugSRWLock : public TDebugLock<CDebugSRWLock> {
    public:
    static void DumpAllEntries();
};

class SRWLock {
    public:
    struct SUNNLOCK {
        /* 0000 */ LONG m_state;
        /* 0004 */ LONG m_event;
    };

    struct SURWLOCK {
        /* 0000 */ SUNNLOCK m_mutex;
        /* 0008 */ LONG m_readerEvent;
    };

    static void IInitialize();
    static void IDestroy();
    static void IOsRWLockIncRef();
    static void IOsRWLockDecRef();
    static uint32_t IAllocEvent(uint32_t type);
    static void IFreeEvent(uint32_t event, uint32_t type, int32_t reset);
    static uint32_t IAddWaiter(volatile LONG* waiters, uint32_t type, uint32_t increment);
    static int32_t IRemoveWaiter(volatile LONG* waiters, uint32_t expected, uint32_t type, uint32_t decrement);
    static void SUNNLockEnter(volatile SUNNLOCK* sunnlock);
    static void SUNNLockLeave(volatile SUNNLOCK* sunnlock);
    static void SURWLockInitialize(volatile SURWLOCK* surwlock);
    static void SURWLockDelete(volatile SURWLOCK* surwlock);
    static void SURWLockEnter(volatile SURWLOCK* surwlock, int32_t forwriting);
    static void SURWLockLeave(volatile SURWLOCK* surwlock, int32_t fromwriting);
};

class CSRWLock {
    public:
    /* 0000 */ SRWLock::SURWLOCK m_opaqueData;
    /* 000C */ uint8_t unused_000C[12];

    CSRWLock();
    ~CSRWLock();
    void Enter(int32_t forwriting);
    void Leave(int32_t fromwriting);
};

#endif
