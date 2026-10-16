#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_STREAMING_REPAIRCACHE_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_STREAMING_REPAIRCACHE_H

#include <cstdint>
#include <map>
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/Digest.h"

namespace Blizzard {
namespace File {
struct StreamRecord;
}
}

namespace Blizzard {
namespace Repair {

struct RepairKey {
    /* 0000 */ Blizzard::Util::Digest<16> m_md5;
    /* 0010 */ int64_t m_index;

    bool operator<(const RepairKey& other) const;
};

struct RepairEntry {
    /* 0000 */ uint32_t m_offset;
    /* 0004 */ uint32_t m_size;
    /* 0008 */ void* m_data;
    /* 000C */ bool m_valid;
    /* 000D */ uint8_t unused_000D[3];
};

class RepairCache {
    public:
    /* 0000 */ std::map<RepairKey, RepairEntry> m_entries;
    /* 000C */ std::map<RepairKey, RepairEntry> m_repaired;
    /* 0018 */ uint32_t m_totalSize;
    /* 001C */ uint32_t m_repairedCount;
    /* 0020 */ uint32_t m_repairedSize;
    /* 0024 */ char m_headPath[260];
    /* 0128 */ char m_dataPath[260];

    RepairCache();
    ~RepairCache();
    bool AddEntry(const Blizzard::Util::Digest<16>& md5, int64_t index, uint32_t offset, uint32_t size);
    bool Load(Blizzard::File::StreamRecord* file);
};

extern RepairCache s_repairCache;

}
}

#endif
