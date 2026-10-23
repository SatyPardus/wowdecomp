#include "ShipPath.h"
#include "Engine/Source/Tempest/CMath.h"
#include <malloc.h>
#include <new>
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/TaxiPathNodeRec.h"
#include <cmath>
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/DB/Rec/TransportPhysicsRec.h"
#include "WoW/Source/Object/ObjectClient/Types.h"

// INLINED
ShipPath::ShipPath() {
    this->m_physics = nullptr;
    this->m_taxiPathId = 0xFFFFFFFF;
    this->m_moveSpeed = 0.0f;
    this->m_totalPathTime = 0;
    this->m_accelRate = 0.0f;
    this->m_waitTime = 0;
    this->m_timeOffset = 0;
    this->m_waiting = 0;
    this->m_waitReached = 0;
    this->unused_003E = 0;
}

// VERIFY: 09b64bdb auto
// OFFSET: 0x007F92F0
void ShipPath::Init(uint32_t taxiPathId, float moveSpeed, float accelRate, const TransportPhysicsRec* physics) {
    this->m_moveSpeed = moveSpeed;
    this->m_taxiPathId = taxiPathId;
    this->m_totalPathTime = 0;
    this->m_accelRate = accelRate;
    this->BuildLegs();
    this->m_physics = physics;
}

// VERIFY: 41a1523f manual
// OFFSET: 0x007F7FC0
void ShipPath::SetTotalPathTime(uint32_t time) {
    this->m_totalPathTime = time;

    if (this->m_legs.Count() > 0) {
        this->m_legs.Top()->m_endTime = time;
    }
}

// VERIFY: b2be9152 auto
// OFFSET: 0x007F7AD0
static int32_t FindFirstPathNode(uint32_t pathId) {
    int32_t high = g_taxiPathNodeDB.m_numRecords;
    int32_t low = 0;

    if (high <= 0) {
        return -1;
    }

    do {
        int32_t mid = (high + low) / 2;
        TaxiPathNodeRec* records = g_taxiPathNodeDB.m_numRecords <= 0 ? nullptr : g_taxiPathNodeDB.m_records;

        if (records[mid].m_pathID >= static_cast<int32_t>(pathId)) {
            high = mid;
        } else {
            low = mid + 1;
        }
    } while (low < high);

    if (low < g_taxiPathNodeDB.m_numRecords) {
        TaxiPathNodeRec* records = g_taxiPathNodeDB.m_numRecords <= 0 ? nullptr : g_taxiPathNodeDB.m_records;

        if (records[low].m_pathID == static_cast<int32_t>(pathId)) {
            return low;
        }
    }

    return -1;
}

// VERIFY: 3e217ff8 manual
// OFFSET: 0x007F8180
template <>
void ShipPathArray<C3Vector>::SetCount(uint32_t count) {
    if (count == this->m_count) {
        return;
    }

    for (uint32_t i = this->m_count; i < count; i++) {
        new (&this->m_data[i]) C3Vector();
    }

    this->m_count = count;
}

