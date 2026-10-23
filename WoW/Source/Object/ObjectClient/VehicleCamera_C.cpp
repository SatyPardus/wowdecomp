#include "VehicleCamera_C.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/WorldClient/World.h"
#include <cmath>

// STATIC: 0x00CA1658
CVehicleCameraHeap* CVehicleCamera_C::s_cameraPool;

// STATIC: 0x00CA165C
uint32_t CVehicleCamera_C::s_cameraCount;

// VERIFY: 700b924a auto
// OFFSET: 0x00759130
int32_t CVehicleCamera_C::ConvertFromRawToWorld(float& facing, CGObject_C* relativeTo) {
    if (!relativeTo) {
        return 0;
    }

    facing = relativeTo->GetFacing() + facing;
    return 1;
}

// VERIFY: 6565d38c manual
// OFFSET: 0x00759160
void CVehicleCamera_C::InitSystem() {
    delete s_cameraPool;

    CVehicleCameraHeap* pool = static_cast<CVehicleCameraHeap*>(SMemAlloc(sizeof(CVehicleCameraHeap), __FILE__, __LINE__, 0));

    if (pool) {
        pool->m_heapId = ObjectAllocAddHeap(sizeof(CVehicleCamera_C), 4, "Vehicle Camera", true);
        s_cameraPool = pool;
    } else {
        s_cameraPool = nullptr;
    }

    s_cameraCount = 0;
}

// VERIFY: cea6750e manual
// OFFSET: 0x007591D0
void CVehicleCamera_C::UninitSystem() {
    if (s_cameraPool) {
        delete s_cameraPool;
        s_cameraPool = nullptr;
        s_cameraCount = 0;
    }
}

// VERIFY: f89c4583 manual
// OFFSET: 0x007599D0
uint64_t CVehicleCamera_C::GetRelativeTo() const {
    WOWGUID target = this->m_targetGuid;

    if (target == this->m_unit->m_obj->m_guid) {
        return this->m_relativeToGuid.m_guid;
    }

    while (true) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return 0;
        }

        CVehicleCamera_C* camera = unit->m_vehicleCamera;

        if (!camera || !camera->m_state) {
            return unit->GetTransportGUID();
        }

        target = camera->m_targetGuid;

        if (target == camera->m_unit->m_obj->m_guid) {
            return camera->m_relativeToGuid.m_guid;
        }
    }
}

// VERIFY: db488c50 manual
// OFFSET: 0x00759AE0
int32_t CVehicleCamera_C::WouldAddingNextCameraCreateLoop(const CVehicleCamera_C* next) const {
    WOWGUID guid = next->m_nextCameraGuid;

    if (!guid) {
        return 0;
    }

    if (guid == this->m_unit->m_obj->m_guid) {
        return 1;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && unit->m_vehicleCamera && this->WouldAddingNextCameraCreateLoop(unit->m_vehicleCamera)) {
        return 1;
    }

    return 0;
}

// VERIFY: cfb3b17c manual
// OFFSET: 0x00759B50
int32_t CVehicleCamera_C::ConvertFromRawOrOffsetToWorld(C3Vector& position, CGObject_C* relativeTo) {
    if (!relativeTo) {
        return 0;
    }

    if (relativeTo->m_obj->m_type & TYPE_UNIT) {
        C3Vector origin = relativeTo->GetPosition();
        position.x = origin.x + position.x;
        position.y = origin.y + position.y;
        position.z = origin.z + position.z;
        return 1;
    }

    position *= relativeTo->GetMatrix();
    return 1;
}

// VERIFY: b4d06088 manual
// OFFSET: 0x00759BD0
int32_t CVehicleCamera_C::ConvertFromWorldToRawOrOffset(C3Vector& position, CGObject_C* relativeTo) {
    if (!relativeTo) {
        return 0;
    }

    if (relativeTo->m_obj->m_type & TYPE_UNIT) {
        C3Vector origin = relativeTo->GetPosition();
        position.x = position.x - origin.x;
        position.y = position.y - origin.y;
        position.z = position.z - origin.z;
        return 1;
    }

    position *= relativeTo->GetMatrix().AffineInverse();
    return 1;
}

// VERIFY: 0a589b7c manual
// OFFSET: 0x00759C60
int32_t CVehicleCamera_C::ConvertSmoothFacingFromRawToWorld(float& facing, CGObject_C* relativeTo) {
    if (!relativeTo) {
        return 0;
    }

    if (relativeTo->m_obj->m_type & TYPE_UNIT) {
        CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ relativeTo->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);
        float smoothFacing = static_cast<CGUnit_C*>(relativeTo)->m_smoothTarget.m_value;
        ConvertSmoothFacingFromRawToWorld(smoothFacing, transport);
        facing = smoothFacing + facing;
    } else {
        facing = relativeTo->GetFacing() + facing;
    }

    return 1;
}

// VERIFY: 33f183c1 auto
// OFFSET: 0x00759D60
void CVehicleCamera_C::Destroy() {
    uint32_t memHandle = this->m_memHandle;
    uint32_t heapId = s_cameraPool->m_heapId;
    s_cameraCount--;
    ObjectFree(heapId, memHandle);
}

