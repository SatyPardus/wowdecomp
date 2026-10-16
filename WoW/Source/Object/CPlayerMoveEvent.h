#ifndef WOW_SOURCE_OBJECT_CPLAYERMOVEEVENT_H
#define WOW_SOURCE_OBJECT_CPLAYERMOVEEVENT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/Object/WGUID.h"

class CMovementStatus;

class CPlayerMoveEvent {
    public:
    /* 0000 */ TSLink<CPlayerMoveEvent> m_link;
    /* 0008 */ uint32_t m_eventTime;
    /* 000C */ MoveEventId m_eventId;
    /* 0010 */ C3Vector m_position;
    /* 001C */ float m_facing;
    /* 0020 */ float m_pitch;
    /* 0024 */ uint32_t unused_0024;
    /* 0028 */ WOWGUID m_transportGuid;
    /* 0030 */ uint32_t m_moveFlags;
    /* 0034 */ uint16_t m_moveExtraFlags;
    /* 0036 */ uint16_t unused_0036;
    /* 0038 */ uint32_t m_fallTime;
    /* 003C */ float m_value;
    /* 0040 */ float m_launchDirY;
    /* 0044 */ float m_launchSpeed;
    /* 0048 */ float m_launchVerticalSpeed;
    /* 004C */ uint32_t m_ackCounter;
    /* 0050 */ uint8_t unk_0050;
    /* 0051 */ uint8_t m_needAck;
    /* 0052 */ uint8_t m_seat;
    /* 0053 */ uint8_t unused_0053;
    /* 0054 */ uint32_t unused_0054;

    void FromMoveStatus(const CMovementStatus& status);
};

#endif
