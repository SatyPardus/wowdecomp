#include "Trade_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include <cstring>

// STATIC: 0x00CA0B7C
uint32_t s_clientStateIndex;

// STATIC: 0x00CA0B80
static TRADE_ITEM s_tradeItems[2][7];

// STATIC: 0x00CA0FE0
static int32_t s_tradeRequested;

// STATIC: 0x00CA0FE4
static int32_t s_tradeUseCursorItem;

// STATIC: 0x00CA0FE8
static WOWGUID s_tradeTarget;

// STATIC: 0x00CA0FF0
static int32_t s_tradeEnchantSpell[2];

// STATIC: 0x00CA0FF8
static uint32_t s_nextTradeRequestTime;

// VERIFY: bcd077ea auto
// OFFSET: 0x00703C10
WOWGUID Trade_C_GetTradeTarget() {
    return s_tradeTarget;
}

// VERIFY: 522e8804 auto
// OFFSET: 0x00703C20
int32_t Trade_C_IsTradeRequested() {
    return s_tradeRequested;
}

// VERIFY: 714e84f5 auto
// OFFSET: 0x00703C30
int32_t Trade_C_UseCursorItem() {
    return s_tradeRequested && s_tradeUseCursorItem;
}

// VERIFY: 71c6ebe1 auto
// OFFSET: 0x00703C50
int32_t Trade_C_GetEnchantSpell(int32_t player) {
    return s_tradeEnchantSpell[player];
}

// VERIFY: c648ea3e auto
// OFFSET: 0x00703C60
static void TradeNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_INITIATE_TRADE_S, name->m_name);
    }
}

// VERIFY: ef2d8e5d auto
// OFFSET: 0x00703CD0
void Trade_C_Destroy() {
    ClientServices::ClearMessageHandler(SMSG_TRADE_STATUS);
    ClientServices::ClearMessageHandler(SMSG_TRADE_STATUS_EXTENDED);
}

// VERIFY: efe27afc manual
// OFFSET: 0x00703CF0
void CGTradeInfo::InitiateTrade(WOWGUID target, int32_t useCursorItem) {
    if (s_tradeTarget) {
        CGGameUI::DisplayError(ERR_ALREADY_TRADING);
        return;
    }

    if (s_nextTradeRequestTime && static_cast<int32_t>(FrameTime::s_curTimeMs - s_nextTradeRequestTime) < 0) {
        return;
    }

    s_nextTradeRequestTime = FrameTime::s_curTimeMs + 1000;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_INITIATE_TRADE));
    msg.Put(target.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    s_tradeUseCursorItem = useCursorItem;
    s_tradeTarget = target;
    s_tradeRequested = 1;
    NameCache* name = g_nameDBCache.GetRecord(target.m_guid, target, &TradeNameCallback, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_INITIATE_TRADE_S, name->m_name);
    }
}

// VERIFY: cc5d4c9b manual
// OFFSET: 0x00703E00
void Trade_C_BeginTrade() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BEGIN_TRADE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: ab43a905 manual
// OFFSET: 0x00703E70
void Trade_C_PlayerBusy() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BUSY_TRADE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: b9daa0d2 manual
// OFFSET: 0x00703EE0
void Trade_C_PlayerIgnored() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_IGNORE_TRADE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 6f4e5623 manual
// OFFSET: 0x00703F50
void Trade_C_AcceptTrade() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ACCEPT_TRADE));
    msg.Put(CGTradeInfo::m_playerTrade.m_stateIndex[1]);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: f77c5a3f manual
// OFFSET: 0x00703FD0
void Trade_C_UnacceptTrade() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_UNACCEPT_TRADE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d10db303 manual
// OFFSET: 0x00704040
void Trade_C_CancelTrade() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CANCEL_TRADE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 805652c0 manual
// OFFSET: 0x007040B0
bool Trade_C_AddItem(WOWGUID item, WOWGUID bag, uint8_t slot, uint8_t tradeSlot) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__)) {
        return false;
    }

    uint8_t bagIndex = player->FindSlotIndex(bag);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_TRADE_ITEM));
    msg.Put(tradeSlot);
    msg.Put(bagIndex);
    msg.Put(slot);
    s_clientStateIndex++;
    msg.Finalize();
    ClientServices::Send2(&msg);

    return true;
}

