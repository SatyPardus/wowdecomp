#include "TradeSkillFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaGroupRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SpellFocusObjectRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/TotemCategoryRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/QuestTextParser.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// VERIFY: 580553ac auto
// OFFSET: 0x005DA120
static int32_t Script_GetNumTradeSkills(lua_State* L) {
    lua_pushnumber(L, CGTradeSkillInfo::m_filteredSkills);
    return 1;
}

// VERIFY: e6343d59 auto
// OFFSET: 0x005DA150
static int32_t Script_GetTradeSkillItemNameFilter(lua_State* L) {
    lua_pushstring(L, CGTradeSkillInfo::m_itemNameFilter);
    return 1;
}

// VERIFY: 992ebdfa manual
// OFFSET: 0x005DA2E0
static int32_t Script_StopTradeSkillRepeat(lua_State* L) {
    if (CGTradeSkillInfo::m_recastSpellID || CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }

    return 0;
}

// VERIFY: 1299596b auto
// OFFSET: 0x005DA170
static int32_t Script_GetTradeSkillItemLevelFilter(lua_State* L) {
    int32_t maxLevel = CGTradeSkillInfo::m_itemMaxLevelFilter;
    lua_pushnumber(L, CGTradeSkillInfo::m_itemMinLevelFilter);
    lua_pushnumber(L, maxLevel);
    return 2;
}

// VERIFY: edab51c5 manual
// OFFSET: 0x005DA8B0
static int32_t Script_CloseTradeSkill(lua_State* L) {
    CGTradeSkillInfo::m_spellID = 0;
    CGTradeSkillInfo::m_skillLineID = 0;

    if (CGTradeSkillInfo::m_recastSpellID || CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_CLOSE, nullptr);
    return 0;
}

// VERIFY: 1d8d0d2f manual
// OFFSET: 0x005DA270
static int32_t Script_GetTradeskillRepeatCount(lua_State* L) {
    uint32_t count;

    if (CGTradeSkillInfo::m_recastSpellID && CGTradeSkillInfo::m_recastSpellID == CGTradeSkillInfo::m_selectedSkill) {
        count = CGTradeSkillInfo::m_recastTimes;
    } else if (CGTradeSkillInfo::m_pendingRecastSpellID && CGTradeSkillInfo::m_pendingRecastSpellID == CGTradeSkillInfo::m_selectedSkill) {
        count = CGTradeSkillInfo::m_pendingRecastTimes;
    } else {
        count = 1;
    }

    if (count <= 1) {
        count = 1;
    }

    lua_pushnumber(L, count);
    return 1;
}

// STATIC: 0x00A1B788
static const char s_difficultyNames[NUM_TRADESKILL_CATEGORIES][32] = { "optimal", "medium", "easy", "trivial" };

// VERIFY: 70374745 auto
// OFFSET: 0x005DA1B0
static int32_t Script_GetTradeSkillSubClassFilter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillSubClassFilter(index)");
        return 0;
    }

    uint32_t filter = CGTradeSkillInfo::m_subClassFilter;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0) {
        for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
            if (!((1 << i) & filter)) {
                lua_pushnil(L);
                return 1;
            }
        }

        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (static_cast<uint32_t>(index) >= CGTradeSkillInfo::m_numSubClasses) {
        luaL_error(L, "Bad sub class in GetTradeSkillSubClassFilter");
        return 0;
    }

    if ((1 << index) & filter) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7d97f342 auto
