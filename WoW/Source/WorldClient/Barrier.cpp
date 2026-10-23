#include "Barrier.h"
#include "MapObjDef.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Tempest/C3Ray.h"
#include "Engine/Source/Tempest/C3Segment.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include <cmath>
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00CD85F8
CM2Model* CBarrier::s_sparkles[4];

// STATIC: 0x00CD8608
CTexture* CBarrier::s_pattern;

// STATIC: 0x00CD860C
CTexture* CBarrier::s_mask;

// STATIC: 0x00CDD79C
static float s_patternPhase;

// VERIFY: 5b06e6d6 auto
// OFFSET: 0x00790900
CGxVertexPCT0T1::CGxVertexPCT0T1() {
    this->m_t0x = 0.0f;
    this->m_t0y = 0.0f;
    this->m_t1x = 0.0f;
    this->m_t1y = 0.0f;
}

// VERIFY: a0dbf924 manual
// OFFSET: 0x00792360
void CBarrier::AddBarrierConvexPoly(C3Vector* verts, uint32_t count, float radius, C44Matrix* mat, int32_t flip, int32_t oneSided) {
    if (!this->m_mode || this->m_verts.Count() > 1000) {
        return;
    }

    static CGxVertexPCT0T1 s_polyVerts[12];
    static C3Vector s_polyPoints[12];

    C3Vector* points = verts;

    if (mat) {
        for (uint32_t i = 0; i < 3; i++) {
            s_polyPoints[i] = verts[i] * *mat;
        }

        points = s_polyPoints;
    }

    CBarrier_Contact contact;
    contact.m_plane.n = C3Vector(0.0f, 0.0f, 1.0f);
    contact.m_plane.d = 0.0f;
    contact.m_plane.From3Pos(points[0], points[1], points[2]);

    C3Vector up(0.0f, 0.0f, 1.0f);

    if (contact.m_plane.n.z > 0.99f || contact.m_plane.n.z < -0.99f) {
        up = C3Vector(0.0f, 1.0f, 0.0f);
    }

    float planeDist = contact.m_plane.n.z * this->m_viewerPos.z + this->m_viewerPos.y * contact.m_plane.n.y + this->m_viewerPos.x * contact.m_plane.n.x + contact.m_plane.d;

    if (flip) {
        planeDist *= -1.0f;
    }

    if (oneSided && planeDist < 0.0f) {
        return;
    }

    if (planeDist > radius) {
        return;
    }

    if (mat && count > 3) {
        for (uint32_t i = 3; i < count; i++) {
            s_polyPoints[i] = verts[i] * *mat;
        }
    }

    float polyDist = NTempest::DistanceFromPolygon(this->m_viewerPos, points, count, contact.m_plane);

    if (radius < polyDist) {
        return;
    }

    float range = radius - polyDist + radius - polyDist;

    if (range < 0.5f) {
        return;
    }

    float fade = polyDist / radius;
    float clamped = fade < 0.0f ? 0.0f : (fade >= 1.0f ? 1.0f : fade);
    contact.m_strength = 1.0f - clamped;

    C3Ray ray;
    ray.origin = this->m_viewerPos;
    ray.dir = -contact.m_plane.n;

    if (!NTempest::Intersect(ray, contact.m_plane, nullptr, &contact.m_point, 0.0099999998f)) {
        return;
    }

    this->m_contacts.Add(1, &contact);

    C44Matrix lookAt;
    GxuXformCreateLookAtSgCompat(C3Vector(0.0f, 0.0f, 0.0f), contact.m_plane.n, up, lookAt);
    C44Matrix pattern = lookAt;
    lookAt.Translate(-this->m_viewerPos);

    if (radius < 12.0f) {
        pattern.Scale(4.0f);
    }

    float scale = 1.0f / range;

    for (uint32_t i = 0; i < count; i++) {
        CGxVertexPCT0T1& vert = s_polyVerts[i];
        vert.m_pos = points[i];
        float alpha = fade < 0.0f ? 0.0f : (fade >= 1.0f ? 1.0f : fade);
        vert.m_color.a = static_cast<uint8_t>((1.0f - alpha) * 255.0f);

        C3Vector maskPos = points[i] * lookAt;
        vert.m_t0x = maskPos.x * scale + 0.5f;
        vert.m_t0y = 0.5f - scale * maskPos.y;

        C3Vector patternPos = points[i] * pattern;
        vert.m_t1x = patternPos.x;
        vert.m_t1y = patternPos.y;
    }

    for (uint32_t i = 0; i < count - 2; i++) {
        this->m_verts.Add(1, &s_polyVerts[0]);
        this->m_verts.Add(2, &s_polyVerts[i + 1]);
    }
}

