#include "MapObj.h"
#include <cstdlib>
#include <malloc.h>
#include <new>
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "WoW/Source/WorldClient/PolyClipper.h"
#include <cstring>
#include <cmath>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxLight.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gxu/GxuLight.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Ray.h"
#include "Engine/Source/Tempest/C3Segment.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CFrustum.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/CWorldMath.h"
#include "WoW/Source/WorldClient/CWorldOcclusion.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/MapRenderChunk.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include <new>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxMatrixStack.h"

// STATIC: 0x00AEE008
uint32_t CMapObj::s_maxLiquidQueryDepth = 2;

// STATIC: 0x00ADFF50
static int32_t s_portalExtCount = 2048;

// STATIC: 0x00D1BEE8
static TSGrowableArray<C3Vector> s_portalVerts;

// STATIC: 0x00D1C3D4
CShaderEffect* CMapObj::s_unifiedShaders[7];

// STATIC: 0x00D1C3F0
CShaderEffect* CMapObj::s_classicShaders[7];

// STATIC: 0x00D1C40C
uint32_t* CMapObj::occluderHeap;

// STATIC: 0x00D1C454
TSHashTable<CMapObj, HASHKEY_NONE> CMapObj::mapObjHash;

// STATIC: 0x00D1C410
int32_t CMapObj::bPoolsDirty;

// STATIC: 0x00D1C41C
CMapObjDef* s_stampedMapObjDef;

// STATIC: 0x00D1C424
uint32_t s_portalStamp;

// STATIC: 0x00D1C428
static HASHKEY_NONE s_mapObjHashKey;

// STATIC: 0x00D1BED8
RENDER_CALLBACK CMapObj::gRenderCallback;

// STATIC: 0x00D1BEDC
void* CMapObj::gRenderUserParam;

// STATIC: 0x00ADFE44
int32_t CMapObj::s_renderMode = 5;

// STATIC: 0x00CFBEC4
void (CMapObj::*CMapObj::s_renderGroupExterior)(CMapObjGroup* group, uint32_t index);

// STATIC: 0x00CFBECC
void (CMapObj::*CMapObj::s_renderGroupInterior)(CMapObjGroup* group, uint32_t index);

// STATIC: 0x00ADFE50
C44Matrix s_modelViewProj;

// STATIC: 0x00ADFE90
C44Matrix s_viewProj;

// STATIC: 0x00ADFED0
C44Matrix s_modelView;

// STATIC: 0x00ADFF10
C44Matrix s_mapObjToWorld;

// STATIC: 0x00ADFF54
SPortalExt s_portalExt[2048];

// STATIC: 0x00CFBEB8
int32_t s_curGroupIsInterior;

// STATIC: 0x00CFBEBC
int32_t s_cullStateValid;

// STATIC: 0x00CFBEC0
int32_t s_interiorPass;

// STATIC: 0x00D1BEE4
uint32_t s_maxPortalDepth = 10;

// STATIC: 0x00D1C42C
C3Vector s_camPosLocal;

// STATIC: 0x00D1C438
C3Vector s_camTargetLocal;

// STATIC: 0x00D1C444
C4Plane s_camPlaneLocal;

// STATIC: 0x00CFBEA8
static int32_t s_shadowInterior;

// STATIC: 0x00CFBEB0
static int32_t s_fogMode;

// STATIC: 0x00CFBEB4
int32_t CWorldScene::s_fogPermute;

// STATIC: 0x00D1BEF8
static uint32_t s_lastSidnColor;

// STATIC: 0x00D1BEFC
static uint32_t s_emissiveAmbient;

// STATIC: 0x00CFBEAC
int32_t CMapObj::s_lightingMode;

// STATIC: 0x00D1BEE0
static uint32_t s_smolVertCacheSize;

// STATIC: 0x00D1BF00
static SMOLiquidVert s_smolVertCache[64];

// VERIFY: 7a37847b auto
// OFFSET: 0x007A6140
World::TriData::Batch* World::TriData::AllocBatch() {
    if (World::TriData::nBatches + 1 >= 32) {
        World::TriData::statusFlags |= 1;
        return nullptr;
    }

    Batch* batch = &World::TriData::batches[World::TriData::nBatches++];
    batch->m_matrix = nullptr;
    batch->m_vertexList = nullptr;
    batch->m_normalList = nullptr;
    batch->m_heightList = nullptr;
    batch->m_indices = nullptr;
    batch->m_faceIndices = nullptr;
    batch->m_indexCount = 0;
    batch->m_faceCount = 0;
    batch->m_minVertexIndex = 0;
    batch->m_maxVertexIndex = 0;
    batch->m_def = nullptr;
    batch->m_minVertexIndex = 0xFFFF;

    return batch;
}

// VERIFY: dd78a681 manual
// OFFSET: 0x007A6190
uint16_t* __stdcall World::TriData::AllocIndices(uint32_t count) {
    uint32_t cursor = World::TriData::indexCursor;

    if (cursor + count >= 0xC000) {
        World::TriData::statusFlags |= 1;
        return nullptr;
    }

    World::TriData::indexCursor += count;
    return &World::TriData::indexPool[cursor];
}

// VERIFY: b6bac718 auto
// OFFSET: 0x007A6B40
void CMapObj::SetGroupRenderCallback(RENDER_CALLBACK callback, void* param) {
    CMapObj::gRenderCallback = callback;
    CMapObj::gRenderUserParam = param;
}

// VERIFY: ade70db6 auto
// OFFSET: 0x007A6B60
void CMapObj::InvokeRenderCallback(uint32_t groupNum) {
    if (this->GetGroup(groupNum, 0) && CMapObj::gRenderCallback) {
        CMapObj::gRenderCallback(groupNum, CMapObj::gRenderUserParam);
    }
}

// VERIFY: dd043936 manual
// OFFSET: 0x007A6E00
void CMapObj::SetupMapObjDefCull(C44Matrix& mat, C44Matrix& invMat, C3Vector& worldPos, C3Vector& camTarget) {
    C44Matrix world;
    C3Vector offset(-worldPos.x, -worldPos.y, -worldPos.z);
    world.Translate(offset);
    world = mat * world;
    GxXformSet(GxXform_World, world);

    s_camPosLocal = worldPos * invMat;
    s_camTargetLocal = camTarget * invMat;

    C3Vector dir;
    dir.x = s_camTargetLocal.x - s_camPosLocal.x;
    dir.y = s_camTargetLocal.y - s_camPosLocal.y;
    dir.z = s_camTargetLocal.z - s_camPosLocal.z;
    float lengthSq = dir.z * dir.z + dir.y * dir.y + dir.x * dir.x;

    if (lengthSq > 0.0001f) {
        float scale = 1.0f / sqrtf(lengthSq);
        dir.x *= scale;
        dir.y *= scale;
        dir.z = scale * dir.z;
    }

    s_camPlaneLocal.n = dir;
    s_camPlaneLocal.d = -(s_camPosLocal.x * dir.x + s_camPosLocal.y * dir.y + s_camPosLocal.z * dir.z);

    GxXformViewProj(s_viewProj);

    C44Matrix view;
    GxXformView(view);
    s_modelView = world * view;
    s_modelViewProj = world * s_viewProj;
    s_mapObjToWorld = mat;

    if (this->m_interiorGroupCount == 0xFFFF) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_groupInfoCount); i++) {
            if (!(this->m_groupInfo[i].m_flags & 0x10008)) {
                this->m_interiorGroupCount++;
            }
        }
    }

    s_cullStateValid = 1;
}

// VERIFY: 2166e3e8 manual
// OFFSET: 0x007A70D0
float CMapObj::CalcPortalFarthestDistance(SMOPortal* portal) {
    float farthest = 0.0f;
    C3Vector* verts = &this->m_portalVertexList[portal->m_startVertex];

    for (uint32_t i = 0; i < portal->m_count; i++) {
        float dist = verts[i].y * s_camPlaneLocal.n.y + verts[i].z * s_camPlaneLocal.n.z + verts[i].x * s_camPlaneLocal.n.x + s_camPlaneLocal.d;

        if (dist > farthest) {
            farthest = dist;
        }
    }

    return farthest;
}

// VERIFY: ceb264eb manual
// OFFSET: 0x007A7210
void CMapObj::ClassifyPortalPlane(SMOPortal* portal, SPortalExt* portalExt) {
    float dist = portal->m_plane.n.y * s_camPosLocal.y + portal->m_plane.n.z * s_camPosLocal.z + portal->m_plane.n.x * s_camPosLocal.x + portal->m_plane.d;

    if (dist > -0.01f && dist < 0.01f && NTempest::Intersect(s_camPosLocal, &this->m_portalVertexList[portal->m_startVertex], portal->m_count, portal->m_plane.n.MajorAxis())) {
        portalExt->m_flags |= 0x2;
    }
}

// VERIFY: 2cc1b7e1 manual
// OFFSET: 0x007A72A0
void __stdcall CWorldScene::ClipVerts(C3Vector* verts, uint32_t count, C3Vector** outVerts, uint32_t* outCount) {
    static C3Vector s_clipVerts[32];
    static int32_t s_sides[17];
    static float s_dists[17];

    C3Vector* buffers[2] = { &s_clipVerts[0], &s_clipVerts[16] };
    uint32_t counts[2] = { count, 0 };

    for (uint32_t i = 0; i < count; i++) {
        s_clipVerts[i] = verts[i];
    }

    for (uint32_t plane = 0; ; ) {
        uint32_t src = plane & 1;
        uint32_t dst = (plane - 1) & 1;
        C3Vector* in = buffers[src];
        uint32_t inCount = counts[src];
        C4Plane& clip = CWorldScene::s_clipFrustum.planes[plane];

        for (uint32_t i = 0; i < inCount; i++) {
            float dist = in[i].x * clip.n.x + in[i].z * clip.n.z + in[i].y * clip.n.y + clip.d;
            s_dists[i] = dist;

            if (dist > 0.0001f) {
                s_sides[i] = 1;
            } else if (dist >= -0.0001f) {
                s_sides[i] = 0;
            } else {
                s_sides[i] = 2;
            }
        }

        s_sides[inCount] = s_sides[0];
        s_dists[inCount] = s_dists[0];

        if (!inCount) {
            break;
        }

        C3Vector* out = buffers[dst];
        uint32_t outCount2 = 0;

        for (uint32_t i = 0; i < inCount; i++) {
            if (s_sides[i] == 0) {
                out[outCount2++] = in[i];
                continue;
            }

            if (s_sides[i] == 1) {
                out[outCount2++] = in[i];
            }

            if (s_sides[i + 1] && s_sides[i + 1] != s_sides[i]) {
                C3Vector& next = in[(i + 1) % inCount];
                float t = s_dists[i] / (s_dists[i] - s_dists[i + 1]);
                out[outCount2].x = (next.x - in[i].x) * t + in[i].x;
                out[outCount2].y = (next.y - in[i].y) * t + in[i].y;
                out[outCount2].z = t * (next.z - in[i].z) + in[i].z;
                outCount2++;
            }
        }

        if (!outCount2) {
            break;
        }

        counts[dst] = outCount2;
        *outCount = outCount2;

        if (++plane >= 5) {
            *outVerts = buffers[dst];
            return;
        }
    }

    *outVerts = nullptr;
    *outCount = 0;
}

// VERIFY: 0d20ad08 manual
// OFFSET: 0x007A7630
bool CMapObj::CullBatch(SMOBatch* batch) {
    C3Vector t(batch->m_tx, batch->m_ty, batch->m_tz);
    CAaBox box;
    box.b.x = batch->m_bx;
    box.t = t;
    box.b.y = batch->m_by;
    box.b.z = batch->m_bz;
    return CWorldScene::FrustumCull(&box);
}

// VERIFY: bd88db75 auto
// OFFSET: 0x007A7B00
static void __stdcall WriteLiquidVertex(CMapObjGroup* group, const C44Matrix& matrix, const C3Vector& vertex, const SMOLiquidVert* liquidVert, const CImVector* color, int32_t magma, float texCoord1U, int32_t* unused, uint32_t stride, C3Vector** position, C3Vector** normal, CImVector** vertexColor, C2Vector** texCoord0, C2Vector** texCoord1) {
    DepthCoordTable* depthTable = CMap::GetDepthCoordTable(group->m_liquidType);

    if (*position) {
        **position = vertex * matrix;
        *position = reinterpret_cast<C3Vector*>(reinterpret_cast<uint8_t*>(*position) + stride);
    }

    if (*normal) {
        (*normal)->x = 0.0f;
        (*normal)->y = 0.0f;
        (*normal)->z = 1.0f;
        *normal = reinterpret_cast<C3Vector*>(reinterpret_cast<uint8_t*>(*normal) + stride);
    }

    if (*vertexColor) {
        CImVector value;

        if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
            value.b = color->r;
            value.g = color->g;
            value.r = color->b;
            value.a = color->a;
        } else {
            value = *color;
        }

        **vertexColor = value;
        *vertexColor = reinterpret_cast<CImVector*>(reinterpret_cast<uint8_t*>(*vertexColor) + stride);
    }

    if (*texCoord0) {
        if (magma) {
            (*texCoord0)->x = liquidVert->m_magmaVert.m_s * 0.00390625f;
            (*texCoord0)->y = liquidVert->m_magmaVert.m_t * 0.00390625f;
        } else {
            C44Matrix rotation = matrix;
            rotation.d0 = 0.0f;
            rotation.d1 = 0.0f;
            rotation.d2 = 0.0f;
            rotation.d3 = 1.0f;

            C3Vector local;
            local.x = vertex.x - group->m_liquidCorner.x;
            local.y = vertex.y - group->m_liquidCorner.y;
            local.z = vertex.z - group->m_liquidCorner.z;

            C2Vector uv(local * rotation);
            (*texCoord0)->x = uv.x * 0.24f;
            (*texCoord0)->y = 0.24f * uv.y;
        }

        *texCoord0 = reinterpret_cast<C2Vector*>(reinterpret_cast<uint8_t*>(*texCoord0) + stride);
    }

    if (*texCoord1) {
        float depth = depthTable ? (*depthTable)[liquidVert->m_waterVert.m_flow1] : 0.0f;
        (*texCoord1)->x = texCoord1U;
        (*texCoord1)->y = depth;
        *texCoord1 = reinterpret_cast<C2Vector*>(reinterpret_cast<uint8_t*>(*texCoord1) + stride);
    }
}

// VERIFY: be9e0132 manual
// OFFSET: 0x007A7E50
static void* PolyClipVtxLerpTexCoords(const void* a, const void* b, float t) {
    const uint16_t* from = static_cast<const uint16_t*>(a);
    const uint16_t* to = static_cast<const uint16_t*>(b);
    uint16_t* vert = reinterpret_cast<uint16_t*>(&s_smolVertCache[s_smolVertCacheSize++]);
    vert[0] = static_cast<uint16_t>(from[0] + (to[0] - from[0]) * t);
    vert[1] = static_cast<uint16_t>(t * (to[1] - from[1]) + from[1]);

    return vert;
}

// VERIFY: 734af3f1 manual
// OFFSET: 0x007A7F00
static void* PolyClipVtxLerpDepth(const void* a, const void* b, float t) {
    const uint8_t* from = static_cast<const uint8_t*>(a);
    const uint8_t* to = static_cast<const uint8_t*>(b);
    uint8_t* vert = reinterpret_cast<uint8_t*>(&s_smolVertCache[s_smolVertCacheSize++]);
    vert[0] = static_cast<uint8_t>(from[0] + (to[0] - from[0]) * t);

    return vert;
}

// VERIFY: 7cd73fec manual
// OFFSET: 0x007A7F60
uint16_t CMapObj::ClipGroupGeometry(CMapObjGroup* group, const C44Matrix& matrix, const CImVector* color, int32_t magma, float texCoord1U, uint32_t stride, C3Vector** position, C3Vector** normal, CImVector** vertexColor, C2Vector** texCoord0, C2Vector** texCoord1, uint16_t** indices, uint16_t baseVertex) {
    uint16_t nextIndex = baseVertex;
    uint16_t vertexCount = 0;
    void* (*lerp)(const void*, const void*, float) = *texCoord1 ? PolyClipVtxLerpDepth : PolyClipVtxLerpTexCoords;
    SMOPortalRef* refs = &this->m_portalRefList[group->m_portalStart];
    SMOLTile* tile = group->m_liquidTileList;

    for (int32_t y = 0; y < group->m_liquidTiles.y; y++) {
        for (int32_t x = 0; x < group->m_liquidTiles.x; x++, tile++) {
            if ((tile->m_flags & 0xF) == 0xF || !(tile->m_flags & 0x80)) {
                continue;
            }

            int32_t row0 = y * group->m_liquidVerts.x;
            int32_t row1 = row0 + group->m_liquidVerts.x;
            int32_t corners[4] = { x + row0, x + row1, x + row1 + 1, x + row0 + 1 };
            int32_t offsets[4][2] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

            PolyClipper clipper;
            C3Vector points[4];

            for (uint32_t i = 0; i < 4; i++) {
                SMOLiquidVert* liquidVert = &group->m_liquidVertexList[corners[i]];
                points[i].x = (x + offsets[i][0]) * 4.1666665f + group->m_liquidCorner.x;
                points[i].y = 4.1666665f * (y + offsets[i][1]) + group->m_liquidCorner.y;
                points[i].z = liquidVert->m_waterVert.m_height;
                clipper.AddVertex(points[i], liquidVert);
            }

            clipper.Begin();

            for (int32_t p = 0; p < group->m_portalCount; p++) {
                SMOPortalRef& ref = refs[p];
                CMapObjGroup* other = this->GetGroup(ref.m_groupIndex, 0);

                if (other && other->m_liquidCorner.x < points[2].x && other->m_liquidCorner.y < points[2].y && other->m_liquidTiles.x * 4.1666665f + other->m_liquidCorner.x > points[0].x && 4.1666665f * other->m_liquidTiles.y + other->m_liquidCorner.y > points[0].y) {
                    clipper.Clip(this->m_portalList[ref.m_portalIndex].m_plane, ref.m_side);
                }
            }

            clipper.End();
            s_smolVertCacheSize = 0;
            int32_t started = 0;

            for (PCResultsIterator it = clipper.GetResults(lerp); !it.HasFinished(); it.Next()) {
                C3Vector vertex(0.0f, 0.0f, 0.0f);
                const void* data;
                it.GetVertex(vertex, data);

                if (!started) {
                    *(*indices)++ = nextIndex;
                    started = 1;
                }

                int32_t unused;
                WriteLiquidVertex(group, matrix, vertex, static_cast<const SMOLiquidVert*>(data), color, magma, texCoord1U, &unused, stride, position, normal, vertexColor, texCoord0, texCoord1);
                vertexCount++;
                *(*indices)++ = nextIndex;
                nextIndex++;
            }

            if (started) {
                *(*indices)++ = nextIndex - 1;
            }
        }
    }

    return vertexCount;
}

