#include "MovementShared.h"
#include <cmath>
#include <cstdio>
#include <malloc.h>
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C3Spline.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/CMovementGlobals.h"
#include "WoW/Source/Object/CMovementStatus.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/Types.h"

// STATIC: 0x00B2D9E8
float CMovementShared::s_terminalVelocity = 60.148003f;

// STATIC: 0x00B2D9EC
float CMovementShared::s_safeFallTerminalVelocity = 7.0f;

// STATIC: 0x00B2D9F0
float CMovementShared::s_twoPi = 6.2831855f;

// VERIFY: d453c161 auto
// OFFSET: 0x00986F00
float __stdcall CMovementShared::CalcFallDistance(float time, int32_t safeFall, float velocity) {
    float terminal = safeFall ? s_safeFallTerminalVelocity : s_terminalVelocity;

    if (velocity > terminal) {
        velocity = terminal;
    }

    if (19.291105f * time + velocity > terminal) {
        float accelTime = (terminal - velocity) * 0.051837362f;
        return terminal * (time - accelTime) + (velocity + 9.6455526f * accelTime) * accelTime;
    }

    return time * (velocity + time * 9.6455526f);
}

// VERIFY: dde025a2 auto
// OFFSET: 0x00987680
void CMovementShared::StopAllLogging() {
    CMovementGlobals* globals = MovementGetGlobals();

    if (globals && globals->m_movementLog) {
        fclose(globals->m_movementLog);
        globals->m_movementLog = nullptr;
    }
}

// VERIFY: e332dc35 auto
// OFFSET: 0x00988220
float CMovementShared::TimeToFallDistance(float distance, int32_t safeFall) {
    float terminal = safeFall ? s_safeFallTerminalVelocity : s_terminalVelocity;
    float accelTime = 0.051837362f * terminal;
    float accelDistance = accelTime * terminal * 0.5f;

    if (distance < accelDistance) {
        if (0.0f < distance) {
            return sqrt(distance * 0.103674725f);
        }

        return 0.0f;
    }

    return (distance - accelDistance) / terminal + accelTime;
}

// VERIFY: 9850a93f auto
// OFFSET: 0x0098C580
CMovementShared::CMovementShared(const WOWGUID& transportGuid, const C3Vector& position, float facing, const WOWGUID& ownerGuid) : CPassenger(transportGuid, position, ownerGuid), m_groundNormal(0.0f, 0.0f, 1.0f), m_anchorPos(position) {
    this->m_flags = 0;
    this->m_flags2 = 0;
    this->m_transportSeat = 0xFF;
    this->m_anchorFacing = facing;
    this->m_anchorPitch = 0.0f;
    this->m_fallTimeMs = 0;
    this->m_pitchSin = 0.0f;
    this->m_pitchCos = 1.0f;
    this->m_passengerFlags |= 1;
    this->m_fallStartZ = position.z;
    this->m_spline = nullptr;
    this->m_currentSpeed = 0.0f;
    this->m_walkSpeed = 0.0f;
    this->m_runSpeed = 0.0f;
    this->m_runBackSpeed = 0.0f;
    this->m_swimSpeed = 0.0f;
    this->m_swimBackSpeed = 0.0f;
    this->m_flightSpeed = 0.0f;
    this->m_flightBackSpeed = 0.0f;
    this->m_turnRate = 0.0f;
    this->m_pitchRate = 0.0f;
    this->m_fallVelocity = 0.0f;
    this->m_facing = facing;
    this->m_statusTimeMs = OsGetAsyncTimeMs();
    this->m_hoverHeight = 1.0f;
}

// VERIFY: 6ed825ce auto
// OFFSET: 0x009880C0
void CMovementShared::CalcCurrentSpeed(int32_t force) {
    if (!(this->m_flags & MOVEMENTFLAG_FALLING) || force) {
        this->m_currentSpeed = this->GetBaseSpeed(force);
    }
}

// VERIFY: 252ae672 manual
// OFFSET: 0x00987E30
void CMovementShared::CalcDirection() {
    float facing = this->m_anchorFacing;
    C2Vector dir2D;
    dir2D.x = cos(facing);
    dir2D.y = sin(facing);
    this->m_moveDir2D = dir2D;

    if ((this->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) && fabs(this->m_anchorPitch) >= 0.00000095367432f) {
        float pitch = this->m_anchorPitch;
        this->m_pitchCos = cos(pitch);
        this->m_pitchSin = sin(pitch);
        this->m_moveDir.x = this->m_pitchCos * dir2D.x;
        this->m_moveDir.y = this->m_pitchCos * dir2D.y;
        this->m_moveDir.z = this->m_pitchSin;
    } else {
        this->m_moveDir.x = this->m_moveDir2D.x;
        this->m_moveDir.y = this->m_moveDir2D.y;
        this->m_pitchSin = 0.0f;
        this->m_moveDir.z = 0.0f;
        this->m_pitchCos = 1.0f;
    }
}

// VERIFY: 440441da manual
// OFFSET: 0x0098B5D0
void CMovementShared::ComputeSplineDurationModifier(float progress) {
    CMoveSpline* spline = this->m_spline;

    if (!spline) {
        return;
    }

    if (!spline->m_duration || spline->m_duration * spline->m_durationMod < 0.01f) {
        spline->m_durationModNext = 1.0f;
        return;
    }

    float delta = progress - (int32_t)spline->m_start / (spline->m_duration * spline->m_durationMod);

    if (0.5f < delta) {
        delta = delta - 1.0f;
    }

    if (-0.5f > delta) {
        delta = delta + 1.0f;
    }

    spline = this->m_spline;
    int64_t offset = (int64_t)(delta * spline->m_duration);
    spline->m_durationModNext = (float)(spline->m_duration - (uint32_t)offset) / spline->m_duration;

    spline = this->m_spline;
    float modifier = spline->m_durationModNext;

    if (modifier < 0.5f) {
        spline->m_durationModNext = 0.5f;
    } else if (modifier >= 2.0f) {
        spline->m_durationModNext = 2.0f;
    } else {
        spline->m_durationModNext = modifier;
    }
}

// VERIFY: dfbd51e3 manual
// OFFSET: 0x00987EF0
void CMovementShared::CalcDirection(int32_t force) {
    if ((this->m_flags & MOVEMENTFLAG_FALLING) && !force) {
        return;
    }

    this->CalcDirection();
    uint32_t flags = this->m_flags;

    if (flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) {
        if (flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            float forward2DX = this->m_moveDir2D.x;
            float forward2DY = this->m_moveDir2D.y;
            float forwardX = this->m_moveDir.x;
            float forwardY = this->m_moveDir.y;
            float forwardZ = this->m_moveDir.z;

            if (flags & MOVEMENTFLAG_BACKWARD) {
                forward2DX = -forward2DX;
                forward2DY = -forward2DY;
                forwardX = -forwardX;
                forwardY = -forwardY;
                forwardZ = -forwardZ;
            }

            float swap = this->m_moveDir2D.x;
            this->m_moveDir2D.x = this->m_moveDir2D.y;
            this->m_moveDir2D.y = swap;

            if (this->m_flags & MOVEMENTFLAG_STRAFE_LEFT) {
                this->m_moveDir2D.x = -this->m_moveDir2D.x;
            } else {
                this->m_moveDir2D.y = -this->m_moveDir2D.y;
            }

            this->m_moveDir.x = this->m_moveDir2D.x + forwardX;
            this->m_moveDir.y = this->m_moveDir2D.y + forwardY;
            this->m_moveDir.z = forwardZ;
            this->m_moveDir2D.x = this->m_moveDir2D.x + forward2DX;
            this->m_moveDir2D.y = this->m_moveDir2D.y + forward2DY;
            this->m_moveDir.x = this->m_moveDir.x * 0.70710677f;
            this->m_moveDir.y = this->m_moveDir.y * 0.70710677f;
            this->m_moveDir.z = this->m_moveDir.z * 0.70710677f;
            this->m_moveDir2D.x = this->m_moveDir2D.x * 0.70710677f;
            this->m_moveDir2D.y = 0.70710677f * this->m_moveDir2D.y;
            return;
        }
    } else if (flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
        float swap = this->m_moveDir2D.x;
        this->m_moveDir2D.x = this->m_moveDir2D.y;
        this->m_moveDir2D.y = swap;

        if (this->m_flags & MOVEMENTFLAG_STRAFE_LEFT) {
            this->m_moveDir2D.x = -this->m_moveDir2D.x;
        } else {
            this->m_moveDir2D.y = -this->m_moveDir2D.y;
        }

        this->m_moveDir.x = this->m_moveDir2D.x;
        this->m_moveDir.y = this->m_moveDir2D.y;
        this->m_moveDir.z = 0.0f;
        return;
    }

    if (flags & MOVEMENTFLAG_BACKWARD) {
        this->m_moveDir = C3Vector(-this->m_moveDir.x, -this->m_moveDir.y, -this->m_moveDir.z);
        float x = -this->m_moveDir2D.x;
        float y = -this->m_moveDir2D.y;
        this->m_moveDir2D.x = x;
        this->m_moveDir2D.y = y;
    }
}

// VERIFY: 68c901ff manual
// OFFSET: 0x00986F70
float CMovementShared::CalcFallStartElevation(uint32_t time) const {
    return CalcFallDistance(time * 0.001f, this->m_flags & MOVEMENTFLAG_SAFE_FALL, this->m_fallVelocity) + this->m_position.z;
}

// VERIFY: bf39d893 manual
// OFFSET: 0x00987D20
float CMovementShared::CalcSplineHeight(uint32_t time, float z) const {
    if (!time) {
        return z;
    }

    CMoveSpline* spline = this->m_spline;

    if (time >= spline->m_duration) {
        return z;
    }

    if (spline->m_flags & SPLINEFLAG_PARABOLIC) {
        if (time <= spline->m_effectStartTime) {
            return z;
        }

        float start = spline->m_effectStartTime * 0.001f;
        float elapsed = time * 0.001f - start;
        return (0.001f * this->m_spline->m_duration - start) * this->m_spline->m_verticalAcceleration * 0.5f * elapsed - 0.5f * (elapsed * (this->m_spline->m_verticalAcceleration * elapsed)) + z;
    }

    if (!(spline->m_flags & SPLINEFLAG_FALLING)) {
        return z;
    }

    float height = this->m_fallStartZ - CalcFallDistance(time * 0.001f, 0, 0.0f);

    if (height <= spline->m_finalDestination.z) {
        return spline->m_finalDestination.z;
    }

    return height;
}

