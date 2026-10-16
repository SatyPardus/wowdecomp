#include "Vehicle_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/Passenger.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00AD6734
static const char* s_vehicleSeatTypes[3] = { "None", "Root", "Child" };

// STATIC: 0x00CA1604
CVehicleHeap* CVehicle_C::s_vehiclePool;

// STATIC: 0x00CA1608
static int32_t s_inBoneAnimEnd;

// VERIFY: 94e658f9 auto
// OFFSET: 0x00756C90
uint32_t CVehicle_C::IsSpecialBoneAnimPlaying() const {
    return this->m_playingBoneAnims[0] & 0x4000000;
}

// VERIFY: ce2db0fa manual
// OFFSET: 0x00756D10
int32_t CVehicle_C::InstallBoneAnimListener(WOWGUID guid, uint32_t boneId, void (*callback)(CGUnit_C* unit, int32_t& a2, TSGrowableArray<CVehiclePassenger_C*>& passengers)) {
    if (boneId >= 35) {
        boneId = 26;
    }

    uint32_t i = 0;

    while (this->m_boneAnimListeners[i].m_guid) {
        if (++i >= 16) {
            return 0;
        }
    }

    this->m_boneAnimListeners[i].m_guid = guid;
    this->m_boneAnimListeners[i].m_boneId = boneId;
    this->m_boneAnimListeners[i].m_callback = callback;
    return 1;
}

// VERIFY: b756abbb manual
// OFFSET: 0x00756D70
void CVehicle_C::UninstallBoneAnimListener(WOWGUID guid) {
    for (uint32_t i = 0; i < 16; i++) {
        if (this->m_boneAnimListeners[i].m_guid == guid) {
            this->m_boneAnimListeners[i].m_guid = WOWGUID();
        }
    }
}

// VERIFY: f7a08bf8 auto
// OFFSET: 0x00756DB0
void CVehicle_C::SetBoneAnimForceFacing(WOWGUID guid, uint32_t boneId) {
    if (boneId >= 35) {
        boneId = 26;
    }

    this->m_forceFacingGuid = guid;
    this->m_forceFacingBone = boneId;
}

// VERIFY: 647ff059 manual
// OFFSET: 0x00756DE0
bool CVehicle_C::IsMirroringAnimsToPassengers() const {
    return this->m_mirroringAnimsMask != 0;
}

// VERIFY: 3e904f3d manual
// OFFSET: 0x00756DF0
void CVehicle_C::OnPassengerStartMirroringAnims(uint8_t seat) {
    this->m_mirroringAnimsMask |= 1 << seat;
}

// VERIFY: 6239aac9 manual
// OFFSET: 0x00756E10
void CVehicle_C::OnPassengerStopMirroringAnims(uint8_t seat) {
    this->m_mirroringAnimsMask &= ~(1 << seat);
}

// VERIFY: 2124a7e1 manual
// OFFSET: 0x00756E30
void CVehicle_C::InitSystem() {
    delete s_vehiclePool;

    CVehicleHeap* pool = static_cast<CVehicleHeap*>(SMemAlloc(sizeof(CVehicleHeap), __FILE__, __LINE__, 0));

    if (pool) {
        pool->m_heapId = ObjectAllocAddHeap(sizeof(CVehicle_C), 16, "Vehicle", true);
        s_vehiclePool = pool;
    } else {
        s_vehiclePool = nullptr;
    }
}

// VERIFY: a31dcf61 manual
// OFFSET: 0x00756E90
void CVehicle_C::UninitSystem() {
    if (s_vehiclePool) {
        delete s_vehiclePool;
        s_vehiclePool = nullptr;
    }
}

// VERIFY: 749d5086 auto
// OFFSET: 0x00756EC0
VehicleSeatRec* CVehicle_C::GetVehicleSeatRec(uint8_t seat) const {
    if (seat >= 8 || !this->m_vehicleRec) {
        return nullptr;
    }

    return g_vehicleSeatDB.GetRecord(this->m_vehicleRec->m_seatID[seat]);
}

// VERIFY: 244d9d64 auto
// OFFSET: 0x00756F00
int32_t CVehicle_C::IsSuppressingCameraPitchWhileMouseAiming() const {
    if (!(this->m_unit->m_movement->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) && this->m_vehicleRec && (this->m_vehicleRec->m_flags & 0x40040000) == 0x40000) {
        return 1;
    }

    return 0;
}

