#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/Types.h"

// VERIFY: 18cf53cd auto
// OFFSET: 0x004F4A50
void CMoveSpline::Skip(CDataStore* msg) {
    uint32_t flags = 0;
    msg->Get(flags);

    uint32_t faceSize = 0;

    if (flags & SPLINEFLAG_FINAL_ANGLE) {
        faceSize = 4;
    } else if (flags & SPLINEFLAG_FINAL_TARGET) {
        faceSize = 8;
    } else if (flags & SPLINEFLAG_FINAL_POINT) {
        faceSize = 12;
    }

    void* data;
    msg->GetDataInSitu(data, faceSize + 28);

    uint32_t count = 0;
    msg->Get(count);
    msg->GetDataInSitu(data, 12 * count + 13);
}

// VERIFY: 7eee1825 manual
// OFFSET: 0x004D4EA0
CMoveSpline::CMoveSpline() {
    this->m_finalDestination.x = 0.0f;
    this->m_finalDestination.y = 0.0f;
    this->m_finalDestination.z = 0.0f;
}

// VERIFY: daf8f9b2 auto
// OFFSET: 0x004D4F00
CMoveSpline::~CMoveSpline() {
}

// VERIFY: b4bbdade auto
// OFFSET: 0x00401190
CDataStore& operator>>(CDataStore& msg, C3Vector& vector) {
    msg.Get(vector.x);
    msg.Get(vector.y);
    msg.Get(vector.z);
    return msg;
}

// VERIFY: f14e9e22 manual
// OFFSET: 0x004F4B50
CDataStore& operator>>(CDataStore& msg, CMoveSpline& spline) {
    msg.Get(spline.m_flags);

    if (spline.m_flags & SPLINEFLAG_FINAL_ANGLE) {
        msg.Get(spline.m_face.m_angle);
    } else if (spline.m_flags & SPLINEFLAG_FINAL_TARGET) {
        msg.Get(*reinterpret_cast<uint64_t*>(&spline.m_face.m_target));
    } else if (spline.m_flags & SPLINEFLAG_FINAL_POINT) {
        msg >> *reinterpret_cast<C3Vector*>(spline.m_face.m_point);
    }

    uint32_t now = OsGetAsyncTimeMs();
    uint32_t timePassed;
    msg.Get(timePassed);
    spline.m_timePassed = now - timePassed;

    msg.Get(spline.m_duration);
    msg.Get(spline.m_id);
    msg.Get(spline.m_durationMod);
    msg.Get(spline.m_durationModNext);
    msg.Get(spline.m_verticalAcceleration);
    msg.Get(spline.m_effectStartTime);
    msg >> spline.m_spline;
    msg.Get(spline.m_finalDestination.x);
    msg.Get(spline.m_finalDestination.y);
    msg.Get(spline.m_finalDestination.z);
    return msg;
}

// VERIFY: d9fccc9a manual
// OFFSET: 0x006F1240
CMoveSpline& CMoveSpline::CopyFrom(const CMoveSpline& source) {
    this->unused_0008 = source.unused_0008;
    this->m_face = source.m_face;
    this->unused_001C = source.unused_001C;
    this->m_flags = source.m_flags;
    this->m_timePassed = source.m_timePassed;
    this->m_start = source.m_start;
    this->m_duration = source.m_duration;
    this->m_id = source.m_id;
    this->m_spline = source.m_spline;
    this->m_finalDestination = source.m_finalDestination;
    this->m_durationMod = source.m_durationMod;
    this->m_durationModNext = source.m_durationModNext;
    this->m_verticalAcceleration = source.m_verticalAcceleration;
    this->m_effectStartTime = source.m_effectStartTime;
    return *this;
}
