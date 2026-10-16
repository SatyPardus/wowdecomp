#ifndef WOW_SOURCE_WOWSERVICES_LOGIN_H
#define WOW_SOURCE_WOWSERVICES_LOGIN_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/WowServices/Grunt.h"
#include "WoW/Source/WowServices/Types.h"

class CDataStore;
struct LoginData;
class LoginResponse;

extern const char* g_LoginStateStringNames[18];
extern const char* g_LoginResultStringNames[41];

class Login : public Grunt::ClientResponse {
    public:
    static uint32_t s_fourCCprogramID;

    /* 0004 */ bool m_online;
    /* 0005 */ bool m_loggedOn;
    /* 0006 */ uint8_t unused_0006[2];
    /* 0008 */ int32_t m_logonStatus;
    /* 000C */ bool m_reconnect;
    /* 000D */ char m_username [1280];
    /* 050D */ char m_rawUsername [1280];
    /* 0A0D */ char unused_0A0D [1280];
    /* 0F0D */ uint8_t unused_0F0D[3];
    /* 0F10 */ char* m_password;
    /* 0F14 */ uint8_t m_sessionKey [40];
    /* 0F3C */ uint32_t unused_0F3C;
    /* 0F40 */ uint64_t m_fileSize;
    /* 0F48 */ uint8_t m_fileMD5 [16];
    /* 0F58 */ char m_fileName [260];
    /* 105C */ uint8_t m_pinFailures;
    /* 105D */ uint8_t m_pinKeyMap [11];
    /* 1068 */ uint32_t unused_1068 [13];
    /* 109C */ bool m_matrixEnabled;
    /* 109D */ uint8_t m_matrixWidth;
    /* 109E */ uint8_t m_matrixHeight;
    /* 109F */ uint8_t m_matrixMinDigitCount;
    /* 10A0 */ uint8_t m_matrixMaxDigitCount;
    /* 10A1 */ uint8_t m_matrixChallengeCount;
    /* 10A2 */ bool m_matrixFlipCoords;
    /* 10A3 */ uint8_t unused_10A3[1];
    /* 10A4 */ uint32_t* m_matrixData;
    /* 10A8 */ uint32_t unused_10A8 [4];
    /* 10B8 */ uint64_t m_matrixSeed;
    /* 10C0 */ uint32_t unused_10C0 [5];
    /* 10D4 */ bool m_tokenRequired;
    /* 10D5 */ uint8_t unused_10D5;
    /* 10D6 */ uint8_t unused_10D6 [17];
    /* 10E7 */ uint8_t unused_10E7[1];
    /* 10E8 */ uint32_t unused_10E8;
    /* 10EC */ LoginResponse* m_loginResponse;

    bool OnlineIdle() override;
    bool GetCdKeys(Grunt::CdKey& cdKey) override;
    void GetFileStatus(const char* fileName, uint64_t size, const uint8_t* md5) override;
    void FileData(const uint8_t* data, uint32_t size) override;
    void RealmListResult(CDataStore* msg) override;
    virtual void GetRealmList() = 0;
    virtual void Reconnect() = 0;
    virtual void Logon(const char* loginServer, const char* portal) = 0;
    virtual int32_t Wait(const char* message, LoginData& loginData) = 0;
    virtual void ProveVersion(const uint8_t* versionChecksum) = 0;
    virtual void FileAccept(bool accept, uint64_t offset) = 0;
    virtual void FileComplete(bool success) = 0;
    virtual int32_t GetSurveyId() = 0;
    virtual void SendSurveyData(uint16_t size, uint8_t* data) = 0;
    virtual void SendSurveyFailed() = 0;
    virtual bool IsReconnect();
    virtual bool IsLoggedOn();
    virtual void Logoff() = 0;
    virtual const uint8_t* GetKeyMap() = 0;
    virtual void EnterPIN(int32_t count, uint8_t* digits) = 0;
    virtual void EnterMatrix(uint8_t value) = 0;
    virtual void RevertMatrix() = 0;
    virtual void CommitMatrix() = 0;
    virtual void FinalizeMatrix() = 0;
    virtual bool GetMatrixInfo(uint32_t* width, uint32_t* height, uint32_t* minDigits, uint32_t* maxDigits, bool* flipCoords, uint32_t* challengeCount) = 0;
    virtual bool GetMatrixCoordinates(uint32_t index, uint32_t* x, uint32_t* y) = 0;
    virtual void EnterToken(const char* token) = 0;
    virtual bool IsTrialAccount() = 0;
    virtual bool IsGMAccount() = 0;
    virtual bool IsProPassAccount() = 0;
    virtual void Update() = 0;
    virtual void Init(LoginResponse* loginResponse) = 0;
    virtual ~Login();
    virtual void JoinRealm(uint32_t a1, uint32_t a2, uint32_t a3) = 0;
    virtual int32_t GetLoginServerType() = 0;

    Login();
    void SetLogonCreds(const char* accountName, const char* password);
    void SetRawAccountName(const char* accountName);
};

#endif
