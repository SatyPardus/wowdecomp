#ifndef WOW_SOURCE_WORLDCLIENT_WORLD_H
#define WOW_SOURCE_WORLDCLIENT_WORLD_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/CBoundingBox.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "Engine/Source/Tempest/CFacet.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Object/WGUID.h"

class CAaSphere;
class CFrustum;
class CM2Lighting;
class CM2Model;
class CMapBaseObjLink;
class CMapObj;
class CMapObjDef;
class CMapObjDefGroup;
class CPortalView;
struct SMOPortal;
struct SMOPortalRef;

class CAaBox;
class C44Matrix;
class CiRect;
class GxuScene;

namespace Liquid {
    class CRenderer;
}
class CImVector;
class CM2Scene;
class CMapEntity;
struct MINIMAPQUADINFO;
class CTexture;
class Particulate;
class Weather;

typedef void (*CALLBACK_FUNC)(float, void*);

class CChunkLiquid;
class CDetailDoodadInst;
class CMapBaseObj;
class CMapChunk;
class CMapDoodadDef;
class CMapDoodadDefMapChunkLink;

class CMapDoodadDefMapChunkLink {
    public:
    /* 0000 */ uint32_t m_objectIndex;
    /* 0004 */ CMapDoodadDef* m_owner;
    /* 0008 */ CMapBaseObj* m_ref;
    /* 000C */ TSLink<CMapDoodadDefMapChunkLink> m_refLink;
    /* 0014 */ TSLink<CMapDoodadDefMapChunkLink> m_ownerLink;
};

class CDestructibleProxy;
class CWorldAntiOccluder;
class CWorldOccluder;
class CBarrier;
class CSortTable;
class CMapAreaLow;
struct SMOFog;

class CSortEntry {
    public:
    /* 0000 */ TSList<CMapChunk, TSGetExplicitLink<CMapChunk> > m_mapChunkList;
    /* 000C */ TSList<CMapObjDefGroup, TSGetExplicitLink<CMapObjDefGroup> > m_exteriorGroupList;
    /* 0018 */ TSList<CMapEntity, TSGetExplicitLink<CMapEntity> > m_entityList;
    /* 0024 */ TSList<CMapDoodadDef, TSGetExplicitLink<CMapDoodadDef> > m_doodadDefList;
    /* 0030 */ TSList<CChunkLiquid, TSGetExplicitLink<CChunkLiquid> > m_liquidList;
    /* 003C */ TSList<CWorldOccluder, TSGetExplicitLink<CWorldOccluder> > m_occluderList;
    /* 0048 */ TSList<CWorldAntiOccluder, TSGetExplicitLink<CWorldAntiOccluder> > m_antiOccluderList;
    /* 0054 */ TSList<CMapChunk, TSGetExplicitLink<CMapChunk> > m_holedChunkList;
    /* 0060 */ TSList<void> m_unkList9;

    CSortEntry();
};

struct OcclusionVolumeData {
    /* 0000 */ CAaBox m_bounds;
    /* 0018 */ C3Vector* m_positions;
    /* 001C */ uint32_t m_positionCount;
};

struct WorldOccluderData {
    /* 0000 */ int32_t m_mapId;
    /* 0004 */ CAaBox m_bounds;
    /* 001C */ C3Vector* m_verts;
    /* 0020 */ uint32_t m_count;

    WorldOccluderData(int32_t mapId, float depth, C3Vector* verts, uint32_t count);
};

struct OcclusionVolume {
    /* 0000 */ int32_t m_mapID;
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ OcclusionVolumeData m_data;
};

struct SPortalExt {
    /* 0000 */ uint16_t m_flags;
    /* 0002 */ uint16_t unused_0002;
    /* 0004 */ CRect m_rect;
    /* 0014 */ uint32_t m_stamp;
    /* 0018 */ uint32_t m_pad;
};

struct WorldStatus {
    /* 0000 */ uint32_t m_prepareAll;
    /* 0004 */ uint32_t m_chunksChanged;
};

