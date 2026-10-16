#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_ITEM_C_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

class CClientObjCreate;
class CImVector;
class ItemStats_C;
class ItemGroupSoundsRec;
class LockRec;

struct INVENTORYART : public TSHashObject<INVENTORYART, HASHKEY_NONE> {
    /* 0018 */ char* m_texture;
};

struct ITEMBYNAME : public TSHashObject<ITEMBYNAME, HASHKEY_UTF8I> {
    /* 0018 */ int32_t m_itemId;
};

int32_t GetItemIDByName(const char* name);

extern const uint32_t s_inventoryTypeSlots[29];

class CGItem_C : public CGObject_C, public CGItem {
    public:
    static void Initialize();
    static void Shutdown();
    struct ItemPurchaseData {
        /* 0000 */ uint32_t m_money;
        /* 0004 */ uint32_t m_honorPoints;
        /* 0008 */ uint32_t m_arenaPoints;
        /* 000C */ uint32_t m_itemId [5];
        /* 0020 */ uint32_t m_itemCount [5];
        /* 0034 */ uint32_t unk_0034;
        /* 0038 */ uint32_t m_refundTimeLeft;
    };

    static char* BuildItemName(char* buffer, uint32_t size, int32_t itemId, int32_t randomPropertiesId);
    static void GetCooldowns(int32_t itemId, uint32_t& cooldown, uint32_t& categoryCooldown, uint32_t& category);
    static int32_t IsMetal(int32_t materialId);
    static int32_t NumBaseCharges(int32_t itemId);
    static void RemoveNameFromCache(const char* name);
    static const char* GetInventoryArt(int32_t displayId);
    const char* GetInventoryArt() const;

    /* 00DC */ TSList<void> m_itemMirrorLists [58];
    /* 0394 */ uint32_t m_itemFlags;
    /* 0398 */ CGUnitVirtualItem m_virtualItem;
    /* 03A0 */ uint32_t m_expirationTime;
    /* 03A4 */ uint32_t m_enchantmentExpiration [12];
    /* 03D4 */ ItemPurchaseData* m_itemPurchaseData;
    /* 03D8 */ const ItemGroupSoundsRec* m_groupSounds;
    /* 03DC */ uint8_t unused_03DC [4];

    ~CGItem_C() override;
    void PostReenable() override;
    int32_t GetSelectionHighlightColor(CImVector* color) const override;
    void OnRightClick() override;
    const char* GetObjectName() const override;
    int32_t GetPageTextID(void (*callback)(int32_t, const WOWGUID&, void*, bool)) const override;
    uint32_t GetItemStaticFlags(int32_t index) override;
    void Disable() override;

    CGItem_C(uint32_t time, const CClientObjCreate& create);
    uint32_t GetBagFamily() const;
    static int32_t CanGoInSlot(int32_t itemId, uint32_t slot, int32_t bypass);
    int32_t CanGoInSlot(uint32_t slot, int32_t bypass);
    uint32_t GetClassID() const;
    int32_t GetCurrentSpellCharges();
    uint32_t GetDisplayID() const;
    uint32_t GetEnchantmentTimeLeft(int32_t slot) const;
    int32_t UseItem(WOWGUID* target, int32_t confirmed);
    static WOWGUID s_bindOnUseTarget;
    static WOWGUID s_bindOnUseItem;
    static void ConfirmBindOnUse();
    int32_t CanBeUsed();
    uint32_t GetExpirationTimeLeft() const;
    char* GetFullItemName(char* buffer, uint32_t size) const;
    uint32_t GetInventoryType() const;
    static char* GetStatName(uint32_t stat, char* buffer, int32_t size);
    int32_t GetNumSockets();
    int32_t GetOnUseSlot() const;
    int32_t GetPrismaticSocketCount();
    int32_t GetRandomItemSlot() const;
    int32_t GetRandomPropertyPoints() const;
    uint32_t GetSheatheType() const;
    uint32_t GetSpellItemEnchantmentId() const;
    ItemStats_C* GetStats() const;
    uint32_t GetSubtypeID() const;
    int32_t GetUsableSpellCharges() const;
    bool HasEnchantments();
    int32_t HasOnUseEnchant() const;
    bool HasSoulboundEnchant();
    int32_t HasUseEffect();
    bool IsBound();
    void Unlock();
    void Lock();
    static void SignalEvent(CGItem_C* item, uint32_t event);
    static int32_t GetUseSpell(int32_t itemId);
    static int32_t GetRandomPropertyValue(int32_t slot, int32_t randomPropertiesId, int32_t points);
    static int32_t IsLocked(const LockRec* lock, int32_t itemLevel, int32_t* spellId, int32_t* skillValue, int32_t* requiredSkill, int32_t* lockIndex, CGItem_C** key, int32_t* entry);
    int32_t GetRandomPropertyValue(int32_t slot);
    int32_t GetRepairCost();
    int32_t GetEnchantmentSpell(int32_t a1);
    void GetSpellCooldownInfo(uint32_t& cooldown, uint32_t& categoryCooldown, uint32_t& category);
    bool IsBoundForTrade();
    bool IsBoundToAccount();
    bool IsInstanceBound() const;
    bool IsPermanentlyBoundForTrade();
    bool IsZoneBound() const;
    void PostInit(uint32_t time, const CClientObjCreate& create, bool a3);
    void PostInitWithStats();
    void RequestItemCacheInfo();
    void RequestItemPurchaseData();
    void SetItemPurchaseData(ItemPurchaseData* data);
    void SetStorage(uint32_t* storage, uint32_t* saved);
    void SetTranslated();
    void UpdateEnchantments();
    int32_t GetItemEnchantmentVisualID() const;
    void UpdateEnchantmentTime(int32_t slot, int32_t seconds);
    void UpdateExpirationTime(int32_t seconds);
    void UpdateSockets(uint32_t gem1, uint32_t gem2, uint32_t gem3, uint32_t socketBonus);
};

#endif
