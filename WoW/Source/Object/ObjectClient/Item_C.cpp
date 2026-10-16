#include "Item_C.h"
#include "WoW/Source/DB/Rec/ItemVisualsRec.h"
#include "WoW/Source/Ui/ItemSocketInfo.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/Game/ViolenceLevels.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ItemRandomPropertiesRec.h"
#include "WoW/Source/DB/Rec/ItemRandomSuffixRec.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/MaterialRec.h"
#include "WoW/Source/DB/Rec/RandPropPointsRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/DB/Rec/DurabilityCostsRec.h"
#include "WoW/Source/DB/Rec/DurabilityQualityRec.h"
#include "WoW/Source/DB/Rec/LockRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/CurrencyTypes.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstring>
#include <ctime>
#include "WoW/Source/Ui/ItemTextFrame.h"
#include "WoW/Source/DB/Rec/ItemGroupSoundsRec.h"
#include "WoW/Source/Object/Item.h"

// STATIC: 0x00A2D288
const uint32_t s_inventoryTypeSlots[29] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 3072, 12288, 98304, 65536, 131072, 16384, 98304, 7864320, 262144, 16, 98304, 98304, 65536, 0, 131072, 131072, 0, 131072 };

// STATIC: 0x00CA1140
static TSHashTable<INVENTORYART, HASHKEY_NONE> s_inventoryArt;

// STATIC: 0x00CA1168
static TSHashTable<ITEMBYNAME, HASHKEY_UTF8I> s_itemByName;

// VERIFY: 1390e3e9 manual
// OFFSET: 0x00707720
static int32_t OnUpdateStackCount(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item->m_item->m_itemFieldOwner == ClntObjMgrGetActivePlayer()) {
        WOWGUID requester = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, requester, nullptr, nullptr, false);

        if (stats && static_cast<int32_t>(stats->m_stackable) > 1) {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

            if (player) {
                player->m_bag.UnlockItemsOfType(item->m_obj->m_entryID, 8);
            }
        } else {
            CGGameUI::UnlockItem(item->m_obj->m_guid);
        }

        CGTradeSkillInfo::RefreshList(0);
        CGActionBar::UpdateItem(item->m_obj->m_entryID);
        CGTradeInfo::UpdatePlayerItem(item->m_obj->m_guid);
        CGContainerInfo::UpdateItem(item->m_obj->m_guid);
        CGCharacterInfo::UpdateItem(item->m_obj->m_guid);
        CGQuestLog::UpdateQuestItem(item->m_obj->m_entryID);

        if (stats && stats->m_class == 10) {
            CGCurrencyTypes::EnterWorld();
        }

        ItemRec* rec = g_itemDB.GetRecord(item->m_obj->m_entryID);

        if (rec && rec->m_inventoryType == 24) {
            CGCharacterInfo::UpdateInventoryAlerts();
        }
    }

    return 1;
}

// VERIFY: 8b8a4f8e manual
// OFFSET: 0x00707880
static int32_t OnUpdateSpellCharges(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item->m_item->m_itemFieldOwner == ClntObjMgrGetActivePlayer()) {
        CGActionBar::UpdateItem(item->m_obj->m_entryID);
        CGContainerInfo::UpdateCooldowns();
    }

    return 1;
}

// VERIFY: ca82e43c manual
// OFFSET: 0x007078E0
static void ItemIDChangedCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (player) {
            int32_t slot = player->m_bag.FindItemSlot(guid);

            if (slot == 15 || slot == 16) {
                CGActionBar::UpdateAttackAction();
            } else if (slot == 17) {
                CGActionBar::UpdateRangedAttackAction();
            }
        }

        CGActionBar::UpdateItem(id);
        CGContainerInfo::UpdateItem(guid);
        CGCharacterInfo::UpdateItem(guid);
    }
}

// VERIFY: d710e893 manual
// OFFSET: 0x00707960
static int32_t OnUpdateItemID(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item && item->m_item->m_itemFieldOwner == ClntObjMgrGetActivePlayer()) {
        int32_t oldItemId = *static_cast<const int32_t*>(oldValue);
        CGActionBar::UpdateItem(oldItemId);
        g_itemDBCache.Invalidate(oldItemId);

        WOWGUID itemGuid = item->m_obj->m_guid;

        if (g_itemDBCache.GetRecord(item->m_obj->m_entryID, itemGuid, ItemIDChangedCacheCallback, nullptr, false)) {
            itemGuid = item->m_obj->m_guid;
            ItemIDChangedCacheCallback(item->m_obj->m_entryID, itemGuid, nullptr, true);
        }

        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
    }

    return 1;
}

// VERIFY: 8b4c986d manual
// OFFSET: 0x00707A30
static int32_t OnUpdateDynamicFlags(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item && ((*static_cast<const uint8_t*>(oldValue) ^ item->m_item->m_itemFieldFlags) & 0x10)) {
        CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(item->m_item->m_itemFieldOwner, TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (owner && owner->m_obj->m_guid == ClntObjMgrGetActivePlayer() && static_cast<uint32_t>(owner->GetBag()->FindItemSlot(item->m_obj->m_guid)) <= 18) {
            CGCharacterInfo::UpdateInventoryAlerts();
        }
    }

    return 1;
}

// VERIFY: 0a592725 manual
// OFFSET: 0x00707AE0
static int32_t OnUpdateDurability(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item) {
        CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(item->m_item->m_itemFieldOwner, TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (owner) {
            int32_t oldDurability = *static_cast<const int32_t*>(oldValue);

            if ((item->m_item->m_itemFieldFlags & 0x8) || !item->m_item->m_itemFieldDurability) {
                int32_t invSlot = -1;
                int32_t slot = owner->GetBag()->FindItemSlot(item->m_obj->m_guid);

                if (slot == 15) {
                    invSlot = 15;
                } else if (slot == 16) {
                    invSlot = 16;
                } else if (slot == 17) {
                    invSlot = (item->m_virtualItem.m_inventoryType != 26) + 15;
                }

                if (oldDurability > 0) {
                    CCharacterComponent::RemoveHandItem(owner->m_worldModel, static_cast<INVENTORY_SLOTS>(invSlot), static_cast<SHEATHE_TYPE>(item->m_virtualItem.m_sheatheType), item->m_virtualItem.m_inventoryType == 14);

                    WOWGUID ownerGuid = owner->m_obj->m_guid;
                    CGGameUI::UnitModelUpdate(ownerGuid, 3);
                }
            } else if (!(item->m_item->m_itemFieldFlags & 0x8) && item->m_item->m_itemFieldDurability && !oldDurability) {
                owner->EquipItem(item);

                WOWGUID ownerGuid = owner->m_obj->m_guid;
                CGGameUI::UnitModelUpdate(ownerGuid, 3);
            }
        }
    }

    CGCharacterInfo::UpdateInventoryAlerts();

    return 1;
}

// VERIFY: fa692b57 manual
// OFFSET: 0x00708BD0
static int32_t OnUpdateEnchantments(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item) {
        item->UpdateEnchantments();
    }

    return 1;
}

// VERIFY: d37d5a1a manual
// OFFSET: 0x007093F0
void CGItem_C::ConfirmBindOnUse() {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGItem_C::s_bindOnUseItem, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item) {
        item->UseItem(&CGItem_C::s_bindOnUseTarget, 1);
    }

    CGItem_C::s_bindOnUseItem = WOWGUID();
}

