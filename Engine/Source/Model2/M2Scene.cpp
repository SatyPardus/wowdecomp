#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>
#include <xmmintrin.h>
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Model2/CM2SceneRender.h"
#include "Engine/Source/Services/GfxSingletonManager.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/Services/RibbonEmitter.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C3Ray.h"
#include "Engine/Source/Tempest/NTempest.h"

// STATIC: 0x00A45374
const uint32_t s_localLighting[M2BLEND_COUNT] = { 1, 1, 1, 1, 1, 0, 0 };

// STATIC: 0x00A453B0
const uint32_t s_blendModes[3][M2BLEND_COUNT] = { { 0, 1, 2, 10, 3, 4, 5 }, { 2, 2, 2, 10, 3, 4, 5 }, { 2, 2, 2, 10, 3, 4, 5 } };

// STATIC: 0x00AF5898
uint32_t CM2Scene::s_optFlags = 0xFFFFFFFF;

// STATIC: 0x00AF58A0
static const float s_lightGridScale = 0.05f;

// STATIC: 0x00D40DA0
static uint32_t s_doodadHash[251];

// VERIFY: abe62346 auto
// OFFSET: 0x0081C990
void CM2Scene::SetTime(uint32_t time) {
    this->m_time = time;
    this->m_cache->UpdateShared();
    this->m_cache->GarbageCollect(0);
}

// VERIFY: fb95d1fc auto
// OFFSET: 0x0081C9C0
void CM2Scene::AdvanceTime(uint32_t delta) {
    this->m_time += delta;
    this->m_cache->UpdateShared();
    this->m_cache->GarbageCollect(0);
    this->m_flags |= 4;
    this->m_timeDelta = delta;

    if (delta) {
        CM2Model* model = this->m_animateList;

        while (model) {
            CM2Model* next = model->m_animateNext;
            model->ProcessCallbacksRecursive();
            model = next;
        }
    }

    this->m_flags &= ~4u;
}

// VERIFY: 19239262 auto
// OFFSET: 0x0081CA10
void CM2Scene::SetFloat18(float value) {
    this->m_float18 = value;
}

// VERIFY: a7f69442 manual
// OFFSET: 0x0081CA20
uint32_t ParticleBlendToM2Blend(uint32_t blend) {
    switch (blend) {
        case 1:
            return M2BLEND_ALPHA_KEY;
        case 2:
            return M2BLEND_ALPHA;
        case 3:
            return M2BLEND_ADD;
        case 4:
            return M2BLEND_MOD;
        case 5:
            return M2BLEND_MOD2X;
        case 10:
            return M2BLEND_NO_ALPHA_ADD;
        default:
            return M2BLEND_OPAQUE;
    }
}

// VERIFY: 26bf08dc manual
// OFFSET: 0x0081CA80
uint32_t ParticleFlagsToM2Flags(uint32_t flags) {
    uint32_t result = 4;

    if (!(flags & 1)) {
        result = 5;
    }

    if (!(flags & 2)) {
        result |= 2;
    }

    if (!(flags & 4)) {
        result |= 0x10;
    }

    return result;
}

// VERIFY: 500fa91d auto
// OFFSET: 0x0081CAB0
void CM2Scene::PurgeModelCache() {
    if (this->m_cache) {
        this->m_cache->GarbageCollect(1);
    }
}

// VERIFY: e5d07ffa auto
// OFFSET: 0x0081CAC0
void CM2Scene::BeginHitTest() {
    this->m_flags |= 2;
}

// VERIFY: fec263be manual
// OFFSET: 0x0081CAD0
void CM2Scene::AllocateSpaceForHitList() {
    uint32_t count = 0;

    for (CM2Model* model = this->m_hitTestList; model; model = model->m_hitTestNext) {
        count++;
    }

    if (count <= this->m_hitCapacity) {
        return;
    }

    delete[] this->m_hitRecs;
    delete[] this->m_hitOrder;

    if (!this->m_hitCapacity) {
        this->m_hitCapacity = 1;
    }

    while (this->m_hitCapacity < count) {
        this->m_hitCapacity *= 2;
    }

    this->m_hitRecs = new (__FILE__, __LINE__) M2HitRec[this->m_hitCapacity];
    this->m_hitOrder = new (__FILE__, __LINE__) uint32_t[this->m_hitCapacity];
}

// VERIFY: 49f2efe7 manual
// OFFSET: 0x0081CBC0
int32_t CM2Scene::SortHitNear(uint32_t a, uint32_t b, const void* param) {
    const M2HitRec* recs = static_cast<const M2HitRec*>(param);
    const M2HitRec* recA = &recs[a];
    const M2HitRec* recB = &recs[b];

    if (recB->m_tNear > recA->m_tNear) {
        return -1;
    }

    if (recB->m_tNear < recA->m_tNear) {
        return 1;
    }

    if (recB->m_tFar > recA->m_tFar) {
        return -1;
    }

    if (recB->m_tFar < recA->m_tFar) {
        return 1;
    }

    if (b > a) {
        return -1;
    }

    return b < a;
}

// VERIFY: 31f1f896 auto
// OFFSET: 0x0081CC30
void CM2Scene::SetProjectTextureCallback(void (*callback)(const CAaBox&, const CImVector&, int32_t, void*, int32_t), void* param) {
    this->m_projectTextureCallback = callback;
    this->m_projectTextureParam = param;
}

// VERIFY: 48d6ecf1 manual
// OFFSET: 0x0081CC50
static uint32_t __stdcall HashDoodadElement(const M2Element* element) {
    const CM2Model* model = element->m_model;
    const CM2Lighting* lighting = model->m_currentLighting;
    uint32_t colorIndex = element->m_batch->m_colorIndex;

    const uint32_t* light = reinterpret_cast<const uint32_t*>(lighting);
    uint32_t hash = light[23] + 19 * (light[22] + 19 * (light[21] + 19 * (light[26] + 19 * (light[25] + 19 * (19 * (light[48] + 19 * (light[47] + 19 * (19 * (light[43] + 19 * (light[42] + 19 * (element->m_index + 19 * static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model->m_shared))))) + light[46]))) + light[24])))));

    if (colorIndex < model->m_shared->m_data->m_colors.m_count) {
        const uint32_t* color = reinterpret_cast<const uint32_t*>(&model->m_colors[colorIndex].m_colorTrack.m_currentValue);
        hash = color[2] + 19 * (color[1] + 19 * (19 * hash + color[0]));
    }

    const uint32_t* diffuse = reinterpret_cast<const uint32_t*>(&model->m_diffuseColor);
    const uint32_t* emissive = reinterpret_cast<const uint32_t*>(&model->m_emissiveColor);

    return reinterpret_cast<const uint32_t*>(&model->unk_01B8)[0] + 19 * (emissive[2] + 19 * (emissive[1] + 19 * (19 * (diffuse[2] + 19 * (diffuse[1] + 19 * (19 * hash + diffuse[0]))) + emissive[0])));
}

// VERIFY: 3a0615ca auto
// OFFSET: 0x0081CD70
CM2Model* CM2ModelHeap::New() {
    uint32_t handle;
    void* memory = nullptr;

    if (!ObjectAlloc(this->m_heapId, &handle, &memory, false)) {
        return nullptr;
    }

    CM2Model* model = memory ? new (memory) CM2Model() : nullptr;
    model->m_handle = handle;

    return model;
}

// VERIFY: f41c995e auto
// OFFSET: 0x0081CE70
void CM2Scene::AnimateThread(void* param) {
    CM2Scene* scene = static_cast<CM2Scene*>(param);

    for (CM2Model* model = scene->m_animateList; model; model = model->m_animateNext) {
        model = model->m_animateNext;

        if (!model) {
            break;
        }

        if (model->m_attachParent) {
            continue;
        }

        if (model->m_fFlags & 0x1000) {
            model->AnimateMTSimple(&scene->m_view, C3Vector(1.0f, 1.0f, 1.0f), C3Vector(0.0f, 0.0f, 0.0f), 1.0f, 1.0f);
        } else {
            model->AnimateMT(&scene->m_view, C3Vector(1.0f, 1.0f, 1.0f), C3Vector(0.0f, 0.0f, 0.0f), 1.0f, 1.0f);
        }
    }
}

// VERIFY: 3ba88f51 manual
// OFFSET: 0x0081CF20
int32_t CM2Scene::ComputeRayDirAndLen(const C3Vector& start, const C3Vector& end, float t, float* length, C3Vector* dir) {
    if (t >= 0.0000099999997f) {
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float dz = end.z - start.z;
        float len = sqrtf(dx * dx + dz * dz + dy * dy);
        *length = len;

        if (len >= 0.0000099999997f) {
            float inv = 1.0f / len;
            dir->x = dx * inv;
            dir->y = dy * inv;
            dir->z = inv * dz;
            return 1;
        }
    }

    for (CM2Model* model = this->m_hitTestList; model; model = model->m_hitTestNext) {
        *model->m_hitTestPrev = nullptr;
        model->m_hitTestPrev = nullptr;
    }

    this->m_flags &= ~2u;

    return 0;
}

