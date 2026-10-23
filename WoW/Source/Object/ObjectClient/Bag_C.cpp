#include "Bag_C.h"
#include "WoW/Common/ComponentCore/Types.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/AuctionHouse.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// STATIC: 0x00A37B10
static const BAGSLOTRANGE s_bagSlotRanges[8] = { { BACKPACK_FIRST, BACKPACK_LAST }, { INVSLOT_BAG0, INVSLOT_BAG3 }, { KEYRING_FIRST, KEYRING_LAST }, { BANKGENERIC_FIRST, BANKGENERIC_LAST }, { BANKBAG_FIRST, BANKBAG_LAST }, { CURRENCYTOKEN_FIRST, CURRENCYTOKEN_LAST }, { INVSLOT_HEAD, INVSLOT_TABARD }, { BUYBACK_FIRST, BUYBACK_LAST } };

// VERIFY: 3944bddf auto
// OFFSET: 0x00753B00
static int32_t FindBagFamilyCallback(const CGItem_C* item, void* param) {
    return *static_cast<uint32_t*>(param) & item->GetBagFamily();
}

// VERIFY: d68f4a94 auto
// OFFSET: 0x00753FA0
static int32_t UnlockItemTypeCallback(const CGItem_C* item, void* param) {
    ItemTypeCountParam* count = static_cast<ItemTypeCountParam*>(param);

    if ((static_cast<int32_t>(item->m_obj->m_entryID) != count->m_itemId && count->m_itemId != -1) || CGTradeInfo::IsItemBeingTraded(item->m_obj->m_guid)) {
        return 0;
    }

    WOWGUID guid = item->m_obj->m_guid;

    if (!CGMailInfo::IsSendItem(guid) && guid != CGAuctionHouse::m_sellItem && !CGAuctionHouse::IsItemForSell(guid)) {
        const_cast<CGItem_C*>(item)->Unlock();
    }

    return 0;
}

// VERIFY: 94284adc manual
// OFFSET: 0x00589C00
int32_t CGBag_C::FindItemSlot(WOWGUID guid) const {
    for (uint32_t slot = 0; slot < static_cast<uint32_t>(this->m_numSlots); slot++) {
        if (this->m_slots[slot] == guid) {
            return slot;
        }
    }

    return -1;
}

// VERIFY: 578d255a manual
// OFFSET: 0x00754040
int32_t CGBag_C::FindItemSlot(const CGItem_C* item) const {
    if (!item) {
        return -1;
    }

    for (uint32_t slot = 0; slot < static_cast<uint32_t>(this->m_numSlots); slot++) {
        WOWGUID guid = slot < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();

        if (ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__) == item) {
            return slot;
        }
    }

    return -1;
}

// VERIFY: 121306f8 manual
// OFFSET: 0x00754390
const CGItem_C* CGBag_C::GetItemPointer(int32_t slot) const {
    if ((this->m_hasBank && ((slot >= BANKGENERIC_FIRST && slot <= BANKGENERIC_LAST) || static_cast<uint32_t>(slot - BANKBAG_FIRST) <= BANKBAG_LAST - BANKBAG_FIRST) && !CGBankInfo::m_banker) || slot >= this->m_numSlots) {
        return nullptr;
    }

    WOWGUID guid = static_cast<uint32_t>(slot) < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();

    return static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));
}

// VERIFY: 6a3b7079 manual
// OFFSET: 0x00754400
CGItem_C* CGBag_C::GetItemPointer(int32_t slot) {
    if ((this->m_hasBank && ((slot >= BANKGENERIC_FIRST && slot <= BANKGENERIC_LAST) || static_cast<uint32_t>(slot - BANKBAG_FIRST) <= BANKBAG_LAST - BANKBAG_FIRST) && !CGBankInfo::m_banker) || slot >= this->m_numSlots) {
        return nullptr;
    }

    WOWGUID guid = static_cast<uint32_t>(slot) < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();

    return static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));
}

