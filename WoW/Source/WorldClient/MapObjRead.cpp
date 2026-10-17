#include "MapObjRead.h"
#include "WmoData.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "Engine/Source/Tempest/C3Ray.h"
#include "WoW/Common/AaBsp.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include <cmath>
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"

// STATIC: 0x00ADFC4C
TSList<CMapObj, TSGetExplicitLink<CMapObj> > CMapObj::s_asyncLoadQueue = TSExplicitList<CMapObj, offsetof(CMapObj, m_asyncLoadLink)>();

// STATIC: 0x00ADFC58
TSList<CMapObjGroup, TSGetExplicitLink<CMapObjGroup> > CMapObj::s_loadedGroups = TSExplicitList<CMapObjGroup, offsetof(CMapObjGroup, m_asyncLoadLink)>();

// VERIFY: 477d94a9 manual
// OFFSET: 0x007D72D0
void CMapObj::CreateMaterials() {
    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_materialsCount); i++) {
        this->m_materialList[i].Textures()[0] = nullptr;
        this->m_materialList[i].Textures()[1] = nullptr;
    }
}

// VERIFY: 12bdf795 manual
// OFFSET: 0x007D7380
void CMapObjGroup::FixColorVertexAlpha() {
    uint32_t firstExterior = this->m_transparencyBatchesCount ? this->m_batchList[this->m_transparencyBatchesCount - 1].m_vertexEnd + 1 : 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_colorVertexListSize); i++) {
        CImVector& color = this->m_colorVertexList[i];

        if (i >= firstExterior) {
            uint32_t alpha = color.a;
            uint32_t r = (color.r + ((alpha * color.r) >> 6)) >> 1;
            uint32_t g = (color.g + ((alpha * color.g) >> 6)) >> 1;
            uint32_t b = (color.b + ((alpha * color.b) >> 6)) >> 1;
            color.r = r > 0xFF ? 0xFF : r;
            color.g = g > 0xFF ? 0xFF : g;
            color.b = b > 0xFF ? 0xFF : b;
            color.a = 0xFF;
        } else {
            color.r >>= 1;
            color.g >>= 1;
            color.b >>= 1;
        }
    }
}

