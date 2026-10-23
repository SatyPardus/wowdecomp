#include "AchievementInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AchievementRec.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/DB/Rec/Achievement_CategoryRec.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/DB/Rec/Achievement_CriteriaRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Common/WowTime.h"

// VERIFY: e0dcaa80 auto
// OFFSET: 0x005B0CE0
static int32_t Script_CanShowAchievementUI(lua_State* L) {
    lua_pushboolean(L, CGAchievementInfo::m_showAchievementUI);
    return 1;
}

// VERIFY: 2eadd0b6 manual
// OFFSET: 0x005B1BB0
static int32_t Script_QueryQuestsCompleted(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUERY_QUESTS_COMPLETED));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 04aea30d manual
// OFFSET: 0x005B1690
static int32_t Script_GetPreviousAchievement(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPreviousAchievement(achievementID)");
        return 0;
    }

    AchievementRec* rec = g_achievementDB.GetRecord(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (!rec || !rec->m_supercedes) {
        return 0;
    }

    lua_pushnumber(L, rec->m_supercedes);
    return 1;
}

// VERIFY: 1f10113f auto
// OFFSET: 0x005B18E0
static int32_t Script_GetAchievementCategory(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementCategory(achievementID)");
        return 0;
    }

    AchievementRec* rec = g_achievementDB.GetRecord(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (!rec || !rec->m_category || (rec->m_flags & 2)) {
        return 0;
    }

    lua_pushnumber(L, rec->m_category);
    return 1;
}

// VERIFY: fd1ca539 auto
// OFFSET: 0x005B1970
static int32_t Script_GetAchievementNumRewards(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementNumRewards(achievementID)");
        return 0;
    }

    AchievementRec* rec = g_achievementDB.GetRecord(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (rec && !(rec->m_flags & 2)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 0f6732e2 auto
// OFFSET: 0x005B1A00
static int32_t Script_GetAchievementReward(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAchievementNumRewards(achievementID, rewardIndex)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    lua_tonumber(L, 2);
    AchievementRec* rec = g_achievementDB.GetRecord(id);

    if (rec && !(rec->m_flags & 2)) {
        lua_pushnumber(L, rec->m_points);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 3bbd7b03 manual
// OFFSET: 0x005B1AB0
static int32_t Script_SetAchievementComparisonUnit(lua_State* L) {
    WOWGUID guid = WOWGUID();

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: AddAchievementComparisonUnit(unitToken)");
        return 0;
    }

    Script_GetGUIDFromToken(lua_tolstring(L, 1, nullptr), guid, false);

    if (!guid) {
        return 0;
    }

    CGAchievementInfo::m_inspect = guid;
    lua_pushnumber(L, 1.0);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUERY_INSPECT_ACHIEVEMENTS));
    SmartGUID packed;
    packed.m_guid = guid.m_guid;
    msg << packed;
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 1;
}

// VERIFY: da85ee7f manual
// OFFSET: 0x005B2040
static int32_t Script_GetTrackedAchievements(lua_State* L) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
        count++;
        lua_pushnumber(L, node->m_id);
    }

    return count;
}

// VERIFY: 8a018839 manual
// OFFSET: 0x005B1EC0
static int32_t Script_HasCompletedAnyAchievement(lua_State* L) {
    lua_pushboolean(L, s_completedAchievements.Head() != nullptr);
    return 1;
}

// VERIFY: 209a02f3 manual
// OFFSET: 0x005B1390
static int32_t Script_GetCategoryList(lua_State* L) {
    int32_t count = 1;

    if (lua_type(L, 1) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    for (int32_t i = 0; i < g_achievement_CategoryDB.m_numRecords; i++) {
        Achievement_CategoryRec* category = g_achievement_CategoryDB.GetRecordByIndex(i);

        if (!category || category->m_id == 1) {
            continue;
        }

        Achievement_CategoryRec* parent = g_achievement_CategoryDB.GetRecord(category->m_parent);

        while (parent && parent->m_id != 1) {
            parent = g_achievement_CategoryDB.GetRecord(parent->m_parent);
        }

        if (parent) {
            continue;
        }

        lua_checkstack(L, 2);
        lua_pushnumber(L, count);
        lua_pushnumber(L, category->m_id);
        lua_settable(L, -3);
        count++;
    }

    return 1;
}

// VERIFY: c2d78337 manual
// OFFSET: 0x005B15C0
static int32_t Script_GetCategoryInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCategoryInfo(categoryID)");
        return 0;
    }

    Achievement_CategoryRec* category = g_achievement_CategoryDB.GetRecord(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (!category) {
        return 0;
    }

    lua_pushstring(L, category->m_name);
    lua_pushnumber(L, category->m_parent == 1 ? -1.0 : category->m_parent);
    lua_pushnumber(L, 0.0);
    return 3;
}

// VERIFY: 2fc15aff auto
// OFFSET: 0x005B1DC0
static int32_t Script_GetTotalAchievementPoints(lua_State* L) {
    int32_t points = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
        AchievementRec* achievement = g_achievementDB.GetRecord(node->m_id);

        if (achievement) {
            points += achievement->m_points;
        }
    }

    lua_pushnumber(L, points);
    return 1;
}

