#include <cstddef>
#include <cstdint>
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include <typeinfo>
#include "Engine/Source/SoundEngine/SoundCache.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/System/Using/Using.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"

// STATIC: 0x00D43430
static CSRWLock s_cacheLock;

// STATIC: 0x00D43448
static CacheChunkRecycler s_chunkRecycler(16, 0x7FFFFFFF);

// STATIC: 0x00B1D5E4
SoundCache g_soundCache;

// VERIFY: e6046ba0 manual
// OFFSET: 0x00877480
static void SortChunkRefs(CacheChunkRef* refs, uint32_t count) {
    if (count < 2) {
        return;
    }

    uint32_t l = (count >> 1) + 1;

    while (true) {
        if (l > 1) {
            l--;
        } else {
            CacheChunkRef tmp = refs[0];
            refs[0] = refs[count - 1];
            refs[count - 1] = tmp;

            count--;

            if (count == 1) {
                return;
            }
        }

        uint32_t i = l;

        while (2 * i <= count) {
            uint32_t j = 2 * i;

            if (j < count && refs[j - 1].m_cacheData->m_chunks[refs[j - 1].m_index].m_time - refs[j].m_cacheData->m_chunks[refs[j].m_index].m_time < 0) {
                j++;
            }

            if (refs[i - 1].m_cacheData->m_chunks[refs[i - 1].m_index].m_time - refs[j - 1].m_cacheData->m_chunks[refs[j - 1].m_index].m_time >= 0) {
                break;
            }

            CacheChunkRef tmp = refs[i - 1];
            refs[i - 1] = refs[j - 1];
            refs[j - 1] = tmp;

            i = j;
        }
    }
}

// INLINED
CacheData::~CacheData() {
    if (this->unk_0038) {
        SMemFree(this->unk_0038, __FILE__, __LINE__, 0);
    }

    if (this->m_chunks) {
        SMemFree(this->m_chunks, __FILE__, __LINE__, 0);
    }

    if (this->m_file) {
        SFileCloseFile(this->m_file);
    }
}

// INLINED
CacheChunkRecycler::CacheChunkRecycler(uint32_t nodesPerBlock, int32_t maxNodes) : CDataRecycler(nodesPerBlock, maxNodes) {
}

// VERIFY: 49c942f0 auto
// OFFSET: 0x00877400
void CacheChunkRecycler::Clear() {
    CDataRecycler::Clear();
}

// VERIFY: 742adbf6 manual
// OFFSET: 0x00877410
CacheChunkRecycler::~CacheChunkRecycler() {
}

// INLINED
SoundCache::SoundCache() : m_using("Blizzard::Mopaq") {
    Blizzard::Using::StartUsing(this->m_using);
    this->m_bytes = 0;
    this->m_maxBytes = 4 * 1024 * 1024;
    this->m_lastUpdate = 0;
    this->m_startTime = 0;
}

// VERIFY: 871f9def auto
// OFFSET: 0x00877A40
SoundCache::~SoundCache() {
    this->Clear();
    Blizzard::Using::StopUsing(this->m_using);
}

// VERIFY: 16524107 auto
// OFFSET: 0x008773F0
void SoundCache::Update() {
    this->m_lastUpdate = OsGetAsyncTimeMs();
}

// VERIFY: cd452fb7 auto
// OFFSET: 0x00877440
void SoundCache::Initialize() {
    uint32_t now = OsGetAsyncTimeMs();
    this->m_lastUpdate = now;
    this->m_startTime = now;

    CVar* cvar = CVar::Register("SoundMemoryCache", "sound cache memory size (MB)", 2, "4", nullptr, 7, false, nullptr, false);

    if (cvar) {
        this->m_maxBytes = cvar->m_intValue << 20;
    }
}

// VERIFY: 66b8671f manual
// OFFSET: 0x008775C0
void SoundCache::Purge(uint32_t bytes) {
    CacheChunkRef* refs = static_cast<CacheChunkRef*>(alloca(sizeof(CacheChunkRef) * (this->m_maxBytes >> 10)));
    uint32_t count = 0;

    struct CacheData* data = this->m_table.m_fulllist.Head();

    while (data) {
        struct CacheData* next = this->m_table.m_fulllist.Next(data);
        bool hasData = false;

        if (data->m_chunks) {
            for (uint32_t i = 0; i < data->m_chunkCount; i++) {
                CacheChunk* chunk = &data->m_chunks[i];

                if (chunk->m_data) {
                    hasData = true;

                    if (!chunk->m_refCount) {
                        refs[count].m_cacheData = data;
                        refs[count].m_index = i;
                        count++;
                    }
                }
            }
        }

        if (!hasData && !data->m_refCount) {
            this->m_table.Delete(data);
        }

        data = next;
    }

    SortChunkRefs(refs, count);

    for (uint32_t i = 0; i < count; i++) {
        CacheChunk* chunk = &refs[i].m_cacheData->m_chunks[refs[i].m_index];
        this->m_bytes -= chunk->m_size;
        s_chunkRecycler.PutData(chunk->m_data, sizeof(Chunk::Data), StormTypeName(typeid(Chunk::Data)), SLOG_OBJECT);
        chunk->m_data = nullptr;
        chunk->m_size = 0;

        if (this->m_bytes <= bytes) {
            break;
        }
    }
}

// VERIFY: f5e24d5b auto
// OFFSET: 0x00877710
void SoundCache::Clear() {
    s_cacheLock.Enter(1);

    this->Purge(0);
    this->Purge(0);
    this->m_table.InternalClear(0);
    this->m_bytes = 0;
    s_chunkRecycler.Clear();

    s_cacheLock.Leave(1);
}
