#ifndef WOW_SOURCE_WOWSERVICES_GRUNTLOGIN_H
#define WOW_SOURCE_WOWSERVICES_GRUNTLOGIN_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "WoW/Source/WowServices/Grunt.h"
#include "WoW/Source/WowServices/HMAC.h"
#include "WoW/Source/WowServices/Login.h"

class GruntLogin : public Login {
    public:
    struct MATRIXINPUT {
        /* 0000 */ bool m_valid;
        /* 0001 */ uint8_t unused_0001[7];
        /* 0008 */ HMAC<SHA1> m_hmac;
        /* 00E8 */ SARC4Key m_key;
        /* 01EA */ uint8_t unused_01EA[6];

        MATRIXINPUT();
        void Finish(SHA1::Digest& digest);
    };

    /* 10F0 */ uint8_t m_versionChallenge [16];
    /* 1100 */ bool m_pinEnabled;
    /* 1101 */ uint8_t m_keyMap [10];
    /* 110B */ uint8_t m_pinSalt [16];
    /* 111B */ uint8_t m_pinClientSalt [16];
    /* 112B */ uint8_t m_pinHash [20];
    /* 113F */ bool m_matrixEnabled;
    /* 1140 */ uint8_t m_matrixWidth;
    /* 1141 */ uint8_t m_matrixHeight;
    /* 1142 */ uint8_t unused_1142;
    /* 1143 */ uint8_t m_matrixChallengeCount;
    /* 1144 */ uint32_t* m_matrixCoordinates;
    /* 1148 */ uint8_t m_matrixKey [16];
    /* 1158 */ GruntLogin::MATRIXINPUT* m_matrixInput;
    /* 115C */ GruntLogin::MATRIXINPUT* m_matrixCommitted;
    /* 1160 */ uint8_t m_matrixProof [20];
    /* 1174 */ bool m_tokenEnabled;
    /* 1175 */ uint8_t m_tokenRequired;
    /* 1176 */ char m_token [17];
    /* 1187 */ uint8_t unused_1187[1];
    /* 1188 */ Grunt::ClientLink* m_clientLink;

    bool Connected(const NETADDR& address) override;
    bool Disconnected() override;
    bool CantConnect() override;
    void GetLogonMethod() override;
    void GetVersionProof(const uint8_t* versionChallenge) override;
    void SetPinInfo(bool enabled, uint32_t seed, const uint8_t* salt) override;
    void GetPinInfo(bool* enabled, uint8_t** salt, uint8_t** hash) override;
    void SetMatrixInfo(bool enabled, uint8_t width, uint8_t height, uint8_t minDigits, uint8_t maxDigits, bool flipCoords, uint8_t challengeCount, uint64_t seed, const uint8_t* sessionKey, uint32_t sessionKeyLength) override;
    void GetMatrixProof(bool* enabled, const uint8_t** proof) const override;
    void SetTokenInfo(bool enabled, uint8_t required) override;
    void GetTokenInfo(bool* enabled, char* token) override;
    void LogonResult(Grunt::Result result, const uint8_t* sessionKey, uint32_t sessionKeyLength, uint16_t flags) override;
    void ReconnectResult(Grunt::Result result, const uint8_t* sessionKey, uint32_t sessionKeyLength, uint16_t flags) override;
    LOGIN_STATE NextSecurityState(LOGIN_STATE state) override;
    int32_t GetServerId() override;
    const uint8_t* GetVersionChallenge() override;
    void GetRealmList() override;
    void Reconnect() override;
    void Logon(const char* loginServer, const char* portal) override;
    int32_t Wait(const char* message, LoginData& loginData) override;
    void ProveVersion(const uint8_t* versionChecksum) override;
    void FileAccept(bool accept, uint64_t offset) override;
    void FileComplete(bool success) override;
    int32_t GetSurveyId() override;
    void SendSurveyData(uint16_t size, uint8_t* data) override;
    void SendSurveyFailed() override;
    void Logoff() override;
    const uint8_t* GetKeyMap() override;
    void EnterPIN(int32_t count, uint8_t* digits) override;
    void EnterMatrix(uint8_t value) override;
    void RevertMatrix() override;
    void CommitMatrix() override;
    void FinalizeMatrix() override;
    bool GetMatrixInfo(uint32_t* width, uint32_t* height, uint32_t* minDigits, uint32_t* maxDigits, bool* flipCoords, uint32_t* challengeCount) override;
    bool GetMatrixCoordinates(uint32_t index, uint32_t* x, uint32_t* y) override;
    void EnterToken(const char* token) override;
    bool IsTrialAccount() override;
    bool IsGMAccount() override;
    bool IsProPassAccount() override;
    void Update() override;
    void Init(LoginResponse* loginResponse) override;
    ~GruntLogin() override;
    void JoinRealm(uint32_t a1, uint32_t a2, uint32_t a3) override;
    int32_t GetLoginServerType() override;

    GruntLogin();
    void EncodePinDigits(uint32_t seed);
};

#endif
