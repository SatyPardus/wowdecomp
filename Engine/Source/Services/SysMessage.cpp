#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00AC3208
static const SYSMSGSEVERITY s_severities[SYSMSG_NUMTYPES] = { { 0xFF, 0xFF, 0xFF, 0, 10.0f }, { 0xFF, 0xFF, 0x7F, 0, 15.0f }, { 0xFF, 0x7F, 0x7F, 0, 15.0f }, { 0x7F, 0xFF, 0xFF, 0, 20.0f } };

// STATIC: 0x00AC3228
static int32_t s_enabled = 1;

// STATIC: 0x00AC322C
static int32_t s_maxSeverity = SYSMSG_FATAL;

// STATIC: 0x00AC3230
static uint32_t s_categoryFilter = 0xFFFFFFFF;

// STATIC: 0x00B49C74
static int32_t s_minSeverity;

// VERIFY: 847a42a2 auto
// OFFSET: 0x004B4EE0
void SysMsgGetSeverityColor(SYSMSG_TYPE severity, float& red, float& green, float& blue) {
    red = s_severities[severity].m_red * 0.0039215689f;
    green = s_severities[severity].m_green * 0.0039215689f;
    blue = s_severities[severity].m_blue * 0.0039215689f;
}

// VERIFY: e98afe5a manual
// OFFSET: 0x004B4F60
int32_t SysMsgAdd(const char* text, SYSMSG_TYPE severity) {
    if (!text || severity >= SYSMSG_NUMTYPES) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return 1;
}

// VERIFY: 852e1d4d manual
// OFFSET: 0x004B4F90
int32_t SysMsgAdd(CStatus* status) {
    if (status->IsEmpty()) {
        return 1;
    }

    char* text = status->GetErrorStrAlloc(STATUS_INFO, nullptr);
    int32_t result = SysMsgAdd(text, status->m_highestSeverity);
    SMemFree(text, __FILE__, __LINE__, 0);

    return result;
}

// VERIFY: 9315395b manual
// OFFSET: 0x004B4FF0
int32_t SysMsgVPrintf(SYSMSG_TYPE severity, uint32_t flags, const char* format, va_list args) {
    char buffer[256];
    _vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[255] = '\0';

    if (severity < SYSMSG_NUMTYPES) {
        return 1;
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);

    return 0;
}

// VERIFY: 82062d21 manual
// OFFSET: 0x004B5040
int32_t SysMsgPrintf(SYSMSG_TYPE severity, uint32_t flags, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (format) {
        char buffer[256];
        _vsnprintf(buffer, sizeof(buffer), format, args);
        buffer[255] = '\0';

        if (severity < SYSMSG_NUMTYPES) {
            return 1;
        }
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);

    return 0;
}

// VERIFY: fe511d8e auto
// OFFSET: 0x004D8520
int32_t SysMsgPrintf(SYSMSG_TYPE severity, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int32_t result = SysMsgVPrintf(severity, 1, format, args);
    va_end(args);

    return result;
}

// VERIFY: 75a92c2f auto
// OFFSET: 0x004B4E50
void SysMsgEnable(int32_t enable) {
    s_enabled = enable;
}

// VERIFY: 134b1bf4 auto
// OFFSET: 0x004B4E60
void SysMsgSetMinDisplayLevel(int32_t level) {
    s_minSeverity = level;

    if (s_maxSeverity < level) {
        s_maxSeverity = level;
    }
}

// VERIFY: 64a6d646 auto
// OFFSET: 0x004B4E80
void SysMsgSetMaxDisplayLevel(int32_t level) {
    s_maxSeverity = level;

    if (s_minSeverity > level) {
        s_minSeverity = level;
    }
}

// VERIFY: fa16c054 auto
// OFFSET: 0x004B4EA0
int32_t SysMsgGetMinDisplayLevel() {
    return s_minSeverity;
}

// VERIFY: 7d5c6e01 auto
// OFFSET: 0x004B4EB0
int32_t SysMsgGetMaxDisplayLevel() {
    return s_maxSeverity;
}

// VERIFY: c7c0bced auto
// OFFSET: 0x004B4EC0
void SysMsgSetFilter(uint32_t filter) {
    s_categoryFilter = filter;
}

// VERIFY: d71710d4 auto
// OFFSET: 0x004B4ED0
uint32_t SysMsgGetFilter() {
    return s_categoryFilter;
}

// VERIFY: dceded4d auto
// OFFSET: 0x004B4F30
void SysMsgInitialize() {
    s_enabled = 1;
    s_minSeverity = SYSMSG_INFO;
    s_maxSeverity = SYSMSG_FATAL;
    s_categoryFilter = 0xFFFFFFFF;
}

// VERIFY: f92a5b55 manual
// OFFSET: 0x005EEB70
void SysMsgSetCallback(void (*callback)(const char* text, SYSMSG_TYPE severity)) {
}

// OFFSET: 0x004B5090
void SysMsgShutdown() {
    s_enabled = 0;
}
