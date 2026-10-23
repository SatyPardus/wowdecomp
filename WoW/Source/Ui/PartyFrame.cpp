#include "PartyFrame.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LfgDungeonsRec.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "WoW/Source/Object/Unit.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include <cstring>
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Os/Time.h"
#include <ctime>
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00BD1948
WOWGUID CGPartyInfo::m_members[4];

// STATIC: 0x00BD1978
uint32_t CGPartyInfo::m_dungeonDifficulty;

// STATIC: 0x00BD0D08
CGPartyMemberState s_partyMemberStates[4];

// STATIC: 0x00BD1968
WOWGUID CGPartyInfo::m_leader;

// STATIC: 0x00BD197C
uint32_t CGPartyInfo::m_raidDifficulty;

// STATIC: 0x00BD1980
int32_t CGPartyInfo::m_dynamicDifficulty;

// STATIC: 0x00BD19A8
int32_t CGPartyInfo::m_lfgRestricted;

// STATIC: 0x00ACC6C4
int32_t CGPartyInfo::m_leaderIndex = -1;

// STATIC: 0x00ACC6CC
int32_t CGPartyInfo::m_lootThreshold = 2;

// STATIC: 0x00BD1998
uint32_t CGPartyInfo::m_realNumMembers;

// STATIC: 0x00BD19A0
int32_t CGPartyInfo::m_optOutOfLoot;

// STATIC: 0x00BD19A4
int32_t CGPartyInfo::m_lfgSlot;

// STATIC: 0x00BD0CD0
uint32_t CGPartyInfo::m_memberRoles[4];

// STATIC: 0x00BD0D00
uint8_t CGPartyInfo::m_memberAssignments[4];

// STATIC: 0x00BD199C
uint32_t CGPartyInfo::m_lastAssignmentTime;

// STATIC: 0x00BD1984
uint32_t CGPartyInfo::m_nextDifficultyChange;

// STATIC: 0x00BD1988
uint8_t CGPartyInfo::m_playerAssignments;

// STATIC: 0x00BD198A
uint8_t CGPartyInfo::m_playerRole;

// STATIC: 0x00BD1989
uint8_t CGPartyInfo::m_playerLFGFlags;

// STATIC: 0x00BD1990
WOWGUID CGPartyInfo::m_realLeader;

// STATIC: 0x00ACC6C8
LOOT_METHOD CGPartyInfo::m_lootMethod = LOOT_METHOD_GROUP;

// STATIC: 0x00BD1970
WOWGUID CGPartyInfo::m_lootMaster;

// VERIFY: e02ce9a8 auto
// OFFSET: 0x0052BC50
WOWGUID GetPartyMemberGUID(uint32_t index) {
    if (index >= 4) {
        return WOWGUID();
    }

    return CGPartyInfo::m_members[index];
}

// VERIFY: b0246e04 manual
// OFFSET: 0x0052BC80
void CGPartyInfo::SetLeader(WOWGUID leader) {
    if (leader == CGPartyInfo::m_leader) {
        return;
    }

    CGPartyInfo::m_leader = leader;
    CGPartyInfo::m_leaderIndex = -1;

    if (leader) {
        for (int32_t i = 0; i < 4; i++) {
            if (CGPartyInfo::m_members[i] == leader) {
                CGPartyInfo::m_leaderIndex = i;
                break;
            }
        }
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_LEADER_CHANGED);
}

// VERIFY: ad92fae3 auto
// OFFSET: 0x0052BD10
uint32_t CGPartyInfo::GetNumPartyMembers() {
    uint32_t count = 0;

    for (uint32_t i = 0; i < sizeof(CGPartyInfo::m_members) / sizeof(CGPartyInfo::m_members[0]); i++) {
        if (CGPartyInfo::m_members[i]) {
            count++;
        }
    }

    return count;
}

