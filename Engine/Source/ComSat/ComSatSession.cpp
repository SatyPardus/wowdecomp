#include "Engine/Source/ComSat/ComSatSession.h"

// VERIFY: 0de96b64 auto
// OFFSET: 0x0087F960
ComSat::SessionCallback::~SessionCallback() {
}

// VERIFY: dd0145f9 auto
// OFFSET: 0x00885AD0
void ComSat::Session::SetNetworkSession(ComSat::Network::Session* session) {
    this->m_networkSession = session;
}

// VERIFY: 7100e0e3 auto
// OFFSET: 0x00885AE0
ComSat::Network::Session* ComSat::Session::GetNetworkSession() {
    return this->m_networkSession;
}

// VERIFY: 9e7e52b0 auto
// OFFSET: 0x00885AF0
bool ComSat::Session::LocalAdd(uint64_t talker) {
    return this->m_networkSession && this->m_networkSession->LocalAdd(talker);
}

// VERIFY: 944f9c23 auto
// OFFSET: 0x00885B20
bool ComSat::Session::Add(uint64_t talker, uint8_t networkId, const uint8_t (&key)[16], const ComSat::Network::Address& address) {
    return this->m_networkSession && this->m_networkSession->Add(talker, networkId, key, address);
}

// VERIFY: b8411e51 auto
// OFFSET: 0x00885B60
void ComSat::Session::Remove(uint64_t talker) {
    if (this->m_networkSession) {
        this->m_networkSession->Remove(talker);
    }
}

// VERIFY: 6f4715dd auto
// OFFSET: 0x00885B80
bool ComSat::Session::Send(uint64_t talker, const uint8_t* data, uint32_t size) {
    return this->m_networkSession && this->m_networkSession->Send(talker, data, size);
}

// VERIFY: 8429dcf3 auto
// OFFSET: 0x00885BC0
void ComSat::Session::Recv(uint64_t talker, const uint8_t* data, uint32_t size) {
    if (data && size) {
        this->m_callback->IncomingVoiceData(this->m_id, talker, data, size);
    }
}

// VERIFY: dedccfab auto
// OFFSET: 0x00885C00
ComSat::Session::Session(ComSat::SessionCallback& callback, uint64_t id, ComSat::Allocator& allocator) {
    this->m_callback = &callback;
    this->m_id = id;
    this->m_networkSession = nullptr;
    this->m_allocator = &allocator;
}

// VERIFY: 0eb3a7af manual
// OFFSET: 0x00885C30
ComSat::Session::~Session() {
}