// VERIFY: 758e6fc9 manual
// OFFSET: 0x007944C0
void CBarrier::AddAxisAlignedWall(const C2Vector& start, const C2Vector& end, float radius) {
    if (!this->m_mode) {
        return;
    }

    float t = 3.4028235e38f;
    C3Segment segment;
    segment.b = C3Vector(start.x, start.y, this->m_viewerPos.z);
    segment.t = C3Vector(end.x, end.y, this->m_viewerPos.z);
    float dist = sqrt(NTempest::DistanceSquared(segment, this->m_viewerPos, &t));

    if (0.99900001f * radius < dist) {
        return;
    }

    float dx = segment.t.x - segment.b.x;
    float height = radius - dist;
    float dy = segment.t.y - segment.b.y;
    float dz = segment.t.z - segment.b.z;
    float halfWidth = height / sqrt(dx * dx + dy * dy + dz * dz);

    float t0 = t - halfWidth;
    t0 = t0 < 0.0f ? 0.0f : (t0 >= 1.0f ? 1.0f : t0);
    float x0 = dx * t0 + segment.b.x;
    float y0 = dy * t0 + segment.b.y;
    float z0 = t0 * dz + segment.b.z;

    float t1 = halfWidth + t;
    t1 = t1 < 0.0f ? 0.0f : (t1 >= 1.0f ? 1.0f : t1);
    float z1 = dz * t1 + segment.b.z;

    C3Vector quad[4];
    quad[0] = C3Vector(x0, y0, height + z0);
    quad[1] = C3Vector(x0, y0, z0 - height);
    quad[2] = C3Vector(dx * t1 + segment.b.x, dy * t1 + segment.b.y, z1 - height);
    quad[3] = C3Vector(dx * t1 + segment.b.x, dy * t1 + segment.b.y, height + z1);
    this->AddBarrierConvexPoly(quad, 4, radius, nullptr, 0, 0);
}

// VERIFY: d2c5f0b9 manual
// OFFSET: 0x007946D0
void CBarrier::AddBarrier(const CAaBox& box, float radius) {
    if (!this->m_mode || !(box.t.x > box.b.x && box.t.y > box.b.y && box.t.z > box.b.z)) {
        return;
    }

    CAaBox grown;
    grown.b.x = box.b.x - 0.050000001f;
    grown.b.y = box.b.y - 0.050000001f;
    grown.b.z = box.b.z - 0.050000001f;
    grown.t.x = box.t.x + 0.050000001f;
    grown.t.y = box.t.y + 0.050000001f;
    grown.t.z = box.t.z + 0.050000001f;

    if (NTempest::Distance(grown, this->m_viewerPos) > radius) {
        return;
    }

    C3Vector quad[4];
    float bottom = grown.b.z - 1.0f;

    quad[0] = C3Vector(grown.b.x, grown.b.y, bottom);
    quad[1] = C3Vector(grown.b.x, grown.t.y, bottom);
    quad[2] = C3Vector(grown.b.x, grown.t.y, grown.t.z);
    quad[3] = C3Vector(grown.b.x, grown.b.y, grown.t.z);
    this->AddBarrierConvexPoly(quad, 4, radius, nullptr, 0, 0);

    quad[0] = C3Vector(grown.b.x, grown.t.y, bottom);
    quad[1] = C3Vector(grown.t.x, grown.t.y, bottom);
    quad[2] = C3Vector(grown.t.x, grown.t.y, grown.t.z);
    quad[3] = C3Vector(grown.b.x, grown.t.y, grown.t.z);
    this->AddBarrierConvexPoly(quad, 4, radius, nullptr, 0, 0);

    quad[0] = C3Vector(grown.t.x, grown.t.y, bottom);
    quad[1] = C3Vector(grown.t.x, grown.b.y, bottom);
    quad[2] = C3Vector(grown.t.x, grown.b.y, grown.t.z);
    quad[3] = C3Vector(grown.t.x, grown.t.y, grown.t.z);
    this->AddBarrierConvexPoly(quad, 4, radius, nullptr, 0, 0);

    quad[0] = C3Vector(grown.t.x, grown.b.y, bottom);
    quad[1] = C3Vector(grown.b.x, grown.b.y, bottom);
    quad[2] = C3Vector(grown.b.x, grown.b.y, grown.t.z);
    quad[3] = C3Vector(grown.t.x, grown.b.y, grown.t.z);
    this->AddBarrierConvexPoly(quad, 4, radius, nullptr, 0, 0);

    quad[0] = C3Vector(grown.b.x, grown.b.y, grown.t.z);
    quad[1] = C3Vector(grown.t.x, grown.b.y, grown.t.z);
    quad[2] = C3Vector(grown.t.x, grown.t.y, grown.t.z);
    quad[3] = C3Vector(grown.b.x, grown.t.y, grown.t.z);
    this->AddBarrierConvexPoly(quad, 4, radius, nullptr, 0, 0);
}

