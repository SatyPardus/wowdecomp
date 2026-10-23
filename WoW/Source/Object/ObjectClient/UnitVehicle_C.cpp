#include "UnitVehicle_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehicleCamera_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// VERIFY: bc21d620 manual
// OFFSET: 0x0074C650
CGUnit_C* CGUnit_C::FindRootVehicle(CGUnit_C* stop) {
    CGUnit_C* unit = this;

    if (this == stop && this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        return this;
    }

    WOWGUID guid = { this->GetTransportGUID() };
    uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);

    if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid)))) {
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return nullptr;
        }

        while (unit != stop) {
            guid.m_guid = unit->GetTransportGUID();
            high = static_cast<uint32_t>(guid.m_guid >> 32);

            if ((high & 0xF0F00000) != 0xF0500000 && ((high & 0xF0000000) || !((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid)))) {
                break;
            }

            unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

            if (!unit) {
                return nullptr;
            }
        }

        return unit;
    }

    if (unit->m_vehicle && unit->m_vehicle->m_vehicleRec) {
        return unit;
    }

    return nullptr;
}

// VERIFY: 26588931 manual
// OFFSET: 0x0074CC40
CGUnit_C* CGUnit_C::FindRootVehicleOfPassengerChain(CGUnit_C* stop) {
    CGUnit_C* unit = this;
    WOWGUID guid = this->m_vehiclePassenger ? this->m_vehiclePassenger->m_transitionVehicleGuid : WOWGUID();

    if (!guid) {
        if (!this->m_vehicle || !this->m_vehicle->m_vehicleRec) {
            return nullptr;
        }

        return unit;
    }

    unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return nullptr;
    }

    while (unit != stop) {
        guid = unit->m_vehiclePassenger ? unit->m_vehiclePassenger->m_transitionVehicleGuid : WOWGUID();

        if (!guid) {
            break;
        }

        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return nullptr;
        }
    }

    return unit;
}

// VERIFY: 9db634f7 manual
// OFFSET: 0x0074B900
bool CGUnit_C::IsVehiclePreventingTurning() const {
    uint32_t high = static_cast<uint32_t>(this->m_movementData.m_transportGuid.m_guid >> 32);
    uint32_t low = static_cast<uint32_t>(this->m_movementData.m_transportGuid.m_guid);

    if ((high & 0xF0F00000) != 0xF0500000 && ((high & 0xF0000000) || !((high & 0xF07FFFFF) | low))) {
        return false;
    }

    CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ static_cast<uint64_t>(high) << 32 | low }, TYPE_UNIT, __FILE__, __LINE__));

    if (!transport || !transport->m_vehicle) {
        return false;
    }

    VehicleSeatRec* seat = transport->m_vehicle->GetVehicleSeatRec(static_cast<uint8_t>(this->m_movementData.m_transportSeat));
    return seat && !(seat->m_flags & 0x400);
}

// VERIFY: 08c11781 manual
// OFFSET: 0x0074BA40
bool CGUnit_C::IsAlteredFormTransitionPreventingMovement() const {
    if (!this->IsLocalClientControlled()) {
        return false;
    }

    CVehiclePassenger_C* passenger = this->m_vehiclePassenger;

    if (passenger && passenger->m_state && passenger->m_state != 3) {
        return true;
    }

    if (passenger && (passenger->m_flags & 0x200)) {
        return true;
    }

    CGUnit_C* charmer = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_unit->m_unitFieldCharmedby, TYPE_UNIT, __FILE__, __LINE__));

    if (charmer) {
        CVehiclePassenger_C* charmerPassenger = charmer->m_vehiclePassenger;

        if (!charmerPassenger || ((!charmerPassenger->m_state || charmerPassenger->m_state == 3) && !(charmerPassenger->m_flags & 0x200))) {
            return charmer->IsInVehicle() && (charmerPassenger->m_flags & 0x8);
        }

        return true;
    }

    if (!(this->m_unit->m_unitFieldFlags & 0x1000000) && this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return true;
    }

    return false;
}

