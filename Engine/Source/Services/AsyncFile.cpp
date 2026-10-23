#include <cstddef>
#include <cstdint>
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00AC3438
static const char* s_asyncQueueNames[3] = { "Disk Queue", "Net Geometry Queue", "Net Texture Queue" };

// STATIC: 0x00AC3444
static uint32_t s_handlerTimeout = 100;

// STATIC: 0x00AC3448
static TSList<CAsyncQueue, TSGetLink<CAsyncQueue> > s_asyncQueueList;

// STATIC: 0x00AC3454
static TSList<CAsyncThread, TSGetLink<CAsyncThread> > s_asyncThreadList;

namespace AsyncFileRead {
    // STATIC: 0x00AC3460
    TSExplicitList<CAsyncObject, offsetof(CAsyncObject, m_link)> s_asyncFileReadFreeList;
}

// STATIC: 0x00AC346C
static TSExplicitList<CAsyncObject, offsetof(CAsyncObject, m_link)> s_asyncFileReadPostList;

// STATIC: 0x00B4A1EC
static int32_t s_queueLocked;

// STATIC: 0x00B4A1F0
static void (*s_ingameStartCallback)();

// STATIC: 0x00B4A1F4
static void (*s_ingameProgressCallback)(float, int32_t);

// STATIC: 0x00B4A1F8
static int32_t s_progressCount;

// STATIC: 0x00B4A1FC
static void* s_asyncProgressCallbackParams;

// STATIC: 0x00B4A200
static void (*s_asyncProgressCallback)(float, void*);

// STATIC: 0x00B4A204
static CAsyncObject* s_asyncWaitObject;

// STATIC: 0x00B4A208
static HPROPCONTEXT s_propContext;

// STATIC: 0x00B4A20C
static CAsyncQueue* s_asyncQueues[3];

// STATIC: 0x00B4A218
static uint32_t s_threadSleep;

namespace AsyncFileRead {
    // STATIC: 0x00B4A21C
    SEvent s_shutdownEvent(1, 0);
}

// STATIC: 0x00B4A220
static TSGrowableArray<void (*)()> s_asyncPollHandlers;

// STATIC: 0x00B4A230
static TSGrowableArray<int32_t (*)()> s_asyncStatusHandlers;

// STATIC: 0x00B4A240
SCritSect g_queueLock;

// VERIFY: 5a8ab5a9 auto
// OFFSET: 0x004B50A0
int32_t CAsyncObject::IsPending() {
    return !this->m_isCurrent && !this->m_isRead && !this->m_isProcessed;
}

// VERIFY: 6305e7e3 auto
// OFFSET: 0x004B9910
void AsyncFileReadSetProgressCallback(void (*callback)(float, void*), void* param) {
    s_asyncProgressCallback = callback;
    s_asyncProgressCallbackParams = param;
}

// VERIFY: fe99c992 auto
// OFFSET: 0x004B9930
void AsyncFileReadIngameProgressCallback(void (*startCallback)(), void (*progressCallback)(float, int32_t)) {
    s_ingameStartCallback = startCallback;
    s_ingameProgressCallback = progressCallback;
}

// VERIFY: 08e4e19e auto
// OFFSET: 0x004B9950
void AsyncFile::EnterQueueLock() {
    s_queueLocked = 1;
    g_queueLock.Enter();
}

// VERIFY: ee2b2778 auto
// OFFSET: 0x004B9970
void AsyncFile::LeaveQueueLock() {
    g_queueLock.Leave();
    s_queueLocked = 0;
}

// VERIFY: ca9687a9 manual
// OFFSET: 0x004B9AB0
void AsyncFileReadUnloadPostList() {
    CAsyncObject* object;

    while ((object = s_asyncFileReadPostList.Head()) != nullptr) {
        object->m_link.Unlink();

        if (object->m_userPostUnloadCallback) {
            reinterpret_cast<void (*)(void*)>(object->m_userPostUnloadCallback)(object->m_userArg);
        }
    }
}

