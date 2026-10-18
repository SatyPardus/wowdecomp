#pragma once
#include "Engine/Source/Tempest/C3Vector.h"
#include <string>
#include <array>
#include <cstdint>

// Owns original outdoor lighting and sky resources for one native terrain view.
// The application owns Services, configuration and the Gx device outside this scope.
class DayNightPreview {
  public:
    explicit DayNightPreview(const std::string& mapName);
    ~DayNightPreview();
    DayNightPreview(const DayNightPreview&) = delete;
    DayNightPreview& operator=(const DayNightPreview&) = delete;
    void SetTime(float minutes);
    void Update(float elapsedSeconds, const C3Vector& position, const C3Vector& camera, const C3Vector& direction, float farClip);

  private:
    void Shutdown();
    bool m_blob = false, m_effects = false, m_models = false, m_map = false;
    double m_daySeconds = 43200, m_elapsedSeconds = 0;
    int32_t m_mapId = -1;
    std::array<uint32_t, 8> m_waterState{};
};
