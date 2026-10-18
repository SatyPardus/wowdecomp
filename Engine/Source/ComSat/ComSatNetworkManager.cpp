#include "Engine/Source/ComSat/ComSatNetwork.h"
#include "Engine/Source/ComSat/ComSatAllocator.h"
#include <cstring>
#include <new>

// VERIFY: 4e6c6ef0 manual
// OFFSET: 0x008851D0
void ComSat::Network::HMAC::Init(const uint8_t* key, uint32_t length) {
    memset(this->m_innerPad, 0x36, sizeof(this->m_innerPad));
    memset(this->m_outerPad, 0x5C, sizeof(this->m_outerPad));
    for (uint32_t i = 0; i < length; i++) {
        this->m_innerPad[i] ^= key[i];
        this->m_outerPad[i] ^= key[i];
    }
    this->m_inner.Prepare();
    this->m_inner.Process(this->m_innerPad, sizeof(this->m_innerPad));
}

// VERIFY: 1eaea78a auto
// OFFSET: 0x008852E0
uint32_t ComSat::Network::ComputeSignature(const uint8_t (&key)[16], const uint8_t* data, uint32_t size) {
    HMAC hmac;
    uint8_t innerDigest[16];
    MD5 outer;
    uint32_t digest[4];

    hmac.Init(key, sizeof(key));
    hmac.m_inner.Process(data + 4, size - 4);
    hmac.m_inner.Finish(innerDigest);
    outer.Prepare();
    outer.Process(hmac.m_outerPad, sizeof(hmac.m_outerPad));
    outer.Process(innerDigest, sizeof(innerDigest));
    outer.Finish(reinterpret_cast<uint8_t(&)[16]>(digest));
    return digest[3] ^ digest[2] ^ digest[1] ^ digest[0];
}

// VERIFY: f663e4d8 auto
// OFFSET: 0x008853D0
ComSat::Network::Manager::Manager(const ComSat::Initialization& init, ComSat::Allocator& allocator) : m_allocator(&allocator), m_sessions(allocator) {
    this->m_socket = ComSat::Network::Socket::Create(allocator);
    this->m_socket->Bind(init.m_port);
    this->m_time = init.m_time;
}

// VERIFY: bab26ca7 manual
// OFFSET: 0x00885620
ComSat::Network::Manager::~Manager() {
    if (this->m_socket) {
        ComSat::Delete(*this->m_allocator, this->m_socket);
        this->m_socket = nullptr;
    }
    while (this->m_sessions.Count()) {
        ComSat::Network::Session* session = this->m_sessions[0];
        this->m_sessions.Remove(session);
        ComSat::Delete(*this->m_allocator, session);
    }
}

// VERIFY: 0d1cf108 auto
// OFFSET: 0x00885410
void ComSat::Network::Manager::Work(uint32_t time) {
    ComSat::Network::Address from;
    ComSat::Network::Packet packet;
    uint32_t size;

    this->m_time = time;
    size = sizeof(packet);
    int32_t remaining = 10;
    if (this->m_socket->RecvFrom(from, reinterpret_cast<uint8_t*>(&packet), size)) {
        while (remaining--) {
            if (size > 7) {
                uint16_t networkId = packet.m_networkId;
                uint8_t talkerId = packet.m_talker;
                uint64_t talker = 0;
                const uint8_t (*key)[16] = nullptr;
                for (uint32_t i = 0; i < this->m_sessions.Count(); i++) {
                    ComSat::Network::Session* session = this->m_sessions[i];
                    if (session->m_networkId == networkId && session->Resolve(talkerId, from, talker, key)) {
                        session = this->m_sessions[i];
                        if (session && ComSat::Network::ComputeSignature(*key, reinterpret_cast<uint8_t*>(&packet), size) == packet.m_signature) {
                            session->Recv(talker, packet.m_data, size - 7);
                        }
                        break;
                    }
                }
            }
            // The original 3.3.5a binary also has these two receive-loop bugs
            // (0x88551a): this is an intentional correction, not decomp repair.
            // RecvFrom replaces capacity with the received size. Restore the
            // full packet budget, and don't consume an eleventh packet that
            // this Work call will never dispatch.
            if (!remaining) break;
            size = sizeof(packet);
            if (!this->m_socket->RecvFrom(from, reinterpret_cast<uint8_t*>(&packet), size)) {
                break;
            }
        }
    }
    for (uint32_t i = 0; i < this->m_sessions.Count(); i++) {
        this->m_sessions[i]->Work();
    }
}

// VERIFY: 2076d25f auto
// OFFSET: 0x00885560
void ComSat::Network::Manager::DestroySession(ComSat::Network::Session* session) {
    this->m_sessions.Remove(session);
    ComSat::Delete(*this->m_allocator, session);
}

// VERIFY: ddf7f762 manual
// OFFSET: 0x00885590
bool ComSat::Network::Manager::Send(uint16_t networkId, uint8_t talker, const uint8_t (&key)[16], const ComSat::Network::Address& address, const uint8_t* data, uint32_t size) {
    ComSat::Network::Packet packet;
    if (size > sizeof(packet.m_data) || (!data && size)) {
        return false;
    }
    uint32_t length = size + 7;
    packet.m_talker = talker;
    packet.m_networkId = networkId;
    if (size) memcpy(packet.m_data, data, size);
    packet.m_signature = ComSat::Network::ComputeSignature(key, reinterpret_cast<uint8_t*>(&packet), length);
    this->m_socket->SendTo(address, reinterpret_cast<uint8_t*>(&packet), length);
    return true;
}

// VERIFY: f240b463 manual
// OFFSET: 0x008856D0
ComSat::Network::Session* ComSat::Network::Manager::CreateSession(uint64_t id, ComSat::Network::Session::Callback& callback) {
    ComSat::Network::Session* session = new (this->m_allocator->Alloc(sizeof(ComSat::Network::Session))) ComSat::Network::Session(*this, id, callback);
    this->m_sessions.PushBack(session);
    return session;
}
