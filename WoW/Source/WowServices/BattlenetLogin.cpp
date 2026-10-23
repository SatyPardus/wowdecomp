#include "BattlenetLogin.h"
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/System/Version/Version.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/OsSecureRandom.h"
#include "Engine/Source/Os/OsTcp.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Battlenet/Tumor/TumorManager.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Ui/BattlenetUI.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "GameVersion.h"

// STATIC: 0x00B24B94
BattlenetAllocator s_battlenetAllocator;

// STATIC: 0x00D45C98
Battlenet::TumorManager* g_tumorManager;

// STATIC: 0x00D45C9C
bool BattlenetLogin::s_monolithicComponent;

// STATIC: 0x00D45CB0
char BattlenetLogin::s_monolithicPlatform[5];

// VERIFY: 4bbf2db9 auto
// OFFSET: 0x008C51D0
void BattlenetAssert(const char* message, const char* file, int32_t line) {
    SErrDisplayErrorFmt(0x85100000, file ? file : "", line, 0, 1, "Internal Battle.net Error: %s", message ? message : "");
    abort();
}

// VERIFY: 9e2345e7 auto
// OFFSET: 0x008C5140
void* BattlenetAllocator::Alloc(uint32_t size) {
    return SMemAlloc(size, __FILE__, __LINE__, 0);
}

// VERIFY: 04e99d71 auto
// OFFSET: 0x008C5160
void BattlenetAllocator::Free(void* memory) {
    SMemFree(memory, __FILE__, __LINE__, 0);
}

// VERIFY: 93afef9d auto
// OFFSET: 0x008C5180
bool BattlenetApplication::Send(uint32_t connection, const void* data, uint32_t size) {
    if (connection != this->m_connection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return this->m_wowConnection->Send(static_cast<uint8_t*>(const_cast<void*>(data)), size, true);
}

// VERIFY: f8dc5f8a manual
// OFFSET: 0x008C51C0
uint64_t BattlenetApplication::GetCurrentTick() const {
    return OsGetAsyncTimeMs();
}

// VERIFY: e3b5cfcb manual
// OFFSET: [0x008C5210, 0x008C6A10]
BattlenetApplication::~BattlenetApplication() {
    if (this->m_wowConnection) {
        this->m_wowConnection->Disconnect();
        this->m_wowConnection->SetResponse(nullptr, false);
        this->m_wowConnection->Release();
        this->m_wowConnection = nullptr;
    }
}

// VERIFY: d7bf219c auto
// OFFSET: 0x008C5270
uint8_t BattlenetLogin::GetCharacterCount(uint32_t region, uint32_t battlegroup, uint32_t id) {
    RealmCharacterCount* counts = this->m_realmCharacterCounts;

    if (!counts) {
        return 0;
    }

    for (uint32_t i = 0; i < this->m_realmCharacterCountCount; i++, counts++) {
        if (counts->m_region == region && counts->m_battlegroup == battlegroup && counts->m_id == id) {
            return static_cast<uint8_t>(counts->m_count);
        }
    }

    return 0;
}

// VERIFY: b1cb90f6 auto
// OFFSET: 0x008C52E0
bool BattlenetLogin::Connected(const NETADDR& address) {
    char text[256];
    char addressString[32];

    this->m_loggedOn = true;

    SStrPrintf(text, sizeof(text), "Connected to %s", OsNetAddrToStr(&address, addressString, sizeof(addressString)));
    ConsoleWrite(text, DEFAULT_COLOR);

    if (!g_tumorManager || !this->m_application) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::SystemClient& client = g_tumorManager->m_tumor;
    client.Connected(this->m_application->m_connection, this->m_application->m_localAddress);

    return true;
}

// VERIFY: a95ceed4 manual
// OFFSET: 0x008C5370
bool BattlenetLogin::Disconnected() {
    if (!g_tumorManager || !this->m_application) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::SystemClient& client = g_tumorManager->m_tumor;
    client.Disconnected(this->m_application->m_connection);

    return false;
}

// VERIFY: 274186f5 manual
// OFFSET: 0x008C53B0
void BattlenetLogin::LogonResultBN(int32_t result) {
    this->m_logonResult = result;

    switch (result) {
        case 0:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_AUTHENTICATED, LOGIN_OK, nullptr, 0);
            break;

        case 103:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_BAD_SERVER_PROOF, nullptr, 0);
            break;

        case 104:
        case 108:
        case 116:
        case 201:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_UNKNOWN_ACCOUNT, nullptr, 0);
            break;

        case 105:
        case 106:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_DISCONNECTED, nullptr, 0);
            break;

        case 107:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NO_GAME_ACCOUNT, nullptr, 0);
            break;

        case 109:
        case 110:
        case 111:
        case 112:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_BADVERSION, nullptr, 0);
            break;

        case 113:
        case 200:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_AUTH_OUTAGE, nullptr, 0);
            break;

        case 115:
        case 205:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_ALREADYONLINE, nullptr, 0);
            break;

        case 123:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_TOO_FAST, nullptr, 0);
            break;

        case 124:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_USE_GRUNT, nullptr, 0);
            break;

        case 140:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NO_GAME_ACCOUNTS_IN_REGION, nullptr, 0);
            break;

        case 141:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_ACCOUNT_LOCKED, nullptr, 0);
            break;

        case 202:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_BANNED, nullptr, 0);
            break;

        case 203:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_SUSPENDED, nullptr, 0);
            break;

        case 204:
        case 212:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_GAME_ACCOUNT_LOCKED, nullptr, 0);
            break;

        case 206:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NOTIME, nullptr, 0);
            break;

        case 207:
        case 208:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_EXPIRED, nullptr, 0);
            break;

        case 209:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_PARENTALCONTROL, nullptr, 0);
            break;

        case 210:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_TRIAL_EXPIRED, nullptr, 0);
            break;

        case 211:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_ANTI_INDULGENCE, nullptr, 0);
            break;

        default:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_FAILED, nullptr, 0);
            break;
    }

    this->m_surveyPending = false;
}

// VERIFY: 4668fa22 auto
// OFFSET: 0x008C5BA0
static void GetComponentVersion(const char* name, uint32_t& version, const char* data, uint32_t size) {
    XMLTree* tree = XMLTree_Load(data, size);

    for (XMLNode* node = XMLTree_GetRoot(tree)->m_child; node; node = node->m_next) {
        if (!SStrCmpI(node->m_name.m_str, "component", STORM_MAX_STR)) {
            const char* componentName = node->GetAttributeByName("name");
            const char* componentVersion = node->GetAttributeByName("version");

            if (componentName && componentVersion && !SStrCmpI(componentName, name, STORM_MAX_STR)) {
                version = SStrToInt(componentVersion);
            }
        }
    }

    XMLTree_Free(tree);
}

// VERIFY: 13a1fff7 auto
// OFFSET: 0x008C5C50
static uint32_t GetWowDataVersion() {
    uint32_t version = GAME_BUILD;
    void* data;
    uint32_t size;

    if (!SFileLoadFileEx(nullptr, "component.wow-data.txt", &data, &size, 0, 0, nullptr) || !data) {
        return GAME_BUILD;
    }

    GetComponentVersion("wow-data", version, static_cast<const char*>(data), size);
    SMemFree(data, __FILE__, __LINE__, 0);

    return version;
}

