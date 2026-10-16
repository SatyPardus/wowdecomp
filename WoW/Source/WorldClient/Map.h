#ifndef WOW_SOURCE_WORLDCLIENT_MAP_H
#define WOW_SOURCE_WORLDCLIENT_MAP_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/MapLowDetail.h"
#include "WoW/Source/WorldClient/MapRenderChunk.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/Object/WGUID.h"

class C2Vector;
class C44Matrix;
class CAaSphere;
class CFacet;
class CFrustum;
class CWFrustum;
class CM2Model;
class CMapAreaTexture;
class CDetailDoodadInst;
class CMapBaseObj;
class CMapBaseObjLink;
class CMapChunk;
class CMapDoodadDef;
class CMapLight;
class CMapObjDef;
class CMapObjDefGroup;
class CMapObjGroup;
class CMapStaticEntity;
class CiRect;
class SFileImpl;
class SFile;
namespace World { class FacetData; class TriData; struct LiquidQueryResult; class ObjectCollisionData; class HitTestDebug; }

struct CGxBuf;
struct CGxPool;
class CGxShader;
class CChunkLiquid;
class CMapArea;
class FVBBList;
class CMapAreaMed;
class CMapEntity;
class LiquidTypeRec;
class CTexture;
struct SMMapHeader;

class CMapAreaEntry {
    public:
    /* 0000 */ CMapArea* m_area;
    /* 0004 */ float m_dist;
};

struct SMAreaInfo {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ uint32_t m_unused;
};

struct SMDoodadDef {
    /* 0000 */ uint32_t m_nameId;
    /* 0004 */ uint32_t m_uniqueId;
    /* 0008 */ C3Vector m_position;
    /* 0014 */ C3Vector m_rotation;
    /* 0020 */ uint16_t m_scale;
    /* 0022 */ uint16_t m_flags;
};

struct SMMapObjDef {
    /* 0000 */ uint32_t m_nameId;
    /* 0004 */ uint32_t m_uniqueId;
    /* 0008 */ C3Vector m_position;
    /* 0014 */ C3Vector m_rotation;
    /* 0020 */ CAaBox m_extents;
    /* 0038 */ uint16_t m_flags;
    /* 003A */ uint16_t m_doodadSet;
    /* 003C */ uint16_t m_nameSet;
    /* 003E */ uint16_t m_scale;
};

struct SMODoodadDef {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ C3Vector m_position;
    /* 0010 */ C4Quaternion m_orientation;
    /* 0020 */ float m_scale;
    /* 0024 */ CImVector m_color;
};

class CGxVertexPT0T1 {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C2Vector m_texture0;
    /* 0014 */ C2Vector m_texture1;
};

struct CMapFootprint {
    /* 0000 */ C3Vector m_position;
    /* 000C */ uint32_t m_alpha;
    /* 0010 */ uint32_t m_textureIndex;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint32_t m_startTime;
    /* 001C */ TSGrowableArray<CGxVertexPT0T1> m_vertices;
    /* 002C */ TSLink<CMapFootprint> m_link;
};

class CMapObj;

class CMapFootprintTexture {
    public:
    /* 0000 */ CTexture* m_texture;
    /* 0004 */ TSGrowableArray<uint32_t> m_footprintIndices;
    /* 0014 */ uint32_t m_vertexCount;

    CMapFootprintTexture();
};

typedef int32_t (*MAP_OBJECT_FUNC)(void*, uint32_t, uint64_t, uintptr_t);

struct MapObjIntersectCandidate {
    /* 0000 */ CMapObjDef* def;
    /* 0004 */ CMapObjDefGroup* group;
    /* 0008 */ float t;
};

struct MapObjIntersectData {
    /* 0000 */ CMapObjDef* def;
    /* 0004 */ CMapObjDefGroup* group;
    /* 0008 */ float t;
    /* 000C */ int16_t faceIndex;
    /* 000E */ int16_t interior;
};

class CDestructibleProxy {
    public:
    static void FindOrCreate(CDestructibleProxy* proxy);
    static void FreeMatching(CDestructibleProxy* proxy);

    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ TSLink<CDestructibleProxy> m_link;
    /* 000C */ CAaSphere m_sphere;
    /* 001C */ uint32_t m_uniqueId;
};

