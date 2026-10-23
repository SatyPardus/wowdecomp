#include "GruntLogin.h"
#include "Engine/Source/Os/OsSecureRandom.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include <cstring>
#include <malloc.h>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Os/OsTcp.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "GameVersion.h"

// INLINED
GruntLogin::MATRIXINPUT::MATRIXINPUT() {
    this->m_valid = false;
}

// VERIFY: 7676960d manual
// OFFSET: 0x008CA0B0
bool GruntLogin::Connected(const NETADDR& address) {
    this->m_loggedOn = true;

    char addressString[32];
    const char* text = OsNetAddrToStr(&address, addressString, sizeof(addressString));
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_RESPONSE_CONNECTED, LOGIN_OK, text, 0);

    return true;
}

// VERIFY: 2720463d manual
// OFFSET: 0x008CA140
bool GruntLogin::CantConnect() {
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_SERVER_DOWN, nullptr, 0);
    this->m_logonStatus = 1;
    this->LogonResult(Grunt::GRUNT_RESULT_CANT_CONNECT, nullptr, 0, 0);
    this->m_loggedOn = false;
    this->m_online = false;

    return false;
}

// VERIFY: 7c5dddc1 manual
// OFFSET: 0x008CA1D0
bool GruntLogin::Disconnected() {
    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_DISCONNECTED, LOGIN_OK, nullptr, 0);

    if (!this->m_online) {
        this->m_logonStatus = 1;
        this->LogonResult(Grunt::GRUNT_RESULT_DISCONNECTED, nullptr, 0, 0);
    }

    this->m_loggedOn = false;
    this->m_online = false;

    return false;
}

// VERIFY: ca1d1b53 manual
// OFFSET: 0x008CA580
void GruntLogin::GetLogonMethod() {
    Grunt::ClientLink::Logon logon = {};

    logon.m_programId = Login::s_fourCCprogramID;

    if (!logon.m_programId) {
        logon.m_programId = SFile::IsTrial() ? 'WoWS' : 'WoW';
    }

    logon.m_processor = OsGetProcessorId();
    logon.m_os = OsGetOsId();
    logon.m_versionMajor = GAME_VERSION_MAJOR;
    logon.m_versionMinor = GAME_VERSION_MINOR;
    logon.m_versionPatch = GAME_VERSION_PATCH;
    logon.m_build = GAME_BUILD;
    logon.m_locale = SFile::s_locale;
    logon.m_timeZone = OsGetTimeZone();

    if (this->IsReconnect()) {
        logon.m_accountName = this->m_username;
        logon.m_password = reinterpret_cast<const char*>(this->m_sessionKey);
        this->m_clientLink->LogonStoredSession(logon);
    } else if (this->m_password) {
        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_AUTHENTICATING, LOGIN_OK, nullptr, 0);

        logon.m_password = this->m_password;
        logon.m_accountName = this->m_username;
        this->m_clientLink->LogonNewSession(logon);

        memset(this->m_password, 0, SStrLen(this->m_password));
        SMemFree(this->m_password, __FILE__, __LINE__, 0);
        this->m_password = nullptr;
    }
}

// VERIFY: eb019dfb manual
// OFFSET: 0x008CA740
void GruntLogin::Logon(const char* loginServer, const char* portal) {
    if (this->m_loggedOn) {
        return;
    }

    this->m_reconnect = false;
    this->m_logonStatus = -1;
    this->m_pinEnabled = false;
    this->m_matrixEnabled = false;
    this->m_tokenEnabled = false;
    this->m_token[0] = '\0';

    this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_CONNECTING, LOGIN_OK, nullptr, 0);
    this->m_clientLink->Connect(loginServer ? loginServer : "us.logon.worldofwarcraft.com:3724");
}

// VERIFY: 1563533a manual
// OFFSET: [0x008CAB80, 0x008CAC20]
GruntLogin::~GruntLogin() {
    if (this->m_matrixCoordinates) {
        delete[] this->m_matrixCoordinates;
    }

    if (this->m_matrixInput) {
        delete this->m_matrixInput;
    }

    if (this->m_matrixCommitted) {
        delete this->m_matrixCommitted;
    }

    if (this->m_clientLink) {
        delete this->m_clientLink;
    }
}

