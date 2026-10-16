#include "QuestLog.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstring>
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/QuestCache.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include <ctime>
#include <cstdlib>
#include <malloc.h>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/QuestInfoRec.h"
#include "WoW/Source/DB/Rec/QuestSortRec.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/Rec/FactionRec.h"
#include "WoW/Source/DB/Rec/QuestXPRec.h"
#include "WoW/Source/Object/CreatureStats.h"
#include "WoW/Source/Object/GameObjectStats.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/QuestLogLeaderBoardParams.h"
#include "WoW/Source/Ui/WorldMap.h"
#include <cmath>
#include "WoW/Source/DB/Rec/DungeonMapRec.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/WorldMapAreaRec.h"

// STATIC: 0x00C23618
int32_t CGQuestLog::m_sortTypes[25];

// STATIC: 0x00C23AD4
uint32_t CGQuestLog::m_numSortTypes;

// STATIC: 0x00C23AEC
int32_t CGQuestLog::m_questResetTime;

// STATIC: 0x00C23AF0
uint8_t CGQuestLog::m_trackedQuestsLoaded;

// STATIC: 0x00C23AD0
int32_t CGQuestLog::m_numQuest;

// STATIC: 0x00C23AF4
static uint32_t s_questCallbackCount;

// STATIC: 0x00AD1018
float CGQuestLog::m_questIconOverlapPushDist = 18.0f;

// STATIC: 0x00C23AD8
int32_t CGQuestLog::m_selectedQuest;

// STATIC: 0x00AD1014
float CGQuestLog::m_questIconOverlapDist = 12.0f;

// STATIC: 0x00A1C4A0
static const int32_t s_questGreenRange[20] = { 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

// STATIC: 0x00C23680
QuestWatch CGQuestLog::m_questWatches[25];

// STATIC: 0x00C237B0
QuestLogEntry CGQuestLog::m_quests[50];

// STATIC: 0x00C23AE4
uint32_t CGQuestLog::m_numShownQuests;

// STATIC: 0x00C23ADC
int32_t CGQuestLog::m_abandonQuest;

// STATIC: 0x00C23AE0
uint32_t CGQuestLog::m_numQuestWatches;

// STATIC: 0x00C23AE8
int32_t CGQuestLog::m_timeOffset;

// STATIC: 0x00C23AF8
int32_t CGQuestLog::m_nextQuestUpdate;

// STATIC: 0x00C23AFC
static CVar* s_cvPOIShiftComplete;

// STATIC: 0x00C23B00
QuestPOIInfo CGQuestLog::m_questPOIs[25];

// VERIFY: 45bd143e manual
// OFFSET: 0x005E6FA0
void CGQuestLog::EnterWorld() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUERY_TIME));
    msg.Finalize();
    ClientServices::Send2(&msg);
    s_questCallbackCount = 0;
    CGQuestLog::Update(1);
}

// VERIFY: ff43712e manual
// OFFSET: 0x005E2860
int32_t CGQuestLog::GetQuestGreenRange() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    uint32_t index = static_cast<int32_t>(player->m_unit->m_unitFieldLevel) / 5;

    if (index >= 20) {
        index = 19;
    }

    return s_questGreenRange[index];
}

// VERIFY: 673a158b manual
// OFFSET: 0x005DEC40
int32_t CGQuestLog::GetQuestAt(int32_t index) {
    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader) {
        return 0;
    }

    return CGQuestLog::m_quests[index].m_questId;
}

// VERIFY: 8ecc7915 manual
// OFFSET: 0x005E71A0
void CGQuestLog::InitializeGame() {
    CGQuestLog::m_selectedQuest = 0;
    CGQuestLog::m_abandonQuest = 0;
    CGQuestLog::m_numQuest = 0;
    CGQuestLog::m_numShownQuests = 0;
    memset(CGQuestLog::m_quests, 0, sizeof(CGQuestLog::m_quests));
    CGQuestLog::m_numQuestWatches = 0;
    memset(CGQuestLog::m_questWatches, 0, sizeof(CGQuestLog::m_questWatches));
    CGQuestLog::m_timeOffset = 0;
    CGQuestLog::m_nextQuestUpdate = 0;

    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];
        poi->m_questId = 0;
        poi->m_iconMapId = 0;
        poi->m_iconObjectiveIndex = 0;
        poi->m_iconMapFloor = 0;
        poi->m_iconX = 0;
        poi->m_iconY = 0;
        poi->unused_0029 = 0;
        poi->m_visible = 0;
        poi->m_objectiveMask = 0;
    }

    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1, 0, 0, 0);
    flags.m_forceMipTracking = 1;
    flags.m_renderTarget = 1;
    ClientServices::SetMessageHandler(SMSG_QUERY_TIME_RESPONSE, Packet_SMSG_QUERY_TIME_RESPONSE, nullptr);
    s_cvPOIShiftComplete = CVar::Register("POIShiftComplete", nullptr, 0, "0.6", nullptr, 0, false, nullptr, false);
    ConsoleCommandRegister("UpdatePOI", UpdateQuestPOIHandler, CATEGORY_GAME, "Updates Quest POI from User Server");
}

// VERIFY: 988b76a4 auto
// OFFSET: 0x005DEBD0
int32_t CGQuestLog::GetSelectedLogEntry() {
    uint32_t index = 0;

    for (; index < static_cast<uint32_t>(CGQuestLog::m_numQuest); index++) {
        if (!CGQuestLog::m_quests[index].m_isHeader && CGQuestLog::m_quests[index].m_questId == CGQuestLog::m_selectedQuest) {
            break;
        }
    }

    if (index == static_cast<uint32_t>(CGQuestLog::m_numQuest)) {
        return -1;
    }

    return index;
}

// VERIFY: 17b653c2 auto
// OFFSET: 0x005DFF40
void CGQuestLog::SetServerTime(int32_t serverTime, int32_t resetTime) {
    int32_t now = static_cast<int32_t>(time(nullptr));
    CGQuestLog::m_timeOffset = now - serverTime;

    if (now == serverTime) {
        CGQuestLog::m_timeOffset = 1;
    }

    CGQuestLog::m_questResetTime = now + resetTime;
    CGQuestLog::m_nextQuestUpdate = now + 3600;

    if (now + 3600 >= now + resetTime) {
        CGQuestLog::m_nextQuestUpdate = now + resetTime;
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
}

// VERIFY: 98372535 manual
// OFFSET: 0x005DFFA0
void CGQuestLog::SetSelectedQuest(int32_t index) {
    if (CGQuestLog::m_selectedQuest) {
        DisplayQuestPoint(0.0f, 0.0f, 0, "");
    }

    if (index < 0 || index >= CGQuestLog::m_numQuest) {
        CGQuestLog::m_selectedQuest = 0;
    } else if (!CGQuestLog::m_quests[index].m_isHeader) {
        CGQuestLog::m_selectedQuest = CGQuestLog::m_quests[index].m_questId;
        AddQuestPointDisplay();
    }
}

// VERIFY: 4aaec04c manual
// OFFSET: 0x005E57E0
int32_t CGQuestLog::SetQuestExpired(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGQuestLog::m_numQuest) || CGQuestLog::IsQuestCompleteEx(index, 1)) {
        return 0;
    }

    CGQuestLog::m_quests[index].m_expired = 1;

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::QueryQuestGiverStatusMultiple();
    }

    return 1;
}

// VERIFY: 5dcc8e24 auto
// OFFSET: 0x005DE980
void DisplayQuestFailed(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
        return;
    }

    QuestCache* quest = g_questDBCache.GetRecord(id, WOWGUID(), nullptr, nullptr, false);

    if (quest) {
        CGGameUI::DisplayError(ERR_QUEST_FAILED_S, quest->m_name);
    }
}

// VERIFY: 6e014a22 auto
// OFFSET: 0x005DE930
void QuestQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
    } else {
        ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
    }
}

// VERIFY: 39afcd7d manual
// OFFSET: 0x005DED90
int32_t GetQuestItemRewards(const char* type, int32_t index) {
    if (!type || !*type || index < 0) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (!quest) {
        return 0;
    }

    if (!SStrCmpI(type, "reward", STORM_MAX_STR)) {
        if (index < 4) {
            return quest->m_rewardItem[index];
        }

        return 0;
    }

    if (SStrCmpI(type, "choice", STORM_MAX_STR) || index >= 6) {
        return 0;
    }

    return quest->m_rewardChoiceItem[index];
}

// VERIFY: 88d84110 auto
// OFFSET: 0x005DEE30
int32_t CGQuestLog::IsQuestFailed(uint32_t index) {
    return index < static_cast<uint32_t>(CGQuestLog::m_numQuest) && !CGQuestLog::m_quests[index].m_isHeader && CGQuestLog::m_quests[index].m_expired;
}

// VERIFY: 1f30d3fb manual
// OFFSET: 0x005DEEB0
int32_t CGQuestLog::GetQuestIndex(int32_t questId) {
    for (int32_t i = 0; i < 50; i++) {
        if (!CGQuestLog::m_quests[i].m_isHeader && CGQuestLog::m_quests[i].m_questId == questId) {
            return i + 1;
        }
    }

    return 0;
}

// VERIFY: 728ea3e9 manual
// OFFSET: 0x005E0EA0
int32_t CGQuestLog::IsQuestCompleteEx(uint32_t index, int32_t a2) {
    if (index >= static_cast<uint32_t>(CGQuestLog::m_numQuest) || CGQuestLog::m_quests[index].m_isHeader) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t slot = CGQuestLog::m_quests[index].m_logSlot;
    PlayerQuestLogEntry* entry = slot >= 0 && slot <= 25 ? &player->m_player->m_playerQuestLog[slot] : nullptr;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, QuestQueryCallback, nullptr, false);

    if (!quest || !entry || (entry->m_state & 2) || CGQuestLog::IsQuestFailed(index)) {
        return 0;
    }

    uint32_t special = quest->m_flags & 6;

    if (!special && !quest->m_friendlyFactionId && !quest->m_hostileFactionId && !quest->m_rewardMoney) {
        int32_t hasObjectives = 0;

        for (uint32_t i = 0; i < 4; i++) {
            if (quest->m_objectiveId[i]) {
                hasObjectives = 1;
                break;
            }
        }

        for (uint32_t i = 0; i < 6; i++) {
            if (quest->m_collectItemId[i]) {
                hasObjectives = 1;
                break;
            }
        }

        if (a2 && !hasObjectives && !quest->m_requiredPlayersKilled) {
            return 0;
        }
    }

    if (special && !(entry->m_state & 1)) {
        return 0;
    }

    if (quest->m_friendlyFactionId && GetRepListRepValue(quest->m_friendlyFactionId) < static_cast<int32_t>(quest->m_friendlyFactionAmount)) {
        return 0;
    }

    if (quest->m_hostileFactionId && GetRepListRepValue(quest->m_hostileFactionId) > static_cast<int32_t>(quest->m_hostileFactionAmount)) {
        return 0;
    }

    if (quest->m_rewardMoney < 0 && player->GetCoinage() < static_cast<uint32_t>(-quest->m_rewardMoney)) {
        return 0;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (quest->m_objectiveId[i] && entry->m_counts[i] < quest->m_objectiveRequiredCount[i]) {
            return 0;
        }
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (quest->m_collectItemId[i] && player->m_bag.GetItemTypeCount(quest->m_collectItemId[i], 8) < static_cast<int32_t>(quest->m_collectItemCount[i])) {
            return 0;
        }
    }

    return static_cast<int32_t>(entry->m_counts[0]) >= static_cast<int32_t>(quest->m_requiredPlayersKilled);
}

