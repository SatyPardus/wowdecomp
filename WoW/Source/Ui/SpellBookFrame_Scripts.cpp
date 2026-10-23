#include "SpellBookFrame.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Base/CDataStore.h"

// STATIC: 0x00ACCCC4
const char* s_companionTypes[2] = { "CRITTER", "MOUNT" };

// VERIFY: d9335da2 manual
// OFFSET: 0x0053C0B0
static int32_t Script_GetNumShapeshiftForms(lua_State* L) {
    lua_pushnumber(L, s_shapeshiftSpells.m_count);
    return 1;
}

// VERIFY: a27f3d8b auto
// OFFSET: 0x0053DD10
static int32_t Script_UpdateSpells(lua_State* L) {
    CGSpellBook::UpdateSpells(0, 0, 0);
    return 0;
}

// VERIFY: dd465bdc manual
// OFFSET: 0x0053B5C0
static int32_t Script_GetNumSpellTabs(lua_State* L) {
    lua_pushnumber(L, s_numSkillInfos);
    return 1;
}

// VERIFY: 0a46e79d manual
// OFFSET: 0x0053B6C0
static int32_t Script_FindSpellBookSlotByID(lua_State* L) {
    int32_t spellId;

    if (!lua_isnumber(L, 1) || (spellId = static_cast<int32_t>(lua_tointeger(L, 1))) <= 0) {
        luaL_error(L, "Usage: FindSpellBookSlotByID(spellID[, isPet])");
        return 0;
    }

    int32_t slot = GetSpellSlot(spellId, StringToBOOL(L, 2, 0));

    if (slot >= 0) {
        lua_pushinteger(L, slot);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 6fb2c8d5 auto
// OFFSET: 0x0053B650
static int32_t Script_GetKnownSlotFromHighestRankSlot(lua_State* L) {
    uint32_t slot = lua_tointeger(L, 1) - 1;
    int32_t index = -1;

    if (slot < s_displayedSpellCount) {
        int32_t spell = s_displayedSpells[slot];

        for (uint32_t i = 0; i < s_numSpells; i++) {
            if (s_spellList[i] == spell) {
                index = i;
                break;
            }
        }
    }

    lua_pushnumber(L, index + 1);
    return 1;
}

// VERIFY: 809ea20d manual
// OFFSET: 0x0053BE70
static int32_t Script_GetSpellTabInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetSpellTabInfo(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    SpellBookSkillInfo* info;

    if (static_cast<uint32_t>(index) >= s_numSkillInfos || !(info = s_skillInfos.m_data[index])) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 6;
    }

    if (!info->m_skillLine) {
        lua_pushstring(L, FrameScript_GetText("GENERAL_SPELLS", -1, GENDER_NOT_APPLICABLE));
        lua_pushstring(L, "Interface\\Icons\\Ability_Kick");
    } else {
        SkillLineRec* skill = g_skillLineDB.GetRecord(info->m_skillLine);

        if (skill) {
            lua_pushstring(L, skill->m_displayName);
            SpellIconRec* icon = g_spellIconDB.GetRecord(skill->m_spellIconID);

            if (icon) {
                lua_pushstring(L, icon->m_textureFilename);
            } else {
                lua_pushnil(L);
            }
        } else {
            lua_pushnil(L);
            lua_pushnil(L);
        }
    }

    uint32_t offset = 0;

    if (index && static_cast<uint32_t>(index) < s_numSkillInfos) {
        for (int32_t i = index - 1; i >= 0; i--) {
            offset += s_skillInfos.m_data[i]->m_count;
        }
    }

    lua_pushnumber(L, offset);
    lua_pushnumber(L, info->m_count);
    uint32_t visibleOffset = 0;

    if (index && static_cast<uint32_t>(index) < s_numSkillInfos) {
        for (int32_t i = index - 1; i >= 0; i--) {
            visibleOffset += s_skillInfos.m_data[i]->m_unk08;
        }
    }

    lua_pushnumber(L, visibleOffset);
    lua_pushnumber(L, info->m_unk08);
    return 6;
}

// VERIFY: 18466665 auto
// OFFSET: 0x0053B600
static bool GetLuaCompanionType(lua_State* L, int32_t* type) {
    const char* token = lua_tolstring(L, 1, nullptr);

    if (!token) {
        return false;
    }

    for (int32_t i = 0; i < 2; i++) {
        if (!SStrCmpI(token, s_companionTypes[i], STORM_MAX_STR)) {
            *type = i;
            return true;
        }
    }

    return false;
}

// VERIFY: 5b1a870f manual
// OFFSET: 0x0053DD30
static int32_t Script_HasPetSpells(lua_State* L) {
    if (!CGSpellBook::m_numPetSpells) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushnumber(L, CGSpellBook::m_numPetSpells);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        lua_pushstring(L, "PET");
        return 2;
    }

    ChrClassesRec* rec = g_chrClassesDB.GetRecord(player->m_unit->m_unitFieldBytes0[1]);
    lua_pushstring(L, rec->m_petNameToken);
    return 2;
}

