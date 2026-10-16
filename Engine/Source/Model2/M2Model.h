#ifndef ENGINE_SOURCE_MODEL2_M2MODEL_H
#define ENGINE_SOURCE_MODEL2_M2MODEL_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CRange.h"

class C3Vector;
class C44Matrix;
class C4Quaternion;
class CAaBox;
class CAaSphere;
class CCamera;
class CFacet;
struct CGxBuf;
struct CGxPool;
class CImVector;
class CM2Lighting;
class CM2Model;
class CM2SequencePlayBack;
class CParticleEmitter2;
class CRibbonEmitter;
class CShaderEffect;
class M2Batch;
class M2SkinSection;
class M2VertexPNT2;
class CM2ModelCall;
class CM2Scene;
class CM2Shared;
class CTexture;
class M2ModelBone;
template <class T>
class M2ModelTrack;
class M2ModelBoneSeq;
class M2SequenceFallback;
class M2ModelCamera;
class M2ModelColor;
class M2ModelLight;
class M2ModelParticle;
class M2ModelRibbon;
class M2ModelTextureTransform;

typedef void (*M2EventCallback)(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param);
typedef void (*M2SequenceCallback)(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);

class CModelBatch {
    public:
    /* 0000 */ CM2Model* m_model;
    /* 0004 */ uint32_t m_batchIndex;
    /* 0008 */ uint32_t m_count;
};

class CModelBatchList {
    public:
    /* 0000 */ CModelBatch* m_batches;
    /* 0004 */ uint32_t m_count;
    /* 0008 */ uint32_t m_maxCount;

    void Add(CM2Model* model, uint32_t batchIndex);
    void Optimize();
};

class SModelBatchLessThan {
    public:
    bool operator()(const CModelBatch& a, const CModelBatch& b) const;
};

class M2ModelCallback {
    public:
    /* 0000 */ uint32_t m_type;
    /* 0004 */ CM2Model* m_model;
    /* 0008 */ uint32_t m_boneId;
    /* 000C */ void* m_callback;
    /* 0010 */ uint32_t m_id;
    /* 0014 */ uint32_t m_value;
    /* 0018 */ uint32_t m_args [3];
    /* 0024 */ uint32_t m_time;
    /* 0028 */ uint64_t m_param;
};

class M2SequenceInfo {
    public:
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ uint32_t m_duration;
    /* 0008 */ float m_moveSpeed;
    /* 000C */ CAaBox m_bounds;
    /* 0024 */ C3Vector m_center;
    /* 0030 */ float m_radius;
    /* 0034 */ uint32_t m_animId;
    /* 0038 */ uint32_t m_fallbackMode;

    void Clear();
};

class M2BoneSequenceInfo {
    public:
    /* 0000 */ uint32_t m_sequenceId;
    /* 0004 */ uint32_t m_variationIndex;
    /* 0008 */ uint32_t m_time;
    /* 000C */ float m_speed;
    /* 0010 */ uint32_t m_startTime;
    /* 0014 */ uint32_t m_endTime;
    /* 0018 */ uint32_t m_finished;
    /* 001C */ uint32_t m_complete;
};

class M2BatchRange {
    public:
    /* 0000 */ uint32_t m_first;
    /* 0004 */ uint32_t m_last;

    M2BatchRange();
};

class M2ModelOptGeo {
    public:
    /* 0000 */ M2Batch* m_batches;
    /* 0004 */ uint32_t m_batchCount;
    /* 0008 */ M2SkinSection* m_skinSections;
    /* 000C */ uint32_t m_skinSectionCount;
    /* 0010 */ M2BatchRange* m_batchRanges;
    /* 0014 */ CGxPool* m_indexPool;
    /* 0018 */ CGxBuf* m_indexBuf;
    /* 001C */ CShaderEffect** m_effects;
};

