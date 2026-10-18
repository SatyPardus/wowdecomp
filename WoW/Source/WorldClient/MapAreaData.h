#pragma once
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/Map.h"
#include <array>
#include <memory>
#include <vector>

// Owned, naturally aligned area tables. MCNK geometry stays in the source file
// for the original chunk loader; its indexed extents and object references are
// validated here before that loader can follow them.
class MapAreaData {
  public:
    static constexpr size_t MaxBytes = 64 * 1024 * 1024;
    explicit MapAreaData(const void *bytes, size_t size);
    SMAreaHeader header{};
    std::vector<SMChunkInfo> chunks;
    std::vector<char> textureNames, modelNames, wmoNames;
    std::vector<uint32_t> modelOffsets, wmoOffsets;
    std::vector<SMDoodadDef> doodads;
    std::vector<SMMapObjDef> objects;
    std::vector<int32_t> textureFlags;
    std::array<int16_t, 18> flyingBounds{};
    uint32_t textureCount = 0;
    std::unique_ptr<Liquid::CAreaData> liquid;
};
