#include "AchievementInfo.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AchievementRec.h"
#include "WoW/Source/DB/Rec/Achievement_CriteriaRec.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <ctime>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/Rec/Achievement_CategoryRec.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00ACFD98
TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_criteria;

// STATIC: 0x00ACFDA4
TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_recentCriteria;

// STATIC: 0x00ACFDB0
TSList<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>> s_completedAchievements;

// STATIC: 0x00ACFDBC
TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_inspectCriteria;

// STATIC: 0x00ACFDC8
TSList<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>> s_inspectRecentCriteria;

// STATIC: 0x00ACFDD4
TSList<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>> s_inspectAchievements;

// STATIC: 0x00ACFDE0
TSList<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>> s_trackedAchievements;

// STATIC: 0x00ACFDEC
TSList<COMPLETED_QUESTS, TSGetLink<COMPLETED_QUESTS>> s_completedQuests;

// STATIC: 0x00C1F450
static int32_t s_unused_C1F450;

// STATIC: 0x00C1F460
static TSHashTable<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE> s_criteriaByType[124];

// STATIC: 0x00C207C0
TSHashTable<ACHIEVEMENT, HASHKEY_NONE> s_achievements;

// STATIC: 0x00C1F454
int32_t CGAchievementInfo::m_showAchievementUI;
// STATIC: 0x00C1F458
WOWGUID CGAchievementInfo::m_inspect;


// VERIFY: edba74cb manual
// OFFSET: 0x005B5190
int32_t Packet_SMSG_QUERY_QUESTS_COMPLETED_RESPONSE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count;
    msg->Get(count);
    COMPLETED_QUESTS* node = s_completedQuests.Head();

    while (node) {
        node = s_completedQuests.DeleteNode(node);
    }

    s_completedQuests.Clear();

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        COMPLETED_QUESTS* quest = s_completedQuests.NewNode(2, 0, 0);
        msg->Get(reinterpret_cast<uint32_t&>(quest->m_questId));
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_QUERY_COMPLETE, nullptr);
    return 1;
}

// VERIFY: b8b29b87 manual
// OFFSET: 0x005B6DF0
void CGAchievementInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_ACHIEVEMENT_EARNED, Packet_SMSG_ACHIEVEMENT_EARNED, nullptr);
    ClientServices::SetMessageHandler(SMSG_CRITERIA_UPDATE, Packet_SMSG_CRITERIA_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESPOND_INSPECT_ACHIEVEMENTS, Packet_SMSG_RESPOND_INSPECT_ACHIEVEMENTS, nullptr);
    ClientServices::SetMessageHandler(SMSG_ALL_ACHIEVEMENT_DATA, Packet_SMSG_ALL_ACHIEVEMENT_DATA, nullptr);
    ClientServices::SetMessageHandler(SMSG_CRITERIA_DELETED, Packet_SMSG_CRITERIA_DELETED, nullptr);
    ClientServices::SetMessageHandler(SMSG_ACHIEVEMENT_DELETED, Packet_SMSG_ACHIEVEMENT_DELETED, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_QUESTS_COMPLETED_RESPONSE, Packet_SMSG_QUERY_QUESTS_COMPLETED_RESPONSE, nullptr);

    s_unused_C1F450 = 1;
    CGAchievementInfo::m_showAchievementUI = 0;
    s_criteria.Clear();
    s_recentCriteria.Clear();
    s_completedAchievements.Clear();
    CGAchievementInfo::m_inspect.m_guid = 0;
    s_inspectCriteria.Clear();
    s_inspectRecentCriteria.Clear();
    s_inspectAchievements.Clear();
    s_trackedAchievements.Clear();
    s_completedQuests.Clear();

    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* rec = i < 0 || i >= g_achievementDB.m_numRecords ? nullptr : &g_achievementDB.m_records[i];
        int32_t id = rec->m_id;

        ACHIEVEMENT* achievement = s_achievements.New(id, HASHKEY_NONE(), 0, 0);
        achievement->m_rec = rec;
    }

    for (int32_t i = 0; i < g_achievement_CriteriaDB.m_numRecords; i++) {
        Achievement_CriteriaRec* rec = i < 0 || i >= g_achievement_CriteriaDB.m_numRecords ? nullptr : &g_achievement_CriteriaDB.m_records[i];

        if (!rec || !g_achievementDB.GetRecord(rec->m_achievementID) || static_cast<uint32_t>(rec->m_type) > 123) {
            continue;
        }

        ACHIEVEMENTCRITERIALIST* list = s_criteriaByType[rec->m_type].Ptr(rec->m_assetID, HASHKEY_NONE());

        if (!list) {
            list = s_criteriaByType[rec->m_type].New(rec->m_assetID, HASHKEY_NONE(), 0, 0);
        }

        list->m_criteria.NewNode(2, 0, 0)->m_criteria = rec;

        ACHIEVEMENT* achievement = s_achievements.Ptr(rec->m_achievementID, HASHKEY_NONE());

        if (achievement) {
            achievement->m_criteria.NewNode(2, 0, 0)->m_criteria = rec;
        }
    }
}

