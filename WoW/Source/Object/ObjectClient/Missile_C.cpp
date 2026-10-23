#include "Missile_C.h"
#include <cfloat>
#include <cmath>
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/ObjectClient/SimpleScript.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "WoW/Source/Object/ObjectClient/UnitMissileTrajectory_C.h"
#include "WoW/Source/WorldClient/MapEntity.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellMissileRec.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/Magic/Spell.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/DB/Rec/SpellMissileMotionRec.h"

static const float MISSILE_HEIGHT_SCAN_RANGE = 20.0f;

// STATIC: 0x00CA0B58
CMissile* CMissile::s_inFlightMissileList;

// STATIC: 0x00CA0B5C
CMissile* CMissile::s_missileDestroyList;

// STATIC: 0x00CA0AF8
static double s_msfNumbers[12];

// STATIC: 0x00ADA828
static SimpleScriptIO s_msfIO = { { 12, 0, s_msfNumbers, 0 }, 0, 0, nullptr };

// STATIC: 0x00ADA7C0
static const char* s_msfArgs[] = { "progress", "time", "missileIndex", "missileCount", "distanceToFirePos", "distanceToImpactPos", "startDistance", "totalDistance", "rand1", "rand2", "rand3", "spellID", nullptr };

// STATIC: 0x00ADA7F4
static const char* s_msfReturns[] = { "transAngle", "transMag", "transRight", "transFront", "transUp", "modelYaw", "modelPitch", "modelRoll", "speedAbs", "speedScalar", "speedOffset", "scale", nullptr };

// STATIC: 0x00CA0B60
uint32_t* CMissile::s_missileHeap;

// VERIFY: fd41554e auto
// OFFSET: 0x006FF210
void CMissile::AdjustAllMissileTrajectoryDurations(uint8_t castID, uint32_t duration) {
    for (CMissile* missile = s_inFlightMissileList; missile; missile = missile->m_next) {
        if (missile->m_castID == castID) {
            missile->AdjustTrajectoryDuration(duration);
        }
    }
}

// VERIFY: 05173280 manual
// OFFSET: 0x00703BE0
void CMissile::Destroy() {
    CSimpleScriptManager::UnregisterCallback(CMissile::ScriptManagerCallback);
    delete s_missileHeap;
    s_missileHeap = nullptr;
}

// VERIFY: 00facf3f manual
// OFFSET: 0x00703B80
void CMissile::Initialize() {
    s_missileHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMissile), 64, "CMissile", true));
    CSimpleScriptManager::RegisterCallback(CMissile::ScriptManagerCallback);
}

// VERIFY: ffb5de8d auto
// OFFSET: 0x006FF2C0
void CMissile::OnRemoteMissileTrajectoryCollision(const MissileCollisionData& data) {
    for (CMissile* missile = s_inFlightMissileList; missile; missile = missile->m_next) {
        if (missile->m_casterGuid == data.m_casterGuid && missile->m_castID == data.m_castID) {
            missile->m_flags |= 0x400000;
            missile->m_remoteCollisionPos = data.m_position;
        }
    }
}

// VERIFY: eb1164f7 auto
// OFFSET: 0x006FF250
void CMissile::StartFizzling(WOWGUID caster, uint8_t castID, uint32_t spellID) {
    for (CMissile* missile = s_inFlightMissileList; missile; missile = missile->m_next) {
        if (missile->m_casterGuid == caster && missile->m_castID == castID && missile->m_spellID == spellID && !missile->m_fizzleTime) {
            missile->m_fizzleTime = OsGetAsyncTimeMs() + 500;

            if (!missile->m_fizzleTime) {
                missile->m_fizzleTime = 1;
            }
        }
    }
}

// VERIFY: a6a16617 manual
// OFFSET: 0x006FF130
void CMissile::AdjustTrajectoryDuration(uint32_t duration) {
    if (!(this->m_flags & 0x10000)) {
        return;
    }

    this->m_flags |= 0x40000;
    uint32_t now = OsGetAsyncTimeMs();
    float remaining = (float)(this->m_arrivalTime - now);
    this->m_adjustTime = now;

    if (!(0.0f < remaining)) {
        this->m_adjustedDuration = 1;
        return;
    }

    if (!duration) {
        duration = 1;
    }

    float ratio = duration / remaining;

    if (ratio < 0.5f) {
        ratio = 0.5f;
    } else if (ratio >= 2.0f) {
        ratio = 2.0f;
    }

    this->m_adjustedDuration = (uint32_t)(int64_t)(remaining * ratio);

    if (!this->m_adjustedDuration) {
        this->m_adjustedDuration = 1;
    }
}

// VERIFY: 4674cb82 manual
// OFFSET: 0x006FEB20
void CMissile::ApplyMotionScriptFunctionRotationAndScale(const SimpleScriptIO& io, C44Matrix& matrix) {
    if (0.0 != io.m_numbers.m_data[5]) {
        matrix.RotateAroundZ(io.m_numbers.m_data[5] * 0.017453292f);
    }

    if (0.0 != io.m_numbers.m_data[6]) {
        matrix.RotateAroundY(io.m_numbers.m_data[6] * 0.017453292f);
    }

    if (0.0 != io.m_numbers.m_data[7]) {
        matrix.RotateAroundX(io.m_numbers.m_data[7] * 0.017453292f);
    }

    const double* numbers = io.m_numbers.m_data;

    if (numbers[11] > 0.0) {
        double scale = numbers[11];

        if (0.0099999998f < scale) {
            matrix.Scale((float)scale);
        } else {
            matrix.Scale(0.0099999998f);
        }
    }
}

// VERIFY: 4f254f8a manual
// OFFSET: 0x006FF030
uint32_t CMissile::GetHitResult() const {
    if (!(this->m_flags & 0x200) || (this->m_flags & 0x400)) {
        return this->m_hitResult;
    }

    if (this->m_currentTarget >= this->m_targetHitResults.m_count) {
        return 0;
    }

    return this->m_targetHitResults.m_data[this->m_currentTarget];
}

// VERIFY: 55dc76e0 manual
// OFFSET: 0x006FF070
uint32_t CMissile::GetMissReason() const {
    if (!(this->m_flags & 0x200)) {
        return this->m_missReason;
    }

    if (this->m_currentTarget >= this->m_targetMissReasons.m_count) {
        return 0;
    }

    return this->m_targetMissReasons.m_data[this->m_currentTarget];
}

// VERIFY: 4bf82194 manual
// OFFSET: 0x006FEFB0
int32_t CMissile::IsHit() const {
    if (!(this->m_flags & 0x200) || (this->m_flags & 0x400)) {
        return this->m_flags & 1;
    }

    return this->m_currentTarget < this->m_targetHitResults.m_count && !this->m_targetHitResults.m_data[this->m_currentTarget];
}

// VERIFY: 95e36e53 manual
// OFFSET: 0x006FEFF0
void CMissile::SetHitResult(uint32_t result) {
    if (!(this->m_flags & 0x200) || (this->m_flags & 0x400) || this->m_currentTarget >= this->m_targetHitResults.m_count) {
        this->m_hitResult = result;
    } else {
        this->m_targetHitResults.m_data[this->m_currentTarget] = result;
    }
}

// VERIFY: c4a5d905 auto
// OFFSET: 0x006FEBE0
void CMissile::SetRemoteCollisionPos(const C3Vector& position) {
    this->m_flags |= 0x400000;
    this->m_remoteCollisionPos = position;
}

// VERIFY: d87dbd12 manual
// OFFSET: 0x006FFBB0
void CMissile::DetermineMissType() {
    if (this->IsHit()) {
        return;
    }

    switch (this->GetHitResult()) {
        case 1:
        case 3:
        case 6: {
            this->m_flags |= 0x8;
            break;
        }

        case 2:
        case 7:
        case 8: {
            this->m_flags |= 0x20;
            break;
        }

        case 4: {
            this->m_flags |= 0x40;
            this->SetHitResult(9);
            break;
        }

        case 5: {
            this->m_flags |= 0x10;
            break;
        }

        case 11: {
            if (this->m_flags & 0x8000000) {
                this->m_flags = (this->m_flags & ~0x8000040) | 0x40;
            } else {
                this->m_flags |= 0x80;
            }

            break;
        }

        default: {
            this->m_flags |= 0x40;
            this->m_hitResult = 9;
            break;
        }
    }
}

// VERIFY: d3123a0c manual
// OFFSET: 0x006FF320
C3Vector CMissile::TransformPositionByTransport() {
    if (!this->m_transportGuid) {
        return this->m_impactPos;
    }

    C44Matrix matrix;

    if (MovementGetTransportMtxSafe(this->m_transportGuid, &matrix)) {
        this->m_worldImpactPos = this->m_impactPos * matrix;
        return this->m_worldImpactPos;
    }

    this->m_impactPos = this->m_worldImpactPos;
    this->m_transportGuid = WOWGUID();
    return this->m_worldImpactPos;
}

// VERIFY: e2196867 manual
// OFFSET: 0x006FFED0
void CMissile::UpdateProgress(float distance) {
    C3Vector impact = this->TransformPositionByTransport();
    float dx = impact.x - this->m_launchPos.x;
    float dy = impact.y - this->m_launchPos.y;
    float dz = impact.z - this->m_launchPos.z;
    float dySq = dy * dy;
    float dxSq = dx * dx;
    float remaining = sqrt(dxSq + dz * dz + dySq);
    float progress = 1.0f;

    if (!(remaining <= 1.0e-6f)) {
        progress = 1.0f - distance / remaining;
    }

    if (progress < this->m_progress) {
        progress = this->m_progress;
    }

    this->m_progress = progress;

    impact = this->TransformPositionByTransport();
    float hx = impact.x - this->m_position.x;
    float hy = impact.y - this->m_position.y;
    float total = sqrt(dxSq + dySq);
    float horizontalProgress = 1.0f;

    if (!(total <= 1.0e-6f)) {
        horizontalProgress = 1.0f - sqrt(hx * hx + hy * hy) / total;
    }

    if (horizontalProgress < this->m_horizontalProgress) {
        horizontalProgress = this->m_horizontalProgress;
    }

    this->m_horizontalProgress = horizontalProgress;
}

