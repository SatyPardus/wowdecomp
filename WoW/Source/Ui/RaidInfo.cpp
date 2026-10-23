#include "RaidInfo.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/Unit.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "Engine/Source/Os/TimeManager.h"
#include <malloc.h>
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/FrameXML/LoadXML.h"

// STATIC: 0x00BEB568
CGRaidMember* CGRaidInfo::m_members[40];

// STATIC: 0x00BEB608
uint32_t CGRaidInfo::m_numMembers;

// STATIC: 0x00BEB618
static int32_t s_pendingNameQueries;

// STATIC: 0x00BEB528
WOWGUID CGRaidInfo::m_raidTargets[8];

// STATIC: 0x00BEB60C
uint32_t CGRaidInfo::m_realNumMembers;

// STATIC: 0x00BEB610
WOWGUID CGRaidInfo::m_selectedMember;

// STATIC: 0x00BEB61C
uint32_t CGRaidInfo::m_readyCheckTime;

// STATIC: 0x00BEB620
WOWGUID CGRaidInfo::m_readyCheckInitiator;

// STATIC: 0x00BEB628
uint32_t CGRaidInfo::m_readyCheckTimer;

// STATIC: 0x00BD0CE0
int32_t CGRaidInfo::m_partyReadyCheckReady[4];

// STATIC: 0x00BD0CF0
int32_t CGRaidInfo::m_partyReadyCheckPending[4];

// STATIC: 0x00ACE788
uint8_t CGRaidInfo::m_rosterCounter = 0xFF;

// VERIFY: 43f57544 auto
// OFFSET: 0x00572490
static void RaidMemberRemovedNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_RAID_MEMBER_REMOVED_S, name->m_name);
    }
}

// VERIFY: 4899d647 auto
// OFFSET: 0x00572530
void CGRaidInfo::UpdateRealStats(uint32_t count, WOWGUID leader) {
    if (count <= 40) {
        CGRaidInfo::m_realNumMembers = count;
    }

    CGPartyInfo::m_realLeader = leader;
}

// VERIFY: 7768a0f0 auto
// OFFSET: 0x00572560
int32_t Packet_MSG_RAID_READY_CHECK_FINISHED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGRaidInfo::m_readyCheckTime = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_READY_CHECK_FINISHED, "%b", 0);
    CGRaidInfo::m_readyCheckInitiator = WOWGUID();
    return 1;
}

// VERIFY: 1ed45d01 auto
// OFFSET: 0x005725A0
int32_t Packet_SMSG_RAID_READY_CHECK_ERROR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_READY_CHECK_THROTTLED);
    return 1;
}

// VERIFY: 495286f8 manual
// OFFSET: 0x005725C0
int32_t Packet_MSG_NOTIFY_PARTY_SQUELCH(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t squelched;
    msg->Get(squelched);

    if (squelched) {
        CGGameUI::DisplayError(ERR_PLAYER_SILENCED);
    } else {
        CGGameUI::DisplayError(ERR_PLAYER_UNSILENCED);
    }

    return 1;
}

// VERIFY: 03dd0fd2 auto
// OFFSET: 0x00572610
int32_t Packet_SMSG_ECHO_PARTY_SQUELCH(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint32_t squelched;
    msg->Get(guid.m_guid);
    msg->Get(squelched);
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return 1;
    }

    if (!squelched) {
        CGGameUI::DisplayError(ERR_PLAYER_UNSILENCED_ECHO, name->m_name);
        return 1;
    }

    CGGameUI::DisplayError(ERR_PLAYER_SILENCED_ECHO, name->m_name);
    return 1;
}

// VERIFY: 31ede3a3 auto
// OFFSET: 0x005724E0
static void SignalReadyCheckConfirmEvents(const WOWGUID& guid, int32_t ready) {
    int32_t count;
    char** tokens = Script_GetTokenIDsFromGUID(guid, &count);

    for (int32_t i = 0; i < count; i++) {
        FrameScript_SignalEvent(SCRIPT_EVENT_READY_CHECK_CONFIRM, "%s%b", tokens[i], ready);
    }
}

// VERIFY: a7e70558 manual
// OFFSET: 0x00572690
int32_t CGRaidInfo::GetSelectedMemberIndex() {
    if (!CGRaidInfo::m_selectedMember) {
        return -1;
    }

    uint32_t i = 0;

    if (!CGRaidInfo::m_numMembers) {
        return -1;
    }

    while (CGRaidInfo::m_members[i]->m_guid != CGRaidInfo::m_selectedMember) {
        if (++i >= CGRaidInfo::m_numMembers) {
            return -1;
        }
    }

    return i;
}

