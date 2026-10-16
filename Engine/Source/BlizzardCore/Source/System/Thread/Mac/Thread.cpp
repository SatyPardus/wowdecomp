#if defined(__APPLE__)

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <set>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/System/Hardware/Hardware.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

bool System_Thread::s_initialized;
Blizzard::Lock::Mutex System_Thread::s_mutex;
Blizzard::Lock::Mutex System_Thread::s_registryMutex;
Blizzard::Thread::TLSSlot System_Thread::s_threadRecordTLS;
Blizzard::Thread::TLSSlot System_Thread::s_stackTraceEntryPointTLS;
Blizzard::Thread::ThreadRecord* System_Thread::s_mainThread;
std::set<Blizzard::Thread::ThreadRecord*, std::less<Blizzard::Thread::ThreadRecord*>, Blizzard::Memory::AllocSTL_raw<Blizzard::Thread::ThreadRecord*>>* System_Thread::s_ThreadRegistry;
int32_t System_Thread::s_DefaultStackSize;
System_Thread::ExternalTLSRecord System_Thread::s_externalTLS[32];
int32_t System_Thread::s_externalTLSCount;

namespace {

pthread_key_t KeyFromSlot(const Blizzard::Thread::TLSSlot* slot) {
    return static_cast<pthread_key_t>(slot->m_key - 1);
}

void* PThreadStarter(void* param) {
    return reinterpret_cast<void*>(static_cast<uintptr_t>(System_Thread::ThreadStarter(param)));
}

pthread_t* ThreadHandle(Blizzard::Thread::ThreadRecord* thread) {
    return static_cast<pthread_t*>(thread->m_handle);
}

} // namespace

bool Blizzard::Thread::Sleep(uint32_t milliseconds) {
    usleep(static_cast<useconds_t>(milliseconds) * 1000);
    return true;
}

bool Blizzard::Thread::IsRunning(ThreadRecord* thread) {
    return thread && thread->m_state >= 0;
}

bool System_Thread::SetPriorityLevel(Blizzard::Thread::ThreadRecord*, int32_t) {
    return true;
}

bool System_Thread::GetPriorityLevel(Blizzard::Thread::ThreadRecord*, int32_t* level) {
    if (level) {
        *level = 3;
    }
    return true;
}

void System_Thread::ShutdownThreadLocalStorage() {
    RunTLSDestructors();
}

void System_Thread::RunExternalTLSDestructors(void* thread) {
    Blizzard::Lock::ScopedMutex lock(s_mutex);
    for (int32_t i = s_externalTLSCount; i > 0;) {
        --i;
        ExternalTLSRecord& record = s_externalTLS[i];
        if (record.m_thread == thread) {
            if (record.m_destructor) {
                record.m_destructor(record.m_value);
            }
            record = s_externalTLS[--s_externalTLSCount];
        }
    }
}

void System_Thread::AddToRegistry(Blizzard::Thread::ThreadRecord* thread) {
    Blizzard::Lock::ScopedMutex lock(s_registryMutex);
    if (!s_ThreadRegistry) {
        s_ThreadRegistry = new std::set<Blizzard::Thread::ThreadRecord*, std::less<Blizzard::Thread::ThreadRecord*>, Blizzard::Memory::AllocSTL_raw<Blizzard::Thread::ThreadRecord*>>();
    }
    s_ThreadRegistry->insert(thread);
}

Blizzard::Thread::ThreadRecord* System_Thread::NewThread(uint32_t (*function)(void*), void* param, const char* name) {
    if (!name) {
        name = "";
    }
    uint32_t length = static_cast<uint32_t>(strlen(name));
    Blizzard::Thread::ThreadRecord* thread = static_cast<Blizzard::Thread::ThreadRecord*>(malloc(sizeof(Blizzard::Thread::ThreadRecord) + length + 1));
    memset(thread, 0, sizeof(Blizzard::Thread::ThreadRecord));
    thread->m_param = param;
    thread->m_function = function;
    thread->m_state = 0;
    thread->m_refCount.m_value = 2;
    Blizzard::String::Copy(thread->m_name, name, sizeof(thread->m_name));
    AddToRegistry(thread);
    return thread;
}

void System_Thread::RemoveFromRegistry(Blizzard::Thread::ThreadRecord* thread) {
    Blizzard::Lock::ScopedMutex lock(s_registryMutex);
    if (!s_ThreadRegistry) {
        return;
    }
    s_ThreadRegistry->erase(thread);
    if (s_ThreadRegistry->empty()) {
        delete s_ThreadRegistry;
        s_ThreadRegistry = nullptr;
    }
}

