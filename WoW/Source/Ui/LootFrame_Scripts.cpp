#include "LootFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Os/Time.h"
#include "WoW/Source/Object/ObjectClient/LootRoll.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

// VERIFY: 55f64cec manual
// OFFSET: 0x00588CC0
static int32_t Script_GetLootRollTimeLeft(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLootRollTimeLeft(id)");
        return 0;
    }

    PendingLootRoll* roll = FindRoll(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (roll) {
        lua_pushnumber(L, static_cast<int32_t>(roll->m_expireTime - OsGetAsyncTimeMs()));
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: 52798a77 manual
// OFFSET: 0x00588750
static int32_t Script_LootSlotIsItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: LootSlotIsItem(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t index = slot - 1;

    if (CGLootInfo::m_lootObj) {
        if (CGLootInfo::m_coins) {
            if (slot == 1) {
                lua_pushnil(L);
                return 1;
            }

            index = slot - 2;
        }

        if (index < 18 && CGLootInfo::m_slots[index].m_itemID > 0) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 07974e21 auto
// OFFSET: 0x00588810
static int32_t Script_LootSlotIsCoin(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: LootSlotIsCoin(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGLootInfo::m_lootObj && CGLootInfo::m_coins > 0 && slot == 1) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 7c2b4fef auto
// OFFSET: 0x005888E0
static int32_t Script_IsFishingLoot(lua_State* L) {
    if (CGLootInfo::m_lootAcquire == LOOT_ACQUIRE_FISHING) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a7a1243d manual
// OFFSET: 0x005889D0
static int32_t Script_GetLootRollItemInfo(lua_State* L) {
    char buffer[256];

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLootRollItemInfo(id)");
        return 0;
    }

    PendingLootRoll* roll = FindRoll(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (!roll) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    const char* separator = *path ? "\\" : "";
    SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, buffer);
    CGItem_C::BuildItemName(buffer, sizeof(buffer), roll->m_itemId, roll->m_randomPropertyId);
    lua_pushstring(L, buffer);
    lua_pushnumber(L, roll->m_count);
    lua_pushnumber(L, static_cast<int32_t>(stats->m_quality));

    if (stats->m_bonding == 1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (roll->m_voteMask & 2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (roll->m_voteMask & 4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (roll->m_voteMask & 8) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, roll->m_reasonNeed);
    lua_pushnumber(L, roll->m_reasonGreed);
    lua_pushnumber(L, roll->m_reasonDisenchant);
    lua_pushnumber(L, static_cast<int32_t>(stats->m_requiredDisenchantSkill));
    return 12;
}

// VERIFY: a8f76425 auto
// OFFSET: 0x00588150
static void MasterLootCandidateNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_MASTER_LOOT_LIST, nullptr);
    }
}

// VERIFY: d0557dee auto
// OFFSET: 0x00588920
static int32_t Script_GetMasterLootCandidate(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMasterLootCandidate(index)");
        return 0;
    }

    WOWGUID guid = GetMasterLootCandidate(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1));
    NameCache* record = guid != WOWGUID() ? g_nameDBCache.GetRecord(guid.m_guid, guid, MasterLootCandidateNameCallback, nullptr, false) : nullptr;

    if (record) {
        lua_pushstring(L, record->m_name);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: c26f542e auto
// OFFSET: 0x00588540
static int32_t Script_GetNumLootItems(lua_State* L) {
    lua_pushnumber(L, CGLootWindow::GetNumLootItems());
    return 1;
}

// VERIFY: dcbdf15b manual
// OFFSET: 0x00588570
static int32_t Script_GetLootSlotInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLootSlotInfo(slot)");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    lua_pushstring(L, CGLootInfo::GetLootSlotTexture(slot));
    char text[256];
    CGLootInfo::GetLootSlotText(text, sizeof(text), slot);
    lua_pushstring(L, text);
    lua_pushnumber(L, CGLootInfo::GetLootItemCount(slot));
    lua_pushnumber(L, CGLootWindow::GetLootItemRarity(slot));
    uint32_t index = slot;

    if (CGLootInfo::m_lootObj) {
        if (CGLootInfo::m_coins) {
            if (!slot) {
                lua_pushnil(L);
                return 5;
            }

            index = slot - 1;
        }

        if (index < 18 && CGLootInfo::m_slots[index].m_slotType == 3) {
            lua_pushnumber(L, 1.0);
            return 5;
        }
    }

    lua_pushnil(L);
    return 5;
}

// VERIFY: fc45465a auto
// OFFSET: 0x005886D0
static int32_t Script_GetLootSlotLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLootSlotLink(slot)");
        return 0;
    }

    char link[1024];
    lua_pushstring(L, CGLootInfo::GetLootSlotLink(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, link, sizeof(link)));
    return 1;
}

// VERIFY: a1b183d2 auto
// OFFSET: 0x005888B0
static int32_t Script_CloseLoot(lua_State* L) {
    CGGameUI::CloseLoot(true, false, StringToBOOL(L, 1, 0) != 0);
    return 0;
}

// VERIFY: 661226f7 auto
// OFFSET: 0x00588C00
static int32_t Script_GetLootRollItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLootRollItemLink(id)");
        return 0;
    }

    PendingLootRoll* roll = FindRoll(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (!roll) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
    return 1;
}

