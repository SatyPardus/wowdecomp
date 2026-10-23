#include "ShadowQuery.h"
#include "WoW/Source/WorldClient/WorldConstants.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cmath>
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/WorldClient/CWorldOcclusion.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Tempest/CFrustum.h"
#include "WoW/Source/WorldClient/MapEntity.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"

// STATIC: 0x00D25300
uint8_t CShadowQuery::s_stamp;

// STATIC: 0x00D2538C
static uint32_t s_maxTexturedBatches;

// STATIC: 0x00D25390
static uint32_t s_maxUntexturedBatches;

// STATIC: 0x00D25304
float CShadowQuery::s_cutoffHeight;

// STATIC: 0x00D25320
ShadowQueryBuffers CShadowQuery::s_bandBuffers[3];

// VERIFY: a0192ad6 auto
// OFFSET: 0x007BD3A0
void CShadowQuery::Initialize() {
    CShadowCache::SetMatrixCallback(CShadowQuery::MatrixCallback);
    CShadowCache::SetFrustumCallback(CShadowQuery::FrustumCallback);
    CShadowCache::SetQueryCallback(CShadowQuery::QueryCallback);
    CShadowCache::SetRenderCallback(CShadowQuery::RenderCallback);

    g_shadowCull = CVar::Register("shadowCull", "enable shadow frustum culling", 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    g_shadowScissor = CVar::Register("shadowScissor", "enable scissoring when rendering shadowmaps", 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    g_shadowInstancing = CVar::Register("shadowInstancing", "enable instancing when rendering shadowmaps", 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);

    CShadowCache::Initialize();
}

// OFFSET: 0x007BA7D0
void CShadowQuery::FreeBuffers() {
    for (ShadowQueryBuffers* buffers = CShadowQuery::s_bandBuffers; buffers < CShadowQuery::s_bandBuffers + sizeof(CShadowQuery::s_bandBuffers) / sizeof(CShadowQuery::s_bandBuffers[0]); buffers++) {
        if (buffers->m_groups) {
            SMemFree(buffers->m_groups, nullptr, 0, 0);
        }

        if (buffers->m_groupData) {
            SMemFree(buffers->m_groupData, nullptr, 0, 0);
        }

        delete[] buffers->m_untextured.m_batches;
        delete[] buffers->m_textured.m_batches;
    }
}

// VERIFY: 11a5d7a4 manual
// OFFSET: 0x007BB3E0
static void GetViewerPosition(C3Vector* position) {
    *position = DayNight::GetInfo()->m_cameraPos;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        *position = object->GetPosition();
    }
}

// VERIFY: aa0465cc auto
// OFFSET: 0x007BB570
void CShadowQuery::Update() {
    C3Vector direction = CMap::s_mapLight->m_light.m_dir;
    int32_t useCullFrustum = CWorldScene::frustumPortalView.m_maxViewDepth >= 0.0f || CWorldScene::s_hasPendingExteriorGroups;
    float z = direction.z * 5.0f;

    if (z < -1.2f) {
        z = -1.2f;
    }

    float inv = 1.0f / std::sqrt(direction.y * direction.y + direction.x * direction.x + z * z);
    direction.x = direction.x * inv;
    direction.y = direction.y * inv;
    direction.z = z * inv;
    CShadowCache::PreUpdate(direction);
    C3Vector viewer(0.0f, 0.0f, 0.0f);
    GetViewerPosition(&viewer);
    CShadowCache::SetDebugParams(1.0f, 0);
    CShadowCache::Update(viewer, useCullFrustum);
    CShadowCache::PostUpdate();
}

// VERIFY: 7421bb67 manual
// OFFSET: 0x007BAC10
void CShadowQuery::MatrixCallback(C3Vector& center, float radius, C44Matrix& matrix, C3Vector& direction, int32_t band) {
    if (radius <= 0.0f) {
        return;
    }

    C44Matrix ortho;
    C44Matrix view;
    C44Matrix inverseView;
    C44Matrix lookAt;
    C3Vector eye(0.0f, 0.0f, 0.0f);
    C3Vector target(0.0f, 0.0f, 0.0f);
    C3Vector camera(0.0f, 0.0f, 0.0f);
    CGWorldFrame::GetCameraPosition(&camera);

    eye.x = center.x - CShadowCache::s_sunDir.x * 2000.0f - camera.x;
    eye.y = center.y - CShadowCache::s_sunDir.y * 2000.0f - camera.y;
    eye.z = center.z - 2000.0f * CShadowCache::s_sunDir.z - camera.z;
    target.x = center.x - camera.x;
    target.y = center.y - camera.y;
    target.z = center.z - camera.z;

    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];
    C44Matrix copy = view;
    float determinant = copy.Determinant();
    view = copy.Inverse(determinant);
    inverseView = view;

    GxuXformCreateOrtho(-radius, radius, -radius, radius, 1.0f, 4000.0f, ortho);
    GxuXformCreateLookAtXXX(eye, target, direction, lookAt);
    matrix = inverseView * lookAt * ortho;

    C44Matrix depth = inverseView * lookAt;
    matrix.a2 = depth.a2;
    matrix.b2 = depth.b2;
    matrix.c2 = depth.c2;
    matrix.d2 = depth.d2;

    float bias = CShaderEffect::s_usePcfFiltering ? 0.5f : 0.0f;
    float scale;

    if (band == 0) {
        scale = 2.0f;
    } else if (band == 1) {
        matrix.d2 = depth.d2 - (bias + 4.0f * 0.2f);
        return;
    } else if (band == 2) {
        matrix.d2 = depth.d2 - (bias + 8.0f * 0.2f);
        return;
    } else {
        scale = 0.5f;
    }

    matrix.d2 = depth.d2 - (bias + scale * 0.2f);
}