// VERIFY: 7a39ad3c manual
// OFFSET: 0x005E10B0
bool CGQuestLog::HasNoObjectives(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGQuestLog::m_numQuest) || CGQuestLog::m_quests[index].m_isHeader) {
        return false;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    int32_t slot = CGQuestLog::m_quests[index].m_logSlot;
    PlayerQuestLogEntry* entry = static_cast<uint32_t>(slot) <= 25 ? &player->m_player->m_playerQuestLog[slot] : nullptr;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, QuestQueryCallback, nullptr, false);

    if (!quest || !entry || (entry->m_state & 2) || CGQuestLog::IsQuestFailed(index)) {
        return false;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (quest->m_objectiveId[i]) {
            return false;
        }
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (quest->m_collectItemId[i] && (quest->m_collectItemId[i] != quest->m_startingItemId || static_cast<int32_t>(quest->m_collectItemCount[i]) > 1)) {
            return false;
        }
    }

    return !quest->m_requiredPlayersKilled && !(quest->m_flags & 6) && !quest->m_friendlyFactionId && !quest->m_hostileFactionId && quest->m_rewardMoney >= 0;
}

// VERIFY: e4a82c9a manual
// OFFSET: 0x005DED30
int32_t GetQuestSuggestedPlayers(int32_t index) {
    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, QuestQueryCallback, nullptr, false);

    if (!quest) {
        return 0;
    }

    return quest->m_suggestedPlayers;
}

// VERIFY: 00b5ee18 manual
// OFFSET: 0x005E0CC0
void CGQuestLog::AbandonSelectedQuest() {
    if (!CGQuestLog::m_abandonQuest || !ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        return;
    }

    for (int32_t i = 0; i < 50; i++) {
        if (!CGQuestLog::m_quests[i].m_isHeader && CGQuestLog::m_quests[i].m_questId == CGQuestLog::m_abandonQuest) {
            CGPlayer_C::QuestLogRemoveQuest(static_cast<uint8_t>(CGQuestLog::m_quests[i].m_logSlot));
            return;
        }
    }
}

// VERIFY: 45625162 manual
// OFFSET: 0x005E1300
int32_t GetNumQuestLeaderBoards(int32_t questId, void (*callback)(int32_t id, const WOWGUID& guid, void* param, bool success), void* param) {
    int32_t count = 0;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, callback, param, false);

    if (quest) {
        if (quest->m_endText[0]) {
            count = 1;
        }

        for (uint32_t i = 0; i < 4; i++) {
            if (quest->m_objectiveId[i]) {
                count++;
            }
        }

        for (uint32_t i = 0; i < 6; i++) {
            if (static_cast<int32_t>(quest->m_collectItemId[i]) > 0 && (quest->m_collectItemId[i] != quest->m_startingItemId || static_cast<int32_t>(quest->m_collectItemCount[i]) > 1)) {
                count++;
            }
        }

        if (quest->m_friendlyFactionId) {
            count++;
        }

        if (quest->m_requiredPlayersKilled) {
            count++;
        }

        if (count) {
            return count;
        }
    }

    return CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(questId) - 1, 1) ? 1 : count;
}

// VERIFY: 390e816f manual
// OFFSET: 0x005E1FF0
int32_t CountQuestItemDropsNeeded(int32_t questId, void (*callback)(int32_t id, const WOWGUID& guid, void* param, bool success), void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t count = 0;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, callback, param, false);

    if (quest) {
        for (uint32_t i = 0; i < 4; i++) {
            int32_t itemId = quest->m_intermediateItemId[i];

            if (itemId > 0 && (static_cast<uint32_t>(itemId) != quest->m_startingItemId || static_cast<int32_t>(quest->m_intermediateItemCount[i]) > 1) && !player->m_bag.GetItemTypeCount(itemId, 8)) {
                count++;
            }
        }
    }

    return count;
}

// VERIFY: 89b5e6a8 manual
// OFFSET: 0x005E64D0
int32_t CGQuestLog::AddQuestWatch(int32_t questId, int32_t duration) {
    int32_t now = static_cast<int32_t>(time(nullptr));
    int32_t i;

    for (i = 0; i < 50; i++) {
        if (!CGQuestLog::m_quests[i].m_isHeader && CGQuestLog::m_quests[i].m_questId == questId) {
            break;
        }
    }

    if (i >= 50) {
        return now;
    }

    for (uint32_t j = 0; j < CGQuestLog::m_numQuestWatches; j++) {
        if (CGQuestLog::m_questWatches[j].m_questId == questId) {
            if (CGQuestLog::m_questWatches[j].m_expireTime > 0) {
                CGQuestLog::m_questWatches[j].m_expireTime = duration ? now + duration : 0;
            }

            return now;
        }
    }

    if (CGQuestLog::m_numQuestWatches < 25) {
        uint32_t index = CGQuestLog::m_numQuestWatches;
        CGQuestLog::m_questWatches[index].m_questId = questId;
        CGQuestLog::m_questWatches[index].m_expireTime = duration ? duration + now : 0;
        CGQuestLog::m_numQuestWatches = index + 1;
        return CGQuestLog::SortQuestWatches();
    }

    return now;
}

// VERIFY: b5dd4e6f auto
// OFFSET: 0x005DE8F0
static void QuestQueryCounterCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
        return;
    }

    if (s_questCallbackCount == 0 || --s_questCallbackCount == 0) {
        CGQuestLog::Update(1);
    }
}

// VERIFY: 51715ead manual
// OFFSET: 0x005DEA60
void CGQuestLog::RemoveQuestWatch(int32_t questId) {
    uint32_t count = CGQuestLog::m_numQuestWatches;
    uint32_t i;

    if (!count) {
        return;
    }

    for (i = 0; CGQuestLog::m_questWatches[i].m_questId != questId; i++) {
        if (i + 1 >= CGQuestLog::m_numQuestWatches) {
            return;
        }
    }

    for (; i < CGQuestLog::m_numQuestWatches - 1; i++) {
        CGQuestLog::m_questWatches[i].m_questId = CGQuestLog::m_questWatches[i + 1].m_questId;
        CGQuestLog::m_questWatches[i].m_expireTime = CGQuestLog::m_questWatches[i + 1].m_expireTime;
    }

    CGQuestLog::m_questWatches[count - 1].m_questId = 0;
    CGQuestLog::m_numQuestWatches = count - 1;
    CGQuestLog::m_questWatches[count - 1].m_expireTime = 0;
}

// VERIFY: cedecb8a auto
// OFFSET: 0x005DEAE0
int32_t GetQuestSortId(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGQuestLog::m_numQuest)) {
        return -1;
    }

    int32_t sortType = CGQuestLog::m_quests[index].m_questId;

    if (!CGQuestLog::m_quests[index].m_isHeader) {
        WOWGUID none = { 0 };
        QuestCache* quest = g_questDBCache.GetRecord(sortType, none, nullptr, nullptr, false);

        if (!quest) {
            return -1;
        }

        sortType = quest->m_areaIdOrSortId;
    }

    for (uint32_t i = 0; i < CGQuestLog::m_numSortTypes; i++) {
        if (CGQuestLog::m_sortTypes[i] == sortType) {
            return i;
        }
    }

    return -1;
}

// VERIFY: d0925147 manual
// OFFSET: 0x005DEC70
const char* GetQuestTitleText(int32_t questId) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, QuestQueryCallback, nullptr, false);

    if (!quest) {
        return nullptr;
    }

    return quest->m_name;
}

// VERIFY: f8dc9db0 manual
// OFFSET: 0x005DECC0
uint32_t IsDailyQuest(int32_t index) {
    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, QuestQueryCallback, nullptr, false);

    if (!quest) {
        return 0;
    }

    return quest->m_flags & 0x1000;
}

// VERIFY: d701767c manual
// OFFSET: 0x005DEEE0
bool CGQuestLog::GetQuestPOIIcon(int32_t questId, bool* complete, int32_t* mapId, uint32_t* mapFloor, int32_t* x, int32_t* y, int32_t* objective) {
    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];

        if (poi->m_questId == questId) {
            *complete = poi->m_objectiveMask == 0xFFFFFFFF;
            *mapId = poi->m_iconMapId;
            *mapFloor = poi->m_iconMapFloor;
            *x = poi->m_iconX;
            *y = poi->m_iconY;
            *objective = poi->m_iconObjectiveIndex;
            return true;
        }
    }

    return false;
}

// VERIFY: e94a9f5a manual
// OFFSET: 0x005DEF60
void CGQuestLog::ShiftQuestWatches(uint32_t from, uint32_t to) {
    int32_t step;

    if (to > from && to < CGQuestLog::m_numQuestWatches) {
        step = 1;
    } else if (to < from && from < CGQuestLog::m_numQuestWatches) {
        step = -1;
    } else {
        return;
    }

    QuestWatch moved = CGQuestLog::m_questWatches[from];

    for (uint32_t i = from; i != to; i += step) {
        CGQuestLog::m_questWatches[i] = CGQuestLog::m_questWatches[i + step];
    }

    CGQuestLog::m_questWatches[to] = moved;
}

// VERIFY: db83baeb manual
// OFFSET: 0x005DFDC0
static int32_t QSortQuestSortTypes(const void* a, const void* b) {
    int32_t sortA = *static_cast<const int32_t*>(a);
    int32_t sortB = *static_cast<const int32_t*>(b);

    if (sortA == sortB) {
        return 0;
    }

    if (!sortA) {
        return -1;
    }

    if (!sortB) {
        return 1;
    }

    char nameA[256] = { 0 };
    char nameB[256] = { 0 };

    if (sortA >= 0) {
        AreaTableRec* area = g_areaTableDB.GetRecord(sortA);

        if (area) {
            SStrCopy(nameA, area->m_areaName, sizeof(nameA));
        }
    } else {
        QuestSortRec* sort = g_questSortDB.GetRecord(-sortA);

        if (sort) {
            SStrCopy(nameA, sort->m_sortName, sizeof(nameA));
        }
    }

    if (sortB >= 0) {
        AreaTableRec* area = g_areaTableDB.GetRecord(sortB);

        if (area) {
            SStrCopy(nameB, area->m_areaName, sizeof(nameB));
        }
    } else {
        QuestSortRec* sort = g_questSortDB.GetRecord(-sortB);

        if (sort) {
            SStrCopy(nameB, sort->m_sortName, sizeof(nameB));
        }
    }

    return SStrSortUTF8I(nameA, nameB, STORM_MAX_STR);
}

// VERIFY: 651d4dbc manual
// OFFSET: 0x005E0000
const char* GetQuestTitle(int32_t index) {
    if (index < 0 || index >= CGQuestLog::m_numQuest) {
        return nullptr;
    }

    int32_t id = CGQuestLog::m_quests[index].m_questId;

    if (!CGQuestLog::m_quests[index].m_isHeader) {
        return GetQuestTitleText(id);
    }

    if (id > 0) {
        AreaTableRec* area = g_areaTableDB.GetRecord(id);

        if (area) {
            return area->m_areaName;
        }
    } else if (id < 0) {
        QuestSortRec* sort = g_questSortDB.GetRecord(-id);

        if (sort) {
            return sort->m_sortName;
        }
    } else {
        return "Missing header! (quest designers)";
    }

    return nullptr;
}