class CM2Model {
    public:
    /* 0000 */ uint32_t m_refCount;
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ CM2Model** m_scenePrev;
    /* 000C */ CM2Model* m_sceneNext;
    /* 0010 */ uint32_t m_fFlags;
    /* 0014 */ uint16_t m_animListHead;
    /* 0016 */ uint16_t unused_0016;
    /* 0018 */ CM2Model** m_callbackPrev;
    /* 001C */ CM2Model* m_callbackNext;
    /* 0020 */ void (*m_loadedCallback)(CM2Model*, void*);
    /* 0024 */ void* m_loadedArg;
    /* 0028 */ CM2Scene* m_scene;
    /* 002C */ CM2Shared* m_shared;
    /* 0030 */ CM2Model* m_model30;
    /* 0034 */ CM2ModelCall* m_modelCallList;
    /* 0038 */ CM2ModelCall** m_modelCallTail;
    /* 003C */ uint32_t m_frameStamp;
    /* 0040 */ CM2Model** m_animatePrev;
    /* 0044 */ CM2Model* m_animateNext;
    /* 0048 */ CM2Model* m_attachParent;
    /* 004C */ M2ModelTrack<uint8_t>* m_attachments;
    /* 0050 */ uint32_t m_attachmentId;
    /* 0054 */ uint32_t m_attachmentIndex;
    /* 0058 */ CM2Model* m_attachList;
    /* 005C */ CM2Model** m_attachPrev;
    /* 0060 */ CM2Model* m_attachNext;
    /* 0064 */ uint32_t m_lastAnimTime;
    /* 0068 */ CM2Model** m_drawPrev;
    /* 006C */ CM2Model* m_drawNext;
    /* 0070 */ int32_t* m_loops;
    /* 0074 */ uint32_t m_loopOrigin;
    /* 0078 */ void* m_sequenceCallback;
    /* 007C */ uint32_t unused_007C;
    /* 0080 */ uint64_t m_sequenceCallbackParam;
    /* 0088 */ float m_scaleSq;
    /* 008C */ uint32_t m_lastEmitterTime;
    /* 0090 */ uint32_t m_uint90;
    /* 0094 */ M2ModelBone* m_bones;
    /* 0098 */ C44Matrix* m_boneMatrices;
    /* 009C */ uint32_t* m_skinSections;
    /* 00A0 */ M2ModelColor* m_colors;
    /* 00A4 */ CTexture** m_textures;
    /* 00A8 */ M2ModelTrack<float>* m_textureWeights;
    /* 00AC */ M2ModelTextureTransform* m_textureTransforms;
    /* 00B0 */ C44Matrix* m_textureMatrices;
    /* 00B4 */ C44Matrix m_worldTransform;
    /* 00F4 */ C44Matrix m_matrixF4;
    /* 0134 */ C44Matrix m_frameOfReferenceMatrix;
    /* 0174 */ void* m_frameOfReference;
    /* 0178 */ float m_fadeAlpha;
    /* 017C */ float m_baseAlpha;
    /* 0180 */ float m_diffuseTintR;
    /* 0184 */ float m_diffuseTintG;
    /* 0188 */ float m_diffuseTintB;
    /* 018C */ float m_emissiveTintR;
    /* 0190 */ float m_emissiveTintG;
    /* 0194 */ float m_emissiveTintB;
    /* 0198 */ float m_effectiveAlpha;
    /* 019C */ float m_alpha;
    /* 01A0 */ C3Vector m_diffuseColor;
    /* 01AC */ C3Vector m_emissiveColor;
    /* 01B8 */ float unk_01B8;
    /* 01BC */ void (*m_drawCallback)(CM2Model*, const CM2Lighting*, void*);
    /* 01C0 */ void* m_drawArg;
    /* 01C4 */ void* m_eventCallback;
    /* 01C8 */ uint64_t m_eventCallbackParam;
    /* 01D0 */ M2ModelLight* m_lights;
    /* 01D4 */ CM2Lighting m_lighting;
    /* 02A8 */ CM2Lighting* m_currentLighting;
    /* 02AC */ void (*m_lightingCallback)(CM2Model*, CM2Lighting*, void*);
    /* 02B0 */ void* m_lightingArg;
    /* 02B4 */ M2ModelCamera* m_cameras;
    /* 02B8 */ M2ModelRibbon* m_ribbons;
    /* 02BC */ CRibbonEmitter** m_ribbonEmitters;
    /* 02C0 */ M2ModelParticle* m_particles;
    /* 02C4 */ CParticleEmitter2** m_particleEmitters;
    /* 02C8 */ CM2Model** m_particlePrev;
    /* 02CC */ CM2Model* m_particleNext;
    /* 02D0 */ M2ModelOptGeo* m_optGeo;
    /* 02D4 */ uint32_t m_hitTestMode;
    /* 02D8 */ CM2Model** m_hitTestPrev;
    /* 02DC */ CM2Model* m_hitTestNext;
    /* 02E0 */ void* m_hitTestOwner;
    /* 02E4 */ uint32_t m_hitTestGroup;
    /* 02E8 */ uint32_t m_handle;
    /* 02EC */ uint32_t unused_02EC;

