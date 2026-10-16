#include "WowConnection.h"
#include <cstdlib>
#include <cstring>
#include <new>
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/WowServices/HMAC.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowServices/WowConnectionNet.h"

// STATIC: 0x00AB96F8
WowConnection::IDBITS WowConnection::s_idBits;

// STATIC: 0x00B38D00
static uint32_t s_unused_B38D00;

// STATIC: 0x00B38D04
static int32_t s_sendNodeCount;

// STATIC: 0x00B38D08
static uint64_t s_sendNodeBytes;

// STATIC: 0x00B38D10
uint64_t WowConnection::s_sendBytes;

// STATIC: 0x00B38D18
bool (*WowConnection::s_verifyAddr)(const NETADDR* addr);

// STATIC: 0x00B38D1C
WowConnectionNet* WowConnection::s_network;

// STATIC: 0x00B38D20
int32_t WowConnection::s_lagTestDelayMin;

// STATIC: 0x00B38D24
int32_t WowConnection::s_lagTestDelayMax;

// STATIC: 0x00B38D28
static volatile LONG s_numConnections;

// STATIC: 0x00B38D34
SCritSect WowConnection::s_idLock;

// STATIC: 0x00B38D60
static uint8_t s_arc4drop1024[1024];

// STATIC: 0x00B39160
static uint8_t s_arc4drop1024Key[1024];

// STATIC: 0x009E8A9C
static const uint8_t s_defaultSeed[32] = { 0xCC, 0x98, 0xAE, 0x04, 0xE8, 0x97, 0xEA, 0xCA, 0x12, 0xDD, 0xC0, 0x93, 0x42, 0x91, 0x53, 0x57, 0xC2, 0xB3, 0x72, 0x3C, 0xC6, 0xAE, 0xD9, 0xB5, 0x34, 0x3C, 0x53, 0xEE, 0x2F, 0x43, 0x67, 0xCE };

// INLINED
void WowConnection::IDBITS::SetCount(uint32_t count) {
    this->m_count = count;
    this->m_bits.SetCount(count ? ((count - 1) >> 5) + 1 : 0);

    if (this->m_count) {
        memset(this->m_bits.m_data, 0, sizeof(uint32_t) * this->m_bits.m_count);
    }
}

// VERIFY: 2fa3073a manual
// OFFSET: 0x00466BF0
void WowConnection::SetEncryptionKey(const uint8_t* key, uint8_t keyLength, uint8_t direction, const uint8_t* seed, uint8_t seedLength) {
    if (!seed) {
        seed = s_defaultSeed;
        seedLength = sizeof(s_defaultSeed);
    }

    uint32_t halfLength = seedLength >> 1;
    const uint8_t* seeds[2] = { seed, seed + halfLength };

    HMAC<SHA1> hmac;
    SHA1::Digest digest;

    hmac.Init(seeds[direction], halfLength);
    hmac.Process(key, keyLength);
    hmac.Finish(digest);
    memcpy(this->m_sendKeyInit, &digest, sizeof(digest));

    hmac.Init(seeds[direction ^ 1], halfLength);
    hmac.Process(key, keyLength);
    hmac.Finish(digest);
    memcpy(this->m_receiveKeyInit, &digest, sizeof(digest));

    SARC4PrepareKey(this->m_sendKeyInit, sizeof(this->m_sendKeyInit), &this->m_sendKey);
    SARC4PrepareKey(this->m_receiveKeyInit, sizeof(this->m_receiveKeyInit), &this->m_receiveKey);
    SARC4ProcessBuffer(s_arc4drop1024Key, sizeof(s_arc4drop1024Key), &this->m_sendKey, &this->m_sendKey);
    SARC4ProcessBuffer(s_arc4drop1024Key, sizeof(s_arc4drop1024Key), &this->m_receiveKey, &this->m_receiveKey);
}

// VERIFY: 9c7e0bb3 manual
// OFFSET: 0x00466DB0
void WowConnection::IDBITS::Or(const WowConnection::IDBITS& source) {
    for (int32_t i = 0; i < static_cast<int32_t>(source.m_bits.m_count); i++) {
        this->m_bits.m_data[i] |= source.m_bits.m_data[i];
    }
}

// VERIFY: 6532a7f0 auto
// OFFSET: 0x006B1B80
void WowConnectionResponse::WCDataReady(WowConnection* connection, uint32_t timeStamp, uint8_t* data, int32_t length) {
}

void WowConnectionResponse::WCGlobalLock() {
}

void WowConnectionResponse::WCGlobalUnlock() {
}

