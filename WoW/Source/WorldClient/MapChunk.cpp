#include "MapChunk.h"
#include "WoW/Source/WorldClient/WorldConstants.h"
#include "WoW/Source/WorldClient/DetailDoodad.h"
#include "WoW/Source/WorldClient/CDetailDoodadData.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/Tempest/C3Ray.h"
#include "Engine/Source/Tempest/NTempest.h"
#include <cmath>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gxu/GxuLight.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CFacet.h"
#include "Engine/Source/Tempest/CFrustum.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LiquidMaterialRec.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/CWorldMath.h"
#include "WoW/Source/WorldClient/CWorldOcclusion.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/MapRenderChunk.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "Engine/Source/Model2/M2Light.h"

// STATIC: 0x00A3FBB8
static const uint16_t s_bitMask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

// STATIC: 0x00A3FBC8
static const uint32_t s_bitShift[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

// STATIC: 0x00D25488
float CMapChunk::s_geoToTex;

// STATIC: 0x00D25498
C3Vector CMapChunk::vertexList[145];

// STATIC: 0x00AEEE3C
const int32_t CMapChunk::cornerVertexIndex[4] = { 0, 8, 136, 144 };

// STATIC: 0x00CD8F38
int32_t CMapChunk::farCornerIndex;

// STATIC: 0x00CD8F3C
int32_t CMapChunk::nearCornerIndex;

// STATIC: 0x00A405E8
static const int32_t s_subVertexTriangles[12] = { 17, 9, 0, 9, 1, 0, 9, 17, 18, 9, 18, 1 };

// STATIC: 0x00A40618
static const int32_t s_subVertexIndexFlat[5] = { 0, 9, 17, 1, 18 };

// VERIFY: ed4a5d5b manual
// OFFSET: 0x00790520
void CMapChunk::GetVertexPosition(int32_t index, C3Vector* pos) {
    pos->x = CMapChunk::vertexList[index].x + this->m_topLeftCoords.x;
    pos->y = CMapChunk::vertexList[index].y + this->m_topLeftCoords.y;
    pos->z = this->m_height[index] + this->m_topLeftCoords.z;
}

// VERIFY: d0968beb manual
// OFFSET: 0x007C3370
void CMapChunk::Purge() {
    if (this->m_renderChunk) {
        CMap::FreeRenderChunk(this->m_renderChunk);
        this->m_renderChunk = nullptr;
    }

    if (this->m_detailDoodadInst) {
        CDetailDoodad::FreeInst(this->m_detailDoodadInst);
        this->m_detailDoodadInst = nullptr;
    }

    CChunkLiquid* liquid = this->m_liquidChunkLinkList.Head();

    while (liquid) {
        CChunkLiquid* next = this->m_liquidChunkLinkList.Next(liquid);
        liquid->m_chunkLink.Unlink();
        liquid->Purge();
        CMap::FreeChunkLiquid(liquid);
        liquid = next;
    }

    this->m_sortListLink.Unlink();

    for (CMapBaseObjLink* link = this->m_doodadDefLinkList.Head(); link;) {
        CMapBaseObjLink* next = this->m_doodadDefLinkList.Next(link);
        CMapDoodadDef* doodadDef = static_cast<CMapDoodadDef*>(link->m_owner);
        CMap::FreeBaseObjLink(link);

        if (!(doodadDef->m_flags & 0x200)) {
            CMapDoodadDef::Purge(doodadDef);
        }

        link = next;
    }

    for (CMapBaseObjLink* link = this->m_mapObjDefLinkList.Head(); link;) {
        CMapBaseObjLink* next = this->m_mapObjDefLinkList.Next(link);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(link->m_owner);
        CMap::FreeBaseObjLink(link);
        CMap::PurgeMapObjDef(mapObjDef);
        link = next;
    }

    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link;) {
        CMapBaseObjLink* next = this->m_parentLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    for (CMapBaseObjLink* link = this->m_entityLinkList.Head(); link;) {
        CMapBaseObjLink* next = this->m_entityLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    for (CMapBaseObjLink* link = this->m_lightLinkList.Head(); link;) {
        CMapBaseObjLink* next = this->m_lightLinkList.Next(link);
        CMap::FreeBaseObjLink(link);
        link = next;
    }

    for (SE2SoundKitObject* sound = this->m_soundHandleList.Head(); sound;) {
        SI2::StopAdvancedSound(sound->m_advancedId);
        sound = this->m_soundHandleList.DeleteNode(sound);
    }
}

// VERIFY: 3a512c10 auto
// OFFSET: 0x007C3DB0
void CMapChunk::SelectLights(CM2Lighting* lighting) {
    lighting->AddLight(&CMap::s_mapLight->m_light);

    if (CWorld::farFog > this->m_distToCamera && (this->m_flags & MAPOBJ_FLAG_UNPLACED)) {
        this->m_flags = static_cast<MapObjFlag>(this->m_flags & ~MAPOBJ_FLAG_UNPLACED);
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    C3Vector fogColor;
    fogColor.x = info->m_fog.m_color.r * 0.0039215689f;
    fogColor.y = info->m_fog.m_color.g * 0.0039215689f;
    fogColor.z = info->m_fog.m_color.b * 0.0039215689f;

    lighting->SetFog(fogColor, info->m_fog.m_start, info->m_fog.m_end, info->m_fog.m_density);
}

// VERIFY: 092514c8 manual
// OFFSET: 0x007C3A10
void CMapChunk::ProcessIffChunks(bool a3) {
    SIffChunk* headerChunk = reinterpret_cast<SIffChunk*>(this->m_chunkInfoBeginPtr);
    this->m_header = reinterpret_cast<SMChunk*>(&headerChunk->m_data);

    int32_t remaining = headerChunk->m_size - sizeof(SMChunk);
    SIffChunk* chunk = reinterpret_cast<SIffChunk*>(reinterpret_cast<uint8_t*>(this->m_header) + sizeof(SMChunk));

    while (remaining > 0) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&chunk->m_data);
        remaining -= offsetof(SIffChunk, m_data);

        switch (chunk->m_token) {
            case 0x4D434E52:
                this->m_normals = reinterpret_cast<int8_t*>(data);

                if (a3) {
                    chunk->m_size = 448;
                }

                break;

            case 0x4D435348:
                this->m_shadowMap = data;
                break;

            case 0x4D435654:
                this->m_height = reinterpret_cast<float*>(data);
                break;

            case 0x4D435246:
                this->m_mcrfPtr = data;
                break;

            case 0x4D435345:
                if (this->m_header->m_nSndEmitters) {
                    this->m_soundEmitters = reinterpret_cast<CWSoundEmitter*>(data);
                }

                break;

            case 0x4D434C59:
                this->m_layers = reinterpret_cast<SMLayer*>(data);
                break;

            case 0x4D434C51:
                if (this->m_header->m_sizeLiquid > 8) {
                    this->m_liquid = reinterpret_cast<SMLiquidChunk*>(data);
                    chunk->m_size = this->m_header->m_sizeLiquid - 8;
                }

                break;

            case 0x4D43414C:
                this->m_additionalShadowmap = data;

                if (a3) {
                    chunk->m_size = this->m_header->m_sizeAlpha - 8;
                }

                break;

            case 0x4D434356:
                this->m_vertexShading = reinterpret_cast<uint32_t*>(data);
                break;
        }

        remaining -= chunk->m_size;
        chunk = reinterpret_cast<SIffChunk*>(data + chunk->m_size);
    }
}

// VERIFY: 86d6cd8c manual
// OFFSET: 0x007C3C60
void CMapChunk::InitializeVertexGrid() {
    C3Vector* vertex = CMapChunk::vertexList;

    for (int32_t row = 0; row < 9; row++) {
        float x = row * -SMOLTILE_SIZE;

        for (int32_t col = 0; col < 9; col++) {
            vertex->x = x;
            vertex->y = 0.0f - col * SMOLTILE_SIZE;
            vertex++;
        }

        if (row < 8) {
            for (int32_t col = 0; col < 8; col++) {
                vertex->x = x - HALF_SMOLTILE_SIZE;
                vertex->y = 0.0f - col * SMOLTILE_SIZE - HALF_SMOLTILE_SIZE;
                vertex++;
            }
        }
    }
}

// VERIFY: 6ef0b88b auto
// OFFSET: 0x007C3D90
void CMapChunk::Initialize() {
    CMapRenderChunk::Initialize();
    CMapChunk::InitializeVertexGrid();
    CMapChunk::s_geoToTex = -1.0f / CMapChunk::vertexList[1].y;
}

// VERIFY: b5717665 auto
// OFFSET: 0x007C3A00
void CMapChunk::Destroy() {
    CMapRenderChunk::Destroy();
}


// VERIFY: 5994fc04 auto
// OFFSET: 0x007C3B40
void CMapChunk::AllocRenderChunkAndBatch() {
    this->m_renderChunk = CMap::AllocRenderChunk();
    this->m_renderChunk->AddBatch(this, nullptr, &this->m_topLeftCoords, 0);
}

// VERIFY: b0c09ba2 manual
// OFFSET: 0x007C3B60
int16_t CMapChunk::CreateIndices(char* buf, int32_t baseVertex) {
    uint16_t* indexDst = reinterpret_cast<uint16_t*>(buf);
    int16_t totalIndices = 0;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int holeMaskIndex = (row / 2) * 4 + (col / 2);
            if (this->m_header->m_holes & CMap::s_holeMask[holeMaskIndex])
                continue;

            uint16_t TL = baseVertex + col;
            uint16_t TR = baseVertex + col + 1;
            uint16_t C = baseVertex + col + 9;
            uint16_t BL = baseVertex + col + 17;
            uint16_t BR = baseVertex + col + 18;

            indexDst[0] = C;
            indexDst[1] = TL;
            indexDst[2] = BL;
            indexDst[3] = C;
            indexDst[4] = TR;
            indexDst[5] = TL;
            indexDst[6] = C;
            indexDst[7] = BR;
            indexDst[8] = TR;
            indexDst[9] = C;
            indexDst[10] = BL;
            indexDst[11] = BR;
            indexDst += 12;
            totalIndices += 12;
        }

        baseVertex += 17;
    }

    return totalIndices;
}