// VERIFY: 6840b2f7 auto
// OFFSET: 0x00709440
void CGItem_C::Initialize() {
    ClntObjMgrSetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldStackCount), sizeof(uint32_t), OnUpdateStackCount, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldSpellCharges), sizeof(CGItemData::m_itemFieldSpellCharges), OnUpdateSpellCharges, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldEnchantment), sizeof(CGItemData::m_itemFieldEnchantment), OnUpdateEnchantments, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_OBJECT, offsetof(CGObjectData, m_entryID), sizeof(uint32_t), OnUpdateItemID, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldFlags), sizeof(uint16_t), OnUpdateDynamicFlags, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldDurability), sizeof(uint32_t), OnUpdateDurability, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
}

// VERIFY: 90fb2c28 auto
// OFFSET: 0x00709A10
void CGItem_C::Shutdown() {
    ClntObjMgrUnsetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldStackCount), OnUpdateStackCount, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldSpellCharges), OnUpdateSpellCharges, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldEnchantment), OnUpdateEnchantments, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_OBJECT, offsetof(CGObjectData, m_entryID), OnUpdateItemID, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldFlags), OnUpdateDynamicFlags, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_ITEM, offsetof(CGItemData, m_itemFieldDurability), OnUpdateDurability, nullptr);
    s_inventoryArt.Clear();
    s_itemByName.Clear();
}

// VERIFY: 546a5abe manual
// OFFSET: 0x0070AE90
static void LoadItemCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!item || !success) {
        return;
    }

    item->PostInitWithStats();

    CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(item->m_item->m_itemFieldOwner, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (owner) {
        owner->m_pendingItemCacheCount--;
    }
}

// VERIFY: 9264aaff manual
// OFFSET: 0x00709840
CGItem_C::CGItem_C(uint32_t time, const CClientObjCreate& create) : CGObject_C(time, create) {
    this->m_itemFlags = 0;
    this->m_expirationTime = 0;
    this->m_groupSounds = nullptr;

    ClntObjMgrHideObject(this->m_obj->m_guid);

    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_classID = rec ? rec->m_classID : 0;

    rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_subclassID = rec ? rec->m_subclassID : 0;

    rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_material = rec ? rec->m_material : 0;

    rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_inventoryType = rec ? rec->m_inventoryType : 0;

    rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_sheatheType = rec ? rec->m_sheatheType : 0;

    rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_soundOverrideSubclassID = rec ? rec->m_soundOverrideSubclassID : -1;

    memset(this->m_enchantmentExpiration, 0, sizeof(this->m_enchantmentExpiration));
    this->m_itemPurchaseData = nullptr;
}

// VERIFY: 6bc496ea manual
// OFFSET: [0x0070B120, 0x0070B1F0]
CGItem_C::~CGItem_C() {
    if (!(this->m_itemFlags & 0x2)) {
        g_itemDBCache.CancelCallback(this->m_obj->m_entryID, &LoadItemCacheCallback, this);
    }

    if (this->m_itemPurchaseData) {
        delete this->m_itemPurchaseData;
        this->m_itemPurchaseData = nullptr;
    }
}

// VERIFY: e95d6600 manual
// OFFSET: 0x0070AF00
void CGItem_C::RequestItemCacheInfo() {
    WOWGUID guid = this->m_obj->m_guid;

    if (g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, &LoadItemCacheCallback, this, false)) {
        this->PostInitWithStats();
        return;
    }

    CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(this->m_item->m_itemFieldOwner, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (owner) {
        owner->m_pendingItemCacheCount++;
    }
}

// OFFSET: 0x0070AFB0
void CGItem_C::PostReenable() {
    this->RequestItemCacheInfo();
}

// VERIFY: fffdf723 auto
// OFFSET: 0x0070AF80
void CGItem_C::PostInit(uint32_t time, const CClientObjCreate& create, bool a3) {
    this->CGObject_C::PostInit(time, create, a3);
    this->RequestItemCacheInfo();
}

// VERIFY: 5b9cdc6d manual
// OFFSET: 0x00706CF0
void CGItem_C::SetItemPurchaseData(ItemPurchaseData* data) {
    if (this->m_itemPurchaseData) {
        delete this->m_itemPurchaseData;
    }

    this->m_itemPurchaseData = data;

    if (data) {
        this->m_itemFlags &= ~0x4;
    }
}

// VERIFY: 25290e05 auto
// OFFSET: 0x00706EC0
uint32_t CGItem_C::GetSpellItemEnchantmentId() const {
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t id = this->m_item->m_itemFieldEnchantment[i].m_id;

        if (!id) {
            continue;
        }

        SpellItemEnchantmentRec* rec = g_spellItemEnchantmentDB.GetRecord(id);

        if (!rec) {
            continue;
        }

        for (uint32_t j = 0; j < 3; j++) {
            if (rec->m_effectArg[j] && rec->m_effect[j] == 7) {
                return this->m_item->m_itemFieldEnchantment[i].m_id;
            }
        }
    }

    return 0;
}

// VERIFY: 414ca99f auto
// OFFSET: 0x00706FA0
int32_t CGItem_C::IsMetal(int32_t materialId) {
    MaterialRec* rec = g_materialDB.GetRecord(materialId);

    if (rec && (rec->m_flags & 0x1)) {
        return 1;
    }

    return 0;
}

// VERIFY: 01abc0a5 auto
// OFFSET: 0x00707060
void CGItem_C::SetTranslated() {
    this->m_item->m_itemFieldFlags |= 0x2;
}

// VERIFY: e174df88 auto
// OFFSET: 0x00707070
void CGItem_C::UpdateExpirationTime(int32_t seconds) {
    if (seconds > 0) {
        this->m_expirationTime = seconds + static_cast<uint32_t>(time(nullptr));
    } else {
        this->m_expirationTime = 0;
    }
}

// VERIFY: d4c997df manual
// OFFSET: 0x007070B0
uint32_t CGItem_C::GetExpirationTimeLeft() const {
    if (!this->m_expirationTime) {
        return 0;
    }

    int32_t left = this->m_expirationTime - static_cast<uint32_t>(time(nullptr));

    return left > 0 ? left : 0;
}

// VERIFY: dc762275 auto
// OFFSET: 0x007070E0
void CGItem_C::UpdateEnchantmentTime(int32_t slot, int32_t seconds) {
    if (seconds > 0) {
        this->m_enchantmentExpiration[slot] = FrameTime::s_curTimeMs + 1000 * seconds;
    } else {
        this->m_enchantmentExpiration[slot] = 0;
    }
}

// VERIFY: 3cedd503 auto
// OFFSET: 0x00707120
uint32_t CGItem_C::GetEnchantmentTimeLeft(int32_t slot) const {
    uint32_t expiration = this->m_enchantmentExpiration[slot];

    if (expiration && static_cast<int32_t>(FrameTime::s_curTimeMs - expiration) < 0) {
        return expiration - FrameTime::s_curTimeMs;
    }

    return 0;
}

// VERIFY: 947c950c manual
// OFFSET: 0x00707150
int32_t CGItem_C::GetSelectionHighlightColor(CImVector* color) const {
    CImVector white;
    white.b = 0xFF;
    white.g = 0xFF;
    white.r = 0xFF;
    white.a = 0xFF;
    *color = white;

    return 1;
}

// VERIFY: 3f2f878c auto
// OFFSET: 0x00707220
uint32_t CGItem_C::GetClassID() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);

    return rec ? rec->m_classID : 0;
}

