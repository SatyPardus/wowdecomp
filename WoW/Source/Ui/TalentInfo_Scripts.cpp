#include "TalentInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/Rec/TalentRec.h"
#include "WoW/Source/DB/Rec/TalentTabRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// STATIC: 0x00AD05E8
static int32_t s_activateTalentGroupSpells[2] = { 63645, 63644 };

// VERIFY: 29096ec7 manual
// OFFSET: 0x005C5CC0
static int32_t Script_GetNumTalentTabs(lua_State* L) {
    int32_t inspect = StringToBOOL(L, 1, 0);
    int32_t pet = StringToBOOL(L, 2, 0);
    uint32_t count;

    if (inspect) {
        if (pet) {
            lua_pushnumber(L, 0.0);
            return 1;
        }

        count = CGTalentInfo_Inspect::m_tabs.m_count;
    } else {
        count = pet ? CGTalentInfo::m_petTabs.m_count : CGTalentInfo::m_tabs.m_count;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: b59980be manual
// OFFSET: 0x005C5D40
static int32_t Script_GetNumTalents(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetNumTalents(tabIndex[, isInspect[, isPet]])");
        return 0;
    }

    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t inspect = StringToBOOL(L, 2, 0);
    int32_t pet = StringToBOOL(L, 3, 0);
    TalentTabInfo* tab = CGTalentInfo::GetTab(tabIndex, inspect, pet);
    lua_pushnumber(L, tab ? tab->m_numTalents : 0);
    return 1;
}

// VERIFY: a4b3fa55 manual
// OFFSET: 0x005C5DF0
static int32_t Script_GetNumTalentGroups(lua_State* L) {
    int32_t inspect = StringToBOOL(L, 1, 0);
    int32_t pet = StringToBOOL(L, 2, 0);
    uint32_t count;

    if (inspect) {
        count = CGTalentInfo_Inspect::m_numTalentGroups;
    } else if (pet) {
        count = CGTalentInfo::m_petTalentTab && CGTalentInfo::m_petTabs.m_count;
    } else {
        count = CGTalentInfo::m_numTalentGroups;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: ede92045 manual
// OFFSET: 0x005C5E70
static int32_t Script_SetActiveTalentGroup(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetActiveTalentGroup(groupIndex)");
        return 0;
    }

    uint32_t group = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (group != CGTalentInfo::m_activeTalentGroup && group < CGTalentInfo::m_numTalentGroups && group < 2) {
        Spell_C_ClickSpell(s_activateTalentGroupSpells[group], nullptr, ClntObjMgrGetActivePlayer());
    }

    return 0;
}

// VERIFY: 2d682fac manual
// OFFSET: 0x005C6150
static int32_t Script_GetTalentTabInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTalentTabInfo(tabIndex[, isInspect[, isPet[, groupIndex]]])");
        return 0;
    }

    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t pet = StringToBOOL(L, 3, 0);
    int32_t inspect = StringToBOOL(L, 2, 0);
    TalentTabInfo* info = CGTalentInfo::GetTab(tabIndex, inspect, pet);
    TalentTabRec* tab;

    if (!info || !(tab = g_talentTabDB.GetRecord(info->m_tabId))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushinteger(L, 0);
        lua_pushnil(L);
        lua_pushinteger(L, 0);
        return 5;
    }

    lua_pushstring(L, tab->m_name);
    SpellIconRec* icon = g_spellIconDB.GetRecord(tab->m_spellIconID);

    if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
    } else {
        lua_pushnil(L);
    }

    CGTalentGroup_C* group = TalentInfoGetTalentGroup(TalentInfoGetScriptGroupIndex(L, 4), inspect, pet);
    TalentGroupTab_C* groupTab;

    if (group && (groupTab = group->GetTab(tabIndex))) {
        lua_pushinteger(L, groupTab->m_pointsSpent);
        lua_pushstring(L, tab->m_backgroundFile);
        lua_pushinteger(L, groupTab->m_previewPointsSpent);
    } else {
        lua_pushinteger(L, 0);
        lua_pushstring(L, tab->m_backgroundFile);
        lua_pushinteger(L, 0);
    }

    return 5;
}

