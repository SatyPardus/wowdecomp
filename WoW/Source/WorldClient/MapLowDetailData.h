#pragma once
#include "WoW/Source/WorldClient/Map.h"
#include <array>
#include <vector>

// WDL chunk payloads follow arbitrary-length filenames and need not be aligned.
// Keep all original table values in owned, naturally aligned storage.
class MapLowDetailData {
  public:
    static constexpr size_t MaxBytes = 64 * 1024 * 1024;
    struct Area {
        std::array<int16_t, 545> heights{};
        std::array<uint16_t, 16> holes{};
        bool hasHoles = false;
    };
    explicit MapLowDetailData(const void* bytes, size_t size);
    std::vector<char> names;
    std::vector<uint32_t> nameOffsets;
    std::vector<SMMapObjDef> objects;
    std::array<uint32_t, 4096> areaOffsets{};
    std::array<int32_t, 4096> areaIndices{};
    std::vector<Area> areas;
};
