#pragma once
#include "Engine/Source/SoundEngine/fmodex/apple/AppleOutput.h"
#include "Engine/Source/Base/Types.h"
#include <memory>
#include <string>

class SESound;

// Owns a standalone game sound engine within an existing Event context. The
// archive mount and Event context must outlive this object. Full SoundInterface
// zone/music selection remains the responsibility of the eventual game host.
class GameAudioPreview {
    std::unique_ptr<SESound> m_sound;
    FMOD::System* m_system = nullptr;
    HPROPCONTEXT m_context = nullptr;
    std::string m_filename;
    bool m_ownsScript = false, m_started = false;
    void Close();
public:
    explicit GameAudioPreview(const char* filename);
    ~GameAudioPreview();
    void SetActive(bool active);
    FMODAppleOutputStats Stats();
    bool IsPlaying();
};
