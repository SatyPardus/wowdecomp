#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_VEHICLE_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_VEHICLE_C_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Object/Passenger.h"

class CM2Model;
class CClientObjCreate;
class CGUnit_C;
class CVehiclePassenger_C;
class VehicleRec;
class VehicleSeatRec;

class CVehicle_C;

class CVehicleHeap {
    public:
    /* 0000 */ uint32_t m_heapId;

    void Delete(CVehicle_C* vehicle);
};

class CVehicle_C {
    public:
    static void EnterWorld();
    struct BoneAnimListener {
        /* 0000 */ WOWGUID m_guid;
        /* 0008 */ uint32_t m_boneId;
        /* 000C */ void (*m_callback)(CGUnit_C* unit, int32_t& a2, TSGrowableArray<CVehiclePassenger_C*>& passengers);
    };

    static CVehicleHeap* s_vehiclePool;

    static void InitSystem();
    static void UninitSystem();
    static void EjectPassenger(WOWGUID guid);
    static CVehicle_C* Create(const CClientObjCreate* create, CGUnit_C* unit, int32_t vehicleId);

    /* 0000 */ uint32_t m_memHandle;
    /* 0004 */ CGUnit_C* m_unit;
    /* 0008 */ uint32_t m_flags;
    /* 000C */ VehicleRec* m_vehicleRec;
    /* 0010 */ C44Matrix m_worldMatrix;
    /* 0050 */ float m_rawFacing;
    /* 0054 */ float m_largestPassengerBoundsRadius;
    /* 0058 */ uint32_t m_playingBoneAnims [2];
    /* 0060 */ BoneAnimListener m_boneAnimListeners [16];
    /* 0160 */ WOWGUID m_forceFacingGuid;
    /* 0168 */ uint32_t m_forceFacingBone;
    /* 016C */ uint8_t m_mirroringAnimsMask;
    /* 016D */ uint8_t m_freeSeatMask;
    /* 0170 */ TSExplicitList<CPassenger, offsetof(CPassenger, m_link)> m_passengers;
    /* 017C */ uint32_t unused_017C;

    CVehicle_C();
    VehicleSeatRec* GetVehicleSeatRec(uint8_t seat) const;
    uint32_t IsSpecialBoneAnimPlaying() const;
    int32_t InstallBoneAnimListener(WOWGUID guid, uint32_t boneId, void (*callback)(CGUnit_C* unit, int32_t& a2, TSGrowableArray<CVehiclePassenger_C*>& passengers));
    void UninstallBoneAnimListener(WOWGUID guid);
    void RemoveBoneAnimListener(uint32_t boneId, WOWGUID guid);
    int32_t CanPlayBoneAnim(uint32_t animId) const;
    void SetBoneAnimForceFacing(WOWGUID guid, uint32_t boneId);
    bool IsMirroringAnimsToPassengers() const;
    void OnPassengerStartMirroringAnims(uint8_t seat);
    void OnPassengerStopMirroringAnims(uint8_t seat);
    int32_t IsSuppressingCameraPitchWhileMouseAiming() const;
    int32_t HasHighPriorityCastingAnim() const;
    uint32_t IsBoneAnimPlaying(uint32_t bone) const;
    void BoneAnimEndHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time);
    void PlayBoneAnimOrFallback(CM2Model* model, uint32_t boneId);
    int32_t HasHighPriorityAnim() const;
    int32_t GetVirtualSeatCount() const;
    int32_t GetVehicleAndSeatFromVirtualSeatsRemaining(int32_t& remaining, const CVehicle_C*& vehicle, uint8_t& seat) const;
    CGUnit_C* FindPassengerUnit(uint8_t seat);
    void BuildWorldMatrix(const C44Matrix* parent);
    void UpdateLargestPassengerBoundsRadius();
    void UpdateFreeSeatMask();
    int32_t GetVehicleAndSeatFromVirtualSeat(int32_t virtualSeat, const CVehicle_C*& vehicle, uint8_t& seat) const;
    int32_t GetVirtualSeatInfo(int32_t virtualSeat, const char*& type, const char*& occupant, const char*& realm, int32_t& canEject, int32_t& canSwitch) const;
    int32_t AddPassenger(CPassenger* passenger);
    void ReceiveVehicleRecIDFromServer(int32_t vehicleId);
    void UpdateWorldMatrix();
    CVehicle_C* FindVehicleToPlayBoneAnim();
    void StartMirroringAnimsToPassengers();
    int32_t PlayBoneAnim(uint32_t boneId, uint32_t animId);
    void HandleBoneAnimGrabEvent(uint32_t boneId, uint32_t eventId);
    void HandleBoneAnimThrowEvent(uint32_t boneId, uint32_t eventId);
    void RemoveAllPassengersLocally();
    void Destroy(int32_t removePassengers);
};

#endif