// VERIFY: 410759cd auto
// OFFSET: 0x0075AA70
CVehicleCamera_C::CVehicleCamera_C() {
}

// VERIFY: 0d3b9410 manual
// OFFSET: 0x007597D0
int32_t CVehicleCamera_C::IsHierarchyChasingFacing() const {
    const CVehicleCamera_C* camera = this;

    while (true) {
        uint32_t state = camera->m_state;
        int32_t transitioning = state == 1 || state == 2 || state == 3;
        const VehicleSeatRec* seat = camera->m_seatRec;

        if (seat) {
            if (!transitioning) {
                int32_t flags = seat->m_flagsB;

                if ((flags & 0x80) && (flags & 0x40000)) {
                    return 1;
                }
            } else {
                if (seat->m_field33512213052 > 0.000099999997f) {
                    return 1;
                }

                if (state == 3) {
                    int32_t flags = seat->m_flagsB;

                    if ((flags & 0x40) && (flags & 0x20000)) {
                        return 1;
                    }
                }
            }
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(camera->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit || !unit->m_vehicleCamera) {
            return 0;
        }

        camera = unit->m_vehicleCamera;
    }
}

// VERIFY: 611f0110 auto
// OFFSET: 0x005FAA10
int32_t CVehicleCamera_C::IsTransitioningFacing() const {
    if (!(this->m_flags & 0x10)) {
        return 0;
    }

    uint32_t state = this->m_state;

    return state == 1 || state == 2 || state == 3;
}

// VERIFY: b1bd4cc1 manual
// OFFSET: 0x00759880
int32_t CVehicleCamera_C::UpdateTransitionFlags() {
    uint32_t state = this->m_state;
    int32_t transitioning;
    int32_t smooth;

    if (state != 1 && state != 2 && state != 3) {
        transitioning = 0;
        smooth = 0;
    } else {
        transitioning = 1;
        smooth = this->m_seatRec && (this->m_seatRec->m_flagsB & 0x1);
    }

    if (smooth) {
        this->m_flags |= 0x2;
    } else {
        this->m_flags &= ~0x2;
    }

    int32_t result = 0;

    if (transitioning && this->m_seatRec && (this->m_seatRec->m_flagsB & 0x80001)) {
        result = 1;
    }

    if (result) {
        this->m_flags |= 0x10;
    } else {
        this->m_flags &= ~0x10;
    }

    return result;
}

// VERIFY: a31203e6 manual
// OFFSET: 0x007598F0
float CVehicleCamera_C::GetTransitionProgress(uint32_t time) const {
    uint32_t state = this->m_state;

    if (state == 3) {
        return 1.0f;
    }

    CVehiclePassenger_C* passenger = this->m_unit->m_vehiclePassenger;
    int32_t waiting;
    int32_t mask;

    if (state == 1 || state == 2) {
        waiting = passenger && passenger->m_state == 2;
        mask = 0x40;
    } else {
        waiting = passenger && passenger->m_state == 5;
        mask = 0x80;
    }

    if (waiting && (!this->m_seatRec || !(mask & this->m_seatRec->m_flagsB))) {
        return passenger->m_facingBlend;
    }

    uint32_t duration = this->m_transitionDuration;

    if (!duration) {
        return 1.0f;
    }

    float progress = static_cast<float>(static_cast<int32_t>(time - this->m_transitionStartTime)) / static_cast<float>(static_cast<int32_t>(duration));

    if (progress < 0.0f) {
        return 0.0f;
    }

    if (progress >= 1.0f) {
        return 1.0f;
    }

    return progress;
}

// VERIFY: 321fd49a manual
// OFFSET: 0x00759CE0
int32_t CVehicleCamera_C::ConvertSmoothFacingFromWorldToRaw(float& facing, CGObject_C* relativeTo) {
    if (!relativeTo) {
        return 0;
    }

    if (relativeTo->m_obj->m_type & TYPE_UNIT) {
        CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ relativeTo->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);
        float smoothFacing = static_cast<CGUnit_C*>(relativeTo)->m_smoothTarget.m_value;
        ConvertSmoothFacingFromRawToWorld(smoothFacing, transport);
        facing = facing - smoothFacing;
    } else {
        facing = facing - relativeTo->GetFacing();
    }

    return 1;
}

// VERIFY: 28de7b3d manual
// OFFSET: 0x0075A720
int32_t CVehicleCamera_C::AttachToHierarchy() {
    CGCamera* activeCamera = CGWorldFrame::GetActiveCamera();

    if (!activeCamera) {
        return 0;
    }

    CVehicleCamera_C* camera = reinterpret_cast<CVehicleCamera_C*>(activeCamera->m_vehicleCamera);

    if (!camera) {
        return 0;
    }

    while (camera->m_nextCameraGuid) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(camera->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return 0;
        }

        camera = unit->m_vehicleCamera;

        if (!camera) {
            return 0;
        }
    }

    CVehiclePassenger_C* passenger = camera->m_unit->m_vehiclePassenger;
    WOWGUID vehicleGuid = passenger ? passenger->m_transitionVehicleGuid : WOWGUID();

    if (vehicleGuid != this->m_unit->m_obj->m_guid || camera->WouldAddingNextCameraCreateLoop(this)) {
        return 0;
    }

    camera->m_nextCameraGuid = this->m_unit->m_obj->m_guid;
    return 1;
}