// VERIFY: 77349147 manual
// OFFSET: 0x00988280
float CMovementShared::CalcTimeFallen(float distance, int32_t upward) const {
    float terminal = (this->m_flags & MOVEMENTFLAG_SAFE_FALL) ? s_safeFallTerminalVelocity : s_terminalVelocity;
    float velocity = this->m_fallVelocity;

    if (velocity > terminal) {
        velocity = terminal;
    }

    if (fabs(velocity) < 0.00000023841858f) {
        return TimeToFallDistance(distance, this->m_flags & MOVEMENTFLAG_SAFE_FALL);
    }

    float discriminant = velocity * velocity + 38.582211f * distance;
    float root = 0.0f;

    if (discriminant > 0.0f) {
        root = sqrt(discriminant);
    }

    float timeUp = (-velocity - root) * 0.051837362f;
    float timeDown = (root - velocity) * 0.051837362f;
    float accelTime = 0.051837362f * (terminal - velocity);
    float result;

    if (timeDown <= accelTime) {
        result = timeDown;
    } else {
        result = (distance - (velocity + accelTime * 9.6455526f) * accelTime) / terminal + accelTime;
    }

    if (upward) {
        result = 0.0f;

        if (timeUp >= 0.0f) {
            return timeUp;
        }
    }

    return result;
}

// VERIFY: 02812862 manual
// OFFSET: 0x0075EE00
int32_t CMovementShared::CanCollideWhileFlying() const {
    if (this->IsSplineFlyerOrFlyingOrSwimming()) {
        return 1;
    }

    CMoveSpline* spline = this->m_spline;

    if ((this->m_flags2 & MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR) && spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_PARABOLIC)) {
        return 1;
    }

    spline = this->m_spline;

    if (!spline) {
        return 0;
    }

    if ((spline->m_flags & SPLINEFLAG_NO_SPLINE) || !(spline->m_flags & SPLINEFLAG_FALLING)) {
        return 0;
    }

    return 1;
}