// VERIFY: de87a831 manual
// OFFSET: 0x007BAFD0
void CShadowQuery::FrustumCallback(CFrustum& frustum, C44Matrix& matrix, const C3Vector& direction, CShadowCacheQueryData& data, int32_t band) {
    C3Vector& pos = data.m_pos[band];
    float* bounds = data.m_bounds[band];
    C3Vector eye;
    eye.x = pos.x - CShadowCache::s_sunDir.x * 2000.0f;
    eye.y = pos.y - CShadowCache::s_sunDir.y * 2000.0f;
    eye.z = pos.z - 2000.0f * CShadowCache::s_sunDir.z;

    GxuXformCreateOrtho(bounds[0], bounds[1], bounds[2], bounds[3], 1.0f, 4000.0f, matrix);

    C44Matrix view;
    C3Vector corners[8];

    for (uint32_t i = 0; i < 8; i++) {
        corners[i] = C3Vector(0.0f, 0.0f, 0.0f);
    }

    GxuXformCreateLookAtXXX(eye, pos, direction, view);
    GxuXformCalcFrustumCorners(view, matrix, corners);
    frustum.CalcPlanesFromCorners(corners);
    frustum.InvertPlanes();
    frustum.SwapCorners();

    float* rect = data.m_cullRects[band];
    rect[0] = -1.0f;
    rect[1] = -1.0f;
    rect[2] = 1.0f;
    rect[3] = 1.0f;

    CFrustum& cull = data.m_bandFrustums[band];
    cull = frustum;

    if (data.m_queryFlags[band] == 8 || !g_shadowCull->m_intValue) {
        return;
    }

    CFrustum clip = CWorldScene::s_clipFrustum;
    clip.Transform(view * matrix);
    CAaBox box = CAaBox::Bounding(clip.corners, 8);
    box.b = C3Vector::Max(box.b, C3Vector(-1.0f, -1.0f, -1.0f));
    box.b = C3Vector::Min(box.b, C3Vector(1.0f, 1.0f, 1.0f));
    box.t = C3Vector::Max(box.t, C3Vector(-1.0f, -1.0f, -1.0f));
    box.t = C3Vector::Min(box.t, C3Vector(1.0f, 1.0f, 1.0f));

    if (box.t.x > box.b.x && box.t.y > box.b.y && box.t.z > box.b.z) {
        C44Matrix ortho;
        GxuXformCreateOrtho(-(bounds[0] * box.b.x), bounds[1] * box.t.x, -(bounds[2] * box.b.y), box.t.y * bounds[3], 1.0f, 4000.0f, ortho);
        GxuXformCalcFrustumCorners(view, ortho, corners);
        cull.CalcPlanesFromCorners(corners);
        cull.InvertPlanes();
        cull.SwapCorners();
        rect[1] = box.b.x;
        rect[3] = box.t.x;
        rect[0] = -box.t.y;
        rect[2] = -box.b.y;
    }
}

// VERIFY: dc5e2519 manual
// OFFSET: 0x007BA840
void CShadowQuery::AllocBuffers() {
    for (uint32_t i = 0; i < 3; i++) {
        ShadowQueryBuffers& buffers = CShadowQuery::s_bandBuffers[i];
        buffers.m_groupCount = 0;
        buffers.m_untextured.m_count = 0;
        buffers.m_untextured.m_maxCount = 0x2000;
        buffers.m_textured.m_count = 0;
        buffers.m_textured.m_maxCount = 0x2000;

        if (!buffers.m_groups) {
            buffers.m_groups = SMemAlloc(0x800 * sizeof(CMapObjGroup*), __FILE__, __LINE__, 0);
        }

        if (!buffers.m_groupData) {
            buffers.m_groupData = SMemAlloc(0x800 * sizeof(C44Matrix*), __FILE__, __LINE__, 0);
        }

        if (!buffers.m_untextured.m_batches) {
            buffers.m_untextured.m_batches = static_cast<CModelBatch*>(SMemAlloc(sizeof(CModelBatch) * 0x2000, __FILE__, __LINE__, 0));
        }

        if (!buffers.m_textured.m_batches) {
            buffers.m_textured.m_batches = static_cast<CModelBatch*>(SMemAlloc(sizeof(CModelBatch) * 0x2000, __FILE__, __LINE__, 0));
        }
    }
}

