#ifndef WOW_SOURCE_UI_TRADEFRAME_H
#define WOW_SOURCE_UI_TRADEFRAME_H

#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

enum TRADE_STATUS : int32_t {
    TRADE_STATUS_PLAYER_BUSY = 0,
    TRADE_STATUS_PROPOSED = 1,
    TRADE_STATUS_INITIATED = 2,
    TRADE_STATUS_CANCELLED = 3,
    TRADE_STATUS_ACCEPTED = 4,
    TRADE_STATUS_ALREADY_TRADING = 5,
    TRADE_STATUS_PLAYER_NOT_FOUND = 6,
    TRADE_STATUS_STATE_CHANGED = 7,
    TRADE_STATUS_COMPLETE = 8,
    TRADE_STATUS_UNACCEPTED = 9,
    TRADE_STATUS_TOO_FAR_AWAY = 10,
    TRADE_STATUS_WRONG_FACTION = 11,
    TRADE_STATUS_FAILED = 12,
    TRADE_STATUS_UNKNOWN_13 = 13,
    TRADE_STATUS_PLAYER_IGNORED = 14,
    TRADE_STATUS_PLAYER_STUNNED = 15,
    TRADE_STATUS_TARGET_STUNNED = 16,
    TRADE_STATUS_DEAD = 17,
    TRADE_STATUS_TARGET_DEAD = 18,
    TRADE_STATUS_LOGGING_OUT = 19,
    TRADE_STATUS_TARGET_LOGGING_OUT = 20,
    TRADE_STATUS_RESTRICTED_ACCOUNT = 21,
    TRADE_STATUS_WRONG_REALM = 22,
    TRADE_STATUS_NOT_ON_TAPLIST = 23,

    // Unused 0.5.3 name/value; current 14 means PLAYER_IGNORED.
    // TRADE_STATUS_PETITION = 14,
};

class CGItem_C;

struct lua_State;

struct PlayerTradeState {
    /* 0000 */ uint32_t m_tradeId;
    /* 0004 */ uint32_t m_stateIndex[2];
    /* 000C */ uint32_t m_money[2];
};

struct TRADE_ITEM {
    /* 0000 */ int32_t m_itemID;
    /* 0004 */ int32_t m_displayID;
    /* 0008 */ int32_t m_count;
    /* 000C */ int32_t m_enchant;
    /* 0010 */ int32_t m_socketEnchants[3];
    /* 001C */ uint32_t unused_001C;
    /* 0020 */ WOWGUID m_creator;
    /* 0028 */ int32_t m_wrapped;
    /* 002C */ uint32_t unused_002C;
    /* 0030 */ WOWGUID m_giftCreator;
    /* 0038 */ int32_t m_spellCharges;
    /* 003C */ int32_t m_suffixFactor;
    /* 0040 */ int32_t m_randomPropertyID;
    /* 0044 */ int32_t m_lockID;
    /* 0048 */ int32_t m_maxDurability;
    /* 004C */ int32_t m_durability;
};

class CGTradeInfo {
    public:
    static void SetTradePartner(WOWGUID partner, int32_t suppressMoneyEvent);
    static void SetEnchantingItem(CGItem_C* item);
    static bool IsItemBeingTraded(WOWGUID item);
    static bool AddPlayerItem(WOWGUID item, WOWGUID container, uint8_t slot);
    static WOWGUID m_enchantingItem;
    static void EnterWorld();
    static uint32_t GetPlayerTradeMoney();
    static void RemoveMoney(uint32_t amount);
    static int32_t m_supressMoneyEvent;
    static int32_t GetTargetEnchant(uint32_t slot);
    static void GetPlayerItem(uint32_t slot, WOWGUID& item, WOWGUID& container, uint8_t& containerSlot);
    static int32_t SetPlayerItem(uint32_t slot, WOWGUID item, WOWGUID container, uint8_t containerSlot);
    static bool AddMoney(int32_t amount);
    static bool SetMoney(uint32_t amount);
    static uint32_t GetTargetTradeMoney();
    static WOWGUID m_tradingPlayer;
    static PlayerTradeState m_playerTrade;
    static int32_t m_targetDurability[7];
    static int32_t m_targetMaxDurability[7];
    static int32_t m_targetLockID[7];
    static int32_t m_targetRandomPropertyID[7];
    static int32_t m_targetSuffixFactor[7];
    static int32_t m_targetSpellCharges[7];
    static WOWGUID m_targetGiftCreator[7];
    static int32_t m_targetWrapped[7];
    static int32_t m_targetEnchant[7];
    static WOWGUID m_targetCreator[7];
    static int32_t m_targetSocketEnchants[7][3];
    static int32_t m_targetCount[7];
    static int32_t m_targetItemID[7];
    static uint8_t m_playerItemContainerSlot[7];
    static WOWGUID m_playerItemContainer[7];
    static WOWGUID m_playerItems[7];
    static int32_t m_playerAccepted;
    static int32_t m_targetAccepted;
    static int32_t m_lastPlayerEnchantSpell;
    static int32_t m_lastTargetEnchantSpell;
    static int32_t m_lastPlayerMoney;
    static int32_t m_lastTargetMoney;

    static void UpdatePlayerItem(WOWGUID guid);
    static void AddCursorItem(int32_t bound);
    static void InitiateTrade(WOWGUID guid, int32_t useCursorItem);
    static void UnlockTradeItems();
    static void DenyPlayerItem(int32_t slot);
    static int32_t GetGameError(int32_t error, int32_t target);
    static void PlayerAccept();
    static void RemovePlayerItem(WOWGUID item);
    static void OnTradeUpdated(const TRADE_ITEM* items);
    static void HandleTradeMessage(TRADE_STATUS status, int32_t bagResult, int32_t target, int32_t itemId, uint8_t slot);
    static void LeaveWorld();
};

void TradeInfoRegisterScriptFunctions();
void TradeInfoUnregisterScriptFunctions();

void TradeItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success);

#endif
