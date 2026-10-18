#pragma once
#include <cstdint>
#include <memory>
class CSimpleMovieFrame;
namespace FMOD { class System; }
// A native viewer boundary around the original MovieFrame; requires live
// Event, CVar and UIComponents. The original game owns these in its Glue host.
class MoviePlayback {
    std::unique_ptr<CSimpleMovieFrame> m_movie;
    FMOD::System* m_system = nullptr;
    bool m_audio = false, m_metadata = false, m_active = true;
    uint32_t m_pausedAt = 0;
    void Close();
public:
    explicit MoviePlayback(const char* path);
    ~MoviePlayback();
    void SetActive(bool active);
    void Report() const;
};
