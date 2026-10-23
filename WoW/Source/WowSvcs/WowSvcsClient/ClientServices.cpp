#include <cstdint>
#include <cstring>
#include "ClientServices.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Common/WowTime.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Game/GameClient/WardenClient.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/GameClientCommands.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/AddOns.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/WowServices/BattlenetLogin.h"
#include "WoW/Source/WowServices/GruntLogin.h"
#include "WoW/Source/WowServices/Login.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"
#include "GameVersion.h"

// STATIC: 0x00AD91F0
static const char* s_errorTokens[] = {
    "RESPONSE_SUCCESS",
    "RESPONSE_FAILURE",
    "RESPONSE_CANCELLED",
    "RESPONSE_DISCONNECTED",
    "RESPONSE_FAILED_TO_CONNECT",
    "RESPONSE_CONNECTED",
    "RESPONSE_VERSION_MISMATCH",
    "CSTATUS_CONNECTING",
    "CSTATUS_NEGOTIATING_SECURITY",
    "CSTATUS_NEGOTIATION_COMPLETE",
    "CSTATUS_NEGOTIATION_FAILED",
    "CSTATUS_AUTHENTICATING",
    "AUTH_OK",
    "AUTH_FAILED",
    "AUTH_REJECT",
    "AUTH_BAD_SERVER_PROOF",
    "AUTH_UNAVAILABLE",
    "AUTH_SYSTEM_ERROR",
    "AUTH_BILLING_ERROR",
    "AUTH_BILLING_EXPIRED",
    "AUTH_VERSION_MISMATCH",
    "AUTH_UNKNOWN_ACCOUNT",
    "AUTH_INCORRECT_PASSWORD",
    "AUTH_SESSION_EXPIRED",
    "AUTH_SERVER_SHUTTING_DOWN",
    "AUTH_ALREADY_LOGGING_IN",
    "AUTH_LOGIN_SERVER_NOT_FOUND",
    "AUTH_WAIT_QUEUE",
    "AUTH_BANNED",
    "AUTH_ALREADY_ONLINE",
    "AUTH_NO_TIME",
    "AUTH_DB_BUSY",
    "AUTH_SUSPENDED",
    "AUTH_PARENTAL_CONTROL",
    "AUTH_LOCKED_ENFORCED",
    "REALM_LIST_IN_PROGRESS",
    "REALM_LIST_SUCCESS",
    "REALM_LIST_FAILED",
    "REALM_LIST_INVALID",
    "REALM_LIST_REALM_NOT_FOUND",
    "ACCOUNT_CREATE_IN_PROGRESS",
    "ACCOUNT_CREATE_SUCCESS",
    "ACCOUNT_CREATE_FAILED",
    "CHAR_LIST_RETRIEVING",
    "CHAR_LIST_RETRIEVED",
    "CHAR_LIST_FAILED",
    "CHAR_CREATE_IN_PROGRESS",
    "CHAR_CREATE_SUCCESS",
    "CHAR_CREATE_ERROR",
    "CHAR_CREATE_FAILED",
    "CHAR_CREATE_NAME_IN_USE",
    "CHAR_CREATE_DISABLED",
    "CHAR_CREATE_PVP_TEAMS_VIOLATION",
    "CHAR_CREATE_SERVER_LIMIT",
    "CHAR_CREATE_ACCOUNT_LIMIT",
    "CHAR_CREATE_SERVER_QUEUE",
    "CHAR_CREATE_ONLY_EXISTING",
    "CHAR_CREATE_EXPANSION",
    "CHAR_CREATE_EXPANSION_CLASS",
    "CHAR_CREATE_LEVEL_REQUIREMENT",
    "CHAR_CREATE_UNIQUE_CLASS_LIMIT",
    "CHAR_CREATE_CHARACTER_IN_GUILD",
    "CHAR_CREATE_RESTRICTED_RACECLASS",
    "CHAR_CREATE_CHARACTER_CHOOSE_RACE",
    "CHAR_CREATE_CHARACTER_ARENA_LEADER",
    "CHAR_CREATE_CHARACTER_DELETE_MAIL",
    "CHAR_CREATE_CHARACTER_SWAP_FACTION",
    "CHAR_CREATE_CHARACTER_RACE_ONLY",
    "CHAR_CREATE_CHARACTER_GOLD_LIMIT",
    "CHAR_CREATE_FORCE_LOGIN",
    "CHAR_DELETE_IN_PROGRESS",
    "CHAR_DELETE_SUCCESS",
    "CHAR_DELETE_FAILED",
    "CHAR_DELETE_FAILED_LOCKED_FOR_TRANSFER",
    "CHAR_DELETE_FAILED_GUILD_LEADER",
    "CHAR_DELETE_FAILED_ARENA_CAPTAIN",
    "CHAR_LOGIN_IN_PROGRESS",
    "CHAR_LOGIN_SUCCESS",
    "CHAR_LOGIN_NO_WORLD",
    "CHAR_LOGIN_DUPLICATE_CHARACTER",
    "CHAR_LOGIN_NO_INSTANCES",
    "CHAR_LOGIN_FAILED",
    "CHAR_LOGIN_DISABLED",
    "CHAR_LOGIN_NO_CHARACTER",
    "CHAR_LOGIN_LOCKED_FOR_TRANSFER",
    "CHAR_LOGIN_LOCKED_BY_BILLING",
    "CHAR_LOGIN_LOCKED_BY_MOBILE_AH",
    "CHAR_NAME_SUCCESS",
    "CHAR_NAME_FAILURE",
    "CHAR_NAME_NO_NAME",
    "CHAR_NAME_TOO_SHORT",
    "CHAR_NAME_TOO_LONG",
    "CHAR_NAME_INVALID_CHARACTER",
    "CHAR_NAME_MIXED_LANGUAGES",
    "CHAR_NAME_PROFANE",
    "CHAR_NAME_RESERVED",
    "CHAR_NAME_INVALID_APOSTROPHE",
    "CHAR_NAME_MULTIPLE_APOSTROPHES",
    "CHAR_NAME_THREE_CONSECUTIVE",
    "CHAR_NAME_INVALID_SPACE",
    "CHAR_NAME_CONSECUTIVE_SPACES",
    "CHAR_NAME_RUSSIAN_CONSECUTIVE_SILENT_CHARACTERS",
    "CHAR_NAME_RUSSIAN_SILENT_CHARACTER_AT_BEGINNING_OR_END",
    "CHAR_NAME_DECLENSION_DOESNT_MATCH_BASE_NAME",
};

// STATIC: 0x00C79620
char ClientServices::s_accountName[1280];

// STATIC: 0x00C79B98
REALM_INFO ClientServices::m_selectRealmInfo;

// STATIC: 0x00C79CE0
ClientConnection* g_clientConnection;

// STATIC: 0x00C79CE4
ClientServices* ClientServices::m_instance;

// STATIC: 0x00C79CE8
bool ClientServices::s_newLogin;

// STATIC: 0x00C79CE9
bool ClientServices::m_selectRealmInfoValid;

// STATIC: 0x00C79CEC
Login* ClientServices::s_loginObj;

// STATIC: 0x00C79E98
static char s_statusText[256];

// STATIC: 0x00C79CF0
RealmResponse* ClientServices::s_clientRealmResponse;

// STATIC: 0x00C79CF4
ClientConnection* ClientServices::s_currentConnection;

// STATIC: 0x00C79D10
CHARACTER_INFO ClientServices::s_characterInfo;

// STATIC: 0x00C79B20
static CLIENTCDKEY s_cdKey;

// STATIC: 0x00C79CEA
static bool s_cdKeyLoaded;