// VERIFY: 17f136a5 auto
// OFFSET: 0x0052C6E0
WOWGUID GetPartyPetGUID(uint32_t index) {
    if (index >= 4 || !CGPartyInfo::m_members[index]) {
        return WOWGUID();
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPartyInfo::m_members[index], TYPE_PLAYER, __FILE__, __LINE__));

    if (unit) {
        const WOWGUID& pet = unit->m_unit->m_unitFieldCharm ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;
        return pet;
    }

    if (s_partyMemberStates[index].m_flags & 0x1) {
        return s_partyMemberStates[index].m_pet.m_guid;
    }

    return WOWGUID();
}

// VERIFY: f87f490a auto
// OFFSET: 0x004FB0B0
bool CGPartyInfo::InParty() {
    return CGPartyInfo::m_members[0].m_guid != 0;
}

// VERIFY: 6e61d8e7 auto
// OFFSET: 0x0052C0A0
void CGPartyInfo::SetLfgSlot(int32_t slot) {
    if (CGPartyInfo::m_lfgSlot != slot) {
        CGPartyInfo::m_lfgSlot = slot;
        FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE, nullptr);
    }
}

// VERIFY: 37904223 auto
// OFFSET: 0x0052C0D0
void CGPartyInfo::SetLfgRestricted(int32_t restricted) {
    if ((CGPartyInfo::m_lfgRestricted != 0) != (restricted != 0)) {
        CGPartyInfo::m_lfgRestricted = restricted;
        FrameScript_SignalEvent(SCRIPT_EVENT_PARTY_LFG_RESTRICTED, nullptr);
    }
}

// VERIFY: 02b3d047 auto
// OFFSET: 0x0052C3C0
static void PartyMemberNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        char fullName[256];

        if (name->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
        } else {
            SStrCopy(fullName, name->m_name, sizeof(fullName));
        }

        CGAutoCompleteName::SetName(guid, fullName, false);
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBERS_CHANGED);
}

// VERIFY: d97eb33e manual
// OFFSET: 0x0052C680
int32_t CGPartyInfo::IsMember(const WOWGUID& guid) {
    if (!guid) {
        return 0;
    }

    if (guid == ClntObjMgrGetActivePlayer()) {
        return 1;
    }

    for (uint32_t i = 0; i < sizeof(CGPartyInfo::m_members) / sizeof(CGPartyInfo::m_members[0]); i++) {
        if (CGPartyInfo::m_members[i] == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 4450fafb auto
// OFFSET: 0x0052BD60
void CGPartyInfo::UpdateRealStats(uint32_t count, WOWGUID leader) {
    if (count <= 4) {
        CGPartyInfo::m_realNumMembers = count;
    }

    CGPartyInfo::m_realLeader = leader;
}

// VERIFY: d56980be auto
// OFFSET: 0x0052BAE0
int32_t CanSendPartyCommand() {
    return !CGPartyInfo::m_lastAssignmentTime || static_cast<int32_t>(OsGetAsyncTimeMs() - CGPartyInfo::m_lastAssignmentTime - 750) >= 0;
}

// VERIFY: 8aac24a4 auto
// OFFSET: 0x0052BB10
int32_t Packet_SMSG_INSTANCE_SAVE_CREATED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t debug;
    char text[3000];
    msg->Get(debug);

    if (debug == 0) {
        SStrCopy(text, FrameScript_GetText("INSTANCE_SAVED", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        CGGameUI::SetInstanceLockTime(0, 0, 0);
    } else if (debug == 1) {
        SStrPrintf(text, sizeof(text), "(Debug-Only Lock Notice) %s", FrameScript_GetText("INSTANCE_SAVED", -1, GENDER_NOT_APPLICABLE));
    }

    CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: daab7afc auto
// OFFSET: 0x0052BBC0
static void LootMasterNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_NEW_LOOT_MASTER_S, name->m_name);
    }
}

// VERIFY: 7acb8197 auto
// OFFSET: 0x0052BD90
int32_t CGPartyInfo::SetLootMethod(LOOT_METHOD method, WOWGUID master, int32_t threshold, int32_t force) {
    int32_t changed = 0;

    if (force || CGPartyInfo::m_lootMethod != method) {
        CGPartyInfo::m_lootMethod = method;

        switch (method) {
            case LOOT_METHOD_FREEFORALL:
                CGGameUI::DisplayError(ERR_SET_LOOT_FREEFORALL);
                break;

            case LOOT_METHOD_ROUNDROBIN:
                CGGameUI::DisplayError(ERR_SET_LOOT_ROUNDROBIN);
                break;

            case LOOT_METHOD_MASTERLOOTER:
                CGGameUI::DisplayError(ERR_SET_LOOT_MASTER);
                break;

            case LOOT_METHOD_GROUP:
                CGGameUI::DisplayError(ERR_SET_LOOT_GROUP);
                break;

            case LOOT_METHOD_NEEDBEFOREGREED:
                CGGameUI::DisplayError(ERR_SET_LOOT_NBG);
                break;

            default:
                break;
        }

        changed = 1;
    }

    if (force || CGPartyInfo::m_lootMaster != master) {
        CGPartyInfo::m_lootMaster = master;

        if (master && method == LOOT_METHOD_MASTERLOOTER) {
            NameCache* name = g_nameDBCache.GetRecord(master.m_guid, master, LootMasterNameCallback, nullptr, false);

            if (name) {
                CGGameUI::DisplayError(ERR_NEW_LOOT_MASTER_S, name->m_name);
            }
        }

        changed = 1;
    }

    if (force || CGPartyInfo::m_lootThreshold != threshold) {
        CGPartyInfo::m_lootThreshold = threshold;
        char token[32];
        SStrPrintf(token, sizeof(token), "ITEM_QUALITY%d_DESC", threshold);
        CGGameUI::DisplayError(ERR_SET_LOOT_THRESHOLD_S, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE));
        changed = 1;
    }

    if (changed) {
        Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_LOOT_METHOD_CHANGED);
    }

    return changed;
}

