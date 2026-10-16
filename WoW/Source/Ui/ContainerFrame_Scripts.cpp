#include "ContainerFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Container_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/Ui/AuctionHouse.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GuildBankFrame.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Ui/ItemSocketInfo.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/Net/Types.h"

// VERIFY: ebcdce34 auto
// OFFSET: 0x005D6FF0
static int32_t Script_GetMaxArenaCurrency(lua_State* L) {
    lua_pushnumber(L, 10000.0);
    return 1;
}

// VERIFY: fb1b77a0 manual
// OFFSET: 0x005D6F60
static int32_t Script_ContainerIDToInventoryID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ContainerIDToInventoryID(containerID)");
        return 0;
    }

    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (id > 10) {
        luaL_error(L, "ContainerIDToInventoryID(): invalid container ID");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(id) >= 4 ? id + 64 : id + 20;
    lua_pushnumber(L, slot);
    return 1;
}

// VERIFY: 8c76374f manual
// OFFSET: 0x005D7590
static int32_t Script_GetContainerNumFreeSlots(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetContainerFreeSlots(index)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    double index = lua_tonumber(L, 1);

    if (index == 0.0 || (index == -1.0 && CGBankInfo::m_banker)) {
        uint32_t first = index == 0.0 ? 23 : 39;
        uint32_t last = index == 0.0 ? 38 : 66;
        int32_t free = 0;

        for (uint32_t slot = first; slot <= last; slot++) {
            if (slot >= static_cast<uint32_t>(player->m_bag.m_numSlots) || !player->m_bag.m_slots[slot]) {
                free++;
            }
        }

        lua_pushnumber(L, free);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    if (index != -1.0 && static_cast<uint32_t>(static_cast<int32_t>(index) - 1) <= 10) {
        CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(GetBagAtIndex(static_cast<int32_t>(index) - 1), TYPEMASK_CONTAINER, __FILE__, __LINE__));

        if (container) {
            lua_pushnumber(L, container->GetFreeSlots());
            lua_pushnumber(L, container->GetBagFamily());
            return 2;
        }
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: 59b9b306 manual
// OFFSET: 0x005D74A0
static int32_t Script_GetContainerNumSlots(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetContainerNumSlots(index)");
        return 0;
    }

    double slots = 0.0;

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

        if (index == -1) {
            slots = 16.0;
        } else if (index == -2) {
            slots = 28.0;
        } else if (index == -3 || index == -5) {
            slots = 32.0;
        } else if (static_cast<uint32_t>(index) <= 10) {
            CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(GetBagAtIndex(index), TYPEMASK_CONTAINER, __FILE__, __LINE__));

            if (container) {
                slots = container->GetBag()->m_numSlots;
            }
        }
    }

    lua_pushnumber(L, slots);
    return 1;
}

