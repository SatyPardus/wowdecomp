#include "GuildBankFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GuildInfo.h"
#include <ctime>
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00A15EEC
static const uint32_t s_guildBankTabCosts[6] = { 1000000, 2500000, 5000000, 10000000, 25000000, 50000000 };

// VERIFY: 245690a9 auto
// OFFSET: 0x005A4410
static int32_t Script_GetCurrentGuildBankTab(lua_State* L) {
    lua_pushnumber(L, CGGuildBankInfo::m_currentTab + 1);
    return 1;
}

// VERIFY: d2e0f5e2 auto
// OFFSET: 0x005A44A0
static int32_t Script_GetGuildBankWithdrawMoney(lua_State* L) {
    lua_pushnumber(L, CGGuildBankInfo::m_remainingWithdrawMoney);
    return 1;
}

// VERIFY: 424f18ba auto
// OFFSET: 0x005A44D0
static int32_t Script_GetNumGuildBankTabs(lua_State* L) {
    lua_pushnumber(L, CGGuildBankInfo::m_numTabs);
    return 1;
}

// VERIFY: 166b513c auto
// OFFSET: 0x005A45B0
static int32_t Script_GetNumGuildBankMoneyTransactions(lua_State* L) {
    lua_pushnumber(L, CGGuildBankInfo::m_numLogEntries[6]);
    return 1;
}

// VERIFY: 1c924350 auto
// OFFSET: 0x005A43C0
static int32_t Script_SetCurrentGuildBankTab(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetCurrentTab(tab)");
        return 0;
    }

    uint32_t tab = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (tab < 6) {
        CGGuildBankInfo::m_currentTab = tab;
    }

    return 0;
}

// VERIFY: 4de92797 auto
// OFFSET: 0x005A4440
static int32_t Script_GetGuildBankMoney(lua_State* L) {
    lua_pushnumber(L, static_cast<double>(CGGuildBankInfo::m_money));
    return 1;
}

