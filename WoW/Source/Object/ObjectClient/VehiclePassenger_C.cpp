#include "VehiclePassenger_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/Object.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/Object/ObjectClient/VehicleCamera_C.h"
#include "WoW/Source/Object/ObjectClient/Types.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include <cmath>

// STATIC: 0x00CA135C
uint32_t* CVehiclePassenger_C::s_passengerPool;

// STATIC: 0x00CA1360
TSGrowableArray<CVehiclePassenger_C*>* CVehiclePassenger_C::s_pendingPassengers;

// STATIC: 0x00CA1364
uint32_t CVehiclePassenger_C::s_pendingCount;

// STATIC: 0x00CA1368
CVehiclePassenger_C* CVehiclePassenger_C::s_headPassenger;

// STATIC: 0x00CA136C
CVehiclePassenger_C* CVehiclePassenger_C::s_tailPassenger;

// STATIC: 0x00CA1358
TSGrowableArray<VehiclePassengerRescueTransition>* CVehiclePassenger_C::s_rescueTransitions;

// VERIFY: c1cea68b auto
// OFFSET: 0x00747900
void CVehiclePassenger_C::ClearFlag8() {
    this->m_flags &= ~0x8;
}

// VERIFY: c6078f8d auto
// OFFSET: 0x00747910
void CVehiclePassenger_C::DeleteAllDeferredVehicleTransitionAnimSplines() {
    if (this->m_deferredTransitionSpline) {
        delete this->m_deferredTransitionSpline;
        this->m_deferredTransitionSpline = nullptr;
    }
}

// VERIFY: 31cbea9a manual
// OFFSET: 0x00747930
int32_t CVehiclePassenger_C::IsWaitingForVehicleTransitionAnimGrabEvent(uint32_t eventId) const {
    uint32_t flags = this->m_flags;
    uint8_t index = static_cast<uint8_t>(eventId >> 24);
    return (flags & 0x200) && !(flags & 0x80) && (flags & 0x100) && (index == '0' || this->m_transitionAnimSeat == index - '0' - 1);
}

// VERIFY: 3edd387b auto
// OFFSET: 0x00747980
void CVehiclePassenger_C::OnVehicleRideLoopAnimEnd(CGUnit_C* unit, int32_t& done, TSGrowableArray<CVehiclePassenger_C*>& passengers) {
    done = 1;
}

// VERIFY: 0697d6f6 auto
// OFFSET: 0x00747A30
void CVehiclePassenger_C::UpdateSmoothFacing() {
    this->m_targetFacing = this->m_unit->GetFacing();

    while (this->m_targetFacing + 3.1415927f < this->m_smoothFacing) {
        this->m_targetFacing = this->m_targetFacing + 6.2831855f;
    }

    while (this->m_targetFacing - 3.1415927f > this->m_smoothFacing) {
        this->m_targetFacing = this->m_targetFacing - 6.2831855f;
    }

    this->m_smoothFacing = (1.0f - this->m_facingBlend) * this->m_startFacing + this->m_targetFacing * this->m_facingBlend;
}

// VERIFY: 2daee16a auto
// OFFSET: 0x00747AE0
void CVehiclePassenger_C::PostRenderUpdateAll() {
    for (CVehiclePassenger_C* passenger = s_headPassenger; passenger; passenger = passenger->m_next) {
        passenger->m_flags &= ~0x1000;
    }
}

// VERIFY: 469c80f9 auto
// OFFSET: 0x00747B00
void CVehiclePassenger_C::LockWorldFacingNextFrame(float facing) {
    this->m_flags |= 0x1000;
    this->m_lockedWorldFacing = facing;
}

// VERIFY: 07557628 manual
// OFFSET: 0x00747FF0
CGUnit_C* CVehiclePassenger_C::GetVehicleUnit() const {
    if (!this->m_transitionVehicleGuid) {
        return static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionVehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    return unit;
}

// VERIFY: aa771f7c manual
// OFFSET: 0x00748170
void CVehiclePassenger_C::StartDeferringVehicleTransitionAnimSpline(CDataStore* msg) {
    CDataStore* spline = new (SMemAlloc(sizeof(CDataStore), __FILE__, __LINE__, 0)) CDataStore();
    this->m_deferredTransitionSpline = spline;

    uint32_t bytes = msg->m_size - msg->m_read;
    void* data;
    msg->GetDataInSitu(data, bytes);
    spline->PutData(data, bytes);
    this->m_deferredTransitionSpline->Finalize();
}

// VERIFY: a002108c manual
// OFFSET: 0x007481E0
int32_t CVehiclePassenger_C::IsWaitingForVehicleTransitionAnimThrowEvent(uint32_t eventId) const {
    uint32_t flags = this->m_flags;
    uint8_t index = static_cast<uint8_t>(eventId >> 24);
    return (flags & 0x200) && !(flags & 0x80) && !(flags & 0x100) && (index == '0' || static_cast<uint8_t>(this->m_unit->m_movementData.m_transportSeat) == index - '0' - 1);
}

// VERIFY: 1f5a0a4e auto
// OFFSET: 0x007484C0
float CVehiclePassenger_C::GetLockedWorldFacing() const {
    if (this->m_flags & 0x1000) {
        return this->m_lockedWorldFacing;
    }

    return this->m_unit->GetFacing();
}

// VERIFY: 02fee933 auto
// OFFSET: 0x007488F0
CVehiclePassenger_C::CVehiclePassenger_C() {
}

// VERIFY: 25b3b5b5 auto
// OFFSET: 0x00748950
void CVehiclePassenger_C::Free() {
    if (this == s_headPassenger) {
        s_headPassenger = this->m_next;

        if (s_headPassenger) {
            s_headPassenger->m_prev = nullptr;
        }
    } else if (this == s_tailPassenger) {
        s_tailPassenger = this->m_prev;

        if (s_tailPassenger) {
            s_tailPassenger->m_next = nullptr;
        }
    } else {
        this->m_prev->m_next = this->m_next;
        this->m_next->m_prev = this->m_prev;
    }

    if (this->m_deferredTransitionSpline) {
        delete this->m_deferredTransitionSpline;
    }

    ObjectFree(*s_passengerPool, this->m_memHandle);
}

// VERIFY: 664149dd manual
// OFFSET: 0x00749060
CVehicle_C* CVehiclePassenger_C::GetVehicle() const {
    CGUnit_C* unit;

    if (!this->m_transitionVehicleGuid || !(unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionVehicleGuid, TYPE_UNIT, __FILE__, __LINE__)))) {
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    if (unit) {
        return unit->m_vehicle;
    }

    return nullptr;
}

// VERIFY: bcb00fbb manual
// OFFSET: 0x00749C20
void CVehiclePassenger_C::AddPendingRescueTransition(WOWGUID passengerGuid, WOWGUID vehicleGuid, uint8_t seat, int32_t instant) {
    VehiclePassengerRescueTransition* transition = s_rescueTransitions->New();
    transition->m_passengerGuid = passengerGuid;
    transition->m_vehicleGuid = vehicleGuid;
    transition->m_seat = seat;
    transition->m_instant = instant != 0;
}

// VERIFY: c31d4cfc manual
// OFFSET: 0x00749D50
void CVehiclePassenger_C::OnVehicleTransitionAnimEnd(CGUnit_C* unit, int32_t& done, TSGrowableArray<CVehiclePassenger_C*>& passengers) {
    done = 0;
    CVehiclePassenger_C* passenger = unit->m_vehiclePassenger;

    if (passenger && static_cast<int32_t>(passenger->m_flags) >= 0) {
        passengers.New(passenger);
    }
}

// VERIFY: 8979bf0a manual
// OFFSET: 0x0074A070
void CVehiclePassenger_C::InitSystem() {
    delete s_passengerPool;
    delete s_pendingPassengers;

    uint32_t* pool = static_cast<uint32_t*>(SMemAlloc(sizeof(uint32_t), __FILE__, __LINE__, 0));

    if (pool) {
        *pool = ObjectAllocAddHeap(sizeof(CVehiclePassenger_C), 16, "Vehicle Passenger", true);
        s_passengerPool = pool;
    } else {
        s_passengerPool = nullptr;
    }

    void* pending = SMemAlloc(sizeof(TSGrowableArray<CVehiclePassenger_C*>), __FILE__, __LINE__, 0);
    s_pendingPassengers = pending ? new (pending) TSGrowableArray<CVehiclePassenger_C*>() : nullptr;
    s_pendingCount = 0;

    void* rescue = SMemAlloc(sizeof(TSGrowableArray<VehiclePassengerRescueTransition>), __FILE__, __LINE__, 0);
    s_headPassenger = nullptr;
    s_tailPassenger = nullptr;
    s_rescueTransitions = rescue ? new (rescue) TSGrowableArray<VehiclePassengerRescueTransition>() : nullptr;
}

// VERIFY: 93875c7e manual
// OFFSET: 0x0074A160
void CVehiclePassenger_C::UninitSystem() {
    if (s_passengerPool) {
        delete s_passengerPool;
        s_passengerPool = nullptr;
    }

    if (s_pendingPassengers) {
        delete s_pendingPassengers;
        s_pendingPassengers = nullptr;
    }

    s_pendingCount = 0;

    if (s_rescueTransitions) {
        delete s_rescueTransitions;
        s_rescueTransitions = nullptr;
    }

    s_tailPassenger = nullptr;
    s_headPassenger = nullptr;
}