// VERIFY: 5491be82 auto
// OFFSET: 0x005D8C70
static int32_t Script_GetBagName(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetBagName(index)");
        return 0;
    }

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        double index = lua_tonumber(L, 1);

        if (index == 0.0) {
            lua_pushstring(L, FrameScript_GetText("BACKPACK_TOOLTIP", -1, GENDER_NOT_APPLICABLE));
            return 1;
        }

        if (static_cast<uint32_t>(static_cast<int32_t>(index) - 1) <= 10) {
            CGContainer_C* container = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(GetBagAtIndex(static_cast<int32_t>(index) - 1), TYPEMASK_CONTAINER, __FILE__, __LINE__));

            if (container) {
                ItemStats_C* stats = g_itemDBCache.GetRecord(container->m_obj->m_entryID, WOWGUID(), nullptr, nullptr, false);

                if (stats) {
                    lua_pushstring(L, stats->GetName(0));
                    return 1;
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 586c8464 manual
// OFFSET: 0x005D7A90
static int32_t Script_GetContainerItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemInfo(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    const char* path = ClientDBStringLookup(3);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
    lua_pushstring(L, texture);
    lua_pushnumber(L, item->m_item->m_itemFieldStackCount);

    if (item->m_itemFlags & 0x1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, WOWGUID(), nullptr, nullptr, false);

    if (stats && stats->m_inventoryType) {
        lua_pushnumber(L, stats->m_quality);
    } else {
        lua_pushnumber(L, -1.0);
    }

    if (item->GetPageTextID(nullptr) || (item->m_item->m_itemFieldFlags & 0x200)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (item->GetItemStaticFlags(0) & 0x4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
    return 7;
}

// VERIFY: 2cd3f9c3 manual
// OFFSET: 0x005D7D90
static int32_t Script_GetContainerItemCooldown(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemCooldown(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;
    CGItem_C* item;

    if (CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank) && (item = bag->GetItemPointer(slot)) && (!item->GetUsableSpellCharges() || item->GetCurrentSpellCharges() || (item->GetUsableSpellCharges() < 0 && !item->GetCurrentSpellCharges()))) {
        uint32_t start = 0;
        uint32_t duration = 0;
        uint32_t enable = 0;
        Spell_C_GetItemCooldown(item, &duration, &start, &enable);
        lua_pushnumber(L, start * 0.001);
        lua_pushnumber(L, duration * 0.001);
        lua_pushnumber(L, enable);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 3;
}

// VERIFY: 0244abbb manual
// OFFSET: 0x005D8650
static int32_t Script_UseContainerItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: UseContainerItem(index, slot[, target])");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    WOWGUID target;

    if (!Script_GetGUIDFromToken(lua_tolstring(L, 3, nullptr), target, true)) {
        return 0;
    }

    CGGameUI::ClearCursor(1, 1);
    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item || (item->m_itemFlags & 1)) {
        return 0;
    }

    WOWGUID guid = item->m_obj->m_guid;
    int32_t itemId = item->m_obj->m_entryID;

    if (Spell_C_IsTargeting() && !Spell_C_CanTargetItems()) {
        Spell_C_StopTargeting();
    }

    if (Spell_C_IsTargeting()) {
        CGGameUI::CastItemSpell(guid, 0);
        return 0;
    }

    if (CursorGetResetMode() == 17 && CGMerchantInfo::CanRepair()) {
        uint32_t cost = CGMerchantInfo::GetRepairCost(item);

        if (cost <= player->GetCoinage()) {
            SI2::PlaySoundKit("ITEM_REPAIR", nullptr, nullptr, nullptr);
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_REPAIR_ITEM));
            msg.Put(CGMerchantInfo::m_merchant.m_guid);
            msg.Put(guid.m_guid);
            msg.Put(static_cast<uint8_t>(0));
            msg.Finalize();
            ClientServices::Send2(&msg);
        } else {
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        }

        return 0;
    }

    if (bank) {
        player->AutoStoreBankItem(bag->m_guid, slot);
        CGGameUI::SetPendingItemSound(guid, itemId);
        CGGameUI::LockItem(guid);
        return 0;
    }

    if (CGBankInfo::m_banker) {
        player->AutoBankItem(bag->m_guid, slot);
        CGGameUI::SetPendingItemSound(guid, itemId);
        CGGameUI::LockItem(guid);
        return 0;
    }

    if (CGMerchantInfo::m_merchant) {
        player->SellItem(CGMerchantInfo::m_merchant, guid, 0);
        CGGameUI::LockItem(guid);
        return 0;
    }

    if (CGMailInfo::m_sendMailShowing && CGMailInfo::m_object) {
        CGMailInfo::AddItemToOutgoingMail(-1, guid, bag->m_guid, slot);
        return 0;
    }

    if (CGAuctionHouse::m_auctionsTabShowing && CGAuctionHouse::m_auctioneer) {
        if (!CGAuctionHouse::IsSellItemValid(item, 1)) {
            return 0;
        }

        WOWGUID sellItem;
        WOWGUID sellContainer;
        uint8_t sellSlot;
        CGAuctionHouse::GetSellItem(&sellItem, &sellContainer, &sellSlot);

        if (sellItem) {
            CGGameUI::UnlockItem(sellItem);
        }

        CGGameUI::LockItem(guid);
        CGAuctionHouse::SetSellItem(guid, bag->m_guid, slot);
        FrameScript_SignalEvent(SCRIPT_EVENT_NEW_AUCTION_UPDATE, nullptr);
        return 0;
    }

    if (CGTradeInfo::m_tradingPlayer) {
        int32_t enchant = item->GetEnchantmentSpell(53) || item->GetEnchantmentSpell(54) || item->GetEnchantmentSpell(156) || item->GetEnchantmentSpell(33);

        if (CGTradeInfo::m_enchantingItem && enchant) {
            CGTradeInfo::SetEnchantingItem(item);
        } else {
            CGTradeInfo::AddPlayerItem(guid, bag->m_guid, slot);
        }

        return 0;
    }

    if (CGGuildBankInfo::m_banker) {
        CGGuildBankInfo::AutoStoreItem(guid, bag->m_guid, slot);
        CGGameUI::SetPendingItemSound(guid, itemId);
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (item->GetInventoryType() && (!stats || !stats->m_startQuest)) {
        if (CGGameUI::m_hasControl || (player->m_unit->m_unitFieldFlags & 0x100000)) {
            player->AutoEquipItem(bag->m_guid, slot, 0);
        }

        return 0;
    }

    if (stats && stats->m_class == 16) {
        FrameScript_SignalEvent(SCRIPT_EVENT_USE_GLYPH, nullptr);
    }

    item->UseItem(&target, 0);
    return 0;
}

// VERIFY: 256ccaab manual
// OFFSET: 0x005D7FF0
int32_t Script_PickupContainerItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: PickupContainerItem(index, slot)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!player || (!CGGameUI::m_hasControl && !(player->m_unit->m_unitFieldFlags & 0x100000)) || !CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    if (bag->m_hasBank && ((slot >= 39 && slot <= 66) || static_cast<uint32_t>(slot - 67) <= 6) && !CGBankInfo::m_banker) {
        return 0;
    }

    WOWGUID cursorItem;
    WOWGUID cursorContainer;
    uint32_t cursorSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
    int32_t virtualId;
    int32_t virtualSlot;
    CGGameUI::GetCursorVirtualItem(virtualId, virtualSlot);
    CGItem_C* item = bag->GetItemPointer(slot);
    WOWGUID itemGuid = item ? item->m_obj->m_guid : WOWGUID();

    if (cursorItem) {
        if (itemGuid == cursorItem) {
            CGGameUI::ClearCursor(1, 1);
            return 0;
        }

        if (CGGameUI::m_stackSplit) {
            player->SplitItem(cursorItem, cursorContainer, cursorSlot, bag->m_guid, slot, CGGameUI::m_stackSplit);
            CGGameUI::ClearCursor(0, 1);
        } else if ((!itemGuid || cursorContainer) && (!item || !(item->m_itemFlags & 1))) {
            CGGameUI::LockItem(itemGuid);
            player->SwapItems(cursorItem, cursorContainer, cursorSlot, bag->m_guid, slot, 0);
        }

        return 0;
    }

    if (virtualId) {
        switch (CGGameUI::m_cursorItemType) {
            case 6:
                player->PutLootInSlot(bag->m_guid, slot, virtualSlot);
                CGGameUI::ClearCursor(1, 1);
                return 0;

            case 5:
                if (CGMerchantInfo::m_merchant) {
                    CGPlayer_C::BuyItemInSlot(CGMerchantInfo::m_merchant, CGMerchantInfo::GetItem(virtualSlot), 1, bag->m_guid, slot);
                    CGGameUI::ClearCursor(1, 1);
                }

                return 0;

            case 11: {
                if (!CGGuildBankInfo::m_banker) {
                    return 0;
                }

                int32_t bankItemId = 0;
                int32_t bankSlot = 0;
                CGGameUI::GetCursorVirtualItem(bankItemId, bankSlot);

                if (!virtualId) {
                    return 0;
                }

                uint8_t bagIndex = player->FindSlotIndex(bag->m_guid);
                uint32_t count = 0;
                WOWGUID none = WOWGUID();
                ItemStats_C* stats = g_itemDBCache.GetRecord(bankItemId, none, nullptr, nullptr, false);
                int32_t itemId = item ? item->m_obj->m_entryID : 0;

                if (stats && stats->m_stackable) {
                    int32_t maxStack = stats->m_stackable;

                    if (!itemId) {
                        count = CGGameUI::m_stackSplit;
                    } else if (bankItemId == itemId) {
                        int32_t room = maxStack - item->m_item->m_itemFieldStackCount;

                        if (CGGameUI::m_stackSplit) {
                            count = CGGameUI::m_stackSplit < room ? CGGameUI::m_stackSplit : room;
                        } else {
                            int32_t available = CGGameUI::GetVirtualItemCount(bankItemId, bankSlot, 0);
                            count = available < room ? available : room;
                        }
                    } else if (virtualId == itemId) {
                        CGGameUI::ClearCursor(1, 1);
                        CGGameUI::UnlockVirtualItem(bankItemId, bankSlot, 0);
                        return 0;
                    }
                } else if (virtualId == itemId) {
                    CGGameUI::ClearCursor(1, 1);
                    CGGameUI::UnlockVirtualItem(bankItemId, bankSlot, 0);
                    return 0;
                }

                player->SwapGuildBankItem(bankItemId, bankSlot, bagIndex, slot, count);
                break;
            }

            default:
                break;
        }

        CGGameUI::ClearCursor(1, 1);
        return 0;
    }

    if (!item) {
        return 0;
    }

    if (Spell_C_IsTargeting()) {
        Spell_C_StopTargeting();
    }

    if (CursorGetResetMode() == 17 && CGMerchantInfo::CanRepair()) {
        uint32_t cost = CGMerchantInfo::GetRepairCost(item);

        if (cost <= player->GetCoinage()) {
            SI2::PlaySoundKit("ITEM_REPAIR", nullptr, nullptr, nullptr);
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_REPAIR_ITEM));
            msg.Put(CGMerchantInfo::m_merchant.m_guid);
            msg.Put(itemGuid.m_guid);
            msg.Put(static_cast<uint8_t>(0));
            msg.Finalize();
            ClientServices::Send2(&msg);
        } else {
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        }

        return 0;
    }

    if (CGPlayer_C::IsGiftWrapping()) {
        CGPlayer_C* activePlayer = ClntObjMgrGetActivePlayerObj();

        if (activePlayer) {
            activePlayer->GiftWrap(item);
        }

        return 0;
    }

    if (!(item->m_itemFlags & 1)) {
        CGGameUI::SetCursorItem(itemGuid, bag->m_guid, slot, 1, 0, 1);
        CGGameUI::LockItem(itemGuid);
    }

    return 0;
}