// VERIFY: 960921b3 manual
// OFFSET: 0x005B38E0
static int32_t Script_GetCategoryNumAchievements(lua_State* L) {
    int32_t total = 0;
    int32_t pending = 0;
    int32_t completed = 0;
    int32_t previous = 0;

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCategoryNumAchievements(categoryID)");
        return 0;
    }

    int32_t categoryId = static_cast<int32_t>(lua_tonumber(L, 1));

    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);
        bool anyCategory = false;
        bool includeAll = false;

        if (categoryId == -1 && achievement && !(achievement->m_flags & 0x1)) {
            anyCategory = true;
            includeAll = true;
        } else if (categoryId == -2 && achievement && (achievement->m_flags & 0x1)) {
            anyCategory = true;
            includeAll = true;
        } else if (lua_type(L, 2) == LUA_TBOOLEAN && lua_toboolean(L, 2)) {
            includeAll = true;
        }

        if (!achievement || (achievement->m_category != categoryId && !anyCategory) || (achievement->m_flags & 0x2)) {
            continue;
        }

        CLIENT_ACHIEVEMENTS* done = nullptr;

        for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
            if (node->m_id == achievement->m_id) {
                done = node;
                break;
            }
        }

        CLIENT_ACHIEVEMENTS* inspected = nullptr;

        for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; node = s_inspectAchievements.Next(node)) {
            if (node->m_id == achievement->m_id) {
                inspected = node;
                break;
            }
        }

        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player && achievement->m_faction != -1 && achievement->m_faction != player->GetPVPFactionIndex() && !inspected) {
            continue;
        }

        if ((achievement->m_category == 81 || (achievement->m_flags & 0x800)) && !done && !inspected) {
            continue;
        }

        if (previous && previous == achievement->m_supercedes) {
            previous = achievement->m_id;

            if (includeAll) {
                total++;
            } else if (inspected) {
                pending++;
            }

            continue;
        }

        if (done) {
            if (!CGAchievementInfo::IsSupersededByCompleted(achievement->m_id) || includeAll) {
                completed++;
                total++;
            }

            continue;
        }

        if (!inspected) {
            previous = achievement->m_id;
            total++;
            continue;
        }

        pending++;
    }

    lua_pushnumber(L, categoryId == 81 ? pending + completed : pending + total);
    lua_pushnumber(L, completed);
    return 2;
}

// VERIFY: a428d377 manual
// OFFSET: 0x005B3CA0
static int32_t Script_GetNumCompletedAchievements(lua_State* L) {
    int32_t total = 0;
    int32_t completed = 0;

    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);

        if (!achievement || !CGAchievementInfo::IsRealAchievement(achievement->m_id) || (achievement->m_flags & 0x2)) {
            continue;
        }

        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player && achievement->m_faction != -1 && achievement->m_faction != player->GetPVPFactionIndex()) {
            continue;
        }

        CLIENT_ACHIEVEMENTS* done = nullptr;

        for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
            if (node->m_id == achievement->m_id) {
                done = node;
                break;
            }
        }

        if (achievement->m_category == 81) {
            continue;
        }

        if (done) {
            completed++;
            total++;
        } else if (!(achievement->m_flags & 0x800)) {
            total++;
        }
    }

    lua_pushnumber(L, total);
    lua_pushnumber(L, completed);
    return 2;
}

// VERIFY: ed883a88 manual
// OFFSET: 0x005B2350
static CLIENT_ACHIEVEMENT_CRITERIA* GetCriteria(int32_t id) {
    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_criteria.Head(); node; node = s_criteria.Next(node)) {
        if (node->m_id == id) {
            return node;
        }
    }

    return nullptr;
}

// VERIFY: fc2ec323 manual
// OFFSET: 0x005B2380
static CLIENT_ACHIEVEMENTS* GetCompletedAchievement(int32_t id) {
    for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
        if (node->m_id == id) {
            return node;
        }
    }

    return nullptr;
}

// VERIFY: 76ebd783 manual
// OFFSET: 0x005B23B0
static CLIENT_ACHIEVEMENTS* GetInspectAchievement(int32_t id) {
    for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; node = s_inspectAchievements.Next(node)) {
        if (node->m_id == id) {
            return node;
        }
    }

    return nullptr;
}

// VERIFY: 917fae55 manual
// OFFSET: 0x005B14B0
static int32_t Script_GetStatisticsCategoryList(lua_State* L) {
    int32_t count = 1;

    if (lua_type(L, 1) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    for (int32_t i = 0; i < g_achievement_CategoryDB.m_numRecords; i++) {
        Achievement_CategoryRec* category = g_achievement_CategoryDB.GetRecordByIndex(i);

        if (!category) {
            continue;
        }

        Achievement_CategoryRec* parent = g_achievement_CategoryDB.GetRecord(category->m_parent);

        while (parent && parent->m_id != 1) {
            parent = g_achievement_CategoryDB.GetRecord(parent->m_parent);
        }

        if (!parent) {
            continue;
        }

        lua_checkstack(L, 2);
        lua_pushnumber(L, count);
        lua_pushnumber(L, category->m_id);
        lua_settable(L, -3);
        count++;
    }

    return 1;
}

// VERIFY: 3a8c4c7a auto
// OFFSET: 0x005B1720
static int32_t Script_GetAchievementInfoFromCriteria(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementInfoFromCriteria(criteriaID)");
        return 0;
    }

    Achievement_CriteriaRec* criteria = g_achievement_CriteriaDB.GetRecord(static_cast<int32_t>(lua_tonumber(L, 1)));

    if (!criteria) {
        return 0;
    }

    AchievementRec* achievement = g_achievementDB.GetRecord(criteria->m_achievementID);

    if (!achievement) {
        return 0;
    }

    lua_pushnumber(L, criteria->m_achievementID);
    char text[3000];
    WOWGUID guid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(achievement->m_title, text, sizeof(text), guid, nullptr, 0);
    lua_pushstring(L, text);
    lua_pushnumber(L, achievement->m_points);
    guid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(achievement->m_description, text, sizeof(text), guid, nullptr, 0);
    lua_pushstring(L, text);
    lua_pushnumber(L, achievement->m_flags);
    SpellIconRec* icon = g_spellIconDB.GetRecord(achievement->m_iconID);
    lua_pushstring(L, icon ? icon->m_textureFilename : "");
    guid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(achievement->m_reward, text, sizeof(text), guid, nullptr, 0);
    lua_pushstring(L, text);
    return 7;
}