// VERIFY: dba1cd72 auto
// OFFSET: 0x00794AD0
void CBarrier::AddBarrierMapObjDefGroup(CMapObjDefGroup* mapObjDefGroup, float radius) {
    if (!this->m_mode) {
        return;
    }

    if (radius < NTempest::Distance(mapObjDefGroup->m_bbox, this->m_viewerPos)) {
        return;
    }

    for (uint32_t i = 0; i < this->m_groups.Count(); i++) {
        if (this->m_groups.m_data[i] == mapObjDefGroup) {
            return;
        }
    }

    this->AddBarrier(mapObjDefGroup->m_bbox, radius);
    this->m_groups.Add(1, &mapObjDefGroup);
}

// VERIFY: 6badcb93 manual
// OFFSET: 0x00799730
void CBarrier::Initialize() {
    CGxTexFlags flags;
    CStatus status;

    flags.m_filter = GxTex_Linear;
    flags.m_wrapU = 0;
    flags.m_wrapV = 0;
    CBarrier::s_mask = TextureCreate("XTextures\\FX\\LBTBarrierMask", flags, &status, 0);

    flags.m_wrapU = 1;
    flags.m_wrapV = 1;
    CBarrier::s_pattern = TextureCreate("XTextures\\FX\\LBTBarrier", flags, &status, 0);
}

// VERIFY: f774ccf6 manual
// OFFSET: 0x0078F510
void CBarrier::Destroy() {
    for (uint32_t i = 0; i < sizeof(CBarrier::s_sparkles) / sizeof(CBarrier::s_sparkles[0]); i++) {
        if (CBarrier::s_sparkles[i]) {
            CBarrier::s_sparkles[i]->Release();
            CBarrier::s_sparkles[i] = nullptr;
        }
    }

    if (CBarrier::s_mask) {
        HandleClose(reinterpret_cast<HOBJECT>(CBarrier::s_mask));
        CBarrier::s_mask = nullptr;
    }

    if (CBarrier::s_pattern) {
        HandleClose(reinterpret_cast<HOBJECT>(CBarrier::s_pattern));
        CBarrier::s_pattern = nullptr;
    }
}

