#pragma once
#include "Map.h"
#include "MapRenderChunk.h"
#include <array>
#include <string>

// WDT's small, owned layout is validated before CMap publishes global state.
// Global-WMO placement keeps the original first-record/name semantics.
class MapLayoutData {
public:
    static constexpr size_t MaxBytes = 1024 * 1024;
    MapLayoutData(const void* bytes, size_t size);
    uint32_t version = 0;
    SMMapHeader header{};
    std::array<SMAreaInfo, 4096> areas{};
    std::string globalName;
    SMMapObjDef globalObject{};
};
