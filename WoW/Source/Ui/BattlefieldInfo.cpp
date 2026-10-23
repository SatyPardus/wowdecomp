#include "BattlefieldInfo.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/BattlemasterListRec.h"
#include "WoW/Source/DB/Rec/PvpDifficultyRec.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstdlib>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/BattlemasterListRec.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/WorldStateUIRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Ui/BarberShop.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/UILCD/LCD.h"

// STATIC: 0x00BEA4A8
WorldPvPQueueStatus CGBattlefieldInfo::m_worldPvPQueues[1];

// STATIC: 0x00BEA2C0
BattlefieldScoreSortCriteria CGBattlefieldInfo::m_scoreSortCriteria[17];

// STATIC: 0x00BEA720
TSGrowableArray<int32_t> CGBattlefieldInfo::m_battlegroundTypes;

// STATIC: 0x00BE9EE0
int32_t CGBattlefieldInfo::m_arenaOpponentSeen[5];

// STATIC: 0x00BE9EF8
WOWGUID CGBattlefieldInfo::m_arenaOpponentPets[5][2];

// STATIC: 0x00BE9F48
WOWGUID CGBattlefieldInfo::m_arenaOpponents[5];

// STATIC: 0x00BE8F90
CGPartyMemberState CGBattlefieldInfo::m_arenaOpponentStats[5];

// STATIC: 0x00BEA580
uint32_t CGBattlefieldInfo::m_numScores;

// STATIC: 0x00BEA5AC
uint32_t CGBattlefieldInfo::m_numWorldStateStats;

// STATIC: 0x00BEA5B8
uint32_t CGBattlefieldInfo::m_numVehicles;

// STATIC: 0x00BE9F70
WOWGUID CGBattlefieldInfo::m_vehicleGuids[40];

// STATIC: 0x00BEA578
uint32_t CGBattlefieldInfo::m_instanceStartTime;

// STATIC: 0x00BEA588
int32_t CGBattlefieldInfo::m_mapDone;

// STATIC: 0x00BEA58C
uint32_t CGBattlefieldInfo::m_winner;

// STATIC: 0x00BEA574
uint32_t CGBattlefieldInfo::m_instanceExpiration;

// STATIC: 0x00BEA0B0
char CGBattlefieldInfo::m_teamNames[2][96];

// STATIC: 0x00BEA594
uint32_t CGBattlefieldInfo::m_teamRatings[2];

// STATIC: 0x00BEA59C
uint32_t CGBattlefieldInfo::m_newTeamRatings[2];

// STATIC: 0x00BEA5A4
uint32_t CGBattlefieldInfo::m_avgMatchmakingRating[2];

// STATIC: 0x00ACD16C
int32_t CGBattlefieldInfo::m_activeIndex = -1;

// STATIC: 0x00BEA4B8
BattlefieldStatus CGBattlefieldInfo::m_status[2];

// STATIC: 0x00BEA564
uint32_t CGBattlefieldInfo::m_activeMapID;

// STATIC: 0x00BEA570
int32_t CGBattlefieldInfo::m_instanceType;

// STATIC: 0x00BEA538
int32_t CGBattlefieldInfo::m_selectedInstance;

// STATIC: 0x00BEA53C
int32_t CGBattlefieldInfo::m_bgTypeID;

// STATIC: 0x00BEA548
uint8_t CGBattlefieldInfo::m_hasHolidayWinToday;

// STATIC: 0x00BEA549
uint8_t CGBattlefieldInfo::m_hasRandomWinToday;

// STATIC: 0x00BEA54A
uint8_t CGBattlefieldInfo::m_arenaFaction;

// STATIC: 0x00BEA54C
int32_t CGBattlefieldInfo::m_holidayLossHonor;

// STATIC: 0x00BEA550
int32_t CGBattlefieldInfo::m_holidayWinHonor;

// STATIC: 0x00BEA554
int32_t CGBattlefieldInfo::m_holidayWinArena;

// STATIC: 0x00BEA558
int32_t CGBattlefieldInfo::m_randomLossHonor;

// STATIC: 0x00BEA55C
int32_t CGBattlefieldInfo::m_randomWinHonor;

// STATIC: 0x00BEA560
int32_t CGBattlefieldInfo::m_randomWinArena;

// STATIC: 0x00BEA5BC
TSGrowableArray<int32_t> CGBattlefieldInfo::m_instances;

// STATIC: 0x00ACD170
int32_t CGBattlefieldInfo::m_logFilterFaction = -1;

// STATIC: 0x00ACD174
int32_t CGBattlefieldInfo::m_allowJoin = 1;

// STATIC: 0x00BEA170
WOWGUID CGBattlefieldInfo::m_flagCarriers[2];

// STATIC: 0x00BEA368
BattlefieldScore* CGBattlefieldInfo::m_scores[80];

// STATIC: 0x00BEA540
uint32_t CGBattlefieldInfo::m_minLevel;

// STATIC: 0x00BEA544
uint32_t CGBattlefieldInfo::m_maxLevel;

// STATIC: 0x00BEA568
WOWGUID CGBattlefieldInfo::m_battlemaster;

// STATIC: 0x00BEA57C
uint32_t CGBattlefieldInfo::m_numScoreEntries;

// STATIC: 0x00BEA584
uint32_t CGBattlefieldInfo::m_nextLogUpdate;

// STATIC: 0x00BEA590
int32_t CGBattlefieldInfo::m_nameCachePending;

// STATIC: 0x00BEA5B0
uint32_t CGBattlefieldInfo::m_numPlayerPositions;

// STATIC: 0x00BEA5B4
uint32_t CGBattlefieldInfo::m_positionUpdateTime;

// STATIC: 0x00BEA710
C2Vector CGBattlefieldInfo::m_flagPositions[2];

// STATIC: 0x00BEA180
WOWGUID CGBattlefieldInfo::m_playerPositionGuids[40];

// STATIC: 0x00BEA348
int32_t CGBattlefieldInfo::m_worldStateStats[8];

// STATIC: 0x00BEA5D0
C2Vector CGBattlefieldInfo::m_playerPositions[40];

// INLINED
static WorldPvPQueueStatus* FindWorldPvPQueue(int32_t queueID) {
    for (uint32_t i = 0; i < sizeof(CGBattlefieldInfo::m_worldPvPQueues) / sizeof(CGBattlefieldInfo::m_worldPvPQueues[0]); i++) {
        if (CGBattlefieldInfo::m_worldPvPQueues[i].m_queueID == queueID) {
            return &CGBattlefieldInfo::m_worldPvPQueues[i];
        }
    }

    for (uint32_t i = 0; i < sizeof(CGBattlefieldInfo::m_worldPvPQueues) / sizeof(CGBattlefieldInfo::m_worldPvPQueues[0]); i++) {
        if (!CGBattlefieldInfo::m_worldPvPQueues[i].m_queueID || !CGBattlefieldInfo::m_worldPvPQueues[i].m_status) {
            return &CGBattlefieldInfo::m_worldPvPQueues[i];
        }
    }

    return nullptr;
}

// VERIFY: 3eb31fb4 manual
// OFFSET: 0x00549500
static WorldPvPQueueStatus* SetWorldPvPQueue(int32_t queueID, int32_t status, int32_t areaID, int32_t expireTime) {
    WorldPvPQueueStatus* queue = FindWorldPvPQueue(queueID);

    if (queue) {
        if (areaID) {
            queue->m_areaID = areaID;
        }

        if (expireTime) {
            queue->m_expireTime = expireTime;
        }

        queue->m_queueID = queueID;
        queue->m_status = status;
    }

    return queue;
}

// VERIFY: 39950e3a auto
// OFFSET: 0x00549630
uint32_t GetArenaPetGUIDCount() {
    uint32_t count = 0;

    do {
        if (!CGBattlefieldInfo::m_arenaOpponents[count] && !CGBattlefieldInfo::m_arenaOpponentPets[count][0]) {
            break;
        }

        count++;
    } while (count < 5);

    return count;
}

// VERIFY: 4d274096 manual
// OFFSET: 0x00608180
WOWGUID CGBattlefieldInfo::GetArenaOpponent(uint32_t index) {
    if (index >= 5) {
        return WOWGUID{ 0 };
    }

    return CGBattlefieldInfo::m_arenaOpponents[index];
}

