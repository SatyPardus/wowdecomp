#include "TradeFrame.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Object/ObjectClient/Trade_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ItemLimitCategoryRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/AuctionHouse.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"

// STATIC: 0x00BFA680
WOWGUID CGTradeInfo::m_enchantingItem;

// STATIC: 0x00BFA658
WOWGUID CGTradeInfo::m_tradingPlayer;

// STATIC: 0x00BFA400
int32_t CGTradeInfo::m_targetDurability[7];

// STATIC: 0x00BFA41C
int32_t CGTradeInfo::m_targetMaxDurability[7];

// STATIC: 0x00BFA438
int32_t CGTradeInfo::m_targetLockID[7];

// STATIC: 0x00BFA454
int32_t CGTradeInfo::m_targetRandomPropertyID[7];

// STATIC: 0x00BFA470
int32_t CGTradeInfo::m_targetSuffixFactor[7];

// STATIC: 0x00BFA48C
int32_t CGTradeInfo::m_targetSpellCharges[7];

// STATIC: 0x00BFA4A8
WOWGUID CGTradeInfo::m_targetGiftCreator[7];

// STATIC: 0x00BFA4E0
int32_t CGTradeInfo::m_targetWrapped[7];

// STATIC: 0x00BFA4FC
int32_t CGTradeInfo::m_targetEnchant[7];

// STATIC: 0x00BFA518
WOWGUID CGTradeInfo::m_targetCreator[7];

// STATIC: 0x00BFA550
int32_t CGTradeInfo::m_targetSocketEnchants[7][3];

// STATIC: 0x00BFA5A4
int32_t CGTradeInfo::m_targetCount[7];

// STATIC: 0x00BFA5C0
int32_t CGTradeInfo::m_targetItemID[7];

// STATIC: 0x00BFA5DC
uint8_t CGTradeInfo::m_playerItemContainerSlot[7];

// STATIC: 0x00BFA5E8
WOWGUID CGTradeInfo::m_playerItemContainer[7];

// STATIC: 0x00BFA620
WOWGUID CGTradeInfo::m_playerItems[7];

// STATIC: 0x00BFA660
int32_t CGTradeInfo::m_playerAccepted;

// STATIC: 0x00BFA664
int32_t CGTradeInfo::m_targetAccepted;

// STATIC: 0x00BFA668
int32_t CGTradeInfo::m_lastPlayerEnchantSpell;

// STATIC: 0x00BFA66C
int32_t CGTradeInfo::m_lastTargetEnchantSpell;

// STATIC: 0x00BFA670
int32_t CGTradeInfo::m_lastPlayerMoney;

// STATIC: 0x00BFA674
int32_t CGTradeInfo::m_lastTargetMoney;

// STATIC: 0x00BFA678
int32_t CGTradeInfo::m_supressMoneyEvent;

// STATIC: 0x00CA0B68
PlayerTradeState CGTradeInfo::m_playerTrade;

// VERIFY: 2df8b30b manual
// OFFSET: 0x0050F470
int32_t CGTradeInfo::GetTargetEnchant(uint32_t slot) {
    if (slot > 6) {
        return 0;
    }

    return CGTradeInfo::m_targetEnchant[slot];
}

// VERIFY: 185ffecc auto
// OFFSET: 0x00586010
void CGTradeInfo::GetPlayerItem(uint32_t slot, WOWGUID& item, WOWGUID& container, uint8_t& containerSlot) {
    item = slot > 6 ? WOWGUID() : CGTradeInfo::m_playerItems[slot];

    if (!item) {
        container = WOWGUID();
        containerSlot = 0;
    } else {
        container = CGTradeInfo::m_playerItemContainer[slot];
        containerSlot = CGTradeInfo::m_playerItemContainerSlot[slot];
    }
}

// VERIFY: efb17c1b auto
// OFFSET: 0x00586080
void TradeItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_UPDATE, nullptr);
    }
}

