#include "Engine/Source/ComSat/ComSatManager.h"
#include "Engine/Source/ComSat/ComSatAllocator.h"
#include "Engine/Source/ComSat/ComSatBitPacker.h"
#include <cstring>
#include <new>

// VERIFY: daaf90f8 auto
// OFFSET: 0x0087F900
ComSat::Interface::~Interface() {
}

// INLINED
ComSat::Manager::LocalTalker::LocalTalker(ComSat::Allocator& allocator) : m_guid(0), m_session(nullptr), m_sessions(allocator) {
}

// VERIFY: 03a47a69 auto
// OFFSET: 0x0087F990
void ComSat::Manager::Work(uint32_t elapsed) {
    this->m_networkManager->Work(elapsed);
    this->m_soundManager->Work(elapsed);
}

// VERIFY: 36e8ca55 auto
// OFFSET: 0x0087F9C0
void ComSat::Manager::CallbackClear() {
    this->m_callback = nullptr;
}

// VERIFY: 814c3774 auto
// OFFSET: 0x0095BE50
void ComSat::Manager::CallbackSet(ComSat::Interface::Callback& callback) {
    this->m_callback = &callback;
}

// VERIFY: e4968ea4 auto
// OFFSET: 0x0087F9D0
bool ComSat::Manager::LocalTalkerChangeOutputDevice(uint8_t localTalker, int32_t device) {
    return this->m_soundManager->LocalTalkerChangeOutputDevice(localTalker, device);
}

// VERIFY: c0eb6067 auto
// OFFSET: 0x0087F9E0
bool ComSat::Manager::LocalTalkerChangeCaptureDevice(uint8_t localTalker, int32_t device) {
    return this->m_soundManager->LocalTalkerChangeCaptureDevice(localTalker, device);
}

// VERIFY: 2b33b977 auto
// OFFSET: 0x0087F9F0
int32_t ComSat::Manager::LocalTalkerGetCurrentOutputDeviceIndex(uint8_t localTalker) {
    return this->m_soundManager->LocalTalkerGetCurrentOutputDeviceIndex(localTalker);
}

// VERIFY: 6fedd7b9 auto
// OFFSET: 0x0087FA00
int32_t ComSat::Manager::LocalTalkerGetCurrentCaptureDeviceIndex(uint8_t localTalker) {
    return this->m_soundManager->LocalTalkerGetCurrentCaptureDeviceIndex(localTalker);
}

// VERIFY: 87fe7308 auto
// OFFSET: 0x0087FA10
bool ComSat::Manager::LocalTalkerEnable(uint8_t localTalker, bool enable) {
    return this->m_soundManager->LocalTalkerEnable(localTalker, enable);
}

// VERIFY: 030f3ac2 auto
// OFFSET: 0x0087FA20
bool ComSat::Manager::LocalTalkerThreshold(uint8_t localTalker, float threshold) {
    if (threshold < 0.0f || threshold > 1.0f) {
        return false;
    }
    return this->m_soundManager->LocalTalkerThreshold(localTalker, threshold);
}

// VERIFY: fc07e902 auto
// OFFSET: 0x0087FA60
bool ComSat::Manager::LocalTalkerEnableVoiceLevelCallback(uint8_t localTalker, bool enable) {
    if (localTalker >= this->m_maxLocalPlayers) {
        return false;
    }
    this->m_soundManager->LocalTalkerEnableVoiceLevelCallback(localTalker, enable);
    return true;
}

// VERIFY: 6ed3bdce auto
// OFFSET: 0x0087FA90
bool ComSat::Manager::LocalTalkerSetRemoteTalkerVolume(uint8_t localTalker, uint64_t talker, float volume) {
    return this->m_soundManager->LocalTalkerSetRemoteTalkerVolume(localTalker, talker, volume);
}

// VERIFY: 6d300b56 auto
// OFFSET: 0x0087FAC0
bool ComSat::Manager::LocalTalkerSetSessionVolume(uint8_t localTalker, uint64_t session, float volume) {
    return this->m_soundManager->LocalTalkerSetSessionVolume(localTalker, session, volume);
}

