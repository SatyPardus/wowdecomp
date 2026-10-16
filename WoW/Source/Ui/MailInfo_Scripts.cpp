#include "MailInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/StationeryRec.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/Rec/PackageRec.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Os/Time.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include <cstdio>
#include "WoW/Common/WowTime.h"
#include "WoW/Source/DB/Rec/MailTemplateRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Calendar.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "Engine/Source/FrameXML/LoadXML.h"

// STATIC: 0x00BEB210
static char s_stationeryTexture[260];

// STATIC: 0x00BEB318
static char s_packageTexture[260];

// VERIFY: 27fef505 manual
// OFFSET: 0x0056D5D0
static int32_t Script_GetSendMailMoney(lua_State* L) {
    lua_pushnumber(L, CGMailInfo::m_sendMoney);
    return 1;
}

// VERIFY: 8948f687 manual
// OFFSET: 0x0056D690
static int32_t Script_GetSendMailCOD(lua_State* L) {
    lua_pushnumber(L, CGMailInfo::m_sendCOD);
    return 1;
}

// VERIFY: 8c50aec0 manual
// OFFSET: 0x0056F350
static int32_t Script_HasNewMail(lua_State* L) {
    if (fabsf(CGMailInfo::m_nextMailTime) >= 2.3841858e-7f) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    return 1;
}

// VERIFY: d43f229a manual
// OFFSET: 0x0056D6D0
static int32_t Script_GetInboxNumItems(lua_State* L) {
    lua_pushnumber(L, CGMailInfo::m_numInboxItems);
    lua_pushnumber(L, CGMailInfo::m_totalInboxItems);
    return 2;
}

// VERIFY: 54d478fd auto
// OFFSET: 0x0056E170
static int32_t Script_GetSelectedStationeryTexture(lua_State* L) {
    const StationeryRec* rec;

    if (CGMailInfo::m_selectedStationery && (rec = g_stationeryDB.GetRecord(CGMailInfo::m_selectedStationery))) {
        lua_pushstring(L, rec->m_texture);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c09e3381 auto
// OFFSET: 0x0056E310
static int32_t Script_SelectPackage(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SelectPackage(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    const PackageRec* rec;

    if (index >= 0 && index < g_packageDB.m_numRecords && (rec = &g_packageDB.m_records[index])) {
        CGMailInfo::m_selectedPackage = rec->m_id;
        return 0;
    }

    CGMailInfo::m_selectedPackage = 0;
    return 0;
}

// VERIFY: 699c569f manual
// OFFSET: 0x0056E440
static int32_t Script_CheckInbox(lua_State* L) {
    if (CGMailInfo::m_commandPending) {
        return 0;
    }

    WOWGUID object = CGMailInfo::m_object;

    if (!object) {
        return 0;
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (CGMailInfo::m_nextUpdateTime && static_cast<int32_t>(now - CGMailInfo::m_nextUpdateTime) < 0) {
        FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_INBOX_UPDATE, nullptr);
        return 0;
    }

    CGMailInfo::m_nextUpdateTime = now + 60000;
    CGMailInfo::m_commandPending = 1;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MAIL_GET_LIST));
    msg.Put(object.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: e89f0c93 auto
// OFFSET: 0x0056EDF0
static int32_t Script_TakeInboxMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TakeInboxMoney(index)");
        return 0;
    }

    if (!CGMailInfo::m_commandPending) {
        WOWGUID object = CGMailInfo::m_object;

        if (object) {
            uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

            if (index < CGMailInfo::m_numInboxItems) {
                MailboxMessageInfo* mail = CGMailInfo::m_inbox.m_data[index];

                if (mail && mail->m_money) {
                    CDataStore msg;
                    msg.Put(static_cast<uint32_t>(CMSG_MAIL_TAKE_MONEY));
                    msg.Put(object.m_guid);
                    msg.Put(static_cast<uint32_t>(mail->m_messageID));
                    msg.Finalize();
                    ClientServices::Send2(&msg);
                    CGMailInfo::m_commandPending = 1;
                }
            }
        }
    }

    return 0;
}

// VERIFY: 0820303b auto
// OFFSET: 0x0056EEF0
static int32_t Script_TakeInboxTextItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TakeInboxTextItem(index)");
        return 0;
    }

    if (!CGMailInfo::m_commandPending) {
        WOWGUID object = CGMailInfo::m_object;

        if (object) {
            uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

            if (index < CGMailInfo::m_numInboxItems) {
                MailboxMessageInfo* mail = CGMailInfo::m_inbox.m_data[index];

                if (mail && (SStrLen(mail->m_body) || mail->m_mailTemplateID) && !(mail->m_flags & 0x4)) {
                    CDataStore msg;
                    msg.Put(static_cast<uint32_t>(CMSG_MAIL_CREATE_TEXT_ITEM));
                    msg.Put(object.m_guid);
                    msg.Put(static_cast<uint32_t>(mail->m_messageID));
                    msg.Finalize();
                    ClientServices::Send2(&msg);
                    CGMailInfo::m_commandPending = 1;
                }
            }
        }
    }

    return 0;
}