// VERIFY: a1007b9c auto
// STATIC: 0x00AD93C0
static const char* s_processorVendors[] = { "Unknown", "intel", "AMD", "PPC" };

// VERIFY: a1007b9c auto
// OFFSET: 0x006B02C0
static void LogConnectionStatus(WOWCS_OPS op, int32_t errorCode, int32_t initiating, int32_t result) {
    char opText[32];
    char errorText[32];

    switch (op) {
        case COP_NONE:
            SStrCopy(opText, "COP_NONE", sizeof(opText));
            break;

        case COP_INIT:
            SStrCopy(opText, "COP_INIT", sizeof(opText));
            break;

        case COP_CONNECT:
            SStrCopy(opText, "COP_CONNECT", sizeof(opText));
            break;

        case COP_AUTHENTICATE:
            SStrCopy(opText, "COP_AUTHENTICATE", sizeof(opText));
            break;

        case COP_CREATE_ACCOUNT:
            SStrCopy(opText, "COP_CREATE_ACCOUNT", sizeof(opText));
            break;

        case COP_CREATE_CHARACTER:
            SStrCopy(opText, "COP_CREATE_CHARACTER", sizeof(opText));
            break;

        case COP_GET_CHARACTERS:
            SStrCopy(opText, "COP_GET_CHARACTERS", sizeof(opText));
            break;

        case COP_DELETE_CHARACTER:
            SStrCopy(opText, "COP_DELETE_CHARACTER", sizeof(opText));
            break;

        case COP_LOGIN_CHARACTER:
            SStrCopy(opText, "COP_LOGIN_CHARACTER", sizeof(opText));
            break;

        case COP_GET_REALMS:
            SStrCopy(opText, "COP_GET_REALMS", sizeof(opText));
            break;

        case COP_WAIT_QUEUE:
            SStrCopy(opText, "COP_WAIT_QUEUE", sizeof(opText));
            break;

        default:
            SStrPrintf(opText, sizeof(opText), "%d", op);
            break;
    }

    switch (errorCode) {
        case 0:
            SStrCopy(errorText, "RESPONSE_SUCCESS", sizeof(errorText));
            break;

        case 1:
            SStrCopy(errorText, "RESPONSE_FAILURE", sizeof(errorText));
            break;

        case 2:
            SStrCopy(errorText, "RESPONSE_CANCELLED", sizeof(errorText));
            break;

        case 3:
            SStrCopy(errorText, "RESPONSE_DISCONNECTED", sizeof(errorText));
            break;

        case 4:
            SStrCopy(errorText, "RESPONSE_FAILED_TO_CONNECT", sizeof(errorText));
            break;

        case 5:
            SStrCopy(errorText, "RESPONSE_CONNECTED", sizeof(errorText));
            break;

        case 6:
            SStrCopy(errorText, "RESPONSE_VERSION_MISMATCH", sizeof(errorText));
            break;

        case 7:
            SStrCopy(errorText, "CSTATUS_CONNECTING", sizeof(errorText));
            break;

        case 8:
            SStrCopy(errorText, "CSTATUS_NEGOTIATING_SECURITY", sizeof(errorText));
            break;

        case 9:
            SStrCopy(errorText, "CSTATUS_NEGOTIATION_COMPLETE", sizeof(errorText));
            break;

        case 10:
            SStrCopy(errorText, "CSTATUS_NEGOTIATION_FAILED", sizeof(errorText));
            break;

        case 11:
            SStrCopy(errorText, "CSTATUS_AUTHENTICATING", sizeof(errorText));
            break;

        case 12:
            SStrCopy(errorText, "AUTH_OK", sizeof(errorText));
            break;

        case 13:
            SStrCopy(errorText, "AUTH_FAILED", sizeof(errorText));
            break;

        case 14:
            SStrCopy(errorText, "AUTH_REJECT", sizeof(errorText));
            break;

        case 15:
            SStrCopy(errorText, "AUTH_BAD_SERVER_PROOF", sizeof(errorText));
            break;

        case 16:
            SStrCopy(errorText, "AUTH_UNAVAILABLE", sizeof(errorText));
            break;

        case 17:
            SStrCopy(errorText, "AUTH_SYSTEM_ERROR", sizeof(errorText));
            break;

        case 18:
            SStrCopy(errorText, "AUTH_BILLING_ERROR", sizeof(errorText));
            break;

        case 19:
            SStrCopy(errorText, "AUTH_BILLING_EXPIRED", sizeof(errorText));
            break;

        case 20:
            SStrCopy(errorText, "AUTH_VERSION_MISMATCH", sizeof(errorText));
            break;

        case 21:
            SStrCopy(errorText, "AUTH_UNKNOWN_ACCOUNT", sizeof(errorText));
            break;

        case 22:
            SStrCopy(errorText, "AUTH_INCORRECT_PASSWORD", sizeof(errorText));
            break;

        case 23:
            SStrCopy(errorText, "AUTH_SESSION_EXPIRED", sizeof(errorText));
            break;

        case 24:
            SStrCopy(errorText, "AUTH_SERVER_SHUTTING_DOWN", sizeof(errorText));
            break;

        case 25:
            SStrCopy(errorText, "AUTH_ALREADY_LOGGING_IN", sizeof(errorText));
            break;

        case 26:
            SStrCopy(errorText, "AUTH_LOGIN_SERVER_NOT_FOUND", sizeof(errorText));
            break;

        case 27:
            SStrCopy(errorText, "AUTH_WAIT_QUEUE", sizeof(errorText));
            break;

        case 35:
            SStrCopy(errorText, "REALM_LIST_IN_PROGRESS", sizeof(errorText));
            break;

        case 36:
            SStrCopy(errorText, "REALM_LIST_SUCCESS", sizeof(errorText));
            break;

        case 37:
            SStrCopy(errorText, "REALM_LIST_FAILED", sizeof(errorText));
            break;

        case 38:
            SStrCopy(errorText, "REALM_LIST_INVALID", sizeof(errorText));
            break;

        case 39:
            SStrCopy(errorText, "REALM_LIST_REALM_NOT_FOUND", sizeof(errorText));
            break;

        case 74:
            SStrCopy(errorText, "CHAR_DELETE_FAILED_GUILD_LEADER", sizeof(errorText));
            break;

        case 75:
            SStrCopy(errorText, "CHAR_DELETE_FAILED_ARENA_CAPTAIN", sizeof(errorText));
            break;

        default:
            SStrPrintf(errorText, sizeof(errorText), "%d", errorCode);
            break;
    }

    if (initiating) {
        CGlueMgr::LogConnectionStatus("ClientConnection %s: %s code=%s %s", "Initiating", opText, errorText, "");
    } else {
        CGlueMgr::LogConnectionStatus("ClientConnection %s: %s code=%s %s", "Completed", opText, errorText, result ? "result=TRUE" : "result=FALSE");
    }
}

// VERIFY: 9e19522f manual
// OFFSET: 0x006B0250
ClientConnection::ClientConnection(RealmResponse* realmResponse) : RealmConnection(realmResponse) {
    this->m_connected = 0;
    this->m_characterLoggedIn = 0;
    this->m_inGame = 0;
    this->m_statusComplete = 1;
    this->m_statusResult = 1;
    this->m_statusOp = COP_NONE;
    this->m_errorCode = RESPONSE_SUCCESS;
    this->m_cleanup = nullptr;
    this->m_quitAfterLogout = 0;
    this->m_loggingOut = 0;
    this->m_realmConnectPending = 0;
}

ClientConnection::~ClientConnection() {
}

