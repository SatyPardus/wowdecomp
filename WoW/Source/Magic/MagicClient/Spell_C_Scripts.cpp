#include "WoW/Source/Magic/Types.h"
#include "Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// VERIFY: bf97997d auto
// OFFSET: 0x007FDCD0
static int32_t Script_SpellIsTargeting(lua_State* L) {
    if (s_targetingSpell) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2d5e9a32 auto
// OFFSET: 0x007FDD10
static int32_t Script_SpellCanTargetItem(lua_State* L) {
    if (s_targetingSpell && (s_needTargets & 0x4010)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: caa8802f manual
// OFFSET: 0x007FDD60
static int32_t Script_SpellCanTargetGlyph(lua_State* L) {
    if (s_targetingSpell && (s_needTargets & 0x20000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 537b33f9 manual
// OFFSET: 0x00802270
static int32_t Script_CannotBeResurrected(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (player->m_auraEffectMask[314 / 8] & (1 << (314 % 8)))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7914559b manual
// OFFSET: 0x008007E0
static int32_t Script_SpellTargetItem(lua_State* L) {
    if (!CGGameUI::CanPerformAction(2) || !s_targetingSpell || !(s_needTargets & 0x4010)) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    CGItem_C* item;

    if (lua_isnumber(L, 1)) {
        item = player->m_bag.FindItemOfType(static_cast<int32_t>(lua_tonumber(L, 1)), 0);
    } else if (lua_isstring(L, 1)) {
        item = player->m_bag.FindItemByName(lua_tolstring(L, 1, nullptr), 0);
    } else {
        luaL_error(L, "Usage: SpellTargetItem(itemID|\"name\"|\"itemlink\")");
        return 0;
    }

    if (item) {
        CGGameUI::CastItemSpell(item->m_obj->m_guid, 0);
    }

    return 0;
}

// VERIFY: 7282e805 manual
// OFFSET: 0x0080DC00
static int32_t Script_SpellTargetUnit(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "Usage: SpellTargetUnit(\"unit\")");
    }

    if (!CGGameUI::CanPerformAction(2) || !s_targetingSpell) {
        return 0;
    }

    CGUnit_C* unit = GetUnitFromName(lua_tolstring(L, 1, nullptr));

    if (unit && (unit->m_obj->m_type & TYPE_UNIT)) {
        WOWGUID guid = unit->m_obj->m_guid;
        Spell_C_TargetUnitByGuid(guid);
        return 0;
    }

    SpellRec spell;
    PendingSpellCast* cast = s_targetingSpell;
    g_spellDB.GetRecord(cast->m_data.m_spellId, &spell);
    Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_OUT_OF_RANGE, -1, -1, false);
    return 0;
}

// VERIFY: dc844493 auto
// OFFSET: 0x00804190
static int32_t Script_SpellCanTargetUnit(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "Usage: SpellCanTargetUnit(\"unit\")");
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (guid && Spell_C::HandleSpriteRay(guid, true)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a88df190 auto
// OFFSET: 0x00809E30
static int32_t Script_SpellStopTargeting(lua_State* L) {
    if (CGGameUI::CanPerformAction(2)) {
        bool targeting = s_targetingSpell != nullptr;

        if (s_targetingSpell) {
            Spell_C_CancelSpell(s_targetingSpell, true, true, 27);
        }

        if (targeting) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 0e0daa18 manual
// OFFSET: 0x00809EA0
static int32_t Script_SpellStopCasting(lua_State* L) {
    if (!CGGameUI::CanPerformAction(2)) {
        lua_pushnil(L);
        return 1;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->m_unit->m_unitFieldCharmedby) {
        lua_pushnil(L);
        return 1;
    }

    if (OnCancelCombat()) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (Spell_C_IsActivePlayerCasting()) {
        Spell_C_StopCasting();
        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (!Spell_C_GetAutoRepeatingSpell()) {
        lua_pushnil(L);
        return 1;
    }

    Spell_C_CancelAutoRepeat(true);
    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: 3c719e98 auto
// OFFSET: 0x008021D0
static int32_t Script_CancelItemTempEnchantment(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    uint32_t slot;

    if (!lua_isnumber(L, 1) || (slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1, slot > 1)) {
        luaL_error(L, "Usage: CancelItemTempEnchantment(slot)");
        return 0;
    }

    int32_t invSlot = slot + 15;
    CGItem_C* item = player->m_bag.GetItemPointer(invSlot);

    if (!item || !item->GetEnchantmentId(1)) {
        return 0;
    }

    CGPlayer_C::RemoveTempEnchantment(invSlot);
    return 0;
}

// VERIFY: e1fc05d2 auto
// OFFSET: 0x00804220
static int32_t Script_CancelUnitBuff(lua_State* L) {
    ScriptAuraInfo info;

    if (!Script_GetUnitAuraInfo(L, 1, &info)) {
        luaL_error(L, "Usage: CancelUnitBuff(\"unit\", [index] or [\"name\", \"rank\"][, \"filter\"])");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_obj->m_guid == info.m_guid) {
        int32_t slot = static_cast<int32_t>(player->GetSortedAuraSlot(info.m_index, info.m_name, info.m_rank, info.m_filter));

        if (slot < 0) {
            return 0;
        }

        SpellRec spell;

        if (g_spellDB.GetRecord(player->GetAuraSpellId(slot), &spell) && ((player->GetAuraFlags(slot) & 0x10) || (spell.m_attributesEx & 0x4))) {
            Spell_C_CancelAura(&spell, player, slot);
        }

        return 0;
    }

    CGInputControl* input = CGInputControl::GetActive();
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!input || !mover || !input->IsVehicleSeatFlagSet(0x2000000)) {
        return 0;
    }

    int32_t slot = static_cast<int32_t>(mover->GetSortedAuraSlot(info.m_index, info.m_name, info.m_rank, info.m_filter));

    if (slot < 0 || mover->GetAuraCaster(slot) != ClntObjMgrGetActivePlayer().m_guid) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(mover->GetAuraSpellId(slot), &spell)) {
        for (uint32_t i = 0; i < 3; i++) {
            if (spell.m_effectAura[i] == SPELL_AURA_CONTROL_VEHICLE) {
                mover->VehicleExit();
                break;
            }
        }
    }

    return 0;
}

// STATIC: 0x00AF51C8
static FrameScript_Method s_ScriptFunctions[] = {
    { "SpellIsTargeting", &Script_SpellIsTargeting },
    { "SpellCanTargetItem", &Script_SpellCanTargetItem },
    { "SpellTargetItem", &Script_SpellTargetItem },
    { "SpellCanTargetUnit", &Script_SpellCanTargetUnit },
    { "SpellTargetUnit", &Script_SpellTargetUnit },
    { "SpellCanTargetGlyph", &Script_SpellCanTargetGlyph },
    { "SpellStopTargeting", &Script_SpellStopTargeting },
    { "SpellStopCasting", &Script_SpellStopCasting },
    { "CancelUnitBuff", &Script_CancelUnitBuff },
    { "CancelItemTempEnchantment", &Script_CancelItemTempEnchantment },
    { "CannotBeResurrected", &Script_CannotBeResurrected },
};

// VERIFY: c99a9227 auto
// OFFSET: 0x007FDDB0
void SpellRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 302aa8ec auto
// OFFSET: 0x007FDDE0
void SpellUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