// VERIFY: 7a2896de manual
// OFFSET: 0x0060A2F0
WOWGUID CGBattlefieldInfo::GetArenaOpponentPet(uint32_t index) {
    if (index >= 5) {
        return WOWGUID();
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGBattlefieldInfo::m_arenaOpponents[index], TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        CGUnitData* data = unit->m_unit;
        return data->m_unitFieldCharm ? data->m_unitFieldCharm : data->m_unitFieldSummon;
    }

    return CGBattlefieldInfo::m_arenaOpponentPets[index][0];
}

// VERIFY: 03843689 auto
// OFFSET: 0x0054CA30
static void ReleaseBattlefieldNameCacheRef(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!CGBattlefieldInfo::m_nameCachePending || !--CGBattlefieldInfo::m_nameCachePending) {
        BattlefieldNameCacheCallback();
    }
}

// VERIFY: 8a2636c7 manual
// OFFSET: 0x0054E330
void CGBattlefieldInfo::LeaveWorld() {
    if (!CGGameUI::m_currentlyReloadingUI) {
        memset(CGBattlefieldInfo::m_vehicleGuids, 0, sizeof(CGBattlefieldInfo::m_vehicleGuids));
        CGBattlefieldInfo::m_numVehicles = 0;
    }

    CGBattlefieldInfo::m_instances.Clear();
    CGBattlefieldInfo::m_instanceType = 0;
}

// VERIFY: cf91ab36 manual
// OFFSET: 0x0054CA50
void CGBattlefieldInfo::EnterWorld() {
    CGBattlefieldInfo::m_selectedInstance = 0;
    CGBattlefieldInfo::m_bgTypeID = 0;
    CGBattlefieldInfo::m_minLevel = 0;
    CGBattlefieldInfo::m_maxLevel = 0;
    CGBattlefieldInfo::m_battlemaster = WOWGUID();
    CGBattlefieldInfo::m_nextLogUpdate = 0;
    CGBattlefieldInfo::m_mapDone = 0;
    CGBattlefieldInfo::m_winner = 0;
    CGBattlefieldInfo::m_logFilterFaction = -1;
    CGBattlefieldInfo::m_allowJoin = 1;

    if (CGBattlefieldInfo::m_nameCachePending) {
        for (uint32_t i = 0; i < CGBattlefieldInfo::m_numScoreEntries; i++) {
            g_nameDBCache.CancelCallback(CGBattlefieldInfo::m_scores[i]->m_guid.m_guid, &ReleaseBattlefieldNameCacheRef, nullptr);
        }
    }

    CGBattlefieldInfo::m_numScoreEntries = 0;
    CGBattlefieldInfo::m_flagPositions[0].x = 0.0f;
    CGBattlefieldInfo::m_numScores = 0;
    CGBattlefieldInfo::m_flagPositions[0].y = 0.0f;
    CGBattlefieldInfo::m_nameCachePending = 0;
    CGBattlefieldInfo::m_flagPositions[1].x = 0.0f;
    CGBattlefieldInfo::m_numWorldStateStats = 0;
    CGBattlefieldInfo::m_flagPositions[1].y = 0.0f;
    CGBattlefieldInfo::m_numPlayerPositions = 0;
    CGBattlefieldInfo::m_flagCarriers[0] = WOWGUID();
    CGBattlefieldInfo::m_flagCarriers[1] = WOWGUID();
    CGBattlefieldInfo::m_positionUpdateTime = 0;

    for (uint32_t i = 0; i < 2; i++) {
        CGBattlefieldInfo::m_teamNames[i][0] = 0;
    }

    CGBattlefieldInfo::m_status[0].m_status = 0;
    CGBattlefieldInfo::m_status[1].m_status = 0;

    if (!CGGameUI::m_currentlyReloadingUI) {
        memset(CGBattlefieldInfo::m_arenaOpponentPets, 0, sizeof(CGBattlefieldInfo::m_arenaOpponentPets));

        for (uint32_t i = 0; i < 5; i++) {
            CGBattlefieldInfo::m_arenaOpponents[i] = WOWGUID();
        }

        for (int32_t i = 1; i <= 5; i++) {
            char unit[32];
            SStrPrintf(unit, sizeof(unit), "arena%d", i);
            FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", unit, "cleared");
            SStrPrintf(unit, sizeof(unit), "arenapet%d", i);
            FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", unit, "cleared");
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BATTLEFIELD_STATUS));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: aaa50c84 auto
// OFFSET: 0x0054A430
bool IsOnTransport(CGObject_C* object) {
    return object->GetTransportGUID() != 0;
}

// VERIFY: b7727b77 manual
// OFFSET: 0x0054A450
int32_t CGBattlefieldInfo::GetPlayerPosition(uint32_t index, C3Vector* position) {
    if (index < CGBattlefieldInfo::m_numPlayerPositions) {
        float y = CGBattlefieldInfo::m_playerPositions[index].y;
        position->x = CGBattlefieldInfo::m_playerPositions[index].x;
        position->y = y;
        position->z = 0.0f;
        return 1;
    }

    return 0;
}

// VERIFY: 37a206cd manual
// OFFSET: 0x0054AD10
void CGBattlefieldInfo::OnArenaUnitDestroyed(const WOWGUID& guid) {
    char token[32];

    for (uint32_t i = 0; i < 5; i++) {
        WOWGUID opponent = CGBattlefieldInfo::m_arenaOpponents[i];

        if (opponent == guid) {
            SStrPrintf(token, sizeof(token), "arena%d", i + 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "destroyed");
            return;
        }

        if (opponent) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(opponent, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                const WOWGUID& pet = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;

                if (pet == guid) {
                    SStrPrintf(token, sizeof(token), "arenapet%d", i + 1);
                    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "destroyed");
                    return;
                }
            }
        }
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponentPets[i][0] == guid) {
            CGBattlefieldInfo::m_arenaOpponentPets[i][0] = WOWGUID();
            CGBattlefieldInfo::m_arenaOpponentPets[i][1] = WOWGUID();
            SStrPrintf(token, sizeof(token), "arenapet%d", i + 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "destroyed");
            return;
        }
    }
}

