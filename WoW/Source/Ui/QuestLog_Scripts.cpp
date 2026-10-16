#include "QuestLog.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include <ctime>
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/QuestCache.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/DB/Rec/CharTitlesRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/QuestLogLeaderBoardParams.h"
#include "WoW/Source/Ui/WorldMap.h"
#include <cmath>
#include "WoW/Source/DB/Rec/DungeonMapRec.h"
#include "Engine/Source/Tempest/C3Vector.h"

// VERIFY: ee052397 auto
// OFFSET: 0x005DF0D0
static int32_t Script_SetAbandonQuest(lua_State* L) {
    CGQuestLog::m_abandonQuest = CGQuestLog::m_selectedQuest;
    return 0;
}

// VERIFY: 4bbea619 auto
// OFFSET: 0x005DF910
static int32_t Script_GetNumQuestLogRewardFactions(lua_State* L) {
    lua_pushnumber(L, CGQuestInfo::m_numRewardFactions);
    return 1;
}

// VERIFY: 5504824c manual
// OFFSET: 0x005DF5D0
static int32_t Script_GetNumQuestWatches(lua_State* L) {
    lua_pushnumber(L, CGQuestLog::m_numQuestWatches);
    return 1;
}

// VERIFY: ac535154 auto
// OFFSET: 0x005DFB00
static int32_t Script_SetPOIIconOverlapDistance(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetPOIIconOverlapDistance(index)");
        return 0;
    }

    CGQuestLog::m_questIconOverlapDist = static_cast<float>(lua_tonumber(L, 1));
    return 0;
}

// VERIFY: 48abddb9 auto
// OFFSET: 0x005DFB40
static int32_t Script_SetPOIIconOverlapPushDistance(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetPOIIconOverlapPushDistance(index)");
        return 0;
    }

    float distance = static_cast<float>(lua_tonumber(L, 1));

    if (CGQuestLog::m_questIconOverlapDist <= distance) {
        CGQuestLog::m_questIconOverlapPushDist = distance;
    }

    return 0;
}

// VERIFY: 13e3138c auto
// OFFSET: 0x005E51A0
static int32_t Script_GetQuestGreenRange(lua_State* L) {
    lua_pushnumber(L, CGQuestLog::GetQuestGreenRange());
    return 1;
}

// VERIFY: 28d4ab3f manual
// OFFSET: 0x005DF010
static int32_t Script_GetNumQuestLogEntries(lua_State* L) {
    lua_pushnumber(L, CGQuestLog::m_numShownQuests);
    uint32_t quests = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(CGQuestLog::m_numQuest); i++) {
        if (static_cast<int32_t>(i) < 0 || static_cast<int32_t>(i) >= CGQuestLog::m_numQuest || !CGQuestLog::m_quests[i].m_isHeader) {
            quests++;
        }
    }

    lua_pushnumber(L, quests);
    return 2;
}

// VERIFY: 8f4872a2 manual
// OFFSET: 0x005DFC40
static int32_t Script_GetQuestWatchIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetQuestWatchIndex(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader) {
        return 0;
    }

    int32_t questId = CGQuestLog::m_quests[index].m_questId;

    if (!questId) {
        return 0;
    }

    for (uint32_t i = 0; i < CGQuestLog::m_numQuestWatches; i++) {
        if ((i < 25 ? CGQuestLog::m_questWatches[i].m_questId : 0) == questId) {
            lua_pushnumber(L, i + 1);
            return 1;
        }
    }

    return 0;
}