// VERIFY: faf0c262 manual
// OFFSET: 0x005A6B20
static int32_t Script_GetGuildBankTabCost(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    uint32_t cost;

    if (CGGuildBankInfo::m_numTabs < 6 && (cost = s_guildBankTabCosts[CGGuildBankInfo::m_numTabs])) {
        lua_pushnumber(L, cost);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: be98e965 manual
// OFFSET: 0x005A5F60
static int32_t Script_CanWithdrawGuildBankMoney(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerGuildid) {
        const GuildRankInfo& rank = CGGuildInfo::m_rankInfo[player->m_player->m_playerGuildrank];

        if (rank.m_bankMoneyPerDay && (rank.m_rights & 0x80000)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: d4895bf7 auto
// OFFSET: 0x005A4330
static int32_t Script_PickupGuildBankMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupGuildBankMoney(amount)");
        return 0;
    }

    uint32_t amount = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (amount && amount <= CGGuildBankInfo::m_money) {
        CGGameUI::SetCursorMoney(amount, UICURSOR_GUILDBANK_MONEY, SCRIPT_EVENT_GUILDBANK_UPDATE_MONEY);
    }

    return 0;
}

// VERIFY: 06a5c736 auto
// OFFSET: 0x005A4500
static int32_t Script_CloseGuildBankFrame(lua_State* L) {
    WOWGUID none = WOWGUID();
    CGGuildBankInfo::SetGuildBanker(none);
    return 0;
}

// VERIFY: e2247c26 auto
// OFFSET: 0x005A4520
static int32_t Script_GetNumGuildBankTransactions(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetNumGuildBankTransactions(tab)");
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    lua_pushnumber(L, tab >= 7 ? 0 : CGGuildBankInfo::m_numLogEntries[tab]);
    return 1;
}

// VERIFY: 1d5f98d7 manual
// OFFSET: 0x005A45E0
static int32_t Script_GetGuildBankText(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildBankText(tab)");
        return 0;
    }

    int32_t value = static_cast<int32_t>(lua_tonumber(L, 1));
    uint8_t tab = value - 1;

    if (static_cast<uint8_t>(value - 1) < 6 && tab < CGGuildBankInfo::m_numTabs) {
        char text[8192];
        SStrCopy(text, CGGuildBankInfo::m_guildBankTabInfo[tab].m_text, sizeof(text));
        lua_pushstring(L, text);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: dfebfdff manual
// OFFSET: 0x005A4D30
static int32_t Script_GetGuildBankItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetGuildBankItemLink(tab, slot)");
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (tab >= 6 || slot >= CGGuildBankInfo::m_tabSlots[tab].m_count) {
        return 0;
    }

    GuildBankSlotInfo* info = &CGGuildBankInfo::m_tabSlots[tab].m_data[slot];

    if (!info->m_itemID) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(info->m_itemID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::CreateItemLink(info->m_itemID, stats->m_quality, info->m_enchantId, reinterpret_cast<const int32_t*>(info->m_enchants), info->m_randomPropertyID, info->m_suffixFactor));
    return 1;
}

// VERIFY: eee3d7b8 manual
// OFFSET: 0x005A4E50
static int32_t Script_GetGuildBankMoneyTransaction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildBankMoneyTransaction(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    GuildBankLogEntry* entry = index >= 25 ? nullptr : &CGGuildBankInfo::m_log[6][index];

    if (entry->m_type == 4) {
        lua_pushstring(L, "deposit");
    } else if (entry->m_type == 5) {
        lua_pushstring(L, "withdraw");
    } else if (entry->m_type == 8) {
        lua_pushstring(L, "withdrawForTab");
    } else if (entry->m_type == 9) {
        lua_pushstring(L, "buyTab");
    } else {
        lua_pushstring(L, "repair");
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(entry->m_guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        lua_pushstring(L, name->m_name);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, entry->m_money);
    int32_t age = static_cast<int32_t>(static_cast<uint32_t>(::time(nullptr)) - entry->m_time);
    int32_t years = age / 3600 / 8760;
    int32_t hours = age / 3600 % 8760;
    int32_t months = hours / 720;
    hours %= 720;
    int32_t days = hours / 24;
    lua_pushnumber(L, years);
    lua_pushnumber(L, months);
    lua_pushnumber(L, days);
    lua_pushnumber(L, hours % 24);
    return 7;
}

// VERIFY: 2728bcf7 auto
// OFFSET: 0x005A5CE0
static int32_t Script_QueryGuildBankTab(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: QueryGuildBankTab(tab)");
        return 0;
    }

    double tab = lua_tonumber(L, 1);

    if (static_cast<uint32_t>(tab - 1.0) < 6) {
        SendGuildBankQueryTabOpcode(static_cast<uint8_t>(tab - 1.0));
    }

    return 0;
}

// VERIFY: a7050066 manual
// OFFSET: 0x005A5D30
static int32_t Script_BuyGuildBankTab(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !CGGameUI::m_hasControl || !CGGuildBankInfo::m_banker) {
        return 0;
    }

    uint8_t tab = CGGuildBankInfo::m_numTabs;
    uint64_t money = CGGuildBankInfo::m_money;

    if (s_guildBankTabCosts[tab] > player->GetCoinage() + money) {
        CGGameUI::DisplayError(ERR_BANKSLOT_INSUFFICIENT_FUNDS);
        return 0;
    }

    CGGuildBankInfo::BuyGuildBankTab();
    return 0;
}

// VERIFY: b6d15730 manual
// OFFSET: 0x005A5E20
static int32_t Script_DepositGuildBankMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: DepositGuildBankMoney(money)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t amount = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (player->GetCoinage() >= amount) {
        CGGuildBankInfo::DepositMoney(amount);
    } else {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
    }

    return 0;
}

// VERIFY: bf1125f5 auto
// OFFSET: 0x005A5EF0
static int32_t Script_WithdrawGuildBankMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: WithdrawGuildBankMoney(money)");
        return 0;
    }

    CGGuildBankInfo::WithdrawMoney(static_cast<uint32_t>(lua_tonumber(L, 1)));
    return 0;
}