// VERIFY: ff4b4a0e auto
// OFFSET: 0x0087FAF0
int32_t ComSat::Manager::SessionNumMembers(uint64_t session) {
    return this->m_soundManager->SessionNumMembers(session);
}

// VERIFY: 9e7d64e0 auto
// OFFSET: 0x0087FB10
bool ComSat::Manager::SessionGetTalkerByIndex(uint64_t session, int32_t index, uint64_t& talker, uint8_t& localTalker) {
    if (!this->m_soundManager->SessionGetTalkerByIndex(session, index, talker, localTalker)) {
        return false;
    }
    if (localTalker != 0xFF) {
        if (localTalker >= this->m_maxLocalPlayers) {
            return false;
        }
        uint64_t guid = this->m_localTalkers[localTalker]->m_guid;
        if (!guid) {
            return false;
        }
        talker = guid;
    }
    return true;
}

// VERIFY: b46ff538 auto
// OFFSET: 0x0087FB70
bool ComSat::Manager::SessionSetTalkerPriority(uint64_t session, uint64_t talker, uint8_t priority) {
    return this->m_soundManager->SetPlaybackPriorityForPlayerInSession(session, talker, priority);
}

// VERIFY: 1b462178 auto
// OFFSET: 0x0087FBA0
bool ComSat::Manager::LocalTalkerSetSessionPriority(uint8_t localTalker, uint64_t session, uint8_t priority) {
    return this->m_soundManager->SetPlaybackPriorityForSession(session, localTalker, priority);
}

// VERIFY: 3b40d07f manual
// OFFSET: 0x0087FBC0
bool ComSat::Manager::IsTalkerMuted(uint8_t localTalker, uint64_t talker) {
    if (localTalker < this->m_maxLocalPlayers) {
        return this->m_soundManager->IsRemoteTalkerMuted(localTalker, talker);
    }
    return false;
}

// VERIFY: d7c8ef33 auto
// OFFSET: 0x0087FBF0
bool ComSat::Manager::MuteTalker(uint8_t localTalker, uint64_t talker, ComSat::MuteSetting setting) {
    return this->m_soundManager->SetTalkerMuted(localTalker, talker, setting);
}

// VERIFY: ed19ad38 auto
// OFFSET: 0x0087FC10
bool ComSat::Manager::MuteSession(uint8_t localTalker, uint64_t session, ComSat::MuteSetting setting) {
    if (localTalker < this->m_maxLocalPlayers && this->m_localTalkers[localTalker]->m_guid) {
        return this->m_soundManager->SetSessionMuted(localTalker, session, setting);
    }
    return false;
}

// VERIFY: 9f29457b auto
// OFFSET: 0x0087FC50
bool ComSat::Manager::IsSessionMuted(uint8_t localTalker, uint64_t session) {
    return this->m_soundManager->IsSessionMuted(localTalker, session);
}

// VERIFY: cfd75ff0 manual
// OFFSET: 0x0087FC70
uint8_t ComSat::Manager::GetLocalTalkerIndex(uint64_t talker) {
    for (uint32_t i = 0; i < this->m_maxLocalPlayers; i++) {
        if (this->m_localTalkers[i]->m_guid == talker) {
            return i;
        }
    }
    return 0xFF;
}

// VERIFY: baa2befb auto
// OFFSET: 0x0087FCB0
bool ComSat::Manager::IsTalkerTalkingInSession(uint64_t session, uint8_t localTalker, uint64_t talker) {
    if (this->GetLocalTalkerIndex(talker) == 0xFF) {
        return this->m_soundManager->IsRemoteTalkerTalkingInSession(session, localTalker, talker);
    }
    return this->m_soundManager->IsLocalPlayerTalking(localTalker);
}

