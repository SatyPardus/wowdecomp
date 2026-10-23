#include "AuctionHouse.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/UILCD/LCD.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

struct AuctionFindItemsParam {
    /* 0000 */ int32_t m_itemId;
    /* 0004 */ uint32_t m_total;
    /* 0008 */ uint32_t m_wanted;
    /* 000C */ bool m_collect;
};

// STATIC: 0x00A394BC
static const uint32_t s_depositRunTimes[3] = { 720, 1440, 2880 };

// STATIC: 0x00C0E740
uint32_t CGAuctionHouse::m_multisellRemaining;

// STATIC: 0x00C0E748
WOWGUID CGAuctionHouse::m_multisellAuctioneer;

// STATIC: 0x00C0E750
uint32_t CGAuctionHouse::m_multisellBid;

// STATIC: 0x00C0E754
uint32_t CGAuctionHouse::m_multisellBuyout;

// STATIC: 0x00C0E758
uint32_t CGAuctionHouse::m_multisellDuration;

// STATIC: 0x00C0E75C
uint32_t CGAuctionHouse::m_multisellStackSize;

// STATIC: 0x00C0E760
uint32_t CGAuctionHouse::m_multisellTotal;

// STATIC: 0x00C0E764
uint32_t CGAuctionHouse::m_numSellItems;

// STATIC: 0x00C0E768
AuctionSellItem CGAuctionHouse::m_sellItems[200];

// STATIC: 0x00C0F434
int32_t CGAuctionHouse::m_havePendingSell;

// STATIC: 0x00C0F3F5
uint8_t CGAuctionHouse::m_auctionsTabShowing;

// STATIC: 0x00C0F3F0
int32_t CGAuctionHouse::m_auctionHouseID;

// STATIC: 0x00C0F3E8
WOWGUID CGAuctionHouse::m_auctioneer;

// STATIC: 0x00C0F408
uint32_t CGAuctionHouse::m_numTotalItems;

// STATIC: 0x00C0F410
uint32_t CGAuctionHouse::m_numTotalBid;

// STATIC: 0x00C0F440
TSGrowableArray<AuctionItemInfo> CGAuctionHouse::m_browseList;

// STATIC: 0x00C0F450
TSGrowableArray<AuctionItemInfo> CGAuctionHouse::m_ownerList;

// STATIC: 0x00C0F460
TSGrowableArray<AuctionItemInfo> CGAuctionHouse::m_bidderList;

// STATIC: 0x00ACF628
int32_t CGAuctionHouse::m_desiredDelayTime = 5000;

// STATIC: 0x00ACF62C
int32_t CGAuctionHouse::m_selectedBrowseItem = -1;

// STATIC: 0x00ACF630
int32_t CGAuctionHouse::m_selectedOwnedItem = -1;

// STATIC: 0x00ACF634
int32_t CGAuctionHouse::m_selectedBidItem = -1;

// STATIC: 0x00C0E728
uint32_t CGAuctionHouse::m_lastQueryTime[3];

// STATIC: 0x00C0E734
int32_t CGAuctionHouse::m_queryPending[3];

// STATIC: 0x00C0F3F4
uint8_t CGAuctionHouse::m_sellItemSlot;

// STATIC: 0x00C0F3F8
WOWGUID CGAuctionHouse::m_sellItem;

// STATIC: 0x00C0F400
WOWGUID CGAuctionHouse::m_sellItemContainer;

// STATIC: 0x00C0F40C
uint32_t CGAuctionHouse::m_numTotalOwned;

// STATIC: 0x00C0F41C
int32_t CGAuctionHouse::m_canGetOwnerList;

// STATIC: 0x00C0F420
int32_t CGAuctionHouse::m_canGetBidderList;

// STATIC: 0x00C0F424
int32_t CGAuctionHouse::m_pendingBrowseItems;

// STATIC: 0x00C0F428
int32_t CGAuctionHouse::m_pendingOwnedItems;

// STATIC: 0x00C0F42C
int32_t CGAuctionHouse::m_pendingSoldItems;

// STATIC: 0x00C0F430
int32_t CGAuctionHouse::m_pendingBidItems;

// STATIC: 0x00C0F438
int32_t CGAuctionHouse::m_lastBidderPage;

// STATIC: 0x00C0F43C
uint32_t CGAuctionHouse::m_lastBidAmount;

// STATIC: 0x00C0E578
AuctionSortCriteria CGAuctionHouse::m_bidSortCriteria[12];

// STATIC: 0x00C0E608
AuctionSortCriteria CGAuctionHouse::m_ownedSortCriteria[12];

// STATIC: 0x00C0E698
AuctionSortCriteria CGAuctionHouse::m_listSortCriteria[12];

// STATIC: 0x00C0F414
int32_t CGAuctionHouse::m_numOutbidItems;

// STATIC: 0x00C0F418
uint32_t CGAuctionHouse::m_lastGetAllTime;

// STATIC: 0x00C0F470
TSGrowableArray<OutbidItemInfo*> CGAuctionHouse::m_outbidItems;

// VERIFY: de3c2741 manual
// OFFSET: 0x0059B280
AuctionItemInfo* GetListAuction(uint32_t index) {
    if (index >= CGAuctionHouse::m_browseList.m_count) {
        return nullptr;
    }

    return &CGAuctionHouse::m_browseList.m_data[index];
}

// VERIFY: 0061e252 manual
// OFFSET: 0x0059B2A0
AuctionItemInfo* GetOwnerAuction(uint32_t index) {
    if (index >= CGAuctionHouse::m_ownerList.m_count) {
        return nullptr;
    }

    return &CGAuctionHouse::m_ownerList.m_data[index];
}

// VERIFY: 3bbf2e96 manual
// OFFSET: 0x0059B2C0
AuctionItemInfo* GetBidderAuction(uint32_t index) {
    if (index >= CGAuctionHouse::m_bidderList.m_count) {
        return nullptr;
    }

    return &CGAuctionHouse::m_bidderList.m_data[index];
}

// VERIFY: 1c82549b auto
// OFFSET: 0x0059DFB0
void CGAuctionHouse::EnterWorld() {
    if (CGAuctionHouse::m_sellItem) {
        CGGameUI::UnlockItem(CGAuctionHouse::m_sellItem);
        CGAuctionHouse::m_sellItem = WOWGUID();
        CGAuctionHouse::m_sellItemContainer = WOWGUID();
        CGAuctionHouse::m_sellItemSlot = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_NEW_AUCTION_UPDATE, nullptr);
    }

    CGAuctionHouse::m_lastQueryTime[0] = 0;
    CGAuctionHouse::m_lastQueryTime[1] = 0;
    CGAuctionHouse::m_lastQueryTime[2] = 0;
    CGAuctionHouse::m_queryPending[0] = 0;
    CGAuctionHouse::m_queryPending[1] = 0;
    CGAuctionHouse::m_queryPending[2] = 0;
    CGAuctionHouse::m_auctioneer = WOWGUID();
    CGAuctionHouse::m_auctionHouseID = 0;
    CGAuctionHouse::m_browseList.m_count = 0;
    CGAuctionHouse::m_numTotalItems = 0;
    CGAuctionHouse::m_numTotalOwned = 0;
    CGAuctionHouse::m_numTotalBid = 0;
    CGAuctionHouse::m_canGetOwnerList = 0;
    CGAuctionHouse::m_canGetBidderList = 0;
    CGAuctionHouse::m_pendingBrowseItems = 0;
    CGAuctionHouse::m_pendingOwnedItems = 0;
    CGAuctionHouse::m_pendingSoldItems = 0;
    CGAuctionHouse::m_pendingBidItems = 0;
    CGAuctionHouse::m_auctionsTabShowing = 0;
    CGAuctionHouse::m_desiredDelayTime = 5000;
    CGAuctionHouse::m_selectedBrowseItem = -1;
    CGAuctionHouse::m_selectedOwnedItem = -1;
    CGAuctionHouse::m_selectedBidItem = -1;
}

// VERIFY: d859c970 auto
// OFFSET: 0x0059A280
void CGAuctionHouse::SetSellItem(WOWGUID item, WOWGUID container, uint8_t slot) {
    CGAuctionHouse::m_sellItem = item;
    CGAuctionHouse::m_sellItemContainer = container;
    CGAuctionHouse::m_sellItemSlot = slot;
}

// VERIFY: ece8da77 auto
// OFFSET: 0x0059A2B0
void CGAuctionHouse::GetSellItem(WOWGUID* item, WOWGUID* container, uint8_t* slot) {
    *item = CGAuctionHouse::m_sellItem;
    *container = CGAuctionHouse::m_sellItemContainer;
    *slot = CGAuctionHouse::m_sellItemSlot;
}

// VERIFY: dc362496 manual
// OFFSET: 0x0059A2F0
bool CGAuctionHouse::CanSendQuery(int32_t type) {
    if (type >= 3 || CGAuctionHouse::m_queryPending[type] > 0) {
        return false;
    }

    return !CGAuctionHouse::m_lastQueryTime[type] || static_cast<int32_t>(OsGetAsyncTimeMs() - CGAuctionHouse::m_lastQueryTime[type] - CGAuctionHouse::m_desiredDelayTime) >= 0;
}

// VERIFY: 82557b75 manual
// OFFSET: 0x0059A340
AuctionSortCriteria* CGAuctionHouse::GetBrowseSortCriteria(uint32_t index) {
    return index < 12 ? &CGAuctionHouse::m_listSortCriteria[index] : nullptr;
}

// VERIFY: 68838e6e manual
// OFFSET: 0x0059A360
AuctionSortCriteria* CGAuctionHouse::GetBidSortCriteria(uint32_t index) {
    return index < 12 ? &CGAuctionHouse::m_bidSortCriteria[index] : nullptr;
}

// VERIFY: 8a8f2619 auto
// OFFSET: 0x0059A380
void Signal_EVENT_AUCTION_ITEM_LIST_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
    }
}

// VERIFY: 9651bd90 auto
// OFFSET: 0x0059A3A0
void Signal_EVENT_AUCTION_OWNED_LIST_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
    }
}

// VERIFY: 43469796 auto
// OFFSET: 0x0059A3C0
void Signal_EVENT_AUCTION_BIDDER_LIST_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_BIDDER_LIST_UPDATE, nullptr);
    }
}