// VERIFY: 228c7c03 auto
// OFFSET: 0x007A8430
void CMapObj::SetEmissiveAmbient(CImVector color) {
    s_emissiveAmbient = *reinterpret_cast<uint32_t*>(&color);
}

// VERIFY: b05f7aa9 manual
// OFFSET: 0x007A8440
static void SetShaderFogFromDayNight(int32_t mode) {
    if (s_fogMode == mode) {
        return;
    }

    s_fogMode = mode;

    if (!mode) {
        CShaderEffect::SetFogEnabled(0);
        return;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    DNFogInfo* fog = (mode & 0x2) ? &info->m_fog : &info->m_fogInterior;
    CImVector black;
    *reinterpret_cast<uint32_t*>(&black) = 0xFF000000;
    CImVector color = (mode & 0x4) ? black : fog->m_color;
    CShaderEffect::SetFogParams(fog->m_start, fog->m_end, fog->m_density, color);
    CShaderEffect::SetFogEnabled(1);
}

// VERIFY: 3cc915d8 auto
// OFFSET: 0x007A84D0
void CMapObj::SelectWorldShaders() {
    if (!CShaderEffect::s_enableShaders) {
        return;
    }

    uint32_t shadow = CShaderEffect::s_shadowValue;

    if (shadow > 2) {
        shadow = 2;
    }

    uint32_t vertexPermute = (CMapObj::s_lightingMode != 0) + 2 * (CWorldScene::s_fogPermute + 15 * shadow);
    CShaderEffect::SetShaders(vertexPermute, CShaderEffect::SelectShadowShader());
}

// VERIFY: 0016a158 manual
// OFFSET: 0x007A8520
void CMapObj::UpdateMaterials() {
    DayNight::DNInfo* info = DayNight::GetInfo();

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_materialsCount); i++) {
        if (this->m_materialList[i].m_flags & 0x10) {
            this->m_materialList[i].m_frameSidnColor = this->m_materialList[i].m_sidnColor;

            int32_t scale = CMath::fint_(info->m_curve0 * 255.0f - 0.5f);
            CImVector& color = this->m_materialList[i].m_frameSidnColor;
            uint8_t r = static_cast<uint8_t>((scale * color.r) >> 8);
            uint8_t g = static_cast<uint8_t>((scale * color.g) >> 8);
            uint8_t b = static_cast<uint8_t>((scale * color.b) >> 8);
            color.b = b;
            color.g = g;
            color.r = r;
            color.a = 0;
        }
    }
}

// VERIFY: b4a27992 manual
// OFFSET: 0x007A85E0
uint32_t CMapObj::TransformAndClipVerts(uint32_t noOcclusion, C3Vector* verts, uint32_t vertCount, C3Vector& offset, C3Vector*& clippedVerts, uint32_t& clippedCount) {
    static C3Vector s_worldVerts[12];

    if (vertCount > 12) {
        vertCount = 12;
    }

    for (uint32_t i = 0; i < vertCount; i++) {
        C3Vector vert(verts[i].x + offset.x, verts[i].y + offset.y, offset.z + verts[i].z);
        s_worldVerts[i] = vert;
        s_worldVerts[i] = s_worldVerts[i] * s_mapObjToWorld;
    }

    if (CWorldOcclusion::GetClipVolumeCount() && !noOcclusion && CWorldOcclusion::QueryVolumes(s_worldVerts, vertCount)) {
        clippedVerts = nullptr;
        clippedCount = 0;
        return 1;
    }

    CWorldScene::ClipVerts(s_worldVerts, vertCount, &clippedVerts, &clippedCount);

    if (clippedCount < 3) {
        clippedVerts = nullptr;
        clippedCount = 0;
        return 1;
    }

    for (uint32_t i = 0; i < clippedCount; i++) {
        C4Vector vert;
        vert.x = clippedVerts[i].x - CWorldScene::s_activeWorldView.x;
        vert.y = clippedVerts[i].y - CWorldScene::s_activeWorldView.y;
        vert.z = clippedVerts[i].z - CWorldScene::s_activeWorldView.z;
        vert.w = 1.0f;
        vert = vert * s_viewProj;

        float w = 0.0001f;

        if (vert.w >= 0.0001f) {
            w = vert.w;
        } else {
            vert.w = 0.0001f;
        }

        float scale = 1.0f / w;
        clippedVerts[i].x = scale * vert.x;
        clippedVerts[i].y = scale * vert.y;
        clippedVerts[i].z = vert.z;
    }

    return 0;
}

// VERIFY: d30bcd8f manual
// OFFSET: 0x007A8940
void CMapObj::SetEmissiveColor(CImVector color) {
    color.a = 0;
    uint32_t value = *reinterpret_cast<uint32_t*>(&color);

    if (s_lastSidnColor == value) {
        return;
    }

    s_lastSidnColor = value;

    CImVector diffuse;
    *reinterpret_cast<uint32_t*>(&diffuse) = 0xFF7F7F7F;

    if (CShaderEffect::s_enableShaders) {
        C3Vector& specular = CMap::s_mapLight->m_light.m_specColor;
        C4Vector specularConst;
        specularConst.x = specular.x;
        specularConst.y = specular.y;
        specularConst.z = specular.z;
        specularConst.w = 14.0f;
        C4Vector colors[2] = { C4Vector(diffuse), C4Vector(color) };
        g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 28, &colors[0].x, 2);
        g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 13, &specularConst.x, 1);
    } else {
        GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0xFF7F7F7F));
        GxRsSet(GxRs_MatEmissive, static_cast<int32_t>(value));
    }
}

// VERIFY: 7e911082 manual
// OFFSET: 0x007A8B10
void CMapObjGroup::SetLighting(CMapObjGroup* group, uint32_t mode) {
    if (CMapObj::s_lightingMode == mode) {
        return;
    }

    CMapObj::s_lightingMode = mode;
    DayNight::DNInfo* info = DayNight::GetInfo();
    C3Vector ambient(0.0f, 0.0f, 0.0f);
    C3Vector diffuse(0.0f, 0.0f, 0.0f);

    switch (mode) {
        case 1:
            diffuse = C3Vector(info->m_light1.m_diffuse.r * 0.0039215689f, info->m_light1.m_diffuse.g * 0.0039215689f, info->m_light1.m_diffuse.b * 0.0039215689f);
            ambient = C3Vector(info->m_light1.m_ambient.r * 0.0039215689f, info->m_light1.m_ambient.g * 0.0039215689f, 0.0039215689f * info->m_light1.m_ambient.b);
            break;

        case 2:
            diffuse = C3Vector(info->m_light1.m_mid0.r * 0.0039215689f, info->m_light1.m_mid0.g * 0.0039215689f, info->m_light1.m_mid0.b * 0.0039215689f);
            ambient = C3Vector(info->m_light1.m_mid1.r * 0.0039215689f, info->m_light1.m_mid1.g * 0.0039215689f, 0.0039215689f * info->m_light1.m_mid1.b);
            break;

        case 3: {
            CImVector& color = group->m_parent->m_argbColor;
            ambient = C3Vector(color.r * 0.0039215689f, color.g * 0.0039215689f, 0.0039215689f * color.b);
            break;
        }

        default:
            break;
    }

    CM2Light* light = &CMap::s_mapLight->m_light;

    if (CShaderEffect::s_enableShaders) {
        if (mode) {
            C44Matrix view;
            GxXformView(view);
            C3Vector dir = TransformDirection(light->m_dir, view);
            C4Vector ambientConst;
            ambientConst.x = ambient.x;
            ambientConst.y = ambient.y;
            ambientConst.z = ambient.z;
            ambientConst.w = 0.0f;
            C4Vector diffuseConst;
            diffuseConst.x = diffuse.x;
            diffuseConst.y = diffuse.y;
            diffuseConst.z = diffuse.z;
            diffuseConst.w = 0.0f;
            C4Vector dirConst;
            dirConst.x = dir.x;
            dirConst.y = dir.y;
            dirConst.z = dir.z;
            dirConst.w = 0.0f;
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 9, &ambientConst.x, 1);
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 10, &ambientConst.x, 1);
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 11, &diffuseConst.x, 1);
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 12, &dirConst.x, 1);
        } else {
            static C4Vector s_unlit = [] {
                C4Vector unlit;
                unlit.x = 0.0f;
                unlit.y = 0.0f;
                unlit.z = 0.0f;
                unlit.w = 0.5f;
                return unlit;
            }();
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 11, &s_unlit.x, 1);
        }
    } else if (mode) {
        GxRsSet(GxRs_Lighting, static_cast<int32_t>(1));
        C3Vector dirColor = light->m_dirColor;
        C3Vector ambColor = light->m_ambColor;
        light->m_dirColor = diffuse;
        light->m_ambColor = ambient;
        light->GxLightSet(0);
        light->m_dirColor = dirColor;
        light->m_ambColor = ambColor;
    } else {
        GxRsSet(GxRs_Lighting, static_cast<int32_t>(0));
    }
}

// VERIFY: 725c9d38 auto
// OFFSET: 0x007A8F20
void CMapObj::AddExteriorPortalView(SMOPortal* portal, SMOPortalRef* ref, SPortalExt* ext, uint32_t destIsExterior) {
    uint16_t flags = ext->m_flags;

    if (flags & 0x4) {
        return;
    }

    flags |= 0x4;
    ext->m_flags = flags;

    C3Vector offset(portal->m_plane.n.x * 0.01f, portal->m_plane.n.y * 0.01f, 0.01f * portal->m_plane.n.z);

    if (ref->m_side > 0) {
        offset = C3Vector(-offset.x, -offset.y, -offset.z);
    }

    C3Vector* clipped = nullptr;
    uint32_t clippedCount = 0;
    ext->m_flags |= this->TransformAndClipVerts((flags >> 4) & 1, &this->m_portalVertexList[portal->m_startVertex], portal->m_count, offset, clipped, clippedCount);

    if (clippedCount > 2) {
        CPortalView view;
        CWorldScene::CalcScreenRectFromVerts(view.m_rect, clipped, clippedCount);
        view.m_rect.minX = (view.m_rect.minX + 1.0f) * 0.5f;
        view.m_rect.maxX = (view.m_rect.maxX + 1.0f) * 0.5f;
        view.m_rect.minY = (view.m_rect.minY + 1.0f) * 0.5f;
        view.m_rect.maxY = 0.5f * (view.m_rect.maxY + 1.0f);
        view.m_maxViewDepth = this->CalcPortalFarthestDistance(portal);
        CWorldScene::PushExteriorPortalView(&view);
        CWorldScene::MergeIntoWorldRect(&view);

        if (destIsExterior) {
            CWorldScene::MergeIntoFrustumRect(&view);
        }
    }
}

// VERIFY: 9de874f4 manual
// OFFSET: 0x007A9090
void CMapObj::TransformPortal(SMOPortal* portal, SPortalExt* portalExt) {
    this->ClassifyPortalPlane(portal, portalExt);

    C3Vector* clipped = nullptr;
    uint32_t clippedCount = 0;
    uint16_t flags = portalExt->m_flags;

    if (!(flags & 0x2)) {
        C3Vector offset(0.0f, 0.0f, 0.0f);
        portalExt->m_flags |= this->TransformAndClipVerts((flags >> 4) & 1, &this->m_portalVertexList[portal->m_startVertex], portal->m_count, offset, clipped, clippedCount);
    }

    if (portalExt->m_flags & 0x2) {
        portalExt->m_rect.minX = -1.0f;
        portalExt->m_rect.maxX = 1.0f;
        portalExt->m_rect.maxY = 1.0f;
        portalExt->m_rect.minY = -1.0f;
    } else if (portalExt->m_flags & 0x1) {
        portalExt->m_rect.minX = 3.4028235e38f;
        portalExt->m_rect.maxX = -3.4028235e38f;
        portalExt->m_rect.maxY = -3.4028235e38f;
        portalExt->m_rect.minY = 3.4028235e38f;
    } else {
        CWorldScene::CalcScreenRectFromVerts(portalExt->m_rect, clipped, clippedCount);
    }
}

// VERIFY: b0d6c3f6 auto
// OFFSET: 0x007A9160
void CWorldScene::SetupLighting(CM2Lighting* lighting, C3Vector* view) {
    if (CShaderEffect::s_enableShaders) {
        CWorldScene::s_fogPermute = lighting->m_lightCount;

        if (CWorldScene::s_fogPermute) {
            LocalLights localLights;
            CShaderEffect::ComputeLocalLights(&localLights, CWorldScene::s_fogPermute, lighting->m_lights, view);
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 17, reinterpret_cast<float*>(&localLights), 11);
        }
    } else {
        lighting->SetupGxLights(view);
    }

    s_emissiveAmbient = 0;
}

// VERIFY: fbeb8080 auto
// OFFSET: 0x007A9200
void CMapObj::AddInteriorPortalView(SMOPortal* portal, SMOPortalRef* ref, SPortalExt* ext, TSGrowableArray<CPortalView>* views) {
    uint16_t flags = ext->m_flags;

    if (flags & 0xC) {
        return;
    }

    C3Vector offset(portal->m_plane.n.x * 0.01f, portal->m_plane.n.y * 0.01f, 0.01f * portal->m_plane.n.z);

    if (ref->m_side > 0) {
        offset = C3Vector(-offset.x, -offset.y, -offset.z);
    }

    C3Vector* clipped = nullptr;
    uint32_t clippedCount = 0;
    ext->m_flags |= this->TransformAndClipVerts((flags >> 4) & 1, &this->m_portalVertexList[portal->m_startVertex], portal->m_count, offset, clipped, clippedCount);

    if (clippedCount > 2) {
        CPortalView view;
        CWorldScene::CalcScreenRectFromVerts(view.m_rect, clipped, clippedCount);
        C3Vector* dst = &s_portalVerts.m_data[s_portalVerts.m_count];
        view.m_vertCount = clippedCount;
        view.m_rect.minX = (view.m_rect.minX + 1.0f) * 0.5f;
        view.m_verts = dst;
        view.m_rect.maxX = (view.m_rect.maxX + 1.0f) * 0.5f;
        view.m_rect.minY = (view.m_rect.minY + 1.0f) * 0.5f;
        view.m_rect.maxY = 0.5f * (view.m_rect.maxY + 1.0f);
        s_portalVerts.SetCount(clippedCount + s_portalVerts.m_count);
        memcpy(dst, clipped, sizeof(C3Vector) * clippedCount);
        views->Add(1, &view);
    }

    ext->m_flags |= 0x8;
}

