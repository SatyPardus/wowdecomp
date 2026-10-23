#include "MerchantFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/ItemExtendedCostRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Ui/ArenaTeamInfo.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// VERIFY: 32f102ca auto
// OFFSET: 0x005841D0
static int32_t Script_GetMerchantNumItems(lua_State* L) {
    lua_pushnumber(L, CGMerchantInfo::m_numMerchantItems);
    return 1;
}

// VERIFY: c2f7524a manual
// OFFSET: 0x005843F0
static int32_t Script_GetNumBuybackItems(lua_State* L) {
    if (CGMerchantInfo::m_merchant) {
        lua_pushnumber(L, CGMerchantInfo::m_numBuybackItems);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: c986aee1 manual
// OFFSET: 0x005849F0
static int32_t Script_CanMerchantRepair(lua_State* L) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGMerchantInfo::m_merchant, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && (unit->m_unit->m_unitNpcFlags & 0x1000)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 9422b5ea auto
// OFFSET: 0x005843B0
static int32_t Script_InRepairMode(lua_State* L) {
    if (CursorGetResetMode() == REPAIR_CURSOR) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 4a349e26 auto
// OFFSET: 0x005842D0
static int32_t Script_GetMerchantItemMaxStack(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMerchantItemMaxStack(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MerchantItem* item = index >= 0 && index < CGMerchantInfo::m_numMerchantItems && CGMerchantInfo::m_merchant ? &CGMerchantInfo::m_items[index] : nullptr;
    ItemStats_C* stats;
    double maxStack;

    if (item && item->m_buyCount <= 1 && item->m_itemID && (stats = g_itemDBCache.GetRecord(item->m_itemID, WOWGUID(), nullptr, nullptr, false))) {
        maxStack = stats->m_stackable;
    } else {
        maxStack = 1.0;
    }

    lua_pushnumber(L, maxStack);
    return 1;
}

// VERIFY: 22eb2eec auto
// OFFSET: 0x005846E0
static int32_t Script_GetMerchantItemCostInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMerchantItemCostInfo(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MerchantItem* item = index >= 0 && index < CGMerchantInfo::m_numMerchantItems && CGMerchantInfo::m_merchant ? &CGMerchantInfo::m_items[index] : nullptr;
    ItemExtendedCostRec* cost;
    double numItems;

    if (item && item->m_itemID && (cost = g_itemExtendedCostDB.GetRecord(item->m_extendedCostID))) {
        lua_pushnumber(L, cost->m_honorPoints);
        lua_pushnumber(L, cost->m_arenaPoints);
        uint32_t count = cost->m_itemID[0] != 0;

        if (cost->m_itemID[1]) {
            count++;
        }

        if (cost->m_itemID[2]) {
            count++;
        }

        if (cost->m_itemID[3]) {
            count++;
        }

        if (cost->m_itemID[4]) {
            count++;
        }

        numItems = count;
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        numItems = 0.0;
    }

    lua_pushnumber(L, numItems);
    return 3;
}

// VERIFY: 7a7ca74d auto
// OFFSET: 0x005840B0
static void BuybackItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_UPDATE, nullptr);
}

// VERIFY: 77dd56c8 auto
// OFFSET: 0x00584200
static int32_t Script_GetMerchantItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetMerchantItemLink(index)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= CGMerchantInfo::m_numMerchantItems || !CGMerchantInfo::m_merchant) {
        return 0;
    }

    MerchantItem* item = &CGMerchantInfo::m_items[index];

    if (!item || !item->m_itemID) {
        return 0;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_itemID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item->m_itemID, stats->m_quality, 0, 0, 0));
    return 1;
}

// VERIFY: ffebd45e auto
// OFFSET: 0x00584390
static int32_t Script_HideRepairCursor(lua_State* L) {
    if (CursorGetResetMode() == REPAIR_CURSOR) {
        CursorSetResetMode(POINT_CURSOR);
        CursorSetMode(POINT_CURSOR);
    }

    return 0;
}

// VERIFY: a936bd94 auto
// OFFSET: 0x005846D0
static int32_t Script_CloseMerchant(lua_State* L) {
    CGMerchantInfo::CloseMerchant();
    return 0;
}