// VERIFY: c6db8f62 auto
// OFFSET: 0x008C5CC0
static uint32_t GetWowLangVersion(const char* locale) {
    char path[256];
    char fileName[32];
    char componentName[32];
    uint32_t version = GAME_BUILD;
    SArchive* archive = nullptr;

    if (SStrCmp(locale, s_localeCode, STORM_MAX_STR)) {
        sprintf(path, "Data/%s/patch-%s-2.MPQ", locale, locale);

        if (!OsFileExists(path)) {
            sprintf(path, "Data/%s/patch-%s.MPQ", locale, locale);

            if (!OsFileExists(path)) {
                sprintf(path, "Data/%s/locale-%s.MPQ", locale, locale);
            }
        }

        if (!SFile::OpenArchive(path, 0, 0x800, &archive)) {
            return GAME_BUILD;
        }
    }

    sprintf(fileName, "component.wow-%s.txt", locale);

    void* data;
    uint32_t size;

    if (!SFile::ReadFile(archive, fileName, &data, &size, 0, 0, nullptr) || !data) {
        if (archive) {
            SFile::CloseArchive(archive);
        }

        return GAME_BUILD;
    }

    sprintf(componentName, "wow-%s", locale);
    GetComponentVersion(componentName, version, static_cast<const char*>(data), size);
    SMemFree(data, __FILE__, __LINE__, 0);

    if (archive) {
        SFile::CloseArchive(archive);
    }

    return version;
}

// VERIFY: 552847c3 manual
// OFFSET: 0x008C5F60
void BattlenetLogin::SetAuthModuleMatrixInfo(const char* info, const char* challenges) {
    this->m_matrixWidth = SStrToUnsigned(info);

    const char* cursor = SStrChr(info, ',');

    if (!cursor) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    cursor++;
    this->m_matrixHeight = SStrToUnsigned(cursor);

    cursor = SStrChr(cursor, ',');

    if (!cursor) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    cursor++;
    this->m_matrixMinDigitCount = SStrToUnsigned(cursor);

    cursor = SStrChr(cursor, ',');

    if (!cursor) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    cursor++;
    this->m_matrixMaxDigitCount = SStrToUnsigned(cursor);

    cursor = SStrChr(cursor, ',');

    if (!cursor) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    cursor++;
    this->m_matrixFlipCoords = SStrToUnsigned(cursor) != 0;

    cursor = SStrChr(cursor, ',');

    if (!cursor) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    cursor++;
    this->m_matrixChallengeCount = SStrToUnsigned(cursor);

    if (SStrChr(cursor, ',')) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!this->m_matrixWidth || !this->m_matrixHeight || !this->m_matrixMinDigitCount || !this->m_matrixMaxDigitCount || !this->m_matrixChallengeCount) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this->m_matrixData) {
        delete[] this->m_matrixData;
    }

    this->m_matrixData = new (__FILE__, __LINE__) uint32_t[this->m_matrixChallengeCount];

    for (uint32_t i = 0; i < this->m_matrixChallengeCount; i++) {
        if (!challenges) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        if (i) {
            challenges++;
        }

        this->m_matrixData[i] = SStrToUnsigned(challenges);
        challenges = SStrChr(challenges, ',');
    }

    if (challenges) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->m_matrixEnabled = true;
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_MATRIX, LOGIN_OK, nullptr, 0);
}

// VERIFY: 8e82c4f5 auto
// OFFSET: 0x008C61E0
void BattlenetLogin::SetAuthModulePinInfo(const char* keyMap) {
    if (!keyMap || SStrLen(keyMap) != 10) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    for (uint32_t i = 0; i < 10; i++) {
        if (!isdigit(static_cast<uint8_t>(keyMap[i]))) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        this->m_pinKeyMap[1 + i] = keyMap[i] - '0';
    }

    this->m_pinKeyMap[0] = 1;
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_PIN, LOGIN_OK, nullptr, 0);
}

// VERIFY: cf41e714 manual
// OFFSET: 0x008C62B0
void BattlenetLogin::SetTokenInfo(bool enabled, uint8_t required) {
    if (enabled) {
        this->m_tokenRequired = true;
        this->unused_10D5 = required;
        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_TOKEN, LOGIN_OK, nullptr, 0);
    }
}

// VERIFY: f11b22e7 auto
// OFFSET: 0x008C6330
static bool IsInOrEnteringWorld() {
    return CGGameUI::s_inWorld || CGlueMgr::m_idleState == IDLE_ENTER_WORLD;
}

// VERIFY: 4f797e26 auto
// OFFSET: 0x008C6350
void BattlenetLogin::LogConnectionStatus(const char* format, ...) {
    char buffer[2048];

    if (format && *format) {
        va_list args;
        va_start(args, format);
        SStrVPrintf(buffer, sizeof(buffer), format, args);
        CGlueMgr::LogConnectionStatus(buffer);
    }
}

// VERIFY: a78fd966 auto
// OFFSET: 0x008C6980
void BattlenetApplication::Disconnect(uint32_t connection) {
    if (connection != this->m_connection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this->m_wowConnection) {
        this->m_wowConnection->Disconnect();
        BattlenetLogin::LogConnectionStatus("Disconnected from %x", this->m_wowConnection->m_connectAddress);
    }
}

// VERIFY: 36d5c66c auto
// OFFSET: 0x008C69D0
BattlenetApplication::BattlenetApplication() {
    this->m_eventId = Battlenet::Event::APP;
    this->m_login = nullptr;
    this->m_wowConnection = nullptr;
    this->m_connection = 0;
}

// VERIFY: cdc67c71 manual
// OFFSET: 0x008C6A60
bool BattlenetLogin::CantConnect() {
    ConsoleWrite("BattlenetLogin was unable to connect to Sunken", DEFAULT_COLOR);
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_SERVER_DOWN, nullptr, 0);
    this->m_logonStatus = 1;
    this->LogonResultBN(2);
    this->m_loggedOn = false;
    this->m_online = false;

    return false;
}

// VERIFY: c20feb7e manual
// OFFSET: 0x008C6B30
const char* BattlenetLogin::GetPatchInstruction(uint32_t index) {
    if (index < this->m_patchInstructions.m_count) {
        return this->m_patchInstructions.m_data[index].m_patch;
    }

    return nullptr;
}

// VERIFY: 4dc6c4b9 manual
// OFFSET: 0x008C6D10
uint32_t BattlenetApplication::Connect(const Battlenet::IP4::AddressPort& address, const uint32_t& tendrilId) {
    char buffer[64];

    this->m_connection = OsGetAsyncTimeMs();
    this->m_remoteAddress = address;

    if (!this->m_wowConnection) {
        this->m_wowConnection = new (__FILE__, __LINE__) WowConnection(this, nullptr);
        this->m_wowConnection->SetType(WOWC_TYPE_STREAM);
    }

    sprintf(buffer, "%d.%d.%d.%d:%d", address.m_address[0], address.m_address[1], address.m_address[2], address.m_address[3], address.m_port[1] + (address.m_port[0] << 8));
    BattlenetLogin::LogConnectionStatus("Connecting to %s", buffer);
    this->m_wowConnection->Connect(buffer, 5000);

    return this->m_connection;
}

