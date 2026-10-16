#include "Engine/Source/ComSat/ComSatSoundIOSoundEngine.h"
#include "Engine/Source/ComSat/ComSatAllocator.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include <new>

// VERIFY: ed1c5c8c manual
// OFFSET: 0x00635DD0
ComSat::Sound::IOCallback::~IOCallback() {
}

// VERIFY: 2b11346e manual
// OFFSET: 0x00887360
ComSat::Sound::IO::~IO() {
}

// VERIFY: f1d0cb09 manual
// OFFSET: 0x00887B10
ComSat::Sound::IO* ComSat::Sound::IO::Create(const ComSat::Initialization& init, ComSat::Sound::IOCallback& callback, ComSat::Allocator& allocator) {
    return new (allocator.Alloc(sizeof(ComSat::Sound::IOSoundEngine))) ComSat::Sound::IOSoundEngine(init, callback, allocator);
}

// INLINED
ComSat::Sound::CaptureChannel::CaptureChannel(ComSat::Allocator& allocator) : m_allocator(&allocator) {
}

// VERIFY: b21ec2d3 auto
// OFFSET: 0x008879F0
ComSat::Sound::CaptureChannel::~CaptureChannel() {
}

// VERIFY: 78652afa auto
// OFFSET: 0x00887810
void ComSat::Sound::CaptureChannel::ProcessData(int16_t* data, uint32_t samples) {
    SI2::VoiceChat_AmplifyMicrophoneInput(data, samples);
    this->m_buffer.Write(data, samples);
}

// INLINED
ComSat::Sound::OutputChannel::OutputChannel(ComSat::Allocator& allocator) : m_allocator(&allocator) {
}

// VERIFY: 5311437b auto
// OFFSET: 0x00887DB0
ComSat::Sound::OutputChannel::~OutputChannel() {
}

// VERIFY: 471a153b manual
// OFFSET: 0x00887840
bool ComSat::Sound::OutputChannel::GetSamples(uint32_t position, uint32_t count, float* buffer) {
    return this->m_buffer.Read(buffer, count) != 0;
}

// VERIFY: 7b6c81ad auto
// OFFSET: 0x00887B50
void ComSat::Sound::OutputChannel::Delete() {
    ComSat::Delete(*this->m_allocator, this);
}

// VERIFY: fb0351ec manual
// OFFSET: 0x00887A10
ComSat::Sound::IOSoundEngine::IOSoundEngine(const ComSat::Initialization& init, ComSat::Sound::IOCallback& callback, ComSat::Allocator& allocator) : m_allocator(&allocator), m_callback(&callback), m_captureChannel(allocator) {
    this->m_outputDevice = 0;
    this->m_captureDevice = 0;
    this->m_channels.SetCount(init.m_maxChannels);
    for (uint32_t i = 0; i < this->m_channels.Count(); i++) {
        this->m_channels[i] = nullptr;
    }
}

// VERIFY: 9d711460 manual
// OFFSET: 0x00887AC0
ComSat::Sound::IOSoundEngine::~IOSoundEngine() {
    this->ShutDownCapture();
}

// VERIFY: eaf35b9b auto
// OFFSET: 0x008A1420
bool ComSat::Sound::IOSoundEngine::Init() {
    return true;
}

// VERIFY: 5996f1c6 auto
// OFFSET: 0x00632050
void ComSat::Sound::IOSoundEngine::Work(uint32_t time) {
}

// VERIFY: 27bc5b3f auto
// OFFSET: 0x005EEB70
void ComSat::Sound::IOSoundEngine::Shutdown() {
}

// VERIFY: f9893773 auto
// OFFSET: 0x008873A0
bool ComSat::Sound::IOSoundEngine::DestroyLocalPlayer(uint8_t player) {
    return true;
}

// VERIFY: 2afc43cd auto
// OFFSET: 0x008873B0
int32_t ComSat::Sound::IOSoundEngine::GetInputBlockSize(uint8_t player) {
    return this->m_samplesPerFrame * this->m_bitsPerSample / 8;
}