// VERIFY: 62fd8da2 auto
// OFFSET: 0x00756CD0
uint32_t CVehicle_C::IsBoneAnimPlaying(uint32_t bone) const {
    if (bone == 26) {
        return 0;
    }

    if (bone >= 35) {
        bone = 26;
    }

    return this->m_playingBoneAnims[bone >> 5] & (1 << (bone & 0x1F));
}

// VERIFY: 0f448e4f manual
// OFFSET: 0x00756F40
int32_t CVehicle_C::HasHighPriorityCastingAnim() const {
    if (this->m_vehicleRec && (this->m_vehicleRec->m_flags & 0x10000)) {
        CGUnit_C* unit = this->m_unit;

        if ((unit->m_animationState & 0x400) || unit->m_specialIdleAnim != -1) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: cfe65bc9 manual
// OFFSET: 0x007571C0
int32_t CVehicle_C::HasHighPriorityAnim() const {
    if (this->m_playingBoneAnims[0] & 0x4000000) {
        return 1;
    }

    if (!this->m_vehicleRec || !(this->m_vehicleRec->m_flags & 0x10000)) {
        return 0;
    }

    CGUnit_C* unit = this->m_unit;

    if (!(unit->m_animationState & 0x400) && unit->m_specialIdleAnim == -1) {
        return 0;
    }

    return 1;
}

// VERIFY: e4293468 manual
// OFFSET: 0x00757200
void CVehicle_C::EjectPassenger(WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_EJECT_PASSENGER));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 010fc2ad manual
// OFFSET: 0x00757470
int32_t CVehicle_C::GetVirtualSeatCount() const {
    if (!this->m_vehicleRec) {
        return 0;
    }

    int32_t count = this->m_vehicleRec->m_seatID[0] > 0;

    if (this->m_vehicleRec->m_seatID[1] > 0) {
        count++;
    }

    if (this->m_vehicleRec->m_seatID[2] > 0) {
        count++;
    }

    if (this->m_vehicleRec->m_seatID[3] > 0) {
        count++;
    }

    if (this->m_vehicleRec->m_seatID[4] > 0) {
        count++;
    }

    if (this->m_vehicleRec->m_seatID[5] > 0) {
        count++;
    }

    if (this->m_vehicleRec->m_seatID[6] > 0) {
        count++;
    }

    if (this->m_vehicleRec->m_seatID[7] > 0) {
        count++;
    }

    for (CPassenger* passenger = const_cast<CVehicle_C*>(this)->m_passengers.Head(); passenger; passenger = const_cast<CVehicle_C*>(this)->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));
        CVehicle_C* vehicle = unit->m_vehicle;

        if (vehicle && vehicle->m_vehicleRec && !(unit->m_obj->m_type & TYPEMASK_PLAYER)) {
            count += vehicle->GetVirtualSeatCount() - 1;
        }
    }

    return count;
}

// VERIFY: e21cc1be manual
// OFFSET: 0x00757550
int32_t CVehicle_C::GetVehicleAndSeatFromVirtualSeatsRemaining(int32_t& remaining, const CVehicle_C*& vehicle, uint8_t& seat) const {
    if (!this->m_vehicleRec) {
        return 0;
    }

    CGUnit_C* seatUnits[8] = {};

    for (CPassenger* passenger = const_cast<CVehicle_C*>(this)->m_passengers.Head(); passenger; passenger = const_cast<CVehicle_C*>(this)->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));
        CVehicle_C* unitVehicle = unit->m_vehicle;

        if (unitVehicle && unitVehicle->m_vehicleRec && !(unit->m_obj->m_type & TYPEMASK_PLAYER)) {
            seatUnits[static_cast<uint8_t>(unit->m_movementData.m_transportSeat)] = unit;
        }
    }

    for (uint32_t i = 0; i < 8; i++) {
        if (seatUnits[i]) {
            if (seatUnits[i]->m_vehicle->GetVehicleAndSeatFromVirtualSeatsRemaining(remaining, vehicle, seat)) {
                return 1;
            }
        } else if (this->m_vehicleRec->m_seatID[i] > 0) {
            if (!remaining) {
                vehicle = this;
                seat = static_cast<uint8_t>(i);
                return 1;
            }

            remaining--;
        }
    }

    return 0;
}

// VERIFY: d1d59b81 manual
// OFFSET: 0x00757680
CGUnit_C* CVehicle_C::FindPassengerUnit(uint8_t seat) {
    for (CPassenger* passenger = this->m_passengers.Head(); passenger; passenger = this->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (static_cast<uint8_t>(unit->m_movementData.m_transportSeat) == seat) {
            return unit;
        }
    }

    return nullptr;
}