// VERIFY: adab4393 manual
// OFFSET: 0x0052BEE0
CGPartyMemberState* CGPartyInfo::GetRemoteStats(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < sizeof(CGPartyInfo::m_members) / sizeof(CGPartyInfo::m_members[0]); i++) {
        if (CGPartyInfo::m_members[i] == guid) {
            return &s_partyMemberStates[i];
        }
    }

    return nullptr;
}

// VERIFY: 25905513 manual
// OFFSET: 0x0052BF80
CGPartyPetState* CGPartyInfo::GetRemotePetStats(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < sizeof(CGPartyInfo::m_members) / sizeof(CGPartyInfo::m_members[0]); i++) {
        if (CGPartyInfo::m_members[i] && (s_partyMemberStates[i].m_flags & 1) && s_partyMemberStates[i].m_pet.m_guid == guid) {
            return &s_partyMemberStates[i].m_pet;
        }
    }

    return nullptr;
}

// VERIFY: 97ee209c manual
// OFFSET: 0x0052C9A0
uint8_t CGPartyInfo::GetPartyAssignments(const WOWGUID& guid) {
    if (guid == ClntObjMgrGetActivePlayer()) {
        return CGPartyInfo::m_playerAssignments;
    }

    for (uint32_t i = 0; i < 4; i++) {
        WOWGUID member = i < 4 ? CGPartyInfo::m_members[i] : WOWGUID();

        if (member == guid) {
            return CGPartyInfo::m_memberAssignments[i];
        }
    }

    return 0;
}

// VERIFY: f781474b manual
// OFFSET: 0x0052CA00
int32_t CGPartyInfo::GetPartyAssignments(lua_State* L, int32_t assignment) {
    if (CGRaidInfo::m_numMembers) {
        return CGRaidInfo::GetPartyAssignments(L, assignment);
    }

    uint8_t flag = 0;

    if (assignment == 0) {
        flag = 2;
    } else if (assignment == 1) {
        flag = 4;
    }

    if (lua_isstring(L, 2)) {
        const char* name = lua_tolstring(L, 2, nullptr);
        WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, 5, StringToBOOL(L, 3, 0), 3.4028235e38f, 0);

        if (!guid) {
            CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
            return 0;
        }

        if (CGPartyInfo::GetPartyAssignments(guid) & flag) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        return 1;
    }

    int32_t count = 0;

    for (uint32_t i = 1; i <= 4; i++) {
        if (flag & CGPartyInfo::m_memberAssignments[i - 1]) {
            lua_checkstack(L, ++count);
            lua_pushnumber(L, i);
        }
    }

    return count;
}