// VERIFY: acd49bbd manual
// OFFSET: 0x004B9B20
int32_t AsyncFileReadPollHandler(const void* data, void* param) {
    uint32_t start = OsGetAsyncTimeMs();

    while (true) {
        g_queueLock.Enter();

        CAsyncObject* object = s_asyncFileReadPostList.Head();

        if (!object) {
            g_queueLock.Leave();
            break;
        }

        object->m_link.Unlink();

        if (object == s_asyncWaitObject) {
            s_asyncWaitObject = nullptr;
        }

        object->m_isProcessed = 1;
        g_queueLock.Leave();
        reinterpret_cast<void (*)(void*)>(object->m_userPostloadCallback)(object->m_userArg);
        s_progressCount--;

        if (OsGetAsyncTimeMs() - start > s_handlerTimeout) {
            break;
        }
    }

    for (uint32_t i = 0; i < s_asyncPollHandlers.Count(); i++) {
        s_asyncPollHandlers[i]();
    }

    return 1;
}

// VERIFY: 132a0c31 manual
// OFFSET: 0x004B9C60
void AsyncFileReadAddPollHandler(void (*handler)()) {
    for (uint32_t i = 0; i < s_asyncPollHandlers.Count(); i++) {
        if (s_asyncPollHandlers[i] == handler) {
            return;
        }
    }

    s_asyncPollHandlers.New(handler);
}

// VERIFY: e6dbc58a manual
// OFFSET: 0x004B9D20
void AsyncFileReadAddStatusHandler(int32_t (*handler)()) {
    for (uint32_t i = 0; i < s_asyncStatusHandlers.Count(); i++) {
        if (s_asyncStatusHandlers[i] == handler) {
            return;
        }
    }

    s_asyncStatusHandlers.New(handler);
}

// VERIFY: 75b60811 manual
// OFFSET: 0x004B9DE0
void AsyncFileReadDestroyObject(CAsyncObject* object) {
    g_queueLock.Enter();

    if (object->m_isCurrent) {
        DebugLog("AsyncFileReadDestroyObject()");
        g_queueLock.Leave();

        while (object->m_isCurrent) {
            OsSleep(1);
        }

        g_queueLock.Enter();
    }

    object->m_link.Unlink();

    if (object->m_file) {
        SFileCloseFile(object->m_file);
    }

    AsyncFileRead::s_asyncFileReadFreeList.LinkToHead(object);
    g_queueLock.Leave();
}

// VERIFY: 37b4ae03 auto
// OFFSET: 0x004B9E90
void DefaultAsyncObjectCleanupCallback(CAsyncObject* object) {
    AsyncFileReadDestroyObject(object);
}

// VERIFY: 3f12de16 manual
// OFFSET: 0x004B9EA0
int32_t AsyncFile::Cancel(CAsyncObject* object, void (*callback)(CAsyncObject*)) {
    g_queueLock.Enter();

    if (object->m_isCurrent) {
        object->m_userArg = object;
        object->m_userPostUnloadCallback = reinterpret_cast<void*>(callback ? callback : DefaultAsyncObjectCleanupCallback);
        object->m_userPostloadCallback = object->m_userPostUnloadCallback;
        g_queueLock.Leave();

        return 0;
    }

    SFileCloseFile(object->m_file);
    object->m_link.Unlink();
    AsyncFileRead::s_asyncFileReadFreeList.LinkToHead(object);
    g_queueLock.Leave();

    return 1;
}

// VERIFY: 8a117547 manual
// OFFSET: 0x004B9F40
int32_t NetQueuePromoteLocalFiles(const void* data, void* param) {
    // STATIC: 0x00B4A268
    static uint32_t s_lastPromoteTime;

    uint32_t time = OsGetAsyncTimeMs();

    if (time - s_lastPromoteTime >= 250) {
        s_lastPromoteTime = time;
        g_queueLock.Enter();

        for (int32_t i = 1; i < 3; i++) {
            CAsyncObject* object = s_asyncQueues[i]->m_readList.Head();

            while (!s_asyncQueues[i]->m_readList.IsEnd(object)) {
                CAsyncObject* next = s_asyncQueues[i]->m_readList.RawNext(object);

                if (SFile::FileIsLocal(object->m_file, 6) == 1) {
                    s_asyncQueues[0]->m_readList.LinkToTail(object);
                    object->m_queue = s_asyncQueues[0];
                }

                object = next;
            }
        }

        g_queueLock.Leave();
    }

    return 1;
}