// VERIFY: 7b346c04 manual
// OFFSET: 0x0074B160
void CVehiclePassenger_C::StoreRootVehicle(WOWGUID guid) {
    this->m_rootVehicleGuid = guid;

    if (!guid) {
        guid = this->m_unit->m_obj->m_guid;
    }

    CVehicle_C* vehicle = this->m_unit->m_vehicle;

    if (!vehicle) {
        return;
    }

    for (CPassenger* passenger = vehicle->m_passengers.Head(); passenger; passenger = vehicle->m_passengers.Next(passenger)) {
        CVehiclePassenger_C* vehiclePassenger = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__))->m_vehiclePassenger;

        if (vehiclePassenger) {
            vehiclePassenger->StoreRootVehicle(guid);
        }
    }
}

// VERIFY: 8a84e9f8 auto
// OFFSET: 0x007479E0
void CVehiclePassenger_C::StopBoneAnimIfSameBone(WOWGUID guid, const VehicleSeatRec* seat) {
    if (guid != this->m_boneAnimGuid) {
        return;
    }

    int32_t enterBone = seat->m_vehicleEnterAnimBone;

    if (enterBone >= 35) {
        enterBone = 26;
    }

    int32_t exitBone = seat->m_vehicleExitAnimBone;

    if (exitBone >= 35) {
        exitBone = 26;
    }

    if (enterBone == exitBone) {
        this->m_boneAnimGuid = WOWGUID();
    }
}

// VERIFY: 342659bc manual
// OFFSET: 0x00748070
void CVehiclePassenger_C::StartWaitingForVehicleTransitionAnim(WOWGUID guid, uint8_t seat, const VehicleSeatRec* seatRec, int32_t entering, CVehicle_C* vehicle) {
    this->m_flags |= 0x200;
    uint32_t flags = this->m_flags;
    this->m_transitionAnimVehicleGuid = vehicle->m_unit->m_obj->m_guid;
    this->m_transitionAnimGuid = guid;
    this->m_transitionAnimSeat = seat;

    if (seatRec->m_flags & (entering ? 0x8000000 : 0x10000000)) {
        float duration = entering ? seatRec->m_field33512213037 : seatRec->m_field33512213038;
        uint32_t timeout = static_cast<uint32_t>(duration * 1000.0f);

        if (timeout > 3000) {
            timeout = 3000;
        }

        this->m_flags = flags | 0x80;
        this->m_transitionAnimTimeout = timeout + FrameTime::s_curTimeMs;
    } else {
        this->m_flags = flags & ~0x80;

        if (entering) {
            this->m_flags |= 0x100;
        } else {
            this->m_flags &= ~0x100;
        }

        this->m_transitionAnimTimeout = FrameTime::s_curTimeMs + 3000;
    }
}

// VERIFY: 06e886f0 manual
// OFFSET: 0x00748400
void CVehiclePassenger_C::ComputeSeatOffset(const VehicleSeatRec* seat, CM2Model* model) {
    if (!seat || !model || !model->IsLoaded(0, 0)) {
        return;
    }

    uint32_t attachment = seat->m_passengerAttachmentID;

    if (model->HasAttachment(attachment)) {
        C3Vector pivot = model->GetAttachmentPivot(attachment);
        float x = -pivot.x;
        float y = -pivot.y;
        float z = -pivot.z;
        this->m_flags |= 0x40;
        this->m_flags |= 0x20;
        this->m_seatOffset.x = x;
        this->m_seatOffset.y = y;
        this->m_seatOffset.z = z;
    } else {
        this->m_seatOffset.x = 0.0f;
        this->m_seatOffset.y = 0.0f;
        this->m_seatOffset.z = 0.0f;
        this->m_flags &= ~0x40;
        this->m_flags |= 0x20;
    }
}

// VERIFY: ac9be9b7 auto
// OFFSET: 0x007485B0
int32_t CVehiclePassenger_C::ChooseSeatExitAnim(uint32_t flags, uint32_t* anim) const {
    uint32_t state = this->m_state;

    if (state != 3 && this->m_unit->m_unit->m_unitFieldHealth > 0) {
        return 0;
    }

    const VehicleSeatRec* seat = this->m_seatRec;

    if (!seat || state != 3 || !(seat->m_flags & 0x4)) {
        return 0;
    }

    int32_t exitAnim;

    if ((this->m_flags & 0x4) || seat->m_rideUpperAnimStart == -1) {
        exitAnim = seat->m_rideUpperAnimLoop;

        if (exitAnim == -1) {
            return 0;
        }
    } else {
        exitAnim = seat->m_rideUpperAnimStart;
    }

    if (exitAnim != 506) {
        *anim = exitAnim;
        return 1;
    }

    return 0;
}

// VERIFY: f673db8c auto
// OFFSET: 0x00747B20
int32_t CVehiclePassenger_C::GetLowerAnim(const VehicleSeatRec* seat) const {
    if (!seat) {
        return 506;
    }

    int32_t anim;

    switch (this->m_state) {
        case 1:
        case 2: {
            if (!(seat->m_flags & 0x1)) {
                return 506;
            }

            if ((this->m_flags & 0x2) || (anim = seat->m_enterAnimStart) == -1) {
                anim = seat->m_enterAnimLoop;

                if (anim == -1) {
                    return 506;
                }
            }

            break;
        }

        case 3: {
            if (!(seat->m_flags & 0x2)) {
                return 506;
            }

            if ((this->m_flags & 0x2) || (anim = seat->m_rideAnimStart) == -1) {
                anim = seat->m_rideAnimLoop;

                if (anim == -1) {
                    return 506;
                }
            }

            break;
        }

        case 4:
        case 5: {
            if (!this->m_unit->ShouldUseVehicleSeatExitAnimation(seat)) {
                return 506;
            }

            if ((this->m_flags & 0x2) || (anim = seat->m_exitAnimStart) == -1) {
                anim = seat->m_exitAnimLoop;

                if (anim == -1) {
                    return 506;
                }
            }

            break;
        }

        default: {
            return 506;
        }
    }

    return anim;
}

// VERIFY: a4d47b4a manual
// OFFSET: 0x00748040
CGUnit_C* CVehiclePassenger_C::GetStoredRootVehicleUnit() const {
    if (this->m_state) {
        return static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_rootVehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    return this->m_unit->FindRootVehicle(nullptr);
}

// VERIFY: 4f73af22 manual
// OFFSET: 0x00748620
void CVehiclePassenger_C::StartMirroringVehicleAnims() {
    if (this->m_transitionSeat >= 8) {
        return;
    }

    CGUnit_C* vehicleUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionVehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!vehicleUnit || !vehicleUnit->m_vehicle) {
        return;
    }

    vehicleUnit->m_vehicle->OnPassengerStartMirroringAnims(this->m_transitionSeat);
    this->m_flags |= 0x800;

    if (this->m_unit->m_unit->m_unitFieldHealth <= 0) {
        return;
    }

    CM2Model* vehicleModel = vehicleUnit->GetObjectModel();
    CM2Model* model = this->m_unit->GetObjectModel();

    if (!vehicleModel || !model || !vehicleModel->IsLoaded(0, 0) || !model->IsLoaded(0, 0)) {
        return;
    }

    M2BoneSequenceInfo info;
    vehicleModel->GetBoneSequenceInfo(0xFFFFFFFF, info);

    if (info.m_sequenceId >= 506) {
        model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        model->SetBoneSequence(0xFFFFFFFF, info.m_sequenceId, info.m_variationIndex, info.m_time, info.m_speed, 1, 1);
    }

    for (uint32_t bone = 0; bone < 35; bone++) {
        int32_t vehicleHasBone = vehicleModel->HasBone(bone);
        int32_t hasBone = model->HasBone(bone);

        if (vehicleHasBone) {
            if (!hasBone) {
                continue;
            }

            vehicleModel->GetBoneSequenceInfo(bone, info);

            if (info.m_sequenceId < 506) {
                model->SetBoneSequence(bone, info.m_sequenceId, info.m_variationIndex, info.m_time, info.m_speed, 1, 1);
                continue;
            }
        } else if (!hasBone) {
            continue;
        }

        if (model->GetBoneSequenceId(bone) != 0xFFFFFFFF) {
            model->UnsetBoneSequence(bone, 0, 1);
        }
    }

    if (vehicleModel->m_lastAnimTime) {
        uint32_t time = model->m_scene->m_time;

        if (!time) {
            time = 1;
        }

        model->m_lastAnimTime = time;
    } else {
        model->m_lastAnimTime = 0;
    }
}