// VERIFY: 12e6b2ec manual
// OFFSET: 0x005E0070
const char* GetQuestTag(int32_t index) {
    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader) {
        return nullptr;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, QuestQueryCallback, nullptr, false);
    QuestInfoRec* info;

    if (!quest || !(info = g_questInfoDB.GetRecord(quest->m_infoId))) {
        return nullptr;
    }

    return info->m_infoName;
}

// VERIFY: 074af98c auto
// OFFSET: 0x005E00E0
int32_t CGQuestLog::IsSelectedQuestExpired() {
    uint32_t index = CGQuestLog::GetSelectedLogEntry();
    return index < static_cast<uint32_t>(CGQuestLog::m_numQuest) && !CGQuestLog::m_quests[index].m_isHeader && CGQuestLog::m_quests[index].m_expired;
}

// VERIFY: e7d44bdf auto
// OFFSET: 0x005E08A0
int32_t Packet_SMSG_QUERY_TIME_RESPONSE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t serverTime = 0;
    uint32_t resetTime;
    msg->Get(serverTime);
    msg->Get(resetTime);
    CGQuestLog::SetServerTime(serverTime, resetTime);
    return 1;
}

// VERIFY: f903a683 manual
// OFFSET: 0x005E08F0
static int32_t QSortQuestLogEntries(const void* a, const void* b) {
    QuestLogEntry entryA = *static_cast<const QuestLogEntry*>(a);
    QuestLogEntry entryB = *static_cast<const QuestLogEntry*>(b);
    int32_t idA = entryA.m_questId;
    int32_t idB = entryB.m_questId;
    int32_t headerA = entryA.m_isHeader;
    int32_t headerB = entryB.m_isHeader;
    WOWGUID none = { 0 };
    QuestCache* questA = headerA ? nullptr : g_questDBCache.GetRecord(idA, none, nullptr, nullptr, false);
    QuestCache* questB = headerB ? nullptr : g_questDBCache.GetRecord(idB, none, nullptr, nullptr, false);
    uint32_t sortA = 0;
    uint32_t sortB;
    uint32_t i;

    if (headerA) {
        for (i = 0; i < 25 && CGQuestLog::m_sortTypes[i] != idA; i++) {
        }
    } else {
        if (!questA) {
            return 0;
        }

        for (i = 0; i < 25 && CGQuestLog::m_sortTypes[i] != static_cast<int32_t>(questA->m_areaIdOrSortId); i++) {
        }
    }

    if (i < 25) {
        sortA = i;
    }

    if (headerB) {
        for (i = 0; i < 25 && CGQuestLog::m_sortTypes[i] != idB; i++) {
        }
    } else {
        if (!questB) {
            return 0;
        }

        for (i = 0; i < 25 && CGQuestLog::m_sortTypes[i] != static_cast<int32_t>(questB->m_areaIdOrSortId); i++) {
        }
    }

    sortB = i < 25 ? i : 0;
    int32_t hiddenA = !headerA && sortA < 25 && !((1 << sortA) & s_cvQuestLogCollapseFilter->m_intValue);
    int32_t hiddenB = !headerB && sortB < 25 && !((1 << sortB) & s_cvQuestLogCollapseFilter->m_intValue);

    if (hiddenA) {
        if (hiddenB) {
            return 0;
        }

        return 2 * hiddenA - 1;
    }

    if (hiddenB) {
        return 2 * hiddenA - 1;
    }

    if (sortA != sortB) {
        return sortA < sortB ? -1 : 1;
    }

    if (headerA) {
        return -1;
    }

    if (headerB) {
        return 1;
    }

    int32_t levelA = questA->m_level;
    int32_t levelB = questB->m_level;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        if (levelA == -1) {
            levelA = player->m_unit->m_unitFieldLevel;
        }

        if (levelB == -1) {
            levelB = player->m_unit->m_unitFieldLevel;
        }
    }

    if (levelA == levelB) {
        return SStrSortUTF8I(questA->m_name, questB->m_name, STORM_MAX_STR);
    }

    return 2 * (levelA >= levelB) - 1;
}

// VERIFY: 01ac7701 auto
// OFFSET: 0x005E0B80
void CGQuestLog::FilterAndSortQuests() {
    qsort(CGQuestLog::m_quests, CGQuestLog::m_numQuest, sizeof(QuestLogEntry), QSortQuestLogEntries);
    CGQuestLog::m_numShownQuests = CGQuestLog::m_numQuest;

    for (uint32_t i = 0; i < static_cast<uint32_t>(CGQuestLog::m_numQuest); i++) {
        if (CGQuestLog::m_quests[i].m_isHeader) {
            continue;
        }

        uint32_t sortId = GetQuestSortId(i);

        if (sortId < 25 && !((1 << sortId) & s_cvQuestLogCollapseFilter->m_intValue)) {
            CGQuestLog::m_numShownQuests--;
        }
    }
}

// VERIFY: 8af272a7 auto
// OFFSET: 0x005E0C00
void CGQuestLog::CollapseHeader(uint32_t index, int32_t collapse) {
    if (index < static_cast<uint32_t>(CGQuestLog::m_numQuest) && CGQuestLog::m_quests[index].m_isHeader) {
        uint32_t i;

        for (i = 0; i < CGQuestLog::m_numSortTypes; i++) {
            if (CGQuestLog::m_sortTypes[i] == CGQuestLog::m_quests[index].m_questId) {
                break;
            }
        }

        if (i == CGQuestLog::m_numSortTypes) {
            return;
        }

        int32_t filter = s_cvQuestLogCollapseFilter->m_intValue;
        int32_t bit = 1 << i;
        s_cvQuestLogCollapseFilter->Set(collapse ? filter & ~bit : filter | bit, true, false, false, true);
    } else {
        s_cvQuestLogCollapseFilter->Set((collapse != 0) - 1, true, false, false, true);
    }

    CGQuestLog::FilterAndSortQuests();
    Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
}

// VERIFY: 89bc5f8b manual
// OFFSET: 0x005E0D20
int32_t GetQuestLevel(int32_t questId) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, QuestQueryCallback, nullptr, false);

    if (!quest) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && quest->m_level == 0xFFFFFFFF) {
        return player->m_unit->m_unitFieldLevel;
    }

    return quest->m_level;
}

// VERIFY: 92305a9d auto
// OFFSET: 0x005E0DA0
CGItem_C* CGQuestLog::GetQuestSpecialItem(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGQuestLog::m_numQuest) || CGQuestLog::m_quests[index].m_isHeader) {
        return nullptr;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, QuestQueryCallback, nullptr, false);

    if (!quest || (!quest->m_startingItemId && !(quest->m_flags & 0x20000))) {
        return nullptr;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return nullptr;
    }

    CGBag_C* bag = &player->m_bag;
    CGItem_C* item = bag->FindItemOfType(quest->m_startingItemId, 0);

    if (item && item->CanBeUsed()) {
        return item;
    }

    if (!(quest->m_flags & 0x20000)) {
        return nullptr;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (!quest->m_intermediateItemId[i]) {
            continue;
        }

        item = bag->FindItemOfType(quest->m_intermediateItemId[i], 0);

        if (item && item->CanBeUsed()) {
            return item;
        }
    }

    return nullptr;
}

// VERIFY: 716d0e51 manual
// OFFSET: 0x005E28B0
bool CGQuestLog::HasQuestPOI(int32_t questId) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__) || questId <= 0) {
        return false;
    }

    for (uint32_t i = 0; i < 25; i++) {
        if (CGQuestLog::m_questPOIs[i].m_questId == questId) {
            return true;
        }
    }

    return false;
}

// VERIFY: 86468e46 manual
// OFFSET: 0x005E6940
void CGQuestLog::Update(int32_t a1) {
    if (!a1) {
        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
        return;
    }

    if (s_questCallbackCount) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    int32_t filter = s_cvQuestLogCollapseFilter->m_intValue;
    uint32_t numOldSortTypes = CGQuestLog::m_numSortTypes;
    int32_t* oldSortTypes = static_cast<int32_t*>(alloca(numOldSortTypes * sizeof(int32_t)));

    for (uint32_t i = 0; i < CGQuestLog::m_numSortTypes; i++) {
        oldSortTypes[i] = CGQuestLog::m_sortTypes[i];

        if ((1 << i) & filter) {
            oldSortTypes[i] = -oldSortTypes[i];
        }
    }

    memset(CGQuestLog::m_quests, 0, sizeof(CGQuestLog::m_quests));
    memset(CGQuestLog::m_sortTypes, 0, sizeof(CGQuestLog::m_sortTypes));
    CGQuestLog::m_numQuest = 0;
    CGQuestLog::m_numSortTypes = 0;
    s_questCallbackCount = 0;
    int32_t numUnknown = 0;
    int32_t unknown[25] = { 0 };
    int32_t timeOffset = CGQuestLog::m_timeOffset;

    for (uint32_t slot = 0; slot < 25; slot++) {
        PlayerQuestLogEntry* entry = slot > 25 ? nullptr : &player->m_player->m_playerQuestLog[slot];
        int32_t questId = entry->m_questId;

        if (questId <= 0) {
            continue;
        }

        WOWGUID none = { 0 };
        QuestCache* quest = g_questDBCache.GetRecord(questId, none, QuestQueryCounterCallback, nullptr, false);

        if (!quest) {
            s_questCallbackCount++;
            continue;
        }

        if (!CGQuestLog::HasQuestPOI(questId)) {
            numUnknown++;
            unknown[slot] = questId;
        }

        CGQuestLog::m_quests[CGQuestLog::m_numQuest].m_questId = questId;
        CGQuestLog::m_quests[CGQuestLog::m_numQuest].m_logSlot = slot;
        CGQuestLog::m_numQuest++;

        if (entry->m_time && !(entry->m_state & 2) && timeOffset + static_cast<int32_t>(entry->m_time) - static_cast<int32_t>(time(nullptr)) - 1 < 0) {
            CGQuestLog::SetQuestExpired(CGQuestLog::m_numQuest - 1);
        }

        uint32_t i;

        for (i = 0; i < CGQuestLog::m_numSortTypes; i++) {
            if (CGQuestLog::m_sortTypes[i] == static_cast<int32_t>(quest->m_areaIdOrSortId)) {
                break;
            }
        }

        if (i == CGQuestLog::m_numSortTypes) {
            CGQuestLog::m_sortTypes[i] = quest->m_areaIdOrSortId;
            CGQuestLog::m_numSortTypes++;
            CGQuestLog::m_quests[CGQuestLog::m_numQuest].m_questId = quest->m_areaIdOrSortId;
            CGQuestLog::m_quests[CGQuestLog::m_numQuest].m_isHeader = 1;
            CGQuestLog::m_numQuest++;
        }
    }

    qsort(CGQuestLog::m_sortTypes, CGQuestLog::m_numSortTypes, sizeof(int32_t), QSortQuestSortTypes);

    for (uint32_t i = 0; i < CGQuestLog::m_numSortTypes; i++) {
        uint32_t j;

        for (j = 0; j < numOldSortTypes; j++) {
            if (oldSortTypes[j] == CGQuestLog::m_sortTypes[i]) {
                filter &= ~(1 << i);
                break;
            }

            if (oldSortTypes[j] == -CGQuestLog::m_sortTypes[i]) {
                filter |= 1 << i;
                break;
            }
        }

        if (j >= numOldSortTypes) {
            filter |= 1 << i;
        }
    }

    s_cvQuestLogCollapseFilter->Set(filter, true, false, false, true);
    CGQuestLog::FilterAndSortQuests();

    if (!CGQuestLog::m_trackedQuestsLoaded && CGQuestLog::m_numQuest) {
        CGQuestLog::LoadTrackedQuests();
    }

    bool found = true;

    for (uint32_t i = 0; i < CGQuestLog::m_numQuestWatches; i++) {
        if (!found) {
            i--;
        }

        found = false;

        for (int32_t j = 0; j < CGQuestLog::m_numQuest; j++) {
            if (!CGQuestLog::m_quests[j].m_isHeader && CGQuestLog::m_quests[j].m_questId == CGQuestLog::m_questWatches[i].m_questId) {
                found = true;
                break;
            }
        }

        if (!found) {
            CGQuestLog::RemoveQuestWatch(CGQuestLog::m_questWatches[i].m_questId);
        }
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);

    if (numUnknown) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_QUEST_POI_QUERY));
        msg.Put(static_cast<uint32_t>(numUnknown));

        for (uint32_t i = 0; i < 25; i++) {
            if (unknown[i]) {
                msg.Put(static_cast<uint32_t>(unknown[i]));
            }
        }

        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (CGQuestLog::m_nextQuestUpdate && static_cast<int32_t>(time(nullptr)) >= CGQuestLog::m_nextQuestUpdate) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_QUERY_TIME));
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGQuestLog::m_nextQuestUpdate = 0;
    }
}