// VERIFY: 955a9609 auto
// OFFSET: 0x00707250
uint32_t CGItem_C::GetSubtypeID() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);

    return rec ? rec->m_subclassID : 0;
}

// VERIFY: 6c114c7d auto
// OFFSET: 0x00707280
uint32_t CGItem_C::GetInventoryType() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);

    return rec ? rec->m_inventoryType : 0;
}

// VERIFY: b59c1299 auto
// OFFSET: 0x00707300
uint32_t CGItem_C::GetDisplayID() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);

    return rec ? rec->m_displayInfoID : 0;
}

// VERIFY: 8c746859 auto
// OFFSET: 0x00707330
uint32_t CGItem_C::GetSheatheType() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);

    return rec ? rec->m_sheatheType : 0;
}

// INLINED
bool CGItem_C::IsBoundToAccount() {
    return (this->GetItemStaticFlags(0) >> 13) & 1;
}

// VERIFY: 6b7047e1 manual
// OFFSET: 0x007073E0
bool CGItem_C::HasSoulboundEnchant() {
    if (this->IsBoundToAccount()) {
        return false;
    }

    for (uint32_t i = 0; i < 12; i++) {
        uint32_t id = this->m_item->m_itemFieldEnchantment[i].m_id;

        if (!id) {
            continue;
        }

        SpellItemEnchantmentRec* rec = g_spellItemEnchantmentDB.GetRecord(id);

        if (rec && (rec->m_flags & 0x1)) {
            return true;
        }
    }

    return false;
}

// VERIFY: a5c81637 auto
// OFFSET: 0x00707500
int32_t CGItem_C::GetRandomItemSlot() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    uint32_t inventoryType = rec ? rec->m_inventoryType : 0;

    switch (inventoryType) {
        case 0:
        case 18:
        case 19:
        case 24:
        case 27:
        case 28:
            return -1;

        case 1:
        case 4:
        case 5:
        case 7:
        case 17:
        case 20:
            return 0;

        case 2:
        case 9:
        case 11:
        case 14:
        case 16:
        case 23:
            return 2;

        case 3:
        case 6:
        case 8:
        case 10:
        case 12:
            return 1;

        case 13:
        case 21:
        case 22:
            return 3;

        case 15:
        case 25:
        case 26:
            return 4;

        default:
            return -1;
    }
}

// VERIFY: 7676d8ac manual
// OFFSET: 0x00708140
int32_t CGItem_C::GetPrismaticSocketCount() {
    if (this->IsBoundToAccount()) {
        return 0;
    }

    uint32_t id = this->m_item->m_itemFieldEnchantment[6].m_id;

    if (!id) {
        return 0;
    }

    SpellItemEnchantmentRec* rec = g_spellItemEnchantmentDB.GetRecord(id);

    if (!rec) {
        return 0;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (rec->m_effect[i] == 8) {
            return rec->m_effectPointsMin[i];
        }
    }

    return 0;
}

// VERIFY: 74178378 manual
// OFFSET: 0x00708200
void CGItem_C::OnRightClick() {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && !player->IsDeadOrGhost()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_AUTOSTORE_GROUND_ITEM));
        msg.Put(this->m_obj->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }
}

// VERIFY: 0f1e7b39 manual
// OFFSET: 0x007089E0
void CGItem_C::RequestItemPurchaseData() {
    if (this->m_itemFlags & 0x4) {
        return;
    }

    this->m_itemFlags |= 0x4;

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GET_ITEM_PURCHASE_DATA));
    msg.Put(this->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: afbf928a manual
// OFFSET: 0x00708AC0
bool CGItem_C::HasEnchantments() {
    if (!this->IsBoundToAccount() && this->m_item->m_itemFieldEnchantment[0].m_id) {
        return true;
    }

    for (uint32_t i = 2; i < 4; i++) {
        if (!this->IsBoundToAccount() && this->m_item->m_itemFieldEnchantment[i].m_id) {
            return true;
        }
    }

    return !this->IsBoundToAccount() && this->m_item->m_itemFieldEnchantment[6].m_id;
}

// VERIFY: 104e4ba4 auto
// OFFSET: 0x00706BF0
int32_t CGItem_C::NumBaseCharges(int32_t itemId) {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (stats->m_spellId[i] && !stats->m_spellTrigger[i]) {
            return stats->m_spellCharges[i];
        }
    }

    return 0;
}

// VERIFY: 85394430 auto
// OFFSET: 0x00706C50
void CGItem_C::GetCooldowns(int32_t itemId, uint32_t& cooldown, uint32_t& categoryCooldown, uint32_t& category) {
    cooldown = 0;
    categoryCooldown = 0;
    category = 0;

    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, nullptr, nullptr, false);

    if (!stats) {
        return;
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (stats->m_spellId[i] && !stats->m_spellTrigger[i]) {
            cooldown = stats->m_spellCooldown[i];
            categoryCooldown = stats->m_spellCategoryCooldown[i];
            category = stats->m_spellCategory[i];
            return;
        }
    }
}

// VERIFY: d25d8e6d manual
// OFFSET: 0x00706D70
char* CGItem_C::BuildItemName(char* buffer, uint32_t size, int32_t itemId, int32_t randomPropertiesId) {
    *buffer = '\0';

    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, nullptr, nullptr, false);

    if (!stats) {
        return nullptr;
    }

    const char* name = stats->m_name[0];

    if (!name) {
        return nullptr;
    }

    if (randomPropertiesId > 0) {
        ItemRandomPropertiesRec* rec = g_itemRandomPropertiesDB.GetRecord(randomPropertiesId);

        if (rec && rec->m_nameLang && *rec->m_nameLang) {
            SStrPrintf(buffer, size, FrameScript_GetText("ITEM_SUFFIX_TEMPLATE", -1, GENDER_NOT_APPLICABLE), name, rec->m_nameLang);

            return buffer;
        }
    }

    if (randomPropertiesId < 0) {
        ItemRandomSuffixRec* rec = g_itemRandomSuffixDB.GetRecord(-randomPropertiesId);
        const char* suffix = rec ? rec->m_name : "";
        SStrPrintf(buffer, size, FrameScript_GetText("ITEM_SUFFIX_TEMPLATE", -1, GENDER_NOT_APPLICABLE), name, suffix);

        return buffer;
    }

    SStrCopy(buffer, name, size);

    return buffer;
}

// VERIFY: 0695924e auto
// OFFSET: 0x00706E60
int32_t CGItem_C::GetOnUseSlot() const {
    WOWGUID guid = ClntObjMgrGetActivePlayer();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    if (!stats) {
        return -1;
    }

    for (int32_t i = 0; i < 5; i++) {
        if (stats->m_spellId[i] && !stats->m_spellTrigger[i]) {
            return i;
        }
    }

    return -1;
}

// VERIFY: d7e003d5 auto
// OFFSET: 0x00706F40
int32_t CGItem_C::GetUsableSpellCharges() const {
    uint32_t id = this->GetSpellItemEnchantmentId();

    if (id) {
        SpellItemEnchantmentRec* rec = g_spellItemEnchantmentDB.GetRecord(id);

        if (rec) {
            for (uint32_t i = 0; i < 3; i++) {
                if (rec->m_effectArg[i] && rec->m_effect[i] == 7) {
                    return rec->m_charges;
                }
            }
        }
    }

    return CGItem_C::NumBaseCharges(this->m_obj->m_entryID);
}

