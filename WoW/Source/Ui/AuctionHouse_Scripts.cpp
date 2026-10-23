#include "AuctionHouse.h"
#include <cstdint>
#include <cstdlib>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AuctionHouseRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/DB/Rec/AuctionHouseRec.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/DB/Rec/ItemClassRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00A14F70
static const AuctionItemClass s_auctionClasses[12] = {
    { 2, 1 },
    { 4, 1 },
    { 1, 1 },
    { 0, 1 },
    { 16, 1 },
    { 7, 1 },
    { 6, 1 },
    { 11, 1 },
    { 9, 1 },
    { 3, 1 },
    { 15, 1 },
    { 12, 0 },
};

// STATIC: 0x00A14FD0
static const AuctionInvType s_auctionInvTypes[14] = {
    { 1, "INVTYPE_HEAD", 1, 0 },
    { 2, "INVTYPE_NECK", 1, 1 },
    { 3, "INVTYPE_SHOULDER", 0, 0 },
    { 4, "INVTYPE_BODY", 1, 1 },
    { 5, "INVTYPE_CHEST", 0, 0 },
    { 6, "INVTYPE_WAIST", 0, 0 },
    { 7, "INVTYPE_LEGS", 0, 0 },
    { 8, "INVTYPE_FEET", 0, 0 },
    { 9, "INVTYPE_WRIST", 0, 0 },
    { 10, "INVTYPE_HAND", 0, 0 },
    { 11, "INVTYPE_FINGER", 1, 1 },
    { 12, "INVTYPE_TRINKET", 1, 1 },
    { 16, "INVTYPE_CLOAK", 0, 1 },
    { 23, "INVTYPE_HOLDABLE", 1, 1 },
};

// VERIFY: 95a282d9 auto
// OFFSET: 0x0059B0B0
static int32_t Script_SetAuctionsTabShowing(lua_State* L) {
    if (lua_type(L, 1) != LUA_TBOOLEAN) {
        luaL_error(L, "Usage: SetAuctionsTabShowing(bool)");
        return 0;
    }

    CGAuctionHouse::m_auctionsTabShowing = lua_toboolean(L, 1);
    return 0;
}

// VERIFY: 1477dadf auto
// OFFSET: 0x0059BC90
static int32_t Script_GetAuctionHouseDepositRate(lua_State* L) {
    const AuctionHouseRec* rec = g_auctionHouseDB.GetRecord(CGAuctionHouse::m_auctionHouseID);

    if (rec) {
        lua_pushnumber(L, rec->m_depositRate);
        return 1;
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: d9ff6315 auto
// OFFSET: 0x0059C420
static int32_t Script_GetAuctionItemClasses(lua_State* L) {
    int32_t count = 0;

    for (uint32_t i = 0; i < 12; i++) {
        ItemClassRec* rec = g_itemClassDB.GetRecord(s_auctionClasses[i].m_classID);

        if (rec) {
            lua_pushstring(L, rec->m_className);
            count++;
        }
    }

    return count;
}

// VERIFY: f1905a55 manual
// OFFSET: 0x0059C480
static int32_t Script_GetAuctionItemSubClasses(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetAuctionItemSubClasses(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 12 || !s_auctionClasses[index].m_hasSubClasses) {
        return 0;
    }

    int32_t count = 0;

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* rec = i < 0 || i >= g_itemSubClassDB.m_numRecords ? nullptr : &g_itemSubClassDB.m_records[i];

        if (rec->m_classID == s_auctionClasses[index].m_classID && !(rec->m_displayFlags & 0x2)) {
            lua_checkstack(L, 1);
            lua_pushstring(L, *rec->m_verboseName ? rec->m_verboseName : rec->m_displayName);
            count++;
        }
    }

    return count;
}

// VERIFY: 5c03f091 manual
// OFFSET: 0x0059C590
static int32_t Script_GetAuctionInvTypes(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAuctionInvTypes(classIndex, subClassIndex)");
        return 0;
    }

    uint32_t classIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t subClass = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;

    if (classIndex >= 12 || !s_auctionClasses[classIndex].m_hasSubClasses) {
        return 0;
    }

    int32_t remaining = subClass;

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* rec = i < 0 || i >= g_itemSubClassDB.m_numRecords ? nullptr : &g_itemSubClassDB.m_records[i];

        if (rec->m_classID != s_auctionClasses[classIndex].m_classID || (rec->m_displayFlags & 0x2)) {
            continue;
        }

        if (!remaining) {
            if (!(rec->m_flags & 0x200)) {
                return 0;
            }

            break;
        }

        remaining--;
    }

    lua_checkstack(L, 28);

    for (uint32_t i = 0; i < 14; i++) {
        lua_pushstring(L, s_auctionInvTypes[i].m_token);

        if (s_auctionClasses[classIndex].m_classID == 4) {
            if (subClass) {
                if ((subClass != 1 || s_auctionInvTypes[i].m_invType != 16) && s_auctionInvTypes[i].unk_0028) {
                    lua_pushnil(L);
                    continue;
                }
            } else if (!s_auctionInvTypes[i].unk_0024) {
                lua_pushnil(L);
                continue;
            }
        }

        lua_pushnumber(L, 1.0);
    }

    return 28;
}

