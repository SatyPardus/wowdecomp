#include "GuildBankFrame.h"
#include <cstring>
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include <ctime>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Ui/GuildInfo.h"

// STATIC: 0x00C1DC10
WOWGUID CGGuildBankInfo::m_banker;

// STATIC: 0x00C1DC18
WOWGUID CGGuildBankInfo::m_pendingBanker;

// STATIC: 0x00C11458
GuildBankTabInfo CGGuildBankInfo::m_guildBankTabInfo[6];

// STATIC: 0x00C1DC0C
uint8_t CGGuildBankInfo::m_numTabs;

// STATIC: 0x00C1DC0D
uint8_t CGGuildBankInfo::m_currentTab;

// STATIC: 0x00C1DC28
static uint32_t s_pendingItemQueries;

// STATIC: 0x00C1DC2C
static uint32_t s_pendingLogQueries;

// STATIC: 0x00C0F900
GuildBankLogEntry CGGuildBankInfo::m_log[7][25];

// STATIC: 0x00C1DC30
uint32_t CGGuildBankInfo::m_withdrawMoneyRemaining;

// STATIC: 0x00C1DC34
int32_t CGGuildBankInfo::m_remainingWithdrawMoney;

// STATIC: 0x00C1DC20
uint64_t CGGuildBankInfo::m_money;

// STATIC: 0x00C1DBF0
int32_t CGGuildBankInfo::m_numLogEntries[7];

// STATIC: 0x00ACF8C8
uint8_t CGGuildBankInfo::m_logTab = 0xFF;

// STATIC: 0x00C0F8E8
int32_t CGGuildBankInfo::m_tabContentsDirty[6];

// STATIC: 0x00C1DC38
TSGrowableArray<GuildBankSlotInfo> CGGuildBankInfo::m_tabSlots[6];

// VERIFY: a63db2b3 auto
// OFFSET: 0x005A77D0
void CGGuildBankInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_GUILD_BANK_QUERY_RESULTS, Packet_SMSG_GUILD_BANK_LIST, nullptr);
    ClientServices::SetMessageHandler(MSG_GUILD_BANK_LOG_QUERY, Packet_MSG_GUILD_BANK_LOG_QUERY, nullptr);
    ClientServices::SetMessageHandler(MSG_GUILD_BANK_MONEY_WITHDRAWN, Packet_MSG_GUILD_BANK_MONEY_WITHDRAWN, nullptr);
    ClientServices::SetMessageHandler(MSG_QUERY_GUILD_BANK_TEXT, Packet_MSG_QUERY_GUILD_BANK_TEXT, nullptr);
    CGGuildBankInfo::m_numTabs = 0;
    memset(CGGuildBankInfo::m_guildBankTabInfo, 0, sizeof(CGGuildBankInfo::m_guildBankTabInfo));
}

// VERIFY: b01d82c8 auto
// OFFSET: 0x005A4220
void CGGuildBankInfo::UpdateTabInfo(uint8_t tab, const char* name, const char* icon) {
    ScrubString(name, CGGuildBankInfo::m_guildBankTabInfo[tab].m_name, 16, sizeof(CGGuildBankInfo::m_guildBankTabInfo[tab].m_name), 1);
    SStrCopy(CGGuildBankInfo::m_guildBankTabInfo[tab].m_icon, icon, STORM_MAX_STR);
    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_TABS, nullptr);
}

// VERIFY: a782ac08 auto
// OFFSET: 0x005A4270
void CGGuildBankInfo::SetGuildBanker(const WOWGUID& banker) {
    if (banker) {
        CGGameUI::SetInteractTarget(banker);
        CGGuildBankInfo::m_banker = banker;
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKFRAME_OPENED, nullptr);
        return;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKFRAME_CLOSED, nullptr);
    CGGameUI::ClearInteractTarget(CGGuildBankInfo::m_banker, 0);
    CGGuildBankInfo::m_banker = WOWGUID();
    CGGuildBankInfo::m_pendingBanker = WOWGUID();
}