// VERIFY: d73ba8b8 auto
// OFFSET: 0x00759A60
void CVehicleCamera_C::DetachFromHierarchy() {
    CGCamera* activeCamera = CGWorldFrame::GetActiveCamera();

    if (!activeCamera) {
        return;
    }

    for (CVehicleCamera_C* camera = reinterpret_cast<CVehicleCamera_C*>(activeCamera->m_vehicleCamera); camera; ) {
        if (!camera->m_nextCameraGuid) {
            return;
        }

        if (camera->m_nextCameraGuid == this->m_unit->m_obj->m_guid) {
            camera->m_nextCameraGuid = WOWGUID();
            return;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(camera->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return;
        }

        camera = unit->m_vehicleCamera;
    }
}

// VERIFY: 0bd15d9b auto
// OFFSET: 0x0075B150
CVehicleCamera_C* CVehicleCameraHeap::New() {
    uint32_t memHandle;
    void* memory = nullptr;

    if (!ObjectAlloc(this->m_heapId, &memHandle, &memory, false)) {
        return nullptr;
    }

    CVehicleCamera_C* camera = memory ? new (memory) CVehicleCamera_C() : nullptr;
    camera->m_memHandle = memHandle;

    return camera;
}

// VERIFY: 731fed16 manual
// OFFSET: 0x00759200
C3Vector CVehicleCamera_C::ComputeEndWorldPosition(CGUnit_C* target) {
    CGUnit_C* unit = target;
    int32_t transitioning;
    uint32_t state = this->m_state;

    if (state == 1 || state == 2 || state == 3) {
        transitioning = 1;

        if (target == this->m_unit && this->m_seatRec && (this->m_seatRec->m_flagsB & 0x200) && this->m_unit->m_vehiclePassenger) {
            CGUnit_C* vehicle = this->m_unit->m_vehiclePassenger->GetVehicleUnit();

            if (vehicle) {
                unit = vehicle;
            }
        }
    } else {
        transitioning = 0;
    }

    C3Vector result(0.0f, 0.0f, 0.0f);
    float facing;

    if (unit && unit != this->m_unit && unit->m_vehicleCamera) {
        result = unit->m_vehicleCamera->m_curWorldPos;
        facing = unit->m_vehicleCamera->m_worldFacing;
    } else {
        if (!unit) {
            unit = this->m_unit;
        }

        C3Vector raw(0.0f, 0.0f, 0.0f);

        if (transitioning && this->m_seatRec && (this->m_seatRec->m_flagsB & 0x100)) {
            C3Vector model = unit->m_vehiclePassenger ? unit->m_vehiclePassenger->GetEndPosition() : unit->GetModelPosition();
            raw = model;
            raw = unit->m_movement->GetRawPosition(raw);
        } else {
            raw = unit->GetRawPosition();
        }

        float targetFacing = unit->m_smoothTarget.m_value;
        CGUnit_C* next = nullptr;

        if (this->m_nextCameraGuid && unit->GetTransportGUID() == this->m_nextCameraGuid.m_guid) {
            next = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));
        }

        if (next && next->m_vehicleCamera) {
            float nextFacing = next->m_vehicleCamera->m_worldFacing;
            C44Matrix matrix;
            matrix.RotateAroundZ(nextFacing);
            matrix.d0 = next->m_vehicleCamera->m_curWorldPos.x;
            matrix.d1 = next->m_vehicleCamera->m_curWorldPos.y;
            matrix.d2 = next->m_vehicleCamera->m_curWorldPos.z;
            result = raw;
            result *= matrix;
            facing = nextFacing + targetFacing;
        } else {
            result = unit->m_movement->GetPosition(raw);
            facing = unit->m_movement->GetFacing(targetFacing);
        }
    }

    if (!transitioning) {
        return result;
    }

    const VehicleSeatRec* seat = this->m_seatRec;

    if (!seat || (seat->m_field33512213048 == 0.0f && seat->m_field33512213049 == 0.0f && seat->m_field33512213050 == 0.0f)) {
        return result;
    }

    C44Matrix matrix;
    matrix.RotateAroundZ(facing);
    C3Vector offset(seat->m_field33512213049, -seat->m_field33512213048, seat->m_field33512213050);
    offset *= matrix;
    result.x = result.x + offset.x;
    result.y = result.y + offset.y;
    result.z = result.z + offset.z;
    return result;
}