// VERIFY: 82317c99 manual
// OFFSET: 0x0054AE40
int32_t Packet_SMSG_BATTLEFIELD_STATUS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t index;
    msg->Get(index);

    if (index >= 2) {
        msg->m_read = msg->m_size;
        return 1;
    }

    BattlefieldStatus* status = &CGBattlefieldInfo::m_status[index];
    status->m_joinedQueueData = 0;
    msg->Get(status->m_queueData);

    if (!status->m_queueData) {
        status->m_minLevel = 0;
        status->m_maxLevel = 0;
        status->m_status = 0;
        status->m_instanceID = 0;
        status->m_portExpiration = 0;
        status->m_estimatedWaitTime = 0;
        status->m_queueStartTime = 0;

        if (CGBattlefieldInfo::m_activeIndex == static_cast<int32_t>(index)) {
            CGBattlefieldInfo::m_numPlayerPositions = 0;
            CGBattlefieldInfo::m_instanceExpiration = 0;
            CGBattlefieldInfo::m_instanceStartTime = 0;
            CGBattlefieldInfo::m_numWorldStateStats = 0;
            CGBattlefieldInfo::m_activeMapID = 0;
            CGBattlefieldInfo::m_instanceType = 0;
            CGBattlefieldInfo::m_activeIndex = -1;
            CGLCD::ShowBattlefieldScreen();
        }

        msg->m_read = msg->m_size;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BATTLEFIELD_STATUS, "%d", index + 1);
        return 1;
    }

    uint8_t queueType = static_cast<uint8_t>(status->m_queueData);
    status->m_bgTypeID = static_cast<uint32_t>(status->m_queueData) >> 16;
    status->m_teamSize = queueType & 0x7F;
    uint8_t minLevel;
    uint8_t maxLevel;
    msg->Get(minLevel);
    msg->Get(maxLevel);
    status->m_maxLevel = maxLevel;
    status->m_minLevel = minLevel;
    msg->Get(status->m_instanceID);
    uint8_t rated;
    msg->Get(rated);
    status->m_rated = rated != 0;
    uint32_t state;
    msg->Get(state);
    status->m_status = state;

    if (state == 2) {
        msg->Get(status->m_mapID);
        msg->Get(status->m_joinedQueueData);
        CGTutorial::TriggerTutorial(48);
        uint32_t expiration;
        msg->Get(expiration);
        status->m_portExpiration = expiration ? expiration + OsGetAsyncTimeMs() : 0;
        CGLCD::OnBattlefieldReady(status);
    } else {
        status->m_portExpiration = 0;
    }

    if (status->m_status == 3) {
        msg->Get(CGBattlefieldInfo::m_activeMapID);
        msg->Get(status->m_joinedQueueData);
        MapRec* map = g_mapDB.GetRecord(CGBattlefieldInfo::m_activeMapID);

        if (map) {
            CGBattlefieldInfo::m_instanceType = map->m_instanceType;
        }

        uint32_t value;
        msg->Get(value);
        CGBattlefieldInfo::m_instanceExpiration = value ? value + OsGetAsyncTimeMs() : 0;
        msg->Get(value);
        CGBattlefieldInfo::m_instanceStartTime = value ? OsGetAsyncTimeMs() - value : 0;
        msg->Get(CGBattlefieldInfo::m_arenaFaction);
        CGBattlefieldInfo::m_activeIndex = index;
        int32_t first = 0;
        CGBattlefieldInfo::m_numWorldStateStats = 0;

        for (int32_t i = 0; i < g_worldStateUIDB.m_numRecords; i++) {
            WorldStateUIRec* rec = g_worldStateUIDB.GetRecordByIndex(i);

            if ((rec->m_mapID == static_cast<int32_t>(CGBattlefieldInfo::m_activeMapID) || rec->m_mapID == -1) && rec->m_type == 2) {
                if (!CGBattlefieldInfo::m_numWorldStateStats) {
                    first = i;
                }

                CGBattlefieldInfo::m_numWorldStateStats++;
            } else if (CGBattlefieldInfo::m_numWorldStateStats) {
                break;
            }
        }

        for (uint32_t i = 0; i < CGBattlefieldInfo::m_numWorldStateStats; i++) {
            CGBattlefieldInfo::m_worldStateStats[i] = g_worldStateUIDB.GetRecordByIndex(first + i)->m_id;
        }

        CGBattlefieldInfo::m_logFilterFaction = -1;
        BattlefieldNameCacheCallback();
        CGBarberShop::DisableBarberShop();
        CGLCD::OnBattlefieldActive();
    } else {
        CGBattlefieldInfo::m_instanceExpiration = 0;
        CGBattlefieldInfo::m_instanceStartTime = 0;

        if (CGBattlefieldInfo::m_activeIndex == static_cast<int32_t>(index)) {
            CGBattlefieldInfo::m_activeIndex = -1;
        }
    }

    if (status->m_status == 1) {
        CGTutorial::TriggerTutorial(47);
        msg->Get(status->m_estimatedWaitTime);
        uint32_t waited;
        msg->Get(waited);
        status->m_queueStartTime = OsGetAsyncTimeMs() - waited;
    } else {
        status->m_estimatedWaitTime = 0;
        status->m_queueStartTime = 0;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BATTLEFIELD_STATUS, "%d", index + 1);
    return 1;
}

// VERIFY: 3df55eba manual
// OFFSET: 0x0054B1C0
int32_t Packet_SMSG_GROUP_JOINED_BATTLEGROUND(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t result;
    msg->Get(reinterpret_cast<uint32_t&>(result));

    switch (result) {
        case -15: {
            CGGameUI::DisplayError(ERR_IN_NON_RANDOM_BG);
            break;
        }

        case -14: {
            CGGameUI::DisplayError(ERR_IN_RANDOM_BG);
            break;
        }

        case -13: {
            CGGameUI::DisplayError(ERR_LFG_CANT_USE_BATTLEGROUND);
            break;
        }

        case -12:
        case -11: {
            WOWGUID guid;
            msg->Get(guid.m_guid);
            WOWGUID none = WOWGUID();
            NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

            if (name) {
                CGGameUI::DisplayError(ERR_BATTLEGROUND_JOIN_TIMED_OUT, name->m_name);
            } else {
                CGGameUI::DisplayError(ERR_BATTLEGROUND_JOIN_FAILED);
            }

            break;
        }

        case -10: {
            CGGameUI::DisplayError(ERR_BATTLEGROUND_JOIN_RANGE_INDEX);
            break;
        }

        case -9: {
            CGGameUI::DisplayError(ERR_BATTLEGROUND_JOIN_XP_GAIN);
            break;
        }

        case -8: {
            CGGameUI::DisplayError(ERR_BATTLEGROUND_NOT_IN_BATTLEGROUND);
            break;
        }

        case -7: {
            CGGameUI::DisplayError(ERR_BATTLEGROUND_TEAM_LEFT_QUEUE);
            break;
        }

        case -6: {
            CGGameUI::DisplayError(ERR_BATTLEDGROUND_QUEUED_FOR_RATED);
            break;
        }

        case -5: {
            CGGameUI::DisplayError(ERR_BATTLEGROUND_CANNOT_QUEUE_FOR_RATED);
            break;
        }

        case -4: {
            CGGameUI::DisplayError(ERR_BATTLEGROUND_TOO_MANY_QUEUES);
            break;
        }

        case -3: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_PARTY_SIZE);
            break;
        }

        case -2: {
            CGGameUI::DisplayError(ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            break;
        }

        case -1: {
            break;
        }

        default: {
            BattlemasterListRec* rec = g_battlemasterListDB.GetRecord(result);

            if (rec) {
                CGGameUI::DisplayError(ERR_GROUP_JOIN_BATTLEGROUND_S, rec->m_name);
            } else {
                CGGameUI::DisplayError(ERR_GROUP_JOIN_BATTLEGROUND_FAIL);
            }

            break;
        }
    }

    return 1;
}

// VERIFY: ded4dbc4 auto
// OFFSET: 0x0054B3F0
int32_t Packet_MSG_BATTLEGROUND_PLAYER_POSITIONS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    msg->Get(CGBattlefieldInfo::m_numPlayerPositions);

    for (uint32_t i = 0; i < CGBattlefieldInfo::m_numPlayerPositions; i++) {
        msg->Get(CGBattlefieldInfo::m_playerPositionGuids[i].m_guid);
        msg->Get(CGBattlefieldInfo::m_playerPositions[i].x);
        msg->Get(CGBattlefieldInfo::m_playerPositions[i].y);
    }

    int32_t numFlags;
    msg->Get(reinterpret_cast<uint32_t&>(numFlags));

    if (numFlags > 0) {
        if (numFlags < 2) {
            CGBattlefieldInfo::m_flagCarriers[1] = WOWGUID();
            CGBattlefieldInfo::m_flagPositions[1].x = 0.0f;
            CGBattlefieldInfo::m_flagPositions[1].y = 0.0f;
        }

        do {
            numFlags--;
            msg->Get(CGBattlefieldInfo::m_flagCarriers[numFlags].m_guid);
            msg->Get(CGBattlefieldInfo::m_flagPositions[numFlags].x);
            msg->Get(CGBattlefieldInfo::m_flagPositions[numFlags].y);
        } while (numFlags > 0);

        return 1;
    }

    CGBattlefieldInfo::m_flagPositions[0].x = 0.0f;
    CGBattlefieldInfo::m_flagPositions[0].y = 0.0f;
    CGBattlefieldInfo::m_flagCarriers[0] = WOWGUID();
    CGBattlefieldInfo::m_flagPositions[1].x = 0.0f;
    CGBattlefieldInfo::m_flagCarriers[1] = WOWGUID();
    CGBattlefieldInfo::m_flagPositions[1].y = 0.0f;
    return 1;
}

// VERIFY: 9326860d auto
// OFFSET: 0x0054A530
static void BattlegroundPlayerJoinedNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return;
    }

    char fullName[305];

    if (name->m_realmName[0]) {
        SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
    } else {
        SStrCopy(fullName, name->m_name, sizeof(fullName));
    }

    CGGameUI::DisplayError(ERR_BG_PLAYER_JOINED_SS, fullName, fullName);
}

// VERIFY: d0a9396b auto
// OFFSET: 0x0054A5C0
static void BattlegroundPlayerLeftNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return;
    }

    char fullName[305];

    if (name->m_realmName[0]) {
        SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
    } else {
        SStrCopy(fullName, name->m_name, sizeof(fullName));
    }

    CGGameUI::DisplayError(ERR_BG_PLAYER_LEFT_S, fullName);
}