// VERIFY: 2381a775 manual
// OFFSET: 0x005DF610
static int32_t Script_IsQuestWatched(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsQuestWatched(index)");
        return 0;
    }

    int32_t watched = 0;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0 && index < CGQuestLog::m_numQuest && !CGQuestLog::m_quests[index].m_isHeader) {
        int32_t questId = CGQuestLog::m_quests[index].m_questId;

        if (questId && CGQuestLog::m_numQuestWatches) {
            for (uint32_t i = 0; i < CGQuestLog::m_numQuestWatches; i++) {
                if ((i < 25 ? CGQuestLog::m_questWatches[i].m_questId : 0) == questId) {
                    watched = 1;
                }
            }

            if (watched) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c6e716e7 manual
// OFFSET: 0x005DF780
static int32_t Script_GetQuestIndexForWatch(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetQuestIndexForWatch(index)");
        return 0;
    }

    uint32_t watch = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t questId;

    if (watch < 25 && (questId = CGQuestLog::m_questWatches[watch].m_questId) && CGQuestLog::m_numQuest) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(CGQuestLog::m_numQuest); i++) {
            int32_t id = static_cast<int32_t>(i) < 0 || static_cast<int32_t>(i) >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[i].m_isHeader ? 0 : CGQuestLog::m_quests[i].m_questId;

            if (id == questId) {
                lua_pushnumber(L, i + 1);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c32d8dd7 manual
// OFFSET: 0x005E02F0
static int32_t Script_SelectQuestLogEntry(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SelectQuestLogEntry(index)");
        return 0;
    }

    CGQuestLog::SetSelectedQuest(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: a201ed70 auto
// OFFSET: 0x005DF0A0
static int32_t Script_GetQuestLogSelection(lua_State* L) {
    lua_pushnumber(L, CGQuestLog::GetSelectedLogEntry() + 1);
    return 1;
}

// VERIFY: 9be26835 auto
// OFFSET: 0x005DF0E0
static int32_t Script_GetAbandonQuestName(lua_State* L) {
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_abandonQuest, WOWGUID(), nullptr, nullptr, false);

    if (quest) {
        lua_pushstring(L, quest->m_name);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9852e1b1 manual
// OFFSET: 0x005DF520
static int32_t Script_GetQuestLogPushable(lua_State* L) {
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1));
    int32_t questId = index ? CGQuestLog::GetQuestAt(index - 1) : CGQuestLog::m_selectedQuest;
    QuestCache* quest = g_questDBCache.GetRecord(questId, WOWGUID(), nullptr, nullptr, false);

    if (!quest || !(quest->m_flags & 8)) {
        return 0;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: b5bc01a2 manual
// OFFSET: 0x005E6240
static int32_t Script_GetQuestTimers(lua_State* L) {
    int32_t count = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    uint32_t numQuest = CGQuestLog::m_numQuest;
    int32_t timeOffset = CGQuestLog::m_timeOffset;
    int32_t failed = 0;

    if (!numQuest) {
        return 0;
    }

    for (int32_t i = 0; static_cast<uint32_t>(i) < numQuest; i++) {
        if (i < 0 || i >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[i].m_isHeader) {
            continue;
        }

        int32_t slot = CGQuestLog::m_quests[i].m_logSlot;

        if (slot < 0 || slot > 25) {
            continue;
        }

        PlayerQuestLogEntry* entry = &player->m_player->m_playerQuestLog[slot];

        if (!entry || !entry->m_time || (entry->m_state & 2) || (i < CGQuestLog::m_numQuest && CGQuestLog::m_quests[i].m_expired)) {
            continue;
        }

        int32_t left = timeOffset + entry->m_time - static_cast<int32_t>(time(nullptr)) - 1;

        if (left < 0) {
            if (CGQuestLog::SetQuestExpired(i)) {
                QuestCache* quest = g_questDBCache.GetRecord(entry->m_questId, WOWGUID(), DisplayQuestFailed, nullptr, false);

                if (quest) {
                    CGGameUI::DisplayError(ERR_QUEST_FAILED_S, quest->m_name);
                }

                failed = 1;
            }
        } else {
            count++;
            lua_checkstack(L, 1);
            lua_pushnumber(L, left);
        }
    }

    if (failed) {
        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
    }

    return count;
}

// VERIFY: 742e67e4 manual
// OFFSET: 0x005DF150
static int32_t Script_GetNumQuestLogRewards(lua_State* L) {
    uint32_t count = 0;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest) {
        uint32_t i;

        for (i = 0; i < 4; i++) {
            if (!quest->m_rewardItem[i]) {
                break;
            }
        }

        if (i < 4) {
            count = i;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: a8688e18 manual
// OFFSET: 0x005DF1D0
static int32_t Script_GetNumQuestLogChoices(lua_State* L) {
    uint32_t count = 0;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest) {
        uint32_t i;

        for (i = 0; i < 6; i++) {
            if (!quest->m_rewardChoiceItem[i]) {
                break;
            }
        }

        if (i <= 6) {
            count = i;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: ad68cc26 manual
// OFFSET: 0x005DF250
static int32_t Script_GetQuestLogItemLink(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        return luaL_error(L, "Usage: GetQuestLogItemLink(\"type\", index)");
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest && type && *type && index >= 0) {
        int32_t itemId = 0;
        bool valid = true;

        if (!SStrCmpI(type, "choice", STORM_MAX_STR) && index < 6) {
            itemId = quest->m_rewardChoiceItem[index];
        } else if (!SStrCmpI(type, "reward", STORM_MAX_STR) && index < 4) {
            itemId = quest->m_rewardItem[index];
        } else {
            valid = false;
        }

        if (valid && itemId) {
            ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

            if (stats) {
                lua_pushstring(L, CGTooltip::GetItemHyperlinkString(itemId, stats->m_quality, 0, 0, 0));
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 2e71d372 manual
// OFFSET: 0x005DF3A0
static int32_t Script_GetQuestLogRewardTalents(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);
    int32_t talents = 0;

    if (quest && static_cast<int32_t>(quest->m_rewardTalentPoints) >= 0) {
        talents = quest->m_rewardTalentPoints;
    }

    lua_pushinteger(L, talents);
    return 1;
}

// VERIFY: d812b140 manual
// OFFSET: 0x005DF400
static int32_t Script_GetQuestLogRewardArenaPoints(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);
    int32_t points = 0;

    if (quest && static_cast<int32_t>(quest->m_rewardArenaPoints) >= 0) {
        points = quest->m_rewardArenaPoints;
    }

    lua_pushinteger(L, points);
    return 1;
}

// VERIFY: 78d48f37 manual
// OFFSET: 0x005DF460
static int32_t Script_GetQuestLogRequiredMoney(lua_State* L) {
    int32_t questId;

    if (lua_isnumber(L, 1)) {
        int32_t index = static_cast<int32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1);
        questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
    } else {
        questId = CGQuestLog::m_selectedQuest;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

    if (quest && quest->m_rewardMoney < 0) {
        lua_pushnumber(L, -quest->m_rewardMoney);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: 635b5e85 auto
// OFFSET: 0x005DF870
static int32_t Script_GetQuestLogGroupNum(lua_State* L) {
    lua_pushnumber(L, GetQuestSuggestedPlayers(CGQuestLog::GetSelectedLogEntry()));
    return 1;
}

// VERIFY: 5cf345e1 manual
// OFFSET: 0x005DF940
static int32_t Script_GetQuestLogRewardFactionInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: Script_GetQuestLogRewardFactionInfo(index)");
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1) - 1.0);
    lua_pushnumber(L, index > 9 ? 0 : CGQuestInfo::m_rewardFactionList[index]);
    lua_pushnumber(L, index > 9 ? 0 : CGQuestInfo::m_rewardFactionAmounts[index]);
    return 2;
}

// VERIFY: 08ede353 auto
// OFFSET: 0x005E0340
static int32_t Script_GetQuestLogQuestText(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, QuestQueryCallback, nullptr, false);

    if (!quest) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    char objectives[3000];
    char description[3000];
    WOWGUID player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(quest->m_description, description, sizeof(description), player, nullptr, 0);
    player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(quest->m_objText, objectives, sizeof(objectives), player, nullptr, 0);
    lua_pushstring(L, description);
    lua_pushstring(L, objectives);
    return 2;
}

// VERIFY: c7dba3cb manual
// OFFSET: 0x005E06D0
static int32_t Script_GetQuestLogCompletionText(lua_State* L) {
    int32_t questId;

    if (lua_isnumber(L, 1)) {
        int32_t index = static_cast<int32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1);
        questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
    } else {
        questId = CGQuestLog::m_selectedQuest;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

    if (!quest) {
        return 0;
    }

    char text[3000];

    if ((quest->m_flags & 0x40000) || !quest->m_completionText[0]) {
        WOWGUID player = ClntObjMgrGetActivePlayer();
        QuestParserParseText(quest->m_objText, text, sizeof(text), player, nullptr, 0);
    } else {
        WOWGUID player = ClntObjMgrGetActivePlayer();
        QuestParserParseText(quest->m_completionText, text, sizeof(text), player, nullptr, 0);
    }

    lua_pushstring(L, text);
    return 1;
}

// VERIFY: ea272adb manual
// OFFSET: 0x005E3D60
static int32_t Script_GetAbandonQuestItems(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    uint32_t count = 0;

    if (!player) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* abandon = g_questDBCache.GetRecord(CGQuestLog::m_abandonQuest, none, nullptr, nullptr, false);

    if (!abandon) {
        return 0;
    }

    int32_t items[11] = { 0 };

    for (uint32_t i = 0; i < 6; i++) {
        int32_t itemId = abandon->m_collectItemId[i];

        if (itemId > 0 && player->m_bag.GetItemTypeCount(itemId, 8) > 0) {
            ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

            if (stats && stats->m_bonding == 4) {
                uint32_t j;

                for (j = 0; j < count && items[j] != itemId; j++) {
                }

                if (j >= count) {
                    items[count++] = itemId;
                }
            }
        }
    }

    int32_t startingItem = abandon->m_startingItemId;

    if (startingItem > 0 && player->m_bag.GetItemTypeCount(startingItem, 8) > 0) {
        ItemStats_C* stats = g_itemDBCache.GetRecord(startingItem, none, nullptr, nullptr, false);

        if (stats && stats->m_bonding == 4) {
            uint32_t j;

            for (j = 0; j < count && items[j] != startingItem; j++) {
            }

            if (j >= count) {
                items[count++] = startingItem;
            }
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        int32_t itemId = abandon->m_intermediateItemId[i];

        if (itemId > 0 && player->m_bag.GetItemTypeCount(itemId, 8) > 0) {
            uint32_t j;

            for (j = 0; j < count && items[j] != itemId; j++) {
            }

            if (j >= count) {
                items[count++] = itemId;
            }
        }
    }

    uint32_t total = count;

    for (int32_t i = 0; i < CGQuestLog::m_numQuest; i++) {
        if (!count) {
            return 0;
        }

        int32_t questId = i < 0 || i >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[i].m_isHeader ? 0 : CGQuestLog::m_quests[i].m_questId;
        QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

        if (!quest || quest == abandon) {
            continue;
        }

        for (uint32_t j = 0; j < 6; j++) {
            int32_t itemId = quest->m_collectItemId[j];

            for (uint32_t k = 0; k < 11; k++) {
                if (itemId > 0 && itemId == items[k]) {
                    items[k] = 0;
                    count--;
                }
            }
        }

        int32_t startingId = quest->m_startingItemId;

        for (uint32_t k = 0; k < 11; k++) {
            if (startingId > 0 && startingId == items[k]) {
                items[k] = 0;
                count--;
            }
        }

        total = count;
    }

    if (!count) {
        return 0;
    }

    bool first = true;

    for (uint32_t k = 0; k < 11; k++) {
        if (!items[k]) {
            continue;
        }

        ItemStats_C* stats = g_itemDBCache.GetRecord(items[k], none, nullptr, nullptr, false);

        if (first) {
            first = false;
        } else {
            lua_pushstring(L, ", ");
        }

        lua_pushstring(L, stats->m_name[0]);
    }

    if (total > 1) {
        lua_concat(L, 2 * total - 1);
    }

    return 1;
}

// VERIFY: f2ea8d44 auto
// OFFSET: 0x005E4060
static int32_t Script_AbandonQuest(lua_State* L) {
    CGQuestLog::AbandonSelectedQuest();
    return 0;
}

// VERIFY: 84435f71 manual
// OFFSET: 0x005E41A0
static int32_t Script_GetNumQuestLeaderBoards(lua_State* L) {
    int32_t count = 0;

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        int32_t questId;

        if (lua_isnumber(L, 1)) {
            int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
            questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
        } else {
            questId = CGQuestLog::m_selectedQuest;
        }

        count = GetNumQuestLeaderBoards(questId, QuestQueryCallback, nullptr);
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: a91ae0a7 manual
// OFFSET: 0x005E4260
static int32_t Script_GetNumQuestItemDrops(lua_State* L) {
    int32_t count = 0;

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        int32_t questId;

        if (lua_isnumber(L, 1)) {
            int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
            questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
        } else {
            questId = CGQuestLog::m_selectedQuest;
        }

        count = CountQuestItemDropsNeeded(questId, QuestQueryCallback, nullptr);
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: ae5c6641 manual
// OFFSET: 0x005E4490
static int32_t Script_GetQuestLogRewardInfo(lua_State* L) {
    // STATIC: 0x00C23FB0
    static char s_texture[260];

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLogRewardInfo(index)");
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);
    ItemStats_C* stats;

    if (!quest || index > 3 || !(stats = g_itemDBCache.GetRecord(quest->m_rewardItem[index], none, ItemQueryCallback, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        return 5;
    }

    const char* path = ClientDBStringLookup(3);
    SStrPrintf(s_texture, sizeof(s_texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, stats->GetName(0));
    lua_pushstring(L, s_texture);
    lua_pushnumber(L, quest->m_rewardItemCount[index]);
    lua_pushnumber(L, stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t error;

    if (!player || player->CanUseItem(stats, error)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 5;
}

// VERIFY: 7d5ae3b6 manual
// OFFSET: 0x005E4660
static int32_t Script_GetQuestLogChoiceInfo(lua_State* L) {
    // STATIC: 0x00C240B8
    static char s_texture[260];

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLogRewardInfo(index)");
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);
    ItemStats_C* stats;

    if (!quest || index > 5 || !(stats = g_itemDBCache.GetRecord(quest->m_rewardChoiceItem[index], none, ItemQueryCallback, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        return 5;
    }

    const char* path = ClientDBStringLookup(3);
    SStrPrintf(s_texture, sizeof(s_texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, stats->GetName(0));
    lua_pushstring(L, s_texture);
    lua_pushnumber(L, quest->m_rewardChoiceItemCount[index]);
    lua_pushnumber(L, stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t error;

    if (!player || player->CanUseItem(stats, error)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 5;
}

// VERIFY: ead48f31 manual
// OFFSET: 0x005E48F0
static int32_t Script_GetQuestLogRewardMoney(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    bool maxLevel = player && static_cast<int32_t>(player->m_unit->m_unitFieldLevel) >= static_cast<int32_t>(player->m_player->m_playerFieldMaxLevel);
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);
    int32_t money = 0;

    if (quest) {
        if (quest->m_rewardMoney > 0) {
            money = quest->m_rewardMoney;
        }

        if (maxLevel && money <= static_cast<int32_t>(quest->m_rewardMoneyInsteadOfXp)) {
            money = quest->m_rewardMoneyInsteadOfXp;
        }
    }

    lua_pushnumber(L, money);
    return 1;
}

// VERIFY: f994e29c manual
// OFFSET: 0x005E4B70
static int32_t Script_GetQuestLogRewardHonor(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    int32_t honor = 0;

    if (player) {
        WOWGUID none = { 0 };
        QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

        if (quest && (static_cast<int32_t>(quest->m_rewardHonor) > 0 || quest->m_rewardHonorBonus > 0.0f)) {
            float bonus = player->CalcRewardHonor(quest->m_rewardHonorBonus) * 0.1f;
            honor = static_cast<int32_t>(bonus) + quest->m_rewardHonor;
        }
    }

    lua_pushinteger(L, honor);
    return 1;
}

// VERIFY: 08bf4d5d auto
// OFFSET: 0x005E4D10
static int32_t Script_GetQuestLogRewardSpell(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (!quest || !quest->m_rewardSpellId) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 4;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(quest->m_rewardSpellId, &spell)) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 4;
    }

    SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_spellIconID);

    if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, spell.m_name);

    if (spell.m_attributes & 0x20) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    int32_t learns = 0;

    if (quest->m_effectOnPlayer) {
        SpellRec effect;

        if (g_spellDB.GetRecord(quest->m_effectOnPlayer, &effect)) {
            for (uint32_t i = 0; i < 3; i++) {
                if (effect.m_effect[i] == 36 || effect.m_effect[i] == 57) {
                    learns = 1;
                    break;
                }
            }
        }
    }

    if (learns) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: 2418c7b5 manual
// OFFSET: 0x005E4FB0
static int32_t Script_GetQuestIndexForTimer(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestIndexForTimer(index)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t found = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        uint32_t numQuest = CGQuestLog::m_numQuest;
        int32_t timeOffset = CGQuestLog::m_timeOffset;

        for (int32_t i = 0; static_cast<uint32_t>(i) < numQuest; i++) {
            if (i < 0 || i >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[i].m_isHeader) {
                continue;
            }

            int32_t slot = CGQuestLog::m_quests[i].m_logSlot;

            if (slot < 0 || slot > 25) {
                continue;
            }

            PlayerQuestLogEntry* entry = &player->m_player->m_playerQuestLog[slot];

            if (!entry || !entry->m_time || (entry->m_state & 2) || timeOffset - static_cast<int32_t>(time(nullptr)) + static_cast<int32_t>(entry->m_time) - 1 < 0) {
                continue;
            }

            if (found == index) {
                lua_pushnumber(L, i + 1);
                return 1;
            }

            found++;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: ddfc0452 auto
// OFFSET: 0x005E5100
static int32_t Script_CollapseQuestHeader(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: CollapseQuestHeader(index)");
    }

    CGQuestLog::CollapseHeader(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, 1);
    return 0;
}

// VERIFY: b84341f7 auto
// OFFSET: 0x005E5150
static int32_t Script_ExpandQuestHeader(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: ExpandQuestHeader(index)");
    }

    CGQuestLog::CollapseHeader(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, 0);
    return 0;
}

// VERIFY: 4ae2b39c manual
// OFFSET: 0x005E51D0
static int32_t Script_GetQuestLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLink(questID)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t questId;
    QuestCache* quest;
    WOWGUID none = { 0 };

    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader || !(questId = CGQuestLog::m_quests[index].m_questId) || !(quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false))) {
        lua_pushnil(L);
        return 1;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    int32_t level = player && quest->m_level == 0xFFFFFFFF ? player->m_unit->m_unitFieldLevel : quest->m_level;
    lua_pushstring(L, CGTooltip::GetQuestLink(questId, level));
    return 1;
}

// VERIFY: 661e57e1 manual
// OFFSET: 0x005E67C0
static int32_t Script_AddQuestWatch(lua_State* L) {
    int32_t questId;

    if (lua_isnumber(L, 1)) {
        int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
        questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
    } else {
        questId = CGQuestLog::m_selectedQuest;
    }

    if (questId) {
        int32_t duration = 0;

        if (lua_isnumber(L, 2)) {
            duration = static_cast<int32_t>(lua_tonumber(L, 2));
        }

        CGQuestLog::AddQuestWatch(questId, duration);
    }

    return 0;
}

// VERIFY: 615c22ea manual
// OFFSET: 0x005DF700
static int32_t Script_RemoveQuestWatch(lua_State* L) {
    int32_t questId;

    if (lua_isnumber(L, 1)) {
        int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
        questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
    } else {
        questId = CGQuestLog::m_selectedQuest;
    }

    if (questId) {
        CGQuestLog::RemoveQuestWatch(questId);
    }

    return 0;
}

// VERIFY: d06a365e auto
// OFFSET: 0x005DF8A0
static int32_t Script_ProcessQuestLogRewardFactions(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest) {
        for (int32_t i = 0; i < 5; i++) {
            CGQuestInfo::InitRewRepFields(i, quest->m_rewardReputationFaction[i], quest->m_factionRewardId[i], quest->m_rewardReputationOverride[i]);
        }

        CGQuestInfo::InitRewRepShowMask(quest->m_rewardReputationMask);
        CGQuestInfo::ProcessQuestLogRewardFactions();
    }

    return 0;
}

// VERIFY: ffc6b635 auto
// OFFSET: 0x005DFA10
static int32_t Script_GetQuestSortIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestSortIndex(questIndex)");
    }

    lua_pushnumber(L, GetQuestSortId(static_cast<int32_t>(lua_tonumber(L, 1)) - 1) + 1);
    return 1;
}

// VERIFY: bf9ab07f manual
// OFFSET: 0x005DFA80
static int32_t Script_QuestPOIGetQuestIDByIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: QuestPOIGetQuestIDByIndex(index)");
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index > 24) {
        return 0;
    }

    int32_t questId = CGQuestLog::m_questPOIs[index].m_questId;

    if (questId < 0) {
        return 0;
    }

    lua_pushnumber(L, questId);
    return 1;
}

// VERIFY: 3f76db08 manual
// OFFSET: 0x005DFBA0
static int32_t Script_ShiftQuestWatches(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        return luaL_error(L, "Usage: ShiftQuestWatches(indexFrom, indexTo)");
    }

    uint32_t to = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t from = static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1);
    CGQuestLog::ShiftQuestWatches(from, to);
    return 0;
}

// VERIFY: 44c065ff manual
// OFFSET: 0x005E4320
static int32_t Script_GetQuestLogTimeLeft(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        int32_t index = CGQuestLog::GetSelectedLogEntry();
        uint32_t slot = index < 0 ? 0 : CGQuestLog::m_quests[index].m_logSlot;

        if (slot <= 25) {
            PlayerQuestLogEntry* entry = &player->m_player->m_playerQuestLog[slot];
            int32_t timeOffset = CGQuestLog::m_timeOffset;

            if (entry && entry->m_time && !(entry->m_state & 2) && timeOffset) {
                int32_t left = timeOffset + static_cast<int32_t>(entry->m_time) - static_cast<int32_t>(time(nullptr)) - 1;

                if (left < 0) {
                    left = 0;
                }

                lua_pushnumber(L, left);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: b1296081 manual
// OFFSET: 0x005E43F0
static int32_t Script_IsCurrentQuestFailed(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        int32_t index = CGQuestLog::GetSelectedLogEntry();
        uint32_t slot = index < 0 ? 0 : CGQuestLog::m_quests[index].m_logSlot;

        if (slot <= 25) {
            PlayerQuestLogEntry* entry = &player->m_player->m_playerQuestLog[slot];

            if (entry && ((entry->m_state & 2) || CGQuestLog::IsSelectedQuestExpired())) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 1b10b958 manual
// OFFSET: 0x005E4070
static int32_t Script_IsUnitOnQuest(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        return luaL_error(L, "Usage: IsUnitOnQuest(index, \"unit\")");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t questId;
    CGPlayer_C* member;
    WOWGUID guid;

    if (index >= 0 && index < CGQuestLog::m_numQuest && !CGQuestLog::m_quests[index].m_isHeader && (questId = CGQuestLog::m_quests[index].m_questId) && (guid = GetGuidByKeyword(lua_tolstring(L, 2, nullptr)), CGPartyInfo::IsMember(guid)) && (member = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__))) && !(member->m_modelFlags & 0x10000)) {
        for (uint32_t slot = 0; slot < 25; slot++) {
            PlayerQuestLogEntry* entry = member->GetQuestLogEntry(slot);

            if (entry && static_cast<int32_t>(entry->m_questId) == questId) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 6fb51c50 auto
// OFFSET: 0x005E4840
static int32_t Script_GetQuestLogSpellLink(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest && quest->m_rewardSpellId) {
        SpellRec spell;

        if (g_spellDB.GetRecord(quest->m_rewardSpellId, &spell)) {
            lua_pushstring(L, CGTooltip::GetSpellLink(quest->m_rewardSpellId));
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 8200413a manual
// OFFSET: 0x005E49B0
static int32_t Script_GetQuestLogRewardXP(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    int32_t questId = CGQuestLog::m_selectedQuest;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);
    int32_t xp = 0;

    if (player && quest) {
        int32_t level = player->m_unit->m_unitFieldLevel;

        if (level < static_cast<int32_t>(player->m_player->m_playerFieldMaxLevel)) {
            xp = CalculateExperience(questId, level);

            for (uint32_t i = 0; i < 5; i++) {
                WOWGUID guid = GetPartyMemberGUID(i);
                CGPlayer_C* member = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

                if (!member || !player->IsRAFPlayer(guid) || !(player->GetDistanceToPos(member->GetPosition()) < 10000.0f)) {
                    continue;
                }

                int32_t questLevel = quest->m_level;

                if (questLevel != -1) {
                    int32_t playerLevel = player->m_unit->m_unitFieldLevel;

                    if (playerLevel > questLevel + player->GetQuestGreenRange()) {
                        continue;
                    }

                    int32_t memberLevel = member->m_unit->m_unitFieldLevel;

                    if (memberLevel > questLevel + member->GetQuestGreenRange() || playerLevel < static_cast<int32_t>(quest->m_requiredLevel) || memberLevel < static_cast<int32_t>(quest->m_requiredLevel)) {
                        continue;
                    }
                }

                if (static_cast<int32_t>(player->m_unit->m_unitFieldLevel) < 60) {
                    xp *= 3;
                }
            }

            xp = static_cast<int32_t>(player->GetQuestXPMultiplier() * xp);
        }
    }

    lua_pushnumber(L, xp);
    return 1;
}

// VERIFY: ef8d1cce manual
// OFFSET: 0x005E4C10
static int32_t Script_GetQuestLogRewardTitle(lua_State* L) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);
    int32_t titleId = quest ? quest->m_rewardTitleId : 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    CharTitlesRec* title = g_charTitlesDB.GetRecord(titleId);

    if (!title) {
        lua_pushnil(L);
        return 1;
    }

    int32_t gender;
    const char* name;

    if (player) {
        gender = player->m_player->m_playerBytes3[0];
        name = player->GetUnitName(nullptr, 1);
    } else {
        gender = 0;
        name = "";
    }

    const char* format = title->m_name1;

    if (!*format || (gender != 1 && *title->m_name)) {
        format = title->m_name;
    }

    char buffer[256];
    SStrPrintf(buffer, sizeof(buffer), format, name);
    lua_pushstring(L, buffer);
    return 1;
}

// VERIFY: a36436a8 manual
// OFFSET: 0x005E4ED0
static int32_t Script_QuestLogPushQuest(lua_State* L) {
    int64_t value = static_cast<int64_t>(lua_tonumber(L, 1));
    int32_t questId;

    if (value) {
        int32_t index = static_cast<int32_t>(value - 1);
        questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
    } else {
        questId = CGQuestLog::m_selectedQuest;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

    if (player && quest && (quest->m_flags & 0x8) && (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers)) {
        CGPlayer_C::PushQuestToParty(questId);
    }

    return 0;
}

// VERIFY: 7dbc9ebd manual
// OFFSET: 0x005E52D0
static int32_t Script_GetQuestLogSpecialItemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLogSpecialItemInfo(index)");
    }

    CGItem_C* item = CGQuestLog::GetQuestSpecialItem(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1));

    if (!item) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
    const char* path = ClientDBStringLookup(3);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
    lua_pushstring(L, texture);
    lua_pushnumber(L, item->GetCurrentSpellCharges());
    return 3;
}

// VERIFY: 32fe8f32 manual
// OFFSET: 0x005E53D0
static int32_t Script_GetQuestLogSpecialItemCooldown(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLogSpecialItemCooldown(index)");
    }

    CGItem_C* item = CGQuestLog::GetQuestSpecialItem(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1));

    if (!item) {
        return 0;
    }

    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;
    Spell_C_GetItemCooldown(item, &duration, &start, &enable);
    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: 19155561 manual
// OFFSET: 0x005E54C0
static int32_t Script_IsQuestLogSpecialItemInRange(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: IsQuestLogSpecialItemInRange(index [,target])");
    }

    CGItem_C* item = CGQuestLog::GetQuestSpecialItem(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1))) - 1);
    int32_t spellId = item ? item->GetEnchantmentSpell(0) : 0;
    SpellRec spell;
    CGPlayer_C* player;
    WOWGUID target;
    uint8_t inRange;

    if (g_spellDB.GetRecord(spellId, &spell) && (player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__))) && Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true) && Spell_C_RangeCheckSelected(player, &spell, target, &inRange, 0, 0)) {
        lua_pushnumber(L, inRange ? 1.0 : 0.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 803edb52 manual
// OFFSET: 0x005E5640
static int32_t Script_UseQuestLogSpecialItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: UseQuestLogSpecialItem(index)");
    }

    CGItem_C* item = CGQuestLog::GetQuestSpecialItem(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1)) - 1));

    if (item) {
        WOWGUID target = { 0 };
        item->UseItem(&target, 0);
    }

    return 0;
}