// VERIFY: bf0a4c25 manual
// OFFSET: 0x005C6310
static int32_t Script_GetUnspentTalentPoints(lua_State* L) {
    int32_t inspect = StringToBOOL(L, 1, 0);
    int32_t pet = StringToBOOL(L, 2, 0);
    CGTalentGroup_C* group = TalentInfoGetTalentGroup(TalentInfoGetScriptGroupIndex(L, 3), inspect, pet);
    lua_pushinteger(L, group ? group->m_unspentPoints : 0);
    return 1;
}

// VERIFY: 2177362a manual
// OFFSET: 0x005C6420
static int32_t Script_GetGroupPreviewTalentPointsSpent(lua_State* L) {
    int32_t pet = StringToBOOL(L, 1, 0);
    uint32_t group = TalentInfoGetScriptGroupIndex(L, 2);
    lua_pushinteger(L, CGTalentInfo::GetPreviewPointsSpent(group, pet));
    return 1;
}

// VERIFY: 45eacd6e manual
// OFFSET: 0x005C6A10
static int32_t Script_LearnPreviewTalents(lua_State* L) {
    int32_t pet = StringToBOOL(L, 1, 0);
    CGTalentGroup_C* group = CGTalentInfo::GetTalentGroup(CGTalentInfo::GetActiveTalentGroup(pet), pet);

    if (!group) {
        return 0;
    }

    int32_t spent = 0;

    for (uint32_t i = 0; i < group->m_tabs.m_count; i++) {
        spent += group->m_tabs.m_data[i].m_previewPointsSpent;
    }

    if (spent <= 0) {
        return 0;
    }

    CGUnit_C* unit = pet ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : ClntObjMgrGetActivePlayerObj();

    if (!unit) {
        return 0;
    }

    CDataStore msg;

    if (pet) {
        msg.Put(static_cast<uint32_t>(CMSG_LEARN_PREVIEW_TALENTS_PET));
        msg.Put(unit->m_obj->m_guid.m_guid);
    } else {
        msg.Put(static_cast<uint32_t>(CMSG_LEARN_PREVIEW_TALENTS));
    }

    uint32_t count = 0;
    uint32_t countPos = msg.m_size;
    msg.Put(0u);

    for (uint32_t i = 0; i < group->m_tabs.m_count; i++) {
        TalentGroupTab_C* tab = &group->m_tabs.m_data[i];

        for (uint32_t j = 0; j < tab->m_tiers.m_count; j++) {
            TalentGroupTier_C* tier = tab->m_tiers.m_data[j];

            for (TalentGroupTierItem_C* node = tier->m_items.m_fulllist.Head(); node; node = tier->m_items.Next(node)) {
                TalentGroupItem_C* item = node->m_item;

                if (item && item->m_previewRank > item->m_rank) {
                    msg.Put(static_cast<uint32_t>(item->m_talentId));
                    msg.Put(static_cast<uint32_t>(item->m_previewRank));
                    count++;
                }
            }
        }
    }

    if (count) {
        msg.Set(countPos, count);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 2c893b81 manual
// OFFSET: 0x005C7130
static int32_t Script_ResetPreviewTalentPoints(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ResetPreviewTalentPoints(tabIndex[, isPet[, groupIndex]])");
        return 0;
    }

    uint32_t tab = lua_tointeger(L, 1) - 1;
    int32_t pet = StringToBOOL(L, 2, 0);
    uint32_t group = TalentInfoGetScriptGroupIndex(L, 3);
    int32_t before = CGTalentInfo::GetPreviewPointsSpent(group, pet);
    CGTalentInfo::ResetPreviewTalentPoints(group, tab, pet);
    int32_t after = CGTalentInfo::GetPreviewPointsSpent(group, pet);

    if (after != before) {
        FrameScript_SignalEvent(pet ? SCRIPT_EVENT_PREVIEW_PET_TALENT_POINTS_CHANGED : SCRIPT_EVENT_PREVIEW_TALENT_POINTS_CHANGED, "%d%d%d%d", 0, tab + 1, group + 1, after - before);
    }

    return 0;
}

// VERIFY: 50931f10 auto
// OFFSET: 0x005C7200
static int32_t Script_ResetGroupPreviewTalentPoints(lua_State* L) {
    int32_t pet = StringToBOOL(L, 1, 0);
    uint32_t group = TalentInfoGetScriptGroupIndex(L, 2);
    int32_t before = CGTalentInfo::GetPreviewPointsSpent(group, pet);
    CGTalentInfo::ResetGroupPreviewTalentPoints(group, pet);
    int32_t after = CGTalentInfo::GetPreviewPointsSpent(group, pet);

    if (after != before) {
        FrameScript_SignalEvent(pet ? SCRIPT_EVENT_PREVIEW_PET_TALENT_POINTS_CHANGED : SCRIPT_EVENT_PREVIEW_TALENT_POINTS_CHANGED, "%d%d%d%d", 0, 0, group + 1, after - before);
    }

    return 0;
}

// VERIFY: 1ba1dad4 manual
// OFFSET: 0x005C7800
static int32_t Script_GetTalentInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetTalentInfo(tabIndex, talentIndex[, isInspect[, isPet[, groupIndex]]])");
        return 0;
    }

    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t talentIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    int32_t inspect = StringToBOOL(L, 3, 0);
    int32_t pet = StringToBOOL(L, 4, 0);
    TalentTabInfo* tab = CGTalentInfo::GetTab(tabIndex, inspect, pet);

    if (tab && talentIndex < tab->m_numTalents) {
        CGUnit_C* unit = pet ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : ClntObjMgrGetActivePlayerObj();
        const TalentRec* talent = &tab->m_talents[talentIndex];

        if (talent && (!pet || CGTalentInfo::IsTalentAvailableForPet(unit, talent))) {
            int32_t maxRank = -1;

            for (int32_t i = 8; i >= 0; i--) {
                if (talent->m_spellRank[i] && maxRank < 0) {
                    maxRank = i;
                }
            }

            int32_t rank = -1;
            int32_t previewRank = -1;
            TalentGroupItem_C* item = TalentInfoGetTalentItem(TalentInfoGetScriptGroupIndex(L, 5), talent->m_id, inspect, pet);

            if (item) {
                rank = item->m_rank;
                previewRank = item->m_previewRank;
            }

            SpellRec spell;

            if (g_spellDB.GetRecord(rank < 0 ? talent->m_spellRank[0] : talent->m_spellRank[rank], &spell)) {
                lua_pushstring(L, spell.m_name);
                SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_spellIconID);

                if (icon) {
                    lua_pushstring(L, icon->m_textureFilename);
                } else {
                    lua_pushnil(L);
                }

                lua_pushnumber(L, talent->m_tierID + 1);
                lua_pushnumber(L, talent->m_columnIndex + 1);
                lua_pushnumber(L, rank + 1);
                lua_pushnumber(L, maxRank + 1);

                if (talent->m_flags & 1) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                int32_t available = 0;

                if (inspect) {
                    available = !pet && rank >= 0;
                } else if (unit) {
                    int32_t requiredSpell = talent->m_requiredSpellID;
                    available = 1;

                    if (requiredSpell && !pet) {
                        CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

                        if ((!player || (!player->IsSpellKnown(requiredSpell) && !player->IsSpellSuperceded(requiredSpell))) && !unit->IsSpellSuperceded(talent->m_requiredSpellID)) {
                            available = 0;
                        }
                    }
                }

                if (available) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                lua_pushnumber(L, previewRank + 1);

                int32_t previewAvailable = 0;

                if (inspect) {
                    previewAvailable = !pet && previewRank >= 0;
                } else if (unit) {
                    int32_t requiredSpell = talent->m_requiredSpellID;
                    previewAvailable = 1;

                    if (requiredSpell && !pet) {
                        CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

                        if ((!player || (!player->IsSpellKnown(requiredSpell) && !player->IsSpellSuperceded(requiredSpell))) && !unit->IsSpellSuperceded(talent->m_requiredSpellID)) {
                            previewAvailable = 0;
                        }
                    }
                }

                if (previewAvailable) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                return 10;
            }
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    return 10;
}

