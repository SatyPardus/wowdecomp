#include "MerchantFrame.h"
#include <cstddef>
#include <cstdlib>
#include "WoW/Source/Object/Player.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include <cstring>
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Ui/Tutorial.h"

// STATIC: 0x00BF90F8
uint32_t CGMerchantInfo::m_buybackSlots[12];

// STATIC: 0x00BFA3F8
uint32_t CGMerchantInfo::m_pendingItemStats;

// STATIC: 0x00BFA3E8
WOWGUID CGMerchantInfo::m_merchant;

// STATIC: 0x00BFA3F0
int32_t CGMerchantInfo::m_numMerchantItems;

// STATIC: 0x00BFA3F4
uint32_t CGMerchantInfo::m_numBuybackItems;

// STATIC: 0x00BF9128
MerchantItem CGMerchantInfo::m_items[150];

// VERIFY: 4b63a9a7 manual
// OFFSET: 0x00584150
MerchantItem* CGMerchantInfo::FindItemById(int32_t itemId) {
    if (!CGMerchantInfo::m_merchant || !CGMerchantInfo::m_numMerchantItems) {
        return nullptr;
    }

    for (uint32_t i = 0; i < sizeof(CGMerchantInfo::m_items) / sizeof(CGMerchantInfo::m_items[0]); i++) {
        if (CGMerchantInfo::m_items[i].m_itemID == itemId) {
            return &CGMerchantInfo::m_items[i];
        }
    }

    return nullptr;
}

// VERIFY: 7160c52f manual
// OFFSET: 0x00584B20
int32_t CGMerchantInfo::GetRepairCost(CGItem_C* item) {
    float multiplier = 1.0f;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGUnit_C* merchant = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGMerchantInfo::m_merchant, TYPE_UNIT, __FILE__, __LINE__));

    if (player && merchant) {
        multiplier = 1.0f - merchant->GetPlayerDiscount(player);
    }

    return CMath::fint_(static_cast<float>(static_cast<uint32_t>(item->GetRepairCost())) * multiplier);
}

// VERIFY: 476fd930 auto
// OFFSET: 0x00584600
void CGMerchantInfo::CloseMerchant() {
    if (!CGMerchantInfo::m_merchant) {
        return;
    }

    CGGameUI::ClearInteractTarget(CGMerchantInfo::m_merchant, 0);
    CGMerchantInfo::m_merchant = WOWGUID();
    CGMerchantInfo::m_numMerchantItems = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_CLOSED, nullptr);

    if (CGGameUI::m_cursorItemType == UICURSOR_MERCHANT && CGGameUI::GetCursorVirtualItem()) {
        CGGameUI::ClearCursor(1, 1);
    }

    if (CursorGetResetMode() == REPAIR_CURSOR) {
        CursorSetResetMode(POINT_CURSOR);
        CursorSetMode(POINT_CURSOR);
    }
}

// VERIFY: ecc91497 manual
// OFFSET: 0x005844C0
static void MerchantItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGMerchantInfo::m_pendingItemStats && --CGMerchantInfo::m_pendingItemStats) {
        return;
    }

    if (CGMerchantInfo::m_merchant) {
        FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_UPDATE, nullptr);
    }
}

// VERIFY: 459936a3 auto
// OFFSET: 0x00584500
void CGMerchantInfo::SetMerchant(WOWGUID merchant, const MerchantItem* items, int32_t count) {
    if (CGMerchantInfo::m_numMerchantItems) {
        for (uint32_t i = 0; i < 150; i++) {
            if (CGMerchantInfo::m_items[i].m_itemID) {
                g_itemDBCache.CancelCallback(CGMerchantInfo::m_items[i].m_itemID, MerchantItemStatsCallback, nullptr);
            }
        }
    }

    CGTutorial::TriggerTutorial(0x13);
    CGGameUI::SetInteractTarget(merchant);
    CGMerchantInfo::m_merchant = merchant;
    memset(CGMerchantInfo::m_items, 0, sizeof(CGMerchantInfo::m_items));
    memcpy(CGMerchantInfo::m_items, items, count * sizeof(MerchantItem));
    CGMerchantInfo::m_numMerchantItems = count;
    CGMerchantInfo::m_pendingItemStats = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_SHOW, nullptr);
}

// VERIFY: 3f87e2cf manual
// OFFSET: 0x00584080
MerchantItem* CGMerchantInfo::GetItem(int32_t index) {
    if (index >= 0 && index < CGMerchantInfo::m_numMerchantItems && CGMerchantInfo::m_merchant) {
        return &CGMerchantInfo::m_items[index];
    }

    return nullptr;
}