// VERIFY: df976625 manual
// OFFSET: 0x005726F0
int32_t IsGuidInMyRaid(const WOWGUID& guid) {
    if (!guid) {
        return 0;
    }

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        if (CGRaidInfo::m_members[i]->m_guid == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 2f9aefc8 manual
// OFFSET: 0x00572740
int32_t CGRaidInfo::IsPlayerRaidOfficer(const WOWGUID& guid) {
    if (!guid) {
        return 0;
    }

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        if (CGRaidInfo::m_members[i]->m_guid == guid) {
            return CGRaidInfo::m_members[i]->m_rank > 0 || CGPartyInfo::m_leader == guid;
        }
    }

    return 0;
}

// VERIFY: 0cbc8b71 manual
// OFFSET: 0x00572ED0
void CGRaidInfo::SetRaidTarget(uint32_t index, const WOWGUID& guid) {
    CGUnit_C* previous = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGRaidInfo::m_raidTargets[index], TYPE_UNIT, __FILE__, __LINE__));
    CGRaidInfo::m_raidTargets[index] = guid;

    if (previous) {
        previous->NamePlateUpdateRaidTarget();
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->NamePlateUpdateRaidTarget();
    }
}

// VERIFY: 3bba49eb auto
// OFFSET: 0x00572F50
int32_t CGRaidInfo::IsPVPRaid() {
    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (map && map->m_instanceType == 3) {
        return 1;
    }

    return map->m_instanceType == 4;
}

// VERIFY: 0cb73745 auto
// OFFSET: 0x00572D60
void RaidRosterNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!s_pendingNameQueries || !--s_pendingNameQueries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_RAID_ROSTER_UPDATE, nullptr);
        Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBERS_CHANGED);
    }
}

// VERIFY: 4fd1911a auto
// OFFSET: 0x00572D90
static void RaidMemberAddedNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return;
    }

    CGGameUI::DisplayError(ERR_RAID_MEMBER_ADDED_S, name->m_name);
    char fullName[256];

    if (name->m_realmName[0]) {
        SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
    } else {
        SStrCopy(fullName, name->m_name, sizeof(fullName));
    }

    CGAutoCompleteName::SetName(guid, fullName, false);
}

// VERIFY: 642f8d2b auto
// OFFSET: 0x00572E40
void CGRaidInfo::EnterWorld() {
    uint32_t i = 0;

    for (; i < CGRaidInfo::m_numMembers; i++) {
        if (!CGRaidInfo::m_members[i]->m_guid) {
            break;
        }
    }

    if (i >= CGRaidInfo::m_numMembers) {
        return;
    }

    CGRaidInfo::m_members[i]->m_guid = ClntObjMgrGetActivePlayer();
    CGRaidMember* member = CGRaidInfo::m_members[i];

    if (g_nameDBCache.GetRecord(member->m_guid.m_guid, member->m_guid, RaidRosterNameCallback, nullptr, false)) {
        if (!s_pendingNameQueries) {
            FrameScript_SignalEvent(SCRIPT_EVENT_RAID_ROSTER_UPDATE, nullptr);
        }
    } else {
        s_pendingNameQueries++;
    }
}

// VERIFY: 6bb7db76 manual
// OFFSET: 0x005728C0
uint32_t GetRaidTargetIndexFromGuid(WOWGUID guid) {
    uint32_t index = 0;

    while (CGRaidInfo::m_raidTargets[index] != guid) {
        if (++index >= sizeof(CGRaidInfo::m_raidTargets) / sizeof(CGRaidInfo::m_raidTargets[0])) {
            return 8;
        }
    }

    return index;
}

// VERIFY: 37639a60 manual
// OFFSET: 0x005727B0
CGPartyMemberState* CGRaidInfo::GetRemoteStats(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < sizeof(CGRaidInfo::m_members) / sizeof(CGRaidInfo::m_members[0]); i++) {
        if (CGRaidInfo::m_members[i]->m_guid == guid) {
            return &CGRaidInfo::m_members[i]->m_stats;
        }
    }

    return nullptr;
}

// VERIFY: a4ba6b2b manual
// OFFSET: 0x00572850
CGPartyPetState* CGRaidInfo::GetRemotePetStats(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < sizeof(CGRaidInfo::m_members) / sizeof(CGRaidInfo::m_members[0]); i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];

        if (member->m_guid && (member->m_stats.m_flags & 1) && member->m_stats.m_pet.m_guid == guid) {
            return &CGRaidInfo::m_members[i]->m_stats.m_pet;
        }
    }

    return nullptr;
}

// VERIFY: 52124f6c manual
// OFFSET: 0x00573150
int32_t CGRaidInfo::IsPetInRaid(const WOWGUID& guid, int32_t* index) {
    if (!guid) {
        return 0;
    }

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];

        if (!member) {
            continue;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(member->m_guid, TYPE_PLAYER, __FILE__, __LINE__));
        bool match;

        if (player) {
            const WOWGUID& pet = player->m_unit->m_unitFieldCharm ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;
            match = pet == guid;
        } else {
            match = (member->m_stats.m_flags & 1) && member->m_stats.m_pet.m_guid == guid;
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

// VERIFY: ded99c24 manual
// OFFSET: 0x005743B0
int32_t Packet_MSG_RAID_TARGET_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID setter = WOWGUID();
    uint8_t full;
    msg->Get(full);

    if (full) {
        WOWGUID previous[8];
        memcpy(previous, CGRaidInfo::m_raidTargets, sizeof(previous));
        memset(CGRaidInfo::m_raidTargets, 0, sizeof(CGRaidInfo::m_raidTargets));

        for (uint32_t i = 0; i < 8; i++) {
            if (previous[i]) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(previous[i], TYPE_UNIT, __FILE__, __LINE__));

                if (unit) {
                    unit->NamePlateUpdateRaidTarget();
                }
            }
        }
    } else {
        msg->Get(setter.m_guid);
    }

    uint8_t index = 0;
    WOWGUID target = WOWGUID();

    while (!msg->IsRead()) {
        msg->Get(index);
        msg->Get(target.m_guid);

        if (index < 8) {
            CGRaidInfo::SetRaidTarget(index, target);
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_RAID_TARGET_UPDATE, nullptr);

    if (!full && setter && target) {
        CGChat::InformTargetIcon(setter, target, index);
    }

    return 1;
}

