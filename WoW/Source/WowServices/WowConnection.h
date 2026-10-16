#ifndef WOW_SOURCE_WOWSERVICES_WOWCONNECTION_H
#define WOW_SOURCE_WOWSERVICES_WOWCONNECTION_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/Os/OsTcp.h"
#include "Engine/Source/Storm/H/Types.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "WoW/Source/WowServices/Types.h"

class CDataStore;
class WowConnectionResponse;

typedef Blizzard::Lock::Atomic ATOMIC32;

class WowConnectionNet;

void ARC4PrepareKey(const uint8_t* key, uint32_t length, SARC4Key* state);

class WowConnection {
    public:
    struct SENDNODE : public TSLinkedNode<WowConnection::SENDNODE> {
        /* 0008 */ uint8_t* m_data;
        /* 000C */ uint32_t m_size;
        /* 0010 */ uint32_t m_offset;
        /* 0014 */ uint32_t m_datasize;
        /* 0018 */ uint8_t m_header [8];
        /* 0020 */ uint32_t m_sendTime;
        /* 0024 */ uint32_t m_allocsize;

        SENDNODE(const void* data, int32_t size, uint8_t* buffer, bool raw);
        SENDNODE(const WowConnection::SENDNODE& source, uint8_t* buffer);
    };

    struct IDBITS {
        /* 0000 */ uint32_t m_count;
        /* 0004 */ TSFixedArray<uint32_t> m_bits;

        void Or(const WowConnection::IDBITS& source);
        void SetCount(uint32_t count);
    };

    static WowConnection::IDBITS s_idBits;
    static SCritSect s_idLock;
    static uint64_t s_sendBytes;
    static WowConnectionNet* s_network;
    static int32_t s_lagTestDelayMin;
    static int32_t s_lagTestDelayMax;
    static bool (*s_verifyAddr)(const NETADDR* addr);

    /* 0000 */ ATOMIC32 m_refCount;
    /* 0004 */ int32_t m_sock;
    /* 0008 */ int32_t m_oldsock;
    /* 000C */ uint8_t unused_000C;
    /* 000D */ uint8_t unused_000D[3];
    /* 0010 */ WOW_CONN_STATE m_connState;
    /* 0014 */ WowConnectionResponse* m_response;
    /* 0018 */ int32_t unused_0018;
    /* 001C */ uint8_t* m_readBuffer;
    /* 0020 */ int32_t m_readBytes;
    /* 0024 */ int32_t m_readBufferSize;
    /* 0028 */ SCritSect m_outLock;
    /* 0050 */ int32_t unused_0050;
    /* 0054 */ int32_t unused_0054;
    /* 0058 */ int32_t unused_0058;
    /* 005C */ int32_t unused_005C;
    /* 0060 */ int32_t unused_0060;
    /* 0064 */ uint16_t unused_0064;
    /* 0066 */ uint8_t unused_0066[2];
    /* 0068 */ void (*m_threadInit)();
    /* 006C */ uint32_t m_connectAddress;
    /* 0070 */ uint16_t m_connectPort;
    /* 0072 */ uint8_t unused_0072[2];
    /* 0074 */ int32_t m_connectRetryInterval;
    /* 0078 */ int32_t unused_0078;
    /* 007C */ int32_t unused_007C;
    /* 0080 */ NETCONNADDR m_peer;
    /* 00A0 */ int32_t unused_00A0;
    /* 00A4 */ SCritSect m_responseLock;
    /* 00CC */ int32_t m_responseRef;
    /* 00D0 */ uintptr_t m_responseRefThread;
    /* 00D4 */ WowConnectionResponse* m_pendingResponse;
    /* 00D8 */ TSLink<WowConnection> m_netLink;
    /* 00E0 */ TSList<WowConnection::SENDNODE> m_sendList;
    /* 00EC */ int32_t m_sendDepth;
    /* 00F0 */ int32_t m_sendBytes;
    /* 00F4 */ int32_t m_maxSendDepth;
    /* 00F8 */ int32_t m_serviceFlags;
    /* 00FC */ TSList<WowConnection::SENDNODE> m_lagTestRecvList;
    /* 0108 */ SCritSect m_lock;
    /* 0130 */ int32_t m_serviceCount;
    /* 0134 */ int32_t unused_0134;
    /* 0138 */ void* m_event;
    /* 013C */ WOWC_TYPE m_type;
    /* 0140 */ uint8_t m_notifyWriteReady;
    /* 0141 */ uint8_t unused_0141[3];
    /* 0144 */ int32_t unused_0144;
    /* 0148 */ SARC4Key m_sendKey;
    /* 024A */ SARC4Key m_receiveKey;
    /* 034C */ uint8_t m_sendKeyInit [20];
    /* 0360 */ uint8_t m_receiveKeyInit [20];
    /* 0374 */ bool m_encrypt;
    /* 0375 */ uint8_t m_uint375;
    /* 0376 */ uint8_t m_uint376;
    /* 0377 */ uint8_t unused_0377[1];