// VERIFY: 44a339cb manual
// OFFSET: 0x007C3E70
void __fastcall CWorldScene::AddMapChunk(CMapChunk* mapChunk) {
    if (CWorldScene::FrustumCull(&mapChunk->m_bbox)) {
        return;
    }

    C3Vector nearest(0.0f, 0.0f, 0.0f);
    CWorldScene::GetNearestCornerToCamera(&mapChunk->m_bbox, &nearest);
    float* height = mapChunk->m_height;
    mapChunk->m_distToCamera = CWorldScene::camPlane.n.z * nearest.z + CWorldScene::camPlane.n.y * nearest.y + CWorldScene::camPlane.n.x * nearest.x + CWorldScene::camPlane.d;

    int32_t index = CMapChunk::cornerVertexIndex[CMapChunk::farCornerIndex];
    C3Vector corner;
    corner.z = 0.0f;
    corner.x = CMapChunk::vertexList[index].x + mapChunk->m_topLeftCoords.x;
    corner.y = CMapChunk::vertexList[index].y + mapChunk->m_topLeftCoords.y;
    corner.z = height[index] + mapChunk->m_topLeftCoords.z;
    CWorldScene::AddMapChunkToRenderList(mapChunk, &corner);
}

// VERIFY: d44747a1 manual
// OFFSET: 0x007C3F30
void CMapChunk::CreateVerticesWorld(CGxVertexPN* v, const C3Vector& offset) {
    static float cols[9];
    static float rows[9];

    float left = 17066.666f - this->m_cOffset.x * 33.333332f;
    float top = 17066.666f - this->m_cOffset.y * 33.333332f;
    rows[0] = top;
    cols[0] = left;
    rows[1] = top - SMOLTILE_SIZE;
    rows[2] = top - 8.333333f;
    rows[3] = top - 12.5f;
    rows[4] = top - 16.666666f;
    rows[5] = top - 20.833332f;
    rows[6] = top - 25.0f;
    rows[7] = top - 29.166666f;
    cols[1] = left - SMOLTILE_SIZE;
    cols[2] = left - 8.333333f;
    cols[3] = left - 12.5f;
    cols[4] = left - 16.666666f;
    cols[5] = left - 20.833332f;
    cols[6] = left - 25.0f;
    cols[7] = left - 29.166666f;
    rows[8] = 17066.666f - 33.333332f * (this->m_cOffset.y + 1);
    cols[8] = 17066.666f - (this->m_cOffset.x + 1) * 33.333332f;

    float* height = this->m_height;
    int8_t* normals = this->m_normals;

    for (int32_t row = 0; row < 9; row++) {
        for (int32_t col = 0; col < 9; col++) {
            v->m_position.x = rows[row];
            v->m_position.y = cols[col];
            v->m_position.z = this->m_topLeftCoords.z + height[col];
            v->m_normal.x = normals[3 * col + 0] * 0.0078740157f;
            v->m_normal.y = normals[3 * col + 1] * 0.0078740157f;
            v->m_normal.z = normals[3 * col + 2] * 0.0078740157f;
            v++;
        }

        height += 9;
        normals += 27;

        if (row < 8) {
            for (int32_t col = 0; col < 8; col++) {
                v->m_position.x = rows[row] - HALF_SMOLTILE_SIZE;
                v->m_position.y = cols[col] - HALF_SMOLTILE_SIZE;
                v->m_position.z = this->m_topLeftCoords.z + height[col];
                v->m_normal.x = normals[3 * col + 0] * 0.0078740157f;
                v->m_normal.y = normals[3 * col + 1] * 0.0078740157f;
                v->m_normal.z = normals[3 * col + 2] * 0.0078740157f;
                v++;
            }

            height += 8;
            normals += 24;
        }
    }
}

// VERIFY: 6a9c0ba2 manual
// OFFSET: 0x007C4620
void CMapChunk::CreateVerticesWorld(CGxVertexPNC* v, const C3Vector& offset) {
    static float cols[9];
    static float rows[9];

    float left = 17066.666f - this->m_cOffset.x * 33.333332f;
    float top = 17066.666f - this->m_cOffset.y * 33.333332f;
    rows[0] = top;
    cols[0] = left;
    rows[1] = top - SMOLTILE_SIZE;
    rows[2] = top - 8.333333f;
    rows[3] = top - 12.5f;
    rows[4] = top - 16.666666f;
    rows[5] = top - 20.833332f;
    rows[6] = top - 25.0f;
    rows[7] = top - 29.166666f;
    cols[1] = left - SMOLTILE_SIZE;
    cols[2] = left - 8.333333f;
    cols[3] = left - 12.5f;
    cols[4] = left - 16.666666f;
    cols[5] = left - 20.833332f;
    cols[6] = left - 25.0f;
    cols[7] = left - 29.166666f;
    rows[8] = 17066.666f - 33.333332f * (this->m_cOffset.y + 1);
    cols[8] = 17066.666f - (this->m_cOffset.x + 1) * 33.333332f;

    int8_t* normals = this->m_normals;
    uint32_t* shading = this->m_vertexShading;
    float* height = this->m_height;

    for (int32_t row = 0; row < 9; row++) {
        for (int32_t col = 0; col < 9; col++) {
            v->m_position.x = rows[row];
            v->m_position.y = cols[col];
            v->m_position.z = this->m_topLeftCoords.z + *height;
            v->m_normal.x = normals[0] * 0.0078740157f;
            v->m_normal.y = normals[1] * 0.0078740157f;
            v->m_normal.z = normals[2] * 0.0078740157f;

            uint32_t color;

            if (!this->m_vertexShading) {
                color = 0xFF7F7F7F;
            } else if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                const uint8_t* src = reinterpret_cast<const uint8_t*>(shading);
                color = src[2] | (src[1] << 8) | (src[0] << 16) | (static_cast<uint32_t>(src[3]) << 24);
            } else {
                color = *shading;
            }

            height++;
            *reinterpret_cast<uint32_t*>(&v->m_color) = color;
            v++;
            normals += 3;
            shading++;
        }

        if (row < 8) {
            for (int32_t col = 0; col < 8; col++) {
                v->m_position.x = rows[row] - HALF_SMOLTILE_SIZE;
                v->m_position.y = cols[col] - HALF_SMOLTILE_SIZE;
                v->m_position.z = this->m_topLeftCoords.z + *height;
                v->m_normal.x = normals[0] * 0.0078740157f;
                v->m_normal.y = normals[1] * 0.0078740157f;
                v->m_normal.z = normals[2] * 0.0078740157f;

                uint32_t color;

                if (!this->m_vertexShading) {
                    color = 0xFF7F7F7F;
                } else if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                    const uint8_t* src = reinterpret_cast<const uint8_t*>(shading);
                    color = src[2] | (src[1] << 8) | (src[0] << 16) | (static_cast<uint32_t>(src[3]) << 24);
                } else {
                    color = *shading;
                }

                height++;
                *reinterpret_cast<uint32_t*>(&v->m_color) = color;
                v++;
                normals += 3;
                shading++;
            }
        }
    }
}