// VERIFY: d0d6eee1 manual
// OFFSET: 0x004BA060
void AsyncFileReadWait(CAsyncObject* object) {
    // STATIC: 0x00B4A26C
    static int32_t s_waiting;

    if (s_waiting) {
        SErrDisplayAppFatal("AsyncFileReadWait(): s_waiting != FALSE");
    }

    s_waiting++;
    g_queueLock.Enter();

    if (object->m_isProcessed) {
        g_queueLock.Leave();
        return;
    }

    s_asyncWaitObject = object;

    if (!object->m_isCurrent && !object->m_isRead) {
        object->m_link.Unlink();
        object->m_queue->m_readList.LinkToHead(object);
    }

    g_queueLock.Leave();

    if (SFile::IsStreamingMode()) {
        SFile::SetFilePriority(object->m_file, 1, 0);
    }

    char name[256];
    SFile::GetActualFileName(object->m_file, name, sizeof(name));
    DebugLog("AsyncFileReadWait():%s\n", name);

    if (s_ingameStartCallback) {
        s_ingameStartCallback();
    }

    do {
        if (s_ingameProgressCallback) {
            s_ingameProgressCallback(0.0f, 0);
        }

        AsyncFileReadPollHandler(nullptr, nullptr);

        if (!s_asyncWaitObject) {
            break;
        }

        OsSleep(1);
    } while (true);

    s_waiting--;
}

// VERIFY: 82068864 manual
// OFFSET: 0x004BA170
CAsyncObject* AsyncFileReadAllocObject() {
    g_queueLock.Enter();

    CAsyncObject* object = AsyncFileRead::s_asyncFileReadFreeList.Head();

    if (!object) {
        object = AsyncFileRead::s_asyncFileReadFreeList.NewNode(1, 0, 0);
    }

    object->m_link.Unlink();
    g_queueLock.Leave();

    object->m_file = nullptr;
    object->m_buffer = nullptr;
    object->m_size = 0;
    object->m_userArg = nullptr;
    object->m_userPostloadCallback = nullptr;
    object->m_userPostUnloadCallback = nullptr;
    object->m_queue = nullptr;
    object->m_isProcessed = 0;
    object->m_isRead = 0;
    object->m_isCurrent = 0;
    object->m_char24 = 0;
    object->m_char25 = 0;
    object->m_queuedFrame = 0;
    object->m_priority = 126;

    return object;
}

// VERIFY: d372c124 manual
// OFFSET: 0x004BA310
CAsyncQueue::~CAsyncQueue() {
}

// VERIFY: 69785110 manual
// OFFSET: 0x004BA3D0
void AsyncFileReadLinkObject(CAsyncObject* object, int32_t urgent) {
    if (!object->m_queue) {
        return;
    }

    object->m_link.Unlink();

    for (CAsyncObject* other = object->m_queue->m_readList.Head(); !object->m_queue->m_readList.IsEnd(other); other = object->m_queue->m_readList.RawNext(other)) {
        if (object->m_priority <= other->m_priority && (urgent || object->m_priority != other->m_priority)) {
            object->m_queue->m_readList.LinkNode(object, 2, other);
            object->m_char25 = 0;
            return;
        }
    }

    object->m_queue->m_readList.LinkToTail(object);
    object->m_char25 = 0;
}

// VERIFY: c818c689 manual
// OFFSET: 0x004BA530
void AsyncFileReadLinkStaleObject(CAsyncObject* object, int32_t urgent) {
    object->m_link.Unlink();

    for (CAsyncObject* other = object->m_queue->m_staleList.Head(); !object->m_queue->m_staleList.IsEnd(other); other = object->m_queue->m_staleList.RawNext(other)) {
        if (object->m_priority <= other->m_priority && (urgent || object->m_priority != other->m_priority)) {
            object->m_queue->m_staleList.LinkNode(object, 2, other);
            object->m_char25 = 1;
            return;
        }
    }

    object->m_queue->m_staleList.LinkToTail(object);
    object->m_char25 = 1;
}