// VERIFY: 311a9ab4 manual
// OFFSET: 0x00759580
void CVehicleCamera_C::ComputeSafeCurWorldPos() {
    this->m_flags |= 0x40;
    this->m_safeCurWorldPos = this->m_curWorldPos;
    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!target) {
        return;
    }

    CGUnit_C* root = target->m_vehiclePassenger ? target->m_vehiclePassenger->GetStoredRootVehicleUnit() : target->FindRootVehicle(nullptr);

    if (!root) {
        return;
    }

    C3Vector origin = root->GetPosition();
    origin.z = root->GetMaxCameraHeight() + origin.z;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    C3Vector position = this->m_curWorldPos;
    float dx = position.x - origin.x;
    float dy = position.y - origin.y;
    float dz = camera->m_height + position.z - origin.z;
    float length = sqrtf(dx * dx + dy * dy + dz * dz);

    if (length <= 0.0000099999997f) {
        return;
    }

    float inverse = 1.0f / length;
    float dirX = dx * inverse;
    float dirY = dy * inverse;
    float dirZ = inverse * dz;
    float distance = length + 1.0f;

    if (distance > 200.0f) {
        distance = 200.0f;
    }

    C3Vector end(origin.x + dirX * distance, dirY * distance + origin.y, dirZ * distance + origin.z);
    C3Vector hit(0.0f, 0.0f, 0.0f);
    float fraction = 1.0f;
    float safe;

    if (World::Intersect(origin, end, &hit, &fraction, CGCamera::GetCollisionQueryFlags(), nullptr)) {
        if (fraction >= this->m_collideFraction) {
            safe = this->m_collideFraction * distance;
        } else {
            safe = distance * fraction;
            this->m_collideFraction = fraction;
            this->m_collideVelocity = 0.0f;
        }

        safe = safe - 1.0f;

        if (safe <= 0.0000099999997f) {
            this->m_safeCurWorldPos = origin;
            this->m_safeCurWorldPos.z = this->m_safeCurWorldPos.z - camera->m_height;
            return;
        }
    } else {
        safe = this->m_collideFraction * distance - 1.0f;
    }

    this->m_safeCurWorldPos = C3Vector(dirX * safe + origin.x, dirY * safe + origin.y, safe * dirZ + origin.z);
    this->m_safeCurWorldPos.z = this->m_safeCurWorldPos.z - camera->m_height;
}

// VERIFY: 081700db manual
// OFFSET: 0x00759D80
void CVehicleCamera_C::UpdatePosition(CGUnit_C* target, uint32_t time, float progress) {
    this->m_flags &= ~0x40;
    C3Vector end = this->ComputeEndWorldPosition(target);
    float elapsed = static_cast<float>(time - this->m_lastUpdateTime) * 0.001f;
    float x = elapsed * 2.5f;
    float damping = 1.0f / (x + x * x * x * 0.235f + x * x * 0.47999999f + 1.0f);
    float offset = this->m_collideFraction - 1.0f;
    float velocity = elapsed * (offset * 2.5f + this->m_collideVelocity);
    this->m_collideFraction = (offset + velocity) * damping + 1.0f;
    this->m_collideVelocity = (this->m_collideVelocity - velocity * 2.5f) * damping;
    CGUnit_C* unit = target ? target : this->m_unit;
    CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ unit->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);
    CVehicleCamera_C* next = nullptr;

    if (transport && unit->GetTransportGUID() == this->m_nextCameraGuid.m_guid) {
        next = static_cast<CGUnit_C*>(transport)->m_vehicleCamera;
    }

    uint32_t state = this->m_state;

    if (this->m_seatRec && this->m_seatRec->m_field33512213051 > 0.000099999997f && (state == 1 || state == 2 || state == 3)) {
        if (next) {
            end -= next->m_curWorldPos;
        } else {
            CVehicleCamera_C::ConvertFromWorldToRawOrOffset(end, transport);
        }

        if (this->m_flags & 0x20) {
            this->m_endPosChaser.m_position = end;
            this->m_endPosChaser.m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
        } else {
            this->m_endPosChaser.ChaserImplementation(end, 20.0f / this->m_seatRec->m_field33512213051, elapsed);
            end = this->m_endPosChaser.m_position;
        }

        if (next) {
            end += next->m_curWorldPos;
        } else {
            CVehicleCamera_C::ConvertFromRawOrOffsetToWorld(end, transport);
        }
    }

    if (this->m_state == 3) {
        this->m_curWorldPos = end;
        return;
    }

    if (transport) {
        this->m_startWorldPos = this->m_startRawPos;

        if (next) {
            this->m_startWorldPos += next->m_curWorldPos;
        } else {
            CVehicleCamera_C::ConvertFromRawOrOffsetToWorld(this->m_startWorldPos, transport);
        }
    }

    state = this->m_state;

    if (state == 1 || state == 4) {
        this->m_curWorldPos = this->m_startWorldPos;
        return;
    }

    int32_t mask = state == 2 || state == 3 ? 0x20000 : 0x40000;

    if (this->m_seatRec && (mask & this->m_seatRec->m_flagsB)) {
        C3Vector raw = end;
        CVehicleCamera_C::ConvertFromWorldToRawOrOffset(raw, transport);

        if (progress >= 0.99999899f) {
            this->m_rawPosChaser = ChaserState(raw, C3Vector(0.0f, 0.0f, 0.0f));
            this->m_curWorldPos = end;
        } else {
            float stiffness = 1.0f / ((1.0f - progress) * (1.0f - progress)) + 1.0f / (this->m_transitionDuration * 0.001f + 0.001f) * 4.0f;
            this->m_rawPosChaser.ChaserImplementation(raw, stiffness, elapsed);
            this->m_curWorldPos = this->m_rawPosChaser.m_position;
            CVehicleCamera_C::ConvertFromRawOrOffsetToWorld(this->m_curWorldPos, transport);
        }
    } else {
        this->m_curWorldPos.x = (end.x - this->m_startWorldPos.x) * progress + this->m_startWorldPos.x;
        this->m_curWorldPos.y = (end.y - this->m_startWorldPos.y) * progress + this->m_startWorldPos.y;
        this->m_curWorldPos.z = (end.z - this->m_startWorldPos.z) * progress + this->m_startWorldPos.z;
    }
}

