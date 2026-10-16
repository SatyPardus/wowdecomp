#include "Movement.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <new>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CFacet.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/CClientMoveUpdate.h"
#include "WoW/Source/Object/CMovementGlobals.h"
#include "WoW/Source/Object/CMovementStatus.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00ADA370
TSList<CPlayerMoveEvent, TSGetExplicitLink<CPlayerMoveEvent> > CPlayerMoveQueue::s_freeList;

// STATIC: 0x00A37F3C
static const uint8_t s_volumeEdges[32] = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 3, 3, 4, 4, 1, 1, 5, 2, 6, 3, 7, 4, 8, 5, 6, 6, 7, 7, 8, 8, 5 };

// STATIC: 0x00ADBA34
static World::FacetData s_moveFacets;

// STATIC: 0x00C9ECC4
static CVar* s_cvSplineOpt;

// STATIC: 0x00ADBA54
static World::FacetData s_liquidFacets;

// STATIC: 0x00CA1660
static CAaBox s_queryBox;

// STATIC: 0x00C9ECC8
static int32_t s_flushingQueue;

// VERIFY: 9482b603 manual
// OFFSET: 0x006EBC70
CPlayerMoveEvent* CPlayerMoveQueue::AllocEvent(uint32_t eventTime, MoveEventId eventId) {
    CPlayerMoveEvent* event = s_freeList.Head();

    if (event) {
        event->m_link.Unlink();
    } else {
        event = s_freeList.NewNode(0, 0, 0);
    }

    event->m_eventTime = eventTime;
    event->m_eventId = eventId;
    event->unk_0050 = 0;
    return event;
}

// VERIFY: 08a36d29 manual
// OFFSET: 0x006EC090
void CPlayerMoveQueue::Enqueue(CPlayerMoveEvent* event) {
    for (CPlayerMoveEvent* node = this->Head(); node; node = node->m_link.Next()) {
        if ((int32_t)(event->m_eventTime - node->m_eventTime) < 0) {
            this->LinkNode(event, 2, node);
            return;
        }
    }

    this->LinkToTail(event);
}

// VERIFY: 9b07340c manual
// OFFSET: 0x006EC1F0
void CPlayerMoveQueue::FreeAll() {
    if (MovementGetGlobals()) {
        s_freeList.Combine(this, 2, nullptr);
    } else {
        this->Clear();
    }
}

// INLINED
CMovementGlobals::CMovementGlobals() {
    this->m_movementLog = nullptr;
    this->m_fallingLog = nullptr;
    this->unused_0110 = 0;
    this->unused_0114 = 0;
    this->m_movers.SetLinkOffset(offsetof(CMovementShared, m_globalUnitsLink));
    this->m_flags &= ~0x1;
    this->m_hasTransportTime2 = 0;
}

// VERIFY: 67bd1f8c manual
// OFFSET: 0x006EC2C0
void MovementInitialize(const char* logFile) {
    MovementSetGlobals(new (SMemAlloc(sizeof(CMovementGlobals), __FILE__, __LINE__, 0)) CMovementGlobals());

    CMovementGlobals* globals = MovementGetGlobals();
    SStrCopy(globals->m_logFileName, logFile, sizeof(globals->m_logFileName));

    uint32_t now = OsGetAsyncTimeMs();
    globals->m_flags |= 0x1;
    globals->m_idleTime = now;
    globals->m_lastUpdateTime = now;

    s_cvSplineOpt = CVar::Register("SplineOpt", "toggles use of spline coll optimization", 0x1, "1", nullptr, 0, false, nullptr, false);
}

// VERIFY: 3179da62 manual
// OFFSET: 0x006F0C30
void MovementDestroy() {
    CMovementGlobals* globals = MovementGetGlobals();

    if (!globals) {
        return;
    }

    CMovementShared::StopAllLogging();
    delete globals;
    MovementSetGlobals(nullptr);
    CPlayerMoveQueue::s_freeList.Clear();
}

// VERIFY: ee0cfb58 manual
// OFFSET: 0x006EB4E0
void CPlayerMoveQueue::FreeAllExceptTimeSync() {
    CPlayerMoveEvent* event = this->Head();

    while (event) {
        CPlayerMoveEvent* next = event->m_link.Next();

        if (event->m_eventId != MOVEEVENT_TIME_SYNC) {
            event->m_link.Unlink();
            s_freeList.LinkToTail(event);
        }

        event = next;
    }
}