// VERIFY: 67c3c3bb auto
// OFFSET: 0x008873B0
int32_t ComSat::Sound::IOSoundEngine::GetPlaybackBlockSize(uint8_t player) {
    return this->m_samplesPerFrame * this->m_bitsPerSample / 8;
}

// VERIFY: 2149bcf9 auto
// OFFSET: 0x008873D0
int32_t ComSat::Sound::IOSoundEngine::EnumerateOutputDevices(ComSat::DeviceInfo* devices, int32_t maxDevices) {
    int32_t count = SI2::GetNumOutputDrivers(1);
    if (maxDevices < count) {
        count = maxDevices;
    }
    for (int32_t i = 0; i < count; i++) {
        SI2::GetOutputDriverName(i, devices[i].m_name, sizeof(devices[i].m_name), 1);
    }
    return count;
}

// VERIFY: 22e8394e auto
// OFFSET: 0x00887420
int32_t ComSat::Sound::IOSoundEngine::EnumerateCaptureDevices(ComSat::DeviceInfo* devices, int32_t maxDevices) {
    int32_t count = SI2::GetNumInputDrivers(1);
    if (maxDevices < count) {
        count = maxDevices;
    }
    for (int32_t i = 0; i < count; i++) {
        SI2::GetInputDriverName(i, devices[i].m_name, sizeof(devices[i].m_name), 1);
    }
    return count;
}

// VERIFY: 7235d738 auto
// OFFSET: 0x00887470
int32_t ComSat::Sound::IOSoundEngine::LocalPlayerGetCurrentOutputDeviceIndex(uint8_t player) {
    return this->m_outputDevice;
}

// VERIFY: 53c51fa7 auto
// OFFSET: 0x00887480
int32_t ComSat::Sound::IOSoundEngine::LocalPlayerGetCurrentCaptureDeviceIndex(uint8_t player) {
    return this->m_captureDevice;
}

// VERIFY: 2c2332ed auto
// OFFSET: 0x00887780
void ComSat::Sound::IOSoundEngine::ShutDownCapture() {
    for (uint32_t i = 0; i < this->m_channels.Count(); i++) {
        if (this->m_channels[i]) {
            SI2::StopSound(&this->m_channels[i]->m_object, 1, -1.0f, 1);
            this->m_channels[i] = nullptr;
        }
    }
    SI2::VoiceChat_CleanUpLoopbackSound();
    SI2::VoiceChat_ShutDownCapture();
}

// VERIFY: 157b0a60 auto
// OFFSET: 0x008877E0
void ComSat::Sound::IOSoundEngine::SetPlayerPlaybackVolume(uint8_t player, int32_t channel, float volume) {
    SI2::SetVolume(&this->m_channels[channel]->m_object, volume);
}

// VERIFY: 4c066a98 manual
// OFFSET: 0x00887860
bool ComSat::Sound::IOSoundEngine::PlayDataForPlayer(uint8_t player, int32_t channel, uint8_t* data, uint32_t size, double rate) {
    double pitch = 0.9;
    if (rate >= 0.9) {
        pitch = rate;
        if (rate >= 1.1) {
            pitch = 1.1;
        }
    }
    SI2::SetPitch(&this->m_channels[channel]->m_object, static_cast<float>(pitch * this->m_sampleRate));
    uint32_t count = size >> 1;
    float* samples = static_cast<float*>(alloca(sizeof(float) * count));
    const int16_t* pcm = reinterpret_cast<const int16_t*>(data);
    for (uint32_t i = 0; i < count; i++) {
        samples[i] = pcm[i] * (1.0f / 32768.0f);
    }
    this->m_channels[channel]->m_buffer.Write(samples, count);
    return true;
}

// VERIFY: ffd08db7 manual
// OFFSET: 0x00887980
int32_t ComSat::Sound::IOSoundEngine::GetRemainingPlaybackBytes(uint8_t player, int32_t channel) {
    return 2 * this->m_channels[channel]->m_buffer.Count();
}

// VERIFY: 11476088 auto
// OFFSET: 0x008879B0
int32_t ComSat::Sound::IOSoundEngine::ReadDataForPlayer(uint8_t player, uint8_t* data, int32_t size) {
    return 2 * this->m_captureChannel.m_buffer.Read(reinterpret_cast<int16_t*>(data), static_cast<uint32_t>(size) >> 1);
}