// VERIFY: 8052ccf3 manual
// OFFSET: 0x007A9380
void CMapObj::UnifiedRender(CMapObjGroup* group, uint32_t index) {
    group->m_timer = 0.0f;
    group->AllocVB();
    group->SetIndexVB();
    group->SetVertexVB();
    g_theGxDevicePtr->RsPush();

    s_fogMode = -1;
    CMapObj::s_lightingMode = -1;
    s_lastSidnColor = 0xFFFFFFFF;
    s_shadowInterior = -1;

    if (!CShaderEffect::s_enableShaders) {
        GxRsSet(GxRs_ColorMaterial, static_cast<int32_t>(2));
    }

    int32_t fogMode = 2 - (s_curGroupIsInterior != 0);
    CGxTex* defaultTex = nullptr;

    if (CMap::s_isStreamingMode) {
        defaultTex = TextureGetGxTex(CWorldScene::s_defaultTexture, 1, nullptr);
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_batchListCount); i++) {
        SMOBatch& batch = group->m_batchList[i];

        if (!index) {
            batch.m_flags &= 0xF;
        }

        if ((batch.m_flags & 0xF0) || CMapObj::CullBatch(&batch)) {
            continue;
        }

        batch.m_flags |= 0xF0;
        SMOMaterial& material = this->m_materialList[batch.m_texture];
        CTexture* texture = material.Textures()[0];
        CGxTex* gxTex0 = TextureGetGxTex(texture, 0, nullptr);

        if (!gxTex0) {
            if (!defaultTex) {
                continue;
            }

            gxTex0 = defaultTex;
        }

        CGxTex* gxTex1 = nullptr;

        if (material.Textures()[1]) {
            gxTex1 = TextureGetGxTex(material.Textures()[1], 0, nullptr);

            if (!gxTex1) {
                if (!defaultTex) {
                    continue;
                }

                gxTex1 = defaultTex;
            }
        }

        uint32_t shader = material.m_shader;

        if (!shader && !material.m_blendMode && !TextureIsOpaque(texture)) {
            shader = 4;
        }

        GxRsSet(GxRs_Culling, static_cast<int32_t>((material.m_flags & 0x4) == 0));

        uint32_t sidn = (material.m_flags & 0x10) ? *reinterpret_cast<uint32_t*>(&material.m_frameSidnColor) : 0;
        uint32_t sum = s_emissiveAmbient + sidn;
        uint32_t carry = (sum ^ s_emissiveAmbient ^ sidn) & 0x1010100;
        uint32_t saturated = (carry - (carry >> 8)) | (sum - carry);
        CImVector emissive = *reinterpret_cast<CImVector*>(&saturated);
        emissive.g >>= 1;
        emissive.b >>= 1;
        emissive.r >>= 1;
        CMapObj::SetEmissiveColor(emissive);

        GxTexSetWrap(gxTex0, static_cast<EGxTexWrapMode>((material.m_flags & 0x40) == 0), static_cast<EGxTexWrapMode>((material.m_flags & 0x80) == 0));
        CShaderEffect* effect = CMapObj::s_unifiedShaders[shader];
        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex0);
        g_theGxDevicePtr->RsSet(GxRs_Texture1, gxTex1);
        effect->SetCurrent();

        if (i >= group->m_transparencyBatchesCount) {
            if (group->m_flags & 0x48) {
                CMapObjGroup::SetLighting(group, (material.m_flags & 0x1) == 0);
                if (s_shadowInterior) {
                    s_shadowInterior = 0;
                    CShadowCache::SetShadowMapGenericInterior(0);
                    CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue();
                }

                SetShaderFogFromDayNight(2);
            } else {
                if (material.m_flags & 0x20) {
                    CMapObjGroup::SetLighting(group, 2);
                } else {
                    CMapObjGroup::SetLighting(group, 3);
                }

                if (s_shadowInterior != 1) {
                    s_shadowInterior = 1;
                    CShadowCache::SetShadowMapGenericInterior(1);
                    CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue() != 0;
                }

                SetShaderFogFromDayNight(fogMode);
            }

            GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(material.m_blendMode));
            CShaderEffect::SetAlphaRefDefault();
            CMapObj::SelectWorldShaders();
            CGxBatch draw;
            draw.m_primType = GxPrim_Triangles;
            draw.m_start = batch.m_indexStart;
            draw.m_count = batch.m_indexCount;
            draw.m_minIndex = batch.m_vertexStart;
            draw.m_maxIndex = batch.m_vertexEnd;
            g_theGxDevicePtr->Draw(&draw, 1);
            continue;
        }

        SetShaderFogFromDayNight((material.m_flags & 0x2) == 0 ? fogMode : 0);
        if (s_shadowInterior) {
            s_shadowInterior = 0;
            CShadowCache::SetShadowMapGenericInterior(0);
            CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue();
        }

        if (CShaderEffect::s_enableShaders) {
            CMapObjGroup::SetLighting(group, (material.m_flags & 0x1) ? 0 : ((material.m_flags & 0x20) != 0) + 1);
            SetShaderFogFromDayNight(~material.m_flags & 0x2);
            GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(9));
            CShaderEffect::SetAlphaRefDefault();
            CMapObj::SelectWorldShaders();
            CGxBatch draw;
            draw.m_primType = GxPrim_Triangles;
            draw.m_start = batch.m_indexStart;
            draw.m_count = batch.m_indexCount;
            draw.m_minIndex = batch.m_vertexStart;
            draw.m_maxIndex = batch.m_vertexEnd;
            g_theGxDevicePtr->Draw(&draw, 1);

            CMapObjGroup::SetLighting(group, 3);
            SetShaderFogFromDayNight((material.m_flags & 0x2) == 0 ? fogMode : 0);
            if (s_shadowInterior != 1) {
                s_shadowInterior = 1;
                CShadowCache::SetShadowMapGenericInterior(1);
                CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue() != 0;
            }

            GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(7));
            CShaderEffect::SetAlphaRefDefault();
            CMapObj::SelectWorldShaders();
            CGxBatch draw2;
            draw2.m_primType = GxPrim_Triangles;
            draw2.m_start = batch.m_indexStart;
            draw2.m_count = batch.m_indexCount;
            draw2.m_minIndex = batch.m_vertexStart;
            draw2.m_maxIndex = batch.m_vertexEnd;
            g_theGxDevicePtr->Draw(&draw2, 1);
            continue;
        }

        GxRsSet(GxRs_ColorMaterial, static_cast<int32_t>(0));
        group->SetTransparencyVB();
        CMapObjGroup::SetLighting(group, (material.m_flags & 0x1) ? 0 : ((material.m_flags & 0x20) != 0) + 1);
        SetShaderFogFromDayNight((material.m_flags & 0x2) ? 0 : 6);
        GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(9));
        CShaderEffect::SetAlphaRefDefault();
        CGxBatch draw;
        draw.m_primType = GxPrim_Triangles;
        draw.m_start = batch.m_indexStart;
        draw.m_count = batch.m_indexCount;
        draw.m_minIndex = batch.m_vertexStart;
        draw.m_maxIndex = batch.m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw, 1);

        CMapObjGroup::SetLighting(group, 3);
        SetShaderFogFromDayNight((material.m_flags & 0x2) ? 0 : (fogMode | 4));
        GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(7));
        CShaderEffect::SetAlphaRefDefault();
        CGxBatch draw2;
        draw2.m_primType = GxPrim_Triangles;
        draw2.m_start = batch.m_indexStart;
        draw2.m_count = batch.m_indexCount;
        draw2.m_minIndex = batch.m_vertexStart;
        draw2.m_maxIndex = batch.m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw2, 1);

        group->SetVertexVB();
        CMapObjGroup::SetLighting(group, 0);
        SetShaderFogFromDayNight((material.m_flags & 0x2) == 0 ? fogMode : 0);
        GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(10));
        CShaderEffect::SetAlphaRefDefault();
        CGxBatch draw3;
        draw3.m_primType = GxPrim_Triangles;
        draw3.m_start = batch.m_indexStart;
        draw3.m_count = batch.m_indexCount;
        draw3.m_minIndex = batch.m_vertexStart;
        draw3.m_maxIndex = batch.m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw3, 1);

        GxRsSet(GxRs_ColorMaterial, static_cast<int32_t>(2));
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 9c6db61b manual
// OFFSET: 0x007ABF50
void CMapObj::RenderGroup(int32_t groupIndex, C44Matrix& matrix, TSExplicitList<CWFrustum, offsetof(CWFrustum, m_link)>* frustumList) {
    CMapObjGroup* group = this->GetGroup(groupIndex, 0);
    DebugLog("%s %s", this->m_wmoName, group->m_groupName);

    if (!(group->m_unkLoadedFlag & 0x2)) {
        this->AttenTransVerts(group);
    }

    CShaderEffect::SetGlobalLighting(&CMap::s_mapLight->m_light);
    CShaderEffect::UpdateProjMatrix();

    uint32_t index = 0;

    for (CWFrustum* frustum = frustumList->Head(); frustum; frustum = frustumList->Next(frustum)) {
        CWorldScene::FrustumSet(frustum);
        CWorldScene::FrustumXform(matrix);

        if (group->m_colorVertexList) {
            (this->*CMapObj::s_renderGroupInterior)(group, index);
        } else {
            (this->*CMapObj::s_renderGroupExterior)(group, index);
        }

        index++;
    }

    if (CWorld::enables & 0x40000000) {
        this->RenderNormals(group);
    }

    if (CWorld::enables & Enable_Portals) {
        this->RenderPortals(group);
    }
}

// VERIFY: 73f59e5b manual
// OFFSET: 0x007AC060
void CMapObj::RenderThruPortals(uint32_t groupNum, uint32_t fromGroup, CRect& ndcRect, uint32_t depth, int32_t interior) {
    if (depth > s_maxPortalDepth) {
        return;
    }

    CMapObjGroup* group = this->GetGroup(groupNum, 0);

    if (!group || (group->m_flags & 0x10000)) {
        return;
    }

    if (interior && (group->m_flags & 0x48)) {
        interior = 0;
    }

    s_curGroupIsInterior = interior;

    if (s_interiorPass && (group->m_flags & 0x40000)) {
        CWorldScene::s_interiorSkybox = this->m_skybox;
    }

    if (CMapObj::gRenderCallback) {
        CMapObj::gRenderCallback(groupNum, CMapObj::gRenderUserParam);
    }

    if (!group->m_portalCount) {
        return;
    }

    static TSGrowableArray<CPortalView> s_pendingPortalViews;
    static TSGrowableArray<CRect> s_coveredRects;

    if (!s_interiorPass && !depth) {
        s_pendingPortalViews.SetCount(0);
        s_coveredRects.SetCount(0);
    }

    SMOPortalRef* ref = &this->m_portalRefList[group->m_portalStart];

    for (uint32_t i = 0; i < group->m_portalCount; i++, ref++) {
        uint32_t destGroup = ref->m_groupIndex;

        if (destGroup == 0xFFFF) {
            continue;
        }

        SMOPortal* portal = &this->m_portalList[ref->m_portalIndex];

        if (destGroup == fromGroup) {
            continue;
        }

        uint32_t destFlags = this->GetGroupFlags(destGroup);
        SPortalExt* ext = &s_portalExt[ref->m_portalIndex];

        if (ext->m_stamp != s_portalStamp) {
            ext->m_stamp = s_portalStamp;
            ext->m_flags = 0;

            if (!(destFlags & 0x8) && !(group->m_flags & 0x8)) {
                ext->m_flags = 0x10;
            }

            this->TransformPortal(portal, ext);
        }

        float side = portal->m_plane.n.y * s_camPosLocal.y + portal->m_plane.n.z * s_camPosLocal.z + portal->m_plane.n.x * s_camPosLocal.x + portal->m_plane.d;

        if (ref->m_side < 0) {
            side = -side;
        }

        if (side < 0.0f) {
            if (!s_interiorPass && !depth) {
                CRect covered;
                covered.minY = 0.0f;
                covered.minX = 0.0f;
                covered.maxY = 1.0f;
                covered.maxX = 1.0f;
                s_coveredRects.Add(1, &covered);
            }

            continue;
        }

        if ((!(ext->m_flags & 0x2) && (ext->m_flags & 0x1)) || ndcRect.maxX < ext->m_rect.minX || ndcRect.minX > ext->m_rect.maxX || ndcRect.maxY < ext->m_rect.minY || ndcRect.minY > ext->m_rect.maxY) {
            continue;
        }

        CRect clip = ext->m_rect;

        if (clip.minX < ndcRect.minX) {
            clip.minX = ndcRect.minX;
        }

        if (clip.maxX > ndcRect.maxX) {
            clip.maxX = ndcRect.maxX;
        }

        if (ndcRect.minY > clip.minY) {
            clip.minY = ndcRect.minY;
        }

        if (clip.maxX > ndcRect.maxX) {
            clip.maxX = ndcRect.maxX;
        }

        if (fequalz(clip.minX, clip.maxX, 0.001f) || fequalz(clip.minY, clip.maxY, 0.001f)) {
            continue;
        }

        if (s_interiorPass) {
            if (destFlags & 0x50148) {
                this->AddExteriorPortalView(portal, ref, ext, destFlags & 0x10008);

                if (destFlags & 0x10008) {
                    continue;
                }
            }
        } else {
            if (destFlags & 0x10008) {
                continue;
            }

            if (!depth && s_cullStateValid && !(destFlags & 0x140)) {
                this->AddInteriorPortalView(portal, ref, ext, &s_pendingPortalViews);
            }
        }

        CRect one;
        one.minY = 1.0f;
        one.minX = 1.0f;
        one.maxY = 1.0f;
        one.maxX = 1.0f;
        CRect two;
        two.minY = 2.0f;
        two.minX = 2.0f;
        two.maxY = 2.0f;
        two.maxX = 2.0f;
        CRect frustumRect = (clip + one) / two;

        CWorldScene::FrustumPush();
        CWorldScene::FrustumSet(CWorldScene::s_frustumCorners, frustumRect);
        this->RenderThruPortals(destGroup, groupNum, clip, depth + 1, interior);
        CWorldScene::FrustumPop();
    }

    if (depth || !s_pendingPortalViews.Count()) {
        return;
    }

    for (uint32_t i = 0; i < s_pendingPortalViews.Count(); i++) {
        CPortalView& view = s_pendingPortalViews[i];
        uint32_t j;

        for (j = 0; j < s_coveredRects.Count(); j++) {
            CRect& covered = s_coveredRects[j];

            if (view.m_rect.maxX >= covered.minX && view.m_rect.maxY >= covered.minY && view.m_rect.minX <= covered.maxX && view.m_rect.minY <= covered.maxY) {
                break;
            }
        }

        if (j >= s_coveredRects.Count()) {
            CWorldScene::PushPortalView(&view);
        }
    }

    s_pendingPortalViews.SetCount(0);
    s_coveredRects.SetCount(0);
}

// VERIFY: 687366c6 manual
// OFFSET: 0x007AC6A0
void CMapObj::ExteriorRender(CMapObjGroup* group, uint32_t index) {
    if (group->m_parent->m_header->m_flags & 0x2) {
        this->UnifiedRender(group, index);
        return;
    }

    group->m_timer = 0.0f;
    group->AllocVB();
    group->SetIndexVB();
    group->SetVertexVB();
    g_theGxDevicePtr->RsPush();

    s_fogMode = -1;
    CMapObj::s_lightingMode = -1;
    s_lastSidnColor = 0xFFFFFFFF;
    s_shadowInterior = -1;

    CGxTex* defaultTex = nullptr;

    if (CMap::s_isStreamingMode) {
        defaultTex = TextureGetGxTex(CWorldScene::s_defaultTexture, 1, nullptr);
    }

    for (uint32_t i = 0; i < group->m_extBatchCount; i++) {
        SMOBatch& batch = group->m_batchList[i];

        if (!index) {
            batch.m_flags &= 0xF;
        }

        if ((batch.m_flags & 0xF0) || CMapObj::CullBatch(&batch)) {
            continue;
        }

        batch.m_flags |= 0xF0;
        SMOMaterial& material = this->m_materialList[batch.m_texture];
        CTexture* texture = material.Textures()[0];
        CGxTex* gxTex0 = TextureGetGxTex(texture, 0, nullptr);

        if (!gxTex0) {
            if (!defaultTex) {
                continue;
            }

            gxTex0 = defaultTex;
        }

        CGxTex* gxTex1 = defaultTex;

        if (material.Textures()[1]) {
            gxTex1 = TextureGetGxTex(material.Textures()[1], 0, nullptr);

            if (!gxTex1) {
                if (!defaultTex) {
                    continue;
                }

                gxTex1 = defaultTex;
            }
        }

        uint32_t shader = material.m_shader;

        if (!shader && !material.m_blendMode && !TextureIsOpaque(texture)) {
            shader = 4;
        }

        SetShaderFogFromDayNight(~material.m_flags & 0x2);
        CMapObjGroup::SetLighting(group, (material.m_flags & 0x1) == 0);

        if (group->m_flags & 0x48) {
            if (s_shadowInterior) {
                s_shadowInterior = 0;
                CShadowCache::SetShadowMapGenericInterior(0);
                CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue();
            }
        } else {
            if (s_shadowInterior != 1) {
                s_shadowInterior = 1;
                CShadowCache::SetShadowMapGenericInterior(1);
                CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue() != 0;
            }
        }

        GxRsSet(GxRs_Culling, static_cast<int32_t>((material.m_flags & 0x4) == 0));

        uint32_t sidn = (material.m_flags & 0x10) ? *reinterpret_cast<uint32_t*>(&material.m_frameSidnColor) : 0;
        uint32_t sum = s_emissiveAmbient + sidn;
        uint32_t carry = (sum ^ s_emissiveAmbient ^ sidn) & 0x1010100;
        uint32_t saturated = (carry - (carry >> 8)) | (sum - carry);
        CImVector emissive = *reinterpret_cast<CImVector*>(&saturated);
        emissive.g >>= 1;
        emissive.b >>= 1;
        emissive.r >>= 1;
        CMapObj::SetEmissiveColor(emissive);

        GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(material.m_blendMode));
        CShaderEffect::SetAlphaRefDefault();
        GxTexSetWrap(gxTex0, static_cast<EGxTexWrapMode>((material.m_flags & 0x40) == 0), static_cast<EGxTexWrapMode>((material.m_flags & 0x80) == 0));
        CShaderEffect* effect = CMapObj::s_classicShaders[shader];
        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex0);
        g_theGxDevicePtr->RsSet(GxRs_Texture1, gxTex1);
        effect->SetCurrent();
        CMapObj::SelectWorldShaders();
        CGxBatch draw;
        draw.m_primType = GxPrim_Triangles;
        draw.m_start = batch.m_indexStart;
        draw.m_count = batch.m_indexCount;
        draw.m_minIndex = batch.m_vertexStart;
        draw.m_maxIndex = batch.m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 8a73759b manual