// VERIFY: 421200ee manual
// OFFSET: 0x007C4960
void CMapChunk::CreateVerticesLocal(CGxVertexPN* v) {
    const float stepX = (17066.666f - (float)this->m_cOffset.x * 33.333332f - (17066.666f - (float)(this->m_cOffset.x + 1) * 33.333332f)) * -0.125f;
    const float stepY = (17066.666f - (float)this->m_cOffset.y * 33.333332f - (17066.666f - (float)(this->m_cOffset.y + 1) * 33.333332f)) * -0.125f;

    const float halfStepX = stepX * 0.5f;

    float* height = this->m_height;
    int8_t* normals = this->m_normals;

    for (int row = 0; row < 9; row++) {
        const float outerX = (float)row * stepY;

        for (int col = 0; col < 9; col++) {
            v->m_position.x = outerX;
            v->m_position.y = (float)col * stepX;
            v->m_position.z = height[col];
            v->m_normal.x = normals[col * 3 + 0] * 0.0078740157f;
            v->m_normal.y = normals[col * 3 + 1] * 0.0078740157f;
            v->m_normal.z = normals[col * 3 + 2] * 0.0078740157f;
            v++;
        }

        height += 9;
        normals += 27;

        if (row < 8) {
            const float innerX = (float)row * stepX + 0.5f * stepY;

            for (int col = 0; col < 8; col++) {
                v->m_position.x = innerX;
                v->m_position.y = (float)col * stepY + halfStepX;
                v->m_position.z = height[col];
                v->m_normal.x = normals[col * 3 + 0] * 0.0078740157f;
                v->m_normal.y = normals[col * 3 + 1] * 0.0078740157f;
                v->m_normal.z = normals[col * 3 + 2] * 0.0078740157f;
                v++;
            }

            height += 8;
            normals += 24;
        }
    }
}


// VERIFY: 682ff916 manual
// OFFSET: 0x007C4F10
void CMapChunk::CreateVerticesLocal(CGxVertexPNC* v) {
    float stepX = (17066.666f - this->m_cOffset.x * 33.333332f - (17066.666f - (this->m_cOffset.x + 1) * 33.333332f)) * -0.125f;
    float stepY = -0.125f * (17066.666f - this->m_cOffset.y * 33.333332f - (17066.666f - 33.333332f * (this->m_cOffset.y + 1)));
    float halfStepX = stepX * 0.5f;
    float halfStepY = 0.5f * stepY;

    float* height = this->m_height;
    int8_t* normals = this->m_normals;
    uint32_t* shading = this->m_vertexShading;

    for (int32_t row = 0; row < 9; row++) {
        for (int32_t col = 0; col < 9; col++) {
            v->m_position.x = stepY * row;
            v->m_position.y = col * stepX;
            v->m_position.z = *height;
            v->m_normal.x = normals[0] * 0.0078740157f;
            v->m_normal.y = normals[1] * 0.0078740157f;
            v->m_normal.z = normals[2] * 0.0078740157f;

            if (this->m_vertexShading) {
                CImVector color = *reinterpret_cast<CImVector*>(shading);

                if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                    CImVector swapped;
                    swapped.b = color.r;
                    swapped.g = color.g;
                    swapped.r = color.b;
                    swapped.a = color.a;
                    v->m_color = swapped;
                } else {
                    v->m_color = color;
                }
            } else {
                *reinterpret_cast<uint32_t*>(&v->m_color) = 0xFF7F7F7F;
            }

            height++;
            v++;
            normals += 3;
            shading++;
        }

        if (row < 8) {
            float innerX = row * stepX + halfStepY;

            for (int32_t col = 0; col < 8; col++) {
                v->m_position.x = innerX;
                v->m_position.y = col * stepY + halfStepX;
                v->m_position.z = *height;
                v->m_normal.x = normals[0] * 0.0078740157f;
                v->m_normal.y = normals[1] * 0.0078740157f;
                v->m_normal.z = normals[2] * 0.0078740157f;

                if (this->m_vertexShading) {
                    CImVector color = *reinterpret_cast<CImVector*>(shading);

                    if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                        CImVector swapped;
                        swapped.b = color.r;
                        swapped.g = color.g;
                        swapped.r = color.b;
                        swapped.a = color.a;
                        v->m_color = swapped;
                    } else {
                        v->m_color = color;
                    }
                } else {
                    *reinterpret_cast<uint32_t*>(&v->m_color) = 0xFF7F7F7F;
                }

                height++;
                v++;
                normals += 3;
                shading++;
            }
        }
    }
}

// VERIFY: a98c62d2 manual
// OFFSET: 0x007C51B0
void CMapChunk::CreateIndices(uint16_t* buf, CGxBatch* batch) {
    int32_t baseVertex = batch->m_maxIndex ? batch->m_maxIndex + 1 : 0;
    int16_t count = this->CreateIndices(reinterpret_cast<char*>(buf), baseVertex);

    uint16_t minIndex = batch->m_minIndex;

    if (minIndex >= baseVertex) {
        minIndex = baseVertex;
    }

    batch->m_minIndex = minIndex;

    int32_t maxIndex = CMapRenderChunk::s_chunkBatch.m_maxIndex + baseVertex;

    if (batch->m_maxIndex > maxIndex) {
        maxIndex = batch->m_maxIndex;
    }

    batch->m_maxIndex = maxIndex;
    batch->m_count += count;
}

// VERIFY: 868d1564 manual
// OFFSET: 0x007C5220
void CMapChunk::CreateBounds() {
    int32_t tileX = this->m_cOffset.x;
    int32_t tileY = this->m_cOffset.y;

    this->m_bbox.b.x = 17066.666f - 33.333332f * (tileY + 1);
    this->m_bbox.t.x = 17066.666f - 33.333332f * tileY;
    this->m_bbox.b.y = 17066.666f - 33.333332f * (tileX + 1);
    this->m_bbox.t.y = 17066.666f - 33.333332f * tileX;
    this->m_bbox.b.z = 3.4028235e38f;
    this->m_bbox.t.z = -3.4028235e38f;

    float* h = this->m_height;
    int count = 29;
    do {
        for (int i = 0; i < 5; i++) {
            float height = h[i];
            if (this->m_bbox.b.z > height)
                this->m_bbox.b.z = height;
            if (this->m_bbox.t.z < height)
                this->m_bbox.t.z = height;
        }
        h += 5;
    } while (--count);

    this->m_bbox.b.z += this->m_topLeftCoords.z;
    this->m_bbox.t.z += this->m_topLeftCoords.z;

    this->m_sphere.c.x = (this->m_bbox.b.x + this->m_bbox.t.x) * 0.5f;
    this->m_sphere.c.y = (this->m_bbox.b.y + this->m_bbox.t.y) * 0.5f;
    this->m_sphere.c.z = (this->m_bbox.b.z + this->m_bbox.t.z) * 0.5f;

    this->m_bbox2 = this->m_bbox;

    float ex = this->m_bbox.t.x - this->m_sphere.c.x;
    float ey = this->m_bbox.t.y - this->m_sphere.c.y;
    float ez = this->m_bbox.t.z - this->m_sphere.c.z;
    this->m_sphere.r = sqrt(ex * ex + ey * ey + ez * ez);
}