void System_Thread::InitMainThread() {
    s_mainThread = NewThread(nullptr, nullptr, nullptr);
    pthread_t* handle = static_cast<pthread_t*>(malloc(sizeof(pthread_t)));
    *handle = pthread_self();
    s_mainThread->m_handle = handle;
    InternalSetLocalStorage(&s_threadRecordTLS, s_mainThread);
    s_mainThread->m_state = 1;
    s_mainThread->m_refCount.m_value = 1;
}

void System_Thread::InitThreadSystem() {
    if (s_initialized) {
        return;
    }
    s_initialized = true;
    Blizzard::Lock::MutexCreate(s_mutex);
    Blizzard::Lock::MutexCreate(s_registryMutex);
    AllocateLocalStorage(&s_threadRecordTLS, nullptr);
    AllocateLocalStorage(&s_stackTraceEntryPointTLS, nullptr);
    InitMainThread();
}

void System_Thread::SetStackTraceEntryPoint(void* entryPoint) {
    InitThreadSystem();
    InternalSetLocalStorage(&s_stackTraceEntryPointTLS, entryPoint);
}

Blizzard::Thread::ThreadRecord* System_Thread::GetThreadRecord() {
    InitThreadSystem();
    return static_cast<Blizzard::Thread::ThreadRecord*>(pthread_getspecific(KeyFromSlot(&s_threadRecordTLS)));
}

bool System_Thread::AllocateLocalStorage(Blizzard::Thread::TLSSlot* slot, Blizzard::Thread::TLSDestructor destructor) {
    pthread_key_t key;
    if (pthread_key_create(&key, nullptr) != 0) {
        return false;
    }
    slot->m_key = static_cast<uint32_t>(key) + 1;
    slot->m_destructor = destructor;
    Blizzard::Lock::MutexEnter(s_mutex);
    s_slotList[s_slotListUsed++] = slot;
    Blizzard::Lock::MutexLeave(s_mutex);
    return true;
}

bool System_Thread::AllocateTLSSlot(Blizzard::Thread::TLSSlot* slot, Blizzard::Thread::TLSDestructor destructor) {
    InitThreadSystem();
    Blizzard::Lock::ScopedMutex lock(s_mutex);
    return slot->m_key || AllocateLocalStorage(slot, destructor);
}

void System_Thread::InitStackTraceEntryPoint() {
    SetStackTraceEntryPoint(__builtin_return_address(0));
}

uint32_t System_Thread::ExternalTLSWatcher(void*) {
    return 0;
}

uint32_t System_Thread::ThreadStarter(void* param) {
    Blizzard::Thread::ThreadRecord* thread = static_cast<Blizzard::Thread::ThreadRecord*>(param);
    InternalSetLocalStorage(&s_threadRecordTLS, thread);
    InitStackTraceEntryPoint();
    while (!thread->m_state) {
        Blizzard::Thread::Sleep(1);
    }
    uint32_t result = thread->m_function(thread->m_param);
    thread->m_result = result;
    RunTLSDestructors();
    thread->m_state = -1;
    Blizzard::Thread::Close(thread);
    return result;
}

bool Blizzard::Thread::Create(uint32_t (*function)(void*), void* param, ThreadRecord** thread, const char* name, int32_t flags, int32_t stackSize) {
    System_Thread::InitThreadSystem();
    ThreadRecord* record = System_Thread::NewThread(function, param, name);
    *thread = record;
    pthread_t* handle = static_cast<pthread_t*>(malloc(sizeof(pthread_t)));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (!stackSize) {
        stackSize = System_Thread::s_DefaultStackSize;
    }
    if (stackSize > 0) {
        pthread_attr_setstacksize(&attr, static_cast<size_t>(stackSize));
    }
    if (pthread_create(handle, &attr, PThreadStarter, record) != 0) {
        pthread_attr_destroy(&attr);
        free(handle);
        record->m_handle = nullptr;
        System_Thread::RemoveFromRegistry(record);
        free(record);
        *thread = nullptr;
        return false;
    }
    pthread_attr_destroy(&attr);
    record->m_handle = handle;
    record->m_threadId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(*handle));
    record->m_state = (flags & 1) == 0;
    return true;
}

void System_Thread::RegisterExternalTLSDestructor(void* value, Blizzard::Thread::TLSDestructor destructor) {
    if (destructor && value) {
        destructor(value);
    }
}