// VERIFY: 7897cce3 manual
// OFFSET: 0x007F90F0
void ShipPath::BuildLegs() {
    ShipPathArray<C3Vector> points;
    points.m_data = static_cast<C3Vector*>(alloca(0x3000));
    points.m_count = 0;
    ShipPathArray<ShipPathPause> pauses;
    pauses.m_data = static_cast<ShipPathPause*>(alloca(0x2000));
    pauses.m_count = 0;
    ShipPathArray<ShipPathEvent> events;
    events.m_data = static_cast<ShipPathEvent*>(alloca(0x3000));
    this->m_totalPathTime = 0;
    this->m_events.m_count = 0;
    events.m_count = 0;
    int32_t leg = -1;
    int32_t index = FindFirstPathNode(this->m_taxiPathId);

    if (index == -1) {
        return;
    }

    TaxiPathNodeRec* node = nullptr;

    if (index >= 0 && index < g_taxiPathNodeDB.m_numRecords) {
        node = &g_taxiPathNodeDB.m_records[index];
    }

    int32_t teleport = 0;

    while (true) {
        if (leg < 0 || node->m_continentID != this->m_legs.m_data[leg].m_mapId || teleport) {
            if (leg >= 0) {
                this->BuildLeg(&points, &pauses, &events, leg);
            }

            leg++;
            this->m_legs.SetCount(leg + 1);
            this->m_legs.m_data[leg].m_mapId = node->m_continentID;
            points.SetCount(0);

            if (pauses.m_count) {
                pauses.m_count = 0;
            }

            if (events.m_count) {
                events.m_count = 0;
            }
        }

        if ((node->m_flags & 0x2) && points.m_count) {
            ShipPathPause* pause = &pauses.m_data[pauses.m_count++];

            if (pause) {
                pause->m_point = points.m_count;
                pause->m_delay = node->m_delay * 1000;
            }
        }

        if (node->m_arrivalEventID || node->m_departureEventID) {
            ShipPathEvent* event = &events.m_data[events.m_count++];

            if (event) {
                event->m_point = points.m_count;
                event->m_arrivalEvent = node->m_arrivalEventID;
                event->m_departureEvent = node->m_departureEventID;
            }
        }

        C3Vector* point = &points.m_data[points.m_count++];

        if (point) {
            point->x = node->m_loc[0];
            point->y = node->m_loc[1];
            point->z = node->m_loc[2];
        }

        teleport = node->m_flags & 0x1;
        index++;

        if (index < 0 || index >= g_taxiPathNodeDB.m_numRecords) {
            break;
        }

        node = &g_taxiPathNodeDB.m_records[index];

        if (node->m_pathID != static_cast<int32_t>(this->m_taxiPathId)) {
            break;
        }
    }

    if (points.m_count) {
        this->BuildLeg(&points, &pauses, &events, leg);
    }
}

// VERIFY: b35e3ee1 manual
// OFFSET: 0x007F7A60
uint32_t ShipPath::LegTimeAccel(float distance) {
    float invAccel = 1.0f / this->m_accelRate;
    float accelTime = invAccel * this->m_moveSpeed;
    float accelDistance = this->m_moveSpeed * 0.5f * accelTime;
    float time;

    if (0.5f * distance <= accelDistance) {
        float root = sqrtf(invAccel * distance);
        time = root + root;
    } else {
        time = 2.0f * accelTime + (distance - accelDistance * 2.0f) / this->m_moveSpeed;
    }

    return static_cast<uint32_t>(time * 1000.0f);
}

