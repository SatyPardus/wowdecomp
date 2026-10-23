#include "LootFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Base/CDataStore.h"
#include <cstring>

// STATIC: 0x00BFA688
uint32_t CGLootInfo::m_itemsPending;

// STATIC: 0x00BFA68C
LOOT_ACQUIRE CGLootInfo::m_lootAcquire;

// STATIC: 0x00BFA690
LootSlot CGLootInfo::m_slots[18];

// STATIC: 0x00BFA8D0
int32_t CGLootInfo::m_coins;

// STATIC: 0x00BFA8D8
WOWGUID CGLootInfo::m_lootObj;

// STATIC: 0x00BFA8E0
int32_t CGLootInfo::m_autolooted;

// STATIC: 0x00BFA8F0
static char s_lootSlotTexture[260];

// STATIC: 0x00ACEE24
static int32_t s_bindConfirmSlot = -1;

// STATIC: 0x00D38130
static uint32_t s_currencyMultiplier[3];

// STATIC: 0x00AF4914
static const char* s_currencyAmountTokens[3] = { "COPPER_AMOUNT", "SILVER_AMOUNT", "GOLD_AMOUNT" };

// STATIC: 0x00AF4920
static const char* s_currencyTextureTokens[3] = { "COPPER_AMOUNT_TEXTURE", "SILVER_AMOUNT_TEXTURE", "GOLD_AMOUNT_TEXTURE" };

// STATIC: 0x00AF492C
static const char* s_currencySymbolTokens[3] = { "COPPER_AMOUNT_SYMBOL", "SILVER_AMOUNT_SYMBOL", "GOLD_AMOUNT_SYMBOL" };

// STATIC: 0x00D3813C
static uint8_t s_lootInitialized;

// VERIFY: cde3bd49 auto
// OFFSET: 0x007E7E10
void GetCurrencyTextureString(int32_t amount, char* buffer, uint32_t size, int32_t height) {
    if (!amount) {
        SStrPrintf(buffer, size, FrameScript_GetText(s_currencyTextureTokens[0], -1, GENDER_NOT_APPLICABLE), 0, height, height);
        return;
    }

    int32_t parts[3];
    CurrencyBreakdown(amount, parts);
    *buffer = 0;

    for (int32_t i = 2; i >= 0; i--) {
        int32_t value = parts[i];

        if (!value) {
            continue;
        }

        if (*buffer) {
            SStrPack(buffer, " ", size);
        }

        char text[1024];
        SStrPrintf(text, sizeof(text), FrameScript_GetText(s_currencyTextureTokens[i], -1, GENDER_NOT_APPLICABLE), value, height, height);
        SStrPack(buffer, text, size);
    }
}

// VERIFY: 4a4538a7 auto
// OFFSET: 0x007E7FA0
void LootInitialize() {
    s_currencyMultiplier[0] = 1;
    s_currencyMultiplier[1] = 100;
    s_currencyMultiplier[2] = 10000;
    s_lootInitialized = 1;
}

// VERIFY: a4a2cb90 auto
// OFFSET: 0x00588170
void CGLootInfo::InitializeGame() {
    CGLootInfo::m_lootObj = WOWGUID();
}

// VERIFY: 58a0373b auto
// OFFSET: 0x007E7FD0
void LootDestroy() {
    s_lootInitialized = 0;
}

// VERIFY: c5565767 manual
// OFFSET: 0x007E7C70
void CurrencyBreakdown(int32_t amount, int32_t* parts) {
    for (int32_t i = 2; i; i--) {
        int32_t multiplier = static_cast<int32_t>(s_currencyMultiplier[i]);

        if (amount < multiplier) {
            parts[i] = 0;
        } else {
            parts[i] = amount / multiplier;
            amount %= multiplier;
        }
    }

    parts[0] = amount;
}

// VERIFY: 4b725563 auto
// OFFSET: 0x007E7D80
void GetCurrencyText(int32_t amount, char* buffer, uint32_t size, const char* separator) {
    int32_t parts[3];
    CurrencyBreakdown(amount, parts);
    *buffer = 0;

    for (int32_t i = 2; i >= 0; i--) {
        int32_t value = parts[i];

        if (!value) {
            continue;
        }

        if (*buffer) {
            SStrPack(buffer, separator, size);
        }

        char text[1024];
        SStrPrintf(text, sizeof(text), FrameScript_GetText(s_currencyAmountTokens[i], -1, GENDER_NOT_APPLICABLE), value);
        SStrPack(buffer, text, size);
    }
}

