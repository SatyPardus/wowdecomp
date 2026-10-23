#include "Camera.h"
#include "Engine/Source/Base/Coordinate.h"
#include <cmath>
#include <cstring>
#include <limits>
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CFacet.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VehicleCamera_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/BarberShop.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"

static const float ROTATION_SPEED = 0.09f;
static const float MAX_DELTA = 0.2617994f;
static const float FOCAL_DISTANCE = 3.3333333f;
static const float WAIST_HEIGHT = 1.6666666f;

static const float MIN_CAMERA_DIST = 0.0f;
static const int32_t DELTA_Z_FREQUENCY = 100;

static const float fadeAngle = -1.1868238f;

// STATIC: 0x00AD1B4C
float MAX_PITCH_ANGLE = 1.55334306f;

// STATIC: 0x00AD1B50
float MIN_PITCH_ANGLE = -1.55334306f;

// STATIC: 0x00AD1B80
int32_t s_cameraViewAngular[3] = { 0, 1, 1 };

// STATIC: 0x00AD1BA8
CAMERA_DEFAULTS s_cameraViewDefaults[8] = {
    { { "0.0", "0.0", "0.0" } },
    { { "0.0", "0.0", "0.0" } },
    { { "5.55", "10.0", "0.0" } },
    { { "5.55", "20.0", "0.0" } },
    { { "13.88", "30.0", "0.0" } },
    { { "13.88", "10.0", "0.0" } },
    { { "0.0", "0.0", "0.0" } },
    { { "5.0", "10.0", "0.0" } },
};

// STATIC: 0x00A1E9C8
const float s_terrainTiltTable[10][2] = {
    { 0.0f, 0.0f },
    { 0.09f, 0.0872664626f },
    { 0.18f, 0.174532925f },
    { 0.27f, 0.261799388f },
    { 0.36f, 0.349065850f },
    { 0.47f, 0.436332313f },
    { 0.58f, 0.523598776f },
    { 0.7f, 0.610865238f },
    { 0.84f, 0.698131701f },
    { 1.0f, 0.785398163f },
};

// STATIC: 0x00C24E80
float s_targetHeightOffset;

// STATIC: 0x00AD213C
static const char* s_cameraViewModeNames[8] = { "FIRST_PERSON", "THIRD_PERSON_A", "THIRD_PERSON_B", "THIRD_PERSON_C", "THIRD_PERSON_D", "THIRD_PERSON_E", "VIEW_COMMENTATOR", "VIEW_BARBER_SHOP" };

// STATIC: 0x00A1E368
static const float s_smoothViewDataMin[2] = { 0.0f, 0.0f };

// STATIC: 0x00A1E370
static const float s_smoothViewDataMax[2] = { 100.0f, 100.0f };

// STATIC: 0x00AD2178
static float s_lastLiquidBumpPitch = std::numeric_limits<float>::max();

static const float CAMERASHAKEMINDISTANCE = 9.0f;
static const float CAMERASHAKEMAXDISTANCE = 80.0f;

// STATIC: 0x00AD2114
static float s_shakeFalloffStartSqr = 81.0f;

// STATIC: 0x00AD2118
static float s_shakeFalloffMaxSqr = 6400.0f;

// VERIFY: 42a53d4c auto
// OFFSET: 0x005FD5C0
static float UnwrapAngleToward(float target, float current, float lo, float hi) {
    while (target - current < lo) {
        current -= TWO_PI;
    }

    while (target - current > hi) {
        current += TWO_PI;
    }

    return current;
}

// VERIFY: 170cb080 auto
// OFFSET: 0x005FE3C0
static float WrapAngle0To2Pi(float angle) {
    if (angle < 0.0f) {
        return angle + TWO_PI;
    }

    if (angle > TWO_PI) {
        return angle - TWO_PI;
    }

    return angle;
}

// VERIFY: 0904e9a9 auto
// OFFSET: 0x005FF500
float CGCamera::FOV() const {
    float fov = this->m_targetFov + this->m_fov;
    return 0.0f > fov ? 0.0f : (3.1241393f <= fov ? 3.1241393f : fov);
}

// VERIFY: ccdea4b6 manual
// OFFSET: 0x00600C20
C3Vector CGCamera::Forward() const {
    C3Vector forward(this->m_facing.a0, this->m_facing.a1, this->m_facing.a2);

    if (!this->m_relativeTo) {
        return forward;
    }

    return forward * this->ParentToWorld();
}

// VERIFY: c03a0375 manual
// OFFSET: 0x00600CC0
C3Vector CGCamera::Right() const {
    C3Vector right(this->m_facing.b0, this->m_facing.b1, this->m_facing.b2);

    if (!this->m_relativeTo) {
        return right;
    }

    return right * this->ParentToWorld();
}

// VERIFY: e84ff384 manual
// OFFSET: 0x00600D60
C3Vector CGCamera::Up() const {
    C3Vector up(this->m_facing.c0, this->m_facing.c1, this->m_facing.c2);

    if (!this->m_relativeTo) {
        return up;
    }

    return up * this->ParentToWorld();
}

// VERIFY: 6dc11132 auto
// OFFSET: 0x00606B30
CGCamera::CGCamera() : CSimpleCamera(World::s_nearClip, World::s_farClip, 1.5707964f) {
    this->m_model = nullptr;
    this->m_someTime = OsGetAsyncTimeMs();
    this->m_modelCameraHandle = nullptr;

    this->m_modelMatrix.a0 = 1.0f;
    this->m_modelMatrix.a1 = 0.0f;
    this->m_modelMatrix.a2 = 0.0f;
    this->m_modelMatrix.b0 = 0.0f;
    this->m_modelMatrix.b1 = 1.0f;
    this->m_modelMatrix.b2 = 0.0f;
    this->m_modelMatrix.c0 = 0.0f;
    this->m_modelMatrix.c1 = 0.0f;
    this->m_modelMatrix.c2 = 1.0f;
    this->m_modelMatrix.d0 = 0.0f;
    this->m_modelMatrix.d1 = 0.0f;
    this->m_modelMatrix.d2 = 0.0f;

    this->m_targetGUID = WOWGUID();
    this->m_terrainTilt = 0.0f;
    this->m_guid0090 = WOWGUID();
    this->m_state = 0;
    this->m_flags = 0;
    this->m_relativeTo = WOWGUID();
    this->m_ignoreFacingRefs = 0;
    this->m_groundTiltRefs = 0;

    this->m_viewIndex = s_cvCameraView->m_intValue;
    this->m_distance = SStrToFloat(s_cameraViewDefaults[this->m_viewIndex].m_values[0]);
    this->m_yaw = 0.0f;
    this->m_pitch = SStrToFloat(s_cameraViewDefaults[this->m_viewIndex].m_values[1]) * 0.017453292f;
    this->m_roll = 0.0f;
    this->m_height = 0.0f;
    this->m_yawOffset = 0.0f;
    this->m_targetOffset = 0.0f;
    this->m_groundTilt = 0.0f;
    this->m_targetFov = 0.0f;
    this->m_flyingMountHeight = 0.0f;
    this->m_bobOffset.x = 0.0f;
    this->m_bobOffset.y = 0.0f;
    this->m_bobOffset.z = 0.0f;
    this->m_targetHeightNear = 0.0f;
    this->m_heightChangedTimeMs = 0;
    this->m_targetHeightFar = 0.0f;
    this->m_motionFlags = 0;
    this->m_targetHeightSwim = 0.0f;
    this->m_mountHeight = 0.0f;
    this->m_targetPosition.x = 0.0f;
    this->m_targetPosition.y = 0.0f;
    this->m_targetPosition.z = 0.0f;
    this->unused_01D0 = 0.0f;
    this->m_targetFacing = 0.0f;
    this->m_terrainTiltTime = 0;
    this->m_freeLookYaw = 0.0f;

    this->m_smoothDistance.m_startTimeMs = 0;
    this->m_smoothDistance.m_rate = 0.0f;
    this->m_smoothDistance.m_target = this->m_distance;
    this->m_smoothDistance.m_startValue = 0.0f;
    this->m_smoothDistance.m_param3 = 0.0f;
    this->m_smoothDistance.m_param4 = 0.0f;

    this->m_smoothGroundTilt.m_startTimeMs = 0;
    this->m_smoothGroundTilt.m_rate = 0.0f;
    this->m_smoothGroundTilt.m_target = 0.0f;
    this->m_smoothGroundTilt.m_startValue = 0.0f;
    this->m_smoothGroundTilt.m_param3 = 0.0f;
    this->m_smoothGroundTilt.m_param4 = 0.0f;

    this->m_smoothHeight.m_startTimeMs = 0;
    this->m_smoothHeight.m_rate = 0.0f;
    this->m_smoothHeight.m_target = 0.0f;
    this->m_smoothHeight.m_startValue = 0.0f;
    this->m_smoothHeight.m_param3 = 0.0f;
    this->m_smoothHeight.m_param4 = 0.0f;

    this->m_smoothPitch.m_startTimeMs = 0;
    this->m_smoothPitch.m_rate = 0.0f;
    this->m_smoothPitch.m_target = this->m_pitch;
    this->m_smoothPitch.m_startValue = 0.0f;
    this->m_smoothPitch.m_param3 = 0.0f;
    this->m_smoothPitch.m_param4 = 0.0f;

    this->m_smoothTargetOffset.m_startTimeMs = 0;
    this->m_smoothTargetOffset.m_rate = 0.0f;
    this->m_smoothTargetOffset.m_target = 0.0f;
    this->m_smoothTargetOffset.m_startValue = 0.0f;
    this->m_smoothTargetOffset.m_param3 = 0.0f;
    this->m_smoothTargetOffset.m_param4 = 0.0f;

    this->m_smoothYaw.m_startTimeMs = 0;
    this->m_smoothYaw.m_rate = 0.0f;
    this->m_smoothYaw.m_target = 0.0f;
    this->m_smoothYaw.m_startValue = 0.0f;
    this->m_smoothYaw.m_param3 = 0.0f;
    this->m_smoothYaw.m_param4 = 0.0f;

    this->m_smoothFoV.m_startTimeMs = 0;
    this->m_smoothFoV.m_rate = 0.0f;
    this->m_smoothFoV.m_target = 0.0f;
    this->m_smoothFoV.m_startValue = 0.0f;
    this->m_smoothFoV.m_param3 = 1.0f;
    this->m_smoothFoV.m_param4 = 0.0f;

    this->m_bobTime = 0;
    this->m_bobDuration = 0.0f;
    this->m_bobStart.x = 0.0f;
    this->m_bobStart.y = 0.0f;
    this->m_bobStart.z = 0.0f;
    this->unused_029C = 1;
    this->m_underwater = 0;
    this->m_wasFlying = 0;

    this->m_smoothFlyingHeight.m_startTimeMs = 0;
    this->m_smoothFlyingHeight.m_rate = 0.0f;
    this->m_smoothFlyingHeight.m_target = 0.0f;
    this->m_smoothFlyingHeight.m_startValue = 0.0f;
    this->m_smoothFlyingHeight.m_param3 = 0.0f;
    this->m_smoothFlyingHeight.m_param4 = 0.0f;

    this->m_collideExtent = 1.0f;
    this->m_vehicleZoomEnabled = 0;
    this->m_overrideDistanceMin = 0.0f;
    this->m_overrideDistanceMax = 0.0f;
    this->m_vehicleExitZoomDistance = 0.0f;
    this->m_vehicleYawStartTime = 0;
    this->m_vehicleYawEndTime = 0;
    this->m_vehicleYawFrom = 0.0f;
    this->m_vehicleYawTo = 0.0f;
    this->m_vehicleYawOffset = 0.0f;
    this->m_vehiclePitchStartTime = 0;
    this->m_vehiclePitchEndTime = 0;
    this->m_vehiclePitchFrom = 0.0f;
    this->m_vehiclePitchTo = 0.0f;
    this->m_vehiclePitchOffset = 0.0f;
    this->m_vehicleTrackingStyle = 0;
    this->m_vehicleCamera = nullptr;

    memset(this->m_views, 0, sizeof(this->m_views));

    this->m_motionStartTime[CAMERA_MOVE_IN] = 0;
    this->m_motionStartTime[CAMERA_MOVE_OUT] = 0;
    this->m_motionStartTime[CAMERA_MOVE_RIGHT] = 0;
    this->m_motionStartTime[CAMERA_MOVE_LEFT] = 0;
    this->m_motionStartTime[CAMERA_MOVE_UP] = 0;
    this->m_motionStartTime[CAMERA_MOVE_DOWN] = 0;
    this->m_motionStopTime[CAMERA_MOVE_IN] = 0;
    this->m_motionStopTime[CAMERA_MOVE_OUT] = 0;
    this->m_motionStopTime[CAMERA_MOVE_RIGHT] = 0;
    this->m_motionStopTime[CAMERA_MOVE_LEFT] = 0;
    this->m_motionStopTime[CAMERA_MOVE_UP] = 0;
    this->m_motionStopTime[CAMERA_MOVE_DOWN] = 0;
    this->m_motionEndTime[CAMERA_MOVE_IN] = 0;
    this->m_motionEndTime[CAMERA_MOVE_OUT] = 0;
    this->m_motionEndTime[CAMERA_MOVE_RIGHT] = 0;
    this->m_motionEndTime[CAMERA_MOVE_LEFT] = 0;
    this->m_motionEndTime[CAMERA_MOVE_UP] = 0;
    this->m_motionEndTime[CAMERA_MOVE_DOWN] = 0;

    this->InitializeView();
    this->SetTarget(nullptr, 0);

    ConsoleCommandRegister("pitchLimit", CGCamera::CCommand_PitchLimit, CATEGORY_GAME, nullptr);

    this->m_state |= 0x50;
    this->m_prevViewIndex = 2;
    this->m_prevDistance = 1.8315002f;
    this->m_preTelescopeDistance = 1.8315002f;
    this->m_telescopePitchOffset = 0.0f;

    if (g_cvAutoInteract->m_intValue) {
        this->m_flags |= 0x1;
    } else {
        this->m_flags &= ~0x1;
    }
}

// VERIFY: 847cb2d7 manual
// OFFSET: 0x00604D40
CGCamera::~CGCamera() {
    this->m_state &= ~0x10;

    if (this->m_model) {
        this->m_state &= ~0x4;

        if (this->m_modelCameraHandle) {
            HandleClose(this->m_modelCameraHandle);
            this->m_modelCameraHandle = nullptr;
        }

        this->m_model->Release();
        this->m_model = nullptr;
    }

    ConsoleCommandUnregister("pitchLimit");
}

// VERIFY: 6952a863 manual
// OFFSET: 0x00603900
void CGCamera::BarberShopSetView(float distance, float facing, float pitch) {
    this->m_views[7].m_distance = distance;
    this->m_views[7].m_yaw = facing;
    this->m_views[7].m_pitch = pitch;
    this->SetView(7, 1, 0);
}

// VERIFY: 3e9e8f60 manual
// OFFSET: 0x00603A10
void CGCamera::CalcTerrainTilt(CGObject_C* target, uint32_t time) {
    if (!target) {
        return;
    }

    if (!(target->m_obj->m_type & TYPE_UNIT) || !s_cvCameraTerrainTilt->m_intValue) {
        this->m_terrainTilt = 0.0f;
        return;
    }

    C3Vector position = this->GetSafeWorldPos(target);
    float facing = target->GetFacing();

    if (static_cast<int32_t>(time - this->m_terrainTiltTime - DELTA_Z_FREQUENCY) < 0) {
        return;
    }

    this->m_terrainTiltTime = time;

    C3Vector points[5];
    position.z += WAIST_HEIGHT;

    C3Vector direction;
    direction.x = cosf(facing);
    direction.y = sinf(facing);

    points[0] = position;

    C3Vector end;
    end.x = direction.x * FOCAL_DISTANCE + position.x;
    end.y = direction.y * FOCAL_DISTANCE + position.y;
    end.z = FOCAL_DISTANCE * direction.z + position.z;

    points[1] = end;
    points[2] = end;

    float distance = 1.0f;
    World::Intersect(points[0], points[1], &points[2], &distance, 0x151, nullptr);

    points[2].x -= direction.x * 0.27777779f;
    points[2].y -= direction.y * 0.27777779f;
    points[2].z -= 0.27777779f * direction.z;

    points[3].x = points[2].x;
    points[3].y = points[2].y;
    points[3].z = points[2].z - 7.1111112f;
    points[4] = points[3];

    distance = 1.0f;
    World::Intersect(points[2], points[3], &points[4], &distance, 0x151, nullptr);

    C3Vector delta;
    delta.x = points[2].x - points[0].x;
    delta.y = points[2].y - points[0].y;
    delta.z = points[2].z - points[0].z;

    C2Vector horizontal(delta);
    float slope = (points[4].z - points[2].z + WAIST_HEIGHT) / sqrtf(horizontal.y * horizontal.y + horizontal.x * horizontal.x);

    float magnitude;
    float sign;

    if (slope >= 0.0f) {
        magnitude = slope;
        sign = -1.0f;
    } else {
        magnitude = -slope;
        sign = 1.0f;
    }

    uint32_t i = 10;

    do {
        i--;

        if (s_terrainTiltTable[i][0] <= magnitude) {
            float tilt = s_terrainTiltTable[i][1];

            if (tilt <= 0.0f) {
                this->m_terrainTilt = 0.0f;
            } else {
                tilt *= sign;
                this->m_terrainTilt = tilt < -0.34906584f ? -0.34906584f : (tilt >= 0.34906584f ? 0.34906584f : tilt);
            }

            return;
        }
    } while (i);

    this->m_terrainTilt = 0.0f;
}

// VERIFY: 59c41f1e manual
// OFFSET: 0x005FFC20
void CGCamera::ClampPitchToLimits(float delta) {
    float target = delta + this->m_smoothPitch.m_target;
    target = MIN_PITCH_ANGLE > target ? MIN_PITCH_ANGLE : (MAX_PITCH_ANGLE <= target ? MAX_PITCH_ANGLE : target);
    this->m_smoothPitch.m_target = target;

    float start = delta + this->m_smoothPitch.m_startValue;
    start = start < MIN_PITCH_ANGLE ? MIN_PITCH_ANGLE : (start < MAX_PITCH_ANGLE ? start : MAX_PITCH_ANGLE);
    this->m_smoothPitch.m_startValue = UnwrapAngleToward(target, start, -PI, PI);

    float pitch = this->m_pitch + delta;
    pitch = MIN_PITCH_ANGLE > pitch ? MIN_PITCH_ANGLE : (MAX_PITCH_ANGLE <= pitch ? MAX_PITCH_ANGLE : pitch);
    this->m_pitch = UnwrapAngleToward(target, pitch, -PI, PI);
}

// VERIFY: 9448842b manual
// OFFSET: 0x00601A70
void CGCamera::DecIgnoreFacing() {
    if (--this->m_ignoreFacingRefs != 0) {
        return;
    }

    float facing = 0.0f;
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        if (target->m_obj->m_type & TYPE_UNIT) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(target);

            if (this->m_vehicleCamera && this->m_vehicleCamera->IsHierarchyChasingFacing()) {
                facing = this->GetChaseFacing(unit);
            } else if (this->m_state & 0x4000) {
                facing = this->m_freeLookYaw;
            } else if (this->m_vehicleCamera) {
                facing = this->GetChaseFacing(unit);
            } else {
                facing = unit->GetRawFacing();
            }
        } else {
            facing = target->GetFacing();
        }
    }

    this->UpdateYaw(-facing);
}