// VERIFY: e65bbdf5 manual
// OFFSET: 0x006EAC00
int32_t CPlayerMoveQueue::HasEventBetween(MoveEventId first, MoveEventId last) {
    for (CPlayerMoveEvent* event = this->Head(); event; event = event->m_link.Next()) {
        if (event->m_eventId >= first && event->m_eventId <= last) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 7f5da03e auto
// OFFSET: 0x006EB590
void CPlayerMoveQueue::UnlinkEventById(MoveEventId eventId) {
    for (CPlayerMoveEvent* event = this->Head(); event; event = event->m_link.Next()) {
        if (event->m_eventId == eventId) {
            event->m_link.Unlink();
            s_freeList.LinkToTail(event);
            return;
        }
    }
}

// VERIFY: 216c2ed6 manual
// OFFSET: 0x0075B610
ClipPolygon::ClipPolygon(const ClipPolygon& source) {
    this->m_count = source.m_count;
    memcpy(this->m_v, source.m_v, sizeof(C3Vector) * source.m_count);
    memcpy(this->m_flags, source.m_flags, sizeof(int32_t) * source.m_count);
}

// VERIFY: 6a25e158 auto
// OFFSET: 0x006E9290
void CMovement_C::WrapFacingToRange(float& facing, float min, float max) {
    while (min > facing) {
        facing = facing + 6.2831855f;
    }

    if (max > facing) {
        return;
    }

    float previous;
    float wrapped = facing;

    do {
        previous = wrapped;
        wrapped = previous - 6.2831855f;
    } while (wrapped > max);

    facing = wrapped;

    if (wrapped <= min && previous - max < min - wrapped) {
        facing = previous;
    }
}

// VERIFY: 29f6438b auto
// OFFSET: 0x006E97D0
int32_t CMovement_C::AddTimeSkewSample(uint32_t localTime, uint32_t serverTime) {
    int32_t skew = serverTime + this->m_timeSkew - localTime;

    if ((uint32_t)(skew + 0x8000) > 0xFFFF) {
        skew = ~((skew + 0x8000) >> 31) - 0x8000;
    }

    this->m_timeSkewSamples[this->m_timeSkewIndex] = skew;
    this->m_timeSkewIndex = (this->m_timeSkewIndex + 1) & 31;

    for (int32_t i = 0; i < 32; i++) {
        if (this->m_timeSkewSamples[i] > skew) {
            skew = this->m_timeSkewSamples[i];
        }
    }

    return skew;
}

// VERIFY: 366620da auto
// OFFSET: 0x0075CD00
void CMovement_C::BuildCollisionBox(const C3Vector& position, CAaBox& box) const {
    box.b = position;
    box.t = position;
    box.b.x = box.b.x - this->m_collisionRadius;
    box.b.y = box.b.y - this->m_collisionRadius;
    box.t.x = box.t.x + this->m_collisionRadius;
    box.t.y = box.t.y + this->m_collisionRadius;
    box.t.z = this->m_collisionHeight + box.t.z;
}

// VERIFY: 5ec7444f manual
// OFFSET: 0x0075C8F0
void CMovement_C::BuildCollisionVolumePlanes(const C3Vector& position, float height, float radius, C4Plane* planes) const {
    planes[0].n.x = -1.0f;
    planes[0].n.y = 0.0f;
    planes[0].n.z = 0.0f;
    planes[0].d = position.x - radius;

    planes[1].n.x = 1.0f;
    planes[1].n.y = 0.0f;
    planes[1].n.z = 0.0f;
    planes[1].d = -position.x - radius;

    planes[2].n.x = 0.0f;
    planes[2].n.y = 1.0f;
    planes[2].n.z = 0.0f;
    planes[2].d = -position.y - radius;

    planes[3].n.x = 0.0f;
    planes[3].n.y = -1.0f;
    planes[3].n.z = 0.0f;
    planes[3].d = position.y - radius;

    planes[4].n.x = 0.0f;
    planes[4].n.y = 0.0f;
    planes[4].n.z = 1.0f;
    planes[4].d = -position.z - height;

    planes[5].n = C3Vector(-0.87964189f, 0.0f, -0.4756366f);
    planes[5].d = position.z * 0.4756366f + position.x * 0.87964189f + position.y * 0.0f;

    planes[6].n = C3Vector(0.87964189f, 0.0f, -0.4756366f);
    planes[6].d = position.z * 0.4756366f - position.x * 0.87964189f + position.y * 0.0f;

    planes[7].n = C3Vector(0.0f, 0.87964189f, -0.4756366f);
    planes[7].d = position.z * 0.4756366f + position.x * 0.0f - position.y * 0.87964189f;

    planes[8].n = C3Vector(0.0f, -0.87964189f, -0.4756366f);
    planes[8].d = position.z * 0.4756366f + position.x * 0.0f + position.y * 0.87964189f;
}

// VERIFY: 5e433e2e manual
// OFFSET: 0x0075E250
C3Vector CMovement_C::CalcRunWalkBlockingObstaclePush(const C3Vector& move, float dt, float maxDistance, const C3Vector& normal) const {
    if (normal.z < 0.0f && -normal.z > 0.64278764f) {
        return C3Vector(0.0f, 0.0f, 0.0f);
    }

    C2Vector direction(normal);
    float lengthSq = direction.y * direction.y + direction.x * direction.x;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        direction.x = direction.x * scale;
        direction.y = direction.y * scale;
    }

    float push = (-normal.x * move.x + -normal.y * move.y + -normal.z * move.z) * dt;
    float denominator = normal.y * direction.y + normal.x * direction.x + normal.z * 0.0f;
    float distance = fabs(denominator) < 0.00000023841858f ? push : push / denominator;
    float resultX = direction.x * (distance + 0.001f);
    float resultY = direction.y * (distance + 0.001f);

    C2Vector move2D(move);
    float moveX = move2D.x * dt;
    float moveY = dt * move2D.y;
    float totalX = moveX + resultX;
    float totalY = moveY + resultY;
    float totalSq = totalY * totalY + totalX * totalX;

    if (maxDistance * maxDistance < totalSq) {
        float scale = maxDistance / sqrt(totalSq);
        resultX = totalX * scale - moveX;
        resultY = totalY * scale - moveY;
    }

    return C3Vector(resultX, resultY, 0.0f);
}

// VERIFY: 2f98d9fc manual
// OFFSET: 0x0075D740
int32_t CMovement_C::ComputeSteepSurfacePushNormalImpl(const C4Plane& plane) const {
    float top = this->m_collisionRadius * 1.849399f;
    C3Vector points[5];
    points[0] = this->m_position;
    points[1] = C3Vector(this->m_collisionRadius + this->m_position.x, this->m_collisionRadius + this->m_position.y, this->m_position.z + top);
    points[2] = C3Vector(this->m_position.x - this->m_collisionRadius, this->m_collisionRadius + this->m_position.y, this->m_position.z + top);
    points[3] = C3Vector(this->m_collisionRadius + this->m_position.x, -this->m_collisionRadius + this->m_position.y, this->m_position.z + top);
    points[4] = C3Vector(-this->m_collisionRadius + this->m_position.x, -this->m_collisionRadius + this->m_position.y, top + this->m_position.z);

    for (uint32_t i = 0; i < 5; i++) {
        if (fabs(fabs(points[i].x * plane.n.x + points[i].z * plane.n.z + points[i].y * plane.n.y + plane.d)) < 0.0013888889f) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 08eb7639 manual
// OFFSET: 0x0075D890
int32_t CMovement_C::PlaneIntersectsVolume(const C4Plane& plane) const {
    float top = this->m_collisionRadius * 1.849399f;
    C3Vector points[9];
    points[0] = this->m_position;
    points[1] = C3Vector(this->m_collisionRadius + this->m_position.x, this->m_collisionRadius + this->m_position.y, this->m_position.z + top);
    points[2] = C3Vector(-this->m_collisionRadius + this->m_position.x, this->m_collisionRadius + this->m_position.y, this->m_position.z + top);
    points[3] = C3Vector(this->m_collisionRadius + this->m_position.x, -this->m_collisionRadius + this->m_position.y, this->m_position.z + top);
    points[4] = C3Vector(-this->m_collisionRadius + this->m_position.x, -this->m_collisionRadius + this->m_position.y, top + this->m_position.z);
    points[5] = C3Vector(this->m_collisionRadius + this->m_position.x, this->m_collisionRadius + this->m_position.y, this->m_collisionHeight + this->m_position.z);
    points[6] = C3Vector(this->m_position.x - this->m_collisionRadius, this->m_collisionRadius + this->m_position.y, this->m_collisionHeight + this->m_position.z);
    points[7] = C3Vector(this->m_collisionRadius + this->m_position.x, -this->m_collisionRadius + this->m_position.y, this->m_collisionHeight + this->m_position.z);
    points[8] = C3Vector(-this->m_collisionRadius + this->m_position.x, -this->m_collisionRadius + this->m_position.y, this->m_collisionHeight + this->m_position.z);

    for (uint32_t i = 0; i < 32; i += 2) {
        const C3Vector& a = points[s_volumeEdges[i]];
        const C3Vector& b = points[s_volumeEdges[i + 1]];
        float distanceA = fabs(a.y * plane.n.y + a.z * plane.n.z + a.x * plane.n.x + plane.d);
        float distanceB = fabs(b.y * plane.n.y + b.z * plane.n.z + b.x * plane.n.x + plane.d);

        if (distanceA * distanceB < 0.0f) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 5e0b6fee manual
// OFFSET: 0x0075DB00
C3Vector CMovement_C::PushOffObstacleEdge(const C3Vector& point, const C3Vector& direction, uint32_t facet) const {
    const CFacet& data = s_moveFacets.facets.m_data[facet];

    C3Vector edge = data.v[1] - data.v[0];
    float lengthSq = edge.y * edge.y + edge.z * edge.z + edge.x * edge.x;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        edge.x = edge.x * scale;
        edge.y = edge.y * scale;
        edge.z = edge.z * scale;
    }

    float t = (point.z - data.v[0].z) * edge.z + (point.y - data.v[0].y) * edge.y + (point.x - data.v[0].x) * edge.x;
    float dz = point.z - (t * edge.z + data.v[0].z);
    float dy = point.y - (edge.y * t + data.v[0].y);
    float dx = point.x - (t * edge.x + data.v[0].x);
    float best = dx * dx + dy * dy + dz * dz;

    C3Vector edge1 = data.v[2] - data.v[1];
    lengthSq = edge1.z * edge1.z + edge1.y * edge1.y + edge1.x * edge1.x;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        edge1.x = edge1.x * scale;
        edge1.y = edge1.y * scale;
        edge1.z = scale * edge1.z;
    }

    t = (point.z - data.v[1].z) * edge1.z + (point.y - data.v[1].y) * edge1.y + (point.x - data.v[1].x) * edge1.x;
    dy = point.y - (edge1.y * t + data.v[1].y);
    dz = point.z - (edge1.z * t + data.v[1].z);
    dx = point.x - (edge1.x * t + data.v[1].x);
    float distance = dx * dx + (dz * dz + dy * dy);

    if (best > distance) {
        best = distance;
        edge = edge1;
    }

    C3Vector edge2 = data.v[0] - data.v[2];
    lengthSq = edge2.y * edge2.y + edge2.z * edge2.z + edge2.x * edge2.x;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        edge2.x = edge2.x * scale;
        edge2.y = edge2.y * scale;
        edge2.z = scale * edge2.z;
    }

    t = (point.z - data.v[2].z) * edge2.z + (point.y - data.v[2].y) * edge2.y + (point.x - data.v[2].x) * edge2.x;
    dx = point.x - (edge2.x * t + data.v[2].x);
    dy = point.y - (edge2.y * t + data.v[2].y);
    dz = point.z - (edge2.z * t + data.v[2].z);

    if (dx * dx + dz * dz + dy * dy < best) {
        edge = edge2;
    }

    C3Vector push(data.plane.n.z * edge.y - data.plane.n.y * edge.z, edge.z * data.plane.n.x - data.plane.n.z * edge.x, edge.x * data.plane.n.y - edge.y * data.plane.n.x);

    if (direction.z * push.z + direction.y * push.y + push.x * direction.x < 0.0f) {
        push = C3Vector(-push.x, -push.y, -push.z);
    }

    return push;
}

// VERIFY: ddb278fe manual
// OFFSET: 0x0075CFE0
int32_t CMovement_C::IsFacetOverhead(uint32_t facet) const {
    float left = this->m_position.x - this->m_collisionRadius;
    float right = this->m_collisionRadius + this->m_position.x;
    float front = this->m_position.y - this->m_collisionRadius;
    float back = this->m_collisionRadius + this->m_position.y;
    float top = this->m_collisionHeight + this->m_position.z;
    C3Vector corners[4] = { C3Vector(left, front, top), C3Vector(right, front, top), C3Vector(left, back, top), C3Vector(right, back, top) };
    const CFacet& data = s_moveFacets.facets.m_data[facet];

    for (uint32_t i = 0; i < 4; i++) {
        if (fabs(corners[i].z * data.plane.n.z + corners[i].x * data.plane.n.x + corners[i].y * data.plane.n.y + data.plane.d) < 0.0013888889f) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: c4ba2cd0 manual
// OFFSET: 0x0075D2D0
int32_t CMovement_C::IsFacetWall(uint32_t facet) const {
    const CFacet& data = s_moveFacets.facets.m_data[facet];

    if (data.plane.n.z > 0.64278764f) {
        return 0;
    }

    return data.plane.n.z >= 0.0f || -data.plane.n.z <= 0.64278764f;
}

// VERIFY: be7db241 manual
// OFFSET: 0x006EC3B0
void CMovement_C::FixupQueuedSetFacingEvents(float delta) {
    for (CPlayerMoveEvent* event = this->m_moveQueue.Head(); event; event = event->m_link.Next()) {
        if (event->m_eventId == MOVEEVENT_SET_FACING || event->unk_0050) {
            event->m_facing = CMath::normalizeangle0to2pi_(delta + event->m_facing);
        }
    }
}

// VERIFY: 7291cd32 manual
// OFFSET: 0x006E9520
float CMovement_C::GetRemainingStepUpHeight() const {
    float height = this->m_unit->IsClientControlled() ? this->m_stepUpHeight : 2.0f;

    if (this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
        height -= this->m_position.z - this->m_stepUpStartZ;
    }

    if (height <= 0.0f) {
        return 0.0f;
    }

    return height;
}

// VERIFY: fe524558 auto
// OFFSET: 0x006E8FC0
float CMovement_C::GetStepUpHeight() const {
    if (!this->m_unit->IsClientControlled()) {
        return 2.0f;
    }

    return this->m_stepUpHeight;
}

// VERIFY: 7f04ce16 manual
// OFFSET: 0x006E9F50
uint32_t CMovement_C::GetMoveStartTime(uint32_t elapsed) {
    if (this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
        return this->m_anchorElapsedMs;
    }

    if (!(this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING))) {
        return this->m_anchorElapsedMs;
    }

    this->m_moveStartDelay += elapsed;
    uint32_t delay = this->m_moveStartDelay;

    if (delay <= 500) {
        return this->m_anchorElapsedMs;
    }

    int32_t lag = delay - 1500;

    if (lag < 0) {
        lag = 0;
    }

    int32_t ramp = delay - 500;

    if (ramp > 1000) {
        ramp = 1000;
    }

    int32_t time = this->m_anchorElapsedMs - lag;

    if (time < 0) {
        time = 0;
    }

    int32_t result = time - ramp * ramp / 2000;

    if (result < 0) {
        return 0;
    }

    return result;
}

// VERIFY: 27d22089 manual
// OFFSET: 0x006EA7E0
int32_t CMovement_C::Interpolate(uint32_t now, uint32_t elapsed, C3Vector* position, float* facing, float* pitch) {
    CPlayerMoveEvent* next = this->m_moveQueue.Head();
    int32_t delta;

    if (!next || (delta = next->m_eventTime - now) < 0) {
        this->m_flags2 &= ~(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT | MOVEMENTFLAG2_INTERPOLATED_TURNING | MOVEMENTFLAG2_INTERPOLATED_PITCHING);
        return 0;
    }

    float t = (float)delta / (float)this->m_interpolation;
    float inverse = 1.0f - t;

    if (this->m_flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT) {
        C3Vector interpolated;
        interpolated.x = (next->m_position.x - this->m_interpolationPos.x * t) * inverse + position->x * t;
        interpolated.y = (next->m_position.y - this->m_interpolationPos.y * t) * inverse + position->y * t;
        interpolated.z = (next->m_position.z - this->m_interpolationPos.z * t) * inverse + position->z * t;

        float dx = interpolated.x - this->m_position.x;
        float dy = interpolated.y - this->m_position.y;
        float maxDistance = elapsed * 0.001f * 60.0f;

        if (dx * dx + dy * dy + 0.00000095367432f <= maxDistance * maxDistance) {
            *position = interpolated;
        } else {
            this->m_flags2 &= ~MOVEMENTFLAG2_INTERPOLATED_MOVEMENT;
        }
    }

    if (this->m_flags2 & MOVEMENTFLAG2_INTERPOLATED_TURNING) {
        float target = CMath::normalizeangle0to2pi_(next->m_facing - t * this->m_interpolationFacing);
        *facing = CMath::normalizeangle0to2pi_(CMath::normalizeAngleNegPiToPi_(target - *facing) * inverse + *facing);
    }

    if (this->m_flags2 & MOVEMENTFLAG2_INTERPOLATED_PITCHING) {
        float target = CMath::normalizeangle0to2pi_(next->m_pitch - t * this->m_interpolationPitch);
        *pitch = CMath::normalizeangle0to2pi_(CMath::normalizeAngleNegPiToPi_(target - *pitch) * inverse + *pitch);
    }

    return this->m_flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT;
}

// VERIFY: 14204d2d auto
// OFFSET: 0x005FEDE0
int32_t CMovement_C::IsJumping() const {
    return (this->m_flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_fallVelocity;
}

// VERIFY: 47ecbc25 auto
// OFFSET: 0x006E9440
int32_t CMovement_C::IsValidPosition() const {
    return World::IsValidPosition(this->m_position.x, this->m_position.y, this->m_position.z, 2.0f) != 0;
}

// VERIFY: 28844f7c auto
// OFFSET: 0x006E90D0
void CMovement_C::OnSetRunMode(int32_t run) {
    this->SetRunMode(run);
}

// VERIFY: f0dec4a3 auto
// OFFSET: 0x006E9280
void CMovement_C::OnSkipTime(uint32_t time) {
    this->m_gameTime += time;
}

// VERIFY: 15cccdda auto
// OFFSET: 0x006E9240
void CMovement_C::OnWaterWalk(int32_t enable) {
    this->WalkOnWater(enable);
}

// VERIFY: 061b2da3 auto
// OFFSET: 0x0075B4F0
void CMovement_C::RestoreMoveState(const MoveState& state) {
    this->m_anchorPos = state.m_anchorPos;
    this->m_anchorFacing = state.m_anchorFacing;
    this->m_anchorPitch = state.m_anchorPitch;
    this->m_anchorElapsedMs = state.m_anchorElapsedMs;
    this->m_fallTimeMs = state.m_fallTimeMs;
    this->m_moveDir = state.m_moveDir;
    this->m_moveDir2D = state.m_moveDir2D;
    this->m_flags = state.m_flags;
    this->m_currentSpeed = state.m_currentSpeed;
}

// VERIFY: 2e29bfd8 auto
// OFFSET: 0x0075B480
void CMovement_C::SaveMoveState(MoveState& state) const {
    state.m_anchorPos = this->m_anchorPos;
    state.m_anchorFacing = this->m_anchorFacing;
    state.m_anchorPitch = this->m_anchorPitch;
    state.m_anchorElapsedMs = this->m_anchorElapsedMs;
    state.m_fallTimeMs = this->m_fallTimeMs;
    state.m_moveDir = this->m_moveDir;
    state.m_moveDir2D = this->m_moveDir2D;
    state.m_flags = this->m_flags;
    state.m_currentSpeed = this->m_currentSpeed;
}

// VERIFY: 20aefe0e manual
// OFFSET: 0x006E9BB0
void CMovement_C::SendTimeSyncResp(uint32_t clientTime, uint32_t counter) {
    CDataStore msg;
    msg.Put((uint32_t)CMSG_TIME_SYNC_RESPONSE);
    msg.Put(counter);
    msg.Put(clientTime);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: dea84484 manual
// OFFSET: 0x006EA6A0
void CMovement_C::SetInterpolation(uint32_t time) {
    this->m_flags2 &= ~(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT | MOVEMENTFLAG2_INTERPOLATED_TURNING | MOVEMENTFLAG2_INTERPOLATED_PITCHING);
    this->m_interpolation = 0;

    CPlayerMoveEvent* next = this->m_moveQueue.Head();

    if (!next || !next->unk_0050 || next->m_eventId == MOVEEVENT_TELEPORT) {
        return;
    }

    if (this->m_transportGuid != next->m_transportGuid) {
        return;
    }

    uint32_t interpolation = next->m_eventTime - time;

    if (!interpolation) {
        return;
    }

    if (this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) {
        this->m_flags2 |= MOVEMENTFLAG2_INTERPOLATED_MOVEMENT;
    }

    if (CMath::fnotequal(this->m_facing, next->m_facing)) {
        this->m_flags2 |= MOVEMENTFLAG2_INTERPOLATED_TURNING;
    }

    if ((this->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) && CMath::fnotequal(this->m_pitch, next->m_pitch)) {
        this->m_flags2 |= MOVEMENTFLAG2_INTERPOLATED_PITCHING;
    }

    this->m_interpolationPos = next->m_position - this->m_position;
    this->m_interpolationFacing = CMath::normalizeAngleNegPiToPi_(next->m_facing - this->m_facing);
    this->m_interpolationPitch = CMath::normalizeAngleNegPiToPi_(next->m_pitch - this->m_pitch);
    this->m_interpolation = interpolation;
}

// VERIFY: 0eac7dea auto
// OFFSET: 0x006E96A0
void CMovement_C::SetSplineFaceAngle(float facing) {
    this->m_spline->m_flags |= SPLINEFLAG_FINAL_ANGLE;
    this->m_spline->m_face.m_angle = facing;
}

// VERIFY: 8b3481cb manual
// OFFSET: 0x006E9640
void CMovement_C::SetSplineFacePoint(const C3Vector& point) {
    this->m_spline->m_flags |= SPLINEFLAG_FINAL_POINT;
    CMoveSplineFaceData& face = this->m_spline->m_face;
    face.m_point[0] = point.x;
    face.m_point[1] = point.y;
    face.m_point[2] = point.z;
}

// VERIFY: 42b61ef1 auto
// OFFSET: 0x006E9670
void CMovement_C::SetSplineFaceTarget(const WOWGUID& target) {
    this->m_spline->m_flags |= SPLINEFLAG_FINAL_TARGET;
    this->m_spline->m_face.m_target = target;
}

// VERIFY: 50aa2f7e auto
// OFFSET: 0x006ECAE0
void CMovement_C::AddPlayerChangeSeatsEvent(uint32_t eventTime, MoveEventId eventId, bool needAck, WOWGUID transport, uint8_t seat) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, eventId);
    event->m_facing = 0.0f;
    event->m_pitch = 0.0f;
    event->m_transportGuid = transport;
    event->m_needAck = needAck;
    event->m_seat = seat;
    event->m_ackCounter = 0;
    event->m_moveExtraFlags = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: ea0758f0 auto
// OFFSET: 0x006EC840
void CMovement_C::AddPlayerMoveEvent(uint32_t eventTime, MoveEventId eventId, bool needAck, uint32_t ackCounter, float facing, float pitch, uint16_t extraFlags) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, eventId);
    event->m_facing = facing;
    event->m_pitch = pitch;
    event->m_needAck = needAck;
    event->m_ackCounter = ackCounter;
    event->m_moveExtraFlags = extraFlags;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: b9e313d9 auto
// OFFSET: 0x006EF2A0
void CMovement_C::OnAscendDescendStartLocal(uint32_t eventTime, int32_t ascend) {
    this->AddPlayerMoveEvent(eventTime, ascend ? MOVEEVENT_START_ASCEND : MOVEEVENT_START_DESCEND, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: d22ae73c auto
// OFFSET: 0x006EF310
void CMovement_C::OnAscendDescendStopLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP_ASCEND, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 76dfcd21 auto
// OFFSET: 0x006EF060
void CMovement_C::OnCanFlyLocal(uint32_t eventTime, uint32_t ackCounter, int32_t canFly) {
    this->AddPlayerMoveEvent(eventTime, canFly ? MOVEEVENT_SET_CAN_FLY : MOVEEVENT_UNSET_CAN_FLY, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: 88ede5f4 auto
// OFFSET: 0x006EF150
void CMovement_C::OnCanTransitionBetweenSwimAndFlyLocal(uint32_t eventTime, uint32_t ackCounter, int32_t canTransition) {
    this->AddPlayerMoveEvent(eventTime, canTransition ? MOVEEVENT_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY : MOVEEVENT_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: 4f5e403f auto
// OFFSET: 0x006EE690
void CMovement_C::OnDisableGravityLocal(uint32_t eventTime, uint32_t ackCounter, int32_t disable) {
    this->AddPlayerMoveEvent(eventTime, disable ? MOVEEVENT_GRAVITY_DISABLE : MOVEEVENT_GRAVITY_ENABLE, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: 21a1bec5 auto
// OFFSET: 0x006EF540
void CMovement_C::OnDismissControlledVehicleLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_DISMISS_VEHICLE, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: f73677d3 manual
// OFFSET: 0x006EA1D0
int32_t CMovement_C::UpdateTransportStatus(WOWGUID transport, uint8_t seat) {
    if (this->m_transportGuid == transport && this->m_transportSeat == seat) {
        return 1;
    }

    uint32_t oldHigh = static_cast<uint32_t>(this->m_transportGuid.m_guid >> 32);
    uint32_t newHigh = static_cast<uint32_t>(transport.m_guid >> 32);

    if ((oldHigh & 0xF0F00000) == 0xF0500000 || (!(oldHigh & 0xF0000000) && ((oldHigh & 0xF07FFFFF) | static_cast<uint32_t>(this->m_transportGuid.m_guid))) || (newHigh & 0xF0F00000) == 0xF0500000 || (!(newHigh & 0xF0000000) && ((newHigh & 0xF07FFFFF) | static_cast<uint32_t>(transport.m_guid)))) {
        this->m_unit->VehiclePassengerOnUpdateTransport(transport, seat, 0);
    }

    this->m_transportSeat = seat;

    if (this->m_transportGuid == transport) {
        return 1;
    }

    if (this->m_transportGuid) {
        if (this->m_flags & 0x1000) {
            C44Matrix matrix;
            MovementGetTransportMtxX(this->m_transportGuid, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);
            this->m_moveDir = TransformDirection(this->m_moveDir, matrix.AffineInverse());
        }

        float facing = MovementGetTransportFacing(this->m_transportGuid);
        this->m_link.Unlink();

        uint32_t high = static_cast<uint32_t>(this->m_transportGuid.m_guid >> 32);

        if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(this->m_transportGuid.m_guid)))) {
            CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transportGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (vehicle && vehicle->m_vehicle) {
                vehicle->m_vehicle->UpdateFreeSeatMask();
            }
        }

        this->m_transportGuid = WOWGUID();
        this->ClearSplineEnabled();

        if (this->m_unit->GetObjectModel() && (this->m_unit->GetObjectModel()->m_fFlags & 0x1)) {
            this->m_unit->GetObjectModel()->ChangeFrameOfReference(nullptr);
        }

        this->m_unit->RotateFacing(facing);
    }

    if (transport) {
        int32_t reason = !(this->m_unit->m_modelFlags & 0x40000) && (!this->m_spline || (this->m_spline->m_flags & 0x400));

        if (!MovementAddToTransport(this, transport, reason)) {
            return 0;
        }

        this->m_transportGuid = transport;
        float facing = -MovementGetTransportFacing(transport);

        C44Matrix matrix;
        MovementGetTransportMtxX(transport, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);

        if (this->m_flags & 0x1000) {
            this->m_moveDir = TransformDirection(this->m_moveDir, matrix);
        }

        if (this->m_unit->GetObjectModel()) {
            CM2Model* model = this->m_unit->GetObjectModel();

            if (model->m_fFlags & 0x1) {
                model->m_frameOfReferenceMatrix = matrix;
                model->ChangeFrameOfReference(&model->m_frameOfReferenceMatrix);
            }
        }

        this->m_unit->RotateFacing(facing);
    }

    if (this->m_flags & 0x1000) {
        this->m_moveDir2D = C2Vector(this->m_moveDir);
        this->m_moveDir2D.NormalizeSafe();
    }

    return 1;
}

// VERIFY: da0fd9da manual
// OFFSET: 0x006F1520
void CMovement_C::SetUpdateInfo(uint32_t time, const CClientMoveUpdate* update, int32_t activePlayer) {
    this->m_walkSpeed = update->m_walkSpeed;
    this->m_runSpeed = update->m_runSpeed;
    this->m_runBackSpeed = update->m_runBackSpeed;
    this->m_swimSpeed = update->m_swimSpeed;
    this->m_swimBackSpeed = update->m_swimBackSpeed;
    this->m_flightSpeed = update->m_flightSpeed;
    this->m_flightBackSpeed = update->m_flightBackSpeed;
    this->m_turnRate = update->m_turnRate;
    this->m_pitchRate = update->m_pitchRate;
    this->m_anchorElapsedMs = 0;
    this->m_flags = 0;

    if (update->m_status.m_moveFlags & 0x8000000) {
        this->AddSpline(this->m_position);
        this->FlushMoveQueue(0, 1);
        this->RemoveFromMoversList(0);
        this->m_spline->CopyFrom(update->m_moveSpline);
        this->m_spline->m_spline.m_splineMode = (this->m_spline->m_flags & 0x42000) != 0;
    } else {
        if (this->m_spline && (this->m_spline->m_flags & 0x800)) {
            this->m_unit->OnCollideFallLand(0, 1);
        }

        this->RemoveSpline();
    }

    int32_t statusChanged;
    this->UpdateStatusInternal(time, update->m_status, statusChanged, activePlayer, 1);

    if ((this->m_flags & 0xC010FF) || this->m_moveQueue.Head()) {
        if (!this->m_globalUnitsLink.m_next) {
            CMovementGlobals* globals = MovementGetGlobals();

            if (globals) {
                globals->m_movers.LinkToHead(this);
            }
        }
    } else {
        this->RemoveFromMoversList(1);
    }

    if ((this->m_flags & 0xC0100F) && this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
        MovementGetGlobals()->m_timeCounterMaybe = time + 500;
    }

    if (!(this->m_flags & 0xC010FF)) {
        if (activePlayer) {
            this->OnFallLocal(time);
        } else if (this->TryStartFalling() && !this->m_globalUnitsLink.m_next) {
            CMovementGlobals* globals = MovementGetGlobals();

            if (globals) {
                globals->m_movers.LinkToHead(this);
            }
        }
    }

    MovementUpdateCameraYaw(*this->m_ownerGuid, WOWGUID{ this->m_unit->GetCameraRelativeTo() });
}

// VERIFY: efedeeb5 auto
// OFFSET: 0x006ECCF0
void CMovement_C::OnFallLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_START_FALLING, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: f9f6ad58 auto
// OFFSET: 0x006EEE50
void CMovement_C::OnFeatherFallLocal(uint32_t eventTime, uint32_t ackCounter, int32_t featherFall) {
    this->AddPlayerMoveEvent(eventTime, featherFall ? MOVEEVENT_FEATHER_FALL : MOVEEVENT_NORMAL_FALL, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: 1b613e65 auto
// OFFSET: 0x006EE120
void CMovement_C::OnFlightBackSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_FLIGHT_BACK_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: d16823ed auto
// OFFSET: 0x006EE0D0
void CMovement_C::OnFlightBackSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_FLIGHT_BACK_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: b7c435c0 auto
// OFFSET: 0x006EF230
void CMovement_C::OnFlightLocal(uint32_t eventTime, int32_t flying) {
    this->AddPlayerMoveEvent(eventTime, flying ? MOVEEVENT_SET_FLYING : MOVEEVENT_UNSET_FLYING, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 435caaf0 auto
// OFFSET: 0x006EE030
void CMovement_C::OnFlightSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_FLIGHT_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 2e542412 auto
// OFFSET: 0x006EDFE0
void CMovement_C::OnFlightSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_FLIGHT_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: ad9f01bd auto
// OFFSET: 0x006EEEF0
void CMovement_C::OnHoverLocal(uint32_t eventTime, uint32_t ackCounter, int32_t hover) {
    this->AddPlayerMoveEvent(eventTime, hover ? MOVEEVENT_SET_HOVER : MOVEEVENT_UNSET_HOVER, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: 814c2c88 auto
// OFFSET: 0x006ECC20
void CMovement_C::OnJumpLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_JUMP, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 25983ce1 auto
// OFFSET: 0x006ECC80
void CMovement_C::OnLaunchLocal(uint32_t eventTime, uint32_t ackCounter, const C2Vector& direction, float speed, float verticalSpeed) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_KNOCK_BACK);
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    event->m_value = direction.x;
    event->m_launchDirY = direction.y;
    event->m_launchSpeed = speed;
    event->m_launchVerticalSpeed = verticalSpeed;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: f22b99fe auto
// OFFSET: 0x006ECB50
void CMovement_C::OnMoveStartLocal(uint32_t eventTime, int32_t forward) {
    this->AddPlayerMoveEvent(eventTime, (MoveEventId)(forward == 0), true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: be31074a auto
// OFFSET: 0x006ECDE0
void CMovement_C::OnMoveStopLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 26308ea1 auto
// OFFSET: 0x006EE300
void CMovement_C::OnPitchRateChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_PITCH_RATE);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 8a5eeb60 auto
// OFFSET: 0x006EE2B0
void CMovement_C::OnPitchRateChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_PITCH_RATE);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 85716caa auto
// OFFSET: 0x006EECA0
void CMovement_C::OnPitchStopLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP_PITCH, true, 0, 0.0f, 0.0f, 0);
    this->m_moveQueue.UnlinkEventById(MOVEEVENT_STOP_PITCH_AT_ANGLE);
}

// VERIFY: d068e179 auto
// OFFSET: 0x006EF490
void CMovement_C::OnPitchToAngleLocal(uint32_t eventTime, float pitch) {
    CVehicle_C* vehicle = this->m_unit->m_vehicle;
    VehicleRec* rec = vehicle ? vehicle->m_vehicleRec : nullptr;

    if (rec && (rec->m_flags & 0x40)) {
        if (pitch < rec->m_pitchMin) {
            pitch = rec->m_pitchMin;
        } else if (rec->m_pitchMax <= pitch) {
            pitch = rec->m_pitchMax;
        }
    }

    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_PITCH_TO_ANGLE, true, 0, 0.0f, pitch, 0);
}

// VERIFY: 721988e2 auto
// OFFSET: 0x006EEFF0
void CMovement_C::OnRootLocal(uint32_t eventTime, uint32_t ackCounter, int32_t root) {
    this->AddPlayerMoveEvent(eventTime, root ? MOVEEVENT_ROOT : MOVEEVENT_UNROOT, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: b5f3f880 auto
// OFFSET: 0x006EDC70
void CMovement_C::OnRunBackSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_RUN_BACK_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 83d31a1b auto
// OFFSET: 0x006EDC20
void CMovement_C::OnRunBackSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_RUN_BACK_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 7b9ea868 auto
// OFFSET: 0x006EDB80
void CMovement_C::OnRunSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_RUN_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 55f19341 auto
// OFFSET: 0x006EDB30
void CMovement_C::OnRunSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_RUN_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 31a38b65 auto
// OFFSET: 0x006EE7B0
void CMovement_C::OnSetCollisionHeight(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_COLLISION_HEIGHT);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 213e5c5a auto
// OFFSET: 0x006EE760
void CMovement_C::OnSetCollisionHeightLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_COLLISION_HEIGHT);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: b7c54e54 auto
// OFFSET: 0x006EE460
void CMovement_C::OnSetPitchLocal(uint32_t eventTime, float pitch) {
    CVehicle_C* vehicle = this->m_unit->m_vehicle;
    VehicleRec* rec = vehicle ? vehicle->m_vehicleRec : nullptr;

    if (rec && (rec->m_flags & 0x40)) {
        if (pitch < rec->m_pitchMin) {
            pitch = rec->m_pitchMin;
        } else if (rec->m_pitchMax <= pitch) {
            pitch = rec->m_pitchMax;
        }
    }

    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_SET_PITCH, true, 0, 0.0f, pitch, 0);
}

// VERIFY: df4fb107 auto
// OFFSET: 0x006ECF10
void CMovement_C::OnSetRunModeLocal(uint32_t eventTime, int32_t run) {
    this->AddPlayerMoveEvent(eventTime, run ? MOVEEVENT_SET_RUN_MODE : MOVEEVENT_SET_WALK_MODE, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 2facaa06 auto
// OFFSET: 0x006ECBB0
void CMovement_C::OnStrafeStartLocal(uint32_t eventTime, int32_t left) {
    this->AddPlayerMoveEvent(eventTime, left ? MOVEEVENT_START_STRAFE_LEFT : MOVEEVENT_START_STRAFE_RIGHT, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 73637dde auto
// OFFSET: 0x006ECE40
void CMovement_C::OnStrafeStopLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP_STRAFE, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 8ba9d836 auto
// OFFSET: 0x006EDF40
void CMovement_C::OnSwimBackSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_SWIM_BACK_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: c6690fa9 auto
// OFFSET: 0x006EDEF0
void CMovement_C::OnSwimBackSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_SWIM_BACK_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: a35422a3 auto
// OFFSET: 0x006EDE50
void CMovement_C::OnSwimSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_SWIM_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 7f0fc534 auto
// OFFSET: 0x006EDE00
void CMovement_C::OnSwimSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_SWIM_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 25306062 auto
// OFFSET: 0x006EE9F0
void CMovement_C::OnSwimStart(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_START_SWIM, false, 0, 0.0f, 0.0f, 0);
}

// VERIFY: e8806cc6 auto
// OFFSET: 0x006EEB20
void CMovement_C::OnSwimStop(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP_SWIM, false, 0, 0.0f, 0.0f, 0);
}

// VERIFY: e8d0639d auto
// OFFSET: 0x006EF370
void CMovement_C::OnTimeSyncRequest(uint32_t eventTime, uint32_t counter) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_TIME_SYNC, true, counter, 0.0f, 0.0f, 0);
}

// VERIFY: b49a921d auto
// OFFSET: 0x006EE210
void CMovement_C::OnTurnRateChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_TURN_RATE);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 003e4758 auto
// OFFSET: 0x006EE1C0
void CMovement_C::OnTurnRateChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_TURN_RATE);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 6df42ae1 auto
// OFFSET: 0x006ECEA0
void CMovement_C::OnTurnStopLocal(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP_TURN, true, 0, 0.0f, 0.0f, 0);
    this->m_moveQueue.UnlinkEventById(MOVEEVENT_STOP_TURN_AT_ANGLE);
}

// VERIFY: 7f375340 auto
// OFFSET: 0x006EDD60
void CMovement_C::OnWalkSpeedChange(uint32_t eventTime, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_WALK_SPEED);
    event->m_value = value;
    event->m_needAck = 0;
    event->m_ackCounter = 0;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: d5ba5c2f auto
// OFFSET: 0x006EDD10
void CMovement_C::OnWalkSpeedChangeLocal(uint32_t eventTime, uint32_t ackCounter, float value) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_SET_WALK_SPEED);
    event->m_value = value;
    event->m_ackCounter = ackCounter;
    event->m_needAck = 1;
    this->m_moveQueue.Enqueue(event);
    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 6af8b4ef auto
// OFFSET: 0x006EED70
void CMovement_C::OnWaterWalkLocal(uint32_t eventTime, uint32_t ackCounter, int32_t waterWalk) {
    this->AddPlayerMoveEvent(eventTime, waterWalk ? MOVEEVENT_WATER_WALK : MOVEEVENT_LAND_WALK, true, ackCounter, 0.0f, 0.0f, 0);
}

// VERIFY: 181a5182 auto
// OFFSET: 0x006ECD50
void CMovement_C::QueueFallEvent(uint32_t eventTime) {
    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_START_FALLING, this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 193562a4 manual
// OFFSET: 0x0075E040
int32_t CMovement_C::CheckFallImpactThreshold(float timeFallen, float elapsed, const C3Vector& move, int32_t force) const {
    if (force) {
        return 1;
    }

    if (this->m_fallVelocity < 0.0f) {
        float peak = this->TimeToJumpPeak();

        if (elapsed + timeFallen < peak) {
            return 1;
        }

        if (timeFallen > peak) {
            return 0;
        }

        float distance = (peak - timeFallen) * this->m_currentSpeed;

        if (move.x * move.x + move.y * move.y < distance * distance) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 987977e3 manual
// OFFSET: 0x0075E3D0
uint32_t CMovement_C::GetFacetQueryFlags() const {
    uint32_t flags = 0x100111;

    if (!this->m_unit->IsClientControlled()) {
        flags = 0x102111;
    }

    if (this->m_unit->IsLocalClientControlled()) {
        flags |= 0x80000000;
    }

    uint32_t moveFlags = this->m_flags;

    if ((moveFlags & MOVEMENTFLAG_WATERWALKING) && !(moveFlags & MOVEMENTFLAG_SWIMMING) && (-0.6457718f < this->m_pitch || (this->m_flags2 & MOVEMENTFLAG2_UNK_0200))) {
        flags |= 0x10000;
    }

    if (moveFlags & MOVEMENTFLAG_FLYING) {
        flags |= 0x200;

        if (!(this->m_flags2 & MOVEMENTFLAG2_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY)) {
            flags |= 0x20000;
        }
    }

    CGUnit_C* unit = this->m_unit;

    if ((unit->m_obj->m_type & TYPEMASK_PLAYER) && (static_cast<CGPlayer_C*>(unit)->m_player->m_playerFlags & 0x10)) {
        flags |= 0x8000;
    }

    return flags;
}

// VERIFY: 44c1133a manual
// OFFSET: 0x0075CA80
void CMovement_C::BuildCollisionVolume(const C3Vector& position, float height, C4Plane* planes, C3Vector* points, uint8_t* faces) const {
    this->BuildCollisionVolumePlanes(position, height, this->m_collisionRadius, planes);

    float lowZ = this->m_collisionRadius * 1.849399f;

    points[0] = position;
    points[1] = C3Vector(position.x - this->m_collisionRadius, position.y - this->m_collisionRadius, lowZ + position.z);
    points[2] = C3Vector(position.x - this->m_collisionRadius, this->m_collisionRadius + position.y, lowZ + position.z);
    points[3] = C3Vector(this->m_collisionRadius + position.x, this->m_collisionRadius + position.y, lowZ + position.z);
    points[4] = C3Vector(this->m_collisionRadius + position.x, position.y - this->m_collisionRadius, lowZ + position.z);
    points[5] = C3Vector(position.x - this->m_collisionRadius, position.y - this->m_collisionRadius, height + position.z);
    points[6] = C3Vector(position.x - this->m_collisionRadius, this->m_collisionRadius + position.y, height + position.z);
    points[7] = C3Vector(this->m_collisionRadius + position.x, this->m_collisionRadius + position.y, height + position.z);
    points[8] = C3Vector(this->m_collisionRadius + position.x, position.y - this->m_collisionRadius, height + position.z);

    faces[0] = 1;
    faces[1] = 2;
    faces[2] = 6;
    faces[3] = 5;
    faces[4] = 3;
    faces[5] = 4;
    faces[6] = 8;
    faces[7] = 7;
    faces[8] = 2;
    faces[9] = 3;
    faces[10] = 7;
    faces[11] = 6;
    faces[12] = 4;
    faces[13] = 1;
    faces[14] = 5;
    faces[15] = 8;
    faces[16] = 5;
    faces[17] = 6;
    faces[18] = 7;
    faces[19] = 8;
    faces[20] = 0;
    faces[21] = 1;
    faces[22] = 2;
    faces[23] = 0;
    faces[24] = 3;
    faces[25] = 4;
    faces[26] = 0;
    faces[27] = 2;
    faces[28] = 3;
    faces[29] = 0;
    faces[30] = 4;
    faces[31] = 1;
}

// VERIFY: 2b0fe832 auto
// OFFSET: 0x006E91D0
int32_t CMovement_C::OnFlightBackSpeedChange(float value) {
    return this->ChangeFlightBackSpeed(value);
}

// VERIFY: 8ce26848 auto
// OFFSET: 0x006E91B0
int32_t CMovement_C::OnFlightSpeedChange(float value) {
    return this->ChangeFlightSpeed(value);
}

// VERIFY: e6981de6 auto
// OFFSET: 0x006E9210
int32_t CMovement_C::OnPitchRateChange(float value) {
    return this->ChangePitchRate(value);
}

// VERIFY: ffcdbdf3 auto
// OFFSET: 0x006E9130
int32_t CMovement_C::OnRunBackSpeedChange(float value) {
    return this->ChangeRunBackSpeed(value);
}

// VERIFY: 8076b053 auto
// OFFSET: 0x006E9110
int32_t CMovement_C::OnRunSpeedChange(float value) {
    return this->ChangeRunSpeed(value);
}

// VERIFY: bb3db46c auto
// OFFSET: 0x006E9190
int32_t CMovement_C::OnSwimBackSpeedChange(float value) {
    return this->ChangeSwimBackSpeed(value);
}

// VERIFY: 622e8b42 auto
// OFFSET: 0x006E9170
int32_t CMovement_C::OnSwimSpeedChange(float value) {
    return this->ChangeSwimSpeed(value);
}

// VERIFY: 56a6fec9 auto
// OFFSET: 0x006E91F0
int32_t CMovement_C::OnTurnRateChange(float value) {
    return this->ChangeTurnRate(value);
}

// VERIFY: a98a8ed5 auto
// OFFSET: 0x006E9150
int32_t CMovement_C::OnWalkSpeedChange(float value) {
    return this->ChangeWalkSpeed(value);
}

// VERIFY: b9bcc84b auto
// OFFSET: 0x006E9250
void CMovement_C::OnFeatherFall(int32_t enable) {
    this->FeatherFall(enable);
    this->PostFeatherFall();
}

// VERIFY: 4bc4b2f5 auto
// OFFSET: 0x006EF5A0
void CMovement_C::OnChangeSeatsOnControlledVehicleLocal(uint32_t eventTime, WOWGUID transport, uint8_t seat) {
    this->AddPlayerChangeSeatsEvent(eventTime, MOVEEVENT_CHANGE_SEAT, true, transport, seat);
}

// VERIFY: 047dad0f auto
// OFFSET: 0x006E9570
void CMovement_C::SetCollisionBox(float width, float height, float scale, float referenceScale, int32_t force) {
    float stepUpScale = 1.0f;

    if (!(fabs(referenceScale) < 2.3841858e-7f)) {
        stepUpScale = scale / referenceScale;
    }

    this->m_stepUpHeight = stepUpScale;
    this->m_collisionRadius = width * scale * 0.5f;

    if (force || !this->m_unit->IsClientControlled()) {
        this->m_collisionHeight = scale * height;
    }

    this->ValidateWorldPosition(CPassenger::GetPosition(this->m_position));
}

// VERIFY: 8b6b9a4f manual
// OFFSET: 0x006E9470
void CMovement_C::SnapToSpline(const C3Vector& position, int32_t force) {
    if (!force) {
        float dx = position.x - this->m_position.x;
        float dy = position.y - this->m_position.y;
        float dz = position.z - this->m_position.z;

        if (dx * dx + dy * dy + dz * dz < 9.0f) {
            return;
        }
    }

    this->m_position = position;

    if (this->m_spline->m_flags & SPLINEFLAG_FALLING) {
        this->m_fallTimeMs = this->m_spline->m_start;
        this->UpdateAnchors(0);
    } else {
        this->StopFallingAlwaysAnchor();
    }

    this->ValidateWorldPosition(CPassenger::GetPosition(this->m_position));
}

// VERIFY: bec49cf6 auto
// OFFSET: 0x00632050
void CMovement_C::ValidateWorldPosition(const C3Vector& position) const {
}

// VERIFY: 98dc8a0f auto
// OFFSET: 0x0075EB00
float CMovement_C::GetTimeJustFallen(float elapsed, float timeFallen, C3Vector* move, float* distance, int32_t force, float speed) {
    int32_t impact = this->CheckFallImpactThreshold(timeFallen, elapsed, *move, force);
    float moveTime = 0.0f;

    if (speed > 9.5367432e-7f) {
        C2Vector move2D(*move);
        moveTime = sqrt(move2D.x * move2D.x + move2D.y * move2D.y) / speed * elapsed;
    }

    float fallTime = this->CalcTimeFallen(this->m_fallStartZ - this->m_position.z - move->z, impact);

    if (!(timeFallen < fallTime)) {
        move->x = 0.0f;
        move->y = 0.0f;
        move->z = 0.0f;
        *distance = 0.0f;
        return 0.0f;
    }

    float remaining = fallTime - timeFallen;

    if (elapsed < remaining) {
        return elapsed;
    }

    if (moveTime > remaining) {
        float scale = remaining / moveTime;
        move->x = move->x * scale;
        move->y = scale * move->y;
        *distance = sqrt(move->x * move->x + move->y * move->y + move->z * move->z);
    }

    return remaining;
}

// VERIFY: 9474deaa manual
// OFFSET: 0x0072FE00
CMovement_C::~CMovement_C() {
    this->Disable();
}

// VERIFY: 927722c0 manual
// OFFSET: 0x006EBD30
CMovement_C::CMovement_C(const C3Vector& position, float facing, const WOWGUID& ownerGuid, CGUnit_C* unit) : CMovementShared(WOWGUID(), position, facing, ownerGuid) {
    this->m_collisionRadius = 0.33333334f;
    this->m_collisionHeight = 2.0277777f;
    this->m_stepUpHeight = 1.0f;
    this->m_interpolationPos.x = 0.0f;
    this->m_interpolationPos.y = 0.0f;
    this->m_interpolationPos.z = 0.0f;
    this->m_timeSkewIndex = 0;
    this->m_interpolation = 0;
    this->m_moveStartDelay = 0;
    this->m_unit = unit;
    this->CalcDirection(0);

    for (uint32_t i = 0; i < 16; i++) {
        this->m_timeSkewSamples[i] = -50;
    }

    for (uint32_t i = 16; i < 32; i++) {
        this->m_timeSkewSamples[i] = 50;
    }

    this->m_timeSkew = 50;
}

// VERIFY: db4836af manual
// OFFSET: 0x0075E0C0
C3Vector CMovement_C::CalcRunWalkWalkableObstaclePush(const C3Vector& move, float* scale, C3Vector normal, int32_t useAltNormal, const C3Vector& altNormal) const {
    if (useAltNormal && normal.z <= 0.64278764f && !(fabs(altNormal.z * altNormal.z + altNormal.y * altNormal.y + altNormal.x * altNormal.x) < 2.3841858e-7f)) {
        normal.x = -altNormal.x;
        normal.y = -altNormal.y;
        normal.z = -altNormal.z;
    }

    float push = (-normal.x * move.x + -normal.y * move.y + -normal.z * move.z) * *scale;
    float height;

    if (fabs(normal.z) < 2.3841858e-7f) {
        height = push >= 0.0f ? 3.4028235e38f : -3.4028235e38f;
    } else {
        height = push / normal.z;
    }

    float result;

    if ((this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) && height < 0.0f) {
        *scale = 0.0f;
        result = this->GetRemainingStepUpHeight();
    } else if (this->GetRemainingStepUpHeight() >= height) {
        result = height;

        if (height < -this->GetStepUpHeight()) {
            *scale = -(this->GetStepUpHeight() * (*scale / height));
            result = -this->GetStepUpHeight();
        }
    } else {
        *scale = this->GetRemainingStepUpHeight() * (*scale / height);
        result = this->GetRemainingStepUpHeight();
    }

    return C3Vector(0.0f * result, 0.0f * result, result);
}

// VERIFY: 4914c81c manual
// OFFSET: 0x006E90E0
void CMovement_C::GetMoveStatus(NETMESSAGE msgId, uint32_t gameTime, CMovementStatus* status) const {
    status->m_moveFlags = this->m_flags & ~(MOVEMENTFLAG_ONTRANSPORT | MOVEMENTFLAG_SPLINE_ENABLED | MOVEMENTFLAG_STATUS_APPLIED);
    CMovementShared::GetMoveStatus(msgId, gameTime, status);
}

// VERIFY: da8cd466 auto
// OFFSET: 0x006EB3B0
int32_t CMovement_C::HandlePendingActions() {
    uint32_t oldFlags = this->m_flags;

    if (oldFlags & MOVEMENTFLAG_PENDING_STOP) {
        this->ForceStopMove(1);

        if (this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover && !this->m_moveQueue.HasEventBetween(MOVEEVENT_START_FORWARD, MOVEEVENT_START_BACKWARD)) {
            CGInputControl::GetActive()->UpdateMoveStopped();
        }
    }

    if (this->m_flags & MOVEMENTFLAG_PENDING_STRAFE_STOP) {
        this->ForceStopStrafe();
    }

    if (this->m_flags & (MOVEMENTFLAG_PENDING_FORWARD | MOVEMENTFLAG_PENDING_BACKWARD)) {
        this->StartMove(this->m_flags & MOVEMENTFLAG_PENDING_FORWARD, 0);
    }

    if (this->m_flags & (MOVEMENTFLAG_PENDING_STRAFE_LEFT | MOVEMENTFLAG_PENDING_STRAFE_RIGHT)) {
        this->StartStrafe(this->m_flags & MOVEMENTFLAG_PENDING_STRAFE_LEFT);
    }

    this->m_flags &= ~(MOVEMENTFLAG_PENDING_STOP | MOVEMENTFLAG_PENDING_STRAFE_STOP | MOVEMENTFLAG_PENDING_FORWARD | MOVEMENTFLAG_PENDING_BACKWARD | MOVEMENTFLAG_PENDING_STRAFE_LEFT | MOVEMENTFLAG_PENDING_STRAFE_RIGHT | MOVEMENTFLAG_PENDING_ROOT);
    return (oldFlags ^ this->m_flags) & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT);
}

// VERIFY: 29d51d84 manual
// OFFSET: 0x006E9FF0
void CMovement_C::Launch(const C2Vector& direction, float speed, float verticalSpeed) {
    if (this->m_flags & MOVEMENTFLAG_SWIMMING) {
        this->StopSwim();
    } else if (this->m_flags & MOVEMENTFLAG_FLYING) {
        this->StopFlying();
    }

    if (this->m_flags & MOVEMENTFLAG_ROOT) {
        this->m_flags = (this->m_flags & ~(MOVEMENTFLAG_ROOT | MOVEMENTFLAG_PENDING_ROOT)) | MOVEMENTFLAG_PENDING_ROOT;
    }

    this->m_flags &= ~MOVEMENTFLAG_CAN_FLY;
    this->StartFalling(verticalSpeed);
    this->m_moveDir2D = direction;
    this->m_moveDir = C3Vector(direction.x, direction.y, 0.0f);

    WOWGUID transport = *(const WOWGUID*)&this->m_transportGuid;

    if (transport && this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
        C44Matrix matrix;
        MovementGetTransportMtxX(transport, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);
        C44Matrix inverse = matrix.AffineInverse();

        this->m_moveDir = C3Vector(
            inverse.b0 * this->m_moveDir2D.y + this->m_moveDir2D.x * inverse.a0,
            inverse.b1 * this->m_moveDir2D.y + inverse.a1 * this->m_moveDir2D.x,
            inverse.b2 * this->m_moveDir2D.y + inverse.a2 * this->m_moveDir2D.x);
        this->m_moveDir2D = C2Vector(this->m_moveDir);

        float lengthSq = this->m_moveDir2D.y * this->m_moveDir2D.y + this->m_moveDir2D.x * this->m_moveDir2D.x;

        if (lengthSq > 2.3841858e-7f) {
            float scale = 1.0f / sqrt(lengthSq);
            this->m_moveDir2D.x = this->m_moveDir2D.x * scale;
            this->m_moveDir2D.y = scale * this->m_moveDir2D.y;
        }
    }

    this->m_currentSpeed = speed;
    this->m_flags = (this->m_flags & ~MOVEMENTFLAG_BACKWARD) | MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_PENDING_STOP;
}

// VERIFY: 9bb92d2b auto
// OFFSET: 0x006EBE50
void CMovement_C::SplineToggleEnableFlying(int32_t enable) {
    if (enable) {
        if (this->StartFlight()) {
            this->HandlePendingActions();
        }
    } else {
        this->StopFlying();
    }
}

// VERIFY: 6e48f367 auto
// OFFSET: 0x006E9B70
void CMovement_C::UpdateHeartbeatTimerA(uint32_t time) {
    if (this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
        MovementGetGlobals()->m_timeCounterMaybe = time + 500;
    }
}

// VERIFY: cd3e6593 auto
// OFFSET: 0x006EAA50
int32_t CMovement_C::ComputeLegalRawFacingRange(float& min, float& max) const {
    CGUnit_C* unit = this->m_unit;
    CVehicle_C* vehicle = unit->m_vehicle;

    if (!vehicle || !vehicle->m_vehicleRec) {
        return 0;
    }

    VehicleRec* rec = vehicle->m_vehicleRec;

    if (!rec || !(rec->m_flags & 0x200000)) {
        return 0;
    }

    float facing = unit->m_vehicle->m_rawFacing;
    min = facing - rec->m_facingLimitRight;
    max = facing + rec->m_facingLimitLeft;
    return 1;
}

// VERIFY: 08ba40d8 manual
// OFFSET: 0x006EF3D0
void CMovement_C::OnTurnToAngleLocal(uint32_t eventTime, float facing) {
    float min;
    float max;

    if (this->ComputeLegalRawFacingRange(min, max)) {
        CMovement_C::WrapFacingToRange(facing, min, max);

        if (facing < min) {
            facing = min;
        } else if (max <= facing) {
            facing = max;
        }
    }

    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_TURN_TO_ANGLE, true, 0, facing, 0.0f, 0);
}

// VERIFY: 77cf4a10 manual
// OFFSET: 0x006EE940
void CMovement_C::OnSwimStartLocal(uint32_t eventTime) {
    if (this->IsOnSpline()) {
        this->StartSwim();
        this->HandlePendingActions();
        this->RemoveFromMoversList(1);
        this->m_unit->ProcessLocalMoveEvent(eventTime, MSG_MOVE_START_SWIM, true, 0.0f, 0, 0, 255);
        return;
    }

    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_START_SWIM, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: a91c7c3b manual
// OFFSET: 0x006EEA50
void CMovement_C::OnSwimStopLocal(uint32_t eventTime) {
    if (this->IsOnSpline()) {
        this->StopSwim();

        if ((this->m_flags & 0xC010FF) || this->m_moveQueue.Head()) {
            if (!this->m_globalUnitsLink.IsLinked()) {
                CMovementGlobals* globals = MovementGetGlobals();

                if (globals) {
                    globals->m_movers.LinkToHead(this);
                }
            }
        }

        this->m_unit->ProcessLocalMoveEvent(eventTime, MSG_MOVE_STOP_SWIM, true, 0.0f, 0, 0, 255);
        return;
    }

    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_STOP_SWIM, true, 0, 0.0f, 0.0f, 0);
}

// VERIFY: 1011e33b auto
// OFFSET: 0x006F0F70
void CMovement_C::OnTurnStartLocal(uint32_t eventTime, int32_t left) {
    float min;
    float max;

    if (this->ComputeLegalRawFacingRange(min, max)) {
        this->OnTurnToAngleLocal(eventTime, left ? max : min);
        return;
    }

    this->AddPlayerMoveEvent(eventTime, left ? MOVEEVENT_START_TURN_LEFT : MOVEEVENT_START_TURN_RIGHT, true, 0, 0.0f, 0.0f, 0);
    this->m_moveQueue.UnlinkEventById(MOVEEVENT_STOP_TURN_AT_ANGLE);
    CGInputControl::GetActive()->OnTurnToAngleStop();
}

// VERIFY: 0d7fb371 manual
// OFFSET: 0x006F1310
void CMovement_C::OnPitchStartLocal(uint32_t eventTime, int32_t up) {
    CVehicle_C* vehicle = this->m_unit->m_vehicle;
    VehicleRec* rec = vehicle ? vehicle->m_vehicleRec : nullptr;

    if (rec && (rec->m_flags & 0x40)) {
        this->OnPitchToAngleLocal(eventTime, up ? rec->m_pitchMax : rec->m_pitchMin);
        return;
    }

    this->AddPlayerMoveEvent(eventTime, up ? MOVEEVENT_START_PITCH_UP : MOVEEVENT_START_PITCH_DOWN, true, 0, 0.0f, 0.0f, 0);
    this->m_moveQueue.UnlinkEventById(MOVEEVENT_STOP_PITCH_AT_ANGLE);
    CGInputControl::GetActive()->OnPitchToAngleStop();
}

// VERIFY: d22f991a auto
// OFFSET: 0x007164B0
void CMovement_C::WriteMovementStatusToPacket(uint32_t msgId, uint32_t time, CDataStore* msg) const {
    if (msgId == MSG_MOVE_TELEPORT_ACK) {
        msg->Put(time);
        return;
    }

    CMovementStatus status;
    this->GetMoveStatus(static_cast<NETMESSAGE>(msgId), time, &status);
    *msg << status;
}

// VERIFY: 4c1da1d0 auto
// OFFSET: 0x0075B560
static void Swap(C4Plane& a, C4Plane& b) {
    C4Plane temp = a;
    a = b;
    b = temp;
}

// VERIFY: 6455122b auto
// OFFSET: 0x0075B690
static int32_t IsSurfaceTooSteep(uint32_t facet) {
    return s_moveFacets.facets.m_data[facet].plane.n.z <= 0.64278764f;
}

// VERIFY: 8257420a manual
// OFFSET: 0x0075B6C0
static int32_t IsTooSteepForUnit(const CGUnit_C* unit, uint32_t facet) {
    if (unit->IsClientControlled()) {
        return !(s_moveFacets.facets.m_data[facet].plane.n.z > 0.64278764f);
    }

    return !(s_moveFacets.facets.m_data[facet].plane.n.z > 0.17364818f);
}

// VERIFY: 83e9ad0d manual
// OFFSET: 0x0075B710
static void ClipPolygonToPlane(ClipPolygon& polygon, int32_t planeIndex, const C4Plane& plane) {
    float minDist = 3.4028235e38f;
    float maxDist = -3.4028235e38f;
    int32_t count = polygon.m_count;
    float dist[15];

    for (int32_t i = 0; i < count; i++) {
        float d = -(polygon.m_v[i].x * plane.n.x + polygon.m_v[i].z * plane.n.z + polygon.m_v[i].y * plane.n.y + plane.d);

        if (d < minDist) {
            minDist = d;
        }

        if (d > maxDist) {
            maxDist = d;
        }

        dist[i] = d;
    }

    if (minDist > -0.0013888889f) {
        return;
    }

    if (maxDist < 0.0013888889f) {
        polygon.m_count = 0;
        return;
    }

    ClipPolygon source(polygon);
    polygon.m_count = 0;

    for (int32_t i = 0, prev = count - 1; i < count; prev = i, i++) {
        float cur = dist[i];
        float prv = dist[prev];

        if (prv >= 0.0f) {
            if (cur < 0.0f) {
                if (prv > 0.0013888889f) {
                    float t = prv / (cur - prv);
                    polygon.m_flags[polygon.m_count] = planeIndex;
                    polygon.m_v[polygon.m_count] = C3Vector(source.m_v[prev].x - (source.m_v[i].x - source.m_v[prev].x) * t, source.m_v[prev].y - (source.m_v[i].y - source.m_v[prev].y) * t, source.m_v[prev].z - (source.m_v[i].z - source.m_v[prev].z) * t);
                    polygon.m_count++;
                }

                continue;
            }
        } else {
            if (cur < 0.0f) {
                continue;
            }

            if (cur > 0.0013888889f) {
                float t = prv / (cur - prv);
                polygon.m_flags[polygon.m_count] = planeIndex;
                polygon.m_v[polygon.m_count] = C3Vector(source.m_v[prev].x - (source.m_v[i].x - source.m_v[prev].x) * t, source.m_v[prev].y - (source.m_v[i].y - source.m_v[prev].y) * t, source.m_v[prev].z - (source.m_v[i].z - source.m_v[prev].z) * t);
                polygon.m_count++;
            }
        }

        polygon.m_flags[polygon.m_count] = source.m_flags[i];
        polygon.m_v[polygon.m_count] = source.m_v[i];
        polygon.m_count++;
    }

    if (static_cast<uint32_t>(polygon.m_count) < 3) {
        polygon.m_count = 0;
    }
}

// VERIFY: ad914dd6 manual
// OFFSET: 0x0075BC50
static int32_t ExtrudeTriangle(const C3Vector* vertices, const uint8_t* indices, const C3Vector& normal, C4Plane* planes, const C3Vector& offset) {
    uint32_t next[3] = { 1, 2, 0 };
    uint32_t prev[3] = { 2, 0, 1 };

    for (uint32_t i = 0; i < 3; i++) {
        const C3Vector& v0 = vertices[indices[i]];
        float ax = (offset.x + v0.x) - v0.x;
        float ay = (offset.y + v0.y) - v0.y;
        float az = (offset.z + v0.z) - v0.z;
        const C3Vector& v1 = vertices[indices[next[i]]];
        float bx = v1.x - v0.x;
        float by = v1.y - v0.y;
        float bz = v1.z - v0.z;
        float nx = by * az - ay * bz;
        float ny = bz * ax - az * bx;
        float nz = bx * ay - ax * by;
        float lengthSq = nz * nz + ny * ny + nx * nx;

        if (lengthSq < 0.00000095367432f) {
            return 0;
        }

        float scale = 1.0f / sqrt(lengthSq);
        C4Plane& plane = planes[i];
        plane.n.x = nx * scale;
        plane.n.y = ny * scale;
        plane.n.z = scale * nz;
        plane.d = -(plane.n.z * v0.z + plane.n.y * v0.y + plane.n.x * v0.x);

        const C3Vector& v2 = vertices[indices[prev[i]]];

        if (plane.n.y * (v2.y - v0.y) + (v2.z - v0.z) * plane.n.z + (v2.x - v0.x) * plane.n.x > 0.0f) {
            plane.n.x = -plane.n.x;
            plane.n.y = -plane.n.y;
            plane.n.z = -plane.n.z;
            plane.d = -plane.d;
        }
    }

    const C3Vector& base = vertices[indices[0]];
    float px = base.x + offset.x;
    float py = base.y + offset.y;
    float pz = base.z + offset.z;
    planes[3].n = normal;
    planes[3].d = -(px * normal.x + py * normal.y + pz * normal.z);
    return 1;
}

// VERIFY: 7d38609a manual
// OFFSET: 0x0075BE80
static int32_t ComputeDistanceToMoveImpl(const C3Vector* vertices, const C3Vector& normal, const C3Vector& move, C4Plane* planes, const uint8_t* indices) {
    for (uint32_t i = 0; i < 4; i++) {
        const C3Vector& v0 = vertices[indices[i]];
        float ax = (v0.x + move.x) - v0.x;
        float ay = (v0.y + move.y) - v0.y;
        float az = (v0.z + move.z) - v0.z;
        const C3Vector& v1 = vertices[indices[(i + 1) & 3]];
        float bx = v1.x - v0.x;
        float by = v1.y - v0.y;
        float bz = v1.z - v0.z;
        float nx = by * az - ay * bz;
        float ny = bz * ax - az * bx;
        float nz = bx * ay - ax * by;
        float lengthSq = nz * nz + ny * ny + nx * nx;

        if (lengthSq < 0.00000095367432f) {
            return 0;
        }

        float scale = 1.0f / sqrt(lengthSq);
        C4Plane& plane = planes[i];
        plane.n.x = nx * scale;
        plane.n.y = ny * scale;
        plane.n.z = scale * nz;
        plane.d = -(plane.n.z * v0.z + plane.n.y * v0.y + plane.n.x * v0.x);

        const C3Vector& v2 = vertices[indices[(i - 1) & 3]];

        if (plane.n.y * (v2.y - v0.y) + (v2.z - v0.z) * plane.n.z + (v2.x - v0.x) * plane.n.x > 0.0f) {
            plane.n.x = -plane.n.x;
            plane.n.y = -plane.n.y;
            plane.n.z = -plane.n.z;
            plane.d = -plane.d;
        }
    }

    const C3Vector& base = vertices[indices[0]];
    float px = base.x + move.x;
    float py = base.y + move.y;
    float pz = base.z + move.z;
    planes[4].n = normal;
    planes[4].d = -(px * normal.x + py * normal.y + pz * normal.z);
    return 1;
}

// VERIFY: 17a4a601 manual
// OFFSET: 0x0075C0B0
static int32_t ClipGenericPolygon(ClipPolygon& polygon, const C4Plane* planes, uint32_t planeIndex, float* distance, const C3Vector& direction) {
    float best = *distance;
    int32_t behind = 1;
    const C4Plane& plane = planes[planeIndex];
    float denom = direction.z * plane.n.z + direction.y * plane.n.y + plane.n.x * direction.x;

    for (int32_t i = 0; i < polygon.m_count; i++) {
        float t = polygon.m_v[i].z * plane.n.z + polygon.m_v[i].x * plane.n.x + polygon.m_v[i].y * plane.n.y + plane.d;

        if (fabs(denom) >= 0.00000023841858f) {
            t = t / denom;
        }

        if (t < best) {
            best = t <= 0.0f ? 0.0f : t;
        }

        if (t > -0.00000095367432f) {
            behind = 0;
        }
    }

    if (behind) {
        for (uint32_t i = 0; i < 9; i++) {
            if (i != planeIndex) {
                ClipPolygonToPlane(polygon, i, planes[i]);

                if (!polygon.m_count) {
                    return 0;
                }
            }
        }

        for (int32_t i = 0; i < polygon.m_count; i++) {
            float t = polygon.m_v[i].z * plane.n.z + polygon.m_v[i].x * plane.n.x + polygon.m_v[i].y * plane.n.y + plane.d;

            if (fabs(denom) >= 0.00000023841858f) {
                t = t / denom;
            }

            if (t > -0.027777778f) {
                behind = 0;
            }
        }

        if (behind) {
            return 0;
        }
    }

    if (best >= *distance) {
        return 0;
    }

    *distance = best;
    return 1;
}

// VERIFY: 8a756579 auto
// OFFSET: 0x0075BB00
static int32_t ClipPolygonToFrustum(ClipPolygon& polygon, const CAaBox& box) {
    C4Plane plane;

    plane.n = C3Vector(-1.0f, 0.0f, 0.0f);
    plane.d = box.b.x;
    ClipPolygonToPlane(polygon, -1, plane);

    if (!polygon.m_count) {
        return 0;
    }

    plane.n = C3Vector(1.0f, 0.0f, 0.0f);
    plane.d = -box.t.x;
    ClipPolygonToPlane(polygon, -1, plane);

    if (!polygon.m_count) {
        return 0;
    }

    plane.n = C3Vector(0.0f, -1.0f, 0.0f);
    plane.d = box.b.y;
    ClipPolygonToPlane(polygon, -1, plane);

    if (!polygon.m_count) {
        return 0;
    }

    plane.n = C3Vector(0.0f, 1.0f, 0.0f);
    plane.d = -box.t.y;
    ClipPolygonToPlane(polygon, -1, plane);

    if (!polygon.m_count) {
        return 0;
    }

    plane.n = C3Vector(0.0f, 0.0f, -1.0f);
    plane.d = box.b.z;
    ClipPolygonToPlane(polygon, -1, plane);

    if (!polygon.m_count) {
        return 0;
    }

    plane.n = C3Vector(0.0f, 0.0f, 1.0f);
    plane.d = -box.t.z;
    ClipPolygonToPlane(polygon, -1, plane);

    return polygon.m_count != 0;
}

// VERIFY: 87ab8510 auto
// OFFSET: 0x0075C6F0
static int32_t PolygonAtPoint(const ClipPolygon& polygon, const C3Vector& point) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(polygon.m_count); i++) {
        float dy = point.y - polygon.m_v[i].y;
        float dz = point.z - polygon.m_v[i].z;
        float dx = point.x - polygon.m_v[i].x;

        if (dx * dx + dz * dz + dy * dy > 0.00077160494f) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 31d8651a manual
// OFFSET: 0x0075C750
static int32_t ClipCollisionBox(const CAaBox& box) {
    ClipPolygon polygon;
    polygon.m_count = 3;

    for (uint32_t i = 0; i < s_moveFacets.facets.m_count; i++) {
        memcpy(polygon.m_v, s_moveFacets.facets.m_data[i].v, sizeof(s_moveFacets.facets.m_data[i].v));
        memset(polygon.m_flags, 0xFF, sizeof(int32_t) * 3);
        polygon.m_count = 3;

        if (ClipPolygonToFrustum(polygon, box)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: dd41383e manual
// OFFSET: 0x0075C800
static int32_t ClipBoxAgainstPlanes(const C3Vector& position, const C4Plane* planes, uint32_t count) {
    ClipPolygon polygon;
    polygon.m_count = 3;

    for (uint32_t i = 0; i < s_moveFacets.facets.m_count; i++) {
        memcpy(polygon.m_v, s_moveFacets.facets.m_data[i].v, sizeof(s_moveFacets.facets.m_data[i].v));
        polygon.m_count = 3;
        memset(polygon.m_flags, 0xFF, sizeof(int32_t) * 3);

        for (uint32_t j = 0; j < count; j++) {
            ClipPolygonToPlane(polygon, j, planes[j]);

            if (!polygon.m_count) {
                break;
            }
        }

        if (polygon.m_count && !PolygonAtPoint(polygon, position)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 124c195e auto
// OFFSET: 0x0075E480
int32_t CMovement_C::CollisionBoxDoesntFit(const CAaBox& box) {
    uint32_t flags = this->GetFacetQueryFlags();
    s_queryBox = box;

    if (!World::GetFacets(&s_queryBox, &s_moveFacets, flags, nullptr)) {
        return 1;
    }

    return ClipCollisionBox(box);
}

// VERIFY: a677f218 manual
// OFFSET: 0x0075E500
int32_t CMovement_C::BoxDoesntFit(float height, float radius) {
    uint32_t flags = this->GetFacetQueryFlags();
    s_queryBox.b = C3Vector(-radius, -radius, 0.0f);
    s_queryBox.t = C3Vector(radius, radius, height);
    C3Vector position = CPassenger::GetPosition(this->m_position);
    s_queryBox.b.x = s_queryBox.b.x + position.x;
    s_queryBox.b.y = position.y + s_queryBox.b.y;
    s_queryBox.b.z = position.z + s_queryBox.b.z;
    s_queryBox.t.x = s_queryBox.t.x + position.x;
    s_queryBox.t.y = position.y + s_queryBox.t.y;
    s_queryBox.t.z = position.z + s_queryBox.t.z;

    if (!World::GetFacets(&s_queryBox, &s_moveFacets, flags, nullptr)) {
        return 1;
    }

    C4Plane planes[9];
    this->BuildCollisionVolumePlanes(this->m_position, height, radius, planes);
    return ClipBoxAgainstPlanes(this->m_position, planes, 9);
}

// VERIFY: cc7ae3d9 auto
// OFFSET: 0x006E8FF0
int32_t CMovement_C::CollisionBoxDoesntFit(float width, float height, float scale) {
    float radius = width * scale * 0.5f;

    if (radius * 1.849399f > scale) {
        radius = scale * 0.54071623f;
    }

    return this->BoxDoesntFit(height * scale, radius);
}

// VERIFY: ae5e5bd7 manual
// OFFSET: 0x0075C5A0
static int32_t ClipMovementPyramid(World::FacetData* facets, const C3Vector& direction, const C4Plane* clipPlanes, uint32_t clipCount, const C4Plane* planes, uint32_t planeIndex, float* distance, uint32_t* facetIndex) {
    int32_t found = 0;
    ClipPolygon polygon;
    polygon.m_count = 3;

    for (uint32_t i = 0; i < facets->facets.m_count; i++) {
        const CFacet& facet = facets->facets.m_data[i];

        if (facet.plane.n.z * direction.z + facet.plane.n.y * direction.y + facet.plane.n.x * direction.x > -0.0000099999997f) {
            continue;
        }

        memcpy(polygon.m_v, facet.v, sizeof(facet.v));
        polygon.m_count = 3;
        memset(polygon.m_flags, 0xFF, sizeof(int32_t) * 3);

        for (uint32_t j = 0; j < clipCount; j++) {
            ClipPolygonToPlane(polygon, j, clipPlanes[j]);

            if (!polygon.m_count) {
                break;
            }
        }

        if (!polygon.m_count) {
            continue;
        }

        float d = 3.4028235e38f;

        if (ClipGenericPolygon(polygon, planes, planeIndex, &d, direction) && d <= *distance) {
            *distance = d;
            *facetIndex = i;
            found = 1;
        }
    }

    return found;
}

// VERIFY: d0f45239 manual
// OFFSET: 0x0075CD70
static void __stdcall DistanceToMovePyramid(World::FacetData* facets, const C3Vector& direction, float distance, const C3Vector& move, const C4Plane* volumePlanes, const C3Vector* volumePoints, const uint8_t* volumeFaces, uint32_t* facetIndex, C4Plane* hitPlanes, uint32_t* hitCount, float* hitDistance) {
    C4Plane extruded[4];

    for (uint32_t i = 0; i < 4; i++) {
        extruded[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        extruded[i].d = 0.0f;
    }

    for (uint32_t i = 0; i < 4; i++) {
        const C4Plane& plane = volumePlanes[5 + i];

        if (plane.n.z * move.z + plane.n.x * move.x + plane.n.y * move.y <= 0.0f) {
            continue;
        }

        if (!ExtrudeTriangle(volumePoints, &volumeFaces[20 + 3 * i], plane.n, extruded, move)) {
            continue;
        }

        float d = distance;

        if (!ClipMovementPyramid(facets, direction, extruded, 3, volumePlanes, 5 + i, &d, facetIndex)) {
            continue;
        }

        if (d >= *hitDistance - 0.0013888889f) {
            if (*hitDistance + 0.0013888889f > d) {
                hitPlanes[*hitCount] = plane;
                ++*hitCount;
            }
        } else {
            hitPlanes[0] = plane;
            *hitCount = 1;
        }

        if (d < *hitDistance) {
            *hitDistance = d;

            if (*hitCount > 1) {
                Swap(hitPlanes[*hitCount - 1], hitPlanes[0]);
            }
        }
    }
}

// VERIFY: 72d79c45 manual
// OFFSET: 0x0075D0A0
static int32_t WalkableFacetEnclosesPoint(uint32_t facet, const C3Vector& point) {
    C4Plane planes[4];

    for (uint32_t i = 0; i < 4; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    C3Vector up(0.0f, 0.0f, 1.0f);
    uint8_t indices[3] = { 0, 1, 2 };
    const CFacet& data = s_moveFacets.facets.m_data[facet];
    ExtrudeTriangle(data.v, indices, data.plane.n, planes, up);

    for (uint32_t i = 0; i < 3; i++) {
        if (planes[i].n.z * point.z + planes[i].n.x * point.x + planes[i].n.y * point.y + planes[i].d > 0.083333336f) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 2873d0d7 manual
// OFFSET: 0x0075D340
int32_t CMovement_C::IsSlopeFallable(uint32_t facet, const C3Vector& move) const {
    C3Vector point(this->m_position.x + move.x, this->m_position.y + move.y, this->m_position.z + move.z);
    float threshold = this->m_unit->IsClientControlled() ? 0.64278764f : 0.17364818f;
    return threshold < s_moveFacets.facets.m_data[facet].plane.n.z && WalkableFacetEnclosesPoint(facet, point);
}

// VERIFY: 8c383a35 manual
// OFFSET: 0x0075D4B0
static void ComputeSteepSurfacePushNormalHelper(uint32_t facet, const C3Vector& direction, C3Vector& edge, const C3Vector& point) {
    uint32_t next[3] = { 1, 2, 0 };
    float best = 3.4028235e38f;
    edge = C3Vector(0.0f, 0.0f, 0.0f);
    const CFacet& data = s_moveFacets.facets.m_data[facet];

    for (uint32_t i = 0; i < 3; i++) {
        const C3Vector& v = data.v[i];
        const C3Vector& w = data.v[next[i]];
        float ex = v.x - w.x;
        float ey = v.y - w.y;
        float ez = v.z - w.z;
        float length = sqrt(ez * ez + ey * ey + ex * ex);

        if (fabs(length) < 0.00000023841858f) {
            continue;
        }

        float scale = 1.0f / length;
        C3Vector n(ex * scale, ey * scale, scale * ez);
        float distSq;

        if (fabs(direction.z * n.z + direction.y * n.y + direction.x * n.x - 1.0f) >= 0.00000095367432f) {
            float cx = direction.z * n.y - direction.y * n.z;
            float cy = n.z * direction.x - direction.z * n.x;
            float cz = n.x * direction.y - n.y * direction.x;
            float d = -(point.z * cz + point.y * cy + point.x * cx);
            float dist = cz * v.z + (cx * v.x + cy * v.y) + d;
            distSq = dist * dist;
        } else {
            float t = (v.x - point.x) * direction.x + (v.y - point.y) * direction.y + (v.z - point.z) * direction.z;
            float dx = v.x - (direction.x * t + point.x);
            float dy = v.y - (direction.y * t + point.y);
            float dz = v.z - (t * direction.z + point.z);
            distSq = dz * dz + dy * dy + dx * dx;
        }

        if (distSq < best) {
            best = distSq;
            edge = n;
        }
    }
}

// VERIFY: 00078aa3 auto
// OFFSET: 0x0075D680
static void IntersectPlanes(const C4Plane& plane0, const C4Plane& plane1, const C4Plane& plane2, C3Vector& point) {
    C33Matrix m(plane0.n.x, plane0.n.y, plane0.n.z, plane1.n.x, plane1.n.y, plane1.n.z, plane2.n.x, plane2.n.y, plane2.n.z);
    float d0 = -plane0.d;
    float d1 = -plane1.d;
    float d2 = -plane2.d;
    C33Matrix inverse = m.Inverse(m.Determinant());
    point.x = inverse.a0 * d0 + inverse.a1 * d1 + inverse.a2 * d2;
    point.y = inverse.b2 * d2 + inverse.b1 * d1 + inverse.b0 * d0;
    point.z = d1 * inverse.c1 + d2 * inverse.c2 + d0 * inverse.c0;
}

// VERIFY: e400573b auto
// OFFSET: 0x0075DE40
static int32_t CheckFallingConditions(const C4Plane* planes, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (fabs(planes[i].n.z - 1.0f) < 0.00000095367432f) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 4a5e4d0d auto
// OFFSET: 0x0075DE80
static int32_t GetBoxPushNormal(const C4Plane* planes, uint32_t count, C3Vector& normal) {
    if (!count || count > 4) {
        return 0;
    }

    for (uint32_t i = 0; i < count; i++) {
        if (fabs(planes[i].n.z - -0.4756366f) >= 0.00000095367432f) {
            return 0;
        }
    }

    switch (count) {
        case 1:
            normal = planes[0].n;
            break;

        case 2:
            normal.x = (planes[1].n.x + planes[0].n.x) * 0.638556f;
            normal.y = (planes[1].n.y + planes[0].n.y) * 0.638556f;
            normal.z = 0.638556f * (planes[1].n.z + planes[0].n.z);
            break;

        case 3: {
            uint32_t flatY[3];
            uint32_t flatX[3];
            uint32_t flatYCount = 0;
            uint32_t flatXCount = 0;

            for (uint32_t i = 0; i < 3; i++) {
                if (fabs(planes[i].n.y) < 0.00000095367432f) {
                    flatY[flatYCount++] = i;
                } else if (fabs(planes[i].n.x) < 0.00000095367432f) {
                    flatX[flatXCount++] = i;
                }
            }

            normal = planes[flatYCount == 1 ? flatY[0] : flatX[0]].n;
            break;
        }

        case 4:
            normal.x = 0.0f;
            normal.y = 0.0f;
            normal.z = 0.0f;
            break;
    }

    return 1;
}

// VERIFY: a48843dc manual
// OFFSET: 0x0075E760
C3Vector CMovement_C::GetSteepSurfacePushNormal(uint32_t facet, const C4Plane* planes, uint32_t count) {
    if (count == 1) {
        if (this->PlaneIntersectsVolume(s_moveFacets.facets.m_data[facet].plane)) {
            return C3Vector(-planes[0].n.x, -planes[0].n.y, -planes[0].n.z);
        }
    } else if (count == 2) {
        float x = planes[1].n.z * planes[0].n.y - planes[0].n.z * planes[1].n.y;
        float y = planes[1].n.x * planes[0].n.z - planes[0].n.x * planes[1].n.z;
        float z = planes[0].n.x * planes[1].n.y - planes[1].n.x * planes[0].n.y;
        float scale = 1.0f / sqrt(y * y + x * x + z * z);
        C3Vector direction(x * scale, y * scale, z * scale);
        const CFacet& data = s_moveFacets.facets.m_data[facet];

        if (fabs(direction.z) >= 0.00000095367432f && CMath::fnotequal(direction.y * data.plane.n.y + direction.z * data.plane.n.z + direction.x * data.plane.n.x, 0.0f) && (fabs(direction.z - 1.0f) < 0.00000095367432f || !this->ComputeSteepSurfacePushNormalImpl(data.plane))) {
            C3Vector point;
            IntersectPlanes(s_moveFacets.facets.m_data[facet].plane, planes[0], planes[1], point);
            C3Vector edge;
            ComputeSteepSurfacePushNormalHelper(facet, direction, edge, point);
            C3Vector normal = C3Vector::Cross(direction, edge);
            float length = sqrt(normal.y * normal.y + normal.z * normal.z + normal.x * normal.x);

            if (fequal(length, 0.0f)) {
                return -planes[0].n;
            }

            normal /= length;

            if (planes[0].n.x * normal.x + normal.y * planes[0].n.y + planes[0].n.z * normal.z <= 0.0f) {
                return normal;
            }

            return C3Vector(-normal.x, -normal.y, -normal.z);
        }
    }

    return s_moveFacets.facets.m_data[facet].plane.n;
}

// VERIFY: 52bf037e auto
// OFFSET: 0x0075E9C0
C2Vector CMovement_C::CalcFallObstaclePush(const C3Vector& direction, float distance, float maxDistance, uint32_t facet, const C4Plane* planes, uint32_t count) {
    C3Vector normal;

    if (this->IsFacetWall(facet)) {
        normal = this->GetSteepSurfacePushNormal(facet, planes, count);
    } else {
        C3Vector point(direction.x * distance + this->m_position.x, direction.y * distance + this->m_position.y, distance * direction.z + this->m_position.z);
        normal = this->PushOffObstacleEdge(point, direction, facet);
    }

    C2Vector push(normal);
    float lengthSq = push.y * push.y + push.x * push.x;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        push.x = push.x * scale;
        push.y = push.y * scale;
    }

    float d = (-normal.x * direction.x + -normal.y * direction.y + -normal.z * direction.z) * (maxDistance - distance);
    float denom = normal.x * push.x + normal.y * push.y + normal.z * 0.0f;
    float scale = fabs(denom) < 0.00000023841858f ? d : d / denom;
    return C2Vector(push.x * (scale + 0.001f), push.y * (scale + 0.001f));
}

// VERIFY: 4eb4d8a8 auto
// OFFSET: 0x006E9270
void CMovement_C::SendFallReset(uint32_t time) {
    this->m_unit->OnCollideFallReset(time);
}

// VERIFY: 6248f542 manual
// OFFSET: 0x0075D1C0
uint32_t CMovement_C::UseWalkableRedirection(uint32_t facet, int32_t* cleared) {
    if (cleared) {
        *cleared = 0;
    }

    float threshold = this->m_unit->IsClientControlled() ? 0.64278764f : 0.17364818f;

    if (threshold < s_moveFacets.facets.m_data[facet].plane.n.z) {
        if (WalkableFacetEnclosesPoint(facet, this->m_position)) {
            this->m_flags &= ~MOVEMENTFLAG_SPLINE_ELEVATION;
        }

        return 1;
    }

    if (s_moveFacets.facets.m_data[facet].plane.n.z < 0.0f && this->IsFacetOverhead(facet)) {
        if (this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
            if (cleared) {
                *cleared = 1;
            }

            this->m_flags &= ~MOVEMENTFLAG_SPLINE_ELEVATION;
        }

        return -s_moveFacets.facets.m_data[facet].plane.n.z > 0.64278764f;
    }

    return this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION;
}

// VERIFY: e09f1b08 auto
// OFFSET: 0x0075D3C0
void CMovement_C::MoveSplineMoverWithoutCollision(const C3Vector& position, uint32_t time, uint32_t elapsed) {
    float dx = position.x - this->m_position.x;
    float dy = position.y - this->m_position.y;
    float dz = position.z - this->m_position.z;
    float lengthSq = dz * dz + dy * dy + dx * dx;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        dx = dx * scale;
        dy = dy * scale;
        dz = dz * scale;
    }

    float px = dy;
    float py = -dx;
    float lengthSq2D = py * py + dy * dy;

    if (lengthSq2D > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq2D);
        px = scale * dy;
        py = scale * py;
    }

    this->m_groundNormal.x = py * dz;
    this->m_groundNormal.y = -(dz * px);
    this->m_groundNormal.z = dy * px - dx * py;
    this->m_position = position;
    this->CallMoveEventHandlers(elapsed + time, 0, this->m_flags, this->m_flags2, 0, 0);
}

// VERIFY: d19c5dcd manual
// OFFSET: 0x0075EC10
static C3Vector __stdcall GetGroundNormal(const C4Plane* planes, uint32_t count) {
    ClipPolygon polygon;
    polygon.m_count = 3;
    C3Vector sum;
    uint32_t hits = 0;

    for (uint32_t i = 0; i < s_moveFacets.facets.m_count; i++) {
        const CFacet& facet = s_moveFacets.facets.m_data[i];

        if (facet.plane.n.z <= 0.017452406f) {
            continue;
        }

        memcpy(polygon.m_v, facet.v, sizeof(facet.v));
        polygon.m_count = 3;
        memset(polygon.m_flags, 0xFF, sizeof(int32_t) * 3);

        for (uint32_t j = 0; j < count; j++) {
            ClipPolygonToPlane(polygon, j, planes[j]);

            if (!polygon.m_count) {
                break;
            }
        }

        if (!polygon.m_count) {
            continue;
        }

        hits++;
        sum.x = s_moveFacets.facets.m_data[i].plane.n.x + sum.x;
        sum.y = s_moveFacets.facets.m_data[i].plane.n.y + sum.y;
        sum.z = s_moveFacets.facets.m_data[i].plane.n.z + sum.z;
    }

    if (!hits) {
        return C3Vector(0.0f, 0.0f, 1.0f);
    }

    float inverse = 1.0f / hits;
    float x = inverse * sum.x;
    float y = sum.y * inverse;
    float z = inverse * sum.z;
    float scale = 1.0f / sqrt(z * z + y * y + x * x);
    return C3Vector(x * scale, y * scale, z * scale);
}

// VERIFY: 0c0e2026 manual
// OFFSET: 0x0075EE60
void CMovement_C::GroundNormal() {
    C4Plane planes[6];
    planes[0].n = C3Vector(1.0f, 0.0f, 0.0f);
    planes[0].d = -this->m_position.x - this->m_collisionRadius;
    planes[1].n = C3Vector(-1.0f, 0.0f, 0.0f);
    planes[1].d = this->m_position.x - this->m_collisionRadius;
    planes[2].n = C3Vector(0.0f, 1.0f, 0.0f);
    planes[2].d = -this->m_position.y - this->m_collisionRadius;
    planes[3].n = C3Vector(0.0f, -1.0f, 0.0f);
    planes[3].d = this->m_position.y - this->m_collisionRadius;
    planes[4].n = C3Vector(0.0f, 0.0f, 1.0f);
    planes[4].d = -this->m_position.z - this->m_collisionHeight;
    planes[5].n = C3Vector(0.0f, 0.0f, -1.0f);
    planes[5].d = this->m_position.z;
    this->m_groundNormal = ::GetGroundNormal(planes, 6);
}

// VERIFY: bf3ee7a3 manual
// OFFSET: 0x0075F0A0
int32_t CMovement_C::ValidateTestVsFacetQuery(C3Vector move) const {
    C44Matrix transform;
    C3Vector position = this->m_position;

    if (this->m_transportGuid) {
        MovementGetTransportMtxX(this->m_transportGuid, &transform, *this->m_ownerGuid, __FILE__, __LINE__);
        position *= transform;
        C3Vector rotated;
        rotated.x = transform.c0 * move.z + transform.b0 * move.y + transform.a0 * move.x;
        rotated.y = move.z * transform.c1 + move.y * transform.b1 + transform.a1 * move.x;
        rotated.z = move.z * transform.c2 + move.y * transform.b2 + move.x * transform.a2;
        move = rotated;
    }

    CAaBox box;
    this->BuildCollisionBox(position, box);
    CAaBox moved(C3Vector(box.b.x + move.x, box.b.y + move.y, box.b.z + move.z), C3Vector(move.x + box.t.x, move.y + box.t.y, move.z + box.t.z));

    if (s_queryBox.ContainsPoint(moved.b) && s_queryBox.ContainsPoint(moved.t)) {
        return 1;
    }

    CAaBox query(C3Vector(moved.b.x - 0.16666667f, moved.b.y - 0.16666667f, moved.b.z - 0.16666667f), C3Vector(moved.t.x + 0.16666667f, moved.t.y + 0.16666667f, moved.t.z + 0.16666667f));
    query |= s_queryBox;

    if (!World::GetFacets(&box, &query, &s_moveFacets, this->GetFacetQueryFlags(), nullptr)) {
        return 0;
    }

    if (this->m_flags & MOVEMENTFLAG_SWIMMING) {
        World::GetFacets(&box, &query, &s_liquidFacets, 0x20000, nullptr);

        for (uint32_t i = 0; i < s_liquidFacets.facets.m_count; i++) {
            CFacet& facet = s_liquidFacets.facets.m_data[i];
            facet.plane.n.x = -facet.plane.n.x;
            facet.plane.n.y = -facet.plane.n.y;
            facet.plane.n.z = -facet.plane.n.z;
            facet.plane.d = -facet.plane.d;
        }
    } else {
        s_liquidFacets.facets.SetCount(0);
        s_liquidFacets.facetIds.m_count = 0;
    }

    s_queryBox = query;

    if (this->m_transportGuid) {
        C44Matrix inverse = transform.AffineInverse();

        for (uint32_t i = 0; i < s_moveFacets.facets.m_count; i++) {
            CFacet& facet = s_moveFacets.facets.m_data[i];
            facet.v[0] *= inverse;
            facet.v[1] *= inverse;
            facet.v[2] *= inverse;
            float x = inverse.b0 * facet.plane.n.y + inverse.c0 * facet.plane.n.z + facet.plane.n.x * inverse.a0;
            float y = facet.plane.n.x * inverse.a1 + inverse.b1 * facet.plane.n.y + inverse.c1 * facet.plane.n.z;
            float z = facet.plane.n.x * inverse.a2 + inverse.b2 * facet.plane.n.y + inverse.c2 * facet.plane.n.z;
            facet.plane.n.x = x;
            facet.plane.n.y = y;
            facet.plane.n.z = z;
            facet.plane.d = -(z * facet.v[0].z + y * facet.v[0].y + x * facet.v[0].x);
        }
    }

    return 1;
}

// VERIFY: db3a9a26 manual
// OFFSET: 0x0075F520
int32_t CMovement_C::GetCurrentHoverHeight(float* height, int32_t* noGround, uint64_t* facetId) const {
    C4Plane hitPlanes[7];

    for (uint32_t i = 0; i < 7; i++) {
        hitPlanes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        hitPlanes[i].d = 0.0f;
    }

    uint32_t hitCount = 0;
    C4Plane volumePlanes[9];

    for (uint32_t i = 0; i < 9; i++) {
        volumePlanes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        volumePlanes[i].d = 0.0f;
    }

    C3Vector volumePoints[9];
    uint8_t volumeFaces[32];

    if (noGround) {
        *noGround = 0;
    }

    if (facetId) {
        *facetId = 0;
    }

    float distance = this->m_hoverHeight + this->m_hoverHeight;
    *height = distance;
    C3Vector direction(0.0f, 0.0f, -1.0f);
    C3Vector move(0.0f, 0.0f, -distance);
    this->BuildCollisionVolume(this->m_position, this->m_collisionHeight, volumePlanes, volumePoints, volumeFaces);

    if (!this->ValidateTestVsFacetQuery(move)) {
        *height = 0.0f;
        return 0;
    }

    uint32_t index = s_moveFacets.facets.m_count;
    DistanceToMovePyramid(&s_moveFacets, direction, distance, move, volumePlanes, volumePoints, volumeFaces, &index, hitPlanes, &hitCount, height);

    if (index < s_moveFacets.facets.m_count) {
        if (facetId) {
            *facetId = s_moveFacets.facetIds.m_data[index];
        }

        if (this->m_hoverHeight > *height && IsTooSteepForUnit(this->m_unit, index)) {
            *height = this->m_hoverHeight;
        }
    } else if (noGround) {
        *noGround = 1;
    }

    if (*height < 0.0013888889f) {
        *height = 0.0f;
    }

    return 1;
}

// VERIFY: 03ee7346 manual
// OFFSET: 0x0075F9D0
int32_t CMovement_C::DistanceToMove(World::FacetData* facets, const C3Vector& direction, float distance, uint32_t* facetIndex, C4Plane* planes, uint32_t* planeCount, float* moveDistance, const C3Vector* position) const {
    *planeCount = 0;

    if (fabs(distance) < 0.00000095367432f) {
        *moveDistance = 0.0f;
        *facetIndex = facets->facets.m_count;
        return 1;
    }

    C4Plane volumePlanes[9];

    for (uint32_t i = 0; i < 9; i++) {
        volumePlanes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        volumePlanes[i].d = 0.0f;
    }

    C3Vector volumePoints[9];
    uint8_t volumeFaces[32];
    float height = this->m_collisionHeight;
    C3Vector start = position ? *position : this->m_position;

    if ((this->m_flags & MOVEMENTFLAG_SWIMMING) && facets == &s_liquidFacets) {
        height = height * 0.75f;
    }

    float scaled = distance;

    if (scaled <= 0.027777778f) {
        scaled = 0.027777778f;
    }

    C3Vector move(scaled * direction.x, direction.y * scaled, scaled * direction.z);
    this->BuildCollisionVolume(start, height, volumePlanes, volumePoints, volumeFaces);

    if (!position && !this->ValidateTestVsFacetQuery(move)) {
        *moveDistance = 0.0f;
        return 0;
    }

    *facetIndex = facets->facets.m_count;
    float hitDistance = distance;
    DistanceToMovePyramid(facets, direction, distance, move, volumePlanes, volumePoints, volumeFaces, facetIndex, planes, planeCount, &hitDistance);

    C4Plane sidePlanes[5];

    for (uint32_t i = 0; i < 5; i++) {
        sidePlanes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        sidePlanes[i].d = 0.0f;
    }

    for (uint32_t i = 0; i < 5; i++) {
        const C4Plane& plane = volumePlanes[i];

        if (plane.n.y * move.y + move.z * plane.n.z + move.x * plane.n.x <= 0.0f) {
            continue;
        }

        if (!ComputeDistanceToMoveImpl(volumePoints, plane.n, move, sidePlanes, &volumeFaces[4 * i])) {
            continue;
        }

        float d = distance;

        if (!ClipMovementPyramid(facets, direction, sidePlanes, 4, volumePlanes, i, &d, facetIndex)) {
            continue;
        }

        if (d >= hitDistance - 0.0013888889f) {
            if (hitDistance + 0.0013888889f > d) {
                planes[*planeCount] = plane;
                ++*planeCount;
            }
        } else {
            planes[0] = plane;
            *planeCount = 1;
        }

        if (d < hitDistance) {
            hitDistance = d;

            if (*planeCount > 1) {
                Swap(planes[*planeCount - 1], planes[0]);
            }
        }
    }

    *moveDistance = hitDistance >= 0.0013888889f ? hitDistance : 0.0f;
    return 1;
}

// VERIFY: 802c4c4c manual
// OFFSET: 0x0075FF90
int32_t CMovement_C::GetMoveFacets(float distance, uint32_t elapsed, C3Vector direction) {
    C44Matrix transform;
    C3Vector position = this->m_position;

    if (this->m_transportGuid) {
        MovementGetTransportMtxX(this->m_transportGuid, &transform, *this->m_ownerGuid, __FILE__, __LINE__);
        position *= transform;
        C3Vector rotated;
        rotated.x = transform.c0 * direction.z + transform.b0 * direction.y + transform.a0 * direction.x;
        rotated.y = direction.z * transform.c1 + direction.y * transform.b1 + transform.a1 * direction.x;
        rotated.z = direction.z * transform.c2 + direction.y * transform.b2 + direction.x * transform.a2;
        direction = rotated;
    }

    World::IsValidPosition(position.x, position.y, position.z, 0.0f);
    CAaBox box;
    this->BuildCollisionBox(position, box);
    s_queryBox = box;
    C3Vector move(direction.x * distance, direction.y * distance, direction.z * distance);

    if (this->m_flags & MOVEMENTFLAG_FALLING) {
        move.z = move.z - this->RelDistanceFallen(elapsed + this->m_fallTimeMs);
    }

    uint32_t flags = this->m_flags;

    if (flags & MOVEMENTFLAG_FALLING) {
        CAaBox moved(C3Vector(move.x + s_queryBox.b.x, s_queryBox.b.y + move.y, s_queryBox.b.z + move.z), C3Vector(move.x + s_queryBox.t.x, move.y + s_queryBox.t.y, move.z + s_queryBox.t.z));
        s_queryBox |= moved;
        float spread = move.z * -1.1866661f;

        if (spread > 0.0f) {
            s_queryBox.b.x = s_queryBox.b.x - spread;
            s_queryBox.t.x = s_queryBox.t.x + spread;
            s_queryBox.b.y = s_queryBox.b.y - spread;
            s_queryBox.t.y = spread + s_queryBox.t.y;
        }
    } else if (flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) {
        float half = distance * 0.5f;
        float cx = direction.x * half + position.x;
        float cy = direction.y * half + position.y;
        float cz = direction.z * half + position.z;
        float radius = this->m_collisionRadius * 1.4142135f;
        float height = radius;

        if (radius <= this->m_collisionHeight) {
            height = this->m_collisionHeight;
        }

        CAaBox moved(C3Vector(cx - half - radius, cy - half - radius, cz - half), C3Vector(cx + half + radius, cy + half + radius, half + cz + height));
        s_queryBox |= moved;
    } else {
        float reach = this->m_collisionRadius + 0.0013888889f;

        if (reach < this->GetStepUpHeight() * 1.1917536f) {
            reach = this->GetStepUpHeight() * 1.1917536f;
        }

        float extent = reach + distance;
        float ex = direction.x * extent;
        float ey = direction.y * extent;
        float ez = extent * direction.z;
        CAaBox moved(C3Vector(ex + s_queryBox.b.x, s_queryBox.b.y + ey, s_queryBox.b.z + ez), C3Vector(ex + s_queryBox.t.x, ey + s_queryBox.t.y, ez + s_queryBox.t.z));
        s_queryBox |= moved;

        float half = distance * 0.5f;
        float cx = direction.x * half + position.x;
        float cy = direction.y * half + position.y;
        float cz = direction.z * half + position.z;
        float r = half + this->m_collisionRadius * 1.4142135f;
        CAaBox around(C3Vector(cx - r, cy - r, cz), C3Vector(cx + r, r + cy, cz));
        s_queryBox |= around;

        float up = distance;
        float stepUp = this->GetStepUpHeight();

        if (distance < stepUp + stepUp) {
            float step = this->GetStepUpHeight();
            up = step + step;
        }

        s_queryBox.t.z = up + s_queryBox.t.z;
        s_queryBox.b.z = s_queryBox.b.z - (this->GetStepUpHeight() + distance * 1.1917536f);
    }

    s_queryBox.b.x = s_queryBox.b.x - 0.0013888889f;
    s_queryBox.b.y = s_queryBox.b.y - 0.0013888889f;
    s_queryBox.b.z = s_queryBox.b.z - 0.0013888889f;
    s_queryBox.t.x = s_queryBox.t.x + 0.0013888889f;
    s_queryBox.t.y = s_queryBox.t.y + 0.0013888889f;
    s_queryBox.t.z = s_queryBox.t.z + 0.0013888889f;

    if (!World::GetFacets(&box, &s_queryBox, &s_moveFacets, this->GetFacetQueryFlags(), nullptr)) {
        return 0;
    }

    if (this->m_flags & MOVEMENTFLAG_SWIMMING) {
        World::GetFacets(&box, &s_queryBox, &s_liquidFacets, 0x20000, nullptr);

        for (uint32_t i = 0; i < s_liquidFacets.facets.m_count; i++) {
            CFacet& facet = s_liquidFacets.facets.m_data[i];
            facet.plane.n.x = -facet.plane.n.x;
            facet.plane.n.y = -facet.plane.n.y;
            facet.plane.n.z = -facet.plane.n.z;
            facet.plane.d = -facet.plane.d;
        }
    } else {
        s_liquidFacets.facets.SetCount(0);
        s_liquidFacets.facetIds.m_count = 0;
    }

    if (this->m_transportGuid) {
        C44Matrix inverse = transform.AffineInverse();

        for (uint32_t i = 0; i < s_moveFacets.facets.m_count; i++) {
            CFacet& facet = s_moveFacets.facets.m_data[i];
            facet.v[0] *= inverse;
            facet.v[1] *= inverse;
            facet.v[2] *= inverse;
            float x = inverse.b0 * facet.plane.n.y + inverse.c0 * facet.plane.n.z + inverse.a0 * facet.plane.n.x;
            float y = inverse.b1 * facet.plane.n.y + inverse.c1 * facet.plane.n.z + inverse.a1 * facet.plane.n.x;
            float z = inverse.b2 * facet.plane.n.y + inverse.c2 * facet.plane.n.z + inverse.a2 * facet.plane.n.x;
            facet.plane.n.x = x;
            facet.plane.n.y = y;
            facet.plane.n.z = z;
            facet.plane.d = -(y * facet.v[0].y + z * facet.v[0].z + x * facet.v[0].x);
        }
    }

    return 1;
}

// VERIFY: 19cf6d99 auto
// OFFSET: 0x006EC7B0
int32_t CMovement_C::SetTransport(WOWGUID transport) {
    if (this->m_unit->m_obj->m_guid != CGUnit_C::s_activeMover) {
        return 0;
    }

    if (this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        return 0;
    }

    if (!transport && this->m_transportGuid && MovementInsideTransport(this->m_transportGuid, this->m_position)) {
        return 0;
    }

    return this->ForceSetTransportInt(transport, 255, 0);
}

// VERIFY: 394468db manual
// OFFSET: 0x00760B40
uint32_t CMovement_C::Swim(uint32_t time, uint32_t elapsed, float distance, C3Vector direction) {
    if (this->m_transportGuid && this->m_link.IsLinked() && this->SetTransport(WOWGUID())) {
        return 0;
    }

    if (!elapsed) {
        return 0;
    }

    if (fabs(distance) < 0.00000095367432f) {
        return elapsed;
    }

    C4Plane planes[7];

    for (uint32_t i = 0; i < 7; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    uint32_t facetIndex;
    float total = static_cast<float>(elapsed);
    float consumed = 0.0f;
    float remaining = distance;
    int32_t stuck = 0;
    int32_t hitSolid = 0;
    int32_t moved = 0;
    uint32_t planeCount;
    float moveDist;

    if (!this->DistanceToMove(&s_moveFacets, direction, distance, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
        return elapsed;
    }

    while (true) {
        World::FacetData* facets = &s_moveFacets;

        if (facetIndex < s_moveFacets.facets.m_count) {
            hitSolid = 1;
        } else {
            if (!hitSolid && !this->IsGravityDisabled()) {
                if (!this->DistanceToMove(&s_liquidFacets, direction, remaining, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
                    return elapsed;
                }

                facets = &s_liquidFacets;
            }

            if (facetIndex == facets->facets.m_count) {
                C3Vector move(direction.x * moveDist, direction.y * moveDist, moveDist * direction.z);
                this->m_position += move;
                goto done;
            }
        }

        {
            this->m_position.x = direction.x * moveDist + this->m_position.x;
            this->m_position.y = direction.y * moveDist + this->m_position.y;
            this->m_position.z = direction.z * moveDist + this->m_position.z;
            float progress = (total - consumed) * (moveDist / remaining);
            float left = remaining - moveDist;

            if (progress + 0.00000095367432f <= 1.0f) {
                if (++stuck > 5) {
                    goto anchors;
                }
            } else {
                stuck = 1;
                consumed = consumed + progress;
            }

            if (!hitSolid && (this->m_flags & MOVEMENTFLAG_ASCENDING)) {
                if (CMath::fnotequal(total, consumed)) {
                    moved = 1;
                }

                this->Jump(1);
                goto done;
            }

            if (total - consumed < 1.0f) {
                if (CMath::fnotequal(total, consumed)) {
                    goto anchors;
                }

                goto done;
            }

            const CFacet& facet = facets->facets.m_data[facetIndex];
            moved = 1;
            float push = (-facet.plane.n.y * direction.y + -facet.plane.n.z * direction.z + -facet.plane.n.x * direction.x) * left + 0.001f;
            float x = direction.x * left + facet.plane.n.x * push;
            float y = facet.plane.n.y * push + direction.y * left;
            float z = facet.plane.n.z * push + left * direction.z;
            direction = C3Vector(x, y, z);
            remaining = sqrt(y * y + z * z + x * x);

            if (fabs(remaining) < 0.00000095367432f) {
                goto anchors;
            }

            float scale = 1.0f / remaining;
            direction.x = x * scale;
            direction.y = scale * direction.y;
            direction.z = scale * direction.z;

            if (!this->DistanceToMove(&s_moveFacets, direction, remaining, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
                return elapsed;
            }
        }
    }

done:
    if (!moved) {
        return elapsed;
    }

anchors:
    this->UpdateAnchors(0);
    return elapsed;
}

// VERIFY: beee8c35 manual
// OFFSET: 0x00760FC0
int32_t CMovement_C::TryFallingDown(float timeFallen, C3Vector* move, float* distance, C2Vector* push, uint64_t* facetId, int32_t* onSlope, int32_t* landed) {
    *landed = 0;
    *onSlope = 0;
    float length = *distance;

    if (fabs(length) < 0.00000023841858f) {
        *facetId = 0;
        return 0;
    }

    C4Plane planes[7];

    for (uint32_t i = 0; i < 7; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    C3Vector direction(move->x * (1.0f / length), move->y * (1.0f / length), 1.0f / length * move->z);
    uint32_t facetIndex;
    uint32_t planeCount;

    if (!this->DistanceToMove(&s_moveFacets, direction, length, &facetIndex, planes, &planeCount, distance, nullptr)) {
        move->x = 0.0f;
        move->y = 0.0f;
        move->z = 0.0f;
        return 2;
    }

    if (facetIndex == s_moveFacets.facets.m_count) {
        *facetId = 0;
        return 0;
    }

    float d = *distance;
    move->x = direction.x * d;
    move->y = direction.y * d;
    move->z = d * direction.z;
    *facetId = s_moveFacets.facetIds.m_data[facetIndex];

    if (this->IsSlopeFallable(facetIndex, *move)) {
        *onSlope = 1;
        return 1;
    }

    if (direction.z > 0.0f) {
        float peak = this->TimeToJumpPeak() - timeFallen;

        if (peak >= 0.0f && peak * this->m_currentSpeed >= *distance && CheckFallingConditions(planes, planeCount)) {
            *landed = 1;
            *facetId = 0;
            return 1;
        }
    }

    *push = this->CalcFallObstaclePush(direction, *distance, length, facetIndex, planes, planeCount);
    return 1;
}

// VERIFY: 8938b491 manual
// OFFSET: 0x007612B0
uint32_t CMovement_C::FallDown(uint32_t time, uint32_t elapsed, C3Vector move, int32_t notify) {
    float timeFallen = 0.0f;
    float totalTime = elapsed * 0.001f;
    float startTime = 0.001f * static_cast<float>(this->m_fallTimeMs);
    int32_t stuck = 0;
    uint64_t transport = 0;
    C2Vector push(0.0f, 0.0f);
    int32_t slid = 0;
    int32_t flattened = 0;
    C2Vector move2D(move);
    C2Vector savedDir2D = this->m_moveDir2D;
    C3Vector savedDir = this->m_moveDir;
    int32_t landed = 0;
    float length = sqrt(move.y * move.y + move.x * move.x + move.z * move.z);
    float length2D = sqrt(move2D.y * move2D.y + move2D.x * move2D.x);
    float savedSpeed = this->m_currentSpeed;
    float vertical = fabs(move.z);
    C2Vector start(this->m_position);
    int32_t iterations = 0;
    float startLength2D = length2D;
    float endTime;
    int32_t onSlope;

    while (true) {
        if (fabs(length) < 0.00000095367432f) {
            endTime = totalTime;
            goto finish;
        }

        {
            float distance = length;
            float scale = 1.0f / length;
            C3Vector direction(move.x * scale, move.y * scale, move.z * scale);
            float fallTime = startTime + timeFallen;
            int32_t result = this->TryFallingDown(fallTime, &move, &distance, &push, &transport, &onSlope, &landed);

            if (result == 2) {
                uint32_t skip = static_cast<uint32_t>((totalTime - timeFallen) * 1000.0f);
                this->m_anchorElapsedMs -= skip;
                this->m_fallTimeMs -= skip;
                this->SkipTime(skip);
                endTime = totalTime;
                goto finish;
            }

            float dt = result ? this->GetTimeJustFallen(totalTime - timeFallen, fallTime, &move, &distance, landed, length2D) : totalTime - timeFallen;
            this->m_position.x = this->m_position.x + move.x;
            this->m_position.y = this->m_position.y + move.y;
            this->m_position.z = this->m_position.z + move.z;
            timeFallen = timeFallen + dt;

            if (landed) {
                this->m_flags |= MOVEMENTFLAG_FALLING_FAR;
                endTime = timeFallen;
                this->m_fallTimeMs = 0;
                this->m_fallVelocity = 0.0f;
                this->m_fallStartZ = this->m_position.z;
                goto finish;
            }

            if (onSlope) {
                goto stop;
            }

            if (timeFallen + 0.00050000002f >= totalTime) {
                endTime = totalTime;
                goto finish;
            }

            slid = 1;
            float left = length - distance;

            if (dt > 0.00050000002f) {
                stuck = 1;
            } else if (++stuck > 5) {
                if (flattened || !CMath::fnotequal(length2D, 0.0f)) {
                    goto stop;
                }

                stuck = 0;
                iterations++;
                this->m_currentSpeed = 0.0f;
                flattened = 1;
                move.x = 0.0f;
                move.y = 0.0f;
                move.z = direction.z * left;
                length = fabs(move.z);
                continue;
            }

            move.x = direction.x * left + push.x;
            move.y = direction.y * left + push.y;
            move.z = left * direction.z;

            if (iterations) {
                C2Vector position(this->m_position);
                float tx = position.x - start.x;
                float ty = position.y - start.y;
                C2Vector next(move);
                float dy = next.y + ty;
                float dx = next.x + tx;
                float distSq = dx * dx + dy * dy;

                if ((startLength2D + 0.00000095367432f) * (startLength2D + 0.00000095367432f) < distSq && vertical * 1.1866661f * (vertical * 1.1866661f) < distSq) {
                    goto stop;
                }
            }

            move2D = C2Vector(move);
            length2D = sqrt(move2D.y * move2D.y + move2D.x * move2D.x);

            if (fabs(length2D) < 0.00000095367432f) {
                this->m_currentSpeed = 0.0f;
            } else {
                float inverse = 1.0f / length2D;
                move2D.x = move2D.x * inverse;
                move2D.y = move2D.y * inverse;
                float speed = (move2D.x * this->m_moveDir2D.x + move2D.y * this->m_moveDir2D.y) * this->m_currentSpeed;
                this->m_currentSpeed = speed;

                if (!(speed > 0.0f) && !(0.0f == speed)) {
                    this->m_currentSpeed = 0.0f;
                }
            }

            iterations++;
            this->m_moveDir2D = move2D;
            length = sqrt(move.y * move.y + move.x * move.x + move.z * move.z);
            this->m_moveDir = C3Vector(this->m_moveDir2D.x, this->m_moveDir2D.y, 0.0f);
        }
    }

stop:
    this->StopFalling();
    endTime = timeFallen;

finish:
    uint32_t used = static_cast<uint32_t>(endTime * 1000.0f);

    if (!landed) {
        this->m_fallTimeMs += used;
    }

    if (notify) {
        if (landed) {
            this->SendFallReset(used + time);
        }

        uint32_t flags = this->m_flags;

        if (flags & MOVEMENTFLAG_FALLING) {
            if (!(flags & (MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_SAFE_FALL))) {
                if (0.0f == this->m_fallVelocity) {
                    if (this->m_fallTimeMs >= 500) {
                        this->m_flags = flags | MOVEMENTFLAG_FALLING_FAR;
                    }
                } else if (!(this->m_fallStartZ - 0.11111111f < this->m_position.z)) {
                    this->m_flags = flags | MOVEMENTFLAG_FALLING_FAR;
                }
            }

            if ((this->m_flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_fallVelocity && (this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT))) {
                this->m_currentSpeed = savedSpeed;
                this->m_moveDir2D = savedDir2D;
                this->m_moveDir = savedDir;
            }

            if (transport) {
                WOWGUID guid;
                guid.m_guid = transport;
                this->SetTransport(guid);
            }
        } else {
            if (transport) {
                WOWGUID guid;
                guid.m_guid = transport;
                this->SetTransport(guid);
            }

            if (this->HandlePendingActions()) {
                return used;
            }
        }
    }

    if (slid) {
        this->UpdateAnchors(0);
    }

    return used;
}

// VERIFY: 730a0c34 auto
// OFFSET: 0x007618B0
uint32_t CMovement_C::Fall(uint32_t time, uint32_t elapsed, float distance, const C2Vector& direction) {
    if ((this->m_transportGuid && this->m_link.IsLinked() && this->SetTransport(WOWGUID())) || !elapsed) {
        return 0;
    }

    float fallen = this->RelDistanceFallen(elapsed + this->m_fallTimeMs);
    C3Vector move(direction.x * distance, distance * direction.y, -fallen);

    if (fabs(sqrt(move.y * move.y + move.z * move.z + move.x * move.x)) < 0.00000023841858f) {
        this->m_fallTimeMs += elapsed;
        return elapsed;
    }

    uint32_t result;

    if (s_moveFacets.facets.m_count) {
        result = this->FallDown(time, elapsed, move, 1);
    } else {
        this->m_position.x = move.x + this->m_position.x;
        this->m_position.y = move.y + this->m_position.y;
        this->m_position.z = move.z + this->m_position.z;
        this->m_fallTimeMs += elapsed;
        this->UpdateFallingFar();
        result = elapsed;
    }

    this->m_groundNormal = C3Vector(0.0f, 0.0f, 1.0f);
    return result;
}

// VERIFY: 1467b615 auto
// OFFSET: 0x007619C0
int32_t CMovement_C::WillPassObstacle(const C2Vector& direction, const C3Vector& position, float height) {
    uint32_t time = static_cast<uint32_t>(sqrt((height + height) * 0.051837362f) * 1000.0f);
    MoveState state;
    this->SaveMoveState(state);
    this->StartFalling(0.0f);
    float fallen = this->RelDistanceFallen(time);
    C3Vector move(this->m_moveDir2D.x, this->m_moveDir2D.y, -fallen);

    for (uint32_t t = 0; t < time; t += this->FallDown(0, time - t, move, 0)) {
        if (!(this->m_flags & MOVEMENTFLAG_FALLING)) {
            break;
        }
    }

    this->RestoreMoveState(state);
    C2Vector from(position);
    C2Vector to(this->m_position);
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float length = sqrt(dy * dy + dx * dx);
    return length >= this->m_collisionRadius && dx * (1.0f / length) * direction.x + dy * (1.0f / length) * direction.y > 0.98480773f;
}

// VERIFY: 095a88eb manual
// OFFSET: 0x00761B00
int32_t CMovement_C::AttemptStepUp(const C2Vector& direction, C3Vector normal) {
    C3Vector saved = this->m_position;
    C4Plane planes[7];

    for (uint32_t i = 0; i < 7; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    C2Vector move = direction;
    float reach = this->m_collisionRadius + 0.0013888889f;

    if (reach < this->GetStepUpHeight() * 1.1917536f) {
        reach = this->GetStepUpHeight() * 1.1917536f;
    }

    uint32_t facetIndex;
    uint32_t planeCount;
    float moveDist;

    if (normal.z <= 0.64278764f && normal.z >= 0.0f) {
        float scale = 1.0f / sqrt(normal.x * normal.x + normal.y * normal.y);
        move.x = -(normal.x * scale);
        move.y = -(normal.y * scale);
        C3Vector probe(move.x, move.y, 0.0f);

        if (!this->DistanceToMove(&s_moveFacets, probe, reach, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
            return 0;
        }

        if (facetIndex == s_moveFacets.facets.m_count || s_moveFacets.facets.m_data[facetIndex].plane.n != normal) {
            move = direction;
        }
    }

    C3Vector up(0.0f, 0.0f, 1.0f);
    float upDist;

    if (!this->DistanceToMove(&s_moveFacets, up, this->GetRemainingStepUpHeight(), &facetIndex, planes, &planeCount, &upDist, nullptr)) {
        return 0;
    }

    float rise = upDist;

    if (this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
        rise = this->m_position.z - this->m_stepUpStartZ + upDist;
    }

    if (fabs(rise) < 0.00000023841858f) {
        this->m_flags &= ~MOVEMENTFLAG_SPLINE_ELEVATION;
        return 1;
    }

    this->m_position.z = upDist + this->m_position.z;
    C3Vector forward(move.x, move.y, 0.0f);

    if (!this->DistanceToMove(&s_moveFacets, forward, reach, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
        return 0;
    }

    this->m_position.x = move.x * moveDist + this->m_position.x;
    this->m_position.y = move.y * moveDist + this->m_position.y;

    int32_t cleared;

    if (CMath::fnotequal(moveDist, reach) && this->UseWalkableRedirection(facetIndex, &cleared)) {
        float remaining = reach - moveDist;
        C3Vector boxNormal;
        int32_t hasBox = GetBoxPushNormal(planes, planeCount, boxNormal);
        C3Vector moveDir(move.x, move.y, 0.0f);
        C3Vector push = this->CalcRunWalkWalkableObstaclePush(moveDir, &remaining, s_moveFacets.facets.m_data[facetIndex].plane.n, hasBox, boxNormal);
        C3Vector slide(move.x * remaining + push.x, remaining * move.y + push.y, push.z);
        float length = sqrt(slide.x * slide.x + slide.y * slide.y + push.z * push.z);

        if (fnotequal(length, 0.0f)) {
            slide /= length;
            float slideDist;

            if (!this->DistanceToMove(&s_moveFacets, slide, length, &facetIndex, planes, &planeCount, &slideDist, nullptr)) {
                return 0;
            }

            slide *= slideDist;
            C2Vector flat(slide);
            moveDist = sqrt(flat.y * flat.y + flat.x * flat.x) + moveDist;
            this->m_position += slide;
            upDist = slide.z + upDist;
        }
    }

    if (facetIndex != s_moveFacets.facets.m_count && this->m_collisionRadius >= moveDist) {
        this->m_flags &= ~MOVEMENTFLAG_SPLINE_ELEVATION;
        this->m_position = saved;
        return 1;
    }

    C3Vector down(0.0f, 0.0f, -1.0f);
    float downDist;

    if (!this->DistanceToMove(&s_moveFacets, down, upDist, &facetIndex, planes, &planeCount, &downDist, nullptr)) {
        return 0;
    }

    this->m_position.z = this->m_position.z - downDist;

    if (facetIndex == s_moveFacets.facets.m_count || !IsSurfaceTooSteep(facetIndex) || this->WillPassObstacle(move, saved, upDist - downDist)) {
        this->m_position = saved;

        if (!(this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION)) {
            this->m_stepUpStartZ = saved.z;
        }

        this->m_flags |= MOVEMENTFLAG_SPLINE_ELEVATION;
        return 1;
    }

    this->m_flags &= ~MOVEMENTFLAG_SPLINE_ELEVATION;
    this->m_position = saved;
    return 1;
}

// VERIFY: cc14d44a manual
// OFFSET: 0x007620F0
uint32_t CMovement_C::TraceSurface(uint32_t time, uint32_t elapsed, float distance, const C2Vector& direction) {
    if (fabs(distance) < 0.00000095367432f) {
        return elapsed;
    }

    if (!s_moveFacets.facets.m_count) {
        return this->StartFalling(0.0f) ? 0 : elapsed;
    }

    C4Plane planes[7];

    for (uint32_t i = 0; i < 7; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    float total = static_cast<float>(elapsed);
    float timeLeft = total;
    float timeUsed = 0.0f;
    int32_t stuck = 0;
    int32_t cleared = 0;
    int32_t moved = 0;
    float remaining = distance;
    float length2D = distance;
    C2Vector dir2D = direction;
    float climbBudget = distance / this->m_collisionRadius;
    C3Vector moveDir(direction.x, direction.y, 0.0f);
    uint32_t facetIndex;
    uint32_t planeCount;
    float moveDist;
    WOWGUID transport;

    if (!this->DistanceToMove(&s_moveFacets, moveDir, distance, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
        return elapsed;
    }

    while (true) {
        C3Vector step(moveDir.x * moveDist, moveDir.y * moveDist, moveDir.z * moveDist);

        if (climbBudget >= step.z) {
            climbBudget = climbBudget - step.z;
        } else {
            moved = 1;
            float scale = climbBudget / step.z;
            step.x = step.x * scale;
            step.y = step.y * scale;
            step.z = step.z * scale;
            moveDist = moveDist * scale;
            climbBudget = 0.0f;
        }

        this->m_position.x = step.x + this->m_position.x;
        this->m_position.y = step.y + this->m_position.y;
        this->m_position.z = step.z + this->m_position.z;

        if (facetIndex >= s_moveFacets.facets.m_count) {
            timeUsed = timeLeft + timeUsed;
            float left = total - timeUsed;

            if (left >= 1.0f) {
                timeLeft = left;
                dir2D = direction;
                remaining = left / total * distance;
                moveDir = C3Vector(direction.x, direction.y, 0.0f);
                goto next;
            }

            C3Vector down(0.0f, 0.0f, -1.0f);
            float downDist;

            if (!this->DistanceToMove(&s_moveFacets, down, distance * 1.849399f, &facetIndex, planes, &planeCount, &downDist, nullptr)) {
                return elapsed;
            }

            this->m_position.z = this->m_position.z - downDist;

            if (facetIndex == s_moveFacets.facets.m_count) {
                transport = WOWGUID();

                if (this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
                    goto finish;
                }
            } else {
                transport.m_guid = s_moveFacets.facetIds.m_data[facetIndex];

                if (!IsSurfaceTooSteep(facetIndex)) {
                    this->m_flags &= ~MOVEMENTFLAG_SPLINE_ELEVATION;
                    goto finish;
                }

                const CFacet& facet = s_moveFacets.facets.m_data[facetIndex];

                if ((this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) && facet.plane.n.y * dir2D.y + facet.plane.n.x * dir2D.x <= -0.0000099999997f) {
                    goto finish;
                }
            }

            goto fall;
        }

        {
            transport.m_guid = s_moveFacets.facetIds.m_data[facetIndex];
            float timeStep = moveDist / remaining * timeLeft;
            remaining = remaining - moveDist;
            C2Vector flat(step);
            length2D = length2D - sqrt(flat.y * flat.y + flat.x * flat.x);

            if (timeStep < 1.0f) {
                if (++stuck > 5) {
                    moved = 1;
                    goto finish;
                }
            } else {
                stuck = 1;
            }

            timeUsed = timeUsed + timeStep;
            timeLeft = timeLeft - timeStep;

            if (total - timeUsed < 1.0f) {
                if (CMath::fnotequal(total, timeUsed)) {
                    moved = 1;
                }

                goto finish;
            }

            C3Vector normal = s_moveFacets.facets.m_data[facetIndex].plane.n;
            C3Vector push;
            C3Vector boxNormal;
            int32_t hasBox;

            if (this->UseWalkableRedirection(facetIndex, &cleared)) {
                hasBox = GetBoxPushNormal(planes, planeCount, boxNormal);
            } else {
                if (cleared) {
                    goto fall;
                }

                moved = 1;
                hasBox = GetBoxPushNormal(planes, planeCount, boxNormal);
                uint32_t wasStepping = this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION;

                if (!this->AttemptStepUp(dir2D, normal)) {
                    return elapsed;
                }

                if (!(this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION)) {
                    if (wasStepping) {
                        goto fall;
                    }

                    push = this->CalcRunWalkBlockingObstaclePush(moveDir, remaining, length2D, normal);
                    goto slide;
                }
            }

            {
                float before = remaining;
                push = this->CalcRunWalkWalkableObstaclePush(moveDir, &remaining, normal, hasBox, boxNormal);
                timeLeft = remaining / before * timeLeft;
            }

        slide:

            C3Vector next(moveDir.x * remaining + push.x, moveDir.y * remaining + push.y, remaining * moveDir.z + push.z);
            moveDir = next;
            remaining = sqrt(next.y * next.y + next.z * next.z + next.x * next.x);

            if (remaining < 0.001f) {
                moved = 1;
                goto finish;
            }

            dir2D = C2Vector(moveDir);
            float scale = 1.0f / remaining;
            moveDir.x = moveDir.x * scale;
            moveDir.y = moveDir.y * scale;
            moveDir.z = scale * moveDir.z;
            float previous = length2D;
            length2D = sqrt(dir2D.y * dir2D.y + dir2D.x * dir2D.x);

            if (fabs(length2D) >= 0.00000023841858f) {
                float inverse = 1.0f / length2D;
                dir2D.x = dir2D.x * inverse;
                dir2D.y = dir2D.y * inverse;
            }

            if (previous - length2D > 0.00000095367432f) {
                moved = 1;
            }

            if ((this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) && next.z > 0.00000095367432f) {
                float top = next.z + this->m_position.z;

                if (this->GetStepUpHeight() + this->m_stepUpStartZ < top) {
                    float allowed = this->GetStepUpHeight() + this->m_stepUpStartZ - this->m_position.z;
                    remaining = allowed / next.z * remaining;

                    if (CMath::fnotequal(allowed, next.z)) {
                        moved = 1;
                    }

                    if (remaining < 0.001f) {
                        goto fall;
                    }
                }
            }
        }

    next:
        if (!this->DistanceToMove(&s_moveFacets, moveDir, remaining, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
            return elapsed;
        }
    }

finish:
    if (cleared) {
        goto fall;
    }

    if (facetIndex >= s_moveFacets.facets.m_count) {
        if (this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
            return elapsed;
        }

        goto fall;
    }

    if (moved) {
        this->UpdateAnchors(0);
    }

    this->SetTransport(transport);
    return elapsed;

fall:
    this->StartFalling(0.0f);
    return elapsed;
}

// VERIFY: 90ecaafa manual
// OFFSET: 0x00762980
uint32_t CMovement_C::HoverMove(uint32_t time, uint32_t elapsed, float distance, C3Vector direction) {
    if (!elapsed) {
        return 0;
    }

    float hoverHeight;
    int32_t noGround;
    uint64_t facetId;

    if (!this->GetCurrentHoverHeight(&hoverHeight, &noGround, &facetId)) {
        return elapsed;
    }

    if (noGround && this->TryStartFalling()) {
        uint32_t result = this->Fall(time, elapsed, distance, C2Vector(direction));

        if (result) {
            return result;
        }
    }

    float total = static_cast<float>(elapsed);
    float climb = total * 0.001f * 3.5f;
    float mx = direction.x * distance;
    float my = direction.y * distance;
    float mz = distance * direction.z;
    float delta = this->m_hoverHeight - hoverHeight;

    if (-climb <= delta) {
        if (delta < climb) {
            climb = delta;
        }
    } else {
        climb = -climb;
    }

    mz = mz + climb;
    distance = sqrt(my * my + mx * mx + mz * mz);

    if (fabs(distance) >= 0.00000023841858f) {
        float scale = 1.0f / distance;
        direction.x = mx * scale;
        direction.y = my * scale;
        direction.z = mz * scale;
    }

    if (fabs(distance) < 0.00000095367432f) {
        return elapsed;
    }

    C4Plane planes[7];

    for (uint32_t i = 0; i < 7; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    uint32_t stuck = 0;
    float consumed = 0.0f;
    int32_t moved = 0;
    uint32_t facetIndex;
    uint32_t planeCount;
    float moveDist;

    if (!this->DistanceToMove(&s_moveFacets, direction, distance, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
        return elapsed;
    }

    while (true) {
        if (facetIndex >= s_moveFacets.facets.m_count) {
            C3Vector move(direction.x * moveDist, direction.y * moveDist, moveDist * direction.z);
            this->m_position += move;
            goto done;
        }

        {
            if (s_moveFacets.facetIds.m_data[facetIndex]) {
                facetId = s_moveFacets.facetIds.m_data[facetIndex];
            }

            this->m_position.x = direction.x * moveDist + this->m_position.x;
            this->m_position.y = direction.y * moveDist + this->m_position.y;
            this->m_position.z = direction.z * moveDist + this->m_position.z;
            float left = distance - moveDist;
            float progress = (total - consumed) * (moveDist / distance);
            distance = left;

            if (progress + 0.00000095367432f <= 1.0f) {
                if (++stuck > 5) {
                    goto anchors;
                }
            } else {
                stuck = 1;
                consumed = consumed + progress;
            }

            if (total - consumed < 1.0f) {
                if (CMath::fnotequal(total, consumed)) {
                    goto anchors;
                }

                goto done;
            }

            C3Vector slide(direction.x * left, direction.y * left, left * direction.z);
            C3Vector normal = s_moveFacets.facets.m_data[facetIndex].plane.n;
            C3Vector push;

            if (this->UseWalkableRedirection(facetIndex, nullptr)) {
                const CFacet& facet = s_moveFacets.facets.m_data[facetIndex];
                float p = (-facet.plane.n.y * direction.y + -facet.plane.n.z * direction.z + -facet.plane.n.x * direction.x) * left + 0.001f;
                push = C3Vector(facet.plane.n.x * p, facet.plane.n.y * p, p * facet.plane.n.z);
            } else {
                push = this->CalcRunWalkBlockingObstaclePush(direction, left, sqrt(slide.y * slide.y + slide.x * slide.x), normal);
            }

            moved = 1;
            float x = slide.x + push.x;
            float y = slide.y + push.y;
            float z = slide.z + push.z;
            direction = C3Vector(x, y, z);
            distance = sqrt(y * y + z * z + x * x);

            if (fabs(distance) < 0.00000095367432f) {
                goto anchors;
            }

            float scale = 1.0f / distance;
            direction.x = x * scale;
            direction.y = scale * direction.y;
            direction.z = scale * direction.z;

            if (!this->DistanceToMove(&s_moveFacets, direction, distance, &facetIndex, planes, &planeCount, &moveDist, nullptr)) {
                return elapsed;
            }
        }
    }

done:
    if (!moved) {
        goto transport;
    }

anchors:
    this->UpdateAnchors(0);

transport:
    WOWGUID guid;
    guid.m_guid = facetId;
    this->SetTransport(guid);
    return elapsed;
}

// VERIFY: 9ba90a88 manual
// OFFSET: 0x00762E00
uint32_t CMovement_C::CollideRequestMove(uint32_t time, uint32_t elapsed, C3Vector move) {
    if (!elapsed) {
        return 0;
    }

    C3Vector target(this->m_position.x + move.x, this->m_position.y + move.y, this->m_position.z + move.z);

    if (!World::IsValidPosition(target.x, target.y, target.z, this->m_collisionRadius + 71.375595f)) {
        return elapsed;
    }

    float vertical = this->CanCollideWhileFlying() ? move.z : 0.0f;
    float length = sqrt(move.y * move.y + move.x * move.x + vertical * vertical);
    float speed = length / (elapsed * 0.001f);
    C3Vector direction;

    if (fabs(length) >= 0.00000095367432f) {
        direction.x = move.x * (1.0f / length);
        direction.y = move.y * (1.0f / length);
        direction.z = 1.0f / length * vertical;
    }

    uint32_t consumed = 0;
    int32_t onSpline = 0;
    uint32_t step;

    while (true) {
        uint32_t flags = this->m_flags;

        if (!(flags & 0x40C0100F) || (flags & MOVEMENTFLAG_ROOT)) {
            goto groundNormal;
        }

        step = elapsed - consumed;
        uint32_t stepTime = time + consumed;
        float stepDist = step * 0.001f * speed;

        if (!this->GetMoveFacets(stepDist, step, direction)) {
            break;
        }

        uint32_t oldFlags2 = this->m_flags2;
        uint32_t oldFlags = this->m_flags;
        int32_t jumping = this->IsJumping();
        WOWGUID oldTransport = this->m_transportGuid;
        float oldSpeed = this->m_currentSpeed;
        uint32_t used;

        if (this->CanCollideWhileFlying()) {
            used = this->Swim(stepTime, step, stepDist, direction);
        } else if (this->m_flags & MOVEMENTFLAG_FALLING) {
            used = this->Fall(stepTime, step, stepDist, C2Vector(direction));
        } else if (this->m_flags & MOVEMENTFLAG_HOVER) {
            used = this->HoverMove(stepTime, step, stepDist, direction);
        } else {
            used = this->TraceSurface(stepTime, step, stepDist, C2Vector(direction));
        }

        consumed += used;
        int32_t transportChanged = oldTransport != this->m_transportGuid;
        this->CallMoveEventHandlers(time + consumed, elapsed - consumed, oldFlags, oldFlags2, jumping, transportChanged);

        if (transportChanged || (!(this->m_flags & MOVEMENTFLAG_FALLING) && (oldFlags & MOVEMENTFLAG_FALLING))) {
            uint32_t unused = step - used;

            if (this->m_anchorElapsedMs >= unused) {
                this->m_anchorElapsedMs = this->m_anchorElapsedMs - unused;
            } else {
                this->m_anchorElapsedMs = 0;
            }

            goto groundNormal;
        }

        if (!this->IsOnSpline() && this->m_currentSpeed != oldSpeed) {
            this->CalcCurrentSpeed(0);
            goto groundNormal;
        }

        if (!this->m_anchorElapsedMs && (this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT))) {
            this->m_anchorElapsedMs = elapsed - consumed;
        }

        if (consumed >= elapsed) {
            goto groundNormal;
        }
    }

    if (this->IsOnSpline()) {
        this->MoveSplineMoverWithoutCollision(target, time, elapsed);
        onSpline = 1;
    } else {
        this->SkipTime(step);
        this->m_anchorElapsedMs -= step;
    }

    consumed = elapsed;

    if (onSpline) {
        goto done;
    }

groundNormal:
    if (!this->m_spline || (this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE) || !(this->m_spline->m_flags & SPLINEFLAG_FLYING)) {
        this->GroundNormal();
    }

done:
    this->ValidateWorldPosition(CPassenger::GetPosition(this->m_position));
    return consumed;
}

// VERIFY: 1038edbb auto
// OFFSET: 0x006EAE70
void CMovement_C::OnSplineStop(uint32_t time) {
    this->m_spline->m_flags |= SPLINEFLAG_NO_SPLINE;

    if (this->m_spline->m_flags & SPLINEFLAG_PARABOLIC) {
        this->m_unit->OnCollideFallLand(0, 1);
    }

    if (this->m_spline->m_flags & (SPLINEFLAG_FALLING | SPLINEFLAG_PARABOLIC)) {
        this->StopFalling();
    }

    this->SetJumpSplineInAir(0);
    CGUnit_C* unit = this->m_unit;

    if (unit->m_obj->m_guid != CGUnit_C::s_activeMover) {
        return;
    }

    CMoveSpline* spline = this->m_spline;
    uint32_t splineId = spline->m_id;

    if (spline && (spline->m_flags & SPLINEFLAG_PARABOLIC)) {
        unit->OnCollideFallLand(0, 1);
    }

    this->RemoveSpline();

    if (this->TryStartFalling() && !this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    this->m_unit->SendSplineDone(time, splineId);
}

// VERIFY: c68d5a1e auto
// OFFSET: 0x006E9B20
void CMovement_C::SkipTime(uint32_t time) {
    if (this->m_unit->m_obj->m_guid != CGUnit_C::s_activeMover) {
        return;
    }

    CMovementGlobals* globals = MovementGetGlobals();
    this->m_unit->SendTimeSkip(time);
    globals->m_timeCounterMaybe += time;
}

// VERIFY: 728cff7e manual
// OFFSET: 0x006EB0B0
void CMovement_C::CallMoveEventHandlers(uint32_t time, uint32_t remaining, uint32_t oldFlags, uint32_t oldFlags2, int32_t jumping, int32_t transportChanged) {
    CMoveSpline* spline = this->m_spline;

    if (!remaining && spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) && (spline->m_flags & SPLINEFLAG_DONE)) {
        this->ForceStopMove(1);
        this->SnapToSpline(this->m_spline->m_finalDestination, 0);
        spline = this->m_spline;

        if (spline->m_flags & SPLINEFLAG_FINAL_ANGLE) {
            this->m_facing = spline->m_face.m_angle;
        } else if (spline->m_flags & SPLINEFLAG_FINAL_TARGET) {
            CGObject_C* target = ClntObjMgrObjectPtr(spline->m_face.m_target, TYPEMASK_OBJECT, __FILE__, __LINE__);

            if (target) {
                C3Vector targetPosition = target->GetPosition();
                C3Vector position = this->GetPosition();
                this->SetFacing(CalculateFacingTo(&position, &targetPosition));
            }
        } else if (spline->m_flags & SPLINEFLAG_FINAL_POINT) {
            this->m_facing = CalculateFacingTo(&this->m_position, reinterpret_cast<const C3Vector*>(spline->m_face.m_point));
        }

        uint32_t splineFlags = this->m_spline->m_flags;
        uint32_t parabolic = splineFlags & SPLINEFLAG_PARABOLIC;
        uint32_t falling = splineFlags & (SPLINEFLAG_FALLING | SPLINEFLAG_PARABOLIC);
        this->OnSplineStop(time);

        if (falling) {
            this->m_unit->OnCollideFallLand(oldFlags, parabolic);
        } else {
            this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP, false, 0.0f, 0, 0, 255);
        }
    } else if (spline && (spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        this->SnapToSpline(spline->m_finalDestination, 0);
        this->OnSplineStop(time);
    }

    if ((this->m_flags & MOVEMENTFLAG_FALLING) || !(oldFlags & MOVEMENTFLAG_FALLING) || !this->m_unit->OnCollideFallLandNotify(time, oldFlags, oldFlags2, jumping)) {
        CGUnit_C* unit = this->m_unit;

        if (transportChanged && unit->m_obj->m_guid == CGUnit_C::s_activeMover && !this->IsOnSpline()) {
            unit->SendMovementUpdate(time, CMSG_MOVE_CHANGE_TRANSPORT, 0.0f, 0, 0, 255);
        } else if ((oldFlags ^ this->m_flags) & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            unit = this->m_unit;

            if (unit->m_obj->m_guid == CGUnit_C::s_activeMover && !this->IsOnSpline()) {
                unit->SendMovementUpdate(time, this->GetMoveEventMsgId(oldFlags, jumping), 0.0f, 0, 0, 255);
            }
        }
    }

    uint32_t flags = this->m_flags;

    if (((flags & MOVEMENTFLAG_FALLING) && !(oldFlags & MOVEMENTFLAG_FALLING)) || ((flags & MOVEMENTFLAG_FALLING_FAR) && !(oldFlags & MOVEMENTFLAG_FALLING_FAR))) {
        this->m_unit->OnCollideFalling();
    }

    if (!(this->m_flags & MOVEMENTFLAG_SWIMMING) && (oldFlags & MOVEMENTFLAG_SWIMMING)) {
        CGUnit_C* unit = this->m_unit;

        if (unit->m_obj->m_guid == CGUnit_C::s_activeMover && !this->IsOnSpline()) {
            unit->ProcessLocalMoveEvent(time, MSG_MOVE_JUMP, true, 0.0f, 0, 0, 255);
        }
    }
}

// VERIFY: 731c68ba auto
// OFFSET: 0x006E9380
int32_t CMovement_C::UpdatePitch(uint32_t time, CPlayerMoveEvent* event) {
    if (!this->SetPitch(event->m_pitch)) {
        return this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_SET_PITCH, event->m_needAck, 0.0f, 0, 0, 255);
    }

    int32_t liquidType;
    float height;
    int32_t liquid;

    if (!event->m_needAck || !World::QueryObjectLiquid(this->m_unit->m_worldObject, &liquidType, &height, &liquid) || !this->TryStartFalling()) {
        return 0;
    }

    this->m_unit->SendMovementUpdate(time, MSG_MOVE_HEARTBEAT, 0.0f, 0, 0, 255);
    return 1;
}

// VERIFY: 68d8e0b0 auto
// OFFSET: 0x006E9600
void CMovement_C::SetCollisionHeight(float height) {
    this->m_collisionHeight = height;
    this->ValidateWorldPosition(CPassenger::GetPosition(this->m_position));
}

// VERIFY: 520b8fb1 manual
// OFFSET: 0x006E9920
void CMovement_C::UpdateHoverState(int32_t hover, int32_t a3) {
    if (a3 && !hover) {
        int32_t started = this->TryStartFalling();
        uint32_t flags = this->m_flags;

        if ((flags & MOVEMENTFLAG_FALLING) && !(flags & MOVEMENTFLAG_FALLING_FAR)) {
            this->m_flags = flags | MOVEMENTFLAG_FALLING_FAR;
            this->m_unit->OnCollideFalling();
        } else if (started) {
            this->m_unit->OnCollideFalling();
        }
    }

    this->Hover(hover);
}

// VERIFY: 35d20696 manual
// OFFSET: 0x006E9980
void CMovement_C::Halt() {
    this->StopFalling();
    this->m_flags &= 0xFB303FFF;

    if (this->StopTurn()) {
        this->m_unit->ProcessLocalMoveEvent(MovementGetGlobals()->m_lastUpdateTime, MSG_MOVE_STOP_TURN, false, 0.0f, 0, 0, 255);
    }

    if (this->StopPitch()) {
        this->m_unit->ProcessLocalMoveEvent(MovementGetGlobals()->m_lastUpdateTime, MSG_MOVE_STOP_PITCH, false, 0.0f, 0, 0, 255);
    }

    if (this->m_flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
        this->ForceStopStrafe();
        this->m_unit->ProcessLocalMoveEvent(MovementGetGlobals()->m_lastUpdateTime, MSG_MOVE_STOP_STRAFE, false, 0.0f, 0, 0, 255);
    }

    this->ForceStopMove(0);
    this->m_flags &= ~(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
}

// VERIFY: e3d98fe3 auto
// OFFSET: 0x006E9F10
void CMovement_C::FinishSpline(int32_t force) {
    this->SnapToSpline(this->m_spline->m_finalDestination, force);
    this->m_spline->m_start = this->m_spline->m_duration;
    this->m_spline->m_flags |= SPLINEFLAG_DONE;
}

// VERIFY: bbf210de auto
// OFFSET: 0x006EA9B0
int32_t CMovement_C::HeartBeat(const CPlayerMoveEvent* event) {
    int32_t result = this->UpdateTransportStatus(event->m_transportGuid, event->m_seat);

    if (!result) {
        return result;
    }

    this->m_flags ^= (this->m_flags ^ event->m_moveFlags) & 0x77FFFDFF;
    uint32_t falling = this->m_flags & MOVEMENTFLAG_FALLING;
    this->m_position = event->m_position;
    this->m_facing = event->m_facing;
    this->m_pitch = event->m_pitch;

    if (falling) {
        uint32_t fallTime = event->m_fallTime;
        this->m_fallTimeMs = fallTime;
        this->m_fallStartZ = this->CalcFallStartElevation(fallTime);
    }

    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 16a88b02 auto
// OFFSET: 0x006EAF50
void CMovement_C::RemoveFromMoversList(int32_t a1) {
    if (!this->m_globalUnitsLink.IsLinked()) {
        return;
    }

    if (this->m_flags & 0xC010FF) {
        return;
    }

    if (this->m_moveQueue.Head()) {
        return;
    }

    if (this->m_flags & MOVEMENTFLAG_HOVER) {
        float height;

        if (!this->GetCurrentHoverHeight(&height, nullptr, nullptr) || CMath::fnotequal(height, this->m_hoverHeight)) {
            return;
        }
    }

    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        if (a1) {
            this->OnSplineStop(MovementGetGlobals()->m_idleTime);

            if (this->m_flags & 0xC010FF) {
                return;
            }
        } else {
            spline->m_flags |= SPLINEFLAG_NO_SPLINE;
        }
    }

    this->m_globalUnitsLink.Unlink();
}

// VERIFY: 8de068f5 manual
// OFFSET: 0x006EBA60
void CMovement_C::GravityStateChanged() {
    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FALLING)) {
        goto falling;
    }

    if ((spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FLYING)) || (this->m_flags2 & MOVEMENTFLAG2_DISABLE_GRAVITY) || (this->m_flags & MOVEMENTFLAG_DISABLE_GRAVITY)) {
        uint32_t flags = this->m_flags;
        int32_t jumping = (flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_fallVelocity;
        uint32_t oldFlags = flags & ~MOVEMENTFLAG_DISABLE_GRAVITY;
        this->StopFalling();
        this->HandlePendingActions();

        if (!(this->m_flags & MOVEMENTFLAG_FALLING) && (oldFlags & MOVEMENTFLAG_FALLING)) {
            this->m_unit->OnCollideFallLand(oldFlags, jumping);
        }

        this->RemoveFromMoversList(1);
        return;
    }

falling:
    if (this->TryStartFalling() && !this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 2f45a2e1 auto
// OFFSET: 0x006EBB40
int32_t CMovement_C::FallStateChangedLocal(uint32_t time, uint32_t msgId, uint32_t oldFlags, uint32_t oldFlags2, bool needAck, float value) {
    if (this->IsFallingSwimmingFlying()) {
        if (this->m_flags & MOVEMENTFLAG_FALLING) {
            int32_t jumping = 0.0f != this->m_fallVelocity;
            this->StopFalling();
            int32_t result = this->m_unit->ProcessLocalMoveEvent(time, msgId, needAck, value, 0, 0, 255);
            this->HandlePendingActions();
            this->CallMoveEventHandlers(time, 0, oldFlags, oldFlags2, jumping, 0);
            return result;
        }

        return this->m_unit->ProcessLocalMoveEvent(time, msgId, needAck, value, 0, 0, 255);
    }

    if (this->TryStartFalling()) {
        return this->m_unit->ProcessLocalMoveEvent(time, msgId, needAck, value, 0, 0, 255);
    }

    return 0;
}

// VERIFY: 667c9636 manual
// OFFSET: 0x006EBC50
int32_t CMovement_C::TryStartFlight() {
    int32_t result = this->StartFlight();

    if (result) {
        this->HandlePendingActions();
        return 1;
    }

    return result;
}

// VERIFY: 69ee52cb manual
// OFFSET: 0x006EC170
int32_t CPlayerMoveQueue::RequeueEntry(MoveEventId eventId, uint32_t eventTime) {
    for (CPlayerMoveEvent* event = this->Head(); event; event = this->Next(event)) {
        if (event->m_eventId == eventId) {
            event->m_link.Unlink();
            event->m_eventTime = eventTime;
            this->Enqueue(event);
            return 1;
        }
    }

    return 0;
}

// VERIFY: 527cd32d manual
// OFFSET: 0x006EF6A0
int32_t CMovement_C::InsertTurnToAngleStop(uint32_t time, float facing) {
    float min;
    float max;
    float delta;

    if (this->ComputeLegalRawFacingRange(min, max)) {
        float current = this->m_facing;
        CMovement_C::WrapFacingToRange(current, min, max);
        delta = facing - current;
    } else {
        delta = CMath::normalizeAngleNegPiToPi_(facing - this->m_facing);
    }

    uint32_t stopTime = fabs(delta) / this->m_turnRate * 1000.0f + time;

    if (!this->m_moveQueue.RequeueEntry(MOVEEVENT_STOP_TURN_AT_ANGLE, stopTime)) {
        this->AddPlayerMoveEvent(stopTime, MOVEEVENT_STOP_TURN_AT_ANGLE, true, 0, 0.0f, 0.0f, 0);
    }

    return delta > 0.0f;
}

// VERIFY: d3e9c1b0 manual
// OFFSET: 0x006EF7A0
int32_t CMovement_C::InsertPitchToAngleStop(uint32_t time, float pitch) {
    float delta = CMath::normalizeAngleNegPiToPi_(pitch - this->m_pitch);
    uint32_t stopTime = fabs(delta) / this->m_pitchRate * 1000.0f + time;

    if (!this->m_moveQueue.RequeueEntry(MOVEEVENT_STOP_PITCH_AT_ANGLE, stopTime)) {
        this->AddPlayerMoveEvent(stopTime, MOVEEVENT_STOP_PITCH_AT_ANGLE, true, 0, 0.0f, 0.0f, 0);
    }

    return delta > 0.0f;
}

// VERIFY: 4ab3008b auto
// OFFSET: 0x006E9E20
uint32_t CMovement_C::RequestMove(uint32_t time, uint32_t elapsed, const C3Vector& move) {
    C3Vector target(move.x + this->m_anchorPos.x, this->m_anchorPos.y + move.y, this->m_anchorPos.z + move.z);
    CGUnit_C* unit = this->m_unit;

    if (unit->m_obj->m_guid == CGUnit_C::s_activeMover || !this->m_spline || (unit->m_modelFlags & 0x800000)) {
        C3Vector delta(target.x - this->m_position.x, target.y - this->m_position.y, target.z - this->m_position.z);
        return this->CollideRequestMove(time, elapsed, delta);
    }

    if (!(this->m_spline->m_flags & SPLINEFLAG_FALLING)) {
        this->StopFalling();
    }

    this->m_anchorElapsedMs += elapsed;
    this->m_position = target;
    this->ValidateWorldPosition(CPassenger::GetPosition(this->m_position));
    return elapsed;
}

// VERIFY: 617282b5 manual
// OFFSET: 0x006EF860
int32_t CMovement_C::UpdatePlayerMovement(uint32_t time) {
    CPlayerMoveEvent* event;

    while ((event = this->m_moveQueue.Head()) && static_cast<int32_t>(time - event->m_eventTime) >= 0) {
        CGUnit_C* unit = this->m_unit;

        if ((unit->m_vehiclePassenger && (unit->m_vehiclePassenger->m_flags & 0x200)) || (event->m_eventId == MOVEEVENT_TELEPORT && CGUnit_C::MaybeDeferTeleport(&event->m_transportGuid))) {
            return 2;
        }

        this->m_moveQueue.Head()->m_link.Unlink();
        uint32_t flags = this->m_flags;
        this->m_moveStartDelay = 0;
        MoveEventId eventId = event->m_eventId;
        bool movement = eventId <= MOVEEVENT_STOP_STRAFE || eventId == MOVEEVENT_START_FALLING || eventId == MOVEEVENT_JUMP || eventId == MOVEEVENT_SET_FLYING || eventId == MOVEEVENT_UNSET_FLYING;
        bool swim = eventId >= MOVEEVENT_START_SWIM && eventId <= MOVEEVENT_STOP_SWIM;

        if ((flags & MOVEMENTFLAG_ONTRANSPORT) && (movement || swim || eventId == MOVEEVENT_KNOCK_BACK)) {
        } else if ((flags & MOVEMENTFLAG_ROOT) && (movement || swim)) {
        } else if ((flags & MOVEMENTFLAG_PENDING_ROOT) && movement) {
        } else {
            uint32_t oldFlags = flags;
            int32_t jumping = this->IsJumping();
            int32_t stoppedFalling = 0;

            if (event->unk_0050) {
                if (!(event->m_moveFlags & MOVEMENTFLAG_SPLINE_ENABLED)) {
                    if (this->m_spline && (this->m_spline->m_flags & SPLINEFLAG_PARABOLIC)) {
                        this->m_unit->OnCollideFallLand(0, 1);
                    }

                    this->RemoveSpline();
                }

                if ((this->m_flags & MOVEMENTFLAG_FALLING) && !(event->m_moveFlags & MOVEMENTFLAG_FALLING)) {
                    this->StopFalling();
                    stoppedFalling = 1;
                }
            }

            int32_t sent = 0;

            switch (event->m_eventId) {
                case MOVEEVENT_START_FORWARD:
                    this->StartMove(1, 0);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_FORWARD, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_START_BACKWARD:
                    this->StartMove(0, 0);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_BACKWARD, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_STOP:
                    this->StopMove();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_START_STRAFE_LEFT:
                    this->StartStrafe(1);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_STRAFE_LEFT, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_START_STRAFE_RIGHT:
                    this->StartStrafe(0);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_STRAFE_RIGHT, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_STOP_STRAFE:
                    this->StopStrafe();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP_STRAFE, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_START_ASCEND:
                    if (this->StartAscensionDescension(1)) {
                        sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_ASCEND, event->m_needAck, 0.0f, 0, 0, 255);
                    }

                    break;

                case MOVEEVENT_START_DESCEND:
                    if (this->StartAscensionDescension(0)) {
                        sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_DESCEND, event->m_needAck, 0.0f, 0, 0, 255);
                    }

                    break;

                case MOVEEVENT_STOP_ASCEND:
                    if (this->StopAscensionDescension()) {
                        sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP_ASCEND, event->m_needAck, 0.0f, 0, 0, 255);
                    }

                    break;

                case MOVEEVENT_START_FALLING:
                    if (!this->TryStartFalling()) {
                        break;
                    }

                    this->m_unit->OnCollideFalling();

                    if (!event->m_needAck) {
                        goto heartbeat;
                    }

                    this->m_unit->SendMovementUpdate(time, MSG_MOVE_HEARTBEAT, 0.0f, 0, 0, 255);
                    sent = 1;
                    break;

                case MOVEEVENT_JUMP:
                    if (this->Jump(1)) {
                        sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_JUMP, event->m_needAck, 0.0f, 0, 0, 255);
                    }

                    break;

                case MOVEEVENT_START_TURN_LEFT:
                    this->StartTurn(1);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_TURN_LEFT, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_START_TURN_RIGHT:
                    this->StartTurn(0);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_TURN_RIGHT, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_STOP_TURN:
                case MOVEEVENT_STOP_TURN_AT_ANGLE:
                    this->StopTurn();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP_TURN, event->m_needAck, 0.0f, 0, 0, 255);
                    CGInputControl::GetActive()->OnTurnToAngleStop();
                    break;

                case MOVEEVENT_START_PITCH_UP:
                    this->StartPitch(1);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_PITCH_UP, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_START_PITCH_DOWN:
                    this->StartPitch(0);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_PITCH_DOWN, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_STOP_PITCH:
                case MOVEEVENT_STOP_PITCH_AT_ANGLE:
                    this->StopPitch();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP_PITCH, event->m_needAck, 0.0f, 0, 0, 255);
                    CGInputControl::GetActive()->OnPitchToAngleStop();
                    break;

                case MOVEEVENT_SET_RUN_MODE:
                    this->SetRunMode(1);
                    this->m_unit->UpdateObjectEffectMovementStates();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_SET_RUN_MODE, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_SET_WALK_MODE:
                    this->SetRunMode(0);
                    this->m_unit->UpdateObjectEffectMovementStates();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_SET_WALK_MODE, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_SET_FACING:
                    this->SetRawFacing(event->m_facing);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_SET_FACING, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_SET_PITCH:
                    sent = this->UpdatePitch(time, event);
                    break;

                case MOVEEVENT_START_SWIM:
                    this->StartSwim();
                    this->HandlePendingActions();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_START_SWIM, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_STOP_SWIM:
                    this->StopSwim();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_STOP_SWIM, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_SET_RUN_SPEED:
                    this->ChangeRunSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_RUN_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_RUN_BACK_SPEED:
                    this->ChangeRunBackSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_WALK_SPEED:
                    this->ChangeWalkSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_WALK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_SWIM_SPEED:
                    this->ChangeSwimSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_SWIM_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_SWIM_BACK_SPEED:
                    this->ChangeSwimBackSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_FLIGHT_SPEED:
                    this->ChangeFlightSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_FLIGHT_BACK_SPEED:
                    this->ChangeFlightBackSpeed(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_TURN_RATE:
                    this->ChangeTurnRate(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_TURN_RATE_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_PITCH_RATE:
                    this->ChangePitchRate(event->m_value);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_PITCH_RATE_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_GRAVITY_ENABLE:
                    if (this->EnableGravity(1)) {
                        this->GravityStateChanged();
                    }

                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_GRAVITY_ENABLE_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_GRAVITY_DISABLE:
                    if (this->EnableGravity(0)) {
                        this->GravityStateChanged();
                    }

                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_GRAVITY_DISABLE_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_KNOCK_BACK:
                    this->Launch(C2Vector(event->m_value, event->m_launchDirY), event->m_launchSpeed, event->m_launchVerticalSpeed);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_KNOCK_BACK_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_FEATHER_FALL:
                case MOVEEVENT_NORMAL_FALL: {
                    int32_t enable = event->m_eventId == MOVEEVENT_FEATHER_FALL;
                    this->FeatherFall(enable);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_FEATHER_FALL_ACK, event->m_needAck, enable ? 1.0f : 0.0f, event->m_ackCounter, 0, 255);
                    this->PostFeatherFall();
                    this->m_unit->OnCollideFalling();
                    break;
                }

                case MOVEEVENT_SET_HOVER:
                    this->Hover(1);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_HOVER_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_UNSET_HOVER:
                    if (!event->m_needAck || this->m_unit->IsActiveMover()) {
                        this->UpdateHoverState(0, 1);
                    } else {
                        this->UpdateHoverState(0, 0);
                    }

                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_HOVER_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_WATER_WALK:
                    this->WalkOnWater(1);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_WATER_WALK_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_LAND_WALK:
                    this->WalkOnWater(0);

                    if (!event->m_needAck || this->m_unit->IsActiveMover()) {
                        this->TryStartFalling();
                    }

                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_WATER_WALK_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_ROOT: {
                    if (event->m_needAck && this->IsOnSpline()) {
                        this->FinishSpline(0);
                    }

                    int32_t wasJumping = this->IsJumping();
                    uint32_t rootFlags = this->m_flags;
                    uint32_t rootFlags2 = this->m_flags2;
                    this->TryRoot();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_MOVE_ROOT_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    this->CallMoveEventHandlers(time, 0, rootFlags, rootFlags2, wasJumping, 0);
                    break;
                }

                case MOVEEVENT_UNROOT: {
                    int32_t wasJumping = this->IsJumping();
                    uint32_t rootFlags = this->m_flags;
                    uint32_t rootFlags2 = this->m_flags2;
                    this->UnRoot(!event->m_needAck || this->m_unit->IsActiveMover());
                    this->m_unit->UnRootEffects();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_MOVE_UNROOT_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    this->CallMoveEventHandlers(time, 0, rootFlags, rootFlags2, wasJumping, 0);
                    break;
                }

                case MOVEEVENT_TELEPORT:
                    this->m_flags2 ^= (this->m_flags2 ^ event->m_moveExtraFlags) & 0x2040;

                    if (event->unk_0050) {
                        this->HeartBeat(event);
                    }

                    this->Teleport(event->m_transportGuid, event->m_position, event->m_facing, 1, event->m_needAck, event->m_seat);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_TELEPORT_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_FLYING: {
                    uint32_t flyFlags = this->m_flags;
                    uint32_t flyFlags2 = this->m_flags2;

                    if (this->TryStartFlight()) {
                        sent = this->FallStateChangedLocal(time, CMSG_MOVE_SET_FLY, flyFlags, flyFlags2, event->m_needAck, 0.0f);
                        this->m_unit->UpdateObjectEffectMovementStates();
                    }

                    break;
                }

                case MOVEEVENT_UNSET_FLYING:
                    this->StopFlying();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_FLY, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_SET_CAN_FLY:
                    this->m_flags |= MOVEMENTFLAG_CAN_FLY;
                    this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_FLY_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_UNSET_CAN_FLY:
                    if (this->m_flags & MOVEMENTFLAG_FLYING) {
                        this->StopFlying();
                    }

                    this->m_flags &= ~MOVEMENTFLAG_CAN_FLY;
                    this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_FLY_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_TIME_SYNC:
                    if (this->m_flags & MOVEMENTFLAG_ONTRANSPORT) {
                        this->m_flags &= ~MOVEMENTFLAG_ONTRANSPORT;

                        if (this->TryStartFalling()) {
                            this->m_unit->OnCollideFalling();
                        }

                        sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_MOVE_UNROOT_ACK, false, 0.0f, 0, 0, 255);
                        this->UpdateHeartbeatTimerA(time);
                    }

                    this->SendTimeSyncResp(event->m_eventTime, event->m_ackCounter);
                    break;

                case MOVEEVENT_DISMISS_VEHICLE:
                    this->Halt();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_DISMISS_CONTROLLED_VEHICLE, event->m_needAck, 0.0f, 0, 0, 255);
                    break;

                case MOVEEVENT_TURN_TO_ANGLE: {
                    int32_t left = this->InsertTurnToAngleStop(time, event->m_facing);
                    this->StartTurn(left);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, left ? MSG_MOVE_START_TURN_LEFT : MSG_MOVE_START_TURN_RIGHT, event->m_needAck, 0.0f, 0, 0, 255);
                    break;
                }

                case MOVEEVENT_PITCH_TO_ANGLE: {
                    int32_t up = this->InsertPitchToAngleStop(time, event->m_pitch);
                    this->StartPitch(up);
                    sent = this->m_unit->ProcessLocalMoveEvent(time, up ? MSG_MOVE_START_PITCH_UP : MSG_MOVE_START_PITCH_DOWN, event->m_needAck, 0.0f, 0, 0, 255);
                    break;
                }

                case MOVEEVENT_CHANGE_SEAT:
                    this->Halt();
                    sent = this->m_unit->ProcessLocalMoveEvent(time, CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE, event->m_needAck, 0.0f, 0, event->m_transportGuid.m_guid, event->m_seat);
                    break;

                case MOVEEVENT_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY:
                    this->m_flags2 |= MOVEMENTFLAG2_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY;
                    this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY:
                    this->m_flags2 &= ~MOVEMENTFLAG2_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY;
                    this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 255);
                    break;

                case MOVEEVENT_SET_COLLISION_HEIGHT:
                    this->SetCollisionHeight(event->m_value);
                    this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_COLLISION_HGT_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 255);
                    break;

                default:
                    break;
            }

            if (event->m_needAck && !sent && (this->m_flags & 0xC0100F) && !(oldFlags & 0xC0100F)) {
                this->UpdateHeartbeatTimerA(time);
            }

        heartbeat:
            if (event->m_eventId != MOVEEVENT_TELEPORT) {
                this->m_unit->m_clientFlags |= 0x20000000;

                if (event->unk_0050 && this->HeartBeat(event) && stoppedFalling) {
                    this->m_unit->OnCollideFallLand(oldFlags, jumping);
                }

                this->m_unit->m_clientFlags &= ~0x20000000u;
            }

            this->SetInterpolation(time);
        }

        event->m_link.Unlink();
        CPlayerMoveQueue::s_freeList.LinkToTail(event);
    }

    return this->m_moveQueue.Head() || (this->m_flags & 0xC010FF);
}

// VERIFY: 18abb990 manual
// OFFSET: 0x006EAC40
void CMovement_C::ApplyMovement(uint32_t time, uint32_t elapsed) {
    if (!World::IsValidPosition(this->m_position.x, this->m_position.y, this->m_position.z, 2.0f)) {
        return;
    }

    if (!elapsed) {
        return;
    }

    C3Vector destination;
    uint32_t consumed = 0;

    if (this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        destination = this->m_spline->m_finalDestination;
    }

    int32_t interpolated = 0;

    while (true) {
        uint32_t flags = this->m_flags;

        if (!(flags & 0xC010FF) && (!(flags & MOVEMENTFLAG_HOVER) || (flags & MOVEMENTFLAG_ROOT))) {
            break;
        }

        uint32_t step = elapsed - consumed;
        C3Vector move;
        this->m_anchorElapsedMs += step;

        if (this->IsOnSpline()) {
            if (!this->PlotUnitSplineMovement(time, step, &destination)) {
                return;
            }

            move.x = destination.x - this->m_anchorPos.x;
            move.y = destination.y - this->m_anchorPos.y;
            move.z = destination.z - this->m_anchorPos.z;
        } else {
            if (!this->PlotUnitMovement(this->GetMoveStartTime(step), &move) && !(this->m_flags & MOVEMENTFLAG_HOVER)) {
                return;
            }

            if (this->m_flags2 & (MOVEMENTFLAG2_INTERPOLATED_MOVEMENT | MOVEMENTFLAG2_INTERPOLATED_TURNING | MOVEMENTFLAG2_INTERPOLATED_PITCHING)) {
                C3Vector target(this->m_anchorPos.x + move.x, this->m_anchorPos.y + move.y, this->m_anchorPos.z + move.z);

                if (this->Interpolate(time - step, step, &target, &this->m_facing, &this->m_pitch)) {
                    interpolated = 1;
                    move.x = target.x - this->m_anchorPos.x;
                    move.y = target.y - this->m_anchorPos.y;
                    move.z = target.z - this->m_anchorPos.z;
                }
            }
        }

        consumed += this->RequestMove(time - step, step, move);

        if (interpolated) {
            this->UpdateAnchors(0);
        }

        if (consumed >= elapsed) {
            break;
        }
    }

    if (this->IsValidPosition()) {
        CMoveSpline* spline = this->m_spline;

        if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
            this->SnapToSpline(destination, 0);
        }

        if (this->IsOnFlyingSpline()) {
            this->UpdateAnchors(0);
        }
    }
}

// VERIFY: d4ff9c2f manual
// OFFSET: 0x006F09F0
void CMovement_C::ExecuteMovement(uint32_t time, uint32_t lastTime) {
    uint32_t cursor = lastTime;
    uint32_t delta = time - lastTime;
    uint32_t total = delta;
    uint32_t processed = 0;
    CMovementGlobals* globals = MovementGetGlobals();
    int32_t result;

    if (delta > 250) {
        cursor = time - 250;
        this->SkipTime(delta - 250);
        total = 250;
        delta = 250;
    } else if (!delta) {
        goto done;
    }

    while (true) {
        uint32_t step = delta - processed;
        CPlayerMoveEvent* head = this->m_moveQueue.Head();

        if (head) {
            int32_t untilEvent = head->m_eventTime - cursor;

            if (untilEvent < 0) {
                untilEvent = 0;
            }

            if (step > static_cast<uint32_t>(untilEvent)) {
                step = untilEvent;
            }
        }

        if (this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover && !this->IsOnSpline() && (this->m_flags & 0xC0100F)) {
            if (globals->m_timeCounterMaybe - cursor < step) {
                step = globals->m_timeCounterMaybe - cursor;
            }
        }

        cursor += step;
        globals->m_idleTime = cursor;

        if (step) {
            uint32_t flags = this->m_flags;

            if (flags & 0x40C010FF) {
                if (flags & MOVEMENTFLAG_ONTRANSPORT) {
                    globals->m_timeCounterMaybe += step;
                } else {
                    this->ApplyMovement(cursor, step);

                    if (this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover && (this->m_flags & 0xC0100F) && !this->IsOnSpline() && static_cast<int32_t>(cursor - globals->m_timeCounterMaybe) >= 0) {
                        this->m_unit->SendMovementUpdate(cursor, MSG_MOVE_HEARTBEAT, 0.0f, 0, 0, 255);
                    }
                }
            }

            processed += step;
        }

        result = this->UpdatePlayerMovement(cursor);

        if (result != 1) {
            break;
        }

        if (processed >= total) {
            goto done;
        }

        delta = total;
    }

    if (result) {
        if (this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
            globals->m_timeCounterMaybe += total - processed;
        }
    } else {
        this->RemoveFromMoversList(1);
    }

done:
    this->m_unit->OnMoveUpdate(time, 0, 0);

    if (std::isnan(static_cast<double>(this->m_position.x)) || std::isnan(static_cast<double>(this->m_position.y)) || std::isnan(static_cast<double>(this->m_position.z))) {
        ConsolePrintf("Mover at invalid position");
    }

    this->m_unit->m_modelFlags &= ~0x800000u;
}

// VERIFY: 8023e94a manual
// OFFSET: 0x006F13E0
void CMovement_C::MoveUnits(uint32_t time, uint32_t lastUpdateTime) {
    CMovementGlobals* globals = MovementGetGlobals();

    if (!globals) {
        return;
    }

    CMovement_C* mover = globals->m_movers.Head();

    while (mover) {
        CMovement_C* next = globals->m_movers.Next(mover);

        if (mover->m_flags & MOVEMENTFLAG_SPLINE_ENABLED) {
            mover->SkipTime(time - lastUpdateTime);
        } else {
            mover->ExecuteMovement(time, lastUpdateTime);
        }

        mover = next;
    }
}

// VERIFY: 1ce66aca manual
// OFFSET: 0x006E9C30
int32_t CMovement_C::PlotUnitSplineMovement(uint32_t time, uint32_t elapsed, C3Vector* position) {
    int32_t wasInAir = (this->m_flags2 & MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR) && this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE) && (this->m_spline->m_flags & SPLINEFLAG_PARABOLIC);
    int32_t wasAnimTier = (this->m_flags2 & MOVEMENTFLAG2_ANIM_TIER_IN_TRANS) && this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE) && (this->m_spline->m_flags & SPLINEFLAG_ANIMATION);

    if (!this->CMovementShared::PlotUnitSplineMovement(time, position)) {
        return 0;
    }

    if (!wasInAir && (this->m_flags2 & MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR) && this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE) && (this->m_spline->m_flags & SPLINEFLAG_PARABOLIC)) {
        this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_JUMP, true, 0.0f, 0, 0, 255);
    }

    if (!wasAnimTier && (this->m_flags2 & MOVEMENTFLAG2_ANIM_TIER_IN_TRANS) && this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE) && (this->m_spline->m_flags & SPLINEFLAG_ANIMATION)) {
        this->m_unit->SetAnimType(static_cast<uint8_t>(this->m_spline->m_flags));
    }

    uint32_t flags = this->m_flags;

    if (!(flags & 0xC0100F)) {
        return 1;
    }

    float dx = position->x - this->m_position.x;
    float dy = position->y - this->m_position.y;
    float dz = position->z - this->m_position.z;
    float distSq = dx * dx + dy * dy;
    float dt = elapsed * 0.001f;

    if (distSq / (dt * dt) <= 3600.0f && dz * dz + distSq <= 9.0f) {
        return 1;
    }

    this->m_position = *position;

    if (flags & MOVEMENTFLAG_FALLING) {
        if (this->IsOnFallingSpline()) {
            this->m_fallTimeMs = this->m_spline->m_start;
        } else {
            this->m_fallStartZ = this->CalcFallStartElevation(this->m_fallTimeMs);
        }
    }

    this->ValidateWorldPosition(this->GetPosition(this->m_position));
    return 0;
}

// VERIFY: 1b35a700 manual
// OFFSET: 0x006EC400
int32_t CMovement_C::ForceSetTransportInt(WOWGUID transport, uint8_t seat, int32_t force) {
    if ((transport == this->m_transportGuid && seat == this->m_transportSeat) || (!force && this->m_spline) || (transport && !MovementObjIsTransport(transport))) {
        return 0;
    }

    if (this->m_transportGuid && !ClntObjMgrObjectPtr(this->m_transportGuid, TYPEMASK_OBJECT, __FILE__, __LINE__)) {
        DebugLog("CMovementData_C::ForceSetTransportInt() was called with an invalid m_transportGUID: (m_transportGUID: %016I64X) (guid: %016I64X)", this->m_transportGuid, transport);
        this->m_transportGuid = WOWGUID();
        return 0;
    }

    uint32_t oldHigh = static_cast<uint32_t>(this->m_transportGuid.m_guid >> 32);
    uint32_t newHigh = static_cast<uint32_t>(transport.m_guid >> 32);

    if ((oldHigh & 0xF0F00000) == 0xF0500000 || (!(oldHigh & 0xF0000000) && ((oldHigh & 0xF07FFFFF) | static_cast<uint32_t>(this->m_transportGuid.m_guid))) || (newHigh & 0xF0F00000) == 0xF0500000 || (!(newHigh & 0xF0000000) && ((newHigh & 0xF07FFFFF) | static_cast<uint32_t>(transport.m_guid)))) {
        this->m_unit->VehiclePassengerOnUpdateTransport(transport, seat, 0);
    }

    this->m_transportSeat = seat;

    if (transport == this->m_transportGuid) {
        return 1;
    }

    C44Matrix transportMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    if (this->m_transportGuid) {
        float facing = this->RemoveFromTransport(&transportMatrix);

        if (this->m_unit->GetObjectModel()) {
            CM2Model* model = this->m_unit->GetObjectModel();

            if (model->m_fFlags & 0x1) {
                model->ChangeFrameOfReference(nullptr);
            }
        }

        this->m_unit->RotateFacing(facing);
        this->m_unit->TrackingMakeRelativeTo(&transportMatrix, facing);

        uint32_t high = static_cast<uint32_t>(this->m_transportGuid.m_guid >> 32);

        if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(this->m_transportGuid.m_guid)))) {
            CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transportGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (vehicle && vehicle->m_vehicle) {
                vehicle->m_vehicle->UpdateFreeSeatMask();
            }
        } else {
            this->FixupQueuedSetFacingEvents(facing);
        }
    }

    if (transport) {
        C44Matrix inverse;
        float facing = this->AddToTransport(transport, &inverse, &transportMatrix);

        if (this->m_unit->GetObjectModel()) {
            CM2Model* model = this->m_unit->GetObjectModel();

            if (model->m_fFlags & 0x1) {
                model->m_frameOfReferenceMatrix = transportMatrix;
                model->ChangeFrameOfReference(&model->m_frameOfReferenceMatrix);
            }
        }

        this->m_unit->RotateFacing(facing);
        this->m_unit->TrackingMakeRelativeTo(&inverse, facing);

        if ((newHigh & 0xF0F00000) != 0xF0500000 && ((newHigh & 0xF0000000) || !((newHigh & 0xF07FFFFF) | static_cast<uint32_t>(transport.m_guid)))) {
            this->FixupQueuedSetFacingEvents(facing);
        }

        if (this->m_flags & MOVEMENTFLAG_SWIMMING) {
            this->StopSwim();
        }

        if (this->m_transportGuid && this->m_unit->IsActiveMover()) {
            CMovementGlobals* globals = MovementGetGlobals();
            globals->m_hasTransportTime2 = globals->m_transportTime != globals->m_transportTime2;
        }
    }

    if (this->m_flags & (MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) {
        this->UpdateAnchors(0);
    }

    this->m_transportGuid = transport;
    MovementUpdateCameraYaw(*this->m_ownerGuid, WOWGUID{ this->m_unit->GetCameraRelativeTo() });
    return 1;
}

// VERIFY: 0fab2c50 manual
// OFFSET: 0x006ECF80
void CMovement_C::Teleport(WOWGUID transport, const C3Vector& position, float facing, int32_t removeSpline, int32_t needAck, uint8_t seat) {
    if (this->m_flags2 & MOVEMENTFLAG2_UNK_2000) {
        this->m_unit->m_clientFlags |= 0x20000000;
    } else {
        this->m_unit->m_clientFlags &= ~0x20000000;
    }

    this->ForceSetTransportInt(transport, seat, 1);
    this->m_unit->m_clientFlags &= ~0x20000000;

    if (!(this->m_transportGuid == transport)) {
        return;
    }

    float dx = this->m_position.x - position.x;
    float dy = this->m_position.y - position.y;
    float dz = this->m_position.z - position.z;
    int32_t preload = dz * dz + dy * dy + dx * dx > 22500.0f;

    this->CMovementShared::Teleport(position, facing, removeSpline);
    this->RemoveFromMoversList(1);

    if ((!needAck || this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) && this->TryStartFalling() && !this->m_globalUnitsLink.m_next) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    C3Vector world = this->GetPosition(this->m_position);
    this->ValidateWorldPosition(world);
    MovementUpdateCameraYaw(*this->m_ownerGuid, WOWGUID{ this->m_unit->GetCameraRelativeTo() });
    MovementUpdateWorld(*this->m_ownerGuid, world, preload);
    this->m_flags2 &= 0xE3FF;
}

// VERIFY: 930c3840 manual
// OFFSET: 0x006EB730
int32_t CMovement_C::UpdateStatusInternal(uint32_t time, const CMovementStatus& status, int32_t& timeOffset, int32_t local, int32_t force) {
    if ((!this->m_spline || (this->m_spline->m_flags & 0x400)) && this->m_unit->m_vehiclePassenger && (this->m_unit->m_vehiclePassenger->m_flags & 0x200)) {
        return 0;
    }

    if (this->m_spline) {
        force = 1;
    }

    if (!(this->m_flags & 0x80000000)) {
        this->m_gameTime = status.m_gameTime;
        this->m_statusTimeMs = time;
        this->m_flags |= 0x80000000;
    }

    CMovementGlobals* globals = MovementGetGlobals();

    if (static_cast<int32_t>(time - globals->m_lastUpdateTime) < 0) {
        time = globals->m_lastUpdateTime;
    }

    int32_t serverDelta = status.m_gameTime - this->m_gameTime;
    int32_t localDelta = time - this->m_statusTimeMs;

    if (serverDelta > 0) {
        this->m_gameTime = status.m_gameTime;
    } else {
        serverDelta = 0;
    }

    timeOffset = serverDelta - localDelta;
    uint32_t skew = this->AddTimeSkewSample(serverDelta, localDelta);

    if (force) {
        this->m_statusTimeMs = time + timeOffset;
    } else {
        if (!(this->m_flags & 0xC010FF) && !this->m_moveQueue.Head()) {
            timeOffset += skew - this->m_timeSkew;
            timeOffset = static_cast<uint32_t>(timeOffset + 500) > 1500 ? (timeOffset + 500 < 0 ? -500 : 1000) : timeOffset;

            if (static_cast<int32_t>(time + timeOffset - this->m_statusTimeMs) < 0) {
                timeOffset = this->m_statusTimeMs - time;
            }

            this->m_timeSkew = skew;
        }

        timeOffset = static_cast<uint32_t>(timeOffset + 500) > 1500 ? (timeOffset + 500 < 0 ? -500 : 1000) : timeOffset;
        this->m_statusTimeMs = time + timeOffset;

        if (static_cast<int32_t>(globals->m_lastUpdateTime - this->m_statusTimeMs) < 0) {
            return 0;
        }
    }

    uint32_t oldFlags = this->m_flags;
    int32_t falling = (oldFlags & 0x1000) && this->m_fallVelocity != 0.0f;

    if (local && !force) {
        this->m_flags = oldFlags ^ ((status.m_moveFlags ^ oldFlags) & 0x77E00DFF);
    } else {
        this->m_flags = oldFlags ^ ((status.m_moveFlags ^ oldFlags) & 0x77FFFDFF);
    }

    this->m_flags2 = status.m_moveExtraFlags;
    this->UpdateBaseStatus(status);

    if (this->m_unit->m_modelFlags & 0x40000) {
        this->UpdateTransportStatus(WOWGUID{ CMovementShared::UpdateTransportStatus(status) }, status.m_transportSeat);
    }

    if (this->m_flags & 0x1000) {
        this->UpdateFallState(status);
    } else {
        this->CalcDirection(0);

        if (oldFlags & 0x1000) {
            this->m_unit->OnCollideFallLand(oldFlags, falling);
        }
    }

    if (!force) {
        this->Extrapolate(this->m_statusTimeMs, globals->m_lastUpdateTime - this->m_statusTimeMs);
    }

    if (!(this->m_flags & 0xC010FF)) {
        this->m_unit->OnMoveUpdate(time, 1, 0);
    }

    this->m_moveQueue.FreeAllExceptTimeSync();
    this->m_moveStartDelay = 0;
    return 1;
}

// VERIFY: cb3d6470 auto
// OFFSET: 0x006EC8B0
void CMovement_C::AddPlayerSpeedEvent(uint32_t eventTime, MoveEventId eventId, bool needAck, uint32_t ackCounter, float value, const CMovementStatus* status) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, eventId);
    event->m_value = value;
    event->m_needAck = needAck;
    event->m_ackCounter = ackCounter;
    bool queued = this->m_moveQueue.Head() != nullptr;

    if (status) {
        event->FromMoveStatus(*status);
    }

    this->m_moveQueue.Enqueue(event);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    if (status && !queued) {
        this->SetInterpolation(MovementGetGlobals()->m_lastUpdateTime);
    }
}

// VERIFY: b4c52bb1 manual
// OFFSET: 0x006ED0F0
void CMovement_C::FlushMoveEvent(CPlayerMoveEvent* event, int32_t a3) {
    uint32_t time = MovementGetGlobals()->m_idleTime;
    event->m_link.Unlink();

    switch (event->m_eventId) {
        case MOVEEVENT_SET_RUN_MODE:
            this->SetRunMode(1);
            break;
        case MOVEEVENT_SET_WALK_MODE:
            this->SetRunMode(0);
            break;
        case MOVEEVENT_SET_RUN_SPEED:
            this->ChangeRunSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_RUN_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_RUN_BACK_SPEED:
            this->ChangeRunBackSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_WALK_SPEED:
            this->ChangeWalkSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_WALK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_SWIM_SPEED:
            this->ChangeSwimSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_SWIM_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_SWIM_BACK_SPEED:
            this->ChangeSwimBackSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_FLIGHT_SPEED:
            this->ChangeFlightSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_FLIGHT_BACK_SPEED:
            this->ChangeFlightBackSpeed(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_TURN_RATE:
            this->ChangeTurnRate(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_TURN_RATE_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_PITCH_RATE:
            this->ChangePitchRate(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_PITCH_RATE_CHANGE_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_GRAVITY_ENABLE:
            this->EnableGravity(1);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_GRAVITY_ENABLE_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_GRAVITY_DISABLE:
            this->EnableGravity(0);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_GRAVITY_DISABLE_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_KNOCK_BACK:
            this->Launch(*reinterpret_cast<const C2Vector*>(&event->m_value), event->m_launchSpeed, event->m_launchVerticalSpeed);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_KNOCK_BACK_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_FEATHER_FALL:
            this->FeatherFall(1);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_FEATHER_FALL_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 0xFF);
            this->PostFeatherFall();
            this->m_unit->OnCollideFalling();
            break;
        case MOVEEVENT_NORMAL_FALL:
            this->FeatherFall(0);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_FEATHER_FALL_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            this->PostFeatherFall();
            this->m_unit->OnCollideFalling();
            break;
        case MOVEEVENT_SET_HOVER:
            this->Hover(1);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_HOVER_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_UNSET_HOVER:
            this->UpdateHoverState(0, !event->m_needAck || this->m_unit->IsActiveMover());
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_HOVER_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_WATER_WALK:
            this->WalkOnWater(1);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_WATER_WALK_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_LAND_WALK:
            this->WalkOnWater(0);

            if (!event->m_needAck || this->m_unit->IsActiveMover()) {
                this->TryStartFalling();
            }

            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_WATER_WALK_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_ROOT:
            if (event->m_needAck && this->IsOnSpline()) {
                this->FinishSpline(0);
            }

            this->TryRoot();
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_MOVE_ROOT_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_UNROOT:
            this->UnRoot(!event->m_needAck || this->m_unit->IsActiveMover());
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_FORCE_MOVE_UNROOT_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_TELEPORT:
            this->m_flags2 ^= (this->m_flags2 ^ event->m_moveExtraFlags) & 0x2040;
            this->Teleport(event->m_transportGuid, event->m_position, event->m_facing, a3, event->m_needAck, event->m_seat);
            this->m_unit->ProcessLocalMoveEvent(time, MSG_MOVE_TELEPORT_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_CAN_FLY:
            this->m_flags |= 0x1000000;
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_FLY_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_UNSET_CAN_FLY:
            if (this->m_flags & 0x2000000) {
                this->StopFlying();
            }

            this->m_flags &= ~0x1000000;
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_FLY_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_TIME_SYNC:
            if (this->m_flags & 0x200) {
                this->m_flags &= ~0x200;
            }

            this->SendTimeSyncResp(event->m_eventTime, event->m_ackCounter);
            break;
        case MOVEEVENT_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY:
            this->m_flags2 |= 0x4000;
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK, event->m_needAck, 1.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY:
            this->m_flags2 &= ~0x4000;
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK, event->m_needAck, 0.0f, event->m_ackCounter, 0, 0xFF);
            break;
        case MOVEEVENT_SET_COLLISION_HEIGHT:
            this->SetCollisionHeight(event->m_value);
            this->m_unit->ProcessLocalMoveEvent(time, CMSG_MOVE_SET_COLLISION_HGT_ACK, event->m_needAck, event->m_value, event->m_ackCounter, 0, 0xFF);
            break;
        default:
            break;
    }

    if (event->unk_0050) {
        this->HeartBeat(event);
    }

    CPlayerMoveQueue::s_freeList.LinkToTail(event);
}

// VERIFY: 7bf10fbf auto
// OFFSET: 0x006ED7E0
void CMovement_C::FlushMoveQueue(int32_t a1, int32_t a2) {
    if (s_flushingQueue) {
        return;
    }

    this->m_flags2 &= 0xE3FF;
    CVehiclePassenger_C* passenger = this->m_unit->m_vehiclePassenger;
    s_flushingQueue = 1;

    if (passenger && passenger->m_deferredTransitionSpline) {
        if (passenger->m_flags & 0x200) {
            passenger->StopWaitingForVehicleTransitionAnim();
        } else {
            passenger->DeleteAllDeferredVehicleTransitionAnimSplines();
        }
    }

    while (this->m_moveQueue.Head()) {
        this->FlushMoveEvent(this->m_moveQueue.Head(), a1);
    }

    if (a2 || !this->m_spline || (this->m_spline->m_flags & 0x400)) {
        this->Halt();
    }

    s_flushingQueue = 0;
}

// VERIFY: 95bc9c30 manual
// OFFSET: 0x006ED990
int32_t CMovement_C::UpdateStatus(uint32_t time, const CMovementStatus& status, MoveEventId eventId, float value) {
    int32_t timeOffset;

    if (!this->UpdateStatusInternal(time, status, timeOffset, 0, 0)) {
        this->AddPlayerSpeedEvent(time + timeOffset, eventId, false, 0, value, &status);
        return 0;
    }

    if (!(status.m_moveFlags & 0x8000000)) {
        if (this->m_spline && (this->m_spline->m_flags & 0x800)) {
            this->m_unit->OnCollideFallLand(0, 1);
        }

        this->RemoveSpline();
    }

    if ((this->m_flags & 0xC010FF) || this->m_moveQueue.Head()) {
        if (!this->m_globalUnitsLink.IsLinked()) {
            CMovementGlobals* globals = MovementGetGlobals();

            if (globals) {
                globals->m_movers.LinkToHead(this);
            }
        }
    } else {
        this->RemoveFromMoversList(1);
    }

    return 1;
}

// VERIFY: 601e11e6 manual
// OFFSET: 0x006EDA60
int32_t CMovement_C::UpdateFacingStatus(uint32_t time, const CMovementStatus& status) {
    int32_t timeOffset;

    if (!this->UpdateStatusInternal(time, status, timeOffset, 0, 0)) {
        this->AddPlayerSpeedEvent(time + timeOffset, MOVEEVENT_SET_FACING, false, 0, 0.0f, &status);
        return 0;
    }

    if (!(status.m_moveFlags & 0x8000000)) {
        if (this->m_spline && (this->m_spline->m_flags & 0x800)) {
            this->m_unit->OnCollideFallLand(0, 1);
        }

        this->RemoveSpline();
    }

    if ((this->m_flags & 0xC010FF) || this->m_moveQueue.Head()) {
        if (!this->m_globalUnitsLink.IsLinked()) {
            CMovementGlobals* globals = MovementGetGlobals();

            if (globals) {
                globals->m_movers.LinkToHead(this);
            }
        }
    } else {
        this->RemoveFromMoversList(1);
    }

    return 1;
}

// VERIFY: a4d55470 auto
// OFFSET: 0x006EDBE0
int32_t CMovement_C::OnRunSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_RUN_SPEED, value)) {
        return 0;
    }

    return this->ChangeRunSpeed(value);
}

// VERIFY: fbc0354f auto
// OFFSET: 0x006EDCD0
int32_t CMovement_C::OnRunBackSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_RUN_BACK_SPEED, value)) {
        return 0;
    }

    return this->ChangeRunBackSpeed(value);
}

// VERIFY: 97a87176 auto
// OFFSET: 0x006EDDC0
int32_t CMovement_C::OnWalkSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_WALK_SPEED, value)) {
        return 0;
    }

    return this->ChangeWalkSpeed(value);
}