// VERIFY: 07c6118c auto
// OFFSET: 0x005E56C0
static int32_t Script_GetQuestWorldMapAreaID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestWorldMapAreaID(questID)");
    }

    int32_t floor = 0;
    int32_t area = CGQuestLog::GetQuestWorldMapAreaID(static_cast<int32_t>(lua_tonumber(L, 1)), &floor);
    lua_pushnumber(L, area);
    lua_pushnumber(L, floor);
    return 2;
}

// VERIFY: d56d7966 auto
// OFFSET: 0x005E5740
static int32_t Script_QuestPOIUpdateIcons(lua_State* L) {
    CGQuestLog::UpdatePOIIcons();
    return 0;
}

// VERIFY: d4a34e6d manual
// OFFSET: 0x005E5750
static int32_t Script_QuestPOIGetQuestIDByVisibleIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: Script_QuestPOIGetQuestIDByVisibleIndex(index)");
    }

    int32_t questId = CGQuestLog::GetPOIQuestIDByVisibleIndex(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);

    if (questId < 0) {
        return 0;
    }

    lua_pushnumber(L, questId);
    lua_pushnumber(L, CGQuestLog::GetQuestIndex(questId));
    return 2;
}

// VERIFY: 75a8ac68 manual
// OFFSET: 0x005E5CC0
static int32_t Script_GetQuestLogTitle(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLogTitle(index)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    lua_pushstring(L, GetQuestTitle(index));
    int32_t level = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : GetQuestLevel(CGQuestLog::m_quests[index].m_questId);
    lua_pushnumber(L, level);
    lua_pushstring(L, GetQuestTag(index));
    lua_pushnumber(L, GetQuestSuggestedPlayers(index));

    if (index >= 0 && index < CGQuestLog::m_numQuest && CGQuestLog::m_quests[index].m_isHeader) {
        lua_pushnumber(L, 1.0);
        uint32_t sortId = GetQuestSortId(index);

        if (sortId >= 25 || ((1 << sortId) & s_cvQuestLogCollapseFilter->m_intValue)) {
            lua_pushnil(L);
        } else {
            lua_pushnumber(L, 1.0);
        }
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    bool failed = false;

    if (player) {
        if (index >= 0 && index < CGQuestLog::m_numQuest && !CGQuestLog::m_quests[index].m_isHeader) {
            uint32_t slot = CGQuestLog::m_quests[index].m_logSlot;

            if (slot <= 25) {
                PlayerQuestLogEntry* entry = &player->m_player->m_playerQuestLog[slot];

                if (entry && (entry->m_state & 2)) {
                    failed = true;
                }
            }
        }

        if (!failed && index < CGQuestLog::m_numQuest && !CGQuestLog::m_quests[index].m_isHeader && CGQuestLog::m_quests[index].m_expired) {
            failed = true;
        }

        if (failed) {
            lua_pushnumber(L, -1.0);
        } else if (CGQuestLog::IsQuestCompleteEx(index, 1)) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnil(L);
    }

    if (IsDailyQuest(index)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    int32_t questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;
    lua_pushnumber(L, questId);
    CVar* displayQuestId = CVar::Lookup("displayQuestID");

    if (!displayQuestId || !displayQuestId->m_intValue) {
        return 9;
    }

    lua_pushboolean(L, 1);
    return 10;
}

// VERIFY: d25e98b1 auto
// OFFSET: 0x005E64B0
static int32_t Script_SortQuestWatches(lua_State* L) {
    lua_pushboolean(L, CGQuestLog::SortQuestWatches());
    return 1;
}

// VERIFY: 513c8836 auto
// OFFSET: 0x005E6DE0
static int32_t Script_GetQuestResetTime(lua_State* L) {
    int32_t now = static_cast<int32_t>(time(nullptr));

    if (CGQuestLog::m_nextQuestUpdate && now >= CGQuestLog::m_nextQuestUpdate) {
        CGQuestLog::Update(1);
    }

    lua_pushnumber(L, CGQuestLog::m_questResetTime - now);
    return 1;
}

// VERIFY: 78e36809 manual
// OFFSET: 0x005E5F60
static int32_t Script_GetQuestLogLeaderBoard(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Usage: GetQuestLogLeaderBoard(index)");
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    QuestLogLeaderBoardParams params;
    params.m_index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    params.m_requireLogEntry = 1;
    int32_t questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;

    if (!CGQuestLog::GetQuestLeaderBoardByID(questId, &params)) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    lua_pushstring(L, params.m_text);
    lua_pushstring(L, params.m_type);

    if (params.m_complete) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: 6f8ab4c9 manual
// OFFSET: 0x005E63D0
static int32_t Script_QuestMapUpdateAllQuests(lua_State* L) {
    int32_t areaId;

    if (CGWorldMap::m_currentContinent < 0) {
        areaId = CGWorldMap::m_currentOrphanZone;
    } else if (CGWorldMap::m_currentZone < 0) {
        areaId = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent].m_areaID;
    } else {
        areaId = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent].m_zones.m_data[CGWorldMap::m_currentZone];
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(areaId);
    int32_t dungeonMapId = CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap;
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);
    int32_t updated = 0;

    if ((area && (CGWorldMap::m_currentZone != -1 || CGWorldMap::m_currentContinent == -2)) || dungeonMap) {
        updated = CGQuestLog::UpdateAllQuestPOI();
    }

    lua_pushnumber(L, updated);
    return 1;
}

// VERIFY: 39212d82 auto
// OFFSET: 0x005E0590
static int32_t Script_QuestPOIGetIconInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: QuestPOIGetIconInfo(questID)");
        return 0;
    }

    bool complete;
    int32_t mapId;
    uint32_t mapFloor;
    int32_t iconX;
    int32_t iconY;
    int32_t objective;

    if (!CGQuestLog::GetQuestPOIIcon(static_cast<int32_t>(lua_tonumber(L, 1)), &complete, &mapId, &mapFloor, &iconX, &iconY, &objective)) {
        return 0;
    }

    C3Vector position;
    position.x = iconX;
    position.y = iconY;
    position.z = 0.0f;
    float x;
    float y;
    int32_t dungeonMapId = CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap;
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);
    bool outland;

    if (!CGWorldMap::GetWorldPosition(position, mapId, &y, &x, &outland, false, dungeonMap ? dungeonMap->m_id : 0) || outland || fabs(fabs(y * x)) < 0.001) {
        return 0;
    }

    lua_pushboolean(L, complete);
    lua_pushnumber(L, y);
    lua_pushnumber(L, x);
    lua_pushinteger(L, objective);
    return 4;
}