void WowConnectionResponse::WCWriteReady(WowConnection* connection) {
}

void WowConnectionResponse::NotifyAboutToDoReads() {
}

void WowConnectionResponse::NotifySendDepthExceeded(WowConnection* connection) {
}

// VERIFY: 4a12b654 manual
// OFFSET: 0x00632270
WowConnectionResponse::~WowConnectionResponse() {
}

// VERIFY: c3a40df5 auto
// OFFSET: 0x004665B0
void WowConnection::Encrypt(uint8_t* data, uint32_t length) {
    SARC4ProcessBuffer(data, length, &this->m_sendKey, &this->m_sendKey);
}

// VERIFY: 5fe5638b manual
// OFFSET: 0x004665D0
void WowConnection::SetSendTime(WowConnection::SENDNODE* node) {
    int32_t delay = WowConnection::s_lagTestDelayMin;

    if (!delay) {
        return;
    }

    if (WowConnection::s_lagTestDelayMax > delay) {
        delay = static_cast<int32_t>(delay + static_cast<double>((WowConnection::s_lagTestDelayMax - delay + 1) * rand()) / 32768.0);
    }

    node->m_sendTime = delay + OsGetAsyncTimeMs();
}

// VERIFY: f06b0330 auto
// OFFSET: 0x00466650
void WowConnection::FreeSendNode(WowConnection::SENDNODE* node) {
    WDataStore::FreeBuffer(node, node->m_datasize + sizeof(WowConnection::SENDNODE) + 3);
}

// VERIFY: 3d72b789 manual
// OFFSET: 0x004666D0
void WowConnection::SetResponse(WowConnectionResponse* response, bool deferred) {
    while (true) {
        this->m_responseLock.Enter();

        if (!this->m_responseRef || this->m_responseRefThread == SGetCurrentThreadId()) {
            break;
        }

        if (deferred) {
            this->m_pendingResponse = response;
            this->m_responseLock.Leave();
            return;
        }

        this->m_responseLock.Leave();
        OsSleep(50);
    }

    this->m_response = response;
    this->m_pendingResponse = nullptr;
    this->m_responseLock.Leave();
}

// VERIFY: 6119f9fe auto
// OFFSET: 0x00466750
void WowConnection::AcquireResponseRef() {
    this->m_responseLock.Enter();
    this->m_responseRef++;
    this->m_responseRefThread = SGetCurrentThreadId();
    this->m_responseLock.Leave();
}

// VERIFY: 31a71f37 auto
// OFFSET: 0x00466780
void WowConnection::ReleaseResponseRef() {
    this->m_responseLock.Enter();

    WowConnectionResponse* pendingResponse = this->m_pendingResponse;
    this->m_responseRef--;

    if (pendingResponse) {
        this->m_response = pendingResponse;
        this->m_pendingResponse = nullptr;
    }

    this->m_responseLock.Leave();
}

// VERIFY: 4ccaefb4 auto
// OFFSET: 0x004667C0
void WowConnection::SetState(WOW_CONN_STATE state) {
    WOW_CONN_STATE oldState = this->m_connState;
    this->m_connState = state;

    if (WowConnection::s_network) {
        WowConnection::s_network->PlatformChangeState(this, oldState);
    }
}

// VERIFY: 85b2e20b auto
// OFFSET: 0x004667F0
void WowConnection::SetType(WOWC_TYPE type) {
    this->m_lock.Enter();
    this->m_type = type;
    this->m_lock.Leave();
}

// VERIFY: b8b5ccdd auto
// OFFSET: 0x00466820
void WowConnection::SetEncryption(bool enabled) {
    this->m_lock.Enter();
    this->m_encrypt = enabled;
    SARC4PrepareKey(this->m_sendKeyInit, sizeof(this->m_sendKeyInit), &this->m_sendKey);
    SARC4PrepareKey(this->m_receiveKeyInit, sizeof(this->m_receiveKeyInit), &this->m_receiveKey);
    SARC4ProcessBuffer(s_arc4drop1024, sizeof(s_arc4drop1024), &this->m_sendKey, &this->m_sendKey);
    SARC4ProcessBuffer(s_arc4drop1024, sizeof(s_arc4drop1024), &this->m_receiveKey, &this->m_receiveKey);
    this->m_lock.Leave();
}