// VERIFY: 4cb312d1 auto
// OFFSET: 0x006EDEB0
int32_t CMovement_C::OnSwimSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_SWIM_SPEED, value)) {
        return 0;
    }

    return this->ChangeSwimSpeed(value);
}

// VERIFY: 561910b3 auto
// OFFSET: 0x006EDFA0
int32_t CMovement_C::OnSwimBackSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_SWIM_BACK_SPEED, value)) {
        return 0;
    }

    return this->ChangeSwimBackSpeed(value);
}

// VERIFY: 97328e94 auto
// OFFSET: 0x006EE090
int32_t CMovement_C::OnFlightSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_FLIGHT_SPEED, value)) {
        return 0;
    }

    return this->ChangeFlightSpeed(value);
}

// VERIFY: e7e57f2a auto
// OFFSET: 0x006EE180
int32_t CMovement_C::OnFlightBackSpeedChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_FLIGHT_BACK_SPEED, value)) {
        return 0;
    }

    return this->ChangeFlightBackSpeed(value);
}

// VERIFY: 366fff91 auto
// OFFSET: 0x006EE270
int32_t CMovement_C::OnTurnRateChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_TURN_RATE, value)) {
        return 0;
    }

    return this->ChangeTurnRate(value);
}

// VERIFY: 66f9bfe5 auto
// OFFSET: 0x006EE360
int32_t CMovement_C::OnPitchRateChangeOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_PITCH_RATE, value)) {
        return 0;
    }

    return this->ChangePitchRate(value);
}

