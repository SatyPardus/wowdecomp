#include "SkillInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include <cmath>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SkillRaceClassInfoRec.h"
#include "WoW/Source/Object/Unit.h"

// VERIFY: b1ab49e5 manual
// OFFSET: 0x005CD1B0
static int32_t Script_GetNumSkillLines(lua_State* L) {
    lua_pushnumber(L, CGSkillInfo::m_numSkillLines);
    return 1;
}

// VERIFY: 6e3565d9 auto
// OFFSET: 0x005CD820
static int32_t Script_CancelSkillUps(lua_State* L) {
    for (uint32_t i = 0; i < CGSkillInfo::m_numLines; i++) {
        CGSkillInfo::m_lines.m_data[i]->m_skillUps = 0;
    }

    CGSkillInfo::m_skillUpPoints = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_SKILL_LINES_CHANGED, nullptr);
    return 0;
}

// VERIFY: 7fed7462 manual
// OFFSET: 0x005CD860
static int32_t Script_SetSelectedSkill(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetSelectedSkill(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGSkillInfo::m_selectedSkill = index >= CGSkillInfo::m_numLines ? 0 : CGSkillInfo::m_lines.m_data[index]->m_skillLineID;
    return 0;
}

// VERIFY: fa236c5d auto
// OFFSET: 0x005CD8F0
static int32_t Script_GetSelectedSkill(lua_State* L) {
    lua_pushnumber(L, CGSkillInfo::GetSelectedIndex() + 1);
    return 1;
}

// VERIFY: 7eaea3f3 manual
// OFFSET: 0x005CE540
static int32_t Script_GetAdjustedSkillPoints(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t points = 0;

    if (player) {
        if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            points = player->m_player->m_playerCharacterPoints[1] - CGSkillInfo::m_skillUpPoints;
        } else {
            points = -CGSkillInfo::m_skillUpPoints;
        }
    }

    lua_pushnumber(L, points);
    return 1;
}

// VERIFY: e41d75e7 manual
// OFFSET: 0x005CDE20
static int32_t Script_GetSkillLineInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetSkillLineInfo(index)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    SkillLineInfo* line = index < CGSkillInfo::m_numLines ? CGSkillInfo::m_lines.m_data[index] : nullptr;

    if (player && line) {
        if (!line->m_skillLineID) {
            SkillLineCategoryRec* category = g_skillLineCategoryDB.GetRecord(line->m_categoryID);

            if (category) {
                lua_pushstring(L, category->m_name);
                lua_pushnumber(L, 1.0);

                if (CGSkillInfo::GetSkillTypeCollapsed(index)) {
                    lua_pushnil(L);
                } else {
                    lua_pushnumber(L, 1.0);
                }

                lua_pushnumber(L, 0.0);
                lua_pushnumber(L, 0.0);
                lua_pushnumber(L, 0.0);
                lua_pushnumber(L, 0.0);
                lua_pushnil(L);
                lua_pushnil(L);
                lua_pushnil(L);
                lua_pushnumber(L, 0.0);
                lua_pushnumber(L, 0.0);
                return 12;
            }
        } else {
            SkillLineRec* skillLine = g_skillLineDB.GetRecord(line->m_skillLineID);
            const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], line->m_skillLineID);

            if (skillLine) {
                lua_pushstring(L, skillLine->m_displayName);
                lua_pushnil(L);
                lua_pushnil(L);

                int32_t skillIndex = player->GetSkillIndex(line->m_skillLineID);
                int32_t value = player->GetSkillValue(skillIndex);
                int32_t maxRank = player->GetSkillMaxRank(skillIndex);
                uint32_t step = player->GetSkillStep(skillIndex);
                int32_t hand = 0;

                while (player->GetSkillLineForSlot(hand) != line->m_skillLineID) {
                    if (++hand >= 3) {
                        break;
                    }
                }

                if (hand < 3) {
                    int32_t bonus = static_cast<int32_t>(lrintf(player->GetCombatRatingPct(0) - 0.5f));

                    if (hand == 0) {
                        bonus = static_cast<int32_t>(lrintf(player->GetCombatRatingPct(20) - 0.5f)) + bonus;
                    } else if (hand == 1) {
                        bonus = static_cast<int32_t>(lrintf(player->GetCombatRatingPct(21) - 0.5f)) + bonus;
                    } else if (hand == 2) {
                        bonus = bonus + static_cast<int32_t>(lrintf(player->GetCombatRatingPct(22) - 0.5f));
                    }

                    value += bonus;
                }

                if (info->m_flags & 0x400) {
                    maxRank = 1;

                    if (value > 1) {
                        value = 1;
                    }
                }

                lua_pushnumber(L, value);
                lua_pushnumber(L, static_cast<uint32_t>(line->m_skillUps));
                lua_pushnumber(L, player->GetSkillModifier(skillIndex));
                lua_pushnumber(L, maxRank);

                if (step && (info->m_flags & 0x20)) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                int32_t tierCost;

                if (((info->m_flags & 0x8) || (!step && (info->m_flags & 0x4))) && (tierCost = GetSkillTierCost(info, step)) != 0) {
                    lua_pushnumber(L, tierCost);
                } else {
                    lua_pushnil(L);
                }

                int32_t skillCost = CGSkillInfo::GetSkillCostField(info, value + line->m_skillUps + 1);

                if (skillCost) {
                    lua_pushnumber(L, skillCost);
                } else {
                    lua_pushnil(L);
                }

                lua_pushnumber(L, info->m_minLevel);
                lua_pushnumber(L, info->m_skillCostIndex + 1);
                lua_pushstring(L, skillLine->m_description);
                return 13;
            }
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    return 13;
}