// VERIFY: cad9b548 manual
// OFFSET: 0x0098AE90
int32_t CMovementShared::ChangeFlightBackSpeed(float speed) {
    if (fabs(speed - this->m_flightBackSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_flightBackSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 525d42d2 manual
// OFFSET: 0x0098AC40
int32_t CMovementShared::ChangeFlightSpeed(float speed) {
    if (fabs(speed - this->m_flightSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_flightSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: ce302667 manual
// OFFSET: 0x00989E80
int32_t CMovementShared::ChangePitchRate(float rate) {
    if (fabs(rate - this->m_pitchRate) < 0.00000023841858f) {
        return 0;
    }

    this->m_pitchRate = rate;
    this->UpdateAnchors(0);
    return 1;
}

// VERIFY: 03d5fe82 manual
// OFFSET: 0x0098A300
int32_t CMovementShared::ChangeRunBackSpeed(float speed) {
    if (fabs(speed - this->m_runBackSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_runBackSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: c93de573 manual
// OFFSET: 0x0098A0B0
int32_t CMovementShared::ChangeRunSpeed(float speed) {
    if (fabs(speed - this->m_runSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_runSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 517aee95 manual
// OFFSET: 0x0098A9F0
int32_t CMovementShared::ChangeSwimBackSpeed(float speed) {
    if (fabs(speed - this->m_swimBackSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_swimBackSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 7a624ff9 manual
// OFFSET: 0x0098A7A0
int32_t CMovementShared::ChangeSwimSpeed(float speed) {
    if (fabs(speed - this->m_swimSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_swimSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 8197cb37 manual
// OFFSET: 0x00989C50
int32_t CMovementShared::ChangeTurnRate(float rate) {
    if (fabs(rate - this->m_turnRate) < 0.00000023841858f) {
        return 0;
    }

    this->m_turnRate = rate;
    this->UpdateAnchors(0);
    return 1;
}

// VERIFY: c9422bef manual
// OFFSET: 0x0098A550
int32_t CMovementShared::ChangeWalkSpeed(float speed) {
    if (fabs(speed - this->m_walkSpeed) < 0.00000023841858f) {
        return 0;
    }

    this->m_walkSpeed = speed;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 62c08b57 auto
// OFFSET: 0x009872B0
void CMovementShared::ClearSplineEnabled() {
    this->m_flags &= ~MOVEMENTFLAG_SPLINE_ENABLED;
}

// VERIFY: 376f5cc2 manual
// OFFSET: 0x0098B310
int32_t CMovementShared::EnableGravity(int32_t enable) {
    uint32_t flags = this->m_flags;
    uint32_t newFlags = enable ? flags & ~MOVEMENTFLAG_DISABLE_GRAVITY : flags | MOVEMENTFLAG_DISABLE_GRAVITY;
    this->m_flags = newFlags;

    if (flags == newFlags) {
        return 0;
    }

    this->UpdateAnchors(0);
    return 1;
}

// VERIFY: 35440c64 auto
// OFFSET: 0x00987440
void CMovementShared::FeatherFall(int32_t enable) {
    if (enable) {
        this->m_flags |= MOVEMENTFLAG_SAFE_FALL;
    } else {
        this->m_flags &= ~MOVEMENTFLAG_SAFE_FALL;
    }
}

// VERIFY: f78ee919 manual
// OFFSET: 0x00988BA0
void CMovementShared::ForceStopStrafe() {
    this->m_flags &= ~(MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_PENDING_STRAFE_STOP);
    this->CalcCurrentSpeed(0);
    this->UpdateAnchors(0);
}

// VERIFY: 2cec3052 manual
// OFFSET: 0x00987570
float CMovementShared::GetBaseSpeed(int32_t walk) const {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING))) {
        return 0.0f;
    }

    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        if (!spline->m_duration) {
            return 0.0f;
        }

        return this->m_spline->m_spline.cachedLength / spline->m_duration * 1000.0f;
    }

    if (flags & MOVEMENTFLAG_FLYING) {
        if ((flags & MOVEMENTFLAG_BACKWARD) && this->m_flightSpeed >= this->m_flightBackSpeed) {
            return this->m_flightBackSpeed;
        }

        return this->m_flightSpeed;
    }

    if (flags & MOVEMENTFLAG_SWIMMING) {
        if ((flags & MOVEMENTFLAG_BACKWARD) && this->m_swimSpeed >= this->m_swimBackSpeed) {
            return this->m_swimBackSpeed;
        }

        return this->m_swimSpeed;
    }

    if ((flags & MOVEMENTFLAG_WALKING) || walk) {
        if (this->m_runSpeed > this->m_walkSpeed) {
            return this->m_walkSpeed;
        }
    } else if ((flags & MOVEMENTFLAG_BACKWARD) && this->m_runSpeed >= this->m_runBackSpeed) {
        return this->m_runBackSpeed;
    }

    return this->m_runSpeed;
}

// VERIFY: 5cf4c226 auto
// OFFSET: 0x00986FB0
float CMovementShared::GetDistanceFallen() const {
    CMoveSpline* spline = this->m_spline;

    if (spline && (spline->m_flags & (SPLINEFLAG_FALLING | SPLINEFLAG_PARABOLIC))) {
        float time = (spline->m_duration - spline->m_effectStartTime) * 0.001f;
        return CalcFallDistance(time, 0, -(spline->m_verticalAcceleration * time * 0.5f));
    }

    float distance = this->m_fallStartZ - this->m_position.z;

    if (this->m_fallVelocity < -0.00000023841858f) {
        distance = distance + this->m_fallVelocity * this->m_fallVelocity * 0.025918681f;
    }

    if (distance < 0.0f) {
        return 0.0f;
    }

    return distance;
}

// VERIFY: d47196c8 manual
// OFFSET: 0x00986E10
float CMovementShared::GetFeatherFallVelocity() const {
    float terminal = (this->m_flags & MOVEMENTFLAG_SAFE_FALL) ? s_safeFallTerminalVelocity : s_terminalVelocity;
    float velocity = this->m_fallVelocity;

    if (velocity > terminal) {
        velocity = terminal;
    }

    velocity = velocity + this->m_fallTimeMs * 0.001f * 19.291105f;

    if (velocity > terminal) {
        return terminal;
    }

    return velocity;
}

// VERIFY: 1929122f manual
// OFFSET: 0x00986DE0
float CMovementShared::GetModifiedSplineDuration() const {
    return this->m_spline->m_duration * this->m_spline->m_durationMod;
}

// VERIFY: f513badf manual
// OFFSET: 0x006E9870
NETMESSAGE CMovementShared::GetMoveEventMsgId(uint32_t oldFlags, int32_t ignoreJump) const {
    uint32_t flags = this->m_flags;
    uint32_t changed = flags ^ oldFlags;

    if (changed & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) {
        if (flags & MOVEMENTFLAG_FORWARD) {
            return MSG_MOVE_START_FORWARD;
        }

        return (flags & MOVEMENTFLAG_BACKWARD) ? MSG_MOVE_START_BACKWARD : MSG_MOVE_STOP;
    }

    if (changed & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
        if (flags & MOVEMENTFLAG_STRAFE_LEFT) {
            return MSG_MOVE_START_STRAFE_LEFT;
        }

        return (flags & MOVEMENTFLAG_STRAFE_RIGHT) ? MSG_MOVE_START_STRAFE_RIGHT : MSG_MOVE_STOP_STRAFE;
    }

    if ((flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_fallVelocity && !ignoreJump) {
        return MSG_MOVE_JUMP;
    }

    if (changed & MOVEMENTFLAG_SWIMMING) {
        return (flags & MOVEMENTFLAG_SWIMMING) ? MSG_MOVE_START_SWIM : MSG_MOVE_STOP_SWIM;
    }

    return MSG_MOVE_STOP;
}

// VERIFY: 7e64f279 auto
// OFFSET: 0x00987460
void CMovementShared::Hover(int32_t enable) {
    if (enable) {
        this->m_flags = this->m_flags & ~MOVEMENTFLAG_SPLINE_ELEVATION | MOVEMENTFLAG_HOVER;
    } else {
        this->m_flags &= ~MOVEMENTFLAG_HOVER;
    }
}

// VERIFY: 3866bcdb manual
// OFFSET: 0x00723350
int32_t CMovementShared::IsFalling() const {
    uint32_t flags = this->m_flags;

    if ((flags & MOVEMENTFLAG_FALLING) && ((flags & MOVEMENTFLAG_FALLING_FAR) || 0.0f != this->m_fallVelocity)) {
        return 1;
    }

    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FALLING)) {
        return 1;
    }

    if (!(this->m_flags2 & MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR)) {
        return 0;
    }

    if (!spline) {
        return 0;
    }

    if ((spline->m_flags & SPLINEFLAG_NO_SPLINE) || !(spline->m_flags & SPLINEFLAG_PARABOLIC)) {
        return 0;
    }

    return 1;
}

// VERIFY: a2dcabe3 auto
// OFFSET: 0x006EABA0
int32_t CMovementShared::IsFallingSwimmingFlying() const {
    if (this->IsGravityDisabled()) {
        return 1;
    }

    return (this->m_flags & (MOVEMENTFLAG_FALLING | MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) != 0;
}

// VERIFY: 3688d9cf auto
// OFFSET: 0x006E9AD0
int32_t CMovementShared::IsGravityDisabled() const {
    CMoveSpline* spline = this->m_spline;

    if (spline) {
        if (!(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FALLING)) {
            return 0;
        }

        if (!(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FLYING)) {
            return 1;
        }
    }

    return (this->m_flags2 & MOVEMENTFLAG2_DISABLE_GRAVITY) || (this->m_flags & MOVEMENTFLAG_DISABLE_GRAVITY);
}

// VERIFY: bc4424ec manual
// OFFSET: 0x00986EA0
int32_t CMovementShared::IsJumpingUp() const {
    return (this->m_flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_fallVelocity && this->m_fallVelocity * -0.051837362f > this->m_fallTimeMs * 0.001f;
}

// VERIFY: df155953 manual
// OFFSET: 0x006E9A70
int32_t CMovementShared::IsOnFallingSpline() const {
    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FALLING)) {
        return 1;
    }

    return 0;
}

// VERIFY: 5918282d manual
// OFFSET: 0x004F5260
int32_t CMovementShared::IsOnFlyingSpline() const {
    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_FLYING)) {
        return 1;
    }

    return 0;
}

// VERIFY: 45332d36 manual
// OFFSET: 0x006E9AA0
int32_t CMovementShared::IsOnParabolicSpline() const {
    CMoveSpline* spline = this->m_spline;

    if (spline && !(spline->m_flags & SPLINEFLAG_NO_SPLINE) && (spline->m_flags & SPLINEFLAG_PARABOLIC)) {
        return 1;
    }

    return 0;
}

// VERIFY: 80c031fa auto
// OFFSET: 0x004F5240
int32_t CMovementShared::IsOnSpline() const {
    return this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE);
}

// VERIFY: 1e461b71 auto
// OFFSET: 0x0075EDA0
int32_t CMovementShared::IsSplineFlyerOrFlyingOrSwimming() const {
    if (this->IsGravityDisabled()) {
        return 1;
    }

    return (this->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) != 0;
}

// VERIFY: bc1f2fdd manual
// OFFSET: 0x00716110
C3Vector CMovementShared::GetDestination() const {
    if (this->m_spline) {
        return this->m_spline->m_finalDestination;
    }

    return this->m_position;
}

// VERIFY: 1fb3fd04 auto
// OFFSET: 0x0071C660
int32_t CMovementShared::IsSplineFlyerOrSwimmingOrSafeFalling() const {
    if (this->IsGravityDisabled()) {
        return 1;
    }

    return (this->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_SAFE_FALL)) != 0;
}

// VERIFY: 034517b9 manual
// OFFSET: 0x0071C6C0
int32_t CMovementShared::IsSplineFlyerOrHovering() const {
    if (this->IsGravityDisabled()) {
        return 1;
    }

    return (this->m_flags & MOVEMENTFLAG_HOVER) != 0;
}

// VERIFY: 5facc0ca auto
// OFFSET: 0x0071C720
int32_t CMovementShared::IsSplineFlyerOrHoveringFlyingOrSwimming() const {
    if (this->IsGravityDisabled()) {
        return 1;
    }

    return (this->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_HOVER)) != 0;
}

// VERIFY: 925c466e manual
// OFFSET: 0x00987700
void CMovementShared::PlotAscendDescend(float dt, C3Vector* move) const {
    float vertical = 0.70710677f;
    float y = this->m_moveDir2D.y * 0.70710677f;

    if (!(this->m_flags & MOVEMENTFLAG_ASCENDING)) {
        vertical = -0.70710677f;
    }

    float speed = this->m_currentSpeed;
    *move = C3Vector(this->m_moveDir2D.x * 0.70710677f * dt * speed, y * dt * speed, speed * (vertical * dt));
}

// VERIFY: b4c5c178 manual
// OFFSET: 0x00987770
float CMovementShared::PlotFacing(float dt) const {
    uint32_t flags = this->m_flags;
    float rate;

    if (flags & MOVEMENTFLAG_TURN_LEFT) {
        rate = this->m_turnRate;
    } else if (flags & MOVEMENTFLAG_TURN_RIGHT) {
        rate = -this->m_turnRate;
    } else {
        rate = 0.0f;
    }

    if ((flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) && !(this->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_TURNING)) {
        rate = rate * 0.75f;
    }

    double facing = fmod((double)(rate * dt + this->m_anchorFacing), (double)s_twoPi);

    if (facing < 0.0) {
        facing = facing + s_twoPi;
    }

    return facing;
}

// VERIFY: 757fdadc manual
// OFFSET: 0x00987A00
void CMovementShared::PlotHorzCircularPosition(float dt, C3Vector* move) const {
    uint32_t flags = this->m_flags;
    float speed = this->m_currentSpeed;
    float rate;

    if (flags & MOVEMENTFLAG_TURN_LEFT) {
        rate = this->m_turnRate;
    } else if (flags & MOVEMENTFLAG_TURN_RIGHT) {
        rate = -this->m_turnRate;
    } else {
        rate = 0.0f;
    }

    if ((flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) && !(this->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_TURNING)) {
        rate = rate * 0.75f;
    }

    float radius = this->m_currentSpeed / rate;
    float angle = rate * dt;
    float c = cos(angle);
    float s = sin(angle);
    float forward = s * radius;
    float side = radius - c * radius;
    float x = this->m_moveDir2D.x * forward - this->m_moveDir2D.y * side;

    if (this->m_flags & MOVEMENTFLAG_ASCENDING) {
        move->x = x * 0.70710677f;
        move->y = (forward * this->m_moveDir2D.y + side * this->m_moveDir2D.x) * 0.70710677f;
        move->z = 0.70710677f * (speed * dt);
    } else if (this->m_flags & MOVEMENTFLAG_DESCENDING) {
        move->x = x * 0.70710677f;
        move->y = (forward * this->m_moveDir2D.y + side * this->m_moveDir2D.x) * 0.70710677f;
        move->z = speed * dt * -0.70710677f;
    } else if (this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) {
        move->x = x * this->m_pitchCos;
        move->y = (forward * this->m_moveDir2D.y + side * this->m_moveDir2D.x) * this->m_pitchCos;
        move->z = this->m_pitchSin * speed * dt;
    } else {
        move->x = x;
        move->y = forward * this->m_moveDir2D.y + side * this->m_moveDir2D.x;
        move->z = 0.0f;
    }
}

// VERIFY: 2a9c308f manual
// OFFSET: 0x009877D0
float CMovementShared::PlotPitch(float dt) const {
    uint32_t flags = this->m_flags;
    float rate;

    if (flags & MOVEMENTFLAG_PITCH_UP) {
        rate = this->m_pitchRate;
    } else if (flags & MOVEMENTFLAG_PITCH_DOWN) {
        rate = -this->m_pitchRate;
    } else {
        rate = 0.0f;
    }

    if ((flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) && !(this->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_PITCHING)) {
        rate = rate * 0.75f;
    }

    return fmod((double)(rate * dt + this->m_anchorPitch), (double)s_twoPi);
}

// VERIFY: 30021875 manual
// OFFSET: 0x00987820
void CMovementShared::PlotSpiralPosition(float dt, C3Vector* move) const {
    uint32_t flags = this->m_flags;
    float speed = this->m_currentSpeed;
    float turnRate;

    if (flags & MOVEMENTFLAG_TURN_LEFT) {
        turnRate = this->m_turnRate;
    } else if (flags & MOVEMENTFLAG_TURN_RIGHT) {
        turnRate = -this->m_turnRate;
    } else {
        turnRate = 0.0f;
    }

    if ((flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) && !(this->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_TURNING)) {
        turnRate = turnRate * 0.75f;
    }

    float pitchRate;

    if (flags & MOVEMENTFLAG_PITCH_UP) {
        pitchRate = this->m_pitchRate;
    } else if (flags & MOVEMENTFLAG_PITCH_DOWN) {
        pitchRate = -this->m_pitchRate;
    } else {
        pitchRate = 0.0f;
    }

    if ((flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) && !(this->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_PITCHING)) {
        pitchRate = 0.75f * pitchRate;
    }

    float turnRadius = speed * 0.70710677f / turnRate;
    float pitchRadius = speed * 0.70710677f / pitchRate;
    float turnAngle = turnRate * dt;
    float pitchAngle = dt * pitchRate;
    float turnCos = cos(turnAngle);
    float turnSin = sin(turnAngle);
    float pitchCos = cos(pitchAngle);
    float pitchSin = sin(pitchAngle);
    float forward = turnSin * turnRadius;
    float side = turnRadius - turnCos * turnRadius;
    move->x = this->m_moveDir2D.x * forward - this->m_moveDir2D.y * side;
    move->y = forward * this->m_moveDir2D.y + side * this->m_moveDir2D.x;
    move->z = (pitchRadius - pitchCos * pitchRadius) * this->m_pitchCos + pitchRadius * pitchSin * this->m_pitchSin;
}

// VERIFY: ff4b8c73 manual
// OFFSET: 0x009876B0
void CMovementShared::PlotStraight(float dt, C3Vector* move) const {
    float speed = this->m_currentSpeed;
    *move = C3Vector(this->m_moveDir.x * dt * speed, this->m_moveDir.y * dt * speed, dt * this->m_moveDir.z * speed);
}

// VERIFY: 1d4a74d1 manual
// OFFSET: 0x00987B50
uint32_t CMovementShared::PlotUnitMovement(uint32_t time, C3Vector* move, float* facing, float* pitch) const {
    if (!time) {
        return this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);
    }

    float dt = time * 0.001f;
    uint32_t mode = 0;

    if (this->m_flags & (MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT)) {
        if (facing) {
            *facing = this->PlotFacing(dt);
        }

        if (!(this->m_flags & MOVEMENTFLAG_FALLING)) {
            mode = 2;
        }
    }

    if (this->m_flags & (MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN)) {
        if (pitch) {
            *pitch = this->PlotPitch(dt);
        }

        if (!(this->m_flags & (MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING))) {
            mode |= 8;
        }
    }

    uint32_t flags = this->m_flags;

    if (flags & (MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) {
        mode |= 0x10;
    }

    if (flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) {
        mode |= 1;
    }

    if (flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
        mode |= 4;
    }

    switch (mode) {
        case 1:
        case 4:
        case 5:
        case 12: {
            this->PlotStraight(dt, move);
            break;
        }

        case 3:
        case 6:
        case 7:
        case 14:
        case 19:
        case 22:
        case 23: {
            this->PlotHorzCircularPosition(dt, move);
            break;
        }

        case 9:
        case 13: {
            this->PlotVertCircularPosition(dt, move);
            break;
        }

        case 11:
        case 15: {
            this->PlotSpiralPosition(dt, move);
            break;
        }

        case 16:
        case 18: {
            float z = dt * this->m_currentSpeed;

            if (!(flags & MOVEMENTFLAG_ASCENDING)) {
                z = -z;
            }

            move->x = 0.0f;
            move->y = 0.0f;
            move->z = z;
            break;
        }

        case 17:
        case 20:
        case 21: {
            this->PlotAscendDescend(dt, move);
            break;
        }
    }

    return this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);
}

// VERIFY: 3bed1026 manual
// OFFSET: 0x00987950
void CMovementShared::PlotVertCircularPosition(float dt, C3Vector* move) const {
    uint32_t flags = this->m_flags;
    float rate;

    if (flags & MOVEMENTFLAG_PITCH_UP) {
        rate = this->m_pitchRate;
    } else if (flags & MOVEMENTFLAG_PITCH_DOWN) {
        rate = -this->m_pitchRate;
    } else {
        rate = 0.0f;
    }

    if ((flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) && !(this->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_PITCHING)) {
        rate = rate * 0.75f;
    }

    float radius = this->m_currentSpeed / rate;
    float angle = rate * dt;
    float c = cos(angle);
    float s = sin(angle);
    float forward = s * radius;
    float up = radius - c * radius;
    move->x = (forward * this->m_pitchCos - up * this->m_pitchSin) * this->m_moveDir2D.x;
    move->y = (forward * this->m_pitchCos - up * this->m_pitchSin) * this->m_moveDir2D.y;
    move->z = up * this->m_pitchCos + forward * this->m_pitchSin;
}

// VERIFY: 335e6fe7 auto
// OFFSET: 0x00987410
void CMovementShared::PostFeatherFall() {
    if (this->m_flags & MOVEMENTFLAG_FALLING) {
        this->m_fallVelocity = this->GetFeatherFallVelocity();
        this->m_fallTimeMs = 0;
        this->m_fallStartZ = this->m_position.z;
    }
}

// VERIFY: f5fd7b56 manual
// OFFSET: 0x00987050
float CMovementShared::RelDistanceFallen(uint32_t time, float z) const {
    uint32_t flags = this->m_flags;
    float velocity = this->m_fallVelocity;

    if (flags & MOVEMENTFLAG_FALLING) {
        return CalcFallDistance(time * 0.001f, flags & MOVEMENTFLAG_SAFE_FALL, velocity) + z - this->m_fallStartZ;
    }

    return CalcFallDistance(time * 0.001f, flags & MOVEMENTFLAG_SAFE_FALL, velocity);
}

// VERIFY: 8e4bbbe3 auto
// OFFSET: 0x009873A0
void CMovementShared::ResetSpline(uint32_t splineId, uint32_t splineFlags) {
    if ((splineFlags & SPLINEFLAG_TRANSPORT_EXIT) && (this->m_flags & MOVEMENTFLAG_ROOT)) {
        this->m_flags = this->m_flags & ~MOVEMENTFLAG_ROOT | MOVEMENTFLAG_PENDING_ROOT;
    }

    if (this->m_spline) {
        this->m_spline->m_id = splineId;
    }

    this->m_spline->m_spline.SetPoints(nullptr, 0);
}

// VERIFY: 2de7f07c manual
// OFFSET: 0x0098B0E0
void CMovementShared::SetRunMode(int32_t run) {
    if (run) {
        this->m_flags &= ~MOVEMENTFLAG_WALKING;
    } else {
        this->m_flags |= MOVEMENTFLAG_WALKING;
    }

    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
}

// VERIFY: f03ebca4 auto
// OFFSET: 0x009872A0
void CMovementShared::SetSplineEnabled() {
    this->m_flags |= MOVEMENTFLAG_SPLINE_ENABLED;
}

// VERIFY: eb9a83e4 auto
// OFFSET: 0x0098B590
void CMovementShared::SetAnimTierInTrans(int32_t enable) {
    if (enable) {
        this->m_flags2 |= MOVEMENTFLAG2_ANIM_TIER_IN_TRANS;
    } else {
        this->m_flags2 &= ~MOVEMENTFLAG2_ANIM_TIER_IN_TRANS;
    }
}

// VERIFY: 9e882e99 auto
// OFFSET: 0x0098B570
void CMovementShared::SetJumpSplineInAir(int32_t enable) {
    if (enable) {
        this->m_flags2 |= MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR;
    } else {
        this->m_flags2 &= ~MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR;
    }
}

// VERIFY: 7691e079 auto
// OFFSET: 0x0098B5B0
void CMovementShared::SetVehicleExitVoluntary(int32_t enable) {
    if (enable) {
        this->m_flags2 |= MOVEMENTFLAG2_VEHICLE_EXIT_VOLUNTARY;
    } else {
        this->m_flags2 &= ~MOVEMENTFLAG2_VEHICLE_EXIT_VOLUNTARY;
    }
}

// VERIFY: b3738b01 manual
// OFFSET: 0x00988A20
int32_t CMovementShared::StartMove(int32_t forward, int32_t force) {
    this->m_flags &= ~(MOVEMENTFLAG_PENDING_STOP | MOVEMENTFLAG_PENDING_FORWARD | MOVEMENTFLAG_PENDING_BACKWARD);
    uint32_t flags = this->m_flags;
    int32_t falling = 0;

    if (!force && (flags & MOVEMENTFLAG_FALLING)) {
        if (flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            if (forward) {
                if (!(flags & MOVEMENTFLAG_FORWARD)) {
                    this->m_flags = flags | MOVEMENTFLAG_PENDING_FORWARD;
                    return 0;
                }
            } else if (!(flags & MOVEMENTFLAG_BACKWARD)) {
                this->m_flags = flags | MOVEMENTFLAG_PENDING_BACKWARD;
            }

            return 0;
        }

        falling = 1;
    }

    if (forward) {
        this->m_flags = flags & ~(MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD) | MOVEMENTFLAG_FORWARD;
    } else {
        this->m_flags = flags & ~(MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD) | MOVEMENTFLAG_BACKWARD;
    }

    this->UpdateAnchors(falling);
    this->CalcCurrentSpeed(falling);
    return 1;
}

// VERIFY: 8361f43c manual
// OFFSET: 0x00989220
void CMovementShared::StartPitch(int32_t up) {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) && !(this->m_flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)) {
        return;
    }

    this->m_flags = flags & ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN) | (up ? MOVEMENTFLAG_PITCH_UP : MOVEMENTFLAG_PITCH_DOWN);
    this->m_flags2 &= ~MOVEMENTFLAG2_INTERPOLATED_PITCHING;
    this->UpdateAnchors(0);
}

// VERIFY: 7f7081f3 manual
// OFFSET: 0x00988DF0
void CMovementShared::StartTurn(int32_t left) {
    if (left) {
        this->m_flags = this->m_flags & ~(MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT) | MOVEMENTFLAG_TURN_LEFT;
    } else {
        this->m_flags = this->m_flags & ~(MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT) | MOVEMENTFLAG_TURN_RIGHT;
    }

    this->m_flags2 &= ~MOVEMENTFLAG2_INTERPOLATED_TURNING;
    this->UpdateAnchors(0);
}

// VERIFY: a15f6480 manual
// OFFSET: 0x00989940
int32_t CMovementShared::StopAscensionDescension() {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING))) {
        return 0;
    }

    this->m_flags = flags & ~(MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);
    this->CalcCurrentSpeed(0);
    this->UpdateAnchors(0);
    return 1;
}

// VERIFY: 8df60ffb manual
// OFFSET: 0x00988490
void CMovementShared::StopFalling() {
    uint32_t flags = this->m_flags;
    uint32_t falling = flags & MOVEMENTFLAG_FALLING;

    if (falling) {
        this->m_flags = flags & ~(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
    }

    if (this->m_flags & MOVEMENTFLAG_PENDING_ROOT) {
        this->m_flags = this->m_flags & 0xFF203700 | MOVEMENTFLAG_ROOT;
    } else if (!falling) {
        return;
    }

    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
}

// VERIFY: 7f3f6747 manual
// OFFSET: 0x009886E0
void CMovementShared::StopFallingAlwaysAnchor() {
    if (this->m_flags & MOVEMENTFLAG_FALLING) {
        this->m_flags &= ~(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
    }

    if (this->m_flags & MOVEMENTFLAG_PENDING_ROOT) {
        this->m_flags = this->m_flags & 0xFF203700 | MOVEMENTFLAG_ROOT;
    }

    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
}

// VERIFY: 0fa8b83d manual
// OFFSET: 0x00989450
int32_t CMovementShared::StopPitch() {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN))) {
        return 0;
    }

    this->m_flags = flags & ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);
    this->UpdateAnchors(0);
    return 1;
}