// VERIFY: 8829b660 manual
// OFFSET: 0x00754470
int32_t CGBag_C::GetNumFreeInvSlots() {
    int32_t freeSlots = 0;

    for (uint32_t slot = 0; slot < static_cast<uint32_t>(this->m_numSlots); slot++) {
        if (this->m_hasBank) {
            if (slot <= INVSLOT_TABARD || (slot >= BANKGENERIC_FIRST && slot <= BANKGENERIC_LAST) || (slot >= BANKBAG_FIRST && slot <= BANKBAG_LAST) || (slot >= KEYRING_FIRST && slot <= KEYRING_LAST) || (slot >= BUYBACK_FIRST && slot <= BUYBACK_LAST) || slot - CURRENCYTOKEN_FIRST <= CURRENCYTOKEN_LAST - CURRENCYTOKEN_FIRST) {
                continue;
            }

            if (slot - INVSLOT_BAG0 <= INVSLOT_BAG3 - INVSLOT_BAG0) {
                WOWGUID guid = slot < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();
                CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

                if (item && item->GetClassID() == 1) {
                    freeSlots += item->GetBag()->GetNumFreeInvSlots();
                }

                continue;
            }
        }

        if (slot >= static_cast<uint32_t>(this->m_numSlots) || !this->m_slots[slot]) {
            freeSlots++;
        }
    }

    return freeSlots;
}

// VERIFY: 8d4f2769 auto
// OFFSET: 0x00754560
int32_t GetItemTypeCountCallback(const CGItem_C* item, void* param) {
    ItemTypeCountParam* count = static_cast<ItemTypeCountParam*>(param);

    if (static_cast<int32_t>(item->m_obj->m_entryID) == count->m_itemId || count->m_itemId == -1) {
        count->m_count += item->m_item->m_itemFieldStackCount;
    }

    return 0;
}

// VERIFY: af9218e7 manual
// OFFSET: 0x00754940
uint32_t CGBag_C::GetAvgEquippedDurability() const {
    int32_t durability = 0;
    int32_t maxDurability = 0;

    for (uint32_t slot = 0; slot <= INVSLOT_TABARD; slot++) {
        if ((this->m_hasBank && ((slot >= BANKGENERIC_FIRST && slot <= BANKGENERIC_LAST) || slot - BANKBAG_FIRST <= BANKBAG_LAST - BANKBAG_FIRST) && !CGBankInfo::m_banker) || static_cast<int32_t>(slot) >= this->m_numSlots) {
            continue;
        }

        WOWGUID guid = slot < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

        if (!item) {
            continue;
        }

        durability += (item->m_item->m_itemFieldFlags >> 3) & 1 ? 0 : item->m_item->m_itemFieldDurability;
        maxDurability += (item->m_item->m_itemFieldFlags >> 3) & 1 ? 0 : item->m_item->m_itemFieldMaxdurability;
    }

    if (!maxDurability) {
        return 0;
    }

    return static_cast<float>(durability) / maxDurability * 100.0f;
}

// VERIFY: c1b9cf4f auto
// OFFSET: 0x005125B0
int32_t CGBag_C::GetNumItems() const {
    int32_t count = 0;

    for (int32_t i = 0; i < this->m_numSlots; i++) {
        if (this->m_slots[i]) {
            count++;
        }
    }

    return count;
}

// VERIFY: 724d9331 auto
// OFFSET: 0x00753A50
static int32_t __stdcall SlotMatchesSearchFlags(uint32_t slot, uint32_t flags) {
    return (slot <= INVSLOT_TABARD && (flags & BAG_SEARCH_EQUIPMENT)) || (slot - BACKPACK_FIRST <= BACKPACK_LAST - BACKPACK_FIRST && (flags & BAG_SEARCH_BACKPACK)) || (slot - INVSLOT_BAG0 <= INVSLOT_BAG3 - INVSLOT_BAG0 && (flags & BAG_SEARCH_BAG_SLOTS)) || (((slot >= BANKGENERIC_FIRST && slot <= BANKGENERIC_LAST) || slot - BANKBAG_FIRST <= BANKBAG_LAST - BANKBAG_FIRST) && (flags & BAG_SEARCH_BANK)) || (slot - KEYRING_FIRST <= KEYRING_LAST - KEYRING_FIRST && (flags & BAG_SEARCH_KEYRING)) || (slot - CURRENCYTOKEN_FIRST <= CURRENCYTOKEN_LAST - CURRENCYTOKEN_FIRST && (flags & BAG_SEARCH_CURRENCY));
}

// VERIFY: ddfa3202 auto
// OFFSET: 0x00753B70
static int32_t FindItemFullNameCallback(const CGItem_C* item, void* param) {
    char fullName[1024];
    item->GetFullItemName(fullName, sizeof(fullName));
    return SStrCmpUTF8I(static_cast<const char*>(param), fullName, STORM_MAX_STR) == 0;
}

