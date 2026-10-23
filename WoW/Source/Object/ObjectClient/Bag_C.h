#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_BAG_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_BAG_C_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/ObjectClient/Object_C.h"

enum BAG_SEARCH_FLAGS : uint32_t {
    BAG_SEARCH_EQUIPMENT = 1,
    BAG_SEARCH_BAG_SLOTS = 2,
    BAG_SEARCH_BACKPACK = 4,
    BAG_SEARCH_BANK = 8,
    BAG_SEARCH_NO_RECURSE = 16,
    BAG_SEARCH_REQUIRE_CHARGES = 32,
    BAG_SEARCH_KEYRING = 64,
    BAG_SEARCH_IGNORE_ITEM_STATE = 128,
    BAG_SEARCH_SMALLEST_STACK = 256,
    BAG_SEARCH_CURRENCY = 512,
    inventoryFlags = BAG_SEARCH_EQUIPMENT | BAG_SEARCH_BAG_SLOTS | BAG_SEARCH_BACKPACK,
    BAG_SEARCH_DEFAULT_LOCATIONS = inventoryFlags | BAG_SEARCH_KEYRING | BAG_SEARCH_CURRENCY,
    BAG_SEARCH_TOTEM_LOCATIONS = inventoryFlags,
};

enum BAG_RESULT : int32_t {
    BAG_OK = 0,
    BAG_CANT_EQUIP_LEVEL_I = 1,
    BAG_CANT_EQUIP_SKILL = 2,
    BAG_WRONG_SLOT = 3,
    BAG_FULL = 4,
    BAG_IN_BAG = 5,
    BAG_TRADE_EQUIPPED_BAG = 6,
    BAG_AMMO_ONLY = 7,
    BAG_PROFICIENCY_NEEDED = 8,
    BAG_NO_SLOT_AVAILABLE = 9,
    BAG_CANT_EQUIP_EVER = 10,
    BAG_CANT_EQUIP_EVER_2 = 11,
    BAG_NO_SLOT_AVAILABLE_2 = 12,
    BAG_2HANDED_EQUIPPED = 13,
    BAG_2HSKILLNOTFOUND = 14,
    BAG_WRONG_BAG_TYPE = 15,
    BAG_WRONG_BAG_TYPE_2 = 16,
    BAG_ITEM_MAX_COUNT = 17,
    BAG_NO_SLOT_AVAILABLE_3 = 18,
    BAG_CANT_STACK = 19,
    BAG_NOT_EQUIPPABLE = 20,
    BAG_CANT_SWAP = 21,
    BAG_SLOT_EMPTY = 22,
    BAG_ITEM_NOT_FOUND = 23,
    BAG_DROP_BOUND_ITEM = 24,
    BAG_OUT_OF_RANGE = 25,
    BAG_TOO_FEW_TO_SPLIT = 26,
    BAG_SPLIT_FAILED = 27,
    BAG_SPELL_FAILED_REAGENTS_GENERIC = 28,
    BAG_NOT_ENOUGH_MONEY = 29,
    BAG_NOT_A_BAG = 30,
    BAG_DESTROY_NONEMPTY_BAG = 31,
    BAG_NOT_OWNER = 32,
    BAG_ONLY_ONE_QUIVER = 33,
    BAG_NO_BANK_SLOT = 34,
    BAG_NO_BANK_HERE = 35,
    BAG_ITEM_LOCKED = 36,
    BAG_GENERIC_STUNNED = 37,
    BAG_PLAYER_DEAD = 38,
    BAG_CLIENT_LOCKED_OUT = 39,
    BAG_INTERNAL_BAG_ERROR = 40,
    BAG_ONLY_ONE_BOLT = 41,
    BAG_ONLY_ONE_AMMO = 42,
    BAG_CANT_WRAP_STACKABLE = 43,
    BAG_CANT_WRAP_EQUIPPED = 44,
    BAG_CANT_WRAP_WRAPPED = 45,
    BAG_CANT_WRAP_BOUND = 46,
    BAG_CANT_WRAP_UNIQUE = 47,
    BAG_CANT_WRAP_BAGS = 48,
    BAG_LOOT_GONE = 49,
    BAG_INV_FULL = 50,
    BAG_BANK_FULL = 51,
    BAG_VENDOR_SOLD_OUT = 52,
    BAG_FULL_2 = 53,
    BAG_ITEM_NOT_FOUND_2 = 54,
    BAG_CANT_STACK_2 = 55,
    BAG_FULL_3 = 56,
    BAG_VENDOR_SOLD_OUT_2 = 57,
    BAG_OBJECT_IS_BUSY = 58,
    BAG_CANT_BE_DISENCHANTED = 59,
    BAG_NOT_IN_COMBAT = 60,
    BAG_NOT_WHILE_DISARMED = 61,
    BAG_FULL_4 = 62,
    BAG_CANT_EQUIP_RANK = 63,
    BAG_CANT_EQUIP_REPUTATION = 64,
    BAG_TOO_MANY_SPECIAL_BAGS = 65,
    BAG_LOOT_CANT_LOOT_THAT_NOW = 66,
    BAG_ITEM_UNIQUE_EQUIPPABLE = 67,
    BAG_VENDOR_MISSING_TURNINS = 68,
    BAG_NOT_ENOUGH_HONOR_POINTS = 69,
    BAG_NOT_ENOUGH_ARENA_POINTS = 70,
    BAG_ITEM_MAX_COUNT_SOCKETED = 71,
    BAG_MAIL_BOUND_ITEM = 72,
    BAG_INTERNAL_BAG_ERROR_2 = 73,
    BAG_FULL_5 = 74,
    BAG_ITEM_MAX_COUNT_EQUIPPED_SOCKETED = 75,
    BAG_ITEM_UNIQUE_EQUIPPABLE_SOCKETED = 76,
    BAG_TOO_MUCH_GOLD = 77,
    BAG_NOT_DURING_ARENA_MATCH = 78,
    BAG_TRADE_BOUND_ITEM = 79,
    BAG_CANT_EQUIP_RATING = 80,
    BAG_AUTOEQUIP_BIND_CONFIRM = 81,
    BAG_NOT_SAME_ACCOUNT = 82,
    BAG_NO_DISPLAY_ERROR = 83,
    BAG_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS = 84,
    BAG_ITEM_MAX_LIMIT_CATEGORY_SOCKETED_EXCEEDED_IS = 85,
    BAG_SCALING_STAT_ITEM_LEVEL_EXCEEDED = 86,
    BAG_PURCHASE_LEVEL_TOO_LOW = 87,
    BAG_CANT_EQUIP_NEED_TALENT = 88,
    BAG_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS = 89,
    BAG_SHAPESHIFT_FORM_CANNOT_EQUIP = 90,
    BAG_ITEM_INVENTORY_FULL_SATCHEL = 91,
};