// VERIFY: baa581e9 auto
// OFFSET: 0x007C5440
void CMapChunk::Batch() {
    if (this->m_bLoaded) {
        return;
    }

    if (CMap::enableChunkBatching) {
        CMapArea* area = static_cast<CMapArea*>(this->m_parentLinkList.Head()->m_ref);

        C2iVector pos;
        pos.x = this->m_aIndex.x & ~1;
        pos.y = this->m_aIndex.y & ~1;
        area->BatchChunks(pos);
    } else {
        this->m_renderChunk = CMap::AllocRenderChunk();
        this->m_renderChunk->AddBatch(this, nullptr, &this->m_topLeftCoords, 0);
        this->m_bLoaded = 1;
    }
}

// VERIFY: 2c8f1c5e auto
// OFFSET: 0x007C54C0
void CMapChunk::CreateVertices(char* buf, int32_t vertexOffset, const C3Vector& offset) {
    if (CMap::enableTerrainShaderVertex) {
        if (CMapRenderChunk::s_gxBufVertexFormat == GxVBF_PN) {
            this->CreateVerticesWorld(reinterpret_cast<CGxVertexPN*>(buf) + vertexOffset, offset);
        } else {
            this->CreateVerticesWorld(reinterpret_cast<CGxVertexPNC*>(buf) + vertexOffset, offset);
        }
    } else if (CMapRenderChunk::s_gxBufVertexFormat == GxVBF_PN) {
        this->CreateVerticesLocal(reinterpret_cast<CGxVertexPN*>(buf));
    } else {
        this->CreateVerticesLocal(reinterpret_cast<CGxVertexPNC*>(buf));
    }
}

// VERIFY: 4f0133d9 auto
// OFFSET: 0x007C5530
void CMapChunk::GetLiquidBounds(CAaBox& box) const {
    box.t = this->m_bbox.t;
    box.b = this->m_bbox.b;

    CChunkLiquid* liquid = const_cast<CMapChunk*>(this)->m_liquidChunkLinkList.Head();

    if (liquid) {
        box.t.z = liquid->m_height.y;
        box.b.z = liquid->m_height.x;

        do {
            box.t.z = liquid->m_height.y >= box.t.z ? liquid->m_height.y : box.t.z;
            box.b.z = liquid->m_height.x <= box.b.z ? liquid->m_height.x : box.b.z;
            liquid = const_cast<CMapChunk*>(this)->m_liquidChunkLinkList.Next(liquid);
        } while (liquid);
    }
}

// VERIFY: 16a231ad manual
// OFFSET: 0x007C55D0
bool CMapChunk::GetLiquidHeight(const C2Vector& pos, float* height) const {
    int32_t x = CMath::fint_(pos.x - 0.5f);
    int32_t y = CMath::fint_(pos.y - 0.5f);

    C2iVector tile;
    tile.x = x & 7;
    tile.y = y & 7;

    C2Vector frac(pos.x - x, pos.y - y);

    for (CChunkLiquid* liquid = const_cast<CMapChunk*>(this)->m_liquidChunkLinkList.Head(); liquid; liquid = const_cast<CMapChunk*>(this)->m_liquidChunkLinkList.Next(liquid)) {
        if (liquid->TileExists(tile.x, tile.y) && liquid->GetHeight(frac, tile, *height)) {
            return true;
        }
    }

    return false;
}

// VERIFY: bb038337 manual
// OFFSET: 0x007C5690
void CMapChunk::CreateLiquids(bool a3) {
    CChunkLiquid* liquid = this->m_liquidChunkLinkList.Head();

    while (liquid) {
        liquid->m_chunkLink.Unlink();
        liquid->Purge();
        CMap::FreeChunkLiquid(liquid);
        liquid = this->m_liquidChunkLinkList.Next(liquid);
    }

    SMLiquidChunk* mclq = this->m_liquid;

    if (mclq) {
        uint32_t flag = 4;

        for (uint32_t i = 0; i < 4; i++) {
            if (flag & this->m_header->m_flags) {
                uint32_t type = i + 1;

                if (s_mapId == 530 && i == 2) {
                    type = 15;
                }

                LiquidTypeRec* typeRec = g_liquidTypeDB.GetRecord(type);
                LiquidMaterialRec* materialRec = g_liquidMaterialDB.GetRecord(typeRec->m_materialID);

                CChunkLiquid* chunkLiquid = CMap::AllocChunkLiquid();
                this->m_liquidChunkLinkList.LinkToTail(chunkLiquid);
                chunkLiquid->m_type = type;
                chunkLiquid->m_owner = this;
                chunkLiquid->m_vertexFormat = materialRec->m_lvf;
                chunkLiquid->m_topLeftCoords = this->m_topLeftCoords;
                chunkLiquid->m_height = *reinterpret_cast<C2Vector*>(&mclq->m_minHeight);

                uint8_t* verts = reinterpret_cast<uint8_t*>(mclq->m_verts);
                chunkLiquid->m_tileBegin.x = 0;
                chunkLiquid->m_tileBegin.y = 0;
                chunkLiquid->m_tileEnd.x = 8;
                chunkLiquid->m_tileEnd.y = 8;

                void* mem = SMemAlloc(sizeof(Liquid::CChunkVertexData), __FILE__, __LINE__, 0);
                Liquid::CChunkVertexData* vertexData = mem ? new (mem) Liquid::CChunkVertexData(verts, chunkLiquid->m_vertexFormat) : nullptr;
                chunkLiquid->m_tiles = reinterpret_cast<uint8_t*>(&mclq->m_tiles);
                chunkLiquid->m_vertexData = vertexData;
                mclq++;

                chunkLiquid->CreateVertXY();
            }

            flag *= 2;
        }
    }

    CMapArea* area = static_cast<CMapArea*>(this->m_parentLinkList.Head()->m_ref);

    if (area->m_liquid && area->m_liquid->HasData()) {
        const SMLiquidChunkDesc* chunkDesc = area->m_liquid->GetChunkDesc(this->m_aIndex.x, this->m_aIndex.y);
        this->m_deepBits.SetRawData(const_cast<char*>(reinterpret_cast<const char*>(area->m_liquid->GetDeepTable(chunkDesc))), 64, false);
        this->m_fishableBits.SetRawData(const_cast<char*>(reinterpret_cast<const char*>(area->m_liquid->GetFishableTable(chunkDesc))), 64, false);

        for (uint32_t i = 0; i < chunkDesc->m_layerCount; i++) {
            const SMLiquidInstance* instance = area->m_liquid->GetInstanceDesc(chunkDesc, i);

            CChunkLiquid* chunkLiquid = CMap::AllocChunkLiquid();
            this->m_liquidChunkLinkList.LinkToTail(chunkLiquid);
            chunkLiquid->m_owner = this;
            chunkLiquid->m_type = instance->m_liquidType;
            chunkLiquid->m_vertexFormat = instance->m_vertexFormat;
            chunkLiquid->m_topLeftCoords = this->m_topLeftCoords;
            chunkLiquid->m_height = *reinterpret_cast<const C2Vector*>(&instance->m_minHeight);

            uint8_t yOffset = instance->m_yOffset;
            uint8_t xOffset = instance->m_xOffset;
            chunkLiquid->m_tileBegin.x = yOffset;
            chunkLiquid->m_tileBegin.y = xOffset;
            chunkLiquid->m_tileEnd.x = yOffset + instance->m_height;
            chunkLiquid->m_tileEnd.y = xOffset + instance->m_width;

            void* mem = SMemAlloc(sizeof(Liquid::CInstanceVertexData), __FILE__, __LINE__, 0);
            chunkLiquid->m_vertexData = mem ? new (mem) Liquid::CInstanceVertexData(area->m_liquid, instance) : nullptr;

            uint32_t tileCount = instance->m_width * instance->m_height;
            chunkLiquid->m_tileBits.SetRawData(const_cast<char*>(reinterpret_cast<const char*>(area->m_liquid->GetExistsTable(instance))), tileCount, false);
            chunkLiquid->CreateVertXY();
        }
    }

    if (!this->m_liquidChunkLinkList.Head()) {
        this->m_liquidBounds.b = C3Vector(3.4028235e38f, 3.4028235e38f, 3.4028235e38f);
        this->m_liquidBounds.t = C3Vector(-3.4028235e38f, -3.4028235e38f, -3.4028235e38f);
    } else {
        this->GetLiquidBounds(this->m_liquidBounds);
    }
}