// VERIFY: 3e3dfd04 manual
// OFFSET: 0x007BD200
int32_t CShadowQuery::QueryCallback(CShadowCacheQueryData& data, uint32_t frame) {
    CShadowQuery::AllocBuffers();

    if (!++CShadowQuery::s_stamp) {
        CShadowQuery::s_stamp = 1;
    }

    int32_t flags = 0;

    for (int32_t i = 0; i <= data.m_lastBand; i++) {
        flags |= data.m_queryFlags[i];
    }

    CShadowQuery::GatherShadowCasters(data, 0, data.m_lastBand);

    if (data.m_useFrustum) {
        if (flags & 0xC) {
            CFrustum& frustum = data.m_frustums[data.m_lastBand];
            C3Vector center(0.0f, 0.0f, 0.0f);
            center.x = frustum.corners[0].x;
            center.y = frustum.corners[0].y;
            center.z = frustum.corners[0].z;
            center.x = center.x + frustum.corners[3].x;
            center.y = center.y + frustum.corners[3].y;
            center.z = center.z + frustum.corners[3].z;
            center.x = center.x + frustum.corners[1].x;
            center.y = center.y + frustum.corners[1].y;
            center.z = center.z + frustum.corners[1].z;
            center.x = center.x + frustum.corners[2].x;
            center.y = center.y + frustum.corners[2].y;
            center.z = center.z + frustum.corners[2].z;
            center.x = center.x * 0.25f;
            center.y = center.y * 0.25f;
            center.z = center.z * 0.25f;
            CWorldScene::CreateOcclusionVolumes(&center, frustum.corners, 1);
            CShadowQuery::QueryMapObjDefs(data, 0, data.m_lastBand);
            CShadowQuery::QueryTerrain(data, 0, data.m_lastBand);
        }
    } else {
        for (int32_t band = 0; band <= data.m_lastBand; band++) {
            if (flags & 0xC) {
                CWorldOcclusion::ClearVolumes();
                CShadowQuery::QueryMapObjDefsBand(data, band);
                CShadowQuery::QueryTerrainBand(data, band);
            }
        }
    }

    return 1;
}

