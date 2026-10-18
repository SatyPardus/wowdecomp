#include "Engine/Source/ComSat/ComSatSoundCODEC.h"
#include "Engine/Source/ComSat/ComSatAllocator.h"
#include "Engine/Source/ComSat/ComSatSoundLevelCalculator.h"
#include "Engine/Source/ComSat/ComSatSpirit.h"
#include <cstring>
#include <new>
#include <array>

// STATIC: 0x00A503A0
const uint8_t ComSat::Sound::CODECIPMR6000::s_invalidPlayer = 0xFF;

// STATIC: 0x00A503A4
const int32_t ComSat::Sound::CODECIPMR6000::s_maxFramesPerPacket = 20;

// STATIC: 0x00A503A8
const int32_t ComSat::Sound::CODECIPMR6000::s_sampleRate = 8000;

// STATIC: 0x00A503AC
const int32_t ComSat::Sound::CODECIPMR6000::s_bitsPerSample = 16;

// STATIC: 0x00A503B0
const int32_t ComSat::Sound::CODECIPMR6000::s_samplesPerFrame = 800;

// STATIC: 0x00A503B4
const int32_t ComSat::Sound::CODECIPMR6000::s_minEncodedFrameLength = 360;

// STATIC: 0x00A503B8
const int32_t ComSat::Sound::CODECIPMR6000::s_maxEncodedFrameLength = 960;

// STATIC: 0x00A503BC
const uint32_t ComSat::Sound::CODECIPMR6000::s_bytesPerFrame = 1600;

// VERIFY: 27be376e manual
// OFFSET: 0x008867E0
ComSat::Sound::CODECIPMR6000::LocalPlayer::LocalPlayer(ComSat::Allocator& allocator, int32_t channelCount, uint8_t* tables) {
    this->m_threshold = 0.5f;
    this->m_tables = tables;
    this->m_allocator = &allocator;
    this->m_channelCount = channelCount;
    this->m_hangover = 0;
    this->m_decoders = static_cast<uint8_t**>(allocator.Alloc(sizeof(uint8_t*) * channelCount));
    if (this->m_decoders) {
        for (int32_t i = 0; i < channelCount; i++) {
            this->m_decoders[i] = static_cast<uint8_t*>(this->m_allocator->Alloc(sizeof(this->m_encoder)));
            if (this->m_decoders[i] && SPIRIT_IP_MR_V3_1_Init(this->m_decoders[i], this->m_tables)) {
                this->m_allocator->Free(this->m_decoders[i]);
                this->m_decoders[i] = nullptr;
            }
        }
    }
    SPIRIT_IP_MR_V3_1_Init(this->m_encoder, this->m_tables);
}

// VERIFY: b5a425c3 auto
// OFFSET: [0x008868C0, 0x00886BA0]
ComSat::Sound::CODECIPMR6000::LocalPlayer::~LocalPlayer() {
    if (this->m_decoders) {
        for (int32_t i = 0; i < this->m_channelCount; i++) {
            this->m_allocator->Free(this->m_decoders[i]);
            this->m_decoders[i] = nullptr;
        }
        this->m_allocator->Free(this->m_decoders);
        this->m_decoders = nullptr;
    }
}

// VERIFY: 2b94f608 manual
// OFFSET: 0x00886930
bool ComSat::Sound::CODECIPMR6000::LocalPlayer::CompressRawData(const uint8_t* pcm, uint32_t size, uint8_t* data, uint32_t* bits, bool* voice) {
    uint8_t* encoder = this->m_encoder;
    if (!pcm || !data || !bits || !voice || size != 1600 || *bits < 960) {
        return false;
    }
    if (this->m_hangover > 0) {
        this->m_hangover--;
    }
    float level;
    float decibels;
    ComSat::Sound::LevelCalculator16Bit::CalculateLevelForFrame(pcm, 1600, level, decibels);
    if (this->m_threshold <= decibels) {
        this->m_hangover = 20;
    }
    if (this->m_hangover <= 0) {
        *voice = false;
        *bits = 0;
        return true;
    }
    // The public API accepts bytes, including an unaligned capture buffer.
    std::array<int16_t, 800> samples;
    std::memcpy(samples.data(), pcm, sizeof(samples));
    pcm = reinterpret_cast<const uint8_t*>(samples.data());
    SPIRIT_IPMR_STREAM stream;
    stream.m_data = data;
    stream.m_bits = 0;
    stream.m_size = 120;
    SPIRIT_IPMR_INFO info;
    memset(&info, 0, sizeof(info));
    SPIRIT_IPMR_PARAMS params;
    params.unk_0000 = 0;
    params.unk_0004 = 0;
    params.unk_0006 = 1;
    int32_t result = 0;
    for (int32_t i = 0; i < 5; i++) {
        result = SPIRIT_IP_MR_V3_1_Encode(encoder, pcm, &params, &stream, &info);
        pcm += 320;
        if (result) {
            break;
        }
    }
    *bits = stream.m_bits;
    *voice = true;
    return result == 0;
}