    static void (*s_transformVertices[5])(CM2Model*, M2SkinSection*, uint8_t*, int32_t);
    static TSGrowableArray<M2ModelCallback> s_sequenceCallbacks;
    static TSGrowableArray<C3Vector> s_collisionPositions;
    static TSGrowableArray<uint32_t> s_collisionCodes;

    static int32_t __stdcall SequenceCallback(void* callback, CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint32_t sequenceIndex, uint32_t startTime, uint64_t param);
    static void RenderModelBatchesShadowMap(const CModelBatchList& untextured, const CModelBatchList& textured);
    static void RenderModelBatchListShadowMap(int32_t untextured, const CModelBatchList& list);
    static void TransformVertices_cpp(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord);
    static void TransformVertices_sse(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord);
    static void TransformVerticesNoUVSelect_cpp(const CM2Model* model, const M2SkinSection* section, M2VertexPNT2* out);
    static void TransformVerticesOneBone_cpp(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord);
    static void TransformVerticesOneBone_sse(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord);

    CM2Model();
    ~CM2Model();
    int32_t CanUpdateNonReadyTextures();
    void ChangeEmittersFrameOfReference(const C44Matrix* matrix);
    void ChangeFrameOfReference(const C44Matrix* matrix);
    void ComputeLoadProgress(int64_t& loaded, int64_t& total);
    int32_t BoneHasParent(uint32_t boneId);
    void DetachAllChildrenById(uint32_t id);
    void DetachFromParent();
    int32_t FindEvent(uint32_t id, C3Vector** position, uint16_t* bone);
    C3Vector GetAttachmentPivot(uint32_t id);
    C3Vector GetEventPivot(uint32_t id);
    uint32_t GetEventSequenceTime(uint32_t sequenceId, uint32_t eventId);
    int32_t HasAttachment(uint32_t id);
    int32_t HasEvent(uint32_t id);
    void LoadSequence(uint32_t id);
    void PickRandomSequenceVariation(uint32_t* variation, uint16_t* sequenceIndex);
    M2SequenceFallback* SequenceFallbackById(M2SequenceFallback* result, uint32_t id);
    void SetBoneProceduralTransform(uint32_t boneId, const C44Matrix& matrix);
    void SetBoneSequenceSpeed(uint32_t boneId, float speed);
    void SetBoneSequenceTime(uint32_t boneId, uint32_t time);
    void SetupBoneSequence(uint16_t sequenceIndex, M2SequenceFallback fallback, uint32_t time, float speed, M2ModelBoneSeq* boneSequence);
    void DetachFromScene();
    uint16_t FindSequenceIndex(const M2Data* data, uint32_t id, int32_t variation);
    uint32_t FindSequenceVariationCount(const M2Data* data, uint32_t id);
    C3Vector GetBonePivot(uint32_t boneId);
    uint32_t GetBoneSequenceAnimId(uint32_t boneId, uint32_t* variation);
    uint32_t GetBoneSequenceId(uint32_t boneId);
    int32_t GetBoneSequenceInfo(uint32_t boneId, M2BoneSequenceInfo& info);
    float GetBoneSequenceSpeed(uint32_t boneId);
    CAaBox GetBoundingBox();
    CAaSphere GetBoundingSphere();
    C44Matrix GetAnimatedWorldTransform();
    CAaBox GetCurrentBoundingBox();
    CAaBox GetMaxBoundingBoxPlusAttachments();
    CAaSphere GetMaxBoundingSpherePlusAttachments();
    uint32_t GetSequenceVariationCount(uint32_t id);
    CAaBox GetSplitBodyBoundingBox(uint32_t boneId);
    C3Vector GetWorldPosition();
    int32_t HasBone(uint32_t boneId);
    int32_t HasSequence(const M2Data* data, uint32_t id);
    int32_t HasSequence(uint32_t id);
    int32_t IsSequenceLoaded(uint32_t id);
    uint32_t ResolveSequenceFallback(uint32_t id);
    void SetBoneFlags(uint32_t boneId, uint32_t flags, uint32_t mask);
    void UnoptimizeVisibleGeometry();
    void Animate();
    void AttachToScene(CM2Scene* scene);
    void AttachToSceneRecursive(CM2Scene* scene);
    void GatherModelBatchesShadowMap(CModelBatchList& untextured, CModelBatchList& textured);
    uint32_t GetBillboardBoneCount() const;
    void CallCallbacks();
    void CallSequenceCallback(M2ModelCallback* entry);
    void CancelAllDeferredSequences();
    void CancelDeferredSequences(uint16_t boneIndex, bool primary);
    int32_t ProcessCallbacks();
    void SetBoneSequence(uint32_t boneId, uint32_t sequenceId, uint32_t variation, uint32_t time, float speed, int32_t blend, int32_t primary);
    void SetBoneSequenceDeferred(uint16_t sequenceIndex, M2Data* data, uint16_t boneIndex, uint32_t time, float speed, M2SequenceFallback fallback, int32_t blend, int32_t primary, int32_t pickRandom);
    void UnsetBoneSequence(uint32_t boneId, int32_t blend, int32_t primary);
    void AnimateSM();
    void AttachToParent(CM2Model* parent, uint32_t id, const C3Vector* position, int32_t attachToRoot);
    C3Vector GetAttachmentPosition(uint32_t id);
    float GetAttachmentWorldScale(uint32_t id);
    C44Matrix GetAttachmentWorldTransform(uint32_t id);
    C3Vector GetEventPosition(uint32_t id);
    C44Matrix GetEventWorldTransform(uint32_t id);
    void AnimateAttachmentsMT();
    void ForceAnimate();
    uint32_t ProcessEventCallbacks(uint32_t boneIndex, uint32_t sequenceIndex, uint32_t timeOffset, uint32_t period, uint32_t startTime, uint32_t endTime, uint32_t callbackTime);
    void AnimateParticlesMT();
    int32_t HasDeferredSequence(uint16_t boneIndex, bool primary);
    uint32_t ProcessSequenceCallback(uint32_t boneIndex, uint32_t frame, uint32_t time, uint32_t lastFrame);
    void AnimateTextureTransformsMT();
    void FreeInternalResources();
    void GetCollisionFacets(const CAaBox& box, const C44Matrix& transform, TSGrowableArray<CFacet>& facets);
    CAaSphere GetCurrentBoundingSphere();
    uint32_t GetNumBatches();
    void GetSequenceInfo(uint32_t id, uint32_t variation, M2SequenceInfo& info);
    void OptimizeVisibleGeometry();
    void SetGeometryVisible(uint32_t first, uint32_t last, int32_t visible);
    void SetGeosetVisibilityRange(uint32_t first, uint32_t last, int32_t visible);
    void FindKey(M2ModelBoneSeq* sequence, const M2TrackBase& track, uint32_t& key0, uint32_t& key1, float& ratio);
    void FreeExternalResources();
    CCamera* GetCameraById(uint32_t id);
    uint32_t GetNumCombinableDoodadBatches();
    uint32_t GetNumGeometryBatches();
    uint32_t GetNumPrimitives();
    int32_t HasCamera(uint32_t id);
    void SetEmittersEnabled(int32_t enabled);
    void* GetCameraByIndex(uint32_t index);
    uint32_t GetCurrentLightCount();
    uint32_t GetInstanceCountMax();
    int32_t HasLiveParticles();
    int32_t HasLiveRibbons();
    uint32_t IsLoaded(int32_t wait, int32_t recursive);
    void LinkToAsyncHead(int32_t textures);
    void LinkToAsyncHeadRecursive(int32_t textures);
    void ReplaceParticleColor(uint32_t id, CImVector color0, CImVector color1, CImVector color2);
    void ReplaceTexture(uint32_t type, CTexture* texture);
    void SetAnimating(int32_t animating);
    void SetEventCallback(void* callback, uint64_t param);
    void SetLightEnabled(uint32_t index, uint32_t enabled);
    void SetLoadedCallback(void (*callback)(CM2Model*, void*), void* arg);
    void SetRibbonsEnabled(uint32_t enabled);
    void SetSequenceCallback(void* callback, uint64_t param);
    void SetWorldTransform(const C44Matrix& matrix);
    void SetWorldTransform(const C3Vector& position, float rotation, float scale);
    void SetWorldTransform(const C3Vector& position, float rotation, float scale, const C3Vector& up, uint32_t flags);
    void UpdateLoaded();
    void WaitForLoad(const char* reason);
    void AnimateMT(const C44Matrix* view, const C3Vector& diffuse, const C3Vector& emissive, float alpha, float alpha2);
    void AnimateMTSimple(const C44Matrix* view, const C3Vector& diffuse, const C3Vector& emissive, float alpha, float alpha2);
    void AnimateParticleST(float deltaTime, uint32_t index);
    void AnimateST();
    int32_t GetRegionBounds(M2SkinSection* section, CAaBox& box, C44Matrix& matrix);
    int32_t Initialize(CM2Scene* scene, CM2Shared* shared, CM2Model* source, uint32_t flags);
    int32_t InitializeLoaded();
    int32_t IsBatchDoodadCompatible(M2Batch* batch);
    int32_t IsDrawable(int32_t a, int32_t b);
    void LinkToCallbackListTail();
    int32_t OnSequenceInterrupted(uint32_t boneId, uint16_t boneIndex);
    void ProcessCallbacksRecursive();
    void RenderBatchShadowMap(int32_t untextured, M2Batch* batch, const CModelBatchList& list, uint32_t index, M2SkinSection* section, M2SkinSection* prevSection);
    void RenderModelBatchesForProjectedTexture();
    int32_t Release();
    int32_t SetIndices();
    void SetPrimaryBoneSequence(uint16_t sequenceIndex, uint16_t boneIndex, M2SequenceFallback fallback, uint32_t time, float speed, int32_t blend);
    void SetSecondaryBoneSequence(uint16_t sequenceIndex, uint16_t boneIndex, M2SequenceFallback fallback, uint32_t time, float speed);
    void SetupLighting();
    int32_t SetVertices(int32_t optimized, M2SkinSection* section, int32_t texCoord);
    int32_t SetVerticesMultiSample(int32_t optimized, M2SkinSection* section);
    void UnlinkFromCallbackList();
};