// VERIFY: 2c9fa8ff manual
// OFFSET: 0x007C5B20
void CMapChunk::CullAndBucketChunk() {
    if (this->m_liquidBounds.t.x < this->m_liquidBounds.b.x || CWorldScene::FrustumCull(&this->m_liquidBounds)) {
        return;
    }

    int32_t index = CMapChunk::cornerVertexIndex[CMapChunk::farCornerIndex];
    float camZ = CWorldScene::s_activeWorldView.z;
    C3Vector corner(CMapChunk::vertexList[index].x + this->m_topLeftCoords.x, CMapChunk::vertexList[index].y + this->m_topLeftCoords.y, this->m_height[index] + this->m_topLeftCoords.z);

    for (CChunkLiquid* liquid = this->m_liquidChunkLinkList.Head(); liquid; liquid = this->m_liquidChunkLinkList.Next(liquid)) {
        CAaBox box;
        liquid->GetAaBox(box);

        if (!CWorldScene::FrustumCull(&box)) {
            corner.z = (camZ <= liquid->m_height.y ? camZ : liquid->m_height.y) < liquid->m_height.x ? liquid->m_height.x : (camZ > liquid->m_height.y ? liquid->m_height.y : camZ);
            CWorldScene::AddToDistanceBucket(liquid, corner);
        }
    }
}

// VERIFY: 555895ad manual
// OFFSET: 0x007C5C50
CMapChunk::CMapChunk() {
    this->m_aIndex.x = 0;
    this->m_aIndex.y = 0;
    this->m_sOffset.x = 0;
    this->m_sOffset.y = 0;
    this->m_cOffset.x = 0;
    this->m_cOffset.y = 0;
    this->m_sphere.r = 0.0f;
    this->m_sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_link.m_prevLink = nullptr;
    this->m_link.m_next = nullptr;
    this->m_sortListLink.m_prevLink = nullptr;
    this->m_sortListLink.m_next = nullptr;
    this->m_doodadDefLinkList.SetLinkOffset(offsetof(CMapBaseObjLink, m_refLink));
    this->m_mapObjDefLinkList.SetLinkOffset(offsetof(CMapBaseObjLink, m_refLink));
    this->m_entityLinkList.SetLinkOffset(offsetof(CMapBaseObjLink, m_refLink));
    this->m_lightLinkList.SetLinkOffset(offsetof(CMapBaseObjLink, m_refLink));
    this->m_soundHandleList.SetLinkOffset(0);
    this->m_type |= 4;
    this->m_distToCamera = 0.0f;
    this->m_flags = static_cast<MapObjFlag>(this->m_flags | MAPOBJ_FLAG_UNPLACED);
    this->m_detailDoodadInst = nullptr;
    this->m_renderChunk = nullptr;
    this->m_chunkInfoBeginPtr = nullptr;
    this->m_header = nullptr;
    this->m_lowQualityTexMap = nullptr;
    this->m_predTexture = nullptr;
    this->m_height = nullptr;
    this->m_vertexShading = nullptr;
    this->m_normals = nullptr;
    this->m_shadowMap = nullptr;
    this->m_layers = nullptr;
    this->m_additionalShadowmap = nullptr;
    this->m_mcrfPtr = nullptr;
    this->m_liquid = nullptr;
    this->m_soundEmitters = nullptr;
    this->m_bLoaded = 0;
}

// VERIFY: ecd160bb manual
// OFFSET: 0x007C6060
void CMapChunk::CreateSoundEmitters(bool a3) {
    if (!this->m_soundHandleList.IsEnd(this->m_soundHandleList.Head())) {
        return;
    }

    CWSoundEmitter* emitter = this->m_soundEmitters;

    if (!emitter) {
        return;
    }

    for (uint32_t i = 0; i < this->m_header->m_nSndEmitters; i++, emitter++) {
        SE2SoundKitObject* object = new (__FILE__, __LINE__) SE2SoundKitObject();
        this->m_soundHandleList.LinkToTail(object);

        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_coneOrientation = &emitter->m_size;
        properties.m_forceLoop = 1;
        SI2::PlaySoundKit(emitter->m_entryId, &emitter->m_position, object, &properties, 0, nullptr, 1, 0);
    }
}

// VERIFY: 80daf949 manual
// OFFSET: 0x007C6150
void CMapChunk::CreateRefs(CMapArea* area, uint32_t* mcrfPtr, uint32_t doodadRefs, uint32_t mapObjRefs) {
    C3Vector center(17066.666f, 17066.666f, 0.0f);
    uint32_t* mapObjRefList = &mcrfPtr[doodadRefs];

    for (uint32_t i = 0; i < mapObjRefs; i++) {
        SMMapObjDef* def = &area->m_mapObjDef[mapObjRefList[i]];

        if (def->m_flags & 0x1) {
            if (def->m_uniqueId != 0x5476ED || CMap::mapObjDefHash.Ptr(0x5476ED, CMap::s_mapObjDefHashKey)) {
                continue;
            }

            CDestructibleProxy* node = CMap::s_destructibleProxyList.Head();

            while (!CMap::s_destructibleProxyList.IsEnd(node) && node->m_uniqueId != def->m_uniqueId) {
                node = CMap::s_destructibleProxyList.RawNext(node);
            }

            if (!CMap::s_destructibleProxyList.IsEnd(node)) {
                continue;
            }

            CDestructibleProxy proxy;
            C3Vector b(center.x - def->m_extents.t.z, center.y - def->m_extents.t.x, center.z + def->m_extents.b.y);
            C3Vector t(center.x - def->m_extents.b.z, center.y - def->m_extents.b.x, center.z + def->m_extents.t.y);
            proxy.m_sphere.c = b + t;
            proxy.m_sphere.c *= 0.5f;
            proxy.m_uniqueId = def->m_uniqueId;
            proxy.m_sphere.r = sqrtf((t - proxy.m_sphere.c).SquaredMag());

            if (proxy.m_sphere.r < 0.001f) {
                proxy.m_sphere.r = 50.0f;
                SysMsgPrintf(SYSMSG_ERROR, 2, "Destructible building WMO(%s) has invalid geobox", &area->m_wmoFileNames[area->m_wmoFilenamesOffsets[def->m_nameId]]);
            }

            World::FindOrCreateDestructibleProxy(&proxy);
        } else {
            CMapObjDef* mapObjDef = CMap::CreateMapObjDef(&area->m_wmoFileNames[area->m_wmoFilenamesOffsets[def->m_nameId]], def, &center, true);
            CMapBaseObjLink* link = CMap::AllocBaseObjLink(mapObjDef);
            link->m_ref = this;
            this->m_mapObjDefLinkList.LinkToTail(link);
        }
    }

    for (uint32_t i = 0; i < doodadRefs; i++) {
        SMDoodadDef* def = &area->m_doodadDef[mcrfPtr[i]];
        CMapDoodadDef* doodadDef = CMap::CreateDoodadDef(&area->m_m2FileNames[area->m_modelFilenamesOffsets[def->m_nameId]], def, &center);
        CMapBaseObjLink* link = CMap::AllocBaseObjLink(doodadDef);
        link->m_ref = this;
        this->m_doodadDefLinkList.LinkToTail(link);
        doodadDef->m_diffuseLightScale = 1.0f;
        doodadDef->m_flags = static_cast<MapObjFlag>(doodadDef->m_flags | MAPOBJ_FLAG_EXTERIOR);

        if (doodadDef->m_flags & MAPOBJ_FLAG_PREPARED) {
            CMapDoodadDef::ExtendBounds(doodadDef, this);
        }
    }
}