// VERIFY: 4909c6a3 manual
// OFFSET: 0x00886A50
bool ComSat::Sound::CODECIPMR6000::LocalPlayer::DecompressEncodedData(int32_t channel, const uint8_t* data, uint32_t bits, uint8_t* pcm, uint32_t* size) {
    if (!data || !pcm || !size || bits < 360 || bits > 960 || *size < 1600) {
        return false;
    }
    if (channel < 0 || channel >= this->m_channelCount || !this->m_decoders || !this->m_decoders[channel]) {
        return false;
    }
    uint32_t bytes = bits >> 3;
    if (bits & 7) {
        bytes++;
    }
    SPIRIT_IPMR_PARAMS params;
    params.unk_0000 = 0;
    params.unk_0004 = 0;
    SPIRIT_IPMR_STREAM stream;
    stream.m_bits = 0;
    stream.m_size = bytes;
    params.unk_0006 = 1;
    stream.m_data = data;
    auto* state = reinterpret_cast<SPIRIT_IPMR_STATE*>((reinterpret_cast<uintptr_t>(this->m_decoders[channel]) + 16) & ~uintptr_t(15));
    const SPIRIT_IPMR_DECODER previous = state->m_decoder;
    std::array<int16_t, 800> samples;
    // One game frame is exactly five 20 ms codec frames, regardless of each
    // variable bit length. Never decode a sixth frame into the 1600-byte PCM
    // buffer, or publish uninitialized samples for a truncated frame.
    for (uint32_t i = 0; i < 5; ++i) {
        const uint32_t before = stream.m_bits;
        if (before > bits || bits - before < 72 ||
            SPIRIT_IP_MR_V3_1_Decode(this->m_decoders[channel], reinterpret_cast<uint8_t*>(samples.data() + i * 160), &params, &stream, 0) ||
            stream.m_bits <= before || stream.m_bits > bits) {
            state->m_decoder = previous;
            return false;
        }
    }
    if (stream.m_bits != bits) {
        state->m_decoder = previous;
        return false;
    }
    std::memcpy(pcm, samples.data(), sizeof(samples));
    *size = 1600;
    return true;
}

// VERIFY: 0560a550 auto
// OFFSET: [0x00886B30, 0x008889A0]
ComSat::Sound::CODEC::~CODEC() {
}

// INLINED
ComSat::Sound::CODEC::CODEC(ComSat::Allocator& allocator) : m_allocator(&allocator) {
}

// VERIFY: 151c084d auto
// OFFSET: 0x008A1420
bool ComSat::Sound::CODEC::Init() {
    return true;
}

// VERIFY: dfe4af70 auto
// OFFSET: 0x008A1420
bool ComSat::Sound::CODEC::Reset() {
    return true;
}

// VERIFY: c46dc152 auto
// OFFSET: 0x005EEB70
void ComSat::Sound::CODEC::Shutdown() {
}

// VERIFY: 0a45f794 auto
// OFFSET: 0x008A1420
bool ComSat::Sound::CODEC::CanBitpackFrames() {
    return true;
}

// VERIFY: d247f3ba auto
// OFFSET: 0x008A1420
bool ComSat::Sound::CODEC::IsEncodedBitrateVariable() {
    return true;
}

// VERIFY: e146742c auto
// OFFSET: 0x0095BFB0
uint32_t ComSat::Sound::CODEC::MinimumSafeFramesBetweenResets() {
    return 32;
}

// VERIFY: 98756928 auto
// OFFSET: 0x00886B40
bool ComSat::Sound::CODECIPMR6000::Init() {
    if (this->m_initialized) {
        this->Shutdown();
    }
    if (SPIRIT_IP_MR_V3_1_GenerateTables(this->m_tables)) {
        return false;
    }
    this->m_initialized = true;
    return true;
}

// VERIFY: d6cc4d79 auto
// OFFSET: 0x00886B70
bool ComSat::Sound::CODECIPMR6000::Reset() {
    return this->Init();
}

// VERIFY: 78c49031 auto
// OFFSET: 0x00886B80
void ComSat::Sound::CODECIPMR6000::GenerateSilence(uint8_t* pcm, uint32_t size) {
    memset(pcm, 0, size);
}

// VERIFY: 5d9ec280 auto
// OFFSET: 0x00886CA0
ComSat::Sound::CODEC* ComSat::Sound::CODEC::Create(const ComSat::Initialization& init, ComSat::Allocator& allocator) {
    ComSat::Sound::CODEC* codec = new (allocator.Alloc(sizeof(ComSat::Sound::CODECIPMR6000))) ComSat::Sound::CODECIPMR6000(init, allocator);
    codec->Init();
    return codec;
}

// INLINED
ComSat::Sound::CODECIPMR6000::CODECIPMR6000(const ComSat::Initialization& init, ComSat::Allocator& allocator) : CODEC(allocator), m_players(allocator) {
    this->m_maxChannels = init.m_maxChannels;
    this->m_initialized = false;
}