// VERIFY: cb8a4ced manual
// OFFSET: 0x006EE550
int32_t CMovement_C::OnSetFacing(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateFacingStatus(time, status)) {
        return 0;
    }

    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: 3c7304ba manual
// OFFSET: 0x006EE590
int32_t CMovement_C::OnSetPitch(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_PITCH, status.m_pitch)) {
        return 0;
    }

    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: f508bc09 manual
// OFFSET: 0x006EE5D0
int32_t CMovement_C::OnMoveRoot(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_ROOT, 0.0f)) {
        return 0;
    }

    this->TryRoot();
    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: 168e7f22 manual
// OFFSET: 0x006EE620
int32_t CMovement_C::OnMoveUnRoot(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_UNROOT, 0.0f)) {
        return 0;
    }

    this->UnRoot(1);

    if ((this->m_flags & 0xC010FF) && !this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    this->m_unit->UnRootEffects();
    return 1;
}

// VERIFY: a945886f manual
// OFFSET: 0x006EE700
int32_t CMovement_C::OnGravityChange(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, (status.m_moveFlags & 0x400) ? MOVEEVENT_GRAVITY_DISABLE : MOVEEVENT_GRAVITY_ENABLE, 0.0f)) {
        return 0;
    }

    this->UpdateAnchors(0);
    this->GravityStateChanged();
    return 1;
}