// VERIFY: 97008da9 manual
// OFFSET: 0x0074C550
void CGUnit_C::ConvertVehicleCameraRelativeFacingToTransportRelativeFacing(float& facing) {
    if (this->m_vehicleCamera) {
        CGObject_C* relativeTo = ClntObjMgrObjectPtr(WOWGUID{ this->m_vehicleCamera->GetRelativeTo() }, TYPE_OBJECT, __FILE__, __LINE__);
        CVehicleCamera_C::ConvertFromRawToWorld(facing, relativeTo);
        facing = this->m_movement->GetRawFacing(facing);
    }
}

// VERIFY: a5b201a2 manual
// OFFSET: 0x0074C5A0
void CGUnit_C::UpdateVehicleUISliders(CGUnit_C* unit, const VehicleRec* vehicleRec) {
    int32_t enable = unit && !unit->IsAlteredFormTransitionPreventingMovement();

    if (enable && vehicleRec && (vehicleRec->m_flags & 0x400)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_ANGLE_SHOW, "%d", 1);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_ANGLE_SHOW, nullptr);
    }

    if (enable && vehicleRec && (vehicleRec->m_flags & 0x800)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_POWER_SHOW, "%d", 1);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_POWER_SHOW, nullptr);
    }

    if (unit) {
        CGInputControl::GetActive();
        CGInputControl::SendUIVehicleAngleUpdate(unit->GetPitch());
    }
}

// VERIFY: 021f7c45 manual
// OFFSET: 0x0074BCB0
int32_t CGUnit_C::VehiclePassengerShouldWaitForVehicleTransitionAnim(uint64_t transport, uint8_t seat) const {
    if (transport == this->GetTransportGUID() && seat == this->m_movementData.m_transportSeat) {
        return 0;
    }

    uint32_t high = static_cast<uint32_t>(transport >> 32);
    bool entering = (high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(transport)));
    WOWGUID guid;

    if (entering) {
        guid.m_guid = transport;
    } else {
        if (!this->HasVehicleTransport()) {
            return 0;
        }

        guid.m_guid = this->GetTransportGUID();
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    CVehicle_C* vehicle = unit->m_vehicle;

    if (!vehicle) {
        return 0;
    }

    VehicleSeatRec* seatRec = vehicle->GetVehicleSeatRec(entering ? seat : this->m_movementData.m_transportSeat);

    if (!seatRec) {
        return 0;
    }

    CVehicle_C* animVehicle = vehicle->FindVehicleToPlayBoneAnim();

    if (entering) {
        if (!(seatRec->m_flags & 0x400000)) {
            return 0;
        }

        uint32_t anim = seatRec->m_vehicleEnterAnim;

        if (anim > 0x1F9) {
            return 0;
        }

        return animVehicle->CanPlayBoneAnim(anim) ? 1 : 0;
    }

    if (!(seatRec->m_flags & ((this->m_movementData.m_flags2 & 0x40) ? 0x40000 : 0x80000))) {
        return 0;
    }

    uint32_t anim = seatRec->m_vehicleExitAnim;
    return anim <= 0x1F9 && animVehicle->CanPlayBoneAnim(anim);
}