// VERIFY: 7ba135e2 manual
// OFFSET: 0x0054A650
int32_t QSortBattlefields(const void* a, const void* b) {
    int32_t idA = *static_cast<const int32_t*>(a);
    int32_t idB = *static_cast<const int32_t*>(b);

    if (idA == 32) {
        return -1;
    }

    if (idB == 32) {
        return 1;
    }

    BattlemasterListRec* recA = g_battlemasterListDB.GetRecord(idA);
    BattlemasterListRec* recB = g_battlemasterListDB.GetRecord(idB);

    if (GetWorldState(recA->m_holidayWorldState)) {
        return -1;
    }

    if (GetWorldState(recB->m_holidayWorldState)) {
        return 1;
    }

    if (recA->m_minLevel == recB->m_minLevel) {
        return idA - idB;
    }

    return recA->m_minLevel - recB->m_minLevel;
}

// VERIFY: a2090821 manual
// OFFSET: 0x0054E720
void CGBattlefieldInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_LIST, Packet_SMSG_BATTLEFIELD_LIST, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_STATUS, Packet_SMSG_BATTLEFIELD_STATUS, nullptr);
    ClientServices::SetMessageHandler(MSG_PVP_LOG_DATA, Packet_MSG_PVP_LOG_DATA, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_STATUS_QUEUED, Packet_SMSG_GROUP_JOINED_BATTLEGROUND, nullptr);
    ClientServices::SetMessageHandler(MSG_BATTLEGROUND_PLAYER_POSITIONS, Packet_MSG_BATTLEGROUND_PLAYER_POSITIONS, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEGROUND_PLAYER_JOINED, Packet_Group_3, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEGROUND_PLAYER_LEFT, Packet_Group_3, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_OPPONENT_UPDATE, Packet_SMSG_ARENA_OPPONENT_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_PORT_DENIED, Packet_SMSG_BATTLEFIELD_PORT_DENIED, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE, Packet_SMSG_BATTLEFIELD_MGR_ENTRY_INVITE, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_ENTERING, Packet_SMSG_BATTLEFIELD_MGR_ENTERED, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, Packet_SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_EJECT_PENDING, Packet_SMSG_BATTLEFIELD_MGR_EJECT_PENDING, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_EJECTED, Packet_SMSG_BATTLEFIELD_MGR_EJECTED, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE, Packet_SMSG_BATTLEFIELD_MGR_QUEUE_INVITE, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEFIELD_MGR_STATE_CHANGE, Packet_SMSG_BATTLEFIELD_MGR_STATE_CHANGE, nullptr);

    CGBattlefieldInfo::m_activeIndex = -1;
    CGBattlefieldInfo::m_activeMapID = 0;
    CGBattlefieldInfo::m_battlemaster.m_guid = 0;
    CGBattlefieldInfo::m_instanceExpiration = 0;
    CGBattlefieldInfo::m_instanceStartTime = 0;
    CGBattlefieldInfo::m_nameCachePending = 0;
    CGBattlefieldInfo::m_numScoreEntries = 0;
    CGBattlefieldInfo::m_numScores = 0;
    CGBattlefieldInfo::m_logFilterFaction = -1;
    CGBattlefieldInfo::m_numVehicles = 0;
    CGBattlefieldInfo::m_allowJoin = 1;

    for (uint32_t i = 0; i < 2; i++) {
        CGBattlefieldInfo::m_status[i].m_status = 0;
        CGBattlefieldInfo::m_status[i].m_queueData = 0;
        CGBattlefieldInfo::m_status[i].m_bgTypeID = 0;
        CGBattlefieldInfo::m_status[i].m_mapID = 0;
    }

    for (uint32_t i = 0; i < 1; i++) {
        CGBattlefieldInfo::m_worldPvPQueues[i].m_status = 0;
        CGBattlefieldInfo::m_worldPvPQueues[i].m_areaID = 0;
        CGBattlefieldInfo::m_worldPvPQueues[i].m_queueID = 0;
    }

    for (uint32_t i = 0; i < 80; i++) {
        CGBattlefieldInfo::m_scores[i] = static_cast<BattlefieldScore*>(SMemAlloc(sizeof(BattlefieldScore), __FILE__, __LINE__, 0));
    }

    for (uint32_t i = 0; i < 17; i++) {
        CGBattlefieldInfo::m_scoreSortCriteria[i].m_column = i;
        CGBattlefieldInfo::m_scoreSortCriteria[i].m_descending = 0;
    }

    for (int32_t i = 0; i < g_battlemasterListDB.m_numRecords; i++) {
        BattlemasterListRec* rec = i < 0 || i >= g_battlemasterListDB.m_numRecords ? nullptr : &g_battlemasterListDB.m_records[i];

        if (rec && rec->m_instanceType == 3) {
            CGBattlefieldInfo::m_battlegroundTypes.New(rec->m_id);
        }
    }

    qsort(CGBattlefieldInfo::m_battlegroundTypes.m_data, CGBattlefieldInfo::m_battlegroundTypes.Count(), sizeof(int32_t), QSortBattlefields);
    memset(CGBattlefieldInfo::m_arenaOpponentPets, 0, sizeof(CGBattlefieldInfo::m_arenaOpponentPets));
}

// VERIFY: b9485434 manual
// OFFSET: 0x0054CC70
void CGBattlefieldInfo::JoinBattlefield(uint32_t index, int32_t asGroup, int32_t isRated) {
    if (!CGBattlefieldInfo::m_allowJoin) {
        return;
    }

    BattlemasterListRec* rec = g_battlemasterListDB.GetRecord(CGBattlefieldInfo::m_bgTypeID);
    int32_t arena = rec && rec->m_instanceType == 4;

    if (asGroup && rec && (rec->m_maxGroupSize < static_cast<int32_t>(CGPartyInfo::GetNumPartyMembers()) || rec->m_maxGroupSize < static_cast<int32_t>(CGRaidInfo::m_numMembers))) {
        CGGameUI::DisplayError(ERR_GROUP_JOIN_BATTLEGROUND_TOO_MANY);
        return;
    }

    MapRec* map;

    if (!arena && (map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID)) && map->m_instanceType == 3) {
        CGGameUI::DisplayError(ERR_BATTLEGROUND_NOT_IN_BATTLEGROUND);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(arena ? CMSG_BATTLEMASTER_JOIN_ARENA : CMSG_BATTLEMASTER_JOIN));
    msg.Put(CGBattlefieldInfo::m_battlemaster.m_guid);

    if (arena) {
        msg.Put(static_cast<uint8_t>(index));
    } else {
        msg.Put(static_cast<uint32_t>(CGBattlefieldInfo::m_bgTypeID));

        if (index < CGBattlefieldInfo::m_instances.Count() && CGBattlefieldInfo::m_battlemaster) {
            msg.Put(static_cast<uint32_t>(CGBattlefieldInfo::m_instances.m_data[index]));
        } else {
            msg.Put(static_cast<uint32_t>(0));
        }
    }

    msg.Put(static_cast<uint8_t>(asGroup ? 1 : 0));

    if (arena) {
        msg.Put(static_cast<uint8_t>(isRated ? 1 : 0));
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 40539af6 manual
// OFFSET: 0x0054CE30
void CGBattlefieldInfo::RequestScoreData() {
    if (CGBattlefieldInfo::m_nextLogUpdate && (CGBattlefieldInfo::m_mapDone || CGBattlefieldInfo::m_instanceType == 4)) {
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (!CGBattlefieldInfo::m_nextLogUpdate || static_cast<int32_t>(now - CGBattlefieldInfo::m_nextLogUpdate) >= 0) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_PVP_LOG_DATA));
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGBattlefieldInfo::m_nextLogUpdate = now + 5000;
    }
}

// VERIFY: 816bde48 manual
// OFFSET: 0x0054CEE0
void CGBattlefieldInfo::SetScoreSortColumn(int32_t column) {
    if (CGBattlefieldInfo::m_nameCachePending) {
        return;
    }

    uint32_t index;

    for (index = 0; index < 17; index++) {
        if (CGBattlefieldInfo::m_scoreSortCriteria[index].m_column == column) {
            break;
        }
    }

    if (index < 17) {
        int32_t descending = CGBattlefieldInfo::m_scoreSortCriteria[index].m_descending;

        if (!index) {
            descending = !descending;
        }

        for (uint32_t i = index; i > 0; i--) {
            CGBattlefieldInfo::m_scoreSortCriteria[i] = CGBattlefieldInfo::m_scoreSortCriteria[i - 1];
        }

        CGBattlefieldInfo::m_scoreSortCriteria[0].m_column = column;
        CGBattlefieldInfo::m_scoreSortCriteria[0].m_descending = descending;
    }

    BattlefieldNameCacheCallback();
}