// VERIFY: c27e7cb1 manual
// OFFSET: 0x005A5010
void CGGuildBankInfo::BuyGuildBankTab() {
    if (!CGGuildBankInfo::m_banker) {
        return;
    }

    uint8_t tab = CGGuildBankInfo::m_numTabs;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_BUY_TAB));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(tab);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 09ff696b manual
// OFFSET: 0x005A50C0
void CGGuildBankInfo::SwapItems(uint8_t tab, uint32_t slot, WOWGUID item, WOWGUID container, uint32_t containerSlot, int32_t split) {
    if (!CGGuildBankInfo::m_banker || !item) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!object) {
        return;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return;
    }

    if (object->IsBound()) {
        if (stats->m_bonding == 4) {
            CGGameUI::DisplayError(ERR_GUILD_BANK_QUEST_ITEM);
        } else {
            CGGameUI::DisplayError(ERR_GUILD_BANK_BOUND_ITEM);
        }

        return;
    }

    if ((object->GetItemStaticFlags(0) & 2) || ((object->GetItemStaticFlags(0) & 0x10000) && static_cast<int32_t>(stats->m_duration) > 0) || object->m_item->m_itemFieldDuration || object->IsZoneBound() || object->IsInstanceBound()) {
        CGGameUI::DisplayError(ERR_GUILD_BANK_CONJURED_ITEM);
        return;
    }

    if (object->m_item->m_itemFieldFlags & 8) {
        CGGameUI::DisplayError(ERR_GUILD_BANK_WRAPPED_ITEM);
        return;
    }

    if (container == player->m_obj->m_guid && containerSlot < 23) {
        CGGameUI::DisplayError(ERR_GUILD_BANK_EQUIPPED_ITEM);
        return;
    }

    CDataStore msg;
    int32_t bankItemId = 0;
    GuildBankSlotInfo* bankItem = CGGuildBankInfo::GetGuildBankItem(tab, slot);

    if (bankItem) {
        bankItemId = bankItem->m_itemID;
    }

    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_SWAP_ITEMS));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(tab);
    msg.Put(static_cast<uint8_t>(slot));
    msg.Put(static_cast<uint32_t>(bankItemId));

    if (!(CGGuildInfo::GetGuildBankTabPermissionsByRankID(tab, player->m_player->m_playerGuildrank) & 2)) {
        CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
        return;
    }

    uint8_t bag;
    uint8_t bagSlot;
    player->GetPackAndSlot(object, &bag, &bagSlot);
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(bag);
    msg.Put(bagSlot);
    int32_t count = 0;

    if (object->GetStats() && object->GetStats()->m_stackable) {
        if (!bankItemId) {
            count = split;
        } else if (bankItemId == object->m_obj->m_entryID) {
            count = split;

            if (split) {
                if (split >= object->GetStats()->m_stackable - bankItem->m_count) {
                    count = object->GetStats()->m_stackable - bankItem->m_count;
                }
            } else if (object->m_item->m_itemFieldStackCount < object->GetStats()->m_stackable - bankItem->m_count) {
                count = object->m_item->m_itemFieldStackCount;
            } else {
                count = object->GetStats()->m_stackable - bankItem->m_count;
            }
        }
    }

    msg.Put(static_cast<uint8_t>(0));
    msg.Put(static_cast<uint32_t>(count));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGameUI::ClearCursor(1, 1);
    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
}

