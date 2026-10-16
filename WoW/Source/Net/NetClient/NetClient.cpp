#include "NetClient.h"
#include <cstdlib>
#include <cstring>
#include <new>
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/WowServices/HMAC.h"

// STATIC: 0x00AD2ED0
static TSList<NETCLIENTNODE> s_clientList;

// STATIC: 0x00C5D624
static HPROPCONTEXT s_propContext;

// STATIC: 0x00C5D628
static uint32_t s_stats[6];

// STATIC: 0x00C5D640
static uint32_t s_initializeTime;

// STATIC: 0x00C5D644
static uint32_t s_idleStats[4];

// STATIC: 0x00C5D654
uint32_t NetClient::s_clientCount;

// VERIFY: 44b868cd manual
// OFFSET: 0x006321B0
static bool __stdcall ComputeProofOfWork(const SHA1_CONTEXT* context, uint32_t difficulty, uint64_t* proof) {
    uint64_t nonce = 0;

    while (true) {
        SHA1_CONTEXT attempt;
        memcpy(&attempt, context, sizeof(attempt));
        SHA1_Update(&attempt, reinterpret_cast<const uint8_t*>(&nonce), sizeof(nonce));

        uint8_t digest[20];
        SHA1_Final(digest, &attempt);

        uint8_t bits = 0;
        int32_t i = 0;

        while (!digest[i]) {
            i++;
            bits += 8;

            if (i >= 20) {
                break;
            }
        }

        if (i < 20) {
            for (uint8_t value = digest[i]; !(value & 1); value >>= 1) {
                bits++;
            }
        }

        if (bits >= difficulty) {
            break;
        }

        if (++nonce == 0) {
            return false;
        }
    }

    *proof = nonce;

    return true;
}

// VERIFY: 0b11eb2d auto
// OFFSET: 0x00633330
void NetClient::WCMessageReady(WowConnection* connection, uint32_t timeStamp, CDataStore* msg) {
    uint32_t size = msg->m_size;
    void* data;
    msg->GetDataInSitu(data, size);
    InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&this->m_bytesReceived), msg->m_size);
    msg->m_read = 0;

    uint16_t msgId;
    msg->Get(msgId);

    switch (msgId) {
        case SMSG_CONNECT_TO:
            this->HandleRedirectClient(connection, msg);
            return;

        case SMSG_PONG:
            this->HandlePong(connection, msg);
            return;

        case SMSG_AUTH_CHALLENGE:
            this->HandleAuthChallenge(connection, msg);
            return;

        case SMSG_SUSPEND_COMMS:
            this->HandleSuspendComms(connection, msg);
            return;

        case SMSG_RESUME_COMMS:
            this->HandleResumeComms(connection, msg);
            return;

        default:
            break;
    }

    if (connection == this->m_serverConnection && !this->m_suspended) {
        msg->m_read = msg->m_size;
        this->m_netEventQueue->AddEvent(EVENT_ID_NET_DATA, connection, this, data, size);
    } else {
        connection->Disconnect();
    }
}

// VERIFY: 74c166be manual
// OFFSET: 0x00633440
NETEVENTQUEUE::NETEVENTQUEUE(NetClient* client) {
    this->m_client = client;
}

