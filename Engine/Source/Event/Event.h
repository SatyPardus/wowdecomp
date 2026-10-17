#ifndef ENGINE_SOURCE_EVENT_EVENT_H
#define ENGINE_SOURCE_EVENT_EVENT_H

#include <cstdint>
#include "Engine/Source/Event/Types.h"

HEVENTCONTEXT EventCreateContextEx(int32_t interactive, EVENTHANDLERFUNC initializeHandler, EVENTHANDLERFUNC destroyHandler, uint32_t idleTime, uint32_t debugFlags);
void EventDestroy();
void EventDoMessageLoop();
// Run at most one due context without blocking the native main run loop.
// Returns false after shutdown. Call on the EventInitialize thread.
bool EventProcessPending();
// On native foreground resume, keep wall-clock timers but reset frame delta.
void EventResetElapsedTime();
HEVENTCONTEXT EventGetCurrentContext();
void EventInitialize(uint32_t threadCount, int32_t netServer);
void EventInitiateShutdown();
void EventInputGetMousePosition(float* x, float* y);
int32_t EventInputProcess(HEVENTCONTEXT contextHandle);
int32_t EventIsButtonDown(MOUSEBUTTON button);
int32_t EventIsKeyDown(KEY key);
void EventKillTimer(uint32_t id, EVENTHANDLERFUNC handler, const char* name);
void EventPostClose();
void EventPostCloseEx(HEVENTCONTEXT contextHandle);
int32_t EventQueuePost(HEVENTCONTEXT contextHandle, EVENTID id, const void* data, uint32_t bytes);
void EventRegister(EVENTID id, EVENTHANDLERFUNC handler);
void EventRegisterEx(EVENTID id, EVENTHANDLERFUNC handler, void* param, float priority);
void EventSetConfirmCloseCallback(int32_t (*callback)(void*), void* param);
void EventSetMouseMode(MOUSEMODE mode, uint32_t holdButton);
uint32_t EventSetTimer(uint32_t delay, EVENTHANDLERFUNC handler, void* param);
uint32_t EventSetTimer(uint32_t delay, EVENTHANDLERGUIDFUNC handler, uint64_t guid, void* param);
void EventUnregister(EVENTID id, EVENTHANDLERFUNC handler);
void EventUnregisterEx(EVENTID id, EVENTHANDLERFUNC handler, void* param, uint32_t flags);

#endif
