#if defined(__APPLE__)

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <sys/time.h>
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

const int32_t System_Time::s_daysBeforeMonth[13] = { 0, 31, 59, 90, 120, 151, 182, 212, 243, 273, 304, 334, 365 };
uint64_t System_Time::timeScaleCycles = 1000000000ULL;
bool System_Time::s_usePerformanceCounter = true;
Blizzard::Lock::Atomic System_Time::s_tickLock;
int64_t System_Time::s_absBegin;
uint32_t System_Time::s_lastTickCount;
double System_Time::timeScaleMilliseconds = 0.000001;
Blizzard::Time::Timestamp System_Time::s_gmBegin;
uint64_t System_Time::s_tickCountHigh;
double System_Time::timeScaleNanoseconds = 1.0;
double System_Time::timeScaleSeconds = 0.000000001;
double System_Time::timeScaleMicroseconds = 0.001;

namespace {
Blizzard::Time::Timestamp NowNs() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<Blizzard::Time::Timestamp>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}
}

Blizzard::Time::Timestamp Blizzard::Time::FromFileTime(uint64_t fileTime) {
    if (fileTime <= 0x0077A5D6AA8BF851ULL) {
        return static_cast<Timestamp>(0x8000000000000000ULL);
    }
    if (fileTime <= 0x030701FFA04E87AEULL) {
        return static_cast<Timestamp>(fileTime * 100ULL + 0x5143382561530000ULL);
    }
    return 0x7FFFFFFFFFFFFFFFLL;
}

double System_Time::MeasureDrift(int64_t, LARGE_INTEGER*, DWORD*) { return 0.0; }
uint64_t System_Time::GetTickCount64() { return static_cast<uint64_t>(Blizzard::Time::Milliseconds()); }
bool System_Time::CalibrateFrequency(LARGE_INTEGER*) { return true; }
bool System_Time::InitFrequency(LARGE_INTEGER*) { return true; }

bool System_Time::TimeInit() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    s_absBegin = static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
    s_gmBegin = NowNs();
    s_usePerformanceCounter = true;
    timeScaleCycles = 1000000000ULL;
    timeScaleNanoseconds = 1.0;
    timeScaleMicroseconds = 0.001;
    timeScaleMilliseconds = 0.000001;
    timeScaleSeconds = 0.000000001;
    return true;
}

Blizzard::Time::Timestamp Blizzard::Time::GetTimestamp() {
    return NowNs();
}

int64_t Blizzard::Time::Milliseconds() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000LL + ts.tv_nsec / 1000000LL;
}

void Blizzard::Time::BreakTime(Timestamp time, TimeRec& rec) {
    time_t seconds = static_cast<time_t>(time / 1000000000LL);
    int32_t nanosecond = static_cast<int32_t>(time % 1000000000LL);
    if (nanosecond < 0) {
        nanosecond += 1000000000;
        --seconds;
    }
    tm broken;
    gmtime_r(&seconds, &broken);
    rec.day = broken.tm_mday;
    rec.hour = broken.tm_hour;
    rec.minute = broken.tm_min;
    rec.second = broken.tm_sec;
    rec.nanosecond = nanosecond;
    rec.weekday = broken.tm_wday;
    rec.year = broken.tm_year + 1900;
    rec.month = broken.tm_mon + 1;
    rec.yearday = broken.tm_yday;
}

Blizzard::Time::Timestamp Blizzard::Time::MakeTime(const TimeRec& rec) {
    tm broken = {};
    broken.tm_year = rec.year - 1900;
    broken.tm_mon = rec.month - 1;
    broken.tm_mday = rec.day;
    broken.tm_hour = rec.hour;
    broken.tm_min = rec.minute;
    broken.tm_sec = rec.second;
    time_t seconds = timegm(&broken);
    if (seconds == static_cast<time_t>(-1) && errno == EOVERFLOW) {
        return static_cast<Timestamp>(0x8000000000000000ULL);
    }
    return static_cast<Timestamp>(seconds) * 1000000000LL + rec.nanosecond;
}

#endif