// VERIFY: 2e7f6ece manual
// OFFSET: 0x0081CFF0
uint32_t CM2Scene::SphereTestModels(const C3Vector& start, const C3Vector& dir, float length, int32_t frameCheck) {
    uint32_t count = 0;

    for (CM2Model* model = this->m_hitTestList; model; model = model->m_hitTestNext) {
        *model->m_hitTestPrev = nullptr;
        model->m_hitTestPrev = nullptr;

        if (!(model->m_fFlags & 1) || model->m_frameStamp == 0xFFFFFFFF) {
            continue;
        }

        if (frameCheck && model->m_frameStamp != this->m_frameStamp) {
            if (model->m_hitTestMode != 3 || model->m_attachParent) {
                continue;
            }

            model->m_matrixF4 = model->m_worldTransform * this->m_view;
        }

        M2Data* data = model->m_shared->m_data;
        const M2Bounds* bounds = model->m_hitTestMode == 3 ? &data->m_collisionBounds : &data->m_sequences[model->m_bones->m_sequence.m_sequenceIndex].m_bounds;

        if (fabsf(bounds->m_radius) < 0.00000023841858f) {
            continue;
        }

        const C44Matrix& matrix = model->m_matrixF4;
        C3Vector center((bounds->m_extent.t.x + bounds->m_extent.b.x) * 0.5f, (bounds->m_extent.t.y + bounds->m_extent.b.y) * 0.5f, 0.5f * (bounds->m_extent.t.z + bounds->m_extent.b.z));
        C3Vector position = center * matrix;
        float vx = position.x - start.x;
        float vy = position.y - start.y;
        float vz = position.z - start.z;
        float proj = dir.x * vx + dir.z * vz + dir.y * vy;
        float px = dir.x * proj - vx;
        float py = dir.y * proj - vy;
        float pz = dir.z * proj - vz;
        float radiusSq = bounds->m_radius * ((matrix.a2 * matrix.a2 + matrix.a1 * matrix.a1 + matrix.a0 * matrix.a0) * bounds->m_radius);
        float distSq = px * px + py * py + pz * pz;

        if (distSq > radiusSq) {
            continue;
        }

        float halfSq = radiusSq - distSq;

        if (proj < 0.0f && proj * proj > halfSq) {
            continue;
        }

        float over = proj - length;

        if (over > 0.0f && over * over > halfSq) {
            continue;
        }

        float half = sqrtf(halfSq);
        float tNear = proj - half;

        if ((tNear >= 0.0f ? tNear : 0.0f) > length) {
            tNear = length;
        } else if (tNear < 0.0f) {
            tNear = 0.0f;
        }

        float tFar = proj + half;

        if ((tFar >= 0.0f ? tFar : 0.0f) > length) {
            tFar = length;
        } else if (tFar < 0.0f) {
            tFar = 0.0f;
        }

        M2HitRec* rec = &this->m_hitRecs[count];
        rec->m_tNear = tNear;
        rec->m_model = model;
        rec->m_tFar = tFar;
        rec->m_priority = model->m_hitTestGroup;
        this->m_hitOrder[count] = count;
        count++;
    }

    return count;
}

// VERIFY: d49fadc9 manual
// OFFSET: 0x0081D2C0
void M2BlendBoneMatricesSSE(const C44Matrix* bones, ubyte4 weights, ubyte4 indices, C44Matrix* result) {
    const float* bone = reinterpret_cast<const float*>(&bones[indices.b[0]]);
    __m128 weight = _mm_set1_ps(weights.b[0] * 0.0039215689f);
    __m128 row0 = _mm_mul_ps(_mm_loadu_ps(&bone[0]), weight);
    __m128 row1 = _mm_mul_ps(_mm_loadu_ps(&bone[4]), weight);
    __m128 row2 = _mm_mul_ps(_mm_loadu_ps(&bone[8]), weight);
    __m128 row3 = _mm_mul_ps(_mm_loadu_ps(&bone[12]), weight);

    for (uint32_t i = 1; i < 4; i++) {
        if (!weights.b[i]) {
            break;
        }

        bone = reinterpret_cast<const float*>(&bones[indices.b[i]]);
        weight = _mm_set1_ps(weights.b[i] * 0.0039215689f);
        row0 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&bone[0]), weight), row0);
        row1 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&bone[4]), weight), row1);
        row2 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&bone[8]), weight), row2);
        row3 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&bone[12]), weight), row3);
    }

    float* out = reinterpret_cast<float*>(result);
    _mm_storeu_ps(&out[0], row0);
    _mm_storeu_ps(&out[4], row1);
    _mm_storeu_ps(&out[8], row2);
    _mm_storeu_ps(&out[12], row3);
    result->a3 = 0.0f;
    result->b3 = 0.0f;
    result->c3 = 0.0f;
    result->d3 = 1.0f;
}

// VERIFY: f7794afd auto
// OFFSET: 0x0081D3D0
void M2BlendBoneMatrices(const C44Matrix* bones, ubyte4 weights, ubyte4 indices, C44Matrix* result) {
    const C44Matrix* bone = &bones[indices.b[0]];
    float weight = weights.b[0] * 0.0039215689f;
    result->a0 = bone->a0 * weight;
    result->a1 = bone->a1 * weight;
    result->a2 = bone->a2 * weight;
    result->b0 = bone->b0 * weight;
    result->b1 = bone->b1 * weight;
    result->b2 = bone->b2 * weight;
    result->c0 = bone->c0 * weight;
    result->c1 = bone->c1 * weight;
    result->c2 = bone->c2 * weight;
    result->d0 = bone->d0 * weight;
    result->d1 = bone->d1 * weight;
    result->d2 = weight * bone->d2;

    for (uint32_t i = 1; i < 4; i++) {
        if (!weights.b[i]) {
            break;
        }

        bone = &bones[indices.b[i]];
        weight = weights.b[i] * 0.0039215689f;
        result->a0 = bone->a0 * weight + result->a0;
        result->a1 = bone->a1 * weight + result->a1;
        result->a2 = bone->a2 * weight + result->a2;
        result->b0 = bone->b0 * weight + result->b0;
        result->b1 = bone->b1 * weight + result->b1;
        result->b2 = bone->b2 * weight + result->b2;
        result->c0 = bone->c0 * weight + result->c0;
        result->c1 = bone->c1 * weight + result->c1;
        result->c2 = bone->c2 * weight + result->c2;
        result->d0 = bone->d0 * weight + result->d0;
        result->d1 = bone->d1 * weight + result->d1;
        result->d2 = weight * bone->d2 + result->d2;
    }
}

// VERIFY: 27fe3564 manual
// OFFSET: 0x0081D510
M2HitRec* CM2Scene::IntersectHitTestTriangles(const uint16_t* indices, const uint16_t* end, uint32_t vertexStart, const C3Vector& start, uint32_t pass, M2HitRec* rec, float& t, M2HitRec* best) {
    for (; indices < end; indices += 3) {
        const C3Vector& v0 = this->m_hitVerts[indices[0] - vertexStart];
        const C3Vector& v1 = this->m_hitVerts[indices[1] - vertexStart];
        const C3Vector& v2 = this->m_hitVerts[indices[2] - vertexStart];
        float det = (v2.y - v0.y) * (v1.x - v0.x) - (v2.x - v0.x) * (v1.y - v0.y);

        if (fabsf(det) < 0.0000099999997f) {
            continue;
        }

        float inv = 1.0f / det;
        float ax = v1.x - start.x;
        float ay = v1.y - start.y;
        float bx = v2.x - start.x;
        float by = v2.y - start.y;
        float w0 = (by * ax - bx * ay) * inv;

        if (w0 < 0.0f) {
            continue;
        }

        float cy = v0.y - start.y;
        float cx = v0.x - start.x;
        float w1 = (bx * cy - by * cx) * inv;

        if (w1 < 0.0f) {
            continue;
        }

        float w2 = inv * (ay * cx - ax * cy);

        if (w2 < 0.0f) {
            continue;
        }

        float z = w1 * v1.z + w2 * v2.z + v0.z * w0;

        if (z >= 0.0f && ((pass && (!best || best->m_priority != rec->m_priority)) || z <= t)) {
            t = z;
            best = rec;
        }
    }

    return best;
}

// VERIFY: 5ae1f294 manual
// OFFSET: 0x0081D680
void CM2Scene::TransformHitTestVerticesBlendSSE(const CM2Model* model, const M2SkinProfile* skinData, const M2SkinSection* section, int32_t normals, const C3Vector& dir, float d) {
    M2Data* data = model->m_shared->m_data;
    C3Vector* out = this->m_hitVerts;
    C44Matrix matrix;
    uint32_t weights = 0;
    uint32_t indices = 0;
    uint32_t end = section->m_vertexStart + section->m_vertexCount;

    for (uint32_t i = section->m_vertexStart; i < end; i++) {
        M2Vertex* vertex = &data->m_vertices[skinData->m_vertices[i]];

        if (*reinterpret_cast<uint32_t*>(&vertex->m_weights) != weights || *reinterpret_cast<uint32_t*>(&vertex->m_indices) != indices) {
            weights = *reinterpret_cast<uint32_t*>(&vertex->m_weights);
            indices = *reinterpret_cast<uint32_t*>(&vertex->m_indices);
            M2BlendBoneMatricesSSE(model->m_boneMatrices, vertex->m_weights, vertex->m_indices, &matrix);
        }

        C3Vector p = vertex->m_position * matrix;

        if (normals) {
            p.x = vertex->m_normal.z * matrix.c0 + vertex->m_normal.y * matrix.b0 + vertex->m_normal.x * matrix.a0 + p.x;
            p.y = vertex->m_normal.z * matrix.c1 + vertex->m_normal.y * matrix.b1 + vertex->m_normal.x * matrix.a1 + p.y;
            p.z = vertex->m_normal.z * matrix.c2 + vertex->m_normal.y * matrix.b2 + vertex->m_normal.x * matrix.a2 + p.z;
        }

        float dist = dir.z * p.z + dir.y * p.y + dir.x * p.x - d;
        out->x = p.x - dir.x * dist;
        out->y = p.y - dir.y * dist;
        out->z = dist;
        out++;
    }
}