// VERIFY: e258ceaa manual
// OFFSET: 0x008CAC50
void GruntLogin::Init(LoginResponse* loginResponse) {
    this->m_loginResponse = loginResponse;
    this->m_clientLink = new (__FILE__, __LINE__) Grunt::ClientLink(*this);
}

// VERIFY: 93a5e7e0 manual
// OFFSET: 0x008CACA0
GruntLogin::GruntLogin() {
    this->m_clientLink = nullptr;
    this->m_matrixChallengeCount = 0;
    this->m_matrixCoordinates = nullptr;
    this->m_matrixInput = new (__FILE__, __LINE__) GruntLogin::MATRIXINPUT();
    this->m_matrixCommitted = new (__FILE__, __LINE__) GruntLogin::MATRIXINPUT();
}

// VERIFY: a3a49e7d auto
// OFFSET: 0x008CAD20
void GruntLogin::Logoff() {
    if (this->m_loggedOn) {
        this->m_clientLink->Disconnect();
    }
}

// VERIFY: 98618c23 manual
// OFFSET: 0x008CB160
void GruntLogin::LogonResult(Grunt::Result result, const uint8_t* sessionKey, uint32_t sessionKeyLength, uint16_t flags) {
    if (this->IsReconnect()) {
        this->m_loginResponse->HandleRealmData(1, nullptr);
        this->m_clientLink->Disconnect();
        return;
    }

    if (result == Grunt::GRUNT_RESULT_SUCCESS || result == Grunt::GRUNT_RESULT_SUCCESS_SURVEY) {
        this->m_logonStatus = 0;
        this->m_pinFailures = 0;
        memcpy(this->m_sessionKey, sessionKey, sessionKeyLength >= sizeof(this->m_sessionKey) ? sizeof(this->m_sessionKey) : sessionKeyLength);

        if (this->m_clientLink->m_surveyId) {
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_SURVEY, LOGIN_OK, nullptr, flags);
        } else {
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_AUTHENTICATED, LOGIN_OK, nullptr, flags);
        }

        return;
    }

    this->m_logonStatus = 1;

    switch (result) {
        case Grunt::GRUNT_RESULT_BANNED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_BANNED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_UNKNOWN_ACCOUNT:
        case Grunt::GRUNT_RESULT_INCORRECT_PASSWORD: {
            LOGIN_RESULT loginResult = LOGIN_UNKNOWN_ACCOUNT;

            if (this->m_pinEnabled) {
                loginResult = ++this->m_pinFailures < 3 ? LOGIN_UNKNOWN_ACCOUNT_PIN : LOGIN_UNKNOWN_ACCOUNT_CALL;
            }

            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, loginResult, nullptr, 0);
            break;
        }

        case Grunt::GRUNT_RESULT_ALREADY_ONLINE:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_ALREADYONLINE, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_NO_TIME:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_NOTIME, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_DB_BUSY:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_DBBUSY, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_BAD_VERSION:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_BADVERSION, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_DOWNLOAD_FILE:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_DOWNLOADFILE, LOGIN_OK, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_SUSPENDED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_SUSPENDED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_PARENTAL_CONTROL:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_PARENTALCONTROL, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_LOCKED_ENFORCED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_LOCKED_ENFORCED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_TRIAL_EXPIRED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_TRIAL_EXPIRED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_ACCOUNT_CONVERTED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_ACCOUNT_CONVERTED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_CHARGEBACK:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_CHARGEBACK, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_IGR_WITHOUT_BNET:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_IGR_WITHOUT_BNET, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_GAME_ACCOUNT_LOCKED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_GAME_ACCOUNT_LOCKED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_UNLOCKABLE_LOCK:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_UNLOCKABLE_LOCK, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_CONVERSION_REQUIRED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_CONVERSION_REQUIRED, nullptr, 0);
            break;

        case Grunt::GRUNT_RESULT_DISCONNECTED:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_DISCONNECTED, nullptr, 0);
            break;

        default:
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_FAILED, nullptr, 0);
            break;
    }
}