// VERIFY: 64f4cf8a auto
// OFFSET: 0x00584820
static int32_t Script_GetMerchantItemCostItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        return luaL_error(L, "Usage: GetMerchantItemCostItem(index, itemIndex)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t costIndex = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    MerchantItem* item;
    ItemExtendedCostRec* cost;

    if (index < 0 || index >= CGMerchantInfo::m_numMerchantItems || !CGMerchantInfo::m_merchant || !(item = &CGMerchantInfo::m_items[index]) || !item->m_itemID || !(cost = g_itemExtendedCostDB.GetRecord(item->m_extendedCostID))) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    int32_t found = 0;
    uint32_t i;

    for (i = 0; i < 5; i++) {
        if (!cost->m_itemID[i]) {
            continue;
        }

        if (found == costIndex) {
            break;
        }

        found++;
    }

    if (i >= 5) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(cost->m_itemID[i], none, BuybackItemStatsCallback, nullptr, true);

    if (!stats) {
        lua_pushnil(L);
        lua_pushnumber(L, cost->m_itemCount[i]);
        return 2;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);
    lua_pushnumber(L, cost->m_itemCount[i]);
    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(cost->m_itemID[i], stats->m_quality, 0, 0, 0));
    return 3;
}

// VERIFY: 93965602 manual
// OFFSET: 0x00584A60
static int32_t Script_ShowRepairCursor(lua_State* L) {
    if (Spell_C_IsTargeting()) {
        return 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGMerchantInfo::m_merchant, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && (unit->m_unit->m_unitNpcFlags & 0x1000)) {
        CGGameUI::ClearCursor(1, 1);
        CursorSetResetMode(REPAIR_CURSOR);
        CursorSetMode(REPAIR_CURSOR);
    }

    return 0;
}

// VERIFY: bc45f8d1 manual
// OFFSET: 0x00584E10
static int32_t Script_GetMerchantItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetMerchantItemInfo(index)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MerchantItem* item;

    if (index < 0 || index >= CGMerchantInfo::m_numMerchantItems || !CGMerchantInfo::m_merchant || !(item = &CGMerchantInfo::m_items[index]) || !item->m_itemID) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 1.0);
        lua_pushnil(L);
        return 7;
    }

    ItemStats_C* stats = CGMerchantInfo::GetItemStats(item->m_itemID);

    if (stats) {
        lua_pushstring(L, stats->GetName(0));
    } else {
        lua_pushnil(L);
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(item->m_displayID));
    lua_pushstring(L, texture);
    lua_pushnumber(L, item->m_price);
    lua_pushnumber(L, item->m_buyCount);
    lua_pushnumber(L, item->m_maxCount);

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    ItemExtendedCostRec* cost = g_itemExtendedCostDB.GetRecord(item->m_extendedCostID);
    bool affordable = false;

    if (!cost || !cost->m_requiredArenaRating || (static_cast<uint32_t>(cost->m_arenaBracket) <= 2 && CGArenaTeamInfo::CheckPersonalAndTeamRating(cost->m_requiredArenaRating, cost->m_arenaBracket))) {
        affordable = true;
    }

    int32_t error;

    if ((player && stats && !player->CanUseItem(stats, error)) || !affordable) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    if (cost) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 7;
}

// VERIFY: a814f0b1 manual
// OFFSET: 0x00585070
static int32_t Script_GetBuybackItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetBuybackItemInfo(slot)");
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t index = static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1);
    uint32_t slot = index < CGMerchantInfo::m_numBuybackItems ? CGMerchantInfo::m_buybackSlots[index] : 0;
    CGItem_C* item;
    ItemStats_C* stats;
    WOWGUID none = { 0 };

    if (player && slot && slot < static_cast<uint32_t>(player->m_bag.m_numSlots) && player->m_bag.m_slots[slot] && (item = player->m_bag.GetItemPointer(slot)) && (stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, BuybackItemStatsCallback, nullptr, false))) {
        char buffer[260];
        item->GetFullItemName(buffer, sizeof(buffer));
        lua_pushstring(L, buffer);

        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
        lua_pushstring(L, buffer);
        lua_pushnumber(L, player->GetBuybackPrice(slot - 74));
        lua_pushnumber(L, item->m_item->m_itemFieldStackCount);
        lua_pushnumber(L, 0.0);

        int32_t error;

        if (!player->CanUseItem(stats, error)) {
            lua_pushnil(L);
            return 6;
        }
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, 0.0);
    }

    lua_pushnumber(L, 1.0);
    return 6;
}