// VERIFY: e5b71395 auto
// OFFSET: 0x00753BB0
int32_t GetDisplayErrorByInventoryError(int32_t error) {
    switch (error) {
        case BAG_OK:
        case BAG_AUTOEQUIP_BIND_CONFIRM:
        case BAG_NO_DISPLAY_ERROR:
            return ERR_MAX_VALUE;

        case BAG_CANT_EQUIP_LEVEL_I:
            return ERR_CANT_EQUIP_LEVEL_I;

        case BAG_CANT_EQUIP_SKILL:
            return ERR_CANT_EQUIP_SKILL;

        case BAG_WRONG_SLOT:
            return ERR_WRONG_SLOT;

        case BAG_IN_BAG:
            return ERR_BAG_IN_BAG;

        case BAG_TRADE_EQUIPPED_BAG:
            return ERR_TRADE_EQUIPPED_BAG;

        case BAG_AMMO_ONLY:
            return ERR_AMMO_ONLY;

        case BAG_PROFICIENCY_NEEDED:
            return ERR_PROFICIENCY_NEEDED;

        case BAG_NO_SLOT_AVAILABLE:
        case BAG_NO_SLOT_AVAILABLE_2:
        case BAG_NO_SLOT_AVAILABLE_3:
            return ERR_NO_SLOT_AVAILABLE;

        case BAG_CANT_EQUIP_EVER:
        case BAG_CANT_EQUIP_EVER_2:
            return ERR_CANT_EQUIP_EVER;

        case BAG_2HANDED_EQUIPPED:
            return ERR_2HANDED_EQUIPPED;

        case BAG_2HSKILLNOTFOUND:
            return ERR_2HSKILLNOTFOUND;

        case BAG_WRONG_BAG_TYPE:
        case BAG_WRONG_BAG_TYPE_2:
            return ERR_WRONG_BAG_TYPE;

        case BAG_ITEM_MAX_COUNT:
            return ERR_ITEM_MAX_COUNT;

        case BAG_CANT_STACK:
        case BAG_CANT_STACK_2:
            return ERR_CANT_STACK;

        case BAG_NOT_EQUIPPABLE:
            return ERR_NOT_EQUIPPABLE;

        case BAG_CANT_SWAP:
            return ERR_CANT_SWAP;

        case BAG_SLOT_EMPTY:
            return ERR_SLOT_EMPTY;

        case BAG_ITEM_NOT_FOUND:
        case BAG_ITEM_NOT_FOUND_2:
            return ERR_ITEM_NOT_FOUND;

        case BAG_DROP_BOUND_ITEM:
            return ERR_DROP_BOUND_ITEM;

        case BAG_OUT_OF_RANGE:
            return ERR_OUT_OF_RANGE;

        case BAG_TOO_FEW_TO_SPLIT:
            return ERR_TOO_FEW_TO_SPLIT;

        case BAG_SPLIT_FAILED:
            return ERR_SPLIT_FAILED;

        case BAG_SPELL_FAILED_REAGENTS_GENERIC:
            return ERR_SPELL_FAILED_REAGENTS_GENERIC;

        case BAG_NOT_ENOUGH_MONEY:
            return ERR_NOT_ENOUGH_MONEY;

        case BAG_NOT_A_BAG:
            return ERR_NOT_A_BAG;

        case BAG_DESTROY_NONEMPTY_BAG:
            return ERR_DESTROY_NONEMPTY_BAG;

        case BAG_NOT_OWNER:
            return ERR_NOT_OWNER;

        case BAG_ONLY_ONE_QUIVER:
            return ERR_ONLY_ONE_QUIVER;

        case BAG_NO_BANK_SLOT:
            return ERR_NO_BANK_SLOT;

        case BAG_NO_BANK_HERE:
            return ERR_NO_BANK_HERE;

        case BAG_ITEM_LOCKED:
            return ERR_ITEM_LOCKED;

        case BAG_GENERIC_STUNNED:
            return ERR_GENERIC_STUNNED;

        case BAG_PLAYER_DEAD:
            return ERR_PLAYER_DEAD;

        case BAG_CLIENT_LOCKED_OUT:
            return ERR_CLIENT_LOCKED_OUT;

        case BAG_INTERNAL_BAG_ERROR:
        case BAG_INTERNAL_BAG_ERROR_2:
            return ERR_INTERNAL_BAG_ERROR;

        case BAG_ONLY_ONE_BOLT:
            return ERR_ONLY_ONE_BOLT;

        case BAG_ONLY_ONE_AMMO:
            return ERR_ONLY_ONE_AMMO;

        case BAG_CANT_WRAP_STACKABLE:
            return ERR_CANT_WRAP_STACKABLE;

        case BAG_CANT_WRAP_EQUIPPED:
            return ERR_CANT_WRAP_EQUIPPED;

        case BAG_CANT_WRAP_WRAPPED:
            return ERR_CANT_WRAP_WRAPPED;

        case BAG_CANT_WRAP_BOUND:
            return ERR_CANT_WRAP_BOUND;

        case BAG_CANT_WRAP_UNIQUE:
            return ERR_CANT_WRAP_UNIQUE;

        case BAG_CANT_WRAP_BAGS:
            return ERR_CANT_WRAP_BAGS;

        case BAG_LOOT_GONE:
            return ERR_LOOT_GONE;

        case BAG_INV_FULL:
            return ERR_INV_FULL;

        case BAG_BANK_FULL:
            return ERR_BANK_FULL;

        case BAG_VENDOR_SOLD_OUT:
        case BAG_VENDOR_SOLD_OUT_2:
            return ERR_VENDOR_SOLD_OUT;

        case BAG_OBJECT_IS_BUSY:
            return ERR_OBJECT_IS_BUSY;

        case BAG_CANT_BE_DISENCHANTED:
            return ERR_CANT_BE_DISENCHANTED;

        case BAG_NOT_IN_COMBAT:
            return ERR_NOT_IN_COMBAT;

        case BAG_NOT_WHILE_DISARMED:
            return ERR_NOT_WHILE_DISARMED;

        case BAG_CANT_EQUIP_RANK:
            return ERR_CANT_EQUIP_RANK;

        case BAG_CANT_EQUIP_REPUTATION:
            return ERR_CANT_EQUIP_REPUTATION;

        case BAG_TOO_MANY_SPECIAL_BAGS:
            return ERR_TOO_MANY_SPECIAL_BAGS;

        case BAG_LOOT_CANT_LOOT_THAT_NOW:
            return ERR_LOOT_CANT_LOOT_THAT_NOW;

        case BAG_ITEM_UNIQUE_EQUIPPABLE:
            return ERR_ITEM_UNIQUE_EQUIPPABLE;

        case BAG_VENDOR_MISSING_TURNINS:
            return ERR_VENDOR_MISSING_TURNINS;

        case BAG_NOT_ENOUGH_HONOR_POINTS:
            return ERR_NOT_ENOUGH_HONOR_POINTS;

        case BAG_NOT_ENOUGH_ARENA_POINTS:
            return ERR_NOT_ENOUGH_ARENA_POINTS;

        case BAG_ITEM_MAX_COUNT_SOCKETED:
            return ERR_ITEM_MAX_COUNT_SOCKETED;

        case BAG_MAIL_BOUND_ITEM:
            return ERR_MAIL_BOUND_ITEM;

        case BAG_ITEM_MAX_COUNT_EQUIPPED_SOCKETED:
            return ERR_ITEM_MAX_COUNT_EQUIPPED_SOCKETED;

        case BAG_ITEM_UNIQUE_EQUIPPABLE_SOCKETED:
            return ERR_ITEM_UNIQUE_EQUIPPABLE_SOCKETED;

        case BAG_TOO_MUCH_GOLD:
            return ERR_TOO_MUCH_GOLD;

        case BAG_NOT_DURING_ARENA_MATCH:
            return ERR_NOT_DURING_ARENA_MATCH;

        case BAG_TRADE_BOUND_ITEM:
            return ERR_TRADE_BOUND_ITEM;

        case BAG_CANT_EQUIP_RATING:
            return ERR_CANT_EQUIP_RATING;

        case BAG_NOT_SAME_ACCOUNT:
            return ERR_NOT_SAME_ACCOUNT;

        case BAG_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS:
            return ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS;

        case BAG_ITEM_MAX_LIMIT_CATEGORY_SOCKETED_EXCEEDED_IS:
            return ERR_ITEM_MAX_LIMIT_CATEGORY_SOCKETED_EXCEEDED_IS;

        case BAG_SCALING_STAT_ITEM_LEVEL_EXCEEDED:
            return ERR_SCALING_STAT_ITEM_LEVEL_EXCEEDED;

        case BAG_PURCHASE_LEVEL_TOO_LOW:
            return ERR_PURCHASE_LEVEL_TOO_LOW;

        case BAG_CANT_EQUIP_NEED_TALENT:
            return ERR_CANT_EQUIP_NEED_TALENT;

        case BAG_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS:
            return ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;

        case BAG_SHAPESHIFT_FORM_CANNOT_EQUIP:
            return ERR_SHAPESHIFT_FORM_CANNOT_EQUIP;

        case BAG_ITEM_INVENTORY_FULL_SATCHEL:
            return ERR_ITEM_INVENTORY_FULL_SATCHEL;

        default:
            return ERR_BAG_FULL;
    }
}