// VERIFY: d258067e manual
// OFFSET: 0x005A5410
void CGGuildBankInfo::SwapBankItems(uint8_t tab, uint32_t slot, int32_t itemId, uint32_t virtualSlot, int32_t split) {
    if (!CGGuildBankInfo::m_banker) {
        return;
    }

    int32_t count = 0;

    if (!itemId) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (!(CGGuildInfo::GetGuildBankTabPermissionsByRankID(tab, player->m_player->m_playerGuildrank) & 2)) {
        CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
        return;
    }

    CDataStore msg;
    int32_t bankItemId = 0;
    uint32_t sourceTab = tab;
    GuildBankSlotInfo* bankItem = CGGuildBankInfo::GetGuildBankItem(tab, slot);

    if (bankItem) {
        bankItemId = bankItem->m_itemID;
    }

    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_SWAP_ITEMS));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(static_cast<uint8_t>(1));
    msg.Put(tab);
    msg.Put(static_cast<uint8_t>(slot));
    msg.Put(static_cast<uint32_t>(bankItemId));
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);
    int32_t stackable;

    if (!stats || !(stackable = stats->m_stackable)) {
        if (itemId == bankItemId) {
            CGGameUI::ClearCursor(1, 1);
            CGGameUI::UnlockVirtualItem(bankItemId, slot + 98 * sourceTab, 0);
            return;
        }
    } else if (!bankItemId) {
        count = split;
    } else if (itemId == bankItemId) {
        count = split;

        if (split) {
            if (split >= stackable - bankItem->m_count) {
                count = stackable - bankItem->m_count;
            }
        } else {
            int32_t space = stackable - bankItem->m_count;

            if (CGGameUI::GetVirtualItemCount(itemId, virtualSlot, 0) >= space) {
                count = space;
            } else {
                count = CGGameUI::GetVirtualItemCount(itemId, virtualSlot, 0);
            }
        }
    }

    uint32_t otherTab = virtualSlot / 98;

    if (otherTab != sourceTab && ((bankItemId && !CGGuildInfo::GetNumGuildBankTabItemWithdrawalsByRankID(tab, player->m_player->m_playerGuildrank)) || !CGGuildInfo::GetNumGuildBankTabItemWithdrawalsByRankID(otherTab, player->m_player->m_playerGuildrank))) {
        CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
        return;
    }

    msg.Put(static_cast<uint8_t>(otherTab));
    msg.Put(static_cast<uint8_t>(virtualSlot % 98));
    msg.Put(static_cast<uint32_t>(itemId));
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(static_cast<uint32_t>(count));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGameUI::ClearCursor(1, 1);
    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
}

// VERIFY: ef743f95 auto
// OFFSET: 0x005A56B0
void CGGuildBankInfo::AutoStoreItem(WOWGUID item, WOWGUID container, uint32_t slot) {
    if (CGGuildBankInfo::m_banker) {
        CGGuildBankInfo::SwapItems(CGGuildBankInfo::m_currentTab, 0xFF, item, container, slot, 0);
    }
}

// VERIFY: 640c8ae9 manual
// OFFSET: 0x005A56F0
void CGGuildBankInfo::EnterWorld() {
    for (int32_t i = 0; i < 6; i++) {
        CGGuildBankInfo::m_numLogEntries[i] = 0;
    }

    CGGuildBankInfo::m_numLogEntries[6] = 0;
    CGGuildBankInfo::m_numTabs = 0;
    CGGuildBankInfo::m_banker = WOWGUID{ 0 };
    CGGuildBankInfo::m_pendingBanker = WOWGUID{ 0 };
    CGGuildBankInfo::m_logTab = 0xFF;

    for (int32_t tab = 0; tab < 6; tab++) {
        for (uint32_t i = 0; i < CGGuildBankInfo::m_tabSlots[tab].m_count; i++) {
            CGGuildBankInfo::m_tabSlots[tab].m_data[i].m_itemID = 0;
        }

        CGGuildBankInfo::m_guildBankTabInfo[tab].m_needsFullUpdate = 1;
        CGGuildBankInfo::m_tabContentsDirty[tab] = 1;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_GUILD_BANK_MONEY_WITHDRAWN));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: be4957f6 auto
// OFFSET: 0x005A42E0
void CGGuildBankInfo::GetTabAndSlotFromVirtual(uint32_t virtualSlot, uint32_t* tab, uint32_t* slot) {
    *tab = virtualSlot / 98;
    *slot = virtualSlot % 98;
}