// VERIFY: b1444fa0 auto
// OFFSET: 0x006B0190
void ClientServices::LoadCDKey() {
    memset(&s_cdKey, 0, sizeof(s_cdKey));

    SArchive* archive;

    if (!SFile::OpenArchive("Data\\base.MPQ", 100, 0x800, &archive) || !archive) {
        return;
    }

    void* data;
    uint32_t size = sizeof(s_cdKey.m_key);

    if (SFile::ReadFile(archive, "telemetry.dat", &data, &size, 0, 0, nullptr) && data) {
        if (size == sizeof(s_cdKey.m_key)) {
            s_cdKeyLoaded = true;
            s_cdKey.m_length = 20;
            memcpy(s_cdKey.m_key, data, sizeof(s_cdKey.m_key));
        }

        SFile::Unload(data);
    }

    SFile::CloseArchive(archive);
}

// OFFSET: 0x006B0850
void ClientConnection::Cleanup() {
    if (this->m_cleanup) {
        this->m_cleanup();
        this->m_cleanup = nullptr;
    }
}

// VERIFY: e82e7a7e auto
// OFFSET: 0x006B0870
void ClientConnection::AccountLogin(const char* accountName, const char* password, int32_t region, int32_t locale) {
    if (this->m_statusComplete != 1 || !accountName || !password) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->Initiate(COP_AUTHENTICATE, CSTATUS_AUTHENTICATING, nullptr);
}

// VERIFY: 2a92ccc9 auto
// OFFSET: 0x006B0940
void ClientConnection::Disconnect() {
    this->NetClient::Disconnect();
    this->m_connected = 0;
    WardenClient_Destroy();
}

// VERIFY: ddc9b4a6 manual
// OFFSET: 0x006B0970
ClientConnection* ClientServices::Connection() {
    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return ClientServices::s_currentConnection;
}

// VERIFY: d41ee95d auto
// OFFSET: 0x006B0990
Login* ClientServices::LoginConnection() {
    return ClientServices::s_loginObj;
}

// VERIFY: 409cba3b auto
// OFFSET: 0x006B0B50
void ClientServices::Send2(CDataStore* msg) {
    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_currentConnection->Send(msg);
}

// VERIFY: 8de1eeef auto
// OFFSET: 0x006B0B80
void ClientServices::SetMessageHandler(NETMESSAGE msgId, MESSAGE_HANDLER handler, void* param) {
    if (!handler) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_currentConnection->SetMessageHandler(msgId, handler, param);
}

// VERIFY: 71af073c auto
// OFFSET: 0x006B0BC0
void ClientServices::ClearMessageHandler(NETMESSAGE msgId) {
    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_currentConnection->ResetHandler(msgId);
}

// VERIFY: 40900e3f auto
// OFFSET: 0x006B0DC0
const char* ClientServices::GetSelectedRealmName() {
    if (!ClientServices::s_realmNameVar) {
        ClientServices::s_realmNameVar = CVar::Register("realmName", "Last realm connected to", 0, "", nullptr, CATEGORY_NET, false, nullptr, false);
    }

    return ClientServices::s_realmNameVar->m_stringValue.m_str;
}

// VERIFY: efa85be0 auto
// OFFSET: 0x006B0DF0
const char* ClientServices::GetServerAlertURL() {
    if (!ClientServices::s_serverAlertVar) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return ClientServices::s_serverAlertVar->m_stringValue.m_str;
}

// VERIFY: 1607691e auto
// OFFSET: 0x006B0ED0
REALM_INFO* ClientServices::GetSelectedRealm() {
    return ClientServices::m_selectRealmInfoValid ? &ClientServices::m_selectRealmInfo : nullptr;
}

// VERIFY: f41bbd12 auto
// OFFSET: 0x006B1200
void ClientConnection::Cancel(int32_t errorCode) {
    if (this->m_cleanup) {
        this->m_cleanup();
        this->m_cleanup = nullptr;
    }

    this->m_statusResult = 0;
    this->m_errorCode = errorCode;
    this->m_statusComplete = 1;
    LogConnectionStatus(this->m_statusOp, errorCode, 0, 0);
}

// VERIFY: 625995e6 auto
// OFFSET: 0x006B0EE0
void ClientServices::FileData(const uint8_t* data, uint32_t size) {
    CGlueMgr::DownloadFeed(data, size);
}

// VERIFY: 74020b91 auto
// OFFSET: 0x006B0F40
const char* ClientServices::GetErrorToken(int32_t errorCode) {
    if (static_cast<uint32_t>(errorCode - 1) > 0x66) {
        return "";
    }

    return s_errorTokens[errorCode];
}

// VERIFY: c031479f auto
// OFFSET: 0x006B0F90
int32_t ClientServices::CharacterValidateName(const char* name) {
    return static_cast<int32_t>(ValidateCharacterNameConst(s_locale, name, true, true, true, true, 0)) + CHAR_NAME_SUCCESS;
}

// VERIFY: ca1062a6 auto
// OFFSET: 0x006B0FC0
int32_t ClientServices::CharacterValidateDeclension(const char* name, const char* declension) {
    return static_cast<int32_t>(ValidateCharacterNameDeclension(s_locale, name, declension)) + CHAR_NAME_SUCCESS;
}

// VERIFY: ae977242 manual
// OFFSET: 0x006B1100
int32_t ClientConnection::PollStatus(WOWCS_OPS& op, const char** text, int32_t& result, int32_t& errorCode) {
    op = this->m_statusOp;
    errorCode = this->m_errorCode;
    result = this->m_statusResult;

    if (!this->m_statusComplete) {
        *text = "";
        return this->m_statusComplete;
    }

    const char* token = errorCode >= 0 && errorCode < static_cast<int32_t>(sizeof(s_errorTokens) / sizeof(s_errorTokens[0])) ? FrameScript_GetText(s_errorTokens[errorCode], -1, GENDER_NOT_APPLICABLE) : nullptr;

    if (!token) {
        SStrPrintf(s_statusText, sizeof(s_statusText), "(%i)", errorCode);
        *text = s_statusText;
        return this->m_statusComplete;
    }

    if (errorCode == AUTH_WAIT_QUEUE) {
        SStrPrintf(s_statusText, sizeof(s_statusText), token, ClientServices::Connection()->m_queuePosition);
        *text = s_statusText;
        return this->m_statusComplete;
    }

    *text = token;
    return this->m_statusComplete;
}

// VERIFY: 5b8bbe2b manual
// OFFSET: 0x006B1BB0
void ClientServices::Destroy() {
    ClientServices::m_selectRealmInfoValid = false;

    if (ClientServices::s_loginObj) {
        ClientServices::s_loginObj->Logoff();
    }

    ConsoleCommandUnregister("logout");

    if (g_clientConnection) {
        if (g_clientConnection->m_connected) {
            g_clientConnection->NetClient::Disconnect();
            g_clientConnection->m_connected = 0;
            WardenClient_Destroy();
        }

        delete g_clientConnection;
        g_clientConnection = nullptr;
    }

    if (ClientServices::s_loginObj) {
        delete ClientServices::s_loginObj;
        ClientServices::s_loginObj = nullptr;
    }

    if (ClientServices::s_clientRealmResponse) {
        delete ClientServices::s_clientRealmResponse;
        ClientServices::s_clientRealmResponse = nullptr;
    }

    if (ClientServices::m_instance) {
        delete ClientServices::m_instance;
        ClientServices::m_instance = nullptr;
    }
}

// VERIFY: fa6032ac auto
// OFFSET: 0x006B21F0
void ClientConnection::CharacterForceLogout() {
    this->CharacterLogout(false, true);
}

