#if defined(__APPLE__)
#include "Engine/Source/Os/OsCall.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include <memory>
#include <mutex>

namespace {
std::mutex& ContextMutex() { static auto* mutex = new std::mutex; return *mutex; }
struct ThreadState {
    std::unique_ptr<OsCallThread> thread;
    void Clear() {
        std::lock_guard<std::mutex> lock(ContextMutex());
        if (thread && thread->m_context) { thread->m_context->m_thread = nullptr; }
        thread.reset();
    }
    ~ThreadState() { Clear(); }
};
thread_local ThreadState state;
}
OsCallContext::~OsCallContext() = default;
void OsCallInitialize(const char* name) {
    state.Clear();
    state.thread = std::make_unique<OsCallThread>();
    state.thread->m_threadId = SGetCurrentThreadId();
    SStrPrintf(state.thread->m_name, sizeof(state.thread->m_name), "%s [%u]", name, state.thread->m_threadId);
}
void OsCallDestroy() { state.Clear(); }
void* OsCallInitializeContext(const char* name) {
    auto* context = new OsCallContext();
    context->unused_000C = -1;
    SStrCopy(context->m_name, name, sizeof(context->m_name));
    return context;
}
void OsCallSetContext(void* value) {
    if (!state.thread || !value) { return; }
    std::lock_guard<std::mutex> lock(ContextMutex());
    auto* context = static_cast<OsCallContext*>(value);
    if (state.thread->m_context) { state.thread->m_context->m_thread = nullptr; }
    context->m_thread = state.thread.get();
    state.thread->m_context = context;
}
void OsCallResetContext(void* value) {
    if (!value) { return; }
    std::lock_guard<std::mutex> lock(ContextMutex());
    auto* context = static_cast<OsCallContext*>(value);
    if (context->m_thread) { context->m_thread->m_context = nullptr; }
    context->m_thread = nullptr;
}
void OsCallDestroyContext(void* value) {
    if (!value) { return; }
    OsCallResetContext(value);
    delete static_cast<OsCallContext*>(value);
}
#endif