// VERIFY: 9e1a898f manual
// OFFSET: 0x004669D0
void WowConnection::Init(WowConnectionResponse* response, void (*threadInit)()) {
    InterlockedExchangeAdd(&s_numConnections, 1);

    this->m_refCount.m_value = 1;
    this->m_responseRef = 0;
    this->m_pendingResponse = nullptr;
    this->m_maxSendDepth = 100000;
    this->m_sendDepth = 0;
    this->m_sendBytes = 0;
    this->m_oldsock = -1;
    this->m_notifyWriteReady = 0;
    this->unused_0144 = -1;

    WOW_CONN_STATE oldState = this->m_connState;
    this->m_connState = WOWC_UNINITIALIZED;

    if (WowConnection::s_network) {
        WowConnection::s_network->PlatformChangeState(this, oldState);
    }

    this->m_response = response;
    this->unused_000C = 0;
    this->m_threadInit = threadInit;
    this->unused_0018 = 0;
    this->unused_0050 = 0;
    this->unused_0054 = 0;
    this->unused_0058 = 0;
    this->unused_005C = 0;
    this->unused_0060 = 0;
    this->unused_0064 = 0;
    this->unused_0078 = 0;
    this->m_connectAddress = 0;
    this->m_connectPort = 0;
    this->m_connectRetryInterval = -1;
    memset(&this->m_peer, 0, sizeof(this->m_peer));
    this->unused_00A0 = 0;
    this->m_serviceFlags = 0;
    this->m_serviceCount = 0;
    this->unused_0134 = OsGetAsyncTimeMs();
    this->m_readBuffer = nullptr;
    this->m_readBytes = 0;
    this->m_readBufferSize = 0;
    this->m_event = nullptr;
    this->m_encrypt = false;
    this->m_uint375 = 0;
    this->m_uint376 = 0;
    memset(this->m_sendKeyInit, 0, sizeof(this->m_sendKeyInit));
    memset(this->m_receiveKeyInit, 0, sizeof(this->m_receiveKeyInit));

    oldState = this->m_connState;
    this->m_connState = WOWC_INITIALIZED;

    if (WowConnection::s_network) {
        WowConnection::s_network->PlatformChangeState(this, oldState);
    }

    this->m_type = WOWC_TYPE_MESSAGES;
}

// VERIFY: f1a773bb auto
// OFFSET: 0x00466B50
void WowConnection::Disconnect() {
    this->m_lock.Enter();

    if (this->m_sock >= 0 && this->m_connState == WOWC_CONNECTED) {
        this->SetState(WOWC_DISCONNECTING);
    }

    this->m_lock.Leave();
}

// VERIFY: baaffdbb auto
// OFFSET: 0x00466E30
void WowConnection::FreeSendList() {
    WowConnection::SENDNODE* node = this->m_sendList.Head();

    while (!TSList<WowConnection::SENDNODE>::IsEnd(node)) {
        WowConnection::SENDNODE* next = this->m_sendList.RawNext(node);
        this->m_sendList.UnlinkNode(node);
        WDataStore::FreeBuffer(node, node->m_datasize + sizeof(WowConnection::SENDNODE) + 3);
        node = next;
    }

    node = this->m_lagTestRecvList.Head();

    while (!TSList<WowConnection::SENDNODE>::IsEnd(node)) {
        WowConnection::SENDNODE* next = this->m_lagTestRecvList.RawNext(node);
        this->m_lagTestRecvList.UnlinkNode(node);
        WDataStore::FreeBuffer(node, node->m_datasize + sizeof(WowConnection::SENDNODE) + 3);
        node = next;
    }
}

// VERIFY: 99f1a990 auto
// OFFSET: 0x00467130
WowConnection::SENDNODE::SENDNODE(const WowConnection::SENDNODE& source, uint8_t* buffer) {
    this->m_data = buffer;
    this->m_size = source.m_size;
    this->m_offset = source.m_offset;
    this->m_datasize = source.m_datasize;
    this->m_sendTime = source.m_sendTime;
    this->m_allocsize = source.m_allocsize;
    memcpy(buffer, source.m_data, source.m_size);
    memcpy(this->m_header, source.m_header, sizeof(this->m_header));
}

// VERIFY: e2045bb0 manual
// OFFSET: 0x00467190
WowConnection::SENDNODE::SENDNODE(const void* data, int32_t size, uint8_t* buffer, bool raw) {
    if (data) {
        this->m_data = buffer;
    }

    if (raw) {
        memcpy(this->m_data, data, size);
        this->m_size = size;
    } else {
        int32_t headerSize = (size > 0x7FFF) + 2;

        if (!data) {
            this->m_data = buffer - headerSize;
        }

        uint8_t* header = this->m_data;

        if (size > 0x7FFF) {
            header[0] = (size >> 16) | 0x80;
            header[1] = size >> 8;
            header[2] = size;
        } else {
            header[0] = size >> 8;
            header[1] = size;
        }

        if (data) {
            memcpy(this->m_data + headerSize, data, size);
        }

        this->m_size = size + headerSize;
    }

    this->m_datasize = size;
    this->m_offset = 0;
    this->m_allocsize = 0;
    memcpy(this->m_header, this->m_data, this->m_size > sizeof(this->m_header) ? sizeof(this->m_header) : this->m_size);
}