// VERIFY: adc50747 manual
// OFFSET: 0x00989010
int32_t CMovementShared::StopTurn() {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT))) {
        return 0;
    }

    this->m_flags = flags & ~(MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT);
    this->UpdateAnchors(0);
    return 1;
}

// VERIFY: af214771 auto
// OFFSET: 0x00986E80
float CMovementShared::TimeToJumpPeak() const {
    if (this->m_flags & MOVEMENTFLAG_FALLING) {
        return this->m_fallVelocity * -0.051837362f;
    }

    return 0.0f;
}

// VERIFY: 1e261ca6 auto
// OFFSET: 0x009881D0
void CMovementShared::UpdateAnchors(int32_t force) {
    this->m_anchorFacing = this->m_facing;
    this->m_anchorPos = this->m_position;
    this->m_anchorPitch = this->m_pitch;
    this->m_anchorElapsedMs = 0;
    this->CalcDirection(force);
    MovementNotifyZoneMgr(*this->m_ownerGuid);
}

// VERIFY: 49695001 manual
// OFFSET: 0x00988920
void CMovementShared::UpdateBaseStatus(const CMovementStatus& status) {
    this->m_position = status.m_position;
    this->m_anchorPos = status.m_position;
    this->m_facing = status.m_facing;
    this->m_anchorFacing = status.m_facing;
    this->m_pitch = status.m_pitch;
    this->m_anchorPitch = status.m_pitch;
    this->m_anchorElapsedMs = 0;
    this->m_stepUpStartZ = status.m_splineElevation;
    this->CalcCurrentSpeed(0);
}

