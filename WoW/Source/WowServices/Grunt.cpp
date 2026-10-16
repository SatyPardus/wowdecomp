#include "Grunt.h"
#include <cstdio>
#include <cstring>
#include <malloc.h>
#include "Engine/Source/Os/OsSecureRandom.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "WoW/Source/WowServices/SRP6_Random.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "WoW/Source/Client.h"

// STATIC: 0x00B24D00
static Grunt::Command<Grunt::ClientLink> s_authCmds[] = {
    { 0x00, {}, "ClientLink::CMD_AUTH_LOGON_CHALLENGE", &Grunt::ClientLink::CmdAuthLogonChallenge },
    { 0x01, {}, "ClientLink::CMD_AUTH_LOGON_PROOF", &Grunt::ClientLink::CmdAuthLogonProof },
    { 0x02, {}, "ClientLink::CMD_AUTH_RECONNECT_CHALLENGE", &Grunt::ClientLink::CmdAuthReconnectChallenge },
    { 0x03, {}, "ClientLink::CMD_AUTH_RECONNECT_PROOF", &Grunt::ClientLink::CmdAuthReconnectProof },
    { 0x10, {}, "ClientLink::CMD_REALM_LIST", &Grunt::ClientLink::CmdRealmList },
    { 0x30, {}, "ClientLink::CMD_XFER_INITIATE", &Grunt::ClientLink::CmdXferInitiate },
    { 0x31, {}, "ClientLink::CMD_XFER_DATA", &Grunt::ClientLink::CmdXferData },
};

// STATIC: 0x00D45CD0
static char s_accountNameUnDecorated[1280];

// STATIC: 0x00A94A9C
static const uint8_t s_serverAuthKey[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };

// STATIC: 0x00B24CA0
static const char* s_linkTypeNames[] = { "Grunt", "Battlenet" };

// STATIC: 0x00B24D70
static Grunt::Command<Grunt::ServerLink> s_serverCmds[] = {
    { 0x00, {}, "ServerLink::CMD_GRUNT_AUTH_CHALLENGE", &Grunt::ServerLink::CmdGruntAuthChallenge },
    { 0x02, {}, "ServerLink::CMD_GRUNT_AUTH_VERIFY", &Grunt::ServerLink::CmdGruntAuthVerify },
    { 0x10, {}, "ServerLink::CMD_GRUNT_CONN_PING", &Grunt::ServerLink::CmdGruntConnPing },
    { 0x11, {}, "ServerLink::CMD_GRUNT_CONN_PONG", &Grunt::ServerLink::CmdGruntConnPong },
    { 0x20, {}, "ServerLink::CMD_GRUNT_HELLO", &Grunt::ServerLink::CmdGruntHello },
    { 0x21, {}, "ServerLink::CMD_GRUNT_PROVESESSION", &Grunt::ServerLink::CmdGruntProveSession },
    { 0x24, {}, "ServerLink::CMD_GRUNT_KICK", &Grunt::ServerLink::CmdGruntKick },
    { 0x29, {}, "ServerLink::CMD_GRUNT_PCWARNING", &Grunt::ServerLink::CmdGruntPcWarning },
    { 0x41, {}, "ServerLink::CMD_GRUNT_STRINGS", &Grunt::ServerLink::CmdGruntStrings },
    { 0x44, {}, "ServerLink::CMD_GRUNT_SUNKENUPDATE", &Grunt::ServerLink::CmdGruntSunkenUpdate },
    { 0x46, {}, "ServerLink::CMD_GRUNT_SUNKEN_ONLINE", &Grunt::ServerLink::CmdGruntSunkenOnline },
};

// STATIC: 0x00B24E48
static Grunt::ServerLink::Feature s_serverFeatures[] = {
    { 0x00030001, 14 },
    { 0x00030002, 14 },
    { 0x00030003, 14 },
    { 0x00030004, 13 },
    { 0x00030005, 1 },
    { 0x00030006, 14 },
    { 0x00040001, 1 },
};

// INLINED
Grunt::Pending::Pending(uint32_t timeout) {
    this->m_timeout = timeout;
    this->unused_0008 = 0;
}

// INLINED
void Grunt::Timer::Shutdown() {
    if (!this->m_thread.Valid()) {
        return;
    }

    this->m_event.Set();
    this->m_thread.Wait(0xFFFFFFFF);
    this->m_thread.Close();

    this->m_lock.Enter();
    this->m_list.UnlinkAll();
    this->m_lock.Leave();
}

// VERIFY: 93be7f01 auto
// OFFSET: 0x008CBA40
void Grunt::ClientLink::Send(CDataStore& msg) {
    this->m_lock.Enter();

    if (this->m_connection) {
        uint32_t size = msg.m_size;
        void* data;
        msg.GetDataInSitu(data, size);
        this->m_connection->Send(static_cast<uint8_t*>(data), size, false);
    }

    this->m_lock.Leave();
}

// VERIFY: c3ecd7d1 auto
// OFFSET: 0x008CBA90
void Grunt::ClientLink::Shutdown() {
    this->m_lock.Enter();

    if (this->m_connection) {
        this->m_connection->Disconnect();
        this->m_connection->SetResponse(nullptr, false);
        this->m_connection->Release();
        this->m_connection = nullptr;
    }

    this->m_lock.Leave();
}

// VERIFY: 08907387 auto
// OFFSET: 0x008CBAE0
void Grunt::ClientLink::Disconnect() {
    this->m_lock.Enter();

    if (this->m_connection) {
        this->m_connection->Disconnect();
    }

    this->m_lock.Leave();
}

// VERIFY: a80ac601 auto
// OFFSET: 0x008CBB10
void Grunt::ClientLink::SetState(int32_t state) {
    this->m_lock.Enter();
    this->m_state = state;
    this->m_lock.Leave();
}

// VERIFY: 32e7119e manual
// OFFSET: 0x008CBB40
void Grunt::ClientLink::Connect(const char* address) {
    if (this->m_state) {
        return;
    }

    this->SetState(1);

    this->m_connection = new (__FILE__, __LINE__) WowConnection(this, nullptr);
    this->m_connection->SetType(WOWC_TYPE_STREAM);

    if (SStrChr(address, ':')) {
        this->m_connection->Connect(address, 5000);
    } else {
        this->m_connection->Connect(address, 3724, 5000);
    }
}

// VERIFY: 8c632a8e auto
// OFFSET: 0x008CCCB0
void Grunt::ClientLink::WCConnected(WowConnection* connection, WowConnection* inbound, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_lock.Enter();
    this->SetState(2);
    bool failed = !this->m_clientResponse->Connected(addr->peerAddr);
    this->m_localAddress = OsNetAddrGetAddress(&addr->selfAddr, nullptr);
    this->m_lock.Leave();

    if (failed) {
        this->Disconnect();
    }
}