// VERIFY: a89b3295 manual
// OFFSET: 0x007041A0
void Trade_C_RemoveItem(uint8_t tradeSlot) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CLEAR_TRADE_ITEM));
    msg.Put(tradeSlot);
    s_clientStateIndex++;
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d84bfa10 manual
// OFFSET: 0x00704220
bool CGTradeInfo::AddMoney(int32_t amount) {
    if (!amount) {
        return true;
    }

    uint32_t total = CGTradeInfo::m_playerTrade.m_money[0] + amount;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->GetCoinage() < total) {
        return false;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_TRADE_GOLD));
    msg.Put(total);
    s_clientStateIndex++;
    msg.Finalize();
    CGTradeInfo::m_playerTrade.m_money[0] = total;
    ClientServices::Send2(&msg);
    return true;
}

// VERIFY: 1c5417e1 manual
// OFFSET: 0x007043C0
bool CGTradeInfo::SetMoney(uint32_t amount) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->GetCoinage() < amount) {
        return false;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_TRADE_GOLD));
    msg.Put(amount);
    s_clientStateIndex++;
    msg.Finalize();
    CGTradeInfo::m_playerTrade.m_money[0] = amount;
    ClientServices::Send2(&msg);
    return true;
}

// VERIFY: 895d61e1 auto
// OFFSET: 0x007048D0
void Trade_C_Initialize() {
    s_tradeRequested = 0;
    s_tradeUseCursorItem = 0;
    s_tradeTarget = WOWGUID();
    ClientServices::SetMessageHandler(SMSG_TRADE_STATUS, Packet_SMSG_TRADE_STATUS, nullptr);
    ClientServices::SetMessageHandler(SMSG_TRADE_STATUS_EXTENDED, Packet_SMSG_TRADE_STATUS_EXTENDED, nullptr);
}

// VERIFY: 54433403 manual
// OFFSET: 0x007044A0
int32_t Packet_SMSG_TRADE_STATUS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t status;
    msg->Get(status);
    int32_t reset = 0;
    uint32_t bagResult = 0;
    uint8_t target = 0;
    uint32_t itemId = 0;
    uint8_t slot = 0;

    switch (status) {
        case TRADE_STATUS_PLAYER_BUSY:
        case TRADE_STATUS_ALREADY_TRADING:
        case TRADE_STATUS_PLAYER_NOT_FOUND:
        case TRADE_STATUS_TOO_FAR_AWAY:
        case TRADE_STATUS_WRONG_FACTION:
        case TRADE_STATUS_PLAYER_IGNORED:
        case TRADE_STATUS_PLAYER_STUNNED:
        case TRADE_STATUS_TARGET_STUNNED:
        case TRADE_STATUS_DEAD:
        case TRADE_STATUS_TARGET_DEAD:
        case TRADE_STATUS_LOGGING_OUT:
        case TRADE_STATUS_TARGET_LOGGING_OUT:
        case TRADE_STATUS_RESTRICTED_ACCOUNT:
            reset = 1;
            break;

        case TRADE_STATUS_PROPOSED: {
            uint64_t guid;
            msg->Get(guid);

            if (s_tradeRequested) {
                Trade_C_PlayerBusy();
            } else {
                s_tradeTarget.m_guid = guid;
            }

            break;
        }

        case TRADE_STATUS_INITIATED:
            s_clientStateIndex = 1;
            CGTradeInfo::m_playerTrade.m_stateIndex[0] = 1;
            CGTradeInfo::m_playerTrade.m_stateIndex[1] = 1;
            memset(s_tradeItems, 0, sizeof(s_tradeItems));
            CGTradeInfo::m_playerTrade.m_money[0] = 0;
            CGTradeInfo::m_playerTrade.m_money[1] = 0;
            s_tradeEnchantSpell[0] = 0;
            s_tradeEnchantSpell[1] = 0;
            msg->Get(CGTradeInfo::m_playerTrade.m_tradeId);
            break;

        case TRADE_STATUS_CANCELLED:
        case TRADE_STATUS_UNKNOWN_13:
            reset = 1;
            memset(s_tradeItems, 0, sizeof(s_tradeItems));
            CGTradeInfo::m_playerTrade.m_money[0] = 0;
            CGTradeInfo::m_playerTrade.m_money[1] = 0;
            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_MONEY, nullptr);
            break;

        case TRADE_STATUS_FAILED:
            msg->Get(bagResult);
            msg->Get(target);
            msg->Get(itemId);

        case TRADE_STATUS_COMPLETE:
            reset = 1;
            memset(s_tradeItems, 0, sizeof(s_tradeItems));
            CGTradeInfo::m_playerTrade.m_money[0] = 0;
            CGTradeInfo::m_playerTrade.m_money[1] = 0;
            break;

        case TRADE_STATUS_WRONG_REALM:
        case TRADE_STATUS_NOT_ON_TAPLIST:
            msg->Get(slot);

            if (slot == 0xFF) {
                CGTradeInfo::m_playerTrade.m_money[0] = 0;
            }

            break;
    }

    CGTradeInfo::HandleTradeMessage(static_cast<TRADE_STATUS>(status), bagResult, target != 0, itemId, slot);

    if (reset) {
        s_tradeRequested = 0;
        s_tradeTarget = WOWGUID();
    }

    return 1;
}

