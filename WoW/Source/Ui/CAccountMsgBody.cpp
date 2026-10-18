#include "CAccountMsgBody.h"
#include "Engine/Source/Base/DynamicString.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00BEA7B0
CAccountMsgBody g_accountMsgBody;

// VERIFY: f292b33b manual
// OFFSET: 0x0054FB10
CAccountMsgBody::~CAccountMsgBody() {
}

// VERIFY: 8fed53e6 auto
// OFFSET: 0x0054FC50
int32_t CAccountMsgBody::LoadBody(int32_t messageId) {
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
    url = FrameScript_GetText("ACCOUNT_MESSAGE_BODY_NO_READ_URL", -1, GENDER_NOT_APPLICABLE);
    url += "?accountName=";
    url += accountName;
    url += "&sessionKeyHash=";

    for (int32_t i = 0; i < 20; i++) {
        url += static_cast<int32_t>(digest[i]);
    }

    url += "&messageId=";
    url += messageId;

#if defined(__APPLE__)
    this->m_iterator.m_node = nullptr;
    if (!this->_StartNativeDownload(url.m_buffer)) {
        return 0;
    }
#else
    if (!OsURLDownload(url.m_buffer, &CKBPage::LoadingCallback, this)) {
        this->m_state = 3;
        return 0;
    }

    this->m_iterator.m_node = nullptr;
    this->_BeginLoading();
#endif
    return 1;
}

// VERIFY: 2087b348 auto
// OFFSET: 0x0054FDB0
void CAccountMsgBody::_SendSuccessEvent() {
    if (CGlueMgr::AreScriptFunctionsLoaded()) {
        FrameScript_SignalEvent(GLUE_EVENT_ACCOUNT_MESSAGES_BODY_LOADED, nullptr);
    }
}

// VERIFY: bc4f5626 auto
// OFFSET: 0x0054FDD0
void CAccountMsgBody::_SendFailureEvent() {
    this->m_iterator.m_node = nullptr;
}
