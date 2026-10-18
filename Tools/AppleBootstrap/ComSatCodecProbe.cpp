#include "Engine/Source/ComSat/ComSatSoundCODEC.h"
#include "Engine/Source/ComSat/ComSatSoundLevelCalculator.h"
#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace {
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
struct Allocator : ComSat::Allocator {
    std::unordered_set<void*> live;
    void* Alloc(uint32_t size) override { auto* p = std::malloc(size); if (!p) throw std::bad_alloc(); live.insert(p); return p; }
    void Free(void* p) override { if (p) { Require(live.erase(p) == 1, "Voice codec freed an unowned allocation"); std::free(p); } }
};
uint64_t Hash(const void* bytes, size_t size) {
    uint64_t hash = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) hash = (hash ^ static_cast<const uint8_t*>(bytes)[i]) * 1099511628211ull;
    return hash;
}
// First 100 ms of the integer sawtooth fixture below, encoded by the shared
// x86 implementation with its original SSE operations. The filter-history
// storage correction is applied; this is not a captured original WoW packet.
constexpr uint8_t Reference[] = {
    0xe0,0xdd,0x18,0x7e,0xfc,0xb6,0x0e,0xb3,0x6e,0x64,0x16,0x70,0xf3,0x04,0x81,0xbe,
    0x8f,0xd8,0x6f,0x76,0xe8,0xbf,0xe9,0xf0,0x66,0x21,0xf7,0x63,0xde,0x5b,0xae,0x6a,
    0xf6,0xa1,0xf5,0xc7,0x6c,0x35,0xfb,0xad,0xe9,0x46,0xa4,0xc3,0xfc,0x58,0xab,0x1a,
    0x41,0x40,0x4c,0xb7,0x5c,0xb5,0xa3,0xfe,0x99,0x05,0x67,0x41,0xbd,0x5d,0xfe,0x80,
    0x59,0x0f,0xaf,0xd2,0xc4,0xea,0x77,0xad,0x6e,0xc4,0xb4,0x5b,0xae,0xfa,0xcf,0x2c,
    0x38,0x0b,0x2a,0x2f,0xbf,0x31,0xbb,0x3a,0xcf,0x75,0x0e,0xba,0x1e,0x5c,0xfd,0x5a,
    0xe7,0xdd,0xea,0x4b
};
constexpr uint64_t ReferencePCM = 0xaed0e202f4ccfa9bull;
void Conversion() {
    const int rounding = std::fegetround(); std::fesetround(FE_TONEAREST);
    const float inf = std::numeric_limits<float>::infinity(), nan = std::numeric_limits<float>::quiet_NaN();
    const float values[] = {0.5f,1.5f,2.5f,3.5f,-0.5f,-1.5f,-2.5f,-3.5f,32767.5f,-32768.5f,40000,-40000,inf,-inf,nan,2147483648.0f};
    const int16_t expected[] = {0,2,2,4,0,-2,-2,-4,32767,-32768,32767,-32768,-32768,-32768,-32768,-32768};
    int16_t pcm[16]; SPIRIT_IP_MR_V3_NAME_200(values, pcm, 16);
    std::fesetround(rounding);
    Require(!std::memcmp(pcm, expected, sizeof(pcm)), "Native voice PCM rounding/clipping differs from SSE nearest/even and indefinite conversion");
    const int16_t samples[] = {-32768,32767,0,-1}; uint8_t unaligned[9]; std::memcpy(unaligned + 1, samples, 8);
    float level, db; ComSat::Sound::LevelCalculator16Bit::CalculateLevelForFrame(unaligned + 1, 8, level, db);
    Require(level == 65535.0f / 65536.0f && std::fabs(db - 20.0f * std::log10(131.07f)) < 0.0001f, "Unaligned native voice level measurement changed");
}
}