// VERIFY: 2abab737 manual
// OFFSET: 0x007BBC50
int32_t CShadowQuery::RenderCallback(CShadowCacheQueryData& data, int32_t band, CGxTex* target, CGxTex* colorTarget, C3Vector& direction) {
    ShadowQueryBuffers& buffers = CShadowQuery::s_bandBuffers[band];

    if (!buffers.m_groupCount && !buffers.m_untextured.m_count && !buffers.m_textured.m_count) {
        CShadowQuery::Clear(target, colorTarget, data.m_texBounds[band]);
        return 1;
    }

    if (colorTarget) {
        GxRenderTargetSet(GxBuffers_Depth, colorTarget, 0);
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Culling, 0);
    CShaderEffect::SetTexMtx_Identity(0);

    C44Matrix projection;
    C3Vector eye(0.0f, 0.0f, 0.0f);
    C3Vector center(0.0f, 0.0f, 0.0f);
    C44Matrix lookAt;
    C44Matrix view;
    C44Matrix inverseView;
    C3Vector camera(0.0f, 0.0f, 0.0f);
    CGxTex* oldTarget;
    g_theGxDevicePtr->DeviceGetRenderTarget(GxBuffers_Color, oldTarget);
    GxRsSet(GxRs_Fog, 0);

    CShaderEffect* effect = CShaderEffectManager::GetEffect("ShadowMapRenderSL");
    CGWorldFrame::GetCameraPosition(&camera);
    C3Vector& pos = data.m_pos[band];
    eye.x = pos.x - CShadowCache::s_sunDir.x * 2000.0f - camera.x;
    eye.y = pos.y - CShadowCache::s_sunDir.y * 2000.0f - camera.y;
    eye.z = pos.z - 2000.0f * CShadowCache::s_sunDir.z - camera.z;
    center.x = pos.x - camera.x;
    center.y = pos.y - camera.y;
    center.z = pos.z - camera.z;

    C44Matrix translation;
    translation.d0 = -camera.x;
    translation.d1 = -camera.y;
    translation.d2 = -camera.z;

    CGxDevice* device = g_theGxDevicePtr;
    projection = device->m_projection;
    float viewportMinX = device->m_viewport.m_x.m_l;
    float viewportMaxX = device->m_viewport.m_x.m_h;
    float viewportMinY = device->m_viewport.m_y.m_l;
    float viewportMaxY = device->m_viewport.m_y.m_h;
    float viewportMinZ = device->m_viewport.m_z.m_l;
    float viewportMaxZ = device->m_viewport.m_z.m_h;
    GxXformSetViewport(data.m_texBounds[band][0], data.m_texBounds[band][1], data.m_texBounds[band][2], data.m_texBounds[band][3], 0.0f, 1.0f);

    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];
    C44Matrix copy = view;
    float determinant = copy.Determinant();
    inverseView = copy.Inverse(determinant);

    effect->SetCurrent();
    float constants[4] = { 0.0f, 0.0f, 0.0f, 0.00025000001f };
    CShadowCache::SetShadowScale(0.00025000001f);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 0, constants, 1);
    CShaderEffect::s_shadowValue = CShadowCache::GetShadowValue();
    g_theGxDevicePtr->XformSetProjection(data.m_matrices[band]);
    CShaderEffect::UpdateProjMatrix();
    GxRsSet(GxRs_ScissorTest, 0);
    g_theGxDevicePtr->DeviceSetRenderTarget(GxBuffers_Color, target, 0);
    GxSceneClear(GxBufferBit_Color | GxBufferBit_Depth, CImVector(0xFF, 0xFF, 0xFF, 0xFF));

    if (g_shadowScissor->m_intValue) {
        GxRsSet(GxRs_ScissorTest, 1);
        float* rect = data.m_cullRects[band];
        CRect scissor;
        scissor.minY = (rect[0] + 1.0f) * 0.5f;
        scissor.minX = (rect[1] + 1.0f) * 0.5f;
        scissor.maxY = (rect[2] + 1.0f) * 0.5f;
        scissor.maxX = 0.5f * (rect[3] + 1.0f);
        g_theGxDevicePtr->SetScissor(scissor);
    }

    GxuXformCreateLookAtXXX(eye, center, direction, lookAt);
    g_theGxDevicePtr->XformSetView(lookAt);
    C44Matrix texMatrix = (inverseView * lookAt).Transpose();
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 14, &texMatrix.a0, 3);

    if (buffers.m_groupCount) {
        CMapObj::RenderMapObjGroupsShadowMap(buffers.m_groups, buffers.m_groupCount, buffers.m_groupData, translation, data.m_frustums[band]);
    }

    if (buffers.m_untextured.m_count || buffers.m_textured.m_count) {
        if (g_shadowInstancing->m_intValue) {
            buffers.m_untextured.Optimize();
            buffers.m_textured.Optimize();
        }

        if (buffers.m_untextured.m_count > s_maxUntexturedBatches) {
            s_maxUntexturedBatches = buffers.m_untextured.m_count;
        }

        if (buffers.m_textured.m_count > s_maxTexturedBatches) {
            s_maxTexturedBatches = buffers.m_textured.m_count;
        }

        CM2Model::RenderModelBatchesShadowMap(buffers.m_untextured, buffers.m_textured);
    }

    g_theGxDevicePtr->RsPop();
    GxXformSetViewport(viewportMinX, viewportMaxX, viewportMinY, viewportMaxY, viewportMinZ, viewportMaxZ);
    g_theGxDevicePtr->XformSetView(view);
    GxRsSet(GxRs_ScissorTest, 0);
    g_theGxDevicePtr->DeviceSetRenderTarget(GxBuffers_Color, oldTarget, 0);
    g_theGxDevicePtr->XformSetProjection(projection);
    CShaderEffect::UpdateProjMatrix();
    return 1;
}

// VERIFY: f464b8c3 manual
// OFFSET: 0x007BB9D0
void CShadowQuery::GatherShadowCasters(CShadowCacheQueryData& data, int32_t firstBand, int32_t lastBand) {
    float maxRadius[3];
    float minRadius[3];
    maxRadius[0] = 10000.0f;

    if ((data.m_queryFlags[0] & 0xC) == 4) {
        maxRadius[0] = 25.0f;
    }

    minRadius[0] = 0.25f;
    maxRadius[1] = 10000.0f;
    minRadius[1] = 2.0f;
    maxRadius[2] = 10000.0f;
    minRadius[2] = 10.0f;

    for (CMapEntity* entity = CMap::entityList.Head(); entity; entity = CMap::entityList.Next(entity)) {
        if (entity->m_staticFlags & 0x4) {
            continue;
        }

        CM2Model* model = entity->m_model;

        if (!model) {
            continue;
        }

        int32_t isUnit = 1;

        if (!(entity->m_type & CMapBaseObj::Type_Entity) || (entity->m_staticFlags & 0x2000)) {
            isUnit = 0;
        }

        uint32_t doodad = (model->m_fFlags & 0x1) ? model->m_shared->m_billboardBoneCount : 0;

        for (int32_t band = firstBand; band <= lastBand; band++) {
            int32_t queryFlags = data.m_queryFlags[band];
            uint32_t flags;

            if (queryFlags & 0x2) {
                if (!isUnit) {
                    continue;
                }

                flags = entity->m_flags;

                if (flags & 0x4) {
                    continue;
                }

                if (CShadowQuery::s_cutoffHeight < entity->m_position.z) {
                    continue;
                }
            } else {
                flags = entity->m_flags;

                if (flags & 0x2) {
                    if (!(entity->m_staticFlags & 0x1000)) {
                        continue;
                    }

                    if (CShadowQuery::s_cutoffHeight < entity->m_position.z) {
                        continue;
                    }
                }
            }

            int32_t wanted;

            if (isUnit) {
                wanted = queryFlags & 0x1;
            } else if (doodad) {
                wanted = queryFlags & 0x4;
            } else {
                wanted = queryFlags & 0x8;
            }

            if (!wanted) {
                continue;
            }

            CAaBox& extents = data.m_extents[band];
            CAaBox& bbox = entity->m_bbox;

            if (extents.t.x >= bbox.b.x && extents.t.y >= bbox.b.y && extents.t.z >= bbox.b.z && extents.b.x <= bbox.t.x && extents.b.y <= bbox.t.y && extents.b.z <= bbox.t.z && minRadius[band] <= entity->m_sphere.r && ((flags & 0x20000) || maxRadius[band] >= entity->m_sphere.r)) {
                if (data.m_bandFrustums[band].Cull(bbox) && (!data.m_useCullFrustum[band] || data.m_cullFrustums[band].CullInv(bbox) != WorldCull_notOutside)) {
                    model->GatherModelBatchesShadowMap(CShadowQuery::s_bandBuffers[band].m_untextured, CShadowQuery::s_bandBuffers[band].m_textured);
                }
            }
        }
    }
}