// VERIFY: b21805db auto
// OFFSET: 0x0059A3E0
void AuctionSellItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_NEW_AUCTION_UPDATE, nullptr);
    }
}

// VERIFY: fa1bd128 manual
// OFFSET: 0x0059A400
static void AucNotifyCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    AuctionNotifyInfo* info = static_cast<AuctionNotifyInfo*>(param);

    if (success) {
        char name[256];
        CGItem_C::BuildItemName(name, sizeof(name), id, info->m_randomPropertyID);
        CGGameUI::DisplayError(static_cast<UIErrors>(info->m_error), name);

        switch (info->m_error) {
            case ERR_AUCTION_WON_S:
                CGLCD::AuctionWon(name);
                break;

            case ERR_AUCTION_SOLD_S:
                CGLCD::AuctionSold(name);
                break;

            case ERR_AUCTION_EXPIRED_S:
                CGLCD::AuctionExpired(name);
                break;
        }
    }

    delete info;
}

// VERIFY: 944a7872 manual
// OFFSET: 0x0059A4A0
static int32_t ParseAuctionMailSubject(const char* subject, uint32_t* itemId, uint32_t* randomPropertyId, uint32_t* action, uint32_t* auctionId) {
    *itemId = 0;
    *randomPropertyId = 0;
    *action = 0;
    *auctionId = 0;

    int32_t fields = 1;
    *itemId = SStrToInt(subject);

    const char* field = SStrChr(subject + 1, ':');

    if (!field || !*field) {
        return fields;
    }

    *randomPropertyId = SStrToInt(field + 1);
    fields++;
    field = SStrChr(field + 1, ':');

    if (!field || !*field) {
        return fields;
    }

    *action = SStrToInt(field + 1);
    fields++;
    field = SStrChr(field + 1, ':');

    if (!field || !*field) {
        return fields;
    }

    *auctionId = SStrToInt(field + 1);
    return fields + 1;
}

// VERIFY: dd44ff0f auto
// OFFSET: 0x0059A560
void CGAuctionHouse::ClearSellItem() {
    if (CGAuctionHouse::m_sellItem) {
        CGGameUI::UnlockItem(CGAuctionHouse::m_sellItem);
        CGAuctionHouse::m_sellItem = WOWGUID();
        CGAuctionHouse::m_sellItemContainer = WOWGUID();
        CGAuctionHouse::m_sellItemSlot = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_NEW_AUCTION_UPDATE, nullptr);
    }
}

// VERIFY: 1e8210f6 auto
// OFFSET: 0x0059A5B0
void CGAuctionHouse::ClearBrowseSortCriteria() {
    for (uint32_t i = 0; i < 12; i++) {
        CGAuctionHouse::m_listSortCriteria[i].m_valid = 0;
    }
}

// VERIFY: cb1e4eed manual
// OFFSET: 0x0059A5E0
void CGAuctionHouse::SetBrowseSortCriteria(int32_t column, bool reverse) {
    uint32_t index = 0;

    while (CGAuctionHouse::m_listSortCriteria[index].m_column != column && CGAuctionHouse::m_listSortCriteria[index].m_valid) {
        index++;

        if (index >= 12) {
            return;
        }
    }

    for (; index > 0; index--) {
        CGAuctionHouse::m_listSortCriteria[index] = CGAuctionHouse::m_listSortCriteria[index - 1];
    }

    CGAuctionHouse::m_listSortCriteria[0].m_column = column;
    CGAuctionHouse::m_listSortCriteria[0].m_reverse = reverse;
    CGAuctionHouse::m_listSortCriteria[0].m_valid = 1;
}

// VERIFY: c8c4b7b4 auto
// OFFSET: 0x0059A660
void CGAuctionHouse::ClearOwnedSortCriteria() {
    for (uint32_t i = 0; i < 12; i++) {
        CGAuctionHouse::m_ownedSortCriteria[i].m_valid = 0;
    }
}

// VERIFY: 33383a3f manual
// OFFSET: 0x0059A690
void CGAuctionHouse::SetOwnedSortCriteria(int32_t column, bool reverse) {
    uint32_t index = 0;

    while (CGAuctionHouse::m_ownedSortCriteria[index].m_column != column && CGAuctionHouse::m_ownedSortCriteria[index].m_valid) {
        index++;

        if (index >= 12) {
            return;
        }
    }

    for (; index > 0; index--) {
        CGAuctionHouse::m_ownedSortCriteria[index] = CGAuctionHouse::m_ownedSortCriteria[index - 1];
    }

    CGAuctionHouse::m_ownedSortCriteria[0].m_column = column;
    CGAuctionHouse::m_ownedSortCriteria[0].m_reverse = reverse;
    CGAuctionHouse::m_ownedSortCriteria[0].m_valid = 1;
}

// VERIFY: 7719865d auto
// OFFSET: 0x0059A710
void CGAuctionHouse::ClearBidSortCriteria() {
    for (uint32_t i = 0; i < 12; i++) {
        CGAuctionHouse::m_bidSortCriteria[i].m_valid = 0;
    }
}

// VERIFY: dcb842e3 manual
// OFFSET: 0x0059A740
void CGAuctionHouse::SetBidSortCriteria(int32_t column, bool reverse) {
    uint32_t index = 0;

    while (CGAuctionHouse::m_bidSortCriteria[index].m_column != column && CGAuctionHouse::m_bidSortCriteria[index].m_valid) {
        index++;

        if (index >= 12) {
            return;
        }
    }

    for (; index > 0; index--) {
        CGAuctionHouse::m_bidSortCriteria[index] = CGAuctionHouse::m_bidSortCriteria[index - 1];
    }

    CGAuctionHouse::m_bidSortCriteria[0].m_column = column;
    CGAuctionHouse::m_bidSortCriteria[0].m_reverse = reverse;
    CGAuctionHouse::m_bidSortCriteria[0].m_valid = 1;
}

// VERIFY: 2152aa2c auto
// OFFSET: 0x0059A7C0
const char* CGAuctionHouse::SortTypeToName(int32_t column) {
    switch (column) {
        case 0:
            return "level";

        case 1:
            return "quality";

        case 2:
            return "buyoutthenbid";

        case 3:
            return "duration";

        case 4:
            return "status";

        case 5:
            return "name";

        case 6:
            return "minbidbuyout";

        case 7:
            return "seller";

        case 8:
            return "bid";

        case 9:
            return "quantity";

        case 10:
            return "buyout";

        default:
            return nullptr;
    }
}

// VERIFY: 19c49428 auto
// OFFSET: 0x0059A850
bool AuctionSortToColumnIndex(const char* name, int32_t* column) {
    if (!SStrCmpI(name, "quality", STORM_MAX_STR)) {
        *column = 1;
        return true;
    }

    if (!SStrCmpI(name, "level", STORM_MAX_STR)) {
        *column = 0;
        return true;
    }

    if (!SStrCmpI(name, "duration", STORM_MAX_STR)) {
        *column = 3;
        return true;
    }

    if (!SStrCmpI(name, "status", STORM_MAX_STR)) {
        *column = 4;
        return true;
    }

    if (!SStrCmpI(name, "bid", STORM_MAX_STR)) {
        *column = 8;
        return true;
    }

    if (!SStrCmpI(name, "name", STORM_MAX_STR)) {
        *column = 5;
        return true;
    }

    if (!SStrCmpI(name, "minbidbuyout", STORM_MAX_STR)) {
        *column = 6;
        return true;
    }

    if (!SStrCmpI(name, "seller", STORM_MAX_STR)) {
        *column = 7;
        return true;
    }

    if (!SStrCmpI(name, "buyoutthenbid", STORM_MAX_STR)) {
        *column = 2;
        return true;
    }

    if (!SStrCmpI(name, "quantity", STORM_MAX_STR)) {
        *column = 9;
        return true;
    }

    if (!SStrCmpI(name, "buyout", STORM_MAX_STR)) {
        *column = 10;
        return true;
    }

    return false;
}

// VERIFY: f11c9771 auto
// OFFSET: 0x0059A9D0
bool CGAuctionHouse::TableNameToType(const char* name, int32_t* type) {
    if (!SStrCmpI(name, "list", STORM_MAX_STR)) {
        *type = 0;
        return true;
    }

    if (!SStrCmpI(name, "owner", STORM_MAX_STR)) {
        *type = 2;
        return true;
    }

    if (!SStrCmpI(name, "bidder", STORM_MAX_STR)) {
        *type = 1;
        return true;
    }

    return false;
}

// VERIFY: 7e97e9c2 manual
// OFFSET: 0x0059AA50
bool CGAuctionHouse::IsItemForSell(WOWGUID item) {
    if (!CGAuctionHouse::m_havePendingSell) {
        return false;
    }

    for (uint32_t i = 0; i < CGAuctionHouse::m_numSellItems; i++) {
        if (CGAuctionHouse::m_sellItems[i].m_guid == item) {
            return true;
        }
    }

    return false;
}

// VERIFY: 46ea6541 auto
// OFFSET: 0x0059AAA0
void CGAuctionHouse::LockAllSellItems() {
    for (uint32_t i = 0; i < CGAuctionHouse::m_numSellItems; i++) {
        CGGameUI::LockItem(CGAuctionHouse::m_sellItems[i].m_guid);
    }
}

// VERIFY: 97e0b16f auto
// OFFSET: 0x0059AAE0
void CGAuctionHouse::UnlockAllSellItems() {
    for (uint32_t i = 0; i < CGAuctionHouse::m_numSellItems; i++) {
        CGGameUI::UnlockItem(CGAuctionHouse::m_sellItems[i].m_guid);
    }
}

// VERIFY: 675ef94f manual
// OFFSET: 0x0059AB20
void CGAuctionHouse::OnMultisellFailure(bool silent) {
    CGAuctionHouse::m_havePendingSell = 0;
    CGAuctionHouse::UnlockAllSellItems();
    CGAuctionHouse::ClearSellItem();

    if (!silent) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_MULTISELL_FAILURE, nullptr);
    }
}

// VERIFY: 48084941 manual
// OFFSET: 0x0059B2E0
void CGAuctionHouse::SetSelectedBrowseItem(uint32_t index) {
    if (index >= CGAuctionHouse::m_browseList.Count()) {
        CGAuctionHouse::m_selectedBrowseItem = -1;
    } else {
        CGAuctionHouse::m_selectedBrowseItem = CGAuctionHouse::m_browseList.m_data[index].m_auctionID;
    }
}