// VERIFY: 1c69ff99 manual
// OFFSET: 0x008CCD40
void Grunt::ClientLink::WCDisconnected(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_lock.Enter();
    this->Shutdown();
    this->SetState(0);
    this->m_lock.Leave();

    if (this->m_clientResponse->Disconnected()) {
        delete this;
    }
}

// VERIFY: bc4a5436 manual
// OFFSET: 0x008CCDA0
void Grunt::ClientLink::WCCantConnect(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_lock.Enter();
    this->Shutdown();
    this->SetState(0);
    this->m_lock.Leave();

    if (this->m_clientResponse->CantConnect()) {
        delete this;
    }
}

// VERIFY: 5fb5257a auto
// OFFSET: 0x008CCE00
void Grunt::ClientLink::PackLogon(CDataStore& msg, const Grunt::ClientLink::Logon& logon) {
    uint32_t start = msg.m_size;

    msg.Put(static_cast<uint16_t>(0));
    msg.Put(logon.m_programId);
    msg.Put(logon.m_versionMajor);
    msg.Put(logon.m_versionMinor);
    msg.Put(logon.m_versionPatch);
    msg.Put(logon.m_build);
    msg.Put(logon.m_processor);
    msg.Put(logon.m_os);
    msg.Put(logon.m_locale);
    msg.Put(static_cast<uint32_t>(logon.m_timeZone));
    msg.Put(this->m_localAddress);

    uint8_t length = SStrLen(this->m_accountName);
    msg.Put(length);
    msg.PutData(this->m_accountName, length);

    msg.Set(start, static_cast<uint16_t>(msg.m_size - start - 2));
}

// VERIFY: abfd5c66 manual
// OFFSET: 0x008CD7E0
void Grunt::ClientLink::LogonNewSession(const Grunt::ClientLink::Logon& logon) {
    this->SetState(3);

    SStrCopy(this->m_accountName, logon.m_accountName, sizeof(this->m_accountName));
    SStrUpper(this->m_accountName);

    char* password = static_cast<char*>(alloca(SStrLen(logon.m_password) + 1));
    SStrCopy(password, logon.m_password, STORM_MAX_STR);
    SStrUpper(password);

    SStrCopy(s_accountNameUnDecorated, this->m_accountName, STORM_MAX_STR);

    char* decoration = SStrChr(s_accountNameUnDecorated, '#');

    if (decoration) {
        *decoration = '\0';
    }

    this->m_srpClient.BeginAuthentication(s_accountNameUnDecorated, password);

    CDataStoreCache<1024> msg;
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(static_cast<uint8_t>(8));
    this->PackLogon(msg, logon);
    msg.Finalize();
    this->Send(msg);

    memset(password, 0, SStrLen(password));
}

// VERIFY: 902b90a7 manual
// OFFSET: 0x008CD9F0
void Grunt::ClientLink::LogonStoredSession(const Grunt::ClientLink::Logon& logon) {
    this->SetState(3);

    SStrCopy(this->m_accountName, logon.m_accountName, sizeof(this->m_accountName));
    SStrUpper(this->m_accountName);
    memcpy(this->m_reconnectKey, logon.m_password, sizeof(this->m_reconnectKey));

    CDataStoreCache<1024> msg;
    msg.Put(static_cast<uint8_t>(2));
    msg.Put(static_cast<uint8_t>(8));
    this->PackLogon(msg, logon);
    msg.Finalize();
    this->Send(msg);
}

// VERIFY: 09a17985 manual
// OFFSET: 0x008CDB90
void Grunt::ClientLink::SendSurveyData(uint16_t size, uint8_t* data) {
    CDataStoreCache<1024> msg;
    msg.Put(static_cast<uint8_t>(4));
    msg.Put(this->m_surveyId);
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(size);
    msg.PutData(data, size);
    msg.Finalize();
    this->Send(msg);
}

// VERIFY: d759d347 manual
// OFFSET: 0x008CDD00
void Grunt::ClientLink::SendSurveyFailed() {
    CDataStoreCache<1024> msg;
    msg.Put(static_cast<uint8_t>(4));
    msg.Put(this->m_surveyId);
    msg.Put(static_cast<uint8_t>(1));
    msg.Put(static_cast<uint16_t>(0));
    msg.Finalize();
    this->Send(msg);
}

// VERIFY: 01aad7f9 auto
// OFFSET: 0x008CE5C0
void Grunt::Timer::Insert(Grunt::Timer::Event& event) {
    this->m_lock.Enter();

    event.m_nextTime = event.m_interval + OsGetAsyncTimeMs();

    for (Grunt::Timer::Event* existing = this->m_list.Head(); !this->m_list.IsEnd(existing); existing = this->m_list.RawNext(existing)) {
        if (static_cast<int32_t>(event.m_nextTime - existing->m_nextTime) < 0) {
            this->m_list.LinkNode(&event, 2, existing);
            this->m_lock.Leave();
            return;
        }
    }

    this->m_list.LinkToTail(&event);
    this->m_lock.Leave();
}

// VERIFY: 2b47cb0c manual
// OFFSET: 0x008CE6D0
Grunt::Pending::~Pending() {
}

// VERIFY: 5ce0f696 manual
// OFFSET: 0x008CED90
int32_t Grunt::Timer::Pump() {
    this->m_lock.Enter();

    Grunt::Timer::Event* event = this->m_list.Head();

    if (!event) {
        this->m_lock.Leave();
        return 100;
    }

    int32_t remaining = event->m_nextTime - OsGetAsyncTimeMs();

    if (remaining > 0) {
        this->m_lock.Leave();
        return remaining;
    }

    this->m_list.UnlinkNode(event);
    this->m_lock.Leave();

    event->Call();
    this->Insert(*event);

    return 100;
}

// VERIFY: c0fe3e15 manual
// OFFSET: 0x008CEE20
Grunt::Timer::~Timer() {
    this->Shutdown();
}

// VERIFY: 374cd430 manual
// OFFSET: 0x008CEF70
void Grunt::Pending::CheckExpired(bool all) {
    uint32_t now = OsGetAsyncTimeMs();

    TSExplicitList<Grunt::Pending::Data, offsetof(Grunt::Pending::Data, m_link)> expired;

    this->m_lock.Enter();

    Grunt::Pending::Data* data = this->m_list.Head();

    while (!this->m_list.IsEnd(data)) {
        if (!all) {
            if (!data->m_time) {
                data = this->m_list.RawNext(data);
                continue;
            }

            if (now - data->m_time < this->m_timeout) {
                break;
            }
        }

        this->m_list.UnlinkNode(data);
        expired.LinkToTail(data);
        data = this->m_list.RawNext(data);
    }

    this->m_lock.Leave();

    for (data = expired.Head(); data; data = expired.DeleteNode(data)) {
        this->Expired(*data);
    }
}