// VERIFY: 0087723a manual
// OFFSET: 0x005A4C10
GuildBankSlotInfo* CGGuildBankInfo::GetGuildBankItem(uint32_t tab, uint32_t slot) {
    if (tab >= 6 || slot >= CGGuildBankInfo::m_tabSlots[tab].m_count) {
        return nullptr;
    }

    GuildBankSlotInfo* info = &CGGuildBankInfo::m_tabSlots[tab].m_data[slot];
    return info->m_itemID ? info : nullptr;
}

// VERIFY: 1ed0b08f auto
// OFFSET: 0x005A4C40
int32_t CGGuildBankInfo::IsLocked(int32_t itemId, uint32_t virtualSlot) {
    uint32_t tab = virtualSlot / 98;
    uint32_t slot = virtualSlot % 98;

    if (tab >= 6 || slot >= CGGuildBankInfo::m_tabSlots[tab].m_count) {
        return 0;
    }

    GuildBankSlotInfo* info = &CGGuildBankInfo::m_tabSlots[tab].m_data[slot];

    if (itemId != info->m_itemID || !info->m_locked) {
        return 0;
    }

    return info->m_locked;
}

// VERIFY: 801e1127 auto
// OFFSET: 0x005A4C90
void CGGuildBankInfo::Lock(int32_t itemId, uint32_t virtualSlot) {
    uint32_t tab = virtualSlot / 98;
    uint32_t slot = virtualSlot % 98;

    if (tab < 6 && slot < CGGuildBankInfo::m_tabSlots[tab].m_count) {
        GuildBankSlotInfo* info = &CGGuildBankInfo::m_tabSlots[tab].m_data[slot];

        if (itemId == info->m_itemID) {
            info->m_locked = 1;
        }
    }
}

// VERIFY: 95bfeaf2 auto
// OFFSET: 0x005A4CE0
void CGGuildBankInfo::Unlock(int32_t itemId, uint32_t virtualSlot) {
    uint32_t tab = virtualSlot / 98;
    uint32_t slot = virtualSlot % 98;

    if (tab < 6 && slot < CGGuildBankInfo::m_tabSlots[tab].m_count) {
        GuildBankSlotInfo* info = &CGGuildBankInfo::m_tabSlots[tab].m_data[slot];

        if (itemId == info->m_itemID) {
            info->m_locked = 0;
        }
    }
}

// VERIFY: 23aee2aa auto
// OFFSET: 0x005A7760
void CGGuildBankInfo::ClearAllTabs() {
    for (uint32_t i = 0; i < 6; i++) {
        CGGuildBankInfo::m_tabSlots[i].Clear();
        CGGuildBankInfo::m_guildBankTabInfo[i].m_needsFullUpdate = 1;
        CGGuildBankInfo::m_tabContentsDirty[i] = 1;
    }

    CGGuildBankInfo::m_numTabs = 0;
    CGGuildBankInfo::m_currentTab = 0;
}

// VERIFY: 23cd4443 auto
// OFFSET: 0x005A7830
void CGGuildBankInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_GUILD_BANK_QUERY_RESULTS);
    ClientServices::ClearMessageHandler(MSG_GUILD_BANK_LOG_QUERY);
    ClientServices::ClearMessageHandler(MSG_GUILD_BANK_MONEY_WITHDRAWN);
    ClientServices::ClearMessageHandler(MSG_QUERY_GUILD_BANK_TEXT);
    CGGuildBankInfo::ClearAllTabs();
}

// VERIFY: b8aeb44a auto
// OFFSET: 0x005A47D0
static void Signal_EVENT_GUILDBANKLOG_UPDATE(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!s_pendingLogQueries || !--s_pendingLogQueries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKLOG_UPDATE, nullptr);
    }
}