// VERIFY: cd3ffadf manual
// OFFSET: 0x007D7470
void CMapObj::Load() {
    static_assert(sizeof(SMOMaterial) == 64, "WMO material disk layout changed");
    auto data = std::make_unique<WmoData>(this->m_pWmoData, this->m_wmoFileSize);
    auto& headerChunk = data->Require(0x4D4F4844);
    if (headerChunk.Count<SMOHeader>() != 1) throw std::runtime_error("Invalid WMO root header");
    auto* header = headerChunk.Records<SMOHeader>();
    auto& texturesChunk = data->Require(0x4D4F5458);
    auto* textures = texturesChunk.Records<char>();
    const auto texturesCount = texturesChunk.Count<char>();
    auto& materialsChunk = data->Require(0x4D4F4D54);
    auto* materials = materialsChunk.Records<SMOMaterial>();
    const auto materialsCount = materialsChunk.Count<SMOMaterial>();
    auto& groupNamesChunk = data->Require(0x4D4F474E);
    auto* groupNames = groupNamesChunk.Records<char>();
    const auto groupNamesCount = groupNamesChunk.Count<char>();
    auto& groupsChunk = data->Require(0x4D4F4749);
    auto* groups = groupsChunk.Records<SMOGroupInfo>();
    const auto groupsCount = groupsChunk.Count<SMOGroupInfo>();
    auto& portalVerticesChunk = data->Require(0x4D4F5056);
    auto* portalVertices = portalVerticesChunk.Records<C3Vector>();
    const auto portalVerticesCount = portalVerticesChunk.Count<C3Vector>();
    auto& portalsChunk = data->Require(0x4D4F5054);
    auto* portals = portalsChunk.Records<SMOPortal>();
    const auto portalsCount = portalsChunk.Count<SMOPortal>();
    auto& portalRefsChunk = data->Require(0x4D4F5052);
    auto* portalRefs = portalRefsChunk.Records<SMOPortalRef>();
    const auto portalRefsCount = portalRefsChunk.Count<SMOPortalRef>();
    auto& visibleVerticesChunk = data->Require(0x4D4F5656);
    auto* visibleVertices = visibleVerticesChunk.Records<C3Vector>();
    const auto visibleVerticesCount = visibleVerticesChunk.Count<C3Vector>();
    auto& visibleBlocksChunk = data->Require(0x4D4F5642);
    auto* visibleBlocks = visibleBlocksChunk.Records<SMOVisibleBlock>();
    const auto visibleBlocksCount = visibleBlocksChunk.Count<SMOVisibleBlock>();
    auto& lightsChunk = data->Require(0x4D4F4C54);
    auto* lights = lightsChunk.Records<SMOLight>();
    const auto lightsCount = lightsChunk.Count<SMOLight>();
    auto& doodadSetsChunk = data->Require(0x4D4F4453);
    auto* doodadSets = doodadSetsChunk.Records<SMODoodadSet>();
    const auto doodadSetsCount = doodadSetsChunk.Count<SMODoodadSet>();
    auto& doodadNamesChunk = data->Require(0x4D4F444E);
    auto* doodadNames = doodadNamesChunk.Records<char>();
    const auto doodadNamesCount = doodadNamesChunk.Count<char>();
    auto& doodadsChunk = data->Require(0x4D4F4444);
    auto* doodads = doodadsChunk.Records<SMODoodadDef>();
    const auto doodadsCount = doodadsChunk.Count<SMODoodadDef>();
    auto& fogsChunk = data->Require(0x4D464F47);
    auto* fogs = fogsChunk.Records<SMOFog>();
    const auto fogsCount = fogsChunk.Count<SMOFog>();
    auto& skyChunk = data->Require(0x4D4F5342);
    if (skyChunk.size) skyChunk.CheckString(0);
    char* skybox = skyChunk.size && *skyChunk.Records<char>() ? skyChunk.Records<char>() : nullptr;
    auto* convexChunk = data->Find(0x4D435650);
    auto* convex = convexChunk ? convexChunk->Records<C4Plane>() : nullptr;
    auto convexCount = convexChunk ? convexChunk->Count<C4Plane>() : 0;

    // Authored patch WMOs (including Stormwind and Dalaran) retain stale MOHD
    // counts. The original loader derives runtime counts from the actual chunks.
    if (groupsCount > 512) throw std::runtime_error("WMO exceeds the original group capacity");
    for (uint32_t i = 0; i < materialsCount; ++i) {
        texturesChunk.CheckString(materials[i].m_texture1);
        texturesChunk.CheckString(materials[i].m_texture2);
        // Runtime texture handles must never be read from serialized padding.
        materials[i].Textures()[0] = nullptr;
        materials[i].Textures()[1] = nullptr;
    }
    for (uint32_t i = 0; i < groupsCount; ++i) {
        if (groups[i].m_nameoffset != UINT32_MAX) groupNamesChunk.CheckString(groups[i].m_nameoffset);
        if (!skybox) groups[i].m_flags &= ~0x40000;
    }
    for (uint32_t i = 0; i < portalsCount; ++i) {
        auto& portal = portals[i];
        if (portal.m_startVertex > portalVerticesCount || portal.m_count > portalVerticesCount - portal.m_startVertex)
            throw std::runtime_error("WMO portal vertices are out of bounds");
        // Preserve the original client's repair for an undefined portal plane.
        if (std::isnan(static_cast<double>(portal.m_plane.d))) {
            portal.m_plane.n = C3Vector(0, 0, 1);
            portal.m_plane.d = 800000.0f;
        }
    }
    for (uint32_t i = 0; i < portalRefsCount; ++i)
        if (portalRefs[i].m_portalIndex >= portalsCount || portalRefs[i].m_groupIndex >= groupsCount)
            throw std::runtime_error("WMO portal reference is out of bounds");
    for (uint32_t i = 0; i < visibleBlocksCount; ++i)
        if (visibleBlocks[i].m_firstVertex > visibleVerticesCount || visibleBlocks[i].m_count > visibleVerticesCount - visibleBlocks[i].m_firstVertex)
            throw std::runtime_error("WMO visibility vertices are out of bounds");
    for (uint32_t i = 0; i < doodadSetsCount; ++i)
        if (doodadSets[i].m_startIdx > doodadsCount || doodadSets[i].m_count > doodadsCount - doodadSets[i].m_startIdx)
            throw std::runtime_error("WMO doodad set is out of bounds");
    for (uint32_t i = 0; i < doodadsCount; ++i) doodadNamesChunk.CheckString(doodads[i].m_flags & 0xFFFFFF);

    // Publish only after all checks pass. The original root bytes remain intact;
    // runtime pointer slots, flags and plane repair live in the aligned copy.
    this->m_header = header;
    this->m_textureNameList = textures; this->m_texturesSize = texturesCount;
    this->m_materialList = materials; this->m_materialsCount = materialsCount;
    this->m_groupNameList = groupNames; this->m_groupNameSize = groupNamesCount;
    this->m_groupInfo = groups; this->m_groupInfoCount = groupsCount;
    this->m_portalVertexList = portalVertices; this->m_planeVertCount = portalVerticesCount;
    this->m_portalList = portals; this->m_portalsCount = portalsCount;
    this->m_portalRefList = portalRefs; this->m_portalRefCount = portalRefsCount;
    this->m_visBlockVertList = visibleVertices; this->m_visBlockVertCount = visibleVerticesCount;
    this->m_visBlockList = visibleBlocks; this->m_visBlockCount = visibleBlocksCount;
    this->m_lightList = lights; this->m_ligtsCount = lightsCount;
    this->m_doodadSetList = doodadSets; this->m_doodadSetCount = doodadSetsCount;
    this->m_doodadNameList = doodadNames; this->m_doodadNameSize = doodadNamesCount;
    this->m_doodadDefList = doodads; this->m_doodadDefCount = doodadsCount;
    this->m_fogList = fogs; this->m_fogsCount = fogsCount;
    this->m_skybox = skybox;
    this->m_convexVolumePlanes = convex; this->m_convexVolumePlaneCount = convexCount;
    this->m_rootData = std::move(data);
}