// VERIFY: 0c16867d manual
// OFFSET: 0x0054CF60
void CGBattlefieldInfo::RequestPlayerPositions() {
    if (CGBattlefieldInfo::m_activeIndex < 0 || !CGBattlefieldInfo::m_status[CGBattlefieldInfo::m_activeIndex].m_bgTypeID) {
        CGBattlefieldInfo::m_numPlayerPositions = 0;
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (CGBattlefieldInfo::m_positionUpdateTime && static_cast<int32_t>(now - CGBattlefieldInfo::m_positionUpdateTime) < 0) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_BATTLEGROUND_PLAYER_POSITIONS));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGBattlefieldInfo::m_positionUpdateTime = now + 5000;
}

// VERIFY: 68e8812a auto
// OFFSET: 0x005496C0
void WorldMapNameUpdateCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    FrameScript_SignalEvent(SCRIPT_EVENT_WORLD_MAP_NAME_UPDATE, nullptr);
}

// VERIFY: 443c0b3d auto
// OFFSET: 0x005496D0
void CGBattlefieldInfo::SetBattlemaster(WOWGUID battlemaster) {
    if (battlemaster) {
        CGGameUI::SetInteractTarget(battlemaster);
        CGBattlefieldInfo::m_battlemaster = battlemaster;
        FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELDS_SHOW, nullptr);
        return;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELDS_CLOSED, nullptr);
    CGGameUI::ClearInteractTarget(CGBattlefieldInfo::m_battlemaster, 0);
    CGBattlefieldInfo::m_battlemaster = WOWGUID();
}

// VERIFY: 3c940682 manual
// OFFSET: 0x005497C0
CGPartyMemberState* CGBattlefieldInfo::GetRemoteStats(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponents[i] == guid) {
            return &CGBattlefieldInfo::m_arenaOpponentStats[i];
        }
    }

    return nullptr;
}

// VERIFY: f926b6be manual
// OFFSET: 0x00549860
CGPartyPetState* CGBattlefieldInfo::GetRemotePetStats(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponents[i] && CGBattlefieldInfo::m_arenaOpponentStats[i].m_pet.m_guid == guid) {
            return &CGBattlefieldInfo::m_arenaOpponentStats[i].m_pet;
        }
    }

    return nullptr;
}

// VERIFY: 25a3318d auto
// OFFSET: 0x0054A4A0
int32_t CGBattlefieldInfo::GetFlagPosition(uint32_t index, C3Vector* position) {
    uint32_t count;

    if (CGBattlefieldInfo::m_flagCarriers[1]) {
        count = 2;
    } else {
        if (!CGBattlefieldInfo::m_flagCarriers[0]) {
            return 0;
        }

        count = 1;
    }

    if (index >= count) {
        return 0;
    }

    position->x = CGBattlefieldInfo::m_flagPositions[index].x;
    position->y = CGBattlefieldInfo::m_flagPositions[index].y;
    position->z = 0.0f;
    return 1;
}

// VERIFY: e2ac477e manual
// OFFSET: 0x0054A510
int32_t CGBattlefieldInfo::GetBattlegroundType(int32_t index) {
    if (index >= static_cast<int32_t>(CGBattlefieldInfo::m_battlegroundTypes.m_count)) {
        return 0;
    }

    return CGBattlefieldInfo::m_battlegroundTypes.m_data[index];
}

// VERIFY: 89ed8b7f auto
// OFFSET: 0x0054B510
int32_t Packet_Group_3(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    void (*callback)(int32_t id, const WOWGUID& guid, void* param, bool success) = &BattlegroundPlayerJoinedNameCallback;

    if (msgId != SMSG_BATTLEGROUND_PLAYER_JOINED) {
        callback = &BattlegroundPlayerLeftNameCallback;
    }

    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, guid, callback, nullptr, false);

    if (!name) {
        return 1;
    }

    char fullName[305];

    if (name->m_realmName[0]) {
        SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
    } else {
        SStrCopy(fullName, name->m_name, sizeof(fullName));
    }

    if (msgId == SMSG_BATTLEGROUND_PLAYER_JOINED) {
        CGGameUI::DisplayError(ERR_BG_PLAYER_JOINED_SS, fullName, fullName);
        return 1;
    }

    CGGameUI::DisplayError(ERR_BG_PLAYER_LEFT_S, fullName);
    return 1;
}

// VERIFY: cdce82fb auto
// OFFSET: 0x0054B5E0
int32_t Packet_SMSG_ARENA_OPPONENT_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    if (CGBattlefieldInfo::m_instanceType == 4) {
        CGBattlefieldInfo::OnArenaUnitDestroyed(guid);
    }

    return 1;
}

// VERIFY: 83a65000 auto
// OFFSET: 0x0054B610
int32_t Packet_SMSG_BATTLEFIELD_MGR_ENTRY_INVITE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t queueID;
    uint32_t areaID;
    msg->Get(queueID);
    msg->Get(areaID);
    uint32_t expireTime = 0;
    msg->Get(expireTime);
    SetWorldPvPQueue(queueID, 2, areaID, expireTime);
    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_ENTRY_INVITE, "%d", queueID);
    return 1;
}

// VERIFY: 4f806aaa auto
// OFFSET: 0x0054B680
int32_t Packet_SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t queueID;
    int32_t areaID;
    uint8_t accepted;
    uint8_t loggingIn;
    uint8_t warmup;
    msg->Get(queueID);
    msg->Get(reinterpret_cast<uint32_t&>(areaID));
    msg->Get(accepted);
    msg->Get(loggingIn);
    msg->Get(warmup);
    int32_t inArea = 0;
    AreaTableRec* area;

    if (CGGameUI::m_areaID == areaID || ((area = g_areaTableDB.GetRecord(CGGameUI::m_areaID)) != nullptr && area->m_parentAreaID == areaID)) {
        inArea = 1;
    }

    SetWorldPvPQueue(queueID, accepted != 0, areaID, 0);
    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, "%d%b%b%b%b", queueID, accepted != 0, warmup == 1, inArea, loggingIn != 0);
    return 1;
}

// VERIFY: ab0a4ac5 auto
// OFFSET: 0x0054B750
int32_t Packet_SMSG_BATTLEFIELD_MGR_EJECT_PENDING(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t queueID;
    uint8_t remove;
    msg->Get(queueID);
    msg->Get(remove);
    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_EJECT_PENDING, "%d%b", queueID, remove != 0);
    return 1;
}

// VERIFY: a96abd0d auto
// OFFSET: 0x0054B7A0
int32_t Packet_SMSG_BATTLEFIELD_MGR_QUEUE_INVITE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t queueID;
    uint8_t warmup;
    msg->Get(queueID);
    msg->Get(warmup);
    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_QUEUE_INVITE, "%d%b", queueID, warmup == 1);
    return 1;
}

// VERIFY: a644b830 auto
// OFFSET: 0x0054B7F0
int32_t Packet_SMSG_BATTLEFIELD_MGR_STATE_CHANGE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t oldState;
    uint32_t newState;
    msg->Get(oldState);
    msg->Get(newState);
    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_STATE_CHANGE, "%d%d", oldState, newState);
    return 1;
}

// VERIFY: 072dadd4 manual
// OFFSET: 0x0054BA00
bool CGBattlefieldInfo::GetBracketForLevel(uint32_t level, const BattlemasterListRec* rec, int32_t* bracket) {
    if (!rec || !bracket) {
        return false;
    }

    *bracket = 0;

    if (rec->m_id == 32) {
        *bracket = 0;
        return level == 80;
    }

    bool foundMap = false;

    for (int32_t i = 0; i < g_pvpDifficultyDB.m_numRecords; i++) {
        PvpDifficultyRec* difficulty = g_pvpDifficultyDB.GetRecordByIndex(i);

        if (!difficulty || difficulty->m_mapID != rec->m_mapID[0]) {
            if (foundMap) {
                return false;
            }

            continue;
        }

        if (level >= static_cast<uint32_t>(difficulty->m_minLevel) && level <= static_cast<uint32_t>(difficulty->m_maxLevel)) {
            *bracket = difficulty->m_rangeIndex;
            return true;
        }

        foundMap = true;
    }

    return false;
}