// VERIFY: 81ca5d00 auto
// OFFSET: 0x005860A0
void CGTradeInfo::EnterWorld() {
    CGTradeInfo::m_tradingPlayer = WOWGUID();
    CGTradeInfo::m_playerAccepted = 0;
    CGTradeInfo::m_targetAccepted = 0;
    CGTradeInfo::m_lastPlayerEnchantSpell = 0;
    CGTradeInfo::m_lastTargetEnchantSpell = 0;
    CGTradeInfo::m_lastPlayerMoney = 0;
    CGTradeInfo::m_lastTargetMoney = 0;

    for (uint32_t i = 0; i < 7; i++) {
        CGTradeInfo::m_targetDurability[i] = 0;
        CGTradeInfo::m_targetMaxDurability[i] = 0;
        CGTradeInfo::m_targetRandomPropertyID[i] = 0;
        CGTradeInfo::m_targetSuffixFactor[i] = 0;
        CGTradeInfo::m_targetSpellCharges[i] = 0;
        CGTradeInfo::m_targetLockID[i] = 0;
        CGTradeInfo::m_targetWrapped[i] = 0;
        CGTradeInfo::m_targetGiftCreator[i] = WOWGUID();
        CGTradeInfo::m_targetCreator[i] = WOWGUID();
    }

    memset(CGTradeInfo::m_targetSocketEnchants, 0, sizeof(CGTradeInfo::m_targetSocketEnchants));

    for (uint32_t i = 0; i < 7; i++) {
        CGTradeInfo::m_targetEnchant[i] = 0;
        CGTradeInfo::m_targetCount[i] = 0;
        CGTradeInfo::m_targetItemID[i] = 0;
        CGTradeInfo::m_playerItemContainerSlot[i] = 0;
        CGTradeInfo::m_playerItemContainer[i] = WOWGUID();
        CGTradeInfo::m_playerItems[i] = WOWGUID();
    }
}

// VERIFY: 67fa8d09 auto
// OFFSET: 0x005863C0
void CGTradeInfo::UpdatePlayerItem(WOWGUID guid) {
    if (!guid) {
        return;
    }

    for (int32_t i = 0; i < 7; i++) {
        if (CGTradeInfo::m_playerItems[i] == guid) {
            FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_PLAYER_ITEM_CHANGED, "%d", i + 1);
            return;
        }
    }
}

// VERIFY: f90faa11 manual
// OFFSET: 0x005869A0
int32_t CGTradeInfo::SetPlayerItem(uint32_t slot, WOWGUID item, WOWGUID container, uint8_t containerSlot) {
    if (slot > 6) {
        return 0;
    }

    if (item) {
        if (!Trade_C_AddItem(item, container, containerSlot, static_cast<uint8_t>(slot))) {
            return 0;
        }

        if (slot == 6) {
            CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));
            bool bound = object && object->IsBound() && !object->IsBoundForTrade();
            FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_POTENTIAL_BIND_ENCHANT, "%b", bound);
        }
    } else {
        Trade_C_RemoveItem(static_cast<uint8_t>(slot));

        if (slot == 6) {
            FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_POTENTIAL_BIND_ENCHANT, "%b", 0);
        }
    }

    CGTradeInfo::m_playerItems[slot] = item;
    CGTradeInfo::m_playerItemContainer[slot] = container;
    CGTradeInfo::m_playerItemContainerSlot[slot] = containerSlot;
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_PLAYER_ITEM_CHANGED, "%d", slot + 1);
    return 1;
}

// VERIFY: 9d5cff16 auto
// OFFSET: 0x00703CB0
uint32_t CGTradeInfo::GetPlayerTradeMoney() {
    return CGTradeInfo::m_playerTrade.m_money[0];
}

// VERIFY: 37c94e9a auto
// OFFSET: 0x00703CC0
uint32_t CGTradeInfo::GetTargetTradeMoney() {
    return CGTradeInfo::m_playerTrade.m_money[1];
}

// VERIFY: b09af206 manual
// OFFSET: 0x005864D0
bool CGTradeInfo::IsItemBeingTraded(WOWGUID item) {
    if (!CGTradeInfo::m_tradingPlayer) {
        return false;
    }

    for (uint32_t i = 0; i < 7; i++) {
        if (CGTradeInfo::m_playerItems[i] == item) {
            return true;
        }
    }

    return false;
}