// VERIFY: adcaf162 auto
// OFFSET: 0x00707180
int32_t CGItem_C::GetPageTextID(void (*callback)(int32_t, const WOWGUID&, void*, bool)) const {
    WOWGUID guid = this->m_obj->m_guid;
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, callback, nullptr, false);

    return stats ? stats->m_pageText : 0;
}

// VERIFY: b12c7a20 auto
// OFFSET: 0x007071D0
const char* CGItem_C::GetObjectName() const {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    return stats ? stats->m_name[0] : nullptr;
}

// VERIFY: ae01919f auto
// OFFSET: 0x007072B0
uint32_t CGItem_C::GetBagFamily() const {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    return stats ? stats->m_bagFamily : 0;
}

// VERIFY: bef4c8ce auto
// OFFSET: 0x00707360
ItemStats_C* CGItem_C::GetStats() const {
    WOWGUID guid = WOWGUID();

    return g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);
}

// VERIFY: 8b84f216 auto
// OFFSET: 0x00707390
uint32_t CGItem_C::GetItemStaticFlags(int32_t index) {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    return stats ? stats->m_flags[index] : 0;
}

// VERIFY: 4418573f auto
// OFFSET: 0x00707460
bool CGItem_C::IsZoneBound() const {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    return stats && stats->m_area;
}

// VERIFY: f1daf4fc auto
// OFFSET: 0x007074B0
bool CGItem_C::IsInstanceBound() const {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    return stats && stats->m_map;
}

// VERIFY: b81de831 manual
// OFFSET: 0x00707C20
char* CGItem_C::GetFullItemName(char* buffer, uint32_t size) const {
    int32_t randomPropertiesId = ((this->m_item->m_itemFieldFlags >> 3) & 1) ? 0 : this->m_item->m_itemFieldRandomPropertiesId;

    return CGItem_C::BuildItemName(buffer, size, this->m_obj->m_entryID, randomPropertiesId);
}

// VERIFY: de006961 auto
// OFFSET: 0x00706D30
void CGItem_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->CGObject_C::SetStorage(storage, saved);
    this->m_item = reinterpret_cast<CGItemData*>(reinterpret_cast<uint8_t*>(storage) + sizeof(CGObjectData));
    this->m_itemMirror = saved + CGObject::TotalFieldsSaved();
}

// VERIFY: a528b6b7 manual
// OFFSET: 0x007075A0
int32_t CGItem_C::GetRandomPropertyPoints() const {
    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    uint32_t itemLevel = stats->m_itemLevel;
    uint32_t quality = stats->m_quality;
    uint32_t slot = this->GetRandomItemSlot();

    if (slot > 4) {
        return 0;
    }

    RandPropPointsRec* rec = g_randPropPointsDB.GetRecord(itemLevel);

    if (!rec) {
        return 0;
    }

    switch (quality) {
        case 2:
            return rec->m_good[slot];

        case 3:
            return rec->m_superior[slot];

        case 4:
            return rec->m_epic[slot];

        default:
            return 0;
    }
}

// VERIFY: 6de9acae auto
// OFFSET: 0x00707DB0
int32_t CGItem_C::HasOnUseEnchant() const {
    return this->GetSpellItemEnchantmentId() != 0;
}

// VERIFY: 56fb9e40 manual
// OFFSET: 0x00707DC0
int32_t CGItem_C::GetCurrentSpellCharges() {
    if (this->GetSpellItemEnchantmentId()) {
        SpellItemEnchantmentRec* rec = g_spellItemEnchantmentDB.GetRecord(this->GetSpellItemEnchantmentId());

        if (rec) {
            for (uint32_t i = 0; i < 3; i++) {
                if (rec->m_effect[i] == 7 && rec->m_effectArg[i]) {
                    if (!this->IsBoundToAccount()) {
                        return this->m_item->m_itemFieldEnchantment[i].m_charges;
                    }

                    return 0;
                }
            }
        }
    }

    int32_t slot = this->GetOnUseSlot();

    if (slot >= 0) {
        return this->m_item->m_itemFieldSpellCharges[slot];
    }

    return 0;
}

// VERIFY: a124b25b manual
// OFFSET: 0x00707E60
int32_t CGItem_C::HasUseEffect() {
    if (this->GetItemStaticFlags(0) & 0x40) {
        return 1;
    }

    return this->GetSpellItemEnchantmentId() ? 1 : 0;
}

// VERIFY: 36aaddde manual
// OFFSET: 0x00708520
bool CGItem_C::IsBound() {
    if (this->m_item->m_itemFieldFlags & 0x1) {
        return true;
    }

    return this->HasSoulboundEnchant();
}

// VERIFY: 60e081ae manual
// OFFSET: 0x00708B40
bool CGItem_C::IsPermanentlyBoundForTrade() {
    if (!(this->m_item->m_itemFieldFlags & 0x1) && !this->HasSoulboundEnchant()) {
        return false;
    }

    if ((this->m_item->m_itemFieldFlags >> 8) & 1) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (player && (this->m_item->m_itemFieldOwner) == player->m_obj->m_guid) {
            int32_t createPlayedTime = this->m_item->m_itemFieldCreatePlayedTime;

            return createPlayedTime - player->GetPlayedTime() + 7200 <= 0;
        }
    }

    return true;
}

// VERIFY: 68c11c7d auto
// OFFSET: 0x007094E0
int32_t CGItem_C::GetNumSockets() {
    int32_t sockets = 0;

    WOWGUID guid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, guid, nullptr, nullptr, false);

    if (stats) {
        if (stats->m_socketColor[0]) {
            sockets = 1;
        }

        if (stats->m_socketColor[1]) {
            sockets++;
        }

        if (stats->m_socketColor[2]) {
            sockets++;
        }
    }

    return sockets + this->GetPrismaticSocketCount();
}

// VERIFY: 134df63d auto
// OFFSET: 0x00709550
bool CGItem_C::IsBoundForTrade() {
    return this->IsPermanentlyBoundForTrade() || this->HasSoulboundEnchant();
}

// VERIFY: 94374eab manual
// OFFSET: 0x00706B90
int32_t CGItem_C::GetUseSpell(int32_t itemId) {
    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    for (uint32_t i = 0; i < sizeof(stats->m_spellId) / sizeof(stats->m_spellId[0]); i++) {
        if (stats->m_spellId[i] && !stats->m_spellTrigger[i]) {
            return stats->m_spellId[i];
        }
    }

    return 0;
}

// VERIFY: 35a89283 auto
// OFFSET: 0x00709E20
void CGItem_C::RemoveNameFromCache(const char* name) {
    ITEMBYNAME* entry = s_itemByName.Ptr(name);

    if (entry) {
        s_itemByName.Delete(entry);
    }
}

// VERIFY: 22bc7913 auto
// OFFSET: 0x00707E90
void CGItem_C::GetSpellCooldownInfo(uint32_t& cooldown, uint32_t& categoryCooldown, uint32_t& category) {
    cooldown = 0;
    categoryCooldown = 0;
    category = 0;

    if (this->GetSpellItemEnchantmentId()) {
        SpellRec spell;

        if (g_spellDB.GetRecord(this->GetEnchantmentSpell(0), &spell)) {
            cooldown = spell.m_recoveryTime;
            categoryCooldown = spell.m_categoryRecoveryTime;
            category = spell.m_category;
            return;
        }
    }

    CGItem_C::GetCooldowns(this->m_obj->m_entryID, cooldown, categoryCooldown, category);
}

