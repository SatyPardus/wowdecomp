#include "CurrencyTypes.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CurrencyTypesRec.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"

// VERIFY: 051f8586 auto
// OFFSET: 0x005AFD10
static int32_t Script_GetCurrencyListSize(lua_State* L) {
    lua_pushnumber(L, CGCurrencyTypes::m_listSize);
    return 1;
}

// VERIFY: 8158a78e manual
// OFFSET: 0x005B0A90
static int32_t Script_GetBackpackCurrencyInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetBackpackCurrencyInfo(index)");
        return 0;
    }

    CurrencyTypesRec* currency = CGCurrencyTypes::GetBackpackCurrencyInfo(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    WOWGUID none = WOWGUID();
    ItemStats_C* stats;

    if (!currency || !(stats = g_itemDBCache.GetRecord(currency->m_itemID, none, nullptr, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 5;
    }

    if (stats->m_name[0]) {
        lua_pushstring(L, stats->GetName(0));
    } else {
        lua_pushnil(L);
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    } else {
        uint32_t count = 0;
        int32_t extraCurrencyType = 0;

        if (currency->m_itemID == 43307) {
            count = player->m_player->m_playerFieldArenaCurrency;
            extraCurrencyType = 1;
        } else if (currency->m_itemID == 43308) {
            count = player->m_player->m_playerFieldHonorCurrency;
            extraCurrencyType = 2;
        } else {
            CGBag_C* bag = player->GetBag();

            if (bag) {
                for (uint32_t slot = 118; slot <= 149; slot++) {
                    WOWGUID itemGuid;
                    itemGuid.m_guid = bag->GetItemGuid(slot);
                    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPE_ITEM, __FILE__, __LINE__));

                    if (item && static_cast<int32_t>(item->m_obj->m_entryID) == currency->m_itemID) {
                        count = item->m_item->m_itemFieldStackCount;
                        break;
                    }
                }
            }
        }

        lua_pushnumber(L, count);
        lua_pushnumber(L, extraCurrencyType);
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);
    lua_pushinteger(L, currency->m_itemID);
    return 5;
}

// VERIFY: 712f77f4 manual
// OFFSET: 0x005B0680
int32_t Script_GetCurrencyListInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCurrencyListInfo(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= CGCurrencyTypes::m_listSize) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 9;
    }

    CurrencyListEntry* entry = &CGCurrencyTypes::m_list[index];

    if (entry->m_name) {
        lua_pushstring(L, entry->m_name);
    } else {
        lua_pushnil(L);
    }

    lua_pushboolean(L, entry->m_isHeader);
    lua_pushboolean(L, entry->m_isExpanded);
    lua_pushboolean(L, CGCurrencyTypes::IsCurrencyShown(index));
    lua_pushboolean(L, entry->m_isWatched);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        uint32_t count = 0;
        int32_t extraCurrencyType = 0;

        if (entry->m_itemID == 43307) {
            count = player->m_player->m_playerFieldArenaCurrency;
            extraCurrencyType = 1;
        } else if (entry->m_itemID == 43308) {
            count = player->m_player->m_playerFieldHonorCurrency;
            extraCurrencyType = 2;
        } else {
            CGBag_C* bag = player->GetBag();

            if (bag) {
                for (uint32_t slot = 118; slot <= 149; slot++) {
                    WOWGUID itemGuid;
                    itemGuid.m_guid = bag->GetItemGuid(slot);
                    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPE_ITEM, __FILE__, __LINE__));

                    if (item && static_cast<int32_t>(item->m_obj->m_entryID) == entry->m_itemID) {
                        count = item->m_item->m_itemFieldStackCount;
                        break;
                    }
                }
            }
        }

        lua_pushnumber(L, count);
        lua_pushinteger(L, extraCurrencyType);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushboolean(L, 0);
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(entry->m_itemID, none, nullptr, nullptr, false);

    if (stats) {
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        char texture[260];
        SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
        lua_pushstring(L, texture);
    } else {
        lua_pushnil(L);
    }

    lua_pushinteger(L, entry->m_itemID);

    return 9;
}

// VERIFY: 7bc6412b manual
// OFFSET: 0x005B0940
static int32_t Script_ExpandCurrencyList(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: ExpandCurrencyList(index,expand)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    CGCurrencyTypes::SetCategoryExpand(index, static_cast<int32_t>(lua_tonumber(L, 2)));
    CGCurrencyTypes::BuildDisplayList();
    return 0;
}

// VERIFY: 2a2053ad manual
// OFFSET: 0x005B09B0
static int32_t Script_SetCurrencyUnused(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetCurrencyUnused(index,unused)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    CGCurrencyTypes::SetUnused(index, static_cast<int32_t>(lua_tonumber(L, 2)));
    CGCurrencyTypes::BuildDisplayList();
    return 0;
}

// VERIFY: 9f3c79b3 manual
// OFFSET: 0x005B0A20
static int32_t Script_SetCurrencyBackpack(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetCurrencyBackpack(index,backpack)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    CGCurrencyTypes::SetBackpack(index, static_cast<int32_t>(lua_tonumber(L, 2)));
    CGCurrencyTypes::BuildDisplayList();
    return 0;
}

// STATIC: 0x00ACFC34
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetCurrencyListSize", &Script_GetCurrencyListSize },
    { "GetCurrencyListInfo", &Script_GetCurrencyListInfo },
    { "ExpandCurrencyList", &Script_ExpandCurrencyList },
    { "SetCurrencyUnused", &Script_SetCurrencyUnused },
    { "SetCurrencyBackpack", &Script_SetCurrencyBackpack },
    { "GetBackpackCurrencyInfo", &Script_GetBackpackCurrencyInfo },
};

// VERIFY: 5ce58e3b auto
// OFFSET: 0x005AFD40
void CurrencyTypesRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: b539a1b6 auto
// OFFSET: 0x005AFD70
void CurrencyTypesUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