// VERIFY: 9c2bc99e auto
// OFFSET: 0x00753F60
int32_t GetItemTypeChargesCallback(const CGItem_C* item, void* param) {
    ItemTypeCountParam* count = static_cast<ItemTypeCountParam*>(param);

    if ((static_cast<int32_t>(item->m_obj->m_entryID) == count->m_itemId || count->m_itemId == -1) && const_cast<CGItem_C*>(item)->GetEnchantmentSpell(0)) {
        int32_t charges = const_cast<CGItem_C*>(item)->GetCurrentSpellCharges();

        if (charges <= 0) {
            charges = -charges;
        }

        count->m_count += charges;
    }

    return 0;
}

// VERIFY: b4bf3df4 auto
// OFFSET: 0x00754020
int32_t FindItemIDCallback(const CGItem_C* item, void* param) {
    return static_cast<int32_t>(item->m_obj->m_entryID) == *static_cast<int32_t*>(param);
}

// VERIFY: 45f963f6 manual
// OFFSET: 0x007541F0
bool CGBag_C::FindTotemCategory(int32_t type, int32_t& mask, bool recurse, uint32_t flags) {
    if (!(flags & BAG_SEARCH_TOTEM_LOCATIONS)) {
        flags |= BAG_SEARCH_TOTEM_LOCATIONS;
    }

    uint32_t slot = this->m_hasBank ? s_bagSlotRanges[0].m_first : 0;

    while (slot < static_cast<uint32_t>(this->m_numSlots)) {
        if (!this->m_hasBank || SlotMatchesSearchFlags(slot, flags)) {
            WOWGUID guid = slot < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

            if (item && (!this->m_hasBank || slot < BUYBACK_FIRST || slot > BUYBACK_LAST)) {
                WOWGUID none = { 0 };
                ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

                if (stats && stats->m_totemCategory) {
                    TotemCategoryRec* category = g_totemCategoryDB.GetRecord(stats->m_totemCategory);

                    if (category && category->m_totemCategoryType == type) {
                        mask &= ~category->m_totemCategoryMask;

                        if (!mask) {
                            return true;
                        }
                    }
                }

                if (recurse && (item->m_obj->m_type & TYPE_CONTAINER) && item->GetBag() && item->GetBag()->FindTotemCategory(type, mask, false, flags)) {
                    return true;
                }
            }
        }

        if (slot < static_cast<uint32_t>(this->m_numSlots)) {
            uint32_t i = 0;

            if (this->m_hasBank) {
                for (; i < sizeof(s_bagSlotRanges) / sizeof(s_bagSlotRanges[0]); i++) {
                    if (s_bagSlotRanges[i].m_last == slot) {
                        break;
                    }
                }
            }

            if (this->m_hasBank && i < sizeof(s_bagSlotRanges) / sizeof(s_bagSlotRanges[0])) {
                slot = i >= sizeof(s_bagSlotRanges) / sizeof(s_bagSlotRanges[0]) - 1 ? this->m_numSlots : s_bagSlotRanges[i + 1].m_first;
            } else {
                slot++;
            }
        } else {
            slot = this->m_numSlots;
        }
    }

    return false;
}