// VERIFY: 28c512ee manual
// OFFSET: 0x0081D830
void CM2Scene::TransformHitTestVerticesBlend(const CM2Model* model, const M2SkinProfile* skinData, const M2SkinSection* section, int32_t normals, const C3Vector& dir, float d) {
    M2Data* data = model->m_shared->m_data;
    C3Vector* out = this->m_hitVerts;
    C44Matrix matrix;
    uint32_t weights = 0;
    uint32_t indices = 0;
    uint32_t end = section->m_vertexStart + section->m_vertexCount;

    for (uint32_t i = section->m_vertexStart; i < end; i++) {
        M2Vertex* vertex = &data->m_vertices[skinData->m_vertices[i]];

        if (*reinterpret_cast<uint32_t*>(&vertex->m_weights) != weights || *reinterpret_cast<uint32_t*>(&vertex->m_indices) != indices) {
            weights = *reinterpret_cast<uint32_t*>(&vertex->m_weights);
            indices = *reinterpret_cast<uint32_t*>(&vertex->m_indices);
            M2BlendBoneMatrices(model->m_boneMatrices, vertex->m_weights, vertex->m_indices, &matrix);
        }

        C3Vector p = vertex->m_position * matrix;

        if (normals) {
            p.x = vertex->m_normal.z * matrix.c0 + vertex->m_normal.y * matrix.b0 + vertex->m_normal.x * matrix.a0 + p.x;
            p.y = vertex->m_normal.z * matrix.c1 + vertex->m_normal.y * matrix.b1 + vertex->m_normal.x * matrix.a1 + p.y;
            p.z = vertex->m_normal.z * matrix.c2 + vertex->m_normal.y * matrix.b2 + vertex->m_normal.x * matrix.a2 + p.z;
        }

        float dist = dir.z * p.z + dir.y * p.y + dir.x * p.x - d;
        out->x = p.x - dir.x * dist;
        out->y = p.y - dir.y * dist;
        out->z = dist;
        out++;
    }
}

// VERIFY: e43ce013 manual
// OFFSET: 0x0081D9C0
void CM2Scene::TransformHitTestVertices(const CM2Model* model, const M2SkinProfile* skinData, const M2SkinSection* section, int32_t normals, const C3Vector& dir, float d) {
    M2Data* data = model->m_shared->m_data;
    C3Vector* out = this->m_hitVerts;
    uint32_t end = section->m_vertexStart + section->m_vertexCount;

    for (uint32_t i = section->m_vertexStart; i < end; i++) {
        M2Vertex* vertex = &data->m_vertices[skinData->m_vertices[i]];
        const C44Matrix& matrix = model->m_boneMatrices[vertex->m_indices.b[0]];
        C3Vector p = vertex->m_position * matrix;

        if (normals) {
            p.x = matrix.c0 * vertex->m_normal.z + matrix.b0 * vertex->m_normal.y + vertex->m_normal.x * matrix.a0 + p.x;
            p.y = matrix.c1 * vertex->m_normal.z + matrix.b1 * vertex->m_normal.y + matrix.a1 * vertex->m_normal.x + p.y;
            p.z = matrix.c2 * vertex->m_normal.z + matrix.b2 * vertex->m_normal.y + matrix.a2 * vertex->m_normal.x + p.z;
        }

        float dist = dir.z * p.z + dir.y * p.y + dir.x * p.x - d;
        out->x = p.x - dir.x * dist;
        out->y = p.y - dir.y * dist;
        out->z = dist;
        out++;
    }
}

// VERIFY: d07f8475 manual
// OFFSET: 0x0081DAF0
M2HitRec* CM2Scene::HitTestGeometry(const CM2Model* model, uint32_t pass, const C3Vector& dir, float d, const C3Vector& start, M2HitRec* rec, float& t, M2HitRec* best) {
    M2SkinProfile* skinData = model->m_shared->m_skinData;
    M2Data* data = model->m_shared->m_data;

    for (uint32_t i = 0; i < skinData->m_batches.m_count; i++) {
        M2Batch* batch = &skinData->m_batches[i];

        if (batch->m_materialLayer || (batch->m_flags & 8)) {
            continue;
        }

        M2Material* material = &data->m_materials[batch->m_materialIndex];

        if (model->m_hitTestMode == 2 && material->m_blendMode == M2BLEND_OPAQUE) {
            continue;
        }

        if (model->m_hitTestMode == 1 && material->m_blendMode && !(material->m_flags & 0x20)) {
            continue;
        }

        if (!model->m_skinSections[batch->m_skinSectionIndex]) {
            continue;
        }

        float alpha = model->m_alpha;

        if (batch->m_colorIndex < data->m_colors.m_count) {
            alpha *= model->m_colors[batch->m_colorIndex].m_alphaTrack.m_currentValue;
        }

        if (batch->m_textureCount) {
            alpha *= model->m_textureWeights[data->m_textureWeightCombos[batch->m_textureWeightComboIndex]].m_currentValue;
        }

        if (alpha <= 0.0f) {
            continue;
        }

        M2SkinSection* section = &skinData->m_skinSections[batch->m_skinSectionIndex];

        if (section->m_vertexCount > this->m_hitVertCapacity) {
            delete[] this->m_hitVerts;

            if (!this->m_hitVertCapacity) {
                this->m_hitVertCapacity = 1;
            }

            while (this->m_hitVertCapacity < section->m_vertexCount) {
                this->m_hitVertCapacity *= 2;
            }

            this->m_hitVerts = new (__FILE__, __LINE__) C3Vector[this->m_hitVertCapacity];
        }

        if (section->m_boneInfluences == 1) {
            this->TransformHitTestVertices(model, skinData, section, pass, dir, d);
        } else if (CM2Cache::s_processorFeatures & 4) {
            this->TransformHitTestVerticesBlendSSE(model, skinData, section, pass, dir, d);
        } else {
            this->TransformHitTestVerticesBlend(model, skinData, section, pass, dir, d);
        }

        const uint16_t* indices = &skinData->m_indices[section->m_indexStart];
        best = this->IntersectHitTestTriangles(indices, indices + section->m_indexCount, section->m_vertexStart, start, pass, rec, t, best);
    }

    return best;
}

// VERIFY: 4f08e629 manual
// OFFSET: 0x0081DD50
M2HitRec* CM2Scene::HitTestCollision(const CM2Model* model, uint32_t pass, const C3Vector& dir, float d, const C3Vector& start, M2HitRec* rec, float& t, M2HitRec* best) {
    M2Data* data = model->m_shared->m_data;

    if (data->m_collisionPositions.m_count > this->m_hitVertCapacity) {
        delete[] this->m_hitVerts;

        if (!this->m_hitVertCapacity) {
            this->m_hitVertCapacity = 1;
        }

        while (this->m_hitVertCapacity < data->m_collisionPositions.m_count) {
            this->m_hitVertCapacity *= 2;
        }

        this->m_hitVerts = new (__FILE__, __LINE__) C3Vector[this->m_hitVertCapacity];
    }

    for (uint32_t i = 0; i < data->m_collisionPositions.m_count; i++) {
        C3Vector p = data->m_collisionPositions[i] * model->m_matrixF4;
        C3Vector* out = &this->m_hitVerts[i];
        float dist = dir.y * p.y + dir.x * p.x + dir.z * p.z - d;
        out->x = p.x - dir.x * dist;
        out->y = p.y - dir.y * dist;
        out->z = dist;
    }

    const uint16_t* indices = data->m_collisionIndices.Data();

    return this->IntersectHitTestTriangles(indices, indices + data->m_collisionIndices.m_count, 0, start, pass, rec, t, best);
}

// VERIFY: eaeb7d7b manual
// OFFSET: 0x0081DF10
void* CM2Scene::EndHitTest(const C3Vector& start, const C3Vector& end, float& t, int32_t twoPass) {
    C3Vector dir(0.0f, 0.0f, 0.0f);
    float length;

    if (!this->ComputeRayDirAndLen(start, end, t, &length, &dir)) {
        return nullptr;
    }

    this->AllocateSpaceForHitList();

    uint32_t count = this->SphereTestModels(start, dir, length, 1);
    M2HeapSort(&CM2Scene::SortHitNear, this->m_hitOrder, count, this->m_hitRecs);

    M2HitRec* best = nullptr;
    float d = start.z * dir.z + start.y * dir.y + start.x * dir.x;
    float tMax = length;

    if (t < 1.0f) {
        tMax = t * length;
    }

    for (uint32_t pass = 0; ;) {
        for (uint32_t i = 0; i < count; i++) {
            M2HitRec* rec = &this->m_hitRecs[this->m_hitOrder[i]];

            if (pass) {
                if (best && (best->m_priority > rec->m_priority || (best->m_priority == rec->m_priority && tMax <= rec->m_tNear))) {
                    continue;
                }
            } else if (tMax <= rec->m_tNear) {
                break;
            }

            if (rec->m_model->m_hitTestMode == 3) {
                best = this->HitTestCollision(rec->m_model, pass, dir, d, start, rec, tMax, best);
            } else {
                best = this->HitTestGeometry(rec->m_model, pass, dir, d, start, rec, tMax, best);
            }
        }

        if (best || !twoPass || ++pass >= 2) {
            break;
        }

        tMax = length;
    }

    this->m_flags &= ~2u;

    if (!best) {
        return nullptr;
    }

    t = tMax / length;

    void* owner = nullptr;

    for (CM2Model* model = best->m_model; model; model = model->m_attachParent) {
        if (model->m_hitTestOwner != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
            owner = model->m_hitTestOwner;
            break;
        }
    }

    this->m_lastHit = *best;
    this->m_lastHitOwner = owner;

    return owner;
}

