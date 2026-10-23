#include "EquipmentManager.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include <cstdlib>
#include <cstring>
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/UIMacros.h"

// VERIFY: 53148363 manual
// OFFSET: 0x005AD770
static int32_t Script_EquipmentManagerIgnoreSlotForSave(lua_State* L) {
    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 18) {
        luaL_error(L, "Usage: EquipmentManagerIgnoreSlotForSave(slot)");
        return 0;
    }

    CGEquipmentManager::m_ignoredSlots[slot] = 1;
    return 0;
}

// VERIFY: 7ab2e1ba manual
// OFFSET: 0x005AD830
static int32_t Script_EquipmentManagerUnignoreSlotForSave(lua_State* L) {
    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 18) {
        luaL_error(L, "Usage: UpdatePendingEquipmentSetSlot(slot)");
        return 0;
    }

    CGEquipmentManager::m_ignoredSlots[slot] = 0;
    return 0;
}

// VERIFY: 8b0d091f manual
// OFFSET: 0x005AD7C0
static int32_t Script_EquipmentManagerIsSlotIgnoredForSave(lua_State* L) {
    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 18) {
        luaL_error(L, "Usage: EquipmentSetIsSlotIgnoredForSave(slot)");
        return 0;
    }

    lua_pushboolean(L, CGEquipmentManager::m_ignoredSlots[slot] != 0);
    return 1;
}

// VERIFY: 9e4abd85 manual
// OFFSET: 0x005AD880
static int32_t Script_CanUseEquipmentSets(lua_State* L) {
    lua_pushnumber(L, CGEquipmentManager::CanUseEquipmentSets(true));
    return 1;
}

// VERIFY: c64c67de auto
// OFFSET: 0x005AD820
static int32_t Script_EquipmentManagerClearIgnoredSlotsForSave(lua_State* L) {
    CGEquipmentManager::ClearIgnoredSlots();
    return 0;
}

// VERIFY: 6bd6b519 auto
// OFFSET: 0x005AE800
static int32_t Script_DeleteEquipmentSet(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: DeleteEquipmentSet(\"setName\")");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (name && *name) {
        EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(name);

        if (set) {
            CGEquipmentManager::DeleteEquipmentSet(set);
        }
    }

    return 0;
}

