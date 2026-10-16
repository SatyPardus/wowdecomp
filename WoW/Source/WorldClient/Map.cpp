#include "Map.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include <cstring>
#include <cmath>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gxu/GxuLight.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C34Matrix.h"
#include "Engine/Source/Tempest/C3Segment.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CFacet.h"
#include "Engine/Source/Tempest/CFrustum.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LiquidMaterialRec.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/WorldClient/Barrier.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/CWorldMath.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/DetailDoodad.h"
#include "WoW/Source/WorldClient/FVBBList.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/MapBaseObj.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapChunkLiquid.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/WorldClient/MapEntity.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/MapLoad.h"
#include "WoW/Source/WorldClient/MapLowDetail.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/MapRenderChunk.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/WaterRipple.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldView.h"
#include "WoW/Source/DB/Rec/GroundEffectTextureRec.h"
#include "WoW/Source/WorldClient/WorldParam.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "WoW/Source/DB/Rec/TerrainTypeRec.h"

// STATIC: 0x00CF4938
TSGrowableArray<CGxVertexPC> CMap::debugVertexArray;

// STATIC: 0x00CF4948
TSGrowableArray<uint16_t> CMap::debugIndexArray;

// STATIC: 0x00CE04A8
CMapLight* CMap::s_mapLight;

// STATIC: 0x00ADF800
float CMap::s_liquidAlphaScale = 0.58333333f;

// STATIC: 0x00ADFBC4
int32_t s_mapId = -1;

// STATIC: 0x00ADFBCC
FVBBList CMap::s_vbbList;

// STATIC: 0x00AEED74
TSExplicitList<CMapRenderChunk, offsetof(CMapRenderChunk, m_renderChunkLink)> CMap::s_mapRenderChunkFreeList;

// STATIC: 0x00AEED80
TSExplicitList<CMapChunkBuf, offsetof(CMapChunkBuf, m_freeLink)> CMap::s_chunkBufFreeList;

// STATIC: 0x00AEED8C
TSList<CMapArea, TSGetExplicitLink<CMapArea> > CMap::s_allocatedAreas = TSExplicitList<CMapArea, offsetof(CMapArea, m_lameAssLink)>();

// STATIC: 0x00AEEDA4
TSList<CMapChunk, TSGetExplicitLink<CMapChunk> > CMap::s_mapChunkList = TSExplicitList<CMapChunk, offsetof(CMapChunk, m_lameAssLink)>();

// STATIC: 0x00AEEDBC
TSList<CMapObjDefGroup, TSGetExplicitLink<CMapObjDefGroup> > CMap::mapObjDefGroupList = TSExplicitList<CMapObjDefGroup, offsetof(CMapObjDefGroup, m_lameAssLink)>();

// STATIC: 0x00AEEDC8
TSList<CMapEntity, TSGetExplicitLink<CMapEntity> > CMap::entityList = TSExplicitList<CMapEntity, offsetof(CMapEntity, m_lameAssLink)>();

// STATIC: 0x00AEEDD4
TSList<CMapLight, TSGetExplicitLink<CMapLight> > CMap::lightList = TSExplicitList<CMapLight, offsetof(CMapLight, m_lameAssLink)>();

// STATIC: 0x00ADFBEC
TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > CMap::s_unk_ADFBEC = TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_refLink)>();

// STATIC: 0x00ADFC28
TSExplicitList<CMapRenderChunk, offsetof(CMapRenderChunk, m_renderChunkLink)> CMap::s_mapRenderChunkUpdateList;

// STATIC: 0x00CE049D
bool CMap::enableSpecularTerrain;

// STATIC: 0x00CE05D0
char CMap::wdtFilename[256];

// STATIC: 0x00CE06D0
char CMap::mapName[256];

// STATIC: 0x00CE07D0
char CMap::mapPath[256];

// STATIC: 0x00CDF7D0
float CMap::s_depthCoordTable0[256];

// STATIC: 0x00CDFBD0
float CMap::s_depthCoordTable1[256];

// STATIC: 0x00CDFFF8
CGxBuf* CMap::lowDetailIndexBuf;

// STATIC: 0x00CDFFFC
CGxPool* CMap::lowDetailIndexPool;

// STATIC: 0x00CE0000
CGxShader* CMap::pixelShader_TerrainSM;

// STATIC: 0x00CE0004
CGxShader* CMap::pixelShader_Terrain0_env;

// STATIC: 0x00CE0008
CGxShader* CMap::vertexShader_Terrain[128];

// STATIC: 0x00CE0208
CGxShader* CMap::pixelShader_Terrain3[96];

// STATIC: 0x00CE0388
CGxShader* CMap::pixelShader_Terrain2[32];

// STATIC: 0x00CE0408
CGxShader* CMap::pixelShader_Terrain1[32];

// STATIC: 0x00CE0488
CGxShader* CMap::pixelShader_Terrain0[3];

// STATIC: 0x00CE04A4
int32_t CMap::uniqueId;

// STATIC: 0x00CE04AC
int32_t CMap::oldSelectLightParm;

// STATIC: 0x00CE04C4
uint32_t CMap::s_queryTag;

// STATIC: 0x00CE04C8
uint32_t CMap::cCount;

// STATIC: 0x00CE08D0
CMapAreaMed* CMap::unk[4096];

// STATIC: 0x00CE48D0
CMapArea* CMap::m_areaTable[4096];

// STATIC: 0x00CE88D0
SMAreaInfo CMap::m_areaInfo[4096];

// STATIC: 0x00CF08F0
int32_t CMap::bActive;

// STATIC: 0x00CF08F4
int32_t CMap::bDungeon;

// STATIC: 0x00CF08F8
int32_t CMap::s_useSSE;

// STATIC: 0x00CF4928
TSGrowableArray<uint32_t> CMap::scCollideList;

// STATIC: 0x00D253A4
int32_t CMap::freeCounts[11];

// STATIC: 0x00D253D0
int32_t CMap::counts[11];

// STATIC: 0x00D253FC
uint32_t* CMap::lightHeap;

// STATIC: 0x00D25400
uint32_t* CMap::cacheLightHeap;

// STATIC: 0x00D25404
uint32_t* CMap::mapObjGroupHeap;

// STATIC: 0x00D25408
uint32_t* CMap::mapObjHeap;

// STATIC: 0x00D2540C
uint32_t* CMap::baseObjLinkHeap;

// STATIC: 0x00D25410
uint32_t* CMap::areaHeap;

// STATIC: 0x00D25414
uint32_t* CMap::areaMedHeap;

// STATIC: 0x00D25418
CMapAreaLowHeap* CMap::areaLowHeap;

// STATIC: 0x00D2541C
uint32_t* CMap::chunkHeap;

// STATIC: 0x00D25420
uint32_t* CMap::doodadDefHeap;

// STATIC: 0x00D25424
uint32_t* CMap::entityHeap;

// STATIC: 0x00CF4958
static uint32_t s_nextPlayerFootprint;

// STATIC: 0x00CF495C
static uint32_t s_nextFootprint;

// STATIC: 0x00A405E8
static const int32_t s_subVertexTriangles[12] = { 17, 9, 0, 9, 1, 0, 9, 17, 18, 9, 18, 1 };

// STATIC: 0x00A40618
static const int32_t s_subVertexIndexFlat[5] = { 0, 9, 17, 1, 18 };

// STATIC: 0x00ADFD04
static World::FacetData s_footprintFacets;

// STATIC: 0x00CF4960
static CMapFootprint s_footprints[144][4];

// STATIC: 0x00CFBE60
static TSGrowableArray<CMapFootprintTexture> s_footprintTextures;

// STATIC: 0x00D25428
uint32_t* CMap::mapObjDefGroupHeap;

// STATIC: 0x00D2542C
uint32_t* CMap::mapObjDefHeap;

// STATIC: 0x00D25430
uint32_t* CMap::chunkLiquidHeap;

// STATIC: 0x00ADFC10
TSList<CMapDoodadDef, TSGetExplicitLink<CMapDoodadDef> > CMap::doodadDefList = TSExplicitList<CMapDoodadDef, offsetof(CMapDoodadDef, m_doodadDefLink)>();

// STATIC: 0x00ADFC64
C3Vector CMap::s_subchunkCoords[5] = { C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, -4.1666665f, 0.0f), C3Vector(-4.1666665f, -4.1666665f, 0.0f), C3Vector(-4.1666665f, 0.0f, 0.0f), C3Vector(-2.0833333f, -2.0833333f, 0.0f) };

// STATIC: 0x00A3FAF0
uint32_t CMap::s_holeMask[16] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000 };

// STATIC: 0x00A3FB30
int32_t CMap::s_fanIndices[8] = { 17, 0, 0, 1, 18, 17, 1, 18 };

// STATIC: 0x00A3FB50
int32_t CMap::s_subchunkIndices[8] = { 3, 0, 0, 1, 2, 3, 1, 2 };

// STATIC: 0x00A3FB88
uint16_t CMap::s_2bitSplatMask[8] = { 0x3, 0xC, 0x30, 0xC0, 0x300, 0xC00, 0x3000, 0xC000 };

// STATIC: 0x00A3FB98
uint32_t CMap::s_2bitSplatShft[8] = { 0, 2, 4, 6, 8, 10, 12, 14 };

// STATIC: 0x00D25434
TSHashTable<CMapObjDef, HASHKEY_NONE> CMap::mapObjDefHash;

// STATIC: 0x00CE0498
int32_t CMap::enableChunkBatching;

// STATIC: 0x00ADFBB4
DepthCoordTable* CMap::s_depthCoordTables[2] = { &CMap::s_depthCoordTable0, &CMap::s_depthCoordTable1 };

// STATIC: 0x00AEEDB0
TSList<CChunkLiquid, TSGetExplicitLink<CChunkLiquid> > CMap::s_chunkLiquidList = TSExplicitList<CChunkLiquid, offsetof(CChunkLiquid, m_link)>();

// STATIC: 0x00ADFC34
TSList<CChunkLiquid, TSGetExplicitLink<CChunkLiquid> > CMap::s_mapChunkLiquidUpdateList = TSExplicitList<CChunkLiquid, offsetof(CChunkLiquid, m_updateLink)>();

// STATIC: 0x00CF0900
CMapLowDetail CMap::s_lowDetail;

// STATIC: 0x00CF08D0
SMMapHeader CMap::header;

// STATIC: 0x00CE04CC
uint32_t CMap::version;

// STATIC: 0x00ADFBC8
int32_t CMap::bPreload = 1;

// STATIC: 0x00CE0494
int32_t CMap::s_isStreamingMode;

// STATIC: 0x00CE049C
bool CMap::s_enableTriangleStrips;

// STATIC: 0x00CE04A0
bool CMap::enableSpecular;

// STATIC: 0x00CE04A1
bool CMap::enablePixelShaders;

// STATIC: 0x00CE04A2
bool CMap::enableVertexShaders;

// STATIC: 0x00CE049E
bool CMap::gTerrainPixelShadersValid;

// STATIC: 0x00CE049F
bool CMap::enableTerrainShaderVertex;

// STATIC: 0x00CE04A3
HASHKEY_NONE CMap::s_mapObjDefHashKey;

// STATIC: 0x00ADFBF8
TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > CMap::s_unk_ADFBF8 = TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_refLink)>();

// STATIC: 0x00ADFC04
TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > CMap::mapObjDefLinkList = TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_refLink)>();

// STATIC: 0x00AF16CC
TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > CMap::s_globalLightList = TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_refLink)>();

// STATIC: 0x00ADFC1C
TSList<CMapChunk, TSGetExplicitLink<CMapChunk> > CMap::s_mapChunkSortList = TSExplicitList<CMapChunk, offsetof(CMapChunk, m_sortListLink)>();

// STATIC: 0x00D2545C
TSHashTable<CMapDoodadDef, HASHKEY_DWORD> CMap::doodadDefHash;

// STATIC: 0x00AEEDE0
TSExplicitList<CDestructibleProxy, offsetof(CDestructibleProxy, m_link)> CMap::s_destructibleProxyList;

// STATIC: 0x00CE04BC
int32_t CMap::mapGetFacetsCount;

// STATIC: 0x00CE04C0
int32_t CMap::bspRecurseCount;

// STATIC: 0x00ADFC40
TSList<CDetailDoodadInst, TSGetExplicitLink<CDetailDoodadInst> > CMap::s_detailDoodadUpdateList = TSExplicitList<CDetailDoodadInst, offsetof(CDetailDoodadInst, m_link)>();

// STATIC: 0x00ADFD90
static const int32_t s_flightBoundsTriangles[24] = { 3, 0, 4, 0, 1, 4, 1, 2, 4, 2, 5, 4, 5, 8, 4, 8, 7, 4, 7, 6, 4, 6, 3, 4 };

// STATIC: 0x00ADFDF0
static const float s_flightBoundsOffsets[9][2] = {
    { 0.0f, 0.0f },
    { 0.0f, -266.66666f },
    { 0.0f, -533.33331f },
    { -266.66666f, 0.0f },
    { -266.66666f, -266.66666f },
    { -266.66666f, -533.33331f },
    { -533.33331f, 0.0f },
    { -533.33331f, -266.66666f },
    { -533.33331f, -533.33331f },
};

// STATIC: 0x00CD7768
WOWGUID s_lastCollisionGUID;

// VERIFY: 738eb1a4 manual
// OFFSET: 0x0079B440
CMapArea* CMap::GetLoadedArea(int32_t areaX, int32_t areaY) {
    CMapArea* area = CMap::m_areaTable[64 * areaY + areaX];

    if (area && area->m_asyncObject) {
        return nullptr;
    }

    return area;
}

// VERIFY: ef54a4a5 auto
// OFFSET: 0x0079B870
DepthCoordTable* CMap::GetDepthCoordTable(uint32_t liquidType) {
    if (!liquidType) {
        return nullptr;
    }

    LiquidTypeRec* typeRec = g_liquidTypeDB.GetRecord(liquidType);

    if (!typeRec) {
        return nullptr;
    }

    LiquidMaterialRec* materialRec = g_liquidMaterialDB.GetRecord(typeRec->m_materialID);

    if (!materialRec || (materialRec->m_lvf != 0 && materialRec->m_lvf != 2)) {
        return nullptr;
    }

    return CMap::s_depthCoordTables[typeRec->m_int[0]];
}

// VERIFY: e651a57d manual
// OFFSET: 0x0079E5C0
CGxShader* CMap::GetTerrainPixelShader(int32_t bigAlpha, int32_t layers, int32_t shadowMode, int32_t a4, int32_t a5) {
    int32_t layerIndex = layers - 1;

    switch (shadowMode) {
        case 0:
            return CMap::pixelShader_Terrain1[16 * a5 + 8 * bigAlpha + 2 * layerIndex + a4];

        case 1:
            if (g_theGxDevicePtr->Caps().m_shaderTargets[4] == 3 || g_theGxDevicePtr->Caps().m_shaderTargets[4] == 12) {
                return CMap::pixelShader_Terrain2[16 * a5 + 8 * bigAlpha + 2 * layerIndex + a4];
            }

            return CMap::pixelShader_Terrain3[48 * a5 + 24 * bigAlpha + 2 * layerIndex + a4];

        case 2:
        case 3:
            return CMap::pixelShader_Terrain3[48 * a5 - 8 + 24 * bigAlpha + 8 * shadowMode + 2 * layerIndex + a4];

        default:
            return nullptr;
    }
}

// VERIFY: 2fffff6e manual
// OFFSET: 0x0079E730
void CMap::ClearChunkDetailDoodads() {
    for (CMapChunk* chunk = CMap::s_mapChunkList.Head(); chunk; chunk = CMap::s_mapChunkList.Next(chunk)) {
        if (chunk->m_detailDoodadInst) {
            CDetailDoodad::FreeInst(chunk->m_detailDoodadInst);
            chunk->m_detailDoodadInst = nullptr;
        }
    }
}

// VERIFY: 77579752 manual
// OFFSET: 0x0079E780
void CMap::ClearAllRenderChunks() {
    for (CMapChunk* chunk = CMap::s_mapChunkList.Head(); chunk; chunk = CMap::s_mapChunkList.Next(chunk)) {
        if (chunk->m_renderChunk) {
            chunk->m_renderChunk->FreeBuf();
        }
    }
}

// VERIFY: 9b396a3d manual
// OFFSET: 0x0079EFF0
void CDestructibleProxy::FindOrCreate(CDestructibleProxy* proxy) {
    if (!proxy->m_uniqueId) {
        CMap::s_destructibleProxyList.LinkToHead(proxy);
        return;
    }

    for (CDestructibleProxy* node = CMap::s_destructibleProxyList.Head(); !CMap::s_destructibleProxyList.IsEnd(node); node = CMap::s_destructibleProxyList.RawNext(node)) {
        if (node->m_uniqueId == proxy->m_uniqueId) {
            return;
        }
    }

    CDestructibleProxy* created = CMap::s_destructibleProxyList.NewNode(2, 0, 0);
    created->unused_0000 = proxy->unused_0000;
    created->m_sphere = proxy->m_sphere;
    created->m_uniqueId = proxy->m_uniqueId;
    CMap::s_destructibleProxyList.LinkToHead(created);
}

// VERIFY: ee012d34 manual
// OFFSET: 0x0079F210
void CDestructibleProxy::FreeMatching(CDestructibleProxy* proxy) {
    CDestructibleProxy* next;

    for (CDestructibleProxy* node = CMap::s_destructibleProxyList.Head(); node; node = next) {
        next = CMap::s_destructibleProxyList.Next(node);

        if (proxy == node || (proxy->m_uniqueId && node->m_uniqueId == proxy->m_uniqueId)) {
            CMap::s_destructibleProxyList.UnlinkNode(node);

            if (proxy != node) {
                CMap::s_destructibleProxyList.DeleteNode(node);
            }
        }
    }
}

// VERIFY: e85ab889 manual
// OFFSET: 0x0079F320
void CMap::Destroy() {
    if (CMap::bActive) {
        CMap::Unload();
    }

    WaterRipple::Destroy();
    CMapObj::Destroy();
    CMapChunk::Destroy();
    CMapArea::Destroy();
    CDetailDoodad::Destroy();
    CMap::FootprintDestroy();

    for (uint32_t i = 0; i < sizeof(CMap::vertexShader_Terrain) / sizeof(CMap::vertexShader_Terrain[0]); i++) {
        if (CMap::vertexShader_Terrain[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::vertexShader_Terrain[i]);
        }
    }

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain0) / sizeof(CMap::pixelShader_Terrain0[0]); i++) {
        if (CMap::pixelShader_Terrain0[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain0[i]);
        }
    }

    if (CMap::pixelShader_Terrain0_env) {
        g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain0_env);
    }

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain1) / sizeof(CMap::pixelShader_Terrain1[0]); i++) {
        if (CMap::pixelShader_Terrain1[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain1[i]);
        }
    }

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain2) / sizeof(CMap::pixelShader_Terrain2[0]); i++) {
        if (CMap::pixelShader_Terrain2[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain2[i]);
        }
    }

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain3) / sizeof(CMap::pixelShader_Terrain3[0]); i++) {
        if (CMap::pixelShader_Terrain3[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain3[i]);
        }
    }

    if (CMap::pixelShader_TerrainSM) {
        g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_TerrainSM);
    }

    CMap::pixelShader_TerrainSM = nullptr;

    CGxBuf* indexBuf = CMap::lowDetailIndexBuf;

    if (indexBuf && indexBuf != g_theGxDevicePtr->BufStream(indexBuf->m_pool->m_target, 0, 0)) {
        g_theGxDevicePtr->BufDestroy(indexBuf);
    }

    if (CMap::lowDetailIndexPool) {
        g_theGxDevicePtr->PoolDestroy(CMap::lowDetailIndexPool);
    }

    CMap::s_vbbList.Destroy();

    if (CMap::lightHeap) {
        SMemFree(CMap::lightHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::cacheLightHeap) {
        SMemFree(CMap::cacheLightHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::mapObjGroupHeap) {
        SMemFree(CMap::mapObjGroupHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::mapObjHeap) {
        SMemFree(CMap::mapObjHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::baseObjLinkHeap) {
        SMemFree(CMap::baseObjLinkHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::areaHeap) {
        SMemFree(CMap::areaHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::areaMedHeap) {
        SMemFree(CMap::areaMedHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::areaLowHeap) {
        SMemFree(CMap::areaLowHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::chunkHeap) {
        SMemFree(CMap::chunkHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::doodadDefHeap) {
        SMemFree(CMap::doodadDefHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::entityHeap) {
        SMemFree(CMap::entityHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::mapObjDefGroupHeap) {
        SMemFree(CMap::mapObjDefGroupHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::mapObjDefHeap) {
        SMemFree(CMap::mapObjDefHeap, "delete", SLOG_FUNCTION, 0);
    }

    if (CMap::chunkLiquidHeap) {
        SMemFree(CMap::chunkLiquidHeap, "delete", SLOG_FUNCTION, 0);
    }

    CMap::s_mapRenderChunkFreeList.Clear();
    CMap::s_chunkBufFreeList.Clear();
}

// VERIFY: f42871fd manual
// OFFSET: 0x0079E3C0
void CMap::InitializeLiquidTables() {
    float scale = 1.0f / (255.0f * CMap::s_liquidAlphaScale);

    for (uint32_t i = 0; i < sizeof(CMap::s_depthCoordTable1) / sizeof(CMap::s_depthCoordTable1[0]); i++) {
        float alpha = static_cast<float>(i) * CMap::s_liquidAlphaScale * scale;
        CMap::s_depthCoordTable1[i] = alpha >= 1.0f ? 1.0f : alpha;

        float depth = static_cast<float>(i) * 0.11111111f;
        CMap::s_depthCoordTable0[i] = depth <= 4.6666665f ? depth * 0.21428572f : 1.0f;
    }

    WaterRipple::Initialize();
    Liquid::CreateProceduralTextures();
}

// VERIFY: cd6270d0 manual
// OFFSET: 0x0079E4F0
void CMap::InitializePCFShaders() {
    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain2) / sizeof(CMap::pixelShader_Terrain2[0]); i++) {
        if (CMap::pixelShader_Terrain2[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain2[i]);
        }
    }

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain3) / sizeof(CMap::pixelShader_Terrain3[0]); i++) {
        if (CMap::pixelShader_Terrain3[i]) {
            g_theGxDevicePtr->ShaderDestroy(CMap::pixelShader_Terrain3[i]);
        }
    }

    g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain2, GxSh_Pixel, "Shaders\\Pixel", CShaderEffect::s_usePcfFiltering ? "Terrain2_pcf" : "Terrain2", sizeof(CMap::pixelShader_Terrain2) / sizeof(CMap::pixelShader_Terrain2[0]));
    g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain3, GxSh_Pixel, "Shaders\\Pixel", CShaderEffect::s_usePcfFiltering ? "Terrain3_pcf" : "Terrain3", sizeof(CMap::pixelShader_Terrain3) / sizeof(CMap::pixelShader_Terrain3[0]));
}

// VERIFY: 98a7e674 manual
// OFFSET: 0x0079E7C0
void CMap::Initialize() {
    CMapArea::Initialize();
    CMapChunk::Initialize();
    CMapObj::Initialize();
    CMapObjGroup::Initialize();
    CDetailDoodad::Initialize();
    CMap::FootprintInitialize();

    memset(CMap::counts, 0, sizeof(CMap::counts));
    memset(CMap::freeCounts, 0, sizeof(CMap::freeCounts));
    memset(CMap::unk, 0, sizeof(CMap::unk));
    memset(CMap::m_areaTable, 0, sizeof(CMap::m_areaTable));

    for (uint32_t i = 0; i < sizeof(CMap::m_areaInfo) / sizeof(CMap::m_areaInfo[0]); i++) {
        CMap::m_areaInfo[i].m_flags = 0;
    }

    CMap::scCollideList.SetCount(2048);

    CMap::cCount = 0;
    CMap::s_queryTag = 0;
    CMap::uniqueId = -2;
    s_mapId = -1;
    CMap::bDungeon = 0;
    CMap::bActive = 0;
    CMap::oldSelectLightParm = 0;

    CMap::InitializeLiquidTables();

    memset(CMap::vertexShader_Terrain, 0, sizeof(CMap::vertexShader_Terrain));

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain0) / sizeof(CMap::pixelShader_Terrain0[0]); i++) {
        CMap::pixelShader_Terrain0[i] = nullptr;
    }

    CMap::pixelShader_Terrain0_env = nullptr;

    for (uint32_t i = 0; i < sizeof(CMap::pixelShader_Terrain1) / sizeof(CMap::pixelShader_Terrain1[0]); i++) {
        CMap::pixelShader_Terrain1[i] = nullptr;
    }

    CMap::InitializePCFShaders();
    CMap::pixelShader_TerrainSM = nullptr;

    g_theGxDevicePtr->ShaderCreate(CMap::vertexShader_Terrain, GxSh_Vertex, "Shaders\\Vertex", "Terrain", 128);
    g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain0, GxSh_Pixel, "Shaders\\Pixel", "Terrain0", 3);
    g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain0_env, GxSh_Pixel, "Shaders\\Pixel", "Terrain0_env", 1);

    switch (g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel]) {
        case 1: {
            g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain1, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 6);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[8], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w", 6);
            break;
        }

        case 2: {
            g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain1, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 8);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[8], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_1", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[9], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_1", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[10], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_2", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[11], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_2", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[12], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_3", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[13], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_3", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[14], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_4", 1);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[15], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w_4", 1);
            break;
        }

        case 8:
        case 9:
        case 10: {
            g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain1, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 4);
            g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_Terrain1[8], GxSh_Pixel, "Shaders\\Pixel", "Terrain1w", 4);
            break;
        }

        default: {
            g_theGxDevicePtr->ShaderCreate(CMap::pixelShader_Terrain1, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 32);
            break;
        }
    }

    g_theGxDevicePtr->ShaderCreate(&CMap::pixelShader_TerrainSM, GxSh_Pixel, "Shaders\\Pixel", "TerrainSM", 1);

    CMap::lowDetailIndexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Dynamic, 0x1800, static_cast<EGxPoolHintBits>(0), "CMap::lowDetailIndexPool");
    CMap::lowDetailIndexBuf = g_theGxDevicePtr->BufCreate(CMap::lowDetailIndexPool, sizeof(uint16_t), 0xC00, 0);
    CMap::s_vbbList.Initialize(GxPoolTarget_Vertex, GxPoolUsage_Dynamic, 16, 0x221, 24);

    CMap::lightHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapLight), 128, "WLIGHT", true));
    CMap::cacheLightHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapCacheLight), 256, "WCACHELIGHT", true));
    CMap::mapObjGroupHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapObjGroup), 128, "WMAPOBJGROUP", true));
    CMap::mapObjHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapObj), 32, "WMAPOBJ", true));
    CMap::baseObjLinkHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapBaseObjLink), 10000, "WBASEOBJLINK", true));
    CMap::areaHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapArea), 16, "WAREA", true));
    CMap::areaMedHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapAreaMed), 16, "WAREAMED", true));
    CMap::areaLowHeap = new (__FILE__, __LINE__) CMapAreaLowHeap{ ObjectAllocAddHeap(sizeof(CMapAreaLow), 16, "WAREALOW", true) };
    CMap::chunkHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapChunk), 256, "WCHUNK", true));
    CMap::doodadDefHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapDoodadDef), 5000, "WDOODADDEF", true));
    CMap::entityHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapEntity), 128, "WENTITY", true));
    CMap::mapObjDefGroupHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapObjDefGroup), 128, "WMAPOBJDEFGROUP", true));
    CMap::mapObjDefHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapObjDef), 64, "WMAPOBJDEF", true));
    CMap::chunkLiquidHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CChunkLiquid), 64, "WCHUNKLIQUID", true));

    int32_t vendor;

    if (OsGetProcessorFeaturesEx(vendor) & 0x4) {
        CMap::s_useSSE = 1;
    }
}

// INLINED
CMapFootprintTexture::CMapFootprintTexture() {
    this->m_texture = nullptr;
    this->m_vertexCount = 0;
}

// VERIFY: fa585208 manual
// OFFSET: 0x0079F860
void CMap::FootprintDestroy() {
    for (uint32_t i = 0; i < 144; i++) {
        for (uint32_t j = 0; j < 4; j++) {
            s_footprints[i][j].m_link.Unlink();
        }
    }

    for (uint32_t i = 0; i < s_footprintTextures.Count(); i++) {
        if (s_footprintTextures[i].m_texture) {
            HandleClose(reinterpret_cast<HOBJECT>(s_footprintTextures[i].m_texture));
        }
    }
}

// VERIFY: 3f19b093 manual
// OFFSET: 0x0079FA10
void CMap::ClearAllFootprints() {
    for (uint32_t i = 0; i < 144; i++) {
        for (uint32_t j = 0; j < 4; j++) {
            s_footprints[i][j].m_vertices.SetCount(0);
            s_footprints[i][j].m_link.Unlink();
        }
    }
}

// VERIFY: bc21f929 manual
// OFFSET: 0x007A03C0
void CMap::FootprintInitialize() {
    CStatus status;

    s_nextFootprint = 64;
    s_nextPlayerFootprint = 0;
    s_footprintTextures.SetCount(g_footprintTexturesDB.m_maxID + 1);

    int32_t index = g_footprintTexturesDB.m_numRecords;

    while (index) {
        index--;

        FootprintTexturesRec* record = (index < 0 || index >= g_footprintTexturesDB.m_numRecords) ? nullptr : &g_footprintTexturesDB.m_records[index];
        CMapFootprintTexture* footprint = &s_footprintTextures[record->m_id];

        if (!footprint->m_texture) {
            footprint->m_texture = TextureCreate(record->m_footstepFilename, CGxTexFlags(), &status, 1);
        }
    }
}


// VERIFY: 7ce5c762 manual
// OFFSET: 0x0079E4B0
CGxShader* CMap::GetPixelShader(bool a1, bool a2, bool a3) {
    if (!a1)
        return CMap::pixelShader_Terrain0[0];
    if (!a2)
        return CMap::pixelShader_Terrain0[2];
    if (a3)
        return CMap::pixelShader_Terrain0_env;
    return CMap::pixelShader_Terrain0[1];
}

// VERIFY: 70d4ac15 manual
// OFFSET: 0x007A06A0
bool CMap::QueryShadow(C3Vector* pos) {
    float y = -(pos->y - 17066.666f);
    float x = -(pos->x - 17066.666f);

    if (y < 0.0f || x < 0.0f || y >= 34133.332f || x >= 34133.332f) {
        return false;
    }

    int32_t cellX = CMath::fint_(y * 0.029999999f - 0.5f);
    int32_t cellY = CMath::fint_(x * 0.029999999f - 0.5f);
    CMapArea* area = CMap::GetLoadedArea((cellX >> 4) & 0x3F, (cellY >> 4) & 0x3F);

    if (!area) {
        return false;
    }

    CMapChunk* chunk = area->m_mapChunks[16 * (cellY & 0xF) + (cellX & 0xF)];

    if (!chunk || !chunk->m_shadowMap) {
        return false;
    }

    int32_t shadowX = CMath::fint_(y * 1.92f - 0.5f) & 0x3F;
    int32_t shadowY = CMath::fint_(x * 1.92f - 0.5f) & 0x3F;
    uint8_t mamp = area->m_header->m_mampValue;
    int32_t bitX = shadowX >> mamp;

    return (chunk->m_shadowMap[(bitX >> 3) + (shadowY >> mamp) * (8 >> mamp)] & (1 << (bitX & 7))) != 0;
}