// VERIFY: 240015da manual
// OFFSET: 0x00700550
int32_t CMissile::ApplyMotionScriptFunctionTranslation(const SimpleScriptIO& io, C44Matrix& matrix) {
    C3Vector impact = this->TransformPositionByTransport();
    C3Vector dir(impact.x - this->m_launchPos.x, impact.y - this->m_launchPos.y, 0.0f);
    float lengthSq = dir.y * dir.y + dir.x * dir.x;

    if (lengthSq > 2.3841858e-7f) {
        float scale = 1.0f / sqrt(lengthSq);
        dir.x = dir.x * scale;
        dir.y = dir.y * scale;
        dir.z = 0.0f * scale;
    }

    C3Vector side(-dir.y, dir.x, dir.z);
    C3Vector start(matrix.d0, matrix.d1, matrix.d2);
    const double* numbers = io.m_numbers.m_data;

    if (0.0 != numbers[1]) {
        double angle = numbers[0] * 0.017453292f;
        double s = sin(angle);
        matrix.d0 = s * side.x * numbers[1] + matrix.d0;
        matrix.d1 = s * side.y * numbers[1] + matrix.d1;
        matrix.d2 = cos(angle) * numbers[1] + matrix.d2;
    }

    matrix.d0 = side.x * numbers[2] + matrix.d0;
    matrix.d1 = numbers[2] * side.y + matrix.d1;
    matrix.d2 = numbers[2] * side.z + matrix.d2;

    matrix.d0 = dir.x * numbers[3] + matrix.d0;
    matrix.d1 = dir.y * numbers[3] + matrix.d1;
    matrix.d2 = numbers[3] * dir.z + matrix.d2;

    matrix.d2 = numbers[4] + matrix.d2;

    float dx = matrix.d0 - start.x;
    float dy = matrix.d1 - start.y;
    float dz = matrix.d2 - start.z;
    float moved = dx * dx + (dy * dy + dz * dz);

    if (moved > 0.2f) {
        this->m_flags |= 0x8000;
    } else {
        this->m_flags &= ~0x8000;
    }

    return moved > 0.2f;
}

// VERIFY: 5d1af25b manual
// OFFSET: 0x00700350
int32_t CMissile::CallMotionScriptFunction(SimpleScriptIO& io) {
    io.m_numbers.m_count = 12;
    io.m_numbers.m_data[0] = this->m_progress;
    io.m_numbers.m_data[1] = (double)(int32_t)(OsGetAsyncTimeMs() - this->m_fireTime) * 0.001f;
    io.m_numbers.m_data[2] = this->m_missileIndex;
    io.m_numbers.m_data[3] = this->m_missileCount;

    float dx = this->m_launchPos.x - this->m_position.x;
    float dy = this->m_launchPos.y - this->m_position.y;
    float dz = this->m_launchPos.z - this->m_position.z;
    io.m_numbers.m_data[4] = sqrt(dz * dz + dy * dy + dx * dx);

    C3Vector impact = this->TransformPositionByTransport();
    dx = impact.x - this->m_position.x;
    dy = impact.y - this->m_position.y;
    dz = impact.z - this->m_position.z;
    io.m_numbers.m_data[5] = sqrt(dz * dz + dy * dy + dx * dx);
    io.m_numbers.m_data[6] = this->m_initialDistance;

    impact = this->TransformPositionByTransport();
    dx = impact.x - this->m_launchPos.x;
    dy = impact.y - this->m_launchPos.y;
    dz = impact.z - this->m_launchPos.z;
    io.m_numbers.m_data[7] = sqrt(dz * dz + dy * dy + dx * dx);
    io.m_numbers.m_data[8] = this->m_random[0];
    io.m_numbers.m_data[9] = this->m_random[1];
    io.m_numbers.m_data[10] = this->m_random[2];
    io.m_numbers.m_data[11] = this->m_spellID;

    if (!CSimpleScriptManager::s_instance->CallFunction(this->m_motionScriptFunction, io, io)) {
        return 0;
    }

    for (int32_t i = 0; i < 12; i++) {
        if (!_finite(io.m_numbers.m_data[i])) {
            io.m_numbers.m_data[i] = 0.0;
        }
    }

    if (io.m_numbers.m_data[8] > 0.0) {
        this->m_flags |= 0x1000000;
        this->m_speed = io.m_numbers.m_data[8];
    }

    if (io.m_numbers.m_data[9] > 0.0) {
        this->m_flags |= 0x1000000;
        this->m_speed = io.m_numbers.m_data[9] * this->m_baseSpeed;
    }

    if (0.0 != io.m_numbers.m_data[10]) {
        float scale = io.m_numbers.m_data[10] + this->m_baseSpeed;
        this->m_speed = scale;
        this->m_flags |= 0x1000000;
        this->m_speed = 0.0099999998f < scale ? scale : 0.0099999998f;
    }

    return 1;
}

// VERIFY: f5fcb978 auto
// OFFSET: 0x006FECA0
ChaserState::ChaserState(const C3Vector& position, const C3Vector& velocity) {
    this->m_position = position;
    this->m_velocity = velocity;
}

// VERIFY: 17e0df9b manual
// OFFSET: 0x006FF4C0
void ChaserState::ChaserImplementation(const C3Vector& target, float stiffness, float elapsed) {
    float x = stiffness * elapsed;
    float damping = 1.0f / (x + x * x * x * 0.235f + x * x * 0.47999999f + 1.0f);
    float dx = this->m_position.x - target.x;
    float dy = this->m_position.y - target.y;
    float dz = this->m_position.z - target.z;
    float vx = (dx * stiffness + this->m_velocity.x) * elapsed;
    float vy = (this->m_velocity.y + dy * stiffness) * elapsed;
    float vz = elapsed * (this->m_velocity.z + dz * stiffness);
    this->m_position = C3Vector((dx + vx) * damping + target.x, (dy + vy) * damping + target.y, (dz + vz) * damping + target.z);
    this->m_velocity = C3Vector((this->m_velocity.x - vx * stiffness) * damping, (this->m_velocity.y - vy * stiffness) * damping, (this->m_velocity.z - stiffness * vz) * damping);
}

// VERIFY: 1f83aaf4 auto
// OFFSET: 0x006FEAE0
static float SmoothStep(float t) {
    if (t < 0.0f) {
        return 0.0f;
    }

    if (t <= 1.0f) {
        return t * ((3.0f - (t + t)) * t);
    }

    return 1.0f;
}

// VERIFY: cdfd2d24 auto
// OFFSET: 0x006FEC10
C3Vector* CMissile::ResolveTargetPosition(C3Vector* position, WOWGUID* transport) {
    if (!this->m_transportGuid) {
        *transport = WOWGUID();
        *position = this->m_impactPos;
    } else if (MovementObjIsTransport(this->m_transportGuid)) {
        *transport = this->m_transportGuid;
        *position = this->m_impactPos;
    } else {
        *transport = WOWGUID();
        *position = this->m_worldImpactPos;
    }

    return position;
}

// VERIFY: 2f03598a manual
// OFFSET: 0x006FEDA0
void CMissile::Link(CMissile** link) {
    if (this->m_prevLink) {
        *this->m_prevLink = this->m_next;
    }

    if (this->m_next) {
        this->m_next->m_prevLink = this->m_prevLink;
    }

    this->m_next = nullptr;
    this->m_prevLink = link;
    this->m_next = *link;
    *link = this;

    if (this->m_next) {
        this->m_next->m_prevLink = &this->m_next;
    }
}

// VERIFY: 4caf78ff manual
// OFFSET: 0x006FEE00
int32_t CMissile::AttachToVictim(CGUnit_C* victim, uint32_t duration) {
    if (!this->m_model || !this->m_model->IsLoaded(0, 0) || !(this->m_model->m_fFlags & 0x8000)) {
        return 0;
    }

    if (!victim) {
        return 0;
    }

    CM2Model* victimModel = victim->m_worldModel;

    if (!victimModel || !victimModel->IsLoaded(0, 0) || !victimModel->HasAttachment(this->m_attachmentID) || this->m_fizzleTime) {
        return 0;
    }

    uint32_t destroyTime = OsGetAsyncTimeMs() + duration;
    CMissile** link = &victim->m_pendingMissiles;

    while (*link && static_cast<int32_t>((*link)->m_destroyTime - destroyTime) < 0) {
        link = &(*link)->m_next;
    }

    if (this->m_mapEntity) {
        World::ObjectDestroy(this->m_mapEntity);
    }

    this->m_mapEntity = nullptr;
    this->Link(link);
    this->m_destroyTime = destroyTime;
    this->m_model->SetBoneSequence(0xFFFFFFFF, 191, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    this->m_model->AttachToParent(victim->m_worldModel, this->m_attachmentID, nullptr, 0);
    C44Matrix attach = victim->m_worldModel->GetAttachmentWorldTransform(this->m_attachmentID);
    C44Matrix world = this->m_model->m_worldTransform;
    this->m_model->SetWorldTransform(world * attach.AffineInverse(sqrt(attach.a1 * attach.a1 + attach.a2 * attach.a2 + attach.a0 * attach.a0)));
    this->m_flags |= 2;
    return 1;
}

// VERIFY: 2870c504 manual
// OFFSET: 0x006FF410
void CMissile::ClearTargetingEffects() {
    if (!this->m_targetingUnit) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetingUnit, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->ClearEffectKit(this->m_spellID, 7, this->m_targetingEffectTime, 1, 1);
    }

    this->m_targetingUnit = WOWGUID();
}

// VERIFY: 4f946d0f auto
// OFFSET: 0x006FF6C0
void CMissile::SetReadyToFire(CMissile** list) {
    this->Link(list);
}