// VERIFY: 52b19b7e manual
// OFFSET: 0x00794B50
void CBarrier::Render(const C3Vector& viewerPos, float elapsedSec) {
    if (!this->m_mode || !this->m_verts.Count()) {
        return;
    }

    int32_t contactCount = this->m_contacts.Count();

    if (contactCount > 4) {
        contactCount = 4;
    }

    for (int32_t i = 0; i < contactCount; i++) {
        CM2Model* sparkle = CBarrier::s_sparkles[i];

        if (!sparkle) {
            continue;
        }

        CBarrier_Contact& contact = this->m_contacts.m_data[i];
        const C3Vector& normal = contact.m_plane.n;
        C3Vector right;
        right.x = normal.y - normal.z * 0.0f;
        right.y = normal.z * 0.0f - normal.x;
        right.z = normal.x * 0.0f - 0.0f * normal.y;
        C3Vector up;
        up.x = right.y * normal.z - right.z * normal.y;
        up.y = right.z * normal.x - right.x * normal.z;
        up.z = right.x * normal.y - right.y * normal.x;
        C44Matrix transform(normal, right, up, contact.m_point);
        sparkle->SetAnimating(1);
        CM2Model* model = CBarrier::s_sparkles[i];

        if (model->m_attachParent) {
            model->m_fFlags |= 0x80;
        } else {
            model->m_fFlags |= 0x8;
        }

        if (model->m_attachParent) {
            model->m_fFlags |= 0x20000;
        } else {
            model->m_fFlags |= 0x10000;
        }

        CBarrier::s_sparkles[i]->m_fFlags |= 0x8000;
        CBarrier::s_sparkles[i]->m_worldTransform = transform;
        CBarrier::s_sparkles[i]->m_fadeAlpha = this->m_contacts.m_data[i].m_strength;

        for (uint32_t j = 0; ; j++) {
            CM2Model* loaded = CBarrier::s_sparkles[i];

            if (!(loaded->m_fFlags & 0x1)) {
                loaded->WaitForLoad(nullptr);
            }

            if (j >= loaded->m_shared->m_data->m_particles.m_count) {
                break;
            }

            CM2Model* emitterModel = CBarrier::s_sparkles[i];

            if (!(emitterModel->m_fFlags & 0x1)) {
                emitterModel->WaitForLoad(nullptr);
            }

            CParticleEmitter2* emitter = emitterModel->m_particleEmitters[j];
            emitter->SetWidth(100.0f);
            emitter->SetHeight(100.0f);
        }
    }

    C44Matrix world;
    world.Translate(-viewerPos);
    g_theGxDevicePtr->XformPushAndSet(GxXform_World, world);
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_DepthTest, 1);
    GxRsSet(GxRs_BlendingMode, 3);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    CGxTex* maskTex = TextureGetGxTex(CBarrier::s_mask, 1, nullptr);

    if (!maskTex) {
        return;
    }

    CGxTex* patternTex = TextureGetGxTex(CBarrier::s_pattern, 1, nullptr);

    if (!patternTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, maskTex);
    GxRsSet(GxRs_TextureCoord0, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, patternTex);
    GxRsSet(GxRs_TextureCoord1, 1);
    C44Matrix identity;
    g_theGxDevicePtr->XformPushAndSet(GxXform_Tex0, identity);
    g_theGxDevicePtr->XformPushAndSet(GxXform_Tex1, identity);
    GxRsSet(GxRs_TextureShader0, 1);
    GxRsSet(GxRs_TextureShader1, 1);
    GxRsSet(GxRs_ColorOp1, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp1, GxTexOp_Mod2x);
    uint32_t vertCount = this->m_verts.Count();
    uint32_t triangleCount = vertCount / 3;
    uint32_t bufferVertCount = 2 * vertCount;
    uint32_t indexCount = 6 * triangleCount;
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT0T1), bufferVertCount);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), indexCount);
    CGxVertexPCT0T1* vertices = reinterpret_cast<CGxVertexPCT0T1*>(g_theGxDevicePtr->BufLock(vertexBuf));
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    s_patternPhase = elapsedSec * 0.1f + s_patternPhase;

    if (s_patternPhase > 3.1415927f) {
        s_patternPhase = s_patternPhase - 3.1415927f;
    }

    if (vertices && indices) {
        int32_t passCount = 2 * triangleCount;

        for (int32_t t = 0; t < passCount; t++) {
            indices[0] = 3 * t;
            indices[2] = 3 * t + 2;
            indices[1] = 3 * t + 1;
            indices += 3;
            const CGxVertexPCT0T1* source = &this->m_verts.m_data[3 * (t / 2)];
            int32_t odd = t & 1;

            for (int32_t k = 0; k < 3; k++) {
                vertices->m_pos = source->m_pos;
                vertices->m_t0x = source->m_t0x;
                vertices->m_t0y = source->m_t0y;
                vertices->m_t1x = source->m_t1x;
                vertices->m_t1y = source->m_t1y;
                CImVector color;
                color.r = 0;
                color.b = 0xFF;
                color.a = source->m_color.a;
                color.g = odd ? 0xFF : 0;

                if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                    CImVector swapped;
                    swapped.a = color.a;
                    swapped.r = 0xFF;
                    swapped.g = color.g;
                    swapped.b = 0;
                    color = swapped;
                }

                vertices->m_color = color;

                if (odd) {
                    vertices->m_t1x = source->m_t1x * 0.050000001f + 0.5f;
                    vertices->m_t1y = 0.050000001f * source->m_t1y - static_cast<float>(fmod(s_patternPhase, 3.1415927410125732)) * 1.5915494f;
                } else {
                    vertices->m_t1x = source->m_t1x * 0.1f;
                    vertices->m_t1y = 0.1f * source->m_t1y - static_cast<float>(fmod(s_patternPhase, 3.1415927410125732)) * 1.5915494f;
                }

                vertices++;
                source++;
            }
        }
    }

    g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
    vertexBuf->m_hasData = 1;
    g_theGxDevicePtr->BufUnlock(indexBuf, 0);
    indexBuf->m_hasData = 1;
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT2);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = bufferVertCount - 1;
    g_theGxDevicePtr->Draw(&batch, 1);
    GxXformPop(GxXform_Tex0);
    GxXformPop(GxXform_Tex1);
    g_theGxDevicePtr->RsPop();
    GxXformPop(GxXform_World);
    this->m_verts.SetCount(0);
    this->m_contacts.SetCount(0);
    this->m_chunks.m_count = 0;
    this->m_groups.m_count = 0;
}