class CGItem_C;

struct BAGSLOTRANGE {
    /* 0000 */ uint32_t m_first;
    /* 0004 */ uint32_t m_last;
};

struct ItemTypeCountParam {
    /* 0000 */ int32_t m_itemId;
    /* 0004 */ int32_t m_count;
};

class ItemStats_C;

class CGBag_C {
    public:
    /* 0000 */ int32_t m_numSlots;
    /* 0004 */ WOWGUID* m_slots;
    /* 0008 */ WOWGUID m_guid;
    /* 0010 */ uint8_t m_hasBank;

    int32_t FindItemSlot(const CGItem_C* item) const;
    int32_t FindItemSlot(WOWGUID guid) const;
    void UnlockItemsOfType(int32_t itemId, uint32_t flags);
    CGItem_C* FindItemOfBagFamily(uint32_t bagFamily, uint32_t flags);
    uint32_t GetAvgEquippedDurability() const;
    const CGItem_C* GetItemPointer(int32_t slot) const;
    int32_t GetNumFreeInvSlots();
    int32_t GetNumItems() const;
    uint64_t GetItemGuid(uint32_t slot) const;
    CGItem_C* FindItemOfType(int32_t itemId, uint32_t flags);
    CGItem_C* FindItemOfClass(int32_t classId, uint32_t subclassMask, uint32_t flags);
    CGItem_C* FindItem(int32_t (*callback)(const CGItem_C* item, void* param), void* param, uint32_t flags);
    CGItem_C* FindItemByName(const char* name, uint32_t flags);
    CGItem_C* FindItemByFullName(const char* name, uint32_t flags);
    CGItem_C* GetItemIdByName(const char* name, WOWGUID& container, uint32_t& slot, uint32_t flags);
    CGItem_C* GetItemPointer(int32_t slot);

    bool ContainsItem(WOWGUID guid, bool recurse, uint32_t flags) const;
    bool FindTotemCategory(int32_t category, uint32_t flags);
    bool FindTotemCategory(int32_t type, int32_t& mask, bool recurse, uint32_t flags);
    CGItem_C* FindItem(int32_t (*callback)(const CGItem_C* item, void* param), void* param, WOWGUID& container, uint32_t& slot, uint32_t flags);
    CGItem_C* FindItemOfType(int32_t itemId, WOWGUID& container, uint32_t& slot, uint32_t flags);
    int32_t GetItemTypeCount(int32_t itemId, uint32_t flags);
    int32_t GetItemTypeCharges(int32_t itemId, uint32_t flags);
    bool ItemCountValid(int32_t itemId, const ItemStats_C* stats, int32_t count, bool showError);
};

int32_t GetItemTypeCountCallback(const CGItem_C* item, void* param);
int32_t GetItemTypeChargesCallback(const CGItem_C* item, void* param);
int32_t FindItemIDCallback(const CGItem_C* item, void* param);
int32_t FindItemClassCallback(const CGItem_C* item, void* param);
int32_t GetItemLimitCategoryCallback(const CGItem_C* item, void* param);
int32_t GetDisplayErrorByInventoryError(int32_t error);

#endif
