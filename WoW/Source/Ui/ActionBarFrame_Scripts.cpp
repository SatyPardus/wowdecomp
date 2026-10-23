#include "ActionBarFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Ui/EquipmentManager.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/DB/Rec/TotemCategoryRec.h"

// VERIFY: 67a7662a manual
// OFFSET: 0x005A7E60
static int32_t Script_IsUsableAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsUsableAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (slot >= 0 && slot < sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0]) && CGActionBar::m_slotUsable[slot]) {
        if (CGActionBar::m_slotNoMana[slot]) {
            lua_pushnil(L);
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnumber(L, 1.0);
            lua_pushnil(L);
        }

        return 2;
    }

    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

// VERIFY: 4790f532 manual
// OFFSET: 0x005A8220
static int32_t Script_HasAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: HasAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGActionBar::HasAction(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5b1d9a76 manual
// OFFSET: 0x005A8F10
static int32_t Script_GetActionInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetActionInfo(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t action = CGActionBar::m_slotActions[slot - 1];

    if (action && !(action & 0xF0000000)) {
        int32_t petSpell = 0;
        int32_t spellId = CGActionBar::GetSpell(slot - 1, petSpell);
        SpellRec spell;
        g_spellDB.GetRecord(spellId, &spell);

        if (SpellIsCallCompanion(&spell)) {
            int32_t type = CGSpellBook::GetCompanionType(&spell);
            lua_pushstring(L, "companion");
            lua_pushnumber(L, CGSpellBook::FindCompanionSlotByID(spellId, type) + 1);

            if (type == 0) {
                lua_pushstring(L, "CRITTER");
            } else if (type == 1) {
                lua_pushstring(L, "MOUNT");
            } else {
                lua_pushstring(L, "UNKNOWN");
            }
        } else {
            lua_pushstring(L, "spell");
            lua_pushnumber(L, GetSpellSlot(spellId, petSpell) + 1);
            lua_pushstring(L, petSpell ? "pet" : "spell");
        }

        lua_pushnumber(L, spellId);
        return 4;
    }

    if (action && (action & 0xF0000000) == 0x10000000) {
        int32_t spellId = CGActionBar::GetPetActionSpell(slot - 1);
        lua_pushstring(L, "spell");
        lua_pushnumber(L, GetSpellSlot(spellId, 1) + 1);
        lua_pushstring(L, "pet");
        lua_pushnumber(L, spellId);
        return 4;
    }

    if (CGActionBar::IsItem(slot - 1)) {
        lua_pushstring(L, "item");
        lua_pushnumber(L, CGActionBar::GetItem(slot - 1));
        return 2;
    }

    if (CGActionBar::IsMacro(slot - 1)) {
        lua_pushstring(L, "macro");
        lua_pushnumber(L, CGUIMacros::GetMacroIndexByID(CGActionBar::GetMacroID(slot - 1)) + 1);
        return 2;
    }

    if (!CGActionBar::IsEquipmentSet(slot - 1)) {
        return 0;
    }

    const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(CGActionBar::GetEquipmentSetID(slot - 1));

    if (!set) {
        return 0;
    }

    lua_pushstring(L, "equipmentset");
    lua_pushstring(L, set->m_name);
    return 2;
}

// VERIFY: 850abda9 auto
// OFFSET: 0x005A91C0
static int32_t Script_GetActionCooldown(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetActionCooldown(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t start;
    uint32_t duration;
    uint32_t enable = 0;
    CGActionBar::GetCooldown(slot - 1, start, duration, enable);
    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: bfeed94d manual
// OFFSET: 0x005A9B30
static int32_t Script_GetActionTexture(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetActionTexture(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    const char* texture = CGActionBar::GetTexture(slot - 1);

    if (texture) {
        lua_pushstring(L, texture);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: f8a66885 manual
// OFFSET: 0x005AAD40
static int32_t Script_IsCurrentAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsCurrentAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGActionBar::IsCurrentAction(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 8e7de78a manual
// OFFSET: 0x005AC090
static int32_t Script_PickupAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupAction(slot)");
        return 0;
    }

    CGActionBar::PickupAction(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: 215bdca3 manual
// OFFSET: 0x005AC000
static int32_t Script_UseAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: UseAction(slot, [, target] [, button])");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    WOWGUID target;

    if (Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true)) {
        CGActionBar::UseAction(slot - 1, target, lua_tolstring(L, 3, nullptr));
    }

    return 0;
}

// VERIFY: 9530d833 auto
// OFFSET: 0x005A7D90
static int32_t Script_GetActionText(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetActionText(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t action = CGActionBar::m_slotActions[slot - 1];

    if (action) {
        if ((action & 0xF0000000) == 0x40000000) {
            MACRONODE* macro = CGUIMacros::GetMacroByID(action & 0xBFFFFFFF);

            if (macro) {
                lua_pushstring(L, macro->m_name);
                return 1;
            }
        } else if ((action & 0xF0000000) == 0x20000000) {
            const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(CGActionBar::GetEquipmentSetID(slot - 1));

            if (set) {
                lua_pushstring(L, set->m_name);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: b41a956b manual
// OFFSET: 0x005A7F20
static int32_t Script_GetBonusBarOffset(lua_State* L) {
    lua_pushnumber(L, CGActionBar::m_bonusBarOffset);
    return 1;
}

// VERIFY: ce8c7e12 auto
// OFFSET: 0x005A7FD0
static int32_t Script_GetActionBarPage(lua_State* L) {
    if (CGActionBar::m_tempPageActiveFlags) {
        lua_pushinteger(L, 1);
    } else {
        lua_pushinteger(L, CGActionBar::m_currentPage + 1);
    }

    return 1;
}

// VERIFY: 0c959dd9 manual
// OFFSET: 0x005AB840
static int32_t Script_PlaceAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PlaceAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (slot >= 120 && static_cast<uint32_t>(slot) < 132) {
        return 0;
    }

    CGActionBar::PutActionInSlot(slot);
    return 0;
}

// VERIFY: ce325702 auto
// OFFSET: 0x005A7F60
static int32_t Script_ChangeActionBarPage(lua_State* L) {
    int32_t page = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (page < 0 || static_cast<uint32_t>(page) >= 6) {
        luaL_error(L, "ChangeActionBarPage() needs a page in the range 1 to %d", 6);
        return 0;
    }

    if (!CGGameUI::CanPerformAction(14) || static_cast<uint32_t>(page) == CGActionBar::m_currentPage) {
        return 0;
    }

    CGActionBar::m_currentPage = page;
    FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_PAGE_CHANGED, nullptr);
    return 0;
}

// VERIFY: ceaea837 manual
// OFFSET: 0x005A7D10
static int32_t Script_GetActionCount(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetActionCount(slot)");
        return 0;
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t count = slot > 143 ? 0 : CGActionBar::m_slotItemCount[slot];
    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 040fd28e manual
// OFFSET: 0x005A8790
static int32_t Script_GetActionBarToggles(lua_State* L) {
    uint8_t toggles = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        toggles = player->m_player->m_playerFieldBytes[2];
    }

    lua_checkstack(L, 4);

    for (uint32_t i = 0; i < 4; i++) {
        if ((1 << i) & toggles) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }
    }

    return 4;
}

// VERIFY: 7688b79a manual
// OFFSET: 0x005A8290
static int32_t Script_SetActionBarToggles(lua_State* L) {
    uint8_t toggles = 0;

    for (int32_t i = 0; i < 4; i++) {
        if (StringToBOOL(L, i + 1, 0)) {
            toggles |= 1 << i;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_ACTION_BAR_TOGGLES));
    msg.Put(toggles);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 143d6bd7 manual
// OFFSET: 0x005A8BC0
static int32_t Script_IsEquippedAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsEquippedAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGActionBar::IsEquippedAction(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: b8c643c7 manual
// OFFSET: 0x005A9C80
static int32_t Script_IsConsumableAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsConsumableAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGActionBar::IsConsumableAction(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 58dd316d manual
// OFFSET: 0x005A8720
static int32_t Script_IsStackableAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsStackableAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (IsStackableAucItem(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7b77eb8c auto
// OFFSET: 0x005A8820
static int32_t Script_IsPossessBarVisible(lua_State* L) {
    int32_t spellId = CGPetInfo::m_possessSpell;

    if (!spellId) {
        lua_pushboolean(L, spellId);
        return spellId + 1;
    }

    SpellRec spell;
    g_spellDB.GetRecord(spellId, &spell);
    lua_pushboolean(L, !SpellRec::HasAura(&spell, 236));
    return 1;
}

// VERIFY: 690fc1a3 auto
// OFFSET: 0x005B71E0
static int32_t Script_GetMultiCastBarOffset(lua_State* L) {
    lua_pushnumber(L, 6.0);
    return 1;
}

// VERIFY: 1c1ed01b manual
// OFFSET: 0x005A9BA0
static int32_t Script_IsAttackAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsAttackAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGActionBar::IsAttackAction(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 394bbc6f manual
// OFFSET: 0x005A9C10
static int32_t Script_IsAutoRepeatAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsAutorepeatAction(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (CGActionBar::IsAutoRepeatAction(slot - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a6dadaad manual
// OFFSET: 0x005A9D50
static int32_t Script_IsActionInRange(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
        int32_t inRange = 0;
        WOWGUID target = WOWGUID();
        Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, false);

        if (CGActionBar::HasValidTarget(slot - 1, target, &inRange)) {
            lua_pushnumber(L, inRange ? 1.0 : 0.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 795b8c46 manual
// OFFSET: 0x005A8330
static int32_t Script_GetMultiCastTotemSpells(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMultiCastTotemSpells(slot)");
        return 0;
    }

    uint32_t slot = (lua_tointeger(L, 1) - 1) & 3;
    uint32_t count = 0;

    if (slot < 4) {
        TSGrowableArray<int32_t>* spells = &CGSpellBook::m_totemSpells[slot];

        if (spells) {
            count = spells->m_count;

            for (uint32_t i = 0; i < count; i++) {
                lua_pushinteger(L, spells->m_data[i]);
            }
        }
    }

    return count;
}

// VERIFY: 48073d98 manual
// OFFSET: 0x005A9290
static int32_t Script_GetActionAutocast(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetActionAutocast(slot)");
        return 0;
    }

    double slot = lua_tonumber(L, 1);
    PetAction* action = nullptr;
    int32_t petSpell;
    int32_t spellId = CGActionBar::GetSpell(static_cast<uint32_t>(slot - 1.0), petSpell);

    if (petSpell && (action = CGPetInfo::GetPetSpellAction(spellId)) && static_cast<int32_t>(action->m_data) < 0) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (action && (action->m_data & 0x40000000)) {
        lua_pushnumber(L, 1.0);
        return 2;
    }

    lua_pushnil(L);
    return 2;
}

// VERIFY: 3af652b3 auto
// OFFSET: 0x005A9CF0
static int32_t Script_ActionHasRange(lua_State* L) {
    if (lua_isnumber(L, 1) && CGActionBar::ActionHasRange(static_cast<int32_t>(lua_tonumber(L, 1)) - 1)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 583a8b5c manual
// OFFSET: 0x005AB8A0
static int32_t Script_SetMultiCastSpell(lua_State* L) {
    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetMultiCastSpell(slot, spellID)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(14)) {
        return 0;
    }

    int32_t slot = lua_tointeger(L, 1) - 1;

    if (slot < 132 || static_cast<uint32_t>(slot) >= 144) {
        return 0;
    }

    int32_t spellId = lua_tointeger(L, 2);

    if (!spellId) {
        CGActionBar::RemoveAction(slot);
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    int32_t category;

    if (IsMultiCastSpellForSlot(slot, &spell, &category)) {
        CGActionBar::SetAction(slot, spellId, 1, 0);
        return 0;
    }

    TotemCategoryRec* totem = g_totemCategoryDB.GetRecord(category);

    if (totem) {
        CGGameUI::DisplayError(ERR_MULTI_CAST_ACTION_TOTEM_S, totem->m_name);
    }

    return 0;
}

// STATIC: 0x00ACF9E0
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetActionInfo", &Script_GetActionInfo },
    { "GetActionTexture", &Script_GetActionTexture },
    { "GetActionCount", &Script_GetActionCount },
    { "GetActionCooldown", &Script_GetActionCooldown },
    { "GetActionAutocast", &Script_GetActionAutocast },
    { "GetActionText", &Script_GetActionText },
    { "HasAction", &Script_HasAction },
    { "UseAction", &Script_UseAction },
    { "PickupAction", &Script_PickupAction },
    { "PlaceAction", &Script_PlaceAction },
    { "IsAttackAction", &Script_IsAttackAction },
    { "IsCurrentAction", &Script_IsCurrentAction },
    { "IsAutoRepeatAction", &Script_IsAutoRepeatAction },
    { "IsUsableAction", &Script_IsUsableAction },
    { "IsConsumableAction", &Script_IsConsumableAction },
    { "IsStackableAction", &Script_IsStackableAction },
    { "IsEquippedAction", &Script_IsEquippedAction },
    { "ActionHasRange", &Script_ActionHasRange },
    { "IsActionInRange", &Script_IsActionInRange },
    { "GetBonusBarOffset", &Script_GetBonusBarOffset },
    { "GetMultiCastBarOffset", &Script_GetMultiCastBarOffset },
    { "ChangeActionBarPage", &Script_ChangeActionBarPage },
    { "GetActionBarPage", &Script_GetActionBarPage },
    { "GetActionBarToggles", &Script_GetActionBarToggles },
    { "SetActionBarToggles", &Script_SetActionBarToggles },
    { "IsPossessBarVisible", &Script_IsPossessBarVisible },
    { "GetMultiCastTotemSpells", &Script_GetMultiCastTotemSpells },
    { "SetMultiCastSpell", &Script_SetMultiCastSpell },
};

// VERIFY: 3766f2a3 auto
// OFFSET: 0x005A8020
void ActionBarRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: f87c01bd auto
// OFFSET: 0x005A8050
void ActionBarUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