// VERIFY: 84034483 auto
// OFFSET: 0x008879D0
int32_t ComSat::Sound::IOSoundEngine::GetAvailableDataBytes(uint8_t player) {
    return 2 * this->m_captureChannel.m_buffer.Count();
}

// VERIFY: 94733445 manual
// OFFSET: 0x00887B70
void ComSat::Sound::IOSoundEngine::InitCapture() {
    SI2::VoiceChat_InitCapture(this->m_sampleRate);
    SI2::VoiceChat_SetPrimaryCaptureCallback(&this->m_captureChannel);
    SI2::VoiceChat_ActivatePrimaryCaptureCallback();
    // STATIC: 0x00D439A0
    static CVar* s_enableMicrophone = CVar::Lookup("EnableMicrophone");
    // STATIC: 0x00D4399C
    static CVar* s_enableVoiceChat = CVar::Lookup("EnableVoiceChat");
    if (s_enableVoiceChat && s_enableVoiceChat->m_intValue && s_enableMicrophone && s_enableMicrophone->m_intValue && !ComSatClient_IsComsatDisabledByClient()) {
        SI2::VoiceChat_StartCapture();
    }
    if (SI2::VoiceChat_IsVoiceChatInitialized()) {
        for (uint32_t i = 0; i < this->m_channels.Count(); i++) {
            this->m_channels[i] = new (this->m_allocator->Alloc(sizeof(ComSat::Sound::OutputChannel))) ComSat::Sound::OutputChannel(*this->m_allocator);
            ComSat::Sound::OutputChannel* channel = this->m_channels[i];
            SI2::VoiceChat_CreateChatSound(&channel->m_object, channel, 1);
            SI2SoundUserData* userData = new (SMemAlloc(sizeof(SI2SoundUserData), __FILE__, __LINE__, 0)) SI2SoundUserData();
            userData->m_name = "<Voice Chat>";
            userData->m_soundKitId = 0;
            userData->m_def = nullptr;
            userData->m_group = 3;
            channel->m_object.m_sound.SetUserData(userData);
            SI2::SetPitch(&channel->m_object, this->m_sampleRate);
            SI2::VoiceChat_Play(&channel->m_object);
        }
    }
}

// VERIFY: ed3c735f auto
// OFFSET: 0x00887D10
bool ComSat::Sound::IOSoundEngine::InitLocalPlayer(uint8_t player, int32_t outputDevice, int32_t captureDevice, int32_t sampleRate, int32_t bitsPerSample, int32_t samplesPerFrame) {
    this->m_outputDevice = outputDevice;
    this->m_sampleRate = static_cast<float>(sampleRate);
    this->m_captureDevice = captureDevice;
    this->m_bitsPerSample = bitsPerSample;
    this->m_samplesPerFrame = samplesPerFrame;
    this->InitCapture();
    return true;
}

// VERIFY: e904d241 auto
// OFFSET: 0x00887D50
bool ComSat::Sound::IOSoundEngine::LocalPlayerChangeOutputDevice(uint8_t player, int32_t device) {
    this->ShutDownCapture();
    SI2::VoiceChat_DestroyChatSystem();
    this->m_outputDevice = device;
    SI2::VoiceChat_SetOutputDevice(device);
    this->InitCapture();
    return true;
}

// VERIFY: dfe4484f auto
// OFFSET: 0x00887D80
bool ComSat::Sound::IOSoundEngine::LocalPlayerChangeCaptureDevice(uint8_t player, int32_t device) {
    this->ShutDownCapture();
    SI2::VoiceChat_DestroyChatSystem();
    this->m_captureDevice = device;
    SI2::VoiceChat_SetCaptureDevice(device);
    this->InitCapture();
    return true;
}

// VERIFY: 0f8734c7 auto
// OFFSET: 0x00653A10
void ComSat::Sound::IOSoundEngine::DebugSetSoundOutputDelay(uint8_t player, int32_t delay) {
}