// VERIFY: 27bb8869 manual
// OFFSET: 0x0075A1C0
void CVehicleCamera_C::UpdateFacing(CGUnit_C* target, uint32_t time, float progress, CGObject_C* relativeTo) {
    float targetFacing;
    WOWGUID relative;

    if (target && target != this->m_unit && target->m_vehicleCamera) {
        targetFacing = target->m_vehicleCamera->m_facing;
        relative.m_guid = target->m_vehicleCamera->GetRelativeTo();
    } else {
        CGUnit_C* unit = target ? target : this->m_unit;
        unit->UpdateSmoothFacing(nullptr);
        targetFacing = unit->m_smoothTarget.m_value;
        relative.m_guid = unit->GetTransportGUID();
    }

    float oldTarget = this->m_targetFacing;
    this->m_targetFacing = targetFacing;
    CGObject_C* object = ClntObjMgrObjectPtr(relative, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && relative == this->m_nextCameraGuid && static_cast<CGUnit_C*>(object)->m_vehicleCamera) {
        this->m_targetFacing = static_cast<CGUnit_C*>(object)->m_vehicleCamera->m_worldFacing + this->m_targetFacing;
    } else {
        CVehicleCamera_C::ConvertSmoothFacingFromRawToWorld(this->m_targetFacing, object);
    }

    if (oldTarget > this->m_targetFacing + 3.1415927f) {
        do {
            this->m_targetFacing = this->m_targetFacing + 6.2831855f;
        } while (this->m_targetFacing + 3.1415927f < oldTarget);
    }

    while (this->m_targetFacing - 3.1415927f > oldTarget) {
        this->m_targetFacing = this->m_targetFacing - 6.2831855f;
    }

    float elapsed = static_cast<float>(time - this->m_lastUpdateTime) * 0.001f;
    uint32_t state = this->m_state;

    if (this->m_seatRec && this->m_seatRec->m_field33512213052 > 0.000099999997f && (state == 1 || state == 2 || state == 3)) {
        if (this->m_flags & 0x20) {
            this->m_endFacingChaser.m_value = this->m_targetFacing;
            this->m_endFacingChaser.m_velocity = 0.0f;
        } else {
            this->m_endFacingChaser.Chase(&this->m_targetFacing, 20.0f / this->m_seatRec->m_field33512213052, elapsed);
            this->m_targetFacing = this->m_endFacingChaser.m_value;
        }
    }

    state = this->m_state;
    float oldWorld = this->m_worldFacing;
    float facing;

    if (state == 3) {
        facing = this->m_targetFacing;
    } else if (state == 1 || state == 4) {
        facing = this->m_startFacing;
    } else {
        int32_t mask = state != 2 ? 0x40000 : 0x20000;

        if (this->m_seatRec && (mask & this->m_seatRec->m_flagsB)) {
            if (progress >= 0.99999899f) {
                this->m_rawFacingChaser.m_value = this->m_targetFacing;
                this->m_rawFacingChaser.m_velocity = 0.0f;
                facing = this->m_rawFacingChaser.m_value;
            } else {
                float chase = this->m_targetFacing;

                while (chase + 3.1415927f < this->m_rawFacingChaser.m_value) {
                    chase = chase + 6.2831855f;
                }

                while (chase - 3.1415927f > this->m_rawFacingChaser.m_value) {
                    chase = chase - 6.2831855f;
                }

                float rate = 1.0f / (0.001f + this->m_transitionDuration * 0.001f) * 4.0f + 1.0f / ((1.0f - progress) * (1.0f - progress));
                this->m_rawFacingChaser.Chase(&chase, rate, elapsed);
                facing = this->m_rawFacingChaser.m_value;
            }
        } else {
            facing = CameraEaseInOut(progress) * (this->m_targetFacing - this->m_startFacing) + this->m_startFacing;
        }
    }

    this->m_worldFacing = facing;
    float oldRaw = this->m_facing;
    this->m_facing = this->m_worldFacing;
    CVehicleCamera_C::ConvertSmoothFacingFromWorldToRaw(this->m_facing, relativeTo);
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    CGInputControl* input = CGInputControl::GetActive();
    state = this->m_state;

    if (!(this->m_flags & 0x2) || !(state == 1 || state == 2 || state == 3) || !camera || camera->m_targetGUID != this->m_unit->m_obj->m_guid) {
        this->m_flags &= ~0x4;
        return;
    }

    if ((this->m_flags & 0x4) && camera->m_ignoreFacingRefs <= 0) {
        if (this->m_state == 3) {
            camera->UpdateYaw(oldWorld - this->m_worldFacing);
        } else {
            this->m_facing = oldRaw;
            this->m_worldFacing = oldWorld;
            this->m_rawFacingChaser.m_value = oldWorld;
            this->m_targetFacing = oldTarget;
        }
    }

    int32_t sync = 1;

    if (input && (input->m_flags & INPUT_TURN_PLAYER_MASK) && !(input->m_flags & INPUT_PLAYER_LOOK_MASK)) {
        sync = 0;
    }

    if (sync) {
        this->m_flags |= 0x4;
    } else {
        this->m_flags &= ~0x4;
    }
}

