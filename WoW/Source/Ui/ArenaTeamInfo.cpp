#include "ArenaTeamInfo.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "Engine/Source/Os/TimeManager.h"
#include <cstdlib>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/Ui/ScriptEvents.h"

// STATIC: 0x00A15ABC
static const int32_t s_arenaBracketTypes[3] = { 2, 3, 5 };

// STATIC: 0x00ACF834
int32_t CGArenaTeamInfo::m_showOffline = 1;
// STATIC: 0x00C0F840
ArenaTeam CGArenaTeamInfo::m_arenaTeams[3];


// STATIC: 0x00C0F7F0
uint32_t CGArenaTeamInfo::m_nextAllowedQuery[3];

// STATIC: 0x00C0F7FC
ArenaTeamSort CGArenaTeamInfo::m_sortOrder[7];

// STATIC: 0x00C0F838
WOWGUID CGArenaTeamInfo::m_selectedMember;

// VERIFY: 3896de59 manual
// OFFSET: 0x005A1F00
ArenaTeam* CGArenaTeamInfo::GetArenaTeam(uint32_t index) {
    if (index >= 3) {
        return nullptr;
    }

    return &CGArenaTeamInfo::m_arenaTeams[index];
}

// VERIFY: 4c036d81 auto
// OFFSET: 0x005A28E0
void CGArenaTeamInfo::UpdateRoster(uint32_t index, int32_t reset) {
    if (index >= 3) {
        return;
    }

    if (reset) {
        CGArenaTeamInfo::m_nextAllowedQuery[index] = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_ROSTER_UPDATE, "%d", 1);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_ROSTER_UPDATE, nullptr);
    }
}

// VERIFY: 7c92b868 auto
// OFFSET: 0x005A2B80
ArenaTeamMember* CGArenaTeamInfo::SelectMember(uint32_t team, uint32_t index) {
    if (team >= 3 || index >= CGArenaTeamInfo::m_arenaTeams[team].m_numMembers) {
        CGArenaTeamInfo::m_selectedMember = WOWGUID();
        return nullptr;
    }

    ArenaTeamMember* member = CGArenaTeamInfo::m_arenaTeams[team].m_members.m_data[index];
    CGArenaTeamInfo::m_selectedMember = member->m_guid;
    return member;
}

// VERIFY: 3ead1611 manual
// OFFSET: 0x005A2BD0
static int32_t QSortRoster(const void* a, const void* b) {
    const ArenaTeamMember* member1 = *static_cast<ArenaTeamMember* const*>(a);
    const ArenaTeamMember* member2 = *static_cast<ArenaTeamMember* const*>(b);

    if (!CGArenaTeamInfo::m_showOffline && (!member1->m_online || !member2->m_online) && member1->m_online != member2->m_online) {
        return 2 * (member1->m_online == 0) - 1;
    }

    for (uint32_t i = 0; i < 7; i++) {
        int32_t result;
        uint32_t value1;
        uint32_t value2;

        switch (CGArenaTeamInfo::m_sortOrder[i].m_column) {
            case 0:
                result = SStrCmp(member1->m_name, member2->m_name, STORM_MAX_STR);
                break;
            case 1: {
                ChrClassesRec* class1 = g_chrClassesDB.GetRecord(member1->m_class);
                ChrClassesRec* class2 = g_chrClassesDB.GetRecord(member2->m_class);

                if (!class1 || !class2) {
                    continue;
                }

                result = SStrSortUTF8I(class1->m_name, class2->m_name, STORM_MAX_STR);
                break;
            }
            case 2:
                value1 = member1->m_weekGames;
                value2 = member2->m_weekGames;

                if (value2 == value1) {
                    continue;
                }

                result = value2 < value1 ? -1 : 1;
                break;
            case 3:
                value1 = member1->m_weekWins;
                value2 = member2->m_weekWins;

                if (value2 == value1) {
                    continue;
                }

                result = value2 < value1 ? -1 : 1;
                break;
            case 4:
                value1 = member1->m_seasonGames;
                value2 = member2->m_seasonGames;

                if (value2 == value1) {
                    continue;
                }

                result = value2 < value1 ? -1 : 1;
                break;
            case 5:
                value1 = member1->m_seasonWins;
                value2 = member2->m_seasonWins;

                if (value2 == value1) {
                    continue;
                }

                result = value2 < value1 ? -1 : 1;
                break;
            case 6:
                value1 = member1->m_personalRating;
                value2 = member2->m_personalRating;

                if (value2 == value1) {
                    continue;
                }

                result = value2 < value1 ? -1 : 1;
                break;
            default:
                continue;
        }

        if (!result) {
            continue;
        }

        if (CGArenaTeamInfo::m_sortOrder[i].m_reverse) {
            result = -result;
        }

        return result;
    }

    return 0;
}