// VERIFY: fead66fc manual
// OFFSET: 0x006FF6D0
void CMissile::UpdateImpactPos() {
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!target) {
        return;
    }

    C3Vector position(0.0f, 0.0f, 0.0f);
    CM2Model* model = target->m_worldModel;

    if (this->m_attachmentID != -1 && model && model->HasAttachment(this->m_attachmentID) && (model->m_fFlags & 0x8000)) {
        position = this->m_attachOffset * model->GetAttachmentWorldTransform(this->m_attachmentID);
    } else {
        position = this->m_attachOffset;
        position.z = target->m_height * target->m_scale * 0.75f + position.z;

        if ((target->m_obj->m_type & TYPE_UNIT) && static_cast<CGUnit_C*>(target)->m_vehiclePassenger && model && (model->m_fFlags & 0x8000)) {
            position *= model->GetAnimatedWorldTransform();
        } else {
            position *= target->GetMatrix();
        }
    }

    if ((this->m_motionFlags & 1) && (this->m_motionFlags & 8)) {
        position.z = target->GetPosition().z + this->m_groundOffset;
    }

    if (this->m_flags & 8) {
        C3Vector delta(position.x - this->m_position.x, position.y - this->m_position.y, position.z - this->m_position.z);
        float length = sqrt(delta.z * delta.z + delta.y * delta.y + delta.x * delta.x);

        if (fabs(length) < 2.3841858e-7f) {
            return;
        }

        float inv = 1.0f / length;
        C3Vector dir(inv * delta.x, delta.y * inv, inv * delta.z);

        if (fabs(fabs(dir.z) - 1.0f) < 2.3841858e-7f) {
            return;
        }

        float extended = length + this->m_speed * 0.5f;
        C3Vector offset(extended * dir.x, dir.y * extended, extended * dir.z);
        C3Vector axis(-(delta.x * delta.z), delta.z * -delta.y, delta.x * delta.x - delta.y * -delta.y);
        float angle = sin(CRandom::sfloat_(g_rndSeed) * 1.5707964f) * 0.052359879f;
        C3Vector rotated = offset * C33Matrix::Rotation(angle, axis, false);
        this->m_impactPos.x = this->m_position.x + rotated.x;
        this->m_impactPos.y = rotated.y + this->m_position.y;
        this->m_impactPos.z = rotated.z + this->m_position.z;
        this->m_transportGuid = WOWGUID();
        return;
    }

    if (target->m_obj->m_type & TYPE_UNIT) {
        CMovement_C* movement = static_cast<CGUnit_C*>(target)->m_movement;
        C3Vector toTarget(position.x - this->m_position.x, position.y - this->m_position.y, position.z - this->m_position.z);
        float speed = movement->m_currentSpeed;
        C3Vector velocity(movement->m_moveDir.x * speed, movement->m_moveDir.y * speed, speed * movement->m_moveDir.z);
        float speedSq = this->m_speed * this->m_speed;

        if (velocity.z * velocity.z + velocity.y * velocity.y + velocity.x * velocity.x > speedSq) {
            velocity.NormalizeSafe();
            velocity.x = velocity.x * this->m_speed;
            velocity.y = velocity.y * this->m_speed;
            velocity.z = this->m_speed * velocity.z;
        }

        C3Vector half(velocity.x * 0.5f, velocity.y * 0.5f, velocity.z * 0.5f);
        float a = half.z * half.z + half.y * half.y + half.x * half.x - speedSq;
        float dot = half.z * toTarget.z + half.y * toTarget.y + half.x * toTarget.x;
        float b = dot + dot;
        float c = (toTarget.y * toTarget.y + toTarget.z * toTarget.z + toTarget.x * toTarget.x) * a;
        float discriminant = b * b - c * 4.0f;

        if (discriminant >= 0.0f && a * a >= 0.0000099999997f) {
            float inv2a = 0.5f / a;
            float root = sqrt(discriminant);
            float t1 = (-b - root) * inv2a;
            float t2 = inv2a * (root - b);
            float best = FLT_MAX;

            if (t1 > 0.0f && t1 < FLT_MAX) {
                best = t1;
            }

            if (t2 > 0.0f && t2 < best) {
                best = t2;
            }

            if (FLT_MAX != best) {
                this->m_impactPos.x = half.x * best + position.x;
                this->m_transportGuid = WOWGUID();
                this->m_impactPos.y = half.y * best + position.y;
                this->m_impactPos.z = best * half.z + position.z;
                return;
            }
        }
    }

    this->m_impactPos = position;
    this->m_transportGuid = WOWGUID();
}

// VERIFY: ca267e5d manual
// OFFSET: 0x006FFCD0
void CMissile::CalcMissFade() {
    C3Vector impact = this->TransformPositionByTransport();
    float fadeDistance = 0.0f;

    if (this->m_flags & 8) {
        fadeDistance = this->m_speed * 0.5f;
    }

    if (!this->m_model || this->m_fizzleTime) {
        return;
    }

    float dy = impact.y - this->m_position.y;
    float dx = impact.x - this->m_position.x;
    float dz = impact.z - this->m_position.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (fadeDistance * fadeDistance > distSq) {
        this->m_model->m_fadeAlpha = SmoothStep(sqrt(distSq) / fadeDistance);
    }
}

// VERIFY: f702a9e0 manual
// OFFSET: 0x006FFD70
void CMissile::ApplyTargetTransportOffset(CGObject_C* target, C3Vector* position, float elapsed) {
    WOWGUID transport;

    if (target) {
        transport.m_guid = target->GetTransportGUID();
    } else {
        transport = this->m_transportGuid;
    }

    if (!transport) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(transport, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    C3Vector transportPos = (object->m_obj->m_type & TYPE_UNIT) ? static_cast<CGUnit_C*>(object)->BuildSmoothPosition() : object->GetPosition();

    if (this->m_flags & 0x4000000) {
        C3Vector delta(transportPos.x - this->m_lastTransportPos.x, transportPos.y - this->m_lastTransportPos.y, transportPos.z - this->m_lastTransportPos.z);
        this->m_transportOffset.ChaserImplementation(delta, 5.0f, elapsed);
        position->x = position->x + this->m_transportOffset.m_position.x;
        position->y = this->m_transportOffset.m_position.y + position->y;
        position->z = this->m_transportOffset.m_position.z + position->z;
    } else {
        this->m_transportOffset.m_position = C3Vector(0.0f, 0.0f, 0.0f);
        this->m_transportOffset.m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
        this->m_flags |= 0x4000000;
    }

    this->m_lastTransportPos = transportPos;
}

// VERIFY: 60128e18 manual
// OFFSET: 0x006FFFC0
void CMissile::FollowGround(const C3Vector& direction, float distance) {
    if (distance < 0.0f) {
        distance = 0.0f;
    } else if (distance >= 50.0f) {
        distance = 50.0f;
    }

    C3Vector target(direction.x * distance * 2.0f + this->m_position.x, direction.y * distance * 2.0f + this->m_position.y, direction.z * distance * 2.0f + this->m_position.z);
    C3Vector flat;

    if (fabs(direction.z) <= 0.99900001f) {
        float inv = 1.0f / sqrt(direction.y * direction.y + direction.x * direction.x);
        flat = C3Vector(direction.x * inv, direction.y * inv, inv * 0.0f);
    } else {
        flat = C3Vector(1.0f, 0.0f, 0.0f);
    }

    int32_t steps = 4;
    float remaining = distance;
    C3Vector end;

    while (true) {
        C3Vector step(flat.x * remaining, flat.y * remaining, remaining * flat.z);
        C3Vector start = this->m_position;
        end = C3Vector(step.x + this->m_position.x, step.y + this->m_position.y, step.z + this->m_position.z);
        float t = 1.0f;
        C3Vector hit(0.0f, 0.0f, 0.0f);

        if (!World::Intersect(start, end, &hit, &t, 0x111, nullptr)) {
            break;
        }

        this->m_position.x = step.x * t + this->m_position.x;
        this->m_position.y = step.y * t + this->m_position.y;
        this->m_position.z = step.z * t + this->m_position.z;
        remaining = (1.0f - t) * remaining;
        this->m_position.z = this->m_groundOffset + this->m_position.z;

        if (t > 0.99900001f || --steps <= 0) {
            break;
        }
    }

    this->m_position = end;

    if (steps == 4) {
        C3Vector start = this->m_position;
        C3Vector down(this->m_position.x, this->m_position.y, this->m_position.z - (this->m_groundOffset + MISSILE_HEIGHT_SCAN_RANGE));
        C3Vector hit(0.0f, 0.0f, 0.0f);
        float t = 1.0f;

        if (World::Intersect(start, down, &hit, &t, 0x111, nullptr)) {
            float ground = this->m_groundOffset + hit.z;
            float z = this->m_position.z - (this->m_position.z - ground) * this->m_followGroundRate * distance;

            if (z > ground) {
                ground = z;
            }

            this->m_position.z = ground;
        }
    }

    if (this->m_followGroundStart < this->m_horizontalProgress) {
        float blend = 1.0f / (1.0f - this->m_followGroundStart) * (this->m_horizontalProgress - this->m_followGroundStart);
        blend = blend * blend;

        if (this->m_motionFlags & 4) {
            float keep = 1.0f - blend;
            this->m_position.x = target.x * blend + this->m_position.x * keep;
            this->m_position.y = this->m_position.y * keep + target.y * blend;
            this->m_position.z = target.z * blend + keep * this->m_position.z;
        } else {
            C3Vector impact = this->TransformPositionByTransport();
            float keep = 1.0f - blend;
            this->m_position.x = impact.x * blend + this->m_position.x * keep;
            this->m_position.y = impact.y * blend + this->m_position.y * keep;
            this->m_position.z = impact.z * blend + keep * this->m_position.z;
        }
    }
}

// VERIFY: c18f5cdc auto
// OFFSET: 0x006FF470
C44Matrix CMissile::GetPositionMatrix() const {
    C44Matrix matrix;
    matrix.a0 = 1.0f;
    matrix.a1 = 0.0f;
    matrix.a2 = 0.0f;
    matrix.a3 = 0.0f;
    matrix.b0 = 0.0f;
    matrix.b2 = 0.0f;
    matrix.b3 = 0.0f;
    matrix.c0 = 0.0f;
    matrix.c1 = 0.0f;
    matrix.c3 = 0.0f;
    matrix.b1 = 1.0f;
    matrix.c2 = 1.0f;
    matrix.d0 = this->m_position.x;
    matrix.d1 = this->m_position.y;
    matrix.d2 = this->m_position.z;
    matrix.d3 = 1.0f;
    return matrix;
}

// VERIFY: cb224bb9 manual
// OFFSET: 0x00700700
void CMissile::BeginWaitingForModelFX(int32_t disableEffects) {
    this->ClearTargetingEffects();

    if (disableEffects) {
        this->m_flags |= 0x10000000;
    }

    this->Link(&CMissile::s_missileDestroyList);
    CGObject_C* object = ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_OBJECT, __FILE__, __LINE__);
    CGUnit_C* target = object && (object->m_obj->m_type & TYPE_UNIT) ? static_cast<CGUnit_C*>(object) : nullptr;

    if (this->m_model && this->m_model->IsLoaded(0, 0) && (this->m_model->m_fFlags & 0x8000)) {
        if (!this->m_fizzleTime) {
            this->m_model->SetBoneSequence(0xFFFFFFFF, 191, 0xFFFFFFFF, 0, 1.0f, 1, 1);
        }

        if (target) {
            CM2Model* targetModel = target->m_worldModel;

            if (targetModel && targetModel->IsLoaded(0, 0) && targetModel->HasAttachment(this->m_attachmentID)) {
                this->m_model->AttachToParent(targetModel, this->m_attachmentID, nullptr, 0);
                C44Matrix attach = targetModel->GetAttachmentWorldTransform(this->m_attachmentID);
                C44Matrix world = this->m_model->m_worldTransform;
                this->m_model->SetWorldTransform(world * attach.AffineInverse(sqrt(attach.a1 * attach.a1 + attach.a2 * attach.a2 + attach.a0 * attach.a0)));
            }
        }
    }

    if (this->m_mapEntity) {
        World::ObjectSetUpdateCallback(this->m_mapEntity, nullptr, nullptr);
    }
}

