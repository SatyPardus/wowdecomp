#include "Tools/AppleBootstrap/GameAudioPreview.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include <stdexcept>

namespace {
void Check(FMOD_RESULT result, const char* operation) {
    if (result != FMOD_OK) throw std::runtime_error(std::string(operation) + ": FMOD error " + std::to_string(result));
}
void Default(const char* name, const char* value) {
    if (!CVar::Lookup(name)) CVar::Register(name, "", 0, value, nullptr, 7, false, nullptr, false);
}
}

GameAudioPreview::GameAudioPreview(const char* filename) : m_filename(filename ? filename : "") {
    if (m_filename.empty()) throw std::runtime_error("WOW_AUDIO must name a game sound resource");
    if (SE2::IsInitialized()) throw std::runtime_error("A game sound engine already owns the native host");
    if (!EventGetCurrentContext()) throw std::runtime_error("Game audio must be created inside its Event context");
    m_context = PropGetSelectedContext();
    try {
        if (!FrameScript_GetContext()) {
            FrameScript_Initialize(0);
            m_ownsScript = true;
        }
        // Original SoundInterface defaults, without starting its world/DBC policy.
        Default("Sound_EnableMode2", "0");
        Default("Sound_EnableHardware", "0");
        Default("Sound_DSPBufferSize", "0");
        Default("Sound_EnableMixMode2", "0");
        Default("Sound_OutputQuality", "1");
        int32_t channels = 32, driver = 0;
        m_started = true;
        if (!SE2::Init(64, nullptr, 1, 0, &channels, &driver, "", nullptr, 0))
            throw std::runtime_error("Original game sound engine initialization failed; see SESound.log");
        m_sound = std::make_unique<SESound>();
        if (!m_sound->Create2D(m_filename.c_str(), 1, nullptr, nullptr, false, true, 0, 128, 0))
            throw std::runtime_error("Cannot stream game sound: " + m_filename);
        m_system = m_sound->m_internal->m_system;
        m_sound->SetVolume(0.1f);
        m_sound->Play();
    } catch (...) { Close(); throw; }
}
void GameAudioPreview::Close() {
    auto previous = PropGetSelectedContext();
    PropSelectContext(m_context);
    if (m_sound) m_sound->Stop(1, 0);
    m_sound.reset();
    if (m_started) {
        SE2::StopAllSounds(0);
        SE2::UnregisterHeartBeat();
        SE2::ShutDown(0);
        m_started = false;
        m_system = nullptr;
    }
    if (m_ownsScript) { FrameScript_Destroy(); m_ownsScript = false; }
    PropSelectContext(previous);
}
GameAudioPreview::~GameAudioPreview() { Close(); }
void GameAudioPreview::SetActive(bool active) { Check(FMODAppleSetActive(m_system, active), "Set game audio activity"); }
FMODAppleOutputStats GameAudioPreview::Stats() {
    if (!m_sound || !m_sound->m_internal) throw std::runtime_error("Game sound stream stopped: " + m_filename);
    FMODAppleOutputStats result{};
    Check(FMODAppleGetOutputStats(m_system, &result), "Read game audio output");
    return result;
}
bool GameAudioPreview::IsPlaying() { return m_sound && m_sound->IsPlaying(); }
