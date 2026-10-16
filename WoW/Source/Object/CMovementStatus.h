#ifndef WOW_SOURCE_OBJECT_CMOVEMENTSTATUS_H
#define WOW_SOURCE_OBJECT_CMOVEMENTSTATUS_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Object/WGUID.h"

class CDataStore;

class CMovementStatus {
    public:
    /* 0000 */ uint32_t m_gameTime;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ WOWGUID m_transportGuid;
    /* 0010 */ uint32_t m_moveFlags;
    /* 0014 */ uint16_t m_moveExtraFlags;
    /* 0016 */ uint8_t m_transportSeat;
    /* 0017 */ uint8_t m_uint17;
    /* 0018 */ C3Vector m_transportPosition;
    /* 0024 */ float m_transportFacing;
    /* 0028 */ C3Vector m_position;
    /* 0034 */ float m_facing;
    /* 0038 */ float m_pitch;
    /* 003C */ uint32_t m_fallTime;
    /* 0040 */ float m_zSpeed;
    /* 0044 */ float m_sinAngle;
    /* 0048 */ float m_cosAngle;
    /* 004C */ float m_xySpeed;
    /* 0050 */ float m_splineElevation;
    /* 0054 */ uint32_t m_transportTime;
    /* 0058 */ uint32_t m_transportTime2;
    /* 005C */ float unused_005C;

    static uint32_t Skip(CDataStore* msg);

    CMovementStatus();
};

CDataStore& operator>>(CDataStore& msg, CMovementStatus& status);
CDataStore& operator<<(CDataStore& msg, const CMovementStatus& status);

#endif
