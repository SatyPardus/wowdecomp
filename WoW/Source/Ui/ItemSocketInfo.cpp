#include "ItemSocketInfo.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/GemPropertiesRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"

// STATIC: 0x00C20FA8
uint8_t CGItemSocketInfo::m_gemContainerSlot[3];

// STATIC: 0x00C20FB0
WOWGUID CGItemSocketInfo::m_gemItemContainer[3];

// STATIC: 0x00C20FC8
WOWGUID CGItemSocketInfo::m_gemItems[3];

// STATIC: 0x00C20FE0
WOWGUID CGItemSocketInfo::m_socketItem;

// STATIC: 0x00C20FE8
ItemStats_C* CGItemSocketInfo::m_socketItemStats;

// VERIFY: ea6a2aaa manual
// OFFSET: 0x005C4680
void CGItemSocketInfo::SetItem(WOWGUID item) {
    CGGameUI::UnlockItem(CGItemSocketInfo::m_socketItem);

    for (uint32_t i = 0; i < 3; i++) {
        if (CGItemSocketInfo::m_gemItems[i]) {
            CGGameUI::UnlockItem(CGItemSocketInfo::m_gemItems[i]);
        }
    }

    CGItemSocketInfo::m_gemItems[0] = WOWGUID();
    CGItemSocketInfo::m_gemItems[1] = WOWGUID();
    CGItemSocketInfo::m_gemItems[2] = WOWGUID();
    CGItemSocketInfo::m_socketItemStats = nullptr;
    CGItemSocketInfo::m_socketItem = item;
    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (object) {
        WOWGUID none = WOWGUID();
        CGItemSocketInfo::m_socketItemStats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, none, nullptr, nullptr, false);
    }

    if (CGItemSocketInfo::m_socketItemStats && item) {
        CGGameUI::LockItem(item);
        FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_UPDATE, nullptr);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_CLOSE, nullptr);
    }
}

// VERIFY: 21c9b3eb manual
// OFFSET: 0x005C4780
int32_t CGItemSocketInfo::SetGem(uint32_t slot, WOWGUID item, WOWGUID container, uint8_t containerSlot) {
    if (slot >= 3 || !CGItemSocketInfo::m_socketItem || !CGItemSocketInfo::m_socketItemStats) {
        return 0;
    }

    if (!item) {
        CGItemSocketInfo::m_gemItems[slot] = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_UPDATE, nullptr);
        return 1;
    }

    CGItem_C* gem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (!gem) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(gem->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats || !stats->m_gemProperties) {
        return 0;
    }

    ItemStats_C* socketStats = CGItemSocketInfo::m_socketItemStats;
    GemPropertiesRec* properties = g_gemPropertiesDB.GetRecord(stats->m_gemProperties);
    uint32_t meta;

    if (!properties || (meta = socketStats->m_socketColor[slot] & 0x1) == (properties->m_type & 0x1)) {
        CGItemSocketInfo::m_gemItems[slot] = item;
        CGItemSocketInfo::m_gemItemContainer[slot] = container;
        CGItemSocketInfo::m_gemContainerSlot[slot] = containerSlot;
        FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_UPDATE, nullptr);
        return 1;
    }

    if (meta) {
        CGGameUI::DisplayError(ERR_SOCKETING_REQUIRES_META_GEM);
        return 0;
    }

    CGGameUI::DisplayError(ERR_SOCKETING_META_GEM_ONLY_IN_METASLOT);
    return 0;
}

// VERIFY: c7291b92 manual
// OFFSET: 0x005C48D0
int32_t CGItemSocketInfo::GemMatchesSocket(const ItemStats_C* item, const ItemStats_C* gem, int32_t socket) {
    if (!item || !gem) {
        return 0;
    }

    uint32_t color = item->m_socketColor[socket];

    if (!color) {
        return 1;
    }

    GemPropertiesRec* properties = g_gemPropertiesDB.GetRecord(gem->m_gemProperties);

    if (!properties) {
        return 0;
    }

    int32_t type = properties->m_type;
    int32_t match = 0;

    if ((type & 1) && (color & 1)) {
        match = 1;
    }

    if ((type & 2) && (color & 2)) {
        match = 1;
    }

    if ((type & 4) && (color & 4)) {
        match = 1;
    }

    if ((type & 8) && (color & 8)) {
        return 1;
    }

    return match;
}