// VERIFY: 0d563d24 manual
// OFFSET: 0x0074BE10
int32_t CGUnit_C::VehiclePassengerStartWaitingForVehicleTransitionAnim(uint64_t transport, uint8_t seat) {
    uint32_t high = static_cast<uint32_t>(transport >> 32);
    int32_t entering = (high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(transport)));
    WOWGUID guid;

    if (entering) {
        guid.m_guid = transport;
    } else {
        if (!this->HasVehicleTransport()) {
            return 0;
        }

        guid.m_guid = this->GetTransportGUID();
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    CVehicle_C* vehicle = unit->m_vehicle;

    if (!vehicle) {
        return 0;
    }

    VehicleSeatRec* seatRec = vehicle->GetVehicleSeatRec(entering ? seat : this->m_movementData.m_transportSeat);

    if (!seatRec) {
        return 0;
    }

    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->StopWaitingForVehicleTransitionAnim();
    }

    CVehicle_C* animVehicle = vehicle->FindVehicleToPlayBoneAnim();

    if (entering) {
        if (!(seatRec->m_flags & 0x400000) || static_cast<uint32_t>(seatRec->m_vehicleEnterAnim) > 0x1F9 || !animVehicle->PlayBoneAnim(seatRec->m_field33512213034, seatRec->m_vehicleEnterAnim)) {
            return 0;
        }

        animVehicle->InstallBoneAnimListener(this->m_obj->m_guid, seatRec->m_field33512213034, &CVehiclePassenger_C::OnVehicleTransitionAnimEnd);

        if (seatRec->m_flagsB & 0x800000) {
            animVehicle->SetBoneAnimForceFacing(this->m_obj->m_guid, seatRec->m_field33512213034);
        }
    } else {
        if (!(seatRec->m_flags & ((this->m_movementData.m_flags2 & 0x40) ? 0x40000 : 0x80000)) || static_cast<uint32_t>(seatRec->m_vehicleExitAnim) > 0x1F9 || !animVehicle->PlayBoneAnim(seatRec->m_vehicleExitAnimBone, seatRec->m_vehicleExitAnim)) {
            return 0;
        }

        if (this->m_vehiclePassenger) {
            this->m_vehiclePassenger->StopBoneAnimIfSameBone(animVehicle->m_unit->m_obj->m_guid, seatRec);
        }

        animVehicle->InstallBoneAnimListener(this->m_obj->m_guid, seatRec->m_vehicleExitAnimBone, &CVehiclePassenger_C::OnVehicleTransitionAnimEnd);
    }

    if (!this->m_vehiclePassenger) {
        this->m_vehiclePassenger = CVehiclePassenger_C::Alloc(this);
    }

    this->m_vehiclePassenger->StartWaitingForVehicleTransitionAnim(WOWGUID{ transport }, seat, seatRec, entering, animVehicle);
    return 1;
}

// VERIFY: 65e43ba1 manual
// OFFSET: 0x0074CD60
void CGUnit_C::DestroyVehicleCamera() {
    if (!this->m_vehicleCamera) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleCamera->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));
    CVehicleCamera_C* camera = this->m_vehicleCamera;
    camera->Destroy();
    this->m_vehicleCamera = nullptr;
    CGCamera* activeCamera = CGWorldFrame::GetActiveCamera();

    if (activeCamera && activeCamera->m_vehicleCamera == camera) {
        activeCamera->OnVehicleCameraDestroyed();
    }

    if (unit) {
        unit->OnVehicleCameraPossiblyUnneeded();
    }
}

// VERIFY: 404e4e04 auto
// OFFSET: 0x004F6230
CGUnit_C* CGUnit_C::GetRootVehicleUnit() {
    if (this->m_vehiclePassenger) {
        return this->m_vehiclePassenger->GetStoredRootVehicleUnit();
    }

    return this->FindRootVehicle(nullptr);
}

// VERIFY: 218b4093 auto
// OFFSET: 0x0074B9C0
void CGUnit_C::PlayVehicleExitEndAnimation(const VehicleSeatRec* seat, uint32_t anim) {
    if (!seat || !(seat->m_flags & 0x200000) || anim == 0xFFFFFFFF) {
        uint32_t flags = this->m_movement->m_flags;

        if (flags & 0x1000) {
            this->PlayBaseAnimation(ANIM_FALL, 0);
            this->m_animationState |= 0x1000000;
            return;
        }

        if (flags & 0x200000) {
            this->PlayBaseAnimation(ANIM_SWIM_IDLE, 0);
            return;
        }

        if ((flags & 0x2000000) || anim == 0xFFFFFFFF) {
            return;
        }
    }

    this->PlayBaseAnimation(anim, 1);
}