// VERIFY: 73fe76db manual
// OFFSET: 0x0052CB40
void CGPartyInfo::SetOptOutOfLoot(int32_t optOut) {
    CGPartyInfo::m_optOutOfLoot = optOut;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_OPT_OUT_OF_LOOT));
    msg.Put(static_cast<uint32_t>(optOut));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 08431a1f auto
// OFFSET: 0x0052CBC0
bool CGGameUI::CanChangePlayerDifficulty() {
    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (!map || map->m_instanceType != 2 || !CGGameUI::IsDynamicDifficultyMap()) {
        return false;
    }

    if (CGRaidInfo::m_numMembers && CGPartyInfo::m_leader && CGPartyInfo::m_leader == ClntObjMgrGetActivePlayer()) {
        return true;
    }

    return false;
}

// VERIFY: 967e64dc auto
// OFFSET: 0x0052CC30
void CGPartyInfo::CheckClearLFGSlot() {
    int32_t count = CGPartyInfo::m_members[0] ? 1 : 0;

    if (CGPartyInfo::m_members[1]) {
        count++;
    }

    if (CGPartyInfo::m_members[2]) {
        count++;
    }

    if (CGPartyInfo::m_members[3]) {
        count++;
    }

    if (count || !CGPartyInfo::m_lfgSlot) {
        return;
    }

    const LfgDungeonsRec* dungeon = g_lfgDungeonsDB.GetRecord(CGPartyInfo::m_lfgSlot & 0xFFFFFF);

    if (dungeon && dungeon->m_mapID != CGGameUI::m_iCurrentMapID) {
        CGPartyInfo::m_lfgSlot = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_LFG_UPDATE, nullptr);
    }
}

// VERIFY: 6ce71a3a manual
// OFFSET: 0x0052D210
int32_t CGPartyInfo::IsMemberPet(const WOWGUID& guid, int32_t* index) {
    if (!guid) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        const WOWGUID& pet = player->m_unit->m_unitFieldCharm ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;

        if (pet == guid) {
            if (index) {
                *index = -1;
            }

            return 1;
        }
    }

    for (int32_t i = 0; i < 4; i++) {
        if (!CGPartyInfo::m_members[i]) {
            continue;
        }

        CGPlayer_C* member = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(CGPartyInfo::m_members[i], TYPE_PLAYER, __FILE__, __LINE__));
        bool match;

        if (member) {
            const WOWGUID& pet = member->m_unit->m_unitFieldCharm ? member->m_unit->m_unitFieldCharm : member->m_unit->m_unitFieldSummon;
            match = pet == guid;
        } else {
            match = (s_partyMemberStates[i].m_flags & 1) && s_partyMemberStates[i].m_pet.m_guid == guid;
        }

        if (match) {
            if (index) {
                *index = i;
            }

            return 1;
        }
    }

    return 0;
}

