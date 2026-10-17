#if defined(__APPLE__)
#include <cerrno>
#include <mutex>
#include <pthread.h>
#include <utility>
#include <vector>
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

namespace {
struct Registry {
    std::mutex mutex;
    std::vector<std::pair<pthread_key_t, Blizzard::Thread::TLSDestructor>> keys;
};
Registry& Slots() {
    // Engine TLS slots live for the process. Keep their registry available to
    // pthread destructors even during static teardown on external threads.
    static auto* registry = new Registry;
    return *registry;
}
}

bool System_Thread::AllocateTLSSlot(Blizzard::Thread::TLSSlot* slot, Blizzard::Thread::TLSDestructor destructor) {
    auto& registry = Slots();
    std::lock_guard<std::mutex> lock(registry.mutex);
    if (__atomic_load_n(&slot->m_key, __ATOMIC_ACQUIRE)) { return true; }
    pthread_key_t key;
    const int result = pthread_key_create(&key, destructor);
    if (result) { errno = result; return false; }
    registry.keys.emplace_back(key, destructor);
    slot->m_destructor = destructor;
    // Zero is the uninitialized value; the native key itself may be zero.
    __atomic_store_n(&slot->m_key, uintptr_t(key) + 1, __ATOMIC_RELEASE);
    return true;
}

bool System_Thread::AllocateLocalStorage(Blizzard::Thread::TLSSlot* slot, Blizzard::Thread::TLSDestructor destructor) {
    return AllocateTLSSlot(slot, destructor);
}

void System_Thread::InternalSetLocalStorage(const Blizzard::Thread::TLSSlot* slot, const void* value) {
    const auto key = __atomic_load_n(&slot->m_key, __ATOMIC_ACQUIRE);
    if (key) { pthread_setspecific(pthread_key_t(key - 1), value); }
}

void* Blizzard::Thread::RegisterLocalStorage(TLSSlot* slot, void* (*constructor)(void*), void* param, TLSDestructor destructor) {
    if (!__atomic_load_n(&slot->m_key, __ATOMIC_ACQUIRE) && !System_Thread::AllocateTLSSlot(slot, destructor)) { return nullptr; }
    const auto key = pthread_key_t(__atomic_load_n(&slot->m_key, __ATOMIC_ACQUIRE) - 1);
    if (void* value = pthread_getspecific(key)) { return value; }
    void* value = constructor(param);
    const int result = pthread_setspecific(key, value);
    if (result) {
        if (destructor && value) { destructor(value); }
        errno = result;
        return nullptr;
    }
    return value;
}

void System_Thread::RunTLSDestructors() {
    // Match pthread's bounded destructor iteration, including destructors that
    // register a new value. Clear before calling so explicit cleanup and native
    // thread exit cannot destroy the same value twice.
    for (int pass = 0; pass < PTHREAD_DESTRUCTOR_ITERATIONS; ++pass) {
        std::vector<std::pair<pthread_key_t, Blizzard::Thread::TLSDestructor>> keys;
        {
            auto& registry = Slots();
            std::lock_guard<std::mutex> lock(registry.mutex);
            keys = registry.keys;
        }
        bool destroyed = false;
        for (const auto& entry : keys) {
            if (!entry.second) { continue; }
            if (void* value = pthread_getspecific(entry.first)) {
                pthread_setspecific(entry.first, nullptr);
                entry.second(value);
                destroyed = true;
            }
        }
        if (!destroyed) { break; }
    }
}
#endif
