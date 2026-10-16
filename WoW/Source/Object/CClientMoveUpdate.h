#ifndef WOW_SOURCE_OBJECT_CCLIENTMOVEUPDATE_H
#define WOW_SOURCE_OBJECT_CCLIENTMOVEUPDATE_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/CMovementStatus.h"
#include "WoW/Source/Object/CMoveSpline.h"

class CClientMoveUpdate {
    public:
    /* 0000 */ CMovementStatus m_status;
    /* 0060 */ float m_walkSpeed;
    /* 0064 */ float m_runSpeed;
    /* 0068 */ float m_runBackSpeed;
    /* 006C */ float m_swimSpeed;
    /* 0070 */ float m_swimBackSpeed;
    /* 0074 */ float m_flightSpeed;
    /* 0078 */ float m_flightBackSpeed;
    /* 007C */ float m_turnRate;
    /* 0080 */ float m_pitchRate;
    /* 0084 */ uint32_t unused_0084;
    /* 0088 */ CMoveSpline m_moveSpline;

    static void Skip(CDataStore* msg);
};

CDataStore& operator>>(CDataStore& msg, CClientMoveUpdate& update);

#endif