// VERIFY: c3b2e18b auto
// OFFSET: 0x0052D0E0
void CGPartyInfo::InitializeGame() {
    CGPartyInfo::m_members[0].m_guid = 0;
    CGPartyInfo::m_members[1].m_guid = 0;
    CGPartyInfo::m_members[2].m_guid = 0;
    CGPartyInfo::m_members[3].m_guid = 0;
    CGPartyInfo::m_memberAssignments[0] = 0;
    CGPartyInfo::m_memberAssignments[1] = 0;
    CGPartyInfo::m_memberAssignments[2] = 0;
    CGPartyInfo::m_memberAssignments[3] = 0;
    CGPartyInfo::m_memberRoles[0] = 0;
    CGPartyInfo::m_memberRoles[1] = 0;
    CGPartyInfo::m_memberRoles[2] = 0;
    CGPartyInfo::m_memberRoles[3] = 0;
    CGPartyInfo::m_leader.m_guid = 0;
    CGPartyInfo::m_leaderIndex = -1;
    CGPartyInfo::m_dungeonDifficulty = 0;
    CGPartyInfo::m_raidDifficulty = 0;
    CGPartyInfo::m_playerAssignments = 0;
    CGPartyInfo::m_playerLFGFlags = 0;
    CGPartyInfo::m_playerRole = 0;
    CGPartyInfo::m_realLeader.m_guid = 0;
    CGPartyInfo::m_realNumMembers = 0;
    CGPartyInfo::m_lootMethod = LOOT_METHOD_GROUP;
    CGPartyInfo::m_lootMaster.m_guid = 0;
    CGPartyInfo::m_lootThreshold = 2;
    CGPartyInfo::m_nextDifficultyChange = 0;
    CGPartyInfo::m_lfgSlot = 0;
    CGPartyInfo::m_lfgRestricted = 0;

    ClientServices::SetMessageHandler(SMSG_INSTANCE_SAVE_CREATED, Packet_SMSG_INSTANCE_SAVE_CREATED, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHANGE_PLAYER_DIFFICULTY_RESULT, Packet_SMSG_PLAYER_DIFFICULTY_CHANGE, nullptr);
    ConsoleCommandRegister("changePlayerDifficulty", CCommand_SetDifficulty, CATEGORY_DEFAULT, nullptr);
}

// VERIFY: 4f4bffd9 manual
// OFFSET: 0x0052C860
uint32_t CGPartyInfo::GetGroupRole(const WOWGUID& guid) {
    if (guid == WOWGUID()) {
        return 0;
    }

    if (guid == ClntObjMgrGetActivePlayer()) {
        return CGPartyInfo::m_playerRole;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (CGPartyInfo::m_members[i] == guid) {
            return CGPartyInfo::m_memberRoles[i];
        }
    }

    return 0;
}