// VERIFY: b10e168c auto
// OFFSET: 0x0087FD00
bool ComSat::Manager::DoesTalkerHaveHeadset(uint64_t talker) {
    return true;
}

// VERIFY: b0dbd72a auto
// OFFSET: 0x008873A0
bool ComSat::Manager::DoesLocalPlayerHaveHeadset(uint8_t localTalker) {
    return true;
}

// VERIFY: 6512fb0f auto
// OFFSET: 0x0087FD10
int32_t ComSat::Manager::EnumerateOutputDevices(ComSat::DeviceInfo* devices, int32_t maxDevices) {
    return this->m_soundManager->EnumerateOutputDevices(devices, maxDevices);
}

// VERIFY: 876fcfd1 auto
// OFFSET: 0x0087FD20
int32_t ComSat::Manager::EnumerateCaptureDevices(ComSat::DeviceInfo* devices, int32_t maxDevices) {
    return this->m_soundManager->EnumerateCaptureDevices(devices, maxDevices);
}

// VERIFY: 6221267c manual
// OFFSET: 0x0087FD30
bool ComSat::Manager::UnpackStableBitrateData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size) {
    bool reliable = false;
    uint32_t offset = 2;
    do {
        uint8_t length = data[offset];
        if (!length) {
            reliable = true;
        }
        offset += length + 1;
    } while (offset < size);
    ComSat::Sound::Manager::VoicePacket packet;
    packet.m_sequence = *reinterpret_cast<const uint16_t*>(data);
    packet.m_bits = this->m_soundManager->GetMaxEncodedFrameLength();
    uint32_t bytes = static_cast<uint32_t>(this->m_soundManager->GetMaxEncodedFrameLength()) >> 3;
    int32_t frames = 0;
    uint32_t position = 2;
    if (size > 2) {
        do {
            if (size - position < bytes) {
                break;
            }
            memcpy(packet.m_data, data + position, bytes);
            this->m_soundManager->SubmitIncomingVoicePacket(session, talker, reinterpret_cast<uint8_t*>(&packet), bytes + 4, reliable);
            packet.m_sequence++;
            frames++;
            position += bytes;
        } while (position < size);
        if (frames > 0) {
            this->m_soundManager->SubmitIncomingFrameTimeData(talker, packet.m_sequence);
        }
    }
    return true;
}

// VERIFY: 866f6aa1 manual
// OFFSET: 0x0087FE30
bool ComSat::Manager::UnpackVariableBitrateData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size) {
    uint16_t sequence = data[0] | (data[1] << 8);
    uint32_t position = 3;
    uint8_t header = data[2];
    bool reliable = header >> 7;
    uint8_t count = header & 0x7F;
    if (count > 20) {
        count = 20;
    }
    ComSat::Sound::Manager::VoicePacket packet;
    uint32_t lengths[20];
    for (int32_t i = 0; i < count; i++) {
        lengths[i] = data[position];
        lengths[i] += this->m_soundManager->GetMinEncodedFrameLength();
        position++;
    }
    ComSat::BitUnpacker unpacker(&data[position], 8 * (size - position));
    for (int32_t i = 0; i < count; i++) {
        uint32_t bits = lengths[i];
        packet.m_sequence = sequence;
        packet.m_bits = bits;
        if (bits && unpacker.UnpackBits(packet.m_data, bits)) {
            uint32_t length = (bits >> 3) + 4;
            if (bits & 7) {
                length = (bits >> 3) + 5;
            }
            this->m_soundManager->SubmitIncomingVoicePacket(session, talker, reinterpret_cast<uint8_t*>(&packet), length, reliable);
        }
        sequence++;
    }
    if (count) {
        this->m_soundManager->SubmitIncomingFrameTimeData(talker, sequence - 1);
    }
    return true;
}

