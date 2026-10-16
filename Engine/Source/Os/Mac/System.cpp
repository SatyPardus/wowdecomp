#if defined(__APPLE__)

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <spawn.h>
#include <string>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"

extern char** environ;

static int32_t s_sleepInBackground = 1;
static uint32_t s_backgroundSleepMs;
static uint64_t s_cpuTicksPerSecond;

void OsSystemEnableCpuLog() {
}

int32_t IOsGetProcessorFeatures(CpuIdInfo* info) {
    if (info) {
        memset(info, 0, sizeof(*info));
        SStrCopy(info->m_vendor, "Apple", sizeof(info->m_vendor));
        SStrCopy(info->m_brand, "Apple Silicon", sizeof(info->m_brand));
    }
    return 1;
}

void IOsSystemCpuLog(const CpuIdInfo* info) {
    (void)info;
}

uint32_t OsGetNumberOfProcessors() {
    int count = 1;
    size_t len = sizeof(count);
    if (sysctlbyname("hw.logicalcpu", &count, &len, nullptr, 0) != 0 || count < 1) {
        count = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
    }
    return count > 0 ? static_cast<uint32_t>(count) : 1;
}

int32_t OsSleepInBackground() {
    return s_sleepInBackground;
}

void OsSetSleepInBackground(int32_t sleep) {
    s_sleepInBackground = sleep;
}

uint32_t OsGetBackgroundSleepMs() {
    return s_backgroundSleepMs;
}

void OsSetBackgroundSleepMs(uint32_t milliseconds) {
    s_backgroundSleepMs = milliseconds;
}

OsType OsGetVersion() {
    return OsType_MacOSX;
}

void OsGetVersionString(char* buffer, int32_t size) {
    if (!buffer || size <= 0) {
        return;
    }
    char version[128] = {};
    size_t len = sizeof(version);
    if (sysctlbyname("kern.osproductversion", version, &len, nullptr, 0) == 0 && version[0]) {
        SStrPrintf(buffer, size, "MacOSX %s", version);
    } else {
        SStrCopy(buffer, "MacOSX", size);
    }
}

uint32_t OsGetProcessorId() {
#if defined(__aarch64__)
    return 0x41524D;
#else
    return 0x783836;
#endif
}

uint32_t OsGetOsId() {
    return 0x4D6163;
}

static int32_t CopySized(char* buffer, uint32_t* size, const char* value) {
    if (!size) {
        errno = EINVAL;
        return 0;
    }
    uint32_t required = static_cast<uint32_t>(strlen(value));
    if (!buffer || *size <= required) {
        *size = required + 1;
        errno = ERANGE;
        return 0;
    }
    SStrCopy(buffer, value, *size);
    *size = required;
    return 1;
}

int32_t OsGetComputerName(char* buffer, uint32_t* size) {
    char host[256] = {};
    if (gethostname(host, sizeof(host)) != 0) {
        SStrCopy(host, "Mac", sizeof(host));
    }
    return CopySized(buffer, size, host);
}

int32_t OsGetUserName(char* buffer, uint32_t* size) {
    const char* user = getenv("USER");
    if (!user || !*user) {
        user = getlogin();
    }
    return CopySized(buffer, size, user && *user ? user : "user");
}

uint64_t OsGetPhysicalMemory() {
    uint64_t mem = 0;
    size_t len = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &len, nullptr, 0);
    return mem;
}

void* CreateNamedEventA(const char* name, int32_t manualReset) {
    (void)name;
    (void)manualReset;
    return calloc(1, 1);
}

void OsFindTextSectionBounds() {
}

void OsValidateFunctionPointer(const void* ptr) {
    (void)ptr;
}

int32_t IOsParseProcessorFrequency(const char* brand) {
    (void)brand;
    uint64_t hz = 0;
    size_t len = sizeof(hz);
    if (sysctlbyname("hw.cpufrequency", &hz, &len, nullptr, 0) == 0 && hz) {
        s_cpuTicksPerSecond = hz;
        return 1;
    }
    return 0;
}

int32_t IOsGetPowerProfFrequency() {
    return IOsParseProcessorFrequency(nullptr);
}

int32_t OsIsRemoteSession() {
    return 0;
}

int32_t OsLaunchURL(const char* url) {
    if (!url || !*url || strlen(url) >= 4096) {
        errno = EINVAL;
        return 0;
    }

    std::string decoded;
    decoded.reserve(strlen(url));
    for (const char* p = url; *p; ++p) {
        if (*p == '%' && p[1] && p[2]) {
            char hex[3] = {p[1], p[2], 0};
            char* end = nullptr;
            long value = strtol(hex, &end, 16);
            if (end && *end == '\0') {
                decoded.push_back(static_cast<char>(value));
                p += 2;
                continue;
            }
        }
        decoded.push_back(*p);
    }

    pid_t pid = 0;
    char* argv[] = {const_cast<char*>("/usr/bin/open"), const_cast<char*>(decoded.c_str()), nullptr};
    int result = posix_spawn(&pid, "/usr/bin/open", nullptr, nullptr, argv, environ);
    return result == 0;
}

uint32_t OsGetProcessorFeaturesEx(int32_t& vendor) {
    vendor = 4;
#if defined(__aarch64__)
    return 0x80000000u | 0x10u;
#else
    return 0x80000000u;
#endif
}

uint64_t OsGetProcessorTicksPerSecond() {
    if (!s_cpuTicksPerSecond) {
        uint64_t hz = 0;
        size_t len = sizeof(hz);
        if (sysctlbyname("hw.cpufrequency", &hz, &len, nullptr, 0) == 0 && hz) {
            s_cpuTicksPerSecond = hz;
        } else {
            s_cpuTicksPerSecond = static_cast<uint64_t>(OsGetAsyncClocksPerSecond());
        }
    }
    return s_cpuTicksPerSecond;
}

#endif