// VERIFY: 1d516d84 manual
// OFFSET: 0x006CF670
int32_t CGBattlefieldInfo::GetArenaOpponentIndex(const WOWGUID& guid, int32_t* index) {
    for (int32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponents[i] == guid) {
            *index = i;
            return 1;
        }
    }

    return 0;
}

// VERIFY: 24eb6783 manual
// OFFSET: 0x00549810
CGPartyMemberState* CGBattlefieldInfo::GetRemoteStatsFromPet(const WOWGUID& petGuid) {
    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponents[i] && CGBattlefieldInfo::m_arenaOpponentStats[i].m_pet.m_guid == petGuid) {
            return &CGBattlefieldInfo::m_arenaOpponentStats[i];
        }
    }

    return nullptr;
}

// VERIFY: 609c1249 manual
// OFFSET: 0x005498C0
int32_t Packet_SMSG_BATTLEFIELD_MGR_ENTERED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t queueID;
    uint8_t invited;
    uint8_t onOffense;
    uint8_t clearedAFK;
    msg->Get(queueID);
    msg->Get(invited);
    msg->Get(onOffense);
    msg->Get(clearedAFK);

    if (clearedAFK) {
        CGChat::AddChatMessage(FrameScript_GetText("CLEARED_AFK", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    WorldPvPQueueStatus* queue = FindWorldPvPQueue(queueID);

    if (queue) {
        queue->m_queueID = queueID;
        queue->m_status = 0;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_ENTERED, "%d%b%b", queueID, invited != 0, onOffense != 0);
    return 1;
}

// VERIFY: 545d7746 manual
// OFFSET: 0x005499C0
int32_t Packet_SMSG_BATTLEFIELD_MGR_EJECTED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t queueID;
    uint8_t reason;
    uint8_t battleStatus;
    uint8_t relocated;
    msg->Get(queueID);
    msg->Get(reason);
    msg->Get(battleStatus);
    msg->Get(relocated);

    switch (reason) {
        case 1:
        case 2:
        case 4:
        case 5:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11: {
            WorldPvPQueueStatus* queue = FindWorldPvPQueue(queueID);

            if (queue) {
                queue->m_queueID = queueID;
                queue->m_status = 0;
            }

            break;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELD_MGR_EJECTED, "%d%b%b%b%b", queueID, reason == 8, relocated != 0, battleStatus == 2, reason == 10);
    return 1;
}

// VERIFY: 8039b7f7 auto
// OFFSET: 0x00549580
uint32_t CGBattlefieldInfo::GetNumFlagCarriers() {
    if (CGBattlefieldInfo::m_flagCarriers[1]) {
        return 2;
    }

    return CGBattlefieldInfo::m_flagCarriers[0] ? 1 : 0;
}

// VERIFY: c2e0f8fd auto
// OFFSET: 0x005495B0
WOWGUID CGBattlefieldInfo::GetFlagCarrier(uint32_t index) {
    uint32_t count;

    if (CGBattlefieldInfo::m_flagCarriers[1]) {
        count = 2;
    } else {
        if (!CGBattlefieldInfo::m_flagCarriers[0]) {
            return WOWGUID();
        }

        count = 1;
    }

    if (index < count) {
        return CGBattlefieldInfo::m_flagCarriers[index];
    }

    return WOWGUID();
}

// VERIFY: f935eb4f manual
// OFFSET: 0x00549600
WOWGUID CGBattlefieldInfo::GetVehicleGuid(uint32_t index) {
    if (index >= CGBattlefieldInfo::m_numVehicles) {
        return WOWGUID();
    }

    return CGBattlefieldInfo::m_vehicleGuids[index];
}

// VERIFY: de2a2830 manual
// OFFSET: 0x00549740
void CGBattlefieldInfo::RemoveVehicle(const WOWGUID& guid) {
    uint32_t index = 0;

    while (CGBattlefieldInfo::m_vehicleGuids[index] != guid) {
        if (++index >= 40) {
            return;
        }
    }

    CGBattlefieldInfo::m_numVehicles--;

    if (index != 39) {
        memcpy(&CGBattlefieldInfo::m_vehicleGuids[index], &CGBattlefieldInfo::m_vehicleGuids[index + 1], sizeof(WOWGUID) * (39 - index));
    }

    CGBattlefieldInfo::m_vehicleGuids[39] = WOWGUID();
}

// VERIFY: 8d4125f3 auto
// OFFSET: 0x0054A700
bool IsCurrentMapArena() {
    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);
    return map && map->m_instanceType == 3;
}

// VERIFY: 9d869e6b manual
// OFFSET: 0x0054A740
int32_t QSortScoreData(const void* a, const void* b) {
    BattlefieldScore* left = *static_cast<BattlefieldScore* const*>(a);
    BattlefieldScore* right = *static_cast<BattlefieldScore* const*>(b);
    WOWGUID none = WOWGUID();
    NameCache* leftName = g_nameDBCache.GetRecord(left->m_guid.m_guid, none, nullptr, nullptr, false);
    WOWGUID none2 = WOWGUID();
    NameCache* rightName = g_nameDBCache.GetRecord(right->m_guid.m_guid, none2, nullptr, nullptr, false);

    if (!leftName || !rightName) {
        return 0;
    }

    if (CGBattlefieldInfo::m_logFilterFaction != -1) {
        int32_t leftFaction;
        ChrRacesRec* leftRace = g_chrRacesDB.GetRecord(leftName->m_race);
        FactionTemplateRec* leftTemplate = g_factionTemplateDB.GetRecord(leftRace->m_factionID);

        if (leftTemplate->m_factionGroup & 0x4) {
            leftFaction = 0;
        } else {
            leftFaction = (g_factionTemplateDB.GetRecord(g_chrRacesDB.GetRecord(leftName->m_race)->m_factionID)->m_factionGroup & 0x2) ? 1 : -1;
        }

        int32_t rightFaction;
        ChrRacesRec* rightRace = g_chrRacesDB.GetRecord(rightName->m_race);
        FactionTemplateRec* rightTemplate = g_factionTemplateDB.GetRecord(rightRace->m_factionID);

        if (rightTemplate->m_factionGroup & 0x4) {
            rightFaction = 0;
        } else {
            rightFaction = (g_factionTemplateDB.GetRecord(g_chrRacesDB.GetRecord(rightName->m_race)->m_factionID)->m_factionGroup & 0x2) ? 1 : -1;
        }

        if (leftFaction != rightFaction) {
            return 2 * (leftFaction != CGBattlefieldInfo::m_logFilterFaction) - 1;
        }
    }

    for (uint32_t i = 0; i < 17; i++) {
        BattlefieldScoreSortCriteria* criteria = &CGBattlefieldInfo::m_scoreSortCriteria[i];
        int32_t result;
        uint32_t leftValue;
        uint32_t rightValue;

        switch (criteria->m_column) {
            case 0:
                leftValue = left->m_killingBlows;
                rightValue = right->m_killingBlows;
                break;

            case 1:
                leftValue = left->m_deaths;
                rightValue = right->m_deaths;
                break;

            case 2:
                leftValue = left->m_bonusHonor;
                rightValue = right->m_bonusHonor;
                break;

            case 3:
                result = SStrCmp(leftName->m_name, rightName->m_name, STORM_MAX_STR);

                if (!result) {
                    continue;
                }

                return criteria->m_descending ? -result : result;

            case 4:
                result = rightName->m_class < leftName->m_class ? -1 : 1;
                return criteria->m_descending ? -result : result;

            case 5:
                leftValue = left->m_honorableKills;
                rightValue = right->m_honorableKills;
                break;

            case 6:
                leftValue = left->m_damageDone;
                rightValue = right->m_damageDone;
                break;

            case 7:
                leftValue = left->m_healingDone;
                rightValue = right->m_healingDone;
                break;

            case 8:
                if (left->m_faction == right->m_faction) {
                    continue;
                }

                result = 2 * (left->m_faction >= right->m_faction) - 1;
                return criteria->m_descending ? -result : result;

            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16: {
                uint32_t stat = criteria->m_column - 9;

                if (stat >= CGBattlefieldInfo::m_numWorldStateStats) {
                    continue;
                }

                result = 2 * (left->m_stats[stat] <= right->m_stats[stat]) - 1;
                return criteria->m_descending ? -result : result;
            }

            default:
                continue;
        }

        if (rightValue == leftValue) {
            continue;
        }

        result = rightValue < leftValue ? -1 : 1;
        return criteria->m_descending ? -result : result;
    }

    return 0;
}

// VERIFY: 8ba9cce2 manual
// OFFSET: 0x0054AA30
void BattlefieldNameCacheCallback() {
    if (CGBattlefieldInfo::m_nameCachePending) {
        return;
    }

    CGBattlefieldInfo::m_numScores = 0;

    for (uint32_t i = 0; i < CGBattlefieldInfo::m_numScoreEntries; i++) {
        WOWGUID none = WOWGUID();
        NameCache* name = g_nameDBCache.GetRecord(CGBattlefieldInfo::m_scores[i]->m_guid.m_guid, none, nullptr, nullptr, false);

        if (CGBattlefieldInfo::m_instanceType != 4) {
            int32_t faction = 0;

            if (name) {
                FactionTemplateRec* factionTemplate = g_factionTemplateDB.GetRecord(g_chrRacesDB.GetRecord(name->m_race)->m_factionID);

                if (!(factionTemplate->m_factionGroup & 0x4)) {
                    faction = (g_factionTemplateDB.GetRecord(g_chrRacesDB.GetRecord(name->m_race)->m_factionID)->m_factionGroup & 0x2) ? 1 : -1;
                }
            }

            CGBattlefieldInfo::m_scores[i]->m_faction = faction;
        }

        if (CGBattlefieldInfo::m_logFilterFaction == -1 || CGBattlefieldInfo::m_scores[i]->m_faction == CGBattlefieldInfo::m_logFilterFaction) {
            CGBattlefieldInfo::m_numScores++;
        }
    }

    qsort(CGBattlefieldInfo::m_scores, CGBattlefieldInfo::m_numScoreEntries, sizeof(BattlefieldScore*), QSortScoreData);
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BATTLEFIELD_SCORE, nullptr);
}