// VERIFY: 51d1357b auto
// OFFSET: 0x0075A630
void CVehicleCamera_C::UpdateWorldCameraState(uint32_t time, uint32_t duration) {
    if ((this->m_flags & 0x8) || (duration && !(this->m_flags & 0x1))) {
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera || (camera->m_targetGUID != this->m_unit->m_obj->m_guid && camera->m_targetGUID != this->m_targetGuid)) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    WOWGUID vehicleGuid = player->m_vehiclePassenger ? player->m_vehiclePassenger->m_transitionVehicleGuid : WOWGUID();
    CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!vehicle || player == this->m_unit || vehicle->FindRootVehicle(this->m_unit) != this->m_unit) {
        this->m_unit->VehiclePassengerUpdateWorldCameraState(this->m_vehicleRec, this->m_seatRec, time, time + duration);
        this->m_flags |= 0x8;
    }
}

// VERIFY: cf5f201f auto
// OFFSET: 0x0075A7D0
void CVehicleCamera_C::SetTarget(WOWGUID target, WOWGUID next) {
    this->m_flags |= 0x80;
    this->m_targetGuid = target;
    WOWGUID self = this->m_unit->m_obj->m_guid;
    WOWGUID chosen = target;

    if (target == self) {
        chosen = next;
    }

    if (this->m_nextCameraGuid == chosen) {
        this->m_flags &= ~0x80;
        return;
    }

    int32_t created = 0;

    if (chosen && chosen != self) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(chosen, TYPE_UNIT, __FILE__, __LINE__));
        int32_t valid = 0;

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->m_state) {
            CVehicleCamera_C* camera = unit->m_vehicleCamera;

            if (!camera) {
                camera = unit->CreateVehicleCamera();
                created = 1;
            }

            if (!this->WouldAddingNextCameraCreateLoop(camera)) {
                valid = 1;
            } else if (created) {
                unit->DestroyVehicleCamera();
            }
        }

        if (!valid) {
            chosen = WOWGUID();
        }
    }

    WOWGUID old = this->m_nextCameraGuid;
    this->m_nextCameraGuid = chosen;

    if (!created) {
        CGCamera* camera = CGWorldFrame::GetActiveCamera();

        if (camera) {
            camera->PickVehicleCamera();
        }
    }

    if (old) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(old, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->OnVehicleCameraPossiblyUnneeded();
        }
    }

    this->m_flags &= ~0x80;
}

// VERIFY: 853b5bce manual
// OFFSET: 0x0075A930
void CVehicleCamera_C::MaybeChangeTarget(CGUnit_C* vehicle, VehicleSeatRec* seat, int32_t exiting, WOWGUID& relativeTo) {
    if (!exiting && vehicle && seat && (seat->m_flags & 0x800)) {
        WOWGUID target = vehicle->m_obj->m_guid;
        relativeTo.m_guid = vehicle->GetTransportGUID();
        this->m_relativeToGuid = WOWGUID();
        this->SetTarget(target, relativeTo);
        return;
    }

    WOWGUID self = this->m_unit->m_obj->m_guid;
    CVehiclePassenger_C* passenger = this->m_unit->m_vehiclePassenger;

    if (passenger) {
        relativeTo = exiting ? passenger->m_startRelativeGuid : passenger->m_transitionVehicleGuid;
    } else {
        relativeTo.m_guid = this->m_unit->GetTransportGUID();
    }

    if (!vehicle) {
        this->m_relativeToGuid.m_guid = this->m_unit->GetTransportGUID();
    } else if (exiting || (seat && (seat->m_flagsB & 0x80001))) {
        CGUnit_C* root = vehicle->m_vehiclePassenger ? vehicle->m_vehiclePassenger->GetStoredRootVehicleUnit() : vehicle->FindRootVehicle(nullptr);
        CGUnit_C* unit = root ? root : vehicle;
        this->m_relativeToGuid.m_guid = unit->GetTransportGUID();
    } else {
        this->m_relativeToGuid = vehicle->m_obj->m_guid;
    }

    this->SetTarget(self, relativeTo);
}

