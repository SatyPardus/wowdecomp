#ifndef WOW_SOURCE_UI_LOOTFRAME_H
#define WOW_SOURCE_UI_LOOTFRAME_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Ui/Types.h"

struct lua_State;
class CGObject_C;

struct LootInfo {
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint32_t unused_0018;
    /* 001C */ uint32_t unused_001C;
};

void LootDestroy();
void LootInitialize();

struct LootSlot {
    /* 0000 */ int32_t m_pending;
    /* 0004 */ int32_t m_itemID;
    /* 0008 */ int32_t m_displayId;
    /* 000C */ int32_t m_count;
    /* 0010 */ int32_t m_randomSuffix;
    /* 0014 */ int32_t m_randomPropertyId;
    /* 0018 */ uint8_t m_slot;
    /* 0019 */ uint8_t unused_0019[3];
    /* 001C */ int32_t m_slotType;
};

class CGLootInfo {
    public:
    static uint32_t m_itemsPending;
    static LOOT_ACQUIRE m_lootAcquire;
    static LootSlot m_slots[18];
    static int32_t m_coins;
    static WOWGUID m_lootObj;
    static int32_t m_autolooted;

    static void InitializeGame();
    static void ShutdownGame();
    static void LeaveWorld();
    static void DoAutoLoot(int32_t a1);
    static int32_t HasLoot();
    static void LootSlot(uint32_t slot);
    static void ConfirmLootSlot(uint32_t slot);
    static void GiveMasterLoot(uint32_t slot, WOWGUID target);
    static void LootButtonItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
    static void SetObject(CGObject_C* object, int32_t coins, LOOT_ACQUIRE lootAcquire);
    static void ClearSlot(uint8_t slot);
    static void OnLootSlotChanged(uint8_t slot);
    static int32_t GetLootItem(uint32_t slot);
    static int32_t GetLootItemCount(uint32_t slot);
    static int32_t GetLootItemRandomSuffix(uint32_t slot);
    static int32_t GetLootItemRandomPropertyId(uint32_t slot);
    static const char* GetLootSlotTexture(uint32_t slot);
    static void GetLootSlotText(char* buffer, uint32_t size, uint32_t slot);
    static char* GetLootSlotLink(uint32_t slot, char* buffer, uint32_t size);
};

class CGLootWindow {
    public:
    static int32_t GetNumLootItems();
    static int32_t GetLootItemRarity(uint32_t slot);
};

class CGLootDisplay {
    public:
    static LOOT_ACQUIRE GetLootAcquireType();
};

void LootInfoRegisterScriptFunctions();
void LootInfoUnregisterScriptFunctions();

void CurrencyBreakdown(int32_t amount, int32_t* parts);
void GetCurrencyTextureString(int32_t amount, char* buffer, uint32_t size, int32_t height);
void GetCurrencyText(int32_t amount, char* buffer, uint32_t size, const char* separator);
void GetCurrencyShortText(int32_t amount, char* buffer, uint32_t size, const char* separator);
void GetCurrencyIcon(int32_t amount, char* buffer, uint32_t size);

#endif