// VERIFY: b3b4c460 auto
// OFFSET: 0x007E7CC0
void GetCurrencyIcon(int32_t amount, char* buffer, uint32_t size) {
    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    SStrPrintf(buffer, size, "%s%s", path, *path ? "\\" : "");

    if (amount < 10) {
        SStrPack(buffer, "INV_Misc_Coin_05", size);
    } else if (amount < 100) {
        SStrPack(buffer, "INV_Misc_Coin_06", size);
    } else if (amount < 1000) {
        SStrPack(buffer, "INV_Misc_Coin_03", size);
    } else if (amount < 10000) {
        SStrPack(buffer, "INV_Misc_Coin_04", size);
    } else {
        SStrPack(buffer, amount >= 100000 ? "INV_Misc_Coin_02" : "INV_Misc_Coin_01", size);
    }
}

// VERIFY: 64cf72fb auto
// OFFSET: 0x00588180
void CGLootInfo::LeaveWorld() {
    CGGameUI::CloseLoot(true, false, false);
}

// VERIFY: c5c33408 auto
// OFFSET: 0x00588190
int32_t CGLootWindow::GetNumLootItems() {
    if (!CGLootInfo::m_lootObj) {
        return 0;
    }

    int32_t count = 0;

    for (uint32_t i = 0; i < 18; i++) {
        if (CGLootInfo::m_slots[i].m_itemID > 0) {
            count = i + 1;
        }
    }

    if (CGLootInfo::m_coins) {
        count++;
    }

    return count;
}

// VERIFY: c8b3d26f manual
// OFFSET: 0x00588210
int32_t CGLootInfo::GetLootItem(uint32_t slot) {
    if (!CGLootInfo::m_lootObj) {
        return 0;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return 0;
        }

        slot--;
    }

    if (slot >= 18) {
        return 0;
    }

    return CGLootInfo::m_slots[slot].m_itemID;
}

// VERIFY: 7938a0f3 manual
// OFFSET: 0x00588250
int32_t CGLootInfo::GetLootItemCount(uint32_t slot) {
    if (!CGLootInfo::m_lootObj) {
        return 0;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return 0;
        }

        slot--;
    }

    if (slot >= 18) {
        return 0;
    }

    return CGLootInfo::m_slots[slot].m_count;
}

// VERIFY: 71dd0d3a manual
// OFFSET: 0x00588290
int32_t CGLootInfo::GetLootItemRandomSuffix(uint32_t slot) {
    if (!CGLootInfo::m_lootObj) {
        return 0;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return 0;
        }

        slot--;
    }

    if (slot >= 18) {
        return 0;
    }

    return CGLootInfo::m_slots[slot].m_randomSuffix;
}

// VERIFY: cddf7717 manual
// OFFSET: 0x005882D0
int32_t CGLootInfo::GetLootItemRandomPropertyId(uint32_t slot) {
    if (!CGLootInfo::m_lootObj) {
        return 0;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return 0;
        }

        slot--;
    }

    if (slot >= 18) {
        return 0;
    }

    return CGLootInfo::m_slots[slot].m_randomPropertyId;
}

// VERIFY: 3e451cc5 auto
// OFFSET: 0x00588310
int32_t CGLootWindow::GetLootItemRarity(uint32_t slot) {
    if (!CGLootInfo::m_lootObj) {
        return 0;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return 0;
        }

        slot--;
    }

    if (slot >= 18) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(CGLootInfo::m_slots[slot].m_itemID, none, nullptr, nullptr, false);

    if (stats) {
        return stats->m_quality;
    }

    return -1;
}