// VERIFY: 1479ef2e auto
// OFFSET: 0x005B0DE0
bool CGAchievementInfo::IsCriteriaStat(int32_t criteriaId) {
    Achievement_CriteriaRec* criteria = g_achievement_CriteriaDB.GetRecord(criteriaId);

    if (!criteria) {
        return false;
    }

    AchievementRec* achievement = g_achievementDB.GetRecord(criteria->m_achievementID);

    if (!achievement) {
        return false;
    }

    Achievement_CategoryRec* category = g_achievement_CategoryDB.GetRecord(achievement->m_category);

    if (!category) {
        return false;
    }

    if (category->m_id == 1) {
        return true;
    }

    category = g_achievement_CategoryDB.GetRecord(category->m_parent);

    if (!category) {
        return false;
    }

    while (category->m_id != 1) {
        category = g_achievement_CategoryDB.GetRecord(category->m_parent);

        if (!category) {
            return false;
        }
    }

    return true;
}

// VERIFY: 17ab1c39 manual
// OFFSET: 0x005B2520
void CGAchievementInfo::UpdateCriteria(int32_t id, uint64_t counter, uint32_t flags, WowTime date, WOWGUID player, uint32_t timer1, uint32_t timer2) {
    CLIENT_ACHIEVEMENT_CRITERIA* criteria = nullptr;

    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_criteria.Head(); node; node = s_criteria.Next(node)) {
        if (node->m_id == id) {
            criteria = node;
            break;
        }
    }

    if (!criteria) {
        criteria = s_criteria.NewNode(0, 0, 0);
        CLIENT_ACHIEVEMENT_CRITERIA* position = s_criteria.Head();

        while (position && position->m_date > date) {
            position = s_criteria.Next(position);
        }

        s_criteria.LinkNode(criteria, 2, position);
    }

    criteria->m_counter = counter;
    criteria->m_id = id;
    criteria->m_flags = flags;
    criteria->m_date = date;
    criteria->m_player = player;
    criteria->m_timer1 = timer1;
    criteria->m_timer2 = timer2;

    if (flags & 1) {
        s_criteria.UnlinkNode(criteria);
        delete criteria;
        return;
    }

    if (!CGAchievementInfo::IsCriteriaStat(id)) {
        return;
    }

    CLIENT_ACHIEVEMENT_CRITERIA* recent = s_recentCriteria.NewNode(0, 0, 0);
    recent->m_id = criteria->m_id;
    recent->m_counter = criteria->m_counter;
    recent->m_flags = criteria->m_flags;
    recent->m_date = criteria->m_date;
    recent->m_player = criteria->m_player;
    recent->m_timer1 = criteria->m_timer1;

    Achievement_CriteriaRec* rec = g_achievement_CriteriaDB.GetRecord(recent->m_id);
    CLIENT_ACHIEVEMENT_CRITERIA* node = s_recentCriteria.Head();
    s_recentCriteria.LinkNode(recent, 2, node);

    for (int32_t count = 0; node; ) {
        Achievement_CriteriaRec* nodeRec = g_achievement_CriteriaDB.GetRecord(node->m_id);

        if (nodeRec) {
            if (node->m_id == recent->m_id || nodeRec->m_achievementID == rec->m_achievementID) {
                s_recentCriteria.UnlinkNode(node);
                delete node;
                node = nullptr;
            } else {
                node = s_recentCriteria.Next(node);
            }
        }

        if (++count >= 5) {
            if (node) {
                s_recentCriteria.UnlinkNode(node);
                delete node;
            }

            return;
        }
    }
}

