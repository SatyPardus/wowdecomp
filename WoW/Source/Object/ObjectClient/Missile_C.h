#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_MISSILE_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_MISSILE_C_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

class C44Matrix;
class CGObject_C;
class CGUnit_C;
class CMapEntity;
class CMissileCollision;
class CM2Model;
struct SimpleScriptIO;

struct MissileCollisionData {
    /* 0000 */ WOWGUID m_casterGuid;
    /* 0008 */ uint8_t m_castID;
    /* 0009 */ uint8_t unused_0009[3];
    /* 000C */ C3Vector m_position;
};

struct ChaserState {
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_velocity;

    ChaserState() = default;
    ChaserState(const C3Vector& position, const C3Vector& velocity);
    void ChaserImplementation(const C3Vector& target, float stiffness, float elapsed);
};

class CMissile {
    public:
    static void UpdateStaticLists();
    static void RemoveMissiles();
    static CMissile* s_inFlightMissileList;
    static CMissile* s_missileDestroyList;
    static CMissile* NewMissile();
    static CMissile* CopyMissile(const CMissile* source);
    static void ClearInFlightMissilesWithTarget(WOWGUID target);
    static uint32_t* s_missileHeap;

    static void AdjustAllMissileTrajectoryDurations(uint8_t castID, uint32_t duration);
    static void Destroy();
    static void Initialize();
    static void OnRemoteMissileTrajectoryCollision(const MissileCollisionData& data);
    static void ScriptManagerCallback(uint32_t event);
    static void StartFizzling(WOWGUID caster, uint8_t castID, uint32_t spellID);
    static void UpdateAttachments(WOWGUID guid);

    /* 0000 */ WOWGUID m_casterGuid;
    /* 0008 */ WOWGUID m_targetGuid;
    /* 0010 */ WOWGUID m_transportGuid;
    /* 0018 */ CM2Model* m_model;
    /* 001C */ uint32_t m_spellID;
    /* 0020 */ uint8_t m_castID;
    /* 0021 */ uint8_t unused_0021[3];
    /* 0024 */ float m_speed;
    /* 0028 */ float m_baseSpeed;
    /* 002C */ C3Vector m_position;
    /* 0038 */ C3Vector m_launchPos;
    /* 0044 */ C3Vector m_impactPos;
    /* 0050 */ C3Vector m_worldImpactPos;
    /* 005C */ C3Vector m_lastModelPos;
    /* 0068 */ uint32_t m_flags;
    /* 006C */ int32_t m_attachmentID;
    /* 0070 */ C3Vector m_attachOffset;
    /* 007C */ int32_t m_soundKitID;
    /* 0080 */ SE2SoundKitObject m_missileSound;
    /* 0094 */ uint32_t m_hitResult;
    /* 0098 */ uint32_t m_missReason;
    /* 009C */ uint32_t m_destroyTime;
    /* 00A0 */ TSGrowableArray<uint64_t> m_targets;
    /* 00B0 */ TSGrowableArray<SpellCastTargetPoint> m_targetPoints;
    /* 00C0 */ TSGrowableArray<uint32_t> m_targetHitResults;
    /* 00D0 */ TSGrowableArray<uint32_t> m_targetMissReasons;
    /* 00E0 */ uint32_t m_victimAnimTime;
    /* 00E4 */ uint32_t m_castTime;
    /* 00E8 */ uint32_t m_duration;
    /* 00EC */ uint32_t m_lastUpdateTime;
    /* 00F0 */ uint32_t m_currentTarget;
    /* 00F4 */ float m_scale;
    /* 00F8 */ float m_progress;
    /* 00FC */ float m_horizontalProgress;
    /* 0100 */ float m_groundOffset;
    /* 0104 */ float m_followGroundRate;
    /* 0108 */ float m_followGroundStart;
    /* 010C */ uint32_t m_motionFlags;
    /* 0110 */ int32_t m_missileIndex;
    /* 0114 */ int32_t m_missileCount;
    /* 0118 */ float m_pitch;
    /* 011C */ float m_trajectorySpeed;
    /* 0120 */ C3Vector m_velocity;
    /* 012C */ float m_travelTime;
    /* 0130 */ float m_timeScale;
    /* 0134 */ uint32_t m_arrivalTime;
    /* 0138 */ uint32_t m_adjustTime;
    /* 013C */ uint32_t m_adjustedDuration;
    /* 0140 */ float m_gravity;
    /* 0144 */ uint32_t m_fizzleTime;
    /* 0148 */ CMissileCollision* m_missileCollision;
    /* 014C */ C3Vector m_remoteCollisionPos;
    /* 0158 */ CMapEntity* m_mapEntity;
    /* 015C */ CMapEntity* m_mapEntity2;
    /* 0160 */ CMissile** m_prevLink;
    /* 0164 */ CMissile* m_next;
    /* 0168 */ uint32_t m_memHandle;
    /* 016C */ uint32_t unused_016C;
    /* 0170 */ WOWGUID m_targetingUnit;
    /* 0178 */ uint32_t m_targetingEffectTime;
    /* 017C */ int32_t m_motionScriptFunction;
    /* 0180 */ uint32_t m_fireTime;
    /* 0184 */ float m_initialDistance;
    /* 0188 */ float m_random[3];
    /* 0194 */ C3Vector m_smoothDirection;
    /* 01A0 */ C3Vector m_lastTransportPos;
    /* 01AC */ ChaserState m_transportOffset;
    /* 01C4 */ uint32_t unused_01C4;

