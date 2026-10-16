#ifndef ENGINE_SOURCE_COMSAT_COMSATNETWORK_H
#define ENGINE_SOURCE_COMSAT_COMSATNETWORK_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/ComSat/ComSatInterface.h"
#include "Engine/Source/ComSat/ComSatMD5.h"
#include "Engine/Source/ComSat/ComSatVector.h"

namespace ComSat {
namespace Network {

class Address {
    public:
    /* 0000 */ int32_t m_length;
    /* 0004 */ uint16_t m_family;
    /* 0006 */ uint16_t m_port;
    /* 0008 */ uint32_t m_ip;
    /* 000C */ uint8_t m_zero[8];
    /* 0014 */ uint8_t unused_0014[12];

    Address();
    int32_t GetPort() const;
    bool operator==(const Address& other) const;
    void SetAddress(const char* address);
    void SetPort(int32_t port);
};

class Socket {
    public:
    static Socket* Create(Allocator& allocator);

    virtual ~Socket();
    virtual bool Bind(int32_t port) = 0;
    virtual int32_t SendTo(const Address& address, const uint8_t* data, uint32_t size) = 0;
    virtual bool RecvFrom(Address& address, uint8_t* data, uint32_t& size) = 0;
};

class SocketPosix : public Socket {
    public:
    /* 0004 */ uintptr_t m_socket;

    virtual ~SocketPosix();
    virtual bool Bind(int32_t port);
    virtual int32_t SendTo(const Address& address, const uint8_t* data, uint32_t size);
    virtual bool RecvFrom(Address& address, uint8_t* data, uint32_t& size);

    SocketPosix();
};

struct Endpoint {
    /* 0000 */ Address m_address;
    /* 0020 */ uint8_t m_key[16];
    /* 0030 */ uint32_t m_refCount;
};

#pragma pack(push, 1)
struct Packet {
    /* 0000 */ uint32_t m_signature;
    /* 0004 */ uint8_t m_talker;
    /* 0005 */ uint16_t m_networkId;
    /* 0007 */ uint8_t m_data[1017];
};
#pragma pack(pop)

class HMAC {
    public:
    /* 0000 */ MD5 m_inner;
    /* 0058 */ uint8_t m_innerPad[64];
    /* 0098 */ uint8_t m_outerPad[64];

    void Init(const uint8_t* key, uint32_t length);
};

class Manager;

class Session {
    public:
    class Callback {
        public:
        virtual ~Callback();
        virtual void Recv(uint64_t talker, const uint8_t* data, uint32_t size) = 0;
    };

    struct LocalTalker {
        /* 0000 */ uint64_t m_talker;
        /* 0008 */ uint8_t m_networkId;

        bool operator==(const LocalTalker& other) const;
    };

    struct RemoteTalker {
        /* 0000 */ uint64_t m_talker;
        /* 0008 */ uint8_t m_networkId;
        /* 000C */ Endpoint* m_endpoint;

        bool operator==(const RemoteTalker& other) const;
    };

    /* 0000 */ Manager* m_manager;
    /* 0008 */ uint64_t m_id;
    /* 0010 */ uint16_t m_networkId;
    /* 0014 */ Callback* m_callback;
    /* 0018 */ uint32_t m_lastSendTime;
    /* 001C */ Vector<LocalTalker> m_localTalkers;
    /* 002C */ Vector<Endpoint*> m_endpoints;
    /* 003C */ Vector<RemoteTalker> m_remoteTalkers;

    Session(Manager& manager, uint64_t id, Callback& callback);
    ~Session();
    bool Add(uint64_t talker, uint8_t networkId, const uint8_t (&key)[16], const Address& address);
    bool LocalAdd(uint64_t talker);
    void Recv(uint64_t talker, const uint8_t* data, uint32_t size);
    bool Remove(uint64_t talker);
    bool Resolve(uint8_t networkId, const Address& address, uint64_t& talker, const uint8_t (*&key)[16]);
    bool Send(uint64_t talker, const uint8_t* data, uint32_t size);
    void SetNetworkId(uint16_t networkId);
    void SetTalkerNetworkIdLocal(uint64_t talker, uint8_t networkId);
    void SetTalkerNetworkIdRemote(uint64_t talker, uint8_t networkId);
    void Work();
};

class Manager {
    public:
    /* 0000 */ Allocator* m_allocator;
    /* 0004 */ Vector<Session*> m_sessions;
    /* 0014 */ Socket* m_socket;
    /* 0018 */ uint32_t m_time;

    Manager(const Initialization& init, Allocator& allocator);
    ~Manager();
    Session* CreateSession(uint64_t id, Session::Callback& callback);
    void DestroySession(Session* session);
    bool Send(uint16_t networkId, uint8_t talker, const uint8_t (&key)[16], const Address& address, const uint8_t* data, uint32_t size);
    void Work(uint32_t time);
};

uint32_t ComputeSignature(const uint8_t (&key)[16], const uint8_t* data, uint32_t size);

}
}

#endif