// VERIFY: 7278bb56 manual
// OFFSET: 0x007BB830
void CShadowQuery::Clear(CGxTex* target, CGxTex* depthTarget, float* rect) {
    CGxTex* oldTarget = nullptr;
    g_theGxDevicePtr->DeviceGetRenderTarget(GxBuffers_Color, oldTarget);
    CGxDevice* device = g_theGxDevicePtr;
    float viewportMinX = device->m_viewport.m_x.m_l;
    float viewportMaxX = device->m_viewport.m_x.m_h;
    float viewportMinY = device->m_viewport.m_y.m_l;
    float viewportMaxY = device->m_viewport.m_y.m_h;
    float viewportMinZ = device->m_viewport.m_z.m_l;
    float viewportMaxZ = device->m_viewport.m_z.m_h;
    GxRenderTargetSet(GxBuffers_Depth, depthTarget, 0);
    GxRenderTargetSet(GxBuffers_Color, target, 0);
    GxXformSetViewport(rect[0], rect[1], rect[2], rect[3], 0.0f, 1.0f);
    GxSceneClear(GxBufferBit_Color | GxBufferBit_Depth, CImVector(0xFF, 0xFF, 0xFF, 0xFF));
    GxXformSetViewport(viewportMinX, viewportMaxX, viewportMinY, viewportMaxY, viewportMinZ, viewportMaxZ);
    GxRenderTargetSet(GxBuffers_Color, oldTarget, 0);
}

// VERIFY: 89a7afcf auto
// OFFSET: 0x007BA8F0
void CShadowQuery::GetSize(CShadowCacheQueryData& data, int32_t band, float* minSize, float* maxSize) {
    *maxSize = 10000.0f;

    if (band == 0) {
        if ((data.m_queryFlags[0] & 0xC) == 4) {
            *maxSize = 25.0f;
            *minSize = 0.25f;
            return;
        }

        *minSize = 0.25f;
        return;
    }

    if (band < 1) {
        *minSize = 0.25f;
        return;
    }

    *minSize = band >= 2 ? 10.0f : 2.0f;
}

// VERIFY: 69b3a5e2 auto
// OFFSET: 0x007BABC0
bool CWorldScene::IsBeyondFadeDistance(const C3Vector& position, uint32_t fadeLevel) {
    float dz = position.z - CWorldScene::s_activeWorldView.z;
    float dy = position.y - CWorldScene::s_activeWorldView.y;
    float dx = position.x - CWorldScene::s_activeWorldView.x;
    return dx * dx + (dy * dy + dz * dz) > CWorldScene::s_fadeDistMinSqr[fadeLevel];
}

// VERIFY: 18313b4e auto
// OFFSET: 0x007BB460
static void GetChunkRect(const CAaBox& box, CiRect& rect) {
    float minY = -(box.t.x - 17066.666f);
    float minX = -(box.t.y - 17066.666f);
    float maxY = -(box.b.x - 17066.666f);
    float maxX = -(box.b.y - 17066.666f);

    if (minX < 0.0f) {
        minX = 0.0f;
    }

    if (minY < 0.0f) {
        minY = 0.0f;
    }

    if (maxX >= 34133.332f) {
        maxX = 34132.332f;
    }

    if (maxY >= 34133.332f) {
        maxY = 34132.332f;
    }

    int32_t cellMaxX = CMath::fint_(maxX * OO_COORD_TO_SUBCHUNK - 0.5f);
    int32_t cellMaxY = CMath::fint_(maxY * OO_COORD_TO_SUBCHUNK - 0.5f);
    int32_t cellMinX = CMath::fint_(minX * OO_COORD_TO_SUBCHUNK - 0.5f);
    int32_t cellMinY = CMath::fint_(minY * OO_COORD_TO_SUBCHUNK - 0.5f);
    rect.minY = cellMinY >> 3;
    rect.minX = cellMinX >> 3;
    rect.maxY = cellMaxY >> 3;
    rect.maxX = cellMaxX >> 3;
}