// VERIFY: 0438c98b manual
// OFFSET: 0x0056E0E0
static int32_t Script_SelectStationery(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SelectStationery(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGMailInfo::m_numStationeries) {
        CGMailInfo::m_selectedStationery = 0;
    } else {
        CGMailInfo::m_selectedStationery = CGMailInfo::m_stationeries.m_data[index]->m_id;
    }

    return 0;
}

// VERIFY: 437ff54e manual
// OFFSET: 0x0056DF80
static int32_t Script_GetStationeryInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetStationeryInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    StationeryInfo* stationery;
    ItemStats_C* stats;
    WOWGUID none = { 0 };

    if (index < CGMailInfo::m_stationeries.Count() && (stationery = CGMailInfo::m_stationeries.m_data[index]) && (stats = g_itemDBCache.GetRecord(stationery->m_itemID, none, nullptr, nullptr, false))) {
        const char* path = ClientDBStringLookup(3);
        SStrPrintf(s_stationeryTexture, sizeof(s_stationeryTexture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
        lua_pushstring(L, stats->m_name[0]);
        lua_pushstring(L, s_stationeryTexture);

        if (stationery->m_itemGUID) {
            lua_pushnil(L);
        } else {
            lua_pushnumber(L, stats->m_buyPrice);
        }

        return 3;
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

// VERIFY: b60ccfe5 auto
// OFFSET: 0x0056D570
static int32_t Script_SetSendMailShowing(lua_State* L) {
    if (lua_type(L, 1) != LUA_TBOOLEAN) {
        luaL_error(L, "Usage: SetSendMailShowing(bool)");
        return 0;
    }

    CGMailInfo::m_sendMailShowing = lua_toboolean(L, 1);
    return 0;
}

// VERIFY: fddd3d55 auto
// OFFSET: 0x0056E210
static int32_t Script_GetPackageInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPackageInfo(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    PackageRec* rec = index >= 0 && index < g_packageDB.m_numRecords ? &g_packageDB.m_records[index] : nullptr;
    double cost;

    if (rec) {
        const char* path = ClientDBStringLookup(3);
        SStrPrintf(s_packageTexture, sizeof(s_packageTexture), "%s%s", path, *path ? "\\" : "");
        SStrPack(s_packageTexture, rec->m_icon, sizeof(s_packageTexture));
        lua_pushstring(L, rec->m_name);
        lua_pushstring(L, s_packageTexture);
        cost = rec->m_cost;
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        cost = 0.0;
    }

    lua_pushnumber(L, cost);
    return 3;
}

// VERIFY: 4291f14f auto
// OFFSET: 0x0056E1E0
static int32_t Script_GetNumPackages(lua_State* L) {
    lua_pushnumber(L, g_packageDB.m_numRecords);
    return 1;
}

// VERIFY: 12b74c11 auto
// OFFSET: 0x00571B00
static int32_t Script_GetSendMailPrice(lua_State* L) {
    uint32_t price = 30;
    uint32_t itemPrice = 30 * CGMailInfo::GetNumSendItems();

    if (itemPrice >= 30) {
        price = itemPrice;
    }

    if (CGMailInfo::m_selectedStationery) {
        for (uint32_t i = 0; i < CGMailInfo::m_numStationeries; i++) {
            StationeryInfo* stationery = CGMailInfo::m_stationeries.m_data[i];

            if (stationery->m_id == CGMailInfo::m_selectedStationery) {
                if (!stationery->m_itemGUID) {
                    ItemStats_C* stats = g_itemDBCache.GetRecord(stationery->m_itemID, WOWGUID{ 0 }, nullptr, nullptr, false);

                    if (stats) {
                        price += stats->m_buyPrice;
                    }
                }

                break;
            }
        }
    }

    if (CGMailInfo::m_selectedPackage) {
        PackageRec* rec = g_packageDB.GetRecord(CGMailInfo::m_selectedPackage);

        if (rec) {
            price += rec->m_cost;
        }
    }

    lua_pushnumber(L, price + CGMailInfo::m_sendMoney);
    return 1;
}

// STATIC: 0x00ACE610
// VERIFY: 73554391 auto
// OFFSET: 0x0056D5C0
static int32_t Script_ClearSendMail(lua_State* L) {
    CGMailInfo::ClearSendMail(true);
    return 0;
}

// VERIFY: 0dd80945 auto
// OFFSET: 0x0056D610
static int32_t Script_SetSendMailCOD(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetSendMailCOD(amount)");
        return 0;
    }

    double amount = lua_tonumber(L, 1);

    if (CGMailInfo::GetNumSendItems()) {
        CGMailInfo::m_sendCOD = static_cast<uint32_t>(amount);
        FrameScript_SignalEvent(SCRIPT_EVENT_SEND_MAIL_COD_CHANGED, nullptr);
    }

    return 0;
}

// VERIFY: c2469762 manual
// OFFSET: 0x0056E3A0
static int32_t Script_GetSendMailItemLink(lua_State* L) {
    uint32_t index;

    if (lua_isnumber(L, 1)) {
        index = static_cast<int32_t>(lua_tonumber(L, 1) - 1.0);

        if (index >= 16) {
            return 0;
        }
    } else {
        index = 0;
    }

    WOWGUID guid = CGMailInfo::m_sendItems[index].m_guid;

    if (!guid) {
        return 0;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!item) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
    return 1;
}

// STATIC: 0x00BEB420
static char s_headerTexture[260];

// VERIFY: 4a59927a manual
// OFFSET: 0x0056E520
static int32_t Script_GetInboxHeaderInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInboxHeaderInfo(index)");
        return 0;
    }

    lua_checkstack(L, 13);
    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MailboxMessageInfo* message;

    if (index >= CGMailInfo::m_numInboxItems || !(message = CGMailInfo::m_inbox.m_data[index])) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 14;
    }

    uint32_t numAttachments = GetNumAttachments(message);
    MailAttachmentInfo* attachment = numAttachments == 1 ? GetFirstAttachment(message) : nullptr;
    const char* iconPath = ClientDBStringLookup(3);
    SStrPrintf(s_headerTexture, sizeof(s_headerTexture), "%s%s", iconPath, *iconPath ? "\\" : "");
    PackageRec* package = message->m_package ? g_packageDB.GetRecord(message->m_package) : nullptr;

    if (package) {
        SStrPack(s_headerTexture, package->m_icon, sizeof(s_headerTexture));
        lua_pushstring(L, s_headerTexture);
    } else if (numAttachments > 1) {
        SStrPack(s_headerTexture, CGItem_C::GetInventoryArt(7913), sizeof(s_headerTexture));
        lua_pushstring(L, s_headerTexture);
    } else {
        ItemStats_C* stats = nullptr;

        if (attachment && attachment->m_itemID) {
            WOWGUID none = WOWGUID();
            stats = g_itemDBCache.GetRecord(attachment->m_itemID, none, &InboxItemQueryCallback, nullptr, false);
        }

        if (stats) {
            SStrPack(s_headerTexture, CGItem_C::GetInventoryArt(stats->m_displayId), sizeof(s_headerTexture));
            lua_pushstring(L, s_headerTexture);
        } else {
            lua_pushnil(L);
        }
    }

    SStrPrintf(s_headerTexture, sizeof(s_headerTexture), "%s%s", iconPath, *iconPath ? "\\" : "");
    StationeryRec* stationery = g_stationeryDB.GetRecord(message->m_stationery);
    ItemStats_C* stationeryStats = nullptr;

    if (stationery) {
        WOWGUID none = WOWGUID();
        stationeryStats = g_itemDBCache.GetRecord(stationery->m_itemID, none, &InboxItemQueryCallback, nullptr, false);
    }

    if (stationeryStats) {
        SStrPack(s_headerTexture, CGItem_C::GetInventoryArt(stationeryStats->m_displayId), sizeof(s_headerTexture));
        lua_pushstring(L, s_headerTexture);
    } else {
        lua_pushnil(L);
    }

    bool isGM = message->m_stationery == 61;
    lua_pushstring(L, GetSenderName(message->m_senderEntry, message->m_messageType, message->m_sender, message->m_stationery));

    if (message->m_flags & 0x8) {
        char subject[512];
        SStrPrintf(subject, sizeof(subject), FrameScript_GetText("COD_PAYMENT", -1, GENDER_NOT_APPLICABLE), message->m_subject);
        lua_pushstring(L, subject);
    } else {
        lua_pushstring(L, message->m_subject);
    }

    lua_pushnumber(L, message->m_money);
    lua_pushnumber(L, message->m_cod);
    lua_pushnumber(L, message->m_expireTime);

    if (GetNumAttachments(message)) {
        lua_pushnumber(L, GetNumAttachments(message));
    } else {
        lua_pushnil(L);
    }

    if (message->m_flags & 0x1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if ((message->m_flags & 0x2) || (message->m_flags & 0x200)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (message->m_flags & 0x4) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (message->m_messageType || isGM) {
        lua_pushnil(L);

        if (isGM) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnumber(L, 1.0);
        lua_pushnil(L);
    }

    if (numAttachments == 1 && attachment) {
        lua_pushnumber(L, static_cast<int32_t>(attachment->m_count));
    } else {
        lua_pushnil(L);
    }

    return 14;
}

// VERIFY: 0bc89b80 manual
// OFFSET: 0x0056E9B0
static int32_t Script_GetInboxInvoiceInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInboxInvoiceInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MailboxMessageInfo* message;

    if (index >= CGMailInfo::m_numInboxItems || !(message = CGMailInfo::m_inbox.m_data[index]) || !SStrLen(message->m_body) || !message->m_invoiceItemID) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 7;
    }

    char itemName[256];
    CGItem_C::BuildItemName(itemName, sizeof(itemName), message->m_invoiceItemID, message->m_invoiceRandomPropertiesID);
    WOWGUID player = WOWGUID();
    int32_t bid = 0;
    int32_t buyout = 0;
    int32_t deposit = 0;
    int32_t consignment = 0;
    int32_t moneyDelay = 0;
    uint32_t etaTime = 0;

    switch (message->m_invoiceType) {
        case 2:
            sscanf(message->m_body, "%16I64X:%d:%d:%d:%d", &player.m_guid, &bid, &buyout, &deposit, &consignment);
            break;

        case 6:
            sscanf(message->m_body, "%16I64X:%d:%d:%d:%d:%d:%d", &player.m_guid, &bid, &buyout, &deposit, &consignment, &moneyDelay, &etaTime);
            break;

        case 1:
            sscanf(message->m_body, "%16I64X:%d:%d", &player.m_guid, &bid, &buyout);
            break;
    }

    WowTime eta;
    WowTime::WowDecodeTime(etaTime, &eta);

    if (message->m_invoiceType == 2) {
        lua_pushstring(L, "seller");
    } else if (message->m_invoiceType == 6) {
        lua_pushstring(L, "seller_temp_invoice");
    } else {
        lua_pushstring(L, "buyer");
    }

    lua_pushstring(L, itemName);
    NameCache* name = g_nameDBCache.GetRecord(player.m_guid, player, &InboxItemQueryCallback, nullptr, false);
    lua_pushstring(L, name ? name->m_name : "");
    lua_pushnumber(L, bid);
    lua_pushnumber(L, buyout);
    lua_pushnumber(L, deposit);
    lua_pushnumber(L, consignment);
    lua_pushnumber(L, moneyDelay);
    lua_pushnumber(L, eta.m_hour);
    lua_pushnumber(L, eta.m_minute);
    return 10;
}

