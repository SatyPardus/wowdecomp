#ifndef WOW_SOURCE_WORLDCLIENT_MAPOBJGROUP_H
#define WOW_SOURCE_WORLDCLIENT_MAPOBJGROUP_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Ray.h"
#include "Engine/Source/Tempest/C3Segment.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Common/AaBsp.h"
#include "WoW/Common/Types.h"
#include "Engine/Source/Tempest/C2iVector.h"

class CFrustum;
class CWFrustum;
struct SIffChunk;

struct BspQuery_Segment;
class C2Vector;
class C3Vector;
class C44Matrix;
class CAaBsp;
class CAsyncObject;
struct CGxBuf;
struct CGxPool;
class CImVector;
class CMapObj;
class CMapObjDef;
class CMapObjGroup;
struct SMOBatch;
struct SMOLTile;
union SMOLiquidVert;
struct SMOMaterial;
struct SMOPoly;
class VBBList;
class VBBList_Block;

namespace World {
    struct LiquidQueryResult;
    class FacetData;
    class TriData;
}

struct BspQuery {
    static uint16_t testFaces[8192];
    static uint16_t hitFaces[8192];
    static uint32_t testFaceSub;
    static uint32_t hitFaceSub;

    /* 0000 */ int32_t* m_overflowFlags;
    /* 0004 */ SMOPoly* m_faces;
    /* 0008 */ C3Vector* m_vertexList;
    /* 000C */ uint16_t* m_indices;

    ~BspQuery();
};

struct BspQuery_Segment : public BspQuery {
    /* 0010 */ float* m_hitT;
    /* 0014 */ float m_origHitT;
    /* 0018 */ C3Segment m_seg;
    /* 0030 */ C3Ray m_ray;
    /* 0048 */ float m_oosegMag;
    /* 004C */ float m_maxT;
    /* 0050 */ SMOMaterial* m_materials;
    /* 0054 */ int32_t unused_0054;
    /* 0058 */ uint16_t m_faceIgnoreFlags;
    /* 005A */ uint8_t unused_005A[2];

    BspQuery_Segment(SMOPoly* faces, C3Vector* vertexList, uint16_t* indices, const C3Segment& seg, float* hitT, uint16_t faceIgnoreFlags, SMOMaterial* materials);
    void operator()(uint16_t face);
    bool GetFaceIndicesUsingCache(const CAaBsp& aaBsp, const CAaBspNode* node);
};

template <class T>
struct BspQuery_Volume : public BspQuery {
    /* 0010 */ const T* m_volume;
    /* 0014 */ uint16_t m_faceIgnoreFlags;
    /* 0016 */ uint8_t unused_0016[2];

    void operator()(uint16_t face);
    bool GetFaceIndicesUsingCache(const CAaBsp& aaBsp, const CAaBspNode* node);
};

template <class T>
class CAaBsp_Query {
    public:
    /* 0000 */ const CAaBsp* m_aaBsp;
    /* 0004 */ T* m_f;

    void GetFaceIndices(const CAaBspNode* node);
};

template <class T>
class CAaBsp_Query_Segment : public CAaBsp_Query<T> {
    public:
    void GetFaceIndices(uint32_t nodeIndex, const C3Segment& seg, const CAaBox& box);
};

template <class T>
class CAaBsp_Query_AaBox : public CAaBsp_Query<T> {
    public:
    void GetFaceIndices(uint32_t nodeIndex, const CAaBox& query, const CAaBox& bounds);
};

class CGxVertexPNC2T2 {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
    /* 0018 */ CImVector m_color [2];
    /* 0020 */ C2Vector m_texture [2];
};

class CGxVertexPNCT {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
    /* 0018 */ CImVector m_color;
    /* 001C */ C2Vector m_texture;
};

class CMapObjOccluder {
    public:
    /* 0000 */ uint32_t m_memHandle;
    /* 0004 */ C3Vector m_start;
    /* 0010 */ C3Vector m_end;
    /* 001C */ TSLink<CMapObjOccluder> m_link;
};

class CMapObjGroup {
    public:
    bool QueryLighting(const C3Vector& point, uint16_t face, CImVector* color, uint8_t* flag);
    struct VertArray {
        /* 0000 */ TSLink<VertArray> m_link;
        /* 0008 */ TSFixedArray<C3Vector> m_verts;
    };

    static TSExplicitList<VertArray, offsetof(VertArray, m_link)> s_freeVertArrays;
    static VBBList indexVBList;
    static VBBList vertexVBList;

    static void Destroy();
    static void Initialize();
    static void AsyncFailedCallback(CAsyncObject* object);
    static void AsyncPostloadCallback(void* arg);
    static void AllocVertArray(CMapObjGroup::VertArray** vertArray, uint32_t count);