// VERIFY: 60da3a42 auto
// OFFSET: 0x005B1C20
static int32_t Script_GetLatestCompletedAchievements(lua_State* L) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node && count < 5; node = s_completedAchievements.Next(node)) {
        int32_t id = node->m_id;

        if (CGAchievementInfo::IsRealAchievement(id)) {
            lua_pushnumber(L, id);
            count++;
        }
    }

    return count;
}

// VERIFY: 6500aa82 auto
// OFFSET: 0x005B1C90
static int32_t Script_GetLatestUpdatedStats(lua_State* L) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_recentCriteria.Head(); node && count < 5; node = s_recentCriteria.Next(node)) {
        lua_pushnumber(L, node->m_id);
        count++;
    }

    return count;
}

// VERIFY: b22a1e00 auto
// OFFSET: 0x005B1CF0
static int32_t Script_GetLatestCompletedComparisonAchievements(lua_State* L) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node && count < 5; node = s_inspectAchievements.Next(node)) {
        int32_t id = node->m_id;

        if (CGAchievementInfo::IsRealAchievement(id)) {
            lua_pushnumber(L, id);
            count++;
        }
    }

    return count;
}

// VERIFY: cf76747b auto
// OFFSET: 0x005B1D60
static int32_t Script_GetLatestUpdatedComparisonStats(lua_State* L) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_inspectRecentCriteria.Head(); node && count < 5; node = s_inspectRecentCriteria.Next(node)) {
        lua_pushnumber(L, node->m_id);
        count++;
    }

    return count;
}

// VERIFY: b6547fd9 auto
// OFFSET: 0x005B1E40
static int32_t Script_GetComparisonAchievementPoints(lua_State* L) {
    int32_t points = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; node = s_inspectAchievements.Next(node)) {
        AchievementRec* achievement = g_achievementDB.GetRecord(node->m_id);

        if (achievement) {
            points += achievement->m_points;
        }
    }

    lua_pushnumber(L, points);
    return 1;
}

// VERIFY: a5598214 manual
// OFFSET: 0x005B2090
static int32_t Script_IsTrackedAchievement(lua_State* L) {
    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));

    if (id) {
        for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
            if (node->m_id == id) {
                lua_pushboolean(L, 1);
                return 1;
            }
        }
    }

    lua_pushboolean(L, 0);
    return 1;
}

// VERIFY: 97128901 manual
// OFFSET: 0x005B2120
static int32_t Script_GetNumTrackedAchievements(lua_State* L) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
        count++;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 59d37944 manual
// OFFSET: 0x005B3B30
static int32_t Script_GetComparisonCategoryNumAchievements(lua_State* L) {
    int32_t count = 0;

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetComparisonCategoryNumAchievements(categoryID)");
        return 0;
    }

    int32_t categoryId = static_cast<int32_t>(lua_tonumber(L, 1));

    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);
        bool includeAll = false;

        if (categoryId == -1 && achievement && !(achievement->m_flags & 0x1)) {
            includeAll = true;
        } else if (categoryId == -2 && achievement && (achievement->m_flags & 0x1)) {
            includeAll = true;
        }

        if (!achievement || (achievement->m_category != categoryId && !includeAll) || (achievement->m_flags & 0x2)) {
            continue;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(CGAchievementInfo::m_inspect, TYPE_PLAYER, __FILE__, __LINE__));

        if (player && achievement->m_faction != -1 && achievement->m_faction != player->GetPVPFactionIndex()) {
            continue;
        }

        for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; node = s_inspectAchievements.Next(node)) {
            if (node->m_id == achievement->m_id) {
                if (!CGAchievementInfo::IsSupersededByCompleted(achievement->m_id) || includeAll) {
                    count++;
                }

                break;
            }
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 4050439b manual
// OFFSET: 0x005B3DC0
static int32_t Script_GetNumComparisonCompletedAchievements(lua_State* L) {
    int32_t total = 0;
    int32_t completed = 0;

    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);

        if (!achievement || !CGAchievementInfo::IsRealAchievement(achievement->m_id) || (achievement->m_flags & 0x2)) {
            continue;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(CGAchievementInfo::m_inspect, TYPE_PLAYER, __FILE__, __LINE__));

        if (player && achievement->m_faction != -1 && achievement->m_faction != player->GetPVPFactionIndex()) {
            continue;
        }

        CLIENT_ACHIEVEMENTS* inspected = nullptr;

        for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; node = s_inspectAchievements.Next(node)) {
            if (node->m_id == achievement->m_id) {
                inspected = node;
                break;
            }
        }

        if (achievement->m_category == 81) {
            continue;
        }

        if (inspected) {
            completed++;
            total++;
        } else if (!(achievement->m_flags & 0x800)) {
            total++;
        }
    }

    lua_pushnumber(L, total);
    lua_pushnumber(L, completed);
    return 2;
}

// VERIFY: d5afdfba manual
// OFFSET: 0x005B3EF0
static int32_t Script_GetNextAchievement(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetNextAchievement(achievementID)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));

    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);

        if (!achievement || achievement->m_supercedes != id) {
            continue;
        }

        lua_pushnumber(L, achievement->m_id);

        for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
            if (node->m_id == achievement->m_id) {
                lua_pushboolean(L, 1);
                return 2;
            }
        }

        return 1;
    }

    return 0;
}

