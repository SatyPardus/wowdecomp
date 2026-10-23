#include "DuelInfo.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstdint>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Event/Event.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"

// STATIC: 0x00C22B58
static WOWGUID s_duelArbiter;

// STATIC: 0x00C22B60
WOWGUID CGDuelInfo::m_dueler;

// STATIC: 0x00C22B68
static uint32_t s_countdown;

// STATIC: 0x00C22B6C
static uint32_t s_countdownTimer;

// VERIFY: 516f4f57 auto
// OFFSET: 0x005CFA50
int32_t Packet_SMSG_DUEL_COUNTDOWN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t countdown;
    msg->Get(countdown);
    s_countdown = countdown / 1000;
    CGDuelInfo::ProcessCountdown(nullptr, nullptr);
    return 1;
}

// VERIFY: ed3c6531 auto
// OFFSET: 0x005CFA90
int32_t Packet_SMSG_DUEL_COMPLETE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t completed;
    msg->Get(completed);

    if (s_duelArbiter) {
        if (!completed) {
            CGGameUI::DisplayError(ERR_DUEL_CANCELLED);
        }

        s_duelArbiter = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_DUEL_FINISHED, nullptr);
    }

    if (s_countdownTimer) {
        ClientKillTimer(s_countdownTimer, CGDuelInfo::ProcessCountdown, "ProcessCountdown");
        s_countdownTimer = 0;
    }

    CGDuelInfo::m_dueler = WOWGUID();
    return 1;
}

// VERIFY: a77809b9 auto
// OFFSET: 0x005CFB20
int32_t Packet_SMSG_DUEL_WINNER(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t retreat;
    char winner[48];
    char loser[48];
    msg->Get(retreat);
    msg->GetString(winner, sizeof(winner));
    msg->GetString(loser, sizeof(loser));
    const char* format;

    if (retreat) {
        format = FrameScript_GetText("DUEL_WINNER_RETREAT", -1, GENDER_NOT_APPLICABLE);
    } else {
        format = FrameScript_GetText("DUEL_WINNER_KNOCKOUT", -1, GENDER_NOT_APPLICABLE);
    }

    char buffer[3000];
    SStrPrintf(buffer, sizeof(buffer), format, winner, loser);
    CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: 484d7c26 manual
// OFFSET: 0x005CFBD0
void CGDuelInfo::AcceptDuel() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_DUEL_ACCEPTED));
    msg.Put(s_duelArbiter.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 2481bb12 manual
// OFFSET: 0x005CFC50
void CGDuelInfo::CancelDuel() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_DUEL_CANCELLED));
    msg.Put(s_duelArbiter.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 019988e7 manual
// OFFSET: 0x005CFCD0
int32_t Packet_SMSG_DUEL_REQUESTED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    msg->Get(s_duelArbiter.m_guid);
    WOWGUID requester;
    msg->Get(requester.m_guid);

    if (requester == ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_DUEL_REQUESTED);
        CGDuelInfo::AcceptDuel();
        return 1;
    }

    if (FriendList::s_friendList->IsIgnored(requester, true)) {
        CGDuelInfo::CancelDuel();
        s_duelArbiter = WOWGUID();
        return 1;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(requester, TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        FrameScript_SignalEvent(SCRIPT_EVENT_DUEL_REQUESTED, "%s", player->GetUnitName(nullptr, 1));
    }

    CGDuelInfo::m_dueler = requester;
    return 1;
}

// VERIFY: 7eda6e31 auto
// OFFSET: 0x005CFDD0
void CGDuelInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_DUEL_REQUESTED, Packet_SMSG_DUEL_REQUESTED, nullptr);
    ClientServices::SetMessageHandler(SMSG_DUEL_OUT_OF_BOUNDS, Packet_SMSG_DUEL_OUTOFBOUNDS, nullptr);
    ClientServices::SetMessageHandler(SMSG_DUEL_IN_BOUNDS, Packet_SMSG_DUEL_INBOUNDS, nullptr);
    ClientServices::SetMessageHandler(SMSG_DUEL_COUNTDOWN, Packet_SMSG_DUEL_COUNTDOWN, nullptr);
    ClientServices::SetMessageHandler(SMSG_DUEL_COMPLETE, Packet_SMSG_DUEL_COMPLETE, nullptr);
    ClientServices::SetMessageHandler(SMSG_DUEL_WINNER, Packet_SMSG_DUEL_WINNER, nullptr);
}

// VERIFY: 254771ff auto
// OFFSET: 0x005CF870
int32_t CGDuelInfo::ProcessCountdown(const void* data, void* param) {
    char buffer[3000];
    SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("DUEL_COUNTDOWN", -1, GENDER_NOT_APPLICABLE), s_countdown);
    CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);

    if (--s_countdown) {
        s_countdownTimer = ClientSetTimer(1000, CGDuelInfo::ProcessCountdown, nullptr);
    } else {
        s_countdownTimer = 0;
    }

    return 1;
}

// VERIFY: 88d0e305 auto
// OFFSET: 0x005CF910
int32_t Packet_SMSG_DUEL_OUTOFBOUNDS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    FrameScript_SignalEvent(SCRIPT_EVENT_DUEL_OUTOFBOUNDS, nullptr);
    return 1;
}

// VERIFY: 2993a7cd auto
// OFFSET: 0x005CF930
int32_t Packet_SMSG_DUEL_INBOUNDS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    FrameScript_SignalEvent(SCRIPT_EVENT_DUEL_INBOUNDS, nullptr);
    return 1;
}

// VERIFY: 0f292ee9 auto
// OFFSET: 0x005CF830
void CGDuelInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_DUEL_REQUESTED);
    ClientServices::ClearMessageHandler(SMSG_DUEL_OUT_OF_BOUNDS);
    ClientServices::ClearMessageHandler(SMSG_DUEL_IN_BOUNDS);
    ClientServices::ClearMessageHandler(SMSG_DUEL_COUNTDOWN);
    ClientServices::ClearMessageHandler(SMSG_DUEL_COMPLETE);
    ClientServices::ClearMessageHandler(SMSG_DUEL_WINNER);
}

// VERIFY: d4b09e29 auto
// OFFSET: 0x005CFA10
void CGDuelInfo::LeaveWorld() {
    s_duelArbiter = WOWGUID();

    if (s_countdownTimer) {
        ClientKillTimer(s_countdownTimer, CGDuelInfo::ProcessCountdown, "ProcessCountdown");
        s_countdownTimer = 0;
    }
}