// VERIFY: f2361e39 manual
// OFFSET: 0x005A6000
static int32_t Script_GetGuildBankItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetGuildBankItemInfo(tab, slot)");
        return 0;
    }

    int32_t value = static_cast<int32_t>(lua_tonumber(L, 1));
    uint8_t tabByte = value - 1;
    uint32_t tab = static_cast<uint8_t>(value - 1);
    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    GuildBankSlotInfo* info;
    ItemRec* rec;

    if (tab < 6 && slot < CGGuildBankInfo::m_tabSlots[tabByte].m_count && (info = &CGGuildBankInfo::m_tabSlots[tab].m_data[slot])->m_itemID && info && ClntObjMgrGetActivePlayerObj() && (rec = g_itemDB.GetRecord(info->m_itemID))) {
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        char texture[260];
        SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(rec->m_displayInfoID));
        lua_pushstring(L, texture);
        lua_pushnumber(L, info->m_count);

        if (CGGameUI::VirtualItemIsLocked(info->m_itemID, slot + 98 * tab, 0)) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        return 3;
    }

    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    return 3;
}

// VERIFY: 2d78d9a3 manual
// OFFSET: 0x005A6200
static int32_t Script_PickupGuildBankItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: PickupGuildBankItem(tab, slot)");
        return 0;
    }

    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    uint8_t slot = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;

    if (tab >= 6 || slot >= 98) {
        return 0;
    }

    int32_t virtualId = 0;
    int32_t virtualSlot = 0;
    WOWGUID item;
    WOWGUID container;
    uint32_t containerSlot;
    CGGameUI::GetCursorItem(item, container, containerSlot);

    if (CGGameUI::m_cursorItemType == UICURSOR_GUILDBANK_ITEM) {
        CGGameUI::GetCursorVirtualItem(virtualId, virtualSlot);
    }

    if (item) {
        CGGuildBankInfo::SwapItems(tab, slot, item, container, containerSlot, CGGameUI::m_stackSplit);
        return 0;
    }

    if (virtualId) {
        CGGuildBankInfo::SwapBankItems(tab, slot, virtualId, virtualSlot, CGGameUI::m_stackSplit);
        return 0;
    }

    GuildBankSlotInfo* info = CGGuildBankInfo::GetGuildBankItem(tab, slot);

    if (!info) {
        return 0;
    }

    if (Spell_C_IsTargeting()) {
        Spell_C_StopTargeting();
    }

    ItemRec* rec = g_itemDB.GetRecord(info->m_itemID);

    if (rec) {
        uint32_t bankSlot = slot + 98 * tab;

        if (!CGGameUI::VirtualItemIsLocked(info->m_itemID, bankSlot, 0)) {
            CGGameUI::SetCursorVirtualItem(info->m_itemID, rec->m_displayInfoID, bankSlot, UICURSOR_GUILDBANK_ITEM, 0);
            CGGameUI::LockVirtualItem(info->m_itemID, bankSlot, 0);
        }
    }

    return 0;
}

// VERIFY: 1d2c15e0 manual
// OFFSET: 0x005A63F0
static int32_t Script_AutoStoreGuildBankItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: AutoStoreGuildBankItem(tab, slot)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    WOWGUID banker = CGGuildBankInfo::m_banker;

    if (!banker) {
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t value = static_cast<int32_t>(lua_tonumber(L, 2));
    uint8_t slot = value - 1;

    if (tab >= 6 || static_cast<uint8_t>(value - 1) >= 98) {
        return 0;
    }

    CGGameUI::ClearCursor(1, 1);
    GuildBankSlotInfo* info = CGGuildBankInfo::GetGuildBankItem(tab, slot);

    if (!info) {
        return 0;
    }

    if (Spell_C_IsTargeting()) {
        Spell_C_StopTargeting();
    }

    if (!CGGuildInfo::GetNumGuildBankTabItemWithdrawalsByRankID(tab, player->m_player->m_playerGuildrank)) {
        CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
        return 0;
    }

    CGGameUI::m_autoStoreItem = WOWGUID();
    CGGameUI::m_autoStoreItemID = info->m_itemID;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_SWAP_ITEMS));
    msg.Put(banker.m_guid);
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(tab);
    msg.Put(slot);
    msg.Put(static_cast<uint32_t>(info->m_itemID));
    msg.Put(static_cast<uint8_t>(1));
    msg.Put(static_cast<uint32_t>(info->m_count));
    msg.Put(static_cast<uint8_t>(1));
    msg.Put(static_cast<uint32_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
    return 0;
}

