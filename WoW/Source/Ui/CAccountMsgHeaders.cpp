#include "CAccountMsgHeaders.h"
#include "Engine/Source/Base/DynamicString.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00BEA750
CAccountMsgHeaders g_accountMsgHeaders;

// VERIFY: 970fc24a auto
// OFFSET: 0x00427A90
static bool AccountMsgReadCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
    return false;
}

// VERIFY: dc5c5838 auto
// OFFSET: 0x0054FB40
void CAccountMsgHeaders::_SendFailureEvent() {
    this->m_iterator.m_node = nullptr;
    this->m_count = 0;
}

// VERIFY: da97ce32 manual
// OFFSET: 0x0054FB50
const char* CAccountMsgHeaders::GetHeaderSubject(int32_t index) {
    if (this->m_state == 1 || index < 0 || index >= this->m_count) {
        return nullptr;
    }

    this->m_iterator.GoTo(index);
    CKBIterator subject = this->m_iterator;
    subject.GoToChild("subject");
    return this->_GetBodyString(subject.m_node);
}

// VERIFY: 40ef5b47 manual
// OFFSET: 0x0054FBB0
int32_t CAccountMsgHeaders::GetHeaderPriority(int32_t index) {
    if (this->m_state == 1 || index < 0 || index >= this->m_count) {
        return 0;
    }

    this->m_iterator.GoTo(index);
    return this->_GetInt(this->m_iterator.m_node, "priority");
}

// VERIFY: 0b60f3e8 auto
// OFFSET: 0x0054FBF0
int32_t CAccountMsgHeaders::GetMessageId(int32_t index) {
    if (this->m_state == 1 || index < 0 || index >= this->m_count) {
        return 0;
    }

    if (!this->m_iterator.m_node) {
        this->_GetRoot(this->m_iterator);
        this->m_iterator.GoToFirstChild();
    }

    this->m_iterator.GoTo(index);
    return this->_GetInt(this->m_iterator.m_node, "id");
}

// VERIFY: 2c3986b9 manual
// OFFSET: 0x005500D0
int32_t CAccountMsgHeaders::GetNumUnreadUrgentMsgs() {
    int32_t count = 0;

    for (int32_t i = 0; i < this->m_count; i++) {
        if (!this->m_opened.m_data[i] && this->GetHeaderPriority(i) >= 3) {
            count++;
        }
    }

    return count;
}

// VERIFY: 4579967f manual
// OFFSET: 0x00550140
int32_t CAccountMsgHeaders::GetIndexHighestPriorityUnreadMsg() {
    if (this->m_state == 1) {
        return 0;
    }

    int32_t index = 0;

    for (int32_t i = 0; i < this->m_count; i++) {
        bool read = this->m_opened.m_data[i];

        if (!read && this->GetHeaderPriority(i) >= 3) {
            index = i;
        }
    }

    return index;
}

// VERIFY: 0b06d57d manual
// OFFSET: 0x005501C0
int32_t CAccountMsgHeaders::GetIndexNextUnreadMsg(int32_t index) {
    if (this->m_state == 1) {
        return -1;
    }

    for (int32_t i = 0; i < this->m_count; i++) {
        bool read = this->m_opened.m_data[i];

        if (i > index && !read) {
            return i;
        }
    }

    return -1;
}

// VERIFY: d9ba6998 manual
// OFFSET: 0x00550210
int32_t CAccountMsgHeaders::SetMsgRead(int32_t index) {
    int32_t messageId = this->GetMessageId(index);

    char accountName[1280];
    accountName[0] = '\0';
    SStrCopy(accountName, ClientServices::GetAccountName(), sizeof(accountName));

    uint8_t* sessionKey = ClientServices::Connection()->GetSessionKey();
    SHA1_CONTEXT context;
    uint8_t digest[20];
    SHA1_Init(&context);
    SHA1_Update(&context, sessionKey, 40);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>("\x0Bz"), 2);
    SHA1_Final(digest, &context);

    CDynamicString url;
    url.SetCapacity(1024);
    url = FrameScript_GetText("ACCOUNT_MESSAGE_READ_URL", -1, GENDER_NOT_APPLICABLE);
    url += "?accountName=";
    url += accountName;
    url += "&sessionKeyHash=";

    for (int32_t i = 0; i < 20; i++) {
        url += static_cast<int32_t>(digest[i]);
    }

    url += "&messageId=";
    url += messageId;

    if (!OsURLDownload(url.m_buffer, &AccountMsgReadCallback, nullptr)) {
        return 0;
    }

    if (index >= 0 && index < this->m_count) {
        this->m_opened.m_data[index] = 1;
    }

    return 1;
}

// VERIFY: 07228351 manual
// OFFSET: 0x00550650
CAccountMsgHeaders::~CAccountMsgHeaders() {
}

// VERIFY: 0dbdc78a auto
// OFFSET: 0x005506A0
int32_t CAccountMsgHeaders::LoadHeaders() {
    if (this->m_state == 1 || this->m_inQueue) {
        return 0;
    }

    char accountName[1280];
    accountName[0] = '\0';
    SStrCopy(accountName, ClientServices::GetAccountName(), sizeof(accountName));

    uint8_t* sessionKey = ClientServices::Connection()->GetSessionKey();
    SHA1_CONTEXT context;
    uint8_t digest[20];
    SHA1_Init(&context);
    SHA1_Update(&context, sessionKey, 40);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>("\x0Bz"), 2);
    SHA1_Final(digest, &context);

    CDynamicString url;
    url.SetCapacity(1024);
    url = FrameScript_GetText("ACCOUNT_MESSAGE_HEADERS_URL", -1, GENDER_NOT_APPLICABLE);
    url += "?accountName=";
    url += accountName;
    url += "&sessionKeyHash=";

    for (int32_t i = 0; i < 20; i++) {
        url += static_cast<int32_t>(digest[i]);
    }

    if (!OsURLDownload(url.m_buffer, &CKBPage::LoadingCallback, this)) {
        this->m_state = 3;
        return 0;
    }

    this->m_iterator.m_node = nullptr;
    this->m_count = 0;
    this->m_opened.Clear();
    this->_BeginLoading();
    return 1;
}

// VERIFY: eea88f7b manual
// OFFSET: 0x00550810
void CAccountMsgHeaders::_SendSuccessEvent() {
    if (!this->m_iterator.m_node) {
        this->_GetRoot(this->m_iterator);
        this->m_iterator.GoToFirstChild();
    }

    CKBIterator iterator = this->m_iterator;
    this->m_count = 0;
    iterator.GoToFirst();

    while (iterator.m_node) {
        this->m_count++;
        iterator.GoToNext();
    }

    this->m_opened.SetCount(this->m_count);

    iterator = this->m_iterator;
    iterator.GoToFirst();

    for (int32_t i = 0; i < this->m_count; i++) {
        iterator.GoTo(i);
        this->m_opened.m_data[i] = this->_GetInt(iterator.m_node, "opened");
    }

    if (CGlueMgr::AreScriptFunctionsLoaded()) {
        FrameScript_SignalEvent(GLUE_EVENT_ACCOUNT_MESSAGES_HEADERS_LOADED, nullptr);
    }
}