// VERIFY: 0476a53f manual
// OFFSET: 0x007540B0
bool CGBag_C::ContainsItem(WOWGUID guid, bool recurse, uint32_t flags) const {
    int32_t hasBank = this->m_hasBank;
    uint32_t slot = hasBank ? s_bagSlotRanges[0].m_first : 0;

    while (slot < static_cast<uint32_t>(this->m_numSlots)) {
        if (!hasBank || SlotMatchesSearchFlags(slot, flags)) {
            WOWGUID slotGuid = slot < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(slotGuid, TYPE_ITEM, __FILE__, __LINE__));

            if (item && (!hasBank || slot < BUYBACK_FIRST || slot > BUYBACK_LAST)) {
                WOWGUID itemGuid = slot < static_cast<uint32_t>(this->m_numSlots) ? this->m_slots[slot] : WOWGUID();

                if (itemGuid == guid) {
                    return true;
                }

                if (recurse && (item->m_obj->m_type & TYPE_CONTAINER) && item->GetBag() && item->GetBag()->ContainsItem(guid, false, flags)) {
                    return true;
                }
            }
        }

        if (slot < static_cast<uint32_t>(this->m_numSlots)) {
            uint32_t i = 0;

            if (this->m_hasBank) {
                for (; i < sizeof(s_bagSlotRanges) / sizeof(s_bagSlotRanges[0]); i++) {
                    if (s_bagSlotRanges[i].m_last == slot) {
                        break;
                    }
                }
            }

            if (this->m_hasBank && i < sizeof(s_bagSlotRanges) / sizeof(s_bagSlotRanges[0])) {
                slot = i >= sizeof(s_bagSlotRanges) / sizeof(s_bagSlotRanges[0]) - 1 ? this->m_numSlots : s_bagSlotRanges[i + 1].m_first;
            } else {
                slot++;
            }
        } else {
            slot = this->m_numSlots;
        }
    }

    return false;
}