// VERIFY: 6deefa52 auto
// OFFSET: 0x005D8BD0
static int32_t Script_ShowContainerSellCursor(lua_State* L) {
    if (Spell_C_IsTargeting() || CursorGetResetMode() != 1) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: ShowContainerSellCursor(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item || (item->m_itemFlags & 1)) {
        return 0;
    }

    CursorSetMode(3);
    return 0;
}

// VERIFY: 13022947 manual
// OFFSET: 0x005D7180
static int32_t Script_SetBagPortraitTexture(lua_State* L) {
    CSimpleTexture* texture = static_cast<CSimpleTexture*>(FrameScript_GetObjectThis(L, CSimpleTexture::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetBagPortraitTexture(texture, slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;

    if (slot < 0) {
        return 0;
    }

    if (slot >= 11) {
        luaL_error(L, "Invalid slot in SetBagPortraitTexture");
        return 0;
    }

    texture->SetTexture(static_cast<CTexture*>(nullptr));
    CGItem_C* bag = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(GetBagAtIndex(slot), TYPEMASK_CONTAINER, __FILE__, __LINE__));

    if (!bag) {
        return 0;
    }

    const char* path = ClientDBStringLookup(3);
    char fileName[260];
    SStrPrintf(fileName, sizeof(fileName), "%s%s%s", path, *path ? "\\" : "", bag->GetInventoryArt());
    SetPortraitTexture(texture, fileName);
    return 0;
}

// VERIFY: 97522acf manual
// OFFSET: 0x005D9400
static int32_t Script_GetContainerItemQuestInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemQuestInfo(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;
    CGItem_C* item;
    ItemStats_C* stats;
    WOWGUID none = WOWGUID();

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank) || !(item = bag->GetItemPointer(slot)) || !(stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    if (stats->m_bonding == 4) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushnil(L);
    }

    if (stats->m_startQuest) {
        lua_pushnumber(L, stats->m_startQuest);
        lua_pushboolean(L, CGQuestLog::IsOnQuest(stats->m_startQuest));
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: d21762d0 manual
// OFFSET: 0x005D8D80
static int32_t Script_GetContainerItemPurchaseInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemPurchaseInfo(index, slot[, isEquipped])");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    CGItem_C* item;

    if (lua_tonumber(L, 3)) {
        CGBag_C* bag = &ClntObjMgrGetActivePlayerObj()->m_bag;
        uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2) - 1);

        if (!bag || slot > 18) {
            return 0;
        }

        item = bag->GetItemPointer(slot);
    } else {
        CGBag_C* bag;
        int32_t slot;
        int32_t bank;

        if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
            return 0;
        }

        item = bag->GetItemPointer(slot);
    }

    if (!item || CGLootInfo::m_lootObj == item->m_obj->m_guid) {
        return 0;
    }

    CGItem_C::ItemPurchaseData* data = item->m_itemPurchaseData;

    if (!data || item->HasEnchantments()) {
        return 0;
    }

    int32_t timeLeft = data->m_refundTimeLeft - player->GetPlayedTime() + 7200;

    if (timeLeft <= 0) {
        return 0;
    }

    int32_t itemCount = 0;

    for (uint32_t i = 0; i < 5; i++) {
        if (static_cast<int32_t>(data->m_itemCount[i]) > 0) {
            itemCount++;
        }
    }

    lua_pushnumber(L, data->m_money);
    lua_pushnumber(L, data->m_honorPoints);
    lua_pushnumber(L, data->m_arenaPoints);
    lua_pushnumber(L, itemCount);
    lua_pushnumber(L, timeLeft);
    return 5;
}