void GruntLogin::Update() {
}

void GruntLogin::JoinRealm(uint32_t a1, uint32_t a2, uint32_t a3) {
}

int32_t GruntLogin::GetLoginServerType() {
    return 0;
}

// VERIFY: d9686987 auto
// OFFSET: 0x008CA260
void GruntLogin::GetVersionProof(const uint8_t* versionChallenge) {
    if (this->IsReconnect()) {
        uint8_t versionChecksum[20] = {};
        this->m_clientLink->ProveVersion(versionChecksum);
        return;
    }

    memcpy(this->m_versionChallenge, versionChallenge, sizeof(this->m_versionChallenge));
    this->m_loginResponse->UpdateLoginStatus(this->NextSecurityState(LOGIN_STATE_FIRST_SECURITY), LOGIN_OK, nullptr, 0);
}

// VERIFY: f68bff9c auto
// OFFSET: 0x008CA2F0
void GruntLogin::EncodePinDigits(uint32_t seed) {
    uint8_t digits[10];

    for (uint32_t i = 0; i < sizeof(digits); i++) {
        digits[i] = i;
    }

    uint8_t* keyMap = this->m_keyMap;

    for (uint32_t count = sizeof(digits); count; count--) {
        uint32_t index = seed % count;
        *keyMap = digits[index];
        seed /= count;
        memmove(&digits[index], &digits[index + 1], count - index - 1);
        keyMap++;
    }
}

// VERIFY: 1be60780 auto
// OFFSET: 0x008CA360
void GruntLogin::SetPinInfo(bool enabled, uint32_t seed, const uint8_t* salt) {
    this->m_pinEnabled = enabled;

    if (enabled) {
        this->EncodePinDigits(seed);
        memcpy(this->m_pinSalt, salt, sizeof(this->m_pinSalt));
    }
}

// VERIFY: 49a3ec8d auto
// OFFSET: 0x008CA3B0
void GruntLogin::GetPinInfo(bool* enabled, uint8_t** salt, uint8_t** hash) {
    *enabled = this->m_pinEnabled;
    *salt = this->m_pinClientSalt;
    *hash = this->m_pinHash;
}

// VERIFY: 00c628e1 auto
// OFFSET: 0x008CA3E0
void GruntLogin::GetMatrixProof(bool* enabled, const uint8_t** proof) const {
    *enabled = this->m_matrixEnabled;
    *proof = this->m_matrixProof;
}

// VERIFY: 42ddcf82 auto
// OFFSET: 0x008CA400
void GruntLogin::SetTokenInfo(bool enabled, uint8_t required) {
    this->m_tokenEnabled = enabled;
    this->m_tokenRequired = required;
}

// VERIFY: 7861663c auto
// OFFSET: 0x008CA420
void GruntLogin::GetTokenInfo(bool* enabled, char* token) {
    *enabled = this->m_tokenEnabled;
    SStrCopy(token, this->m_token, sizeof(this->m_token));
}

// VERIFY: af8b5075 manual
// OFFSET: 0x008CA450
void GruntLogin::ProveVersion(const uint8_t* versionChecksum) {
    if (this->m_loggedOn) {
        this->m_clientLink->ProveVersion(versionChecksum);
        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_HANDSHAKING, LOGIN_OK, nullptr, 0);
    }
}

// VERIFY: 022f5ee3 manual
// OFFSET: 0x008CA4F0
LOGIN_STATE GruntLogin::NextSecurityState(LOGIN_STATE state) {
    switch (state) {
        case LOGIN_STATE_FIRST_SECURITY:
            if (this->m_pinEnabled) {
                return LOGIN_STATE_PIN;
            }

        case LOGIN_STATE_PIN:
            if (this->m_matrixEnabled) {
                return LOGIN_STATE_MATRIX;
            }

        case LOGIN_STATE_MATRIX:
            return this->m_tokenEnabled ? LOGIN_STATE_TOKEN : LOGIN_STATE_CHECKINGVERSIONS;
    }

    return LOGIN_STATE_CHECKINGVERSIONS;
}