struct SWModelFadeout {
    /* 0000 */ CMapEntity* m_entity;
    /* 0004 */ uint32_t m_startTime;
    /* 0008 */ float m_alpha;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ WOWGUID m_transportGuid;
    /* 0018 */ C44Matrix m_localTransform;
    /* 0058 */ TSLink<SWModelFadeout> m_link;
};

extern TSExplicitList<SWModelFadeout, offsetof(SWModelFadeout, m_link)> s_fadeoutList;
extern TSExplicitList<SWModelFadeout, offsetof(SWModelFadeout, m_link)> s_fadeoutDataPool;

class CWorldScene {
    public:
    static bool IsBeyondFadeDistance(const C3Vector& position, uint32_t fadeLevel);
    static Liquid::CRenderer* s_liquidRenderer;
    static TSExplicitList<CWFrustum, offsetof(CWFrustum, m_link)> frustumFreeList;
    static int32_t frustumIndex;
    static C3Vector s_frustumCorners[8];

    static void GetFrustumCorners(C3Vector* corners);
    static CWFrustum frustumStack[32];
    static GxuScene s_unk_B2EB68;
    static CTexture* s_defaultBlendTexture;
    static CTexture* s_defaultTexture;
    static int32_t s_hasPendingExteriorGroups;
    static int32_t s_fogPermute;
    static float s_fogFovPadding;
    static CMapEntity* camTargetEntity;
    static C3Vector s_activeWorldView;
    static C3Vector camTarget;
    static C4Plane camPlaneXY;
    static C4Plane camPlane;
    static C44Matrix viewMatrix;
    static C44Matrix camTransportView;
    static C44Matrix camTransportMtx;
    static WOWGUID camTransportGUID;
    static C44Matrix s_viewProjMatrix;
    static C4Vector s_viewProjW;
    static CBoundingBox viewPort;
    static CFrustum s_clipFrustum;
    static CAaBox boundingBox;
    static CiRect s_frustumChunkRect;
    static CPortalView frustumPortalView;
    static CPortalView s_unk_ADF570;
    static char* s_interiorSkybox;
    static int32_t s_unk_CD8620;
    static float s_lodFarClip;
    static C3Vector camVec;
    static C44Matrix projMatrix;
    static C44Matrix s_clipBufferMatrix;
    static float s_clipBuffer[384];
    static uint8_t s_clipBufferMask[384];
    static uint32_t s_chunksRendered;
    static float s_occlusionNearDepth;
    static float s_occlusionFarDepth;
    static uint32_t nDoodadsRendered;
    static CBarrier s_barrier;
    static CSortTable sortTable;
    static TSGrowableArray<CPortalView> s_exteriorPortalViews;
    static TSGrowableArray<CPortalView> s_portalViews;
    static CMapObjDef* s_curMapObjDef;
    static CMapObjDef* s_viewerMapObjDef;
    static CMapObjDef* s_viewerMovedMapObjDef;
    static TSList<void, TSGetExplicitLink<void> > s_unkList9FreeList;
    static TSGrowableArray<uint32_t> s_viewerMovedMapObjGroups;
    static TSGrowableArray<uint32_t> s_viewerMapObjGroups;
    static float s_fadeDist[5];
    static float s_fadeDistDefault[5];
    static float s_fadeDistMin[5];
    static float s_fadeDistMinSqr[5];
    static float s_fadeRangeDefault[5];
    static float s_fadeSizeDefault[5];
    static int32_t s_cullPassStamp;

    static void ClearObjectFadeoutList();
    static uint32_t s_unused_CD8624;
    static uint32_t s_unused_CD8774;