// VERIFY: 826cb8a9 manual
// OFFSET: 0x00749790
CVehiclePassenger_C* CVehiclePassenger_C::Alloc(CGUnit_C* unit) {
    uint32_t memHandle;
    void* memory = nullptr;
    CVehiclePassenger_C* passenger = reinterpret_cast<CVehiclePassenger_C*>(ObjectAlloc(*s_passengerPool, &memHandle, &memory, false));

    if (passenger) {
        passenger = static_cast<CVehiclePassenger_C*>(memory);

        if (passenger) {
            new (passenger) CVehiclePassenger_C();
        }

        passenger->m_memHandle = memHandle;
    }

    passenger->m_facingBlend = 0.0f;
    passenger->m_unit = unit;
    passenger->m_flags = 0;
    passenger->m_state = 0;
    passenger->m_prevState = 0;
    passenger->m_vehicleGuid = WOWGUID();
    passenger->m_transitionVehicleGuid = WOWGUID();
    passenger->m_startTime = 0;
    passenger->m_endTime = 0;
    passenger->m_startRelativeGuid = WOWGUID();
    passenger->m_stopMovementTime = 0;
    passenger->m_vehicleRec = nullptr;
    passenger->m_seatRec = nullptr;
    passenger->m_rootVehicleGuid = WOWGUID();
    passenger->m_deferredTransitionSpline = nullptr;
    passenger->m_transitionAnimTimeout = 0;
    passenger->m_transitionAnimVehicleGuid = WOWGUID();
    passenger->m_boneAnimGuid = WOWGUID();
    passenger->m_transitionAnimGuid = WOWGUID();
    passenger->m_seat = 0xFF;
    passenger->m_transitionSeat = 0xFF;
    passenger->m_transitionAnimSeat = 0xFF;
    passenger->m_startModelPosition = C3Vector();
    passenger->m_startModelOffset = C3Vector();
    passenger->m_startPosition = C3Vector();
    passenger->m_startOffset = C3Vector();
    passenger->m_startFacing = 0.0f;
    passenger->m_startTargetFacing = 0.0f;
    passenger->m_smoothFacing = 0.0f;
    passenger->m_targetFacing = 0.0f;
    passenger->m_arcGravity = 0.0f;
    passenger->m_arcScale = 0.0f;
    passenger->m_seatOffset = C3Vector();
    passenger->m_lockedWorldFacing = 0.0f;
    passenger->m_prev = nullptr;

    CVehiclePassenger_C* head = s_headPassenger;
    s_headPassenger = passenger;

    if (!head) {
        passenger->m_next = nullptr;
        s_tailPassenger = passenger;
    } else {
        passenger->m_next = head;
        head->m_prev = passenger;
    }

    return passenger;
}

// VERIFY: 09b1b72f manual
// OFFSET: 0x00749A30
void CVehiclePassenger_C::Uninit() {
    if (this->m_flags & 0x800) {
        this->m_flags &= ~0x800;

        if (this->m_seat < 8) {
            CGUnit_C* vehicleUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (vehicleUnit && vehicleUnit->m_vehicle) {
                vehicleUnit->m_vehicle->OnPassengerStopMirroringAnims(this->m_seat);
            }
        }
    }

    CVehicle_C* vehicle = this->GetVehicle();

    if (vehicle) {
        vehicle->UpdateFreeSeatMask();
    }

    this->Free();
}

// VERIFY: a5679fee manual
// OFFSET: 0x00747E90
C3Vector CVehiclePassenger_C::ComputeStartWorldPosition(const C3Vector& offset, const C3Vector& fallback) const {
    CGObject_C* relative = ClntObjMgrObjectPtr(this->m_startRelativeGuid, TYPE_OBJECT, __FILE__, __LINE__);

    if (relative) {
        if (relative->m_obj->m_type & TYPE_UNIT) {
            C3Vector position = static_cast<CGUnit_C*>(relative)->BuildSmoothPosition();
            return C3Vector(offset.x + position.x, position.y + offset.y, position.z + offset.z);
        }

        return offset * relative->GetMatrix();
    }

    return fallback;
}

// VERIFY: b021a3dc auto
// OFFSET: 0x007484E0
void CVehiclePassenger_C::OnAnimEnd(uint32_t anim) {
    const VehicleSeatRec* seat = this->m_seatRec;

    if (this->GetLowerAnim(seat) != 506 && seat && this->m_state == 3 && (seat->m_flags & 0x4)) {
        int32_t upper;

        if ((this->m_flags & 0x4) || (upper = seat->m_rideUpperAnimStart) == -1) {
            upper = seat->m_rideUpperAnimLoop;

            if (upper == -1) {
                this->m_flags |= 0x6;
                return;
            }
        }

        if (upper != 506) {
            if (anim == 0xFFFFFFFF || anim == 26) {
                this->m_flags |= 0x2;
            } else {
                this->m_flags |= 0x4;
            }

            return;
        }
    }

    this->m_flags |= 0x6;
}

// VERIFY: 9156af0b auto
// OFFSET: 0x00748560
int32_t CVehiclePassenger_C::PickHighPriorityAnimation(int32_t unused_a2, uint32_t* anim) const {
    if ((this->m_state == 0 || this->m_state == 3) && this->m_unit->m_unit->m_unitFieldHealth > 0) {
        return 0;
    }

    if (!this->m_seatRec) {
        return 0;
    }

    int32_t lower = this->GetLowerAnim(this->m_seatRec);

    if (lower == 506) {
        return 0;
    }

    *anim = lower;
    return 1;
}

// VERIFY: e55483fa auto
// OFFSET: 0x007482A0
C3Vector CVehiclePassenger_C::ComputeInterpolatedUnitPosition() const {
    switch (this->m_state) {
        case 1:
        case 4:
            if (this->m_flags & 0x1) {
                return this->ComputeStartWorldPosition(this->m_startOffset, this->m_startPosition);
            }

            return this->m_startPosition;
        case 2:
        case 5:
            if (this->m_flags & 0x1) {
                float blend = this->m_facingBlend;
                C3Vector smooth = this->m_unit->BuildSmoothPosition();
                C3Vector end(blend * smooth.x, smooth.y * blend, blend * smooth.z);
                float inverse = 1.0f - this->m_facingBlend;
                C3Vector start = this->ComputeStartWorldPosition(this->m_startOffset, this->m_startPosition);
                return C3Vector(start.x * inverse + end.x, start.y * inverse + end.y, start.z * inverse + end.z);
            }

            return this->m_startPosition;
        case 3:
            return this->m_unit->BuildSmoothPosition();
        default:
            return this->m_unit->GetPosition();
    }
}

// VERIFY: 1e2a1e8c auto
// OFFSET: 0x007487E0
int32_t CVehiclePassenger_C::OverridesModelAnimation() const {
    return (this->m_flags & 0x800) && this->m_unit->m_unit->m_unitFieldHealth > 0;
}

// VERIFY: 1c7d54aa auto
// OFFSET: 0x007478F0
void CVehiclePassenger_C::StartAddingPendingRescueTransitions() {
    CVehiclePassenger_C::s_pendingCount = 1;
}

// VERIFY: f56a1f77 auto
// OFFSET: 0x00747BD0
int32_t CVehiclePassenger_C::GetUpperAnim(const VehicleSeatRec* seat) const {
    if (!seat || this->m_state != 3 || !(seat->m_flags & 0x4)) {
        return ANIM_COUNT;
    }

    int32_t anim;

    if ((this->m_flags & 0x4) || (anim = seat->m_rideUpperAnimStart) == -1) {
        anim = seat->m_rideUpperAnimLoop;

        if (anim == -1) {
            return ANIM_COUNT;
        }
    }

    return anim;
}

// VERIFY: 4bdd68c2 auto
// OFFSET: 0x007490C0
int32_t CVehiclePassenger_C::CheckForVehicleTransitionAnimTimeout() {
    if (!(this->m_flags & 0x200) || static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_transitionAnimTimeout) < 0) {
        return 0;
    }

    this->StopWaitingForVehicleTransitionAnim();
    return 1;
}

// VERIFY: d1f79f7d auto
// OFFSET: 0x00747F40
void CVehiclePassenger_C::OnSetActiveMover(CGUnit_C* unit) {
    const VehicleRec* vehicleRec = nullptr;

    if (unit) {
        vehicleRec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;
        FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_UPDATE, nullptr);

        if (vehicleRec) {
            CGUnit_C::StartMissileTrajectoryTargeting(unit, vehicleRec);
            CGUnit_C::UpdateVehicleUISliders(unit, vehicleRec);
            return;
        }
    }

    CGUnit_C::StopMissileTrajectoryTargeting();
    CGUnit_C::UpdateVehicleUISliders(unit, vehicleRec);
}

// VERIFY: 3d010f1f manual
// OFFSET: 0x00748230
void CVehiclePassenger_C::StopWaitingForVehicleTransitionAnim() {
    if (!(this->m_flags & 0x200)) {
        return;
    }

    this->m_flags &= ~0x200;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionAnimVehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && unit->m_vehicle) {
        unit->m_vehicle->UninstallBoneAnimListener(this->m_unit->m_obj->m_guid);
    }

    if (this->m_deferredTransitionSpline) {
        this->ExecuteAllDeferredVehicleTransitionAnimSplines();
    }
}

// VERIFY: b352fb18 auto
// OFFSET: 0x00747990
void CVehiclePassenger_C::ExecuteAllDeferredVehicleTransitionAnimSplines() {
    CDataStore* spline = this->m_deferredTransitionSpline;
    WOWGUID transport = this->m_transitionAnimGuid;
    this->m_deferredTransitionSpline = nullptr;
    this->m_unit->OnMonsterMove(spline, transport.m_guid ? SMSG_MONSTER_MOVE_TRANSPORT : SMSG_ON_MONSTER_MOVE, transport.m_guid, this->m_transitionAnimSeat, 0);

    if (spline) {
        delete spline;
    }
}