// VERIFY: eb4e833d manual
// OFFSET: 0x006334F0
void NETEVENTQUEUE::Poll() {
    this->m_critSect.Enter();

    bool destroyed = false;
    this->m_client->m_refCount.Increment();

    for (NETEVENTQUEUENODE* node = this->m_eventQueue.Head(); node; node = this->m_eventQueue.Next(node)) {
        NetClient* client = this->m_client;

        if (!client->m_deleteMe) {
            switch (node->m_eventId) {
                case EVENT_ID_NET_DATA:
                    client->HandleData(node->m_timeReceived, node->m_data, node->m_dataSize);
                    break;

                case EVENT_ID_NET_CONNECT:
                    client->HandleConnect();
                    break;

                case EVENT_ID_NET_DISCONNECT:
                    client->HandleDisconnect();
                    break;

                case EVENT_ID_NET_CANTCONNECT:
                    client->HandleCantConnect();
                    break;

                case EVENT_ID_NET_DESTROY:
                    client->m_deleteMe = true;
                    destroyed = true;
                    break;

                case EVENT_ID_NET_AUTH_CHALLENGE:
                    client->HandleAuthChallenge(static_cast<AuthenticationChallenge*>(node->m_data));
                    break;

                default:
                    break;
            }
        }

        client = this->m_client;

        if (client->m_refCount.Decrement() == 0 && client->m_deleteMe) {
            delete client;
        }
    }

    if (!destroyed) {
        this->m_client->HandleIdle();
    }

    NetClient* client = this->m_client;

    if (client->m_refCount.Decrement() == 0 && client->m_deleteMe) {
        delete client;
    }

    this->m_eventQueue.Clear();
    this->m_critSect.Leave();
}

// VERIFY: dd90b2c3 auto
// OFFSET: 0x00631D10
static void InitializePropContext() {
    if (PropGetSelectedContext() != s_propContext) {
        PropSelectContext(s_propContext);
    }
}

// VERIFY: 8093d1d7 manual
// OFFSET: 0x00631D30
int32_t NetClient::Initialize() {
    if (!NetClient::s_clientCount) {
        s_propContext = PropGetSelectedContext();

        int32_t result = WowConnection::InitOsNet(nullptr, InitializePropContext, 1, false);

        if (!result) {
            return result;
        }
    }

    NetClient::s_clientCount++;

    this->m_netEventQueue = new (SMemAlloc(sizeof(NETEVENTQUEUE), __FILE__, __LINE__, 0)) NETEVENTQUEUE(this);
    memset(this->m_handlers, 0, sizeof(this->m_handlers));
    memset(this->m_handlerParams, 0, sizeof(this->m_handlerParams));
    this->m_serverConnection = new (SMemAlloc(sizeof(WowConnection), __FILE__, __LINE__, 0)) WowConnection(this, nullptr);
    this->m_netState = NS_INITIALIZED;
    s_initializeTime = OsGetAsyncTimeMs();

    return 1;
}

// VERIFY: 22bd59ed auto
// OFFSET: 0x00631E10
int32_t NetClient::ConnectInternal(const char* host, uint16_t port) {
    if (this->m_netState != NS_INITIALIZED) {
        SErrDisplayAppFatal("Expected (m_netState == NS_INITIALIZED), got %d", this->m_netState);
    }

    this->m_netState = NS_CONNECTING;
    this->m_serverConnection->Connect(host, port, -1);

    if (this->m_redirectConnection) {
        this->m_redirectConnection->SetResponse(nullptr, false);
        this->m_redirectConnection->Disconnect();
        this->m_redirectConnection->Release();
        this->m_redirectConnection = nullptr;
    }

    return 1;
}

// VERIFY: 0454197e auto
// OFFSET: 0x00631F70
void NetClient::SetLoginData(LoginData* loginData) {
    memcpy(&this->m_loginData, loginData, sizeof(this->m_loginData));
}

// VERIFY: 10a620f0 auto
// OFFSET: 0x00631F90
uint8_t* NetClient::GetSessionKey() {
    return this->m_loginData.m_sessionKey;
}

// VERIFY: f1080d7c auto
// OFFSET: 0x00631FA0
void NetClient::SetMessageHandler(NETMESSAGE msgId, MESSAGE_HANDLER handler, void* param) {
    this->m_handlers[msgId] = handler;
    this->m_handlerParams[msgId] = param;
}

// VERIFY: acb4d340 auto
// OFFSET: 0x00631FC0
void NetClient::ResetHandler(NETMESSAGE msgId) {
    this->m_handlers[msgId] = nullptr;
    this->m_handlerParams[msgId] = nullptr;
}