// VERIFY: 12498d60 manual
// OFFSET: 0x0059B320
void CGAuctionHouse::SetSelectedOwnedItem(uint32_t index) {
    if (index >= CGAuctionHouse::m_ownerList.Count()) {
        CGAuctionHouse::m_selectedOwnedItem = -1;
    } else {
        CGAuctionHouse::m_selectedOwnedItem = CGAuctionHouse::m_ownerList.m_data[index].m_auctionID;
    }
}

// VERIFY: 34a62e17 manual
// OFFSET: 0x0059B360
void CGAuctionHouse::SetSelectedBidItem(uint32_t index) {
    if (index >= CGAuctionHouse::m_bidderList.Count()) {
        CGAuctionHouse::m_selectedBidItem = -1;
    } else {
        CGAuctionHouse::m_selectedBidItem = CGAuctionHouse::m_bidderList.m_data[index].m_auctionID;
    }
}

// VERIFY: a196d1d9 manual
// OFFSET: 0x0059B3A0
int32_t CGAuctionHouse::GetSelectedBrowseIndex() {
    if (CGAuctionHouse::m_selectedBrowseItem < 0) {
        return -1;
    }

    for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
        if (CGAuctionHouse::m_browseList.m_data[i].m_auctionID == static_cast<uint32_t>(CGAuctionHouse::m_selectedBrowseItem)) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 8ecf791e manual
// OFFSET: 0x0059B3E0
int32_t CGAuctionHouse::GetSelectedOwnedIndex() {
    if (CGAuctionHouse::m_selectedOwnedItem < 0) {
        return -1;
    }

    for (uint32_t i = 0; i < CGAuctionHouse::m_ownerList.Count(); i++) {
        if (CGAuctionHouse::m_ownerList.m_data[i].m_auctionID == static_cast<uint32_t>(CGAuctionHouse::m_selectedOwnedItem)) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 6c009016 manual
// OFFSET: 0x0059B420
int32_t CGAuctionHouse::GetSelectedBidIndex() {
    if (CGAuctionHouse::m_selectedBidItem < 0) {
        return -1;
    }

    for (uint32_t i = 0; i < CGAuctionHouse::m_bidderList.Count(); i++) {
        if (CGAuctionHouse::m_bidderList.m_data[i].m_auctionID == static_cast<uint32_t>(CGAuctionHouse::m_selectedBidItem)) {
            return i;
        }
    }

    return -1;
}

// VERIFY: cad54c03 manual
// OFFSET: 0x0059B460
static int32_t CheckSortCriteria(int32_t column, const AuctionItemInfo* a, const AuctionItemInfo* b) {
    switch (column) {
        case 0: {
            WOWGUID noneA = WOWGUID();
            ItemStats_C* statsA = g_itemDBCache.GetRecord(a->m_itemID, noneA, nullptr, nullptr, false);
            WOWGUID noneB = WOWGUID();
            ItemStats_C* statsB = g_itemDBCache.GetRecord(b->m_itemID, noneB, nullptr, nullptr, false);

            if (!statsA || !statsB) {
                return 0;
            }

            int32_t levelA = static_cast<int32_t>(statsA->m_requiredLevel);

            if (levelA < 1) {
                levelA = 1;
            }

            int32_t levelB = static_cast<int32_t>(statsB->m_requiredLevel);

            if (levelB < 1) {
                levelB = 1;
            }

            if (levelA == levelB) {
                return 0;
            }

            return levelA >= levelB ? 1 : -1;
        }

        case 1: {
            WOWGUID noneA = WOWGUID();
            ItemStats_C* statsA = g_itemDBCache.GetRecord(a->m_itemID, noneA, nullptr, nullptr, false);
            WOWGUID noneB = WOWGUID();
            ItemStats_C* statsB = g_itemDBCache.GetRecord(b->m_itemID, noneB, nullptr, nullptr, false);

            if (!statsA || !statsB || statsA->m_quality == statsB->m_quality) {
                return 0;
            }

            return statsA->m_quality < statsB->m_quality ? 1 : -1;
        }

        case 2: {
            uint32_t bidA = a->m_bid ? a->m_bid : a->m_minBid;
            uint32_t bidB = b->m_bid ? b->m_bid : b->m_minBid;
            uint32_t buyoutA = a->m_buyout ? a->m_buyout : bidA;
            uint32_t buyoutB = b->m_buyout ? b->m_buyout : bidB;

            if (buyoutA != buyoutB) {
                return buyoutA >= buyoutB ? 1 : -1;
            }

            if (bidA == bidB) {
                return 0;
            }

            return bidA >= bidB ? 1 : -1;
        }

        case 3: {
            if (a->m_sold != b->m_sold) {
                return a->m_sold <= b->m_sold ? 1 : -1;
            }

            if (a->m_expireTime == b->m_expireTime) {
                return 0;
            }

            return a->m_expireTime >= b->m_expireTime ? 1 : -1;
        }

        case 4: {
            int32_t statusA;

            if (a->m_bidder == ClntObjMgrGetActivePlayer()) {
                statusA = 2;
            } else if (a->m_bidder) {
                statusA = 1;
            } else {
                statusA = 0;
            }

            int32_t statusB = 0;

            if (b->m_bidder == ClntObjMgrGetActivePlayer()) {
                statusB = 2;
            } else if (b->m_bidder) {
                statusB = 1;
            }

            if (statusA == statusB) {
                return 0;
            }

            return statusA < statusB ? 1 : -1;
        }

        case 5: {
            WOWGUID noneA = WOWGUID();
            ItemStats_C* statsA = g_itemDBCache.GetRecord(a->m_itemID, noneA, nullptr, nullptr, false);
            WOWGUID noneB = WOWGUID();
            ItemStats_C* statsB = g_itemDBCache.GetRecord(b->m_itemID, noneB, nullptr, nullptr, false);

            if (!statsA || !statsB) {
                return 0;
            }

            return SStrSortUTF8I(statsA->GetName(0), statsB->GetName(0), STORM_MAX_STR);
        }

        case 6: {
            uint32_t bidA = a->m_bid ? a->m_bid : a->m_minBid;
            uint32_t bidB = b->m_bid ? b->m_bid : b->m_minBid;
            uint32_t buyoutA = a->m_buyout ? a->m_buyout : bidA;
            uint32_t buyoutB = b->m_buyout ? b->m_buyout : bidB;

            if (buyoutA == buyoutB) {
                return 0;
            }

            return buyoutA >= buyoutB ? 1 : -1;
        }

        case 7: {
            WOWGUID noneA = WOWGUID();
            NameCache* nameA = g_nameDBCache.GetRecord(a->m_owner.m_guid, noneA, nullptr, nullptr, false);
            WOWGUID noneB = WOWGUID();
            NameCache* nameB = g_nameDBCache.GetRecord(b->m_owner.m_guid, noneB, nullptr, nullptr, false);

            if (nameA && nameB) {
                return SStrCmp(nameA->m_name, nameB->m_name, STORM_MAX_STR);
            }

            if (a->m_owner.m_guid == b->m_owner.m_guid) {
                return 0;
            }

            return a->m_owner.m_guid < b->m_owner.m_guid ? -1 : 1;
        }

        case 8: {
            uint32_t bidA = a->m_bid ? a->m_bid : a->m_minBid;
            uint32_t bidB = b->m_bid ? b->m_bid : b->m_minBid;

            if (bidA == bidB) {
                return 0;
            }

            return bidA >= bidB ? 1 : -1;
        }

        case 9: {
            if (a->m_count == b->m_count) {
                return 0;
            }

            return a->m_count >= b->m_count ? 1 : -1;
        }

        case 10: {
            if (a->m_buyout == b->m_buyout) {
                return 0;
            }

            return a->m_buyout >= b->m_buyout ? 1 : -1;
        }

        default:
            return 0;
    }
}

// VERIFY: a6bde7a8 manual
// OFFSET: 0x0059B890
int32_t AuctionListComparer(const void* a, const void* b) {
    for (uint32_t i = 0; i < 12; i++) {
        int32_t result = CheckSortCriteria(CGAuctionHouse::m_listSortCriteria[i].m_column, static_cast<const AuctionItemInfo*>(a), static_cast<const AuctionItemInfo*>(b));

        if (result) {
            return CGAuctionHouse::m_listSortCriteria[i].m_reverse ? -result : result;
        }
    }

    return 0;
}

// VERIFY: 8d1b1c27 manual
// OFFSET: 0x0059B8E0
int32_t CompareFunction(const void* a, const void* b) {
    for (uint32_t i = 0; i < 12; i++) {
        int32_t result = CheckSortCriteria(CGAuctionHouse::m_ownedSortCriteria[i].m_column, static_cast<const AuctionItemInfo*>(a), static_cast<const AuctionItemInfo*>(b));

        if (result) {
            return CGAuctionHouse::m_ownedSortCriteria[i].m_reverse ? -result : result;
        }
    }

    return 0;
}

// VERIFY: 7f1b65b0 manual
// OFFSET: 0x0059B930
int32_t AuctionBidderListComparer(const void* a, const void* b) {
    for (uint32_t i = 0; i < 12; i++) {
        int32_t result = CheckSortCriteria(CGAuctionHouse::m_bidSortCriteria[i].m_column, static_cast<const AuctionItemInfo*>(a), static_cast<const AuctionItemInfo*>(b));

        if (result) {
            return CGAuctionHouse::m_bidSortCriteria[i].m_reverse ? -result : result;
        }
    }

    return 0;
}

// VERIFY: 27279a9d auto
// OFFSET: 0x0059B980
void CGAuctionHouse::ApplyBrowseSort() {
    qsort(CGAuctionHouse::m_browseList.m_data, CGAuctionHouse::m_browseList.Count(), sizeof(AuctionItemInfo), AuctionListComparer);
    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
}

// VERIFY: bf9a3158 auto
// OFFSET: 0x0059B9E0
void CGAuctionHouse::ApplyBidderSort() {
    qsort(CGAuctionHouse::m_bidderList.m_data, CGAuctionHouse::m_bidderList.Count(), sizeof(AuctionItemInfo), AuctionBidderListComparer);
    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_BIDDER_LIST_UPDATE, nullptr);
}

// VERIFY: 3f8a8a99 manual
// OFFSET: 0x0059BA10
void CGAuctionHouse::ClearOutbidID(int32_t auctionID) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(CGAuctionHouse::m_numOutbidItems); i++) {
        if (CGAuctionHouse::m_outbidItems.m_data[i]->m_auctionID == auctionID) {
            CGAuctionHouse::m_numOutbidItems--;

            for (; i < static_cast<uint32_t>(CGAuctionHouse::m_numOutbidItems); i++) {
                *CGAuctionHouse::m_outbidItems.m_data[i] = *CGAuctionHouse::m_outbidItems.m_data[i + 1];
            }

            return;
        }
    }
}