// VERIFY: 9364bff5 manual
// OFFSET: 0x005744F0
int32_t Packet_MSG_RAID_READY_CHECK_CONFIRM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid = WOWGUID();
    msg->Get(guid.m_guid);
    uint8_t ready = 0;

    if (!msg->IsRead()) {
        msg->Get(ready);
    }

    bool pending = false;
    bool found = false;
    char buffer[256];

    if (CGRaidInfo::m_numMembers) {
        bool initiator = CGRaidInfo::m_readyCheckInitiator == ClntObjMgrGetActivePlayer();

        for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
            CGRaidMember* member = CGRaidInfo::m_members[i];

            if (member->m_guid == guid) {
                member->m_readyCheckPending = 0;
                CGRaidInfo::m_members[i]->m_readyCheckReady = ready;
                found = true;

                if (!ready) {
                    WOWGUID none = WOWGUID();
                    NameCache* name = g_nameDBCache.GetRecord(CGRaidInfo::m_members[i]->m_guid.m_guid, none, nullptr, nullptr, false);

                    if (name && initiator) {
                        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("RAID_MEMBER_NOT_READY", -1, GENDER_NOT_APPLICABLE), name->m_name);
                        CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
                    }
                }

                SignalReadyCheckConfirmEvents(guid, ready);
            }

            member = CGRaidInfo::m_members[i];

            if (pending || ((member->m_stats.m_flags & 0x1) && member->m_readyCheckPending)) {
                pending = true;

                if (found) {
                    return 1;
                }
            } else {
                pending = false;
            }
        }

        if (pending) {
            return 1;
        }
    } else {
        for (uint32_t i = 0; i < CGPartyInfo::GetNumPartyMembers(); i++) {
            PartyReadyCheckInfo info;
            CGPartyInfo::GetReadyCheckInfo(&info, i);

            if (info.m_guid == guid) {
                info.m_pending = 0;
                info.m_ready = ready;
                CGPartyInfo::SetReadyCheckInfo(&info, i);
                found = true;

                if (!ready) {
                    WOWGUID none = WOWGUID();
                    NameCache* name = g_nameDBCache.GetRecord(info.m_guid.m_guid, none, nullptr, nullptr, false);

                    if (name) {
                        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("RAID_MEMBER_NOT_READY", -1, GENDER_NOT_APPLICABLE), name->m_name);
                        CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
                    }
                }

                SignalReadyCheckConfirmEvents(guid, ready);
            }

            CGPartyMemberState* state = CGPartyInfo::m_members[i] ? &s_partyMemberStates[i] : nullptr;

            if (pending || (state && (state->m_flags & 0x1) && i < 4 && CGRaidInfo::m_partyReadyCheckPending[i])) {
                pending = true;

                if (found) {
                    return 1;
                }
            } else {
                pending = false;
            }
        }

        if (pending) {
            return 1;
        }
    }

    CGRaidInfo::m_readyCheckTime = OsGetAsyncTimeMs() - 1;
    CGRaidInfo::CheckReadyCheckTime();
    return 1;
}

// VERIFY: 4dc0a7ac manual
// OFFSET: 0x005747D0
void CGRaidInfo::InitiateReadyCheck(const char* name) {
    uint32_t now = OsGetAsyncTimeMs();
    uint32_t remaining;

    if (CGRaidInfo::m_readyCheckTime && static_cast<int32_t>(now - CGRaidInfo::m_readyCheckTime) < 0) {
        remaining = CGRaidInfo::m_readyCheckTime - now;
    } else {
        remaining = 0;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_READY_CHECK, "%s%d", name, remaining / 1000);

    if (CGRaidInfo::m_readyCheckInitiator != ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_RAID_LEADER_READY_CHECK_START_S, name);
        return;
    }

    if (CGRaidInfo::m_readyCheckTimer) {
        ClientKillTimer(CGRaidInfo::m_readyCheckTimer, CGRaidInfo::ProcessReadyCheckTimer, "ProcessReadyCheckTimer");
        CGRaidInfo::m_readyCheckTimer = 0;
    }

    CGRaidInfo::m_readyCheckTimer = ClientSetTimer(30000, CGRaidInfo::ProcessReadyCheckTimer, nullptr);
}

