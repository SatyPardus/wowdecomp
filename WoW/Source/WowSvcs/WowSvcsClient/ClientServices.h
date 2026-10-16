#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTSERVICES_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTSERVICES_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowServices/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/Types.h"

struct CVar;
class Login;

struct CLIENTCDKEY {
    /* 0000 */ uint16_t m_length;
    /* 0002 */ uint8_t unused_0002[2];
    /* 0004 */ uint8_t m_key[28];
};

class ClientConnection : public RealmConnection {
    public:
    /* 2F3C */ uint32_t m_connected;
    /* 2F40 */ uint32_t m_characterLoggedIn;
    /* 2F44 */ int32_t m_statusComplete;
    /* 2F48 */ int32_t m_statusResult;
    /* 2F4C */ WOWCS_OPS m_statusOp;
    /* 2F50 */ int32_t m_errorCode;
    /* 2F54 */ int32_t m_inGame;
    /* 2F58 */ uint8_t m_quitAfterLogout;
    /* 2F59 */ uint8_t m_loggingOut;
    /* 2F5A */ uint8_t m_realmConnectPending;
    /* 2F5B */ uint8_t unused_2F5B[1];
    /* 2F5C */ void (*m_cleanup)();

    ~ClientConnection() override;
    int32_t HandleConnect() override;
    int32_t HandleDisconnect() override;
    int32_t HandleCantConnect() override;
    void HandleCharacterCreate(uint8_t result) override;
    void HandleCharacterDelete(uint8_t result) override;
    void HandleCharacterLoginFailed(uint8_t result) override;
    void HandleLogoutComplete() override;
    void HandleLogoutResponse(int32_t result, int32_t instant) override;
    void HandleLogoutAbortAck() override;

    ClientConnection(RealmResponse* realmResponse);
    void AccountLogin(const char* accountName, const char* password, int32_t region, int32_t locale);
    void Cancel(int32_t errorCode);
    void Cleanup();
    void Complete(int32_t result, int32_t errorCode);
    void CharacterAbortLogout();
    void CharacterLogin(WOWGUID guid, int32_t a2);
    int32_t CharacterLoggingOut();
    void CharacterLogout(bool quit, bool instant);
    void CharacterForceLogout();
    void CharacterSetInGame(int32_t inGame);
    void Connect();
    void Disconnect();
    void HandleAuthResult(int32_t result);
    int32_t EnumerateCharacters(void (*callback)(CHARACTER_INFO& info, void* param), void* param);
    void GetCharacterList();
    void RequestCharacterCreate(const CHARACTER_CREATE_INFO& info);
    void RequestCharacterDelete(WOWGUID guid);
    void Initiate(WOWCS_OPS op, int32_t errorCode, void (*cleanup)());
    int32_t PollStatus(WOWCS_OPS& op, const char** text, int32_t& result, int32_t& errorCode);
};

class ClientRealmResponseAdapter : public RealmResponse {
    public:
    void LoginServerResult(LOGIN_STATE state, LOGIN_RESULT result, const char* message1, const char* message2, const char* message3) override;
    void RealmListReceived(TSFixedArray<REALM_INFO>& realms) override;
    void HandleAuthResponse(RealmConnection* connection, uint8_t result) override;
    void AddOnListReceived(RealmConnection* connection) override;
    void CharacterListReceived(RealmConnection* connection, const TSFixedArray<CHARACTER_INFO>& characters, int32_t success) override;
    void GameServerResult(RealmConnection* connection, const char* message1, const char* message2, const char* message3) override;
    void MessageReceived(RealmConnection* connection, uint32_t message) override;
    void OnChatReceived(RealmConnection* connection, WOWGUID guid, const char* text, uint8_t type) override;
    void OnAccountOnline(RealmConnection* connection, int32_t unused_a2, WOWGUID guid) override;
    void OnAFKMonitorInfoResponse(RealmConnection* connection, WOWGUID guid, int32_t unused_a3, int32_t unused_a4, int32_t unused_a5, int32_t unused_a6) override;
    void HandleConnect(RealmConnection* connection) override;
    void HandleDisconnect(RealmConnection* connection) override;
    void HandleCantConnect(RealmConnection* connection) override;
};