// VERIFY: cd43ef56 manual
// OFFSET: 0x0059BA80
void CGAuctionHouse::SellNextItem() {
    if (!CGAuctionHouse::m_multisellRemaining) {
        CGAuctionHouse::m_havePendingSell = 0;
        CGAuctionHouse::UnlockAllSellItems();
        CGAuctionHouse::ClearSellItem();
        return;
    }

    CGAuctionHouse::m_multisellRemaining--;

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUCTION_SELL_ITEM));
    msg.Put(CGAuctionHouse::m_multisellAuctioneer.m_guid);

    uint32_t remaining = CGAuctionHouse::m_multisellStackSize;
    int32_t last = CGAuctionHouse::m_numSellItems - 1;

    while (remaining > CGAuctionHouse::m_sellItems[last].m_count) {
        remaining -= CGAuctionHouse::m_sellItems[last].m_count;
        last--;

        if (last < 0) {
            CGAuctionHouse::OnMultisellFailure(false);
            return;
        }
    }

    msg.Put(static_cast<uint32_t>(CGAuctionHouse::m_numSellItems - last));

    for (int32_t i = CGAuctionHouse::m_numSellItems - 1; i > last; i--) {
        msg.Put(CGAuctionHouse::m_sellItems[i].m_guid.m_guid);
        msg.Put(CGAuctionHouse::m_sellItems[i].m_count);
    }

    msg.Put(CGAuctionHouse::m_sellItems[last].m_guid.m_guid);
    msg.Put(remaining);

    bool emptied = CGAuctionHouse::m_sellItems[last].m_count == remaining;
    CGAuctionHouse::m_sellItems[last].m_count -= remaining;

    if (!emptied) {
        last++;
    }

    CGAuctionHouse::m_numSellItems = last;

    msg.Put(CGAuctionHouse::m_multisellBid);
    msg.Put(CGAuctionHouse::m_multisellBuyout);
    msg.Put(CGAuctionHouse::m_multisellDuration);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 39c2f087 auto
// OFFSET: 0x0059CB70
void AuctionListItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGAuctionHouse::m_pendingBrowseItems) {
        CGAuctionHouse::m_pendingBrowseItems--;

        if (CGAuctionHouse::m_pendingBrowseItems) {
            return;
        }
    }

    CGAuctionHouse::ApplyBrowseSort();
}

// VERIFY: 9033446b auto
// OFFSET: 0x0059CBB0
void AuctionOwnedItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGAuctionHouse::m_pendingOwnedItems) {
        CGAuctionHouse::m_pendingOwnedItems--;

        if (CGAuctionHouse::m_pendingOwnedItems) {
            return;
        }
    }

    if (!CGAuctionHouse::m_pendingSoldItems && CGAuctionHouse::m_queryPending[2] <= 0) {
        qsort(CGAuctionHouse::m_ownerList.m_data, CGAuctionHouse::m_ownerList.Count(), sizeof(AuctionItemInfo), CompareFunction);
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
    }
}

// VERIFY: f2909fa8 auto
// OFFSET: 0x0059CBE0
void AuctionOwnedSoldItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGAuctionHouse::m_pendingSoldItems) {
        CGAuctionHouse::m_pendingSoldItems--;
    }

    if (!CGAuctionHouse::m_pendingOwnedItems && !CGAuctionHouse::m_pendingSoldItems && CGAuctionHouse::m_queryPending[2] <= 0) {
        qsort(CGAuctionHouse::m_ownerList.m_data, CGAuctionHouse::m_ownerList.Count(), sizeof(AuctionItemInfo), CompareFunction);
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
    }
}

// VERIFY: 857effd2 auto
// OFFSET: 0x0059CC10
void AuctionBidItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGAuctionHouse::m_pendingBidItems) {
        CGAuctionHouse::m_pendingBidItems--;

        if (CGAuctionHouse::m_pendingBidItems) {
            return;
        }
    }

    CGAuctionHouse::ApplyBidderSort();
}

// VERIFY: 45fce788 manual
// OFFSET: 0x0059CC50
void CGAuctionHouse::GetOwnerList() {
    if (!CGAuctionHouse::m_canGetOwnerList || !CGAuctionHouse::CanSendQuery(2)) {
        return;
    }

    CDataStore ownerMsg;
    ownerMsg.Put(static_cast<uint32_t>(CMSG_AUCTION_LIST_OWNER_ITEMS));
    ownerMsg.Put(CGAuctionHouse::m_auctioneer.m_guid);
    ownerMsg.Put(static_cast<uint32_t>(0));
    ownerMsg.Finalize();
    ClientServices::Send2(&ownerMsg);

    CDataStore salesMsg;
    salesMsg.Put(static_cast<uint32_t>(CMSG_AUCTION_LIST_PENDING_SALES));
    salesMsg.Put(CGAuctionHouse::m_auctioneer.m_guid);
    salesMsg.Finalize();
    ClientServices::Send2(&salesMsg);

    CGAuctionHouse::m_queryPending[2] += 2;

    if (CGAuctionHouse::m_queryPending[2] < 0) {
        CGAuctionHouse::m_queryPending[2] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[2] = OsGetAsyncTimeMs();
    CGAuctionHouse::m_canGetOwnerList = 0;
}

// VERIFY: 8157fca2 manual
// OFFSET: 0x0059CD90
void RequestListPendingSales() {
    if (!CGAuctionHouse::CanSendQuery(2)) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUCTION_LIST_PENDING_SALES));
    msg.Put(CGAuctionHouse::m_auctioneer.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (++CGAuctionHouse::m_queryPending[2] < 0) {
        CGAuctionHouse::m_queryPending[2] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[2] = OsGetAsyncTimeMs();
}

// VERIFY: 07dc7173 manual
// OFFSET: 0x0059CE50
void BidderListItems(int32_t page) {
    if (!CGAuctionHouse::m_canGetBidderList) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUCTION_LIST_BIDDER_ITEMS));
    msg.Put(CGAuctionHouse::m_auctioneer.m_guid);
    msg.Put(static_cast<uint32_t>(page));

    uint32_t countPos = msg.m_size;
    msg.Put(static_cast<uint32_t>(0));

    uint32_t count = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(CGAuctionHouse::m_numOutbidItems); i++) {
        if (CGAuctionHouse::m_outbidItems.m_data[i]->m_auctionHouseID == CGAuctionHouse::m_auctionHouseID) {
            msg.Put(static_cast<uint32_t>(CGAuctionHouse::m_outbidItems.m_data[i]->m_auctionID));
            count++;
        }
    }

    msg.Set(countPos, count);
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (++CGAuctionHouse::m_queryPending[1] < 0) {
        CGAuctionHouse::m_queryPending[1] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[1] = OsGetAsyncTimeMs();
    CGAuctionHouse::m_canGetBidderList = 0;
    CGAuctionHouse::m_lastBidderPage = page;
}

// VERIFY: 997708e3 auto
// OFFSET: 0x0059CF90
void CGAuctionHouse::SortBrowseList(int32_t column) {
    if (CGAuctionHouse::m_listSortCriteria[0].m_column == column && CGAuctionHouse::m_listSortCriteria[0].m_valid) {
        CGAuctionHouse::SetBrowseSortCriteria(column, !CGAuctionHouse::m_listSortCriteria[0].m_reverse);
    } else {
        CGAuctionHouse::SetBrowseSortCriteria(column, false);
    }

    CGAuctionHouse::ApplyBrowseSort();
}

// VERIFY: 6aacb4e0 auto
// OFFSET: 0x0059CFF0
void CGAuctionHouse::SortOwnedList(int32_t column) {
    if (CGAuctionHouse::m_ownedSortCriteria[0].m_column == column && CGAuctionHouse::m_ownedSortCriteria[0].m_valid) {
        CGAuctionHouse::SetOwnedSortCriteria(column, !CGAuctionHouse::m_ownedSortCriteria[0].m_reverse);
    } else {
        CGAuctionHouse::SetOwnedSortCriteria(column, false);
    }

    qsort(CGAuctionHouse::m_ownerList.m_data, CGAuctionHouse::m_ownerList.Count(), sizeof(AuctionItemInfo), CompareFunction);
    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
}

// VERIFY: 4d902f57 auto
// OFFSET: 0x0059D050
void CGAuctionHouse::SortBidderList(int32_t column) {
    if (CGAuctionHouse::m_bidSortCriteria[0].m_column == column && CGAuctionHouse::m_bidSortCriteria[0].m_valid) {
        CGAuctionHouse::SetBidSortCriteria(column, !CGAuctionHouse::m_bidSortCriteria[0].m_reverse);
    } else {
        CGAuctionHouse::SetBidSortCriteria(column, false);
    }

    CGAuctionHouse::ApplyBidderSort();
}

// VERIFY: a9ee9c34 manual
// OFFSET: 0x0059D0B0
bool CGAuctionHouse::IsSellItemValid(CGItem_C* item, int32_t showError) {
    UIErrors error;
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        error = ERR_ITEM_NOT_FOUND;
    } else if (item->GetMaxDurability() && item->GetDurability() < item->GetMaxDurability()) {
        error = ERR_AUCTION_REPAIR_ITEM;
    } else if (stats->m_spellId[0] && item->m_item->m_itemFieldSpellCharges[0] != stats->m_spellCharges[0]) {
        error = ERR_AUCTION_USED_CHARGES;
    } else if (item->IsBound()) {
        error = stats->m_bonding != 4 ? ERR_AUCTION_BOUND_ITEM : ERR_AUCTION_QUEST_ITEM;
    } else if (item->GetItemStaticFlags(0) & ITEM_FLAG_CONJURED) {
        error = ERR_AUCTION_CONJURED_ITEM;
    } else if (((item->GetItemStaticFlags(0) & 0x10000) && static_cast<int32_t>(stats->m_duration) > 0) || stats->m_holidayId) {
        error = ERR_AUCTION_LIMITED_DURATION_ITEM;
    } else if (item->m_item->m_itemFieldDuration) {
        error = ERR_AUCTION_LIMITED_DURATION_ITEM;
    } else if (item->IsZoneBound() || item->IsInstanceBound()) {
        error = ERR_AUCTION_CONJURED_ITEM;
    } else {
        const CGBag_C* bag;

        if ((item->m_obj->m_type & TYPE_CONTAINER) && (bag = static_cast<const CGItem_C*>(item)->GetBag()) && bag->GetNumItems()) {
            error = ERR_AUCTION_BAG;
        } else if (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) {
            error = ERR_AUCTION_WRAPPED_ITEM;
        } else if (item->GetItemStaticFlags(0) & ITEM_FLAG_HAS_LOOT) {
            error = ERR_AUCTION_LOOT_ITEM;
        } else {
            return true;
        }
    }

    if (showError) {
        CGGameUI::DisplayError(error);
    }

    return false;
}