// VERIFY: cdb37c44 manual
// OFFSET: 0x007A0820
bool CMap::QueryLiquidTerrain(const C3Vector* pos, int32_t* liquidType, float* height, int32_t checkGround) {
    float cellYf = -(pos->y - 17066.666f) * 0.24f;
    float cellXf = 0.24f * -(pos->x - 17066.666f);
    int32_t cellX = CMath::fint_(cellYf - 0.5f);
    int32_t cellY = CMath::fint_(cellXf - 0.5f);

    CMapArea* area = CMap::m_areaTable[64 * ((cellY >> 7) & 0x3F) + ((cellX >> 7) & 0x3F)];

    if (!area || area->m_asyncObject) {
        return false;
    }

    CMapChunk* chunk = area->m_mapChunks[((cellX >> 3) & 0xF) + 16 * ((cellY >> 3) & 0xF)];

    if (!chunk) {
        return false;
    }

    C2iVector tile;
    tile.x = cellX & 7;
    tile.y = cellY & 7;

    C2Vector frac(cellYf - cellX, cellXf - cellY);

    for (CChunkLiquid* liquid = chunk->m_liquidChunkLinkList.Head(); liquid; liquid = chunk->m_liquidChunkLinkList.Next(liquid)) {
        if (liquid->TileExists(tile.x, tile.y) && liquid->GetHeight(frac, tile, *height) && *height + 0.0099999998f > pos->z) {
            if (!checkGround) {
                *liquidType = liquid->m_type;
                return true;
            }

            float ground = -10000.0f;
            CMap::GetHeightTerrain(chunk, *const_cast<C3Vector*>(pos), cellX, cellY, &ground);

            if (pos->z + 0.0099999998f > ground) {
                *liquidType = liquid->m_type;
                return true;
            }
        }
    }

    return false;
}

// VERIFY: c7588881 manual
// OFFSET: 0x007A09D0
bool CMap::QueryLiquidStatusMapObjsExt(C3Vector* pos, int32_t* liquidType, float* height) {
    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.m_fulllist.Next(mapObjDef)) {
        if (mapObjDef->m_flags & 0x100) {
            continue;
        }

        const CAaBox& box = mapObjDef->m_bbox;

        if (box.b.x > pos->x || box.b.y > pos->y || box.b.z > pos->z || box.t.x < pos->x || box.t.y < pos->y || box.t.z < pos->z) {
            continue;
        }

        C3Vector local = *pos * mapObjDef->m_invMat;
        CMapObj* mapObj = mapObjDef->m_owner;

        if (mapObj->ContainsPointInBounds(local) && mapObj->QueryLiquidStatus(0x2000, local, liquidType, height)) {
            C3Vector level(local.x, local.y, *height);
            level *= mapObjDef->m_mat;
            *height = level.z;
            return true;
        }
    }

    return false;
}

// VERIFY: 7c5ab3e1 auto
// OFFSET: 0x007A0B00
bool CMap::QueryLiquidStatus(C3Vector* pos, int32_t* liquidType, float* height, int32_t* a4, int32_t checkGround) {
    *a4 = 0;

    if (CMap::QueryLiquidStatusMapObjsExt(pos, liquidType, height)) {
        return true;
    }

    return CMap::QueryLiquidTerrain(pos, liquidType, height, checkGround);
}

// VERIFY: 2f11b480 manual
// OFFSET: 0x007A2070
static int32_t CompareIntersectCandidates(const void* a, const void* b) {
    const MapObjIntersectCandidate* ca = static_cast<const MapObjIntersectCandidate*>(a);
    const MapObjIntersectCandidate* cb = static_cast<const MapObjIntersectCandidate*>(b);

    if (cb->t > ca->t)
        return -1;

    if (cb->t < ca->t)
        return 1;

    return 0;
}

// VERIFY: 24e6cf3e auto
// OFFSET: 0x007A20B0
static const char* PathFileName(const char* path) {
    const char* name = path;

    for (const char* separator = SStrStrI(path, "\\"); separator; separator = SStrStrI(separator + 1, "\\")) {
        name = separator + 1;
    }

    return name;
}

// VERIFY: 85e5829e manual
// OFFSET: 0x007A20E0
void CMap::VectorIntersectSX(CiRect& rect) {
    int32_t x = rect.minX;

    if (x <= rect.maxX) {
        do {
            CMap::scCollideList.m_data[CMap::cCount++] = x;
            CMap::scCollideList.m_data[CMap::cCount++] = rect.minY;
            x++;
        } while (x <= rect.maxX);
    } else {
        do {
            CMap::scCollideList.m_data[CMap::cCount++] = x;
            CMap::scCollideList.m_data[CMap::cCount++] = rect.minY;
            x--;
        } while (x >= rect.maxX);
    }
}

// VERIFY: 8a929a73 manual
// OFFSET: 0x007A2180
void CMap::VectorIntersectSY(CiRect& rect) {
    int32_t y = rect.minY;

    if (y <= rect.maxY) {
        do {
            CMap::scCollideList.m_data[CMap::cCount++] = rect.minX;
            CMap::scCollideList.m_data[CMap::cCount++] = y;
            y++;
        } while (y <= rect.maxY);
    } else {
        do {
            CMap::scCollideList.m_data[CMap::cCount++] = rect.minX;
            CMap::scCollideList.m_data[CMap::cCount++] = y;
            y--;
        } while (y >= rect.maxY);
    }
}

// VERIFY: 6d1fef26 manual
// OFFSET: 0x007A2230
void CMap::VectorIntersectDX(C3Vector& start, C3Vector& end, CiRect& cells) {
    int32_t y = cells.minY;
    float slope = (end.y - start.y) / (end.x - start.x);
    int32_t x = cells.minX;
    float intercept = start.y - start.x * slope;

    int32_t next;
    int32_t step;

    if (cells.maxX <= x) {
        next = cells.minX;
        step = -1;
    } else {
        next = x + 1;
        step = 1;
    }

    float edge = next * 4.1666665f;

    CMap::scCollideList.m_data[CMap::cCount++] = x;
    CMap::scCollideList.m_data[CMap::cCount++] = y;

    while (x != cells.maxX + step) {
        if (CMap::cCount >= 0x7FC) {
            return;
        }

        int32_t edgeY = CMath::fint_((slope * edge + intercept) * 0.24f - 0.5f);

        if (edgeY != y) {
            CMap::scCollideList.m_data[CMap::cCount++] = x;
            CMap::scCollideList.m_data[CMap::cCount++] = edgeY;
        }

        x += step;
        edge = step * 4.1666665f + edge;

        CMap::scCollideList.m_data[CMap::cCount++] = x;
        CMap::scCollideList.m_data[CMap::cCount++] = edgeY;

        y = edgeY;
    }

    if (y != cells.maxY) {
        CMap::scCollideList.m_data[CMap::cCount++] = cells.maxX;
        CMap::scCollideList.m_data[CMap::cCount++] = cells.maxY;
    }
}

// VERIFY: 6264b08b manual
// OFFSET: 0x007A23E0
void CMap::VectorIntersectDY(C3Vector& start, C3Vector& end, CiRect& cells) {
    float slope = (end.y - start.y) / (end.x - start.x);
    int32_t x = cells.minX;
    int32_t y = cells.minY;
    float intercept = start.y - start.x * slope;
    float invSlope = 1.0f / slope;

    int32_t next;
    int32_t step;

    if (cells.maxY <= cells.minY) {
        next = cells.minY;
        step = -1;
    } else {
        next = y + 1;
        step = 1;
    }

    float edge = next * 4.1666665f;

    CMap::scCollideList.m_data[CMap::cCount++] = cells.minX;
    CMap::scCollideList.m_data[CMap::cCount++] = cells.minY;

    while (y != cells.maxY + step) {
        if (CMap::cCount >= 0x7FC) {
            return;
        }

        int32_t edgeX = CMath::fint_((edge - intercept) * invSlope * 0.24f - 0.5f);

        if (edgeX != x) {
            CMap::scCollideList.m_data[CMap::cCount++] = edgeX;
            CMap::scCollideList.m_data[CMap::cCount++] = y;
        }

        CMap::scCollideList.m_data[CMap::cCount++] = edgeX;
        edge = step * 4.1666665f + edge;
        y += step;
        CMap::scCollideList.m_data[CMap::cCount++] = y;

        x = edgeX;
    }

    if (x != cells.maxX) {
        CMap::scCollideList.m_data[CMap::cCount++] = cells.maxX;
        CMap::scCollideList.m_data[CMap::cCount++] = cells.maxY;
    }
}


// VERIFY: 136b6340 manual
// OFFSET: 0x007A25A0
World::ObjectCollisionData::ObjectCollisionData() {
}

// VERIFY: 09bec685 manual
// OFFSET: 0x007A25F0
static void AddHitDebugModel(World::HitTestDebug* hitInfo, int32_t depth, CM2Model* model) {
    for (; model; model = model->m_attachNext) {
        if (hitInfo->m_numModels == 16) {
            break;
        }

        World::HitTestDebug::Model* entry = &hitInfo->m_models[hitInfo->m_numModels++];
        entry->m_name = PathFileName(model->m_shared->m_fileName);
        entry->m_depth = depth;
        entry->m_bbox = model->GetCurrentBoundingBox();
        entry->m_sphere = model->GetCurrentBoundingSphere();
        entry->m_numGeometryBatches = model->GetNumGeometryBatches();
        entry->m_numPrimitives = model->GetNumPrimitives();
        entry->m_numOtherBatches = model->GetNumBatches() - entry->m_numGeometryBatches;
        entry->m_numBones = (model->m_fFlags & 1) ? model->m_shared->m_data->m_bones.m_count : 0;

        if (!(model->m_fFlags & 1)) {
            model->WaitForLoad(nullptr);
        }

        entry->m_numLights = model->m_shared->m_data->m_lights.m_count;
        entry->m_numCurrentLights = model->GetCurrentLightCount();
        entry->m_attachmentId = model->m_attachmentId;
        entry->m_attachmentScale = 1.0f;

        if (model->m_attachmentId != 0xFFFFFFFF && model->m_attachParent) {
            entry->m_attachmentScale = model->m_attachParent->GetAttachmentWorldScale(model->m_attachmentId);
        }

        entry->m_model = model;
        entry->m_numCombinableBatches = model->GetNumCombinableDoodadBatches();
        entry->m_instanceCountMax = model->GetInstanceCountMax();
        entry->m_groupName = nullptr;
        entry->m_groupNum = 0;
        entry->m_groupFlags = 0;
        AddHitDebugModel(hitInfo, depth + 1, model->m_attachList);
    }
}

// VERIFY: 0869a654 manual
// OFFSET: 0x007A2760
void CMap::VectorIntersectDoodadDefs(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, uint32_t flags) {
    for (CMapBaseObjLink* link = list->Head(); link; link = list->Next(link)) {
        CMapDoodadDef* doodadDef = static_cast<CMapDoodadDef*>(link->m_owner);

        if ((flags & 0x1000000) && doodadDef->m_culled) {
            continue;
        }

        if ((doodadDef->m_flags & 0x100) || !(doodadDef->m_flags & 0x80) || doodadDef->m_unkCounter == CMap::s_queryTag) {
            continue;
        }

        CM2Model* model = doodadDef->m_model;

        if (!model) {
            continue;
        }

        int32_t mode = -1;

        if (doodadDef->m_collisionGuid) {
            if (flags & 0x100000) {
                mode = 3;
            } else if (flags & 0x600000) {
                mode = 0;
            }
        } else if (flags & 0x1) {
            mode = 3;
        } else if (flags & 0xE) {
            mode = (flags & 0x8) ? 2 : (flags >> 24) & 1;
        }

        if (mode >= 0 && (model->m_fFlags & 1)) {
            if (!model->m_hitTestPrev) {
                model->m_hitTestPrev = &model->m_scene->m_hitTestList;
                model->m_hitTestNext = model->m_scene->m_hitTestList;
                model->m_scene->m_hitTestList = model;

                if (model->m_hitTestNext) {
                    model->m_hitTestNext->m_hitTestPrev = &model->m_hitTestNext;
                }
            }

            model->m_hitTestMode = mode;
            model->m_hitTestOwner = doodadDef;
            model->m_hitTestGroup = 0;
        }

        doodadDef->m_unkCounter = CMap::s_queryTag;
    }
}

// VERIFY: 690f8e3c manual
// OFFSET: 0x007A2960
void CMap::VectorIntersectEntitys(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, uint32_t flags) {
    if (!World::s_objectCollisionCallback) {
        return;
    }

    for (CMapBaseObjLink* link = list->Head(); link; link = list->Next(link)) {
        CMapEntity* entity = static_cast<CMapEntity*>(link->m_owner);

        if ((flags & 0x1000000) && entity->m_culled) {
            continue;
        }

        if (!(entity->m_staticFlags & 0x2)) {
            return;
        }

        if (entity->m_unkCounter == CMap::s_queryTag || (entity->m_staticFlags & 0x4)) {
            continue;
        }

        World::ObjectCollisionData data;
        WOWGUID guid = { entity->m_funcParam64 };

        if (!World::s_objectCollisionCallback(&guid, entity->m_funcParam32, &data, flags)) {
            continue;
        }

        CM2Model* model = data.m_model;

        if (!model) {
            continue;
        }

        if (model->m_fFlags & 1) {
            if (!model->m_hitTestPrev) {
                model->m_hitTestPrev = &model->m_scene->m_hitTestList;
                model->m_hitTestNext = model->m_scene->m_hitTestList;
                model->m_scene->m_hitTestList = model;

                if (model->m_hitTestNext) {
                    model->m_hitTestNext->m_hitTestPrev = &model->m_hitTestNext;
                }
            }

            model->m_hitTestMode = (flags & 0x100000) ? 3 : 0;
            model->m_hitTestGroup = 0;
            model->m_hitTestOwner = entity;
        }

        entity->m_unkCounter = CMap::s_queryTag;
    }
}

// VERIFY: 3fe9ef3a manual
// OFFSET: 0x007A3570
bool CMap::VectorIntersectSubChunkList(C3Vector* start, C3Vector* end, float* distance, uint32_t flags, CMapChunk** hitChunk) {
    uint32_t* entry = CMap::scCollideList.m_data;
    uint32_t lastY = entry[0] & 0x2000;
    uint32_t lastX = entry[1] & 0x2000;
    uint32_t remaining = CMap::cCount;
    CMapChunk* chunk = nullptr;
    CMapChunk* bestChunk = nullptr;
    uint32_t modelFlags = flags & 0x40F0000F;

    if (modelFlags) {
        World::s_m2Scene->BeginHitTest();
    }

    float dx = end->x - start->x;
    float dy = end->y - start->y;
    float dz = end->z - start->z;
    C3Vector origin = *start;
    float invLength = 1.0f / sqrt(dz * dz + dy * dy + dx * dx);
    C3Vector dir(dx * invLength, dy * invLength, invLength * dz);
    float bestDist = *distance;

    while (remaining) {
        uint32_t cellX = entry[0];
        uint32_t cellY = entry[1];
        remaining -= 2;
        entry += 2;

        if (cellX > 0x2000 || cellY > 0x2000) {
            break;
        }

        if ((cellX & 0x1FF8) != lastX || (cellY & 0x1FF8) != lastY) {
            CMapArea* area = CMap::m_areaTable[64 * ((cellY >> 7) & 0x3F) + ((cellX >> 7) & 0x3F)];

            if (!area || area->m_asyncObject) {
                break;
            }

            chunk = area->m_mapChunks[16 * ((cellY >> 3) & 0xF) + ((cellX >> 3) & 0xF)];

            if (!chunk) {
                break;
            }

            lastX = cellX & 0x1FF8;
            lastY = cellY & 0x1FF8;
            origin.x = start->x - chunk->m_topLeftCoords.x;
            origin.y = start->y - chunk->m_topLeftCoords.y;
            origin.z = start->z - chunk->m_topLeftCoords.z;

            if (modelFlags) {
                CMap::VectorIntersectDoodadDefs(&chunk->m_doodadDefLinkList, flags);
            }

            if (flags & 0x40F00000) {
                CMap::VectorIntersectEntitys(&chunk->m_entityLinkList, flags);
            }
        }

        uint32_t subX = cellX & 7;
        uint32_t subY = cellY & 7;
        C3Ray ray;
        ray.origin = origin;
        ray.dir = dir;

        if (flags & 0x100) {
            float t = 3.4028235e38f;

            if (chunk->Intersect(subX, subY, ray, &t)) {
                float dist = t * invLength;

                if (bestDist > dist && dist >= 0.0f) {
                    bestDist = dist;
                    bestChunk = chunk;
                }
            }
        }

        if (flags & 0x30000) {
            int32_t checkType = (flags & 0x10000) && !(flags & 0x20000);

            for (CChunkLiquid* liquid = chunk->m_liquidChunkLinkList.Head(); liquid; liquid = chunk->m_liquidChunkLinkList.Next(liquid)) {
                if (checkType && !(g_liquidTypeDB.GetRecord(liquid->m_type)->m_flags & 4)) {
                    continue;
                }

                if (!liquid->TileExists(subX, subY)) {
                    continue;
                }

                uint32_t tilesY = liquid->m_tileEnd.y - liquid->m_tileBegin.y + 1;
                uint32_t vertsY = liquid->m_tileEnd.y - liquid->m_tileBegin.y + 2;
                uint32_t base = subX + tilesY * (subY - liquid->m_tileBegin.x) - liquid->m_tileBegin.y;
                uint32_t pattern[6] = { 0, vertsY, tilesY, 0, 1, vertsY };

                for (uint32_t i = 0; i < 2; i++) {
                    float t = 0.0f;
                    uint32_t tri[3] = { base + pattern[3 * i], base + pattern[3 * i + 1], base + pattern[3 * i + 2] };

                    if (NTempest::Intersect(ray, liquid->m_verts, tri, &t, nullptr, 0.0099999998f)) {
                        float dist = t * invLength;

                        if (bestDist > dist && dist >= 0.0f) {
                            bestDist = dist;
                            bestChunk = chunk;
                        }
                    }
                }
            }
        }
    }

    if (modelFlags) {
        if (flags & 0x16000AE) {
            C3Vector localStart = *start * CWorldScene::camTransportView;
            C3Vector localEnd = *end * CWorldScene::camTransportView;
            World::s_m2Scene->EndHitTest(localStart, localEnd, bestDist, 0);
        } else {
            World::s_m2Scene->EndHitTestCollisionWorld(*start, *end, bestDist);
        }
    }

    if (bestDist >= *distance) {
        return false;
    }

    *distance = bestDist;

    if (hitChunk) {
        *hitChunk = bestChunk;
    }

    return true;
}

// VERIFY: 2f69e7ee auto
// OFFSET: 0x007A39F0
bool CMap::VectorIntersectTerrain(C3Vector* start, C3Vector* end, float* distance, uint32_t flags, CMapChunk** hitChunk) {
    float startX = -(start->y - 17066.666f);
    float startY = -(start->x - 17066.666f);
    float endX = -(end->y - 17066.666f);
    float endY = -(end->x - 17066.666f);

    float dX = endX - startX;
    float dY = endY - startY;

    CiRect cells;
    cells.maxY = CMath::fint_(endY * 0.24f - 0.5f);
    cells.minX = CMath::fint_(startX * 0.24f - 0.5f);
    cells.minY = CMath::fint_(startY * 0.24f - 0.5f);
    cells.maxX = CMath::fint_(endX * 0.24f - 0.5f);

    CMap::cCount = 0;

    if (fabsf(dX) < 0.00000023841858f || cells.minX == cells.maxX) {
        CMap::VectorIntersectSY(cells);
    } else if (fabsf(dY) < 0.00000023841858f || cells.minY == cells.maxY) {
        CMap::VectorIntersectSX(cells);
    } else {
        C3Vector e(endX, endY, 0.0f);
        C3Vector s(startX, startY, 0.0f);

        if (fabsf(dY) >= fabsf(dX)) {
            CMap::VectorIntersectDY(s, e, cells);
        } else {
            CMap::VectorIntersectDX(s, e, cells);
        }
    }

    return CMap::VectorIntersectSubChunkList(start, end, distance, flags, hitChunk);
}


// VERIFY: 35b33716 manual
// OFFSET: 0x007A4270
CFacet* CMap::BuildImpassableFacets(World::FacetData* facets, C3Vector* up, C3Vector* edge, C3Vector* normal, C3Vector* origin) {
    float d = -(normal->z * origin->z + normal->x * origin->x + normal->y * origin->y);

    CFacet* facet = facets->facets.New();
    facet->plane.n = *normal;
    facet->plane.d = d;
    facet->v[0] = *origin;
    facet->v[1] = { origin->x + edge->x, origin->y + edge->y, origin->z + edge->z };
    facet->v[2] = { origin->x + edge->x + up->x, origin->y + edge->y + up->y, origin->z + edge->z + up->z };

    facet = facets->facets.New();
    facet->plane.n = *normal;
    facet->plane.d = d;
    facet->v[0] = *origin;
    facet->v[1] = { origin->x + edge->x + up->x, origin->y + edge->y + up->y, origin->z + edge->z + up->z };
    facet->v[2] = { origin->x + up->x, origin->y + up->y, origin->z + up->z };

    return facet;
}

// VERIFY: f65ff37c manual
// OFFSET: 0x007A43D0
void CMap::CreateImpassableFacets(CMapChunk* chunk, CAaBox* box, World::FacetData* facets) {
    C3Vector up = { 0.0f, 0.0f, 32000.0f };
    C3Vector origin = { 0.0f, 0.0f, 0.0f };
    C3Vector normal = { 0.0f, 0.0f, 0.0f };
    C3Vector edge = { 0.0f, 0.0f, 0.0f };

    if (chunk->m_bbox.b.y > box->b.y) {
        origin = { chunk->m_bbox.b.x, chunk->m_bbox.b.y, chunk->m_bbox.b.z };
        normal = { 0.0f, -1.0f, 0.0f };
        edge = { chunk->m_bbox.t.x - chunk->m_bbox.b.x, 0.0f, 0.0f };
        CMap::BuildImpassableFacets(facets, &up, &edge, &normal, &origin);
    }

    if (chunk->m_bbox.t.y < box->t.y) {
        origin = { chunk->m_bbox.t.x, chunk->m_bbox.t.y, chunk->m_bbox.b.z };
        normal = { 0.0f, 1.0f, 0.0f };
        edge = { chunk->m_bbox.b.x - chunk->m_bbox.t.x, 0.0f, 0.0f };
        CMap::BuildImpassableFacets(facets, &up, &edge, &normal, &origin);
    }

    if (chunk->m_bbox.b.x > box->b.x) {
        origin = { chunk->m_bbox.b.x, chunk->m_bbox.t.y, chunk->m_bbox.b.z };
        normal = { -1.0f, 0.0f, 0.0f };
        edge = { 0.0f, chunk->m_bbox.b.y - chunk->m_bbox.t.y, 0.0f };
        CMap::BuildImpassableFacets(facets, &up, &edge, &normal, &origin);
    }

    if (chunk->m_bbox.t.x < box->t.x) {
        origin = { chunk->m_bbox.t.x, chunk->m_bbox.b.y, chunk->m_bbox.b.z };
        normal = { 1.0f, 0.0f, 0.0f };
        edge = { 0.0f, chunk->m_bbox.t.y - chunk->m_bbox.b.y, 0.0f };
        CMap::BuildImpassableFacets(facets, &up, &edge, &normal, &origin);
    }
}


// VERIFY: 2b594300 manual
// OFFSET: 0x007A4AF0
void CMap::AppendMapObjFacets(CMapDoodadDef* def, CAaBox* box, World::FacetData* facets) {
    uint32_t first = facets->facets.Count();

    def->m_model->GetCollisionFacets(*box, def->m_mat, facets->facets);

    uint32_t count = facets->facets.Count();
    facets->facetIds.SetCount(count);

    for (uint32_t i = first; i < count; i++) {
        facets->facetIds.m_data[i] = def->m_collisionGuid.m_guid;
    }
}

// VERIFY: e2b4677b manual
// OFFSET: 0x007A4C10
void CMap::TestQueryAdd(const CFacet& facet, CImVector color, const C44Matrix* mat) {
    C44Matrix identity;
    uint16_t base = static_cast<uint16_t>(CMap::debugVertexArray.Count());

    if (!mat) {
        mat = &identity;
    }

    for (uint32_t i = 0; i < 3; i++) {
        CGxVertexPC* vertex = CMap::debugVertexArray.New();
        vertex->p = facet.v[i] * *mat;
        vertex->c = color;
    }

    *CMap::debugIndexArray.New() = base;
    *CMap::debugIndexArray.New() = base + 1;
    *CMap::debugIndexArray.New() = base + 2;
}

// VERIFY: de8ab0e3 manual
// OFFSET: 0x007AD3B0
bool CMap::GetHeightTerrain(CMapChunk* chunk, C3Vector& pos, int32_t cellX, int32_t cellY, float* outHeight) {
    int32_t col = cellX & 7;
    int32_t row = cellY & 7;

    if (chunk->m_header->m_holes & CMap::s_holeMask[4 * (row >> 1) + (col >> 1)]) {
        return false;
    }

    float originX = row * -4.1666665f;
    float originY = -4.1666665f * col;

    float dx = pos.x - chunk->m_topLeftCoords.x;
    float dy = pos.y - chunk->m_topLeftCoords.y;

    int32_t fan = 0;

    if (((originX + CMap::s_subchunkCoords[2].x) - (originX + CMap::s_subchunkCoords[0].x)) * ((originY + CMap::s_subchunkCoords[2].y) - dy) - ((originY + CMap::s_subchunkCoords[2].y) - (originY + CMap::s_subchunkCoords[0].y)) * ((originX + CMap::s_subchunkCoords[2].x) - dx) <= 0.0f) {
        fan = 1;
    }

    if (((originX + CMap::s_subchunkCoords[3].x) - (originX + CMap::s_subchunkCoords[1].x)) * ((originY + CMap::s_subchunkCoords[3].y) - dy) - ((originY + CMap::s_subchunkCoords[3].y) - (originY + CMap::s_subchunkCoords[1].y)) * ((originX + CMap::s_subchunkCoords[3].x) - dx) <= 0.0f) {
        fan += 2;
    }

    float* height = &chunk->m_height[row + col + 16 * row];

    C3Vector a(originX + CMap::s_subchunkCoords[CMap::s_subchunkIndices[2 * fan]].x, originY + CMap::s_subchunkCoords[CMap::s_subchunkIndices[2 * fan]].y, height[CMap::s_fanIndices[2 * fan]]);
    C3Vector b(originX + CMap::s_subchunkCoords[CMap::s_subchunkIndices[2 * fan + 1]].x, originY + CMap::s_subchunkCoords[CMap::s_subchunkIndices[2 * fan + 1]].y, height[CMap::s_fanIndices[2 * fan + 1]]);
    C3Vector c(originX + CMap::s_subchunkCoords[4].x, originY + CMap::s_subchunkCoords[4].y, height[9]);

    if (CMap::s_useSSE) {
        float ax = a.x - c.x;
        float ay = a.y - c.y;
        float az = a.z - c.z;
        float bx = b.x - c.x;
        float by = b.y - c.y;
        float bz = b.z - c.z;

        float nx = bz * ay - az * by;
        float ny = az * bx - bz * ax;
        float nz = ax * by - ay * bx;

        float scale = 1.0f / sqrtf(ny * ny + nz * nz + 0.0f * 0.0f + nx * nx);

        *outHeight = chunk->m_topLeftCoords.z - (ny * scale * dy + nx * scale * dx - (c.z * (nz * scale) + c.y * (ny * scale) + c.x * (nx * scale))) / (nz * scale);

        return true;
    }

    C4Plane plane;
    plane.From3Pos(c, a, b);

    *outHeight = chunk->m_topLeftCoords.z - (plane.n.y * dy + plane.n.x * dx + plane.d) / plane.n.z;

    return true;
}

// VERIFY: 468f4c9b manual
// OFFSET: 0x007B4090
void CMapObjDefGroup::UpdateLights() {
    for (CMapBaseObjLink* link = this->m_doodadDefLinkList.Head(); link; link = this->m_doodadDefLinkList.Next(link)) {
        link->m_owner->m_flags = static_cast<MapObjFlag>(link->m_owner->m_flags | 0x1);
    }

    for (CMapBaseObjLink* link = this->m_entityLinkList.Head(); link; link = this->m_entityLinkList.Next(link)) {
        link->m_owner->m_flags = static_cast<MapObjFlag>(link->m_owner->m_flags | 0x1);
    }
}

// VERIFY: 7a5c5c66 manual
// OFFSET: 0x007B47F0
static int32_t CompareAreaEntries(const void* a, const void* b) {
    const CMapAreaEntry* entryA = static_cast<const CMapAreaEntry*>(a);
    const CMapAreaEntry* entryB = static_cast<const CMapAreaEntry*>(b);

    if (entryB->m_dist > entryA->m_dist) {
        return -1;
    }

    return entryB->m_dist < entryA->m_dist;
}

// VERIFY: 9faa97d5 manual
// OFFSET: 0x007B4960
CMapArea* CMap::GetAreaAtPosition(const C2Vector* pos) {
    float x = -(pos->x - 17066.666f);
    float y = -(pos->y - 17066.666f) * 0.001875f;
    x = x * 0.001875f;

    return CMap::m_areaTable[64 * CMath::fint_(x - 0.5f) + CMath::fint_(y - 0.5f)];
}

// VERIFY: 8231832c manual
// OFFSET: 0x007B49C0
CMapChunk* CMap::GetChunkAtPosition(const C2Vector* pos) {
    float x = -(pos->x - 17066.666f);
    float y = -(pos->y - 17066.666f) * 0.03f;
    x = x * 0.03f;

    int32_t chunkY = CMath::fint_(x - 0.5f);
    int32_t chunkX = CMath::fint_(y - 0.5f);
    CMapArea* area = CMap::m_areaTable[64 * ((chunkY >> 4) & 0x3F) + ((chunkX >> 4) & 0x3F)];

    if (!area) {
        return nullptr;
    }

    return area->m_mapChunks[(chunkX & 0xF) + 16 * (chunkY & 0xF)];
}

