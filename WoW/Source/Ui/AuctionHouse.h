#ifndef WOW_SOURCE_UI_AUCTIONHOUSE_H
#define WOW_SOURCE_UI_AUCTIONHOUSE_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
class CGItem_C;
struct lua_State;

struct AuctionSortCriteria {
    /* 0000 */ int32_t m_column;
    /* 0004 */ int32_t m_reverse;
    /* 0008 */ int32_t m_valid;
};

struct AuctionSellItem {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint32_t m_count;
    /* 000C */ uint32_t unused_000C;
};

struct OutbidItemInfo {
    /* 0000 */ int32_t m_auctionHouseID;
    /* 0004 */ int32_t m_auctionID;
};

struct AuctionItemClass {
    /* 0000 */ int32_t m_classID;
    /* 0004 */ int32_t m_hasSubClasses;
};

struct AuctionInvType {
    /* 0000 */ int32_t m_invType;
    /* 0004 */ char m_token[32];
    /* 0024 */ int32_t unk_0024;
    /* 0028 */ int32_t unk_0028;
};

struct AuctionItemInfo {
    /* 0000 */ int32_t m_auctionHouseID;
    /* 0004 */ uint32_t m_auctionID;
    /* 0008 */ uint32_t m_itemID;
    /* 000C */ uint32_t m_enchantID[7];
    /* 0028 */ uint32_t m_enchantDuration[7];
    /* 0044 */ uint32_t m_enchantCharges[7];
    /* 0060 */ uint32_t m_randomPropertyID;
    /* 0064 */ uint32_t m_suffixFactor;
    /* 0068 */ uint32_t m_count;
    /* 006C */ uint32_t m_spellCharges;
    /* 0070 */ uint32_t m_flags;
    /* 0074 */ uint32_t unused_0074;
    /* 0078 */ WOWGUID m_owner;
    /* 0080 */ uint32_t m_minBid;
    /* 0084 */ uint32_t m_minIncrement;
    /* 0088 */ uint32_t m_buyout;
    /* 008C */ uint32_t m_expireTime;
    /* 0090 */ WOWGUID m_bidder;
    /* 0098 */ uint32_t m_bid;
    /* 009C */ int32_t m_sold;
};

struct AuctionNotifyInfo {
    /* 0000 */ int32_t m_randomPropertyID;
    /* 0004 */ int32_t m_error;
};