// VERIFY: be40c141 auto
// OFFSET: 0x00574D00
static void ReadyCheckNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        CGRaidInfo::InitiateReadyCheck(name->m_name);
    }
}

// VERIFY: a30fa835 auto
// OFFSET: 0x00574D50
int32_t Packet_MSG_RAID_READY_CHECK(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID initiator;
    msg->Get(initiator.m_guid);
    CGRaidInfo::m_readyCheckInitiator = initiator;
    CGRaidInfo::m_readyCheckTime = OsGetAsyncTimeMs() + 30000;

    if (initiator == ClntObjMgrGetActivePlayer()) {
        CGChat::AddChatMessage(FrameScript_GetText("READY_CHECK_START", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (CGRaidInfo::m_numMembers) {
        CGRaidMember* self = CGRaidInfo::FindMemberByGuid(ClntObjMgrGetActivePlayer());

        if (self && self->m_rank > 0) {
            for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
                CGRaidMember* member = CGRaidInfo::m_members[i];

                if (member->m_guid) {
                    if (member->m_guid == initiator) {
                        member->m_readyCheckPending = 0;
                        CGRaidInfo::m_members[i]->m_readyCheckReady = 1;
                    } else {
                        member->m_readyCheckPending = 1;
                    }
                }
            }
        }
    } else if (CGPartyInfo::m_leader == ClntObjMgrGetActivePlayer()) {
        for (uint32_t i = 0; i < CGPartyInfo::GetNumPartyMembers(); i++) {
            if (GetPartyMemberGUID(i)) {
                PartyReadyCheckInfo info;
                CGPartyInfo::GetReadyCheckInfo(&info, i);

                if (info.m_guid == initiator) {
                    info.m_pending = 0;
                    info.m_ready = 1;
                } else {
                    info.m_pending = 1;
                }

                CGPartyInfo::SetReadyCheckInfo(&info, i);
            }
        }
    }

    NameCache* name = g_nameDBCache.GetRecord(initiator.m_guid, initiator, &ReadyCheckNameCallback, nullptr, false);

    if (name) {
        CGRaidInfo::InitiateReadyCheck(name->m_name);
    }

    return 1;
}

// VERIFY: ccf05258 manual
// OFFSET: 0x00574880
void CGRaidInfo::RosterUpdateRefreshReadyCheck(uint32_t removed, WOWGUID leader) {
    if (!CGRaidInfo::m_numMembers || !CGPartyInfo::m_members[0] || leader != CGPartyInfo::m_leader) {
        CGRaidInfo::m_readyCheckTime = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_READY_CHECK_FINISHED, "%b", 1);
        CGRaidInfo::m_readyCheckInitiator = WOWGUID();

        if (CGRaidInfo::m_readyCheckTimer) {
            ClientKillTimer(CGRaidInfo::m_readyCheckTimer, CGRaidInfo::ProcessReadyCheckTimer, "ProcessReadyCheckTimer");
            CGRaidInfo::m_readyCheckTimer = 0;
        }

        return;
    }

    WOWGUID player = ClntObjMgrGetActivePlayer();
    uint32_t count = CGRaidInfo::m_numMembers;
    bool done;

    if (count) {
        if (!CGRaidInfo::m_readyCheckTime || !removed || !player || !CGRaidInfo::IsPlayerRaidOfficer(player)) {
            return;
        }

        uint32_t i = 0;

        while (i < count && !(CGRaidInfo::m_members[i]->m_readyCheckPending && (CGRaidInfo::m_members[i]->m_stats.m_flags & 0x1))) {
            i++;
        }

        done = i == count;
    } else {
        if (!CGRaidInfo::m_readyCheckTime || !removed || !player || player != CGPartyInfo::m_leader) {
            return;
        }

        uint32_t i = 0;

        for (; i < CGPartyInfo::GetNumPartyMembers(); i++) {
            CGPartyMemberState* state = CGPartyInfo::GetMemberState(i);

            if (state && (state->m_flags & 0x1) && CGRaidInfo::IsPartyMemberReadyCheckPending(i)) {
                break;
            }
        }

        done = i == CGPartyInfo::GetNumPartyMembers();
    }

    if (done) {
        CGRaidInfo::m_readyCheckTime = OsGetAsyncTimeMs() - 1;
        CGRaidInfo::CheckReadyCheckTime();
    }
}

// VERIFY: 7baa5663 manual
// OFFSET: 0x00574F10
void CGRaidInfo::UpdateRoster(const WOWGUID* guids, const uint32_t* subgroups, const uint8_t* flags, const uint32_t* roles, const uint8_t* online, uint32_t count, uint8_t battleground) {
    if (count > 40) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (count) {
        CGTutorial::TriggerTutorial(45);
    }

    uint32_t oldCount = CGRaidInfo::m_numMembers;
    int32_t joining = oldCount == 0;

    if (s_pendingNameQueries) {
        for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
            g_nameDBCache.CancelCallback(CGRaidInfo::m_members[i]->m_guid.m_guid, RaidRosterNameCallback, nullptr);
        }

        oldCount = CGRaidInfo::m_numMembers;
    }

    uint32_t addedCount = 0;
    WOWGUID* added = static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * count));
    uint32_t removedCount = 0;
    WOWGUID* removed = static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * oldCount));
    CGPartyMemberState* stats = static_cast<CGPartyMemberState*>(alloca(sizeof(CGPartyMemberState) * count));

    for (uint32_t i = 0; i < count; i++) {
        uint32_t j = 0;

        for (; j < oldCount; j++) {
            if (guids[i] == CGRaidInfo::m_members[j]->m_guid) {
                memcpy(&stats[i], &CGRaidInfo::m_members[j]->m_stats, sizeof(CGPartyMemberState));
                oldCount = CGRaidInfo::m_numMembers;
                break;
            }
        }

        if (j != oldCount) {
            continue;
        }

        memset(&stats[i], 0, sizeof(CGPartyMemberState));
        stats[i].m_health = 1;
        stats[i].m_maxHealth = 1;
        stats[i].m_power = 1;
        stats[i].m_maxPower = 1;

        if (oldCount) {
            NameCache* name = g_nameDBCache.GetRecord(guids[i].m_guid, guids[i], RaidMemberAddedNameCallback, nullptr, false);
            const char* fullNamePtr = nullptr;
            char fullName[256];

            if (name) {
                CGGameUI::DisplayError(ERR_RAID_MEMBER_ADDED_S, name->m_name);

                if (name->m_realmName[0]) {
                    SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
                } else {
                    SStrCopy(fullName, name->m_name, sizeof(fullName));
                }

                fullNamePtr = fullName;
            }

            CGAutoCompleteName::AddFlags(guids[i], 1, 0, fullNamePtr);
            oldCount = CGRaidInfo::m_numMembers;
        }

        added[addedCount++] = guids[i];
    }

    if (CGRaidInfo::m_rosterCounter == battleground) {
        for (uint32_t j = 0; j < oldCount; j++) {
            uint32_t i = 0;

            while (i < count && CGRaidInfo::m_members[j]->m_guid != guids[i]) {
                i++;
            }

            if (i != count) {
                continue;
            }

            NameCache* name = g_nameDBCache.GetRecord(CGRaidInfo::m_members[j]->m_guid.m_guid, CGRaidInfo::m_members[j]->m_guid, RaidMemberRemovedNameCallback, nullptr, false);

            if (name && ClntObjMgrGetActivePlayer()) {
                CGGameUI::DisplayError(ERR_RAID_MEMBER_REMOVED_S, name->m_name);
                CGAutoCompleteName::RemoveFlags(CGRaidInfo::m_members[j]->m_guid, 1);
            }

            oldCount = CGRaidInfo::m_numMembers;
            removed[removedCount++] = CGRaidInfo::m_members[j]->m_guid;
        }
    }

    CGRaidInfo::m_rosterCounter = battleground;

    if (!oldCount) {
        CGGameUI::DisplayError(ERR_RAID_YOU_JOINED);
        oldCount = CGRaidInfo::m_numMembers;
    }

    PartyReadyCheckInfo* previous = static_cast<PartyReadyCheckInfo*>(alloca(sizeof(PartyReadyCheckInfo) * oldCount));
    WOWGUID leader = WOWGUID();

    for (uint32_t i = 0; i < oldCount; i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];
        previous[i].m_guid = member->m_guid;
        previous[i].m_pending = member->m_readyCheckPending;
        previous[i].m_ready = member->m_readyCheckReady;

        if (member->m_rank == 2) {
            leader = member->m_guid;
        }
    }

    s_pendingNameQueries = 0;
    CGRaidInfo::m_numMembers = count;

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];
        member->m_guid = guids[i];
        member->m_subgroup = subgroups[i];
        member->m_assignments = flags[i];
        member->m_groupRole = roles[i];
        member->m_rank = member->m_guid == CGPartyInfo::m_leader ? 2 : flags[i] & 0x1;
        memcpy(&member->m_stats, &stats[i], sizeof(CGPartyMemberState));

        if (online[i]) {
            member->m_stats.m_flags |= 0x1;
        } else {
            member->m_stats.m_flags &= ~0x1;
        }

        uint32_t j = 0;

        while (j < oldCount && member->m_guid != previous[j].m_guid) {
            j++;
        }

        if (j < oldCount) {
            member->m_readyCheckPending = previous[j].m_pending;
            member->m_readyCheckReady = previous[j].m_ready;
        } else {
            member->m_readyCheckPending = 0;
            member->m_readyCheckReady = 0;
        }

        if (member->m_guid && !g_nameDBCache.GetRecord(member->m_guid.m_guid, member->m_guid, RaidRosterNameCallback, nullptr, false)) {
            s_pendingNameQueries++;
        }
    }

    for (uint32_t i = 0; i < removedCount; i++) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(removed[i], TYPE_PLAYER, __FILE__, __LINE__));

        if (!unit) {
            CGGameUI::ClearTarget(removed[i], 1);
            continue;
        }

        const WOWGUID& petGuid = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;

        if (petGuid) {
            CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (pet) {
                pet->UnregisterScript(512);
            } else {
                CGGameUI::ClearTarget(petGuid, 1);
            }
        }

        unit->UnregisterScript(256);
    }

    for (uint32_t i = 0; i < addedCount; i++) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(added[i], TYPE_PLAYER, __FILE__, __LINE__));

        if (unit) {
            unit->RegisterScript(256);
            const WOWGUID& petGuid = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;
            CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (pet) {
                pet->RegisterScript(512);
            }
        } else {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PARTY_MEMBER_STATS));
            msg.Put(added[i].m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    if (!s_pendingNameQueries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_RAID_ROSTER_UPDATE, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_ROLES_ASSIGNED, nullptr);

    if (joining) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_RAID_TARGET_UPDATE));
        msg.Put(static_cast<uint8_t>(255));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    CGRaidInfo::RosterUpdateRefreshReadyCheck(removedCount, leader);
}