// VERIFY: 55d630e5 manual
// OFFSET: 0x00701F50
CMissile::CMissile() {
    this->m_speed = 0.0f;
    this->m_baseSpeed = 0.0f;
    this->m_casterGuid = WOWGUID();
    this->m_targetGuid = WOWGUID();
    this->m_transportGuid = WOWGUID();
    this->m_model = nullptr;
    this->m_spellID = 0;
    this->m_position = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_launchPos = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_impactPos = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_worldImpactPos = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_lastModelPos = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_flags = 0;
    this->m_attachmentID = -1;
    this->m_attachOffset = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_soundKitID = 0;
    this->m_hitResult = 0;
    this->m_destroyTime = 0;
    this->m_victimAnimTime = 0;
    this->m_castTime = 0;
    this->m_duration = 1000;
    this->m_lastUpdateTime = 0;
    this->m_currentTarget = 0;
    this->m_scale = 1.0f;
    this->m_progress = 0.0f;
    this->m_horizontalProgress = 0.0f;
    this->m_groundOffset = 0.0f;
    this->m_followGroundRate = 0.0f;
    this->m_followGroundStart = 0.0f;
    this->m_motionFlags = 0;
    this->m_missileIndex = 0;
    this->m_missileCount = 1;
    this->m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_fizzleTime = 0;
    this->m_missileCollision = nullptr;
    this->m_timeScale = 1.0f;
    this->m_remoteCollisionPos = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_mapEntity = nullptr;
    this->m_initialDistance = 0.0f;
    this->m_mapEntity2 = nullptr;
    this->m_random[0] = 0.0f;
    this->m_prevLink = nullptr;
    this->m_random[1] = 0.0f;
    this->m_next = nullptr;
    this->m_random[2] = 0.0f;
    this->m_targetingUnit = WOWGUID();
    this->m_targetingEffectTime = 0;
    this->m_motionScriptFunction = -1;
    this->m_smoothDirection = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_lastTransportPos = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_transportOffset.m_position = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_transportOffset.m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_targets.SetCount(0);
    this->m_targetHitResults.SetCount(0);
    this->m_targetMissReasons.SetCount(0);
    this->m_targetPoints.SetCount(0);
}

// VERIFY: fd33567b manual
// OFFSET: 0x00702190
CMissile::~CMissile() {
    this->ClearTargetingEffects();

    if (this->m_model) {
        if (this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        this->m_model->Release();
        this->m_model = nullptr;
    }

    if (this->m_mapEntity) {
        World::ObjectDestroy(this->m_mapEntity);
        this->m_mapEntity = nullptr;
    }

    if (this->m_mapEntity2) {
        World::ObjectDestroy(this->m_mapEntity2);
        this->m_mapEntity2 = nullptr;
    }

    if (this->m_prevLink) {
        *this->m_prevLink = this->m_next;
    }

    if (this->m_next) {
        this->m_next->m_prevLink = this->m_prevLink;
    }

    this->m_next = nullptr;
    this->m_prevLink = nullptr;
    SI2::StopSound(&this->m_missileSound, 0, 0.15000001f, 1);
    this->m_targets.SetCount(0);
    this->m_targetHitResults.SetCount(0);
    this->m_targetMissReasons.SetCount(0);
    this->m_targetPoints.SetCount(0);
}

// VERIFY: 28f4590a auto
// OFFSET: 0x00702B10
CMissile* CMissile::NewMissile() {
    uint32_t memHandle;
    CMissile* missile = nullptr;

    if (!ObjectAlloc(*CMissile::s_missileHeap, &memHandle, reinterpret_cast<void**>(&missile), false)) {
        return nullptr;
    }

    if (missile) {
        new (missile) CMissile();
    }

    missile->m_memHandle = memHandle;
    return missile;
}

// VERIFY: 02259a6e auto
// OFFSET: 0x00702B60
CMissile* CMissile::CopyMissile(const CMissile* source) {
    uint32_t memHandle;
    CMissile* missile = nullptr;

    if (ObjectAlloc(*CMissile::s_missileHeap, &memHandle, reinterpret_cast<void**>(&missile), false)) {
        if (missile) {
            new (missile) CMissile();
        }

        missile->m_memHandle = memHandle;
    } else {
        missile = nullptr;
    }

    missile->m_casterGuid = source->m_casterGuid;
    missile->m_targetGuid = source->m_targetGuid;
    missile->m_spellID = source->m_spellID;
    missile->m_speed = source->m_speed;
    missile->m_baseSpeed = source->m_baseSpeed;
    missile->m_position = source->m_position;
    missile->m_transportGuid = source->m_transportGuid;
    missile->m_impactPos = source->m_impactPos;
    missile->m_flags = source->m_flags;
    missile->m_attachmentID = source->m_attachmentID;
    missile->m_attachOffset = source->m_attachOffset;
    missile->m_soundKitID = source->m_soundKitID;
    missile->m_hitResult = source->m_hitResult;
    missile->m_missReason = source->m_missReason;
    missile->m_destroyTime = source->m_destroyTime;
    missile->m_victimAnimTime = source->m_victimAnimTime;
    missile->m_castTime = source->m_castTime;
    missile->m_duration = source->m_duration;
    missile->m_currentTarget = source->m_currentTarget;
    missile->m_scale = source->m_scale;
    missile->m_progress = source->m_progress;
    missile->m_horizontalProgress = source->m_horizontalProgress;
    missile->m_groundOffset = source->m_groundOffset;
    missile->m_followGroundRate = source->m_followGroundRate;
    missile->m_followGroundStart = source->m_followGroundStart;
    missile->m_motionFlags = source->m_motionFlags;
    missile->m_missileIndex = source->m_missileIndex;
    missile->m_missileCount = source->m_missileCount;
    missile->m_motionScriptFunction = source->m_motionScriptFunction;
    missile->m_initialDistance = source->m_initialDistance;
    missile->m_random[0] = source->m_random[0];
    missile->m_random[1] = source->m_random[1];
    missile->m_random[2] = source->m_random[2];
    missile->m_fizzleTime = source->m_fizzleTime;
    missile->m_model = World::s_m2Scene->DuplicateModel(source->m_model, 0);
    missile->m_targets.Set(source->m_targets.m_count, source->m_targets.m_data);
    missile->m_targetHitResults.Set(source->m_targetHitResults.m_count, source->m_targetHitResults.m_data);
    missile->m_targetMissReasons.Set(source->m_targetMissReasons.m_count, source->m_targetMissReasons.m_data);
    missile->m_targetPoints.Set(source->m_targetPoints.m_count, source->m_targetPoints.m_data);
    return missile;
}

// VERIFY: f9a1bf6f auto
// OFFSET: 0x00703730
void CMissile::DeleteSelf() {
    if (this->m_missileCollision) {
        this->m_missileCollision->Free();
    }

    uint32_t* heap = CMissile::s_missileHeap;
    this->~CMissile();
    ObjectFree(*heap, this->m_memHandle);
}

// VERIFY: 878d0027 auto
// OFFSET: 0x00703770
void CMissile::RemoveMissiles() {
    while (CMissile::s_inFlightMissileList) {
        CMissile::s_inFlightMissileList->DeleteSelf();
    }

    while (CMissile::s_missileDestroyList) {
        CMissile::s_missileDestroyList->DeleteSelf();
    }
}

// VERIFY: a1f308a5 manual
// OFFSET: 0x00703810
int32_t CMissile::WaitForModelFX() {
    if (this->m_model && this->m_model->IsLoaded(0, 0) && (this->m_model->m_fFlags & 0x8000)) {
        CM2Model* model = this->m_model;

        if (this->m_flags & 0x800000) {
            if (this->m_flags & 0x10000000) {
                model->m_baseAlpha = 0.0f;
                this->m_model->SetEmittersEnabled(0);
                this->m_model->SetRibbonsEnabled(0);
                this->m_flags &= ~0x10000000;
            }
        } else {
            if (!(this->m_flags & 0x10000000)) {
                model->m_baseAlpha = 0.0f;
                this->m_model->SetEmittersEnabled(0);
                this->m_model->SetRibbonsEnabled(0);
            }

            this->m_flags |= 0x800000;
        }

        CM2Model* current = this->m_model;

        if (current->m_scene->m_frameStamp - current->m_frameStamp < 2 && (current->HasLiveParticles() || this->m_model->HasLiveRibbons())) {
            return 1;
        }
    }

    this->DeleteSelf();
    return 0;
}

// VERIFY: ae6a2e34 manual
// OFFSET: 0x00703900
void CMissile::UpdateAttachments(WOWGUID guid) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    CM2Model* unitModel = unit->m_worldModel;

    for (CMissile* missile = CMissile::s_missileDestroyList; missile; ) {
        CMissile* next = missile->m_next;

        if (missile->m_targetGuid == guid && missile->m_model && missile->m_model->m_attachParent) {
            if (unitModel && unitModel->IsLoaded(0, 0) && unitModel->HasAttachment(missile->m_attachmentID)) {
                missile->m_model->DetachFromParent();
                missile->m_model->AttachToParent(unitModel, missile->m_attachmentID, nullptr, 0);
                C44Matrix attach = unitModel->GetAttachmentWorldTransform(missile->m_attachmentID);
                C44Matrix world = missile->m_model->m_worldTransform;
                missile->m_model->SetWorldTransform(world * attach.AffineInverse(sqrt(attach.a1 * attach.a1 + attach.a2 * attach.a2 + attach.a0 * attach.a0)));
            } else {
                missile->DeleteSelf();
            }
        }

        missile = next;
    }
}

