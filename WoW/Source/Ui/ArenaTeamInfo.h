#ifndef WOW_SOURCE_UI_ARENATEAMINFO_H
#define WOW_SOURCE_UI_ARENATEAMINFO_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Net/Types.h"

struct lua_State;
class CDataStore;

struct ArenaTeamMember {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ char m_name[48];
    /* 0038 */ uint32_t m_level;
    /* 003C */ uint32_t m_class;
    /* 0040 */ uint32_t m_rank;
    /* 0044 */ int32_t m_online;
    /* 0048 */ uint32_t m_weekGames;
    /* 004C */ uint32_t m_weekWins;
    /* 0050 */ uint32_t m_seasonGames;
    /* 0054 */ uint32_t m_seasonWins;
    /* 0058 */ uint32_t m_personalRating;
    /* 005C */ float m_matchmakerRatingBefore;
    /* 0060 */ float m_matchmakerRatingAfter;
    /* 0064 */ uint32_t unused_0064;
};

struct ArenaTeam {
    /* 0000 */ uint32_t m_teamId;
    /* 0004 */ TSGrowableArray<ArenaTeamMember*> m_members;
    /* 0014 */ uint32_t m_type;
    /* 0018 */ uint32_t m_numMembers;
    /* 001C */ uint32_t m_numOnline;
    /* 0020 */ uint32_t m_rating;
    /* 0024 */ uint32_t m_weekGames;
    /* 0028 */ uint32_t m_weekWins;
    /* 002C */ uint32_t m_seasonGames;
    /* 0030 */ uint32_t m_seasonWins;
    /* 0034 */ uint32_t m_rank;

    void FreeMemberArray();
};

struct ArenaTeamSort {
    /* 0000 */ int32_t m_column;
    /* 0004 */ int32_t m_reverse;
};

class CGArenaTeamInfo {
    public:
    static ArenaTeam m_arenaTeams[3];
    static int32_t m_showOffline;
    static uint32_t m_nextAllowedQuery[3];
    static ArenaTeamSort m_sortOrder[7];
    static WOWGUID m_selectedMember;
    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static void QueryRoster(uint32_t index);
    static void UpdateRoster(uint32_t index, int32_t reset);
    static void UpdateTeams(int32_t signal);
    static bool CheckPersonalAndTeamRating(uint32_t rating, int32_t bracket);
    static ArenaTeamMember* SelectMember(uint32_t team, uint32_t index);
    static void SetRosterShowOffline(int32_t showOffline);
    static void SortRoster(int32_t column);
    static int32_t GetSelectedMemberIndex(uint32_t team);
    static ArenaTeam* GetArenaTeam(uint32_t index);
};

void ArenaTeamInfoRegisterScriptFunctions();
void ArenaTeamInfoUnregisterScriptFunctions();
int32_t Packet_SMSG_ARENA_TEAM_STATS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_ARENA_TEAM_ROSTER(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

#endif
