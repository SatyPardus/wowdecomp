#ifndef WOW_SOURCE_UI_ACHIEVEMENTINFO_H
#define WOW_SOURCE_UI_ACHIEVEMENTINFO_H

#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Common/WowTime.h"

class AchievementRec;
class Achievement_CriteriaRec;
class CDataStore;
struct lua_State;

struct CLIENT_ACHIEVEMENT_CRITERIA : public TSLinkedNode<CLIENT_ACHIEVEMENT_CRITERIA> {
    /* 0008 */ int32_t m_id;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ uint64_t m_counter;
    /* 0018 */ uint32_t m_flags;
    /* 001C */ WowTime m_date;
    /* 003C */ uint32_t unused_003C;
    /* 0040 */ WOWGUID m_player;
    /* 0048 */ uint32_t m_timer1;
    /* 004C */ uint32_t m_timer2;
};

struct CLIENT_ACHIEVEMENTS : public TSLinkedNode<CLIENT_ACHIEVEMENTS> {
    /* 0008 */ int32_t m_id;
    /* 000C */ WowTime m_date;
};

struct COMPLETED_QUESTS : public TSLinkedNode<COMPLETED_QUESTS> {
    /* 0008 */ int32_t m_questId;
};

struct SPECIFIC_CRITERIA : public TSLinkedNode<SPECIFIC_CRITERIA> {
    /* 0008 */ Achievement_CriteriaRec* m_criteria;
};

struct ACHIEVEMENTCRITERIALIST : public TSHashObject<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE> {
    /* 0018 */ TSList<SPECIFIC_CRITERIA, TSGetLink<SPECIFIC_CRITERIA>> m_criteria;
};

struct ACHIEVEMENT : public TSHashObject<ACHIEVEMENT, HASHKEY_NONE> {
    /* 0018 */ AchievementRec* m_rec;
    /* 001C */ TSList<SPECIFIC_CRITERIA, TSGetLink<SPECIFIC_CRITERIA>> m_criteria;
};

int32_t Packet_SMSG_ACHIEVEMENT_EARNED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_CRITERIA_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_RESPOND_INSPECT_ACHIEVEMENTS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_ALL_ACHIEVEMENT_DATA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_CRITERIA_DELETED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_ACHIEVEMENT_DELETED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_QUERY_QUESTS_COMPLETED_RESPONSE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

class CGAchievementInfo {
    public:
    static void LeaveWorld();
    static void SaveTrackedAchievements();
    static int32_t m_showAchievementUI;
    static void LoadTrackedAchievements();
    static void RemoveTrackedAchievement(int32_t id);
    static WOWGUID m_inspect;
    static bool IsRealAchievement(int32_t id);
    static bool IsSupersededByCompleted(int32_t id);
    static uint64_t BuildOutputString(char* buffer, int32_t size, uint64_t sum, uint64_t highest, int32_t highestId, uint64_t count, uint64_t value, CLIENT_ACHIEVEMENT_CRITERIA* criteria, int32_t flags, int32_t criteriaFlags, bool completed, const AchievementRec* achievement, const Achievement_CriteriaRec* criteriaRec, int32_t required);
    static void ReleaseInspect();

    static void InitializeGame();
    static void ShutdownGame();
    static bool IsCriteriaStat(int32_t criteriaId);
    static void UpdateAchievement(int32_t id, WowTime date);
    static void UpdateCriteria(int32_t id, uint64_t counter, uint32_t flags, WowTime date, WOWGUID player, uint32_t timer1, uint32_t timer2);
    static void UpdateInspectAchievement(WOWGUID guid, int32_t id, WowTime date);
    static void UpdateInspectCriteria(WOWGUID guid, int32_t id, uint64_t counter, WOWGUID player, uint32_t flags, WowTime date, uint32_t timer1, uint32_t timer2);
};

int32_t AddTrackedAchievement(int32_t id);
void AchievementInfoRegisterScriptFunctions();
void AchievementInfoUnregisterScriptFunctions();

extern TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_criteria;
extern TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_recentCriteria;
extern TSList<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>> s_completedAchievements;
extern TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_inspectCriteria;
extern TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_inspectRecentCriteria;
extern TSList<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>> s_trackedAchievements;
extern TSList<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>> s_inspectAchievements;
extern TSList<COMPLETED_QUESTS, TSGetLink<COMPLETED_QUESTS>> s_completedQuests;
extern TSHashTable<ACHIEVEMENT, HASHKEY_NONE> s_achievements;

#endif