// VERIFY: 22ec6d31 auto
// OFFSET: 0x00703A80
void CMissile::ClearInFlightMissilesWithTarget(WOWGUID target) {
    for (CMissile* missile = CMissile::s_inFlightMissileList; missile; ) {
        CMissile* next = missile->m_next;

        if (missile->m_targetGuid == target) {
            missile->DeleteSelf();
        }

        missile = next;
    }
}

// VERIFY: d6890537 auto
// OFFSET: 0x00703AE0
void CMissile::ScriptManagerCallback(uint32_t event) {
    if (event == 1) {
        CMissile::RemoveMissiles();
    }
}

// VERIFY: 9611b505 manual
// OFFSET: 0x00700880
void CMissile::InitializeTrajectory() {
    C3Vector impact = this->TransformPositionByTransport();
    C3Vector delta(impact.x - this->m_launchPos.x, impact.y - this->m_launchPos.y, impact.z - this->m_launchPos.z);
    float pitch = this->m_pitch;
    C3Vector velocity(0.0f, 0.0f, 0.0f);

    if (pitch > 1.5707964f) {
        pitch = pitch - (pitch - 1.5707964f) * 2.0f;
    } else if (pitch < -1.5707964f) {
        pitch = 2.0f * (-1.5707964f - pitch) + pitch;
    }

    SpellRec spell;
    int32_t hasSpell = g_spellDB.GetRecord(this->m_spellID, &spell);
    SpellMissileRec* missile = hasSpell ? g_spellMissileDB.GetRecord(spell.m_spellMissileID) : nullptr;
    this->m_gravity = missile ? missile->m_gravity : 19.291105f;
    float speed;
    int32_t result = SpellUtils_ComputeMissileTrajectory(pitch, &delta, this->m_gravity, &speed, &this->m_travelTime, &velocity);

    if (!result) {
        pitch = (this->m_gravity < 0.0f ? -0.0099999998f : 0.0099999998f) + this->m_pitch;

        if (pitch > 1.5707964f) {
            pitch = pitch - (pitch - 1.5707964f) * 2.0f;
        } else if (pitch < -1.5707964f) {
            pitch = 2.0f * (-1.5707964f - pitch) + pitch;
        }

        result = SpellUtils_ComputeMissileTrajectory(pitch, &delta, this->m_gravity, &speed, &this->m_travelTime, &velocity);

        if (!result) {
            this->m_flags &= ~0x10000;
        }
    }

    if (result == 1) {
        if (hasSpell && missile) {
            speed = this->m_trajectorySpeed;
            this->m_travelTime = SpellUtils_ComputeMissileTrajectoryTravelTime(pitch, speed, &delta, this->m_gravity);

            if (this->m_travelTime < 0.000099999997f) {
                speed = missile->m_defaultSpeedMin;
                this->m_travelTime = SpellUtils_ComputeMissileTrajectoryTravelTime(pitch, speed, &delta, this->m_gravity);

                if (this->m_travelTime < 0.000099999997f) {
                    speed = missile->m_defaultSpeedMax;
                    this->m_travelTime = SpellUtils_ComputeMissileTrajectoryTravelTime(pitch, speed, &delta, this->m_gravity);

                    if (this->m_travelTime < 0.000099999997f) {
                        this->m_flags &= ~0x10000;
                    }
                }
            }

            if (this->m_flags & 0x10000) {
                float inv = 1.0f / this->m_travelTime;
                velocity = C3Vector(delta.x * inv, delta.y * inv, inv * delta.z);
            }
        } else {
            this->m_flags &= ~0x10000;
        }
    }

    float remaining = static_cast<int32_t>(this->m_arrivalTime - OsGetAsyncTimeMs()) * 0.001f;

    if (remaining < 0.000099999997f) {
        remaining = 0.000099999997f;
    }

    if (this->m_flags & 0x10000) {
        this->m_velocity = velocity;
        this->m_timeScale = this->m_travelTime / remaining;

        if (this->m_timeScale < 0.0f) {
            this->m_timeScale = 0.0f;
        } else if (this->m_timeScale >= 2.0f) {
            this->m_timeScale = 2.0f;
        }
    } else {
        float newSpeed = sqrt(delta.z * delta.z + delta.y * delta.y + delta.x * delta.x) / remaining;
        this->m_speed = newSpeed;
        this->m_baseSpeed = newSpeed;
    }

    if ((this->m_flags & 0x200000) && (missile->m_flags & 0x3C)) {
        CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_casterGuid, TYPE_UNIT, __FILE__, __LINE__));
        this->m_missileCollision = CMissileCollision::Alloc();
        CGUnit_C* root = caster && caster->m_vehiclePassenger && caster->m_vehiclePassenger->m_transitionVehicleGuid ? caster->m_vehiclePassenger->GetStoredRootVehicleUnit() : caster;
        C3Vector end = this->TransformPositionByTransport();
        this->m_missileCollision->Init(&this->m_launchPos, &end, pitch, speed, missile, root);
    }
}

// VERIFY: 859a92cc manual
// OFFSET: 0x00700E20
void CMissile::HitTargetAreaEffect(const SpellRec* spell, const SpellVisualRec* visual) {
    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_casterGuid, TYPE_UNIT, __FILE__, __LINE__));
    SpellVisualKitRec* areaKit = g_spellVisualKitDB.GetRecord(visual->m_impactAreaKit);

    if (areaKit && !this->m_fizzleTime) {
        C3Vector position;
        WOWGUID transport;
        this->ResolveTargetPosition(&position, &transport);

        if (caster) {
            PlaySpellVisuaKitData data(spell, areaKit, 3);
            data.m_position = &position;
            data.m_transportGUID = transport;
            data.m_someGUID = this->m_casterGuid;
            caster->PlaySpellVisualKit(data);
        } else {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                PlaySpellVisuaKitData data(spell, areaKit, 3);
                data.m_transportGUID = transport;
                data.m_position = &position;
                data.m_someGUID = this->m_casterGuid;
                data.m_playAnim = 0;
                player->PlaySpellVisualKit(data);
            }
        }
    }

    const SpellVisualKitRec* casterKit = Spell_C_GetAppropriateImpactKit(visual, 1);
    const SpellVisualKitRec* targetKit = Spell_C_GetAppropriateImpactKit(visual, 0);
    SpellVisualKitRec* stateKit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);
    uint32_t hitResultCount = this->m_targetHitResults.m_count;

    for (uint32_t i = 0; i < this->m_targets.m_count; i++) {
        WOWGUID targetGuid;
        targetGuid.m_guid = this->m_targets.m_data[i];
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(targetGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            continue;
        }

        if (hitResultCount) {
            uint32_t result = this->m_targetHitResults.m_data[i];

            if (result && !(this->m_flags & 0x100) && !this->m_missileIndex) {
                if (spell && !(spell->m_attributes & 0x100)) {
                    unit->AddWorldText(this->m_casterGuid, static_cast<MISS_REASON>(result), spell, 0);
                    CGGameUI::ShowSpellMissFeedback(unit->m_obj->m_guid, result);
                }

                UnitCombatLogSpellMissed(this->m_casterGuid, unit->m_obj->m_guid, this->m_spellID, result, 0, 0, 0);
            }
        } else if (!this->m_fizzleTime) {
            const SpellVisualKitRec* kit = unit != caster ? targetKit : casterKit;

            if (kit) {
                PlaySpellVisuaKitData data(spell, kit, 1);
                data.m_someGUID = this->m_casterGuid;
                unit->PlaySpellVisualKit(data);
            }

            if (stateKit && unit->m_unit->m_unitFieldHealth > 0 && unit->HasAuraBySpellId(spell->m_id) && (static_cast<int8_t>(stateKit->m_flags) >= 0 || unit->HasAuraActive(spell->m_id))) {
                PlaySpellVisuaKitData data(spell, stateKit, 2);
                data.m_someGUID = this->m_casterGuid;
                unit->PlaySpellVisualKit(data);
            }
        }
    }

    for (uint32_t i = 0; i < this->m_targetPoints.m_count; i++) {
        if (!targetKit || !caster) {
            continue;
        }

        PlaySpellVisuaKitData data(spell, targetKit, 1);
        SpellCastTargetPoint* point = &this->m_targetPoints.m_data[i];

        if (point->m_transport && MovementObjIsTransport(point->m_transport)) {
            data.m_transportGUID = point->m_transport;
            data.m_position = &point->m_position;
        } else {
            data.m_transportGUID = WOWGUID();
            data.m_position = &point->m_worldPosition;
        }

        caster->PlaySpellVisualKit(data);
    }

    if (hitResultCount) {
        this->m_flags |= 0x100;
    }

    this->BeginWaitingForModelFX(0);
}