// VERIFY: f0f42da3 manual
// OFFSET: 0x005A6600
static int32_t Script_SplitGuildBankItem(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: SplitGuildBankItem(tab, slot, amount)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || (!CGGameUI::m_hasControl && !(player->m_unit->m_unitFieldFlags & 0x100000))) {
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    uint8_t slot = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;

    if (tab >= 6 || slot >= 98) {
        return 0;
    }

    int32_t virtualId = 0;
    int32_t virtualSlot = 0;
    uint32_t bankSlot = slot + 98 * tab;

    if (CGGameUI::m_cursorItemType == UICURSOR_GUILDBANK_ITEM) {
        CGGameUI::GetCursorVirtualItem(virtualId, virtualSlot);
    }

    GuildBankSlotInfo* info = CGGuildBankInfo::GetGuildBankItem(tab, slot);

    if (!info || info->m_locked) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(info->m_itemID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    int32_t amount = static_cast<int32_t>(lua_tonumber(L, 3));

    if (amount < 1 || amount > stats->m_stackable) {
        return 0;
    }

    CGGameUI::ClearCursor(1, 1);
    Spell_C_StopTargeting();

    if (amount == stats->m_stackable) {
        CGGameUI::SetCursorVirtualItem(info->m_itemID, stats->m_displayId, bankSlot, UICURSOR_GUILDBANK_ITEM, 0);
        CGGameUI::LockVirtualItem(info->m_itemID, bankSlot, 0);
    }

    CGGameUI::SetCursorVirtualItem(info->m_itemID, stats->m_displayId, bankSlot, UICURSOR_GUILDBANK_ITEM, amount);
    CGGameUI::LockVirtualItem(info->m_itemID, bankSlot, 0);
    return 0;
}

// VERIFY: 227f7152 manual
// OFFSET: 0x005A67F0
static int32_t Script_GetGuildBankTabInfo(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGuildBankTabInfo(tab)");
        return 0;
    }

    int32_t value = static_cast<int32_t>(lua_tonumber(L, 1));
    uint8_t tab = value - 1;

    if (static_cast<uint8_t>(value - 1) >= 6 || tab >= CGGuildBankInfo::m_numTabs) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 6;
    }

    GuildBankTabInfo* info = &CGGuildBankInfo::m_guildBankTabInfo[tab];

    if (info->m_name[0]) {
        lua_pushstring(L, info->m_name);
    } else {
        char name[64];
        SStrPrintf(name, sizeof(name), FrameScript_GetText("GUILDBANK_TAB_NUMBER", -1, GENDER_NOT_APPLICABLE), tab + 1);
        lua_pushstring(L, name);
    }

    char* icon = info->m_icon;

    if (!SStrLen(icon)) {
        SStrCopy(icon, "INV_Misc_QuestionMark", STORM_MAX_STR);
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", icon);
    lua_pushstring(L, texture);
    uint32_t rank = player->m_player->m_playerGuildrank;
    uint8_t rights = CGGuildInfo::GetGuildBankTabPermissionsByRankID(tab, rank);

    if (rights & 1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (rights & 2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, CGGuildInfo::GetNumGuildBankTabItemWithdrawalsByRankID(tab, rank));
    lua_pushnumber(L, CGGuildBankInfo::m_withdrawMoneyRemaining);
    return 6;
}

// VERIFY: 2abc9d81 auto
// OFFSET: 0x005A6A00
static int32_t Script_SetGuildBankTabInfo(lua_State* L) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: SetGuildBankTabInfo(tab, name, iconIndex)");
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    const char* name = lua_tolstring(L, 2, nullptr);
    double index = lua_tonumber(L, 3);

    if (!s_macroItemIcons.m_count) {
        CGUIMacros::LoadIcons();
    }

    if (static_cast<uint32_t>(index - 1.0) < s_macroItemIcons.m_count) {
        const char* icon = GetMacroItemIcon(static_cast<uint32_t>(index - 1.0));

        if (icon) {
            CGGuildBankInfo::SetTabInfo(tab, name, icon);
        }
    }

    return 0;
}

