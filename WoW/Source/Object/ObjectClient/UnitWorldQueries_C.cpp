#include "Unit_C.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Services/SysMessage.h"

// VERIFY: 1313ac12 manual
// OFFSET: 0x0071ED80
CAaBox CGUnit_C::GetStandBox() const {
    CAaBox box;
    box.b.x = 0.0f;
    box.b.y = 0.0f;
    box.b.z = 0.0f;
    box.t.x = 0.0f;
    box.t.y = 0.0f;
    box.t.z = 0.0f;

    CreatureModelDataRec* modelData = this->m_modelData;

    if (modelData || (modelData = this->GetModelData())) {
        box = *reinterpret_cast<const CAaBox*>(&modelData->m_geoBoxMinX);
    }

    if (this->m_unit->m_unitFieldMountdisplayid > 0) {
        CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_mountDisplayId);

        if (displayInfo) {
            CreatureModelDataRec* mountModel = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

            if (mountModel) {
                float offset = mountModel->m_mountHeight - (box.t.z - box.b.z) * 0.5f;
                box.b.z = box.b.z + offset;
                box.t.z = offset + box.t.z;
                box |= *reinterpret_cast<const CAaBox*>(&mountModel->m_geoBoxMinX);
            }
        }
    }

    return box;
}

// VERIFY: 272cc5b1 manual
// OFFSET: 0x00717A20
CreatureModelDataRec* CGUnit_C::GetModelData() const {
    int32_t displayId = this->m_displayId;

    if (!displayId || this->m_unit->m_unitFieldNativedisplayid != this->m_unit->m_unitFieldDisplayid) {
        displayId = this->m_unit->m_unitFieldDisplayid;
    }

    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!displayInfo) {
        SysMsgPrintf(STATUS_WARNING, 2, "NOCREATUREDISPLAYIDFOUND|%d", displayId);
        return nullptr;
    }

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

    if (!modelData) {
        SysMsgPrintf(STATUS_WARNING, 16, "INVALIDDISPLAYMODELRECORD|%d|%d", displayInfo->m_modelID, displayInfo->m_id);
        return nullptr;
    }

    return modelData;
}

// VERIFY: 99f1723a manual
// OFFSET: 0x00717870
C3Spline_CatmullRom* CGUnit_C::GetSpline() {
    CMoveSpline* spline = this->m_movementData.m_spline;
    return spline ? &spline->m_spline : nullptr;
}

// VERIFY: 1e31eb25 auto
// OFFSET: 0x00717890
float CGUnit_C::GetSplineElapsedTime() {
    CMoveSpline* spline = this->m_movementData.m_spline;
    return spline ? static_cast<float>(static_cast<int32_t>(spline->m_start)) : 0.0f;
}

// VERIFY: 80a0af13 manual
// OFFSET: 0x007178B0
float CGUnit_C::GetSplineDuration() {
    CMoveSpline* spline = this->m_movementData.m_spline;
    return spline ? static_cast<float>(spline->m_duration) : 0.0f;
}