// VERIFY: 72a88afd manual
// OFFSET: 0x00701230
void CMissile::ComputeModelMatrix(C44Matrix* matrix, const C3Vector* position, const C3Vector* direction) {
    matrix->d0 = position->x;
    matrix->d1 = position->y;
    matrix->d2 = position->z;

    if (this->m_motionScriptFunction != -1) {
        if (this->CallMotionScriptFunction(s_msfIO)) {
            this->ApplyMotionScriptFunctionTranslation(s_msfIO, *matrix);
        } else {
            this->m_motionScriptFunction = -1;
        }
    }

    int32_t animated = 0;

    if (this->m_flags & 0x2000000) {
        CM2Model* model = this->m_model;
        C3Vector translation(matrix->d0, matrix->d1, matrix->d2);
        model->Animate();
        *matrix = model->m_matrixF4 * model->m_scene->m_viewInv;
        matrix->d0 = translation.x;
        matrix->d1 = translation.y;
        matrix->d2 = translation.z;
        this->m_flags &= ~0x2000000;
        animated = 1;
    } else {
        bool haveForward = false;

        if (this->m_model && (this->m_flags & 0x800)) {
            C3Vector delta(matrix->d0 - this->m_lastModelPos.x, matrix->d1 - this->m_lastModelPos.y, matrix->d2 - this->m_lastModelPos.z);
            float length = sqrt(delta.y * delta.y + delta.z * delta.z + delta.x * delta.x);

            if (length > 2.3841858e-7f) {
                if (length < 0.0099999998f && (this->m_flags & 0x100000)) {
                    float weight = length * 100.0f;

                    if (weight < 0.050000001f) {
                        weight = 0.050000001f;
                    }

                    delta *= weight;
                    delta += this->m_smoothDirection * (1.0f - weight);
                    length = sqrt(delta.y * delta.y + delta.z * delta.z + delta.x * delta.x);
                }

                this->m_smoothDirection = delta;
                this->m_flags |= 0x100000;
                float inv = 1.0f / length;
                matrix->a0 = delta.x * inv;
                matrix->a1 = delta.y * inv;
                matrix->a2 = inv * delta.z;
                haveForward = true;
            }
        }

        if (!haveForward) {
            matrix->a0 = direction->x;
            matrix->a1 = direction->y;
            matrix->a2 = direction->z;
        }

        float forwardX = matrix->a0;
        matrix->b0 = -matrix->a1;
        matrix->b1 = forwardX;
        matrix->b2 = 0.0f;

        if (matrix->b1 * matrix->b1 + matrix->b0 * matrix->b0 > 0.0000099999997f) {
            float inv = 1.0f / sqrt(matrix->b0 * matrix->b0 + matrix->b1 * matrix->b1);
            matrix->b0 = inv * matrix->b0;
            matrix->b1 = inv * matrix->b1;
            C2Vector side(matrix->b0, matrix->b1);
            matrix->c0 = -(side.y * matrix->a2);
            matrix->c1 = side.x * matrix->a2;
            matrix->c2 = matrix->a0 * side.y - matrix->a1 * side.x;
        } else if (this->m_model && (this->m_flags & 0x800) && (matrix->a0 = direction->x, matrix->a1 = direction->y, matrix->a2 = direction->z, matrix->b0 = -direction->y, matrix->b1 = direction->x, matrix->b2 = 0.0f, matrix->b1 * matrix->b1 + matrix->b0 * matrix->b0 > 0.0000099999997f)) {
            C2Vector side(matrix->b0, matrix->b1);
            side.Normalize();
            matrix->b0 = side.x;
            matrix->b1 = side.y;
            C3Vector up = C3Vector(matrix->a0, matrix->a1, matrix->a2) % C2Vector(matrix->b0, matrix->b1);
            matrix->c0 = up.x;
            matrix->c1 = up.y;
            matrix->c2 = up.z;
        } else {
            matrix->a0 = 0.0f;
            matrix->a1 = 0.0f;
            matrix->a2 = 1.0f;
            matrix->b0 = 1.0f;
            matrix->b1 = 0.0f;
            matrix->b2 = 0.0f;
            matrix->c0 = 0.0f;
            matrix->c2 = 0.0f;
            matrix->c1 = 1.0f;
        }
    }

    if (this->m_motionScriptFunction != -1) {
        this->ApplyMotionScriptFunctionRotationAndScale(s_msfIO, *matrix);
    }

    if (!animated) {
        matrix->Scale(this->m_scale);
    }
}

// VERIFY: 4d97ff64 manual
// OFFSET: 0x00702A60
int32_t CMissile::PossiblyHandleBounce() {
    if (!(this->m_flags & 0x200)) {
        return 0;
    }

    int32_t fired = 0;
    uint32_t index = ++this->m_currentTarget;

    if (index < this->m_targets.m_count) {
        this->m_targetGuid.m_guid = this->m_targets.m_data[index];
        this->m_flags &= 0xF7FFFA04;
        this->FireMissiles(&this->m_position);
        fired = 1;
    }

    if (this->m_currentTarget >= this->m_targetPoints.m_count) {
        return fired;
    }

    SpellCastTargetPoint* point = &this->m_targetPoints.m_data[this->m_currentTarget];
    this->m_impactPos = point->m_position;
    this->m_transportGuid = point->m_transport;
    this->m_flags &= 0xF7FFFA04;
    this->FireMissiles(&this->m_position);
    return 1;
}

// VERIFY: 87fb757a manual
// OFFSET: 0x00702DB0
void CMissile::HitTarget(CGUnit_C* target, const SpellRec* spell, const SpellVisualRec* visual) {
    if (target->m_unit->m_unitFieldHealth > 0) {
        if (!visual || !(visual->m_flags & 0x40)) {
            target->PlayWoundAnimKit(0);
        }
    } else {
        target->PlayDeathAnimation(0);
    }

    if (spell && visual && !this->m_fizzleTime) {
        CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_casterGuid, TYPE_UNIT, __FILE__, __LINE__));
        const SpellVisualKitRec* kit = Spell_C_GetAppropriateImpactKit(visual, target == caster);
        C3Vector position(0.0f, 0.0f, 0.0f);
        C3Vector* positionPtr = nullptr;
        WOWGUID transport = WOWGUID();

        if (this->m_model && (this->m_flags & 0x8000)) {
            position = (this->m_model->m_fFlags & 0x8000) ? this->m_model->GetWorldPosition() : target->GetPosition();
            positionPtr = &position;
        } else if ((this->m_motionFlags & 1) && (this->m_motionFlags & 8)) {
            C3Vector resolved;
            position = *this->ResolveTargetPosition(&resolved, &transport);
            positionPtr = &position;
        }

        if (kit) {
            PlaySpellVisuaKitData data(spell, kit, 1);
            data.m_transportGUID = transport;
            data.m_someGUID = this->m_casterGuid;
            data.m_position = positionPtr;
            target->PlaySpellVisualKit(data);
        }

        SpellVisualKitRec* stateKit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);

        if (stateKit && target->m_unit->m_unitFieldHealth > 0 && target->HasAuraBySpellId(spell->m_id) && (static_cast<int8_t>(stateKit->m_flags) >= 0 || target->HasAuraActive(spell->m_id))) {
            PlaySpellVisuaKitData data(spell, stateKit, 2);
            data.m_someGUID = this->m_casterGuid;
            target->PlaySpellVisualKit(data);
        }
    }

    if ((this->m_attachmentID == -1 || !target->GetObjectModel() || !(this->m_flags & 4) || !this->AttachToVictim(target, 5000)) && !this->PossiblyHandleBounce()) {
        this->BeginWaitingForModelFX(0);
    }
}

// VERIFY: f4431d3e manual
// OFFSET: 0x00703410
void CMissile::ReachedTarget(CGUnit_C* target, const C3Vector* direction) {
    this->m_progress = 1.0f;
    this->m_horizontalProgress = 1.0f;
    this->m_position = this->TransformPositionByTransport();
    C3Vector dir = *direction;
    float length = sqrt(dir.z * dir.z + dir.y * dir.y + dir.x * dir.x);

    if (fabs(length) < 2.3841858e-7f) {
        dir = C3Vector(0.0f, 0.0f, 1.0f);
    } else {
        float inv = 1.0f / length;
        dir.x = inv * dir.x;
        dir.y = dir.y * inv;
        dir.z = inv * dir.z;
    }

    if (this->m_model && (this->m_model->m_fFlags & 0x8000)) {
        C44Matrix matrix;
        this->ComputeModelMatrix(&matrix, &this->m_position, &dir);
        CM2Model* model = this->m_model;
        model->m_fFlags |= 0x8000;
        model->m_worldTransform = matrix;
        this->m_lastModelPos = C3Vector(matrix.d0, matrix.d1, matrix.d2);
    }

    SI2::StopSound(&this->m_missileSound, 0, 0.15000001f, 1);
    this->m_flags &= ~0x10000;
    SpellRec spell;

    if (g_spellDB.GetRecord(this->m_spellID, &spell)) {
        const SpellVisualRec* visual = GetSpellVisual(&spell);

        if (target) {
            if (this->IsHit()) {
                this->HitTarget(target, &spell, visual);
                return;
            }
        } else if (visual) {
            this->HitTargetAreaEffect(&spell, visual);
            return;
        }
    } else if (target && this->IsHit()) {
        this->HitTarget(target, nullptr, nullptr);
        return;
    }

    this->MissedTarget(target, direction);
}

// VERIFY: ad9ee4a8 manual
// OFFSET: 0x00703640
void CMissile::HandleMissileTrajectoryCollision(const C3Vector* direction) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_UPDATE_PROJECTILE_POSITION));
    msg.Put(this->m_casterGuid.m_guid);
    msg.Put(this->m_spellID);
    msg.Put(this->m_castID);
    msg.Put(this->m_position.x);
    msg.Put(this->m_position.y);
    msg.Put(this->m_position.z);
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->m_impactPos = this->m_position;
    this->m_transportGuid = WOWGUID();
    this->ReachedTarget(nullptr, direction);
}