// VERIFY: 6502db21 manual
// OFFSET: 0x005A4800
int32_t Packet_MSG_GUILD_BANK_LOG_QUERY(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t now = static_cast<uint32_t>(::time(nullptr));
    uint8_t tab;
    uint8_t count;
    msg->Get(tab);
    msg->Get(count);

    if (s_pendingLogQueries && CGGuildBankInfo::m_logTab < 7) {
        s_pendingLogQueries = 0;

        for (uint32_t i = 0; i < 25; i++) {
            GuildBankLogEntry* entry = &CGGuildBankInfo::m_log[CGGuildBankInfo::m_logTab][i];
            g_nameDBCache.CancelCallback(entry->m_guid.m_guid, Signal_EVENT_GUILDBANKLOG_UPDATE, nullptr);

            if (entry->m_itemID) {
                g_itemDBCache.CancelCallback(entry->m_itemID, Signal_EVENT_GUILDBANKLOG_UPDATE, nullptr);
            }
        }
    }

    CGGuildBankInfo::m_logTab = tab;

    for (uint32_t i = 0; i < count; i++) {
        GuildBankLogEntry* entry = &CGGuildBankInfo::m_log[tab][i];
        uint8_t type;
        msg->Get(type);
        entry->m_type = type;
        msg->Get(entry->m_guid.m_guid);
        entry->m_index = i;

        if (entry->m_type == 4 || entry->m_type == 5 || entry->m_type == 6 || entry->m_type == 8 || entry->m_type == 9) {
            msg->Get(entry->m_money);
        } else {
            msg->Get(*reinterpret_cast<uint32_t*>(&entry->m_itemID));
            msg->Get(entry->m_count);

            if (entry->m_type == 3 || entry->m_type == 7) {
                uint8_t otherTab;
                msg->Get(otherTab);
                entry->m_otherTab = otherTab;
            }

            if (!g_itemDBCache.GetRecord(entry->m_itemID, entry->m_guid, Signal_EVENT_GUILDBANKLOG_UPDATE, nullptr, false)) {
                s_pendingLogQueries++;
            }
        }

        if (!g_nameDBCache.GetRecord(entry->m_guid.m_guid, entry->m_guid, Signal_EVENT_GUILDBANKLOG_UPDATE, nullptr, false)) {
            s_pendingLogQueries++;
        }

        uint32_t age;
        msg->Get(age);
        entry->m_time = now - age;
    }

    CGGuildBankInfo::m_numLogEntries[tab] = count;

    if (CGGuildBankInfo::m_banker && !s_pendingLogQueries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKLOG_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: fa9b79e8 auto
// OFFSET: 0x005A4AB0
int32_t Packet_MSG_GUILD_BANK_MONEY_WITHDRAWN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    msg->Get(reinterpret_cast<uint32_t&>(CGGuildBankInfo::m_remainingWithdrawMoney));
    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_WITHDRAWMONEY, nullptr);

    return 1;
}

// VERIFY: fcedae56 auto
// OFFSET: 0x005A47A0
static void Signal_EVENT_GUILDBANKBAGSLOTS_CHANGED(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!s_pendingItemQueries || !--s_pendingItemQueries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
    }
}

// VERIFY: 259ab13d auto
// OFFSET: 0x005A4AE0
int32_t Packet_MSG_QUERY_GUILD_BANK_TEXT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t tab = 0;
    char text[8192];
    msg->Get(tab);
    msg->GetString(text, sizeof(text));
    ScrubString(text, CGGuildBankInfo::m_guildBankTabInfo[tab].m_text, 2048, sizeof(CGGuildBankInfo::m_guildBankTabInfo[tab].m_text), 0);
    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_TEXT, "%d", tab + 1);
    return 1;
}

// VERIFY: 03f83d73 manual
// OFFSET: 0x005A4B60
void CGGuildBankInfo::ClearGuildTabList(uint32_t tab) {
    if (tab >= 6) {
        return;
    }

    for (uint32_t i = 0; i < CGGuildBankInfo::m_tabSlots[tab].m_count; i++) {
        GuildBankSlotInfo& slot = CGGuildBankInfo::m_tabSlots[tab].m_data[i];
        slot.m_itemID = 0;
        slot.m_randomPropertyID = 0;
        slot.m_suffixFactor = 0;
        slot.m_count = 0;
        slot.m_enchantId = 0;
        slot.m_spellCharges = 0;
        slot.m_enchants[0] = 0;
        slot.m_enchants[1] = 0;
        slot.m_enchants[2] = 0;
        slot.m_locked = 0;
        slot.m_flags = 0;
    }
}