// VERIFY: 82403266 manual
// OFFSET: 0x00704320
void CGTradeInfo::RemoveMoney(uint32_t amount) {
    if (!amount || amount > CGTradeInfo::m_playerTrade.m_money[0]) {
        return;
    }

    uint32_t money = CGTradeInfo::m_playerTrade.m_money[0] - amount;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_TRADE_GOLD));
    msg.Put(money);
    s_clientStateIndex++;
    msg.Finalize();
    CGTradeInfo::m_playerTrade.m_money[0] = money;
    ClientServices::Send2(&msg);
}

// VERIFY: 369a38df auto
// OFFSET: 0x00586390
static void SetTargetAccepted(int32_t accepted) {
    if (CGTradeInfo::m_targetAccepted != accepted) {
        CGTradeInfo::m_targetAccepted = accepted;
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_ACCEPT_UPDATE, "%d%d", CGTradeInfo::m_playerAccepted, accepted);
    }
}

// VERIFY: e8e301ad auto
// OFFSET: 0x00586410
void CGTradeInfo::DenyPlayerItem(int32_t slot) {
    if (slot == 255) {
        CGTradeInfo::m_lastPlayerMoney = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TRADE_MONEY, nullptr);
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_MONEY, nullptr);
        return;
    }

    CGGameUI::UnlockItem(CGTradeInfo::m_playerItems[slot]);
    CGTradeInfo::m_playerItems[slot] = WOWGUID();
    CGTradeInfo::m_playerItemContainer[slot] = WOWGUID();
    CGTradeInfo::m_playerItemContainerSlot[slot] = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_PLAYER_ITEM_CHANGED, "%d", slot + 1);
}

// VERIFY: aaae39b8 manual
// OFFSET: 0x005864A0
void CGTradeInfo::UnlockTradeItems() {
    for (uint32_t i = 0; i < 7; i++) {
        if (CGTradeInfo::m_playerItems[i]) {
            CGGameUI::UnlockItem(CGTradeInfo::m_playerItems[i]);
        }
    }
}

// VERIFY: 001d35c0 auto
// OFFSET: 0x00586520
int32_t CGTradeInfo::GetGameError(int32_t error, int32_t target) {
    switch (error) {
        case BAG_FULL:
            return target ? ERR_TRADE_BAG_FULL : ERR_TRADE_TARGET_BAG_FULL;

        case BAG_ITEM_MAX_COUNT:
            return target ? ERR_TRADE_TARGET_MAX_COUNT_EXCEEDED : ERR_TRADE_MAX_COUNT_EXCEEDED;

        case BAG_SPELL_FAILED_REAGENTS_GENERIC:
            return target ? ERR_MAX_VALUE : ERR_SPELL_FAILED_REAGENTS_GENERIC;

        case BAG_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS:
            return target ? ERR_TRADE_TARGET_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS : ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS;

        default:
            return GetDisplayErrorByInventoryError(error);
    }
}

// VERIFY: fa685c0b auto
// OFFSET: 0x00586930
void CGTradeInfo::PlayerAccept() {
    if (CGGameUI::CanPerformAction(7) && CGTradeInfo::m_playerAccepted) {
        CGTradeInfo::m_playerAccepted = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_ACCEPT_UPDATE, "%d%d", 0, CGTradeInfo::m_targetAccepted);
        Trade_C_UnacceptTrade();
    }

    if (CGTradeInfo::m_targetAccepted) {
        CGTradeInfo::m_targetAccepted = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_ACCEPT_UPDATE, "%d%d", CGTradeInfo::m_playerAccepted, 0);
    }
}

// VERIFY: 5320b972 manual
// OFFSET: 0x00586AA0
void CGTradeInfo::RemovePlayerItem(WOWGUID item) {
    if (!item) {
        return;
    }

    for (uint32_t i = 0; i < 7; i++) {
        if (CGTradeInfo::m_playerItems[i] == item) {
            CGTradeInfo::SetPlayerItem(i, WOWGUID(), WOWGUID(), 0);
            return;
        }
    }
}

