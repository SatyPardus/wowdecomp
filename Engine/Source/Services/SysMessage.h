#ifndef ENGINE_SOURCE_SERVICES_SYSMESSAGE_H
#define ENGINE_SOURCE_SERVICES_SYSMESSAGE_H

#include <cstdarg>
#include <cstdint>
#include "Engine/Source/Base/Status.h"

struct SYSMSGSEVERITY {
    /* 0000 */ uint8_t m_red;
    /* 0001 */ uint8_t m_green;
    /* 0002 */ uint8_t m_blue;
    /* 0003 */ uint8_t unused_0003;
    /* 0004 */ float m_duration;
};

int32_t SysMsgAdd(const char* text, STATUS_TYPE severity);
int32_t SysMsgAdd(CStatus* status);
int32_t SysMsgPrintf(STATUS_TYPE severity, const char* format, ...);
int32_t SysMsgPrintf(STATUS_TYPE severity, uint32_t flags, const char* format, ...);
int32_t SysMsgVPrintf(STATUS_TYPE severity, uint32_t flags, const char* format, va_list args);

void SysMsgGetSeverityColor(STATUS_TYPE severity, float& red, float& green, float& blue);
void SysMsgInitialize();
void SysMsgShutdown();
void SysMsgSetCallback(void (*callback)(const char* text, STATUS_TYPE severity));

void SysMsgEnable(int32_t enable);
uint32_t SysMsgGetFilter();
int32_t SysMsgGetMaxDisplayLevel();
int32_t SysMsgGetMinDisplayLevel();
void SysMsgSetFilter(uint32_t filter);
void SysMsgSetMaxDisplayLevel(int32_t level);
void SysMsgSetMinDisplayLevel(int32_t level);

#endif