// VERIFY: 9a48c159 manual
// OFFSET: 0x0081E110
void* CM2Scene::EndHitTestCollisionWorld(const C3Vector& start, const C3Vector& end, float& t) {
    C3Vector dir(0.0f, 0.0f, 0.0f);
    float length;

    if (!this->ComputeRayDirAndLen(start, end, t, &length, &dir)) {
        return nullptr;
    }

    this->AllocateSpaceForHitList();

    C3Vector viewStart = start * this->m_view;
    C33Matrix rotation(this->m_view);
    C3Vector viewDir(rotation.a0 * dir.x + rotation.b0 * dir.y + rotation.c0 * dir.z, rotation.c1 * dir.z + rotation.b1 * dir.y + rotation.a1 * dir.x, dir.y * rotation.b2 + dir.z * rotation.c2 + dir.x * rotation.a2);
    float viewLength = sqrtf(this->m_view.a2 * this->m_view.a2 + this->m_view.a1 * this->m_view.a1 + this->m_view.a0 * this->m_view.a0) * length;

    uint32_t count = this->SphereTestModels(viewStart, viewDir, viewLength, 0);
    M2HeapSort(&CM2Scene::SortHitNear, this->m_hitOrder, count, this->m_hitRecs);

    float tMax = length;

    if (t < 1.0f) {
        tMax = length * t;
    }

    M2HitRec* best = nullptr;

    for (uint32_t i = 0; i < count; i++) {
        M2HitRec* rec = &this->m_hitRecs[this->m_hitOrder[i]];

        if (tMax <= rec->m_tNear) {
            break;
        }

        CM2Model* model = rec->m_model;
        M2Data* data = model->m_shared->m_data;

        if (model->m_hitTestMode != 3) {
            continue;
        }

        C44Matrix inverse = model->m_worldTransform.Inverse(model->m_worldTransform.Determinant());
        C3Vector localStart = start * inverse;
        C3Vector localEnd = end * inverse;

        C3Ray ray;
        float dx = localEnd.x - localStart.x;
        float dy = localEnd.y - localStart.y;
        float dz = localEnd.z - localStart.z;
        float invLength = 1.0f / sqrtf(dy * dy + dz * dz + dx * dx);
        ray.origin = localStart;
        ray.dir.x = dx * invLength;
        ray.dir.y = dy * invLength;
        ray.dir.z = invLength * dz;

        const uint16_t* indices = data->m_collisionIndices.Data();
        const uint16_t* indicesEnd = indices + data->m_collisionIndices.m_count;

        for (; indices < indicesEnd; indices += 3) {
            float hitT;

            if (NTempest::Intersect(ray, data->m_collisionPositions.Data(), indices, &hitT, nullptr, 0.000001f) && hitT >= 0.0f) {
                float dist = hitT * invLength * length;

                if (tMax >= dist) {
                    tMax = dist;
                    best = rec;
                }
            }
        }
    }

    this->m_flags &= ~2u;

    if (!best) {
        return nullptr;
    }

    t = tMax / length;

    return best->m_model->m_hitTestOwner;
}

// VERIFY: c1963846 manual
// OFFSET: 0x0081E400
void CM2Scene::SelectLights(CM2Lighting* lighting) {
    for (CM2Light* light = this->m_lightList; light; light = light->m_lightNext) {
        lighting->AddLight(light);
    }

    if (!this->m_lightGrid) {
        return;
    }

    const CAaSphere& sphere = lighting->m_sphere4;
    uint32_t maxX = static_cast<int32_t>(floorf((sphere.c.x + sphere.r) * s_lightGridScale + 0.5f)) & 0x3F;
    uint32_t minY = static_cast<int32_t>(floorf((sphere.c.y - sphere.r) * s_lightGridScale - 0.5f)) & 0x3F;
    uint32_t maxY = static_cast<int32_t>(floorf((sphere.r + sphere.c.y) * s_lightGridScale + 0.5f)) & 0x3F;
    uint32_t minX = static_cast<int32_t>(floorf((sphere.c.x - sphere.r) * s_lightGridScale - 0.5f)) & 0x3F;

    for (uint32_t x = minX; ; x = (x + 1) & 0x3F) {
        for (uint32_t y = minY; ; y = (y + 1) & 0x3F) {
            CM2Light* light = this->m_lightGrid[x + 64 * y];

            while (light) {
                CM2Light* next = light->m_lightNext;

                if (!light->m_scene || light->m_stamp == this->m_frameStamp) {
                    lighting->AddLight(light);
                } else {
                    light->SetVisible(0);
                }

                light = next;
            }

            if (y == maxY) {
                break;
            }
        }

        if (x == maxX) {
            break;
        }
    }
}

// VERIFY: b4238a8b auto
// OFFSET: 0x0081E590
void CM2Scene::SetProjectPositionCallback(int32_t (*callback)(const C3Vector&, float&, void*), void* param) {
    this->m_projectPositionCallback = callback;
    this->m_projectPositionParam = param;
    ParticleSystemManager::sm_projectCallback = callback;
    ParticleSystemManager::sm_projectParam = param;
}

// VERIFY: 00778290 manual
// OFFSET: 0x0081E5C0
int32_t CM2Scene::SortDoodad(uint32_t a, uint32_t b, const void* param) {
    const CM2Scene* scene = static_cast<const CM2Scene*>(param);
    const M2Element* elementA = &scene->m_elements.m_data[a];
    const M2Element* elementB = &scene->m_elements.m_data[b];
    const CM2Model* modelA = elementA->m_model;
    const CM2Model* modelB = elementB->m_model;

    if (modelA->m_shared < modelB->m_shared) {
        return -1;
    }

    if (modelA->m_shared > modelB->m_shared) {
        return 1;
    }

    if (static_cast<uint32_t>(elementA->m_index) < static_cast<uint32_t>(elementB->m_index)) {
        return -1;
    }

    if (static_cast<uint32_t>(elementA->m_index) > static_cast<uint32_t>(elementB->m_index)) {
        return 1;
    }

    const CM2Lighting* lightingA = modelA->m_currentLighting;
    const CM2Lighting* lightingB = modelB->m_currentLighting;

    if (lightingA != lightingB) {
        if (lightingB->m_fogStart > lightingA->m_fogStart) {
            return -1;
        }

        if (lightingB->m_fogStart < lightingA->m_fogStart) {
            return 1;
        }

        if (lightingB->m_fogEnd > lightingA->m_fogEnd) {
            return -1;
        }

        if (lightingB->m_fogEnd < lightingA->m_fogEnd) {
            return 1;
        }

        int32_t result = memcmp(&lightingA->m_fogColor, &lightingB->m_fogColor, sizeof(C3Vector));

        if (result < 0) {
            return -1;
        }

        if (result > 0) {
            return 1;
        }

        result = memcmp(&lightingA->m_sunDiffuse, &lightingB->m_sunDiffuse, sizeof(C3Vector));

        if (result < 0) {
            return -1;
        }

        if (result > 0) {
            return 1;
        }

        result = memcmp(&lightingA->m_sunAmbient, &lightingB->m_sunAmbient, sizeof(C3Vector));

        if (result < 0) {
            return -1;
        }

        if (result > 0) {
            return 1;
        }
    }

    uint32_t colorIndex = elementA->m_batch->m_colorIndex;

    if (colorIndex < modelA->m_shared->m_data->m_colors.m_count) {
        int32_t result = memcmp(&modelA->m_colors[colorIndex].m_colorTrack.m_currentValue, &modelB->m_colors[colorIndex].m_colorTrack.m_currentValue, sizeof(C3Vector));

        if (result < 0) {
            return -1;
        }

        if (result > 0) {
            return 1;
        }
    }

    int32_t result = memcmp(&modelA->m_diffuseColor, &modelB->m_diffuseColor, sizeof(C3Vector));

    if (result < 0) {
        return -1;
    }

    if (result > 0) {
        return 1;
    }

    result = memcmp(&modelA->m_emissiveColor, &modelB->m_emissiveColor, sizeof(C3Vector));

    if (result < 0) {
        return -1;
    }

    if (result > 0) {
        return 1;
    }

    if (modelB->unk_01B8 > modelA->unk_01B8) {
        return -1;
    }

    return modelB->unk_01B8 < modelA->unk_01B8;
}

// VERIFY: 3739cccf manual
// OFFSET: 0x0081EA90
int32_t CM2Scene::SortDoodadProxy(uint32_t a, uint32_t b, const void* param) {
    const CM2Scene* scene = static_cast<const CM2Scene*>(param);
    uint32_t keyB = scene->m_elements.m_data[b].m_doodadKey;
    uint32_t keyA = scene->m_elements.m_data[a].m_doodadKey;

    if (keyB > keyA) {
        return -1;
    }

    return keyB < keyA;
}