// VERIFY: e0aa5b39 manual
// OFFSET: 0x007579E0
int32_t CVehicle_C::GetVirtualSeatInfo(int32_t virtualSeat, const char*& type, const char*& occupant, const char*& realm, int32_t& canEject, int32_t& canSwitch) const {
    const CVehicle_C* vehicle;
    uint8_t seat;

    if (!this->GetVehicleAndSeatFromVirtualSeatsRemaining(virtualSeat, vehicle, seat)) {
        return 0;
    }

    VehicleSeatRec* seatRec;

    if (seat >= 8 || !vehicle->m_vehicleRec || !(seatRec = g_vehicleSeatDB.GetRecord(vehicle->m_vehicleRec->m_seatID[seat]))) {
        return 0;
    }

    if (seatRec->m_flags & 0x800) {
        type = vehicle == this ? s_vehicleSeatTypes[1] : s_vehicleSeatTypes[2];
    } else {
        type = s_vehicleSeatTypes[0];
    }

    realm = nullptr;
    CGUnit_C* passenger = const_cast<CVehicle_C*>(vehicle)->FindPassengerUnit(seat);

    if (passenger) {
        occupant = passenger->GetUnitName(&realm, 1);
    } else {
        occupant = nullptr;
    }

    canEject = seatRec->m_flagsB & 0x20;
    canSwitch = (seatRec->m_flags >> 26) & 1;
    return 1;
}

// VERIFY: d1b6b7db manual
// OFFSET: 0x00757AE0
CVehicle_C::CVehicle_C() {
}

// VERIFY: bbf75d50 manual
// OFFSET: 0x00757BE0
void CVehicle_C::BuildWorldMatrix(const C44Matrix* parent) {
    C44Matrix* matrix = &this->m_worldMatrix;
    matrix->d3 = 1.0f;
    matrix->c2 = 1.0f;
    matrix->b1 = 1.0f;
    matrix->a0 = 1.0f;
    matrix->d2 = 0.0f;
    matrix->d1 = 0.0f;
    matrix->d0 = 0.0f;
    matrix->c3 = 0.0f;
    matrix->c1 = 0.0f;
    matrix->c0 = 0.0f;
    matrix->b3 = 0.0f;
    matrix->b2 = 0.0f;
    matrix->b0 = 0.0f;
    matrix->a3 = 0.0f;
    matrix->a2 = 0.0f;
    matrix->a1 = 0.0f;

    C3Vector position = this->m_unit->GetRawPosition();
    matrix->d0 = position.x;
    matrix->d1 = position.y;
    matrix->d2 = position.z;
    matrix->RotateAroundZ(this->m_unit->GetRawFacing());

    if (parent) {
        *matrix *= *parent;
    }

    for (CPassenger* passenger = this->m_passengers.Head(); passenger; passenger = this->m_passengers.Next(passenger)) {
        WOWGUID guid = *passenger->m_ownerGuid;
        uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);

        if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid)))) {
            CVehicle_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__))->m_vehicle;

            if (vehicle && vehicle->m_vehicleRec) {
                vehicle->BuildWorldMatrix(matrix);
            }
        }
    }
}

// VERIFY: 1bf60603 manual
// OFFSET: 0x00757D10
void CVehicle_C::UpdateLargestPassengerBoundsRadius() {
    CVehicle_C* vehicle = this;

    while (true) {
        float previous = vehicle->m_largestPassengerBoundsRadius;
        vehicle->m_largestPassengerBoundsRadius = 0.0f;

        for (CPassenger* passenger = vehicle->m_passengers.Head(); passenger; passenger = vehicle->m_passengers.Next(passenger)) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));
            CM2Model* model = unit->GetObjectModel();

            if (model && model->IsLoaded(0, 0)) {
                float radius = model->GetBoundingSphere().r;
                CVehicle_C* unitVehicle = unit->m_vehicle;

                if (unitVehicle && unitVehicle->m_vehicleRec) {
                    radius = radius + unitVehicle->m_largestPassengerBoundsRadius + unitVehicle->m_largestPassengerBoundsRadius;
                }

                if (radius > vehicle->m_largestPassengerBoundsRadius) {
                    vehicle->m_largestPassengerBoundsRadius = radius;
                }
            }
        }

        if (vehicle->m_largestPassengerBoundsRadius == previous) {
            break;
        }

        CGUnit_C* unit = vehicle->m_unit;
        CVehiclePassenger_C* vehiclePassenger = unit->m_vehiclePassenger;

        if (!vehiclePassenger || vehiclePassenger->m_state != 3) {
            break;
        }

        CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ unit->GetTransportGUID() }, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (!transport) {
            break;
        }

        CVehicle_C* transportVehicle = transport->m_vehicle;

        if (!transportVehicle || !transportVehicle->m_vehicleRec) {
            break;
        }

        vehicle = transportVehicle;
    }
}

