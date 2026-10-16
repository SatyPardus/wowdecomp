#ifndef WOW_SOURCE_OBJECT_MOVEMENTSHARED_H
#define WOW_SOURCE_OBJECT_MOVEMENTSHARED_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/Passenger.h"

class C44Matrix;
class CMoveSpline;
class CMovementStatus;

class CMovementShared : public CPassenger {
    public:
    void ResetFallOnControlLoss();
    void Disable();
    using CPassenger::GetPosition;

    static float s_terminalVelocity;
    static float s_safeFallTerminalVelocity;
    static float s_twoPi;

    static float __stdcall CalcFallDistance(float time, int32_t safeFall, float velocity);
    static void StopAllLogging();
    static float TimeToFallDistance(float distance, int32_t safeFall);

    /* 0030 */ TSLink<void> m_globalUnitsLink;
    /* 0038 */ C3Vector m_groundNormal;
    /* 0044 */ uint32_t m_flags;
    /* 0048 */ uint16_t m_flags2;
    /* 004A */ uint8_t m_transportSeat;
    /* 004B */ uint8_t unused_004B;
    /* 004C */ C3Vector m_anchorPos;
    /* 0058 */ float m_anchorFacing;
    /* 005C */ float m_anchorPitch;
    /* 0060 */ uint32_t m_anchorElapsedMs;
    /* 0064 */ C3Vector m_moveDir;
    /* 0070 */ C2Vector m_moveDir2D;
    /* 0078 */ float m_pitchCos;
    /* 007C */ float m_pitchSin;
    /* 0080 */ uint32_t m_fallTimeMs;
    /* 0084 */ float m_fallStartZ;
    /* 0088 */ float m_stepUpStartZ;
    /* 008C */ float m_currentSpeed;
    /* 0090 */ float m_walkSpeed;
    /* 0094 */ float m_runSpeed;
    /* 0098 */ float m_runBackSpeed;
    /* 009C */ float m_swimSpeed;
    /* 00A0 */ float m_swimBackSpeed;
    /* 00A4 */ float m_flightSpeed;
    /* 00A8 */ float m_flightBackSpeed;
    /* 00AC */ float m_turnRate;
    /* 00B0 */ float m_pitchRate;
    /* 00B4 */ float m_hoverHeight;
    /* 00B8 */ float m_fallVelocity;
    /* 00BC */ CMoveSpline* m_spline;
    /* 00C0 */ uint32_t m_statusTimeMs;

