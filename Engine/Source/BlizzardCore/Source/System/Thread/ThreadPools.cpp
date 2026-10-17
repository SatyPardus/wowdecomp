#include <cstdint>
#include <cstdlib>
#include "Engine/Source/BlizzardCore/Source/System/Hardware/Hardware.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

// STATIC: 0x00B382B4
Blizzard::Thread::ThreadPoolRecord* System_Thread::s_defaultPool;

// VERIFY: 75a5f6e2 manual
// OFFSET: 0x00438AD0
System_Thread::ThreadWorker::ThreadWorker() {
    this->m_thread = nullptr;
    this->m_pool = nullptr;
    this->m_task = nullptr;
    this->m_taskFunction = nullptr;
    this->m_taskParam = nullptr;
}

// VERIFY: 3cc98b69 manual
// OFFSET: 0x00450DF0
Blizzard::Thread::ThreadPoolRecord::ThreadPoolRecord(uint32_t count, uint32_t maxWorkers, int64_t unk) {
    Blizzard::Lock::ScopedMutex lock(this->m_mutex.m_mutex);
    this->m_count = count;
    this->m_maxWorkers = maxWorkers;

    for (uint32_t i = 0; i < count; i++) {
        System_Thread::ThreadWorker* worker = new System_Thread::ThreadWorker();

        if (!worker) {
            break;
        }

        worker->Start(this, 0, 0);
        this->m_workers.push_back(worker);
        this->m_idle.push_back(worker);
    }
}

// VERIFY: d753310e manual
// OFFSET: 0x0044CFD0
Blizzard::Thread::ThreadPoolRecord::~ThreadPoolRecord() {
    {
        Blizzard::Lock::ScopedMutex lock(this->m_mutex.m_mutex);
        this->m_idle.erase(this->m_idle.begin(), this->m_idle.end());
        this->m_count = 0;
    }

    for (std::vector<System_Thread::ThreadWorker*>::iterator it = this->m_workers.begin(); it != this->m_workers.end(); ++it) {
        if (*it) {
            delete *it;
        }
    }

    {
        Blizzard::Lock::ScopedMutex lock(this->m_mutex.m_mutex);
        this->m_workers.erase(this->m_workers.begin(), this->m_workers.end());
    }
}

// VERIFY: 00c35e94 manual
// OFFSET: 0x00450EF0
System_Thread::ThreadWorker* Blizzard::Thread::ThreadPoolRecord::GetWorker(int32_t flags, int32_t stackSize) {
    Blizzard::Lock::ScopedMutex lock(this->m_mutex.m_mutex);

    if (!this->m_count) {
        return nullptr;
    }

    if (!this->m_idle.empty()) {
        System_Thread::ThreadWorker* worker = this->m_idle.back();
        this->m_idle.pop_back();

        if (worker) {
            return worker;
        }
    }

    if (this->m_maxWorkers && this->m_workers.size() >= this->m_maxWorkers) {
        return nullptr;
    }

    System_Thread::ThreadWorker* worker = new System_Thread::ThreadWorker();

    if (worker->Start(this, 0, stackSize)) {
        this->m_workers.push_back(worker);
    } else {
        delete worker;
        worker = nullptr;
    }

    return worker;
}

// VERIFY: 77e1890a manual
// OFFSET: 0x00451040
bool System_Thread::CreateDefaultPool() {
    s_defaultPool = new Blizzard::Thread::ThreadPoolRecord(1, 0, 0);

    return s_defaultPool != nullptr;
}

// VERIFY: 625cc875 manual
// OFFSET: 0x004510A0
bool Blizzard::Thread::CreateFromPool(ThreadPoolRecord* pool, uint32_t (*function)(void*), void* param, ThreadRecord** thread, int32_t flags, int32_t stackSize) {
    if (!pool) {
        if (!System_Thread::s_defaultPool && !System_Thread::CreateDefaultPool()) {
            return false;
        }

        pool = System_Thread::s_defaultPool;
    }

    System_Thread::ThreadWorker* worker = pool->GetWorker(flags, stackSize);

    if (!worker) {
        return false;
    }

    Blizzard::Lock::EventWaitForever(worker->m_readyEvent.m_event);
    worker->m_task = System_Thread::NewThread(function, param, nullptr);
    worker->m_task->m_state = (flags & 1) == 0;
    worker->m_task->m_worker = worker;
    worker->m_thread->m_worker = worker;
    worker->m_taskFunction = function;
    worker->m_taskParam = param;
    *thread = worker->m_task;

    if (worker->m_task->m_state == 1) {
        Blizzard::Lock::EventSet(worker->m_wakeEvent.m_event);
    }

    return true;
}

// VERIFY: 1143ca89 manual
// OFFSET: 0x00451140
bool Blizzard::Thread::CreateDetachedFromPool(ThreadPoolRecord* pool, uint32_t (*function)(void*), void* param, int32_t flags, int32_t stackSize) {
    ThreadRecord* thread;
    bool result = CreateFromPool(pool, function, param, &thread, flags, stackSize);

    if (result) {
        Close(thread);
    }

    return result;
}

// VERIFY: c9ca2e23 auto
// OFFSET: 0x0044FB30
void System_Thread::ShutdownThreadSystem() {
    if (s_mainThread) {
        RunTLSDestructors();
        Blizzard::Thread::ThreadRecord* thread = s_mainThread;
        RemoveFromRegistry(thread);
        free(thread);
        s_mainThread = nullptr;
    }

    if (s_defaultPool) {
        delete s_defaultPool;
        s_defaultPool = nullptr;
    }
}