// VERIFY: 4e3ee2fd auto
// OFFSET: 0x007D7710
void CMapObj::CreateMaterial(uint8_t texture) {
    SMOMaterial& material = this->m_materialList[texture];

    if (material.Textures()[0]) {
        return;
    }

    const char* texture1 = &this->m_textureNameList[material.m_texture1];
    const char* texture2 = &this->m_textureNameList[material.m_texture2];

    if (!*texture1) {
        texture1 = "createcrappygreentexture.blp";
    }

    if (!CShaderEffect::s_enableShaders) {
        texture2 = "";
    }

    switch (material.m_shader) {
        case 3:
        case 5:
        case 6:
            if (*texture2) {
                break;
            }

            material.m_shader = 4;
            texture2 = nullptr;
            break;

        case 0:
        case 1:
        case 2:
        case 4:
            texture2 = nullptr;
            break;

        default:
            break;
    }

    material.Textures()[0] = CMap::LoadTexture(texture1);

    if (texture2) {
        material.Textures()[1] = CMap::LoadTexture(texture2);
    } else {
        material.Textures()[1] = nullptr;
    }
}

// VERIFY: 03cc8920 manual
// OFFSET: 0x007D78C0
CMapObjGroup* CMapObj::AttenTransVerts(CMapObjGroup* group) {
    group->m_unkLoadedFlag |= 0x2;

    if (!group->m_transparencyBatchesCount) {
        return group;
    }

    uint32_t vertexEnd = group->m_batchList[group->m_transparencyBatchesCount - 1].m_vertexEnd;

    for (uint32_t i = 0; i <= vertexEnd; i++) {
        C3Vector& vert = group->m_vertexList[i];
        CImVector& color = group->m_colorVertexList[i];
        float atten = 0.0f;

        if (group->m_portalCount) {
            uint32_t p;

            for (p = 0; p < static_cast<uint32_t>(group->m_portalCount); p++) {
                SMOPortalRef& ref = this->m_portalRefList[p + group->m_portalStart];
                SMOPortal& portal = this->m_portalList[ref.m_portalIndex];
                C3Vector onPlane = vert;
                float side = portal.m_plane.n.z * vert.z + portal.m_plane.n.y * vert.y + portal.m_plane.n.x * vert.x + portal.m_plane.d;

                if (side > 0.001f) {
                    C3Vector end(vert.x - portal.m_plane.n.x, vert.y - portal.m_plane.n.y, vert.z - portal.m_plane.n.z);
                    NTempest::Intersect(C3Ray::FromStartEnd(vert, end, false), portal.m_plane, nullptr, &onPlane, 0.01f);
                } else if (side < -0.001f) {
                    C3Vector end(portal.m_plane.n.x + vert.x, portal.m_plane.n.y + vert.y, portal.m_plane.n.z + vert.z);
                    NTempest::Intersect(C3Ray::FromStartEnd(vert, end, false), portal.m_plane, nullptr, &onPlane, 0.01f);
                }

                C3Vector* portalVerts = &this->m_portalVertexList[portal.m_startVertex];
                float dist;

                if (NTempest::Intersect(onPlane, portalVerts, portal.m_count, portal.m_plane.n.MajorAxis())) {
                    dist = portal.m_plane.n.z * vert.z + portal.m_plane.n.y * vert.y + portal.m_plane.n.x * vert.x + portal.m_plane.d;

                    if (ref.m_side != 1) {
                        dist = dist * -1.0f;
                    }
                } else {
                    dist = NTempest::DistanceFromPolygonEdge(vert, portalVerts, portal.m_count);
                }

                if (this->m_groupInfo[ref.m_groupIndex].m_flags & 0x48) {
                    if (dist < 0.0f) {
                        dist = 0.0f;
                    }

                    float weight = 1.0f - dist * 0.15f;

                    if (weight > 0.001f) {
                        atten = weight + atten;
                    }
                } else if (dist > -1.0f && dist < 1.0f) {
                    atten = 0.0f;
                    break;
                }
            }

            if (p == static_cast<uint32_t>(group->m_portalCount)) {
                if (atten <= 0.001f) {
                    atten = 0.0f;
                } else if (atten > 1.0f) {
                    atten = 1.0f;
                }
            }
        }

        color.r = static_cast<uint8_t>(atten * (127.0f - color.r) + color.r);
        color.g = static_cast<uint8_t>(color.g + (127.0f - color.g) * atten);
        color.b = static_cast<uint8_t>(color.b + (127.0f - color.b) * atten);
        color.a = static_cast<uint8_t>(atten * 255.0f);
    }

    return group;
}