// VERIFY: 6e5ba8e8 auto
// OFFSET: 0x005A2D50
int32_t Packet_SMSG_ARENA_TEAM_STATS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t teamId;
    msg->Get(teamId);
    uint32_t index = 0;

    while (CGArenaTeamInfo::m_arenaTeams[index].m_teamId != teamId) {
        if (++index >= 3) {
            return 1;
        }
    }

    ArenaTeam& team = CGArenaTeamInfo::m_arenaTeams[index];
    msg->Get(team.m_rating);
    msg->Get(team.m_weekGames);
    msg->Get(team.m_weekWins);
    msg->Get(team.m_seasonGames);
    msg->Get(team.m_seasonWins);
    msg->Get(team.m_rank);
    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_UPDATE, nullptr);
    return 1;
}

// VERIFY: a2576927 manual
// OFFSET: 0x005A3D60
void ArenaTeam::FreeMemberArray() {
    this->m_teamId = 0;

    for (uint32_t i = 0; i < this->m_members.m_count; i++) {
        delete this->m_members.m_data[i];
        this->m_members.m_data[i] = nullptr;
    }

    this->m_members.m_count = 0;
    this->m_type = 0;
    this->m_numMembers = 0;
    this->m_numOnline = 0;
    this->m_rating = 0;
    this->m_weekGames = 0;
    this->m_weekWins = 0;
    this->m_seasonGames = 0;
    this->m_seasonWins = 0;
    this->m_rank = 0;
}

// VERIFY: 8e8debc9 manual
// OFFSET: 0x005A3E10
int32_t Packet_SMSG_ARENA_TEAM_ROSTER(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t teamId;
    msg->Get(teamId);
    uint8_t hasRatings = 0;
    msg->Get(hasRatings);
    uint32_t index = 0;

    while (CGArenaTeamInfo::m_arenaTeams[index].m_teamId != teamId) {
        if (++index >= 3) {
            return 1;
        }
    }

    if (index >= 3) {
        return 1;
    }

    ArenaTeam* team = &CGArenaTeamInfo::m_arenaTeams[index];
    uint32_t oldCount = team->m_members.m_count;
    team->m_numOnline = 0;
    msg->Get(team->m_numMembers);
    msg->Get(team->m_type);

    if (team->m_numMembers > oldCount) {
        team->m_members.SetCount(team->m_numMembers);

        for (uint32_t i = oldCount; i < team->m_numMembers; i++) {
            team->m_members.m_data[i] = static_cast<ArenaTeamMember*>(SMemAlloc(sizeof(ArenaTeamMember), __FILE__, __LINE__, 0));
        }
    }

    for (uint32_t i = 0; i < team->m_numMembers; i++) {
        msg->Get(team->m_members.m_data[i]->m_guid.m_guid);
        uint8_t online;
        msg->Get(online);
        team->m_members.m_data[i]->m_online = online != 0;
        msg->GetString(team->m_members.m_data[i]->m_name, sizeof(team->m_members.m_data[i]->m_name));
        msg->Get(team->m_members.m_data[i]->m_rank);
        uint8_t level;
        uint8_t classId;
        msg->Get(level);
        msg->Get(classId);
        team->m_members.m_data[i]->m_level = level;
        team->m_members.m_data[i]->m_class = classId;
        msg->Get(team->m_members.m_data[i]->m_weekGames);
        msg->Get(team->m_members.m_data[i]->m_weekWins);
        msg->Get(team->m_members.m_data[i]->m_seasonGames);
        msg->Get(team->m_members.m_data[i]->m_seasonWins);
        msg->Get(team->m_members.m_data[i]->m_personalRating);

        if (hasRatings) {
            msg->Get(team->m_members.m_data[i]->m_matchmakerRatingBefore);
            msg->Get(team->m_members.m_data[i]->m_matchmakerRatingAfter);
        } else {
            team->m_members.m_data[i]->m_matchmakerRatingBefore = 0.0f;
            team->m_members.m_data[i]->m_matchmakerRatingAfter = 0.0f;
        }

        if (team->m_members.m_data[i]->m_online) {
            team->m_numOnline++;
        }
    }

    qsort(team->m_members.m_data, team->m_numMembers, sizeof(ArenaTeamMember*), QSortRoster);
    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_ROSTER_UPDATE, nullptr);
    return 1;
}

