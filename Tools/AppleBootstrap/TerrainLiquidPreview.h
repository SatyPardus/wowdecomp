#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include "Engine/Source/Tempest/C3Vector.h"
namespace Terrain {
struct Surface;
}

class TerrainLiquidPreview {
  public:
    TerrainLiquidPreview(const std::vector<uint8_t> &adt, const Terrain::Surface &surface, uint32_t tileX, uint32_t tileY, float radius,
                         const std::string &mapName);
    ~TerrainLiquidPreview();
    bool Ready(bool wait);
    void Draw(const C3Vector &camera);
    size_t LayerCount() const;
    size_t BatchCount() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> m;
};
