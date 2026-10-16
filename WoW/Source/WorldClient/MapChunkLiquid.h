#ifndef WOW_SOURCE_WORLDCLIENT_MAPCHUNKLIQUID_H
#define WOW_SOURCE_WORLDCLIENT_MAPCHUNKLIQUID_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

struct CGxBuf;
struct CGxPool;
class CMapChunkLiquidBuf;
class CMapChunkLiquid;

class CMapChunkLiquidBuf {
    public:
    static TSList<CMapChunkLiquid, TSGetExplicitLink<CMapChunkLiquid> > s_freeLists;

    static CMapChunkLiquidBuf* Alloc(uint32_t vertexCount, uint32_t indexCount, int32_t flags, EGxVertexBufferFormat format);
    static CMapChunkLiquidBuf* CreateBuf(uint32_t vertexCount, uint32_t indexCount, int32_t flags, uint32_t vertexSize);
    static void Free(CMapChunkLiquidBuf* buf);
    static void Update(float elapsed);

    /* 0000 */ uint32_t m_flags;
    /* 0004 */ float m_flushTimer;
    /* 0008 */ uint32_t m_vertexDataSize;
    /* 000C */ uint32_t m_indexDataSize;
    /* 0010 */ CGxBuf* m_vertexBuf;
    /* 0014 */ CGxBuf* m_indexBuf;
    /* 0018 */ CGxPool* m_vertexPool;
    /* 001C */ CGxPool* m_indexPool;
    /* 0020 */ TSLink<CMapChunkLiquidBuf> m_link;

    CMapChunkLiquidBuf();
    ~CMapChunkLiquidBuf();
    bool Valid() const;
};

class CMapChunkLiquid {
    public:
    /* 0000 */ uint32_t m_vertexDataSize;
    /* 0004 */ uint32_t m_indexDataSize;
    /* 0008 */ TSExplicitList<CMapChunkLiquidBuf, offsetof(CMapChunkLiquidBuf, m_link)> m_liquids;
    /* 0014 */ TSLink<CMapChunkLiquid> m_link;

    CMapChunkLiquid();
    ~CMapChunkLiquid();
};

#endif
