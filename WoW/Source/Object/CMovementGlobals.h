#ifndef WOW_SOURCE_OBJECT_CMOVEMENTGLOBALS_H
#define WOW_SOURCE_OBJECT_CMOVEMENTGLOBALS_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "Engine/Source/Storm/H/stpl.h"

class CMovement_C;

class CMovementGlobals {
    public:
    /* 0000 */ char m_logFileName [260];
    /* 0104 */ FILE* m_movementLog;
    /* 0108 */ FILE* m_fallingLog;
    /* 010C */ uint32_t unused_010C;
    /* 0110 */ uint32_t unused_0110;
    /* 0114 */ uint32_t unused_0114;
    /* 0118 */ TSList<CMovement_C, TSGetExplicitLink<CMovement_C> > m_movers;
    /* 0124 */ uint32_t m_flags;
    /* 0128 */ uint32_t m_lastUpdateTime;
    /* 012C */ uint32_t m_idleTime;
    /* 0130 */ uint32_t m_transportTime;
    /* 0134 */ uint32_t m_hasTransportTime2;
    /* 0138 */ uint32_t m_transportTime2;
    /* 013C */ uint32_t m_timeCounterMaybe;

    CMovementGlobals();
    ~CMovementGlobals();
};

#endif