// VERIFY: f1ab17a0 manual
// OFFSET: 0x007B4BC0
void CMap::UpdateBarriers() {
    int32_t mode = CWorldScene::s_barrier.m_mode;
    int32_t minX = CWorld::gbChunkRect.minX >> 4;
    int32_t maxX = CWorld::gbChunkRect.maxX >> 4;
    int32_t minY = CWorld::gbChunkRect.minY >> 4;
    int32_t maxY = CWorld::gbChunkRect.maxY >> 4;

    if (!mode) {
        return;
    }

    for (int32_t y = minY; y <= maxY; y++) {
        for (int32_t x = minX; x <= maxX; x++) {
            int32_t index = 64 * y + x;

            if (!(CMap::m_areaInfo[index].m_flags & 1)) {
                continue;
            }

            CMapArea* area = CMap::m_areaTable[index];

            if (!area || area->m_asyncObject) {
                continue;
            }

            CMapArea* neighbor = CMap::m_areaTable[index - 1];

            if (!neighbor || neighbor->m_asyncObject || mode == 2) {
                C2Vector start(area->m_bounds.b.x, area->m_bounds.t.y);
                C2Vector end(area->m_bounds.t.x, area->m_bounds.t.y);
                CWorldScene::s_barrier.AddAxisAlignedWall(start, end, 40.0f);
                mode = CWorldScene::s_barrier.m_mode;
            }

            neighbor = CMap::m_areaTable[index - 64];

            if (!neighbor || neighbor->m_asyncObject || mode == 2) {
                C2Vector start(area->m_bounds.t.x, area->m_bounds.t.y);
                C2Vector end(area->m_bounds.t.x, area->m_bounds.b.y);
                CWorldScene::s_barrier.AddAxisAlignedWall(start, end, 40.0f);
                mode = CWorldScene::s_barrier.m_mode;
            }

            neighbor = CMap::m_areaTable[index + 1];

            if (!neighbor || neighbor->m_asyncObject || mode == 2) {
                C2Vector start(area->m_bounds.t.x, area->m_bounds.b.y);
                C2Vector end(area->m_bounds.b.x, area->m_bounds.b.y);
                CWorldScene::s_barrier.AddAxisAlignedWall(start, end, 40.0f);
                mode = CWorldScene::s_barrier.m_mode;
            }

            neighbor = CMap::m_areaTable[index + 64];

            if (!neighbor || neighbor->m_asyncObject || mode == 2) {
                C2Vector start(area->m_bounds.b.x, area->m_bounds.b.y);
                C2Vector end(area->m_bounds.b.x, area->m_bounds.t.y);
                CWorldScene::s_barrier.AddAxisAlignedWall(start, end, 40.0f);
                mode = CWorldScene::s_barrier.m_mode;
            }
        }
    }
}

// VERIFY: 9eee5402 auto
// OFFSET: 0x007B4DF0
void CMap::UpdateArea(int32_t addToScene, CMapArea* area, CiRect* chunkRect, int32_t depth) {
    if (chunkRect->minX > CWorld::gbChunkRect.maxX || chunkRect->minY > CWorld::gbChunkRect.maxY || chunkRect->maxX < CWorld::gbChunkRect.minX || chunkRect->maxY < CWorld::gbChunkRect.minY) {
        area->PurgeChunks(chunkRect);
    } else if (depth == 2) {
        area->Update(addToScene, chunkRect);
    } else {
        CiRect rect;
        rect.minY = chunkRect->minY;
        rect.minX = chunkRect->minX;
        rect.maxY = chunkRect->minY + ((chunkRect->maxY - chunkRect->minY) >> 1);
        rect.maxX = chunkRect->minX + ((chunkRect->maxX - chunkRect->minX) >> 1);
        CMap::UpdateArea(addToScene, area, &rect, depth + 1);

        rect.minX = ((chunkRect->maxX - chunkRect->minX) >> 1) + chunkRect->minX + 1;
        rect.maxX = chunkRect->maxX;
        CMap::UpdateArea(addToScene, area, &rect, depth + 1);

        rect.minY = ((chunkRect->maxY - chunkRect->minY) >> 1) + chunkRect->minY + 1;
        rect.maxY = chunkRect->maxY;
        CMap::UpdateArea(addToScene, area, &rect, depth + 1);

        rect.minX = chunkRect->minX;
        rect.maxX = chunkRect->minX + ((chunkRect->maxX - chunkRect->minX) >> 1);
        CMap::UpdateArea(addToScene, area, &rect, depth + 1);
    }
}

// VERIFY: a9b440d7 manual
// OFFSET: 0x007B50B0
uint32_t CMap::ComputeMapObjLoadProgress(CMapChunk* chunk, float* progress, uint32_t total) {
    CAaBox box;
    box.b.x = 0.0f;
    box.b.y = 0.0f;
    box.b.z = 0.0f;
    box.t.x = 0.0f;
    box.t.y = 0.0f;
    box.t.z = 0.0f;
    *progress = 0.0f;
    float scale = 0.0f;

    if (total) {
        scale = 1.0f / static_cast<float>(total);
    }

    TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list;

    if (CMap::bDungeon) {
        box.b.x = World::s_currentWorldPos.x - 16.666666f;
        box.b.y = World::s_currentWorldPos.y - 16.666666f;
        box.b.z = World::s_currentWorldPos.z - 16.666666f;
        box.t.x = World::s_currentWorldPos.x + 16.666666f;
        box.t.y = World::s_currentWorldPos.y + 16.666666f;
        box.t.z = World::s_currentWorldPos.z + 16.666666f;
        list = &CMap::mapObjDefLinkList;
    } else {
        if (!chunk) {
            return 0;
        }

        box = chunk->m_bbox;
        box.b.z = World::s_currentWorldPos.z - 16.666666f;
        box.t.z = World::s_currentWorldPos.z + 16.666666f;
        list = &chunk->m_mapObjDefLinkList;
    }

    uint32_t remaining = 0;

    for (CMapBaseObjLink* link = list->Head(); link; link = list->Next(link)) {
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(link->m_owner);

        if (!(mapObjDef->m_flags & MAPOBJ_FLAG_PREPARED) || !box.Intersects(mapObjDef->m_bbox)) {
            continue;
        }

        for (CMapBaseObjLink* groupLink = mapObjDef->m_mapObjDefGroupLinkList.Head(); groupLink; groupLink = mapObjDef->m_mapObjDefGroupLinkList.Next(groupLink)) {
            CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(groupLink->m_owner);

            if (box.t.x < defGroup->m_bbox.b.x || box.t.y < defGroup->m_bbox.b.y || box.t.z < defGroup->m_bbox.b.z || box.b.x > defGroup->m_bbox.t.x || box.b.y > defGroup->m_bbox.t.y || box.b.z > defGroup->m_bbox.t.z) {
                continue;
            }

            if (!(defGroup->m_flags & MAPOBJ_FLAG_PREPARED)) {
                if (!mapObjDef->m_owner->IsGroupLoading(defGroup->m_groupNum)) {
                    continue;
                }

                CMapObjGroup* group = mapObjDef->m_owner->GetGroup(defGroup->m_groupNum, 0);

                if (group && group->m_distToCamera > 0.0f) {
                    group->m_distToCamera = 0.0f;
                }

                remaining++;
                continue;
            }

            int32_t pending = 0;

            for (CMapBaseObjLink* doodadLink = defGroup->m_doodadDefLinkList.Head(); doodadLink; doodadLink = defGroup->m_doodadDefLinkList.Next(doodadLink)) {
                CMapDoodadDef* doodadDef = static_cast<CMapDoodadDef*>(doodadLink->m_owner);

                if ((!doodadDef->m_model || !(doodadDef->m_flags & MAPOBJ_FLAG_PREPARED)) && !(doodadDef->m_staticFlags & 0x10000)) {
                    doodadDef->m_staticFlags |= 0x10000;

                    if (!pending) {
                        pending = 1;
                    }
                }
            }

            if (pending) {
                remaining++;
            }
        }
    }

    *progress += static_cast<float>(total - remaining) * scale;
    return remaining;
}

// VERIFY: f7ac6b15 manual
// OFFSET: 0x007B53B0
void CMap::ClearChunkSortList() {
    CMapChunk* chunk = CMap::s_mapChunkSortList.Head();

    while (chunk) {
        CMapChunk* next = CMap::s_mapChunkSortList.Next(chunk);
        chunk->m_sortListLink.Unlink();
        chunk = next;
    }
}

// VERIFY: fa65766b manual
// OFFSET: 0x007B5420
void CMap::ProcessChunkLiquidUpdateList() {
    CChunkLiquid* liquid = CMap::s_mapChunkLiquidUpdateList.Head();

    while (liquid) {
        CChunkLiquid* next = CMap::s_mapChunkLiquidUpdateList.Next(liquid);
        liquid->UpdateResourcePurging();

        if (liquid->m_resPurgeTimer < 0.0f) {
            liquid->m_updateLink.Unlink();
        }

        liquid = next;
    }
}

// VERIFY: cd78794a manual
// OFFSET: 0x007B54A0
void CMap::ProcessDetailDoodadUpdateList() {
    CDetailDoodadInst* inst = CMap::s_detailDoodadUpdateList.Head();

    while (inst) {
        CDetailDoodadInst* next = CMap::s_detailDoodadUpdateList.Next(inst);

        if (++inst->m_age > 10) {
            if (inst->HasBatches()) {
                inst->FreeBuffers();
            }

            if (inst->m_age > 20) {
                CDetailDoodad::FreeInst(inst);
            }
        }

        inst = next;
    }
}

// VERIFY: 21d0bf10 auto
// OFFSET: 0x007B5500
void CMap::ProcessRenderChunkUpdateList() {
    CMapRenderChunk* renderChunk = CMap::s_mapRenderChunkUpdateList.Head();

    while (renderChunk) {
        renderChunk->m_lastUpdateTime = FrameTime::s_tickTimeSec + renderChunk->m_lastUpdateTime;
        CMapRenderChunk* next = CMap::s_mapRenderChunkUpdateList.Next(renderChunk);

        if (renderChunk->m_lastUpdateTime > 2.0f) {
            renderChunk->FreeBuf();
        }

        if (!renderChunk->m_chunkBuf) {
            renderChunk->m_renderChunkLink.Unlink();
        }

        renderChunk = next;
    }
}

// VERIFY: a5870e86 manual
// OFFSET: 0x007B5590
void CMap::PrepareEntitys(int32_t addToScene) {
    if (!addToScene) {
        return;
    }

    for (CMapEntity* entity = CMap::entityList.Head(); entity; entity = CMap::entityList.Next(entity)) {
        if (!(entity->m_staticFlags & 0x4)) {
            CWorldScene::AddEntityToSortTable(entity);
        }
    }
}

// VERIFY: c3245130 manual
// OFFSET: 0x007B5630
void CMap::PrepareMapDoodadDefs() {
    int32_t streaming = SFile::IsStreamingMode();
    CMapDoodadDef* doodadDef = CMap::doodadDefList.Head();

    while (doodadDef) {
        CM2Model* model = doodadDef->m_model;
        CMapDoodadDef* next = CMap::doodadDefList.Next(doodadDef);

        if (!model || model->IsLoaded(0, 0)) {
            if (!(doodadDef->m_staticFlags & 0x10)) {
                doodadDef->UpdateBounds();

                if (!(doodadDef->m_flags & MAPOBJ_FLAG_INTERIOR) && CMap::QueryShadow(&doodadDef->m_position)) {
                    doodadDef->m_diffuseLightScale = 0.5f;
                }

                doodadDef->ExtendChunkBounds();
                doodadDef->m_flags = static_cast<MapObjFlag>(doodadDef->m_flags | MAPOBJ_FLAG_PREPARED | MAPOBJ_FLAG_UNPLACED);
            }

            doodadDef->m_doodadDefLink.Unlink();
        } else if (streaming && (doodadDef->m_staticFlags & 0x10000)) {
            doodadDef->m_model->LinkToAsyncHead(1);
            doodadDef->m_staticFlags &= ~0x10000u;
        }

        doodadDef = next;
    }
}

// VERIFY: d6c4755a manual
// OFFSET: 0x007B5950
void CMap::PreUpdateAreas(bool addToScene) {
    int32_t maxY = CWorld::gbChunkRect.maxY >> 4;
    int32_t minX = CWorld::gbChunkRect.minX >> 4;
    int32_t minY = CWorld::gbChunkRect.minY >> 4;
    int32_t maxX = CWorld::gbChunkRect.maxX >> 4;
    int32_t entryCount = 0;
    CMapAreaEntry* entries = static_cast<CMapAreaEntry*>(alloca(1024 * sizeof(CMapAreaEntry)));

    CMap::ClearChunkSortList();
    CMap::ProcessChunkLiquidUpdateList();
    CMap::ProcessRenderChunkUpdateList();
    CMap::ProcessDetailDoodadUpdateList();

    int32_t waitForLoad = !(CMap::s_isStreamingMode || CMap::bPreload);
    CMapBaseObjLink* link = CMap::s_unk_ADFBEC.Head();

    while (link) {
        CMapArea* area = static_cast<CMapArea*>(link->m_owner);
        CMapBaseObjLink* next = CMap::s_unk_ADFBEC.Next(link);

        if (area->m_index.x >= minX && area->m_index.x <= maxX && area->m_index.y >= minY && area->m_index.y <= maxY) {
            C2Vector pos(World::s_currentWorldPos.x, World::s_currentWorldPos.y);
            entries[entryCount].m_dist = NTempest::DistanceSquared(area->m_bounds, pos);
            entries[entryCount].m_area = area;
            entryCount++;
        } else if (!area->m_asyncObject || !area->m_asyncObject->m_isCurrent) {
            CMap::FreeBaseObjLink(link);
            CMap::PurgeArea(area);
        }

        link = next;
    }

    for (int32_t y = minY; y <= maxY; y++) {
        for (int32_t x = minX; x <= maxX; x++) {
            int32_t index = 64 * y + x;

            if ((CMap::m_areaInfo[index].m_flags & 1) && !CMap::m_areaTable[index]) {
                CMapArea* area = CMap::PrepareArea(x, y);
                C2Vector pos(World::s_currentWorldPos.x, World::s_currentWorldPos.y);
                entries[entryCount].m_area = area;
                entries[entryCount].m_dist = NTempest::DistanceSquared(area->m_bounds, pos);
                entryCount++;
            }
        }
    }

    if (entryCount) {
        qsort(entries, entryCount, sizeof(CMapAreaEntry), CompareAreaEntries);
    }

    int32_t lastPending = -1;
    CiRect rect;
    memset(&rect, 0, sizeof(rect));

    for (int32_t i = 0; i < entryCount; i++) {
        CMapArea* area = entries[i].m_area;

        if (!area->m_fileBuffer) {
            CMap::LoadArea(area);
        }

        if (area->m_index.x < minX || area->m_index.x > maxX || area->m_index.y < minY || area->m_index.y > maxY) {
            continue;
        }

        rect.minX = 16 * area->m_index.x;
        rect.minY = 16 * area->m_index.y;
        rect.maxX = 16 * area->m_index.x + 15;
        rect.maxY = 16 * area->m_index.y + 15;

        if (rect.minX <= CWorld::chunkRect.maxX && rect.minY <= CWorld::chunkRect.maxY && rect.maxX >= CWorld::chunkRect.minX && rect.maxY >= CWorld::chunkRect.minY && waitForLoad && area->m_asyncObject) {
            AsyncFileReadWait(area->m_asyncObject);
        }

        if (area->m_asyncObject) {
            if (CMap::s_isStreamingMode && entries[i].m_dist < 71111.109f) {
                lastPending = i;
            }
        } else {
            rect.minX = area->m_tileChunkIndex.x;
            rect.minY = area->m_tileChunkIndex.y;
            rect.maxX = area->m_tileChunkIndex.x + 15;
            rect.maxY = area->m_tileChunkIndex.y + 15;
            CMap::UpdateArea(addToScene, area, &rect, 0);
        }
    }

    if (CMap::s_isStreamingMode) {
        AsyncFile::EnterQueueLock();

        for (int32_t i = lastPending; i >= 0; i--) {
            CMapArea* area = entries[i].m_area;

            if (area->m_asyncObject && (entries[i].m_dist < 1111.1111f || (entries[i].m_dist < 71111.109f && !CWorldScene::FrustumCull(&area->m_bounds)))) {
                AsyncFileReadLinkObject(area->m_asyncObject, 1);
            }
        }

        AsyncFile::LeaveQueueLock();
    }

    if (addToScene) {
        CMap::UpdateBarriers();
    }
}

// VERIFY: 11c34df4 manual
// OFFSET: 0x007B5D00
void CMap::PrepareMapObjDef(CMapObjDef* mapObjDef, CMapObj* mapObj) {
    CAaBox bounds;
    mapObjDef->m_flags = static_cast<MapObjFlag>(mapObjDef->m_flags | 0x80);
    mapObj->GetBounds(mapObjDef->m_sphere);
    mapObjDef->m_sphere.c *= mapObjDef->m_mat;
    mapObj->GetBounds(&bounds);
    CWorldMath::TransformAABox(mapObjDef->m_mat, bounds, mapObjDef->m_bbox);
    mapObjDef->m_argbColor = *reinterpret_cast<uint32_t*>(&mapObj->m_argbColor);
    mapObjDef->m_lightsArray.SetCount(mapObj->m_ligtsCount);

    for (uint32_t i = 0; i < mapObjDef->m_lightsArray.Count(); i++) {
        mapObjDef->m_lightsArray[i] = nullptr;
    }

    CMap::CreateMapObjDefGroups(mapObj, mapObjDef);
}

// VERIFY: 1db2d5f4 manual
// OFFSET: 0x007B5E80
uint32_t CMap::ComputeDoodadLoadProgress(CMapChunk* chunk, float* progress, uint32_t total) {
    *progress = 0.0f;
    float scale = 0.0f;

    if (total) {
        scale = 1.0f / static_cast<float>(total);
    }

    CAaBox box;
    TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list;

    if (CMap::bDungeon) {
        box.b.x = World::s_currentWorldPos.x - 16.666666f;
        box.b.y = World::s_currentWorldPos.y - 16.666666f;
        box.b.z = World::s_currentWorldPos.z - 16.666666f;
        box.t.x = World::s_currentWorldPos.x + 16.666666f;
        box.t.y = World::s_currentWorldPos.y + 16.666666f;
        box.t.z = World::s_currentWorldPos.z + 16.666666f;
        list = &CMap::s_unk_ADFBF8;
    } else {
        if (!chunk) {
            return 0;
        }

        box = chunk->m_bbox;
        box.b.z = World::s_currentWorldPos.z - 16.666666f;
        box.t.z = World::s_currentWorldPos.z + 16.666666f;
        list = &chunk->m_doodadDefLinkList;
    }

    uint32_t remaining = 0;

    for (CMapBaseObjLink* link = list->Head(); link; link = list->Next(link)) {
        CMapDoodadDef* doodadDef = static_cast<CMapDoodadDef*>(link->m_owner);

        if ((doodadDef->m_model && (doodadDef->m_flags & MAPOBJ_FLAG_PREPARED)) || (doodadDef->m_staticFlags & 0x10000)) {
            continue;
        }

        if (total) {
            int64_t loaded = 0;
            int64_t size = 0;
            doodadDef->m_model->ComputeLoadProgress(loaded, size);
            *progress += static_cast<float>(loaded) / static_cast<float>(size) * scale;
        }

        doodadDef->m_staticFlags |= 0x10000;
        remaining++;
    }

    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.m_fulllist.Next(mapObjDef)) {
        if (mapObjDef->m_flags & MAPOBJ_FLAG_PREPARED) {
            continue;
        }

        CMapObj* mapObj = mapObjDef->m_owner;

        if (!mapObj->m_asyncObject) {
            continue;
        }

        if (box.t.x >= mapObjDef->m_bbox.b.x && box.t.y >= mapObjDef->m_bbox.b.y && box.t.z >= mapObjDef->m_bbox.b.z && box.b.x <= mapObjDef->m_bbox.t.x && box.b.y <= mapObjDef->m_bbox.t.y && box.b.z <= mapObjDef->m_bbox.t.z) {
            remaining++;

            if (mapObj->m_distToCamera > 0.0f) {
                mapObj->m_distToCamera = 0.0f;
            }
        }
    }

    *progress += static_cast<float>(total - remaining) * scale;
    return remaining;
}

// VERIFY: 61aca6d6 manual
// OFFSET: 0x007B6110
void CMap::PrepareMapObjDefs(bool addToScene) {
    int32_t waitLoad = 1;

    if (CMap::s_isStreamingMode || CMap::bPreload) {
        waitLoad = 0;
    }

    CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head();

    while (mapObjDef) {
        CMapObj* mapObj = mapObjDef->m_owner;
        CAaBox& defBox = mapObjDef->m_bbox;

        if (CWorld::s_objectAreaOfInterest.b.x <= defBox.t.x && CWorld::s_objectAreaOfInterest.b.y <= defBox.t.y && CWorld::s_objectAreaOfInterest.b.z <= defBox.t.z && CWorld::s_objectAreaOfInterest.t.x >= defBox.b.x && CWorld::s_objectAreaOfInterest.t.y >= defBox.b.y && CWorld::s_objectAreaOfInterest.t.z >= defBox.b.z) {
            if (waitLoad && !mapObj->m_isGroupLoaded) {
                mapObj->WaitLoad();
            }

            if (!(mapObjDef->m_flags & MAPOBJ_FLAG_PREPARED) && mapObj->m_isGroupLoaded) {
                CMap::PrepareMapObjDef(mapObjDef, mapObj);
            }
        }

        float dist = NTempest::DistanceSquared(defBox, World::s_currentWorldPos);

        if (dist < mapObj->m_distToCamera) {
            mapObj->m_distToCamera = dist;
        }

        for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
            CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(link->m_owner);
            CMapObjGroup* group = mapObj->GetGroup(defGroup->m_groupNum, 1);
            CAaBox& box = defGroup->m_bbox;

            if (CWorld::s_objectAreaOfInterest.b.x <= box.t.x && CWorld::s_objectAreaOfInterest.b.y <= box.t.y && CWorld::s_objectAreaOfInterest.b.z <= box.t.z && CWorld::s_objectAreaOfInterest.t.x >= box.b.x && CWorld::s_objectAreaOfInterest.t.y >= box.b.y && CWorld::s_objectAreaOfInterest.t.z >= box.b.z) {
                if (!(group->m_unkLoadedFlag & 1)) {
                    if (!group->m_asyncObjPtr) {
                        mapObj->ReadGroup(defGroup->m_groupNum);
                    }

                    if (waitLoad && CWorld::s_groupAreaOfInterest.b.x <= box.t.x && CWorld::s_groupAreaOfInterest.b.y <= box.t.y && CWorld::s_groupAreaOfInterest.b.z <= box.t.z && CWorld::s_groupAreaOfInterest.t.x >= box.b.x && CWorld::s_groupAreaOfInterest.t.y >= box.b.y && CWorld::s_groupAreaOfInterest.t.z >= box.b.z) {
                        mapObj->WaitLoadGroup(defGroup->m_groupNum);
                    }
                }

                group->unused_0190 = 0.0f;

                if (group->m_unkLoadedFlag & 1) {
                    if (!(defGroup->m_flags & MAPOBJ_FLAG_GROUP_INIT)) {
                        CMapObj::InitDefGroup(mapObj, group, mapObjDef, defGroup);
                    }

                    if (!(defGroup->m_flags & MAPOBJ_FLAG_REFS_CREATED)) {
                        CMapObj::CreateRefs(mapObj, group, mapObjDef, defGroup);
                        CMap::FreeBaseObjLinksInBounds(&box);
                    }
                }
            }

            float groupDist = NTempest::DistanceSquared(box, World::s_currentWorldPos);

            if (groupDist < group->m_distToCamera) {
                group->m_distToCamera = groupDist;
            }

            if (!addToScene || !(mapObjDef->m_flags & MAPOBJ_FLAG_PREPARED)) {
                continue;
            }

            if (!(group->m_unkLoadedFlag & 1)) {
                CWorldScene::s_barrier.AddBarrierMapObjDefGroup(defGroup, 50.0f);
                continue;
            }

            if (mapObjDef->m_flags & MAPOBJ_FLAG_DISABLED) {
                continue;
            }

            if (CWorldScene::boundingBox.Intersects(box)) {
                CWorldScene::AddMapObjDefGroup(mapObjDef, defGroup);
            }

            for (CMapObjOccluder* occluder = group->m_occluderList.Head(); occluder; occluder = group->m_occluderList.Next(occluder)) {
                C3Vector a = occluder->m_start * mapObjDef->m_mat;
                C3Vector b = occluder->m_end * mapObjDef->m_mat;
                CWorldView::AddOccluder(a, b);
            }
        }

        if (!(mapObjDef->m_flags & MAPOBJ_FLAG_PREPARED)) {
            CWorldScene::s_barrier.AddBarrier(defBox, 50.0f);
        }

        mapObjDef = CMap::mapObjDefHash.Next(mapObjDef);
    }
}

// VERIFY: 6908aa4f manual
// OFFSET: 0x007B6B00
void CMap::PrepareUpdate(bool addToScene) {
    if (CWorld::status.m_chunksChanged) {
        CMap::PurgeMaps();
    }

    CMap::bspRecurseCount = 0;
    CMap::mapGetFacetsCount = 0;
    CMap::oldSelectLightParm = 0;
    CMapChunkLiquidBuf::Update(FrameTime::s_tickTimeSec);
    CMapObj::PrepareUpdate();
    CMapRenderChunk::PurgeChunkBufBlocks();
    CMap::PreUpdateAreas(addToScene);
    CMap::PrepareMapObjDefs(addToScene);
    CMap::PrepareMapDoodadDefs();
    CMap::PrepareEntitys(addToScene);

    if (!CMap::bPreload) {
        return;
    }

    if (!CMap::s_isStreamingMode) {
        AsyncFileReadWaitAll();

        if (World::s_loadProgressCallback) {
            World::s_loadProgressCallback(0.25f, World::s_loadProgressParam);
        }

        CMap::PreUpdateAreas(addToScene);
        CMap::PrepareMapObjDefs(addToScene);
        CMapObj::PrepareUpdate();
        CMap::PrepareMapDoodadDefs();
        AsyncFileReadWaitAll();

        if (World::s_loadProgressCallback) {
            World::s_loadProgressCallback(0.5f, World::s_loadProgressParam);
        }

        CMap::PreUpdateAreas(addToScene);
        CMap::PrepareMapObjDefs(addToScene);
        CMapObj::PrepareUpdate();
        CMap::PrepareMapDoodadDefs();
        AsyncFileReadWaitAll();

        if (World::s_loadProgressCallback) {
            World::s_loadProgressCallback(0.66000003f, World::s_loadProgressParam);
        }

        CMap::PreUpdateAreas(addToScene);
        CMap::PrepareMapObjDefs(addToScene);
        CMapObj::PrepareUpdate();
        CMap::PrepareMapDoodadDefs();
        AsyncFileReadWaitAll();

        if (World::s_loadProgressCallback) {
            World::s_loadProgressCallback(0.75f, World::s_loadProgressParam);
        }

        return;
    }

    CMapArea* area = CMap::GetAreaAtPosition(reinterpret_cast<const C2Vector*>(&World::s_currentWorldPos));

    if (area && area->m_asyncObject) {
        char fileName[260];
        SFile::GetActualFileName(area->m_asyncObject->m_file, fileName, sizeof(fileName));
        AsyncFile::EnterQueueLock();
        AsyncFileReadLinkObject(area->m_asyncObject, 1);
        AsyncFile::LeaveQueueLock();

        while (area->m_asyncObject) {
            int64_t local = 0;
            int64_t total = 0;
            SFile::FileGetIsLocalAmount(fileName, local, total);
            AsyncFileReadPollHandler(nullptr, nullptr);

            if (World::s_loadProgressCallback) {
                World::s_loadProgressCallback(static_cast<float>(local) / static_cast<float>(total) * 0.2f, World::s_loadProgressParam);
            }

            OsSleep(10);
        }
    }

    if (World::s_loadProgressCallback) {
        World::s_loadProgressCallback(0.2f, World::s_loadProgressParam);
    }

    CMap::PreUpdateAreas(addToScene);
    CMap::PrepareMapObjDefs(addToScene);
    CMapObj::PrepareUpdate();
    CMap::PrepareMapDoodadDefs();
    AsyncFileReadPollHandler(nullptr, nullptr);

    CMapChunk* chunk = CMap::GetChunkAtPosition(reinterpret_cast<const C2Vector*>(&World::s_currentWorldPos));
    float progress = 0.0f;
    uint32_t doodadTotal = CMap::ComputeDoodadLoadProgress(chunk, &progress, 0);

    if (doodadTotal) {
        uint32_t remaining;

        do {
            OsSleep(10);
            AsyncFileReadPollHandler(nullptr, nullptr);
            CMap::PreUpdateAreas(addToScene);
            CMap::PrepareMapObjDefs(addToScene);
            CMapObj::PrepareUpdate();
            CMap::PrepareMapDoodadDefs();
            remaining = CMap::ComputeDoodadLoadProgress(chunk, &progress, doodadTotal);

            if (World::s_loadProgressCallback) {
                World::s_loadProgressCallback(progress * 0.25f + 0.2f, World::s_loadProgressParam);
            }
        } while (remaining);
    }

    CMap::PreUpdateAreas(addToScene);
    CMap::PrepareMapObjDefs(addToScene);
    CMapObj::PrepareUpdate();
    CMap::PrepareMapDoodadDefs();
    AsyncFileReadPollHandler(nullptr, nullptr);

    uint32_t mapObjTotal = CMap::ComputeMapObjLoadProgress(chunk, &progress, 0);
    CMapObj::PrepareUpdate();

    if (mapObjTotal) {
        uint32_t remaining;

        do {
            OsSleep(10);
            AsyncFileReadPollHandler(nullptr, nullptr);
            CMap::PreUpdateAreas(addToScene);
            CMap::PrepareMapObjDefs(addToScene);
            CMapObj::PrepareUpdate();
            CMap::PrepareMapDoodadDefs();
            remaining = CMap::ComputeMapObjLoadProgress(chunk, &progress, mapObjTotal);

            if (World::s_loadProgressCallback) {
                World::s_loadProgressCallback(static_cast<float>(mapObjTotal - remaining) * 0.30000001f / static_cast<float>(mapObjTotal) + 0.44999999f, World::s_loadProgressParam);
            }
        } while (remaining);
    }

    CMap::PreUpdateAreas(addToScene);
    CMap::PrepareMapObjDefs(addToScene);
    CMapObj::PrepareUpdate();
    CMap::PrepareMapDoodadDefs();
    AsyncFileReadPollHandler(nullptr, nullptr);
}

// VERIFY: 6cf7464a auto
// OFFSET: 0x007BD480
int32_t CMap::SafeOpen(const char* fileName, SFileImpl** file) {
    int32_t tries = 10;

    while (!SFileOpenFile(fileName, file)) {
        DebugLog("CMap::SafeOpen() failed %s\n", fileName);

        if (!--tries) {
            SErrDisplayAppFatal("CMap::SafeOpen() failed %s", fileName);
            return 0;
        }
    }

    return 1;
}

// VERIFY: 3ace6e8d manual
// OFFSET: 0x007BD4D0
int32_t CMap::SafeRead(const char* fileName, SFileImpl* file, void* buffer, uint32_t size) {
    int32_t tries = 10;

    while (!SFileReadFile(file, buffer, size, nullptr, nullptr, 0)) {
        DebugLog("CMap::SafeRead() failed %s\n", fileName);

        if (!--tries) {
            DebugLog("CMap::SafeRead() failed %s", fileName);
            return 0;
        }
    }

    return 1;
}


