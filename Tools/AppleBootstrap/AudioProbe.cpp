#include "Tools/AppleBootstrap/AudioProbe.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/SoundEngine/fmodex/apple/AppleOutput.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void Check(FMOD_RESULT result, const char* operation) {
    if (result != FMOD_OK) throw std::runtime_error(std::string(operation) + ": FMOD error " + std::to_string(result));
}
struct ReleaseSystem { void operator()(FMOD::System* value) const { value->release(); } };
struct ReleaseSound { void operator()(FMOD::Sound* value) const { value->release(); } };
using System = std::unique_ptr<FMOD::System, ReleaseSystem>;
using Sound = std::unique_ptr<FMOD::Sound, ReleaseSound>;
System CreateSystem(bool output) {
    FMOD::System* raw = nullptr;
    Check(FMOD::System_Create(&raw), "Create audio system");
    System system(raw);
    Check(system->setOutput(output ? FMOD_OUTPUTTYPE_COREAUDIO : FMOD_OUTPUTTYPE_NOSOUND_NRT), "Select audio output");
    Check(system->setSoftwareFormat(44100, FMOD_SOUND_FORMAT_PCMFLOAT, 2, 2, FMOD_DSP_RESAMPLER_SPLINE), "Original spline resampler");
    Check(system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO), "Stereo mode");
    Check(system->setDSPBufferSize(512, 4), "Audio block size");
    Check(system->init(32, output ? FMOD_INIT_NORMAL : FMOD_INIT_STREAM_FROM_UPDATE, nullptr), "Initialize audio");
    return system;
}
Sound Open(FMOD::System* system, const std::vector<uint8_t>& bytes, FMOD_MODE mode) {
    FMOD_CREATESOUNDEXINFO info{};
    info.cbsize = sizeof(info);
    info.length = static_cast<unsigned>(bytes.size());
    FMOD::Sound* sound = nullptr;
    Check(system->createSound(reinterpret_cast<const char*>(bytes.data()), mode | FMOD_OPENMEMORY | FMOD_SOFTWARE, &info, &sound), "Decode game audio");
    return Sound(sound);
}
uint32_t Word(const std::vector<uint8_t>& data, size_t offset) {
    Require(offset <= data.size() && data.size() - offset >= 4, "Invalid WAV fixture");
    uint32_t result; memcpy(&result, data.data() + offset, 4); return result;
}
struct Capture {
    std::array<double, 2> energy{};
    unsigned frames = 0;
    bool finite = true;
};
FMOD_RESULT CaptureMix(FMOD_DSP_STATE* state, float* input, float* output, unsigned frames, int inChannels, int outChannels) {
    void* user = nullptr;
    auto* dsp = reinterpret_cast<FMOD::DSP*>(state->instance);
    if (dsp->getUserData(&user) != FMOD_OK || inChannels != 2 || outChannels != 2) return FMOD_ERR_DSP_FORMAT;
    auto& capture = *static_cast<Capture*>(user);
    for (unsigned i = 0; i < frames * 2; ++i) {
        capture.finite &= std::isfinite(input[i]);
        capture.energy[i % 2] += double(input[i]) * input[i];
        output[i] = input[i];
    }
    capture.frames += frames;
    return FMOD_OK;
}
}