// VERIFY: 93d9343c manual
// OFFSET: 0x0070A850
static void AddInventoryArtHash(int32_t displayId, const char* texture) {
    if (s_inventoryArt.Ptr(displayId, HASHKEY_NONE())) {
        return;
    }

    INVENTORYART* art = s_inventoryArt.New(displayId, HASHKEY_NONE(), 0, 0);

    if (art->m_texture) {
        SMemFree(art->m_texture, __FILE__, __LINE__, 0);
        art->m_texture = nullptr;
    }

    if (texture && *texture) {
        art->m_texture = SStrDupA(texture, __FILE__, __LINE__);
    }
}

// VERIFY: a1157b4b auto
// OFFSET: 0x0070A910
const char* CGItem_C::GetInventoryArt(int32_t displayId) {
    INVENTORYART* art = s_inventoryArt.Ptr(displayId, HASHKEY_NONE());

    if (art && art->m_texture) {
        return art->m_texture;
    }

    ItemDisplayInfoRec display;

    if (g_itemDisplayInfoDB.GetRecord(displayId, &display)) {
        const char* icon = ViolenceGetLevel() >= 2 || !*display.m_inventoryIcon[1] ? display.m_inventoryIcon[0] : display.m_inventoryIcon[1];

        if (icon) {
            if (*icon) {
                char buffer[260];

                if (TextureDiscoverFileType(icon) == TEXFILE_TGA) {
                    TexturePickAlternateFilename(icon, TEXFILE_TGA, buffer, sizeof(buffer));
                    icon = buffer;
                }

                AddInventoryArtHash(displayId, icon);
                art = s_inventoryArt.Ptr(displayId, HASHKEY_NONE());
                icon = art ? art->m_texture : nullptr;
            }

            if (icon && *icon) {
                return icon;
            }
        }
    }

    SysMsgPrintf(STATUS_ERROR, 2, "NOINVENTORYICON|%d", displayId);
    return "INV_Misc_QuestionMark";
}

// VERIFY: 3c4650e1 auto
// OFFSET: 0x0070AA00
const char* CGItem_C::GetInventoryArt() const {
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);

    if (rec) {
        return CGItem_C::GetInventoryArt(rec->m_displayInfoID);
    }

    return CGItem_C::GetInventoryArt(0);
}

// VERIFY: 61c58006 manual
// OFFSET: 0x00584AC0
uint32_t CGItem::GetDurability() const {
    if (this->m_item->m_itemFieldFlags & 8) {
        return 0;
    }

    return this->m_item->m_itemFieldDurability;
}

// VERIFY: e00bdbda manual
// OFFSET: 0x00584AE0
uint32_t CGItem::GetMaxDurability() const {
    if (this->m_item->m_itemFieldFlags & 8) {
        return 0;
    }

    return this->m_item->m_itemFieldMaxdurability;
}

// VERIFY: d3247ea5 manual
// OFFSET: 0x0061DC90
int32_t CGItem::GetPetitionSignatures() {
    if (this->GetItemStaticFlags(0) & 0x2000) {
        return this->m_item->m_itemFieldEnchantment[0].m_duration;
    }

    return 0;
}

// VERIFY: 5cea77df manual
// OFFSET: 0x00707650
int32_t CGItem_C::GetRandomPropertyValue(int32_t slot, int32_t randomPropertiesId, int32_t points) {
    float value = 0.0f;

    if (randomPropertiesId < 0) {
        ItemRandomSuffixRec* suffix = g_itemRandomSuffixDB.GetRecord(-randomPropertiesId);
        uint32_t index = slot - 7;

        if (suffix && static_cast<int32_t>(index) >= 0 && index < 5) {
            value = suffix->m_allocationPct[index] * 0.0001f * points;
        }
    }

    return CMath::ftrunc(value);
}

// VERIFY: bf61ad32 manual
// OFFSET: 0x00707C60
int32_t CGItem_C::GetEnchantmentSpell(int32_t effect) {
    SpellRec spell;
    int32_t spellId = 0;
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, WOWGUID{ 0 }, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (!stats->m_spellId[i] || stats->m_spellTrigger[i]) {
            continue;
        }

        bool match = !effect;

        if (!match && g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
            for (uint32_t j = 0; j < 3; j++) {
                if (spell.m_effect[j] == effect) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            spellId = stats->m_spellId[i];

            if (spellId) {
                return spellId;
            }

            break;
        }
    }

    for (uint32_t slot = 0; slot < 12; slot++) {
        SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(this->m_item->m_itemFieldEnchantment[slot].m_id);

        if (enchant) {
            for (uint32_t i = 0; i < 3; i++) {
                if (enchant->m_effect[i] == 7) {
                    spellId = enchant->m_effectArg[i];
                    break;
                }
            }
        }

        if (spellId) {
            break;
        }
    }

    return spellId;
}

// VERIFY: 24e20657 manual
// OFFSET: 0x00708540
int32_t CGItem_C::GetRepairCost() {
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, WOWGUID{ 0 }, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    CGItemData* item = this->m_item;

    if ((item->m_itemFieldFlags & 8) || !item->m_itemFieldMaxdurability) {
        return 0;
    }

    uint32_t durability = (item->m_itemFieldFlags & 8) ? 0 : item->m_itemFieldDurability;

    if ((item->m_itemFieldFlags & 8) || durability >= item->m_itemFieldMaxdurability) {
        return 0;
    }

    DurabilityCostsRec* costs = g_durabilityCostsDB.GetRecord(stats->m_itemLevel);

    if (!costs) {
        return 0;
    }

    uint32_t itemClass = this->GetClassID();

    if (itemClass != 2 && itemClass != 4) {
        return 0;
    }

    uint32_t subClass = this->GetSubtypeID();
    int32_t cost;

    if (itemClass == 2) {
        if (subClass > 21) {
            return 0;
        }

        cost = costs->m_weaponSubClassCost[subClass];
    } else {
        if (itemClass != 4 || subClass > 8) {
            return 0;
        }

        cost = costs->m_armorSubClassCost[subClass];
    }

    DurabilityQualityRec* quality = g_durabilityQualityDB.GetRecordByIndex(2 * stats->m_quality + 1);

    if (!quality) {
        return 0;
    }

    int32_t lost = this->GetMaxDurability() - this->GetDurability();
    int32_t result = CMath::fint_n(lost * quality->m_data * cost);
    return result ? result : 1;
}

