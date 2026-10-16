#ifndef WOW_SOURCE_WOWSERVICES_LOGINRESPONSE_H
#define WOW_SOURCE_WOWSERVICES_LOGINRESPONSE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/WowServices/Types.h"

class CDataStore;

namespace Grunt {
struct CdKey;
}

class LoginResponse {
    public:
    /* 0004 */ TSFixedArray<REALM_INFO> m_realmList;
    /* 0010 */ uint16_t m_realmListTrailer;
    /* 0012 */ uint8_t unused_0012[2];
    /* 0014 */ LOGIN_STATE m_loginState;
    /* 0018 */ LOGIN_RESULT m_loginResult;

    virtual ~LoginResponse();
    virtual bool GetCdKeys(Grunt::CdKey& cdKey);
    virtual void FileData(const uint8_t* data, uint32_t size);
    virtual const char* GetLoginServer() = 0;
    virtual int32_t GetLoginServerType() = 0;
    virtual void JoinRealmResult(uint32_t realmId, int32_t a2, int32_t a3, int32_t a4, int32_t a5) = 0;
    virtual void LoginServerStatus(LOGIN_STATE state, LOGIN_RESULT result, const char* addressString, const char* stateString, const char* resultString, uint16_t flags) = 0;
    virtual void RealmEnumCallback(uint32_t result) = 0;

    void HandleRealmData(uint32_t result, CDataStore* msg);
    void UpdateLoginStatus(LOGIN_STATE state, LOGIN_RESULT result, const char* addressString, uint16_t flags);
};

#endif