// VERIFY: d1664376 manual
// OFFSET: 0x0059E160
int32_t Packet_SMSG_AUCTION_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (--CGAuctionHouse::m_queryPending[0] < 0) {
        CGAuctionHouse::m_queryPending[0] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[0] = OsGetAsyncTimeMs();

    if (CGAuctionHouse::m_pendingBrowseItems) {
        CGAuctionHouse::m_pendingBrowseItems = 0;

        for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
            g_itemDBCache.CancelCallback(CGAuctionHouse::m_browseList.m_data[i].m_itemID, AuctionListItemQueryCallback, nullptr);
        }
    }

    uint32_t count;
    msg->Get(count);
    CGAuctionHouse::m_browseList.SetCount(count);

    uint32_t now = OsGetAsyncTimeMs();

    for (uint32_t i = 0; i < count; i++) {
        AuctionItemInfo* auction = &CGAuctionHouse::m_browseList.m_data[i];
        auction->m_auctionHouseID = CGAuctionHouse::m_auctionHouseID;
        msg->Get(auction->m_auctionID);
        msg->Get(auction->m_itemID);

        for (uint32_t j = 0; j < 7; j++) {
            msg->Get(auction->m_enchantID[j]);
            msg->Get(auction->m_enchantDuration[j]);
            msg->Get(auction->m_enchantCharges[j]);
        }

        msg->Get(auction->m_randomPropertyID);
        msg->Get(auction->m_suffixFactor);
        msg->Get(auction->m_count);
        msg->Get(auction->m_spellCharges);
        msg->Get(auction->m_flags);
        msg->Get(auction->m_owner.m_guid);
        msg->Get(auction->m_minBid);
        msg->Get(auction->m_minIncrement);
        msg->Get(auction->m_buyout);

        uint32_t timeLeft = 0;
        msg->Get(timeLeft);
        auction->m_expireTime = now + timeLeft;

        msg->Get(auction->m_bidder.m_guid);
        msg->Get(auction->m_bid);
        auction->m_sold = 0;

        WOWGUID none = WOWGUID();

        if (!g_itemDBCache.GetRecord(auction->m_itemID, none, AuctionListItemQueryCallback, nullptr, false)) {
            CGAuctionHouse::m_pendingBrowseItems++;
        }
    }

    msg->Get(CGAuctionHouse::m_numTotalItems);
    msg->Get(reinterpret_cast<uint32_t&>(CGAuctionHouse::m_desiredDelayTime));

    if (!CGAuctionHouse::m_pendingBrowseItems) {
        CGAuctionHouse::ApplyBrowseSort();
    }

    return 1;
}