// VERIFY: 84e69779 manual
// OFFSET: 0x007086B0
int32_t CGItem_C::IsLocked(const LockRec* lock, int32_t itemLevel, int32_t* spellId, int32_t* skillValue, int32_t* requiredSkill, int32_t* lockIndex, CGItem_C** key, int32_t* entry) {
    int32_t locked = 0;
    SpellRec spell;

    for (int32_t i = 0; i < 8; i++) {
        int32_t type = lock->m_type[i];

        if (!type) {
            continue;
        }

        if (type == 2) {
            locked = 1;

            for (uint32_t j = 0; j < s_openLockSpells.m_count; j++) {
                g_spellDB.GetRecord(s_openLockSpells.m_data[j], &spell);

                for (uint32_t effect = 0; effect < 3; effect++) {
                    if (spell.m_effect[effect] != 33 || spell.m_effectMiscValue[effect] != lock->m_index[i]) {
                        continue;
                    }

                    int32_t required = lock->m_skill[i] ? lock->m_skill[i] : 5 * itemLevel;
                    int32_t minPoints;
                    int32_t maxPoints;
                    Spell_C_GetMinMaxPoints(&spell, effect, &minPoints, &maxPoints, 0, 0, 0, 0);

                    if (spellId) {
                        *spellId = s_openLockSpells.m_data[j];
                    }

                    if (skillValue) {
                        *skillValue = minPoints;
                    }

                    if (requiredSkill) {
                        *requiredSkill = required;
                    }

                    if (lockIndex) {
                        *lockIndex = lock->m_index[i];
                    }

                    if (minPoints >= required) {
                        if (entry) {
                            *entry = i;
                        }

                        return 0;
                    }
                }
            }
        } else if (type == 3) {
            locked = 1;

            g_spellDB.GetRecord(lock->m_index[i], &spell);

            for (uint32_t effect = 0; effect < 3; effect++) {
                if (spell.m_effect[effect] == 33) {
                    if (spellId) {
                        *spellId = spell.m_id;
                    }

                    if (entry) {
                        *entry = i;
                    }

                    return 0;
                }
            }
        } else if (type == 1) {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

            if (player) {
                locked = 1;
                CGItem_C* item = player->m_bag.FindItemOfType(lock->m_index[i], 0);

                if (item) {
                    if (spellId) {
                        *spellId = item->GetEnchantmentSpell(0);
                    }

                    if (key) {
                        *key = item;
                    }

                    if (entry) {
                        *entry = i;
                    }

                    return 0;
                }
            }
        }
    }

    return locked;
}

// VERIFY: bb164cd0 auto
// OFFSET: 0x007089B0
int32_t CGItem_C::GetRandomPropertyValue(int32_t slot) {
    return CGItem_C::GetRandomPropertyValue(slot, this->m_item->m_itemFieldRandomPropertiesId, this->GetRandomPropertyPoints());
}

// VERIFY: eaccfb41 auto
// OFFSET: 0x00708C10
int32_t CGItem_C::CanBeUsed() {
    return this->GetEnchantmentSpell(0) != 0;
}

// VERIFY: 2efe0bae auto
// OFFSET: 0x006081E0
static int32_t FindItemCallback(const CGItem_C* item, void* param) {
    return item == param;
}

// VERIFY: cb205c40 manual
// OFFSET: 0x0060BFE0
void CGItem_C::SignalEvent(CGItem_C* item, uint32_t event) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    CGBag_C* bag = &player->m_bag;

    if (!bag) {
        return;
    }

    WOWGUID container;
    uint32_t slot;

    if (!bag->FindItem(FindItemCallback, item, container, slot, 136)) {
        return;
    }

    if (container == player->m_bag.m_guid) {
        if (slot <= 22) {
            FrameScript_SignalEvent(event, "%d", slot + 1);
        } else if (slot - 23 <= 15) {
            FrameScript_SignalEvent(event, "%d%d", 0, slot - 22);
        } else if ((slot >= 39 && slot <= 66) || slot - 67 <= 6) {
            FrameScript_SignalEvent(event, "%d%d", -1, slot - 38);
        } else if (slot - 86 <= 31) {
            FrameScript_SignalEvent(event, "%d%d", -2, slot - 85);
        } else if (slot - 118 <= 31) {
            FrameScript_SignalEvent(event, "%d%d", -4, slot - 117);
        }

        return;
    }

    for (uint32_t i = 0; i < 4; i++) {
        WOWGUID guid = i + 19 < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[i + 19] : WOWGUID();

        if (guid == container) {
            FrameScript_SignalEvent(event, "%d%d", i + 1, slot + 1);
            return;
        }
    }

    for (uint32_t i = 0; i < 7; i++) {
        WOWGUID guid = i + 67 < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[i + 67] : WOWGUID();

        if (guid == container) {
            FrameScript_SignalEvent(event, "%d%d", i + 5, slot + 1);
            return;
        }
    }
}

// VERIFY: 06278cc4 auto
// OFFSET: 0x00706FE0
void CGItem_C::Lock() {
    if (!(this->m_itemFlags & 1)) {
        this->m_itemFlags |= 1;
        CGItem_C::SignalEvent(this, SCRIPT_EVENT_ITEM_LOCK_CHANGED);
        CGItem_C::SignalEvent(this, SCRIPT_EVENT_ITEM_LOCKED);
    }
}

// VERIFY: 8c3234ae auto
// OFFSET: 0x00707020
void CGItem_C::Unlock() {
    if (this->m_itemFlags & 1) {
        this->m_itemFlags &= ~1u;
        CGItem_C::SignalEvent(this, SCRIPT_EVENT_ITEM_LOCK_CHANGED);
        CGItem_C::SignalEvent(this, SCRIPT_EVENT_ITEM_UNLOCKED);
    }
}

// STATIC: 0x00CA1128
WOWGUID CGItem_C::s_bindOnUseTarget;

// STATIC: 0x00CA1138
WOWGUID CGItem_C::s_bindOnUseItem;