// VERIFY: db975c72 manual
// OFFSET: 0x007F8660
void ShipPath::BuildLeg(ShipPathArray<C3Vector>* points, ShipPathArray<ShipPathPause>* pauses, ShipPathArray<ShipPathEvent>* events, int32_t legIndex) {
    ShipPathLeg* leg = &this->m_legs.m_data[legIndex];
    leg->m_spline.SetPoints(points->m_data, points->m_count);
    leg = &this->m_legs.m_data[legIndex];
    leg->m_segments.SetCount(pauses->m_count + 1);
    float prevDistance = 0.0f;
    this->m_legs.m_data[legIndex].m_duration = 0;
    uint32_t eventCount = this->m_events.m_count;
    uint32_t accumDelay = 0;
    uint32_t eventIndex = 0;
    this->m_events.SetCount(eventCount + 2 * events->m_count);
    uint32_t i;

    for (i = 0; i < pauses->m_count; i++) {
        if (pauses->m_data[i].m_point >= points->m_count - 1) {
            break;
        }

        for (; eventIndex < events->m_count; eventIndex++) {
            ShipPathEvent* event = &events->m_data[eventIndex];

            if (event->m_point > pauses->m_data[i].m_point) {
                break;
            }

            float distance = this->m_legs.m_data[legIndex].m_spline.LengthBeforePoint(event->m_point) - prevDistance;
            float invAccel = 1.0f / this->m_accelRate;
            float accelTime = this->m_moveSpeed * invAccel;
            float time;

            if (i) {
                float accelDistance = this->m_moveSpeed * 0.5f * accelTime;

                if (accelDistance >= 0.5f * distance) {
                    float root = sqrtf(distance * invAccel);
                    time = root + root;
                } else {
                    time = 2.0f * accelTime + (distance - accelDistance * 2.0f) / this->m_moveSpeed;
                }
            } else {
                float accelDistance = this->m_moveSpeed * 0.5f * accelTime;

                if (accelDistance >= distance) {
                    time = sqrtf(distance * invAccel + distance * invAccel);
                } else {
                    time = accelTime + (distance - accelDistance) / this->m_moveSpeed;
                }
            }

            uint32_t eventTime = static_cast<uint32_t>(time * 1000.0f);
            uint32_t delay = event->m_point == pauses->m_data[i].m_point ? pauses->m_data[i].m_delay : 0;
            uint32_t base = accumDelay + eventTime + this->m_legs.m_data[legIndex].m_duration;

            if (event->m_arrivalEvent) {
                this->m_events.m_data[eventCount].m_time = base + this->m_totalPathTime;
                this->m_events.m_data[eventCount].m_eventId = event->m_arrivalEvent;
                eventCount++;
            }

            if (event->m_departureEvent) {
                this->m_events.m_data[eventCount].m_time = delay + base + this->m_totalPathTime;
                this->m_events.m_data[eventCount].m_eventId = event->m_departureEvent;
                eventCount++;
            }
        }

        float pauseDistance = this->m_legs.m_data[legIndex].m_spline.LengthBeforePoint(pauses->m_data[i].m_point);
        float distance = pauseDistance - prevDistance;
        uint32_t segmentTime;

        if (i) {
            segmentTime = this->LegTimeAccel(distance);
        } else {
            float invAccel = 1.0f / this->m_accelRate;
            float accelTime = this->m_moveSpeed * invAccel;
            float accelDistance = this->m_moveSpeed * 0.5f * accelTime;
            float time = accelDistance >= distance ? sqrtf(distance * invAccel + distance * invAccel) : accelTime + (distance - accelDistance) / this->m_moveSpeed;
            segmentTime = static_cast<uint32_t>(time * 1000.0f);
        }

        this->m_legs.m_data[legIndex].m_duration += segmentTime;
        prevDistance = pauseDistance;
        ShipPathSegment* segment = &this->m_legs.m_data[legIndex].m_segments.m_data[i];
        segment->m_time = accumDelay + this->m_legs.m_data[legIndex].m_duration;
        segment->m_distance = pauseDistance;
        segment->m_delay = pauses->m_data[i].m_delay;
        accumDelay += pauses->m_data[i].m_delay;
    }

    for (; eventIndex < events->m_count; eventIndex++) {
        ShipPathEvent* event = &events->m_data[eventIndex];
        float distance = this->m_legs.m_data[legIndex].m_spline.LengthBeforePoint(event->m_point) - prevDistance;
        uint32_t eventTime;

        if (i) {
            float invAccel = 1.0f / this->m_accelRate;
            float accelTime = this->m_moveSpeed * invAccel;
            float accelDistance = this->m_moveSpeed * 0.5f * accelTime;
            float time = accelDistance >= distance ? sqrtf(distance * invAccel + distance * invAccel) : accelTime + (distance - accelDistance) / this->m_moveSpeed;
            eventTime = static_cast<uint32_t>(time * 1000.0f);
        } else {
            eventTime = static_cast<uint32_t>(distance / this->m_moveSpeed * 1000.0f);
        }

        uint32_t base = accumDelay + eventTime + this->m_legs.m_data[legIndex].m_duration;

        if (event->m_arrivalEvent) {
            this->m_events.m_data[eventCount].m_time = base + this->m_totalPathTime;
            this->m_events.m_data[eventCount].m_eventId = event->m_arrivalEvent;
            eventCount++;
        }

        if (event->m_departureEvent) {
            this->m_events.m_data[eventCount].m_time = base + this->m_totalPathTime;
            this->m_events.m_data[eventCount].m_eventId = event->m_departureEvent;
            eventCount++;
        }
    }

    this->m_events.SetCount(eventCount);
    float length = this->m_legs.m_data[legIndex].m_spline.cachedLength;
    float distance = length - prevDistance;
    uint32_t lastTime;

    if (i) {
        float invAccel = 1.0f / this->m_accelRate;
        float accelTime = this->m_moveSpeed * invAccel;
        float accelDistance = this->m_moveSpeed * 0.5f * accelTime;
        float time = accelDistance >= distance ? sqrtf(distance * invAccel + distance * invAccel) : accelTime + (distance - accelDistance) / this->m_moveSpeed;
        lastTime = static_cast<uint32_t>(time * 1000.0f);
    } else {
        lastTime = static_cast<uint32_t>(distance / this->m_moveSpeed * 1000.0f);
    }

    leg = &this->m_legs.m_data[legIndex];
    leg->m_duration += lastTime;
    ShipPathSegment* last = &leg->m_segments.m_data[i];
    last->m_time = accumDelay + leg->m_duration;
    last->m_distance = length;
    last->m_delay = 0;
    leg->m_startTime = this->m_totalPathTime;
    this->m_totalPathTime += leg->m_segments.m_data[i].m_time;

    for (uint32_t j = 0; j < leg->m_segments.m_count; j++) {
        leg->m_segments.m_data[j].m_time += leg->m_startTime;
    }

    leg->m_endTime = this->m_totalPathTime;
}