// VERIFY: 143f6b55 manual
// OFFSET: 0x0059E480
int32_t Packet_SMSG_AUCTION_OWNER_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (--CGAuctionHouse::m_queryPending[2] < 0) {
        CGAuctionHouse::m_queryPending[2] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[2] = OsGetAsyncTimeMs();

    uint32_t kept = 0;

    for (uint32_t i = 0; i < CGAuctionHouse::m_ownerList.Count(); i++) {
        AuctionItemInfo* auction = &CGAuctionHouse::m_ownerList.m_data[i];

        if (!auction->m_sold) {
            if (CGAuctionHouse::m_pendingOwnedItems) {
                g_itemDBCache.CancelCallback(auction->m_itemID, AuctionOwnedItemQueryCallback, nullptr);
            }
        } else {
            if (i != kept) {
                memcpy(&CGAuctionHouse::m_ownerList.m_data[kept], auction, sizeof(AuctionItemInfo));
            }

            kept++;
        }
    }

    CGAuctionHouse::m_ownerList.SetCount(kept);
    CGAuctionHouse::m_pendingOwnedItems = 0;

    uint32_t count;
    msg->Get(count);

    if (CGAuctionHouse::m_ownerList.Count() + count > CGAuctionHouse::m_ownerList.m_alloc) {
        CGAuctionHouse::m_ownerList.ReallocData(CGAuctionHouse::m_ownerList.Count() + count);
    }

    uint32_t now = OsGetAsyncTimeMs();

    for (uint32_t i = 0; i < count; i++) {
        uint32_t index = kept + i;
        CGAuctionHouse::m_ownerList.SetCount(index + 1);

        AuctionItemInfo* auction = &CGAuctionHouse::m_ownerList.m_data[index];
        auction->m_auctionHouseID = CGAuctionHouse::m_auctionHouseID;
        msg->Get(auction->m_auctionID);
        msg->Get(auction->m_itemID);

        for (uint32_t j = 0; j < 7; j++) {
            msg->Get(auction->m_enchantID[j]);
            msg->Get(auction->m_enchantDuration[j]);
            msg->Get(auction->m_enchantCharges[j]);
        }

        msg->Get(auction->m_randomPropertyID);
        msg->Get(auction->m_suffixFactor);
        msg->Get(auction->m_count);
        msg->Get(auction->m_spellCharges);
        msg->Get(auction->m_flags);
        msg->Get(auction->m_owner.m_guid);
        msg->Get(auction->m_minBid);
        msg->Get(auction->m_minIncrement);
        msg->Get(auction->m_buyout);

        uint32_t timeLeft = 0;
        msg->Get(timeLeft);
        auction->m_expireTime = now + timeLeft;

        msg->Get(auction->m_bidder.m_guid);
        msg->Get(auction->m_bid);
        auction->m_sold = 0;

        WOWGUID none = WOWGUID();

        if (!g_itemDBCache.GetRecord(auction->m_itemID, none, AuctionOwnedItemQueryCallback, nullptr, false)) {
            CGAuctionHouse::m_pendingOwnedItems++;
        }
    }

    msg->Get(CGAuctionHouse::m_numTotalOwned);
    msg->Get(reinterpret_cast<uint32_t&>(CGAuctionHouse::m_desiredDelayTime));

    if (!CGAuctionHouse::m_pendingOwnedItems && !CGAuctionHouse::m_pendingSoldItems && CGAuctionHouse::m_queryPending[2] <= 0) {
        qsort(CGAuctionHouse::m_ownerList.m_data, CGAuctionHouse::m_ownerList.Count(), sizeof(AuctionItemInfo), CompareFunction);
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: 2b014930 manual
// OFFSET: 0x0059E880
int32_t Packet_SMSG_AUCTION_LIST_PENDING_SALES(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (--CGAuctionHouse::m_queryPending[2] < 0) {
        CGAuctionHouse::m_queryPending[2] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[2] = OsGetAsyncTimeMs();

    uint32_t kept = 0;

    for (uint32_t i = 0; i < CGAuctionHouse::m_ownerList.Count(); i++) {
        AuctionItemInfo* auction = &CGAuctionHouse::m_ownerList.m_data[i];

        if (auction->m_sold == 1) {
            if (CGAuctionHouse::m_pendingSoldItems) {
                g_itemDBCache.CancelCallback(auction->m_itemID, AuctionOwnedSoldItemQueryCallback, nullptr);
            }
        } else {
            if (i != kept) {
                memcpy(&CGAuctionHouse::m_ownerList.m_data[kept], auction, sizeof(AuctionItemInfo));
            }

            kept++;
        }
    }

    CGAuctionHouse::m_ownerList.SetCount(kept);
    CGAuctionHouse::m_pendingSoldItems = 0;

    uint32_t count;
    msg->Get(count);

    if (CGAuctionHouse::m_ownerList.Count() + count > CGAuctionHouse::m_ownerList.m_alloc) {
        CGAuctionHouse::m_ownerList.ReallocData(CGAuctionHouse::m_ownerList.Count() + count);
    }

    uint32_t now = OsGetAsyncTimeMs();
    uint32_t index = kept;

    for (uint32_t i = 0; i < count; i++) {
        char subject[256];
        char body[256];
        uint32_t mailId;
        uint32_t mailTemplate;
        float daysLeft;
        msg->GetString(subject, sizeof(subject));
        msg->GetString(body, sizeof(body));
        msg->Get(mailId);
        msg->Get(mailTemplate);
        msg->Get(daysLeft);

        uint32_t itemId;
        uint32_t randomPropertyId;
        uint32_t action;
        uint32_t auctionId;

        if (ParseAuctionMailSubject(subject, &itemId, &randomPropertyId, &action, &auctionId) != 4 || action != 2) {
            continue;
        }

        uint64_t buyer = 0;
        int32_t bid = 0;
        int32_t buyout = 0;
        int32_t deposit = 0;
        int32_t consignment = 0;
        sscanf(body, "%16I64X:%d:%d:%d:%d", &buyer, &bid, &buyout, &deposit, &consignment);

        CGAuctionHouse::m_ownerList.SetCount(index + 1);

        AuctionItemInfo* auction = &CGAuctionHouse::m_ownerList.m_data[index];
        auction->m_auctionHouseID = CGAuctionHouse::m_auctionHouseID;
        auction->m_auctionID = auctionId;
        auction->m_itemID = itemId;

        for (uint32_t j = 0; j < 7; j++) {
            auction->m_enchantID[j] = 0;
            auction->m_enchantDuration[j] = 0;
            auction->m_enchantCharges[j] = 0;
        }

        auction->m_randomPropertyID = randomPropertyId;
        auction->m_suffixFactor = 0;
        auction->m_count = 0;
        auction->m_spellCharges = 0;
        auction->m_flags = 0;
        auction->m_owner = ClntObjMgrGetActivePlayer();
        auction->m_minBid = 0;
        auction->m_minIncrement = 0;
        auction->m_buyout = buyout;
        auction->m_expireTime = now - static_cast<int64_t>(daysLeft * -86400000.0);
        auction->m_bidder.m_guid = buyer;
        auction->m_bid = bid;
        auction->m_sold = 1;

        WOWGUID none = WOWGUID();

        if (!g_itemDBCache.GetRecord(auction->m_itemID, none, AuctionOwnedSoldItemQueryCallback, nullptr, false)) {
            CGAuctionHouse::m_pendingSoldItems++;
        }

        index++;
    }

    if (!CGAuctionHouse::m_pendingOwnedItems && !CGAuctionHouse::m_pendingSoldItems && CGAuctionHouse::m_queryPending[2] <= 0) {
        qsort(CGAuctionHouse::m_ownerList.m_data, CGAuctionHouse::m_ownerList.Count(), sizeof(AuctionItemInfo), CompareFunction);
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: 6dc0db61 manual
// OFFSET: 0x0059ECD0
int32_t Packet_SMSG_AUCTION_BIDDER_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (--CGAuctionHouse::m_queryPending[1] < 0) {
        CGAuctionHouse::m_queryPending[1] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[1] = OsGetAsyncTimeMs();

    if (CGAuctionHouse::m_pendingBidItems) {
        CGAuctionHouse::m_pendingBidItems = 0;

        for (uint32_t i = 0; i < CGAuctionHouse::m_bidderList.Count(); i++) {
            g_itemDBCache.CancelCallback(CGAuctionHouse::m_bidderList.m_data[i].m_itemID, AuctionBidItemQueryCallback, nullptr);
        }
    }

    uint32_t count;
    msg->Get(count);
    CGAuctionHouse::m_bidderList.SetCount(count);

    uint32_t now = OsGetAsyncTimeMs();

    for (uint32_t i = 0; i < count; i++) {
        AuctionItemInfo* auction = &CGAuctionHouse::m_bidderList.m_data[i];
        auction->m_auctionHouseID = CGAuctionHouse::m_auctionHouseID;
        msg->Get(auction->m_auctionID);
        msg->Get(auction->m_itemID);

        for (uint32_t j = 0; j < 7; j++) {
            msg->Get(auction->m_enchantID[j]);
            msg->Get(auction->m_enchantDuration[j]);
            msg->Get(auction->m_enchantCharges[j]);
        }

        msg->Get(auction->m_randomPropertyID);
        msg->Get(auction->m_suffixFactor);
        msg->Get(auction->m_count);
        msg->Get(auction->m_spellCharges);
        msg->Get(auction->m_flags);
        msg->Get(auction->m_owner.m_guid);
        msg->Get(auction->m_minBid);
        msg->Get(auction->m_minIncrement);
        msg->Get(auction->m_buyout);

        uint32_t timeLeft = 0;
        msg->Get(timeLeft);
        auction->m_expireTime = now + timeLeft;

        msg->Get(auction->m_bidder.m_guid);
        msg->Get(auction->m_bid);
        auction->m_sold = 0;

        WOWGUID none = WOWGUID();

        if (!g_itemDBCache.GetRecord(auction->m_itemID, none, AuctionBidItemQueryCallback, nullptr, false)) {
            CGAuctionHouse::m_pendingBidItems++;
        }
    }

    msg->Get(CGAuctionHouse::m_numTotalBid);
    msg->Get(reinterpret_cast<uint32_t&>(CGAuctionHouse::m_desiredDelayTime));

    if (count != CGAuctionHouse::m_numTotalBid) {
        CGAuctionHouse::m_canGetBidderList = 1;
    }

    if (!CGAuctionHouse::m_pendingBidItems) {
        CGAuctionHouse::ApplyBidderSort();
    }

    return 1;
}

// VERIFY: db534163 manual
// OFFSET: 0x0059F000
void CGAuctionHouse::SetAuctioneer(WOWGUID auctioneer, int32_t auctionHouseID) {
    CGGameUI::SetInteractTarget(auctioneer);
    CGAuctionHouse::m_auctionHouseID = auctionHouseID;
    CGAuctionHouse::m_lastQueryTime[0] = 0;
    CGAuctionHouse::m_lastQueryTime[1] = 0;
    CGAuctionHouse::m_lastQueryTime[2] = 0;
    CGAuctionHouse::m_queryPending[0] = 0;
    CGAuctionHouse::m_queryPending[1] = 0;
    CGAuctionHouse::m_queryPending[2] = 0;
    CGAuctionHouse::m_canGetOwnerList = 1;
    CGAuctionHouse::m_canGetBidderList = 1;
    CGAuctionHouse::m_auctioneer = auctioneer;
    CGAuctionHouse::m_desiredDelayTime = 5000;
    CGAuctionHouse::m_browseList.Clear();
    CGAuctionHouse::m_numTotalItems = 0;
    CGAuctionHouse::m_ownerList.Clear();
    CGAuctionHouse::m_numTotalOwned = 0;
    CGAuctionHouse::m_bidderList.Clear();
    CGAuctionHouse::m_numTotalBid = 0;
    CGAuctionHouse::m_lastBidderPage = 0;
    CGAuctionHouse::ClearSellItem();
    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_HOUSE_SHOW, nullptr);
}

// VERIFY: eaf143bf manual
// OFFSET: 0x0059F150
void CGAuctionHouse::CloseAuctionHouse() {
    if (!CGAuctionHouse::m_auctioneer) {
        return;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_HOUSE_CLOSED, nullptr);

    if (CGAuctionHouse::m_havePendingSell) {
        CGAuctionHouse::OnMultisellFailure(false);
    }

    CGGameUI::ClearInteractTarget(CGAuctionHouse::m_auctioneer, 0);
    CGAuctionHouse::m_auctioneer = WOWGUID();
    CGAuctionHouse::m_auctionHouseID = 0;
    CGAuctionHouse::m_canGetOwnerList = 0;
    CGAuctionHouse::m_browseList.Clear();
    CGAuctionHouse::m_ownerList.Clear();
    CGAuctionHouse::m_bidderList.Clear();
    CGAuctionHouse::m_canGetBidderList = 0;
    CGAuctionHouse::ClearSellItem();
}

// VERIFY: 3f70f54a manual
// OFFSET: 0x0059F240
void CGAuctionHouse::AddOutbidID(int32_t auctionHouseID, int32_t auctionID) {
    uint32_t capacity = CGAuctionHouse::m_outbidItems.Count();

    if (static_cast<uint32_t>(CGAuctionHouse::m_numOutbidItems) >= capacity) {
        CGAuctionHouse::m_outbidItems.SetCount(2 * capacity);

        for (uint32_t i = capacity; i < 2 * capacity; i++) {
            CGAuctionHouse::m_outbidItems.m_data[i] = static_cast<OutbidItemInfo*>(SMemAlloc(sizeof(OutbidItemInfo), __FILE__, __LINE__, 0));
        }
    }

    CGAuctionHouse::m_outbidItems.m_data[CGAuctionHouse::m_numOutbidItems]->m_auctionHouseID = auctionHouseID;
    CGAuctionHouse::m_outbidItems.m_data[CGAuctionHouse::m_numOutbidItems]->m_auctionID = auctionID;
    CGAuctionHouse::m_numOutbidItems++;
}

// VERIFY: b6d03a91 manual
// OFFSET: 0x0059F2F0
void CGAuctionHouse::AuctionItemExpired(int32_t auctionID) {
    for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
        if (CGAuctionHouse::m_browseList.m_data[i].m_auctionID == static_cast<uint32_t>(auctionID)) {
            if (i < CGAuctionHouse::m_browseList.Count() - 1) {
                memmove(&CGAuctionHouse::m_browseList.m_data[i], &CGAuctionHouse::m_browseList.m_data[i + 1], sizeof(AuctionItemInfo) * (CGAuctionHouse::m_browseList.Count() - i - 1));
            }

            CGAuctionHouse::m_browseList.SetCount(CGAuctionHouse::m_browseList.Count() - 1);
            CGAuctionHouse::m_numTotalItems--;

            if (!CGAuctionHouse::m_pendingBrowseItems) {
                FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
            }

            break;
        }
    }

    for (uint32_t i = 0; i < CGAuctionHouse::m_ownerList.Count(); i++) {
        if (CGAuctionHouse::m_ownerList.m_data[i].m_auctionID == static_cast<uint32_t>(auctionID)) {
            if (i < CGAuctionHouse::m_ownerList.Count() - 1) {
                memmove(&CGAuctionHouse::m_ownerList.m_data[i], &CGAuctionHouse::m_ownerList.m_data[i + 1], sizeof(AuctionItemInfo) * (CGAuctionHouse::m_ownerList.Count() - i - 1));
            }

            CGAuctionHouse::m_ownerList.SetCount(CGAuctionHouse::m_ownerList.Count() - 1);
            CGAuctionHouse::m_numTotalOwned--;
            RequestListPendingSales();
            break;
        }
    }

    for (uint32_t i = 0; i < CGAuctionHouse::m_bidderList.Count(); i++) {
        if (CGAuctionHouse::m_bidderList.m_data[i].m_auctionID == static_cast<uint32_t>(auctionID)) {
            if (i < CGAuctionHouse::m_bidderList.Count() - 1) {
                memmove(&CGAuctionHouse::m_bidderList.m_data[i], &CGAuctionHouse::m_bidderList.m_data[i + 1], sizeof(AuctionItemInfo) * (CGAuctionHouse::m_bidderList.Count() - i - 1));
            }

            CGAuctionHouse::m_bidderList.SetCount(CGAuctionHouse::m_bidderList.Count() - 1);
            CGAuctionHouse::m_numTotalBid--;

            if (!CGAuctionHouse::m_pendingBidItems) {
                FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_BIDDER_LIST_UPDATE, nullptr);
            }

            return;
        }
    }
}

// VERIFY: 033f8959 auto
// OFFSET: 0x0059F5A0
static int32_t FindSellItemsCallback(const CGItem_C* item, void* param) {
    AuctionFindItemsParam* find = static_cast<AuctionFindItemsParam*>(param);

    if (static_cast<int32_t>(item->m_obj->m_entryID) != find->m_itemId || !CGAuctionHouse::IsSellItemValid(const_cast<CGItem_C*>(item), 0)) {
        return 0;
    }

    find->m_total += item->m_item->m_itemFieldStackCount;

    if (find->m_collect) {
        if (CGAuctionHouse::m_numSellItems >= 200) {
            CGAuctionHouse::m_numSellItems = 0;
            return 1;
        }

        CGAuctionHouse::m_sellItems[CGAuctionHouse::m_numSellItems].m_guid = item->m_obj->m_guid;
        CGAuctionHouse::m_sellItems[CGAuctionHouse::m_numSellItems].m_count = item->m_item->m_itemFieldStackCount;
        CGAuctionHouse::m_numSellItems++;
    }

    if (find->m_wanted && find->m_total >= find->m_wanted) {
        return 1;
    }

    return 0;
}

// VERIFY: 26ac0b42 manual
// OFFSET: 0x0059F640
int32_t FindItemForAuction(WOWGUID item, uint32_t count) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    CGItem_C* sellItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (!sellItem) {
        return 0;
    }

    AuctionFindItemsParam find;
    find.m_itemId = sellItem->m_obj->m_entryID;
    find.m_wanted = count;
    find.m_total = 0;
    find.m_collect = true;
    CGAuctionHouse::m_numSellItems = 0;

    WOWGUID container;
    uint32_t slot;
    return player->m_bag.FindItem(FindSellItemsCallback, &find, container, slot, 70) ? CGAuctionHouse::m_numSellItems : 0;
}

// VERIFY: f69c4580 manual
// OFFSET: 0x0059F6E0
int32_t FindSellableItems(int32_t itemId) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    AuctionFindItemsParam find;
    find.m_total = 0;
    find.m_wanted = 0;
    find.m_collect = false;
    find.m_itemId = itemId;

    WOWGUID container;
    uint32_t slot;
    player->m_bag.FindItem(FindSellItemsCallback, &find, container, slot, 70);
    return find.m_total;
}