// VERIFY: 0e65977c manual
// OFFSET: 0x006B2200
void ClientServices::Initialize() {
    if (!g_clientConnection) {
        ClientServices::s_clientRealmResponse = new (__FILE__, __LINE__) ClientRealmResponseAdapter();
        g_clientConnection = new (__FILE__, __LINE__) ClientConnection(ClientServices::s_clientRealmResponse);

        if (!g_clientConnection) {
            SErrDisplayAppFatalMessage("", "", 0);
        }
    }

    ClientServices::s_currentConnection = g_clientConnection;
    ConsoleCommandRegister("logout", ConsoleCommand_Logout, CATEGORY_DEFAULT, nullptr);
}

// VERIFY: 8f39a885 manual
// OFFSET: 0x006B22A0
int32_t SendBugOpcode(uint32_t suggestion, const char* text) {
    if (!text || !*text) {
        return 0;
    }

    char info[4096] = { 0 };
    char line[256];
    char name[256];
    uint32_t size = sizeof(name);

    if (OsGetUserName(name, &size)) {
        SStrPrintf(line, sizeof(line), "Username:\t%s\n", name);
        SStrPack(info, line, sizeof(info));
    }

    size = sizeof(name);

    if (OsGetComputerName(name, &size)) {
        SStrPrintf(line, sizeof(line), "Computer:\t%s\n", name);
        SStrPack(info, line, sizeof(info));
    }

    SStrPrintf(line, sizeof(line), "Processors:\t%u\n", OsGetNumberOfProcessors());
    SStrPack(info, line, sizeof(info));
    int32_t vendor;
    OsGetProcessorFeaturesEx(vendor);

    if (static_cast<uint32_t>(vendor) > OS_PROCESSOR_VENDOR_CPUID_FAILED) {
        vendor = OS_PROCESSOR_VENDOR_UNKNOWN;
    }

    SStrPrintf(line, sizeof(line), "Processor vendor:\t%s\n", s_processorVendors[vendor]);
    SStrPack(info, line, sizeof(info));
    uint64_t clocks = OsGetAsyncClocksPerSecond();
    double speed;
    char unit;

    if (clocks >= 990000000) {
        speed = static_cast<double>(clocks / 1000000);
        unit = 'G';
    } else {
        speed = static_cast<double>(clocks / 1000);
        unit = 'M';
    }

    SStrPrintf(line, sizeof(line), "Processor speed:\t%6.2f%cHz\n", speed * 0.001, unit);
    SStrPack(info, line, sizeof(info));
    SStrPrintf(line, sizeof(line), "Memory:\t%uMB\n", static_cast<uint32_t>(OsGetPhysicalMemory() >> 20));
    SStrPack(info, line, sizeof(info));
    SStrPack(info, "OS:\t\t", sizeof(info));
    OsGetVersionString(line, sizeof(line));
    SStrPack(info, line, sizeof(info));
    SStrPack(info, "\n", sizeof(info));
    SStrPrintf(line, sizeof(line), "Version:\t%s\n", "WoW [Release] Build " GAME_BUILD_STRING " (" GAME_BUILD_DATE " " GAME_BUILD_TIME ")");
    SStrPack(info, line, sizeof(info));
    const char* account = ClientServices::s_accountName[0] ? ClientServices::s_accountName : nullptr;

    if (account) {
        SStrPrintf(line, sizeof(line), "Realm:\t%s\n", ClientServices::GetSelectedRealmName());
        SStrPack(info, line, sizeof(info));
        SStrPrintf(line, sizeof(line), "Account:\t%s\n", account);
        SStrPack(info, line, sizeof(info));

        if (ClientServices::Connection() && ClientServices::Connection()->m_inGame) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                CGUnitData* unit = player->m_unit;
                ChrRacesRec* race = g_chrRacesDB.GetRecord(unit->m_unitFieldBytes0[0]);
                ChrClassesRec* playerClass = g_chrClassesDB.GetRecord(unit->m_unitFieldBytes0[1]);
                const char* className = playerClass ? playerClass->m_name : "Unknown";
                const char* raceName = race ? race->m_name : "Unknown";
                SStrPrintf(line, sizeof(line), "Character:\t%s (level %i %s %s %s)\n", player->GetUnitName(nullptr, 1), player->m_unit->m_unitFieldLevel, raceName, s_sexNames[player->m_unit->m_unitFieldBytes0[2]], className);
                SStrPack(info, line, sizeof(info));
                int32_t mapId = CGPlayer_C::GetNewContinentID();
                MapRec* map = g_mapDB.GetRecord(mapId);

                if (map) {
                    SStrPrintf(line, sizeof(line), "Map:\t\t%u (%s)\n", mapId, map->m_directory);
                } else {
                    SStrPrintf(line, sizeof(line), "Map:\t\t%u\n", mapId);
                }

                SStrPack(info, line, sizeof(info));

                if (CGGameUI::m_zoneText && *CGGameUI::m_zoneText) {
                    SStrPrintf(line, sizeof(line), "Zone:\t\t%s", CGGameUI::m_zoneText);
                    SStrPack(info, line, sizeof(info));

                    if (CGGameUI::m_subZoneText && *CGGameUI::m_subZoneText) {
                        SStrPrintf(line, sizeof(line), " - %s", CGGameUI::m_subZoneText);
                        SStrPack(info, line, sizeof(info));
                    }

                    SStrPack(info, "\n", sizeof(info));
                }

                C3Vector position = player->GetPosition();
                SStrPrintf(line, sizeof(line), "Position:\t%f %f %f, %f\n", position.x, position.y, position.z, player->GetFacing());
                SStrPack(info, line, sizeof(info));

                if (CGGameUI::m_lockedTarget) {
                    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGameUI::m_lockedTarget, TYPE_UNIT, __FILE__, __LINE__));

                    if (target) {
                        SStrPrintf(line, sizeof(line), "Target:\t%s\n", target->GetUnitName(nullptr, 1));
                        SStrPack(info, line, sizeof(info));
                    }
                }

                SStrPrintf(line, sizeof(line), "Auras:\n");
                SStrPack(info, line, sizeof(info));
                SpellRec spell;

                for (uint32_t i = 0; i < player->GetAuraCount(); i++) {
                    int32_t spellId = player->GetAuraSpellId(i);

                    if (!spellId) {
                        continue;
                    }

                    const char* spellName = g_spellDB.GetRecord(spellId, &spell) ? spell.m_name : "UNKNOWN";
                    SStrPrintf(line, sizeof(line), "\t%s(%d)\n", spellName, spellId);
                    SStrPack(info, line, sizeof(info));
                }
            }
        }
    }

    uint32_t infoSize = SStrLen(info) + 1;
    uint32_t textSize = SStrLen(text) + 1;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BUG));
    msg.Put(suggestion);
    msg.Put(infoSize);
    msg.PutData(info, infoSize);
    msg.Put(textSize);
    msg.PutData(text, textSize);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 1;
}

