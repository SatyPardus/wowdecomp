#include "TradeFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Object/ObjectClient/Trade_C.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// VERIFY: 35618cea auto
// OFFSET: 0x00586730
static int32_t Script_CancelTradeAccept(lua_State* L) {
    if (CGGameUI::CanPerformAction(7) && CGTradeInfo::m_playerAccepted) {
        CGTradeInfo::m_playerAccepted = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_ACCEPT_UPDATE, "%d%d", 0, CGTradeInfo::m_targetAccepted);
        Trade_C_UnacceptTrade();
    }

    return 0;
}

// VERIFY: 2d9f2b3c manual
// OFFSET: 0x00586780
static int32_t Script_GetPlayerTradeMoney(lua_State* L) {
    uint32_t money = CGTradeInfo::m_tradingPlayer ? CGTradeInfo::GetPlayerTradeMoney() : 0;
    lua_pushnumber(L, money);
    return 1;
}

// VERIFY: 89405660 manual
// OFFSET: 0x005867D0
static int32_t Script_GetTargetTradeMoney(lua_State* L) {
    lua_pushnumber(L, CGTradeInfo::GetTargetTradeMoney());
    return 1;
}

// VERIFY: 0ea1cc12 manual
// OFFSET: 0x005865F0
static int32_t Script_GetTradeTargetItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeTargetItemLink(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index > 6) {
        return 0;
    }

    int32_t itemId = CGTradeInfo::m_targetItemID[index];

    if (!itemId) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    int32_t gems[3];

    for (uint32_t i = 0; i < 3; i++) {
        gems[i] = CGTradeInfo::m_targetSocketEnchants[index][i];
    }

    lua_pushstring(L, CGTooltip::CreateItemLink(itemId, stats->m_quality, CGTradeInfo::m_targetEnchant[index], gems, CGTradeInfo::m_targetRandomPropertyID[index], CGTradeInfo::m_targetSuffixFactor[index]));
    return 1;
}

// VERIFY: 54f3c718 auto
// OFFSET: 0x005866E0
static int32_t Script_AcceptTrade(lua_State* L) {
    if (CGGameUI::CanPerformAction(7) && CGTradeInfo::m_playerAccepted != 1) {
        CGTradeInfo::m_playerAccepted = 1;
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_ACCEPT_UPDATE, "%d%d", 1, CGTradeInfo::m_targetAccepted);
        Trade_C_AcceptTrade();
    }

    return 0;
}

// VERIFY: 2f99e941 auto
// OFFSET: 0x00586810
static int32_t Script_PickupTradeMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupTradeMoney(amount)");
        return 0;
    }

    int32_t amount = static_cast<int32_t>(lua_tonumber(L, 1));

    if (amount > 0 && static_cast<uint32_t>(amount) <= CGTradeInfo::GetPlayerTradeMoney()) {
        CGTradeInfo::RemoveMoney(amount);
        CGGameUI::SetCursorMoney(amount);
    }

    return 0;
}

// VERIFY: ac0f520f auto
// OFFSET: 0x00586870
static int32_t Script_SetTradeMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetTradeMoney(amount)");
        return 0;
    }

    CGTradeInfo::SetMoney(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1))));
    return 0;
}

// VERIFY: 28feac8a manual
// OFFSET: 0x00586C80
static int32_t Script_ClickTargetTradeButton(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ClickTargetTradeButton(index)");
        return 0;
    }

    if (CGGameUI::m_cursorMoney) {
        if (CGTradeInfo::AddMoney(CGGameUI::m_cursorMoney)) {
            CGGameUI::ClearCursor(1, 0);
        }
    } else if (static_cast<int32_t>(lua_tonumber(L, 1)) - 1 == 6) {
        CGGameUI::TargetTradeItem();
    }

    return 0;
}

// VERIFY: aa5fb0e2 auto
// OFFSET: 0x00586D00
static int32_t Script_GetTradePlayerItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradePlayerItemLink(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    WOWGUID guid = index > 6 ? WOWGUID() : CGTradeInfo::m_playerItems[index];
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
    return 1;
}

// VERIFY: e1fe08df auto
// OFFSET: 0x00586D90
static int32_t Script_AddTradeMoney(lua_State* L) {
    if (CGGameUI::m_cursorMoney && CGTradeInfo::AddMoney(CGGameUI::m_cursorMoney)) {
        CGGameUI::ClearCursor(1, 0);
    }

    return 0;
}

// VERIFY: 73f06b5f auto
// OFFSET: 0x00587940
static int32_t Script_CloseTrade(lua_State* L) {
    if (!CGTradeInfo::m_supressMoneyEvent) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TRADE_MONEY, nullptr);
    }

    if (CGTradeInfo::m_tradingPlayer) {
        if (CGTradeInfo::m_enchantingItem) {
            CGGameUI::UnlockItem(CGTradeInfo::m_enchantingItem);
            CGTradeInfo::m_enchantingItem = WOWGUID();
        }

        CGTradeInfo::m_supressMoneyEvent = 0;
        CGGameUI::ClearInteractTarget(CGTradeInfo::m_tradingPlayer, 0);
        CGTradeInfo::m_tradingPlayer = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_CLOSED, nullptr);
    }

    Trade_C_CancelTrade();
    return 0;
}