// VERIFY: 5d367394 auto
// OFFSET: 0x0056ECA0
static int32_t Script_GetInboxItemLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInboxItemLink(messageIndex, attachIndex)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t attachIndex = lua_isnumber(L, 2) ? static_cast<int32_t>(lua_tonumber(L, 2) - 1.0) : -1;

    if (index >= CGMailInfo::m_numInboxItems) {
        return 0;
    }

    MailboxMessageInfo* message = CGMailInfo::m_inbox.m_data[index];

    if (!message) {
        return 0;
    }

    MailAttachmentInfo* attachment = GetAttachment(message, attachIndex);

    if (!attachment || !attachment->m_itemID) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(attachment->m_itemID, none, &InboxItemQueryCallback, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::CreateItemLink(attachment->m_itemID, stats->m_quality, attachment->m_enchantIDs[0], &attachment->m_enchantIDs[2], attachment->m_randomPropertiesID, attachment->m_suffixFactor));
    return 1;
}

// VERIFY: 9102324a auto
// OFFSET: 0x0056F000
static int32_t Script_ReturnInboxItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ReturnInboxItem(index)");
        return 0;
    }

    if (CGMailInfo::m_commandPending) {
        return 0;
    }

    WOWGUID mailbox = CGMailInfo::m_object;

    if (!mailbox) {
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGMailInfo::m_numInboxItems) {
        return 0;
    }

    MailboxMessageInfo* message = CGMailInfo::m_inbox.m_data[index];

    if (!message || message->m_deletePending || message->m_messageType || (!GetNumAttachments(message) && !message->m_money) || (message->m_flags & 0x20A)) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MAIL_RETURN_TO_SENDER));
    msg.Put(mailbox.m_guid);
    msg.Put(static_cast<uint32_t>(message->m_messageID));
    msg.Put(message->m_sender.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    message->m_deletePending = 1;
    CGMailInfo::m_commandPending = 1;
    return 0;
}