void CheckComSatCodec() {
    Conversion();
    for (int cycle = 0; cycle < 2; ++cycle) {
        Allocator allocator;
        {
            ComSat::Initialization init{}; init.m_maxChannels = 2;
            ComSat::Sound::CODECIPMR6000 codec(init, allocator);
            Require(codec.Init() && codec.InitLocalPlayer(7, 0.0f) && !codec.InitLocalPlayer(7, 0.0f), "Original voice codec/player initialization changed");
            std::array<uint8_t,1602> output; output.fill(0xAD); uint32_t size = 1600;
            Require(codec.DecompressEncodedData(7, 0, Reference, 799, output.data() + 1, &size) && size == 1600 && output.front() == 0xAD && output.back() == 0xAD &&
                Hash(output.data() + 1, size) == ReferencePCM, "Native voice decoding differs from the x86 reference PCM");
            // Failed packets must leave both the caller buffer and the decoder
            // history intact so that a following valid packet can still play.
            for (uint32_t bits : {0u,359u,360u,361u,400u,600u,797u,798u,800u,895u,960u,961u,UINT32_MAX}) {
                std::vector<uint8_t> packet(bits > 960 ? 120 : (bits + 7) / 8, 0);
                if (!packet.empty()) std::memcpy(packet.data(), Reference, std::min(packet.size(), sizeof(Reference)));
                codec.ResetPlayerChannelDecoder(7, 0); output.fill(0xAD); size = 1600;
                Require(!codec.DecompressEncodedData(7, 0, packet.data(), bits, output.data() + 1, &size) && size == 1600 &&
                    std::all_of(output.begin(), output.end(), [](uint8_t b) { return b == 0xAD; }), "Invalid voice packet published partial PCM or exceeded its frame budget");
                Require(codec.DecompressEncodedData(7, 0, Reference, 799, output.data() + 1, &size) && Hash(output.data() + 1, size) == ReferencePCM,
                    "Failed voice packet corrupted subsequent decoder history");
            }
            uint32_t seed = 0x12345678, encoded = 0, suppressed = 0;
            codec.ResetPlayerChannelEncoder(7); codec.ResetPlayerChannelDecoder(7, 0); codec.ResetPlayerChannelDecoder(7, 1);
            for (int frame = 0; frame < 60; ++frame) {
                std::array<int16_t,800> pcm{}; std::array<uint8_t,1601> unaligned{};
                for (int i = 0; i < 800; ++i) {
                    const int n = frame * 800 + i; seed = 1664525 * seed + 1013904223;
                    switch (frame / 10) {
                    case 0: pcm[i] = int16_t(((n % 79) - 39) * 400); break;
                    case 1: pcm[i] = int16_t(int(seed >> 16) - 32768); break;
                    case 2: pcm[i] = n % 157 == 0 ? 24000 : 0; break;
                    case 3: pcm[i] = n % 2 ? 32767 : -32768; break;
                    case 4: pcm[i] = int16_t(((n % 71) - 35) * 5); break;
                    default: pcm[i] = 0; break;
                    }
                }
                std::memcpy(unaligned.data() + 1, pcm.data(), 1600);
                std::array<uint8_t,122> packet{}; packet.front() = packet.back() = 0xAD;
                uint32_t bits = 960; bool voice = false;
                Require(codec.CompressRawData(7, unaligned.data() + 1, 1600, packet.data() + 1, &bits, &voice) && packet.front() == 0xAD && packet.back() == 0xAD,
                    "Native voice encoder exceeded its original packet budget");
                if (!voice) { Require(bits == 0, "Suppressed voice frame exposed stale encoded length"); ++suppressed; continue; }
                ++encoded;
                Require(bits >= 360 && bits <= 960, "Original voice codec emitted an invalid bit length");
                std::array<uint8_t,1600> second{}; size = 1600;
                Require(codec.DecompressEncodedData(7, 0, packet.data() + 1, bits, output.data() + 1, &size) &&
                    codec.DecompressEncodedData(7, 1, packet.data() + 1, bits, second.data(), &size) && !std::memcmp(output.data() + 1, second.data(), 1600),
                    "Original independent voice channels decode different PCM");
            }
            Require(encoded == 59 && suppressed == 1, "Original voice threshold/hangover changed");
            // Exercise syntax from an untrusted voice peer with physically
            // bounded packets. No network or microphone is used here.
            for (unsigned i = 0; i < 128; ++i) {
                std::array<uint8_t,120> packet{};
                for (auto& byte : packet) { seed = 1664525 * seed + 1013904223; byte = uint8_t(seed >> 24); }
                codec.ResetPlayerChannelDecoder(7, 0); size = 1600;
                codec.DecompressEncodedData(7, 0, packet.data(), 960, output.data() + 1, &size);
                Require(output.front() == 0xAD && output.back() == 0xAD, "Malformed voice packet crossed PCM boundaries");
            }
            Require(codec.RemoveLocalPlayer(7) && codec.InitLocalPlayer(8, 1.0f), "Original voice player removal/recreation failed");
            Require(codec.Reset() && codec.m_players.m_used == 0 && codec.InitLocalPlayer(7, 0.0f), "Original codec reset retained old players");
        }
        Require(allocator.live.empty(), "Original voice codec teardown leaked owned state");
    }
    std::fputs("Original ComSat codec: native PCM rounding, x86 packet/PCM fixture, 120 synthetic 100 ms frames, independent channels, threshold/hangover, bounded malformed/truncated packets and two lifetimes passed\n", stderr);
}