// VERIFY: a694c251 auto
// OFFSET: 0x0059C9F0
static int32_t Script_CanCancelAuction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CanCancelAuction(index)");
        return 0;
    }

    if (!CGAuctionHouse::m_auctioneer) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    AuctionItemInfo* auction;

    if (index >= CGAuctionHouse::m_ownerList.Count() || !(auction = &CGAuctionHouse::m_ownerList.m_data[index]) || auction->m_sold) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: afaf3777 manual
// OFFSET: 0x0059DDF0
static int32_t Script_CancelAuction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetSelectedAuctionItem(index)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(8)) {
        return 0;
    }

    WOWGUID auctioneer = CGAuctionHouse::m_auctioneer;

    if (!auctioneer) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGAuctionHouse::m_ownerList.Count()) {
        return 0;
    }

    AuctionItemInfo* auction = &CGAuctionHouse::m_ownerList.m_data[index];

    if (!auction || auction->m_sold) {
        return 0;
    }

    if (auction->m_bid) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (!player || player->GetCoinage() < static_cast<uint32_t>(auction->m_bid * 0.05f)) {
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
            return 0;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUCTION_REMOVE_ITEM));
    msg.Put(auctioneer.m_guid);
    msg.Put(static_cast<uint32_t>(auction->m_auctionID));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: c9d2ca3b auto
// OFFSET: 0x0059C1A0
static int32_t Script_GetNumAuctionItems(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetNumAuctionItems(\"type\")");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint32_t total;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        lua_pushnumber(L, CGAuctionHouse::m_ownerList.Count());
        total = CGAuctionHouse::m_ownerList.Count();
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        lua_pushnumber(L, CGAuctionHouse::m_bidderList.Count());
        total = CGAuctionHouse::m_numTotalBid;
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        lua_pushnumber(L, CGAuctionHouse::m_browseList.Count());
        total = CGAuctionHouse::m_numTotalItems;
    } else {
        return 2;
    }

    lua_pushnumber(L, total);
    return 2;
}

// STATIC: 0x00A15238
static const uint32_t s_timeLeftThresholds[3] = { 1800000, 7200000, 43200000 };

// STATIC: 0x00A15248
static const uint32_t AUCTION_RUN_TIMES[3] = { 720, 1440, 2880 };

// VERIFY: ae094fc9 manual
// OFFSET: 0x0059AB90
static int32_t Script_IsAuctionSortReversed(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: IsAuctionSortReversed(\"type\", \"sort\")");
        return 0;
    }

    int32_t column = 1;
    const char* type = lua_tolstring(L, 1, nullptr);
    const char* sort = lua_tolstring(L, 2, nullptr);

    if (!AuctionSortToColumnIndex(sort, &column)) {
        return 0;
    }

    AuctionSortCriteria* criteria;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        criteria = CGAuctionHouse::m_ownedSortCriteria;
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        criteria = CGAuctionHouse::m_bidSortCriteria;
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        criteria = CGAuctionHouse::m_listSortCriteria;
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    for (uint32_t i = 0; i < 12; i++) {
        AuctionSortCriteria* entry = i < 12 ? &criteria[i] : nullptr;

        if (entry->m_valid && entry->m_column == column) {
            if (entry->m_reverse) {
                lua_pushnumber(L, 1.0);
            } else {
                lua_pushnil(L);
            }

            lua_pushnumber(L, 1.0);
            return 2;
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

// VERIFY: 7b782a18 manual
// OFFSET: 0x0059AD40
static int32_t Script_GetAuctionSort(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAuctionSort(\"type\", \"index\")");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint32_t index = static_cast<uint32_t>(lua_tointeger(L, 2)) - 1;
    AuctionSortCriteria* criteria;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        criteria = index < 12 ? &CGAuctionHouse::m_ownedSortCriteria[index] : nullptr;
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        criteria = CGAuctionHouse::GetBidSortCriteria(index);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        criteria = CGAuctionHouse::GetBrowseSortCriteria(index);
    } else {
        criteria = nullptr;
    }

    if (!criteria || !criteria->m_valid) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, CGAuctionHouse::SortTypeToName(criteria->m_column));

    if (criteria->m_reverse) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: a7708e62 auto
// OFFSET: 0x0059AE70
static int32_t Script_SortAuctionClearSort(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SortAuctionClearSort(\"type\")");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        CGAuctionHouse::ClearOwnedSortCriteria();
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        CGAuctionHouse::ClearBidSortCriteria();
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        CGAuctionHouse::ClearBrowseSortCriteria();
    }

    return 0;
}