// VERIFY: 7a8c9939 manual
// OFFSET: 0x0081EAD0
int32_t CM2Scene::SortOpaqueGeoBatches(M2Element* a, M2Element* b) {
    M2Data* dataA = a->m_model->m_shared->m_data;
    M2Batch* batchA = a->m_batch;
    M2Data* dataB = b->m_model->m_shared->m_data;
    M2Batch* batchB = b->m_batch;

    if (a->m_type == 0) {
        if (batchA->m_materialLayer < batchB->m_materialLayer) {
            return -1;
        }

        if (batchA->m_materialLayer > batchB->m_materialLayer) {
            return 1;
        }

        if (a->m_effect && b->m_effect) {
            CGxShader* vertexA = a->m_effect->m_vertexShaders[a->m_vertexPermute];
            CGxShader* pixelA = a->m_effect->m_pixelShaders[a->m_pixelPermute];
            CGxShader* vertexB = b->m_effect->m_vertexShaders[b->m_vertexPermute];
            CGxShader* pixelB = b->m_effect->m_pixelShaders[b->m_pixelPermute];

            if (vertexA < vertexB) {
                return -1;
            }

            if (vertexA > vertexB) {
                return 1;
            }

            if (pixelA < pixelB) {
                return -1;
            }

            if (pixelA > pixelB) {
                return 1;
            }
        }

        if (a->m_model->m_shared < b->m_model->m_shared) {
            return -1;
        }

        if (a->m_model->m_shared > b->m_model->m_shared) {
            return 1;
        }

        int32_t optimizedA = a->m_flags & 4;
        int32_t optimizedB = b->m_flags & 4;

        if (optimizedA < optimizedB) {
            return -1;
        }

        if (optimizedA > optimizedB) {
            return 1;
        }

        if (a->m_model < b->m_model) {
            return -1;
        }

        if (a->m_model > b->m_model) {
            return 1;
        }

        if (a->m_skinSection->m_boneComboIndex < b->m_skinSection->m_boneComboIndex) {
            return -1;
        }

        if (a->m_skinSection->m_boneComboIndex > b->m_skinSection->m_boneComboIndex) {
            return 1;
        }
    }

    M2Material* materialA = &dataA->m_materials[batchA->m_materialIndex];
    M2Material* materialB = &dataB->m_materials[batchB->m_materialIndex];

    if (materialA->m_blendMode < materialB->m_blendMode) {
        return -1;
    }

    if (materialA->m_blendMode > materialB->m_blendMode) {
        return 1;
    }

    uint32_t flagsA = materialA->m_flags & 0x1F;
    uint32_t flagsB = materialB->m_flags & 0x1F;

    if (flagsA < flagsB) {
        return -1;
    }

    if (flagsA > flagsB) {
        return 1;
    }

    uint16_t count = batchA->m_textureCount < batchB->m_textureCount ? batchA->m_textureCount : batchB->m_textureCount;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t comboA = dataA->m_textureCombos[batchA->m_textureComboIndex + i];
        uint32_t comboB = dataB->m_textureCombos[batchB->m_textureComboIndex + i];
        CTexture* textureA = comboA >= dataA->m_textures.m_count ? nullptr : a->m_model->m_textures[comboA];
        CTexture* textureB = comboB >= dataB->m_textures.m_count ? nullptr : b->m_model->m_textures[comboB];
        intptr_t result = (reinterpret_cast<intptr_t>(textureA) - reinterpret_cast<intptr_t>(textureB)) >> 2;

        if (result < 0) {
            return -1;
        }

        if (result > 0) {
            return 1;
        }
    }

    if (batchA->m_textureCount < batchB->m_textureCount) {
        return -1;
    }

    if (batchA->m_textureCount > batchB->m_textureCount) {
        return 1;
    }

    if (batchB > batchA) {
        return -1;
    }

    return batchB < batchA;
}

// VERIFY: 3f02cc62 manual
// OFFSET: 0x0081ED10
int32_t CM2Scene::SortOpaqueRibbons(M2Element* a, M2Element* b) {
    M2Ribbon* ribbonA = &a->m_model->m_shared->m_data->m_ribbons[a->m_index];
    M2Ribbon* ribbonB = &b->m_model->m_shared->m_data->m_ribbons[b->m_index];
    uint32_t count = ribbonA->m_textureIndices.m_count < ribbonB->m_textureIndices.m_count ? ribbonA->m_textureIndices.m_count : ribbonB->m_textureIndices.m_count;

    for (uint32_t i = 0; i < count; i++) {
        CTexture* textureA = a->m_model->m_textures[ribbonA->m_textureIndices[i]];
        CTexture* textureB = b->m_model->m_textures[ribbonB->m_textureIndices[i]];
        intptr_t result = (reinterpret_cast<intptr_t>(textureA) - reinterpret_cast<intptr_t>(textureB)) >> 2;

        if (result < 0) {
            return -1;
        }

        if (result > 0) {
            return 1;
        }
    }

    if (ribbonA->m_textureIndices.m_count < ribbonB->m_textureIndices.m_count) {
        return -1;
    }

    if (ribbonA->m_textureIndices.m_count > ribbonB->m_textureIndices.m_count) {
        return 1;
    }

    if (static_cast<uint32_t>(b->m_index) > static_cast<uint32_t>(a->m_index)) {
        return -1;
    }

    return static_cast<uint32_t>(b->m_index) < static_cast<uint32_t>(a->m_index);
}

// VERIFY: b3646c2c manual
// OFFSET: 0x0081EDF0
int32_t CM2Scene::SortOpaqueParticles(M2Element* a, M2Element* b) {
    CParticleEmitter2* emitterA = a->m_emitter;
    CParticleEmitter2* emitterB = b->m_emitter;
    int32_t blendA = emitterA->m_params.m_materialBlend;
    int32_t blendB = emitterB->m_params.m_materialBlend;

    if (blendA < blendB) {
        return -1;
    }

    if (blendA > blendB) {
        return 1;
    }

    uint32_t flagsA = ParticleFlagsToM2Flags(emitterA->m_params.m_materialFlags);
    uint32_t flagsB = ParticleFlagsToM2Flags(emitterB->m_params.m_materialFlags);

    if (flagsA < flagsB) {
        return -1;
    }

    if (flagsA > flagsB) {
        return 1;
    }

    return static_cast<int32_t>((reinterpret_cast<intptr_t>(emitterA->m_texture) - reinterpret_cast<intptr_t>(emitterB->m_texture)) >> 2);
}

// VERIFY: 23fd3c71 auto
// OFFSET: 0x0081EEA0
int32_t CM2Scene::SortOpaque(uint32_t a, uint32_t b, const void* param) {
    const CM2Scene* scene = static_cast<const CM2Scene*>(param);
    M2Element* elementA = &scene->m_elements.m_data[a];
    M2Element* elementB = &scene->m_elements.m_data[b];

    if (elementA->m_type < elementB->m_type) {
        return -1;
    }

    if (elementA->m_type > elementB->m_type) {
        return 1;
    }

    switch (elementA->m_type) {
        case 0:
        case 1:
            return CM2Scene::SortOpaqueGeoBatches(elementA, elementB);
        case 3:
            return CM2Scene::SortOpaqueRibbons(elementA, elementB);
        case 4:
            return CM2Scene::SortOpaqueParticles(elementA, elementB);
        default:
            return 0;
    }
}

// VERIFY: 962fda53 manual
// OFFSET: 0x0081EF30
int32_t CM2Scene::SortTransparent(uint32_t a, uint32_t b, const void* param) {
    const CM2Scene* scene = static_cast<const CM2Scene*>(param);
    M2Element* elementA = &scene->m_elements.m_data[a];
    M2Element* elementB = &scene->m_elements.m_data[b];

    if (elementB->m_sortDepth < elementA->m_sortDepth) {
        return -1;
    }

    if (elementB->m_sortDepth > elementA->m_sortDepth) {
        return 1;
    }

    uint32_t splitB = elementB->m_flags & 1;
    uint32_t splitA = elementA->m_flags & 1;

    if (splitA > splitB) {
        return -1;
    }

    if (splitA < splitB) {
        return 1;
    }

    if (elementA->m_priorityPlane < elementB->m_priorityPlane) {
        return -1;
    }

    if (elementA->m_priorityPlane > elementB->m_priorityPlane) {
        return 1;
    }

    if (elementB->m_depth < elementA->m_depth) {
        return -1;
    }

    if (elementB->m_depth > elementA->m_depth) {
        return 1;
    }

    uint32_t shaderSort = CM2Scene::s_optFlags & 0x4000;

    if (shaderSort && (elementA->m_type != elementB->m_type || elementA->m_model != elementB->m_model) && elementA->m_effect && elementB->m_effect) {
        CGxShader* vertexA = elementA->m_effect->m_vertexShaders[elementA->m_vertexPermute];
        CGxShader* vertexB = elementB->m_effect->m_vertexShaders[elementB->m_vertexPermute];
        CGxShader* pixelA = elementA->m_effect->m_pixelShaders[elementA->m_pixelPermute];
        CGxShader* pixelB = elementB->m_effect->m_pixelShaders[elementB->m_pixelPermute];

        if (vertexA < vertexB) {
            return -1;
        }

        if (vertexA > vertexB) {
            return 1;
        }

        if (pixelA < pixelB) {
            return -1;
        }

        if (pixelA > pixelB) {
            return 1;
        }

        shaderSort = CM2Scene::s_optFlags & 0x4000;
    }

    if (elementA->m_model < elementB->m_model) {
        return -1;
    }

    if (elementA->m_model > elementB->m_model) {
        return 1;
    }

    if (elementA->m_type < elementB->m_type) {
        return -1;
    }

    if (elementA->m_type > elementB->m_type) {
        return 1;
    }

    if (elementA->m_type <= 2) {
        if (elementA->m_batch->m_materialLayer < elementB->m_batch->m_materialLayer) {
            return -1;
        }

        if (elementA->m_batch->m_materialLayer > elementB->m_batch->m_materialLayer) {
            return 1;
        }
    }

    if (!shaderSort || !elementA->m_effect || !elementB->m_effect) {
        return CM2Scene::SortOpaque(a, b, param);
    }

    CGxShader* vertexA = elementA->m_effect->m_vertexShaders[elementA->m_vertexPermute];
    CGxShader* vertexB = elementB->m_effect->m_vertexShaders[elementB->m_vertexPermute];
    CGxShader* pixelA = elementA->m_effect->m_pixelShaders[elementA->m_pixelPermute];
    CGxShader* pixelB = elementB->m_effect->m_pixelShaders[elementB->m_pixelPermute];

    if (vertexA < vertexB) {
        return -1;
    }

    if (vertexA > vertexB) {
        return 1;
    }

    if (pixelA < pixelB) {
        return -1;
    }

    if (pixelA > pixelB) {
        return 1;
    }

    return CM2Scene::SortOpaque(a, b, param);
}