// VERIFY: 4de0ffc3 auto
// OFFSET: 0x0056F140
static int32_t Script_DeleteInboxItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: DeleteInboxItem(index)");
        return 0;
    }

    if (CGMailInfo::m_commandPending) {
        return 0;
    }

    WOWGUID mailbox = CGMailInfo::m_object;

    if (!mailbox) {
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGMailInfo::m_numInboxItems) {
        return 0;
    }

    MailboxMessageInfo* message = CGMailInfo::m_inbox.m_data[index];

    if (!message || message->m_deletePending) {
        return 0;
    }

    if (!(message->m_flags & 0x20A) && !message->m_messageType && (GetNumAttachments(message) || message->m_money)) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MAIL_DELETE));
    msg.Put(mailbox.m_guid);
    msg.Put(static_cast<uint32_t>(message->m_messageID));
    msg.Put(static_cast<uint32_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    message->m_deletePending = 1;
    CGMailInfo::m_commandPending = 1;
    return 0;
}

// VERIFY: bbafceb6 auto
// OFFSET: 0x0056F280
static int32_t Script_InboxItemCanDelete(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: InboxItemCanDelete(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MailboxMessageInfo* message;

    if (index < CGMailInfo::m_numInboxItems && (message = CGMailInfo::m_inbox.m_data[index]) && ((message->m_flags & 0x20A) || message->m_messageType || (!GetNumAttachments(message) && !message->m_money))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: bde1d865 auto
// OFFSET: 0x0056F3A0
static int32_t Script_GetLatestThreeSenders(lua_State* L) {
    int32_t count = 0;

    for (uint32_t i = 0; i < 3; i++) {
        MailSenderInfo& sender = s_recentSenders[i];

        if ((sender.m_guid || sender.m_entry) && sender.m_timeLeft <= 0.0f) {
            lua_pushstring(L, GetSenderName(sender.m_entry, sender.m_messageType, sender.m_guid, sender.m_stationery));
            count++;
        }
    }

    return count;
}

// VERIFY: f93ff448 manual
// OFFSET: 0x0056F410
static int32_t Script_RespondMailLockSendItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: RespondMailLockSendItem(slot, keepItem)");
        return 0;
    }

    uint32_t slot = lua_tointeger(L, 1) - 1;
    int32_t keepItem = StringToBOOL(L, 2, 0);

    if (!CGMailInfo::m_lockSendItems) {
        return 0;
    }

    CGMailInfo::m_lockSendItems = 0;

    if (keepItem) {
        FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_UNLOCK_SEND_ITEMS, nullptr);
        return 0;
    }

    WOWGUID item;
    WOWGUID container;
    uint8_t itemSlot;
    CGMailInfo::GetSendItem(slot, item, container, itemSlot);
    CGMailInfo::ClearSendItem(slot);

    if (item) {
        CGGameUI::UnlockItem(item);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_SEND_INFO_UPDATE, nullptr);
    FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_UNLOCK_SEND_ITEMS, nullptr);
    return 0;
}

