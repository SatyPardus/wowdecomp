#if defined(__APPLE__)
#include <climits>
#include <ctime>
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

namespace Blizzard::Time {
// Blizzard timestamps count nanoseconds from 2000-01-01 UTC.
static constexpr int64_t UnixToBlizzard = 946684800;
static Timestamp Saturate(__int128 value) {
    return value < INT64_MIN ? INT64_MIN : value > INT64_MAX ? INT64_MAX : Timestamp(value);
}
Timestamp FromFileTime(uint64_t value) {
    return Saturate((__int128(value) - 125911584000000000LL) * 100);
}
Timestamp GetTimestamp() {
    // Anchor wall time once, then use monotonic elapsed time, as in the original
    // calibrated-performance-counter path. Wall clock changes cannot jump IO deadlines.
    struct Anchor {
        timespec wall{}, monotonic{};
        Anchor() { clock_gettime(CLOCK_REALTIME, &wall); clock_gettime(CLOCK_MONOTONIC, &monotonic); }
    };
    static const Anchor anchor;
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return Saturate((__int128(anchor.wall.tv_sec) - UnixToBlizzard + now.tv_sec - anchor.monotonic.tv_sec) * 1000000000
        + anchor.wall.tv_nsec + now.tv_nsec - anchor.monotonic.tv_nsec);
}
int64_t Milliseconds() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return int64_t(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
}
void BreakTime(Timestamp value, TimeRec& result) {
    int64_t seconds = value / 1000000000;
    int32_t nanos = value % 1000000000;
    if (nanos < 0) { nanos += 1000000000; --seconds; }
    time_t unixTime = seconds + UnixToBlizzard;
    tm date{};
    gmtime_r(&unixTime, &date);
    result.year = date.tm_year + 1900;
    result.month = date.tm_mon + 1;
    result.day = date.tm_mday;
    result.hour = date.tm_hour;
    result.minute = date.tm_min;
    result.second = date.tm_sec;
    result.nanosecond = nanos;
    result.weekday = date.tm_wday;
    result.yearday = date.tm_yday;
}
Timestamp MakeTime(const TimeRec& value) {
    tm date{};
    date.tm_year = value.year - 1900;
    date.tm_mon = value.month - 1;
    date.tm_mday = value.day;
    date.tm_hour = value.hour;
    date.tm_min = value.minute;
    date.tm_sec = value.second;
    return Saturate((__int128(timegm(&date)) - UnixToBlizzard) * 1000000000 + value.nanosecond);
}
}
#endif