// VERIFY: b31d5df0 manual
// OFFSET: 0x005B28D0
void CGAchievementInfo::UpdateAchievement(int32_t id, WowTime date) {
    CLIENT_ACHIEVEMENTS* achievement = nullptr;

    for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
        if (node->m_id == id) {
            achievement = node;
            break;
        }
    }

    AchievementRec* rec = g_achievementDB.GetRecord(id);

    if (!rec) {
        return;
    }

    if (!achievement) {
        achievement = s_completedAchievements.NewNode(0, 0, 0);
        CLIENT_ACHIEVEMENTS* position = s_completedAchievements.Head();
        bool found = false;

        while (position && position->m_date >= date && !found) {
            if (position->m_date == date) {
                while (position && position->m_date >= date && !found) {
                    AchievementRec* other = g_achievementDB.GetRecord(position->m_id);

                    if (other && rec->m_uiOrder > other->m_uiOrder) {
                        found = true;
                    } else {
                        position = s_completedAchievements.Next(position);
                    }
                }
            } else {
                position = s_completedAchievements.Next(position);
            }
        }

        s_completedAchievements.LinkNode(achievement, 2, position);
    }

    achievement->m_id = id;
    achievement->m_date = date;
}

// VERIFY: 27756e9f manual
// OFFSET: 0x005B32F0
int32_t Packet_SMSG_ALL_ACHIEVEMENT_DATA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGAchievementInfo::m_showAchievementUI = 1;
    WowTime date;
    int32_t now = static_cast<int32_t>(::time(nullptr));
    int32_t id;
    msg->Get(reinterpret_cast<uint32_t&>(id));

    while (id >= 0) {
        *msg >> date;
        CGAchievementInfo::UpdateAchievement(id, date);
        msg->Get(reinterpret_cast<uint32_t&>(id));
    }

    msg->Get(reinterpret_cast<uint32_t&>(id));

    while (id >= 0) {
        SmartGUID counter;
        SmartGUID player;
        uint32_t flags;
        uint32_t timer1;
        uint32_t timer2;
        *msg >> counter;
        *msg >> player;
        msg->Get(flags);
        *msg >> date;
        msg->Get(timer1);
        msg->Get(timer2);
        CGAchievementInfo::UpdateCriteria(id, counter.m_guid, flags, date, player, now - timer1, now - timer2);
        Achievement_CriteriaRec* rec = g_achievement_CriteriaDB.GetRecord(id);

        if (rec && rec->m_timerTime) {
            FrameScript_SignalEvent(SCRIPT_EVENT_TRACKED_ACHIEVEMENT_UPDATE, "%d%d%d%d", rec->m_achievementID, rec->m_id, timer1, rec->m_timerTime);
        }

        msg->Get(reinterpret_cast<uint32_t&>(id));
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_CRITERIA_UPDATE, nullptr);
    FrameScript_SignalEvent(SCRIPT_EVENT_RECEIVED_ACHIEVEMENT_LIST, nullptr);
    return 1;
}

// VERIFY: e308b3c1 manual
// OFFSET: 0x005B23E0
int32_t AddTrackedAchievement(int32_t id) {
    int32_t count = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
        count++;

        if (node->m_id == id) {
            FrameScript_SignalEvent(SCRIPT_EVENT_TRACKED_ACHIEVEMENT_UPDATE, "%d", id);
            return 1;
        }

        if (count == 10) {
            return 0;
        }
    }

    CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.NewNode(2, 0, 0);
    node->m_id = id;
    FrameScript_SignalEvent(SCRIPT_EVENT_TRACKED_ACHIEVEMENT_UPDATE, "%d", id);
    return 1;
}