    void AdjustTrajectoryDuration(uint32_t duration);
    void ApplyMotionScriptFunctionRotationAndScale(const SimpleScriptIO& io, C44Matrix& matrix);
    int32_t ApplyMotionScriptFunctionTranslation(const SimpleScriptIO& io, C44Matrix& matrix);
    int32_t CallMotionScriptFunction(SimpleScriptIO& io);
    void DetermineMissType();
    uint32_t GetHitResult() const;
    uint32_t GetMissReason() const;
    int32_t IsHit() const;
    void SetHitResult(uint32_t result);
    void SetRemoteCollisionPos(const C3Vector& position);
    C3Vector TransformPositionByTransport();
    void UpdateProgress(float distance);
    CMissile();
    ~CMissile();
    C44Matrix GetPositionMatrix() const;
    void BeginWaitingForModelFX(int32_t disableEffects);
    int32_t WaitForModelFX();
    C3Vector* ResolveTargetPosition(C3Vector* position, WOWGUID* transport);
    void Link(CMissile** link);
    int32_t AttachToVictim(CGUnit_C* victim, uint32_t duration);
    void ClearTargetingEffects();
    void SetReadyToFire(CMissile** list);
    void UpdateImpactPos();
    void CalcMissFade();
    void ApplyTargetTransportOffset(CGObject_C* target, C3Vector* position, float elapsed);
    void FollowGround(const C3Vector& direction, float distance);
    void FireMissiles(const C3Vector* position);
    int32_t PossiblyHandleBounce();
    void HitTarget(CGUnit_C* target, const SpellRec* spell, const SpellVisualRec* visual);
    void HitTargetAreaEffect(const SpellRec* spell, const SpellVisualRec* visual);
    void MissedTarget(CGUnit_C* target, const C3Vector* direction);
    void ReachedTarget(CGUnit_C* target, const C3Vector* direction);
    void HandleMissileTrajectoryCollision(const C3Vector* direction);
    void ComputeModelMatrix(C44Matrix* matrix, const C3Vector* position, const C3Vector* direction);
    void InitializeTrajectory();
    void InstallMotionScriptFunction(int32_t motionId);
    void CalcPosition(int32_t advance);
    void DeleteSelf();
};

#endif
