#include "PetitionVendor.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"

// VERIFY: f68ff2aa manual
// OFFSET: 0x005A1F70
static int32_t Script_GetNumPetitionItems(lua_State* L) {
    lua_pushnumber(L, CGPetitionVendorInfo::m_numPetitionItems);
    return 1;
}

// VERIFY: b1f3e113 manual
// OFFSET: 0x005A2600
static int32_t Script_TurnInPetition(lua_State* L) {
    if (!CGPetitionVendorInfo::m_vendor) {
        return 0;
    }

    WOWGUID item = CGPetitionVendorInfo::m_petitionItem;

    if (item) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_TURN_IN_PETITION));
        msg.Put(item.m_guid);
        msg.Put(static_cast<uint32_t>(-1));
        msg.Put(static_cast<uint32_t>(-1));
        msg.Put(static_cast<uint32_t>(-1));
        msg.Put(static_cast<uint32_t>(-1));
        msg.Put(static_cast<uint32_t>(-1));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: f93a14c6 manual
// OFFSET: 0x005A1FB0
static int32_t Script_GetPetitionItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPetitionItemInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    GuildPetitionInfo* petition;
    WOWGUID none;

    if (index < CGPetitionVendorInfo::m_numPetitionItems && (petition = &CGPetitionVendorInfo::m_petitionItems[index]) && petition->m_itemID && (none = WOWGUID(), g_itemDBCache.GetRecord(petition->m_itemID, none, PetitionVendorItemCallback, nullptr, false))) {
        char name[256];
        CGItem_C::BuildItemName(name, sizeof(name), petition->m_itemID, 0);
        lua_pushstring(L, name);
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        char texture[260];
        SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(petition->m_displayID));
        lua_pushstring(L, texture);
        lua_pushnumber(L, petition->m_cost);
        return 3;
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    return 3;
}

// VERIFY: a2e0308f auto
// OFFSET: 0x005A2200
static int32_t Script_ClickPetitionButton(lua_State* L) {
    WOWGUID item;
    WOWGUID container;
    uint32_t slot;
    CGGameUI::GetCursorItem(item, container, slot);
    WOWGUID previous = CGPetitionVendorInfo::m_petitionItem;
    WOWGUID previousContainer = CGPetitionVendorInfo::m_petitionItemContainer;
    uint8_t previousSlot = CGPetitionVendorInfo::m_petitionItemSlot;

    if (CGPetitionVendorInfo::ClickPetitionButton(item, container, slot)) {
        CGGameUI::ClearCursor(0, 1);

        if (previous) {
            CGGameUI::SetCursorItem(previous, previousContainer, previousSlot, 1, 0, 1);
        }
    }

    return 0;
}

// VERIFY: f219737d auto
// OFFSET: 0x005A24F0
static int32_t Script_ClosePetitionVendor(lua_State* L) {
    if (CGPetitionVendorInfo::m_vendor) {
        CGGameUI::ClearInteractTarget(CGPetitionVendorInfo::m_vendor, 0);
        CGPetitionVendorInfo::m_vendor = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_CLOSED, nullptr);
    }

    return 0;
}

// VERIFY: c105ad23 manual
// OFFSET: 0x005A2530
static int32_t Script_BuyPetition(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: BuyPetition(index, name)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* name = lua_tolstring(L, 2, nullptr);
    VALIDATE_NAME_RESULT error = ValidateGuildName(s_locale, name);

    if (error) {
        CGGameUI::DisplayNameError(error);
        lua_pushnil(L);
    } else {
        CGPetitionVendorInfo::BuyPetition(index, name);
        lua_pushnumber(L, 1.0);
    }

    return 1;
}

// VERIFY: 73187861 manual
// OFFSET: 0x005A26D0
static int32_t Script_TurnInArenaPetition(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TurnInArenaPetition(teamSize)");
        return 0;
    }

    if (!CGPetitionVendorInfo::m_vendor) {
        return 0;
    }

    uint32_t background = 0xFFFFFFFF;
    uint32_t emblemColor = 0xFFFFFFFF;
    uint32_t borderStyle = 0xFFFFFFFF;
    uint32_t emblemStyle;
    uint32_t borderColor;
    uint32_t teamSize = static_cast<uint32_t>(lua_tonumber(L, 1));
    uint32_t i = 2;

    while (lua_isnumber(L, i)) {
        if (++i > 12) {
            break;
        }
    }

    if (i > 12) {
        CImVector color = CImVector(0, 0, 0, 0);
        FrameScript_GetColorNoAlpha(L, 2, color);
        background = *reinterpret_cast<uint32_t*>(&color);
        emblemStyle = static_cast<int32_t>(lua_tonumber(L, 5));
        FrameScript_GetColorNoAlpha(L, 6, color);
        emblemColor = *reinterpret_cast<uint32_t*>(&color);
        borderStyle = static_cast<int32_t>(lua_tonumber(L, 9));
        FrameScript_GetColorNoAlpha(L, 10, color);
        borderColor = *reinterpret_cast<uint32_t*>(&color);
    } else {
        emblemStyle = background;
        borderColor = background;
    }

    CGPetitionVendorInfo::m_arenaTeamSize = teamSize;
    CGPetitionVendorInfo::m_arenaBackgroundColor = background;
    CGPetitionVendorInfo::m_arenaEmblemStyle = emblemStyle;
    CGPetitionVendorInfo::m_arenaEmblemColor = emblemColor;
    CGPetitionVendorInfo::m_arenaBorderStyle = borderStyle;
    CGPetitionVendorInfo::m_arenaBorderColor = borderColor;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->TurnInArenaCharter(1);
    }

    return 0;
}

// VERIFY: ab7f0d73 manual
// OFFSET: 0x005A2820
static int32_t Script_HasFilledPetition(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->TurnInArenaCharter(0)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// STATIC: 0x00ACF7F0
static FrameScript_Method s_ScriptFunctions[] = {
    { "ClosePetitionVendor", &Script_ClosePetitionVendor },
    { "GetNumPetitionItems", &Script_GetNumPetitionItems },
    { "GetPetitionItemInfo", &Script_GetPetitionItemInfo },
    { "BuyPetition", &Script_BuyPetition },
    { "ClickPetitionButton", &Script_ClickPetitionButton },
    { "TurnInPetition", &Script_TurnInPetition },
    { "TurnInArenaPetition", &Script_TurnInArenaPetition },
    { "HasFilledPetition", &Script_HasFilledPetition },
};

// VERIFY: b54a5949 auto
// OFFSET: 0x005A20F0
void PetitionVendorRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 3fbc64fe auto
// OFFSET: 0x005A2120
void PetitionVendorUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