// VERIFY: bf154031 manual
// OFFSET: 0x00757EF0
void CVehicle_C::UpdateFreeSeatMask() {
    this->m_freeSeatMask = 0;

    if (!this->m_vehicleRec) {
        return;
    }

    uint8_t occupied = 0;

    for (CPassenger* passenger = this->m_passengers.Head(); passenger; passenger = this->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit) {
            occupied |= 1 << static_cast<uint8_t>(unit->m_movementData.m_transportSeat);
        }
    }

    uint8_t bit = 1;

    for (uint32_t i = 0; i < 8; i++) {
        if (this->m_vehicleRec->m_seatID[i] && !(occupied & bit)) {
            this->m_freeSeatMask |= bit;
        }

        bit *= 2;
    }
}

// VERIFY: 6cbe8d0f auto
// OFFSET: 0x00757960
int32_t CVehicle_C::GetVehicleAndSeatFromVirtualSeat(int32_t virtualSeat, const CVehicle_C*& vehicle, uint8_t& seat) const {
    return this->GetVehicleAndSeatFromVirtualSeatsRemaining(virtualSeat, vehicle, seat);
}

// VERIFY: bfa3b17f manual
// OFFSET: 0x00757980
int32_t CVehicle_C::AddPassenger(CPassenger* passenger) {
    if (!(passenger->m_passengerFlags & 0x1)) {
        return 0;
    }

    passenger->m_link.Unlink();
    this->m_passengers.LinkToTail(passenger);
    return 1;
}

// VERIFY: c435c0c9 auto
// OFFSET: 0x00757BB0
void CVehicle_C::Destroy(int32_t removePassengers) {
    if (removePassengers) {
        this->RemoveAllPassengersLocally();
    }

    CVehicle_C::s_vehiclePool->Delete(this);
}

// VERIFY: f33a02d3 manual
// OFFSET: 0x00757FA0
CVehicle_C* CVehicle_C::Create(const CClientObjCreate* create, CGUnit_C* unit, int32_t vehicleId) {
    CVehicle_C* vehicle;
    uint32_t memHandle;
    void* memory = nullptr;

    if (ObjectAlloc(s_vehiclePool->m_heapId, &memHandle, &memory, false)) {
        vehicle = static_cast<CVehicle_C*>(memory);

        if (vehicle) {
            new (vehicle) CVehicle_C();
        }

        vehicle->m_memHandle = memHandle;
    } else {
        vehicle = nullptr;
    }

    vehicle->m_flags = 0;
    vehicle->m_unit = unit;

    float facing;

    if (create) {
        vehicle->m_vehicleRec = g_vehicleDB.GetRecord(create->m_vehicleId);
        vehicle->m_worldMatrix.Translate(create->m_move.m_status.m_position);
        vehicle->m_worldMatrix.RotateAroundZ(create->m_move.m_status.m_facing);
        facing = create->m_vehicleOrientation;
    } else {
        vehicle->m_vehicleRec = g_vehicleDB.GetRecord(vehicleId);
        vehicle->m_worldMatrix.Translate(unit->GetPosition());
        vehicle->m_worldMatrix.RotateAroundZ(unit->GetFacing());
        facing = unit->GetRawFacing();
    }

    vehicle->m_rawFacing = facing;
    vehicle->m_largestPassengerBoundsRadius = 0.0f;
    vehicle->UpdateFreeSeatMask();
    vehicle->m_playingBoneAnims[0] = 0;
    vehicle->m_playingBoneAnims[1] = 0;

    for (uint32_t i = 0; i < 16; i++) {
        vehicle->m_boneAnimListeners[i].m_guid = WOWGUID();
    }

    vehicle->m_forceFacingGuid = WOWGUID();
    vehicle->m_forceFacingBone = 0xFFFFFFFF;
    vehicle->m_mirroringAnimsMask = 0;
    return vehicle;
}

// VERIFY: 411fcddb auto
// OFFSET: 0x007580F0
void CVehicle_C::ReceiveVehicleRecIDFromServer(int32_t vehicleId) {
    this->m_vehicleRec = g_vehicleDB.GetRecord(vehicleId);
    this->UpdateFreeSeatMask();
}