    static void BuildViewMatrices(const C3Vector& position);
    static void RenderHorizon();
    static void UpdateSortedModels();
    static void RenderEntitys();
    static void CullModelSplitBody(CM2Model* model, uint32_t boneId, int32_t skip);
    static void CullModelRecursive(CM2Model* model);
    static void RequestModelStreaming(CM2Model* model, const CAaBox* box);
    static void RenderStreamingRequestDecal(CM2Model* model, const CAaBox* box, float scale);
    static void RenderBarriers(float elapsedSec);
    static void RenderParticulate();
    static void RenderWeather();
    static void Destroy();
    static void Initialize();
    static void ScaleFadeDist(float scale);
    static void Update(C3Vector* camPos, C3Vector* camTarget);
    static void GetNearestCornerToCamera(CAaBox* box, C3Vector* outCorner);
    static void __fastcall AddMapChunk(CMapChunk* mapChunk);
    static void AddMapObjDefGroup(CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup);
    static void ComputeExteriorSortDistances();
    static void AddMapChunkToRenderList(CMapChunk* mapChunk, C3Vector* pos);
    static void AddMapObjDefGroupToSortTable(uint32_t groupIndex, void* param);
    static void MergeIntoWorldRect(CPortalView* portalView);
    static void MergeIntoFrustumRect(CPortalView* portalView);
    static void AddEntityToSortTable(CMapEntity* entity);
    static CWFrustum* AllocFrustum();
    static int32_t FrustumCull(CAaBox* box);
    static int32_t FrustumCull(const C3Vector& center, float radius);
    static void __fastcall FrustumSet(const C3Vector* corners, const CRect& rect);
    static void FrustumSet(CRect* rect);
    static void FrustumSet(CFrustum* frustum);
    static void FrustumPush();
    static void FrustumPop();
    static void FrustumXform(C44Matrix& mat);
    static bool InsideFrustumRect(CiRect* rect);
    static C3Vector* GetCamTarget();
    static void ApplyFogState();
    static void RenderLiquidPass();
    static void CullDoodads(CSortEntry* entry, uint16_t fadeLevel);
    static void CullLiquid(CSortEntry* entry);
    static void CullChunks(CSortEntry* entry, uint32_t index);
    static void UpdateOcclusion(CSortEntry* entry);
    static void CullSortTable(CRect* rect);
    static void CullHorizon(CRect* rect);
    static void BuildHorizonProjection(C44Matrix* projection);
    static void AddHorizonChunk(CMapAreaLow* area);
    static void AddHorizonMapObjDef(CMapObjDef* def);
    static void AddWorldOccluders();
    static void CreateOcclusionVolumes(const C3Vector* eye, const C3Vector* corners, int32_t onlyFlagged);
    static void PrepareLiquidInstances();
    static void AddChunkToSortList(CDetailDoodadInst* inst);
    static void AddToDistanceBucket(CChunkLiquid* liquid, const C3Vector& pos);
    static void AddDoodadDefModelToModelScene(CMapDoodadDef* doodadDef);
    static void AddDoodadDefs(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* linkList, uint32_t index);
    static void CullDoodadsExterior(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* linkList, uint8_t fadeLevel);
    static void CullMapObjDefGroup();
    static void CullMapObjDefGroups(CSortEntry* entry, CRect* a2, uint32_t a3);
    static void CullEntitys(CSortEntry* entry);
    static void CullThroughPortal(CRect* rect);
    static void CreateInversePortalList();
    static void RenderPortalList(TSGrowableArray<CPortalView>* views, int32_t interior);
    static void UpdateViewerLiquid();
    static void CullDoodadsInterior(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* linkList, CWFrustum* frustum, uint16_t fadeLevel, int32_t interior);
    static void CullEntitysInterior(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* linkList, CWFrustum* frustum, int32_t force, int32_t interior);
    static void CullChunks(CSortEntry* entry, int32_t index);
    static void Render(const C3Vector& cameraPos, float time);
    static void RenderChunks();
    static void RenderMapObjDefGroups();
    static void RenderDetailDoodads();
    static void RenderOccluders();
    static void RenderDebugFrustum();
    static void RenderOcclusionVolumes();
    static void SetWorldProjection(C44Matrix& mat);
    static void RenderChunksSolid();
    static void RenderChunksZoneDebug();
    static void RenderChunksSinglePass();
    static void PushExteriorPortalView(CPortalView* portalView);
    static void PushPortalView(CPortalView* portalView);
    static float DistanceToCamPlaneXY(const C3Vector* pos);
    static void __stdcall ClipVerts(C3Vector* verts, uint32_t count, C3Vector** outVerts, uint32_t* outCount);
    static void __stdcall CalcScreenRectFromVerts(CRect& rect, C3Vector* clippedVerts, uint32_t clippedCount);
    static void LocateViewer3();
    static void RenderCollisionDebug();
    static uint32_t AddViewerGroup(TSGrowableArray<uint32_t>* groups, uint32_t group);
    static void SetupLighting(CM2Lighting* lighting, C3Vector* view);
    static void SetCameraTarget(CMapEntity* entity, WOWGUID transportGuid);

