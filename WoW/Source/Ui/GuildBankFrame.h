#ifndef WOW_SOURCE_UI_GUILDBANKFRAME_H
#define WOW_SOURCE_UI_GUILDBANKFRAME_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/WGUID.h"

class CDataStore;
struct lua_State;

struct GuildBankTabInfo {
    /* 0000 */ char m_name[64];
    /* 0040 */ char m_icon[256];
    /* 0140 */ char m_text[8192];
    /* 2140 */ int32_t m_needsFullUpdate;
};

struct GuildBankLogEntry {
    /* 0000 */ uint32_t m_type;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ WOWGUID m_guid;
    /* 0010 */ int32_t m_itemID;
    /* 0014 */ uint32_t m_count;
    /* 0018 */ uint32_t m_money;
    /* 001C */ uint32_t m_otherTab;
    /* 0020 */ uint32_t m_index;
    /* 0024 */ uint32_t m_time;
};

struct GuildBankSlotInfo {
    /* 0000 */ int32_t m_itemID;
    /* 0004 */ uint32_t m_suffixFactor;
    /* 0008 */ int32_t m_randomPropertyID;
    /* 000C */ int32_t m_count;
    /* 0010 */ uint32_t m_enchantId;
    /* 0014 */ uint32_t m_spellCharges;
    /* 0018 */ uint32_t m_enchants[3];
    /* 0024 */ int32_t m_locked;
    /* 0028 */ uint32_t m_flags;
};

int32_t Packet_SMSG_GUILD_BANK_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_GUILD_BANK_LOG_QUERY(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
void SendGuildBankQueryTabOpcode(uint8_t tab);
int32_t Packet_MSG_GUILD_BANK_MONEY_WITHDRAWN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_QUERY_GUILD_BANK_TEXT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

class CGGuildBankInfo {
    public:
    static void UpdateTabInfo(uint8_t tab, const char* name, const char* icon);
    static void SetTextUpdate(uint8_t tab, int32_t update);
    static void ClearGuildTabList(uint32_t tab);
    static int32_t m_withdrawMoneyRemaining;
    static WOWGUID m_banker;
    static WOWGUID m_pendingBanker;
    static void AutoStoreItem(WOWGUID item, WOWGUID container, uint32_t slot);
    static uint8_t m_numTabs;
    static GuildBankTabInfo m_guildBankTabInfo[6];
    static uint8_t m_currentTab;
    static int32_t m_remainingWithdrawMoney;
    static uint64_t m_money;
    static int32_t m_numLogEntries[7];
    static uint8_t m_logTab;
    static GuildBankLogEntry m_log[7][25];
    static int32_t m_tabContentsDirty[6];
    static TSGrowableArray<GuildBankSlotInfo> m_tabSlots[6];

    static void SetGuildBanker(const WOWGUID& banker);
    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static void ClearAllTabs();
    static void Unlock(int32_t itemId, uint32_t virtualSlot);
    static void GetTabAndSlotFromVirtual(uint32_t virtualSlot, uint32_t* tab, uint32_t* slot);
    static GuildBankSlotInfo* GetGuildBankItem(uint32_t tab, uint32_t slot);
    static int32_t IsLocked(int32_t itemId, uint32_t virtualSlot);
    static void Lock(int32_t itemId, uint32_t virtualSlot);
    static void BuyGuildBankTab();
    static void SwapItems(uint8_t tab, uint32_t slot, WOWGUID item, WOWGUID container, uint32_t containerSlot, int32_t split);
    static void SwapBankItems(uint8_t tab, uint32_t slot, int32_t itemId, uint32_t virtualSlot, int32_t split);
    static void QueryGuildBankLog(uint8_t tab);
    static void QueryGuildBankText(uint8_t tab);
    static void SetTabInfo(uint8_t tab, const char* name, const char* icon);
    static void DepositMoney(uint32_t amount);
    static void WithdrawMoney(uint32_t amount);
    static void SetGuildBankText(uint8_t tab, const char* text);
};

void GuildBankRegisterScriptFunctions();
void GuildBankUnregisterScriptFunctions();

#endif
