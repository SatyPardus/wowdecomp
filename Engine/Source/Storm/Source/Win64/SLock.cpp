#if defined(WOWDECOMP_ARCH_64)

#include <cstring>
#include <intrin.h>
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "Engine/Source/Storm/H/storm.h"

#define SLOCK_SPIN_NOPS() __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();

// STATIC: 0x00ADE8B8
static uint32_t s_spinCount = 1;

// STATIC: 0x00CAE988
static volatile LONG s_eventCount[2];

// STATIC: 0x00CAE990
static volatile LONG s_freeIndex[2];

// STATIC: 0x00CAE998
static volatile LONG s_allocIndex[2];

// STATIC: 0x00CAE9A0
static volatile LONG s_eventIds[2][2048];

// STATIC: 0x00CB29A0
static HANDLE s_events[2][1024];

// STATIC: 0x00CB49A0
static int32_t s_refCount;

// STATIC: 0x00CB49A4
static CInitCritSect s_refCritsect;

// STATIC: [0x00CB49C8, 0x00CB5DC8]
template <class T>
CDebugLockEntry TDebugLock<T>::s_entries[CDebugLock::MAX_ENTRIES];

// STATIC: [0x00CB71C8<CDebugSCritSect>, 0x00CB71F0<CDebugSRWLock>]
template <class T>
uint32_t TDebugLock<T>::s_freeEntry;

// STATIC: [0x00CB71CC<CDebugSCritSect>, 0x00CB71F4<CDebugSRWLock>]
template <class T>
CInitCritSect TDebugLock<T>::s_critsect;

// STATIC: [0x00CB71EC<CDebugSCritSect>, 0x00CB7214<CDebugSRWLock>]
template <class T>
CDebugLock* TDebugLock<T>::s_lockList;

// STATIC: 0x00CB7218
static volatile LONG s_stallCount;

// OFFSET: 0x007741D0
void SRWLock::IInitialize() {
    for (uint32_t type = 0; type < 2; type++) {
        for (uint32_t i = 0; i < 1024; i++) {
            HANDLE event = CreateEventA(nullptr, type == 1, FALSE, nullptr);
            s_eventIds[type][i] = (i + 1) << 21;
            s_events[type][i] = event;
        }

        s_allocIndex[type] = 2047;
        s_freeIndex[type] = 1023;
        s_eventCount[type] = 1024;
    }

    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    s_spinCount = systemInfo.dwNumberOfProcessors > 1 ? 4000 : 1;

    SServerInitialize();
}

// OFFSET: 0x007742D0
void SRWLock::IDestroy() {
    for (uint32_t type = 0; type < 2; type++) {
        for (uint32_t i = 0; i < 1024; i++) {
            CloseHandle(s_events[type][i]);
            s_events[type][i] = nullptr;
            s_eventIds[type][i] = 0;
        }

        s_allocIndex[type] = 0;
        s_freeIndex[type] = 0;
        s_eventCount[type] = 0;
    }

    SServerDestroy();
}

// OFFSET: 0x00774360
uint32_t SRWLock::IAllocEvent(uint32_t type) {
    if (InterlockedDecrement(&s_eventCount[type]) < 0) {
        DebugLog("IAllocEvent: too many %s event allocs", type ? "SUMREVTYPE" : "SUAREVTYPE");
    }

    volatile LONG* slot = &s_eventIds[type][InterlockedIncrement(&s_allocIndex[type]) & 0x7FF];
    uint32_t event = InterlockedExchange(slot, 0);

    while (!event) {
        uint32_t spin = s_spinCount;

        while (true) {
            event = InterlockedExchange(slot, 0);

            if (event) {
                return event;
            }

            SLOCK_SPIN_NOPS();

            if (!--spin) {
                break;
            }
        }

        Sleep(0);
    }

    return event;
}

// OFFSET: 0x00774410
void SRWLock::IFreeEvent(uint32_t event, uint32_t type, int32_t reset) {
    if (InterlockedIncrement(&s_eventCount[type]) > 1024) {
        DebugLog("IFreeEvent: too many %s event frees", type ? "SUMREVTYPE" : "SUAREVTYPE");
    }

    if (reset) {
        ResetEvent(s_events[type][(event >> 21) - 1]);
    }

    volatile LONG* slot = &s_eventIds[type][InterlockedIncrement(&s_freeIndex[type]) & 0x7FF];
    LONG value = InterlockedExchange(slot, event & 0xFFE00000);

    while (value) {
        uint32_t spin = s_spinCount;

        while (true) {
            value = InterlockedExchange(slot, value);

            if (!value) {
                return;
            }

            SLOCK_SPIN_NOPS();

            if (!--spin) {
                break;
            }
        }

        Sleep(0);
    }
}