class CGAuctionHouse {
    public:
    static void CloseAuctionHouse();
    static bool IsItemForSell(WOWGUID item);
    static AuctionSortCriteria m_bidSortCriteria[12];
    static AuctionSortCriteria m_ownedSortCriteria[12];
    static AuctionSortCriteria m_listSortCriteria[12];
    static int32_t m_numOutbidItems;
    static uint32_t m_lastGetAllTime;
    static TSGrowableArray<OutbidItemInfo*> m_outbidItems;
    static void SetBrowseSortCriteria(int32_t column, bool reverse);
    static void SetOwnedSortCriteria(int32_t column, bool reverse);
    static void SetBidSortCriteria(int32_t column, bool reverse);
    static WOWGUID m_auctioneer;
    static uint8_t m_sellItemSlot;
    static WOWGUID m_sellItem;
    static uint32_t m_multisellRemaining;
    static WOWGUID m_multisellAuctioneer;
    static uint32_t m_multisellBid;
    static uint32_t m_multisellBuyout;
    static uint32_t m_multisellDuration;
    static uint32_t m_multisellStackSize;
    static uint32_t m_multisellTotal;
    static uint32_t m_numSellItems;
    static AuctionSellItem m_sellItems[200];
    static int32_t m_havePendingSell;
    static WOWGUID m_sellItemContainer;
    static uint32_t m_lastQueryTime[3];
    static int32_t m_queryPending[3];
    static uint32_t m_numTotalOwned;
    static int32_t m_canGetOwnerList;
    static int32_t m_canGetBidderList;
    static int32_t m_pendingBrowseItems;
    static int32_t m_pendingOwnedItems;
    static int32_t m_pendingSoldItems;
    static int32_t m_pendingBidItems;
    static int32_t m_lastBidderPage;
    static uint32_t m_lastBidAmount;
    static int32_t m_desiredDelayTime;
    static int32_t m_selectedBrowseItem;
    static int32_t m_selectedOwnedItem;
    static int32_t m_selectedBidItem;
    static uint32_t m_numTotalItems;
    static uint32_t m_numTotalBid;
    static TSGrowableArray<AuctionItemInfo> m_browseList;
    static TSGrowableArray<AuctionItemInfo> m_ownerList;
    static TSGrowableArray<AuctionItemInfo> m_bidderList;
    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static uint8_t m_auctionsTabShowing;
    static bool IsSellItemValid(CGItem_C* item, int32_t showError);
    static void GetSellItem(WOWGUID* item, WOWGUID* container, uint8_t* slot);
    static void SetSellItem(WOWGUID item, WOWGUID container, uint8_t slot);
    static int32_t m_auctionHouseID;
    static bool CanSendQuery(int32_t type);
    static AuctionSortCriteria* GetBrowseSortCriteria(uint32_t index);
    static AuctionSortCriteria* GetBidSortCriteria(uint32_t index);
    static void ClearSellItem();
    static void ClearBrowseSortCriteria();
    static void ClearOwnedSortCriteria();
    static void ClearBidSortCriteria();
    static const char* SortTypeToName(int32_t column);
    static bool TableNameToType(const char* name, int32_t* type);
    static void LockAllSellItems();
    static void UnlockAllSellItems();
    static void OnMultisellFailure(bool silent);
    static void SetSelectedBrowseItem(uint32_t index);
    static void SetSelectedOwnedItem(uint32_t index);
    static void SetSelectedBidItem(uint32_t index);
    static int32_t GetSelectedBrowseIndex();
    static int32_t GetSelectedOwnedIndex();
    static int32_t GetSelectedBidIndex();
    static void ApplyBrowseSort();
    static void ApplyBidderSort();
    static void ClearOutbidID(int32_t auctionID);
    static void SellNextItem();
    static void GetOwnerList();
    static void SortBrowseList(int32_t column);
    static void SortOwnedList(int32_t column);
    static void SortBidderList(int32_t column);
    static void SetAuctioneer(WOWGUID auctioneer, int32_t auctionHouseID);
    static void AddOutbidID(int32_t auctionHouseID, int32_t auctionID);
    static void AuctionItemExpired(int32_t auctionID);
};

void AuctionHouseRegisterScriptFunctions();
void AuctionHouseUnregisterScriptFunctions();

int32_t Packet_MSG_AUCTION_HELLO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_COMMAND_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_OWNER_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_BIDDER_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_BIDDER_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_OWNER_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_REMOVED_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_AUCTION_LIST_PENDING_SALES(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

AuctionItemInfo* GetListAuction(uint32_t index);
AuctionItemInfo* GetOwnerAuction(uint32_t index);
AuctionItemInfo* GetBidderAuction(uint32_t index);
bool AuctionSortToColumnIndex(const char* name, int32_t* column);
int32_t AuctionListComparer(const void* a, const void* b);
int32_t CompareFunction(const void* a, const void* b);
int32_t AuctionBidderListComparer(const void* a, const void* b);
void RequestListPendingSales();
void BidderListItems(int32_t page);
void AuctionListItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void AuctionOwnedItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void AuctionOwnedSoldItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void AuctionBidItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void Signal_EVENT_AUCTION_ITEM_LIST_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success);
void Signal_EVENT_AUCTION_OWNED_LIST_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success);
void Signal_EVENT_AUCTION_BIDDER_LIST_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success);
void AuctionSellItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
int32_t FindItemForAuction(WOWGUID item, uint32_t count);
int32_t FindSellableItems(int32_t itemId);
uint32_t CalculateAuctionDeposit(uint32_t depositRate, uint32_t sellPrice, uint32_t runTime);

#endif