// VERIFY: 7ef12198 manual
// OFFSET: 0x00588380
const char* CGLootInfo::GetLootSlotTexture(uint32_t slot) {
    if (!CGLootInfo::m_lootObj) {
        return nullptr;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            if (CGLootInfo::m_coins >= 0) {
                GetCurrencyIcon(CGLootInfo::m_coins, s_lootSlotTexture, sizeof(s_lootSlotTexture));
                return s_lootSlotTexture;
            }

            return nullptr;
        }

        slot--;
    }

    if (slot >= 18 || !CGLootInfo::m_slots[slot].m_itemID) {
        return nullptr;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    SStrPrintf(s_lootSlotTexture, sizeof(s_lootSlotTexture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(CGLootInfo::m_slots[slot].m_displayId));
    return s_lootSlotTexture;
}

// VERIFY: c23ec458 auto
// OFFSET: 0x00588430
void CGLootInfo::GetLootSlotText(char* buffer, uint32_t size, uint32_t slot) {
    *buffer = 0;

    if (!CGLootInfo::m_lootObj) {
        return;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            if (CGLootInfo::m_coins > 0) {
                GetCurrencyText(CGLootInfo::m_coins, buffer, size, "\n");
            }

            return;
        }

        slot--;
    }

    if (slot < 18 && CGLootInfo::m_slots[slot].m_itemID) {
        CGItem_C::BuildItemName(buffer, size, CGLootInfo::m_slots[slot].m_itemID, CGLootInfo::m_slots[slot].m_randomPropertyId);
    }
}

// VERIFY: 8cf50ab0 auto
// OFFSET: 0x005884A0
char* CGLootInfo::GetLootSlotLink(uint32_t slot, char* buffer, uint32_t size) {
    if (!CGLootInfo::m_lootObj) {
        return nullptr;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return nullptr;
        }

        slot--;
    }

    if (slot >= 18 || !CGLootInfo::m_slots[slot].m_itemID) {
        return nullptr;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(CGLootInfo::m_slots[slot].m_itemID, CGLootInfo::m_lootObj, nullptr, nullptr, false);

    if (!stats) {
        return nullptr;
    }

    SStrCopy(buffer, CGTooltip::GetItemHyperlinkString(CGLootInfo::m_slots[slot].m_itemID, stats->m_quality, 0, CGLootInfo::m_slots[slot].m_randomPropertyId, CGLootInfo::m_slots[slot].m_randomSuffix), size);
    return buffer;
}

// VERIFY: d8364d91 auto
// OFFSET: 0x00588530
LOOT_ACQUIRE CGLootDisplay::GetLootAcquireType() {
    return CGLootInfo::m_lootAcquire;
}

// VERIFY: 500b1dce manual
// OFFSET: 0x00588EB0
int32_t CGLootInfo::HasLoot() {
    if (!ClntObjMgrObjectPtr(CGLootInfo::m_lootObj, TYPE_OBJECT, __FILE__, __LINE__)) {
        return 0;
    }

    if (CGLootInfo::m_coins > 0) {
        return 1;
    }

    for (uint32_t i = 0; i < 18; i++) {
        if (CGLootInfo::m_slots[i].m_itemID) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 8a56cb16 manual
// OFFSET: 0x00588F90
void CGLootInfo::DoAutoLoot(int32_t a1) {
    if (CGLootInfo::m_itemsPending) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_OPENED, "%d", 1);
        return;
    }

    if (CGLootInfo::m_coins) {
        player->LootMoney();
    }

    int32_t confirmShown = 0;

    for (uint32_t i = 0; i < 18; i++) {
        ::LootSlot& slot = CGLootInfo::m_slots[i];

        if (!slot.m_itemID) {
            continue;
        }

        int32_t slotType = CGPlayer_C::GetLootItemSlotType(slot.m_slot);

        if (slotType && slotType != 4) {
            continue;
        }

        ItemStats_C* stats = g_itemDBCache.GetRecord(slot.m_itemID, CGLootInfo::m_lootObj, nullptr, nullptr, false);

        if (stats->m_bonding != 1 || stats->m_quality < 2 || (stats->m_flags[0] & 0x800) || slotType == 4) {
            SI2::PlayItemSound(ITEMSOUND_PICKUP, stats->m_displayId);
            CGPlayer_C::AutoStoreLootItem(slot.m_slot);
        } else if (!confirmShown) {
            FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_BIND_CONFIRM, "%u", CGLootInfo::m_coins ? i + 2 : i + 1);
            s_bindConfirmSlot = i;
            confirmShown = 1;
        }
    }

    player->SetAutoLoot(0);
    CGLootInfo::m_autolooted = 1;

    if (!CGLootInfo::HasLoot()) {
        CGGameUI::CloseLoot(true, false, false);
        return;
    }

    if (!a1) {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_OPENED, "%d", 1);
    }
}