// VERIFY: 8872d360 manual
// OFFSET: 0x005B34A0
int32_t Packet_SMSG_RESPOND_INSPECT_ACHIEVEMENTS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WowTime date;
    int32_t now = static_cast<int32_t>(::time(nullptr));
    SmartGUID guid;
    *msg >> guid;
    int32_t id;
    msg->Get(reinterpret_cast<uint32_t&>(id));

    while (id >= 0) {
        *msg >> date;
        CGAchievementInfo::UpdateInspectAchievement(guid, id, date);
        msg->Get(reinterpret_cast<uint32_t&>(id));
    }

    msg->Get(reinterpret_cast<uint32_t&>(id));

    while (id >= 0) {
        SmartGUID counter;
        SmartGUID player;
        uint32_t flags;
        uint32_t timer1;
        uint32_t timer2;
        *msg >> counter;
        *msg >> player;
        msg->Get(flags);
        *msg >> date;
        msg->Get(timer1);
        msg->Get(timer2);
        CGAchievementInfo::UpdateInspectCriteria(guid, id, counter.m_guid, player, flags, date, now - timer1, now - timer2);
        msg->Get(reinterpret_cast<uint32_t&>(id));
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_INSPECT_ACHIEVEMENT_READY, nullptr);
    return 1;
}

// VERIFY: 0126902f manual
// OFFSET: 0x005B3610
int32_t Packet_SMSG_ACHIEVEMENT_DELETED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t id;
    msg->Get(reinterpret_cast<uint32_t&>(id));

    for (CLIENT_ACHIEVEMENTS* node = s_completedAchievements.Head(); node; node = s_completedAchievements.Next(node)) {
        if (node->m_id == id) {
            s_completedAchievements.UnlinkNode(node);
            FrameScript_SignalEvent(SCRIPT_EVENT_ACHIEVEMENT_EARNED, "%d", id);
            delete node;
            return 1;
        }
    }

    return 1;
}

// VERIFY: bbcea87a manual
// OFFSET: 0x005B36F0
int32_t Packet_SMSG_CRITERIA_DELETED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t id;
    msg->Get(reinterpret_cast<uint32_t&>(id));

    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_criteria.Head(); node; node = s_criteria.Next(node)) {
        if (node->m_id == id) {
            s_criteria.UnlinkNode(node);
            FrameScript_SignalEvent(SCRIPT_EVENT_CRITERIA_UPDATE, nullptr);
            delete node;
            return 1;
        }
    }

    return 1;
}

// VERIFY: 9a464c17 manual
// OFFSET: 0x005B3840
void CGAchievementInfo::LoadTrackedAchievements() {
    CVarIntListWrapper list;
    list.m_cvar = CVar::Lookup("trackedAchievements");
    list.m_pos = 2;
    list.m_state = 0;

    for (uint32_t id = list.GetNextValue(); id; id = list.GetNextValue()) {
        AddTrackedAchievement(id);
    }

    if (list.m_state != 1) {
        char value[256];
        value[0] = 'v';
        value[1] = 1;
        value[list.m_pos] = '\0';
        list.m_cvar->Set(value, true, false, false, true);
    }
}

// VERIFY: 4767fcb2 manual
// OFFSET: 0x005B4F00
void CGAchievementInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_ACHIEVEMENT_EARNED);
    ClientServices::ClearMessageHandler(SMSG_CRITERIA_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_RESPOND_INSPECT_ACHIEVEMENTS);
    ClientServices::ClearMessageHandler(SMSG_ALL_ACHIEVEMENT_DATA);
    ClientServices::ClearMessageHandler(SMSG_ACHIEVEMENT_DELETED);
    ClientServices::ClearMessageHandler(SMSG_CRITERIA_DELETED);
    ClientServices::ClearMessageHandler(SMSG_QUERY_QUESTS_COMPLETED_RESPONSE);
    s_unused_C1F450 = 0;
    s_criteria.Clear();
    s_recentCriteria.Clear();
    s_completedAchievements.Clear();
    s_inspectCriteria.Clear();
    s_inspectRecentCriteria.Clear();
    s_inspectAchievements.Clear();
    s_trackedAchievements.Clear();
    s_completedQuests.Clear();

    for (uint32_t i = 0; i < 124; i++) {
        ACHIEVEMENTCRITERIALIST* list = s_criteriaByType[i].m_fulllist.Head();

        while (list) {
            ACHIEVEMENTCRITERIALIST* next = s_criteriaByType[i].Next(list);
            list->m_criteria.Clear();
            s_criteriaByType[i].Delete(list);
            list = next;
        }
    }

    ACHIEVEMENT* achievement = s_achievements.m_fulllist.Head();

    while (achievement) {
        ACHIEVEMENT* next = s_achievements.Next(achievement);
        achievement->m_criteria.Clear();
        s_achievements.Delete(achievement);
        achievement = next;
    }
}