// VERIFY: da47c164 manual
// OFFSET: 0x00467330
WowConnection::WowConnection(WowConnectionResponse* response, void (*threadInit)()) {
    this->Init(response, threadInit);
    this->m_sock = -1;
}

// VERIFY: e9974d81 manual
// OFFSET: 0x004673C0
WowConnection::~WowConnection() {
    InterlockedExchangeAdd(&s_numConnections, -1);

    if (WowConnection::s_network) {
        WowConnection::s_network->Remove(this);
        WowConnection::s_network->PlatformDestruct(this);
    }

    if (this->m_sock >= 0) {
        this->CloseSocket(this->m_sock);
        this->m_sock = -1;
    }

    if (this->m_readBuffer) {
        SMemFree(this->m_readBuffer, __FILE__, __LINE__, 0);
    }

    this->FreeSendList();

    memset(this->m_receiveKey.m_state, 0, sizeof(this->m_receiveKey.m_state));
    this->m_receiveKey.m_y = 0;
    this->m_receiveKey.m_x = 0;
    memset(this->m_sendKey.m_state, 0, sizeof(this->m_sendKey.m_state));
    this->m_sendKey.m_y = 0;
    this->m_sendKey.m_x = 0;
}

// VERIFY: 8a04af59 manual
// OFFSET: 0x00467540
WowConnection::SENDNODE* WowConnection::NewSendNodePacket(const void* data, int32_t size, bool raw) {
    s_sendNodeCount++;
    s_sendNodeBytes += size;

    uint32_t allocSize = size + sizeof(WowConnection::SENDNODE) + 3;
    void* buffer = WDataStore::AllocBuffer(allocSize);
    WowConnection::SENDNODE* node = new (buffer) WowConnection::SENDNODE(data, size, static_cast<uint8_t*>(buffer) + sizeof(WowConnection::SENDNODE), raw);
    node->m_allocsize = allocSize;
    this->SetSendTime(node);

    return node;
}

// VERIFY: 1b9183ae manual
// OFFSET: 0x004675A0
WowConnection::SENDNODE* WowConnection::NewSendNodeCopy(const WowConnection::SENDNODE* source, int32_t size) {
    s_sendNodeCount++;
    s_sendNodeBytes += size;

    void* buffer = WDataStore::AllocBuffer(size + sizeof(WowConnection::SENDNODE) + 3);

    if (!buffer) {
        return nullptr;
    }

    return new (buffer) WowConnection::SENDNODE(*source, static_cast<uint8_t*>(buffer) + sizeof(WowConnection::SENDNODE));
}

// VERIFY: 925f6659 manual
// OFFSET: 0x004680D0
void WowConnection::MarkSocketId(int32_t sock) {
    if (sock < 0) {
        return;
    }

    WowConnection::s_idLock.Enter();

    if (sock >= static_cast<int32_t>(WowConnection::s_idBits.m_count)) {
        int32_t count = WowConnection::s_idBits.m_count;

        do {
            count *= 2;
        } while (sock >= count);

        WowConnection::IDBITS bits;
        bits.SetCount(WowConnection::s_idBits.m_count);
        bits.Or(WowConnection::s_idBits);
        WowConnection::s_idBits.SetCount(count);
        WowConnection::s_idBits.Or(bits);
    }

    WowConnection::s_idBits.m_bits.m_data[sock >> 5] |= 1 << (sock & 0x1F);

    WowConnection::s_idLock.Leave();
}

// VERIFY: a9157009 auto
// OFFSET: 0x00468200
int32_t WowConnection::Release() {
    int32_t refCount = InterlockedDecrement(reinterpret_cast<volatile LONG*>(&this->m_refCount.m_value));

    if (refCount <= 0) {
        if (WowConnection::s_network) {
            WowConnection::s_network->Delete(this);
        } else {
            delete this;
        }
    }

    return refCount;
}