// VERIFY: 5789fe57 manual
// OFFSET: 0x007D7C30
void CMapObjGroup::CreateOptionalDataPointers(SIffChunk* dataChunk) {
    SIffChunk* chunk = dataChunk;
    uint32_t flags = this->m_flags;

    if (flags & 0x200) {
        this->m_lightRefList = reinterpret_cast<uint16_t*>(&chunk->m_data);
        this->m_lightRefListCount = chunk->m_size >> 1;
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    }

    if (flags & 0x800) {
        this->m_doodadRefList = reinterpret_cast<uint16_t*>(&chunk->m_data);
        this->m_doodadRefListCount = chunk->m_size >> 1;
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    }

    if (flags & 0x1) {
        uint32_t nodeSize = chunk->m_size;
        CAaBspNode* nodes = reinterpret_cast<CAaBspNode*>(&chunk->m_data);
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + nodeSize);
        uint32_t faceSize = chunk->m_size;
        uint16_t* faces = reinterpret_cast<uint16_t*>(&chunk->m_data);
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + faceSize);
        this->m_cAaBspNodePtr1.Set(nodes, nodeSize >> 4, faces, faceSize >> 1, this->m_bbox);
    }

    if (this->m_flags & 0x400) {
        for (uint32_t i = 0; i < 4; i++) {
            chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
        }
    }

    if (this->m_flags & 0x4) {
        CMapObj* parent = this->m_parent;
        this->m_colorVertexList = reinterpret_cast<CImVector*>(&chunk->m_data);
        this->m_colorVertexListSize = chunk->m_size >> 2;
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);

        if (!(parent->m_header->m_flags & 0x8)) {
            this->FixColorVertexAlpha();
        }
    }

    if (this->m_flags & 0x1000) {
        uint32_t* header = reinterpret_cast<uint32_t*>(&chunk->m_data);
        this->m_liquidVerts.x = header[0];
        this->m_liquidVerts.y = header[1];
        this->m_liquidTiles.x = header[2];
        this->m_liquidTiles.y = header[3];
        this->m_liquidCorner = *reinterpret_cast<C3Vector*>(&header[4]);

        uint16_t materialId = *reinterpret_cast<uint16_t*>(&header[7]);
        uint8_t* data = reinterpret_cast<uint8_t*>(&header[7]) + 2;
        this->m_liquidVertexList = reinterpret_cast<SMOLiquidVert*>(data);
        data += 8 * this->m_liquidVerts.x * this->m_liquidVerts.y;
        this->m_liquidTileList = reinterpret_cast<SMOLTile*>(data);
        data += this->m_liquidTiles.x * this->m_liquidTiles.y;
        chunk = reinterpret_cast<SIffChunk*>(data);
        this->m_liquidMaterialId = materialId;

        if (!this->m_liquidType) {
            this->m_liquidType = this->GetLiquidType(this->FindLiquidType());
        }

        CMapObjGroup::AllocVertArray(&this->m_liquidVertArray, this->m_liquidVerts.x * this->m_liquidVerts.y);
        this->GenLiquidVerts();
    }

    if (this->m_flags & 0x20000) {
        this->m_stripIndices = reinterpret_cast<uint16_t*>(&chunk->m_data);
        this->m_stripIndexCount = chunk->m_size >> 1;
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size + 8);
        this->m_stripBatches = reinterpret_cast<uint8_t*>(chunk);

        if (CMap::s_enableTriangleStrips) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_batchListCount); i++) {
                this->m_batchList[i].m_indexStart = *reinterpret_cast<uint32_t*>(&this->m_stripBatches[8 * i]);
                this->m_batchList[i].m_indexCount = *reinterpret_cast<uint16_t*>(&this->m_stripBatches[8 * i + 4]);
            }
        }
    }

    flags = this->m_flags;

    if (flags & 0x2000000) {
        this->m_textureVertexList2 = reinterpret_cast<C2Vector*>(&chunk->m_data);
        this->m_textureVertexList2Count = chunk->m_size >> 3;
        chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    }

    if (flags & 0x1000000) {
        this->m_colorVertexListExtra = reinterpret_cast<CImVector*>(&chunk->m_data);
        this->m_colorVertexListExtraSize = chunk->m_size >> 2;
    }
}