// VERIFY: 69b638d4 auto
// OFFSET: 0x007BD540
void CMap::LoadTex() {
    char fileName[256];
    SStrCopy(fileName, CMap::wdtFilename, STORM_MAX_STR);
    char* ext = strrchr(fileName, '.');
    SStrCopy(ext, ".tex", STORM_MAX_STR);
    TextureLoadBlob(fileName);
}

// VERIFY: 4c68f31b manual
// OFFSET: 0x007BD8A0
void CMap::ValidateShaders() {
    CMap::enableVertexShaders = CWorld::enables2 & Enable_VertexShader;
    CMap::enablePixelShaders = (CWorld::enables & Enable_PixelShaders) != 0;
    CMap::enableSpecular = (CWorld::enables & Enable_Specular) && (CWorld::enables & Enable_PixelShaders);

    bool valid = false;
    bool flag = (CMap::header.m_flags >> 2) & 1;
    CMap::gTerrainPixelShadersValid = false;
    CMap::enableSpecularTerrain = false;

    if (CWorld::enables & Enable_PixelShaders) {
        CGxShader* specularShader = CMap::GetPixelShader(flag, true, false);
        CGxShader* shader = CMap::GetPixelShader(flag, false, false);
        valid = specularShader && specularShader->Valid() && shader && shader->Valid();
        CMap::gTerrainPixelShadersValid = valid;
    }

    if (CMap::enableSpecular && valid) {
        CGxShader* specularShader = CMap::GetPixelShader(flag, true, false);
        CGxShader* shader = CMap::GetPixelShader(flag, false, false);
        CMap::enableSpecularTerrain = specularShader && specularShader->Valid() && shader && shader->Valid();
    }

    if (CMap::enableVertexShaders) {
        CMap::enableTerrainShaderVertex = CMap::gTerrainPixelShadersValid && CMap::vertexShader_Terrain[0] && CMap::vertexShader_Terrain[0]->Valid();
    }

    CMap::enableChunkBatching = 0;

    if (CMap::gTerrainPixelShadersValid) {
        CMap::enableChunkBatching = CMap::enableTerrainShaderVertex != 0;
    }
}

// VERIFY: a8f937fc auto
// OFFSET: 0x007BD9F0
void CMap::Preload() {
    CMap::PurgeMaps();
    CMapObj::ClearCache(0);
    CMap::ClearAllFootprints();
    TextureGxCacheFlush();
    CMap::bPreload = 1;
    CMap::PrepareUpdate(false);

    if (!CMap::s_isStreamingMode) {
        AsyncFileReadWaitAll();
    }

    if (World::s_loadProgressCallback) {
        World::s_loadProgressCallback(1.0f, World::s_loadProgressParam);
    }

    CMap::bPreload = 0;
    CWorld::status.m_prepareAll = 1;
    CShadowCache::s_needsRebuild = 1;
}

// VERIFY: 4426576e auto
// OFFSET: 0x007BDD70
void CMapObj::InitDefGroup(CMapObj* mapObj, CMapObjGroup* mapObjGroup, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup) {
    mapObjDefGroup->m_flags = static_cast<MapObjFlag>(mapObjDefGroup->m_flags | MAPOBJ_FLAG_GROUP_INIT);
}

// VERIFY: 5b940caf manual
// OFFSET: 0x007BDE50
void CMap::CreateMapObjDefGroups(CMapObj* mapObj, CMapObjDef* mapObjDef) {
    mapObjDef->m_defGroups.SetCount(mapObj->m_groupInfoCount);

    for (int32_t i = 0; i < mapObj->m_groupInfoCount; i++) {
        CMapObjDefGroup* group = CMap::AllocMapObjDefGroup();
        CMapBaseObjLink* link = CMap::AllocBaseObjLink(group);
        link->m_ref = mapObjDef;
        mapObjDef->m_mapObjDefGroupLinkList.LinkToTail(link);
        mapObjDef->m_defGroups[i] = group;

        CAaBox box;
        mapObj->GetGroupBounds(&group->m_sphere, i);
        group->m_sphere.c *= mapObjDef->m_mat;
        mapObj->GetGroupBounds(&box, i);
        CWorldMath::TransformAABox(mapObjDef->m_mat, box, group->m_bbox);

        group->m_groupNum = i;
        group->m_ambientColor = mapObjDef->m_argbColor;
        group->m_flags = static_cast<MapObjFlag>(0);
        group->m_flags = static_cast<MapObjFlag>(group->m_flags | ((mapObj->GetGroupFlags(i) & 0x48) ? MAPOBJ_FLAG_EXTERIOR : MAPOBJ_FLAG_INTERIOR));
    }
}

// VERIFY: a7bf14e1 manual
// OFFSET: 0x007BECD0
CMapDoodadDef* CMap::CreateDoodadDef(const char* fileName, SMDoodadDef* doodadDef, C3Vector* center) {
    HASHKEY_DWORD key = { 0 };
    CMapDoodadDef* mapDoodadDef = CMap::doodadDefHash.Ptr(doodadDef->m_uniqueId, key);

    if (mapDoodadDef) {
        return mapDoodadDef;
    }

    mapDoodadDef = CMap::AllocDoodadDef();
    HASHKEY_DWORD insertKey = { 0 };
    CMap::doodadDefHash.Insert(mapDoodadDef, doodadDef->m_uniqueId, insertKey);
    CMap::doodadDefList.LinkToTail(mapDoodadDef);

    mapDoodadDef->m_position.x = -doodadDef->m_position.z;
    mapDoodadDef->m_position.y = -doodadDef->m_position.x;
    mapDoodadDef->m_position.z = doodadDef->m_position.y;
    mapDoodadDef->m_position.x = mapDoodadDef->m_position.x + center->x;
    mapDoodadDef->m_position.y = center->y + mapDoodadDef->m_position.y;
    mapDoodadDef->m_position.z = center->z + mapDoodadDef->m_position.z;

    mapDoodadDef->m_sphere.c = mapDoodadDef->m_position;
    mapDoodadDef->m_bbox.b = mapDoodadDef->m_position;
    mapDoodadDef->m_bbox.t = mapDoodadDef->m_position;
    mapDoodadDef->m_scale = doodadDef->m_scale * 0.0009765625f;
    mapDoodadDef->m_sphere.r = 0.0f;
    mapDoodadDef->m_flags = MAPOBJ_FLAG_UNPLACED;

    if (doodadDef->m_flags & 0x1) {
        mapDoodadDef->m_flags = static_cast<MapObjFlag>(MAPOBJ_FLAG_BIODOME | MAPOBJ_FLAG_UNPLACED);
    }

    mapDoodadDef->m_model = nullptr;

    float rotX = doodadDef->m_rotation.z * 0.017453292f;
    float rotY = doodadDef->m_rotation.x * 0.017453292f;
    float rotZ = 0.017453292f * doodadDef->m_rotation.y;

    mapDoodadDef->m_mat = C44Matrix();
    mapDoodadDef->m_mat.d0 = mapDoodadDef->m_position.x;
    mapDoodadDef->m_mat.d1 = mapDoodadDef->m_position.y;
    mapDoodadDef->m_mat.d2 = mapDoodadDef->m_position.z;
    mapDoodadDef->m_mat.RotateAroundZ(rotZ + 3.1415927f);
    mapDoodadDef->m_mat.RotateAroundY(rotY);
    mapDoodadDef->m_mat.RotateAroundX(rotX);
    mapDoodadDef->m_mat.Scale(mapDoodadDef->m_scale);
    mapDoodadDef->m_identity = C44Matrix();

    CM2Model* model = World::s_m2Scene->CreateModel(fileName, 0x20);
    mapDoodadDef->m_model = model;

    if (model) {
        model->m_fFlags |= 0x8000;
        model->m_worldTransform = mapDoodadDef->m_mat;
        mapDoodadDef->m_model->SetEventCallback(reinterpret_cast<void*>(CMapStaticEntity::ModelEventCallback), reinterpret_cast<uintptr_t>(mapDoodadDef));
        mapDoodadDef->m_model->m_lightingCallback = CMapStaticEntity::ModelLightingCallback;
        mapDoodadDef->m_model->m_lightingArg = mapDoodadDef;
        mapDoodadDef->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }

    return mapDoodadDef;
}

// VERIFY: 84c1a3e9 manual
// OFFSET: 0x007BEB40
CMapDoodadDef* CMap::CreateDoodadDef(const char* fileName, const C3Vector* position, float facing, int32_t waitForLoad) {
    int32_t id = CMap::uniqueId--;
    HASHKEY_DWORD key = { 0 };
    CMap::doodadDefHash.Ptr(id, key);
    CMapDoodadDef* doodadDef = CMap::AllocDoodadDef();
    HASHKEY_DWORD insertKey = { 0 };
    CMap::doodadDefHash.Insert(doodadDef, id, insertKey);
    doodadDef->m_position = *position;
    doodadDef->m_scale = 1.0f;
    doodadDef->m_sphere.r = 0.0f;
    doodadDef->m_sphere.c = doodadDef->m_position;
    doodadDef->m_bbox.b = doodadDef->m_position;
    doodadDef->m_bbox.t = doodadDef->m_position;
    doodadDef->m_mat = C44Matrix();
    doodadDef->m_mat.d0 = position->x;
    doodadDef->m_mat.d1 = position->y;
    doodadDef->m_mat.d2 = position->z;
    doodadDef->m_mat.RotateAroundZ(facing);
    doodadDef->m_identity = C44Matrix();
    doodadDef->m_flags = MAPOBJ_FLAG_UNPLACED;
    doodadDef->m_model = nullptr;
    CMapDoodadDef::CreateModel(fileName, doodadDef, waitForLoad, 0);

    return doodadDef;
}

// VERIFY: ea13d7f6 manual
// OFFSET: 0x007BEF40
CMapDoodadDef* CMap::CreateDoodadDef(uint32_t doodadRef, SMODoodadDef* doodadDef, char* name, uint32_t key, C44Matrix* mat, uint16_t doodadSet) {
    HASHKEY_DWORD hashKey = { key };
    CMapDoodadDef* mapDoodadDef = CMap::doodadDefHash.Ptr(doodadRef, hashKey);

    if (mapDoodadDef) {
        return mapDoodadDef;
    }

    mapDoodadDef = CMap::AllocDoodadDef();
    CMap::doodadDefHash.Insert(mapDoodadDef, doodadRef, hashKey);
    CMap::doodadDefList.LinkToTail(mapDoodadDef);

    mapDoodadDef->m_position = doodadDef->m_position;
    mapDoodadDef->m_position *= *mat;
    mapDoodadDef->m_scale = doodadDef->m_scale;
    mapDoodadDef->m_sphere.c = mapDoodadDef->m_position;
    mapDoodadDef->m_sphere.r = 0.0f;
    mapDoodadDef->m_bbox.b = mapDoodadDef->m_position;
    mapDoodadDef->m_bbox.t = mapDoodadDef->m_position;
    mapDoodadDef->m_doodadSet = doodadSet;
    mapDoodadDef->m_flags = MAPOBJ_FLAG_UNPLACED;

    if (doodadDef->m_flags & 0x1000000) {
        mapDoodadDef->m_flags = static_cast<MapObjFlag>(MAPOBJ_FLAG_PROJ_TEX | MAPOBJ_FLAG_UNPLACED);
    }

    mapDoodadDef->m_model = nullptr;
    mapDoodadDef->m_mat = C44Matrix();
    mapDoodadDef->m_mat.Translate(doodadDef->m_position);
    mapDoodadDef->m_mat.Rotate(doodadDef->m_orientation);
    mapDoodadDef->m_mat.Scale(mapDoodadDef->m_scale);
    mapDoodadDef->m_identity = mapDoodadDef->m_mat;
    mapDoodadDef->m_mat *= *mat;

    CImVector limit;
    *reinterpret_cast<uint32_t*>(&limit) = 112;
    mapDoodadDef->AdjustLightmap(&doodadDef->m_color, &mapDoodadDef->m_m2DiffuseColor, limit, &mapDoodadDef->m_m2AmbientColor, 96);

    CM2Model* model = World::s_m2Scene->CreateModel(name, 0x20);
    mapDoodadDef->m_model = model;

    if (model) {
        model->m_fFlags |= 0x8000;
        model->m_worldTransform = mapDoodadDef->m_mat;
        mapDoodadDef->m_model->SetEventCallback(reinterpret_cast<void*>(CMapStaticEntity::ModelEventCallback), reinterpret_cast<uintptr_t>(mapDoodadDef));
        mapDoodadDef->m_model->m_lightingCallback = CMapStaticEntity::ModelLightingCallback;
        mapDoodadDef->m_model->m_lightingArg = mapDoodadDef;
        mapDoodadDef->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }

    return mapDoodadDef;
}

// VERIFY: 140b192a manual
// OFFSET: 0x007BF460
CMapObjDef* CMap::CreateMapObjDef(char* fileName, SMMapObjDef* mapObjectDef, C3Vector* center, bool cached) {
    CMapObjDef* mapObjDef;

    if (cached && (mapObjDef = CMap::mapObjDefHash.Ptr(mapObjectDef->m_uniqueId, CMap::s_mapObjDefHashKey)) != nullptr) {
        return mapObjDef;
    }

    mapObjDef = CMap::AllocMapObjDef();

    if (cached) {
        CMap::mapObjDefHash.Insert(mapObjDef, mapObjectDef->m_uniqueId, CMap::s_mapObjDefHashKey);
    }

    mapObjDef->m_position.x = -mapObjectDef->m_position.z;
    mapObjDef->m_position.y = -mapObjectDef->m_position.x;
    mapObjDef->m_position.z = mapObjectDef->m_position.y;
    mapObjDef->m_position.x = center->x + mapObjDef->m_position.x;
    mapObjDef->m_position.y = mapObjDef->m_position.y + center->y;
    mapObjDef->m_position.z = center->z + mapObjDef->m_position.z;

    float rotX = mapObjectDef->m_rotation.z * 0.017453292f;
    float rotY = mapObjectDef->m_rotation.x * 0.017453292f;
    float rotZ = 0.017453292f * mapObjectDef->m_rotation.y;

    mapObjDef->m_flags = static_cast<MapObjFlag>(0);
    mapObjDef->m_nameId = mapObjectDef->m_nameId;
    mapObjDef->m_doodadSet = mapObjectDef->m_doodadSet;
    mapObjDef->m_nameSet = mapObjectDef->m_nameSet;
    mapObjDef->m_objectGuid = WOWGUID();
    mapObjDef->m_doodadSetOverrides[0] = 0;
    mapObjDef->m_doodadSetOverrides[1] = 0;
    mapObjDef->m_doodadSetOverrides[2] = 0;

    mapObjDef->m_mat.d3 = 1.0f;
    mapObjDef->m_mat.c2 = 1.0f;
    mapObjDef->m_mat.b1 = 1.0f;
    mapObjDef->m_mat.a0 = 1.0f;
    mapObjDef->m_mat.d2 = 0.0f;
    mapObjDef->m_mat.d1 = 0.0f;
    mapObjDef->m_mat.d0 = 0.0f;
    mapObjDef->m_mat.c3 = 0.0f;
    mapObjDef->m_mat.c1 = 0.0f;
    mapObjDef->m_mat.c0 = 0.0f;
    mapObjDef->m_mat.b3 = 0.0f;
    mapObjDef->m_mat.b2 = 0.0f;
    mapObjDef->m_mat.b0 = 0.0f;
    mapObjDef->m_mat.a3 = 0.0f;
    mapObjDef->m_mat.a2 = 0.0f;
    mapObjDef->m_mat.a1 = 0.0f;
    mapObjDef->m_mat.d0 = mapObjDef->m_position.x;
    mapObjDef->m_mat.d1 = mapObjDef->m_position.y;
    mapObjDef->m_mat.d2 = mapObjDef->m_position.z;
    mapObjDef->m_mat.RotateAroundZ(rotZ + 3.1415927f);
    mapObjDef->m_mat.RotateAroundY(rotY);
    mapObjDef->m_mat.RotateAroundX(rotX);
    mapObjDef->m_invMat = mapObjDef->m_mat.AffineInverse();

    mapObjDef->m_bbox.b.x = -mapObjectDef->m_extents.t.z;
    mapObjDef->m_bbox.b.y = -mapObjectDef->m_extents.t.x;
    mapObjDef->m_bbox.b.z = mapObjectDef->m_extents.b.y;
    mapObjDef->m_bbox.b.x = center->x + mapObjDef->m_bbox.b.x;
    mapObjDef->m_bbox.b.y = mapObjDef->m_bbox.b.y + center->y;
    mapObjDef->m_bbox.b.z = center->z + mapObjDef->m_bbox.b.z;
    mapObjDef->m_bbox.t.x = -mapObjectDef->m_extents.b.z;
    mapObjDef->m_bbox.t.y = -mapObjectDef->m_extents.b.x;
    mapObjDef->m_bbox.t.z = mapObjectDef->m_extents.t.y;
    mapObjDef->m_bbox.t.x = center->x + mapObjDef->m_bbox.t.x;
    mapObjDef->m_bbox.t.y = mapObjDef->m_bbox.t.y + center->y;
    mapObjDef->m_bbox.t.z = mapObjDef->m_bbox.t.z + center->z;

    mapObjDef->m_sphere.c.x = (mapObjDef->m_bbox.b.x + mapObjDef->m_bbox.t.x) * 0.5f;
    mapObjDef->m_sphere.c.y = (mapObjDef->m_bbox.t.y + mapObjDef->m_bbox.b.y) * 0.5f;
    mapObjDef->m_sphere.c.z = 0.5f * (mapObjDef->m_bbox.t.z + mapObjDef->m_bbox.b.z);
    float dz = mapObjDef->m_bbox.t.z - mapObjDef->m_sphere.c.z;
    float dy = mapObjDef->m_bbox.t.y - mapObjDef->m_sphere.c.y;
    float dx = mapObjDef->m_bbox.t.x - mapObjDef->m_sphere.c.x;
    mapObjDef->m_sphere.r = sqrtf(dx * dx + dy * dy + dz * dz);

    mapObjDef->m_lightsArray.m_count = 0;
    mapObjDef->m_owner = CMapObj::Create(fileName);

    return mapObjDef;
}

// VERIFY: 5ac717a0 manual
// OFFSET: 0x007BF120
CMapObjDef* CMap::CreateMapObjDef(const char* fileName, const C3Vector* position, float facing, int32_t waitLoad, uint32_t doodadSetCount, const int16_t* doodadSets, float* radius, uint32_t id) {
    uint32_t uniqueId = id;

    if (!id) {
        uniqueId = CMap::uniqueId--;
    }

    CMap::mapObjDefHash.Ptr(uniqueId, CMap::s_mapObjDefHashKey);
    CMapObjDef* mapObjDef = CMap::AllocMapObjDef();
    CMap::mapObjDefHash.Insert(mapObjDef, uniqueId, CMap::s_mapObjDefHashKey);
    CMapObj* mapObj = CMapObj::Create(const_cast<char*>(fileName));
    mapObjDef->m_owner = mapObj;

    if (waitLoad && !mapObj->m_isGroupLoaded) {
        mapObj->WaitLoad();
    }

    mapObjDef->m_flags = static_cast<MapObjFlag>(0);

    if (doodadSetCount) {
        mapObjDef->m_flags = MAPOBJ_FLAG_MAPOBJDEF;
    }

    mapObjDef->m_nameId = 0;
    mapObjDef->m_doodadSet = 0;
    mapObjDef->m_nameSet = 0;
    mapObjDef->m_position = *position;
    mapObjDef->m_objectGuid = WOWGUID();

    for (int32_t i = 0; i < 3; i++) {
        mapObjDef->m_doodadSetOverrides[i] = i < static_cast<int32_t>(doodadSetCount) ? doodadSets[i] : 0;
    }

    mapObjDef->m_mat = C44Matrix();
    mapObjDef->m_mat.d0 = position->x;
    mapObjDef->m_mat.d1 = position->y;
    mapObjDef->m_mat.d2 = position->z;
    mapObjDef->m_mat.RotateAroundZ(facing);
    mapObjDef->m_invMat = mapObjDef->m_mat.AffineInverse();

    if (mapObj->m_isGroupLoaded) {
        CAaBox bounds(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f));
        mapObj->GetBounds(mapObjDef->m_sphere);
        mapObjDef->m_sphere.c *= mapObjDef->m_mat;
        mapObj->GetBounds(&bounds);
        CWorldMath::TransformAABox(mapObjDef->m_mat, bounds, mapObjDef->m_bbox);
        mapObjDef->m_lightsArray.m_count = 0;
        return mapObjDef;
    }

    float sphereRadius;

    if (radius) {
        CAaBox extents;
        extents.b.x = *radius * -1.0f;
        extents.b.y = extents.b.x;
        extents.b.z = extents.b.x;
        extents.t.x = *radius;
        extents.t.y = *radius;
        extents.t.z = *radius;
        CWorldMath::TransformAABox(mapObjDef->m_mat, extents, mapObjDef->m_bbox);
        sphereRadius = *radius;
    } else {
        sphereRadius = 0.0f;
        mapObjDef->m_bbox.b = *position;
        mapObjDef->m_bbox.t = *position;
    }

    mapObjDef->m_sphere.c = *position;
    mapObjDef->m_sphere.r = sphereRadius;
    mapObjDef->m_lightsArray.m_count = 0;

    return mapObjDef;
}

// VERIFY: aade514a manual
// OFFSET: 0x007BF8B0
void CMap::LoadWdt() {
    SFileImpl* file = nullptr;
    SFileOpenFile(CMap::wdtFilename, &file);

    if (!file) {
        SErrDisplayAppFatal("CMap::LoadWdt() failed %s\n", CMap::wdtFilename);
    }

    SIffChunk iffChunk;
    iffChunk.m_token = 0;
    iffChunk.m_size = 0;

    SFileReadFile(file, &iffChunk, offsetof(SIffChunk, m_data), nullptr, nullptr, 0);
    SFileReadFile(file, &CMap::version, sizeof(CMap::version), nullptr, nullptr, 0);
    SFileReadFile(file, &iffChunk, offsetof(SIffChunk, m_data), nullptr, nullptr, 0);
    SFileReadFile(file, &CMap::header, sizeof(CMap::header), nullptr, nullptr, 0);
    SFileReadFile(file, &iffChunk, offsetof(SIffChunk, m_data), nullptr, nullptr, 0);
    SFileReadFile(file, CMap::m_areaInfo, sizeof(CMap::m_areaInfo), nullptr, nullptr, 0);

    if (CMap::header.m_flags & 1) {
        char fileName[256];
        SFileReadFile(file, &iffChunk, offsetof(SIffChunk, m_data), nullptr, nullptr, 0);
        SFileReadFile(file, fileName, iffChunk.m_size, nullptr, nullptr, 0);
        SFileReadFile(file, &iffChunk, offsetof(SIffChunk, m_data), nullptr, nullptr, 0);

        SMMapObjDef def;
        SFileReadFile(file, &def, sizeof(def), nullptr, nullptr, 0);
        def.m_uniqueId = CMap::uniqueId--;

        CMapObjDef* mapObjDef = CMap::mapObjDefHash.Ptr(def.m_uniqueId, CMap::s_mapObjDefHashKey);

        if (!mapObjDef) {
            mapObjDef = CMap::AllocMapObjDef();
            CMap::mapObjDefHash.Insert(mapObjDef, def.m_uniqueId, CMap::s_mapObjDefHashKey);

            mapObjDef->m_position.x = -def.m_position.z;
            mapObjDef->m_position.y = -def.m_position.x;
            mapObjDef->m_position.z = def.m_position.y;

            float rotX = def.m_rotation.z * 0.017453292f;
            float rotY = def.m_rotation.x * 0.017453292f;
            float rotZ = 0.017453292f * def.m_rotation.y;

            mapObjDef->m_flags = static_cast<MapObjFlag>(0);
            mapObjDef->m_nameId = def.m_nameId;
            mapObjDef->m_doodadSet = def.m_doodadSet;
            mapObjDef->m_nameSet = def.m_nameSet;
            mapObjDef->m_objectGuid = WOWGUID();
            mapObjDef->m_doodadSetOverrides[0] = 0;
            mapObjDef->m_doodadSetOverrides[1] = 0;
            mapObjDef->m_doodadSetOverrides[2] = 0;

            mapObjDef->m_mat.d3 = 1.0f;
            mapObjDef->m_mat.c2 = 1.0f;
            mapObjDef->m_mat.b1 = 1.0f;
            mapObjDef->m_mat.a0 = 1.0f;
            mapObjDef->m_mat.d2 = 0.0f;
            mapObjDef->m_mat.d1 = 0.0f;
            mapObjDef->m_mat.d0 = 0.0f;
            mapObjDef->m_mat.c3 = 0.0f;
            mapObjDef->m_mat.c1 = 0.0f;
            mapObjDef->m_mat.c0 = 0.0f;
            mapObjDef->m_mat.b3 = 0.0f;
            mapObjDef->m_mat.b2 = 0.0f;
            mapObjDef->m_mat.b0 = 0.0f;
            mapObjDef->m_mat.a3 = 0.0f;
            mapObjDef->m_mat.a2 = 0.0f;
            mapObjDef->m_mat.a1 = 0.0f;
            mapObjDef->m_mat.d0 = mapObjDef->m_position.x;
            mapObjDef->m_mat.d1 = mapObjDef->m_position.y;
            mapObjDef->m_mat.d2 = mapObjDef->m_position.z;
            mapObjDef->m_mat.RotateAroundZ(rotZ + 3.1415927f);
            mapObjDef->m_mat.RotateAroundY(rotY);
            mapObjDef->m_mat.RotateAroundX(rotX);
            mapObjDef->m_invMat = mapObjDef->m_mat.AffineInverse();

            mapObjDef->m_bbox.b.x = -def.m_extents.t.z;
            mapObjDef->m_bbox.b.y = -def.m_extents.t.x;
            mapObjDef->m_bbox.b.z = def.m_extents.b.y;
            mapObjDef->m_bbox.t.x = -def.m_extents.b.z;
            mapObjDef->m_bbox.t.y = -def.m_extents.b.x;
            mapObjDef->m_bbox.t.z = def.m_extents.t.y;

            mapObjDef->m_sphere.c.x = (mapObjDef->m_bbox.b.x + mapObjDef->m_bbox.t.x) * 0.5f;
            mapObjDef->m_sphere.c.y = (mapObjDef->m_bbox.t.y + mapObjDef->m_bbox.b.y) * 0.5f;
            mapObjDef->m_sphere.c.z = 0.5f * (mapObjDef->m_bbox.t.z + mapObjDef->m_bbox.b.z);
            float dz = mapObjDef->m_bbox.t.z - mapObjDef->m_sphere.c.z;
            float dy = mapObjDef->m_bbox.t.y - mapObjDef->m_sphere.c.y;
            float dx = mapObjDef->m_bbox.t.x - mapObjDef->m_sphere.c.x;
            mapObjDef->m_sphere.r = sqrtf(dx * dx + dy * dy + dz * dz);

            mapObjDef->m_lightsArray.m_count = 0;
            mapObjDef->m_owner = CMapObj::Create(fileName);
        }

        CMapBaseObjLink* link = CMap::AllocBaseObjLink(mapObjDef);
        link->m_ref = nullptr;
        CMap::mapObjDefLinkList.LinkToTail(link);
        CMap::bDungeon = 1;
    }

    if (CMap::header.m_flags & 2) {
        CMapRenderChunk::SetVertexFormat(GxVBF_PNC);
    } else {
        CMapRenderChunk::SetVertexFormat(GxVBF_PN);
    }

    CMap::ValidateShaders();
    SFileCloseFile(file);
}

// VERIFY: 99e76495 manual
// OFFSET: 0x007BFCE0
void CMap::Load(const char* mapName, int32_t zoneID) {
    CMap::s_enableTriangleStrips = false;

    uint32_t length = SStrCopy(CMap::mapPath, "World\\Maps\\", STORM_MAX_STR);
    SStrCopy(&CMap::mapPath[length], mapName, STORM_MAX_STR);
    SStrCopy(CMap::mapName, mapName, STORM_MAX_STR);
    SStrPrintf(CMap::wdtFilename, sizeof(CMap::wdtFilename), "%s\\%s.wdt", CMap::mapPath, CMap::mapName);

    CMap::s_mapLight = CMap::CreateLight(1, 0);
    CMap::s_mapLight->m_light.SetLightType(M2LIGHT_DIRECT);
    CMap::EnableLight(CMap::s_mapLight);
    CMap::UpdateLight(CMap::s_mapLight);
    CMap::PurgeMaps();
    CMapObj::ClearCache(1);
    CMap::ClearAllFootprints();

    s_mapId = zoneID;
    CMap::bActive = 1;
    CMap::bDungeon = 0;
    CMap::bPreload = 1;

    int32_t streaming = SFile::IsStreamingMode();
    CMap::s_isStreamingMode = SFile::IsTrial() | streaming;
    bool wait = CMap::s_isStreamingMode == 0;

    CMap::s_lowDetail.Load(CMap::mapPath, CMap::mapName);
    CMap::LoadWdt();
    CMap::LoadTex();
    DayNight::LoadMap(zoneID);
    CMap::PrepareUpdate(false);

    if (wait) {
        AsyncFileReadWaitAll();
    }

    if (World::s_loadProgressCallback) {
        World::s_loadProgressCallback(1.0f, World::s_loadProgressParam);
    }

    CMap::bPreload = 0;
    World::s_loadProgressCallback = nullptr;
    CWorld::status.m_prepareAll = 1;
}

// VERIFY: 512418e2 auto
// OFFSET: 0x007BFF20
CMapObj* CMap::AllocMapObj() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::mapObjHeap, &memHandle, &object, false)) {
        return nullptr;
    }

    CMapObj* mapObj = new (object) CMapObj();
    mapObj->m_objectIndex = memHandle;

    return mapObj;
}

// VERIFY: a3468102 manual
// OFFSET: 0x007BFF70
void CMap::FreeMapObj(CMapObj* mapObj) {
    mapObj->m_asyncLoadLink.Unlink();
    mapObj->~CMapObj();
    ObjectFree(*CMap::mapObjHeap, mapObj->m_objectIndex);
}

// VERIFY: 3e380871 auto
// OFFSET: 0x007BFFE0
CMapObjGroup* CMap::AllocMapObjGroup() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::mapObjGroupHeap, &memHandle, &object, false)) {
        return nullptr;
    }

    CMapObjGroup* mapObjGroup = new (object) CMapObjGroup();
    mapObjGroup->m_objectIndex = memHandle;

    return mapObjGroup;
}

// VERIFY: 13962642 manual
// OFFSET: 0x007C0030
void CMap::FreeMapObjGroup(CMapObjGroup* mapObjGroup) {
    mapObjGroup->m_asyncLoadLink.Unlink();
    mapObjGroup->~CMapObjGroup();
    ObjectFree(*CMap::mapObjGroupHeap, mapObjGroup->m_objectIndex);
}

// VERIFY: 8cfa8101 manual
// OFFSET: 0x007C00A0
void CMap::FreeArea(CMapArea* area) {
    area->m_lameAssLink.Unlink();
    area->~CMapArea();
    ObjectFree(*CMap::areaHeap, area->m_objectIndex);
}