// VERIFY: 06b75110 manual
// OFFSET: 0x008CEEC0
Grunt::Pending::Data* Grunt::Pending::Remove(uint32_t id) {
    this->m_lock.Enter();

    for (Grunt::Pending::Data* data = this->m_list.Head(); data; data = this->m_list.Next(data)) {
        if (data->m_id == id) {
            this->m_list.UnlinkNode(data);
            this->m_lock.Leave();

            return data;
        }
    }

    this->m_lock.Leave();

    return nullptr;
}

// VERIFY: a73f2c4e manual
// OFFSET: 0x008CF600
void Grunt::ClientLink::Call() {
    this->CheckExpired(false);

    this->m_lock.Enter();

    if (this->m_state == 2) {
        this->m_clientResponse->GetLogonMethod();
    } else if (this->m_state == 6 && !this->m_clientResponse->OnlineIdle()) {
        this->Disconnect();
    }

    this->m_lock.Leave();
}

// VERIFY: 10d55201 manual
// OFFSET: [0x008CF680, 0x008CF910]
Grunt::ClientLink::~ClientLink() {
    this->m_timer.Shutdown();
    this->Shutdown();
}

// VERIFY: 8e029f02 auto
// OFFSET: 0x008CF770
uint32_t STORMAPI Grunt::Timer::ThreadProc(void* param) {
    Grunt::Timer* timer = static_cast<Grunt::Timer*>(param);
    uint32_t timeout;

    do {
        timeout = timer->Pump();
    } while (timer->m_event.Wait(timeout));

    return 1;
}

// VERIFY: 44245c80 manual
// OFFSET: 0x008CF7A0
Grunt::Timer::Timer() : m_event(0, 0) {
    SThread::Create(Grunt::Timer::ThreadProc, this, this->m_thread, const_cast<char*>("GruntTimerEvt"), 0);
}

// VERIFY: ec01e82e manual
// OFFSET: 0x008CF800
Grunt::ClientLink::ClientLink(Grunt::ClientResponse& clientResponse) : Grunt::Pending(18000), m_serverId(0), m_accountFlags(0), m_surveyId(0), m_localAddress(0), m_connection(nullptr), m_clientResponse(&clientResponse) {
    this->SetState(0);

    if (this->m_timer.m_thread.Valid()) {
        this->m_interval = 100;
        this->m_timer.Insert(*this);
    }
}

void Grunt::ClientLink::WCMessageReady(WowConnection* connection, uint32_t timeStamp, CDataStore* msg) {
}

void Grunt::ClientLink::Expired(Grunt::Pending::Data& data) {
}

// VERIFY: 54a85476 auto
// OFFSET: 0x008CBBF0
bool CanReadNumBytes(CDataStore& msg, uint32_t bytes) {
    return msg.m_read <= msg.m_size && msg.m_size - msg.m_read >= bytes;
}

// VERIFY: 58b79446 auto
// OFFSET: 0x008CC250
int32_t Grunt::ClientLink::CmdRealmList(CDataStore& msg) {
    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 2) {
        return 0;
    }

    uint16_t size;
    msg.Get(size);

    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < size) {
        return 0;
    }

    uint32_t start = msg.m_read;

    uint32_t unknown;
    msg.Get(unknown);

    uint32_t realmStart = msg.m_read;

    uint16_t count;
    msg.Get(count);

    for (uint32_t i = 0; i < count; i++) {
        uint8_t type;
        uint8_t locked;
        uint8_t flags;
        char name[256];
        char address[256];
        float population;
        uint8_t characters;
        uint8_t timeZone;
        uint8_t realmId;

        msg.Get(type);
        msg.Get(locked);
        msg.Get(flags);
        msg.GetString(name, sizeof(name));
        msg.GetString(address, sizeof(address));
        msg.Get(population);
        msg.Get(characters);
        msg.Get(timeZone);
        msg.Get(realmId);

        if (flags & 0x4) {
            uint8_t versionMajor;
            uint8_t versionMinor;
            uint8_t versionRevision;
            uint16_t build;

            msg.Get(versionMajor);
            msg.Get(versionMinor);
            msg.Get(versionRevision);
            msg.Get(build);
        }

        if (msg.m_read > msg.m_size) {
            return 1;
        }
    }

    uint16_t trailer;
    msg.Get(trailer);

    uint32_t end = msg.m_read;

    if (end > msg.m_size || end - start != size) {
        return 1;
    }

    msg.m_read = realmStart;
    this->m_clientResponse->RealmListResult(&msg);
    msg.m_read = end;

    return 2;
}

