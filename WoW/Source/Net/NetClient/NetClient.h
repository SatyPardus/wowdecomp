#ifndef WOW_SOURCE_NET_NETCLIENT_NETCLIENT_H
#define WOW_SOURCE_NET_NETCLIENT_NETCLIENT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "WoW/Source/Net/NetClient/Types.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/WowServices/WowConnection.h"

class CGObject_C;
class CHashKeyGUID;
class ClientConnection;
struct ClntObjMgr;

struct ClntObjMgr {
    /* 0000 */ TSHashTable<CGObject_C, CHashKeyGUID> m_objects;
    /* 0028 */ TSHashTable<CGObject_C, CHashKeyGUID> m_lazyCleanupObjects;
    /* 0050 */ TSList<CGObject_C> m_lazyCleanupFifo [7];
    /* 00A4 */ TSList<CGObject_C> m_visibleObjects;
    /* 00B0 */ TSList<CGObject_C> m_reenabledObjects;
    /* 00BC */ uint8_t unused_00BC[4];
    /* 00C0 */ WOWGUID m_activePlayer;
    /* 00C8 */ PLAYER_TYPE m_type;
    /* 00CC */ uint32_t m_mapID;
    /* 00D0 */ ClientConnection* m_net;
    /* 00D4 */ uint8_t unused_00D4[4];
};

class AuthenticationChallenge {
    public:
    /* 0000 */ uint32_t m_serverSeed;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint64_t m_proof;
};

struct LoginData {
    /* 0000 */ char m_account [1280];
    /* 0500 */ int32_t m_loginServerID;
    /* 0504 */ uint8_t m_sessionKey [40];
    /* 052C */ int32_t m_loginServerType;
};