    /* 0000 */ uint8_t unused_0000[16];
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint8_t unused_0014[24];
    /* 002C */ uint32_t unused_002C;
    /* 0030 */ uint8_t unused_0030[28];
    /* 004C */ uint32_t unused_004C;
    /* 0050 */ uint8_t unused_0050[32];
    /* 0070 */ uint32_t unused_0070;
    /* 0074 */ uint8_t unused_0074[4];
    /* 0078 */ uint32_t unused_0078;
    /* 007C */ uint32_t unused_007C;
    /* 0080 */ uint8_t unused_0080[8];
    /* 0088 */ uint32_t unused_0088;
    /* 008C */ uint8_t unused_008C[36];
    /* 00B0 */ uint32_t unused_00B0;
    /* 00B4 */ uint8_t unused_00B4[64];
    /* 00F4 */ uint32_t unused_00F4;
    /* 00F8 */ uint8_t unused_00F8[36];
    /* 011C */ uint32_t unused_011C;
    /* 0120 */ uint8_t unused_0120[16];
    /* 0130 */ uint32_t unused_0130;
    /* 0134 */ uint32_t unused_0134;
    /* 0138 */ uint8_t unused_0138[52];
    /* 016C */ uint32_t unused_016C;
    /* 0170 */ uint8_t unused_0170[84];
    /* 01C4 */ uint32_t unused_01C4;
    /* 01C8 */ uint8_t unused_01C8[28];
    /* 01E4 */ uint32_t unused_01E4;
};

extern C3Vector s_camPosLocal;
extern C3Vector s_camTargetLocal;
extern C4Plane s_camPlaneLocal;
extern C44Matrix s_modelViewProj;
extern C44Matrix s_viewProj;
extern C44Matrix s_modelView;
extern C44Matrix s_mapObjToWorld;
extern SPortalExt s_portalExt[2048];
extern int32_t s_curGroupIsInterior;
extern int32_t s_cullStateValid;
extern int32_t s_interiorPass;
extern uint32_t s_maxPortalDepth;
extern int32_t s_entityCanLink;
extern WOWGUID s_highlightedObjectGuid;

class WMOAreaTableRec;

namespace World {
    class TriData;
    class HitTestDebug {
        public:
        struct Model {
            /* 0000 */ const char* m_name;
            /* 0004 */ int32_t m_depth;
            /* 0008 */ CAaBox m_bbox;
            /* 0020 */ CAaSphere m_sphere;
            /* 0030 */ uint32_t m_numGeometryBatches;
            /* 0034 */ uint32_t m_numPrimitives;
            /* 0038 */ uint32_t m_numOtherBatches;
            /* 003C */ uint32_t m_numBones;
            /* 0040 */ uint32_t m_numLights;
            /* 0044 */ uint32_t m_numCurrentLights;
            /* 0048 */ uint32_t m_attachmentId;
            /* 004C */ float m_attachmentScale;
            /* 0050 */ CM2Model* m_model;
            /* 0054 */ uint32_t m_numCombinableBatches;
            /* 0058 */ uint32_t m_instanceCountMax;
            /* 005C */ const char* m_groupName;
            /* 0060 */ uint32_t m_groupNum;
            /* 0064 */ uint32_t m_groupFlags;
            /* 0068 */ int32_t m_doodadSet;
        };

        struct Parent {
            /* 0000 */ const char* m_name;
            /* 0004 */ const char* m_groupName;
            /* 0008 */ C2iVector m_chunkOffset;
        };

