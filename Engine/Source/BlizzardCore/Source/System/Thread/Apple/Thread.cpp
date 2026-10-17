#if defined(__APPLE__)
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <new>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"

namespace System_Thread {
struct NativeThreadState {
    pthread_t native{};
    std::mutex mutex;
    std::condition_variable completed;
    bool done = false;
    char name[64]{};
};
}

namespace Blizzard::Thread {
bool Close(ThreadRecord* thread) {
    if (!thread) { errno = EINVAL; return false; }
    if (thread->m_refCount.Decrement() == 0) {
        delete thread->m_handle;
        delete thread;
    }
    return true;
}

static void* Start(void* argument) {
    auto* thread = static_cast<ThreadRecord*>(argument);
    auto* state = thread->m_handle;
    if (state->name[0]) { pthread_setname_np(state->name); }
    thread->m_result = thread->m_function(thread->m_param);
    System_Thread::RunTLSDestructors();
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        __atomic_store_n(&thread->m_state, -1, __ATOMIC_RELEASE);
        state->done = true;
        state->completed.notify_all();
    }
    Close(thread); // release execution ownership; caller may already be gone
    return nullptr;
}

bool Create(uint32_t (*function)(void*), void* param, ThreadRecord** output, const char* name, int32_t flags, int32_t stackSize) {
    if (!output) { errno = EINVAL; return false; }
    *output = nullptr;
    if (!function || stackSize < 0) { errno = EINVAL; return false; }
    // There is no resume operation in this API. Do not silently start callers
    // that requested the Windows suspended-thread flag.
    if (flags & 1) { errno = ENOTSUP; return false; }
    auto* thread = new (std::nothrow) ThreadRecord{};
    if (!thread) { errno = ENOMEM; return false; }
    auto* state = new (std::nothrow) System_Thread::NativeThreadState;
    if (!state) { delete thread; errno = ENOMEM; return false; }
    thread->m_handle = state;
    thread->m_function = function;
    thread->m_param = param;
    thread->m_refCount.m_value = 2;
    thread->m_state = 1;
    if (name) {
        std::strncpy(state->name, name, sizeof(state->name) - 1);
        std::strncpy(thread->m_name, name, sizeof(thread->m_name) - 1);
    }
    pthread_attr_t attributes;
    int result = pthread_attr_init(&attributes);
    if (!result) {
        result = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
        if (!result && stackSize) {
            const size_t page = size_t(sysconf(_SC_PAGESIZE));
            const size_t size = std::max(size_t(stackSize), size_t(PTHREAD_STACK_MIN));
            result = pthread_attr_setstacksize(&attributes, (size + page - 1) / page * page);
        }
        if (!result) { result = pthread_create(&state->native, &attributes, Start, thread); }
        pthread_attr_destroy(&attributes);
    }
    if (result) { delete state; delete thread; errno = result; return false; }
    *output = thread;
    return true;
}

bool CreateDetached(uint32_t (*function)(void*), void* param, const char* name) {
    ThreadRecord* thread;
    return Create(function, param, &thread, name, 0, 0) && Close(thread);
}

bool IsRunning(ThreadRecord* thread) { return __atomic_load_n(&thread->m_state, __ATOMIC_ACQUIRE) >= 0; }
bool WaitForever(ThreadRecord* thread) {
    if (!thread || !thread->m_handle) { errno = EINVAL; return false; }
    auto* state = thread->m_handle;
    if (pthread_equal(pthread_self(), state->native)) { errno = EDEADLK; return false; }
    std::unique_lock<std::mutex> lock(state->mutex);
    state->completed.wait(lock, [&] { return state->done; });
    return true;
}
bool Sleep(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return true;
}
}
#endif