// VERIFY: 8be84ede manual
// OFFSET: 0x00758130
void CVehicle_C::UpdateWorldMatrix() {
    if (!this->m_unit->GetTransportGUID()) {
        this->BuildWorldMatrix(nullptr);
        return;
    }

    CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ this->m_unit->GetTransportGUID() }, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (transport) {
        C44Matrix transportMatrix = transport->GetMatrix();
        this->BuildWorldMatrix(&transportMatrix);
    }
}

// VERIFY: d821daf4 manual
// OFFSET: 0x00757B40
void CVehicleHeap::Delete(CVehicle_C* vehicle) {
    vehicle->~CVehicle_C();
    ObjectFree(this->m_heapId, vehicle->m_memHandle);
}

// VERIFY: bd1ef250 manual
// OFFSET: 0x00757000
CVehicle_C* CVehicle_C::FindVehicleToPlayBoneAnim() {
    CGUnit_C* unit = this->m_unit;

    while (unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
        CGUnit_C* vehicleUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(unit->m_vehiclePassenger->m_transitionVehicleGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (!vehicleUnit) {
            break;
        }

        unit = vehicleUnit;
    }

    return unit->m_vehicle ? unit->m_vehicle : this;
}

// VERIFY: e727cd63 manual
// OFFSET: 0x00757E70
void CVehicle_C::StartMirroringAnimsToPassengers() {
    for (CPassenger* passenger = this->m_passengers.Head(); passenger; passenger = this->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
            unit->m_vehiclePassenger->StartMirroringVehicleAnims();
        }
    }
}