// VERIFY: 796aacf3 manual
// OFFSET: 0x0087FF70
bool ComSat::Manager::UnpackBitpackedVariableBitrateData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size) {
    ComSat::Sound::Manager::VoicePacket packet;
    packet.m_sequence = *reinterpret_cast<const uint16_t*>(data);
    ComSat::BitUnpacker unpacker(data + 2, 8 * size - 16);
    packet.m_bits = 8 * size - 16;
    if (!unpacker.UnpackBits(packet.m_data, packet.m_bits)) {
        return false;
    }
    this->m_soundManager->SubmitIncomingVoicePacket(session, talker, reinterpret_cast<uint8_t*>(&packet), 132, false);
    this->m_soundManager->SubmitIncomingFrameTimeData(talker, packet.m_sequence);
    return true;
}

// VERIFY: 70cd887e auto
// OFFSET: 0x00880030
void ComSat::Manager::IncomingVoiceData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size) {
    if (this->m_soundManager->IsUsingVariableBitrateCodec()) {
        if (this->m_soundManager->CanBitpackEncodedData()) {
            this->UnpackBitpackedVariableBitrateData(session, talker, data, size);
        } else {
            this->UnpackVariableBitrateData(session, talker, data, size);
        }
    } else {
        this->UnpackStableBitrateData(session, talker, data, size);
    }
}

// VERIFY: e0a023cd manual
// OFFSET: 0x008800B0
bool ComSat::Manager::PackBitpackedVariableBitrateData(uint8_t player, uint8_t* buffer, int32_t& size) {
    size = 0;
    if (player >= this->m_maxLocalPlayers) {
        return false;
    }
    LocalTalker* localTalker = this->m_localTalkers[player];
    if (!localTalker || !localTalker->m_frameCount) {
        return false;
    }
    *reinterpret_cast<uint16_t*>(buffer) = localTalker->m_frames[0].m_sequence;
    size += 2;
    ComSat::BitPacker packer(buffer + size, 2560 - size);
    for (int32_t i = 0; i < static_cast<int32_t>(localTalker->m_frameCount); i++) {
        packer.PackBits(localTalker->m_frames[i].m_data, localTalker->m_frames[i].m_bits);
    }
    size += packer.Finalize();
    return true;
}

// VERIFY: 16e90e0b manual
// OFFSET: 0x00880160
bool ComSat::Manager::PackVariableBitrateData(uint8_t player, uint8_t* buffer, int32_t& size) {
    size = 0;
    if (player >= this->m_maxLocalPlayers) {
        return false;
    }
    LocalTalker* localTalker = this->m_localTalkers[player];
    if (!localTalker || !localTalker->m_frameCount) {
        return false;
    }
    int32_t minimum = this->m_soundManager->GetMinEncodedFrameLength();
    *reinterpret_cast<uint16_t*>(buffer + size) = localTalker->m_frames[0].m_sequence;
    size += 2;
    buffer[size] = localTalker->m_frameCount;
    if (localTalker->m_time < 100) {
        buffer[size] |= 0x80;
    }
    size++;
    for (int32_t i = 0; i < static_cast<int32_t>(localTalker->m_frameCount); i++) {
        int32_t length = localTalker->m_frames[i].m_bits - minimum;
        if (length < 0) {
            length = 0;
        } else if (length > 255) {
            length = 255;
        }
        buffer[size] = length;
        size++;
    }
    ComSat::BitPacker packer(buffer + size, 2560 - size);
    for (int32_t i = 0; i < static_cast<int32_t>(localTalker->m_frameCount); i++) {
        packer.PackBits(localTalker->m_frames[i].m_data, localTalker->m_frames[i].m_bits);
    }
    size += packer.Finalize();
    return true;
}