// VERIFY: d8cede4e auto
// OFFSET: 0x0074C4E0
void CGUnit_C::OnVehicleCameraPossiblyUnneeded() {
    CVehicleCamera_C* camera = this->m_vehicleCamera;

    if (!camera || static_cast<int8_t>(camera->m_flags) < 0) {
        return;
    }

    CGCamera* active = CGWorldFrame::GetActiveCamera();

    if (active) {
        CVehicleCamera_C* current = active->m_vehicleCamera;

        while (current) {
            if (current == this->m_vehicleCamera) {
                return;
            }

            if (!current->m_nextCameraGuid) {
                break;
            }

            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(current->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (!unit) {
                break;
            }

            current = unit->m_vehicleCamera;
        }
    }

    this->DestroyVehicleCamera();
}

// VERIFY: 6c23ff21 auto
// OFFSET: 0x0074CF30
void CGUnit_C::VehiclePassengerOnUpdateTransport(WOWGUID transport, uint8_t seat, int32_t a3) {
    if (!(this->m_clientFlags & 0x80000) && !a3) {
        return;
    }

    CGUnit_C* root = this->m_vehiclePassenger ? this->m_vehiclePassenger->GetStoredRootVehicleUnit() : this->FindRootVehicle(nullptr);

    if (root) {
        root->SetMaxAlpha(1.0f);
    }

    WOWGUID guid = transport;
    uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);
    CGUnit_C* vehicle;

    if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid)))) {
        vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (!vehicle) {
            guid = WOWGUID{ 0 };
            seat = 0xFF;

            if (!this->m_vehiclePassenger || !this->m_vehiclePassenger->m_transitionVehicleGuid) {
                return;
            }
        }
    } else {
        vehicle = nullptr;
    }

    if (!this->m_vehiclePassenger) {
        if (!guid) {
            return;
        }

        this->m_vehiclePassenger = CVehiclePassenger_C::Alloc(this);
    }

    this->m_vehiclePassenger->OnUpdateTransport(guid, seat, vehicle, a3);

    if (this->m_vehicleCamera) {
        this->m_vehicleCamera->BeginTransition(FrameTime::s_curTimeMs);

        if (!this->m_vehicleCamera->m_state) {
            this->DestroyVehicleCamera();
        }
    } else {
        this->OnVehicleCameraPossiblyNeeded();
    }
}

// VERIFY: c6051623 auto
// OFFSET: 0x0074B9A0
uint32_t CGUnit_C::IsStrafingDisabled() const {
    return this->m_movementData.m_flags2 & MOVEMENTFLAG2_NO_STRAFE;
}

// VERIFY: dcad4d49 auto
// OFFSET: 0x0074B9B0
void CGUnit_C::SetVehicleExitVoluntary(int32_t enable) {
    this->m_movementData.SetVehicleExitVoluntary(enable);
}

// VERIFY: 5d120117 auto
// OFFSET: 0x0074BB20
void CGUnit_C::OnDismissControlledVehicleLocal(uint32_t eventTime) {
    this->m_movementData.OnDismissControlledVehicleLocal(eventTime);
}

// VERIFY: 26b35972 auto
// OFFSET: 0x0074BB30
void CGUnit_C::OnChangeSeatsOnControlledVehicleLocal(uint32_t eventTime, WOWGUID transport, uint8_t seat) {
    this->m_movementData.OnChangeSeatsOnControlledVehicleLocal(eventTime, transport, seat);
}

// VERIFY: 561780c9 auto
// OFFSET: 0x0074BB60
int32_t CGUnit_C::ShouldUseVehicleSeatExitAnimation(const VehicleSeatRec* seat) const {
    return seat->m_flags & ((this->m_movementData.m_flags2 & 0x40) ? 0x8 : 0x8000);
}

// VERIFY: d8212e45 auto
// OFFSET: 0x0074BB90
uint32_t CGUnit_C::IsVehicleCurrentlyUnstoppable() const {
    VehicleRec* vehicle = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;

    if (!vehicle) {
        return 0;
    }

    return vehicle->m_flags & ((this->m_movement->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) ? 0x100 : 0x80);
}

// VERIFY: 2533cf6d auto
// OFFSET: 0x0074BBD0
void CGUnit_C::SignalPlayerGainsVehicleDataEvent() {
    VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;
    WOWGUID guid = this->m_obj->m_guid;
    int32_t count;
    char** tokens = Script_GetTokenIDsFromGUID(guid, &count);

    for (int32_t i = 0; i < count; i++) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_GAINS_VEHICLE_DATA, "%s%d", tokens[i], vehicleRec ? vehicleRec->m_vehicleUiindicatorID : 0);
    }
}

// VERIFY: abf1b455 auto
// OFFSET: 0x0074BC50
void CGUnit_C::SignalPlayerLosesVehicleDataEvent() {
    WOWGUID guid = this->m_obj->m_guid;
    int32_t count;
    char** tokens = Script_GetTokenIDsFromGUID(guid, &count);

    for (int32_t i = 0; i < count; i++) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_LOSES_VEHICLE_DATA, "%s", tokens[i]);
    }
}