// VERIFY: b7033214 manual
// OFFSET: 0x005DE960
static void QuestSelectQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        AddQuestPointDisplay();
    } else {
        ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
    }
}

// VERIFY: 428ace85 auto
// OFFSET: 0x005DE9E0
void QuestCreatureQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
    } else {
        ConsoleWrite("Invalid creature in quest", DEFAULT_COLOR);
    }
}

// VERIFY: d89748b4 auto
// OFFSET: 0x005DEA10
void ObjectQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
    } else {
        ConsoleWrite("Invalid object in quest", DEFAULT_COLOR);
    }
}

// VERIFY: 3a839b20 auto
// OFFSET: 0x005DEA40
void ItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
    }
}

// VERIFY: 594f69d5 auto
// OFFSET: 0x005DEB60
void AddQuestPointDisplay() {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, QuestSelectQueryCallback, nullptr, false);

    if (quest) {
        DisplayQuestPoint(quest->m_pointX, quest->m_pointY, quest->m_pointOptional, quest->m_name);
    }
}

// VERIFY: 7928c50e manual
// OFFSET: 0x005DE8C0
int32_t CGQuestLog::GetQuestLogSlot(int32_t index) {
    if (index < 0 || index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader) {
        return -1;
    }

    return CGQuestLog::m_quests[index].m_logSlot;
}

// VERIFY: f0f3c6ea manual
// OFFSET: 0x005E0420
int32_t CalculateExperience(int32_t questId, int32_t level) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

    if (!quest) {
        return 0;
    }

    int32_t questLevel = quest->m_level;
    QuestXPRec* xp = g_questXPDB.GetRecord(questLevel != -1 ? questLevel : level);

    if (!xp) {
        return 0;
    }

    int32_t diff = 2 * ((questLevel != -1 ? questLevel : level) - level) + 20;
    int32_t factor;

    if (diff > 10) {
        factor = 10;
    } else if (diff >= 1) {
        factor = diff;
    } else {
        factor = 1;
    }

    int32_t value = factor * xp->m_difficulty[quest->m_xPId] / 10;

    if (value <= 100) {
        return 5 * ((value + 2) / 5);
    }

    if (value <= 500) {
        return 10 * ((value + 5) / 10);
    }

    if (value <= 1000) {
        return 25 * ((value + 12) / 25);
    }

    return 50 * ((value + 25) / 50);
}

// VERIFY: 0b7beaf9 manual
// OFFSET: 0x005E11D0
int32_t CGQuestLog::IsOnQuest(int32_t questId) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player || (player->m_modelFlags & 0x10000)) {
        return 0;
    }

    for (uint32_t slot = 0; slot < 25; slot++) {
        PlayerQuestLogEntry* entry = slot > 25 ? nullptr : &player->m_player->m_playerQuestLog[slot];

        if (entry && entry->m_questId && static_cast<int32_t>(entry->m_questId) == questId) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: c20187b5 manual
// OFFSET: 0x005E1460
int32_t CGQuestLog::GetQuestLeaderBoardByID(int32_t questId, QuestLogLeaderBoardParams* params) {
    if (!params) {
        return 0;
    }

    char* text = params->m_text;
    params->m_text[0] = 0;
    char* type = params->m_type;
    params->m_type[0] = 0;
    params->m_complete = 0;
    int32_t showProgress = params->m_showProgress;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    if (!questId) {
        questId = CGQuestLog::m_selectedQuest;
    }

    int32_t requireLog = params->m_requireLogEntry;
    int32_t slot = -1;

    if (requireLog) {
        int32_t index = CGQuestLog::GetQuestIndex(questId) - 1;

        if (index < 0) {
            int32_t selected = CGQuestLog::GetSelectedLogEntry();
            slot = selected < 0 ? 0 : CGQuestLog::m_quests[selected].m_logSlot;
        } else {
            slot = CGQuestLog::GetQuestLogSlot(index);
        }
    }

    if (questId <= 0 || (requireLog && slot < 0)) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, params->m_questCallback, params->m_param, false);
    PlayerQuestLogEntry* entry = static_cast<uint32_t>(slot) > 25 ? nullptr : &player->m_player->m_playerQuestLog[slot];

    if (!quest || (params->m_requireLogEntry && !entry)) {
        return 0;
    }

    int32_t found = -1;

    if (quest->m_endText[0]) {
        found = 0;

        if (!params->m_index) {
            SStrCopy(text, quest->m_endText, STORM_MAX_STR);
            SStrCopy(type, "event", STORM_MAX_STR);

            if (entry) {
                params->m_complete = entry->m_state & 1;
            }

            return 1;
        }
    }

    char buffer[4096];

    for (uint32_t i = 0; found < params->m_index && i < 4; i++) {
        if (!quest->m_objectiveId[i]) {
            if (params->m_countEmpty) {
                found++;
            }

            continue;
        }

        if (++found != params->m_index) {
            continue;
        }

        int32_t count = entry ? entry->m_counts[i] : 0;
        int32_t required = quest->m_objectiveRequiredCount[i];
        int32_t objective = quest->m_objectiveId[i];
        const char* objectiveText = quest->m_objectiveText[i];

        if (objectiveText[0]) {
            if (showProgress) {
                SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_OBJECTS_FOUND", -1, GENDER_NOT_APPLICABLE), objectiveText, count, required);
                params->m_complete = count >= required;
            } else {
                SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_OBJECTS_FOUND_NOPROGRESS", -1, GENDER_NOT_APPLICABLE), objectiveText, required);
            }
        } else if (objective >= 0) {
            CreatureStats_C* creature = g_creatureDBCache.GetRecord(objective, none, params->m_creatureCallback, params->m_param, false);
            uint32_t plural = FrameScript_GetPluralIndex(required);

            if (showProgress) {
                const char* format = FrameScript_GetText("QUEST_MONSTERS_KILLED", -1, GENDER_NOT_APPLICABLE);
                SStrPrintf(buffer, sizeof(buffer), format, creature ? creature->GetName(plural) : " ", count, required);
                params->m_complete = count >= required;
            } else {
                const char* format = FrameScript_GetText("QUEST_MONSTERS_KILLED_NOPROGRESS", -1, GENDER_NOT_APPLICABLE);
                SStrPrintf(buffer, sizeof(buffer), format, creature ? creature->GetName(plural) : " ", required);
            }
        } else {
            GameObjectStats_C* object = g_gameObjectDBCache.GetRecord(objective & 0x7FFFFFFF, none, params->m_objectCallback, params->m_param, false);
            uint32_t plural = FrameScript_GetPluralIndex(required);

            if (showProgress) {
                const char* format = FrameScript_GetText("QUEST_OBJECTS_FOUND", -1, GENDER_NOT_APPLICABLE);
                SStrPrintf(buffer, sizeof(buffer), format, object ? object->GetName(plural) : " ", count, required);
                params->m_complete = count >= required;
            } else {
                const char* format = FrameScript_GetText("QUEST_OBJECTS_FOUND_NOPROGRESS", -1, GENDER_NOT_APPLICABLE);
                SStrPrintf(buffer, sizeof(buffer), format, object ? object->GetName(plural) : " ", required);
            }
        }

        SStrCopy(text, buffer, STORM_MAX_STR);
        SStrCopy(type, quest->m_objectiveId[i] >= 0 ? "monster" : "object", STORM_MAX_STR);
        return 1;
    }

    for (uint32_t i = 0; i < 6 && found < params->m_index; i++) {
        int32_t itemId = quest->m_collectItemId[i];

        if (itemId <= 0 || (static_cast<uint32_t>(itemId) == quest->m_startingItemId && static_cast<int32_t>(quest->m_collectItemCount[i]) <= 1)) {
            if (params->m_countEmpty) {
                found++;
            }

            continue;
        }

        if (++found != params->m_index) {
            continue;
        }

        ItemStats_C* item = g_itemDBCache.GetRecord(itemId, none, params->m_itemCallback, params->m_param, false);
        int32_t required = quest->m_collectItemCount[i];
        uint32_t plural = FrameScript_GetPluralIndex(required);

        if (showProgress) {
            int32_t have = required;

            if (player->m_bag.GetItemTypeCount(itemId, 8) < required) {
                have = player->m_bag.GetItemTypeCount(itemId, 8);
            }

            const char* format = FrameScript_GetText("QUEST_ITEMS_NEEDED", -1, GENDER_NOT_APPLICABLE);
            SStrPrintf(buffer, sizeof(buffer), format, item ? item->GetName(plural) : " ", have, required);
            params->m_complete = have >= required;
        } else {
            const char* format = FrameScript_GetText("QUEST_ITEMS_NEEDED_NOPROGRESS", -1, GENDER_NOT_APPLICABLE);
            SStrPrintf(buffer, sizeof(buffer), format, item ? item->GetName(plural) : " ", required);
        }

        SStrCopy(text, buffer, STORM_MAX_STR);
        SStrCopy(type, "item", STORM_MAX_STR);
        return 1;
    }

    if (!CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(questId) - 1, 1)) {
        for (uint32_t i = 0; found < params->m_index && i < 4; i++) {
            int32_t itemId = quest->m_intermediateItemId[i];

            if (itemId <= 0) {
                if (params->m_countEmpty) {
                    found++;
                }

                continue;
            }

            if (++found != params->m_index) {
                continue;
            }

            ItemStats_C* item = g_itemDBCache.GetRecord(itemId, none, params->m_itemCallback, params->m_param, false);
            uint32_t plural = FrameScript_GetPluralIndex(1);
            int32_t have = player->m_bag.GetItemTypeCount(itemId, 8);
            const char* format = FrameScript_GetText("QUEST_INTERMEDIATE_ITEMS_NEEDED", -1, GENDER_NOT_APPLICABLE);
            const char* name;

            if (item) {
                name = item->m_name[plural];

                if (!name || !*name) {
                    name = item->m_name[0];
                }
            } else {
                name = " ";
            }

            SStrPrintf(buffer, sizeof(buffer), format, name, have);
            params->m_complete = have >= 1;
            SStrCopy(text, buffer, STORM_MAX_STR);
            SStrCopy(type, "item", STORM_MAX_STR);
            return 1;
        }
    }

    if (quest->m_requiredPlayersKilled) {
        int32_t count = entry ? entry->m_counts[0] : 0;

        if (showProgress) {
            SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_PLAYERS_KILLED", -1, GENDER_NOT_APPLICABLE), count, quest->m_requiredPlayersKilled);
            params->m_complete = count >= static_cast<int32_t>(quest->m_requiredPlayersKilled);
        } else {
            SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_PLAYERS_KILLED_NOPROGRESS", -1, GENDER_NOT_APPLICABLE), quest->m_requiredPlayersKilled);
        }

        SStrCopy(text, buffer, STORM_MAX_STR);
        SStrCopy(type, "player", STORM_MAX_STR);
        return 1;
    }

    FactionRec* faction;

    if (quest->m_friendlyFactionId && (faction = g_factionDB.GetRecord(quest->m_friendlyFactionId))) {
        int32_t amount = quest->m_friendlyFactionAmount;
        int32_t rank;

        if (amount >= 42000) {
            rank = 7;
        } else if (amount >= 21000) {
            rank = 6;
        } else if (amount >= 9000) {
            rank = 5;
        } else if (amount >= 3000) {
            rank = 4;
        } else if (amount >= 0) {
            rank = 3;
        } else if (amount >= -3000) {
            rank = 2;
        } else {
            rank = amount >= -6000;
        }

        char needed[64];
        char current[64];
        SStrPrintf(buffer, sizeof(buffer), "FACTION_STANDING_LABEL%d", rank + 1);
        SStrCopy(needed, player->GetFrameScriptText(buffer, -1), sizeof(needed));

        if (showProgress) {
            SStrPrintf(buffer, sizeof(buffer), "FACTION_STANDING_LABEL%d", CGReputationInfo::GetFactionStandingReaction(quest->m_friendlyFactionId) + 1);
            SStrCopy(current, player->GetFrameScriptText(buffer, -1), sizeof(current));
            SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_FACTION_NEEDED", -1, GENDER_NOT_APPLICABLE), faction->m_name, current, needed);
            params->m_complete = GetRepListRepValue(quest->m_friendlyFactionId) >= amount;
        } else {
            SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_FACTION_NEEDED_NOPROGRESS", -1, GENDER_NOT_APPLICABLE), faction->m_name, needed);
        }

        SStrCopy(text, buffer, STORM_MAX_STR);
        SStrCopy(type, "reputation", STORM_MAX_STR);
        return 1;
    }

    if (!CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(questId) - 1, 1)) {
        return 0;
    }

    params->m_complete = 1;
    char objectives[3000];
    WOWGUID playerGuid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(quest->m_objText, objectives, sizeof(objectives), playerGuid, nullptr, 0);
    SStrCopy(text, objectives, STORM_MAX_STR);
    SStrCopy(type, "log", STORM_MAX_STR);
    return 1;
}