// VERIFY: 942eebbc manual
// OFFSET: 0x0052C8C0
int32_t CGPartyInfo::IsPlayerOrPetInParty(const WOWGUID& guid) {
    if (!static_cast<uint32_t>(guid.m_guid)) {
        return static_cast<uint32_t>(guid.m_guid >> 32);
    }

    for (uint32_t i = 0; i < CGPartyInfo::GetNumPartyMembers(); i++) {
        WOWGUID member = i < 4 ? CGPartyInfo::m_members[i] : WOWGUID();

        if (guid == member) {
            return 1;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(i < 4 ? CGPartyInfo::m_members[i] : WOWGUID(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player) {
            CGUnitData* unit = player->m_unit;
            const WOWGUID& pet = unit->m_unitFieldCharm ? unit->m_unitFieldCharm : unit->m_unitFieldSummon;

            if (pet == guid) {
                return 1;
            }
        } else if ((s_partyMemberStates[i].m_flags & 1) && s_partyMemberStates[i].m_pet.m_guid == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: fc0197a5 manual
// OFFSET: 0x0052D310
int32_t CGPartyInfo::IsMemberOrPet(const WOWGUID& guid) {
    if (!guid) {
        return 0;
    }

    if (guid == ClntObjMgrGetActivePlayer()) {
        return 1;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        const WOWGUID& pet = player->m_unit->m_unitFieldCharm ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;

        if (pet == guid) {
            return 1;
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (!CGPartyInfo::m_members[i]) {
            continue;
        }

        if (CGPartyInfo::m_members[i] == guid) {
            return 1;
        }

        CGUnit_C* member = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPartyInfo::m_members[i], TYPE_PLAYER, __FILE__, __LINE__));

        if (member) {
            const WOWGUID& pet = member->m_unit->m_unitFieldCharm ? member->m_unit->m_unitFieldCharm : member->m_unit->m_unitFieldSummon;

            if (pet == guid) {
                return 1;
            }
        } else if ((s_partyMemberStates[i].m_flags & 1) && s_partyMemberStates[i].m_pet.m_guid == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 9e72f7d6 manual
// OFFSET: 0x0052BF30
CGPartyMemberState* CGPartyInfo::GetRemoteStatsFromPet(const WOWGUID& petGuid) {
    for (uint32_t i = 0; i < sizeof(CGPartyInfo::m_members) / sizeof(CGPartyInfo::m_members[0]); i++) {
        if (CGPartyInfo::m_members[i] && s_partyMemberStates[i].m_pet.m_guid == petGuid) {
            return &s_partyMemberStates[i];
        }
    }

    return nullptr;
}

// VERIFY: d1e4647b manual
// OFFSET: 0x0052D410
void CGPartyInfo::AddMember(WOWGUID guid, int32_t online, const CGPartyMemberState* state, uint8_t assignments, uint32_t role, const PartyReadyCheckInfo* readyCheck) {
    if (!guid) {
        return;
    }

    uint32_t index = 0;

    while (index < 4 && CGPartyInfo::m_members[index]) {
        index++;
    }

    if (index == 4) {
        return;
    }

    CGPartyInfo::m_members[index] = guid;
    CGPartyInfo::m_memberAssignments[index] = assignments;
    CGPartyInfo::m_memberRoles[index] = role;

    if (state) {
        memcpy(&s_partyMemberStates[index], state, sizeof(CGPartyMemberState));
    } else {
        memset(&s_partyMemberStates[index], 0, sizeof(CGPartyMemberState));
        s_partyMemberStates[index].m_health = 1;
        s_partyMemberStates[index].m_maxHealth = 1;
        s_partyMemberStates[index].m_power = 1;
        s_partyMemberStates[index].m_maxPower = 1;
    }

    if (online) {
        s_partyMemberStates[index].m_flags |= 0x1;
        CGAutoCompleteName::AddFlags(guid, 33, 0, nullptr);
    } else {
        s_partyMemberStates[index].m_flags &= ~0x1;
        CGAutoCompleteName::AddFlags(guid, 1, 0, nullptr);
        CGAutoCompleteName::RemoveFlags(guid, 32);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_PLAYER, __FILE__, __LINE__));

    if (unit) {
        unit->RegisterScript(16);
        const WOWGUID& petGuid = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;
        CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (pet) {
            pet->RegisterScript(32);
        }

        unit->UpdateReaction(true);
    } else if (!state) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PARTY_MEMBER_STATS));
        msg.Put(guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (!CGPartyInfo::m_realNumMembers) {
        CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player) {
            player->UpdateReaction(true);
        }
    }

    if (!online) {
        CGGameUI::ClearTarget(guid, 1);
    }

    if (readyCheck) {
        CGRaidInfo::m_partyReadyCheckPending[index] = readyCheck->m_pending;
        CGRaidInfo::m_partyReadyCheckReady[index] = readyCheck->m_ready;
    } else {
        CGRaidInfo::m_partyReadyCheckPending[index] = 0;
        CGRaidInfo::m_partyReadyCheckReady[index] = 0;
    }

    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, guid, PartyMemberNameCallback, nullptr, false);

    if (name) {
        char fullName[256];

        if (name->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
        } else {
            SStrCopy(fullName, name->m_name, sizeof(fullName));
        }

        CGAutoCompleteName::SetName(guid, fullName, false);
    }
}

// VERIFY: 295fee69 manual
// OFFSET: 0x0052D6E0
void CGPartyInfo::RemoveAll(int32_t a1) {
    uint32_t dungeonDifficulty = CGGameUI::GetCurrentDungeonDifficulty();
    uint32_t raidDifficulty = CGGameUI::GetCurrentRaidDifficulty();

    for (uint32_t i = 0; i < 4; i++) {
        if (!CGPartyInfo::m_members[i]) {
            continue;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPartyInfo::m_members[i], TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            const WOWGUID& petGuid = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;

            if (petGuid) {
                CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petGuid, TYPE_UNIT, __FILE__, __LINE__));

                if (pet) {
                    pet->UnregisterScript(32);
                } else if (a1) {
                    CGGameUI::ClearTarget(petGuid, 1);
                }
            }

            unit->UnregisterScript(16);
            unit->UpdateReaction(true);
        } else if (a1) {
            CGGameUI::ClearTarget(CGPartyInfo::m_members[i], 1);
        }

        CGPartyInfo::m_members[i] = WOWGUID();
        CGPartyInfo::m_memberAssignments[i] = 0;
        CGPartyInfo::m_memberRoles[i] = 0;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->UpdateReaction(true);
    }

    CGPartyInfo::m_leader = WOWGUID();
    CGPartyInfo::m_leaderIndex = 0;
    CGPartyInfo::m_playerAssignments = 0;
    CGPartyInfo::m_playerLFGFlags = 0;
    CGPartyInfo::m_playerRole = 0;
    CGPartyInfo::m_lfgSlot = 0;
    CGPartyInfo::m_lfgRestricted = 0;
    CGAutoCompleteName::RemoveFlagsFromAllNodes(1);

    if (a1) {
        CGGameUI::CheckInstanceDifficultyMessages(dungeonDifficulty, raidDifficulty);
        Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBERS_CHANGED);
        Script_SendUniqueSignal(SCRIPT_EVENT_PLAYER_ROLES_ASSIGNED);
    }
}