// VERIFY: 706189e1 manual
// OFFSET: 0x0081F0E0
int32_t CM2Scene::SortAdditiveParticles(uint32_t a, uint32_t b, const void* param) {
    const CM2Scene* scene = static_cast<const CM2Scene*>(param);
    M2Element* elementA = &scene->m_elements.m_data[a];
    M2Element* elementB = &scene->m_elements.m_data[b];

    if (elementA->m_sortKey > elementB->m_sortKey) {
        return 1;
    }

    if (elementA->m_sortKey < elementB->m_sortKey) {
        return -1;
    }

    if (elementA->m_type == 4 || elementB->m_type == 4) {
        if (elementA->m_type > elementB->m_type) {
            return -1;
        }

        if (elementA->m_type < elementB->m_type) {
            return 1;
        }
    }

    if (elementA->m_type != 4) {
        return CM2Scene::SortTransparent(a, b, param);
    }

    CParticleEmitter2* emitterA = elementA->m_emitter;
    CParticleEmitter2* emitterB = elementB->m_emitter;
    int32_t blendA = emitterA->m_params.m_materialBlend;
    int32_t blendB = emitterB->m_params.m_materialBlend;

    if (blendA < blendB) {
        return -1;
    }

    if (blendA > blendB) {
        return 1;
    }

    uint32_t flagsA = ParticleFlagsToM2Flags(emitterA->m_params.m_materialFlags);
    uint32_t flagsB = ParticleFlagsToM2Flags(emitterB->m_params.m_materialFlags);

    if (flagsA < flagsB) {
        return -1;
    }

    if (flagsA > flagsB) {
        return 1;
    }

    return static_cast<int32_t>((reinterpret_cast<intptr_t>(emitterA->m_texture) - reinterpret_cast<intptr_t>(emitterB->m_texture)) >> 2);
}

// VERIFY: 44a0c3d0 manual
// OFFSET: 0x0081F1D0
void __stdcall CM2Scene::ComputeElementShaders(M2Element* element) {
    M2Material* material = &element->m_model->m_shared->m_data->m_materials[element->m_batch->m_materialIndex];
    CM2Lighting* lighting = element->m_model->m_currentLighting;
    uint32_t specular;
    uint32_t lightCount;
    uint32_t shadow;

    if ((material->m_flags & 1) || s_localLighting[material->m_blendMode] == (material->m_flags & 1u)) {
        shadow = 0;
        specular = 0;

        if (material->m_flags & 1) {
            lightCount = 0;
        } else {
            lightCount = lighting->m_lightCount;
        }
    } else {
        specular = 1;
        lightCount = lighting->m_lightCount;
        shadow = 0;
    }

    uint32_t boneInfluences = element->m_skinSection->m_boneInfluences;
    int32_t alphaTest;

    if (material->m_blendMode == M2BLEND_OPAQUE) {
        alphaTest = 0;
    } else if (material->m_blendMode == M2BLEND_ALPHA_KEY) {
        alphaTest = static_cast<int32_t>(element->m_alpha * 224.0f);
    } else {
        alphaTest = 1;
    }

    if (!(material->m_flags & 1) && (lighting->m_flags & 0x10)) {
        if (material->m_flags & 0x100) {
            shadow = 0;
        } else {
            shadow = CShadowCache::GetShadowValue();

            if (shadow) {
                if (lighting->m_flags & 8) {
                    shadow = 1;
                }

                if (element->m_type == 1) {
                    shadow = 0;
                }
            }
        }
    }

    int32_t useAlphaTest = alphaTest && (!g_theGxDevicePtr->Caps().m_shaderAlphaRef || shadow);
    uint32_t shadowIndex = shadow > 2 ? 2 : shadow;
    uint32_t boneIndex = boneInfluences > 2 ? 2 : boneInfluences;

    element->m_vertexPermute = specular + 2 * (shadowIndex + boneIndex + 2 * shadowIndex + lightCount + 4 * (shadowIndex + boneIndex + 2 * shadowIndex));
    element->m_shadowValue = shadow;
    element->m_pixelPermute = shadow + 4 * (CShaderEffect::s_usePcfFiltering + 2 * useAlphaTest);
}

// VERIFY: 0712a448 auto
// OFFSET: 0x0081F8C0
void CM2ModelHeap::Delete(CM2Model* model) {
    model->~CM2Model();
    ObjectFree(this->m_heapId, model->m_handle);
}

// VERIFY: 6625d3c0 auto
// OFFSET: 0x0081F8F0
CM2Model* CM2Scene::CreateModel(const char* fileName, uint32_t flags) {
    if (!fileName) {
        return nullptr;
    }

    CM2Shared* shared = this->m_cache->CreateShared(fileName, flags);

    if (!shared) {
        shared = this->m_cache->CreateShared("Spells\\ErrorCube.mdx", 0);
    }

    CM2Model* model = nullptr;

    if (shared) {
        model = g_modelPool->New();

        if (model && !model->Initialize(this, shared, nullptr, flags)) {
            g_modelPool->Delete(model);
            model = nullptr;
        }

        shared->Release();
    }

    return model;
}

// VERIFY: 5dcce774 auto
// OFFSET: 0x0081F970
CM2Model* CM2Scene::DuplicateModel(CM2Model* source, uint32_t flags) {
    if (!source) {
        return nullptr;
    }

    CM2Model* model = g_modelPool->New();

    if (model && !model->Initialize(this, source->m_shared, source, flags)) {
        CM2ModelHeap* heap = g_modelPool;
        model->~CM2Model();
        ObjectFree(heap->m_heapId, model->m_handle);
        return nullptr;
    }

    return model;
}

// VERIFY: 4c9b1c1c manual
// OFFSET: 0x0081F9E0
void CM2Scene::SortAdditiveParticleElements(uint32_t pass) {
    TSGrowableArray<uint32_t>& list = this->m_passElements[pass];
    uint32_t group = 0;
    int32_t prevAdditive = 0;

    for (uint32_t i = 0; i < list.m_count; i++) {
        M2Element* element = &this->m_elements.m_data[list.m_data[i]];
        M2Data* data = element->m_model->m_shared->m_data;
        uint32_t blend = M2BLEND_OPAQUE;

        switch (element->m_type) {
            case 0:
            case 1:
            case 2:
                blend = data->m_materials[element->m_batch->m_materialIndex].m_blendMode;
                break;
            case 3:
                blend = data->m_materials[data->m_ribbons[element->m_index].m_materialIndices[0]].m_blendMode;
                break;
            case 4:
                blend = (this->m_cache->m_flags & 0x100) ? M2BLEND_ADD : ParticleBlendToM2Blend(element->m_emitter->m_params.m_materialBlend);
                break;
            default:
                break;
        }

        uint32_t gxBlend = s_blendModes[1][blend];
        int32_t additive = gxBlend == 3 || gxBlend == 10;

        if (!additive || !prevAdditive) {
            group++;
        }

        prevAdditive = additive;
        element->m_sortKey = group;
    }

    M2HeapSort(&CM2Scene::SortAdditiveParticles, list.m_data, list.m_count, this);
}

// VERIFY: 3e6be15c manual
// OFFSET: 0x008216C0
CM2Scene::CM2Scene(CM2Cache* cache) {
    this->m_float18 = 0.0f;
    this->m_refCount = 1;
    this->m_cache = cache;
    this->m_modelList = nullptr;
    this->m_time = 0;
    this->m_frameStamp = 0;
    this->m_flags = 0;
    this->m_lightList = nullptr;
    this->m_lightGrid = nullptr;
    this->m_animateList = nullptr;
    this->m_drawList = nullptr;
    this->m_particleList = nullptr;
    this->m_projectTextureCallback = nullptr;
    this->m_projectPositionCallback = nullptr;
    this->m_hitTestList = nullptr;
    this->m_hitRecs = nullptr;
    this->m_hitOrder = nullptr;
    this->m_hitCapacity = 0;
    this->m_hitVerts = nullptr;
    this->m_hitVertCapacity = 0;
    this->m_liquidTypeId = 0;
    CM2Scene::s_optFlags = 0;
    this->m_passMask = 0xFFFFFFFF;
}

// VERIFY: aa195899 manual
// OFFSET: 0x00821850
CM2Scene::~CM2Scene() {
    for (CM2Model* model = this->m_modelList; model; model = this->m_modelList) {
        while (model->Release()) {
        }
    }

    delete[] this->m_lightGrid;
    delete[] this->m_hitRecs;
    delete[] this->m_hitOrder;
    delete[] this->m_hitVerts;
}

// VERIFY: 7bb79c57 manual
// OFFSET: 0x00821930
M2Element* CM2Scene::AddParticleElement(CParticleEmitter2* emitter, CM2Model* model, float depth, float alpha, int32_t split, uint32_t& index, uint32_t& additiveCount) {
    if (!emitter->HasLiveParticles() || emitter->m_params.m_hasModel == CParticleEmitter2::PT_MODEL) {
        return nullptr;
    }

    M2Element* element = this->m_elements.New();

    if (!element) {
        return nullptr;
    }

    element->m_alpha = alpha;
    element->m_model = model;
    element->m_type = 4;
    element->m_flags = 0;
    element->m_emitter = emitter;
    element->m_priorityPlane = emitter->m_priorityPlane;
    element->m_sortDepth = model->m_scaleSq;
    element->m_effect = nullptr;
    element->m_depth = depth;
    element->m_vertexPermute = 0xFFFFFFFF;
    element->m_pixelPermute = 0xFFFFFFFF;
    element->m_shadowValue = 0;

    int32_t blend = emitter->m_params.m_materialBlend;

    if (blend == 10 || blend == 3) {
        additiveCount++;
    }

    if (blend > 1 || alpha < 0.99998999f) {
        TSGrowableArray<uint32_t>* list = (!split || (emitter->m_flags & 0x40000)) ? &this->m_passElements[2] : &this->m_passElements[1];
        list->New(index);
    } else {
        this->m_passElements[0].New(index);
    }

    index++;

    return element;
}