// VERIFY: 619198b6 manual
// OFFSET: 0x006B2A90
void ClientServices::Logon(const char* username, const char* password) {
    if (ClientServices::s_newLogin) {
        if (ClientServices::s_loginObj) {
            delete ClientServices::s_loginObj;
            ClientServices::s_loginObj = nullptr;
        }

        ClientServices::s_newLogin = false;
    }

    bool battlenet = SStrChr(username, '@') != nullptr;

    if (ClientServices::s_loginObj && ((ClientServices::s_loginObj->GetLoginServerType() == 1 && !battlenet) || (ClientServices::s_loginObj->GetLoginServerType() == 0 && battlenet))) {
        delete ClientServices::s_loginObj;
        ClientServices::s_loginObj = nullptr;
    }

    if (!ClientServices::s_loginObj) {
        if (battlenet) {
            ClientServices::s_loginObj = new (__FILE__, __LINE__) BattlenetLogin();
        } else {
            ClientServices::s_loginObj = new (__FILE__, __LINE__) GruntLogin();
        }

        ClientServices::s_loginObj->Init(ClientServices::CreateLoginResponse());
    }

    ClientServices::s_loginObj->SetRawAccountName(username);

    char accountName[1280];

    if (ClientServices::s_decorateAccountName->m_intValue) {
        switch (s_locale) {
            case 0:
            case 7:
                SStrPrintf(accountName, sizeof(accountName), s_region == 3 ? "%s#EU" : "%s#US", username);
                break;

            case 1:
                SStrPrintf(accountName, sizeof(accountName), "%s#KR", username);
                break;

            case 2:
            case 3:
            case 6:
            case 8:
                SStrPrintf(accountName, sizeof(accountName), "%s#EU", username);
                break;

            case 4:
            case 5:
                SStrPrintf(accountName, sizeof(accountName), "%s#CN", username);
                break;

            default:
                break;
        }
    } else {
        SStrPrintf(accountName, sizeof(accountName), "%s", username);
    }

    ClientServices::s_loginObj->SetLogonCreds(accountName, password);

    const char* portal = ClientServices::s_loginObj->GetLoginServerType() == 1 ? ClientServices::s_portalVar->m_stringValue.m_str : "";
    const char* realmList = ClientServices::s_loginObj->GetLoginServerType() == 1 ? ClientServices::s_realmListBNVar->m_stringValue.m_str : ClientServices::s_realmListVar->m_stringValue.m_str;
    ClientServices::s_loginObj->Logon(realmList, portal);
}

// STATIC: 0x00C79B40
static char s_clientStamp[88];

// VERIFY: 7f19facb manual
// OFFSET: 0x006B1C70
char* ClientServices::GetClientStamp() {
    const char* account = ClientServices::s_accountName[0] ? ClientServices::s_accountName : nullptr;
    REALM_INFO* realm = ClientServices::m_selectRealmInfoValid ? &ClientServices::m_selectRealmInfo : nullptr;
    uint32_t time;
    WowTime::WowEncodeTime(time, &g_clientGameTime);
    memset(s_clientStamp, 0, sizeof(s_clientStamp));

    if (!account || !realm) {
        return s_clientStamp;
    }

    SStrCopy(s_clientStamp, account, STORM_MAX_STR);
    const char* address = realm->m_realmAddress;
    *reinterpret_cast<uint32_t*>(&s_clientStamp[64]) = time;
    const char* source = &address[SStrLen(address) - 1];
    memset(&s_clientStamp[68], '0', 17);
    char* dest = &s_clientStamp[84];

    while (*source >= '0' && *source <= '9') {
        *dest-- = *source--;
    }

    source--;
    dest = &s_clientStamp[79];

    while (*source >= '0' && *source <= '9') {
        *dest-- = *source--;
    }

    source--;
    dest = &s_clientStamp[76];

    while (*source >= '0' && *source <= '9') {
        *dest-- = *source--;
    }

    source--;
    dest = &s_clientStamp[73];

    while (*source >= '0' && *source <= '9') {
        *dest-- = *source--;
    }

    source--;

    for (dest = &s_clientStamp[70]; source >= address; dest--) {
        *dest = *source--;
    }

    s_clientStamp[85] = static_cast<char>(0xFF);
    s_clientStamp[86] = 63;
    s_clientStamp[87] = 15;
    return s_clientStamp;
}

// VERIFY: 9a7893f1 auto
// OFFSET: 0x006B1E00
void ClientServices::LoginServerStatus(LOGIN_STATE state, LOGIN_RESULT result, const char* addressString, const char* stateString, const char* resultString, uint16_t flags) {
    CGlueMgr::SetLoginStateAndResult(state, result, addressString, stateString, resultString, flags);

    if (state != LOGIN_STATE_AUTHENTICATED) {
        return;
    }

    LoginData loginData;
    SStrCopy(loginData.m_account, ClientServices::s_loginObj->m_username, sizeof(loginData.m_account));
    loginData.m_loginServerID = ClientServices::s_loginObj->GetServerId();
    memcpy(loginData.m_sessionKey, ClientServices::s_loginObj->m_sessionKey, sizeof(loginData.m_sessionKey));
    loginData.m_loginServerType = ClientServices::s_loginObj->GetLoginServerType();

    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_currentConnection->SetLoginData(&loginData);
    SStrCopy(ClientServices::s_accountName, loginData.m_account, sizeof(ClientServices::s_accountName));
    CGlueMgr::SetCurrentAccount(loginData.m_account);
    InitializeAddOns(loginData.m_account);
    CVar::DeleteAccountCVars();
}

// VERIFY: c43cea73 manual
// OFFSET: 0x00406F40
void ClientServices::SendPacket(CDataStore* msg) {
    StaticSingleton<CommandManager>::Instance()->m_connection->Send(msg);
}

// VERIFY: 89403ad4 manual
// OFFSET: 0x004D9FE0
int32_t ClientServices::GetLoginServerType() {
    if (!ClientServices::LoginConnection()) {
        return 0;
    }

    return ClientServices::LoginConnection()->GetLoginServerType();
}

// VERIFY: b4926eb8 manual
// OFFSET: 0x004DA280
LoginResponse* ClientServices::CreateLoginResponse() {
    if (!ClientServices::m_instance) {
        ClientServices::m_instance = new (__FILE__, __LINE__) ClientServices;
    }

    return ClientServices::m_instance;
}

// VERIFY: ba3ec0a7 auto
// OFFSET: 0x006B0810
void ClientConnection::Initiate(WOWCS_OPS op, int32_t errorCode, void (*cleanup)()) {
    this->m_cleanup = cleanup;
    this->m_statusOp = op;
    this->m_errorCode = errorCode;
    this->m_statusComplete = 0;
    LogConnectionStatus(op, errorCode, 1, 0);
}