// VERIFY: 28f905c7 manual
// OFFSET: 0x004BA680
uint32_t STORMAPI AsyncFileReadThread(void* param) {
    CAsyncThread* thread = static_cast<CAsyncThread*>(param);
    CAsyncQueue* queue = thread->m_queue;
    PropSelectContext(s_propContext);

    while (AsyncFileRead::s_shutdownEvent.Wait(0)) {
        uint32_t count = 0;

        while (true) {
            g_queueLock.Enter();

            CAsyncObject* object = queue->m_readList.Head();

            if (object) {
                if (queue->m_frameLimited && g_theGxDevicePtr->m_frameCount - object->m_queuedFrame > 3) {
                    AsyncFileReadLinkStaleObject(object, 1);
                    g_queueLock.Leave();
                    continue;
                }
            } else {
                object = queue->m_staleList.Head();

                if (!object) {
                    break;
                }
            }

            object->m_link.Unlink();
            object->m_queue = nullptr;
            object->m_isCurrent = 1;
            thread->m_currentObject = object;
            g_queueLock.Leave();

            for (int32_t retries = 10; retries; ) {
                if (SFile::IsStreamingMode() && object->m_file->m_type) {
                    SFile::MakeFileLocal(object->m_file, (object->m_priority > 127) + 1, true);
                }

                if (SFileReadFile(object->m_file, object->m_buffer, object->m_size, nullptr, nullptr, 0)) {
                    break;
                }

                if (!--retries) {
                    char name[512];
                    SFile::GetActualFileName(object->m_file, name, sizeof(name));
                    char error[512];
                    SErrGetErrorStr(SErrGetLastError(), error, sizeof(error));
                    DebugLog("AsyncFileReadThread() failed reading %s: %s", name, error);
                }
            }

            g_queueLock.Enter();
            s_asyncFileReadPostList.LinkToTail(object);
            thread->m_currentObject = nullptr;
            object->m_isCurrent = 0;
            object->m_isRead = 1;
            g_queueLock.Leave();

            if (s_threadSleep && ++count == s_threadSleep) {
                OsSleep(1);
                count = 0;
            }
        }

        g_queueLock.Leave();
        OsSleep(1);
    }

    return 0;
}

// VERIFY: 1d3b9267 manual
// OFFSET: 0x004BA8E0
CAsyncQueue* AsyncFileReadCreateQueue() {
    return s_asyncQueueList.NewNode(2, 0, 0);
}

// VERIFY: 3de6d82d auto
// OFFSET: 0x004BA980
void AsyncFileReadCreateThread(CAsyncQueue* queue, const char* name) {
    CAsyncThread* thread = s_asyncThreadList.NewNode(2, 0, 0);
    thread->m_queue = queue;
    thread->m_currentObject = nullptr;
    SThread::Create(AsyncFileReadThread, thread, thread->m_thread, const_cast<char*>(name), 0);
}

// VERIFY: e94485f6 auto
// OFFSET: 0x004BAA40
void AsyncFileReadInitialize(uint32_t threadSleep, uint32_t handlerTimeout) {
    s_threadSleep = threadSleep;
    s_handlerTimeout = handlerTimeout;

    if (threadSleep > 100) {
        s_threadSleep = 100;
    }

    if (handlerTimeout < 20) {
        s_handlerTimeout = 20;
    }

    EventRegisterEx(EVENT_ID_POLL, AsyncFileReadPollHandler, nullptr, EVENT_PRIORITY_NORMAL);

    if (SFile::IsStreamingMode()) {
        EventRegisterEx(EVENT_ID_IDLE, NetQueuePromoteLocalFiles, nullptr, EVENT_PRIORITY_BELOW_NORMAL);
    }

    s_asyncWaitObject = nullptr;
    s_asyncProgressCallback = nullptr;
    s_asyncProgressCallbackParams = nullptr;
    s_ingameProgressCallback = nullptr;
    s_ingameStartCallback = nullptr;
    s_propContext = PropGetSelectedContext();
    AsyncFileRead::s_shutdownEvent.Reset();

    int32_t queues = SFile::IsStreamingMode() ? 3 : 1;

    for (int32_t i = 0; i < queues; i++) {
        s_asyncQueues[i] = AsyncFileReadCreateQueue();
        AsyncFileReadCreateThread(s_asyncQueues[i], s_asyncQueueNames[i]);
    }

    if (SFile::IsStreamingMode()) {
        s_asyncQueues[2]->m_frameLimited = 1;
    }

    s_queueLocked = 0;
}

// VERIFY: 35423985 auto
// OFFSET: 0x004BAB50
void AsyncFileReadObject(CAsyncObject* object, int32_t urgent) {
    CAsyncQueue* queue = s_asyncQueues[0];

    if (SFile::IsStreamingMode()) {
        int32_t local = SFile::FileIsLocal(object->m_file, 6);

        if (!local || local == 2) {
            object->m_char24 = 1;
            queue = object->m_priority > 127 ? s_asyncQueues[2] : s_asyncQueues[1];
        }
    }

    g_queueLock.Enter();
    object->m_queue = queue;

    if (object == s_asyncWaitObject) {
        object->m_priority = object->m_priority > 127 ? 128 : 0;
        object->m_queuedFrame = g_theGxDevicePtr->m_frameCount;
        queue->m_readList.LinkToHead(object);
        object->m_char25 = 0;
    } else if (queue->m_frameLimited) {
        AsyncFileReadLinkStaleObject(object, urgent);
    } else {
        AsyncFileReadLinkObject(object, urgent);
    }

    g_queueLock.Leave();

    if (SFile::IsStreamingMode()) {
        SFile::SetFilePriority(object->m_file, 0, 0);
    }
}