// VERIFY: 268f30a3 manual
// OFFSET: 0x005756C0
void CGRaidInfo::InitializeGame() {
    ClientServices::SetMessageHandler(MSG_RAID_TARGET_UPDATE, Packet_MSG_RAID_TARGET_UPDATE, nullptr);
    ClientServices::SetMessageHandler(MSG_RAID_READY_CHECK, Packet_MSG_RAID_READY_CHECK, nullptr);
    ClientServices::SetMessageHandler(MSG_RAID_READY_CHECK_CONFIRM, Packet_MSG_RAID_READY_CHECK_CONFIRM, nullptr);
    ClientServices::SetMessageHandler(MSG_RAID_READY_CHECK_FINISHED, Packet_MSG_RAID_READY_CHECK_FINISHED, nullptr);
    ClientServices::SetMessageHandler(SMSG_READY_CHECK_ERROR, Packet_SMSG_RAID_READY_CHECK_ERROR, nullptr);
    ClientServices::SetMessageHandler(MSG_NOTIFY_PARTY_SQUELCH, Packet_MSG_NOTIFY_PARTY_SQUELCH, nullptr);
    ClientServices::SetMessageHandler(SMSG_ECHO_PARTY_SQUELCH, Packet_SMSG_ECHO_PARTY_SQUELCH, nullptr);
    CGRaidInfo::m_numMembers = 0;
    CGRaidInfo::m_realNumMembers = 0;
    CGRaidInfo::m_selectedMember = WOWGUID();
    s_pendingNameQueries = 0;
    CGRaidInfo::m_rosterCounter = 0xFF;
    memset(CGRaidInfo::m_raidTargets, 0, sizeof(CGRaidInfo::m_raidTargets));

    for (uint32_t i = 0; i < sizeof(CGRaidInfo::m_members) / sizeof(CGRaidInfo::m_members[0]); i++) {
        CGRaidInfo::m_members[i] = static_cast<CGRaidMember*>(SMemAlloc(sizeof(CGRaidMember), __FILE__, __LINE__, 0));
        memset(CGRaidInfo::m_members[i], 0, sizeof(CGRaidMember));
    }

    CGRaidInfo::m_readyCheckTime = 0;
    CGRaidInfo::m_readyCheckInitiator = WOWGUID();
    CGRaidInfo::m_readyCheckTimer = 0;
}