// VERIFY: 9b4413e5 manual
// OFFSET: 0x007BC890
void CShadowQuery::QueryMapObjDefsBand(CShadowCacheQueryData& data, int32_t band) {
    float minSize;
    float maxSize;
    CShadowQuery::GetSize(data, band, &minSize, &maxSize);

    for (CMapObjDef* def = CMap::mapObjDefHash.m_fulllist.Head(); def; def = CMap::mapObjDefHash.Next(def)) {
        uint32_t flags = def->m_flags;

        if (flags & 0x20) {
            continue;
        }

        float maxDoodadSize = maxSize;
        int32_t interior = 0;
        int32_t castDoodads = 0;
        int32_t castOther = 0;

        if (flags & 0x400) {
            if (!(data.m_queryFlags[band] & 0x4)) {
                continue;
            }

            maxDoodadSize = 100000.0f;
            castDoodads = 1;
            castOther = 1;
            interior = 1;
        } else {
            int32_t queryFlags = data.m_queryFlags[band];

            if (queryFlags & 0x8) {
                castOther = 1;
            }

            if (queryFlags & 0x4) {
                castDoodads = 1;
            }
        }

        CAaBox& extents = data.m_extents[band];

        if (!extents.Intersects(def->m_bbox)) {
            continue;
        }

        if (!data.m_bandFrustums[band].Cull(def->m_bbox) || (data.m_useCullFrustum[band] && data.m_cullFrustums[band].CullInv(def->m_bbox) == WorldCull_notOutside)) {
            continue;
        }

        uint32_t groupCount = def->m_defGroups.m_count == 0xFFFFFFFF ? def->m_defGroups.m_dynamic.m_count : def->m_defGroups.m_count;

        for (uint32_t i = 0; i < groupCount; i++) {
            CMapObjDefGroup* defGroup = def->m_defGroups[i];
            CMapObjGroup* group = def->m_owner->GetGroup(i, 0);

            if (!group || !(group->m_flags & 0x48)) {
                continue;
            }

            if (!extents.Intersects(defGroup->m_bbox) || !data.m_bandFrustums[band].Cull(defGroup->m_bbox)) {
                continue;
            }

            if (data.m_useCullFrustum[band] && data.m_cullFrustums[band].CullInv(defGroup->m_bbox) == WorldCull_notOutside) {
                continue;
            }

            if (interior ? (data.m_queryFlags[band] & 0x4) : (data.m_queryFlags[band] & 0x8)) {
                ShadowQueryBuffers& buffers = CShadowQuery::s_bandBuffers[band];

                if (buffers.m_groupCount >= 0x800) {
                    break;
                }

                static_cast<C44Matrix**>(buffers.m_groupData)[buffers.m_groupCount] = &def->m_mat;
                static_cast<CMapObjGroup**>(buffers.m_groups)[buffers.m_groupCount++] = group;
            }

            for (CMapBaseObjLink* link = defGroup->m_doodadDefLinkList.Head(); link; link = defGroup->m_doodadDefLinkList.Next(link)) {
                CMapDoodadDef* doodad = static_cast<CMapDoodadDef*>(link->m_owner);

                if (doodad->m_shadowStamps[band] == CShadowQuery::s_stamp) {
                    continue;
                }

                doodad->m_shadowStamps[band] = CShadowQuery::s_stamp;
                CM2Model* model = doodad->m_model;

                if (!model || !(doodad->m_flags & 0x80) || CWorldScene::IsBeyondFadeDistance(doodad->m_sphere.c, doodad->m_fadeLevel) || maxDoodadSize < doodad->m_sphere.r) {
                    continue;
                }

                int32_t wanted = model->GetBillboardBoneCount() ? castDoodads : castOther;

                if (wanted && extents.Intersects(doodad->m_bbox) && data.m_bandFrustums[band].Cull(doodad->m_bbox) && (!data.m_useCullFrustum[band] || data.m_cullFrustums[band].CullInv(doodad->m_bbox) != WorldCull_notOutside)) {
                    model->GatherModelBatchesShadowMap(CShadowQuery::s_bandBuffers[band].m_untextured, CShadowQuery::s_bandBuffers[band].m_textured);
                }
            }
        }
    }
}

