#ifndef WOW_SOURCE_WORLDCLIENT_MAPOBJ_H
#define WOW_SOURCE_WORLDCLIENT_MAPOBJ_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"

struct MINIMAPQUADINFO;
class C2Vector;
class C44Matrix;
class CFrustum;
class CWFrustum;
class CMapObjDefGroup;
struct C3Segment;

class C3Vector;
class C4Plane;
class CAsyncObject;
class CMapObj;
class CMapObjDef;
class CMapObjGroup;
class CShaderEffect;
struct SMODoodadDef;
struct SMODoodadSet;
struct SMOFog;
struct SMOGroupInfo;
class CAaSphere;
struct SMOHeader;
struct SMOLight;
struct SMOMaterial;
struct SMOPortal;
struct SMOBatch;
class CTexture;
class CRect;
struct SPortalExt;
class CPortalView;
struct SMOPortalRef;
struct SMOVisibleBlock;
namespace World { struct LiquidQueryResult; }

class CMapObjIndex {
    public:
    /* 0000 */ int32_t m_objectIndex;
};

typedef void (*RENDER_CALLBACK)(uint32_t, void*);

class CMapObj : public CMapObjIndex, public TSHashObject<CMapObj, HASHKEY_NONE> {
    public:
    static void RenderMapObjGroupsShadowMap(void* groups, uint32_t count, void* groupData, const C44Matrix& translation, CFrustum& frustum);
    bool QueryLighting(uint32_t groupNum, const C3Vector& point, uint16_t face, CImVector* color, uint8_t* flag);
    bool QueryLighting(uint32_t groupNum, const C3Segment& seg, CImVector* color, uint8_t* flag);
    static TSList<CMapObj, TSGetExplicitLink<CMapObj> > s_asyncLoadQueue;
    static TSList<CMapObjGroup, TSGetExplicitLink<CMapObjGroup> > s_loadedGroups;
    static TSHashTable<CMapObj, HASHKEY_NONE> mapObjHash;
    static RENDER_CALLBACK gRenderCallback;
    static void* gRenderUserParam;
    static int32_t bPoolsDirty;
    static uint32_t* occluderHeap;
    static int32_t s_renderMode;
    static void (CMapObj::*s_renderGroupExterior)(CMapObjGroup* group, uint32_t index);
    static void (CMapObj::*s_renderGroupInterior)(CMapObjGroup* group, uint32_t index);
    static CShaderEffect* s_classicShaders[7];
    static CShaderEffect* s_unifiedShaders[7];
    static int32_t s_lightingMode;
    static uint32_t s_maxLiquidQueryDepth;

    static void ClearCache(int32_t unused_a1);
    static void DestroyPools();

    int32_t IsAllLoaded();
    int32_t IsAllDrawable();
    bool TestConvexVolume(const C3Vector& point);
    static void Destroy();
    static void Initialize();
    static void RenderMapObjGroupsColor(CMapObjGroup** groups, uint32_t count, C44Matrix** matrices, const C44Matrix* world, int32_t* a5);
    static uint32_t CreateWmoIgnoreFlags(uint32_t flags);
    static void PrepareUpdate();
    static CMapObj* Create(char* fileName);
    static void PostloadCallback(void* arg);
    static void __stdcall RenderGroupCollidableFaces(CMapObjGroup* mapObjGroup);
    static void AsyncFailedCallback(CAsyncObject* object);
    static void ProcessAsyncLoadQueue();
    static CMapObjOccluder* AllocOccluder();
    static CMapObjOccluder* AllocOccluderNode(uint32_t* heap);
    static void CreateRefs(CMapObj* mapObj, CMapObjGroup* mapObjGroup, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup);
    static void InitDefGroup(CMapObj* mapObj, CMapObjGroup* mapObjGroup, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup);
    static void SetEmissiveAmbient(CImVector color);
    static bool CullBatch(SMOBatch* batch);
    static void __stdcall RenderNormals(CMapObjGroup* group);
    static void InvokeGroupRenderCallback(CMapObj* mapObj, uint32_t groupNum);
    static void SetGroupRenderCallback(RENDER_CALLBACK callback, void* param);
    static void SetRenderModeLight();
    static void UpdatePools();
    static void Delete(CMapObj* mapObj);
    static void FreeOccluderEntry(uint32_t* heap, CMapObjOccluder* occluder);
    static void FreeOccluder(CMapObjOccluder* occluder);
    static void SetEmissiveColor(CImVector color);
    static void SelectWorldShaders();