// VERIFY: 41cff6c4 manual
// OFFSET: 0x008CC3E0
int32_t Grunt::ClientLink::CmdAuthLogonChallenge(CDataStore& msg) {
    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 2) {
        return 0;
    }

    uint8_t protocol;
    msg.Get(protocol);

    if (protocol) {
        return 1;
    }

    uint8_t result;
    msg.Get(result);

    if (result == Grunt::GRUNT_RESULT_SUCCESS) {
        if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 33) {
            return 0;
        }

        void* serverPublicKey;
        msg.GetDataInSitu(serverPublicKey, 32);

        uint8_t generatorLength;
        msg.Get(generatorLength);

        if (!CanReadNumBytes(msg, generatorLength + 1)) {
            return 0;
        }

        void* generator;
        msg.GetDataInSitu(generator, generatorLength);

        uint8_t largeSafePrimeLength;
        msg.Get(largeSafePrimeLength);

        if (!CanReadNumBytes(msg, largeSafePrimeLength + 48)) {
            return 0;
        }

        void* largeSafePrime;
        msg.GetDataInSitu(largeSafePrime, largeSafePrimeLength);

        void* salt;
        msg.GetDataInSitu(salt, 32);

        void* versionChallenge;
        msg.GetDataInSitu(versionChallenge, 16);

        if (!CanReadNumBytes(msg, 1)) {
            return 0;
        }

        uint8_t securityFlags;
        msg.Get(securityFlags);

        uint32_t pinSeed = 0;
        void* pinSalt = nullptr;

        if (securityFlags & 0x1) {
            if (!CanReadNumBytes(msg, 20)) {
                return 0;
            }

            msg.Get(pinSeed);
            msg.GetDataInSitu(pinSalt, 16);
        }

        uint8_t matrixWidth = 0;
        uint8_t matrixHeight = 0;
        uint8_t matrixDigitCount = 0;
        uint8_t matrixChallengeCount = 0;
        uint64_t matrixSeed = 0;

        if (securityFlags & 0x2) {
            if (!CanReadNumBytes(msg, 12)) {
                return 0;
            }

            msg.Get(matrixWidth);
            msg.Get(matrixHeight);
            msg.Get(matrixDigitCount);
            msg.Get(matrixChallengeCount);
            msg.Get(matrixSeed);

            if ((securityFlags & 0x2) && !matrixChallengeCount) {
                return 1;
            }
        }

        uint8_t tokenRequired = 0;

        if (securityFlags & 0x4) {
            if (!CanReadNumBytes(msg, 1)) {
                return 0;
            }

            msg.Get(tokenRequired);
        }

        if (msg.m_read > msg.m_size) {
            return 1;
        }

        memcpy(this->m_serverChallenge, serverPublicKey, sizeof(this->m_serverChallenge));

        uint8_t seed[16];
        OsSecureRandom(seed, sizeof(seed));
        SRP6_Random random(seed, sizeof(seed));

        if (this->m_srpClient.CalculateProof(static_cast<uint8_t*>(largeSafePrime), largeSafePrimeLength, static_cast<uint8_t*>(generator), generatorLength, static_cast<uint8_t*>(salt), 32, static_cast<uint8_t*>(serverPublicKey), 32, random)) {
            this->SetState(2);
            this->m_clientResponse->LogonResult(Grunt::GRUNT_RESULT_INCORRECT_PASSWORD, nullptr, 0, 0);
        } else {
            this->SetState(4);
            this->m_clientResponse->SetPinInfo((securityFlags & 0x1) == 0x1, pinSeed, static_cast<uint8_t*>(pinSalt));
            this->m_clientResponse->SetMatrixInfo((securityFlags & 0x2) == 0x2, matrixWidth, matrixHeight, matrixDigitCount, matrixDigitCount, false, matrixChallengeCount, matrixSeed, this->m_srpClient.m_sessionKey, sizeof(this->m_srpClient.m_sessionKey));
            this->m_clientResponse->SetTokenInfo((securityFlags & 0x4) == 0x4, tokenRequired);
            this->m_clientResponse->GetVersionProof(static_cast<uint8_t*>(versionChallenge));
        }

        return 2;
    }

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    this->SetState(2);

    if (result >= 33) {
        DebugLog("Unknown grunt result %d @ %d", result, __LINE__);
        result = Grunt::GRUNT_RESULT_DISCONNECTED;
    }

    this->m_clientResponse->LogonResult(static_cast<Grunt::Result>(result), nullptr, 0, 0);

    return 2;
}

// VERIFY: c4dc2df1 manual
// OFFSET: 0x008CC7A0
int32_t Grunt::ClientLink::CmdAuthLogonProof(CDataStore& msg) {
    if (msg.m_read >= msg.m_size) {
        return 0;
    }

    uint8_t result;
    msg.Get(result);

    if (result == Grunt::GRUNT_RESULT_SUCCESS) {
        if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 24) {
            return 0;
        }

        void* serverProof;
        msg.GetDataInSitu(serverProof, 20);

        uint32_t accountFlags = 0;
        msg.Get(accountFlags);

        uint32_t surveyId;
        msg.Get(surveyId);

        if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 2) {
            return 0;
        }

        uint16_t loginFlags = 0;
        msg.Get(loginFlags);

        if (msg.m_read > msg.m_size) {
            return 1;
        }

        if (this->m_srpClient.VerifyServerProof(static_cast<uint8_t*>(serverProof), 20)) {
            this->SetState(2);
            this->m_clientResponse->LogonResult(Grunt::GRUNT_RESULT_UNKNOWN_11, nullptr, 0, 0);
        } else {
            this->m_accountFlags = accountFlags;
            this->m_serverId = 0;
            this->m_surveyId = surveyId;
            this->SetState(6);
            this->m_clientResponse->LogonResult(Grunt::GRUNT_RESULT_SUCCESS, this->m_srpClient.m_sessionKey, sizeof(this->m_srpClient.m_sessionKey), loginFlags);
        }

        return 2;
    }

    if (result == Grunt::GRUNT_RESULT_UNKNOWN_ACCOUNT) {
        if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 2) {
            return 0;
        }

        uint16_t unknown = 0;
        msg.Get(unknown);
    }

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    if (result != Grunt::GRUNT_RESULT_DOWNLOAD_FILE) {
        this->SetState(2);

        if (result >= 33) {
            DebugLog("Unknown grunt result %d @ %d", result, __LINE__);
            result = Grunt::GRUNT_RESULT_DISCONNECTED;
        }

        this->m_clientResponse->LogonResult(static_cast<Grunt::Result>(result), nullptr, 0, 0);
    }

    return 2;
}

// VERIFY: ea70b617 manual
// OFFSET: 0x008CD710
void Grunt::ClientLink::WCDataReady(WowConnection* connection, uint32_t timeStamp, uint8_t* data, int32_t length) {
    this->m_datastore.PutArray(data, length);
    this->m_datastore.Finalize();

    uint32_t pos = 0;

    if (Grunt::Command<Grunt::ClientLink>::Process(this->m_datastore, s_authCmds, sizeof(s_authCmds) / sizeof(s_authCmds[0]), *this, pos)) {
        uint32_t remaining = this->m_datastore.m_size - pos;
        this->m_datastore.m_read = pos;

        void* rest;
        this->m_datastore.GetDataInSitu(rest, remaining);
        this->m_datastore.m_read = -1;
        this->m_datastore.Reset();
        this->m_datastore.PutData(rest, remaining);
    } else {
        this->m_datastore.m_read = -1;
        this->m_datastore.Reset();
        this->Disconnect();
    }
}

// VERIFY: 8e4d36ef manual
// OFFSET: 0x008CE0F0
void Grunt::ClientLink::GetRealmList() {
    if (this->m_state != 6) {
        return;
    }

    CDataStoreCache<1024> msg;
    msg.Put(static_cast<uint8_t>(0x10));
    msg.Put(static_cast<uint32_t>(0));
    msg.Finalize();
    this->Send(msg);
}