        /* 0000 */ int32_t m_type;
        /* 0004 */ uint32_t m_numModels;
        /* 0008 */ uint32_t m_numParents;
        /* 000C */ Model m_models[16];
        /* 06CC */ Parent m_parents[8];
        /* 074C */ CAaBox m_bbox;
        /* 0764 */ CAaSphere m_sphere;
        /* 0774 */ C3Vector m_labelPos;
    };

    struct LiquidQueryResult {
        /* 0000 */ int32_t m_liquidType;
        /* 0004 */ C3Vector m_offset;
    };

    extern CALLBACK_FUNC s_loadProgressCallback;
    extern void* s_loadProgressParam;
    extern C3Vector s_currentWorldPos;

    void GetFrustumCorners(C3Vector* corners);
    void PrepareAreaOfInterest(const C3Vector& position);
    void SetLoadProgressCallback(CALLBACK_FUNC callback, void* param);
    void UnloadMap();

    class ObjectCollisionData {
        public:
        /* 0000 */ CM2Model* m_model;
        /* 0004 */ CAaBox m_bounds;
        /* 001C */ float m_scale;
        /* 0020 */ C44Matrix m_mat;

        ObjectCollisionData();
    };

    extern int32_t (*s_objectCollisionCallback)(WOWGUID* guid, uintptr_t param, ObjectCollisionData* data, uint32_t flags);

    C3Vector* GetCamPos();
    void SetObjectCollisionCallback(int32_t (*callback)(WOWGUID* guid, uintptr_t param, ObjectCollisionData* data, uint32_t flags));
    void FindOrCreateDestructibleProxy(CDestructibleProxy* proxy);
    void CreateWaterRipple(const C3Vector& pos, float a2, float a3, float a4, float a5, float a6, int32_t a7, int32_t a8);
    int32_t QueryLiquidSounds(CMapEntity* entity, float radius, TSGrowableArray<World::LiquidQueryResult>& results);
    int32_t QueryLiquidStatus(C3Vector* pos, int32_t* liquidType, float* height);
    int32_t ObjectIsDrawable(CMapBaseObj* object);
    int32_t QueryObjectInside(CMapEntity* entity);
    int32_t QueryAreaIdAtPosition(const C3Vector* position, int32_t* areaId);
    double GetFramerate();
    bool QueryMapObjIDs(CMapEntity* entity, int32_t* wmoId, int32_t* wmoHash, int32_t* groupNum);
    int32_t QueryMapObjZoneName(CMapEntity* entity, const char** zoneName);
    int32_t QueryMapObjSubzoneName(CMapEntity* entity, const char** subzoneName, int32_t* areaId);
    bool QueryMapObjAreaTable(CMapEntity* entity, WMOAreaTableRec** groupRec, WMOAreaTableRec** rootRec, int32_t* wmoGroupId);
    int32_t QueryMapObjFileName(CMapEntity* entity, const char** fileName);
    bool QueryMapObjMinimap(CMapEntity* entity, CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint8_t indoorArea);
    bool QueryMapObjMatrix(CMapEntity* entity, C44Matrix* matrix, C44Matrix* invMatrix);
    void Preload(const C3Vector& position, int32_t a2);
    int32_t QueryMapId();
    void UpdateObjectFadeouts();
    void Update(const C3Vector& position);
    int32_t QueryOutdoors(CMapEntity* entity);
    int32_t QueryObjectLiquid(CMapEntity* entity, int32_t* liquidType, float* height, int32_t* a4);
    void NewFootprint(int32_t textureId, const float* size, const C3Vector* position, float facing, int32_t left, int32_t terrainType, int32_t activePlayer);
    int32_t QueryGroundType(CMapEntity* entity, int32_t* groundType);
    int32_t QueryObjectLiquidFlags(CMapEntity* entity, int32_t* flag8, int32_t* flag9);
    int32_t SceneCamLiquidStatus(float* depth);
    int32_t QueryCameraFog(SMOFog* fog, CMapObjDef** mapObjDef, uint8_t* found, TSGrowableArray<uint32_t>** groups, float* distance);

    class FacetData {
        public:
        TSGrowableArray<CFacet> facets;
        TSGrowableArray<uint64_t> facetIds;