// VERIFY: 7da8b539 auto
// OFFSET: 0x005D7C80
static int32_t Script_GetContainerItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemLink(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
    return 1;
}

// VERIFY: c109decd manual
// OFFSET: 0x005D84F0
static int32_t Script_SplitContainerItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: SplitContainerItem(index, slot, amount)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!player || (!CGGameUI::m_hasControl && !(player->m_unit->m_unitFieldFlags & 0x100000)) || !CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);
    WOWGUID itemGuid = item ? item->m_obj->m_guid : WOWGUID();

    if (!item || (item->m_itemFlags & 1)) {
        return 0;
    }

    int32_t amount = static_cast<int32_t>(lua_tonumber(L, 3));

    if (amount < 1 || amount > item->m_item->m_itemFieldStackCount) {
        return 0;
    }

    CGGameUI::ClearCursor(1, 1);
    Spell_C_StopTargeting();

    if (amount == item->m_item->m_itemFieldStackCount) {
        CGGameUI::SetCursorItem(itemGuid, bag->m_guid, slot, 1, 0, 1);
    } else {
        CGGameUI::SetCursorItem(itemGuid, bag->m_guid, slot, 1, amount, 1);
    }

    CGGameUI::LockItem(itemGuid);
    return 0;
}

// VERIFY: 38975cf9 auto
// OFFSET: 0x005D8B10
static int32_t Script_SocketContainerItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SocketContainerItem(index, slot)");
        return 0;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item || (item->m_itemFlags & 1) || !item->GetNumSockets()) {
        return 0;
    }

    CGItemSocketInfo::SetItem(item->m_obj->m_guid);
    return 0;
}

