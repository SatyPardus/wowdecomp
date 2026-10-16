#if defined(__APPLE__)

#include <cerrno>
#include <cstdint>
#include <ctime>
#include <mach/mach_time.h>
#include <unistd.h>
#include "Engine/Source/Os/TimeManager.h"

static OsTimeManager* s_OsTimeMgr;

static uint64_t MonotonicNanoseconds() {
    static mach_timebase_info_data_t s_timebase;
    if (!s_timebase.denom) {
        mach_timebase_info(&s_timebase);
    }
    uint64_t now = mach_absolute_time();
    return now * s_timebase.numer / s_timebase.denom;
}

OsTimeManager::TimingMethod OsTimeManager::Calibrate() {
    this->m_timingTestError = 0;
    this->m_performanceFrequency = 1000000000LL;
    return SystemMethod2;
}

uint32_t OsTimeGetTestError() {
    return s_OsTimeMgr ? s_OsTimeMgr->m_timingTestError : 0;
}

OsTimeManager::TimingMethod OsTimeGetTimingMethod() {
    return s_OsTimeMgr ? s_OsTimeMgr->m_timingMethod : OsTimeManager::NotSet;
}

const char* GetTimingMethodName(OsTimeManager::TimingMethod method) {
    switch (method) {
        case OsTimeManager::NotSet:
            return "[Not Set]";
        case OsTimeManager::BestAvailable:
            return "[Best Available]";
        case OsTimeManager::SystemMethod1:
            return "clock_gettime";
        case OsTimeManager::SystemMethod2:
            return "mach_absolute_time";
        default:
            return "[Unknown]";
    }
}

uint64_t OsTimeManager::Snapshot() {
    return static_cast<uint64_t>(static_cast<double>(MonotonicNanoseconds()) * this->m_scaleToMs + this->m_timeBegin);
}

uint32_t OsGetAsyncTimeMs() {
    if (!s_OsTimeMgr) {
        return static_cast<uint32_t>(MonotonicNanoseconds() / 1000000ULL);
    }
    return static_cast<uint32_t>(s_OsTimeMgr->Snapshot());
}

uint64_t OsGetAsyncTimeClocks() {
    return MonotonicNanoseconds();
}

int64_t OsGetAsyncClocksPerSecond() {
    return 1000000000LL;
}

OsTimeManager::OsTimeManager(OsTimeManager::TimingMethod method) {
    this->m_timingTestError = 0;
    this->m_performanceFrequency = 1000000000LL;
    this->m_timingMethod = method == SystemMethod1 ? SystemMethod1 : this->Calibrate();
    this->m_scaleToMs = 1000.0 / static_cast<double>(this->m_performanceFrequency);
    this->m_timeBegin = 0.0;
}

void OsTimeManagerCreate(OsTimeManager::TimingMethod method) {
    delete s_OsTimeMgr;
    s_OsTimeMgr = new OsTimeManager(method);
}

void OsTimeManagerDestroy() {
    delete s_OsTimeMgr;
    s_OsTimeMgr = nullptr;
}

void OsSleep(uint32_t milliseconds) {
    timespec req;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000L;
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
    }
}

#endif