// VERIFY: 87720e76 manual
// OFFSET: 0x008802A0
bool ComSat::Manager::PackStableBitrateData(uint8_t player, uint8_t* buffer, int32_t& size) {
    size = 0;
    if (player >= this->m_maxLocalPlayers) {
        return false;
    }
    LocalTalker* localTalker = this->m_localTalkers[player];
    if (!localTalker || !localTalker->m_frameCount) {
        return false;
    }
    *reinterpret_cast<uint16_t*>(buffer) = localTalker->m_frames[0].m_sequence;
    size += 2;
    uint32_t bytes = static_cast<uint32_t>(this->m_soundManager->GetMaxEncodedFrameLength()) >> 3;
    if (this->m_soundManager->GetMaxEncodedFrameLength() & 7) {
        bytes++;
    }
    for (uint32_t i = 0; i < localTalker->m_frameCount; i++) {
        memcpy(buffer + size, localTalker->m_frames[i].m_data, bytes);
        size += bytes;
    }
    if (localTalker->m_time < 100) {
        buffer[size++] = 0;
    }
    return true;
}

// VERIFY: 6ca2c7b2 auto
// OFFSET: 0x00880380
void ComSat::Manager::SendVoiceDataToCurrentSession(uint8_t player) {
    if (player >= this->m_maxLocalPlayers) {
        return;
    }
    LocalTalker* localTalker = this->m_localTalkers[player];
    if (!localTalker || !localTalker->m_frameCount) {
        return;
    }
    if (localTalker->m_session) {
        uint8_t buffer[2560];
        int32_t size = 0;
        bool packed;
        if (this->m_soundManager->IsUsingVariableBitrateCodec()) {
            if (this->m_soundManager->CanBitpackEncodedData()) {
                packed = this->PackBitpackedVariableBitrateData(player, buffer, size);
            } else {
                packed = this->PackVariableBitrateData(player, buffer, size);
            }
        } else {
            packed = this->PackStableBitrateData(player, buffer, size);
        }
        if (!packed) {
            return;
        }
        localTalker->m_session->Send(localTalker->m_guid, buffer, size);
    }
    localTalker->m_frameCount = 0;
    localTalker->m_time = 0;
}

// VERIFY: 42ef3c1d manual
// OFFSET: 0x00880450
void ComSat::Manager::LocalChatDataReady(uint8_t player, uint8_t* data, uint32_t size, uint32_t time) {
    if (player >= this->m_maxLocalPlayers) {
        return;
    }
    LocalTalker* localTalker = this->m_localTalkers[player];
    if (!localTalker->m_guid) {
        return;
    }
    if (!size) {
        if (localTalker->m_talking) {
            localTalker->m_talking = false;
            if (this->m_callback) {
                this->m_callback->NotifyLocalTalkerStop(player);
            }
        }
        this->SendVoiceDataToCurrentSession(player);
        return;
    }
    if (!localTalker->m_talking) {
        localTalker->m_talking = true;
        if (this->m_callback) {
            this->m_callback->NotifyLocalTalkerStart(player);
        }
    }
    if (size <= 128) {
        LocalTalker* target = this->m_localTalkers[player];
        target->m_frames[target->m_frameCount].m_size = size;
        memcpy(&target->m_frames[target->m_frameCount], data, size);
        target->m_frameCount++;
        target->m_time += time;
        if (target->m_time >= 100) {
            this->SendVoiceDataToCurrentSession(player);
        }
    }
}

// VERIFY: aaea3c06 manual
// OFFSET: 0x00880550
void ComSat::Manager::RemotePlayerTalkNotifyStart(uint8_t player, uint64_t talker, uint64_t session) {
    if (this->m_callback) {
        this->m_callback->NotifyTalkerStart(player, session, talker);
    }
}

// VERIFY: 39fa1c5d manual
// OFFSET: 0x00880580
void ComSat::Manager::RemotePlayerTalkNotifyStop(uint8_t player, uint64_t talker, uint64_t session) {
    if (this->m_callback) {
        this->m_callback->NotifyTalkerStop(player, session, talker);
    }
}

// VERIFY: 582ba733 manual
// OFFSET: 0x008805B0
void ComSat::Manager::LocalTalkerVoiceLevel(uint8_t player, float level, bool talking) {
    if (this->m_callback) {
        this->m_callback->NotifyLocalTalkerVoiceLevel(player, level, talking);
    }
}