// VERIFY: 637523f5 manual
// OFFSET: 0x007C64B0
void CMapChunk::Create(SIffChunk* headerChunk, bool a3) {
    this->m_chunkInfoBeginPtr = reinterpret_cast<uint8_t*>(headerChunk);
    this->ProcessIffChunks(a3);

    this->m_areaId = this->m_header->m_areaid;
    this->m_topLeftCoords.x = 17066.666f - this->m_cOffset.y * 33.333332f;
    this->m_topLeftCoords.y = 17066.666f - this->m_cOffset.x * 33.333332f;
    this->m_topLeftCoords.z = 0.0f;
    this->m_topLeftCoords.z = this->m_header->m_position.z;
    this->m_lowQualityTexMap = this->m_header->m_lowQualityTextureMap;
    this->m_predTexture = reinterpret_cast<uint8_t*>(&this->m_header->m_predTex);

    this->CreateBounds();
    this->CreateLiquids(a3);
    this->CreateSoundEmitters(a3);

    this->m_flags = static_cast<MapObjFlag>(0);

    if (this->m_header->m_flags & 2) {
        this->m_flags = MAPOBJ_FLAG_IMPASSABLE;
    }

    CMapArea* area = static_cast<CMapArea*>(this->m_parentLinkList.Head()->m_ref);
    this->CreateRefs(area, reinterpret_cast<uint32_t*>(this->m_mcrfPtr), this->m_header->m_nDoodadRefs, this->m_header->m_nMapObjRefs);
    area->m_mapChunks[16 * this->m_aIndex.y + this->m_aIndex.x] = this;
    this->m_flags = static_cast<MapObjFlag>(this->m_flags | MAPOBJ_FLAG_PREPARED);
}

// VERIFY: 1d9777de manual
// OFFSET: 0x007C65A0
void CMapChunk::UpdateLights() {
    for (CMapBaseObjLink* link = this->m_doodadDefLinkList.Head(); link; link = this->m_doodadDefLinkList.Next(link)) {
        link->m_owner->m_flags = static_cast<MapObjFlag>(link->m_owner->m_flags | 0x1);
    }

    for (CMapBaseObjLink* link = this->m_entityLinkList.Head(); link; link = this->m_entityLinkList.Next(link)) {
        link->m_owner->m_flags = static_cast<MapObjFlag>(link->m_owner->m_flags | 0x1);
    }
}

// VERIFY: c9630247 manual
// OFFSET: 0x007CFB10
void CMapChunk::ClipBufferUpdate() {
    uint32_t indices[9];
    uint32_t base = 0;

    if (CWorldScene::s_activeWorldView.x > CWorldScene::camTarget.x) {
        base = 136;
    }

    for (uint32_t i = 0; i < 9; i++) {
        indices[i] = i + base;
    }

    CWorldOcclusion::UpdateBufferChunk(CMapChunk::vertexList, this->m_height, indices, 9, &this->m_topLeftCoords, this->m_header->m_holes);

    base = 0;

    if (CWorldScene::s_activeWorldView.y > CWorldScene::camTarget.y) {
        base = 8;
    }

    for (uint32_t i = 0; i < 9; i++) {
        indices[i] = base;
        base += 17;
    }

    CWorldOcclusion::UpdateBufferChunk(CMapChunk::vertexList, this->m_height, indices, 9, &this->m_topLeftCoords, this->m_header->m_holes);
}

// VERIFY: bd05b674 auto
// OFFSET: 0x007D05F0
int32_t CMapChunk::LoadDetailDoodadModels() {
    int32_t loaded = 1;

    for (uint32_t i = 0; i < this->m_header->m_nLayers; i++) {
        int32_t effectId = this->m_layers[i].m_offectId;

        if (!effectId) {
            continue;
        }

        GroundEffectTextureRec* record = g_groundEffectTextureDB.GetRecord(effectId);

        if (!record) {
            continue;
        }

        for (uint32_t j = 0; j < 4; j++) {
            if (record->m_doodadID[j] && !CDetailDoodad::LoadModel(record->m_doodadID[j])) {
                loaded = 0;
            }
        }
    }

    return loaded;
}