// VERIFY: 1171cfab auto
// OFFSET: 0x0075CF80
void CMovementShared::UpdateFallingFar() {
    uint32_t flags = this->m_flags;

    if (flags & (MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_SAFE_FALL)) {
        return;
    }

    if ((flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_fallVelocity) {
        if (this->m_fallStartZ - 0.11111111f >= this->m_position.z) {
            this->m_flags = flags | MOVEMENTFLAG_FALLING_FAR;
        }
    } else if (this->m_fallTimeMs >= 500) {
        this->m_flags = flags | MOVEMENTFLAG_FALLING_FAR;
    }
}

// VERIFY: 23497d62 manual
// OFFSET: 0x00988990
void CMovementShared::UpdateFallState(const CMovementStatus& status) {
    this->m_fallTimeMs = status.m_fallTime;
    this->m_fallVelocity = status.m_zSpeed;
    this->m_fallStartZ = CalcFallDistance(status.m_fallTime * 0.001f, this->m_flags & MOVEMENTFLAG_SAFE_FALL, status.m_zSpeed) + this->m_position.z;
    this->m_moveDir2D.x = status.m_sinAngle;
    this->m_moveDir2D.y = status.m_cosAngle;
    this->m_moveDir.x = status.m_sinAngle;
    this->m_moveDir.y = status.m_cosAngle;
    this->m_moveDir.z = 0.0f;
    this->m_currentSpeed = status.m_xySpeed;
}

// VERIFY: 5452bd48 manual
// OFFSET: 0x009872C0
uint64_t CMovementShared::UpdateTransportStatus(const CMovementStatus& status) {
    WOWGUID transport = *(const WOWGUID*)&status.m_transportGuid;

    if (transport) {
        if (!MovementObjIsTransport(transport)) {
            return 0;
        }

        this->m_position = status.m_transportPosition;
        this->m_anchorPos = status.m_transportPosition;
        this->m_facing = status.m_transportFacing;
        this->m_anchorFacing = status.m_transportFacing;
        this->m_fallStartZ = CalcFallDistance(this->m_fallTimeMs * 0.001f, this->m_flags & MOVEMENTFLAG_SAFE_FALL, this->m_fallVelocity) + this->m_position.z;
        this->m_stepUpStartZ = this->m_stepUpStartZ - (status.m_position.z - status.m_transportPosition.z);
        this->m_transportSeat = status.m_transportSeat;
    }

    return transport.m_guid;
}

// VERIFY: a88b400e manual
// OFFSET: 0x009873F0
void CMovementShared::WalkOnWater(int32_t enable) {
    if (enable) {
        this->m_flags |= MOVEMENTFLAG_WATERWALKING;
    } else {
        this->m_flags &= ~MOVEMENTFLAG_WATERWALKING;
    }
}

// VERIFY: ad77b25c auto
// OFFSET: 0x006E6EA0
C3Vector CMovementShared::GetPosition() const {
    return CPassenger::GetPosition(this->m_position);
}

// VERIFY: 85c9efaf auto
// OFFSET: 0x00987D00
uint32_t CMovementShared::PlotUnitMovement(uint32_t time, C3Vector* move) {
    return this->PlotUnitMovement(time, move, &this->m_facing, &this->m_pitch);
}

// VERIFY: 311937c1 auto
// OFFSET: 0x009870D0
float CMovementShared::RelDistanceFallen(uint32_t time) const {
    return this->RelDistanceFallen(time, this->m_position.z);
}

// VERIFY: fe244ccd auto
// OFFSET: 0x0098C480
void CMovementShared::SetFacing(float facing) {
    WOWGUID transport = *(const WOWGUID*)&this->m_transportGuid;
    float transportFacing = transport ? MovementGetTransportFacing(transport) : 0.0f;
    float rawFacing = CMath::normalizeangle0to2pi_(facing - transportFacing);

    if (!(fabs(rawFacing - this->m_facing) < 9.5367432e-7f)) {
        int32_t falling = this->m_flags & MOVEMENTFLAG_FALLING;
        this->m_facing = rawFacing;

        if (!falling) {
            this->UpdateAnchors(0);
        }
    }

    this->m_flags &= ~(MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT);
}

// VERIFY: 2924c0f9 manual
// OFFSET: 0x00989BC0
int32_t CMovementShared::SetPitch(float pitch) {
    if (fabs(pitch - this->m_pitch) < 9.5367432e-7f) {
        this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);
        return 0;
    }

    uint32_t flags = this->m_flags;
    this->m_pitch = pitch;

    if (flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) {
        this->UpdateAnchors(0);
        this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);
        return 0;
    }

    if (!(flags & MOVEMENTFLAG_WATERWALKING) || (flags & 0xC0100F) || (this->m_flags2 & MOVEMENTFLAG2_UNK_0200) || pitch > -0.6457718f) {
        this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);
        return 0;
    }

    this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);
    return 1;
}

// VERIFY: dabb60f1 auto
// OFFSET: 0x00989B70
void CMovementShared::SetRawFacing(float facing) {
    if (!(fabs(facing - this->m_facing) < 9.5367432e-7f)) {
        int32_t falling = this->m_flags & MOVEMENTFLAG_FALLING;
        this->m_facing = facing;

        if (!falling) {
            this->UpdateAnchors(0);
        }
    }

    this->m_flags &= ~(MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT);
}

// VERIFY: e1cfb82a manual
// OFFSET: 0x009898E0
int32_t CMovementShared::StartAscensionDescension(int32_t ascend) {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (this->m_flags2 & MOVEMENTFLAG2_NO_STRAFE)) {
        return 0;
    }

    this->m_flags = flags | (ascend ? MOVEMENTFLAG_ASCENDING : MOVEMENTFLAG_DESCENDING);
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 531a9f71 auto
// OFFSET: 0x00988370
int32_t CMovementShared::StartFalling(float velocity) {
    if (this->m_flags & (MOVEMENTFLAG_ONTRANSPORT | MOVEMENTFLAG_ROOT)) {
        return 0;
    }

    if (this->m_spline && (this->m_spline->m_flags & (SPLINEFLAG_FALLING | SPLINEFLAG_PARABOLIC))) {
        return 0;
    }

    this->UpdateAnchors(0);
    this->m_flags = (this->m_flags & ~(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SPLINE_ELEVATION)) | MOVEMENTFLAG_FALLING;

    if (!(this->m_flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)) {
        this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);
    }

    this->m_fallTimeMs = 0;
    this->m_fallStartZ = this->m_position.z;
    this->m_fallVelocity = velocity;
    return 1;
}

// VERIFY: bf0343ac manual
// OFFSET: 0x00989890
int32_t CMovementShared::StartFlight() {
    uint32_t flags = this->m_flags;

    if (!(flags & MOVEMENTFLAG_CAN_FLY) || (flags & MOVEMENTFLAG_PENDING_ROOT)) {
        return 0;
    }

    this->m_flags = (flags & ~(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SPLINE_ELEVATION)) | MOVEMENTFLAG_FLYING;
    this->UpdateAnchors(0);
    this->CalcCurrentSpeed(0);
    return 1;
}

// VERIFY: 3deb3e94 auto
// OFFSET: 0x00988DC0
int32_t CMovementShared::StopStrafe(uint32_t moveFlags) {
    if (!(moveFlags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) || (this->m_flags & MOVEMENTFLAG_FALLING)) {
        return 0;
    }

    this->ForceStopStrafe();
    return 1;
}

// VERIFY: 7d2f8dfe manual
// OFFSET: 0x0098C4F0
void CMovementShared::TryRoot() {
    uint32_t flags = this->m_flags;

    if (flags & MOVEMENTFLAG_ROOT) {
        return;
    }

    if ((flags & MOVEMENTFLAG_FALLING) && !this->m_spline) {
        this->m_flags = flags | MOVEMENTFLAG_PENDING_ROOT;
        return;
    }

    this->m_flags = flags | MOVEMENTFLAG_ROOT;
    this->StopFalling();
    this->m_flags &= 0xFF203F00;
    this->CalcCurrentSpeed(0);
}