// VERIFY: 5d8f334b auto
// OFFSET: 0x00468250
void WowConnection::DoDisconnect() {
    this->m_lock.Enter();

    if (this->m_sock >= 0) {
        WowConnection::s_network->Remove(this);
        this->CloseSocket(this->m_sock);
    }

    this->SetState(WOWC_DISCONNECTED);
    this->AddRef();
    this->AcquireResponseRef();

    WowConnectionResponse* response = this->m_response;
    int32_t sock = this->m_sock;

    this->m_lock.Leave();

    if (response && sock >= 0) {
        response->WCDisconnected(this, OsGetAsyncTimeMs(), &this->m_peer);
    }

    this->m_lock.Enter();
    this->m_sock = -1;
    this->ReleaseResponseRef();
    this->m_lock.Leave();

    this->Release();
}

// VERIFY: f834433c manual
// OFFSET: 0x00468A90
void WowConnection::DoReads() {
    this->AddRef();
    this->m_lock.Enter();

    if (this->m_connState == WOWC_LISTENING) {
        this->CheckAccept();
    } else if (this->m_connState == WOWC_CONNECTED) {
        switch (this->m_type) {
            case WOWC_TYPE_MESSAGES:
                this->DoMessageReads();
                break;

            case WOWC_TYPE_STREAM:
                this->DoStreamReads();
                break;
        }
    }

    this->m_lock.Leave();
    this->Release();
}

// VERIFY: 18fc5293 manual
// OFFSET: 0x00468B30
void WowConnection::DoExceptions() {
    this->AddRef();
    this->m_lock.Enter();

    if (this->m_connState == WOWC_CONNECTING) {
        this->CheckConnect();
    }

    this->m_lock.Leave();
    this->Release();
}

// VERIFY: 2e088c06 auto
// OFFSET: 0x00468D00
bool WowConnection::Connect(uint32_t address, uint16_t port, int32_t timeout) {
    this->m_connectAddress = address;
    this->m_connectPort = port;
    this->StartConnect();

    return true;
}

// VERIFY: 30cc0530 manual
// OFFSET: 0x00466B90
void WowConnection::DestroyOsNet() {
    s_unused_B38D00 = 1;

    if (!WowConnection::s_network) {
        return;
    }

    WowConnection::s_network->Stop();
    WowConnection::s_network->PlatformDestroy();
    delete WowConnection::s_network;
    WowConnection::s_network = nullptr;
    WDataStore::StaticDestroy();
}

// VERIFY: a65da1b0 manual
// OFFSET: 0x00468D90
int32_t WowConnection::InitOsNet(bool (*netAddrFilter)(const NETADDR* addr), void (*threadInit)(), int32_t numThreads, bool useEngine) {
    if (!WowConnection::s_network) {
        WowConnection::s_idBits.SetCount(256);
        WDataStore::StaticInitialize();
        WowConnection::s_verifyAddr = netAddrFilter;
        s_unused_B38D00 = 0;

        if (static_cast<uint32_t>(numThreads) > 32) {
            numThreads = 32;
        }

        WowConnection::s_network = new (SMemAlloc(sizeof(WowConnectionNet), __FILE__, __LINE__, 0)) WowConnectionNet(numThreads, threadInit);
        WowConnection::s_network->PlatformInit(useEngine);
        WowConnection::s_network->Start();
    }

    return 1;
}

// VERIFY: f15c6280 auto
// OFFSET: 0x00468E50
bool WowConnection::Connect(const char* address, int32_t timeout) {
    char host[256];
    const char* separator = SStrChr(address, ':');

    if (separator) {
        this->m_connectPort = SStrToInt(separator + 1);

        uint32_t length = separator - address + 1;

        if (length >= sizeof(host)) {
            length = sizeof(host);
        }

        SStrCopy(host, address, length);
    } else {
        this->m_connectPort = 0;
        SStrCopy(host, address, sizeof(host));
    }

    this->Connect(host, this->m_connectPort, timeout);

    return true;
}

// INLINED
void WowConnection::AddRef() {
    InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&this->m_refCount.m_value), 1);
}

// VERIFY: 050b636a manual
// OFFSET: 0x00468EE0
void ARC4PrepareKey(const uint8_t* key, uint32_t length, SARC4Key* state) {
    for (int32_t i = 0; i < 256; i++) {
        state->m_state[i] = i;
    }

    state->m_x = 0;
    state->m_y = 0;

    uint32_t keyIndex = 0;
    uint8_t j = 0;

    for (int32_t i = 0; i < 256; i++) {
        j += state->m_state[i] + key[keyIndex];

        uint8_t swap = state->m_state[i];
        state->m_state[i] = state->m_state[j];
        state->m_state[j] = swap;

        keyIndex = (keyIndex + 1) % length;
    }
}