// VERIFY: 6ed45ee8 auto
// OFFSET: 0x007548F0
bool CGBag_C::FindTotemCategory(int32_t category, uint32_t flags) {
    TotemCategoryRec* rec = g_totemCategoryDB.GetRecord(category);

    if (!rec) {
        return false;
    }

    int32_t mask = rec->m_totemCategoryMask;
    return this->FindTotemCategory(rec->m_totemCategoryType, mask, true, flags);
}

// VERIFY: 12f34396 auto
// OFFSET: 0x00754A20
CGItem_C* CGBag_C::FindItemOfType(int32_t itemId, uint32_t flags) {
    WOWGUID container;
    uint32_t slot;
    return this->FindItem(FindItemIDCallback, &itemId, container, slot, flags);
}

// VERIFY: ef1d9110 auto
// OFFSET: 0x00754A50
CGItem_C* CGBag_C::FindItemOfType(int32_t itemId, WOWGUID& container, uint32_t& slot, uint32_t flags) {
    return this->FindItem(FindItemIDCallback, &itemId, container, slot, flags);
}

// VERIFY: 98f5a653 auto
// OFFSET: 0x00754D00
int32_t CGBag_C::GetItemTypeCount(int32_t itemId, uint32_t flags) {
    if (itemId == -2) {
        return this->GetNumFreeInvSlots();
    }

    ItemTypeCountParam param;
    param.m_itemId = itemId;
    param.m_count = 0;
    WOWGUID container;
    uint32_t slot;
    this->FindItem(GetItemTypeCountCallback, &param, container, slot, flags);
    return param.m_count;
}

// VERIFY: a7154eb9 auto
// OFFSET: 0x00754D50
int32_t CGBag_C::GetItemTypeCharges(int32_t itemId, uint32_t flags) {
    ItemTypeCountParam param;
    param.m_itemId = itemId;
    param.m_count = 0;
    WOWGUID container;
    uint32_t slot;
    this->FindItem(GetItemTypeChargesCallback, &param, container, slot, flags);
    return param.m_count;
}

// VERIFY: 0a0051b7 auto
// OFFSET: 0x00753AC0
int32_t FindItemClassCallback(const CGItem_C* item, void* param) {
    const uint32_t* classParam = static_cast<const uint32_t*>(param);
    return item->GetClassID() == classParam[0] && (!classParam[1] || ((1 << item->GetSubtypeID()) & classParam[1]));
}

// VERIFY: 4b4db806 auto
// OFFSET: 0x00754A80
CGItem_C* CGBag_C::FindItemOfClass(int32_t classId, uint32_t subclassMask, uint32_t flags) {
    uint32_t param[2];
    param[0] = classId;
    param[1] = subclassMask;
    WOWGUID container;
    uint32_t slot;
    return this->FindItem(FindItemClassCallback, param, container, slot, flags);
}

// VERIFY: 3ef5ccd0 manual
// OFFSET: 0x00512590
uint64_t CGBag_C::GetItemGuid(uint32_t slot) const {
    if (slot < static_cast<uint32_t>(this->m_numSlots)) {
        return this->m_slots[slot].m_guid;
    }

    return 0;
}