// VERIFY: 66fda709 manual
// OFFSET: 0x005B3FC0
static int32_t Script_GetAchievementInfo(lua_State* L) {
    int32_t previous = 0;
    int32_t id = -1;

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementInfo(achievementID)");
        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        id = static_cast<int32_t>(lua_tonumber(L, 1));
    } else {
        int32_t categoryId = static_cast<int32_t>(lua_tonumber(L, 1));
        int32_t index = static_cast<int32_t>(lua_tonumber(L, 2));

        for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node && id == -1; node = s_completedAchievements.Next(node)) {
            CLIENT_ACHIEVEMENTS* inspected = nullptr;

            for (CLIENT_ACHIEVEMENTS* other = s_inspectAchievements.Head(); other; other = s_inspectAchievements.Next(other)) {
                if (other->m_id == node->m_id) {
                    inspected = other;
                    break;
                }
            }

            if (!inspected) {
                continue;
            }

            AchievementRec* achievement = g_achievementDB.GetRecord(node->m_id);

            if (categoryId == -1 && !(achievement->m_flags & 0x1)) {
                if (!(achievement->m_flags & 0x2)) {
                    index--;
                }

                if (!index) {
                    id = achievement->m_id;
                }
            } else if (categoryId == -2 && achievement && (achievement->m_flags & 0x1)) {
                if (!(achievement->m_flags & 0x2)) {
                    index--;
                }

                if (!index) {
                    id = achievement->m_id;
                }
            } else if (achievement && achievement->m_category == categoryId && !CGAchievementInfo::IsSupersededByCompleted(achievement->m_id) && !--index) {
                id = achievement->m_id;
            }
        }

        if (id == -1 && index > 0) {
            for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node && id == -1; node = s_completedAchievements.Next(node)) {
                CLIENT_ACHIEVEMENTS* inspected = nullptr;

                for (CLIENT_ACHIEVEMENTS* other = s_inspectAchievements.Head(); other; other = s_inspectAchievements.Next(other)) {
                    if (other->m_id == node->m_id) {
                        inspected = other;
                        break;
                    }
                }

                if (inspected) {
                    continue;
                }

                AchievementRec* achievement = g_achievementDB.GetRecord(node->m_id);

                if (categoryId == -1 && !(achievement->m_flags & 0x1)) {
                    if (!(achievement->m_flags & 0x2)) {
                        index--;
                    }

                    if (!index) {
                        id = achievement->m_id;
                    }
                } else if (categoryId == -2 && achievement && (achievement->m_flags & 0x1)) {
                    if (!(achievement->m_flags & 0x2)) {
                        index--;
                    }

                    if (!index) {
                        id = achievement->m_id;
                    }
                } else if (achievement && achievement->m_category == categoryId && !CGAchievementInfo::IsSupersededByCompleted(achievement->m_id) && !--index) {
                    id = achievement->m_id;
                }
            }

            if (id == -1 && index > 0) {
                for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node && id == -1; node = s_inspectAchievements.Next(node)) {
                    CLIENT_ACHIEVEMENTS* done = nullptr;

                    for (CLIENT_ACHIEVEMENTS* other = s_completedAchievements.Head(); other; other = s_completedAchievements.Next(other)) {
                        if (other->m_id == node->m_id) {
                            done = other;
                            break;
                        }
                    }

                    if (done) {
                        continue;
                    }

                    AchievementRec* achievement = g_achievementDB.GetRecord(node->m_id);

                    if (categoryId == -1 && !(achievement->m_flags & 0x1)) {
                        if (!(achievement->m_flags & 0x2)) {
                            index--;
                        }

                        if (!index) {
                            id = achievement->m_id;
                        }
                    } else if (categoryId == -2 && achievement && (achievement->m_flags & 0x1)) {
                        if (!(achievement->m_flags & 0x2)) {
                            index--;
                        }

                        if (!index) {
                            id = achievement->m_id;
                        }
                    } else if (achievement && achievement->m_category == categoryId && !CGAchievementInfo::IsSupersededByCompleted(achievement->m_id) && !--index) {
                        id = achievement->m_id;
                    }
                }
            }
        }

        if (id == -1) {
            if (categoryId == 81) {
                return 0;
            }

            for (int32_t i = 0; i < g_achievementDB.m_numRecords && id == -1; i++) {
                AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

                if (player && achievement->m_faction != -1 && achievement->m_faction != player->GetPVPFactionIndex()) {
                    continue;
                }

                if (categoryId == -1) {
                    if (!(achievement->m_flags & 0x1) && achievement->m_category != 81) {
                        if (GetCompletedAchievement(achievement->m_id) || GetInspectAchievement(achievement->m_id)) {
                            continue;
                        }

                        if (!(achievement->m_flags & 0x800)) {
                            if (!(achievement->m_flags & 0x2)) {
                                index--;
                            }

                            if (!index) {
                                id = achievement->m_id;
                            }
                        }

                        continue;
                    }
                } else if (categoryId == -2) {
                    if (!achievement) {
                        continue;
                    }

                    if ((achievement->m_flags & 0x1) && achievement->m_category != 81) {
                        if (achievement->m_flags & 0x800) {
                            continue;
                        }

                        if (!(achievement->m_flags & 0x2)) {
                            index--;
                        }

                        if (!index) {
                            id = achievement->m_id;
                        }

                        continue;
                    }
                }

                if (!achievement || achievement->m_category != categoryId) {
                    continue;
                }

                CLIENT_ACHIEVEMENTS* done = GetCompletedAchievement(achievement->m_id);
                CLIENT_ACHIEVEMENTS* inspected = GetInspectAchievement(achievement->m_id);

                if (previous && previous == achievement->m_supercedes) {
                    previous = achievement->m_id;
                } else if (!done && !inspected && !(achievement->m_flags & 0x800)) {
                    previous = achievement->m_id;

                    if (index-- == 1) {
                        id = achievement->m_id;
                    }
                }
            }

            if (id == -1) {
                return 0;
            }
        }
    }

    if (id < 0) {
        return 0;
    }

    AchievementRec* achievement = g_achievementDB.GetRecord(id);

    if (!achievement || (achievement->m_flags & 0x2)) {
        return 0;
    }

    CLIENT_ACHIEVEMENTS* done = nullptr;

    for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
        if (node->m_id == achievement->m_id) {
            done = node;
            break;
        }
    }

    if ((achievement->m_flags & 0x800) && !done) {
        return 0;
    }

    lua_pushnumber(L, id);
    char text[3000];
    WOWGUID guid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(achievement->m_title, text, sizeof(text), guid, nullptr, 0);
    lua_pushstring(L, text);
    lua_pushnumber(L, achievement->m_points);

    if (done) {
        lua_pushboolean(L, 1);
        lua_pushnumber(L, done->m_date.m_month + 1);
        lua_pushnumber(L, done->m_date.m_monthDay + 1);
        lua_pushnumber(L, done->m_date.m_year);
    } else {
        lua_pushboolean(L, 0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    guid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(achievement->m_description, text, sizeof(text), guid, nullptr, 0);
    lua_pushstring(L, text);
    lua_pushnumber(L, achievement->m_flags);
    SpellIconRec* icon = g_spellIconDB.GetRecord(achievement->m_iconID);
    lua_pushstring(L, icon ? icon->m_textureFilename : "");
    guid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(achievement->m_reward, text, sizeof(text), guid, nullptr, 0);
    lua_pushstring(L, text);
    return 11;
}

// VERIFY: c47def18 auto
// OFFSET: 0x005B4760
static int32_t Script_ClearAchievementComparisonUnit(lua_State* L) {
    CGAchievementInfo::ReleaseInspect();
    return 0;
}

// VERIFY: 4a0dc04c manual
// OFFSET: 0x005B4770
static int32_t Script_GetAchievementComparisonInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementComparisonInfo(achievementID)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));

    if (!CGAchievementInfo::m_inspect) {
        return 0;
    }

    CLIENT_ACHIEVEMENTS* inspected = GetInspectAchievement(id);
    AchievementRec* achievement = g_achievementDB.GetRecord(id);

    if (!inspected || !achievement) {
        return 0;
    }

    lua_pushboolean(L, 1);
    lua_pushnumber(L, inspected->m_date.m_month + 1);
    lua_pushnumber(L, inspected->m_date.m_monthDay + 1);
    lua_pushnumber(L, inspected->m_date.m_year);
    lua_pushnumber(L, achievement->m_flags);
    return 5;
}