// VERIFY: 58cb67ed manual
// OFFSET: 0x008CE720
void Grunt::ClientLink::ProveVersion(const uint8_t* versionChecksum) {
    CDataStoreCache<1024> msg;
    Grunt::CdKey cdKeys;
    SHA1_CONTEXT context;
    uint8_t hash[20];

    if (!this->m_clientResponse->GetCdKeys(cdKeys)) {
        cdKeys.m_count = 0;
    }

    if (this->m_state == 4) {
        msg.Put(static_cast<uint8_t>(1));
        msg.PutData(this->m_srpClient.m_publicKey, sizeof(this->m_srpClient.m_publicKey));
        msg.PutData(this->m_srpClient.m_clientProof, sizeof(this->m_srpClient.m_clientProof));

        SHA1_Init(&context);
        SHA1_Update(&context, this->m_srpClient.m_publicKey, sizeof(this->m_srpClient.m_publicKey));
        SHA1_Update(&context, versionChecksum, 20);
        SHA1_Final(hash, &context);
        msg.PutData(hash, sizeof(hash));

        msg.Put(static_cast<uint8_t>(cdKeys.m_count));

        for (uint32_t i = 0; i < static_cast<uint8_t>(cdKeys.m_count); i++) {
            Grunt::CdKey::Info& key = cdKeys.m_keys.m_data[i];
            msg.Put(key.m_keyLength);
            msg.Put(key.m_productId);
            msg.PutData(key.m_publicValue, sizeof(key.m_publicValue));

            SHA1_Init(&context);
            SHA1_Update(&context, this->m_srpClient.m_publicKey, sizeof(this->m_srpClient.m_publicKey));
            SHA1_Update(&context, this->m_serverChallenge, sizeof(this->m_serverChallenge));
            SHA1_Update(&context, key.m_hash, sizeof(key.m_hash));
            SHA1_Final(hash, &context);
            msg.PutData(hash, sizeof(hash));
        }

        uint8_t securityFlags = 0;

        bool pinEnabled;
        uint8_t* pinSalt;
        uint8_t* pinHash;
        this->m_clientResponse->GetPinInfo(&pinEnabled, &pinSalt, &pinHash);

        if (pinEnabled) {
            securityFlags = 0x1;
        }

        bool matrixEnabled;
        const uint8_t* matrixProof;
        this->m_clientResponse->GetMatrixProof(&matrixEnabled, &matrixProof);

        if (matrixEnabled) {
            securityFlags |= 0x2;
        }

        char token[17] = {};
        bool tokenEnabled;
        this->m_clientResponse->GetTokenInfo(&tokenEnabled, token);
        uint8_t tokenLength = SStrLen(token);

        if (tokenEnabled) {
            securityFlags |= 0x4;
        }

        msg.Put(securityFlags);

        if (pinEnabled) {
            msg.PutData(pinSalt, 16);
            msg.PutData(pinHash, 20);
        }

        if (matrixEnabled) {
            msg.PutData(matrixProof, 20);
        }

        if (tokenEnabled) {
            msg.Put(tokenLength);
            msg.PutArray(reinterpret_cast<uint8_t*>(token), tokenLength);
        }
    } else {
        uint32_t length = SStrLen(this->m_accountName);

        uint8_t clientSalt[16];
        MD5_CTX md5;
        MD5Init(&md5);
        MD5Update(&md5, reinterpret_cast<uint8_t*>(this->m_accountName), length);
        OsSecureRandom(clientSalt, sizeof(clientSalt));
        MD5Update(&md5, clientSalt, sizeof(clientSalt));
        MD5Final(clientSalt, &md5);

        msg.Put(static_cast<uint8_t>(3));
        msg.PutData(clientSalt, sizeof(clientSalt));

        SHA1_Init(&context);
        SHA1_Update(&context, reinterpret_cast<uint8_t*>(this->m_accountName), length);
        SHA1_Update(&context, clientSalt, sizeof(clientSalt));
        SHA1_Update(&context, this->m_serverChallenge, 16);
        SHA1_Update(&context, this->m_reconnectKey, sizeof(this->m_reconnectKey));
        SHA1_Final(hash, &context);
        msg.PutData(hash, sizeof(hash));

        SHA1_Init(&context);
        SHA1_Update(&context, clientSalt, sizeof(clientSalt));
        SHA1_Update(&context, versionChecksum, 20);
        SHA1_Final(hash, &context);
        msg.PutData(hash, sizeof(hash));

        msg.Put(static_cast<uint8_t>(cdKeys.m_count));

        for (uint32_t i = 0; i < static_cast<uint8_t>(cdKeys.m_count); i++) {
            Grunt::CdKey::Info& key = cdKeys.m_keys.m_data[i];
            msg.Put(key.m_keyLength);
            msg.Put(key.m_productId);
            msg.PutData(key.m_publicValue, sizeof(key.m_publicValue));

            SHA1_Init(&context);
            SHA1_Update(&context, clientSalt, sizeof(clientSalt));
            SHA1_Update(&context, this->m_serverChallenge, 16);
            SHA1_Update(&context, key.m_hash, sizeof(key.m_hash));
            SHA1_Final(hash, &context);
            msg.PutData(hash, sizeof(hash));
        }
    }

    msg.Finalize();
    this->Send(msg);
}

// VERIFY: 53c21c45 manual
// OFFSET: 0x008CC980
int32_t Grunt::ClientLink::CmdAuthReconnectChallenge(CDataStore& msg) {
    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 17) {
        return 0;
    }

    uint8_t result;
    msg.Get(result);

    if (result) {
        return 1;
    }

    void* challenge;
    msg.GetDataInSitu(challenge, 16);

    void* versionChallenge;
    msg.GetDataInSitu(versionChallenge, 16);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    memcpy(this->m_serverChallenge, challenge, 16);
    this->SetState(5);
    this->m_clientResponse->GetVersionProof(static_cast<const uint8_t*>(versionChallenge));
    return 2;
}

// VERIFY: 39d99ecd auto
// OFFSET: 0x008CCA50
int32_t Grunt::ClientLink::CmdAuthReconnectProof(CDataStore& msg) {
    if (msg.m_read >= msg.m_size) {
        return 0;
    }

    uint8_t result;
    msg.Get(result);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    if (result) {
        this->SetState(2);

        if (result >= 33) {
            result = Grunt::GRUNT_RESULT_DISCONNECTED;
        }

        this->m_clientResponse->ReconnectResult(static_cast<Grunt::Result>(result), nullptr, 0, 0);
        return 2;
    }

    if (msg.m_size - msg.m_read < 2) {
        return 0;
    }

    uint16_t flags = 0;
    msg.Get(flags);
    this->SetState(6);
    this->m_clientResponse->ReconnectResult(Grunt::GRUNT_RESULT_SUCCESS, this->m_reconnectKey, sizeof(this->m_reconnectKey), flags);
    return 2;
}

