#include "MapObjRead.h"
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
#include "WoW/Source/Extension/ExtensionMgr.h"
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
    SIffChunk* chunk = reinterpret_cast<SIffChunk*>(static_cast<uint8_t*>(this->m_pWmoData) + 12);
    this->m_header = reinterpret_cast<SMOHeader*>(&chunk->m_data);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_textureNameList = reinterpret_cast<char*>(&chunk->m_data);
    this->m_texturesSize = chunk->m_size;

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_materialList = reinterpret_cast<SMOMaterial*>(&chunk->m_data);
    this->m_materialsCount = chunk->m_size / sizeof(SMOMaterial);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_groupNameList = reinterpret_cast<char*>(&chunk->m_data);
    this->m_groupNameSize = chunk->m_size;

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_groupInfo = reinterpret_cast<SMOGroupInfo*>(&chunk->m_data);
    this->m_groupInfoCount = chunk->m_size / sizeof(SMOGroupInfo);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_skybox = reinterpret_cast<char*>(&chunk->m_data);

    if (!this->m_skybox[0]) {
        this->m_skybox = nullptr;
    }

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);

    if (!this->m_skybox) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_groupInfoCount); i++) {
            this->m_groupInfo[i].m_flags &= ~0x40000;
        }
    }

    this->m_portalVertexList = reinterpret_cast<C3Vector*>(&chunk->m_data);
    this->m_planeVertCount = chunk->m_size / sizeof(C3Vector);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_portalList = reinterpret_cast<SMOPortal*>(&chunk->m_data);
    this->m_portalsCount = chunk->m_size / sizeof(SMOPortal);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);

    for (int32_t i = 0; i < this->m_portalsCount; i++) {
        if (std::isnan(static_cast<double>(this->m_portalList[i].m_plane.d))) {
            this->m_portalList[i].m_plane.n.x = 0.0f;
            this->m_portalList[i].m_plane.n.y = 0.0f;
            this->m_portalList[i].m_plane.n.z = 1.0f;
            this->m_portalList[i].m_plane.d = 800000.0f;
        }
    }

    this->m_portalRefList = reinterpret_cast<SMOPortalRef*>(&chunk->m_data);
    this->m_portalRefCount = chunk->m_size / sizeof(SMOPortalRef);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_visBlockVertList = reinterpret_cast<C3Vector*>(&chunk->m_data);
    this->m_visBlockVertCount = chunk->m_size / sizeof(C3Vector);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_visBlockList = reinterpret_cast<SMOVisibleBlock*>(&chunk->m_data);
    this->m_visBlockCount = chunk->m_size / sizeof(SMOVisibleBlock);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_lightList = reinterpret_cast<SMOLight*>(&chunk->m_data);
    this->m_ligtsCount = chunk->m_size / sizeof(SMOLight);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_doodadSetList = reinterpret_cast<SMODoodadSet*>(&chunk->m_data);
    this->m_doodadSetCount = chunk->m_size / sizeof(SMODoodadSet);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_doodadNameList = reinterpret_cast<char*>(&chunk->m_data);
    this->m_doodadNameSize = chunk->m_size;

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_doodadDefList = reinterpret_cast<SMODoodadDef*>(&chunk->m_data);
    this->m_doodadDefCount = chunk->m_size / sizeof(SMODoodadDef);

    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);
    this->m_fogList = reinterpret_cast<SMOFog*>(&chunk->m_data);
    this->m_fogsCount = chunk->m_size / sizeof(SMOFog);

    uint8_t* end = static_cast<uint8_t*>(this->m_pWmoData) + this->m_wmoFileSize;
    chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(&chunk->m_data) + chunk->m_size);

    if (reinterpret_cast<uint8_t*>(chunk) < end && chunk->m_token == 0x4D435650) {
        this->m_convexVolumePlanes = reinterpret_cast<C4Plane*>(&chunk->m_data);
        this->m_convexVolumePlaneCount = chunk->m_size / sizeof(C4Plane);
    }
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
    mapObj->Load();
    mapObj->CreateMaterials();
    mapObj->m_argbColor = *reinterpret_cast<CImVector*>(&mapObj->m_header->m_ambColor);
    mapObj->m_bbox = mapObj->m_header->m_boundingBox;
    mapObj->m_mapObjGroupCount = mapObj->m_groupInfoCount;

    for (uint32_t i = 0; i < static_cast<uint32_t>(mapObj->m_groupInfoCount); i++) {
        mapObj->m_mapObjGroupArray[i] = CMap::AllocMapObjGroup();
    }

    mapObj->m_isGroupLoaded = 1;

    // Not in the original: the root WMO is parsed and its groups allocated, none loaded.
    sExtensionMgr->OnMapObjLoaded(mapObj);
}

// VERIFY: 737e6a62 manual
// OFFSET: 0x007D80C0
bool CMapObj::Read(char* fileName) {
    SStrCopy(this->m_wmoName, fileName, STORM_MAX_STR);

    SFileImpl* file = nullptr;

    if (CMap::SafeOpen(fileName, &file)) {
        SStrCopy(this->m_wmoName, fileName, STORM_MAX_STR);
    } else {
        CMap::SafeOpen("world\\wmo\\Dungeon\\test\\missingwmo.wmo", &file);
        SStrCopy(this->m_wmoName, "world\\wmo\\Dungeon\\test\\missingwmo.wmo", STORM_MAX_STR);
    }

    this->m_wmoFileSize = SFileGetFileSize(file, nullptr);
    this->m_pWmoData = SMemAlloc(this->m_wmoFileSize, __FILE__, __LINE__, 0);

    this->m_asyncObject = AsyncFileReadAllocObject();
    this->m_asyncObject->m_file = file;
    this->m_asyncObject->m_userArg = this;
    this->m_asyncObject->m_buffer = this->m_pWmoData;
    this->m_asyncObject->m_size = this->m_wmoFileSize;
    this->m_asyncObject->m_userPostloadCallback = reinterpret_cast<void*>(CMapObj::PostloadCallback);
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

    // Not in the original: the group is parsed (asynchronous read path).
    sExtensionMgr->OnMapObjGroupLoaded(group);
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

        // Not in the original: the group is parsed (synchronous preload path).
        sExtensionMgr->OnMapObjGroupLoaded(group);

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