// VERIFY: 9a10a883 auto
// OFFSET: 0x005B4870
static int32_t Script_AddTrackedAchievement(lua_State* L) {
    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));

    if (!id || !g_achievementDB.GetRecord(id)) {
        luaL_error(L, "Usage: AddTrackedAchievement(achievementID)");
        return 0;
    }

    AddTrackedAchievement(id);
    return 0;
}

// VERIFY: 7eb57b4e auto
// OFFSET: 0x005B48D0
static int32_t Script_RemoveTrackedAchievement(lua_State* L) {
    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));

    if (!id) {
        luaL_error(L, "Usage: RemoveTrackedAchievement(achievementID)");
        return 0;
    }

    CGAchievementInfo::RemoveTrackedAchievement(id);
    return 0;
}

// VERIFY: 0c281765 auto
// OFFSET: 0x005B5290
static int32_t Script_GetQuestsCompleted(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    lua_checkstack(L, 2);

    for (COMPLETED_QUESTS* node = s_completedQuests.Head(); node; node = s_completedQuests.Next(node)) {
        lua_pushnumber(L, node->m_questId);
        lua_pushboolean(L, 1);
        lua_settable(L, -3);
    }

    return 1;
}

// VERIFY: 3304a2dc manual
// OFFSET: 0x005B5530
static int32_t Script_GetAchievementNumCriteria(lua_State* L) {
    int32_t count = 0;
    int32_t result = 0;

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementNumCriteria(achievementID)");
        return 0;
    }

    ACHIEVEMENT* achievement = s_achievements.Ptr(static_cast<int32_t>(lua_tonumber(L, 1)), HASHKEY_NONE());

    if (!achievement) {
        return 0;
    }

    ACHIEVEMENT* source = achievement;

    if (achievement->m_rec->m_sharesCriteria) {
        source = s_achievements.Ptr(achievement->m_rec->m_sharesCriteria, HASHKEY_NONE());
    }

    if (source && !(achievement->m_rec->m_flags & 0x2)) {
        if (achievement->m_rec->m_flags & 0xB0) {
            count = 1;
        }

        for (SPECIFIC_CRITERIA* node = source->m_criteria.Head(); node; node = source->m_criteria.Next(node)) {
            if (node->m_criteria && !(node->m_criteria->m_flags & 0x2)) {
                count++;
            }
        }

        result = count;
    }

    lua_pushnumber(L, result);
    return 1;
}