// VERIFY: 3cf29e43 manual
// OFFSET: 0x008CCB40
int32_t Grunt::ClientLink::CmdXferInitiate(CDataStore& msg) {
    if (msg.m_read >= msg.m_size) {
        return 0;
    }

    uint8_t length;
    msg.Get(length);

    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < static_cast<uint32_t>(length + 24)) {
        return 0;
    }

    if (length >= 0xFE) {
        length = 0xFE;
    }

    char fileName[256];
    msg.GetArray(reinterpret_cast<uint8_t*>(fileName), length);
    fileName[length] = '\0';

    uint64_t size;
    msg.Get(size);

    void* md5;
    msg.GetDataInSitu(md5, 16);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    this->m_clientResponse->GetFileStatus(fileName, size, static_cast<const uint8_t*>(md5));
    return 2;
}

// VERIFY: 296d35b8 auto
// OFFSET: 0x008CCC20
int32_t Grunt::ClientLink::CmdXferData(CDataStore& msg) {
    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < 2) {
        return 0;
    }

    uint16_t size;
    msg.Get(size);

    if (msg.m_read > msg.m_size || msg.m_size - msg.m_read < size) {
        return 0;
    }

    void* data;
    msg.GetDataInSitu(data, size);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    this->m_clientResponse->FileData(static_cast<const uint8_t*>(data), size);
    return 2;
}

// VERIFY: e707dad6 manual
// OFFSET: 0x008CDE60
void Grunt::ClientLink::FileStatus(bool accept, uint64_t offset) {
    CDataStoreCache<1024> msg;

    if (accept) {
        if (offset) {
            msg.Put(static_cast<uint8_t>(0x33));
            msg.Put(offset);
        } else {
            msg.Put(static_cast<uint8_t>(0x32));
        }
    } else {
        msg.Put(static_cast<uint8_t>(0x34));
    }

    msg.Finalize();
    this->Send(msg);
}

// VERIFY: 37b6a6bc manual
// OFFSET: 0x008CDFC0
void Grunt::ClientLink::FileComplete() {
    CDataStoreCache<1024> msg;
    msg.Put(static_cast<uint8_t>(0x34));
    msg.Finalize();
    this->Send(msg);
}

// INLINED
const char* Grunt::ServerLink::GetLinkTypeName() const {
    return this->m_linkType <= 1 ? s_linkTypeNames[this->m_linkType] : "None";
}

// VERIFY: 24f16393 manual
// OFFSET: 0x008CBC10
int32_t Grunt::ServerLink::CmdGruntKick(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 9)) {
        return 0;
    }

    uint32_t accountId;
    msg.Get(accountId);
    uint64_t guid;
    msg.Get(guid);
    uint8_t reason;
    msg.Get(reason);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    DebugLog("received kick message for account id [%d:%I64d]", accountId, guid);
    this->m_response->Kick(accountId, guid, reason);

    return 2;
}

// VERIFY: 29da684a auto
// OFFSET: 0x008CBCC0
int32_t Grunt::ServerLink::CmdGruntPcWarning(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 16)) {
        return 0;
    }

    uint32_t accountId;
    msg.Get(accountId);
    uint64_t guid;
    msg.Get(guid);
    uint32_t warning;
    msg.Get(warning);

    return msg.m_read > msg.m_size ? 1 : 2;
}

// VERIFY: b8f17b92 manual
// OFFSET: 0x008CBD20
int32_t Grunt::ServerLink::CmdGruntAuthVerify(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 1)) {
        return 0;
    }

    uint8_t nameLength;
    msg.Get(nameLength);

    if (nameLength >= 32) {
        return 1;
    }

    if (!CanReadNumBytes(msg, nameLength + 36)) {
        return 0;
    }

    DebugLog("%s Auth Verify. Server name=%s", this->GetLinkTypeName(), this->m_serverName);

    uint8_t name[32];
    msg.GetArray(name, nameLength);
    name[nameLength] = 0;

    uint8_t salt[16];
    msg.GetArray(salt, sizeof(salt));

    uint8_t proof[20];
    msg.GetArray(proof, sizeof(proof));

    return msg.m_read > msg.m_size ? 1 : 2;
}

// VERIFY: 6aee0c06 auto
// OFFSET: 0x008CBE00
int32_t Grunt::ServerLink::CmdGruntConnPong(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 4)) {
        return 0;
    }

    uint32_t sequence;
    msg.Get(sequence);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    this->m_lastPongTime = OsGetAsyncTimeMs();

    return 2;
}

// VERIFY: 164b8020 auto
// OFFSET: 0x008CBE60
void Grunt::ServerLink::Send(CDataStore& msg) {
    this->m_lock.Enter();

    if (this->m_connection) {
        uint32_t size = msg.m_size;
        void* data;
        msg.GetDataInSitu(data, size);
        this->m_connection->Send(static_cast<uint8_t*>(data), size, false);
    }

    this->m_lock.Leave();
}

// VERIFY: abbc873f auto
// OFFSET: 0x008CBEB0
void Grunt::ServerLink::SendConnected(CDataStore& msg) {
    this->m_lock.Enter();

    if (this->m_connection && this->m_state == 3) {
        uint32_t size = msg.m_size;
        void* data;
        msg.GetDataInSitu(data, size);
        this->m_connection->Send(static_cast<uint8_t*>(data), size, false);
    }

    this->m_lock.Leave();
}

// VERIFY: f8a6c108 manual
// OFFSET: 0x008CBF10
int32_t Grunt::ServerLink::CmdGruntSunkenUpdate(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 8)) {
        return 0;
    }

    uint32_t accountId;
    msg.Get(accountId);
    uint32_t sunkenId;
    msg.Get(sunkenId);
    uint8_t online;
    msg.Get(online);

    this->m_response->SunkenUpdate(accountId, sunkenId, online != 0);
    DebugLog(">>>>BNet: Received SunkenUpdate for WoW Account ID: %d, Sunken ID: %d,  online: %s", accountId, sunkenId, online ? "true" : "false");

    return 2;
}

// VERIFY: b22aebf4 manual
// OFFSET: 0x008CBFB0
int32_t Grunt::ServerLink::CmdGruntSunkenOnline(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 5)) {
        return 0;
    }

    uint32_t sunkenId;
    msg.Get(sunkenId);
    uint8_t online;
    msg.Get(online);

    this->m_response->SunkenOnline(sunkenId, online != 0);
    DebugLog(">>>>BNet: received SunkenOnline: Sunken ID : %08x, online: %s", sunkenId, online ? "true" : "false");

    return 2;
}