// VERIFY: d6f69411 manual
// OFFSET: 0x005E60D0
static int32_t Script_GetQuestLogItemDrop(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetQuestLogItemDrop(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    QuestLogLeaderBoardParams params;
    params.m_index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    params.m_requireLogEntry = 1;
    int32_t questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;

    if (!CGQuestLog::GetQuestLogItemDropByID(questId, &params)) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    lua_pushstring(L, params.m_text);
    lua_pushstring(L, params.m_type);

    if (params.m_complete) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: ba2f887a manual
// OFFSET: 0x005E6650
static int32_t Script_GetQuestPOILeaderBoard(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetQuestLogLeaderBoard(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    QuestLogLeaderBoardParams params;
    params.m_index = static_cast<int32_t>(lua_tonumber(L, 1));
    params.m_requireLogEntry = 1;
    int32_t questId = index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader ? 0 : CGQuestLog::m_quests[index].m_questId;

    if (!CGQuestLog::GetQuestPOILeaderBoardByID(questId, &params)) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    lua_pushstring(L, params.m_text);
    lua_pushstring(L, params.m_type);

    if (params.m_complete) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 3;
}

// VERIFY: e6195e77 auto
// OFFSET: 0x008E5250
static int32_t Script_QuestPOIUpdateTexture(lua_State* L) {
    return 0;
}

// STATIC: 0x00AD1020
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumQuestLogEntries", &Script_GetNumQuestLogEntries },
    { "GetQuestLogTitle", &Script_GetQuestLogTitle },
    { "SelectQuestLogEntry", &Script_SelectQuestLogEntry },
    { "GetQuestLogSelection", &Script_GetQuestLogSelection },
    { "SetAbandonQuest", &Script_SetAbandonQuest },
    { "GetAbandonQuestName", &Script_GetAbandonQuestName },
    { "GetAbandonQuestItems", &Script_GetAbandonQuestItems },
    { "AbandonQuest", &Script_AbandonQuest },
    { "IsUnitOnQuest", &Script_IsUnitOnQuest },
    { "GetQuestLogQuestText", &Script_GetQuestLogQuestText },
    { "GetNumQuestLeaderBoards", &Script_GetNumQuestLeaderBoards },
    { "GetQuestLogLeaderBoard", &Script_GetQuestLogLeaderBoard },
    { "GetNumQuestItemDrops", &Script_GetNumQuestItemDrops },
    { "GetQuestLogItemDrop", &Script_GetQuestLogItemDrop },
    { "GetQuestPOILeaderBoard", &Script_GetQuestPOILeaderBoard },
    { "GetQuestLogTimeLeft", &Script_GetQuestLogTimeLeft },
    { "IsCurrentQuestFailed", &Script_IsCurrentQuestFailed },
    { "GetNumQuestLogRewards", &Script_GetNumQuestLogRewards },
    { "GetNumQuestLogChoices", &Script_GetNumQuestLogChoices },
    { "GetQuestLogRewardInfo", &Script_GetQuestLogRewardInfo },
    { "GetQuestLogChoiceInfo", &Script_GetQuestLogChoiceInfo },
    { "GetQuestLogItemLink", &Script_GetQuestLogItemLink },
    { "GetQuestLogSpellLink", &Script_GetQuestLogSpellLink },
    { "GetQuestLogRewardMoney", &Script_GetQuestLogRewardMoney },
    { "GetQuestLogRewardXP", &Script_GetQuestLogRewardXP },
    { "GetQuestLogRewardHonor", &Script_GetQuestLogRewardHonor },
    { "GetQuestLogRewardSpell", &Script_GetQuestLogRewardSpell },
    { "GetQuestLogRequiredMoney", &Script_GetQuestLogRequiredMoney },
    { "GetQuestLogPushable", &Script_GetQuestLogPushable },
    { "QuestLogPushQuest", &Script_QuestLogPushQuest },
    { "GetQuestTimers", &Script_GetQuestTimers },
    { "GetQuestIndexForTimer", &Script_GetQuestIndexForTimer },
    { "CollapseQuestHeader", &Script_CollapseQuestHeader },
    { "ExpandQuestHeader", &Script_ExpandQuestHeader },
    { "GetQuestGreenRange", &Script_GetQuestGreenRange },
    { "GetNumQuestWatches", &Script_GetNumQuestWatches },
    { "IsQuestWatched", &Script_IsQuestWatched },
    { "AddQuestWatch", &Script_AddQuestWatch },
    { "RemoveQuestWatch", &Script_RemoveQuestWatch },
    { "GetQuestIndexForWatch", &Script_GetQuestIndexForWatch },
    { "GetQuestLogGroupNum", &Script_GetQuestLogGroupNum },
    { "GetQuestResetTime", &Script_GetQuestResetTime },
    { "GetQuestLink", &Script_GetQuestLink },
    { "GetQuestLogRewardTitle", &Script_GetQuestLogRewardTitle },
    { "GetQuestLogRewardTalents", &Script_GetQuestLogRewardTalents },
    { "GetQuestLogRewardArenaPoints", &Script_GetQuestLogRewardArenaPoints },
    { "GetQuestLogSpecialItemInfo", &Script_GetQuestLogSpecialItemInfo },
    { "GetQuestLogSpecialItemCooldown", &Script_GetQuestLogSpecialItemCooldown },
    { "IsQuestLogSpecialItemInRange", &Script_IsQuestLogSpecialItemInRange },
    { "UseQuestLogSpecialItem", &Script_UseQuestLogSpecialItem },
    { "ProcessQuestLogRewardFactions", &Script_ProcessQuestLogRewardFactions },
    { "GetNumQuestLogRewardFactions", &Script_GetNumQuestLogRewardFactions },
    { "GetQuestLogRewardFactionInfo", &Script_GetQuestLogRewardFactionInfo },
    { "SortQuestWatches", &Script_SortQuestWatches },
    { "ShiftQuestWatches", &Script_ShiftQuestWatches },
    { "GetQuestWatchIndex", &Script_GetQuestWatchIndex },
    { "QuestMapUpdateAllQuests", &Script_QuestMapUpdateAllQuests },
    { "GetQuestSortIndex", &Script_GetQuestSortIndex },
    { "GetQuestWorldMapAreaID", &Script_GetQuestWorldMapAreaID },
    { "QuestPOIUpdateTexture", &Script_QuestPOIUpdateTexture },
    { "QuestPOIUpdateIcons", &Script_QuestPOIUpdateIcons },
    { "QuestPOIGetIconInfo", &Script_QuestPOIGetIconInfo },
    { "QuestPOIGetQuestIDByIndex", &Script_QuestPOIGetQuestIDByIndex },
    { "QuestPOIGetQuestIDByVisibleIndex", &Script_QuestPOIGetQuestIDByVisibleIndex },
    { "GetQuestLogCompletionText", &Script_GetQuestLogCompletionText },
    { "SetPOIIconOverlapDistance", &Script_SetPOIIconOverlapDistance },
    { "SetPOIIconOverlapPushDistance", &Script_SetPOIIconOverlapPushDistance },
};

// VERIFY: 3feca0f3 auto
// OFFSET: 0x005DFD20
void QuestLogRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 8c72da83 auto
// OFFSET: 0x005DFD50
void QuestLogUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