// VERIFY: cc760869 manual
// OFFSET: 0x005D72A0
static bool BuildItemIconPath(int32_t itemId, char* buffer, uint32_t size) {
    if (!itemId) {
        return false;
    }

    ItemRec* rec = g_itemDB.GetRecord(itemId);

    if (!rec) {
        return false;
    }

    const char* path = ClientDBStringLookup(3);
    SStrPrintf(buffer, size, "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(rec->m_displayInfoID));
    return true;
}

// VERIFY: 8bb31b86 manual
// OFFSET: 0x005D7820
static int32_t Script_GetContainerFreeSlots(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetContainerFreeSlots(index [, returnTable])");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (lua_type(L, 2) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    uint32_t count = 1;
    double index = lua_tonumber(L, 1);

    if (index == 0.0) {
        for (int32_t i = 23; i <= 38; i++) {
            if (static_cast<uint32_t>(i) >= static_cast<uint32_t>(player->m_bag.m_numSlots) || !player->m_bag.m_slots[i]) {
                lua_checkstack(L, 2);
                lua_pushnumber(L, count);
                lua_pushnumber(L, i - 22);
                lua_settable(L, -3);
                count++;
            }
        }

        return 1;
    }

    if (index == -1.0) {
        if (!CGBankInfo::m_banker) {
            return 0;
        }

        for (int32_t i = 39; i <= 66; i++) {
            if (static_cast<uint32_t>(i) >= static_cast<uint32_t>(player->m_bag.m_numSlots) || !player->m_bag.m_slots[i]) {
                lua_checkstack(L, 2);
                lua_pushnumber(L, count);
                lua_pushnumber(L, i - 38);
                lua_settable(L, -3);
                count++;
            }
        }

        return 1;
    }

    if (static_cast<uint32_t>(index - 1.0) > 10) {
        return 0;
    }

    CGItem_C* container = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(GetBagAtIndex(static_cast<uint32_t>(index - 1.0)), TYPEMASK_CONTAINER, __FILE__, __LINE__));

    if (!container) {
        return 0;
    }

    CGBag_C* bag = container->GetBag();

    if (!bag) {
        return 0;
    }

    for (uint32_t slot = 0; slot < static_cast<uint32_t>(bag->m_numSlots); slot++) {
        if (!bag->GetItemGuid(slot)) {
            lua_checkstack(L, 2);
            lua_pushnumber(L, count);
            lua_pushnumber(L, slot + 1);
            lua_settable(L, -3);
            count++;
        }
    }

    return 1;
}