// VERIFY: f42c8dbe manual
// OFFSET: 0x005B1FA0
void CGAchievementInfo::SaveTrackedAchievements() {
    CVarIntListWrapper list;
    list.m_cvar = CVar::Lookup("trackedAchievements");
    list.m_pos = 2;
    list.m_state = 0;

    for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
        list.AddNextValue(node->m_id);
    }

    if (list.m_state != 1) {
        list.m_buffer[0] = 'v';
        list.m_buffer[1] = 1;
        list.m_buffer[list.m_pos] = 0;
        list.m_cvar->Set(list.m_buffer, true, false, false, true);
    }
}

// VERIFY: 4929d50d manual
// OFFSET: 0x005B24D0
void CGAchievementInfo::RemoveTrackedAchievement(int32_t id) {
    bool removed = false;

    for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
        if (node->m_id == id) {
            removed = true;
            s_trackedAchievements.DeleteNode(node);
            break;
        }
    }

    if (removed) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TRACKED_ACHIEVEMENT_UPDATE, "%d", id);
    }
}

// VERIFY: 1dcaec12 manual
// OFFSET: 0x005B2AB0
void CGAchievementInfo::UpdateInspectAchievement(WOWGUID guid, int32_t id, WowTime date) {
    if (CGAchievementInfo::m_inspect != guid) {
        return;
    }

    CLIENT_ACHIEVEMENTS* achievement = nullptr;

    for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; node = s_inspectAchievements.Next(node)) {
        if (node->m_id == id) {
            achievement = node;
            break;
        }
    }

    AchievementRec* rec = g_achievementDB.GetRecord(id);

    if (!achievement) {
        achievement = s_inspectAchievements.NewNode(0, 0, 0);
        CLIENT_ACHIEVEMENTS* position = s_inspectAchievements.Head();
        bool found = false;

        while (position && position->m_date >= date && !found) {
            if (position->m_date == date) {
                while (position && position->m_date >= date && !found) {
                    AchievementRec* other = g_achievementDB.GetRecord(position->m_id);

                    if (rec && other && rec->m_uiOrder > other->m_uiOrder) {
                        found = true;
                    } else {
                        position = s_inspectAchievements.Next(position);
                    }
                }
            } else {
                position = s_inspectAchievements.Next(position);
            }
        }

        s_inspectAchievements.LinkNode(achievement, 2, position);
    }

    achievement->m_id = id;
    achievement->m_date = date;
}

// VERIFY: aa2a9c25 manual
// OFFSET: 0x005B2CB0
void CGAchievementInfo::UpdateInspectCriteria(WOWGUID guid, int32_t id, uint64_t counter, WOWGUID player, uint32_t flags, WowTime date, uint32_t timer1, uint32_t timer2) {
    if (CGAchievementInfo::m_inspect != guid) {
        return;
    }

    CLIENT_ACHIEVEMENT_CRITERIA* criteria = nullptr;

    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_inspectCriteria.Head(); node; node = s_inspectCriteria.Next(node)) {
        if (node->m_id == id) {
            criteria = node;
            break;
        }
    }

    if (!criteria) {
        criteria = s_inspectCriteria.NewNode(0, 0, 0);
        CLIENT_ACHIEVEMENT_CRITERIA* position = s_inspectCriteria.Head();

        while (position && position->m_date > date) {
            position = s_inspectCriteria.Next(position);
        }

        s_inspectCriteria.LinkNode(criteria, 2, position);
    }

    criteria->m_counter = counter;
    criteria->m_flags = flags;
    criteria->m_id = id;
    criteria->m_date = date;
    criteria->m_player = player;
    criteria->m_timer1 = timer1;
    criteria->m_timer2 = timer2;

    if (!CGAchievementInfo::IsCriteriaStat(id)) {
        return;
    }

    CLIENT_ACHIEVEMENT_CRITERIA* recent = s_inspectRecentCriteria.NewNode(0, 0, 0);
    recent->m_id = criteria->m_id;
    recent->m_counter = criteria->m_counter;
    recent->m_flags = criteria->m_flags;
    recent->m_date = criteria->m_date;
    recent->m_player = criteria->m_player;
    recent->m_timer1 = criteria->m_timer1;
    recent->m_timer2 = criteria->m_timer2;

    CLIENT_ACHIEVEMENT_CRITERIA* node = s_inspectRecentCriteria.Head();
    s_inspectRecentCriteria.LinkNode(recent, 2, node);

    for (int32_t count = 0; node; ) {
        if (node->m_id == recent->m_id) {
            s_inspectRecentCriteria.UnlinkNode(node);
            delete node;
            node = nullptr;
        } else {
            node = s_inspectRecentCriteria.Next(node);
        }

        if (++count >= 5) {
            if (node) {
                s_inspectRecentCriteria.UnlinkNode(node);
                delete node;
            }

            return;
        }
    }
}