// VERIFY: 0641685b auto
// OFFSET: 0x008805E0
void ComSat::Manager::DeviceError(uint8_t player) {
    if (this->m_callback && player < this->m_maxLocalPlayers) {
        LocalTalker* localTalker = this->m_localTalkers[player];
        if (localTalker->m_guid && localTalker->m_deviceError != true) {
            localTalker->m_deviceError = true;
            this->m_callback->NotifyDeviceError(player);
        }
    }
}

// VERIFY: de61f2c6 auto
// OFFSET: 0x00653A10
void ComSat::Manager::MicrophoneStatusUpdate(uint8_t player, ComSat::Sound::HeadsetDeviceStatus status) {
}

// VERIFY: f7010444 auto
// OFFSET: 0x00880630
void ComSat::Manager::DebugSetSequenceDelay(int32_t delay) {
    this->m_soundManager->DebugSetSequenceDelay(delay);
}

// VERIFY: 4f0e7df6 auto
// OFFSET: 0x00880640
void ComSat::Manager::DebugSetSoundOutputDelay(uint8_t localTalker, int32_t delay) {
    this->m_soundManager->DebugSetSoundOutputDelay(localTalker, delay);
}

// VERIFY: 51a7470f auto
// OFFSET: 0x00880710
bool ComSat::Manager::LocalTalkerCreate(uint8_t localTalker, uint64_t talker, float threshold, int32_t outputDevice, int32_t captureDevice) {
    LocalTalker* local = this->m_localTalkers[localTalker];
    if (!local) {
        return false;
    }
    local->m_guid = talker;
    local->m_session = nullptr;
    local->m_frameCount = 0;
    local->m_time = 0;
    local->m_deviceError = false;
    local->m_sessions.m_used = 0;
    local->m_talking = false;
    return this->m_soundManager->InitLocalPlayer(localTalker, threshold, outputDevice, captureDevice);
}

// VERIFY: ae78af2b auto
// OFFSET: 0x00880780
bool ComSat::Manager::LocalTalkerDestroy(uint8_t localTalker) {
    LocalTalker* local = this->m_localTalkers[localTalker];
    if (!local || !local->m_guid) {
        return true;
    }
    uint32_t count = local->m_sessions.Count();
    for (uint32_t i = 0; i < count; i++) {
        this->SessionRemove(local->m_sessions[i], local->m_guid);
    }
    bool result = this->m_soundManager->DestroyLocalPlayer(localTalker);
    local->m_guid = 0;
    return result;
}

// VERIFY: 4f3fb2d5 manual
// OFFSET: 0x00880810
bool ComSat::Manager::SessionDestroy(uint64_t session) {
    for (uint8_t i = 0; i < this->m_maxLocalPlayers; i++) {
        LocalTalker* local = this->m_localTalkers[i];
        if (local->m_guid) {
            ComSat::Session* current = local->m_session;
            if (current && current->m_id == session) {
                local->m_session = nullptr;
                local->m_frameCount = 0;
                local->m_time = 0;
            }
        }
    }
    uint32_t count = this->m_sessions.Count();
    uint32_t index = 0;
    if (!count) {
        return false;
    }
    while (this->m_sessions[index]->m_id != session) {
        if (++index >= count) {
            return false;
        }
    }
    ComSat::Session* removed = this->m_sessions[index];
    this->m_sessions.RemoveAt(index);
    if (!removed) {
        return false;
    }
    this->m_networkManager->DestroySession(removed->GetNetworkSession());
    ComSat::Delete(*this->m_allocator, removed);
    this->m_soundManager->SessionDestroy(session);
    return true;
}

// VERIFY: 4bc9f19f manual
// OFFSET: 0x00880920
ComSat::Session* ComSat::Manager::FindSession(uint64_t id) {
    for (uint32_t i = 0; i < this->m_sessions.Count(); i++) {
        if (this->m_sessions[i]->m_id == id) {
            return this->m_sessions[i];
        }
    }
    return nullptr;
}