// VERIFY: 34693e83 manual
// OFFSET: 0x007BC490
void CShadowQuery::QueryMapObjDefs(CShadowCacheQueryData& data, int32_t firstBand, int32_t lastBand) {
    for (CMapObjDef* def = CMap::mapObjDefHash.m_fulllist.Head(); def; def = CMap::mapObjDefHash.Next(def)) {
        if (def->m_flags & 0x20) {
            continue;
        }

        CAaBox& lastExtents = data.m_extents[lastBand];

        if (!(lastExtents.t.x >= def->m_bbox.b.x && lastExtents.t.y >= def->m_bbox.b.y && lastExtents.t.z >= def->m_bbox.b.z && lastExtents.b.x <= def->m_bbox.t.x && lastExtents.b.y <= def->m_bbox.t.y && lastExtents.b.z <= def->m_bbox.t.z)) {
            continue;
        }

        CFrustum& lastFrustum = data.m_bandFrustums[lastBand];

        if (!lastFrustum.Cull(def->m_bbox)) {
            continue;
        }

        uint32_t groupCount = def->m_defGroups.m_count == 0xFFFFFFFF ? def->m_defGroups.m_dynamic.m_count : def->m_defGroups.m_count;

        for (uint32_t i = 0; i < groupCount; i++) {
            CMapObjDefGroup* defGroup = def->m_defGroups[i];
            CMapObjGroup* group = def->m_owner->GetGroup(i, 0);

            if (!group || !(group->m_flags & 0x48)) {
                continue;
            }

            if (!lastExtents.Intersects(defGroup->m_bbox) || !lastFrustum.Cull(defGroup->m_bbox)) {
                continue;
            }

            for (int32_t band = firstBand; band <= lastBand; band++) {
                if (data.m_extents[band].Intersects(defGroup->m_bbox) && data.m_bandFrustums[band].Cull(defGroup->m_bbox) && (!data.m_useCullFrustum[band] || data.m_cullFrustums[band].CullInv(defGroup->m_bbox) != WorldCull_notOutside)) {
                    ShadowQueryBuffers& buffers = CShadowQuery::s_bandBuffers[band];

                    if (buffers.m_groupCount >= 0x800) {
                        break;
                    }

                    static_cast<C44Matrix**>(buffers.m_groupData)[buffers.m_groupCount] = &def->m_mat;
                    static_cast<CMapObjGroup**>(buffers.m_groups)[buffers.m_groupCount++] = group;
                }
            }

            for (CMapBaseObjLink* link = defGroup->m_doodadDefLinkList.Head(); link; link = defGroup->m_doodadDefLinkList.Next(link)) {
                CMapDoodadDef* doodad = static_cast<CMapDoodadDef*>(link->m_owner);

                if (!doodad->m_model || !(doodad->m_flags & 0x80)) {
                    continue;
                }

                for (int32_t band = firstBand; band <= lastBand; band++) {
                    if (doodad->m_shadowStamps[band] == CShadowQuery::s_stamp) {
                        continue;
                    }

                    doodad->m_shadowStamps[band] = CShadowQuery::s_stamp;

                    if (!CWorldScene::IsBeyondFadeDistance(doodad->m_sphere.c, doodad->m_fadeLevel) && data.m_extents[band].Intersects(doodad->m_bbox) && data.m_bandFrustums[band].Cull(doodad->m_bbox) && (!data.m_useCullFrustum[band] || data.m_cullFrustums[band].CullInv(doodad->m_bbox) != WorldCull_notOutside)) {
                        doodad->m_model->GatherModelBatchesShadowMap(CShadowQuery::s_bandBuffers[band].m_untextured, CShadowQuery::s_bandBuffers[band].m_textured);
                    }
                }
            }
        }
    }
}