// VERIFY: 7782f635 manual
// OFFSET: 0x00604E00
int32_t CGCamera::FinishLoadingTarget(CGObject_C* target) {
    CM2Model* model = target->GetObjectModel();

    if (model && !model->IsLoaded(0, 0)) {
        return 0;
    }

    this->m_state |= 0x4;
    this->m_targetHeightNear = 0.0f;
    this->m_targetHeightFar = 0.0f;
    this->m_targetHeightSwim = 0.0f;

    float scale = target->GetScale() * target->m_scaleModifier;
    float maxHeight;

    if ((target->m_obj->m_type & TYPE_UNIT) && model) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(target);

        float mountScale = scale;

        if (unit->m_mountModel) {
            mountScale = scale * unit->m_mountScale;
        }

        CM2Model* worldModel = unit->m_worldModel;
        uint32_t attachment = 0xFFFFFFFF;
        uint32_t sequence;

        switch (unit->GetClientStandState()) {
            case 1:
                sequence = 97;
                break;

            case 3:
                sequence = 100;
                break;

            case 4:
                sequence = 102;
                break;

            case 5:
                sequence = 103;
                break;

            case 6:
                sequence = 104;
                break;

            case 7:
                sequence = 6;
                break;

            case 8:
                sequence = 115;
                break;

            default:
                sequence = 0;
                break;
        }

        if (s_cvCameraHeightIgnoreStandState->m_intValue) {
            sequence = 0;
        }

        if (unit->m_mountModel) {
            sequence = 91;
            attachment = 0;
        }

        M2SequenceInfo standInfo;
        M2SequenceInfo sequenceInfo;
        standInfo.Clear();
        sequenceInfo.Clear();
        worldModel->GetSequenceInfo(0, 0, standInfo);
        worldModel->GetSequenceInfo(sequence, 0, sequenceInfo);
        standInfo.m_bounds *= scale;
        sequenceInfo.m_bounds *= scale;

        float standOffset = standInfo.m_bounds.t.z - sequenceInfo.m_bounds.t.z;
        float height;

        if (worldModel->HasAttachment(0x11)) {
            C3Vector pivot = worldModel->GetAttachmentPivot(0x11);

            if (CGBarberShop::m_barberShopEnabled && unit->IsActivePlayer()) {
                height = (pivot.z + 0.097222224f - standOffset) * scale;
            } else {
                height = (pivot.z + 0.097222224f) * scale;
            }
        } else {
            if (!(worldModel->m_shared->m_flags & 0x1)) {
                worldModel->WaitForLoad(nullptr);
            }

            CAaBox bounds = worldModel->m_shared->m_data->m_collisionBounds.m_extent;
            height = (bounds.t.z - bounds.b.z) * scale * 0.9f;
        }

        this->m_targetHeightNear += height;
        this->m_targetHeightFar += height;
        this->m_targetHeightSwim = height + this->m_targetHeightSwim;

        if (!worldModel->HasSequence(sequence)) {
            sequence = 0;
        }

        if (worldModel->HasSequence(42)) {
            M2SequenceInfo swimStandInfo;
            M2SequenceInfo swimInfo;
            swimStandInfo.Clear();
            swimInfo.Clear();
            worldModel->GetSequenceInfo(0, 0, swimStandInfo);
            worldModel->GetSequenceInfo(42, 0, swimInfo);
            swimStandInfo.m_bounds *= scale;
            swimInfo.m_bounds *= scale;
            this->m_targetHeightSwim -= swimStandInfo.m_bounds.t.z - swimInfo.m_bounds.t.z;
        }

        this->m_targetHeightNear -= standOffset;

        if (sequence == 91) {
            this->m_targetHeightFar -= standOffset;
        }

        if (attachment != 0xFFFFFFFF && model->HasAttachment(attachment)) {
            C3Vector pivot = model->GetAttachmentPivot(attachment);
            float mountHeight = pivot.z * mountScale;

            if ((unit->m_unit->m_unitFieldFlags & 0x100000) && model->HasSequence(135)) {
                M2SequenceInfo mountStandInfo;
                M2SequenceInfo mountInfo;
                mountStandInfo.Clear();
                mountInfo.Clear();
                model->GetSequenceInfo(0, 0, mountStandInfo);
                model->GetSequenceInfo(135, 0, mountInfo);
                mountStandInfo.m_bounds *= mountScale;
                mountInfo.m_bounds *= mountScale;
                mountHeight -= mountStandInfo.m_bounds.t.z - mountInfo.m_bounds.t.z;
            }

            this->m_targetHeightNear += mountHeight;
            this->m_targetHeightFar += mountHeight;
            this->m_targetHeightSwim = mountHeight + this->m_targetHeightSwim;
        }

        maxHeight = unit->GetMaxCameraHeight();

        if (maxHeight <= 0.0f) {
            maxHeight = 15.0f;
        }

        if (CGBarberShop::m_barberShopEnabled) {
            maxHeight *= 1.2;
        }
    } else {
        float height;

        if ((target->m_obj->m_type & TYPE_DYNAMICOBJECT) && model) {
            CAaSphere sphere = model->GetBoundingSphere();

            if (sphere.r > 0.01f) {
                height = sphere.r * 0.99f * scale;
            } else {
                height = scale + scale;
            }
        } else {
            height = scale + scale;
        }

        this->m_targetHeightSwim = height;
        this->m_targetHeightFar = height;
        this->m_targetHeightNear = height;
        maxHeight = 15.0f;
    }

    this->m_targetHeightNear = this->m_targetHeightNear < 0.8333333f ? 0.8333333f : (this->m_targetHeightNear >= maxHeight ? maxHeight : this->m_targetHeightNear);
    this->m_targetHeightFar = this->m_targetHeightFar < 0.8333333f ? 0.8333333f : (this->m_targetHeightFar >= maxHeight ? maxHeight : this->m_targetHeightFar);
    this->m_targetHeightSwim = this->m_targetHeightSwim < 0.8333333f ? 0.8333333f : (this->m_targetHeightSwim >= maxHeight ? maxHeight : this->m_targetHeightSwim);

    this->UpdateTargetHeight(target, OsGetAsyncTimeMs());
    this->UpdateLiquidSurfaceStatus(target);

    return 1;
}

// VERIFY: ebd6bcc3 auto
// OFFSET: 0x006009E0
float CGCamera::GetChaseFacing(CGUnit_C* unit) const {
    if (this->m_vehicleCamera) {
        return this->m_vehicleCamera->m_facing;
    }

    return unit->m_smoothTarget.m_value;
}

// VERIFY: 92dc79b4 auto
// OFFSET: 0x00603090
C3Vector CGCamera::GetSafeWorldPos(CGObject_C* target) const {
    CVehicleCamera_C* vehicleCamera = this->m_vehicleCamera;

    if (vehicleCamera) {
        if (!(vehicleCamera->m_flags & 0x40)) {
            vehicleCamera->ComputeSafeCurWorldPos();
        }

        return vehicleCamera->m_safeCurWorldPos;
    }

    return target->GetPosition();
}

// VERIFY: e4b8b4fb manual
// OFFSET: 0x006019B0
void CGCamera::IncIgnoreFacing() {
    if (this->m_ignoreFacingRefs++ != 0) {
        return;
    }

    float facing = 0.0f;
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        if (target->m_obj->m_type & TYPE_UNIT) {
            facing = this->GetChaseFacing(static_cast<CGUnit_C*>(target));
        } else {
            facing = target->GetFacing();
        }
    }

    this->UpdateYaw(facing);
}

// VERIFY: 7c4b102e auto
// OFFSET: 0x005FE510
void CGCamera::InitializeView() {
    for (uint32_t i = 0; i < 8; i++) {
        for (uint32_t j = 0; j < 3; j++) {
            float value = s_cvCameraViewData[i][j]->m_floatValue;

            if (s_cameraViewAngular[j]) {
                value *= 0.017453292f;
            }

            this->m_views[i].Component(j) = value;
        }
    }
}

// VERIFY: 39e54cfd manual
// OFFSET: 0x00604A70
void CGCamera::MakeRelativeTo(WOWGUID guid) {
    if (guid == this->m_relativeTo) {
        return;
    }

    if (this->m_relativeTo && this->m_ignoreFacingRefs > 0) {
        CGObject_C* relativeTo = ClntObjMgrObjectPtr(this->m_relativeTo, TYPE_OBJECT, __FILE__, __LINE__);

        if (relativeTo) {
            float facing;

            if (relativeTo->m_obj->m_type & TYPE_UNIT) {
                facing = static_cast<CGUnit_C*>(relativeTo)->GetSmoothFacing();
            } else {
                facing = relativeTo->GetFacing();
            }

            this->UpdateYaw(facing);
        }
    }

    this->m_relativeTo = guid;

    if (guid && this->m_ignoreFacingRefs > 0) {
        CGObject_C* relativeTo = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);
        float facing;

        if (relativeTo->m_obj->m_type & TYPE_UNIT) {
            facing = static_cast<CGUnit_C*>(relativeTo)->GetSmoothFacing();
        } else {
            facing = relativeTo->GetFacing();
        }

        this->UpdateYaw(-facing);
    }

    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        float yaw;
        float pitch;
        float roll;
        this->UpdateTargetFacing(target, yaw, pitch, roll);
    }
}

// VERIFY: 728f9ebd manual
// OFFSET: 0x00600B60
C33Matrix CGCamera::ParentToWorld() const {
    CGObject_C* parent = ClntObjMgrObjectPtr(this->m_relativeTo, TYPE_OBJECT, __FILE__, __LINE__);

    if (!parent) {
        C33Matrix identity;
        identity.a0 = 1.0f;
        identity.a1 = 0.0f;
        identity.a2 = 0.0f;
        identity.b0 = 0.0f;
        identity.b1 = 1.0f;
        identity.b2 = 0.0f;
        identity.c0 = 0.0f;
        identity.c1 = 0.0f;
        identity.c2 = 1.0f;
        return identity;
    }

    float facing;

    if (parent->m_obj->m_type & TYPE_UNIT) {
        CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ parent->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);
        facing = static_cast<CGUnit_C*>(parent)->m_smoothTarget.m_value;
        CVehicleCamera_C::ConvertSmoothFacingFromRawToWorld(facing, transport);
    } else {
        facing = parent->GetFacing();
    }

    return C33Matrix::Rotation(facing);
}

// VERIFY: fcfbed22 manual
// OFFSET: 0x00601B70
void CGCamera::PerformTerrainTilt(CGObject_C* target, uint32_t time, int32_t immediate) {
    if (!target || !(target->m_obj->m_type & TYPE_UNIT)) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(target);
    int32_t style = s_cvCameraSmoothStyle->m_intValue;
    int32_t state;

    if (unit->m_unit->m_unitFieldFlags & 0x100000) {
        state = 7;
    } else {
        uint32_t moveFlags = unit->m_movement->m_flags;

        if (moveFlags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING)) {
            state = 6;
        } else if (moveFlags & MOVEMENTFLAG_FALLING) {
            state = 0;
        } else if (unit->IsJumping()) {
            state = 3;
        } else if (this->m_state & 0x100) {
            state = 8;
        } else if (this->m_state & 0x1000) {
            state = 1;
        } else if (moveFlags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD)) {
            state = 4;
        } else if (moveFlags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            state = 5;
        } else {
            state = (moveFlags & (MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT)) ? 9 : 2;
        }
    }

    float delay = s_cvCameraTerrainTiltState[style][state][1]->m_floatValue;
    float scale = s_cvCameraTerrainTiltState[style][state][2]->m_floatValue;
    float tilt = s_cvCameraTerrainTiltState[style][state][0]->m_floatValue * this->m_terrainTilt;

    if (scale < 0.0f) {
        tilt = this->m_groundTilt;
    } else if (immediate || fequal(scale, 0.0f)) {
        this->m_groundTilt = tilt;
    } else {
        if (this->SetDesiredGroundOffset(tilt, delay, scale, time)) {
            this->m_smoothGroundTilt.m_rate = fclamp(this->m_smoothGroundTilt.m_rate, scale * s_cvCameraTerrainTiltTimeMin->m_floatValue, s_cvCameraTerrainTiltTimeMax->m_floatValue * scale);
        }

        return;
    }

    this->m_state &= ~0x10000000;
    this->m_smoothGroundTilt.m_target = tilt;
    this->m_smoothGroundTilt.m_startTimeMs = 0;
    this->m_smoothGroundTilt.m_rate = 0.0f;
}

// VERIFY: 3bb6ba68 manual
// OFFSET: 0x00604B90
void CGCamera::PickVehicleCamera() {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGObject_C* target;

    if (!player || !(target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__)) || !(target->m_obj->m_type & TYPE_UNIT)) {
        this->m_vehicleCamera = nullptr;
        return;
    }

    CVehicleCamera_C* previous = this->m_vehicleCamera;
    this->m_vehicleCamera = player->m_vehicleCamera;

    if (!this->m_vehicleCamera || this->m_vehicleCamera->m_targetGuid != this->m_targetGUID) {
        this->m_vehicleCamera = static_cast<CGUnit_C*>(target)->m_vehicleCamera;
    }

    if (this->m_vehicleCamera) {
        this->MakeRelativeTo(WOWGUID{ this->m_vehicleCamera->GetRelativeTo() });
    }

    if (previous && previous != this->m_vehicleCamera) {
        previous->m_unit->OnVehicleCameraPossiblyUnneeded();
    }
}