// VERIFY: df7afd61 auto
// OFFSET: 0x00747FA0
void CVehiclePassenger_C::StopMovement() {
    if (this->m_state != 3 || this->m_unit->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return;
    }

    uint32_t time = FrameTime::s_curTimeMs;
    CGInputControl::GetActive()->StopMovementAndTurning(time);
    this->m_flags |= 0x8;
    this->m_stopMovementTime = time;
}

// VERIFY: 20bfa0c2 manual
// OFFSET: 0x007498E0
void CVehiclePassenger_C::Init(CGUnit_C* unit) {
    WOWGUID transport;
    transport.m_guid = unit->GetTransportGUID();
    uint32_t high = static_cast<uint32_t>(transport.m_guid >> 32);

    if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(transport.m_guid)))) {
        CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transport, TYPE_UNIT, __FILE__, __LINE__));
        uint8_t seat = unit->m_movementData.m_transportSeat;
        unit->VehiclePassengerOnUpdateTransport(transport, seat, 1);
        CVehiclePassenger_C* passenger = unit->m_vehiclePassenger;

        if (passenger) {
            passenger->SetState(3, vehicle, seat, passenger->m_seatRec, FrameTime::s_curTimeMs, transport);
        }

        return;
    }

    if (!(unit->m_obj->m_type & TYPE_PLAYER)) {
        return;
    }

    WOWGUID guid = unit->m_obj->m_guid;
    CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(guid);

    if (!state || !(state->m_flags & 0x200)) {
        return;
    }

    guid = unit->m_obj->m_guid;
    Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_EXITING_VEHICLE);
    int32_t count;
    char** tokens = Script_GetTokenIDsFromGUID(guid, &count);

    for (int32_t i = 0; i < count; i++) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_EXITED_VEHICLE, "%s%s", tokens[i], "");
    }
}

// VERIFY: 825df5ed manual
// OFFSET: 0x0074B0B0
void CVehiclePassenger_C::Update(uint32_t time) {
    VehicleSeatRec* seatRec = this->m_seatRec;
    CGUnit_C* vehicle;

    if (this->m_transitionVehicleGuid) {
        vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionVehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    } else {
        vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    this->UpdateBlend(time, seatRec);

    if (!this->UpdateState(vehicle, seatRec, time)) {
        this->m_unit->VehiclePassengerOnUninit();
        this->Uninit();
    }
}

// VERIFY: 53b38716 auto
// OFFSET: 0x0074B130
void CVehiclePassenger_C::UpdateAll(uint32_t time) {
    for (CVehiclePassenger_C* passenger = CVehiclePassenger_C::s_headPassenger; passenger; ) {
        CVehiclePassenger_C* next = passenger->m_next;
        passenger->Update(time);
        passenger = next;
    }
}

// VERIFY: 8bae8c48 manual
// OFFSET: 0x00749CB0
void CVehiclePassenger_C::ExecutePendingRescueTransitions() {
    for (uint32_t i = 0; i < CVehiclePassenger_C::s_rescueTransitions->Count(); i++) {
        VehiclePassengerRescueTransition& transition = CVehiclePassenger_C::s_rescueTransitions->m_data[i];
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transition.m_passengerGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger) {
            unit->m_vehiclePassenger->BeginTransition(transition.m_vehicleGuid, transition.m_seat, transition.m_instant);
        }
    }

    CVehiclePassenger_C::s_rescueTransitions->m_count = 0;
    CVehiclePassenger_C::s_pendingCount = 0;
}

// STATIC: 0x00A2D3F0
static const uint32_t s_seatAttachments[22] = { 20, 34, 19, 21, 22, 17, 23, 24, 25, 15, 16, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 0 };

// VERIFY: aa11b3c6 auto
// OFFSET: 0x00716450
int32_t CVehiclePassenger_C::IsInTransitionMotion() const {
    return this->m_state == 2 || this->m_state == 5;
}

// VERIFY: 42d85f81 auto
// OFFSET: 0x00747D50
int32_t CVehiclePassenger_C::IsEntering() const {
    return this->m_state == 1 || this->m_state == 2;
}

// VERIFY: 7a79302e manual
// OFFSET: 0x00747D70
void CVehiclePassenger_C::UpdateBlend(uint32_t time, VehicleSeatRec* seatRec) {
    if (!seatRec || !(this->m_flags & 0x1)) {
        this->m_facingBlend = 0.0f;
        return;
    }

    float duration = static_cast<int32_t>(this->m_endTime - this->m_startTime) * 0.001f;

    if (duration < 0.0001f) {
        this->m_facingBlend = 1.0f;
        return;
    }

    this->m_facingBlend = static_cast<int32_t>(time - this->m_startTime) * 0.001f / duration;
    float blend = this->m_facingBlend;
    this->m_facingBlend = blend < 0.0f ? 0.0f : (blend < 1.0f ? blend : 1.0f);

    uint32_t easeIn;
    uint32_t easeOut;

    if (this->m_state == 2) {
        easeIn = 0x20;
        easeOut = 0x40;
    } else if (this->m_state == 5) {
        easeIn = 0x80;
        easeOut = 0x100;
    } else {
        return;
    }

    uint32_t flags = seatRec->m_flags;

    if ((flags & easeIn) && (flags & easeOut)) {
        this->m_facingBlend = 0.5f - std::cos(this->m_facingBlend * 3.1415927f) * 0.5f;
    } else if (seatRec->m_flags & easeIn) {
        this->m_facingBlend = this->m_facingBlend * this->m_facingBlend;
    } else if (flags & easeOut) {
        this->m_facingBlend = 1.0f - (1.0f - this->m_facingBlend) * (1.0f - this->m_facingBlend);
    }
}

// VERIFY: 21751f98 auto
// OFFSET: 0x00748810
static void SignalVehicleTransitionEvent(WOWGUID guid, const VehicleRec* vehicleRec, const VehicleSeatRec* seatRec, uint32_t event) {
    const char* sound = "";
    bool controlSeat;
    const char* skin;
    bool hasVehicleUI;

    if (seatRec) {
        controlSeat = (seatRec->m_flags & 0x20000000) != 0;
        skin = static_cast<uint32_t>(seatRec->m_uiSkin) < 2 ? s_vehicleSkinNames[seatRec->m_uiSkin] : "";
        SoundEntriesRec* soundRec = g_soundEntriesDB.GetRecord(seatRec->m_enterUisoundID);

        if (soundRec) {
            sound = soundRec->m_name;
        }

        hasVehicleUI = (seatRec->m_flags & 0x800) != 0;
    } else {
        controlSeat = false;
        skin = "";
        hasVehicleUI = false;
    }

    int32_t count;
    char** tokens = Script_GetTokenIDsFromGUID(guid, &count);

    for (int32_t i = 0; i < count; i++) {
        FrameScript_SignalEvent(event, "%s%b%s%s%b%d", tokens[i], controlSeat, skin, sound, hasVehicleUI, vehicleRec ? vehicleRec->m_vehicleUiindicatorID : 0);
    }
}

