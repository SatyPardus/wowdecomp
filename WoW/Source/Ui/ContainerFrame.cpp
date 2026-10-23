#include "ContainerFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Container_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/CurrencyTypes.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/Ui/Tutorial.h"
#include <cstddef>

// STATIC: 0x00C23540
WOWGUID CGContainerInfo::m_containers[TOTAL_BAG_SLOTS];

// VERIFY: 2f485abd auto
// OFFSET: 0x005D9C20
void CGContainerInfo::EnterWorld() {
    for (uint32_t offset = 0x358; offset <= 0x370; offset += sizeof(WOWGUID)) {
        ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, sizeof(WOWGUID), InvUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    }

    for (uint32_t offset = 0x378; offset <= 0x3F0; offset += sizeof(WOWGUID)) {
        ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, sizeof(WOWGUID), UpdateInvContents, nullptr, HANDLER_PRIORITY_NORMAL, 1);
    }

    for (uint32_t offset = 0x4D8; offset <= 0x508; offset += sizeof(WOWGUID)) {
        ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, sizeof(WOWGUID), InvUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    }

    for (uint32_t offset = 0x570; offset <= 0x668; offset += sizeof(WOWGUID)) {
        ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, sizeof(WOWGUID), UpdateInvContents, nullptr, HANDLER_PRIORITY_NORMAL, 1);
    }

    for (uint32_t offset = 0x670; offset <= 0x768; offset += sizeof(WOWGUID)) {
        ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, sizeof(WOWGUID), UpdateInvContents, nullptr, HANDLER_PRIORITY_NORMAL, 1);
    }

    for (uint32_t i = 0; i < sizeof(CGContainerInfo::m_containers) / sizeof(CGContainerInfo::m_containers[0]); i++) {
        CGContainerInfo::m_containers[i] = WOWGUID();
    }

    CGContainerInfo::UpdateContainers();
}

// VERIFY: 9a1d25a2 auto
// OFFSET: 0x005D7110
void CGContainerInfo::OpenContainer(WOWGUID guid) {
    if (guid == ClntObjMgrGetActivePlayer()) {
        FrameScript_SignalEvent(SCRIPT_EVENT_BAG_OPEN, "%d", 0);
        return;
    }

    for (uint32_t i = 0; i < TOTAL_BAG_SLOTS; i++) {
        if (CGContainerInfo::m_containers[i] == guid) {
            FrameScript_SignalEvent(SCRIPT_EVENT_BAG_OPEN, "%d", i + 1);
        }
    }
}

// VERIFY: bf060c1c manual
// OFFSET: 0x005D9D90
void CGContainerInfo::LeaveWorld() {
    for (uint32_t i = 0; i < TOTAL_BAG_SLOTS; i++) {
        if (!CGContainerInfo::m_containers[i]) {
            continue;
        }

        CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(CGContainerInfo::m_containers[i], TYPE_CONTAINER, __FILE__, __LINE__));

        if (!container) {
            continue;
        }

        int32_t count = container->GetBag()->m_numSlots;

        for (int32_t j = 0; j < count; j++) {
            ClntObjMgrUnsetObjMirrorHandler(CGContainerInfo::m_containers[i], ID_CONTAINER, offsetof(CGContainerData, m_containerFieldSlot1) + j * sizeof(WOWGUID), UpdateContainerContents, nullptr);
        }
    }

    for (uint32_t offset = 0x378; offset <= 0x3F0; offset += sizeof(WOWGUID)) {
        ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, UpdateInvContents, nullptr);
    }

    for (uint32_t offset = 0x358; offset <= 0x370; offset += sizeof(WOWGUID)) {
        ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, InvUpdateHandler, nullptr);
    }

    for (uint32_t offset = 0x4D8; offset <= 0x508; offset += sizeof(WOWGUID)) {
        ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, InvUpdateHandler, nullptr);
    }

    for (uint32_t offset = 0x570; offset <= 0x668; offset += sizeof(WOWGUID)) {
        ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, UpdateInvContents, nullptr);
    }

    for (uint32_t offset = 0x670; offset <= 0x768; offset += sizeof(WOWGUID)) {
        ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, offset, UpdateInvContents, nullptr);
    }
}