class M2SequenceFallback {
    public:
    /* 0000 */ uint16_t m_id;
    /* 0002 */ uint16_t m_mode;
};

class CM2SequencePlayBack : public TSLinkedNode<CM2SequencePlayBack> {
    public:
    /* 0008 */ CM2Model* m_model;
    /* 000C */ uint16_t m_boneIndex;
    /* 000E */ uint16_t m_flags;
    /* 0010 */ uint32_t m_time;
    /* 0014 */ float m_speed;
    /* 0018 */ M2SequenceFallback m_fallback;
};

class M2ModelBoneSeq {
    public:
    /* 0000 */ uint32_t m_currentTime;
    /* 0004 */ uint16_t m_animIndex;
    /* 0006 */ uint16_t m_sourceBoneIndex;
    /* 0008 */ uint16_t m_sequenceIndex;
    /* 000A */ uint8_t m_finished;
    /* 000B */ uint8_t m_pickRandomVariation;
    /* 000C */ uint32_t m_startTime;
    /* 0010 */ uint32_t m_endTime;
    /* 0014 */ float m_speed;
    /* 0018 */ float m_invSpeed;
    /* 001C */ uint32_t m_startOffset;
    /* 0020 */ uint32_t m_repeatCount;

    M2ModelBoneSeq();
};