// VERIFY: 9e468ccc auto
// OFFSET: 0x006EE810
int32_t CMovement_C::OnSetCollisionHeightOther(uint32_t time, const CMovementStatus& status, float value) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_SET_COLLISION_HEIGHT, value)) {
        return 0;
    }

    this->m_collisionHeight = value;
    this->ValidateWorldPosition(CPassenger::GetPosition(this->m_position));
    return 1;
}

// VERIFY: 17386a46 manual
// OFFSET: 0x006EEB80
int32_t CMovement_C::OnStartSwim(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_START_SWIM, 0.0f)) {
        return 0;
    }

    this->StartSwim();
    this->HandlePendingActions();
    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: f6d6b699 auto
// OFFSET: 0x006EEBD0
int32_t CMovement_C::OnStopSwim(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_STOP_SWIM, 0.0f)) {
        return 0;
    }

    this->StopSwim();

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: 5ee7b4e3 auto
// OFFSET: 0x006EEC30
int32_t CMovement_C::OnPitchStart(uint32_t time, const CMovementStatus& status, int32_t up) {
    if (!this->UpdateStatus(time, status, up ? MOVEEVENT_START_PITCH_UP : MOVEEVENT_START_PITCH_DOWN, 0.0f)) {
        return 0;
    }

    this->StartPitch(up);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: 4eecd34a manual
// OFFSET: 0x006EED10
int32_t CMovement_C::OnPitchStop(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_STOP_PITCH, 0.0f)) {
        return 0;
    }

    int32_t result = this->StopPitch();

    if (!(this->m_flags & 0xC010FF) && !this->m_moveQueue.Head()) {
        this->RemoveFromMoversList(1);
    }

    return result;
}