// VERIFY: bf332834 auto
// OFFSET: 0x0059AF10
static int32_t Script_SortAuctionSetSort(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SortAuctionAddSort(\"type\", \"sort\", \"reverse\")");
        return 0;
    }

    int32_t column = 1;
    const char* type = lua_tolstring(L, 1, nullptr);
    const char* sort = lua_tolstring(L, 2, nullptr);
    bool reverse = lua_type(L, 3) == LUA_TBOOLEAN && lua_toboolean(L, 3);

    if (!AuctionSortToColumnIndex(sort, &column)) {
        return 0;
    }

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        CGAuctionHouse::SetOwnedSortCriteria(column, reverse);
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        CGAuctionHouse::SetBidSortCriteria(column, reverse);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        CGAuctionHouse::SetBrowseSortCriteria(column, reverse);
    }

    return 0;
}

// VERIFY: c7c95ee4 manual
// OFFSET: 0x0059B040
static int32_t Script_CancelSell(lua_State* L) {
    if (CGAuctionHouse::m_havePendingSell) {
        CGAuctionHouse::OnMultisellFailure(false);
    }

    return 0;
}

// VERIFY: 048eae8f manual
// OFFSET: 0x0059BCF0
static int32_t Script_QueryAuctionItems(lua_State* L) {
    WOWGUID auctioneer = CGAuctionHouse::m_auctioneer;

    if (!auctioneer || !CGAuctionHouse::CanSendQuery(0)) {
        return 0;
    }

    const char* name = lua_isstring(L, 1) ? lua_tolstring(L, 1, nullptr) : "";
    uint8_t minLevel = 0;
    uint8_t maxLevel = 0;

    if (lua_isnumber(L, 2)) {
        minLevel = static_cast<uint8_t>(lua_tonumber(L, 2));
    }

    if (lua_isnumber(L, 3)) {
        maxLevel = static_cast<uint8_t>(lua_tonumber(L, 3));
    }

    int32_t invType = -1;

    if (lua_isnumber(L, 4)) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 4)) - 1;

        if (index < 14) {
            invType = s_auctionInvTypes[index].m_invType;
        }
    }

    int32_t classID = -1;

    if (lua_isnumber(L, 5)) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 5)) - 1;

        if (index < 12) {
            classID = s_auctionClasses[index].m_classID;
        }
    }

    int32_t subClassID = -1;

    if (classID >= 0 && lua_isnumber(L, 6)) {
        int32_t remaining = static_cast<int32_t>(lua_tonumber(L, 6)) - 1;

        for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
            ItemSubClassRec* rec = i < 0 || i >= g_itemSubClassDB.m_numRecords ? nullptr : &g_itemSubClassDB.m_records[i];

            if (rec->m_classID == classID && !(rec->m_displayFlags & 0x2)) {
                if (!remaining) {
                    subClassID = rec->m_subClassID;
                    break;
                }

                remaining--;
            }
        }
    }

    int32_t page = 0;

    if (lua_isnumber(L, 7)) {
        page = 50 * static_cast<int32_t>(lua_tonumber(L, 7));
    }

    uint8_t usable = 0;

    if (lua_isnumber(L, 8)) {
        usable = static_cast<uint8_t>(lua_tonumber(L, 8));
    }

    int32_t quality = -1;

    if (lua_isnumber(L, 9)) {
        quality = static_cast<int32_t>(lua_tonumber(L, 9));
    }

    bool getAll = lua_type(L, 10) == LUA_TBOOLEAN && lua_toboolean(L, 10);

    if (getAll && (CGAuctionHouse::m_queryPending[0] > 0 || (CGAuctionHouse::m_lastGetAllTime && static_cast<int32_t>(OsGetAsyncTimeMs() - CGAuctionHouse::m_lastGetAllTime - 900000) < 0) || !CGAuctionHouse::CanSendQuery(0))) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUCTION_LIST_ITEMS));
    msg.Put(auctioneer.m_guid);
    msg.Put(static_cast<uint32_t>(page));
    msg.PutString(name);
    msg.Put(minLevel);
    msg.Put(maxLevel);
    msg.Put(static_cast<uint32_t>(invType));
    msg.Put(static_cast<uint32_t>(classID));
    msg.Put(static_cast<uint32_t>(subClassID));
    msg.Put(static_cast<uint32_t>(quality));
    msg.Put(usable);
    msg.Put(static_cast<uint8_t>(getAll));

    uint8_t numCriteria = 0;
    uint32_t countPos = msg.m_size;
    msg.Put(static_cast<uint8_t>(0));

    for (uint32_t i = 0; i < 12; i++) {
        AuctionSortCriteria* criteria = i < 12 ? &CGAuctionHouse::m_listSortCriteria[i] : nullptr;

        if (criteria && criteria->m_valid) {
            msg.Put(static_cast<uint8_t>(criteria->m_column));
            msg.Put(static_cast<uint8_t>(criteria->m_reverse != 0));
            numCriteria++;
        }
    }

    msg.Set(countPos, numCriteria);
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (++CGAuctionHouse::m_queryPending[0] < 0) {
        CGAuctionHouse::m_queryPending[0] = 0;
    }

    CGAuctionHouse::m_lastQueryTime[0] = OsGetAsyncTimeMs();

    if (getAll) {
        CGAuctionHouse::m_lastGetAllTime = OsGetAsyncTimeMs();
    }

    return 0;
}