// VERIFY: 182a9a50 manual
// OFFSET: 0x007D7F50
void CMapObjGroup::CreateDataPointers(SIffChunk* dataChunk) {
    SIffChunk* chunk = dataChunk;
    this->m_polyList = reinterpret_cast<SMOPoly*>(&chunk->m_data);
    this->m_polyListSize = chunk->m_size >> 1;

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_indices = reinterpret_cast<uint16_t*>(&chunk->m_data);
    this->m_indicesCount = chunk->m_size >> 1;

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_vertexList = reinterpret_cast<C3Vector*>(&chunk->m_data);
    this->m_vertexListCount = chunk->m_size / sizeof(C3Vector);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_normalList = reinterpret_cast<C3Vector*>(&chunk->m_data);
    this->m_normalListCount = chunk->m_size / sizeof(C3Vector);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_textureVertexList = reinterpret_cast<C2Vector*>(&chunk->m_data);
    this->m_textureVertexListCount = chunk->m_size >> 3;

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_batchList = reinterpret_cast<SMOBatch*>(&chunk->m_data);
    this->m_batchListCount = chunk->m_size / sizeof(SMOBatch);

    this->CreateOptionalDataPointers(reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size));
}

// VERIFY: aea1ef5e manual
// OFFSET: 0x007D8050
void CMapObj::PostloadCallback(void* arg) {
    CMapObj* mapObj = static_cast<CMapObj*>(arg);

    AsyncFileReadDestroyObject(mapObj->m_asyncObject);
    mapObj->m_asyncObject = nullptr;
    mapObj->m_asyncLoadLink.Unlink();
    try {
        mapObj->Load();
        mapObj->CreateMaterials();
        mapObj->m_argbColor = *reinterpret_cast<CImVector*>(&mapObj->m_header->m_ambColor);
        mapObj->m_bbox = mapObj->m_header->m_boundingBox;
        mapObj->m_mapObjGroupCount = 0;
        for (uint32_t i = 0; i < static_cast<uint32_t>(mapObj->m_groupInfoCount); i++) {
            auto* group = CMap::AllocMapObjGroup();
            if (!group) throw std::bad_alloc();
            mapObj->m_mapObjGroupArray[mapObj->m_mapObjGroupCount++] = group;
        }
        mapObj->m_isGroupLoaded = 1;
    } catch (...) {
        mapObj->Clear();
        mapObj->m_rootReadFailed = true;
        throw;
    }
}