// VERIFY: 2f496060 auto
// OFFSET: 0x004BAC20
void AsyncFileReadIncreasePriority(CAsyncObject* object) {
    object->m_queuedFrame = g_theGxDevicePtr->m_frameCount;

    if (object->m_char25) {
        AsyncFileReadLinkObject(object, 1);
    }
}

// VERIFY: 3aa538d7 manual
// OFFSET: 0x004BAC50
void AsyncFileReadDestroy() {
    for (CAsyncThread* thread = s_asyncThreadList.Head(); !s_asyncThreadList.IsEnd(thread); thread = s_asyncThreadList.RawNext(thread)) {
        AsyncFileRead::s_shutdownEvent.Set();
        thread->m_thread.Wait(0xFFFFFFFF);
    }

    AsyncFileReadUnloadPostList();
    AsyncFileRead::s_asyncFileReadFreeList.Clear();

    for (CAsyncQueue* queue = s_asyncQueueList.Head(); !s_asyncQueueList.IsEnd(queue); queue = s_asyncQueueList.RawNext(queue)) {
    }

    s_asyncQueueList.Clear();
    s_asyncQueues[0] = nullptr;
    s_asyncQueues[1] = nullptr;
    s_asyncQueues[2] = nullptr;

    for (CAsyncThread* thread = s_asyncThreadList.Head(); !s_asyncThreadList.IsEnd(thread); thread = s_asyncThreadList.RawNext(thread)) {
    }

    s_asyncThreadList.Clear();
    s_asyncPollHandlers.SetCount(0);
    s_asyncStatusHandlers.SetCount(0);
    s_asyncProgressCallback = nullptr;
    s_asyncProgressCallbackParams = nullptr;
    s_ingameProgressCallback = nullptr;
    s_ingameStartCallback = nullptr;

    if (SFile::IsStreamingMode()) {
        EventUnregisterEx(EVENT_ID_IDLE, NetQueuePromoteLocalFiles, nullptr, 2);
    }

    EventUnregisterEx(EVENT_ID_POLL, AsyncFileReadPollHandler, nullptr, 0xFFFFFFFF);
}

// VERIFY: e7566958 auto
// OFFSET: 0x004BAD80
bool AsyncFileReadIsReading() {
    g_queueLock.Enter();

    bool reading = false;

    for (CAsyncThread* thread = s_asyncThreadList.Head(); !s_asyncThreadList.IsEnd(thread); thread = s_asyncThreadList.RawNext(thread)) {
        if (thread->m_currentObject) {
            reading = true;
        }
    }

    for (CAsyncQueue* queue = s_asyncQueueList.Head(); !s_asyncQueueList.IsEnd(queue); queue = s_asyncQueueList.RawNext(queue)) {
        if (queue->m_readList.Head()) {
            reading = true;
        }
    }

    if (s_asyncFileReadPostList.Head()) {
        reading = true;
    }

    g_queueLock.Leave();

    return reading;
}

// VERIFY: 3e612f72 auto
// OFFSET: 0x004BAE10
void AsyncFileReadWaitAll() {
    DebugLog("AsyncFileReadWaitAll() Enter\n");

    int32_t total = 0;

    for (uint32_t i = 0; i < s_asyncStatusHandlers.Count(); i++) {
        total += s_asyncStatusHandlers[i]();
    }

    s_progressCount = total;

    while (AsyncFileReadIsReading()) {
        AsyncFileReadPollHandler(nullptr, nullptr);

        if (s_asyncProgressCallback) {
            float progress = 1.0f;

            if (total) {
                progress = static_cast<float>(total - s_progressCount) / total;

                if (progress < 0.0f) {
                    progress = 0.0f;
                } else if (progress > 1.0f) {
                    progress = 1.0f;
                }
            }

            s_asyncProgressCallback(progress, s_asyncProgressCallbackParams);
        }

        OsSleep(1);
    }

    s_asyncProgressCallback = nullptr;
    DebugLog("AsyncFileReadWaitAll() Leave\n");
}