// VERIFY: c8996cba manual
// OFFSET: 0x007F77D0
bool ShipPath::TimeInRange(uint32_t start, uint32_t end, uint32_t time) {
    if (end == start) {
        return end == time;
    }

    if (end <= start) {
        if (time > end && time <= start) {
            return false;
        }
    } else if (time <= start || time > end) {
        return false;
    }

    return true;
}

// VERIFY: c709f2c7 manual
// OFFSET: 0x007F7810
uint32_t ShipPath::GetPathTime(uint32_t time) const {
    uint32_t total = this->m_totalPathTime;

    if (!total) {
        return 0;
    }

    uint32_t pathTime = (time + this->m_timeOffset) % total;

    if (this->m_waitReached) {
        return this->m_waitTime;
    }

    return pathTime;
}

// VERIFY: 96558271 auto
// OFFSET: 0x007F7840
uint32_t ShipPath::UpdatePathTime(uint32_t time, uint32_t elapsed, int32_t absolute) {
    uint32_t total = this->m_totalPathTime;

    if (!total) {
        return 0;
    }

    if (absolute) {
        return time % total;
    }

    uint32_t pathTime = (time + this->m_timeOffset) % total;

    if (this->m_waiting) {
        if (this->m_waitReached) {
            return this->m_waitTime;
        }

        uint32_t waitTime = this->m_waitTime;

        if (this->TimeInRange(waitTime, (waitTime + elapsed) % total, pathTime)) {
            pathTime = waitTime;
            this->m_waitReached = 1;
        }
    }

    return pathTime;
}

// VERIFY: 316a0013 manual
// OFFSET: 0x007F7660
float ShipPath::GetDecelDistance(float t, float segTime, uint32_t* sequence, float* speed) {
    float accelTime = this->m_moveSpeed / this->m_accelRate;
    float decelTime = segTime >= accelTime ? accelTime : segTime;
    float cruiseTime = segTime - decelTime;

    if (t <= cruiseTime) {
        *sequence = ANIM_SHIP_MOVING;
        *speed = this->m_moveSpeed;
        return t * this->m_moveSpeed;
    }

    float startSpeed = decelTime * this->m_accelRate;
    float dt = t - cruiseTime;
    *speed = startSpeed - this->m_accelRate * dt;
    *sequence = ANIM_SHIP_STOP;
    return dt * ((startSpeed + startSpeed - this->m_accelRate * dt) * 0.5f) + cruiseTime * this->m_moveSpeed;
}

