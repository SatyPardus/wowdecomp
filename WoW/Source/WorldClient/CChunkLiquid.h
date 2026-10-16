#ifndef WOW_SOURCE_WORLDCLIENT_CCHUNKLIQUID_H
#define WOW_SOURCE_WORLDCLIENT_CCHUNKLIQUID_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/BitArray.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"

class CMapChunk;

namespace Liquid {
    class CInstance;
    class CWaveManager;
    class IVertexData;
}

namespace World { class TriData; }
class CiRect;
class CFrustum;

class CChunkLiquid {
    public:
    static Liquid::CWaveManager* s_waveManager;
    static int32_t s_waveManagerRefs;

    /* 0000 */ int32_t m_objectIndex;
    /* 0004 */ int32_t m_type;
    /* 0008 */ int32_t m_vertexFormat;
    /* 000C */ C3Vector m_topLeftCoords;
    /* 0018 */ CAaSphere m_sphere;
    /* 0028 */ C2Vector m_height;
    /* 0030 */ float m_resPurgeTimer;
    /* 0034 */ C2iVector m_tileBegin;
    /* 003C */ C2iVector m_tileEnd;
    /* 0044 */ Liquid::IVertexData* m_vertexData;
    /* 0048 */ CBitArray m_tileBits;
    /* 0054 */ uint8_t* m_tiles;
    /* 0058 */ Liquid::CInstance* m_instance;
    /* 005C */ CMapChunk* m_owner;
    /* 0060 */ TSLink<CChunkLiquid> m_link;
    /* 0068 */ TSLink<CChunkLiquid> m_updateLink;
    /* 0070 */ TSLink<CChunkLiquid> m_chunkLink;
    /* 0078 */ C3Vector m_verts[81];

    CChunkLiquid();
    ~CChunkLiquid();
    void Batch();
    void BuildIndices(uint16_t* indices, uint16_t baseVertex, CGxBatch* batch) const;
    void BuildVertices(const C44Matrix* transform, uint32_t stride, C3Vector** position, C3Vector** normal, uint32_t** color, C2Vector** texCoord1, C2Vector** texCoord0) const;
    void CreateVertXY();
    void GetAaBox(CAaBox& box) const;
    bool GetHeight(const C2Vector& frac, const C2iVector& tile, float& height) const;
    CAaSphere* GetSphere();
    bool GetTileInfo(const C2iVector& tile, int32_t* type, int32_t* flag6, int32_t* flag7) const;
    void Purge();
    void RenderPrep();
    bool TileExists(uint32_t x, uint32_t y) const;
    bool GetTris(World::TriData* triData, const CAaBox& box, const CiRect& rect);
    bool GetTris(World::TriData* triData, uint8_t* codes, const C2iVector& start, const C2iVector& size) const;
    bool GetTris(World::TriData* triData, const CFrustum& frustum, const CiRect& rect) const;
    void UpdateResourcePurging();
};

#endif