// VERIFY: 92b5a602 manual
// OFFSET: 0x00708C20
int32_t CGItem_C::UseItem(WOWGUID* target, int32_t confirmed) {
    if ((this->m_obj->m_type & TYPEMASK_CONTAINER) || (this->m_itemFlags & 1)) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t spellId = this->GetEnchantmentSpell(0);
    SpellRec spell;
    int32_t hasSpell = g_spellDB.GetRecord(spellId, &spell);
    int16_t overrideId = static_cast<int16_t>(player->m_player->m_playerFieldBytes2);

    if (overrideId) {
        OverrideSpellDataRec* overrideRec = g_overrideSpellDataDB.GetRecord(overrideId);

        if (overrideRec && !(overrideRec->m_flags & 1)) {
            CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
            return 0;
        }
    }

    if (player->HasVehicleTransport() && !hasSpell) {
        WOWGUID transport;
        transport.m_guid = player->GetTransportGUID();
        CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transport, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (vehicle) {
            VehicleSeatRec* seat = vehicle->GetVehicleSeatRec(player->m_movementData.m_transportSeat);

            if (seat && !(seat->m_flags & 0x4000)) {
                CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
                return 0;
            }
        }
    }

    if (Spell_C_IsTargeting() && Spell_C_CanTargetItems()) {
        CGGameUI::CastItemSpell(this->m_obj->m_guid, 0);
        return 0;
    }

    if (stats->m_flags[0] & 0x200) {
        if (this->m_item->m_itemFieldFlags & 0x8) {
            player->OpenWrappedItem(this);
        } else {
            CGPlayer_C::StartGiftWrap(this);
        }

        return 0;
    }

    if (stats->m_startQuest) {
        CGPlayer_C::QueryQuest(this->m_obj->m_guid, stats->m_startQuest);
        return 0;
    }

    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (map && map->m_instanceType == 4 && !(stats->m_flags[0] & 0x200000)) {
        uint32_t cooldown;
        uint32_t categoryCooldown;
        uint32_t category;

        if (stats->m_bonding == 4 || (stats->m_flags[0] & 0x4000000) || (this->GetSpellCooldownInfo(cooldown, categoryCooldown, category), this->GetUsableSpellCharges() < 0) || cooldown > 900000 || categoryCooldown > 900000) {
            CGGameUI::DisplayError(ERR_CANT_USE_ITEM_IN_ARENA);
            return 0;
        }
    }

    int32_t error = ERR_MAX_VALUE;

    if (!player->CanUseItem(stats, error)) {
        if (error != ERR_CANT_EQUIP_LEVEL_I) {
            CGGameUI::DisplayError(static_cast<UIErrors>(error));
        } else {
            CGGameUI::DisplayError(ERR_CANT_EQUIP_LEVEL_I, stats->m_requiredLevel);
        }

        return 0;
    }

    uint32_t enchantId = this->GetSpellItemEnchantmentId();

    if (enchantId) {
        SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(enchantId);

        if (enchant && enchant->m_requiredSkillID && static_cast<int32_t>(player->GetSkillValue(player->GetSkillIndex(enchant->m_requiredSkillID))) < enchant->m_requiredSkillRank) {
            CGGameUI::DisplayError(static_cast<UIErrors>(GetDisplayErrorByInventoryError(2)));
            return 0;
        }
    }

    if (stats->m_pageText || (this->m_item->m_itemFieldFlags & 0x200)) {
        CGItemText::SetItem(this->m_obj->m_guid, 0);
        return 0;
    }

    if (!CGGameUI::m_hasControl) {
        int32_t allowed = 0;

        if (!hasSpell) {
            CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
            return 0;
        }

        if ((player->m_unit->m_unitFieldFlags & 0x800000) && ((spell.m_attributesExE & 0x20000) || Spell_C_CancelsFear(player, &spell, 0))) {
            allowed = 1;
        }

        if ((player->m_unit->m_unitFieldFlags & 0x400000) && ((spell.m_attributesExE & 0x40000) || Spell_C_CancelsConfuse(player, &spell, 0))) {
            allowed = 1;
        }

        int32_t charmAllowed = player->IsCharmed() && Spell_C_CancelsCharm(player, &spell, 0) ? 1 : allowed;

        if ((!(player->m_unit->m_unitFieldFlags & 0x100000) || !(spell.m_attributes & 0x1000000)) && !charmAllowed) {
            CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
            return 0;
        }
    }

    if (stats->m_flags[0] & 0x4) {
        player->OpenLootItem(this);
        return 0;
    }

    if (stats->m_flags[0] & 0x2000) {
        CGPlayer_C::RequestPetitionSignatures(this->m_obj->m_guid);
        return 0;
    }

    if (!this->GetEnchantmentSpell(0)) {
        return 0;
    }

    if (this->GetInventoryType()) {
        if (this->m_item->m_itemFieldContained != ClntObjMgrGetActivePlayer()) {
            CGGameUI::DisplayError(ERR_MUST_EQUIP_ITEM);
            return 0;
        }

        if (this->GetMaxDurability() && !this->GetDurability()) {
            CGGameUI::DisplayError(ERR_MUST_REPAIR_DURABILITY);
            return 0;
        }

        int32_t slot = player->GetBag()->FindItemSlot(this->m_obj->m_guid);

        if (slot == 15) {
            if (player->IsDisarmed(0)) {
                CGGameUI::DisplayError(ERR_CANT_USE_DISARMED);
                return 0;
            }
        } else if (slot == 16) {
            if (player->IsDisarmed(1)) {
                CGGameUI::DisplayError(ERR_CANT_USE_DISARMED);
                return 0;
            }
        } else if (slot >= 23) {
            CGGameUI::DisplayError(ERR_MUST_EQUIP_ITEM);
            return 0;
        }
    }

    if (hasSpell) {
        if (Spell_C_IsActiveSpell(&spell, player)) {
            if (!Spell_C_WasItemRecentlyUsed(this->m_obj->m_entryID)) {
                Spell_C_CancelAura(&spell, player, -1);
            }

            return 0;
        }

        if (this->GetUsableSpellCharges() > 0 && !this->GetCurrentSpellCharges()) {
            CGGameUI::DisplayError(ERR_SPELL_FAILED_S, FrameScript_GetText("SPELL_FAILED_NO_CHARGES_REMAIN", -1, GENDER_NOT_APPLICABLE));
            return 0;
        }

        if (SpellRec::HasAura(&spell, 78) && !player->CanCurrentFormMount() && player->CanAutoUnshift()) {
            player->CancelShapeshiftForMount();
        }
    }

    if (player->IsShapeShifted()) {
        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm());

        if (form && !(form->m_flags & 0x80) && (!(form->m_flags & 0x40) || !this->GetInventoryType()) && !(stats->m_flags[0] & 0x800000)) {
            if (!player->CanAutoUnshift()) {
                CGGameUI::DisplayError(ERR_NO_ITEMS_WHILE_SHAPESHIFTED);
                return 0;
            }

            player->CancelShapeshift();
        }
    }

    if (stats->m_bonding != 3 || this->IsBound() || confirmed) {
        return Spell_C::CastSpell(spellId, this, *target, false);
    }

    CGItem_C::s_bindOnUseItem = this->m_obj->m_guid;
    CGItem_C::s_bindOnUseTarget = *target;
    FrameScript_SignalEvent(SCRIPT_EVENT_USE_BIND_CONFIRM, nullptr);
    return 0;
}

// VERIFY: 6c2aeb5c manual
// OFFSET: 0x0070AA40
void CGItem_C::PostInitWithStats() {
    this->m_itemFlags |= 2;
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    this->m_virtualItem.m_classID = rec ? rec->m_classID : 0;
    this->m_virtualItem.m_subclassID = rec ? rec->m_subclassID : 0;
    this->m_virtualItem.m_material = rec ? rec->m_material : 0;
    this->m_virtualItem.m_inventoryType = rec ? rec->m_inventoryType : 0;
    this->m_virtualItem.m_sheatheType = rec ? rec->m_sheatheType : 0;
    this->m_virtualItem.m_soundOverrideSubclassID = rec ? rec->m_soundOverrideSubclassID : 0xFF;
    ItemDisplayInfoRec display;

    if (g_itemDisplayInfoDB.GetRecord(rec ? rec->m_displayInfoID : 0, &display)) {
        this->m_groupSounds = g_itemGroupSoundsDB.GetRecord(display.m_groupSoundIndex);
    } else {
        this->m_groupSounds = nullptr;
    }

    char name[1024];
    CGItem_C::BuildItemName(name, sizeof(name), this->m_obj->m_entryID, (this->m_item->m_itemFieldFlags & 0x8) ? 0 : this->m_item->m_itemFieldRandomPropertiesId);

    if (!s_itemByName.Ptr(name)) {
        ITEMBYNAME* entry = s_itemByName.New(name, 0, 0);
        entry->m_itemId = this->m_obj->m_entryID;
    }

    if (!CGGameUI::s_inWorld) {
        return;
    }

    if (this->m_item->m_itemFieldOwner == ClntObjMgrGetActivePlayer()) {
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(this->m_obj->m_entryID, none, nullptr, nullptr, false);
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (stats->m_stackable <= 1) {
            CGGameUI::UnlockItem(this->m_obj->m_guid);
        } else if (player) {
            player->m_bag.UnlockItemsOfType(this->m_obj->m_entryID, 0);
        }

        if (player) {
            player->GetPendingEnchantTime(this);
            int32_t slot = player->m_bag.FindItemSlot(this->m_obj->m_guid);

            if (slot == 15 || slot == 16) {
                CGActionBar::UpdateAttackAction();
            } else if (slot == 17) {
                CGActionBar::UpdateRangedAttackAction();
            }
        }

        CGTradeSkillInfo::RefreshList(0);
        CGActionBar::UpdateItem(this->m_obj->m_entryID);
        CGQuestLog::UpdateQuestItem(this->m_obj->m_entryID);
        CGContainerInfo::UpdateItem(this->m_obj->m_guid);

        if ((stats->m_flags[0] & 0x1000) && !this->m_itemPurchaseData && !(this->m_itemFlags & 4)) {
            this->RequestItemPurchaseData();
        }

        if (CGActionBar::GetMultiCastSlotMaskForTotemCategory(stats->m_totemCategory)) {
            CGActionBar::UpdateMultiCastBar();
        }
    }

    Script_SendUnitSignal(this->m_item->m_itemFieldOwner, SCRIPT_EVENT_UNIT_INVENTORY_CHANGED);
}