// VERIFY: f9ba7356 auto
// OFFSET: 0x00584680
ItemStats_C* CGMerchantInfo::GetItemStats(int32_t itemId) {
    if (!itemId) {
        return nullptr;
    }

    WOWGUID merchant = CGMerchantInfo::m_merchant;
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, merchant, MerchantItemStatsCallback, nullptr, false);

    if (!stats) {
        CGMerchantInfo::m_pendingItemStats++;
    }

    return stats;
}

// VERIFY: e792c48f manual
// OFFSET: 0x005841A0
static int32_t QSortBuybackTimestamps(const void* a, const void* b) {
    uint32_t timeA = static_cast<const BuybackEntry*>(a)->m_timestamp;
    uint32_t timeB = static_cast<const BuybackEntry*>(b)->m_timestamp;

    if (timeB > timeA) {
        return -1;
    }

    return timeB < timeA;
}

// VERIFY: e50eb21c manual
// OFFSET: 0x00584BB0
void CGMerchantInfo::SortBuybackItems() {
    memset(CGMerchantInfo::m_buybackSlots, 0, sizeof(CGMerchantInfo::m_buybackSlots));
    CGMerchantInfo::m_numBuybackItems = 0;

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    BuybackEntry entries[12];

    for (uint32_t slot = 74; slot - 74 < 12; slot++) {
        if (slot < static_cast<uint32_t>(player->m_bag.m_numSlots) && player->m_bag.m_slots[slot] && slot - 74 < 12 && player->m_player->m_playerFieldBuybackPrice1[slot - 74]) {
            entries[CGMerchantInfo::m_numBuybackItems].m_timestamp = player->m_player->m_playerFieldBuybackTimestamp1[slot - 74];
            entries[CGMerchantInfo::m_numBuybackItems].m_slot = slot;
            CGMerchantInfo::m_numBuybackItems++;
        }
    }

    qsort(entries, CGMerchantInfo::m_numBuybackItems, sizeof(BuybackEntry), QSortBuybackTimestamps);

    for (uint32_t i = 0; i < CGMerchantInfo::m_numBuybackItems; i++) {
        CGMerchantInfo::m_buybackSlots[i] = entries[i].m_slot;
    }
}

// VERIFY: 9d924543 manual
// OFFSET: 0x00584D90
bool CGMerchantInfo::IsBuybackItem(WOWGUID item) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return false;
    }

    for (uint32_t i = 0; i < 12; i++) {
        WOWGUID guid = static_cast<int32_t>(i + 74) < player->m_bag.m_numSlots ? player->m_bag.m_slots[i + 74] : WOWGUID();

        if (guid == item) {
            return true;
        }
    }

    return false;
}

// VERIFY: 620ce8ba auto
// OFFSET: 0x00585F60
static int32_t BuybackUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGMerchantInfo::SortBuybackItems();
    FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_UPDATE, nullptr);
    return 1;
}

// VERIFY: 6e746dba auto
// OFFSET: 0x00585F80
void CGMerchantInfo::EnterWorld() {
    CGMerchantInfo::m_merchant = WOWGUID{ 0 };
    CGMerchantInfo::m_numMerchantItems = 0;
    memset(CGMerchantInfo::m_items, 0, sizeof(CGMerchantInfo::m_items));
    ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offsetof(CGPlayerLocalData, m_playerFieldBuybackPrice1), sizeof(CGPlayerLocalData::m_playerFieldBuybackPrice1) + sizeof(CGPlayerLocalData::m_playerFieldBuybackTimestamp1), BuybackUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    CGMerchantInfo::SortBuybackItems();
}

// VERIFY: 20b7ca61 auto
// OFFSET: 0x005840C0
void CGMerchantInfo::RefreshList() {
    if (CGMerchantInfo::m_merchant) {
        FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_UPDATE, nullptr);
    }
}

// VERIFY: ceeabfca manual
// OFFSET: 0x005845B0
int32_t CGMerchantInfo::CanRepair() {
    CGUnit_C* merchant = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGMerchantInfo::m_merchant, TYPE_UNIT, __FILE__, __LINE__));
    return merchant && (merchant->m_unit->m_unitNpcFlags & 0x1000);
}

// VERIFY: f4c282b1 manual
// OFFSET: 0x005840E0
void CGMerchantInfo::UpdateItemQuantity(WOWGUID merchant, int32_t slot, int32_t quantity) {
    if (merchant != CGMerchantInfo::m_merchant) {
        return;
    }

    for (uint32_t i = 0; i < 150; i++) {
        if (CGMerchantInfo::m_items[i].m_slot == slot) {
            CGMerchantInfo::m_items[i].m_maxCount = quantity;
            break;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_UPDATE, nullptr);
}

// VERIFY: ff887f91 auto
// OFFSET: 0x00585FE0
void CGMerchantInfo::LeaveWorld() {
    CGMerchantInfo::CloseMerchant();
    ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offsetof(CGPlayerLocalData, m_playerFieldBuybackPrice1), BuybackUpdateHandler, nullptr);
}