// VERIFY: 32cfa04f auto
// OFFSET: 0x0059FF40
int32_t Packet_MSG_AUCTION_HELLO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID auctioneer;
    uint32_t auctionHouseID;
    msg->Get(auctioneer.m_guid);
    msg->Get(auctionHouseID);

    uint8_t enabled = 0;
    msg->Get(enabled);

    if (enabled) {
        CGAuctionHouse::SetAuctioneer(auctioneer, auctionHouseID);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_HOUSE_DISABLED, nullptr);
    }

    return 1;
}

// VERIFY: 7f72240d manual
// OFFSET: 0x0059FFB0
int32_t Packet_SMSG_AUCTION_COMMAND_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t inventoryError = BAG_INTERNAL_BAG_ERROR;
    uint32_t auctionID;
    uint32_t action;
    uint32_t result;
    msg->Get(auctionID);
    msg->Get(action);
    msg->Get(result);

    if (result == 1) {
        msg->Get(inventoryError);
    }

    if (CGAuctionHouse::m_havePendingSell && !action && result) {
        CGAuctionHouse::OnMultisellFailure(false);
    }

    switch (result) {
        case 0: {
            if (action == 0) {
                if (CGAuctionHouse::m_havePendingSell) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_MULTISELL_UPDATE, "%d%d", CGAuctionHouse::m_multisellTotal - CGAuctionHouse::m_multisellRemaining, CGAuctionHouse::m_multisellTotal);
                    CGAuctionHouse::SellNextItem();
                }

                CGGameUI::DisplayError(ERR_AUCTION_STARTED);
                CGAuctionHouse::m_canGetOwnerList = 1;
                CGAuctionHouse::GetOwnerList();
            } else if (action == 2) {
                uint32_t minIncrement;
                msg->Get(minIncrement);
                CGGameUI::DisplayError(ERR_AUCTION_BID_PLACED);
                CGAuctionHouse::ClearOutbidID(auctionID);
                CGAuctionHouse::m_canGetBidderList = 1;
                BidderListItems(CGAuctionHouse::m_lastBidderPage);

                if (CGAuctionHouse::m_auctioneer) {
                    for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
                        AuctionItemInfo* auction = &CGAuctionHouse::m_browseList.m_data[i];

                        if (auction->m_auctionID == auctionID) {
                            auction->m_bid = CGAuctionHouse::m_lastBidAmount;
                            auction->m_minIncrement = minIncrement;
                            auction->m_bidder = ClntObjMgrGetActivePlayer();
                            auction->m_expireTime = auction->m_expireTime <= OsGetAsyncTimeMs() + 90000 ? OsGetAsyncTimeMs() + 90000 : auction->m_expireTime;

                            if (!CGAuctionHouse::m_pendingBrowseItems) {
                                FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
                            }

                            break;
                        }
                    }
                }

                CDataStore emote;
                emote.Put(static_cast<uint32_t>(CMSG_EMOTE));
                emote.Put(static_cast<uint32_t>(3));
                emote.Finalize();
                ClientServices::Send2(&emote);
            } else if (action == 1) {
                CGGameUI::DisplayError(ERR_AUCTION_REMOVED);
                CGMailInfo::ReceivedMail(0.0f);
                CGAuctionHouse::AuctionItemExpired(auctionID);
            }

            return 1;
        }

        case 1:
            CGGameUI::DisplayError(static_cast<UIErrors>(GetDisplayErrorByInventoryError(inventoryError)));
            return 1;

        case 3:
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
            return 1;

        case 4:
            CGGameUI::DisplayError(ERR_ITEM_NOT_FOUND);
            return 1;

        case 5: {
            WOWGUID bidder;
            uint32_t bid;
            uint32_t minIncrement;
            msg->Get(bidder.m_guid);
            msg->Get(bid);
            msg->Get(minIncrement);

            if (CGAuctionHouse::m_auctioneer) {
                for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
                    AuctionItemInfo* auction = &CGAuctionHouse::m_browseList.m_data[i];

                    if (auction->m_auctionID == auctionID) {
                        auction->m_bid = bid;
                        auction->m_minIncrement = minIncrement;
                        auction->m_bidder = bidder;

                        if (!CGAuctionHouse::m_pendingBrowseItems) {
                            FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
                        }

                        break;
                    }
                }

                for (uint32_t i = 0; i < CGAuctionHouse::m_bidderList.Count(); i++) {
                    AuctionItemInfo* auction = &CGAuctionHouse::m_bidderList.m_data[i];

                    if (auction->m_auctionID == auctionID) {
                        auction->m_bid = bid;
                        auction->m_minIncrement = minIncrement;
                        auction->m_bidder = bidder;

                        if (!CGAuctionHouse::m_pendingBidItems) {
                            FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_BIDDER_LIST_UPDATE, nullptr);
                        }

                        break;
                    }
                }
            }

            CGGameUI::DisplayError(ERR_AUCTION_HIGHER_BID);
            return 1;
        }

        case 7:
            CGGameUI::DisplayError(ERR_AUCTION_BID_INCREMENT);
            return 1;

        case 10:
            CGGameUI::DisplayError(ERR_AUCTION_BID_OWN);
            return 1;

        case 13:
            CGGameUI::DisplayError(ERR_RESTRICTED_ACCOUNT);
            return 1;

        default:
            CGGameUI::DisplayError(ERR_AUCTION_DATABASE_ERROR);
            return 1;
    }
}

// VERIFY: ed9ac24c manual
// OFFSET: 0x005A0480
int32_t Packet_SMSG_AUCTION_BIDDER_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t auctionHouseID;
    uint32_t auctionID;
    WOWGUID bidder;
    uint32_t bid;
    uint32_t minIncrement;
    uint32_t itemID;
    uint32_t randomPropertyID;
    msg->Get(auctionHouseID);
    msg->Get(auctionID);
    msg->Get(bidder.m_guid);
    msg->Get(bid);
    msg->Get(minIncrement);
    msg->Get(itemID);
    msg->Get(randomPropertyID);

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemID, none, nullptr, nullptr, false);

    if (!stats) {
        AuctionNotifyInfo* info = static_cast<AuctionNotifyInfo*>(SMemAlloc(sizeof(AuctionNotifyInfo), __FILE__, __LINE__, 0));
        info->m_randomPropertyID = randomPropertyID;
        info->m_error = bid ? ERR_AUCTION_OUTBID_S : ERR_AUCTION_WON_S;
        none = WOWGUID();
        stats = g_itemDBCache.GetRecord(itemID, none, AucNotifyCallback, info, false);
    }

    char name[256];
    CGItem_C::BuildItemName(name, sizeof(name), itemID, randomPropertyID);

    if (bid) {
        if (stats) {
            CGGameUI::DisplayError(ERR_AUCTION_OUTBID_S, name);
            CGLCD::AuctionOutbid(name);
        }

        CGAuctionHouse::AddOutbidID(auctionHouseID, auctionID);

        if (CGAuctionHouse::m_auctionHouseID == static_cast<int32_t>(auctionHouseID)) {
            for (uint32_t i = 0; i < CGAuctionHouse::m_bidderList.Count(); i++) {
                AuctionItemInfo* auction = &CGAuctionHouse::m_bidderList.m_data[i];

                if (auction->m_auctionID == auctionID) {
                    auction->m_bid = bid;
                    auction->m_minIncrement = minIncrement;
                    auction->m_bidder = bidder;
                    auction->m_expireTime = auction->m_expireTime <= OsGetAsyncTimeMs() + 90000 ? OsGetAsyncTimeMs() + 90000 : auction->m_expireTime;

                    if (!CGAuctionHouse::m_pendingBidItems) {
                        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_BIDDER_LIST_UPDATE, nullptr);
                    }

                    break;
                }
            }

            for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
                AuctionItemInfo* auction = &CGAuctionHouse::m_browseList.m_data[i];

                if (auction->m_auctionID == auctionID) {
                    auction->m_bid = bid;
                    auction->m_minIncrement = minIncrement;
                    auction->m_bidder = bidder;
                    auction->m_expireTime = auction->m_expireTime <= OsGetAsyncTimeMs() + 90000 ? OsGetAsyncTimeMs() + 90000 : auction->m_expireTime;

                    if (!CGAuctionHouse::m_pendingBrowseItems) {
                        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
                    }

                    break;
                }
            }
        }
    } else {
        if (stats) {
            CGGameUI::DisplayError(ERR_AUCTION_WON_S, name);
            CGLCD::AuctionWon(name);
        }

        CGAuctionHouse::AuctionItemExpired(auctionID);
    }

    CGMailInfo::ReceivedMail(0.0f);
    return 1;
}

