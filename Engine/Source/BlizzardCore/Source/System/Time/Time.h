#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_TIME_TIME_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_TIME_TIME_H

#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#else
using DWORD = uint32_t;
struct LARGE_INTEGER {
    int64_t QuadPart;
};
#endif
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Types.h"

namespace System_Time {

extern const int32_t s_daysBeforeMonth[13];
extern uint64_t timeScaleCycles;
extern bool s_usePerformanceCounter;
extern Blizzard::Lock::Atomic s_tickLock;
extern int64_t s_absBegin;
extern uint32_t s_lastTickCount;
extern double timeScaleMilliseconds;
extern Blizzard::Time::Timestamp s_gmBegin;
extern uint64_t s_tickCountHigh;
extern double timeScaleNanoseconds;
extern double timeScaleSeconds;
extern double timeScaleMicroseconds;

bool CalibrateFrequency(LARGE_INTEGER* frequency);
uint64_t GetTickCount64();
bool InitFrequency(LARGE_INTEGER* frequency);
double MeasureDrift(int64_t frequency, LARGE_INTEGER* counter, DWORD* tickCount);
bool TimeInit();

}

namespace Blizzard {
namespace Time {

void BreakTime(Timestamp time, TimeRec& rec);
Timestamp FromFileTime(uint64_t fileTime);
void GetElapsedSinceStart(bool start);
Timestamp GetTimestamp();
Timestamp MakeTime(const TimeRec& rec);
int64_t Milliseconds();

}
}

#endif