// VERIFY: f846068d manual
// OFFSET: 0x0053E2C0
static int32_t Script_GetCompanionInfo(lua_State* L) {
    int32_t type;

    if (!GetLuaCompanionType(L, &type) || lua_tonumber(L, 2) == 0.0) {
        luaL_error(L, "Usage: GetCompanionInfo(type, index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    TSGrowableArray<int32_t>& companions = CGSpellBook::m_companions[type];

    if (index >= companions.m_count) {
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(companions.m_data[index], &spell)) {
        return 0;
    }

    int32_t creatureId = spell.m_effectMiscValue[0];
    lua_pushinteger(L, creatureId);
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(creatureId, WOWGUID(), nullptr, nullptr, false);

    if (stats) {
        lua_pushstring(L, stats->GetName(0));
    } else {
        lua_pushnil(L);
    }

    lua_pushinteger(L, spell.m_id);
    bool active = Spell_C_IsActiveSpell(&spell, ClntObjMgrGetActivePlayerObj());
    SpellIconRec* icon = g_spellIconDB.GetRecord(active && spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

    if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
    } else {
        lua_pushnil(L);
    }

    if (active) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 5;
}

// VERIFY: c4c6702d manual
// OFFSET: 0x005407F0
static int32_t Script_GetSpellName(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0), &spell)) {
        lua_pushstring(L, spell.m_name);
        lua_pushstring(L, spell.m_nameSubtext);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 867b8465 manual
// OFFSET: 0x00541340
static int32_t Script_IsPassiveSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0), &spell) && (spell.m_attributes & 0x40)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9622fae9 manual
// OFFSET: 0x00541420
static int32_t Script_IsAttackSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0), &spell) && spell.m_effect[0] == SPELL_EFFECT_ATTACK) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1dfe31dc manual
// OFFSET: 0x005415D0
static int32_t Script_IsAutoRepeatSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    int32_t spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);

    if (spellId && !petSpell && (Spell_C_GetAutoRepeatingSpell() == spellId || Spell_C_GetAutoRangedCombatSpell() == spellId)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 450169e3 manual
// OFFSET: 0x005419F0
static int32_t Script_IsConsumableSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0), &spell)) {
        for (uint32_t i = 0; i < 8; i++) {
            if (spell.m_reagent[i] && spell.m_reagentCount[i] > 0) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 33061c66 auto
// OFFSET: 0x0053C2A0
static int32_t Script_GetNumCompanions(lua_State* L) {
    int32_t type;

    if (!GetLuaCompanionType(L, &type)) {
        luaL_error(L, "Usage: GetNumCompanions(type)");
        return 0;
    }

    lua_pushnumber(L, CGSpellBook::m_companions[type].Count());
    return 1;
}

// VERIFY: ef9ff06e manual
// OFFSET: 0x0053C3A0
static int32_t Script_IsSpellKnown(lua_State* L) {
    int32_t spellId;

    if (!lua_isnumber(L, 1) || (spellId = lua_tointeger(L, 1)) <= 0) {
        luaL_error(L, "Usage: IsSpellKnown(spellID[, isPet])");
        return 0;
    }

    int32_t isPet = StringToBOOL(L, 2, 0);
    bool known;

    if (!(GetSpellSlot(spellId, isPet) & 0x80000000)) {
        known = true;
    } else if (isPet) {
        known = CGPetInfo::FindSpellByID(spellId) != nullptr;
    } else {
        known = false;
    }

    lua_pushboolean(L, known);
    return 1;
}

// VERIFY: 9bf51350 auto
// OFFSET: 0x00540D70
static int32_t Script_GetSpellTexture(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    const char* texture = CGSpellBook::GetSpellTexture(slot, petSpell);

    if (texture && *texture) {
        lua_pushstring(L, texture);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 80a729be auto
// OFFSET: 0x005412C0
static int32_t Script_IsSelectedSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    if (CGSpellBook::IsSelectedSlot(slot, petSpell)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 0553b4ec manual
// OFFSET: 0x00541800
static int32_t Script_IsHelpfulSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0), &spell) && SpellRec::IsHelpfulOrHarmful(&spell) == 1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: ae8a4c53 manual
// OFFSET: 0x005418F0
static int32_t Script_IsHarmfulSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0), &spell) && SpellRec::IsHelpfulOrHarmful(&spell) == 2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 8c2be304 manual
// OFFSET: 0x00540DF0
static int32_t Script_GetSpellCount(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    lua_pushnumber(L, Spell_C_GetSpellReagentUses(static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0)));
    return 1;
}