// VERIFY: 04ba1328 manual
// OFFSET: 0x007D3390
void CMapChunk::CreateDetailDoodads() {
    if (!this->m_header->m_nLayers || !this->LoadDetailDoodadModels()) {
        return;
    }

    CRndSeed seed;
    seed.SetSeed(0);
    seed.SetSeed(this->m_cOffset.x | (this->m_cOffset.y << 16));

    this->m_detailDoodadInst = CDetailDoodad::AllocInst();
    this->m_detailDoodadInst->m_chunk = this;
    CDetailDoodadInst* inst = this->m_detailDoodadInst;

    if (CWorld::detailDoodadTest) {
        return;
    }

    static C4Plane s_planes[256];
    static C2iVector s_cells[256];
    C3Vector verts[3];
    static C3Vector s_colorBase[256];
    static C3Vector s_colorEdgeA[256];
    static C3Vector s_colorEdgeB[256];

    uint8_t used[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    for (uint32_t i = 0; i < World::s_detailDoodadDensity; i++) {
        uint32_t x = CRandom::uint32_(seed) & 7;
        uint32_t y = CRandom::uint32_(seed) & 7;
        s_cells[i].x = x;
        s_cells[i].y = y;

        if (used[y] & (1 << x)) {
            continue;
        }

        used[y] |= 1 << x;

        uint32_t cell = x + 8 * y;
        float* height = &this->m_height[17 * y + x];
        uint8_t* shadeCorner = nullptr;
        uint8_t* shadeCenter = nullptr;

        if (this->m_vertexShading) {
            shadeCorner = reinterpret_cast<uint8_t*>(&this->m_vertexShading[17 * y + x]);
            shadeCenter = shadeCorner + 9 * sizeof(uint32_t);
        }

        float offsetX = y * -SMOLTILE_SIZE;
        float offsetY = x * -SMOLTILE_SIZE;

        verts[0] = C3Vector(CMap::s_subchunkCoords[4].x + offsetX, CMap::s_subchunkCoords[4].y + offsetY, CMap::s_subchunkCoords[4].z);
        verts[0].z = height[9];

        for (uint32_t j = 0; j < 4; j++) {
            const C3Vector& a = CMap::s_subchunkCoords[CMap::s_subchunkIndices[2 * j]];
            const C3Vector& b = CMap::s_subchunkCoords[CMap::s_subchunkIndices[2 * j + 1]];
            int32_t fanA = CMap::s_fanIndices[2 * j];
            int32_t fanB = CMap::s_fanIndices[2 * j + 1];

            verts[1] = C3Vector(offsetX + a.x, a.y + offsetY, a.z);
            verts[2] = C3Vector(offsetX + b.x, offsetY + b.y, b.z);
            verts[1].z = height[fanA];
            verts[2].z = height[fanB];

            s_planes[4 * cell + j].From3Pos(verts[0], verts[2], verts[1]);

            if (this->m_vertexShading) {
                uint8_t* shadeA = shadeCorner + fanA * sizeof(uint32_t);
                uint8_t* shadeB = shadeCorner + fanB * sizeof(uint32_t);
                uint32_t k = 4 * cell + j;

                s_colorBase[k] = C3Vector(shadeCenter[2], shadeCenter[1], shadeCenter[0]);
                s_colorEdgeA[k] = C3Vector(shadeA[2] - shadeCenter[2], shadeA[1] - shadeCenter[1], shadeA[0] - shadeCenter[0]);
                s_colorEdgeB[k] = C3Vector(shadeB[2] - shadeA[2], shadeB[1] - shadeA[1], shadeB[0] - shadeA[0]);

                s_colorBase[k] *= 2.0f;
                s_colorEdgeA[k] *= 2.0f;
                s_colorEdgeB[k] *= 2.0f;
            }
        }
    }

    C3Vector position(0.0f, 0.0f, 0.0f);
    CImVector color;

    for (uint32_t i = 0; i < World::s_detailDoodadDensity; i++) {
        int32_t x = s_cells[i].x;
        int32_t y = s_cells[i].y;

        if ((this->m_predTexture[y] & s_bitMask[x]) >> s_bitShift[x]) {
            continue;
        }

        if (this->m_header->m_holes & CMap::s_holeMask[4 * (y >> 1) + (x >> 1)]) {
            continue;
        }

        uint32_t layer = (reinterpret_cast<uint16_t*>(this->m_lowQualityTexMap)[y] & CMap::s_2bitSplatMask[x]) >> CMap::s_2bitSplatShft[x];
        GroundEffectTextureRec* record = g_groundEffectTextureDB.GetRecord(this->m_layers[layer].m_offectId);

        if (!record) {
            continue;
        }

        uint32_t count = record->m_density;

        if (!count) {
            count = 8;
        }

        int32_t doodads[16];
        uint32_t slot = 0;
        uint32_t filled = 0;

        for (uint32_t k = 0; k < 4; k++) {
            int32_t weight = record->m_doodadWeight[k];
            int32_t id = record->m_doodadID[k];

            if (weight) {
                filled += weight;

                do {
                    doodads[slot & 15] = id;
                    slot += 13;
                } while (--weight);
            }
        }

        for (; filled < 16; filled++) {
            doodads[slot & 15] = record->m_doodadID[filled & 3];
            slot += 13;
        }

        uint32_t cellBase = 4 * (x + 8 * y);
        float cellX = x * SMOLTILE_SIZE;
        float cellY = y * SMOLTILE_SIZE;

        for (uint32_t j = 0; j < count; j++) {
            float u = CRandom::sfloat_(seed);
            float v = CRandom::sfloat_(seed);
            uint32_t id = doodads[(i + j) & 15];
            float px = u * HALF_SMOLTILE_SIZE + HALF_SMOLTILE_SIZE;
            float py = v * HALF_SMOLTILE_SIZE + HALF_SMOLTILE_SIZE;

            if (!id) {
                continue;
            }

            CDetailDoodadData* data = CDetailDoodad::s_doodadList[id];
            position.x = -py;
            position.y = -px;

            uint32_t tri = 0;
            float diagonal = -px - -py;

            if (diagonal < 0.0f) {
                tri = 1;
            }

            if (px - SMOLTILE_SIZE - -py > 0.0f) {
                tri += 2;
            }

            tri += cellBase;
            C4Plane& plane = s_planes[tri];

            if (plane.n.z < 0.4f) {
                continue;
            }

            if ((data->m_record->m_flags & 2) || !this->m_vertexShading) {
                color.r = 0xFF;
                color.g = 0xFF;
                color.b = 0xFF;
            } else {
                float s;
                float t;

                if (fabs(v) < fabs(u)) {
                    s = fabs(u);
                    t = 0.5f - v * 0.5f;
                } else {
                    s = fabs(v);
                    t = 0.5f - u * 0.5f;
                }

                if (diagonal < 0.0f) {
                    t = 1.0f - t;
                }

                float st = s * t;
                C3Vector shade(s_colorEdgeA[tri].x * s + s_colorBase[tri].x + s_colorEdgeB[tri].x * st, s_colorEdgeA[tri].y * s + s_colorBase[tri].y + s_colorEdgeB[tri].y * st, s_colorEdgeA[tri].z * s + s_colorBase[tri].z + s_colorEdgeB[tri].z * st);

                if (shade.x > 255.0f) {
                    shade.x = 255.0f;
                }

                if (shade.y > 255.0f) {
                    shade.y = 255.0f;
                }

                if (shade.z > 255.0f) {
                    shade.z = 255.0f;
                }

                color.r = static_cast<uint8_t>(CMath::fint_(shade.x - 0.5f));
                color.g = static_cast<uint8_t>(CMath::fint_(shade.y - 0.5f));
                color.b = static_cast<uint8_t>(CMath::fint_(shade.z - 0.5f));
            }

            color.a = 0xFF;

            if (this->m_shadowMap) {
                float shadowX = (px + cellX) * OO_COORD_TO_SHADOW;
                int32_t sx = CMath::fint_(shadowX - 0.5f);
                float shadowY = (py + cellY) * OO_COORD_TO_SHADOW;
                int32_t sy = CMath::fint_(shadowY - 0.5f);

                if (this->m_shadowMap[8 * sy + (sx >> 3)] & (1 << (sx & 7))) {
                    color.a = 0;
                }
            }

            position.x -= cellY;
            position.y -= cellX;
            position.z = -((position.x * plane.n.x + position.y * plane.n.y + plane.d) / plane.n.z);

            float rotation = (CRandom::sfloat_(seed) + 1.0f) * 3.1415927f;
            float scale = CRandom::sfloat_(seed) * 0.33f + 1.0f;

            inst->AddDoodad(id, position, rotation, scale, plane.n, static_cast<uint16_t>(tri), color);
        }
    }
}

// VERIFY: a8fc9219 auto
// OFFSET: 0x007D3FE0
void CMapChunk::RenderPrep() {
    if (this->m_renderChunk || (this->Batch(), this->m_renderChunk)) {
        float dy = CWorldScene::s_activeWorldView.y - this->m_sphere.c.y;
        float dz = CWorldScene::s_activeWorldView.z - this->m_sphere.c.z;
        float dx = CWorldScene::s_activeWorldView.x - this->m_sphere.c.x;

        if (CWorld::shadowMipLevel || dx * dx + dz * dz + dy * dy > 603729.0f) {
            this->m_renderChunk->m_renderFlags |= 0x10;
        }

        this->m_renderChunk->RenderPrep();
    }

    if ((CWorld::enables & Enable_DetailDoodads) && CWorld::detailDoodadDist > this->m_distToCamera) {
        if (!this->m_detailDoodadInst) {
            this->CreateDetailDoodads();
        }

        if (this->m_detailDoodadInst) {
            CWorldScene::AddChunkToSortList(this->m_detailDoodadInst);
        }
    }
}

// VERIFY: 6e3fa3c6 manual
// OFFSET: 0x007D66D0
bool CMapChunk::CanMergeChunkLayers(CMapChunk* chunkA, CMapChunk* chunkB) {
    chunkB->m_bLoaded = 1;
    chunkA->m_bLoaded = 1;

    if (CMap::header.m_flags & 4) {
        uint32_t count = chunkB->m_header->m_nLayers;
        SMLayer* layerA = chunkA->m_layers;

        for (uint32_t i = 0; i < chunkA->m_header->m_nLayers; i++, layerA++) {
            if (layerA->m_flags & 0x4C0) {
                return false;
            }

            SMLayer* layerB = chunkB->m_layers;
            bool found = false;

            for (uint32_t j = 0; j < chunkB->m_header->m_nLayers; j++, layerB++) {
                if (layerB->m_flags & 0x4C0) {
                    return false;
                }

                if (layerB->m_textureId == layerA->m_textureId) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                count++;
            }
        }

        return count <= 4;
    }

    uint32_t layerCount = chunkA->m_header->m_nLayers;

    if (chunkB->m_header->m_nLayers != layerCount) {
        return false;
    }

    for (uint32_t i = 0; i < layerCount; i++) {
        SMLayer& layerA = chunkA->m_layers[i];
        SMLayer& layerB = chunkB->m_layers[i];

        if ((layerA.m_flags & 0xC0) || (layerB.m_flags & 0xC0) || layerB.m_textureId != layerA.m_textureId) {
            return false;
        }
    }

    return true;
}

// VERIFY: 774ebb1a manual
// OFFSET: 0x007D8730
int32_t CMapChunk::Intersect(int32_t subX, int32_t subY, const C3Ray& ray, float* t) {
    int32_t hit = 0;

    if (this->m_header->m_holes & CMap::s_holeMask[4 * (subY >> 1) + (subX >> 1)]) {
        return 0;
    }

    int32_t base = 16 * subY + subX + subY;

    for (uint32_t i = 0; i < 4; i++) {
        uint32_t tri[3] = { static_cast<uint32_t>(base + 9), static_cast<uint32_t>(base + CMap::s_fanIndices[2 * i + 1]), static_cast<uint32_t>(base + CMap::s_fanIndices[2 * i]) };
        CMapChunk::vertexList[tri[0]].z = this->m_height[tri[0]];
        CMapChunk::vertexList[tri[1]].z = this->m_height[tri[1]];
        CMapChunk::vertexList[tri[2]].z = this->m_height[tri[2]];
        float hitT = 0.0f;

        if (NTempest::Intersect(ray, CMapChunk::vertexList, tri, &hitT, nullptr, 0.0099999998f)) {
            hit = 1;

            if (hitT < *t && hitT >= 0.0f) {
                *t = hitT;
            }
        }
    }

    return hit;
}

// VERIFY: ba2b9143 manual
// OFFSET: 0x007D8840
void CMapChunk::Intersect(CiRect* rect, CAaBox* box, TSGrowableArray<CFacet>* facets) {
    int32_t base = 16 * rect->minY + rect->minX + rect->minY;
    float* height = &this->m_height[base];
    C3Vector* vertices = &CMapChunk::vertexList[base];
    int32_t rowSkip = 17 - (rect->maxX - rect->minX + 1);

    for (int32_t y = rect->minY; y <= rect->maxY; y++) {
        for (int32_t x = rect->minX; x <= rect->maxX; x++, height++, vertices++) {
            if (this->m_header->m_holes & CMap::s_holeMask[4 * (y >> 1) + (x >> 1)]) {
                continue;
            }

            uint8_t outcodes[20];

            for (uint32_t i = 0; i < sizeof(s_subVertexIndexFlat) / sizeof(s_subVertexIndexFlat[0]); i++) {
                int32_t index = s_subVertexIndexFlat[i];
                vertices[index].z = height[index];
                outcodes[index] = static_cast<uint8_t>(CWorldMath::ComputeAaBoxOutcode(box, &vertices[index]));
            }

            for (uint32_t tri = 0; tri < sizeof(s_subVertexTriangles) / sizeof(s_subVertexTriangles[0]); tri += 3) {
                const int32_t* indices = &s_subVertexTriangles[tri];

                if (outcodes[indices[0]] & outcodes[indices[1]] & outcodes[indices[2]]) {
                    if (!(CWorld::enables & Enable_ShowQuery)) {
                        continue;
                    }

                    CFacet facet(0.0f);
                    facet.v[0] = this->m_topLeftCoords + vertices[indices[0]];
                    facet.v[1] = this->m_topLeftCoords + vertices[indices[1]];
                    facet.v[2] = this->m_topLeftCoords + vertices[indices[2]];
                    facet.plane.From3Pos(facet.v[0], facet.v[1], facet.v[2]);
                    CImVector color;
                    *reinterpret_cast<uint32_t*>(&color) = 0x80FF0000;
                    CMap::TestQueryAdd(facet, color, nullptr);
                    continue;
                }

                CFacet* facet = facets->New();
                facet->v[0] = vertices[indices[0]] + this->m_topLeftCoords;
                facet->v[1] = vertices[indices[1]] + this->m_topLeftCoords;
                facet->v[2] = vertices[indices[2]] + this->m_topLeftCoords;

                if (CMap::s_useSSE) {
                    C3Vector e1 = facet->v[1] - facet->v[0];
                    C3Vector e2 = facet->v[2] - facet->v[0];
                    facet->plane.n.x = e1.y * e2.z - e1.z * e2.y;
                    facet->plane.n.y = e1.z * e2.x - e2.z * e1.x;
                    facet->plane.n.z = e2.y * e1.x - e2.x * e1.y;
                    float scale = 1.0f / sqrtf(facet->plane.n.x * facet->plane.n.x + facet->plane.n.y * facet->plane.n.y + facet->plane.n.z * facet->plane.n.z);
                    facet->plane.n.x *= scale;
                    facet->plane.n.y *= scale;
                    facet->plane.n.z *= scale;
                    facet->plane.d = -(facet->plane.n.z * facet->v[0].z + facet->plane.n.y * facet->v[0].y + facet->plane.n.x * facet->v[0].x);
                } else {
                    facet->plane.From3Pos(facet->v[0], facet->v[1], facet->v[2]);
                }

                if (CWorld::enables & Enable_ShowQuery) {
                    CImVector color;
                    *reinterpret_cast<uint32_t*>(&color) = 0x8000FF00;
                    CMap::TestQueryAdd(*facet, color, nullptr);
                }
            }
        }

        height += rowSkip;
        vertices += rowSkip;
    }
}

// VERIFY: 2452b304 manual
// OFFSET: 0x007D8E00
void CMapChunk::Intersect(CiRect* rect, const CFrustum* frustum, TSGrowableArray<CFacet>* facets) {
    int32_t base = 16 * rect->minY + rect->minX + rect->minY;
    C3Vector* vertices = &CMapChunk::vertexList[base];
    float* height = &this->m_height[base];
    int32_t rowSkip = 17 - (rect->maxX - rect->minX + 1);

    for (int32_t y = rect->minY; y <= rect->maxY; y++) {
        for (int32_t x = rect->minX; x <= rect->maxX; x++, vertices++, height++) {
            if (this->m_header->m_holes & CMap::s_holeMask[4 * (y >> 1) + (x >> 1)]) {
                continue;
            }

            uint8_t outcodes[20];

            for (uint32_t i = 0; i < sizeof(s_subVertexIndexFlat) / sizeof(s_subVertexIndexFlat[0]); i++) {
                int32_t index = s_subVertexIndexFlat[i];
                vertices[index].z = height[index];
                frustum->Cull(vertices[index], outcodes[index]);
            }

            for (uint32_t tri = 0; tri < sizeof(s_subVertexTriangles) / sizeof(s_subVertexTriangles[0]); tri += 3) {
                const int32_t* indices = &s_subVertexTriangles[tri];

                if (outcodes[indices[0]] & outcodes[indices[1]] & outcodes[indices[2]]) {
                    continue;
                }

                CFacet* facet = facets->New();
                facet->v[0] = vertices[indices[0]] + this->m_topLeftCoords;
                facet->v[1] = vertices[indices[1]] + this->m_topLeftCoords;
                facet->v[2] = vertices[indices[2]] + this->m_topLeftCoords;

                if (CMap::s_useSSE) {
                    C3Vector e1 = facet->v[1] - facet->v[0];
                    C3Vector e2 = facet->v[2] - facet->v[0];
                    facet->plane.n.x = e1.y * e2.z - e1.z * e2.y;
                    facet->plane.n.y = e1.z * e2.x - e2.z * e1.x;
                    facet->plane.n.z = e2.y * e1.x - e2.x * e1.y;
                    float scale = 1.0f / sqrtf(facet->plane.n.x * facet->plane.n.x + facet->plane.n.y * facet->plane.n.y + facet->plane.n.z * facet->plane.n.z);
                    facet->plane.n.x *= scale;
                    facet->plane.n.y *= scale;
                    facet->plane.n.z *= scale;
                    facet->plane.d = -(facet->plane.n.z * facet->v[0].z + facet->plane.n.y * facet->v[0].y + facet->plane.n.x * facet->v[0].x);
                } else {
                    facet->plane.From3Pos(facet->v[0], facet->v[1], facet->v[2]);
                }
            }
        }

        vertices += rowSkip;
        height += rowSkip;
    }
}

// VERIFY: 15f51c4a manual
// OFFSET: [0x007C5E50, 0x007C6480]
CMapChunk::~CMapChunk() {
}