// VERIFY: bf489b49 auto
// OFFSET: 0x005D6F10
void CGContainerInfo::UpdateCooldowns() {
    FrameScript_SignalEvent(SCRIPT_EVENT_BAG_UPDATE_COOLDOWN, nullptr);
}

// VERIFY: 1e625228 manual
// OFFSET: 0x005D6F20
WOWGUID GetBagAtIndex(uint32_t index) {
    if (index <= TOTAL_BAG_SLOTS - 1 && (index < NUM_BAG_SLOTS || CGBankInfo::m_banker)) {
        return CGContainerInfo::m_containers[index];
    }

    return WOWGUID();
}

// VERIFY: 8542c504 manual
// OFFSET: 0x005D7380
bool CGContainerInfo::GetLuaBagAndSlot(lua_State* L, CGBag_C** bag, int32_t* slot, int32_t* bank) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    *bag = nullptr;
    *bank = 0;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    *slot = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;

    if (index >= 0) {
        if (index < TOTAL_BAG_SLOTS) {
            CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(GetBagAtIndex(index), TYPE_CONTAINER, __FILE__, __LINE__));

            if (container) {
                *bag = container->GetBag();
            }

            if (index >= NUM_BAG_SLOTS) {
                *bank = 1;
            }
        }
    } else {
        *bag = &player->m_bag;

        switch (index) {
            case -5:
                return false;

            case -3:
                *slot += KEYRING_FIRST;

                if (*slot > KEYRING_LAST) {
                    return false;
                }

                break;

            case -2:
                *slot += BANKGENERIC_FIRST;

                if (*slot > BANKGENERIC_LAST) {
                    return false;
                }

                *bank = 1;
                break;

            case -1:
                *slot += BACKPACK_FIRST;

                if (*slot > BACKPACK_LAST) {
                    return false;
                }

                break;
        }
    }

    if (!*bag) {
        return false;
    }

    return *slot >= 0 && *slot < (*bag)->m_numSlots;
}

// VERIFY: 15a3090a manual
// OFFSET: 0x005D7070
void CGContainerInfo::UpdateContents(WOWGUID guid) {
    CGActionBar::UpdateUsable();

    if (guid == ClntObjMgrGetActivePlayer()) {
        FrameScript_SignalEvent(SCRIPT_EVENT_BAG_UPDATE, "%d", 0);
        FrameScript_SignalEvent(SCRIPT_EVENT_BAG_UPDATE, "%d", -2);
        FrameScript_SignalEvent(SCRIPT_EVENT_BAG_UPDATE, "%d", -4);
        return;
    }

    for (uint32_t i = 0; i < TOTAL_BAG_SLOTS; i++) {
        if (CGContainerInfo::m_containers[i] == guid) {
            FrameScript_SignalEvent(SCRIPT_EVENT_BAG_UPDATE, "%d", i + 1);
            return;
        }
    }
}

// VERIFY: 86768ce2 manual
// OFFSET: 0x005D7320
void CGContainerInfo::UpdateItem(WOWGUID guid) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

    if (item && item->m_item->m_itemFieldOwner == ClntObjMgrGetActivePlayer()) {
        CGContainerInfo::UpdateContents(item->m_item->m_itemFieldContained);
    }
}

// VERIFY: dcc6ac8d manual
// OFFSET: 0x005D9540
static void TriggerMultiCastBarUpdate(const ItemStats_C* stats) {
    if (!stats) {
        return;
    }

    uint32_t mask = CGActionBar::GetMultiCastSlotMaskForTotemCategory(stats->m_totemCategory);

    if (!mask) {
        return;
    }

    if (CGTutorial::m_tutorialShown.m_count && !(CGTutorial::m_tutorialShown.m_bits.m_data[1] & 0x4000)) {
        for (uint32_t i = 0; i < 4; i++) {
            if (((1 << i) & mask) && CGSpellBook::m_totemSpells[i].Count()) {
                CGTutorial::TriggerTutorial(46);
                break;
            }
        }
    }

    CGActionBar::UpdateMultiCastBar();
}