// VERIFY: 6dac478d manual
// OFFSET: 0x005C7ED0
static int32_t Script_GetTalentPrereqs(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetTalentPrereqs(tabIndex, talentIndex[, isInspect[, isPet[, groupIndex]]])");
        return 0;
    }

    int32_t count = 0;
    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t talentIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    int32_t inspect = StringToBOOL(L, 3, 0);
    int32_t pet = StringToBOOL(L, 4, 0);
    uint32_t group = TalentInfoGetScriptGroupIndex(L, 5);
    TalentTabInfo* tab = CGTalentInfo::GetTab(tabIndex, inspect, pet);

    if (tab && talentIndex < tab->m_numTalents) {
        const TalentRec* talent = &tab->m_talents[talentIndex];

        if (talent) {
            for (uint32_t i = 0; i < 3; i++) {
                int32_t prereqId = talent->m_prereqTalent[i];

                if (!prereqId) {
                    continue;
                }

                TalentRec* prereq = g_talentDB.GetRecord(prereqId);

                if (!prereq) {
                    continue;
                }

                lua_checkstack(L, 4);
                lua_pushnumber(L, prereq->m_tierID + 1);
                lua_pushnumber(L, prereq->m_columnIndex + 1);
                TalentGroupItem_C* item = TalentInfoGetTalentItem(group, prereqId, inspect, pet);

                if (item) {
                    if (item->m_rank < talent->m_prereqRank[i]) {
                        lua_pushnil(L);
                    } else {
                        lua_pushnumber(L, 1.0);
                    }

                    if (item->m_previewRank < talent->m_prereqRank[i]) {
                        lua_pushnil(L);
                    } else {
                        lua_pushnumber(L, 1.0);
                    }
                } else {
                    lua_pushnil(L);
                    lua_pushnil(L);
                }

                count++;
            }
        }
    }

    return 4 * count;
}