static void RootReadFailed(void* arg) {
    auto* mapObj = static_cast<CMapObj*>(arg);
    AsyncFileReadDestroyObject(mapObj->m_asyncObject);
    mapObj->m_asyncObject = nullptr;
    mapObj->m_asyncLoadLink.Unlink();
    mapObj->Clear();
    mapObj->m_rootReadFailed = true;
}

// VERIFY: 737e6a62 manual
// OFFSET: 0x007D80C0
bool CMapObj::Read(char* fileName) {
    this->m_rootReadFailed = false;
    SStrCopy(this->m_wmoName, fileName, STORM_MAX_STR);

    SFileImpl* file = nullptr;

    if (CMap::SafeOpen(fileName, &file)) {
        SStrCopy(this->m_wmoName, fileName, STORM_MAX_STR);
    } else {
        if (!CMap::SafeOpen("world\\wmo\\Dungeon\\test\\missingwmo.wmo", &file)) return false;
        SStrCopy(this->m_wmoName, "world\\wmo\\Dungeon\\test\\missingwmo.wmo", STORM_MAX_STR);
    }

    const uint32_t size = SFileGetFileSize(file, nullptr);
    if (size < 12 || size > WmoData::MaxBytes) {
        SFileCloseFile(file);
        throw std::runtime_error("Invalid WMO byte size or resource budget exceeded");
    }
    this->m_wmoFileSize = size;
    this->m_pWmoData = SMemAlloc(this->m_wmoFileSize, __FILE__, __LINE__, 0);

    this->m_asyncObject = AsyncFileReadAllocObject();
    this->m_asyncObject->m_file = file;
    this->m_asyncObject->m_userArg = this;
    this->m_asyncObject->m_buffer = this->m_pWmoData;
    this->m_asyncObject->m_size = this->m_wmoFileSize;
    this->m_asyncObject->m_userPostloadCallback = reinterpret_cast<void*>(CMapObj::PostloadCallback);
    this->m_asyncObject->m_userPostUnloadCallback = reinterpret_cast<void*>(RootReadFailed);
    this->m_asyncObject->m_priority = 124;
    AsyncFileReadObject(this->m_asyncObject, 0);

    CMapObj::s_asyncLoadQueue.LinkToTail(this);

    return true;
}

// VERIFY: a3a005c2 manual
// OFFSET: 0x007D81C0
void CMapObjGroup::HandleAntiportal() {
    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_polyListSize); i++) {
        C3Vector* verts[3];
        verts[0] = &this->m_vertexList[this->m_indices[3 * i]];
        verts[1] = &this->m_vertexList[this->m_indices[3 * i + 1]];
        verts[2] = &this->m_vertexList[this->m_indices[3 * i + 2]];

        float height = (verts[1]->z + verts[2]->z + verts[0]->z) * 0.33333334f;
        uint32_t above[3];
        uint32_t count = 0;

        if (height < verts[0]->z) {
            above[count++] = 0;
        }

        if (height < verts[1]->z) {
            above[count++] = 1;
        }

        if (height < verts[2]->z) {
            above[count++] = 2;
        }

        if (count >= 2) {
            CMapObjOccluder* occluder = CMapObj::AllocOccluder();
            occluder->m_start = *verts[above[0]];
            occluder->m_end = *verts[above[1]];
            this->m_occluderList.LinkToTail(occluder);
        }
    }

    this->m_extBatchCount = 0;
    this->m_intBatchCount = 0;
}