// VERIFY: a740421a manual
// OFFSET: 0x00988B00
int32_t CMovementShared::StartStrafe(int32_t left) {
    this->m_flags &= ~(MOVEMENTFLAG_PENDING_STRAFE_STOP | MOVEMENTFLAG_PENDING_STRAFE_LEFT | MOVEMENTFLAG_PENDING_STRAFE_RIGHT);
    uint32_t flags = this->m_flags;

    if (this->m_flags2 & MOVEMENTFLAG2_NO_STRAFE) {
        return 0;
    }

    int32_t force = 0;

    if (flags & MOVEMENTFLAG_FALLING) {
        if (flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            if (left) {
                if (!(flags & MOVEMENTFLAG_STRAFE_LEFT)) {
                    this->m_flags = flags | MOVEMENTFLAG_PENDING_STRAFE_LEFT;
                    return 0;
                }
            } else if (!(flags & MOVEMENTFLAG_STRAFE_RIGHT)) {
                this->m_flags = flags | MOVEMENTFLAG_PENDING_STRAFE_RIGHT;
            }

            return 0;
        }

        force = 1;
    }

    this->m_flags = (flags & ~(MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) | (left ? MOVEMENTFLAG_STRAFE_LEFT : MOVEMENTFLAG_STRAFE_RIGHT);
    this->UpdateAnchors(force);
    this->CalcCurrentSpeed(force);
    return 1;
}

// VERIFY: ffa99d8e manual
// OFFSET: 0x009880F0
C3Vector CMovementShared::GetDirection() const {
    WOWGUID transport = *(const WOWGUID*)&this->m_transportGuid;

    if (!transport) {
        return this->m_moveDir;
    }

    C44Matrix matrix;
    MovementGetTransportMtxX(transport, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);

    return C3Vector(
        this->m_moveDir.x * matrix.a0 + matrix.c0 * this->m_moveDir.z + matrix.b0 * this->m_moveDir.y,
        this->m_moveDir.x * matrix.a1 + matrix.c1 * this->m_moveDir.z + matrix.b1 * this->m_moveDir.y,
        this->m_moveDir.x * matrix.a2 + matrix.c2 * this->m_moveDir.z + matrix.b2 * this->m_moveDir.y);
}

// VERIFY: 64bfd6f8 manual
// OFFSET: 0x00987490
C3Vector CMovementShared::GetGroundNormal() const {
    WOWGUID transport = *(const WOWGUID*)&this->m_transportGuid;

    if (!transport) {
        return this->m_groundNormal;
    }

    C44Matrix matrix;
    MovementGetTransportMtxX(transport, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);

    return C3Vector(
        this->m_groundNormal.x * matrix.a0 + matrix.c0 * this->m_groundNormal.z + matrix.b0 * this->m_groundNormal.y,
        this->m_groundNormal.x * matrix.a1 + matrix.c1 * this->m_groundNormal.z + matrix.b1 * this->m_groundNormal.y,
        this->m_groundNormal.x * matrix.a2 + matrix.c2 * this->m_groundNormal.z + matrix.b2 * this->m_groundNormal.y);
}

// VERIFY: f9c9cd25 auto
// OFFSET: 0x00987140
void CMovementShared::GetMoveStatus(NETMESSAGE msgId, uint32_t gameTime, CMovementStatus* status) const {
    status->m_gameTime = gameTime;
    status->m_moveExtraFlags = this->m_flags2;

    if (this->m_transportGuid || msgId == CMSG_MOVE_CHANGE_TRANSPORT) {
        status->m_transportGuid = this->m_transportGuid;
        status->m_transportTime = MovementGetGlobals()->m_transportTime;

        if (MovementGetGlobals()->m_hasTransportTime2) {
            uint32_t transportTime2 = MovementGetGlobals()->m_transportTime2;
            status->m_moveExtraFlags |= MOVEMENTFLAG2_INTERPOLATED_MOVEMENT;
            status->m_transportTime2 = transportTime2;
            MovementGetGlobals()->m_hasTransportTime2 = 0;
        } else {
            status->m_transportTime2 = 0;
        }

        status->m_moveFlags |= MOVEMENTFLAG_ONTRANSPORT;
        status->m_transportSeat = this->m_transportSeat;
    } else {
        status->m_moveFlags &= ~MOVEMENTFLAG_ONTRANSPORT;
        status->m_transportGuid = WOWGUID();
        status->m_transportTime = 0;
        status->m_transportTime2 = 0;
    }

    if (!this->m_spline || (this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        status->m_moveFlags &= ~MOVEMENTFLAG_SPLINE_ENABLED;
    } else {
        status->m_moveFlags |= MOVEMENTFLAG_SPLINE_ENABLED;
    }

    status->m_transportPosition = this->m_position;
    status->m_transportFacing = this->m_facing;
    status->m_position = CPassenger::GetPosition(this->m_position);
    status->m_facing = this->GetFacing(this->m_facing);
    status->m_pitch = this->m_pitch;
    status->m_fallTime = this->m_fallTimeMs;

    if (status->m_moveFlags & MOVEMENTFLAG_FALLING) {
        status->m_zSpeed = this->m_fallVelocity;
        status->m_sinAngle = this->m_moveDir2D.x;
        status->m_cosAngle = this->m_moveDir2D.y;
        status->m_xySpeed = this->m_currentSpeed;
    }

    if (this->m_flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
        status->m_splineElevation = this->m_stepUpStartZ;
    }
}

// VERIFY: 422385bb manual
// OFFSET: 0x00989660
void CMovementShared::StartSwim() {
    this->m_flags = (this->m_flags & ~(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SPLINE_ELEVATION)) | MOVEMENTFLAG_SWIMMING;
    this->UpdateAnchors(0);
    this->StopFalling();
    this->CalcCurrentSpeed(0);
}

// VERIFY: 60f68e18 manual
// OFFSET: 0x0098BD10
void CMovementShared::ForceStopMove(int32_t tryFall) {
    this->m_flags &= ~(MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_PENDING_STOP);
    this->CalcCurrentSpeed(0);
    this->UpdateAnchors(0);

    if (this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE)) {
        this->m_spline->m_flags |= SPLINEFLAG_NO_SPLINE;

        if (tryFall && this->m_spline && (this->m_spline->m_flags & SPLINEFLAG_FLYING) && !this->IsFallingSwimmingFlying()) {
            this->StartFalling(0.0f);
        }
    }
}

// VERIFY: a09b3274 manual
// OFFSET: 0x009883F0
int32_t CMovementShared::Jump(int32_t checkHover) {
    if (checkHover && (this->m_flags & MOVEMENTFLAG_HOVER) && !(this->m_flags & MOVEMENTFLAG_SWIMMING)) {
        return 0;
    }

    if (this->m_spline && !(this->m_spline->m_flags & SPLINEFLAG_NO_SPLINE) && (this->m_spline->m_flags & SPLINEFLAG_FLYING)) {
        return 0;
    }

    uint32_t flags = this->m_flags;

    if (flags & (MOVEMENTFLAG_ROOT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FLYING)) {
        return 0;
    }

    uint16_t flags2 = this->m_flags2;

    if (!(flags2 & MOVEMENTFLAG2_DISABLE_GRAVITY) && this->IsGravityDisabled()) {
        return 0;
    }

    if (flags2 & MOVEMENTFLAG2_NO_JUMPING) {
        return 0;
    }

    this->StartFalling((flags & MOVEMENTFLAG_SWIMMING) ? -9.0967484f : -7.9555473f);
    return 1;
}

// VERIFY: e9407a9e manual
// OFFSET: 0x0098C240
void CMovementShared::StopFlying() {
    this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING | MOVEMENTFLAG_FLYING);
    this->m_anchorPitch = 0.0f;
    this->m_pitch = 0.0f;
    this->TryStartFalling();
    this->CalcCurrentSpeed(0);
    this->UpdateAnchors(0);
}

// VERIFY: e4f266ce auto
// OFFSET: 0x0098BF80
int32_t CMovementShared::StopStrafe() {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT))) {
        if (flags & (MOVEMENTFLAG_PENDING_STRAFE_LEFT | MOVEMENTFLAG_PENDING_STRAFE_RIGHT)) {
            this->m_flags = flags & ~(MOVEMENTFLAG_PENDING_STRAFE_LEFT | MOVEMENTFLAG_PENDING_STRAFE_RIGHT);
        }

        return 0;
    }

    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
        this->m_flags = flags & ~MOVEMENTFLAG_SPLINE_ELEVATION;
        this->TryStartFalling();
    }

    if (this->m_flags & MOVEMENTFLAG_FALLING) {
        this->m_flags = (this->m_flags & ~(MOVEMENTFLAG_PENDING_STRAFE_STOP | MOVEMENTFLAG_PENDING_STRAFE_LEFT | MOVEMENTFLAG_PENDING_STRAFE_RIGHT)) | MOVEMENTFLAG_PENDING_STRAFE_STOP;
        return 0;
    }

    this->ForceStopStrafe();
    return 1;
}

// VERIFY: 0a61949c manual
// OFFSET: 0x0098BFF0
void CMovementShared::StopSwim() {
    if (this->m_flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING) {
        this->m_flags &= ~(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);
    } else {
        this->m_flags &= ~(MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);
        this->m_anchorPitch = 0.0f;
        this->m_pitch = 0.0f;
    }

    this->TryStartFalling();
    this->CalcCurrentSpeed(0);
    this->UpdateAnchors(0);
}

// VERIFY: 8cee2ec0 auto
// OFFSET: 0x0098B710
int32_t CMovementShared::TryStartFalling() {
    if (this->IsFallingSwimmingFlying()) {
        return 0;
    }

    return this->StartFalling(0.0f);
}