// VERIFY: a2ac32e1 auto
// OFFSET: 0x0054ABA0
void CGBattlefieldInfo::AddVehicle(const WOWGUID& guid) {
    if (!guid || CGBattlefieldInfo::m_numVehicles >= 40) {
        return;
    }

    CGBattlefieldInfo::RemoveVehicle(guid);
    CGBattlefieldInfo::m_vehicleGuids[CGBattlefieldInfo::m_numVehicles] = guid;
    CGBattlefieldInfo::m_numVehicles++;
}

// VERIFY: d69a70d5 manual
// OFFSET: 0x0054ABF0
void CGBattlefieldInfo::OnArenaUnitOutOfRange(const WOWGUID& guid) {
    char token[32];

    for (uint32_t i = 0; i < 5; i++) {
        const WOWGUID& opponent = CGBattlefieldInfo::m_arenaOpponents[i];

        if (opponent == guid) {
            SStrPrintf(token, sizeof(token), "arena%d", i + 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "unseen");
            return;
        }

        if (opponent) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(opponent, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                CGUnitData* data = unit->m_unit;
                const WOWGUID& pet = data->m_unitFieldCharm ? data->m_unitFieldCharm : data->m_unitFieldSummon;

                if (pet == guid) {
                    SStrPrintf(token, sizeof(token), "arenapet%d", i + 1);
                    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "unseen");
                    return;
                }
            }
        }
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponentPets[i][0] == guid) {
            SStrPrintf(token, sizeof(token), "arenapet%d", i + 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "unseen");
            return;
        }
    }
}

// STATIC: 0x00ACD310
static const char* s_flagTokens[2] = { "AllianceFlag", "HordeFlag" };

// VERIFY: 3c841256 manual
// OFFSET: 0x0054D010
const char* CGBattlefieldInfo::GetFlagToken(uint32_t index) {
    uint32_t count;

    if (CGBattlefieldInfo::m_flagCarriers[1]) {
        count = 2;
    } else {
        if (!CGBattlefieldInfo::m_flagCarriers[0]) {
            return nullptr;
        }

        count = 1;
    }

    if (index >= count) {
        return nullptr;
    }

    uint32_t faction = index;
    CGPlayer_C* carrier = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(CGBattlefieldInfo::m_flagCarriers[index], TYPE_PLAYER, __FILE__, __LINE__));

    if (carrier) {
        faction = carrier->GetPVPFactionIndex();
    } else {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player) {
            CGRaidMember* member = CGRaidInfo::FindMemberByGuid(CGBattlefieldInfo::m_flagCarriers[index]);
            faction = player->GetPVPFactionIndex();

            if (!member) {
                faction = faction == 0;
            }
        }
    }

    if (CGBattlefieldInfo::m_activeMapID == 566) {
        return s_flagTokens[faction == 0];
    }

    return s_flagTokens[faction];
}

// VERIFY: 64fce666 manual
// OFFSET: 0x0054D280
int32_t Packet_MSG_PVP_LOG_DATA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (CGBattlefieldInfo::m_nameCachePending) {
        for (uint32_t i = 0; i < CGBattlefieldInfo::m_numScoreEntries; i++) {
            g_nameDBCache.CancelCallback(CGBattlefieldInfo::m_scores[i]->m_guid.m_guid, &ReleaseBattlefieldNameCacheRef, nullptr);
        }
    }

    uint8_t arena;
    msg->Get(arena);

    if (arena) {
        for (uint32_t i = 0; i < 2; i++) {
            msg->Get(CGBattlefieldInfo::m_teamRatings[i]);
            msg->Get(CGBattlefieldInfo::m_newTeamRatings[i]);
            msg->Get(CGBattlefieldInfo::m_avgMatchmakingRating[i]);
        }

        for (uint32_t i = 0; i < 2; i++) {
            msg->GetString(CGBattlefieldInfo::m_teamNames[i], sizeof(CGBattlefieldInfo::m_teamNames[i]));
        }
    }

    uint8_t done;
    msg->Get(done);
    CGBattlefieldInfo::m_mapDone = done != 0;

    if (done) {
        uint8_t winner;
        msg->Get(winner);
        CGBattlefieldInfo::m_winner = winner;
    }

    CGBattlefieldInfo::m_nameCachePending = 0;
    msg->Get(CGBattlefieldInfo::m_numScoreEntries);

    for (uint32_t i = 0; i < CGBattlefieldInfo::m_numScoreEntries; i++) {
        BattlefieldScore* score = CGBattlefieldInfo::m_scores[i];
        msg->Get(score->m_guid.m_guid);
        msg->Get(score->m_killingBlows);

        if (arena) {
            uint8_t team;
            msg->Get(team);
            score->m_faction = team;
            score->m_honorableKills = 0;
            score->m_deaths = 0;
            score->m_bonusHonor = 0;
        } else {
            msg->Get(score->m_honorableKills);
            msg->Get(score->m_deaths);
            msg->Get(score->m_bonusHonor);
        }

        msg->Get(score->m_damageDone);
        msg->Get(score->m_healingDone);
        uint32_t count;
        msg->Get(count);

        for (uint32_t j = 0; j < count; j++) {
            uint32_t value;
            msg->Get(value);

            if (j < 8) {
                score->m_stats[j] = value;
            }
        }

        if (!g_nameDBCache.GetRecord(score->m_guid.m_guid, score->m_guid, &ReleaseBattlefieldNameCacheRef, nullptr, false)) {
            CGBattlefieldInfo::m_nameCachePending++;
        }
    }

    if (!CGBattlefieldInfo::m_nameCachePending) {
        BattlefieldNameCacheCallback();
    }

    return 1;
}

// VERIFY: 5f79b25a manual
// OFFSET: 0x0054D4F0
void CGBattlefieldInfo::BattlefieldMgrEntryInviteResponse(int32_t battleId, uint8_t accept) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BF_MGR_ENTRY_INVITE_RESPONSE));
    msg.Put(static_cast<uint32_t>(battleId));
    msg.Put(accept);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 51d05d09 manual
// OFFSET: 0x0054D590
void CGBattlefieldInfo::BattlefieldMgrQueueRequest(int32_t battleId) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BF_MGR_QUEUE_REQUEST));
    msg.Put(static_cast<uint32_t>(battleId));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 70c354e5 manual
// OFFSET: 0x0054D630
void CGBattlefieldInfo::BattlefieldMgrQueueInviteResponse(int32_t battleId, uint8_t accept) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BF_MGR_QUEUE_INVITE_RESPONSE));
    msg.Put(static_cast<uint32_t>(battleId));
    msg.Put(accept);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: fa825599 manual