// VERIFY: 98fb4bdd auto
// OFFSET: 0x00589100
void CGPlayer_C::OnLootClearMoney() {
    if (CGLootInfo::m_coins > 0) {
        CGLootInfo::m_coins = -1;
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_SLOT_CLEARED, "%d", 1);

        if (!CGLootInfo::HasLoot()) {
            CGGameUI::CloseLoot(true, false, false);
        }
    }
}

// VERIFY: 0569898a manual
// OFFSET: 0x00589140
void CGLootInfo::LootSlot(uint32_t slot) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            if (CGLootInfo::m_coins > 0) {
                player->LootMoney();
                CGLootInfo::m_coins = -1;
                FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_SLOT_CLEARED, "%d", 1);

                if (!CGLootInfo::HasLoot()) {
                    CGGameUI::CloseLoot(true, false, false);
                }
            }

            return;
        }

        slot--;
    }

    if (slot >= 18 || !CGLootInfo::m_slots[slot].m_itemID) {
        return;
    }

    ::LootSlot& entry = CGLootInfo::m_slots[slot];
    int32_t slotType = CGPlayer_C::GetLootItemSlotType(entry.m_slot);

    switch (slotType) {
    case 1:
        CGGameUI::DisplayError(ERR_LOOT_ROLL_PENDING);
        break;

    case 2:
        FrameScript_SignalEvent(SCRIPT_EVENT_OPEN_MASTER_LOOT_LIST, nullptr);
        break;

    case 3:
        CGGameUI::DisplayError(ERR_LOOT_CANT_LOOT_THAT);
        break;

    default: {
        ItemStats_C* stats = g_itemDBCache.GetRecord(entry.m_itemID, CGLootInfo::m_lootObj, nullptr, nullptr, false);

        if (!stats) {
            break;
        }

        if (stats->m_bonding != 1 || stats->m_quality < 2 || (stats->m_flags[0] & 0x800) || slotType == 4) {
            SI2::PlayItemSound(ITEMSOUND_PICKUP, stats->m_displayId);
            CGPlayer_C::AutoStoreLootItem(entry.m_slot);
        } else if (player->GetBag()->ItemCountValid(entry.m_itemID, stats, entry.m_count, true)) {
            FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_BIND_CONFIRM, "%u", CGLootInfo::m_coins ? slot + 2 : slot + 1);
            s_bindConfirmSlot = slot;
        }

        break;
    }
    }
}

// VERIFY: f307de88 manual
// OFFSET: 0x00589300
void CGLootInfo::ConfirmLootSlot(uint32_t slot) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return;
    }

    if (CGLootInfo::m_coins) {
        if (!slot) {
            return;
        }

        slot--;
    }

    if (static_cast<int32_t>(slot) != s_bindConfirmSlot || !CGLootInfo::m_slots[slot].m_itemID) {
        return;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(CGLootInfo::m_slots[slot].m_itemID, CGLootInfo::m_lootObj, nullptr, nullptr, false);
    SI2::PlayItemSound(ITEMSOUND_PICKUP, stats->m_displayId);
    CGPlayer_C::AutoStoreLootItem(CGLootInfo::m_slots[slot].m_slot);
    s_bindConfirmSlot = -1;
}