// VERIFY: 32558787 auto
// OFFSET: 0x006B0E10
void ClientServices::GetRealmList() {
    ClientConnection* connection = ClientServices::s_currentConnection;

    if (!connection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    connection->Initiate(COP_GET_REALMS, REALM_LIST_IN_PROGRESS, nullptr);

    if (ClientServices::s_loginObj->IsLoggedOn()) {
        ClientServices::s_loginObj->GetRealmList();
    } else {
        ClientServices::s_loginObj->Reconnect();
    }
}

// VERIFY: c726c41b auto
// OFFSET: 0x006B10B0
void ClientConnection::Complete(int32_t result, int32_t errorCode) {
    if (this->m_cleanup) {
        this->m_cleanup();
        this->m_cleanup = nullptr;
    }

    this->m_statusResult = result;
    this->m_errorCode = errorCode;
    this->m_statusComplete = 1;
    LogConnectionStatus(this->m_statusOp, errorCode, 0, result);
}

// VERIFY: aafb3f47 auto
// OFFSET: 0x006B2F20
bool ClientServices::GetCdKeys(Grunt::CdKey& cdKey) {
    if (!s_cdKeyLoaded) {
        return false;
    }

    Grunt::CdKey::Info* info = cdKey.m_keys.New();
    cdKey.m_count = 1;
    memcpy(info, &s_cdKey, sizeof(*info));

    return true;
}

// VERIFY: 2d0bcb86 manual
// OFFSET: 0x006B2F60
void ClientServices::RealmEnumCallback(uint32_t result) {
    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (result == 1) {
        ClientServices::s_currentConnection->Complete(0, AUTH_SESSION_EXPIRED);
        return;
    }

    if (result == 3 || result == 2 || result == 4) {
        ClientServices::s_currentConnection->Complete(0, REALM_LIST_FAILED);
        return;
    }

    ClientConnection* connection = ClientServices::s_currentConnection;

    if (connection->m_statusOp != COP_CONNECT || connection->m_realmConnectPending) {
        if (connection->m_statusOp == COP_GET_REALMS) {
            connection->Complete(1, REALM_LIST_SUCCESS);
        }

        return;
    }

    if (ClientServices::s_loginObj->GetLoginServerType() == 1) {
        if (ClientServices::m_selectRealmInfoValid || ClientServices::SetSelectedRealmInfo(0)) {
            connection->m_realmConnectPending = 1;
            ClientServices::s_loginObj->JoinRealm(ClientServices::m_selectRealmInfo.m_regionId, ClientServices::m_selectRealmInfo.m_battlegroupId, ClientServices::m_selectRealmInfo.m_realmId);
        } else {
            ClientServices::Connection()->Complete(0, REALM_LIST_REALM_NOT_FOUND);
        }
    } else {
        connection->m_realmConnectPending = 1;
        ClientServices::ConnectToSelectedServer();
    }
}

// VERIFY: a4f8699b auto
// OFFSET: 0x006B1260
int32_t ClientConnection::HandleConnect() {
    this->Complete(1, RESPONSE_CONNECTED);
    this->m_connected = 1;
    WardenClient_Initialize();

    return this->NetClient::HandleConnect();
}

// VERIFY: cd1a7c20 auto
// OFFSET: 0x006B12D0
int32_t ClientConnection::HandleDisconnect() {
    if (this->m_cleanup) {
        this->m_cleanup();
        this->m_cleanup = nullptr;
    }

    this->m_statusComplete = 1;
    this->m_connected = 0;
    this->m_loggingOut = 0;
    WardenClient_Destroy();

    return this->NetClient::HandleDisconnect();
}

// VERIFY: c357ca6f auto
// OFFSET: 0x006B1320
int32_t ClientConnection::HandleCantConnect() {
    this->Complete(0, RESPONSE_FAILED_TO_CONNECT);
    this->m_connected = 0;
    WardenClient_Destroy();

    return this->NetClient::HandleCantConnect();
}

// VERIFY: e3e4369c manual
// OFFSET: 0x006B1390
void ClientConnection::Connect() {
    if (this->m_statusComplete != 1) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CGlueMgr::ResetQueueState();
    this->m_realmConnectPending = 0;
    this->Initiate(COP_CONNECT, CSTATUS_CONNECTING, nullptr);

    if (this->m_connected) {
        this->Complete(1, RESPONSE_CONNECTED);
    }

    ClientServices::s_loginObj->GetRealmList();
}

// VERIFY: 3d1c919f manual
// OFFSET: 0x006B2CF0
bool ClientServices::SetSelectedRealmInfo(int32_t force) {
    LoginResponse* response = ClientServices::CreateLoginResponse();

    if (!response) {
        return false;
    }

    for (uint32_t i = 0; i < response->m_realmList.m_count; i++) {
        REALM_INFO* realm = &response->m_realmList.m_data[i];

        if (!SStrCmpUTF8I(realm->m_realmName, ClientServices::GetSelectedRealmName(), STORM_MAX_STR)) {
            if (!(realm->m_flags & 0x1) || force) {
                ClientServices::m_selectRealmInfoValid = true;
                memcpy(&ClientServices::m_selectRealmInfo, realm, sizeof(ClientServices::m_selectRealmInfo));
                return true;
            }

            break;
        }
    }

    ClientServices::m_selectRealmInfoValid = false;

    return false;
}

// VERIFY: 299e1103 manual
// OFFSET: 0x006B2E30
void ClientServices::ConnectToSelectedServer() {
    if (!ClientServices::m_selectRealmInfoValid && !ClientServices::SetSelectedRealmInfo(0)) {
        ClientServices::Connection()->Complete(0, REALM_LIST_REALM_NOT_FOUND);
        return;
    }

    if (ClientServices::Connection()->m_netState != NS_INITIALIZED) {
        ClientServices::Connection()->Complete(0, REALM_LIST_REALM_NOT_FOUND);
        return;
    }

    ClientServices::s_currentConnection->SetSelectedRealm(ClientServices::m_selectRealmInfo.m_regionId, ClientServices::m_selectRealmInfo.m_battlegroupId, ClientServices::m_selectRealmInfo.m_realmId);
    ClientServices::Connection()->NetClient::Connect(ClientServices::m_selectRealmInfo.m_realmAddress);
}

// VERIFY: eaf9daac auto
// OFFSET: 0x006B2DF0
bool ClientServices::SelectRealm(const char* realmName) {
    if (!ClientServices::s_realmNameVar) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_realmNameVar->Set(realmName, true, false, false, true);

    return ClientServices::SetSelectedRealmInfo(1);
}

// VERIFY: 2dcc2486 manual
// OFFSET: 0x006B2EB0
REALM_INFO* ClientServices::GetRealmInfoByIndex(uint32_t index) {
    if (index < ClientServices::CreateLoginResponse()->m_realmList.Count()) {
        return &ClientServices::CreateLoginResponse()->m_realmList[index];
    }

    return nullptr;
}

// VERIFY: 815a18e9 auto
// OFFSET: 0x006B1460
void ClientConnection::HandleAuthResult(int32_t result) {
    this->Complete(result == AUTH_OK, result);
}

// VERIFY: 0d280d76 manual
// OFFSET: 0x006B14C0
void ClientConnection::GetCharacterList() {
    this->Initiate(COP_GET_CHARACTERS, CHAR_LIST_RETRIEVING, nullptr);

    if (this->m_connected) {
        this->RequestCharacterEnum();
    } else {
        this->Cancel(RESPONSE_FAILED_TO_CONNECT);
    }
}

// VERIFY: b7eabfaf auto
// OFFSET: 0x006B1B70
void ClientRealmResponseAdapter::AddOnListReceived(RealmConnection* connection) {
    FrameScript_SignalEvent(GLUE_EVENT_ADDON_LIST_UPDATE, nullptr);
}

// VERIFY: 1ed67c13 manual
// OFFSET: 0x006B1F80
void ClientRealmResponseAdapter::HandleAuthResponse(RealmConnection* connection, uint8_t result) {
    ClientConnection* clientConnection = static_cast<ClientConnection*>(connection);

    if (result == AUTH_WAIT_QUEUE) {
        clientConnection->m_statusOp = COP_WAIT_QUEUE;
        clientConnection->m_errorCode = AUTH_WAIT_QUEUE;
        clientConnection->m_statusComplete = 0;
        LogConnectionStatus(COP_WAIT_QUEUE, AUTH_WAIT_QUEUE, 1, 1);
        CGlueMgr::UpdateWaitQueue(clientConnection->m_queuePosition);
    } else {
        clientConnection->HandleAuthResult(result);
    }

    AccountDataInitialize(1);
}

// VERIFY: d7b1f9d9 manual
// OFFSET: 0x006B2000
void ClientRealmResponseAdapter::CharacterListReceived(RealmConnection* connection, const TSFixedArray<CHARACTER_INFO>& characters, int32_t success) {
    ClientConnection* clientConnection = static_cast<ClientConnection*>(connection);

    if (success) {
        clientConnection->Complete(1, CHAR_LIST_RETRIEVED);
    } else {
        clientConnection->Complete(0, CHAR_LIST_FAILED);
    }
}

// VERIFY: a6d3ccfb auto
// OFFSET: 0x006B1560
int32_t ClientConnection::EnumerateCharacters(void (*callback)(CHARACTER_INFO& info, void* param), void* param) {
    if (!callback) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    int32_t count = this->m_characterList.Count();

    for (int32_t i = 0; i < count; i++) {
        callback(this->m_characterList.m_data[i], param);
    }

    return count;
}

// VERIFY: 2ff353bc auto
// OFFSET: 0x006B0F60
int32_t ClientServices::ValidDisconnect(const void* connection) {
    if (!connection || !ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return connection == ClientServices::s_currentConnection;
}

// VERIFY: a5ac59f0 auto
// OFFSET: 0x006B1010
const char* ClientServices::GetAccountName() {
    return ClientServices::s_accountName[0] ? ClientServices::s_accountName : nullptr;
}

// VERIFY: f57b2dcd auto
// OFFSET: 0x006B0FE0
void ClientServices::SetAccountName(const char* accountName) {
    if (!accountName) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    SStrCopy(ClientServices::s_accountName, accountName, sizeof(ClientServices::s_accountName));
}

// VERIFY: 09816eb7 auto
// OFFSET: 0x006B1020
void ClientServices::SetCharacterInfo(const CHARACTER_INFO* info) {
    if (!info) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    memcpy(&ClientServices::s_characterInfo, info, sizeof(ClientServices::s_characterInfo));
}

// VERIFY: 68b19d7a auto
// OFFSET: 0x006B1050
CHARACTER_INFO* ClientServices::GetCharacterInfo() {
    return &ClientServices::s_characterInfo;
}

// VERIFY: 9c058f7c auto
// OFFSET: 0x006B1060
const char* ClientServices::GetCharacterName() {
    return ClientServices::s_characterInfo.m_name[0] ? ClientServices::s_characterInfo.m_name : nullptr;
}

// VERIFY: f259cd0a auto
// OFFSET: 0x006B15C0
void ClientConnection::HandleCharacterCreate(uint8_t result) {
    this->Complete(result == CHAR_CREATE_SUCCESS, result);
}

// VERIFY: 28ca124a manual
// OFFSET: 0x006B1620
void ClientConnection::RequestCharacterCreate(const CHARACTER_CREATE_INFO& info) {
    this->Initiate(COP_CREATE_CHARACTER, CHAR_CREATE_IN_PROGRESS, nullptr);

    if (!this->m_connected) {
        this->Cancel(RESPONSE_FAILED_TO_CONNECT);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAR_CREATE));
    msg.PutString(info.m_name);
    msg.Put(info.m_raceID);
    msg.Put(info.m_classID);
    msg.Put(info.m_sexID);
    msg.Put(info.m_skinID);
    msg.Put(info.m_faceID);
    msg.Put(info.m_hairStyleID);
    msg.Put(info.m_hairColorID);
    msg.Put(info.m_facialHairStyleID);
    msg.Put(info.m_outfitID);
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: ac426310 manual
// OFFSET: 0x006B1790
void ClientConnection::CharacterLogin(WOWGUID guid, int32_t a2) {
    this->Initiate(COP_LOGIN_CHARACTER, CHAR_LOGIN_IN_PROGRESS, nullptr);

    if (!this->m_connected) {
        this->Cancel(RESPONSE_FAILED_TO_CONNECT);
        return;
    }

    this->RequestCharacterLogin(guid, a2);
    this->m_characterLoggedIn = 1;
}

// VERIFY: 74e67f09 auto
// OFFSET: 0x006B0930
int32_t ClientConnection::CharacterLoggingOut() {
    return this->m_loggingOut;
}

// VERIFY: 10f2b254 auto
// OFFSET: 0x006B1840
void ClientConnection::CharacterSetInGame(int32_t inGame) {
    this->Complete(1, CHAR_LOGIN_SUCCESS);
    this->m_inGame = inGame;

    if (this->m_loggingOut) {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOGOUT_CANCEL, nullptr);
        this->m_loggingOut = 0;
    }
}

// VERIFY: 4b884dc6 auto
// OFFSET: 0x006B1A10
void ClientConnection::HandleCharacterDelete(uint8_t result) {
    this->Complete(result == CHAR_DELETE_SUCCESS, result);
}

// VERIFY: 5c3a6030 manual
// OFFSET: 0x006B1A70
void ClientConnection::RequestCharacterDelete(WOWGUID guid) {
    this->Initiate(COP_DELETE_CHARACTER, CHAR_DELETE_IN_PROGRESS, nullptr);

    if (!this->m_connected) {
        this->Cancel(RESPONSE_FAILED_TO_CONNECT);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAR_DELETE));
    msg.Put(guid.m_guid);
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: 0fdcb75a auto
// OFFSET: 0x006B1B80
void ClientRealmResponseAdapter::GameServerResult(RealmConnection* connection, const char* message1, const char* message2, const char* message3) {
}

// VERIFY: 13314496 auto
// OFFSET: 0x006B1B90
void ClientRealmResponseAdapter::LoginServerResult(LOGIN_STATE state, LOGIN_RESULT result, const char* message1, const char* message2, const char* message3) {
}

// VERIFY: 851669dd auto
// OFFSET: 0x00632050
void ClientRealmResponseAdapter::RealmListReceived(TSFixedArray<REALM_INFO>& realms) {
}

// VERIFY: af92e680 auto
// OFFSET: 0x00653A10
void ClientRealmResponseAdapter::MessageReceived(RealmConnection* connection, uint32_t message) {
}

// VERIFY: e5acd1b5 auto
// OFFSET: 0x006B1B90
void ClientRealmResponseAdapter::OnChatReceived(RealmConnection* connection, WOWGUID guid, const char* text, uint8_t type) {
}

// VERIFY: 01b28e4e auto
// OFFSET: 0x006B1B80
void ClientRealmResponseAdapter::OnAccountOnline(RealmConnection* connection, int32_t unused_a2, WOWGUID guid) {
}

// VERIFY: 93d4ae81 auto
// OFFSET: 0x006B1BA0
void ClientRealmResponseAdapter::OnAFKMonitorInfoResponse(RealmConnection* connection, WOWGUID guid, int32_t unused_a3, int32_t unused_a4, int32_t unused_a5, int32_t unused_a6) {
}

// VERIFY: a13283da auto
// OFFSET: 0x00632050
void ClientRealmResponseAdapter::HandleConnect(RealmConnection* connection) {
}

// VERIFY: be0ed1e5 auto
// OFFSET: 0x00632050
void ClientRealmResponseAdapter::HandleDisconnect(RealmConnection* connection) {
}

// VERIFY: bbda4733 auto
// OFFSET: 0x00632050
void ClientRealmResponseAdapter::HandleCantConnect(RealmConnection* connection) {
}

// VERIFY: f198842b auto
// OFFSET: 0x006B1070
uint8_t ClientServices::GetCharacterRace() {
    return ClientServices::s_characterInfo.m_raceID;
}

// VERIFY: c50dd741 auto
// OFFSET: 0x006B1080
uint8_t ClientServices::GetCharacterClass() {
    return ClientServices::s_characterInfo.m_classID;
}

// VERIFY: aad17c20 auto
// OFFSET: 0x006B1090
uint8_t ClientServices::GetCharacterSex() {
    return ClientServices::s_characterInfo.m_sexID;
}

// VERIFY: ecaae621 auto
// OFFSET: 0x006B0900
void ClientConnection::HandleLogoutAbortAck() {
    if (this->m_loggingOut) {
        FrameScript_SignalEvent(SCRIPT_EVENT_LOGOUT_CANCEL, nullptr);
        this->m_loggingOut = 0;
    }
}

// VERIFY: 52ab6642 manual
// OFFSET: 0x006B0F00
const char* ClientServices::GetLoginServer() {
    if (ClientServices::s_loginObj->GetLoginServerType() == 1) {
        return ClientServices::s_realmListBNVar->m_stringValue.m_str;
    }

    return ClientServices::s_realmListVar->m_stringValue.m_str;
}

// VERIFY: 3cff0526 manual
// OFFSET: 0x006B18C0
void ClientConnection::CharacterAbortLogout() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LOGOUT_CANCEL));
    msg.Finalize();
    this->Send(&msg);
    this->m_loggingOut = 0;
}