// VERIFY: 63b104b5 auto
// OFFSET: 0x008CA700
void GruntLogin::Reconnect() {
    this->m_reconnect = true;
    this->m_clientLink->Connect(this->m_loginResponse->GetLoginServer());
}

// VERIFY: acb6d000 auto
// OFFSET: 0x008CA730
void GruntLogin::GetRealmList() {
    this->m_clientLink->GetRealmList();
}

// VERIFY: ccea54f9 auto
// OFFSET: 0x008CA9F0
void GruntLogin::CommitMatrix() {
    GruntLogin::MATRIXINPUT* committed = this->m_matrixCommitted;
    GruntLogin::MATRIXINPUT* input = this->m_matrixInput;

    input->m_valid = committed->m_valid;
    memcpy(&input->m_hmac, &committed->m_hmac, sizeof(input->m_hmac));
    memcpy(&input->m_key, &committed->m_key, sizeof(input->m_key));
}

// VERIFY: 9523577b auto
// OFFSET: 0x008CABF0
int32_t GruntLogin::GetServerId() {
    return this->m_clientLink->m_serverId;
}

// VERIFY: 8000f875 auto
// OFFSET: 0x008CAC00
const uint8_t* GruntLogin::GetVersionChallenge() {
    return this->m_versionChallenge;
}

// VERIFY: e2adbcde auto
// OFFSET: 0x008CAAF0
bool GruntLogin::IsTrialAccount() {
    return (this->m_clientLink->m_accountFlags >> 3) & 1;
}

// VERIFY: d289ac5b auto
// OFFSET: 0x008CAB10
bool GruntLogin::IsProPassAccount() {
    return (this->m_clientLink->m_accountFlags >> 23) & 1;
}

// VERIFY: 431ffacd auto
// OFFSET: 0x008CAB30
bool GruntLogin::IsGMAccount() {
    return this->m_clientLink->m_accountFlags & 1;
}

// VERIFY: cc6621fa auto
// OFFSET: 0x008CAC10
const uint8_t* GruntLogin::GetKeyMap() {
    return this->m_keyMap;
}

// VERIFY: 49ea575a manual
// OFFSET: 0x008CAD40
void GruntLogin::SetMatrixInfo(bool enabled, uint8_t width, uint8_t height, uint8_t minDigits, uint8_t maxDigits, bool flipCoords, uint8_t challengeCount, uint64_t seed, const uint8_t* sessionKey, uint32_t sessionKeyLength) {
    this->m_matrixEnabled = enabled;

    if (!enabled) {
        return;
    }

    this->m_matrixWidth = width;
    this->m_matrixMaxDigitCount = maxDigits;
    this->m_matrixHeight = height;
    this->m_matrixMinDigitCount = minDigits;
    this->m_matrixChallengeCount = challengeCount;
    this->m_matrixFlipCoords = flipCoords;

    if (this->m_matrixCoordinates) {
        delete[] this->m_matrixCoordinates;
    }

    this->m_matrixCoordinates = new (__FILE__, __LINE__) uint32_t[challengeCount];

    uint32_t cellCount = height * width;
    uint32_t* cells = static_cast<uint32_t*>(alloca(sizeof(uint32_t) * cellCount));

    for (uint32_t i = 0; i < cellCount; i++) {
        cells[i] = i;
    }

    uint64_t value = seed;

    for (uint32_t i = 0; i < challengeCount; i++) {
        uint32_t remaining = cellCount - i;
        uint32_t index = static_cast<uint32_t>(value % remaining);
        value /= remaining;

        this->m_matrixCoordinates[i] = cells[index];
        memmove(&cells[index], &cells[index + 1], sizeof(uint32_t) * (remaining - index) - sizeof(uint32_t));
    }

    MD5_CTX md5;
    MD5Init(&md5);
    MD5Update(&md5, reinterpret_cast<uint8_t*>(&seed), sizeof(seed));
    MD5Update(&md5, sessionKey, sessionKeyLength);
    MD5Final(this->m_matrixKey, &md5);

    GruntLogin::MATRIXINPUT* input = this->m_matrixCommitted;
    memset(input->m_hmac.m_innerPad, 0x36, sizeof(input->m_hmac.m_innerPad));
    memset(input->m_hmac.m_outerPad, 0x5C, sizeof(input->m_hmac.m_outerPad));

    for (uint32_t i = 0; i < sizeof(this->m_matrixKey); i++) {
        input->m_hmac.m_innerPad[i] ^= this->m_matrixKey[i];
        input->m_hmac.m_outerPad[i] ^= this->m_matrixKey[i];
    }

    input->m_hmac.m_hash.Prepare();
    input->m_hmac.m_hash.Process(input->m_hmac.m_innerPad, sizeof(input->m_hmac.m_innerPad));
    ARC4PrepareKey(this->m_matrixKey, sizeof(this->m_matrixKey), &input->m_key);
    input->m_valid = true;

    this->CommitMatrix();

    memset(this->m_matrixProof, 0, sizeof(this->m_matrixProof));
}