// VERIFY: 1615a1e6 manual
// OFFSET: 0x0075AAC0
void CVehicleCamera_C::BeginTransition(uint32_t time) {
    CVehiclePassenger_C* passenger = this->m_unit->m_vehiclePassenger;

    if (!passenger) {
        return;
    }

    this->m_flags &= ~0x8;
    this->m_vehicleRec = passenger->m_vehicleRec;
    this->m_seatRec = passenger->m_seatRec;
    uint32_t passengerState = passenger->m_state;
    float delay = 0.0f;
    int32_t exiting;

    switch (passengerState) {
        case 0:
        case 5:
            exiting = 1;
            break;

        case 1:
            exiting = 0;

            if (this->m_seatRec) {
                delay = this->m_seatRec->m_field33512213004;
            }

            break;

        case 2:
        case 3:
            exiting = 0;
            break;

        case 4:
            exiting = 1;

            if (this->m_seatRec) {
                delay = this->m_seatRec->m_field33512213017;
            }

            break;

        default:
            return;
    }

    VehicleSeatRec* seat = this->m_seatRec;

    if (!this->m_unit->VehiclePassengerIsTransitionAllowed(seat, exiting)) {
        this->m_motionDuration = 0;
        this->m_flags |= 0x1;
    } else if (!seat) {
        this->m_motionDuration = static_cast<uint32_t>(500.0f);
        this->m_flags &= ~0x1;
    } else if (!exiting && (seat->m_flagsB & 0x40)) {
        delay = seat->m_field33512213044;
        this->m_motionDuration = CMath::fint_(seat->m_field33512213045 * 1000.0f);
        this->m_flags |= 0x1;
    } else if (exiting && (seat->m_flagsB & 0x80)) {
        delay = seat->m_field33512213046;
        this->m_motionDuration = CMath::fint_(seat->m_field33512213047 * 1000.0f);
        this->m_flags |= 0x1;
    } else {
        float duration = exiting ? seat->m_exitMaxDuration : seat->m_enterMaxDuration;
        this->m_motionDuration = CMath::fint_(fclamp(duration, 0.5f, 3.0f) * 1000.0f);
        this->m_flags &= ~0x1;
    }

    if (exiting) {
        if (delay > 0.0f) {
            this->m_state = 4;
            this->m_transitionDuration = CMath::fint_(delay * 1000.0f);
        } else if (!passengerState && (!this->m_motionDuration || !(this->m_flags & 0x1))) {
            this->m_state = 0;
            this->m_transitionDuration = 0;
        } else {
            this->m_state = 5;
            this->m_transitionDuration = this->m_motionDuration;
        }
    } else if (delay > 0.0f) {
        this->m_state = 1;
        this->m_transitionDuration = CMath::fint_(delay * 1000.0f);
    } else if (passengerState != 3 || (this->m_motionDuration && (this->m_flags & 0x1))) {
        this->m_state = 2;
        this->m_transitionDuration = this->m_motionDuration;
    } else {
        this->m_state = 3;
        this->m_transitionDuration = 0;
    }

    WOWGUID relativeTo;
    this->MaybeChangeTarget(passenger->GetVehicleUnit(), this->m_seatRec, exiting, relativeTo);

    if (delay < 0.0000099999997f) {
        if (exiting ? (!passengerState && !(this->m_flags & 0x1)) : (passengerState == 3 && !(this->m_flags & 0x1))) {
            this->UpdateWorldCameraState(time, 0);
        } else {
            this->UpdateWorldCameraState(time, this->m_motionDuration);
        }
    }

    this->m_startFacing = this->m_worldFacing;
    this->m_targetFacing = this->m_worldFacing;
    this->m_transitionStartTime = time;
    this->m_endFacingChaser.m_value = this->m_worldFacing;
    this->m_startWorldPos = this->m_curWorldPos;
    this->m_startRawPos = this->m_curWorldPos;
    CGObject_C* object = ClntObjMgrObjectPtr(relativeTo, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && relativeTo == this->m_nextCameraGuid && static_cast<CGUnit_C*>(object)->m_vehicleCamera) {
        this->m_startRawPos -= static_cast<CGUnit_C*>(object)->m_vehicleCamera->m_curWorldPos;
    } else {
        CVehicleCamera_C::ConvertFromWorldToRawOrOffset(this->m_startRawPos, object);
    }

    this->m_facing = this->m_worldFacing;
    CVehicleCamera_C::ConvertSmoothFacingFromWorldToRaw(this->m_facing, ClntObjMgrObjectPtr(WOWGUID{ this->GetRelativeTo() }, TYPE_OBJECT, __FILE__, __LINE__));
    this->m_rawPosChaser.m_position = this->m_startRawPos;
    this->m_rawPosChaser.m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_rawFacingChaser.m_value = this->m_worldFacing;
    this->m_rawFacingChaser.m_velocity = 0.0f;
    this->UpdateTransitionFlags();
    uint32_t state = this->m_state;

    if ((this->m_flags & 0x2) && (state == 1 || state == 2 || state == 3)) {
        this->m_flags |= 0x4;
    }

    this->m_flags |= 0x20;
}

// VERIFY: 8c33e512 auto
// OFFSET: 0x0075AF00
void CVehicleCamera_C::OnPassengerTransitionMotionBegin(uint32_t time, uint32_t duration) {
    if (this->m_flags & 0x1) {
        return;
    }

    this->m_flags |= 0x1;
    this->m_motionDuration = duration;

    if (this->m_state == 2 || this->m_state == 5) {
        this->m_transitionDuration = duration;
        this->UpdateWorldCameraState(time, duration);
    }
}