// VERIFY: 579978be manual
// OFFSET: 0x007D82E0
void CMapObjGroup::Create() {
    SMOGroupHeader* header = reinterpret_cast<SMOGroupHeader*>(static_cast<uint8_t*>(this->m_filePtr) + 20);
    this->m_parent->m_mapObjGroupList.LinkToTail(this);

    CMapObj* parent = this->m_parent;
    this->m_groupName = &parent->m_groupNameList[header->m_groupName];
    this->m_flags = header->m_flags;

    if (!parent->m_skybox) {
        this->m_flags &= ~0x40000;
    }

    this->m_bbox = header->m_boundingBox;
    this->m_portalStart = header->m_portalStart;
    this->m_portalCount = header->m_portalCount;
    this->m_transparencyBatchesCount = header->m_transBatchCount;
    this->m_intBatchCount = header->m_intBatchCount;
    this->m_extBatchCount = header->m_extBatchCount;
    memcpy(this->m_fogs, header->m_fogIds, sizeof(this->m_fogs));
    this->m_wmoGroupId = header->m_uniqueID;

    if (parent->m_header->m_flags & 0x4) {
        this->m_liquidType = this->GetLiquidType(header->m_groupLiquid);
    } else {
        this->m_liquidType = this->GetLiquidType(header->m_groupLiquid == 15 ? 0 : header->m_groupLiquid + 1);
    }

    this->CreateDataPointers(reinterpret_cast<SIffChunk*>(header + 1));

    CMapObj* mapObj = this->m_parent;

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_batchListCount); i++) {
        mapObj->CreateMaterial(this->m_batchList[i].m_texture);
    }

    this->m_unkLoadedFlag = (this->m_unkLoadedFlag & ~0x2) | 0x1;

    if (mapObj->m_header->m_flags & 0x1) {
        this->m_unkLoadedFlag |= 0x2;
    }

    if (!SStrCmp(this->m_groupName, "antiportal", STORM_MAX_STR)) {
        this->HandleAntiportal();
    }

    this->m_unkLoadedFlag |= 0x4;

    uint32_t batchCount = this->m_batchListCount;

    if (!batchCount) {
        return;
    }

    this->m_indexMin = 0xFFFFFFFF;
    this->m_indexMax = 0;
    this->m_vertexMin = 0xFFFF;
    this->m_vertexMax = 0;

    for (uint32_t i = 0; i < batchCount; i++) {
        SMOBatch& batch = this->m_batchList[i];

        if (mapObj->m_materialList[batch.m_texture].m_blendMode) {
            this->m_unkLoadedFlag &= ~0x4;
            break;
        }

        uint32_t indexEnd = batch.m_indexCount + batch.m_indexStart - 1;

        if (this->m_vertexMax < batch.m_vertexEnd) {
            this->m_vertexMax = batch.m_vertexEnd;
        }

        if (this->m_vertexMin > batch.m_vertexStart) {
            this->m_vertexMin = batch.m_vertexStart;
        }

        if (this->m_indexMin > batch.m_indexStart) {
            this->m_indexMin = batch.m_indexStart;
        }

        if (this->m_indexMax < indexEnd) {
            this->m_indexMax = indexEnd;
        }
    }

    for (uint32_t i = 0; i < batchCount; i++) {
        if (mapObj->m_materialList[this->m_batchList[i].m_texture].m_shader == 6) {
            this->m_unkLoadedFlag |= 0x8;
            return;
        }
    }
}

// VERIFY: 2026a068 auto
// OFFSET: 0x007D8570
void CMapObjGroup::AsyncPostloadCallback(void* arg) {
    CMapObjGroup* group = static_cast<CMapObjGroup*>(arg);
    AsyncFileReadDestroyObject(group->m_asyncObjPtr);
    group->m_asyncObjPtr = nullptr;
    group->m_asyncLoadLink.Unlink();
    group->Create();
}

// VERIFY: 1fdd96d9 manual
// OFFSET: 0x007D85E0
void CMapObj::ReadGroup(uint32_t index, bool preLoad) {
    CMapObjGroup* group = this->m_mapObjGroupArray[index];

    char fileName[256];
    strcpy(fileName, this->m_wmoName);
    SStrPrintf(strrchr(fileName, '.'), sizeof(fileName), "_%03d", index);
    strcat(fileName, ".wmo");

    SFileImpl* file = nullptr;
    CMap::SafeOpen(fileName, &file);

    group->m_fileSize = SFileGetFileSize(file, nullptr);
    group->m_filePtr = SMemAlloc(group->m_fileSize, __FILE__, __LINE__, 0);
    group->m_parent = this;

    if (preLoad) {
        CMap::SafeRead(fileName, file, group->m_filePtr, group->m_fileSize);
        group->Create();
        SFileCloseFile(file);
        return;
    }

    CAsyncObject* object = AsyncFileReadAllocObject();
    object->m_file = file;
    object->m_buffer = group->m_filePtr;
    object->m_size = group->m_fileSize;
    object->m_userArg = group;
    object->m_userPostloadCallback = reinterpret_cast<void*>(CMapObjGroup::AsyncPostloadCallback);
    group->m_asyncObjPtr = object;
    AsyncFileReadObject(object, 0);

    CMapObj::s_loadedGroups.LinkToTail(group);
}