// VERIFY: b125a9fb manual
// OFFSET: 0x00540E80
static int32_t Script_GetSpellCooldown(lua_State* L) {
    int32_t petSpell = 0;
    int32_t spellId;

    if (lua_isnumber(L, 1) && lua_gettop(L) == 1) {
        spellId = static_cast<int32_t>(lua_tonumber(L, 1));

        if (GetSpellSlot(spellId, 0) >= 0) {
            petSpell = 0;
        } else if (GetSpellSlot(spellId, 1) >= 0 || CGPetInfo::FindSpellByID(spellId)) {
            petSpell = 1;
        }
    } else {
        uint32_t slot = 0;

        if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
            return 0;
        }

        spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    }

    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;
    Spell_C_GetSpellCooldown(spellId, petSpell, &duration, &start, &enable);
    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: 8f904716 manual
// OFFSET: 0x00541010
static int32_t Script_GetSpellAutocast(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    PetAction* action = nullptr;
    int32_t spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);

    if (petSpell && (action = CGPetInfo::GetPetSpellAction(spellId)) && (action->m_data & 0x80000000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (action && (action->m_data & 0x40000000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: f7b499fd auto
// OFFSET: 0x005410E0
static int32_t Script_ToggleSpellAutocast(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (GetSlotFromLua(L, &slot, &petSpell, nullptr, 1) && petSpell && slot < 1024 && s_petSpellList[slot]) {
        CGPetInfo::SetSpellAutocast(s_petSpellList[slot], -1);
    }

    return 0;
}

// VERIFY: 7ac0a6a3 auto
// OFFSET: 0x00541140
static int32_t Script_EnableSpellAutocast(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (GetSlotFromLua(L, &slot, &petSpell, nullptr, 1) && petSpell && slot < 1024 && s_petSpellList[slot]) {
        CGPetInfo::SetSpellAutocast(s_petSpellList[slot], 1);
    }

    return 0;
}

// VERIFY: e8d81005 auto
// OFFSET: 0x005411A0
static int32_t Script_DisableSpellAutocast(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (GetSlotFromLua(L, &slot, &petSpell, nullptr, 1) && petSpell && slot < 1024 && s_petSpellList[slot]) {
        CGPetInfo::SetSpellAutocast(s_petSpellList[slot], 0);
    }

    return 0;
}

// VERIFY: 155d2aba auto
// OFFSET: 0x00541200
static int32_t Script_PickupSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        CGSpellBook::PickupSpell(slot, petSpell);
    }

    return 0;
}

// VERIFY: 73de6661 auto
// OFFSET: 0x00541250
static int32_t Script_CastSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (GetSlotFromLua(L, &slot, &petSpell, nullptr, 0)) {
        WOWGUID target = CGGameUI::m_lockedTarget;

        if (StringToBOOL(L, 3, 0)) {
            target = ClntObjMgrGetActivePlayer();
        }

        CGSpellBook::CastSpell(slot, petSpell, target);
    }

    return 0;
}

// VERIFY: 37a181ff manual
// OFFSET: 0x00541500
static int32_t Script_IsCurrentSpell(lua_State* L) {
    int32_t spellId;

    if (lua_isnumber(L, 1) && lua_gettop(L) == 1) {
        spellId = lua_tointeger(L, 1);
    } else {
        uint32_t slot = 0;
        int32_t petSpell;

        if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
            return 0;
        }

        spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    }

    if (Spell_C_IsCurrentSpell(spellId)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 6c00b0fc manual
// OFFSET: 0x00541680
static int32_t Script_IsUsableSpell(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    bool noMana = false;
    SpellRec spell;
    int32_t spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    CGUnit_C* caster;

    if (g_spellDB.GetRecord(spellId, &spell)
        && (caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : static_cast<CGUnit_C*>(ClntObjMgrGetActivePlayerObj()))
        && Spell_C_IsUsableAction(caster, &spell, WOWGUID{ 0 }, nullptr, false)
        && !(noMana = !Spell_C_HaveSpellPower(caster, &spell))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (noMana) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: c550d244 manual
// OFFSET: 0x00541AF0
static int32_t Script_SpellHasRange(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;

    if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
        return 0;
    }

    SpellRec spell;
    int32_t spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    CGUnit_C* caster;
    float minRange;
    float maxRange;

    if (g_spellDB.GetRecord(spellId, &spell)
        && (caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : static_cast<CGUnit_C*>(ClntObjMgrGetActivePlayerObj()))) {
        Spell_C_GetMinMaxRange(caster, &spell, &minRange, &maxRange, nullptr);

        if (fnotequal(minRange, 0.0f) || fnotequal(maxRange, 0.0f)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3ecc20db manual
// OFFSET: 0x00541C60
static int32_t Script_IsSpellInRange(lua_State* L) {
    uint32_t slot = 0;
    int32_t petSpell;
    int32_t unitIndex;

    if (!GetSlotFromLua(L, &slot, &petSpell, &unitIndex, 1)) {
        return 0;
    }

    SpellRec spell;
    int32_t spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    CGUnit_C* caster;
    WOWGUID target;
    uint8_t inRange;

    if (g_spellDB.GetRecord(spellId, &spell)
        && (caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : static_cast<CGUnit_C*>(ClntObjMgrGetActivePlayerObj()))
        && Script_GetGUIDFromToken(lua_tolstring(L, unitIndex, nullptr), target, true)
        && Spell_C_RangeCheckSelected(caster, &spell, target, &inRange, false, nullptr)) {
        lua_pushnumber(L, inRange ? 1.0 : 0.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 9e0e8af9 manual
// OFFSET: 0x0053C0F0
static int32_t Script_CastShapeshiftForm(lua_State* L) {
    if (lua_tonumber(L, 1) == 0.0) {
        luaL_error(L, "Usage: CastShapeshiftForm(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < s_shapeshiftSpells.m_count) {
        Spell_C_ClickSpell(s_shapeshiftSpells.m_data[index].m_spellId, nullptr, WOWGUID{ 0 });
    }

    return 0;
}

// VERIFY: e1b78d65 manual
// OFFSET: 0x0053C180
static int32_t Script_GetShapeshiftFormCooldown(lua_State* L) {
    if (lua_tonumber(L, 1) == 0.0) {
        luaL_error(L, "Usage: GetShapeshiftFormCooldown(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= s_shapeshiftSpells.m_count) {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 1.0);
        return 3;
    }

    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;
    Spell_C_GetSpellCooldown(s_shapeshiftSpells.m_data[index].m_spellId, 0, &duration, &start, &enable);
    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: 2cf49f7e auto
// OFFSET: 0x0053C310
static int32_t Script_PickupCompanion(lua_State* L) {
    int32_t type;

    if (!GetLuaCompanionType(L, &type) || lua_tonumber(L, 2) == 0.0) {
        luaL_error(L, "Usage: PickupCompanion(type, index)");
        return 0;
    }

    CGSpellBook::PickupCompanion(type, static_cast<uint32_t>(lua_tonumber(L, 2)) - 1);
    return 0;
}

// VERIFY: a881f31c manual
// OFFSET: 0x0053DE10
static int32_t Script_GetShapeshiftForm(lua_State* L) {
    lua_pushnumber(L, CGSpellBook::GetShapeshiftForm(StringToBOOL(L, 1, 0)));
    return 1;
}

// VERIFY: 0cf3162e manual
// OFFSET: 0x0053DE60
static int32_t Script_CancelShapeshiftForm(lua_State* L) {
    CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->CancelShapeshift();
    }

    return 0;
}

// VERIFY: f72ddf21 manual
// OFFSET: 0x0053DE90
static int32_t Script_GetShapeshiftFormInfo(lua_State* L) {
    if (lua_tonumber(L, 1) == 0.0) {
        luaL_error(L, "Usage: GetShapeshiftFormInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

    if (index >= s_shapeshiftSpells.m_count || !player) {
        return 0;
    }

    SpellRec spell;
    SpellUsableInfo* info = &s_shapeshiftSpells.m_data[index];

    if (!g_spellDB.GetRecord(info->m_spellId, &spell)) {
        return 0;
    }

    bool active = Spell_C_IsActiveSpell(&spell, player);
    bool castable;
    int32_t iconId;

    if (!active && (!info->m_usable || info->m_noMana)) {
        castable = false;
        iconId = spell.m_spellIconID;
    } else {
        castable = true;
        iconId = active && spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID;
    }

    SpellIconRec* icon = g_spellIconDB.GetRecord(iconId);

    if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, spell.m_name);

    if (active) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (castable) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: dbc14ce1 manual
// OFFSET: 0x0053E490
static int32_t Script_GetCompanionCooldown(lua_State* L) {
    int32_t type;

    if (!GetLuaCompanionType(L, &type) || lua_tonumber(L, 2) == 0.0) {
        luaL_error(L, "Usage: GetCompanionCooldown(type, index)");
        return 0;
    }

    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (index >= CGSpellBook::m_companions[type].m_count) {
        return 0;
    }

    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;
    Spell_C_GetSpellCooldown(CGSpellBook::m_companions[type].m_data[index], 0, &duration, &start, &enable);
    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: 7c5d589b manual
// OFFSET: 0x0053E5E0
static int32_t Script_CallCompanion(lua_State* L) {
    int32_t type;

    if (!GetLuaCompanionType(L, &type) || lua_tonumber(L, 2) == 0.0) {
        luaL_error(L, "Usage: CallCompanion(type, index)");
        return 0;
    }

    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (index >= CGSpellBook::m_companions[type].m_count) {
        return 0;
    }

    Spell_C::CastSpell(CGSpellBook::m_companions[type].m_data[index], nullptr, WOWGUID{ 0 }, false);
    return 0;
}

// VERIFY: 75ea406b manual
// OFFSET: 0x0053E6B0
static int32_t Script_DismissCompanion(lua_State* L) {
    int32_t type;

    if (!GetLuaCompanionType(L, &type)) {
        luaL_error(L, "Usage: DismissCompanion(type)");
        return 0;
    }

    CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

    for (uint32_t i = 0; i < CGSpellBook::m_companions[type].m_count; i++) {
        SpellRec spell;

        if (g_spellDB.GetRecord(CGSpellBook::m_companions[type].m_data[i], &spell) && Spell_C_IsActiveSpell(&spell, player)) {
            Spell_C_CancelAura(&spell, player, -1);
        }
    }

    return 0;
}

// VERIFY: d79d2bf4 manual
// OFFSET: 0x0053E7E0
static int32_t Script_SummonRandomCritter(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->CastRandomCritter();
    }

    return 1;
}

// VERIFY: e3bbfec9 auto
// OFFSET: 0x0053E060
static int32_t Script_CastSpellByID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CastSpellByID(spellID[, target])");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    WOWGUID target;

    if (!Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true)) {
        return 0;
    }

    int32_t spellId = lua_tointeger(L, 1);

    if (!spellId) {
        return 0;
    }

    if (GetSpellSlot(spellId, 0) >= 0) {
        Spell_C::CastSpell(spellId, nullptr, target, false);
        return 0;
    }

    if (!CGPetInfo::PetActionsUsable(false)) {
        return 0;
    }

    PetAction petAction;
    petAction.m_data = 0;
    PetAction* action = CGPetInfo::FindSpellByID(spellId);

    if (!action) {
        if (GetSpellSlot(spellId, 1) < 0) {
            return 0;
        }

        petAction.m_data = spellId | 0x1000000;
        action = &petAction;
    }

    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (!pet) {
        return 0;
    }

    SpellRec spell;
    g_spellDB.GetRecord(spellId, &spell);

    if (!(spell.m_attributesExD & 0x20) && player->GetPossessedUnit() != pet) {
        Spell_C_StartSpellCooldown(&spell, pet);

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_PET_ACTION));
        msg.Put(CGPetInfo::GetPet(0).m_guid);
        msg.Put(action->m_data);
        msg.Put(target.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
        return 0;
    }

    Spell_C_CastSpell(pet, spell.m_id, nullptr, target, nullptr, false);
    return 0;
}

// VERIFY: c96444c2 manual
// OFFSET: 0x00540310
static int32_t Script_CastSpellByName(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: CastSpellByName(name[, target])");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    WOWGUID target;

    if (!Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true)) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    int32_t petSpell;
    int32_t spellId = GetSpellIdByName(name, &petSpell);

    if (!spellId) {
        return 0;
    }

    if (*name != '!') {
        int32_t slot = GetSpellSlot(spellId, petSpell);

        if (slot >= 0) {
            CGSpellBook::CastSpell(slot, petSpell, target);
            return 0;
        }

        PetAction* action = CGPetInfo::FindSpellByID(spellId);

        if (action) {
            CGPetInfo::SendPetAction(*action, target, 0);
            return 0;
        }

        if (!petSpell) {
            Spell_C_ClickSpell(spellId, nullptr, target);
        }

        return 0;
    }

    if (!petSpell) {
        Spell_C::CastSpell(spellId, nullptr, target, false);
        return 0;
    }

    if (!CGPetInfo::PetActionsUsable(false)) {
        return 0;
    }

    PetAction petAction;
    petAction.m_data = 0;
    PetAction* action = CGPetInfo::FindSpellByID(spellId);

    if (!action) {
        if (GetSpellSlot(spellId, petSpell) < 0) {
            return 0;
        }

        petAction.m_data = spellId | 0x1000000;
        action = &petAction;
    }

    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (!pet) {
        return 0;
    }

    SpellRec spell;
    g_spellDB.GetRecord(spellId, &spell);

    if ((spell.m_attributesExD & 0x20) || player->GetPossessedUnit() == pet) {
        Spell_C_CastSpell(pet, spell.m_id, nullptr, target, nullptr, false);
        return 0;
    }

    Spell_C_StartSpellCooldown(&spell, pet);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_ACTION));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.Put(action->m_data);
    msg.Put(target.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: cfe51b8e manual
// OFFSET: 0x005408E0
static int32_t Script_GetSpellLink(lua_State* L) {
    int32_t spellId;

    if (lua_isnumber(L, 1) && lua_gettop(L) == 1) {
        spellId = static_cast<int32_t>(lua_tonumber(L, 1));
    } else {
        uint32_t slot = 0;
        int32_t petSpell;

        if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
            return 0;
        }

        spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    }

    SpellRec spell;
    char link[256];

    if (!g_spellDB.GetRecord(spellId, &spell) || ((spell.m_attributes & 0x80) && (spell.m_attributes & 0x100))) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, CGTooltip::GetSpellLink(spellId));

    if (!CGTradeSkillInfo::GetTradeSkillListLink(spellId, link, sizeof(link))) {
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, link);
    return 2;
}

// VERIFY: 2db217bf manual
// OFFSET: 0x00540A30
static int32_t Script_GetSpellInfo(lua_State* L) {
    int32_t petSpell = 0;
    int32_t spellId;

    if (lua_isnumber(L, 1) && lua_gettop(L) == 1) {
        spellId = static_cast<int32_t>(lua_tonumber(L, 1));
    } else {
        uint32_t slot = 0;

        if (!GetSlotFromLua(L, &slot, &petSpell, nullptr, 1)) {
            return 0;
        }

        spellId = static_cast<int32_t>(slot) < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (slot < s_numSpells ? s_spellList[slot] : 0);
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGUnit_C* caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : player;

    lua_pushstring(L, spell.m_name);
    lua_pushstring(L, spell.m_nameSubtext);

    const char* texture = "";

    if (spell.m_effect[0] == SPELL_EFFECT_ATTACK) {
        texture = CGActionBar::GetAttackTexture();
    } else if ((spell.m_attributes & 0x2) && (spell.m_attributesExB & 0x20)) {
        texture = CGActionBar::GetRangedTexture();
    } else {
        SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_spellIconID);

        if (icon) {
            texture = icon->m_textureFilename;
        }
    }

    lua_pushstring(L, texture);

    uint32_t powerCost = Spell_C_GetPowerCost(&spell, caster);
    uint32_t powerCostPerSecond = Spell_C_GetPowerCostPerSecond(&spell, caster);
    uint32_t cost = powerCost / Spell_C_GetPowerDisplayMod(static_cast<POWER_TYPE>(spell.m_powerType));
    int32_t costPerSecond = powerCostPerSecond / Spell_C_GetPowerDisplayMod(static_cast<POWER_TYPE>(spell.m_powerType));

    if (costPerSecond <= 0) {
        lua_pushnumber(L, cost);
        lua_pushboolean(L, 0);
    } else {
        lua_pushnumber(L, costPerSecond);
        lua_pushboolean(L, 1);
    }

    lua_pushnumber(L, spell.m_powerType);

    if (spell.m_attributes & 0x40) {
        lua_pushnumber(L, 0.0);
    } else {
        lua_pushnumber(L, static_cast<uint32_t>(SpellRec::GetCastTime(&spell, petSpell, 0, 1)));
    }

    if (!player || !caster) {
        return 7;
    }

    if (SpellRec::IsMeleeRange(&spell)) {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 9;
    }

    float minRange = 0.0f;
    float maxRange = 0.0f;
    Spell_C_GetMinMaxRange(caster, &spell, &minRange, &maxRange, nullptr);

    if (SpellRec::IsMinimumRange(&spell)) {
        SpellRec::GetMinRange(&spell, &minRange);
    }

    lua_pushnumber(L, minRange);
    lua_pushnumber(L, maxRange);
    return 9;
}

// STATIC: 0x00ACCCE0
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumSpellTabs", &Script_GetNumSpellTabs },
    { "GetSpellTabInfo", &Script_GetSpellTabInfo },
    { "GetSpellName", &Script_GetSpellName },
    { "GetSpellLink", &Script_GetSpellLink },
    { "GetSpellInfo", &Script_GetSpellInfo },
    { "GetSpellTexture", &Script_GetSpellTexture },
    { "GetSpellCount", &Script_GetSpellCount },
    { "GetSpellCooldown", &Script_GetSpellCooldown },
    { "GetSpellAutocast", &Script_GetSpellAutocast },
    { "ToggleSpellAutocast", &Script_ToggleSpellAutocast },
    { "EnableSpellAutocast", &Script_EnableSpellAutocast },
    { "DisableSpellAutocast", &Script_DisableSpellAutocast },
    { "PickupSpell", &Script_PickupSpell },
    { "CastSpell", &Script_CastSpell },
    { "IsSelectedSpell", &Script_IsSelectedSpell },
    { "IsPassiveSpell", &Script_IsPassiveSpell },
    { "IsAttackSpell", &Script_IsAttackSpell },
    { "IsCurrentSpell", &Script_IsCurrentSpell },
    { "IsAutoRepeatSpell", &Script_IsAutoRepeatSpell },
    { "IsUsableSpell", &Script_IsUsableSpell },
    { "IsHelpfulSpell", &Script_IsHelpfulSpell },
    { "IsHarmfulSpell", &Script_IsHarmfulSpell },
    { "IsConsumableSpell", &Script_IsConsumableSpell },
    { "SpellHasRange", &Script_SpellHasRange },
    { "IsSpellInRange", &Script_IsSpellInRange },
    { "UpdateSpells", &Script_UpdateSpells },
    { "HasPetSpells", &Script_HasPetSpells },
    { "GetNumShapeshiftForms", &Script_GetNumShapeshiftForms },
    { "GetShapeshiftForm", &Script_GetShapeshiftForm },
    { "CancelShapeshiftForm", &Script_CancelShapeshiftForm },
    { "GetShapeshiftFormInfo", &Script_GetShapeshiftFormInfo },
    { "CastShapeshiftForm", &Script_CastShapeshiftForm },
    { "GetShapeshiftFormCooldown", &Script_GetShapeshiftFormCooldown },
    { "CastSpellByName", &Script_CastSpellByName },
    { "CastSpellByID", &Script_CastSpellByID },
    { "GetNumCompanions", &Script_GetNumCompanions },
    { "GetCompanionInfo", &Script_GetCompanionInfo },
    { "GetCompanionCooldown", &Script_GetCompanionCooldown },
    { "PickupCompanion", &Script_PickupCompanion },
    { "CallCompanion", &Script_CallCompanion },
    { "DismissCompanion", &Script_DismissCompanion },
    { "GetKnownSlotFromHighestRankSlot", &Script_GetKnownSlotFromHighestRankSlot },
    { "IsSpellKnown", &Script_IsSpellKnown },
    { "FindSpellBookSlotByID", &Script_FindSpellBookSlotByID },
    { "SummonRandomCritter", &Script_SummonRandomCritter },
};

// VERIFY: e97b7dd8 auto
// OFFSET: 0x0053B740
void SpellBookRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: c614d7c6 auto
// OFFSET: 0x0053B770
void SpellBookUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