// VERIFY: 74ff654b manual
// OFFSET: 0x00880980
bool ComSat::Manager::SessionAddRemote(uint64_t session, uint64_t talker, uint8_t networkId, const uint8_t (&key)[16], const char* address) {
    ComSat::Session* current = this->FindSession(session);
    if (!current) {
        return false;
    }
    for (uint32_t i = 0; i < this->m_maxLocalPlayers; i++) {
        if (this->m_localTalkers[i]->m_guid == talker) {
            return false;
        }
    }
    ComSat::Network::Address remote;
    remote.SetAddress(address);
    if (!remote.GetPort()) {
        remote.SetPort(this->m_defaultPort);
    }
    current->Add(talker, networkId, key, remote);
    return this->m_soundManager->RegisterRemoteTalkerForSession(session, talker);
}

// VERIFY: a80b2754 manual
// OFFSET: 0x00880A50
bool ComSat::Manager::SessionSetTalkerNetworkIdLocal(uint64_t session, uint64_t talker, uint8_t networkId) {
    ComSat::Session* current = this->FindSession(session);
    if (!current) {
        return false;
    }
    ComSat::Network::Session* networkSession = current->GetNetworkSession();
    if (!networkSession) {
        return false;
    }
    networkSession->SetTalkerNetworkIdLocal(talker, networkId);
    return true;
}

// VERIFY: 07b80142 manual
// OFFSET: 0x00880A90
bool ComSat::Manager::SessionSetTalkerNetworkIdRemote(uint64_t session, uint64_t talker, uint8_t networkId) {
    ComSat::Session* current = this->FindSession(session);
    if (!current) {
        return false;
    }
    ComSat::Network::Session* networkSession = current->GetNetworkSession();
    if (!networkSession) {
        return false;
    }
    networkSession->SetTalkerNetworkIdRemote(talker, networkId);
    return true;
}

// VERIFY: bd6b44ca auto
// OFFSET: 0x00880BB0
ComSat::Manager::Manager(ComSat::Sound::Manager& soundManager, ComSat::Network::Manager& networkManager, const ComSat::Initialization& init, ComSat::Allocator& allocator) : m_sessions(allocator) {
    this->m_maxLocalPlayers = init.m_maxLocalPlayers;
    this->unused_0010 = init.unk_0004;
    this->m_soundManager = &soundManager;
    this->m_networkManager = &networkManager;
    this->m_allocator = &allocator;
    this->m_defaultPort = init.m_port;
    this->m_localTalkers = nullptr;
    this->m_callback = nullptr;
    this->m_localTalkers = static_cast<LocalTalker**>(this->m_allocator->Alloc(sizeof(LocalTalker*) * this->m_maxLocalPlayers));
    for (uint32_t i = 0; i < this->m_maxLocalPlayers; i++) {
        this->m_localTalkers[i] = new (this->m_allocator->Alloc(sizeof(LocalTalker))) LocalTalker(*this->m_allocator);
    }
    soundManager.SetCallback(this);
}

// VERIFY: 27b35473 manual
// OFFSET: [0x00881150, 0x00881250]
ComSat::Manager::~Manager() {
    for (uint32_t i = 0; i < this->m_maxLocalPlayers; i++) {
        if (this->m_localTalkers[i]) {
            ComSat::Delete(*this->m_allocator, this->m_localTalkers[i]);
            this->m_localTalkers[i] = nullptr;
        }
    }
    if (this->m_localTalkers) {
        this->m_allocator->Free(this->m_localTalkers);
    }
    for (uint32_t i = 0; i < this->m_sessions.Count(); i++) {
        ComSat::Delete(*this->m_allocator, this->m_sessions[i]);
    }
}

// VERIFY: 22930931 manual
// OFFSET: 0x00880CB0
bool ComSat::Manager::SessionSetNetworkId(uint64_t session, uint16_t networkId) {
    ComSat::Session* current = this->FindSession(session);
    if (!current) {
        return false;
    }
    ComSat::Network::Session* networkSession = current->GetNetworkSession();
    if (!networkSession) {
        return false;
    }
    networkSession->SetNetworkId(networkId);
    return true;
}