// VERIFY: 51446391 auto
// OFFSET: 0x005893A0
void CGLootInfo::GiveMasterLoot(uint32_t slot, WOWGUID target) {
    if (!CGLootInfo::m_lootObj) {
        return;
    }

    if (CGLootInfo::m_coins) {
        slot--;
    }

    if (slot >= 18 || !CGLootInfo::m_slots[slot].m_itemID || CGPlayer_C::GetLootItemSlotType(CGLootInfo::m_slots[slot].m_slot) != 2) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LOOT_MASTER_GIVE));
    msg.Put(CGLootInfo::m_lootObj.m_guid);
    msg.Put(CGLootInfo::m_slots[slot].m_slot);
    msg.Put(target.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 95e3db4e manual
// OFFSET: 0x00589460
void CGLootInfo::LootButtonItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    for (uint32_t i = 0; i < 18; i++) {
        ::LootSlot& slot = CGLootInfo::m_slots[i];

        if (slot.m_pending && slot.m_itemID == id) {
            if (slot.m_pending == SCRIPT_EVENT_LOOT_SLOT_CHANGED) {
                FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_SLOT_CHANGED, "%d", (CGLootInfo::m_coins != 0) + i + 1);
            }

            slot.m_pending = 0;
        }
    }

    if (--CGLootInfo::m_itemsPending) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_autoLoot) {
        CGLootInfo::DoAutoLoot(0);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_OPENED, "%d", 0);
    }
}

// VERIFY: 98edc3f9 manual
// OFFSET: 0x005896B0
void CGLootInfo::SetObject(CGObject_C* object, int32_t coins, LOOT_ACQUIRE lootAcquire) {
    if (CGLootInfo::m_lootObj) {
        CGObject_C* current = ClntObjMgrObjectPtr(CGLootInfo::m_lootObj, TYPE_OBJECT, __FILE__, __LINE__);

        if (current) {
            if (current->m_obj->m_type & TYPE_UNIT) {
                if (!CGLootInfo::HasLoot()) {
                    static_cast<CGUnit_C*>(current)->RemoveLootEffect();
                }
            } else if (current->m_obj->m_type & TYPE_GAMEOBJECT) {
                static_cast<CGGameObject_C*>(current)->SetDisplayState(1);
            } else if (current->m_obj->m_type & TYPE_ITEM) {
                CGGameUI::UnlockItem(CGLootInfo::m_lootObj);
            }
        }

        for (uint32_t i = 0; i < 18; i++) {
            if (CGLootInfo::m_slots[i].m_pending) {
                g_itemDBCache.CancelCallback(CGLootInfo::m_slots[i].m_itemID, CGLootInfo::LootButtonItemStatsCallback, nullptr);
            }
        }

        WOWGUID guid = object ? object->m_obj->m_guid : WOWGUID();

        if (CGLootInfo::m_lootObj != guid) {
            CGLootInfo::m_autolooted = 0;
        }

        CGLootInfo::m_lootObj = WOWGUID();
        memset(CGLootInfo::m_slots, 0, sizeof(CGLootInfo::m_slots));
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_CLOSED, nullptr);
    }

    if (!object) {
        return;
    }

    CGLootInfo::m_lootObj = object->m_obj->m_guid;
    CGLootInfo::m_coins = coins;
    CGLootInfo::m_itemsPending = 0;
    CGLootInfo::m_lootAcquire = lootAcquire;
    s_bindConfirmSlot = -1;
    uint32_t count = 0;
    memset(CGLootInfo::m_slots, 0, sizeof(CGLootInfo::m_slots));

    for (uint32_t i = 0; i < 18; i++) {
        int32_t itemId = CGPlayer_C::GetLootItem(i);

        if (!itemId) {
            continue;
        }

        ::LootSlot& slot = CGLootInfo::m_slots[count++];
        slot.m_itemID = itemId;
        slot.m_displayId = CGPlayer_C::GetLootItemDisplayId(i);
        slot.m_count = CGPlayer_C::GetLootItemCount(i);
        slot.m_randomSuffix = CGPlayer_C::GetLootItemRandomSuffix(i);
        slot.m_randomPropertyId = CGPlayer_C::GetLootItemRandomPropertyId(i);
        slot.m_slot = static_cast<uint8_t>(i);
        slot.m_slotType = CGPlayer_C::GetLootItemSlotType(i);
    }

    for (uint32_t i = 0; i < count; i++) {
        if (g_itemDBCache.GetRecord(CGLootInfo::m_slots[i].m_itemID, CGLootInfo::m_lootObj, CGLootInfo::LootButtonItemStatsCallback, nullptr, false)) {
            CGLootInfo::m_slots[i].m_pending = 0;
        } else {
            CGLootInfo::m_itemsPending++;
            CGLootInfo::m_slots[i].m_pending = 1;
        }
    }

    if (CGLootInfo::m_itemsPending) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_autoLoot) {
        CGLootInfo::DoAutoLoot(0);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_OPENED, "%d", 0);
    }
}