// VERIFY: 4864864b manual
// OFFSET: 0x005A5800
void SendGuildBankQueryTabOpcode(uint8_t tab) {
    if (!CGGuildBankInfo::m_banker) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_QUERY_TAB));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(tab);
    msg.Put(static_cast<uint8_t>(CGGuildBankInfo::m_guildBankTabInfo[tab].m_needsFullUpdate != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1df8dfdf manual
// OFFSET: 0x005A58C0
void CGGuildBankInfo::QueryGuildBankLog(uint8_t tab) {
    if (!CGGuildBankInfo::m_banker) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_GUILD_BANK_LOG_QUERY));
    msg.Put(tab);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: fbc140ba manual
// OFFSET: 0x005A5940
void CGGuildBankInfo::QueryGuildBankText(uint8_t tab) {
    if (!CGGuildBankInfo::m_banker || !CGGuildBankInfo::m_tabContentsDirty[tab]) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_TEXT, "%d", tab + 1);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_QUERY_GUILD_BANK_TEXT));
    msg.Put(tab);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGuildBankInfo::m_tabContentsDirty[tab] = 0;
}

// VERIFY: 1d7a663f manual
// OFFSET: 0x005A5A00
void CGGuildBankInfo::SetTabInfo(uint8_t tab, const char* name, const char* icon) {
    char scrubbed[64];
    ScrubString(name, scrubbed, 16, sizeof(scrubbed), 1);
    int32_t error = ValidateGuildBankTabName(s_locale, scrubbed);

    if (error) {
        CGGameUI::DisplayNameError(error);
        return;
    }

    if (!CGGuildBankInfo::m_banker || tab >= CGGuildBankInfo::m_numTabs) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_UPDATE_TAB));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(tab);
    msg.PutString(scrubbed);
    msg.PutString(icon);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 8bfdc543 manual
// OFFSET: 0x005A5B00
void CGGuildBankInfo::DepositMoney(uint32_t amount) {
    if (!CGGuildBankInfo::m_banker) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_DEPOSIT_MONEY));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(amount);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: db5237e3 manual
// OFFSET: 0x005A5BA0
void CGGuildBankInfo::WithdrawMoney(uint32_t amount) {
    if (!CGGuildBankInfo::m_banker) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_WITHDRAW_MONEY));
    msg.Put(CGGuildBankInfo::m_banker.m_guid);
    msg.Put(amount);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 994e4ef0 manual
// OFFSET: 0x005A5C40
void CGGuildBankInfo::SetGuildBankText(uint8_t tab, const char* text) {
    if (!CGGuildBankInfo::m_banker || tab >= CGGuildBankInfo::m_numTabs) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_SET_TAB_TEXT));
    msg.Put(tab);
    msg.PutString(text);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: bfba15d4 manual