// VERIFY: 4f7f7cdf manual
// OFFSET: 0x00570550
static int32_t Script_ClickSendMailItemButton(lua_State* L) {
    uint32_t index = lua_isnumber(L, 1) ? static_cast<int32_t>(lua_tonumber(L, 1) - 1.0) : -1;

    if (!lua_toboolean(L, 2)) {
        WOWGUID cursorItem;
        WOWGUID cursorContainer;
        uint32_t cursorSlot;
        CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
        WOWGUID item;
        WOWGUID container;
        uint8_t slot;
        CGMailInfo::GetSendItem(index, item, container, slot);

        if (item == cursorItem) {
            CGGameUI::ClearCursor(1, 1);
            return 0;
        }

        if (cursorItem) {
            if (!CGMailInfo::AddItemToOutgoingMail(index, cursorItem, cursorContainer, static_cast<uint8_t>(cursorSlot))) {
                return 0;
            }
        } else {
            CGMailInfo::ClearSendItem(index);
        }

        CGGameUI::SetCursorItem(item, container, slot, 0, 0, 1);
        FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_SEND_INFO_UPDATE, nullptr);
        return 0;
    }

    if (index >= 16) {
        return 0;
    }

    WOWGUID item = CGMailInfo::m_sendItems[index].m_guid;

    if (!item) {
        return 0;
    }

    CGMailInfo::ClearSendItem(index);
    CGGameUI::UnlockItem(item);
    FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_SEND_INFO_UPDATE, nullptr);
    return 0;
}