class ClientServices : public LoginResponse {
    public:
    static ClientServices* m_instance;
    static RealmResponse* s_clientRealmResponse;
    static char s_accountName[1280];
    static char* GetClientStamp();
    static Login* s_loginObj;
    static CVar* s_serverAlertVar;
    static CVar* s_realmListBNVar;
    static CVar* s_portalVar;
    static CVar* s_realmListVar;
    static CVar* s_decorateAccountName;
    static bool s_newLogin;
    static CVar* s_realmNameVar;
    static REALM_INFO m_selectRealmInfo;
    static bool m_selectRealmInfoValid;
    static ClientConnection* s_currentConnection;
    static CHARACTER_INFO s_characterInfo;
    static uint8_t GetCharacterLevel();

    static int32_t CharacterValidateDeclension(const char* name, const char* declension);
    static int32_t CharacterValidateName(const char* name);
    static void ClearMessageHandler(NETMESSAGE msgId);
    static void Send2(CDataStore* msg);
    static void SendPacket(CDataStore* msg);
    static ClientConnection* Connection();
    static void Destroy();
    static void ConnectToSelectedServer();
    static LoginResponse* CreateLoginResponse();
    static void GetRealmList();
    static const char* GetErrorToken(int32_t errorCode);
    static const char* GetAccountName();
    static uint8_t GetCharacterClass();
    static const char* GetCharacterName();
    static uint8_t GetCharacterRace();
    static uint8_t GetCharacterSex();
    static const char* GetSelectedRealmName();
    static int32_t LaunchURL(const char* url);
    static void LoginEnterToken(const char* token);
    static void LoginEnterPIN(int32_t count, uint8_t* digits);
    static void LoginEnterMatrix(uint8_t value);
    static void LoginCommitMatrix();
    static void LoginRevertMatrix();
    static void LoginFinalizeMatrix();
    static void FileAccept(bool accept, uint64_t offset);
    static void StartDownload();
    static void SendSurveyData(uint16_t size, uint8_t* data);
    static void SendSurveyFailed();
    static int32_t GetSurveyId();
    static const char* GetServerAlertURL();
    static Login* LoginConnection();
    static void Logon(const char* username, const char* password);
    static void SetAccountName(const char* accountName);
    static void SetCharacterInfo(const CHARACTER_INFO* info);
    static CHARACTER_INFO* GetCharacterInfo();
    static REALM_INFO* GetRealmInfoByIndex(uint32_t index);
    static REALM_INFO* GetSelectedRealm();
    static void Initialize();
    static void SetMessageHandler(NETMESSAGE msgId, MESSAGE_HANDLER handler, void* param);
    static void InitLoginServerCVars(int32_t overwrite, const char* locale);
    static void LoadCDKey();
    static bool SelectRealm(const char* realmName);
    static bool SetSelectedRealmInfo(int32_t index);
    static int32_t ValidDisconnect(const void* connection);

    bool GetCdKeys(Grunt::CdKey& cdKey) override;
    void FileData(const uint8_t* data, uint32_t size) override;
    const char* GetLoginServer() override;
    int32_t GetLoginServerType() override;
    void JoinRealmResult(uint32_t realmId, int32_t a2, int32_t a3, int32_t a4, int32_t a5) override;
    void LoginServerStatus(LOGIN_STATE state, LOGIN_RESULT result, const char* addressString, const char* stateString, const char* resultString, uint16_t flags) override;
    void RealmEnumCallback(uint32_t result) override;
};

extern ClientConnection* g_clientConnection;

int32_t ConsoleCommand_Logout(const char* command, const char* arguments);

int32_t SendBugOpcode(uint32_t suggestion, const char* text);

#endif