// OFFSET: 0x005A7250
int32_t Packet_SMSG_GUILD_BANK_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t money;
    uint8_t tab;
    uint8_t hasTabInfo;
    msg->Get(money);
    msg->Get(tab);
    msg->Get(CGGuildBankInfo::m_withdrawMoneyRemaining);
    msg->Get(hasTabInfo);

    if (s_pendingItemQueries) {
        s_pendingItemQueries = 0;

        for (uint32_t i = 0; i < CGGuildBankInfo::m_tabSlots[tab].m_count; i++) {
            g_nameDBCache.CancelCallback(*reinterpret_cast<uint64_t*>(&CGGuildBankInfo::m_tabSlots[tab].m_data[i]), Signal_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
        }
    }

    if (money >= 0x7FFFFFFFFFFFFull) {
        money = 0x7FFFFFFFFFFFFull;
    }

    CGGuildBankInfo::m_money = money;

    if (!CGGuildBankInfo::m_tabSlots[tab].m_count) {
        CGGuildBankInfo::m_tabSlots[tab].SetCount(98);
        CGGuildBankInfo::ClearGuildTabList(tab);
    }

    if (hasTabInfo && !tab) {
        msg->Get(CGGuildBankInfo::m_numTabs);

        for (uint32_t i = 0; i < CGGuildBankInfo::m_numTabs; i++) {
            char name[64];
            char icon[256];
            msg->GetString(name, sizeof(name));
            msg->GetString(icon, sizeof(icon));
            ScrubString(name, CGGuildBankInfo::m_guildBankTabInfo[i].m_name, 16, sizeof(name), 1);
            SStrCopy(CGGuildBankInfo::m_guildBankTabInfo[i].m_icon, icon, STORM_MAX_STR);
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_TABS, nullptr);
        }

        CGGuildBankInfo::ClearGuildTabList(tab);
    }

    uint8_t numSlots;
    msg->Get(numSlots);
    int32_t cursorId = 0;
    int32_t cursorSlot = 0;

    if (CGGameUI::m_cursorItemType == 11) {
        CGGameUI::GetCursorVirtualItem(cursorId, cursorSlot);
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    WOWGUID playerGuid = player->m_obj->m_guid;

    for (uint32_t i = 0; i < numSlots; i++) {
        uint8_t slotIndex;
        msg->Get(slotIndex);

        if (cursorId && tab == static_cast<uint32_t>(cursorSlot) / 98 && slotIndex == static_cast<uint32_t>(cursorSlot) % 98) {
            CGGameUI::ClearCursor(1, 1);
            CGGameUI::UnlockVirtualItem(cursorId, cursorSlot, 0);
        }

        msg->Get(reinterpret_cast<uint32_t&>(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_itemID));
        GuildBankSlotInfo* slot = &CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex];

        if (!slot->m_itemID) {
            continue;
        }

        msg->Get(slot->m_flags);
        msg->Get(reinterpret_cast<uint32_t&>(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_randomPropertyID));
        slot = &CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex];

        if (slot->m_randomPropertyID) {
            msg->Get(slot->m_suffixFactor);
        } else {
            slot->m_suffixFactor = 0;
        }

        msg->Get(reinterpret_cast<uint32_t&>(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_count));
        msg->Get(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_enchantId);
        msg->Get(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_spellCharges);
        uint8_t numEnchants;
        msg->Get(numEnchants);
        slot = &CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex];
        slot->m_enchants[0] = 0;
        slot->m_enchants[1] = 0;
        slot->m_enchants[2] = 0;

        for (uint32_t j = 0; j < numEnchants; j++) {
            uint8_t enchantSlot;
            msg->Get(enchantSlot);
            msg->Get(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_enchants[enchantSlot]);
        }

        CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_locked = 0;

        if (!g_itemDBCache.GetRecord(CGGuildBankInfo::m_tabSlots[tab].m_data[slotIndex].m_itemID, playerGuid, Signal_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr, false)) {
            s_pendingItemQueries++;
        }
    }

    CGGuildBankInfo::m_guildBankTabInfo[tab].m_needsFullUpdate = 0;

    if (CGGuildBankInfo::m_banker == WOWGUID()) {
        if (CGGuildBankInfo::m_pendingBanker == WOWGUID()) {
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKFRAME_CLOSED, nullptr);
            CGGameUI::ClearInteractTarget(CGGuildBankInfo::m_banker, 0);
            CGGuildBankInfo::m_banker = WOWGUID();
            CGGuildBankInfo::m_pendingBanker = WOWGUID();
            return 1;
        }

        CGGameUI::SetInteractTarget(CGGuildBankInfo::m_pendingBanker);
        CGGuildBankInfo::m_banker = CGGuildBankInfo::m_pendingBanker;
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKFRAME_OPENED, nullptr);
    } else if (!s_pendingItemQueries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
    }

    return 1;
}

// VERIFY: 9c7c4a73 auto
// OFFSET: 0x005A4310
void CGGuildBankInfo::SetTextUpdate(uint8_t tab, int32_t update) {
    CGGuildBankInfo::m_tabContentsDirty[tab] = update;
}