// VERIFY: a7438679 auto
// OFFSET: 0x0074C040
int32_t CGUnit_C::DeferVehicleTransitionSpline(CDataStore* msg, uint64_t transport, uint8_t seat) {
    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->DeleteAllDeferredVehicleTransitionAnimSplines();
        this->m_vehiclePassenger->StopWaitingForVehicleTransitionAnim();
    }

    int32_t result = this->VehiclePassengerShouldWaitForVehicleTransitionAnim(transport, seat);

    if (!result) {
        return result;
    }

    this->m_movementData.FlushMoveQueue(0, 0);

    if (!this->m_vehiclePassenger) {
        this->m_vehiclePassenger = CVehiclePassenger_C::Alloc(this);
    }

    this->m_vehiclePassenger->StartDeferringVehicleTransitionAnimSpline(msg);

    if (!this->VehiclePassengerStartWaitingForVehicleTransitionAnim(transport, seat)) {
        this->m_vehiclePassenger->ExecuteAllDeferredVehicleTransitionAnimSplines();
    }

    return 1;
}

// VERIFY: cd9f8160 manual
// OFFSET: 0x0074C0E0
void CGUnit_C::VehiclePassengerUpdateWorldCameraState(VehicleRec* vehicleRec, VehicleSeatRec* seatRec, uint32_t startTime, uint32_t endTime) {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    uint32_t active = 0;

    if (this->m_vehiclePassenger && (this->m_vehiclePassenger->m_state == 1 || this->m_vehiclePassenger->m_state == 2 || this->m_vehiclePassenger->m_state == 3)) {
        active = 1;
    }

    if (CGUnit_C::s_vehiclePassengerCameraActive == active && CGUnit_C::s_vehiclePassengerCameraVehicle == vehicleRec && CGUnit_C::s_vehiclePassengerCameraSeat == seatRec) {
        return;
    }

    uint32_t restoreZoom = camera->m_flags & 0x20;
    CGUnit_C::s_vehiclePassengerCameraVehicle = vehicleRec;
    CGUnit_C::s_vehiclePassengerCameraActive = active;
    CGUnit_C::s_vehiclePassengerCameraSeat = seatRec;
    int32_t resetMin = 1;
    int32_t resetMax = 1;
    float duration = static_cast<float>(endTime - startTime) * 0.001f;

    if (!active) {
        camera->SetVehicleZoomEnabled(0, duration, restoreZoom == 0);
        camera->SetDesiredVehicleYawOffset(0.0f, startTime, endTime);
        camera->SetDesiredVehiclePitchOffset(0.0f, startTime, endTime);
        camera->SetVehicleTrackingStyle(0);
    } else {
        if (seatRec && (seatRec->m_flags & 0x1000000)) {
            camera->SetVehicleZoomEnabled(1, duration, 1);
            restoreZoom = 0;
        } else if (seatRec && (seatRec->m_flagsB & 0x400)) {
            camera->SetVehicleZoomEnabled(0, duration, 0);
            float current = camera->m_distance;
            float target = current;
            uint32_t flagsB = seatRec->m_flagsB;
            int32_t hasMin = flagsB & 0x4000;
            int32_t hasMax = flagsB & 0x8000;
            uint32_t wasRestore = restoreZoom;
            restoreZoom = 0;
            float minDistance = hasMin ? seatRec->m_field33512213054 : camera->GetDefaultZoomMinDistance();
            float maxDistance = hasMax ? seatRec->m_field33512213055 : CGCamera::GetDefaultZoomMaxDistance();

            if (seatRec->m_flagsB & 0x800) {
                float preferred = seatRec->m_field33512213053;

                if (preferred > current && !(seatRec->m_flagsB & 0x1000)) {
                    target = preferred;
                }

                if (preferred < target && !(seatRec->m_flagsB & 0x2000)) {
                    target = preferred;
                }

                if (!hasMin && minDistance > preferred) {
                    hasMin = 1;
                    minDistance = preferred;
                }

                if (!hasMax && maxDistance < preferred) {
                    hasMax = 1;
                    maxDistance = preferred;
                }
            }

            if (hasMin) {
                resetMin = 0;
                camera->SetVehicleSeatZoomMinDistance(minDistance);

                if (minDistance > target) {
                    target = minDistance;
                }
            }

            if (hasMax) {
                resetMax = 0;
                camera->SetVehicleSeatZoomMaxDistance(maxDistance);

                if (maxDistance < target) {
                    target = maxDistance;
                }
            }

            if (fnotequalz(current, target, 0.001f)) {
                if (!wasRestore) {
                    camera->SetVehicleExitZoomDistance(current);
                }

                camera->ZoomToDistance(target, startTime, duration, 1);
            }
        } else {
            camera->SetVehicleZoomEnabled(0, duration, restoreZoom == 0);
        }

        camera->SetDesiredVehicleYawOffset(vehicleRec ? vehicleRec->m_cameraYawOffset : 0.0f, startTime, endTime);
        camera->SetDesiredVehiclePitchOffset(vehicleRec ? vehicleRec->m_cameraPitchOffset : 0.0f, startTime, endTime);

        if (seatRec && (seatRec->m_flags & 0x800) && (seatRec->m_flagsB & 0x80001)) {
            camera->SetVehicleTrackingStyle(1);
        } else {
            camera->SetVehicleTrackingStyle(2);
        }
    }

    if (restoreZoom) {
        camera->ZoomToDistance(camera->m_vehicleExitZoomDistance, startTime, duration, 1);
        camera->ClearVehicleExitZoomDistance();
    }

    if (resetMin) {
        camera->SetVehicleSeatZoomMinDistanceToDefault();
    }

    if (resetMax) {
        camera->SetVehicleSeatZoomMaxDistanceToDefault();
    }
}