// VERIFY: 6eafa297 auto
// OFFSET: 0x006EEDE0
int32_t CMovement_C::OnSetWaterWalk(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, (status.m_moveFlags & 0x10000000) ? MOVEEVENT_WATER_WALK : MOVEEVENT_LAND_WALK, 0.0f)) {
        return 0;
    }

    if (!(this->m_flags & 0x10000000) && this->TryStartFalling() && !this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: 1cb0d9e1 auto
// OFFSET: 0x006EEEC0
int32_t CMovement_C::OnSetFeatherFall(uint32_t time, const CMovementStatus& status) {
    return this->UpdateStatus(time, status, (status.m_moveFlags & 0x20000000) ? MOVEEVENT_FEATHER_FALL : MOVEEVENT_NORMAL_FALL, 0.0f);
}

// VERIFY: eef93de1 auto
// OFFSET: 0x006EEF60
int32_t CMovement_C::OnMoveHover(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, (status.m_moveFlags & 0x40000000) ? MOVEEVENT_SET_HOVER : MOVEEVENT_UNSET_HOVER, 0.0f)) {
        return 0;
    }

    uint32_t hover = this->m_flags & 0x40000000;

    if (hover) {
        this->Jump(0);
    } else {
        this->TryStartFalling();
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    this->Hover(hover);
    return 1;
}