// VERIFY: 33aafb9b manual
// OFFSET: 0x0059C2D0
static int32_t Script_GetAuctionItemLink(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAuctionItemLink(\"type\", index)");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    AuctionItemInfo* auction;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        if (index >= CGAuctionHouse::m_ownerList.Count()) {
            return 0;
        }

        auction = &CGAuctionHouse::m_ownerList.m_data[index];
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        auction = GetBidderAuction(index);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        auction = GetListAuction(index);
    } else {
        return 0;
    }

    if (!auction) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(auction->m_itemID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::CreateItemLink(auction->m_itemID, stats->m_quality, auction->m_enchantID[0], reinterpret_cast<const int32_t*>(&auction->m_enchantID[2]), auction->m_randomPropertyID, auction->m_suffixFactor));
    return 1;
}

// VERIFY: d702b148 auto
// OFFSET: 0x0059C740
static int32_t Script_CanSendAuctionQuery(lua_State* L) {
    int32_t type = 0;

    if (lua_isstring(L, 1)) {
        CGAuctionHouse::TableNameToType(lua_tolstring(L, 1, nullptr), &type);
    }

    if (CGAuctionHouse::CanSendQuery(type)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (type >= 3 || CGAuctionHouse::m_queryPending[type] > 0 || (CGAuctionHouse::m_lastGetAllTime && static_cast<int32_t>(OsGetAsyncTimeMs() - CGAuctionHouse::m_lastGetAllTime - 900000) < 0) || !CGAuctionHouse::CanSendQuery(type)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    return 2;
}

// VERIFY: ec7c594c manual
// OFFSET: 0x0059C830
static int32_t Script_SetSelectedAuctionItem(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetSelectedAuctionItem(\"type\", index)");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        CGAuctionHouse::SetSelectedOwnedItem(index);
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        CGAuctionHouse::SetSelectedBidItem(index);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        CGAuctionHouse::SetSelectedBrowseItem(index);
    }

    return 0;
}

// VERIFY: f95e702d auto
// OFFSET: 0x0059C920
static int32_t Script_GetSelectedAuctionItem(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetSelectedAuctionItem(\"type\")");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        lua_pushnumber(L, CGAuctionHouse::GetSelectedOwnedIndex() + 1);
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        lua_pushnumber(L, CGAuctionHouse::GetSelectedBidIndex() + 1);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        lua_pushnumber(L, CGAuctionHouse::GetSelectedBrowseIndex() + 1);
    } else {
        lua_pushnumber(L, 1.0);
    }

    return 1;
}