// VERIFY: 9d0ccd2a auto
// OFFSET: 0x00532A60
uint32_t BattlenetApplication::OpenDatagram(int32_t port, const uint32_t& tendrilId) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 2c202c4c auto
// OFFSET: 0x00532A60
void BattlenetApplication::CloseDatagram(uint32_t connection) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 30835edb auto
// OFFSET: 0x00532A60
bool BattlenetApplication::SendDatagram(uint32_t connection, const void* data, uint32_t size, const Battlenet::IP4::AddressPort& address) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 2a2561a1 auto
// OFFSET: 0x00532A60
uint64_t BattlenetApplication::GetCurrentTime() const {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 2aca7dfc auto
// OFFSET: 0x00653A10
void BattlenetApplication::LogLevel(const char* message, int32_t level) {
}

// VERIFY: 6b1ff200 auto
// OFFSET: 0x004899F0
Battlenet::ApplicationFile* BattlenetApplication::OpenApplicationFile(const char* name) {
    return nullptr;
}

// VERIFY: cada2529 auto
// OFFSET: 0x008813A0
void BattlenetApplication::CloseApplicationFile(Battlenet::ApplicationFile* file) {
}

// VERIFY: 5b409c1d auto
// OFFSET: 0x0047ADD0
void BattlenetApplication::WCMessageReady(WowConnection* connection, uint32_t timeStamp, CDataStore* msg) {
}

// VERIFY: b99dcdf7 manual
// OFFSET: 0x008C7030
void BattlenetLogin::HandleUpdateInvoke(const Battlenet::Event::Update::Invoke* invoke) {
    char program[5];
    char component[5];

    this->m_patchInstructionLock.Enter();

    PatchInstruction* instruction = this->m_patchInstructions.New();
    SStrCopy(instruction->m_patch, invoke->m_patch.m_data, sizeof(instruction->m_patch));

    invoke->m_program.ToString(program, sizeof(program));
    invoke->m_component.ToString(component, sizeof(component));
    BattlenetLogin::LogConnectionStatus("program=%s component=%s patch=%s", program, component, instruction->m_patch);

    this->m_patchInstructionLock.Leave();

    if (!invoke->unk_010C) {
        SStrCopy(this->m_fileName, "Patch", sizeof(this->m_fileName));
        this->m_fileSize = 0;
        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_DOWNLOADFILE, LOGIN_OK, nullptr, 0);
    }
}

// VERIFY: 3d0e41a1 manual
// OFFSET: 0x008C7130
void BattlenetLogin::BuildRealmListResult() {
    CDataStore* msg = new (__FILE__, __LINE__) CDataStore();
    uint16_t count = 0;

    msg->Put(count);

    this->m_realmLock.Enter();

    for (BN_REALM* realm = this->m_realms.Head(); realm; realm = this->m_realms.Next(realm)) {
        REALM_INFO& info = realm->m_info;

        msg->Put(info.m_type);
        msg->Put(info.m_otherFlags);
        msg->Put(info.m_flags);
        msg->PutString(info.m_realmName);
        msg->PutString(info.m_realmAddress);
        msg->Put(info.m_population);
        msg->Put(info.m_numCharacters);
        msg->Put(info.m_category);
        msg->Put(info.m_regionId);
        msg->Put(info.m_battlegroupId);
        msg->Put(info.m_realmId);
        msg->Put(info.m_build);

        if (info.m_flags & 0x4) {
            msg->Put(info.m_versionMajor);
            msg->Put(info.m_versionMinor);
            msg->Put(info.m_versionRevision);
            msg->Put(info.m_versionBuild);
        }

        count++;
    }

    this->m_realmLock.Leave();

    msg->Put(static_cast<uint16_t>(0));
    msg->Set(0, count);
    msg->Finalize();

    this->RealmListResult(msg);

    delete msg;
}

