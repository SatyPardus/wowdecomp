#include "WoW/Source/Battlenet/Tumor/CreepTendril.h"
#include <cstring>
#if defined(_WIN32)
#include <malloc.h>
#else
#include <alloca.h>
#define _alloca alloca
#endif
#include <new>
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00DCED30
uint32_t Battlenet::CreepTendril::s_instances;

// STATIC: 0x00DCED34
Battlenet::CreepTendril::CreepInitializeFunc Battlenet::CreepTendril::s_creepInitialize;

// STATIC: 0x00DCED38
Battlenet::CreepTendril::CreepTeardownFunc Battlenet::CreepTendril::s_creepTeardown;

// STATIC: 0x00DCED3C
Battlenet::CreepTendril::CreepCreateFunc Battlenet::CreepTendril::s_creepCreate;

// STATIC: 0x00DCED40
Battlenet::CreepTendril::CreepDestroyFunc Battlenet::CreepTendril::s_creepDestroy;

// STATIC: 0x00DCED44
Battlenet::Library* Battlenet::CreepTendril::s_creepModule;

// VERIFY: 6aafdbb2 manual
// OFFSET: 0x009C18D0
Battlenet::CreepTendril::Host::~Host() {
}

// VERIFY: db46cbc5 auto
// OFFSET: 0x009C18B0
void Battlenet::CreepTendril::Host::Log(const char* message) {
    this->LogLevel(message, 5);
}

// VERIFY: 74960614 auto
// OFFSET: 0x009C2460
bool Battlenet::CreepTendril::Initialize() {
    if (Battlenet::CreepTendril::s_creepModule) {
        return true;
    }

    Battlenet::CreepTendril::s_creepModule = Battlenet::Library::LoadApplicationBundle("Battle.net-prearxan", *Battlenet::g_allocator);

    if (!Battlenet::CreepTendril::s_creepModule) {
        Battlenet::CreepTendril::s_creepModule = Battlenet::Library::LoadApplicationBundle("Battle.net", *Battlenet::g_allocator);

        if (!Battlenet::CreepTendril::s_creepModule) {
            return false;
        }
    }

    Battlenet::CreepTendril::s_creepInitialize = reinterpret_cast<CreepInitializeFunc>(Battlenet::CreepTendril::s_creepModule->GetProc(1u));

    if (Battlenet::CreepTendril::s_creepInitialize) {
        Battlenet::CreepTendril::s_creepTeardown = reinterpret_cast<CreepTeardownFunc>(Battlenet::CreepTendril::s_creepModule->GetProc(2u));

        if (Battlenet::CreepTendril::s_creepTeardown) {
            Battlenet::CreepTendril::s_creepCreate = reinterpret_cast<CreepCreateFunc>(Battlenet::CreepTendril::s_creepModule->GetProc(3u));

            if (Battlenet::CreepTendril::s_creepCreate) {
                if (Battlenet::GetProc(Battlenet::CreepTendril::s_creepModule, reinterpret_cast<void*&>(Battlenet::CreepTendril::s_creepDestroy), 4) && Battlenet::CreepTendril::s_creepInitialize()) {
                    return true;
                }
            }
        }
    }

    Battlenet::CreepTendril::s_creepModule->Destroy();
    Battlenet::CreepTendril::s_creepModule = nullptr;
    return false;
}

// VERIFY: bd9022bf auto
// OFFSET: 0x009C25D0
Battlenet::CreepTendril::Instance::Instance(const Authentication::Settings& settings, TumorManager& manager) : Tendril(Battlenet::Event::CREEP, manager) {
    this->m_creep = nullptr;
    memcpy(&this->m_settings, &settings, sizeof(this->m_settings));
    Battlenet::CreepTendril::s_instances++;

    BSN::Hard::SizeEncoder sizeEncoder;
    ProtocolHard::TypeInfoOf<Authentication::Settings>::Encode(this->m_settings, sizeEncoder);

    uint32_t size = (sizeEncoder.m_size + 7) >> 3;
    BSN::Hard::Encoder encoder;
    encoder.SetData(_alloca(size), size);
    ProtocolHard::TypeInfoOf<Authentication::Settings>::Encode(this->m_settings, encoder);

    this->m_creep = Battlenet::CreepTendril::s_creepCreate(this, encoder.m_data, (encoder.m_position + 7) >> 3);

    this->m_eventHandlers[1] = &Instance::ProcessEventData<1>;
    this->m_eventHandlers[2] = &Instance::ProcessEventData<2>;
    this->m_eventHandlers[3] = &Instance::ProcessEventData<3>;
    this->m_eventHandlers[10] = &Instance::ProcessEventData<10>;
    this->m_eventHandlers[11] = &Instance::ProcessEventData<11>;
    this->m_eventHandlers[5] = &Instance::ProcessEventData<5>;
    this->m_eventHandlers[6] = &Instance::ProcessEventData<6>;
    this->m_eventHandlers[9] = &Instance::ProcessEventData<9>;
    this->m_eventHandlers[8] = &Instance::ProcessEventData<8>;
    this->m_eventHandlers[14] = &Instance::ProcessEventData<14>;
    this->m_eventHandlers[15] = &Instance::ProcessEventData<15>;
}