// VERIFY: 896bfb08 manual
// OFFSET: 0x007BCF20
void CShadowQuery::QueryTerrainBand(CShadowCacheQueryData& data, int32_t band) {
    float minSize;
    float maxSize;
    CShadowQuery::GetSize(data, band, &minSize, &maxSize);
    CAaBox& extents = data.m_extents[band];
    CiRect rect;
    rect.minY = 0;
    rect.minX = 0;
    rect.maxY = 0;
    rect.maxX = 0;
    GetChunkRect(extents, rect);

    for (int32_t row = rect.minY; row <= rect.maxY; row++) {
        for (int32_t col = rect.minX; col <= rect.maxX; col++) {
            CMapArea* area = CMap::m_areaTable[(((row >> 4) & 0x3F) << 6) + ((col >> 4) & 0x3F)];

            if (!area || area->m_asyncObject) {
                continue;
            }

            CMapChunk* chunk = area->m_mapChunks[(col & 0xF) + 16 * (row & 0xF)];

            if (!chunk) {
                continue;
            }

            if (!(extents.t.x >= chunk->m_bbox.b.x && extents.t.y >= chunk->m_bbox.b.y && extents.t.z >= chunk->m_bbox.b.z && extents.b.x <= chunk->m_bbox.t.x && extents.b.y <= chunk->m_bbox.t.y && extents.b.z <= chunk->m_bbox.t.z)) {
                continue;
            }

            if (!data.m_bandFrustums[band].Cull(chunk->m_bbox) || (data.m_useCullFrustum[band] && data.m_cullFrustums[band].CullInv(chunk->m_bbox) == WorldCull_notOutside)) {
                continue;
            }

            for (CMapBaseObjLink* link = chunk->m_doodadDefLinkList.Head(); link; link = chunk->m_doodadDefLinkList.Next(link)) {
                CMapDoodadDef* doodad = static_cast<CMapDoodadDef*>(link->m_owner);

                if (doodad->m_shadowStamps[band] == CShadowQuery::s_stamp) {
                    continue;
                }

                doodad->m_shadowStamps[band] = CShadowQuery::s_stamp;
                CM2Model* model = doodad->m_model;

                if (!model || !(doodad->m_flags & 0x80) || CWorldScene::IsBeyondFadeDistance(doodad->m_sphere.c, doodad->m_fadeLevel) || maxSize < doodad->m_sphere.r) {
                    continue;
                }

                int32_t wanted = model->GetBillboardBoneCount() ? (data.m_queryFlags[band] & 0x4) : (data.m_queryFlags[band] & 0x8);

                if (wanted && extents.Intersects(doodad->m_bbox) && data.m_bandFrustums[band].Cull(doodad->m_bbox) && (!data.m_useCullFrustum[band] || data.m_cullFrustums[band].CullInv(doodad->m_bbox) != WorldCull_notOutside)) {
                    model->GatherModelBatchesShadowMap(CShadowQuery::s_bandBuffers[band].m_untextured, CShadowQuery::s_bandBuffers[band].m_textured);
                }
            }
        }
    }
}

// VERIFY: ba8a3951 manual
// OFFSET: 0x007BCC00
void CShadowQuery::QueryTerrain(CShadowCacheQueryData& data, int32_t firstBand, int32_t lastBand) {
    CiRect rect;
    rect.minY = 0;
    rect.minX = 0;
    rect.maxY = 0;
    rect.maxX = 0;
    GetChunkRect(data.m_extents[data.m_lastBand], rect);

    for (int32_t row = rect.minY; row <= rect.maxY; row++) {
        for (int32_t col = rect.minX; col <= rect.maxX; col++) {
            CMapArea* area = CMap::m_areaTable[(((row >> 4) & 0x3F) << 6) + ((col >> 4) & 0x3F)];

            if (!area || area->m_asyncObject) {
                continue;
            }

            CMapChunk* chunk = area->m_mapChunks[(col & 0xF) + 16 * (row & 0xF)];

            if (!chunk) {
                continue;
            }

            CAaBox& lastExtents = data.m_extents[lastBand];

            if (!(lastExtents.t.x >= chunk->m_bbox.b.x && lastExtents.t.y >= chunk->m_bbox.b.y && lastExtents.t.z >= chunk->m_bbox.b.z && lastExtents.b.x <= chunk->m_bbox.t.x && lastExtents.b.y <= chunk->m_bbox.t.y && lastExtents.b.z <= chunk->m_bbox.t.z)) {
                continue;
            }

            if (!data.m_bandFrustums[lastBand].Cull(chunk->m_bbox) || CWorldOcclusion::QueryVolumes(&chunk->m_sphere)) {
                continue;
            }

            for (CMapBaseObjLink* link = chunk->m_doodadDefLinkList.Head(); link; link = chunk->m_doodadDefLinkList.Next(link)) {
                CMapDoodadDef* doodad = static_cast<CMapDoodadDef*>(link->m_owner);
                CM2Model* model = doodad->m_model;

                if (!model || !(doodad->m_flags & 0x80)) {
                    continue;
                }

                uint32_t doodadFlag = model->GetBillboardBoneCount();

                for (int32_t band = firstBand; band <= lastBand; band++) {
                    if (doodad->m_shadowStamps[band] == CShadowQuery::s_stamp) {
                        continue;
                    }

                    doodad->m_shadowStamps[band] = CShadowQuery::s_stamp;

                    if (CWorldScene::IsBeyondFadeDistance(doodad->m_sphere.c, doodad->m_fadeLevel)) {
                        continue;
                    }

                    if (!(doodadFlag ? (data.m_queryFlags[band] & 0x4) : (data.m_queryFlags[band] & 0x8))) {
                        continue;
                    }

                    if (data.m_extents[band].Intersects(doodad->m_bbox) && data.m_bandFrustums[band].Cull(doodad->m_bbox) && (!data.m_useCullFrustum[band] || data.m_cullFrustums[band].CullInv(doodad->m_bbox) != WorldCull_notOutside)) {
                        model->GatherModelBatchesShadowMap(CShadowQuery::s_bandBuffers[band].m_untextured, CShadowQuery::s_bandBuffers[band].m_textured);
                    }
                }
            }
        }
    }
}