// VERIFY: 1dda108b manual
// OFFSET: 0x005B5620
static int32_t Script_GetStatistic(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetStatistic(achievementID)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    uint64_t value = 0;
    uint64_t sum = 0;
    uint64_t highest = 0;
    uint64_t count = 0;
    int32_t highestId = -1;
    int32_t criteriaFlags = 0;
    bool tie = false;
    CLIENT_ACHIEVEMENT_CRITERIA* earliest = nullptr;
    ACHIEVEMENT* achievement = s_achievements.Ptr(id, HASHKEY_NONE());

    if (!achievement) {
        return 0;
    }

    ACHIEVEMENT* source = achievement;

    if (achievement->m_rec->m_sharesCriteria) {
        source = s_achievements.Ptr(achievement->m_rec->m_sharesCriteria, HASHKEY_NONE());
    }

    Achievement_CriteriaRec* earliestRec = nullptr;

    if (source) {
        for (SPECIFIC_CRITERIA* spec = source->m_criteria.Head(); spec; spec = source->m_criteria.Next(spec)) {
            for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_criteria.Head(); node; node = s_criteria.Next(node)) {
                if (node->m_id != spec->m_criteria->m_id) {
                    continue;
                }

                int32_t flags = achievement->m_rec->m_flags;

                if (flags & 0x8) {
                    sum += node->m_counter;
                }

                if (flags & 0x10) {
                    if (node->m_counter > highest) {
                        highest = node->m_counter;
                        highestId = spec->m_criteria->m_id;
                        tie = false;
                    } else if (node->m_counter == highest) {
                        tie = true;
                    }
                }

                if ((flags & 0x20) && node->m_counter >= static_cast<uint64_t>(static_cast<int64_t>(spec->m_criteria->m_quantity))) {
                    count++;
                }

                value = node->m_counter;
                Achievement_CriteriaRec* rec = g_achievement_CriteriaDB.GetRecord(node->m_id);

                if (rec) {
                    criteriaFlags = rec->m_flags;
                }

                if (!earliest || node->m_timer2 < earliest->m_timer2) {
                    earliest = node;
                }

                break;
            }
        }

        if (tie && (source->m_rec->m_flags & 0x400)) {
            highestId = 0;
        }

        if (earliest) {
            earliestRec = g_achievement_CriteriaDB.GetRecord(earliest->m_id);
        }
    }

    char text[3000];
    CGAchievementInfo::BuildOutputString(text, sizeof(text), sum, highest, highestId, count, value, earliest, achievement->m_rec->m_flags, criteriaFlags, false, achievement->m_rec, earliestRec, 0);
    lua_pushstring(L, text);
    return 1;
}

// VERIFY: d4e23c37 manual
// OFFSET: 0x005B58B0
static int32_t Script_GetAchievementCriteriaInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementCrieriaInfo(achievementID, criteriaIndex)");
        return 0;
    }

    int32_t achievementId = -1;
    bool showAll = false;
    int32_t criteriaId;

    if (lua_isnumber(L, 2)) {
        achievementId = static_cast<int32_t>(lua_tonumber(L, 1));
        int32_t index = static_cast<int32_t>(lua_tonumber(L, 2));
        ACHIEVEMENT* achievement = s_achievements.Ptr(achievementId, HASHKEY_NONE());

        if (!achievement) {
            return 0;
        }

        ACHIEVEMENT* source = achievement;

        if (achievement->m_rec->m_sharesCriteria) {
            source = s_achievements.Ptr(achievement->m_rec->m_sharesCriteria, HASHKEY_NONE());
        }

        if (!source) {
            return 0;
        }

        SPECIFIC_CRITERIA* spec = source->m_criteria.Head();

        for (;; spec = source->m_criteria.Next(spec)) {
            if (!spec) {
                return 0;
            }

            if (!(spec->m_criteria->m_flags & 0x2) || (achievement->m_rec->m_flags & 0xB0)) {
                if (achievement->m_rec->m_flags & 0xB0) {
                    showAll = true;
                }

                if (!--index) {
                    break;
                }
            }
        }

        criteriaId = spec->m_criteria->m_id;
    } else {
        criteriaId = static_cast<int32_t>(lua_tonumber(L, 1));
    }

    if (criteriaId < 0) {
        return 0;
    }

    Achievement_CriteriaRec* criteria = g_achievement_CriteriaDB.GetRecord(criteriaId);

    if (!criteria) {
        return 0;
    }

    if (achievementId == -1) {
        achievementId = criteria->m_achievementID;
    }

    AchievementRec* achievementRec = g_achievementDB.GetRecord(achievementId);

    if (!achievementRec || (achievementRec->m_flags & 0x2)) {
        return 0;
    }

    CLIENT_ACHIEVEMENT_CRITERIA* progress = GetCriteria(criteriaId);
    CLIENT_ACHIEVEMENTS* done = GetCompletedAchievement(achievementId);
    uint64_t sum = 0;
    uint64_t highest = 0;
    uint64_t count = 0;
    int32_t highestId = -1;
    bool tie = false;
    ACHIEVEMENT* achievement = s_achievements.Ptr(achievementId, HASHKEY_NONE());

    if (!achievement) {
        return 0;
    }

    ACHIEVEMENT* source = achievement;

    if (achievement->m_rec->m_sharesCriteria) {
        source = s_achievements.Ptr(achievement->m_rec->m_sharesCriteria, HASHKEY_NONE());
    }

    if (source) {
        for (SPECIFIC_CRITERIA* spec = source->m_criteria.Head(); spec; spec = source->m_criteria.Next(spec)) {
            CLIENT_ACHIEVEMENT_CRITERIA* node = GetCriteria(spec->m_criteria->m_id);

            if (!node) {
                continue;
            }

            int32_t flags = achievement->m_rec->m_flags;

            if (flags & 0x8) {
                sum += node->m_counter;
            }

            if (flags & 0x10) {
                if (node->m_counter > highest) {
                    highest = node->m_counter;
                    highestId = spec->m_criteria->m_id;
                    tie = false;
                } else if (node->m_counter == highest) {
                    tie = true;
                }
            }

            if ((flags & 0x20) && node->m_counter >= static_cast<uint64_t>(static_cast<int64_t>(spec->m_criteria->m_quantity))) {
                count++;
            }
        }

        if (tie && (achievement->m_rec->m_flags & 0x400)) {
            highestId = 0;
        }
    }

    AchievementRec* rec = achievement->m_rec;
    int32_t criteriaFlags = criteria->m_flags;
    int32_t flags = rec->m_flags;
    int32_t required = 0;

    if (flags & 0x80) {
        criteriaFlags |= 0x1;

        if (done && (flags & 0x8)) {
            sum = static_cast<int64_t>(criteria->m_quantity);
        }

        if (flags & 0x20) {
            required = rec->m_minimumCriteria;
        }
    }

    char text[3000];
    int64_t shown = CGAchievementInfo::BuildOutputString(text, sizeof(text), sum, highest, highestId, count, progress ? progress->m_counter : 0, progress, flags, criteriaFlags, done != nullptr, rec, criteria, required);

    if (!progress || showAll) {
        lua_pushstring(L, criteria->m_description);
        lua_pushnumber(L, criteria->m_type);

        if (!done || achievement->m_rec->m_minimumCriteria) {
            lua_pushboolean(L, 0);
            lua_pushnumber(L, static_cast<double>(shown));
            lua_pushnumber(L, required ? required : criteria->m_quantity);
        } else {
            lua_pushboolean(L, 1);
            lua_pushnumber(L, required ? required : criteria->m_quantity);
            lua_pushnumber(L, required ? required : criteria->m_quantity);
        }

        lua_pushnil(L);
    } else {
        lua_pushstring(L, criteria->m_description);
        lua_pushnumber(L, criteria->m_type);

        if ((!done || achievementRec->m_minimumCriteria) && progress->m_counter < static_cast<uint64_t>(static_cast<int64_t>(criteria->m_quantity))) {
            lua_pushboolean(L, 0);
        } else {
            lua_pushboolean(L, 1);
        }

        lua_pushnumber(L, static_cast<double>(shown));
        lua_pushnumber(L, required ? required : criteria->m_quantity);
        NameCache* name = g_nameDBCache.GetRecord(progress->m_player.m_guid, WOWGUID(), nullptr, nullptr, false);

        if (name) {
            lua_pushstring(L, name->m_name);
        } else {
            lua_pushnil(L);
        }
    }

    lua_pushnumber(L, criteriaFlags);
    lua_pushnumber(L, criteria->m_assetID);
    lua_pushstring(L, text);
    lua_pushnumber(L, criteriaId);
    return 10;
}