// VERIFY: 58336dcd auto
// OFFSET: 0x009C2840
Battlenet::CreepTendril::Instance* Battlenet::CreepTendril::Instance::Create(const Authentication::Settings& settings, TumorManager& manager) {
    if (!Battlenet::CreepTendril::s_instances && !Battlenet::CreepTendril::Initialize()) {
        return nullptr;
    }

    void* memory = Battlenet::g_allocator->Alloc(sizeof(Instance));

    if (!memory) {
        return nullptr;
    }

    return new (memory) Instance(settings, manager);
}

// VERIFY: 3c5d1abe auto
// OFFSET: [0x009C1A40, 0x009C1C50, 0x009C1CF0]
Battlenet::CreepTendril::Instance::~Instance() {
    Battlenet::CreepTendril::s_creepDestroy(this->m_creep);
    this->m_creep = nullptr;

    if (!--Battlenet::CreepTendril::s_instances && Battlenet::CreepTendril::s_creepModule) {
        Battlenet::CreepTendril::s_creepTeardown();
        Battlenet::CreepTendril::s_creepModule->Destroy();
        Battlenet::CreepTendril::s_creepModule = nullptr;
    }
}

// OFFSET: 0x009C1CC0
void Battlenet::CreepTendril::Instance::Destroy() {
    Battlenet::Delete(this);
}

// VERIFY: f6e9fe2a manual
// OFFSET: 0x009C1B10
void Battlenet::CreepTendril::Instance::EnumFunctions(EnumFunctionsCallback callback, void* param) {
    this->m_creep->EnumFunctions(callback, param);
}

// VERIFY: 6bc061ba auto
// OFFSET: 0x009C1B50
void Battlenet::CreepTendril::Instance::DoSubscriptions() {
    this->m_eventAdapter->Subscribe(Battlenet::Event::DATAGRAM_CONNECTION_INIT, this->m_tendrilId);
    this->m_eventAdapter->Subscribe(Battlenet::Event::S2_LOBBY, this->m_tendrilId);
}

// VERIFY: 5e829b17 manual
// OFFSET: 0x009C2530
void Battlenet::CreepTendril::Instance::RecvEvent(const Event::Header& header, const BSN::EncodedObject* data) {
    BSN::BSNEncodedObjectPtr<Event::Header, ::Protocol, 0> object(&header);
    object.IncRef();
    this->m_creep->RecvEvent(&object, data);
}

// VERIFY: eeea3cd7 manual
// OFFSET: 0x009C1D20
void Battlenet::CreepTendril::Instance::Resolved(uint32_t request, const Client::DNSInfo& info) {
    BSN::Hard::SizeEncoder sizeEncoder;
    ProtocolHard::TypeInfoOf<Client::DNSInfo>::Encode(info, sizeEncoder);

    BSN::Hard::Encoder encoder;
    encoder.SetData(_alloca(sizeEncoder.m_size), sizeEncoder.m_size);
    ProtocolHard::TypeInfoOf<Client::DNSInfo>::Encode(info, encoder);

    this->m_creep->Resolved(request, encoder.m_data, (encoder.m_position + 7) >> 3);
}

// VERIFY: ae8ecddb manual
// OFFSET: 0x009C1DB0
void Battlenet::CreepTendril::Instance::Connected(uint32_t connection, const IP4::AddressPort& address) {
    BSN::Hard::SizeEncoder sizeEncoder;
    ProtocolHard::TypeInfoOf<IP4::AddressPort>::Encode(address, sizeEncoder);

    BSN::Hard::Encoder encoder;
    encoder.SetData(_alloca(sizeEncoder.m_size), sizeEncoder.m_size);
    ProtocolHard::TypeInfoOf<IP4::AddressPort>::Encode(address, encoder);

    this->m_creep->Connected(connection, encoder.m_data, (encoder.m_position + 7) >> 3);
}

// VERIFY: 40115e88 manual
// OFFSET: 0x009C1B20
void Battlenet::CreepTendril::Instance::Disconnected(uint32_t connection) {
    this->m_creep->Disconnected(connection);
}

// VERIFY: d4ecf866 auto
// OFFSET: 0x009C1B30
bool Battlenet::CreepTendril::Instance::Recv(uint32_t connection, void* data, uint32_t size, uint32_t& used) {
    return this->m_creep->Recv(connection, data, size, used);
}