// VERIFY: d41a5153 auto
// OFFSET: 0x006B2180
void ClientConnection::HandleLogoutComplete() {
    if (this->m_inGame) {
        this->CharacterSetInGame(0);
    }

    this->m_characterLoggedIn = 0;

    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this == ClientServices::s_currentConnection) {
        ClientDestroyGame(1, 1, 0);
    }

    this->m_loggingOut = 0;

    if (this->m_quitAfterLogout) {
        ClientPostClose(0);
    }
}

// VERIFY: a00d1e9e manual
// OFFSET: 0x006B3070
void ClientServices::JoinRealmResult(uint32_t realmId, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    if (ClientServices::s_loginObj->GetLoginServerType() != 1) {
        return;
    }

    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!ClientServices::m_selectRealmInfoValid || !realmId || ClientServices::m_selectRealmInfo.m_realmId != realmId) {
        ClientServices::s_currentConnection->Complete(0, RESPONSE_FAILED_TO_CONNECT);
        return;
    }

    LoginData loginData;
    SStrCopy(loginData.m_account, ClientServices::s_loginObj->m_username, sizeof(loginData.m_account));
    loginData.m_loginServerID = ClientServices::s_loginObj->GetServerId();
    memcpy(loginData.m_sessionKey, ClientServices::s_loginObj->m_sessionKey, sizeof(loginData.m_sessionKey));
    loginData.m_loginServerType = ClientServices::s_loginObj->GetLoginServerType();
    ClientServices::Connection()->SetLoginData(&loginData);

    if (a2 || a3) {
        SStrPrintf(ClientServices::m_selectRealmInfo.m_realmAddress, sizeof(ClientServices::m_selectRealmInfo.m_realmAddress), "%d.%d.%d.%d:%u", (a2 >> 24) & 0xFF, (a2 >> 16) & 0xFF, (a2 >> 8) & 0xFF, a2 & 0xFF, a3);
    }

    ClientServices::ConnectToSelectedServer();
}