// VERIFY: f77670df auto
// OFFSET: 0x005706C0
static int32_t Script_SetSendMailMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetSendMailMoney(amount)");
        return 0;
    }

    if (CGMailInfo::SetSendMoney(static_cast<int32_t>(lua_tonumber(L, 1)))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 987d2cb2 manual
// OFFSET: 0x00570750
static int32_t Script_GetSendMailItem(lua_State* L) {
    uint32_t index;

    if (lua_isnumber(L, 1)) {
        index = static_cast<int32_t>(lua_tonumber(L, 1) - 1.0);

        if (index >= 16) {
            lua_pushnil(L);
            lua_pushnil(L);
            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            return 4;
        }
    } else {
        index = 0;
    }

    WOWGUID guid = CGMailInfo::m_sendItems[index].m_guid;
    CGItem_C* item = guid ? static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__)) : nullptr;
    ItemStats_C* stats = nullptr;

    if (item) {
        WOWGUID none = WOWGUID();
        stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, &SendItemQueryCallback, nullptr, false);
    }

    if (!stats) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 4;
    }

    char name[256];
    item->GetFullItemName(name, sizeof(name));
    lua_pushstring(L, name);
    const char* iconPath = ClientDBStringLookup(3);
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", iconPath, *iconPath ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);
    lua_pushnumber(L, item->m_item->m_itemFieldStackCount);
    lua_pushnumber(L, stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);
    return 4;
}

// VERIFY: 6c063b68 auto
// OFFSET: 0x00570910
static int32_t Script_SendMail(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SendMail(target, subject, body)");
        return 0;
    }

    if (CGMailInfo::m_commandPending) {
        return 0;
    }

    WOWGUID mailbox = CGMailInfo::m_object;

    if (!mailbox || CGMailInfo::m_sendPending) {
        return 0;
    }

    const char* target = lua_tolstring(L, 1, nullptr);

    if (!*target || !lua_isstring(L, 2)) {
        return 0;
    }

    const char* subject = lua_tolstring(L, 2, nullptr);

    if (!*subject) {
        return 0;
    }

    const char* body = lua_isstring(L, 3) ? lua_tolstring(L, 3, nullptr) : "";
    int32_t stationery = CGMailInfo::m_selectedStationery;

    if (!stationery) {
        return 0;
    }

    uint32_t count = 0;

    for (uint32_t i = 0; i < 16; i++) {
        WOWGUID guid = CGMailInfo::GetSendItemGuid(i);

        if (!guid) {
            continue;
        }

        count++;
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!item || item->m_item->m_itemFieldOwner != ClntObjMgrGetActivePlayer()) {
            CGGameUI::DisplayError(static_cast<UIErrors>(GetDisplayErrorByInventoryError(23)));
            CGMailInfo::ClearSendItem(i);
            FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_SEND_INFO_UPDATE, nullptr);
            return 0;
        }
    }

    int32_t package = count ? CGMailInfo::m_selectedPackage : 0;
    uint32_t money = CGMailInfo::m_sendMoney;
    uint32_t cod = CGMailInfo::m_sendCOD;

    if (money ? cod != 0 : (cod && !count)) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SEND_MAIL));
    msg.Put(mailbox.m_guid);
    msg.PutString(target);
    msg.PutString(subject);
    msg.PutString(body);
    msg.Put(static_cast<uint32_t>(stationery));
    msg.Put(static_cast<uint32_t>(package));
    msg.Put(static_cast<uint8_t>(count));

    for (uint32_t i = 0; i < 16; i++) {
        WOWGUID guid = CGMailInfo::GetSendItemGuid(i);

        if (guid) {
            msg.Put(static_cast<uint8_t>(i));
            msg.Put(guid.m_guid);
        }
    }

    msg.Put(money);
    msg.Put(cod);
    msg.Put(static_cast<uint64_t>(0));
    msg.Put(static_cast<uint8_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGMailInfo::m_sendPending = 1;
    CGMailInfo::m_commandPending = 1;
    return 0;
}