// VERIFY: 7cc1108f auto
// OFFSET: 0x0059CAB0
static int32_t Script_SortAuctionApplySort(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SortAuctionApplySort(\"type\")");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        qsort(CGAuctionHouse::m_ownerList.m_data, CGAuctionHouse::m_ownerList.Count(), sizeof(AuctionItemInfo), CompareFunction);
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_OWNED_LIST_UPDATE, nullptr);
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        CGAuctionHouse::ApplyBidderSort();
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        CGAuctionHouse::ApplyBrowseSort();
    }

    return 0;
}

// VERIFY: 3de5ed5e auto
// OFFSET: 0x0059D270
static int32_t Script_CalculateAuctionDeposit(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalculateAuctionDeposit(runTime)");
        return 0;
    }

    const AuctionHouseRec* rec = g_auctionHouseDB.GetRecord(CGAuctionHouse::m_auctionHouseID);
    WOWGUID sellItem;
    CGItem_C* item;
    ItemStats_C* stats;

    if (!rec || !(sellItem = CGAuctionHouse::m_sellItem) || !(item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGAuctionHouse::m_sellItem, TYPE_ITEM, __FILE__, __LINE__))) || !(stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, sellItem, AuctionSellItemQueryCallback, nullptr, false))) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1) - 1.0);
    uint32_t runTime = index >= 3 ? 1440 : AUCTION_RUN_TIMES[index];
    int32_t count;

    if (lua_isnumber(L, 2)) {
        count = static_cast<int32_t>(lua_tonumber(L, 2));
    } else {
        count = item->m_item->m_itemFieldStackCount;
    }

    lua_pushnumber(L, CalculateAuctionDeposit(rec->m_depositRate, count * stats->m_sellPrice, runTime));
    return 1;
}

// VERIFY: a2920350 manual
// OFFSET: 0x0059D410
static int32_t Script_ClickAuctionSellItemButton(lua_State* L) {
    WOWGUID item;
    WOWGUID container;
    uint32_t slot;
    CGGameUI::GetCursorItem(item, container, slot);

    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (object && !CGAuctionHouse::IsSellItemValid(object, 1)) {
        return 0;
    }

    WOWGUID previous = CGAuctionHouse::m_sellItem;

    if (CGAuctionHouse::m_sellItem == item) {
        CGGameUI::ClearCursor(1, 1);
        return 0;
    }

    CGAuctionHouse::m_sellItem = item;
    uint8_t previousSlot = CGAuctionHouse::m_sellItemSlot;
    WOWGUID previousContainer = CGAuctionHouse::m_sellItemContainer;
    CGAuctionHouse::m_sellItemContainer = container;
    CGAuctionHouse::m_sellItemSlot = slot;
    CGGameUI::SetCursorItem(previous, previousContainer, previousSlot, 0, 0, 1);
    FrameScript_SignalEvent(SCRIPT_EVENT_NEW_AUCTION_UPDATE, nullptr);
    return 0;
}

// VERIFY: a837f6c9 manual
// OFFSET: 0x0059D4F0
static int32_t Script_GetOwnerAuctionItems(lua_State* L) {
    if (CGAuctionHouse::m_auctioneer && CGAuctionHouse::m_canGetOwnerList && CGAuctionHouse::CanSendQuery(2)) {
        CGAuctionHouse::GetOwnerList();
    }

    return 0;
}

// VERIFY: 34540ddd manual
// OFFSET: 0x0059D540
static int32_t Script_GetBidderAuctionItems(lua_State* L) {
    if (CGAuctionHouse::m_auctioneer && CGAuctionHouse::m_canGetBidderList && CGAuctionHouse::CanSendQuery(1)) {
        int32_t page = 0;

        if (lua_isnumber(L, 1)) {
            page = 50 * static_cast<int32_t>(lua_tonumber(L, 1));
        }

        BidderListItems(page);
    }

    return 0;
}