// VERIFY: d32a70e0 auto
// OFFSET: 0x005A40E0
void CGArenaTeamInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_ROSTER, Packet_SMSG_ARENA_TEAM_ROSTER, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_STATS, Packet_SMSG_ARENA_TEAM_STATS, nullptr);

    for (uint32_t i = 0; i < 3; i++) {
        CGArenaTeamInfo::m_arenaTeams[i].FreeMemberArray();
    }

    CGArenaTeamInfo::m_selectedMember = WOWGUID();

    for (uint32_t i = 0; i < 7; i++) {
        CGArenaTeamInfo::m_sortOrder[i].m_reverse = 0;
    }

    CGArenaTeamInfo::m_showOffline = 1;

    for (uint32_t i = 0; i < 3; i++) {
        CGArenaTeamInfo::m_nextAllowedQuery[i] = 0;
    }

    for (int32_t i = 0; i < 7; i++) {
        CGArenaTeamInfo::m_sortOrder[i].m_column = i;
    }
}

// VERIFY: 04a71da4 manual
// OFFSET: 0x005A37B0
bool CGArenaTeamInfo::CheckPersonalAndTeamRating(uint32_t rating, int32_t bracket) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    for (uint32_t i = 0; i < 3; i++) {
        uint32_t* info = &player->m_player->m_playerFieldArenaTeamInfo11[i * 7];

        if (!info) {
            continue;
        }

        int32_t type = info[1];

        if (type < s_arenaBracketTypes[bracket] || info[6] < rating) {
            continue;
        }

        for (uint32_t j = 0; j < 3; j++) {
            ArenaTeam* team = &CGArenaTeamInfo::m_arenaTeams[j];

            if (team && team->m_type == static_cast<uint32_t>(type) && team->m_rating >= rating) {
                return true;
            }
        }
    }

    return false;
}

// VERIFY: ce2b2810 manual
// OFFSET: 0x005A2E10
void CGArenaTeamInfo::SetRosterShowOffline(int32_t showOffline) {
    if (showOffline == CGArenaTeamInfo::m_showOffline) {
        return;
    }

    CGArenaTeamInfo::m_showOffline = showOffline;

    for (uint32_t i = 0; i < 3; i++) {
        if (CGArenaTeamInfo::m_arenaTeams[i].m_numMembers) {
            qsort(CGArenaTeamInfo::m_arenaTeams[i].m_members.m_data, CGArenaTeamInfo::m_arenaTeams[i].m_numMembers, sizeof(ArenaTeamMember*), QSortRoster);

            if (i < 3) {
                FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_ROSTER_UPDATE, nullptr);
            }
        }
    }
}