// VERIFY: d262d027 manual
// OFFSET: 0x008CC040
void Grunt::ServerLink::SendStrings(uint32_t id, uint32_t locale) {
    CDataStore strings;
    this->m_response->GetStrings(id, locale, &strings);
    strings.m_read = 0;

    for (int32_t offset = 0; offset < static_cast<int32_t>(strings.m_size); ) {
        uint32_t chunk = strings.m_size - offset;

        if (chunk > 8000) {
            chunk = 8000;
        }

        CDataStore msg;
        msg.Put(static_cast<uint8_t>(0x41));
        msg.Put(id);
        msg.Put(locale);
        msg.Put(static_cast<uint32_t>(offset));
        msg.Put(strings.m_size);
        msg.Put(chunk);

        void* data;
        strings.GetDataInSitu(data, chunk);
        msg.PutData(data, chunk);
        msg.m_read = 0;
        this->SendConnected(msg);

        offset += chunk;
    }

    CDataStore msg;
    msg.Put(static_cast<uint8_t>(0x41));
    msg.Put(id);
    msg.Put(locale);
    msg.Put(static_cast<uint32_t>(-1));
    msg.Put(strings.m_size);
    msg.Put(static_cast<uint32_t>(0));
    msg.m_read = 0;
    this->SendConnected(msg);
}

// VERIFY: 4be2a9ee manual
// OFFSET: 0x008CD320
int32_t Grunt::ServerLink::CmdGruntAuthChallenge(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 16)) {
        return 0;
    }

    uint8_t challenge[16];
    msg.GetArray(challenge, sizeof(challenge));

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    uint32_t nameLength = SStrLen(this->m_serverName);
    DebugLog("%s Auth Challenge. Server name=%s", this->GetLinkTypeName(), this->m_serverName);

    uint8_t salt[16];
    OsSecureRandom(salt, sizeof(salt));

    SHA1_CONTEXT context;
    SHA1_Init(&context);
    SHA1_Update(&context, salt, sizeof(salt));
    SHA1_Update(&context, challenge, sizeof(challenge));
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(this->m_serverName), nameLength);
    SHA1_Update(&context, s_serverAuthKey, sizeof(s_serverAuthKey));

    uint8_t digest[20];
    SHA1_Final(digest, &context);

    CDataStoreCache<1024> reply;
    reply.Put(static_cast<uint8_t>(1));
    reply.Put(static_cast<uint8_t>(nameLength));
    reply.PutData(this->m_serverName, nameLength);
    reply.Put(static_cast<uint8_t>(0));
    reply.PutData(salt, sizeof(salt));
    reply.PutData(digest, sizeof(digest));
    reply.m_read = 0;
    this->Send(reply);

    return 2;
}

// VERIFY: 060a37cc auto
// OFFSET: 0x008CD4C0
int32_t Grunt::ServerLink::CmdGruntConnPing(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 4)) {
        return 0;
    }

    uint32_t sequence;
    msg.Get(sequence);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    CDataStoreCache<1024> reply;
    reply.Put(static_cast<uint8_t>(0x11));
    reply.Put(sequence);
    reply.m_read = 0;
    this->SendConnected(reply);

    return 2;
}

// VERIFY: 351f5624 auto
// OFFSET: 0x008CD570
int32_t Grunt::ServerLink::CmdGruntStrings(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 4)) {
        return 0;
    }

    uint32_t id;
    msg.Get(id);
    uint32_t locale;
    msg.Get(locale);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    if (locale) {
        this->SendStrings(id, locale);
        return 2;
    }

    for (uint32_t i = 0; i < sizeof(s_localeIds) / sizeof(s_localeIds[0]); i++) {
        this->SendStrings(id, s_localeIds[i]);
    }

    return 2;
}

// VERIFY: 63c32cc9 manual
// OFFSET: 0x008CD610
void Grunt::ServerLink::SendFeatures() {
    CDataStoreCache<8192> msg;
    msg.Put(static_cast<uint8_t>(0x45));
    Grunt::ServerLink::PutFeatures(msg);
    msg.m_read = 0;
    this->SendConnected(msg);
}

// VERIFY: c9b8bda2 manual
// OFFSET: 0x008CE200
int32_t Grunt::ServerLink::CmdGruntHello(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 4)) {
        return 0;
    }

    uint32_t value;
    msg.Get(value);

    if (msg.m_read > msg.m_size) {
        return 1;
    }

    this->unused_0094 = value;

    CDataStoreCache<1024> reply;
    reply.Put(static_cast<uint8_t>(0x20));

    if (this->m_linkType != 0 && this->m_linkType != 1) {
        return 1;
    }

    const char* typeName;
    uint8_t revision = 0;

    switch (this->m_linkType) {
        case 0:
            typeName = "Grunt";
            revision = 0x20;
            break;

        case 1:
            typeName = "Battle.net";
            revision = 0x21;
            break;

        default:
            typeName = "<unknown>";
            break;
    }

    reply.Put(revision);
    uint32_t sizePos = reply.m_size;
    reply.Put(static_cast<uint16_t>(0));

    Grunt::ServerHelloInfo info;
    memset(&info, 0, sizeof(info));
    info.m_linkType = this->m_linkType;

    if (!this->m_response->GetHelloInfo(&info)) {
        DebugLog("Unable to get the information necessary to identify self to %s", typeName);
        return 1;
    }

    uint8_t nameLength = static_cast<uint8_t>(SStrLen(info.m_realmName));
    reply.Put(nameLength);
    reply.PutData(info.m_realmName, nameLength);

    uint32_t address = info.m_address;

    if (!address) {
        address = this->m_publicAddress;
    }

    reply.Put(address);
    reply.Put(info.m_port);
    reply.Put(info.m_realmType);
    reply.Put(info.unk_0004);
    reply.Put(info.m_versionMajor);
    reply.Put(info.m_versionMinor);
    reply.Put(info.m_versionPatch);
    reply.Put(info.m_build);
    reply.Put(info.unk_0018);
    reply.Put(info.unk_001C);
    reply.Put(info.unk_0020);
    reply.Put(info.m_realmId);
    reply.Put(info.unk_0028);
    reply.Put(info.unk_004C);
    reply.Put(this->unk_0098);
    reply.Set(sizePos, static_cast<uint16_t>(reply.m_size - sizePos - 2));

    NETADDR addr;
    char addrText[64];
    OsNetAddrMake(this->m_serverAddress, 0, &addr);
    DebugLog("Sending HELLO to %s login server at %s", typeName, OsNetAddrToStr(&addr, addrText, sizeof(addrText)));
    OsNetAddrMake(address, info.m_port, &addr);
    DebugLog("My public address is %s", OsNetAddrToStr(&addr, addrText, sizeof(addrText)));
    DebugLog("My realm name is %s", info.m_realmName);
    DebugLog("My realm ID is %d", info.m_realmId);
    DebugLog("My version is %d.%d.%d.%d", info.m_versionMajor, info.m_versionMinor, info.m_versionPatch, info.m_build);
    DebugLog("My realm type is %d", info.m_realmType);
    DebugLog("My server revision is %d", revision);

    reply.m_read = 0;
    this->Send(reply);

    if (this->m_linkType == 1) {
        this->m_response->HelloComplete();
    }

    this->m_state = 3;

    if (this->m_linkType == 1) {
        this->SendFeatures();
    }

    return 2;
}