// VERIFY: 90b0ddd4 manual
// OFFSET: 0x008C7300
void BattlenetLogin::JoinRealm(uint32_t region, uint32_t battlegroup, uint32_t id) {
    this->m_joinRealmId = id;

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Realm::Request* request = invoke.Set<6, Battlenet::Realm::Request>();
    BSN::Optional<Battlenet::Realm::Address>* join = request->Set<1, BSN::Optional<Battlenet::Realm::Address>>();

    if (region || battlegroup || id) {
        join->m_exists = true;
        join->m_value.m_region = region;
        join->m_value.m_battlegroup = battlegroup;
        join->m_value.m_id = id;
    } else {
        join->m_exists = false;
    }

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: 102510bd manual
// OFFSET: 0x008C73D0
void BattlenetLogin::GetLogonMethod() {
    if (!this->m_password) {
        return;
    }

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::RequestPassword;

    Battlenet::Authentication::ModuleParam* param = message->m_params.New();
    *param->Set<0, BSN::String<256>>() = this->m_password;

    memset(this->m_password, 0, SStrLen(this->m_password));
    SMemFree(this->m_password, __FILE__, __LINE__, 0);
    this->m_password = nullptr;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: a8bc2c7e auto
// OFFSET: 0x008C7530
void BattlenetLogin::Logoff() {
    Battlenet::Event::Creep::Invoke invoke;
    invoke.Set<9, Battlenet::Event::Creep::Logoff>();

    if (g_tumorManager) {
        g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
    }
}

// VERIFY: b6892c7c manual
// OFFSET: 0x008C7590
Battlenet::TumorManager* BattlenetLogin::CreateTumorManager() {
    if (g_tumorManager) {
        BattlenetUI::Uninit();
    }

    const char* program = "WoW";

    if (SFile::IsTrial()) {
        program = "WoWS";
    }

    Battlenet::Authentication::Settings settings;

    if (settings.m_seedLength < 32) {
        memset(&settings.m_seed[settings.m_seedLength], 0, 32 - settings.m_seedLength);
    }

    settings.m_seedLength = 32;
    OsSecureRandom(settings.m_seed, 32);

    settings.m_program = BSN::FourCC("WoW");
    settings.m_platform = BSN::FourCC("Win");
    settings.m_locale.m_value = SFile::s_locale;
    settings.m_systemLocale.m_value = SFile::s_locale;

    BattlenetLogin::LogConnectionStatus("Login program=%s platform=%s locale=%s", "WoW", "Win", s_localeCode);

    if (BattlenetLogin::s_monolithicComponent) {
        BattlenetLogin::s_monolithicComponent = false;
        SStrCopy(BattlenetLogin::s_monolithicPlatform, "Win", sizeof(BattlenetLogin::s_monolithicPlatform));
        SStrCopy(&BattlenetLogin::s_monolithicPlatform[1], &s_localeCode[1], 4);

        Battlenet::Authentication::Component* component = settings.m_components.New();
        component->m_program = BSN::FourCC(program);
        component->m_platform = BSN::FourCC(BattlenetLogin::s_monolithicPlatform);
        component->m_version = GAME_BUILD;
    } else {
        Battlenet::Authentication::Component* component = settings.m_components.New();
        component->m_program = BSN::FourCC(program);
        component->m_platform = BSN::FourCC("Win");
        component->m_version = GAME_BUILD;

        component = settings.m_components.New();
        component->m_program = BSN::FourCC("WoW");
        component->m_platform = BSN::FourCC("base");
        component->m_version = GetWowDataVersion();

        uint32_t localeCount = 0;

        for (int32_t i = 0; i < NUM_DB_LOCALES; i++) {
            if (g_hasIsoLocale[i]) {
                component = settings.m_components.New();
                component->m_program = BSN::FourCC("WoW");

                const char* locale = g_localeNames[i];
                component->m_platform = BSN::FourCC(locale);
                localeCount++;
                component->m_version = GetWowLangVersion(locale);
            }
        }

        if (!localeCount) {
            component = settings.m_components.New();
            component->m_program = BSN::FourCC("WoW");

            const char* locale = s_iniLocaleNames[s_locale];
            component->m_platform = BSN::FourCC(locale);
            component->m_version = GetWowLangVersion(locale);
        }

        component = settings.m_components.New();
        component->m_program = BSN::FourCC("Tool");
        component->m_platform = BSN::FourCC("Win");

        uint32_t launcherVersion = 0;
        Blizzard::Version::FileVersion version;

        if (Blizzard::Version::GetFileVersion("Launcher.exe", version)) {
            launcherVersion = version.m_revision;
        }

        component->m_version = launcherVersion;
    }

    this->m_patchInstructionLock.Enter();
    this->m_patchInstructions.Clear();
    this->m_patchInstructionLock.Leave();

    for (uint32_t i = 0; i < settings.m_components.m_count; i++) {
        char componentProgram[5];
        char componentPlatform[5];

        Battlenet::Authentication::Component& component = settings.m_components[i];
        component.m_program.ToString(componentProgram, sizeof(componentProgram));
        component.m_platform.ToString(componentPlatform, sizeof(componentPlatform));
        BattlenetLogin::LogConnectionStatus("Component %s.%s.%d", componentProgram, componentPlatform, component.m_version);
    }

    return Battlenet::TumorManager::Create(*this->m_application, settings);
}

// VERIFY: a05b79fa manual
// OFFSET: 0x008C7AB0
void BattlenetLogin::EnterPIN(int32_t count, uint8_t* digits) {
    if (!this->m_pinKeyMap[0]) {
        return;
    }

    this->m_pinKeyMap[0] = 0;

    if (static_cast<uint32_t>(count) > 10) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char pin[12];

    for (int32_t i = 0; i < count; i++) {
        pin[i] = digits[i] + '0';
    }

    pin[count] = '\0';

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::RequestVirtualKeypadPIN;

    Battlenet::Authentication::ModuleParam* param = message->m_params.New();
    *param->Set<0, BSN::String<256>>() = pin;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: e5dd91ee manual
// OFFSET: 0x008C7C10
void BattlenetLogin::EnterMatrix(uint8_t value) {
    if (!this->m_matrixEnabled) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::EnterMatrixCard;

    Battlenet::Authentication::ModuleParam* param = message->m_params.New();
    *param->Set<1, uint32_t>() = value;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: 34fe48e7 manual
// OFFSET: 0x008C7D00
void BattlenetLogin::RevertMatrix() {
    if (!this->m_matrixEnabled) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::RevertMatrixCard;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: 1993fec7 manual
// OFFSET: 0x008C7DC0
void BattlenetLogin::CommitMatrix() {
    if (!this->m_matrixEnabled) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::CommitMatrixCard;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: a977f694 manual
// OFFSET: 0x008C7E80
void BattlenetLogin::FinalizeMatrix() {
    if (!this->m_matrixEnabled) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->m_matrixEnabled = false;

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::FinalizeMatrixCard;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: d8ce7391 manual
// OFFSET: 0x008C7F40
void BattlenetLogin::EnterToken(const char* token) {
    if (!this->m_tokenRequired) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->m_tokenRequired = false;

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::RequestToken;

    Battlenet::Authentication::ModuleParam* param = message->m_params.New();
    *param->Set<0, BSN::String<256>>() = token;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: efe382b2 manual
// OFFSET: 0x008C8050
void BattlenetLogin::SendSurveyInfoToModule() {
    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;

    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 98c6abd3 manual
// OFFSET: 0x008C80E0
bool BattlenetLogin::SetGameAccount(uint32_t index) {
    if (index >= this->m_gameAccounts.m_count || this->m_gameAccountSelected) {
        return false;
    }

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
    message->m_moduleId = this->m_moduleId;
    message->m_name = Battlenet::Event::AuthModule::RequestGameAccount;

    GameAccount& account = this->m_gameAccounts.m_data[index];

    Battlenet::Authentication::ModuleParam* param = message->m_params.New();
    *param->Set<1, uint32_t>() = account.m_region;

    param = message->m_params.New();
    *param->Set<0, BSN::String<256>>() = account.m_name;

    if (!g_tumorManager) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
    this->m_gameAccountSelected = 1;

    return true;
}

// VERIFY: 5050708b manual
// OFFSET: 0x008C8280
void BattlenetLogin::Logon(const char* loginServer, const char* portal) {
    if (this->m_loggedOn) {
        return;
    }

    if (!g_tumorManager) {
        g_tumorManager = this->CreateTumorManager();

        if (!g_tumorManager) {
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NO_BATTLENET_MANAGER, nullptr, 0);
            return;
        }

        BattlenetUI::Init();
    }

    if (!SStrChr(this->m_username, '@') || static_cast<uint32_t>(SStrLen(this->m_username) - 3) > 317) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY_HTML", FrameScript_GetText("LOGIN_UNKNOWN_ACCOUNT", -1, GENDER_NOT_APPLICABLE));
        return;
    }

    this->m_reconnect = false;
    this->m_serviceState = 4;
    this->m_logonStatus = -1;
    this->m_logonResult = 0;
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_CONNECTING, LOGIN_OK, nullptr, 0);

    if (portal && *portal) {
        Battlenet::Event::Creep::Invoke invoke;
        Battlenet::Event::Creep::Portal* request = invoke.Set<12, Battlenet::Event::Creep::Portal>();
        *request->Set<0, BSN::AsciiString<31>>() = portal;
        g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
    }

    this->m_gameAccountSelected = 0;

    Battlenet::Event::Creep::Invoke invoke;
    Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
    BSN::AsciiString<320>* accountName = authentication->Set<1, BSN::AsciiString<320>>();
    SStrLower(this->m_username);
    *accountName = this->m_username;
    g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
}

// VERIFY: 61a752e9 manual
// OFFSET: 0x008C8490
void BattlenetLogin::QueueEvent(BN_EVENT_ID id, void* data, int32_t size) {
    this->m_eventLock.Enter();

    BN_EVENT* event = this->m_events[this->m_eventIndex].NewNode(2, 0, 0);
    event->m_data = data;
    event->m_id = id;
    event->m_size = size;

    this->m_eventLock.Leave();
}

// VERIFY: 6fe1b770 auto
// OFFSET: 0x008C8730
void BattlenetApplication::WCConnected(WowConnection* connection, WowConnection* inbound, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_localAddress.m_address[0] = addr->selfAddr.data[4];
    this->m_localAddress.m_address[1] = addr->selfAddr.data[5];
    this->m_localAddress.m_address[2] = addr->selfAddr.data[6];
    this->m_localAddress.m_address[3] = addr->selfAddr.data[7];
    this->m_localAddress.m_port[0] = addr->selfAddr.data[2];
    this->m_localAddress.m_port[1] = addr->selfAddr.data[3];

    if (!this->m_login) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    NETADDR* peer = static_cast<NETADDR*>(malloc(sizeof(NETADDR)));
    *peer = addr->peerAddr;
    this->m_login->QueueEvent(BN_EVENT_CONNECTED, peer, sizeof(NETADDR));
}

// VERIFY: d780dfa0 auto
// OFFSET: 0x008C87B0
void BattlenetApplication::WCDataReady(WowConnection* connection, uint32_t timeStamp, uint8_t* data, int32_t length) {
    if (!this->m_login) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    void* copy = malloc(length);
    memcpy(copy, data, length);
    this->m_login->QueueEvent(BN_EVENT_DATA, copy, length);
}

// VERIFY: fbe31bb4 auto
// OFFSET: 0x008C8800
void BattlenetApplication::WCCantConnect(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_login->QueueEvent(BN_EVENT_CANT_CONNECT, nullptr, 0);
}

// VERIFY: 06ceae2d auto
// OFFSET: 0x008C8820
void BattlenetApplication::WCDisconnected(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_login->QueueEvent(BN_EVENT_DISCONNECTED, nullptr, 0);
}

// VERIFY: f9ecbbad manual
// OFFSET: 0x008C8840
void BattlenetLogin::UpdateRealmListData(uint32_t region, uint32_t battlegroup, uint32_t id, const Battlenet::Realm::Info* info, uint8_t flags, bool remove) {
    this->m_realmLock.Enter();

    REALM_INFO* realm = nullptr;

    for (BN_REALM* node = this->m_realms.Head(); node; node = this->m_realms.Next(node)) {
        if (node->m_info.m_regionId == region && node->m_info.m_battlegroupId == battlegroup && node->m_info.m_realmId == id) {
            realm = &node->m_info;

            if (remove) {
                this->m_realms.DeleteNode(node);
                this->m_realmLock.Leave();
                return;
            }

            break;
        }
    }

    if (!info) {
        this->m_realmLock.Leave();
        return;
    }

    if (!realm) {
        BN_REALM* node = this->m_realms.NewNode(2, 0, 0);

        if (!node) {
            this->m_realmLock.Leave();
            return;
        }

        realm = &node->m_info;
    }

    realm->m_type = info->m_type;
    realm->m_otherFlags = info->m_otherFlags;
    realm->m_flags = flags;
    SStrCopy(realm->m_realmName, info->m_name.m_data, sizeof(realm->m_realmName));
    SStrCopy(realm->m_realmAddress, "0.0.0.0", sizeof(realm->m_realmAddress));
    realm->m_population = info->m_population;
    realm->m_numCharacters = this->GetCharacterCount(region, battlegroup, id);
    realm->m_category = info->m_category;
    realm->m_regionId = region;
    realm->m_battlegroupId = battlegroup;
    realm->m_realmId = id;

    if (!info->m_build.m_exists) {
        realm->m_build = 0;
        realm->m_versionMajor = 0;
        realm->m_versionMinor = 0;
        realm->m_versionRevision = 0;
        realm->m_versionBuild = 0;
        this->m_realmLock.Leave();
        return;
    }

    realm->m_build = info->m_build.Get().m_build;
    realm->m_versionMajor = SStrToInt(info->m_build.Get().m_version.m_data);

    const char* cursor = SStrChr(info->m_build.Get().m_version.m_data, '.');

    if (cursor) {
        cursor++;
        realm->m_versionMinor = SStrToInt(cursor);
        cursor = SStrChr(cursor, '.');

        if (cursor) {
            cursor++;
            realm->m_versionRevision = SStrToInt(cursor);
            cursor = SStrChr(cursor, '.');

            if (cursor) {
                realm->m_versionBuild = SStrToInt(cursor + 1);
                realm->m_flags |= 0x4;
            }
        }
    }

    const Battlenet::IP4::AddressPort& address = info->m_build.Get().m_address;
    SStrPrintf(realm->m_realmAddress, sizeof(realm->m_realmAddress), "%u.%u.%u.%u:%u", address.m_address[0], address.m_address[1], address.m_address[2], address.m_address[3], address.m_port[1] + (address.m_port[0] << 8));

    this->m_realmLock.Leave();
}

// VERIFY: 2265afc4 manual
// OFFSET: 0x008C8B50
void BattlenetLogin::GetRealmList() {
    if (this->m_realmListRequested) {
        this->QueueEvent(BN_EVENT_REALM_LIST, nullptr, 0);
    } else if (g_tumorManager && this->IsLoggedOn()) {
        this->m_realmListRequested = true;

        Battlenet::Event::Creep::Invoke invoke;
        Battlenet::Realm::Request* request = invoke.Set<6, Battlenet::Realm::Request>();
        Battlenet::Realm::ListRequest* list = request->Set<0, Battlenet::Realm::ListRequest>();
        list->m_subscribe = true;
        list->unk_0004 = 0;
        list->unk_0008 = 0;

        g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
    }
}

// VERIFY: 913b397e manual
// OFFSET: 0x008C8C00
BattlenetLogin::BattlenetLogin() {
    this->m_application = nullptr;
    g_tumorManager = nullptr;
    this->m_moduleId = 0;
    this->m_logonResult = 0;
    this->m_serviceState = 4;

    this->m_realmLock.Enter();
    this->m_realms.Clear();
    this->m_realmLock.Leave();

    this->m_realmListRequested = false;
    this->m_joinRealmId = 0;

    this->m_eventLock.Enter();
    this->m_events[0].Clear();
    this->m_events[1].Clear();
    this->m_eventLock.Leave();

    this->m_eventIndex = 0;

    this->m_gameAccountLock.Enter();
    this->m_gameAccounts.Clear();
    this->m_gameAccountLock.Leave();

    this->m_gameAccountSelected = 0;

    this->m_patchInstructionLock.Enter();
    this->m_patchInstructions.Clear();
    this->m_patchInstructionLock.Leave();

    this->m_accountFlags = 0;
    this->m_accountFlags2 = 0;
    this->m_surveyPending = false;
    this->m_surveyTime = 0;
}

// VERIFY: 32745cec auto
// OFFSET: 0x008C8DF0
void BattlenetLogin::DestroyTumorManager() {
    s_battlenetAllocator.Delete(g_tumorManager);

    this->m_eventLock.Enter();
    this->m_events[0].Clear();
    this->m_events[1].Clear();
    this->m_eventLock.Leave();

    this->m_realmLock.Enter();
    this->m_realms.Clear();
    this->m_realmLock.Leave();

    this->m_realmListRequested = false;

    this->m_gameAccountLock.Enter();
    this->m_gameAccounts.Clear();
    this->m_gameAccountLock.Leave();

    this->m_gameAccountSelected = 0;

    this->m_patchInstructionLock.Enter();
    this->m_patchInstructions.Clear();
    this->m_patchInstructionLock.Leave();
}

// VERIFY: 4c57ab63 manual
// OFFSET: 0x008C8F00
void BattlenetLogin::Init(LoginResponse* loginResponse) {
    if (!loginResponse) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->m_loginResponse = loginResponse;
    this->m_logonResult = 0;
    this->m_serviceState = 4;

    this->m_realmLock.Enter();
    this->m_realms.Clear();
    this->m_realmLock.Leave();

    this->m_eventLock.Enter();
    this->m_events[0].Clear();
    this->m_events[1].Clear();
    this->m_eventLock.Leave();

    this->m_gameAccountLock.Enter();
    this->m_gameAccounts.Clear();
    this->m_gameAccountLock.Leave();

    this->m_gameAccountSelected = 0;
    this->m_realmCharacterCounts = static_cast<RealmCharacterCount*>(SMemAlloc(64 * sizeof(RealmCharacterCount), __FILE__, __LINE__, 0));
    this->m_realmCharacterCountCount = 0;
    this->m_surveyPending = false;
    this->m_surveyTime = 0;

    Battlenet::Initialize(s_battlenetAllocator);

    delete this->m_application;

    this->m_application = new (__FILE__, __LINE__) BattlenetApplication();

    if (this->m_application) {
        this->m_application->m_login = this;
        g_tumorManager = this->CreateTumorManager();

        if (g_tumorManager) {
            BattlenetUI::Init();
        } else {
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NO_BATTLENET_MANAGER, nullptr, 0);
        }
    } else {
        if (g_tumorManager) {
            s_battlenetAllocator.Delete(g_tumorManager);
            g_tumorManager = nullptr;
        }

        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NO_BATTLENET_APPLICATION, nullptr, 0);
    }
}

// VERIFY: 4a3d7376 manual
// OFFSET: 0x008C9140
void BattlenetLogin::HandleDisconnectEvents(uint32_t index) {
    TSList<BN_EVENT>& events = this->m_events[index];
    bool disconnected = false;
    bool serviceDisconnected = false;

    for (BN_EVENT* event = events.Head(); event; event = events.Next(event)) {
        if (event->m_id == BN_EVENT_SERVICE_DISCONNECTED) {
            BattlenetUI::SetDisconnectedFromBattlenet(true);
            disconnected = true;
            serviceDisconnected = true;
            break;
        }

        if (event->m_id == BN_EVENT_SERVICE_FAILED) {
            BattlenetUI::SetDisconnectedFromBattlenet(true);
            this->m_loggedOn = false;

            if (!IsInOrEnteringWorld()) {
                disconnected = true;
                break;
            }
        }
    }

    if (!disconnected) {
        return;
    }

    for (BN_EVENT* event = events.Head(); event;) {
        BN_EVENT* next = events.Next(event);
        free(event->m_data);
        events.DeleteNode(event);
        event = next;
    }

    if (CGlueMgr::m_loginState == LOGIN_STATE_DOWNLOADFILE) {
        return;
    }

    if (!this->m_online) {
        this->m_logonStatus = 1;
    }

    if (serviceDisconnected) {
        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_DISCONNECTED, LOGIN_OK, nullptr, 0);
    } else {
        int32_t result = this->m_logonResult;

        if (!result) {
            result = 105;
        }

        this->LogonResultBN(result);
    }

    this->m_loggedOn = false;
    this->m_online = false;

    Battlenet::TumorManager* manager = this->CreateTumorManager();
    this->DestroyTumorManager();
    g_tumorManager = manager;
    BattlenetUI::Init();
}

// VERIFY: 8d22a9d5 manual
// OFFSET: 0x008C9310
void BattlenetLogin::HandleInvoke(const Battlenet::Event::Larva::Invoke* invoke) {
    if (invoke->m_tag == 0) {
        if (invoke->m_authentication.m_tag == 0) {
            const Battlenet::Authentication::ModuleRequest& request = invoke->Get<0, Battlenet::Authentication::Response>().Get<0, Battlenet::Authentication::ModuleRequest>();

            if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::RequestPassword.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->GetLogonMethod();
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::RequestToken.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->SetTokenInfo(true, request.m_params[0].Get<1, uint32_t>());
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::RequestMatrixCard.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                const char* challenges = request.m_params[1].Get<0, BSN::String<256>>().m_data;
                const char* info = request.m_params[0].Get<0, BSN::String<256>>().m_data;
                this->SetAuthModuleMatrixInfo(info, challenges);
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::RequestVirtualKeypadPIN.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->SetAuthModulePinInfo(request.m_params[0].Get<0, BSN::String<256>>().m_data);
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::ClearGameAccounts.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->m_gameAccountLock.Enter();
                this->m_gameAccounts.Clear();
                this->m_gameAccountLock.Leave();
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::AddGameAccount.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->m_gameAccountLock.Enter();

                GameAccount* account = this->m_gameAccounts.New();
                account->m_region = request.m_params[0].Get<1, uint32_t>();
                SStrCopy(account->m_name, request.m_params[1].Get<0, BSN::String<256>>().m_data, sizeof(account->m_name));

                this->m_gameAccountLock.Leave();
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::RequestGameAccount.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->QueueEvent(BN_EVENT_GAME_ACCOUNTS_UPDATED, nullptr, 0);
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::RequestSurveyInfo.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->QueueEvent(BN_EVENT_SEND_SURVEY_INFO, nullptr, 0);
            } else if (!SStrCmpI(request.m_name.m_data, Battlenet::Event::AuthModule::ShowSurveyDialog.m_data, STORM_MAX_STR)) {
                this->m_moduleId = request.m_moduleId;
                this->QueueEvent(BN_EVENT_SHOW_SURVEY, nullptr, 0);
            }
        } else if (invoke->m_authentication.m_tag == 1) {
            if (this->m_serviceState == 6) {
                this->m_application->LogLevel("Ignoring Authentication Success invoke while in Service::RECOVERED state.", 4);
            } else {
                const Battlenet::Authentication::Complete& complete = invoke->Get<0, Battlenet::Authentication::Response>().Get<1, Battlenet::Authentication::Complete>();
                SStrCopy(this->m_username, complete.m_accountName.m_data, STORM_MAX_STR);
                this->m_accountFlags = complete.m_accountFlags;
                this->m_accountFlags2 = complete.m_accountFlags2;
                this->LogonResultBN(0);
            }
        } else if (invoke->m_authentication.m_tag == 2 && this->m_serviceState != 5) {
            char text[64];
            const Battlenet::Authentication::Failure& failure = invoke->Get<0, Battlenet::Authentication::Response>().Get<2, Battlenet::Authentication::Failure>();

            SStrPrintf(text, 63, "Battle.net Authentication failure code: %d", failure.m_error);
            ConsoleWrite(text, DEFAULT_COLOR);

            if (failure.m_error != 138) {
                this->LogonResultBN(failure.m_error);

                if (this->m_serviceState != 7) {
                    this->QueueEvent(BN_EVENT_LOGOFF, nullptr, 0);
                }
            }
        }
    } else if (invoke->m_tag == 1) {
        this->m_serviceState = invoke->m_service.m_tag;

        switch (invoke->m_service.m_tag) {
            case 2:
                this->QueueEvent(BN_EVENT_SERVICE_FAILED, nullptr, 0);
                break;

            case 3:
                if (!invoke->Get<1, Battlenet::Service::State>().Get<3, uint32_t>()) {
                    this->m_serviceState = 6;
                }

                break;

            case 4:
                this->QueueEvent(BN_EVENT_SERVICE_DISCONNECTED, nullptr, 0);
                break;

            case 5:
                this->QueueEvent(BN_EVENT_SERVICE_LOST, nullptr, 0);
                break;

            case 6:
                this->QueueEvent(BN_EVENT_SERVICE_RECOVERED, nullptr, 0);
                break;

            case 7:
                this->QueueEvent(BN_EVENT_RECONNECTED, nullptr, 0);
                break;
        }
    } else if (invoke->m_tag == 9) {
        if (invoke->m_realm.m_tag == 0) {
            int32_t tag = invoke->Get<9, Battlenet::Realm::Response>().Get<0, Battlenet::Realm::Update>().m_info.m_tag;

            if (tag == 0) {
                const Battlenet::Realm::Update& update = invoke->Get<9, Battlenet::Realm::Response>().Get<0, Battlenet::Realm::Update>();
                this->UpdateRealmListData(update.m_region, update.m_battlegroup, update.m_id, &update.m_info.Get<0, Battlenet::Realm::Info>(), update.m_info.Get<0, Battlenet::Realm::Info>().m_flags, false);
            } else if (tag == 1) {
                const Battlenet::Realm::Update& update = invoke->Get<9, Battlenet::Realm::Response>().Get<0, Battlenet::Realm::Update>();
                this->UpdateRealmListData(update.m_region, update.m_battlegroup, update.m_id, nullptr, 0, true);
            }
        } else if (invoke->m_realm.m_tag == 1) {
            int32_t tag = invoke->Get<9, Battlenet::Realm::Response>().Get<1, Battlenet::Realm::List>().m_tag;

            if (tag == 0) {
                this->m_realmLock.Enter();
                this->m_realms.Clear();
                this->m_realmListRequested = true;

                uint32_t count = invoke->Get<9, Battlenet::Realm::Response>().Get<1, Battlenet::Realm::List>().Get<0, BSN::Array<Battlenet::Realm::CharacterCount, 64>>().m_count;
                this->m_realmCharacterCountCount = count;

                if (count >= 64) {
                    count = 64;
                }

                this->m_realmCharacterCountCount = count;

                if (this->m_realmCharacterCounts) {
                    for (uint32_t i = 0; i < this->m_realmCharacterCountCount; i++) {
                        const Battlenet::Realm::CharacterCount& characterCount = invoke->Get<9, Battlenet::Realm::Response>().Get<1, Battlenet::Realm::List>().Get<0, BSN::Array<Battlenet::Realm::CharacterCount, 64>>()[i];
                        this->m_realmCharacterCounts[i].m_region = characterCount.m_region;
                        this->m_realmCharacterCounts[i].m_battlegroup = characterCount.m_battlegroup;
                        this->m_realmCharacterCounts[i].m_id = characterCount.m_id;
                        this->m_realmCharacterCounts[i].m_count = characterCount.m_count;
                    }
                }

                this->m_realmLock.Leave();
            } else if (tag == 1) {
                this->QueueEvent(BN_EVENT_REALM_LIST, nullptr, 0);
            }
        } else if (invoke->m_realm.m_tag == 2) {
            int32_t tag = invoke->Get<9, Battlenet::Realm::Response>().Get<2, Battlenet::Realm::JoinResponse>().m_tag;

            if (tag == 0) {
                const Battlenet::Realm::JoinInfo& join = invoke->Get<9, Battlenet::Realm::Response>().Get<2, Battlenet::Realm::JoinResponse>().Get<0, Battlenet::Realm::JoinInfo>();

                for (uint32_t i = 0; i < 40; i++) {
                    this->m_sessionKey[i] = join.m_sessionKey[i];
                }

                uint16_t port = (join.m_address.m_port[0] << 8) | join.m_address.m_port[1];
                uint32_t address = (((((join.m_address.m_address[0] << 8) | join.m_address.m_address[1]) << 8) | join.m_address.m_address[2]) << 8) | join.m_address.m_address[3];
                this->m_loginResponse->JoinRealmResult(this->m_joinRealmId, address, port, 0, 0);
            } else if (tag == 1) {
                uint32_t error = invoke->Get<9, Battlenet::Realm::Response>().Get<2, Battlenet::Realm::JoinResponse>().Get<1, uint32_t>();
                this->m_loginResponse->JoinRealmResult(0, 0, 0, 0, error);
            }
        }
    }
}

// VERIFY: 0e3dbd5d manual
// OFFSET: [0x008C9A40, 0x008CA050]
BattlenetLogin::~BattlenetLogin() {
    this->m_patchInstructionLock.Enter();
    this->m_patchInstructions.Clear();
    this->m_patchInstructionLock.Leave();

    this->m_gameAccountLock.Enter();
    this->m_gameAccounts.Clear();
    this->m_gameAccountLock.Leave();

    this->m_gameAccountSelected = 0;

    this->m_eventLock.Enter();
    this->m_events[0].Clear();
    this->m_events[1].Clear();
    this->m_eventLock.Leave();

    this->m_realmLock.Enter();
    this->m_realms.Clear();
    this->m_realmLock.Leave();

    if (g_tumorManager) {
        BattlenetUI::Uninit();
    }

    this->DestroyTumorManager();

    delete this->m_application;
    delete[] this->m_realmCharacterCounts;

    this->m_realmCharacterCounts = nullptr;

    Battlenet::Teardown();
}

// VERIFY: 5ebf6bfa manual
// OFFSET: 0x008C9C70
void BattlenetLogin::EventQueueUpdate() {
    uint32_t index = this->m_eventIndex;

    this->m_eventLock.Enter();
    this->m_eventIndex = 1 - this->m_eventIndex;
    this->m_eventLock.Leave();

    this->HandleDisconnectEvents(index);

    TSList<BN_EVENT>& events = this->m_events[index];

    for (BN_EVENT* event = events.Head(); event;) {
        BN_EVENT* next = events.Next(event);

        switch (event->m_id) {
            case BN_EVENT_CONNECTED:
                this->Connected(*static_cast<NETADDR*>(event->m_data));
                break;

            case BN_EVENT_CANT_CONNECT:
                this->CantConnect();
                break;

            case BN_EVENT_DISCONNECTED:
                this->Disconnected();
                break;

            case BN_EVENT_DATA: {
                if (!g_tumorManager) {
                    SErrDisplayAppFatalMessage("", "", 0);
                }

                uint32_t used;
                Battlenet::SystemClient& client = g_tumorManager->m_tumor;
                client.Recv(this->m_application->m_connection, event->m_data, event->m_size, used);
                break;
            }

            case BN_EVENT_REALM_LIST:
                this->BuildRealmListResult();
                break;

            case BN_EVENT_RECONNECTED:
                this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_RESPONSE_CONNECTED, LOGIN_OK, nullptr, 0);
                BattlenetUI::SetBattlenetConnectionRecovered();
                break;

            case BN_EVENT_SERVICE_LOST:
                BattlenetUI::SetDisconnectedFromBattlenet(false);
                break;

            case BN_EVENT_SERVICE_RECOVERED:
                BattlenetUI::SetBattlenetConnectionRecovered();
                break;

            case BN_EVENT_GAME_ACCOUNTS_UPDATED:
                FrameScript_SignalEvent(GLUE_EVENT_GAME_ACCOUNTS_UPDATED, nullptr);
                break;

            case BN_EVENT_LOGOFF:
                this->Logoff();
                break;

            case BN_EVENT_SHOW_SURVEY:
                this->m_surveyPending = true;
                this->m_surveyTime = OsGetAsyncTimeMs();
                FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("LOGIN_STATE_SURVEY", -1, GENDER_NOT_APPLICABLE));
                break;

            case BN_EVENT_SEND_SURVEY_INFO:
                this->SendSurveyInfoToModule();

            default:
                SErrDisplayAppFatalMessage("", "", 0);
        }

        free(event->m_data);
        events.DeleteNode(event);
        event = next;
    }
}