// OFFSET: 0x007AC9F0
void CMapObj::InteriorRender(CMapObjGroup* group, uint32_t index) {
    if (group->m_parent->m_header->m_flags & 0x2) {
        this->UnifiedRender(group, index);
        return;
    }

    group->m_timer = 0.0f;
    group->AllocVB();
    group->SetIndexVB();
    group->SetVertexVB();
    g_theGxDevicePtr->RsPush();

    s_fogMode = -1;
    CMapObj::s_lightingMode = -1;
    s_lastSidnColor = 0xFFFFFFFF;
    s_shadowInterior = -1;

    int32_t fogMode = 2 - (s_curGroupIsInterior != 0);
    CGxTex* defaultTex = nullptr;

    if (CMap::s_isStreamingMode) {
        defaultTex = TextureGetGxTex(CWorldScene::s_defaultTexture, 1, nullptr);
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_batchListCount); i++) {
        SMOBatch& batch = group->m_batchList[i];

        if (!index) {
            batch.m_flags &= 0xF;
        }

        if ((batch.m_flags & 0xF0) || CMapObj::CullBatch(&batch)) {
            continue;
        }

        batch.m_flags |= 0xF0;
        SMOMaterial& material = this->m_materialList[batch.m_texture];
        CTexture* texture = material.Textures()[0];
        CGxTex* gxTex0 = TextureGetGxTex(texture, 0, nullptr);

        if (!gxTex0) {
            if (!defaultTex) {
                continue;
            }

            gxTex0 = defaultTex;
        }

        CGxTex* gxTex1 = nullptr;

        if (material.Textures()[1]) {
            gxTex1 = TextureGetGxTex(material.Textures()[1], 0, nullptr);

            if (!gxTex1) {
                if (!defaultTex) {
                    continue;
                }

                gxTex1 = defaultTex;
            }
        }

        uint32_t shader = material.m_shader;

        if (!shader && !material.m_blendMode && !TextureIsOpaque(texture)) {
            shader = 4;
        }

        GxRsSet(GxRs_Culling, static_cast<int32_t>((material.m_flags & 0x4) == 0));

        uint32_t sidn = (material.m_flags & 0x10) ? *reinterpret_cast<uint32_t*>(&material.m_frameSidnColor) : 0;
        uint32_t sum = s_emissiveAmbient + sidn;
        uint32_t carry = (sum ^ s_emissiveAmbient ^ sidn) & 0x1010100;
        uint32_t saturated = (carry - (carry >> 8)) | (sum - carry);
        CImVector emissive = *reinterpret_cast<CImVector*>(&saturated);
        emissive.g >>= 1;
        emissive.b >>= 1;
        emissive.r >>= 1;
        CMapObj::SetEmissiveColor(emissive);

        GxTexSetWrap(gxTex0, static_cast<EGxTexWrapMode>((material.m_flags & 0x40) == 0), static_cast<EGxTexWrapMode>((material.m_flags & 0x80) == 0));
        CShaderEffect* effect = CMapObj::s_classicShaders[shader];
        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex0);
        g_theGxDevicePtr->RsSet(GxRs_Texture1, gxTex1);
        effect->SetCurrent();

        if (i >= group->m_transparencyBatchesCount) {
            bool interiorBatch = i < static_cast<uint32_t>(group->m_transparencyBatchesCount + group->m_intBatchCount);
            SetShaderFogFromDayNight((material.m_flags & 0x2) == 0 ? fogMode : 0);

            if (interiorBatch) {
                CMapObjGroup::SetLighting(group, 0);
            } else {
                CMapObjGroup::SetLighting(group, ((material.m_flags & 0x20) != 0) + 1);
            }

            if (s_shadowInterior != 1) {
                s_shadowInterior = 1;
                CShadowCache::SetShadowMapGenericInterior(1);
                CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue() != 0;
            }

            GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(material.m_blendMode));
            CShaderEffect::SetAlphaRefDefault();
            CMapObj::SelectWorldShaders();
            CGxBatch draw;
            draw.m_primType = GxPrim_Triangles;
            draw.m_start = batch.m_indexStart;
            draw.m_count = batch.m_indexCount;
            draw.m_minIndex = batch.m_vertexStart;
            draw.m_maxIndex = batch.m_vertexEnd;
            g_theGxDevicePtr->Draw(&draw, 1);
            continue;
        }

        if (s_shadowInterior) {
            s_shadowInterior = 0;
            CShadowCache::SetShadowMapGenericInterior(0);
            CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue();
        }

        CMapObjGroup::SetLighting(group, ((material.m_flags & 0x20) != 0) + 1);
        SetShaderFogFromDayNight(~material.m_flags & 0x2);
        GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(9));
        CShaderEffect::SetAlphaRefDefault();
        CMapObj::SelectWorldShaders();
        CGxBatch draw;
        draw.m_primType = GxPrim_Triangles;
        draw.m_start = batch.m_indexStart;
        draw.m_count = batch.m_indexCount;
        draw.m_minIndex = batch.m_vertexStart;
        draw.m_maxIndex = batch.m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw, 1);

        CMapObjGroup::SetLighting(group, 0);
        SetShaderFogFromDayNight((material.m_flags & 0x2) == 0 ? fogMode : 0);
        if (s_shadowInterior != 1) {
            s_shadowInterior = 1;
            CShadowCache::SetShadowMapGenericInterior(1);
            CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue() != 0;
        }

        GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(7));
        CShaderEffect::SetAlphaRefDefault();
        CMapObj::SelectWorldShaders();
        CGxBatch draw2;
        draw2.m_primType = GxPrim_Triangles;
        draw2.m_start = batch.m_indexStart;
        draw2.m_count = batch.m_indexCount;
        draw2.m_minIndex = batch.m_vertexStart;
        draw2.m_maxIndex = batch.m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw2, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 9b1bf024 manual
// OFFSET: 0x007AD020
void CMapObj::PrepareUpdate() {
    s_portalStamp++;
    CWorldScene::s_curMapObjDef = nullptr;
    s_stampedMapObjDef = nullptr;
    s_portalVerts.SetCount(0);

    switch (CMapObj::s_renderMode) {
        case 0:
            CMapObj::s_renderGroupExterior = &CMapObj::RenderGroupCollisionFaces;
            CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupCollisionFaces;
            break;

        case 1:
            CMapObj::s_renderGroupExterior = &CMapObj::RenderGroupDetailFaces;
            CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupDetailFaces;
            break;

        case 2:
            CMapObj::s_renderGroupExterior = &CMapObj::RenderGroupRenderFaces;
            CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupRenderFaces;
            break;

        case 3:
            CMapObj::s_renderGroupExterior = &CMapObj::RenderGroupTransFaces;
            CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupTransFaces;
            break;

        case 4:
            CMapObj::s_renderGroupExterior = &CMapObj::RenderGroupCollidable;
            CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupCollidable;
            break;

        default:
            CMapObj::s_renderGroupExterior = &CMapObj::ExteriorRender;
            CMapObj::s_renderGroupInterior = &CMapObj::InteriorRender;

            if (!(CWorld::enables & Enable_MapObjTex)) {
                CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupLightmapTex;
            } else if (!(CWorld::enables & Enable_MapObjLight)) {
                CMapObj::s_renderGroupInterior = &CMapObj::RenderGroupColorTex;
            }

            break;
    }

    CMapObj* mapObj = CMapObj::mapObjHash.m_fulllist.Head();

    while (mapObj) {
        CMapObj* next = CMapObj::mapObjHash.Next(mapObj);
        mapObj->UpdateMaterials();

        CMapObjGroup* group = mapObj->m_mapObjGroupList.Head();

        while (group) {
            CMapObjGroup* nextGroup = mapObj->m_mapObjGroupList.Next(group);
            group->m_timer += FrameTime::s_tickTimeSec;

            if (group->m_timer > 5.0f) {
                group->FreeVB();
            }

            group = nextGroup;
        }

        if (!mapObj->m_refCount) {
            mapObj->m_flushTimer += FrameTime::s_tickTimeSec;

            if (mapObj->m_flushTimer > 10.0f) {
                CMapObj::mapObjHash.Unlink(mapObj);
                CMap::FreeMapObj(mapObj);
            }
        }

        mapObj = next;
    }

    if (CMap::s_isStreamingMode) {
        CMapObj::ProcessAsyncLoadQueue();
    }
}

// VERIFY: d0b199c6 manual
// OFFSET: 0x007AD1F0
void CMapObj::RenderInterior(C44Matrix& mat, C44Matrix& invMat, C3Vector& worldPos, C3Vector& camTarget, TSGrowableArray<uint32_t>* groups) {
    this->SetupMapObjDefCull(mat, invMat, worldPos, camTarget);
    s_interiorPass = 1;
    CWorldScene::s_unk_CD8620 = 0;

    if (CWorldScene::s_curMapObjDef != s_stampedMapObjDef) {
        s_portalStamp++;
        s_stampedMapObjDef = CWorldScene::s_curMapObjDef;
    }

    CWorldScene::FrustumPush();

    CRect ndcRect;
    ndcRect.minY = -1.0f;
    ndcRect.minX = -1.0f;
    ndcRect.maxY = 1.0f;
    ndcRect.maxX = 1.0f;

    for (uint32_t i = 0; i < groups->Count(); i++) {
        CMapObjGroup* group = this->GetGroup(groups->m_data[i], 0);

        if (!group) {
            continue;
        }

        if (!(group->m_flags & 0x48)) {
            CWorldScene::s_unk_CD8620 = 1;
        }

        this->RenderThruPortals(groups->m_data[i], 0xFFFF, ndcRect, 0, 1);
    }

    CWorldScene::FrustumPop();

    CAaBox box;
    box.b = C3Vector(0.0f, 0.0f, 0.0f);
    box.t = C3Vector(0.0f, 0.0f, 0.0f);

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_groupInfoCount); i++) {
        SMOGroupInfo& info = this->m_groupInfo[i];

        if (!(info.m_flags & 0x10000)) {
            continue;
        }

        CWorldMath::TransformAABox(mat, info.m_boundingBox, box);

        if (CWorldScene::FrustumCull(&box)) {
            continue;
        }

        if (this->GetGroup(i, 0) && CMapObj::gRenderCallback) {
            CMapObj::gRenderCallback(i, CMapObj::gRenderUserParam);
        }
    }
}

// VERIFY: 8c94ba81 auto
// OFFSET: 0x007AD350
void CMapObj::RenderThruPortalsExterior(C44Matrix& mat, C44Matrix& invMat, C3Vector& worldPos, C3Vector& camTarget, CRect& ndcRect, int32_t groupNum) {
    this->SetupMapObjDefCull(mat, invMat, worldPos, camTarget);
    s_interiorPass = 0;

    if (CWorldScene::s_curMapObjDef != s_stampedMapObjDef) {
        s_portalStamp++;
        s_stampedMapObjDef = CWorldScene::s_curMapObjDef;
    }

    this->RenderThruPortals(groupNum, 0xFFFF, ndcRect, 0, 0);
}

// VERIFY: cf5959ca auto
// OFFSET: 0x007AE030
void CMapObj::Delete(CMapObj* mapObj) {
    if (--mapObj->m_refCount <= 0) {
        mapObj->m_flushTimer = 0.0f;
    }
}

// VERIFY: 38e3657c auto
// OFFSET: 0x007AE060
void CMapObj::UpdatePools() {
    CMapObj::bPoolsDirty = 0;
}

// VERIFY: 1dd1286b auto
// OFFSET: 0x007AE070
void CMapObj::InitPtrs() {
    this->m_header = nullptr;
    this->m_textureNameList = nullptr;
    this->m_groupNameList = nullptr;
    this->m_groupInfo = nullptr;
    this->m_portalVertexList = nullptr;
    this->m_portalList = nullptr;
    this->m_portalRefList = nullptr;
    this->m_lightList = nullptr;
    this->m_doodadSetList = nullptr;
    this->m_doodadNameList = nullptr;
    this->m_doodadDefList = nullptr;
    this->m_fogList = nullptr;
    this->m_convexVolumePlanes = nullptr;
    this->m_texturesSize = 0;
    this->m_groupNameSize = 0;
    this->m_groupInfoCount = 0;
    this->m_planeVertCount = 0;
    this->m_portalsCount = 0;
    this->m_portalRefCount = 0;
    this->m_ligtsCount = 0;
    this->m_doodadSetCount = 0;
    this->m_doodadNameSize = 0;
    this->m_doodadDefCount = 0;
    this->m_fogsCount = 0;
    this->m_convexVolumePlaneCount = 0;
}

// VERIFY: 692709d1 manual
// OFFSET: 0x007AE110
void CMapObj::AsyncFailedCallback(CAsyncObject* object) {
    void* buffer = object->m_buffer;
    AsyncFileReadDestroyObject(object);
    SMemFree(buffer, __FILE__, __LINE__, 0);
}

// VERIFY: f7f5970e auto
// OFFSET: 0x007AE1A0
void CMapObj::ReadGroup(uint32_t index) {
    this->ReadGroup(index, false);
}

// VERIFY: d92cbc0c auto
// OFFSET: 0x007AE1C0
void CMapObj::WaitLoad() {
    while (this->m_asyncObject) {
        AsyncFileReadWait(this->m_asyncObject);
    }
}

// VERIFY: edb456aa manual
// OFFSET: 0x007AE300
void CMapObj::Init() {
    this->m_flushTimer = 0.0f;
    this->m_refCount = 0;
    this->m_pWmoData = nullptr;
    this->m_bbox.b = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_distToCamera = 3.4028235e38f;
    this->m_bbox.t = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_wmoFileSize = 0;
    this->m_asyncObject = nullptr;
    this->m_isGroupLoaded = 0;
    this->m_interiorGroupCount = 0xFFFF;
    this->m_materialList = nullptr;
    this->m_materialsCount = 0;
    this->m_mapObjGroupCount = 0;
    this->InitPtrs();
}

// VERIFY: b756cecb manual
// OFFSET: 0x007AE3B0
void CMapObj::Clear() {
    int32_t freeData = 1;

    if (this->m_asyncObject) {
        freeData = AsyncFile::Cancel(this->m_asyncObject, CMapObj::AsyncFailedCallback);
        this->m_asyncObject = nullptr;
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_materialsCount); i++) {
        for (uint32_t j = 0; j < 2; j++) {
            if (this->m_materialList[i].Textures()[j]) {
                HandleClose(reinterpret_cast<HOBJECT>(this->m_materialList[i].Textures()[j]));
                this->m_materialList[i].Textures()[j] = nullptr;
            }
        }
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_mapObjGroupCount); i++) {
        CMap::FreeMapObjGroup(this->m_mapObjGroupArray[i]);
        this->m_mapObjGroupArray[i] = nullptr;
    }

    this->m_mapObjGroupCount = 0;
    this->InitPtrs();

    if (this->m_pWmoData) {
        if (freeData) {
            SMemFree(this->m_pWmoData, __FILE__, __LINE__, 0);
        }

        this->m_pWmoData = nullptr;
    }

    this->m_wmoFileSize = 0;
}

// VERIFY: c6f5dc77 manual
// OFFSET: 0x007AE4C0
int32_t CMapObj::IsGroupLoaded(int32_t index) {
    if (this->m_isGroupLoaded) {
        return this->m_mapObjGroupArray[index]->m_unkLoadedFlag & 1;
    }

    return 0;
}

// VERIFY: 2a027261 manual
// OFFSET: 0x007AE4F0
int32_t CMapObj::IsGroupLoading(int32_t index) {
    return this->m_isGroupLoaded && this->m_mapObjGroupArray[index]->m_asyncObjPtr != nullptr;
}

// VERIFY: bd150d57 manual
// OFFSET: 0x007AE520
void CMapObj::GetBounds(CAaSphere& sphere) {
    if (this->m_isGroupLoaded) {
        float sumY = this->m_bbox.t.y + this->m_bbox.b.y;
        float sumZ = this->m_bbox.t.z + this->m_bbox.b.z;
        sphere.c.x = (this->m_bbox.t.x + this->m_bbox.b.x) * 0.5f;
        sphere.c.y = sumY * 0.5f;
        sphere.c.z = 0.5f * sumZ;

        float dz = this->m_bbox.t.z - sphere.c.z;
        float dy = this->m_bbox.t.y - sphere.c.y;
        float dx = this->m_bbox.t.x - sphere.c.x;
        sphere.r = sqrt(dx * dx + (dy * dy + dz * dz));
    } else {
        sphere.c.x = 0.0f;
        sphere.r = 0.0f;
        sphere.c.y = 0.0f;
        sphere.c.z = 0.0f;
    }
}

// VERIFY: ebd0cfad manual
// OFFSET: 0x007AE5E0
void CMapObj::GetBounds(CAaBox* box) {
    if (this->m_isGroupLoaded) {
        *box = this->m_bbox;
    } else {
        box->b = C3Vector(0.0f, 0.0f, 0.0f);
        box->t = C3Vector(0.0f, 0.0f, 0.0f);
    }
}

// VERIFY: 3ce41eea manual
// OFFSET: 0x007AE670
void CMapObj::GetGroupBounds(CAaSphere* sphere, int32_t index) {
    if (this->m_isGroupLoaded) {
        CAaBox& box = this->m_groupInfo[index].m_boundingBox;
        float sumY = box.t.y + box.b.y;
        float sumZ = box.t.z + box.b.z;
        sphere->c.x = (box.t.x + box.b.x) * 0.5f;
        sphere->c.y = sumY * 0.5f;
        sphere->c.z = 0.5f * sumZ;

        float dz = box.t.z - sphere->c.z;
        float dy = box.t.y - sphere->c.y;
        float dx = box.t.x - sphere->c.x;
        sphere->r = sqrt(dx * dx + (dy * dy + dz * dz));
    } else {
        sphere->c.x = 0.0f;
        sphere->r = 0.0f;
        sphere->c.y = 0.0f;
        sphere->c.z = 0.0f;
    }
}

// VERIFY: a237d260 manual
// OFFSET: 0x007AE720
void CMapObj::GetGroupBounds(CAaBox* box, int32_t index) {
    if (this->m_isGroupLoaded) {
        *box = this->m_groupInfo[index].m_boundingBox;
    } else {
        box->b.x = 0.0f;
        box->b.y = 0.0f;
        box->t.x = 0.0f;
        box->b.z = 0.0f;
        box->t.y = 0.0f;
        box->t.z = 0.0f;
    }
}

// VERIFY: edac55fb manual
// OFFSET: 0x007AE7B0
uint32_t CMapObj::GetGroupFlags(int32_t index) {
    if (this->m_isGroupLoaded) {
        return this->m_groupInfo[index].m_flags;
    }

    return 0;
}

// VERIFY: 8c25cdca manual
// OFFSET: 0x007AE7E0
bool CMapObj::TestBounds(CAaBox& box) {
    return this->m_isGroupLoaded && this->m_bbox.Intersects(box);
}

// VERIFY: 1571ed48 manual
// OFFSET: 0x007AE810
bool CMapObj::ContainsPointInBounds(C3Vector& point) {
    return this->m_isGroupLoaded && this->m_bbox.ContainsPoint(point);
}

// VERIFY: 8e7e27fe manual
// OFFSET: 0x007AE840
bool CMapObj::TestBounds(C3Vector& start, C3Vector& end) {
    return this->m_isGroupLoaded && CWorldMath::VectorIntersectAABox2(this->m_bbox, start, end);
}

// VERIFY: 735b2379 auto
// OFFSET: 0x007AE880
bool CMapObj::TestGroupBounds(C3Vector& start, C3Vector& end, uint32_t groupNum) {
    return this->m_isGroupLoaded && (this->m_mapObjGroupArray[groupNum]->m_unkLoadedFlag & 1) && CWorldMath::VectorIntersectAABox2(this->m_groupInfo[groupNum].m_boundingBox, start, end);
}

// VERIFY: 6fd4dc44 auto
// OFFSET: 0x007AE970
bool CMapObj::GroupBoundingBoxIntersectsSphere(C3Vector& pos, uint32_t groupNum, float radius) {
    if (!this->m_isGroupLoaded || !(this->m_mapObjGroupArray[groupNum]->m_unkLoadedFlag & 1)) {
        return false;
    }

    CAaBox& box = this->m_groupInfo[groupNum].m_boundingBox;

    if (pos.x + radius < box.b.x || pos.x - radius > box.t.x || pos.y + radius < box.b.y || pos.y - radius > box.t.y || radius + pos.z < box.b.z || pos.z - radius > box.t.z) {
        return false;
    }

    return true;
}