// VERIFY: 853abd14 auto
// OFFSET: 0x008CA540
int32_t GruntLogin::GetSurveyId() {
    return this->m_clientLink->m_surveyId;
}

// VERIFY: 4d5b8b79 manual
// OFFSET: 0x008CA9B0
void GruntLogin::RevertMatrix() {
    GruntLogin::MATRIXINPUT* input = this->m_matrixInput;
    GruntLogin::MATRIXINPUT* committed = this->m_matrixCommitted;
    committed->m_valid = input->m_valid;
    committed->m_hmac = input->m_hmac;
    committed->m_key = input->m_key;
}

// VERIFY: 29727393 auto
// OFFSET: 0x008CAB40
void GruntLogin::EnterToken(const char* token) {
    SStrCopy(this->m_token, token, sizeof(this->m_token));
    LOGIN_STATE state = this->NextSecurityState(LOGIN_STATE_TOKEN);
    this->m_loginResponse->UpdateLoginStatus(state, LOGIN_OK, nullptr, 0);
}

// VERIFY: d42deb51 auto
// OFFSET: 0x008CAA30
bool GruntLogin::GetMatrixInfo(uint32_t* width, uint32_t* height, uint32_t* minDigits, uint32_t* maxDigits, bool* flipCoords, uint32_t* challengeCount) {
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

// VERIFY: ab7f843b auto
// OFFSET: 0x008CAA90
bool GruntLogin::GetMatrixCoordinates(uint32_t index, uint32_t* x, uint32_t* y) {
    if (!this->m_matrixEnabled || !this->m_matrixCoordinates || index >= this->m_matrixChallengeCount) {
        return false;
    }

    uint32_t coordinate = this->m_matrixCoordinates[index];
    *x = coordinate % this->m_matrixWidth;
    *y = coordinate / this->m_matrixWidth;
    return true;
}

// VERIFY: 10ae7b25 manual
// OFFSET: 0x00465870
void GruntLogin::MATRIXINPUT::Finish(SHA1::Digest& digest) {
    this->m_hmac.Finish(digest);
    this->m_valid = false;
}

// VERIFY: b49b6056 manual
// OFFSET: 0x00468F60
static void ProcessMatrixRC4(uint8_t* data, uint32_t length, SARC4Key* key) {
    uint8_t x = key->m_x;
    uint8_t y = key->m_y;

    for (uint32_t i = 0; i < length; i++) {
        x++;
        uint8_t a = key->m_state[x];
        y += a;
        key->m_state[x] = key->m_state[y];
        key->m_state[y] = a;
        data[i] ^= key->m_state[static_cast<uint8_t>(a + key->m_state[x])];
    }

    key->m_x = x;
    key->m_y = y;
}

// VERIFY: d046e16b auto
// OFFSET: 0x008CA4D0
void GruntLogin::FileAccept(bool accept, uint64_t offset) {
    this->m_clientLink->FileStatus(accept, offset);
}

// VERIFY: 50215692 auto
// OFFSET: 0x008CA550
void GruntLogin::SendSurveyData(uint16_t size, uint8_t* data) {
    this->m_clientLink->SendSurveyData(size, data);
}

// VERIFY: d5f0969d auto
// OFFSET: 0x008CA560
void GruntLogin::SendSurveyFailed() {
    this->m_clientLink->SendSurveyFailed();
}

// VERIFY: e0ac23d3 auto
// OFFSET: 0x008CA570
void GruntLogin::FileComplete(bool success) {
    this->m_clientLink->FileComplete();
}

// VERIFY: 20c829de manual
// OFFSET: 0x008CA7F0
int32_t GruntLogin::Wait(const char* message, LoginData& loginData) {
    while (this->m_logonStatus == -1) {
        OsSleep(100);
    }

    if (this->m_logonStatus) {
        memset(&loginData, 0, sizeof(loginData));
    } else {
        loginData.m_loginServerID = 0;
        SStrCopy(loginData.m_account, this->m_username, sizeof(loginData.m_account));
        memcpy(loginData.m_sessionKey, this->m_sessionKey, sizeof(loginData.m_sessionKey));
    }

    return this->m_logonStatus == 0;
}

// VERIFY: 66d294df manual
// OFFSET: 0x008CA880
void GruntLogin::EnterPIN(int32_t count, uint8_t* digits) {
    char pin[12];

    for (int32_t i = 0; i < count; i++) {
        pin[i] = digits[i] + '0';
    }

    pin[count] = '\0';
    OsSecureRandom(this->m_pinClientSalt, sizeof(this->m_pinClientSalt));

    SHA1 sha;
    sha.Prepare();
    sha.Process(this->m_pinSalt, sizeof(this->m_pinSalt));
    sha.Process(pin);

    SHA1::Digest digest;
    sha.Finish(digest);

    sha.Prepare();
    sha.Process(this->m_pinClientSalt, sizeof(this->m_pinClientSalt));
    sha.Process(&digest, sizeof(digest));
    sha.Finish(*reinterpret_cast<SHA1::Digest*>(this->m_pinHash));

    this->m_loginResponse->UpdateLoginStatus(this->NextSecurityState(LOGIN_STATE_PIN), LOGIN_OK, nullptr, 0);
}

// VERIFY: b2336d08 auto
// OFFSET: 0x008CAFA0
void GruntLogin::EnterMatrix(uint8_t value) {
    GruntLogin::MATRIXINPUT* input = this->m_matrixCommitted;
    ProcessMatrixRC4(&value, 1, &input->m_key);
    input->m_hmac.Process(&value, 1);
}

// VERIFY: be3941c2 auto
// OFFSET: 0x008CAFE0
void GruntLogin::FinalizeMatrix() {
    this->m_matrixInput->Finish(*reinterpret_cast<SHA1::Digest*>(this->m_matrixProof));
    this->m_loginResponse->UpdateLoginStatus(this->NextSecurityState(LOGIN_STATE_MATRIX), LOGIN_OK, nullptr, 0);
}

// VERIFY: 2aeb1ed6 manual
// OFFSET: 0x008CB020
void GruntLogin::ReconnectResult(Grunt::Result result, const uint8_t* sessionKey, uint32_t sessionKeyLength, uint16_t flags) {
    if (!this->IsReconnect()) {
        this->m_logonStatus = 1;
        this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_FAILED, LOGIN_FAILED, nullptr, 0);
        return;
    }

    switch (result) {
        case Grunt::GRUNT_RESULT_SUCCESS:
        case Grunt::GRUNT_RESULT_SUCCESS_SURVEY:
            this->m_clientLink->GetRealmList();
            this->m_loginResponse->UpdateLoginStatus(LOGIN_STATE_RESPONSE_CONNECTED, LOGIN_OK, nullptr, 0);
            return;

        case Grunt::GRUNT_RESULT_BAD_VERSION:
            this->m_loginResponse->HandleRealmData(2, nullptr);
            break;

        case Grunt::GRUNT_RESULT_DOWNLOAD_FILE:
            this->m_loginResponse->HandleRealmData(3, nullptr);
            break;

        default:
            this->m_loginResponse->HandleRealmData(1, nullptr);
            break;
    }

    this->m_clientLink->Disconnect();
}