// VERIFY: 59f00590 auto
// OFFSET: 0x00631FE0
int32_t NetClient::ProcessMessage(uint32_t timeReceived, CDataStore* msg, int32_t unused_a4) {
    s_stats[4]++;

    uint16_t msgId;
    msg->Get(msgId);
    this->ValidateMessageId(msgId);

    if (msgId < NUM_MSG_TYPES && this->m_handlers[msgId]) {
        return this->m_handlers[msgId](this->m_handlerParams[msgId], static_cast<NETMESSAGE>(msgId), timeReceived, msg);
    }

    msg->Reset();

    return 0;
}

void NetClient::ValidateMessageId(uint32_t msgId) {
}

// VERIFY: eb4bad4d auto
// OFFSET: 0x00632060
void NetClient::WCConnected(WowConnection* connection, WowConnection* inbound, uint32_t timeStamp, const NETCONNADDR* addr) {
    if (connection != this->m_serverConnection) {
        return;
    }

    this->m_pingLock.Enter();
    this->m_connectedTimestamp = timeStamp;
    this->m_bytesReceived = 0;
    this->m_bytesSent = 0;
    this->m_latencyStart = 0;
    this->m_latencyEnd = 0;
    this->m_pingSent = OsGetAsyncTimeMs();
    this->m_pingLock.Leave();

    this->m_netEventQueue->AddEvent(EVENT_ID_NET_CONNECT, connection, this, nullptr, 0);
}

// VERIFY: d632b494 manual
// OFFSET: 0x006320D0
void NetClient::GetNetStats(float& bytesIn, float& bytesOut, uint32_t& latency) {
    this->m_pingLock.Enter();

    double elapsed = (OsGetAsyncTimeMs() - this->m_connectedTimestamp) * 0.001;
    bytesIn = this->m_bytesReceived * 0.0009765625 / elapsed;
    bytesOut = 0.0009765625 * this->m_bytesSent / elapsed;

    uint32_t count = 0;
    uint32_t total = 0;
    uint32_t index = this->m_latencyStart;
    uint32_t end = this->m_latencyEnd;

    while (index != end) {
        if (index >= 16) {
            index = 0;

            if (!end) {
                break;
            }
        }

        total += this->m_latency[index++];
        count++;
    }

    latency = count ? total / count : 0;

    this->m_pingLock.Leave();
}

// VERIFY: 2549c452 auto
// OFFSET: 0x006321A0
void NetClient::PollEventQueue() {
    this->m_netEventQueue->Poll();
}

// VERIFY: 1eecebd5 auto
// OFFSET: 0x006323C0
int32_t NetClient::Connect(const char* address) {
    if (this->m_netState != NS_INITIALIZED) {
        SErrDisplayAppFatal("Expected (m_netState == NS_INITIALIZED), got %d", this->m_netState);
    }

    uint16_t port = 9090;
    char host[1024];
    SStrCopy(host, address, sizeof(host));

    char* separator = SStrChr(host, ':');

    if (separator) {
        *separator = '\0';
        port = atol(separator + 1);
    }

    this->m_serverConnection->SetEncryption(false);
    this->m_netState = NS_INITIALIZED;

    return this->ConnectInternal(host, port);
}

// VERIFY: f0345188 manual
// OFFSET: 0x00632460
int32_t NetClient::HandleData(uint32_t timeReceived, void* data, int32_t size) {
    if (this->m_objMgr) {
        NetClient::PushObjMgr(this->m_objMgr);
    }

    s_stats[3] += size + 2;

    if (this->m_netState == NS_CONNECTED) {
        CDataStore msg(static_cast<uint8_t*>(data), size);
        this->ProcessMessage(timeReceived, &msg, 0);
    }

    NetClient::PopObjMgr();

    return 1;
}

// VERIFY: 72928567 auto
// OFFSET: 0x00632510
int32_t NetClient::HandleConnect() {
    if (this->m_objMgr) {
        NetClient::PushObjMgr(this->m_objMgr);
    }

    this->m_netState = NS_CONNECTED;
    NetClient::PopObjMgr();

    return 1;
}