// VERIFY: 964f20dc manual
// OFFSET: 0x005E20A0
int32_t CGQuestLog::GetQuestLogItemDropByID(int32_t questId, QuestLogLeaderBoardParams* params) {
    if (!params) {
        return 0;
    }

    char* text = params->m_text;
    params->m_text[0] = 0;
    char* type = params->m_type;
    params->m_type[0] = 0;
    params->m_complete = 0;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (!questId) {
        questId = CGQuestLog::m_selectedQuest;
    }

    int32_t requireLog = params->m_requireLogEntry;
    int32_t slot = -1;

    if (requireLog) {
        int32_t index = CGQuestLog::GetQuestIndex(questId) - 1;

        if (index < 0) {
            int32_t selected = CGQuestLog::GetSelectedLogEntry();
            slot = selected < 0 ? 0 : CGQuestLog::m_quests[selected].m_logSlot;
        } else {
            slot = CGQuestLog::GetQuestLogSlot(index);
        }
    }

    if (questId <= 0 || (requireLog && slot < 0)) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, params->m_questCallback, params->m_param, false);
    PlayerQuestLogEntry* entry = static_cast<uint32_t>(slot) > 25 ? nullptr : &player->m_player->m_playerQuestLog[slot];

    if (!quest || (params->m_requireLogEntry && !entry)) {
        return 0;
    }

    int32_t found = -1;
    char buffer[4096];

    for (uint32_t i = 0; found < params->m_index && i < 4; i++) {
        if (static_cast<int32_t>(quest->m_intermediateItemId[i]) <= 0) {
            if (params->m_countEmpty) {
                found++;
            }

            continue;
        }

        if (++found != params->m_index) {
            continue;
        }

        WOWGUID itemGuid = { 0 };
        ItemStats_C* stats = g_itemDBCache.GetRecord(quest->m_intermediateItemId[i], itemGuid, params->m_itemCallback, params->m_param, false);
        uint32_t plural = FrameScript_GetPluralIndex(1);

        if (params->m_showProgress) {
            int32_t required = quest->m_intermediateItemCount[i];
            int32_t count = required;

            if (player->m_bag.GetItemTypeCount(quest->m_intermediateItemId[i], 8) < required) {
                count = player->m_bag.GetItemTypeCount(quest->m_intermediateItemId[i], 8);
            }

            const char* format = FrameScript_GetText("QUEST_ITEMS_NEEDED", -1, GENDER_NOT_APPLICABLE);
            SStrPrintf(buffer, sizeof(buffer), format, stats ? stats->GetName(plural) : " ", count, quest->m_intermediateItemCount[i]);
            params->m_complete = count >= static_cast<int32_t>(quest->m_intermediateItemCount[i]);
        } else {
            const char* format = FrameScript_GetText("QUEST_ITEMS_NEEDED_NOPROGRESS", -1, GENDER_NOT_APPLICABLE);
            SStrPrintf(buffer, sizeof(buffer), format, stats ? stats->GetName(plural) : " ", quest->m_intermediateItemCount[i]);
        }

        SStrCopy(text, buffer, STORM_MAX_STR);
        SStrCopy(type, "item", STORM_MAX_STR);
        return 1;
    }

    return 0;
}

// VERIFY: 12e81cb8 manual
// OFFSET: 0x005E2370
int32_t CGQuestLog::GetQuestLeaderBoardForObject(int32_t questId, int32_t objectId, QuestLogLeaderBoardParams* params) {
    if (!params) {
        return 0;
    }

    char* text = params->m_text;
    params->m_text[0] = 0;
    char* type = params->m_type;
    params->m_type[0] = 0;
    params->m_complete = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, params->m_questCallback, params->m_param, false);
    int32_t index = CGQuestLog::GetQuestIndex(questId) - 1;
    PlayerQuestLogEntry* entry = nullptr;

    if (index >= 0 && index < CGQuestLog::m_numQuest && !CGQuestLog::m_quests[index].m_isHeader) {
        uint32_t slot = CGQuestLog::m_quests[index].m_logSlot;

        if (slot <= 25) {
            entry = &player->m_player->m_playerQuestLog[slot];
        }
    }

    if (!quest || !entry) {
        return 0;
    }

    uint32_t i;

    for (i = 0; static_cast<int32_t>(quest->m_objectiveId[i]) != objectId; i++) {
        if (i + 1 >= 4) {
            return 0;
        }
    }

    int32_t count = entry->m_counts[i];
    int32_t objective = quest->m_objectiveId[i];
    const char* format = quest->m_objectiveText[i];
    char buffer[4096];

    if (objective >= 0 && !*format) {
        CreatureStats_C* creature = g_creatureDBCache.GetRecord(objective, none, params->m_creatureCallback, params->m_param, false);
        uint32_t plural = FrameScript_GetPluralIndex(quest->m_objectiveRequiredCount[i]);
        format = FrameScript_GetText("QUEST_MONSTERS_KILLED", -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(buffer, sizeof(buffer), format, creature ? creature->GetName(plural) : " ", count, quest->m_objectiveRequiredCount[i]);
    } else if (objective < 0 && !*format) {
        GameObjectStats_C* object = g_gameObjectDBCache.GetRecord(objective & 0x7FFFFFFF, none, params->m_objectCallback, params->m_param, false);
        uint32_t plural = FrameScript_GetPluralIndex(quest->m_objectiveRequiredCount[i]);
        format = FrameScript_GetText("QUEST_OBJECTS_FOUND", -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(buffer, sizeof(buffer), format, object ? object->GetName(plural) : " ", count, quest->m_objectiveRequiredCount[i]);
    } else {
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("QUEST_OBJECTS_FOUND", -1, GENDER_NOT_APPLICABLE), format, count, quest->m_objectiveRequiredCount[i]);
    }

    params->m_complete = count >= static_cast<int32_t>(quest->m_objectiveRequiredCount[i]);
    SStrCopy(text, buffer, STORM_MAX_STR);
    SStrCopy(type, quest->m_objectiveId[i] >= 0 ? "monster" : "object", STORM_MAX_STR);
    return 1;
}

// VERIFY: 82a1d9ef manual
// OFFSET: 0x005E65B0
void CGQuestLog::LoadTrackedQuests() {
    CVarIntListWrapper list;
    list.m_cvar = CVar::Lookup("trackedQuests");
    list.m_pos = 2;
    list.m_state = 0;

    for (uint32_t questId = list.GetNextValue(); questId; questId = list.GetNextValue()) {
        CGQuestLog::AddQuestWatch(questId, 0);
    }

    CGQuestLog::m_trackedQuestsLoaded = 1;

    if (list.m_state != 1) {
        list.m_buffer[0] = 'v';
        list.m_buffer[1] = 1;
        list.m_buffer[list.m_pos] = 0;
        list.m_cvar->Set(list.m_buffer, true, false, false, true);
    }
}

// VERIFY: 0b80614f manual
// OFFSET: 0x005E7020
int32_t CGQuestLog::FindOrFreePOISlot(int32_t questId) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__) || questId <= 0) {
        return -1;
    }

    for (int32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];

        if (poi->m_questId == questId) {
            return i;
        }

        if (!CGQuestLog::IsOnQuest(poi->m_questId)) {
            poi->m_questId = 0;
            poi->m_blobs.Clear();
            return i;
        }
    }

    return -1;
}

// VERIFY: 89be692d manual
// OFFSET: 0x005E7370
void CGQuestLog::HandlePOIQueryResponse(CDataStore* msg) {
    if (!msg) {
        return;
    }

    uint32_t count;
    msg->Get(count);

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        int32_t questId;
        msg->Get(reinterpret_cast<uint32_t&>(questId));

        uint32_t numBlobs;
        msg->Get(numBlobs);

        bool known = CGQuestLog::HasQuestPOI(questId);
        uint32_t slot = CGQuestLog::FindOrFreePOISlot(questId);
        QuestPOIInfo* poi = nullptr;

        if (!known && slot <= 24) {
            poi = &CGQuestLog::m_questPOIs[slot];
            poi->m_questId = questId;
            poi->m_blobs.SetCount(numBlobs);
            poi->m_iconMapId = 0;
            poi->m_iconObjectiveIndex = 0;
            poi->m_iconMapFloor = 0;
            poi->m_iconX = 0;
            poi->m_iconY = 0;
            poi->unused_0029 = 0;
            poi->m_visible = 0;
            poi->m_objectiveMask = 0;
        }

        for (int32_t j = 0; j < static_cast<int32_t>(numBlobs); j++) {
            int32_t values[7];
            uint32_t numPoints;

            for (uint32_t k = 0; k < 7; k++) {
                msg->Get(reinterpret_cast<uint32_t&>(values[k]));
            }

            msg->Get(numPoints);
            QuestPOIBlob* blob = poi ? &poi->m_blobs.m_data[j] : nullptr;

            if (blob) {
                blob->m_points.SetCount(numPoints);
                blob->m_blobId = values[0];
                blob->m_objectiveIndex = values[1];
                blob->m_mapId = values[2];
                blob->m_worldMapAreaId = values[3];
                blob->m_floor = values[4];
                blob->m_flags = values[6];
                blob->m_priority = values[5];
                blob->m_centerX = 0;
                blob->m_centerY = 0;
            }

            for (int32_t k = 0; k < static_cast<int32_t>(numPoints); k++) {
                int32_t x;
                int32_t y;
                msg->Get(reinterpret_cast<uint32_t&>(x));
                msg->Get(reinterpret_cast<uint32_t&>(y));

                if (blob) {
                    blob->m_points.m_data[k].m_x = x;
                    blob->m_points.m_data[k].m_y = y;
                    blob->m_centerX += x;
                    blob->m_centerY += y;
                }
            }

            if (static_cast<int32_t>(numPoints) > 1 && blob) {
                blob->m_centerX /= static_cast<int32_t>(numPoints);
                blob->m_centerY /= static_cast<int32_t>(numPoints);
            }
        }
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_POI_UPDATE);
}