// VERIFY: 53440967 manual
// OFFSET: 0x007C0110
void CMap::FreeAreaMed(CMapAreaMed* areaMed) {
    areaMed->m_lameAssLink.Unlink();
    areaMed->~CMapAreaMed();
    ObjectFree(*CMap::areaMedHeap, areaMed->m_objectIndex);
}

// VERIFY: 27b1c9a8 manual
// OFFSET: 0x007C0180
void CMap::FreeChunk(CMapChunk* chunk) {
    chunk->m_lameAssLink.Unlink();
    chunk->~CMapChunk();
    ObjectFree(*CMap::chunkHeap, chunk->m_objectIndex);
}

// VERIFY: d1b91d2a auto
// OFFSET: 0x007C01F0
CMapDoodadDef* CMap::AllocDoodadDef() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::doodadDefHeap, &memHandle, &object, false)) {
        return nullptr;
    }

    CMapDoodadDef* doodadDef = new (object) CMapDoodadDef();
    doodadDef->m_objectIndex = memHandle;

    return doodadDef;
}

// VERIFY: ef01a812 manual
// OFFSET: 0x007C0240
void CMap::FreeDoodadDef(CMapDoodadDef* doodadDef) {
    SI2::StopSound(&doodadDef->m_soundKitObject, 0, -1.0f, 1);
    doodadDef->m_lameAssLink.Unlink();
    CMap::doodadDefHash.Unlink(doodadDef);
    uint32_t* heap = CMap::doodadDefHeap;
    doodadDef->~CMapDoodadDef();
    ObjectFree(*heap, doodadDef->m_objectIndex);
}

// VERIFY: d7c82155 manual
// OFFSET: 0x007C02D0
void CMap::FreeEntity(CMapEntity* entity) {
    entity->m_lameAssLink.Unlink();
    entity->~CMapEntity();
    ObjectFree(*CMap::entityHeap, entity->m_objectIndex);
}

// VERIFY: 56cb0afe auto
// OFFSET: 0x007C0340
void CMap::FreeLight(CMapLight* light) {
    light->~CMapLight();
    ObjectFree(*CMap::lightHeap, light->m_objectIndex);
}

// VERIFY: eeef95f1 manual
// OFFSET: 0x007C0370
void CMap::FreeMapObjDefGroup(CMapObjDefGroup* mapObjDefGroup) {
    mapObjDefGroup->m_lameAssLink.Unlink();
    mapObjDefGroup->~CMapObjDefGroup();
    ObjectFree(*CMap::mapObjDefGroupHeap, mapObjDefGroup->m_objectIndex);
}

// VERIFY: fe23a5b5 auto
// OFFSET: 0x007C03E0
CMapObjDef* CMap::AllocMapObjDef() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::mapObjDefHeap, &memHandle, &object, false)) {
        return nullptr;
    }

    CMapObjDef* mapObjDef = new (object) CMapObjDef();
    mapObjDef->m_objectIndex = memHandle;

    return mapObjDef;
}

// VERIFY: eb574028 manual
// OFFSET: 0x007C0430
void CMap::FreeMapObjDef(CMapObjDef* mapObjDef) {
    mapObjDef->m_lameAssLink.Unlink();
    CMap::mapObjDefHash.Unlink(mapObjDef);
    uint32_t* heap = CMap::mapObjDefHeap;
    mapObjDef->~CMapObjDef();
    ObjectFree(*heap, mapObjDef->m_objectIndex);
}

// VERIFY: 37cad926 manual
// OFFSET: 0x007C04A0
void CMap::FreeChunkLiquid(CChunkLiquid* liquid) {
    liquid->m_link.Unlink();
    liquid->~CChunkLiquid();
    ObjectFree(*CMap::chunkLiquidHeap, liquid->m_objectIndex);
}

// VERIFY: 03529b4c manual
// OFFSET: 0x007C0500
CMapRenderChunk* CMap::AllocRenderChunk() {
    CMapRenderChunk* renderChunk = CMap::s_mapRenderChunkFreeList.Head();

    if (!renderChunk) {
        renderChunk = CMap::s_mapRenderChunkFreeList.NewNode(2, 0, 0);
        renderChunk->m_renderChunkLink.Unlink();
        return renderChunk;
    }

    renderChunk->m_renderChunkLink.Unlink();
    new (renderChunk) CMapRenderChunk();

    return renderChunk;
}

// VERIFY: 8d425b50 manual
// OFFSET: 0x007C0610
void CMap::FreeRenderChunk(CMapRenderChunk* renderChunk) {
    renderChunk->m_renderChunkLink.Unlink();
    renderChunk->~CMapRenderChunk();
    CMap::s_mapRenderChunkFreeList.LinkToHead(renderChunk);
}

// VERIFY: d11dd1c9 auto
// OFFSET: 0x007C0670
CMapEntity* CMap::AllocEntity(int32_t linkToHead) {
    uint32_t memHandle;
    void* object = nullptr;
    CMapEntity* entity;

    if (ObjectAlloc(*CMap::entityHeap, &memHandle, &object, false)) {
        entity = new (object) CMapEntity();
        entity->m_objectIndex = memHandle;
    } else {
        entity = nullptr;
    }

    if (linkToHead) {
        CMap::entityList.LinkToHead(entity);
    } else {
        CMap::entityList.LinkToTail(entity);
    }

    return entity;
}

// VERIFY: 3adb704b manual
// OFFSET: 0x007C0750
CMapBaseObjLink* CMap::AllocBaseObjLink(CMapBaseObj* baseObj) {
    uint32_t memHandle;
    void* object = nullptr;
    CMapBaseObjLink* link;

    if (ObjectAlloc(*CMap::baseObjLinkHeap, &memHandle, &object, false)) {
        link = new (object) CMapBaseObjLink;
        link->m_objectIndex = memHandle;
    } else {
        link = nullptr;
    }

    baseObj->m_refCount++;
    link->m_owner = baseObj;
    link->m_ref = nullptr;
    baseObj->m_parentLinkList.LinkToTail(link);

    return link;
}

// VERIFY: a8354093 auto
// OFFSET: 0x007C07C0
CMapArea* CMap::AllocArea() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::areaHeap, &memHandle, &object, false)) {
        CMap::s_allocatedAreas.LinkToTail(nullptr);
        return nullptr;
    }

    CMapArea* area = new (object) CMapArea();
    area->m_objectIndex = memHandle;
    CMap::s_allocatedAreas.LinkToTail(area);

    return area;
}

// VERIFY: 2acaa66b auto
// OFFSET: 0x007C0830
CMapChunk* CMap::AllocMapChunk() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::chunkHeap, &memHandle, &object, false)) {
        CMap::s_mapChunkList.LinkToTail(nullptr);
        return nullptr;
    }

    CMapChunk* chunk = new (object) CMapChunk();
    chunk->m_objectIndex = memHandle;
    CMap::s_mapChunkList.LinkToTail(chunk);

    return chunk;
}

// VERIFY: 430261e3 auto
// OFFSET: 0x007C08A0
CMapLight* CMap::AllocLight() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::lightHeap, &memHandle, &object, false)) {
        CMap::lightList.LinkToTail(nullptr);
        return nullptr;
    }

    CMapLight* light = new (object) CMapLight();
    light->m_objectIndex = memHandle;
    CMap::lightList.LinkToTail(light);

    return light;
}

// VERIFY: 7128fd21 auto
// OFFSET: 0x007C0910
CMapObjDefGroup* CMap::AllocMapObjDefGroup() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::mapObjDefGroupHeap, &memHandle, &object, false)) {
        CMap::mapObjDefGroupList.LinkToTail(nullptr);
        return nullptr;
    }

    CMapObjDefGroup* group = new (object) CMapObjDefGroup();
    group->m_objectIndex = memHandle;
    CMap::mapObjDefGroupList.LinkToTail(group);

    return group;
}

// VERIFY: ecff775f auto
// OFFSET: 0x007C0980
CChunkLiquid* CMap::AllocChunkLiquid() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CMap::chunkLiquidHeap, &memHandle, &object, false)) {
        CMap::s_chunkLiquidList.LinkToTail(nullptr);
        return nullptr;
    }

    CChunkLiquid* liquid = new (object) CChunkLiquid();
    liquid->m_objectIndex = memHandle;
    CMap::s_chunkLiquidList.LinkToTail(liquid);

    return liquid;
}

// VERIFY: c9309208 manual
// OFFSET: 0x007C09F0
void CMap::FreeBaseObjLink(CMapBaseObjLink* link) {
    link->m_ownerLink.Unlink();
    link->m_refLink.Unlink();
    link->m_owner->m_refCount--;
    link->m_ref = nullptr;
    link->m_owner = nullptr;
    link->~CMapBaseObjLink();
    ObjectFree(*CMap::baseObjLinkHeap, link->m_objectIndex);
}

// VERIFY: f22f4d25 auto
// OFFSET: 0x007C0A90
CMapAreaLow* CMap::AllocAreaLow() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(CMap::areaLowHeap->m_heapId, &memHandle, &object, false)) {
        return nullptr;
    }

    CMapAreaLow* areaLow = new (object) CMapAreaLow();
    areaLow->m_objectIndex = memHandle;

    return areaLow;
}

// VERIFY: 41bd42d1 manual
// OFFSET: 0x007C0AE0
void CMapAreaLowHeap::Delete(CMapAreaLow* areaLow) {
    areaLow->m_horizonLink.Unlink();
    ObjectFree(this->m_heapId, areaLow->m_objectIndex);
}

// VERIFY: 51f512f8 auto
// OFFSET: 0x007C0C60
void CMap::FreeAreaLow(CMapAreaLow* areaLow) {
    CMap::areaLowHeap->Delete(areaLow);
}

// VERIFY: 9da97e61 auto
// OFFSET: 0x007C15F0
void CMap::ClassifyStaticEntityLink(CMapStaticEntity* entity, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup, uint32_t faceInfo, C3Vector* pos) {
    CMapObjGroup* group = mapObjDef->m_owner->GetGroup(mapObjDefGroup->m_groupNum, 0);

    if (!group) {
        entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_EXTERIOR);
    } else if (group->m_flags & 0x8) {
        entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_EXTERIOR);
    } else if (group->m_flags & 0x40) {
        entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_EXTERIOR);
    } else {
        entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_INTERIOR);
    }

    if (entity->m_flags & MAPOBJ_FLAG_INTERIOR) {
        entity->QueryInteriorLighting(mapObjDef, mapObjDefGroup->m_groupNum, reinterpret_cast<uint16_t*>(&faceInfo), pos);
    }
}

// VERIFY: d84c44fe manual
// OFFSET: 0x007C1660
bool CMap::LinkStaticEntityGetChunk(C3Vector& pos, float* outHeight, CMapChunk** outChunk) {
    *outChunk = nullptr;

    int32_t cellX = CMath::fint_(-(pos.y - 17066.666f) * 0.24f - 0.5f);
    int32_t cellY = CMath::fint_(0.24f * -(pos.x - 17066.666f) - 0.5f);

    CMapArea* area = CMap::m_areaTable[64 * ((cellY >> 7) & 0x3F) + ((cellX >> 7) & 0x3F)];

    if (!area || area->m_asyncObject) {
        return false;
    }

    CMapChunk* chunk = area->m_mapChunks[((cellX >> 3) & 0xF) + 16 * ((cellY >> 3) & 0xF)];

    if (!chunk) {
        return false;
    }

    *outChunk = chunk;

    return CMap::GetHeightTerrain(chunk, pos, cellX, cellY, outHeight);
}

// VERIFY: abd054b1 manual
// OFFSET: 0x007C1DC0
void CMap::LinkIntersectMapObjDefGroup(CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup, C3Vector& start, C3Vector& end, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit) {
    CMapObj* mapObj = mapObjDef->m_owner;
    CMapObjGroup* group = mapObj->GetGroup(mapObjDefGroup->m_groupNum, 0);

    if (!group) {
        return;
    }

    int32_t interiorFace = -1;
    int32_t groundFace = -1;

    C3Segment seg;
    seg.b = start;
    seg.t = end;

    int16_t interior = !(group->m_flags & 0x8);

    if (group->GetFacesForLinking(seg, &interiorHit->t, &interiorFace, &groundHit->t, &groundFace)) {
        if (interiorFace != -1) {
            interiorHit->def = mapObjDef;
            interiorHit->group = mapObjDefGroup;
            interiorHit->faceIndex = interiorFace;
            interiorHit->interior = interior;
        }

        if (groundFace != -1) {
            groundHit->faceIndex = groundFace;
            groundHit->def = mapObjDef;
            groundHit->group = mapObjDefGroup;
            groundHit->interior = interior;
        }
    }

    if (!interior) {
        return;
    }

    float t = 1.05f;
    int32_t groups[2];

    if (mapObj->VectorIntersectPortals(mapObjDefGroup->m_groupNum, seg, &t, groups) && t - interiorHit->t < 0.0001f) {
        interiorHit->def = mapObjDef;
        CMapObjDefGroup* portalGroup = mapObjDef->m_defGroups[groups[0]];
        interiorHit->t = t;
        interiorHit->group = portalGroup;
        interiorHit->faceIndex = -1;
        interiorHit->interior = !(mapObj->GetGroup(portalGroup->m_groupNum, 0)->m_flags & 0x8);
    }
}

// VERIFY: e5163286 manual
// OFFSET: 0x007C1F20
void CMap::FreeBaseObjLinksInBounds(CAaBox* box) {
    for (CMapEntity* entity = CMap::entityList.Head(); entity; entity = CMap::entityList.Next(entity)) {
        if (box->t.x >= entity->m_bbox.b.x && box->t.y >= entity->m_bbox.b.y && box->t.z >= entity->m_bbox.b.z && box->b.x <= entity->m_bbox.t.x && box->b.y <= entity->m_bbox.t.y && box->b.z <= entity->m_bbox.t.z) {
            CMapBaseObjLink* link = entity->m_parentLinkList.Head();

            while (link) {
                CMapBaseObjLink* next = entity->m_parentLinkList.Next(link);
                CMap::FreeBaseObjLink(link);
                link = next;
            }
        }
    }
}

// VERIFY: 86bfa752 auto
// OFFSET: 0x007C1FF0
void CMap::LinkObjectToMapObjDefGroup(CMapStaticEntity* object, CMapObjDefGroup* mapObjDefGroup) {
    CMapBaseObjLink* link = CMap::AllocBaseObjLink(object);
    link->m_ref = mapObjDefGroup;

    if (object->m_type & 0x20) {
        if (object->m_staticFlags & 0x2) {
            mapObjDefGroup->m_entityLinkList.LinkToHead(link);
            return;
        }

        mapObjDefGroup->m_entityLinkList.LinkToTail(link);
    } else if (object->m_type & 0x40) {
        mapObjDefGroup->m_doodadDefLinkList.LinkToTail(link);
    }
}

// VERIFY: d4db675a auto
// OFFSET: 0x007C2040
bool CMap::LinkObjectToMapExterior(CMapStaticEntity* object) {
    bool linked = false;

    if (CMap::bDungeon) {
        return false;
    }

    float maxX = -(object->m_bbox.t.x - 17066.666f);
    float maxY = -(object->m_bbox.t.y - 17066.666f);
    float minX = -(object->m_bbox.b.x - 17066.666f);

    int32_t lastCol = CMath::fint_(-(object->m_bbox.b.y - 17066.666f) * 0.03f - 0.5f);
    int32_t lastRow = CMath::fint_(minX * 0.03f - 0.5f);
    int32_t firstCol = CMath::fint_(maxY * 0.03f - 0.5f);
    int32_t firstRow = CMath::fint_(maxX * 0.03f - 0.5f);

    for (int32_t row = firstRow; row <= lastRow; row++) {
        for (int32_t col = firstCol; col <= lastCol; col++) {
            CMapArea* area = CMap::m_areaTable[(((row >> 4) & 0x3F) << 6) + ((col >> 4) & 0x3F)];

            if (!area || area->m_asyncObject) {
                continue;
            }

            CMapChunk* chunk = area->m_mapChunks[(col & 0xF) + 16 * (row & 0xF)];

            if (!chunk || chunk->m_bbox.b.z > object->m_bbox.t.z) {
                continue;
            }

            CMapBaseObjLink* link = CMap::AllocBaseObjLink(object);
            link->m_ref = chunk;

            if (object->m_type & 0x20) {
                if (object->m_staticFlags & 0x2) {
                    chunk->m_entityLinkList.LinkToHead(link);
                } else {
                    chunk->m_entityLinkList.LinkToTail(link);
                }
            } else if (object->m_type & 0x40) {
                chunk->m_doodadDefLinkList.LinkToTail(link);
            }

            object->m_flags = static_cast<MapObjFlag>(object->m_flags | MAPOBJ_FLAG_EXTERIOR);
            linked = true;
        }
    }

    return linked;
}

// VERIFY: 4d233b7e manual
// OFFSET: 0x007C25D0
bool CMap::LinkIntersectMapObjDef(CMapObjDef* mapObjDef, C3Vector& start, C3Vector& end, C3Vector& mid, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit) {
    CMapObj* mapObj = mapObjDef->m_owner;

    if (!mapObj) {
        return false;
    }

    C3Vector localStart = start * mapObjDef->m_invMat;
    C3Vector localEnd = end * mapObjDef->m_invMat;
    C3Vector localMid = mid * mapObjDef->m_invMat;

    uint32_t noDepthSort = mapObjDef->m_flags & MAPOBJ_FLAG_NO_DEPTH_SORT;

    for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
        CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(link->m_owner);
        uint32_t groupFlags = mapObj->GetGroupFlags(group->m_groupNum);

        if (!(groupFlags & 0x410080) && mapObj->TestGroupBounds(localStart, localEnd, group->m_groupNum) && ((!noDepthSort && (groupFlags & 0x8)) || mapObj->TestGroupBounds(localMid, group->m_groupNum))) {
            CMap::LinkIntersectMapObjDefGroup(mapObjDef, group, localStart, localEnd, &interiorHit[noDepthSort == 0], &groundHit[noDepthSort == 0]);
        }
    }

    return true;
}

// VERIFY: c04d5774 manual
// OFFSET: 0x007C2700
bool CMap::LinkIntersectMapObjDefs(C3Vector& start, C3Vector& end, C3Vector& mid, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit, CMapChunk* chunk) {
    groundHit[0].t = 1.05f;
    groundHit[0].def = nullptr;
    groundHit[1].t = 1.05f;
    groundHit[1].def = nullptr;
    interiorHit[0].t = 1.05f;
    interiorHit[0].def = nullptr;
    interiorHit[1].t = 1.05f;
    interiorHit[1].def = nullptr;

    if (chunk) {
        for (CMapBaseObjLink* link = chunk->m_mapObjDefLinkList.Head(); link; link = chunk->m_mapObjDefLinkList.Next(link)) {
            CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(link->m_owner);

            if (!(mapObjDef->m_flags & MAPOBJ_FLAG_DISABLED) && mapObjDef->TestAABox(start, end)) {
                CMap::LinkIntersectMapObjDef(mapObjDef, start, end, mid, interiorHit, groundHit);
            }
        }
    } else {
        for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.Next(mapObjDef)) {
            if (!(mapObjDef->m_flags & MAPOBJ_FLAG_DISABLED) && mapObjDef->TestAABox(start, end)) {
                CMap::LinkIntersectMapObjDef(mapObjDef, start, end, mid, interiorHit, groundHit);
            }
        }
    }

    if (!interiorHit[0].def && groundHit[0].def) {
        interiorHit[0] = groundHit[0];
    }

    if (!interiorHit[1].def && groundHit[1].def) {
        interiorHit[1] = groundHit[1];
    }

    if (!interiorHit[0].def) {
        if (!interiorHit[1].def) {
            return false;
        }

        interiorHit[0] = interiorHit[1];
        groundHit[0] = groundHit[1];
        interiorHit[1].def = nullptr;
        groundHit[1].def = nullptr;
    }

    if (!groundHit[0].def) {
        groundHit[0] = interiorHit[0];
        groundHit[0].faceIndex = -1;
    }

    if (!groundHit[1].def) {
        groundHit[1] = interiorHit[1];
        groundHit[1].faceIndex = -1;
    }

    return true;
}

// VERIFY: fbe3b25a manual
// OFFSET: 0x007C28F0
void CMap::LinkStaticEntity(CMapStaticEntity* entity, C3Vector& top, C3Vector& bottom, C3Vector& mid, int32_t& outInterior, int32_t& outHit, MapObjIntersectData* interiorHit, MapObjIntersectData* groundHit) {
    float height = 100000.0f;
    float terrainT = 2.0f;

    interiorHit[0].def = nullptr;
    groundHit[0].def = nullptr;
    interiorHit[1].def = nullptr;
    groundHit[1].def = nullptr;

    CMapChunk* chunk = nullptr;
    bool terrain = false;

    if (!CMap::bDungeon) {
        terrain = CMap::LinkStaticEntityGetChunk(top, &height, &chunk);
        terrainT = (top.z - height) * 0.001f;

        if (terrainT < 0.0f) {
            terrain = false;
        }
    }

    bool hit;

    if (entity->m_flags & MAPOBJ_FLAG_NO_MAPOBJ_LINK) {
        hit = false;
    } else {
        hit = CMap::LinkIntersectMapObjDefs(top, bottom, mid, interiorHit, groundHit, nullptr);
    }

    if (!terrain && !hit) {
        top = entity->m_vec2;
        bottom = entity->m_vec2;
        bottom.z = bottom.z + 1000.0f;
        CMap::LinkIntersectMapObjDefs(top, bottom, mid, interiorHit, groundHit, nullptr);
    }

    if (entity->m_staticFlags & 0x2000) {
        interiorHit[1].def = nullptr;
        groundHit[1].def = nullptr;
    }

    if (terrain) {
        if (terrainT < interiorHit[0].t) {
            interiorHit[0].def = nullptr;
            groundHit[0].def = nullptr;
        }

        if (terrainT < interiorHit[1].t) {
            interiorHit[1].def = nullptr;
            groundHit[1].def = nullptr;
        }
    }

    outInterior = 0;
    outHit = 0;

    if (interiorHit[0].def) {
        outHit = 1;
    }

    if (interiorHit[1].def) {
        outHit = 1;
    }

    if (interiorHit[0].def) {
        outInterior = interiorHit[0].interior;
    } else if (interiorHit[1].def) {
        outInterior = interiorHit[1].interior;
    }
}

// VERIFY: c4018711 auto
// OFFSET: 0x007C2A70
void CMap::LinkStaticEntitySingle2(CMapStaticEntity* entity) {
    C3Vector top(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z + 0.1f);
    C3Vector bottom(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z - 1000.0f);

    int32_t index = 0;
    int32_t interior = 0;
    int32_t hit = 0;
    MapObjIntersectData interiorHit[2];
    MapObjIntersectData groundHit[2];

    CMap::LinkStaticEntity(entity, top, bottom, top, interior, hit, interiorHit, groundHit);

    uint32_t isEntity = entity->m_type & 0x20;
    uint32_t groundType = 0xFFFFFFFF;

    if (!hit) {
        CMap::LinkObjectToMapExterior(entity);

        if (isEntity) {
            static_cast<CMapEntity*>(entity)->m_groundType = CMap::QueryGroundTypeTerrain(&entity->m_position, &groundType) ? groundType : -1;
        }

        entity->m_flags = static_cast<MapObjFlag>((entity->m_flags & ~MAPOBJ_FLAG_IN_MAPOBJ) | MAPOBJ_FLAG_EXTERIOR);
        return;
    }

    if (!groundHit[0].def) {
        index = 1;
    }

    if (interiorHit[0].def) {
        CMap::LinkObjectToMapObjDefGroup(entity, interiorHit[0].group);
    }

    if (interiorHit[1].def) {
        CMap::LinkObjectToMapObjDefGroup(entity, interiorHit[1].group);
    }

    if (isEntity) {
        static_cast<CMapEntity*>(entity)->m_groundType = groundHit[index].def->GetGroundType(groundHit[index].group->m_groupNum, groundHit[index].faceIndex);
    }

    entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_IN_MAPOBJ);

    if (!interior) {
        entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_EXTERIOR);
        return;
    }

    entity->m_staticFlags |= 0x1;

    C3Vector pos(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z - (bottom.z - top.z) * groundHit[index].t);
    CMap::ClassifyStaticEntityLink(entity, groundHit[index].def, groundHit[index].group, groundHit[index].faceIndex, &pos);
}

// VERIFY: 68d8dacb manual
// OFFSET: 0x007C2BF0
void CMap::LinkStaticEntityMultiple2ToMapObjDefExterior(CMapStaticEntity* entity) {
    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.Next(mapObjDef)) {
        if (mapObjDef->m_flags & MAPOBJ_FLAG_DISABLED) {
            continue;
        }

        CMapObj* mapObj = mapObjDef->m_owner;

        if (!mapObj || !mapObj->m_isGroupLoaded) {
            continue;
        }

        CAaBox box;
        CWorldMath::TransformAABox(mapObjDef->m_invMat, entity->m_bbox, box);

        if (!mapObj->TestBounds(box)) {
            continue;
        }

        for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
            CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(link->m_owner);
            uint32_t groupFlags = mapObj->GetGroupFlags(group->m_groupNum);

            if (!(groupFlags & 0x410080) && (groupFlags & 0x8) && mapObj->TestGroupBounds(box, group->m_groupNum, 1)) {
                CMap::LinkObjectToMapObjDefGroup(entity, group);
            }
        }
    }
}

// VERIFY: 3091ac75 manual
// OFFSET: 0x007C2D30
void CMap::LinkStaticEntityMultiple2ToMapObjDefInterior(CMapStaticEntity* entity, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup) {
    CMapObj* mapObj = mapObjDef->m_owner;

    if (!mapObj || !mapObj->m_isGroupLoaded) {
        return;
    }

    CAaBox box;
    CWorldMath::TransformAABox(mapObjDef->m_invMat, entity->m_bbox, box);

    if (!mapObj->TestBounds(box)) {
        return;
    }

    CMap::LinkObjectToMapObjDefGroup(entity, mapObjDefGroup);

    for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
        CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(link->m_owner);

        if (group != mapObjDefGroup && !(mapObj->GetGroupFlags(group->m_groupNum) & 0x410088) && mapObj->TestGroupBounds(box, group->m_groupNum, 1)) {
            CMap::LinkObjectToMapObjDefGroup(entity, group);
        }
    }
}

// VERIFY: e348a76f auto
// OFFSET: 0x007C2E70
void CMap::LinkStaticEntityMultiple2(CMapStaticEntity* entity) {
    C3Vector top(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z + 4.0f);
    C3Vector bottom(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z - 1000.0f);
    C3Vector mid(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z + 0.15000001f);

    float boxTop = entity->m_bbox.t.z + 0.1f;

    if (boxTop < top.z) {
        top.z = boxTop;
    }

    int32_t interior = 0;
    int32_t hit;
    MapObjIntersectData interiorHit[2];
    MapObjIntersectData groundHit[2];

    CMap::LinkStaticEntity(entity, top, bottom, mid, interior, hit, interiorHit, groundHit);

    if (interior) {
        CMap::LinkStaticEntityMultiple2ToMapObjDefInterior(entity, interiorHit[0].def, interiorHit[0].group);
        entity->m_staticFlags |= 0x1;

        C3Vector pos(entity->m_vec2.x, entity->m_vec2.y, entity->m_vec2.z - (bottom.z - top.z) * groundHit[0].t);
        CMap::ClassifyStaticEntityLink(entity, groundHit[0].def, groundHit[0].group, groundHit[0].faceIndex, &pos);
    } else {
        CMap::LinkStaticEntityMultiple2ToMapObjDefExterior(entity);
        CMap::LinkObjectToMapExterior(entity);
        entity->m_flags = static_cast<MapObjFlag>(entity->m_flags | MAPOBJ_FLAG_EXTERIOR);
    }
}

