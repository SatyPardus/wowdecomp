#include "CGGameObject_C_Type_TrapDoor.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/WorldClient/MapEntity.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/WorldFrame.h"

// VERIFY: 6e6904ff manual
// OFFSET: 0x0070D980
void CGGameObject_C_Type_TrapDoor::PostInitWithStats() {
    this->CreateWorldObj();

    if (!this->m_worldObject) {
        return;
    }

    MovementAddTransport(this->m_owner);

    if (!this->m_worldObject) {
        return;
    }

    C44Matrix& matrix = this->m_owner->m_matrix;
    matrix = this->m_owner->GetRotation();
    matrix.d0 = this->m_position.x;
    matrix.d1 = this->m_position.y;
    matrix.d2 = this->m_position.z;
    World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_WorldObj::SequenceFinishedHandler, 0, 0);
    World::ObjectUpdate(this->m_worldObject, this->m_owner->GetMatrix());

    if (this->m_owner->m_gameObject->m_state != 1) {
        this->UpdateState(1, 0);
    }
}

// VERIFY: 58fe01f9 auto
// OFFSET: 0x00710460
void CGGameObject_C_Type_TrapDoor::UpdateState(int32_t oldState, int32_t newState) {
    if (!this->m_worldObject) {
        return;
    }

    if (newState) {
        World::ObjectSetSequence(this->m_worldObject, 146, 0, 0);
        this->m_owner->m_hasWorldBounds = 1;
        World::ObjectEnableCollision(this->m_worldObject, 1);
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    CPassenger* next;

    for (CPassenger* passenger = this->m_passengers.Head(); passenger; passenger = next) {
        next = this->m_passengers.Next(passenger);
        CMovement_C* movement = static_cast<CMovement_C*>(passenger);

        if (camera->m_targetGUID == *passenger->m_ownerGuid) {
            camera->MakeRelativeTo(WOWGUID());
        }

        if (*passenger->m_ownerGuid == CGUnit_C::s_activeMover) {
            movement->OnFallLocal(OsGetAsyncTimeMs());
            movement->ForceSetTransport(WOWGUID(), 255, 0);
        }
    }

    World::ObjectSetSequence(this->m_worldObject, 148, 0, 0);
    this->m_owner->m_hasWorldBounds = 0;
    World::ObjectEnableCollision(this->m_worldObject, 0);
}

// VERIFY: 84b1b349 auto
// OFFSET: 0x00713790
CGGameObject_C_Type_TrapDoor::CGGameObject_C_Type_TrapDoor(CGGameObject_C* owner) : CGGameObject_C_Type_TransportBase(owner) {
}

// VERIFY: 7c534dd2 auto
// OFFSET: 0x0070B580
void CGGameObject_C_Type_TrapDoor::PostReenable() {
    this->CreateWorldObj();

    if (this->m_worldObject) {
        MovementAddTransport(this->m_owner);
    }

    if (this->m_worldObject) {
        World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_WorldObj::SequenceFinishedHandler, 0, 0);
    }
}

// VERIFY: 70c8e6d1 auto
// OFFSET: 0x007137B0
void CGGameObject_C_Type_TrapDoor::UpdateMovement(uint32_t time, uint32_t elapsed) {
    if (!this->m_worldObject) {
        return;
    }

    if (!this->m_worldObjectReady && World::ObjectIsLoaded(this->m_worldObject)) {
        this->m_worldObjectReady = 1;
        int32_t moved;
        this->MovePassengers(&moved);
        return;
    }

    World::ObjectUpdate(this->m_worldObject, this->m_owner->GetMatrix());
}
