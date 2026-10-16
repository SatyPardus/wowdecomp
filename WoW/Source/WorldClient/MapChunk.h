#ifndef WOW_SOURCE_WORLDCLIENT_MAPCHUNK_H
#define WOW_SOURCE_WORLDCLIENT_MAPCHUNK_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C2Vector.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "Engine/Source/Base/BitArray.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/WorldClient/MapBaseObj.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"

class CFacet;
class CFrustum;
class CMapArea;
class CiRect;
namespace World { class FacetData; }
struct SIffChunk;

class C3Ray;
class CM2Lighting;
class CDetailDoodadInst;
class CChunkLiquid;
class CMapChunk;
class CMapChunkDoodadDefLink;
class CMapChunkMapObjDefLink;
class CMapObjDef;
class CMapRenderChunk;
class CWSoundEmitter;
class SE2SoundKitObject;
struct SMChunk;
struct SMLayer;
struct SMLiquidChunk;

class CGxVertexPN {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
};

class CGxVertexPNC {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
    /* 0018 */ CImVector m_color;
};

class CMapChunkMapObjDefLink {
    public:
    /* 0000 */ uint32_t m_objectIndex;
    /* 0004 */ CMapObjDef* m_owner;
    /* 0008 */ CMapChunk* m_ref;
    /* 000C */ TSLink<CMapChunkMapObjDefLink> m_refLink;
    /* 0014 */ TSLink<CMapChunkMapObjDefLink> m_ownerLink;
};

class CMapChunk : public CMapBaseObj {
    public:
    static float s_geoToTex;
    static C3Vector vertexList[145];
    static int32_t farCornerIndex;
    static int32_t nearCornerIndex;
    static const int32_t cornerVertexIndex[4];

    static void Destroy();
    static void Initialize();
    static void InitializeVertexGrid();
    static bool CanMergeChunkLayers(CMapChunk* chunkA, CMapChunk* chunkB);

    /* 0024 */ C2iVector m_aIndex;
    /* 002C */ C2iVector m_sOffset;
    /* 0034 */ C2iVector m_cOffset;
    /* 003C */ CAaSphere m_sphere;
    /* 004C */ CAaBox m_bbox;
    /* 0064 */ CAaBox m_liquidBounds;
    /* 007C */ C3Vector m_topLeftCoords;
    /* 0088 */ float m_distToCamera;
    /* 008C */ CAaBox m_bbox2;
    /* 00A4 */ CDetailDoodadInst* m_detailDoodadInst;
    /* 00A8 */ CMapRenderChunk* m_renderChunk;
    /* 00AC */ int32_t m_bLoaded;
    /* 00B0 */ int32_t m_areaId;
    /* 00B4 */ TSLink<CMapChunk> m_link;
    /* 00BC */ TSLink<CMapChunk> m_sortListLink;
    /* 00C4 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_doodadDefLinkList;
    /* 00D0 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_mapObjDefLinkList;
    /* 00DC */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_entityLinkList;
    /* 00E8 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_lightLinkList;
    /* 00F4 */ TSList<SE2SoundKitObject, TSGetExplicitLink<SE2SoundKitObject> > m_soundHandleList;
    /* 0100 */ TSExplicitList<CChunkLiquid, offsetof(CChunkLiquid, m_chunkLink)> m_liquidChunkLinkList;
    /* 010C */ uint8_t* m_chunkInfoBeginPtr;
    /* 0110 */ SMChunk* m_header;
    /* 0114 */ uint8_t* m_lowQualityTexMap;
    /* 0118 */ uint8_t* m_predTexture;
    /* 011C */ float* m_height;
    /* 0120 */ uint32_t* m_vertexShading;
    /* 0124 */ int8_t* m_normals;
    /* 0128 */ uint8_t* m_shadowMap;
    /* 012C */ SMLayer* m_layers;
    /* 0130 */ uint8_t* m_additionalShadowmap;
    /* 0134 */ uint8_t* m_mcrfPtr;
    /* 0138 */ SMLiquidChunk* m_liquid;
    /* 013C */ CWSoundEmitter* m_soundEmitters;
    /* 0140 */ CBitArray m_deepBits;
    /* 014C */ CBitArray m_fishableBits;

    virtual void SelectLights(CM2Lighting* lighting);