// VERIFY: a7b3383e manual
// OFFSET: 0x00570BD0
static int32_t Script_GetInboxText(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInboxText(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    MailboxMessageInfo* message;

    if (index >= CGMailInfo::m_numInboxItems || !(message = CGMailInfo::m_inbox.m_data[index])) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 4;
    }

    CGMailInfo::MarkAsRead(index);
    char buffer[8000];
    char text[8000];

    if (!SStrLen(message->m_body)) {
        MailTemplateRec* mailTemplate = message->m_mailTemplateID ? g_mailTemplateDB.GetRecord(message->m_mailTemplateID) : nullptr;

        if (mailTemplate) {
            WOWGUID player = ClntObjMgrGetActivePlayer();
            QuestParserParseText(mailTemplate->m_body, buffer, sizeof(buffer), player, nullptr, 0);
            lua_pushstring(L, buffer);
        } else {
            lua_pushnil(L);
        }
    } else {
        switch (message->m_messageType) {
            case 2:
                lua_pushnil(L);
                break;

            case 0:
                SStrCopy(text, message->m_body, sizeof(text));
                ProfanityFilter(text, false, false);
                lua_pushstring(L, text);
                break;

            case 5: {
                CGCalendar::GetDateTimeText(buffer, sizeof(buffer), SStrToUnsigned(message->m_body));
                const char* format = FrameScript_GetText((message->m_flags & 0x100) ? "CALENDAR_INVITE_REMOVED_MAIL_BODY" : "CALENDAR_EVENT_REMOVED_MAIL_BODY", -1, GENDER_NOT_APPLICABLE);

                if (!format || !*format) {
                    text[0] = '\0';
                } else {
                    SStrPrintf(text, sizeof(text), format, GetSenderName(message->m_senderEntry, message->m_messageType, message->m_sender, message->m_stationery), message->m_invoiceSubject, buffer);
                }

                lua_pushstring(L, text);
                break;
            }

            default: {
                WOWGUID player = ClntObjMgrGetActivePlayer();
                QuestParserParseText(message->m_body, text, sizeof(text), player, nullptr, 0);
                lua_pushstring(L, text);
                break;
            }
        }
    }

    StationeryRec* stationery = g_stationeryDB.GetRecord(message->m_stationery);

    if (stationery) {
        lua_pushstring(L, stationery->m_texture);
    } else {
        lua_pushnil(L);
    }

    if ((!SStrLen(message->m_body) && !message->m_mailTemplateID) || (message->m_flags & 0x4) || message->m_stationery == 61 || message->m_messageType == 5 || (message->m_messageType == 2 && !message->m_invoiceItemID)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    if (message->m_invoiceItemID) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: af16e1e0 manual
// OFFSET: 0x00570F10
static int32_t Script_GetInboxItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInboxItem(messageIndex, attachIndex)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t attachIndex = lua_isnumber(L, 2) ? static_cast<int32_t>(lua_tonumber(L, 2) - 1.0) : -1;
    MailAttachmentInfo* attachment = nullptr;

    if (index < CGMailInfo::m_numInboxItems) {
        MailboxMessageInfo* message = CGMailInfo::m_inbox.m_data[index];

        if (message) {
            attachment = GetAttachment(message, attachIndex);
        }
    }

    if (!attachment || !attachment->m_itemID) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        return 5;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(attachment->m_itemID, none, &InboxItemQueryCallback, nullptr, false);
    char texture[260];

    if (!stats) {
        lua_pushstring(L, FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE));
        const char* iconPath = ClientDBStringLookup(3);
        SStrPrintf(texture, sizeof(texture), "%s%s%s", iconPath, *iconPath ? "\\" : "", "INV_Misc_QuestionMark");
        lua_pushstring(L, texture);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        return 5;
    }

    char name[256];
    CGItem_C::BuildItemName(name, sizeof(name), attachment->m_itemID, attachment->m_randomPropertiesID);
    lua_pushstring(L, name);
    const char* iconPath = ClientDBStringLookup(3);
    SStrPrintf(texture, sizeof(texture), "%s%s%s", iconPath, *iconPath ? "\\" : "", CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);
    lua_pushnumber(L, static_cast<int32_t>(attachment->m_count));
    lua_pushnumber(L, stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t error;

    if (!player || player->CanUseItem(stats, error)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 5;
}

// VERIFY: 46d21530 auto
// OFFSET: 0x005711C0
static int32_t Script_TakeInboxItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TakeInboxItem(messageIndex, attachIndex)");
        return 0;
    }

    if (CGMailInfo::m_commandPending) {
        return 0;
    }

    WOWGUID mailbox = CGMailInfo::m_object;

    if (!mailbox) {
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGMailInfo::m_numInboxItems) {
        return 0;
    }

    MailboxMessageInfo* message = CGMailInfo::m_inbox.m_data[index];

    if (!message) {
        return 0;
    }

    uint32_t attachIndex = lua_isnumber(L, 2) ? static_cast<int32_t>(lua_tonumber(L, 2)) - 1 : -1;
    MailAttachmentInfo* attachment = GetAttachment(message, attachIndex);

    if (!attachment || !attachment->m_itemID) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->GetCoinage() < static_cast<uint32_t>(message->m_cod)) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MAIL_TAKE_ITEM));
    msg.Put(mailbox.m_guid);
    msg.Put(static_cast<uint32_t>(message->m_messageID));
    msg.Put(attachment->m_itemGuid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGMailInfo::m_commandPending = 1;
    return 0;
}