// VERIFY: df419859 auto
// OFFSET: 0x00632540
int32_t NetClient::HandleDisconnect() {
    if (this->m_objMgr) {
        NetClient::PushObjMgr(this->m_objMgr);
    }

    this->m_netState = NS_INITIALIZED;
    ConsolePrintf("NetClient::HandleDisconnect()");
    CGlueMgr::NetDisconnectHandler(this);
    NetClient::PopObjMgr();

    return 1;
}

// VERIFY: 251c34c2 auto
// OFFSET: 0x00632590
int32_t NetClient::HandleCantConnect() {
    if (this->m_objMgr) {
        NetClient::PushObjMgr(this->m_objMgr);
    }

    this->m_netState = NS_INITIALIZED;
    NetClient::PopObjMgr();

    return 1;
}

// VERIFY: 5652082f auto
// OFFSET: 0x006325C0
void NetClient::HandlePong(WowConnection* connection, CDataStore* msg) {
    if (connection != this->m_serverConnection || this->m_suspended) {
        connection->Disconnect();
        return;
    }

    this->m_pingLock.Enter();

    uint32_t sequence;
    msg->Get(sequence);

    if (sequence != this->m_pingSequence) {
        ConsolePrintf("Received pong with old sequence");
        this->m_pingLock.Leave();
        return;
    }

    this->m_latency[this->m_latencyEnd++] = OsGetAsyncTimeMs() - this->m_pingSent;

    if (this->m_latencyEnd >= 16) {
        this->m_latencyEnd = 0;
    }

    if (this->m_latencyEnd == this->m_latencyStart) {
        if (++this->m_latencyStart >= 16) {
            this->m_latencyStart = 0;
        }
    }

    this->m_pingLock.Leave();
}

// VERIFY: 1421ef4d manual
// OFFSET: 0x00632690
void NetClient::EnableEncryption(WowConnection* connection, const uint8_t* seed, uint8_t seedLength) {
    connection->SetEncryptionKey(this->m_loginData.m_sessionKey, sizeof(this->m_loginData.m_sessionKey), 1, seed, seedLength);
    connection->m_uint375 = 4;
    connection->m_uint376 = 2;
    connection->SetEncryption(true);
}

// VERIFY: 08d8deb0 auto
// OFFSET: 0x006326D0
void NetClient::WCDisconnected(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) {
    this->m_pingLock.Enter();
    OsGetAsyncTimeMs();

    float bytesIn;
    float bytesOut;
    uint32_t latency;
    this->GetNetStats(bytesIn, bytesOut, latency);

    this->m_pingLock.Leave();

    if (this->m_netEventQueue) {
        this->m_netEventQueue->AddEvent(EVENT_ID_NET_DISCONNECT, connection, this, nullptr, 0);
    }
}

// VERIFY: 4a65a25e manual
// OFFSET: 0x00632730
void NetClient::HandleAuthChallenge(WowConnection* connection, CDataStore* msg) {
    AuthenticationChallenge* challenge = static_cast<AuthenticationChallenge*>(SMemAlloc(sizeof(AuthenticationChallenge), __FILE__, __LINE__, 0));

    uint32_t difficulty;
    msg->Get(difficulty);
    msg->Get(challenge->m_serverSeed);

    uint32_t seeds[8];

    for (int32_t i = 0; i < 8; i++) {
        msg->Get(seeds[i]);
    }

    SHA1_CONTEXT context;
    SHA1_Init(&context);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(this->m_loginData.m_account), SStrLen(this->m_loginData.m_account));

    for (int32_t i = 0; i < 8; i++) {
        SHA1_Update(&context, reinterpret_cast<const uint8_t*>(&seeds[i]), sizeof(seeds[i]));
    }

    if (!ComputeProofOfWork(&context, difficulty, &challenge->m_proof)) {
        connection->Disconnect();
    } else if (connection == this->m_serverConnection) {
        this->m_netEventQueue->AddEvent(EVENT_ID_NET_AUTH_CHALLENGE, connection, this, challenge, sizeof(AuthenticationChallenge));
    } else if (connection == this->m_redirectConnection) {
        CDataStore reply;
        reply.Put(static_cast<uint32_t>(CMSG_AUTH_CONTINUED_SESSION));
        reply.PutString(this->m_loginData.m_account);
        reply.Put(challenge->m_proof);

        SHA1_Init(&context);
        SHA1_Update(&context, reinterpret_cast<const uint8_t*>(this->m_loginData.m_account), SStrLen(this->m_loginData.m_account));
        SHA1_Update(&context, this->m_loginData.m_sessionKey, sizeof(this->m_loginData.m_sessionKey));
        SHA1_Update(&context, reinterpret_cast<const uint8_t*>(&challenge->m_serverSeed), sizeof(challenge->m_serverSeed));

        uint8_t digest[20];
        SHA1_Final(digest, &context);
        reply.PutData(digest, sizeof(digest));
        reply.Finalize();

        this->m_redirectConnection->SendPacket(&reply, 0);

        WowConnection* redirect = this->m_redirectConnection;
        redirect->SetEncryptionKey(this->m_loginData.m_sessionKey, sizeof(this->m_loginData.m_sessionKey), 1, reinterpret_cast<const uint8_t*>(seeds), sizeof(seeds));
        redirect->m_uint375 = 4;
        redirect->m_uint376 = 2;
        redirect->SetEncryption(true);
    } else {
        connection->Disconnect();
    }

    if (challenge) {
        SMemFree(challenge, "delete", SLOG_FUNCTION, 0);
    }
}