    CMapChunk();
    ~CMapChunk() override;
    void Create(SIffChunk* headerChunk, bool a3);
    void CreateRefs(CMapArea* mapChunk, uint32_t* mcrfPtr, uint32_t doodadRefs, uint32_t mapObjRefs);
    void ProcessIffChunks(bool a3);
    void CreateBounds();
    int32_t Intersect(int32_t subX, int32_t subY, const C3Ray& ray, float* t);
    void RenderPrep();
    void Batch();
    void AllocRenderChunkAndBatch();
    void CullAndBucketChunk();
    void Purge();
    void UpdateLights();
    void GetVertexPosition(int32_t index, C3Vector* pos);
    void ClipBufferUpdate();
    void CreateLiquids(bool a3);
    void CreateSoundEmitters(bool a3);
    void CreateDetailDoodads();
    int32_t LoadDetailDoodadModels();
    void GetLiquidBounds(CAaBox& box) const;
    bool GetLiquidHeight(const C2Vector& pos, float* height) const;
    int16_t CreateIndices(char* buf, int32_t offset);
    void CreateIndices(uint16_t* buf, CGxBatch* batch);
    void CreateVertices(char* buf, int32_t vertexOffset, const C3Vector& offset);
    void CreateVerticesLocal(CGxVertexPN* buf);
    void CreateVerticesLocal(CGxVertexPNC* buf);
    void CreateVerticesWorld(CGxVertexPN* buf, const C3Vector& offset);
    void CreateVerticesWorld(CGxVertexPNC* buf, const C3Vector& offset);
    void Intersect(CiRect* rect, CAaBox* box, TSGrowableArray<CFacet>* facets);
    void Intersect(CiRect* rect, const CFrustum* frustum, TSGrowableArray<CFacet>* facets);
};

class CWSoundEmitter {
    public:
    /* 0000 */ uint32_t m_entryId;
    /* 0004 */ C3Vector m_position;
    /* 0010 */ C3Vector m_size;
};

struct SLTiles {
    /* 0000 */ char m_tiles [8][8];
};

struct SLVert {
    /* 0000 */ uint8_t unused_0000[8];
};

struct SMChunk {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ C2iVector m_index;
    /* 000C */ uint32_t m_nLayers;
    /* 0010 */ uint32_t m_nDoodadRefs;
    /* 0014 */ uint32_t m_ofsHeight;
    /* 0018 */ uint32_t m_ofsNormal;
    /* 001C */ uint32_t m_ofsLayer;
    /* 0020 */ uint32_t m_ofsRefs;
    /* 0024 */ uint32_t m_ofsAlpha;
    /* 0028 */ uint32_t m_sizeAlpha;
    /* 002C */ uint32_t m_ofsShadow;
    /* 0030 */ uint32_t m_sizeShadow;
    /* 0034 */ uint32_t m_areaid;
    /* 0038 */ uint32_t m_nMapObjRefs;
    /* 003C */ uint16_t m_holes;
    /* 003E */ uint16_t unused_003E;
    /* 0040 */ uint8_t m_lowQualityTextureMap [16];
    /* 0050 */ uint32_t m_predTex;
    /* 0054 */ uint32_t m_nEffectDoodad;
    /* 0058 */ uint32_t m_ofsSndEmitters;
    /* 005C */ uint32_t m_nSndEmitters;
    /* 0060 */ uint32_t m_ofsLiquid;
    /* 0064 */ uint32_t m_sizeLiquid;
    /* 0068 */ C3Vector m_position;
    /* 0074 */ uint32_t m_ofsMCCV;
    /* 0078 */ uint32_t m_unused1;
    /* 007C */ uint32_t m_unused2;
};

struct SWFlowv {
    /* 0000 */ CAaSphere m_sphere;
    /* 0010 */ C3Vector m_dir;
    /* 001C */ float m_velocity;
    /* 0020 */ float m_amplitude;
    /* 0024 */ float m_frequency;
};

struct SMLiquidChunk {
    /* 0000 */ float m_minHeight;
    /* 0004 */ float m_maxHeight;
    /* 0008 */ SLVert m_verts [81];
    /* 0290 */ SLTiles m_tiles;
    /* 02D0 */ uint32_t m_nFlowvs;
    /* 02D4 */ SWFlowv m_flowvs [2];
};

struct SMVert {
    /* 0000 */ uint16_t m_s;
    /* 0002 */ uint16_t m_t;
    /* 0004 */ float m_height;
};

struct SOVert {
    /* 0000 */ char m_depth;
    /* 0001 */ char m_foam;
    /* 0002 */ char m_wet;
    /* 0003 */ char m_filler;
};

struct SWVert {
    /* 0000 */ char m_depth;
    /* 0001 */ char m_flow0Pct;
    /* 0002 */ char m_flow1Pct;
    /* 0003 */ char m_filler;
    /* 0004 */ float m_height;
};

#endif
