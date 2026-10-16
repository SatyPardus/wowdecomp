#ifndef WOW_SOURCE_BATTLENET_BATTLENET_H
#define WOW_SOURCE_BATTLENET_BATTLENET_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Battlenet/BSN/BSNEncodeInterface.h"

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace Battlenet {
class Allocator {
    public:
    virtual ~Allocator();
    virtual void* Alloc(uint32_t size) = 0;
    virtual void Free(void* memory) = 0;

    template <class T>
    void Delete(T*& object);
};

extern Allocator* g_allocator;

bool Initialize(Allocator& allocator);
void Teardown();

namespace IP4 {
struct Address {
    /* 0000 */ uint8_t m_value[4];
};

struct AddressPort {
    /* 0000 */ uint8_t m_address[4];
    /* 0004 */ uint8_t m_port[2];
};
}

namespace Client {
struct DNSInfo {
    /* 0000 */ BSN::Array<IP4::Address, 127> m_addresses;
    /* 0200 */ BSN::AsciiString<255> m_name;
};
}

namespace Event {
struct Header {
    struct Source {
        /* 0000 */ bool m_exists;
        /* 0004 */ uint32_t m_adapter;
        /* 0008 */ uint32_t m_event;
    };

    struct Target {
        /* 0000 */ uint32_t m_type;
        /* 0004 */ uint32_t m_value;
    };

    /* 0000 */ uint8_t m_eventType;
    /* 0004 */ Source m_source;
    /* 0010 */ Target m_target;

    Header();
};
}

class ApplicationFile;

class SystemClient {
    public:
    virtual void Resolved(uint32_t request, const Client::DNSInfo& info) = 0;
    virtual void Connected(uint32_t connection, const IP4::AddressPort& address) = 0;
    virtual void Disconnected(uint32_t connection) = 0;
    virtual bool Recv(uint32_t connection, void* data, uint32_t size, uint32_t& used) = 0;
    virtual bool RecvDatagram(uint32_t connection, void* data, uint32_t size, const IP4::AddressPort& address) = 0;
    virtual void Process(uint32_t time) = 0;
    virtual ~SystemClient();
};

class SystemServer {
    public:
    virtual uint32_t Resolve(uint32_t token, const char* hostname) = 0;
    virtual uint32_t Connect(const IP4::AddressPort& address, const uint32_t& tendrilId) = 0;
    virtual void Disconnect(uint32_t connection) = 0;
    virtual bool Send(uint32_t connection, const void* data, uint32_t size) = 0;
    virtual uint32_t OpenDatagram(int32_t port, const uint32_t& tendrilId) = 0;
    virtual void CloseDatagram(uint32_t connection) = 0;
    virtual bool SendDatagram(uint32_t connection, const void* data, uint32_t size, const IP4::AddressPort& address) = 0;
    virtual uint64_t GetCurrentTime() const = 0;
    virtual uint64_t GetCurrentTick() const = 0;
    virtual void LogLevel(const char* message, int32_t level) = 0;
    virtual void Log(const char* message);
    virtual ApplicationFile* OpenApplicationFile(const char* name) = 0;
    virtual void CloseApplicationFile(ApplicationFile* file) = 0;
    virtual ~SystemServer();
};

class EventSubscriber {
    public:
    /* 0004 */ BSN::FourCC m_eventId;

    virtual void RecvEvent(const Event::Header& header, const BSN::EncodedObject* data) = 0;
    virtual ~EventSubscriber();

    EventSubscriber();
};

class EventInterface {
    public:
    virtual void SendEvent(uint8_t eventType, uint32_t adapter, const BSN::EncodedObject* data) = 0;
    virtual void SendEvent(uint8_t eventType, BSN::FourCC eventId, const BSN::EncodedObject* data) = 0;
    virtual void Subscribe(BSN::FourCC eventId, const uint32_t& tendrilId) = 0;
    virtual ~EventInterface();
};

class Application : public EventSubscriber, public SystemServer {
    public:
};

class Library {
    public:
    static Library* Load(const char* path, Allocator& allocator);
    static Library* LoadApplicationBundle(const char* name, Allocator& allocator);

    virtual void Destroy() = 0;
    virtual ~Library();
    virtual void* GetProc(const char* name) = 0;
    virtual void* GetProc(uint32_t ordinal) = 0;
};

class WindowsLibrary : public Library {
    public:
    /* 0004 */ void* m_module;
    /* 0008 */ Allocator* m_allocator;

    void Destroy() override;
    ~WindowsLibrary() override;
    void* GetProc(const char* name) override;
    void* GetProc(uint32_t ordinal) override;
};

#if !defined(_WIN32)
class PosixLibrary : public Library {
    public:
    void* m_module;
    Allocator* m_allocator;

    void Destroy() override;
    ~PosixLibrary() override;
    void* GetProc(const char* name) override;
    void* GetProc(uint32_t ordinal) override;
};
#endif

bool GetProc(Library* library, void*& proc, uint32_t ordinal);
uint32_t StringCopy(void* dest, const char* source, uint32_t size);

template <class T>
void Delete(T* object);
}

// VERIFY: 4e792b5b auto
// OFFSET: 0x008C65F0<Battlenet::TumorManager>
template <class T>
void Battlenet::Allocator::Delete(T*& object) {
    if (object) {
        object->~T();
        this->Free(object);
        object = nullptr;
    }
}

// INLINED
template <class T>
void Battlenet::Delete(T* object) {
    Allocator* allocator = Battlenet::g_allocator;

    if (object) {
        object->~T();
        allocator->Free(object);
    }
}

#endif
