#ifndef WOW_SOURCE_GAME_SHIPPATH_H
#define WOW_SOURCE_GAME_SHIPPATH_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Spline_CatmullRom.h"

class C3Vector;
class TransportPhysicsRec;

struct ShipPathSegment {
    /* 0000 */ uint32_t m_time;
    /* 0004 */ float m_distance;
    /* 0008 */ uint32_t m_delay;
};

struct ShipPathTimeEvent {
    /* 0000 */ uint32_t m_time;
    /* 0004 */ int32_t m_eventId;
};

struct ShipPathLeg {
    /* 0000 */ int32_t m_mapId;
    /* 0004 */ C3Spline_CatmullRom m_spline;
    /* 01C8 */ uint32_t m_startTime;
    /* 01CC */ uint32_t m_duration;
    /* 01D0 */ uint32_t m_endTime;
    /* 01D4 */ TSGrowableArray<ShipPathSegment> m_segments;
};

struct ShipPathPause {
    /* 0000 */ uint32_t m_point;
    /* 0004 */ uint32_t m_delay;
};

struct ShipPathEvent {
    /* 0000 */ uint32_t m_point;
    /* 0004 */ int32_t m_arrivalEvent;
    /* 0008 */ int32_t m_departureEvent;
};

template <class T>
class ShipPathArray {
    public:
    /* 0000 */ uint32_t m_count;
    /* 0004 */ T* m_data;

    void SetCount(uint32_t count);
};

class ShipPath {
    public:
    /* 0000 */ const TransportPhysicsRec* m_physics;
    /* 0004 */ uint32_t m_taxiPathId;
    /* 0008 */ TSGrowableArray<ShipPathLeg> m_legs;
    /* 0018 */ TSGrowableArray<ShipPathTimeEvent> m_events;
    /* 0028 */ float m_moveSpeed;
    /* 002C */ uint32_t m_totalPathTime;
    /* 0030 */ float m_accelRate;
    /* 0034 */ uint32_t m_waitTime;
    /* 0038 */ uint32_t m_timeOffset;
    /* 003C */ uint8_t m_waiting;
    /* 003D */ uint8_t m_waitReached;
    /* 003E */ uint8_t unused_003E;

    ShipPath();
    void SetTimeOffsetToPoint(uint32_t time, uint32_t point);
    void FindLegAndSegment(uint32_t pathTime, uint32_t* leg, uint32_t* segment);
    void WaitAtNextPausePoint(uint32_t time, bool pause);
    void Init(uint32_t taxiPathId, float moveSpeed, float accelRate, const TransportPhysicsRec* physics);
    void SetTotalPathTime(uint32_t time);
    void BuildLegs();
    uint32_t LegTimeAccel(float distance);
    void BuildLeg(ShipPathArray<C3Vector>* points, ShipPathArray<ShipPathPause>* pauses, ShipPathArray<ShipPathEvent>* events, int32_t leg);
    void SetTimeOffset(uint32_t time, float progress);
    void WaitAtCurrentLocation(uint32_t time);
    bool TimeInRange(uint32_t start, uint32_t end, uint32_t time);
    uint32_t GetPathTime(uint32_t time) const;
    uint32_t UpdatePathTime(uint32_t time, uint32_t elapsed, int32_t absolute);
    float GetDecelDistance(float t, float segTime, uint32_t* sequence, float* speed);
    float GetAccelDecelDistance(float t, float segTime, uint32_t* sequence, float* speed);
    void GetRollAndPitch(uint32_t time, float speed, float turn, C3Vector* position, float* roll, float* pitch);
    void ComputeRollAndPitch(float speed, float facing, float distance, uint32_t leg, float turn, uint32_t time, C3Vector* position, float* roll, float* pitch);
    void GetPosition(uint32_t time, uint32_t elapsed, int32_t* mapId, uint32_t* sequence, C3Vector* position, float* facing, uint32_t* leg, float* roll, float* pitch, int32_t absolute);
};

#endif
