#ifndef ENGINE_SOURCE_MODEL2_M2SCENE_H
#define ENGINE_SOURCE_MODEL2_M2SCENE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/Types.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Storm/H/stpl.h"

class C3Vector;
class CAaBox;
class CImVector;
class CM2Cache;
class CM2Light;
class CM2Lighting;
class CM2Model;
class CParticleEmitter2;
class CShaderEffect;

class M2HitRec {
    public:
    /* 0000 */ CM2Model* m_model;
    /* 0004 */ float m_tNear;
    /* 0008 */ float m_tFar;
    /* 000C */ uint32_t m_priority;
};

class M2Element {
    public:
    /* 0000 */ int32_t m_type;
    /* 0004 */ CM2Model* m_model;
    /* 0008 */ uint32_t m_flags;
    /* 000C */ float m_alpha;
    /* 0010 */ float m_sortDepth;
    /* 0014 */ float m_depth;
    union {
        /* 0018 */ int32_t m_index;
        /* 0018 */ CParticleEmitter2* m_emitter;
    };
    /* 001C */ uint32_t m_count;
    /* 0020 */ uint32_t m_doodadKey;
    /* 0024 */ int32_t m_priorityPlane;
    /* 0028 */ M2Batch* m_batch;
    /* 002C */ M2SkinSection* m_skinSection;
    /* 0030 */ CShaderEffect* m_effect;
    /* 0034 */ uint32_t m_vertexPermute;
    /* 0038 */ uint32_t m_pixelPermute;
    /* 003C */ uint32_t m_shadowValue;
    /* 0040 */ uint32_t m_sortKey;
};

class CM2Scene {
    public:
    /* 0000 */ int32_t m_refCount;
    /* 0004 */ CM2Cache* m_cache;
    /* 0008 */ CM2Model* m_modelList;
    /* 000C */ uint32_t m_time;
    /* 0010 */ uint32_t m_timeDelta;
    /* 0014 */ uint32_t m_frameStamp;
    /* 0018 */ float m_float18;
    /* 001C */ uint32_t m_flags;
    /* 0020 */ CM2Light* m_lightList;
    /* 0024 */ CM2Light** m_lightGrid;
    /* 0028 */ CM2Model* m_animateList;
    /* 002C */ CM2Model* m_drawList;
    /* 0030 */ CM2Model* m_particleList;
    /* 0034 */ TSGrowableArray<M2Element> m_elements;
    /* 0044 */ TSGrowableArray<uint32_t> m_doodadElements;
    /* 0054 */ TSGrowableArray<uint32_t> m_passElements [3];
    /* 0084 */ C44Matrix m_view;
    /* 00C4 */ C44Matrix m_viewInv;
    /* 0104 */ void (*m_projectTextureCallback)(const CAaBox&, const CImVector&, int32_t, void*, int32_t);
    /* 0108 */ void* m_projectTextureParam;
    /* 010C */ int32_t (*m_projectPositionCallback)(const C3Vector&, float&, void*);
    /* 0110 */ void* m_projectPositionParam;
    /* 0114 */ CM2Model* m_hitTestList;
    /* 0118 */ M2HitRec* m_hitRecs;
    /* 011C */ uint32_t* m_hitOrder;
    /* 0120 */ uint32_t m_hitCapacity;
    /* 0124 */ C3Vector* m_hitVerts;
    /* 0128 */ uint32_t m_hitVertCapacity;
    /* 012C */ M2HitRec m_lastHit;
    /* 013C */ void* m_lastHitOwner;
    /* 0140 */ uint32_t m_liquidTypeId;
    /* 0144 */ uint32_t m_passMask;

    static uint32_t s_optFlags;