template <class T>
class M2ModelTrack {
    public:
    /* 0000 */ uint32_t m_currentKey;
    /* 0004 */ uint32_t m_secondaryKey;
    /* 0008 */ T m_currentValue;

    M2ModelTrack();
};

class M2ModelBone {
    public:
    /* 0000 */ M2ModelTrack<C3Vector> m_translation;
    /* 0014 */ M2ModelTrack<C4Quaternion> m_rotation;
    /* 002C */ M2ModelTrack<C3Vector> m_scale;
    /* 0040 */ M2ModelBoneSeq m_sequence;
    /* 0064 */ M2ModelBoneSeq m_secondarySequence;
    /* 0088 */ C44Matrix* m_proceduralTransform;
    /* 008C */ uint32_t m_flags;
    /* 0090 */ uint32_t m_sequenceId;
    /* 0094 */ uint16_t m_variationIndex;
    /* 0096 */ uint16_t m_animListNext;
    /* 0098 */ uint16_t* m_animListPrev;
    /* 009C */ uint32_t m_blendEndTime;
    /* 00A0 */ float m_invBlendDuration;
    /* 00A4 */ float m_blendWeightMax;
    /* 00A8 */ float m_blendFactor;

    M2ModelBone();
};

class M2ModelCamera {
    public:
    /* 0000 */ M2ModelTrack<C3Vector> m_positionTrack;
    /* 0014 */ M2ModelTrack<C3Vector> m_targetTrack;
    /* 0028 */ M2ModelTrack<float> m_rollTrack;
    /* 0034 */ CCamera* m_camera;
};