// VERIFY: 01220e79 manual
// OFFSET: 0x007489C0
void CVehiclePassenger_C::SetState(uint32_t state, CGUnit_C* vehicle, uint8_t seat, VehicleSeatRec* seatRec, uint32_t time, WOWGUID relativeGuid) {
    if (this->m_flags & 0x800) {
        this->m_flags &= ~0x800;

        if (this->m_seat < 8) {
            CGUnit_C* vehicleUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (vehicleUnit && vehicleUnit->m_vehicle) {
                vehicleUnit->m_vehicle->OnPassengerStopMirroringAnims(this->m_seat);
            }
        }

        this->m_unit->UpdateBaseAnimation(0, -1);
    }

    if (this->m_state == 3 && this->m_boneAnimGuid) {
        CGUnit_C* boneUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_boneAnimGuid, TYPE_UNIT, __FILE__, __LINE__));
        CVehicle_C* boneVehicle = boneUnit ? boneUnit->m_vehicle : nullptr;

        if (boneVehicle) {
            if (vehicle && vehicle->m_vehicle) {
                VehicleSeatRec* oldSeat = vehicle->m_vehicle->GetVehicleSeatRec(this->m_seat);

                if (oldSeat && (oldSeat->m_flags & 0x20000)) {
                    boneVehicle->RemoveBoneAnimListener(oldSeat->m_vehicleEnterAnimBone, this->m_unit->m_obj->m_guid);
                }
            }

            this->m_boneAnimGuid = WOWGUID();
        }
    }

    if (state == 1 || state == 4) {
        relativeGuid.m_guid = this->m_unit->GetTransportGUID();
    }

    this->m_startRelativeGuid = relativeGuid;

    switch (state) {
        case 1:
        case 2:
        case 4:
        case 5: {
            this->m_startModelPosition = this->m_unit->GetModelPosition();
            this->m_startPosition = this->m_unit->BuildSmoothPosition();

            switch (this->m_state) {
                case 0:
                case 3: {
                    float facing = this->m_unit->GetSmoothFacing();
                    this->m_smoothFacing = facing;
                    this->m_startFacing = facing;
                    this->m_startTargetFacing = this->m_unit->m_smoothTarget.m_value;
                    break;
                }

                case 1:
                case 2:
                case 4:
                case 5: {
                    this->m_startTargetFacing = this->m_smoothFacing;
                    this->m_startFacing = this->m_smoothFacing;
                    break;
                }
            }

            break;
        }

        case 3: {
            WOWGUID guid = vehicle ? vehicle->m_obj->m_guid : WOWGUID();
            this->m_transitionVehicleGuid = guid;
            this->m_vehicleGuid = guid;
            this->m_transitionSeat = seat;
            this->m_seat = seat;
            break;
        }
    }

    if (state == 3 && seatRec && (seatRec->m_flagsB & 0x10000)) {
        this->StartMirroringVehicleAnims();
    }

    this->m_flags &= ~0x8;
    CM2Model* model = this->m_unit->GetObjectModel();

    if ((this->m_unit->m_modelFlags & 0x40000) && model && (model->m_fFlags & 0x8000)) {
        this->m_flags |= 0x10;
    } else {
        this->m_flags &= ~0x10;
    }

    this->m_flags &= ~0x20;

    if (!state) {
        if (model && model->m_attachParent) {
            model->DetachFromParent();
        }

        C3Vector normal = this->m_unit->m_movement->GetGroundNormal();
        float scale = this->m_unit->GetTrueScale();
        float facing = this->m_unit->GetFacing();
        model->SetWorldTransform(this->m_unit->GetPosition(), facing, scale, normal, 0xFFFFFFFF);
    }

    if (model) {
        uint32_t hidden = !(this->m_unit->m_unit->m_unitFieldFlags & 0x2000000) && (!seatRec || !(seatRec->m_flags & 0x100000)) && state;
        model->m_fFlags ^= (model->m_fFlags ^ (hidden << 19)) & 0x80000;
    }

    this->m_startTime = time;

    if (this->m_unit->m_unit->m_unitFieldHealth > 0) {
        this->m_unit->ResetKeyBoneSequences();
    } else if (this->m_state != 5 && (state == 5 || !state)) {
        this->m_unit->OnDeathAnimate();
    }

    if (this->m_state == 5 && !state && this->m_unit->m_unit->m_unitFieldHealth > 0) {
        uint32_t anim;

        if (seatRec) {
            anim = seatRec->m_exitAnimEnd;

            if (anim == ANIM_JUMP_END && (this->m_unit->m_movement->m_flags & 0xF)) {
                anim = ANIM_JUMP_LAND_RUN;
            }
        } else {
            anim = 0xFFFFFFFF;
        }

        this->m_unit->PlayVehicleExitEndAnimation(seatRec, anim);
    }

    uint32_t prevState = this->m_state;
    int32_t keepAnimFlags = (prevState == 1 && state == 2) || prevState == 4;
    int32_t entered = !prevState && state == 3;
    this->m_state = state;
    CGUnit_C* root = nullptr;

    if (vehicle) {
        root = vehicle->GetRootVehicleUnit();
    }

    if (seatRec && (seatRec->m_flags & 0x200)) {
        if ((this->m_state == 1 || this->m_state == 2) && (this->m_flags & 0x400)) {
            CGUnit_C* oldVehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
            CGUnit_C* oldRoot = oldVehicle ? oldVehicle->GetRootVehicleUnit() : nullptr;

            if (oldRoot == root) {
                this->m_flags |= 0x400;
            } else {
                this->m_flags &= ~0x400;
            }
        } else if (this->m_state == 3) {
            this->m_flags |= 0x400;
        } else {
            this->m_flags &= ~0x400;
        }
    } else {
        this->m_flags &= ~0x400;
    }

    if (this->m_state && this->m_unit->m_unit->m_unitFieldHealth > 0) {
        if (!keepAnimFlags) {
            this->m_flags &= ~0x6;
        }

        this->m_unit->UpdateBaseAnimation(entered != 0, -1);

        if (!keepAnimFlags) {
            this->m_flags &= ~0x6;
        }
    }

    CGUnit_C* sliderUnit = this->m_unit;
    WOWGUID guid = sliderUnit->m_obj->m_guid;

    if (CGUnit_C::s_activeMover == guid || ((sliderUnit = vehicle) != nullptr && CGUnit_C::s_activeMover == vehicle->m_obj->m_guid)) {
        CGUnit_C::UpdateVehicleUISliders(sliderUnit, sliderUnit->m_vehicle ? sliderUnit->m_vehicle->m_vehicleRec : nullptr);
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera && ClntObjMgrGetActivePlayer() == guid && (camera->m_targetGUID == guid || camera->m_targetGUID == this->m_vehicleGuid)) {
        uint32_t current = this->m_state;

        if ((current == 1 || current == 2 || current == 3) && vehicle && seatRec && (seatRec->m_flags & 0x800)) {
            camera->SetTarget(vehicle, 0);
        } else {
            camera->SetTarget(this->m_unit, 0);
        }
    }

    this->m_flags &= ~0x1;
    CVehicle_C* vehicleObject = vehicle ? vehicle->m_vehicle : nullptr;

    if (this->m_state == 3 && vehicleObject && seatRec && (seatRec->m_flags & 0x20000) && seatRec->m_vehicleRideAnimLoop < 506) {
        CVehicle_C* boneVehicle = vehicleObject->FindVehicleToPlayBoneAnim();

        if (boneVehicle->PlayBoneAnim(seatRec->m_vehicleEnterAnimBone, seatRec->m_vehicleRideAnimLoop)) {
            boneVehicle->InstallBoneAnimListener(this->m_unit->m_obj->m_guid, seatRec->m_vehicleEnterAnimBone, CVehiclePassenger_C::OnVehicleRideLoopAnimEnd);
            this->m_boneAnimGuid = boneVehicle->m_unit->m_obj->m_guid;
        }
    }

    if (this->m_unit->IsLocalClientControlled() && (!this->m_state || this->m_state == 3)) {
        CGInputControl::GetActive()->UpdatePlayer(time, 1);
    }

    if (vehicleObject && (prevState == 3 || this->m_state == 3)) {
        vehicleObject->UpdateLargestPassengerBoundsRadius();
    }

    this->UpdateBlend(time, seatRec);
}

// VERIFY: daac9e18 manual
// OFFSET: 0x007490F0
void CVehiclePassenger_C::ComputeModelTransform(C44Matrix& matrix, CM2Model*& model, CGUnit_C* vehicle, const VehicleSeatRec* seat, uint32_t attachment, float facing) {
    CVehiclePassenger_C* vehiclePassenger = vehicle->m_vehiclePassenger;
    float vehicleFacing = vehiclePassenger ? vehiclePassenger->GetLockedWorldFacing() : vehicle->GetFacing();

    if (this->m_flags & 0x1000) {
        facing = this->m_lockedWorldFacing - vehicleFacing;
    }

    int32_t hasAttachment = model->HasAttachment(attachment);
    matrix.RotateAroundZ(facing - seat->m_field33512213027);

    if (0.0f != seat->m_field33512213029) {
        matrix.RotateAroundX(seat->m_field33512213029);
    }

    if (0.0f != seat->m_passengerPitch) {
        matrix.RotateAroundY(seat->m_passengerPitch);
    }

    float vehicleScale;

    if (hasAttachment) {
        C44Matrix transform = model->GetAttachmentWorldTransform(attachment);
        float length = std::sqrt(transform.a2 * transform.a2 + transform.a1 * transform.a1 + transform.a0 * transform.a0);

        if (length <= 0.0001f) {
            matrix.Scale(this->m_unit->GetTrueScale());
        } else {
            matrix.Scale(this->m_unit->GetTrueScale() / length);
        }

        vehicleScale = 1.0f;
    } else {
        matrix.Scale(this->m_unit->GetTrueScale());
        vehicleScale = vehicle->GetTrueScale();
    }

    C3Vector offset;

    if ((this->m_flags & 0x20) && (this->m_flags & 0x40)) {
        offset = this->m_seatOffset;
        offset *= matrix;
        offset.x = seat->m_attachmentOffset[0] * vehicleScale + offset.x;
        offset.y = seat->m_attachmentOffset[1] * vehicleScale + offset.y;
        offset.z = seat->m_attachmentOffset[2] * vehicleScale + offset.z;
    } else {
        offset.x = seat->m_attachmentOffset[0] * vehicleScale;
        offset.y = seat->m_attachmentOffset[1] * vehicleScale;
        offset.z = vehicleScale * seat->m_attachmentOffset[2];
    }

    matrix.d0 = offset.x;
    matrix.d1 = offset.y;
    matrix.d2 = offset.z;

    if (hasAttachment) {
        return;
    }

    C44Matrix world;
    C3Vector position;
    vehiclePassenger = vehicle->m_vehiclePassenger;

    if (vehiclePassenger && vehiclePassenger->m_state && vehiclePassenger->m_state != 3) {
        CM2Model* vehicleModel = model;
        vehicleModel->Animate();
        position = *reinterpret_cast<const C3Vector*>(&vehicleModel->m_matrixF4.d0) * vehicleModel->m_scene->m_viewInv;
    } else {
        position = vehicle->GetPosition();
    }

    world.d0 = position.x;
    world.d1 = position.y;
    world.d2 = position.z;
    vehiclePassenger = vehicle->m_vehiclePassenger;
    world.RotateAroundZ(vehiclePassenger && vehiclePassenger->m_state && vehiclePassenger->m_state != 3 ? vehiclePassenger->m_smoothFacing : vehicleFacing);
    matrix *= world;
    model = nullptr;
}

