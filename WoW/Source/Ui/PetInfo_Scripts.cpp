#include "PetInfo.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/Rec/SummonPropertiesRec.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/DB/Rec/ItemPetFoodRec.h"
#include "WoW/Source/DB/Rec/TalentTabRec.h"
#include "WoW/Source/DB/Rec/PetPersonalityRec.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Client.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

// VERIFY: d3a2ff98 manual
// OFFSET: 0x005D4EC0
static int32_t Script_GetPetActionInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPetActionInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    PetAction* action = CGPetInfo::GetPet(0) && index < sizeof(CGPetInfo::m_actions) / sizeof(CGPetInfo::m_actions[0]) ? &CGPetInfo::m_actions[index] : nullptr;

    if (!action || !action->m_data) {
        for (uint32_t i = 0; i < 7; i++) {
            lua_pushnil(L);
        }

        return 7;
    }

    uint32_t id = action->m_data & 0xFFFFFF;
    bool autoCastEnabled = false;
    char buffer[64];

    switch ((action->m_data >> 24) & 0x3F) {
        case 1:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17: {
            SpellRec spell;
            int32_t found = g_spellDB.GetRecord(id, &spell);

            if (found) {
                lua_pushstring(L, spell.m_name);
                lua_pushstring(L, spell.m_nameSubtext);
                CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
                SpellIconRec* icon = g_spellIconDB.GetRecord(Spell_C_IsActiveSpell(&spell, pet) && spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

                if (icon) {
                    lua_pushstring(L, icon->m_textureFilename);
                } else {
                    lua_pushnil(L);
                }
            } else {
                lua_pushnil(L);
                lua_pushnil(L);
                lua_pushnil(L);
            }

            lua_pushnil(L);
            lua_pushnil(L);
            bool autoCastAllowed = found && (action->m_data & 0x80000000);
            autoCastEnabled = found && (action->m_data & 0x40000000);

            if (autoCastAllowed) {
                lua_pushnumber(L, 1.0);
            } else {
                lua_pushnil(L);
            }

            break;
        }
        case 6: {
            SStrPrintf(buffer, sizeof(buffer), "PET_MODE_%s", CGPetInfo::GetModeToken(id));
            lua_pushstring(L, buffer);
            lua_pushnil(L);
            SStrPrintf(buffer, sizeof(buffer), "PET_%s_TEXTURE", CGPetInfo::GetModeToken(id));
            lua_pushstring(L, buffer);
            lua_pushnumber(L, 1.0);

            if (CGPetInfo::GetPetMode() == id) {
                lua_pushnumber(L, 1.0);
            } else {
                lua_pushnil(L);
            }

            lua_pushnil(L);
            break;
        }
        case 7: {
            SStrPrintf(buffer, sizeof(buffer), "PET_ACTION_%s", CGPetInfo::GetOrdersToken(id));
            lua_pushstring(L, buffer);
            lua_pushnil(L);
            SStrPrintf(buffer, sizeof(buffer), "PET_%s_TEXTURE", CGPetInfo::GetOrdersToken(id));
            lua_pushstring(L, buffer);
            lua_pushnumber(L, 1.0);

            if (id == 2 ? CGPetInfo::m_attackingTarget != 0 : (CGPetInfo::m_petMode >> 8) == id) {
                lua_pushnumber(L, 1.0);
            } else {
                lua_pushnil(L);
            }

            lua_pushnil(L);
            break;
        }
        default:
            lua_pushnil(L);
            break;
    }

    if (autoCastEnabled) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 7;
}

// VERIFY: ab3a26c1 auto
// OFFSET: 0x005D5640
static int32_t Script_PetStopAttack(lua_State* L) {
    CGPetInfo::PetClearTarget();
    return 0;
}

// VERIFY: 6841a93e auto
// OFFSET: 0x005D3160
static int32_t Script_IsPetAttackActive(lua_State* L) {
    lua_pushboolean(L, CGPetInfo::m_attackingTarget);
    return 1;
}