class CMap {
    public:
    static void UpdateDoodadDefTransform(CMapDoodadDef* doodadDef, const C44Matrix& matrix);
    static void UpdateDoodadDefBounds(CMapDoodadDef* doodadDef);
    static CMapObjDef* CreateMapObjDef(const char* fileName, const C3Vector* position, float facing, int32_t waitLoad, uint32_t doodadSetCount, const int16_t* doodadSets, float* radius, uint32_t id);
    static void SnapBaseObjToSubChunk(CMapBaseObj* baseObj, const C3Vector* position, float facing);
    static void UpdateDoodadDef(CMapDoodadDef* doodadDef, const C3Vector* position, float facing);
    static TSGrowableArray<CGxVertexPC> debugVertexArray;
    static TSGrowableArray<uint16_t> debugIndexArray;
    static CMapLight* s_mapLight;
    static CMapLowDetail s_lowDetail;
    static SMMapHeader header;
    static bool gTerrainPixelShadersValid;
    static bool enableTerrainShaderVertex;
    static uint32_t version;
    static int32_t bPreload;
    static int32_t s_isStreamingMode;
    static bool s_enableTriangleStrips;
    static bool enableSpecular;
    static bool enablePixelShaders;
    static bool enableVertexShaders;
    static TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > mapObjDefLinkList;
    static TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > s_unk_ADFBF8;
    static TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > s_globalLightList;
    static CGxBuf* lowDetailIndexBuf;
    static CGxPool* lowDetailIndexPool;
    static CGxShader* pixelShader_TerrainSM;
    static CGxShader* pixelShader_Terrain0_env;
    static CGxShader* vertexShader_Terrain[128];
    static CGxShader* pixelShader_Terrain3[96];
    static CGxShader* pixelShader_Terrain2[32];
    static CGxShader* pixelShader_Terrain1[32];
    static CGxShader* pixelShader_Terrain0[3];
    static int32_t uniqueId;
    static int32_t oldSelectLightParm;
    static int32_t mapGetFacetsCount;
    static int32_t bspRecurseCount;
    static uint32_t s_queryTag;
    static uint32_t cCount;
    static CMapAreaMed* unk[4096];
    static CMapArea* m_areaTable[4096];
    static SMAreaInfo m_areaInfo[4096];
    static TSGrowableArray<uint32_t> scCollideList;
    static int32_t bActive;
    static int32_t bDungeon;
    static int32_t s_useSSE;
    static int32_t freeCounts[11];
    static float s_liquidAlphaScale;
    static float s_depthCoordTable0[256];
    static float s_depthCoordTable1[256];
    static int32_t counts[11];
    static uint32_t* lightHeap;
    static uint32_t* cacheLightHeap;
    static uint32_t* mapObjGroupHeap;
    static uint32_t* mapObjHeap;
    static uint32_t* baseObjLinkHeap;
    static uint32_t* areaHeap;
    static uint32_t* areaMedHeap;
    static CMapAreaLowHeap* areaLowHeap;
    static uint32_t* chunkHeap;
    static uint32_t* doodadDefHeap;
    static uint32_t* entityHeap;
    static uint32_t* mapObjDefGroupHeap;
    static uint32_t* mapObjDefHeap;
    static uint32_t* chunkLiquidHeap;
    static FVBBList s_vbbList;
    static TSExplicitList<CMapRenderChunk, offsetof(CMapRenderChunk, m_renderChunkLink)> s_mapRenderChunkFreeList;
    static TSExplicitList<CMapChunkBuf, offsetof(CMapChunkBuf, m_freeLink)> s_chunkBufFreeList;
    static TSList<CMapArea, TSGetExplicitLink<CMapArea> > s_allocatedAreas;
    static TSList<CMapChunk, TSGetExplicitLink<CMapChunk> > s_mapChunkList;
    static TSList<CMapChunk, TSGetExplicitLink<CMapChunk> > s_mapChunkSortList;
    static TSList<CMapObjDefGroup, TSGetExplicitLink<CMapObjDefGroup> > mapObjDefGroupList;
    static TSList<CMapEntity, TSGetExplicitLink<CMapEntity> > entityList;
    static TSList<CMapLight, TSGetExplicitLink<CMapLight> > lightList;
    static TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > s_unk_ADFBEC;
    static TSList<CDetailDoodadInst, TSGetExplicitLink<CDetailDoodadInst> > s_detailDoodadUpdateList;
    static TSExplicitList<CMapRenderChunk, offsetof(CMapRenderChunk, m_renderChunkLink)> s_mapRenderChunkUpdateList;
    static bool enableSpecularTerrain;
    static char wdtFilename[256];
    static char mapName[256];
    static char mapPath[256];
    static C3Vector s_subchunkCoords[5];
    static uint32_t s_holeMask[16];
    static int32_t s_subchunkIndices[8];
    static int32_t s_fanIndices[8];
    static uint16_t s_2bitSplatMask[8];
    static uint32_t s_2bitSplatShft[8];
    static TSList<CMapDoodadDef, TSGetExplicitLink<CMapDoodadDef> > doodadDefList;
    static TSHashTable<CMapObjDef, HASHKEY_NONE> mapObjDefHash;
    static TSHashTable<CMapDoodadDef, HASHKEY_DWORD> doodadDefHash;
    static TSExplicitList<CDestructibleProxy, offsetof(CDestructibleProxy, m_link)> s_destructibleProxyList;
    static HASHKEY_NONE s_mapObjDefHashKey;
    static int32_t enableChunkBatching;
    static TSList<CChunkLiquid, TSGetExplicitLink<CChunkLiquid> > s_chunkLiquidList;
    static TSList<CChunkLiquid, TSGetExplicitLink<CChunkLiquid> > s_mapChunkLiquidUpdateList;
    static DepthCoordTable* s_depthCoordTables[2];