// VERIFY: fa51d8fa manual
// OFFSET: 0x007AEA80
CMapObjGroup* CMapObj::GetGroup(int32_t index, int32_t a3) {
    if (!this->m_isGroupLoaded) {
        return nullptr;
    }

    CMapObjGroup* group = this->m_mapObjGroupArray[index];

    if (!(group->m_unkLoadedFlag & 1) && !a3) {
        return nullptr;
    }

    return group;
}

// VERIFY: 0d13eb33 auto
// OFFSET: 0x007AEAB0
void CMapObj::WaitLoadGroup(uint32_t index) {
    CMapObjGroup* group = this->m_mapObjGroupArray[index];

    while (group->m_asyncObjPtr) {
        AsyncFileReadWait(group->m_asyncObjPtr);
    }
}

// VERIFY: ca64417c auto
// OFFSET: 0x007AEAE0
char* CMapObj::GetGroupName(int32_t index) {
    if (this->m_isGroupLoaded) {
        CMapObjGroup* group = this->m_mapObjGroupArray[index];

        if (group->m_unkLoadedFlag & 1) {
            return group->m_groupName;
        }
    }

    return nullptr;
}

// VERIFY: 5d16452e manual
// OFFSET: 0x007AEB10
SMOGroupInfo* CMapObj::GetGroupInfo(int32_t index) {
    if (this->m_isGroupLoaded) {
        return &this->m_groupInfo[index];
    }

    return nullptr;
}

// VERIFY: 3afaacbc manual
// OFFSET: 0x007AEC30
int32_t CMapObj::GetDoodadSet(uint32_t index) {
    if (!this->m_isGroupLoaded || index >= this->m_doodadDefCount) {
        return -1;
    }

    for (uint32_t i = 0; i < this->m_doodadSetCount; i++) {
        SMODoodadSet& set = this->m_doodadSetList[i];

        if (set.m_count && index >= set.m_startIdx && index <= set.m_count + set.m_startIdx - 1) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 9a315686 auto
// OFFSET: 0x007AF280
bool CMapObj::VectorIntersectPortal(const C3Segment& seg, float* t, uint32_t* groups, int32_t useSphere) {
    C3Ray ray;
    ray.origin = seg.b;
    float dx = seg.t.x - seg.b.x;
    float dy = seg.t.y - seg.b.y;
    float dz = seg.t.z - seg.b.z;
    float length = sqrtf(dz * dz + dy * dy + dx * dx);
    float inv = 1.0f / length;
    ray.dir = C3Vector(dx * inv, dy * inv, inv * dz);

    float maxT = length * *t;
    bool found = false;

    for (int32_t g = 0; g < this->m_groupInfoCount; g++) {
        bool hit;

        if (useSphere) {
            hit = this->GroupBoundingBoxIntersectsSphere(const_cast<C3Vector&>(seg.b), g, 0.0099999998f);
        } else {
            if (!this->m_isGroupLoaded || !(this->m_mapObjGroupArray[g]->m_unkLoadedFlag & 1)) {
                continue;
            }

            hit = CWorldMath::VectorIntersectAABox2(this->m_groupInfo[g].m_boundingBox, const_cast<C3Vector&>(seg.b), const_cast<C3Vector&>(seg.t));
        }

        if (!hit || !this->m_isGroupLoaded) {
            continue;
        }

        CMapObjGroup* group = this->m_mapObjGroupArray[g];

        if (!(group->m_unkLoadedFlag & 1)) {
            continue;
        }

        SMOPortalRef* ref = &this->m_portalRefList[group->m_portalStart];

        for (int32_t i = 0; i < group->m_portalCount; i++, ref++) {
            SMOPortal& portal = this->m_portalList[ref->m_portalIndex];
            C3Vector point(0.0f, 0.0f, 0.0f);
            float portalT;

            if (!NTempest::Intersect(ray, portal.m_plane, &portalT, &point, 0.1f) || portalT < 0.0f || portalT > maxT) {
                continue;
            }

            C3Vector::EAxis axis = portal.m_plane.n.MajorAxis();

            if (!NTempest::Intersect(point, &this->m_portalVertexList[portal.m_startVertex], portal.m_count, axis)) {
                continue;
            }

            maxT = portalT;
            found = true;

            float side = portal.m_plane.n.y * seg.b.y + portal.m_plane.n.z * seg.b.z + portal.m_plane.n.x * seg.b.x + portal.m_plane.d;

            if ((side >= 0.0f && ref->m_side <= 0) || (side < 0.0f && ref->m_side > 0)) {
                groups[0] = ref->m_groupIndex;
                groups[1] = g;
            } else {
                groups[0] = g;
                groups[1] = ref->m_groupIndex;
            }
        }
    }

    if (found) {
        *t = maxT * inv;
    }

    return found;
}

// VERIFY: 69522e9c manual
// OFFSET: 0x007AFEE0
void CMapObj::Initialize() {
    s_portalStamp = 0;
    CWorldScene::s_curMapObjDef = nullptr;
    s_stampedMapObjDef = nullptr;
    s_portalExtCount = 2048;
    s_portalVerts.Reserve(1024, 0);
    s_portalVerts.m_chunk = 1024;

    CMapObj::s_classicShaders[0] = CShaderEffectManager::GetEffect("MapObjDiffuse");
    CMapObj::s_classicShaders[1] = CShaderEffectManager::GetEffect("MapObjSpecular");
    CMapObj::s_classicShaders[2] = CShaderEffectManager::GetEffect("MapObjMetal");
    CMapObj::s_classicShaders[3] = CShaderEffectManager::GetEffect("MapObjEnv");
    CMapObj::s_classicShaders[4] = CShaderEffectManager::GetEffect("MapObjOpaque");
    CMapObj::s_classicShaders[5] = CShaderEffectManager::GetEffect("MapObjEnvMetal");
    CMapObj::s_classicShaders[6] = nullptr;

    CMapObj::s_unifiedShaders[0] = CShaderEffectManager::GetEffect("MapObjUDiffuse");
    CMapObj::s_unifiedShaders[1] = CShaderEffectManager::GetEffect("MapObjUSpecular");
    CMapObj::s_unifiedShaders[2] = CShaderEffectManager::GetEffect("MapObjUMetal");
    CMapObj::s_unifiedShaders[3] = CShaderEffectManager::GetEffect("MapObjUEnv");
    CMapObj::s_unifiedShaders[4] = CShaderEffectManager::GetEffect("MapObjUOpaque");
    CMapObj::s_unifiedShaders[5] = CShaderEffectManager::GetEffect("MapObjUEnvMetal");
    CMapObj::s_unifiedShaders[6] = CShaderEffectManager::GetEffect("MapObjUComposite");

    CMapObj::bPoolsDirty = 1;
    CMapObj::occluderHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CMapObjOccluder), 128, "MAPOBJOCC", true));
}

// VERIFY: ff23c0be manual
// OFFSET: 0x007B0160
CMapObjOccluder* CMapObj::AllocOccluderNode(uint32_t* heap) {
    uint32_t memHandle;
    void* mem = nullptr;

    if (!ObjectAlloc(*heap, &memHandle, &mem, false)) {
        return nullptr;
    }

    CMapObjOccluder* occluder = static_cast<CMapObjOccluder*>(mem);

    if (occluder) {
        occluder->m_start = C3Vector(0.0f, 0.0f, 0.0f);
        occluder->m_link.m_prevLink = nullptr;
        occluder->m_link.m_next = nullptr;
        occluder->m_end = C3Vector(0.0f, 0.0f, 0.0f);
    }

    occluder->m_memHandle = memHandle;
    return occluder;
}

// VERIFY: 81a97e96 auto
// OFFSET: 0x007B01C0
void CMapObj::Destroy() {
    CMapObj::ClearCache(1);
    CMapObjGroup::Destroy();

    for (uint32_t i = 0; i < sizeof(CMapObj::s_classicShaders) / sizeof(CMapObj::s_classicShaders[0]); i++) {
        CMapObj::s_classicShaders[i] = nullptr;
    }

    for (uint32_t i = 0; i < sizeof(CMapObj::s_unifiedShaders) / sizeof(CMapObj::s_unifiedShaders[0]); i++) {
        CMapObj::s_unifiedShaders[i] = nullptr;
    }

    CShaderEffectManager::DestroyAllEffects();

    if (CMapObj::occluderHeap) {
        SMemFree(CMapObj::occluderHeap, "delete", SLOG_FUNCTION, 0);
        CMapObj::occluderHeap = nullptr;
    }
}

// VERIFY: e6f8b5e3 manual
// OFFSET: 0x007B0040
void CMapObj::ClearCache(int32_t unused_a1) {
    CMapObj* mapObj = CMapObj::mapObjHash.m_fulllist.Head();

    while (mapObj) {
        CMapObj* next = CMapObj::mapObjHash.Next(mapObj);

        if (!mapObj->m_refCount) {
            CMapObj::mapObjHash.Unlink(mapObj);
            CMap::FreeMapObj(mapObj);
        }

        mapObj = next;
    }
}







// VERIFY: 0c534f5e manual
// OFFSET: 0x007A8800
void CMapObj::SetRenderModeLight() {
    CGxLight light;
    light.m_flags |= 0x1;

    float d = 1.0f / sqrtf(3.0f);
    light.m_dir = { d, d, d };
    light.m_ambientColor = { 0.33f, 0.33f, 0.33f };
    light.m_dirColor = { 0.75f, 0.75f, 0.75f };
    light.m_specularColor = { 0.0f, 0.0f, 0.0f };
    light.m_constantAttenuation = 0.0f;
    light.m_linearAttenuation = 0.0f;
    light.m_quadraticAttenuation = 0.0f;

    C3Vector origin = { 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->LightSet(0, light, origin);
    g_theGxDevicePtr->LightEnable(0, 1);
    g_theGxDevicePtr->LightEnable(0, 1);

    for (int i = 1; i < 4; i++)
        g_theGxDevicePtr->LightEnable(i, 0);

    GxRsSet(GxRs_Lighting, static_cast<int32_t>(1));
}






// VERIFY: 51ea4524 manual
// OFFSET: 0x007AE140
uint32_t CMapObj::CreateWmoIgnoreFlags(uint32_t flags) {
    uint32_t result = 0xEE;
    if ((flags & 0x80u) != 0)
        return 0x2CA;
    if ((flags & 0x10) != 0)
        result = 0xC6;
    if ((flags & 0x20) != 0)
        result &= 0xDBu;
    if ((flags & 0x40) == 0)
        result &= ~2u;
    if ((flags & 0x4000) == 0)
        result &= ~0x40u;
    if ((flags & 0x1000000) != 0) {
        if ((flags & 0x40000000) != 0)
            return 0xFFFFFCDB;
        else
            return result | 0x100;
    }
    return result;
}



























// VERIFY: 688ff659 manual
// OFFSET: 0x0079BBF0
void CMapObj::QueryLiquidSounds(uint32_t groupIndex, uint32_t fromGroup, uint32_t depth, uint32_t* minDepth, const C3Vector& pos, TSGrowableArray<World::LiquidQueryResult>& results) {
    if (depth > CMapObj::s_maxLiquidQueryDepth) {
        return;
    }

    if (!this->GetGroupInfo(groupIndex)->m_boundingBox.ContainsPoint(pos)) {
        return;
    }

    CMapObjGroup* group = this->GetGroup(groupIndex, 0);

    if (!group) {
        return;
    }

    if ((group->m_flags & 0x8) && depth < *minDepth) {
        *minDepth = depth;
    }

    group->QueryLiquidSounds(pos, results);

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_portalCount); i++) {
        uint32_t neighbor = this->m_portalRefList[group->m_portalStart + i].m_groupIndex;

        if (neighbor != 0xFFFF && neighbor != fromGroup) {
            this->QueryLiquidSounds(neighbor, groupIndex, depth + 1, minDepth, pos, results);
        }
    }
}

// VERIFY: 6114c894 auto
// OFFSET: 0x007A6D70
SMOMaterial* CMapObj::GetLiquidMaterial(int32_t groupIndex) {
    CMapObjGroup* group = this->GetGroup(groupIndex, 0);

    if (!group) {
        return nullptr;
    }

    return &this->m_materialList[static_cast<uint16_t>(group->m_liquidMaterialId)];
}

// VERIFY: 64db3118 auto
// OFFSET: 0x007AEB90
bool CMapObj::QueryLiquidStatus(uint32_t ignoreFlags, const C3Vector& pos, int32_t* liquidType, float* level) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_groupInfoCount); i++) {
        if (!this->m_isGroupLoaded) {
            continue;
        }

        SMOGroupInfo* info = &this->m_groupInfo[i];

        if (!info || (ignoreFlags & info->m_flags) || !info->m_boundingBox.ContainsPoint(pos)) {
            continue;
        }

        CMapObjGroup* group = this->m_mapObjGroupArray[i];

        if ((group->m_unkLoadedFlag & 0x1) && group->QueryLiquid(pos, liquidType, level)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 928e1617 manual
// OFFSET: 0x007A7920
void CMapObj::BuildLiquidIndices(CMapObjGroup* group, const TSGrowableArray<uint8_t>* sharedVerts, uint16_t** indices, uint16_t baseVertex) {
    int32_t rowStride = group->m_liquidVerts.x + sharedVerts[0].m_count;
    uint16_t lastIndex = 0;
    int32_t inStrip = 0;
    uint32_t sharedRow = 0;
    int32_t outRow = 0;

    for (int32_t y = 0; y < group->m_liquidTiles.y; y++) {
        int32_t duplicate = 0;

        if (sharedRow < sharedVerts[1].m_count && sharedVerts[1].m_data[sharedRow] == y) {
            duplicate = 1;
            sharedRow++;
        }

        int32_t nextRow = baseVertex + rowStride;

        for (int32_t r = duplicate + 1; r; r--) {
            uint16_t top = outRow * rowStride + baseVertex;
            uint16_t bottom = nextRow + top;
            uint32_t sharedCol = 0;

            for (int32_t x = 0; x < group->m_liquidTiles.x; x++) {
                uint8_t flags = group->m_liquidTileList[group->m_liquidTiles.x * y + x].m_flags;
                int32_t visible = (flags & 0xF) != 0xF && !(flags & 0x80) ? 1 : 0;
                int32_t duplicateCol = 0;

                if (sharedCol < sharedVerts[0].m_count && sharedVerts[0].m_data[sharedCol] == x) {
                    duplicateCol = 1;
                    sharedCol++;
                }

                uint16_t bottomNext = bottom + 1;

                for (int32_t c = duplicateCol + 1; c; c--) {
                    if (visible) {
                        if (!inStrip) {
                            inStrip = 1;
                            *(*indices)++ = top;
                            *(*indices)++ = top;
                            *(*indices)++ = bottom;
                        }

                        *(*indices)++ = top + 1;
                        **indices = bottomNext;
                        lastIndex = bottomNext;
                        (*indices)++;
                    } else if (inStrip) {
                        **indices = lastIndex;
                        inStrip = 0;
                        (*indices)++;
                    }

                    top++;
                    bottom++;
                    bottomNext++;
                }
            }

            if (inStrip) {
                *(*indices)++ = lastIndex;
                inStrip = 0;
            }

            outRow++;
        }
    }
}

// VERIFY: 5e3b42d7 manual
// OFFSET: 0x007A7CC0
uint16_t CMapObj::BuildLiquidVerts(CMapObjGroup* group, const C44Matrix& matrix, const CImVector* color, int32_t magma, float texCoord1U, const TSGrowableArray<uint8_t>* sharedVerts, uint32_t stride, C3Vector** position, C3Vector** normal, CImVector** vertexColor, C2Vector** texCoord0, C2Vector** texCoord1) {
    float y = group->m_liquidCorner.y;
    uint32_t sharedRow = 0;

    for (int32_t row = 0; row < group->m_liquidVerts.y; row++) {
        int32_t duplicateRow = 0;

        if (sharedRow < sharedVerts[1].m_count && sharedVerts[1].m_data[sharedRow] == row) {
            duplicateRow = 1;
            sharedRow++;
        }

        for (int32_t r = duplicateRow + 1; r; r--) {
            float x = group->m_liquidCorner.x;
            uint32_t sharedCol = 0;

            for (int32_t col = 0; col < group->m_liquidVerts.x; col++) {
                const SMOLiquidVert* liquidVert = &group->m_liquidVertexList[group->m_liquidVerts.x * row + col];

                C3Vector vertex;
                vertex.x = x;
                vertex.y = y;
                vertex.z = liquidVert->m_waterVert.m_height;

                int32_t duplicateCol = 0;

                if (sharedCol < sharedVerts[0].m_count && sharedVerts[0].m_data[sharedCol] == col) {
                    duplicateCol = 1;
                    sharedCol++;
                }

                int32_t unused;

                for (int32_t c = duplicateCol + 1; c; c--) {
                    WriteLiquidVertex(group, matrix, vertex, liquidVert, color, magma, texCoord1U, &unused, stride, position, normal, vertexColor, texCoord0, texCoord1);
                }

                x += 4.1666665f;
            }
        }

        y += 4.1666665f;
    }

    return (group->m_liquidVerts.x + sharedVerts[0].m_count) * (sharedVerts[1].m_count + group->m_liquidVerts.y);
}

// VERIFY: 44a2f715 manual
// OFFSET: 0x007B0250
CMapObjOccluder* CMapObj::AllocOccluder() {
    return CMapObj::AllocOccluderNode(CMapObj::occluderHeap);
}

// VERIFY: 13be7ec2 manual
// OFFSET: 0x007B0260
CMapObj::~CMapObj() {
    this->Clear();
}

// VERIFY: 39e97d07 manual
// OFFSET: 0x007B0310
void CMapObj::FreeOccluderEntry(uint32_t* heap, CMapObjOccluder* occluder) {
    occluder->m_link.Unlink();
    ObjectFree(*heap, occluder->m_memHandle);
}

// VERIFY: cd8a34ce auto
// OFFSET: 0x007B0560
void CMapObj::FreeOccluder(CMapObjOccluder* occluder) {
    CMapObj::FreeOccluderEntry(CMapObj::occluderHeap, occluder);
}

// VERIFY: 8e1114de manual
// OFFSET: 0x007B0580
CMapObj::CMapObj() {
    this->m_mapObjGroupCount = 512;
    this->Init();
}

// VERIFY: 3da651a7 auto
// OFFSET: 0x007B0CC0
CMapObj* CMapObj::Create(char* fileName) {
    uint32_t hashval = SStrHash(fileName, 0, 0);
    CMapObj* mapObj = CMapObj::mapObjHash.Ptr(hashval, s_mapObjHashKey);

    if (mapObj) {
        mapObj->m_refCount++;
        return mapObj;
    }

    mapObj = CMap::AllocMapObj();

    if (!mapObj->Read(fileName)) {
        DebugLog("CMapObj::Create(): mapObj->Read(\"%s\") failed", fileName);
    }

    CMapObj::mapObjHash.Insert(mapObj, hashval, s_mapObjHashKey);
    mapObj->m_refCount = 1;

    return mapObj;
}

// VERIFY: 31d2020e auto
// OFFSET: 0x007B3A10
void CMapObjDef::CullMapObjDefGroupFromExterior(CMapObjDefGroup* mapObjDefGroup, CRect* rect, uint32_t skipOcclusion) {
    CMapObj::SetGroupRenderCallback(CWorldScene::AddMapObjDefGroupToSortTable, this);
    CWorldScene::s_curMapObjDef = this;
    CWorldScene::FrustumPush();
    CWorldScene::FrustumSet(CWorldScene::s_frustumCorners, *rect);

    uint32_t groupFlags = this->m_owner->GetGroupFlags(mapObjDefGroup->m_groupNum);

    if (!CWorldScene::FrustumCull(&mapObjDefGroup->m_bbox) && (skipOcclusion || !CWorldOcclusion::QueryBuffer(&mapObjDefGroup->m_bbox, 1))) {
        if (groupFlags & 0x10000) {
            this->m_owner->InvokeRenderCallback(mapObjDefGroup->m_groupNum);
            CWorldScene::FrustumPop();
            return;
        }

        if (groupFlags & 0x8) {
            CRect ndcRect;
            ndcRect.minY = rect->minY * 2.0f - 1.0f;
            ndcRect.minX = rect->minX * 2.0f - 1.0f;
            ndcRect.maxY = rect->maxY * 2.0f - 1.0f;
            ndcRect.maxX = 2.0f * rect->maxX - 1.0f;
            this->m_owner->RenderThruPortalsExterior(this->m_mat, this->m_invMat, CWorldScene::s_activeWorldView, CWorldScene::camTarget, ndcRect, mapObjDefGroup->m_groupNum);
        }
    }

    CWorldScene::FrustumPop();
}

// VERIFY: 7164266f manual
// OFFSET: 0x007BF740
void CMapObj::CreateRefs(CMapObj* mapObj, CMapObjGroup* mapObjGroup, CMapObjDef* mapObjDef, CMapObjDefGroup* mapObjDefGroup) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(mapObjGroup->m_doodadRefListCount); i++) {
        uint16_t doodadRef = mapObjGroup->m_doodadRefList[i];
        int32_t doodadSet = mapObj->GetDoodadSet(doodadRef);
        int32_t overrideSet = 0;

        if (doodadSet && doodadSet != mapObjDef->m_doodadSet) {
            uint32_t j = 0;

            while (doodadSet != mapObjDef->m_doodadSetOverrides[j]) {
                if (++j >= 3) {
                    break;
                }
            }

            if (j >= 3) {
                continue;
            }

            overrideSet = doodadSet;

            if (!overrideSet) {
                continue;
            }
        }

        SMODoodadDef* doodadDef = &mapObj->m_doodadDefList[doodadRef];
        CMapDoodadDef* mapDoodadDef = CMap::CreateDoodadDef(doodadRef, doodadDef, &mapObj->m_doodadNameList[doodadDef->m_flags & 0xFFFFFF], mapObjDef->m_hashval + 1, &mapObjDef->m_mat, static_cast<uint16_t>(overrideSet));
        CMapBaseObjLink* link = CMap::AllocBaseObjLink(mapDoodadDef);
        link->m_ref = mapObjDefGroup;

        if (overrideSet) {
            mapObjDefGroup->m_doodadDefLinkList.LinkToHead(link);
        } else {
            mapObjDefGroup->m_doodadDefLinkList.LinkToTail(link);
        }

        if (!(mapObjDefGroup->m_flags & 0x2) || (mapDoodadDef->m_flags & 0x4)) {
            mapDoodadDef->m_diffuseLightScale = 1.0f;
            mapDoodadDef->m_flags = static_cast<MapObjFlag>((mapDoodadDef->m_flags & ~0x2) | 0x4);
        } else {
            mapDoodadDef->m_flags = static_cast<MapObjFlag>(mapDoodadDef->m_flags | 0x2);
        }
    }

    mapObjDefGroup->m_flags = static_cast<MapObjFlag>(mapObjDefGroup->m_flags | 0x8);
}