// VERIFY: 7c8b39af auto
// OFFSET: 0x00588D60
static int32_t Script_RollOnLoot(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: RollOnLoot(id, rollType)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    LootRoll(id, static_cast<int32_t>(lua_tonumber(L, 2)), false);
    return 0;
}

// VERIFY: 88e937ae auto
// OFFSET: 0x00588DD0
static int32_t Script_ConfirmLootRoll(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: ConfirmLootRoll(id, rollType)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    LootRoll(id, static_cast<int32_t>(lua_tonumber(L, 2)), true);
    return 0;
}

// VERIFY: 2f666037 auto
// OFFSET: 0x00589520
static int32_t Script_LootSlot(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: LootSlot(slot)");
        return 0;
    }

    CGLootInfo::LootSlot(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: 085a0c07 auto
// OFFSET: 0x00589590
static int32_t Script_ConfirmLootSlot(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: LootSlot(slot)");
        return 0;
    }

    CGLootInfo::ConfirmLootSlot(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: c63300ff manual
// OFFSET: 0x00589600
static int32_t Script_GiveMasterLoot(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GiveMasterLoot(slot, index)");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    WOWGUID target = GetMasterLootCandidate(static_cast<int32_t>(lua_tonumber(L, 2)) - 1);

    if (target) {
        CGLootInfo::GiveMasterLoot(slot, target);
    }

    return 0;
}

// VERIFY: 9813f818 manual
// OFFSET: 0x00588F00
static int32_t Script_SetLootPortrait(lua_State* L) {
    CSimpleTexture* texture = static_cast<CSimpleTexture*>(FrameScript_GetObjectThis(L, CSimpleTexture::GetObjectType()));
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGLootInfo::m_lootObj, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        RenderPortraitTexture(texture, unit);
        lua_pushnumber(L, 1.0);
    } else {
        texture->SetTexture(nullptr);
        lua_pushnil(L);
    }

    return 1;
}

// STATIC: 0x00ACEE28
static FrameScript_Method s_ScriptFunctions[] = {
    { "SetLootPortrait", &Script_SetLootPortrait },
    { "GetNumLootItems", &Script_GetNumLootItems },
    { "GetLootSlotInfo", &Script_GetLootSlotInfo },
    { "GetLootSlotLink", &Script_GetLootSlotLink },
    { "LootSlotIsItem", &Script_LootSlotIsItem },
    { "LootSlotIsCoin", &Script_LootSlotIsCoin },
    { "LootSlot", &Script_LootSlot },
    { "ConfirmLootSlot", &Script_ConfirmLootSlot },
    { "CloseLoot", &Script_CloseLoot },
    { "IsFishingLoot", &Script_IsFishingLoot },
    { "GetMasterLootCandidate", &Script_GetMasterLootCandidate },
    { "GiveMasterLoot", &Script_GiveMasterLoot },
    { "GetLootRollItemInfo", &Script_GetLootRollItemInfo },
    { "GetLootRollItemLink", &Script_GetLootRollItemLink },
    { "GetLootRollTimeLeft", &Script_GetLootRollTimeLeft },
    { "RollOnLoot", &Script_RollOnLoot },
    { "ConfirmLootRoll", &Script_ConfirmLootRoll },
};

// VERIFY: 25122094 auto
// OFFSET: 0x00588E60
void LootInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 2186206f auto
// OFFSET: 0x00588E90
void LootInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