// VERIFY: eb9a8ff7 manual
// OFFSET: 0x007545D0
static int32_t FindItemLinkCallback(const CGItem_C* item, void* param) {
    const ItemLinkInfo* link = static_cast<const ItemLinkInfo*>(param);
    CGItem_C* object = const_cast<CGItem_C*>(item);

    if (static_cast<int32_t>(link->m_itemId) != object->m_obj->m_entryID) {
        return 0;
    }

    if (object->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) {
        if (link->m_enchants[0]) {
            return 0;
        }

        for (uint32_t i = 0; i < 3; i++) {
            if (link->m_enchants[2 + i]) {
                return 0;
            }
        }

        if (link->m_randomPropertiesId) {
            return 0;
        }

        return !link->m_propertySeed;
    }

    if (link->m_enchants[0] && link->m_enchants[0] != object->GetEnchantmentId(0)) {
        return 0;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (link->m_enchants[2 + i] && link->m_enchants[2 + i] != object->GetEnchantmentId(2 + i)) {
            return 0;
        }
    }

    if (link->m_randomPropertiesId && link->m_randomPropertiesId != object->m_item->m_itemFieldRandomPropertiesId) {
        return 0;
    }

    if (link->m_propertySeed && link->m_propertySeed != object->m_item->m_itemFieldPropertySeed) {
        return 0;
    }

    return 1;
}

// VERIFY: b7f49e2f auto
// OFFSET: 0x00753B20
static int32_t FindItemNameCallback(const CGItem_C* item, void* param) {
    const char* name = static_cast<const char*>(param);
    int32_t length = SStrLenUTF8(name);
    char fullName[1024];
    item->GetFullItemName(fullName, sizeof(fullName));
    return SStrCmpUTF8I(name, fullName, length) == 0;
}

// VERIFY: 19b67d6e manual
// OFFSET: 0x00754AF0
CGItem_C* CGBag_C::FindItemByName(const char* name, uint32_t flags) {
    if (!name || !*name) {
        return nullptr;
    }

    WOWGUID container;
    uint32_t slot;

    if (!SStrStr(name, "item:")) {
        return this->FindItem(&FindItemNameCallback, const_cast<char*>(name), container, slot, flags);
    }

    ItemLinkInfo link;
    link.Clear();
    link.GetItemIdFromLink(name);
    return this->FindItem(&FindItemLinkCallback, &link, container, slot, flags);
}

// VERIFY: 57bdbcdf manual
// OFFSET: 0x00754B90
CGItem_C* CGBag_C::GetItemIdByName(const char* name, WOWGUID& container, uint32_t& slot, uint32_t flags) {
    if (!name || !*name) {
        return nullptr;
    }

    if (!SStrStr(name, "item:")) {
        return this->FindItem(&FindItemNameCallback, const_cast<char*>(name), container, slot, flags);
    }

    ItemLinkInfo link;
    link.Clear();
    link.GetItemIdFromLink(name);
    return this->FindItem(&FindItemLinkCallback, &link, container, slot, flags);
}

// VERIFY: f40a505c manual
// OFFSET: 0x00754C30
CGItem_C* CGBag_C::FindItemByFullName(const char* name, uint32_t flags) {
    if (!name || !*name) {
        return nullptr;
    }

    if (!SStrStr(name, "item:")) {
        return this->FindItem(&FindItemFullNameCallback, const_cast<char*>(name), flags);
    }

    ItemLinkInfo link;
    link.Clear();
    link.GetItemIdFromLink(name);
    return this->FindItem(&FindItemLinkCallback, &link, flags);
}

// VERIFY: cfc4d837 auto
// OFFSET: 0x00754CD0
CGItem_C* CGBag_C::FindItem(int32_t (*callback)(const CGItem_C* item, void* param), void* param, uint32_t flags) {
    WOWGUID container;
    return this->FindItem(callback, param, container, flags, flags);
}

