#include "ClassTrainerFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SkillLineAbilityRec.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Ui/QuestTextParser.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// VERIFY: cea75209 auto
// OFFSET: 0x00593D90
static int32_t Script_GetTrainerGreetingText(lua_State* L) {
    lua_pushstring(L, CGClassTrainer::m_greetingText);
    return 1;
}

// VERIFY: 0f2603fe manual
// OFFSET: 0x00593D10
static int32_t Script_GetNumTrainerServices(lua_State* L) {
    lua_pushnumber(L, CGClassTrainer::m_filteredServices);
    return 1;
}

// VERIFY: ea1fc55f auto
// OFFSET: 0x00593D50
static int32_t Script_IsTradeskillTrainer(lua_State* L) {
    if (CGClassTrainer::m_trainerType == TRAINER_TYPE_TRADESKILLS) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: c40929fc manual
// OFFSET: 0x005943A0
static int32_t Script_SelectTrainerService(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SelectTrainerService(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGClassTrainer::m_numServices || (CGClassTrainer::m_currentSelection = CGClassTrainer::m_services.m_data[index]->m_spellID) < 0) {
        CGClassTrainer::m_currentSelection = 0;
    }

    return 0;
}

// VERIFY: 2aa7a31c auto
// OFFSET: 0x00594480
static int32_t Script_GetTrainerServiceCost(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceCost(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service = index < CGClassTrainer::m_numServices ? CGClassTrainer::m_services.m_data[index] : nullptr;
    int32_t moneyCost = 0;
    int32_t talentCost;
    int32_t professionCost;

    if (service) {
        moneyCost = service->m_moneyCost;
        talentCost = service->m_talentCost;
        professionCost = service->m_professionCost;
    } else {
        talentCost = 0;
        professionCost = 0;
    }

    lua_pushnumber(L, moneyCost);
    lua_pushnumber(L, talentCost);
    lua_pushnumber(L, professionCost);
    return 3;
}

// VERIFY: bdbc7f4e auto
// OFFSET: 0x00594530
static int32_t Script_GetTrainerServiceLevelReq(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceLevelReq(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t levelReq = 0;

    if (index < CGClassTrainer::m_numServices) {
        TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

        if (service) {
            levelReq = service->m_levelReq;
        }
    }

    lua_pushnumber(L, levelReq);
    return 1;
}

// VERIFY: 023b70fb auto
// OFFSET: 0x005945B0
static int32_t Script_GetTrainerServiceNumAbilityReq(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceAbilityReq(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service = index < CGClassTrainer::m_numServices ? CGClassTrainer::m_services.m_data[index] : nullptr;
    uint32_t count = 0;

    if (service) {
        if (service->m_abilityReq[0] > 0) {
            count = 1;
        }

        if (service->m_abilityReq[1] > 0) {
            count++;
        }

        if (service->m_abilityReq[2] > 0) {
            count++;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: e8d6bc99 auto
// OFFSET: 0x00595B60
static int32_t Script_IsTrainerServiceSkillStep(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsTrainerServiceSkillStep(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < CGClassTrainer::m_numServices) {
        TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

        if (service) {
            SpellRec spell;

            if (g_spellDB.GetRecord(service->m_spellID, &spell)) {
                for (uint32_t i = 0; i < 3; i++) {
                    if (spell.m_effect[i] == SPELL_EFFECT_SKILL_STEP) {
                        lua_pushnumber(L, 1.0);
                        return 1;
                    }
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 48b1be31 auto
// OFFSET: 0x00594650
static int32_t Script_GetTrainerSkillLines(lua_State* L) {
    uint32_t count = CGClassTrainer::m_numSkillLines;

    for (uint32_t i = 0; i < count; i++) {
        int32_t skillLine = i < CGClassTrainer::m_numSkillLines ? CGClassTrainer::m_skillLines.m_data[i]->m_skillLine : 0;
        SkillLineRec* rec = g_skillLineDB.GetRecord(skillLine);
        lua_pushstring(L, rec ? rec->m_displayName : nullptr);
    }

    return count;
}

// VERIFY: fedc35c9 auto
// OFFSET: 0x00593EB0
static int32_t Script_GetTrainerSkillLineFilter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerSkillLineFilter(index)");
        return 0;
    }

    uint32_t filter = CGClassTrainer::m_skillLineFilter;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0) {
        for (uint32_t i = 0; i < CGClassTrainer::m_numSkillLines; i++) {
            if (!((1 << i) & filter)) {
                lua_pushnil(L);
                return 1;
            }
        }

        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (index >= CGClassTrainer::m_numSkillLines) {
        luaL_error(L, "Bad skill line in GetTrainerSkillLineFilter");
        return 0;
    }

    if ((1 << index) & filter) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 57f4fadd manual
// OFFSET: 0x005952F0
static int32_t Script_GetTrainerServiceSkillLine(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceSkillLine(index)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service = index < CGClassTrainer::m_numServices ? CGClassTrainer::m_services.m_data[index] : nullptr;

    if (player && service) {
        SpellRec spell;

        if (g_spellDB.GetRecord(service->m_spellID, &spell)) {
            for (uint32_t i = 0; i < 3; i++) {
                int32_t effect = spell.m_effect[i];

                if (!effect) {
                    continue;
                }

                SkillLineRec* skill;

                if (effect == SPELL_EFFECT_SKILL_STEP) {
                    skill = g_skillLineDB.GetRecord(spell.m_effectMiscValue[i]);
                } else {
                    int32_t spellId = effect != SPELL_EFFECT_LEARN_SPELL || spell.m_implicitTargetA[i] == 5 ? spell.m_id : spell.m_effectTriggerSpell[i];
                    SkillLineAbilityRec* ability = SpellTableLookupAbility(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], spellId);

                    if (!ability) {
                        continue;
                    }

                    skill = g_skillLineDB.GetRecord(ability->m_skillLine);
                }

                if (skill) {
                    lua_pushstring(L, skill->m_displayName);
                    return 1;
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 800b7538 auto
// OFFSET: 0x00593DB0
static TRAINER_SERVICE GetServiceTypeFromString(const char* type) {
    if (!type) {
        return NUM_TRAINER_SERVICE_TYPES;
    }

    if (!SStrCmpI(type, "available", STORM_MAX_STR)) {
        return TRAINER_SERVICE_AVAILABLE;
    }

    if (!SStrCmpI(type, "unavailable", STORM_MAX_STR)) {
        return TRAINER_SERVICE_UNAVAILABLE;
    }

    return SStrCmpI(type, "used", STORM_MAX_STR) ? NUM_TRAINER_SERVICE_TYPES : TRAINER_SERVICE_USED;
}

// VERIFY: e17cbd49 auto
// OFFSET: 0x00593E10
static int32_t Script_GetTrainerServiceTypeFilter(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceTypeFilter(\"type\")");
        return 0;
    }

    TRAINER_SERVICE type = GetServiceTypeFromString(lua_tolstring(L, 1, nullptr));

    if (type == NUM_TRAINER_SERVICE_TYPES) {
        luaL_error(L, "Bad service type in GetTrainerServiceTypeFilter");
        return 0;
    }

    if ((1 << type) & g_serviceTypeFilterCVar->m_intValue) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: d3fae411 auto
// OFFSET: 0x00594370
static int32_t Script_CloseTrainer(lua_State* L) {
    WOWGUID none = WOWGUID();
    CGClassTrainer::SetTrainer(none, TRAINER_TYPE_GENERAL);
    return 0;
}

// VERIFY: 400374a9 manual
// OFFSET: 0x00594430
static int32_t Script_GetTrainerSelectionIndex(lua_State* L) {
    if (CGClassTrainer::GetSelectionIndex() < 0) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, CGClassTrainer::GetSelectionIndex() + 1);
    }

    return 1;
}

// VERIFY: df37d019 manual
// OFFSET: 0x00595040
static int32_t Script_OpenTrainer(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        WOWGUID guid = player->m_obj->m_guid;
        CGPlayer_C::TalkToTrainer(guid);
    }

    return 0;
}

// VERIFY: 08dbb36d manual
// OFFSET: 0x00595090
static int32_t Script_GetTrainerServiceInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    lua_pushstring(L, CGClassTrainer::GetServiceName(index));
    lua_pushstring(L, CGClassTrainer::GetServiceSubtext(index));
    lua_pushstring(L, CGClassTrainer::GetServiceType(index));

    if (CGClassTrainer::IsCollapsedHeader(index)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    return 4;
}

// VERIFY: 287a55ea auto
// OFFSET: 0x00595150
static int32_t Script_GetTrainerServiceIcon(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceIcon(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service;

    if (index >= CGClassTrainer::m_numServices || !(service = CGClassTrainer::m_services.m_data[index])) {
        lua_pushnil(L);
        return 1;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(service->m_spellID, &spell)) {
        lua_pushnil(L);
        return 1;
    }

    ItemRec* item;

    if (CGClassTrainer::m_trainerType == TRAINER_TYPE_TRADESKILLS && (spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && (item = g_itemDB.GetRecord(spell.m_effectItemType[0]))) {
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        char texture[260];
        SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(item->m_displayInfoID));
        lua_pushstring(L, texture);
        return 1;
    }

    SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_spellIconID);

    if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c73f9ea5 manual
// OFFSET: 0x00595470
static int32_t Script_GetTrainerServiceSkillReq(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceSkillReq(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t met = 1;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    TrainerServiceInfo* service = index < CGClassTrainer::m_numServices ? CGClassTrainer::m_services.m_data[index] : nullptr;
    SkillLineRec* skillLine;

    if (player && service && service->m_skillReq && service->m_skillRankReq && (skillLine = g_skillLineDB.GetRecord(service->m_skillReq))) {
        int32_t i;

        for (i = 0; i < 128; i++) {
            if (player->GetSkillLineID(i) == service->m_skillReq) {
                break;
            }
        }

        if (i >= 128 || static_cast<int32_t>(player->GetSkillValue(i)) < service->m_skillRankReq) {
            met = 0;
        }

        if (skillLine->m_displayName && *skillLine->m_displayName) {
            lua_pushstring(L, skillLine->m_displayName);
            lua_pushnumber(L, service->m_skillRankReq);

            if (met) {
                lua_pushnumber(L, 1.0);
            } else {
                lua_pushnil(L);
            }

            return 3;
        }
    }

    lua_pushnil(L);
    lua_pushnumber(L, 0.0);

    if (met) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: d5f23f70 manual
// OFFSET: 0x005955E0
static int32_t Script_GetTrainerServiceAbilityReq(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetTrainerServiceAbilityReq(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    char text[256] = { 0 };
    uint32_t ability = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    int32_t met = 1;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    TrainerServiceInfo* service = index < CGClassTrainer::m_numServices ? CGClassTrainer::m_services.m_data[index] : nullptr;

    if (player && service && ability < 3) {
        SpellRec spell;

        if (g_spellDB.GetRecord(service->m_abilityReq[ability], &spell)) {
            if (spell.m_nameSubtext && *spell.m_nameSubtext) {
                SStrPrintf(text, sizeof(text), "%s (%s)", spell.m_name, spell.m_nameSubtext);
            } else {
                SStrCopy(text, spell.m_name, sizeof(text));
            }

            if (!player->IsSpellKnown(service->m_abilityReq[ability]) && !player->IsSpellSuperceded(service->m_abilityReq[ability])) {
                met = 0;
            }
        }
    }

    if (text[0]) {
        lua_pushstring(L, text);
    } else {
        lua_pushnil(L);
    }

    if (met) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: f19026cb manual
// OFFSET: 0x005957D0
static int32_t Script_GetTrainerServiceStepReq(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceStepReq(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* name = nullptr;
    int32_t met = 1;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    TrainerServiceInfo* service = index < CGClassTrainer::m_numServices ? CGClassTrainer::m_services.m_data[index] : nullptr;

    if (player && service && service->m_stepSpellID) {
        SpellRec spell;

        if (g_spellDB.GetRecord(service->m_stepSpellID, &spell)) {
            met = 0;
            int32_t effect;

            for (effect = 0; effect < 3; effect++) {
                if (spell.m_effect[effect] == SPELL_EFFECT_SKILL_STEP) {
                    break;
                }
            }

            if (effect < 3) {
                for (int32_t i = 0; i < 128; i++) {
                    uint16_t skillId = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerSkillInfo[i].m_skillId : 0;

                    if (skillId == spell.m_effectMiscValue[effect]) {
                        int32_t minPoints;
                        int32_t maxPoints;
                        Spell_C_GetMinMaxPoints(&spell, effect, &minPoints, &maxPoints, 0, 0, 0, 0);

                        if (static_cast<int32_t>(player->GetSkillMaxRank(i)) >= 5 * maxPoints) {
                            met = 1;
                        }

                        break;
                    }
                }
            }

            name = spell.m_name;
        }
    }

    if (name && *name) {
        lua_pushstring(L, name);
    } else {
        lua_pushnil(L);
    }

    if (met) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 98de4b57 auto
// OFFSET: 0x005959D0
static int32_t Script_GetTrainerServiceDescription(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceDescription(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service;

    if (index < CGClassTrainer::m_numServices && (service = CGClassTrainer::m_services.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(service->m_spellID, &spell)) {
            if (spell.m_description && *spell.m_description) {
                char text[1024];
                SpellParserParseText(&spell, text, sizeof(text), 0, 0, 0, 0, 1, 0);
                lua_pushstring(L, text);
                return 1;
            }

            if ((spell.m_attributes & 0x20) && (spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && spell.m_effectItemType[0]) {
                WOWGUID guid;
                guid.m_guid = static_cast<uint32_t>(spell.m_id) | 0x1FE0000000000000ull;
                ItemStats_C* stats = g_itemDBCache.GetRecord(spell.m_effectItemType[0], guid, TrainerItemCallback, nullptr, false);

                if (stats && stats->m_description && *stats->m_description) {
                    lua_pushstring(L, stats->m_description);
                    return 1;
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 7d343579 auto
// OFFSET: 0x00595C40
static int32_t Script_GetTrainerServiceStepIncrease(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceStepIncrease(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service;

    if (index < CGClassTrainer::m_numServices && (service = CGClassTrainer::m_services.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(service->m_spellID, &spell)) {
            int32_t effect;

            for (effect = 0; effect < 3; effect++) {
                if (spell.m_effect[effect] == SPELL_EFFECT_SKILL_STEP) {
                    break;
                }
            }

            if (effect < 3 && service->m_state != TRAINER_SERVICE_USED) {
                int32_t previous = 0;
                SpellRec step;
                int32_t minPoints;
                int32_t maxPoints;

                if (g_spellDB.GetRecord(service->m_stepSpellID, &step)) {
                    int32_t stepEffect;

                    for (stepEffect = 0; stepEffect < 3; stepEffect++) {
                        if (step.m_effect[stepEffect] == SPELL_EFFECT_SKILL_STEP) {
                            break;
                        }
                    }

                    if (stepEffect < 3) {
                        Spell_C_GetMinMaxPoints(&step, stepEffect, &minPoints, &maxPoints, 0, 0, 0, 0);
                        previous = maxPoints;
                    }
                }

                Spell_C_GetMinMaxPoints(&spell, effect, &minPoints, &maxPoints, 0, 0, 0, 0);
                int32_t increase = maxPoints - previous;
                SkillLineRec* skillLine;

                if (maxPoints != previous && (skillLine = g_skillLineDB.GetRecord(spell.m_effectMiscValue[effect]))) {
                    char format[256];
                    char text[256];
                    SStrCopy(format, FrameScript_GetText("INCREASE_POTENTIAL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, skillLine->m_displayName, 5 * increase);
                    lua_pushstring(L, text);
                    return 1;
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: f83e44a1 auto
// OFFSET: 0x00595E60
static int32_t Script_BuyTrainerService(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: BuyTrainerService(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        CGPlayer_C::TrainerBuySpell(index);
    } else {
        CGClassTrainer::BuyService();
    }

    return 0;
}

// VERIFY: 71ea3b5a auto
// OFFSET: 0x00595EC0
static int32_t Script_SetTrainerServiceTypeFilter(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetTrainerServiceTypeFilter(\"type\", on/off [, exclusive])");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(type, "all", STORM_MAX_STR)) {
        g_serviceTypeFilterCVar->Set(7, true, false, false, true);
        CGClassTrainer::FilterAndSortServices();
        FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
        return 0;
    }

    TRAINER_SERVICE serviceType = GetServiceTypeFromString(type);

    if (serviceType == NUM_TRAINER_SERVICE_TYPES) {
        luaL_error(L, "Bad service type in SetTrainerServiceTypeFilter");
        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Missing on/off parameter in SetTrainerServiceTypeFilter");
        return 0;
    }

    int32_t filter = g_serviceTypeFilterCVar->m_intValue;

    if (!static_cast<int32_t>(lua_tonumber(L, 2))) {
        CGClassTrainer::SetServiceTypeFilter(filter & ~(1 << serviceType));
        return 0;
    }

    if (lua_isnumber(L, 3) && static_cast<int32_t>(lua_tonumber(L, 3))) {
        CGClassTrainer::SetServiceTypeFilter(1 << serviceType);
        return 0;
    }

    CGClassTrainer::SetServiceTypeFilter(filter | (1 << serviceType));
    return 0;
}

// VERIFY: cb7d0499 auto
// OFFSET: 0x00596010
static int32_t Script_SetTrainerSkillLineFilter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetTrainerSkillLineFilter(index [, on\\off, exclusive])");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0) {
        CGClassTrainer::m_skillLineFilter = 0xFFFFFFFF;
        CGClassTrainer::FilterAndSortServices();
        FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
        return 0;
    }

    if (index >= CGClassTrainer::m_numSkillLines) {
        luaL_error(L, "Bad skill line in SetTrainerSkillLineFilter");
        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Missing on//off parameter in SetTrainerSkillLineFilter");
        return 0;
    }

    uint32_t filter = CGClassTrainer::m_skillLineFilter;

    if (!static_cast<int32_t>(lua_tonumber(L, 2))) {
        CGClassTrainer::SetSkillLineFilter(filter & ~(1 << index));
        return 0;
    }

    if (lua_isnumber(L, 3) && static_cast<int32_t>(lua_tonumber(L, 3))) {
        CGClassTrainer::SetSkillLineFilter(1 << index);
        return 0;
    }

    CGClassTrainer::SetSkillLineFilter(filter | (1 << index));
    return 0;
}

// VERIFY: 05a5d0f5 auto
// OFFSET: 0x00596150
static int32_t Script_CollapseTrainerSkillLine(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CollapseTrainerSkillLine(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        int32_t skillLine = CGClassTrainer::GetSkillLineIndexFromService(index);

        if (skillLine < 0) {
            luaL_error(L, "Bad skill line in CollapseTrainerSkillLine");
            return 0;
        }

        CGClassTrainer::m_expandedSkillLines &= ~(1 << skillLine);
    } else {
        CGClassTrainer::m_expandedSkillLines = 0;
    }

    CGClassTrainer::FilterAndSortServices();
    FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
    return 0;
}

// VERIFY: 9515160f auto
// OFFSET: 0x005961F0
static int32_t Script_ExpandTrainerSkillLine(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ExpandTrainerSkillLine(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        int32_t skillLine = CGClassTrainer::GetSkillLineIndexFromService(index);

        if (skillLine < 0) {
            luaL_error(L, "Bad skill line in ExpandTrainerSkillLine");
            return 0;
        }

        CGClassTrainer::m_expandedSkillLines |= 1 << skillLine;
    } else {
        CGClassTrainer::m_expandedSkillLines = 0xFFFFFFFF;
    }

    CGClassTrainer::FilterAndSortServices();
    FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
    return 0;
}

// VERIFY: 34d380d3 auto
// OFFSET: 0x00596290
static int32_t Script_GetTrainerServiceItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTrainerServiceItemLink(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TrainerServiceInfo* service;

    if (index < CGClassTrainer::m_numServices && (service = CGClassTrainer::m_services.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(service->m_spellID, &spell) && CGClassTrainer::m_trainerType == TRAINER_TYPE_TRADESKILLS && (spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && spell.m_effectItemType[0]) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(spell.m_effectItemType[0], none, nullptr, nullptr, false);

            if (stats) {
                lua_pushstring(L, CGTooltip::GetItemHyperlinkString(spell.m_effectItemType[0], stats->m_quality, 0, 0, 0));
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// STATIC: 0x00ACF3E8
static FrameScript_Method s_ScriptFunctions[] = {
    { "OpenTrainer", &Script_OpenTrainer },
    { "CloseTrainer", &Script_CloseTrainer },
    { "GetNumTrainerServices", &Script_GetNumTrainerServices },
    { "GetTrainerServiceInfo", &Script_GetTrainerServiceInfo },
    { "SelectTrainerService", &Script_SelectTrainerService },
    { "IsTradeskillTrainer", &Script_IsTradeskillTrainer },
    { "GetTrainerSelectionIndex", &Script_GetTrainerSelectionIndex },
    { "GetTrainerGreetingText", &Script_GetTrainerGreetingText },
    { "GetTrainerServiceIcon", &Script_GetTrainerServiceIcon },
    { "GetTrainerServiceSkillLine", &Script_GetTrainerServiceSkillLine },
    { "GetTrainerServiceCost", &Script_GetTrainerServiceCost },
    { "GetTrainerServiceLevelReq", &Script_GetTrainerServiceLevelReq },
    { "GetTrainerServiceSkillReq", &Script_GetTrainerServiceSkillReq },
    { "GetTrainerServiceNumAbilityReq", &Script_GetTrainerServiceNumAbilityReq },
    { "GetTrainerServiceAbilityReq", &Script_GetTrainerServiceAbilityReq },
    { "GetTrainerServiceStepReq", &Script_GetTrainerServiceStepReq },
    { "GetTrainerServiceDescription", &Script_GetTrainerServiceDescription },
    { "IsTrainerServiceSkillStep", &Script_IsTrainerServiceSkillStep },
    { "GetTrainerServiceStepIncrease", &Script_GetTrainerServiceStepIncrease },
    { "BuyTrainerService", &Script_BuyTrainerService },
    { "SetTrainerServiceTypeFilter", &Script_SetTrainerServiceTypeFilter },
    { "SetTrainerSkillLineFilter", &Script_SetTrainerSkillLineFilter },
    { "GetTrainerServiceTypeFilter", &Script_GetTrainerServiceTypeFilter },
    { "GetTrainerSkillLineFilter", &Script_GetTrainerSkillLineFilter },
    { "GetTrainerSkillLines", &Script_GetTrainerSkillLines },
    { "CollapseTrainerSkillLine", &Script_CollapseTrainerSkillLine },
    { "ExpandTrainerSkillLine", &Script_ExpandTrainerSkillLine },
    { "GetTrainerServiceItemLink", &Script_GetTrainerServiceItemLink },
};

// VERIFY: 9493cc0a auto
// OFFSET: 0x00593F70
void ClassTrainerRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: b1982bc0 auto
// OFFSET: 0x00593FA0
void ClassTrainerUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