// VERIFY: 49c5c356 manual
// OFFSET: 0x008CF100
int32_t Grunt::ServerLink::CmdGruntProveSession(CDataStore& msg) {
    if (!CanReadNumBytes(msg, 5)) {
        return 0;
    }

    uint32_t requestId;
    msg.Get(requestId);
    uint8_t result;
    msg.Get(result);

    Grunt::ServerSessionInfo info;
    memset(&info, 0, sizeof(info));

    if (!result) {
        msg.Get(info.unk_0000);
        msg.Get(info.m_accountId);
        msg.Get(info.unk_0008);
        msg.Get(info.unk_0010);
        msg.Get(info.unk_0018);
        msg.Get(info.unk_0078);
        msg.Get(info.unk_001C);
        msg.Get(info.m_timeOptions);

        uint64_t accountFlags;
        msg.Get(accountFlags);
        info.m_accountFlags = static_cast<uint32_t>(accountFlags);

        msg.GetArray(info.unk_0028, sizeof(info.unk_0028));
        msg.Get(info.unk_0050);
        msg.Get(info.unk_0054);
        msg.Get(info.m_os);

        uint8_t versionLength = 0;
        msg.Get(versionLength);
        char* version = static_cast<char*>(alloca(versionLength + 1));
        memset(version, 0, versionLength + 1);
        msg.GetArray(reinterpret_cast<uint8_t*>(version), versionLength);
        sscanf(version, "%d.%d.%d.%d", reinterpret_cast<int32_t*>(&info.m_versionMajor), reinterpret_cast<int32_t*>(&info.m_versionMinor), reinterpret_cast<int32_t*>(&info.m_versionPatch), reinterpret_cast<int32_t*>(&info.m_build));

        msg.Get(info.m_timePlayedMinutes);
        msg.Get(info.unk_0064);
        msg.Get(info.m_expansionLevel);
        msg.Get(reinterpret_cast<uint32_t&>(info.unk_0068));

        info.m_accountFlags &= 0x6A8E040F;

        if (info.unk_0068 > 0) {
            info.m_accountFlags |= 0x10;
        }

        msg.Get(info.unk_006C);
        msg.Get(info.m_referrerAccountId);
        msg.Get(reinterpret_cast<uint32_t&>(info.m_referralTime));

        if (this->m_linkType == 1) {
            msg.Get(info.unk_007C);
        }

        msg.Get(info.m_bnetLicenseCount);

        if (info.m_bnetLicenseCount >= 256) {
            return 1;
        }

        for (uint32_t i = 0; i < info.m_bnetLicenseCount; i++) {
            Grunt::ServerLicense& license = info.m_bnetLicenses[i];
            msg.Get(license.m_id);
            msg.Get(license.m_hasExpiration);

            if (license.m_hasExpiration) {
                msg.Get(license.m_expiration);
            } else {
                license.m_expiration = 0;
            }
        }

        if (info.m_wowLicenseCount >= 256) {
            return 1;
        }

        msg.Get(info.m_wowLicenseCount);

        for (uint32_t i = 0; i < info.m_wowLicenseCount; i++) {
            Grunt::ServerLicense& license = info.m_wowLicenses[i];
            msg.Get(license.m_id);
            msg.Get(license.m_hasExpiration);

            if (license.m_hasExpiration) {
                msg.Get(license.m_expiration);
            } else {
                license.m_expiration = 0;
            }
        }
    }

    if (msg.m_read > msg.m_size) {
        return 0;
    }

    bool notMac = info.m_os != 'MacP' && info.m_os != 'PcaM';

    Grunt::ServerLink::Request* request = static_cast<Grunt::ServerLink::Request*>(this->Remove(requestId));

    if (!request) {
        return 2;
    }

    if (!result) {
        if (info.m_referrerAccountId) {
            DebugLog("(Refer-A-Friend) Account [%d] was referred by account [%d]. Referral expires in %.02f minutes.", info.m_accountId, info.m_referrerAccountId, static_cast<float>(info.m_referralTime) * (1.0f / 60.0f));
        }

        DebugLog("account id [%d] has passed authentication from %s, accountflags=0x%lx, timeOptions=0x%lx, timePlayedMinutes=%d, expansionLevel=%d", info.m_accountId, this->GetLinkTypeName(), info.m_accountFlags, info.m_timeOptions, info.m_timePlayedMinutes, info.m_expansionLevel);

        if (info.m_bnetLicenseCount > 0 || info.m_wowLicenseCount > 0) {
            DebugLog("Account [%d] has numBnetAccountLicenses=%d, numWowAccountLicenses=%d", info.m_accountId, info.m_bnetLicenseCount, info.m_wowLicenseCount);
        }

        this->m_response->ProveSessionResult(request->m_context, 0, &info, this->m_linkType, notMac);
    } else {
        if (result >= 0x21) {
            DebugLog("Unknown result %d from %s", result, this->GetLinkTypeName());
            result = 0xFF;
        }

        this->m_response->ProveSessionResult(request->m_context, result, nullptr, this->m_linkType, notMac);
    }

    delete request;

    return 2;
}

// VERIFY: cf6cc05b manual
// OFFSET: 0x008CF940
void Grunt::ServerLink::PutFeatures(CDataStore& msg) {
    msg.Put(static_cast<uint16_t>(sizeof(s_serverFeatures) / sizeof(s_serverFeatures[0])));

    for (uint32_t i = 0; i < sizeof(s_serverFeatures) / sizeof(s_serverFeatures[0]); i++) {
        msg.Put(s_serverFeatures[i].m_id);
        msg.Put(static_cast<uint8_t>(s_serverFeatures[i].m_version));
        msg.Put(static_cast<uint8_t>(0));
        msg.Put(static_cast<uint8_t>(0));
    }

    uint32_t countPos = msg.m_size;
    msg.Put(static_cast<uint16_t>(0));
    msg.Put(static_cast<uint32_t>('Relm'));
    msg.Put(static_cast<uint32_t>('Race'));
    msg.Put(static_cast<uint32_t>('Clss'));
    msg.Put(static_cast<uint32_t>('Area'));
    msg.Set(countPos, static_cast<uint16_t>(4));
}