// VERIFY: 223cdc99 manual
// OFFSET: 0x005B3020
int32_t Packet_SMSG_ACHIEVEMENT_EARNED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WowTime date;
    SmartGUID player;
    int32_t id;
    uint32_t silent;
    *msg >> player;
    msg->Get(reinterpret_cast<uint32_t&>(id));
    *msg >> date;
    msg->Get(silent);

    if (ClntObjMgrGetActivePlayer() == player) {
        CGAchievementInfo::UpdateAchievement(id, date);
    }

    AchievementRec* achievement = g_achievementDB.GetRecord(id);

    if (!achievement || silent) {
        return 1;
    }

    if (!(achievement->m_flags & 0x4)) {
        void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
        CEffect* effect = memory ? new (memory) CEffect() : nullptr;
        WOWGUID guid = player;
        effect->AddSpecialEffect(SPECIALEFFECT_ACHIEVEMENTBASE, guid, CGObject_C::EffectOnceHandler);
        effect->m_flags |= 0x20;
        effect->Release();
    }

    if (ClntObjMgrGetActivePlayer() == player) {
        CGAchievementInfo::RemoveTrackedAchievement(achievement->m_id);
        FrameScript_SignalEvent(SCRIPT_EVENT_ACHIEVEMENT_EARNED, "%d", achievement->m_id);
    }

    return 1;
}

// VERIFY: 76d5a2f3 manual
// OFFSET: 0x005B3160
int32_t Packet_SMSG_CRITERIA_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGAchievementInfo::m_showAchievementUI = 1;
    WowTime date;
    int32_t id;
    SmartGUID counter;
    SmartGUID player;
    uint32_t flags;
    uint32_t timer1;
    uint32_t timer2;
    msg->Get(reinterpret_cast<uint32_t&>(id));
    *msg >> counter;
    *msg >> player;
    msg->Get(flags);
    *msg >> date;
    msg->Get(timer1);
    msg->Get(timer2);
    uint32_t elapsed1 = ::time(nullptr) - timer1;
    uint32_t elapsed2 = ::time(nullptr) - timer2;
    CGAchievementInfo::UpdateCriteria(id, counter.m_guid, flags, date, player, elapsed1, elapsed2);
    FrameScript_SignalEvent(SCRIPT_EVENT_CRITERIA_UPDATE, nullptr);
    Achievement_CriteriaRec* criteria = g_achievement_CriteriaDB.GetRecord(id);

    if (!criteria) {
        return 1;
    }

    if (criteria->m_timerTime) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TRACKED_ACHIEVEMENT_UPDATE, "%d%d%d%d", criteria->m_achievementID, criteria->m_id, timer1, criteria->m_timerTime);
        return 1;
    }

    for (CLIENT_ACHIEVEMENTS* node = s_trackedAchievements.Head(); node; node = s_trackedAchievements.Next(node)) {
        AchievementRec* achievement;

        if (criteria->m_achievementID == node->m_id || ((achievement = g_achievementDB.GetRecord(node->m_id)) && achievement->m_sharesCriteria == criteria->m_achievementID)) {
            FrameScript_SignalEvent(SCRIPT_EVENT_TRACKED_ACHIEVEMENT_UPDATE, "%d", node->m_id);
        }
    }

    return 1;
}