// VERIFY: f8514842 manual
// OFFSET: 0x00608130
int32_t CGRaidInfo::GetMemberIndexByGuid(WOWGUID guid) {
    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        if (CGRaidInfo::m_members[i]->m_guid == guid) {
            return i;
        }
    }

    return -1;
}

// VERIFY: f3031c34 auto
// OFFSET: 0x004FB0D0
CGRaidMember* CGRaidInfo::GetMember(uint32_t index) {
    if (index >= CGRaidInfo::m_numMembers) {
        return nullptr;
    }

    return CGRaidInfo::m_members[index];
}

// VERIFY: 2a11deb6 manual
// OFFSET: 0x00549670
CGRaidMember* CGRaidInfo::FindMemberByGuid(WOWGUID guid) {
    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        if (CGRaidInfo::m_members[i]->m_guid == guid) {
            return CGRaidInfo::m_members[i];
        }
    }

    return nullptr;
}

// VERIFY: 2ec03ec8 manual
// OFFSET: 0x00573200
int32_t CGRaidInfo::IsPlayerOrPetInRaid(const WOWGUID& guid) {
    if (!static_cast<uint32_t>(guid.m_guid)) {
        return static_cast<uint32_t>(guid.m_guid >> 32);
    }

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];

        if (!member) {
            continue;
        }

        if (member->m_guid == guid) {
            return 1;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(member->m_guid, TYPE_PLAYER, __FILE__, __LINE__));

        if (player) {
            CGUnitData* unit = player->m_unit;
            const WOWGUID& pet = unit->m_unitFieldCharm ? unit->m_unitFieldCharm : unit->m_unitFieldSummon;

            if (pet == guid) {
                return 1;
            }
        } else if ((member->m_stats.m_flags & 1) && member->m_stats.m_pet.m_guid == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 2ceb7b94 manual
// OFFSET: 0x005723E0
int32_t CGRaidInfo::GetPartyMemberIndex(WOWGUID guid) {
    for (int32_t i = 0; i < 4; i++) {
        if (CGPartyInfo::m_members[i] == guid) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 7ec85a21 manual
// OFFSET: 0x00572450
int32_t CGRaidInfo::IsPartyMemberReadyCheckPending(uint32_t index) {
    return index < 4 ? CGRaidInfo::m_partyReadyCheckPending[index] : 0;
}

// VERIFY: 1c65956c manual
// OFFSET: 0x00572470
int32_t CGRaidInfo::IsPartyMemberReadyCheckReady(uint32_t index) {
    return index < 4 ? CGRaidInfo::m_partyReadyCheckReady[index] : 0;
}

// VERIFY: 052e75dd manual
// OFFSET: 0x00572800
CGPartyMemberState* CGRaidInfo::GetRemoteStatsFromPet(const WOWGUID& petGuid) {
    for (uint32_t i = 0; i < sizeof(CGRaidInfo::m_members) / sizeof(CGRaidInfo::m_members[0]); i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];

        if (member->m_guid && member->m_stats.m_pet.m_guid == petGuid) {
            return &CGRaidInfo::m_members[i]->m_stats;
        }
    }

    return nullptr;
}

// VERIFY: c00f8c5d manual
// OFFSET: 0x00572900
uint8_t CGRaidInfo::GetPartyAssignments(const WOWGUID& guid) {
    uint32_t i = 0;

    if (!CGRaidInfo::m_numMembers) {
        return 0;
    }

    while (CGRaidInfo::m_members[i]->m_guid != guid) {
        if (++i >= CGRaidInfo::m_numMembers) {
            return 0;
        }
    }

    return CGRaidInfo::m_members[i]->m_assignments;
}

// VERIFY: 747447c0 manual
// OFFSET: 0x00572950
int32_t CGRaidInfo::GetPartyAssignments(lua_State* L, int32_t assignment) {
    uint8_t flag = 0;

    if (assignment == 0) {
        flag = 2;
    } else if (assignment == 1) {
        flag = 4;
    }

    if (lua_isstring(L, 2)) {
        const char* name = lua_tolstring(L, 2, nullptr);
        WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, 6, StringToBOOL(L, 3, 0), 3.4028235e38f, 0);

        if (!guid) {
            CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
            return 0;
        }

        if (CGRaidInfo::GetPartyAssignments(guid) & flag) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        return 1;
    }

    int32_t count = 0;

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        if (flag & CGRaidInfo::m_members[i]->m_assignments) {
            lua_checkstack(L, ++count);
            lua_pushnumber(L, i + 1);
        }
    }

    return count;
}