// VERIFY: bb0724f1 auto
// OFFSET: 0x006B10A0
uint8_t ClientServices::GetCharacterLevel() {
    return ClientServices::s_characterInfo.m_experienceLevel;
}

// VERIFY: 4ed26edd manual
// OFFSET: 0x006B1930
void ClientConnection::CharacterLogout(bool quit, bool instant) {
    if ((this->m_loggingOut && !instant) || !this->m_inGame) {
        return;
    }

    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this != ClientServices::s_currentConnection) {
        this->CharacterSetInGame(0);
        this->m_characterLoggedIn = 0;
        return;
    }

    this->m_quitAfterLogout = quit;

    if (!instant) {
        this->m_loggingOut = 1;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(instant ? CMSG_PLAYER_LOGOUT : CMSG_LOGOUT_REQUEST));
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: a547ad11 auto
// OFFSET: 0x006B0F30
int32_t ClientServices::LaunchURL(const char* url) {
    return OsLaunchURL(url);
}

// VERIFY: 12c793a8 auto
// OFFSET: 0x006B0AA0
void ClientServices::LoginEnterToken(const char* token) {
    if (!ClientServices::s_loginObj) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_loginObj->EnterToken(token);
}

// VERIFY: e448981b manual
// OFFSET: 0x006B08B0
void ClientConnection::HandleLogoutResponse(int32_t result, int32_t instant) {
    if (result) {
        CGGameUI::DisplayError(ERR_LOGOUT_FAILED);
        this->m_loggingOut = 0;
    } else if (!instant) {
        FrameScript_SignalEvent(this->m_quitAfterLogout ? SCRIPT_EVENT_PLAYER_QUITING : SCRIPT_EVENT_PLAYER_CAMPING, nullptr);
    }
}

// VERIFY: fb46efae auto
// OFFSET: 0x006B09A0
void ClientServices::LoginEnterPIN(int32_t count, uint8_t* digits) {
    if (!ClientServices::s_loginObj) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_loginObj->EnterPIN(count, digits);
}

// VERIFY: 82562748 auto
// OFFSET: 0x006B09E0
void ClientServices::LoginEnterMatrix(uint8_t value) {
    if (!ClientServices::s_loginObj) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_loginObj->EnterMatrix(value);
}

// VERIFY: 43f73fd2 manual
// OFFSET: 0x006B0A10
void ClientServices::LoginRevertMatrix() {
    if (!ClientServices::s_loginObj) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_loginObj->RevertMatrix();
}

// VERIFY: 46fc579e manual
// OFFSET: 0x006B0A40
void ClientServices::LoginCommitMatrix() {
    if (!ClientServices::s_loginObj) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_loginObj->CommitMatrix();
}

// VERIFY: 9c7ee31c manual
// OFFSET: 0x006B0A70
void ClientServices::LoginFinalizeMatrix() {
    if (!ClientServices::s_loginObj) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClientServices::s_loginObj->FinalizeMatrix();
}

// VERIFY: 664db17c auto
// OFFSET: 0x006B0AD0
void ClientServices::FileAccept(bool accept, uint64_t offset) {
    ClientServices::s_loginObj->FileAccept(accept, offset);
    ClientServices::s_newLogin = true;
}

// VERIFY: 93a34d92 auto
// OFFSET: 0x006B0B00
void ClientServices::StartDownload() {
    ClientServices::s_newLogin = true;
}

// VERIFY: c07ca7da auto
// OFFSET: 0x006B0B10
void ClientServices::SendSurveyData(uint16_t size, uint8_t* data) {
    ClientServices::s_loginObj->SendSurveyData(size, data);
}

// VERIFY: dd529696 manual
// OFFSET: 0x006B0B30
void ClientServices::SendSurveyFailed() {
    ClientServices::s_loginObj->SendSurveyFailed();
}

// VERIFY: 5b32617c manual
// OFFSET: 0x006B0B40
int32_t ClientServices::GetSurveyId() {
    return ClientServices::s_loginObj->GetSurveyId();
}

// VERIFY: e32f76a9 auto
// OFFSET: 0x006B2070
void ClientConnection::HandleCharacterLoginFailed(uint8_t result) {
    if (this->m_inGame) {
        this->CharacterSetInGame(0);
    }

    this->m_characterLoggedIn = 0;

    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this == ClientServices::s_currentConnection) {
        ClientDestroyGame(1, 1, 1);
    }

    switch (result) {
        case 1:
            this->Cancel(CHAR_LOGIN_NO_WORLD);
            break;

        case 2:
            this->Cancel(CHAR_LOGIN_DUPLICATE_CHARACTER);
            break;

        case 3:
            this->Cancel(CHAR_LOGIN_NO_INSTANCES);
            break;

        case 4:
            this->Cancel(CHAR_LOGIN_DISABLED);
            break;

        case 5:
            this->Cancel(CHAR_LOGIN_NO_CHARACTER);
            break;

        case 6:
            this->Cancel(CHAR_LOGIN_LOCKED_FOR_TRANSFER);
            break;

        case 7:
            this->Cancel(CHAR_LOGIN_LOCKED_BY_BILLING);
            break;

        case 8:
            this->Cancel(CHAR_LOGIN_LOCKED_BY_MOBILE_AH);
            break;

        default:
            this->Cancel(CHAR_LOGIN_FAILED);
            break;
    }
}