// VERIFY: 617d1e5d auto
// OFFSET: 0x00586B50
void CGTradeInfo::AddCursorItem(int32_t bound) {
    WOWGUID item;
    WOWGUID container;
    uint32_t slot;
    CGGameUI::GetCursorItem(item, container, slot);

    if (!item) {
        return;
    }

    int32_t index = -1;

    if (bound) {
        if (CGTradeInfo::m_playerItems[6]) {
            return;
        }

        index = 6;
    } else {
        for (int32_t i = 0; i < 6; i++) {
            if (CGTradeInfo::m_playerItems[i] == item) {
                return;
            }

            if (index == -1 && !CGTradeInfo::m_playerItems[i]) {
                index = i;
            }
        }

        if (index == -1) {
            return;
        }
    }

    if (CGTradeInfo::SetPlayerItem(index, item, container, static_cast<uint8_t>(slot))) {
        CGGameUI::ClearCursor(0, 1);
    }
}

// VERIFY: 65a17914 auto
// OFFSET: 0x00586C00
void CGTradeInfo::SetEnchantingItem(CGItem_C* item) {
    if (CGTradeInfo::m_enchantingItem) {
        CGGameUI::UnlockItem(CGTradeInfo::m_enchantingItem);
        CGTradeInfo::m_enchantingItem = WOWGUID();
    }

    if (item) {
        WOWGUID none = WOWGUID();
        item->UseItem(&none, 0);
        CGGameUI::LockItem(item->m_obj->m_guid);
        CGTradeInfo::m_enchantingItem = item->m_obj->m_guid;
    }
}

// VERIFY: b172eb13 manual
// OFFSET: 0x00586DC0
void CGTradeInfo::OnTradeUpdated(const TRADE_ITEM* items) {
    CGTradeInfo::PlayerAccept();

    for (int32_t i = 0; i < 7; i++) {
        const TRADE_ITEM& item = items[i];
        int32_t socketsChanged = 0;

        for (uint32_t j = 0; j < 3; j++) {
            if (CGTradeInfo::m_targetSocketEnchants[i][j] != item.m_socketEnchants[j]) {
                socketsChanged = 1;
                break;
            }
        }

        if (CGTradeInfo::m_targetItemID[i] == item.m_itemID && CGTradeInfo::m_targetCount[i] == item.m_count && CGTradeInfo::m_targetEnchant[i] == item.m_enchant && !socketsChanged && CGTradeInfo::m_targetCreator[i] == item.m_creator && CGTradeInfo::m_targetWrapped[i] == item.m_wrapped && CGTradeInfo::m_targetGiftCreator[i] == item.m_giftCreator && CGTradeInfo::m_targetSpellCharges[i] == item.m_spellCharges && CGTradeInfo::m_targetSuffixFactor[i] == item.m_suffixFactor && CGTradeInfo::m_targetRandomPropertyID[i] == item.m_randomPropertyID && CGTradeInfo::m_targetLockID[i] == item.m_lockID && CGTradeInfo::m_targetMaxDurability[i] == item.m_maxDurability && CGTradeInfo::m_targetDurability[i] == item.m_durability) {
            continue;
        }

        CGTradeInfo::m_targetItemID[i] = item.m_itemID;
        CGTradeInfo::m_targetCount[i] = item.m_count;
        CGTradeInfo::m_targetEnchant[i] = item.m_enchant;
        CGTradeInfo::m_targetSocketEnchants[i][0] = item.m_socketEnchants[0];
        CGTradeInfo::m_targetSocketEnchants[i][1] = item.m_socketEnchants[1];
        CGTradeInfo::m_targetSocketEnchants[i][2] = item.m_socketEnchants[2];
        CGTradeInfo::m_targetCreator[i] = item.m_creator;
        CGTradeInfo::m_targetWrapped[i] = item.m_wrapped;
        CGTradeInfo::m_targetLockID[i] = item.m_lockID;
        CGTradeInfo::m_targetGiftCreator[i] = item.m_giftCreator;
        CGTradeInfo::m_targetSpellCharges[i] = item.m_spellCharges;
        CGTradeInfo::m_targetSuffixFactor[i] = item.m_suffixFactor;
        CGTradeInfo::m_targetRandomPropertyID[i] = item.m_randomPropertyID;
        CGTradeInfo::m_targetMaxDurability[i] = item.m_maxDurability;
        CGTradeInfo::m_targetDurability[i] = item.m_durability;
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_TARGET_ITEM_CHANGED, "%d", i + 1);

        if (i == 6 && CGTradeInfo::m_enchantingItem) {
            CGGameUI::UnlockItem(CGTradeInfo::m_enchantingItem);
            CGTradeInfo::m_enchantingItem = WOWGUID();
        }
    }

    if (CGTradeInfo::m_lastPlayerEnchantSpell != Trade_C_GetEnchantSpell(0)) {
        CGTradeInfo::m_lastPlayerEnchantSpell = Trade_C_GetEnchantSpell(0);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_TARGET_ITEM_CHANGED, "%d", 7);
    }

    if (CGTradeInfo::m_lastTargetEnchantSpell != Trade_C_GetEnchantSpell(1)) {
        CGTradeInfo::m_lastTargetEnchantSpell = Trade_C_GetEnchantSpell(1);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_PLAYER_ITEM_CHANGED, "%d", 7);
    }

    if (CGTradeInfo::m_lastPlayerMoney != static_cast<int32_t>(CGTradeInfo::GetPlayerTradeMoney())) {
        CGTradeInfo::m_lastPlayerMoney = CGTradeInfo::GetPlayerTradeMoney();
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TRADE_MONEY, nullptr);
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_MONEY, nullptr);
    }

    if (CGTradeInfo::m_lastTargetMoney != static_cast<int32_t>(CGTradeInfo::GetTargetTradeMoney())) {
        CGTradeInfo::m_lastTargetMoney = CGTradeInfo::GetTargetTradeMoney();
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_MONEY_CHANGED, nullptr);
    }
}