// VERIFY: 3943554d auto
// OFFSET: 0x007D77C0
void CMapObj::DistFromClosestExtPortal(uint32_t depth, uint32_t flags, CMapObjGroup* group, CMapObjGroup* fromGroup, const C3Vector& pos, float* dist) {
    if (depth > 3) {
        return;
    }

    for (int32_t i = 0; i < group->m_portalCount; i++) {
        SMOPortalRef& ref = this->m_portalRefList[group->m_portalStart + i];
        CMapObjGroup* other = this->m_mapObjGroupArray[ref.m_groupIndex];

        if (!other || other == fromGroup) {
            continue;
        }

        if (flags & this->m_groupInfo[ref.m_groupIndex].m_flags) {
            SMOPortal& portal = this->m_portalList[ref.m_portalIndex];
            float portalDist = NTempest::DistanceFromPolygon(pos, &this->m_portalVertexList[portal.m_startVertex], portal.m_count, portal.m_plane);

            if (portalDist < 25.0f && portalDist < *dist) {
                *dist = portalDist;
            }
        } else {
            this->DistFromClosestExtPortal(depth + 1, flags, other, group, pos, dist);
        }
    }
}

// VERIFY: 231987cf manual
// OFFSET: 0x007D8010
float CMapObj::DistFromClosestExtPortal(CMapObjGroup* group, const C3Vector& pos, int32_t exteriorLit) {
    uint32_t flags = exteriorLit ? 0x48 : 0x8;
    float dist = 3.4028235e38f;
    this->DistFromClosestExtPortal(0, flags, group, nullptr, pos, &dist);

    return dist;
}

// VERIFY: 484c8c47 manual
// OFFSET: 0x007AEF00
bool CMapObj::GetTris(const CAaBox& box, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef) {
    bool result = false;
    uint16_t ignoreFlags = static_cast<uint16_t>(CMapObj::CreateWmoIgnoreFlags(flags));

    for (int32_t i = 0; i < this->m_groupInfoCount; i++) {
        const CAaBox& bounds = this->m_groupInfo[i].m_boundingBox;

        if (box.t.x < bounds.b.x || box.t.y < bounds.b.y || box.t.z < bounds.b.z || box.b.x > bounds.t.x || box.b.y > bounds.t.y || box.b.z > bounds.t.z) {
            continue;
        }

        if (!this->m_isGroupLoaded) {
            continue;
        }

        CMapObjGroup* group = this->m_mapObjGroupArray[i];

        if ((group->m_unkLoadedFlag & 1) && !(group->m_flags & 0x80)) {
            result |= group->GetTris(box, flags, ignoreFlags, triData, mapObjDef);
        }
    }

    return result;
}

// VERIFY: d1ce1d3d manual
// OFFSET: 0x007AF000
bool CMapObj::GetLiquidFacets(const CAaBox& box, uint32_t flags, World::FacetData* facets, CMapObjDef* mapObjDef) {
    bool result = false;

    for (int32_t i = 0; i < this->m_groupInfoCount; i++) {
        const CAaBox& bounds = this->m_groupInfo[i].m_boundingBox;

        if (box.t.x < bounds.b.x || box.t.y < bounds.b.y || box.t.z < bounds.b.z || box.b.x > bounds.t.x || box.b.y > bounds.t.y || box.b.z > bounds.t.z) {
            continue;
        }

        if (!this->m_isGroupLoaded) {
            continue;
        }

        CMapObjGroup* group = this->m_mapObjGroupArray[i];

        if ((group->m_unkLoadedFlag & 1) && !(group->m_flags & 0x400080)) {
            result |= group->GetLiquidFacets(box, flags, facets, mapObjDef);
        }
    }

    return result;
}

// VERIFY: d6cd6b2c auto
// OFFSET: 0x007AE920
bool CMapObj::TestGroupBounds(const C3Vector& point, uint32_t groupNum) {
    return this->m_isGroupLoaded && (this->m_mapObjGroupArray[groupNum]->m_unkLoadedFlag & 1) && this->m_groupInfo[groupNum].m_boundingBox.ContainsPoint(point);
}

// VERIFY: 15693e65 auto
// OFFSET: 0x007AF200
bool CMapObj::Intersect(C3Vector& start, C3Vector& end, float* distance, uint32_t flags, uint32_t ignoreFlags, uint32_t groupNum, int32_t* hitIndex) {
    if (!this->m_isGroupLoaded) {
        return false;
    }

    CMapObjGroup* group = this->m_mapObjGroupArray[groupNum];

    if (!(group->m_unkLoadedFlag & 1)) {
        return false;
    }

    C3Segment seg;
    seg.b = start;
    seg.t = end;
    return group->Intersect(seg, distance, flags, static_cast<uint16_t>(ignoreFlags), hitIndex);
}

// VERIFY: d090e248 manual
// OFFSET: 0x007AE8D0
bool CMapObj::TestGroupBounds(CAaBox& box, uint32_t groupNum, int32_t a3) {
    return (!a3 || (this->m_isGroupLoaded && (this->m_mapObjGroupArray[groupNum]->m_unkLoadedFlag & 1))) && this->m_groupInfo[groupNum].m_boundingBox.Intersects(box);
}

// VERIFY: aacb9f67 manual
// OFFSET: 0x007AEB40
bool CMapObj::QueryLighting(uint32_t groupNum, const C3Vector& point, uint16_t face, CImVector* color, uint8_t* flag) {
    if (!this->m_isGroupLoaded) {
        return false;
    }

    CMapObjGroup* group = this->m_mapObjGroupArray[groupNum];

    if (!(group->m_unkLoadedFlag & 1) || (group->m_flags & 0x48)) {
        return false;
    }

    return group->QueryLighting(point, face, color, flag);
}

// VERIFY: 155a20aa manual
// OFFSET: 0x007AF780
bool CMapObj::QueryLighting(uint32_t groupNum, const C3Segment& seg, CImVector* color, uint8_t* flag) {
    float dist = 1.0f;

    if (!this->m_isGroupLoaded) {
        return false;
    }

    CMapObjGroup* group = this->m_mapObjGroupArray[groupNum];

    if (!(group->m_unkLoadedFlag & 1) || (group->m_flags & 0x48)) {
        return false;
    }

    World::TriData triData;
    World::TriData::statusFlags = 0;
    World::TriData::nBatches = 0;
    World::TriData::faceIndexCursor = 0;
    World::TriData::indexCursor = 0;
    World::TriData::matrixPoolCount = 0;

    if (!group->GetTris(seg, &dist, 0, 8, &triData, nullptr)) {
        return false;
    }

    C3Vector point = C3Vector((seg.t.x - seg.b.x) * dist + seg.b.x, (seg.t.y - seg.b.y) * dist + seg.b.y, (seg.t.z - seg.b.z) * dist + seg.b.z);
    return group->QueryLighting(point, World::TriData::batches[0].m_faceIndices[0], color, flag);
}

// VERIFY: dd3ad3a2 manual
// OFFSET: 0x007AF0F0
bool CMapObj::GetTris(const CWFrustum& frustum, uint32_t flags, World::TriData* triData, CMapObjDef* mapObjDef) {
    bool result = false;
    uint16_t ignoreFlags = static_cast<uint16_t>(CMapObj::CreateWmoIgnoreFlags(flags));
    CAaBox box = CAaBox::Bounding(frustum.corners, 8);

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_groupInfoCount); i++) {
        const CAaBox& bounds = this->m_groupInfo[i].m_boundingBox;

        if (box.t.x < bounds.b.x || box.t.y < bounds.b.y || box.t.z < bounds.b.z || box.b.x > bounds.t.x || box.b.y > bounds.t.y || box.b.z > bounds.t.z) {
            continue;
        }

        if (!this->m_isGroupLoaded) {
            continue;
        }

        CMapObjGroup* group = this->m_mapObjGroupArray[i];

        if ((group->m_unkLoadedFlag & 1) && !(group->m_flags & 0x80)) {
            result |= group->GetTris(frustum, flags, ignoreFlags, triData, mapObjDef);
        }
    }

    return result;
}

// STATIC: 0x00D1C418
uint32_t CMapObj::sMinimapTag;

// STATIC: 0x00AEE00C
static float s_minimapTexelSize = 0.5f;

// STATIC: 0x00D1C47C
static float s_minimapQuadSize = s_minimapTexelSize * 256.0f;

// VERIFY: 75e1421c manual
// OFFSET: 0x007AE1F0
static uint32_t ComputeAaBoxOutcodeExact(const C3Vector* point, const CAaBox* box) {
    float delta[6];
    delta[0] = point->x - box->b.x;
    delta[1] = point->y - box->b.y;
    delta[2] = point->z - box->b.z;
    delta[3] = box->t.x - point->x;
    delta[4] = box->t.y - point->y;
    delta[5] = box->t.z - point->z;
    uint32_t bits[6];
    memcpy(bits, delta, sizeof(bits));
    return (bits[0] >> 31) | ((bits[1] >> 30) & 2) | ((bits[2] >> 29) & 4) | ((bits[3] >> 28) & 8) | ((bits[4] >> 27) & 0x10) | ((bits[5] >> 26) & 0x20);
}

// VERIFY: c28f57e3 manual
// OFFSET: 0x007AF8D0
static void GetMinimapQuads(const CAaBox* bounds, const CAaBox* box, int32_t groupId, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads) {
    float centerZ = (bounds->t.z + bounds->b.z) * 0.5f;
    float inv = 1.0f / s_minimapQuadSize;
    float sizeX = (bounds->t.x - bounds->b.x) * inv;
    float sizeY = inv * (bounds->t.y - bounds->b.y);
    float invLog2 = 1.0f / std::log10(2.0f);

    float exponentY = invLog2 * std::log10(sizeY * 256.0f);
    uint32_t texelsY;

    if ((1u << static_cast<int32_t>(std::ceil(exponentY))) < 256 && (1u << static_cast<int32_t>(std::ceil(exponentY))) < 32) {
        texelsY = 32;
    } else if ((1u << static_cast<int32_t>(std::ceil(exponentY))) < 256) {
        texelsY = 1u << static_cast<int32_t>(std::ceil(exponentY));
    } else {
        texelsY = 256;
    }

    float exponentX = std::log10(sizeX * 256.0f) * invLog2;
    uint32_t texelsX;

    if ((1u << static_cast<int32_t>(std::ceil(exponentX))) < 256 && (1u << static_cast<int32_t>(std::ceil(exponentX))) < 32) {
        texelsX = 32;
    } else if ((1u << static_cast<int32_t>(std::ceil(exponentX))) < 256) {
        texelsX = 1u << static_cast<int32_t>(std::ceil(exponentX));
    } else {
        texelsX = 256;
    }

    int32_t countY = static_cast<int32_t>(std::ceil(sizeY));
    int32_t countX = static_cast<int32_t>(std::ceil(sizeX));
    float stepX = texelsX * s_minimapTexelSize;
    float stepY = texelsY * s_minimapTexelSize;

    for (int32_t y = 0; y < countY; y++) {
        for (int32_t x = 0; x < countX; x++) {
            float minX = x * stepX + bounds->b.x;
            float minY = y * stepY + bounds->b.y;
            float maxX = minX + stepX;
            float maxY = minY + stepY;

            static float s_border = s_minimapTexelSize + s_minimapTexelSize;

            if (!x) {
                minX = minX - s_border;
            }

            if (!y) {
                minY = minY - s_border;
            }

            if (x == countX - 1) {
                maxX = maxX + s_border;
            }

            if (y == countY - 1) {
                maxY = maxY + s_border;
            }

            if (maxX >= box->b.x && maxY >= box->b.y && minX <= box->t.x && minY <= box->t.y && quads->m_count < maxQuads) {
                MINIMAPQUADINFO* quad = new (&quads->m_data[quads->m_count++]) MINIMAPQUADINFO();
                quad->m_groupId = groupId;
                quad->m_y = y;
                quad->m_bounds.b = C3Vector(minX, minY, centerZ);
                quad->m_bounds.t = C3Vector(maxX, maxY, centerZ);
                quad->m_x = x;
            }
        }
    }
}

// VERIFY: eae22eb6 auto
// OFFSET: 0x007AFE70
void CMapObj::GetMinimapQuads(CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads) {
    CAaBox bounds = this->m_bbox;
    ::GetMinimapQuads(&bounds, box, this->m_groupInfoCount, quads, maxQuads);
}