// VERIFY: c7766f46 manual
// OFFSET: 0x005AE860
static int32_t Script_GetEquipmentSetLocations(lua_State* L) {
    if (!lua_isstring(L, 1) || (lua_type(L, 2) > LUA_TNIL && lua_type(L, 2) != LUA_TTABLE)) {
        luaL_error(L, "Usage: GetEquipmentSetLocations(\"setName\" [, returnTable])");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(lua_tolstring(L, 1, nullptr));

    if (!set) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (lua_type(L, 2) == LUA_TTABLE) {
        lua_settop(L, 2);
    } else {
        lua_createtable(L, 0, 0);
    }

    int32_t pending[19];
    int32_t count = 0;

    for (int32_t slot = 0; slot <= 18; slot++) {
        WOWGUID guid = set->m_items[slot];

        if (guid) {
            if (guid.m_guid > 1) {
                pending[count++] = slot;
                continue;
            }

            lua_checkstack(L, 2);
            lua_pushnumber(L, slot + 1);
            lua_pushnumber(L, static_cast<int32_t>(set->m_items[slot].m_guid));
        } else {
            lua_checkstack(L, 2);
            lua_pushnumber(L, slot + 1);
            lua_pushnumber(L, 0.0);
        }

        lua_settable(L, -3);
    }

    CGBag_C* inventory = player->GetBag();

    for (int32_t i = 0; i <= 18; i++) {
        CGItem_C* item = inventory->GetItemPointer(i);

        if (!item) {
            continue;
        }

        for (int32_t j = 0; j < count; j++) {
            if (set->m_items[pending[j]] == item->m_obj->m_guid) {
                lua_checkstack(L, 2);
                lua_pushnumber(L, pending[j] + 1);
                lua_pushnumber(L, i + 0x100001);
                lua_settable(L, -3);

                if (j + 1 < count) {
                    memmove(&pending[j], &pending[j + 1], sizeof(int32_t) * (count - (j + 1)));
                }

                count--;
                break;
            }
        }
    }

    if (!count) {
        return 1;
    }

    for (int32_t i = 23; i <= 38; i++) {
        CGItem_C* item = inventory->GetItemPointer(i);

        if (!item) {
            continue;
        }

        for (int32_t j = 0; j < count; j++) {
            if (set->m_items[pending[j]] == item->m_obj->m_guid) {
                lua_checkstack(L, 2);
                lua_pushnumber(L, pending[j] + 1);
                lua_pushnumber(L, i + 0x2FFFEA);
                lua_settable(L, -3);

                if (j + 1 < count) {
                    memmove(&pending[j], &pending[j + 1], sizeof(int32_t) * (count - (j + 1)));
                }

                count--;
                break;
            }
        }
    }

    if (!count) {
        return 1;
    }

    for (uint32_t bagIndex = 0; bagIndex < 4; bagIndex++) {
        CGObject_C* container = ClntObjMgrObjectPtr(GetBagAtIndex(bagIndex), TYPE_CONTAINER, __FILE__, __LINE__);

        if (!container) {
            continue;
        }

        CGBag_C* bag = container->GetBag();
        int32_t location = ((bagIndex + 1) << 8) | 0x300000;

        for (int32_t k = 0; k <= bag->m_numSlots; k++) {
            CGItem_C* item = bag->GetItemPointer(k);

            if (!item) {
                continue;
            }

            for (int32_t j = 0; j < count; j++) {
                if (set->m_items[pending[j]] == item->m_obj->m_guid) {
                    lua_checkstack(L, 2);
                    lua_pushnumber(L, pending[j] + 1);
                    lua_pushnumber(L, k + location + 1);
                    lua_settable(L, -3);

                    if (j + 1 < count) {
                        memmove(&pending[j], &pending[j + 1], sizeof(int32_t) * (count - (j + 1)));
                    }

                    count--;
                    break;
                }
            }
        }
    }

    if (!count) {
        return 1;
    }

    if (CGBankInfo::m_banker) {
        for (int32_t i = 39; i <= 66; i++) {
            CGItem_C* item = inventory->GetItemPointer(i);

            if (!item) {
                continue;
            }

            for (int32_t j = 0; j < count; j++) {
                if (set->m_items[pending[j]] == item->m_obj->m_guid) {
                    lua_checkstack(L, 2);
                    lua_pushnumber(L, pending[j] + 1);
                    lua_pushnumber(L, i + 0x400001);
                    lua_settable(L, -3);

                    if (j + 1 < count) {
                        memmove(&pending[j], &pending[j + 1], sizeof(int32_t) * (count - (j + 1)));
                    }

                    count--;
                    break;
                }
            }
        }

        for (uint32_t bagIndex = 4; bagIndex < 11; bagIndex++) {
            CGObject_C* container = ClntObjMgrObjectPtr(GetBagAtIndex(bagIndex), TYPE_CONTAINER, __FILE__, __LINE__);

            if (!container) {
                continue;
            }

            CGBag_C* bag = container->GetBag();

            for (int32_t k = 0; k <= bag->m_numSlots; k++) {
                CGItem_C* item = bag->GetItemPointer(k);

                if (!item) {
                    continue;
                }

                int32_t location = ((bagIndex - 3) | 0x6000) << 8;

                for (int32_t j = 0; j < count; j++) {
                    if (set->m_items[pending[j]] == item->m_obj->m_guid) {
                        lua_checkstack(L, 2);
                        lua_pushnumber(L, pending[j] + 1);
                        lua_pushnumber(L, k + location + 1);
                        lua_settable(L, -3);

                        if (j + 1 < count) {
                            memmove(&pending[j], &pending[j + 1], sizeof(int32_t) * (count - (j + 1)));
                        }

                        count--;
                        break;
                    }
                }
            }
        }
    }

    for (int32_t i = 0; i < count; i++) {
        lua_checkstack(L, 2);
        lua_pushnumber(L, pending[i] + 1);
        lua_pushnumber(L, -1.0);
        lua_settable(L, -3);
    }

    return 1;
}

// VERIFY: c5f296f6 manual
// OFFSET: 0x005AEF90
static int32_t Script_GetEquipmentSetItemIDs(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetEquipmentSetItemIDs(\"setName\" [, returnTable])");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(lua_tolstring(L, 1, nullptr));

    if (!set) {
        return 0;
    }

    if (lua_type(L, 2) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    for (int32_t slot = 0; slot < 19; slot++) {
        WOWGUID guid = set->m_items[slot];
        double value;

        if (!static_cast<uint32_t>(guid.m_guid)) {
            lua_checkstack(L, 2);
            lua_pushnumber(L, slot + 1);
            value = 0.0;
        } else if (guid.m_guid <= 1) {
            lua_checkstack(L, 2);
            lua_pushnumber(L, slot + 1);
            value = static_cast<int32_t>(guid.m_guid);
        } else {
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));
            lua_checkstack(L, 2);
            lua_pushnumber(L, slot + 1);
            value = item ? static_cast<int32_t>(item->m_obj->m_entryID) : -1.0;
        }

        lua_pushnumber(L, value);
        lua_settable(L, -3);
    }

    return 1;
}