// VERIFY: c832bd12 manual
// OFFSET: 0x005852C0
static int32_t Script_GetBuybackItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetBuybackItemLink(slot)");
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t slot = index < CGMerchantInfo::m_numBuybackItems ? CGMerchantInfo::m_buybackSlots[index] : 0;

    if (!player || !slot || slot >= static_cast<uint32_t>(player->m_bag.m_numSlots) || !player->m_bag.m_slots[slot]) {
        return 0;
    }

    CGItem_C* item = player->m_bag.GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
    return 1;
}

// VERIFY: 3a995819 manual
// OFFSET: 0x005853A0
static int32_t Script_PickupMerchantItem(lua_State* L) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    WOWGUID cursorItem = CGGameUI::GetCursorItem();

    if (cursorItem) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(cursorItem, TYPE_ITEM, __FILE__, __LINE__));

        if (!item || !CGGameUI::m_stackSplit || item->m_item->m_itemFieldStackCount == CGGameUI::m_stackSplit) {
            CGPlayer_C::SellItem(CGMerchantInfo::m_merchant, cursorItem, CGGameUI::m_stackSplit);
            CGGameUI::ClearCursor(0, 1);
        }

        return 0;
    }

    int32_t index;
    MerchantItem* item;

    if (!lua_isnumber(L, 1) || !(item = CGMerchantInfo::GetItem(index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1)) || !item->m_slot) {
        CGGameUI::ClearCursor(1, 1);
        return 0;
    }

    int32_t itemId = item->m_itemID;

    if (CGGameUI::GetCursorVirtualItem() == itemId) {
        CGGameUI::ClearCursor(1, 1);
    } else {
        CGGameUI::SetCursorVirtualItem(itemId, item->m_displayID, index, UICURSOR_MERCHANT, 0);
    }

    return 0;
}

// VERIFY: 1597625c manual
// OFFSET: 0x005854C0
static int32_t Script_BuyMerchantItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: BuyMerchantItem(index)");
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    CGGameUI::ClearCursor(1, 1);
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t count = 1;

    if (lua_isnumber(L, 2) && static_cast<int32_t>(lua_tonumber(L, 2)) >= 1) {
        count = static_cast<int32_t>(lua_tonumber(L, 2));

        if (count >= 255) {
            count = 255;
        }
    }

    if (index < 0 || index >= CGMerchantInfo::m_numMerchantItems || !CGMerchantInfo::m_merchant) {
        return 0;
    }

    MerchantItem* item = &CGMerchantInfo::m_items[index];

    if (item && item->m_slot) {
        CGPlayer_C::BuyItem(CGMerchantInfo::m_merchant, item, static_cast<uint8_t>(count), 1);
    }

    return 0;
}

// VERIFY: e5e81d26 manual
// OFFSET: 0x005855C0
static int32_t Script_BuybackItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: BuybackItem(slot)");
    }

    WOWGUID merchant = CGMerchantInfo::m_merchant;

    if (!merchant) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1);
    uint32_t slot = index < CGMerchantInfo::m_numBuybackItems ? CGMerchantInfo::m_buybackSlots[index] : 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && slot && player->m_bag.GetItemGuid(slot)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_BUY_BACK_ITEM));
        msg.Put(merchant.m_guid);
        msg.Put(slot);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 7ad42e00 manual