// VERIFY: c3324cc2 manual
// OFFSET: 0x006EF0D0
int32_t CMovement_C::OnUpdateCanFly(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, (status.m_moveFlags & 0x1000000) ? MOVEEVENT_SET_CAN_FLY : MOVEEVENT_UNSET_CAN_FLY, 0.0f)) {
        return 0;
    }

    if (status.m_moveFlags & 0x1000000) {
        this->m_flags |= 0x1000000;
        return 1;
    }

    if (this->m_flags & 0x2000000) {
        this->StopFlying();
    }

    this->m_flags &= ~0x1000000;
    return 1;
}

// VERIFY: a2673483 manual
// OFFSET: 0x006EF1C0
int32_t CMovement_C::OnUpdateCanTransitionBetweenSwimAndFly(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, (status.m_moveExtraFlags & 0x4000) ? MOVEEVENT_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY : MOVEEVENT_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY, 0.0f)) {
        return 0;
    }

    if (status.m_moveExtraFlags & 0x4000) {
        this->m_flags2 |= 0x4000;
    } else {
        this->m_flags2 &= ~0x4000;
    }

    return 1;
}

// VERIFY: dbacfc0d auto
// OFFSET: 0x006EF5C0
int32_t CMovement_C::OnStartAscendDescend(uint32_t time, const CMovementStatus& status, int32_t ascend) {
    if (!this->UpdateStatus(time, status, ascend ? MOVEEVENT_START_ASCEND : MOVEEVENT_START_DESCEND, 0.0f) || !this->StartAscensionDescension(ascend)) {
        return 0;
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: d6b72520 auto
// OFFSET: 0x006EF630
int32_t CMovement_C::OnStopAscend(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_STOP_ASCEND, 0.0f) || (this->m_flags & 0xF)) {
        return 0;
    }

    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: d8bc09d5 auto
// OFFSET: 0x006EF680
int32_t CMovement_C::OnHeartBeat(uint32_t time, const CMovementStatus& status) {
    return this->UpdateStatus(time, status, MOVEEVENT_HEARTBEAT, 0.0f);
}

// VERIFY: fb49d8f7 auto
// OFFSET: 0x006F0CF0
int32_t CMovement_C::OnMoveStart(uint32_t time, const CMovementStatus& status, int32_t forward) {
    if (!this->UpdateStatus(time, status, forward ? MOVEEVENT_START_FORWARD : MOVEEVENT_START_BACKWARD, 0.0f) || !this->StartMove(forward, 0)) {
        return 0;
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: ed1ed978 auto
// OFFSET: 0x006F0D60
int32_t CMovement_C::OnStrafeStart(uint32_t time, const CMovementStatus& status, int32_t left) {
    if (!this->UpdateStatus(time, status, left ? MOVEEVENT_START_STRAFE_LEFT : MOVEEVENT_START_STRAFE_RIGHT, 0.0f) || !this->StartStrafe(left)) {
        return 0;
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: dc4c0e32 auto
// OFFSET: 0x006F0DD0
int32_t CMovement_C::OnJump(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_JUMP, 0.0f) || !this->Jump(1)) {
        return 0;
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: c7518338 auto
// OFFSET: 0x006F0EB0
int32_t CMovement_C::OnMoveStop(uint32_t time, const CMovementStatus& status) {
    uint32_t oldFlags = this->m_flags;

    if (!this->UpdateStatus(time, status, MOVEEVENT_STOP, 0.0f) || !this->StopMove(oldFlags) || (this->m_flags & 0xC)) {
        return 0;
    }

    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: dc709fa2 manual
// OFFSET: 0x006F0F10
int32_t CMovement_C::OnStrafeStop(uint32_t time, const CMovementStatus& status) {
    uint32_t oldFlags = this->m_flags;

    if (!this->UpdateStatus(time, status, MOVEEVENT_STOP_STRAFE, 0.0f) || !this->StopStrafe(oldFlags) || (this->m_flags & 0xC0000F)) {
        return 0;
    }

    this->RemoveFromMoversList(1);
    return 1;
}

// VERIFY: 6d7dfd89 auto
// OFFSET: 0x006F1010
int32_t CMovement_C::OnTurnStart(uint32_t time, const CMovementStatus& status, int32_t left) {
    if (!this->UpdateStatus(time, status, left ? MOVEEVENT_START_TURN_LEFT : MOVEEVENT_START_TURN_RIGHT, 0.0f)) {
        return 0;
    }

    this->StartTurn(left);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: 0b0c6839 auto
// OFFSET: 0x006F1080
int32_t CMovement_C::OnTurnStop(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_STOP_TURN, 0.0f)) {
        return 0;
    }

    int32_t result = this->StopTurn();
    this->RemoveFromMoversList(1);
    return result;
}

// VERIFY: 1d65bf31 manual
// OFFSET: 0x006F10D0
int32_t CMovement_C::OnSetRunMode(uint32_t time, const CMovementStatus& status, int32_t run) {
    if (!this->UpdateStatus(time, status, run ? MOVEEVENT_SET_RUN_MODE : MOVEEVENT_SET_WALK_MODE, 0.0f)) {
        return 0;
    }

    this->SetRunMode(run);
    return 1;
}

// VERIFY: c9e3d5be manual
// OFFSET: 0x006F1120
int32_t CMovement_C::OnTeleport(uint32_t time, const CMovementStatus& status) {
    if (!this->UpdateStatus(time, status, MOVEEVENT_TELEPORT, 0.0f)) {
        return 0;
    }

    if (status.m_transportGuid.m_guid) {
        this->Teleport(status.m_transportGuid, status.m_transportPosition, status.m_transportFacing, 1, 0, status.m_transportSeat);
    } else {
        this->Teleport(WOWGUID{ 0 }, status.m_position, status.m_facing, 1, 0, 0xFF);
    }

    if (this->m_flags & 0xC0100F) {
        this->UpdateHeartbeatTimerA(MovementGetGlobals()->m_idleTime);
    }

    return 1;
}

// VERIFY: 64beef1a manual
// OFFSET: 0x006EB020
void CMovement_C::OnSplineStopSwim() {
    this->StopSwim();

    if (((this->m_flags & 0xC010FF) || this->m_moveQueue.Head()) && !this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }
}

// VERIFY: 02499f55 auto
// OFFSET: 0x006EB060
void CMovement_C::OnSplineToggleHover(int32_t hover) {
    if (hover) {
        this->Jump(0);
    } else {
        this->TryStartFalling();
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    this->Hover(hover);
}

// VERIFY: c5691530 auto
// OFFSET: 0x006EB9D0
void CMovement_C::OnSplineRoot() {
    this->TryRoot();

    if (this->m_spline) {
        this->SnapToSpline(this->m_spline->m_finalDestination, 0);
        this->m_spline->m_start = this->m_spline->m_duration;
        this->m_spline->m_flags |= 0x100;
    }

    this->RemoveFromMoversList(1);
}

// VERIFY: f48a070e manual
// OFFSET: 0x006EBA20
void CMovement_C::OnSplineUnRoot() {
    this->UnRoot(1);

    if ((this->m_flags & 0xC010FF) && !this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    this->m_unit->UnRootEffects();
}

// VERIFY: 341da3f2 auto
// OFFSET: 0x006EBC20
int32_t CMovement_C::OnSplineEnableGravity(int32_t enable) {
    if (!this->EnableGravity(enable)) {
        return 0;
    }

    this->GravityStateChanged();
    return 1;
}

// VERIFY: d3d4d3de auto
// OFFSET: 0x006EBF70
void CMovement_C::OnSplineStartSwim() {
    this->StartSwim();
    this->HandlePendingActions();
    this->RemoveFromMoversList(1);
}

// VERIFY: 7c5d2bd6 manual
// OFFSET: 0x006E96C0
void CMovement_C::SetSplineParabolic(float verticalAcceleration, uint32_t effectStartTime) {
    this->m_spline->m_verticalAcceleration = verticalAcceleration;
    this->m_spline->m_effectStartTime = effectStartTime;

    if (this->m_spline->m_effectStartTime) {
        this->SetJumpSplineInAir(0);
        return;
    }

    this->SetJumpSplineInAir(1);

    if (this->m_spline->m_flags & 0x4000) {
        this->m_unit->ProcessLocalMoveEvent(MovementGetGlobals()->m_lastUpdateTime, CMSG_MOVE_KNOCK_BACK_ACK, false, 0.0f, 0, 0, 0xFF);
    } else {
        this->m_unit->ProcessLocalMoveEvent(MovementGetGlobals()->m_lastUpdateTime, MSG_MOVE_JUMP, false, 0.0f, 0, 0, 0xFF);
    }
}

// VERIFY: e1e1b87f auto
// OFFSET: 0x006E9780
void CMovement_C::SetSplineAnimation(uint8_t animType, uint32_t effectStartTime) {
    this->m_spline->m_flags = (this->m_spline->m_flags & 0xFFFFFF00) | animType;
    this->m_spline->m_effectStartTime = effectStartTime;

    if (!effectStartTime) {
        this->SetAnimTierInTrans(1);
        this->m_unit->SetAnimType(animType);
    }
}

// VERIFY: ac05273e auto
// OFFSET: 0x006EB650
int32_t CMovement_C::TryStartFallingLocal() {
    if (!this->TryStartFalling()) {
        return 0;
    }

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: b800678d manual
// OFFSET: 0x006EB680
int32_t CMovement_C::OnSpline(const C3Vector* points, uint32_t pointCount, uint32_t duration, uint32_t splineFlags, uint32_t splineId) {
    if (splineFlags & 0x1800000) {
        return 0;
    }

    this->RemoveFromMoversList(0);

    if ((this->m_flags & 0x100800) && !(splineFlags & 0xA00)) {
        return 0;
    }

    this->AddSpline(points[pointCount - 1]);
    CMovementShared::OnSpline(MovementGetGlobals()->m_lastUpdateTime, points, pointCount, duration, splineFlags, splineId);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    this->m_spline->m_spline.Pos(1.0f, this->m_spline->m_finalDestination, C3Spline::EVAL_ARCLENGTH);
    return 1;
}

// VERIFY: 64ba484e manual
// OFFSET: 0x006EE510
void CMovement_C::SetFacingAndFlush(float facing, int32_t flush) {
    this->SetFacing(facing);

    if (!(this->m_flags & 0xC010FF) && flush) {
        this->FlushMoveQueue(1, 1);
        this->RemoveFromMoversList(1);
    }
}

// VERIFY: 1d0f2200 auto
// OFFSET: 0x006F0C70
void CMovement_C::ForceSetTransport(WOWGUID transport, uint8_t seat, int32_t a3) {
    if (this->ForceSetTransportInt(transport, seat, a3) && this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
        this->m_unit->SendMovementUpdate(MovementGetGlobals()->m_lastUpdateTime, CMSG_MOVE_CHANGE_TRANSPORT, 0.0f, 0, 0, 0xFF);
    }
}

// VERIFY: 05a10712 auto
// OFFSET: 0x006F11B0
void CMovement_C::OnSplineSnap(uint32_t splineId, const C3Vector& destination, uint32_t splineFlags, int32_t flush) {
    this->AddSpline(destination);

    if (flush) {
        this->FlushMoveQueue(0, 1);
        this->RemoveFromMoversList(0);
    }

    this->ResetSpline(splineId, splineFlags);
    this->SnapToSpline(this->m_spline->m_finalDestination, splineFlags & 0x1800000);
    this->m_spline->m_start = this->m_spline->m_duration;
    this->m_spline->m_flags |= 0x100;
    this->OnSplineStop(MovementGetGlobals()->m_idleTime);
}

// VERIFY: ada38338 manual
// OFFSET: 0x006E9050
void CPlayerMoveEvent::FromMoveStatus(const CMovementStatus& status) {
    this->m_transportGuid = status.m_transportGuid;
    this->m_moveFlags = status.m_moveFlags;
    this->m_moveExtraFlags = status.m_moveExtraFlags;

    if (status.m_transportGuid.m_guid) {
        this->m_position = status.m_transportPosition;
        this->m_facing = status.m_transportFacing;
        this->m_seat = status.m_transportSeat;
        this->m_pitch = status.m_pitch;
        this->unk_0050 = 1;
    } else {
        this->m_position = status.m_position;
        this->m_facing = status.m_facing;
        this->unk_0050 = 1;
        this->m_pitch = status.m_pitch;
    }

    this->m_fallTime = status.m_fallTime;
}

// VERIFY: 90a2e8c4 manual
// OFFSET: 0x006EA550
void CMovement_C::Extrapolate(uint32_t time, uint32_t elapsed) {
    if (!elapsed || !(this->m_flags & 0xC0100F) || (this->m_spline && !(this->m_spline->m_flags & 0x400))) {
        return;
    }

    C3Vector move = { 0.0f, 0.0f, 0.0f };

    if (elapsed > 250) {
        for (uint32_t steps = (elapsed - 251) / 250 + 1; steps; steps--) {
            if (this->m_flags & 0xC0000F) {
                this->m_anchorElapsedMs += 250;
            }

            this->PlotUnitMovement(250, &move);

            if (World::IsValidPosition(this->m_position.x + move.x, this->m_position.y + move.y, this->m_position.z + move.z, 0.0f)) {
                this->RequestMove(time, 250, move);
            }

            elapsed -= 250;
            time += 250;
        }
    }

    if (this->m_flags & 0xC0000F) {
        this->m_anchorElapsedMs += elapsed;
    }

    this->PlotUnitMovement(this->m_anchorElapsedMs, &move);

    if (World::IsValidPosition(this->m_position.x + move.x, this->m_position.y + move.y, this->m_position.z + move.z, 0.0f)) {
        this->RequestMove(time, elapsed, move);
    }
}

// VERIFY: 760d3d30 auto
// OFFSET: 0x006EC950
void CMovement_C::AddPlayerMoveEvent(uint32_t eventTime, MoveEventId eventId, bool needAck, uint32_t ackCounter, const C2Vector& direction, float speed, float verticalSpeed, const CMovementStatus* status) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, eventId);
    event->m_ackCounter = ackCounter;
    event->m_needAck = needAck;
    event->m_value = direction.x;
    event->m_launchDirY = direction.y;
    event->m_launchSpeed = speed;
    event->m_launchVerticalSpeed = verticalSpeed;
    bool queued = this->m_moveQueue.Head() != nullptr;

    if (status) {
        event->FromMoveStatus(*status);
    }

    this->m_moveQueue.Enqueue(event);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    if (status && !queued) {
        this->SetInterpolation(MovementGetGlobals()->m_lastUpdateTime);
    }
}

// VERIFY: 0708a62a auto
// OFFSET: 0x006ECA00
void CMovement_C::AddPlayerTeleportEvent(uint32_t eventTime, bool needAck, uint32_t ackCounter, const CMovementStatus& status) {
    CPlayerMoveEvent* event = CPlayerMoveQueue::AllocEvent(eventTime, MOVEEVENT_TELEPORT);
    event->m_needAck = needAck;
    event->m_ackCounter = ackCounter;
    event->m_pitch = status.m_pitch;
    event->m_transportGuid = status.m_transportGuid;
    event->m_moveFlags = status.m_moveFlags;
    event->m_moveExtraFlags = status.m_moveExtraFlags;

    if (status.m_transportGuid.m_guid) {
        event->m_position = status.m_transportPosition;
        event->m_facing = status.m_transportFacing;
        event->m_seat = status.m_transportSeat;
    } else {
        event->m_position = status.m_position;
        event->m_facing = status.m_facing;
    }

    this->m_moveQueue.Enqueue(event);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    if (status.m_moveExtraFlags & 0x2000) {
        this->m_unit->VehiclePassengerStartWaitingForVehicleTransitionAnim(status.m_transportGuid.m_guid, status.m_transportSeat);
    }
}

// VERIFY: 69b75879 auto
// OFFSET: 0x006ED890
void CMovement_C::OnTeleportLocal(uint32_t eventTime, uint32_t ackCounter, const CMovementStatus& status) {
    this->AddPlayerTeleportEvent(eventTime, true, ackCounter, status);
}

// VERIFY: 72a548fd manual
// OFFSET: 0x006ED8B0
int32_t CMovement_C::UpdateLaunchStatus(uint32_t time, const CMovementStatus& status, MoveEventId eventId, const C2Vector& direction, float speed, float verticalSpeed) {
    int32_t timeOffset;

    if (!this->UpdateStatusInternal(time, status, timeOffset, 0, 0)) {
        this->AddPlayerMoveEvent(time + timeOffset, eventId, false, 0, direction, speed, verticalSpeed, &status);
        return 0;
    }

    if (!(status.m_moveFlags & 0x8000000)) {
        if (this->m_spline && (this->m_spline->m_flags & 0x800)) {
            this->m_unit->OnCollideFallLand(0, 1);
        }

        this->RemoveSpline();
    }

    if ((this->m_flags & 0xC010FF) || this->m_moveQueue.Head()) {
        if (!this->m_globalUnitsLink.IsLinked()) {
            CMovementGlobals* globals = MovementGetGlobals();

            if (globals) {
                globals->m_movers.LinkToHead(this);
            }
        }
    } else {
        this->RemoveFromMoversList(1);
    }

    return 1;
}

// VERIFY: 2804acae manual
// OFFSET: 0x006EE870
void CMovement_C::OnSetActiveMover() {
    this->FlushMoveQueue(0, 0);
    CMovementGlobals* globals = MovementGetGlobals();

    if (this->m_transportGuid.m_guid) {
        CGGameObject_C* transport = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(this->m_transportGuid, TYPEMASK_GAMEOBJECT, __FILE__, __LINE__));

        if (transport) {
            uint32_t transportTime = transport->m_typeData->GetTransportUpdateTime();
            globals->m_transportTime2 = globals->m_transportTime;
            globals->m_transportTime = transportTime;
        }
    }

    if (this->m_spline && (this->m_spline->m_flags & 0x400)) {
        this->OnSplineStop(globals->m_lastUpdateTime);
    }

    this->m_flags &= ~0x80000000;

    if (!this->m_spline || (this->m_spline->m_flags & 0x400)) {
        this->OnFallLocal(globals->m_lastUpdateTime);
    }
}

// VERIFY: d88e4018 auto
// OFFSET: 0x006EE920
void CMovement_C::OnUnsetActiveMover() {
    this->FlushMoveQueue(1, 1);
    this->m_flags &= ~0x80000000;
}

// VERIFY: aa2810cf manual
// OFFSET: 0x006F0E30
int32_t CMovement_C::OnLaunch(uint32_t time, const CMovementStatus& status, const C2Vector& direction, float speed, float verticalSpeed) {
    if (!this->UpdateLaunchStatus(time, status, MOVEEVENT_KNOCK_BACK, direction, speed, verticalSpeed)) {
        return 0;
    }

    this->Launch(direction, speed, verticalSpeed);

    if (!this->m_globalUnitsLink.IsLinked()) {
        CMovementGlobals* globals = MovementGetGlobals();

        if (globals) {
            globals->m_movers.LinkToHead(this);
        }
    }

    return 1;
}

// VERIFY: a537acac manual
// OFFSET: 0x0075EFE0
void CMovement_C::SnapToGroundIfCloseEnough() {
    if (!(this->m_flags & MOVEMENTFLAG_ROOT) || this->m_transportGuid.m_guid) {
        return;
    }

    C3Vector hit(0.0f, 0.0f, 0.0f);
    uint32_t flags = this->GetFacetQueryFlags() & ~0x100000;
    float distance = 1.0f;
    C3Vector end(this->m_position.x, this->m_position.y, this->m_position.z - 0.2f);

    if (World::Intersect(this->m_position, end, &hit, &distance, flags, nullptr)) {
        this->m_position.z = hit.z;
        this->BuildCollisionBox(this->m_position, s_queryBox);

        if (World::GetFacets(&s_queryBox, &s_moveFacets, flags, nullptr)) {
            this->GroundNormal();
        }
    }
}

// VERIFY: d0faedce manual
// OFFSET: 0x006EE3A0
void CMovement_C::OnSetRawFacingLocal(uint32_t eventTime, float facing) {
    float min;
    float max;

    if (this->ComputeLegalRawFacingRange(min, max)) {
        CMovement_C::WrapFacingToRange(facing, min, max);

        if (facing < min) {
            facing = min;
        } else if (max <= facing) {
            facing = max;
        }
    }

    this->AddPlayerMoveEvent(eventTime, MOVEEVENT_SET_FACING, true, 0, facing, 0.0f, 0);
}

// VERIFY: 7d47e49c auto
// OFFSET: 0x006EA520
int32_t CMovement_C::UpdateTransportStatus(const CClientObjCreate& create) {
    return this->UpdateTransportStatus(WOWGUID{ CMovementShared::UpdateTransportStatus(create.m_move.m_status) }, create.m_move.m_status.m_transportSeat);
}

// VERIFY: 81c65e19 auto
// OFFSET: 0x006EC390
void CMovement_C::Disable() {
    CMovementShared::Disable();
    this->m_moveQueue.FreeAll();
}

// VERIFY: 4286b6ab manual
// OFFSET: 0x00760720
void CMovement_C::CollideWithTransport(uint32_t time, uint32_t elapsed, float distance, const C3Vector& direction, const C44Matrix& transportMatrix) {
    C3Vector position = this->m_position * transportMatrix;
    World::IsValidPosition(position.x, position.y, position.z, 0.0f);
    CAaBox box;
    box.b = C3Vector(0.0f, 0.0f, 0.0f);
    box.t = C3Vector(0.0f, 0.0f, 0.0f);
    this->BuildCollisionBox(position, box);
    s_queryBox = box;
    CAaBox moved;
    moved.b.x = box.b.x + direction.x * distance;
    moved.b.y = box.b.y + direction.y * distance;
    moved.b.z = box.b.z + direction.z * distance;
    moved.t.x = box.t.x + direction.x * distance;
    moved.t.y = box.t.y + direction.y * distance;
    moved.t.z = box.t.z + direction.z * distance;
    s_queryBox |= moved;
    uint32_t flags = this->m_unit->IsClientControlled() ? 0x111 : 0x2111;
    CGUnit_C* unit = this->m_unit;

    if ((unit->m_obj->m_type & TYPEMASK_PLAYER) && (static_cast<CGPlayer_C*>(unit)->m_player->m_playerFlags & 0x10)) {
        flags |= 0x8000;
    }

    if (!World::GetFacets(&box, &s_queryBox, &s_moveFacets, flags, nullptr)) {
        return;
    }

    C4Plane planes[7];

    for (uint32_t i = 0; i < 7; i++) {
        planes[i].n = C3Vector(0.0f, 0.0f, 1.0f);
        planes[i].d = 0.0f;
    }

    uint32_t facetIndex;
    uint32_t planeCount;
    float moveDist;
    this->DistanceToMove(&s_moveFacets, direction, distance, &facetIndex, planes, &planeCount, &moveDist, &position);

    if (facetIndex == s_moveFacets.facets.m_count) {
        return;
    }

    if (this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        return;
    }

    CGGameObject_C* transport = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(this->m_transportGuid, TYPEMASK_GAMEOBJECT, __FILE__, __LINE__));
    uint32_t eventTime = static_cast<uint32_t>(static_cast<int64_t>(moveDist / distance * static_cast<float>(elapsed) + 0.5f)) + transport->m_transportTimeOffset - elapsed + time;
    uint32_t transportTime = transport->m_typeData->GetSequenceTimeFromRealTime(eventTime);
    CMovementGlobals* globals = MovementGetGlobals();
    uint32_t savedTime = globals->m_transportTime;
    globals->m_transportTime2 = savedTime;
    globals->m_transportTime = transportTime;
    C44Matrix inverse = transportMatrix.AffineInverse();
    C3Vector localDir = TransformDirection(direction, inverse);
    C3Vector move(localDir.x * moveDist, localDir.y * moveDist, localDir.z * moveDist);
    this->m_position.x = move.x + this->m_position.x;
    this->m_position.y = move.y + this->m_position.y;
    this->m_position.z = move.z + this->m_position.z;
    this->m_fallStartZ = move.z + this->m_fallStartZ;
    this->ForceSetTransport(WOWGUID(), 0xFF, 0);
    this->QueueFallEvent(eventTime);
    globals->m_transportTime2 = globals->m_transportTime;
    globals->m_transportTime = savedTime;
}
