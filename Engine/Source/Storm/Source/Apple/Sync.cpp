#if defined(__APPLE__)
#include "Engine/Source/Storm/Source/SThread.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace StormApple {
enum class Kind { Event, Semaphore, Mutex, Thread };
struct SyncState {
    Kind kind;
    bool manual = false;
    bool signaled = false;
    bool abandoned = false;
    uint32_t count = 0;
    uint32_t maximum = 0;
    uint32_t owner = 0;
    explicit SyncState(Kind value) : kind(value) {}
};

// One condition variable makes wait-all an atomic acquisition of the complete
// set. A timeout must not consume a semaphore or an auto-reset event early.
struct WaitDomain {
    std::mutex mutex;
    std::condition_variable changed;
    std::atomic<uint32_t> nextId{1};
};
static WaitDomain& Domain() {
    // Synchronization objects also live in other translation units' statics.
    // Keep their wait domain alive through static and thread-local destruction.
    static auto* domain = new WaitDomain;
    return *domain;
}

struct ThreadIdentity {
    uint32_t id = Domain().nextId.fetch_add(1, std::memory_order_relaxed);
    std::vector<std::weak_ptr<SyncState>> owned;
    void Abandon() {
        auto& domain = Domain();
        std::lock_guard<std::mutex> lock(domain.mutex);
        for (auto& weak : owned) {
            if (auto state = weak.lock(); state && state->owner == id) {
                state->owner = 0;
                state->count = 0;
                state->abandoned = true;
            }
        }
        owned.clear();
        domain.changed.notify_all();
    }
    ~ThreadIdentity() { Abandon(); }
};
static ThreadIdentity& Identity() {
    thread_local ThreadIdentity identity;
    return identity;
}

static bool Ready(const SyncState& state, uint32_t id) {
    switch (state.kind) {
        case Kind::Event: case Kind::Thread: return state.signaled;
        case Kind::Semaphore: return state.count != 0;
        case Kind::Mutex: return !state.owner || state.owner == id;
    }
    return false;
}
static bool Acquire(const std::shared_ptr<SyncState>& state, ThreadIdentity& identity) {
    if (state->kind == Kind::Event && !state->manual) { state->signaled = false; }
    if (state->kind == Kind::Semaphore) { --state->count; }
    if (state->kind == Kind::Mutex) {
        if (!state->owner) { identity.owned.emplace_back(state); }
        state->owner = identity.id;
        ++state->count;
        return std::exchange(state->abandoned, false);
    }
    return false;
}
static std::shared_ptr<SyncState> State(SSyncObject& object) {
    return std::atomic_load(&object.m_opaqueData);
}
static void Store(SSyncObject& object, std::shared_ptr<SyncState> state) {
    std::atomic_store(&object.m_opaqueData, std::move(state));
}
static int32_t Failure(int error) { errno = error; return 0; }
static void Check(int result) {
    if (result) { throw std::system_error(result, std::generic_category(), "Storm native lock"); }
}
}

using namespace StormApple;

SSyncObject::~SSyncObject() { Close(); }
void SSyncObject::Close() { Store(*this, nullptr); }
int32_t SSyncObject::Valid() { return bool(State(*this)); }
uint32_t SSyncObject::Wait(uint32_t timeout) {
    SSyncObject* object = this;
    if (!Valid()) { errno = EINVAL; return UINT32_MAX; }
    return WaitMultiplePtr(1, &object, 0, timeout);
}
uint32_t SSyncObject::WaitMultiplePtr(uint32_t count, SSyncObject** objects, int32_t waitAll, uint32_t timeout) {
    if (count > 64 || !objects) { errno = EINVAL; return UINT32_MAX; }
    std::array<std::shared_ptr<SyncState>, 64> states;
    size_t used = 0;
    // Original Storm ignores null objects and returns indices in this compact set.
    for (uint32_t i = 0; i < count; ++i) {
        if (!objects[i]) { continue; }
        auto state = State(*objects[i]);
        if (!state) { continue; }
        if (std::find(states.begin(), states.begin() + used, state) != states.begin() + used) {
            errno = EINVAL; return UINT32_MAX;
        }
        states[used++] = std::move(state);
    }
    if (!used) { errno = EINVAL; return UINT32_MAX; }
    auto& identity = Identity();
    auto& domain = Domain();
    std::unique_lock<std::mutex> lock(domain.mutex);
    auto ready = [&] {
        if (waitAll) {
            return std::all_of(states.begin(), states.begin() + used, [&](auto& s) { return Ready(*s, identity.id); });
        }
        return std::any_of(states.begin(), states.begin() + used, [&](auto& s) { return Ready(*s, identity.id); });
    };
    if (timeout == UINT32_MAX) { domain.changed.wait(lock, ready); }
    else if (!domain.changed.wait_for(lock, std::chrono::milliseconds(timeout), ready)) { return 258; }
    uint32_t abandoned = UINT32_MAX;
    for (uint32_t i = 0; i < used; ++i) {
        if (!Ready(*states[i], identity.id)) { continue; }
        bool wasAbandoned = Acquire(states[i], identity);
        if (!waitAll) { return i + (wasAbandoned ? 0x80 : 0); }
        if (wasAbandoned && abandoned == UINT32_MAX) { abandoned = i; }
    }
    return abandoned == UINT32_MAX ? 0 : 0x80 + abandoned;
}