// OFFSET: 0x005856D0
static int32_t Script_ShowMerchantSellCursor(lua_State* L) {
    if (Spell_C_IsTargeting() || CursorGetResetMode() != POINT_CURSOR) {
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: ShowMerchantSellCursor(index)");
    }

    MerchantItem* item = CGMerchantInfo::GetItem(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);

    if (!item || !item->m_itemID) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (player->GetCoinage() < static_cast<uint32_t>(item->m_price)) {
        CursorSetMode(BUY_ERROR_CURSOR);
        return 0;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_itemID, none, nullptr, nullptr, false);

    if (!stats) {
        CursorSetMode(BUY_CURSOR);
        return 0;
    }

    if (stats->m_requiredReputationFaction && GetRepListRepValue(stats->m_requiredReputationFaction) < CGReputationInfo::m_reactionThresholds[stats->m_requiredReputationRank]) {
        CursorSetMode(BUY_ERROR_CURSOR);
        return 0;
    }

    ItemExtendedCostRec* cost = g_itemExtendedCostDB.GetRecord(item->m_extendedCostID);

    if (!cost) {
        CursorSetMode(BUY_CURSOR);
        return 0;
    }

    int32_t unable = 0;

    if (player->m_player->m_playerFieldArenaCurrency < static_cast<uint32_t>(cost->m_arenaPoints) || player->m_player->m_playerFieldHonorCurrency < static_cast<uint32_t>(cost->m_honorPoints)) {
        unable = 1;
    } else {
        for (uint32_t i = 0; i < 5; i++) {
            if (cost->m_itemID[i] && cost->m_itemCount[i] > 0 && player->m_bag.GetItemTypeCount(cost->m_itemID[i], 0) < cost->m_itemCount[i]) {
                unable = 1;
                break;
            }
        }
    }

    if (cost->m_requiredArenaRating && static_cast<uint32_t>(cost->m_arenaBracket) <= 2 && !CGArenaTeamInfo::CheckPersonalAndTeamRating(cost->m_requiredArenaRating, cost->m_arenaBracket)) {
        unable = 1;
    }

    CursorSetMode(unable ? BUY_ERROR_CURSOR : BUY_CURSOR);
    return 0;
}

// VERIFY: 26f78e7c manual
// OFFSET: 0x00585890
static int32_t Script_ShowBuybackSellCursor(lua_State* L) {
    if (Spell_C_IsTargeting() || CursorGetResetMode() != POINT_CURSOR) {
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: ShowBuybackSellCursor(index)");
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t slot = index < CGMerchantInfo::m_numBuybackItems ? CGMerchantInfo::m_buybackSlots[index] : 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && slot && player->m_bag.GetItemGuid(slot)) {
        uint32_t price = player->GetBuybackPrice(slot - 74);

        if (player->GetCoinage() >= price) {
            CursorSetMode(BUY_CURSOR);
            return 0;
        }

        CursorSetMode(BUY_ERROR_CURSOR);
    }

    return 0;
}