        FacetData() {
            this->facets.m_chunk = 256;
            this->facetIds.m_chunk = 256;
        }
    };

extern CM2Scene* s_m2Scene;
extern Weather* s_weather;
extern float groundEffectDistValueSqr;
extern C4Vector texVect[8];
extern int32_t lodMax;
void SetFrameCntCallback(void (*callback)());
extern int32_t lodMin;
extern C44Matrix idMat;
extern float s_farClip;
extern float s_horizonFarClipScale;
extern float s_horizonNearClipScale;
extern int32_t s_detailDoodadDensity;
extern float s_nearClip;
extern float s_prevFarClip;
extern int32_t s_texCacheDirty;
extern uint32_t s_texCacheOverride;
extern int32_t s_waterLOD;

int32_t FrustumCull(const CAaBox& box);
int32_t ProjectPositionCallback(const C3Vector& position, float& height, void* param);
void ProjectTextureCallback(const CAaBox& box, const CImVector& color, int32_t index, void* param, int32_t flags);
bool MapIsDungeon();
void ObjectDestroy(CMapEntity* entity);
void ObjectSetUpdateCallback(CMapEntity* entity, int32_t (*func)(void*, uint32_t, uint64_t, uintptr_t), void* param);
void SetDetailDoodadDensity(int32_t density);
void SetDetailDoodadDist(float dist);
void SetFarClip(float farClip);
void SetHorizonFarClipScale(float scale);
void SetHorizonNearClipScale(float scale);
void SetNearClip(float nearClip);
void SetTexLodBias(float bias);
float ValidateFarClip(float farClip, int32_t mapID);
    bool NDCClip(C3Vector* verts, uint32_t count, C3Vector*** outVerts, uint32_t* outCount);
    bool GetFacets(CAaBox* a1, CAaBox* a2, FacetData* a3, uint32_t a4, uint32_t* a5);
    bool NDCXform(CFrustum* frustum, C44Matrix* out, bool includeTranslation);
    bool IsValidPosition(float x, float y, float z, float a4);
    bool IsValidPosition(const C3Vector& position, float a4);
    void AddAaBoxFacets(CAaBox* box, FacetData* facets);
    uint32_t TriDataToFacetData(void* unused, FacetData* facets, WOWGUID guid);
    bool GetFacets(CAaBox* box, FacetData* facets, uint32_t flags, uint32_t* status);
    bool GetFacets(CFrustum* frustum, FacetData* facets, uint32_t flags, uint32_t* status);
    int32_t GetFlightBoundsLower(const C3Vector& pos, float* height);
    int32_t Intersect(const C3Vector& start, const C3Vector& end, C3Vector* hit, float* distance, uint32_t flags, HitTestDebug* debug);
    void ObjectSetModel(CMapEntity* entity, CM2Model* model);
    void ObjectSetMoveable(CMapEntity* entity, int32_t moveable);
    int32_t ObjectIsLoaded(CMapBaseObj* object);
    int32_t ObjectIsWMO(CMapBaseObj* object);
    void ObjectSetHidden(CMapEntity* entity, int32_t hidden);
    void ObjectEnableCollision(CMapBaseObj* object, int32_t enable);
    void FreeDestructibleProxy(CDestructibleProxy* proxy);
    void ObjectUpdate(CMapBaseObj* object, const C3Vector* position, float facing, float a4, float a5, int32_t snap);
    void ObjectSetAlwaysExterior(CMapBaseObj* object, int32_t enable);
    int32_t ObjectMoveDoodads(CMapBaseObj* source, uint16_t doodadSet, CMapBaseObj* target, uint16_t targetDoodadSet);
    void ObjectSetDoodadEmittersIgnoresDistance(CMapBaseObj* object, int32_t ignore, uint16_t doodadSet);
    void ObjectSetDoodadsEnabled(CMapBaseObj* object, int32_t enable, uint16_t doodadSet);
    void ObjectSetEventCallback(CMapBaseObj* object, void (*callback)(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param), uint64_t param, uint16_t doodadSet);
    void ObjectGetExtents(CMapBaseObj* object, CAaBox* extents);
    uint64_t QueryObjectParentParam64(CMapBaseObj* object);
    void ObjectSetRenderEnabled(CMapBaseObj* object, int32_t enable);
    bool GetTris(const CAaBox& box, World::TriData* triData, uint32_t flags);
    uint32_t GetIntersectingDoodadModels(CM2Model** models, uint32_t maxModels, const CAaBox& box, uint32_t flags);
    void ObjectSetSequence(CMapEntity* entity, uint32_t sequence, int32_t a3, int32_t a4);
    void ObjectSetShadow(CMapEntity* entity, uint32_t shadow, uint32_t mask);
    void ObjectSetSequenceFinishedCallback(CMapEntity* entity, void (*callback)(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param), uint64_t param, uint16_t doodad);
    bool ObjectTestConvexVolume(CMapEntity* entity, const C3Vector& point);
    void ObjectUpdate(CMapEntity* entity, const C44Matrix& matrix);
}