// VERIFY: d3a6eff0 manual
// OFFSET: 0x0059D5E0
static int32_t Script_GetAuctionItemInfo(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAuctionItemInfo(\"type\", index)");
        return 0;
    }

    lua_checkstack(L, 12);

    const char* type = lua_tolstring(L, 1, nullptr);
    bool owned = false;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    AuctionItemInfo* auction = nullptr;
    void (*callback)(int32_t, const WOWGUID&, void*, bool) = nullptr;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        auction = index < CGAuctionHouse::m_ownerList.Count() ? &CGAuctionHouse::m_ownerList.m_data[index] : nullptr;
        owned = true;
        callback = Signal_EVENT_AUCTION_OWNED_LIST_UPDATE;
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        auction = GetBidderAuction(index);
        callback = Signal_EVENT_AUCTION_BIDDER_LIST_UPDATE;
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        auction = GetListAuction(index);
        callback = Signal_EVENT_AUCTION_ITEM_LIST_UPDATE;
    }

    ItemStats_C* stats = nullptr;

    if (auction) {
        WOWGUID none = WOWGUID();
        stats = g_itemDBCache.GetRecord(auction->m_itemID, none, nullptr, nullptr, false);
    }

    if (!stats) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, -1.0);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 13;
    }

    char name[256];
    CGItem_C::BuildItemName(name, sizeof(name), auction->m_itemID, auction->m_randomPropertyID);
    lua_pushstring(L, name);

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);

    lua_pushnumber(L, auction->m_count);
    lua_pushnumber(L, stats->m_quality);

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t error;

    if (!player || player->CanUseItem(stats, error)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    int32_t level = static_cast<int32_t>(stats->m_requiredLevel);
    lua_pushnumber(L, level >= 1 ? level : 1);
    lua_pushnumber(L, auction->m_minBid);
    lua_pushnumber(L, auction->m_minIncrement);
    lua_pushnumber(L, auction->m_buyout);
    lua_pushnumber(L, auction->m_bid);

    if (owned && auction->m_owner == ClntObjMgrGetActivePlayer()) {
        NameCache* bidder;

        if (auction->m_bidder && (bidder = g_nameDBCache.GetRecord(auction->m_bidder.m_guid, auction->m_bidder, callback, nullptr, true))) {
            lua_pushstring(L, bidder->m_name);
        } else {
            lua_pushnil(L);
        }
    } else if (auction->m_bidder == ClntObjMgrGetActivePlayer()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    NameCache* owner;

    if (auction->m_owner && (owner = g_nameDBCache.GetRecord(auction->m_owner.m_guid, auction->m_owner, callback, nullptr, true))) {
        lua_pushstring(L, owner->m_name);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, auction->m_sold);
    return 13;
}

// VERIFY: e2337546 manual
// OFFSET: 0x0059DA40
static int32_t Script_PlaceAuctionBid(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: PlaceAuctionBid(\"type\", index, bid)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(8)) {
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    AuctionItemInfo* auction = nullptr;
    uint32_t bid = static_cast<uint32_t>(lua_tonumber(L, 3));

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        auction = GetOwnerAuction(index);
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        auction = GetBidderAuction(index);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        auction = GetListAuction(index);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!auction || !player) {
        return 0;
    }

    WOWGUID auctioneer = CGAuctionHouse::m_auctioneer;

    if (!auctioneer) {
        return 0;
    }

    if (auction->m_buyout && bid > auction->m_buyout) {
        bid = auction->m_buyout;
    }

    if (bid < auction->m_minBid) {
        CGGameUI::DisplayError(ERR_AUCTION_MIN_BID);
        return 0;
    }

    if (bid <= auction->m_bid) {
        CGGameUI::DisplayError(ERR_AUCTION_HIGHER_BID);
        return 0;
    }

    if (auction->m_bid && bid < auction->m_bid + auction->m_minIncrement && (!auction->m_buyout || bid < auction->m_buyout)) {
        CGGameUI::DisplayError(ERR_AUCTION_BID_INCREMENT);
        return 0;
    }

    if (bid > 2000000000 && bid > auction->m_bid + auction->m_minIncrement) {
        return 0;
    }

    uint32_t cost = bid;

    if (auction->m_bidder == player->m_obj->m_guid) {
        cost = bid - auction->m_bid;
    }

    if (player->GetCoinage() < cost) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(auction->m_itemID, none, nullptr, nullptr, false);

    if (stats && !player->GetBag()->ItemCountValid(auction->m_itemID, stats, auction->m_count, true)) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUCTION_PLACE_BID));
    msg.Put(auctioneer.m_guid);
    msg.Put(auction->m_auctionID);
    msg.Put(bid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGAuctionHouse::m_lastBidAmount = bid;
    return 0;
}

// VERIFY: bd9b02e3 auto
// OFFSET: 0x0059DD00
static int32_t Script_SortAuctionItems(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SortAuctionItems(\"type\", \"sort\")");
        return 0;
    }

    int32_t column = 1;
    const char* type = lua_tolstring(L, 1, nullptr);
    const char* sort = lua_tolstring(L, 2, nullptr);

    if (!AuctionSortToColumnIndex(sort, &column)) {
        return 0;
    }

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        CGAuctionHouse::SortOwnedList(column);
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        CGAuctionHouse::SortBidderList(column);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        CGAuctionHouse::SortBrowseList(column);
    }

    return 0;
}