// VERIFY: d35a4a66 manual
// OFFSET: 0x007C2F80
void CMap::LinkStaticEntity(CMapStaticEntity* entity) {
    CMapBaseObjLink* link = entity->m_parentLinkList.Head();

    while (link) {
        CMapBaseObjLink* next = entity->m_parentLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    if (entity->m_type & 0x20) {
        static_cast<CMapEntity*>(entity)->m_groundType = -1;
    }

    if (entity->m_staticFlags & 0x2000) {
        CMap::LinkStaticEntityMultiple2(entity);
    } else {
        CMap::LinkStaticEntitySingle2(entity);
    }
}

// VERIFY: 04f6c26e manual
// OFFSET: 0x007C3150
void CMap::PurgeMapObjDefGroup(CMapObjDefGroup* mapObjDefGroup) {
    for (CMapBaseObjLink* link = mapObjDefGroup->m_doodadDefLinkList.Head(); link;) {
        CMapBaseObjLink* next = mapObjDefGroup->m_doodadDefLinkList.Next(link);
        CMapDoodadDef* doodadDef = static_cast<CMapDoodadDef*>(link->m_owner);
        CMap::FreeBaseObjLink(link);

        if (!(doodadDef->m_flags & 0x200)) {
            CMapDoodadDef::Purge(doodadDef);
        }

        link = next;
    }

    for (CMapBaseObjLink* link = mapObjDefGroup->m_entityLinkList.Head(); link;) {
        CMapBaseObjLink* next = mapObjDefGroup->m_entityLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    for (CMapBaseObjLink* link = mapObjDefGroup->m_lightLinkList.Head(); link;) {
        CMapBaseObjLink* next = mapObjDefGroup->m_lightLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    for (CMapBaseObjLink* link = mapObjDefGroup->m_unkLinkList.Head(); link;) {
        CMapBaseObjLink* next = mapObjDefGroup->m_unkLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    CMap::FreeMapObjDefGroup(mapObjDefGroup);
}

// VERIFY: e0cd9ea4 manual
// OFFSET: 0x007C3250
void CMap::PurgeMapObjDef(CMapObjDef* mapObjDef) {
    if (mapObjDef->m_refCount) {
        return;
    }

    for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link;) {
        CMapBaseObjLink* next = mapObjDef->m_mapObjDefGroupLinkList.Next(link);
        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(link->m_owner);
        CMap::FreeBaseObjLink(link);
        CMap::PurgeMapObjDefGroup(mapObjDefGroup);
        link = next;
    }

    if (mapObjDef->m_defGroups.m_count == 0xFFFFFFFF) {
        mapObjDef->m_defGroups.m_dynamic.m_count = 0;
    } else {
        mapObjDef->m_defGroups.m_count = 0;
    }

    for (uint32_t i = 0; i < mapObjDef->m_lightsArray.m_count; i++) {
        if (mapObjDef->m_lightsArray.m_data[i]) {
            CMap::DestroyLight(mapObjDef->m_lightsArray.m_data[i]);
        }
    }

    mapObjDef->m_lightsArray.m_count = 0;
    CMapObj::Delete(mapObjDef->m_owner);
    mapObjDef->m_owner = nullptr;
    CMap::FreeMapObjDef(mapObjDef);

    DayNight::DNInfo* info = DayNight::GetInfo();

    if (info->m_interiorMapObjDef == mapObjDef) {
        info->m_interiorMapObjDef = nullptr;
        info->m_interiorFogBlend = 0.0f;
        info->m_interiorGroups.m_count = 0;
    }
}

// VERIFY: ad81d8da auto
// OFFSET: 0x007C3700
void CMap::PurgeArea(CMapArea* area) {
    CMap::m_areaTable[64 * area->m_index.y + area->m_index.x] = nullptr;
    area->Purge();
    CMap::FreeArea(area);
}

// VERIFY: e2dae8ec manual
// OFFSET: 0x007C3730
void CMap::PurgeMaps() {
    CMapBaseObjLink* link = CMap::s_unk_ADFBEC.Head();

    while (link) {
        CMapBaseObjLink* next = CMap::s_unk_ADFBEC.Next(link);
        CMapArea* area = static_cast<CMapArea*>(link->m_owner);
        CMap::FreeBaseObjLink(link);
        CMap::m_areaTable[64 * area->m_index.y + area->m_index.x] = nullptr;
        area->Purge();
        CMap::FreeArea(area);
        link = next;
    }

    for (uint32_t i = 0; i < 4096; i++) {
        if (CMap::unk[i]) {
            CMap::FreeAreaMed(CMap::unk[i]);
            CMap::unk[i] = nullptr;
        }
    }

    CMap::s_lowDetail.Purge();
}

// VERIFY: 7e85d0ed manual
// OFFSET: 0x007D59B0
bool CMap::LocateViewerMapObjs(C3Vector& start, C3Vector& end, float dist, CMapObjDef** outDefs, uint32_t* outGroups) {
    float dists[2] = { dist, dist };
    uint32_t slot = 0;

    outDefs[0] = nullptr;
    outDefs[1] = nullptr;
    outGroups[1] = 0xFFFF;
    outGroups[0] = 0xFFFF;
    outGroups[3] = 0xFFFF;
    outGroups[2] = 0xFFFF;

    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.m_fulllist.Next(mapObjDef)) {
        if (mapObjDef->m_flags & 0x20) {
            continue;
        }

        if (mapObjDef->m_flags & 0x400) {
            slot = 1;
        }

        CMapObj* mapObj = mapObjDef->m_owner;

        if (!mapObjDef->TestAABox(start, end) || !mapObj) {
            slot = 0;
            continue;
        }

        C3Vector localStart = start * mapObjDef->m_invMat;
        C3Vector localEnd = end * mapObjDef->m_invMat;

        if (!mapObj->TestBounds(localStart, localEnd)) {
            slot = 0;
            continue;
        }

        uint32_t exterior = 0;

        for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
            CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(link->m_owner);
            CMapObjGroup* group = mapObj->GetGroup(defGroup->m_groupNum, 0);

            if (!group || (group->m_flags & 0x410080) || !mapObj->TestGroupBounds(localStart, localEnd, defGroup->m_groupNum)) {
                continue;
            }

            C3Segment seg;
            seg.b = localStart;
            seg.t = localEnd;
            World::TriData::statusFlags = 0;
            World::TriData::nBatches = 0;
            World::TriData::faceIndexCursor = 0;
            World::TriData::indexCursor = 0;
            World::TriData::matrixPoolCount = 0;

            World::TriData triData;

            if (group->GetTris(seg, &dists[slot], 0, 0, &triData, mapObjDef)) {
                exterior = (group->m_flags >> 3) & 1;
                outDefs[slot] = mapObjDef;
                outGroups[2 * slot] = defGroup->m_groupNum;
                outGroups[2 * slot + 1] = 0xFFFF;
            }
        }

        float portalT = 1.05f;
        C3Segment seg;
        seg.b = localStart;
        seg.t = localEnd;
        uint32_t portalGroups[2];

        if (mapObj->VectorIntersectPortal(seg, &portalT, portalGroups, 0) && portalT - dists[slot] < 0.000099999997f) {
            exterior = (mapObj->GetGroupInfo(portalGroups[0])->m_flags >> 3) & 1;
            dists[slot] = portalT;
            outDefs[slot] = mapObjDef;
            outGroups[2 * slot] = portalGroups[0];

            if (mapObj->GetGroupInfo(portalGroups[1])->m_flags & 8) {
                outGroups[2 * slot + 1] = 0xFFFF;
            } else {
                outGroups[2 * slot + 1] = portalGroups[1];
            }
        }

        if (exterior) {
            outDefs[slot] = nullptr;
        }

        slot = 0;
    }

    if (outDefs[0]) {
        return true;
    }

    if (!outDefs[1]) {
        return false;
    }

    outDefs[0] = outDefs[1];
    outGroups[0] = outGroups[2];
    outGroups[1] = outGroups[3];
    outDefs[1] = nullptr;
    outGroups[2] = 0;
    outGroups[3] = 0;

    return true;
}

// VERIFY: 6db55b9e manual
// OFFSET: 0x007D9990
CTexture* CMap::LoadTexture(const char* fileName) {
    CStatus status;
    CTexture* texture = TextureCreate(fileName, CGxTexFlags(GxTex_LinearMipLinear, 1, 1, 0, 0, 0, 1), &status, 0);
    SysMsgAdd(&status);

    if (!texture) {
        DebugLog("Failed to load texture: %s\n", fileName);
    }

    return texture;
}

// VERIFY: fedf0bee auto
// OFFSET: 0x007D9A20
void CMap::LoadArea(CMapArea* area) {
    char fileName[256];
    SStrPrintf(fileName, sizeof(fileName), "%s\\%s_%d_%d.adt", CMap::mapPath, CMap::mapName, area->m_index.x, area->m_index.y);
    area->Load(fileName);
}

// VERIFY: a0b22da5 auto
// OFFSET: 0x007D9A70
CMapArea* CMap::PrepareArea(int32_t areaIndexX, int32_t areaIndexY) {
    CMapArea* area = CMap::AllocArea();
    CMapBaseObjLink* link = CMap::AllocBaseObjLink(area);
    CMap::s_unk_ADFBEC.LinkToTail(link);

    area->m_tileChunkIndex.x = 16 * areaIndexX;
    area->m_index.y = areaIndexY;
    area->m_flags = static_cast<MapObjFlag>(0);
    area->m_index.x = areaIndexX;
    area->m_tileChunkIndex.y = 16 * areaIndexY;

    area->m_topLeft2.x = 17066.666f - area->m_tileChunkIndex.y * 33.333332f;
    area->m_bounds.t.x = area->m_topLeft2.x;
    area->m_topLeft2.y = area->m_tileChunkIndex.x * -33.333332f + 17066.666f;
    area->m_bounds.t.y = area->m_topLeft2.y;
    area->m_bounds.b.x = area->m_topLeft2.x - 533.33331f;
    area->m_bounds.b.y = area->m_topLeft2.y - 533.33331f;

    CMap::m_areaTable[64 * areaIndexY + areaIndexX] = area;

    return area;
}

// VERIFY: e1002a60 manual
// OFFSET: 0x007D9B10
CMapLight::CMapLight() {
    this->m_sphere.r = 0.0f;
    this->m_type |= 0x80;
    this->unk_00CC = 0.0f;
    this->m_radius = 0.0f;
    this->unused_00D0 = 0;
    this->unused_00C4 = 0.0f;
}

// VERIFY: 0aba9189 manual
// OFFSET: 0x007D9BD0
CMapLight* CMap::CreateLight(uint8_t a1, uint8_t a2) {
    CMapLight* light = CMap::AllocLight();
    light->m_flags = static_cast<MapObjFlag>(0);
    light->m_position = C3Vector(0.0f, 0.0f, 0.0f);
    light->m_sphere.r = 0.0f;
    light->m_sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    light->m_bbox.b = C3Vector(0.0f, 0.0f, 0.0f);
    light->unk_00CC = 0.0f;
    light->m_radius = 0.0f;
    light->m_bbox.t = C3Vector(0.0f, 0.0f, 0.0f);
    light->unused_00C4 = 0.0f;
    light->unused_00D0 = a1;
    light->unk_00D1 = a2;

    return light;
}

// VERIFY: e4a742da manual
// OFFSET: 0x007D9C80
void CMap::UpdateLightBounds(CMapLight* light) {
    float radius = light->m_radius;

    if (light->unk_00CC == 0.0f) {
        radius = 5.0f;
    }

    light->m_radius = radius;

    C3Vector& pos = light->m_light.m_pos;
    light->m_position = pos;
    light->m_bbox.b = C3Vector(pos.x - radius, pos.y - radius, pos.z - radius);
    light->m_bbox.t = C3Vector(pos.x + radius, pos.y + radius, pos.z + radius);
    light->m_sphere.c = pos;
    light->m_sphere.r = radius;
}

// VERIFY: 66f3244f auto
// OFFSET: 0x007D9D50
void CMap::EnableLight(CMapLight* light) {
    light->m_flags = static_cast<MapObjFlag>(light->m_flags & ~0x20);
    light->m_light.SetVisible(1);
}

// VERIFY: c0ee5cb9 manual
// OFFSET: 0x007D9D70
void CMap::DestroyLight(CMapLight* light) {
    CMapBaseObjLink* link = light->m_parentLinkList.Head();

    while (link) {
        CMapBaseObj* ref = link->m_ref;
        CMapBaseObjLink* next = light->m_parentLinkList.Next(link);

        if (ref) {
            if (ref->m_type & 0x4) {
                static_cast<CMapChunk*>(ref)->UpdateLights();
            } else if (ref->m_type & 0x10) {
                static_cast<CMapObjDefGroup*>(ref)->UpdateLights();
            }
        }

        CMap::FreeBaseObjLink(link);
        link = next;
    }

    CMap::FreeLight(light);
}

// VERIFY: 8d2bb286 auto
// OFFSET: 0x007D9DE0
void CMap::LinkLightToChunks(CMapLight* light) {
    int32_t maxY = CMath::fint_(-(light->m_bbox.b.y - 17066.666f) * 0.03f - 0.5f);
    int32_t maxX = CMath::fint_(-(light->m_bbox.b.x - 17066.666f) * 0.03f - 0.5f);
    int32_t minY = CMath::fint_(-(light->m_bbox.t.y - 17066.666f) * 0.03f - 0.5f);
    int32_t minX = CMath::fint_(-(light->m_bbox.t.x - 17066.666f) * 0.03f - 0.5f);

    for (int32_t x = minX; x <= maxX; x++) {
        for (int32_t y = minY; y <= maxY; y++) {
            CMapArea* area = CMap::m_areaTable[(((x >> 4) & 0x3F) << 6) + ((y >> 4) & 0x3F)];

            if (!area || area->m_asyncObject) {
                continue;
            }

            CMapChunk* chunk = area->m_mapChunks[(y & 0xF) + 16 * (x & 0xF)];

            if (!chunk) {
                continue;
            }

            CMapBaseObjLink* link = CMap::AllocBaseObjLink(light);
            link->m_ref = chunk;
            chunk->m_lightLinkList.LinkToTail(link);
            chunk->UpdateLights();
        }
    }
}

// VERIFY: 75a5e857 manual
// OFFSET: 0x007D9F90
void CMap::LinkLightToMapObjDefs(CMapLight* light) {
    CAaBox box;

    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.Next(mapObjDef)) {
        if (mapObjDef->m_flags & 0x20) {
            continue;
        }

        if (light->m_bbox.t.x >= mapObjDef->m_bbox.b.x && light->m_bbox.t.y >= mapObjDef->m_bbox.b.y && light->m_bbox.t.z >= mapObjDef->m_bbox.b.z
            && light->m_bbox.b.x <= mapObjDef->m_bbox.t.x && light->m_bbox.b.y <= mapObjDef->m_bbox.t.y && light->m_bbox.b.z <= mapObjDef->m_bbox.t.z) {
            CMapObj* mapObj = mapObjDef->m_owner;

            if (mapObj && mapObj->TestBounds(box)) {
                for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
                    CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(link->m_owner);

                    if (!(group->m_flags & 0x2) && light->m_bbox.Intersects(group->m_bbox)) {
                        CMapBaseObjLink* lightLink = CMap::AllocBaseObjLink(light);
                        lightLink->m_ref = group;
                        group->m_lightLinkList.LinkToTail(lightLink);
                        group->UpdateLights();
                    }
                }
            }
        }
    }
}

// VERIFY: fe97de56 manual
// OFFSET: 0x007DA100
void CMap::UpdateLight(CMapLight* light) {
    if (!light->m_parentLinkList.Head() && !light->m_light.m_type) {
        CMapBaseObjLink* link = CMap::AllocBaseObjLink(light);
        link->m_ref = nullptr;
        CMap::s_globalLightList.LinkToTail(link);
    }

    if (CMap::bActive && light->m_light.m_type) {
        CMapBaseObjLink* link = light->m_parentLinkList.Head();

        while (link) {
            CMapBaseObjLink* next = light->m_parentLinkList.Next(link);
            CMap::FreeBaseObjLink(link);
            link = next;
        }

        CMap::UpdateLightBounds(light);
        CMap::LinkLightToMapObjDefs(light);
        CMap::LinkLightToChunks(light);
    }
}

// VERIFY: 34c22451 manual
// OFFSET: 0x007A50C0
bool CMap::GetDoodadDefFacets(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* linkList, CAaBox* box, World::FacetData* facets, uint32_t flags) {
    if (!(flags & 0xF0000F)) {
        return true;
    }

    for (CMapBaseObjLink* link = linkList->Head(); link; link = linkList->Next(link)) {
        CMapDoodadDef* def = static_cast<CMapDoodadDef*>(link->m_owner);

        if ((def->m_flags & 0x100) || def->m_unkCounter == CMap::s_queryTag || !def->m_model) {
            continue;
        }

        if (def->m_collisionGuid.m_guid ? !(flags & 0xF00000) : !(flags & 0xF)) {
            continue;
        }

        CAaBox bbox = def->m_bboxDoodadDef;

        if (!(def->m_flags & 0x80)) {
            def->m_staticFlags |= 0x10000;
            CAaBox modelBox = def->m_model->m_shared->m_boundingBox;
            CWorldMath::TransformAABox(def->m_mat, modelBox, bbox);
        }

        if (box->Intersects(bbox)) {
            if (def->m_flags & 0x80) {
                CMap::AppendMapObjFacets(def, box, facets);
            } else {
                World::AddAaBoxFacets(&bbox, facets);
            }
        }

        def->m_unkCounter = CMap::s_queryTag;
    }

    return true;
}

// VERIFY: e531c817 manual
// OFFSET: 0x007A4B80
void CMap::AppendDestructibleFacets(World::ObjectCollisionData* data, CAaBox* box, World::FacetData* facets, WOWGUID guid) {
    uint32_t first = facets->facets.Count();
    data->m_model->GetCollisionFacets(*box, data->m_mat, facets->facets);
    uint32_t count = facets->facets.Count();
    facets->facetIds.SetCount(count);

    for (uint32_t i = first; i < count; i++) {
        facets->facetIds.m_data[i] = guid.m_guid;
    }
}

// VERIFY: cee45fe5 manual
// OFFSET: 0x007A5240
void CMap::QueryDestructibleFacets(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, CAaBox* box, World::FacetData* facets, uint32_t flags) {
    if (!World::s_objectCollisionCallback || !(flags & 0xF00000)) {
        return;
    }

    for (CMapBaseObjLink* link = list->Head(); link; link = list->Next(link)) {
        CMapEntity* entity = static_cast<CMapEntity*>(link->m_owner);

        if (!(entity->m_staticFlags & 0x2)) {
            break;
        }

        if (entity->m_unkCounter == CMap::s_queryTag || (entity->m_staticFlags & 0x4)) {
            continue;
        }

        World::ObjectCollisionData data;
        WOWGUID guid = WOWGUID{ entity->m_funcParam64 };

        if (World::s_objectCollisionCallback(&guid, entity->m_funcParam32, &data, flags) && box->Intersects(data.m_bounds)) {
            CMap::AppendDestructibleFacets(&data, box, facets, guid);
        }

        entity->m_unkCounter = CMap::s_queryTag;
    }
}

// VERIFY: aa0cecc5 manual
// OFFSET: 0x007A3CF0
bool CMap::GetChunkLiquidFacets(CMapChunk* chunk, CAaBox* box, CiRect* rect, CChunkLiquid* liquid, World::FacetData* facets) {
    World::TriData::statusFlags = 0;
    World::TriData::nBatches = 0;
    World::TriData::faceIndexCursor = 0;
    World::TriData::indexCursor = 0;
    World::TriData::matrixPoolCount = 0;
    World::TriData triData;

    if (!liquid->GetTris(&triData, *box, *rect)) {
        return false;
    }

    return World::TriDataToFacetData(&triData, facets, WOWGUID{ 0 }) != 0;
}

// VERIFY: 92053872 manual
// OFFSET: 0x007A5A60
bool CMap::GetChunkFacets(int32_t chunkX, int32_t chunkY, CiRect* subRect, CAaBox* queryBox, CAaBox* box, World::FacetData* facets, uint32_t flags) {
    uint32_t first = facets->facets.Count();
    int32_t areaIndex = ((chunkX >> 4) & 0x3F) + (((chunkY >> 4) & 0x3F) << 6);
    CMapArea* area = CMap::m_areaTable[areaIndex];

    if (area && !area->m_asyncObject) {
        CMapChunk* chunk = area->m_mapChunks[16 * (chunkY & 0xF) + (chunkX & 0xF)];

        if (!chunk) {
            return false;
        }

        if (chunk->m_flags & MAPOBJ_FLAG_IMPASSABLE) {
            CMap::CreateImpassableFacets(chunk, box, facets);
        }

        CiRect rect;
        rect.minY = subRect->minY - 8 * chunkY;
        rect.minX = subRect->minX - 8 * chunkX;
        rect.maxY = subRect->maxY - 8 * chunkY;
        rect.maxX = subRect->maxX - 8 * chunkX;

        if (rect.minY < 0) {
            rect.minY = 0;
        }

        if (rect.minX < 0) {
            rect.minX = 0;
        }

        if (rect.maxY >= 8) {
            rect.maxY = 7;
        }

        if (rect.maxX >= 8) {
            rect.maxX = 7;
        }

        CAaBox localBox;
        localBox.b = box->b - chunk->m_topLeftCoords;
        localBox.t = box->t - chunk->m_topLeftCoords;

        if (flags & 0x100) {
            chunk->Intersect(&rect, &localBox, &facets->facets);
        }

        if (flags & 0x30000) {
            bool fishableOnly = (flags & 0x10000) && !(flags & 0x20000);

            for (CChunkLiquid* liquid = chunk->m_liquidChunkLinkList.Head(); liquid; liquid = chunk->m_liquidChunkLinkList.Next(liquid)) {
                if (!fishableOnly || (g_liquidTypeDB.GetRecord(liquid->m_type)->m_flags & 0x4)) {
                    CMap::GetChunkLiquidFacets(chunk, &localBox, &rect, liquid, facets);
                }
            }
        }

        uint32_t count = facets->facets.Count();
        facets->facetIds.SetCount(count);

        if (count > first) {
            memset(&facets->facetIds.m_data[first], 0, sizeof(uint64_t) * (count - first));
        }

        if ((flags & 0xF0000F) && !CMap::GetDoodadDefFacets(&chunk->m_doodadDefLinkList, box, facets, flags)) {
            return false;
        }

        if (flags & 0xF00000) {
            CMap::QueryDestructibleFacets(&chunk->m_entityLinkList, box, facets, flags);
        }

        return true;
    }

    if (!(CMap::m_areaInfo[areaIndex].m_flags & 1)) {
        return true;
    }

    if (static_cast<int32_t>(flags) >= 0) {
        return false;
    }

    CAaBox cellBox = *box;
    cellBox.b.x = (chunkY + 1) * -33.333332f + 17066.666f;
    cellBox.b.y = (chunkX + 1) * -33.333332f + 17066.666f;
    cellBox.b.z = box->b.z - 1000.0f;
    cellBox.t.x = chunkY * -33.333332f + 17066.666f;
    cellBox.t.y = -33.333332f * chunkX + 17066.666f;
    cellBox.t.z = box->t.z + 1000.0f;

    if (cellBox.Intersects(*queryBox)) {
        return false;
    }

    World::AddAaBoxFacets(&cellBox, facets);
    return true;
}

// VERIFY: c903d39d manual
// OFFSET: 0x007A55E0
bool CMap::GetMapObjFacets(CAaBox* queryBox, CAaBox* box, World::FacetData* facets, uint32_t flags, uint32_t* status) {
    CAaBox localBox;
    localBox.b = C3Vector(0.0f, 0.0f, 0.0f);
    localBox.t = C3Vector(0.0f, 0.0f, 0.0f);
    C3Vector center;
    center.x = (box->t.x + box->b.x) * 0.5f;
    center.y = (box->t.y + box->b.y) * 0.5f;
    center.z = 0.5f * (box->t.z + box->b.z);
    CAaBox centered;
    centered.b = box->b - center;
    centered.t = box->t - center;

    for (CDestructibleProxy* proxy = CMap::s_destructibleProxyList.Head(); proxy; proxy = CMap::s_destructibleProxyList.Next(proxy)) {
        C3Vector extent = box->t - box->b;
        float radius = sqrtf(extent.x * extent.x + (extent.y * extent.y + extent.z * extent.z)) + proxy->m_sphere.r;
        C3Vector delta = proxy->m_sphere.c - center;

        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < radius * radius) {
            return false;
        }
    }

    for (CMapObjDef* def = CMap::mapObjDefHash.m_fulllist.Head(); def; def = CMap::mapObjDefHash.m_fulllist.Next(def)) {
        if (def->m_flags & 0x100) {
            continue;
        }

        CMapObj* owner = def->m_owner;

        if (!owner) {
            continue;
        }

        if (box->t.x < def->m_bbox.b.x || box->t.y < def->m_bbox.b.y || box->t.z < def->m_bbox.b.z || box->b.x > def->m_bbox.t.x || box->b.y > def->m_bbox.t.y || box->b.z > def->m_bbox.t.z) {
            continue;
        }

        if (!owner->m_isGroupLoaded || !def->m_mapObjDefGroupLinkList.Head()) {
            if (static_cast<int32_t>(flags) >= 0 || def->m_bbox.Intersects(*queryBox)) {
                return false;
            }

            World::AddAaBoxFacets(&def->m_bbox, facets);
            continue;
        }

        C3Vector localCenter = center * def->m_invMat;
        C33Matrix rotation(def->m_invMat);
        CWorldMath::TransformAABox(rotation, centered, localBox);
        localBox.b += localCenter;
        localBox.t += localCenter;

        if (owner->TestBounds(localBox)) {
            if (flags & 0xF0) {
                World::TriData::statusFlags = 0;
                World::TriData::nBatches = 0;
                World::TriData::faceIndexCursor = 0;
                World::TriData::indexCursor = 0;
                World::TriData::matrixPoolCount = 0;
                World::TriData triData;
                owner->GetTris(localBox, flags, &triData, def);
                World::TriDataToFacetData(&triData, facets, def->m_objectGuid);

                if (status) {
                    *status |= World::TriData::statusFlags;
                }
            }

            if (flags & 0x30000) {
                owner->GetLiquidFacets(localBox, flags, facets, def);
            }
        }

        for (CMapBaseObjLink* link = def->m_mapObjDefGroupLinkList.Head(); link; link = def->m_mapObjDefGroupLinkList.Next(link)) {
            CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(link->m_owner);

            if (box->t.x < group->m_bbox.b.x || box->t.y < group->m_bbox.b.y || box->t.z < group->m_bbox.b.z || box->b.x > group->m_bbox.t.x || box->b.y > group->m_bbox.t.y || box->b.z > group->m_bbox.t.z) {
                continue;
            }

            if (owner->IsGroupLoaded(group->m_groupNum)) {
                if ((flags & 0xF0000F) && !CMap::GetDoodadDefFacets(&group->m_doodadDefLinkList, box, facets, flags)) {
                    return false;
                }

                if (flags & 0xF00000) {
                    CMap::QueryDestructibleFacets(&group->m_entityLinkList, box, facets, flags);
                }
            } else {
                if (static_cast<int32_t>(flags) >= 0) {
                    return false;
                }

                World::AddAaBoxFacets(&group->m_bbox, facets);
            }
        }
    }

    return true;
}

// VERIFY: 5c2f5676 manual
// OFFSET: 0x007A4590
bool CMap::CreateFlightBoundsFacets(int32_t areaX, int32_t areaY, CAaBox* box, World::FacetData* facets) {
    CMapArea* area = CMap::m_areaTable[64 * areaY + areaX];

    if (!area || area->m_asyncObject) {
        return false;
    }

    const int16_t* heights = area->m_flyingBbox;

    if (!heights) {
        return true;
    }

    uint32_t first = facets->facets.Count();
    int16_t top = static_cast<int16_t>(static_cast<int32_t>(box->t.z + 1.0f));
    int32_t above[9];

    for (uint32_t i = 0; i < 9; i++) {
        above[i] = top < heights[i];
    }

    for (uint32_t tri = 0; tri < sizeof(s_flightBoundsTriangles) / sizeof(s_flightBoundsTriangles[0]); tri += 3) {
        int32_t i0 = s_flightBoundsTriangles[tri];
        int32_t i1 = s_flightBoundsTriangles[tri + 1];
        int32_t i2 = s_flightBoundsTriangles[tri + 2];

        if (above[i0] && above[i1] && above[i2]) {
            continue;
        }

        C3Vector v0(s_flightBoundsOffsets[i0][0] + area->m_topLeft2.x, s_flightBoundsOffsets[i0][1] + area->m_topLeft2.y, static_cast<float>(heights[i0]));
        C3Vector v1(s_flightBoundsOffsets[i1][0] + area->m_topLeft2.x, s_flightBoundsOffsets[i1][1] + area->m_topLeft2.y, static_cast<float>(heights[i1]));
        C3Vector v2(s_flightBoundsOffsets[i2][0] + area->m_topLeft2.x, s_flightBoundsOffsets[i2][1] + area->m_topLeft2.y, static_cast<float>(heights[i2]));
        CFacet* facet = facets->facets.New();
        facet->plane.From3Pos(v0, v1, v2);
        facet->v[0] = v0;
        facet->v[1] = v1;
        facet->v[2] = v2;
    }

    int16_t bottom = static_cast<int16_t>(static_cast<int32_t>(box->b.z - 1.0f));
    int32_t below[9];

    for (uint32_t i = 0; i < 9; i++) {
        below[i] = bottom > heights[9 + i];
    }

    for (uint32_t tri = 0; tri < sizeof(s_flightBoundsTriangles) / sizeof(s_flightBoundsTriangles[0]); tri += 3) {
        int32_t i0 = s_flightBoundsTriangles[tri];
        int32_t i1 = s_flightBoundsTriangles[tri + 1];
        int32_t i2 = s_flightBoundsTriangles[tri + 2];

        if (below[i0] && below[i1] && below[i2]) {
            continue;
        }

        C3Vector v0(s_flightBoundsOffsets[i0][0] + area->m_topLeft2.x, s_flightBoundsOffsets[i0][1] + area->m_topLeft2.y, static_cast<float>(heights[9 + i0]));
        C3Vector v1(s_flightBoundsOffsets[i1][0] + area->m_topLeft2.x, s_flightBoundsOffsets[i1][1] + area->m_topLeft2.y, static_cast<float>(heights[9 + i1]));
        C3Vector v2(s_flightBoundsOffsets[i2][0] + area->m_topLeft2.x, s_flightBoundsOffsets[i2][1] + area->m_topLeft2.y, static_cast<float>(heights[9 + i2]));
        CFacet* facet = facets->facets.New();
        facet->plane.From3Pos(v0, v2, v1);
        facet->v[0] = v0;
        facet->v[1] = v2;
        facet->v[2] = v1;
    }

    uint32_t count = facets->facets.Count();
    facets->facetIds.SetCount(count);

    if (count > first) {
        memset(&facets->facetIds.m_data[first], 0, sizeof(uint64_t) * (count - first));
    }

    return true;
}

// VERIFY: f40f6721 manual
// OFFSET: 0x007A5F20
bool CMap::GetFacets(CAaBox* a1, CAaBox* a2, World::FacetData* a3, uint32_t a4, uint32_t* a5) {
    CMap::mapGetFacetsCount++;
    CMap::s_queryTag++;
    a3->facets.SetCount(0);

    if (!CMap::GetMapObjFacets(a1, a2, a3, a4, a5)) {
        return false;
    }

    if (CMap::bDungeon) {
        return true;
    }

    float topX = -(a2->t.x - 17066.666f);
    float topY = -(a2->t.y - 17066.666f);
    float bottomX = -(a2->b.x - 17066.666f);

    if (topY < 0.0f) {
        return false;
    }

    float bottomY = -(a2->b.y - 17066.666f);

    if (topX < 0.0f || bottomY >= 34133.332f || bottomX >= 34133.332f) {
        return false;
    }

    CiRect rect;
    rect.maxX = CMath::fint_(bottomY * 0.24f - 0.5f);
    rect.maxY = CMath::fint_(bottomX * 0.24f - 0.5f);
    rect.minX = CMath::fint_(topY * 0.24f - 0.5f);
    rect.minY = CMath::fint_(topX * 0.24f - 0.5f);
    bool result = true;

    for (int32_t y = rect.minY >> 3; y <= rect.maxY >> 3; y++) {
        for (int32_t x = rect.minX >> 3; x <= rect.maxX >> 3; x++) {
            if (!CMap::GetChunkFacets(x, y, &rect, a1, a2, a3, a4)) {
                result = false;
            }
        }
    }

    if (a4 & 0x200) {
        for (int32_t y = (rect.minY >> 3) >> 4; y <= (rect.maxY >> 3) >> 4; y++) {
            for (int32_t x = (rect.minX >> 3) >> 4; x <= (rect.maxX >> 3) >> 4; x++) {
                if (!CMap::CreateFlightBoundsFacets(x, y, a2, a3)) {
                    result = false;
                }
            }
        }
    }

    return result;
}