// VERIFY: 6e29aa6c manual
// OFFSET: 0x005A6BA0
static int32_t Script_GetGuildTabardFileNames(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    int32_t emblemStyle;
    int32_t emblemColor;
    int32_t borderStyle;
    int32_t borderColor;
    int32_t background;
    GuildGetGuildTabard(player->m_obj->m_guid, player->m_player->m_playerGuildid, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background);

    if (emblemStyle == -1 || borderStyle == -1 || background == -1) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 6;
    }

    char file[260];
    CCharacterComponent::GetTabardBackgroundFileName(SECTION_TORSO_UPPER, background, file, sizeof(file));
    lua_pushstring(L, file);
    CCharacterComponent::GetTabardBackgroundFileName(SECTION_TORSO_LOWER, background, file, sizeof(file));
    lua_pushstring(L, file);
    CCharacterComponent::GetTabardEmblemFileName(SECTION_TORSO_UPPER, emblemStyle, emblemColor, file, sizeof(file));
    lua_pushstring(L, file);
    CCharacterComponent::GetTabardEmblemFileName(SECTION_TORSO_LOWER, emblemStyle, emblemColor, file, sizeof(file));
    lua_pushstring(L, file);
    CCharacterComponent::GetTabardBorderFileName(SECTION_TORSO_UPPER, borderStyle, borderColor, file, sizeof(file));
    lua_pushstring(L, file);
    CCharacterComponent::GetTabardBorderFileName(SECTION_TORSO_LOWER, borderStyle, borderColor, file, sizeof(file));
    lua_pushstring(L, file);
    return 6;
}