void System_Thread::InternalSetLocalStorage(const Blizzard::Thread::TLSSlot* slot, const void* value) {
    pthread_setspecific(KeyFromSlot(slot), const_cast<void*>(value));
}

void System_Thread::RunTLSDestructors() {
    if (!s_initialized) {
        return;
    }
    Blizzard::Lock::ScopedMutex lock(s_mutex);
    for (uint32_t i = 0; i < s_slotListUsed; i++) {
        Blizzard::Thread::TLSSlot* slot = s_slotList[i];
        if (slot->m_destructor) {
            void* value = pthread_getspecific(KeyFromSlot(slot));
            if (value) {
                pthread_setspecific(KeyFromSlot(slot), nullptr);
                slot->m_destructor(value);
            }
        }
    }
}

void* Blizzard::Thread::RegisterLocalStorage(TLSSlot* slot, void* (*constructor)(void*), void* param, TLSDestructor destructor) {
    if (!slot->m_key && !System_Thread::AllocateTLSSlot(slot, destructor)) {
        return nullptr;
    }
    void* value = pthread_getspecific(KeyFromSlot(slot));
    if (value) {
        return value;
    }
    value = constructor(param);
    System_Thread::InternalSetLocalStorage(slot, value);
    return value;
}

bool Blizzard::Thread::Close(ThreadRecord* thread) {
    if (!thread || thread->m_refCount.Decrement() > 0) {
        return true;
    }
    if (thread->m_handle) {
        free(thread->m_handle);
        thread->m_handle = nullptr;
    }
    System_Thread::RemoveFromRegistry(thread);
    free(thread);
    return true;
}

bool Blizzard::Thread::WaitForever(ThreadRecord* thread) {
    if (!thread || thread->m_state < 0) {
        return true;
    }
    System_Thread::ThreadWorker* worker = thread->m_worker;
    if (worker && thread != worker->m_thread) {
        if (thread == worker->m_task) {
            Blizzard::Lock::EventWaitForever(worker->m_readyEvent.m_event);
            Blizzard::Lock::EventSet(worker->m_readyEvent.m_event);
        }
        return true;
    }
    pthread_t* handle = ThreadHandle(thread);
    return handle && pthread_join(*handle, nullptr) == 0;
}

System_Thread::ThreadWorker::~ThreadWorker() {
    this->m_taskFunction = nullptr;
    Blizzard::Lock::EventSet(this->m_wakeEvent.m_event);
    Blizzard::Lock::EventWaitForever(this->m_readyEvent.m_event);
    if (this->m_thread) {
        Blizzard::Lock::EventSet(this->m_wakeEvent.m_event);
        Blizzard::Thread::WaitForever(this->m_thread);
        Blizzard::Thread::Close(this->m_thread);
        this->m_thread = nullptr;
    }
}

uint32_t System_Thread::ThreadWorker::Executive(void* param) {
    ThreadWorker* worker = static_cast<ThreadWorker*>(param);
    Blizzard::Lock::EventSet(worker->m_readyEvent.m_event);
    Blizzard::Lock::EventWaitForever(worker->m_wakeEvent.m_event);
    while (worker->m_taskFunction) {
        worker->m_task->m_state = 1;
        worker->m_task->m_result = worker->m_taskFunction(worker->m_taskParam);
        worker->m_task->m_state = -1;
        Blizzard::Thread::Close(worker->m_task);
        worker->m_task = nullptr;
        worker->m_taskFunction = nullptr;
        worker->m_taskParam = nullptr;
        {
            Blizzard::Lock::ScopedMutex lock(worker->m_pool->m_mutex.m_mutex);
            worker->m_pool->m_idle.push_back(worker);
        }
        Blizzard::Lock::EventSet(worker->m_readyEvent.m_event);
        Blizzard::Lock::EventWaitForever(worker->m_wakeEvent.m_event);
    }
    Blizzard::Lock::EventSet(worker->m_readyEvent.m_event);
    return 0;
}

bool System_Thread::ThreadWorker::Start(Blizzard::Thread::ThreadPoolRecord* pool, int32_t flags, int32_t stackSize) {
    if (!Blizzard::Thread::Create(Executive, this, &this->m_thread, "Blizzard Thread Pool Worker", flags, stackSize)) {
        return false;
    }
    GetPriorityLevel(this->m_thread, &this->m_priority);
    Blizzard::Hardware::GetProcessorFeatures();
    this->m_affinity = static_cast<int32_t>(System_Hardware::s_processorFeatures.m_processorCount);
    this->m_pool = pool;
    return true;
}

#endif