// VERIFY: 95c5a21d manual
// OFFSET: 0x007015D0
void CMissile::CalcPosition(int32_t advance) {
    uint32_t now = OsGetAsyncTimeMs();
    float elapsed = advance ? (now - this->m_lastUpdateTime) * 0.001f : 0.0f;
    this->m_lastUpdateTime = now;

    if (0.0f == this->m_speed) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_OBJECT, __FILE__, __LINE__);
    CGUnit_C* target = static_cast<CGUnit_C*>(object);
    int32_t targetIsUnit;

    if (object && (object->m_obj->m_type & TYPE_UNIT)) {
        targetIsUnit = 1;

        if (!(this->m_flags & 8)) {
            this->UpdateImpactPos();
        }
    } else {
        targetIsUnit = 0;
    }

    if (this->m_victimAnimTime && static_cast<int32_t>(now - this->m_victimAnimTime) >= 0) {
        this->m_victimAnimTime = 0;

        if (targetIsUnit) {
            uint32_t animId = (this->m_flags & 0x10) ? 24 : 30;
            M2SequenceInfo info;
            info.Clear();
            target->m_worldModel->GetSequenceInfo(animId, 0, info);

            if (info.m_animId) {
                target->PlayBaseAnimation(animId, 0);
            }
        }
    }

    if (this->m_flags & 0x2000) {
        this->ApplyTargetTransportOffset(object, &this->m_launchPos, elapsed);
    }

    this->ApplyTargetTransportOffset(object, &this->m_position, elapsed);
    float step = elapsed * this->m_speed;
    C3Vector impact = this->TransformPositionByTransport();
    uint32_t flags = this->m_flags;
    C3Vector delta(impact.x - this->m_position.x, impact.y - this->m_position.y, impact.z - this->m_position.z);
    uint32_t arrival = this->m_arrivalTime;
    float distance = sqrt(delta.y * delta.y + delta.x * delta.x + delta.z * delta.z);
    float timeScale = this->m_timeScale;
    int32_t timed = (flags & 0x1024000) == 0x20000 && elapsed > 0.0f && this->m_duration > 0.0f;
    uint32_t trajectory = flags & 0x10000;

    if (trajectory) {
        if (flags & 0x40000) {
            float t = this->m_adjustedDuration ? (now - this->m_adjustTime) / static_cast<float>(this->m_adjustedDuration) : 1.0f;
            arrival += (this->m_adjustedDuration - (arrival - this->m_adjustTime)) * t;
            float targetScale = arrival == this->m_fireTime ? 1.0f : this->m_travelTime / ((arrival - this->m_fireTime) * 0.001f);
            timeScale = t * (targetScale - this->m_timeScale) + this->m_timeScale;
        }

        if (static_cast<int32_t>(now - arrival) >= 0) {
            this->ReachedTarget(targetIsUnit ? target : nullptr, &delta);
            return;
        }
    } else if (timed) {
        if (static_cast<int32_t>(now - this->m_fireTime - this->m_duration) >= 0) {
            this->ReachedTarget(targetIsUnit ? target : nullptr, &delta);
            return;
        }
    } else if (step >= distance && !(flags & 2)) {
        this->ReachedTarget(targetIsUnit ? target : nullptr, &delta);
        return;
    }

    if (step <= 0.0f && this->m_mapEntity) {
        return;
    }

    C3Vector dir = delta;

    if (fabs(distance) < 2.3841858e-7f) {
        dir = C3Vector(0.0f, 0.0f, 1.0f);
    } else {
        float inv = 1.0f / distance;
        dir.x = delta.x * inv;
        dir.y = dir.y * inv;
        dir.z = inv * dir.z;
    }

    C3Vector previous = this->m_position;

    if (trajectory) {
        float total = (arrival - this->m_fireTime) * 0.001f;
        float left = 0.001f * (arrival - now);
        float t = timeScale * (total - left);
        this->m_position.x = this->m_velocity.x * t + this->m_launchPos.x;
        this->m_position.y = this->m_velocity.y * t + this->m_launchPos.y;
        this->m_position.z = this->m_velocity.z * t + this->m_launchPos.z;
        this->m_position.z = this->m_position.z - t * (this->m_gravity * t) * 0.5f;

        if (this->m_flags & 0x1000) {
            float progress = 1.0f - left / total;
            this->m_progress = progress;
            this->m_horizontalProgress = progress;
        }
    } else if (timed) {
        float t = (now - this->m_fireTime) / static_cast<float>(this->m_duration);
        this->m_progress = t;
        this->m_horizontalProgress = t;
        C3Vector target = this->TransformPositionByTransport();
        float keep = 1.0f - this->m_progress;
        this->m_position.x = target.x * t + this->m_launchPos.x * keep;
        this->m_position.y = target.y * t + this->m_launchPos.y * keep;
        this->m_position.z = keep * this->m_launchPos.z + target.z * t;
    } else {
        if (step > 0.0f) {
            if (this->m_flags & 0x4000) {
                this->FollowGround(dir, step);
            } else {
                this->m_position += C3Vector(dir.x * step, dir.y * step, step * dir.z);
            }
        }

        if (this->m_flags & 0x1000) {
            this->UpdateProgress(distance - step);
        }
    }

    if (this->m_flags & 0x400000) {
        float dz = this->m_position.z - this->m_remoteCollisionPos.z;
        float dy = this->m_position.y - this->m_remoteCollisionPos.y;
        float dx = this->m_position.x - this->m_remoteCollisionPos.x;
        float current = dx * dx + dy * dy + dz * dz;
        float pz = previous.z - this->m_remoteCollisionPos.z;
        float py = previous.y - this->m_remoteCollisionPos.y;
        float px = previous.x - this->m_remoteCollisionPos.x;

        if (py * py + pz * pz + px * px < current) {
            this->m_impactPos = previous;
            this->m_transportGuid = WOWGUID();
            this->ReachedTarget(nullptr, &delta);
            return;
        }
    }

    if (this->m_missileCollision) {
        float t = this->m_missileCollision->ComputeSegmentImpactTime(&previous, &this->m_position);

        if (t >= 0.0f) {
            this->m_position.x = (this->m_position.x - previous.x) * t + previous.x;
            this->m_position.y = (this->m_position.y - previous.y) * t + previous.y;
            this->m_position.z = t * (this->m_position.z - previous.z) + previous.z;
            this->HandleMissileTrajectoryCollision(&delta);
            return;
        }
    }

    if (this->m_model) {
        C44Matrix matrix;
        this->ComputeModelMatrix(&matrix, &this->m_position, &dir);
        CM2Model* model = this->m_model;
        model->m_fFlags |= 0x8000;
        model->m_worldTransform = matrix;
        this->m_lastModelPos = C3Vector(matrix.d0, matrix.d1, matrix.d2);

        if (this->m_fizzleTime) {
            float fade = (this->m_fizzleTime - now) * 0.0020000001f;
            this->m_model->m_fadeAlpha = fclamp(fade, 0.0f, 1.0f);

            if (fade <= 0.0f) {
                this->BeginWaitingForModelFX(0);
                return;
            }
        }

        if (!this->m_mapEntity && (this->m_model->m_fFlags & 0x8000)) {
            this->m_mapEntity = CMap::ObjectCreate(this->m_model, &CGWorldFrame::MissileEnumProc, nullptr, 0, reinterpret_cast<uintptr_t>(this), 6);
        }

        CAaBox box;
        CAaSphere sphere;

        if (this->m_model->IsLoaded(0, 0)) {
            box = this->m_model->GetBoundingBox();
            sphere = this->m_model->GetBoundingSphere();

            if ((this->m_flags & 0x90000) == 0x10000) {
                for (uint32_t i = 0; ; i++) {
                    CM2Model* emitterModel = this->m_model;

                    if (!(emitterModel->m_fFlags & 1)) {
                        emitterModel->WaitForLoad(nullptr);
                    }

                    if (i >= emitterModel->m_shared->m_data->m_particles.m_count) {
                        break;
                    }

                    emitterModel = this->m_model;

                    if (!(emitterModel->m_fFlags & 1)) {
                        emitterModel->WaitForLoad(nullptr);
                    }

                    emitterModel->m_particleEmitters[i]->m_flags |= 0x400000;
                }

                this->m_flags |= 0x80000;
            }
        }

        C3Vector zero(0.0f, 0.0f, 0.0f);
        CMap::ObjectUpdate(this->m_mapEntity, matrix, box, sphere, zero, false, 0xFFFFFFFF);
        this->m_model->SetAnimating(1);
    }

    if (SI2::IsPlaying(&this->m_missileSound)) {
        C3Vector position = this->m_model ? this->m_model->GetWorldPosition() : this->m_position;
        SI2::Set3DPosition(&this->m_missileSound, &position);
    }
}

// VERIFY: fa72d815 manual
// OFFSET: 0x00702FC0
void CMissile::MissedTarget(CGUnit_C* target, const C3Vector* direction) {
    if (target) {
        if (!(this->m_flags & 0x100)) {
            SpellRec spell;

            if (g_spellDB.GetRecord(this->m_spellID, &spell) && !(spell.m_attributes & 0x100) && !this->m_missileIndex) {
                target->AddWorldText(this->m_casterGuid, static_cast<MISS_REASON>(this->GetHitResult()), &spell, 0);
                CGGameUI::ShowSpellMissFeedback(target->m_obj->m_guid, this->GetHitResult());
                UnitCombatLogSpellMissed(this->m_casterGuid, target->m_obj->m_guid, this->m_spellID, this->GetHitResult(), 0, 0, 0);
            }

            this->m_flags |= 0x100;
        }

        if (!(this->m_flags & 0x58)) {
            M2SequenceInfo info;
            info.Clear();
            target->m_worldModel->GetSequenceInfo(30, 0, info);

            if (info.m_animId) {
                target->PlayBaseAnimation(30, 0);
            }
        }

        uint32_t flags = this->m_flags;

        if (flags & 0x10) {
            if (this->AttachToVictim(target, 5000)) {
                this->PossiblyHandleBounce();
                return;
            }

            if (!this->PossiblyHandleBounce()) {
                this->BeginWaitingForModelFX(0);
            }

            return;
        }

        if (flags & 0x20) {
            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->AddSpecialByPosition(SPECIALEFFECT_RESISTSPELL, this->m_position, &target->m_obj->m_guid, 32, &CGObject_C::EffectOnceHandler);
            effect->m_flags |= 0x8000;
            C44Matrix matrix = this->m_model ? this->m_model->m_worldTransform : this->GetPositionMatrix();
            effect->m_facing = atan2(matrix.b1, matrix.b0) - 1.5707964f;
            effect->Release();

            if (!this->PossiblyHandleBounce()) {
                this->BeginWaitingForModelFX(0);
            }

            return;
        }
    }

    uint32_t flags = this->m_flags;

    if (!(flags & 8)) {
        if (flags & 0x40) {
            C3Vector dir(-direction->x, -direction->y, -direction->z);
            float length = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

            if (fnotequal(length, 0.0f) && fnotequal(length, fabs(dir.z))) {
                dir /= length;
                C33Matrix first = C33Matrix::Rotation(CRandom::sfloat_(g_rndSeed) * 1.5707964f, dir, true);
                C3Vector side(dir.y, -dir.x, 0.0f);
                side *= first;
                C33Matrix second = C33Matrix::Rotation(CRandom::sfloat_(g_rndSeed) * 1.5707964f, side, false);
                dir *= second;
                float speed = this->m_speed * 0.5f;
                this->m_speed = speed;
                this->m_impactPos.x = dir.x * (0.5f * speed) + this->m_position.x;
                this->m_impactPos.y = 0.5f * speed * dir.y + this->m_position.y;
                this->m_impactPos.z = 0.5f * speed * dir.z + this->m_position.z;
                this->m_transportGuid = WOWGUID();

                if (this->m_model) {
                    this->m_model->SetBoneSequence(0xFFFFFFFF, 198, 0xFFFFFFFF, 0, 1.0f, 1, 1);
                }

                this->m_flags = (this->m_flags & ~0x48) | 8;
                this->PossiblyHandleBounce();
                return;
            }
        } else if (flags & 0x80) {
            this->m_targetGuid = this->m_casterGuid;
            this->m_flags = (flags & 0xFFFFFA7F) | 0x400;
            uint32_t reason = this->GetMissReason();
            this->SetHitResult(reason);

            if (!reason) {
                this->m_flags |= 1;
            }

            this->FireMissiles(&this->m_position);
            return;
        }
    }

    if (!this->PossiblyHandleBounce()) {
        this->BeginWaitingForModelFX(0);
    }
}

