#include "SpellCast.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Magic/MagicClient/PendingSpellCastData.h"
#include "WoW/Source/Object/MovementShared.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/WGUID.h"

// VERIFY: 2ee73875 auto
// OFFSET: 0x009AB770
PendingSpellCastData::PendingSpellCastData() {
    this->m_castItem = WOWGUID{ 0 };
    this->unused_0010 = 0;
    this->unused_0014 = 0;
    this->m_spellId = 0;
    this->m_targetFlags = 0;
    this->m_objectTarget = WOWGUID{ 0 };
    this->m_itemTarget = WOWGUID{ 0 };
    this->unk_0038 = WOWGUID{ 0 };
    this->m_sourceTarget = WOWGUID{ 0 };
    this->m_destTarget = WOWGUID{ 0 };
    this->m_sourcePos = C3Vector();
    this->m_destPos = C3Vector();
    this->unused_006C = -1;
    this->unused_0068 = 0.0f;
    this->unused_0070 = 0;
    this->m_elevation = 0.0f;
    this->unused_0074 = 0;
    this->m_missileSpeed = 0.0f;
    this->unused_0078 = 0;
    this->unused_007C = 0;
    this->unused_0080 = 0;
    this->unused_0084 = 0;
    this->unused_0088 = 0;
    this->unused_008C = 0;
    this->unused_0110 = 0;
    this->m_castFlags = 0;
    this->unused_0118 = 0;
    this->m_glyphSlot = 0;
}

// VERIFY: 33b96970 manual
// OFFSET: 0x009AB810
C3Vector PendingSpellCastData::GetDestLocation() const {
    if (!this->m_destTarget.m_guid) {
        return this->m_destPos;
    }

    C44Matrix matrix;
    MovementGetTransportMtxX(this->m_destTarget, &matrix, WOWGUID{ 0 }, __FILE__, __LINE__);
    return this->m_destPos * matrix;
}

// VERIFY: 42f238a6 auto
// OFFSET: 0x009AB8B0
void PendingSpellCastData::WriteTargetInfo(const PendingSpellCastData* data, CDataStore* msg) {
    msg->Put(data->m_targetFlags);

    if (data->m_targetFlags & 0x18A02) {
        SmartGUID guid;
        guid.m_guid = data->m_objectTarget.m_guid;
        *msg << guid;
    }

    if (data->m_targetFlags & 0x1010) {
        SmartGUID guid;
        guid.m_guid = data->m_itemTarget.m_guid;
        *msg << guid;
    }

    if (data->m_targetFlags & 0x20) {
        SmartGUID guid;
        guid.m_guid = data->m_sourceTarget.m_guid;
        *msg << guid;
        msg->Put(data->m_sourcePos.x);
        msg->Put(data->m_sourcePos.y);
        msg->Put(data->m_sourcePos.z);
    }

    if (data->m_targetFlags & 0x40) {
        SmartGUID guid;
        guid.m_guid = data->m_destTarget.m_guid;
        *msg << guid;
        msg->Put(data->m_destPos.x);
        msg->Put(data->m_destPos.y);
        msg->Put(data->m_destPos.z);
    }

    if (data->m_targetFlags & 0x2000) {
        msg->PutArray(reinterpret_cast<const uint8_t*>(data->m_targetString), sizeof(data->m_targetString));
    }
}

// VERIFY: 8b44a52e manual
// OFFSET: 0x009AB9C0
void SpellGetCastTargets(PendingSpellCastData* data, CDataStore* msg) {
    msg->Get(data->m_targetFlags);
    SmartGUID guid;

    if (data->m_targetFlags & 0x18A02) {
        *msg >> guid;
        data->m_objectTarget = guid;
    }

    if (data->m_targetFlags & 0x1010) {
        *msg >> guid;
        data->m_itemTarget = guid;
    }

    if (data->m_targetFlags & 0x20) {
        *msg >> guid;
        data->m_sourceTarget = guid;
        msg->Get(data->m_sourcePos.x);
        msg->Get(data->m_sourcePos.y);
        msg->Get(data->m_sourcePos.z);
    }

    if (data->m_targetFlags & 0x40) {
        *msg >> guid;
        data->m_destTarget = guid;
        msg->Get(data->m_destPos.x);
        msg->Get(data->m_destPos.y);
        msg->Get(data->m_destPos.z);
    }

    if (data->m_targetFlags & 0x2000) {
        msg->GetArray(reinterpret_cast<uint8_t*>(data->m_targetString), sizeof(data->m_targetString));
    }
}