// VERIFY: 6a4a3506 manual
// OFFSET: 0x00704680
int32_t Packet_SMSG_TRADE_STATUS_EXTENDED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t side;
    uint32_t tradeId;
    uint32_t stateIndex;
    msg->Get(side);
    msg->Get(tradeId);
    msg->Get(stateIndex);

    if (tradeId != CGTradeInfo::m_playerTrade.m_tradeId) {
        Trade_C_CancelTrade();
        return 1;
    }

    if (!side && stateIndex < s_clientStateIndex) {
        msg->m_read = msg->m_size;
        return 1;
    }

    msg->Get(CGTradeInfo::m_playerTrade.m_stateIndex[side]);
    msg->Get(CGTradeInfo::m_playerTrade.m_money[side]);
    bool received[7] = {};
    msg->Get(reinterpret_cast<uint32_t&>(s_tradeEnchantSpell[side]));

    while (msg->m_read < msg->m_size) {
        uint8_t slot;
        TRADE_ITEM item;
        msg->Get(slot);
        msg->Get(reinterpret_cast<uint32_t&>(item.m_itemID));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_displayID));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_count));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_wrapped));
        msg->Get(item.m_giftCreator.m_guid);
        msg->Get(reinterpret_cast<uint32_t&>(item.m_enchant));

        for (uint32_t i = 0; i < 3; i++) {
            msg->Get(reinterpret_cast<uint32_t&>(item.m_socketEnchants[i]));
        }

        msg->Get(item.m_creator.m_guid);
        msg->Get(reinterpret_cast<uint32_t&>(item.m_spellCharges));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_suffixFactor));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_randomPropertyID));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_lockID));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_maxDurability));
        msg->Get(reinterpret_cast<uint32_t&>(item.m_durability));

        if (!side && s_tradeItems[0][slot].m_itemID && !item.m_itemID) {
            CGTradeInfo::DenyPlayerItem(slot);
        }

        s_tradeItems[side][slot] = item;
        received[slot] = true;
    }

    for (uint32_t i = 0; i < 7; i++) {
        if (!received[i]) {
            if (!side && s_tradeItems[0][i].m_itemID) {
                CGTradeInfo::DenyPlayerItem(i);
            }

            memset(&s_tradeItems[side][i], 0, sizeof(TRADE_ITEM));
        }
    }

    CGTradeInfo::OnTradeUpdated(s_tradeItems[1]);
    return 1;
}