// VERIFY: 9014dcdf manual
// OFFSET: 0x005CE2C0
static int32_t Script_AbandonSkill(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: AbandonSkill(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < CGSkillInfo::m_numLines) {
        SkillLineInfo* line = CGSkillInfo::m_lines.m_data[index];

        if (line) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_UNLEARN_SKILL));
            msg.Put(static_cast<uint32_t>(line->m_skillLineID));
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    return 0;
}

// VERIFY: 407a57be auto
// OFFSET: 0x005CE3A0
static int32_t Script_CollapseSkillHeader(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CollapseSkillHeader(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        CGSkillInfo::CollapseHeader(index);
    } else {
        CGSkillInfo::m_expandedTypes = 0;
        CGSkillInfo::UpdateSkillLines();
    }

    return 0;
}

// VERIFY: 3836a490 auto
// OFFSET: 0x005CE400
static int32_t Script_ExpandSkillHeader(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ExpandSkillHeader(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        CGSkillInfo::ExpandHeader(index);
    } else {
        CGSkillInfo::m_expandedTypes = -1;
        CGSkillInfo::UpdateSkillLines();
    }

    return 0;
}

// VERIFY: 4bd54e72 auto
// OFFSET: 0x005CE460
static int32_t Script_AddSkillUp(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: AddSkillUp(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGSkillInfo::AddSkillUp(index);
    return 0;
}

// VERIFY: fea9ba45 auto
// OFFSET: 0x005CE4D0
static int32_t Script_RemoveSkillUp(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: RemoveSkillUp(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGSkillInfo::RemoveSkillUp(index);
    return 0;
}

// VERIFY: 32c1e0b1 manual
// OFFSET: 0x005CE5D0
static int32_t Script_AcceptSkillUps(lua_State* L) {
    if (!CGSkillInfo::m_skillUpPoints) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SKILL_BUY_RANK));
    uint32_t count = 0;
    uint32_t countPos = msg.m_size;
    msg.Put(static_cast<uint32_t>(0));

    for (uint32_t i = 0; i < CGSkillInfo::m_numSkillLines; i++) {
        SkillLineInfo* line = i < CGSkillInfo::m_numLines ? CGSkillInfo::m_lines.m_data[i] : nullptr;

        if (line->m_skillLineID && line->m_skillUps) {
            msg.Put(static_cast<uint32_t>(line->m_skillLineID));
            msg.Put(static_cast<uint32_t>(line->m_skillUps));
            count++;
        }
    }

    msg.Set(countPos, count);
    msg.Finalize();
    ClientServices::Send2(&msg);

    for (uint32_t i = 0; i < CGSkillInfo::m_numLines; i++) {
        CGSkillInfo::m_lines.m_data[i]->m_skillUps = 0;
    }

    CGSkillInfo::m_skillUpPoints = 0;
    return 0;
}

// VERIFY: a3367e9c auto
// OFFSET: 0x005CE6F0
static int32_t Script_BuySkillTier(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: BuySkillTier(index)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    SkillLineInfo* line = index < CGSkillInfo::m_numLines ? CGSkillInfo::m_lines.m_data[index] : nullptr;

    if (!player || !line) {
        return 0;
    }

    SkillLineRec* skillLine = g_skillLineDB.GetRecord(line->m_skillLineID);
    const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], line->m_skillLineID);

    if (!skillLine || !info) {
        return 0;
    }

    uint32_t step = player->GetSkillStep(player->GetSkillIndex(line->m_skillLineID));

    if (!(info->m_flags & 8) && (step || !(info->m_flags & 4))) {
        return 0;
    }

    int32_t cost = GetSkillTierCost(info, step);

    if (cost && cost <= player->GetCharacterPoints(1)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SKILL_BUY_STEP));
        msg.Put(static_cast<uint32_t>(line->m_skillLineID));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// STATIC: 0x00AD09B8
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumSkillLines", &Script_GetNumSkillLines },
    { "GetSkillLineInfo", &Script_GetSkillLineInfo },
    { "AbandonSkill", &Script_AbandonSkill },
    { "CollapseSkillHeader", &Script_CollapseSkillHeader },
    { "ExpandSkillHeader", &Script_ExpandSkillHeader },
    { "AddSkillUp", &Script_AddSkillUp },
    { "RemoveSkillUp", &Script_RemoveSkillUp },
    { "GetAdjustedSkillPoints", &Script_GetAdjustedSkillPoints },
    { "AcceptSkillUps", &Script_AcceptSkillUps },
    { "CancelSkillUps", &Script_CancelSkillUps },
    { "BuySkillTier", &Script_BuySkillTier },
    { "SetSelectedSkill", &Script_SetSelectedSkill },
    { "GetSelectedSkill", &Script_GetSelectedSkill },
};

// VERIFY: 8f357cd9 auto
// OFFSET: 0x005CD1F0
void SkillInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: bec2993b auto
// OFFSET: 0x005CD220
void SkillInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
