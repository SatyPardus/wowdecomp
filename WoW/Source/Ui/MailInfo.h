#ifndef WOW_SOURCE_UI_MAILINFO_H
#define WOW_SOURCE_UI_MAILINFO_H

#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
struct lua_State;

struct MailSenderInfo {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ int32_t m_entry;
    /* 000C */ int32_t m_messageType;
    /* 0010 */ int32_t m_stationery;
    /* 0014 */ float m_timeLeft;
};

struct StationeryInfo {
    /* 0000 */ int32_t m_id;
    /* 0004 */ int32_t m_itemID;
    /* 0008 */ int32_t m_isDefault;
    /* 000C */ int32_t unused_000C;
    /* 0010 */ WOWGUID m_itemGUID;
};

struct MailAttachmentInfo {
    /* 0000 */ uint32_t m_itemGuid;
    /* 0004 */ int32_t m_itemID;
    /* 0008 */ int32_t m_randomPropertiesID;
    /* 000C */ uint32_t m_suffixFactor;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t m_count;
    /* 0018 */ int32_t m_spellCharges;
    /* 001C */ int32_t m_enchantIDs[7];
    /* 0038 */ uint32_t m_enchantDurations[7];
    /* 0054 */ int32_t m_enchantCharges[7];
    /* 0070 */ uint32_t m_maxDurability;
    /* 0074 */ uint32_t m_durability;
    /* 0078 */ uint32_t unk_0078;
};

struct MailboxMessageInfo {
    /* 0000 */ int32_t m_messageID;
    /* 0004 */ int32_t m_messageType;
    /* 0008 */ WOWGUID m_sender;
    /* 0010 */ int32_t m_senderEntry;
    /* 0014 */ char m_subject[256];
    /* 0114 */ char m_body[8000];
    /* 2054 */ int32_t m_package;
    /* 2058 */ int32_t m_stationery;
    /* 205C */ int32_t m_money;
    /* 2060 */ int32_t m_cod;
    /* 2064 */ uint32_t m_flags;
    /* 2068 */ float m_expireTime;
    /* 206C */ char m_invoiceSubject[256];
    /* 216C */ int32_t m_invoiceItemID;
    /* 2170 */ int32_t m_invoiceRandomPropertiesID;
    /* 2174 */ int32_t m_invoiceType;
    /* 2178 */ int32_t m_mailTemplateID;
    /* 217C */ uint32_t m_invoiceValue;
    /* 2180 */ MailAttachmentInfo m_attachments[16];
    /* 2940 */ uint32_t m_deletePending;
    /* 2944 */ uint32_t unused_2944;
};

struct SendMailItem {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ WOWGUID m_container;
    /* 0010 */ uint8_t m_slot;
    /* 0011 */ uint8_t unused_0011[7];
};

class CGMailInfo {
    public:
    static SendMailItem m_sendItems[16];
    static uint8_t m_lockSendItems;
    static uint32_t m_sendPending;
    static void ClearSendMail(bool signal);
    static bool IsSendItem(WOWGUID item);
    static void CloseMail(bool leavingWorld);
    static uint8_t m_sendMailShowing;
    static bool AddItemToOutgoingMail(uint32_t index, WOWGUID item, WOWGUID container, uint8_t slot);
    static TSFixedArray<StationeryInfo*> m_stationeries;
    static TSGrowableArray<MailboxMessageInfo*> m_inbox;
    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static void LeaveWorld();
    static void UpdatePendingMail(float elapsed);
    static uint32_t m_sendMoney;
    static uint32_t m_sendCOD;
    static float m_nextMailTime;
    static uint32_t m_numInboxItems;
    static uint32_t m_totalInboxItems;
    static int32_t m_selectedStationery;
    static int32_t m_selectedPackage;
    static WOWGUID m_object;
    static uint32_t m_nextUpdateTime;
    static int32_t m_commandPending;
    static uint32_t m_numStationeries;
    static uint32_t m_pendingStationeryCallbacks;
    static int32_t m_doPendingQuery;
    static MailboxMessageInfo* m_autoLooting;
    static void SetMailCopied(uint32_t messageID);
    static void RemoveMail(uint32_t messageID);
    static void ParseInboxSubject(MailboxMessageInfo* message);
    static void ParseInboxSubject(uint32_t messageID);
    static void RemoveMoney(uint32_t messageID);
    static void RemoveItem(uint32_t messageID, uint32_t itemGuid);
    static void ProcessAutoLootAttachments(MailboxMessageInfo* message);
    static void ReceivedMail(float delay);

    static uint32_t GetNumSendItems();
    static void SetSendItem(uint32_t index, WOWGUID item, WOWGUID container, uint8_t slot);
    static void GetSendItem(uint32_t index, WOWGUID& item, WOWGUID& container, uint8_t& slot);
    static WOWGUID GetSendItemGuid(uint32_t index);
    static void ClearSendItem(uint32_t index);
    static MailboxMessageInfo* GetInboxItem(uint32_t index);
    static void UpdateRecentSenders();
    static bool CanComplain(uint32_t index);
    static void Complain(uint32_t index);
    static int32_t SetSendMoney(uint32_t money);
    static void MarkAsRead(uint32_t index);
    static void AutoLoot(uint32_t index);
    static void QueryNextMailTime();
    static void UpdateStationeryList();
};

void StationeryItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success);

void MailInfoRegisterScriptFunctions();
void MailInfoUnregisterScriptFunctions();

int32_t Packet_SMSG_SEND_MAIL_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_MAIL_LIST_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_MSG_QUERY_NEXT_MAIL_TIME(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_RECEIVED_MAIL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_SHOW_MAILBOX(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

extern MailSenderInfo s_recentSenders[3];

MailAttachmentInfo* GetFirstAttachment(MailboxMessageInfo* message);
MailAttachmentInfo* GetAttachment(MailboxMessageInfo* message, uint32_t index);
int32_t GetNumAttachments(const MailboxMessageInfo* message);
const char* GetSenderName(int32_t entry, int32_t type, WOWGUID guid, int32_t stationery);
void SendItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void InboxItemQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);

#endif