// VERIFY: 09ca53f7 manual
// OFFSET: 0x008C9EA0
void BattlenetApplication::RecvEvent(const Battlenet::Event::Header& header, const BSN::EncodedObject* data) {
    if (header.m_eventType == 5) {
        BSN::BSNEncodedObject<Battlenet::Event::Larva::Invoke, Protocol> invoke;
        invoke.WalkAndGet(data);
        this->m_login->HandleInvoke(&invoke.m_data);
    } else if (header.m_eventType == 1) {
        BSN::BSNEncodedObject<Battlenet::Event::Update::Invoke, Protocol> invoke;
        invoke.WalkAndGet(data);
        this->m_login->HandleUpdateInvoke(&invoke.m_data);
    }
}

// VERIFY: a186d659 manual
// OFFSET: 0x008C9F70
void BattlenetLogin::Update() {
    this->EventQueueUpdate();

    if (g_tumorManager) {
        Battlenet::SystemClient& client = g_tumorManager->m_tumor;
        client.Process(OsGetAsyncTimeMs());
    }

    if (this->m_surveyPending && this->m_serviceState != 2 && this->m_serviceState != 5 && OsGetAsyncTimeMs() - this->m_surveyTime > 1000) {
        this->m_surveyPending = false;
        FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);

        if (g_tumorManager) {
            Battlenet::Event::Creep::Invoke invoke;
            Battlenet::Authentication::Request* authentication = invoke.Set<0, Battlenet::Authentication::Request>();
            Battlenet::Authentication::ModuleMessage* message = authentication->Set<2, Battlenet::Authentication::ModuleMessage>();
            message->m_moduleId = this->m_moduleId;
            message->m_name = Battlenet::Event::AuthModule::SurveyDialogComplete;
            g_tumorManager->SendEvent<Battlenet::Event::Creep::Invoke, Protocol>(Battlenet::Event::CREEP, invoke);
        }
    }
}

