#ifndef ENGINE_SOURCE_SOUNDENGINE_SOUNDCACHE_H
#define ENGINE_SOURCE_SOUNDENGINE_SOUNDCACHE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataRecycler.h"
#include "Engine/Source/Storm/H/stpl.h"

class SFileImpl;

namespace Chunk {

struct Data {
    /* 0000 */ uint8_t unused_0000[0x8000];
};

}

struct CacheChunk {
    /* 0000 */ Chunk::Data* m_data;
    /* 0004 */ uint32_t m_size;
    /* 0008 */ int32_t m_time;
    /* 000C */ int32_t m_refCount;
};

struct CacheData : public TSHashObject<struct CacheData, HASHKEY_NONE> {
    /* 0018 */ SFileImpl* m_file;
    /* 001C */ CacheChunk* m_chunks;
    /* 0020 */ uint32_t unused_0020;
    /* 0024 */ uint32_t unused_0024;
    /* 0028 */ uint32_t unused_0028;
    /* 002C */ uint32_t m_chunkCount;
    /* 0030 */ uint32_t unused_0030;
    /* 0034 */ int32_t m_refCount;
    /* 0038 */ void* unk_0038;

    ~CacheData();
};

struct CacheChunkRef {
    /* 0000 */ struct CacheData* m_cacheData;
    /* 0004 */ uint32_t m_index;
};

class CacheChunkRecycler : public CDataRecycler {
    public:
    CacheChunkRecycler(uint32_t nodesPerBlock, int32_t maxNodes);
    virtual ~CacheChunkRecycler();
    virtual void Clear();
};

class SoundCache {
    public:
    /* 0000 */ const char* m_using;
    /* 0004 */ TSHashTable<struct CacheData, HASHKEY_NONE> m_table;
    /* 002C */ uint32_t m_bytes;
    /* 0030 */ uint32_t m_maxBytes;
    /* 0034 */ uint32_t m_lastUpdate;
    /* 0038 */ uint32_t m_startTime;

    SoundCache();
    ~SoundCache();
    void Clear();
    void Initialize();
    void Purge(uint32_t bytes);
    void Update();
};

extern SoundCache g_soundCache;

#endif