// VERIFY: c4e057ce manual
// OFFSET: 0x005D3180
static int32_t Script_GetPetTimeRemaining(lua_State* L) {
    uint32_t expiration = CGPetInfo::m_expirationTime;

    if (!expiration) {
        lua_pushnil(L);
        return 1;
    }

    uint32_t remaining = expiration == OsGetAsyncTimeMs() ? 0 : expiration - OsGetAsyncTimeMs();
    lua_pushnumber(L, remaining);
    return 1;
}

// VERIFY: cab5e1fd manual
// OFFSET: 0x005D3780
static int32_t Script_PetCanBeAbandoned(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (pet && pet->m_unit->m_unitFieldSummonedby == ClntObjMgrGetActivePlayer() && (pet->m_unit->m_unitFieldBytes2[2] & 2)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2e7b5ffe manual
// OFFSET: 0x005D3820
static int32_t Script_PetCanBeDismissed(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    SummonPropertiesRec* properties;

    if (pet && pet->m_unit->m_unitFieldCreatedby == ClntObjMgrGetActivePlayer() && (properties = pet->GetSummonProperties()) && (properties->m_flags & 0x20)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    return 1;
}

// VERIFY: a98e1721 manual
// OFFSET: 0x005D38C0
static int32_t Script_PetCanBeRenamed(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (pet && pet->m_unit->m_unitFieldSummonedby == ClntObjMgrGetActivePlayer() && (pet->m_unit->m_unitFieldBytes2[2] & 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: d37eccba manual
// OFFSET: 0x005D3CE0
static int32_t Script_GetPetIcon(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    CreatureFamilyRec* family;

    if (pet && pet->CanGainXP() && (family = g_creatureFamilyDB.GetRecord(pet->GetCreatureFamily()))) {
        lua_pushstring(L, family->m_iconFile);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 80fd0a13 manual
// OFFSET: 0x005D3960
static int32_t Script_HasPetUI(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (pet && !(pet->m_obj->m_type & TYPE_PLAYER) && pet->m_unit->m_unitFieldPetnumber) {
        lua_pushnumber(L, 1.0);

        if (pet->CanGainXP()) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        return 2;
    }

    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

// VERIFY: 4ac52271 manual
// OFFSET: 0x005D3A20
static int32_t Script_GetPetExperience(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (pet && pet->CanGainXP()) {
        lua_pushnumber(L, pet->m_unit->m_unitFieldPetexperience);
        lua_pushnumber(L, pet->m_unit->m_unitFieldPetnextlevelexp);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 2;
}

// VERIFY: e7c0a80b manual
// OFFSET: 0x005D5A10
static int32_t Script_IsPetAttackAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsPetAttackAction(index)");
        return 0;
    }

    lua_pushboolean(L, CGPetInfo::IsAttackAction(lua_tointeger(L, 1) - 1));
    return 1;
}

// VERIFY: 50d858a4 manual
// OFFSET: 0x005D5280
static int32_t Script_GetPetActionCooldown(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPetActionCooldown(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    PetAction* action = CGPetInfo::GetPet(0) && index < sizeof(CGPetInfo::m_actions) / sizeof(CGPetInfo::m_actions[0]) ? &CGPetInfo::m_actions[index] : nullptr;
    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;

    if (action && action->m_data) {
        uint32_t type = (action->m_data >> 24) & 0x3F;

        if (type == 1 || type - 8 <= 9) {
            Spell_C_GetSpellCooldown(action->m_data & 0xFFFFFF, 1, &duration, &start, &enable);
        }
    }

    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: 716497e3 auto
// OFFSET: 0x005D3720
static int32_t Script_PetHasActionBar(lua_State* L) {
    if (s_pets.Count() && s_pets.m_data[0] && !(CGActionBar::m_tempPageActiveFlags & 1) && CGPetInfo::IsPetBarUsed()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1775c19c manual
// OFFSET: 0x005D5820
static int32_t Script_GetPossessInfo(lua_State* L) {
    if (lua_tonumber(L, 1) == 0.0) {
        luaL_error(L, "Usage: GetPossessInfo(index)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && CGPetInfo::m_possessSpell) {
        int64_t index = static_cast<int64_t>(lua_tonumber(L, 1));

        if (index == 1) {
            SpellRec spell;

            if (g_spellDB.GetRecord(CGPetInfo::m_possessSpell, &spell)) {
                SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

                if (icon) {
                    lua_pushstring(L, icon->m_textureFilename);
                } else {
                    lua_pushnil(L);
                }

                lua_pushstring(L, spell.m_name);
                lua_pushboolean(L, 1);
                return 3;
            }
        } else if (index == 2) {
            SpellIconRec* icon = g_spellIconDB.GetRecord(693);

            if (icon) {
                lua_pushstring(L, icon->m_textureFilename);
            } else {
                lua_pushnil(L);
            }

            int32_t enabled = 1;
            SpellRec spell;

            if (g_spellDB.GetRecord(CGPetInfo::m_possessSpell, &spell) && spell.m_attributes < 0) {
                enabled = 0;
            }

            lua_pushstring(L, spell.m_name);
            lua_pushboolean(L, enabled);
            return 3;
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

// STATIC: 0x00AD0C30
// VERIFY: 2920ba20 manual
// OFFSET: 0x005D3B00
static int32_t Script_GetPetHappiness(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    PetPersonalityRec* personality;

    if (pet && (personality = pet->GetPetPersonality())) {
        uint32_t index = pet->GetHappinessIndex();
        lua_pushnumber(L, static_cast<int32_t>(index + 1));
        lua_pushnumber(L, personality->m_happinessDamage[index] * 100.0);
        return 2;
    }

    lua_pushnil(L);
    lua_pushnumber(L, 100.0);
    return 2;
}

// VERIFY: c0d0f3b9 manual
// OFFSET: 0x005D3BD0
static int32_t Script_GetPetFoodTypes(lua_State* L) {
    int32_t count = 0;
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (!pet || !pet->CanGainXP()) {
        return 0;
    }

    CreatureFamilyRec* family = g_creatureFamilyDB.GetRecord(pet->GetCreatureFamily());

    if (!family) {
        return 0;
    }

    for (int32_t i = 0; i < g_itemPetFoodDB.m_numRecords; i++) {
        ItemPetFoodRec* food = g_itemPetFoodDB.GetRecordByIndex(i);

        if (food && ((1 << (food->m_id - 1)) & family->m_petFoodMask)) {
            lua_checkstack(L, 1);
            lua_pushstring(L, food->m_name);
            count++;
        }
    }

    return count;
}

// VERIFY: 05982c17 manual
// OFFSET: 0x005D3D80
static int32_t Script_GetPetTalentTree(lua_State* L) {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    CreatureFamilyRec* family;

    if (pet && pet->CanGainXP() && (family = g_creatureFamilyDB.GetRecord(pet->GetCreatureFamily())) && family->m_petTalentType >= 0) {
        for (int32_t i = 0; i < g_talentTabDB.m_numRecords; i++) {
            TalentTabRec* tab = g_talentTabDB.GetRecordByIndex(i);

            if (tab->m_categoryEnumID && ((1 << family->m_petTalentType) & tab->m_categoryEnumID)) {
                lua_pushstring(L, tab->m_name);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 5f419e5d auto
// OFFSET: 0x005D53C0
static int32_t Script_GetPetActionsUsable(lua_State* L) {
    if (CGPetInfo::PetActionsUsable(false)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 81f687fb manual
// OFFSET: 0x005D5400
static int32_t Script_GetPetActionSlotUsable(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPetActionSlotsUsable(index)");
        return 0;
    }

    if (CGPetInfo::PetActionSlotUsable(GetPetAction(static_cast<int32_t>(lua_tonumber(L, 1)) - 1))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 4e6f97af auto
// OFFSET: 0x005D54C0
static int32_t Script_PetPassiveMode(lua_State* L) {
    WOWGUID target = WOWGUID();
    PetAction action;
    action.m_data = 0x06000000;
    CGPetInfo::SendPetAction(action, target, 0);
    return 0;
}

// VERIFY: 556ce6f4 auto
// OFFSET: 0x005D54F0
static int32_t Script_PetDefensiveMode(lua_State* L) {
    WOWGUID target = WOWGUID();
    PetAction action;
    action.m_data = 0x06000001;
    CGPetInfo::SendPetAction(action, target, 0);
    return 0;
}

// VERIFY: 115b2ebe auto
// OFFSET: 0x005D5520
static int32_t Script_PetAggressiveMode(lua_State* L) {
    WOWGUID target = WOWGUID();
    PetAction action;
    action.m_data = 0x06000002;
    CGPetInfo::SendPetAction(action, target, 0);
    return 0;
}

// VERIFY: 0797ff1f auto
// OFFSET: 0x005D5550
static int32_t Script_PetWait(lua_State* L) {
    WOWGUID target = WOWGUID();
    PetAction action;
    action.m_data = 0x07000000;
    CGPetInfo::SendPetAction(action, target, 0);
    return 0;
}

// VERIFY: 5b55975e auto
// OFFSET: 0x005D5580
static int32_t Script_PetFollow(lua_State* L) {
    WOWGUID target = WOWGUID();
    PetAction action;
    action.m_data = 0x07000001;
    CGPetInfo::SendPetAction(action, target, 0);
    return 0;
}

// VERIFY: 4e80b8d7 auto
// OFFSET: 0x005D55B0
static int32_t Script_PetAttack(lua_State* L) {
    int32_t a3 = 0;
    WOWGUID target = WOWGUID();
    const char* token = lua_tolstring(L, 1, nullptr);

    if (token && *token) {
        if (!Script_GetGUIDFromToken(token, target, false)) {
            target = CGGameUI::ClosestObjectMatch(token, 8, 1, StringToBOOL(L, 2, 0), 3.4028235e38f, 1);
        }

        a3 = 1;
    }

    PetAction action;
    action.m_data = 0x07000002;
    CGPetInfo::SendPetAction(action, target, a3);
    return 0;
}

// VERIFY: 9128a6d3 auto
// OFFSET: 0x005D5650
static int32_t Script_PetAbandon(lua_State* L) {
    CGPetInfo::PetAbandon();
    return 0;
}

// VERIFY: b649ae2a auto
// OFFSET: 0x005D5660
static int32_t Script_PetDismiss(lua_State* L) {
    CGPetInfo::PetDismiss();
    return 0;
}

// VERIFY: f2305b6f manual
// OFFSET: 0x005D5670
static int32_t Script_PetRename(lua_State* L) {
    const char* name = lua_tolstring(L, 1, nullptr);
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (!pet) {
        CGGameUI::DisplayError(ERR_NO_PET);
        return 0;
    }

    if (pet->m_unit->m_unitFieldSummonedby != ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_NOTYOURPET);
        return 0;
    }

    if (!(pet->m_unit->m_unitFieldBytes2[2] & 0x1)) {
        CGGameUI::DisplayError(ERR_PET_NOT_RENAMEABLE);
        return 0;
    }

    if (!name || !*name) {
        CGGameUI::DisplayNameError(NAME_NO_NAME);
        return 0;
    }

    VALIDATE_NAME_RESULT error = ValidatePetName(s_locale, name, true, true, true, true, 0);

    if (error) {
        CGGameUI::DisplayNameError(error);
        return 0;
    }

    if (!NameNeedsDeclensions(s_locale, name)) {
        CGPetInfo::PetRename(name, nullptr);
        return 0;
    }

    char declinedNames[5][96];

    for (int32_t i = 0; i < 5; i++) {
        const char* declined = lua_tolstring(L, i + 2, nullptr);

        if (!declined || !*declined) {
            FrameScript_SignalEvent(SCRIPT_EVENT_PET_FORCE_NAME_DECLENSION, "%s", name);
            return 0;
        }

        SStrCopy(declinedNames[i], declined, 96);
    }

    CGPetInfo::PetRename(name, declinedNames[0]);
    return 0;
}

// VERIFY: 608c8214 manual
// OFFSET: 0x005D65C0
static int32_t Script_PickupPetAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupPetAction(index)");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 10) {
        luaL_error(L, "Invalid slot in PickupPetAction");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(14)) {
        return 0;
    }

    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (!pet || (pet->m_unit->m_unitFieldFlags & 0x1000000)) {
        return 0;
    }

    PetAction cursorAction = CGGameUI::m_cursorPetAction;
    CGGameUI::ClearCursor(1, 1);

    if (cursorAction.m_data) {
        CGPetInfo::PlaceCursorAction(cursorAction, slot);
        return 0;
    }

    PetAction* action = GetPetAction(slot);

    if (action) {
        CGGameUI::SetCursorPetAction(action);

        if (((action->m_data >> 24) & 0x3F) == 1) {
            CGPetInfo::PickupAction(slot, 1);
        }
    }

    return 0;
}

// VERIFY: a3190f42 auto
// OFFSET: 0x005D66F0
static int32_t Script_TogglePetAutocast(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TogglePetAutocast(index)");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 10) {
        luaL_error(L, "Invalid slot in TogglePetAutocast");
        return 0;
    }

    PetAction cursorAction = CGGameUI::m_cursorPetAction;
    CGGameUI::ClearCursor(1, 1);

    if (cursorAction.m_data) {
        CGPetInfo::PlaceAction(cursorAction, slot);
    } else {
        CGPetInfo::SetAutocast(slot, -1);
    }

    return 0;
}

// VERIFY: 31323429 manual
// OFFSET: 0x005D67B0
static int32_t Script_CastPetAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CastPetAction(index, target)");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 10) {
        luaL_error(L, "Invalid slot in CastPetAction");
        return 0;
    }

    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (pet && !(pet->m_unit->m_unitFieldFlags & 0x1000000)) {
        PetAction cursorAction = CGGameUI::m_cursorPetAction;
        CGGameUI::ClearCursor(1, 1);

        if (cursorAction.m_data) {
            CGPetInfo::PlaceCursorAction(cursorAction, slot);
            return 0;
        }
    }

    PetAction* action = GetPetAction(slot);

    if (action) {
        WOWGUID target;

        if (Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true)) {
            CGPetInfo::SendPetAction(*action, target, 0);
            SI2::PlaySoundKit("GAMEABILITYACTIVATE", nullptr, nullptr, nullptr);
        }
    }

    return 0;
}

static FrameScript_Method s_ScriptFunctions[] = {
    { "PetHasActionBar", &Script_PetHasActionBar },
    { "GetPetActionInfo", &Script_GetPetActionInfo },
    { "GetPetActionCooldown", &Script_GetPetActionCooldown },
    { "GetPetActionsUsable", &Script_GetPetActionsUsable },
    { "GetPetActionSlotUsable", &Script_GetPetActionSlotUsable },
    { "IsPetAttackActive", &Script_IsPetAttackActive },
    { "PickupPetAction", &Script_PickupPetAction },
    { "TogglePetAutocast", &Script_TogglePetAutocast },
    { "CastPetAction", &Script_CastPetAction },
    { "PetPassiveMode", &Script_PetPassiveMode },
    { "PetDefensiveMode", &Script_PetDefensiveMode },
    { "PetAggressiveMode", &Script_PetAggressiveMode },
    { "PetWait", &Script_PetWait },
    { "PetFollow", &Script_PetFollow },
    { "PetAttack", &Script_PetAttack },
    { "PetStopAttack", &Script_PetStopAttack },
    { "PetAbandon", &Script_PetAbandon },
    { "PetDismiss", &Script_PetDismiss },
    { "PetRename", &Script_PetRename },
    { "PetCanBeAbandoned", &Script_PetCanBeAbandoned },
    { "PetCanBeDismissed", &Script_PetCanBeDismissed },
    { "PetCanBeRenamed", &Script_PetCanBeRenamed },
    { "GetPetTimeRemaining", &Script_GetPetTimeRemaining },
    { "HasPetUI", &Script_HasPetUI },
    { "GetPetExperience", &Script_GetPetExperience },
    { "GetPetHappiness", &Script_GetPetHappiness },
    { "GetPetFoodTypes", &Script_GetPetFoodTypes },
    { "GetPetIcon", &Script_GetPetIcon },
    { "GetPetTalentTree", &Script_GetPetTalentTree },
    { "GetPossessInfo", &Script_GetPossessInfo },
    { "IsPetAttackAction", &Script_IsPetAttackAction },
};

// VERIFY: c9cfe2ae auto
// OFFSET: 0x005D31F0
void PetInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: dfb5a269 auto
// OFFSET: 0x005D3220
void PetInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