    static void ClearAllRenderChunks();
    static void ClearChunkDetailDoodads();
    static void Destroy();
    static void FootprintDestroy();
    static void FootprintInitialize();
    static void FreeMapObj(CMapObj* mapObj);
    static void Initialize();
    static void InitializeLiquidTables();
    static void InitializePCFShaders();
    static bool QueryAreaId(CMapEntity* entity, uint32_t& areaId);
    static void Unload();
    static void ValidateShaders();
    static CGxShader* GetPixelShader(bool a1, bool a2, bool a3);
    static CGxShader* GetTerrainPixelShader(int32_t bigAlpha, int32_t layers, int32_t shadowMode, int32_t a4, int32_t a5);
    static void Load(const char* mapName, int32_t zoneID);
    static void LoadTex();
    static void LoadWdt();
    static CTexture* LoadTexture(const char* fileName);
    static int32_t SafeOpen(const char* fileName, SFileImpl** file);
    static int32_t SafeRead(const char* fileName, SFileImpl* file, void* buffer, uint32_t size);
    static CMapArea* AllocArea();
    static CMapArea* GetLoadedArea(int32_t areaX, int32_t areaY);
    static CMapArea* GetAreaAtPosition(const C2Vector* pos);
    static CMapChunk* GetChunkAtPosition(const C2Vector* pos);
    static CMapAreaLow* AllocAreaLow();
    static bool QueryTris(const CAaBox& box, World::TriData* triData, uint32_t flags);
    static bool GetTrisTerrain(const CAaBox& box, World::TriData* triData, uint32_t flags);
    static bool GetTrisChunk(int32_t chunkX, int32_t chunkY, const CiRect& cellRect, const CAaBox& box, World::TriData* triData, uint32_t flags);
    static bool GetTriSubchunk(CMapChunk* chunk, const CiRect* rect, const CAaBox* box);
    static bool QueryTrisMapObjDefs(const CAaBox& box, World::TriData* triData, uint32_t flags);
    static uint32_t QueryIntersectingModels(CM2Model** models, uint32_t maxModels, const CAaBox& box, uint32_t flags);
    static void BuildFootprintMatrix(C44Matrix& matrix, const float* size, int32_t left, float facing);
    static void NewFootprint(int32_t textureId, const float* size, const C3Vector* position, float facing, int32_t left, int32_t terrainType, int32_t activePlayer);
    static void FreeAreaLow(CMapAreaLow* areaLow);
    static void RenderLowDetail(CMapAreaLow* area);
    static void BuildLowDetailGridVertices(CMapAreaLow* area, CGxBuf* buf);
    static void BuildLowDetailGridIndices(uint16_t* holes, int32_t pass, CGxBuf* buf);
    static void FreeArea(CMapArea* area);
    static void FreeAreaMed(CMapAreaMed* areaMed);
    static void FreeChunk(CMapChunk* chunk);
    static void FreeEntity(CMapEntity* entity);
    static void FreeLight(CMapLight* light);
    static void FreeMapObjDefGroup(CMapObjDefGroup* mapObjDefGroup);
    static void FreeDoodadDef(CMapDoodadDef* doodadDef);
    static void FreeMapObjDef(CMapObjDef* mapObjDef);
    static void PurgeArea(CMapArea* area);
    static void PurgeMapObjDef(CMapObjDef* mapObjDef);
    static void PurgeMapObjDefGroup(CMapObjDefGroup* mapObjDefGroup);
    static void DestroyLight(CMapLight* light);
    static void ClearChunkSortList();
    static void ProcessDetailDoodadUpdateList();
    static void UpdateBarriers();
    static void FreeMapObjGroup(CMapObjGroup* mapObjGroup);
    static void FreeRenderChunk(CMapRenderChunk* renderChunk);
    static CMapChunk* AllocMapChunk();
    static CMapRenderChunk* AllocRenderChunk();
    static CMapBaseObjLink* AllocBaseObjLink(CMapBaseObj* baseObj);
    static CMapDoodadDef* AllocDoodadDef();
    static CMapObjDef* AllocMapObjDef();
    static CMapObj* AllocMapObj();
    static CMapEntity* AllocEntity(int32_t linkToHead);
    static CMapObjGroup* AllocMapObjGroup();
    static CMapObjDefGroup* AllocMapObjDefGroup();
    static CMapLight* AllocLight();
    static CMapObjDef* CreateMapObjDef(char* fileName, SMMapObjDef* mapObjectDef, C3Vector* center, bool cached);
    static CMapDoodadDef* CreateDoodadDef(const char* fileName, SMDoodadDef* doodadDef, C3Vector* center);
    static CMapDoodadDef* CreateDoodadDef(const char* fileName, const C3Vector* position, float facing, int32_t waitForLoad);
    static CMapLight* CreateLight(uint8_t a1, uint8_t a2);
    static CMapDoodadDef* CreateDoodadDef(uint32_t doodadRef, SMODoodadDef* doodadDef, char* name, uint32_t key, C44Matrix* mat, uint16_t doodadSet);
    static void FreeBaseObjLinksInBounds(CAaBox* box);
    static void FreeBaseObjLink(CMapBaseObjLink* link);
    static void LinkDoodadDefToMapObjDefs(CMapDoodadDef* doodadDef, CMapObjDef* skip);
    static void Preload();
    static CMapArea* PrepareArea(int32_t areaIndexX, int32_t areaIndexY);
    static void LoadArea(CMapArea* area);
    static void PrepareUpdate(bool addToScene);
    static uint32_t ComputeDoodadLoadProgress(CMapChunk* chunk, float* progress, uint32_t total);
    static uint32_t ComputeMapObjLoadProgress(CMapChunk* chunk, float* progress, uint32_t total);
    static void PurgeMaps();
    static void ClearAllFootprints();
    static void RenderFootprints();
    static void PreUpdateAreas(bool addToScene);
    static void UpdateArea(int32_t addToScene, CMapArea* area, CiRect* chunkRect, int32_t depth);
    static void PrepareMapObjDefs(bool addToScene);
    static void PrepareMapObjDef(CMapObjDef* mapObjDef, CMapObj* mapObj);
    static void CreateMapObjDefGroups(CMapObj* mapObj, CMapObjDef* mapObjDef);
    static bool QueryShadow(C3Vector* pos);
    static DepthCoordTable* GetDepthCoordTable(uint32_t liquidType);
    static CChunkLiquid* AllocChunkLiquid();
    static void CreateWaterRipple(const C3Vector& pos, float a2, float a3, float a4, float a5, float a6, int32_t a7, int32_t a8);
    static void QueryLiquidSounds(C3Vector* pos, float radius, TSGrowableArray<World::LiquidQueryResult>& results);
    static bool QueryLiquidStatus(C3Vector* pos, int32_t* liquidType, float* height, int32_t* a4, int32_t checkGround);
    static bool QueryLiquidTerrain(const C3Vector* pos, int32_t* liquidType, float* height, int32_t checkGround);
    static void RenderWaterRipples();
    static void FreeChunkLiquid(CChunkLiquid* liquid);
    static void FreeAllChunkLiquids();
    static void ProcessChunkLiquidUpdateList();
    static void PrepareMapDoodadDefs();
    static void ProcessRenderChunkUpdateList();
    static CMapEntity* ObjectCreate(CM2Model* model, MAP_OBJECT_FUNC func, void* funcParam, uint64_t param64, uintptr_t param32, uint32_t flags);
    static CMapEntity* ObjectCreateStatic(CM2Model* model, const C44Matrix& matrix, MAP_OBJECT_FUNC func, void* funcParam, uintptr_t param32);
    static CMapEntity* ObjectCreateStaticEx(CM2Model* model, const C44Matrix& matrix, MAP_OBJECT_FUNC func, void* funcParam, uint64_t param64, uintptr_t param32, const char* file, int32_t line);
    static void ObjectUpdate(CMapEntity* entity, C44Matrix& mat, CAaBox& box, CAaSphere& sphere, C3Vector& vec, bool a6, uint32_t a7);
    static void PrepareEntitys(int32_t addToScene);
    static void LinkStaticEntityMultiple2(CMapStaticEntity* entity);
    static void LinkStaticEntity(CMapStaticEntity* entity, C3Vector& top, C3Vector& bottom, C3Vector& mid, int32_t& outInterior, int32_t& outHit, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit);
    static void UpdateEntity(CMapEntity* entity);
    static void LinkStaticEntityMultiple2ToMapObjDefInterior(CMapStaticEntity* entity, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup);
    static void LinkObjectToMapObjDefGroup(CMapStaticEntity* object, CMapObjDefGroup* mapObjDefGroup);
    static void LinkStaticEntitySingle2(CMapStaticEntity* entity);
    static void LinkStaticEntity(CMapStaticEntity* entity);
    static void LinkIntersectMapObjDefGroup(CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup, C3Vector& start, C3Vector& end, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit);
    static bool LinkIntersectMapObjDef(CMapObjDef* mapObjDef, C3Vector& start, C3Vector& end, C3Vector& mid, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit);
    static bool LinkIntersectMapObjDefs(C3Vector& start, C3Vector& end, C3Vector& mid, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit, CMapChunk* chunk);
    static bool GetHeightTerrain(CMapChunk* chunk, C3Vector& pos, int32_t cellX, int32_t cellY, float* outHeight);
    static void QueryDoodadDefHitTests(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, const C3Vector& start, const C3Vector& end);
    static void VectorIntersectMapObjs(CMapChunk* chunk, const C3Vector& start, const C3Vector& end, float* t);
    static bool QueryHeight(const C3Vector& position, float range, float* height);
    static bool LinkStaticEntityGetChunk(C3Vector& pos, float* outHeight, CMapChunk** outChunk);
    static void ClassifyStaticEntityLink(CMapStaticEntity* entity, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup, uint32_t faceInfo, C3Vector* pos);
    static bool LinkObjectToMapExterior(CMapStaticEntity* object);
    static bool QueryGroundTypeTerrain(C3Vector* pos, uint32_t* groundType);
    static int32_t QueryAreaIdTerrain(const C3Vector* pos);
    static void LinkStaticEntityMultiple2ToMapObjDefExterior(CMapStaticEntity* entity);
    static void EnableLight(CMapLight* light);
    static void UpdateLight(CMapLight* light);
    static void UpdateLightBounds(CMapLight* light);
    static void LinkLightToMapObjDefs(CMapLight* light);
    static void LinkLightToChunks(CMapLight* light);
    static bool Intersect(C3Vector* start, C3Vector* end, C3Vector* hitPoint, float* distance, uint32_t flags, World::HitTestDebug* hitInfo);
    static bool VectorIntersect(C3Vector* start, C3Vector* end, uint32_t flags, uint32_t defIgnoreFlags, float* distance, uint16_t* hitIndex, CMapObj** outMapObj, CMapObjDef** outMapObjDef, CMapObjDefGroup** outMapObjDefGroup);
    static bool VectorIntersectTerrain(C3Vector* start, C3Vector* end, float* distance, uint32_t flags, CMapChunk** hitChunk);
    static bool VectorIntersectSubChunkList(C3Vector* start, C3Vector* end, float* distance, uint32_t flags, CMapChunk** hitChunk);
    static void VectorIntersectDoodadDefs(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, uint32_t flags);
    static void VectorIntersectEntitys(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, uint32_t flags);
    static void VectorIntersectDoodadDefs(TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_refLink)>* list, uint32_t flags);
    static void VectorIntersectSY(CiRect& rect);
    static void VectorIntersectSX(CiRect& rect);
    static void VectorIntersectDY(C3Vector& a1, C3Vector& a2, CiRect& rect);
    static void VectorIntersectDX(C3Vector& a1, C3Vector& a2, CiRect& rect);
    static void SetHitTestDebug(World::HitTestDebug* hitInfo, CMapBaseObj* hitObject);
    static bool LocateViewerMapObjs(C3Vector& start, C3Vector& end, float dist, CMapObjDef** outDefs, uint32_t* outGroups);
    static void TestQueryAdd(const CFacet& facet, CImVector color, const C44Matrix* mat);
    static bool QueryLiquidStatusMapObjsExt(C3Vector* pos, int32_t* liquidType, float* height);
    static bool GetFacets(CAaBox* a1, CAaBox* a2, World::FacetData* a3, uint32_t a4, uint32_t* a5);
    static bool GetMapObjFacets(CAaBox* queryBox, CAaBox* box, World::FacetData* facets, uint32_t flags, uint32_t* status);
    static bool QueryFacets(CFrustum* frustum, World::FacetData* facets, uint32_t flags, uint32_t* count);
    static bool GetChunkFacets(int32_t chunkX, int32_t chunkY, CiRect* subRect, CAaBox* queryBox, CAaBox* box, World::FacetData* facets, uint32_t flags);
    static bool GetChunkLiquidFacets(CMapChunk* chunk, CAaBox* box, CiRect* rect, CChunkLiquid* liquid, World::FacetData* facets);
    static bool GetChunkLiquidFacets(CMapChunk* chunk, CWFrustum* frustum, CiRect* rect, CChunkLiquid* liquid, World::FacetData* facets);
    static bool GetChunkFacets(int32_t chunkX, int32_t chunkY, CiRect* subRect, CFrustum* frustum, World::FacetData* facets, uint32_t flags);
    static bool GetMapObjFacets(CFrustum* frustum, World::FacetData* facets, uint32_t flags, uint32_t* status);
    static void QueryDestructibleFacets(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, CAaBox* box, World::FacetData* facets, uint32_t flags);
    static void AppendDestructibleFacets(World::ObjectCollisionData* data, CAaBox* box, World::FacetData* facets, WOWGUID guid);
    static CFacet* BuildImpassableFacets(World::FacetData* facets, C3Vector* up, C3Vector* edge, C3Vector* normal, C3Vector* origin);
    static void CreateImpassableFacets(CMapChunk* chunk, CAaBox* box, World::FacetData* facets);
    static bool CreateFlightBoundsFacets(int32_t areaX, int32_t areaY, CAaBox* box, World::FacetData* facets);
    static void AppendMapObjFacets(CMapDoodadDef* def, CAaBox* box, World::FacetData* facets);
    static bool GetDoodadDefFacets(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* linkList, CAaBox* box, World::FacetData* facets, uint32_t flags);
    static int32_t GetFlightBounds(const C3Vector& pos, float* height, int32_t which);
    static int32_t QueryFlightBounds(const C3Vector& pos, float* height);

    /* 0000 */ uint8_t unused_0000[180];
    /* 00B4 */ uint32_t unused_00B4;
};

extern int32_t s_mapId;
extern WOWGUID s_lastCollisionGUID;

const LiquidTypeRec* GetLiquidTypeRecForArea(uint32_t areaId, uint16_t liquidType);
class WMOAreaTableRec;
WMOAreaTableRec* GetWMOAreaRec(int32_t wmoId, int32_t nameSet, int32_t groupId);

CGxTex* ProjectTex2dGetModFadeTex();

#endif