// VERIFY: cfdf0028 manual
// OFFSET: 0x006030E0
int32_t CGCamera::SetDesiredFoVOffset(float offset, float delay, float scale, uint32_t time) {
    this->m_targetFov = UnwrapAngleToward(offset, this->m_targetFov, -PI, PI);

    if ((this->m_state & 0x40000000) && fabsf(this->m_smoothFoV.m_target - offset) < 0.001f && fabsf(this->m_smoothFoV.m_param4 - delay) < 0.001f && fabsf(this->m_smoothFoV.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_targetFov - offset) < 0.001f) {
        return 0;
    }

    this->m_smoothFoV.m_param4 = delay;
    this->m_smoothFoV.m_param3 = scale;

    float duration;

    if (this->m_state & 0x80008000) {
        duration = 0.5f;
    } else {
        duration = scale * (fabsf(offset - this->m_targetFov) / (s_cvCameraFoVSmoothSpeed->m_floatValue * 0.017453292f));
    }

    return this->StartSmoothFoV(offset, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: efa747ec manual
// OFFSET: 0x00600FA0
int32_t CGCamera::SetDesiredGroundOffset(float offset, float delay, float scale, uint32_t time) {
    this->m_groundTilt = UnwrapAngleToward(offset, this->m_groundTilt, -PI, PI);

    if ((this->m_state & 0x10000000) && fabsf(this->m_smoothGroundTilt.m_target - offset) < 0.001f && fabsf(this->m_smoothGroundTilt.m_param4 - delay) < 0.001f && fabsf(this->m_smoothGroundTilt.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_groundTilt - offset) < 0.001f) {
        return 0;
    }

    this->m_smoothGroundTilt.m_param4 = delay;
    this->m_smoothGroundTilt.m_param3 = scale;

    float duration = scale * (fabsf(offset - this->m_groundTilt) / (s_cvCameraGroundSmoothSpeed->m_floatValue * 0.017453292f));

    return this->StartSmoothGroundTilt(offset, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: 74aed777 manual
// OFFSET: 0x00601FF0
void CGCamera::SetModeFreeLook() {
    if (this->m_state & 0x1) {
        return;
    }

    this->m_state |= 0x1;
    SysMsgAdd("Camera FREELOOK", SYSMSG_INFO);
    this->IncIgnoreFacing();

    if (this->m_groundTiltRefs++ == 0) {
        this->ClampPitchToLimits(this->m_groundTilt);
    }

    this->m_state &= 0xF4FFBFFF;
    this->m_flags &= ~0x4;

    this->m_smoothPitch.m_target = this->m_pitch;
    this->m_smoothPitch.m_startTimeMs = 0;
    this->m_smoothPitch.m_rate = 0.0f;

    this->m_smoothTargetOffset.m_startTimeMs = 0;
    this->m_smoothTargetOffset.m_target = this->m_targetOffset;
    this->m_smoothTargetOffset.m_rate = 0.0f;

    this->m_smoothYaw.m_startTimeMs = 0;
    this->m_smoothYaw.m_target = this->m_yaw;
    this->m_smoothYaw.m_rate = 0.0f;
}

// VERIFY: 02e820ff manual
// OFFSET: 0x00601F70
void CGCamera::SetModeNormal() {
    if (!(this->m_state & 0x1) || (this->m_flags & 0x2)) {
        return;
    }

    this->m_state &= ~0x1;

    if (this->m_vehicleCamera && (this->m_vehicleCamera->m_flags & 0x10)) {
        this->m_state &= ~0x4000;
    }

    this->DecIgnoreFacing();

    if (--this->m_groundTiltRefs == 0) {
        this->ClampPitchToLimits(-this->m_groundTilt);
    }

    SysMsgAdd("Camera NORMAL", SYSMSG_INFO);
    this->m_state &= ~0x4000;
}

// VERIFY: 62256cd1 manual
// OFFSET: 0x00606570
int32_t CGCamera::SetModelCamera(const char* fileName, const C3Vector* position, float facing, void (*callback)(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param), uint64_t param) {
    if (this->m_model) {
        this->m_state &= ~0x4;

        if (this->m_modelCameraHandle) {
            HandleClose(this->m_modelCameraHandle);
            this->m_modelCameraHandle = nullptr;
        }

        this->m_model->Release();
        this->m_model = nullptr;
    }

    CStatus status;
    this->m_model = this->GetScene()->CreateModel(fileName, 0);
    SysMsgAdd(&status);

    if (!this->m_model) {
        return 0;
    }

    this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), param);
    this->m_modelMatrix.c2 = 1.0f;
    this->m_modelMatrix.b1 = 1.0f;
    this->m_modelMatrix.a0 = 1.0f;
    this->m_modelMatrix.d2 = 0.0f;
    this->m_modelMatrix.d1 = 0.0f;
    this->m_modelMatrix.d0 = 0.0f;
    this->m_modelMatrix.c1 = 0.0f;
    this->m_modelMatrix.c0 = 0.0f;
    this->m_modelMatrix.b2 = 0.0f;
    this->m_modelMatrix.b0 = 0.0f;
    this->m_modelMatrix.a2 = 0.0f;
    this->m_modelMatrix.a1 = 0.0f;
    this->m_modelMatrix.d0 = position->x;
    this->m_modelMatrix.d1 = position->y;
    this->m_modelMatrix.d2 = position->z;
    this->m_modelMatrix.RotateAroundZ(facing);
    CM2Model* model = this->m_model;
    model->m_fFlags |= 0x8000;
    model->m_worldTransform = C44Matrix(this->m_modelMatrix);

    if (!this->m_model->IsLoaded(1, 0)) {
        this->m_state &= ~0x4;
        return 1;
    }

    if (!this->FinishLoadingModel()) {
        return 0;
    }

    return 1;
}

// VERIFY: 13e871f6 manual
// OFFSET: 0x006066E0
void CGCamera::SetTarget(CGObject_C* target, int32_t freeLook) {
    if (this->m_state & 0x10) {
        CGWorldFrame::s_currentWorldFrame->SetPlayerFadeCameraValue(0xFF);
    }

    WOWGUID previous = this->m_targetGUID;
    int32_t newTarget = target && target->m_obj->m_guid != this->m_targetGUID;
    int32_t moved = 0;

    if (target) {
        C3Vector position = this->GetSafeWorldPos(target);
        moved = this->m_targetPosition.x != position.x || this->m_targetPosition.y != position.y || this->m_targetPosition.z != position.z;
    }

    int32_t immediate = !(this->m_state & 0x10) || newTarget;

    if (!(this->m_state & 0x10) || newTarget || moved) {
        uint32_t time = OsGetAsyncTimeMs();
        this->CalcTerrainTilt(target, time);
        this->PerformTerrainTilt(target, time, immediate);
    }

    float drunkness = 0.0f;

    if (target && (target->m_obj->m_type & TYPE_PLAYER)) {
        drunkness = static_cast<CGPlayer_C*>(target)->GetDrunknessTurnValue();
    }

    this->UpdateInebriation(drunkness, immediate);
    this->unused_01D0 = 0.0f;

    if (target) {
        this->m_targetGUID = target->m_obj->m_guid;

        if (target->m_obj->m_type & TYPE_UNIT) {
            this->m_targetFacing = target->GetRawFacing();
            WOWGUID relativeTo = WOWGUID{ static_cast<CGUnit_C*>(target)->GetCameraRelativeTo() };

            if (ClntObjMgrObjectPtr(relativeTo, TYPE_OBJECT, __FILE__, __LINE__)) {
                this->MakeRelativeTo(relativeTo);
            } else {
                this->MakeRelativeTo(WOWGUID());
            }
        } else {
            this->m_targetFacing = target->GetFacing();
            this->m_relativeTo = WOWGUID();
        }

        if (freeLook) {
            this->m_flags |= 0x2;
            this->SetModeFreeLook();
        } else if (this->m_flags & 0x2) {
            this->m_flags &= ~0x2;
            this->SetModeNormal();
        }

        this->m_guid0090 = WOWGUID();

        if (!this->FinishLoadingTarget(target)) {
            this->m_state &= ~0x4;
        }
    } else {
        this->m_targetGUID = WOWGUID();
        this->m_relativeTo = WOWGUID();
        this->m_cameraShakeList.Clear();
        this->m_flags &= ~0x2;
    }

    this->m_state &= ~0x800000;

    if (!previous) {
        this->PickVehicleCamera();
    }
}

// VERIFY: 465238a4 auto
// OFFSET: 0x005FE880
void CGCamera::SetupWorldProjection(const CRect& rect) {
    this->SetGxProjectionAndView(rect);
}

// VERIFY: 9dd566d0 manual
// OFFSET: 0x00601550
int32_t CGCamera::SmoothSetFlyingMountHeight(float height, float delay, float scale, uint32_t time) {
    if ((this->m_state & 0x400000) && fabsf(this->m_smoothFlyingHeight.m_target - height) < 0.001f && fabsf(this->m_smoothFlyingHeight.m_param4 - delay) < 0.001f && fabsf(this->m_smoothFlyingHeight.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_flyingMountHeight - height) < 0.001f) {
        return 0;
    }

    this->m_smoothFlyingHeight.m_param4 = delay;
    this->m_smoothFlyingHeight.m_param3 = scale;

    float duration = fabsf(height - this->m_flyingMountHeight) / s_cvCameraFlyingMountHeightSmoothSpeed->m_floatValue * scale;

    return this->StartSmoothFlyingMountHeight(height, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: d9ad0d06 manual
// OFFSET: 0x00603230
int32_t CGCamera::SmoothSetHeight(float height, float delay, float scale, uint32_t time) {
    if ((this->m_state & 0x20000000) && fabsf(this->m_smoothHeight.m_target - height) < 0.001f && fabsf(this->m_smoothHeight.m_param4 - delay) < 0.001f && fabsf(this->m_smoothHeight.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_height - height) < 0.001f) {
        return 0;
    }

    this->m_smoothHeight.m_param4 = delay;
    this->m_smoothHeight.m_param3 = scale;

    float duration = fabsf(height - this->m_height) / s_cvCameraHeightSmoothSpeed->m_floatValue * scale;

    return this->StartSmoothHeight(height, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: 28d96c09 auto
// OFFSET: 0x005FF5A0
float CGCamera::SmoothTargetFacing(float facing, int32_t moved) {
    if (!s_cameraSmooth->m_intValue || !moved) {
        return facing;
    }

    float delta = facing - this->m_targetFacing;

    if (delta > PI) {
        delta -= TWO_PI;
    } else if (delta < -PI) {
        delta += TWO_PI;
    }

    float magnitude = fabsf(delta);
    float step;

    if (magnitude > MAX_DELTA) {
        step = delta < 0.0f ? -MAX_DELTA : MAX_DELTA;
    } else if (magnitude > ROTATION_SPEED) {
        step = delta < 0.0f ? -ROTATION_SPEED : ROTATION_SPEED;
    } else {
        return facing;
    }

    return WrapAngle0To2Pi(step + this->m_targetFacing);
}

// VERIFY: d70414b5 auto
// OFFSET: 0x005FEBF0
int32_t CGCamera::StartSmoothFlyingMountHeight(float height, float rate, uint32_t startTime) {
    if (fabsf(this->m_flyingMountHeight - height) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x400000;
    this->m_smoothFlyingHeight.m_startValue = this->m_flyingMountHeight;
    this->m_smoothFlyingHeight.m_startTimeMs = startTime;
    this->m_smoothFlyingHeight.m_target = height;
    this->m_smoothFlyingHeight.m_rate = rate;

    return 1;
}

// VERIFY: 18e5c001 auto
// OFFSET: 0x00600F00
int32_t CGCamera::StartSmoothFoV(float offset, float rate, uint32_t startTime) {
    this->m_targetFov = UnwrapAngleToward(offset, this->m_targetFov, -PI, PI);

    if (fabsf(this->m_targetFov - offset) < 0.001f && !(this->m_state & 0x80008000)) {
        return 0;
    }

    this->m_state |= 0x40000000;
    this->m_smoothFoV.m_startValue = this->m_targetFov;
    this->m_smoothFoV.m_startTimeMs = startTime;
    this->m_smoothFoV.m_target = offset;
    this->m_smoothFoV.m_rate = rate;

    return 1;
}

// VERIFY: 443312a1 auto
// OFFSET: 0x005FE9B0
int32_t CGCamera::StartSmoothGroundTilt(float offset, float rate, uint32_t startTime) {
    this->m_groundTilt = UnwrapAngleToward(offset, this->m_groundTilt, -PI, PI);

    if (fabsf(this->m_groundTilt - offset) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x10000000;
    this->m_smoothGroundTilt.m_startValue = this->m_groundTilt;
    this->m_smoothGroundTilt.m_startTimeMs = startTime;
    this->m_smoothGroundTilt.m_target = offset;
    this->m_smoothGroundTilt.m_rate = rate;

    return 1;
}

// VERIFY: b02bcfd3 auto
// OFFSET: 0x006010E0
int32_t CGCamera::StartSmoothHeight(float height, float rate, uint32_t startTime) {
    if (fabsf(this->m_height - height) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x20000000;
    this->m_smoothHeight.m_startValue = this->m_height;
    this->m_smoothHeight.m_startTimeMs = startTime;
    this->m_smoothHeight.m_target = height;
    this->m_smoothHeight.m_rate = rate;

    if (!(this->m_state & 0x80)) {
        this->m_height = height;
        this->m_smoothHeight.m_target = height;
        this->m_state = (this->m_state & ~0x20000000) | 0x80;
        this->m_smoothHeight.m_rate = 0.0f;
        this->m_smoothHeight.m_startTimeMs = 0;
    }

    return 1;
}

// VERIFY: e32ed6b7 auto
// OFFSET: 0x00604940
int32_t CGCamera::UpdateInebriation(float drunkness, int32_t immediate) {
    if (this->m_state & 0x80008000) {
        return 0;
    }

    float offset = drunkness * 1.5533429f;

    if (immediate) {
        bool changed = fnotequal(this->m_targetFov, offset);
        this->m_targetFov = offset;
        return changed;
    }

    return this->SetDesiredFoVOffset(offset, 0.0f, 1.0f, OsGetAsyncTimeMs());
}

// VERIFY: 6c4cbefa manual
// OFFSET: 0x006049C0
float CGCamera::UpdateLiquidSurfaceStatus(CGObject_C* target) {
    this->m_state &= ~0x300000;

    int32_t liquidType;
    float height;
    int32_t liquidFlags;

    if (!target || !World::QueryObjectLiquid(target->m_worldObject, &liquidType, &height, &liquidFlags)) {
        return 0.0f;
    }

    float depth = height - this->GetSafeWorldPos(target).z;
    float targetHeight;

    if (target->m_obj->m_type & TYPE_UNIT) {
        targetHeight = static_cast<CGUnit_C*>(target)->m_movementData.m_collisionHeight;
    } else {
        targetHeight = target->m_height * target->m_scale;
    }

    if (targetHeight - 0.22222222f < depth) {
        this->m_state |= 0x200000;
    } else {
        this->m_state |= 0x100000;
    }

    return depth;
}

// VERIFY: 3ef07327 manual
// OFFSET: 0x00604490
void CGCamera::UpdateTargetFacing(CGObject_C* target, float& yaw, float& pitch, float& roll) {
    yaw = this->m_yaw;
    pitch = this->m_pitch;
    roll = this->m_roll;

    if (!target) {
        return;
    }

    C3Vector position = this->GetSafeWorldPos(target);
    bool moved = this->m_targetPosition.x != position.x || this->m_targetPosition.y != position.y || this->m_targetPosition.z != position.z;
    float facing;

    if (target->m_obj->m_type & TYPE_UNIT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(target);
        unit->UpdateSmoothFacing(nullptr);
        facing = this->GetChaseFacing(unit);

        if (unit->m_obj->m_guid != CGUnit_C::s_activeMover && !this->m_vehicleCamera) {
            facing = this->SmoothTargetFacing(facing, moved);
        }
    } else {
        facing = target->GetFacing();
    }

    this->m_targetFacing = facing;
    yaw = this->m_yawOffset + yaw;

    if (this->m_ignoreFacingRefs <= 0) {
        yaw = facing + yaw;
    }

    if (this->m_groundTiltRefs <= 0) {
        pitch = this->m_groundTilt + pitch;
    }

    yaw = CMath::normalizeangle0to2pi_(yaw);
    pitch = MIN_PITCH_ANGLE > pitch ? MIN_PITCH_ANGLE : (MAX_PITCH_ANGLE <= pitch ? MAX_PITCH_ANGLE : pitch);

    this->SetFacing(yaw, pitch, roll);
}

// VERIFY: 9718580e manual
// OFFSET: 0x00604640
void CGCamera::UpdateTargetHeight(CGObject_C* target, uint32_t time) {
    float scale = 1.0f;

    if (static_cast<int32_t>(this->m_heightChangedTimeMs) > 0) {
        if (OsGetAsyncTimeMs() - this->m_heightChangedTimeMs > 2999) {
            this->m_heightChangedTimeMs = 0;
        } else {
            scale = 0.5f;
        }
    }

    if (target && (target->m_obj->m_type & TYPE_UNIT) && (static_cast<CGUnit_C*>(target)->m_movement->m_flags & MOVEMENTFLAG_SWIMMING)) {
        this->SmoothSetHeight(this->m_targetHeightSwim + s_targetHeightOffset, 0.0f, scale, time);
    } else if (this->m_smoothDistance.m_target >= 1.8315002f || CGBarberShop::m_barberShopEnabled) {
        this->SmoothSetHeight(this->m_targetHeightFar + s_targetHeightOffset, 0.0f, scale, time);

        if (target && (target->m_obj->m_type & TYPE_UNIT) && static_cast<CGUnit_C*>(target)->GetCanFly()) {
            this->SmoothSetFlyingMountHeight(this->m_mountHeight + s_targetHeightOffset, 0.0f, scale, time);
        } else {
            this->SmoothSetFlyingMountHeight(s_targetHeightOffset, 0.0f, scale, time);
        }
    } else {
        this->SmoothSetHeight(this->m_targetHeightNear + s_targetHeightOffset, 0.0f, scale, time);
        this->SmoothSetFlyingMountHeight(s_targetHeightOffset, 0.0f, 1.0f, time);
    }
}

// VERIFY: edc6c43f auto
// OFFSET: 0x005FE5F0
void CGCamera::UpdateYaw(float delta) {
    this->m_smoothYaw.m_target = CMath::normalizeangle0to2pi_(delta + this->m_smoothYaw.m_target);
    this->m_smoothYaw.m_startValue = UnwrapAngleToward(this->m_smoothYaw.m_target, CMath::normalizeangle0to2pi_(delta + this->m_smoothYaw.m_startValue), -PI, PI);
    this->m_yaw = UnwrapAngleToward(this->m_smoothYaw.m_target, CMath::normalizeangle0to2pi_(this->m_yaw + delta), -PI, PI);
}

// VERIFY: 3e57388a auto
// OFFSET: 0x005FE400
int32_t CGCamera::CCommand_PitchLimit(const char* command, const char* arguments) {
    MAX_PITCH_ANGLE = SStrToFloat(arguments) * 0.017453292f;
    MIN_PITCH_ANGLE = SStrToFloat(arguments) * -0.017453292f;
    return 1;
}

// VERIFY: 4129fc26 auto
// OFFSET: 0x005FEC50
uint32_t CGCamera::GetCollisionQueryFlags() {
    return s_cameraWaterCollision->m_intValue ? 0x120171 : 0x100171;
}

// VERIFY: cf6cb77a auto
// OFFSET: 0x005FE570
float CGCamera::GetDefaultZoomMaxDistance() {
    return 50.0f;
}

// VERIFY: cfc63e8a manual
// OFFSET: 0x00606330
void CGCamera::AddShake(const C3Vector& position, int32_t type, CGCameraDir dir, float amplitude, float frequency, float duration, float timeOffset, float damping) {
    CameraShake* shake = this->m_cameraShakeList.NewNode(2, 0, 0);

    if (shake) {
        shake->m_amplitude = amplitude;
        shake->m_type = static_cast<CGCameraShakeType>(type);
        shake->m_frequency = frequency;
        shake->m_dir = dir;
        shake->m_duration = duration;
        shake->m_timeOffset = timeOffset;
        shake->m_damping = damping;
        shake->m_startTime = OsGetAsyncTimeMs();
        shake->m_position = position;
    }
}

// VERIFY: 11191f2c manual
// OFFSET: 0x00600030
void CGCamera::BeginBobbingSmooth() {
    this->m_state &= ~0x200;
    this->m_bobDuration = fabsf((&this->m_bobOffset.x)[this->m_bobOffset.MajorAxis()]) / s_cvCameraBobbingSmoothSpeed->m_floatValue;
    this->m_bobStart = this->m_bobOffset;
    this->m_bobTime = OsGetAsyncTimeMs();
}

// VERIFY: 14bc36d9 manual
// OFFSET: 0x00600530
void CGCamera::CameraTeleported(float facing) {
    if (!(this->m_state & 0x1)) {
        return;
    }

    float current = 0.0f;
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        current = target->GetFacing();
    }

    this->UpdateYaw(facing - current);
}

// VERIFY: fd583d08 manual
// OFFSET: 0x005FFDE0
uint32_t CGCamera::CanSmoothTargetFacing(CGObject_C* target) {
    if (!target || !(target->m_obj->m_type & TYPE_UNIT) || !s_cvCameraPivot->m_intValue) {
        return 0;
    }

    uint32_t moveFlags = static_cast<CGUnit_C*>(target)->m_movement->m_flags;

    if (moveFlags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
        return 0;
    }

    if (this->m_pitch > 0.0f) {
        return 0;
    }

    if (this->m_state & 0x8) {
        return 0;
    }

    if (moveFlags & MOVEMENTFLAG_FLYING) {
        return this->m_state & 0x10000;
    }

    return this->m_state & 0x30000;
}

// VERIFY: 1d04ff0c auto
// OFFSET: 0x005FEEE0
void CGCamera::CancelSmoothPitch() {
    this->m_state &= ~0x2000000;
    this->m_smoothPitch.m_target = this->m_pitch;
    this->m_smoothPitch.m_startTimeMs = 0;
    this->m_smoothPitch.m_rate = 0.0f;
}

// VERIFY: 25517997 auto
// OFFSET: 0x005FEF10
void CGCamera::CancelSmoothTargetOffset() {
    this->m_state &= ~0x8000000;
    this->m_smoothTargetOffset.m_target = this->m_targetOffset;
    this->m_smoothTargetOffset.m_startTimeMs = 0;
    this->m_smoothTargetOffset.m_rate = 0.0f;
}

// VERIFY: 8a18d3c9 auto
// OFFSET: 0x005FEF40
void CGCamera::CancelSmoothYaw() {
    this->m_state &= ~0x1000000;
    this->m_smoothYaw.m_target = this->m_yaw;
    this->m_smoothYaw.m_startTimeMs = 0;
    this->m_smoothYaw.m_rate = 0.0f;
}

// VERIFY: fb086d48 auto
// OFFSET: 0x005FE7B0
void CGCamera::CheckUnderwater() {
    float depth;
    int32_t underwater = World::SceneCamLiquidStatus(&depth);
    SI2::SetUnderwaterStatus(underwater);

    if (!(this->m_state & 0x2) || this->m_underwater != underwater) {
        this->m_state |= 0x2;
        this->m_underwater = underwater;
    }
}

// VERIFY: 7eba4da4 auto
// OFFSET: 0x005FF530
void CGCamera::ClampPitchAndNormalize() {
    this->m_pitch = MIN_PITCH_ANGLE > this->m_pitch ? MIN_PITCH_ANGLE : (MAX_PITCH_ANGLE <= this->m_pitch ? MAX_PITCH_ANGLE : this->m_pitch);
    this->m_yaw = CMath::normalizeangle0to2pi_(this->m_yaw);
    this->m_yawOffset = CMath::normalizeangle0to2pi_(this->m_yawOffset);
}

// VERIFY: fcc0db6a auto
// OFFSET: 0x005FE360
void CGCamera::ClearModelCamera() {
    if (!this->m_model) {
        return;
    }

    this->m_state &= ~0x4;

    if (this->m_modelCameraHandle) {
        HandleClose(this->m_modelCameraHandle);
        this->m_modelCameraHandle = nullptr;
    }

    this->m_model->Release();
    this->m_model = nullptr;
}

// VERIFY: 07a00ca2 auto
// OFFSET: 0x005FE940
void CGCamera::ClearVehicleExitZoomDistance() {
    this->m_flags &= ~0x20;
}

// VERIFY: cbb0bca4 auto
// OFFSET: 0x005FE4E0
void CGCamera::CreateViewFromCamera(uint32_t view) {
    this->SaveViewToCVars(view, this->m_smoothDistance.m_target, this->m_smoothPitch.m_target, this->m_smoothYaw.m_target);
}

// VERIFY: 6166086b auto
// OFFSET: 0x00604850
void CGCamera::DisableFreeLook(int32_t keepSmoothing) {
    this->SetModeNormal();

    if (keepSmoothing) {
        this->m_state |= 0x20;
    } else {
        this->m_state &= ~0x20;
    }

    this->m_smoothPitch.m_target = this->m_pitch;
    this->m_smoothYaw.m_target = this->m_yaw;
    this->m_smoothTargetOffset.m_target = this->m_targetOffset;
}

// VERIFY: 18ff5acf manual
// OFFSET: 0x006047E0
void CGCamera::EnableFreeLook() {
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target && (target->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(target)->m_unit->m_unitFieldFlags & 0x100000) && (static_cast<CGPlayer_C*>(target)->m_player->m_playerFlags & 0x20000)) {
        return;
    }

    this->SetModeFreeLook();
}

// VERIFY: 83cdf005 manual
// OFFSET: 0x00601D60
C3Vector CGCamera::GetCameraPosition(const C3Vector& target, float distance, const C3Vector& offset) {
    C3Vector position = target;
    C3Vector forward = this->Forward();
    this->RotateOffsetByRoll(forward);

    position.x -= forward.x * distance;
    position.y -= forward.y * distance;
    position.z -= forward.z * distance;

    if (distance > 0.0f && fabsf(this->m_flyingMountHeight) >= 2.3841858e-7f) {
        float scale = this->m_collideExtent * this->m_flyingMountHeight * distance;
        float maxDistance = this->m_distance >= this->m_smoothDistance.m_target ? this->m_distance : this->m_smoothDistance.m_target;

        if (maxDistance > 0.0f) {
            scale /= maxDistance;
            C3Vector up = this->Up();
            position.x = up.x * scale + position.x;
            position.y = up.y * scale + position.y;
            position.z = up.z * scale + position.z;
        }
    }

    position.x = position.x + offset.x;
    position.y = offset.y + position.y;
    position.z = offset.z + position.z;

    return position;
}

// VERIFY: 94f5ae4c auto
// OFFSET: 0x005FE310
int32_t CGCamera::HasModelCamera() {
    return this->m_modelCameraHandle != nullptr;
}

// VERIFY: 0bbc9b0f manual
// OFFSET: 0x005FFE50
int32_t CGCamera::HasTargetOffset() {
    if (fabsf(this->m_bobOffset.z + this->m_bobOffset.y + this->m_bobOffset.x) < 0.001f) {
        return 0;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);
    return this->IsBobbingAllowed(target) == 0;
}

// VERIFY: 2f9adb7e manual
// OFFSET: 0x005FFD20
uint32_t CGCamera::IsBobbingAllowed(CGObject_C* target) {
    if (this->m_distance > 0.16666667f) {
        return 0;
    }

    if (!target || !(target->m_obj->m_type & TYPE_UNIT)) {
        return 0;
    }

    uint32_t state = this->m_state;

    if ((state & 0x8) || !s_cvCameraBobbing->m_intValue) {
        return 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(target);
    uint32_t moveFlags = unit->m_movement->m_flags;

    if ((moveFlags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING)) || (moveFlags & MOVEMENTFLAG_FALLING)) {
        return 0;
    }

    if (unit->IsJumping() || unit->m_mountModel || (unit->m_unit->m_unitFieldFlags & 0x100000)) {
        return 0;
    }

    return state & 0x200;
}

// VERIFY: a444f5b9 manual
// OFFSET: 0x005FFF40
int32_t CGCamera::IsCustomViewSmoothingActive() {
    return !(this->m_state & 0x20) && (s_cvCameraCustomViewSmoothing->m_intValue || this->IsViewDefault(this->m_viewIndex)) && !(this->m_state & 0x8000);
}

// VERIFY: 5125ed06 auto
// OFFSET: 0x005FE3A0
int32_t CGCamera::IsFirstPersonView() {
    return this->m_distance < 1.8315002f;
}

// VERIFY: 00e1843f manual
// OFFSET: 0x005FF8E0
int32_t CGCamera::IsViewDefault(uint32_t view) {
    for (uint32_t i = 0; i < 3; i++) {
        float value = SStrToFloat(s_cameraViewDefaults[view].m_values[i]);

        if (s_cameraViewAngular[i]) {
            value *= 0.017453292f;
        }

        if (!(fabsf(this->m_views[view].Component(i) - value) < 0.001f)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 91dbc19d auto
// OFFSET: 0x005FF3E0
void CGCamera::LoadCameraSettings() {
    float distance = s_cvCameraSavedDistance->m_floatValue;
    distance = 0.0f > distance ? 0.0f : (50.0f <= distance ? 50.0f : distance);
    this->m_distance = distance;
    this->m_smoothDistance.m_target = distance;

    float pitch = s_cvCameraSavedPitch->m_floatValue * 0.017453292f;
    this->m_pitch = pitch;
    this->m_smoothPitch.m_target = pitch;
}

// VERIFY: 76663636 auto
// OFFSET: 0x00604C70
void CGCamera::OnVehicleCameraDestroyed() {
    this->m_vehicleCamera = nullptr;
    this->PickVehicleCamera();
}

// VERIFY: a21be849 auto
// OFFSET: 0x005FE320
void CGCamera::ResetModelCamera() {
    this->m_someTime = OsGetAsyncTimeMs();
    this->GetScene()->SetTime(0);
    this->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 0, 1);
}

// VERIFY: 24591d4d auto
// OFFSET: 0x00600730
void CGCamera::RotateOffsetByRoll(C3Vector& offset) {
    if (0.0f != this->m_vehicleYawOffset) {
        C44Matrix rotation;
        rotation.RotateAroundZ(this->m_vehicleYawOffset);
        offset *= rotation;
    }
}

// VERIFY: 0adeaf5d auto
// OFFSET: 0x005FF320
void CGCamera::SaveCameraSettings() {
    char buffer[32];
    float distance;

    if (this->m_state & 0x8) {
        distance = this->m_preTelescopeDistance;
    } else if (this->m_flags & 0x20) {
        distance = this->m_vehicleExitZoomDistance;
    } else {
        distance = this->m_smoothDistance.m_target;
    }

    SStrPrintf(buffer, sizeof(buffer), "%f", distance);

    if (this->m_vehicleZoomEnabled) {
        s_cvCameraSavedVehicleDistance->Set(buffer, true, false, false, true);
    } else {
        s_cvCameraSavedDistance->Set(buffer, true, false, false, true);
    }

    SStrPrintf(buffer, sizeof(buffer), "%f", this->m_smoothPitch.m_target * 57.29578f);
    s_cvCameraSavedPitch->Set(buffer, true, false, false, true);
}

// VERIFY: 1d053571 manual
// OFFSET: 0x005FE440
void CGCamera::SaveViewToCVars(uint32_t view, float distance, float pitch, float yaw) {
    char buffer[40];

    this->m_views[view].m_distance = distance;
    this->m_views[view].m_pitch = pitch;
    this->m_views[view].m_yaw = yaw;

    for (uint32_t i = 0; i < 3; i++) {
        float value = this->m_views[view].Component(i);

        if (s_cameraViewAngular[i]) {
            value *= 57.29578f;
        }

        SStrPrintf(buffer, sizeof(buffer), "%f", value);
        s_cvCameraViewData[view][i]->Set(buffer, true, false, false, true);
    }
}

// VERIFY: 68a3e1df manual
// OFFSET: 0x00601190
int32_t CGCamera::SetDesiredPitchAngle(float pitch, float delay, float scale, uint32_t time) {
    this->m_pitch = UnwrapAngleToward(pitch, this->m_pitch, -PI, PI);

    if ((this->m_state & 0x2000000) && fabsf(this->m_smoothPitch.m_target - pitch) < 0.001f && fabsf(this->m_smoothPitch.m_param4 - delay) < 0.001f && fabsf(this->m_smoothPitch.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_pitch - pitch) < 0.001f) {
        return 0;
    }

    this->m_smoothPitch.m_param4 = delay;
    this->m_smoothPitch.m_param3 = scale;

    float duration = scale * (fabsf(pitch - this->m_pitch) / (s_cvCameraPitchSmoothSpeed->m_floatValue * 0.017453292f));

    return this->StartSmoothPitch(pitch, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: cf7fb294 manual
// OFFSET: 0x006012D0
int32_t CGCamera::SetDesiredTargetOffset(float offset, float delay, float scale, uint32_t time) {
    this->m_targetOffset = UnwrapAngleToward(offset, this->m_targetOffset, -PI, PI);

    if ((this->m_state & 0x8000000) && fabsf(this->m_smoothTargetOffset.m_target - offset) < 0.001f && fabsf(this->m_smoothTargetOffset.m_param4 - delay) < 0.001f && fabsf(this->m_smoothTargetOffset.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_targetOffset - offset) < 0.001f) {
        return 0;
    }

    this->m_smoothTargetOffset.m_param4 = delay;
    this->m_smoothTargetOffset.m_param3 = scale;

    float duration = scale * (fabsf(offset - this->m_targetOffset) / (s_cvCameraTargetSmoothSpeed->m_floatValue * 0.017453292f));

    return this->StartSmoothTargetOffset(offset, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: 6959f8ee auto
// OFFSET: 0x005FE8D0
void CGCamera::SetDesiredVehiclePitchOffset(float offset, uint32_t startTime, uint32_t endTime) {
    this->m_vehiclePitchStartTime = startTime;
    this->m_vehiclePitchEndTime = endTime;

    if (startTime != endTime) {
        this->m_vehiclePitchFrom = this->m_vehiclePitchOffset;
        this->m_vehiclePitchTo = offset;
    } else {
        this->m_vehiclePitchOffset = offset;
    }
}

// VERIFY: 1da15eb3 auto
// OFFSET: 0x005FE890
void CGCamera::SetDesiredVehicleYawOffset(float offset, uint32_t startTime, uint32_t endTime) {
    this->m_vehicleYawStartTime = startTime;
    this->m_vehicleYawEndTime = endTime;

    if (startTime != endTime) {
        this->m_vehicleYawFrom = this->m_vehicleYawOffset;
        this->m_vehicleYawTo = offset;
    } else {
        this->m_vehicleYawOffset = offset;
    }
}

// VERIFY: 3c5a1872 manual
// OFFSET: 0x00601410
int32_t CGCamera::SetDesiredYawAngle(float yaw, float delay, float scale, uint32_t time) {
    this->m_yaw = UnwrapAngleToward(yaw, this->m_yaw, -PI, PI);

    if ((this->m_state & 0x1000000) && fabsf(this->m_smoothYaw.m_target - yaw) < 0.001f && fabsf(this->m_smoothYaw.m_param4 - delay) < 0.001f && fabsf(this->m_smoothYaw.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_yaw - yaw) < 0.001f) {
        return 0;
    }

    this->m_smoothYaw.m_param4 = delay;
    this->m_smoothYaw.m_param3 = scale;

    float duration = scale * (fabsf(yaw - this->m_yaw) / (s_cvCameraYawSmoothSpeed->m_floatValue * 0.017453292f));

    return this->StartSmoothYaw(yaw, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: 1cec2544 auto
// OFFSET: 0x005FE920
void CGCamera::SetVehicleExitZoomDistance(float distance) {
    this->m_flags |= 0x20;
    this->m_vehicleExitZoomDistance = distance;
}

// VERIFY: ac42541a auto
// OFFSET: 0x00600890
void CGCamera::SetVehicleSeatZoomMaxDistance(float distance) {
    this->m_flags |= 0x10;
    this->m_overrideDistanceMax = distance;

    if (distance < this->m_smoothDistance.m_target) {
        this->m_state &= ~0x4000000;
        this->m_smoothDistance.m_target = this->m_distance;
        this->m_smoothDistance.m_startTimeMs = 0;
        this->m_smoothDistance.m_rate = 0.0f;
    }
}

// VERIFY: 08d40f6a auto
// OFFSET: 0x00600920
void CGCamera::SetVehicleSeatZoomMaxDistanceToDefault() {
    this->m_flags &= ~0x10;

    if (this->m_smoothDistance.m_target > 50.0f) {
        this->m_state &= ~0x4000000;
        this->m_smoothDistance.m_target = this->m_distance;
        this->m_smoothDistance.m_startTimeMs = 0;
        this->m_smoothDistance.m_rate = 0.0f;
    }
}

// VERIFY: 88d024de auto
// OFFSET: 0x00600840
void CGCamera::SetVehicleSeatZoomMinDistance(float distance) {
    this->m_flags |= 0x8;
    this->m_overrideDistanceMin = distance;

    if (distance > this->m_smoothDistance.m_target) {
        this->m_state &= ~0x4000000;
        this->m_smoothDistance.m_target = this->m_distance;
        this->m_smoothDistance.m_startTimeMs = 0;
        this->m_smoothDistance.m_rate = 0.0f;
    }
}

// VERIFY: 399ce7db auto
// OFFSET: 0x006008E0
void CGCamera::SetVehicleSeatZoomMinDistanceToDefault() {
    this->m_flags &= ~0x8;

    if (this->m_smoothDistance.m_target < 0.0f) {
        this->m_state &= ~0x4000000;
        this->m_smoothDistance.m_target = this->m_distance;
        this->m_smoothDistance.m_startTimeMs = 0;
        this->m_smoothDistance.m_rate = 0.0f;
    }
}

// VERIFY: 300a157b auto
// OFFSET: 0x005FE910
void CGCamera::SetVehicleTrackingStyle(uint32_t style) {
    this->m_vehicleTrackingStyle = style;
}

// VERIFY: 2dba7f2c auto
// OFFSET: 0x00600590
void CGCamera::SetVehicleZoomEnabled(int32_t enabled, float duration, int32_t animate) {
    if (this->m_vehicleZoomEnabled == enabled) {
        return;
    }

    if (duration < 0.5f) {
        duration = 0.5f;
    }

    this->SaveCameraSettings();

    float distance;

    if (enabled) {
        distance = s_cvCameraSavedVehicleDistance->m_floatValue;

        if (distance < 0.0f) {
            distance = 50.0f;
        }
    } else {
        distance = s_cvCameraSavedDistance->m_floatValue;
        distance = distance < 0.0f ? 0.0f : (distance >= 50.0f ? 50.0f : distance);
    }

    if (animate && fabsf(distance - this->m_distance) >= 0.001f) {
        if (distance <= this->m_distance) {
            this->ZoomIn(this->m_distance - distance, OsGetAsyncTimeMs(), duration);
        } else {
            this->ZoomOut(distance - this->m_distance, OsGetAsyncTimeMs(), duration);
        }
    }

    this->m_vehicleZoomEnabled = enabled;
}

// VERIFY: ebb194ac manual
// OFFSET: 0x005FFEB0
int32_t CGCamera::ShouldSmoothPitch(float minPitch, float maxPitch) {
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__);

    return target && !(this->m_state & 0x1) && s_cvCameraSmoothPitch->m_intValue && !(static_cast<CGUnit_C*>(target)->m_movement->m_flags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING)) && (minPitch > this->m_pitch || maxPitch < this->m_pitch);
}

// VERIFY: d2778348 manual
// OFFSET: 0x00600E00
int32_t CGCamera::SmoothSetDistance(float distance, float delay, float scale, uint32_t time) {
    if ((this->m_state & 0x4000000) && fabsf(this->m_smoothDistance.m_target - distance) < 0.001f && fabsf(this->m_smoothDistance.m_param4 - delay) < 0.001f && fabsf(this->m_smoothDistance.m_param3 - scale) < 0.001f) {
        return 1;
    }

    if (fabsf(this->m_distance - distance) < 0.001f) {
        return 0;
    }

    this->m_smoothDistance.m_param4 = delay;
    this->m_smoothDistance.m_param3 = scale;

    float duration = fabsf(distance - this->m_distance) / s_cvCameraDistanceSmoothSpeed->m_floatValue * scale;

    return this->StartSmoothDistance(distance, duration, time - static_cast<int32_t>(delay * -1000.0f));
}

// VERIFY: 6daea45a auto
// OFFSET: 0x005FE580
void CGCamera::StartMotion(CGCameraMotion motion, uint32_t time, uint32_t duration, float speed) {
    if (this->m_flags & 0x40) {
        return;
    }

    uint32_t bit = 1 << (2 * motion);

    if (!(this->m_motionFlags & bit)) {
        this->m_motionFlags |= bit;
        this->m_motionStartTime[motion] = time;
    }

    this->m_motionSpeed[motion] = speed;
    this->m_motionEndTime[motion] = duration ? duration + time : 0;
}

// VERIFY: f3b61fd8 auto
// OFFSET: 0x005FE950
int32_t CGCamera::StartSmoothDistance(float distance, float rate, uint32_t startTime) {
    if (fabsf(this->m_distance - distance) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x4000000;
    this->m_smoothDistance.m_rate = rate;
    this->m_smoothDistance.m_startTimeMs = startTime;
    this->m_smoothDistance.m_startValue = this->m_distance;
    this->m_smoothDistance.m_target = distance;

    return 1;
}

// VERIFY: d9c18e76 auto
// OFFSET: 0x005FEA40
int32_t CGCamera::StartSmoothPitch(float pitch, float rate, uint32_t startTime) {
    this->m_pitch = UnwrapAngleToward(pitch, this->m_pitch, -PI, PI);

    if (fabsf(this->m_pitch - pitch) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x2000000;
    this->m_smoothPitch.m_startValue = this->m_pitch;
    this->m_smoothPitch.m_startTimeMs = startTime;
    this->m_smoothPitch.m_target = pitch;
    this->m_smoothPitch.m_rate = rate;

    return 1;
}

// VERIFY: bc92f038 auto
// OFFSET: 0x005FEAD0
int32_t CGCamera::StartSmoothTargetOffset(float offset, float rate, uint32_t startTime) {
    this->m_targetOffset = UnwrapAngleToward(offset, this->m_targetOffset, -PI, PI);

    if (fabsf(this->m_targetOffset - offset) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x8000000;
    this->m_smoothTargetOffset.m_startValue = this->m_targetOffset;
    this->m_smoothTargetOffset.m_startTimeMs = startTime;
    this->m_smoothTargetOffset.m_target = offset;
    this->m_smoothTargetOffset.m_rate = rate;

    return 1;
}

// VERIFY: fed19b69 auto
// OFFSET: 0x005FEB60
int32_t CGCamera::StartSmoothYaw(float yaw, float rate, uint32_t startTime) {
    this->m_yaw = UnwrapAngleToward(yaw, this->m_yaw, -PI, PI);

    if (fabsf(this->m_yaw - yaw) < 0.001f) {
        return 0;
    }

    this->m_state |= 0x1000000;
    this->m_smoothYaw.m_startValue = this->m_yaw;
    this->m_smoothYaw.m_startTimeMs = startTime;
    this->m_smoothYaw.m_target = yaw;
    this->m_smoothYaw.m_rate = rate;

    return 1;
}

// VERIFY: 64b57dbf auto
// OFFSET: 0x005FE6A0
void CGCamera::UpdateAutoInteractState(int32_t enabled) {
    if (enabled) {
        this->m_flags |= 0x1;
    } else {
        this->m_flags &= ~0x1;
    }
}

// VERIFY: 8b6e732e auto
// OFFSET: 0x00600090
void CGCamera::UpdateBobbingState(int32_t moving) {
    if (!(this->m_state & 0x200) && moving) {
        this->m_state |= 0x200;
        this->m_bobTime = OsGetAsyncTimeMs();
    }

    if ((this->m_state & 0x200) && !moving) {
        this->BeginBobbingSmooth();
    }
}

// VERIFY: acb663e0 manual
// OFFSET: 0x00600A10
void CGCamera::UpdateMountHeightOrOffset(CGUnit_C* unit) {
    this->m_flags &= ~0x80;
    this->m_mountHeight = 0.0f;

    if (!unit->GetCanFly()) {
        return;
    }

    CM2Model* mount = unit->m_mountModel;

    if (!mount || !mount->IsLoaded(0, 0) || !unit->m_worldModel || !unit->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    if (mount->HasEvent(0x414D4324)) {
        this->m_heightChangedTimeMs = OsGetAsyncTimeMs();
        this->m_mountHeight = 0.0f;

        float height = mount->GetEventPosition(0x414D4324).z - mount->GetWorldPosition().z;
        float delta = fabsf(height - this->m_targetHeightFar);
        this->m_flags |= 0x80;

        if (delta > 0.05f) {
            this->m_targetHeightFar = height;
            this->m_targetHeightNear = height;
            this->m_targetHeightSwim = height;
        }
    } else if (!(this->m_state & 0x800000)) {
        if (mount->HasEvent(0x4D464324)) {
            this->m_mountHeight = mount->GetEventPivot(0x4D464324).z;
        }

        this->m_state |= 0x800000;
    }
}

// VERIFY: f7142bf7 manual
// OFFSET: 0x00600970
void CGCamera::UpdateVehicleTarget(uint32_t time) {
    if (!this->m_vehicleCamera) {
        return;
    }

    this->m_vehicleCamera->Update(time);

    if (!this->m_vehicleCamera) {
        return;
    }

    WOWGUID relativeTo = WOWGUID{ this->m_vehicleCamera->GetRelativeTo() };

    if (!relativeTo || ClntObjMgrObjectPtr(relativeTo, TYPE_OBJECT, __FILE__, __LINE__)) {
        this->m_relativeTo = relativeTo;
    }
}

// VERIFY: d360aee9 manual
// OFFSET: 0x005FF950
void CGCamera::ZoomIn(float distance, uint32_t time, float duration) {
    if (this->m_flags & 0x40) {
        return;
    }

    float speed = 1.0f;
    uint32_t motionTime;

    if (duration > 0.0f) {
        motionTime = static_cast<uint32_t>(duration * 1000.0f);
        speed = static_cast<float>(static_cast<uint32_t>(distance * 1000.0f)) / (1000.0f * (duration * s_cvCameraDistanceMoveSpeed->m_floatValue));
    } else {
        motionTime = static_cast<uint32_t>(distance / s_cvCameraDistanceMoveSpeed->m_floatValue * 1000.0f);
    }

    if (this->m_motionFlags & 0x4) {
        this->m_motionStopTime[CAMERA_MOVE_OUT] = time;
        this->m_motionFlags |= 0x8;
    }

    if (this->m_motionEndTime[CAMERA_MOVE_IN] && (this->m_motionFlags & 0x1)) {
        this->m_motionEndTime[CAMERA_MOVE_IN] += motionTime;
    } else {
        this->StartMotion(CAMERA_MOVE_IN, time, motionTime, speed);
    }
}

// VERIFY: b31ca820 manual
// OFFSET: 0x005FFA60
void CGCamera::ZoomOut(float distance, uint32_t time, float duration) {
    if (this->m_flags & 0x40) {
        return;
    }

    float speed = 1.0f;
    uint32_t motionTime;

    if (duration > 0.0f) {
        motionTime = static_cast<uint32_t>(duration * 1000.0f);
        speed = static_cast<float>(static_cast<uint32_t>(distance * 1000.0f)) / (1000.0f * (duration * s_cvCameraDistanceMoveSpeed->m_floatValue));
    } else {
        motionTime = static_cast<uint32_t>(distance / s_cvCameraDistanceMoveSpeed->m_floatValue * 1000.0f);
    }

    if (this->m_motionFlags & 0x1) {
        this->m_motionStopTime[CAMERA_MOVE_IN] = time;
        this->m_motionFlags |= 0x2;
    }

    if (this->m_motionEndTime[CAMERA_MOVE_OUT] && (this->m_motionFlags & 0x4)) {
        this->m_motionEndTime[CAMERA_MOVE_OUT] += motionTime;
    } else {
        this->StartMotion(CAMERA_MOVE_OUT, time, motionTime, speed);
    }
}

// VERIFY: bf3dc605 auto
// OFFSET: 0x005FFB70
void CGCamera::ZoomToDistance(float distance, uint32_t time, float duration, int32_t lock) {
    if (this->m_flags & 0x40) {
        return;
    }

    if (fabsf(distance - this->m_distance) < 0.001f) {
        return;
    }

    if (this->m_motionFlags & 0x1) {
        this->m_motionStopTime[CAMERA_MOVE_IN] = time;
        this->m_motionFlags &= ~0x3;
    }

    if (this->m_motionFlags & 0x4) {
        this->m_motionStopTime[CAMERA_MOVE_OUT] = time;
        this->m_motionFlags &= ~0xC;
    }

    if (distance <= this->m_distance) {
        this->ZoomIn(this->m_distance - distance, time, duration);
    } else {
        this->ZoomOut(distance - this->m_distance, time, duration);
    }

    if (lock) {
        this->m_flags |= 0x40;
    }
}

// VERIFY: ac24e033 manual
// OFFSET: 0x00606F90
void CGCamera::CalcTargetCamera(CGObject_C* target, uint32_t time) {
    if (!target) {
        SErrSetLastError(87);
        return;
    }

    if (!(this->m_state & 0x4) && !this->FinishLoadingTarget(target)) {
        return;
    }

    this->UpdateVehicleTarget(FrameTime::s_curTimeMs);

    float unitPitch = 0.0f;
    CGUnit_C* unit = nullptr;
    uint32_t flying = 0;
    int32_t autoTracking = 0;
    int32_t onFlyingSpline = 0;
    uint32_t swimming = 0;
    int32_t uncontrolled = 0;

    if (target->m_obj->m_type & TYPE_UNIT) {
        unit = static_cast<CGUnit_C*>(target);
        flying = (unit->m_unit->m_unitFieldFlags >> 20) & 0x1;
        autoTracking = unit->IsAutoTracking();

        if (unit->IsOnFlyingSpline() || (unit->m_movement->m_flags & 0x2000000)) {
            onFlyingSpline = 1;
        }

        swimming = unit->m_movement->m_flags & 0x200000;

        CMoveSpline* spline = unit->m_movement->m_spline;
        int32_t splineControlled = spline && !(spline->m_flags & 0x400) ? 1 : 0;

        unitPitch = unit->GetPitch();
        this->UpdateMountHeightOrOffset(unit);
        uncontrolled = splineControlled && !flying ? 1 : 0;
    }

    this->UpdateUncontrolledState(uncontrolled);

    C3Vector safePos = this->GetSafeWorldPos(target);
    float flightLower = -500.0f;
    World::GetFlightBoundsLower(safePos, &flightLower);

    // STATIC: 0x00C24EA8
    static uint8_t s_aboveFlightBounds;

    if (flightLower <= safePos.z) {
        s_aboveFlightBounds = 1;
    } else {
        safePos = C3Vector(this->m_targetPosition.x, this->m_targetPosition.y, flightLower);

        if (s_aboveFlightBounds) {
            SI2::PlaySoundKit("SpaceDeathUniversal", nullptr, nullptr, nullptr);
        }

        s_aboveFlightBounds = 0;
    }

    if (flying && !this->m_wasFlying) {
        this->SetDesiredYawAngle(0.0f, 0.0f, 1.0f, OsGetAsyncTimeMs());

        if (unit && (unit->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(unit)->m_player->m_playerFlags & 0x20000)) {
            this->SetDesiredPitchAngle(0.0f, 0.0f, 1.0f, OsGetAsyncTimeMs());
        }
    }

    if (this->m_motionFlags) {
        this->UpdateMotion(time);
    }

    this->UpdateTargetSmoothing(target, time);

    float facingYaw;
    float facingDistance;
    float facingRoll;
    this->UpdateTargetFacing(target, facingYaw, facingDistance, facingRoll);

    C3Vector offset = C3Vector(0.0f, 0.0f, 0.0f);

    if (!onFlyingSpline && !swimming) {
        C3Vector base = C3Vector(safePos.x, safePos.y, this->m_height + safePos.z);
        C3Vector forward = this->Forward();
        C3Vector position = C3Vector(base.x - forward.x * this->m_distance, base.y - forward.y * this->m_distance, base.z - forward.z * this->m_distance);
        this->RunShakes(position, offset);
    }

    this->RunBobs(this->m_bobOffset);
    offset.x = offset.x + this->m_bobOffset.x;
    offset.y = this->m_bobOffset.y + offset.y;
    offset.z = this->m_bobOffset.z + offset.z;

    uint32_t wasSubmerged = this->m_state & 0x200000;
    uint32_t wasSurfaced = this->m_state & 0x100000;
    float liquid = this->UpdateLiquidSurfaceStatus(target);
    CGInputControl* input = CGInputControl::GetActive();
    float distance = facingDistance + this->m_targetOffset;

    if (swimming && !(unit && (unit->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(unit)->GetDrunknessTurnValue() != 0.0f)) {
        uint32_t state = this->m_state;
        uint32_t surfacing = state & 0x100000;

        if ((surfacing || (state & 0x200000)) && s_cvCameraDive->m_intValue && fnotequal(unitPitch, 0.0f) && !autoTracking) {
            float surfacePitch = s_cvCameraSurfacePitch->m_floatValue * 0.017453292f;
            float submergePitch = 0.017453292f * s_cvCameraSubmergePitch->m_floatValue;
            bool pitchPlayer = false;
            bool decided = false;

            if ((state & 0x200000) && surfacePitch < 0.0f) {
                if (!(input->m_flags & INPUT_PLAYER_LOOK_MASK) || (distance < 0.0f && distance > surfacePitch)) {
                    pitchPlayer = true;
                    decided = true;
                }
            }

            if (!decided && surfacing && 0.0f < submergePitch) {
                if (!(input->m_flags & INPUT_PLAYER_LOOK_MASK) || (0.0f < distance && distance < submergePitch)) {
                    pitchPlayer = true;
                }
            }

            if (pitchPlayer) {
                CGInputControl::GetActive()->CameraPitchPlayer(time, 0.0f);
            }
        }
    }

    if (s_cameraWaterCollision->m_intValue) {
        float finalPitch;
        bool haveFinalPitch = false;

        if (wasSubmerged && (this->m_state & 0x100000)) {
            finalPitch = s_cvCameraSurfaceFinalPitch->m_floatValue;
            haveFinalPitch = true;
        } else if (wasSurfaced && (this->m_state & 0x200000)) {
            finalPitch = s_cvCameraSubmergeFinalPitch->m_floatValue;
            haveFinalPitch = true;
        }

        if (haveFinalPitch) {
            finalPitch = finalPitch * 0.017453292f;

            if (0.0f != finalPitch) {
                if (this->m_state & 0x1) {
                    this->m_pitch = finalPitch;
                    this->m_smoothPitch.m_target = finalPitch;
                } else {
                    this->SetDesiredPitchAngle(finalPitch, 0.0f, 1.0f, time);
                }
            }
        }
    }

    if (this->m_vehicleYawStartTime != this->m_vehicleYawEndTime) {
        this->UpdateVehicleYawOffset(time);
    }

    float heightAdjust = 0.0f;
    this->m_state &= ~0x30000;
    this->m_state |= this->CollideCameraWithWorld(safePos, distance, heightAdjust, offset, liquid, this->m_collideExtent);

    C3Vector cameraTarget = C3Vector(safePos.x + offset.x, safePos.y + offset.y, offset.z + safePos.z + heightAdjust);

    if (!(this->m_state & 0x8) && !(this->m_state & 0x80000000)) {
        if (this->CanSmoothTarget()) {
            this->SetDesiredTargetOffset(0.0f, 0.0f, 1.0f, time);
        } else {
            this->CancelSmoothTargetOffset();
        }
    }

    uint32_t state = this->m_state;

    if ((state & 0x8) && !(state & 0x40000000)) {
        distance = 0.0f;
        this->m_state = state & ~0x8000;
    } else if ((state & 0x80000000) && !(state & 0x40000000)) {
        this->m_state = state & 0x7FFFFFFF;
    }

    C3Vector noOffset = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_position = this->GetCameraPosition(cameraTarget, distance, noOffset);
    this->m_position = this->BumpCameraPositionFromLiquid(cameraTarget, distance, this->m_position);

    if (!this->m_relativeTo && !s_cameraWaterCollision->m_intValue && 0.0f == this->m_vehicleYawOffset && (!unit || !unit->GetCanFly())) {
        C3Vector forward = C3Vector(cameraTarget.x - this->m_position.x, cameraTarget.y - this->m_position.y, cameraTarget.z - this->m_position.z);

        if (forward.x * forward.x + forward.y * forward.y + forward.z * forward.z > 0.1f) {
            forward.Normalize();
            this->SetFacing(forward);
        }
    }

    if (this->m_vehiclePitchStartTime != this->m_vehiclePitchEndTime) {
        this->UpdateVehiclePitchOffset(time);
    }

    if (0.0f != this->m_vehiclePitchOffset) {
        this->m_facing.RotateAroundY(-this->m_vehiclePitchOffset);
    }

    if (fabsf(this->m_targetOffset) >= 0.001f) {
        this->m_facing.RotateAroundY(this->m_targetOffset);
    }

    if (this->m_distance - distance > 0.11111111f) {
        this->m_state |= 0x4000000;
        this->m_smoothDistance.m_startTimeMs = time;
        this->m_distance = distance + 0.11111207f;
        this->m_smoothDistance.m_startValue = distance + 0.11111207f;
        this->m_smoothDistance.m_rate = 2.0f;
    }

    if (this->m_height - heightAdjust > 0.11111111f) {
        this->m_state |= 0x20000000;
        this->m_smoothHeight.m_startTimeMs = time;
        this->m_height = heightAdjust + 0.11111207f;
        this->m_smoothHeight.m_startValue = heightAdjust + 0.11111207f;
        this->m_smoothHeight.m_rate = 2.0f;
    }

    uint8_t fade = 0xFF;
    CGPlayer_C* activePlayer = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t sharedVehicle = 0;

    if (activePlayer && unit && activePlayer->m_vehiclePassenger && activePlayer->m_vehiclePassenger->m_state && activePlayer->m_vehiclePassenger->m_state != 3) {
        if (activePlayer == unit || activePlayer->GetTransportGUID() == unit->m_obj->m_guid.m_guid) {
            sharedVehicle = 1;
        }
    }

    CGUnit_C* fadeUnit = unit;

    while (true) {
        float fadeStart = 1.8315002f;
        float fadeEnd = 0.0027777778f;

        if (fadeUnit && ((fadeUnit->m_vehicle && fadeUnit->m_vehicle->m_vehicleRec) || (fadeUnit->m_vehiclePassenger && fadeUnit->m_vehiclePassenger->m_state == 3)) && !sharedVehicle) {
            if (!fadeUnit->m_vehicle || !fadeUnit->m_vehicle->m_vehicleRec) {
                fadeUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ fadeUnit->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));
            }

            if (fadeUnit) {
                VehicleRec* vehicleRec = fadeUnit->m_vehicle ? fadeUnit->m_vehicle->m_vehicleRec : nullptr;

                if (vehicleRec && (vehicleRec->m_flags & 0x80000)) {
                    CM2Model* model = fadeUnit->GetObjectModel();

                    if (model && model->IsLoaded(0, 0)) {
                        M2SequenceInfo info;
                        info.Clear();
                        model->GetSequenceInfo(0, 0, info);
                        fadeEnd = vehicleRec->m_cameraFadeDistScalarMin * info.m_radius;
                        fadeStart = info.m_radius * vehicleRec->m_cameraFadeDistScalarMax;
                        distance = C3Vector::Distance(this->m_position, model->GetWorldPosition());
                    }
                }
            }
        }

        if (CGBarberShop::m_barberShopEnabled) {
            fadeStart = fadeStart * 0.33333334f;
        }

        if (distance < this->m_height && this->m_pitch < fadeAngle) {
            float scale = (-1.5707964f - this->m_pitch) * -2.604353f;

            if (fabsf(scale) >= 0.00000023841858f) {
                fadeStart = fadeStart / (scale * scale);
            }
        }

        float nearDistance = distance - this->m_nearZ;

        if (nearDistance < fadeStart) {
            if (fadeEnd >= nearDistance) {
                fade = 0;
                break;
            }

            uint8_t value = static_cast<uint8_t>(static_cast<int32_t>(OrganicSmooth(0.0f, 255.0f, (nearDistance - fadeEnd) / (fadeStart - fadeEnd))));

            if (fade > value) {
                fade = value;
            }
        }

        if (fadeUnit && fadeUnit->HasVehicleTransport() && !sharedVehicle) {
            fadeUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ fadeUnit->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

            if (fadeUnit) {
                continue;
            }
        }

        break;
    }

    CGWorldFrame::s_currentWorldFrame->SetPlayerFadeCameraValue(fade);

    state = this->m_state;

    if ((state & 0x40000000) && ((state & 0x8000) || (state & 0x80000000))) {
        int32_t elapsed = time - this->m_smoothFoV.m_startTimeMs;

        if (elapsed >= 0) {
            float t = static_cast<uint32_t>(elapsed) * 0.001f / this->m_smoothFoV.m_rate * 1.5f;

            if (t >= 1.0f) {
                t = 1.0f;
            }

            t = t * t;

            if (state & 0x8000) {
                t = 1.0f - t;
            }

            uint8_t value = static_cast<uint8_t>(static_cast<int32_t>(t * 255.0f));

            if (fade >= value) {
                fade = value;
            }

            CGWorldFrame::s_currentWorldFrame->SetPlayerFadeCameraValue(fade);
        }
    }

    this->UpdateTargetHeight(target, time);
    this->m_targetPosition = target->GetPosition();
    this->m_wasFlying = flying;
}

// VERIFY: 4dd64323 manual
// OFFSET: 0x00607B00
int32_t CGCamera::UpdateCallback(const void* data, void* param) {
    CGCamera* camera = static_cast<CGCamera*>(param);

    if (!camera) {
        return 1;
    }

    uint32_t time = OsGetAsyncTimeMs();
    camera->m_nearZ = World::s_nearClip;
    camera->m_farZ = World::s_farClip;

    if (camera->m_model) {
        camera->CalcModelCamera(time);
        camera->CheckUnderwater();
        return 1;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        camera->CalcTargetCamera(target, time);
        camera->CheckUnderwater();
        return 1;
    }

    CGObject_C* faceTarget = ClntObjMgrObjectPtr(camera->m_guid0090, TYPE_OBJECT, __FILE__, __LINE__);

    if (faceTarget) {
        camera->FaceTarget(faceTarget, time);
        camera->CheckUnderwater();
    }

    return 1;
}

// VERIFY: b42392d2 manual
// OFFSET: 0x006020B0
void CGCamera::UpdateFreeLookFacing(float x, float y, float* pitchOut) {
    if (pitchOut) {
        *pitchOut = 0.0f;
    }

    if (this->m_state & 0x8000) {
        return;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target && (target->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGUnit_C*>(target)->m_unit->m_unitFieldFlags & 0x100000) && (static_cast<CGPlayer_C*>(target)->m_player->m_playerFlags & 0x20000)) {
        return;
    }

    this->m_state |= 0x40;
    float pitchSpeed = s_cvCameraPitchMoveSpeed->m_floatValue;
    float yawSpeed = s_cvCameraYawMoveSpeed->m_floatValue;
    float pivotDXMax = s_cvCameraPivotDXMax->m_floatValue;
    float pivotDYMin = s_cvCameraPivotDYMin->m_floatValue;
    DDCToNDC(x, y, &x, &y);

    float yawDelta = yawSpeed * 0.017453292f * (x * 0.00125f);
    float pitchDelta = 0.017453292f * pitchSpeed * (y * 0.0016666667f);
    float pitchSign = s_cvMouseInvertPitch->m_intValue ? -1.0f : 1.0f;
    float yawSign = s_cvMouseInvertYaw->m_intValue ? -1.0f : 1.0f;

    if (pitchOut) {
        *pitchOut = pitchDelta * pitchSign;
        pitchDelta = 0.0f;
    }

    uint32_t canSmooth = this->CanSmoothTargetFacing(target);
    uint32_t state = this->m_state;
    int32_t adjust = 0;

    if (!(state & 0x8000000) && std::fabs(this->m_targetOffset) >= 0.001f) {
        adjust = 1;
    }

    if (canSmooth) {
        if (std::fabs(pitchDelta) > pivotDYMin && std::fabs(yawDelta) < pivotDXMax) {
            adjust = 1;
        }

        if (pitchDelta > 0.0f) {
            int32_t nearZero = std::fabs(this->m_targetOffset) < 0.00000023841858f;

            if (this->m_targetOffset < 0.0f && pitchSign * pitchDelta + this->m_targetOffset > 0.0f) {
                pitchDelta = pitchDelta + this->m_targetOffset;
                adjust = 0;
                this->m_targetOffset = 0.0f;
                this->m_smoothTargetOffset.m_target = 0.0f;
            } else if (nearZero) {
                adjust = 0;
                this->m_targetOffset = 0.0f;
                this->m_smoothTargetOffset.m_target = 0.0f;
            }
        }
    }

    int32_t setDesired = adjust == 0;

    if (adjust) {
        this->m_targetOffset = pitchDelta * pitchSign + this->m_targetOffset;

        if (this->m_pitch >= 0.0f) {
            setDesired = 1;
        }

        float minOffset = MIN_PITCH_ANGLE - this->m_pitch;

        if (this->m_pitch < 0.0f && this->m_targetOffset < minOffset) {
            this->m_targetOffset = minOffset;
        }

        if (this->m_targetOffset > 0.0f) {
            setDesired = 1;
        }
    }

    if (setDesired) {
        this->SetDesiredTargetOffset(0.0f, 0.0f, 1.0f, OsGetAsyncTimeMs());
        this->ClampPitchToLimits(pitchSign * pitchDelta);
    } else {
        this->m_smoothTargetOffset.m_target = this->m_targetOffset;
        this->m_smoothTargetOffset.m_startTimeMs = 0;
        this->m_state = state & ~0x8000000;
        this->m_smoothTargetOffset.m_rate = 0.0f;
    }

    this->UpdateYaw(-(yawSign * yawDelta));
    this->ClampPitchAndNormalize();
}

// VERIFY: 7981f8de manual
// OFFSET: 0x006023D0
void CGCamera::SyncFreeLookFacing() {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__));
    float yaw = this->m_yaw;

    if (!unit || !unit->ClampRawAngleToLegalFacingRange(yaw)) {
        this->m_state |= 0x4000;
        this->m_flags |= 0x4;
        this->m_freeLookYaw = this->m_yaw;
    } else if (this->m_flags & 0x4) {
        this->m_state |= 0x4000;
        this->m_smoothYaw.m_target = yaw;
        this->m_yaw = yaw;
        this->m_freeLookYaw = yaw;
    } else {
        this->m_state &= ~0x4000;
    }

    float pitch = this->m_targetOffset + this->m_pitch;

    if (MIN_PITCH_ANGLE > pitch) {
        pitch = MIN_PITCH_ANGLE;
    } else if (MAX_PITCH_ANGLE <= pitch) {
        pitch = MAX_PITCH_ANGLE;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t state = this->m_state;

    if (!(state & 0x100) && ((state & 0x100000) || (state & 0x200000)) && s_cvCameraDive->m_intValue && (!player || player->GetDrunknessTurnValue() == 0.0f)) {
        float surfacePitch = s_cvCameraSurfacePitch->m_floatValue * 0.017453292f;
        float submergePitch = 0.017453292f * s_cvCameraSubmergePitch->m_floatValue;

        if ((state & 0x200000) && surfacePitch < 0.0f && pitch < 0.0f && pitch > surfacePitch) {
            pitch = 0.0f;
        }

        if ((state & 0x100000) && submergePitch > 0.0f && pitch > 0.0f && pitch < submergePitch) {
            pitch = 0.0f;
        }
    }

    uint32_t time = OsGetAsyncTimeMs();
    CGInputControl* input = CGInputControl::GetActive();
    input->CameraTurnPlayer(time, this->m_yaw);

    if (!unit || !unit->m_vehicle || !unit->m_vehicle->m_vehicleRec || !unit->m_vehicle->IsSuppressingCameraPitchWhileMouseAiming()) {
        input->CameraPitchPlayer(time, pitch);
    }
}

// VERIFY: 33ddff2e manual
// OFFSET: 0x006000E0
void CGCamera::UpdateMotion(uint32_t time) {
    float minDistance;

    if (this->m_flags & 0x8) {
        minDistance = this->m_overrideDistanceMin;
    } else if (CGBarberShop::m_barberShopEnabled) {
        minDistance = CGBarberShop::GetMinCameraDistance();
    } else {
        minDistance = MIN_CAMERA_DIST;
    }

    float maxDistance;

    if (this->m_flags & 0x10) {
        maxDistance = this->m_overrideDistanceMax;
    } else if (this->m_vehicleZoomEnabled) {
        maxDistance = 50.0f;
    } else {
        maxDistance = s_cvCameraDistanceMaxFactor->m_floatValue * s_cvCameraDistanceMax->m_floatValue;

        if (maxDistance < 0.0f) {
            maxDistance = 0.0f;
        } else if (50.0f <= maxDistance) {
            maxDistance = 50.0f;
        }
    }

    this->m_state |= 0x40;

    for (int32_t i = 0; i < NUM_CAMERA_MOTIONS; i++) {
        uint32_t bit = 1 << (2 * i);

        if (!(this->m_motionFlags & bit)) {
            continue;
        }

        uint32_t endTime = this->m_motionEndTime[i];

        if (endTime && static_cast<int32_t>(time - endTime) >= 0) {
            this->m_motionStopTime[i] = endTime;
            this->m_motionFlags |= 1 << (2 * i + 1);
        }

        uint32_t elapsed;

        if (this->m_motionFlags & (1 << (2 * i + 1))) {
            int32_t delta = this->m_motionStopTime[i] - this->m_motionStartTime[i];
            elapsed = delta < 0 ? 0 : delta;
            this->m_motionFlags &= ~(3 << (2 * i));

            if (!(this->m_motionFlags & bit)) {
                this->m_flags &= ~0x40;
            }
        } else {
            elapsed = time - this->m_motionStartTime[i];

            if (static_cast<int32_t>(elapsed) < 0) {
                continue;
            }

            this->m_motionStartTime[i] = time;
        }

        switch (i) {
            case CAMERA_MOVE_IN:
                this->m_smoothDistance.m_target = this->m_smoothDistance.m_target - s_cvCameraDistanceMoveSpeed->m_floatValue * this->m_motionSpeed[CAMERA_MOVE_IN] * static_cast<float>(elapsed) * 0.001f;

                if (this->m_smoothDistance.m_target < minDistance) {
                    this->m_smoothDistance.m_target = minDistance;
                }

                if (!(this->m_state & 0x4000000)) {
                    this->m_distance = this->m_smoothDistance.m_target;
                }

                break;

            case CAMERA_MOVE_OUT:
                this->m_smoothDistance.m_target = s_cvCameraDistanceMoveSpeed->m_floatValue * this->m_motionSpeed[CAMERA_MOVE_OUT] * static_cast<float>(elapsed) * 0.001f + this->m_smoothDistance.m_target;

                if (this->m_smoothDistance.m_target > maxDistance) {
                    this->m_smoothDistance.m_target = maxDistance;
                }

                if (!(this->m_state & 0x4000000)) {
                    this->m_distance = this->m_smoothDistance.m_target;
                }

                break;

            case CAMERA_MOVE_RIGHT:
                this->m_smoothYaw.m_target = s_cvCameraYawMoveSpeed->m_floatValue * this->m_motionSpeed[CAMERA_MOVE_RIGHT] * static_cast<float>(elapsed) * 0.001f * 0.017453292f + this->m_smoothYaw.m_target;

                if (this->m_smoothYaw.m_target > TWO_PI) {
                    do {
                        this->m_smoothYaw.m_target = this->m_smoothYaw.m_target - TWO_PI;
                    } while (TWO_PI < this->m_smoothYaw.m_target);
                }

                if (!(this->m_state & 0x1000000)) {
                    this->m_yaw = this->m_smoothYaw.m_target;
                }

                break;

            case CAMERA_MOVE_LEFT:
                this->m_smoothYaw.m_target = this->m_smoothYaw.m_target - s_cvCameraYawMoveSpeed->m_floatValue * this->m_motionSpeed[CAMERA_MOVE_LEFT] * static_cast<float>(elapsed) * 0.001f * 0.017453292f;

                if (this->m_smoothYaw.m_target < 0.0f) {
                    do {
                        this->m_smoothYaw.m_target = this->m_smoothYaw.m_target + TWO_PI;
                    } while (0.0f > this->m_smoothYaw.m_target);
                }

                if (!(this->m_state & 0x1000000)) {
                    this->m_yaw = this->m_smoothYaw.m_target;
                }

                break;

            case CAMERA_MOVE_UP:
                this->m_smoothPitch.m_target = s_cvCameraPitchMoveSpeed->m_floatValue * this->m_motionSpeed[CAMERA_MOVE_UP] * static_cast<float>(elapsed) * 0.001f * 0.017453292f + this->m_smoothPitch.m_target;

                if (this->m_smoothPitch.m_target > MAX_PITCH_ANGLE) {
                    this->m_smoothPitch.m_target = MAX_PITCH_ANGLE;
                }

                if (!(this->m_state & 0x2000000)) {
                    this->m_pitch = this->m_smoothPitch.m_target;
                }

                break;

            case CAMERA_MOVE_DOWN:
                this->m_smoothPitch.m_target = this->m_smoothPitch.m_target - s_cvCameraPitchMoveSpeed->m_floatValue * this->m_motionSpeed[CAMERA_MOVE_DOWN] * static_cast<float>(elapsed) * 0.001f * 0.017453292f;

                if (this->m_smoothPitch.m_target < MIN_PITCH_ANGLE) {
                    this->m_smoothPitch.m_target = MIN_PITCH_ANGLE;
                }

                if (!(this->m_state & 0x2000000)) {
                    this->m_pitch = this->m_smoothPitch.m_target;
                }

                break;
        }
    }
}

// VERIFY: 8786c823 manual
// OFFSET: 0x006048A0
void CGCamera::ResetView(int32_t view) {
    if (view < 0) {
        for (int32_t i = 0; i < 8; i++) {
            this->ResetView(i);
        }

        return;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (s_cameraViewAngular[i]) {
            this->m_views[view].Component(i) = SStrToFloat(s_cameraViewDefaults[view].m_values[i]) * 0.017453292f;
        } else {
            this->m_views[view].Component(i) = SStrToFloat(s_cameraViewDefaults[view].m_values[i]);
        }
    }

    if (view == this->m_viewIndex) {
        this->SetView(this->m_viewIndex, s_cvCameraViewBlendStyle->m_intValue, 0);
    }
}

// VERIFY: 9a9cfabb manual
// OFFSET: 0x00603330
void CGCamera::SetView(int32_t view, int32_t blendStyle, int32_t transition) {
    if (this->m_state & 0x8) {
        return;
    }

    if (view == 6) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (!(player->m_player->m_playerFlags & 0x80000)) {
            return;
        }
    }

    if (this->m_viewIndex == view && !(this->m_state & 0x40) && blendStyle) {
        blendStyle = 2;
    }

    SysMsgPrintf(SYSMSG_INFO, 4, "Camera view %s", s_cameraViewModeNames[view]);
    uint32_t time = OsGetAsyncTimeMs();

    if (this->m_viewIndex != view) {
        if (!transition) {
            char value[16];
            SStrPrintf(value, sizeof(value), "%d", view);
            s_cvCameraView->Set(value, true, false, false, true);
        }

        this->m_viewIndex = view;
    }

    this->m_state &= ~0x40;
    uint32_t state = this->m_state;

    if (transition == 1) {
        this->m_preTelescopeDistance = this->m_smoothDistance.m_target;
    }

    this->m_smoothDistance.m_target = this->m_distance;
    state &= 0xF9FFFFFF;
    this->m_smoothDistance.m_rate = 0.0f;
    this->m_smoothDistance.m_startTimeMs = 0;
    this->m_smoothPitch.m_startTimeMs = 0;
    this->m_smoothPitch.m_target = this->m_pitch;
    this->m_state = state;
    this->m_smoothPitch.m_rate = 0.0f;

    if (transition != 1) {
        this->m_smoothTargetOffset.m_target = this->m_targetOffset;
        this->m_smoothTargetOffset.m_startTimeMs = 0;
        this->m_state = state & ~0x8000000;
        this->m_smoothTargetOffset.m_rate = 0.0f;
    }

    if (transition != 2) {
        this->m_state &= ~0x1000000;
        this->m_smoothYaw.m_target = this->m_yaw;
        this->m_smoothYaw.m_startTimeMs = 0;
        this->m_smoothYaw.m_rate = 0.0f;
    }

    float yaw = this->m_views[this->m_viewIndex].m_yaw;

    if (this->m_ignoreFacingRefs > 0) {
        CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);
        float facing;

        if (!target) {
            facing = 0.0f;
        } else if (target->m_obj->m_type & TYPE_UNIT) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(target);
            unit->UpdateSmoothFacing(nullptr);
            facing = this->m_vehicleCamera ? this->m_vehicleCamera->m_facing : unit->m_smoothTarget.m_value;
        } else {
            facing = target->GetFacing();
        }

        yaw = CMath::normalizeangle0to2pi_(facing + yaw);
    }

    if (blendStyle == 1) {
        int32_t distanceSet = this->SmoothSetDistance(this->m_views[view].m_distance, 0.0f, 1.0f, time);
        int32_t pitchSet = this->SetDesiredPitchAngle(this->m_views[view].m_pitch, 0.0f, 1.0f, time);
        int32_t yawSet = this->SetDesiredYawAngle(yaw, 0.0f, 1.0f, time);
        int32_t offsetSet = this->SetDesiredTargetOffset(0.0f, 0.0f, 1.0f, time);
        float rate = 0.0f;

        if (distanceSet && !(this->m_smoothDistance.m_rate < 0.0f)) {
            rate = this->m_smoothDistance.m_rate;
        }

        if (pitchSet && rate <= this->m_smoothPitch.m_rate) {
            rate = this->m_smoothPitch.m_rate;
        }

        if (yawSet && rate <= this->m_smoothYaw.m_rate) {
            rate = this->m_smoothYaw.m_rate;
        }

        if (offsetSet && rate <= this->m_smoothTargetOffset.m_rate) {
            rate = this->m_smoothTargetOffset.m_rate;
        }

        float minTime = s_cvCameraSmoothTimeMin->m_floatValue;
        float maxTime = s_cvCameraSmoothTimeMax->m_floatValue;

        if (rate < minTime) {
            rate = minTime;
        }

        if (rate > maxTime) {
            rate = maxTime;
        }

        if (distanceSet) {
            this->m_smoothDistance.m_rate = rate;
        }

        if (pitchSet) {
            this->m_smoothPitch.m_rate = rate;
        }

        if (yawSet) {
            this->m_smoothYaw.m_rate = rate;
        }

        if (offsetSet) {
            this->m_smoothTargetOffset.m_rate = rate;
        }

        if ((this->m_state & 0x10000000) && (distanceSet || pitchSet || yawSet || offsetSet)) {
            this->m_smoothGroundTilt.m_rate = rate;
        }

        return;
    }

    if (blendStyle != 2) {
        return;
    }

    int32_t changed;

    if (!transition) {
        int32_t distanceChanged = fnotequalz(this->m_distance, this->m_views[view].m_distance, 0.001f) != 0;
        int32_t pitchChanged = fnotequalz(this->m_pitch, this->m_views[view].m_pitch, 0.001f) != 0;
        int32_t yawChanged = fnotequalz(this->m_yaw, yaw, 0.001f) != 0;
        int32_t offsetChanged = fnotequalz(this->m_targetOffset, 0.0f, 0.001f) != 0;
        this->m_distance = this->m_views[view].m_distance;
        this->m_smoothDistance.m_target = this->m_views[view].m_distance;
        this->m_pitch = this->m_views[view].m_pitch;
        this->m_smoothPitch.m_target = this->m_views[view].m_pitch;
        this->m_yaw = yaw;
        this->m_smoothYaw.m_target = yaw;
        this->m_targetOffset = 0.0f;
        this->m_smoothTargetOffset.m_target = 0.0f;
        changed = distanceChanged || pitchChanged || yawChanged || offsetChanged;
    } else if (transition == 1) {
        this->m_prevDistance = this->m_distance;
        this->m_prevViewIndex = this->m_viewIndex;
        changed = 1;
    } else if (transition == 2) {
        this->m_smoothDistance.m_target = 0.0f;
        this->m_distance = 0.0f;
        changed = 1;
    } else {
        changed = 0;
    }

    if ((this->m_state & 0x10) && (this->m_state & 0x10000000) && changed) {
        this->PerformTerrainTilt(ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__), time, 1);
    }
}

// VERIFY: 4ff11508 manual
// OFFSET: 0x00602DC0
void CGCamera::UpdateTrackingState(int32_t tracking) {
    uint32_t wasTracking = this->m_state & 0x100;
    this->m_state &= ~0x4000;

    if (wasTracking) {
        if (!tracking) {
            this->m_state &= ~0x100;
        }
    } else if (tracking) {
        this->m_state |= 0x100;
    }

    uint32_t ignoring = this->m_state & 0x2000;
    int32_t ignore;

    if (!(this->m_state & 0x100)) {
        ignore = 0;
    } else if (this->m_vehicleTrackingStyle == 1) {
        ignore = 1;
    } else if (this->m_vehicleTrackingStyle == 2) {
        ignore = 0;
    } else {
        int32_t style = s_cvCameraSmoothTrackingStyle->m_intValue;

        if (!style) {
            ignore = 1;
        } else if (style == 2) {
            ignore = 0;
        } else {
            uint32_t type = CGUnit_C::GetTrackingType();
            ignore = type != 1 && type != 3;
        }
    }

    if (ignoring) {
        if (!ignore) {
            this->DecIgnoreFacing();
            this->m_state &= ~0x2000;
        }
    } else if (ignore) {
        this->IncIgnoreFacing();
        this->m_state |= 0x2000;
    }

    this->SmoothFreeLook(CGInputControl::GetActive(), 0);
}

// VERIFY: 3634e85f manual
// OFFSET: 0x00602600
int32_t CGCamera::CanSmoothTarget() {
    if (fabs(this->m_targetOffset) < 0.001f) {
        return 0;
    }

    if (this->m_state & 0x100) {
        if (this->m_vehicleTrackingStyle == 1) {
            return 0;
        }

        if (this->m_vehicleTrackingStyle != 2 && (!s_cvCameraSmoothTrackingStyle->m_intValue || !CGUnit_C::GetTrackingType())) {
            return 0;
        }
    }

    return !this->CanSmoothTargetFacing(ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__));
}

// VERIFY: 76fb08be manual
// OFFSET: 0x00602680
int32_t CGCamera::CanSmoothYaw(float yawMin, float yawMax) {
    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (!target || (this->m_state & 0x1) || !s_cvCameraSmoothYaw->m_intValue) {
        return 0;
    }

    if (this->m_ignoreFacingRefs <= 0) {
        return yawMin > this->m_yaw || yawMax < this->m_yaw;
    }

    float facing;

    if (target->m_obj->m_type & TYPE_UNIT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(target);

        if (this->m_state & 0x100) {
            facing = CGUnit_C::GetTrackingTurn();
        } else {
            unit->UpdateSmoothFacing(nullptr);
            facing = this->GetChaseFacing(unit);
        }
    } else {
        facing = target->GetFacing();
    }

    return fnotequal(this->m_yaw, facing);
}

// VERIFY: e499a980 manual
// OFFSET: 0x00602760
void CGCamera::SmoothFreeLook(CGInputControl* input, int32_t idle) {
    if (!input) {
        return;
    }

    uint32_t reasons = 0;

    if (this->m_state & 0x1000) {
        reasons = 0x40;
    }

    uint32_t flags = input->m_flags;

    if (((flags & INPUT_TURN_PLAYER_MASK) && !(flags & INPUT_PLAYER_LOOK_MASK)) || (flags & INPUT_PLAYER_LOOK_MASK)) {
        reasons |= 0x20;
    }

    if ((flags & INPUT_STRAFE_PLAYER_MASK) || ((flags & INPUT_PLAYER_LOOK_MASK) && (flags & INPUT_TURN_PLAYER_MASK))) {
        reasons |= 0x10;
    }

    if ((flags & INPUT_MOVE_WITH_AUTORUN_MASK) || ((flags & INPUT_TURN_PLAYER) && (flags & INPUT_TURN_CAMERA))) {
        reasons |= 0x8;
    }

    if (this->m_state & 0x100) {
        if (CGUnit_C::GetTrackingType() == TRACKTYPE_FOLLOW && !(this->m_flags & 0x1)) {
            reasons |= 0x8;
        } else {
            reasons |= 0x4;
        }
    }

    if (idle) {
        reasons |= 0x2;
    }

    flags = input->m_flags;

    if (!(flags & INPUT_MOVE_WITH_AUTORUN_MASK) && !(flags & INPUT_STRAFE_PLAYER_MASK) && (!(flags & INPUT_PLAYER_LOOK_MASK) || !(flags & INPUT_TURN_PLAYER_MASK)) && (!(flags & INPUT_TURN_PLAYER_MASK) || (flags & INPUT_PLAYER_LOOK_MASK)) && !(flags & INPUT_TRACKING_MASK)) {
        reasons |= 0x1;
    }

    this->m_state &= ~0x4000u;
    reasons &= 0x7F;

    if (!reasons || !this->IsCustomViewSmoothingActive()) {
        return;
    }

    uint32_t time = OsGetAsyncTimeMs();
    uint32_t style;

    if (reasons & 0x44) {
        if (this->m_vehicleTrackingStyle == 1) {
            style = 0;
        } else if (this->m_vehicleTrackingStyle == 2) {
            style = 3;
        } else {
            style = s_cvCameraSmoothTrackingStyle->m_intValue;
        }
    } else {
        style = s_cvCameraSmoothStyle->m_intValue;
    }

    float smoothState[2];

    for (int32_t state = 7; state-- > 0;) {
        if (!((1 << state) & reasons) || style >= 5 || static_cast<uint32_t>(state) >= 7) {
            continue;
        }

        smoothState[0] = s_cvCameraSmoothState[style][state][0]->m_floatValue;

        if (smoothState[0] < 0.0f) {
            smoothState[0] = 0.0f;
        } else if (smoothState[0] >= 100.0f) {
            smoothState[0] = 99.0f;
        }

        smoothState[1] = s_cvCameraSmoothState[style][state][1]->m_floatValue;

        if (smoothState[1] < 0.0f) {
            smoothState[1] = 0.0f;
        } else if (smoothState[1] >= 100.0f) {
            smoothState[1] = 99.0f;
        }

        break;
    }

    float viewData[3][2];

    for (uint32_t view = 3; view-- > 0;) {
        if (style >= 5 || view >= 3) {
            continue;
        }

        for (int32_t i = 2; i-- > 0;) {
            viewData[view][i] = s_cvCameraSmoothViewData[style][view][i]->m_floatValue;

            switch (i) {
                case 0:
                    viewData[view][0] = viewData[view][0] + smoothState[0];
                    break;
                case 1:
                    viewData[view][1] = viewData[view][1] * smoothState[1];
                    break;
            }

            if (s_smoothViewDataMin[i] > viewData[view][i]) {
                viewData[view][i] = s_smoothViewDataMin[i];
            }

            if (s_smoothViewDataMax[i] <= viewData[view][i]) {
                viewData[view][i] = s_smoothViewDataMax[i] - 1.0f;
            }
        }
    }

    float pitchMin = s_cvCameraPitchSmoothMin->m_floatValue * 0.017453292f;
    float pitchMax = s_cvCameraPitchSmoothMax->m_floatValue * 0.017453292f;
    float yawMin = s_cvCameraYawSmoothMin->m_floatValue * 0.017453292f;
    float yawMax = 0.017453292f * s_cvCameraYawSmoothMax->m_floatValue;

    int32_t smoothPitch = this->ShouldSmoothPitch(pitchMin, pitchMax);
    int32_t smoothTarget = this->CanSmoothTarget();
    int32_t smoothYaw = this->CanSmoothYaw(yawMin, yawMax);
    float rate = 0.0f;

    if (smoothPitch) {
        if (viewData[1][1] == 0.0f) {
            this->CancelSmoothPitch();
            smoothPitch = 0;
        } else {
            float pitch = this->m_views[this->m_viewIndex].m_pitch;

            if (pitchMin > this->m_pitch) {
                pitch = pitchMin;
            }

            if (pitchMax < this->m_pitch) {
                pitch = pitchMax;
            }

            smoothPitch = this->SetDesiredPitchAngle(pitch, viewData[1][0], viewData[1][1], time);

            if (smoothPitch && this->m_smoothPitch.m_rate >= 0.0f) {
                rate = this->m_smoothPitch.m_rate;
            }
        }
    }

    if (smoothTarget) {
        if (smoothState[1] == 0.0f) {
            this->CancelSmoothTargetOffset();
            smoothTarget = 0;
        } else {
            smoothTarget = this->SetDesiredTargetOffset(0.0f, smoothState[0], smoothState[1], time);

            if (smoothTarget && rate <= this->m_smoothTargetOffset.m_rate) {
                rate = this->m_smoothTargetOffset.m_rate;
            }
        }
    }

    if (smoothYaw) {
        if (0.0f == viewData[2][1]) {
            this->CancelSmoothYaw();
            smoothYaw = 0;
        } else {
            float base = this->m_views[this->m_viewIndex].m_yaw;
            float yaw = base;

            if (this->m_ignoreFacingRefs <= 0) {
                if (yawMin > this->m_yaw) {
                    yaw = yawMin;
                }

                if (yawMax < this->m_yaw) {
                    yaw = yawMax;
                }
            } else {
                CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

                if (target && (target->m_obj->m_type & TYPE_UNIT)) {
                    CGUnit_C* unit = static_cast<CGUnit_C*>(target);

                    if (this->m_state & 0x100) {
                        yaw = CMath::normalizeangle0to2pi_(CGUnit_C::GetTrackingTurn() + base);
                    } else {
                        unit->UpdateSmoothFacing(nullptr);

                        if (input->m_facingOverrideActive) {
                            yaw = CMath::normalizeangle0to2pi_(input->m_facingOverride + base);
                        } else {
                            yaw = CMath::normalizeangle0to2pi_(this->GetChaseFacing(unit) + base);
                        }
                    }
                } else {
                    yaw = CMath::normalizeangle0to2pi_(target->GetFacing() + base);
                }
            }

            smoothYaw = this->SetDesiredYawAngle(yaw, viewData[2][0], viewData[2][1], time);

            if (smoothYaw && rate <= this->m_smoothYaw.m_rate) {
                rate = this->m_smoothYaw.m_rate;
            }
        }
    }

    float minTime = s_cvCameraSmoothTimeMin->m_floatValue;
    float maxTime = s_cvCameraSmoothTimeMax->m_floatValue;

    if (rate >= minTime) {
        minTime = rate;
    }

    if (minTime <= maxTime) {
        maxTime = minTime;
    }

    if (smoothYaw) {
        this->m_smoothYaw.m_rate = maxTime;
    }

    if (smoothPitch) {
        this->m_smoothPitch.m_rate = maxTime;
    }

    if (smoothTarget) {
        this->m_smoothTargetOffset.m_rate = maxTime;
    }
}

// VERIFY: 3503b548 manual
// OFFSET: 0x006061D0
C3Vector CGCamera::BumpCameraPositionFromLiquid(const C3Vector& target, float distance, const C3Vector& position) {
    C3Vector result = position;

    if (!(0.0f < distance)) {
        return result;
    }

    C3Vector start = C3Vector(position.x, position.y, position.z + 0.22222222f);
    C3Vector end = C3Vector(position.x, position.y, position.z - 0.22222222f);
    float hitDistance = 1.0f;
    C3Vector hit = C3Vector(0.0f, 0.0f, 0.0f);

    if (World::Intersect(start, end, &hit, &hitDistance, 0x20000, nullptr)) {
        result = hit;

        if (result.z < position.z) {
            result.z = result.z + 0.22222222f;
        } else {
            result.z = result.z - 0.22222222f;
        }

        if (this->GetCameraDistance(distance, result, target, 0x100171)) {
            C3Vector forward = this->Forward();
            this->RotateOffsetByRoll(forward);
            result = C3Vector(target.x - forward.x * distance, target.y - forward.y * distance, target.z - forward.z * distance);
        }
    }

    s_lastLiquidBumpPitch = this->m_pitch;
    return result;
}

// VERIFY: 6643b868 manual
// OFFSET: 0x00602F30
void CGCamera::RunBobs(C3Vector& offset) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit || !this->IsBobbingAllowed(unit)) {
        return;
    }

    float facing = this->m_vehicleCamera ? this->m_vehicleCamera->m_facing : unit->m_smoothTarget.m_value;
    float horizontal = s_cameraBobHorizontalAmplitude * 0.027777778f;
    float vertical = 0.027777778f * s_cameraBobVerticalAmplitude;
    float frequency = s_cameraBobFrequency;
    float speed = unit->m_movement->GetBaseSpeed(0) * 0.14285715f;

    if (0.5f > speed) {
        speed = 0.5f;
    } else if (1.5f <= speed) {
        speed = 1.5f;
    }

    frequency = speed * frequency;
    float time = static_cast<float>(OsGetAsyncTimeMs() - this->m_bobTime) * 0.001f;
    float sway = std::sin(time * frequency * TWO_PI) * horizontal;
    float angle = facing + 1.5707964f;
    float c = std::cos(angle);
    float s = std::sin(angle);
    offset.x = c * sway;
    offset.y = sway * s;
    offset.z = std::sin((frequency + frequency) * time * TWO_PI) * vertical;
}

// VERIFY: e7e240bc manual
// OFFSET: 0x00602EB0
void CGCamera::UpdateUncontrolledState(int32_t uncontrolled) {
    if (uncontrolled) {
        if (!(this->m_state & 0x1000)) {
            this->IncIgnoreFacing();
            this->m_state = (this->m_state & 0xFFFFAFFF) | 0x1000;
        }

        this->SmoothFreeLook(CGInputControl::GetActive(), 0);
    }

    if ((this->m_state & 0x1000) && !uncontrolled) {
        this->DecIgnoreFacing();
        this->m_state &= 0xFFFFAFFF;
        this->SmoothFreeLook(CGInputControl::GetActive(), 0);
    }
}

// VERIFY: 13ff6f44 manual
// OFFSET: 0x00605D60
uint32_t CGCamera::CollideCameraWithWorld(const C3Vector& target, float& distance, float& height, const C3Vector& offset, float liquid, float& extent) {
    extent = 1.0f;
    uint32_t result = 0;
    distance = this->m_distance < this->m_smoothDistance.m_target ? this->m_smoothDistance.m_target : this->m_distance;
    height = this->m_height < this->m_smoothHeight.m_target ? this->m_smoothHeight.m_target : this->m_height;
    uint32_t state = this->m_state;

    if (state & 0x8) {
        distance = this->m_distance;
        return 0;
    }

    uint32_t flags = s_cameraWaterCollision->m_intValue ? 0x120171 : 0x100171;
    float minHeight = 0.83333331f;
    float maxHeight = this->m_height;

    if (height - this->m_nearZ > 0.00000095367432f) {
        if (flags & 0x30000) {
            if (state & 0x100000) {
                minHeight = liquid + 0.22222222f;

                if (!(minHeight < maxHeight)) {
                    maxHeight = minHeight;
                }
            } else if (state & 0x200000) {
                maxHeight = liquid - 0.83333331f;

                if (maxHeight <= 0.83333331f) {
                    maxHeight = 0.83333331f;
                }
            }
        }

        C3Vector start = C3Vector(target.x, target.y, target.z + minHeight);
        height = height - minHeight;
        height = height + this->m_flyingMountHeight;

        if (!(0.11111111f < height)) {
            height = 0.11111111f;
        }

        C3Vector hit = C3Vector(0.0f, 0.0f, 0.0f);
        float probe = height;
        C3Vector end = C3Vector(target.x, target.y, start.z + height);
        float hitDistance = 1.0f;

        if (World::Intersect(start, end, &hit, &hitDistance, flags, nullptr)) {
            extent = hitDistance;
            result = 0x20000;
            height = hitDistance * height;
        }

        if (std::fabs(probe - height) >= 0.00000023841858f) {
            height = height - 0.11111111f;

            if (height <= 0.11111111f) {
                height = 0.11111111f;
            }
        }

        height = height + minHeight;
        height = height - this->m_flyingMountHeight;
        CGObject_C* object = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

        if (object && (object->m_obj->m_type & TYPE_UNIT)) {
            float collisionHeight = static_cast<CGUnit_C*>(object)->m_movementData.m_collisionHeight;
            float minimum = object->m_obj->m_guid == ClntObjMgrGetActivePlayer() && CGBarberShop::m_barberShopEnabled ? collisionHeight : collisionHeight * 0.75f;

            if (!(minimum <= height)) {
                height = minimum;
            }
        }
    }

    if (!(minHeight <= height)) {
        height = minHeight;
    }

    if (maxHeight < height) {
        height = maxHeight;
    }

    C3Vector base = C3Vector(target.x, target.y, height + target.z);
    float original = distance;

    if (distance - this->m_nearZ > 0.00000095367432f) {
        C3Vector hit = C3Vector(0.0f, 0.0f, 0.0f);
        C3Vector forward = this->Forward();
        C3Vector end = C3Vector(base.x - forward.x * distance, base.y - forward.y * distance, base.z - forward.z * distance);

        if (std::fabs(this->m_flyingMountHeight) >= 0.00000023841858f) {
            float lift = this->m_flyingMountHeight;
            C3Vector up = this->Up();
            end.x = up.x * lift * extent + end.x;
            end.y = up.y * lift * extent + end.y;
            end.z = up.z * lift * extent + end.z;
        }

        float hitDistance = 1.0f;

        if (World::Intersect(base, end, &hit, &hitDistance, flags, nullptr)) {
            result |= 0x10000;
            distance = distance * hitDistance;
        }

        if (0.00000095367432f < distance) {
            C3Vector position = this->GetCameraPosition(base, distance, offset);

            if (this->GetCameraDistance(distance, position, base, flags)) {
                result |= 0x10000;
            }

            if (fnotequal(original, distance)) {
                distance = distance - 0.11111111f;

                if (!(0.0f < distance)) {
                    distance = 0.0f;
                }
            }
        }
    }

    if (this->m_distance < distance) {
        distance = this->m_distance;
    }

    return result;
}


// VERIFY: fd0b92f6 manual
// OFFSET: 0x00603D30
void CGCamera::UpdateTargetSmoothing(CGObject_C* target, uint32_t time) {
    if (this->HasTargetOffset()) {
        float t = (time - this->m_bobTime) * 0.001f / this->m_bobDuration;

        if (t < 1.0f) {
            this->m_bobOffset.x = OrganicSmooth(this->m_bobStart.x, 0.0f, t);
            this->m_bobOffset.y = OrganicSmooth(this->m_bobStart.y, 0.0f, t);
            this->m_bobOffset.z = OrganicSmooth(this->m_bobStart.z, 0.0f, t);
        } else {
            this->m_bobOffset.x = 0.0f;
            this->m_bobOffset.y = 0.0f;
            this->m_bobOffset.z = 0.0f;
        }
    }

    if (fabsf(this->m_smoothFoV.m_target - this->m_targetFov) < 2.3841858e-7f) {
        this->m_state &= ~0x40000000;
        this->m_smoothFoV.m_target = this->m_targetFov;
        this->m_smoothFoV.m_startTimeMs = 0;
        this->m_smoothFoV.m_rate = 0.0f;
    } else if ((this->m_state & 0x40000000) && static_cast<int32_t>(time - this->m_smoothFoV.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothFoV.m_startTimeMs) * 0.001f / this->m_smoothFoV.m_rate;
        this->m_targetFov = t < 1.0f ? OrganicSmooth(this->m_smoothFoV.m_startValue, this->m_smoothFoV.m_target, t) : this->m_smoothFoV.m_target;
    }

    if (fabsf(this->m_smoothDistance.m_target - this->m_distance) < 2.3841858e-7f) {
        this->m_state &= ~0x4000000;
        this->m_smoothDistance.m_target = this->m_distance;
        this->m_smoothDistance.m_startTimeMs = 0;
        this->m_smoothDistance.m_rate = 0.0f;
    } else if ((this->m_state & 0x4000000) && static_cast<int32_t>(time - this->m_smoothDistance.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothDistance.m_startTimeMs) * 0.001f / this->m_smoothDistance.m_rate;
        this->m_distance = t < 1.0f ? OrganicSmooth(this->m_smoothDistance.m_startValue, this->m_smoothDistance.m_target, t) : this->m_smoothDistance.m_target;
    }

    if (fabsf(this->m_smoothHeight.m_target - this->m_height) < 2.3841858e-7f) {
        this->m_state &= ~0x20000000;
        this->m_smoothHeight.m_target = this->m_height;
        this->m_smoothHeight.m_startTimeMs = 0;
        this->m_smoothHeight.m_rate = 0.0f;
    } else if ((this->m_state & 0x20000000) && static_cast<int32_t>(time - this->m_smoothHeight.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothHeight.m_startTimeMs) * 0.001f / this->m_smoothHeight.m_rate;
        this->m_height = t < 1.0f ? OrganicSmooth(this->m_smoothHeight.m_startValue, this->m_smoothHeight.m_target, t) : this->m_smoothHeight.m_target;
    }

    if (fabsf(this->m_smoothGroundTilt.m_target - this->m_groundTilt) < 2.3841858e-7f) {
        this->m_state &= ~0x10000000;
        this->m_smoothGroundTilt.m_target = this->m_groundTilt;
        this->m_smoothGroundTilt.m_startTimeMs = 0;
        this->m_smoothGroundTilt.m_rate = 0.0f;
    } else if ((this->m_state & 0x10000000) && static_cast<int32_t>(time - this->m_smoothGroundTilt.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothGroundTilt.m_startTimeMs) * 0.001f / this->m_smoothGroundTilt.m_rate;
        this->m_groundTilt = t < 1.0f ? OrganicSmooth(this->m_smoothGroundTilt.m_startValue, this->m_smoothGroundTilt.m_target, t) : this->m_smoothGroundTilt.m_target;
    }

    if (fabsf(this->m_smoothTargetOffset.m_target - this->m_targetOffset) < 2.3841858e-7f) {
        if (!(this->m_state & 0x80000000)) {
            this->m_smoothTargetOffset.m_target = this->m_targetOffset;
            this->m_smoothTargetOffset.m_startTimeMs = 0;
            this->m_state &= ~0x8000000;
            this->m_smoothTargetOffset.m_rate = 0.0f;
        }
    } else if ((this->m_state & 0x8000000) && static_cast<int32_t>(time - this->m_smoothTargetOffset.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothTargetOffset.m_startTimeMs) * 0.001f / this->m_smoothTargetOffset.m_rate;
        this->m_targetOffset = t < 1.0f ? OrganicSmooth(this->m_smoothTargetOffset.m_startValue, this->m_smoothTargetOffset.m_target, t) : this->m_smoothTargetOffset.m_target;
    }

    if (fabsf(this->m_smoothPitch.m_target - this->m_pitch) < 2.3841858e-7f) {
        this->m_state &= ~0x2000000;
        this->m_smoothPitch.m_target = this->m_pitch;
        this->m_smoothPitch.m_startTimeMs = 0;
        this->m_smoothPitch.m_rate = 0.0f;
    } else if ((this->m_state & 0x2000000) && static_cast<int32_t>(time - this->m_smoothPitch.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothPitch.m_startTimeMs) * 0.001f / this->m_smoothPitch.m_rate;
        this->m_pitch = t < 1.0f ? OrganicSmooth(this->m_smoothPitch.m_startValue, this->m_smoothPitch.m_target, t) : this->m_smoothPitch.m_target;
    }

    uint32_t state = this->m_state;

    if ((state & 0x8) && (state & 0x1000000)) {
        this->m_smoothYaw.m_startTimeMs = time;
    }

    if (!(state & 0x1) && !(state & 0x8)) {
        if (fabsf(this->m_smoothYaw.m_target - this->m_yaw) < 2.3841858e-7f) {
            this->m_smoothYaw.m_target = this->m_yaw;
            this->m_smoothYaw.m_startTimeMs = 0;
            this->m_state = state & ~0x1000000;
            this->m_smoothYaw.m_rate = 0.0f;
        } else if ((state & 0x1000000) && static_cast<int32_t>(time - this->m_smoothYaw.m_startTimeMs) >= 0) {
            float t = (time - this->m_smoothYaw.m_startTimeMs) * 0.001f / this->m_smoothYaw.m_rate;
            this->m_yaw = t < 1.0f ? OrganicSmooth(this->m_smoothYaw.m_startValue, this->m_smoothYaw.m_target, t) : this->m_smoothYaw.m_target;
        }
    }

    this->ClampPitchAndNormalize();

    if (fabsf(this->m_smoothFlyingHeight.m_target - this->m_flyingMountHeight) < 2.3841858e-7f) {
        this->m_state &= ~0x400000;
        this->m_smoothFlyingHeight.m_target = this->m_flyingMountHeight;
        this->m_smoothFlyingHeight.m_startTimeMs = 0;
        this->m_smoothFlyingHeight.m_rate = 0.0f;
    } else if ((this->m_state & 0x400000) && static_cast<int32_t>(time - this->m_smoothFlyingHeight.m_startTimeMs) >= 0) {
        float t = (time - this->m_smoothFlyingHeight.m_startTimeMs) * 0.001f / this->m_smoothFlyingHeight.m_rate;
        this->m_flyingMountHeight = t < 1.0f ? OrganicSmooth(this->m_smoothFlyingHeight.m_startValue, this->m_smoothFlyingHeight.m_target, t) : this->m_smoothFlyingHeight.m_target;
    }

    this->CalcTerrainTilt(target, time);
    this->PerformTerrainTilt(target, time, 0);
}

// VERIFY: 471c9dcd manual
// OFFSET: 0x005FF670
static void CreateFrustum(const C44Matrix& proj, const C44Matrix& view, float scale, CWFrustum& frustum, const C3Vector& offset) {
    C3Vector corners[8];
    GxuXformCalcFrustumCorners(view, proj, corners);

    C3Vector center = (corners[1] + corners[0] + corners[2] + corners[3]);
    center.x *= 0.25f;
    center.y *= 0.25f;
    center.z *= 0.25f;

    for (uint32_t i = 0; i < 4; i++) {
        corners[i].x = (corners[i].x - center.x) * scale + center.x;
        corners[i].y = (corners[i].y - center.y) * scale + center.y;
        corners[i].z = (corners[i].z - center.z) * scale + center.z;
    }

    for (uint32_t i = 0; i < 4; i++) {
        corners[i + 4] = offset + corners[i];
    }

    frustum.CalcPlanesFromCorners(corners);
}

// VERIFY: 02fe038b manual
// OFFSET: 0x006057B0
static int32_t CollideWithWorld(CWFrustum& frustum, uint32_t flags, float& hit) {
    if (!flags) {
        return 0;
    }

    // STATIC: 0x00C24E84
    static World::FacetData s_facets;

    s_facets.facets.SetCount(0);
    s_facets.facetIds.m_count = 0;
    World::GetFacets(&frustum, &s_facets, flags, nullptr);

    if (!s_facets.facets.m_count) {
        return 0;
    }

    C44Matrix xform;

    if (!World::NDCXform(&frustum, &xform, false)) {
        return 0;
    }

    int32_t result = 0;

    for (uint32_t i = 0; i < s_facets.facets.m_count; i++) {
        CFacet& facet = s_facets.facets.m_data[i];

        for (uint32_t j = 0; j < 3; j++) {
            facet.v[j] = (facet.v[j] - frustum.corners[0]) * xform;
        }

        C3Vector** clipped;
        uint32_t count;

        if (World::NDCClip(facet.v, 3, &clipped, &count)) {
            result = 1;

            for (uint32_t j = 0; j < count; j++) {
                if (hit < clipped[j]->z) {
                    hit = clipped[j]->z;
                }
            }
        }
    }

    return result;
}

// VERIFY: a4eee95a manual
// OFFSET: 0x006059E0
int32_t CGCamera::GetCameraDistance(float& distance, const C3Vector& position, const C3Vector& target, uint32_t flags) {
    float range = distance - this->m_nearZ;

    if (range < 0.001f) {
        distance = 0.0f;
        return 1;
    }

    C3Vector dir = target - position;

    if (dir.x * dir.x + dir.y * dir.y + dir.z * dir.z < 0.001f) {
        return 0;
    }

    float inv = 1.0f / sqrtf(dir.z * dir.z + dir.y * dir.y + dir.x * dir.x);
    C3Vector forward(dir.x * inv, dir.y * inv, dir.z * inv);
    C3Vector up(forward.y * 0.0f - forward.z * 0.0f, forward.z - forward.x * 0.0f, forward.x * 0.0f - forward.y);

    if (up.y * up.y + up.z * up.z + up.x * up.x < 0.001f) {
        up = C3Vector(forward.y * 0.0f - forward.z, forward.z * 0.0f - forward.x * 0.0f, forward.x - forward.y * 0.0f);
    }

    float upInv = 1.0f / sqrtf(up.z * up.z + up.y * up.y + up.x * up.x);
    up.x *= upInv;
    up.y *= upInv;
    up.z *= upInv;

    C3Vector offset(forward.x * range, forward.y * range, forward.z * range);
    C44Matrix view;
    C44Matrix proj;
    C3Vector at(forward.x + position.x, forward.y + position.y, forward.z + position.z);
    GxuXformCreateLookAtSgCompat(position, at, up, view);
    GxuXformCreateProjection_SG(this->FOV(), this->m_aspect, this->m_nearZ, this->m_farZ, proj);

    float hit = 0.0f;
    int32_t collided = 0;
    CWFrustum frustum;
    CreateFrustum(proj, view, 1.0f, frustum, offset);

    if (CollideWithWorld(frustum, flags & 0x30000, hit)) {
        collided = 1;
    }

    CreateFrustum(proj, view, 1.75f, frustum, offset);

    if (!CollideWithWorld(frustum, flags & 0xFFFCFFFF, hit) && !collided) {
        return 0;
    }

    distance -= hit * range;

    if (distance < 0.0f) {
        distance = 0.0f;
    }

    return 1;
}

// VERIFY: fd0d5ecb manual
// OFFSET: 0x006004B0
static int32_t __stdcall ComputeShakeFalloff(const C3Vector& position, const C3Vector& shakePosition, float& amplitude, float start, float startSqr, float max, float maxSqr) {
    float dy = position.y - shakePosition.y;
    float dz = position.z - shakePosition.z;
    float dx = position.x - shakePosition.x;
    float distSqr = dx * dx + dz * dz + dy * dy;

    if (maxSqr < distSqr) {
        return 0;
    }

    if (startSqr < distSqr) {
        amplitude = static_cast<float>(pow(0.699999988079071, (sqrtf(distSqr) - start) / start)) * amplitude;
    }

    return 1;
}

// VERIFY: 6b01b912 manual
// OFFSET: 0x005FE6C0
void CGCamera::AddShakeOffset(C3Vector& offset, float facing, float amplitude, float time, float frequency, CGCameraShakeType type, float damping, CGCameraDir dir) {
    float value = sinf(frequency * time * TWO_PI) * amplitude;

    if (type == CAMSHAKE_DECAYED_SINE) {
        value = value * expf(-(time * damping));
    }

    if (dir == CAMERA_FORWARD) {
        float c = cosf(facing);
        float s = sinf(facing);
        offset.x = c * value + offset.x;
        offset.y = value * s + offset.y;
    } else if (dir == CAMERA_RIGHT) {
        float angle = facing + 1.5707964f;
        float c = cosf(angle);
        float s = sinf(angle);
        offset.x = c * value + offset.x;
        offset.y = value * s + offset.y;
    } else if (dir == CAMERA_UP) {
        offset.z = value + offset.z;
    }
}

// VERIFY: 1bac5159 manual
// OFFSET: 0x00606970
void CGCamera::RunShakes(const C3Vector& position, C3Vector& offset) {
    if (!this->m_cameraShakeList.Head()) {
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    float facing = unit->GetSmoothFacing();
    float amplitudes[NUM_CAMERA_DIRECTIONS] = { 0.0f, 0.0f, 0.0f };
    float times[NUM_CAMERA_DIRECTIONS] = { 0.0f, 0.0f, 0.0f };
    CameraShake* shakes[NUM_CAMERA_DIRECTIONS] = { nullptr, nullptr, nullptr };
    CameraShake* next;

    for (CameraShake* shake = this->m_cameraShakeList.Head(); !this->m_cameraShakeList.IsEnd(shake); shake = next) {
        next = this->m_cameraShakeList.RawNext(shake);
        float time = (now - shake->m_startTime) * 0.001f + shake->m_timeOffset;

        if (time < shake->m_duration) {
            float amplitude = shake->m_amplitude;

            if (ComputeShakeFalloff(position, shake->m_position, amplitude, CAMERASHAKEMINDISTANCE, s_shakeFalloffStartSqr, CAMERASHAKEMAXDISTANCE, s_shakeFalloffMaxSqr) && amplitude > amplitudes[shake->m_dir]) {
                amplitudes[shake->m_dir] = amplitude;
                shakes[shake->m_dir] = shake;
                times[shake->m_dir] = time;
            }
        } else {
            this->m_cameraShakeList.DeleteNode(shake);
        }
    }

    for (uint32_t i = 0; i < NUM_CAMERA_DIRECTIONS; i++) {
        CameraShake* shake = shakes[i];

        if (shake) {
            CGCamera::AddShakeOffset(offset, facing, amplitudes[i], times[i], shake->m_frequency, shake->m_type, shake->m_damping, shake->m_dir);
        }
    }
}

// VERIFY: 1192d076 manual
// OFFSET: 0x006053D0
void CGCamera::SetTelescope(int32_t telescope) {
    if (!telescope) {
        if (this->m_state & 0x8) {
            float offset = this->m_smoothTargetOffset.m_target;
            uint32_t time = OsGetAsyncTimeMs();
            this->StartSmoothFoV(0.0f, 0.5f, time);
            this->m_state = (this->m_state & 0x7FFF7FF7) | 0x80000000;
            this->SetView(this->m_prevViewIndex, 2, 2);
            this->StartSmoothDistance(this->m_preTelescopeDistance, 0.5f, time);

            if (fabsf(this->m_telescopePitchOffset) >= 2.3841858e-7f) {
                this->StartSmoothTargetOffset(offset - this->m_telescopePitchOffset, 0.5f, time);
                this->m_telescopePitchOffset = 0.0f;
            }
        }

        return;
    }

    uint32_t time = OsGetAsyncTimeMs();
    this->SetView(0, 2, 1);
    this->m_state = (this->m_state & 0x7FFF7FF7) | 0x8008;
    this->StartSmoothFoV(telescope * 0.017453292f - this->m_fov, 0.5f, time);
    this->StartSmoothDistance(0.0f, 0.5f, time);

    if (fabsf(this->m_targetOffset) < 2.3841858e-7f) {
        return;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(this->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (!target) {
        return;
    }

    C33Matrix facing = this->m_facing;
    C3Vector forward = this->Forward();
    C3Vector aim(forward.x * 100.0f + this->m_position.x, this->m_position.y + forward.y * 100.0f, this->m_position.z + forward.z * 100.0f);

    float yaw;
    float pitch;
    float roll;
    this->UpdateTargetFacing(target, yaw, pitch, roll);

    float distance = this->m_distance;
    C3Vector back = this->Forward();
    C3Vector eye(back.x * distance + this->m_position.x, this->m_position.y + back.y * distance, this->m_position.z + back.z * distance);
    this->m_facing = facing;

    C3Vector dir(aim.x - eye.x, aim.y - eye.y, aim.z - eye.z);
    dir.NormalizeSafe();

    float offset = this->m_smoothTargetOffset.m_target;
    this->m_targetOffset = offset;
    float base = offset + this->m_pitch;

    if (this->m_groundTiltRefs <= 0) {
        base = base + this->m_groundTilt;
    }

    float angle = UnwrapAngleToward(0.0f, -atan2f(dir.z, sqrtf(1.0f - dir.z * dir.z)) - base, -PI, PI);
    this->StartSmoothTargetOffset(angle + offset, 0.5f, time);
    this->m_telescopePitchOffset = angle;
}

// VERIFY: 6665d6c2 auto
// OFFSET: 0x005FF440
void CGCamera::SetPositionAndTargetWithRoll(const C3Vector& position, const C3Vector& target, float roll) {
    C3Vector forward(target.x - position.x, target.y - position.y, target.z - position.z);
    float lengthSqr = forward.z * forward.z + forward.y * forward.y + forward.x * forward.x;

    if (lengthSqr > 2.3841858e-7f) {
        float inv = 1.0f / sqrtf(lengthSqr);
        forward.x *= inv;
        forward.y *= inv;
        forward.z *= inv;
    }

    float c = cosf(roll);
    float s = sinf(roll);
    C3Vector up(0.0f, s, c);
    this->m_position = position;
    this->SetFacing(forward, up);
}

// VERIFY: beb906c7 auto
// OFFSET: 0x006018C0
int32_t CGCamera::FinishLoadingModel() {
    this->m_state |= 0x4;
    this->m_someTime = OsGetAsyncTimeMs();
    this->GetScene()->SetTime(0);
    this->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 0, 1);
    this->m_model->SetAnimating(1);
    C3Vector cameraPos;
    this->GetScene()->Animate(cameraPos);
    void* camera = this->m_model->GetCameraByIndex(0);

    if (!camera) {
        this->m_model->Release();
        this->m_model = nullptr;
        return 0;
    }

    this->m_modelCameraHandle = HandleDuplicate(static_cast<HOBJECT>(camera));
    C3Vector position;
    C3Vector target;
    DataMgrGetCoord(reinterpret_cast<CDataMgr*>(this->m_modelCameraHandle), 7, &position);
    DataMgrGetCoord(reinterpret_cast<CDataMgr*>(this->m_modelCameraHandle), 8, &target);
    this->SetPositionAndTargetWithRoll(position, target, CameraGetFloat(reinterpret_cast<CCamera*>(this->m_modelCameraHandle), 5));
    return 1;
}

// VERIFY: 580b1867 auto
// OFFSET: 0x00601E90
void CGCamera::CalcModelCamera(uint32_t time) {
    if (!(this->m_state & 0x4) && !(this->m_model->IsLoaded(0, 0) && this->FinishLoadingModel())) {
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();
    this->m_model->SetAnimating(1);
    this->GetScene()->AdvanceTime(now - this->m_someTime);
    C3Vector cameraPos;
    this->GetScene()->Animate(cameraPos);
    C3Vector position;
    C3Vector target;
    this->m_someTime = now;
    DataMgrGetCoord(reinterpret_cast<CDataMgr*>(this->m_modelCameraHandle), 7, &position);
    DataMgrGetCoord(reinterpret_cast<CDataMgr*>(this->m_modelCameraHandle), 8, &target);
    this->SetPositionAndTargetWithRoll(position, target, CameraGetFloat(reinterpret_cast<CCamera*>(this->m_modelCameraHandle), 5));
}

// VERIFY: 399367f6 auto
// OFFSET: 0x006038A0
void CGCamera::CommentatorSetViewAndPosition(const C3Vector& position, float facing, float pitch) {
    this->m_views[6].m_distance = 0.0f;
    this->m_views[6].m_yaw = facing;
    this->m_views[6].m_pitch = pitch;
    this->m_position = position;
    this->m_position = position;
    this->SetView(6, 2, 0);
}

// VERIFY: fcc9622c auto
// OFFSET: 0x005FF3E0
void CGCamera::RestoreSavedDistancePitch() {
    float distance = s_cvCameraSavedDistance->m_floatValue;

    if (distance < 0.0f) {
        distance = 0.0f;
    } else if (distance >= 50.0f) {
        distance = 50.0f;
    }

    this->m_distance = distance;
    this->m_smoothDistance.m_target = distance;

    float pitch = s_cvCameraSavedPitch->m_floatValue * 0.017453292f;
    this->m_pitch = pitch;
    this->m_smoothPitch.m_target = pitch;
}

// VERIFY: e29a855f auto
// OFFSET: 0x004D5EE0
float CGCamera::GetDefaultZoomMinDistance() {
    return 0.0f;
}

// VERIFY: 26a4f582 manual
// OFFSET: 0x00606410
void CGCamera::AddShakeById(int32_t shakeId, const C3Vector* position) {
    CameraShakesRec* shake = g_cameraShakesDB.GetRecord(shakeId);

    if (shake) {
        this->AddShake(*position, shake->m_shakeType, static_cast<CGCameraDir>(shake->m_direction), shake->m_amplitude * 0.027777778f, shake->m_frequency, shake->m_duration, shake->m_phase, shake->m_coefficient);
    }
}

// VERIFY: 8de48629 auto
// OFFSET: 0x005FFFD0
float CameraEaseInOut(float t) {
    return 0.5f - CMath::cos_(t * PI) * 0.5f;
}

// VERIFY: f1d24674 auto
// OFFSET: 0x006006A0
void CGCamera::UpdateVehicleYawOffset(uint32_t time) {
    float t = static_cast<float>(time - this->m_vehicleYawStartTime) / static_cast<float>(this->m_vehicleYawEndTime - this->m_vehicleYawStartTime);

    if (t > 1.0f) {
        this->m_vehicleYawStartTime = this->m_vehicleYawEndTime;
        this->m_vehicleYawOffset = this->m_vehicleYawTo;
    } else {
        this->m_vehicleYawOffset = CameraEaseInOut(t) * (this->m_vehicleYawTo - this->m_vehicleYawFrom) + this->m_vehicleYawFrom;
    }
}

// VERIFY: 1b6d5b25 auto
// OFFSET: 0x006007B0
void CGCamera::UpdateVehiclePitchOffset(uint32_t time) {
    float t = static_cast<float>(time - this->m_vehiclePitchStartTime) / static_cast<float>(this->m_vehiclePitchEndTime - this->m_vehiclePitchStartTime);

    if (t > 1.0f) {
        this->m_vehiclePitchStartTime = this->m_vehiclePitchEndTime;
        this->m_vehiclePitchOffset = this->m_vehiclePitchTo;
    } else {
        this->m_vehiclePitchOffset = CameraEaseInOut(t) * (this->m_vehiclePitchTo - this->m_vehiclePitchFrom) + this->m_vehiclePitchFrom;
    }
}

// VERIFY: cf40d06b manual
// OFFSET: 0x00605700
void CGCamera::FaceTarget(CGObject_C* target, uint32_t time) {
    if (!target) {
        SErrSetLastError(87);
        return;
    }

    if (!(this->m_state & 0x4) && !this->FinishLoadingTarget(target)) {
        return;
    }

    C3Vector direction(0.0f, 0.0f, 0.0f);
    direction = this->GetSafeWorldPos(target);
    direction.x = direction.x - this->m_position.x;
    direction.y = direction.y - this->m_position.y;
    direction.z = this->m_targetHeightFar + direction.z - this->m_position.z;
    direction.NormalizeSafe();
    this->SetFacing(direction);
}