// VERIFY: 664d65d3 auto
// OFFSET: 0x005DEC10
void RemoveQuestPointDisplay(int32_t questId) {
    if (questId && questId == CGQuestLog::m_selectedQuest) {
        DisplayQuestPoint(0.0f, 0.0f, 0, "");
    }
}

// VERIFY: fca734d5 auto
// OFFSET: 0x005DEE60
int32_t CGQuestLog::IsQuestExpiredByID(int32_t questId) {
    for (int32_t i = 0; i < CGQuestLog::m_numQuest; i++) {
        if (CGQuestLog::m_quests[i].m_questId == questId) {
            return !CGQuestLog::m_quests[i].m_isHeader && CGQuestLog::m_quests[i].m_expired;
        }
    }

    return 0;
}

// VERIFY: f75725e2 auto
// OFFSET: 0x005DFEF0
void CGQuestLog::UpdateWatchTimers() {
    for (uint32_t i = 0; i < CGQuestLog::m_numQuestWatches; i++) {
        if (CGQuestLog::m_questWatches[i].m_expireTime > 0 && static_cast<int32_t>(time(nullptr)) >= CGQuestLog::m_questWatches[i].m_expireTime) {
            CGQuestLog::RemoveQuestWatch(CGQuestLog::m_questWatches[i].m_questId);
            Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);
        }
    }
}

// VERIFY: 993444bb manual
// OFFSET: 0x005E1250
void CGQuestLog::UpdateQuestItem(int32_t itemId) {
    for (int32_t i = 0; i < CGQuestLog::m_numQuest; i++) {
        WOWGUID none = WOWGUID();
        QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[i].m_questId, none, nullptr, nullptr, false);

        if (!quest) {
            continue;
        }

        for (uint32_t j = 0; j < 6; j++) {
            if (static_cast<int32_t>(quest->m_collectItemId[j]) == itemId) {
                Script_SendUniqueSignal(SCRIPT_EVENT_QUEST_LOG_UPDATE);

                if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
                    CGPlayer_C::QueryQuestGiverStatusMultiple();
                }

                return;
            }
        }
    }
}

// VERIFY: 4d319090 manual
// OFFSET: 0x005E5840
int32_t CGQuestLog::SetQuestExpiredByID(int32_t questId) {
    uint32_t index = 0;

    if (!CGQuestLog::m_numQuest) {
        return 0;
    }

    while (CGQuestLog::m_quests[index].m_questId != questId) {
        if (++index >= static_cast<uint32_t>(CGQuestLog::m_numQuest)) {
            return 0;
        }
    }

    if (index >= static_cast<uint32_t>(CGQuestLog::m_numQuest) || CGQuestLog::IsQuestCompleteEx(index, 1)) {
        return 0;
    }

    CGQuestLog::m_quests[index].m_expired = 1;

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::QueryQuestGiverStatusMultiple();
    }

    return 1;
}

// VERIFY: 6a79749a manual
// OFFSET: 0x005E58C0
int32_t CGQuestLog::GetQuestPOILeaderBoardByID(int32_t questId, QuestLogLeaderBoardParams* params) {
    if (!params) {
        return 0;
    }

    params->m_text[0] = 0;
    params->m_type[0] = 0;
    params->m_complete = 0;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (!questId) {
        questId = CGQuestLog::m_selectedQuest;
    }

    if (questId <= 0) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, params->m_questCallback, params->m_param, false);

    if (!quest) {
        return 0;
    }

    int32_t index = params->m_index;

    if (index >= 0 && index < 4) {
        return CGQuestLog::GetQuestLeaderBoardForObject(questId, quest->m_objectiveId[index], params);
    }

    if (index >= 0 && index < 10) {
        return CGQuestLog::GetQuestLeaderBoardForItem(questId, quest->m_collectItemId[index - 4], params);
    }

    if (index >= 0 && index < 16) {
        return CGQuestLog::GetQuestLeaderBoardForItem(questId, quest->m_intermediateItemId[index - 10], params);
    }

    if (index != 16 && index != 17) {
        return 0;
    }

    int32_t logIndex = CGQuestLog::GetQuestIndex(questId) - 1;

    if (logIndex < 0) {
        return 0;
    }

    int32_t slot = CGQuestLog::GetQuestLogSlot(logIndex);
    PlayerQuestLogEntry* entry = player->GetQuestLogEntry(slot);

    if (!entry || !quest->m_endText[0]) {
        return 0;
    }

    SStrCopy(params->m_text, quest->m_endText, STORM_MAX_STR);
    SStrCopy(params->m_type, "event", STORM_MAX_STR);
    params->m_complete = entry->m_state & 1;
    return 1;
}

// VERIFY: 246aa072 manual
// OFFSET: 0x005E5A50
int32_t CGQuestLog::UpdateAllQuestPOI() {
    if (!g_questPOICVar->m_intValue) {
        return 0;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    WorldMapAreaRec* area = nullptr;
    int32_t dungeonMapId = CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap;
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);

    if (!dungeonMap) {
        area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());

        if (CGWorldMap::GetCurrentAreaID() == -1 || !area) {
            return 0;
        }
    }

    int32_t visible = 0;

    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo& poi = CGQuestLog::m_questPOIs[i];
        poi.m_objectiveMask = CGQuestLog::ComputeBlobQuestObjectives(poi.m_questId);
        poi.m_visible = 0;

        for (int32_t j = 0; j < static_cast<int32_t>(poi.m_blobs.Count()); j++) {
            QuestPOIBlob& blob = poi.m_blobs.m_data[j];

            if (poi.m_objectiveMask == 0xFFFFFFFF) {
                if (blob.m_objectiveIndex != -1) {
                    continue;
                }
            } else if (blob.m_objectiveIndex == -1 || !((1 << blob.m_objectiveIndex) & poi.m_objectiveMask)) {
                continue;
            }

            bool onMap;

            if (dungeonMap) {
                onMap = CGQuestLog::IsPOIVisibleOnDungeonMap(&blob.m_centerX, blob.m_mapId, blob.m_floor, dungeonMap->m_id);
            } else {
                if (!area || ((blob.m_flags & 0x4) && blob.m_worldMapAreaId != area->m_id)) {
                    continue;
                }

                onMap = CGQuestLog::IsPOIOnCurrentMap(&blob.m_centerX, blob.m_mapId, area->m_mapID);
            }

            if (onMap) {
                visible++;
                poi.m_visible = 1;
                break;
            }
        }
    }

    return visible;
}

// VERIFY: a01f8ce6 manual
// OFFSET: 0x005E6F30
void CGQuestLog::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_QUERY_TIME_RESPONSE);

    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];
        poi->m_questId = 0;
        poi->m_blobs.Clear();
    }

    CGQuestLog::m_trackedQuestsLoaded = 0;
    ConsoleCommandUnregister("UpdatePOI");
}

// VERIFY: e4df824d manual
// OFFSET: 0x005E2950
uint32_t CGQuestLog::ComputeBlobQuestObjectives(int32_t questId) {
    uint32_t result = 0;
    int32_t index = CGQuestLog::GetQuestIndex(questId) - 1;

    if (index >= CGQuestLog::m_numQuest || CGQuestLog::m_quests[index].m_isHeader || index < 0) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    uint32_t slot = CGQuestLog::m_quests[index].m_logSlot;
    PlayerQuestLogEntry* entry = slot <= 25 ? &player->m_player->m_playerQuestLog[slot] : nullptr;
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_quests[index].m_questId, none, nullptr, nullptr, false);

    if (!quest || !entry || (entry->m_state & 2) || CGQuestLog::IsQuestFailed(index)) {
        return 0;
    }

    if (CGQuestLog::IsQuestCompleteEx(index, 0)) {
        return 0xFFFFFFFF;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (quest->m_objectiveId[i] && entry->m_counts[i] < quest->m_objectiveRequiredCount[i]) {
            result |= 1 << i;
        }
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (quest->m_collectItemId[i] && player->m_bag.GetItemTypeCount(quest->m_collectItemId[i], 8) < static_cast<int32_t>(quest->m_collectItemCount[i])) {
            result |= 1 << (i + 4);
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        uint32_t itemId = quest->m_intermediateItemId[i];

        if (itemId && itemId != quest->m_startingItemId && !player->m_bag.GetItemTypeCount(itemId, 8)) {
            result |= 1 << (i + 10);
        }
    }

    if ((quest->m_flags & 6) && !(entry->m_state & 1)) {
        result |= 0x10000;
    }

    return result;
}

// VERIFY: 89e426ef auto
// OFFSET: 0x005E0110
bool CGQuestLog::IsPOIOnCurrentMap(const int32_t* point, int32_t mapId, int32_t currentMapId) {
    if (mapId != currentMapId) {
        return false;
    }

    C3Vector position(static_cast<float>(point[0]), static_cast<float>(point[1]), 0.0f);
    float x;
    float y;
    bool outland;

    if (!CGWorldMap::GetWorldPosition(position, currentMapId, &x, &y, &outland, false, 0)) {
        return false;
    }

    return !outland && fabs(fabs(x * y)) >= 0.001f;
}

// VERIFY: 6344359d auto
// OFFSET: 0x005E0180
bool CGQuestLog::IsPOIVisibleOnDungeonMap(const int32_t* point, int32_t mapId, int32_t floor, int32_t dungeonMapId) {
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);

    if (!dungeonMap || mapId != dungeonMap->m_mapID || floor != dungeonMap->m_floorIndex) {
        return false;
    }

    C3Vector position(static_cast<float>(point[0]), static_cast<float>(point[1]), 0.0f);
    float x;
    float y;
    bool outland;

    if (!CGWorldMap::GetWorldPosition(position, mapId, &x, &y, &outland, false, dungeonMapId) || outland) {
        return false;
    }

    return !fequalz(fabs(x * y), 0.0f, 0.001f);
}

// VERIFY: 2b97ec8b manual
// OFFSET: 0x005E0240
void CGQuestLog::SaveTrackedQuests() {
    CVarIntListWrapper list;
    list.m_cvar = CVar::Lookup("trackedQuests");
    list.m_pos = 2;
    list.m_state = 0;

    for (uint32_t i = 0; i < CGQuestLog::m_numQuestWatches; i++) {
        if (!CGQuestLog::m_questWatches[i].m_expireTime) {
            list.AddNextValue(CGQuestLog::m_questWatches[i].m_questId);
        }
    }

    if (list.m_state != 1) {
        list.m_buffer[0] = 'v';
        list.m_buffer[1] = 1;
        list.m_buffer[list.m_pos] = 0;
        list.m_cvar->Set(list.m_buffer, true, false, false, true);
    }
}