// VERIFY: 207bdc12 manual
// OFFSET: 0x00585990
static int32_t Script_GetRepairAllCost(lua_State* L) {
    CGUnit_C* merchant = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGMerchantInfo::m_merchant, TYPE_UNIT, __FILE__, __LINE__));
    CGPlayer_C* player;

    if (!merchant || !(merchant->m_unit->m_unitNpcFlags & 0x1000) || !(player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)))) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    CGBag_C* bag = &player->m_bag;
    uint32_t cost = 0;
    int32_t needsRepair = 0;

    for (uint32_t i = 0; i <= 18; i++) {
        CGItem_C* item = bag->GetItemPointer(i);

        if (item && (item->m_obj->m_type & TYPE_ITEM) && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
            cost += CGMerchantInfo::GetRepairCost(item);
            uint32_t durability = (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) ? 0 : item->m_item->m_itemFieldDurability;

            if (!(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && durability < item->m_item->m_itemFieldMaxdurability) {
                needsRepair = 1;
            }
        }
    }

    for (uint32_t i = 23; i <= 38; i++) {
        CGItem_C* item = bag->GetItemPointer(i);

        if (item && (item->m_obj->m_type & TYPE_ITEM) && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
            cost += CGMerchantInfo::GetRepairCost(item);
            uint32_t durability = (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) ? 0 : item->m_item->m_itemFieldDurability;

            if (!(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && durability < item->m_item->m_itemFieldMaxdurability) {
                needsRepair = 1;
            }
        }
    }

    for (uint32_t i = 19; i <= 22; i++) {
        CGItem_C* container = bag->GetItemPointer(i);

        if (!container || !container->GetBag()) {
            continue;
        }

        CGBag_C* containerBag = container->GetBag();

        for (uint32_t j = 0; j < static_cast<uint32_t>(containerBag->m_numSlots); j++) {
            CGItem_C* item = containerBag->GetItemPointer(j);

            if (item && (item->m_obj->m_type & TYPE_ITEM) && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
                cost += CGMerchantInfo::GetRepairCost(item);
                uint32_t durability = (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) ? 0 : item->m_item->m_itemFieldDurability;

                if (!(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && durability < item->m_item->m_itemFieldMaxdurability) {
                    needsRepair = 1;
                }
            }
        }
    }

    lua_pushnumber(L, cost);

    if (needsRepair) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: a91c8774 manual
// OFFSET: 0x00585C90
static int32_t Script_RepairAllItems(lua_State* L) {
    CGUnit_C* merchant = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGMerchantInfo::m_merchant, TYPE_UNIT, __FILE__, __LINE__));

    if (!merchant || !(merchant->m_unit->m_unitNpcFlags & 0x1000)) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    uint32_t cost = 0;
    int32_t guildBank = StringToBOOL(L, 1, 0);
    CGBag_C* bag = &player->m_bag;

    for (uint32_t i = 0; i <= 18; i++) {
        CGItem_C* item = bag->GetItemPointer(i);

        if (item && (item->m_obj->m_type & TYPE_ITEM) && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
            cost += CGMerchantInfo::GetRepairCost(item);
        }
    }

    for (uint32_t i = 23; i <= 38; i++) {
        CGItem_C* item = bag->GetItemPointer(i);

        if (item && (item->m_obj->m_type & TYPE_ITEM) && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
            cost += CGMerchantInfo::GetRepairCost(item);
        }
    }

    for (uint32_t i = 19; i <= 22; i++) {
        CGItem_C* container = player->m_bag.GetItemPointer(i);

        if (!container || !container->GetBag()) {
            continue;
        }

        CGBag_C* containerBag = container->GetBag();

        for (uint32_t j = 0; j < static_cast<uint32_t>(containerBag->m_numSlots); j++) {
            CGItem_C* item = containerBag->GetItemPointer(j);

            if (item && (item->m_obj->m_type & TYPE_ITEM) && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
                cost += CGMerchantInfo::GetRepairCost(item);
            }
        }
    }

    if (!guildBank && cost > player->GetCoinage()) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REPAIR_ITEM));
    msg.Put(CGMerchantInfo::m_merchant.m_guid);
    msg.Put(static_cast<uint64_t>(0));
    msg.Put(static_cast<uint8_t>(guildBank != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// STATIC: 0x00ACED00
static FrameScript_Method s_ScriptFunctions[] = {
    { "CloseMerchant", &Script_CloseMerchant },
    { "GetMerchantNumItems", &Script_GetMerchantNumItems },
    { "GetMerchantItemInfo", &Script_GetMerchantItemInfo },
    { "GetMerchantItemCostInfo", &Script_GetMerchantItemCostInfo },
    { "GetMerchantItemCostItem", &Script_GetMerchantItemCostItem },
    { "GetBuybackItemInfo", &Script_GetBuybackItemInfo },
    { "GetBuybackItemLink", &Script_GetBuybackItemLink },
    { "GetMerchantItemLink", &Script_GetMerchantItemLink },
    { "GetMerchantItemMaxStack", &Script_GetMerchantItemMaxStack },
    { "PickupMerchantItem", &Script_PickupMerchantItem },
    { "BuyMerchantItem", &Script_BuyMerchantItem },
    { "BuybackItem", &Script_BuybackItem },
    { "CanMerchantRepair", &Script_CanMerchantRepair },
    { "ShowMerchantSellCursor", &Script_ShowMerchantSellCursor },
    { "ShowBuybackSellCursor", &Script_ShowBuybackSellCursor },
    { "ShowRepairCursor", &Script_ShowRepairCursor },
    { "HideRepairCursor", &Script_HideRepairCursor },
    { "InRepairMode", &Script_InRepairMode },
    { "GetRepairAllCost", &Script_GetRepairAllCost },
    { "RepairAllItems", &Script_RepairAllItems },
    { "GetNumBuybackItems", &Script_GetNumBuybackItems },
};

// VERIFY: 47ef76cb auto
// OFFSET: 0x00584450
void MerchantRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: d1fc8388 auto
// OFFSET: 0x00584480
void MerchantUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