// VERIFY: 5a142082 manual
// OFFSET: 0x00880CF0
bool ComSat::Manager::LocalTalkerSelectSession(uint8_t localTalker, uint64_t session) {
    if (localTalker >= this->m_maxLocalPlayers) {
        return false;
    }
    LocalTalker* local = this->m_localTalkers[localTalker];
    if (local->m_session && local->m_session->m_id == session) {
        return true;
    }
    this->SendVoiceDataToCurrentSession(localTalker);
    if (!session) {
        local->m_session = nullptr;
        return true;
    }
    local->m_session = this->FindSession(session);
    return local->m_session != nullptr;
}

// VERIFY: a74d7eee manual
// OFFSET: 0x00880D70
bool ComSat::Manager::SessionRemove(uint64_t session, uint64_t talker) {
    ComSat::Session* current = this->FindSession(session);
    if (!current) {
        return false;
    }
    current->Remove(talker);
    if (!this->m_localTalkers) {
        return false;
    }
    uint8_t localTalker = 0xFF;
    for (uint32_t i = 0; i < this->m_maxLocalPlayers; i++) {
        if (this->m_localTalkers[i]->m_guid == talker) {
            localTalker = i;
        }
    }
    if (localTalker == 0xFF) {
        return this->m_soundManager->UnregisterRemoteTalkerFromSession(session, talker);
    }
    bool result = this->m_soundManager->UnregisterLocalTalkerFromSession(session, localTalker);
    bool unused = true;
    for (uint32_t i = 0; i < this->m_maxLocalPlayers; i++) {
        LocalTalker* local = this->m_localTalkers[i];
        if (local->m_guid) {
            if (local->m_guid == talker) {
                local->m_sessions.Remove(session);
            } else {
                uint32_t index = 0;
                if (local->m_sessions.Find(session, index)) {
                    unused = false;
                }
            }
        }
    }
    if (unused) {
        this->SessionDestroy(session);
    }
    return result;
}

// VERIFY: 0ca1a41a auto
// OFFSET: 0x00880FC0
bool ComSat::Manager::SessionCreate(uint64_t session) {
    if (this->FindSession(session)) {
        return false;
    }
    void* memory = this->m_allocator->Alloc(sizeof(ComSat::Session));
    if (!memory) {
        return false;
    }
    ComSat::Session* created = new (memory) ComSat::Session(*this, session, *this->m_allocator);
    if (!created) {
        return false;
    }
    ComSat::Network::Session* networkSession = this->m_networkManager->CreateSession(session, *created);
    if (!networkSession) {
        ComSat::Delete(*this->m_allocator, created);
        return false;
    }
    this->m_sessions.PushBack(created);
    created->SetNetworkSession(networkSession);
    this->m_soundManager->SessionCreate(session);
    return true;
}

// VERIFY: 076cfaa1 manual
// OFFSET: 0x00881070
bool ComSat::Manager::SessionAddLocal(uint64_t session, uint8_t localTalker, uint64_t talker) {
    ComSat::Session* current = this->FindSession(session);
    if (!current) {
        return false;
    }
    if (localTalker >= this->m_maxLocalPlayers) {
        return false;
    }
    LocalTalker* local = this->m_localTalkers[localTalker];
    if (!local->m_guid) {
        local->m_guid = talker;
    }
    ComSat::Vector<uint64_t>& sessions = this->m_localTalkers[localTalker]->m_sessions;
    uint32_t index = 0;
    if (!sessions.Find(session, index)) {
        sessions.PushBack(session);
    }
    current->LocalAdd(talker);
    bool result = this->m_soundManager->RegisterLocalTalkerForSession(session, localTalker);
    if (result && !this->m_localTalkers[localTalker]->m_session) {
        this->LocalTalkerSelectSession(localTalker, session);
    }
    return result;
}