// VERIFY: 6820d853 auto
// OFFSET: 0x005870A0
void CGTradeInfo::SetTradePartner(WOWGUID partner, int32_t suppressMoneyEvent) {
    if (!partner) {
        if (CGTradeInfo::m_tradingPlayer) {
            if (CGTradeInfo::m_enchantingItem) {
                CGGameUI::UnlockItem(CGTradeInfo::m_enchantingItem);
                CGTradeInfo::m_enchantingItem = WOWGUID();
            }

            CGTradeInfo::m_supressMoneyEvent = suppressMoneyEvent;
            CGGameUI::ClearInteractTarget(CGTradeInfo::m_tradingPlayer, 0);
            CGTradeInfo::m_tradingPlayer = WOWGUID();
            FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_CLOSED, nullptr);
        }

        Trade_C_CancelTrade();
        return;
    }

    CGGameUI::SetInteractTarget(partner);
    CGTradeInfo::m_tradingPlayer = partner;
    CGTradeInfo::m_playerAccepted = 0;
    CGTradeInfo::m_targetAccepted = 0;
    CGTradeInfo::m_lastPlayerEnchantSpell = 0;
    CGTradeInfo::m_lastTargetEnchantSpell = 0;
    CGTradeInfo::m_lastPlayerMoney = 0;
    CGTradeInfo::m_lastTargetMoney = 0;
    CGTradeInfo::m_supressMoneyEvent = 0;

    for (uint32_t i = 0; i < 7; i++) {
        CGTradeInfo::m_targetEnchant[i] = 0;
        CGTradeInfo::m_targetCount[i] = 0;
        CGTradeInfo::m_targetItemID[i] = 0;
        CGTradeInfo::m_playerItemContainerSlot[i] = 0;
        CGTradeInfo::m_playerItemContainer[i] = WOWGUID();
        CGTradeInfo::m_playerItems[i] = WOWGUID();
    }

    if (Trade_C_UseCursorItem()) {
        if (CGGameUI::m_cursorMoney) {
            if (CGTradeInfo::AddMoney(CGGameUI::m_cursorMoney)) {
                CGGameUI::ClearCursor(1, 0);
                CGTradeInfo::m_lastPlayerMoney = CGTradeInfo::GetPlayerTradeMoney();
                FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TRADE_MONEY, nullptr);
                FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_MONEY, nullptr);
                FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SHOW, nullptr);
                return;
            }
        } else {
            WOWGUID item;
            WOWGUID container;
            uint32_t slot;
            CGGameUI::GetCursorItem(item, container, slot);

            if (item && container) {
                uint32_t index = 0;
                CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

                if (object && object->IsBound()) {
                    index = 6;
                    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_POTENTIAL_BIND_ENCHANT, "%b", !object->IsBoundForTrade());
                }

                Trade_C_AddItem(item, container, static_cast<uint8_t>(slot), static_cast<uint8_t>(index));
                CGGameUI::ClearCursor(0, 1);
                CGTradeInfo::m_playerItems[index] = item;
                CGTradeInfo::m_playerItemContainer[index] = container;
                CGTradeInfo::m_playerItemContainerSlot[index] = static_cast<uint8_t>(slot);
            }
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SHOW, nullptr);
}