class M2ModelColor {
    public:
    /* 0000 */ M2ModelTrack<C3Vector> m_colorTrack;
    /* 0014 */ M2ModelTrack<float> m_alphaTrack;
};

class M2ModelParticle {
    public:
    /* 0000 */ M2ModelTrack<float> m_speedTrack;
    /* 000C */ M2ModelTrack<float> m_variationTrack;
    /* 0018 */ M2ModelTrack<float> m_latitudeTrack;
    /* 0024 */ M2ModelTrack<float> m_longitudeTrack;
    /* 0030 */ M2ModelTrack<float> m_gravityTrack;
    /* 003C */ M2ModelTrack<float> m_lifeTrack;
    /* 0048 */ M2ModelTrack<float> m_emissionRateTrack;
    /* 0054 */ M2ModelTrack<float> m_widthTrack;
    /* 0060 */ M2ModelTrack<float> m_lengthTrack;
    /* 006C */ M2ModelTrack<float> m_zsourceTrack;
    /* 0078 */ M2ModelTrack<uint8_t> m_visibilityTrack;
    /* 0084 */ uint8_t m_emitting;
    /* 0085 */ uint8_t m_active;
    /* 0086 */ uint8_t m_burstFired;
    /* 0087 */ uint8_t unused_0087;

    M2ModelParticle();
};

class M2ModelRibbon {
    public:
    /* 0000 */ M2ModelTrack<C3Vector> m_colorTrack;
    /* 0014 */ M2ModelTrack<float> m_alphaTrack;
    /* 0020 */ M2ModelTrack<float> m_heightAboveTrack;
    /* 002C */ M2ModelTrack<float> m_heightBelowTrack;
    /* 0038 */ M2ModelTrack<uint16_t> m_textureSlotTrack;
    /* 0044 */ M2ModelTrack<uint8_t> m_visibilityTrack;
};

class M2ModelTextureTransform {
    public:
    /* 0000 */ M2ModelTrack<C3Vector> m_translation;
    /* 0014 */ M2ModelTrack<C4Quaternion> m_rotation;
    /* 002C */ M2ModelTrack<C3Vector> m_scaling;
};

template <class T, class V>
void M2AnimateSplineTrack(CM2Model* model, M2ModelBone* bone, const M2Track<T>& track, M2ModelTrack<V>& modelTrack, const V& defaultValue);

template <class T, class V>
void M2AnimateTrack(CM2Model* model, M2ModelBone* bone, const M2Track<T>& track, M2ModelTrack<V>& modelTrack, const V& defaultValue);

template <class T>
M2ModelTrack<T>::M2ModelTrack() {
    this->m_currentKey = 0;
    this->m_secondaryKey = 0;
    memset(&this->m_currentValue, 0, sizeof(T));
}

#endif