    /* 0000 */ int32_t m_objectIndex;
    /* 0004 */ VBBList_Block* m_vertsBlock;
    /* 0008 */ VBBList_Block* m_transparencyVertsBlock;
    /* 000C */ VBBList_Block* m_indicesBlock;
    /* 0010 */ VBBList_Block* m_liquidVertsBlock;
    /* 0014 */ VBBList_Block* m_liquidIndicesBlock;
    /* 0018 */ float m_timer;
    /* 001C */ CMapObjGroup::VertArray* m_liquidVertArray;
    /* 0020 */ CGxBatch m_liquidBatch;
    /* 0030 */ int32_t m_flags;
    /* 0034 */ CAaBox m_bbox;
    /* 004C */ float m_distToCamera;
    /* 0050 */ int32_t m_portalStart;
    /* 0054 */ int32_t m_portalCount;
    /* 0058 */ uint8_t m_fogs [4];
    /* 005C */ uint16_t m_transparencyBatchesCount;
    /* 005E */ uint16_t m_intBatchCount;
    /* 0060 */ uint16_t m_extBatchCount;
    /* 0062 */ uint8_t unused_0062[2];
    /* 0064 */ CAaBsp m_cAaBspNodePtr1;
    /* 00CC */ int32_t m_unkFlags;
    /* 00D0 */ int32_t unused_00D0;
    /* 00D4 */ int32_t m_minimapTag;
    /* 00D8 */ char* m_groupName;
    /* 00DC */ SMOPoly* m_polyList;
    /* 00E0 */ uint16_t* m_indices;
    /* 00E4 */ uint16_t* m_stripIndices;
    /* 00E8 */ C3Vector* m_vertexList;
    /* 00EC */ C3Vector* m_normalList;
    /* 00F0 */ C2Vector* m_textureVertexList;
    /* 00F4 */ C2Vector* m_textureVertexList2;
    /* 00F8 */ SMOBatch* m_batchList;
    /* 00FC */ uint8_t* m_stripBatches;
    /* 0100 */ uint16_t* m_lightRefList;
    /* 0104 */ uint16_t* m_doodadRefList;
    /* 0108 */ CImVector* m_colorVertexList;
    /* 010C */ CImVector* m_colorVertexListExtra;
    /* 0110 */ int32_t m_liquidVBRefCount;
    /* 0114 */ C2iVector m_liquidVerts;
    /* 011C */ C2iVector m_liquidTiles;
    /* 0124 */ C3Vector m_liquidCorner;
    /* 0130 */ uint16_t m_liquidMaterialId;
    /* 0132 */ uint8_t unused_0132[2];
    /* 0134 */ SMOLiquidVert* m_liquidVertexList;
    /* 0138 */ SMOLTile* m_liquidTileList;
    /* 013C */ float m_liquidHeight;
    /* 0140 */ float m_liquidMaxHeight;
    /* 0144 */ int32_t m_liquidType;
    /* 0148 */ int32_t m_numSharedTiles;
    /* 014C */ int32_t unused_014C;
    /* 0150 */ int32_t m_polyListSize;
    /* 0154 */ int32_t m_indicesCount;
    /* 0158 */ int32_t m_stripIndexCount;
    /* 015C */ int32_t m_vertexListCount;
    /* 0160 */ int32_t m_normalListCount;
    /* 0164 */ int32_t m_textureVertexListCount;
    /* 0168 */ int32_t m_textureVertexList2Count;
    /* 016C */ int32_t m_batchListCount;
    /* 0170 */ int32_t m_lightRefListCount;
    /* 0174 */ int32_t m_doodadRefListCount;
    /* 0178 */ int32_t m_colorVertexListSize;
    /* 017C */ int32_t m_colorVertexListExtraSize;
    /* 0180 */ int32_t m_wmoGroupId;
    /* 0184 */ void* m_filePtr;
    /* 0188 */ int32_t m_fileSize;
    /* 018C */ CMapObj* m_parent;
    /* 0190 */ float unused_0190;
    /* 0194 */ CAsyncObject* m_asyncObjPtr;
    /* 0198 */ uint32_t m_unkLoadedFlag;
    /* 019C */ uint32_t m_indexMin;
    /* 01A0 */ uint32_t m_indexMax;
    /* 01A4 */ uint16_t m_vertexMin;
    /* 01A6 */ uint16_t m_vertexMax;
    /* 01A8 */ TSExplicitList<CMapObjOccluder, offsetof(CMapObjOccluder, m_link)> m_occluderList;
    /* 01B4 */ TSLink<CMapObjGroup> m_asyncLoadLink;