// VERIFY: 905ed32b manual
// OFFSET: 0x007AFC70
void CMapObj::QueryMapObjMinimapGroup(int32_t groupNum, int32_t fromGroup, CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint32_t flagsMask, uint32_t depth) {
    if (!this->m_isGroupLoaded) {
        return;
    }

    CMapObjGroup* group = this->m_mapObjGroupArray[groupNum];

    if (!(group->m_unkLoadedFlag & 1)) {
        return;
    }

    if ((inside ? (group->m_flags & 0x48) : (group->m_flags & 0x8)) != flagsMask || group->m_minimapTag == static_cast<int32_t>(CMapObj::sMinimapTag)) {
        return;
    }

    group->m_minimapTag = CMapObj::sMinimapTag;

    if (!group->m_bbox.IntersectsXY(*box)) {
        return;
    }

    ::GetMinimapQuads(&group->m_bbox, box, groupNum, quads, maxQuads);
    int32_t wmoGroupId = group->m_wmoGroupId;

    if (wmoGroupId == 23015 || wmoGroupId == 23016 || !group->m_portalCount || !g_minimapPortalMaxCVar || depth >= static_cast<uint32_t>(g_minimapPortalMaxCVar->m_intValue)) {
        return;
    }

    for (int32_t i = 0; i < group->m_portalCount; i++) {
        SMOPortalRef* portalRef = &this->m_portalRefList[group->m_portalStart + i];
        uint32_t target = portalRef->m_groupIndex;

        if (target == 0xFFFF || target == static_cast<uint32_t>(fromGroup) || (this->GetGroupFlags(target) & 0x80)) {
            continue;
        }

        SMOPortal* portal = &this->m_portalList[portalRef->m_portalIndex];

        if (!portal->m_count) {
            continue;
        }

        uint32_t outcode = 0xFFFFFFFF;
        C3Vector* vertex = &this->m_portalVertexList[portal->m_startVertex];

        for (uint32_t j = 0; j < portal->m_count; j++) {
            outcode &= ComputeAaBoxOutcodeExact(vertex, box);
            vertex++;
        }

        if (outcode) {
            continue;
        }

        CMapObjGroup* targetGroup;

        if (!this->m_isGroupLoaded || !((targetGroup = this->m_mapObjGroupArray[target])->m_unkLoadedFlag & 1) || (targetGroup->m_wmoGroupId != 23015 && targetGroup->m_wmoGroupId != 23016)) {
            this->QueryMapObjMinimapGroup(target, groupNum, box, quads, maxQuads, inside, flagsMask, ++depth);
        }
    }
}

// VERIFY: ddad8ead auto
// OFFSET: 0x007B00A0
bool CMapObj::QueryMapObjMinimap(int32_t groupNum, CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint8_t indoorArea) {
    CMapObj::sMinimapTag++;

    if (!this->m_isGroupLoaded) {
        return false;
    }

    CAaBox query = *box;
    CMapObjGroup* group = this->m_mapObjGroupArray[groupNum];

    if (!(group->m_unkLoadedFlag & 1)) {
        return false;
    }

    query.t.z = group->m_bbox.t.z;

    if (indoorArea) {
        this->GetMinimapQuads(&query, quads, maxQuads);
        return true;
    }

    uint32_t flagsMask = inside ? (this->m_groupInfo[groupNum].m_flags & 0x40) : 8;
    this->QueryMapObjMinimapGroup(groupNum, groupNum, &query, quads, maxQuads, inside, flagsMask, 0);
    return true;
}

// VERIFY: 6b25295a manual
// OFFSET: 0x007AF520
bool CMapObj::VectorIntersectPortals(uint32_t groupNum, C3Segment& seg, float* t, int32_t* outGroups) {
    if (!this->m_isGroupLoaded) {
        return false;
    }

    CMapObjGroup* group = this->m_mapObjGroupArray[groupNum];

    if (!(group->m_unkLoadedFlag & 1)) {
        return false;
    }

    C3Ray ray;
    ray.origin = seg.b;
    float dx = seg.t.x - seg.b.x;
    float dy = seg.t.y - seg.b.y;
    float dz = seg.t.z - seg.b.z;
    float length = sqrtf(dz * dz + dx * dx + dy * dy);
    float inv = 1.0f / length;
    ray.dir = C3Vector(dx * inv, dy * inv, inv * dz);

    float maxT = length * *t;
    bool found = false;
    SMOPortalRef* ref = &this->m_portalRefList[group->m_portalStart];

    for (int32_t i = 0; i < group->m_portalCount; i++, ref++) {
        SMOPortal& portal = this->m_portalList[ref->m_portalIndex];

        if (!this->m_isGroupLoaded || !(this->m_mapObjGroupArray[ref->m_groupIndex]->m_unkLoadedFlag & 1)) {
            continue;
        }

        C3Vector point(0.0f, 0.0f, 0.0f);
        float portalT;

        if (!NTempest::Intersect(ray, portal.m_plane, &portalT, &point, 0.1f) || portalT < 0.0f || portalT > maxT) {
            continue;
        }

        C3Vector::EAxis axis = portal.m_plane.n.MajorAxis();

        if (!NTempest::Intersect(point, &this->m_portalVertexList[portal.m_startVertex], portal.m_count, axis)) {
            continue;
        }

        found = true;
        maxT = portalT;

        float side = portal.m_plane.n.z * seg.b.z + portal.m_plane.n.y * seg.b.y + seg.b.x * portal.m_plane.n.x + portal.m_plane.d;

        if ((side >= 0.0f && ref->m_side > 0) || (side < 0.0f && ref->m_side <= 0)) {
            outGroups[0] = groupNum;
            outGroups[1] = ref->m_groupIndex;
        } else {
            outGroups[0] = ref->m_groupIndex;
            outGroups[1] = groupNum;
        }
    }

    if (found) {
        *t = maxT * inv;
    }

    return found;
}

// VERIFY: e67c38fa manual
// OFFSET: 0x007AB760
void CMapObj::RenderMapObjGroupsShadowMap(void* groups, uint32_t count, void* groupData, const C44Matrix& translation, CFrustum& frustum) {
    CAaBox box;
    box.b.x = 0.0f;
    box.b.y = 0.0f;
    box.b.z = 0.0f;
    box.t.x = 0.0f;
    box.t.y = 0.0f;
    box.t.z = 0.0f;
    g_theGxDevicePtr->RsPush();
    float constants[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    CMapObjGroup** groupList = static_cast<CMapObjGroup**>(groups);
    C44Matrix** matrices = static_cast<C44Matrix**>(groupData);

    for (uint32_t i = 0; i < count; i++) {
        C44Matrix* matrix = matrices[i];
        CMapObjGroup* group = groupList[i];
        CMapObj* mapObj = group->m_parent;
        C44Matrix world = *matrix * translation;
        CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_World];
        stack.m_dirty = 1;
        stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
        stack.m_mtx[stack.m_level] = world;
        CWorldMath::TransformAABox(*matrix, group->m_bbox, box);

        if (!group->m_batchListCount || !frustum.Cull(box)) {
            continue;
        }

        group->m_timer = 0.0f;
        group->AllocVB();
        group->SetIndexVB();
        group->SetVertexVB();
        CShaderEffect::UpdateWorldViewMatrix();

        if (group->m_unkLoadedFlag & 0x4) {
            CShaderEffect::SetAlphaRef(0.0f);
            CShaderEffect::SetDefaultShaders(0);
            GxRsSet(GxRs_Texture0, 0);
            constants[3] = 0.0f;
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 2, constants, 1);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = group->m_indexMin;
            batch.m_count = group->m_indexMax - group->m_indexMin + 1;
            batch.m_minIndex = group->m_vertexMin;
            batch.m_maxIndex = group->m_vertexMax;
            g_theGxDevicePtr->Draw(&batch, 1);
            continue;
        }

        CShaderEffect::SetAlphaRef(0.87843138f);
        CShaderEffect::SetDefaultShaders(0);

        for (int32_t j = 0; j < group->m_batchListCount; j++) {
            SMOBatch& smoBatch = group->m_batchList[j];
            SMOMaterial& material = mapObj->m_materialList[smoBatch.m_texture];
            CGxTex* gxTex = TextureGetGxTex(material.Textures()[0], 0, nullptr);

            if (!gxTex) {
                continue;
            }

            g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
            constants[3] = material.m_blendMode == 1 ? 0.87843138f : 0.0f;
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 2, constants, 1);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = smoBatch.m_indexStart;
            batch.m_count = smoBatch.m_indexCount;
            batch.m_minIndex = smoBatch.m_vertexStart;
            batch.m_maxIndex = smoBatch.m_vertexEnd;
            g_theGxDevicePtr->Draw(&batch, 1);
        }
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 2d3fae74 auto
// OFFSET: 0x007AE050
void CMapObj::DestroyPools() {
    CMapObj::bPoolsDirty = 1;
}

// VERIFY: 45ac5b73 manual
// OFFSET: 0x007ABAC0
void CMapObj::RenderMapObjGroupsColor(CMapObjGroup** groups, uint32_t count, C44Matrix** matrices, const C44Matrix* world, int32_t* a5) {
    uint32_t color = static_cast<uint32_t>(*a5);

    if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
        color = (color & 0xFF00FF00) | ((color >> 16) & 0xFF) | ((color & 0xFF) << 16);
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 0);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_VertexShader, static_cast<void*>(nullptr));
    GxRsSet(GxRs_PixelShader, static_cast<void*>(nullptr));

    for (uint32_t i = 0; i < count; i++) {
        CMapObjGroup* group = groups[i];
        CMapObj* mapObj = group->m_parent;
        C44Matrix transform = *matrices[i] * *world;
        CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_World];
        stack.m_dirty = 1;
        stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
        stack.m_mtx[stack.m_level] = transform;

        if (!group->m_batchListCount) {
            continue;
        }

        CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPNC), group->m_vertexListCount);
        CGxVertexPNC* vertex = reinterpret_cast<CGxVertexPNC*>(g_theGxDevicePtr->BufLock(vertexBuf));

        for (int32_t j = 0; j < group->m_vertexListCount; j++) {
            vertex->m_position = group->m_vertexList[j];
            vertex->m_normal = group->m_normalList[j];
            *reinterpret_cast<uint32_t*>(&vertex->m_color) = color;
            vertex++;
        }

        g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
        vertexBuf->m_hasData = 1;
        GxPrimVertexPtr(vertexBuf, GxVBF_PNC);
        CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), group->m_indicesCount);
        memcpy(g_theGxDevicePtr->BufLock(indexBuf), group->m_indices, sizeof(uint16_t) * group->m_indicesCount);
        g_theGxDevicePtr->BufUnlock(indexBuf, 0);
        indexBuf->m_hasData = 1;
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);

        if (group->m_unkLoadedFlag & 0x4) {
            GxRsSet(GxRs_Texture0, static_cast<void*>(nullptr));
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = group->m_indexMin;
            batch.m_count = group->m_indexMax - group->m_indexMin + 1;
            batch.m_minIndex = group->m_vertexMin;
            batch.m_maxIndex = group->m_vertexMax;
            g_theGxDevicePtr->Draw(&batch, 1);
            continue;
        }

        for (int32_t j = 0; j < group->m_batchListCount; j++) {
            SMOBatch& smoBatch = group->m_batchList[j];
            CGxTex* gxTex = TextureGetGxTex(mapObj->m_materialList[smoBatch.m_texture].Textures()[0], 0, nullptr);

            if (!gxTex) {
                continue;
            }

            g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = smoBatch.m_indexStart;
            batch.m_count = smoBatch.m_indexCount;
            batch.m_minIndex = smoBatch.m_vertexStart;
            batch.m_maxIndex = smoBatch.m_vertexEnd;
            g_theGxDevicePtr->Draw(&batch, 1);
        }
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 44371f63 auto
// OFFSET: 0x007AECB0
bool CMapObj::VectorIntersect(C3Vector& start, C3Vector& end, uint32_t flags, uint32_t groupIgnoreFlags, float* t, uint16_t* hitFace, CMapObjDef* mapObjDef, float* normalDot, uint32_t* hitGroup) {
    if (!this->m_isGroupLoaded) {
        return false;
    }

    C3Vector normal(0.0f, 0.0f, 0.0f);
    C3Segment seg;
    seg.b = start;
    seg.t = end;
    uint16_t face = 0;
    bool hit = false;
    uint16_t ignoreFlags = static_cast<uint16_t>(CMapObj::CreateWmoIgnoreFlags(flags));

    for (int32_t i = 0; i < this->m_groupInfoCount; i++) {
        if (!this->m_isGroupLoaded) {
            continue;
        }

        CMapObjGroup* group = this->m_mapObjGroupArray[i];

        if (!(group->m_unkLoadedFlag & 1) || (groupIgnoreFlags & group->m_flags)) {
            continue;
        }

        if (!CWorldMath::VectorIntersectAABox2(this->m_groupInfo[i].m_boundingBox, start, end)) {
            continue;
        }

        World::TriData::statusFlags = 0;
        World::TriData::nBatches = 0;
        World::TriData::faceIndexCursor = 0;
        World::TriData::indexCursor = 0;
        World::TriData::matrixPoolCount = 0;
        World::TriData triData;

        if (!group->GetTris(seg, t, flags, ignoreFlags, &triData, mapObjDef)) {
            continue;
        }

        if (World::TriData::batches[0].m_faceIndices) {
            face = *World::TriData::batches[0].m_faceIndices;

            if (normalDot) {
                C3Vector* vertices = World::TriData::batches[0].m_vertexList;
                uint16_t* indices = World::TriData::batches[0].m_indices;
                C3Vector& v0 = vertices[indices[0]];
                C3Vector& v1 = vertices[indices[1]];
                C3Vector& v2 = vertices[indices[2]];
                normal = C3Vector::Cross(v1 - v0, v2 - v0);
                *normalDot = (seg.t.z - seg.b.z) * normal.z + (seg.t.y - seg.b.y) * normal.y + (seg.t.x - seg.b.x) * normal.x;
            }
        }

        if (hitGroup) {
            *hitGroup = i;
        }

        hit = true;
    }

    if (hit && hitFace) {
        *hitFace = face;
    }

    if (!normalDot) {
        return hit;
    }

    float length = sqrtf(normal.z * normal.z + normal.y * normal.y + normal.x * normal.x);

    if (fabs(length) < 0.00000023841858f) {
        return hit;
    }

    *normalDot = *normalDot / length;
    return hit;
}

// STATIC: 0x00CFBED8
static uint16_t s_portalIndices[65536];

// VERIFY: 6960856a manual
// OFFSET: 0x007A76C0
void __stdcall CMapObj::RenderGroupCollidableFaces(CMapObjGroup* mapObjGroup) {
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 24, mapObjGroup->m_vertexListCount);
    float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(vertexBuf));

    for (uint32_t i = 0; i < static_cast<uint32_t>(mapObjGroup->m_vertexListCount); i++) {
        vertices[0] = mapObjGroup->m_vertexList[i].x;
        vertices[1] = mapObjGroup->m_vertexList[i].y;
        vertices[2] = mapObjGroup->m_vertexList[i].z;
        vertices[3] = mapObjGroup->m_normalList[i].x;
        vertices[4] = mapObjGroup->m_normalList[i].y;
        vertices[5] = mapObjGroup->m_normalList[i].z;
        vertices += 6;
    }

    GxBufUnlock(vertexBuf, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PN);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    SMOPoly* poly = mapObjGroup->m_polyList;
    uint32_t count = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(mapObjGroup->m_polyListSize); i++, poly++) {
        uint8_t flags = poly->m_flags;

        if (!(flags & 0x8) && (!(flags & 0x20) || (flags & 0x4))) {
            continue;
        }

        indices[0] = mapObjGroup->m_indices[i * 3];
        indices[1] = mapObjGroup->m_indices[i * 3 + 1];
        indices[2] = mapObjGroup->m_indices[i * 3 + 2];
        count += 3;
        indices += 3;

        if (count == 3000) {
            count = 0;
            GxBufUnlock(indexBuf, 0);
            g_theGxDevicePtr->PrimIndexPtr(indexBuf);
            CGxBatch batch;
            batch.m_maxIndex = mapObjGroup->m_vertexListCount;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = 3000;
            batch.m_minIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 1);
            indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
            indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
        }
    }

    GxBufUnlock(indexBuf, 0);

    if (count) {
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);
        CGxBatch batch;
        batch.m_start = 0;
        batch.m_minIndex = 0;
        batch.m_maxIndex = mapObjGroup->m_vertexListCount;
        batch.m_primType = GxPrim_Triangles;
        batch.m_count = count;
        g_theGxDevicePtr->Draw(&batch, 1);
    }
}