// VERIFY: 8bd59f6a auto
// OFFSET: 0x0052D8A0
void CGPartyInfo::ResetLootMethod() {
    CGPartyInfo::m_lootMethod = LOOT_METHOD_GROUP;
    CGPartyInfo::m_lootMaster = WOWGUID();
    CGPartyInfo::m_lootThreshold = 2;
    CGPartyInfo::SetOptOutOfLoot(0);
}

// VERIFY: 3eaac349 auto
// OFFSET: 0x0052D8D0
void CGPartyInfo::ChangeRaidDifficulty(int32_t heroic) {
    if (!CGGameUI::CanChangePlayerDifficulty()) {
        return;
    }

    if (CGPartyInfo::m_nextDifficultyChange && static_cast<int32_t>(static_cast<uint32_t>(::time(nullptr)) - CGPartyInfo::m_nextDifficultyChange) < 0) {
        char duration[1024];
        CGTooltip::GetDurationString(duration, sizeof(duration), static_cast<uint32_t>(1000 * (CGPartyInfo::m_nextDifficultyChange - static_cast<uint32_t>(::time(nullptr)))), "INT_SPELL_DURATION", nullptr, 1, 0);
        CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_COOLDOWN_S, duration);
        return;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHANGEPLAYER_DIFFICULTY));
    msg.Put(static_cast<uint32_t>(heroic));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d7980a8a auto
// OFFSET: 0x00572420
CGPartyMemberState* CGPartyInfo::GetMemberState(uint32_t index) {
    if (CGPartyInfo::m_members[index]) {
        return &s_partyMemberStates[index];
    }

    return nullptr;
}

// VERIFY: ebe19d7d auto
// OFFSET: 0x0052BFF0
void CGPartyInfo::GetReadyCheckInfo(PartyReadyCheckInfo* info, uint32_t index) {
    info->m_guid = WOWGUID();
    info->m_pending = 0;
    info->m_ready = 0;

    if (index < 4) {
        info->m_guid = CGPartyInfo::m_members[index];
        info->m_pending = CGRaidInfo::m_partyReadyCheckPending[index];
        info->m_ready = CGRaidInfo::m_partyReadyCheckReady[index];
    }
}

// VERIFY: fe995f4a auto
// OFFSET: 0x0052C040
void CGPartyInfo::SetReadyCheckInfo(const PartyReadyCheckInfo* info, uint32_t index) {
    if (index < 4 && CGPartyInfo::m_members[index] == info->m_guid) {
        CGRaidInfo::m_partyReadyCheckPending[index] = info->m_pending;
        CGRaidInfo::m_partyReadyCheckReady[index] = info->m_ready;
    }
}

// VERIFY: 141be72d auto
// OFFSET: 0x0052C080
void CGPartyInfo::ClearRecruitAFriend(WOWGUID guid) {
    CGPartyMemberState* stats = CGPartyInfo::GetRemoteStats(guid);

    if (stats) {
        stats->m_flags &= ~0x100;
    }
}