// OFFSET: 0x005DA310
static int32_t Script_IsTradeSkillLinked(lua_State* L) {
    if (CGTradeSkillInfo::m_linkData) {
        lua_pushnumber(L, 1.0);
        WOWGUID none = WOWGUID();
        NameCache* name = g_nameDBCache.GetRecord(CGTradeSkillInfo::m_playerGuid.m_guid, none, nullptr, nullptr, false);
        lua_pushstring(L, name ? name->m_name : nullptr);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 1b04086e auto
// OFFSET: 0x005DA900
static int32_t Script_SelectTradeSkill(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SelectTradeSkill(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < CGTradeSkillInfo::m_numDisplayEntries) {
        CGTradeSkillInfo::m_selectedSkill = CGTradeSkillInfo::m_displayList.m_data[index]->m_spellId;

        if (CGTradeSkillInfo::m_selectedSkill > 0) {
            return 0;
        }
    }

    CGTradeSkillInfo::m_selectedSkill = 0;
    return 0;
}

// VERIFY: 01f23c2c auto
// OFFSET: 0x005DA970
static int32_t Script_GetTradeSkillSelectionIndex(lua_State* L) {
    lua_pushnumber(L, CGTradeSkillInfo::GetSelectionIndex() + 1);
    return 1;
}

// VERIFY: 15ddefb1 auto
// OFFSET: 0x005DA9A0
static int32_t Script_GetTradeSkillCooldown(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillCooldown(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TradeSkillInfo* info;
    uint32_t duration;
    uint32_t start;
    uint32_t enable;

    if (index < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[index]) && info->m_spellId > 0 && Spell_C_GetSpellCooldown(info->m_spellId, 0, &duration, &start, &enable)) {
        uint32_t remaining = start + duration - OsGetAsyncTimeMs();
        lua_pushnumber(L, remaining * 0.001);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3ff43b67 manual
// OFFSET: 0x005DAA70
static int32_t Script_GetTradeSkillSubClasses(lua_State* L) {
    uint32_t count = CGTradeSkillInfo::m_numSubClasses;

    for (uint32_t i = 0; i < count; i++) {
        TradeSkillSubClassInfo* subClass = i < CGTradeSkillInfo::m_numSubClasses ? CGTradeSkillInfo::m_subClassList.m_data[i] : nullptr;

        if (subClass->m_itemClass == -1 && subClass->m_itemSubClass == -1) {
            SkillLineRec* skillLine = g_skillLineDB.GetRecord(CGTradeSkillInfo::m_skillLineID);

            if (skillLine) {
                lua_pushstring(L, skillLine->m_alternateVerb);
            } else {
                lua_pushnil(L);
            }

            continue;
        }

        for (int32_t j = 0; j < g_itemSubClassDB.m_numRecords; j++) {
            ItemSubClassRec* rec = g_itemSubClassDB.GetRecordByIndex(j);

            if (rec->m_classID == subClass->m_itemClass && rec->m_subClassID == subClass->m_itemSubClass) {
                if (rec->m_verboseName && *rec->m_verboseName) {
                    lua_pushstring(L, rec->m_verboseName);
                } else {
                    lua_pushstring(L, rec->m_displayName);
                }

                break;
            }
        }
    }

    return count;
}

// VERIFY: 0f2e6dd9 manual
// OFFSET: 0x005DAB70
static int32_t Script_GetFirstTradeSkill(lua_State* L) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(CGTradeSkillInfo::m_filteredSkills); i++) {
        TradeSkillInfo* info;

        if (i < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[i]) && info->m_spellId > 0) {
            lua_pushnumber(L, i + 1);
            return 1;
        }
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: 84674611 auto
// OFFSET: 0x005DAC00
static int32_t Script_DoTradeSkill(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: DoTradeSkill(index [, repeat])");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t times = 1;

    if (lua_isnumber(L, 2)) {
        times = static_cast<uint32_t>(lua_tonumber(L, 2));
    }

    if (index < CGTradeSkillInfo::m_numDisplayEntries) {
        TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

        if (info) {
            uint32_t available = info->m_numAvailable;

            if (available != 0xFFFFFFFF && times >= available) {
                times = info->m_numAvailable;
            }

            if (CGTradeSkillInfo::m_recastSpellID == info->m_spellId || Spell_C::CastSpell(info->m_spellId, nullptr, WOWGUID{ 0 }, false)) {
                CGTradeSkillInfo::DoTradeSkill(info->m_spellId, times);
            }
        }
    }

    return 0;
}

// VERIFY: 7c0942ae manual
// OFFSET: 0x005DB2A0
static int32_t Script_GetTradeSkillInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TradeSkillInfo* info;

    if (index < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[index])) {
        if (info->m_spellId == -1) {
            bool skillHeader = info->m_itemClass == -1 && info->m_itemSubClass == -1;
            ItemSubClassRec* rec = nullptr;

            if (!skillHeader) {
                for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
                    ItemSubClassRec* subClass = g_itemSubClassDB.GetRecordByIndex(i);

                    if (subClass->m_classID == info->m_itemClass && subClass->m_subClassID == info->m_itemSubClass) {
                        rec = subClass;
                        break;
                    }
                }
            }

            if (skillHeader || rec) {
                if (skillHeader) {
                    SkillLineRec* skillLine = g_skillLineDB.GetRecord(CGTradeSkillInfo::m_skillLineID);

                    if (skillLine) {
                        lua_pushstring(L, skillLine->m_alternateVerb);
                    } else {
                        lua_pushnil(L);
                    }
                } else {
                    lua_pushstring(L, rec->m_verboseName && *rec->m_verboseName ? rec->m_verboseName : rec->m_displayName);
                }

                lua_pushstring(L, "header");
                lua_pushnumber(L, 0.0);

                if (CGTradeSkillInfo::IsCollapsedHeader(index)) {
                    lua_pushnil(L);
                } else {
                    lua_pushnumber(L, 1.0);
                }

                lua_pushnil(L);
                return 5;
            }
        } else {
            SpellRec spell;

            if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
                lua_pushstring(L, spell.m_name);
                lua_pushstring(L, s_difficultyNames[info->m_difficulty]);
                lua_pushnumber(L, info->m_numAvailable);
                lua_pushnil(L);

                SkillLineRec* skillLine;

                if (spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM || !(skillLine = g_skillLineDB.GetRecord(CGTradeSkillInfo::m_skillLineID))) {
                    lua_pushnil(L);
                } else {
                    lua_pushstring(L, skillLine->m_alternateVerb);
                }

                return 5;
            }
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnil(L);
    return 5;
}

// VERIFY: 5c82d036 auto
// OFFSET: 0x005DB550
static int32_t Script_GetTradeSkillIcon(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillIcon(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TradeSkillInfo* info;

    if (index < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
            if ((spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && spell.m_effectItemType[0]) {
                ItemRec* item = g_itemDB.GetRecord(spell.m_effectItemType[0]);

                if (item) {
                    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
                    const char* separator = *path ? "\\" : "";
                    char texture[260];
                    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, separator, CGItem_C::GetInventoryArt(item->m_displayInfoID));
                    lua_pushstring(L, texture);
                    return 1;
                }
            } else {
                SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_spellIconID);

                if (icon) {
                    lua_pushstring(L, icon->m_textureFilename);
                    return 1;
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: dbf6e832 auto
// OFFSET: 0x005DB6E0
static int32_t Script_GetTradeSkillNumMade(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillNumMade(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TradeSkillInfo* info;

    if (index < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
            int32_t minMade;
            int32_t maxMade;
            Spell_C_GetMinMaxPoints(&spell, 0, &minMade, &maxMade, 0, 0, 0, 0);

            if (minMade < 1) {
                minMade = 1;
            }

            if (maxMade < 1) {
                maxMade = 1;
            }

            lua_pushnumber(L, minMade);
            lua_pushnumber(L, maxMade);
            return 2;
        }
    }

    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 1.0);
    return 2;
}

// VERIFY: c6337250 manual
// OFFSET: 0x005DB810
static int32_t Script_GetTradeSkillLine(lua_State* L) {
    SkillLineRec* skillLine = g_skillLineDB.GetRecord(CGTradeSkillInfo::m_skillLineID);

    if (!skillLine) {
        lua_pushstring(L, "UNKNOWN");
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 3;
    }

    lua_pushstring(L, skillLine->m_displayName);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t rank;

    if (player && CGTradeSkillInfo::m_playerGuid == player->m_obj->m_guid) {
        rank = player->GetSkillValue(player->GetSkillIndex(CGTradeSkillInfo::m_skillLineID));
    } else {
        rank = CGTradeSkillInfo::m_skillRank;
    }

    lua_pushnumber(L, rank);
    lua_pushnumber(L, CGTradeSkillInfo::GetSkillMaxRank());
    return 3;
}

// VERIFY: aadc4baa auto
// OFFSET: 0x005DB920
static int32_t Script_GetTradeSkillRecipeLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillRecipeLink(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGTradeSkillInfo::m_numDisplayEntries) {
        return 0;
    }

    TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

    if (!info) {
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(info->m_spellId, &spell) || !(spell.m_attributes & 0x20)) {
        return 0;
    }

    WOWGUID guid = ClntObjMgrGetActivePlayer();
    SkillLineRec* skillLine = CGTradeSkillInfo::GetSkillLineRec(info->m_spellId, guid);
    char link[1024];

    if (skillLine) {
        SStrPrintf(link, sizeof(link), "|cffffd000|Henchant:%d|h[%s: %s]|h|r", spell.m_id, skillLine->m_displayName, spell.m_name);
    } else {
        SStrPrintf(link, sizeof(link), "|cffffd000|Henchant:%d|h[%s]|h|r", spell.m_id, spell.m_name);
    }

    lua_pushstring(L, link);
    return 1;
}

// VERIFY: 50491ff5 auto
// OFFSET: 0x005DBA70
static int32_t Script_GetTradeSkillItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillItemLink(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGTradeSkillInfo::m_numDisplayEntries) {
        return 0;
    }

    TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

    if (!info) {
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(info->m_spellId, &spell)) {
        return 0;
    }

    const char* link;
    char buffer[1024];
    ItemStats_C* stats;

    if (spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) {
        int32_t itemId = spell.m_effectItemType[0];
        WOWGUID none = WOWGUID();

        if ((stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false))) {
            link = CGTooltip::GetItemHyperlinkString(itemId, stats->m_quality, 0, 0, 0);
            lua_pushstring(L, link);
            return 1;
        }
    }

    SStrPrintf(buffer, sizeof(buffer), "|cffffd000|Henchant:%d|h[%s]|h|r", spell.m_id, spell.m_name);
    link = buffer;
    lua_pushstring(L, link);
    return 1;
}

// VERIFY: 32109739 manual
// OFFSET: 0x005DBBA0
static int32_t Script_GetTradeSkillNumReagents(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillNumReagents(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t count = 0;

    if (index < CGTradeSkillInfo::m_numDisplayEntries) {
        TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

        if (info) {
            SpellRec spell;

            if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
                for (uint32_t i = 0; i < 8; i++) {
                    if (spell.m_reagent[i]) {
                        count++;
                    }
                }
            }
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 56eb94ae manual
// OFFSET: 0x005DBCC0
static int32_t Script_GetTradeSkillReagentItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetTradeReagentSkillItemLink(index, reagentIndex)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t reagentIndex = static_cast<int32_t>(lua_tonumber(L, 2));
    TradeSkillInfo* info;

    if (index < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
            int32_t count = 0;

            for (uint32_t i = 0; i < 8; i++) {
                int32_t itemId = spell.m_reagent[i];

                if (!itemId || ++count != reagentIndex) {
                    continue;
                }

                WOWGUID none = WOWGUID();
                ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

                if (stats) {
                    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(itemId, stats->m_quality, 0, 0, 0));
                    return 1;
                }
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 981c3ee2 auto
// OFFSET: 0x005DBE10
static int32_t Script_GetTradeSkillDescription(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillDescription(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < CGTradeSkillInfo::m_numDisplayEntries) {
        TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

        if (info) {
            SpellRec spell;

            if (g_spellDB.GetRecord(info->m_spellId, &spell) && spell.m_description && *spell.m_description) {
                char description[1024];
                SpellParserParseText(&spell, description, sizeof(description), 0, 0, 0, 0, 1, 0);
                lua_pushstring(L, description);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 9fc2d9e5 manual
// OFFSET: 0x005DBF10
static int32_t Script_GetTradeSkillInvSlots(lua_State* L) {
    int32_t count = 0;
    TradeSkillSubClassInfo* subClass;
    uint32_t mask;

    if (CGTradeSkillInfo::m_subClassFilterIndex < CGTradeSkillInfo::m_numSubClasses && (subClass = CGTradeSkillInfo::m_subClassList.m_data[CGTradeSkillInfo::m_subClassFilterIndex])) {
        mask = subClass->m_invTypeMask;
    } else {
        mask = 0xFFFFFFFF;
    }

    mask &= CGTradeSkillInfo::m_invSlots;

    for (uint32_t i = 0; i < 27; i++) {
        if ((1 << i) & mask) {
            lua_pushstring(L, FrameScript_GetText(s_invSlotTokens[i], -1, GENDER_NOT_APPLICABLE));
            count++;
        }
    }

    return count;
}

// VERIFY: f80437a8 auto
// OFFSET: 0x005DBF90
static int32_t Script_GetTradeSkillInvSlotFilter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillInvSlotFilter(index)");
        return 0;
    }

    uint32_t filter = CGTradeSkillInfo::m_invTypeFilter;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0) {
        if (CGTradeSkillInfo::m_invTypeFilter == 0xFFFFFFFF) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        return 1;
    }

    if (CGTradeSkillInfo::m_invTypeFilter == 0xFFFFFFFF) {
        lua_pushnil(L);
        return 1;
    }

    uint32_t slots = CGTradeSkillInfo::GetInvSlotFilter();
    int32_t count = 0;
    uint32_t slot = 0;

    while (true) {
        if ((1 << slot) & slots) {
            if (count == index) {
                break;
            }

            count++;
        }

        if (++slot >= 27) {
            luaL_error(L, "Bad inv type in GetTradeSkillInvSlotFilter");
            return 0;
        }
    }

    if ((1 << slot) & filter) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: abba7b34 auto
// OFFSET: 0x005DC0B0
static int32_t Script_GetTradeSkillListLink(lua_State* L) {
    char link[256];

    if (CGTradeSkillInfo::GetTradeSkillListLink(CGTradeSkillInfo::m_spellID, link, sizeof(link))) {
        lua_pushstring(L, link);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: b52a6ab1 auto
// OFFSET: 0x005DD070
static int32_t Script_SetTradeSkillItemNameFilter(lua_State* L) {
    CGTradeSkillInfo::SetItemNameFilter(lua_tostring(L, 1));
    return 0;
}

// VERIFY: 6022a2ad auto
// OFFSET: 0x005DD090
static int32_t Script_SetTradeSkillItemLevelFilter(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetTradeSkillItemLevelFilter(minLevel, maxLevel)");
        return 0;
    }

    int32_t maxLevel = lua_tointeger(L, 2);
    int32_t minLevel = lua_tointeger(L, 1);
    CGTradeSkillInfo::SetItemLevelFilter(minLevel, maxLevel);
    return 0;
}

// VERIFY: 58bc80f5 auto
// OFFSET: 0x005DD0F0
static int32_t Script_SetTradeSkillSubClassFilter(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetTradeSkillSubClassFilter(index [, on\\off, exclusive])");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0) {
        CGTradeSkillInfo::m_subClassFilter = 0xFFFFFFFF;
        CGTradeSkillInfo::m_subClassFilterIndex = 0xFFFFFFFF;
        CGTradeSkillInfo::SetInvTypeFilter(1);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
        return 0;
    }

    if (static_cast<uint32_t>(index) >= CGTradeSkillInfo::m_numSubClasses) {
        luaL_error(L, "Bad sub class in SetTradeSkillSubClassFilter");
        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Missing on//off parameter in SetTradeSkillSubClassFilter");
        return 0;
    }

    uint32_t filter = CGTradeSkillInfo::m_subClassFilter;

    if (!static_cast<int32_t>(lua_tonumber(L, 2))) {
        CGTradeSkillInfo::SetSubClassFilter(filter & ~(1 << index), index);
    } else if (lua_isnumber(L, 3) && static_cast<int32_t>(lua_tonumber(L, 3))) {
        CGTradeSkillInfo::SetSubClassFilter(1 << index, index);
    } else {
        CGTradeSkillInfo::SetSubClassFilter(filter | (1 << index), index);
    }

    return 0;
}

// VERIFY: f0baba28 manual
// OFFSET: 0x005DD230
static int32_t Script_SetTradeSkillInvSlotFilter(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetTradeSkillInvSlotFilter(index [, on\\off, exclusive])");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0) {
        CGTradeSkillInfo::m_invTypeFilter = 0xFFFFFFFF;
        CGTradeSkillInfo::SetInvTypeFilter(1);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
        return 0;
    }

    TradeSkillSubClassInfo* subClass;
    uint32_t mask;

    if (CGTradeSkillInfo::m_subClassFilterIndex < CGTradeSkillInfo::m_numSubClasses && (subClass = CGTradeSkillInfo::m_subClassList.m_data[CGTradeSkillInfo::m_subClassFilterIndex])) {
        mask = subClass->m_invTypeMask;
    } else {
        mask = 0xFFFFFFFF;
    }

    int32_t count = 0;
    uint32_t slot = 0;

    while (true) {
        if ((1 << slot) & mask & CGTradeSkillInfo::m_invSlots) {
            if (count == index) {
                break;
            }

            count++;
        }

        if (++slot >= 27) {
            luaL_error(L, "Bad inv slot in SetTradeSkillInvSlotFilter");
        }
    }

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Missing on//off parameter in SetTradeSkillInvSlotFilter");
    }

    uint32_t filter = CGTradeSkillInfo::m_invTypeFilter;

    if (!lua_tonumber(L, 2)) {
        CGTradeSkillInfo::m_invTypeFilter = filter & ~(1 << slot);
    } else if (lua_isnumber(L, 3) && lua_tonumber(L, 3)) {
        CGTradeSkillInfo::SetInvSlotFilter(1 << slot);
        return 0;
    } else {
        CGTradeSkillInfo::m_invTypeFilter = filter | (1 << slot);
    }

    CGTradeSkillInfo::SetInvTypeFilter(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
    return 0;
}

// VERIFY: 049a4657 auto
// OFFSET: 0x005DD3E0
static int32_t Script_TradeSkillOnlyShowMakeable(lua_State* L) {
    int32_t value = StringToBOOL(L, 1, 1);

    if (value != CGTradeSkillInfo::m_showOnlyMakeable) {
        CGTradeSkillInfo::m_showOnlyMakeable = value;
        CGTradeSkillInfo::SetInvTypeFilter(1);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
    }

    return 0;
}

// VERIFY: eead00f2 auto
// OFFSET: 0x005DD420
static int32_t Script_TradeSkillOnlyShowSkillUps(lua_State* L) {
    int32_t value = StringToBOOL(L, 1, 1);

    if (value != CGTradeSkillInfo::m_showOnlySkillUps) {
        CGTradeSkillInfo::m_showOnlySkillUps = value;
        CGTradeSkillInfo::SetInvTypeFilter(1);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
    }

    return 0;
}

// VERIFY: c74e0f4e auto
// OFFSET: 0x005DD460
static int32_t Script_CollapseTradeSkillSubClass(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CollapseTradeSkillSubClass(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        int32_t subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(index);

        if (subClass < 0) {
            luaL_error(L, "Bad sub class in CollapseTradeSkillSubClass");
            return 0;
        }

        CGTradeSkillInfo::m_collapseFilter &= ~(1 << subClass);
    } else {
        CGTradeSkillInfo::m_collapseFilter = 0;
    }

    CGTradeSkillInfo::SetInvTypeFilter(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
    return 0;
}

// VERIFY: 2ef40ac1 auto
// OFFSET: 0x005DD500
static int32_t Script_ExpandTradeSkillSubClass(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ExpandTradeSkillSubClass(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0) {
        int32_t subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(index);

        if (subClass < 0) {
            luaL_error(L, "Bad skill line in ExpandTradeSkillSubClass");
            return 0;
        }

        CGTradeSkillInfo::m_collapseFilter |= 1 << subClass;
    } else {
        CGTradeSkillInfo::m_collapseFilter = 0xFFFFFFFF;
    }

    CGTradeSkillInfo::SetInvTypeFilter(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
    return 0;
}

// VERIFY: 89248ba3 auto
// OFFSET: 0x005DE100
static void TradeSkillItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGTradeSkillInfo::RefreshList(0);
    }
}

// VERIFY: 54980266 manual
// OFFSET: 0x005DE400
static int32_t Script_GetTradeSkillReagentInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetTradeSkillReagentInfo(index, reagentIndex)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t reagentIndex = static_cast<int32_t>(lua_tonumber(L, 2));
    TradeSkillInfo* info;

    if (index < CGTradeSkillInfo::m_numDisplayEntries && (info = CGTradeSkillInfo::m_displayList.m_data[index])) {
        SpellRec spell;

        if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
            int32_t count = 0;

            for (uint32_t i = 0; i < 8; i++) {
                if (!spell.m_reagent[i] || ++count != reagentIndex) {
                    continue;
                }

                int32_t itemId = spell.m_reagent[i];
                WOWGUID requester;
                requester.m_guid = static_cast<uint64_t>(static_cast<int64_t>(info->m_spellId)) | 0x1FE0000000000000ULL;
                ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, requester, TradeSkillItemCallback, nullptr, false);

                if (stats) {
                    lua_pushstring(L, stats->GetName(0));
                    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
                    const char* separator = *path ? "\\" : "";
                    char texture[260];
                    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, separator, CGItem_C::GetInventoryArt(stats->m_displayId));
                    lua_pushstring(L, texture);
                } else {
                    lua_pushnil(L);
                    lua_pushnil(L);
                }

                lua_pushnumber(L, spell.m_reagentCount[i]);
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
                lua_pushnumber(L, player ? player->m_bag.GetItemTypeCount(itemId, 0) : 0.0);
                return 4;
            }
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 4;
}

// VERIFY: db33e997 manual
// OFFSET: 0x005DE620
static int32_t Script_GetTradeSkillTools(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTradeSkillTools(index)");
        return 0;
    }

    int32_t count = 0;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    TradeSkillInfo* info = index < CGTradeSkillInfo::m_numDisplayEntries ? CGTradeSkillInfo::m_displayList.m_data[index] : nullptr;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!info || !player) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(info->m_spellId, &spell)) {
        if (spell.m_requiresSpellFocus) {
            SpellFocusObjectRec* focus = g_spellFocusObjectDB.GetRecord(spell.m_requiresSpellFocus);

            if (focus) {
                lua_checkstack(L, 2);
                lua_pushstring(L, focus->m_name);
                lua_pushnumber(L, 1.0);
                count = 1;
            }
        }

        if (spell.m_requiredAreasID > 0) {
            AreaGroupRec* group = g_areaGroupDB.GetRecord(spell.m_requiredAreasID);

            if (group) {
                AreaTableRec* area = g_areaTableDB.GetRecord(group->m_areaID[0]);

                if (area) {
                    lua_checkstack(L, 2);
                    lua_pushstring(L, area->m_areaName);
                    lua_pushnumber(L, 1.0);
                    count++;
                }
            }
        }

        for (uint32_t i = 0; i < 2; i++) {
            if (!spell.m_totem[i]) {
                continue;
            }

            WOWGUID requester;
            requester.m_guid = static_cast<uint64_t>(static_cast<int64_t>(info->m_spellId)) | 0x1FE0000000000000ULL;
            ItemStats_C* stats = g_itemDBCache.GetRecord(spell.m_totem[i], requester, TradeSkillItemCallback, nullptr, false);

            if (stats) {
                lua_checkstack(L, 2);
                lua_pushstring(L, stats->m_name[0]);

                if (player->m_bag.FindItemOfType(spell.m_totem[i], 0)) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                count++;
            }
        }

        for (uint32_t i = 0; i < 2; i++) {
            TotemCategoryRec* category;

            if (spell.m_requiredTotemCategoryID[i] && (category = g_totemCategoryDB.GetRecord(spell.m_requiredTotemCategoryID[i]))) {
                lua_checkstack(L, 2);
                lua_pushstring(L, category->m_name);

                if (player->m_bag.FindTotemCategory(spell.m_requiredTotemCategoryID[i], 0)) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                count++;
            }
        }
    }

    return 2 * count;
}

// STATIC: 0x00AD0E80
static FrameScript_Method s_ScriptFunctions[] = {
    { "CloseTradeSkill", &Script_CloseTradeSkill },
    { "GetNumTradeSkills", &Script_GetNumTradeSkills },
    { "GetTradeSkillInfo", &Script_GetTradeSkillInfo },
    { "SelectTradeSkill", &Script_SelectTradeSkill },
    { "GetTradeSkillSelectionIndex", &Script_GetTradeSkillSelectionIndex },
    { "GetTradeSkillCooldown", &Script_GetTradeSkillCooldown },
    { "GetTradeSkillIcon", &Script_GetTradeSkillIcon },
    { "GetTradeSkillNumMade", &Script_GetTradeSkillNumMade },
    { "GetTradeSkillLine", &Script_GetTradeSkillLine },
    { "GetTradeSkillItemLink", &Script_GetTradeSkillItemLink },
    { "SetTradeSkillItemNameFilter", &Script_SetTradeSkillItemNameFilter },
    { "GetTradeSkillItemNameFilter", &Script_GetTradeSkillItemNameFilter },
    { "SetTradeSkillItemLevelFilter", &Script_SetTradeSkillItemLevelFilter },
    { "GetTradeSkillItemLevelFilter", &Script_GetTradeSkillItemLevelFilter },
    { "GetTradeSkillNumReagents", &Script_GetTradeSkillNumReagents },
    { "GetTradeSkillReagentInfo", &Script_GetTradeSkillReagentInfo },
    { "GetTradeSkillReagentItemLink", &Script_GetTradeSkillReagentItemLink },
    { "GetTradeSkillTools", &Script_GetTradeSkillTools },
    { "GetTradeSkillDescription", &Script_GetTradeSkillDescription },
    { "GetTradeSkillSubClasses", &Script_GetTradeSkillSubClasses },
    { "GetTradeSkillInvSlots", &Script_GetTradeSkillInvSlots },
    { "SetTradeSkillSubClassFilter", &Script_SetTradeSkillSubClassFilter },
    { "GetTradeSkillSubClassFilter", &Script_GetTradeSkillSubClassFilter },
    { "SetTradeSkillInvSlotFilter", &Script_SetTradeSkillInvSlotFilter },
    { "GetTradeSkillInvSlotFilter", &Script_GetTradeSkillInvSlotFilter },
    { "TradeSkillOnlyShowMakeable", &Script_TradeSkillOnlyShowMakeable },
    { "TradeSkillOnlyShowSkillUps", &Script_TradeSkillOnlyShowSkillUps },
    { "CollapseTradeSkillSubClass", &Script_CollapseTradeSkillSubClass },
    { "ExpandTradeSkillSubClass", &Script_ExpandTradeSkillSubClass },
    { "GetFirstTradeSkill", &Script_GetFirstTradeSkill },
    { "GetTradeskillRepeatCount", &Script_GetTradeskillRepeatCount },
    { "DoTradeSkill", &Script_DoTradeSkill },
    { "GetTradeSkillRecipeLink", &Script_GetTradeSkillRecipeLink },
    { "StopTradeSkillRepeat", &Script_StopTradeSkillRepeat },
    { "GetTradeSkillListLink", &Script_GetTradeSkillListLink },
    { "IsTradeSkillLinked", &Script_IsTradeSkillLinked },
};

// VERIFY: c517a484 auto
// OFFSET: 0x005DA3A0
void TradeSkillRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 02805132 auto
// OFFSET: 0x005DA3D0
void TradeSkillUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