// OFFSET: 0x0054D6D0
void CGBattlefieldInfo::BattlefieldMgrExitRequest(int32_t battleId) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BF_MGR_QUEUE_EXIT_REQUEST));
    msg.Put(static_cast<uint32_t>(battleId));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: b9ad3927 manual
// OFFSET: 0x0054D4B0
int32_t Packet_SMSG_BATTLEFIELD_PORT_DENIED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (ClntObjMgrGetActivePlayerObj()) {
        CGGameUI::DisplayError(ERR_PLAYER_DEAD);
    }

    return 1;
}

// VERIFY: e38a0e4a manual
// OFFSET: 0x0054E220
void CGBattlefieldInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_LIST);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_STATUS);
    ClientServices::ClearMessageHandler(MSG_PVP_LOG_DATA);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_STATUS_QUEUED);
    ClientServices::ClearMessageHandler(MSG_BATTLEGROUND_PLAYER_POSITIONS);
    ClientServices::ClearMessageHandler(SMSG_BATTLEGROUND_PLAYER_JOINED);
    ClientServices::ClearMessageHandler(SMSG_BATTLEGROUND_PLAYER_LEFT);
    ClientServices::ClearMessageHandler(SMSG_ARENA_OPPONENT_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_PORT_DENIED);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_ENTERING);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_EJECT_PENDING);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_EJECTED);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE);
    ClientServices::ClearMessageHandler(SMSG_BATTLEFIELD_MGR_STATE_CHANGE);

    for (uint32_t i = 0; i < 80; i++) {
        delete CGBattlefieldInfo::m_scores[i];
    }

    CGBattlefieldInfo::m_battlegroundTypes.Clear();
}

// VERIFY: 8e23bef8 manual
// OFFSET: 0x0054E5B0
void CGBattlefieldInfo::RequestBattlegroundInstanceInfo(uint32_t index) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || static_cast<int32_t>(index) < 0 || index >= CGBattlefieldInfo::m_battlegroundTypes.m_count) {
        return;
    }

    BattlemasterListRec* rec = g_battlemasterListDB.GetRecord(CGBattlefieldInfo::m_battlegroundTypes.m_data[index]);

    if (!rec) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BATTLEFIELD_LIST));
    msg.Put(static_cast<uint32_t>(rec->m_id));
    msg.Put(static_cast<uint8_t>(1));
    uint8_t flag;

    if (player->m_unit->m_unitFieldLevel >= player->m_player->m_playerFieldMaxLevel) {
        flag = 0;
    } else {
        flag = (player->m_player->m_playerFlags >> 25) & 1;
    }

    msg.Put(flag);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGBattlefieldInfo::m_instances.Clear();
    CGBattlefieldInfo::m_bgTypeID = rec->m_id;
}

// VERIFY: b0443e9f auto
// OFFSET: 0x0054E390
int32_t Packet_SMSG_BATTLEFIELD_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID battlemaster;
    uint8_t fromAnywhere;
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t value;
    uint8_t isRandom;
    msg->Get(battlemaster.m_guid);
    msg->Get(fromAnywhere);
    msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_bgTypeID));
    msg->Get(minLevel);
    msg->Get(maxLevel);
    CGBattlefieldInfo::m_minLevel = minLevel;
    CGBattlefieldInfo::m_maxLevel = maxLevel;
    msg->Get(value);
    CGBattlefieldInfo::m_hasHolidayWinToday = value != 0;
    msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_holidayWinHonor));
    msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_holidayWinArena));
    msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_holidayLossHonor));
    msg->Get(isRandom);

    if (isRandom) {
        msg->Get(value);
        CGBattlefieldInfo::m_hasRandomWinToday = value != 0;
        msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_randomWinHonor));
        msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_randomWinArena));
        msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_randomLossHonor));
    } else {
        CGBattlefieldInfo::m_hasRandomWinToday = 0;
        CGBattlefieldInfo::m_randomWinHonor = 0;
        CGBattlefieldInfo::m_randomWinArena = 0;
        CGBattlefieldInfo::m_randomLossHonor = 0;
    }

    uint32_t count;
    msg->Get(count);
    CGBattlefieldInfo::m_instances.SetCount(count);

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        msg->Get(reinterpret_cast<uint32_t&>(CGBattlefieldInfo::m_instances.m_data[i]));
    }

    CGBattlefieldInfo::m_allowJoin = 1;

    if (CGBattlefieldInfo::m_bgTypeID == 32 && battlemaster) {
        CGGameUI::SetInteractTarget(battlemaster);
        CGBattlefieldInfo::m_battlemaster = battlemaster;
        FrameScript_SignalEvent(SCRIPT_EVENT_NPC_PVPQUEUE_ANYWHERE, nullptr);
        return 1;
    }

    if (fromAnywhere) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PVPQUEUE_ANYWHERE_SHOW, nullptr);
        return 1;
    }

    CGBattlefieldInfo::SetBattlemaster(battlemaster);

    if (!battlemaster) {
        FrameScript_SignalEvent(SCRIPT_EVENT_BATTLEFIELDS_SHOW, nullptr);
    }

    return 1;
}

// VERIFY: 225ed4a9 manual
// OFFSET: 0x0054B840
void CGBattlefieldInfo::AddArenaOpponentToSlot(const WOWGUID& guid, int32_t slot) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (CGBattlefieldInfo::m_arenaOpponents[slot] != guid) {
        CGBattlefieldInfo::m_arenaOpponents[slot] = guid;
        memset(&CGBattlefieldInfo::m_arenaOpponentStats[slot], 0, sizeof(CGPartyMemberState));

        if (unit) {
            unit->RegisterScript(0x4000);
        }
    }

    char token[32];
    SStrPrintf(token, sizeof(token), "arena%d", slot + 1);
    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "seen");

    if (!unit) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "unseen");
    }
}

// VERIFY: c68ace60 manual
// OFFSET: 0x0054B920
void CGBattlefieldInfo::AddArenaOpponentPetToSlot(const WOWGUID& petGuid, const WOWGUID& ownerGuid, int32_t slot, int32_t registerNew) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (registerNew && CGBattlefieldInfo::m_arenaOpponentPets[slot][0] != petGuid && unit) {
        unit->RegisterScript(0x4000);
    }

    CGBattlefieldInfo::m_arenaOpponentPets[slot][0] = petGuid;
    CGBattlefieldInfo::m_arenaOpponentPets[slot][1] = ownerGuid;

    char token[32];
    SStrPrintf(token, sizeof(token), "arenapet%d", slot + 1);
    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "seen");

    if (!unit) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_OPPONENT_UPDATE, "%s%s", token, "unseen");
    }
}

// VERIFY: a9f9f494 manual
// OFFSET: 0x0054D100
void CGBattlefieldInfo::AddArenaOpponent(const WOWGUID& guid) {
    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponentPets[i][1] == guid) {
            CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGBattlefieldInfo::m_arenaOpponentPets[i][0], TYPE_UNIT, __FILE__, __LINE__));

            if (pet) {
                pet->UnregisterScript(0x4000);
            }

            CGBattlefieldInfo::AddArenaOpponentToSlot(guid, i);
            return;
        }
    }

    for (int32_t i = 0; i < 5; i++) {
        if (!CGBattlefieldInfo::m_arenaOpponentPets[i][0] && (CGBattlefieldInfo::m_arenaOpponents[i] == guid || !CGBattlefieldInfo::m_arenaOpponents[i])) {
            CGBattlefieldInfo::AddArenaOpponentToSlot(guid, i);
            return;
        }
    }
}

// VERIFY: 93acc678 manual
// OFFSET: 0x0054D1C0
void CGBattlefieldInfo::AddArenaOpponentPet(const WOWGUID& petGuid, const WOWGUID& ownerGuid) {
    for (uint32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponents[i] == ownerGuid) {
            CGBattlefieldInfo::AddArenaOpponentPetToSlot(petGuid, ownerGuid, i, 0);
            return;
        }

        if (CGBattlefieldInfo::m_arenaOpponentPets[i][1] == ownerGuid) {
            CGBattlefieldInfo::AddArenaOpponentPetToSlot(petGuid, ownerGuid, i, 1);
            return;
        }
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (!CGBattlefieldInfo::m_arenaOpponents[i] && !CGBattlefieldInfo::m_arenaOpponentPets[i][0]) {
            CGBattlefieldInfo::AddArenaOpponentPetToSlot(petGuid, ownerGuid, i, 1);
            return;
        }
    }
}