// STATIC: 0x00AD1264
static int32_t s_poiOverlapIterations = 20;

// VERIFY: 0b11702f manual
// OFFSET: 0x005E2EB0
void CGQuestLog::UpdatePOIIcons() {
    if (!g_questPOICVar->m_intValue) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    WorldMapAreaRec* area = nullptr;
    int32_t dungeonMapId = CGWorldMap::m_currentOrphanZone != -1 ? CGWorldMap::m_currentDungeonMap : -1;
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);

    if (!dungeonMap) {
        area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());

        if (CGWorldMap::GetCurrentAreaID() == -1 || !area) {
            return;
        }
    }

    C3Vector position = player->GetPosition();
    int32_t numActive = 0;
    int32_t numComplete = 0;
    int32_t chain[25];
    int32_t completeList[25];
    memset(chain, 0, sizeof(chain));
    uint8_t movable[25];
    memset(movable, 0, sizeof(movable));
    int32_t activeList[25];

    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];
        float bestDistance = 3.4028235e38f;
        int32_t best = -1;
        int32_t bestPriority = 10;

        for (int32_t j = 0; j < static_cast<int32_t>(poi->m_blobs.Count()); j++) {
            QuestPOIBlob* blob = &poi->m_blobs.m_data[j];
            int32_t floor = blob->m_objectiveIndex;

            if (!((floor == -1 && poi->m_objectiveMask == 0xFFFFFFFF) || (poi->m_objectiveMask != 0xFFFFFFFF && ((1 << floor) & poi->m_objectiveMask)))) {
                continue;
            }

            bool visible;

            if (dungeonMap) {
                visible = CGQuestLog::IsPOIVisibleOnDungeonMap(&blob->m_centerX, blob->m_mapId, blob->m_floor, dungeonMap->m_id);
            } else {
                if (!area || ((blob->m_flags & 0x4) && blob->m_worldMapAreaId != area->m_id)) {
                    continue;
                }

                visible = CGQuestLog::IsPOIOnCurrentMap(&blob->m_centerX, blob->m_mapId, area->m_mapID);
            }

            if (!visible) {
                continue;
            }

            int32_t priority = blob->m_priority;
            float dy = fabs(position.y - blob->m_centerY);
            float dx = fabs(position.x - blob->m_centerX);
            float distance = dx * dx + dy * dy;

            if (priority < bestPriority || (bestDistance > distance && priority == bestPriority)) {
                bestDistance = distance;
                best = j;
                bestPriority = priority;
            }
        }

        if (best == -1) {
            poi->m_iconMapId = 0;
            poi->m_iconObjectiveIndex = 0;
            poi->m_iconMapFloor = 0;
            poi->m_iconX = 999999;
            poi->m_iconY = 999999;
            continue;
        }

        QuestPOIBlob* blob = &poi->m_blobs.m_data[best];
        poi->m_iconMapId = blob->m_mapId;
        poi->m_iconObjectiveIndex = blob->m_objectiveIndex;
        poi->m_iconMapFloor = blob->m_floor;
        poi->m_iconX = blob->m_centerX;
        poi->m_iconY = blob->m_centerY;

        uint32_t logIndex = 0;

        for (uint32_t k = 0; k < 50; k++) {
            if (!CGQuestLog::m_quests[k].m_isHeader && CGQuestLog::m_quests[k].m_questId == poi->m_questId) {
                logIndex = k + 1;
                break;
            }
        }

        if (CGQuestLog::IsQuestCompleteEx(logIndex - 1, 1)) {
            completeList[numComplete++] = i;
        } else {
            movable[i] = blob->m_points.Count() != 1;
            activeList[numActive++] = i;
        }
    }

    if (!area) {
        return;
    }

    int32_t zoneId;

    if (CGWorldMap::m_currentContinent < 0) {
        zoneId = CGWorldMap::m_currentOrphanZone;
    } else {
        WorldMapContinentInfo* continent = &CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent];
        zoneId = CGWorldMap::m_currentZone < 0 ? continent->m_areaID : continent->m_zones.m_data[CGWorldMap::m_currentZone];
    }

    if (zoneId == -1) {
        return;
    }

    int32_t nudges[3][2] = { { 0, 2 }, { 1, -2 }, { -2, 1 } };
    int32_t span = static_cast<int32_t>(fabs(area->m_locLeft - area->m_locRight));
    float radius = span * (CGQuestLog::m_questIconOverlapDist + CGQuestLog::m_questIconOverlapDist) * 0.0009765625f;
    uint32_t nudge = 0;
    float maxStep = 0.050000001f * radius;
    float radiusSq = radius * radius;

    for (int32_t k = 0; k < numActive; k++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[activeList[k]];
        poi->m_iconY = static_cast<int32_t>(poi->m_iconY * 0.75001878f);
    }

    for (int32_t iteration = 0; iteration < s_poiOverlapIterations; iteration++) {
        int32_t forces[25][2];
        memset(forces, 0, sizeof(forces));

        for (int32_t a = 0; a < numActive; a++) {
            int32_t indexA = activeList[a];
            int32_t ax = CGQuestLog::m_questPOIs[indexA].m_iconX;
            int32_t ay = CGQuestLog::m_questPOIs[indexA].m_iconY;

            for (int32_t b = a + 1; b < numActive; b++) {
                int32_t indexB = activeList[b];
                int32_t dx = ax - CGQuestLog::m_questPOIs[indexB].m_iconX;
                int32_t dy = ay - CGQuestLog::m_questPOIs[indexB].m_iconY;
                int32_t distanceSq = dx * dx + dy * dy;

                if (distanceSq >= radiusSq) {
                    continue;
                }

                if (!distanceSq) {
                    dx = nudges[nudge][0];
                    dy = nudges[nudge][1];
                    nudge = (nudge + 1) % 3;
                }

                double scale = radius / sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy);
                int32_t fx = static_cast<int32_t>(dx * scale);
                int32_t fy = static_cast<int32_t>(scale * dy);

                if (movable[indexA]) {
                    forces[indexA][0] += fx;
                    forces[indexA][1] += fy;
                }

                if (distanceSq && movable[indexB]) {
                    forces[indexB][0] -= fx;
                    forces[indexB][1] -= fy;
                }
            }
        }

        float maxForce = -9999999.0f;

        for (int32_t k = 0; k < numActive; k++) {
            float fx = fabs(static_cast<float>(forces[activeList[k]][0]));
            float fy = fabs(static_cast<float>(forces[activeList[k]][1]));

            if (fx > maxForce) {
                maxForce = fx;
            }

            if (fy > maxForce) {
                maxForce = fy;
            }
        }

        if (0.0f == maxForce) {
            break;
        }

        if (maxStep < maxForce) {
            float scale = maxStep / maxForce;

            for (int32_t k = 0; k < numActive; k++) {
                int32_t index = activeList[k];
                forces[index][0] = static_cast<int32_t>(forces[index][0] * scale);
                forces[index][1] = static_cast<int32_t>(forces[index][1] * scale);
            }
        }

        for (int32_t k = 0; k < numActive; k++) {
            int32_t index = activeList[k];
            CGQuestLog::m_questPOIs[index].m_iconX += forces[index][0];
            CGQuestLog::m_questPOIs[index].m_iconY += forces[index][1];
        }
    }

    if (s_cvPOIShiftComplete && s_cvPOIShiftComplete->m_floatValue > 0.001f) {
        for (int32_t c = 0; c < numComplete - 1; c++) {
            int32_t indexC = completeList[c + 1];
            int32_t cx = CGQuestLog::m_questPOIs[indexC].m_iconX;
            int32_t cy = CGQuestLog::m_questPOIs[indexC].m_iconY;
            float threshold = s_cvPOIShiftComplete->m_floatValue * 100.0f;

            for (int32_t d = c; d >= 0; d--) {
                QuestPOIInfo* other = &CGQuestLog::m_questPOIs[completeList[d]];
                int32_t distanceSq = (cx - other->m_iconX) * (cx - other->m_iconX) + (cy - other->m_iconY) * (cy - other->m_iconY);

                if (distanceSq < threshold) {
                    chain[indexC] = chain[completeList[d]] + 1;
                    break;
                }
            }
        }

        int32_t offsets[4][2] = { { 50, 86 }, { -86, 50 }, { -50, 86 }, { -90, 10 } };
        int32_t offset = 0;

        for (int32_t k = 1; k < numComplete; k++) {
            int32_t index = completeList[k];

            if (chain[index]) {
                float shift = s_cvPOIShiftComplete->m_floatValue;
                int32_t ox = offsets[offset][0];
                int32_t oy = offsets[offset][1];
                offset = (offset + 1) & 3;
                CGQuestLog::m_questPOIs[index].m_iconX += static_cast<int32_t>(ox * shift);
                CGQuestLog::m_questPOIs[index].m_iconY += static_cast<int32_t>(shift * oy);
            }
        }
    }

    for (int32_t k = 0; k < numActive; k++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[activeList[k]];
        poi->m_iconY = static_cast<int32_t>(poi->m_iconY * 1.3333f);
    }
}

// VERIFY: 44e619f4 manual
// OFFSET: 0x005E2900
int32_t CGQuestLog::GetQuestPOIIndex(int32_t questId) {
    if (!ClntObjMgrGetActivePlayerObj() || questId <= 0) {
        return -1;
    }

    for (int32_t i = 0; i < 25; i++) {
        if (CGQuestLog::m_questPOIs[i].m_questId == questId) {
            return i;
        }
    }

    return -1;
}

// VERIFY: a15a7449 manual
// OFFSET: 0x005E3AC0
QuestPOIInfo* CGQuestLog::GetQuestPOIInfo(int32_t questId) {
    uint32_t index = CGQuestLog::GetQuestPOIIndex(questId);

    if (index > 24) {
        return nullptr;
    }

    return &CGQuestLog::m_questPOIs[index];
}

// VERIFY: c90d8914 manual
// OFFSET: 0x005E3840
int32_t CGQuestLog::GetPOIQuestIDByVisibleIndex(int32_t index) {
    if (static_cast<uint32_t>(index) > 24) {
        return -1;
    }

    int32_t visible = 0;

    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];

        if (!poi->m_visible) {
            continue;
        }

        if (CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(poi->m_questId) - 1, 1) || CGQuestLog::HasNoObjectives(CGQuestLog::GetQuestIndex(poi->m_questId) - 1)) {
            if (index == visible++) {
                return CGQuestLog::m_questPOIs[i].m_questId;
            }
        }
    }

    for (int32_t i = 0; i < static_cast<int32_t>(CGQuestLog::m_numQuestWatches); i++) {
        int32_t questId = CGQuestLog::m_questWatches[i].m_questId;

        if (CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(questId) - 1, 1) || CGQuestLog::HasNoObjectives(CGQuestLog::GetQuestIndex(questId) - 1)) {
            continue;
        }

        int32_t poi = CGQuestLog::GetQuestPOIIndex(questId);

        if (poi >= 0 && CGQuestLog::m_questPOIs[poi].m_visible && index == visible++) {
            return CGQuestLog::m_questPOIs[poi].m_questId;
        }
    }

    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];

        if (!poi->m_visible) {
            continue;
        }

        if (CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(poi->m_questId) - 1, 1) || CGQuestLog::HasNoObjectives(CGQuestLog::GetQuestIndex(poi->m_questId) - 1)) {
            continue;
        }

        uint32_t watch = 0;

        while (watch < CGQuestLog::m_numQuestWatches && CGQuestLog::m_questWatches[watch].m_questId != poi->m_questId) {
            watch++;
        }

        if (watch >= CGQuestLog::m_numQuestWatches && index == visible++) {
            return CGQuestLog::m_questPOIs[i].m_questId;
        }
    }

    return -1;
}