// VERIFY: dfb5bc68 manual
// OFFSET: 0x007493B0
void CVehiclePassenger_C::ComputeSeatPos(CGUnit_C* vehicle, CM2Model* model, VehicleSeatRec* seat, C3Vector& position) {
    CM2Model* ownModel = this->m_unit->GetObjectModel();

    if (!(this->m_flags & 0x20)) {
        this->ComputeSeatOffset(seat, ownModel);
    }

    uint32_t attachment = static_cast<uint32_t>(seat->m_attachmentID) >= 22 ? 0xFFFFFFFF : s_seatAttachments[seat->m_attachmentID];

    if ((this->m_flags & 0x20) && (this->m_flags & 0x40)) {
        C44Matrix matrix;
        float facing = (this->m_state == 4 || this->m_state == 5) ? this->m_startTargetFacing : this->m_unit->m_smoothTarget.m_value;
        this->ComputeModelTransform(matrix, model, vehicle, seat, attachment, facing);
        position.x = matrix.d0;
        position.y = matrix.d1;
        position.z = matrix.d2;
    } else {
        position.x = seat->m_attachmentOffset[0];
        position.y = seat->m_attachmentOffset[1];
        position.z = seat->m_attachmentOffset[2];
    }

    if (!model) {
        return;
    }

    if (model->HasAttachment(attachment)) {
        position *= model->GetAttachmentWorldTransform(attachment);
        CVehiclePassenger_C* vehiclePassenger = vehicle->m_vehiclePassenger;

        if (!vehiclePassenger || !vehiclePassenger->m_state || vehiclePassenger->m_state == 3) {
            C44Matrix animated = model->GetAnimatedWorldTransform();
            float scale = vehicle->GetTrueScale();

            if (animated.c2 <= 0.999f) {
                position.x = position.x - animated.d0;
                position.y = position.y - animated.d1;
                position.z = position.z - animated.d2;
            } else {
                position *= animated.AffineInverse(scale);
            }

            C44Matrix smooth;
            vehicle->BuildSmoothMatrix(smooth);
            smooth.Scale(scale);

            if (animated.c2 <= 0.999f) {
                position.x = position.x + smooth.d0;
                position.y = position.y + smooth.d1;
                position.z = smooth.d2 + position.z;
            } else {
                position *= smooth;
            }
        }
    } else {
        CVehiclePassenger_C* vehiclePassenger = vehicle->m_vehiclePassenger;

        if (vehiclePassenger && vehiclePassenger->m_state && vehiclePassenger->m_state != 3) {
            position *= model->GetAnimatedWorldTransform();
        } else {
            position *= vehicle->GetMatrix();
        }
    }
}

// VERIFY: 04e9dae7 auto
// OFFSET: 0x00749650
void CVehiclePassenger_C::SignalTransitionUIEvents(WOWGUID guid, const VehicleRec* vehicleRec, const VehicleSeatRec* seatRec, uint32_t events) {
    if (events & 0x1) {
        SignalVehicleTransitionEvent(guid, vehicleRec, seatRec, SCRIPT_EVENT_UNIT_ENTERING_VEHICLE);
    }

    if (events & 0x2) {
        SignalVehicleTransitionEvent(guid, vehicleRec, seatRec, SCRIPT_EVENT_UNIT_ENTERED_VEHICLE);
    }

    if (events & 0x4) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_EXITING_VEHICLE);
    }

    if (events & 0x8) {
        const char* sound = "";
        SoundEntriesRec* soundRec;

        if (seatRec && (soundRec = g_soundEntriesDB.GetRecord(seatRec->m_field33512213041)) != nullptr) {
            sound = soundRec->m_name;
        }

        int32_t count;
        char** tokens = Script_GetTokenIDsFromGUID(guid, &count);

        for (int32_t i = 0; i < count; i++) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_EXITED_VEHICLE, "%s%s", tokens[i], sound);
        }
    }
}

// VERIFY: b2c1b566 manual
// OFFSET: 0x00749AA0
void CVehiclePassenger_C::BeginTransition(WOWGUID vehicleGuid, uint8_t seat, uint8_t instant) {
    this->StopWaitingForVehicleTransitionAnim();

    switch (this->m_state) {
        case 0:
        case 5: {
            this->m_vehicleGuid = WOWGUID();
            this->m_seat = 0xFF;
            break;
        }

        case 2:
        case 3: {
            this->m_vehicleGuid = this->m_transitionVehicleGuid;
            this->m_seat = this->m_transitionSeat;
            break;
        }
    }

    uint32_t high = static_cast<uint32_t>(vehicleGuid.m_guid >> 32);
    int32_t entering;
    WOWGUID target;

    if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(vehicleGuid.m_guid)))) {
        target = vehicleGuid;
        entering = 1;
    } else {
        entering = 0;
        target = WOWGUID();
    }

    this->m_flags &= ~0x1;
    VehicleSeatRec* seatRec = this->m_seatRec;
    this->m_transitionVehicleGuid = target;
    uint32_t state = entering ? 3 : 0;
    this->m_transitionSeat = seat;
    WOWGUID lookup;

    if (instant && seatRec) {
        if (!entering) {
            if (this->m_unit->ShouldUseVehicleSeatExitAnimation(seatRec) && seatRec->m_exitSpeed > 0.0001f) {
                state = seatRec->m_field33512213017 <= 0.0001f ? 5 : 4;
            }

            lookup = this->m_vehicleGuid;
        } else if (!(seatRec->m_flags & 0x1) || seatRec->m_enterSpeed <= 0.0001f) {
            lookup = vehicleGuid;
        } else {
            state = seatRec->m_field33512213004 <= 0.0001f ? 2 : 1;
            lookup = vehicleGuid;
        }
    } else {
        lookup = entering ? vehicleGuid : this->m_vehicleGuid;
    }

    CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(lookup, TYPE_UNIT, __FILE__, __LINE__));
    this->SetState(state, vehicle, seat, seatRec, FrameTime::s_curTimeMs, vehicleGuid);
}

// VERIFY: c52dd509 auto
// OFFSET: 0x00749D80
C3Vector CVehiclePassenger_C::GetEndPosition() {
    VehicleSeatRec* seatRec = this->m_seatRec;
    CGUnit_C* vehicle;
    CM2Model* model;

    if (this->m_state && static_cast<uint32_t>(this->m_transitionVehicleGuid.m_guid) && (vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionVehicleGuid, TYPE_UNIT, __FILE__, __LINE__))) != nullptr && (model = vehicle->GetObjectModel()) != nullptr && model->IsLoaded(0, 0) && seatRec) {
        C3Vector position(0.0f, 0.0f, 0.0f);
        this->ComputeSeatPos(vehicle, model, seatRec, position);
        return position;
    }

    return this->m_unit->GetPosition();
}

// VERIFY: 69636cc4 auto
// OFFSET: 0x00749E40
void CVehiclePassenger_C::ComputeEndPos(CGUnit_C* vehicle, VehicleSeatRec* seat, C3Vector& position) {
    CM2Model* model = vehicle ? vehicle->GetObjectModel() : nullptr;

    if (this->m_state == 2 && model && model->IsLoaded(0, 0) && seat) {
        this->ComputeSeatPos(vehicle, model, seat, position);
    } else {
        position = this->m_unit->GetPosition();
    }
}

// VERIFY: f28358f7 auto
// OFFSET: 0x00749ED0
void CVehiclePassenger_C::OnReloadUI() {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CVehiclePassenger_C* passenger = player->m_vehiclePassenger;

    if (!passenger || !passenger->m_state) {
        return;
    }

    WOWGUID transport;
    transport.m_guid = player->GetTransportGUID();
    CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transport, TYPE_UNIT, __FILE__, __LINE__));

    if (!vehicle) {
        return;
    }

    VehicleRec* vehicleRec = vehicle->m_vehicle ? vehicle->m_vehicle->m_vehicleRec : nullptr;

    if (!vehicleRec) {
        return;
    }

    uint8_t seat = player->m_movementData.m_transportSeat;

    if (seat >= 8) {
        return;
    }

    VehicleSeatRec* seatRec = g_vehicleSeatDB.GetRecord(vehicleRec->m_seatID[seat]);

    if (!seatRec) {
        return;
    }

    int32_t state = static_cast<int32_t>(player->m_vehiclePassenger->m_state);

    if (state <= 0) {
        return;
    }

    if (state <= 2) {
        CVehiclePassenger_C::SignalTransitionUIEvents(player->m_obj->m_guid, vehicleRec, seatRec, 1);
    } else if (state == 3) {
        CVehiclePassenger_C::SignalTransitionUIEvents(player->m_obj->m_guid, vehicleRec, seatRec, 3);
    }
}