// VERIFY: f72af9bc manual
// OFFSET: 0x007082B0
int32_t CGItem_C::CanGoInSlot(int32_t itemId, uint32_t slot, int32_t bypass) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    ItemRec* rec = g_itemDB.GetRecord(itemId);

    if (!rec) {
        return 0;
    }

    if (slot == 0xFFFFFFFF) {
        return rec->m_classID == 6;
    }

    if (slot == 17) {
        if (bypass != (rec->m_inventoryType == 28)) {
            return 0;
        }

        return (s_inventoryTypeSlots[rec->m_inventoryType] & (1 << slot)) != 0;
    }

    if ((slot != 15 && slot != 16) || rec->m_classID != 2) {
        if (slot >= 23) {
            return 1;
        }

        return (s_inventoryTypeSlots[rec->m_inventoryType] & (1 << slot)) != 0;
    }

    if (slot == 16) {
        if (rec->m_inventoryType != 17 && rec->m_inventoryType != 21) {
            return (s_inventoryTypeSlots[rec->m_inventoryType] & (1 << slot)) != 0;
        }
    } else if (rec->m_inventoryType != 22) {
        return (s_inventoryTypeSlots[rec->m_inventoryType] & (1 << slot)) != 0;
    }

    if ((stats->m_flags[0] & 0x40000000) || rec->m_subclassID == 13) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    for (uint32_t i = 0; i < player->m_equipBypassSkills.m_count; i++) {
        SpellRec spell;

        if (!g_spellDB.GetRecord(player->GetEquipBypassSkill(i), &spell) || rec->m_classID != spell.m_equippedItemClass || !((1 << rec->m_subclassID) & spell.m_equippedItemSubclass)) {
            continue;
        }

        bool found = false;

        for (int32_t invType = 0; invType < 29; invType++) {
            if (((1 << invType) & spell.m_equippedItemInvTypes) && (s_inventoryTypeSlots[invType] & (1 << slot))) {
                found = true;
            }
        }

        if (found) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: e12d8511 auto
// OFFSET: 0x00708500
int32_t CGItem_C::CanGoInSlot(uint32_t slot, int32_t bypass) {
    return CGItem_C::CanGoInSlot(this->m_obj->m_entryID, slot, bypass);
}

// VERIFY: f2b21d47 auto
// OFFSET: 0x00709DE0
int32_t GetItemIDByName(const char* name) {
    const char* link = SStrStr(name, "item:");

    if (link) {
        return SStrToInt(link + 5);
    }

    ITEMBYNAME* entry = s_itemByName.Ptr(name);

    if (entry) {
        return entry->m_itemId;
    }

    return 0;
}

// VERIFY: 6c2f725a manual
// OFFSET: 0x00707F50
void CGItem_C::UpdateEnchantments() {
    if (this->GetItemStaticFlags(0) & 0x2000) {
        return;
    }

    CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(this->m_item->m_itemFieldOwner, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!owner) {
        return;
    }

    if (owner->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGActionBar::UpdateItem(this->m_obj->m_entryID);
        CGTradeInfo::UpdatePlayerItem(this->m_obj->m_guid);
        CGContainerInfo::UpdateItem(this->m_obj->m_guid);
        CGCharacterInfo::UpdateItem(this->m_obj->m_guid);
        CGCharacterInfo::UpdateInventoryAlerts();
        CGItemSocketInfo::UpdateItem(this->m_obj->m_guid);
    }

    uint8_t slot = owner->FindSlotIndex(this->m_obj->m_guid);

    if (slot <= 18) {
        owner->UpdateObjComponentVisualsSlot(owner->GetVisibleItem(slot), slot);
    }
}

// VERIFY: 43b3d3d7 manual
// OFFSET: 0x00708040
int32_t CGItem_C::GetItemEnchantmentVisualID() const {
    ItemDisplayInfoRec displayRec;
    ItemRec* rec = g_itemDB.GetRecord(this->m_obj->m_entryID);
    int32_t displayId = rec ? rec->m_displayInfoID : 0;

    if (!g_itemDisplayInfoDB.GetRecord(displayId, &displayRec)) {
        return 0;
    }

    if (g_itemVisualsDB.GetRecord(displayRec.m_itemVisual)) {
        return -1;
    }

    for (uint32_t i = 0; i < 12; i++) {
        SpellItemEnchantmentRec* enchantment = g_spellItemEnchantmentDB.GetRecord(this->m_item->m_itemFieldEnchantment[i].m_id);

        if (enchantment && enchantment->m_itemVisual) {
            return enchantment->m_itemVisual;
        }
    }

    return 0;
}

// VERIFY: 8be7f81c auto
// OFFSET: 0x007081B0
void CGItem_C::UpdateSockets(uint32_t gem1, uint32_t gem2, uint32_t gem3, uint32_t socketBonus) {
    if (gem1) {
        this->m_item->m_itemFieldEnchantment[2].m_id = gem1;
    }

    if (gem2) {
        this->m_item->m_itemFieldEnchantment[3].m_id = gem2;
    }

    if (gem3) {
        this->m_item->m_itemFieldEnchantment[4].m_id = gem3;
    }

    if (socketBonus) {
        this->m_item->m_itemFieldEnchantment[5].m_id = socketBonus;
    }

    this->UpdateEnchantments();
}

// VERIFY: ccea7448 auto
// OFFSET: 0x0070AFC0
void CGItem_C::Disable() {
    CGObject_C::Disable();

    if (CGLootInfo::m_lootObj == this->m_obj->m_guid) {
        CGGameUI::CloseLoot(true, false, false);
    }

    WOWGUID guid = this->m_obj->m_guid;
    Spell_C_CancelSpellByCaster(guid);

    if (this->m_obj->m_guid == CGGameUI::GetCursorItem()) {
        CGGameUI::ClearCursor(1, 1);
    }

    if (this->m_obj->m_guid == CGItemText::m_itemGUID) {
        CGItemText::SetItem(WOWGUID(), 0);
    }

    if (!(this->m_itemFlags & 0x2)) {
        g_itemDBCache.CancelCallback(this->m_obj->m_entryID, &LoadItemCacheCallback, this);
        CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(this->m_item->m_itemFieldOwner, TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (owner) {
            owner->m_pendingItemCacheCount--;
        }
    }

    if (this->m_item->m_itemFieldOwner == ClntObjMgrGetActivePlayer()) {
        CGTradeSkillInfo::RefreshList(0);
        CGActionBar::UpdateItem(this->m_obj->m_entryID);
        CGTradeInfo::RemovePlayerItem(this->m_obj->m_guid);
        CGQuestLog::UpdateQuestItem(this->m_obj->m_entryID);
        CGCharacterInfo::UpdateItem(this->m_obj->m_guid);
        CGItemSocketInfo::RemoveItem(this->m_obj->m_guid);
    }
}