// OFFSET: 0x007744F0
uint32_t SRWLock::IAddWaiter(volatile LONG* waiters, uint32_t type, uint32_t increment) {
    uint32_t event = IAllocEvent(type);
    LONG value = InterlockedCompareExchange(waiters, event, 0);

    while (value) {
        if (InterlockedCompareExchange(waiters, value + increment, value) == value) {
            IFreeEvent(event, type, 0);
            return value + increment;
        }

        value = InterlockedCompareExchange(waiters, event, 0);
    }

    return event;
}

// OFFSET: 0x00774550
int32_t SRWLock::IRemoveWaiter(volatile LONG* waiters, uint32_t expected, uint32_t type, uint32_t decrement) {
    LONG value = InterlockedCompareExchange(waiters, 0, expected);

    while (value != static_cast<LONG>(expected)) {
        if (InterlockedCompareExchange(waiters, value - decrement, value) == value) {
            return 0;
        }

        value = InterlockedCompareExchange(waiters, 0, expected);
    }

    IFreeEvent(expected, type, 1);
    return 1;
}

// INLINED
inline void SRWLock::SUNNLockLeave(volatile SUNNLOCK* sunnlock) {
    sunnlock->m_state = 1;

    uint32_t event = sunnlock->m_event;

    if (event) {
        SetEvent(s_events[0][(event >> 21) - 1]);
    }
}

// OFFSET: 0x007745B0
void SRWLock::SURWLockLeave(volatile SURWLOCK* surwlock, int32_t fromwriting) {
    if (fromwriting) {
        SUNNLockLeave(&surwlock->m_mutex);
    } else if (IRemoveWaiter(&surwlock->m_readerEvent, (surwlock->m_readerEvent & 0xFFE00000) + 1, 1, 2)) {
        SUNNLockLeave(&surwlock->m_mutex);
    }
}

// OFFSET: 0x00774620
SCritSect::SCritSect() {
    InitializeCriticalSection(&this->m_critsect);
}

// OFFSET: 0x00774630
SCritSect::~SCritSect() {
    DeleteCriticalSection(&this->m_critsect);
}

// OFFSET: 0x00774640
void SCritSect::Enter() {
    EnterCriticalSection(&this->m_critsect);
}

// OFFSET: 0x00774650
void SCritSect::Leave() {
    LeaveCriticalSection(&this->m_critsect);
}

// OFFSET: 0x00921A00
SSyncObject::SSyncObject() {
    this->m_opaqueData = nullptr;
}

// OFFSET: 0x00774660
void CSRWLock::Leave(int32_t fromwriting) {
    SRWLock::SURWLockLeave(&this->m_opaqueData, fromwriting);
}

// OFFSET: 0x00774680
int32_t SSyncObject::Valid() {
    return this->m_opaqueData != nullptr;
}

// OFFSET: 0x00774690
uint32_t SSyncObject::Wait(uint32_t timeout) {
    return WaitForSingleObject(this->m_opaqueData, timeout);
}

// OFFSET: 0x007746B0
uint32_t STORMAPI SSyncObject::WaitMultiplePtr(uint32_t count, SSyncObject** objects, int32_t waitAll, uint32_t timeout) {
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];

    if (count > MAXIMUM_WAIT_OBJECTS) {
        return 0xFFFFFFFF;
    }

    uint32_t handleCount = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (objects[i] && objects[i]->m_opaqueData) {
            handles[handleCount++] = objects[i]->m_opaqueData;
        }
    }

    return WaitForMultipleObjects(handleCount, handles, waitAll, timeout);
}

// OFFSET: 0x00774720
int32_t SEvent::Set() {
    return SetEvent(this->m_opaqueData);
}

// OFFSET: 0x00774730
int32_t SEvent::Reset() {
    return ResetEvent(this->m_opaqueData);
}

// OFFSET: 0x00774740
int32_t SThread::Create(uint32_t (STORMAPI* threadProc)(void*), void* param, SThread& thread, char* threadName, uint32_t stackSize) {
    uint32_t threadId;
    thread.m_opaqueData = SCreateThread(threadProc, param, &threadId, nullptr, nullptr, stackSize);
    return thread.m_opaqueData != nullptr;
}

