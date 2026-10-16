#ifndef WOW_SOURCE_UI_TOTEMINFO_H
#define WOW_SOURCE_UI_TOTEMINFO_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

class TotemInfo {
    public:
    /* 0000 */ int32_t m_slot;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ WOWGUID m_guid;
    /* 0010 */ const char* m_name;
    /* 0014 */ int32_t m_duration;
    /* 0018 */ uint32_t m_startTime;
    /* 001C */ const char* m_icon;

    uint32_t GetSpawnTimeLeft();
};

#endif
