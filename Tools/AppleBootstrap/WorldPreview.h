#pragma once
#include "WoW/Source/UIUtil/CSimpleCamera.h"
#include <string>

// Standalone native host for the original map/update/scene-render path.
// Services, configuration, audio and Gx outlive this owner. No server/player or
// WorldFrame UI is created; the camera and time are explicit viewer inputs.
class WorldPreview {
public:
    explicit WorldPreview(const std::string& selection); // Map:worldX:worldY:worldZ
    ~WorldPreview();
    WorldPreview(const WorldPreview&) = delete;
    WorldPreview& operator=(const WorldPreview&) = delete;
    void SetTime(float minutes);
    bool Draw(float elapsedSeconds, uint32_t width, uint32_t height);
private:
    void Shutdown();
    std::string m_map;
    CSimpleCamera m_camera{0.2f, 500, 1};
    bool m_blob = false, m_models = false, m_world = false, m_loaded = false;
    int32_t m_timeOverride = -1;
    double m_elapsed = 0, m_daySeconds = 43200;
};