// VERIFY: a6ae58f9 manual
// OFFSET: 0x00749FB0
void CVehiclePassenger_C::SendTransitionUIEvents(WOWGUID guid, const VehicleRec* vehicleRec, const VehicleSeatRec* seatRec, int32_t fromState, int32_t toState) {
    switch (toState) {
        case 0: {
            if (fromState == 3) {
                CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 12);
            } else {
                CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 8);
            }

            return;
        }

        case 1: {
            CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 1);
            return;
        }

        case 2: {
            if (fromState != 1) {
                CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 1);
            }

            return;
        }

        case 3: {
            if (!fromState || fromState == 3) {
                CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 3);
            } else {
                CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 2);
            }

            return;
        }

        case 4: {
            CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 4);
            return;
        }

        case 5: {
            if (fromState != 4) {
                CVehiclePassenger_C::SignalTransitionUIEvents(guid, vehicleRec, seatRec, 4);
            }

            return;
        }
    }
}

// VERIFY: e6bbf669 manual
// OFFSET: 0x0074A200
void CVehiclePassenger_C::ComputeEndValues(CGUnit_C* vehicle, VehicleSeatRec* seatRec, uint32_t time, C3Vector& endPosition) {
    this->m_flags |= 0x1;

    if (!seatRec) {
        return;
    }

    CGObject_C* relative = ClntObjMgrObjectPtr(this->m_startRelativeGuid, TYPE_OBJECT, __FILE__, __LINE__);

    if (relative) {
        if (relative->m_obj->m_type & TYPE_UNIT) {
            C3Vector position = static_cast<CGUnit_C*>(relative)->BuildSmoothPosition();
            this->m_startModelOffset = C3Vector(this->m_startModelPosition.x - position.x, this->m_startModelPosition.y - position.y, this->m_startModelPosition.z - position.z);
            this->m_startOffset = C3Vector(this->m_startPosition.x - position.x, this->m_startPosition.y - position.y, this->m_startPosition.z - position.z);
        } else {
            C44Matrix inverse = relative->GetMatrix().AffineInverse();
            this->m_startModelOffset = this->m_startModelPosition * inverse;
            this->m_startOffset = this->m_startPosition * inverse;
        }
    } else {
        this->m_startModelOffset = C3Vector(0.0f, 0.0f, 0.0f);
        this->m_startOffset = C3Vector(0.0f, 0.0f, 0.0f);
    }

    if (this->m_state == 3) {
        return;
    }

    this->m_targetFacing = this->m_unit->GetFacing();

    while (this->m_targetFacing + 3.1415927f < this->m_smoothFacing) {
        this->m_targetFacing = this->m_targetFacing + 6.2831855f;
    }

    while (this->m_targetFacing - 3.1415927f > this->m_smoothFacing) {
        this->m_targetFacing = this->m_targetFacing - 6.2831855f;
    }

    this->ComputeEndPos(vehicle, seatRec, endPosition);
    uint32_t state = this->m_state;
    float minDuration;
    float maxDuration;
    float minArcHeight;
    float maxArcHeight;

    switch (state) {
        case 1: {
            float duration = seatRec->m_field33512213004 < 10.0f ? seatRec->m_field33512213004 : 10.0f;
            this->m_endTime = this->m_startTime - static_cast<int32_t>(duration * -1000.0f);
            return;
        }

        case 4: {
            float duration = seatRec->m_field33512213017 < 10.0f ? seatRec->m_field33512213017 : 10.0f;
            this->m_endTime = this->m_startTime - static_cast<int32_t>(duration * -1000.0f);
            return;
        }

        case 2: {
            this->m_arcGravity = seatRec->m_enterGravity;
            minDuration = seatRec->m_enterMinDuration;
            maxDuration = seatRec->m_enterMaxDuration;
            minArcHeight = seatRec->m_enterMinArcHeight;
            maxArcHeight = seatRec->m_enterMaxArcHeight;
            break;
        }

        default: {
            this->m_arcGravity = seatRec->m_exitGravity;
            minDuration = seatRec->m_exitMinDuration;
            maxDuration = seatRec->m_exitMaxDuration;
            minArcHeight = seatRec->m_exitMinArcHeight;
            maxArcHeight = seatRec->m_exitMaxArcHeight;
            break;
        }
    }

    if (maxDuration < 0.0001f) {
        maxDuration = 1.5f;
    } else if (maxDuration > 10.0f) {
        maxDuration = 10.0f;
    }

    float duration;

    if (state == 2 && vehicle) {
        C3Vector velocity = vehicle->GetVelocity();
        C3Vector direction = endPosition - this->m_startModelPosition;
        float distance = std::sqrt(direction.y * direction.y + direction.z * direction.z + direction.x * direction.x);

        if (distance <= 0.0001f) {
            duration = 0.0f;
            this->m_endTime = this->m_startTime;
        } else {
            direction /= distance;
            float speed = seatRec->m_enterSpeed - (velocity.y * direction.y + velocity.z * direction.z + velocity.x * direction.x);

            if (speed <= 0.0001f) {
                duration = maxDuration;
            } else {
                duration = distance / speed;
                ClampInPlace(duration, minDuration, maxDuration);
            }

            this->m_endTime = this->m_startTime - static_cast<int32_t>(-1000.0f * duration);
        }
    } else {
        C3Vector position = this->m_unit->GetPosition();
        float dy = position.y - this->m_startModelPosition.y;
        float dx = position.x - this->m_startModelPosition.x;
        float dz = position.z - this->m_startModelPosition.z;
        duration = std::sqrt(dx * dx + dy * dy + dz * dz) / seatRec->m_exitSpeed;
        ClampInPlace(duration, minDuration, maxDuration);
        this->m_endTime = this->m_startTime - static_cast<int32_t>(-1000.0f * duration);
    }

    if (std::fabs(this->m_arcGravity) < 0.0001f) {
        if (minArcHeight > 0.0f) {
            this->m_arcGravity = 0.1f;
        } else if (maxArcHeight < 0.0f) {
            this->m_arcGravity = 0.1f;
        }
    }

    float arcScale;

    if (duration <= 0.0001f) {
        arcScale = 1.0f;
    } else {
        float height = this->m_arcGravity * duration * duration * 0.125f;

        if (height < minArcHeight) {
            arcScale = minArcHeight / height;
        } else if (maxArcHeight < height) {
            arcScale = maxArcHeight / height;
        } else {
            arcScale = 1.0f;
        }
    }

    this->m_arcScale = arcScale;

    if (this->m_unit->m_vehicleCamera) {
        this->m_unit->m_vehicleCamera->OnPassengerTransitionMotionBegin(time, CMath::fint_(duration * 1000.0f));
    }

    this->UpdateBlend(time, seatRec);

    if (vehicle && vehicle->m_fadeDuration && this->IsEntering()) {
        vehicle->DoFade(vehicle->GetBaseAlpha(), this->m_endTime - this->m_startTime);
    }
}