// VERIFY: 82bfbd36 manual
// OFFSET: 0x005AF0D0
static int32_t Script_GetNumEquipmentSets(lua_State* L) {
    int32_t count = 0;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        count++;
    }

    lua_pushnumber(L, count);
    return 1;
}

// STATIC: 0x00ACFB74
static const char* s_equipmentSetIconPath = "Interface\\Icons\\";

// VERIFY: 4c8d1592 manual
// OFFSET: 0x005AF120
static int32_t Script_GetEquipmentSetInfo(lua_State* L) {
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1));

    if (static_cast<uint32_t>(index - 1) > 9) {
        luaL_error(L, "Usage: GetEquipmentSetInfo(setIndex)");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByIndex(index - 1);

    if (!set) {
        return 0;
    }

    lua_pushstring(L, set->m_name);
    char icon[260];
    SStrPrintf(icon, sizeof(icon), "%s%s", s_equipmentSetIconPath, set->m_icon);
    lua_pushstring(L, icon);
    lua_pushnumber(L, set->m_id);
    return 3;
}

// VERIFY: 43db9cf0 auto
// OFFSET: 0x005AF1D0
static int32_t Script_GetEquipmentSetInfoByName(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetEquipmentSetInfoByName(\"setName\")");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (!name || !*name) {
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(name);

    if (!set) {
        return 0;
    }

    lua_pushstring(L, set->m_icon);
    lua_pushnumber(L, set->m_id);
    return 2;
}

// VERIFY: 6bce95fb auto
// OFFSET: 0x005AF250
static int32_t Script_PickupEquipmentSetByName(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: PickupEquipmentSetByName(\"setName\")");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(14)) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (name && *name) {
        EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(name);

        if (set) {
            CGGameUI::SetCursorEquipmentSet(set->m_id);
        }
    }

    return 0;
}

// VERIFY: 178e5e18 auto
// OFFSET: 0x005AF2C0
static int32_t Script_PickupEquipmentSet(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupEquipmentSet(setIndex)");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByIndex(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);

    if (set) {
        CGGameUI::SetCursorEquipmentSet(set->m_id);
    }

    return 0;
}