// VERIFY: e209f673 manual
// OFFSET: 0x00756F80
int32_t CVehicle_C::PlayBoneAnim(uint32_t boneId, uint32_t animId) {
    CM2Model* model = this->m_unit->GetObjectModel();

    if (!model || animId > 505) {
        return 0;
    }

    uint32_t slot = boneId >= 35 ? 26 : boneId;
    this->m_unit->SetKeyBoneSequence(model, slot == 26 ? 0xFFFFFFFF : slot, animId, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
    this->m_playingBoneAnims[slot >> 5] |= 1 << (slot & 0x1F);
    this->m_flags |= 0x1;
    return 1;
}

// VERIFY: 039e64eb auto
// OFFSET: 0x00756CA0
int32_t CVehicle_C::CanPlayBoneAnim(uint32_t animId) const {
    if (!this->m_unit->GetObjectModel()) {
        return 0;
    }

    return animId <= 0x1F9;
}

// VERIFY: 86ddcfee manual
// OFFSET: 0x00757060
void CVehicle_C::HandleBoneAnimGrabEvent(uint32_t boneId, uint32_t eventId) {
    if (boneId >= 35) {
        boneId = 26;
    }

    for (uint32_t i = 0; i < 16; i++) {
        BoneAnimListener& listener = this->m_boneAnimListeners[i];

        if (!listener.m_guid || listener.m_boneId != boneId) {
            continue;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(listener.m_guid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->IsWaitingForVehicleTransitionAnimGrabEvent(eventId)) {
            unit->m_vehiclePassenger->StopWaitingForVehicleTransitionAnim();
            listener.m_guid = WOWGUID();
        }
    }
}

// VERIFY: b18617fe manual
// OFFSET: 0x007570F0
void CVehicle_C::HandleBoneAnimThrowEvent(uint32_t boneId, uint32_t eventId) {
    if (boneId >= 35) {
        boneId = 26;
    }

    for (uint32_t i = 0; i < 16; i++) {
        BoneAnimListener& listener = this->m_boneAnimListeners[i];

        if (!listener.m_guid || listener.m_boneId != boneId) {
            continue;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(listener.m_guid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->IsWaitingForVehicleTransitionAnimThrowEvent(eventId)) {
            unit->m_vehiclePassenger->StopWaitingForVehicleTransitionAnim();
            listener.m_guid = WOWGUID();
        }
    }
}

// VERIFY: 35671fad manual
// OFFSET: 0x00757180
void CVehicle_C::EnterWorld() {
    CGUnit_C::UpdateVehicleUISliders(nullptr, nullptr);
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && player->m_vehicle && player->m_vehicle->m_vehicleRec) {
        player->SignalPlayerGainsVehicleDataEvent();
    }
}

// VERIFY: 269d0959 auto
// OFFSET: 0x00757420
void CVehicle_C::PlayBoneAnimOrFallback(CM2Model* model, uint32_t boneId) {
    uint32_t bone = boneId == 26 ? 0xFFFFFFFF : boneId;

    if (!this->m_unit->UnsetKeyBoneSequence(model, bone, 1, 1, 0)) {
        this->BoneAnimEndHandler(model, bone, 0xFFFFFFFF, 1, 0);
        this->m_unit->UpdateBaseAnimation(0, -1);
    }
}

// VERIFY: f94bb593 manual
// OFFSET: 0x007577E0
void CVehicle_C::RemoveBoneAnimListener(uint32_t boneId, WOWGUID guid) {
    CM2Model* model = this->m_unit->GetObjectModel();

    if (!model) {
        return;
    }

    if (boneId >= 35) {
        boneId = 26;
    }

    this->UninstallBoneAnimListener(guid);
    uint32_t i;

    for (i = 0; i < 16; i++) {
        if (this->m_boneAnimListeners[i].m_guid && this->m_boneAnimListeners[i].m_boneId == boneId) {
            break;
        }
    }

    if (i != 16 || !((1 << (boneId & 0x1F)) & this->m_playingBoneAnims[boneId >> 5])) {
        return;
    }

    if (boneId == 26) {
        boneId = 0xFFFFFFFF;
    }

    if (!this->m_unit->UnsetKeyBoneSequence(model, boneId, 1, 1, 0)) {
        this->BoneAnimEndHandler(model, boneId, 0xFFFFFFFF, 1, 0);
        this->m_unit->UpdateBaseAnimation(0, -1);
    }
}

// VERIFY: e3016364 manual
// OFFSET: 0x00757280
void CVehicle_C::BoneAnimEndHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time) {
    if (interrupted && s_inBoneAnimEnd) {
        return;
    }

    if (boneId >= 35) {
        boneId = 26;
    }

    TSGrowableArray<CVehiclePassenger_C*> passengers;
    int32_t handled = 0;

    for (int32_t i = 0; i < 16; i++) {
        BoneAnimListener& listener = this->m_boneAnimListeners[i];

        if (!listener.m_guid || listener.m_boneId != boneId) {
            continue;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(listener.m_guid, TYPEMASK_UNIT, __FILE__, __LINE__));
        int32_t keep = 0;

        if (unit && listener.m_callback && (listener.m_callback(unit, keep, passengers), keep)) {
            handled = 1;
        } else {
            listener.m_guid = WOWGUID{ 0 };
        }
    }

    if (interrupted || !handled) {
        this->m_playingBoneAnims[boneId >> 5] &= ~(1 << (boneId & 0x1F));
    }

    if (boneId == this->m_forceFacingBone) {
        this->m_forceFacingGuid = WOWGUID{ 0 };
        this->m_forceFacingBone = 0xFFFFFFFF;
    }

    if (!interrupted) {
        if (handled) {
            s_inBoneAnimEnd = 1;
            this->m_unit->SetKeyBoneSequence(model, boneId == 26 ? 0xFFFFFFFF : boneId, sequenceId, 0xFFFFFFFF, time, 1.0f, 1, 1, 0);
            s_inBoneAnimEnd = 0;
        } else if (boneId == 26) {
            this->m_unit->UpdateBaseAnimation(0, -1);
        } else {
            this->PlayBoneAnimOrFallback(model, boneId);
        }
    }

    for (uint32_t i = 0; i < passengers.Count(); i++) {
        passengers[i]->StopWaitingForVehicleTransitionAnim();
    }
}

// VERIFY: 26bcc49f auto
// OFFSET: 0x007576E0
void CVehicle_C::RemoveAllPassengersLocally() {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    uint32_t time = FrameTime::s_curTimeMs;
    CPassenger* passenger = this->m_passengers.Head();

    while (passenger) {
        CPassenger* next = this->m_passengers.Next(passenger);

        if (camera->m_targetGUID == *passenger->m_ownerGuid) {
            camera->MakeRelativeTo(WOWGUID{ 0 });
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPEMASK_UNIT, __FILE__, __LINE__));
        CMovement_C* movement = static_cast<CMovement_C*>(passenger);

        if (unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
            movement->OnFallLocal(time);
        } else {
            movement->TryStartFallingLocal();
        }

        unit->m_clientFlags |= 0x20000000;
        movement->ForceSetTransport(WOWGUID{ 0 }, 0xFF, 1);
        unit->m_clientFlags &= ~0x20000000;
        passenger = next;
    }
}