// VERIFY: 77b47832 auto
// OFFSET: 0x00530CE0
bool BattlenetLogin::IsConnected() {
    return this->m_serviceState == 7 || this->m_serviceState == 6 || this->m_serviceState == 3;
}

// VERIFY: 191ce575 auto
// OFFSET: 0x008C52D0
LOGIN_STATE BattlenetLogin::NextSecurityState(LOGIN_STATE state) {
    return static_cast<LOGIN_STATE>(14);
}

// VERIFY: 9df286c6 auto
// OFFSET: 0x008C5E20
const uint8_t* BattlenetLogin::GetKeyMap() {
    return &this->m_pinKeyMap[1];
}

// VERIFY: f1a20f90 manual
// OFFSET: 0x008C5B40
bool BattlenetLogin::IsTrialAccount() {
    return (this->m_accountFlags & 0x8) != 0;
}

// VERIFY: 8e8b170c manual
// OFFSET: 0x008C5B60
bool BattlenetLogin::IsProPassAccount() {
    return (this->m_accountFlags & 0x800000) != 0;
}

// VERIFY: 3d74de1d manual
// OFFSET: 0x008C5B80
bool BattlenetLogin::IsGMAccount() {
    return (this->m_accountFlags & 0x1) != 0;
}

// VERIFY: 781acb9e auto
// OFFSET: 0x008C5E30
bool BattlenetLogin::GetMatrixInfo(uint32_t* width, uint32_t* height, uint32_t* minDigits, uint32_t* maxDigits, bool* flipCoords, uint32_t* challengeCount) {
    if (!width || !height || !minDigits || !maxDigits || !challengeCount || !flipCoords) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!this->m_matrixEnabled) {
        return false;
    }

    *width = this->m_matrixWidth;
    *height = this->m_matrixHeight;
    *minDigits = this->m_matrixMinDigitCount;
    *maxDigits = this->m_matrixMaxDigitCount;
    *flipCoords = this->m_matrixFlipCoords;
    *challengeCount = this->m_matrixChallengeCount;
    return true;
}