// VERIFY: 9af530d4 auto
// OFFSET: 0x00572A90
void CGRaidInfo::ClearRecruitAFriend(WOWGUID guid) {
    CGPartyMemberState* stats = CGRaidInfo::GetRemoteStats(guid);

    if (stats) {
        stats->m_flags &= ~0x100;
    }
}

// VERIFY: 58d37c92 manual
// OFFSET: 0x00572F90
void CGRaidInfo::ClearRoster() {
    if (CGRaidInfo::m_numMembers) {
        CGGameUI::DisplayError(ERR_RAID_YOU_LEFT);
        CGRaidInfo::m_numMembers = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_RAID_ROSTER_UPDATE, nullptr);

        for (uint32_t i = 0; i < 40; i++) {
            CGRaidInfo::m_members[i]->m_guid = WOWGUID();
        }
    }

    WOWGUID targets[8];
    memcpy(targets, CGRaidInfo::m_raidTargets, sizeof(targets));
    memset(CGRaidInfo::m_raidTargets, 0, sizeof(CGRaidInfo::m_raidTargets));

    for (uint32_t i = 0; i < 8; i++) {
        if (!targets[i]) {
            continue;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(targets[i], TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->NamePlateUpdateRaidTarget();
        }
    }

    CGAutoCompleteName::RemoveFlagsFromAllNodes(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_RAID_TARGET_UPDATE, nullptr);
    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_ROLES_ASSIGNED, nullptr);
}