// VERIFY: 6ec9487c manual
// OFFSET: 0x007A9BF0
void CMapObj::RenderGroupLightmapTex(CMapObjGroup* group, uint32_t index) {
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxPrimVertexPtr(group->m_vertexListCount, group->m_vertexList, 12, nullptr, 0, group->m_colorVertexList, 4, nullptr, 0, nullptr, 0);

    if (group->m_stripIndices && CMap::s_enableTriangleStrips) {
        GxPrimIndexPtr(group->m_stripIndexCount, group->m_stripIndices);
    } else {
        GxPrimIndexPtr(group->m_indicesCount, group->m_indices);
    }

    SMOBatch* batch = group->m_batchList;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_batchListCount); i++, batch++) {
        if (!index) {
            batch->m_flags &= 0xF;
        }

        if ((batch->m_flags & 0xF0) || CMapObj::CullBatch(batch)) {
            continue;
        }

        batch->m_flags |= 0xF0;
        CGxBatch draw;
        draw.m_primType = GxPrim_Triangles;
        draw.m_start = batch->m_indexStart;
        draw.m_count = batch->m_indexCount;
        draw.m_minIndex = batch->m_vertexStart;
        draw.m_maxIndex = batch->m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: addd8d14 manual
// OFFSET: 0x007A9D30
void CMapObj::RenderGroupColorTex(CMapObjGroup* group, uint32_t index) {
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxPrimVertexPtr(group->m_vertexListCount, group->m_vertexList, 12, nullptr, 0, nullptr, 0, group->m_textureVertexList, 8, nullptr, 0);

    if (group->m_stripIndices && CMap::s_enableTriangleStrips) {
        GxPrimIndexPtr(group->m_stripIndexCount, group->m_stripIndices);
    } else {
        GxPrimIndexPtr(group->m_indicesCount, group->m_indices);
    }

    SMOBatch* batch = group->m_batchList;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_batchListCount); i++, batch++) {
        if (!index) {
            batch->m_flags &= 0xF;
        }

        if ((batch->m_flags & 0xF0) || CMapObj::CullBatch(batch)) {
            continue;
        }

        batch->m_flags |= 0xF0;
        SMOMaterial& material = this->m_materialList[batch->m_texture];
        CGxTex* gxTex = TextureGetGxTex(material.Textures()[0], 1, nullptr);
        GxTexSetWrap(gxTex, static_cast<EGxTexWrapMode>((material.m_flags & 0x40) == 0), static_cast<EGxTexWrapMode>((material.m_flags & 0x80) == 0));
        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
        CGxBatch draw;
        draw.m_primType = GxPrim_Triangles;
        draw.m_start = batch->m_indexStart;
        draw.m_count = batch->m_indexCount;
        draw.m_minIndex = batch->m_vertexStart;
        draw.m_maxIndex = batch->m_vertexEnd;
        g_theGxDevicePtr->Draw(&draw, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: a666e7f7 manual
// OFFSET: 0x007A9ED0
void CMapObj::RenderPortals(CMapObjGroup* group) {
    if (!group->m_portalCount) {
        return;
    }

    CImVector* colors = static_cast<CImVector*>(alloca(sizeof(CImVector) * this->m_planeVertCount));

    for (int32_t i = 0; i < this->m_planeVertCount; i++) {
        new (&colors[i]) CImVector();
    }

    uint32_t count = 0;
    SMOPortalRef* ref = &this->m_portalRefList[group->m_portalStart];

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_portalCount); i++, ref++) {
        SMOPortal* portal = &this->m_portalList[ref->m_portalIndex];

        if (portal->m_count == 2) {
            continue;
        }

        for (uint32_t j = 0; j < static_cast<uint32_t>(portal->m_count - 2); j++) {
            s_portalIndices[count] = j + portal->m_startVertex;
            s_portalIndices[count + 1] = j + portal->m_startVertex + 1;
            s_portalIndices[count + 2] = portal->m_count + portal->m_startVertex - 1;
            *reinterpret_cast<uint32_t*>(&colors[s_portalIndices[count]]) = 0x60FF0000;
            *reinterpret_cast<uint32_t*>(&colors[s_portalIndices[count + 1]]) = 0x60FF0000;
            *reinterpret_cast<uint32_t*>(&colors[s_portalIndices[count + 2]]) = 0x60FF0000;
            count += 3;
        }
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>((group->m_flags & 0x8) ? 0xFFFFFFFF : 0x60FF0000));
    GxRsSet(GxRs_BlendingMode, 3);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    GxPrimVertexPtr(this->m_planeVertCount, this->m_portalVertexList, 12, nullptr, 0, colors, 4, nullptr, 0, nullptr, 0);
    GxPrimIndexPtr(count, s_portalIndices);
    CGxBatch batch;
    batch.m_maxIndex = this->m_planeVertCount;
    batch.m_start = 0;
    batch.m_minIndex = 0;
    batch.m_primType = GxPrim_Triangles;
    batch.m_count = count;
    g_theGxDevicePtr->Draw(&batch, 1);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 2c5531e1 manual
// OFFSET: 0x007AA2E0
void CMapObj::RenderGroupCollisionFaces(CMapObjGroup* group, uint32_t index) {
    if (index) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 0);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0x80FF0000));
    GxRsSet(GxRs_Fog, 0);
    CMapObj::SetRenderModeLight();
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 24, group->m_vertexListCount);
    float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(vertexBuf));

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_vertexListCount); i++) {
        vertices[0] = group->m_vertexList[i].x;
        vertices[1] = group->m_vertexList[i].y;
        vertices[2] = group->m_vertexList[i].z;
        vertices[3] = group->m_normalList[i].x;
        vertices[4] = group->m_normalList[i].y;
        vertices[5] = group->m_normalList[i].z;
        vertices += 6;
    }

    GxBufUnlock(vertexBuf, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PN);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    SMOPoly* poly = group->m_polyList;
    uint32_t count = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_polyListSize); i++, poly++) {
        if (!(poly->m_flags & 0x8)) {
            continue;
        }

        indices[0] = group->m_indices[i * 3];
        indices[1] = group->m_indices[i * 3 + 1];
        indices[2] = group->m_indices[i * 3 + 2];
        count += 3;
        indices += 3;

        if (count == 3000) {
            count = 0;
            GxBufUnlock(indexBuf, 0);
            g_theGxDevicePtr->PrimIndexPtr(indexBuf);
            CGxBatch batch;
            batch.m_maxIndex = group->m_vertexListCount;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = 3000;
            batch.m_minIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 1);
            indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
            indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
        }
    }

    GxBufUnlock(indexBuf, 0);

    if (count) {
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);
        CGxBatch batch;
        batch.m_start = 0;
        batch.m_minIndex = 0;
        batch.m_maxIndex = group->m_vertexListCount;
        batch.m_primType = GxPrim_Triangles;
        batch.m_count = count;
        g_theGxDevicePtr->Draw(&batch, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 481cfef2 manual
// OFFSET: 0x007AA690
void CMapObj::RenderGroupDetailFaces(CMapObjGroup* group, uint32_t index) {
    if (index) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 0);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0xFF8080FF));
    GxRsSet(GxRs_Fog, 0);
    CMapObj::SetRenderModeLight();
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 24, group->m_vertexListCount);
    float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(vertexBuf));

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_vertexListCount); i++) {
        vertices[0] = group->m_vertexList[i].x;
        vertices[1] = group->m_vertexList[i].y;
        vertices[2] = group->m_vertexList[i].z;
        vertices[3] = group->m_normalList[i].x;
        vertices[4] = group->m_normalList[i].y;
        vertices[5] = group->m_normalList[i].z;
        vertices += 6;
    }

    GxBufUnlock(vertexBuf, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PN);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    SMOPoly* poly = group->m_polyList;
    uint32_t count = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_polyListSize); i++, poly++) {
        if (!(poly->m_flags & 0x4)) {
            continue;
        }

        indices[0] = group->m_indices[i * 3];
        indices[1] = group->m_indices[i * 3 + 1];
        indices[2] = group->m_indices[i * 3 + 2];
        count += 3;
        indices += 3;

        if (count == 3000) {
            count = 0;
            GxBufUnlock(indexBuf, 0);
            g_theGxDevicePtr->PrimIndexPtr(indexBuf);
            CGxBatch batch;
            batch.m_maxIndex = group->m_vertexListCount;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = 3000;
            batch.m_minIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 1);
            indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
            indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
        }
    }

    GxBufUnlock(indexBuf, 0);

    if (count) {
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);
        CGxBatch batch;
        batch.m_start = 0;
        batch.m_minIndex = 0;
        batch.m_maxIndex = group->m_vertexListCount;
        batch.m_primType = GxPrim_Triangles;
        batch.m_count = count;
        g_theGxDevicePtr->Draw(&batch, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 1463fac3 manual
// OFFSET: 0x007AAA40
void CMapObj::RenderGroupRenderFaces(CMapObjGroup* group, uint32_t index) {
    if (index) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 0);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0xFF00FF00));
    GxRsSet(GxRs_Fog, 0);
    CMapObj::SetRenderModeLight();
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 24, group->m_vertexListCount);
    float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(vertexBuf));

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_vertexListCount); i++) {
        vertices[0] = group->m_vertexList[i].x;
        vertices[1] = group->m_vertexList[i].y;
        vertices[2] = group->m_vertexList[i].z;
        vertices[3] = group->m_normalList[i].x;
        vertices[4] = group->m_normalList[i].y;
        vertices[5] = group->m_normalList[i].z;
        vertices += 6;
    }

    GxBufUnlock(vertexBuf, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PN);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    SMOPoly* poly = group->m_polyList;
    uint32_t count = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_polyListSize); i++, poly++) {
        if (!(poly->m_flags & 0x20) || (poly->m_flags & 0x4)) {
            continue;
        }

        indices[0] = group->m_indices[i * 3];
        indices[1] = group->m_indices[i * 3 + 1];
        indices[2] = group->m_indices[i * 3 + 2];
        count += 3;
        indices += 3;

        if (count == 3000) {
            count = 0;
            GxBufUnlock(indexBuf, 0);
            g_theGxDevicePtr->PrimIndexPtr(indexBuf);
            CGxBatch batch;
            batch.m_maxIndex = group->m_vertexListCount;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = 3000;
            batch.m_minIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 1);
            indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
            indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
        }
    }

    GxBufUnlock(indexBuf, 0);

    if (count) {
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);
        CGxBatch batch;
        batch.m_start = 0;
        batch.m_minIndex = 0;
        batch.m_maxIndex = group->m_vertexListCount;
        batch.m_primType = GxPrim_Triangles;
        batch.m_count = count;
        g_theGxDevicePtr->Draw(&batch, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: ac639cbe manual
// OFFSET: 0x007AAE00
void CMapObj::RenderGroupTransFaces(CMapObjGroup* group, uint32_t index) {
    if (index || !group->m_transparencyBatchesCount) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 0);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0xFF7F7F7F));
    GxRsSet(GxRs_Fog, 0);
    CMapObj::SetRenderModeLight();
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 28, group->m_vertexListCount);
    float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(vertexBuf));
    CImVector color;
    color.a = 0xFF;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_vertexListCount); i++) {
        vertices[0] = group->m_vertexList[i].x;
        vertices[1] = group->m_vertexList[i].y;
        vertices[2] = group->m_vertexList[i].z;
        vertices[3] = group->m_normalList[i].x;
        vertices[4] = group->m_normalList[i].y;
        vertices[5] = group->m_normalList[i].z;
        color.b = group->m_colorVertexList[i].a;
        color.g = color.b;
        color.r = color.b;
        *reinterpret_cast<CImVector*>(&vertices[6]) = color;
        vertices += 7;
    }

    GxBufUnlock(vertexBuf, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PNC);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    SMOPoly* poly = group->m_polyList;
    uint32_t count = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(group->m_polyListSize); i++, poly++) {
        if (!(poly->m_flags & 0x1) || !(poly->m_flags & 0x24)) {
            continue;
        }

        indices[0] = group->m_indices[i * 3];
        indices[1] = group->m_indices[i * 3 + 1];
        indices[2] = group->m_indices[i * 3 + 2];
        count += 3;
        indices += 3;

        if (count == 3000) {
            count = 0;
            GxBufUnlock(indexBuf, 0);
            g_theGxDevicePtr->PrimIndexPtr(indexBuf);
            CGxBatch batch;
            batch.m_maxIndex = group->m_vertexListCount;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = 3000;
            batch.m_minIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 1);
            indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 3000);
            indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
        }
    }

    GxBufUnlock(indexBuf, 0);

    if (count) {
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);
        CGxBatch batch;
        batch.m_start = 0;
        batch.m_minIndex = 0;
        batch.m_maxIndex = group->m_vertexListCount;
        batch.m_primType = GxPrim_Triangles;
        batch.m_count = count;
        g_theGxDevicePtr->Draw(&batch, 1);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 46cfab1c manual
// OFFSET: 0x007AB1E0
void CMapObj::RenderGroupCollidable(CMapObjGroup* group, uint32_t index) {
    if (index) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    int32_t polygonFill = g_theGxDevicePtr->MasterEnable(GxMasterEnable_PolygonFill);
    GxRsSet(GxRs_Fog, 0);
    CMapObj::SetRenderModeLight();
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    g_theGxDevicePtr->MasterEnableSet(GxMasterEnable_PolygonFill, 1);
    GxRsSet(GxRs_BlendingMode, 0);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0x80CCCCCC));
    GxRsSet(GxRs_DepthWrite, 1);
    CMapObj::RenderGroupCollidableFaces(group);
    g_theGxDevicePtr->MasterEnableSet(GxMasterEnable_PolygonFill, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0x80111111));
    GxRsSet(GxRs_DepthWrite, 0);
    CMapObj::RenderGroupCollidableFaces(group);
    g_theGxDevicePtr->MasterEnableSet(GxMasterEnable_PolygonFill, polygonFill);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: dd167be3 manual
// OFFSET: 0x007AB4C0
void __stdcall CMapObj::RenderNormals(CMapObjGroup* group) {
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Fog, 0);
    CMapObj::SetRenderModeLight();
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_PixelShader, 0);
    uint32_t remaining = group->m_vertexListCount;
    uint32_t chunk = remaining >= 0x4000 ? 0x4000 : remaining;
    uint32_t maxChunk = chunk;
    uint32_t start = 0;

    while (remaining) {
        if (remaining < chunk) {
            chunk = remaining;
        }

        uint32_t left = remaining - chunk;
        CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 16, 2 * chunk);
        CGxVertexPC* vertices = reinterpret_cast<CGxVertexPC*>(g_theGxDevicePtr->BufLock(vertexBuf));
        CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, 2, 2 * chunk);
        uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));

        CImVector color;
        *reinterpret_cast<uint32_t*>(&color) = 0xFFFFFF00;

        for (uint32_t i = 0; i < chunk; i++) {
            const C3Vector& vertex = group->m_vertexList[start + i];
            const C3Vector& normal = group->m_normalList[start + i];
            vertices[0].p = vertex;
            vertices[0].c = color;
            vertices[1].p = C3Vector(normal.x * 0.75f + vertex.x, normal.y * 0.75f + vertex.y, normal.z * 0.75f + vertex.z);
            vertices[1].c = color;
            indices[0] = 2 * i;
            indices[1] = 2 * i + 1;
            vertices += 2;
            indices += 2;
        }

        GxBufUnlock(vertexBuf, 0);
        GxPrimVertexPtr(vertexBuf, GxVBF_PC);
        GxBufUnlock(indexBuf, 0);
        g_theGxDevicePtr->PrimIndexPtr(indexBuf);
        CGxBatch batch;
        batch.m_count = 2 * chunk;
        batch.m_maxIndex = 2 * chunk;
        batch.m_primType = GxPrim_Lines;
        batch.m_start = 0;
        batch.m_minIndex = 0;
        g_theGxDevicePtr->Draw(&batch, 1);
        start += chunk;

        if (!left) {
            break;
        }

        remaining = left;
        chunk = maxChunk;
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 107cc03e manual
// OFFSET: 0x007AEA10
bool CMapObj::TestConvexVolume(const C3Vector& point) {
    if (!this->m_isGroupLoaded) {
        return false;
    }

    int32_t count = this->m_convexVolumePlaneCount;

    if (!count) {
        return true;
    }

    C4Plane* plane = this->m_convexVolumePlanes;

    for (int32_t i = 0; i < count; i++, plane++) {
        if (plane->n.y * point.y + plane->n.z * point.z + plane->n.x * point.x + plane->d > 0.0f) {
            return false;
        }
    }

    return true;
}

// VERIFY: c777c00f manual
// OFFSET: 0x007AF740
int32_t CMapObj::IsAllLoaded() {
    if (!this->m_isGroupLoaded) {
        return 0;
    }

    for (int32_t i = 0; i < this->m_groupInfoCount; i++) {
        if (!(this->m_mapObjGroupArray[i]->m_unkLoadedFlag & 0x1)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 2b62f89e auto
// OFFSET: 0x007AF850
int32_t CMapObj::IsAllDrawable() {
    int32_t loaded = this->IsAllLoaded();

    if (!loaded) {
        return loaded;
    }

    for (int32_t i = 0; i < this->m_materialsCount; i++) {
        CTexture* texture0 = this->m_materialList[i].Textures()[0];

        if (texture0 && !TextureGetGxTex(texture0, 0, nullptr)) {
            return 0;
        }

        CTexture* texture1 = this->m_materialList[i].Textures()[1];

        if (texture1 && !TextureGetGxTex(texture1, 0, nullptr)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 55e33c2a auto
// OFFSET: 0x007D9780
static int32_t CompareGroupDistance(const void* a, const void* b) {
    float distA = (*static_cast<CMapObjGroup* const*>(a))->m_distToCamera;
    float distB = (*static_cast<CMapObjGroup* const*>(b))->m_distToCamera;

    if (distB < distA) {
        return -1;
    }

    return distB > distA;
}

// VERIFY: 18154d58 auto
// OFFSET: 0x007D97C0
static int32_t CompareMapObjDistance(const void* a, const void* b) {
    float distA = (*static_cast<CMapObj* const*>(a))->m_distToCamera;
    float distB = (*static_cast<CMapObj* const*>(b))->m_distToCamera;

    if (distB < distA) {
        return -1;
    }

    return distB > distA;
}

// VERIFY: 43651b8e manual
// OFFSET: 0x007D9810
void CMapObj::ProcessAsyncLoadQueue() {
    void** pending = static_cast<void**>(alloca(0x1000));
    uint32_t count = 0;

    for (CMapObjGroup* group = CMapObj::s_loadedGroups.Head(); group; group = CMapObj::s_loadedGroups.Next(group)) {
        if (group->m_asyncObjPtr) {
            pending[count++] = group;
        }
    }

    if (count) {
        qsort(pending, count, sizeof(void*), CompareGroupDistance);
        AsyncFile::EnterQueueLock();

        for (uint32_t i = 0; i < count; i++) {
            CMapObjGroup* group = static_cast<CMapObjGroup*>(pending[i]);

            if (group->m_distToCamera < 40000.0f) {
                CAsyncObject* object = group->m_asyncObjPtr;

                if (object->m_isCurrent || object->m_isRead || object->m_isProcessed) {
                    continue;
                }

                AsyncFileReadLinkObject(object, 1);
            }

            group->m_distToCamera = 3.4028235e38f;
        }

        AsyncFile::LeaveQueueLock();
    }

    count = 0;

    for (CMapObj* mapObj = CMapObj::s_asyncLoadQueue.Head(); mapObj; mapObj = CMapObj::s_asyncLoadQueue.Next(mapObj)) {
        if (mapObj->m_asyncObject) {
            pending[count++] = mapObj;
        }
    }

    if (count) {
        qsort(pending, count, sizeof(void*), CompareMapObjDistance);
        AsyncFile::EnterQueueLock();

        for (uint32_t i = 0; i < count; i++) {
            CMapObj* mapObj = static_cast<CMapObj*>(pending[i]);

            if (mapObj->m_distToCamera < 40000.0f) {
                CAsyncObject* object = mapObj->m_asyncObject;

                if (object->m_isCurrent || object->m_isRead || object->m_isProcessed) {
                    continue;
                }

                AsyncFileReadLinkObject(object, 1);
            }

            mapObj->m_distToCamera = 3.4028235e38f;
        }

        AsyncFile::LeaveQueueLock();
    }
}