// OFFSET: 0x00774770
int32_t SThread::SetPriority(int32_t priority) {
    static const int32_t priorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL };

    if (priority < 6) {
        return SetThreadPriority(this->m_opaqueData, priorities[priority]);
    }

    return 0;
}

// OFFSET: 0x007747A0
int32_t SSemaphore::Signal(uint32_t count) {
    return ReleaseSemaphore(this->m_opaqueData, count, nullptr);
}

// OFFSET: 0x007747C0
void SMutex::Create(int32_t initialOwner, const char* name) {
    if (this->m_opaqueData) {
        CloseHandle(this->m_opaqueData);
        this->m_opaqueData = nullptr;
    }

    this->m_opaqueData = CreateMutexA(nullptr, initialOwner, name);

    if (!this->m_opaqueData) {
        DebugLog("SMutex::Create: CreateMutex failed");
    }
}

// OFFSET: 0x00774810
int32_t SMutex::Release() {
    return ReleaseMutex(this->m_opaqueData);
}

// INLINED
inline void SRWLock::IOsRWLockIncRef() {
    s_refCritsect.Enter();

    if (s_refCount++ == 0) {
        IInitialize();
    }

    s_refCritsect.Leave();
}

// INLINED
inline void SRWLock::IOsRWLockDecRef() {
    s_refCritsect.Enter();

    if (--s_refCount == 0) {
        IDestroy();
    }

    s_refCritsect.Leave();
}

// INLINED
inline void SRWLock::SURWLockInitialize(volatile SURWLOCK* surwlock) {
    surwlock->m_mutex.m_state = 1;
    surwlock->m_mutex.m_event = 0;
    surwlock->m_readerEvent = 0;
}

// INLINED
inline void SRWLock::SURWLockDelete(volatile SURWLOCK* surwlock) {
    surwlock->m_mutex.m_state = 0;

    uint32_t event = InterlockedExchange(&surwlock->m_mutex.m_event, 0);

    if (event) {
        IFreeEvent(event, 0, 1);
    }

    event = InterlockedExchange(&surwlock->m_readerEvent, 0);

    if (event) {
        IFreeEvent(event, 1, 1);
    }
}

// OFFSET: 0x00774820
CSRWLock::CSRWLock() {
    SRWLock::IOsRWLockIncRef();
    SRWLock::SURWLockInitialize(&this->m_opaqueData);
}

// OFFSET: 0x00774870
CSRWLock::~CSRWLock() {
    SRWLock::SURWLockDelete(&this->m_opaqueData);
    SRWLock::IOsRWLockDecRef();
}

// OFFSET: 0x007748E0
SSyncObject::~SSyncObject() {
    if (this->m_opaqueData) {
        CloseHandle(this->m_opaqueData);
        this->m_opaqueData = nullptr;
    }
}

void SSyncObject::Close() {
    if (this->m_opaqueData) {
        CloseHandle(this->m_opaqueData);
        this->m_opaqueData = nullptr;
    }
}

// OFFSET: 0x00774900
SEvent::SEvent(int32_t manualReset, int32_t initialState) {
    this->m_opaqueData = CreateEventA(nullptr, manualReset, initialState, nullptr);

    if (!this->m_opaqueData) {
        DebugLog("SEvent::SEvent: CreateEvent failed");
    }
}

// OFFSET: 0x00774940
SSemaphore::SSemaphore(int32_t initialCount, int32_t maximumCount) {
    this->m_opaqueData = CreateSemaphoreA(nullptr, initialCount, maximumCount, nullptr);
}

// OFFSET: 0x00774970
SMutex::SMutex() {
    this->Create(0, nullptr);
}

// OFFSET: [0x007749A0, 0x00774AD0]
template <class T>
void TDebugLock<T>::LockEntries() {
    if (s_critsect.Enter()) {
        memset(s_entries, 0, sizeof(s_entries));

        for (uint32_t i = 1; i < 255; i++) {
            s_entries[i].m_next = i + 1;
        }

        s_freeEntry = 1;
    }
}