// VERIFY: da39173b auto
// OFFSET: 0x005B0D50
bool CGAchievementInfo::IsRealAchievement(int32_t id) {
    AchievementRec* achievement = g_achievementDB.GetRecord(id);

    if (!achievement) {
        return false;
    }

    Achievement_CategoryRec* category = g_achievement_CategoryDB.GetRecord(achievement->m_category);

    if (!category || category->m_id == 1) {
        return false;
    }

    for (category = g_achievement_CategoryDB.GetRecord(category->m_parent); category; category = g_achievement_CategoryDB.GetRecord(category->m_parent)) {
        if (category->m_id == 1) {
            return false;
        }
    }

    return true;
}

// VERIFY: 57bfe33b manual
// OFFSET: 0x005B37C0
bool CGAchievementInfo::IsSupersededByCompleted(int32_t id) {
    for (int32_t i = 0; i < g_achievementDB.m_numRecords; i++) {
        AchievementRec* achievement = g_achievementDB.GetRecordByIndex(i);

        if (!achievement || achievement->m_supercedes != id) {
            continue;
        }

        for (CLIENT_ACHIEVEMENTS* completed = s_completedAchievements.Head(); completed; completed = s_completedAchievements.Next(completed)) {
            if (completed->m_id == achievement->m_id) {
                return true;
            }
        }
    }

    return false;
}