// VERIFY: d24f2186 auto
// OFFSET: 0x0075AF40
void CVehicleCamera_C::Update(uint32_t time) {
    if (!this->m_state || time == this->m_lastUpdateTime) {
        return;
    }

    if ((this->m_flags & 0x10) && this->m_unit->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGCamera* camera = CGWorldFrame::GetActiveCamera();
        CGInputControl* input = CGInputControl::GetActive();

        if (camera && input && (input->m_flags & INPUT_PLAYER_LOOK_MASK)) {
            camera->UpdateFreeLookFacing(0.0f, 0.0f, nullptr);

            if (input->CanSyncFreeLookFacing(this->m_unit)) {
                camera->SyncFreeLookFacing();
            }
        }
    }

    CGUnit_C* next = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_nextCameraGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (next && next->m_vehicleCamera) {
        next->m_vehicleCamera->Update(time);
    }

    uint32_t state = this->m_state;

    if (state && state != 3) {
        int32_t elapsed = time - this->m_transitionDuration - this->m_transitionStartTime;

        if (elapsed >= 0) {
            this->m_transitionStartTime = time;
            uint32_t duration = (this->m_flags & 0x1) ? this->m_motionDuration : 1000;

            switch (state) {
                case 1:
                case 4:
                    this->m_state = 2;
                    this->m_transitionDuration = duration - elapsed;
                    this->UpdateWorldCameraState(time, this->m_transitionDuration);
                    break;

                case 2:
                    this->m_state = 3;
                    this->m_transitionDuration = 0;
                    this->UpdateWorldCameraState(time, 0);
                    break;

                case 5:
                    this->m_state = 0;
                    this->UpdateWorldCameraState(time, 0);
                    this->SetTarget(WOWGUID(), WOWGUID());
                    this->DetachFromHierarchy();
                    this->m_unit->DestroyVehicleCamera();
                    return;
            }
        }
    }

    float progress = this->GetTransitionProgress(time);
    CGObject_C* relativeTo = ClntObjMgrObjectPtr(WOWGUID{ this->GetRelativeTo() }, TYPE_OBJECT, __FILE__, __LINE__);
    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (target) {
        this->UpdatePosition(target, time, progress);
        this->UpdateFacing(target, time, progress, relativeTo);
    }

    this->m_flags &= ~0x20;
    this->m_lastUpdateTime = time;
}

// VERIFY: 1f37c671 manual
// OFFSET: 0x0075B1A0
CVehicleCamera_C* CVehicleCamera_C::Create(CGUnit_C* unit, uint32_t time) {
    if (!unit->m_vehiclePassenger || !unit->m_vehiclePassenger->m_state) {
        return nullptr;
    }

    CVehicleCamera_C* camera = CVehicleCamera_C::s_cameraPool->New();
    CVehicleCamera_C::s_cameraCount++;
    camera->m_lastUpdateTime = time - 1;
    camera->m_transitionStartTime = time;
    camera->m_unit = unit;
    camera->m_flags = 0;
    camera->m_state = 0;
    camera->m_vehicleRec = nullptr;
    camera->m_seatRec = nullptr;
    camera->m_transitionDuration = 0;
    camera->m_motionDuration = 0;
    camera->m_targetGuid = WOWGUID();
    camera->m_nextCameraGuid = WOWGUID();
    WOWGUID self = unit->m_obj->m_guid;
    camera->SetTarget(self, WOWGUID{ unit->GetTransportGUID() });
    camera->m_relativeToGuid.m_guid = unit->GetTransportGUID();
    C44Matrix matrix;
    unit->BuildSmoothMatrix(matrix);
    camera->m_startWorldPos = C3Vector(matrix.d0, matrix.d1, matrix.d2);
    camera->m_curWorldPos = C3Vector(matrix.d0, matrix.d1, matrix.d2);
    camera->m_startRawPos = unit->GetRawPosition();
    camera->m_rawPosChaser.m_position = camera->m_startRawPos;
    camera->m_rawPosChaser.m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
    camera->m_endPosChaser.m_position = C3Vector(0.0f, 0.0f, 0.0f);
    camera->m_endPosChaser.m_velocity = C3Vector(0.0f, 0.0f, 0.0f);
    unit->UpdateSmoothFacing(nullptr);
    float facing = unit->GetSmoothFacing();
    camera->m_startFacing = facing;
    camera->m_worldFacing = facing;
    camera->m_facing = unit->m_smoothTarget.m_value;
    camera->m_targetFacing = facing;
    camera->m_collideFraction = 1.0f;
    camera->m_collideVelocity = 0.0f;
    camera->m_rawFacingChaser.m_value = camera->m_startFacing;
    camera->m_rawFacingChaser.m_velocity = 0.0f;
    camera->m_endFacingChaser.m_value = camera->m_startFacing;
    camera->m_endFacingChaser.m_velocity = 0.0f;
    camera->BeginTransition(time);

    if (camera->m_state) {
        return camera;
    }

    CVehicleCamera_C::s_cameraCount--;
    ObjectFree(CVehicleCamera_C::s_cameraPool->m_heapId, camera->m_memHandle);
    return nullptr;
}