// VERIFY: ad3a094a manual
// OFFSET: 0x005E2630
int32_t CGQuestLog::GetQuestLeaderBoardForItem(int32_t questId, int32_t itemId, QuestLogLeaderBoardParams* params) {
    if (!params) {
        return 0;
    }

    char* text = params->m_text;
    params->m_text[0] = 0;
    char* type = params->m_type;
    params->m_type[0] = 0;
    params->m_complete = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, params->m_questCallback, params->m_param, false);
    int32_t index = CGQuestLog::GetQuestIndex(questId) - 1;
    PlayerQuestLogEntry* entry = nullptr;

    if (index >= 0 && index < CGQuestLog::m_numQuest && !CGQuestLog::m_quests[index].m_isHeader) {
        uint32_t slot = CGQuestLog::m_quests[index].m_logSlot;

        if (slot <= 25) {
            entry = &player->m_player->m_playerQuestLog[slot];
        }
    }

    if (!quest || !entry) {
        return 0;
    }

    uint32_t i;

    for (i = 0; static_cast<int32_t>(quest->m_collectItemId[i]) != itemId || (quest->m_collectItemId[i] == quest->m_startingItemId && static_cast<int32_t>(quest->m_collectItemCount[i]) <= 1); i++) {
        if (i + 1 >= 6) {
            return 0;
        }
    }

    ItemStats_C* item = g_itemDBCache.GetRecord(quest->m_collectItemId[i], none, params->m_itemCallback, params->m_param, false);
    uint32_t plural = FrameScript_GetPluralIndex(quest->m_collectItemCount[i]);
    int32_t required = quest->m_collectItemCount[i];
    int32_t have = required;

    if (player->m_bag.GetItemTypeCount(quest->m_collectItemId[i], 8) < required) {
        have = player->m_bag.GetItemTypeCount(quest->m_collectItemId[i], 8);
    }

    const char* format = FrameScript_GetText("QUEST_ITEMS_NEEDED", -1, GENDER_NOT_APPLICABLE);
    char buffer[4096];
    SStrPrintf(buffer, sizeof(buffer), format, item ? item->GetName(plural) : " ", have, quest->m_collectItemCount[i]);
    params->m_complete = have >= static_cast<int32_t>(quest->m_collectItemCount[i]);
    SStrCopy(text, buffer, STORM_MAX_STR);
    SStrCopy(type, "item", STORM_MAX_STR);
    return 1;
}

// VERIFY: d30d7e2c manual
// OFFSET: 0x005E2B60
int32_t CGQuestLog::GetQuestWorldMapAreaID(int32_t questId, int32_t* floor) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || questId <= 0) {
        return 0;
    }

    CGWorldMap::SetMapToCurrentZone(0);
    WorldMapAreaRec* current = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());
    int32_t dungeonMapId = CGWorldMap::m_currentOrphanZone != -1 ? CGWorldMap::m_currentDungeonMap : -1;
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);
    int32_t currentFloor = dungeonMap ? dungeonMap->m_floorIndex : 0;

    if (current) {
        uint32_t index;

        for (index = 0; index < 25 && CGQuestLog::m_questPOIs[index].m_questId != questId; index++) {
        }

        if (index < 25) {
            QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[index];
            uint32_t objectives = CGQuestLog::ComputeBlobQuestObjectives(poi->m_questId);
            float bestDistance = 3.4028235e38f;
            poi->m_objectiveMask = objectives;
            int32_t bestArea = 0;
            int32_t bestFloor = 0;
            int32_t bestFloorDelta = 0x7FFFFFFF;

            for (int32_t i = 0; i < static_cast<int32_t>(poi->m_blobs.m_count); i++) {
                QuestPOIBlob* blob = &poi->m_blobs.m_data[i];

                if (objectives == 0xFFFFFFFF) {
                    if (blob->m_objectiveIndex != -1) {
                        continue;
                    }
                } else if (blob->m_objectiveIndex == -1 || !((1 << blob->m_objectiveIndex) & objectives)) {
                    continue;
                }

                if (current->m_id == blob->m_worldMapAreaId && currentFloor == blob->m_floor) {
                    *floor = poi->m_blobs.m_data[i].m_floor;
                    return poi->m_blobs.m_data[i].m_worldMapAreaId;
                }

                WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(blob->m_worldMapAreaId);

                if (!area) {
                    continue;
                }

                if (area->m_mapID == current->m_mapID) {
                    C3Vector position = player->GetPosition();
                    float dx = position.x - blob->m_centerX;
                    float dy = position.y - blob->m_centerY;
                    int32_t floorDelta = abs(currentFloor - blob->m_floor);
                    float distance = dx * dx + dy * dy;

                    if (floorDelta < bestFloorDelta || (floorDelta == bestFloorDelta && bestDistance > distance)) {
                        bestDistance = distance;
                        bestFloorDelta = floorDelta;
                        bestArea = blob->m_worldMapAreaId;
                        bestFloor = blob->m_floor;
                    }
                } else if (!bestArea) {
                    bestArea = blob->m_worldMapAreaId;
                    bestFloor = blob->m_floor;
                }
            }

            if (bestArea) {
                *floor = bestFloor;
                return bestArea;
            }
        }
    }

    uint32_t header = 0;
    uint32_t i;

    for (i = 0; i < 50; i++) {
        if (CGQuestLog::m_quests[i].m_isHeader) {
            header = i;
        } else if (CGQuestLog::m_quests[i].m_questId == questId) {
            break;
        }
    }

    if (i >= 50) {
        return 0;
    }

    int32_t areaId = CGQuestLog::m_quests[header].m_questId;

    if (areaId <= 0) {
        return 0;
    }

    AreaTableRec* area;

    while (true) {
        area = g_areaTableDB.GetRecord(areaId);

        if (!area) {
            return 0;
        }

        if (!area->m_parentAreaID) {
            break;
        }

        areaId = area->m_parentAreaID;
    }

    if (!areaId) {
        return 0;
    }

    for (int32_t j = 0; j < g_worldMapAreaDB.m_numRecords; j++) {
        WorldMapAreaRec* map = g_worldMapAreaDB.GetRecordByIndex(j);

        if (map && map->m_areaID == areaId) {
            return map->m_id;
        }
    }

    return 0;
}

// VERIFY: b4c14128 manual
// OFFSET: 0x005E3AF0
static int QSortQuestWatches(const void* a, const void* b) {
    const QuestWatch* watchA = static_cast<const QuestWatch*>(a);
    const QuestWatch* watchB = static_cast<const QuestWatch*>(b);
    int32_t questA = watchA->m_questId;
    int32_t questB = watchB->m_questId;
    int32_t sort = g_trackerSortingCVar->m_intValue;

    if (sort == 1) {
        return watchB->m_distance >= watchA->m_distance ? -1 : 1;
    }

    if (sort <= 1 || sort > 3) {
        return questA - questB;
    }

    int32_t levelA = GetQuestLevel(questA);
    int32_t levelB = GetQuestLevel(questB);

    if (levelA == levelB) {
        return questA - questB;
    }

    return sort == 3 ? levelA - levelB : levelB - levelA;
}

// VERIFY: 3900de0c manual
// OFFSET: 0x005E3B90
static float GetQuestPOIDistance(int32_t questId) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0.0f;
    }

    int32_t index = CGQuestLog::GetQuestPOIIndex(questId);

    if (index < 0) {
        return 3.4028235e38f;
    }

    float bestDistance = 3.4028235e38f;
    int32_t bestFloorDelta = 0x7FFFFFFF;
    int32_t continent = CGPlayer_C::GetNewContinentID();
    int32_t dungeonMapId = CGWorldMap::m_currentOrphanZone != -1 ? CGWorldMap::m_currentDungeonMap : -1;
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(dungeonMapId);
    int32_t currentFloor = dungeonMap ? dungeonMap->m_floorIndex : 0;
    QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[index];
    uint32_t objectives = CGQuestLog::ComputeBlobQuestObjectives(poi->m_questId);
    poi->m_objectiveMask = objectives;

    for (int32_t i = 0; i < static_cast<int32_t>(poi->m_blobs.m_count); i++) {
        QuestPOIBlob* blob = &poi->m_blobs.m_data[i];

        if (objectives == 0xFFFFFFFF) {
            if (blob->m_objectiveIndex != -1) {
                continue;
            }
        } else if (blob->m_objectiveIndex == -1 || !((1 << blob->m_objectiveIndex) & objectives)) {
            continue;
        }

        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(blob->m_worldMapAreaId);

        if (!area || area->m_mapID != continent) {
            continue;
        }

        C3Vector position = player->GetPosition();
        float dx = position.x - blob->m_centerX;
        float dy = position.y - blob->m_centerY;
        int32_t floorDelta = abs(currentFloor - blob->m_floor);
        float distance = dx * dx + dy * dy;

        if (floorDelta < bestFloorDelta || (floorDelta == bestFloorDelta && bestDistance > distance)) {
            bestDistance = distance;
            bestFloorDelta = floorDelta;
        }
    }

    return bestDistance;
}

// VERIFY: b3c23830 manual
// OFFSET: 0x005E5C10
bool CGQuestLog::SortQuestWatches() {
    int32_t sort = g_trackerSortingCVar->m_intValue;

    if (!sort || CGQuestLog::m_numQuestWatches <= 1) {
        return false;
    }

    if (sort == 1) {
        float last = 0.0f;
        bool sorted = true;

        for (uint32_t i = 0; i < CGQuestLog::m_numQuestWatches; i++) {
            float distance = GetQuestPOIDistance(CGQuestLog::m_questWatches[i].m_questId);
            CGQuestLog::m_questWatches[i].m_distance = distance;

            if (sorted) {
                if (last <= distance) {
                    last = distance;
                } else {
                    sorted = false;
                }
            }
        }

        if (sorted) {
            return false;
        }
    }

    qsort(CGQuestLog::m_questWatches, CGQuestLog::m_numQuestWatches, sizeof(QuestWatch), QSortQuestWatches);
    return true;
}

// VERIFY: f8c30a89 manual
// OFFSET: 0x005E70D0
void CGQuestLog::UpdateQuestPOIFromUserServer() {
    for (uint32_t i = 0; i < 25; i++) {
        QuestPOIInfo* poi = &CGQuestLog::m_questPOIs[i];

        for (uint32_t j = 0; j < poi->m_blobs.m_count; j++) {
            poi->m_blobs.m_data[j].m_points.Clear();
        }

        poi->m_blobs.Clear();
        poi->m_questId = 0;
        poi->m_iconMapId = 0;
        poi->m_iconObjectiveIndex = 0;
        poi->m_iconMapFloor = 0;
        poi->m_iconX = 0;
        poi->m_iconY = 0;
        poi->unused_0029 = 0;
        poi->m_visible = 0;
        poi->m_objectiveMask = 0;
    }

    CGQuestLog::Update(1);
}

// VERIFY: 0cb67ed4 auto
// OFFSET: 0x005EEB70
void CGQuestLog::LeaveWorld() {
}
