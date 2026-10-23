#ifndef ENGINE_SOURCE_EVENT_CEVENT_H
#define ENGINE_SOURCE_EVENT_CEVENT_H

#include <cstdint>
#include "Engine/Source/Base/RefCount.h"

enum : uint32_t {
    EVENT_WINDOW_SIZE = 0x40040064,
    EVENT_WINDOW_FOCUS = 0x40040065,
};

class CEvent : public TRefCnt {
    public:
    /* 0008 */ uint32_t m_id;
    /* 000C */ void* m_param;

    CEvent();
};

#endif
