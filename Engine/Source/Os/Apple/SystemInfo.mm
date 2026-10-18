#include "Engine/Source/Os/System.h"
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <pwd.h>
#include <sys/param.h>
#include <unistd.h>
#include <vector>

namespace {
int32_t CopyName(const char* value, char* buffer, uint32_t* size, bool includeTerminator) {
    if (!size) { errno = EINVAL; return 0; }
    const size_t length = std::strlen(value);
    if (!buffer || *size <= length) {
        if (buffer && *size) buffer[0] = 0;
        *size = uint32_t(length + 1); errno = ERANGE; return 0;
    }
    std::memcpy(buffer, value, length + 1);
    // Original GetUserName includes NUL in successful size; GetComputerName
    // excludes it. Both report required space including NUL on failure.
    *size = uint32_t(length + (includeTerminator ? 1 : 0));
    return 1;
}
}
int32_t OsGetComputerName(char* buffer, uint32_t* size) {
    char name[MAXHOSTNAMELEN + 1]{};
    if (gethostname(name, sizeof(name) - 1)) return 0;
    return CopyName(name, buffer, size, false);
}
int32_t OsGetUserName(char* buffer, uint32_t* size) {
    if (!size) { errno = EINVAL; return 0; }
    long preferred = sysconf(_SC_GETPW_R_SIZE_MAX);
    size_t capacity = preferred > 0 && preferred <= 1024 * 1024 ? size_t(preferred) : 16384;
    for (;;) {
        std::vector<char> storage(capacity);
        passwd account{}, *result = nullptr;
        const int error = getpwuid_r(geteuid(), &account, storage.data(), storage.size(), &result);
        if (!error && result && result->pw_name) return CopyName(result->pw_name, buffer, size, true);
        if (error != ERANGE || capacity >= 1024 * 1024) { errno = error ? error : ENOENT; return 0; }
        capacity = std::min(capacity * 2, size_t(1024 * 1024));
    }
}
void OsGetVersionString(char* buffer, int32_t size) {
    if (!buffer || size <= 0) { errno = EINVAL; return; }
    @autoreleasepool {
        const NSOperatingSystemVersion version = NSProcessInfo.processInfo.operatingSystemVersion;
#if TARGET_OS_SIMULATOR
        const char* platform = "iOS Simulator";
#elif TARGET_OS_IPHONE
        const char* platform = "iOS";
#else
        const char* platform = "macOS";
#endif
        std::snprintf(buffer, size_t(size), "%s %ld.%ld.%ld", platform,
            long(version.majorVersion), long(version.minorVersion), long(version.patchVersion));
    }
}
