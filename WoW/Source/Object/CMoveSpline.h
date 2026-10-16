#ifndef WOW_SOURCE_OBJECT_CMOVESPLINE_H
#define WOW_SOURCE_OBJECT_CMOVESPLINE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Spline_CatmullRom.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Object/WGUID.h"

class CDataStore;

#pragma pack(push, 4)
union CMoveSplineFaceData {
    /* 0000 */ float m_point[3];
    /* 0000 */ WOWGUID m_target;
    /* 0000 */ float m_angle;
};
#pragma pack(pop)

class CMoveSpline {
    public:
    /* 0000 */ TSLink<void> unused_0000;
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ CMoveSplineFaceData m_face;
    /* 001C */ uint32_t unused_001C;
    /* 0020 */ uint32_t m_flags;
    /* 0024 */ uint32_t m_timePassed;
    /* 0028 */ uint32_t m_start;
    /* 002C */ uint32_t m_duration;
    /* 0030 */ uint32_t m_id;
    /* 0034 */ C3Spline_CatmullRom m_spline;
    /* 01F8 */ C3Vector m_finalDestination;
    /* 0204 */ float m_durationMod;
    /* 0208 */ float m_durationModNext;
    /* 020C */ float m_verticalAcceleration;
    /* 0210 */ uint32_t m_effectStartTime;
    /* 0214 */ TSLink<void> unused_0214;
    /* 021C */ uint32_t unused_021C;

    static void Skip(CDataStore* msg);

    CMoveSpline();
    ~CMoveSpline();
    CMoveSpline& CopyFrom(const CMoveSpline& source);
};

CDataStore& operator>>(CDataStore& msg, C3Vector& vector);
CDataStore& operator>>(CDataStore& msg, CMoveSpline& spline);

#endif