// VERIFY: 93e9c613 manual
// OFFSET: 0x0052C760
void CGPartyInfo::EnableMember(CGPlayer_C* player, int32_t enable) {
    WOWGUID guid = player->m_obj->m_guid;
    uint32_t index;

    for (index = 0; index < 4; index++) {
        if (CGPartyInfo::m_members[index] == guid) {
            break;
        }
    }

    if (index == 4) {
        return;
    }

    if (enable) {
        Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBER_ENABLE);
        return;
    }

    player->FillPartyMemberState(&s_partyMemberStates[index]);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PARTY_MEMBER_STATS));
    msg.Put(player->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);

    Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBER_DISABLE);
}

// VERIFY: 4289bd20 auto
// OFFSET: 0x0052BC10
void CGPartyInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(MSG_SET_DUNGEON_DIFFICULTY);
    ClientServices::ClearMessageHandler(MSG_SET_RAID_DIFFICULTY);
    ClientServices::ClearMessageHandler(SMSG_INSTANCE_SAVE_CREATED);
    ClientServices::ClearMessageHandler(SMSG_CHANGE_PLAYER_DIFFICULTY_RESULT);
    ConsoleCommandUnregister("changePlayerDifficulty");
}

// VERIFY: 1c547e21 auto
// OFFSET: 0x0052C460
int32_t Packet_SMSG_PLAYER_DIFFICULTY_CHANGE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t result;
    uint8_t difficulty;
    uint32_t value;
    char duration[1024];
    char token[64];
    msg->Get(result);

    switch (result) {
        case 0:
            msg->Get(difficulty);
            CGPartyInfo::m_dynamicDifficulty = difficulty;
            break;

        case 1:
            msg->Get(value);
            CGTooltip::GetDurationString(duration, sizeof(duration), 1000 * value, "INT_SPELL_DURATION", nullptr, 1, 0);
            CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_COOLDOWN_S, duration);
            break;

        case 2:
            CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_WORLDSTATE);
            break;

        case 3:
            CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_ENCOUNTER);
            break;

        case 4:
            CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_COMBAT);
            break;

        case 5:
            CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_PLAYER_BUSY);
            break;

        case 6:
            msg->Get(value);
            CGPartyInfo::m_nextDifficultyChange = value + static_cast<uint32_t>(::time(nullptr));
            LoadingScreenEnable(CGGameUI::m_iCurrentMapID, false);
            break;

        case 7:
            CGGameUI::DisplayError(ERR_DIFFICULTY_CHANGE_ALREADY_STARTED);
            break;

        case 8: {
            msg->Get(value);
            MapDifficultyRec* rec = g_mapDifficultyDB.GetRecord(value);

            if (rec && rec->m_message) {
                CGChat::AddChatMessage(rec->m_message, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            }

            break;
        }

        case 9:
            LoadingScreenDisable();
            ConsoleWrite("Changed difficulty successfully", DEFAULT_COLOR);
            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_DIFFICULTY_CHANGED, nullptr);
            SStrPrintf(token, sizeof(token), "PLAYER_DIFFICULTY%d", CGPartyInfo::m_dynamicDifficulty + 1);
            CGGameUI::DisplayError(ERR_PLAYER_DIFFICULTY_CHANGED_S, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE));
            break;
    }

    return 1;
}

// VERIFY: 940f24d5 manual
// OFFSET: 0x0052D060
int32_t CCommand_SetDifficulty(const char* command, const char* arguments) {
    uint32_t difficulty = SStrToInt(arguments);

    if (difficulty > 1) {
        ConsoleWrite("Invalid difficulty", ERROR_COLOR);
        return 1;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHANGEPLAYER_DIFFICULTY));
    msg.Put(difficulty);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 1;
}

// VERIFY: a24a260e manual
// OFFSET: 0x0052D1E0
void CGPartyInfo::LeaveWorld() {
    if (ClntObjMgrGetActivePlayerObj()) {
        CGPlayer_C::DeclineGroup();
    }
}