// VERIFY: 35fbcc70 manual
// OFFSET: 0x007546F0
CGItem_C* CGBag_C::FindItem(int32_t (*callback)(const CGItem_C* item, void* param), void* param, WOWGUID& container, uint32_t& slot, uint32_t flags) {
    if (this->m_hasBank && !(flags & BAG_SEARCH_DEFAULT_LOCATIONS)) {
        flags |= BAG_SEARCH_DEFAULT_LOCATIONS;
    }

    container = WOWGUID();
    slot = 0;
    uint32_t numSlots = this->m_numSlots;
    CGItem_C* best = nullptr;
    uint32_t index = this->m_hasBank ? 23 : 0;

    if (index >= numSlots) {
        return best;
    }

    while (true) {
        if (!this->m_hasBank || SlotMatchesSearchFlags(index, flags)) {
            WOWGUID guid = index < numSlots ? this->m_slots[index] : WOWGUID();
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

            if (item && !(item->m_modelFlags & 0x10000) && ((flags & BAG_SEARCH_IGNORE_ITEM_STATE) || (((item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) || !item->m_item->m_itemFieldMaxdurability || item->m_item->m_itemFieldDurability) && !(item->m_item->m_itemFieldFlags & 0x10))) && (!(flags & BAG_SEARCH_REQUIRE_CHARGES) || (item->GetBag() && !(flags & BAG_SEARCH_NO_RECURSE)) || (item->GetEnchantmentSpell(0) && item->GetCurrentSpellCharges()))) {
                if (callback(item, param)) {
                    container = this->m_guid;
                    slot = index;
                } else if (item->GetBag() && !(flags & BAG_SEARCH_NO_RECURSE)) {
                    item = item->GetBag()->FindItem(callback, param, container, slot, flags);
                } else {
                    item = nullptr;
                }

                if (item) {
                    if (!(flags & BAG_SEARCH_SMALLEST_STACK)) {
                        return item;
                    }

                    if (!best || item->m_item->m_itemFieldStackCount < best->m_item->m_itemFieldStackCount) {
                        best = item;
                    }
                }
            }
        }

        numSlots = this->m_numSlots;

        if (index >= numSlots) {
            index = numSlots;
        } else if (this->m_hasBank) {
            uint32_t range = 0;

            while (s_bagSlotRanges[range].m_last != index) {
                if (++range >= 8) {
                    break;
                }
            }

            if (range >= 8) {
                index++;
            } else if (range >= 7) {
                index = numSlots;
            } else {
                index = s_bagSlotRanges[range + 1].m_first;
            }
        } else {
            index++;
        }

        if (index >= numSlots) {
            return best;
        }
    }
}

// VERIFY: 82130edd auto
// OFFSET: 0x00754590
int32_t GetItemLimitCategoryCallback(const CGItem_C* item, void* param) {
    int32_t* data = static_cast<int32_t*>(param);

    if (item && item->GetStats() && static_cast<int32_t>(item->GetStats()->m_itemLimitCategory) == data[0]) {
        data[1] += item->m_item->m_itemFieldStackCount;
    }

    return 0;
}

// VERIFY: 2670f6b3 auto
// OFFSET: 0x00754DD0
bool CGBag_C::ItemCountValid(int32_t itemId, const ItemStats_C* stats, int32_t count, bool showError) {
    if (stats->m_maxCount) {
        int32_t current;

        if (itemId == -2) {
            current = this->GetNumFreeInvSlots();
        } else {
            int32_t data[2] = { itemId, 0 };
            WOWGUID container;
            uint32_t slot;
            this->FindItem(GetItemTypeCountCallback, data, container, slot, 79);
            current = data[1];
        }

        if (count + current > static_cast<int32_t>(stats->m_maxCount)) {
            if (showError) {
                CGGameUI::DisplayError(ERR_ITEM_MAX_COUNT);
            }

            return false;
        }
    }

    if (stats->m_itemLimitCategory) {
        ItemLimitCategoryRec* category = g_itemLimitCategoryDB.GetRecord(stats->m_itemLimitCategory);
        int32_t data[2] = { static_cast<int32_t>(stats->m_itemLimitCategory), 0 };
        WOWGUID container;
        uint32_t slot;
        this->FindItem(GetItemLimitCategoryCallback, data, container, slot, 79);

        if (count + data[1] > category->m_quantity) {
            if (showError) {
                CGGameUI::DisplayError(ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS, category->m_quantity, category->m_name);
            }

            return false;
        }
    }

    return true;
}

// VERIFY: b3aecff9 auto
// OFFSET: 0x00754D90
void CGBag_C::UnlockItemsOfType(int32_t itemId, uint32_t flags) {
    ItemTypeCountParam param;
    param.m_itemId = itemId;
    param.m_count = 0;
    WOWGUID container;
    uint32_t slot;
    this->FindItem(UnlockItemTypeCallback, &param, container, slot, flags);
}

// VERIFY: 095e35cd auto
// OFFSET: 0x00754AC0
CGItem_C* CGBag_C::FindItemOfBagFamily(uint32_t bagFamily, uint32_t flags) {
    WOWGUID container;
    uint32_t slot;
    return this->FindItem(FindBagFamilyCallback, &bagFamily, container, slot, flags);
}