// VERIFY: 1f76bddf manual
// OFFSET: 0x00632A40
NetClient::NetClient() {
    this->m_netState = NS_UNINITIALIZED;
    this->m_netEventQueue = nullptr;
    this->m_serverConnection = nullptr;
    this->m_redirectConnection = nullptr;
    this->m_suspended = false;
    this->m_refCount.m_value = 0;
    this->m_deleteMe = false;
    this->m_pingSent = 0;
    this->m_pingSequence = 0;
    this->m_bytesSent = 0;
    this->m_bytesReceived = 0;
    this->m_connectedTimestamp = 0;
    this->m_objMgr = nullptr;

    NETCLIENTNODE* node = s_clientList.NewNode(2, 0, 0);
    node->m_client = this;
}

// VERIFY: 3e4c9045 manual
// OFFSET: 0x00632B50
void NetClient::Send(CDataStore* msg) {
    if (this->m_netState != NS_CONNECTED) {
        return;
    }

    uint32_t size = msg->m_size - msg->m_read;

    if (!size) {
        return;
    }

    if (this->m_suspended) {
        NetClient::HELDMESSAGE* held = this->m_heldMessages.NewNode(2, 0, 0);
        uint32_t bytes = msg->m_size - msg->m_read;
        void* data;
        msg->GetDataInSitu(data, bytes);
        held->m_msg.PutData(data, bytes);
        held->m_msg.Finalize();
        return;
    }

    this->m_serverConnection->SendPacket(msg, 0);
    WowConnection* connection = this->m_serverConnection;
    s_stats[0] += size;
    s_stats[1]++;
    this->m_bytesSent += size;

    if (!connection->m_encrypt) {
        this->EnableEncryption(connection, nullptr, 0);
    }
}

// VERIFY: 30fc936b manual
// OFFSET: 0x00632C10
void NetClient::WCCantConnect(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) {
    if (connection == this->m_redirectConnection) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CONNECT_TO_FAILED));
        msg.Put(this->m_redirectKey);
        msg.Finalize();
        this->Send(&msg);
        this->m_redirectConnection->Release();
        this->m_redirectConnection = nullptr;
    } else if (connection == this->m_serverConnection) {
        this->m_netEventQueue->AddEvent(EVENT_ID_NET_CANTCONNECT, connection, this, nullptr, 0);
    }
}

