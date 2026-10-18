#if defined(__APPLE__)
#include <atomic>
#include <chrono>
#include <thread>
#include <time.h>
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Os/Time.h"

// The raw uptime clock is monotonic, independent of wall-clock corrections,
// and excludes device sleep. Nanoseconds keep profiling units explicit.
static std::atomic<OsTimeManager::TimingMethod> s_method{OsTimeManager::SystemMethod2};

OsTimeManager::TimingMethod OsTimeManager::Calibrate() {
    m_timingTestError = 0;
    m_performanceFrequency = 1000000000;
    return SystemMethod2;
}
OsTimeManager::OsTimeManager(TimingMethod method) {
    Calibrate();
    m_timingMethod = method == SystemMethod1 ? SystemMethod1 : SystemMethod2;
    m_scaleToMs = m_timingMethod == SystemMethod1 ? 1.0 : 0.000001;
    m_timeBegin = 0;
}
uint64_t OsTimeManager::Snapshot() {
    const uint64_t time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    const uint64_t clocks = m_timingMethod == SystemMethod1 ? time / 1000000 : time;
    return static_cast<uint64_t>(static_cast<double>(clocks) * m_scaleToMs + m_timeBegin);
}
uint32_t OsTimeGetTestError() { return 0; }
OsTimeManager::TimingMethod OsTimeGetTimingMethod() { return s_method.load(); }
const char* GetTimingMethodName(OsTimeManager::TimingMethod method) {
    switch (method) {
        case OsTimeManager::NotSet: return "[Not Set]";
        case OsTimeManager::BestAvailable: return "[Best Available]";
        case OsTimeManager::SystemMethod1: return "Apple uptime (milliseconds)";
        case OsTimeManager::SystemMethod2: return "Apple uptime (nanoseconds)";
        default: return "[Unknown]";
    }
}
uint32_t OsGetAsyncTimeMs() {
    return static_cast<uint32_t>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000);
}
uint64_t OsGetAsyncTimeClocks() {
    const uint64_t time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    return s_method.load() == OsTimeManager::SystemMethod1 ? time / 1000000 : time;
}
int64_t OsGetAsyncClocksPerSecond() {
    return s_method.load() == OsTimeManager::SystemMethod1 ? 1000 : 1000000000;
}
void OsTimeManagerCreate(OsTimeManager::TimingMethod method) {
    s_method.store(method == OsTimeManager::SystemMethod1 ? method : OsTimeManager::SystemMethod2);
}
void OsTimeManagerDestroy() { s_method.store(OsTimeManager::NotSet); }
void OsSleep(uint32_t milliseconds) {
    if (!milliseconds) { std::this_thread::yield(); }
    else { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); }
}
void OsGetTimeStamp(char* buffer, uint32_t size) {
    if (!buffer || !size) return;
    const time_t now = time(nullptr); tm local{};
    if (!localtime_r(&now, &local) || !strftime(buffer, size, "%m%d%y_%H%M%S", &local)) buffer[0] = 0;
}
#endif