// VERIFY: 4b94dd93 manual
// OFFSET: 0x007A2C60
void CMap::SetHitTestDebug(World::HitTestDebug* hitInfo, CMapBaseObj* hitObject) {
    hitInfo->m_bbox.b.x = 0.0f;
    hitInfo->m_bbox.b.y = 0.0f;
    hitInfo->m_bbox.b.z = 0.0f;
    hitInfo->m_bbox.t.x = 0.0f;
    hitInfo->m_bbox.t.y = 0.0f;
    hitInfo->m_bbox.t.z = 0.0f;
    hitInfo->m_sphere.c.x = 0.0f;
    hitInfo->m_sphere.c.y = 0.0f;
    hitInfo->m_sphere.c.z = 0.0f;
    hitInfo->m_sphere.r = 0.0f;
    hitInfo->m_labelPos.x = 0.0f;
    hitInfo->m_labelPos.y = 0.0f;
    hitInfo->m_type = 0;
    hitInfo->m_numModels = 0;
    hitInfo->m_numParents = 0;
    hitInfo->m_labelPos.z = 0.0f;

    if (!hitObject) {
        return;
    }

    CM2Model* model = nullptr;

    if (hitObject->m_type & 0x40) {
        CMapStaticEntity* entity = static_cast<CMapStaticEntity*>(hitObject);
        hitInfo->m_type = 1;
        hitInfo->m_bbox = entity->m_bbox;
        hitInfo->m_sphere = entity->m_sphere;
        hitInfo->m_labelPos = entity->m_vec2;
        hitInfo->m_labelPos.z += 0.1f;
        model = entity->m_model;
    } else if (hitObject->m_type & 0x20) {
        CMapStaticEntity* entity = static_cast<CMapStaticEntity*>(hitObject);
        hitInfo->m_type = 2;
        hitInfo->m_bbox = entity->m_bbox;
        hitInfo->m_sphere = entity->m_sphere;
        hitInfo->m_labelPos = entity->m_vec2;

        if (entity->m_staticFlags & 0x2000) {
            float raised = hitInfo->m_labelPos.z + 4.0f;
            float top = entity->m_bbox.t.z + 0.1f;
            hitInfo->m_labelPos.z = top > raised ? raised : top;
        }

        model = entity->m_model;
    } else if (hitObject->m_type & 0x10) {
        CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(hitObject);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(defGroup->m_parentLinkList.Head()->m_ref);
        CMapObj* mapObj = mapObjDef->m_owner;
        CMapObjGroup* group = mapObj->GetGroup(defGroup->m_groupNum, 0);
        World::HitTestDebug::Model* entry = &hitInfo->m_models[hitInfo->m_numModels++];
        hitInfo->m_type = 4;
        entry->m_name = PathFileName(mapObj->m_wmoName);
        entry->m_depth = 0;
        entry->m_bbox = defGroup->m_bbox;
        entry->m_sphere = defGroup->m_sphere;
        entry->m_numGeometryBatches = group->m_batchListCount;
        entry->m_numPrimitives = group->m_polyListSize;
        entry->m_model = nullptr;
        entry->m_attachmentScale = 1.0f;
        entry->m_numOtherBatches = 0;
        entry->m_numBones = 0;
        entry->m_numLights = 0;
        entry->m_numCurrentLights = 0;
        entry->m_attachmentId = 0;
        entry->m_numCombinableBatches = 0;
        entry->m_instanceCountMax = 0;
        entry->m_groupName = group->m_groupName;
        entry->m_groupNum = defGroup->m_groupNum;
        entry->m_groupFlags = group->m_flags & 8;
        entry->m_doodadSet = mapObjDef->m_doodadSet;
    }

    CMapBaseObj* object = hitObject;

    if (object->m_type & 0x10) {
        object = object->m_parentLinkList.Head()->m_ref;
    }

    for (CMapBaseObjLink* link = object->m_parentLinkList.Head(); link; link = object->m_parentLinkList.Next(link)) {
        CMapBaseObj* parent = link->m_ref;

        if (!parent) {
            continue;
        }

        if (parent->m_type & 0x4) {
            if (hitInfo->m_numParents < 8) {
                World::HitTestDebug::Parent* entry = &hitInfo->m_parents[hitInfo->m_numParents++];
                entry->m_name = "Terrain";
                entry->m_groupName = nullptr;
                entry->m_chunkOffset = static_cast<CMapChunk*>(parent)->m_cOffset;
            }
        } else if (parent->m_type & 0x10) {
            CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(parent);
            CMapObj* mapObj = static_cast<CMapObjDef*>(defGroup->m_parentLinkList.Head()->m_ref)->m_owner;
            CMapObjGroup* group = mapObj->GetGroup(defGroup->m_groupNum, 0);

            if (hitInfo->m_numParents < 8) {
                World::HitTestDebug::Parent* entry = &hitInfo->m_parents[hitInfo->m_numParents++];
                entry->m_name = PathFileName(mapObj->m_wmoName);
                entry->m_groupName = group->m_groupName;
                entry->m_chunkOffset.x = 0;
                entry->m_chunkOffset.y = 0;
            }
        }
    }

    if (model) {
        while (model->m_attachParent) {
            model = model->m_attachParent;
        }

        AddHitDebugModel(hitInfo, 0, model);
    }
}

// VERIFY: a85f0fdc manual
// OFFSET: 0x007A30D0
bool CMap::VectorIntersect(C3Vector* start, C3Vector* end, uint32_t flags, uint32_t defIgnoreFlags, float* distance, uint16_t* hitIndex, CMapObj** outMapObj, CMapObjDef** outMapObjDef, CMapObjDefGroup** outMapObjDefGroup) {
    uint32_t modelFlags = flags & 0x40F0000F;

    if (modelFlags) {
        World::s_m2Scene->BeginHitTest();
    }

    C3Vector delta = *end - *start;
    float length = sqrtf(delta.x * delta.x + (delta.y * delta.y + delta.z * delta.z));
    CAaBox box;
    box.b = C3Vector(0.0f, 0.0f, 0.0f);
    box.t = C3Vector(0.0f, 0.0f, 0.0f);
    MapObjIntersectCandidate* candidates = static_cast<MapObjIntersectCandidate*>(alloca(sizeof(MapObjIntersectCandidate) * 500));
    uint32_t candidateCount = 0;

    for (CMapObjDef* def = CMap::mapObjDefHash.m_fulllist.Head(); def; def = CMap::mapObjDefHash.m_fulllist.Next(def)) {
        if (defIgnoreFlags & def->m_flags) {
            continue;
        }

        CMapObj* owner = def->m_owner;

        if (!owner || !owner->m_isGroupLoaded || !CWorldMath::VectorIntersectAABox2(def->m_bbox, *start, *end)) {
            continue;
        }

        uint32_t groupCount = def->m_defGroups.m_count == 0xFFFFFFFF ? def->m_defGroups.m_dynamic.m_count : def->m_defGroups.m_count;

        for (uint32_t i = 0; i < groupCount; i++) {
            CMapObjDefGroup* group = def->m_defGroups[i];

            if (!owner->IsGroupLoaded(i) || !CWorldMath::VectorIntersectAABox2(group->m_bbox, *start, *end)) {
                continue;
            }

            MapObjIntersectCandidate& candidate = candidates[candidateCount++];
            candidate.def = def;
            candidate.group = group;
            C3Vector localStart = *start * def->m_invMat;

            if (owner->TestGroupBounds(localStart, i)) {
                candidate.t = 0.0f;
                continue;
            }

            owner->GetGroupBounds(&box, i);
            float x = localStart.x >= box.b.x ? (box.t.x >= localStart.x ? localStart.x : box.t.x) : box.b.x;
            float y = localStart.y >= box.b.y ? (box.t.y >= localStart.y ? localStart.y : box.t.y) : box.b.y;
            float z = localStart.z >= box.b.z ? (box.t.z >= localStart.z ? localStart.z : box.t.z) : box.b.z;
            candidate.t = sqrtf((localStart.x - x) * (localStart.x - x) + (localStart.z - z) * (localStart.z - z) + (localStart.y - y) * (localStart.y - y)) / length;
        }
    }

    qsort(candidates, candidateCount, sizeof(MapObjIntersectCandidate), CompareIntersectCandidates);
    uint32_t ignoreFlags = CMapObj::CreateWmoIgnoreFlags(flags);
    float bestT = *distance;
    bool hit = false;
    int32_t bestFace = -1;

    for (uint32_t i = 0; i < candidateCount; i++) {
        CMapObjDef* def = candidates[i].def;
        CMapObjDefGroup* group = candidates[i].group;
        CMapObj* mapObj = def->m_owner;
        C3Vector localStart = *start * def->m_invMat;
        C3Vector localEnd = *end * def->m_invMat;

        if (candidates[i].t <= bestT && mapObj->TestGroupBounds(localStart, localEnd, group->m_groupNum) && mapObj->Intersect(localStart, localEnd, &bestT, flags, ignoreFlags, group->m_groupNum, &bestFace)) {
            hit = true;

            if (outMapObj) {
                *outMapObj = mapObj;
            }

            if (outMapObjDef) {
                *outMapObjDef = def;
            }

            if (outMapObjDefGroup) {
                *outMapObjDefGroup = group;
            }
        }

        if (modelFlags) {
            CMap::VectorIntersectDoodadDefs(&group->m_doodadDefLinkList, flags);
        }

        if (flags & 0x40F00000) {
            CMap::VectorIntersectEntitys(&group->m_entityLinkList, flags);
        }
    }

    if (hit) {
        *distance = bestT;

        if (hitIndex) {
            *hitIndex = static_cast<uint16_t>(bestFace);
        }
    }

    if (modelFlags) {
        C3Vector localStart = *start * CWorldScene::camTransportView;
        C3Vector localEnd = *end * CWorldScene::camTransportView;
        float t = *distance;
        CMapBaseObj* owner = static_cast<CMapBaseObj*>(World::s_m2Scene->EndHitTest(localStart, localEnd, t, 0));

        if (t < *distance) {
            if ((owner->m_type & 0x40) && static_cast<CMapDoodadDef*>(owner)->m_collisionGuid.m_guid) {
                s_lastCollisionGUID = static_cast<CMapDoodadDef*>(owner)->m_collisionGuid;
            }

            *distance = t;

            if (hitIndex) {
                *hitIndex = 0xFFFF;
            }

            return true;
        }
    }

    if (!hit && hitIndex) {
        *hitIndex = 0xFFFF;
    }

    return hit;
}

// VERIFY: b625f45f manual
// OFFSET: 0x007A3B70
bool CMap::Intersect(C3Vector* start, C3Vector* end, C3Vector* hitPoint, float* distance, uint32_t flags, World::HitTestDebug* hitInfo) {
    CMap::s_queryTag++;
    bool hit = false;
    CMapObjDef* mapObjDef = nullptr;
    CMapObjDefGroup* mapObjDefGroup = nullptr;

    if (flags & 0x40F300FF) {
        World::s_m2Scene->m_lastHit.m_model = nullptr;
        s_lastCollisionGUID = WOWGUID{ 0 };
        CMapObj* mapObj;

        if (CMap::VectorIntersect(start, end, flags, 0x100, distance, nullptr, &mapObj, &mapObjDef, &mapObjDefGroup)) {
            if (mapObjDef && mapObjDef->m_objectGuid.m_guid) {
                s_lastCollisionGUID = mapObjDef->m_objectGuid;
            }

            if (hitInfo) {
                if (World::s_m2Scene->m_lastHit.m_model) {
                    CMap::SetHitTestDebug(hitInfo, static_cast<CMapBaseObj*>(World::s_m2Scene->m_lastHitOwner));
                } else {
                    CMap::SetHitTestDebug(hitInfo, mapObjDefGroup);
                }
            }

            hit = true;
        }
    }

    if (flags & 0x40F3010F) {
        World::s_m2Scene->m_lastHit.m_model = nullptr;

        if (CMap::VectorIntersectTerrain(start, end, distance, flags, nullptr)) {
            s_lastCollisionGUID = WOWGUID{ 0 };

            if (hitInfo && World::s_m2Scene->m_lastHit.m_model) {
                CMap::SetHitTestDebug(hitInfo, static_cast<CMapBaseObj*>(World::s_m2Scene->m_lastHitOwner));
            }

            hit = true;
        }
    }

    if (!hit) {
        return false;
    }

    if (hitPoint) {
        float t = *distance;
        C3Vector point;
        point.x = (end->x - start->x) * t + start->x;
        point.y = (end->y - start->y) * t + start->y;
        point.z = (end->z - start->z) * t + start->z;
        *hitPoint = point;
    }

    return true;
}

// VERIFY: 7a1bc3ec manual
// OFFSET: 0x007A0490
int32_t CMap::QueryAreaIdTerrain(const C3Vector* pos) {
    float x = -(pos->x - 17066.666f);
    float y = -(pos->y - 17066.666f) * 0.03f;
    int32_t cellY = CMath::fint_(y - 0.5f);
    x = x * 0.03f;
    int32_t cellX = CMath::fint_(x - 0.5f);
    CMapArea* area = CMap::m_areaTable[64 * ((cellX >> 4) & 0x3F) + ((cellY >> 4) & 0x3F)];

    if (!area || area->m_asyncObject) {
        return 0;
    }

    CMapChunk* chunk = area->m_mapChunks[16 * (cellX & 0xF) + (cellY & 0xF)];

    if (!chunk) {
        return 0;
    }

    return chunk->m_areaId;
}

// VERIFY: 0a8e296f manual
// OFFSET: 0x00782560
bool CMap::QueryAreaId(CMapEntity* entity, uint32_t& areaId) {
    if (CMap::bDungeon) {
        int32_t areaTableId = g_mapDB.GetRecord(s_mapId)->m_areaTableID;

        if (!areaTableId) {
            return false;
        }

        areaId = areaTableId;
        return true;
    }

    if (!entity) {
        return false;
    }

    CMapBaseObjLink* link = entity->m_parentLinkList.Head();

    if (link) {
        if (link->m_ref->m_type & 0x4) {
            areaId = static_cast<CMapChunk*>(link->m_ref)->m_areaId;
            return true;
        }

        CMapObjDef* mapObjDef;

        while (true) {
            CMapBaseObj* ref = link->m_ref;
            mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);

            if (!(mapObjDef->m_flags & 0x400)) {
                break;
            }

            link = entity->m_parentLinkList.Next(link);

            if (!link) {
                areaId = CMap::QueryAreaIdTerrain(&entity->m_position);
                return true;
            }
        }

        CMapObjGroup* group = mapObjDef->m_owner->GetGroup(static_cast<CMapObjDefGroup*>(link->m_ref)->m_groupNum, 0);
        WMOAreaTableRec* rec = GetWMOAreaRec(mapObjDef->m_owner->m_header->m_wmoID, mapObjDef->m_nameSet, group->m_wmoGroupId);

        if (rec && rec->m_areaTableID) {
            areaId = rec->m_areaTableID;
            return true;
        }
    }

    areaId = CMap::QueryAreaIdTerrain(&entity->m_position);
    return true;
}

// VERIFY: 0728f3ec auto
// OFFSET: 0x009905C0
const LiquidTypeRec* GetLiquidTypeRecForArea(uint32_t areaId, uint16_t liquidType) {
    if (!liquidType) {
        return nullptr;
    }

    if (areaId && liquidType < 21) {
        uint32_t index = (liquidType - 1) & 3;
        AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

        if (area) {
            if (!area->m_liquidTypeID[index] && area->m_parentAreaID) {
                area = g_areaTableDB.GetRecord(area->m_parentAreaID);
            }

            if (area && area->m_liquidTypeID[index]) {
                return g_liquidTypeDB.GetRecord(area->m_liquidTypeID[index]);
            }
        }
    }

    return g_liquidTypeDB.GetRecord(liquidType);
}

// VERIFY: 44408b24 auto
// OFFSET: 0x00990530
static int32_t CompareWMOAreaRec(const void* a, const void* b) {
    const WMOAreaTableRec* ra = static_cast<const WMOAreaTableRec*>(a);
    const WMOAreaTableRec* rb = static_cast<const WMOAreaTableRec*>(b);
    int32_t result = ra->m_wmoid - rb->m_wmoid;

    if (!result) {
        result = ra->m_nameSetID - rb->m_nameSetID;

        if (!result) {
            return ra->m_wmogroupID - rb->m_wmogroupID;
        }
    }

    return result;
}

// VERIFY: 5bad450c auto
// OFFSET: 0x00990560
WMOAreaTableRec* GetWMOAreaRec(int32_t wmoId, int32_t nameSet, int32_t groupId) {
    WMOAreaTableRec key;
    key.m_nameSetID = nameSet;
    key.m_wmoid = wmoId;
    key.m_wmogroupID = groupId;
    return static_cast<WMOAreaTableRec*>(bsearch(&key, g_wmoAreaTableDB.m_numRecords <= 0 ? nullptr : g_wmoAreaTableDB.m_records, g_wmoAreaTableDB.m_numRecords, sizeof(WMOAreaTableRec), CompareWMOAreaRec));
}

// VERIFY: ff49ec1c manual
// OFFSET: 0x007A3D50
bool CMap::GetChunkLiquidFacets(CMapChunk* chunk, CWFrustum* frustum, CiRect* rect, CChunkLiquid* liquid, World::FacetData* facets) {
    World::TriData::statusFlags = 0;
    World::TriData::nBatches = 0;
    World::TriData::faceIndexCursor = 0;
    World::TriData::indexCursor = 0;
    World::TriData::matrixPoolCount = 0;
    World::TriData triData;

    if (!liquid->GetTris(&triData, *frustum, *rect)) {
        return false;
    }

    return World::TriDataToFacetData(&triData, facets, WOWGUID{ 0 }) != 0;
}

// VERIFY: 784e1e63 manual
// OFFSET: 0x007A5330
bool CMap::GetChunkFacets(int32_t chunkX, int32_t chunkY, CiRect* subRect, CFrustum* frustum, World::FacetData* facets, uint32_t flags) {
    uint32_t first = facets->facets.Count();
    CMapArea* area = CMap::m_areaTable[((chunkX >> 4) & 0x3F) + (((chunkY >> 4) & 0x3F) << 6)];

    if (!area || area->m_asyncObject) {
        return false;
    }

    CMapChunk* chunk = area->m_mapChunks[16 * (chunkY & 0xF) + (chunkX & 0xF)];

    if (!chunk) {
        return false;
    }

    CiRect rect;
    rect.minY = subRect->minY - 8 * chunkY;
    rect.minX = subRect->minX - 8 * chunkX;
    rect.maxY = subRect->maxY - 8 * chunkY;
    rect.maxX = subRect->maxX - 8 * chunkX;

    if (rect.minY < 0) {
        rect.minY = 0;
    }

    if (rect.minX < 0) {
        rect.minX = 0;
    }

    if (rect.maxY >= 8) {
        rect.maxY = 7;
    }

    if (rect.maxX >= 8) {
        rect.maxX = 7;
    }

    CWFrustum localFrustum(*frustum);
    localFrustum.Translate(C3Vector(-chunk->m_topLeftCoords.x, -chunk->m_topLeftCoords.y, -chunk->m_topLeftCoords.z));

    if (flags & 0x100) {
        chunk->Intersect(&rect, &localFrustum, &facets->facets);
    }

    if (flags & 0x30000) {
        bool fishableOnly = (flags & 0x10000) && !(flags & 0x20000);

        for (CChunkLiquid* liquid = chunk->m_liquidChunkLinkList.Head(); liquid; liquid = chunk->m_liquidChunkLinkList.Next(liquid)) {
            if (!fishableOnly || (g_liquidTypeDB.GetRecord(liquid->m_type)->m_flags & 0x4)) {
                CMap::GetChunkLiquidFacets(chunk, &localFrustum, &rect, liquid, facets);
            }
        }
    }

    if (flags & 0xF) {
        CAaBox box = CAaBox::Bounding(frustum->corners, 8);

        for (CMapBaseObjLink* link = chunk->m_doodadDefLinkList.Head(); link; link = chunk->m_doodadDefLinkList.Next(link)) {
            CMapDoodadDef* def = static_cast<CMapDoodadDef*>(link->m_owner);

            if ((def->m_flags & 0x100) || !(def->m_flags & 0x80) || def->m_unkCounter == CMap::s_queryTag) {
                continue;
            }

            CM2Model* model = def->m_model;

            if (model && box.Intersects(def->m_bboxDoodadDef)) {
                model->GetCollisionFacets(box, def->m_mat, facets->facets);
                def->m_unkCounter = CMap::s_queryTag;
            }
        }
    }

    return first != facets->facets.Count();
}

// VERIFY: 97cd8735 manual
// OFFSET: 0x007A4EE0
bool CMap::GetMapObjFacets(CFrustum* frustum, World::FacetData* facets, uint32_t flags, uint32_t* status) {
    bool result = false;

    for (CMapObjDef* def = CMap::mapObjDefHash.m_fulllist.Head(); def; def = CMap::mapObjDefHash.m_fulllist.Next(def)) {
        if ((def->m_flags & 0x100) || !frustum->Cull(def->m_bbox)) {
            continue;
        }

        C3Vector corners[8];

        for (uint32_t i = 0; i < 8; i++) {
            corners[i] = frustum->corners[i] * def->m_invMat;
        }

        CWFrustum localFrustum(corners);
        CMapObj* owner = def->m_owner;

        if (owner) {
            World::TriData::statusFlags = 0;
            World::TriData::nBatches = 0;
            World::TriData::faceIndexCursor = 0;
            World::TriData::indexCursor = 0;
            World::TriData::matrixPoolCount = 0;
            World::TriData triData;
            result |= owner->GetTris(localFrustum, flags, &triData, def);
            World::TriDataToFacetData(&triData, facets, WOWGUID{ 0 });

            if (status) {
                *status |= World::TriData::statusFlags;
            }
        }
    }

    return result;
}

// VERIFY: 041a89f8 manual
// OFFSET: 0x007A5DD0
bool CMap::QueryFacets(CFrustum* frustum, World::FacetData* facets, uint32_t flags, uint32_t* count) {
    CMap::s_queryTag++;
    facets->facets.SetCount(0);
    CAaBox box = CAaBox::Bounding(frustum->corners, 8);
    float topX = -(box.t.x - 17066.666f);
    float topY = -(box.t.y - 17066.666f);
    float bottomX = -(box.b.x - 17066.666f);
    float bottomY = -(box.b.y - 17066.666f);
    CiRect rect;
    rect.maxX = CMath::fint_(bottomY * 0.24f - 0.5f);
    rect.maxY = CMath::fint_(bottomX * 0.24f - 0.5f);
    rect.minX = CMath::fint_(topY * 0.24f - 0.5f);
    rect.minY = CMath::fint_(topX * 0.24f - 0.5f);

    for (int32_t y = rect.minY >> 3; y <= rect.maxY >> 3; y++) {
        for (int32_t x = rect.minX >> 3; x <= rect.maxX >> 3; x++) {
            CMap::GetChunkFacets(x, y, &rect, frustum, facets, flags);
        }
    }

    CMap::GetMapObjFacets(frustum, facets, flags, count);
    return facets->facets.Count() != 0;
}

// VERIFY: 10d86f1c auto
// OFFSET: 0x0077F8B0
int32_t CMap::QueryFlightBounds(const C3Vector& pos, float* height) {
    return CMap::GetFlightBounds(pos, height, 0);
}

// VERIFY: 680c26b6 manual
// OFFSET: 0x007A0530
bool CMap::QueryGroundTypeTerrain(C3Vector* pos, uint32_t* groundType) {
    float y = -(pos->y - 17066.666f);
    float x = -(pos->x - 17066.666f);

    if (y < 0.0f || x < 0.0f || y >= 34133.332f || x >= 34133.332f) {
        return false;
    }

    float cellYf = x * 0.24f;
    float cellXf = y * 0.24f;
    int32_t cellY = CMath::fint_(cellYf - 0.5f);
    int32_t cellX = CMath::fint_(cellXf - 0.5f);

    CMapArea* area = CMap::GetLoadedArea((cellX >> 7) & 0x3F, (cellY >> 7) & 0x3F);

    if (!area) {
        return false;
    }

    CMapChunk* chunk = area->m_mapChunks[16 * ((cellY >> 3) & 0xF) + ((cellX >> 3) & 0xF)];

    if (!chunk) {
        return false;
    }

    int32_t col = cellX & 7;
    int32_t row = cellY & 7;

    if (chunk->m_header->m_holes & CMap::s_holeMask[4 * (row >> 1) + (col >> 1)]) {
        return false;
    }

    uint32_t layer = (CMap::s_2bitSplatMask[col] & reinterpret_cast<uint16_t*>(chunk->m_lowQualityTexMap)[row]) >> CMap::s_2bitSplatShft[col];
    GroundEffectTextureRec* rec = g_groundEffectTextureDB.GetRecord(chunk->m_layers[layer].m_offectId);

    if (!rec) {
        return false;
    }

    *groundType = rec->m_sound;
    return true;
}

// VERIFY: ddd4e04d manual
// OFFSET: 0x0079D2B0
void CMap::QueryLiquidSounds(C3Vector* pos, float radius, TSGrowableArray<World::LiquidQueryResult>& results) {
    float x = -(pos->x - 17066.666f);
    float cellYf = -(pos->y - 17066.666f) * 0.24f;
    int32_t cellY = CMath::fint_(cellYf - 0.5f);
    x = x * 0.24f;
    int32_t cellX = CMath::fint_(x - 0.5f);

    CAaSphere sphere;
    sphere.c.x = pos->x;
    sphere.c.y = pos->y;
    sphere.c.z = 0.0f;
    sphere.r = radius;

    int32_t areaRow = (cellX >> 7) & 0x3F;
    int32_t areaCol = (cellY >> 7) & 0x3F;
    int32_t minRow = areaRow - 1 > 0 ? areaRow - 1 : 0;
    int32_t maxRow = areaRow + 1 >= 63 ? 63 : areaRow + 1;
    int32_t minCol = areaCol - 1 > 0 ? areaCol - 1 : 0;
    int32_t maxCol = areaCol + 1 >= 63 ? 63 : areaCol + 1;

    for (int32_t row = minRow; row <= maxRow; row++) {
        for (int32_t col = minCol; col <= maxCol; col++) {
            CMapArea* area = CMap::m_areaTable[(row << 6) + col];

            if (!area || area->m_asyncObject) {
                continue;
            }

            CAaBox box;
            box.b.x = area->m_topLeft2.x - 533.33331f;
            box.b.y = area->m_topLeft2.y - 533.33331f;
            box.b.z = 0.0f;
            box.t.x = area->m_topLeft2.x;
            box.t.y = area->m_topLeft2.y;
            box.t.z = 0.0f;

            if (NTempest::Intersect2d(box, sphere, NTempest::SOLID_BOTH)) {
                area->QueryLiquidSounds(pos, radius, results);
            }
        }
    }
}

// STATIC: 0x00AEDF58
static const uint32_t s_flightTriangles[8][3] = {
    { 3, 0, 4 },
    { 0, 1, 4 },
    { 1, 2, 4 },
    { 2, 5, 4 },
    { 5, 8, 4 },
    { 8, 7, 4 },
    { 7, 6, 4 },
    { 6, 3, 4 },
};

// STATIC: 0x00AEDFB8
static const float s_flightVertices[9][2] = {
    { 0.0f, 0.0f },
    { 0.0f, -266.66666f },
    { 0.0f, -533.33331f },
    { -266.66666f, 0.0f },
    { -266.66666f, -266.66666f },
    { -266.66666f, -533.33331f },
    { -533.33331f, 0.0f },
    { -533.33331f, -266.66666f },
    { -533.33331f, -533.33331f },
};

// VERIFY: b2919fa1 manual
// OFFSET: 0x007AD700
int32_t CMap::GetFlightBounds(const C3Vector& pos, float* height, int32_t which) {
    if (CMap::bDungeon) {
        return 0;
    }

    int32_t layer = which == 1 ? 9 : 0;
    float x = -(pos.x - 17066.666f);
    float y = -(pos.y - 17066.666f) * 0.00375f;
    int32_t cellY = CMath::fint_(y - 0.5f);
    x = x * 0.00375f;
    int32_t cellX = CMath::fint_(x - 0.5f);

    CMapArea* area = CMap::m_areaTable[((cellX >> 1) << 6) + (cellY >> 1)];

    if (!area || area->m_asyncObject || !area->m_flyingBbox) {
        return 0;
    }

    int16_t* bounds = area->m_flyingBbox;
    float dx = pos.x - area->m_topLeft2.x;
    float dy = pos.y - area->m_topLeft2.y;
    uint32_t triangle;

    if (cellY & 1) {
        if (cellX & 1) {
            triangle = dy < dx ? 4 : 5;
        } else {
            triangle = dy < -533.33331f - dx ? 3 : 2;
        }
    } else {
        if (cellX & 1) {
            triangle = dy < -533.33331f - dx ? 6 : 7;
        } else {
            triangle = dy < dx ? 1 : 0;
        }
    }

    uint32_t a = s_flightTriangles[triangle][0];
    uint32_t b = s_flightTriangles[triangle][1];
    uint32_t c = s_flightTriangles[triangle][2];

    float ax = s_flightVertices[a][0];
    float ay = s_flightVertices[a][1];
    float az = static_cast<float>(bounds[layer + a]);
    float bx = s_flightVertices[b][0];
    float by = s_flightVertices[b][1];
    float bz = static_cast<float>(bounds[layer + b]);
    float cx = s_flightVertices[c][0];
    float cy = s_flightVertices[c][1];
    float cz = static_cast<float>(bounds[layer + c]);

    float e1x = bx - ax;
    float e1y = by - ay;
    float e1z = bz - az;
    float e2x = cx - ax;
    float e2y = cy - ay;
    float e2z = cz - az;

    C3Vector normal(e1y * e2z - e1z * e2y, e1z * e2x - e2z * e1x, e1x * e2y - e1y * e2x);
    normal.Normalize();

    *height = (normal.y * dy - (ax * normal.x + ay * normal.y + az * normal.z) + normal.x * dx) * (-1.0f / normal.z);
    return 1;
}

// VERIFY: aa1eecbc auto
// OFFSET: 0x007AD940
void CMap::QueryDoodadDefHitTests(TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> >* list, const C3Vector& start, const C3Vector& end) {
    for (CMapBaseObjLink* link = list->Head(); link; link = list->Next(link)) {
        CMapDoodadDef* doodadDef = static_cast<CMapDoodadDef*>(link->m_owner);

        if (doodadDef->m_unkCounter == CMap::s_queryTag || doodadDef->m_collisionGuid) {
            continue;
        }

        if ((doodadDef->m_flags & 0x100) || !(doodadDef->m_flags & 0x80)) {
            continue;
        }

        CM2Model* model = doodadDef->m_model;

        if (!model) {
            continue;
        }

        if (start.x > doodadDef->m_bbox.t.x || start.x < doodadDef->m_bbox.b.x || start.y > doodadDef->m_bbox.t.y || start.y < doodadDef->m_bbox.b.y || end.z > doodadDef->m_bbox.t.z || start.z < doodadDef->m_bbox.b.z) {
            continue;
        }

        if (model->m_fFlags & 1) {
            if (!model->m_hitTestPrev) {
                model->m_hitTestPrev = &model->m_scene->m_hitTestList;
                model->m_hitTestNext = model->m_scene->m_hitTestList;
                model->m_scene->m_hitTestList = model;

                if (model->m_hitTestNext) {
                    model->m_hitTestNext->m_hitTestPrev = &model->m_hitTestNext;
                }
            }

            model->m_hitTestMode = 3;
            model->m_hitTestGroup = 0;
            model->m_hitTestOwner = doodadDef;
        }

        doodadDef->m_unkCounter = CMap::s_queryTag;
    }
}