SEvent::SEvent(int32_t manualReset, int32_t initialState) {
    auto state = std::make_shared<SyncState>(Kind::Event);
    state->manual = manualReset != 0;
    state->signaled = initialState != 0;
    Store(*this, std::move(state));
}
int32_t SEvent::Set() {
    auto state = State(*this);
    if (!state) { return Failure(EINVAL); }
    auto& domain = Domain();
    std::lock_guard<std::mutex> lock(domain.mutex);
    state->signaled = true;
    domain.changed.notify_all();
    return 1;
}
int32_t SEvent::Reset() {
    auto state = State(*this);
    if (!state) { return Failure(EINVAL); }
    std::lock_guard<std::mutex> lock(Domain().mutex);
    state->signaled = false;
    return 1;
}
SSemaphore::SSemaphore(int32_t initialCount, int32_t maximumCount) {
    if (initialCount < 0 || maximumCount <= 0 || initialCount > maximumCount) { Failure(EINVAL); return; }
    auto state = std::make_shared<SyncState>(Kind::Semaphore);
    state->count = initialCount;
    state->maximum = maximumCount;
    Store(*this, std::move(state));
}
int32_t SSemaphore::Signal(uint32_t count) {
    auto state = State(*this);
    if (!state || !count) { return Failure(EINVAL); }
    auto& domain = Domain();
    std::lock_guard<std::mutex> lock(domain.mutex);
    if (count > state->maximum - state->count) { return Failure(EOVERFLOW); }
    state->count += count;
    domain.changed.notify_all();
    return 1;
}
SMutex::SMutex() { Create(0, nullptr); }
void SMutex::Create(int32_t initialOwner, const char* name) {
    Close();
    // Named Windows mutexes have cross-process identity. Do not misrepresent an
    // ordinary process-local mutex as providing that contract.
    if (name) { Failure(ENOTSUP); return; }
    auto state = std::make_shared<SyncState>(Kind::Mutex);
    if (initialOwner) { Acquire(state, Identity()); }
    Store(*this, std::move(state));
}
int32_t SMutex::Release() {
    auto state = State(*this);
    if (!state) { return Failure(EINVAL); }
    auto& identity = Identity();
    auto& domain = Domain();
    std::lock_guard<std::mutex> lock(domain.mutex);
    if (state->owner != identity.id) { return Failure(EPERM); }
    if (!--state->count) {
        state->owner = 0;
        auto& owned = identity.owned;
        owned.erase(std::remove_if(owned.begin(), owned.end(), [&](auto& weak) {
            auto value = weak.lock();
            return !value || value == state;
        }), owned.end());
        domain.changed.notify_all();
    }
    return 1;
}

CSRWLock::CSRWLock() { Check(pthread_rwlock_init(&m_lock, nullptr)); }
CSRWLock::~CSRWLock() { if (pthread_rwlock_destroy(&m_lock)) { std::terminate(); } }
void CSRWLock::Enter(int32_t writing) { Check(writing ? pthread_rwlock_wrlock(&m_lock) : pthread_rwlock_rdlock(&m_lock)); }
void CSRWLock::Leave(int32_t) { Check(pthread_rwlock_unlock(&m_lock)); }
int32_t CInitCritSect::Enter() {
    m_critsect.Enter();
    return !std::exchange(m_initialized, true);
}
void CInitCritSect::Leave() { m_critsect.Leave(); }

namespace {
struct ThreadStart {
    std::shared_ptr<SyncState> state;
    uint32_t (STORMAPI* function)(void*);
    void* parameter;
    char name[64]{};
};
void* StartThread(void* parameter) {
    std::unique_ptr<ThreadStart> start(static_cast<ThreadStart*>(parameter));
    if (start->name[0]) { pthread_setname_np(start->name); }
    start->function(start->parameter);
    System_Thread::RunTLSDestructors();
    Identity().Abandon();
    auto& domain = Domain();
    std::lock_guard<std::mutex> lock(domain.mutex);
    start->state->signaled = true;
    domain.changed.notify_all();
    return nullptr;
}
}

int32_t SThread::Create(uint32_t (STORMAPI* function)(void*), void* parameter, SThread& thread, char* name, uint32_t stackSize) {
    if (!function || thread.Valid()) { return Failure(EINVAL); }
    auto start = std::make_unique<ThreadStart>();
    start->state = std::make_shared<SyncState>(Kind::Thread);
    auto state = start->state;
    start->function = function;
    start->parameter = parameter;
    if (name) { std::strncpy(start->name, name, sizeof(start->name) - 1); }
    pthread_attr_t attributes;
    int result = pthread_attr_init(&attributes);
    if (result) { return Failure(result); }
    result = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (!result && stackSize) {
        size_t page = size_t(sysconf(_SC_PAGESIZE));
        size_t bytes = std::max(size_t(stackSize), size_t(PTHREAD_STACK_MIN));
        result = pthread_attr_setstacksize(&attributes, (bytes + page - 1) / page * page);
    }
    pthread_t native;
    // Transfer before pthread_create: the worker can finish before it returns.
    auto* argument = start.release();
    if (!result) { result = pthread_create(&native, &attributes, StartThread, argument); }
    pthread_attr_destroy(&attributes);
    if (result) { delete argument; return Failure(result); }
    Store(thread, std::move(state));
    return 1;
}
int32_t SThread::SetPriority(int32_t priority) {
    if (!Valid() || priority < 0 || priority >= 6) { return Failure(EINVAL); }
    return Failure(ENOTSUP); // no Windows realtime-priority emulation
}
uint32_t STORMAPI SGetCurrentThreadId() { return Identity().id; }
uint32_t STORMAPI SGetCurrentProcessId() { return uint32_t(getpid()); }
#endif
