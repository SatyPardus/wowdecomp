#if defined(__APPLE__)

#include <chrono>
#include <condition_variable>
#include <mutex>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"

struct Blizzard::Lock::EventState {
    std::mutex mutex;
    std::condition_variable condition;
    bool signaled = false;
};

void Blizzard::Lock::EventCreate(Event& event) {
    event = new EventState{};
}

void Blizzard::Lock::EventDestroy(Event& event) {
    delete event;
    event = nullptr;
}

void Blizzard::Lock::EventSet(Event& event) {
    {
        std::lock_guard<std::mutex> lock(event->mutex);
        event->signaled = true;
    }
    event->condition.notify_one();
}

void Blizzard::Lock::EventWaitForever(Event& event) {
    std::unique_lock<std::mutex> lock(event->mutex);
    event->condition.wait(lock, [&event] { return event->signaled; });
    event->signaled = false;
}

Blizzard::Lock::Event* Blizzard::Lock::EventWait(uint32_t timeout, Event& event) {
    std::unique_lock<std::mutex> lock(event->mutex);
    if (!event->condition.wait_for(lock, std::chrono::milliseconds(timeout), [&event] { return event->signaled; })) {
        return nullptr;
    }
    event->signaled = false;
    return &event;
}

Blizzard::Lock::EventObject::EventObject() {
    EventCreate(this->m_event);
}

Blizzard::Lock::EventObject::~EventObject() {
    EventDestroy(this->m_event);
}

#endif