// VERIFY: 5375d8a2 manual
// OFFSET: 0x0074A7F0
void CVehiclePassenger_C::UpdateModel() {
    if ((this->m_state == 2 || this->m_state == 5) && !(this->m_flags & 0x1)) {
        return;
    }

    VehicleSeatRec* seatRec = this->m_seatRec;
    CGUnit_C* vehicle;

    if (this->m_transitionVehicleGuid) {
        vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_transitionVehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    } else {
        vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    this->m_flags &= ~0x10;
    CM2Model* model = this->m_unit->GetObjectModel();

    if (!model) {
        return;
    }

    if (!this->m_state) {
        if (model->m_attachParent) {
            model->DetachFromParent();
        }

        C3Vector normal = this->m_unit->m_movement->GetGroundNormal();
        float scale = this->m_unit->GetTrueScale();
        float facing = this->m_unit->GetFacing();
        model->SetWorldTransform(this->m_unit->GetPosition(), facing, scale, normal, 0xFFFFFFFF);
        return;
    }

    C3Vector start;

    if (this->m_state == 3 || !this->m_startRelativeGuid) {
        start = this->m_startModelPosition;
    } else {
        start = this->ComputeStartWorldPosition(this->m_startModelOffset, this->m_startModelPosition);
    }

    C44Matrix matrix;
    int32_t useMatrix = 0;
    C3Vector position(0.0f, 0.0f, 0.0f);
    C3Vector up(0.0f, 0.0f, 0.0f);
    float facing;
    float scale;
    CM2Model* parent;
    uint32_t attachment;

    if (!seatRec || !this->m_state) {
        position = this->m_unit->GetPosition();
        facing = this->m_unit->GetFacing();
        scale = this->m_unit->GetTrueScale();
        up = this->m_unit->m_movement->GetGroundNormal();
        parent = nullptr;
        attachment = 0;
    } else if (this->IsInTransitionMotion()) {
        float arc;

        if (std::fabs(this->m_arcGravity) <= 0.0001f) {
            arc = 0.0f;
        } else {
            float total = static_cast<int32_t>(this->m_endTime - this->m_startTime) * 0.001f;
            float elapsed = this->m_facingBlend * total;
            float gravity = this->m_arcGravity * elapsed * 0.5f;
            arc = (total * gravity - elapsed * gravity) * this->m_arcScale;
        }

        C3Vector end(0.0f, 0.0f, 0.0f);
        this->ComputeEndPos(vehicle, seatRec, end);
        float blend = this->m_facingBlend;
        start.x = end.x * blend + start.x * (1.0f - blend);
        start.y = end.y * blend + start.y * (1.0f - blend);
        start.z = blend * end.z + (1.0f - blend) * start.z;
        position = start;
        position.z = start.z + arc;
        facing = this->m_smoothFacing;
        scale = this->m_unit->GetTrueScale();
        up = C3Vector(0.0f, 0.0f, 1.0f);
        parent = nullptr;
        attachment = 0;
    } else if (this->m_state == 3) {
        if (!vehicle) {
            return;
        }

        if (!(this->m_flags & 0x20)) {
            this->ComputeSeatOffset(seatRec, model);
        }

        CM2Model* vehicleModel = vehicle->GetObjectModel();

        if (vehicleModel && vehicleModel->IsLoaded(0, 0)) {
            attachment = static_cast<uint32_t>(seatRec->m_attachmentID) >= 22 ? 0xFFFFFFFF : s_seatAttachments[seatRec->m_attachmentID];
            this->ComputeModelTransform(matrix, vehicleModel, vehicle, seatRec, attachment, this->m_unit->m_smoothTarget.m_value);
            parent = vehicleModel;
            useMatrix = 1;
        } else {
            position = this->m_unit->GetPosition();
            facing = this->m_unit->GetFacing();
            scale = this->m_unit->GetTrueScale();
            up = this->m_unit->GetGroundNormal();
            parent = nullptr;
            attachment = 0;
        }
    } else {
        if (!(this->m_flags & 0x20)) {
            this->ComputeSeatOffset(seatRec, model);
        }

        CGUnit_C* oldVehicle;
        VehicleSeatRec* oldSeat;
        CM2Model* oldModel;

        if (this->m_vehicleGuid && (oldVehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_vehicleGuid, TYPE_UNIT, __FILE__, __LINE__))) != nullptr && ((oldSeat = oldVehicle->GetVehicleSeatRec(this->m_seat)), (oldModel = oldVehicle->GetObjectModel()), oldSeat) && oldSeat->m_attachmentID >= 0 && oldModel && oldModel->IsLoaded(0, 0)) {
            attachment = static_cast<uint32_t>(seatRec->m_attachmentID) >= 22 ? 0xFFFFFFFF : s_seatAttachments[oldSeat->m_attachmentID];
            this->ComputeModelTransform(matrix, oldModel, vehicle, oldSeat, attachment, this->m_startTargetFacing);
            parent = oldModel;
            useMatrix = 1;
        } else {
            facing = this->m_startFacing;
            position = start;
            scale = this->m_unit->GetTrueScale();
            up = this->m_unit->GetGroundNormal();
            parent = nullptr;
            attachment = 0;
        }
    }

    if (model->m_attachParent != parent || model->m_attachmentId != attachment) {
        if (model->m_attachParent) {
            model->DetachFromParent();
        }

        if (parent) {
            model->AttachToParent(parent, attachment, nullptr, 0);
        }
    }

    if (useMatrix) {
        model->m_fFlags |= 0x8000;
        model->m_worldTransform = matrix;
    } else {
        model->SetWorldTransform(position, facing, scale, up, 0xFFFFFFFF);
    }

    if (this->m_unit->m_modelFlags & 0x40000) {
        this->m_flags |= 0x10;
    } else {
        this->m_flags &= ~0x10;
    }
}

// VERIFY: 47409945 manual
// OFFSET: 0x0074AD70
int32_t CVehiclePassenger_C::PostSetState(CGUnit_C* vehicle, VehicleSeatRec* seatRec, uint32_t time) {
    C3Vector endPosition(0.0f, 0.0f, 0.0f);

    if (this->m_state) {
        this->ComputeEndValues(vehicle, seatRec, time, endPosition);
    }

    uint32_t state = this->m_state;

    if (state && state != 3) {
        float dz = endPosition.z - this->m_startModelPosition.z;
        float dy = endPosition.y - this->m_startModelPosition.y;
        float dx = endPosition.x - this->m_startModelPosition.x;

        if (dx * dx + (dy * dy + dz * dz) > 22500.0f) {
            WOWGUID transport;
            transport.m_guid = this->m_unit->GetTransportGUID();

            if (state == 1 || state == 2) {
                this->SetState(3, vehicle, this->m_transitionSeat, seatRec, time, transport);
            } else {
                this->SetState(0, vehicle, this->m_transitionSeat, seatRec, time, transport);
            }

            this->m_flags |= 0x1;
        }
    }

    VehicleRec* vehicleRec = this->m_vehicleRec;
    WOWGUID guid = this->m_unit->m_obj->m_guid;

    if (ClntObjMgrGetActivePlayer() == guid || CGPartyInfo::IsPlayerOrPetInParty(guid) || CGRaidInfo::IsPlayerOrPetInRaid(guid)) {
        CVehiclePassenger_C::SendTransitionUIEvents(guid, vehicleRec, seatRec, this->m_prevState, this->m_state);
    }

    if (vehicle) {
        CGUnit_C* root = vehicle->GetRootVehicleUnit();

        if (root && (!this->m_state || !this->m_prevState || this->m_state == 3 || this->m_prevState == 3)) {
            CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

            if (player && player->GetRootVehicleUnit() == root) {
                FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_PASSENGERS_CHANGED, nullptr);
            }
        }
    }

    this->m_prevState = this->m_state;

    if (vehicle && vehicle->m_vehicle) {
        vehicle->m_vehicle->UpdateFreeSeatMask();
    }

    return this->m_state || (this->m_flags & 0x200);
}

// VERIFY: 6038651c auto
// OFFSET: 0x0074AF70
int32_t CVehiclePassenger_C::UpdateState(CGUnit_C* vehicle, VehicleSeatRec* seatRec, uint32_t time) {
    if (!(this->m_flags & 0x1) && !this->PostSetState(vehicle, seatRec, time)) {
        return 0;
    }

    if (this->m_state == 1 && static_cast<int32_t>(time - this->m_endTime) >= 0) {
        WOWGUID transport;
        transport.m_guid = this->m_unit->GetTransportGUID();
        this->SetState(2, vehicle, this->m_transitionSeat, seatRec, time, transport);

        if (!this->PostSetState(vehicle, seatRec, time)) {
            return 0;
        }
    }

    if (this->m_state == 2) {
        if (static_cast<int32_t>(time - this->m_endTime) < 0) {
            this->UpdateSmoothFacing();
        } else {
            WOWGUID transport;
            transport.m_guid = this->m_unit->GetTransportGUID();
            this->SetState(3, vehicle, this->m_transitionSeat, seatRec, time, transport);
        }
    }

    if (this->m_state == 4 && static_cast<int32_t>(time - this->m_endTime) >= 0) {
        WOWGUID transport;
        transport.m_guid = this->m_unit->GetTransportGUID();
        this->SetState(5, vehicle, 0xFF, seatRec, time, transport);

        if (!this->PostSetState(vehicle, seatRec, time)) {
            return 0;
        }
    }

    if (this->m_state == 5) {
        if (static_cast<int32_t>(time - this->m_endTime) < 0) {
            this->UpdateSmoothFacing();
        } else {
            WOWGUID transport;
            transport.m_guid = this->m_unit->GetTransportGUID();
            this->SetState(0, vehicle, 0xFF, seatRec, time, transport);
        }
    }

    uint32_t flags = this->m_flags;

    if ((flags & 0x8) && static_cast<int32_t>(time - this->m_stopMovementTime - 2000) >= 0) {
        this->m_flags = flags & ~0x8;
    }

    return 1;
}

// VERIFY: 8024836e manual
// OFFSET: 0x0074B200
void CVehiclePassenger_C::OnUpdateTransport(WOWGUID transport, uint8_t seat, CGUnit_C* vehicle, int32_t deferred) {
    if (transport) {
        if (vehicle) {
            this->m_vehicleRec = vehicle->m_vehicle ? vehicle->m_vehicle->m_vehicleRec : nullptr;
            this->m_seatRec = vehicle->m_vehicle ? vehicle->m_vehicle->GetVehicleSeatRec(seat) : nullptr;
        } else {
            this->m_vehicleRec = nullptr;
            this->m_seatRec = nullptr;
        }
    }

    CGUnit_C* root;

    if (vehicle && (root = vehicle->FindRootVehicle(nullptr)) != nullptr) {
        this->StoreRootVehicle(root->m_obj->m_guid);
    } else {
        this->StoreRootVehicle(WOWGUID());
    }

    uint32_t high = static_cast<uint32_t>(transport.m_guid >> 32);
    int32_t exiting = (high & 0xF0F00000) != 0xF0500000 && ((high & 0xF0000000) || !((high & 0xF07FFFFF) | static_cast<uint32_t>(transport.m_guid)));
    int32_t allowed = this->m_unit->VehiclePassengerIsTransitionAllowed(this->m_seatRec, exiting);

    if (this->m_vehicleGuid && !vehicle && CVehiclePassenger_C::s_pendingCount) {
        CVehiclePassenger_C::AddPendingRescueTransition(this->m_unit->m_obj->m_guid, transport, seat, allowed);
    } else if (!deferred) {
        this->BeginTransition(transport, seat, allowed);
    }
}