// OFFSET: [0x007749F0, 0x00774B20]
template <class T>
void TDebugLock<T>::DumpEntries(CDebugLock* lock) {
    CDebugLockEntry* entry = nullptr;
    uint32_t now = GetTickCount();
    uint32_t index = lock->m_entry;

    while (index) {
        if (index >= CDebugLock::MAX_ENTRIES) {
            SDebugPrintf("%s(%u) : CDebugLock:%08x: entry has bad next %u\n", __FILE__, __LINE__, lock, index);

            if (entry) {
                entry->m_next = 0;
            } else {
                lock->m_entry = 0;
            }

            return;
        }

        entry = &s_entries[index];

        char threadName[16];
        SGetThreadName(entry->m_threadId, threadName, sizeof(threadName));

        SDebugPrintf("%s(%u) : CDebugLock:%08x: tid:%03x %s %c %c t:%u\n", entry->m_file, entry->m_line & 0x3FFFFFFF, lock, entry->m_threadId, threadName, (entry->m_line & 0x40000000) ? 'W' : 'R', (entry->m_line & 0x80000000) ? 'T' : 'F', now - entry->m_time);

        index = entry->m_next;
    }
}

// OFFSET: 0x00774C00
void CDebugSCritSect::DumpAllEntries() {
    LockEntries();

    SDebugPrintf("%s(%u) : CDebugSCritSect:DumpAllEntries\n", __FILE__, __LINE__);

    for (CDebugLock* lock = s_lockList; lock; lock = lock->m_next) {
        DumpEntries(lock);
    }

    s_critsect.Leave();
}

// OFFSET: 0x00774C50
void CDebugSRWLock::DumpAllEntries() {
    LockEntries();

    SDebugPrintf("%s(%u) : CDebugSRWLock:DumpAllEntries\n", __FILE__, __LINE__);

    for (CDebugLock* lock = s_lockList; lock; lock = lock->m_next) {
        DumpEntries(lock);
    }

    s_critsect.Leave();
}

// OFFSET: 0x00774CA0
void SRWLock::SUNNLockEnter(volatile SUNNLOCK* sunnlock) {
    if (!sunnlock->m_event) {
        uint32_t spin = s_spinCount;

        while (true) {
            if (InterlockedExchange(&sunnlock->m_state, 0)) {
                return;
            }

            SLOCK_SPIN_NOPS();

            if (!--spin || sunnlock->m_event) {
                break;
            }
        }
    }

    uint32_t event = IAddWaiter(&sunnlock->m_event, 0, 1) & 0xFFE00000;

    while (true) {
        uint32_t spin = s_spinCount;

        while (true) {
            if (InterlockedExchange(&sunnlock->m_state, 0)) {
                IRemoveWaiter(&sunnlock->m_event, event, 0, 1);
                return;
            }

            SLOCK_SPIN_NOPS();

            if (!--spin) {
                break;
            }
        }

        if (WaitForSingleObject(s_events[0][(static_cast<uint32_t>(sunnlock->m_event) >> 21) - 1], 60000)) {
            InterlockedExchangeAdd(&s_stallCount, 1);
            CDebugSCritSect::DumpAllEntries();
            CDebugSRWLock::DumpAllEntries();
        }
    }
}

// OFFSET: 0x00774DA0
void SRWLock::SURWLockEnter(volatile SURWLOCK* surwlock, int32_t forwriting) {
    if (forwriting) {
        SUNNLockEnter(&surwlock->m_mutex);
        return;
    }

    uint32_t event = IAddWaiter(&surwlock->m_readerEvent, 1, 2);

    if (event == (event & 0xFFE00000)) {
        SUNNLockEnter(&surwlock->m_mutex);
        InterlockedExchangeAdd(&surwlock->m_readerEvent, 1);

        if (event) {
            SetEvent(s_events[1][(event >> 21) - 1]);
        }

        return;
    }

    while (true) {
        uint32_t spin = s_spinCount;

        while (true) {
            if (surwlock->m_readerEvent & 1) {
                return;
            }

            SLOCK_SPIN_NOPS();

            if (!--spin) {
                break;
            }
        }

        if (!WaitForSingleObject(s_events[1][(event >> 21) - 1], 60000)) {
            return;
        }

        InterlockedExchangeAdd(&s_stallCount, 1);
        CDebugSCritSect::DumpAllEntries();
        CDebugSRWLock::DumpAllEntries();
    }
}

// OFFSET: 0x00774E80
void CSRWLock::Enter(int32_t forwriting) {
    SRWLock::SURWLockEnter(&this->m_opaqueData, forwriting);
}

#endif