// VERIFY: af338d14 manual
// OFFSET: 0x00632CD0
void NetClient::Ping() {
    if (!this->m_serverConnection->m_encrypt) {
        return;
    }

    CDataStore msg;

    this->m_pingLock.Enter();
    this->m_pingSent = OsGetAsyncTimeMs();
    msg.Put(static_cast<uint32_t>(CMSG_PING));
    msg.Put(++this->m_pingSequence);

    if (this->m_netState == NS_CONNECTED) {
        if (this->m_latencyEnd) {
            msg.Put(this->m_latency[this->m_latencyEnd - 1]);
        } else {
            msg.Put(static_cast<uint32_t>(0));
        }
    }

    this->m_pingLock.Leave();
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: 55fb6d51 auto
// OFFSET: 0x00632DB0
void NetClient::HandleIdle() {
    s_idleStats[0] = s_stats[0];
    s_idleStats[1] = s_stats[3];
    s_idleStats[2] = s_stats[1];
    s_idleStats[3] = s_stats[4];

    if (static_cast<int32_t>(OsGetAsyncTimeMs() - this->m_pingSent - 30000) >= 0) {
        this->Ping();
    }
}

// VERIFY: 4b27bdaa manual
// OFFSET: 0x00631EA0
void NetClient::Disconnect() {
    if (this->m_redirectConnection) {
        this->m_redirectConnection->SetResponse(nullptr, false);
        this->m_redirectConnection->Disconnect();
        this->m_redirectConnection->Release();
        this->m_redirectConnection = nullptr;
    }

    if (this->m_netState == NS_CONNECTED) {
        this->m_netState = NS_UNK_6;
        this->m_serverConnection->Disconnect();
        return;
    }

    this->m_serverConnection->SetResponse(nullptr, false);
    this->m_serverConnection->Disconnect();
    this->m_netEventQueue->Clear();
    this->m_serverConnection->Release();
    this->m_serverConnection = new (SMemAlloc(sizeof(WowConnection), __FILE__, __LINE__, 0)) WowConnection(this, nullptr);
    this->m_netState = NS_INITIALIZED;
}

// VERIFY: 7891283c manual
// OFFSET: 0x006322A0
void NetClient::Destroy() {
    if (this->m_netState == NS_UNINITIALIZED) {
        return;
    }

    this->Disconnect();
    this->m_serverConnection->SetResponse(nullptr, false);
    this->m_serverConnection->Release();
    this->m_serverConnection = nullptr;
    memset(this->m_handlers, 0, sizeof(this->m_handlers));
    memset(this->m_handlerParams, 0, sizeof(this->m_handlerParams));

    delete this->m_netEventQueue;
    this->m_netEventQueue = nullptr;

    if (--NetClient::s_clientCount == 0) {
        OsSleep(1);
        WowConnection::DestroyOsNet();
    }

    this->m_netState = NS_UNINITIALIZED;

    if (!NetClient::s_clientCount) {
        ConsolePrintf("Client net stats: %Lu bytes sent, %Lu bytes received, %Lu msec elapsed", s_stats[0], s_stats[3], OsGetAsyncTimeMs() - s_initializeTime);
    }
}

// VERIFY: a5e5149c manual
// OFFSET: [0x006331B0, 0x00633300]
NetClient::~NetClient() {
    this->Destroy();

    for (NETCLIENTNODE* node = s_clientList.Head(); node; node = s_clientList.Next(node)) {
        if (node->m_client == this) {
            s_clientList.DeleteNode(node);
            break;
        }
    }
}

// VERIFY: 4136bd3d auto
// OFFSET: 0x00633630
void NETEVENTQUEUE::Clear() {
    this->m_critSect.Enter();
    this->m_eventQueue.Clear();
    this->m_critSect.Leave();
}

// VERIFY: 796de75c manual
// OFFSET: 0x00633650
void NETEVENTQUEUE::AddEvent(EVENTID eventId, void* connection, NetClient* client, const void* data, uint32_t size) {
    this->m_critSect.Enter();

    NETEVENTQUEUENODE* node = this->m_eventQueue.NewNode(2, 0, 8);
    node->m_eventId = eventId;

    if (size) {
        node->m_data = SMemAlloc(size, __FILE__, __LINE__, 0);
        memcpy(node->m_data, data, size);
        node->m_dataSize = size;
    } else {
        node->m_data = nullptr;
        node->m_dataSize = 0;
    }

    node->m_timeReceived = OsGetAsyncTimeMs();
    client->m_refCount.Increment();

    this->m_critSect.Leave();
}

// VERIFY: 4d2c7b3d manual
// OFFSET: 0x00633730
NETEVENTQUEUE::~NETEVENTQUEUE() {
    this->m_critSect.Enter();
    this->m_eventQueue.Clear();
    this->m_critSect.Leave();
}

// VERIFY: 75864678 auto
// OFFSET: 0x00633020
void NetClient::HandleSuspendComms(WowConnection* connection, CDataStore* msg) {
    if (connection != this->m_serverConnection || this->m_suspended || !this->m_redirectConnection) {
        connection->Disconnect();
        return;
    }

    uint32_t serial = 0;
    msg->Get(serial);

    if (!msg->IsRead() || msg->m_read > msg->m_size) {
        connection->Disconnect();
        return;
    }

    CDataStore ack;
    ack.Put(static_cast<uint32_t>(CMSG_SUSPEND_COMMS_ACK));
    ack.Put(serial);
    ack.Finalize();
    this->Send(&ack);
    this->m_suspended = 1;
}

// VERIFY: 5dad1904 manual
// OFFSET: 0x00633250
void NetClient::HandleResumeComms(WowConnection* connection, CDataStore* msg) {
    if (connection == this->m_redirectConnection) {
        WowConnection* old = this->m_serverConnection;
        this->m_serverConnection = this->m_redirectConnection;
        this->m_redirectConnection = nullptr;
        old->SetResponse(nullptr, false);
        old->Disconnect();
        old->Release();
    } else {
        this->m_redirectConnection->SetResponse(nullptr, false);
        this->m_redirectConnection->Disconnect();
        this->m_redirectConnection->Release();
        this->m_redirectConnection = nullptr;
    }

    this->m_suspended = 0;

    for (NetClient::HELDMESSAGE* held = this->m_heldMessages.Head(); held; held = this->m_heldMessages.DeleteNode(held)) {
        this->Send(&held->m_msg);
    }
}

// VERIFY: 4e75a52e auto
// OFFSET: 0x00632390
int32_t NetClient::DelayedDelete() {
    if (!this->m_netState) {
        return 0;
    }

    this->Disconnect();
    this->m_netEventQueue->AddEvent(EVENT_ID_NET_DESTROY, nullptr, this, nullptr, 0);
    return 1;
}

// VERIFY: 1be3a11a manual
// OFFSET: 0x00632970
NetClient::HELDMESSAGE::~HELDMESSAGE() {
}

// VERIFY: e0527e61 manual
// OFFSET: 0x00632E00
void NetClient::HandleRedirectClient(WowConnection* connection, CDataStore* msg) {
    uint32_t address = 0;
    uint16_t port = 0;
    this->m_redirectKey = 0;
    msg->Get(address);
    msg->Get(port);
    msg->Get(this->m_redirectKey);

    if (this->m_redirectConnection || this->m_suspended) {
        CDataStore response;
        response.Put(static_cast<uint32_t>(CMSG_CONNECT_TO_FAILED));
        response.Put(this->m_redirectKey);
        response.Finalize();
        this->Send(&response);
        return;
    }

    this->m_redirectConnection = new (SMemAlloc(sizeof(WowConnection), __FILE__, __LINE__, 0)) WowConnection(this, nullptr);

    HMAC<SHA1> hmac;
    SHA1::Digest digest;
    hmac.Init(this->m_loginData.m_sessionKey, sizeof(this->m_loginData.m_sessionKey));
    hmac.Process(&address, sizeof(address));
    hmac.Process(&port, sizeof(port));
    hmac.Finish(digest);

    uint8_t proof[20];
    msg->GetData(proof, sizeof(proof));

    if (memcmp(&digest, proof, sizeof(proof)) || !msg->IsRead() || msg->m_read > msg->m_size) {
        connection->Disconnect();
    } else {
        this->m_redirectConnection->Connect(address, port, -1);
    }
}