    static void AnimateThread(void* param);
    static void __stdcall ComputeElementShaders(M2Element* element);
    static int32_t SortAdditiveParticles(uint32_t a, uint32_t b, const void* param);
    static int32_t SortDoodad(uint32_t a, uint32_t b, const void* param);
    static int32_t SortDoodadProxy(uint32_t a, uint32_t b, const void* param);
    static int32_t SortHitNear(uint32_t a, uint32_t b, const void* param);
    static int32_t SortOpaque(uint32_t a, uint32_t b, const void* param);
    static int32_t SortOpaqueGeoBatches(M2Element* a, M2Element* b);
    static int32_t SortOpaqueParticles(M2Element* a, M2Element* b);
    static int32_t SortOpaqueRibbons(M2Element* a, M2Element* b);
    static int32_t SortTransparent(uint32_t a, uint32_t b, const void* param);

    CM2Scene(CM2Cache* cache);
    ~CM2Scene();
    M2Element* AddParticleElement(CParticleEmitter2* emitter, CM2Model* model, float depth, float alpha, int32_t split, uint32_t& index, uint32_t& additiveCount);
    void AdvanceTime(uint32_t delta);
    void AllocateSpaceForHitList();
    int32_t Animate(const C3Vector& cameraPos);
    void BeginHitTest();
    int32_t ComputeRayDirAndLen(const C3Vector& start, const C3Vector& end, float t, float* length, C3Vector* dir);
    CM2Model* CreateModel(const char* fileName, uint32_t flags);
    void Draw(M2PASS pass);
    CM2Model* DuplicateModel(CM2Model* model, uint32_t flags);
    void* EndHitTest(const C3Vector& start, const C3Vector& end, float& t, int32_t twoPass);
    void* EndHitTestCollisionWorld(const C3Vector& start, const C3Vector& end, float& t);
    M2HitRec* HitTestCollision(const CM2Model* model, uint32_t pass, const C3Vector& dir, float d, const C3Vector& start, M2HitRec* rec, float& t, M2HitRec* best);
    M2HitRec* HitTestGeometry(const CM2Model* model, uint32_t pass, const C3Vector& dir, float d, const C3Vector& start, M2HitRec* rec, float& t, M2HitRec* best);
    M2HitRec* IntersectHitTestTriangles(const uint16_t* indices, const uint16_t* end, uint32_t vertexStart, const C3Vector& start, uint32_t pass, M2HitRec* rec, float& t, M2HitRec* best);
    void PurgeModelCache();
    void Release();
    void SelectLights(CM2Lighting* lighting);
    void SetFloat18(float value);
    void SetProjectPositionCallback(int32_t (*callback)(const C3Vector&, float&, void*), void* param);
    void SetProjectTextureCallback(void (*callback)(const CAaBox&, const CImVector&, int32_t, void*, int32_t), void* param);
    void SetTime(uint32_t time);
    void SortAdditiveParticleElements(uint32_t pass);
    uint32_t SphereTestModels(const C3Vector& start, const C3Vector& dir, float length, int32_t frameCheck);
    void TransformHitTestVertices(const CM2Model* model, const M2SkinProfile* skinData, const M2SkinSection* section, int32_t normals, const C3Vector& dir, float d);
    void TransformHitTestVerticesBlend(const CM2Model* model, const M2SkinProfile* skinData, const M2SkinSection* section, int32_t normals, const C3Vector& dir, float d);
    void TransformHitTestVerticesBlendSSE(const CM2Model* model, const M2SkinProfile* skinData, const M2SkinSection* section, int32_t normals, const C3Vector& dir, float d);
};

extern const uint32_t s_blendModes[3][M2BLEND_COUNT];
extern const uint32_t s_localLighting[M2BLEND_COUNT];

uint32_t ParticleBlendToM2Blend(uint32_t blend);
uint32_t ParticleFlagsToM2Flags(uint32_t flags);
void M2BlendBoneMatrices(const C44Matrix* bones, ubyte4 weights, ubyte4 indices, C44Matrix* result);
void M2BlendBoneMatricesSSE(const C44Matrix* bones, ubyte4 weights, ubyte4 indices, C44Matrix* result);

#endif