// VERIFY: 9555ffc2 manual
// OFFSET: 0x005B5E70
static int32_t Script_GetComparisonStatistic(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetComparisonStatistic(achievementID)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    uint64_t value = 0;
    uint64_t sum = 0;
    uint64_t highest = 0;
    uint64_t count = 0;
    int32_t highestId = -1;
    int32_t criteriaFlags = 0;
    bool tie = false;
    ACHIEVEMENT* achievement = s_achievements.Ptr(id, HASHKEY_NONE());

    if (!achievement) {
        return 0;
    }

    ACHIEVEMENT* source = achievement;
    CLIENT_ACHIEVEMENT_CRITERIA* earliest = nullptr;

    if (achievement->m_rec->m_sharesCriteria) {
        source = s_achievements.Ptr(achievement->m_rec->m_sharesCriteria, HASHKEY_NONE());
    }

    Achievement_CriteriaRec* earliestRec = nullptr;

    if (source) {
        for (SPECIFIC_CRITERIA* spec = source->m_criteria.Head(); spec; spec = source->m_criteria.Next(spec)) {
            int32_t criteriaId = spec->m_criteria->m_id;

            for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_inspectCriteria.Head(); node; node = s_inspectCriteria.Next(node)) {
                if (node->m_id != criteriaId) {
                    continue;
                }

                int32_t flags = achievement->m_rec->m_flags;

                if (flags & 0x8) {
                    sum += node->m_counter;
                }

                if (flags & 0x10) {
                    if (node->m_counter > highest) {
                        highest = node->m_counter;
                        highestId = spec->m_criteria->m_id;
                        tie = false;
                    } else if (node->m_counter == highest) {
                        tie = true;
                    }
                }

                if ((flags & 0x20) && node->m_counter >= static_cast<uint64_t>(static_cast<int64_t>(spec->m_criteria->m_quantity))) {
                    count++;
                }

                value = node->m_counter;
                Achievement_CriteriaRec* rec = g_achievement_CriteriaDB.GetRecord(node->m_id);

                if (rec) {
                    criteriaFlags = rec->m_flags;
                }

                if (!earliest || node->m_timer2 < earliest->m_timer2) {
                    earliest = node;
                }

                break;
            }
        }

        if (tie && (source->m_rec->m_flags & 0x400)) {
            highestId = 0;
        }

        if (earliest) {
            earliestRec = g_achievement_CriteriaDB.GetRecord(earliest->m_id);
        }
    }

    char text[3000];
    CGAchievementInfo::BuildOutputString(text, sizeof(text), sum, highest, highestId, count, value, earliest, achievement->m_rec->m_flags, criteriaFlags, false, achievement->m_rec, earliestRec, 0);
    lua_pushstring(L, text);
    return 1;
}