// VERIFY: 664618d2 manual
// OFFSET: 0x005A0790
int32_t Packet_SMSG_AUCTION_OWNER_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t auctionID;
    uint32_t bid;
    uint32_t minIncrement;
    WOWGUID bidder;
    uint32_t itemID;
    uint32_t randomPropertyID;
    float mailDelay;
    msg->Get(auctionID);
    msg->Get(bid);
    msg->Get(minIncrement);
    msg->Get(bidder.m_guid);
    msg->Get(itemID);
    msg->Get(randomPropertyID);
    msg->Get(mailDelay);

    if (bidder) {
        if (!CGAuctionHouse::m_auctioneer) {
            return 1;
        }

        for (uint32_t i = 0; i < CGAuctionHouse::m_ownerList.Count(); i++) {
            AuctionItemInfo* auction = &CGAuctionHouse::m_ownerList.m_data[i];

            if (auction->m_auctionID == auctionID) {
                auction->m_bid = bid;
                auction->m_minIncrement = minIncrement;
                auction->m_bidder = bidder;
                auction->m_expireTime = auction->m_expireTime <= OsGetAsyncTimeMs() + 90000 ? OsGetAsyncTimeMs() + 90000 : auction->m_expireTime;

                if (!CGAuctionHouse::m_pendingOwnedItems && !CGAuctionHouse::m_pendingSoldItems) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
                }

                break;
            }
        }

        for (uint32_t i = 0; i < CGAuctionHouse::m_browseList.Count(); i++) {
            AuctionItemInfo* auction = &CGAuctionHouse::m_browseList.m_data[i];

            if (auction->m_auctionID == auctionID) {
                auction->m_bid = bid;
                auction->m_minIncrement = minIncrement;
                auction->m_bidder = bidder;
                auction->m_expireTime = auction->m_expireTime <= OsGetAsyncTimeMs() + 90000 ? OsGetAsyncTimeMs() + 90000 : auction->m_expireTime;

                if (!CGAuctionHouse::m_pendingBrowseItems) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_ITEM_LIST_UPDATE, nullptr);
                }

                break;
            }
        }

        return 1;
    }

    WOWGUID none = WOWGUID();

    if (g_itemDBCache.GetRecord(itemID, none, nullptr, nullptr, false)) {
        char name[256];
        CGItem_C::BuildItemName(name, sizeof(name), itemID, randomPropertyID);

        if (bid) {
            CGGameUI::DisplayError(ERR_AUCTION_SOLD_S, name);
            CGLCD::AuctionSold(name);
        } else {
            CGGameUI::DisplayError(ERR_AUCTION_EXPIRED_S, name);
            CGLCD::AuctionExpired(name);
        }
    } else {
        AuctionNotifyInfo* info = static_cast<AuctionNotifyInfo*>(SMemAlloc(sizeof(AuctionNotifyInfo), __FILE__, __LINE__, 0));
        info->m_randomPropertyID = randomPropertyID;
        info->m_error = bid ? ERR_AUCTION_SOLD_S : ERR_AUCTION_EXPIRED_S;
        none = WOWGUID();
        g_itemDBCache.GetRecord(itemID, none, AucNotifyCallback, info, false);
    }

    CGMailInfo::ReceivedMail(bid ? mailDelay : 0.0f);
    CGAuctionHouse::AuctionItemExpired(auctionID);
    return 1;
}

// VERIFY: bb5f4e66 manual
// OFFSET: 0x005A0AC0
int32_t Packet_SMSG_AUCTION_REMOVED_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t auctionID;
    uint32_t itemID;
    uint32_t randomPropertyID;
    msg->Get(auctionID);
    msg->Get(itemID);
    msg->Get(randomPropertyID);

    WOWGUID none = WOWGUID();

    if (g_itemDBCache.GetRecord(itemID, none, nullptr, nullptr, false)) {
        char name[256];
        CGItem_C::BuildItemName(name, sizeof(name), itemID, randomPropertyID);
        CGGameUI::DisplayError(ERR_AUCTION_REMOVED_S, name);
    } else {
        AuctionNotifyInfo* info = static_cast<AuctionNotifyInfo*>(SMemAlloc(sizeof(AuctionNotifyInfo), __FILE__, __LINE__, 0));
        info->m_randomPropertyID = randomPropertyID;
        info->m_error = ERR_AUCTION_REMOVED_S;
        none = WOWGUID();
        g_itemDBCache.GetRecord(itemID, none, AucNotifyCallback, info, false);
    }

    CGAuctionHouse::AuctionItemExpired(auctionID);
    return 1;
}

// VERIFY: 85fd82d7 manual
// OFFSET: 0x005A0BA0
void CGAuctionHouse::InitializeGame() {
    ClientServices::SetMessageHandler(MSG_AUCTION_HELLO, Packet_MSG_AUCTION_HELLO, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_COMMAND_RESULT, Packet_SMSG_AUCTION_COMMAND_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_LIST_RESULT, Packet_SMSG_AUCTION_LIST_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_LIST_OWNER_ITEMS_RESULT, Packet_SMSG_AUCTION_OWNER_LIST_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_LIST_BIDDER_ITEMS_RESULT, Packet_SMSG_AUCTION_BIDDER_LIST_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_BIDDER_NOTIFICATION, Packet_SMSG_AUCTION_BIDDER_NOTIFICATION, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_OWNER_NOTIFICATION, Packet_SMSG_AUCTION_OWNER_NOTIFICATION, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_REMOVED_NOTIFICATION, Packet_SMSG_AUCTION_REMOVED_NOTIFICATION, nullptr);
    ClientServices::SetMessageHandler(SMSG_AUCTION_LIST_PENDING_SALES, Packet_SMSG_AUCTION_LIST_PENDING_SALES, nullptr);

    CGAuctionHouse::m_ownerList.SetCount(0);
    CGAuctionHouse::m_bidderList.SetCount(0);
    CGAuctionHouse::m_browseList.SetCount(0);
    CGAuctionHouse::m_lastGetAllTime = 0;
    CGAuctionHouse::m_lastQueryTime[0] = 0;
    CGAuctionHouse::m_lastQueryTime[1] = 0;
    CGAuctionHouse::m_lastQueryTime[2] = 0;
    CGAuctionHouse::m_queryPending[0] = 0;
    CGAuctionHouse::m_queryPending[1] = 0;
    CGAuctionHouse::m_queryPending[2] = 0;
    CGAuctionHouse::m_numOutbidItems = 0;

    CGAuctionHouse::m_outbidItems.SetCount(8);

    for (uint32_t i = 0; i < 8; i++) {
        CGAuctionHouse::m_outbidItems.m_data[i] = static_cast<OutbidItemInfo*>(SMemAlloc(sizeof(OutbidItemInfo), __FILE__, __LINE__, 0));
    }

    for (uint32_t i = 0; i < 12; i++) {
        CGAuctionHouse::m_listSortCriteria[i].m_valid = 0;
    }

    CGAuctionHouse::SetBrowseSortCriteria(3, false);
    CGAuctionHouse::SetBrowseSortCriteria(8, false);
    CGAuctionHouse::SetBrowseSortCriteria(9, true);
    CGAuctionHouse::SetBrowseSortCriteria(6, false);
    CGAuctionHouse::SetBrowseSortCriteria(5, false);
    CGAuctionHouse::SetBrowseSortCriteria(0, true);
    CGAuctionHouse::SetBrowseSortCriteria(1, false);

    for (uint32_t i = 0; i < 12; i++) {
        CGAuctionHouse::m_ownedSortCriteria[i].m_valid = 0;
    }

    CGAuctionHouse::SetOwnedSortCriteria(9, true);
    CGAuctionHouse::SetOwnedSortCriteria(8, false);
    CGAuctionHouse::SetOwnedSortCriteria(5, false);
    CGAuctionHouse::SetOwnedSortCriteria(0, true);
    CGAuctionHouse::SetOwnedSortCriteria(1, false);
    CGAuctionHouse::SetOwnedSortCriteria(4, false);
    CGAuctionHouse::SetOwnedSortCriteria(3, false);

    for (uint32_t i = 0; i < 12; i++) {
        CGAuctionHouse::m_bidSortCriteria[i].m_valid = 0;
    }

    CGAuctionHouse::SetBidSortCriteria(9, false);
    CGAuctionHouse::SetBidSortCriteria(5, false);
    CGAuctionHouse::SetBidSortCriteria(0, true);
    CGAuctionHouse::SetBidSortCriteria(1, false);
    CGAuctionHouse::SetBidSortCriteria(6, false);
    CGAuctionHouse::SetBidSortCriteria(4, false);
    CGAuctionHouse::SetBidSortCriteria(8, false);
    CGAuctionHouse::SetBidSortCriteria(3, false);
}

// VERIFY: 93924a65 manual
// OFFSET: 0x0059E090
void CGAuctionHouse::ShutdownGame() {
    ClientServices::ClearMessageHandler(MSG_AUCTION_HELLO);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_COMMAND_RESULT);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_LIST_RESULT);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_LIST_OWNER_ITEMS_RESULT);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_LIST_BIDDER_ITEMS_RESULT);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_BIDDER_NOTIFICATION);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_OWNER_NOTIFICATION);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_REMOVED_NOTIFICATION);
    ClientServices::ClearMessageHandler(SMSG_AUCTION_LIST_PENDING_SALES);

    for (uint32_t i = 0; i < CGAuctionHouse::m_outbidItems.m_count; i++) {
        delete CGAuctionHouse::m_outbidItems.m_data[i];
    }

    CGAuctionHouse::m_outbidItems.Clear();
    CGAuctionHouse::m_numOutbidItems = 0;
}

// VERIFY: c943a21b manual
// OFFSET: 0x0076DDE0
uint32_t CalculateAuctionDeposit(uint32_t depositRate, uint32_t sellPrice, uint32_t runTime) {
    for (uint32_t i = 0; i < 3; i++) {
        if (runTime == s_depositRunTimes[i]) {
            uint32_t deposit = static_cast<uint32_t>(static_cast<float>(depositRate * sellPrice / 100) * runTime * 0.0041666669f);
            return deposit < 100 ? 100 : deposit;
        }
    }

    return 100;
}
