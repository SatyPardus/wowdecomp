#if defined(__APPLE__)

#include <cstdint>
#include <ctime>
#include "Engine/Source/Os/Time.h"

int32_t OsGetTimeZone() {
    time_t now = time(nullptr);
    struct tm localTime;
    if (!localtime_r(&now, &localTime)) {
        return 0;
    }
    return static_cast<int32_t>(localTime.tm_gmtoff / 60);
}

void OsGetTimeStamp(char* buffer, uint32_t size) {
    time_t now = time(nullptr);
    struct tm localTime;
    if (!buffer || !size) {
        return;
    }
    if (localtime_r(&now, &localTime)) {
        strftime(buffer, size, "%m%d%y_%H%M%S", &localTime);
    } else {
        buffer[0] = '\0';
    }
}

#endif