// VERIFY: d0f1a70e manual
// OFFSET: 0x007022D0
void CMissile::FireMissiles(const C3Vector* position) {
    if (!this->m_currentTarget) {
        this->m_lastUpdateTime = OsGetAsyncTimeMs();
    }

    this->DetermineMissType();
    this->m_launchPos = *position;
    this->m_position = *position;
    this->Link(&CMissile::s_inFlightMissileList);

    if (this->m_flags & 0x10000) {
        this->InitializeTrajectory();
    }

    if (this->m_flags & 0x10) {
        this->m_attachmentID = 0;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (target && target->m_worldModel && target->m_worldModel->IsLoaded(0, 0)) {
        if (!target->m_worldModel->HasAttachment(this->m_attachmentID)) {
            if (target->m_worldModel->HasAttachment(15)) {
                this->m_attachmentID = 15;
            } else {
                this->m_attachmentID = target->m_worldModel->HasAttachment(19) ? 19 : -1;
            }
        }
    } else if (!this->m_targetGuid || target) {
        this->m_attachmentID = -1;
    } else {
        if (!this->PossiblyHandleBounce()) {
            C3Vector up(1.0f, 0.0f, 0.0f);
            this->ReachedTarget(nullptr, &up);
        }

        return;
    }

    if (!this->m_targetGuid) {
        this->m_flags |= 0x20000;
    }

    this->m_progress = 0.0f;
    this->m_horizontalProgress = 0.0f;
    C3Vector impact = this->TransformPositionByTransport();
    C3Vector delta(impact.x - this->m_position.x, impact.y - this->m_position.y, impact.z - this->m_position.z);
    this->m_fireTime = OsGetAsyncTimeMs();
    this->m_initialDistance = sqrt(delta.x * delta.x + delta.z * delta.z + delta.y * delta.y);

    uint32_t bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
    this->m_random[0] = *reinterpret_cast<float*>(&bits) - 1.0f;
    bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
    this->m_random[1] = *reinterpret_cast<float*>(&bits) - 1.0f;
    bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
    this->m_random[2] = *reinterpret_cast<float*>(&bits) - 1.0f;

    this->m_lastModelPos = C3Vector(this->m_position.x - delta.x, this->m_position.y - delta.y, this->m_position.z - delta.z);
    this->m_smoothDirection = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_flags &= ~0x100000;
    this->UpdateImpactPos();
    this->CalcPosition(0);
    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_spellID, &spell)) {
        return;
    }

    SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell.m_spellVisualID[0]);

    if (this->m_model) {
        if (this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        if (spell.m_attributesExE & 0x40000000) {
            this->m_model->m_fFlags |= 4;
        }

        float distance;

        if (this->m_speed <= 0.0f || this->m_currentTarget) {
            distance = this->m_initialDistance;
        } else {
            float maxSpeed = this->m_speed + this->m_speed;
            CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_casterGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (visual && visual->m_flags < 0) {
                if (caster) {
                    C3Vector offset = this->TransformPositionByTransport() - this->m_launchPos;
                    distance = sqrt(offset.z * offset.z + offset.y * offset.y + offset.x * offset.x);
                } else {
                    distance = this->m_initialDistance;
                }
            } else if (caster) {
                C3Vector casterPos = caster->GetPosition();
                C3Vector offset = this->TransformPositionByTransport() - casterPos;
                distance = sqrt(offset.z * offset.z + offset.y * offset.y + offset.x * offset.x);
            } else {
                distance = this->m_initialDistance;
            }

            float remainingTime = distance / this->m_speed - static_cast<int32_t>(this->m_fireTime - this->m_castTime) * 0.001f;
            float speed;

            if (remainingTime > 0.0f) {
                speed = distance / remainingTime;

                if (maxSpeed < speed) {
                    speed = maxSpeed;
                }
            } else {
                remainingTime = 0.0f;
                speed = maxSpeed;
            }

            this->m_speed = speed;
            this->m_duration = static_cast<uint32_t>(static_cast<int64_t>(distance / this->m_speed * 1000.0f));

            if (target && (this->m_flags & 0x50)) {
                uint32_t animId = (this->m_flags & 0x10) ? 24 : 30;
                M2SequenceInfo info;
                info.Clear();
                target->m_worldModel->GetSequenceInfo(animId, 0, info);
                int32_t delay = FloatToInt64(remainingTime * 1000.0f) - (info.m_duration >> 1);

                if (delay <= 0) {
                    if (info.m_animId) {
                        target->PlayBaseAnimation(animId, 0);
                    }
                } else {
                    this->m_victimAnimTime = delay + OsGetAsyncTimeMs();
                }
            }
        }

        if (!this->m_currentTarget) {
            if (this->m_initialDistance <= 0.0f) {
                this->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
            } else {
                float speedScale;
                M2SequenceInfo info;

                if (this->m_model->IsLoaded(0, 0) && this->m_model->HasSequence(144) && (info.Clear(), this->m_model->GetSequenceInfo(144, 0, info), info.m_flags & 1)) {
                    speedScale = info.m_duration * this->m_speed * 0.001f / distance;
                } else {
                    speedScale = 1.0f;
                }

                this->m_model->SetBoneSequence(0xFFFFFFFF, 144, 0, 0, speedScale, 1, 1);
            }
        }
    }

    this->ClearTargetingEffects();

    if (visual && visual->m_missileTargetingKit > 0) {
        SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_missileTargetingKit);

        if (kit) {
            CGUnit_C* unit = target;

            if (unit || (this->m_casterGuid && !this->m_missileIndex && (unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_casterGuid, TYPE_UNIT, __FILE__, __LINE__))))) {
                this->m_targetingEffectTime = OsGetAsyncTimeMs();

                if (!target) {
                    C3Vector targetPos;
                    WOWGUID transport;
                    this->ResolveTargetPosition(&targetPos, &transport);
                    PlaySpellVisuaKitData data(&spell, kit, 7);
                    data.m_position = &targetPos;
                    data.m_transportGUID = transport;
                    data.m_someGUID = this->m_casterGuid;
                    data.m_playAnim = 0;
                    data.m_effectFadeTime = this->m_targetingEffectTime;
                    unit->PlaySpellVisualKit(data);
                } else {
                    PlaySpellVisuaKitData data(&spell, kit, 2);
                    data.m_someGUID = this->m_casterGuid;
                    data.m_effectFadeTime = this->m_targetingEffectTime;
                    unit->PlaySpellVisualKit(data);
                }

                this->m_targetingUnit = unit->m_obj->m_guid;
            }
        }
    }

    if (!SI2::IsPlaying(&this->m_missileSound) && this->m_soundKitID) {
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_fadeInTime = 0.5f;
        properties.m_loopMode = 1;
        properties.m_noRandomStart = 1;
        properties.m_maxAudibleBehavior = 0;
        properties.m_streaming = true;

        if (this->m_casterGuid == CGUnit_C::s_activeMover || this->m_targetGuid == CGUnit_C::s_activeMover) {
            properties.m_priority = 110;
        }

        SI2::PlaySoundKit(this->m_soundKitID, &this->m_position, &this->m_missileSound, &properties, 0, nullptr, 1, 0);
    }
}

// VERIFY: fbab6522 auto
// OFFSET: 0x00703B00
void CMissile::UpdateStaticLists() {
    for (CMissile* missile = CMissile::s_inFlightMissileList; missile; ) {
        CMissile* next = missile->m_next;
        missile->CalcPosition(CGWorldFrame::s_currentWorldFrame->m_elapsedSec > 0.0f);

        if (missile->m_flags & 8) {
            missile->CalcMissFade();
        }

        missile = next;
    }

    for (CMissile* missile = CMissile::s_missileDestroyList; missile; ) {
        CMissile* next = missile->m_next;
        missile->WaitForModelFX();
        missile = next;
    }
}

// VERIFY: 589857c0 auto
// OFFSET: 0x006FF0A0
void CMissile::InstallMotionScriptFunction(int32_t motionId) {
    this->m_motionScriptFunction = CSimpleScriptManager::s_instance->FindFunction(motionId);

    if (this->m_motionScriptFunction != -1) {
        return;
    }

    SpellMissileMotionRec* motion = g_spellMissileMotionDB.GetRecord(motionId);

    if (!motion) {
        return;
    }

    SimpleScriptFunctionDesc desc;
    desc.m_name = motion->m_name;
    desc.m_args = s_msfArgs;
    desc.m_stringArgs = nullptr;
    desc.m_body = motion->m_scriptBody;
    desc.m_returns = s_msfReturns;
    desc.m_stringReturns = nullptr;
    this->m_motionScriptFunction = CSimpleScriptManager::s_instance->CreateFunction(motionId, desc);
}