// VERIFY: d5bfa4f5 manual
// OFFSET: 0x0098C550
void CMovementShared::UnRoot(int32_t tryFall) {
    this->m_flags &= ~(MOVEMENTFLAG_ROOT | MOVEMENTFLAG_PENDING_ROOT);

    if (tryFall) {
        this->TryStartFalling();
    }
}

// VERIFY: c57f9a1e auto
// OFFSET: 0x0098C770
void CMovementShared::OnSpline(uint32_t timePassed, const C3Vector* points, uint32_t pointCount, uint32_t duration, uint32_t splineFlags, uint32_t splineId) {
    this->m_spline->m_flags = splineFlags;
    this->m_spline->m_timePassed = timePassed;
    this->m_spline->m_start = 0;
    this->m_spline->m_duration = duration;
    this->m_spline->m_spline.splineMode = (this->m_spline->m_flags & (SPLINEFLAG_CATMULLROM | SPLINEFLAG_FLYING)) != 0 ? MODE_CATMULLROM : MODE_LINEAR;
    this->m_spline->m_spline.SetPoints(points, pointCount);
    this->m_spline->m_id = splineId;
    this->m_spline->m_durationModNext = 1.0f;
    this->m_spline->m_durationMod = 1.0f;
    this->StartMove(!(this->m_spline->m_flags & SPLINEFLAG_BACKWARD), 1);

    uint32_t flags = this->m_flags;

    if ((flags & MOVEMENTFLAG_ROOT) && (this->m_spline->m_flags & (SPLINEFLAG_FALLING | SPLINEFLAG_PARABOLIC))) {
        this->m_flags = (flags & ~(MOVEMENTFLAG_ROOT | MOVEMENTFLAG_PENDING_ROOT)) | MOVEMENTFLAG_PENDING_ROOT;
    }

    if (this->m_spline->m_flags & SPLINEFLAG_FALLING) {
        if (this->m_flags & MOVEMENTFLAG_FLYING) {
            this->StopFlying();
        }

        this->StartFalling(0.0f);
    }

    this->m_spline->m_effectStartTime = 0;
    this->m_spline->m_verticalAcceleration = 0.0f;
}

// VERIFY: 79ac262b auto
// OFFSET: 0x0098C8D0
int32_t CMovementShared::StopMove() {
    uint32_t flags = this->m_flags;

    if (!(flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD))) {
        if (flags & (MOVEMENTFLAG_PENDING_FORWARD | MOVEMENTFLAG_PENDING_BACKWARD)) {
            this->m_flags = flags & ~(MOVEMENTFLAG_PENDING_FORWARD | MOVEMENTFLAG_PENDING_BACKWARD);
        }

        return 0;
    }

    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION) {
        this->m_flags = flags & ~MOVEMENTFLAG_SPLINE_ELEVATION;
        this->TryStartFalling();
    }

    if (this->m_flags & MOVEMENTFLAG_FALLING) {
        this->m_flags = (this->m_flags & ~(MOVEMENTFLAG_PENDING_STOP | MOVEMENTFLAG_PENDING_FORWARD | MOVEMENTFLAG_PENDING_BACKWARD)) | MOVEMENTFLAG_PENDING_STOP;
        return 0;
    }

    this->ForceStopMove(1);
    return 1;
}

// VERIFY: d8b7a48c auto
// OFFSET: 0x0098C8A0
int32_t CMovementShared::StopMove(uint32_t moveFlags) {
    if (!(moveFlags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) || (this->m_flags & MOVEMENTFLAG_FALLING)) {
        return 0;
    }

    this->ForceStopMove(1);
    return 1;
}

// VERIFY: 238f6d7a auto
// OFFSET: 0x00990370
int32_t AckMessageNeedsIndex(uint32_t msgId) {
    return msgId == CMSG_FORCE_MOVE_ROOT_ACK
        || msgId == CMSG_FORCE_MOVE_UNROOT_ACK
        || msgId == MSG_MOVE_TELEPORT_ACK
        || msgId == CMSG_FORCE_RUN_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_SWIM_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_WALK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_TURN_RATE_CHANGE_ACK
        || msgId == CMSG_FORCE_PITCH_RATE_CHANGE_ACK
        || msgId == CMSG_MOVE_HOVER_ACK
        || msgId == CMSG_MOVE_SET_CAN_FLY_ACK
        || msgId == CMSG_MOVE_FEATHER_FALL_ACK
        || msgId == CMSG_MOVE_WATER_WALK_ACK
        || msgId == CMSG_MOVE_KNOCK_BACK_ACK
        || msgId == CMSG_MOVE_GRAVITY_DISABLE_ACK
        || msgId == CMSG_MOVE_GRAVITY_ENABLE_ACK
        || msgId == CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK
        || msgId == CMSG_MOVE_SET_COLLISION_HGT_ACK;
}

// VERIFY: 16a6e259 auto
// OFFSET: 0x00990420
int32_t IsAckMessage(uint32_t msgId) {
    return msgId == CMSG_FORCE_MOVE_ROOT_ACK
        || msgId == CMSG_FORCE_MOVE_UNROOT_ACK
        || msgId == MSG_MOVE_TELEPORT_ACK
        || msgId == CMSG_FORCE_RUN_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_SWIM_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_WALK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_TURN_RATE_CHANGE_ACK
        || msgId == CMSG_FORCE_PITCH_RATE_CHANGE_ACK
        || msgId == CMSG_MOVE_HOVER_ACK
        || msgId == CMSG_MOVE_SET_CAN_FLY_ACK
        || msgId == CMSG_MOVE_FEATHER_FALL_ACK
        || msgId == CMSG_MOVE_WATER_WALK_ACK
        || msgId == CMSG_MOVE_KNOCK_BACK_ACK
        || msgId == CMSG_MOVE_NOT_ACTIVE_MOVER
        || msgId == CMSG_MOVE_GRAVITY_DISABLE_ACK
        || msgId == CMSG_MOVE_GRAVITY_ENABLE_ACK
        || msgId == CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK
        || msgId == CMSG_MOVE_SET_COLLISION_HGT_ACK;
}