// VERIFY: f5d25aa2 manual
// OFFSET: 0x005B6110
static int32_t Script_GetAchievementLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAchievementLink(achievementID)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    AchievementRec* achievement = g_achievementDB.GetRecord(id);

    if (!achievement) {
        return 0;
    }

    int32_t index = 0;
    uint32_t criteriaBits[4] = { 0, 0, 0, 0 };
    CLIENT_ACHIEVEMENTS* done = GetCompletedAchievement(achievement->m_id);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    ACHIEVEMENT* source = s_achievements.Ptr(id, HASHKEY_NONE());

    if (source->m_rec->m_sharesCriteria) {
        source = s_achievements.Ptr(source->m_rec->m_sharesCriteria, HASHKEY_NONE());
    }

    if (source) {
        for (SPECIFIC_CRITERIA* spec = source->m_criteria.Head(); spec; spec = source->m_criteria.Next(spec)) {
            Achievement_CriteriaRec* criteria = spec->m_criteria;

            if (criteria->m_flags & 0x2) {
                continue;
            }

            for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_criteria.Head(); node; node = s_criteria.Next(node)) {
                if (node->m_id != criteria->m_id) {
                    continue;
                }

                if (node->m_counter >= static_cast<uint64_t>(static_cast<int64_t>(spec->m_criteria->m_quantity))) {
                    if (index < 32) {
                        criteriaBits[0] |= 1 << index;
                    } else if (index < 64) {
                        criteriaBits[1] |= 1 << (index - 32);
                    } else if (index < 96) {
                        criteriaBits[2] |= 1 << (index - 64);
                    } else if (index < 128) {
                        criteriaBits[3] |= 1 << (index - 96);
                    }
                }

                break;
            }

            index++;
        }
    }

    if (done) {
        if (!source->m_rec->m_minimumCriteria) {
            criteriaBits[0] = 0xFFFFFFFF;
            criteriaBits[1] = 0xFFFFFFFF;
            criteriaBits[2] = 0xFFFFFFFF;
            criteriaBits[3] = 0xFFFFFFFF;
        }

        lua_pushstring(L, CGTooltip::GetAchievementHyperlink(id, player->m_obj->m_guid, 1, done->m_date, criteriaBits[0], criteriaBits[1], criteriaBits[2], criteriaBits[3]));
        return 1;
    }

    WowTime time;
    lua_pushstring(L, CGTooltip::GetAchievementHyperlink(id, player->m_obj->m_guid, 0, time, criteriaBits[0], criteriaBits[1], criteriaBits[2], criteriaBits[3]));
    return 1;
}

// STATIC: 0x00ACFC70
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetCategoryList", &Script_GetCategoryList },
    { "GetStatisticsCategoryList", &Script_GetStatisticsCategoryList },
    { "GetCategoryInfo", &Script_GetCategoryInfo },
    { "GetCategoryNumAchievements", &Script_GetCategoryNumAchievements },
    { "GetComparisonCategoryNumAchievements", &Script_GetComparisonCategoryNumAchievements },
    { "GetAchievementInfo", &Script_GetAchievementInfo },
    { "GetAchievementNumRewards", &Script_GetAchievementNumRewards },
    { "GetAchievementReward", &Script_GetAchievementReward },
    { "GetAchievementNumCriteria", &Script_GetAchievementNumCriteria },
    { "GetAchievementCriteriaInfo", &Script_GetAchievementCriteriaInfo },
    { "SetAchievementComparisonUnit", &Script_SetAchievementComparisonUnit },
    { "ClearAchievementComparisonUnit", &Script_ClearAchievementComparisonUnit },
    { "GetAchievementComparisonInfo", &Script_GetAchievementComparisonInfo },
    { "GetPreviousAchievement", &Script_GetPreviousAchievement },
    { "GetNextAchievement", &Script_GetNextAchievement },
    { "GetAchievementCategory", &Script_GetAchievementCategory },
    { "GetAchievementLink", &Script_GetAchievementLink },
    { "GetNumCompletedAchievements", &Script_GetNumCompletedAchievements },
    { "GetNumComparisonCompletedAchievements", &Script_GetNumComparisonCompletedAchievements },
    { "GetLatestCompletedAchievements", &Script_GetLatestCompletedAchievements },
    { "GetLatestUpdatedStats", &Script_GetLatestUpdatedStats },
    { "GetLatestCompletedComparisonAchievements", &Script_GetLatestCompletedComparisonAchievements },
    { "GetLatestUpdatedComparisonStats", &Script_GetLatestUpdatedComparisonStats },
    { "GetTotalAchievementPoints", &Script_GetTotalAchievementPoints },
    { "GetAchievementInfoFromCriteria", &Script_GetAchievementInfoFromCriteria },
    { "GetStatistic", &Script_GetStatistic },
    { "GetComparisonStatistic", &Script_GetComparisonStatistic },
    { "GetComparisonAchievementPoints", &Script_GetComparisonAchievementPoints },
    { "CanShowAchievementUI", &Script_CanShowAchievementUI },
    { "GetTrackedAchievements", &Script_GetTrackedAchievements },
    { "AddTrackedAchievement", &Script_AddTrackedAchievement },
    { "RemoveTrackedAchievement", &Script_RemoveTrackedAchievement },
    { "IsTrackedAchievement", &Script_IsTrackedAchievement },
    { "GetNumTrackedAchievements", &Script_GetNumTrackedAchievements },
    { "HasCompletedAnyAchievement", &Script_HasCompletedAnyAchievement },
    { "QueryQuestsCompleted", &Script_QueryQuestsCompleted },
    { "GetQuestsCompleted", &Script_GetQuestsCompleted },
};

// VERIFY: 5d0272df auto
// OFFSET: 0x005B0D00
void AchievementInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: f7d67813 auto
// OFFSET: 0x005B0D30
void AchievementInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