void CheckAudioResources(const ClientData& client, std::ostream& report) {
    Capture capture; // Must outlive system teardown, which can perform a final mix.
    auto system = CreateSystem(false);
    auto wav = client.Read("Sound\\Interface\\iUIMainMenuButtonA.wav", 1024 * 1024, true);
    Require(Word(wav.bytes, 0) == 0x46464952 && Word(wav.bytes, 8) == 0x45564157, "Unexpected WAV container");
    size_t pcm = 0, size = 0;
    for (size_t position = 12; position + 8 <= wav.bytes.size();) {
        uint32_t chunkSize = Word(wav.bytes, position + 4);
        Require(chunkSize <= wav.bytes.size() - position - 8, "Invalid WAV chunk");
        if (Word(wav.bytes, position) == 0x61746164) { pcm = position + 8; size = chunkSize; break; }
        position += 8 + chunkSize + (chunkSize & 1);
    }
    Require(pcm && size, "WAV has no authored samples");
    auto sound = Open(system.get(), wav.bytes, FMOD_OPENONLY);
    FMOD_SOUND_FORMAT format;
    int channels = 0, bits = 0;
    Check(sound->getFormat(nullptr, &format, &channels, &bits), "WAV format");
    Require(format == FMOD_SOUND_FORMAT_PCM16, "WAV fixture is not PCM16");
    std::vector<uint8_t> decoded(size);
    unsigned read = 0;
    Check(sound->readData(decoded.data(), unsigned(decoded.size()), &read), "Read WAV samples");
    Require(read == size && !memcmp(decoded.data(), wav.bytes.data() + pcm, size), "WAV decoder changed authored PCM");
    sound.reset();

    const auto music = client.Read("Sound\\Music\\GlueScreenMusic\\Wotlk_main_title.mp3", 16 * 1024 * 1024, true);
    std::vector<uint8_t> first(512 * 1024), second(first.size());
    for (unsigned pass = 0; pass < 2; ++pass) {
        auto mpeg = Open(system.get(), music.bytes, FMOD_OPENONLY);
        Check(mpeg->getFormat(nullptr, &format, &channels, &bits), "MP3 format");
        auto& output = pass ? second : first;
        Check(mpeg->readData(output.data(), unsigned(output.size()), &read), "Read original MP3 decoder");
        Require(read == output.size(), "MP3 decoded too few samples");
    }
    Require(first == second, "MP3 decode was not reproducible");
    Require(std::any_of(first.begin(), first.end(), [](uint8_t value) { return value != 0; }), "MP3 decode produced only silence");

    FMOD_DSP_DESCRIPTION description{};
    strcpy(description.name, "Apple audio diagnostic");
    description.channels = 0;
    description.read = CaptureMix;
    FMOD::DSP* dsp = nullptr;
    Check(system->createDSP(&description, &dsp), "Create mix observer");
    struct DetachDSP { FMOD::DSP* dsp; ~DetachDSP() { dsp->remove(); dsp->release(); } } detach{dsp};
    Check(dsp->setUserData(&capture), "Set mix observer");
    Check(system->addDSP(dsp), "Attach mix observer");
    Check(dsp->setActive(true), "Activate mix observer");
    sound = Open(system.get(), wav.bytes, FMOD_LOOP_NORMAL);
    FMOD::Channel* channel = nullptr;
    Check(system->playSound(FMOD_CHANNEL_FREE, sound.get(), true, &channel), "Start original software channel");
    Check(channel->setVolume(0.25f), "Channel volume");
    Check(channel->setPan(-1), "Channel left pan");
    Check(channel->setPaused(false), "Unpause software channel");
    for (unsigned i = 0; i < 90; ++i) Check(system->update(), "Mix left channel");
    Require(capture.finite && capture.frames && capture.energy[0] > 0 && capture.energy[1] < capture.energy[0] * 0.0001, "Left channel mix invalid");
    Check(channel->setPan(1), "Channel right pan");
    for (unsigned i = 0; i < 4; ++i) Check(system->update(), "Settle channel ramp");
    capture = {};
    for (unsigned i = 0; i < 90; ++i) Check(system->update(), "Mix right channel");
    Require(capture.finite && capture.energy[1] > 0 && capture.energy[0] < capture.energy[1] * 0.0001, "Right channel mix invalid");
    Check(channel->stop(), "Stop software channel");

    report << "Original audio: WAV " << size << " PCM bytes exact; MP3 " << first.size()
           << " decoded bytes reproducible; spline-resampled looping stereo pan verified.\n";
}

void CheckAudioOutput(const ClientData& client, std::ostream& report) {
    auto system = CreateSystem(true);
    const auto asset = client.Read("Sound\\Music\\GlueScreenMusic\\Wotlk_main_title.mp3", 16 * 1024 * 1024, true);
    auto sound = Open(system.get(), asset.bytes, FMOD_CREATESTREAM | FMOD_LOOP_NORMAL);
    FMOD::Channel* channel = nullptr;
    Check(system->playSound(FMOD_CHANNEL_FREE, sound.get(), true, &channel), "Play game music");
    Check(channel->setVolume(0.1f), "Preview volume");
    Check(channel->setPaused(false), "Start music");
    auto advance = [&] {
        for (unsigned i = 0; i < 100; ++i) {
            Check(system->update(), "Update live audio");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    advance();
    FMODAppleOutputStats before{}, suspended{}, resumed{};
    Check(FMODAppleSetActive(system.get(), false), "Suspend native audio");
    Check(FMODAppleGetOutputStats(system.get(), &before), "Read output counters");
    advance();
    Check(FMODAppleGetOutputStats(system.get(), &suspended), "Read suspended counters");
    Require(!suspended.running && before.frames == suspended.frames, "Audio callback continued while suspended");
    Check(FMODAppleSetActive(system.get(), true), "Resume native audio");
    advance();
    Check(FMODAppleGetOutputStats(system.get(), &resumed), "Read resumed counters");
    Require(resumed.running && resumed.frames > suspended.frames && resumed.nonzeroSamples > before.nonzeroSamples, "Native audio did not resume nonzero output");
    Check(channel->stop(), "Stop music channel");
    report << "Audio Unit: " << resumed.sampleRate << " Hz, " << resumed.channels << " channels, "
           << resumed.frames << " frames / " << resumed.callbacks << " callbacks; suspend/resume passed.\n";
}