// VERIFY: 46e794c2 manual
// OFFSET: 0x009904E0
int32_t IsMessageAllowedWhileOnSpline(uint32_t msgId) {
    switch (msgId) {
        case MSG_MOVE_START_SWIM:
        case MSG_MOVE_STOP_SWIM:
        case MSG_MOVE_TOGGLE_COLLISION_CHEAT:
        case CMSG_MOVE_START_SWIM_CHEAT:
        case CMSG_MOVE_STOP_SWIM_CHEAT:
        case CMSG_MOVE_SET_FLY:
        case CMSG_DISMISS_CONTROLLED_VEHICLE:
        case CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: c48ff64a auto
// OFFSET: 0x0098B730
void CMovementShared::RemoveSpline() {
    this->m_flags2 &= ~MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR;

    if (!this->m_spline) {
        return;
    }

    MovementDelSpline(this->m_spline);
    uint32_t falling = this->m_flags & MOVEMENTFLAG_FALLING;
    this->m_spline = nullptr;

    if (!falling) {
        this->m_currentSpeed = this->GetBaseSpeed(0);
    }
}

// VERIFY: 19d887a8 manual
// OFFSET: 0x0098B770
void CMovementShared::TransformSpline(const C44Matrix& matrix, float facing) {
    *reinterpret_cast<C3Vector*>(this->m_spline->m_face.m_point) *= matrix;
    this->m_spline->m_face.m_angle = CMath::normalizeangle0to2pi_(this->m_spline->m_face.m_angle + facing);

    TSStackArray<C3Vector> points(static_cast<C3Vector*>(alloca(sizeof(C3Vector) * this->m_spline->m_spline.m_pointCount)), this->m_spline->m_spline.m_pointCount, 1);

    for (uint32_t i = 0; i < points.m_count; i++) {
        points.m_data[i] = this->m_spline->m_spline.Point(i) * matrix;
    }

    this->m_spline->m_spline.SetPoints(points.m_data, points.m_count);
}

// VERIFY: 4a68cfb3 manual
// OFFSET: 0x0098B850
void CMovementShared::TransportTransform(const C44Matrix& matrix, float facing, const C3Vector& oldPosition) {
    this->m_fallStartZ = matrix.b2 * oldPosition.y + matrix.c2 * this->m_fallStartZ + matrix.a2 * oldPosition.x + matrix.d2;
    this->m_stepUpStartZ = matrix.b2 * oldPosition.y + matrix.a2 * oldPosition.x + this->m_stepUpStartZ * matrix.c2 + matrix.d2;
    this->m_anchorPos *= matrix;
    this->m_anchorFacing = CMath::normalizeangle0to2pi_(this->m_anchorFacing + facing);

    C3Vector direction;
    direction.x = matrix.c0 * this->m_moveDir.z + matrix.b0 * this->m_moveDir.y + matrix.a0 * this->m_moveDir.x;
    direction.y = matrix.a1 * this->m_moveDir.x + matrix.c1 * this->m_moveDir.z + matrix.b1 * this->m_moveDir.y;
    direction.z = matrix.b2 * this->m_moveDir.y + matrix.a2 * this->m_moveDir.x + this->m_moveDir.z * matrix.c2;
    this->m_moveDir = direction;

    this->m_moveDir2D = C2Vector(this->m_moveDir);
    this->m_moveDir2D.NormalizeSafe();

    if (this->IsOnSpline()) {
        this->TransformSpline(matrix, facing);
    }
}

// VERIFY: cf40ac09 auto
// OFFSET: 0x0098B9A0
float CMovementShared::RemoveFromTransportFinish(const C44Matrix& matrix, float facing, const C3Vector& oldPosition) {
    this->TransportTransform(matrix, facing, oldPosition);
    this->m_link.Unlink();
    MovementNotifyTransportUnitRemoved(*this->m_ownerGuid, this->m_transportGuid, this->m_transportSeat);
    this->m_flags &= ~MOVEMENTFLAG_SPLINE_ENABLED;
    return facing;
}

// VERIFY: 2195bbbc auto
// OFFSET: 0x0098BA20
float CMovementShared::AddToTransport(WOWGUID transport, C44Matrix* inverse, C44Matrix* transportMatrix) {
    C3Vector oldPosition = this->m_position;
    float facing = CPassenger::AddToTransport(transport, inverse, transportMatrix);
    this->TransportTransform(*inverse, facing, oldPosition);
    MovementAddToTransport(this, transport, 0);
    return facing;
}

// VERIFY: e9c7d6c2 manual
// OFFSET: 0x0098BA90
void CMovementShared::Teleport(const C3Vector& position, float facing, int32_t removeSpline) {
    this->m_position = position;
    this->m_facing = facing;
    this->m_pitch = 0.0f;
    this->StopFalling();
    this->m_anchorFacing = this->m_facing;
    this->m_flags &= 0xF9003F00;
    this->m_anchorPitch = this->m_pitch;
    this->m_anchorPos = this->m_position;
    this->m_anchorElapsedMs = 0;
    this->CalcDirection(0);
    MovementNotifyZoneMgr(*this->m_ownerGuid);

    if (!(this->m_flags & MOVEMENTFLAG_FALLING)) {
        this->m_currentSpeed = this->GetBaseSpeed(0);
    }

    if (this->IsOnSpline()) {
        if (removeSpline) {
            this->RemoveSpline();
        } else {
            this->m_spline->m_flags |= SPLINEFLAG_NO_SPLINE;
        }
    }
}

// VERIFY: cdc954ca auto
// OFFSET: 0x0098C730
float CMovementShared::RemoveFromTransport(C44Matrix* matrix) {
    C3Vector oldPosition = this->m_position;
    float facing = CPassenger::RemoveFromTransport(matrix);
    return this->RemoveFromTransportFinish(*matrix, facing, oldPosition);
}

// VERIFY: 78c2f0a4 manual
// OFFSET: 0x0098C940
void CMovementShared::ConvertCurrentSplineToLoopingSpline(uint32_t timePassed) {
    CMoveSpline* spline = this->m_spline;

    if (!spline) {
        return;
    }

    uint32_t count = spline->m_spline.m_pointCount;

    if (count < 4) {
        return;
    }

    C3Vector* points = static_cast<C3Vector*>(alloca(sizeof(C3Vector) * count));

    if (!points) {
        return;
    }

    this->m_spline = MovementNewSpline();

    if (!this->m_spline) {
        this->m_spline = spline;
        return;
    }

    spline->m_spline.GetPoints(points, count);
    points[1] = points[count - 3];
    this->OnSpline(timePassed, &points[1], count - 1, spline->m_duration, spline->m_flags, spline->m_id);
    MovementDelSpline(spline);
}

// VERIFY: 022de096 manual
// OFFSET: 0x0098CA00
uint32_t CMovementShared::PlotUnitSplineMovement(uint32_t time, C3Vector* position) {
    *position = this->m_position;

    if (!(this->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD))) {
        return this->m_flags & 0xC0100F;
    }

    if (this->m_spline->m_flags & SPLINEFLAG_FROZEN) {
        return 1;
    }

    this->m_spline->m_start = time - this->m_spline->m_timePassed;
    uint32_t duration = RoundToInt64(this->GetModifiedSplineDuration());
    CMoveSpline* spline = this->m_spline;
    float t;

    if (!duration) {
        spline->m_flags |= SPLINEFLAG_DONE;
        t = 1.0f;
    } else if (static_cast<int32_t>(spline->m_start) < 0) {
        t = 0.0f;
    } else if (spline->m_start < duration) {
        t = static_cast<float>(static_cast<int32_t>(spline->m_start)) / static_cast<float>(duration);
    } else if (!(spline->m_flags & SPLINEFLAG_CYCLIC)) {
        spline->m_flags |= SPLINEFLAG_DONE;
        t = 1.0f;
    } else {
        spline->m_start -= duration;
        this->m_spline->m_timePassed = time - this->m_spline->m_start;

        if (this->m_spline->m_flags & SPLINEFLAG_ENTER_CYCLE) {
            this->ConvertCurrentSplineToLoopingSpline(this->m_spline->m_timePassed);
            this->m_spline->m_flags &= ~SPLINEFLAG_ENTER_CYCLE;
        }

        this->m_spline->m_durationMod = this->m_spline->m_durationModNext;
        this->m_spline->m_durationModNext = 1.0f;
        duration = RoundToInt64(this->GetModifiedSplineDuration());

        if (duration) {
            t = static_cast<float>(static_cast<int32_t>(this->m_spline->m_start)) / static_cast<float>(duration);
        } else {
            t = 1.0f;
        }
    }

    C44Matrix frame(this->m_moveDir.x, this->m_moveDir.y, this->m_moveDir.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    this->m_spline->m_spline.Frame(t, frame, C3Spline::EVAL_ARCLENGTH);
    spline = this->m_spline;

    if (!(spline->m_flags & (SPLINEFLAG_ORIENTATION_FIXED | SPLINEFLAG_FALLING))) {
        if (frame.a0 * frame.a0 + frame.a1 * frame.a1 > 0.0018490001f) {
            this->m_facing = static_cast<float>(atan2(static_cast<double>(frame.a1), static_cast<double>(frame.a0)));
        }
    }

    if (spline->m_flags & SPLINEFLAG_BACKWARD) {
        this->m_facing = this->m_facing - 3.1415927f;
    }

    if (this->m_facing < 0.0f) {
        this->m_facing = s_twoPi + this->m_facing;
    }

    this->m_moveDir = *reinterpret_cast<C3Vector*>(&frame.a0);

    if (this->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) {
        this->m_pitch = static_cast<float>(asin(static_cast<double>(frame.a2)));
    } else if (spline->m_flags & SPLINEFLAG_FLYING) {
        C3Vector point;
        float progress = 1.0f;

        if (duration) {
            progress = static_cast<float>(static_cast<int32_t>(spline->m_start + 1000)) / static_cast<float>(duration);
        }

        if ((spline->m_flags & SPLINEFLAG_CYCLIC) && progress > 1.0f) {
            progress = progress - 1.0f;
        }

        this->m_spline->m_spline.Pos(fclamp(progress, 0.0f, 1.0f), point, C3Spline::EVAL_ARCLENGTH);

        C2Vector direction(this->m_moveDir);
        C3Vector world = this->GetPosition(this->m_position);
        C3Vector delta(point.x - world.x, point.y - world.y, point.z - world.z);
        C2Vector target(delta);
        direction.NormalizeSafe();
        target.NormalizeSafe();

        float cosAngle = direction.y * target.y + target.x * direction.x;
        ClampInPlace(cosAngle, -1.0f, 1.0f);

        double angle = target.y * direction.x - direction.y * target.x < 0.0f ? acos(static_cast<double>(cosAngle)) : -acos(static_cast<double>(cosAngle));
        C33Matrix rotation = C33Matrix::Rotation(fclamp(static_cast<float>(angle + angle), -1.5707964f, 1.5707964f), this->m_moveDir, false);
        this->m_groundNormal = *reinterpret_cast<C3Vector*>(&frame.c0) * rotation;
    }

    *position = *reinterpret_cast<C3Vector*>(&frame.d0);
    spline = this->m_spline;
    uint32_t flags = spline->m_flags;

    if (flags & SPLINEFLAG_ANIMATION) {
        if (!(this->m_flags2 & MOVEMENTFLAG2_ANIM_TIER_IN_TRANS) && spline->m_start > spline->m_effectStartTime) {
            this->m_flags2 |= MOVEMENTFLAG2_ANIM_TIER_IN_TRANS;
        }
    } else if (flags & SPLINEFLAG_PARABOLIC) {
        if (spline->m_start > spline->m_effectStartTime && !(this->m_flags2 & MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR)) {
            this->m_fallStartZ = this->m_position.z;
            this->m_flags2 |= MOVEMENTFLAG2_JUMP_SPLINE_IN_AIR;
        }

        position->z = this->CalcSplineHeight(this->m_spline->m_start, position->z);

        if (position->z > this->m_fallStartZ) {
            this->m_fallStartZ = this->m_position.z;
        }
    } else if (flags & SPLINEFLAG_FALLING) {
        float z = this->CalcSplineHeight(spline->m_start, position->z);
        position->z = z;

        if (fequal(z, this->m_spline->m_finalDestination.z)) {
            int32_t fallTime = CMath::fint_(TimeToFallDistance(this->m_fallStartZ - z, 0) * 1000.0f);

            if (static_cast<int32_t>(this->m_spline->m_start) > fallTime) {
                this->m_spline->m_start = fallTime;
            }

            this->m_spline->m_flags |= SPLINEFLAG_DONE;
        }
    }

    return 1;
}

// VERIFY: 4ba8163d manual
// OFFSET: 0x009870F0
void CMovementShared::AddSpline(const C3Vector& position) {
    if (!this->m_spline) {
        this->m_spline = MovementNewSpline();
    }

    this->m_spline->m_flags = 0;
    this->m_spline->m_finalDestination = position;
}

// VERIFY: 088e12ab manual
// OFFSET: 0x006E9A60
void CMovementShared::ResetFallOnControlLoss() {
    if (!(this->m_flags & 0x100000)) {
        return;
    }

    this->m_flags |= 0x800;
    this->StopFalling();
    this->m_flags &= 0xFF203F00;

    if (!(this->m_flags & 0x1000)) {
        this->m_currentSpeed = this->GetBaseSpeed(0);
    }
}

// VERIFY: 6c0feab8 manual
// OFFSET: 0x0098C670
void CMovementShared::Disable() {
    this->m_flags2 &= ~0x80;

    if (this->m_spline) {
        MovementDelSpline(this->m_spline);
        this->m_spline = nullptr;

        if (!(this->m_flags & 0x1000)) {
            this->m_currentSpeed = this->GetBaseSpeed(0);
        }
    }

    this->m_globalUnitsLink.Unlink();
    WOWGUID transport = this->m_link.m_next ? this->m_transportGuid : WOWGUID();
    this->m_link.Unlink();
    this->m_transportGuid = WOWGUID();

    if (transport != WOWGUID()) {
        MovementNotifyTransportUnitRemoved(*this->m_ownerGuid, transport, this->m_transportSeat);
    }

    this->m_flags &= 0x77FFFFFF;
}