// VERIFY: 396eb2a7 auto
// OFFSET: 0x0074C4A0
void CGUnit_C::VehiclePassengerUpdateWorldCameraStateInstantly() {
    CVehiclePassenger_C* passenger = this->m_vehiclePassenger;
    VehicleRec* vehicleRec = passenger ? passenger->m_vehicleRec : nullptr;

    if (passenger) {
        this->VehiclePassengerUpdateWorldCameraState(vehicleRec, passenger->m_seatRec, FrameTime::s_curTimeMs, FrameTime::s_curTimeMs);
    } else {
        this->VehiclePassengerUpdateWorldCameraState(vehicleRec, nullptr, FrameTime::s_curTimeMs, FrameTime::s_curTimeMs);
    }
}

// VERIFY: 51ad0765 auto
// OFFSET: 0x0074C750
void CGUnit_C::CreateVehicleData(const CClientObjCreate* create, int32_t vehicleId) {
    if (this->m_vehicle) {
        this->m_vehicle->ReceiveVehicleRecIDFromServer(vehicleId);
        return;
    }

    this->m_vehicle = CVehicle_C::Create(create, this, vehicleId);

    if ((this->m_modelFlags & 0x40000) && this->IsActivePlayer()) {
        this->SignalPlayerGainsVehicleDataEvent();
    }
}

// VERIFY: d3f8bddf auto
// OFFSET: 0x0074C7B0
void CGUnit_C::DestroyVehicleData(int32_t removePassengers) {
    if (!this->m_vehicle) {
        return;
    }

    this->m_vehicle->Destroy(removePassengers);
    this->m_vehicle = nullptr;

    if (this->IsActivePlayer()) {
        this->SignalPlayerLosesVehicleDataEvent();
    }
}

// VERIFY: ab8a24d3 manual
// OFFSET: 0x0074C7F0
void CGUnit_C::VehicleExit() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && player && player->GetTransportGUID() == this->m_obj->m_guid.m_guid) {
        CGPetInfo::PetDismiss();
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_VEHICLE_EXIT));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d32caa20 manual
// OFFSET: 0x0074C8B0
void CGUnit_C::VehiclePrevSeat() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID guid = this->m_obj->m_guid;

    if (guid == CGUnit_C::s_activeMover && player && player->GetTransportGUID() == guid.m_guid) {
        if (player->m_vehiclePassenger) {
            player->m_vehiclePassenger->StopMovement();
        }

        this->m_movementData.OnChangeSeatsOnControlledVehicleLocal(FrameTime::s_curTimeMs, WOWGUID(), 0xFF);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_VEHICLE_PREV_SEAT));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 55253920 manual