// VERIFY: 87b09612 manual
// OFFSET: 0x009C1E40
bool Battlenet::CreepTendril::Instance::RecvDatagram(uint32_t connection, void* data, uint32_t size, const IP4::AddressPort& address) {
    BSN::Hard::SizeEncoder sizeEncoder;
    ProtocolHard::TypeInfoOf<IP4::AddressPort>::Encode(address, sizeEncoder);

    uint32_t addressSize = sizeEncoder.m_size;
    uint8_t* buffer = static_cast<uint8_t*>(_alloca(addressSize));

    BSN::Hard::Encoder encoder;
    encoder.SetData(buffer, addressSize);
    ProtocolHard::TypeInfoOf<IP4::AddressPort>::Encode(address, encoder);

    return this->m_creep->RecvDatagram(connection, data, size, buffer, addressSize);
}

// VERIFY: 48ab284a manual
// OFFSET: 0x009C1B40
void Battlenet::CreepTendril::Instance::Process(uint32_t time) {
    this->m_creep->Process(time);
}

// VERIFY: e27bead4 manual
// OFFSET: 0x009C1B80
uint32_t Battlenet::CreepTendril::Instance::Resolve(uint32_t token, const char* hostname) {
    return this->m_adapter->Resolve(token, hostname);
}

// VERIFY: f9b90477 manual
// OFFSET: 0x009C1ED0
uint32_t Battlenet::CreepTendril::Instance::Connect(const void* address, uint32_t addressSize) {
    BSN::Hard::Decoder decoder;
    decoder.SetData(address, addressSize);

    IP4::AddressPort decoded;

    if (ProtocolHard::TypeInfoOf<IP4::AddressPort>::Decode(decoded, decoder)) {
        DebugLog("Bad address from Creep");
        return 0;
    }

    return this->m_adapter->Connect(decoded, this->m_tendrilId);
}

// VERIFY: 225d7eb8 manual
// OFFSET: 0x009C1B90
void Battlenet::CreepTendril::Instance::Disconnect(uint32_t connection) {
    this->m_adapter->Disconnect(connection);
}

// VERIFY: ba4c1f69 auto
// OFFSET: 0x009C1BA0
bool Battlenet::CreepTendril::Instance::Send(uint32_t connection, const void* data, uint32_t size) {
    return this->m_adapter->Send(connection, data, size);
}

// VERIFY: 2472e52e auto
// OFFSET: 0x009C1BB0
uint32_t Battlenet::CreepTendril::Instance::OpenDatagram(int32_t port) {
    return this->m_adapter->OpenDatagram(port, this->m_tendrilId);
}

// VERIFY: 08f27f1c manual
// OFFSET: 0x009C1BD0
void Battlenet::CreepTendril::Instance::CloseDatagram(uint32_t connection) {
    this->m_adapter->CloseDatagram(connection);
}

// VERIFY: d3253092 manual
// OFFSET: 0x009C1F50
bool Battlenet::CreepTendril::Instance::SendDatagram(uint32_t connection, const void* data, uint32_t size, const void* address, uint32_t addressSize) {
    BSN::Hard::Decoder decoder;
    decoder.SetData(address, addressSize);

    IP4::AddressPort decoded;

    if (ProtocolHard::TypeInfoOf<IP4::AddressPort>::Decode(decoded, decoder)) {
        DebugLog("Bad address from Creep");
        return false;
    }

    return this->m_adapter->SendDatagram(connection, data, size, decoded);
}

// VERIFY: a3ce1d3b auto
// OFFSET: 0x009C1BE0
uint64_t Battlenet::CreepTendril::Instance::GetCurrentTime() const {
    return this->m_adapter->GetCurrentTime();
}

// VERIFY: 2b779f09 auto
// OFFSET: 0x009C1BF0
uint64_t Battlenet::CreepTendril::Instance::GetCurrentTick() const {
    return this->m_adapter->GetCurrentTick();
}

// VERIFY: bd8c52da manual
// OFFSET: 0x009C1C00
void Battlenet::CreepTendril::Instance::LogLevel(const char* message, int32_t level) {
    this->m_adapter->LogLevel(message, level);
}

// VERIFY: e12fdecc manual
// OFFSET: 0x009C1C10
Battlenet::ApplicationFile* Battlenet::CreepTendril::Instance::OpenApplicationFile(const char* name) {
    return this->m_adapter->OpenApplicationFile(name);
}

// VERIFY: dc3d8a73 manual
// OFFSET: 0x009C1C20
void Battlenet::CreepTendril::Instance::CloseApplicationFile(ApplicationFile* file) {
    this->m_adapter->CloseApplicationFile(file);
}

// VERIFY: a4b9282f manual
// OFFSET: 0x009C2760
void Battlenet::CreepTendril::Instance::SendEvent(const BSN::EncodedObject* header, const BSN::EncodedObject* data) {
    BSN::BSNEncodedObject<Event::Header, ::Protocol> decoded;

    if (!decoded.EncodedObject::WalkAndGet(header)) {
        DebugLog("Bad event from Creep");
        return;
    }

    EventHandler handler = this->m_eventHandlers[decoded.m_data.m_eventType];
    (this->*handler)(decoded.m_data, data);
}