// VERIFY: fd00d146 manual
// OFFSET: 0x005B0EA0
uint64_t CGAchievementInfo::BuildOutputString(char* buffer, int32_t size, uint64_t sum, uint64_t highest, int32_t highestId, uint64_t count, uint64_t value, CLIENT_ACHIEVEMENT_CRITERIA* criteria, int32_t flags, int32_t criteriaFlags, bool completed, const AchievementRec* achievement, const Achievement_CriteriaRec* criteriaRec, int32_t required) {
    float perDay = 0.0f;
    CVar* colorblindMode = CVar::Lookup("colorblindMode");
    uint64_t result;

    if (flags & 0x8) {
        result = sum;

        if (criteria && (flags & 0x40)) {
            uint32_t days = static_cast<uint32_t>(::time(nullptr) - criteria->m_timer2) / 86400;

            if (days) {
                perDay = static_cast<float>(sum) / static_cast<float>(days);
                result = static_cast<int64_t>(perDay);
            }
        }

        if (criteriaFlags & 0x20) {
            if (colorblindMode && colorblindMode->m_intValue) {
                GetCurrencyShortText(static_cast<int32_t>(result), buffer, size, " ");
            } else {
                GetCurrencyTextureString(static_cast<int32_t>(result), buffer, size, 0);
            }
        } else if (perDay != 0.0f) {
            SStrPrintf(buffer, size, "%0.2f", perDay);
        } else if (result) {
            SStrPrintf(buffer, size, "%I64d", result);
        } else if (flags & 0x1) {
            SStrPrintf(buffer, size, "--");
        } else {
            SStrPrintf(buffer, size, "0");
        }
    } else if (flags & 0x10) {
        Achievement_CriteriaRec* best = g_achievement_CriteriaDB.GetRecord(highestId);
        result = highest;

        if (best) {
            if (flags & 0x1) {
                SStrPrintf(buffer, size, "%I64d (%s)", highest, best->m_description);
            } else {
                SStrPrintf(buffer, size, "%I64d", highest);
            }
        } else if (result) {
            SStrPrintf(buffer, size, "%I64d", result);
        } else if (flags & 0x1) {
            SStrPrintf(buffer, size, "--");
        } else {
            SStrPrintf(buffer, size, "0");
        }
    } else if (flags & 0x20) {
        result = count;

        if (result) {
            SStrPrintf(buffer, size, "%I64d", result);
        } else if (flags & 0x1) {
            SStrPrintf(buffer, size, "--");
        } else {
            SStrPrintf(buffer, size, "0");
        }
    } else if (value) {
        result = value;

        if (criteriaRec && !(flags & 0x1) && result >= static_cast<uint64_t>(static_cast<int64_t>(criteriaRec->m_quantity))) {
            result = static_cast<int64_t>(criteriaRec->m_quantity);
        }

        if (flags & 0x40) {
            uint32_t days = static_cast<uint32_t>(::time(nullptr) - criteria->m_timer2) / 86400;

            if (days) {
                perDay = static_cast<float>(result) / static_cast<float>(days);
                result = static_cast<int64_t>(perDay);
            }
        }

        Achievement_CriteriaRec* rec = g_achievement_CriteriaDB.GetRecord(criteria->m_id);

        if (rec && (rec->m_flags & 0x10)) {
            WowTime time;
            DBDATESTRUCT date;
            WowTime::WowDecodeTimeToDbDate(static_cast<uint32_t>(result), &date);
            SStrPrintf(buffer, size, "%02d/%02d/%d", date.m_month, date.m_day, static_cast<int16_t>(date.m_year));
        } else if (rec && (rec->m_flags & 0x20)) {
            if (colorblindMode && colorblindMode->m_intValue) {
                GetCurrencyShortText(static_cast<int32_t>(result), buffer, size, " ");
            } else {
                GetCurrencyTextureString(static_cast<int32_t>(result), buffer, size, 0);
            }
        } else if (!(flags & 0x1)) {
            SStrPrintf(buffer, size, "%I64d", result);
        } else if (perDay != 0.0f) {
            SStrPrintf(buffer, size, "%0.2f", perDay);
        } else if (result) {
            SStrPrintf(buffer, size, "%I64d", result);
        } else {
            SStrPrintf(buffer, size, "--");
        }
    } else if (completed && !achievement->m_minimumCriteria) {
        result = static_cast<int64_t>(criteriaRec->m_quantity);

        if (!result) {
            if (flags & 0x1) {
                SStrPrintf(buffer, size, "--");
            } else {
                SStrPrintf(buffer, size, "0");
            }
        } else if (criteriaFlags & 0x20) {
            if (colorblindMode && colorblindMode->m_intValue) {
                GetCurrencyShortText(static_cast<int32_t>(result), buffer, size, " ");
            } else {
                GetCurrencyTextureString(static_cast<int32_t>(result), buffer, size, 0);
            }
        } else {
            SStrPrintf(buffer, size, "%I64d", result);
        }
    } else {
        if ((criteriaFlags & 0x20) && (criteriaFlags & 0x1)) {
            if (colorblindMode && colorblindMode->m_intValue) {
                GetCurrencyShortText(0, buffer, size, " ");
            } else {
                GetCurrencyTextureString(0, buffer, size, 0);
            }
        } else if (flags & 0x1) {
            SStrPrintf(buffer, size, "--");
        } else {
            SStrPrintf(buffer, size, "%I64d", static_cast<int64_t>(0));
        }

        result = 0;
    }

    if ((criteriaFlags & 0x1) && criteriaRec) {
        char total[128];

        if (criteriaFlags & 0x20) {
            if (colorblindMode && colorblindMode->m_intValue) {
                GetCurrencyShortText(required ? required : criteriaRec->m_quantity, total, sizeof(total), " ");
            } else {
                GetCurrencyTextureString(required ? required : criteriaRec->m_quantity, total, sizeof(total), 0);
            }
        } else {
            SStrPrintf(total, sizeof(total), "%d", required ? required : criteriaRec->m_quantity);
        }

        SStrPrintf(buffer, size, "%s / %s", buffer, total);
    }

    return result;
}

// VERIFY: 203cf12b manual
// OFFSET: 0x005B2FB0
void CGAchievementInfo::ReleaseInspect() {
    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_inspectCriteria.Head(); node; ) {
        node = s_inspectCriteria.DeleteNode(node);
    }

    for (CLIENT_ACHIEVEMENT_CRITERIA* node = s_inspectRecentCriteria.Head(); node; ) {
        node = s_inspectRecentCriteria.DeleteNode(node);
    }

    for (CLIENT_ACHIEVEMENTS* node = s_inspectAchievements.Head(); node; ) {
        node = s_inspectAchievements.DeleteNode(node);
    }
}

// VERIFY: 011d99d0 auto
// OFFSET: 0x005EEB70
void CGAchievementInfo::LeaveWorld() {
}