// VERIFY: 4be450a4 auto
// OFFSET: 0x00886D10
void ComSat::Sound::CODECIPMR6000::ResetPlayerChannelDecoder(uint8_t player, int32_t channel) {
    if (this->m_initialized) {
        LocalPlayer* localPlayer;
        if (this->m_players.Get(ComSat::MapKey<uint8_t>(player), localPlayer) && localPlayer->m_decoders && channel >= 0 && channel < localPlayer->m_channelCount && localPlayer->m_decoders[channel]) {
            SPIRIT_IP_MR_V3_1_Init(localPlayer->m_decoders[channel], localPlayer->m_tables);
        }
    }
}

// VERIFY: cecb1a97 auto
// OFFSET: 0x00886DA0
void ComSat::Sound::CODECIPMR6000::ResetPlayerChannelEncoder(uint8_t player) {
    if (this->m_initialized) {
        LocalPlayer* localPlayer;
        if (this->m_players.Get(ComSat::MapKey<uint8_t>(player), localPlayer)) {
            SPIRIT_IP_MR_V3_1_Init(localPlayer->m_encoder, localPlayer->m_tables);
        }
    }
}

// VERIFY: adcedc12 manual
// OFFSET: 0x00886E10
bool ComSat::Sound::CODECIPMR6000::CompressRawData(uint8_t player, const uint8_t* pcm, uint32_t size, uint8_t* data, uint32_t* bits, bool* voice) {
    LocalPlayer* localPlayer;
    return this->m_initialized && this->m_players.Get(ComSat::MapKey<uint8_t>(player), localPlayer) && localPlayer->CompressRawData(pcm, size, data, bits, voice);
}

// VERIFY: 0fa6d60b manual
// OFFSET: 0x00886E90
bool ComSat::Sound::CODECIPMR6000::DecompressEncodedData(uint8_t player, int32_t channel, const uint8_t* data, uint32_t bits, uint8_t* pcm, uint32_t* size) {
    LocalPlayer* localPlayer;
    return this->m_initialized && this->m_players.Get(ComSat::MapKey<uint8_t>(player), localPlayer) && localPlayer->DecompressEncodedData(channel, data, bits, pcm, size);
}

// VERIFY: e47f31e7 auto
// OFFSET: 0x00886F10
void ComSat::Sound::CODECIPMR6000::SetThreshold(uint8_t player, float threshold) {
    LocalPlayer* localPlayer;
    if (this->m_players.Get(ComSat::MapKey<uint8_t>(player), localPlayer)) {
        localPlayer->m_threshold = ComSat::Sound::LevelCalculator16Bit::GetMaxDecibelLevel() * threshold;
    }
}

// VERIFY: 02adc07a manual
// OFFSET: 0x008870F0
void ComSat::Sound::CODECIPMR6000::Shutdown() {
    LocalPlayer* localPlayer = nullptr;
    ComSat::MapKey<uint8_t> key(s_invalidPlayer);
    uint32_t iterator = 0;
    while (this->m_players.Next(iterator, key, localPlayer)) {
        LocalPlayer* removed;
        this->m_players.Remove(key, removed);
        ComSat::Delete(*this->m_allocator, localPlayer);
    }
    this->m_initialized = false;
}

// VERIFY: b32780fd manual
// OFFSET: 0x008871A0
bool ComSat::Sound::CODECIPMR6000::InitLocalPlayer(uint8_t player, float threshold) {
    if (!this->m_initialized) {
        return false;
    }
    uint32_t index;
    if (this->m_players.Find(ComSat::MapKey<uint8_t>(player), index)) {
        return false;
    }
    LocalPlayer* localPlayer = new (this->m_allocator->Alloc(sizeof(LocalPlayer))) LocalPlayer(*this->m_allocator, this->m_maxChannels, this->m_tables);
    localPlayer->m_threshold = ComSat::Sound::LevelCalculator16Bit::GetMaxDecibelLevel() * threshold;
    this->m_players.Insert(ComSat::MapKey<uint8_t>(player), localPlayer);
    return true;
}

// VERIFY: b9d5e115 auto
// OFFSET: 0x00887280
bool ComSat::Sound::CODECIPMR6000::RemoveLocalPlayer(uint8_t player) {
    if (!this->m_initialized) {
        return false;
    }
    LocalPlayer* localPlayer = nullptr;
    if (this->m_players.Remove(ComSat::MapKey<uint8_t>(player), localPlayer)) {
        ComSat::Delete(*this->m_allocator, localPlayer);
    }
    return true;
}

// VERIFY: 8e319759 auto
// OFFSET: 0x00887300
ComSat::Sound::CODECIPMR6000::~CODECIPMR6000() {
    this->Shutdown();
}

// VERIFY: 9adda6e6 auto
// OFFSET: 0x0047ADD0
void ComSat::Sound::CODECIPMR6000::ApplyVoiceEffects(uint8_t player, uint8_t* pcm, uint32_t* size) {
}