// VERIFY: 47fcce66 auto
// OFFSET: 0x005C7CE0
static int32_t Script_GetTalentLink(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetTalentLink(tabIndex, talentIndex[, isInspect[, isPet[, groupIndex[, isPreview]]]])");
        return 0;
    }

    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t talentIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    int32_t inspect = StringToBOOL(L, 3, 0);
    int32_t pet = StringToBOOL(L, 4, 0);
    TalentTabInfo* tab = CGTalentInfo::GetTab(tabIndex, inspect, pet);

    if (tab && talentIndex < tab->m_numTalents) {
        CGUnit_C* unit = pet ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : nullptr;
        const TalentRec* talent = &tab->m_talents[talentIndex];

        if (talent && (!pet || CGTalentInfo::IsTalentAvailableForPet(unit, talent))) {
            int32_t rank = -1;
            TalentGroupItem_C* item = TalentInfoGetTalentItem(TalentInfoGetScriptGroupIndex(L, 5), talent->m_id, inspect, pet);

            if (item) {
                rank = StringToBOOL(L, 5, 0) ? item->m_previewRank : item->m_rank;
            }

            SpellRec spell;

            if (g_spellDB.GetRecord(talent->m_spellRank[rank < 0 ? 0 : rank], &spell)) {
                lua_pushstring(L, CGTooltip::GetTalentHyperlinkString(talent, spell.m_name, rank));
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 141c8756 manual
// OFFSET: 0x005C80E0
static int32_t Script_LearnTalent(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: LearnTalent(tabIndex, talentIndex[, isPet])");
        return 0;
    }

    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t talentIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    int32_t pet = StringToBOOL(L, 3, 0);
    CGUnit_C* unit = pet ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : ClntObjMgrGetActivePlayerObj();
    TalentTabInfo* tab = CGTalentInfo::GetTab(tabIndex, 0, pet);

    if (!unit || !tab || talentIndex >= tab->m_numTalents) {
        return 0;
    }

    const TalentRec* talent = &tab->m_talents[talentIndex];

    if (!talent || (pet && !CGTalentInfo::IsTalentAvailableForPet(unit, talent))) {
        return 0;
    }

    int32_t rank = -1;
    int32_t maxRank = -1;

    for (int32_t i = 8; i >= 0; i--) {
        if (talent->m_spellRank[i]) {
            maxRank = i;
            break;
        }
    }

    TalentGroupItem_C* item = CGTalentInfo::GetTalentItem(CGTalentInfo::GetActiveTalentGroup(pet), talent->m_id, pet);

    if (item) {
        rank = item->m_rank;
    }

    if (rank < maxRank) {
        CDataStore msg;

        if (pet) {
            msg.Put(static_cast<uint32_t>(CMSG_PET_LEARN_TALENT));
            msg.Put(unit->m_obj->m_guid.m_guid);
        } else {
            msg.Put(static_cast<uint32_t>(CMSG_LEARN_TALENT));
        }

        msg.Put(static_cast<uint32_t>(talent->m_id));
        msg.Put(static_cast<uint32_t>(rank + 1));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: d7959687 manual
// OFFSET: 0x005C9590
static int32_t Script_AddPreviewTalentPoints(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: AddPreviewTalentPoints(tabIndex, talentIndex, points[, isPet[, groupIndex]])");
        return 0;
    }

    uint32_t tab = lua_tointeger(L, 1) - 1;
    uint32_t talent = lua_tointeger(L, 2) - 1;
    int32_t points = lua_tointeger(L, 3);
    int32_t pet = StringToBOOL(L, 4, 0);
    uint32_t group = TalentInfoGetScriptGroupIndex(L, 5);
    int32_t before = CGTalentInfo::GetPreviewPointsSpent(group, pet);
    CGTalentInfo::AddPreviewTalentPoints(group, tab, talent, points, pet);
    int32_t after = CGTalentInfo::GetPreviewPointsSpent(group, pet);

    if (after != before) {
        FrameScript_SignalEvent(pet ? SCRIPT_EVENT_PREVIEW_PET_TALENT_POINTS_CHANGED : SCRIPT_EVENT_PREVIEW_TALENT_POINTS_CHANGED, "%d%d%d%d", talent + 1, tab + 1, group + 1, after - before);
    }

    return 0;
}

// VERIFY: 3e3b2c80 auto
// OFFSET: 0x005C5810
static int32_t Script_GetActiveTalentGroup(lua_State* L) {
    int32_t inspect = StringToBOOL(L, 1, 0);
    int32_t pet = StringToBOOL(L, 2, 0);
    uint32_t group;

    if (inspect) {
        group = CGTalentInfo_Inspect::m_activeTalentGroup;
    } else {
        group = pet ? 0 : CGTalentInfo::m_activeTalentGroup;
    }

    lua_pushnumber(L, group + 1);
    return 1;
}

// VERIFY: 8eb75c42 auto
// OFFSET: 0x005C63B0
static int32_t Script_GetPreviewTalentPointsSpent(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPreviewTalentPointsSpent(tabIndex[, isPet[, groupIndex]])");
        return 0;
    }

    lua_tointeger(L, 1);
    StringToBOOL(L, 2, 0);

    if (lua_isnumber(L, 3)) {
        lua_tointeger(L, 3);
    }

    lua_pushinteger(L, 0);
    return 1;
}

// STATIC: 0x00AD05F0
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumTalentTabs", &Script_GetNumTalentTabs },
    { "GetTalentTabInfo", &Script_GetTalentTabInfo },
    { "GetNumTalents", &Script_GetNumTalents },
    { "GetTalentInfo", &Script_GetTalentInfo },
    { "GetTalentLink", &Script_GetTalentLink },
    { "GetTalentPrereqs", &Script_GetTalentPrereqs },
    { "LearnTalent", &Script_LearnTalent },
    { "GetUnspentTalentPoints", &Script_GetUnspentTalentPoints },
    { "GetNumTalentGroups", &Script_GetNumTalentGroups },
    { "GetActiveTalentGroup", &Script_GetActiveTalentGroup },
    { "SetActiveTalentGroup", &Script_SetActiveTalentGroup },
    { "GetPreviewTalentPointsSpent", &Script_GetPreviewTalentPointsSpent },
    { "GetGroupPreviewTalentPointsSpent", &Script_GetGroupPreviewTalentPointsSpent },
    { "AddPreviewTalentPoints", &Script_AddPreviewTalentPoints },
    { "ResetPreviewTalentPoints", &Script_ResetPreviewTalentPoints },
    { "ResetGroupPreviewTalentPoints", &Script_ResetGroupPreviewTalentPoints },
    { "LearnPreviewTalents", &Script_LearnPreviewTalents },
};

// VERIFY: c3db1349 auto
// OFFSET: 0x005C5880
void TalentInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 3367c21b auto
// OFFSET: 0x005C58B0
void TalentInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