// VERIFY: 9a345d1e auto
// OFFSET: 0x005A6D50
static int32_t Script_QueryGuildBankLog(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: QueryGuildBankLog(tab)");
        return 0;
    }

    CGGuildBankInfo::QueryGuildBankLog(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: 4c64bf1d manual
// OFFSET: 0x005A6DC0
static int32_t Script_GetGuildBankTransaction(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetGuildBankTransaction(tab, index)");
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    uint8_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    GuildBankLogEntry* entry = tab >= 7 || index >= 25 ? nullptr : &CGGuildBankInfo::m_log[tab][index];

    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    char type[16];

    switch (entry->m_type) {
        case 1:
            SStrCopy(type, "deposit", STORM_MAX_STR);
            break;
        case 2:
            SStrCopy(type, "withdraw", STORM_MAX_STR);
            break;
        case 3:
        case 7:
            SStrCopy(type, "move", STORM_MAX_STR);
            break;
        default:
            SStrCopy(type, "unknown", STORM_MAX_STR);
            break;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(entry->m_guid.m_guid, none, nullptr, nullptr, false);
    WOWGUID itemGuid = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(entry->m_itemID, itemGuid, nullptr, nullptr, false);
    lua_pushstring(L, type);

    if (name) {
        lua_pushstring(L, name->m_name);
    } else {
        lua_pushnil(L);
    }

    if (stats) {
        lua_pushstring(L, CGTooltip::GetItemHyperlinkString(entry->m_itemID, stats->m_quality, 0, 0, 0));
    } else {
        lua_pushstring(L, "");
    }

    lua_pushnumber(L, entry->m_count);

    if (entry->m_type == 3) {
        lua_pushnumber(L, tab + 1);
        lua_pushnumber(L, entry->m_otherTab + 1);
    } else if (entry->m_type == 7) {
        lua_pushnumber(L, entry->m_otherTab + 1);
        lua_pushnumber(L, tab + 1);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    int32_t age = static_cast<int32_t>(static_cast<uint32_t>(::time(nullptr)) - entry->m_time);
    int32_t years = age / 3600 / 8760;
    int32_t hours = age / 3600 % 8760;
    int32_t months = hours / 720;
    hours %= 720;
    int32_t days = hours / 24;
    lua_pushnumber(L, years);
    lua_pushnumber(L, months);
    lua_pushnumber(L, days);
    lua_pushnumber(L, hours % 24);
    return 10;
}

// VERIFY: 5b511218 auto
// OFFSET: 0x005A7110
static int32_t Script_QueryGuildBankText(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: QueryGuildBankLog(tab)");
        return 0;
    }

    CGGuildBankInfo::QueryGuildBankText(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: 96d4bc1a auto
// OFFSET: 0x005A7180
static int32_t Script_SetGuildBankText(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetGuildBankText(tab, text)");
        return 0;
    }

    uint8_t tab = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    const char* text = lua_tolstring(L, 2, nullptr);
    char scrubbed[8194];
    scrubbed[0] = '\0';

    if (text && *text) {
        ScrubString(text, scrubbed, 2048, 8192, 0);
    }

    CGGuildBankInfo::SetGuildBankText(tab, scrubbed);
    return 0;
}

// STATIC: 0x00ACF8D0
static FrameScript_Method s_ScriptFunctions[] = {
    { "QueryGuildBankTab", &Script_QueryGuildBankTab },
    { "SetCurrentGuildBankTab", &Script_SetCurrentGuildBankTab },
    { "GetCurrentGuildBankTab", &Script_GetCurrentGuildBankTab },
    { "GetGuildBankItemInfo", &Script_GetGuildBankItemInfo },
    { "SetGuildBankTabInfo", &Script_SetGuildBankTabInfo },
    { "GetGuildBankItemLink", &Script_GetGuildBankItemLink },
    { "PickupGuildBankItem", &Script_PickupGuildBankItem },
    { "AutoStoreGuildBankItem", &Script_AutoStoreGuildBankItem },
    { "SplitGuildBankItem", &Script_SplitGuildBankItem },
    { "GetNumGuildBankTabs", &Script_GetNumGuildBankTabs },
    { "GetGuildBankTabInfo", &Script_GetGuildBankTabInfo },
    { "GetGuildBankTabCost", &Script_GetGuildBankTabCost },
    { "BuyGuildBankTab", &Script_BuyGuildBankTab },
    { "DepositGuildBankMoney", &Script_DepositGuildBankMoney },
    { "WithdrawGuildBankMoney", &Script_WithdrawGuildBankMoney },
    { "CanWithdrawGuildBankMoney", &Script_CanWithdrawGuildBankMoney },
    { "PickupGuildBankMoney", &Script_PickupGuildBankMoney },
    { "GetGuildBankMoney", &Script_GetGuildBankMoney },
    { "GetGuildBankWithdrawMoney", &Script_GetGuildBankWithdrawMoney },
    { "CloseGuildBankFrame", &Script_CloseGuildBankFrame },
    { "GetGuildTabardFileNames", &Script_GetGuildTabardFileNames },
    { "QueryGuildBankLog", &Script_QueryGuildBankLog },
    { "GetNumGuildBankTransactions", &Script_GetNumGuildBankTransactions },
    { "GetGuildBankTransaction", &Script_GetGuildBankTransaction },
    { "GetNumGuildBankMoneyTransactions", &Script_GetNumGuildBankMoneyTransactions },
    { "GetGuildBankMoneyTransaction", &Script_GetGuildBankMoneyTransaction },
    { "QueryGuildBankText", &Script_QueryGuildBankText },
    { "GetGuildBankText", &Script_GetGuildBankText },
    { "SetGuildBankText", &Script_SetGuildBankText },
};

// VERIFY: 91a3bc69 auto
// OFFSET: 0x005A46A0
void GuildBankRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 4bbe0f1b auto
// OFFSET: 0x005A46D0
void GuildBankUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