// VERIFY: b5ad35a3 manual
// OFFSET: 0x007F76F0
float ShipPath::GetAccelDecelDistance(float t, float segTime, uint32_t* sequence, float* speed) {
    float accelTime = this->m_moveSpeed / this->m_accelRate;
    float rampTime = segTime * 0.5f;

    if (rampTime >= accelTime) {
        rampTime = accelTime;
    }

    if (t <= segTime - rampTime) {
        if (rampTime >= t) {
            float current = this->m_accelRate * t;
            *speed = current;
            *sequence = ANIM_SHIP_START;
            return 0.5f * current * t;
        }

        *sequence = ANIM_SHIP_MOVING;
        *speed = this->m_moveSpeed;
        return accelTime * (0.5f * this->m_moveSpeed) + (t - accelTime) * this->m_moveSpeed;
    }

    float peak = this->m_accelRate * rampTime;
    float dt = t - (segTime - rampTime);
    *speed = peak - this->m_accelRate * dt;
    *sequence = ANIM_SHIP_STOP;
    return rampTime * (0.5f * peak) + dt * ((peak * 2.0f - this->m_accelRate * dt) * 0.5f) + (segTime - 2.0f * rampTime) * this->m_moveSpeed;
}

// VERIFY: 5639ad48 manual
// OFFSET: 0x007F7B30
void ShipPath::GetRollAndPitch(uint32_t time, float speed, float turn, C3Vector* position, float* roll, float* pitch) {
    const TransportPhysicsRec* physics = this->m_physics;
    float damp;

    if (speed >= physics->m_speedDampThresh) {
        damp = 1.0f;
    } else {
        float scale = sqrtf(speed * (1.0f / physics->m_speedDampThresh));
        damp = scale * ((1.0f - physics->m_speedDamp) * scale) + physics->m_speedDamp;
    }

    if (fabsf(physics->m_waveTimeScale) >= 0.00000023841858f) {
        uint32_t period = static_cast<uint32_t>(6283.1855f / physics->m_waveTimeScale);
        position->z = sinf(static_cast<float>(time % period) * TWO_PI / static_cast<float>(period)) * physics->m_waveAmp * damp + position->z;
    }

    float bankTurn;

    if (physics->m_maxBankTurnSpeed >= turn) {
        bankTurn = -physics->m_maxBankTurnSpeed <= turn ? turn : -physics->m_maxBankTurnSpeed;
    } else {
        bankTurn = physics->m_maxBankTurnSpeed;
    }

    float bank = bankTurn * (physics->m_maxBank / physics->m_maxBankTurnSpeed);

    if (fabsf(physics->m_rollTimeScale) < 0.00000023841858f) {
        *roll = 0.0f;
    } else {
        uint32_t period = static_cast<uint32_t>(6283.1855f / physics->m_rollTimeScale);
        *roll = (bank + sinf(static_cast<float>(time % period) * TWO_PI / static_cast<float>(period) + 0.5f) * physics->m_rollAmp) * damp;
    }

    if (fabsf(physics->m_pitchTimeScale) < 0.00000023841858f) {
        *pitch = 0.0f;
    } else {
        uint32_t period = static_cast<uint32_t>(6283.1855f / physics->m_pitchTimeScale);
        *pitch = damp * (sinf(TWO_PI * static_cast<float>(time % period) / static_cast<float>(period) + 0.2f) * physics->m_pitchAmp);
    }
}

// VERIFY: 0f888a8c auto
// OFFSET: 0x007F7DD0
void ShipPath::ComputeRollAndPitch(float speed, float facing, float distance, uint32_t leg, float turn, uint32_t time, C3Vector* position, float* roll, float* pitch) {
    if (speed > 0.0f) {
        float low = this->m_physics->m_speedDampThresh * 0.15000001f;
        float high = this->m_physics->m_speedDampThresh * 0.30000001f;

        if (speed < low) {
            speed = 0.0f;
        } else if (speed < high) {
            float scale = (speed - low) / (high - low);
            speed = speed * (scale * scale);
        }

        C3Vector vel(0.0f, 0.0f, 0.0f);

        for (int32_t i = 1; i < 5; i++) {
            float ahead = distance - speed * static_cast<float>(i) * 0.44999999f;

            if (ahead < 0.0f) {
                ahead = 0.0f;
            }

            ShipPathLeg* pathLeg = &this->m_legs.m_data[leg];
            float u = ahead / pathLeg->m_spline.cachedLength;

            if (u < 0.0f) {
                u = 0.0f;
            } else if (u >= 1.0f) {
                u = 1.0f;
            }

            pathLeg->m_spline.Vel(u, vel, C3Spline::EVAL_ARCLENGTH);
            float lengthSq = vel.z * vel.z + vel.y * vel.y + vel.x * vel.x;

            if (lengthSq > 0.00000023841858f) {
                float inv = 1.0f / sqrtf(lengthSq);
                vel.x *= inv;
                vel.y *= inv;
                vel.z *= inv;
            }

            float heading = atan2f(-vel.y, -vel.x);
            float delta = facing - heading;

            if (delta > PI) {
                delta -= TWO_PI;
            } else if (delta < -PI) {
                delta += TWO_PI;
            }

            turn = delta * 2.2222223f + turn;
            facing = heading;
        }

        turn *= 0.2f;
    }

    this->GetRollAndPitch(time, speed, turn, position, roll, pitch);
}