class CWorld {
    public:
    static float s_unk_ADEEE4;
    static float s_unk_CD7758;

    static void Render(const C3Vector& cameraPos, float time);
    static int32_t bShowSimpleDoodads;
    static int32_t bLoadSimpleDoodads;
    static Particulate* particulate;
    static int32_t s_waterRipples;
    static int32_t shadowMipLevel;
    static int32_t s_worldPoolUsage;
    static uint32_t maxLights;
    static int32_t terrainAlphaBitDepth;
    static int32_t enableLayerCnt;
    static float farFog;
    static int32_t detailDoodadAlphaRef;
    static int32_t detailDoodadTest;
    static WorldStatus status;
    static int32_t chunkCnt;
    static int32_t frameCnt;
    static int32_t s_elapsedTicks;
    static float s_elapsedTimeSec;
    static uint32_t enables;
    static uint32_t enables2;
    static uint32_t m_M2PassMask;
    static CAaBox s_groupAreaOfInterest;
    static CAaBox s_objectAreaOfInterest;
    static CiRect chunkRect;
    static CiRect gbChunkRect;
    static CiRect gbChunkRectOld;
    static uint32_t shadowColor;
    static float detailDoodadDist;
    static CTexture* shadowModTexture;

    static void UpdateTextureCacheBudget();
    static void SetShadowColor(const CImVector& color);
    static int32_t ConsoleCommand_CharacterAmbient(const char* command, const char* arguments);
    static int32_t ConsoleCommand_DetailDoodadAlpha(const char* command, const char* arguments);
    static int32_t ConsoleCommand_MaxLOD(const char* command, const char* arguments);
    static int32_t ConsoleCommand_SetShadow(const char* command, const char* arguments);
    static int32_t ConsoleCommand_ShowCull(const char* command, const char* arguments);
    static int32_t ConsoleCommand_ShowDetailDoodads(const char* command, const char* arguments);
    static int32_t ConsoleCommand_ShowLowDetail(const char* command, const char* arguments);
    static int32_t ConsoleCommand_ShowShadow(const char* command, const char* arguments);
    static int32_t ConsoleCommand_ShowSimpleDoodads(const char* command, const char* arguments);
    static int32_t ConsoleCommand_WaterParticulates(const char* command, const char* arguments);
    static int32_t ConsoleCommand_WaterRipples(const char* command, const char* arguments);
    static void Destroy();
    static void Initialize();
    static void LoadMap(const char* mapName, C3Vector* position, int32_t zoneID, int32_t unused_a4);
    static void ObjectDestroy(CMapBaseObj* object);
    static CMapBaseObj* ObjectCreate(const char* name, const C3Vector& position, float facing, int32_t waitLoad, int32_t snap, uint64_t guid, uint32_t doodadSetCount, const int16_t* doodadSets, float* radius, uint32_t id);
    static void Update(C3Vector* camPos, C3Vector* camTarget, C3Vector* position);
};

extern float s_viewerLiquidDepth;
extern uint32_t s_liquidTypeId;

extern int32_t s_disableAreaAmbient;

#endif