// VERIFY: d0cc9bd0 auto
// OFFSET: 0x005D95C0
static void MultiCastBarItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        WOWGUID none = WOWGUID();
        TriggerMultiCastBarUpdate(g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false));
    }
}

// VERIFY: 733aa4e0 auto
// OFFSET: 0x005D9600
int32_t UpdateContainerContents(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(guid, TYPE_CONTAINER, __FILE__, __LINE__));

    if (!container) {
        return 1;
    }

    uint32_t slot = (fieldOffset - offsetof(CGContainerData, m_containerFieldSlot1)) / sizeof(WOWGUID);
    CGBag_C* bag = container->GetBag();
    WOWGUID newGuid = slot < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[slot] : WOWGUID();
    const WOWGUID& oldGuid = *static_cast<const WOWGUID*>(oldValue);
    CGItem_C* oldItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(oldGuid, TYPE_ITEM, __FILE__, __LINE__));
    CGItem_C* newItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(newGuid, TYPE_ITEM, __FILE__, __LINE__));

    if (oldGuid != newGuid) {
        if (oldItem) {
            CGActionBar::UpdateItem(oldItem->m_obj->m_entryID);
        }

        if (newItem) {
            CGActionBar::UpdateItem(newItem->m_obj->m_entryID);
        }

        CGTradeSkillInfo::RefreshList(0);
    }

    CGGameUI::UnlockItem(newGuid);

    if (CGGameUI::m_autoStoreItem == oldGuid || CGGameUI::m_autoStoreItem == newGuid || (!CGGameUI::m_autoStoreItem && CGGameUI::m_autoStoreItemID)) {
        ItemRec* item = g_itemDB.GetRecord(CGGameUI::m_autoStoreItemID);

        if (item) {
            SI2::PlayItemSound(ITEMSOUND_DROP, item->m_displayInfoID);
        }
    }

    CGGameUI::m_autoStoreItem = WOWGUID();
    CGGameUI::m_autoStoreItemID = 0;
    CGContainerInfo::UpdateContents(guid);

    if (newItem) {
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(newItem->m_obj->m_entryID, none, MultiCastBarItemStatsCallback, nullptr, false);

        if (stats) {
            TriggerMultiCastBarUpdate(stats);
        }
    }

    return 1;
}

// VERIFY: d73cd982 manual
// OFFSET: 0x005D9790
int32_t UpdateInvContents(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    uint32_t slot = (fieldOffset - offsetof(CGPlayerLocalData, m_playerFieldInvSlotHead)) / sizeof(WOWGUID);
    WOWGUID newGuid = slot < static_cast<uint32_t>(player->m_bag.m_numSlots) ? player->m_bag.m_slots[slot] : WOWGUID();
    const WOWGUID& oldGuid = *static_cast<const WOWGUID*>(oldValue);
    CGItem_C* oldItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(oldGuid, TYPE_ITEM, __FILE__, __LINE__));
    CGItem_C* newItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(newGuid, TYPE_ITEM, __FILE__, __LINE__));

    if (oldGuid != newGuid) {
        if (oldItem) {
            CGActionBar::UpdateItem(oldItem->m_obj->m_entryID);
        }

        if (newItem) {
            CGActionBar::UpdateItem(newItem->m_obj->m_entryID);
        }

        CGTradeSkillInfo::RefreshList(0);
    }

    CGGameUI::UnlockItem(newGuid);

    if (CGGameUI::m_autoStoreItem == oldGuid || CGGameUI::m_autoStoreItem == newGuid || (!CGGameUI::m_autoStoreItem && CGGameUI::m_autoStoreItemID)) {
        ItemRec* item = g_itemDB.GetRecord(CGGameUI::m_autoStoreItemID);

        if (item) {
            SI2::PlayItemSound(ITEMSOUND_DROP, item->m_displayInfoID);
        }
    }

    CGGameUI::m_autoStoreItem = WOWGUID();
    CGGameUI::m_autoStoreItemID = 0;
    CGContainerInfo::UpdateContents(guid);

    if ((oldItem && oldItem->GetClassID() == 10) || (newItem && newItem->GetClassID() == 10) || slot - CURRENCYTOKEN_FIRST <= CURRENCYTOKEN_LAST - CURRENCYTOKEN_FIRST) {
        CGCurrencyTypes::EnterWorld();
    }

    if (newItem) {
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(newItem->m_obj->m_entryID, none, MultiCastBarItemStatsCallback, nullptr, false);

        if (stats) {
            TriggerMultiCastBarUpdate(stats);
        }
    }

    return 1;
}