    CMovementShared(const WOWGUID& transportGuid, const C3Vector& position, float facing, const WOWGUID& ownerGuid);
    void CalcCurrentSpeed(int32_t force);
    void CalcDirection();
    void CalcDirection(int32_t force);
    float CalcFallStartElevation(uint32_t time) const;
    float CalcSplineHeight(uint32_t time, float z) const;
    float CalcTimeFallen(float distance, int32_t upward) const;
    int32_t CanCollideWhileFlying() const;
    int32_t ChangeFlightBackSpeed(float speed);
    int32_t ChangeFlightSpeed(float speed);
    int32_t ChangePitchRate(float rate);
    int32_t ChangeRunBackSpeed(float speed);
    int32_t ChangeRunSpeed(float speed);
    int32_t ChangeSwimBackSpeed(float speed);
    int32_t ChangeSwimSpeed(float speed);
    int32_t ChangeTurnRate(float rate);
    int32_t ChangeWalkSpeed(float speed);
    void AddSpline(const C3Vector& position);
    void ClearSplineEnabled();
    void RemoveSpline();
    void ComputeSplineDurationModifier(float progress);
    int32_t EnableGravity(int32_t enable);
    void FeatherFall(int32_t enable);
    void ForceStopMove(int32_t tryFall);
    void ForceStopStrafe();
    float GetBaseSpeed(int32_t walk) const;
    C3Vector GetDirection() const;
    float GetDistanceFallen() const;
    float GetFeatherFallVelocity() const;
    C3Vector GetGroundNormal() const;
    float GetModifiedSplineDuration() const;
    NETMESSAGE GetMoveEventMsgId(uint32_t oldFlags, int32_t ignoreJump) const;
    void GetMoveStatus(NETMESSAGE msgId, uint32_t gameTime, CMovementStatus* status) const;
    C3Vector GetPosition() const;
    void Hover(int32_t enable);
    int32_t IsFalling() const;
    int32_t IsFallingSwimmingFlying() const;
    int32_t IsGravityDisabled() const;
    int32_t IsJumpingUp() const;
    int32_t Jump(int32_t checkHover);
    int32_t IsOnFallingSpline() const;
    int32_t IsOnFlyingSpline() const;
    int32_t IsOnParabolicSpline() const;
    int32_t IsOnSpline() const;
    int32_t IsSplineFlyerOrFlyingOrSwimming() const;
    int32_t IsSplineFlyerOrHovering() const;
    int32_t IsSplineFlyerOrHoveringFlyingOrSwimming() const;
    int32_t IsSplineFlyerOrSwimmingOrSafeFalling() const;
    C3Vector GetDestination() const;
    void PlotAscendDescend(float dt, C3Vector* move) const;
    float PlotFacing(float dt) const;
    void PlotHorzCircularPosition(float dt, C3Vector* move) const;
    float PlotPitch(float dt) const;
    void PlotSpiralPosition(float dt, C3Vector* move) const;
    void PlotStraight(float dt, C3Vector* move) const;
    void OnSpline(uint32_t timePassed, const C3Vector* points, uint32_t pointCount, uint32_t duration, uint32_t splineFlags, uint32_t splineId);
    uint32_t PlotUnitMovement(uint32_t time, C3Vector* move);
    uint32_t PlotUnitMovement(uint32_t time, C3Vector* move, float* facing, float* pitch) const;
    void PlotVertCircularPosition(float dt, C3Vector* move) const;
    void PostFeatherFall();
    float RelDistanceFallen(uint32_t time) const;
    float RelDistanceFallen(uint32_t time, float z) const;
    void ResetSpline(uint32_t splineId, uint32_t splineFlags);
    void SetAnimTierInTrans(int32_t enable);
    void SetFacing(float facing);
    void SetJumpSplineInAir(int32_t enable);
    int32_t SetPitch(float pitch);
    void SetRawFacing(float facing);
    void SetRunMode(int32_t run);
    void SetSplineEnabled();
    void SetVehicleExitVoluntary(int32_t enable);
    int32_t StartAscensionDescension(int32_t ascend);
    int32_t StartFalling(float velocity);
    int32_t StartFlight();
    int32_t StartMove(int32_t forward, int32_t force);
    void StartPitch(int32_t up);
    int32_t StartStrafe(int32_t left);
    void StartSwim();
    void StartTurn(int32_t left);
    int32_t StopAscensionDescension();
    void StopFalling();
    void StopFallingAlwaysAnchor();
    void StopFlying();
    int32_t StopMove();
    int32_t StopMove(uint32_t moveFlags);
    int32_t StopPitch();
    int32_t StopStrafe();
    int32_t StopStrafe(uint32_t moveFlags);
    void StopSwim();
    int32_t StopTurn();
    float TimeToJumpPeak() const;
    void TryRoot();
    int32_t TryStartFalling();
    void UnRoot(int32_t tryFall);
    void UpdateAnchors(int32_t force);
    void UpdateBaseStatus(const CMovementStatus& status);
    void UpdateFallState(const CMovementStatus& status);
    void UpdateFallingFar();
    uint64_t UpdateTransportStatus(const CMovementStatus& status);
    void WalkOnWater(int32_t enable);
    void Teleport(const C3Vector& position, float facing, int32_t removeSpline);
    void TransformSpline(const C44Matrix& matrix, float facing);
    void TransportTransform(const C44Matrix& matrix, float facing, const C3Vector& oldPosition);
    float RemoveFromTransportFinish(const C44Matrix& matrix, float facing, const C3Vector& oldPosition);
    float AddToTransport(WOWGUID transport, C44Matrix* inverse, C44Matrix* transportMatrix);
    float RemoveFromTransport(C44Matrix* matrix);
    void ConvertCurrentSplineToLoopingSpline(uint32_t timePassed);
    uint32_t PlotUnitSplineMovement(uint32_t time, C3Vector* position);
};

int32_t AckMessageNeedsIndex(uint32_t msgId);
int32_t IsAckMessage(uint32_t msgId);
int32_t IsMessageAllowedWhileOnSpline(uint32_t msgId);

#endif