// VERIFY: 11b93c9c manual
// OFFSET: 0x005D7D00
static int32_t Script_GetContainerItemID(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemID(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    lua_pushnumber(L, item->m_obj->m_entryID);
    return 1;
}

// VERIFY: c4bf89e8 manual
// OFFSET: 0x005D7EF0
static int32_t Script_GetContainerItemDurability(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemDurability(index, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        return 0;
    }

    CGItem_C* item = bag->GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    uint32_t maxDurability = (item->m_item->m_itemFieldFlags & 8) ? 0 : item->m_item->m_itemFieldMaxdurability;
    uint32_t durability = (item->m_item->m_itemFieldFlags & 8) ? 0 : item->m_item->m_itemFieldDurability;

    if (!maxDurability) {
        return 0;
    }

    lua_pushnumber(L, durability);
    lua_pushnumber(L, maxDurability);
    return 2;
}

// VERIFY: bc1a0153 manual
// OFFSET: 0x005D8F70
static int32_t Script_GetContainerItemPurchaseItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: ContainerItemPurchaseItem(bag, slot, itemIndex[, isEquipped])");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    CGItem_C* item;

    if (lua_tonumber(L, 4)) {
        CGBag_C* bag = &ClntObjMgrGetActivePlayerObj()->m_bag;
        uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2) - 1);

        if (!bag || slot > 18) {
            return 0;
        }

        item = bag->GetItemPointer(slot);
    } else {
        CGBag_C* bag;
        int32_t slot;
        int32_t bank;

        if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
            return 0;
        }

        item = bag->GetItemPointer(slot);
    }

    if (!item || CGLootInfo::m_lootObj == item->m_obj->m_guid) {
        return 0;
    }

    CGItem_C::ItemPurchaseData* data = item->m_itemPurchaseData;

    if (!data || item->HasEnchantments() || static_cast<int32_t>(data->m_refundTimeLeft + 7200 - player->GetPlayedTime()) <= 0) {
        return 0;
    }

    int32_t wanted = static_cast<int32_t>(lua_tonumber(L, 3)) - 1;
    int32_t found = 0;
    uint32_t i;

    for (i = 0; i < 5; i++) {
        if (static_cast<int32_t>(data->m_itemId[i]) <= 0) {
            continue;
        }

        if (found == wanted) {
            break;
        }

        found++;
    }

    if (i >= 5) {
        return 0;
    }

    char texture[260];

    if (BuildItemIconPath(data->m_itemId[i], texture, sizeof(texture))) {
        lua_pushstring(L, texture);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, data->m_itemCount[i]);
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(data->m_itemId[i], none, nullptr, nullptr, true);

    if (stats) {
        lua_pushstring(L, CGTooltip::GetItemHyperlinkString(data->m_itemId[i], stats->m_quality, 0, 0, 0));
    } else {
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: a90b94c4 manual
// OFFSET: 0x005D91B0
static int32_t Script_ContainerRefundItemPurchase(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: ContainerRefundItemPurchase(index, slot[, isEquipped])");
        return 0;
    }

    CGItem_C* item;

    if (static_cast<int32_t>(lua_tonumber(L, 3))) {
        CGBag_C* bag = &ClntObjMgrGetActivePlayerObj()->m_bag;
        uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2) - 1);

        if (!bag || slot > 18) {
            CGGameUI::DisplayError(ERR_INTERNAL_BAG_ERROR);
            return 0;
        }

        item = bag->GetItemPointer(slot);
    } else {
        CGBag_C* bag;
        int32_t slot;
        int32_t bank;

        if (!CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
            CGGameUI::DisplayError(ERR_INTERNAL_BAG_ERROR);
            return 0;
        }

        item = bag->GetItemPointer(slot);
    }

    if (item && item->m_itemPurchaseData && CGMerchantInfo::m_merchant) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_ITEM_PURCHASE_REFUND));
        msg.Put(item->m_obj->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
        return 0;
    }

    CGGameUI::DisplayError(ERR_INTERNAL_BAG_ERROR);
    return 0;
}