    static void DestroyOsNet();
    static int32_t InitOsNet(bool (*netAddrFilter)(const NETADDR* addr), void (*threadInit)(), int32_t numThreads, bool useEngine);
    static void MarkSocketId(int32_t sock);

    WowConnection(WowConnectionResponse* response, void (*threadInit)());
    WowConnection(int32_t sock, const NETADDR* addr, WowConnectionResponse* response);
    ~WowConnection();
    void AcquireResponseRef();
    void AddRef();
    void CheckAccept();
    void CheckConnect();
    void CloseSocket(int32_t sock);
    bool Connect(const char* address, int32_t timeout);
    bool Connect(const char* host, uint16_t port, int32_t timeout);
    bool Connect(uint32_t address, uint16_t port, int32_t timeout);
    void Disconnect();
    void DoDisconnect();
    void DoExceptions();
    void DoMessageReads();
    void DoReads();
    void DoStreamReads();
    void DoWrites();
    void Encrypt(uint8_t* data, uint32_t length);
    void FreeSendList();
    void FreeSendNode(WowConnection::SENDNODE* node);
    void Init(WowConnectionResponse* response, void (*threadInit)());
    WowConnection::SENDNODE* NewSendNodeCopy(const WowConnection::SENDNODE* source, int32_t size);
    WowConnection::SENDNODE* NewSendNodePacket(const void* data, int32_t size, bool raw);
    char* PeerAddrToStr(char* buffer, uint32_t size);
    int32_t Release();
    void ReleaseResponseRef();
    int32_t Send(uint8_t* data, int32_t size, bool raw);
    int32_t SendPacket(CDataStore* msg, int32_t headroom);
    void SetEncryption(bool enabled);
    void SetEncryptionKey(const uint8_t* key, uint8_t keyLength, uint8_t direction, const uint8_t* seed, uint8_t seedLength);
    void SetResponse(WowConnectionResponse* response, bool deferred);
    void SetSendTime(WowConnection::SENDNODE* node);
    void SetState(WOW_CONN_STATE state);
    void SetType(WOWC_TYPE type);
    void StartConnect();
};

class WowConnectionResponse {
    public:
    virtual ~WowConnectionResponse();
    virtual void WCMessageReady(WowConnection* connection, uint32_t timeStamp, CDataStore* msg) = 0;
    virtual void WCConnected(WowConnection* connection, WowConnection* inbound, uint32_t timeStamp, const NETCONNADDR* addr) = 0;
    virtual void WCCantConnect(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) = 0;
    virtual void WCDisconnected(WowConnection* connection, uint32_t timeStamp, const NETCONNADDR* addr) = 0;
    virtual void WCGlobalLock();
    virtual void WCGlobalUnlock();
    virtual void WCDataReady(WowConnection* connection, uint32_t timeStamp, uint8_t* data, int32_t length);
    virtual void WCWriteReady(WowConnection* connection);
    virtual void NotifyAboutToDoReads();
    virtual void NotifySendDepthExceeded(WowConnection* connection);
};

class ServerConnection {
    public:
    /* 0000 */ uint8_t unused_0000[4];
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint8_t unused_0008[8];
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint8_t unused_0018[180];
    /* 00CC */ uint32_t unused_00CC;
    /* 00D0 */ uint32_t unused_00D0;
    /* 00D4 */ uint32_t unused_00D4;
    /* 00D8 */ uint8_t unused_00D8[16];
    /* 00E8 */ uint32_t unused_00E8;
    /* 00EC */ uint32_t unused_00EC;
    /* 00F0 */ uint32_t unused_00F0;
    /* 00F4 */ uint32_t unused_00F4;
    /* 00F8 */ uint8_t unused_00F8[596];
    /* 034C */ uint32_t unused_034C;
    /* 0350 */ uint32_t unused_0350;
    /* 0354 */ uint32_t unused_0354;
    /* 0358 */ uint32_t unused_0358;
    /* 035C */ uint32_t unused_035C;
    /* 0360 */ uint32_t unused_0360;
    /* 0364 */ uint32_t unused_0364;
    /* 0368 */ uint32_t unused_0368;
    /* 036C */ uint32_t unused_036C;
    /* 0370 */ uint32_t unused_0370;
    /* 0374 */ uint8_t unused_0374;
    /* 0375 */ uint8_t unused_0375;
};

class WoWConnection {
    public:
    /* 0000 */ uint8_t unused_0000[232];
    /* 00E8 */ uint32_t unused_00E8;
    /* 00EC */ uint8_t unused_00EC[24];
    /* 0104 */ uint32_t unused_0104;
};

#endif