// VERIFY: 15291c4e auto
// OFFSET: 0x005873E0
void CGTradeInfo::HandleTradeMessage(TRADE_STATUS status, int32_t bagResult, int32_t target, int32_t itemId, uint8_t slot) {
    switch (status) {
        case TRADE_STATUS_PLAYER_BUSY:
        case TRADE_STATUS_ALREADY_TRADING:
            if (Trade_C_IsTradeRequested()) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(Trade_C_GetTradeTarget(), TYPE_PLAYER, __FILE__, __LINE__));

                if (unit) {
                    CGGameUI::DisplayError(ERR_PLAYER_BUSY_S, unit->GetUnitName(nullptr, 1));
                }
            }

            break;

        case TRADE_STATUS_PROPOSED:
            if (!Trade_C_IsTradeRequested()) {
                if (FriendList::s_friendList->IsIgnored(Trade_C_GetTradeTarget(), true)) {
                    Trade_C_PlayerIgnored();
                } else {
                    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(Trade_C_GetTradeTarget(), TYPE_PLAYER, __FILE__, __LINE__));

                    if (unit) {
                        if (unit->IsDeadOrGhost() || CGGameUI::m_inCinematic || !CGGameUI::m_hasControl || CGAuctionHouse::m_auctioneer) {
                            Trade_C_PlayerBusy();
                        } else if (CVar::Lookup("BlockTrades")->m_intValue) {
                            Trade_C_PlayerBusy();
                            CGGameUI::DisplayError(ERR_TRADE_BLOCKED_S, unit->GetUnitName(nullptr, 1));
                        } else {
                            Trade_C_BeginTrade();
                        }
                    }
                }
            }

            break;

        case TRADE_STATUS_INITIATED:
            CGTradeInfo::SetTradePartner(Trade_C_GetTradeTarget(), 0);
            break;

        case TRADE_STATUS_CANCELLED:
            FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_REQUEST_CANCEL, nullptr);
            CGTradeInfo::UnlockTradeItems();
            CGTradeInfo::SetTradePartner(WOWGUID(), 0);
            CGGameUI::DisplayError(ERR_TRADE_CANCELLED);
            break;

        case TRADE_STATUS_ACCEPTED:
            SetTargetAccepted(1);
            break;

        case TRADE_STATUS_PLAYER_NOT_FOUND:
            CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
            break;

        case TRADE_STATUS_STATE_CHANGED:
            CGTradeInfo::PlayerAccept();
            break;

        case TRADE_STATUS_COMPLETE:
            CGTradeInfo::UnlockTradeItems();
            CGTradeInfo::SetTradePartner(WOWGUID(), 1);
            CGGameUI::DisplayError(ERR_TRADE_COMPLETE);
            break;

        case TRADE_STATUS_UNACCEPTED:
            SetTargetAccepted(0);
            break;

        case TRADE_STATUS_TOO_FAR_AWAY:
            CGGameUI::DisplayError(ERR_TRADE_TOO_FAR);
            break;

        case TRADE_STATUS_WRONG_FACTION:
            CGGameUI::DisplayError(ERR_PLAYER_WRONG_FACTION);
            break;

        case TRADE_STATUS_FAILED:
            CGTradeInfo::UnlockTradeItems();

            if (bagResult == 84 && itemId) {
                WOWGUID none = WOWGUID();
                ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

                if (stats) {
                    ItemLimitCategoryRec* category = g_itemLimitCategoryDB.GetRecord(stats->m_itemLimitCategory);

                    if (category) {
                        CGGameUI::DisplayError(static_cast<UIErrors>(CGTradeInfo::GetGameError(84, target)), category->m_quantity, category->m_name);
                    }
                }
            } else {
                CGGameUI::DisplayError(static_cast<UIErrors>(CGTradeInfo::GetGameError(bagResult, target)));
            }

            CGTradeInfo::SetTradePartner(WOWGUID(), 0);
            break;

        case TRADE_STATUS_PLAYER_IGNORED:
            if (Trade_C_IsTradeRequested()) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(Trade_C_GetTradeTarget(), TYPE_PLAYER, __FILE__, __LINE__));

                if (unit) {
                    CGGameUI::DisplayError(ERR_IGNORING_YOU_S, unit->GetUnitName(nullptr, 1));
                }
            }

            break;

        case TRADE_STATUS_PLAYER_STUNNED:
            CGGameUI::DisplayError(ERR_GENERIC_STUNNED);
            break;

        case TRADE_STATUS_TARGET_STUNNED:
            CGGameUI::DisplayError(ERR_TARGET_STUNNED);
            break;

        case TRADE_STATUS_DEAD:
            CGGameUI::DisplayError(ERR_PLAYER_DEAD);
            break;

        case TRADE_STATUS_TARGET_DEAD:
            CGGameUI::DisplayError(ERR_TRADE_TARGET_DEAD);
            break;

        case TRADE_STATUS_LOGGING_OUT:
            CGGameUI::DisplayError(ERR_LOGGING_OUT);
            break;

        case TRADE_STATUS_TARGET_LOGGING_OUT:
            CGGameUI::DisplayError(ERR_TARGET_LOGGING_OUT);
            break;

        case TRADE_STATUS_RESTRICTED_ACCOUNT:
            CGGameUI::DisplayError(ERR_RESTRICTED_ACCOUNT);
            break;

        case TRADE_STATUS_WRONG_REALM:
            CGGameUI::DisplayError(ERR_TRADE_WRONG_REALM);
            CGTradeInfo::DenyPlayerItem(slot);
            break;

        case TRADE_STATUS_NOT_ON_TAPLIST:
            CGGameUI::DisplayError(ERR_TRADE_NOT_ON_TAPLIST);
            CGTradeInfo::DenyPlayerItem(slot);
            break;
    }
}