// VERIFY: d0feeb6c manual
// OFFSET: 0x00821A20
int32_t CM2Scene::Animate(const C3Vector& cameraPos) {
    this->m_frameStamp++;

    uint32_t optFlags = this->m_cache->m_flags & 0xE000;

    if (CM2Scene::s_optFlags != optFlags) {
        CM2Scene::s_optFlags = optFlags;
    }

    CGxMatrixStack& viewStack = g_theGxDevicePtr->m_xforms[GxXform_View];
    this->m_view = viewStack.m_mtx[viewStack.m_level];
    C3Vector move(-cameraPos.x, -cameraPos.y, -cameraPos.z);
    this->m_view.Translate(move);
    this->m_viewInv = this->m_view.Inverse(this->m_view.Determinant());

    if (this->m_cache->m_flags & 4) {
        this->m_cache->BeginThread(&CM2Scene::AnimateThread, this);

        for (CM2Model* model = this->m_animateList; model; ) {
            if (!model->m_attachParent) {
                if (model->m_fFlags & 0x1000) {
                    model->AnimateMTSimple(&this->m_view, C3Vector(1.0f, 1.0f, 1.0f), C3Vector(0.0f, 0.0f, 0.0f), 1.0f, 1.0f);
                } else {
                    model->AnimateMT(&this->m_view, C3Vector(1.0f, 1.0f, 1.0f), C3Vector(0.0f, 0.0f, 0.0f), 1.0f, 1.0f);
                }
            }

            CM2Model* next = model->m_animateNext;

            if (!next) {
                break;
            }

            model = next->m_animateNext;
        }

        this->m_cache->WaitThread();
    } else {
        for (CM2Model* model = this->m_animateList; model; model = model->m_animateNext) {
            if (model->m_attachParent) {
                continue;
            }

            if (model->m_fFlags & 0x1000) {
                model->AnimateMTSimple(&this->m_view, C3Vector(1.0f, 1.0f, 1.0f), C3Vector(0.0f, 0.0f, 0.0f), 1.0f, 1.0f);
            } else {
                model->AnimateMT(&this->m_view, C3Vector(1.0f, 1.0f, 1.0f), C3Vector(0.0f, 0.0f, 0.0f), 1.0f, 1.0f);
            }
        }
    }

    for (CM2Model* model = this->m_animateList; model; model = model->m_animateNext) {
        if (!model->m_attachParent) {
            model->AnimateST();
        }
    }

    while (this->m_animateList) {
        CM2Model* model = this->m_animateList;
        this->m_animateList = model->m_animateNext;
        model->m_animatePrev = nullptr;
        model->m_animateNext = nullptr;
        model->SetupLighting();
    }

    this->m_doodadElements.m_count = 0;

    for (uint32_t i = 0; i < 3; i++) {
        this->m_passElements[i].m_count = 0;
    }

    this->m_elements.m_count = 0;

    uint32_t elementIndex = 0;

    while (this->m_drawList) {
        CM2Model* model = this->m_drawList;
        this->m_drawList = model->m_drawNext;
        model->m_fFlags &= ~0x10008u;
        model->m_drawPrev = nullptr;
        model->m_drawNext = nullptr;

        if (!model->IsDrawable(0, 0) || (model->m_fFlags & 0x4000)) {
            continue;
        }

        CM2Lighting* lighting = model->m_currentLighting;
        M2Data* data = model->m_shared->m_data;
        int32_t above = lighting->m_flags & 0x20;
        int32_t below = lighting->m_flags & 0x40;

        if (above && below) {
            C3Vector center((data->m_bounds.m_extent.b.x + data->m_bounds.m_extent.t.x) * 0.5f, (data->m_bounds.m_extent.t.y + data->m_bounds.m_extent.b.y) * 0.5f, 0.5f * (data->m_bounds.m_extent.t.z + data->m_bounds.m_extent.b.z));
            const C44Matrix& matrix = model->m_matrixF4;
            float radius = sqrtf(matrix.a2 * matrix.a2 + matrix.a1 * matrix.a1 + matrix.a0 * matrix.a0) * data->m_bounds.m_radius;
            C3Vector position = center * matrix;
            const C4Plane& plane = lighting->m_liquidPlane;
            float dist = plane.n.y * position.y + plane.n.z * position.z + position.x * plane.n.x + plane.d;

            above = 1;

            if (-radius > dist) {
                above = 0;
            }

            below = radius >= dist;

            if (!(this->m_cache->m_flags & 2) && above && radius >= dist) {
                below = this->m_liquidTypeId != 0;
                above = this->m_liquidTypeId == 0;
            }
        }

        M2ModelOptGeo* optGeo = model->m_optGeo;
        M2SkinProfile* skinData = model->m_shared->m_skinData;
        int32_t optimized = optGeo != nullptr;
        int32_t depthSort = (this->m_cache->m_flags & 1) && !(model->m_flags & 1) && (model->m_fFlags & 0x40);
        uint32_t batchCount = optGeo ? optGeo->m_batchCount : skinData->m_batches.m_count;

        for (uint32_t i = 0; i < batchCount; i++) {
            M2Batch* batch;
            M2SkinSection* section;

            if (optimized) {
                batch = &optGeo->m_batches[i];
                section = &optGeo->m_skinSections[batch->m_skinSectionIndex];
            } else {
                batch = &skinData->m_batches[i];
                section = &model->m_shared->m_skinSections[batch->m_skinSectionIndex];

                if (!model->m_skinSections[batch->m_skinSectionIndex]) {
                    continue;
                }
            }

            if (batch->m_shader == 0x8000) {
                continue;
            }

            float alpha = model->m_alpha;

            if (batch->m_colorIndex < data->m_colors.m_count) {
                alpha *= model->m_colors[batch->m_colorIndex].m_alphaTrack.m_currentValue;
            }

            if (batch->m_textureCount) {
                alpha *= model->m_textureWeights[data->m_textureWeightCombos[batch->m_textureWeightComboIndex]].m_currentValue;
            }

            if (alpha < 0.000099999997f) {
                continue;
            }

            M2Material* material = &data->m_materials[batch->m_materialIndex];
            int32_t projected = (batch->m_flags & 4) && this->m_projectTextureCallback;
            M2Material* baseMaterial = batch->m_materialLayer ? &data->m_materials[batch->m_materialIndex - batch->m_materialLayer] : material;
            int32_t transparent = baseMaterial->m_blendMode > M2BLEND_ALPHA_KEY || alpha < 0.99998999f;
            CShaderEffect* effect = optimized ? model->m_optGeo->m_effects[i] : model->m_shared->m_effects[i];

            if (!effect) {
                continue;
            }

            M2Element* element = this->m_elements.New();

            if (!element) {
                return 0;
            }

            if (projected) {
                element->m_type = 1;
            } else if (!model->IsBatchDoodadCompatible(batch) || transparent) {
                element->m_type = 0;
            } else {
                element->m_type = 2;
            }

            element->m_model = model;
            element->m_flags = 0;

            if (transparent == 1 && above && below && !projected) {
                element->m_flags = 2;
            }

            if (optimized) {
                element->m_flags |= 4;
            }

            element->m_alpha = alpha;
            element->m_index = i;
            element->m_priorityPlane = batch->m_priorityPlane;
            element->m_batch = batch;
            element->m_skinSection = section;
            element->m_effect = effect;
            CM2Scene::ComputeElementShaders(element);

            float sortDepth;

            if (transparent < 1) {
                element->m_depth = model->m_scaleSq;
                sortDepth = model->m_scaleSq;
            } else if (data->m_flags & 0x10) {
                C3Vector position = section->m_sortCenterPosition * model->m_boneMatrices[section->m_centerBoneIndex];
                element->m_depth = position.z * position.z + position.y * position.y + position.x * position.x;
                sortDepth = model->m_scaleSq;
            } else {
                const C44Matrix& bone = model->m_boneMatrices[section->m_centerBoneIndex];
                float depth;

                if (batch->m_flags & 3) {
                    C3Vector position = section->m_sortCenterPosition * bone;
                    C3Vector normal = position;
                    float lengthSq = position.x * position.x + position.z * position.z + position.y * position.y;

                    if (lengthSq > 0.00000023841858f) {
                        float inv = 1.0f / sqrtf(lengthSq);
                        normal.x = position.x * inv;
                        normal.y = inv * position.y;
                        normal.z = inv * position.z;
                    }

                    float radius = sqrtf(bone.a2 * bone.a2 + bone.a1 * bone.a1 + bone.a0 * bone.a0) * section->m_sortRadius;

                    if (batch->m_flags & 1) {
                        position.x = position.x - normal.x * radius;
                        position.y = position.y - normal.y * radius;
                        position.z = position.z - normal.z * radius;
                    } else {
                        position.x = normal.x * radius + position.x;
                        position.y = normal.y * radius + position.y;
                        position.z = normal.z * radius + position.z;
                    }

                    depth = position.y * position.y + position.x * position.x + position.z * position.z;

                    if (position.z < 0.0f) {
                        depth = -depth;
                    }
                } else {
                    C3Vector position = section->m_sortCenterPosition * bone;
                    depth = position.z * position.z + position.y * position.y + position.x * position.x;
                }

                element->m_depth = depth;

                if (!depthSort || projected || (material->m_flags & 0x10)) {
                    sortDepth = element->m_depth;
                } else {
                    sortDepth = model->m_scaleSq;
                }
            }

            element->m_sortDepth = sortDepth;

            if (element->m_type == 2) {
                this->m_doodadElements.New(elementIndex);
            } else if (transparent == 1) {
                if (projected) {
                    if (!below) {
                        this->m_passElements[1].New(elementIndex);
                    } else {
                        this->m_passElements[2].New(elementIndex);
                    }
                } else {
                    if (above) {
                        this->m_passElements[1].New(elementIndex);
                    }

                    if (below) {
                        this->m_passElements[2].New(elementIndex);
                    }
                }
            } else {
                this->m_passElements[transparent].New(elementIndex);
            }

            elementIndex++;

            if (depthSort && !projected && transparent >= 1 && !(material->m_flags & 0x10)) {
                element->m_depth = FLT_MAX;

                M2Element* copy = this->m_elements.New();

                if (!copy) {
                    return 0;
                }

                *copy = this->m_elements.m_data[elementIndex - 1];
                copy->m_flags |= 1;

                if (above) {
                    this->m_passElements[1].New(elementIndex);
                }

                if (below) {
                    this->m_passElements[2].New(elementIndex);
                }

                elementIndex++;
            }
        }

        for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
            if (model->m_ribbonEmitters[i]->IsDead()) {
                continue;
            }

            float alpha = model->m_effectiveAlpha;
            M2Ribbon* ribbon = &data->m_ribbons[i];

            if (ribbon->m_alphaTrack.m_sequenceTimes.m_count) {
                alpha *= model->m_ribbons[i].m_alphaTrack.m_currentValue;
            }

            M2Material* material = &data->m_materials[ribbon->m_materialIndices[0]];
            M2Element* element = this->m_elements.New();

            if (!element) {
                continue;
            }

            element->m_alpha = alpha;
            element->m_index = i;
            element->m_type = 3;
            element->m_model = model;
            element->m_flags = 0;
            element->m_priorityPlane = ribbon->m_priorityPlane;
            element->m_sortDepth = model->m_scaleSq;
            element->m_effect = nullptr;
            element->m_depth = model->m_scaleSq;
            element->m_vertexPermute = 0xFFFFFFFF;
            element->m_pixelPermute = 0xFFFFFFFF;
            element->m_shadowValue = 0;

            if (material->m_blendMode > M2BLEND_ALPHA_KEY || alpha < 0.99998999f) {
                if (above) {
                    this->m_passElements[1].New(elementIndex);
                } else {
                    this->m_passElements[2].New(elementIndex);
                }
            } else {
                this->m_passElements[0].New(elementIndex);
            }

            elementIndex++;
        }

        if (model->m_drawCallback) {
            M2Element* element = this->m_elements.New();

            if (element) {
                element->m_type = 5;
                element->m_alpha = 1.0f;
                element->m_model = model;
                element->m_flags = 0;
                element->m_index = 0;
                element->m_priorityPlane = 0;
                element->m_sortDepth = model->m_scaleSq;
                element->m_effect = nullptr;
                element->m_depth = model->m_scaleSq;
                element->m_vertexPermute = 0xFFFFFFFF;
                element->m_pixelPermute = 0xFFFFFFFF;
                element->m_shadowValue = 0;

                if (model->m_fFlags & 0x20) {
                    this->m_passElements[0].New(elementIndex);
                } else if (above) {
                    this->m_passElements[1].New(elementIndex);
                } else {
                    this->m_passElements[2].New(elementIndex);
                }

                elementIndex++;
            }
        }
    }

    uint32_t additiveCount = 0;

    while (this->m_particleList) {
        CM2Model* model = this->m_particleList;
        this->m_particleList = model->m_particleNext;
        model->m_particlePrev = nullptr;
        model->m_particleNext = nullptr;

        if (!model->IsDrawable(0, 0)) {
            continue;
        }

        CM2Lighting* lighting = model->m_currentLighting;
        M2Data* data = model->m_shared->m_data;
        int32_t split = lighting->m_flags & 0x20;

        if ((lighting->m_flags & 0x20) && (lighting->m_flags & 0x40)) {
            C3Vector center((data->m_bounds.m_extent.t.x + data->m_bounds.m_extent.b.x) * 0.5f, (data->m_bounds.m_extent.t.y + data->m_bounds.m_extent.b.y) * 0.5f, 0.5f * (data->m_bounds.m_extent.t.z + data->m_bounds.m_extent.b.z));
            const C44Matrix& matrix = model->m_matrixF4;
            float radius = sqrtf(matrix.a2 * matrix.a2 + matrix.a1 * matrix.a1 + matrix.a0 * matrix.a0) * data->m_bounds.m_radius;
            C3Vector position = center * matrix;
            const C4Plane& plane = lighting->m_liquidPlane;
            split = -radius <= plane.n.y * position.y + plane.n.z * position.z + plane.n.x * position.x + plane.d;
        }

        for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
            CParticleEmitter2* emitter = model->m_particleEmitters[i];

            if ((model->m_fFlags & 0x2000) && (emitter->m_flags & 0x200)) {
                continue;
            }

            if (emitter->m_flags & 0x2000000) {
                continue;
            }

            if (!model->m_particles[i].m_active) {
                continue;
            }

            float alpha = model->m_effectiveAlpha;

            if (alpha < 0.000099999997f) {
                continue;
            }

            M2Particle* particle = &data->m_particles[i];
            C3Vector position = particle->m_position * model->m_boneMatrices[particle->m_boneIndex];
            float depth = position.z * position.z + position.y * position.y + position.x * position.x;

            this->AddParticleElement(emitter, model, depth, alpha, split, elementIndex, additiveCount);

            for (uint32_t j = 0; j < emitter->m_childEmitterCount; j++) {
                this->AddParticleElement(emitter->m_childEmitters[j], model, depth, alpha, split, elementIndex, additiveCount);
            }
        }
    }

    uint32_t doodadCount = this->m_doodadElements.m_count;

    if (doodadCount > 1) {
        memset(s_doodadHash, 0xFF, sizeof(s_doodadHash));

        for (uint32_t i = 0; i < doodadCount; i++) {
            uint32_t index = this->m_doodadElements.m_data[i];
            M2Element* element = &this->m_elements.m_data[index];
            uint32_t start = HashDoodadElement(element) % (sizeof(s_doodadHash) / sizeof(s_doodadHash[0]));
            uint32_t slot = start;

            while (true) {
                if (++slot >= sizeof(s_doodadHash) / sizeof(s_doodadHash[0])) {
                    slot = 0;
                }

                uint32_t other = s_doodadHash[slot];

                if (other == 0xFFFFFFFF || slot == start) {
                    s_doodadHash[slot] = index;
                    break;
                }

                if (!CM2Scene::SortDoodad(index, other, this)) {
                    break;
                }
            }

            this->m_elements.m_data[index].m_doodadKey = s_doodadHash[slot];
        }
    }

    M2HeapSort(&CM2Scene::SortDoodadProxy, this->m_doodadElements.m_data, doodadCount, this);

    uint32_t out = 0;

    for (uint32_t i = 0; i < doodadCount; ) {
        uint32_t index = this->m_doodadElements.m_data[i];
        this->m_doodadElements.m_data[out++] = index;

        uint32_t j = i + 1;

        if (j < doodadCount) {
            uint32_t key = this->m_elements.m_data[index].m_doodadKey;

            do {
                uint32_t other = this->m_doodadElements.m_data[j];

                if (key != this->m_elements.m_data[other].m_doodadKey) {
                    break;
                }

                this->m_doodadElements.m_data[out++] = other;
                j++;
            } while (j < doodadCount);
        }

        if (j - i <= 1) {
            this->m_elements.m_data[index].m_type = 0;
            CM2Scene::ComputeElementShaders(&this->m_elements.m_data[index]);
            this->m_passElements[0].New(index);
            out--;
            i++;
        } else {
            this->m_elements.m_data[index].m_count = j - i;
            i = j;
        }
    }

    this->m_doodadElements.SetCount(out);

    M2HeapSort(&CM2Scene::SortOpaque, this->m_passElements[0].m_data, this->m_passElements[0].m_count, this);
    M2HeapSort(&CM2Scene::SortTransparent, this->m_passElements[1].m_data, this->m_passElements[1].m_count, this);
    M2HeapSort(&CM2Scene::SortTransparent, this->m_passElements[2].m_data, this->m_passElements[2].m_count, this);

    if ((this->m_cache->m_flags & 0x80) && additiveCount > 1) {
        this->SortAdditiveParticleElements(1);
        this->SortAdditiveParticleElements(2);
    }

    return 1;
}

// VERIFY: 651a4228 auto
// OFFSET: 0x00823040
void CM2Scene::Release() {
    if (this->m_refCount-- == 1) {
        delete this;
    }
}

// VERIFY: 610b7e45 manual
// OFFSET: 0x00823CB0
void CM2Scene::Draw(M2PASS pass) {
    if (!((1 << pass) & this->m_passMask)) {
        return;
    }

    DebugLog("Model2 Pass %d", pass);

    uint32_t optFlags = this->m_cache->m_flags & 0xE000;

    if (CM2Scene::s_optFlags != optFlags) {
        CM2Scene::s_optFlags = optFlags;
    }

    CM2SceneRender render(this);
    render.Draw(pass, this->m_elements.m_data, this->m_passElements[pass].m_data, this->m_passElements[pass].m_count);

    if (pass == M2PASS_0) {
        render.Draw(M2PASS_0, this->m_elements.m_data, this->m_doodadElements.m_data, this->m_doodadElements.m_count);
    }
}