// VERIFY: bebfa321 manual
// OFFSET: 0x007F82B0
void ShipPath::GetPosition(uint32_t time, uint32_t elapsed, int32_t* mapId, uint32_t* sequence, C3Vector* position, float* facing, uint32_t* leg, float* roll, float* pitch, int32_t absolute) {
    C44Matrix frame;
    uint32_t pathTime = this->UpdatePathTime(time, elapsed, absolute);
    *sequence = ANIM_STAND;
    float speed = 0.0f;
    position->x = 0.0f;
    position->y = 0.0f;
    position->z = 0.0f;
    *facing = 0.0f;

    if (roll && pitch) {
        *pitch = 0.0f;
        *roll = 0.0f;
    }

    *leg = 0;

    if (!this->m_legs.m_count) {
        return;
    }

    while (pathTime >= this->m_legs.m_data[*leg].m_endTime) {
        if (++*leg >= this->m_legs.m_count) {
            return;
        }
    }

    float prevDistance = 0.0f;
    ShipPathLeg* pathLeg = &this->m_legs.m_data[*leg];
    uint32_t startTime = pathLeg->m_startTime;
    uint32_t seg = 0;
    float distance;

    if (pathLeg->m_segments.m_count != 1) {
        ShipPathSegment* segments = pathLeg->m_segments.m_data;

        while (pathTime >= segments[seg].m_time) {
            if (pathTime - segments[seg].m_time < segments[seg].m_delay) {
                distance = segments[seg].m_distance;
                *sequence = ANIM_STAND;
                goto frame;
            }

            prevDistance = segments[seg].m_distance;
            startTime = segments[seg].m_time + segments[seg].m_delay;

            if (++seg >= this->m_legs.m_data[*leg].m_segments.m_count - 1) {
                break;
            }
        }
    }

    {
        float t = static_cast<float>(pathTime - startTime) * 0.001f;
        float segTime = 0.001f * static_cast<float>(pathLeg->m_segments.m_data[seg].m_time - startTime);
        bool last = seg == pathLeg->m_segments.m_count - 1;
        float moved;

        if (seg == 0) {
            if (!last) {
                moved = this->GetDecelDistance(t, segTime, sequence, &speed);
            } else {
                *sequence = ANIM_SHIP_MOVING;
                speed = this->m_moveSpeed;
                moved = t * this->m_moveSpeed;
            }
        } else if (!last) {
            moved = this->GetAccelDecelDistance(t, segTime, sequence, &speed);
        } else {
            float accelTime = this->m_moveSpeed / this->m_accelRate;
            float rampTime = accelTime <= segTime ? accelTime : segTime;

            if (rampTime >= t) {
                *sequence = ANIM_SHIP_START;
                speed = this->m_accelRate * t;
                moved = t * (speed * 0.5f);
            } else {
                *sequence = ANIM_SHIP_MOVING;
                moved = accelTime * (this->m_moveSpeed * 0.5f) + (t - accelTime) * this->m_moveSpeed;
                speed = this->m_moveSpeed;
            }
        }

        distance = moved + prevDistance;
    }

frame:
    pathLeg = &this->m_legs.m_data[*leg];
    float u = distance / pathLeg->m_spline.cachedLength;

    if (u < 0.0f) {
        u = 0.0f;
    } else if (u >= 1.0f) {
        u = 1.0f;
    }

    pathLeg->m_spline.Frame(u, frame, C3Spline::EVAL_ARCLENGTH);
    float x = -frame.a0;
    float y = -frame.a1;
    float lengthSq = y * y + x * x;

    if (lengthSq > 0.00000023841858f) {
        float inv = 1.0f / sqrtf(lengthSq);
        x *= inv;
        y *= inv;
    }

    *facing = atan2f(y, x);
    position->x = frame.d0;
    position->y = frame.d1;
    position->z = frame.d2;

    if (this->m_physics && roll && pitch) {
        this->ComputeRollAndPitch(speed, *facing, distance, *leg, 0.0f, pathTime, position, roll, pitch);
    }

    *mapId = this->m_legs.m_data[*leg].m_mapId;
}