// VERIFY: 464fbb82 manual
// OFFSET: 0x00589920
void CGLootInfo::ClearSlot(uint8_t slot) {
    uint32_t index = 0;

    while (!CGLootInfo::m_slots[index].m_itemID || CGLootInfo::m_slots[index].m_slot != slot) {
        if (++index >= 18) {
            return;
        }
    }

    if (CGLootInfo::m_slots[index].m_pending) {
        g_itemDBCache.CancelCallback(CGLootInfo::m_slots[index].m_itemID, CGLootInfo::LootButtonItemStatsCallback, nullptr);
    }

    memset(&CGLootInfo::m_slots[index], 0, sizeof(CGLootInfo::m_slots[index]));

    if (CGLootInfo::m_coins) {
        index++;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_SLOT_CLEARED, "%d", index + 1);

    if (!CGLootInfo::HasLoot()) {
        CGGameUI::CloseLoot(true, false, false);
    }
}

// VERIFY: 653fe234 manual
// OFFSET: 0x00589A30
void CGLootInfo::OnLootSlotChanged(uint8_t slot) {
    uint32_t index = 0xFFFFFFFF;

    for (uint32_t i = 0; i < 18; i++) {
        if (CGLootInfo::m_slots[i].m_itemID && CGLootInfo::m_slots[i].m_slot == slot) {
            index = i;
            break;
        }
    }

    if (index == 0xFFFFFFFF) {
        for (uint32_t i = 0; i < 18; i++) {
            if (!CGLootInfo::m_slots[i].m_itemID) {
                index = i;
                break;
            }
        }

        if (index >= 18) {
            return;
        }
    }

    ::LootSlot& entry = CGLootInfo::m_slots[index];
    entry.m_itemID = CGPlayer_C::GetLootItem(slot);
    entry.m_displayId = CGPlayer_C::GetLootItemDisplayId(slot);
    entry.m_count = CGPlayer_C::GetLootItemCount(slot);
    entry.m_randomSuffix = CGPlayer_C::GetLootItemRandomSuffix(slot);
    entry.m_randomPropertyId = CGPlayer_C::GetLootItemRandomPropertyId(slot);
    entry.m_slot = slot;
    entry.m_slotType = CGPlayer_C::GetLootItemSlotType(slot);

    if (g_itemDBCache.GetRecord(entry.m_itemID, CGLootInfo::m_lootObj, CGLootInfo::LootButtonItemStatsCallback, nullptr, false)) {
        entry.m_pending = 0;
    } else {
        CGLootInfo::m_itemsPending++;
        entry.m_pending = SCRIPT_EVENT_LOOT_SLOT_CHANGED;
    }

    if (CGLootInfo::m_coins) {
        index++;
    }

    if (!CGLootInfo::m_itemsPending) {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_SLOT_CHANGED, "%d", index + 1);
    }
}

// VERIFY: 29eb8814 auto
// OFFSET: 0x005EEB70
void CGLootInfo::ShutdownGame() {
}

// VERIFY: 072a9880 auto
// OFFSET: 0x007E7EE0
void GetCurrencyShortText(int32_t amount, char* buffer, uint32_t size, const char* separator) {
    if (!amount) {
        SStrPrintf(buffer, size, "%d%s", 0, FrameScript_GetText(s_currencySymbolTokens[0], -1, GENDER_NOT_APPLICABLE));
        return;
    }

    int32_t parts[3];
    CurrencyBreakdown(amount, parts);
    *buffer = 0;

    for (int32_t i = 2; i >= 0; i--) {
        int32_t value = parts[i];

        if (!value) {
            continue;
        }

        if (*buffer) {
            SStrPack(buffer, separator, size);
        }

        char text[1024];
        SStrPrintf(text, sizeof(text), "%d%s", value, FrameScript_GetText(s_currencySymbolTokens[i], -1, GENDER_NOT_APPLICABLE));
        SStrPack(buffer, text, size);
    }
}