    /* 001C */ char m_wmoName [260];
    /* 0120 */ SMOHeader* m_header;
    /* 0124 */ char* m_textureNameList;
    /* 0128 */ char* m_groupNameList;
    /* 012C */ char* m_skybox;
    /* 0130 */ SMOGroupInfo* m_groupInfo;
    /* 0134 */ C3Vector* m_portalVertexList;
    /* 0138 */ SMOPortal* m_portalList;
    /* 013C */ SMOPortalRef* m_portalRefList;
    /* 0140 */ C3Vector* m_visBlockVertList;
    /* 0144 */ SMOVisibleBlock* m_visBlockList;
    /* 0148 */ SMOLight* m_lightList;
    /* 014C */ SMODoodadSet* m_doodadSetList;
    /* 0150 */ char* m_doodadNameList;
    /* 0154 */ SMODoodadDef* m_doodadDefList;
    /* 0158 */ SMOFog* m_fogList;
    /* 015C */ C4Plane* m_convexVolumePlanes;
    /* 0160 */ SMOMaterial* m_materialList;
    /* 0164 */ int32_t m_texturesSize;
    /* 0168 */ int32_t m_groupNameSize;
    /* 016C */ int32_t m_groupInfoCount;
    /* 0170 */ int32_t m_planeVertCount;
    /* 0174 */ int32_t m_portalsCount;
    /* 0178 */ int32_t m_portalRefCount;
    /* 017C */ int32_t m_visBlockVertCount;
    /* 0180 */ int32_t m_visBlockCount;
    /* 0184 */ int32_t m_ligtsCount;
    /* 0188 */ uint32_t m_doodadSetCount;
    /* 018C */ int32_t m_doodadNameSize;
    /* 0190 */ int32_t m_doodadDefCount;
    /* 0194 */ int32_t m_fogsCount;
    /* 0198 */ int32_t m_convexVolumePlaneCount;
    /* 019C */ int32_t m_materialsCount;
    /* 01A0 */ CImVector m_argbColor;
    /* 01A4 */ int32_t unused_01A4;
    /* 01A8 */ CAaBox m_bbox;
    /* 01C0 */ float m_distToCamera;
    /* 01C4 */ TSLink<CMapObj> m_asyncLoadLink;
    /* 01CC */ void* m_pWmoData;
    /* 01D0 */ int32_t m_wmoFileSize;
    /* 01D4 */ int32_t m_refCount;
    /* 01D8 */ float m_flushTimer;
    /* 01DC */ CAsyncObject* m_asyncObject;
    /* 01E0 */ int32_t m_isGroupLoaded;
    /* 01E4 */ uint16_t m_interiorGroupCount;
    /* 01E6 */ uint8_t unused_01E6[2];
    /* 01E8 */ TSExplicitList<CMapObjGroup, offsetof(CMapObjGroup, m_asyncLoadLink)> m_mapObjGroupList;
    /* 01F4 */ int32_t m_mapObjGroupCount;
    /* 01F8 */ CMapObjGroup* m_mapObjGroupArray [512];