    CMapObjGroup();
    ~CMapObjGroup();
    void Init();
    void Clear();
    void FreeVB();
    void InitPtrs();
    void Create();
    void CreateDataPointers(SIffChunk* dataChunk);
    void CreateOptionalDataPointers(SIffChunk* dataChunk);
    void AllocVB();
    void AllocLiquidVB(EGxVertexBufferFormat format, uint32_t vertexCount, uint32_t indexCount, CGxBatch** batch);
    void FreeLiquidVB();
    void SetIndexVB();
    void UploadIndexBuffer(CGxBuf* buf);
    void SetVertexVB();
    void FillVertexVB(CGxBuf* buf, EGxVertexBufferFormat format);
    void SetTransparencyVB();
    void FillTransparencyVB(CGxBuf* buf);
    void FixColorVertexAlpha();
    void HandleAntiportal();
    int32_t FindLiquidType();
    bool GetTris(const CWFrustum& frustum, uint32_t flags, uint16_t faceIgnoreFlags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool GetTris(const C3Segment& seg, float* dist, uint32_t flags, uint16_t faceIgnoreFlags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool GetTris(const CAaBox& box, uint32_t flags, uint16_t faceIgnoreFlags, World::TriData* triData, CMapObjDef* mapObjDef);
    void GetTrisFromQuery(World::TriData* triData, BspQuery* query, CMapObjDef* mapObjDef, uint32_t statusFlags);
    void GetTrisFromQuery(uint32_t a2, BspQuery* a3, CMapObjDef* mapObjDef, uint32_t a5);
    static void SetLighting(CMapObjGroup* group, uint32_t mode);
    bool Intersect(C3Segment& seg, float* dist, uint32_t flags, uint16_t faceIgnoreFlags, int32_t* hitIndex);
    bool GetFacesForLinking(C3Segment& seg, float* t0, int32_t* face0, float* t1, int32_t* face1);
    bool QueryLiquid(const C3Vector& pos, int32_t* liquidType, float* level);
    void QueryLiquidSounds(const C3Vector& pos, TSGrowableArray<World::LiquidQueryResult>& results);
    void GenLiquidVerts();
    int32_t GetLiquidType() const;
    int32_t GetLiquidType(uint32_t groupLiquid);
    uint32_t GetNumSharedTiles();
    bool GetLiquidTris(const CAaBox& box, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool GetLiquidTris(const CFrustum& frustum, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool GetLiquidFacets(const CAaBox& box, uint32_t flags, World::FacetData* facets, CMapObjDef* mapObjDef);
    bool VectorIntersectLiquid(const C3Segment& seg, float* t, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool VectorIntersectLiquidTiles(const C3Segment& seg, float* t, uint32_t flags, uint32_t* tileCount, uint8_t* tiles, World::TriData* triData, CMapObjDef* mapObjDef);
    void VectorIntersectLiquidSX(const C3Vector& start, const C3Vector& end, uint32_t* tileCount, uint8_t* tiles, uint32_t maxTiles);
    void VectorIntersectLiquidSY(const C3Vector& start, const C3Vector& end, uint32_t* tileCount, uint8_t* tiles, uint32_t maxTiles);
    void VectorIntersectLiquidDX(const C3Vector& start, const C3Vector& end, uint32_t* tileCount, uint8_t* tiles, uint32_t maxTiles);
    void VectorIntersectLiquidDY(const C3Vector& start, const C3Vector& end, uint32_t* tileCount, uint8_t* tiles, uint32_t maxTiles);
};

struct SMOGroupHeader {
    /* 0000 */ uint32_t m_groupName;
    /* 0004 */ uint32_t m_descriptiveGroupName;
    /* 0008 */ uint32_t m_flags;
    /* 000C */ CAaBox m_boundingBox;
    /* 0024 */ uint16_t m_portalStart;
    /* 0026 */ uint16_t m_portalCount;
    /* 0028 */ uint16_t m_transBatchCount;
    /* 002A */ uint16_t m_intBatchCount;
    /* 002C */ uint16_t m_extBatchCount;
    /* 002E */ uint16_t m_paddingOrBatchTypeD;
    /* 0030 */ uint8_t m_fogIds [4];
    /* 0034 */ uint32_t m_groupLiquid;
    /* 0038 */ uint32_t m_uniqueID;
    /* 003C */ uint32_t unused_003C;
    /* 0040 */ uint32_t unused_0040;
};

struct SMOLTile {
    /* 0000 */ uint8_t m_flags;
};

struct SMOMVert {
    /* 0000 */ int16_t m_s;
    /* 0002 */ int16_t m_t;
    /* 0004 */ float m_height;
};

struct SMOWVert {
    /* 0000 */ uint8_t m_flow1;
    /* 0001 */ uint8_t m_flow2;
    /* 0002 */ uint8_t m_flow1Pct;
    /* 0003 */ uint8_t m_filler;
    /* 0004 */ float m_height;
};

union SMOLiquidVert {
    /* 0000 */ SMOWVert m_waterVert;
    /* 0000 */ SMOMVert m_magmaVert;
};

class VBBList_Block {
    public:
    /* 0000 */ TSLink<VBBList_Block> m_link;
    /* 0008 */ float unused_0008;
    /* 000C */ uint32_t m_offset;
    /* 0010 */ uint32_t m_capacity;
    /* 0014 */ CGxPool* m_pool;
    /* 0018 */ CGxBuf* m_buffer;
    /* 001C */ VBBList_Block** m_listHead;

    void Set(uint32_t itemSize, uint32_t itemCount, uint32_t offset);
};

class VBBList {
    public:
    /* 0000 */ int32_t m_singlePool;
    /* 0004 */ EGxPoolTarget m_target;
    /* 0008 */ EGxPoolUsage m_usage;
    /* 000C */ CGxPool* m_pool;
    /* 0010 */ TSExplicitList<VBBList_Block, offsetof(VBBList_Block, m_link)> unk_0010;
    /* 001C */ TSExplicitList<VBBList_Block, offsetof(VBBList_Block, m_link)> unk_001C;

    void Destroy();
    VBBList_Block* AllocBlock();
    void AllocVBB(VBBList_Block** a1, uint32_t itemSize, uint32_t itemCount);
    void FreeVBB(VBBList_Block* block);
    void AssignBlock(VBBList_Block* a1, VBBList_Block** a2, uint32_t itemSize, uint32_t itemCount);
};

namespace World {
    class TriData {
        public:
        struct Batch {
            /* 0000 */ C44Matrix* m_matrix;
            /* 0004 */ C3Vector* m_vertexList;
            /* 0008 */ C3Vector* m_normalList;
            /* 000C */ float* m_heightList;
            /* 0010 */ uint16_t* m_indices;
            /* 0014 */ uint16_t* m_faceIndices;
            /* 0018 */ uint16_t m_indexCount;
            /* 001A */ uint16_t m_faceCount;
            /* 001C */ uint16_t m_minVertexIndex;
            /* 001E */ uint16_t m_maxVertexIndex;
            /* 0020 */ CMapObjDef* m_def;
        };

        static uint32_t statusFlags;
        static uint32_t nBatches;
        static uint32_t faceIndexCursor;
        static uint32_t indexCursor;
        static uint32_t matrixPoolCount;
        static uint16_t faceIndexPool[0x4000];
        static uint16_t indexPool[0xC000];
        static Batch batches[32];
        static C44Matrix matrixPool[32];

        static Batch* AllocBatch();
        static Batch* __stdcall AllocBatch(uint32_t indexCount, uint32_t faceCount);
        static uint16_t* __stdcall AllocIndices(uint32_t count);
    };
}

class WMOGroup {
    public:
    /* 0000 */ uint8_t unused_0000[284];
    /* 011C */ uint32_t unused_011C;
    /* 0120 */ uint8_t unused_0120[24];
    /* 0138 */ uint32_t unused_0138;
};

// VERIFY: d3f8a7cc auto
// OFFSET: [0x007C9A00<BspQuery_Segment>, 0x007C9A60, 0x007C9AB0, 0x007CA8C0<BspQuery_Volume<CAaBox>>]
template <class T>
void CAaBsp_Query<T>::GetFaceIndices(const CAaBspNode* node) {
    if (this->m_f->GetFaceIndicesUsingCache(*this->m_aaBsp, node)) {
        return;
    }

    const uint16_t* faces = &this->m_aaBsp->m_nodeFaceIndices[node->m_faceStart];

    for (uint32_t i = 0; i < node->m_nFaces; i++) {
        (*this->m_f)(faces[i]);
    }
}

// VERIFY: b715a408 manual
// OFFSET: [0x007CA180<BspQuery_Segment>, 0x007CA600<BspQuery_SegmentLink>]
template <class T>
void CAaBsp_Query_Segment<T>::GetFaceIndices(uint32_t nodeIndex, const C3Segment& seg, const CAaBox& box) {
    const CAaBspNode* node = &this->m_aaBsp->m_nodes[nodeIndex];

    if (!node) {
        return;
    }

    if (node->m_flags & 4) {
        this->CAaBsp_Query<T>::GetFaceIndices(node);
        return;
    }

    uint32_t axis = node->m_flags & 3;
    const float* segB = &seg.b.x;
    const float* segT = &seg.t.x;
    const float* boxB = &box.b.x;
    const float* boxT = &box.t.x;

    if (!(segB[axis] - boxB[axis] >= -0.0099999998f || segT[axis] - boxB[axis] >= -0.0099999998f)) {
        return;
    }

    if (!(boxT[axis] - segB[axis] >= 0.0099999998f || boxT[axis] - segT[axis] >= 0.0099999998f)) {
        return;
    }

    CAaBox posBox = box;
    (&posBox.b.x)[axis] = node->m_planeDist;
    CAaBox negBox = box;
    (&negBox.t.x)[axis] = node->m_planeDist;

    float distB = segB[axis] - node->m_planeDist;
    float distT;

    if ((distB >= -0.0099999998f && distB <= 0.0099999998f) || ((distT = segT[axis] - node->m_planeDist) >= -0.0099999998f && distT <= 0.0099999998f)) {
        if (node->m_posChild != 0xFFFF) {
            this->GetFaceIndices(node->m_posChild, seg, posBox);
        }

        if (node->m_negChild != 0xFFFF) {
            this->GetFaceIndices(node->m_negChild, seg, negBox);
        }

        return;
    }

    if (distB > 0.0099999998f && distT > 0.0099999998f) {
        if (node->m_posChild != 0xFFFF) {
            this->GetFaceIndices(node->m_posChild, seg, posBox);
        }

        return;
    }

    if (distB < -0.0099999998f && distT < -0.0099999998f) {
        if (node->m_negChild != 0xFFFF) {
            this->GetFaceIndices(node->m_negChild, seg, negBox);
        }

        return;
    }

    C3Vector split = seg.Lerp(distB / (distB - distT));
    C3Segment part;

    if (distB <= 0.0f) {
        if (node->m_negChild != 0xFFFF) {
            part.b = seg.b;
            part.t = split;
            this->GetFaceIndices(node->m_negChild, part, negBox);
        }

        if (node->m_posChild != 0xFFFF) {
            part.b = split;
            part.t = seg.t;
            this->GetFaceIndices(node->m_posChild, part, posBox);
        }
    } else {
        if (node->m_posChild != 0xFFFF) {
            part.b = seg.b;
            part.t = split;
            this->GetFaceIndices(node->m_posChild, part, posBox);
        }

        if (node->m_negChild != 0xFFFF) {
            part.b = split;
            part.t = seg.t;
            this->GetFaceIndices(node->m_negChild, part, negBox);
        }
    }
}

// VERIFY: 1ae93370 auto
// OFFSET: [0x007CA440<BspQuery_Volume<CWFrustum>>, 0x007CA920<BspQuery_Volume<CAaBox>>]
template <class T>
void CAaBsp_Query_AaBox<T>::GetFaceIndices(uint32_t nodeIndex, const CAaBox& query, const CAaBox& bounds) {
    const CAaBspNode* node = &this->m_aaBsp->m_nodes[nodeIndex];

    if (node->m_flags & 4) {
        this->CAaBsp_Query<T>::GetFaceIndices(node);
        return;
    }

    uint32_t axis = node->m_flags & 3;
    const float* queryB = &query.b.x;
    const float* queryT = &query.t.x;
    const float* boundsB = &bounds.b.x;
    const float* boundsT = &bounds.t.x;

    if (boundsB[axis] > queryT[axis] || boundsT[axis] < queryB[axis]) {
        return;
    }

    CAaBox posBounds = bounds;
    (&posBounds.b.x)[axis] = node->m_planeDist;
    CAaBox negBounds = bounds;
    (&negBounds.t.x)[axis] = node->m_planeDist;

    if (node->m_planeDist < queryB[axis]) {
        if (node->m_posChild != 0xFFFF) {
            this->GetFaceIndices(node->m_posChild, query, posBounds);
        }

        return;
    }

    if (node->m_planeDist > queryT[axis]) {
        if (node->m_negChild != 0xFFFF) {
            this->GetFaceIndices(node->m_negChild, query, negBounds);
        }

        return;
    }

    if (node->m_posChild != 0xFFFF) {
        CAaBox part = query;
        (&part.b.x)[axis] = node->m_planeDist;
        this->GetFaceIndices(node->m_posChild, part, posBounds);
    }

    if (node->m_negChild != 0xFFFF) {
        CAaBox part = query;
        (&part.t.x)[axis] = node->m_planeDist;
        this->GetFaceIndices(node->m_negChild, part, negBounds);
    }
}

#endif