typedef int32_t (* MESSAGE_HANDLER_interface )(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

typedef MESSAGE_HANDLER_interface MESSAGE_HANDLER;

class NetClient;

struct NETEVENTQUEUENODE : public TSLinkedNode<NETEVENTQUEUENODE> {
    /* 0008 */ uint32_t m_eventId;
    /* 000C */ uint32_t m_timeReceived;
    /* 0010 */ void* m_data;
    /* 0014 */ uint32_t m_dataSize;
};

class NETEVENTQUEUE {
    public:
    /* 0000 */ NetClient* m_client;
    /* 0004 */ SCritSect m_critSect;
    /* 002C */ TSList<NETEVENTQUEUENODE> m_eventQueue;

    NETEVENTQUEUE(NetClient* client);
    ~NETEVENTQUEUE();
    void AddEvent(EVENTID eventId, void* connection, NetClient* client, const void* data, uint32_t size);
    void Clear();
    void Poll();
};

class NetClient : public WowConnectionResponse {
    public:
    static uint32_t s_clientCount;

    struct HELDMESSAGE : public TSLinkedNode<NetClient::HELDMESSAGE> {
        /* 0008 */ CDataStore m_msg;

        ~HELDMESSAGE();
    };

    /* 0004 */ LoginData m_loginData;
    /* 0534 */ NETSTATE m_netState;
    /* 0538 */ bool m_suspended;
    /* 0539 */ uint8_t unused_0539[3];
    /* 053C */ MESSAGE_HANDLER m_handlers [1311];
    /* 19B8 */ void* m_handlerParams [1311];
    /* 2E34 */ NETEVENTQUEUE* m_netEventQueue;
    /* 2E38 */ WowConnection* m_serverConnection;
    /* 2E3C */ WowConnection* m_redirectConnection;
    /* 2E40 */ uint32_t m_redirectKey;
    /* 2E44 */ ATOMIC32 m_refCount;
    /* 2E48 */ bool m_deleteMe;
    /* 2E49 */ uint8_t unused_2E49[3];
    /* 2E4C */ uint32_t m_pingSent;
    /* 2E50 */ uint32_t m_pingSequence;
    /* 2E54 */ uint32_t m_latency [16];
    /* 2E94 */ uint32_t m_latencyStart;
    /* 2E98 */ uint32_t m_latencyEnd;
    /* 2E9C */ uint32_t m_bytesSent;
    /* 2EA0 */ uint32_t m_bytesReceived;
    /* 2EA4 */ uint32_t m_connectedTimestamp;
    /* 2EA8 */ SCritSect m_pingLock;
    /* 2ED0 */ ClntObjMgr* m_objMgr;
    /* 2ED4 */ TSList<NetClient::HELDMESSAGE> m_heldMessages;

    ~NetClient() override;
    void WCMessageReady(WowConnection* connection, uint32_t timeStamp, CDataStore* msg) override;
    void WCConnected(WowConnection* connection, WowConnection* inbound, uint32_t timeStamp, const NETCONNADDR* addr) override;
    void WCCantConnect(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) override;
    void WCDisconnected(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) override;
    virtual int32_t Initialize();
    virtual void Destroy();
    virtual int32_t DelayedDelete();
    virtual int32_t HandleData(uint32_t timeReceived, void* data, int32_t size);
    virtual void HandleAuthChallenge(AuthenticationChallenge* challenge) = 0;
    virtual int32_t HandleConnect();
    virtual int32_t HandleDisconnect();
    virtual int32_t HandleCantConnect();
    virtual void ValidateMessageId(uint32_t msgId);

    static void PopObjMgr();
    static void PushObjMgr(ClntObjMgr* mgr);

    NetClient();
    int32_t Connect(const char* address);
    int32_t ConnectInternal(const char* host, uint16_t port);
    void EnableEncryption(WowConnection* connection, const uint8_t* seed, uint8_t seedLength);
    void GetNetStats(float& bytesIn, float& bytesOut, uint32_t& latency);
    uint8_t* GetSessionKey();
    void HandleAuthChallenge(WowConnection* connection, CDataStore* msg);
    void HandleIdle();
    void HandlePong(WowConnection* connection, CDataStore* msg);
    void HandleRedirectClient(WowConnection* connection, CDataStore* msg);
    void HandleResumeComms(WowConnection* connection, CDataStore* msg);
    void HandleSuspendComms(WowConnection* connection, CDataStore* msg);
    int32_t ProcessMessage(uint32_t timeReceived, CDataStore* msg, int32_t unused_a4);
    void Ping();
    void PollEventQueue();
    void Disconnect();
    void ResetHandler(NETMESSAGE msgId);
    void Send(CDataStore* msg);
    void SetMessageHandler(NETMESSAGE msgId, MESSAGE_HANDLER handler, void* param);
    void SetLoginData(LoginData* loginData);
};

class CNetClient {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint8_t unused_0004[1328];
    /* 0534 */ uint32_t unused_0534;
    /* 0538 */ uint8_t unused_0538[4];
    /* 053C */ uint32_t unused_053C;
    /* 0540 */ uint8_t unused_0540[5240];
    /* 19B8 */ uint32_t unused_19B8;
    /* 19BC */ uint8_t unused_19BC[5244];
    /* 2E38 */ uint32_t unused_2E38;
    /* 2E3C */ uint8_t unused_2E3C[256];
    /* 2F3C */ uint32_t unused_2F3C;
    /* 2F40 */ uint8_t unused_2F40[4];
    /* 2F44 */ uint32_t unused_2F44;
    /* 2F48 */ uint32_t unused_2F48;
    /* 2F4C */ uint32_t unused_2F4C;
    /* 2F50 */ uint32_t unused_2F50;
    /* 2F54 */ uint32_t unused_2F54;
    /* 2F58 */ uint8_t unused_2F58[1];
    /* 2F59 */ uint8_t unused_2F59;
    /* 2F5A */ uint8_t unused_2F5A[2];
    /* 2F5C */ uint32_t unused_2F5C;
};

struct NETCLIENTNODE : public TSLinkedNode<NETCLIENTNODE> {
    /* 0008 */ NetClient* m_client;
};

#endif
