#include "PlayerSound_C.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/MaterialRec.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"

// VERIFY: 91d27b03 manual
// OFFSET: 0x007634F0
uint32_t CGPlayer_C::GetImpactType() const {
    const CGBag_C* bag = this->GetBag();
    WOWGUID guid = static_cast<uint32_t>(bag->m_numSlots) > 4 ? bag->m_slots[4] : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));
    const CGUnitVirtualItem* chest = item ? &item->m_virtualItem : nullptr;

    if (!chest || chest->m_classID != 4) {
        return 0;
    }

    MaterialRec* rec = g_materialDB.GetRecord(chest->m_material);

    if (!rec) {
        return 0;
    }

    if (rec->m_flags & 0x2) {
        return 2;
    }

    return (static_cast<int8_t>(rec->m_flags) & 0x4) >> 2;
}