// VERIFY: 161558e0 manual
// OFFSET: 0x005879D0
static int32_t Script_ClickTradeButton(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ClickTradeButton(index)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (lua_toboolean(L, 2)) {
        WOWGUID item;
        WOWGUID container;
        uint8_t containerSlot;
        CGTradeInfo::GetPlayerItem(slot, item, container, containerSlot);

        if (item) {
            CGTradeInfo::SetPlayerItem(slot, WOWGUID(), WOWGUID(), 0);
            CGGameUI::UnlockItem(item);
            CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

            if (object) {
                SndInterfacePlayItemSound(ITEMSOUND_DROP, object);
            }
        }

        return 0;
    }

    if (CGGameUI::m_cursorMoney) {
        if (CGTradeInfo::AddMoney(CGGameUI::m_cursorMoney)) {
            CGGameUI::ClearCursor(1, 0);
        }

        return 0;
    }

    WOWGUID cursorItem;
    WOWGUID cursorContainer;
    uint32_t cursorSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(cursorItem, TYPE_ITEM, __FILE__, __LINE__));

    if (slot < 6 && item) {
        if (item->IsPermanentlyBoundForTrade()) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

            if (stats && stats->m_bonding == 4) {
                CGGameUI::DisplayError(ERR_TRADE_QUEST_ITEM);
            } else {
                CGGameUI::DisplayError(ERR_TRADE_BOUND_ITEM);
            }

            return 0;
        }

        if (item->HasSoulboundEnchant()) {
            CGGameUI::DisplayError(ERR_TRADE_TEMP_ENCHANT_BOUND);
            return 0;
        }

        if (item->m_obj->m_type & TYPE_CONTAINER) {
            CGBag_C* bag = item->GetBag();

            if (bag) {
                if (bag->GetNumItems()) {
                    CGGameUI::DisplayError(ERR_TRADE_BAG);
                    return 0;
                }

                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

                if (static_cast<uint32_t>(player->GetBag()->FindItemSlot(item) - 19) <= 3) {
                    CGGameUI::DisplayError(ERR_TRADE_EQUIPPED_BAG);
                    return 0;
                }
            }
        }
    }

    WOWGUID current;
    WOWGUID container;
    uint8_t containerSlot;
    CGTradeInfo::GetPlayerItem(slot, current, container, containerSlot);

    if (cursorItem == current) {
        CGGameUI::ClearCursor(1, 1);
        return 0;
    }

    int32_t set = CGTradeInfo::SetPlayerItem(slot, cursorItem, cursorContainer, static_cast<uint8_t>(cursorSlot));
    CGGameUI::SetCursorItem(current, container, containerSlot, set == 0, 0, 1);
    return 0;
}

// VERIFY: 8c588e45 auto
// OFFSET: 0x00587C60
static int32_t Script_GetTradeTargetItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeTargetItemInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t itemId;
    ItemStats_C* stats;

    if (index > 6 || !(itemId = CGTradeInfo::m_targetItemID[index]) || !(stats = g_itemDBCache.GetRecord(itemId, CGTradeInfo::m_tradingPlayer, &TradeItemStatsCallback, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        return 6;
    }

    char name[256];
    CGItem_C::BuildItemName(name, sizeof(name), itemId, CGTradeInfo::m_targetRandomPropertyID[index]);
    lua_pushstring(L, name);
    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);
    lua_pushnumber(L, CGTradeInfo::m_targetCount[index]);
    lua_pushnumber(L, stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t error;

    if (!player || player->CanUseItem(stats, error)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (index == 6) {
        int32_t spellId = Trade_C_GetEnchantSpell(0);

        if (spellId) {
            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell)) {
                lua_pushstring(L, spell.m_name);
                return 6;
            }
        }
    }

    lua_pushnil(L);
    return 6;
}

// VERIFY: 3df471df auto
// OFFSET: 0x00587EB0
static int32_t Script_GetTradePlayerItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradePlayerItemInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    WOWGUID guid = index > 6 ? WOWGUID() : CGTradeInfo::m_playerItems[index];
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        return 5;
    }

    char name[256];
    item->GetFullItemName(name, sizeof(name));
    lua_pushstring(L, name);
    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
    lua_pushstring(L, texture);
    lua_pushnumber(L, item->m_item->m_itemFieldStackCount);
    WOWGUID requester = ClntObjMgrGetActivePlayer();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, requester, &TradeItemStatsCallback, nullptr, false);
    lua_pushnumber(L, stats && stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);

    if (index == 6) {
        int32_t spellId = Trade_C_GetEnchantSpell(1);

        if (spellId) {
            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell)) {
                lua_pushstring(L, spell.m_name);
                return 5;
            }
        }
    }

    lua_pushnil(L);
    return 5;
}

// STATIC: 0x00ACEDB0
static FrameScript_Method s_ScriptFunctions[] = {
    { "CloseTrade", &Script_CloseTrade },
    { "ClickTradeButton", &Script_ClickTradeButton },
    { "ClickTargetTradeButton", &Script_ClickTargetTradeButton },
    { "GetTradeTargetItemInfo", &Script_GetTradeTargetItemInfo },
    { "GetTradeTargetItemLink", &Script_GetTradeTargetItemLink },
    { "GetTradePlayerItemInfo", &Script_GetTradePlayerItemInfo },
    { "GetTradePlayerItemLink", &Script_GetTradePlayerItemLink },
    { "AcceptTrade", &Script_AcceptTrade },
    { "CancelTradeAccept", &Script_CancelTradeAccept },
    { "GetPlayerTradeMoney", &Script_GetPlayerTradeMoney },
    { "GetTargetTradeMoney", &Script_GetTargetTradeMoney },
    { "PickupTradeMoney", &Script_PickupTradeMoney },
    { "AddTradeMoney", &Script_AddTradeMoney },
    { "SetTradeMoney", &Script_SetTradeMoney },
};

// VERIFY: d36bca96 auto
// OFFSET: 0x005868E0
void TradeInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 936871e9 auto
// OFFSET: 0x00586910
void TradeInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