// VERIFY: d0ecbbe1 auto
// OFFSET: 0x008C5ED0
bool BattlenetLogin::GetMatrixCoordinates(uint32_t index, uint32_t* x, uint32_t* y) {
    if (!x || !y) {
        SErrDisplayAppFatalMessage("", "", 0);
        return false;
    }

    if (!this->m_matrixEnabled || !this->m_matrixData || index >= this->m_matrixChallengeCount) {
        return false;
    }

    uint32_t coordinate = this->m_matrixData[index];
    *x = coordinate % this->m_matrixWidth;
    *y = coordinate / this->m_matrixWidth;

    if (*y >= this->m_matrixHeight) {
        SErrDisplayAppFatalMessage("", "", 0);
        return false;
    }

    return true;
}

// VERIFY: 5782eea7 auto
// OFFSET: 0x008C6AF0
uint32_t BattlenetLogin::GetNumGameAccounts() {
    return this->m_gameAccounts.m_count;
}

// VERIFY: e9ea1376 manual
// OFFSET: 0x008C6B00
const char* BattlenetLogin::GetGameAccountName(uint32_t index) {
    if (index < this->m_gameAccounts.m_count) {
        return this->m_gameAccounts.m_data[index].m_name;
    }

    return nullptr;
}

// VERIFY: d642e343 manual
// OFFSET: 0x008C6B60
uint32_t BattlenetLogin::GetGameAccountRegion(uint32_t index) {
    if (index < this->m_gameAccounts.m_count) {
        return this->m_gameAccounts.m_data[index].m_region;
    }

    return 0;
}