// VERIFY: 6dd08087 manual
// OFFSET: 0x005D9960
void CGContainerInfo::UpdateContainers() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    for (uint32_t i = 0; i < TOTAL_BAG_SLOTS; i++) {
        uint32_t slot = i < 4 ? i + 19 : i + 63;
        WOWGUID guid = slot < static_cast<uint32_t>(player->m_bag.m_numSlots) ? player->m_bag.m_slots[slot] : WOWGUID();

        if (guid == CGContainerInfo::m_containers[i]) {
            continue;
        }

        if (CGContainerInfo::m_containers[i]) {
            CGContainer_C* old = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(CGContainerInfo::m_containers[i], TYPE_CONTAINER, __FILE__, __LINE__));

            if (old && old->GetBag()) {
                int32_t count = old->GetBag()->m_numSlots;

                for (int32_t j = 0; j < count; j++) {
                    ClntObjMgrUnsetObjMirrorHandler(CGContainerInfo::m_containers[i], ID_CONTAINER, offsetof(CGContainerData, m_containerFieldSlot1) + j * sizeof(WOWGUID), UpdateContainerContents, nullptr);
                }
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_BAG_CLOSED, "%d", i + 1);
        }

        CGContainerInfo::m_containers[i] = guid;

        if (guid) {
            CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(guid, TYPE_CONTAINER, __FILE__, __LINE__));

            if (container && container->GetBag()) {
                int32_t count = container->GetBag()->m_numSlots;

                for (int32_t j = 0; j < count; j++) {
                    ClntObjMgrSetObjMirrorHandler(guid, ID_CONTAINER, offsetof(CGContainerData, m_containerFieldSlot1) + j * sizeof(WOWGUID), sizeof(WOWGUID), UpdateContainerContents, nullptr, HANDLER_PRIORITY_NORMAL, 1);
                }
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_BAG_UPDATE, "%d", i + 1);
        }
    }
}

// VERIFY: 4c8ee089 manual
// OFFSET: 0x005D9B40
int32_t InvUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    uint32_t slot = (fieldOffset - offsetof(CGPlayerLocalData, m_playerFieldInvSlotHead)) / sizeof(WOWGUID);

    if (slot < static_cast<uint32_t>(player->m_bag.m_numSlots) && player->m_bag.m_slots[slot]) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(player->m_bag.m_slots[slot], TYPE_ITEM, __FILE__, __LINE__));

        if (item && (item->m_obj->m_type & TYPE_CONTAINER)) {
            CGBag_C* bag = item->GetBag();

            if (bag) {
                for (uint32_t i = 0; i < static_cast<uint32_t>(bag->m_numSlots); i++) {
                    CGItem_C* contained = bag->GetItemPointer(i);

                    if (contained) {
                        CGActionBar::UpdateItem(contained->m_obj->m_entryID);
                    }
                }
            }
        }
    }

    CGContainerInfo::UpdateContents(guid);
    CGContainerInfo::UpdateContainers();
    CGTradeSkillInfo::RefreshList(0);
    return 1;
}