// VERIFY: 78933462 auto
// OFFSET: 0x005AF320
static int32_t Script_EquipmentSetContainsLockedItems(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: EquipmentSetContainsLockedItems(\"setName\")");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(lua_tolstring(L, 1, nullptr));

    if (!set || !CGEquipmentManager::SetContainsLockedItems(set)) {
        return 0;
    }

    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: 14344e5d auto
// OFFSET: 0x005AF380
static int32_t Script_UseEquipmentSet(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: UseEquipmentSet(\"setName\")");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(lua_tolstring(L, 1, nullptr));

    if (!set) {
        return 0;
    }

    if (CGEquipmentManager::UseEquipmentSet(set)) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: bc671bf0 auto
// OFFSET: 0x005AF910
static int32_t Script_RenameEquipmentSet(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: RenameEquipmentSet(\"oldName\", \"newName\")");
        return 0;
    }

    const char* oldName = lua_tolstring(L, 1, nullptr);
    const char* newName = lua_tolstring(L, 2, nullptr);
    char name[128];

    if (!ScrubString(newName, name, 32, sizeof(name), 1)) {
        luaL_error(L, "Invalid string for setName");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(oldName);

    if (set) {
        CGEquipmentManager::ModifyEquipmentSet(set, name);
    }

    return 0;
}

// VERIFY: e2bb42ab manual
// OFFSET: 0x005AF9C0
static int32_t Script_SaveEquipmentSet(lua_State* L) {
    if (!CGEquipmentManager::m_loaded) {
        return 0;
    }

    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SaveEquipmentSet(\"setName\", iconIndex)");
        return 0;
    }

    const char* setName = lua_tolstring(L, 1, nullptr);
    int32_t iconIndex = static_cast<int32_t>(lua_tonumber(L, 2));
    char name[128];

    if (!ScrubString(setName, name, 32, sizeof(name), 1)) {
        luaL_error(L, "Invalid string for setName");
        return 0;
    }

    char icon[64] = { 0 };

    if (iconIndex >= 0) {
        uint32_t index = iconIndex - 1;

        if (index < GetNumMacroIcons()) {
            SStrCopy(icon, GetMacroIcon(index), STORM_MAX_STR);
        }
    } else {
        int32_t slot = abs(iconIndex) - 1;

        if (slot <= 18) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                uint32_t displayId = 0;
                const VisibleItem_C* visible = player->GetVisibleItem(slot);

                if (visible && abs(visible->m_itemId)) {
                    displayId = const_cast<VisibleItem_C*>(visible)->GetDisplayID();
                } else {
                    CGItem_C* item = player->GetBag()->GetItemPointer(slot);

                    if (item && !(item->m_modelFlags & 0x10000)) {
                        displayId = item->GetDisplayID();
                    }
                }

                if (displayId) {
                    SStrCopy(icon, CGItem_C::GetInventoryArt(displayId), STORM_MAX_STR);
                }
            }
        }
    }

    if (!icon[0]) {
        SStrCopy(icon, "INV_Misc_QuestionMark", STORM_MAX_STR);
    }

    if (CGEquipmentManager::HasSet(name)) {
        CGEquipmentManager::SaveSetByName(name, icon);
        return 0;
    }

    if (!CGEquipmentManager::CanCreateSet()) {
        luaL_error(L, "Too many sets! You can only have %d sets", 10);
        return 0;
    }

    CGEquipmentManager::CreateEquipmentSet(name, icon);
    return 0;
}

// STATIC: 0x00ACFB78
static FrameScript_Method s_ScriptFunctions[] = {
    { "SaveEquipmentSet", &Script_SaveEquipmentSet },
    { "DeleteEquipmentSet", &Script_DeleteEquipmentSet },
    { "RenameEquipmentSet", &Script_RenameEquipmentSet },
    { "EquipmentManagerIgnoreSlotForSave", &Script_EquipmentManagerIgnoreSlotForSave },
    { "EquipmentManagerIsSlotIgnoredForSave", &Script_EquipmentManagerIsSlotIgnoredForSave },
    { "EquipmentManagerClearIgnoredSlotsForSave", &Script_EquipmentManagerClearIgnoredSlotsForSave },
    { "EquipmentManagerUnignoreSlotForSave", &Script_EquipmentManagerUnignoreSlotForSave },
    { "GetEquipmentSetLocations", &Script_GetEquipmentSetLocations },
    { "GetEquipmentSetItemIDs", &Script_GetEquipmentSetItemIDs },
    { "GetNumEquipmentSets", &Script_GetNumEquipmentSets },
    { "GetEquipmentSetInfo", &Script_GetEquipmentSetInfo },
    { "GetEquipmentSetInfoByName", &Script_GetEquipmentSetInfoByName },
    { "EquipmentSetContainsLockedItems", &Script_EquipmentSetContainsLockedItems },
    { "PickupEquipmentSetByName", &Script_PickupEquipmentSetByName },
    { "PickupEquipmentSet", &Script_PickupEquipmentSet },
    { "UseEquipmentSet", &Script_UseEquipmentSet },
    { "CanUseEquipmentSets", &Script_CanUseEquipmentSets },
};

// VERIFY: 2ca3f5dd auto
// OFFSET: 0x005AD8F0
void EquipmentManagerRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 9c68a83a auto
// OFFSET: 0x005AD920
void EquipmentManagerUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