// VERIFY: 588e5d04 auto
// OFFSET: 0x00532A60
void BattlenetLogin::GetVersionProof(const uint8_t* versionChallenge) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 7c681e4b auto
// OFFSET: 0x00532A60
void BattlenetLogin::SetPinInfo(bool enabled, uint32_t seed, const uint8_t* salt) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: e59eb6e7 auto
// OFFSET: 0x00532A60
void BattlenetLogin::GetPinInfo(bool* enabled, uint8_t** salt, uint8_t** hash) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 0834ae87 auto
// OFFSET: 0x00532A60
void BattlenetLogin::SetMatrixInfo(bool enabled, uint8_t width, uint8_t height, uint8_t minDigits, uint8_t maxDigits, bool flipCoords, uint8_t challengeCount, uint64_t seed, const uint8_t* sessionKey, uint32_t sessionKeyLength) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: a0319163 auto
// OFFSET: 0x00532A60
void BattlenetLogin::GetMatrixProof(bool* enabled, const uint8_t** proof) const {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: e23e08a1 auto
// OFFSET: 0x00532A60
void BattlenetLogin::GetTokenInfo(bool* enabled, char* token) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 8e3e2131 auto
// OFFSET: 0x00532A60
void BattlenetLogin::LogonResult(Grunt::Result result, const uint8_t* sessionKey, uint32_t sessionKeyLength, uint16_t flags) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 14fadb30 auto
// OFFSET: 0x00532A60
void BattlenetLogin::ReconnectResult(Grunt::Result result, const uint8_t* sessionKey, uint32_t sessionKeyLength, uint16_t flags) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 9b7ebb81 auto
// OFFSET: 0x008E5250
int32_t BattlenetLogin::GetServerId() {
    return 0;
}

// VERIFY: 16ee8ba4 auto
// OFFSET: 0x00532A60
const uint8_t* BattlenetLogin::GetVersionChallenge() {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 6aac27e5 auto
// OFFSET: 0x005EEB70
void BattlenetLogin::Reconnect() {
}

// VERIFY: 6c77d2b7 auto
// OFFSET: 0x00532A60
int32_t BattlenetLogin::Wait(const char* message, LoginData& loginData) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: b98bce31 auto
// OFFSET: 0x00532A60
void BattlenetLogin::ProveVersion(const uint8_t* versionChecksum) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 7d626730 auto
// OFFSET: 0x00532A60
void BattlenetLogin::FileAccept(bool accept, uint64_t offset) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: eb437353 auto
// OFFSET: 0x00532A60
void BattlenetLogin::FileComplete(bool success) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: b94b273e auto
// OFFSET: 0x00532A60
int32_t BattlenetLogin::GetSurveyId() {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 3249b71d auto
// OFFSET: 0x00532A60
void BattlenetLogin::SendSurveyData(uint16_t size, uint8_t* data) {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 8a2c0499 auto
// OFFSET: 0x00532A60
void BattlenetLogin::SendSurveyFailed() {
    SErrDisplayAppFatalMessage("", "", 0);
}

// VERIFY: 4092e10a auto
// OFFSET: 0x008C8DE0
int32_t BattlenetLogin::GetLoginServerType() {
    return 1;
}