// VERIFY: 312c5739 auto
// OFFSET: 0x0059F750
static int32_t Script_CloseAuctionHouse(lua_State* L) {
    CGAuctionHouse::CloseAuctionHouse();
    return 0;
}

// VERIFY: 2f0e242d manual
// OFFSET: 0x0059F760
static int32_t Script_GetAuctionSellItemInfo(lua_State* L) {
    WOWGUID sellItem = CGAuctionHouse::m_sellItem;
    CGItem_C* item;
    ItemStats_C* stats;

    if (!sellItem || !(item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGAuctionHouse::m_sellItem, TYPE_ITEM, __FILE__, __LINE__))) || !(stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, sellItem, AuctionSellItemQueryCallback, nullptr, false))) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, -1.0);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 9;
    }

    char name[256];
    item->GetFullItemName(name, sizeof(name));
    lua_pushstring(L, name);

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);

    lua_pushnumber(L, item->m_item->m_itemFieldStackCount);
    lua_pushnumber(L, stats->m_quality);

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t error;

    if (!player || player->CanUseItem(stats, error)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, stats->m_sellPrice * item->m_item->m_itemFieldStackCount);
    lua_pushnumber(L, stats->m_sellPrice);
    lua_pushnumber(L, stats->m_stackable);
    lua_pushnumber(L, FindSellableItems(item->m_obj->m_entryID));
    return 9;
}

// VERIFY: 6bd4bdb4 auto
// OFFSET: 0x0059F990
static int32_t Script_StartAuction(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: StartAuction(minBid, buyoutPrice, runTime, count)");
        return 0;
    }

    WOWGUID auctioneer = CGAuctionHouse::m_auctioneer;
    WOWGUID sellItem = CGAuctionHouse::m_sellItem;

    if (!auctioneer || !sellItem) {
        return 0;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGAuctionHouse::m_sellItem, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    if (item->GetMaxDurability() && item->GetDurability() < item->GetMaxDurability()) {
        CGGameUI::DisplayError(ERR_AUCTION_REPAIR_ITEM);
        return 0;
    }

    if (stats->m_spellId[0] && item->m_item->m_itemFieldSpellCharges[0] != stats->m_spellCharges[0]) {
        CGGameUI::DisplayError(ERR_AUCTION_USED_CHARGES);
        return 0;
    }

    uint32_t minBid = static_cast<uint32_t>(lua_tonumber(L, 1));
    uint32_t buyout = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (buyout && buyout < minBid) {
        buyout = minBid;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3) - 1.0);
    uint32_t runTime = index >= 3 ? 1440 : AUCTION_RUN_TIMES[index];
    uint32_t stackSize = 0;

    if (lua_isnumber(L, 4)) {
        stackSize = static_cast<uint32_t>(lua_tonumber(L, 4));
    }

    uint32_t numStacks = 1;

    if (lua_isnumber(L, 5)) {
        numStacks = static_cast<uint32_t>(lua_tonumber(L, 5));
    }

    if (static_cast<uint32_t>(item->m_item->m_itemFieldStackCount) >= stackSize && numStacks <= 1) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_AUCTION_SELL_ITEM));
        msg.Put(auctioneer.m_guid);
        msg.Put(static_cast<uint32_t>(1));
        msg.Put(sellItem.m_guid);
        msg.Put(stackSize);
        msg.Put(minBid);
        msg.Put(buyout);
        msg.Put(runTime);
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGAuctionHouse::ClearSellItem();
        return 0;
    }

    int32_t numItems = FindItemForAuction(sellItem, stackSize * numStacks);

    if (!numItems) {
        CGGameUI::DisplayError(ERR_AUCTION_ENOUGH_ITEMS);
        CGAuctionHouse::ClearSellItem();
        return 0;
    }

    CGAuctionHouse::m_multisellAuctioneer = auctioneer;
    CGAuctionHouse::m_multisellBid = minBid;
    CGAuctionHouse::m_multisellBuyout = buyout;
    CGAuctionHouse::m_multisellDuration = runTime;
    CGAuctionHouse::m_multisellStackSize = stackSize;
    CGAuctionHouse::m_multisellRemaining = numStacks;
    CGAuctionHouse::m_multisellTotal = numStacks;
    CGAuctionHouse::m_numSellItems = numItems;

    if (numStacks <= 1) {
        CGAuctionHouse::SellNextItem();
        CGAuctionHouse::ClearSellItem();
    } else {
        CGAuctionHouse::m_havePendingSell = 1;
        FrameScript_SignalEvent(SCRIPT_EVENT_AUCTION_MULTISELL_START, "%d", numStacks);
        CGAuctionHouse::LockAllSellItems();
        CGAuctionHouse::SellNextItem();
    }

    return 0;
}