// VERIFY: bbebe902 manual
// OFFSET: 0x005877B0
bool CGTradeInfo::AddPlayerItem(WOWGUID item, WOWGUID container, uint8_t slot) {
    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));
    uint32_t index = 0;

    if (!object) {
        return false;
    }

    if (object->IsBound()) {
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, none, nullptr, nullptr, false);

        if (stats && stats->m_bonding == 4) {
            CGGameUI::DisplayError(ERR_TRADE_QUEST_ITEM);
            return false;
        }

        if (CGTradeInfo::m_playerItems[6]) {
            return false;
        }

        index = 6;
    } else {
        while (CGTradeInfo::m_playerItems[index]) {
            if (++index >= 6) {
                return false;
            }
        }
    }

    if (object->m_obj->m_type & TYPE_CONTAINER) {
        const CGBag_C* bag = object->GetBag();

        if (bag) {
            if (bag->GetNumItems()) {
                CGGameUI::DisplayError(ERR_TRADE_BAG);
                return false;
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (static_cast<uint32_t>(player->GetBag()->FindItemSlot(object) - 19) <= 3) {
                CGGameUI::DisplayError(ERR_TRADE_EQUIPPED_BAG);
                return false;
            }
        }
    }

    CGTradeInfo::SetPlayerItem(index, item, container, slot);
    CGGameUI::LockItem(item);
    SndInterfacePlayItemSound(ITEMSOUND_DROP, object);
    return true;
}

// VERIFY: 20baf230 manual
// OFFSET: 0x005880C0
void CGTradeInfo::LeaveWorld() {
    CGTradeInfo::UnlockTradeItems();
    CGTradeInfo::SetTradePartner(WOWGUID(), 0);
    Trade_C_CancelTrade();
}