// VERIFY: 639eb906 manual
// OFFSET: 0x005A2E80
void CGArenaTeamInfo::SortRoster(int32_t column) {
    for (uint32_t i = 0; i < 7; i++) {
        if (CGArenaTeamInfo::m_sortOrder[i].m_column != column) {
            continue;
        }

        int32_t reverse = CGArenaTeamInfo::m_sortOrder[i].m_reverse;

        if (!i) {
            reverse = reverse == 0;
        }

        for (uint32_t j = i; j; j--) {
            CGArenaTeamInfo::m_sortOrder[j] = CGArenaTeamInfo::m_sortOrder[j - 1];
        }

        CGArenaTeamInfo::m_sortOrder[0].m_column = column;
        CGArenaTeamInfo::m_sortOrder[0].m_reverse = reverse;
        break;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (CGArenaTeamInfo::m_arenaTeams[i].m_numMembers) {
            qsort(CGArenaTeamInfo::m_arenaTeams[i].m_members.m_data, CGArenaTeamInfo::m_arenaTeams[i].m_numMembers, sizeof(ArenaTeamMember*), QSortRoster);

            if (i < 3) {
                FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_ROSTER_UPDATE, nullptr);
            }
        }
    }
}

// VERIFY: 38e6d51d manual
// OFFSET: 0x005A2F40
int32_t CGArenaTeamInfo::GetSelectedMemberIndex(uint32_t team) {
    if (team >= 3 || !CGArenaTeamInfo::m_selectedMember) {
        return -1;
    }

    uint32_t count = CGArenaTeamInfo::m_arenaTeams[team].m_numMembers;
    int32_t index = 0;

    if (!count) {
        return -1;
    }

    for (ArenaTeamMember** member = CGArenaTeamInfo::m_arenaTeams[team].m_members.m_data; (*member)->m_guid != CGArenaTeamInfo::m_selectedMember; member++) {
        if (++index >= count) {
            return -1;
        }
    }

    return index;
}

// VERIFY: 3cf409c5 manual
// OFFSET: 0x005A3600
void CGArenaTeamInfo::QueryRoster(uint32_t index) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || index >= 3) {
        return;
    }

    uint32_t* info = &player->m_player->m_playerFieldArenaTeamInfo11[index * 7];

    if (!info || !*info) {
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (CGArenaTeamInfo::m_nextAllowedQuery[index] && static_cast<int32_t>(now - CGArenaTeamInfo::m_nextAllowedQuery[index]) < 0) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_ROSTER));
    msg.Put(*info);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGArenaTeamInfo::m_nextAllowedQuery[index] = now + 10000;
}

// VERIFY: 45de7426 manual
// OFFSET: 0x005A3700
void CGArenaTeamInfo::UpdateTeams(int32_t signal) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    for (uint32_t i = 0; i < 3; i++) {
        uint32_t* info = i < 3 ? &player->m_player->m_playerFieldArenaTeamInfo11[i * 7] : nullptr;

        if (!info) {
            continue;
        }

        if (CGArenaTeamInfo::m_arenaTeams[i].m_teamId && !*info) {
            g_arenaTeamCache.Invalidate(CGArenaTeamInfo::m_arenaTeams[i].m_teamId);
        }

        CGArenaTeamInfo::m_arenaTeams[i].m_teamId = *info;

        if (*info) {
            CGArenaTeamInfo::QueryRoster(i);
        }
    }

    if (signal) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_UPDATE, nullptr);
    }
}

// VERIFY: de2c9b7d auto
// OFFSET: 0x005A3DC0
void CGArenaTeamInfo::EnterWorld() {
    CGArenaTeamInfo::UpdateTeams(0);
}

// VERIFY: 05e453ae auto
// OFFSET: 0x005A3DD0
void CGArenaTeamInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_ROSTER);
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_STATS);

    for (uint32_t i = 0; i < 3; i++) {
        CGArenaTeamInfo::m_arenaTeams[i].FreeMemberArray();
    }
}