// VERIFY: 574c5d75 auto
// OFFSET: 0x007F78C0
void ShipPath::SetTimeOffsetToPoint(uint32_t time, uint32_t point) {
    uint32_t total = this->m_totalPathTime;
    uint32_t current = time % total;

    if (point <= current) {
        this->m_timeOffset = point + total - current;
    } else {
        this->m_timeOffset = point - current;
    }
}

// VERIFY: ef046ea6 manual
// OFFSET: 0x007F7D30
void ShipPath::FindLegAndSegment(uint32_t pathTime, uint32_t* leg, uint32_t* segment) {
    for (uint32_t i = 0; i < this->m_legs.m_count; i++) {
        ShipPathLeg& current = this->m_legs.m_data[i];

        if (pathTime >= current.m_endTime) {
            continue;
        }

        for (uint32_t j = 0; j < current.m_segments.m_count - 1; j++) {
            ShipPathSegment& entry = current.m_segments.m_data[j];

            if (pathTime < entry.m_time + entry.m_delay) {
                *leg = i;
                *segment = j;
                return;
            }
        }
    }

    *leg = 0;
    *segment = 0;
}

// VERIFY: f3f0526d manual
// OFFSET: 0x007F8000
void ShipPath::WaitAtNextPausePoint(uint32_t time, bool pause) {
    uint32_t total = this->m_totalPathTime;

    if (!total || this->m_waiting == pause) {
        return;
    }

    uint8_t reached = this->m_waitReached;
    uint32_t pathTime = reached ? this->m_waitTime + total - 1 : time + this->m_timeOffset;
    uint32_t leg;
    uint32_t segment;
    this->FindLegAndSegment(pathTime % total, &leg, &segment);
    ShipPathSegment& entry = this->m_legs.m_data[leg].m_segments.m_data[segment];

    if (pause) {
        this->m_waitTime = (entry.m_time + entry.m_delay) % total;
    } else if (reached) {
        this->SetTimeOffsetToPoint(time, entry.m_time + entry.m_delay);
    }

    this->m_waiting = pause;
    this->m_waitReached = 0;
    this->unused_003E = 0;
}

// VERIFY: defabc1a manual
// OFFSET: 0x007F80A0
void ShipPath::WaitAtCurrentLocation(uint32_t time) {
    uint32_t total = this->m_totalPathTime;

    if (!total || this->m_waitReached) {
        return;
    }

    uint32_t leg;
    uint32_t segment;
    this->FindLegAndSegment((total + this->m_timeOffset + time - 1) % total, &leg, &segment);
    ShipPathSegment& entry = this->m_legs.m_data[leg].m_segments.m_data[segment];
    this->m_waiting = 1;
    this->m_waitReached = 1;
    this->unused_003E = 0;
    this->m_waitTime = (entry.m_time + entry.m_delay) % total;
}

// VERIFY: 18f10a1b manual
// OFFSET: 0x007F8120
void ShipPath::SetTimeOffset(uint32_t time, float progress) {
    if (!this->m_totalPathTime) {
        return;
    }

    uint32_t point = CMath::fint_(static_cast<float>(this->m_totalPathTime) * progress);
    uint32_t total = this->m_totalPathTime;
    uint32_t current = time % total;

    if (point <= current) {
        this->m_timeOffset = point + total - current;
    } else {
        this->m_timeOffset = point - current;
    }
}