    CMapObj();
    ~CMapObj();
    void Clear();
    void UpdateMaterials();
    float DistFromClosestExtPortal(CMapObjGroup* group, const C3Vector& pos, int32_t exteriorLit);
    void DistFromClosestExtPortal(uint32_t depth, uint32_t flags, CMapObjGroup* group, CMapObjGroup* fromGroup, const C3Vector& pos, float* dist);
    void ExteriorRender(CMapObjGroup* group, uint32_t index);
    void InteriorRender(CMapObjGroup* group, uint32_t index);
    void RenderGroupLightmapTex(CMapObjGroup* group, uint32_t index);
    void RenderGroupColorTex(CMapObjGroup* group, uint32_t index);
    void RenderGroupCollisionFaces(CMapObjGroup* group, uint32_t index);
    void RenderGroupDetailFaces(CMapObjGroup* group, uint32_t index);
    void RenderGroupRenderFaces(CMapObjGroup* group, uint32_t index);
    void RenderGroupTransFaces(CMapObjGroup* group, uint32_t index);
    void RenderGroupCollidable(CMapObjGroup* group, uint32_t index);
    void Init();
    void InitPtrs();
    bool Read(char* fileName);
    void ReadGroup(uint32_t index);
    void ReadGroup(uint32_t index, bool preLoad);
    void Load();
    void WaitLoad();
    void WaitLoadGroup(uint32_t index);
    void GetBounds(CAaBox* box);
    void GetBounds(CAaSphere& sphere);
    void GetGroupBounds(CAaSphere* sphere, int32_t index);
    void GetGroupBounds(CAaBox* box, int32_t index);
    int32_t GetDoodadSet(uint32_t index);
    uint32_t GetGroupFlags(int32_t index);
    static uint32_t sMinimapTag;
    bool QueryMapObjMinimap(int32_t groupNum, CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint8_t indoorArea);
    void QueryMapObjMinimapGroup(int32_t groupNum, int32_t fromGroup, CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint32_t flagsMask, uint32_t depth);
    void GetMinimapQuads(CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads);
    SMOGroupInfo* GetGroupInfo(int32_t index);
    int32_t IsGroupLoaded(int32_t index);
    char* GetGroupName(int32_t index);
    CMapObjGroup* GetGroup(int32_t index, int32_t a3);
    int32_t IsGroupLoading(int32_t index);
    void RenderGroup(int32_t groupIndex, C44Matrix& matrix, TSExplicitList<CWFrustum, offsetof(CWFrustum, m_link)>* frustumList);
    void CreateMaterial(uint8_t texture);
    void CreateMaterials();
    bool TestBounds(CAaBox& box);
    bool TestBounds(C3Vector& start, C3Vector& end);
    bool VectorIntersect(C3Vector& start, C3Vector& end, uint32_t flags, uint32_t groupIgnoreFlags, float* t, uint16_t* hitFace, CMapObjDef* mapObjDef, float* normalDot, uint32_t* hitGroup);
    bool ContainsPointInBounds(C3Vector& point);
    bool TestGroupBounds(C3Vector& start, C3Vector& end, uint32_t groupNum);
    bool TestGroupBounds(const C3Vector& point, uint32_t groupNum);
    bool TestGroupBounds(CAaBox& box, uint32_t groupNum, int32_t a3);
    bool GroupBoundingBoxIntersectsSphere(C3Vector& pos, uint32_t groupNum, float radius);
    bool GetTris(const CWFrustum& frustum, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool GetTris(const CAaBox& box, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef);
    bool GetLiquidFacets(const CAaBox& box, uint32_t flags, World::FacetData* facets, CMapObjDef* mapObjDef);
    bool VectorIntersectPortal(C3Segment& seg, float* t, int* outGroups, int useSphereTest);
    bool VectorIntersectPortals(uint32_t groupNum, C3Segment& seg, float* t, int32_t* outGroups);
    float CalcPortalFarthestDistance(SMOPortal* portal);
    void InvokeRenderCallback(uint32_t groupNum);
    void UnifiedRender(CMapObjGroup* group, uint32_t index);
    void RenderPortals(CMapObjGroup* group);
    CMapObjGroup* AttenTransVerts(CMapObjGroup* group);
    void SetupMapObjDefCull(C44Matrix& mat, C44Matrix& invMat, C3Vector& worldPos, C3Vector& camTarget);
    void ClassifyPortalPlane(SMOPortal* portal, SPortalExt* portalExt);
    uint32_t TransformAndClipVerts(uint32_t noOcclusion, C3Vector* verts, uint32_t vertCount, C3Vector& offset, C3Vector*& clippedVerts, uint32_t& clippedCount);
    void AddExteriorPortalView(SMOPortal* portal, SMOPortalRef* ref, SPortalExt* ext, uint32_t destIsExterior);
    void TransformPortal(SMOPortal* portal, SPortalExt* portalExt);
    void AddInteriorPortalView(SMOPortal* portal, SMOPortalRef* ref, SPortalExt* ext, TSGrowableArray<CPortalView>* views);
    void RenderThruPortals(uint32_t groupNum, uint32_t fromGroup, CRect& ndcRect, uint32_t depth, int32_t interior);
    void RenderInterior(C44Matrix& mat, C44Matrix& invMat, C3Vector& worldPos, C3Vector& camTarget, TSGrowableArray<uint32_t>* groups);
    bool VectorIntersectPortal(const C3Segment& seg, float* t, uint32_t* groups, int32_t useSphere);
    void RenderThruPortalsExterior(C44Matrix& mat, C44Matrix& invMat, C3Vector& worldPos, C3Vector& camTarget, CRect& ndcRect, int32_t groupNum);
    bool Intersect(C3Vector& start, C3Vector& end, float* distance, uint32_t flags, uint32_t ignoreFlags, uint32_t groupNum, int32_t* hitIndex);
    SMOMaterial* GetLiquidMaterial(int32_t groupIndex);
    bool QueryLiquidStatus(uint32_t ignoreFlags, const C3Vector& pos, int32_t* liquidType, float* level);
    void QueryLiquidSounds(uint32_t groupIndex, uint32_t fromGroup, uint32_t depth, uint32_t* minDepth, const C3Vector& pos, TSGrowableArray<World::LiquidQueryResult>& results);
    uint16_t BuildLiquidVerts(CMapObjGroup* group, const C44Matrix& matrix, const CImVector* color, int32_t magma, float texCoord1U, const TSGrowableArray<uint8_t>* sharedVerts, uint32_t stride, C3Vector** position, C3Vector** normal, CImVector** vertexColor, C2Vector** texCoord0, C2Vector** texCoord1);
    void BuildLiquidIndices(CMapObjGroup* group, const TSGrowableArray<uint8_t>* sharedVerts, uint16_t** indices, uint16_t baseVertex);
    uint16_t ClipGroupGeometry(CMapObjGroup* group, const C44Matrix& matrix, const CImVector* color, int32_t magma, float texCoord1U, uint32_t stride, C3Vector** position, C3Vector** normal, CImVector** vertexColor, C2Vector** texCoord0, C2Vector** texCoord1, uint16_t** indices, uint16_t baseVertex);
};

struct SMOBatch {
    /* 0000 */ int16_t m_bx;
    /* 0002 */ int16_t m_by;
    /* 0004 */ int16_t m_bz;
    /* 0006 */ int16_t m_tx;
    /* 0008 */ int16_t m_ty;
    /* 000A */ int16_t m_tz;
    /* 000C */ uint32_t m_indexStart;
    /* 0010 */ uint16_t m_indexCount;
    /* 0012 */ uint16_t m_vertexStart;
    /* 0014 */ uint16_t m_vertexEnd;
    /* 0016 */ uint8_t m_flags;
    /* 0017 */ uint8_t m_texture;
};

struct SMODoodadSet {
    /* 0000 */ char m_name [20];
    /* 0014 */ uint32_t m_startIdx;
    /* 0018 */ uint32_t m_count;
    /* 001C */ uint32_t m_pad;
};

struct SMOGroupInfo {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ CAaBox m_boundingBox;
    /* 001C */ uint32_t m_nameoffset;
};

struct SMOHeader {
    /* 0000 */ uint32_t m_nTextures;
    /* 0004 */ uint32_t m_nGroups;
    /* 0008 */ uint32_t m_nPortals;
    /* 000C */ uint32_t m_nLights;
    /* 0010 */ uint32_t m_nDoodadNames;
    /* 0014 */ uint32_t m_nDoodadDefs;
    /* 0018 */ uint32_t m_nDoodadSets;
    /* 001C */ uint32_t m_ambColor;
    /* 0020 */ uint32_t m_wmoID;
    /* 0024 */ CAaBox m_boundingBox;
    /* 003C */ uint16_t m_flags;
    /* 003E */ uint16_t m_numLod;
};

struct SMOLight {
    /* 0000 */ uint8_t m_type;
    /* 0001 */ uint8_t m_atten;
    /* 0002 */ uint8_t m_pad [2];
    /* 0004 */ CImVector m_color;
    /* 0008 */ C3Vector m_position;
    /* 0014 */ float m_intensity;
    /* 0018 */ C4Quaternion m_rotation;
    /* 0028 */ float m_attenStart;
    /* 002C */ float m_attenEnd;
};

struct SMOMaterial {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ uint32_t m_shader;
    /* 0008 */ uint32_t m_blendMode;
    /* 000C */ uint32_t m_texture1;
    /* 0010 */ CImVector m_sidnColor;
    /* 0014 */ CImVector m_frameSidnColor;
    /* 0018 */ uint32_t m_texture2;
    /* 001C */ CImVector m_diffColor;
    /* 0020 */ uint32_t m_groundType;
    /* 0024 */ uint32_t m_texture3;
    /* 0028 */ uint32_t m_color2;
    /* 002C */ uint32_t m_flags2;
    /* 0030 */ uint32_t m_runTimeData [4];

    CTexture** Textures();
};

inline CTexture** SMOMaterial::Textures() {
    return reinterpret_cast<CTexture**>(&this->m_runTimeData[4 - 2 * sizeof(CTexture*) / sizeof(uint32_t)]);
}

struct SMOPortal {
    /* 0000 */ uint16_t m_startVertex;
    /* 0002 */ uint16_t m_count;
    /* 0004 */ C4Plane m_plane;
};

struct SMOVisibleBlock {
    /* 0000 */ uint16_t m_firstVertex;
    /* 0002 */ uint16_t m_count;
};

extern uint32_t s_portalStamp;
extern CMapObjDef* s_stampedMapObjDef;

#endif