// VERIFY: 9403a697 manual
// OFFSET: 0x00573070
void CGRaidInfo::EnableMember(CGPlayer_C* player, int32_t enable) {
    WOWGUID guid = player->m_obj->m_guid;
    uint32_t index;

    for (index = 0; index < CGRaidInfo::m_numMembers; index++) {
        if (CGRaidInfo::m_members[index]->m_guid == guid) {
            break;
        }
    }

    if (index == CGRaidInfo::m_numMembers || enable) {
        return;
    }

    player->FillPartyMemberState(&CGRaidInfo::m_members[index]->m_stats);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PARTY_MEMBER_STATS));
    msg.Put(player->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: a1f440f2 manual
// OFFSET: 0x005732B0
void CGRaidInfo::SendReadyCheck() {
    WOWGUID player = ClntObjMgrGetActivePlayer();

    if (!player) {
        return;
    }

    if (CGRaidInfo::m_numMembers) {
        if (!CGRaidInfo::IsPlayerRaidOfficer(player)) {
            return;
        }
    } else if (player != CGPartyInfo::m_leader) {
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (CGRaidInfo::m_readyCheckTime && static_cast<int32_t>(now - CGRaidInfo::m_readyCheckTime) < 0 && CGRaidInfo::m_readyCheckTime != now) {
        CGGameUI::DisplayError(ERR_READY_CHECK_IN_PROGRESS);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_RAID_READY_CHECK));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d4327f37 manual
// OFFSET: 0x00573390
void CGRaidInfo::CheckReadyCheckTime() {
    if (!CGRaidInfo::m_readyCheckTime || !CGRaidInfo::m_readyCheckInitiator || CGRaidInfo::m_readyCheckInitiator != ClntObjMgrGetActivePlayer() || static_cast<int32_t>(OsGetAsyncTimeMs() - CGRaidInfo::m_readyCheckTime) < 0) {
        return;
    }

    if (CGRaidInfo::m_readyCheckTimer) {
        ClientKillTimer(CGRaidInfo::m_readyCheckTimer, CGRaidInfo::ProcessReadyCheckTimer, "ProcessReadyCheckTimer");
        CGRaidInfo::m_readyCheckTimer = 0;
    }

    CGRaidInfo::m_readyCheckTime = 0;
    const char* format = FrameScript_GetText("RAID_MEMBERS_AFK", -1, GENDER_NOT_APPLICABLE);
    int32_t first = 1;
    int32_t allReady = 1;
    char names[512];

    if (CGRaidInfo::m_numMembers) {
        for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
            CGRaidMember* member = CGRaidInfo::m_members[i];

            if (!member->m_readyCheckReady) {
                allReady = 0;
            }

            if (!member->m_readyCheckPending) {
                continue;
            }

            WOWGUID none = WOWGUID();
            NameCache* name = g_nameDBCache.GetRecord(member->m_guid.m_guid, none, nullptr, nullptr, false);

            if (!name) {
                continue;
            }

            if (first) {
                SStrCopy(names, name->m_name, sizeof(names));
                first = 0;
            } else {
                SStrPack(names, ", ", sizeof(names));
                SStrPack(names, name->m_name, sizeof(names));
            }
        }
    } else {
        for (uint32_t i = 0; i < CGPartyInfo::GetNumPartyMembers(); i++) {
            if (i >= 4 || !CGRaidInfo::m_partyReadyCheckReady[i]) {
                allReady = 0;
            }

            if (i >= 4 || !CGRaidInfo::m_partyReadyCheckPending[i]) {
                continue;
            }

            WOWGUID none = WOWGUID();
            NameCache* name = g_nameDBCache.GetRecord(GetPartyMemberGUID(i).m_guid, none, nullptr, nullptr, false);

            if (!name) {
                continue;
            }

            if (first) {
                SStrCopy(names, name->m_name, sizeof(names));
                first = 0;
            } else {
                SStrPack(names, ", ", sizeof(names));
                SStrPack(names, name->m_name, sizeof(names));
            }
        }
    }

    if (!first) {
        char text[512];
        SStrPrintf(text, sizeof(text), format, names);
        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else if (!allReady) {
        CGChat::AddChatMessage(FrameScript_GetText("READY_CHECK_FINISHED", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else {
        CGChat::AddChatMessage(FrameScript_GetText("READY_CHECK_ALL_READY", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_RAID_READY_CHECK_FINISHED));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 9571779b manual
// OFFSET: 0x00574310
void CGRaidInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(MSG_RAID_TARGET_UPDATE);
    ClientServices::ClearMessageHandler(MSG_RAID_READY_CHECK);
    ClientServices::ClearMessageHandler(MSG_RAID_READY_CHECK_CONFIRM);
    ClientServices::ClearMessageHandler(MSG_RAID_READY_CHECK_FINISHED);
    ClientServices::ClearMessageHandler(SMSG_READY_CHECK_ERROR);
    ClientServices::ClearMessageHandler(MSG_NOTIFY_PARTY_SQUELCH);
    ClientServices::ClearMessageHandler(SMSG_ECHO_PARTY_SQUELCH);

    for (uint32_t i = 0; i < 40; i++) {
        delete CGRaidInfo::m_members[i];
        CGRaidInfo::m_members[i] = nullptr;
    }

    if (CGRaidInfo::m_readyCheckTimer) {
        ClientKillTimer(CGRaidInfo::m_readyCheckTimer, CGRaidInfo::ProcessReadyCheckTimer, "ProcessReadyCheckTimer");
    }
}

// VERIFY: 7980cdfb auto
// OFFSET: 0x00573670
int32_t CGRaidInfo::ProcessReadyCheckTimer(const void* data, void* param) {
    CGRaidInfo::m_readyCheckTimer = 0;
    CGRaidInfo::CheckReadyCheckTime();
    return 1;
}

// VERIFY: c54805fa auto
// OFFSET: 0x005EEB70
void CGRaidInfo::LeaveWorld() {
}