// VERIFY: 50db1a05 manual
// OFFSET: 0x007ADA80
void CMap::VectorIntersectMapObjs(CMapChunk* chunk, const C3Vector& start, const C3Vector& end, float* t) {
    float invHeight = 1.0f / (start.z - end.z);

    if (chunk) {
        float hitT = 1.0f;

        for (CMapBaseObjLink* link = chunk->m_mapObjDefLinkList.Head(); link; link = chunk->m_mapObjDefLinkList.Next(link)) {
            CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(link->m_owner);

            if (mapObjDef->m_flags & 0x500) {
                continue;
            }

            CMapObj* mapObj = mapObjDef->m_owner;

            if (!mapObj) {
                continue;
            }

            C3Vector localStart = start * mapObjDef->m_invMat;
            C3Vector localEnd = end * mapObjDef->m_invMat;

            if (mapObj->TestBounds(localStart, localEnd)) {
                float normalDot = 0.0f;

                if (mapObj->VectorIntersect(localStart, localEnd, 0x220122, 0, &hitT, nullptr, mapObjDef, &normalDot, nullptr) && hitT < *t) {
                    *t = hitT;

                    if (normalDot >= 0.0f) {
                        *t = hitT - ((1.0f - normalDot * invHeight) * invHeight + invHeight);

                        if (*t < 0.0f) {
                            *t = 0.0f;
                        }
                    }
                }
            }

            for (CMapBaseObjLink* groupLink = mapObjDef->m_mapObjDefGroupLinkList.Head(); groupLink; groupLink = mapObjDef->m_mapObjDefGroupLinkList.Next(groupLink)) {
                CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(groupLink->m_owner);

                if (start.x <= group->m_bbox.t.x && start.x >= group->m_bbox.b.x && start.y <= group->m_bbox.t.y && start.y >= group->m_bbox.b.y && end.z <= group->m_bbox.t.z && start.z >= group->m_bbox.b.z) {
                    CMap::QueryDoodadDefHitTests(&group->m_doodadDefLinkList, start, end);
                }
            }
        }

        return;
    }

    float hitT = 1.0f;

    for (CMapBaseObjLink* link = CMap::mapObjDefLinkList.Head(); link; link = CMap::mapObjDefLinkList.Next(link)) {
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(link->m_owner);

        if (mapObjDef->m_flags & 0x100) {
            continue;
        }

        CMapObj* mapObj = mapObjDef->m_owner;

        if (!mapObj) {
            continue;
        }

        C3Vector localStart = start * mapObjDef->m_invMat;
        C3Vector localEnd = end * mapObjDef->m_invMat;

        if (mapObj->TestBounds(localStart, localEnd)) {
            float normalDot = 0.0f;

            if (mapObj->VectorIntersect(const_cast<C3Vector&>(start), const_cast<C3Vector&>(end), 0x220122, 0, &hitT, nullptr, mapObjDef, &normalDot, nullptr) && hitT < *t) {
                *t = hitT;

                if (normalDot >= 0.0f) {
                    *t = hitT - ((1.0f - normalDot * invHeight) * invHeight + invHeight);

                    if (*t < 0.0f) {
                        *t = 0.0f;
                    }
                }
            }
        }

        for (CMapBaseObjLink* groupLink = mapObjDef->m_mapObjDefGroupLinkList.Head(); groupLink; groupLink = mapObjDef->m_mapObjDefGroupLinkList.Next(groupLink)) {
            CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(groupLink->m_owner);

            if (start.x <= group->m_bbox.t.x && start.x >= group->m_bbox.b.x && start.y <= group->m_bbox.t.y && start.y >= group->m_bbox.b.y && end.z <= group->m_bbox.t.z && start.z >= group->m_bbox.b.z) {
                CMap::QueryDoodadDefHitTests(&group->m_doodadDefLinkList, start, end);
            }
        }
    }
}

// VERIFY: c6ec57f9 manual
// OFFSET: 0x007ADE10
bool CMap::QueryHeight(const C3Vector& position, float range, float* height) {
    if (range < 0.01f) {
        return false;
    }

    CMap::s_queryTag++;
    float bestHeight = -100000.0f;
    bool found = false;
    CMapChunk* chunk = nullptr;

    if (!CMap::bDungeon) {
        float chunkY = -(position.y - 17066.666f) * 0.24f;
        float chunkX = 0.24f * -(position.x - 17066.666f);
        int32_t cellY = CMath::fint_(chunkY - 0.5f);
        int32_t cellX = CMath::fint_(chunkX - 0.5f);
        CMapArea* area = CMap::m_areaTable[64 * ((cellX >> 7) & 0x3F) + ((cellY >> 7) & 0x3F)];

        if (!area || area->m_asyncObject) {
            return false;
        }

        chunk = area->m_mapChunks[((cellY >> 3) & 0xF) + 16 * ((cellX >> 3) & 0xF)];

        if (chunk) {
            float liquidHeight = 0.0f;
            C2Vector chunkPos(chunkY, chunkX);
            C3Vector pos = position;
            found = CMap::GetHeightTerrain(chunk, pos, cellY, cellX, &bestHeight);

            if (chunk->GetLiquidHeight(chunkPos, &liquidHeight) && liquidHeight > bestHeight) {
                bestHeight = liquidHeight;
            }
        }
    }

    C3Vector top(position.x, position.y, position.z + range);
    C3Vector bottom(position.x, position.y, position.z - range);
    World::s_m2Scene->BeginHitTest();
    float t = 1.0f;
    CMap::VectorIntersectMapObjs(chunk, top, bottom, &t);

    if (chunk) {
        CMap::QueryDoodadDefHitTests(&chunk->m_doodadDefLinkList, top, bottom);
    }

    World::s_m2Scene->EndHitTestCollisionWorld(top, bottom, t);

    if (t >= 1.0f) {
        if (found) {
            *height = bestHeight;
            return true;
        }

        return false;
    }

    *height = top.z - t * (range + range);

    if (bestHeight > *height) {
        *height = bestHeight;
    }

    return true;
}

// VERIFY: 5c4ccdae manual
// OFFSET: 0x0079FCC0
void CMap::RenderFootprints() {
    if (!(CWorld::enables & 0x400)) {
        return;
    }

    uint32_t now = FrameTime::s_curTimeMs;
    uint32_t index = 0;
    CMapFootprint* footprints = &s_footprints[0][0];

    for (uint32_t n = 576; n; n--) {
        CMapFootprint* footprint = &footprints[index];

        if (footprint->m_vertices.Count()) {
            uint32_t elapsed = now - footprint->m_startTime;
            float fade = 1.0f - static_cast<float>(elapsed) * 0.00016666666f;

            if (fade >= 0.0f) {
                float alpha = fade * 255.0f;
                int32_t value = CMath::fint_(alpha - 0.5f);
                footprint->m_alpha = value < 127 ? CMath::fint_(alpha - 0.5f) : 127;
                CMapFootprintTexture& texture = s_footprintTextures.m_data[footprint->m_textureIndex];
                texture.m_footprintIndices.Add(1, &index);
                texture.m_vertexCount += footprint->m_vertices.Count();
            } else {
                footprint->m_vertices.SetCount(0);
            }
        }

        if (++index >= 576) {
            index = 0;
        }
    }

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsSet(GxRs_Texture1, ProjectTex2dGetModFadeTex());
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[2]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_PolygonOffset, CWorldParam::cvar_footstepBias->m_floatValue * 0.0039063096f);

    C44Matrix world;
    world.d0 = -CWorldScene::s_activeWorldView.x;
    world.d1 = -CWorldScene::s_activeWorldView.y;
    world.d2 = -CWorldScene::s_activeWorldView.z;
    g_theGxDevicePtr->XformPushAndSet(GxXform_World, world);

    for (uint32_t t = 0; t < s_footprintTextures.Count(); t++) {
        CMapFootprintTexture& texture = s_footprintTextures.m_data[t];

        if (texture.m_vertexCount) {
            CGxTex* gxTex = TextureGetGxTex(texture.m_texture, 0, nullptr);

            if (gxTex) {
                CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 44, texture.m_vertexCount);
                float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(vertexBuf));
                CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, texture.m_vertexCount);
                uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
                uint16_t next = 0;

                for (uint32_t k = 0; k < texture.m_footprintIndices.Count(); k++) {
                    CMapFootprint* footprint = &footprints[texture.m_footprintIndices.m_data[k]];
                    CImVector color;
                    color.b = 0xFF;
                    color.g = 0xFF;
                    color.r = 0xFF;
                    color.a = static_cast<uint8_t>(footprint->m_alpha);

                    for (uint32_t v = 0; v < footprint->m_vertices.Count(); v++) {
                        CGxVertexPT0T1& src = footprint->m_vertices.m_data[v];
                        vertices[0] = src.m_position.x;
                        vertices[1] = src.m_position.y;
                        vertices[2] = src.m_position.z;
                        vertices[3] = 0.0f;
                        vertices[4] = 0.0f;
                        vertices[5] = 1.0f;
                        *reinterpret_cast<CImVector*>(&vertices[6]) = color;
                        vertices[7] = src.m_texture0.x;
                        vertices[8] = src.m_texture0.y;
                        vertices[9] = src.m_texture1.x;
                        vertices[10] = src.m_texture1.y;
                        *indices = next++;
                        vertices += 11;
                        indices++;
                    }
                }

                g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
                vertexBuf->m_hasData = 1;
                g_theGxDevicePtr->BufUnlock(indexBuf, 0);
                indexBuf->m_hasData = 1;
                GxPrimVertexPtr(vertexBuf, GxVBF_PNCT2);
                g_theGxDevicePtr->PrimIndexPtr(indexBuf);
                g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);

                CGxBatch batch;
                batch.m_primType = GxPrim_Triangles;
                batch.m_start = 0;
                batch.m_count = texture.m_vertexCount;
                batch.m_minIndex = 0;
                batch.m_maxIndex = static_cast<uint16_t>(texture.m_vertexCount - 1);
                g_theGxDevicePtr->Draw(&batch, 1);
            }
        }

        texture.m_footprintIndices.m_count = 0;
        texture.m_vertexCount = 0;
    }

    GxXformPop(GxXform_World);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 331b297f manual
// OFFSET: 0x007C3830
void CMap::Unload() {
    CMap::PurgeMaps();
    CMap::s_lowDetail.Unload();

    for (CMapBaseObjLink* link = CMap::mapObjDefLinkList.Head(); link;) {
        CMapBaseObjLink* next = CMap::mapObjDefLinkList.Next(link);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(link->m_owner);
        CMap::FreeBaseObjLink(link);
        CMap::PurgeMapObjDef(mapObjDef);
        link = next;
    }

    for (CMapBaseObjLink* link = CMap::s_unk_ADFBF8.Head(); link;) {
        CMapBaseObjLink* next = CMap::s_unk_ADFBF8.Next(link);
        CMapBaseObj* owner = link->m_owner;
        CMap::FreeBaseObjLink(link);

        if (!(owner->m_type & 0x200)) {
            CMapDoodadDef::Purge(static_cast<CMapDoodadDef*>(owner));
        }

        link = next;
    }

    CDestructibleProxy* proxy = CMap::s_destructibleProxyList.Head();

    while (!CMap::s_destructibleProxyList.IsEnd(proxy)) {
        CDestructibleProxy* next = CMap::s_destructibleProxyList.RawNext(proxy);

        if (proxy->m_uniqueId) {
            CMap::s_destructibleProxyList.UnlinkNode(proxy);
            CMap::s_destructibleProxyList.DeleteNode(proxy);
        }

        proxy = next;
    }

    CMap::s_destructibleProxyList.Clear();
    CMapObj::ClearCache(1);
    World::s_m2Scene->PurgeModelCache();
    CMap::ClearAllFootprints();
    CDetailDoodad::FreeGxBufMat();
    CMapRenderChunk::Destroy();
    CMapObj::DestroyPools();
    TextureGxCacheFlush();
    g_theGxDevicePtr->DeviceEvictResources();
    DayNight::UnloadMap();
    CMap::DestroyLight(CMap::s_mapLight);
    CMap::FreeAllChunkLiquids();
    CMap::bActive = 0;
    CMap::bDungeon = 0;
    CMap::enableChunkBatching = 0;
    char path[260];
    SStrCopy(path, CMap::wdtFilename, 0x7FFFFFFF);
    SStrCopy(strrchr(path, '.'), ".tex", 0x7FFFFFFF);
    TextureUnloadBlob(path);
}

// VERIFY: 6b4c3b7f auto
// OFFSET: 0x0079D460
void CMap::CreateWaterRipple(const C3Vector& pos, float a2, float a3, float a4, float a5, float a6, int32_t a7, int32_t a8) {
    if (CWorld::s_waterRipples && !World::s_waterLOD) {
        float alpha = 1.0f;

        if (a5 < 0.16666667f) {
            alpha = a5 * 6.0f;
        }

        WaterRipple::Create(pos, a2, a3, alpha, a4, a6, a7, a8);
    }
}

// VERIFY: b6f5c768 auto
// OFFSET: 0x007B4A50
void CMap::SnapBaseObjToSubChunk(CMapBaseObj* baseObj, const C3Vector* position, float facing) {
    C44Matrix rotation = C44Matrix::RotationAroundZ(facing);
    CAaBox bounds(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f));

    if (baseObj->m_type & 0x8) {
        static_cast<CMapObjDef*>(baseObj)->m_owner->GetBounds(&bounds);
    } else {
        bounds = static_cast<CMapStaticEntity*>(baseObj)->m_model->GetBoundingBox();
    }

    float centerX = (bounds.t.x + bounds.b.x) * 0.5f;
    float centerY = (bounds.t.y + bounds.b.y) * 0.5f;
    float centerZ = (bounds.t.z + bounds.b.z) * 0.5f;
    bounds.t.x = bounds.t.x - centerX;
    bounds.t.y = bounds.t.y - centerY;
    bounds.t.z = bounds.t.z - centerZ;
    bounds.b.x = bounds.b.x - centerX;
    bounds.b.y = bounds.b.y - centerY;
    bounds.b.z = bounds.b.z - centerZ;
    CAaBox rotated(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f));
    CWorldMath::TransformAABox(rotation, bounds, rotated);
    C3Vector* snapped = const_cast<C3Vector*>(position);
    float x = snapped->x + rotated.t.x;
    float y = snapped->y + rotated.t.y;
    int32_t cellX = CMath::fint_((x + 4.1666665f) * 0.24f - 0.5f);
    int32_t cellY = CMath::fint_((y + 4.1666665f) * 0.24f - 0.5f);
    snapped->x = snapped->x - (x - cellX * 4.1666665f);
    snapped->y = snapped->y - (y - 4.1666665f * cellY);
}

// VERIFY: 153db93c manual
// OFFSET: 0x007B58C0
void CMap::UpdateDoodadDef(CMapDoodadDef* doodadDef, const C3Vector* position, float facing) {
    doodadDef->m_position = *position;
    doodadDef->m_scale = 1.0f;
    doodadDef->m_mat = C44Matrix();
    doodadDef->m_mat.d0 = position->x;
    doodadDef->m_mat.d1 = position->y;
    doodadDef->m_mat.d2 = position->z;
    doodadDef->m_mat.RotateAroundZ(facing);
    CMap::UpdateDoodadDefBounds(doodadDef);
}

// VERIFY: 05946402 manual
// OFFSET: 0x007B5870
void CMap::UpdateDoodadDefTransform(CMapDoodadDef* doodadDef, const C44Matrix& matrix) {
    doodadDef->m_position.x = matrix.d0;
    doodadDef->m_position.y = matrix.d1;
    doodadDef->m_position.z = matrix.d2;
    doodadDef->m_scale = sqrtf(matrix.a2 * matrix.a2 + matrix.a1 * matrix.a1 + matrix.a0 * matrix.a0);
    doodadDef->m_mat = matrix;
    CMap::UpdateDoodadDefBounds(doodadDef);
}

// VERIFY: 7fce3415 manual
// OFFSET: 0x0079F760
void CMap::BuildFootprintMatrix(C44Matrix& matrix, const float* size, int32_t left, float facing) {
    if (left) {
        matrix.Scale(C3Vector(-1.0f, 1.0f, 1.0f));
    }

    if (size[1] <= size[0]) {
        if (size[0] > size[1]) {
            matrix.Scale(C3Vector(size[0] / size[1], 1.0f, 1.0f));
        }
    } else {
        matrix.Scale(C3Vector(1.0f, size[1] / size[0], 1.0f));
    }

    matrix.RotateAroundZ(-facing);
}

// VERIFY: a2657405 manual
// OFFSET: 0x0079FA70
void CMap::NewFootprint(int32_t textureId, const float* size, const C3Vector* position, float facing, int32_t left, int32_t terrainType, int32_t activePlayer) {
    if (static_cast<uint32_t>(textureId) >= s_footprintTextures.Count()) {
        return;
    }

    TerrainTypeRec* rec = g_terrainTypeDB.GetRecord(terrainType);

    if (!rec || !(rec->m_flags & 0x1)) {
        return;
    }

    CAaBox box;
    box.b = C3Vector(0.0f, 0.0f, 0.0f);
    box.t = C3Vector(0.0f, 0.0f, 0.0f);
    CWorldMath::MakeAaBox(box, *position, size);

    if (!World::GetFacets(&box, &s_footprintFacets, 0x200122, nullptr)) {
        return;
    }

    CMapFootprint* footprint;

    if (activePlayer) {
        footprint = &s_footprints[0][0] + s_nextPlayerFootprint++;

        if (s_nextPlayerFootprint >= 64) {
            s_nextPlayerFootprint = 0;
        }
    } else {
        footprint = &s_footprints[0][0] + s_nextFootprint++;

        if (s_nextFootprint >= 576) {
            s_nextFootprint = 64;
        }
    }

    footprint->m_link.Unlink();
    footprint->m_position = *position;
    footprint->m_startTime = FrameTime::s_curTimeMs;
    footprint->m_textureIndex = textureId;
    C44Matrix texMatrix;
    C44Matrix fadeMatrix;
    C44Matrix rotation;
    CMap::BuildFootprintMatrix(rotation, size, left, facing);
    ProjectTex2dMakeMatrices(texMatrix, fadeMatrix, box, &rotation, 0.5f, 1);
    footprint->m_vertices.SetCount(3 * s_footprintFacets.facets.m_count);
    CGxVertexPT0T1* vertex = footprint->m_vertices.m_data;

    for (uint32_t i = 0; i < s_footprintFacets.facets.m_count; i++) {
        CFacet& facet = s_footprintFacets.facets.m_data[i];

        for (uint32_t j = 0; j < 3; j++) {
            vertex->m_position = facet.v[j];
            vertex->m_texture0 = C2Vector(facet.v[j] * texMatrix);
            vertex->m_texture1 = C2Vector(facet.v[j] * fadeMatrix);
            vertex++;
        }
    }
}

// VERIFY: 599297b5 manual
// OFFSET: 0x007A6260
bool CMap::GetTriSubchunk(CMapChunk* chunk, const CiRect* rect, const CAaBox* box) {
    int32_t base = 16 * rect->minY + rect->minX + rect->minY;
    C3Vector* vertices = &CMapChunk::vertexList[base];
    float* height = &chunk->m_height[base];
    int32_t rowSkip = 17 - (rect->maxX - rect->minX + 1);
    World::TriData::Batch* batch = nullptr;
    uint16_t* indices = nullptr;
    bool found = false;

    for (int32_t y = rect->minY; y <= rect->maxY; y++, vertices += rowSkip, height += rowSkip) {
        for (int32_t x = rect->minX; x <= rect->maxX; x++, vertices++, height++) {
            if (chunk->m_header->m_holes & CMap::s_holeMask[4 * (y >> 1) + (x >> 1)]) {
                continue;
            }

            uint8_t outcodes[20];

            for (uint32_t i = 0; i < 5; i++) {
                int32_t index = s_subVertexIndexFlat[i];
                vertices[index].z = height[index];
                outcodes[index] = static_cast<uint8_t>(CWorldMath::ComputeAaBoxOutcode(const_cast<CAaBox*>(box), &vertices[index]));
            }

            uint16_t vertexIndex = static_cast<uint16_t>(vertices - CMapChunk::vertexList);

            for (uint32_t tri = 0; tri < 12; tri += 3) {
                const int32_t* triangle = &s_subVertexTriangles[tri];

                if (outcodes[triangle[0]] & outcodes[triangle[1]] & outcodes[triangle[2]]) {
                    continue;
                }

                if (!found) {
                    batch = World::TriData::AllocBatch();

                    if (batch) {
                        batch->m_def = reinterpret_cast<CMapObjDef*>(chunk);
                        C44Matrix* matrix;

                        if (World::TriData::matrixPoolCount + 1 < 32) {
                            matrix = &World::TriData::matrixPool[World::TriData::matrixPoolCount++];
                        } else {
                            World::TriData::statusFlags |= 1;
                            matrix = nullptr;
                        }

                        matrix->a0 = 1.0f;
                        matrix->a1 = 0.0f;
                        matrix->a2 = 0.0f;
                        matrix->a3 = 0.0f;
                        matrix->b0 = 0.0f;
                        matrix->b1 = 1.0f;
                        matrix->b2 = 0.0f;
                        matrix->b3 = 0.0f;
                        matrix->c0 = 0.0f;
                        matrix->c1 = 0.0f;
                        matrix->c2 = 1.0f;
                        matrix->c3 = 0.0f;
                        matrix->d0 = chunk->m_topLeftCoords.x;
                        matrix->d1 = chunk->m_topLeftCoords.y;
                        matrix->d2 = chunk->m_topLeftCoords.z;
                        matrix->d3 = 1.0f;
                        batch->m_matrix = matrix;
                        batch->m_vertexList = CMapChunk::vertexList;
                        batch->m_heightList = chunk->m_height;
                        batch->m_normalList = nullptr;
                        indices = World::TriData::AllocIndices(12 * (rect->maxX - rect->minX + 1) * (rect->maxY - rect->minY + 1));
                        batch->m_indices = indices;
                    }
                }

                found = true;

                if (!batch || !batch->m_indices || !batch->m_vertexList) {
                    continue;
                }

                for (uint32_t i = 0; i < 3; i++) {
                    uint16_t index = vertexIndex + static_cast<uint16_t>(triangle[i]);
                    *indices++ = index;

                    if (batch->m_minVertexIndex >= index) {
                        batch->m_minVertexIndex = index;
                    }

                    if (batch->m_maxVertexIndex <= index) {
                        batch->m_maxVertexIndex = index;
                    }
                }

                batch->m_indexCount += 3;
                batch->m_faceCount++;
            }
        }
    }

    return found;
}

// VERIFY: c11b2d43 manual
// OFFSET: 0x007A6630
bool CMap::GetTrisChunk(int32_t chunkX, int32_t chunkY, const CiRect& cellRect, const CAaBox& box, World::TriData* triData, uint32_t flags) {
    CMapArea* area = CMap::m_areaTable[64 * ((chunkY >> 4) & 0x3F) + ((chunkX >> 4) & 0x3F)];

    if (!area || area->m_asyncObject) {
        return false;
    }

    CMapChunk* chunk = area->m_mapChunks[(chunkX & 0xF) + 16 * (chunkY & 0xF)];

    if (!chunk) {
        return false;
    }

    CAaBox localBox = box;
    localBox.b.x = localBox.b.x - chunk->m_topLeftCoords.x;
    localBox.b.y = localBox.b.y - chunk->m_topLeftCoords.y;
    localBox.b.z = localBox.b.z - chunk->m_topLeftCoords.z;
    localBox.t.x = localBox.t.x - chunk->m_topLeftCoords.x;
    localBox.t.y = localBox.t.y - chunk->m_topLeftCoords.y;
    localBox.t.z = localBox.t.z - chunk->m_topLeftCoords.z;
    bool result = false;
    static CiRect s_chunkRect(0, 0, 7, 7);
    CiRect rect;
    rect.minY = cellRect.minY - 8 * chunkY;
    rect.minX = cellRect.minX - 8 * chunkX;
    rect.maxY = cellRect.maxY - 8 * chunkY;
    rect.maxX = cellRect.maxX - 8 * chunkX;
    rect &= s_chunkRect;

    if (flags & 0x100) {
        result = CMap::GetTriSubchunk(chunk, &rect, &localBox);
    }

    if (flags & 0x30000) {
        bool oceanOnly = (flags & 0x10000) && !(flags & 0x20000);

        for (CChunkLiquid* liquid = chunk->m_liquidChunkLinkList.Head(); liquid; liquid = chunk->m_liquidChunkLinkList.Next(liquid)) {
            if (oceanOnly && !(g_liquidTypeDB.GetRecord(liquid->m_type)->m_flags & 0x4)) {
                continue;
            }

            result |= liquid->GetTris(triData, localBox, rect);
        }
    }

    return result;
}

// VERIFY: bd067831 manual
// OFFSET: 0x007A6830
bool CMap::GetTrisTerrain(const CAaBox& box, World::TriData* triData, uint32_t flags) {
    bool result = false;
    CiRect cellRect;
    cellRect.minY = CMath::fint_(-(box.t.x - 17066.666f) * 0.23999999f - 0.5f);
    cellRect.minX = CMath::fint_(-(box.t.y - 17066.666f) * 0.23999999f - 0.5f);
    cellRect.maxY = CMath::fint_(-(box.b.x - 17066.666f) * 0.23999999f - 0.5f);
    cellRect.maxX = CMath::fint_(-(box.b.y - 17066.666f) * 0.23999999f - 0.5f);

    for (int32_t chunkY = cellRect.minY >> 3; chunkY <= cellRect.maxY >> 3; chunkY++) {
        for (int32_t chunkX = cellRect.minX >> 3; chunkX <= cellRect.maxX >> 3; chunkX++) {
            result |= CMap::GetTrisChunk(chunkX, chunkY, cellRect, box, triData, flags);
        }
    }

    return result;
}

// VERIFY: 24d73f6d manual
// OFFSET: 0x007A6940
bool CMap::QueryTrisMapObjDefs(const CAaBox& box, World::TriData* triData, uint32_t flags) {
    CAaBox localBox;
    localBox.b = C3Vector(0.0f, 0.0f, 0.0f);
    localBox.t = C3Vector(0.0f, 0.0f, 0.0f);
    C3Vector center((box.b.x + box.t.x) * 0.5f, (box.t.y + box.b.y) * 0.5f, 0.5f * (box.t.z + box.b.z));
    CAaBox extents;
    extents.b = box.b - center;
    extents.t = box.t - center;
    bool result = false;

    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.Next(mapObjDef)) {
        if (mapObjDef->m_flags & 0x20) {
            continue;
        }

        CMapObj* mapObj = mapObjDef->m_owner;

        if (!mapObj) {
            continue;
        }

        C3Vector localCenter = center * mapObjDef->m_invMat;
        C33Matrix rotation(mapObjDef->m_invMat);
        CWorldMath::TransformAABox(rotation, extents, localBox);
        localBox.b += localCenter;
        localBox.t += localCenter;

        if (mapObj->TestBounds(localBox)) {
            result |= mapObj->GetTris(localBox, flags, triData, mapObjDef);
        }
    }

    return result;
}

// VERIFY: 87b59ea2 auto
// OFFSET: 0x007A6AF0
bool CMap::QueryTris(const CAaBox& box, World::TriData* triData, uint32_t flags) {
    bool result = false;

    if (flags & 0x300F0) {
        result = CMap::QueryTrisMapObjDefs(box, triData, flags);
    }

    if (flags & 0x30100) {
        return CMap::GetTrisTerrain(box, triData, flags) | result;
    }

    return result;
}

// VERIFY: 0e38b513 manual
// OFFSET: 0x007A2AA0
uint32_t CMap::QueryIntersectingModels(CM2Model** models, uint32_t maxModels, const CAaBox& box, uint32_t flags) {
    if (!maxModels) {
        return 0;
    }

    uint32_t count = 0;

    for (CMapObjDef* mapObjDef = CMap::mapObjDefHash.m_fulllist.Head(); mapObjDef; mapObjDef = CMap::mapObjDefHash.Next(mapObjDef)) {
        if ((mapObjDef->m_flags & 0x100) || !mapObjDef->m_bbox.Intersects(box)) {
            continue;
        }

        CMapObj* mapObj = mapObjDef->m_owner;

        for (CMapBaseObjLink* link = mapObjDef->m_mapObjDefGroupLinkList.Head(); link; link = mapObjDef->m_mapObjDefGroupLinkList.Next(link)) {
            CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(link->m_owner);

            if (!mapObj->IsGroupLoaded(defGroup->m_groupNum) || !defGroup->m_bbox.Intersects(box)) {
                continue;
            }

            for (CMapBaseObjLink* doodadLink = defGroup->m_doodadDefLinkList.Head(); doodadLink; doodadLink = defGroup->m_doodadDefLinkList.Next(doodadLink)) {
                CMapStaticEntity* doodadDef = static_cast<CMapStaticEntity*>(doodadLink->m_owner);
                CM2Model* model = doodadDef->m_model;

                if (!model) {
                    continue;
                }

                uint32_t doodadFlags = doodadDef->m_flags;

                if (!(doodadFlags & 0x80) || !doodadDef->m_bbox.Intersects(box)) {
                    continue;
                }

                uint32_t match = (flags & 0x200122) != 0;

                if (flags & 0x2000000) {
                    match |= doodadFlags & 0x1000;
                }

                if (flags & 0x100111) {
                    match |= (doodadFlags & 0x100) == 0;
                }

                if (match) {
                    models[count++] = model;

                    if (count == maxModels) {
                        return count;
                    }
                }
            }
        }
    }

    return count;
}

// VERIFY: d3ebe146 manual
// OFFSET: 0x007B5740
void CMap::UpdateDoodadDefBounds(CMapDoodadDef* doodadDef) {
    CM2Model* model = doodadDef->m_model;
    doodadDef->m_flags = static_cast<MapObjFlag>(doodadDef->m_flags | MAPOBJ_FLAG_UNPLACED);

    if (model) {
        if (model->IsLoaded(0, 0)) {
            doodadDef->UpdateBounds();
            doodadDef->m_flags = static_cast<MapObjFlag>(doodadDef->m_flags | MAPOBJ_FLAG_PREPARED);
        } else {
            doodadDef->m_bbox = doodadDef->m_model->m_shared->m_boundingBox;
            doodadDef->m_bbox.b.x = doodadDef->m_position.x + doodadDef->m_bbox.b.x;
            doodadDef->m_bbox.b.y = doodadDef->m_bbox.b.y + doodadDef->m_position.y;
            doodadDef->m_bbox.b.z = doodadDef->m_position.z + doodadDef->m_bbox.b.z;
            doodadDef->m_bbox.t.x = doodadDef->m_position.x + doodadDef->m_bbox.t.x;
            doodadDef->m_bbox.t.y = doodadDef->m_bbox.t.y + doodadDef->m_position.y;
            doodadDef->m_bbox.t.z = doodadDef->m_position.z + doodadDef->m_bbox.t.z;
            doodadDef->m_sphere.c = doodadDef->m_position;
            float dz = doodadDef->m_bbox.t.z - doodadDef->m_bbox.b.z;
            float dy = doodadDef->m_bbox.t.y - doodadDef->m_bbox.b.y;
            float dx = doodadDef->m_bbox.t.x - doodadDef->m_bbox.b.x;
            doodadDef->m_sphere.r = sqrtf(dx * dx + (dy * dy + dz * dz));
        }

        CM2Model* current = doodadDef->m_model;
        current->m_fFlags |= 0x8000;
        current->m_worldTransform = doodadDef->m_mat;
    }

    doodadDef->m_staticFlags &= ~0x1u;
    doodadDef->m_flags = static_cast<MapObjFlag>((doodadDef->m_flags & 0xFFFFFFF8) | MAPOBJ_FLAG_UNPLACED);
    CMap::LinkStaticEntity(doodadDef);
    doodadDef->ExtendChunkBounds();
    CMap::doodadDefList.LinkToTail(doodadDef);
}
