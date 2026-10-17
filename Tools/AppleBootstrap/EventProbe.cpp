#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
struct State {
    std::vector<int> order;
    int initialized = 0, destroyed = 0, timers = 0, paints = 0, keys = 0, messages = 0;
    HEVENTCONTEXT context = nullptr;
    float elapsed = -1;
};
State* state;
int32_t Timer(const void*, void*) { ++state->timers; state->order.push_back(2); return 1; }
int32_t Update(const void* data, void*) { state->elapsed = static_cast<const EVENT_DATA_UPDATE*>(data)->m_elapsedSec; state->order.push_back(1); return 1; }
int32_t Poll(const void*, void*) { state->order.push_back(3); return 1; }
int32_t Message(const void* data, void*) {
    Require(*static_cast<const uint32_t*>(data) == 0x12345678, "Event queue payload changed");
    ++state->messages; state->order.push_back(4); return 1;
}
int32_t Idle(const void*, void*) { state->order.push_back(5); return 1; }
int32_t Paint(const void*, void*) { ++state->paints; state->order.push_back(6); return 1; }
int32_t KeyDown(const void*, void*) {
    Require(EventIsKeyDown(KEY_A), "Event synchronous key state was not updated before callback");
    ++state->keys; return 1;
}
int32_t Initialize(const void*, void*) {
    ++state->initialized; state->context = EventGetCurrentContext();
    EventRegister(EVENT_ID_UPDATE, Update); EventRegister(EVENT_ID_POLL, Poll);
    EventRegister(EVENT_ID_IDLE, Idle); EventRegister(EVENT_ID_PAINT, Paint);
    EventRegister(EVENT_ID_KEYDOWN, KeyDown); EventRegister(EVENT_ID_KEYDOWN_REPEATING, KeyDown);
    EventRegister(EVENT_ID_0, Message);
    EventSetTimer(0, Timer, nullptr);
    EventSetTimer(20, Timer, nullptr);
    auto canceled = EventSetTimer(0, Timer, nullptr);
    EventKillTimer(canceled, Timer, "cancelled native timer");
    return 1;
}
int32_t Destroy(const void*, void*) { ++state->destroyed; return 1; }
}
void CheckGameEvents() {
    // Repeat complete teardown/reinitialization, as an iOS host can recreate its
    // game context after relinquishing graphics resources in the background.
    for (int cycle = 0; cycle < 2; ++cycle) {
        State data; state = &data;
        PropInitialize(); EventInitialize(1, 0);
        struct Cleanup { bool active = true; ~Cleanup() { if (active) { EventDestroy(); } PropDestroy(); state = nullptr; } } cleanup;
        auto context = EventCreateContextEx(1, Initialize, Destroy, 1, 1);
        Require(context != nullptr, "Cannot create native Event context");
        uint32_t payload = 0x12345678;
        EventQueuePost(context, EVENT_ID_0, &payload, sizeof(payload));
        OsInputSetDrawableSize(200, 100);
        OsQueuePut(OS_INPUT_KEY_DOWN, KEY_A, 1, 0, 0);
        OsQueuePut(OS_INPUT_KEY_DOWN, KEY_A, 2, 0, 0);
        OsQueuePut(OS_INPUT_KEY_UP, KEY_A, 1, 0, 0);
        Require(EventProcessPending(), "Native Event loop shut down early");
        Require(data.context == context && data.initialized == 1 && data.timers == 1 && data.paints == 1 && data.keys == 2 && data.messages == 1,
            "Native Event initialization, timer, input or queued message failed");
        Require(data.order == std::vector<int>({1,2,3,4,5,6}), "Native Event dispatch order changed");
        Require(!EventGetCurrentContext() && !OsInputIsKeyDown(KEY_A), "Native Event leaked selected context or input state");
        std::thread producer([&] {
            for (int i = 0; i < 80; ++i) { EventQueuePost(context, EVENT_ID_0, &payload, sizeof(payload)); }
        });
        producer.join();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        EventResetElapsedTime();
        Require(EventProcessPending(), "Native Event resume failed");
        Require(data.timers == 2 && data.messages == 81 && data.elapsed == 0,
            "Native Event resume lost wall-clock timers, queued messages or frame-time reset");
        // Shutdown destroys contexts without requiring another UI frame.
        EventInitiateShutdown();
        Require(!EventProcessPending(), "Native Event did not report shutdown");
        // Verify destruction before the scope releases its property TLS.
        EventDestroy(); cleanup.active = false;
        Require(data.destroyed == 1, "Native Event context was not destroyed exactly once");
    }
    State future; state = &future;
    PropInitialize(); EventInitialize(1, 0);
    struct Cleanup { ~Cleanup() { EventDestroy(); PropDestroy(); state = nullptr; } } cleanup;
    EventCreateContextEx(0, [](const void*, void*) -> int32_t {
        EventRegister(EVENT_ID_UPDATE, Update);
        EventSetTimer(1000, Timer, nullptr);
        return 1;
    }, nullptr, 1000, 0);
    EventProcessPending(); future.order.clear();
    const auto start = std::chrono::steady_clock::now();
    EventProcessPending();
    Require(future.order.empty() && !future.timers && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100),
        "Native Event step blocked or dispatched a future context early");

}
