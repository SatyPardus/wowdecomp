#ifndef ENGINE_SOURCE_EVENT_CKEYEVENT_H
#define ENGINE_SOURCE_EVENT_CKEYEVENT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Event/CEvent.h"
#include "Engine/Source/Event/Types.h"

// Reference IDA: anonymous keyboard event IDs (0.5.3 ordinal 44).
enum : uint32_t {
    EVENT_KEY_DOWN = 0x40060064,
    EVENT_KEY_REPEAT = 0x40060065,
    EVENT_KEY_UP = 0x40060066,
    EVENT_CHAR = 0x40060067,
    EVENT_IME = 0x40060068,
};

class CKeyEvent : public CEvent {
    public:
    /* 0010 */ EVENT_DATA_KEY m_data;
};

#endif