// VERIFY: d282c11a auto
// OFFSET: 0x00571350
static int32_t Script_ComplainInboxItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ComplainInboxItem(index)");
        return 0;
    }

    CGMailInfo::Complain(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: cf2ffa69 auto
// OFFSET: 0x005713C0
static int32_t Script_CanComplainInboxItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: InboxItemCanComplain(index)");
        return 0;
    }

    if (CGMailInfo::CanComplain(static_cast<int32_t>(lua_tonumber(L, 1)) - 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e42e96ea auto
// OFFSET: 0x00571450
static int32_t Script_AutoLootMailItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: AutoLootMailItem(index)");
        return 0;
    }

    if (!CGMailInfo::m_commandPending && CGMailInfo::m_object) {
        CGMailInfo::AutoLoot(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);
    }

    return 0;
}

// VERIFY: 3ad96506 auto
// OFFSET: 0x00571A40
static int32_t Script_CloseMail(lua_State* L) {
    CGMailInfo::ClearSendMail(false);

    if (CGMailInfo::m_object) {
        CGGameUI::ClearInteractTarget(CGMailInfo::m_object, 0);
        CGMailInfo::m_object = WOWGUID();
        CGMailInfo::m_autoLooting = nullptr;
        FrameScript_SignalEvent(SCRIPT_EVENT_MAIL_CLOSED, nullptr);

        if (CGMailInfo::m_doPendingQuery) {
            CGMailInfo::QueryNextMailTime();
        }
    }

    return 0;
}

// VERIFY: 0b1d4227 auto
// OFFSET: 0x00571AB0
static int32_t Script_GetNumStationeries(lua_State* L) {
    if (!CGMailInfo::m_pendingStationeryCallbacks) {
        CGMailInfo::UpdateStationeryList();
    }

    lua_pushnumber(L, CGMailInfo::m_numStationeries);
    return 1;
}

static FrameScript_Method s_ScriptFunctions[] = {
    { "CloseMail", &Script_CloseMail },
    { "ClearSendMail", &Script_ClearSendMail },
    { "ClickSendMailItemButton", &Script_ClickSendMailItemButton },
    { "SetSendMailMoney", &Script_SetSendMailMoney },
    { "GetSendMailMoney", &Script_GetSendMailMoney },
    { "SetSendMailCOD", &Script_SetSendMailCOD },
    { "GetSendMailCOD", &Script_GetSendMailCOD },
    { "GetNumStationeries", &Script_GetNumStationeries },
    { "GetStationeryInfo", &Script_GetStationeryInfo },
    { "SelectStationery", &Script_SelectStationery },
    { "GetSelectedStationeryTexture", &Script_GetSelectedStationeryTexture },
    { "GetNumPackages", &Script_GetNumPackages },
    { "GetPackageInfo", &Script_GetPackageInfo },
    { "SelectPackage", &Script_SelectPackage },
    { "GetSendMailItem", &Script_GetSendMailItem },
    { "GetSendMailItemLink", &Script_GetSendMailItemLink },
    { "GetSendMailPrice", &Script_GetSendMailPrice },
    { "SendMail", &Script_SendMail },
    { "CheckInbox", &Script_CheckInbox },
    { "GetInboxNumItems", &Script_GetInboxNumItems },
    { "GetInboxHeaderInfo", &Script_GetInboxHeaderInfo },
    { "GetInboxText", &Script_GetInboxText },
    { "GetInboxInvoiceInfo", &Script_GetInboxInvoiceInfo },
    { "GetInboxItem", &Script_GetInboxItem },
    { "GetInboxItemLink", &Script_GetInboxItemLink },
    { "TakeInboxMoney", &Script_TakeInboxMoney },
    { "TakeInboxItem", &Script_TakeInboxItem },
    { "TakeInboxTextItem", &Script_TakeInboxTextItem },
    { "ReturnInboxItem", &Script_ReturnInboxItem },
    { "DeleteInboxItem", &Script_DeleteInboxItem },
    { "InboxItemCanDelete", &Script_InboxItemCanDelete },
    { "HasNewMail", &Script_HasNewMail },
    { "ComplainInboxItem", &Script_ComplainInboxItem },
    { "CanComplainInboxItem", &Script_CanComplainInboxItem },
    { "GetLatestThreeSenders", &Script_GetLatestThreeSenders },
    { "SetSendMailShowing", &Script_SetSendMailShowing },
    { "AutoLootMailItem", &Script_AutoLootMailItem },
    { "RespondMailLockSendItem", &Script_RespondMailLockSendItem },
};

// VERIFY: d763d4b6 auto
// OFFSET: 0x0056D730
void MailInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 6a19438b auto
// OFFSET: 0x0056D760
void MailInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