// VERIFY: d9030c33 auto
// OFFSET: 0x005C4470
void CGItemSocketInfo::GetGemItemForSlot(uint32_t slot, WOWGUID& item, WOWGUID& container, uint8_t& containerSlot) {
    if (slot >= 3) {
        item = WOWGUID();
        container = WOWGUID();
        containerSlot = 0;
    } else {
        item = CGItemSocketInfo::m_gemItems[slot];
        container = CGItemSocketInfo::m_gemItemContainer[slot];
        containerSlot = CGItemSocketInfo::m_gemContainerSlot[slot];
    }
}

// VERIFY: fddce4cd auto
// OFFSET: 0x005C44D0
void EnchantItemCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_UPDATE, nullptr);
    }
}

// VERIFY: 147f3731 auto
// OFFSET: 0x005C44F0
void CGItemSocketInfo::EnterWorld() {
    CGItemSocketInfo::m_socketItem = WOWGUID{ 0 };
    CGItemSocketInfo::m_socketItemStats = nullptr;
    CGItemSocketInfo::m_gemItems[0] = WOWGUID{ 0 };
    CGItemSocketInfo::m_gemItems[1] = WOWGUID{ 0 };
    CGItemSocketInfo::m_gemItems[2] = WOWGUID{ 0 };
    CGItemSocketInfo::m_gemItemContainer[0] = WOWGUID{ 0 };
    CGItemSocketInfo::m_gemItemContainer[1] = WOWGUID{ 0 };
    CGItemSocketInfo::m_gemItemContainer[2] = WOWGUID{ 0 };
    memset(CGItemSocketInfo::m_gemContainerSlot, 0, sizeof(CGItemSocketInfo::m_gemContainerSlot));
}

// VERIFY: a44ae1bf manual
// OFFSET: 0x005C4970
bool CGItemSocketInfo::IsSocketItemBoundTradeable() {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGItemSocketInfo::m_socketItem, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return false;
    }

    return item->IsBound() && !item->IsBoundForTrade();
}

// VERIFY: 53eae933 manual
// OFFSET: 0x005C4FF0
void CGItemSocketInfo::Accept() {
    if (!CGItemSocketInfo::m_socketItem) {
        return;
    }

    int32_t count = 0;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SOCKET_GEMS));
    msg.Put(CGItemSocketInfo::m_socketItem.m_guid);

    for (uint32_t i = 0; i < 3; i++) {
        if (CGItemSocketInfo::m_gemItems[i]) {
            count++;
        }

        msg.Put(CGItemSocketInfo::m_gemItems[i].m_guid);
    }

    if (count) {
        msg.Finalize();
        ClientServices::Send2(&msg);
    }
}

// VERIFY: 5ccf6cd7 manual
// OFFSET: 0x005C50E0
bool CGItemSocketInfo::IsSocketItemRefundable() {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGItemSocketInfo::m_socketItem, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return false;
    }

    CGItem_C::ItemPurchaseData* purchase = item->m_itemPurchaseData;

    if (!purchase || item->HasEnchantments()) {
        return false;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    return static_cast<int32_t>(purchase->m_refundTimeLeft + 7200 - player->GetPlayedTime()) > 0;
}

// VERIFY: 47e2ebf9 auto
// OFFSET: 0x005C4EE0
void CGItemSocketInfo::LeaveWorld() {
    CGItemSocketInfo::SetItem(WOWGUID{ 0 });
}

// VERIFY: e9a41a83 auto
// OFFSET: 0x005C4EF0
void CGItemSocketInfo::UpdateItem(WOWGUID item) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->CountEquippedGems();
    }

    if (item == CGItemSocketInfo::m_socketItem) {
        FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_UPDATE, nullptr);
    }
}

// VERIFY: db24a767 manual
// OFFSET: 0x005C4F40
void CGItemSocketInfo::RemoveItem(WOWGUID item) {
    if (item == CGItemSocketInfo::m_socketItem) {
        CGItemSocketInfo::SetItem(WOWGUID{ 0 });
        return;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (item == CGItemSocketInfo::m_gemItems[i]) {
            if (CGItemSocketInfo::m_socketItem.m_guid && CGItemSocketInfo::m_socketItemStats) {
                CGItemSocketInfo::m_gemItems[i] = WOWGUID{ 0 };
                FrameScript_SignalEvent(SCRIPT_EVENT_SOCKET_INFO_UPDATE, nullptr);
            }

            break;
        }
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->CountEquippedGems();
    }
}