// VERIFY: 2ff417f7 manual
// OFFSET: 0x005D9300
static int32_t Script_GetContainerItemGems(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetContainerItemGems(bag, slot)");
        return 0;
    }

    CGBag_C* bag;
    int32_t slot;
    int32_t bank;

    if (CGContainerInfo::GetLuaBagAndSlot(L, &bag, &slot, &bank)) {
        CGItem_C* item = bag->GetItemPointer(slot);

        if (item) {
            for (int32_t i = 0; i < 3; i++) {
                int32_t enchantId = (item->GetItemStaticFlags(0) & 0x2000) ? 0 : item->m_item->m_itemFieldEnchantment[2 + i].m_id;
                SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(enchantId);

                if (enchant && enchant->m_srcItemID) {
                    lua_pushnumber(L, enchant->m_srcItemID);
                } else {
                    lua_pushnil(L);
                }
            }
        }
    }

    return 3;
}

// STATIC: 0x00AD0D50
static FrameScript_Method s_ScriptFunctions[] = {
    { "ContainerIDToInventoryID", &Script_ContainerIDToInventoryID },
    { "GetContainerNumSlots", &Script_GetContainerNumSlots },
    { "GetContainerItemInfo", &Script_GetContainerItemInfo },
    { "GetContainerItemID", &Script_GetContainerItemID },
    { "GetContainerItemLink", &Script_GetContainerItemLink },
    { "GetContainerItemCooldown", &Script_GetContainerItemCooldown },
    { "PickupContainerItem", &Script_PickupContainerItem },
    { "SplitContainerItem", &Script_SplitContainerItem },
    { "UseContainerItem", &Script_UseContainerItem },
    { "SocketContainerItem", &Script_SocketContainerItem },
    { "ShowContainerSellCursor", &Script_ShowContainerSellCursor },
    { "SetBagPortraitTexture", &Script_SetBagPortraitTexture },
    { "GetBagName", &Script_GetBagName },
    { "GetContainerItemDurability", &Script_GetContainerItemDurability },
    { "GetContainerNumFreeSlots", &Script_GetContainerNumFreeSlots },
    { "GetContainerFreeSlots", &Script_GetContainerFreeSlots },
    { "GetContainerItemPurchaseInfo", &Script_GetContainerItemPurchaseInfo },
    { "GetContainerItemPurchaseItem", &Script_GetContainerItemPurchaseItem },
    { "ContainerRefundItemPurchase", &Script_ContainerRefundItemPurchase },
    { "GetMaxArenaCurrency", &Script_GetMaxArenaCurrency },
    { "GetContainerItemGems", &Script_GetContainerItemGems },
    { "GetContainerItemQuestInfo", &Script_GetContainerItemQuestInfo },
};

// VERIFY: 1248b250 auto
// OFFSET: 0x005D7020
void ContainerRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 372ff3b8 auto
// OFFSET: 0x005D7050
void ContainerUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