// OFFSET: 0x0074C9A0
void CGUnit_C::VehicleNextSeat() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID guid = this->m_obj->m_guid;

    if (guid == CGUnit_C::s_activeMover && player && player->GetTransportGUID() == guid.m_guid) {
        if (player->m_vehiclePassenger) {
            player->m_vehiclePassenger->StopMovement();
        }

        this->m_movementData.OnChangeSeatsOnControlledVehicleLocal(FrameTime::s_curTimeMs, WOWGUID(), 1);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_VEHICLE_NEXT_SEAT));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: a64bdd42 manual
// OFFSET: 0x0074CA90
int32_t CGUnit_C::SwitchToVehicleSeat(CGUnit_C* vehicle, int32_t virtualSeat) {
    CVehicle_C* root = vehicle->m_vehicle;
    const CVehicle_C* target;
    uint8_t seat;

    if (!root || !root->m_vehicleRec || !root->GetVehicleAndSeatFromVirtualSeat(virtualSeat, target, seat)) {
        return 0;
    }

    VehicleSeatRec* seatRec = target->GetVehicleSeatRec(seat);

    if (!seatRec || const_cast<CVehicle_C*>(target)->FindPassengerUnit(seat) || !(seatRec->m_flags & 0x4000000) || !CGInputControl::GetActive() || !CGInputControl::GetActive()->IsVehicleSeatFlagSet(0x4000000)) {
        return 0;
    }

    if (this->GetTransportGUID() != CGUnit_C::s_activeMover.m_guid) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REQUEST_VEHICLE_SWITCH_SEAT));
        SmartGUID guid;
        guid.m_guid = target->m_unit->m_obj->m_guid.m_guid;
        msg << guid;
        msg.Put(seat);
        msg.Finalize();
        ClientServices::Send2(&msg);
        return 1;
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 1;
    }

    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->StopMovement();
    }

    mover->OnChangeSeatsOnControlledVehicleLocal(FrameTime::s_curTimeMs, target->m_unit->m_obj->m_guid, seat);
    return 1;
}

// VERIFY: 29d19ef8 manual
// OFFSET: 0x0074CCE0
void CGUnit_C::CheckVehicleExitOnCast() {
    VehicleSeatRec* seat = this->m_vehiclePassenger ? this->m_vehiclePassenger->m_seatRec : nullptr;

    if (!seat || !(seat->m_flagsB & 0x100000)) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (this->m_obj->m_guid != CGUnit_C::s_activeMover || !player || player->GetTransportGUID() != this->m_obj->m_guid.m_guid) {
        this->VehicleExit();
    }
}

// VERIFY: 119d11a0 auto
// OFFSET: 0x0074CDF0
CVehicleCamera_C* CGUnit_C::CreateVehicleCamera() {
    this->DestroyVehicleCamera();
    this->m_vehicleCamera = CVehicleCamera_C::Create(this, FrameTime::s_curTimeMs);

    if (!this->m_vehicleCamera) {
        return nullptr;
    }

    if (!this->m_vehicleCamera->m_state) {
        this->DestroyVehicleCamera();
        return nullptr;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        camera->PickVehicleCamera();
    }

    return this->m_vehicleCamera;
}

// VERIFY: 7160120e manual
// OFFSET: 0x0074CE40
void CGUnit_C::OnVehicleCameraPossiblyNeeded() {
    if (!this->m_vehiclePassenger || !this->m_vehiclePassenger->m_state || this->m_vehicleCamera) {
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    CVehicleCamera_C* activeVehicleCamera = camera->m_vehicleCamera;
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (this->IsActivePlayer() || this->m_obj->m_guid == camera->m_targetGUID || (player && player->FindRootVehicleOfPassengerChain(this) == this) || (activeVehicleCamera && activeVehicleCamera->m_unit && activeVehicleCamera->m_unit != player && activeVehicleCamera->m_unit->FindRootVehicleOfPassengerChain(this) == this)) {
        this->CreateVehicleCamera();

        if (this->m_vehicleCamera && this->m_vehicleCamera != camera->m_vehicleCamera && !this->m_vehicleCamera->AttachToHierarchy()) {
            this->DestroyVehicleCamera();
        }
    }
}