// VERIFY: 2139eff4 manual
// OFFSET: 0x0059FD60
static int32_t Script_GetAuctionItemTimeLeft(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetAuctionItemTimeLeft(\"type\", index)");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    AuctionItemInfo* auction;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        auction = index < CGAuctionHouse::m_ownerList.Count() ? &CGAuctionHouse::m_ownerList.m_data[index] : nullptr;
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        auction = GetBidderAuction(index);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        auction = GetListAuction(index);
    } else {
        auction = nullptr;
    }

    if (!auction) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (static_cast<int32_t>(now - auction->m_expireTime) < 0) {
        if (auction->m_sold == 0) {
            uint32_t timeLeft = auction->m_expireTime - now;

            for (uint32_t i = 0; i < 3; i++) {
                if (timeLeft < s_timeLeftThresholds[i]) {
                    lua_pushnumber(L, i + 1);
                    return 1;
                }
            }

            lua_pushnumber(L, 4.0);
        } else if (auction->m_sold == 1) {
            lua_pushnumber(L, (auction->m_expireTime - now) / 1000);
        }

        return 1;
    }

    lua_pushnumber(L, 1.0);

    if (auction->m_sold == 1 || now - auction->m_expireTime > 10000) {
        CGAuctionHouse::AuctionItemExpired(auction->m_auctionID);
    }

    return 1;
}

// STATIC: 0x00ACF638
static FrameScript_Method s_ScriptFunctions[] = {
    { "CloseAuctionHouse", &Script_CloseAuctionHouse },
    { "GetAuctionHouseDepositRate", &Script_GetAuctionHouseDepositRate },
    { "CalculateAuctionDeposit", &Script_CalculateAuctionDeposit },
    { "ClickAuctionSellItemButton", &Script_ClickAuctionSellItemButton },
    { "GetAuctionSellItemInfo", &Script_GetAuctionSellItemInfo },
    { "StartAuction", &Script_StartAuction },
    { "QueryAuctionItems", &Script_QueryAuctionItems },
    { "GetOwnerAuctionItems", &Script_GetOwnerAuctionItems },
    { "GetBidderAuctionItems", &Script_GetBidderAuctionItems },
    { "GetNumAuctionItems", &Script_GetNumAuctionItems },
    { "GetAuctionItemInfo", &Script_GetAuctionItemInfo },
    { "GetAuctionItemLink", &Script_GetAuctionItemLink },
    { "GetAuctionItemTimeLeft", &Script_GetAuctionItemTimeLeft },
    { "PlaceAuctionBid", &Script_PlaceAuctionBid },
    { "GetAuctionItemClasses", &Script_GetAuctionItemClasses },
    { "GetAuctionItemSubClasses", &Script_GetAuctionItemSubClasses },
    { "GetAuctionInvTypes", &Script_GetAuctionInvTypes },
    { "CanSendAuctionQuery", &Script_CanSendAuctionQuery },
    { "SortAuctionItems", &Script_SortAuctionItems },
    { "SetSelectedAuctionItem", &Script_SetSelectedAuctionItem },
    { "GetSelectedAuctionItem", &Script_GetSelectedAuctionItem },
    { "IsAuctionSortReversed", &Script_IsAuctionSortReversed },
    { "CancelAuction", &Script_CancelAuction },
    { "CanCancelAuction", &Script_CanCancelAuction },
    { "GetAuctionSort", &Script_GetAuctionSort },
    { "SortAuctionClearSort", &Script_SortAuctionClearSort },
    { "SortAuctionSetSort", &Script_SortAuctionSetSort },
    { "SortAuctionApplySort", &Script_SortAuctionApplySort },
    { "CancelSell", &Script_CancelSell },
    { "SetAuctionsTabShowing", &Script_SetAuctionsTabShowing },
};

// VERIFY: 1cd28449 auto
// OFFSET: 0x0059B100
void AuctionHouseRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 1534eb2e auto
// OFFSET: 0x0059B130
void AuctionHouseUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
