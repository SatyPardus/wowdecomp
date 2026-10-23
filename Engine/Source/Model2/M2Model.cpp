#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <new>
#include <xmmintrin.h>
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Base/SIMD.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2ModelLight.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/Services/RibbonEmitter.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/CAaRect.h"
#include "Engine/Source/Tempest/CFacet.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Types.h"

// STATIC: 0x00A45570
static const uint32_t s_ribbonBlendModes[] = { 0, 1, 2, 10, 3, 4, 5 };

// STATIC: 0x00D4118C
void (*CM2Model::s_transformVertices[5])(CM2Model*, M2SkinSection*, uint8_t*, int32_t);

// STATIC: 0x00D411A0
TSGrowableArray<C3Vector> CM2Model::s_collisionPositions;

// STATIC: 0x00D411B0
TSGrowableArray<uint32_t> CM2Model::s_collisionCodes;

// STATIC: 0x00D411C0
TSGrowableArray<M2ModelCallback> CM2Model::s_sequenceCallbacks;

// INLINED
CM2ModelCall::CM2ModelCall() {
    this->m_type = MODEL_COMMAND_NOT_QUEUED;
}

// VERIFY: 35d86a94 auto
// OFFSET: 0x00823D50
void CModelBatchList::Add(CM2Model* model, uint32_t batchIndex) {
    if (this->m_count < this->m_maxCount) {
        this->m_batches[this->m_count].m_model = model;
        this->m_batches[this->m_count].m_batchIndex = batchIndex;
        this->m_batches[this->m_count++].m_count = 1;
    }
}

// VERIFY: d951ddda manual
// OFFSET: 0x00823D90
void CM2Model::DetachFromScene() {
    if (this->m_scenePrev) {
        *this->m_scenePrev = this->m_sceneNext;
    }

    if (this->m_sceneNext) {
        this->m_sceneNext->m_scenePrev = this->m_scenePrev;
    }

    this->m_scenePrev = nullptr;
    this->m_sceneNext = nullptr;

    if (!(this->m_fFlags & 1)) {
        for (CM2ModelCall* call = this->m_modelCallList; call; call = call->m_next) {
            call->m_time -= this->m_scene->m_time;
        }
    } else {
        M2Data* data = this->m_shared->m_data;

        for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
            CM2Light* light = &this->m_lights[i].m_light;
            light->Unlink();
            light->m_scene = nullptr;
        }
    }

    this->m_scene = nullptr;
}

// VERIFY: d45d8886 auto
// OFFSET: 0x00823E40
int32_t CM2Model::CanUpdateNonReadyTextures() {
    if (!(this->m_fFlags & 1)) {
        return 0;
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_textures.m_count; i++) {
        CTexture* texture = this->m_textures[i];

        if (texture && !TextureCanUpdate(texture) && !TextureIsReadyNow(this->m_textures[i])) {
            return 0;
        }
    }

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        if (!model->CanUpdateNonReadyTextures()) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: b2c20bd9 auto
// OFFSET: 0x00823ED0
void CM2Model::WaitForLoad(const char* reason) {
    if (this->m_shared->m_asyncObject) {
        AsyncFileReadWait(this->m_shared->m_asyncObject);
    }

    if (this->m_shared->m_asyncObject) {
        AsyncFileReadWait(this->m_shared->m_asyncObject);
    }

    if (this->m_flags & 0x20) {
        this->InitializeLoaded();
    }
}

// VERIFY: 240e75c9 auto
// OFFSET: 0x00823F10
void CM2Model::SetAnimating(int32_t animating) {
    if (this->m_flags & 0x20) {
        if (!animating) {
            goto unlink;
        }

        if (!(this->m_fFlags & 1)) {
            this->WaitForLoad(nullptr);
        }
    }

    if (animating) {
        if (!this->m_animatePrev) {
            CM2Model** head = &this->m_scene->m_animateList;
            this->m_animatePrev = head;
            this->m_animateNext = *head;
            *head = this;

            if (this->m_animateNext) {
                this->m_animateNext->m_animatePrev = &this->m_animateNext;
            }
        }

        return;
    }

unlink:
    if (this->m_animatePrev) {
        *this->m_animatePrev = this->m_animateNext;

        if (this->m_animateNext) {
            this->m_animateNext->m_animatePrev = this->m_animatePrev;
        }

        this->m_animatePrev = nullptr;
        this->m_animateNext = nullptr;
    }
}

// VERIFY: 716d467d auto
// OFFSET: 0x00823F90
static int32_t BatchesMatch(const M2Batch* a, const M2Batch* b, const M2SkinSection* sectionA, const M2SkinSection* sectionB) {
    return a->m_colorIndex == b->m_colorIndex && a->m_materialIndex == b->m_materialIndex && a->m_textureComboIndex == b->m_textureComboIndex && a->m_textureWeightComboIndex == b->m_textureWeightComboIndex && a->m_textureTransformComboIndex == b->m_textureTransformComboIndex && sectionA->m_boneComboIndex == sectionB->m_boneComboIndex;
}

// VERIFY: bf4ba22e auto
// OFFSET: 0x00823FE0
void CM2Model::SetSequenceCallback(void* callback, uint64_t param) {
    if (this->m_sequenceCallback == callback && this->m_sequenceCallbackParam == param) {
        return;
    }

    this->m_sequenceCallbackParam = param;
    this->m_sequenceCallback = callback;

    if (!(this->m_fFlags & 1)) {
        return;
    }

    M2Data* data = this->m_shared->m_data;

    if (callback) {
        this->m_fFlags |= 0x400000;
    } else if (data->m_sequences.m_count <= 1 && (!this->m_eventCallback || !data->m_events.m_count)) {
        this->m_fFlags &= ~0x400000u;
    }
}

// VERIFY: 673d0acf auto
// OFFSET: 0x00824060
void CM2Model::SetEventCallback(void* callback, uint64_t param) {
    if (this->m_eventCallback == callback && this->m_eventCallbackParam == param) {
        return;
    }

    this->m_eventCallbackParam = param;
    this->m_eventCallback = callback;

    if (!(this->m_fFlags & 1)) {
        return;
    }

    M2Data* data = this->m_shared->m_data;

    if (!callback || !data->m_events.m_count) {
        if (!this->m_sequenceCallback && data->m_sequences.m_count <= 1) {
            this->m_fFlags &= ~0x400000u;
        }
    } else {
        this->m_fFlags |= 0x400000;
    }
}

// VERIFY: af6c7545 manual
// OFFSET: 0x008240F0
void CM2Model::SetLightEnabled(uint32_t index, uint32_t enabled) {
    if (this->m_fFlags & 1) {
        this->m_lights[index].m_enabled = enabled;
        return;
    }

    CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

    if (call) {
        call->m_type = MODEL_SET_LIGHT_ENABLED;
        call->m_next = nullptr;
        call->m_time = this->m_scene->m_time;
        call->m_args[0] = index;
        call->m_args[1] = enabled;
        *this->m_modelCallTail = call;
        this->m_modelCallTail = &call->m_next;
    }
}

// VERIFY: 15a8f3dc auto
// OFFSET: 0x00824170
void* CM2Model::GetCameraByIndex(uint32_t index) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    return this->m_cameras[index].m_camera;
}

// VERIFY: 38985dd6 auto
// OFFSET: 0x008241D0
int32_t CM2Model::HasLiveRibbons() {
    if (!(this->m_fFlags & 1)) {
        return 0;
    }

    int32_t count = this->m_shared->m_data->m_ribbons.m_count;

    for (int32_t i = 0; i < count; i++) {
        if (!this->m_ribbonEmitters[i]->IsDead()) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: fdbbd9d7 manual
// OFFSET: 0x00824230
void CM2Model::SetRibbonsEnabled(uint32_t enabled) {
    if (this->m_fFlags & 1) {
        int32_t count = this->m_shared->m_data->m_ribbons.m_count;

        for (int32_t i = 0; i < count; i++) {
            this->m_ribbonEmitters[i]->SetUserEnabled(enabled);
        }

        return;
    }

    CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

    if (call) {
        call->m_type = MODEL_SET_RIBBONS_ENABLED;
        call->m_next = nullptr;
        call->m_time = this->m_scene->m_time;
        call->m_args[0] = enabled;
        *this->m_modelCallTail = call;
        this->m_modelCallTail = &call->m_next;
    }
}

// VERIFY: 8386034a manual
// OFFSET: 0x008242C0
int32_t CM2Model::HasLiveParticles() {
    if (!(this->m_fFlags & 1)) {
        return 0;
    }

    uint32_t count = this->m_shared->m_data->m_particles.m_count;

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_particleEmitters[i]->HasLiveParticles()) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: b133747e auto
// OFFSET: 0x00824320
uint32_t CM2Model::GetInstanceCountMax() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    return this->m_shared->m_maxInstances;
}

// VERIFY: 1593de47 auto
// OFFSET: 0x00824380
uint32_t CM2Model::GetCurrentLightCount() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    CM2Lighting* lighting = this->m_currentLighting;

    return lighting ? lighting->m_lightCount : 0;
}

// VERIFY: 06f8b93f auto
// OFFSET: 0x008243E0
void CM2Model::ChangeEmittersFrameOfReference(const C44Matrix* matrix) {
    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
        this->m_particleEmitters[i]->ChangeFrameOfReference(matrix);
    }

    for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
        this->m_ribbonEmitters[i]->ChangeFrameOfReference(matrix);
    }
}

// VERIFY: 2e7cc77d auto
// OFFSET: 0x00824460
void CM2Model::ChangeFrameOfReference(const C44Matrix* matrix) {
    if (!(this->m_fFlags & 1)) {
        return;
    }

    if (matrix) {
        void* previous = this->m_frameOfReference;
        this->m_frameOfReference = const_cast<C44Matrix*>(matrix);

        if (!previous) {
            C44Matrix inverse = matrix->AffineInverse();
            this->ChangeEmittersFrameOfReference(&inverse);
        }
    } else {
        if (this->m_frameOfReference) {
            this->ChangeEmittersFrameOfReference(static_cast<C44Matrix*>(this->m_frameOfReference));
        }

        this->m_frameOfReference = nullptr;
    }

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        if (model->m_attachmentIndex != 0xFFFF) {
            model->ChangeFrameOfReference(matrix);
        }
    }
}

// VERIFY: d320c318 auto
// OFFSET: 0x008244F0
void CM2Model::LinkToCallbackListTail() {
    CM2Shared* shared = this->m_shared;
    this->m_callbackPrev = shared->m_callbackListTail;
    this->m_callbackNext = nullptr;
    *shared->m_callbackListTail = this;
    this->m_shared->m_callbackListTail = &this->m_callbackNext;
}

// VERIFY: f8581834 auto
// OFFSET: 0x00824510
void CM2Model::UnlinkFromCallbackList() {
    if (!this->m_callbackPrev) {
        return;
    }

    *this->m_callbackPrev = this->m_callbackNext;

    if (this->m_callbackNext) {
        this->m_callbackNext->m_callbackPrev = this->m_callbackPrev;
    } else {
        this->m_shared->m_callbackListTail = this->m_callbackPrev;
    }

    this->m_callbackPrev = nullptr;
    this->m_callbackNext = nullptr;
}

// VERIFY: a722b0dd auto
// OFFSET: 0x00824550
int32_t CM2Model::IsBatchDoodadCompatible(M2Batch* batch) {
    uint32_t cacheFlags = M2GetCacheFlags();

    if ((cacheFlags & 0x20) && (this->m_fFlags & 0x10) && (this->m_shared->m_data->m_bones.m_count > 1 || !(cacheFlags & 0x40)) && (batch->m_flags & 0x10)) {
        CM2Lighting* lighting = this->m_currentLighting;

        if (!lighting || !lighting->m_lightCount) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 4aed42f9 auto
// OFFSET: 0x008245B0
void CM2Model::ComputeLoadProgress(int64_t& loaded, int64_t& total) {
    SFile::FileGetIsLocalAmount(this->m_shared->m_fileName, loaded, total);
    total++;

    if (this->m_fFlags & 1) {
        loaded++;

        for (uint32_t i = 0; i < this->m_shared->m_data->m_textures.m_count; i++) {
            if (this->m_textures[i]) {
                TextureComputeLoadProgress(this->m_textures[i], loaded, total);
            }
        }
    }

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        model->ComputeLoadProgress(loaded, total);
    }
}

// VERIFY: 4f897dd8 manual
// OFFSET: 0x008248C0
CM2Lighting::CM2Lighting() {
    this->m_sphere4.c.x = 0.0f;
    this->m_sphere4.r = 0.0f;
    this->m_sphere4.c.y = 0.0f;
    this->m_sphere4.c.z = 0.0f;
    this->m_vector18 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector24 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector30 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector3C = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector48 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunAmbient = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDiffuse = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunSpecular = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDir = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_fogColor = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_liquidPlane.n.x = 0.0f;
    this->m_liquidPlane.n.y = 0.0f;
    this->m_liquidPlane.n.z = 1.0f;
    this->m_liquidPlane.d = 0.0f;
}

// VERIFY: 5e9baa29 auto
// OFFSET: 0x00824980
M2ModelLight::M2ModelLight() {
    this->m_enabled = 1;
}

// VERIFY: c4261ac5 manual
// OFFSET: 0x00824A00
M2ModelParticle::M2ModelParticle() {
    this->m_emitting = 1;
    this->m_active = 1;
    this->m_burstFired = 0;
}

// VERIFY: f7c9049f manual
// OFFSET: 0x00824A80
static void __fastcall ComputeWorldTransformBasis(C44Matrix* matrix, const C3Vector& up) {
    float bx = matrix->a2 * up.y - matrix->a1 * up.z;
    float by = matrix->a0 * up.z - matrix->a2 * up.x;
    float bz = matrix->a1 * up.x - up.y * matrix->a0;
    matrix->b0 = bx;
    matrix->b1 = by;
    matrix->b2 = bz;

    float length = matrix->b2 * matrix->b2 + matrix->b1 * matrix->b1 + matrix->b0 * matrix->b0;

    if (length > 0.00000023841858f) {
        float scale = 1.0f / sqrtf(length);
        matrix->b0 = matrix->b0 * scale;
        matrix->b1 = scale * matrix->b1;
        matrix->b2 = scale * matrix->b2;
    }

    float ax = matrix->b1 * up.z - up.y * matrix->b2;
    float ay = up.x * matrix->b2 - matrix->b0 * up.z;
    float az = matrix->b0 * up.y - up.x * matrix->b1;
    matrix->a0 = ax;
    matrix->a1 = ay;
    matrix->a2 = az;
    matrix->c0 = up.x;
    matrix->c1 = up.y;
    matrix->c2 = up.z;
}

// VERIFY: b81a5759 manual
// OFFSET: 0x00824D30
void CM2Model::FreeExternalResources() {
    if (!(this->m_fFlags & 1)) {
        return;
    }

    M2Data* data = this->m_shared->m_data;

    if (this->m_bones) {
        for (uint32_t i = 0; i < data->m_bones.m_count; i++) {
            SIMD::FreeA16(this->m_bones[i].m_proceduralTransform);
        }
    }

    if (this->m_textures) {
        for (uint32_t i = 0; i < data->m_textures.m_count; i++) {
            if (this->m_textures[i]) {
                HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[i]));
            }
        }
    }

    if (this->m_lights) {
        for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
            this->m_lights[i].m_light.SetVisible(0);
        }
    }

    if (this->m_cameras) {
        for (uint32_t i = 0; i < data->m_cameras.m_count; i++) {
            M2ModelCamera* camera = &this->m_cameras[i];

            if (camera->m_camera) {
                HandleClose(reinterpret_cast<HOBJECT>(camera->m_camera));
            }
        }
    }

    for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
        this->m_ribbonEmitters[i]->~CRibbonEmitter();
        this->m_ribbonEmitters[i] = nullptr;
    }

    for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
        this->m_particleEmitters[i]->~CParticleEmitter2();
        this->m_particleEmitters[i] = nullptr;
    }
}

// VERIFY: 78234ce5 manual
// OFFSET: 0x00824ED0
int32_t CM2Model::Release() {
    if (this->m_refCount-- == 1) {
        g_modelPool->Delete(this);
        return 0;
    }

    return this->m_refCount;
}

// VERIFY: f4fc2dd4 manual
// OFFSET: 0x00824F00
uint32_t CM2Model::IsLoaded(int32_t wait, int32_t recursive) {
    if (this->m_flags & 0x20) {
        if (this->m_fFlags & 1) {
            return 1;
        }

        if (wait) {
            this->WaitForLoad(nullptr);
        }

        uint32_t sharedFlags = this->m_shared->m_flags;

        return (this->m_fFlags & 1) | (((sharedFlags & (2 * sharedFlags)) >> 1) & 1);
    }

    if (!(this->m_fFlags & 1) && wait) {
        this->WaitForLoad(nullptr);
    }

    if (!(this->m_fFlags & 1)) {
        return 0;
    }

    if (!recursive || (this->m_fFlags & 0x100)) {
        return 1;
    }

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        if (!model->IsLoaded(wait, 1)) {
            return 0;
        }
    }

    this->m_fFlags |= 0x100;

    return 1;
}

// VERIFY: 1574132e auto
// OFFSET: 0x00824FC0
int32_t CM2Model::IsDrawable(int32_t wait, int32_t recursive) {
    if (!(this->m_fFlags & 1) && wait) {
        this->WaitForLoad(nullptr);
    }

    if (!(this->m_fFlags & 2)) {
        if (!(this->m_fFlags & 1)) {
            return 0;
        }

        for (uint32_t i = 0; i < this->m_shared->m_data->m_textures.m_count; i++) {
            if (this->m_textures[i] && !TextureGetGxTex(this->m_textures[i], wait, nullptr)) {
                return 0;
            }
        }

        this->m_fFlags |= 2;
    }

    if (recursive && !(this->m_fFlags & 0x200)) {
        for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
            if (!model->IsDrawable(wait, 1)) {
                return 0;
            }
        }

        this->m_fFlags |= 0x200;
    }

    return 1;
}

// VERIFY: bf2e7ab4 auto
// OFFSET: 0x008250B0
void CM2Model::LinkToAsyncHeadRecursive(int32_t textures) {
    CAsyncObject* object = this->m_shared->m_asyncObject;

    if (object && !object->m_isCurrent && !object->m_isRead && !object->m_isProcessed) {
        AsyncFileReadLinkObject(object, 1);
    }

    if ((this->m_fFlags & 1) && textures) {
        for (uint32_t i = 0; i < this->m_shared->m_data->m_textures.m_count; i++) {
            if (this->m_textures[i]) {
                TextureIncreasePriority(this->m_textures[i]);
            }
        }
    }

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        model->LinkToAsyncHeadRecursive(textures);
    }
}

// VERIFY: e8cd7c5c auto
// OFFSET: 0x00825150
void CM2Model::LinkToAsyncHead(int32_t textures) {
    AsyncFile::EnterQueueLock();
    this->LinkToAsyncHeadRecursive(textures);
    AsyncFile::LeaveQueueLock();
}

// VERIFY: 517a3986 manual
// OFFSET: 0x00825170
void CM2Model::UpdateLoaded() {
    CM2Model* model = this;

    do {
        if (!model->IsLoaded(0, !(model->m_flags & 0x20))) {
            break;
        }

        if (model->m_loadedCallback) {
            model->m_loadedCallback(model, model->m_loadedArg);
            model->m_loadedCallback = nullptr;
        }

        model = model->m_attachParent;
    } while (model);
}

// VERIFY: 1dcb21eb auto
// OFFSET: 0x008251B0
void CM2Model::SetLoadedCallback(void (*callback)(CM2Model*, void*), void* arg) {
    this->m_loadedCallback = callback;
    this->m_loadedArg = arg;
    this->UpdateLoaded();
}

// VERIFY: 2966a4ec manual
// OFFSET: 0x008251D0
void CM2Model::SetWorldTransform(const C3Vector& position, float rotation, float scale) {
    C44Matrix& matrix = this->m_worldTransform;
    matrix.d3 = 1.0f;
    matrix.c2 = 1.0f;
    matrix.b1 = 1.0f;
    matrix.a0 = 1.0f;
    matrix.d2 = 0.0f;
    matrix.d1 = 0.0f;
    matrix.d0 = 0.0f;
    matrix.c3 = 0.0f;
    matrix.c1 = 0.0f;
    matrix.c0 = 0.0f;
    matrix.b3 = 0.0f;
    matrix.b2 = 0.0f;
    matrix.b0 = 0.0f;
    matrix.a3 = 0.0f;
    matrix.a2 = 0.0f;
    matrix.a1 = 0.0f;
    matrix.RotateAroundZ(rotation);
    matrix.Scale(scale);
    matrix.d0 = position.x;
    matrix.d1 = position.y;
    this->m_fFlags |= 0x8000;
    matrix.d2 = position.z;
}

// VERIFY: ceb10740 manual
// OFFSET: 0x00825260
void CM2Model::ReplaceTexture(uint32_t type, CTexture* texture) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_REPLACE_TEXTURE;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = type;
            call->m_args[1] = texture ? reinterpret_cast<uintptr_t>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture))) : 0;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_textures.m_count; i++) {
        if (data->m_textures[i].m_type != type) {
            continue;
        }

        if (this->m_textures[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[i]));
        }

        if (texture) {
            this->m_textures[i] = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture)));

            if (!TextureGetGxTex(this->m_textures[i], 0, nullptr)) {
                this->m_fFlags &= ~2u;
            }
        } else {
            this->m_textures[i] = nullptr;
        }
    }

    for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
        this->m_ribbonEmitters[i]->ReplaceTexture(type, texture);
    }

    for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
        if (data->m_textures[data->m_particles[i].m_textureIndex].m_type == type) {
            this->m_particleEmitters[i]->SetTexture(texture);
        }
    }

    this->m_fFlags &= ~0x10u;
}

// VERIFY: 859397f9 manual
// OFFSET: 0x00825410
void CM2Model::ReplaceParticleColor(uint32_t id, CImVector color0, CImVector color1, CImVector color2) {
    if (this->m_fFlags & 1) {
        M2Data* data = this->m_shared->m_data;

        for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
            if (data->m_particles[i].m_colorIndex == id) {
                this->m_particleEmitters[i]->SetParticleColors(color0, color1, color2);
            }
        }

        return;
    }

    CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

    if (call) {
        call->m_type = MODEL_REPLACE_PARTICLE_COLOR;
        call->m_next = nullptr;
        call->m_time = this->m_scene->m_time;
        call->m_args[0] = id;
        call->m_args[1] = *reinterpret_cast<uint32_t*>(&color0);
        call->m_args[2] = *reinterpret_cast<uint32_t*>(&color1);
        call->m_args[3] = *reinterpret_cast<uint32_t*>(&color2);
        *this->m_modelCallTail = call;
        this->m_modelCallTail = &call->m_next;
    }
}

// VERIFY: bf0dc568 manual
// OFFSET: 0x008254F0
CAaBox CM2Model::GetMaxBoundingBoxPlusAttachments() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    CAaBox box = this->m_shared->m_data->m_bounds.m_extent;

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        CAaBox child = model->GetMaxBoundingBoxPlusAttachments();

        if (child.t.x < child.b.x && child.b.y > child.t.y && child.t.z < child.b.z) {
            continue;
        }

        const CAaBox& extent = this->m_shared->m_data->m_bounds.m_extent;
        float sizeX = child.t.x - child.b.x;
        float sizeY = child.t.y - child.b.y;
        float sizeZ = child.t.z - child.b.z;
        CAaBox grown;
        grown.b.x = extent.b.x - sizeX;
        grown.b.y = extent.b.y - sizeY;
        grown.b.z = extent.b.z - sizeZ;
        grown.t.x = sizeX + extent.t.x;
        grown.t.y = sizeY + extent.t.y;
        grown.t.z = sizeZ + extent.t.z;
        box |= grown;
    }

    return box;
}

// VERIFY: fbee199d manual
// OFFSET: 0x00825660
CAaSphere CM2Model::GetMaxBoundingSpherePlusAttachments() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    const M2Bounds& bounds = this->m_shared->m_data->m_bounds;
    CAaSphere sphere;
    sphere.c.x = (bounds.m_extent.b.x + bounds.m_extent.t.x) * 0.5f;
    sphere.c.y = (bounds.m_extent.t.y + bounds.m_extent.b.y) * 0.5f;
    sphere.c.z = 0.5f * (bounds.m_extent.t.z + bounds.m_extent.b.z);
    sphere.r = bounds.m_radius;

    float radius = 0.0f;

    for (CM2Model* model = this->m_attachList; model; model = model->m_attachNext) {
        CAaSphere child = model->GetMaxBoundingSpherePlusAttachments();

        if (child.r > radius) {
            radius = child.r;
        }
    }

    sphere.r = radius + sphere.r;

    return sphere;
}

// VERIFY: 4e809ba8 manual
// OFFSET: 0x00825750
CAaBox CM2Model::GetCurrentBoundingBox() {
    if (!(this->m_fFlags & 1) || !(this->m_fFlags & 0x8000)) {
        CAaBox result;
        result.b.x = FLT_MAX;
        result.b.y = FLT_MAX;
        result.b.z = FLT_MAX;
        result.t.x = -FLT_MAX;
        result.t.y = -FLT_MAX;
        result.t.z = -FLT_MAX;
        return result;
    }

    M2Data* data = this->m_shared->m_data;
    C44Matrix matrix;

    if (data->m_particles.m_count) {
        const C44Matrix* world = &this->m_worldTransform;

        if (this->m_attachParent) {
            matrix = this->m_matrixF4 * this->m_scene->m_viewInv;
            world = &matrix;
        }

        return CAaBox::Transform(this->m_shared->m_data->m_bounds.m_extent, *world);
    }

    const CAaBox& extent = data->m_sequences[this->m_bones->m_sequence.m_sequenceIndex].m_bounds.m_extent;
    CAaBox box;

    if (extent.IsInverted()) {
        box = extent;
    } else {
        const C44Matrix* world = &this->m_worldTransform;

        if (this->m_attachParent) {
            matrix = this->m_matrixF4 * this->m_scene->m_viewInv;
            world = &matrix;
        }

        box = CAaBox::Transform(extent, *world);
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_particles.m_count; i++) {
        CParticleEmitter2* emitter = this->m_particleEmitters[i];

        if (emitter->m_flags & 0x200) {
            box |= emitter->WorldBounds();
        }
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_ribbons.m_count; i++) {
        box = box | this->m_ribbonEmitters[i]->m_worldBounds;
    }

    return box;
}

// VERIFY: 21639f35 manual
// OFFSET: 0x00825A60
CAaBox CM2Model::GetSplitBodyBoundingBox(uint32_t boneId) {
    if (!(this->m_fFlags & 1) || !(this->m_fFlags & 0x8000)) {
        CAaBox result;
        result.b.x = FLT_MAX;
        result.b.y = FLT_MAX;
        result.t.x = -FLT_MAX;
        result.b.z = FLT_MAX;
        result.t.y = -FLT_MAX;
        result.t.z = -FLT_MAX;
        return result;
    }

    M2Data* data = this->m_shared->m_data;
    C44Matrix matrix;

    if (data->m_particles.m_count) {
        const C44Matrix* world = &this->m_worldTransform;

        if (this->m_attachParent) {
            matrix = this->m_matrixF4 * this->m_scene->m_viewInv;
            world = &matrix;
        }

        return CAaBox::Transform(this->m_shared->m_data->m_bounds.m_extent, *world);
    }

    uint32_t count = data->m_sequences.m_count;

    if (count <= 1 || boneId == 0xFFFFFFFF || boneId >= data->m_boneIndicesById.m_count) {
        return this->GetCurrentBoundingBox();
    }

    uint16_t boneIndex = data->m_boneIndicesById[boneId];

    if (!boneIndex || boneIndex == 0xFFFF || this->m_bones[boneIndex].m_sequence.m_sequenceIndex >= count) {
        return this->GetCurrentBoundingBox();
    }

    CAaBox box = data->m_sequences[this->m_bones->m_sequence.m_sequenceIndex].m_bounds.m_extent;
    box |= this->m_shared->m_data->m_sequences[this->m_bones[boneIndex].m_sequence.m_sequenceIndex].m_bounds.m_extent;

    if (box.IsValid()) {
        const C44Matrix* world = &this->m_worldTransform;

        if (this->m_attachParent) {
            matrix = this->m_matrixF4 * this->m_scene->m_viewInv;
            world = &matrix;
        }

        box = CAaBox::Transform(box, *world);
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_particles.m_count; i++) {
        CParticleEmitter2* emitter = this->m_particleEmitters[i];

        if (emitter->m_flags & 0x200) {
            box |= emitter->WorldBounds();
        }
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_ribbons.m_count; i++) {
        box |= this->m_ribbonEmitters[i]->m_worldBounds;
    }

    return box;
}

// VERIFY: ad274cd1 manual
// OFFSET: 0x00825D70
void CM2Model::UnoptimizeVisibleGeometry() {
    M2ModelOptGeo* optGeo = this->m_optGeo;

    if (!optGeo) {
        return;
    }

    if (optGeo->m_indexBuf) {
        CGxBuf* buf = optGeo->m_indexBuf;

        if (buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }
    }

    if (this->m_optGeo->m_indexPool) {
        g_theGxDevicePtr->PoolDestroy(this->m_optGeo->m_indexPool);
    }

    SMemFree(this->m_optGeo, __FILE__, __LINE__, 0);
    this->m_optGeo = nullptr;
}

// VERIFY: 8a7a94e4 manual
// OFFSET: 0x00825E00
int32_t CM2Model::HasSequence(const M2Data* data, uint32_t id) {
    uint32_t hashCount = data->m_sequenceIdxHashById.m_count;
    uint32_t index;

    if (!hashCount) {
        for (index = 0; index < data->m_sequences.m_count; index++) {
            if (data->m_sequences[index].m_id == id) {
                return index < data->m_sequences.m_count;
            }
        }

        return data->m_sequences.m_count > 0xFFFF;
    }

    uint32_t slot = id % hashCount;
    index = data->m_sequenceIdxHashById[slot];

    if (index == 0xFFFF) {
        return data->m_sequences.m_count > 0xFFFF;
    }

    for (uint32_t step = 1; data->m_sequences[index].m_id != id; step++) {
        slot = (slot + step * step) % data->m_sequenceIdxHashById.m_count;
        index = data->m_sequenceIdxHashById[slot];

        if (index == 0xFFFF) {
            return data->m_sequences.m_count > 0xFFFF;
        }
    }

    return index < data->m_sequences.m_count;
}

// VERIFY: 73b3e0d6 auto
// OFFSET: 0x00825EE0
int32_t CM2Model::HasSequence(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    return this->HasSequence(this->m_shared->m_data, id);
}

// VERIFY: cf5b210a auto
// OFFSET: 0x00825F40
uint32_t CM2Model::ResolveSequenceFallback(uint32_t id) {
    if (id >= ANIM_COUNT) {
        return 0xFFFFFFFF;
    }

    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    int32_t visited[ANIM_COUNT];
    memset(visited, 0, sizeof(visited));

    M2Data* data = this->m_shared->m_data;

    if (this->HasSequence(data, id)) {
        return id;
    }

    while (id < ANIM_COUNT) {
        AnimationDataRec* rec = g_animationDataDB.GetRecord(id);

        if (visited[id] || !rec) {
            break;
        }

        uint32_t fallback = rec->m_fallback;

        if (id == fallback) {
            break;
        }

        visited[id] = 1;
        id = fallback;

        if (this->HasSequence(data, fallback)) {
            return id;
        }
    }

    return 0xFFFFFFFF;
}

// VERIFY: af94bf41 manual
// OFFSET: 0x00826050
int32_t CM2Model::IsSequenceLoaded(uint32_t id) {
    if (id >= 506) {
        return 0;
    }

    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    return this->ResolveSequenceFallback(id) != 0xFFFFFFFF;
}

// VERIFY: 89cf9cd8 manual
// OFFSET: 0x008260C0
uint16_t CM2Model::FindSequenceIndex(const M2Data* data, uint32_t id, int32_t variation) {
    uint32_t hashCount = data->m_sequenceIdxHashById.m_count;
    uint32_t index = 0xFFFF;

    if (hashCount) {
        uint32_t slot = id % hashCount;
        index = data->m_sequenceIdxHashById[slot];

        if (index != 0xFFFF) {
            for (uint32_t step = 1; data->m_sequences[index].m_id != id; step++) {
                slot = (slot + step * step) % hashCount;
                index = data->m_sequenceIdxHashById[slot];

                if (index == 0xFFFF) {
                    break;
                }
            }
        }
    } else {
        for (index = 0; index < data->m_sequences.m_count; index++) {
            if (data->m_sequences[index].m_id == id) {
                break;
            }
        }

        if (index >= data->m_sequences.m_count) {
            index = 0xFFFF;
        }
    }

    uint32_t count = data->m_sequences.m_count;

    while (index < count && variation) {
        index = data->m_sequences[index].m_variationNext;
        variation--;
    }

    if (index >= count || variation) {
        return 0xFFFF;
    }

    return static_cast<uint16_t>(index);
}

// VERIFY: 5546bff4 manual
// OFFSET: 0x008261B0
uint32_t CM2Model::FindSequenceVariationCount(const M2Data* data, uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    uint32_t hashCount = data->m_sequenceIdxHashById.m_count;
    uint32_t index = 0xFFFF;

    if (hashCount) {
        uint32_t slot = id % hashCount;
        index = data->m_sequenceIdxHashById[slot];

        if (index != 0xFFFF) {
            for (uint32_t step = 1; data->m_sequences[index].m_id != id; step++) {
                slot = (slot + step * step) % hashCount;
                index = data->m_sequenceIdxHashById[slot];

                if (index == 0xFFFF) {
                    break;
                }
            }
        }
    } else {
        for (index = 0; index < data->m_sequences.m_count; index++) {
            if (data->m_sequences[index].m_id == id) {
                break;
            }
        }

        if (index >= data->m_sequences.m_count) {
            index = 0xFFFF;
        }
    }

    if (index >= data->m_sequences.m_count) {
        return 0;
    }

    uint32_t count = 1;

    for (uint16_t next = data->m_sequences[index].m_variationNext; next != 0xFFFF; next = data->m_sequences[next].m_variationNext) {
        count++;
    }

    return count;
}

// VERIFY: 339ad9b4 auto
// OFFSET: 0x008262F0
uint32_t CM2Model::GetSequenceVariationCount(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    return this->FindSequenceVariationCount(this->m_shared->m_data, id);
}

// VERIFY: 0ce274db manual
// OFFSET: 0x00826350
M2SequenceFallback* CM2Model::SequenceFallbackById(M2SequenceFallback* result, uint32_t id) {
    M2Data* data = this->m_shared->m_data;
    uint32_t defaultId;

    if (this->HasSequence(data, ANIM_STAND)) {
        defaultId = ANIM_STAND;
    } else if (this->HasSequence(data, ANIM_CLOSED)) {
        defaultId = ANIM_CLOSED;
    } else {
        defaultId = data->m_sequences[0].m_id;
    }

    int32_t visited[ANIM_COUNT];
    memset(visited, 0, sizeof(visited));

    int32_t direction = 1;
    int32_t reversals = 0;

    if (!this->HasSequence(data, id)) {
        uint32_t fallback;

        do {
            AnimationDataRec* rec = g_animationDataDB.GetRecord(id);

            if (id >= ANIM_COUNT || visited[id] || !rec || id == (fallback = rec->m_fallback)) {
                result->m_id = defaultId;
                result->m_mode = 0;
                return result;
            }

            int32_t flags = rec->m_flags;
            visited[id] = 1;
            id = fallback;

            if (flags & 0x10) {
                reversals += direction;
                direction = -direction;
            }

            if (flags & 0x20) {
                reversals += direction;
                direction = 0;
            }
        } while (!this->HasSequence(data, fallback));

        if (direction <= 0) {
            result->m_id = id;

            if (direction >= 0) {
                result->m_mode = (reversals > 0) + 2;
            } else {
                result->m_mode = 1;
            }

            return result;
        }
    }

    result->m_id = id;
    result->m_mode = 0;
    return result;
}

// VERIFY: ba1ac68b auto
// OFFSET: 0x008264B0
int32_t CM2Model::HasBone(uint32_t boneId) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;

    return data->m_bones.m_count && (boneId == 0xFFFFFFFF || (boneId < data->m_boneIndicesById.m_count && data->m_boneIndicesById[boneId] != 0xFFFF));
}

// VERIFY: addba282 manual
// OFFSET: 0x00826530
C3Vector CM2Model::GetBonePivot(uint32_t boneId) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index < data->m_bones.m_count) {
        return data->m_bones[index].m_pivot;
    }

    return C3Vector(0.0f, 0.0f, 0.0f);
}

// VERIFY: 5861eddd manual
// OFFSET: 0x008265E0
void CM2Model::SetBoneFlags(uint32_t boneId, uint32_t flags, uint32_t mask) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_BONE_FLAGS;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = boneId;
            call->m_args[1] = flags;
            call->m_args[2] = mask;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index < data->m_bones.m_count) {
        this->m_bones[index].m_flags = (mask & flags) | (static_cast<uint16_t>(this->m_bones[index].m_flags) & ~mask);
    }
}

// VERIFY: 5b14b345 manual
// OFFSET: 0x008266B0
int32_t CM2Model::GetBoneSequenceInfo(uint32_t boneId, M2BoneSequenceInfo& info) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index >= data->m_bones.m_count) {
        return 0;
    }

    M2ModelBone* bone = &this->m_bones[index];
    info.m_sequenceId = bone->m_sequenceId;
    info.m_variationIndex = bone->m_variationIndex;
    info.m_time = bone->m_sequence.m_startOffset + static_cast<uint32_t>((this->m_scene->m_time - bone->m_sequence.m_startTime) * bone->m_sequence.m_speed);
    info.m_speed = bone->m_sequence.m_speed;
    info.m_startTime = bone->m_sequence.m_startTime;
    info.m_endTime = bone->m_sequence.m_endTime;
    info.m_finished = bone->m_sequence.m_finished;

    uint16_t sequenceIndex = bone->m_sequence.m_sequenceIndex;

    if (sequenceIndex == 0xFFFF || info.m_time < data->m_sequences[sequenceIndex].m_duration) {
        info.m_complete = 0;
    } else {
        info.m_complete = 1;
    }

    return 1;
}

// VERIFY: 271ac2da manual
// OFFSET: 0x008267E0
uint32_t CM2Model::GetBoneSequenceId(uint32_t boneId) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index < data->m_bones.m_count) {
        return this->m_bones[index].m_sequenceId;
    }

    return 0;
}

// VERIFY: 324c5a2e manual
// OFFSET: 0x00826870
uint32_t CM2Model::GetBoneSequenceAnimId(uint32_t boneId, uint32_t* variation) {
    if (variation) {
        *variation = 0;
    }

    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index >= data->m_bones.m_count) {
        return 0xFFFFFFFF;
    }

    uint16_t sequenceIndex = this->m_bones[index].m_sequence.m_sequenceIndex;

    if (sequenceIndex >= data->m_sequences.m_count) {
        return 0xFFFFFFFF;
    }

    if (variation) {
        *variation = data->m_sequences[sequenceIndex].m_variationIndex;
    }

    return data->m_sequences[sequenceIndex].m_id;
}

// VERIFY: a5462ec6 manual
// OFFSET: 0x00826930
float CM2Model::GetBoneSequenceSpeed(uint32_t boneId) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index < data->m_bones.m_count) {
        return this->m_bones[index].m_sequence.m_speed;
    }

    return 1.0f;
}

// VERIFY: 6b99d12c manual
// OFFSET: 0x008269C0
int32_t CM2Model::OnSequenceInterrupted(uint32_t boneId, uint16_t boneIndex) {
    M2SequenceCallback callback = reinterpret_cast<M2SequenceCallback>(this->m_sequenceCallback);
    M2ModelBone* bone = &this->m_bones[boneIndex];

    if (!callback || bone->m_sequence.m_sequenceIndex == 0xFFFF || bone->m_sequence.m_finished) {
        return 1;
    }

    M2CompBone* compBone = &this->m_shared->m_data->m_bones[boneIndex];

    if (compBone->m_boneId == 0xFFFFFFFF && boneIndex) {
        return 1;
    }

    uint32_t id = compBone->m_parentIndex == 0xFFFF ? 0xFFFFFFFF : boneId;
    this->m_refCount++;
    callback(this, id, bone->m_sequenceId, 1, 0, this->m_sequenceCallbackParam);

    int32_t result = this->Release();

    return result ? 1 : result;
}

// VERIFY: a5ebd842 manual
// OFFSET: 0x00826A60
int32_t CM2Model::BoneHasParent(uint32_t boneId) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    return index < data->m_bones.m_count && index && data->m_bones[index].m_parentIndex != 0xFFFF;
}

// VERIFY: 781d6a98 manual
// OFFSET: 0x00826B00
void CM2Model::SetupBoneSequence(uint16_t sequenceIndex, M2SequenceFallback fallback, uint32_t time, float speed, M2ModelBoneSeq* boneSequence) {
    M2Sequence* sequence = &this->m_shared->m_data->m_sequences[sequenceIndex];
    int32_t minRepeat = sequence->m_replay.m_l;
    int32_t random = rand();
    uint32_t startOffset = 0;
    uint32_t repeats = minRepeat + (sequence->m_replay.m_h - minRepeat) * random / 0x8000;
    repeats += repeats == 0;
    uint32_t duration = repeats * sequence->m_duration;

    if (fallback.m_mode == 1 || fallback.m_mode == 3) {
        speed = -speed;
    }

    float invSpeed = 0.0f;

    if (speed < 0.0f) {
        startOffset = duration;
    }

    if (fallback.m_mode == 2 || fallback.m_mode == 3) {
        speed = 0.0f;
    }

    if (fabsf(speed) > 0.0000099999997f) {
        invSpeed = 1.0f / speed;
    }

    CM2Scene* scene = this->m_scene;
    float absInvSpeed = fabsf(invSpeed);
    uint32_t startTime = scene->m_time - static_cast<uint32_t>(time * absInvSpeed);

    if (!((scene->m_flags >> 2) & 1)) {
        startTime++;
    }

    boneSequence->m_sequenceIndex = sequenceIndex;
    boneSequence->m_startTime = startTime;
    boneSequence->m_repeatCount = repeats;
    boneSequence->m_finished = 0;
    boneSequence->m_endTime = static_cast<uint32_t>(absInvSpeed * duration) + startTime;
    boneSequence->m_startOffset = startOffset;
    boneSequence->m_speed = speed;
    boneSequence->m_invSpeed = invSpeed;
}

// VERIFY: 70eb6b82 manual
// OFFSET: 0x00826C40
void CM2Model::SetPrimaryBoneSequence(uint16_t sequenceIndex, uint16_t boneIndex, M2SequenceFallback fallback, uint32_t time, float speed, int32_t blend) {
    M2ModelBone* bone = &this->m_bones[boneIndex];
    M2Sequence* sequence = &this->m_shared->m_data->m_sequences[sequenceIndex];

    if (blend) {
        if (!bone->m_sequence.m_finished || sequenceIndex != bone->m_sequence.m_sequenceIndex) {
            float weight = 0.0f;

            if (bone->m_secondarySequence.m_sequenceIndex != 0xFFFF) {
                float t = (bone->m_blendEndTime - this->m_scene->m_time) * bone->m_invBlendDuration;

                if (t >= 0.0f) {
                    weight = t <= 1.0f ? t * ((3.0f - (t + t)) * t) : 1.0f;
                }
            }

            if (bone->m_secondarySequence.m_sequenceIndex == 0xFFFF || weight * bone->m_blendWeightMax <= 0.5f) {
                bone->m_secondarySequence = bone->m_sequence;
                bone->m_blendEndTime = sequence->m_blendTime + this->m_scene->m_time;
                bone->m_invBlendDuration = sequence->m_blendTime ? 1.0f / sequence->m_blendTime : 1.0f;
                bone->m_blendWeightMax = 1.0f;
            }
        }
    } else {
        bone->m_secondarySequence.m_sequenceIndex = 0xFFFF;
    }

    this->SetupBoneSequence(sequenceIndex, fallback, time, speed, &bone->m_sequence);

    if (bone->m_sequence.m_startTime == bone->m_sequence.m_endTime || ((sequence->m_flags & 1) && static_cast<int32_t>(bone->m_sequence.m_endTime - this->m_scene->m_time) <= 0)) {
        bone->m_sequence.m_finished = 1;
    }

    if (!bone->m_animListPrev) {
        bone->m_animListPrev = &this->m_animListHead;
        bone->m_animListNext = this->m_animListHead;

        if (this->m_animListHead != 0xFFFF) {
            this->m_bones[this->m_animListHead].m_animListPrev = &bone->m_animListNext;
        }

        this->m_animListHead = boneIndex;
    }
}

// VERIFY: bac43143 manual
// OFFSET: 0x00826DD0
void CM2Model::SetSecondaryBoneSequence(uint16_t sequenceIndex, uint16_t boneIndex, M2SequenceFallback fallback, uint32_t time, float speed) {
    M2ModelBone* bone = &this->m_bones[boneIndex];
    uint32_t duration = this->m_shared->m_data->m_sequences[sequenceIndex].m_duration;

    bone->m_blendEndTime = duration + this->m_scene->m_time;
    bone->m_invBlendDuration = duration ? 1.0f / duration : 1.0f;
    bone->m_blendWeightMax = 0.75f;
    this->SetupBoneSequence(sequenceIndex, fallback, time, speed, &bone->m_secondarySequence);
}

// VERIFY: ddf01210 manual
// OFFSET: 0x00826E60
void CM2Model::PickRandomSequenceVariation(uint32_t* variation, uint16_t* sequenceIndex) {
    *variation = 0;

    uint32_t count = 0;
    int32_t random = rand();
    uint16_t index = *sequenceIndex;

    while (index != 0xFFFF) {
        M2Sequence* sequence = &this->m_shared->m_data->m_sequences[index];

        if (static_cast<uint32_t>(random) < sequence->m_frequency) {
            *sequenceIndex = index;
            *variation = count;
            return;
        }

        index = sequence->m_variationNext;
        random -= sequence->m_frequency;
        count++;
    }
}

// VERIFY: b25072e9 manual
// OFFSET: 0x00826ED0
void CM2Model::SetBoneSequenceTime(uint32_t boneId, uint32_t time) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_BONE_SEQUENCE_TIME;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = boneId;
            call->m_args[1] = time;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFF;
    }

    if (index >= data->m_bones.m_count) {
        return;
    }

    M2ModelBone* bone = &this->m_bones[index];
    uint16_t sequenceIndex = bone->m_sequence.m_sequenceIndex;

    if (sequenceIndex == 0xFFFF) {
        return;
    }

    float absInvSpeed = fabsf(bone->m_sequence.m_invSpeed);
    uint32_t duration = bone->m_sequence.m_repeatCount * data->m_sequences[sequenceIndex].m_duration;
    uint32_t startTime = this->m_scene->m_time - static_cast<uint32_t>(time * absInvSpeed);
    bone->m_sequence.m_startTime = startTime;
    bone->m_sequence.m_endTime = startTime + static_cast<uint32_t>(absInvSpeed * duration);
}

// VERIFY: 1344bdcc manual
// OFFSET: 0x00827000
void CM2Model::SetBoneSequenceSpeed(uint32_t boneId, float speed) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_BONE_SEQUENCE_SPEED;
            call->m_next = nullptr;
            *reinterpret_cast<float*>(&call->m_args[1]) = speed;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = boneId;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFF;
    }

    if (index >= data->m_bones.m_count) {
        return;
    }

    M2ModelBone* bone = &this->m_bones[index];
    uint16_t sequenceIndex = bone->m_sequence.m_sequenceIndex;

    if (sequenceIndex == 0xFFFF) {
        return;
    }

    uint32_t now = this->m_scene->m_time;
    uint32_t duration = bone->m_sequence.m_repeatCount * data->m_sequences[sequenceIndex].m_duration;
    float invSpeed = fabsf(speed) <= 0.0000099999997f ? 0.0f : 1.0f / speed;
    float absInvSpeed = fabsf(invSpeed);
    uint32_t startTime = static_cast<uint32_t>(now - (bone->m_sequence.m_startOffset + (now - bone->m_sequence.m_startTime) * bone->m_sequence.m_speed) * absInvSpeed);
    bone->m_sequence.m_startTime = startTime;
    bone->m_sequence.m_endTime = static_cast<uint32_t>(startTime + absInvSpeed * duration);
    bone->m_sequence.m_speed = speed;
    bone->m_sequence.m_invSpeed = invSpeed;
}

// VERIFY: 13ebc48b manual
// OFFSET: 0x00827190
void CM2Model::LoadSequence(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_LOAD_SEQUENCE;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = id;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    M2SequenceFallback fallback;
    this->SequenceFallbackById(&fallback, id);

    uint32_t key = fallback.m_id | (fallback.m_mode << 16);
    M2Data* data = this->m_shared->m_data;
    uint32_t hashCount = data->m_sequenceIdxHashById.m_count;
    uint32_t index = 0xFFFFFFFF;

    if (hashCount) {
        uint32_t slot = key % hashCount;
        uint32_t entry = data->m_sequenceIdxHashById[slot];

        if (entry != 0xFFFF) {
            for (uint32_t step = 1; data->m_sequences[entry].m_id != key; step++) {
                slot = (slot + step * step) % data->m_sequenceIdxHashById.m_count;
                entry = data->m_sequenceIdxHashById[slot];

                if (entry == 0xFFFF) {
                    break;
                }
            }

            if (entry != 0xFFFF) {
                index = entry;
            }
        }
    } else {
        for (uint32_t i = 0; i < data->m_sequences.m_count; i++) {
            if (data->m_sequences[i].m_id == key) {
                index = i;
                break;
            }
        }
    }

    while (index < data->m_sequences.m_count) {
        M2Sequence* sequence = &data->m_sequences[index];

        if (!(sequence->m_flags & 0x30)) {
            this->m_shared->LoadLowPrioritySequence(static_cast<uint16_t>(index));
        }

        index = sequence->m_variationNext;
    }
}

// VERIFY: 0b075a87 manual
// OFFSET: 0x008272F0
void CM2Model::SetBoneProceduralTransform(uint32_t boneId, const C44Matrix& matrix) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_BONE_PROCEDURAL_TRANSFORM;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = boneId;
            memcpy(&call->m_args[1], &matrix, sizeof(C44Matrix));
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index;

    if (boneId == 0xFFFFFFFF) {
        index = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        index = data->m_boneIndicesById[boneId];
    } else {
        index = 0xFFFFFFFF;
    }

    if (index >= data->m_bones.m_count) {
        return;
    }

    M2ModelBone* bone = &this->m_bones[index];

    if (!bone->m_proceduralTransform) {
        bone->m_proceduralTransform = static_cast<C44Matrix*>(SIMD::AllocA16(sizeof(C44Matrix), __FILE__, __LINE__));
    }

    *bone->m_proceduralTransform = matrix;
}

// VERIFY: 9445f766 auto
// OFFSET: 0x008273D0
int32_t CM2Model::HasAttachment(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;

    if (id < data->m_attachmentIndicesById.m_count) {
        return data->m_attachmentIndicesById[id] < data->m_attachments.m_count;
    }

    return data->m_attachments.m_count > 0xFFFF;
}

// VERIFY: f01becc2 manual
// OFFSET: 0x00827460
C3Vector CM2Model::GetAttachmentPivot(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t index = id < data->m_attachmentIndicesById.m_count ? data->m_attachmentIndicesById[id] : 0xFFFFFFFF;

    return data->m_attachments[index].m_position;
}

// VERIFY: dca38176 manual
// OFFSET: 0x008274F0
void CM2Model::DetachFromParent() {
    if (this->m_attachPrev) {
        *this->m_attachPrev = this->m_attachNext;
    }

    if (this->m_attachNext) {
        this->m_attachNext->m_attachPrev = this->m_attachPrev;
    }

    this->m_fFlags &= ~0x40000u;

    bool release = this->m_refCount-- == 1;
    this->m_attachPrev = nullptr;
    this->m_attachNext = nullptr;
    this->m_attachParent = nullptr;
    this->m_attachmentId = 0xFFFFFFFF;
    this->m_frameOfReference = nullptr;

    if (release) {
        g_modelPool->Delete(this);
    }
}

// VERIFY: 9901bc0a manual
// OFFSET: 0x00827560
void CM2Model::DetachAllChildrenById(uint32_t id) {
    CM2Model* model = this->m_attachList;

    while (model) {
        CM2Model* next = model->m_attachNext;

        if (model->m_attachmentId == id) {
            if (model->m_attachPrev) {
                *model->m_attachPrev = model->m_attachNext;
            }

            if (model->m_attachNext) {
                model->m_attachNext->m_attachPrev = model->m_attachPrev;
            }

            model->m_fFlags &= ~0x40000u;

            bool release = model->m_refCount-- == 1;
            model->m_attachPrev = nullptr;
            model->m_attachNext = nullptr;
            model->m_attachParent = nullptr;
            model->m_attachmentId = 0xFFFFFFFF;
            model->m_frameOfReference = nullptr;

            if (release) {
                g_modelPool->Delete(model);
            }
        }

        model = next;
    }
}

// VERIFY: ad2a989b manual
// OFFSET: 0x008275F0
int32_t CM2Model::HasEvent(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_events.m_count; i++) {
        if (data->m_events[i].m_eventId == id) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 9984810c manual
// OFFSET: 0x00827670
int32_t CM2Model::FindEvent(uint32_t id, C3Vector** position, uint16_t* bone) {
    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_events.m_count; i++) {
        if (data->m_events[i].m_eventId == id) {
            *position = &this->m_shared->m_data->m_events[i].m_position;
            *bone = this->m_shared->m_data->m_events[i].m_bone;
            return 1;
        }
    }

    return 0;
}

// VERIFY: 3c23ff24 manual
// OFFSET: 0x008276F0
C3Vector CM2Model::GetEventPivot(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    C3Vector* position;
    uint16_t bone;

    if (!this->FindEvent(id, &position, &bone)) {
        return C3Vector(0.0f, 0.0f, 0.0f);
    }

    return *position;
}

// VERIFY: 64018054 manual
// OFFSET: 0x00827780
uint32_t CM2Model::GetEventSequenceTime(uint32_t sequenceId, uint32_t eventId) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint32_t hashCount = data->m_sequenceIdxHashById.m_count;
    uint32_t index = 0xFFFF;

    if (hashCount) {
        uint32_t slot = sequenceId % hashCount;
        index = data->m_sequenceIdxHashById[slot];

        if (index != 0xFFFF) {
            for (uint32_t step = 1; data->m_sequences[index].m_id != sequenceId; step++) {
                slot = (slot + step * step) % hashCount;
                index = data->m_sequenceIdxHashById[slot];

                if (index == 0xFFFF) {
                    break;
                }
            }
        }
    } else {
        for (index = 0; index < data->m_sequences.m_count; index++) {
            if (data->m_sequences[index].m_id == sequenceId) {
                break;
            }
        }

        if (index >= data->m_sequences.m_count) {
            index = 0xFFFF;
        }
    }

    if (index >= data->m_sequences.m_count) {
        return 0;
    }

    for (uint32_t i = 0; i < data->m_events.m_count; i++) {
        M2Event* event = &data->m_events[i];

        if (event->m_eventId == eventId) {
            return event ? event->m_enabled.m_sequenceTimes[index][0] : 0;
        }
    }

    return 0;
}

// VERIFY: db72d94c manual
// OFFSET: 0x008278E0
int32_t CM2Model::HasCamera(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    return id < data->m_cameraIndicesById.m_count && data->m_cameraIndicesById[id] != 0xFFFF;
}

// VERIFY: 37987c84 manual
// OFFSET: 0x00827960
CCamera* CM2Model::GetCameraById(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;

    if (id >= data->m_cameraIndicesById.m_count) {
        return nullptr;
    }

    uint16_t index = data->m_cameraIndicesById[id];

    if (index == 0xFFFF) {
        return nullptr;
    }

    return this->m_cameras[index].m_camera;
}

// VERIFY: be32ea2b manual
// OFFSET: 0x008279F0
void CM2Model::SetEmittersEnabled(int32_t enabled) {
    if (this->m_fFlags & 1) {
        uint32_t count = this->m_shared->m_data->m_particles.m_count;

        for (uint32_t i = 0; i < count; i++) {
            CParticleEmitter2* emitter = this->m_particleEmitters[i];

            if (enabled) {
                emitter->m_flags |= 2;
            } else {
                emitter->m_flags &= ~2u;
            }
        }
    } else {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_EMITTERS_ENABLED;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = enabled;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }
    }
}

// VERIFY: 962d124c manual
// OFFSET: 0x00827A90
uint32_t CM2Model::GetNumGeometryBatches() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2ModelOptGeo* optGeo = this->m_optGeo;
    uint32_t visible = 0;
    M2SkinProfile* skin = this->m_shared->m_skinData;
    bool optimized = optGeo != nullptr;
    uint32_t count = optimized ? optGeo->m_batchCount : skin->m_batches.m_count;

    for (uint32_t i = 0; i < count; i++) {
        M2Batch* batch;

        if (optimized) {
            batch = &this->m_optGeo->m_batches[i];
        } else {
            batch = &skin->m_batches[i];

            if (!this->m_skinSections[batch->m_skinSectionIndex]) {
                continue;
            }
        }

        if (batch->m_shader == 0x8000) {
            continue;
        }

        float alpha = this->m_alpha;
        M2Data* data = this->m_shared->m_data;

        if (batch->m_colorIndex < data->m_colors.m_count) {
            alpha *= this->m_colors[batch->m_colorIndex].m_alphaTrack.m_currentValue;
        }

        if (batch->m_textureCount) {
            alpha *= this->m_textureWeights[data->m_textureWeightCombos[batch->m_textureWeightComboIndex]].m_currentValue;
        }

        if (alpha >= 0.000099999997f) {
            visible++;
        }
    }

    return visible;
}

// VERIFY: 71314806 manual
// OFFSET: 0x00827E30
uint32_t CM2Model::GetNumCombinableDoodadBatches() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2ModelOptGeo* optGeo = this->m_optGeo;
    M2SkinProfile* skin = this->m_shared->m_skinData;
    uint32_t combinable = 0;
    bool optimized = optGeo != nullptr;
    uint32_t count = optimized ? optGeo->m_batchCount : skin->m_batches.m_count;

    if (!count) {
        return 0;
    }

    for (uint32_t i = 0; i < count; i++) {
        M2Batch* batch;

        if (optimized) {
            batch = &this->m_optGeo->m_batches[i];
        } else {
            batch = &skin->m_batches[i];

            if (!this->m_skinSections[batch->m_skinSectionIndex]) {
                continue;
            }
        }

        if (batch->m_shader == 0x8000) {
            continue;
        }

        float alpha = this->m_alpha;
        M2Data* data = this->m_shared->m_data;

        if (batch->m_colorIndex < data->m_colors.m_count) {
            alpha *= this->m_colors[batch->m_colorIndex].m_alphaTrack.m_currentValue;
        }

        if (batch->m_textureCount) {
            alpha *= this->m_textureWeights[data->m_textureWeightCombos[batch->m_textureWeightComboIndex]].m_currentValue;
        }

        if (alpha >= 0.000099999997f && this->IsBatchDoodadCompatible(batch)) {
            combinable++;
        }
    }

    return combinable;
}

// VERIFY: d7b7ec48 manual
// OFFSET: 0x00827F70
uint32_t CM2Model::GetNumPrimitives() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2ModelOptGeo* optGeo = this->m_optGeo;
    M2SkinProfile* skin = this->m_shared->m_skinData;
    uint32_t primitives = 0;
    bool optimized = optGeo != nullptr;
    uint32_t count = optimized ? optGeo->m_batchCount : skin->m_batches.m_count;

    for (uint32_t i = 0; i < count; i++) {
        M2Batch* batch;
        M2SkinSection* section;

        if (optimized) {
            batch = &this->m_optGeo->m_batches[i];
            section = &this->m_optGeo->m_skinSections[batch->m_skinSectionIndex];
        } else {
            batch = &skin->m_batches[i];
            section = &this->m_shared->m_skinSections[batch->m_skinSectionIndex];

            if (!this->m_skinSections[batch->m_skinSectionIndex]) {
                continue;
            }
        }

        if (batch->m_shader == 0x8000) {
            continue;
        }

        float alpha = this->m_alpha;
        M2Data* data = this->m_shared->m_data;

        if (batch->m_colorIndex < data->m_colors.m_count) {
            alpha *= this->m_colors[batch->m_colorIndex].m_alphaTrack.m_currentValue;
        }

        if (batch->m_textureCount) {
            alpha *= this->m_textureWeights[this->m_shared->m_data->m_textureWeightCombos[batch->m_textureWeightComboIndex]].m_currentValue;
        }

        if (alpha >= 0.000099999997f) {
            primitives += section->m_indexCount / 3;
        }
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_ribbons.m_count; i++) {
        if (this->m_ribbons[i].m_visibilityTrack.m_currentValue) {
            primitives += this->m_ribbonEmitters[i]->GetNumPrimitives();
        }
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_particles.m_count; i++) {
        if (this->m_particles[i].m_active && this->m_effectiveAlpha >= 0.000099999997f) {
            primitives += this->m_particleEmitters[i]->GetNumPrimitives();
        }
    }

    return primitives;
}

// VERIFY: 89dd83b7 manual
// OFFSET: 0x008284D0
void CM2Model::FindKey(M2ModelBoneSeq* sequence, const M2TrackBase& track, uint32_t& key0, uint32_t& key1, float& ratio) {
    uint32_t time = sequence->m_currentTime;
    uint32_t animIndex = sequence->m_animIndex;
    uint32_t animCount = track.m_sequenceTimes.m_count;

    if (!animCount) {
        key1 = 0;
        key0 = 0;
        ratio = 0.0f;
        return;
    }

    if (track.m_loopIndex != 0xFFFF) {
        time = this->m_loops[track.m_loopIndex];
        animIndex = 0;
    } else if (animIndex >= animCount) {
        animIndex = 0;
    }

    M2Array<uint32_t>& times = track.m_sequenceTimes[animIndex];
    uint32_t count = times.m_count;

    if (count <= 1) {
        key1 = 0;
        key0 = 0;
        ratio = 0.0f;
        return;
    }

    if (key0 >= count) {
        key0 = 0;
    }

    uint32_t* keys = times.Data();
    uint32_t key = key0;
    uint32_t delta = time - keys[key];

    if (delta < 500) {
        while (key < count - 1 && keys[key + 1] <= time) {
            key++;
        }
    } else if (delta >= 0xFFFFFE0C) {
        while (key && keys[key] > time) {
            key--;
        }
    } else {
        key = 0;

        if (time < 500) {
            while (key < count - 1 && keys[key + 1] <= time) {
                key++;
            }
        } else {
            uint32_t high = count;

            while (key < high) {
                uint32_t mid = (high + key) >> 1;

                if (time < keys[mid]) {
                    // When time is before the first key, mid - 1 underflows high to 0xFFFFFFFF and the next probe reads far out of bounds.
                    if (mid == 0) {
                        break;
                    }

                    high = mid - 1;
                } else {
                    key = mid + 1;

                    if (key >= count || time < keys[key]) {
                        key = mid;
                        break;
                    }
                }
            }
        }
    }

    if (key + 1 >= times.m_count) {
        key1 = key;
        key0 = key;
        ratio = 0.0f;
    } else {
        key0 = key;
        key1 = key + 1;
        ratio = static_cast<float>(time - keys[key]) / static_cast<float>(keys[key + 1] - keys[key]);
    }
}

// VERIFY: 3e6ce0d8 auto
// OFFSET: 0x00828680
template <>
void M2AnimateTrack<M2CompQuat, C4Quaternion>(CM2Model* model, M2ModelBone* bone, const M2Track<M2CompQuat>& track, M2ModelTrack<C4Quaternion>& modelTrack, const C4Quaternion& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<M2CompQuat>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];

    if (keys.m_count) {
        uint32_t key1;
        float ratio;
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            M2CompQuat& key = keys[modelTrack.m_currentKey];
            modelTrack.m_currentValue.x = key.m_values[0] * 0.000030518044f - 1.0f;
            modelTrack.m_currentValue.y = key.m_values[1] * 0.000030518044f - 1.0f;
            modelTrack.m_currentValue.z = key.m_values[2] * 0.000030518044f - 1.0f;
            modelTrack.m_currentValue.w = key.m_values[3] * 0.000030518044f - 1.0f;
            return;
        }

        M2CompQuat& k0 = keys[modelTrack.m_currentKey];
        M2CompQuat& k1 = keys[key1];
        C4Quaternion q0;
        q0.x = k0.m_values[0] * 0.000030518044f - 1.0f;
        q0.y = k0.m_values[1] * 0.000030518044f - 1.0f;
        q0.z = k0.m_values[2] * 0.000030518044f - 1.0f;
        q0.w = k0.m_values[3] * 0.000030518044f - 1.0f;
        C4Quaternion q1;
        q1.x = k1.m_values[0] * 0.000030518044f - 1.0f;
        q1.y = k1.m_values[1] * 0.000030518044f - 1.0f;
        q1.z = k1.m_values[2] * 0.000030518044f - 1.0f;
        q1.w = k1.m_values[3] * 0.000030518044f - 1.0f;
        modelTrack.m_currentValue = C4Quaternion::Nlerp(ratio, q0, q1);
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<M2CompQuat>& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        C4Quaternion blend;

        if (blendKeys.m_count) {
            uint32_t key1;
            float ratio;
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            M2CompQuat& k0 = blendKeys[modelTrack.m_secondaryKey];
            M2CompQuat& k1 = blendKeys[key1];
            C4Quaternion q0;
            q0.x = k0.m_values[0] * 0.000030518044f - 1.0f;
            q0.y = k0.m_values[1] * 0.000030518044f - 1.0f;
            q0.z = k0.m_values[2] * 0.000030518044f - 1.0f;
            q0.w = k0.m_values[3] * 0.000030518044f - 1.0f;
            C4Quaternion q1;
            q1.x = k1.m_values[0] * 0.000030518044f - 1.0f;
            q1.y = k1.m_values[1] * 0.000030518044f - 1.0f;
            q1.z = k1.m_values[2] * 0.000030518044f - 1.0f;
            q1.w = k1.m_values[3] * 0.000030518044f - 1.0f;
            blend = C4Quaternion::Nlerp(ratio, q0, q1);
        } else {
            blend = defaultValue;
        }

        modelTrack.m_currentValue = C4Quaternion::Slerp(bone->m_blendFactor, modelTrack.m_currentValue, blend);
    }
}

// VERIFY: 4a40646d manual
// OFFSET: 0x00828A00
void CM2Model::AnimateST() {
    if (!(this->m_fFlags & 1)) {
        return;
    }

    M2Data* data = this->m_shared->m_data;
    CM2Model* parent = this->m_attachParent;

    if (!parent || (this->m_fFlags ^= (this->m_fFlags ^ parent->m_fFlags) & 0x8000, parent->m_flags & 1)) {
        this->m_currentLighting = &this->m_lighting;
    } else {
        this->m_currentLighting = parent->m_currentLighting;
    }

    if (!this->m_currentLighting) {
        this->m_currentLighting = &this->m_lighting;
    }

    for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
        M2ModelLight* light = &this->m_lights[i];
        M2Light* lightData = &data->m_lights[i];
        int32_t visible;

        if (light->m_enabled && light->m_visibilityTrack.m_currentValue) {
            visible = 1;

            if (lightData->m_type == 1) {
                light->m_light.SetPosition(lightData->m_position * this->m_boneMatrices[lightData->m_bone] * this->m_scene->m_viewInv);
            } else {
                C44Matrix& boneMatrix = this->m_boneMatrices[lightData->m_bone];
                float x = -boneMatrix.c0;
                float y = -boneMatrix.c1;
                float z = -boneMatrix.c2;
                C44Matrix& view = this->m_scene->m_viewInv;
                float dy = view.c1 * z + view.b1 * y + view.a1 * x;
                float dz = view.c2 * z + view.b2 * y + view.a2 * x;
                float dx = x * view.a0 + (y * view.b0 + z * view.c0);
                light->m_light.SetDirection(C3Vector(dx, dy, dz));
            }
        } else {
            visible = 0;
        }

        light->m_light.SetVisible(visible);
        light->m_light.m_stamp = this->m_scene->m_frameStamp;
    }

    for (uint32_t i = 0; i < data->m_cameras.m_count; i++) {
        M2ModelCamera* camera = &this->m_cameras[i];
        M2Camera* cameraData = &data->m_cameras[i];

        C3Vector position;
        position.x = camera->m_positionTrack.m_currentValue.x + cameraData->m_positionBase.x;
        position.y = cameraData->m_positionBase.y + camera->m_positionTrack.m_currentValue.y;
        position.z = cameraData->m_positionBase.z + camera->m_positionTrack.m_currentValue.z;
        CameraSetCoord(camera->m_camera, 7, position * this->m_matrixF4 * this->m_scene->m_viewInv);

        C3Vector target;
        target.x = cameraData->m_targetPositionBase.x + camera->m_targetTrack.m_currentValue.x;
        target.y = camera->m_targetTrack.m_currentValue.y + cameraData->m_targetPositionBase.y;
        target.z = camera->m_targetTrack.m_currentValue.z + cameraData->m_targetPositionBase.z;
        CameraSetCoord(camera->m_camera, 8, target * this->m_matrixF4 * this->m_scene->m_viewInv);

        CameraSetFloat(camera->m_camera, 5, camera->m_rollTrack.m_currentValue);
    }

    uint32_t time = this->m_scene->m_time;
    uint32_t elapsed = time - this->m_lastEmitterTime;
    float deltaTime = elapsed * 0.001f;
    this->m_lastEmitterTime = time;

    for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
        M2Ribbon* ribbonData = &data->m_ribbons[i];
        CRibbonEmitter* emitter = this->m_ribbonEmitters[i];
        M2ModelRibbon* ribbon = &this->m_ribbons[i];

        if (ribbonData->m_colorTrack.m_sequenceTimes.m_count > 1 || (ribbonData->m_colorTrack.m_sequenceTimes.m_count == 1 && ribbonData->m_colorTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetColor(ribbon->m_colorTrack.m_currentValue.x, ribbon->m_colorTrack.m_currentValue.y, ribbon->m_colorTrack.m_currentValue.z);
        }

        emitter->SetAlpha(ribbon->m_alphaTrack.m_currentValue * this->m_effectiveAlpha);

        if (ribbonData->m_heightAboveTrack.m_sequenceTimes.m_count > 1 || (ribbonData->m_heightAboveTrack.m_sequenceTimes.m_count == 1 && ribbonData->m_heightAboveTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetAbove(ribbon->m_heightAboveTrack.m_currentValue);
        }

        if (ribbonData->m_heightBelowTrack.m_sequenceTimes.m_count > 1 || (ribbonData->m_heightBelowTrack.m_sequenceTimes.m_count == 1 && ribbonData->m_heightBelowTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetBelow(ribbon->m_heightBelowTrack.m_currentValue);
        }

        if (ribbonData->m_textureSlotTrack.m_sequenceTimes.m_count > 1 || (ribbonData->m_textureSlotTrack.m_sequenceTimes.m_count == 1 && ribbonData->m_textureSlotTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetTexSlot(ribbon->m_textureSlotTrack.m_currentValue);
        }

        C44Matrix transform = this->m_boneMatrices[ribbonData->m_boneIndex];
        transform.Translate(ribbonData->m_position);
        transform *= this->m_scene->m_viewInv;

        emitter->SetDataEnabled(ribbon->m_visibilityTrack.m_currentValue != 0);

        if (this->m_fFlags & 0x8000) {
            emitter->SetPos(transform, C3Vector(0.0f, 0.0f, 0.0f), static_cast<const C44Matrix*>(this->m_frameOfReference));
            emitter->Update(deltaTime, ribbon->m_visibilityTrack.m_currentValue == 0);
        }
    }

    for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
        this->AnimateParticleST(deltaTime, i);
    }

    if (this->m_fFlags & 8) {
        CM2Model** head = &this->m_scene->m_drawList;
        this->m_drawPrev = head;
        this->m_drawNext = *head;
        *head = this;

        if (this->m_drawNext) {
            this->m_drawNext->m_drawPrev = &this->m_drawNext;
        }
    }

    if ((this->m_fFlags & 0x400) && (this->m_fFlags & 0x10000)) {
        CM2Model** head = &this->m_scene->m_particleList;
        this->m_particlePrev = head;
        this->m_particleNext = *head;
        *head = this;

        if (this->m_particleNext) {
            this->m_particleNext->m_particlePrev = &this->m_particleNext;
        }
    }

    for (CM2Model* child = this->m_attachList; child; child = child->m_attachNext) {
        bool hidden;

        if (child->m_attachmentIndex == 0xFFFF) {
            hidden = !(child->m_fFlags & 0x40000);
        } else {
            hidden = this->m_attachments[child->m_attachmentIndex].m_currentValue == 0;
        }

        if (!hidden) {
            child->AnimateST();
        }
    }

    if (this->m_effectiveAlpha == 1.0f) {
        this->m_uint90 = 1;
    }
}

// VERIFY: 852eaff5 manual
// OFFSET: 0x00828F90
int32_t CM2Model::SetIndices() {
    CGxBuf* buf = this->m_optGeo->m_indexBuf;

    if (!buf->m_hasData || !buf->m_valid) {
        uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(buf));

        if (!indices) {
            return 0;
        }

        CM2Shared* shared = this->m_shared;
        M2SkinProfile* skin = shared->m_skinData;
        uint32_t copyIndices = shared->m_cache->m_flags & 8;
        int32_t singleBone;

        if (shared->m_data->m_bones.m_count != 1 || (singleBone = 1, !(this->m_shared->m_cache->m_flags & 0x40))) {
            singleBone = 0;
        }

        for (uint32_t i = 0; i < this->m_optGeo->m_skinSectionCount; i++) {
            M2BatchRange* range = &this->m_optGeo->m_batchRanges[this->m_optGeo->m_skinSections[i].m_skinSectionId];
            uint16_t vertexBase = 0;

            for (uint32_t b = range->m_first; b <= range->m_last; b++) {
                uint16_t sectionIndex = skin->m_batches[b].m_skinSectionIndex;

                if (!this->m_skinSections[sectionIndex]) {
                    continue;
                }

                M2SkinSection* section = &skin->m_skinSections[sectionIndex];

                if (copyIndices || singleBone) {
                    memcpy(indices, &skin->m_indices[section->m_indexStart], section->m_indexCount * sizeof(uint16_t));
                } else {
                    for (uint32_t j = 0; j < section->m_indexCount; j++) {
                        indices[j] = vertexBase + skin->m_indices[j + section->m_indexStart] - section->m_vertexStart;
                    }
                }

                vertexBase += section->m_vertexCount;
                indices += section->m_indexCount;
            }
        }

        buf = this->m_optGeo->m_indexBuf;
        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;
    }

    g_theGxDevicePtr->PrimIndexPtr(this->m_optGeo->m_indexBuf);
    return 1;
}

// VERIFY: d0a96bb9 manual
// OFFSET: 0x00829160
int32_t CM2Model::SetVertices(int32_t optimized, M2SkinSection* section, int32_t texCoord) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 32, section->m_vertexCount);
    uint8_t* vertices = reinterpret_cast<uint8_t*>(g_theGxDevicePtr->BufLock(buf));

    if (!vertices) {
        return 0;
    }

    if (optimized) {
        M2SkinProfile* skin = this->m_shared->m_skinData;
        M2BatchRange* range = &this->m_optGeo->m_batchRanges[section->m_skinSectionId];

        for (uint32_t b = range->m_first; b <= range->m_last; b++) {
            uint16_t sectionIndex = skin->m_batches[b].m_skinSectionIndex;

            if (this->m_skinSections[sectionIndex]) {
                M2SkinSection* batchSection = &skin->m_skinSections[sectionIndex];
                CM2Model::s_transformVertices[batchSection->m_boneInfluences](this, batchSection, vertices, texCoord);
                vertices += 32 * batchSection->m_vertexCount;
            }
        }
    } else {
        CM2Model::s_transformVertices[section->m_boneInfluences](this, section, vertices, texCoord);
    }

    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
    GxPrimVertexPtr(buf, GxVBF_PNT);
    return 1;
}

// VERIFY: 78ff9dca manual
// OFFSET: 0x008292A0
int32_t CM2Model::GetRegionBounds(M2SkinSection* section, CAaBox& box, C44Matrix& matrix) {
    if (section->m_vertexCount < 4) {
        return 0;
    }

    M2Data* data = this->m_shared->m_data;
    M2SkinProfile* skin = this->m_shared->m_skinData;
    C44Matrix transform;
    uint32_t weights = 0;
    uint32_t indices = 0;
    C3Vector positions[4];
    C2Vector texCoords[4];

    box.b = C3Vector(FLT_MAX, FLT_MAX, FLT_MAX);
    box.t = C3Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (uint32_t i = 0; i < section->m_vertexCount; i++) {
        M2Vertex* vertex = &data->m_vertices[skin->m_vertices[i + section->m_vertexStart]];

        if (*reinterpret_cast<uint32_t*>(&vertex->m_weights) != weights || *reinterpret_cast<uint32_t*>(&vertex->m_indices) != indices) {
            weights = *reinterpret_cast<uint32_t*>(&vertex->m_weights);
            indices = *reinterpret_cast<uint32_t*>(&vertex->m_indices);

            if (CM2Cache::s_processorFeatures & 4) {
                M2BlendBoneMatricesSSE(this->m_boneMatrices, vertex->m_weights, vertex->m_indices, &transform);
            } else {
                M2BlendBoneMatrices(this->m_boneMatrices, vertex->m_weights, vertex->m_indices, &transform);
            }

            transform *= this->m_scene->m_viewInv;
        }

        C3Vector position = vertex->m_position * transform;

        if (position.x < box.b.x) {
            box.b.x = position.x;
        }

        if (position.y < box.b.y) {
            box.b.y = position.y;
        }

        if (position.z < box.b.z) {
            box.b.z = position.z;
        }

        if (box.t.x < position.x) {
            box.t.x = position.x;
        }

        if (box.t.y < position.y) {
            box.t.y = position.y;
        }

        if (position.z > box.t.z) {
            box.t.z = position.z;
        }

        if (i < 3) {
            C44Matrix& view = this->m_scene->m_viewInv;
            positions[i].x = position.x - view.d0;
            positions[i].y = position.y - view.d1;
            positions[i].z = position.z - view.d2;
            texCoords[i] = vertex->m_texcoord[0];
        }
    }

    uint32_t low = positions[0].y < positions[1].y ? 0 : 1;

    if (positions[2].y <= positions[low].y) {
        low = 2;
    }

    uint32_t high = positions[1].y >= positions[0].y ? 1 : 0;

    if (positions[2].y >= positions[high].y) {
        high = 2;
    }

    uint32_t mid = 3 - high - low;

    if (fabsf(positions[low].y - positions[high].y) < 0.00000023841858f) {
        return 0;
    }

    float t = (positions[mid].y - positions[low].y) / (positions[high].y - positions[low].y);
    positions[3].x = (positions[high].x - positions[low].x) * t + positions[low].x;
    positions[3].y = (positions[high].y - positions[low].y) * t + positions[low].y;
    positions[3].z = (positions[high].z - positions[low].z) * t + positions[low].z;
    texCoords[3].x = (texCoords[high].x - texCoords[low].x) * t + texCoords[low].x;
    texCoords[3].y = (texCoords[high].y - texCoords[low].y) * t + texCoords[low].y;

    float scale = 1.0f / (positions[3].x - positions[mid].x);
    matrix.a0 = (texCoords[3].x - texCoords[mid].x) * scale;
    matrix.a1 = (texCoords[3].y - texCoords[mid].y) * scale;
    matrix.a2 = 0.0f;
    matrix.a3 = 0.0f;

    low = positions[1].x <= positions[0].x ? 1 : 0;

    if (positions[2].x <= positions[low].x) {
        low = 2;
    }

    high = positions[1].x >= positions[0].x ? 1 : 0;

    if (positions[2].x >= positions[high].x) {
        high = 2;
    }

    mid = 3 - high - low;

    if (fabsf(positions[low].x - positions[high].x) < 0.00000023841858f) {
        return 0;
    }

    t = (positions[mid].x - positions[low].x) / (positions[high].x - positions[low].x);
    positions[3].x = (positions[high].x - positions[low].x) * t + positions[low].x;
    positions[3].y = (positions[high].y - positions[low].y) * t + positions[low].y;
    positions[3].z = (positions[high].z - positions[low].z) * t + positions[low].z;
    texCoords[3].x = (texCoords[high].x - texCoords[low].x) * t + texCoords[low].x;
    texCoords[3].y = (texCoords[high].y - texCoords[low].y) * t + texCoords[low].y;

    scale = 1.0f / (positions[3].y - positions[mid].y);
    matrix.b0 = (texCoords[3].x - texCoords[mid].x) * scale;
    matrix.b1 = (texCoords[3].y - texCoords[mid].y) * scale;
    matrix.b2 = 0.0f;
    matrix.b3 = 0.0f;
    matrix.c0 = 0.0f;
    matrix.c1 = 0.0f;
    matrix.c2 = 0.0f;
    matrix.c3 = 0.0f;
    matrix.d0 = texCoords[0].x - matrix.a0 * positions[0].x - matrix.b0 * positions[0].y;
    matrix.d1 = texCoords[0].y - positions[0].x * matrix.a1 - positions[0].y * matrix.b1;
    matrix.d2 = 0.0f;
    matrix.d3 = 1.0f;

    return 1;
}

// VERIFY: 15cdfc90 manual
// OFFSET: 0x00829AA0
void CM2Model::RenderModelBatchesForProjectedTexture() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    CM2Shared* shared = this->m_shared;
    uint32_t count = shared->m_skinData->m_batches.m_count;
    M2SkinProfile* skin = shared->m_skinData;

    if (!shared->SetIndices() || !this->m_shared->SetVertices(0)) {
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        M2Batch* batch = &skin->m_batches[i];

        if (!batch->m_materialLayer && !(batch->m_flags & 0x20)) {
            M2SkinSection* section = &this->m_shared->m_skinSections[batch->m_skinSectionIndex];

            CGxBatch gxBatch;
            gxBatch.m_primType = GxPrim_Triangles;
            gxBatch.m_start = section->m_indexStart;
            gxBatch.m_count = section->m_indexCount;
            gxBatch.m_minIndex = 0;
            gxBatch.m_maxIndex = this->m_shared->m_skinData->m_vertices.m_count - 1;
            g_theGxDevicePtr->Draw(&gxBatch, 1);
        }
    }
}

// VERIFY: be5156ea manual
// OFFSET: 0x00829BA0
void CM2Model::RenderBatchShadowMap(int32_t untextured, M2Batch* batch, const CModelBatchList& list, uint32_t index, M2SkinSection* section, M2SkinSection* prevSection) {
    if (!untextured) {
        M2Data* data = this->m_shared->m_data;
        uint16_t textureIndex = data->m_textureCombos[batch->m_textureComboIndex];

        if (textureIndex >= data->m_textures.m_count) {
            return;
        }

        CTexture* texture = this->m_textures[textureIndex];

        if (!texture) {
            return;
        }

        CGxTex* gxTex = TextureGetGxTex(texture, 0, nullptr);

        if (!gxTex) {
            return;
        }

        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    }

    uint32_t total = list.m_batches[index].m_count;
    uint32_t instances = this->m_shared->AllocInstances(total);

    if (instances > total) {
        instances = total;
    }

    if (total > 1) {
        this->m_shared->SetIndices();
        this->m_shared->SetVertices(0);
    }

    CGxBatch gxBatch;
    gxBatch.m_primType = GxPrim_Triangles;
    gxBatch.m_start = section->m_indexStart;
    gxBatch.m_count = 0;
    gxBatch.m_minIndex = 0;
    gxBatch.m_maxIndex = 0;

    for (uint32_t done = 0; done < total; done += instances) {
        if (done + instances > total) {
            instances = total - done;
        }

        if (total > 1 || !prevSection || section->m_boneComboIndex != prevSection->m_boneComboIndex) {
            float* constants = g_theGxDevicePtr->ShaderConstantsLock(GxSh_Vertex) + 31 * 4;
            uint32_t registers = 0;

            for (uint32_t n = 0; n < instances; n++) {
                CM2Model* model = list.m_batches[done + index + n].m_model;

                for (uint32_t i = 0; i < section->m_boneCount; i++) {
                    const C44Matrix& bone = model->m_boneMatrices[this->m_shared->m_data->m_boneCombos[i + section->m_boneComboIndex]];
                    constants[0] = bone.a0;
                    constants[1] = bone.b0;
                    constants[2] = bone.c0;
                    constants[3] = bone.d0;
                    constants[4] = bone.a1;
                    constants[5] = bone.b1;
                    constants[6] = bone.c1;
                    constants[7] = bone.d1;
                    constants[8] = bone.a2;
                    constants[9] = bone.b2;
                    constants[10] = bone.c2;
                    constants[11] = bone.d2;
                    constants += 12;
                    registers++;
                }
            }

            g_theGxDevicePtr->ShaderConstantsUnlock(GxSh_Vertex, 31, 3 * registers);
        }

        if (!prevSection || section->m_boneInfluences != prevSection->m_boneInfluences) {
            CShaderEffect::SetDefaultShaders(section->m_boneInfluences);
        }

        gxBatch.m_count = instances * section->m_indexCount;
        gxBatch.m_maxIndex = static_cast<uint16_t>(instances * this->m_shared->m_skinData->m_vertices.m_count - 1);
        g_theGxDevicePtr->Draw(&gxBatch, 1);
    }
}

// VERIFY: ef747db8 manual
// OFFSET: 0x00829E40
void CM2Model::RenderModelBatchListShadowMap(int32_t untextured, const CModelBatchList& list) {
    M2SkinSection* prevSection = nullptr;
    CM2Model* prevModel = nullptr;
    uint32_t i = 0;

    while (i < list.m_count) {
        CModelBatch* entry = &list.m_batches[i];
        CM2Model* model = entry->m_model;
        CM2Shared* shared = model->m_shared;

        if (model != prevModel) {
            prevSection = nullptr;
            prevModel = model;

            if (entry->m_count == 1) {
                int32_t indexed = model->m_optGeo ? model->SetIndices() : shared->SetIndices();

                if (!indexed || !shared->SetVertices(0)) {
                    continue;
                }
            }
        }

        M2Batch* batch;
        M2SkinSection* section;

        if (model->m_optGeo) {
            M2ModelOptGeo* optGeo = model->m_optGeo;
            batch = &optGeo->m_batches[entry->m_batchIndex];
            section = &optGeo->m_skinSections[batch->m_skinSectionIndex];
        } else {
            batch = &shared->m_skinData->m_batches[entry->m_batchIndex];
            section = &shared->m_skinSections[batch->m_skinSectionIndex];
        }

        model->RenderBatchShadowMap(untextured, batch, list, i, section, prevSection);
        i += entry->m_count;
        prevSection = section;
    }
}

// VERIFY: 0efd8ae8 manual
// OFFSET: 0x00829F40
void CM2Model::TransformVerticesNoUVSelect_cpp(const CM2Model* model, const M2SkinSection* section, M2VertexPNT2* out) {
    C44Matrix matrix;
    uint32_t weights = 0;
    uint32_t indices = 0;
    const C44Matrix* bones = model->m_boneMatrices;
    M2Vertex* vertex = &model->m_shared->m_data->m_vertices[section->m_vertexStart];
    M2Vertex* end = vertex + section->m_vertexCount;

    for (; vertex < end; vertex++, out++) {
        if (*reinterpret_cast<uint32_t*>(&vertex->m_weights) != weights || *reinterpret_cast<uint32_t*>(&vertex->m_indices) != indices) {
            weights = *reinterpret_cast<uint32_t*>(&vertex->m_weights);
            indices = *reinterpret_cast<uint32_t*>(&vertex->m_indices);

            const C44Matrix& bone = bones[vertex->m_indices.b[0]];
            float weight = vertex->m_weights.b[0] * 0.0039215689f;
            matrix.a0 = bone.a0 * weight;
            matrix.a1 = bone.a1 * weight;
            matrix.a2 = bone.a2 * weight;
            matrix.b0 = bone.b0 * weight;
            matrix.b1 = bone.b1 * weight;
            matrix.b2 = bone.b2 * weight;
            matrix.c0 = bone.c0 * weight;
            matrix.c1 = bone.c1 * weight;
            matrix.c2 = bone.c2 * weight;
            matrix.d0 = bone.d0 * weight;
            matrix.d1 = bone.d1 * weight;
            matrix.d2 = bone.d2 * weight;

            for (uint32_t k = 1; k < 4 && vertex->m_weights.b[k]; k++) {
                const C44Matrix& next = bones[vertex->m_indices.b[k]];
                weight = vertex->m_weights.b[k] * 0.0039215689f;
                matrix.a0 += next.a0 * weight;
                matrix.a1 += next.a1 * weight;
                matrix.a2 += next.a2 * weight;
                matrix.b0 += next.b0 * weight;
                matrix.b1 += next.b1 * weight;
                matrix.b2 += next.b2 * weight;
                matrix.c0 += next.c0 * weight;
                matrix.c1 += next.c1 * weight;
                matrix.c2 += next.c2 * weight;
                matrix.d0 += next.d0 * weight;
                matrix.d1 += next.d1 * weight;
                matrix.d2 += next.d2 * weight;
            }
        }

        out->m_position = vertex->m_position * matrix;
        out->m_normal.x = vertex->m_normal.z * matrix.c0 + vertex->m_normal.x * matrix.a0 + matrix.b0 * vertex->m_normal.y;
        out->m_normal.y = vertex->m_normal.z * matrix.c1 + vertex->m_normal.x * matrix.a1 + matrix.b1 * vertex->m_normal.y;
        out->m_normal.z = vertex->m_normal.z * matrix.c2 + vertex->m_normal.x * matrix.a2 + matrix.b2 * vertex->m_normal.y;
        out->m_texcoord[0] = vertex->m_texcoord[0];
        out->m_texcoord[1] = vertex->m_texcoord[1];
    }
}

// VERIFY: 3101ba14 manual
// OFFSET: 0x0082A210
void CM2Model::TransformVertices_cpp(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord) {
    C44Matrix matrix;
    uint32_t weights = 0;
    uint32_t indices = 0;
    const C44Matrix* bones = model->m_boneMatrices;
    M2Vertex* vertex = &model->m_shared->m_data->m_vertices[section->m_vertexStart];
    M2Vertex* end = vertex + section->m_vertexCount;
    float* out = reinterpret_cast<float*>(vertices);

    for (; vertex < end; vertex++, out += 8) {
        if (*reinterpret_cast<uint32_t*>(&vertex->m_weights) != weights || *reinterpret_cast<uint32_t*>(&vertex->m_indices) != indices) {
            weights = *reinterpret_cast<uint32_t*>(&vertex->m_weights);
            indices = *reinterpret_cast<uint32_t*>(&vertex->m_indices);

            const C44Matrix& bone = bones[vertex->m_indices.b[0]];
            float weight = vertex->m_weights.b[0] * 0.0039215689f;
            matrix.a0 = bone.a0 * weight;
            matrix.a1 = bone.a1 * weight;
            matrix.a2 = bone.a2 * weight;
            matrix.b0 = bone.b0 * weight;
            matrix.b1 = bone.b1 * weight;
            matrix.b2 = bone.b2 * weight;
            matrix.c0 = bone.c0 * weight;
            matrix.c1 = bone.c1 * weight;
            matrix.c2 = bone.c2 * weight;
            matrix.d0 = bone.d0 * weight;
            matrix.d1 = bone.d1 * weight;
            matrix.d2 = bone.d2 * weight;

            for (uint32_t k = 1; k < 4 && vertex->m_weights.b[k]; k++) {
                const C44Matrix& next = bones[vertex->m_indices.b[k]];
                weight = vertex->m_weights.b[k] * 0.0039215689f;
                matrix.a0 += next.a0 * weight;
                matrix.a1 += next.a1 * weight;
                matrix.a2 += next.a2 * weight;
                matrix.b0 += next.b0 * weight;
                matrix.b1 += next.b1 * weight;
                matrix.b2 += next.b2 * weight;
                matrix.c0 += next.c0 * weight;
                matrix.c1 += next.c1 * weight;
                matrix.c2 += next.c2 * weight;
                matrix.d0 += next.d0 * weight;
                matrix.d1 += next.d1 * weight;
                matrix.d2 += next.d2 * weight;
            }
        }

        C3Vector position = vertex->m_position * matrix;
        out[0] = position.x;
        out[1] = position.y;
        out[2] = position.z;
        out[3] = vertex->m_normal.y * matrix.b0 + matrix.c0 * vertex->m_normal.z + matrix.a0 * vertex->m_normal.x;
        out[4] = vertex->m_normal.y * matrix.b1 + matrix.c1 * vertex->m_normal.z + matrix.a1 * vertex->m_normal.x;
        out[5] = vertex->m_normal.y * matrix.b2 + matrix.c2 * vertex->m_normal.z + matrix.a2 * vertex->m_normal.x;
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
    }
}

// VERIFY: 3dfd45aa manual
// OFFSET: 0x0082A4E0
void CM2Model::TransformVerticesOneBone_cpp(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord) {
    const C44Matrix* bones = model->m_boneMatrices;
    M2Vertex* vertex = &model->m_shared->m_data->m_vertices[section->m_vertexStart];
    M2Vertex* end = vertex + section->m_vertexCount;
    float* out = reinterpret_cast<float*>(vertices);

    for (; vertex < end; vertex++, out += 8) {
        const C44Matrix& bone = bones[vertex->m_indices.b[0]];
        out[0] = bone.c0 * vertex->m_position.z + vertex->m_position.y * bone.b0 + vertex->m_position.x * bone.a0 + bone.d0;
        out[1] = vertex->m_position.x * bone.a1 + vertex->m_position.y * bone.b1 + bone.c1 * vertex->m_position.z + bone.d1;
        out[2] = vertex->m_position.x * bone.a2 + vertex->m_position.y * bone.b2 + bone.c2 * vertex->m_position.z + bone.d2;
        out[3] = vertex->m_normal.x * bone.a0 + bone.c0 * vertex->m_normal.z + bone.b0 * vertex->m_normal.y;
        out[4] = bone.c1 * vertex->m_normal.z + bone.b1 * vertex->m_normal.y + bone.a1 * vertex->m_normal.x;
        out[5] = bone.c2 * vertex->m_normal.z + bone.b2 * vertex->m_normal.y + bone.a2 * vertex->m_normal.x;
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
    }
}

// VERIFY: edef765a manual
// OFFSET: 0x0082A600
void CM2Model::TransformVertices_sse(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord) {
    if (reinterpret_cast<uintptr_t>(vertices) & 0xF) {
        CM2Model::TransformVertices_cpp(model, section, vertices, texCoord);
        return;
    }

    uint32_t weights = 0;
    uint32_t indices = 0;
    const C44Matrix* bones = model->m_boneMatrices;
    M2Vertex* vertex = &model->m_shared->m_data->m_vertices[section->m_vertexStart];
    M2Vertex* end = vertex + section->m_vertexCount;
    float* out = reinterpret_cast<float*>(vertices);
    __m128 row0;
    __m128 row1;
    __m128 row2;
    __m128 row3;

    while (vertex != end) {
        if (static_cast<uint32_t>(vertex->m_weights) != weights || static_cast<uint32_t>(vertex->m_indices) != indices) {
            weights = static_cast<uint32_t>(vertex->m_weights);
            indices = static_cast<uint32_t>(vertex->m_indices);

            __m128 weight = _mm_set1_ps(vertex->m_weights[0] * 0.0039215689f);
            const float* bone = reinterpret_cast<const float*>(&bones[vertex->m_indices[0]]);
            row0 = _mm_mul_ps(_mm_load_ps(&bone[0]), weight);
            row1 = _mm_mul_ps(_mm_load_ps(&bone[4]), weight);
            row2 = _mm_mul_ps(_mm_load_ps(&bone[8]), weight);
            row3 = _mm_mul_ps(_mm_load_ps(&bone[12]), weight);

            for (uint32_t i = 1; i < 4 && vertex->m_weights[i]; i++) {
                weight = _mm_set1_ps(vertex->m_weights[i] * 0.0039215689f);
                bone = reinterpret_cast<const float*>(&bones[vertex->m_indices[i]]);
                row0 = _mm_add_ps(row0, _mm_mul_ps(_mm_load_ps(&bone[0]), weight));
                row1 = _mm_add_ps(row1, _mm_mul_ps(_mm_load_ps(&bone[4]), weight));
                row2 = _mm_add_ps(row2, _mm_mul_ps(_mm_load_ps(&bone[8]), weight));
                row3 = _mm_add_ps(row3, _mm_mul_ps(_mm_load_ps(&bone[12]), weight));
            }
        }

        __m128 normal = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(vertex->m_normal.x), row0), _mm_mul_ps(_mm_set1_ps(vertex->m_normal.y), row1)), _mm_mul_ps(_mm_set1_ps(vertex->m_normal.z), row2));
        __m128 position = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(vertex->m_position.x), row0), _mm_mul_ps(_mm_set1_ps(vertex->m_position.y), row1)), _mm_mul_ps(_mm_set1_ps(vertex->m_position.z), row2)), row3);
        _mm_store_ps(&out[0], position);
        _mm_storeu_ps(&out[3], normal);
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
        vertex++;
        out += 8;
    }
}

// VERIFY: 9d870aa4 manual
// OFFSET: 0x0082AC10
void CM2Model::TransformVerticesOneBone_sse(CM2Model* model, M2SkinSection* section, uint8_t* vertices, int32_t texCoord) {
    if (reinterpret_cast<uintptr_t>(vertices) & 0xF) {
        CM2Model::TransformVerticesOneBone_cpp(model, section, vertices, texCoord);
        return;
    }

    const C44Matrix* bones = model->m_boneMatrices;
    M2Vertex* vertex = &model->m_shared->m_data->m_vertices[section->m_vertexStart];
    M2Vertex* end = vertex + section->m_vertexCount;
    float* out = reinterpret_cast<float*>(vertices);

    for (; vertex < end; vertex++, out += 8) {
        const float* bone = reinterpret_cast<const float*>(&bones[vertex->m_indices.b[0]]);
        __m128 row0 = _mm_load_ps(&bone[0]);
        __m128 row1 = _mm_load_ps(&bone[4]);
        __m128 row2 = _mm_load_ps(&bone[8]);
        __m128 position = _mm_add_ps(_mm_add_ps(_mm_mul_ps(row2, _mm_set1_ps(vertex->m_position.z)), _mm_add_ps(_mm_mul_ps(row1, _mm_set1_ps(vertex->m_position.y)), _mm_mul_ps(row0, _mm_set1_ps(vertex->m_position.x)))), _mm_load_ps(&bone[12]));
        __m128 normal = _mm_add_ps(_mm_add_ps(_mm_mul_ps(row0, _mm_set1_ps(vertex->m_normal.x)), _mm_mul_ps(row1, _mm_set1_ps(vertex->m_normal.y))), _mm_mul_ps(_mm_set1_ps(vertex->m_normal.z), row2));
        _mm_store_ps(&out[0], position);
        _mm_storeu_ps(&out[3], normal);
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
    }
}

// VERIFY: c111136b auto
// OFFSET: 0x0082AD50
template <>
void M2AnimateTrack<C4Quaternion, C4Quaternion>(CM2Model* model, M2ModelBone* bone, const M2Track<C4Quaternion>& track, M2ModelTrack<C4Quaternion>& modelTrack, const C4Quaternion& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<C4Quaternion>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];

    if (keys.m_count) {
        uint32_t key1;
        float ratio;
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
            return;
        }

        modelTrack.m_currentValue = C4Quaternion::Nlerp(ratio, keys[modelTrack.m_currentKey], keys[key1]);
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<C4Quaternion>& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        C4Quaternion blend;

        if (blendKeys.m_count) {
            uint32_t key1;
            float ratio;
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            blend = C4Quaternion::Nlerp(ratio, blendKeys[modelTrack.m_secondaryKey], blendKeys[key1]);
        } else {
            blend = defaultValue;
        }

        modelTrack.m_currentValue = C4Quaternion::Slerp(bone->m_blendFactor, modelTrack.m_currentValue, blend);
    }
}

// VERIFY: 00e4d4f4 auto
// OFFSET: 0x0082AF40
template <>
void M2AnimateTrack<fixed16, float>(CM2Model* model, M2ModelBone* bone, const M2Track<fixed16>& track, M2ModelTrack<float>& modelTrack, const float& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<fixed16>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];

    if (keys.m_count) {
        uint32_t key1;
        float ratio;
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey].m_value * 0.000030518509f;
            return;
        }

        float value0 = keys[modelTrack.m_currentKey].m_value * 0.000030518509f;
        modelTrack.m_currentValue = value0 + (keys[key1].m_value * 0.000030518509f - value0) * ratio;
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<fixed16>& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        float blend;

        if (blendKeys.m_count) {
            uint32_t key1;
            float ratio;
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            float value0 = blendKeys[modelTrack.m_secondaryKey].m_value * 0.000030518509f;
            blend = value0 + (blendKeys[key1].m_value * 0.000030518509f - value0) * ratio;
        } else {
            blend = defaultValue;
        }

        modelTrack.m_currentValue = modelTrack.m_currentValue + (blend - modelTrack.m_currentValue) * bone->m_blendFactor;
    }
}

// VERIFY: 45a65fad auto
// OFFSET: 0x0082B0A0
template <>
void M2AnimateTrack<C3Vector, C3Vector>(CM2Model* model, M2ModelBone* bone, const M2Track<C3Vector>& track, M2ModelTrack<C3Vector>& modelTrack, const C3Vector& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<C3Vector>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];

    if (keys.m_count) {
        uint32_t key1;
        float ratio;
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
            return;
        }

        C3Vector& value0 = keys[modelTrack.m_currentKey];
        C3Vector& value1 = keys[key1];
        modelTrack.m_currentValue.x = (value1.x - value0.x) * ratio + value0.x;
        modelTrack.m_currentValue.y = (value1.y - value0.y) * ratio + value0.y;
        modelTrack.m_currentValue.z = ratio * (value1.z - value0.z) + value0.z;
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<C3Vector>& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        C3Vector blend;

        if (blendKeys.m_count) {
            uint32_t key1;
            float ratio;
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            C3Vector& value0 = blendKeys[modelTrack.m_secondaryKey];
            C3Vector& value1 = blendKeys[key1];
            blend.x = (value1.x - value0.x) * ratio + value0.x;
            blend.y = (value1.y - value0.y) * ratio + value0.y;
            blend.z = ratio * (value1.z - value0.z) + value0.z;
        } else {
            blend = defaultValue;
        }

        float factor = bone->m_blendFactor;
        modelTrack.m_currentValue.x = (blend.x - modelTrack.m_currentValue.x) * factor + modelTrack.m_currentValue.x;
        modelTrack.m_currentValue.y = (blend.y - modelTrack.m_currentValue.y) * factor + modelTrack.m_currentValue.y;
        modelTrack.m_currentValue.z = factor * (blend.z - modelTrack.m_currentValue.z) + modelTrack.m_currentValue.z;
    }
}

// VERIFY: d42aa12d manual
// OFFSET: 0x0082B270
template <>
void M2AnimateTrack<uint8_t, uint8_t>(CM2Model* model, M2ModelBone* bone, const M2Track<uint8_t>& track, M2ModelTrack<uint8_t>& modelTrack, const uint8_t& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<uint8_t>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];
    uint32_t key1;
    float ratio;

    if (keys.m_count) {
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
            return;
        }

        modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;

        if (track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0].m_count) {
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
        }
    }
}

// VERIFY: 6fc3d990 manual
// OFFSET: 0x0082B340
template <>
void M2AnimateTrack<float, float>(CM2Model* model, M2ModelBone* bone, const M2Track<float>& track, M2ModelTrack<float>& modelTrack, const float& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<float>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];
    uint32_t key1;
    float ratio;

    if (keys.m_count) {
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
            return;
        }

        float value0 = keys[modelTrack.m_currentKey];
        modelTrack.m_currentValue = value0 + (keys[key1] - value0) * ratio;
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<float>& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        float blend;

        if (blendKeys.m_count) {
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            float value0 = blendKeys[modelTrack.m_secondaryKey];
            blend = value0 + (blendKeys[key1] - value0) * ratio;
        } else {
            blend = defaultValue;
        }

        modelTrack.m_currentValue = modelTrack.m_currentValue + (blend - modelTrack.m_currentValue) * bone->m_blendFactor;
    }
}

// VERIFY: bd536b44 manual
// OFFSET: 0x0082B460
template <>
void M2AnimateSplineTrack<M2SplineKey<C3Vector>, C3Vector>(CM2Model* model, M2ModelBone* bone, const M2Track<M2SplineKey<C3Vector> >& track, M2ModelTrack<C3Vector>& modelTrack, const C3Vector& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<M2SplineKey<C3Vector> >& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];
    uint32_t key1;
    float ratio;

    if (keys.m_count) {
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey].m_value;
            return;
        }

        M2SplineKey<C3Vector>& k0 = keys[modelTrack.m_currentKey];
        M2SplineKey<C3Vector>& k1 = keys[key1];

        switch (track.m_trackType) {
            case M2TRACK_LINEAR: {
                modelTrack.m_currentValue.x = (k1.m_value.x - k0.m_value.x) * ratio + k0.m_value.x;
                modelTrack.m_currentValue.y = (k1.m_value.y - k0.m_value.y) * ratio + k0.m_value.y;
                modelTrack.m_currentValue.z = ratio * (k1.m_value.z - k0.m_value.z) + k0.m_value.z;
                break;
            }

            case M2TRACK_BEZIER: {
                float r2 = ratio * ratio;
                float r3 = ratio * ratio * ratio;
                float b0 = ratio * ratio * 3.0f - r3 - ratio * 3.0f + 1.0f;
                float b1 = ratio * 3.0f + 3.0f * r3 - r2 * 6.0f;
                float b2 = ratio * ratio * 3.0f - 3.0f * r3;
                modelTrack.m_currentValue.x = k0.m_outTan.x * b1 + k1.m_value.x * r3 + k1.m_inTan.x * b2 + k0.m_value.x * b0;
                modelTrack.m_currentValue.y = k1.m_inTan.y * b2 + k0.m_outTan.y * b1 + k0.m_value.y * b0 + k1.m_value.y * r3;
                modelTrack.m_currentValue.z = b0 * k0.m_value.z + (b1 * k0.m_outTan.z + b2 * k1.m_inTan.z) + r3 * k1.m_value.z;
                break;
            }

            case M2TRACK_HERMITE: {
                float r2 = ratio * ratio;
                float r2x3 = ratio * ratio * 3.0f;
                float r3 = ratio * ratio * ratio;
                float h3 = ratio + r3 - 2.0f * (ratio * ratio);
                float h4 = r3 - r2;
                float h1 = r3 * 2.0f - r2x3 + 1.0f;
                float h2 = r2x3 - r3 * 2.0f;
                modelTrack.m_currentValue.x = k0.m_outTan.x * h3 + k1.m_inTan.x * h4 + k1.m_value.x * h2 + k0.m_value.x * h1;
                modelTrack.m_currentValue.y = k1.m_inTan.y * h4 + k0.m_outTan.y * h3 + k1.m_value.y * h2 + k0.m_value.y * h1;
                modelTrack.m_currentValue.z = h1 * k0.m_value.z + (h2 * k1.m_value.z + (h3 * k0.m_outTan.z + h4 * k1.m_inTan.z));
                break;
            }
        }
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<M2SplineKey<C3Vector> >& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        C3Vector blend;

        if (blendKeys.m_count) {
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            M2SplineKey<C3Vector>& k0 = blendKeys[modelTrack.m_secondaryKey];
            M2SplineKey<C3Vector>& k1 = blendKeys[key1];

            if (track.m_trackType == M2TRACK_BEZIER) {
                float r2 = ratio * ratio;
                float r3 = ratio * ratio * ratio;
                float b0 = ratio * ratio * 3.0f - r3 - ratio * 3.0f + 1.0f;
                float b1 = ratio * 3.0f + 3.0f * r3 - r2 * 6.0f;
                float b2 = ratio * ratio * 3.0f - 3.0f * r3;
                blend.x = k1.m_value.x * r3 + k0.m_value.x * b0 + b1 * k0.m_outTan.x + b2 * k1.m_inTan.x;
                blend.y = k1.m_inTan.y * b2 + k0.m_outTan.y * b1 + k0.m_value.y * b0 + k1.m_value.y * r3;
                blend.z = r3 * k1.m_value.z + (b0 * k0.m_value.z + b1 * k0.m_outTan.z + b2 * k1.m_inTan.z);
            } else if (track.m_trackType == M2TRACK_HERMITE) {
                float r2 = ratio * ratio;
                float r2x3 = ratio * ratio * 3.0f;
                float r3 = ratio * ratio * ratio;
                float h3 = ratio + r3 - 2.0f * (ratio * ratio);
                float h4 = r3 - r2;
                float h1 = r3 * 2.0f - r2x3 + 1.0f;
                float h2 = r2x3 - r3 * 2.0f;
                blend.x = k1.m_value.x * h2 + k1.m_inTan.x * h4 + k0.m_outTan.x * h3 + k0.m_value.x * h1;
                blend.y = k0.m_outTan.y * h3 + k1.m_inTan.y * h4 + k1.m_value.y * h2 + k0.m_value.y * h1;
                blend.z = h1 * k0.m_value.z + h2 * k1.m_value.z + (h4 * k1.m_inTan.z + h3 * k0.m_outTan.z);
            } else {
                blend.x = (k1.m_value.x - k0.m_value.x) * ratio + k0.m_value.x;
                blend.y = (k1.m_value.y - k0.m_value.y) * ratio + k0.m_value.y;
                blend.z = ratio * (k1.m_value.z - k0.m_value.z) + k0.m_value.z;
            }
        } else {
            blend = defaultValue;
        }

        float factor = bone->m_blendFactor;
        modelTrack.m_currentValue.x = (blend.x - modelTrack.m_currentValue.x) * factor + modelTrack.m_currentValue.x;
        modelTrack.m_currentValue.y = (blend.y - modelTrack.m_currentValue.y) * factor + modelTrack.m_currentValue.y;
        modelTrack.m_currentValue.z = factor * (blend.z - modelTrack.m_currentValue.z) + modelTrack.m_currentValue.z;
    }
}

// VERIFY: 342a7165 manual
// OFFSET: 0x0082B8A0
template <>
void M2AnimateSplineTrack<M2SplineKey<float>, float>(CM2Model* model, M2ModelBone* bone, const M2Track<M2SplineKey<float> >& track, M2ModelTrack<float>& modelTrack, const float& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<M2SplineKey<float> >& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];
    uint32_t key1;
    float ratio;

    if (keys.m_count) {
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey].m_value;
            return;
        }

        M2SplineKey<float>& k0 = keys[modelTrack.m_currentKey];
        M2SplineKey<float>& k1 = keys[key1];

        switch (track.m_trackType) {
            case M2TRACK_LINEAR: {
                modelTrack.m_currentValue = k0.m_value + (k1.m_value - k0.m_value) * ratio;
                break;
            }

            case M2TRACK_BEZIER: {
                float r3 = ratio * ratio * ratio;
                modelTrack.m_currentValue = r3 * k1.m_value + (r3 * 3.0f - ratio * ratio * 6.0f + ratio * 3.0f) * k0.m_outTan + (3.0f * (ratio * ratio) - r3 * 3.0f) * k1.m_inTan + (3.0f * (ratio * ratio) - r3 - ratio * 3.0f + 1.0f) * k0.m_value;
                break;
            }

            case M2TRACK_HERMITE: {
                float r3 = ratio * ratio * ratio;
                modelTrack.m_currentValue = (ratio + r3 - 2.0f * (ratio * ratio)) * k0.m_outTan + (r3 - ratio * ratio) * k1.m_inTan + (r3 * 2.0f - ratio * ratio * 3.0f + 1.0f) * k0.m_value + (ratio * ratio * 3.0f - r3 * 2.0f) * k1.m_value;
                break;
            }
        }
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;
        M2Array<M2SplineKey<float> >& blendKeys = track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0];
        float blend;

        if (blendKeys.m_count) {
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
            M2SplineKey<float>& k0 = blendKeys[modelTrack.m_secondaryKey];
            M2SplineKey<float>& k1 = blendKeys[key1];

            if (track.m_trackType == M2TRACK_BEZIER) {
                float r3 = ratio * ratio * ratio;
                blend = r3 * k1.m_value + (r3 * 3.0f - ratio * ratio * 6.0f + ratio * 3.0f) * k0.m_outTan + (3.0f * (ratio * ratio) - r3 * 3.0f) * k1.m_inTan + (3.0f * (ratio * ratio) - r3 - ratio * 3.0f + 1.0f) * k0.m_value;
            } else if (track.m_trackType == M2TRACK_HERMITE) {
                float r3 = ratio * ratio * ratio;
                blend = (ratio + r3 - 2.0f * (ratio * ratio)) * k0.m_outTan + (r3 - ratio * ratio) * k1.m_inTan + (r3 * 2.0f - ratio * ratio * 3.0f + 1.0f) * k0.m_value + (ratio * ratio * 3.0f - r3 * 2.0f) * k1.m_value;
            } else {
                blend = k0.m_value + (k1.m_value - k0.m_value) * ratio;
            }
        } else {
            blend = defaultValue;
        }

        modelTrack.m_currentValue = modelTrack.m_currentValue + (blend - modelTrack.m_currentValue) * bone->m_blendFactor;
    }
}

// VERIFY: a1a69eb5 manual
// OFFSET: 0x0082BB50
template <>
void M2AnimateTrack<uint16_t, uint16_t>(CM2Model* model, M2ModelBone* bone, const M2Track<uint16_t>& track, M2ModelTrack<uint16_t>& modelTrack, const uint16_t& defaultValue) {
    uint32_t animIndex = bone->m_sequence.m_animIndex;
    M2Array<uint16_t>& keys = track.m_sequenceKeys[animIndex < track.m_sequenceKeys.m_count ? animIndex : 0];
    uint32_t key1;
    float ratio;

    if (keys.m_count) {
        model->FindKey(&bone->m_sequence, track, modelTrack.m_currentKey, key1, ratio);

        if (track.m_trackType == M2TRACK_NONE) {
            modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
            return;
        }

        modelTrack.m_currentValue = keys[modelTrack.m_currentKey];
    } else {
        modelTrack.m_currentValue = defaultValue;

        if (track.m_trackType == M2TRACK_NONE) {
            return;
        }
    }

    if (bone->m_blendFactor != 0.0f && track.m_loopIndex == 0xFFFF) {
        uint32_t blendIndex = bone->m_secondarySequence.m_animIndex;

        if (track.m_sequenceKeys[blendIndex < track.m_sequenceKeys.m_count ? blendIndex : 0].m_count) {
            model->FindKey(&bone->m_secondarySequence, track, modelTrack.m_secondaryKey, key1, ratio);
        }
    }
}

// INLINED
M2ModelBoneSeq::M2ModelBoneSeq() {
    this->m_speed = 0.0f;
    this->m_repeatCount = 1;
    this->m_invSpeed = 0.0f;
    this->m_finished = 1;
    this->m_pickRandomVariation = 1;
    this->m_currentTime = 0;
    this->m_animIndex = 0xFFFF;
    this->m_sourceBoneIndex = 0xFFFF;
    this->m_sequenceIndex = 0xFFFF;
    this->m_startTime = 0;
    this->m_endTime = 0;
    this->m_startOffset = 0;
}

// VERIFY: 305d932d manual
// OFFSET: 0x0082BD60
M2ModelBone::M2ModelBone() {
    this->m_invBlendDuration = 0.0f;
    this->m_blendFactor = 0.0f;
    this->m_proceduralTransform = nullptr;
    this->m_flags = 0;
    this->m_blendWeightMax = 1.0f;
    this->m_sequenceId = 0xFFFFFFFF;
    this->m_variationIndex = 0;
    this->m_blendEndTime = 0;
    this->m_animListPrev = nullptr;
    this->m_animListNext = 0xFFFF;
}

// INLINED
M2BatchRange::M2BatchRange() {
    this->m_first = 0;
    this->m_last = 0;
}

// VERIFY: 2ddf06e6 manual
// OFFSET: 0x0082BE60
CM2Model::CM2Model() {
    this->m_scaleSq = 0.0f;
    this->m_refCount = 1;
    this->m_fFlags = (this->m_fFlags & 0xFF800000) | 0x340;
    this->m_flags = 0;
    this->m_scenePrev = nullptr;
    this->m_sceneNext = nullptr;
    this->m_callbackPrev = nullptr;
    this->m_callbackNext = nullptr;
    this->m_loadedCallback = nullptr;
    this->m_loadedArg = nullptr;
    this->m_scene = nullptr;
    this->m_shared = nullptr;
    this->m_model30 = nullptr;
    this->m_modelCallList = nullptr;
    this->m_modelCallTail = nullptr;
    this->m_animatePrev = nullptr;
    this->m_animateNext = nullptr;
    this->m_attachParent = nullptr;
    this->m_attachments = nullptr;
    this->m_attachList = nullptr;
    this->m_attachPrev = nullptr;
    this->m_attachNext = nullptr;
    this->m_lastAnimTime = 0;
    this->m_drawPrev = nullptr;
    this->m_drawNext = nullptr;
    this->m_loops = nullptr;
    this->m_loopOrigin = 0;
    this->m_sequenceCallback = nullptr;
    this->m_sequenceCallbackParam = 0;
    this->m_lastEmitterTime = 0;
    this->m_uint90 = 0;
    this->m_bones = nullptr;
    this->m_boneMatrices = nullptr;
    this->m_skinSections = nullptr;
    this->m_colors = nullptr;
    this->m_textures = nullptr;
    this->m_textureWeights = nullptr;
    this->m_textureTransforms = nullptr;
    this->m_textureMatrices = nullptr;
    this->m_animListHead = 0xFFFF;
    this->m_attachmentIndex = 0xFFFF;
    this->m_frameStamp = 0xFFFFFFFF;
    this->m_attachmentId = 0xFFFFFFFF;
    this->m_worldTransform.a0 = 1.0f;
    this->m_worldTransform.b1 = 1.0f;
    this->m_worldTransform.c2 = 1.0f;
    this->m_worldTransform.d3 = 1.0f;
    this->m_worldTransform.a1 = 0.0f;
    this->m_worldTransform.a2 = 0.0f;
    this->m_worldTransform.a3 = 0.0f;
    this->m_worldTransform.b0 = 0.0f;
    this->m_worldTransform.b2 = 0.0f;
    this->m_worldTransform.b3 = 0.0f;
    this->m_worldTransform.c0 = 0.0f;
    this->m_worldTransform.c1 = 0.0f;
    this->m_worldTransform.c3 = 0.0f;
    this->m_worldTransform.d0 = 0.0f;
    this->m_worldTransform.d1 = 0.0f;
    this->m_worldTransform.d2 = 0.0f;
    this->m_matrixF4.a0 = 1.0f;
    this->m_matrixF4.b1 = 1.0f;
    this->m_matrixF4.c2 = 1.0f;
    this->m_matrixF4.d3 = 1.0f;
    this->m_matrixF4.a1 = 0.0f;
    this->m_matrixF4.a2 = 0.0f;
    this->m_matrixF4.a3 = 0.0f;
    this->m_matrixF4.b0 = 0.0f;
    this->m_matrixF4.b2 = 0.0f;
    this->m_matrixF4.b3 = 0.0f;
    this->m_matrixF4.c0 = 0.0f;
    this->m_matrixF4.c1 = 0.0f;
    this->m_matrixF4.c3 = 0.0f;
    this->m_matrixF4.d0 = 0.0f;
    this->m_matrixF4.d1 = 0.0f;
    this->m_matrixF4.d2 = 0.0f;
    this->m_frameOfReferenceMatrix.a0 = 1.0f;
    this->m_frameOfReferenceMatrix.b1 = 1.0f;
    this->m_frameOfReferenceMatrix.c2 = 1.0f;
    this->m_frameOfReferenceMatrix.d3 = 1.0f;
    this->m_frameOfReferenceMatrix.a1 = 0.0f;
    this->m_frameOfReferenceMatrix.a2 = 0.0f;
    this->m_frameOfReferenceMatrix.a3 = 0.0f;
    this->m_frameOfReferenceMatrix.b0 = 0.0f;
    this->m_frameOfReferenceMatrix.b2 = 0.0f;
    this->m_frameOfReferenceMatrix.b3 = 0.0f;
    this->m_frameOfReferenceMatrix.c0 = 0.0f;
    this->m_frameOfReferenceMatrix.c1 = 0.0f;
    this->m_frameOfReferenceMatrix.c3 = 0.0f;
    this->m_frameOfReferenceMatrix.d0 = 0.0f;
    this->m_frameOfReferenceMatrix.d1 = 0.0f;
    this->m_frameOfReferenceMatrix.d2 = 0.0f;
    this->m_frameOfReference = nullptr;
    this->m_fadeAlpha = 1.0f;
    this->m_baseAlpha = 1.0f;
    this->m_diffuseTintR = 1.0f;
    this->m_diffuseTintG = 1.0f;
    this->m_diffuseTintB = 1.0f;
    this->m_emissiveTintR = 0.0f;
    this->m_emissiveTintG = 0.0f;
    this->m_emissiveTintB = 0.0f;
    this->m_effectiveAlpha = 1.0f;
    this->m_alpha = 1.0f;
    this->m_diffuseColor.x = 1.0f;
    this->m_diffuseColor.y = 1.0f;
    this->m_diffuseColor.z = 1.0f;
    this->m_emissiveColor.x = 0.0f;
    this->m_emissiveColor.y = 0.0f;
    this->m_emissiveColor.z = 0.0f;
    this->m_drawCallback = nullptr;
    this->m_drawArg = nullptr;
    this->unk_01B8 = 1.0f;
    this->m_eventCallback = nullptr;
    this->m_eventCallbackParam = 0;
    this->m_lights = nullptr;
    this->m_currentLighting = nullptr;
    this->m_lightingCallback = nullptr;
    this->m_lightingArg = nullptr;
    this->m_cameras = nullptr;
    this->m_ribbons = nullptr;
    this->m_ribbonEmitters = nullptr;
    this->m_particles = nullptr;
    this->m_particleEmitters = nullptr;
    this->m_particlePrev = nullptr;
    this->m_particleNext = nullptr;
    this->m_optGeo = nullptr;
    this->m_hitTestMode = 0;
    this->m_hitTestPrev = nullptr;
    this->m_hitTestNext = nullptr;
    this->m_hitTestOwner = nullptr;
    this->m_hitTestGroup = 0;

    if (!CM2Model::s_transformVertices[0]) {
        CM2Model::s_transformVertices[0] = CM2Model::TransformVertices_cpp;
        CM2Model::s_transformVertices[1] = CM2Model::TransformVerticesOneBone_cpp;
        CM2Model::s_transformVertices[2] = CM2Model::TransformVertices_cpp;
        CM2Model::s_transformVertices[3] = CM2Model::TransformVertices_cpp;
        CM2Model::s_transformVertices[4] = CM2Model::TransformVertices_cpp;

        if (CM2Cache::s_processorFeatures & 4) {
            CM2Model::s_transformVertices[0] = CM2Model::TransformVertices_sse;
            CM2Model::s_transformVertices[1] = CM2Model::TransformVerticesOneBone_sse;
            CM2Model::s_transformVertices[2] = CM2Model::TransformVertices_sse;
            CM2Model::s_transformVertices[3] = CM2Model::TransformVertices_sse;
            CM2Model::s_transformVertices[4] = CM2Model::TransformVertices_sse;
        }
    }
}

// VERIFY: 6ff1ddfd manual
// OFFSET: 0x0082C1C0
void CM2Model::FreeInternalResources() {
    M2ModelBone* bones = this->m_bones;

    if (!bones) {
        return;
    }

    M2Data* data = this->m_shared->m_data;
    this->m_bones = nullptr;

    if (this->m_loops) {
        this->m_loops = nullptr;
    }

    if (this->m_skinSections) {
        this->m_skinSections = nullptr;
    }

    if (this->m_colors) {
        this->m_colors = nullptr;
    }

    if (this->m_textureWeights) {
        this->m_textureWeights = nullptr;
    }

    if (this->m_textureTransforms) {
        this->m_textureTransforms = nullptr;
    }

    if (this->m_attachments) {
        this->m_attachments = nullptr;
    }

    if (this->m_lights) {
        for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
            this->m_lights[i].m_light.Unlink();
        }

        this->m_lights = nullptr;
    }

    if (this->m_cameras) {
        this->m_cameras = nullptr;
    }

    if (this->m_ribbons) {
        this->m_ribbons = nullptr;
    }

    if (this->m_particles) {
        this->m_particles = nullptr;
    }

    SMemFree(bones, __FILE__, __LINE__, 0);
}

// VERIFY: 1f601c7d manual
// OFFSET: 0x0082C2C0
CAaSphere CM2Model::GetCurrentBoundingSphere() {
    CAaSphere result;

    if (!(this->m_fFlags & 1) || !(this->m_fFlags & 0x8000)) {
        result.c = C3Vector(0.0f, 0.0f, 0.0f);
        result.r = 0.0f;
        return result;
    }

    M2Data* data = this->m_shared->m_data;

    if (data->m_particles.m_count) {
        C44Matrix matrix = this->m_attachParent ? this->m_matrixF4 * this->m_scene->m_viewInv : this->m_worldTransform;
        CAaBox& extent = this->m_shared->m_data->m_bounds.m_extent;
        C3Vector center((extent.b.x + extent.t.x) * 0.5f, (extent.t.y + extent.b.y) * 0.5f, 0.5f * (extent.t.z + extent.b.z));
        float radius = sqrtf(matrix.a2 * matrix.a2 + matrix.a1 * matrix.a1 + matrix.a0 * matrix.a0) * this->m_shared->m_data->m_bounds.m_radius;
        result.c = center * matrix;
        result.r = radius;
        return result;
    }

    M2Bounds* bounds = &data->m_sequences[this->m_bones->m_sequence.m_sequenceIndex].m_bounds;
    CAaSphere spheres[2];
    spheres[0].r = 0.0f;
    spheres[1].r = 0.0f;

    C44Matrix matrix = this->m_attachParent ? this->m_matrixF4 * this->m_scene->m_viewInv : this->m_worldTransform;

    if (fnotequal(bounds->m_radius, 0.0f)) {
        C3Vector center((bounds->m_extent.t.x + bounds->m_extent.b.x) * 0.5f, (bounds->m_extent.t.y + bounds->m_extent.b.y) * 0.5f, 0.5f * (bounds->m_extent.t.z + bounds->m_extent.b.z));
        spheres[0].c = center * matrix;
    }

    spheres[0].r = sqrtf(matrix.a1 * matrix.a1 + matrix.a2 * matrix.a2 + matrix.a0 * matrix.a0) * bounds->m_radius;

    M2Data* current = this->m_shared->m_data;
    uint32_t particleCount = current->m_particles.m_count;

    if (!particleCount && !current->m_ribbons.m_count) {
        return spheres[0];
    }

    CAaBox box;
    box.b = C3Vector(FLT_MAX, FLT_MAX, FLT_MAX);
    box.t = C3Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (uint32_t i = 0; i < particleCount; i = i + 1) {
        CParticleEmitter2* emitter = this->m_particleEmitters[i];

        if (emitter->m_flags & 0x200) {
            box |= emitter->WorldBounds();
        }

        particleCount = this->m_shared->m_data->m_particles.m_count;
    }

    uint32_t ribbonCount = this->m_shared->m_data->m_ribbons.m_count;

    for (uint32_t i = 0; i < ribbonCount; i++) {
        const CAaBox& ribbonBox = this->m_ribbonEmitters[i]->m_worldBounds;
        C3Vector top;
        top.x = box.t.x > ribbonBox.t.x ? box.t.x : ribbonBox.t.x;
        top.y = box.t.y > ribbonBox.t.y ? box.t.y : ribbonBox.t.y;
        top.z = box.t.z > ribbonBox.t.z ? box.t.z : ribbonBox.t.z;
        C3Vector bottom;
        bottom.x = box.b.x < ribbonBox.b.x ? box.b.x : ribbonBox.b.x;
        bottom.y = box.b.y < ribbonBox.b.y ? box.b.y : ribbonBox.b.y;
        bottom.z = box.b.z < ribbonBox.b.z ? box.b.z : ribbonBox.b.z;
        box.t = top;
        box.b = bottom;
    }

    if (box.t.x < box.b.x && box.b.y > box.t.y && box.t.z < box.b.z) {
        return spheres[0];
    }

    spheres[1].c.x = (box.t.x + box.b.x) * 0.5f;
    spheres[1].c.y = (box.t.y + box.b.y) * 0.5f;
    spheres[1].c.z = (box.t.z + box.b.z) * 0.5f;
    spheres[1].r = 0.5f * sqrtf((box.t.x - box.b.x) * (box.t.x - box.b.x) + (box.t.z - box.b.z) * (box.t.z - box.b.z) + (box.t.y - box.b.y) * (box.t.y - box.b.y));

    if (fabsf(spheres[0].r) >= 0.00000023841858f) {
        return CAaSphere::BoundingAccurate(spheres, 2);
    }

    return spheres[1];
}

// VERIFY: 34ca59a5 manual
// OFFSET: 0x0082C7C0
void CM2Model::SetGeometryVisible(uint32_t first, uint32_t last, int32_t visible) {
    if (this->m_fFlags & 1) {
        M2SkinProfile* skin = this->m_shared->m_skinData;
        int32_t changed = 0;

        for (uint32_t i = 0; i < skin->m_skinSections.m_count; i++) {
            uint32_t id = skin->m_skinSections[i].m_skinSectionId;

            if (first <= id && id <= last) {
                uint32_t* section = &this->m_skinSections[i];

                if ((*section == 0) != (visible == 0)) {
                    *section = visible;
                    changed = 1;
                }
            }
        }

        if (changed) {
            this->UnoptimizeVisibleGeometry();
        }
    } else {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_GEOMETRY_VISIBLE;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = first;
            call->m_args[1] = last;
            call->m_args[2] = visible;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }
    }
}

// VERIFY: 36243f77 manual
// OFFSET: 0x0082C8A0
void CM2Model::SetGeosetVisibilityRange(uint32_t first, uint32_t last, int32_t visible) {
    if (this->m_fFlags & 1) {
        M2SkinProfile* skin = this->m_shared->m_skinData;
        int32_t changed = 0;

        for (uint32_t i = 0; i < skin->m_skinSections.m_count; i++) {
            if (first <= i && i <= last) {
                uint32_t* section = &this->m_skinSections[i];

                if ((*section == 0) != (visible == 0)) {
                    *section = visible;
                    changed = 1;
                }
            }
        }

        if (changed) {
            this->UnoptimizeVisibleGeometry();
        }
    } else {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_GEOSET_VISIBILITY_RANGE;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = first;
            call->m_args[1] = last;
            call->m_args[2] = visible;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }
    }
}

// VERIFY: 88605bc8 manual
// OFFSET: 0x0082C970
void CM2Model::OptimizeVisibleGeometry() {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_OPTIMIZE_VISIBLE_GEOSETS;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    this->UnoptimizeVisibleGeometry();
    this->m_fFlags &= ~0x10u;

    CM2Shared* shared = this->m_shared;
    uint32_t cacheFlags = shared->m_cache->m_flags;
    M2SkinProfile* skin = shared->m_skinData;
    M2Batch* prevBatch = nullptr;
    M2SkinSection* prevSection = nullptr;
    uint32_t batchCount = 0;
    uint32_t sectionCount = 0;
    uint32_t visibleCount = 0;
    int32_t keepVertices = (cacheFlags & 8) || (shared->m_data->m_bones.m_count == 1 && (cacheFlags & 0x40));

    for (uint32_t i = 0; i < skin->m_batches.m_count; i++) {
        M2Batch* batch = &skin->m_batches[i];
        uint16_t sectionIndex = batch->m_skinSectionIndex;

        if (this->m_skinSections[sectionIndex]) {
            visibleCount++;
            M2SkinSection* section = &skin->m_skinSections[sectionIndex];

            if (!prevBatch || !BatchesMatch(batch, prevBatch, section, prevSection)) {
                batchCount++;
                sectionCount++;
                prevBatch = batch;
                prevSection = section;
            }
        }
    }

    if (visibleCount <= batchCount) {
        return;
    }

    M2ModelOptGeo* optGeo = static_cast<M2ModelOptGeo*>(SMemAlloc((sizeof(M2Batch) + sizeof(CShaderEffect*)) * batchCount + (sizeof(M2SkinSection) + sizeof(M2BatchRange)) * sectionCount + sizeof(M2ModelOptGeo), __FILE__, __LINE__, 0));

    if (!optGeo) {
        return;
    }

    optGeo->m_batches = reinterpret_cast<M2Batch*>(optGeo + 1);
    optGeo->m_skinSections = reinterpret_cast<M2SkinSection*>(optGeo->m_batches + batchCount);
    optGeo->m_batchRanges = nullptr;
    optGeo->m_indexBuf = nullptr;
    optGeo->m_indexPool = nullptr;

    M2BatchRange* ranges = reinterpret_cast<M2BatchRange*>(optGeo->m_skinSections + sectionCount);

    for (uint32_t i = 0; i < sectionCount; i++) {
        new (&optGeo->m_skinSections[i]) M2SkinSection();
    }

    optGeo->m_batchRanges = ranges;
    CShaderEffect** effects = reinterpret_cast<CShaderEffect**>(ranges + sectionCount);

    for (uint32_t i = 0; i < sectionCount; i++) {
        new (&optGeo->m_batchRanges[i]) M2BatchRange();
    }

    optGeo->m_effects = effects;
    this->m_optGeo = optGeo;

    M2Batch* lastBatch = nullptr;
    M2SkinSection* lastSection = nullptr;
    M2SkinSection* target = nullptr;
    M2BatchRange* range = nullptr;
    uint32_t indexCount = 0;
    batchCount = 0;
    sectionCount = 0;

    int32_t* indexStarts = static_cast<int32_t*>(alloca(sizeof(int32_t) * skin->m_skinSections.m_count));

    if (skin->m_skinSections.m_count) {
        memset(indexStarts, 0xFF, sizeof(int32_t) * skin->m_skinSections.m_count);
    }

    for (uint32_t i = 0; i < skin->m_batches.m_count; i++) {
        M2Batch* batch = &skin->m_batches[i];
        uint16_t sectionIndex = batch->m_skinSectionIndex;

        if (!this->m_skinSections[sectionIndex]) {
            continue;
        }

        M2SkinSection* section = &skin->m_skinSections[sectionIndex];

        if (lastBatch && BatchesMatch(batch, lastBatch, section, lastSection)) {
            if (keepVertices) {
                if (target->m_vertexStart > section->m_vertexStart) {
                    target->m_vertexCount += target->m_vertexStart - section->m_vertexStart;
                    target->m_vertexStart = section->m_vertexStart;
                }

                if (target->m_vertexCount + target->m_vertexStart < section->m_vertexStart + section->m_vertexCount) {
                    target->m_vertexCount = section->m_vertexStart + section->m_vertexCount - target->m_vertexStart;
                }
            } else {
                target->m_vertexCount += section->m_vertexCount;
            }

            target->m_indexCount += section->m_indexCount;

            if (target->m_boneCount < section->m_boneCount) {
                target->m_boneCount = section->m_boneCount;
            }

            if (target->m_boneInfluences < section->m_boneInfluences) {
                target->m_boneInfluences = section->m_boneInfluences;
            }

            range->m_last = i;
        } else {
            M2Batch* newBatch = &optGeo->m_batches[batchCount];
            target = &optGeo->m_skinSections[sectionCount];
            range = &optGeo->m_batchRanges[sectionCount];
            *newBatch = *batch;
            newBatch->m_skinSectionIndex = sectionCount;
            lastSection = section;
            *target = *section;
            target->m_skinSectionId = sectionCount;
            lastBatch = batch;

            int32_t indexStart = indexStarts[batch->m_skinSectionIndex];

            if (indexStart < 0) {
                indexStart = static_cast<uint16_t>(indexCount);
                indexStarts[batch->m_skinSectionIndex] = indexCount;
            }

            target->m_indexStart = indexStart;

            if (!keepVertices) {
                target->m_vertexStart = 0;
            }

            range->m_last = i;
            range->m_first = i;
            batchCount++;
            sectionCount++;
        }

        indexCount += section->m_indexCount;
    }

    optGeo->m_batchCount = batchCount;
    optGeo->m_skinSectionCount = sectionCount;

    for (uint32_t i = 0; i < batchCount; i++) {
        optGeo->m_effects[i] = this->m_shared->GetEffect(&optGeo->m_batches[i]);
    }

    optGeo->m_indexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, sizeof(uint16_t) * indexCount, GxPoolHintBit_Unk0, this->m_shared->m_fileNameWithoutPath);
    optGeo->m_indexBuf = g_theGxDevicePtr->BufCreate(optGeo->m_indexPool, sizeof(uint16_t), indexCount, 0);

    if (!optGeo->m_indexPool || !optGeo->m_indexBuf) {
        this->UnoptimizeVisibleGeometry();
    }
}

// VERIFY: aa052318 manual
// OFFSET: 0x0082CED0
void CM2Model::GetSequenceInfo(uint32_t id, uint32_t variation, M2SequenceInfo& info) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2SequenceFallback fallback;
    this->SequenceFallbackById(&fallback, id);
    info.m_fallbackMode = fallback.m_mode;
    info.m_animId = fallback.m_id;

    M2Data* data = this->m_shared->m_data;
    uint16_t index = this->FindSequenceIndex(data, fallback.m_id, variation);

    if (index == 0xFFFF) {
        info.m_moveSpeed = 0.0f;
        info.m_flags = 0;
        info.m_duration = 0;
        info.m_bounds.b = C3Vector(0.0f, 0.0f, 0.0f);
        info.m_bounds.t = C3Vector(0.0f, 0.0f, 0.0f);
        info.m_center = C3Vector(0.0f, 0.0f, 0.0f);
        info.m_radius = 0.0f;
        return;
    }

    M2Sequence* sequence = &data->m_sequences[index];
    info.m_flags = sequence->m_flags;
    info.m_duration = sequence->m_duration;
    float moveSpeed = sequence->m_moveSpeed;
    info.m_moveSpeed = moveSpeed;

    if (this->m_fFlags & 0x8000) {
        if (this->m_attachParent) {
            const C44Matrix& m = this->m_matrixF4;
            const C44Matrix& v = this->m_scene->m_viewInv;
            float z = v.a2 * m.a0 + (v.b2 * m.a1 + v.d2 * m.a3 + v.c2 * m.a2);
            float y = v.c1 * m.a2 + m.a3 * v.d1 + m.a1 * v.b1 + v.a1 * m.a0;
            float x = v.c0 * m.a2 + v.d0 * m.a3 + m.a1 * v.b0 + v.a0 * m.a0;
            info.m_moveSpeed = sqrtf(x * x + y * y + z * z) * moveSpeed;
        } else {
            const C44Matrix& m = this->m_worldTransform;
            info.m_moveSpeed = moveSpeed * sqrtf(m.a2 * m.a2 + m.a1 * m.a1 + m.a0 * m.a0);
        }
    }

    info.m_bounds = sequence->m_bounds.m_extent;
    info.m_center.x = (sequence->m_bounds.m_extent.b.x + sequence->m_bounds.m_extent.t.x) * 0.5f;
    info.m_center.y = (sequence->m_bounds.m_extent.t.y + sequence->m_bounds.m_extent.b.y) * 0.5f;
    info.m_center.z = 0.5f * (sequence->m_bounds.m_extent.t.z + sequence->m_bounds.m_extent.b.z);
    info.m_radius = sequence->m_bounds.m_radius;
}

// VERIFY: 349f9473 manual
// OFFSET: 0x0082D1A0
uint32_t CM2Model::GetNumBatches() {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    uint32_t batches = this->GetNumGeometryBatches();
    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
        M2ModelRibbon* ribbon = &this->m_ribbons[i];

        if (ribbon->m_visibilityTrack.m_currentValue) {
            float alpha = this->m_effectiveAlpha;

            if (data->m_ribbons[i].m_alphaTrack.m_sequenceTimes.m_count) {
                alpha *= ribbon->m_alphaTrack.m_currentValue;
            }

            if (alpha >= 0.000099999997f) {
                batches += this->m_ribbonEmitters[i]->GetNumBatches();
            }
        }

        data = this->m_shared->m_data;
    }

    for (uint32_t i = 0; i < this->m_shared->m_data->m_particles.m_count; i++) {
        if (this->m_particles[i].m_active && this->m_effectiveAlpha >= 0.000099999997f) {
            batches += this->m_particleEmitters[i]->GetNumBatches();
        }
    }

    return batches;
}

// VERIFY: 55912be9 manual
// OFFSET: 0x0082D2F0
void CM2Model::AnimateParticlesMT() {
    for (uint32_t i = 0; i < this->m_shared->m_data->m_particles.m_count; i++) {
        M2Particle* particleData = &this->m_shared->m_data->m_particles[i];
        CParticleEmitter2* emitter = this->m_particleEmitters[i];
        M2ModelParticle* particle = &this->m_particles[i];
        uint32_t count = particleData->m_visibilityTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_visibilityTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            uint8_t visible = 1;
            M2AnimateTrack<uint8_t, uint8_t>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_visibilityTrack, particle->m_visibilityTrack, visible);
        }

        particle->m_emitting = particle->m_visibilityTrack.m_currentValue && (emitter->m_flags & 2);
        particle->m_active = particle->m_emitting || emitter->HasLiveParticles();
        this->m_fFlags |= static_cast<uint32_t>(particle->m_active) << 10;

        if (!particle->m_visibilityTrack.m_currentValue && this->m_uint90) {
            continue;
        }

        if (particleData->m_speedTrack.m_sequenceTimes.m_count > 1 || (particleData->m_speedTrack.m_sequenceTimes.m_count == 1 && particleData->m_speedTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_speedTrack, particle->m_speedTrack, value);
        }

        if (particleData->m_variationTrack.m_sequenceTimes.m_count > 1 || (particleData->m_variationTrack.m_sequenceTimes.m_count == 1 && particleData->m_variationTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_variationTrack, particle->m_variationTrack, value);
        }

        if (particleData->m_latitudeTrack.m_sequenceTimes.m_count > 1 || (particleData->m_latitudeTrack.m_sequenceTimes.m_count == 1 && particleData->m_latitudeTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_latitudeTrack, particle->m_latitudeTrack, value);
        }

        if (particleData->m_longitudeTrack.m_sequenceTimes.m_count > 1 || (particleData->m_longitudeTrack.m_sequenceTimes.m_count == 1 && particleData->m_longitudeTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_longitudeTrack, particle->m_longitudeTrack, value);
        }

        if (particleData->m_gravityTrack.m_sequenceTimes.m_count > 1 || (particleData->m_gravityTrack.m_sequenceTimes.m_count == 1 && particleData->m_gravityTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_gravityTrack, particle->m_gravityTrack, value);
        }

        if (particleData->m_lifeTrack.m_sequenceTimes.m_count > 1 || (particleData->m_lifeTrack.m_sequenceTimes.m_count == 1 && particleData->m_lifeTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_lifeTrack, particle->m_lifeTrack, value);
        }

        if (particleData->m_emissionRateTrack.m_sequenceTimes.m_count > 1 || (particleData->m_emissionRateTrack.m_sequenceTimes.m_count == 1 && particleData->m_emissionRateTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_emissionRateTrack, particle->m_emissionRateTrack, value);
        }

        if (particleData->m_widthTrack.m_sequenceTimes.m_count > 1 || (particleData->m_widthTrack.m_sequenceTimes.m_count == 1 && particleData->m_widthTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_widthTrack, particle->m_widthTrack, value);
        }

        if (particleData->m_lengthTrack.m_sequenceTimes.m_count > 1 || (particleData->m_lengthTrack.m_sequenceTimes.m_count == 1 && particleData->m_lengthTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_lengthTrack, particle->m_lengthTrack, value);
        }

        if (particleData->m_zsourceTrack.m_sequenceTimes.m_count > 1 || (particleData->m_zsourceTrack.m_sequenceTimes.m_count == 1 && particleData->m_zsourceTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[particleData->m_boneIndex], particleData->m_zsourceTrack, particle->m_zsourceTrack, value);
        }
    }
}

// VERIFY: 37a58209 manual
// OFFSET: 0x0082D6F0
void CM2Model::AnimateTextureTransformsMT() {
    static C3Vector center(0.5f, 0.5f, 0.0f);

    for (uint32_t i = 0; i < this->m_shared->m_data->m_textureTransforms.m_count; i++) {
        C44Matrix* matrix = &this->m_textureMatrices[i];
        M2TextureTransform* transformData = &this->m_shared->m_data->m_textureTransforms[i];
        M2ModelTextureTransform* transform = &this->m_textureTransforms[i];

        matrix->d3 = 1.0f;
        matrix->c2 = 1.0f;
        matrix->b1 = 1.0f;
        matrix->a0 = 1.0f;
        matrix->d2 = 0.0f;
        matrix->d1 = 0.0f;
        matrix->d0 = 0.0f;
        matrix->c3 = 0.0f;
        matrix->c1 = 0.0f;
        matrix->c0 = 0.0f;
        matrix->b3 = 0.0f;
        matrix->b2 = 0.0f;
        matrix->b0 = 0.0f;
        matrix->a3 = 0.0f;
        matrix->a2 = 0.0f;
        matrix->a1 = 0.0f;

        if (transformData->m_rotation.m_sequenceTimes.m_count) {
            C4Quaternion rotation;
            rotation.x = 0.0f;
            rotation.y = 0.0f;
            rotation.z = 0.0f;
            rotation.w = 1.0f;
            M2AnimateTrack<C4Quaternion, C4Quaternion>(this, this->m_bones, transformData->m_rotation, transform->m_rotation, rotation);
            matrix->Translate(center);
            matrix->Rotate(transform->m_rotation.m_currentValue);
            matrix->Translate(C3Vector(-center.x, -center.y, -center.z));
        }

        if (transformData->m_scaling.m_sequenceTimes.m_count) {
            C3Vector scale(1.0f, 1.0f, 1.0f);
            M2AnimateTrack<C3Vector, C3Vector>(this, this->m_bones, transformData->m_scaling, transform->m_scaling, scale);
            matrix->Translate(center);
            matrix->Scale(transform->m_scaling.m_currentValue);
            matrix->Translate(C3Vector(-center.x, -center.y, -center.z));
        }

        if (transformData->m_translation.m_sequenceTimes.m_count) {
            C3Vector translation(0.0f, 0.0f, 0.0f);
            M2AnimateTrack<C3Vector, C3Vector>(this, this->m_bones, transformData->m_translation, transform->m_translation, translation);
            matrix->Translate(transform->m_translation.m_currentValue);
        }
    }
}

// VERIFY: bbe25130 manual
// OFFSET: 0x0082D910
int32_t CM2Model::SetVerticesMultiSample(int32_t optimized, M2SkinSection* section) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(M2VertexPNT2), section->m_vertexCount);
    M2VertexPNT2* vertices = reinterpret_cast<M2VertexPNT2*>(g_theGxDevicePtr->BufLock(buf));

    if (!vertices) {
        return 0;
    }

    if (optimized) {
        M2SkinProfile* skin = this->m_shared->m_skinData;
        M2BatchRange* range = &this->m_optGeo->m_batchRanges[section->m_skinSectionId];

        for (uint32_t b = range->m_first; b <= range->m_last; b++) {
            uint16_t sectionIndex = skin->m_batches[b].m_skinSectionIndex;

            if (this->m_skinSections[sectionIndex]) {
                M2SkinSection* batchSection = &skin->m_skinSections[sectionIndex];
                CM2Model::TransformVerticesNoUVSelect_cpp(this, batchSection, vertices);
                vertices += batchSection->m_vertexCount;
            }
        }
    } else {
        CM2Model::TransformVerticesNoUVSelect_cpp(this, section, vertices);
    }

    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
    GxPrimVertexPtr(buf, GxVBF_PNT2);
    return 1;
}

// VERIFY: 7330ed7c manual
// OFFSET: 0x0082DA40
void CM2Model::RenderModelBatchesShadowMap(const CModelBatchList& untextured, const CModelBatchList& textured) {
    GxRsSet(GxRs_BlendingMode, static_cast<int32_t>(0));
    CShaderEffect::SetAlphaRef(0.0f);
    CShaderEffect::SetDefaultShaders(0);
    CM2Model::RenderModelBatchListShadowMap(1, untextured);
    CShaderEffect::SetAlphaRef(0.50196081f);
    CShaderEffect::SetDefaultShaders(0);
    CM2Model::RenderModelBatchListShadowMap(0, textured);
}

// VERIFY: d9ea968f manual
// OFFSET: 0x0082DD80
void CM2Model::SetWorldTransform(const C3Vector& position, float rotation, float scale, const C3Vector& up, uint32_t flags) {
    C44Matrix& transform = this->m_worldTransform;
    transform.d3 = 1.0f;
    transform.c2 = 1.0f;
    transform.b1 = 1.0f;
    transform.a0 = 1.0f;
    transform.d2 = 0.0f;
    transform.d1 = 0.0f;
    transform.d0 = 0.0f;
    transform.c3 = 0.0f;
    transform.c1 = 0.0f;
    transform.c0 = 0.0f;
    transform.b3 = 0.0f;
    transform.b2 = 0.0f;
    transform.b0 = 0.0f;
    transform.a3 = 0.0f;
    transform.a2 = 0.0f;
    transform.a1 = 0.0f;
    transform.RotateAroundZ(rotation);
    transform.d0 = position.x;
    transform.d1 = position.y;
    transform.d2 = position.z;

    C44Matrix original = transform;

    if (flags == 0xFFFFFFFF) {
        flags = this->m_shared->m_data->m_flags;
    }

    C3Vector& rowA = *reinterpret_cast<C3Vector*>(&transform.a0);
    C3Vector& rowB = *reinterpret_cast<C3Vector*>(&transform.b0);
    C3Vector& rowC = *reinterpret_cast<C3Vector*>(&transform.c0);

    switch (flags & 3) {
        case 1: {
            float x = rowA.x;
            rowB.x = -rowA.y;
            rowB.y = x;
            rowB.z = 0.0f;
            rowB.NormalizeSafe();
            float ax = rowB.y * up.z - up.y * rowB.z;
            float ay = up.x * rowB.z - rowB.x * up.z;
            rowA.x = ax;
            rowA.y = ay;
            rowA.z = up.y * rowB.x - up.x * rowB.y;
            rowA.NormalizeSafe();
            float cx = rowA.y * rowB.z - rowB.y * rowA.z;
            float cy = rowB.x * rowA.z - rowB.z * rowA.x;
            rowC.x = cx;
            rowC.y = cy;
            rowC.z = rowB.y * rowA.x - rowA.y * rowB.x;
            break;
        }

        case 3: {
            ComputeWorldTransformBasis(&transform, up);
            break;
        }
    }

    if (this->m_fFlags & 1) {
        M2BoneSequenceInfo state;
        this->GetBoneSequenceInfo(0xFFFFFFFF, state);

        if (state.m_sequenceId != 0xFFFFFFFF) {
            M2SequenceInfo info;
            info.m_bounds.b = C3Vector(0.0f, 0.0f, 0.0f);
            info.m_bounds.t = C3Vector(0.0f, 0.0f, 0.0f);
            info.m_center = C3Vector(0.0f, 0.0f, 0.0f);
            info.m_radius = 0.0f;
            this->GetSequenceInfo(state.m_sequenceId, state.m_variationIndex, info);

            float blend = 0.0f;
            uint32_t mode = info.m_flags & 0xE;
            int32_t apply = 0;

            if (mode == 2 || mode == 4) {
                float speed = state.m_speed;

                if (speed == 0.0f) {
                    blend = 1.0f;
                } else if (speed > 0.0f) {
                    int32_t halfDuration = (state.m_endTime - state.m_startTime) >> 1;
                    float t = FloatToInt64(state.m_time / speed) / static_cast<float>(halfDuration);

                    if (t >= 0.0f && t >= 1.0f) {
                        t = 1.0f;
                    } else if (t < 0.0f) {
                        t = 0.0f;
                    }

                    blend = t;
                }

                if (info.m_flags & 4) {
                    blend = 1.0f - blend;
                }

                apply = blend > 0.0f;
            } else if (mode == 8) {
                blend = 1.0f;
                apply = 1;
            }

            if (apply) {
                ComputeWorldTransformBasis(&original, up);
                transform = original * blend + transform * (1.0f - blend);
            }
        }
    }

    transform.Scale(scale);
    this->m_fFlags |= 0x8000;
}

// VERIFY: 577fb428 manual
// OFFSET: 0x0082E0C0
int32_t CM2Model::HasDeferredSequence(uint16_t boneIndex, bool primary) {
    TSList<CM2SequenceLoad>& loads = this->m_shared->m_sequenceLoads;

    for (CM2SequenceLoad* load = loads.Head(); !loads.IsEnd(load); load = loads.RawNext(load)) {
        for (CM2SequencePlayBack* playback = load->m_playbacks.Head(); !load->m_playbacks.IsEnd(playback); playback = load->m_playbacks.RawNext(playback)) {
            if (playback->m_model == this && playback->m_boneIndex == boneIndex && ((playback->m_flags & 2) != 0) == primary) {
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: 3115a6f8 manual
// OFFSET: 0x0082E140
void CM2Model::AnimateMTSimple(const C44Matrix* view, const C3Vector& diffuse, const C3Vector& emissive, float alpha, float alpha2) {
    uint32_t flags = this->m_fFlags;

    if (!(flags & 1) || this->m_frameStamp == this->m_scene->m_frameStamp) {
        return;
    }

    M2Data* data = this->m_shared->m_data;
    CM2Model* parent = this->m_attachParent;

    if (parent) {
        uint32_t drawn = (parent->m_fFlags & 8) && (flags & 0x80);
        this->m_fFlags = (this->m_fFlags & ~8u) | (drawn << 3);
        uint32_t particles = (parent->m_fFlags & 0x10000) && (this->m_fFlags & 0x20000);
        this->m_fFlags = (this->m_fFlags & ~0x10000u) | (particles << 16);
        this->m_frameOfReference = parent->m_frameOfReference;
    }

    if (data->m_flags & 4) {
        this->m_effectiveAlpha = this->m_fadeAlpha;
        this->m_diffuseColor.x = this->m_diffuseTintR;
        this->m_diffuseColor.y = this->m_diffuseTintG;
        this->m_diffuseColor.z = this->m_diffuseTintB;
        this->m_alpha = this->m_baseAlpha * this->m_fadeAlpha;
        this->m_emissiveColor.x = this->m_emissiveTintR;
        this->m_emissiveColor.y = this->m_emissiveTintG;
        this->m_emissiveColor.z = this->m_emissiveTintB;
    } else {
        this->m_diffuseColor.x = diffuse.x * this->m_diffuseTintR;
        this->m_diffuseColor.y = this->m_diffuseTintG * diffuse.y;
        this->m_diffuseColor.z = this->m_diffuseTintB * diffuse.z;
        this->m_emissiveColor.x = this->m_emissiveTintR;
        this->m_emissiveColor.y = this->m_emissiveTintG;
        this->m_emissiveColor.z = this->m_emissiveTintB;

        uint32_t current = this->m_fFlags;

        if (current & 0x100000) {
            this->m_effectiveAlpha = this->m_fadeAlpha;
        } else {
            this->m_effectiveAlpha = alpha * this->m_fadeAlpha;
        }

        this->m_alpha = this->m_baseAlpha * alpha2 * this->m_fadeAlpha;

        if (!(current & 0x80000)) {
            this->m_emissiveColor.x = this->m_emissiveColor.x + emissive.x;
            this->m_emissiveColor.y = emissive.y + this->m_emissiveColor.y;
            this->m_emissiveColor.z = emissive.z + this->m_emissiveColor.z;
        }
    }

    for (uint32_t i = 0; i < data->m_loops.m_count; i++) {
        uint32_t length = data->m_loops[i].m_length;
        uint32_t time = 0;

        if (length) {
            time = (this->m_scene->m_time - this->m_loopOrigin) % length;
        }

        this->m_loops[i] = time;
    }

    this->m_matrixF4 = this->m_worldTransform * *view;

    CM2Model* attachParent = this->m_attachParent;

    if (!attachParent || (attachParent->m_flags & 1)) {
        this->m_scaleSq = this->m_matrixF4.d2 * this->m_matrixF4.d2 + this->m_matrixF4.d1 * this->m_matrixF4.d1 + this->m_matrixF4.d0 * this->m_matrixF4.d0;
    } else {
        this->m_scaleSq = attachParent->m_scaleSq;
    }

    uint32_t delta = 0;

    if (this->m_lastAnimTime && this->m_scene->m_time) {
        uint32_t time = this->m_scene->m_time;
        delta = time - this->m_lastAnimTime;
        this->m_lastAnimTime = time;
    }

    M2ModelBone* bone = this->m_bones;
    uint16_t sequenceIndex = bone->m_sequence.m_sequenceIndex;

    if (sequenceIndex != 0xFFFF) {
        if (this->m_lastAnimTime) {
            bone->m_sequence.m_startTime += delta;
            bone->m_sequence.m_endTime += delta;
        }

        uint32_t now = this->m_scene->m_time;
        M2Sequence* sequence = &data->m_sequences[sequenceIndex];
        uint32_t time;

        if ((sequence->m_flags & 1) && static_cast<int32_t>(bone->m_sequence.m_endTime - now) <= 0) {
            int32_t elapsed = bone->m_sequence.m_endTime - bone->m_sequence.m_startTime;
            int32_t current = static_cast<int32_t>(elapsed * bone->m_sequence.m_speed) + bone->m_sequence.m_startOffset;

            if (current < 0) {
                time = 0;
            } else if (static_cast<uint32_t>(current) > sequence->m_duration) {
                time = sequence->m_duration;
            } else {
                time = current;
            }
        } else {
            if ((sequence->m_flags & 1) && static_cast<int32_t>(bone->m_sequence.m_startTime - now) > 0) {
                now = bone->m_sequence.m_startTime;
            }

            uint32_t duration = sequence->m_duration;
            time = 0;

            if (duration) {
                int32_t elapsed = now - bone->m_sequence.m_startTime;
                time = static_cast<uint32_t>(static_cast<int32_t>(elapsed * bone->m_sequence.m_speed) + bone->m_sequence.m_startOffset) % duration;
            }
        }

        bone->m_sequence.m_currentTime = time;
        bone->m_sequence.m_animIndex = bone->m_sequence.m_sequenceIndex;
        bone->m_sequence.m_sourceBoneIndex = 0;
    }

    *this->m_boneMatrices = this->m_matrixF4;

    for (uint32_t i = 0; i < data->m_textureWeights.m_count; i++) {
        M2TextureWeight* weight = &data->m_textureWeights[i];
        uint32_t count = weight->m_weight.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && weight->m_weight.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float value = 1.0f;
            M2AnimateTrack<fixed16, float>(this, this->m_bones, weight->m_weight, this->m_textureWeights[i], value);
        }
    }

    if (data->m_textureTransforms.m_count) {
        this->AnimateTextureTransformsMT();
    }

    this->m_fFlags &= ~0x400u;

    if (this->m_attachments || this->m_attachList) {
        this->AnimateAttachmentsMT();
    }

    this->m_frameStamp = this->m_scene->m_frameStamp;
}

// VERIFY: 932496a8 manual
// OFFSET: 0x0082E550
void CM2Model::AnimateAttachmentsMT() {
    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_attachments.m_count; i++) {
        M2Attachment* attachment = &data->m_attachments[i];
        uint32_t count = attachment->m_animateAttached.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && attachment->m_animateAttached.m_sequenceTimes[0].m_count > this->m_uint90)) {
            uint8_t visible = 1;
            M2AnimateTrack<uint8_t, uint8_t>(this, &this->m_bones[attachment->m_bone], attachment->m_animateAttached, this->m_attachments[i], visible);
        }

        data = this->m_shared->m_data;
    }

    for (CM2Model* child = this->m_attachList; child; child = child->m_attachNext) {
        C44Matrix matrix;

        if (child->m_attachmentIndex == 0xFFFF) {
            if (!(child->m_fFlags & 0x40000)) {
                continue;
            }

            matrix = *this->m_boneMatrices;
        } else {
            if (!this->m_attachments[child->m_attachmentIndex].m_currentValue) {
                continue;
            }

            M2Attachment* attachment = &this->m_shared->m_data->m_attachments[child->m_attachmentIndex];
            matrix = this->m_boneMatrices[attachment->m_bone];
            matrix.Translate(attachment->m_position);
        }

        if (child->m_fFlags & 0x1000) {
            child->AnimateMTSimple(&matrix, this->m_diffuseColor, this->m_emissiveColor, this->m_effectiveAlpha, this->m_alpha);
        } else {
            child->AnimateMT(&matrix, this->m_diffuseColor, this->m_emissiveColor, this->m_effectiveAlpha, this->m_alpha);
        }
    }
}

// VERIFY: a16ddee7 auto
// OFFSET: 0x0082E720
int32_t __stdcall CM2Model::SequenceCallback(void* callback, CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint32_t sequenceIndex, uint32_t startTime, uint64_t param) {
    if (CM2Model::s_sequenceCallbacks.Count() >= 0x10000) {
        return 0;
    }

    M2ModelCallback* entry = CM2Model::s_sequenceCallbacks.New();

    if (!entry) {
        return 0;
    }

    entry->m_callback = callback;
    entry->m_model = model;
    entry->m_boneId = boneId;
    entry->m_id = sequenceId;
    entry->m_value = interrupted;
    entry->m_time = time;
    entry->m_args[0] = sequenceIndex;
    entry->m_args[1] = startTime;
    entry->m_type = 0;
    entry->m_param = param;
    return 1;
}

// VERIFY: 7336813c manual
// OFFSET: 0x0082E790
uint32_t CM2Model::ProcessSequenceCallback(uint32_t boneIndex, uint32_t frame, uint32_t time, uint32_t lastFrame) {
    M2Data* data = this->m_shared->m_data;
    M2ModelBone* bone = &this->m_bones[boneIndex];

    if (data->m_bones[boneIndex].m_boneId != 0xFFFFFFFF || !boneIndex) {
        if (frame - time != lastFrame) {
            CM2Model::s_sequenceCallbacks.SetCount(0);
            lastFrame = frame - time;
        }

        M2CompBone* compBone = &data->m_bones[boneIndex];
        uint32_t boneId = compBone->m_parentIndex == 0xFFFF ? 0xFFFFFFFF : compBone->m_boneId;
        CM2Model::SequenceCallback(this->m_sequenceCallback, this, boneId, bone->m_sequenceId, 0, time, bone->m_sequence.m_sequenceIndex, bone->m_sequence.m_startTime, this->m_sequenceCallbackParam);
    }

    return lastFrame;
}

// VERIFY: 745168f8 manual
// OFFSET: 0x0082EC30
void CM2Model::GetCollisionFacets(const CAaBox& box, const C44Matrix& transform, TSGrowableArray<CFacet>& facets) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;

    if (CM2Model::s_collisionPositions.Count() < data->m_collisionPositions.m_count) {
        CM2Model::s_collisionPositions.SetCount(data->m_collisionPositions.m_count);
    }

    if (CM2Model::s_collisionCodes.Count() < data->m_collisionPositions.m_count) {
        CM2Model::s_collisionCodes.SetCount(data->m_collisionPositions.m_count);
    }

    C3Vector* sourcePositions = data->m_collisionPositions.Data();
    uint32_t positionCount = data->m_collisionPositions.m_count;

    for (uint32_t i = 0; i < positionCount; i++) {
        C3Vector* position = &CM2Model::s_collisionPositions.m_data[i];
        *position = sourcePositions[i] * transform;
        uint32_t* code = &CM2Model::s_collisionCodes.m_data[i];
        *code = 0;

        if (box.b.x <= position->x) {
            if (box.t.x < position->x) {
                *code = 2;
            }
        } else {
            *code = 1;
        }

        if (box.b.y <= position->y) {
            if (box.t.y < position->y) {
                *code |= 8;
            }
        } else {
            *code |= 4;
        }

        if (box.b.z <= position->z) {
            if (box.t.z < position->z) {
                *code |= 0x20;
            }
        } else {
            *code |= 0x10;
        }
    }

    C33Matrix rotation(transform);
    float length = rotation.a1 * rotation.a1 + rotation.a2 * rotation.a2 + rotation.a0 * rotation.a0;

    if (length > 0.00000023841858f) {
        float scale = 1.0f / sqrtf(length);
        rotation.a0 = rotation.a0 * scale;
        rotation.a1 = rotation.a1 * scale;
        rotation.a2 = rotation.a2 * scale;
    }

    length = rotation.b2 * rotation.b2 + rotation.b1 * rotation.b1 + rotation.b0 * rotation.b0;

    if (length > 0.00000023841858f) {
        float scale = 1.0f / sqrtf(length);
        rotation.b0 = rotation.b0 * scale;
        rotation.b1 = rotation.b1 * scale;
        rotation.b2 = rotation.b2 * scale;
    }

    length = rotation.c2 * rotation.c2 + rotation.c1 * rotation.c1 + rotation.c0 * rotation.c0;

    if (0.00000023841858f < length) {
        float scale = 1.0f / sqrtf(length);
        rotation.c0 = rotation.c0 * scale;
        rotation.c1 = rotation.c1 * scale;
        rotation.c2 = rotation.c2 * scale;
    }

    uint32_t* codes = CM2Model::s_collisionCodes.m_data;
    uint32_t count = facets.Count();

    for (uint32_t i = 0; i < data->m_collisionFaceNormals.m_count; i++) {
        uint16_t* indices = &data->m_collisionIndices[i * 3];

        if (!(codes[indices[1]] & codes[indices[2]] & codes[indices[0]])) {
            count++;
        }
    }

    facets.Reserve(count, 0);
    codes = CM2Model::s_collisionCodes.m_data;

    for (uint32_t i = 0; i < data->m_collisionFaceNormals.m_count; i++) {
        uint16_t* indices = &data->m_collisionIndices[i * 3];

        if (codes[indices[1]] & codes[indices[2]] & codes[indices[0]]) {
            continue;
        }

        CFacet* facet = facets.New();
        facet->v[0] = CM2Model::s_collisionPositions.m_data[indices[0]];
        facet->v[1] = CM2Model::s_collisionPositions.m_data[indices[1]];
        facet->v[2] = CM2Model::s_collisionPositions.m_data[indices[2]];

        C3Vector& normal = data->m_collisionFaceNormals[i];
        facet->plane.n.x = normal.y * rotation.b0 + normal.z * rotation.c0 + normal.x * rotation.a0;
        facet->plane.n.y = normal.z * rotation.c1 + normal.x * rotation.a1 + normal.y * rotation.b1;
        facet->plane.n.z = normal.x * rotation.a2 + normal.y * rotation.b2 + normal.z * rotation.c2;
        facet->plane.d = -(facet->v[0].z * facet->plane.n.z + facet->v[0].y * facet->plane.n.y + facet->v[0].x * facet->plane.n.x);
        codes = CM2Model::s_collisionCodes.m_data;
    }
}

// VERIFY: d48ec2da manual
// OFFSET: 0x0082F0F0
void CM2Model::AnimateMT(const C44Matrix* view, const C3Vector& diffuse, const C3Vector& emissive, float alpha, float alpha2) {
    uint32_t flags = this->m_fFlags;

    if (!(flags & 1) || this->m_frameStamp == this->m_scene->m_frameStamp) {
        return;
    }

    M2Data* data = this->m_shared->m_data;
    CM2Model* parent = this->m_attachParent;

    if (parent) {
        uint32_t drawn = (parent->m_fFlags & 8) && (flags & 0x80);
        this->m_fFlags = (this->m_fFlags & ~8u) | (drawn << 3);
        uint32_t particles = (parent->m_fFlags & 0x10000) && (this->m_fFlags & 0x20000);
        this->m_fFlags = (this->m_fFlags & ~0x10000u) | (particles << 16);
        this->m_frameOfReference = parent->m_frameOfReference;
    }

    if (data->m_flags & 4) {
        this->m_effectiveAlpha = this->m_fadeAlpha;
        this->m_diffuseColor.x = this->m_diffuseTintR;
        this->m_diffuseColor.y = this->m_diffuseTintG;
        this->m_diffuseColor.z = this->m_diffuseTintB;
        this->m_alpha = this->m_baseAlpha * this->m_fadeAlpha;
        this->m_emissiveColor.x = this->m_emissiveTintR;
        this->m_emissiveColor.y = this->m_emissiveTintG;
        this->m_emissiveColor.z = this->m_emissiveTintB;
    } else {
        this->m_diffuseColor.x = diffuse.x * this->m_diffuseTintR;
        this->m_diffuseColor.y = this->m_diffuseTintG * diffuse.y;
        this->m_diffuseColor.z = this->m_diffuseTintB * diffuse.z;
        this->m_emissiveColor.x = this->m_emissiveTintR;
        this->m_emissiveColor.y = this->m_emissiveTintG;
        this->m_emissiveColor.z = this->m_emissiveTintB;

        uint32_t current = this->m_fFlags;

        if (current & 0x100000) {
            this->m_effectiveAlpha = this->m_fadeAlpha;
        } else {
            this->m_effectiveAlpha = alpha * this->m_fadeAlpha;
        }

        this->m_alpha = this->m_baseAlpha * alpha2 * this->m_fadeAlpha;

        if (!(current & 0x80000)) {
            this->m_emissiveColor.x = emissive.x + this->m_emissiveColor.x;
            this->m_emissiveColor.y = emissive.y + this->m_emissiveColor.y;
            this->m_emissiveColor.z = emissive.z + this->m_emissiveColor.z;
        }
    }

    for (uint32_t i = 0; i < data->m_loops.m_count; i++) {
        uint32_t length = data->m_loops[i].m_length;
        uint32_t time = 0;

        if (length) {
            time = (this->m_scene->m_time - this->m_loopOrigin) % length;
        }

        this->m_loops[i] = time;
    }

    this->m_matrixF4 = this->m_worldTransform * *view;

    CM2Model* attachParent = this->m_attachParent;

    if (!attachParent || (attachParent->m_flags & 1)) {
        this->m_scaleSq = this->m_matrixF4.d2 * this->m_matrixF4.d2 + this->m_matrixF4.d1 * this->m_matrixF4.d1 + this->m_matrixF4.d0 * this->m_matrixF4.d0;
    } else {
        this->m_scaleSq = attachParent->m_scaleSq;
    }

    C44Matrix parentTransform;
    C44Matrix local;
    C3Vector scale;
    uint32_t delta = 0;

    if (this->m_lastAnimTime && this->m_scene->m_time) {
        uint32_t time = this->m_scene->m_time;
        delta = time - this->m_lastAnimTime;
        this->m_lastAnimTime = time;
    }

    for (uint32_t i = 0; i < data->m_bones.m_count; i++) {
        M2ModelBone* bone = &this->m_bones[i];
        M2CompBone* boneData = &data->m_bones[i];

        if (bone->m_sequence.m_sequenceIndex != 0xFFFF) {
            if (this->m_lastAnimTime) {
                bone->m_sequence.m_startTime += delta;
                bone->m_sequence.m_endTime += delta;
            }

            uint32_t now = this->m_scene->m_time;
            M2Sequence* sequence = &data->m_sequences[bone->m_sequence.m_sequenceIndex];
            uint32_t time;

            if ((sequence->m_flags & 1) && static_cast<int32_t>(bone->m_sequence.m_endTime - now) <= 0) {
                int32_t elapsed = bone->m_sequence.m_endTime - bone->m_sequence.m_startTime;
                int32_t current = static_cast<int32_t>(elapsed * bone->m_sequence.m_speed) + bone->m_sequence.m_startOffset;

                if (current < 0) {
                    time = 0;
                } else if (static_cast<uint32_t>(current) > sequence->m_duration) {
                    time = sequence->m_duration;
                } else {
                    time = current;
                }
            } else {
                if ((sequence->m_flags & 1) && static_cast<int32_t>(bone->m_sequence.m_startTime - now) > 0) {
                    now = bone->m_sequence.m_startTime;
                }

                uint32_t duration = sequence->m_duration;
                time = 0;

                if (duration) {
                    int32_t elapsed = now - bone->m_sequence.m_startTime;
                    time = static_cast<uint32_t>(static_cast<int32_t>(elapsed * bone->m_sequence.m_speed) + bone->m_sequence.m_startOffset) % duration;
                }
            }

            bone->m_sequence.m_currentTime = time;
            bone->m_sequence.m_animIndex = bone->m_sequence.m_sequenceIndex;
            bone->m_sequence.m_sourceBoneIndex = i;
        } else {
            uint16_t parentIndex = boneData->m_parentIndex;

            if (parentIndex >= data->m_bones.m_count) {
                if (i) {
                    bone->m_sequence.m_currentTime = this->m_bones->m_sequence.m_currentTime;
                    bone->m_sequence.m_animIndex = this->m_bones->m_sequence.m_animIndex;
                    bone->m_sequence.m_sourceBoneIndex = this->m_bones->m_sequence.m_sourceBoneIndex;
                }
            } else {
                M2ModelBone* parentBone = &this->m_bones[parentIndex];
                bone->m_sequence.m_currentTime = parentBone->m_sequence.m_currentTime;
                bone->m_sequence.m_animIndex = parentBone->m_sequence.m_animIndex;
                bone->m_sequence.m_sourceBoneIndex = parentBone->m_sequence.m_sourceBoneIndex;
            }
        }

        if (bone->m_secondarySequence.m_sequenceIndex == 0xFFFF) {
            uint16_t parentIndex = boneData->m_parentIndex;

            if (parentIndex >= data->m_bones.m_count) {
                if (!i) {
                    bone->m_secondarySequence.m_currentTime = bone->m_sequence.m_currentTime;
                    bone->m_secondarySequence.m_animIndex = bone->m_sequence.m_animIndex;
                } else {
                    bone->m_secondarySequence.m_currentTime = this->m_bones->m_secondarySequence.m_currentTime;
                    bone->m_secondarySequence.m_animIndex = this->m_bones->m_secondarySequence.m_animIndex;
                }
            } else {
                M2ModelBone* parentBone = &this->m_bones[parentIndex];
                bone->m_secondarySequence.m_currentTime = parentBone->m_secondarySequence.m_currentTime;
                bone->m_secondarySequence.m_animIndex = parentBone->m_secondarySequence.m_animIndex;
            }
        } else {
            if (this->m_lastAnimTime) {
                bone->m_secondarySequence.m_startTime += delta;
                bone->m_secondarySequence.m_endTime += delta;
            }

            M2Sequence* sequence = &data->m_sequences[bone->m_secondarySequence.m_sequenceIndex];
            uint32_t now = this->m_scene->m_time;
            uint32_t time;

            if ((sequence->m_flags & 0x80) && static_cast<int32_t>(bone->m_secondarySequence.m_endTime - now) <= 0) {
                int32_t elapsed = bone->m_secondarySequence.m_endTime - bone->m_secondarySequence.m_startTime;
                int32_t current = static_cast<int32_t>(elapsed * bone->m_secondarySequence.m_speed) + bone->m_secondarySequence.m_startOffset;

                if (current < 0) {
                    time = 0;
                } else if (static_cast<uint32_t>(current) > sequence->m_duration) {
                    time = sequence->m_duration;
                } else {
                    time = current;
                }
            } else {
                if ((sequence->m_flags & 0x80) && static_cast<int32_t>(bone->m_secondarySequence.m_startTime - now) > 0) {
                    now = bone->m_secondarySequence.m_startTime;
                }

                uint32_t duration = sequence->m_duration;
                time = 0;

                if (duration) {
                    int32_t elapsed = now - bone->m_secondarySequence.m_startTime;
                    time = static_cast<uint32_t>(static_cast<int32_t>(elapsed * bone->m_secondarySequence.m_speed) + bone->m_secondarySequence.m_startOffset) % duration;
                }
            }

            bone->m_secondarySequence.m_currentTime = time;
            bone->m_secondarySequence.m_animIndex = bone->m_secondarySequence.m_sequenceIndex;

            if (static_cast<int32_t>(this->m_scene->m_time - bone->m_blendEndTime) >= 0) {
                bone->m_secondarySequence.m_sequenceIndex = 0xFFFF;
            }
        }

        if (bone->m_sequence.m_sequenceIndex == 0xFFFF && bone->m_secondarySequence.m_sequenceIndex == 0xFFFF) {
            uint16_t parentIndex = boneData->m_parentIndex;

            if (parentIndex < data->m_bones.m_count) {
                bone->m_blendFactor = this->m_bones[parentIndex].m_blendFactor;
            } else if (i) {
                bone->m_blendFactor = this->m_bones->m_blendFactor;
            } else {
                bone->m_blendFactor = 0.0f;
            }
        } else {
            int32_t remaining = bone->m_blendEndTime - this->m_scene->m_time;

            if (remaining <= 0 || (bone->m_sequence.m_currentTime == bone->m_secondarySequence.m_currentTime && bone->m_sequence.m_animIndex == bone->m_secondarySequence.m_animIndex)) {
                bone->m_blendFactor = 0.0f;
            } else {
                float t = remaining * bone->m_invBlendDuration;

                if (t < 0.0f) {
                    bone->m_blendFactor = 0.0f * bone->m_blendWeightMax;
                } else if (t <= 1.0f) {
                    bone->m_blendFactor = t * ((3.0f - (t + t)) * t) * bone->m_blendWeightMax;
                } else {
                    bone->m_blendFactor = 1.0f * bone->m_blendWeightMax;
                }
            }
        }

        uint16_t parentIndex = boneData->m_parentIndex;
        uint32_t boneFlags = boneData->m_flags | bone->m_flags;
        const C44Matrix* parentMatrix;

        if (parentIndex == 0xFFFF) {
            parentMatrix = &this->m_matrixF4;
        } else {
            parentMatrix = &this->m_boneMatrices[parentIndex];

            if (boneFlags & 7) {
                parentTransform = this->m_boneMatrices[parentIndex];
                parentMatrix = &parentTransform;
                C3Vector pivot = boneData->m_pivot * parentTransform;
                C3Vector& rowA = *reinterpret_cast<C3Vector*>(&parentTransform.a0);
                C3Vector& rowB = *reinterpret_cast<C3Vector*>(&parentTransform.b0);
                C3Vector& rowC = *reinterpret_cast<C3Vector*>(&parentTransform.c0);
                const C44Matrix& world = this->m_matrixF4;

                switch (boneFlags & 6) {
                    case 2: {
                        rowA.NormalizeSafe();
                        rowB.NormalizeSafe();
                        rowC.NormalizeSafe();
                        float lengthA = sqrtf(world.a2 * world.a2 + world.a1 * world.a1 + world.a0 * world.a0);
                        parentTransform.a0 = parentTransform.a0 * lengthA;
                        parentTransform.a1 = parentTransform.a1 * lengthA;
                        parentTransform.a2 = lengthA * parentTransform.a2;
                        float lengthB = sqrtf(world.b2 * world.b2 + world.b1 * world.b1 + world.b0 * world.b0);
                        parentTransform.b0 = parentTransform.b0 * lengthB;
                        parentTransform.b1 = parentTransform.b1 * lengthB;
                        parentTransform.b2 = lengthB * parentTransform.b2;
                        float lengthC = sqrtf(world.c2 * world.c2 + world.c1 * world.c1 + world.c0 * world.c0);
                        parentTransform.c0 = parentTransform.c0 * lengthC;
                        parentTransform.c1 = parentTransform.c1 * lengthC;
                        parentTransform.c2 = lengthC * parentTransform.c2;
                        break;
                    }

                    case 4: {
                        float lengthA = world.a2 * world.a2 + world.a1 * world.a1 + world.a0 * world.a0;
                        float scaleA = lengthA <= 0.0000099999997f ? 1.0f : sqrtf((parentTransform.a0 * parentTransform.a0 + parentTransform.a2 * parentTransform.a2 + parentTransform.a1 * parentTransform.a1) / lengthA);
                        parentTransform.a0 = world.a0 * scaleA;
                        parentTransform.a1 = world.a1 * scaleA;
                        parentTransform.a2 = scaleA * world.a2;
                        float lengthB = world.b2 * world.b2 + world.b1 * world.b1 + world.b0 * world.b0;
                        float scaleB = lengthB <= 0.0000099999997f ? 1.0f : sqrtf((parentTransform.b2 * parentTransform.b2 + parentTransform.b1 * parentTransform.b1 + parentTransform.b0 * parentTransform.b0) / lengthB);
                        parentTransform.b0 = world.b0 * scaleB;
                        parentTransform.b1 = world.b1 * scaleB;
                        parentTransform.b2 = scaleB * world.b2;
                        float lengthC = world.c2 * world.c2 + world.c1 * world.c1 + world.c0 * world.c0;
                        float scaleC = lengthC <= 0.0000099999997f ? 1.0f : sqrtf((parentTransform.c2 * parentTransform.c2 + parentTransform.c1 * parentTransform.c1 + parentTransform.c0 * parentTransform.c0) / lengthC);
                        parentTransform.c0 = world.c0 * scaleC;
                        parentTransform.c1 = world.c1 * scaleC;
                        parentTransform.c2 = scaleC * world.c2;
                        break;
                    }

                    case 6: {
                        parentTransform.a0 = world.a0;
                        parentTransform.a1 = world.a1;
                        parentTransform.a2 = world.a2;
                        parentTransform.b0 = world.b0;
                        parentTransform.b1 = world.b1;
                        parentTransform.b2 = world.b2;
                        parentTransform.c0 = world.c0;
                        parentTransform.c1 = world.c1;
                        parentTransform.c2 = world.c2;
                        break;
                    }
                }

                if (boneFlags & 1) {
                    parentTransform.d0 = world.d0;
                    parentTransform.d1 = world.d1;
                    parentTransform.d2 = world.d2;
                } else {
                    const C3Vector& p = boneData->m_pivot;
                    float y = p.y * parentTransform.b1 + p.x * parentTransform.a1 + p.z * parentTransform.c1;
                    float z = p.y * parentTransform.b2 + p.x * parentTransform.a2 + p.z * parentTransform.c2;
                    parentTransform.d0 = pivot.x - (p.y * parentTransform.b0 + p.z * parentTransform.c0 + parentTransform.a0 * p.x);
                    parentTransform.d1 = pivot.y - y;
                    parentTransform.d2 = pivot.z - z;
                }
            }
        }

        int32_t animated = boneFlags & 0x280;
        C44Matrix& boneMatrix = this->m_boneMatrices[i];

        if (animated) {
            if (boneData->m_rotationTrack.m_sequenceTimes.m_count) {
                uint32_t count = boneData->m_rotationTrack.m_sequenceTimes.m_count;

                if (count > 1 || (count == 1 && boneData->m_rotationTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
                    C4Quaternion rotation;
                    rotation.x = 0.0f;
                    rotation.y = 0.0f;
                    rotation.z = 0.0f;
                    rotation.w = 1.0f;
                    M2AnimateTrack<M2CompQuat, C4Quaternion>(this, bone, boneData->m_rotationTrack, bone->m_rotation, rotation);
                }

                local = C44Matrix(bone->m_rotation.m_currentValue);
            } else {
                local.d3 = 1.0f;
                local.c2 = 1.0f;
                local.b1 = 1.0f;
                local.a0 = 1.0f;
                local.d2 = 0.0f;
                local.d1 = 0.0f;
                local.d0 = 0.0f;
                local.c3 = 0.0f;
                local.c1 = 0.0f;
                local.c0 = 0.0f;
                local.b3 = 0.0f;
                local.b2 = 0.0f;
                local.b0 = 0.0f;
                local.a3 = 0.0f;
                local.a2 = 0.0f;
                local.a1 = 0.0f;
            }

            if (boneData->m_scaleTrack.m_sequenceTimes.m_count) {
                uint32_t count = boneData->m_scaleTrack.m_sequenceTimes.m_count;

                if (count > 1 || (count == 1 && boneData->m_scaleTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
                    C3Vector one(1.0f, 1.0f, 1.0f);
                    M2AnimateTrack<C3Vector, C3Vector>(this, bone, boneData->m_scaleTrack, bone->m_scale, one);
                }

                local.Scale(bone->m_scale.m_currentValue);
            }

            if ((boneFlags & 0x80) && bone->m_proceduralTransform) {
                local *= *bone->m_proceduralTransform;
            }

            C3Vector offset = boneData->m_pivot;

            if (boneData->m_translationTrack.m_sequenceTimes.m_count) {
                uint32_t count = boneData->m_translationTrack.m_sequenceTimes.m_count;

                if (count > 1 || (count == 1 && boneData->m_translationTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
                    C3Vector zero(0.0f, 0.0f, 0.0f);
                    M2AnimateTrack<C3Vector, C3Vector>(this, bone, boneData->m_translationTrack, bone->m_translation, zero);
                }

                offset.x = offset.x + bone->m_translation.m_currentValue.x;
                offset.y = bone->m_translation.m_currentValue.y + offset.y;
                offset.z = bone->m_translation.m_currentValue.z + offset.z;
            }

            local.d0 = offset.x + local.d0;
            local.d1 = offset.y + local.d1;
            local.d2 = offset.z + local.d2;
            local.Translate(C3Vector(-boneData->m_pivot.x, -boneData->m_pivot.y, -boneData->m_pivot.z));
            boneMatrix = local * *parentMatrix;
        } else {
            boneMatrix = *parentMatrix;
        }

        uint32_t billboard = boneFlags & 0x78;

        if (billboard) {
            C3Vector& rowA = *reinterpret_cast<C3Vector*>(&boneMatrix.a0);
            C3Vector& rowB = *reinterpret_cast<C3Vector*>(&boneMatrix.b0);
            C3Vector& rowC = *reinterpret_cast<C3Vector*>(&boneMatrix.c0);
            scale.x = sqrtf(boneMatrix.a0 * boneMatrix.a0 + boneMatrix.a1 * boneMatrix.a1 + boneMatrix.a2 * boneMatrix.a2);
            scale.y = sqrtf(boneMatrix.b2 * boneMatrix.b2 + boneMatrix.b1 * boneMatrix.b1 + boneMatrix.b0 * boneMatrix.b0);
            scale.z = sqrtf(boneMatrix.c2 * boneMatrix.c2 + boneMatrix.c1 * boneMatrix.c1 + boneMatrix.c0 * boneMatrix.c0);
            const C3Vector& p = boneData->m_pivot;
            C3Vector pivot = p * boneMatrix;

            switch (billboard) {
                case 8: {
                    if (animated) {
                        rowA.x = local.a1;
                        rowA.y = local.a2;
                        rowA.z = -local.a0;
                        rowA.NormalizeSafe();
                        rowB.x = local.b1;
                        rowB.y = local.b2;
                        rowB.z = -local.b0;
                        rowB.NormalizeSafe();
                        rowC.x = local.c1;
                        rowC.y = local.c2;
                        rowC.z = -local.c0;
                        rowC.NormalizeSafe();
                    } else {
                        rowA.x = 0.0f;
                        rowA.y = 0.0f;
                        rowA.z = -1.0f;
                        rowB.x = 1.0f;
                        rowB.y = 0.0f;
                        rowB.z = 0.0f;
                        rowC.x = 0.0f;
                        rowC.y = 1.0f;
                        rowC.z = 0.0f;
                    }

                    break;
                }

                case 16: {
                    rowA.NormalizeSafe();
                    float x = rowA.x;
                    rowB.x = rowA.y;
                    rowB.y = -x;
                    rowB.z = 0.0f;
                    rowB.NormalizeSafe();
                    float cx = rowA.z * rowB.y - rowB.z * rowA.y;
                    float cy = rowA.x * rowB.z - rowB.x * rowA.z;
                    rowC.x = cx;
                    rowC.y = cy;
                    rowC.z = rowB.x * rowA.y - rowA.x * rowB.y;
                    break;
                }

                case 32: {
                    rowB.NormalizeSafe();
                    float x = rowB.x;
                    rowA.x = -rowB.y;
                    rowA.y = x;
                    rowA.z = 0.0f;
                    rowA.NormalizeSafe();
                    float cx = rowA.z * rowB.y - rowB.z * rowA.y;
                    float cy = rowA.x * rowB.z - rowB.x * rowA.z;
                    rowC.x = cx;
                    rowC.y = cy;
                    rowC.z = rowB.x * rowA.y - rowA.x * rowB.y;
                    break;
                }

                case 64: {
                    rowC.NormalizeSafe();
                    float x = rowC.x;
                    rowB.x = rowC.y;
                    rowB.y = -x;
                    rowB.z = 0.0f;
                    rowB.NormalizeSafe();
                    float ax = rowC.y * rowB.z - rowC.z * rowB.y;
                    float ay = rowB.x * rowC.z - rowB.z * rowC.x;
                    rowA.x = ax;
                    rowA.y = ay;
                    rowA.z = rowB.y * rowC.x - rowB.x * rowC.y;
                    break;
                }
            }

            boneMatrix.Scale(scale);
            float y = boneMatrix.c1 * p.z + boneMatrix.b1 * p.y + boneMatrix.a1 * p.x;
            float z = boneMatrix.c2 * p.z + boneMatrix.b2 * p.y + boneMatrix.a2 * p.x;
            boneMatrix.d0 = pivot.x - (boneMatrix.a0 * p.x + boneMatrix.c0 * p.z + p.y * boneMatrix.b0);
            boneMatrix.d1 = pivot.y - y;
            boneMatrix.d2 = pivot.z - z;
            boneMatrix.a3 = 0.0f;
            boneMatrix.b3 = 0.0f;
            boneMatrix.c3 = 0.0f;
            boneMatrix.d3 = 1.0f;
        }
    }

    for (uint32_t i = 0; i < data->m_colors.m_count; i++) {
        M2Color* colorData = &data->m_colors[i];
        M2ModelColor* color = &this->m_colors[i];
        uint32_t count = colorData->m_color.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && colorData->m_color.m_sequenceTimes[0].m_count > this->m_uint90)) {
            C3Vector zero(0.0f, 0.0f, 0.0f);
            M2AnimateTrack<C3Vector, C3Vector>(this, this->m_bones, colorData->m_color, color->m_colorTrack, zero);
        }

        count = colorData->m_alpha.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && colorData->m_alpha.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float one = 1.0f;
            M2AnimateTrack<fixed16, float>(this, this->m_bones, colorData->m_alpha, color->m_alphaTrack, one);
        }
    }

    for (uint32_t i = 0; i < data->m_textureWeights.m_count; i++) {
        M2TextureWeight* weight = &data->m_textureWeights[i];
        uint32_t count = weight->m_weight.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && weight->m_weight.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float one = 1.0f;
            M2AnimateTrack<fixed16, float>(this, this->m_bones, weight->m_weight, this->m_textureWeights[i], one);
        }
    }

    if (data->m_textureTransforms.m_count) {
        this->AnimateTextureTransformsMT();
    }

    for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
        M2ModelLight* light = &this->m_lights[i];
        M2Light* lightData = &data->m_lights[i];

        if (light->m_enabled) {
            if (lightData->m_visibility.m_sequenceTimes.m_count) {
                uint8_t visible = 1;
                M2AnimateTrack<uint8_t, uint8_t>(this, &this->m_bones[lightData->m_bone], lightData->m_visibility, light->m_visibilityTrack, visible);
            }
        }

        if ((!light->m_enabled || !light->m_visibilityTrack.m_currentValue) && this->m_uint90) {
            continue;
        }

        uint32_t count = lightData->m_ambientIntensity.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && lightData->m_ambientIntensity.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float zero = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[lightData->m_bone], lightData->m_ambientIntensity, light->m_ambientIntensityTrack, zero);
        }

        count = lightData->m_ambientColor.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && lightData->m_ambientColor.m_sequenceTimes[0].m_count > this->m_uint90)) {
            C3Vector zero(0.0f, 0.0f, 0.0f);
            M2AnimateTrack<C3Vector, C3Vector>(this, &this->m_bones[lightData->m_bone], lightData->m_ambientColor, light->m_ambientColorTrack, zero);
            float intensity = light->m_ambientIntensityTrack.m_currentValue * this->m_effectiveAlpha;
            light->m_light.m_ambColor.x = light->m_ambientColorTrack.m_currentValue.x * intensity;
            light->m_light.m_ambColor.y = light->m_ambientColorTrack.m_currentValue.y * intensity;
            light->m_light.m_ambColor.z = intensity * light->m_ambientColorTrack.m_currentValue.z;
        }

        count = lightData->m_diffuseIntensity.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && lightData->m_diffuseIntensity.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float zero = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[lightData->m_bone], lightData->m_diffuseIntensity, light->m_diffuseIntensityTrack, zero);
        }

        count = lightData->m_diffuseColor.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && lightData->m_diffuseColor.m_sequenceTimes[0].m_count > this->m_uint90)) {
            C3Vector zero(0.0f, 0.0f, 0.0f);
            M2AnimateTrack<C3Vector, C3Vector>(this, &this->m_bones[lightData->m_bone], lightData->m_diffuseColor, light->m_diffuseColorTrack, zero);
            float intensity = light->m_diffuseIntensityTrack.m_currentValue * this->m_effectiveAlpha;
            light->m_light.m_dirColor.x = intensity * light->m_diffuseColorTrack.m_currentValue.x;
            light->m_light.m_dirColor.y = light->m_diffuseColorTrack.m_currentValue.y * intensity;
            light->m_light.m_dirColor.z = intensity * light->m_diffuseColorTrack.m_currentValue.z;
        }
    }

    for (uint32_t i = 0; i < data->m_cameras.m_count; i++) {
        M2Camera* cameraData = &data->m_cameras[i];
        M2ModelCamera* camera = &this->m_cameras[i];
        uint32_t count = cameraData->m_positions.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && cameraData->m_positions.m_sequenceTimes[0].m_count > this->m_uint90)) {
            C3Vector zero(0.0f, 0.0f, 0.0f);
            M2AnimateSplineTrack<M2SplineKey<C3Vector>, C3Vector>(this, this->m_bones, cameraData->m_positions, camera->m_positionTrack, zero);
        }

        count = cameraData->m_targetPositions.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && cameraData->m_targetPositions.m_sequenceTimes[0].m_count > this->m_uint90)) {
            C3Vector zero(0.0f, 0.0f, 0.0f);
            M2AnimateSplineTrack<M2SplineKey<C3Vector>, C3Vector>(this, this->m_bones, cameraData->m_targetPositions, camera->m_targetTrack, zero);
        }

        count = cameraData->m_roll.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && cameraData->m_roll.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float zero = 0.0f;
            M2AnimateSplineTrack<M2SplineKey<float>, float>(this, this->m_bones, cameraData->m_roll, camera->m_rollTrack, zero);
        }
    }

    for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
        M2Ribbon* ribbonData = &data->m_ribbons[i];
        M2ModelRibbon* ribbon = &this->m_ribbons[i];
        uint32_t count = ribbonData->m_visibilityTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && ribbonData->m_visibilityTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            uint8_t visible = 1;
            M2AnimateTrack<uint8_t, uint8_t>(this, &this->m_bones[ribbonData->m_boneIndex], ribbonData->m_visibilityTrack, ribbon->m_visibilityTrack, visible);
        }

        count = ribbonData->m_colorTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && ribbonData->m_colorTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            C3Vector zero(0.0f, 0.0f, 0.0f);
            M2AnimateTrack<C3Vector, C3Vector>(this, &this->m_bones[ribbonData->m_boneIndex], ribbonData->m_colorTrack, ribbon->m_colorTrack, zero);
        }

        count = ribbonData->m_alphaTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && ribbonData->m_alphaTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float one = 1.0f;
            M2AnimateTrack<fixed16, float>(this, &this->m_bones[ribbonData->m_boneIndex], ribbonData->m_alphaTrack, ribbon->m_alphaTrack, one);
        }

        count = ribbonData->m_heightAboveTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && ribbonData->m_heightAboveTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float zero = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[ribbonData->m_boneIndex], ribbonData->m_heightAboveTrack, ribbon->m_heightAboveTrack, zero);
        }

        count = ribbonData->m_heightBelowTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && ribbonData->m_heightBelowTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float zero = 0.0f;
            M2AnimateTrack<float, float>(this, &this->m_bones[ribbonData->m_boneIndex], ribbonData->m_heightBelowTrack, ribbon->m_heightBelowTrack, zero);
        }

        count = ribbonData->m_textureSlotTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && ribbonData->m_textureSlotTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            uint16_t zero = 0;
            M2AnimateTrack<uint16_t, uint16_t>(this, &this->m_bones[ribbonData->m_boneIndex], ribbonData->m_textureSlotTrack, ribbon->m_textureSlotTrack, zero);
        }
    }

    this->m_fFlags &= ~0x400u;

    if (data->m_particles.m_count) {
        this->AnimateParticlesMT();
    }

    if (this->m_attachments || this->m_attachList) {
        this->AnimateAttachmentsMT();
    }

    this->m_frameStamp = this->m_scene->m_frameStamp;
}

// VERIFY: a2d095a0 manual
// OFFSET: 0x008309C0
void CM2Model::AnimateParticleST(float deltaTime, uint32_t index) {
    if (!(this->m_fFlags & 1)) {
        return;
    }

    float rate = 0.0f;
    M2Particle* particleData = &this->m_shared->m_data->m_particles[index];
    CParticleEmitter2* emitter = this->m_particleEmitters[index];
    M2ModelParticle* particle = &this->m_particles[index];

    if (!(particleData->m_flags & 0x8000)) {
        if (particle->m_emitting) {
            emitter->m_flags |= 1;
        } else {
            emitter->m_flags &= ~1u;
        }
    } else if (particle->m_visibilityTrack.m_currentValue && particle->m_emissionRateTrack.m_currentValue > 0.0f) {
        if (!particle->m_burstFired) {
            emitter->m_flags |= 0x40;
        }

        particle->m_burstFired = 1;
    } else {
        particle->m_burstFired = 0;
    }

    if (particle->m_emitting) {
        rate = particle->m_emissionRateTrack.m_currentValue;
    }

    emitter->SetEmissionRate(rate);

    if (particle->m_visibilityTrack.m_currentValue || !this->m_uint90) {
        uint32_t count = particleData->m_speedTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_speedTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->m_params.m_speed = particle->m_speedTrack.m_currentValue;
        }

        count = particleData->m_variationTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_variationTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->m_params.m_variation = particle->m_variationTrack.m_currentValue;
        }

        count = particleData->m_latitudeTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_latitudeTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetLatitude(particle->m_latitudeTrack.m_currentValue);
        }

        count = particleData->m_longitudeTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_longitudeTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetLongitude(particle->m_longitudeTrack.m_currentValue);
        }

        count = particleData->m_gravityTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_gravityTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->m_params.m_gravity = particle->m_gravityTrack.m_currentValue;
        }

        count = particleData->m_lifeTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_lifeTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->m_params.m_life = particle->m_lifeTrack.m_currentValue;
        }

        count = particleData->m_widthTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_widthTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetWidth(particle->m_widthTrack.m_currentValue);
        }

        count = particleData->m_lengthTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_lengthTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetHeight(particle->m_lengthTrack.m_currentValue);
        }

        count = particleData->m_zsourceTrack.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && particleData->m_zsourceTrack.m_sequenceTimes[0].m_count > this->m_uint90)) {
            emitter->SetZsource(particle->m_zsourceTrack.m_currentValue);
        }

        float alpha = this->m_effectiveAlpha;

        if (alpha < 0.0f) {
            alpha = 0.0f;
        } else if (alpha >= 1.0f) {
            alpha = 1.0f;
        }

        emitter->m_alphaScale = alpha;
    }

    if (!particle->m_active) {
        return;
    }

    C44Matrix transform = this->m_boneMatrices[particleData->m_boneIndex];
    transform.Translate(particleData->m_position);
    transform *= this->m_scene->m_viewInv;

    static C44Matrix s_emitterRotation(0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    transform = s_emitterRotation * transform;
    emitter->Update(deltaTime, transform, *reinterpret_cast<const C3Vector*>(&this->m_scene->m_viewInv.d0), static_cast<const C44Matrix*>(this->m_frameOfReference));

    uint32_t modelCount = emitter->GetNumParticleModels();

    for (uint32_t i = 0; i < modelCount; i++) {
        uint32_t modelIndex = i;
        CM2Model* model = emitter->GetParticleModelInternal(modelIndex);
        model->AnimateMT(&this->m_scene->m_view, this->m_diffuseColor, this->m_emissiveColor, this->m_effectiveAlpha, this->m_alpha);
        model->AnimateST();
        model->m_currentLighting = this->m_currentLighting;
    }
}

// VERIFY: 68879b30 auto
// OFFSET: 0x004E2790
C3Vector CM2Model::GetWorldPosition() {
    this->Animate();
    return *reinterpret_cast<const C3Vector*>(&this->m_matrixF4.d0) * this->m_scene->m_viewInv;
}

// VERIFY: 4377f416 manual
// OFFSET: 0x00830DC0
void CM2Model::Animate() {
    CM2Scene* scene = this->m_scene;

    if (this->m_frameStamp == scene->m_frameStamp) {
        return;
    }

    if (this->m_attachParent) {
        this->m_attachParent->Animate();
    } else if (this->m_fFlags & 0x1000) {
        C3Vector emissive(0.0f, 0.0f, 0.0f);
        C3Vector diffuse(1.0f, 1.0f, 1.0f);
        this->AnimateMTSimple(&scene->m_view, diffuse, emissive, 1.0f, 1.0f);
    } else {
        C3Vector emissive(0.0f, 0.0f, 0.0f);
        C3Vector diffuse(1.0f, 1.0f, 1.0f);
        this->AnimateMT(&scene->m_view, diffuse, emissive, 1.0f, 1.0f);
    }

    scene = this->m_scene;

    if (this->m_frameStamp == scene->m_frameStamp) {
        return;
    }

    CM2Model* parent = this->m_attachParent;

    if (parent && (this->m_fFlags & 1)) {
        C44Matrix matrix;
        const C44Matrix* transform = &parent->m_matrixF4;

        if ((parent->m_fFlags & 1) && parent->m_frameStamp == scene->m_frameStamp && this->m_attachmentIndex != 0xFFFF) {
            M2Attachment* attachment = &parent->m_shared->m_data->m_attachments[this->m_attachmentIndex];
            matrix = parent->m_boneMatrices[attachment->m_bone];
            C3Vector position = attachment->m_position;
            matrix.Translate(position);
            transform = &matrix;
        }

        parent = this->m_attachParent;

        if (this->m_fFlags & 0x1000) {
            this->AnimateMTSimple(transform, parent->m_diffuseColor, parent->m_emissiveColor, parent->m_effectiveAlpha, parent->m_alpha);
        } else {
            this->AnimateMT(transform, parent->m_diffuseColor, parent->m_emissiveColor, parent->m_effectiveAlpha, parent->m_alpha);
        }

        scene = this->m_scene;

        if (this->m_frameStamp == scene->m_frameStamp) {
            return;
        }
    }

    if (this->m_attachParent) {
        this->m_matrixF4 = this->m_attachParent->m_matrixF4;
    } else {
        this->m_matrixF4 = this->m_worldTransform * scene->m_view;
    }
}

// VERIFY: e229dbcf auto
// OFFSET: 0x00830F90
void CM2Model::ForceAnimate() {
    if (this->m_fFlags & 1) {
        this->m_frameStamp = this->m_scene->m_frameStamp - 1;
        this->Animate();
    }
}

// VERIFY: cb1e5db5 manual
// OFFSET: 0x00830FB0
uint32_t CM2Model::ProcessEventCallbacks(uint32_t boneIndex, uint32_t sequenceIndex, uint32_t timeOffset, uint32_t period, uint32_t startTime, uint32_t endTime, uint32_t callbackTime) {
    if (!this->m_eventCallback) {
        return endTime;
    }

    M2Data* data = this->m_shared->m_data;
    M2ModelBone* bone = &this->m_bones[boneIndex];

    if (data->m_sequences[sequenceIndex].m_flags & 1) {
        period = 0;
    }

    for (uint32_t i = 0; i < data->m_events.m_count; i++) {
        M2Event* event = &data->m_events[i];

        if (!event->m_enabled.m_sequenceTimes.m_count) {
            continue;
        }

        M2Array<uint32_t>* times = &event->m_enabled.m_sequenceTimes[event->m_enabled.m_loopIndex != 0xFFFF ? 0 : sequenceIndex];
        uint32_t count = times->m_count;

        if (!count) {
            continue;
        }

        if (boneIndex) {
            uint16_t parentIndex = data->m_bones[event->m_bone].m_parentIndex;

            if (parentIndex != 0xFFFF && parentIndex != boneIndex) {
                while (parentIndex != 0xFFFF) {
                    parentIndex = data->m_bones[parentIndex].m_parentIndex;

                    if (parentIndex == boneIndex) {
                        break;
                    }
                }

                if (parentIndex == 0xFFFF) {
                    continue;
                }
            }
        }

        uint32_t offset = timeOffset;

        do {
            for (uint32_t k = 0; k < count; k++) {
                int32_t key = (*times)[k] - bone->m_sequence.m_startOffset;
                uint32_t time = offset + bone->m_sequence.m_startTime + static_cast<int32_t>(key * bone->m_sequence.m_invSpeed);

                if (static_cast<int32_t>(time - startTime) > 0 && static_cast<int32_t>(time - endTime) <= 0) {
                    if (time != endTime) {
                        CM2Model::s_sequenceCallbacks.SetCount(0);
                        endTime = time;
                    }

                    this->Animate();

                    C3Vector position = event->m_position * this->m_boneMatrices[event->m_bone] * this->m_scene->m_viewInv;

                    if (CM2Model::s_sequenceCallbacks.Count() < 0x10000) {
                        M2ModelCallback* entry = CM2Model::s_sequenceCallbacks.New();

                        if (entry) {
                            entry->m_boneId = data->m_bones[event->m_bone].m_boneId;
                            entry->m_callback = this->m_eventCallback;
                            entry->m_value = event->m_data;
                            entry->m_id = event->m_eventId;
                            entry->m_type = 1;
                            entry->m_model = this;
                            *reinterpret_cast<C3Vector*>(entry->m_args) = position;
                            entry->m_time = callbackTime - time;
                            entry->m_param = this->m_eventCallbackParam;
                        }
                    }
                }
            }

            offset += period;
        } while (period && static_cast<int32_t>(offset + bone->m_sequence.m_startTime - endTime) <= 0);
    }

    return endTime;
}

// VERIFY: 4c99f6ab manual
// OFFSET: 0x00831330
C3Vector CM2Model::GetAttachmentPosition(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint16_t bone = 0xFFFF;
    uint16_t index = id < data->m_attachmentIndicesById.m_count ? data->m_attachmentIndicesById[id] : 0xFFFF;

    if (index < data->m_attachments.m_count) {
        bone = data->m_attachments[index].m_bone;
    }

    this->Animate();

    return data->m_attachments[index].m_position * this->m_boneMatrices[bone] * this->m_scene->m_viewInv;
}

// VERIFY: 726b46b5 manual
// OFFSET: 0x00831410
C44Matrix CM2Model::GetAttachmentWorldTransform(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint16_t bone = 0xFFFF;
    uint16_t index = id < data->m_attachmentIndicesById.m_count ? data->m_attachmentIndicesById[id] : 0xFFFF;

    if (index < data->m_attachments.m_count) {
        bone = data->m_attachments[index].m_bone;
    }

    this->Animate();

    C44Matrix transform;

    if (index == 0xFFFF) {
        transform = *this->m_boneMatrices;
    } else {
        M2Attachment* attachment = &data->m_attachments[index];
        transform = this->m_boneMatrices[bone];
        transform.Translate(attachment->m_position);
    }

    transform *= this->m_scene->m_viewInv;
    return transform;
}

// VERIFY: fbe80c3c auto
// OFFSET: 0x00831550
float CM2Model::GetAttachmentWorldScale(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    M2Data* data = this->m_shared->m_data;
    uint16_t bone = 0xFFFF;
    uint16_t index = id < data->m_attachmentIndicesById.m_count ? data->m_attachmentIndicesById[id] : 0xFFFF;

    if (index < data->m_attachments.m_count) {
        bone = data->m_attachments[index].m_bone;
    }

    this->Animate();

    C44Matrix transform;

    if (index == 0xFFFF) {
        transform = *this->m_boneMatrices;
    } else {
        transform = this->m_boneMatrices[bone];
    }

    return sqrtf(transform.a2 * transform.a2 + transform.a1 * transform.a1 + transform.a0 * transform.a0);
}

// VERIFY: dcb378e1 manual
// OFFSET: 0x00831630
void CM2Model::AttachToParent(CM2Model* parent, uint32_t id, const C3Vector* position, int32_t attachToRoot) {
    if (this->m_attachParent) {
        this->DetachFromParent();
    }

    if (this->m_animatePrev) {
        *this->m_animatePrev = this->m_animateNext;

        if (this->m_animateNext) {
            this->m_animateNext->m_animatePrev = this->m_animatePrev;
        }

        this->m_animatePrev = nullptr;
        this->m_animateNext = nullptr;
    }

    uint32_t index;

    if (parent->m_fFlags & 1) {
        M2Data* data = parent->m_shared->m_data;
        index = id < data->m_attachmentIndicesById.m_count ? data->m_attachmentIndicesById[id] : 0xFFFF;

        if (index == 0xFFFF && !attachToRoot) {
            return;
        }
    } else {
        index = 0xFFFF;
    }

    this->m_attachmentIndex = index;
    this->m_attachmentId = id;
    this->m_attachParent = parent;
    this->m_fFlags = ((this->m_fFlags & ~0x40000u) | ((attachToRoot != 0) << 18)) | 0x20080;

    this->m_attachPrev = &parent->m_attachList;
    this->m_attachNext = parent->m_attachList;

    if (parent->m_attachList) {
        parent->m_attachList->m_attachPrev = &this->m_attachNext;
    }

    parent->m_attachList = this;

    if (!(this->m_fFlags & 1) || !(this->m_fFlags & 0x100)) {
        for (CM2Model* model = parent; model; model = model->m_attachParent) {
            model->m_fFlags &= ~0x100u;
        }
    }

    if (!(this->m_fFlags & 2)) {
        for (CM2Model* model = parent; model; model = model->m_attachParent) {
            model->m_fFlags &= ~0x200u;
        }
    }

    if (position && (this->m_attachParent->m_fFlags & 1) && this->m_attachmentIndex != 0xFFFF) {
        C44Matrix transform = parent->GetAttachmentWorldTransform(id);
        float scale = sqrtf(transform.a1 * transform.a1 + transform.a2 * transform.a2 + transform.a0 * transform.a0);
        transform = transform.AffineInverse(scale);

        if (!(this->m_fFlags & 0x8000)) {
            this->m_worldTransform.Identity();
        }

        C3Vector local = *position * transform;
        this->m_worldTransform.d0 = local.x;
        this->m_worldTransform.d1 = local.y;
        this->m_fFlags |= 0x8000;
        this->m_worldTransform.d2 = local.z;
    }

    this->m_refCount++;
}

// VERIFY: c5304a4f auto
// OFFSET: 0x008317E0
C3Vector CM2Model::GetEventPosition(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    C3Vector* position;
    uint16_t bone;

    if (!this->FindEvent(id, &position, &bone)) {
        return C3Vector(0.0f, 0.0f, 0.0f);
    }

    this->Animate();
    return *position * this->m_boneMatrices[bone] * this->m_scene->m_viewInv;
}

// VERIFY: 462f2ec0 manual
// OFFSET: 0x008318A0
C44Matrix CM2Model::GetEventWorldTransform(uint32_t id) {
    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    C3Vector* position;
    uint16_t bone;

    if (!this->FindEvent(id, &position, &bone)) {
        return C44Matrix();
    }

    this->Animate();

    C44Matrix transform = this->m_boneMatrices[bone];
    transform.Translate(*position);
    transform *= this->m_scene->m_viewInv;
    return transform;
}

// VERIFY: 44b90448 manual
// OFFSET: 0x00831990
void CM2Model::AnimateSM() {
    CM2Scene* scene = this->m_scene;

    if (this->m_frameStamp == scene->m_frameStamp) {
        return;
    }

    if (this->m_attachParent || this->m_shared->m_billboardBoneCount) {
        this->Animate();
        return;
    }

    *this->m_boneMatrices = this->m_worldTransform * scene->m_view;
    this->m_alpha = this->m_baseAlpha * this->m_fadeAlpha;

    M2Data* data = this->m_shared->m_data;

    for (uint32_t i = 0; i < data->m_colors.m_count; i++) {
        M2Color* colorData = &data->m_colors[i];
        uint32_t count = colorData->m_alpha.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && colorData->m_alpha.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float one = 1.0f;
            M2AnimateTrack<fixed16, float>(this, this->m_bones, colorData->m_alpha, this->m_colors[i].m_alphaTrack, one);
        }
    }

    for (uint32_t i = 0; i < data->m_textureWeights.m_count; i++) {
        M2TextureWeight* weight = &data->m_textureWeights[i];
        uint32_t count = weight->m_weight.m_sequenceTimes.m_count;

        if (count > 1 || (count == 1 && weight->m_weight.m_sequenceTimes[0].m_count > this->m_uint90)) {
            float one = 1.0f;
            M2AnimateTrack<fixed16, float>(this, this->m_bones, weight->m_weight, this->m_textureWeights[i], one);
        }
    }
}

// VERIFY: e596f33c auto
// OFFSET: 0x00831AF0
void CM2Model::SetupLighting() {
    CM2Model* parent = this->m_attachParent;

    if (!parent || (parent->m_flags & 1)) {
        this->Animate();

        C3Vector center = *reinterpret_cast<const C3Vector*>(&this->m_matrixF4.d0) * this->m_scene->m_viewInv;
        CAaSphere sphere;
        sphere.r = 0.0f;
        sphere.c = center;

        this->m_lighting.Initialize(this->m_scene, sphere);
        this->m_scene->SelectLights(&this->m_lighting);

        if (this->m_lightingCallback) {
            this->m_lightingCallback(this, &this->m_lighting, this->m_lightingArg);
        }

        this->m_lighting.SetupSunlight();
        this->m_lighting.CameraSpace();
    }

    for (CM2Model* child = this->m_attachList; child; child = child->m_attachNext) {
        child->SetupLighting();
    }
}

// VERIFY: 133dedff manual
// OFFSET: 0x00831C30
void CM2Model::SetBoneSequenceDeferred(uint16_t sequenceIndex, M2Data* data, uint16_t boneIndex, uint32_t time, float speed, M2SequenceFallback fallback, int32_t blend, int32_t primary, int32_t pickRandom) {
    CM2SequencePlayBack* playback;

    if (data->m_sequences[sequenceIndex].m_flags & 0x10) {
        uint16_t index = sequenceIndex;

        while (true) {
            TSList<CM2SequenceLoad>& loads = this->m_shared->m_sequenceLoads;

            for (CM2SequenceLoad* load = loads.Head(); !loads.IsEnd(load); load = loads.RawNext(load)) {
                if (load->m_sequenceIndex != index) {
                    continue;
                }

                for (playback = load->m_playbacks.Head(); !load->m_playbacks.IsEnd(playback); playback = load->m_playbacks.RawNext(playback)) {
                    if (playback->m_model == this) {
                        goto found;
                    }
                }

                playback = load->m_playbacks.NewNode(2, 0, 0);
                playback->m_model = this;
                goto found;
            }

            index = data->m_sequences[index].m_aliasNext;

            if (index == sequenceIndex) {
                return;
            }
        }
    } else {
        CM2SequenceLoad* load = this->m_shared->LoadLowPrioritySequence(sequenceIndex);

        if (!load) {
            return;
        }

        playback = load->m_playbacks.NewNode(2, 0, 0);
        playback->m_model = this;
    }

found:
    playback->m_speed = speed;
    playback->m_boneIndex = boneIndex;
    playback->m_time = time;
    playback->m_fallback = fallback;
    playback->m_flags = 0;

    if (blend) {
        playback->m_flags = 1;
    }

    if (primary) {
        playback->m_flags |= 2;
    }

    if (pickRandom) {
        playback->m_flags |= 4;
    }
}

// VERIFY: 7cd1ff77 manual
// OFFSET: 0x00831E20
void CM2Model::CancelAllDeferredSequences() {
    TSList<CM2SequenceLoad>& loads = this->m_shared->m_sequenceLoads;

    if (!(this->m_shared->m_flags & 0x10)) {
        for (CM2SequenceLoad* load = loads.Head(); !loads.IsEnd(load); load = loads.RawNext(load)) {
            CM2SequencePlayBack* playback = load->m_playbacks.Head();

            while (!load->m_playbacks.IsEnd(playback)) {
                if (playback->m_model == this) {
                    playback = load->m_playbacks.DeleteNode(playback);
                } else {
                    playback = load->m_playbacks.RawNext(playback);
                }
            }
        }
    } else {
        for (CM2SequenceLoad* load = loads.Head(); !loads.IsEnd(load); load = loads.RawNext(load)) {
            for (CM2SequencePlayBack* playback = load->m_playbacks.Head(); !load->m_playbacks.IsEnd(playback); playback = load->m_playbacks.RawNext(playback)) {
                if (playback->m_model == this) {
                    playback->m_flags |= 8;
                }
            }
        }
    }
}

// VERIFY: df5ba622 manual
// OFFSET: 0x00831EC0
void CM2Model::CancelDeferredSequences(uint16_t boneIndex, bool primary) {
    TSList<CM2SequenceLoad>& loads = this->m_shared->m_sequenceLoads;

    if (!(this->m_shared->m_flags & 0x10)) {
        for (CM2SequenceLoad* load = loads.Head(); !loads.IsEnd(load); load = loads.RawNext(load)) {
            CM2SequencePlayBack* playback = load->m_playbacks.Head();

            while (!load->m_playbacks.IsEnd(playback)) {
                if (playback->m_model == this && playback->m_boneIndex == boneIndex && ((playback->m_flags & 2) != 0) == primary) {
                    playback = load->m_playbacks.DeleteNode(playback);
                } else {
                    playback = load->m_playbacks.RawNext(playback);
                }
            }
        }
    } else {
        for (CM2SequenceLoad* load = loads.Head(); !loads.IsEnd(load); load = loads.RawNext(load)) {
            for (CM2SequencePlayBack* playback = load->m_playbacks.Head(); !load->m_playbacks.IsEnd(playback); playback = load->m_playbacks.RawNext(playback)) {
                if (playback->m_model == this && playback->m_boneIndex == boneIndex && ((playback->m_flags & 2) != 0) == primary) {
                    playback->m_flags |= 8;
                }
            }
        }
    }
}

// VERIFY: a99fdf56 manual
// OFFSET: 0x00831FC0
void CM2Model::CallSequenceCallback(M2ModelCallback* entry) {
    M2Data* data = this->m_shared->m_data;
    uint32_t boneId = entry->m_boneId;
    uint16_t boneIndex;

    if (boneId == 0xFFFFFFFF) {
        boneIndex = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        boneIndex = data->m_boneIndicesById[boneId];
    } else {
        boneIndex = 0xFFFF;
    }

    M2ModelBone* bone = &this->m_bones[boneIndex];
    uint16_t sequenceIndex = bone->m_sequence.m_sequenceIndex;
    uint32_t startTime = bone->m_sequence.m_startTime;

    if (entry->m_args[1] != startTime || entry->m_args[0] != sequenceIndex || sequenceIndex == 0xFFFF) {
        return;
    }

    M2Sequence* sequence = &data->m_sequences[sequenceIndex];

    if (sequence->m_flags & 1) {
        bone->m_sequence.m_finished = 1;
    }

    M2SequenceCallback callback = reinterpret_cast<M2SequenceCallback>(entry->m_callback);

    if (callback) {
        callback(entry->m_model, entry->m_boneId, entry->m_id, entry->m_value, entry->m_time, entry->m_param);

        if (startTime != bone->m_sequence.m_startTime || sequenceIndex != bone->m_sequence.m_sequenceIndex) {
            return;
        }
    }

    if ((sequence->m_flags & 1) || !bone->m_sequence.m_pickRandomVariation || (!sequence->m_variationIndex && sequence->m_variationNext == 0xFFFF)) {
        return;
    }

    float speed = bone->m_sequence.m_speed;
    M2SequenceFallback fallback;
    this->SequenceFallbackById(&fallback, entry->m_id);
    uint16_t index = this->FindSequenceIndex(data, fallback.m_id, 0);
    uint32_t variation;
    this->PickRandomSequenceVariation(&variation, &index);

    uint32_t time;

    if (fequal(speed, 1.0f)) {
        time = entry->m_time;
    } else {
        time = static_cast<uint32_t>(entry->m_time * bone->m_sequence.m_invSpeed);
    }

    if (data->m_sequences[index].m_flags & 0x20) {
        float currentSpeed = bone->m_sequence.m_speed;
        bone->m_sequenceId = entry->m_id;
        bone->m_variationIndex = variation;
        this->SetPrimaryBoneSequence(index, boneIndex, fallback, time, currentSpeed, 1);
    } else if (!this->HasDeferredSequence(boneIndex, true)) {
        this->SetBoneSequenceDeferred(index, data, boneIndex, time, bone->m_sequence.m_speed, fallback, 1, 1, 1);
    }
}

// VERIFY: e7e49f5d manual
// OFFSET: 0x008321E0
void CM2Model::CallCallbacks() {
    uint32_t count = CM2Model::s_sequenceCallbacks.Count();

    for (uint32_t i = 0; i < count; i++) {
        M2ModelCallback* entry = &CM2Model::s_sequenceCallbacks.m_data[i];

        if (entry->m_type == 0) {
            this->CallSequenceCallback(entry);
        } else if (entry->m_type == 1) {
            M2EventCallback callback = reinterpret_cast<M2EventCallback>(entry->m_callback);
            callback(entry->m_model, entry->m_boneId, entry->m_id, entry->m_value, reinterpret_cast<const C3Vector*>(entry->m_args), entry->m_time, entry->m_param);
        }
    }

    CM2Model::s_sequenceCallbacks.SetCount(0);
}

// VERIFY: 7263f300 manual
// OFFSET: 0x00832260
int32_t CM2Model::ProcessCallbacks() {
    if (this->m_lastAnimTime || !(this->m_fFlags & 0x400000)) {
        return 1;
    }

    CM2Scene* scene = this->m_scene;
    uint32_t now = scene->m_time;
    uint32_t start = now - scene->m_timeDelta;
    M2Data* data = this->m_shared->m_data;

    if (!scene->m_timeDelta) {
        return 1;
    }

    uint32_t current;

    do {
        uint16_t boneIndex = this->m_animListHead;
        current = now;

        while (boneIndex != 0xFFFF) {
            M2ModelBone* bone = &this->m_bones[boneIndex];

            if (!bone->m_sequence.m_finished) {
                float speed = fabsf(bone->m_sequence.m_invSpeed);
                uint16_t sequenceIndex = bone->m_sequence.m_sequenceIndex;
                M2Sequence* sequence = &data->m_sequences[sequenceIndex];
                uint32_t duration;

                if (fabsf(speed - 1.0f) >= 0.00000023841858f) {
                    duration = static_cast<uint32_t>(speed * sequence->m_duration);
                } else {
                    duration = sequence->m_duration;
                }

                if (duration) {
                    uint32_t loopStart;
                    uint32_t loopEnd;
                    int32_t finished;

                    if (sequence->m_flags & 1) {
                        loopEnd = bone->m_sequence.m_endTime;
                        loopStart = 0;
                        finished = static_cast<int32_t>(loopEnd - current) <= 0;
                    } else {
                        uint32_t from = bone->m_sequence.m_startTime;

                        if (static_cast<int32_t>(start - from) >= 0) {
                            from = start;
                        }

                        uint32_t loops = (from - bone->m_sequence.m_startTime) / duration;
                        loopStart = duration * loops;
                        loopEnd = duration * (loops + 1) + bone->m_sequence.m_startTime - 1;
                        finished = static_cast<int32_t>(loopEnd - start) > 0 && static_cast<int32_t>(loopEnd - current) <= 0;
                    }

                    current = this->ProcessEventCallbacks(boneIndex, sequenceIndex, loopStart, duration, start, current, now);

                    if (finished) {
                        current = this->ProcessSequenceCallback(boneIndex, now, now - loopEnd, current);
                    }
                }
            }

            boneIndex = this->m_bones[boneIndex].m_animListNext;
        }

        if (!CM2Model::s_sequenceCallbacks.Count()) {
            break;
        }

        this->CallCallbacks();

        if (this->m_refCount == 1) {
            return 0;
        }

        start = current;
    } while (static_cast<int32_t>(current - now) < 0);

    return 1;
}

// VERIFY: 8ce97631 manual
// OFFSET: 0x00832450
void CM2Model::ProcessCallbacksRecursive() {
    if (!(this->m_fFlags & 1)) {
        return;
    }

    this->m_refCount++;

    if (this->ProcessCallbacks()) {
        for (CM2Model* child = this->m_attachList; child; child = child->m_attachNext) {
            bool hidden;

            if (child->m_attachmentIndex == 0xFFFF) {
                hidden = !(child->m_fFlags & 0x40000);
            } else {
                hidden = this->m_attachments[child->m_attachmentIndex].m_currentValue == 0;
            }

            if (!hidden) {
                child->ProcessCallbacksRecursive();
            }
        }
    }

    if (this->m_refCount-- == 1) {
        g_modelPool->Delete(this);
    }
}

// VERIFY: d96ccadd manual
// OFFSET: 0x00832640
CM2Model::~CM2Model() {
    CM2Model* model30 = this->m_model30;

    if (model30 && model30->m_refCount-- == 1) {
        g_modelPool->Delete(model30);
    }

    this->CancelAllDeferredSequences();

    if (this->m_callbackPrev) {
        *this->m_callbackPrev = this->m_callbackNext;

        if (this->m_callbackNext) {
            this->m_callbackNext->m_callbackPrev = this->m_callbackPrev;
        } else {
            this->m_shared->m_callbackListTail = this->m_callbackPrev;
        }

        this->m_callbackPrev = nullptr;
        this->m_callbackNext = nullptr;
    }

    if (this->m_animatePrev) {
        *this->m_animatePrev = this->m_animateNext;
    }

    if (this->m_animateNext) {
        this->m_animateNext->m_animatePrev = this->m_animatePrev;
    }

    if (this->m_drawPrev) {
        *this->m_drawPrev = this->m_drawNext;
    }

    if (this->m_drawNext) {
        this->m_drawNext->m_drawPrev = this->m_drawPrev;
    }

    if (this->m_particlePrev) {
        *this->m_particlePrev = this->m_particleNext;
    }

    if (this->m_particleNext) {
        this->m_particleNext->m_particlePrev = this->m_particlePrev;
    }

    if (this->m_hitTestPrev) {
        *this->m_hitTestPrev = this->m_hitTestNext;
    }

    if (this->m_hitTestNext) {
        this->m_hitTestNext->m_hitTestPrev = this->m_hitTestPrev;
    }

    this->DetachFromScene();

    if (this->m_shared) {
        this->FreeExternalResources();
        this->FreeInternalResources();
        this->m_shared->Release();
        this->m_shared = nullptr;
    }

    while (this->m_attachList) {
        CM2Model* child = this->m_attachList;

        if (child->m_attachPrev) {
            *child->m_attachPrev = child->m_attachNext;
        }

        if (child->m_attachNext) {
            child->m_attachNext->m_attachPrev = child->m_attachPrev;
        }

        child->m_fFlags &= ~0x40000u;
        bool release = child->m_refCount-- == 1;
        child->m_attachPrev = nullptr;
        child->m_attachNext = nullptr;
        child->m_attachParent = nullptr;
        child->m_attachmentId = 0xFFFFFFFF;
        child->m_frameOfReference = nullptr;

        if (release) {
            g_modelPool->Delete(child);
        }
    }

    if (this->m_attachPrev) {
        *this->m_attachPrev = this->m_attachNext;
    }

    if (this->m_attachNext) {
        this->m_attachNext->m_attachPrev = this->m_attachPrev;
    }

    while (this->m_modelCallList) {
        CM2ModelCall* call = this->m_modelCallList;
        this->m_modelCallList = call->m_next;

        if (!call->m_type && call->m_args[1]) {
            HandleClose(reinterpret_cast<HOBJECT>(call->m_args[1]));
        }

        delete call;
    }

    this->UnoptimizeVisibleGeometry();
    SIMD::FreeA16(this->m_boneMatrices);
    SIMD::FreeA16(this->m_textureMatrices);
    this->m_attachParent = nullptr;
    this->m_currentLighting = nullptr;
}

// VERIFY: 3202a5f5 manual
// OFFSET: 0x00832840
void CM2Model::UnsetBoneSequence(uint32_t boneId, int32_t blend, int32_t primary) {
    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_UNSET_BONE_SEQUENCE;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            call->m_args[0] = boneId;
            call->m_args[1] = blend;
            call->m_args[2] = primary;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    if (this->m_fFlags & 0x800) {
        blend = 0;
    }

    M2Data* data = this->m_shared->m_data;
    uint16_t boneIndex;

    if (boneId == 0xFFFFFFFF) {
        boneIndex = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        boneIndex = data->m_boneIndicesById[boneId];
    } else {
        boneIndex = 0xFFFF;
    }

    if (boneIndex >= data->m_bones.m_count || !boneIndex || data->m_bones[boneIndex].m_parentIndex == 0xFFFF || !this->OnSequenceInterrupted(boneId, boneIndex)) {
        return;
    }

    this->CancelDeferredSequences(boneIndex, primary != 0);
    M2ModelBone* bone = &this->m_bones[boneIndex];

    if (primary) {
        if (bone->m_animListPrev) {
            *bone->m_animListPrev = bone->m_animListNext;
        }

        if (bone->m_animListNext != 0xFFFF) {
            this->m_bones[bone->m_animListNext].m_animListPrev = bone->m_animListPrev;
        }

        bone->m_animListPrev = nullptr;
        bone->m_animListNext = 0xFFFF;

        if (blend) {
            float weight;

            if (bone->m_secondarySequence.m_sequenceIndex != 0xFFFF) {
                float t = (bone->m_blendEndTime - this->m_scene->m_time) * bone->m_invBlendDuration;

                if (t < 0.0f) {
                    weight = 0.0f;
                } else if (t <= 1.0f) {
                    weight = t * ((3.0f - (t + t)) * t);
                } else {
                    weight = 1.0f;
                }
            }

            if (bone->m_secondarySequence.m_sequenceIndex == 0xFFFF || weight * bone->m_blendWeightMax <= 0.5f) {
                bone->m_secondarySequence = bone->m_sequence;
                bone->m_invBlendDuration = 0.0066666668f;
                bone->m_blendEndTime = this->m_scene->m_time + 150;
                bone->m_blendWeightMax = 1.0f;
            }
        } else {
            bone->m_secondarySequence.m_sequenceIndex = 0xFFFF;
        }

        bone->m_sequence.m_speed = 0.0f;
        bone->m_sequence.m_sequenceIndex = 0xFFFF;
        bone->m_sequence.m_invSpeed = 0.0f;
        bone->m_sequenceId = 0xFFFFFFFF;
        bone->m_variationIndex = 0;
        bone->m_sequence.m_startTime = 0;
        bone->m_sequence.m_endTime = 0;
        bone->m_sequence.m_startOffset = 0;
    } else {
        bone->m_secondarySequence.m_sequenceIndex = 0xFFFF;
        bone->m_secondarySequence.m_speed = 0.0f;
        bone->m_secondarySequence.m_startTime = 0;
        bone->m_secondarySequence.m_invSpeed = 0.0f;
        bone->m_secondarySequence.m_endTime = 0;
        bone->m_secondarySequence.m_startOffset = 0;
    }
}

// VERIFY: 6d0c8da6 manual
// OFFSET: 0x00832AB0
void CM2Model::SetBoneSequence(uint32_t boneId, uint32_t sequenceId, uint32_t variation, uint32_t time, float speed, int32_t blend, int32_t primary) {
    if (sequenceId == 0xFFFFFFFF) {
        this->UnsetBoneSequence(boneId, blend, primary);
        return;
    }

    if (!(this->m_fFlags & 1)) {
        CM2ModelCall* call = new (__FILE__, __LINE__) CM2ModelCall();

        if (call) {
            call->m_type = MODEL_SET_BONE_SEQUENCE;
            call->m_next = nullptr;
            call->m_time = this->m_scene->m_time;
            *reinterpret_cast<float*>(&call->m_args[4]) = speed;
            call->m_args[0] = boneId;
            call->m_args[2] = variation;
            call->m_args[3] = time;
            call->m_args[5] = blend;
            call->m_args[1] = sequenceId;
            call->m_args[6] = primary;
            *this->m_modelCallTail = call;
            this->m_modelCallTail = &call->m_next;
        }

        return;
    }

    if (this->m_fFlags & 0x800) {
        blend = 0;
    }

    M2Data* data = this->m_shared->m_data;
    uint16_t boneIndex;

    if (boneId == 0xFFFFFFFF) {
        boneIndex = 0;
    } else if (boneId < data->m_boneIndicesById.m_count) {
        boneIndex = data->m_boneIndicesById[boneId];
    } else {
        boneIndex = 0xFFFF;
    }

    if (boneIndex >= data->m_bones.m_count) {
        return;
    }

    M2SequenceFallback fallback;
    this->SequenceFallbackById(&fallback, sequenceId);
    int32_t pickRandom = variation == 0xFFFFFFFF;
    uint16_t index = this->FindSequenceIndex(data, fallback.m_id, variation != 0xFFFFFFFF ? variation : 0);

    if (index == 0xFFFF) {
        pickRandom = 1;
        uint32_t hashCount = data->m_sequenceIdxHashById.m_count;

        if (hashCount) {
            uint32_t slot = fallback.m_id % hashCount;
            index = data->m_sequenceIdxHashById[slot];

            if (index != 0xFFFF) {
                for (uint32_t step = 1; data->m_sequences[index].m_id != fallback.m_id; step++) {
                    slot = (slot + step * step) % hashCount;
                    index = data->m_sequenceIdxHashById[slot];

                    if (index == 0xFFFF) {
                        break;
                    }
                }
            }
        } else {
            for (index = 0; index < data->m_sequences.m_count; index++) {
                if (data->m_sequences[index].m_id == fallback.m_id) {
                    break;
                }
            }

            if (index >= data->m_sequences.m_count) {
                index = 0xFFFF;
            }
        }

        this->PickRandomSequenceVariation(&variation, &index);
    } else if (pickRandom) {
        this->PickRandomSequenceVariation(&variation, &index);
    }

    if (!(data->m_sequences[index].m_flags & 0x20)) {
        this->SetBoneSequenceDeferred(index, data, boneIndex, time, speed, fallback, blend, primary, pickRandom);
        return;
    }

    if (!this->OnSequenceInterrupted(boneId, boneIndex)) {
        return;
    }

    this->CancelDeferredSequences(boneIndex, primary != 0);
    M2ModelBone* bone = &this->m_bones[boneIndex];

    if (primary) {
        bone->m_sequenceId = sequenceId;
        bone->m_variationIndex = variation;
        this->SetPrimaryBoneSequence(index, boneIndex, fallback, time, speed, blend);
        bone->m_sequence.m_pickRandomVariation = pickRandom;
    } else {
        this->SetSecondaryBoneSequence(index, boneIndex, fallback, time, speed);
        bone->m_secondarySequence.m_pickRandomVariation = pickRandom;
    }
}

// OFFSET: 0x00824B70
bool SModelBatchLessThan::operator()(const CModelBatch& a, const CModelBatch& b) const {
    CM2Model* modelA = a.m_model;
    M2ModelOptGeo* optGeoA = modelA->m_optGeo;
    uint32_t batchA = a.m_batchIndex;
    M2SkinSection* sectionA;

    if (optGeoA) {
        sectionA = &optGeoA->m_skinSections[optGeoA->m_batches[batchA].m_skinSectionIndex];
    } else {
        sectionA = &modelA->m_shared->m_skinSections[modelA->m_shared->m_skinData->m_batches[batchA].m_skinSectionIndex];
    }

    CM2Model* modelB = b.m_model;
    uint32_t batchB = b.m_batchIndex;
    M2SkinSection* sectionB;

    if (modelB->m_optGeo) {
        sectionB = &modelB->m_optGeo->m_skinSections[modelB->m_optGeo->m_batches[batchB].m_skinSectionIndex];
    } else {
        sectionB = &modelB->m_shared->m_skinSections[modelB->m_shared->m_skinData->m_batches[batchB].m_skinSectionIndex];
    }

    uint16_t influencesA = sectionA->m_boneInfluences;
    uint16_t influencesB = sectionB->m_boneInfluences;

    if (influencesA < influencesB) {
        return true;
    }

    if (influencesA > influencesB) {
        return false;
    }

    uintptr_t geoA = reinterpret_cast<uintptr_t>(modelA->m_optGeo);
    uintptr_t geoB = reinterpret_cast<uintptr_t>(modelB->m_optGeo);

    if (geoA < geoB) {
        return true;
    }

    if (geoA > geoB) {
        return false;
    }

    uintptr_t sharedA = reinterpret_cast<uintptr_t>(modelA->m_shared);
    uintptr_t sharedB = reinterpret_cast<uintptr_t>(modelB->m_shared);

    if (sharedA < sharedB) {
        return true;
    }

    if (sharedA > sharedB) {
        return false;
    }

    return batchA < batchB;
}

// VERIFY: dda4a700 manual
// OFFSET: 0x00824C60
static bool CanCombineBatches(const CModelBatch& a, const CModelBatch& b) {
    CM2Model* modelA = a.m_model;
    CM2Model* modelB = b.m_model;

    if (modelA->m_shared != modelB->m_shared || a.m_batchIndex != b.m_batchIndex || modelA->m_optGeo != modelB->m_optGeo) {
        return false;
    }

    M2SkinSection* sectionA;
    M2SkinSection* sectionB;

    if (modelA->m_optGeo) {
        sectionA = &modelA->m_optGeo->m_skinSections[modelA->m_optGeo->m_batches[a.m_batchIndex].m_skinSectionIndex];
        sectionB = &modelB->m_optGeo->m_skinSections[modelB->m_optGeo->m_batches[b.m_batchIndex].m_skinSectionIndex];
    } else {
        sectionA = &modelA->m_shared->m_skinSections[modelA->m_shared->m_skinData->m_batches[a.m_batchIndex].m_skinSectionIndex];
        sectionB = &modelB->m_shared->m_skinSections[modelB->m_shared->m_skinData->m_batches[b.m_batchIndex].m_skinSectionIndex];
    }

    return sectionA->m_boneInfluences == sectionB->m_boneInfluences;
}

// VERIFY: 01820d35 manual
// OFFSET: 0x00832DD0
void CModelBatchList::Optimize() {
    std::sort(this->m_batches, this->m_batches + this->m_count, SModelBatchLessThan());

    for (uint32_t i = 0; i < this->m_count; i += this->m_batches[i].m_count) {
        CModelBatch* batch = &this->m_batches[i];
        uint32_t j = i + 1;

        if (!batch->m_model->m_optGeo) {
            for (; j < this->m_count; j++) {
                if (!CanCombineBatches(*batch, this->m_batches[j])) {
                    break;
                }
            }
        }

        batch->m_count = j - i;
    }
}

// VERIFY: 0f5f3316 manual
// OFFSET: 0x00832EA0
int32_t CM2Model::InitializeLoaded() {
    CM2Shared* shared = this->m_shared;

    if (!(shared->m_flags & 1) || !(shared->m_flags & 2)) {
        return 1;
    }

    M2Data* data = shared->m_data;
    M2SkinProfile* skin = shared->m_skinData;
    uint32_t emitterBytes = 0;

    for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
        switch (data->m_particles[i].m_emitterType) {
            case 1:
                emitterBytes += sizeof(CPlaneParticleEmitter);
                break;

            case 2:
                emitterBytes += sizeof(CSphereParticleEmitter);
                break;

            case 3:
                emitterBytes += sizeof(CSplineParticleEmitter);
                break;
        }
    }

    uint32_t ribbonCount = data->m_ribbons.m_count;
    uint32_t ribbonOffset = skin->m_skinSections.m_count * sizeof(uint32_t)
        + data->m_textures.m_count * sizeof(CTexture*)
        + data->m_loops.m_count * sizeof(int32_t)
        + data->m_bones.m_count * sizeof(M2ModelBone)
        + data->m_particles.m_count * (sizeof(M2ModelParticle) + sizeof(CParticleEmitter2*))
        + data->m_lights.m_count * sizeof(M2ModelLight)
        + data->m_textureWeights.m_count * sizeof(M2ModelTrack<float>)
        + data->m_attachments.m_count * sizeof(M2ModelTrack<uint8_t>)
        + ribbonCount * (sizeof(M2ModelRibbon) + sizeof(CRibbonEmitter*))
        + data->m_colors.m_count * sizeof(M2ModelColor)
        + data->m_textureTransforms.m_count * sizeof(M2ModelTextureTransform)
        + data->m_cameras.m_count * sizeof(M2ModelCamera);
    uint32_t emitterOffset = ribbonOffset + ribbonCount * sizeof(CRibbonEmitter);

    uint8_t* memory = static_cast<uint8_t*>(SMemAlloc(emitterBytes + emitterOffset, __FILE__, __LINE__, 0));

    if (!memory) {
        return 0;
    }

    uint8_t* cursor = memory;

    if (data->m_bones.m_count) {
        this->m_bones = reinterpret_cast<M2ModelBone*>(cursor);
        cursor += data->m_bones.m_count * sizeof(M2ModelBone);

        for (uint32_t i = 0; i < data->m_bones.m_count; i++) {
            new (&this->m_bones[i]) M2ModelBone();
        }

        for (uint32_t i = 0; i < data->m_bones.m_count; i++) {
            this->m_bones[i].m_flags = data->m_bones[i].m_flags;
        }

        this->m_boneMatrices = static_cast<C44Matrix*>(SIMD::AllocA16(data->m_bones.m_count * sizeof(C44Matrix), __FILE__, __LINE__));

        if (!this->m_boneMatrices) {
            return 0;
        }

        for (uint32_t i = 0; i < data->m_bones.m_count; i++) {
            this->m_boneMatrices[i].Identity();
        }
    }

    if (data->m_loops.m_count) {
        this->m_loops = reinterpret_cast<int32_t*>(cursor);
        cursor += data->m_loops.m_count * sizeof(int32_t);

        for (uint32_t i = 0; i < data->m_loops.m_count; i++) {
            new (&this->m_loops[i]) int32_t(0);
        }
    }

    if (skin->m_skinSections.m_count) {
        CM2Model* source = this->m_model30;
        this->m_skinSections = reinterpret_cast<uint32_t*>(cursor);
        cursor += skin->m_skinSections.m_count * sizeof(uint32_t);

        if (source) {
            memcpy(this->m_skinSections, source->m_skinSections, skin->m_skinSections.m_count * sizeof(uint32_t));
        } else {
            for (uint32_t i = 0; i < skin->m_skinSections.m_count; i++) {
                new (&this->m_skinSections[i]) uint32_t(1);
            }
        }
    }

    if (data->m_colors.m_count) {
        this->m_colors = reinterpret_cast<M2ModelColor*>(cursor);
        cursor += data->m_colors.m_count * sizeof(M2ModelColor);

        for (uint32_t i = 0; i < data->m_colors.m_count; i++) {
            new (&this->m_colors[i]) M2ModelColor();
        }
    }

    if (data->m_textures.m_count) {
        this->m_textures = reinterpret_cast<CTexture**>(cursor);
        cursor += data->m_textures.m_count * sizeof(CTexture*);

        for (uint32_t i = 0; i < data->m_textures.m_count; i++) {
            CTexture* texture = this->m_model30 ? this->m_model30->m_textures[i] : this->m_shared->m_textures[i];
            this->m_textures[i] = texture ? reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture))) : nullptr;
        }
    }

    if (data->m_textureWeights.m_count) {
        this->m_textureWeights = reinterpret_cast<M2ModelTrack<float>*>(cursor);
        cursor += data->m_textureWeights.m_count * sizeof(M2ModelTrack<float>);

        for (uint32_t i = 0; i < data->m_textureWeights.m_count; i++) {
            new (&this->m_textureWeights[i]) M2ModelTrack<float>();
        }
    }

    if (data->m_textureTransforms.m_count) {
        this->m_textureTransforms = reinterpret_cast<M2ModelTextureTransform*>(cursor);
        cursor += data->m_textureTransforms.m_count * sizeof(M2ModelTextureTransform);

        for (uint32_t i = 0; i < data->m_textureTransforms.m_count; i++) {
            new (&this->m_textureTransforms[i]) M2ModelTextureTransform();
        }

        this->m_textureMatrices = static_cast<C44Matrix*>(SIMD::AllocA16(data->m_textureTransforms.m_count * sizeof(C44Matrix), __FILE__, __LINE__));

        if (!this->m_textureMatrices) {
            return 0;
        }
    }

    if (data->m_attachments.m_count) {
        this->m_attachments = reinterpret_cast<M2ModelTrack<uint8_t>*>(cursor);
        cursor += data->m_attachments.m_count * sizeof(M2ModelTrack<uint8_t>);

        for (uint32_t i = 0; i < data->m_attachments.m_count; i++) {
            new (&this->m_attachments[i]) M2ModelTrack<uint8_t>();
        }

        for (uint32_t i = 0; i < data->m_attachments.m_count; i++) {
            this->m_attachments[i].m_currentValue = 1;
        }
    }

    if (data->m_lights.m_count) {
        this->m_lights = reinterpret_cast<M2ModelLight*>(cursor);
        cursor += data->m_lights.m_count * sizeof(M2ModelLight);

        for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
            M2Light* lightData = &data->m_lights[i];
            M2ModelLight* light = new (&this->m_lights[i]) M2ModelLight();
            light->m_light.Initialize(this->m_scene);
            light->m_light.SetLightType(static_cast<M2LIGHTTYPE>(lightData->m_type));
            light->m_ambientIntensityTrack.m_currentValue = 1.0f;
            light->m_diffuseIntensityTrack.m_currentValue = 1.0f;
            light->m_visibilityTrack.m_currentValue = 1;
        }
    }

    if (data->m_cameras.m_count) {
        this->m_cameras = reinterpret_cast<M2ModelCamera*>(cursor);
        cursor += data->m_cameras.m_count * sizeof(M2ModelCamera);

        for (uint32_t i = 0; i < data->m_cameras.m_count; i++) {
            new (&this->m_cameras[i]) M2ModelCamera();
        }

        for (uint32_t i = 0; i < data->m_cameras.m_count; i++) {
            M2Camera* cameraData = &data->m_cameras[i];
            CCamera* camera = CameraCreate();

            if (!(cameraData->m_fieldOfView > 0.0f) || cameraData->m_fieldOfView >= 3.1415927f) {
                DebugLog("\"%s\", %s = %g", "shared->fieldOfView > 0.0f && shared->fieldOfView < PI", "shared->fieldOfView", static_cast<double>(cameraData->m_fieldOfView));
                break;
            }

            if (cameraData->m_farClip <= cameraData->m_nearClip) {
                DebugLog("\"%s\", %s = %g, %s = %g", "shared->nearClip < shared->farClip", "shared->nearClip", static_cast<double>(cameraData->m_nearClip), "shared->farClip", static_cast<double>(cameraData->m_farClip));
                break;
            }

            CameraSetFloat(camera, 4, cameraData->m_fieldOfView);
            CameraSetFloat(camera, 3, cameraData->m_nearClip);
            CameraSetFloat(camera, 2, cameraData->m_farClip);
            this->m_cameras[i].m_camera = camera;
        }
    }

    if (data->m_ribbons.m_count) {
        this->m_ribbons = reinterpret_cast<M2ModelRibbon*>(cursor);
        cursor += data->m_ribbons.m_count * sizeof(M2ModelRibbon);

        for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
            new (&this->m_ribbons[i]) M2ModelRibbon();
        }

        this->m_ribbonEmitters = reinterpret_cast<CRibbonEmitter**>(cursor);
        cursor += data->m_ribbons.m_count * sizeof(CRibbonEmitter*);
        memset(this->m_ribbonEmitters, 0, data->m_ribbons.m_count * sizeof(CRibbonEmitter*));

        uint8_t* emitterMemory = memory + ribbonOffset;

        for (uint32_t i = 0; i < data->m_ribbons.m_count; i++) {
            M2Ribbon* ribbonData = &data->m_ribbons[i];
            CRibbonEmitter* emitter = new (emitterMemory) CRibbonEmitter();
            emitterMemory += sizeof(CRibbonEmitter);
            this->m_ribbonEmitters[i] = emitter;

            static TSGrowableArray<CTexture*> s_ribbonTextures;
            static TSGrowableArray<CRibbonMat> s_ribbonMaterials;
            static TSGrowableArray<uint32_t> s_ribbonTextureTypes;

            s_ribbonTextures.SetCount(ribbonData->m_textureIndices.m_count);
            s_ribbonMaterials.SetCount(ribbonData->m_textureIndices.m_count);
            s_ribbonTextureTypes.SetCount(ribbonData->m_textureIndices.m_count);

            for (uint32_t j = 0; j < ribbonData->m_textureIndices.m_count; j++) {
                uint16_t textureIndex = ribbonData->m_textureIndices[j];
                s_ribbonTextures.m_data[j] = this->m_shared->m_textures[textureIndex];
                s_ribbonTextureTypes.m_data[j] = this->m_shared->m_data->m_textures[textureIndex].m_type;

                M2Material* material = &this->m_shared->m_data->m_materials[ribbonData->m_materialIndices[j]];
                CRibbonMat* ribbonMaterial = &s_ribbonMaterials.m_data[j];
                ribbonMaterial->m_lit = !(material->m_flags & 1);
                ribbonMaterial->m_fogged = !(material->m_flags & 2);
                ribbonMaterial->m_depthTest = !(material->m_flags & 8);
                ribbonMaterial->m_depthWrite = !(material->m_flags & 0x10);
                ribbonMaterial->m_cull = !(material->m_flags & 4);
                ribbonMaterial->m_blendMode = s_ribbonBlendModes[material->m_blendMode];
            }

            CAaRect texCoords;
            texCoords.minX = 0.0f;
            texCoords.minY = 0.0f;
            texCoords.maxX = 1.0f;
            texCoords.maxY = 1.0f;

            CImVector color;
            color.b = 0xFF;
            color.g = 0xFF;
            color.r = 0xFF;
            color.a = 0xFF;

            this->m_ribbonEmitters[i]->Initialize(ribbonData->m_edgesPerSecond, ribbonData->m_edgeLifetime, color, s_ribbonTextures, s_ribbonMaterials, s_ribbonTextureTypes, texCoords, ribbonData->m_textureRows, ribbonData->m_textureCols);
            this->m_ribbonEmitters[i]->SetGravity(ribbonData->m_gravity);
            this->m_ribbonEmitters[i]->SetDataEnabled(0);
            this->m_ribbons[i].m_visibilityTrack.m_currentValue = 1;
            this->m_ribbons[i].m_alphaTrack.m_currentValue = 1.0f;
        }
    }

    if (data->m_particles.m_count) {
        this->m_particles = reinterpret_cast<M2ModelParticle*>(cursor);
        cursor += data->m_particles.m_count * sizeof(M2ModelParticle);

        for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
            new (&this->m_particles[i]) M2ModelParticle();
        }

        this->m_particleEmitters = reinterpret_cast<CParticleEmitter2**>(cursor);
        memset(this->m_particleEmitters, 0, data->m_particles.m_count * sizeof(CParticleEmitter2*));

        uint8_t* emitterMemory = memory + emitterOffset;

        for (uint32_t i = 0; i < data->m_particles.m_count; i++) {
            M2Particle* particleData = &data->m_particles[i];
            M2ModelParticle* particle = &this->m_particles[i];

            if (particleData->m_emitterType == 1) {
                this->m_particleEmitters[i] = new (emitterMemory) CPlaneParticleEmitter();
                emitterMemory += sizeof(CPlaneParticleEmitter);
            } else if (particleData->m_emitterType == 2) {
                this->m_particleEmitters[i] = new (emitterMemory) CSphereParticleEmitter();
                emitterMemory += sizeof(CSphereParticleEmitter);
            } else if (particleData->m_emitterType == 3) {
                this->m_particleEmitters[i] = new (emitterMemory) CSplineParticleEmitter();
                emitterMemory += sizeof(CSplineParticleEmitter);
            }

            CParticleEmitter2* emitter = this->m_particleEmitters[i];
            emitter->m_flags &= ~1u;

            if (particleData->m_emissionRateTrack.m_sequenceKeys[0].m_count) {
                emitter->SetEmissionRate(particleData->m_emissionRateTrack.m_sequenceKeys[0][0]);
            }

            emitter->m_params.m_emissionRateVariation = particleData->m_emissionRateVariation;

            if (particleData->m_speedTrack.m_sequenceKeys[0].m_count) {
                emitter->m_params.m_speed = particleData->m_speedTrack.m_sequenceKeys[0][0];
            }

            if (particleData->m_variationTrack.m_sequenceKeys[0].m_count) {
                emitter->m_params.m_variation = particleData->m_variationTrack.m_sequenceKeys[0][0];
            }

            if (particleData->m_latitudeTrack.m_sequenceKeys[0].m_count) {
                emitter->SetLatitude(particleData->m_latitudeTrack.m_sequenceKeys[0][0]);
            }

            if (particleData->m_longitudeTrack.m_sequenceKeys[0].m_count) {
                emitter->SetLongitude(particleData->m_longitudeTrack.m_sequenceKeys[0][0]);
            }

            if (particleData->m_gravityTrack.m_sequenceKeys[0].m_count) {
                emitter->m_params.m_gravity = particleData->m_gravityTrack.m_sequenceKeys[0][0];
            }

            if (particleData->m_lifeTrack.m_sequenceKeys[0].m_count) {
                emitter->m_params.m_life = particleData->m_lifeTrack.m_sequenceKeys[0][0];
            }

            emitter->m_params.m_lifeVariation = particleData->m_lifeVariation;

            if (particleData->m_widthTrack.m_sequenceKeys[0].m_count) {
                emitter->SetWidth(particleData->m_widthTrack.m_sequenceKeys[0][0]);
            }

            if (particleData->m_lengthTrack.m_sequenceKeys[0].m_count) {
                emitter->SetHeight(particleData->m_lengthTrack.m_sequenceKeys[0][0]);
            }

            if (particleData->m_zsourceTrack.m_sequenceKeys[0].m_count) {
                emitter->SetZsource(particleData->m_zsourceTrack.m_sequenceKeys[0][0]);
            }

            uint32_t flags = particleData->m_flags;

            if (flags & 0x10) {
                emitter->m_flags |= 0x200;
            } else {
                emitter->m_flags &= ~0x200u;
            }

            if (flags & 0x40) {
                emitter->m_flags |= 0x800;
            }

            if (flags & 0x20) {
                emitter->m_flags |= 0x400;
            }

            if (flags & 0x800) {
                emitter->m_flags |= 0x2000;
            }

            if (flags & 0x1000) {
                emitter->m_flags |= 0x4000;
            }

            if (particleData->m_emitterType == 2) {
                if (flags & 0x80) {
                    emitter->m_flags |= 0x1000;
                }

                if (flags & 0x100) {
                    emitter->m_flags |= 0x8000;
                }
            }

            if (flags & 0x200) {
                emitter->m_flags |= 0x10000;
            }

            if (flags & 0x2000) {
                emitter->m_flags |= 0x40000;
            }

            if (flags & 0x4000) {
                emitter->m_flags |= 0x80000;
            }

            if (flags & 0x8000) {
                emitter->m_flags &= ~1u;
            }

            if (flags & 0x80000) {
                emitter->m_flags |= 0x800000;
            }

            emitter->SetParticleStyle(flags & 0x20000, flags & 0x40000, particleData->m_tailLength, flags & 0x400);

            CParticleMat material;
            material.m_flags = 7;
            material.m_blendMode = GxBlend_Opaque;

            switch (particleData->m_blendMode) {
                case M2PARTICLEBLEND_OPAQUE:
                    material.m_blendMode = GxBlend_Opaque;
                    material.m_flags |= 4;
                    break;

                case M2PARTICLEBLEND_ALPHA_KEY:
                    material.m_blendMode = GxBlend_AlphaKey;
                    material.m_flags |= 4;
                    break;

                case M2PARTICLEBLEND_ALPHA:
                    material.m_blendMode = GxBlend_Alpha;
                    material.m_flags &= ~4u;
                    break;

                case M2PARTICLEBLEND_NO_ALPHA_ADD:
                    material.m_blendMode = GxBlend_NoAlphaAdd;
                    material.m_flags &= ~4u;
                    break;

                case M2PARTICLEBLEND_ADD:
                    material.m_blendMode = GxBlend_Add;
                    material.m_flags &= ~4u;
                    break;

                case M2PARTICLEBLEND_MOD:
                    material.m_blendMode = GxBlend_Mod;
                    material.m_flags &= ~4u;
                    break;

                case M2PARTICLEBLEND_MOD2X:
                    material.m_blendMode = GxBlend_Mod2x;
                    material.m_flags &= ~4u;
                    break;
            }

            material.m_flags = (material.m_flags & ~1u) | ((flags & 1) ? 0 : 1);
            material.m_flags = (material.m_flags & ~2u) | ((flags & 8) ? 0 : 2);

            if (flags & 2) {
                emitter->m_flags |= 0x20;
            }

            if (flags & 4) {
                emitter->m_flags |= 0x200000;
            }

            emitter->SetTextureDimensions(particleData->m_rows, particleData->m_cols);

            if (flags & 0x10000) {
                emitter->SetChooseRandomTexture(1);
            }

            emitter->SetMaterial(material, this->m_shared->m_textures[particleData->m_textureIndex]);
            emitter->m_params.m_colorTrack = &particleData->m_colorTrack;
            emitter->m_params.m_scaleTrack = &particleData->m_scaleTrack;
            emitter->m_params.m_alphaTrack = &particleData->m_alphaTrack;
            emitter->m_params.m_scaleVariationX = particleData->m_scaleVariation.x;
            emitter->m_params.m_scaleVariationY = particleData->m_scaleVariation.y;
            emitter->m_params.m_headCellTrack = &particleData->m_headCellTrack;
            emitter->m_params.m_tailCellTrack = &particleData->m_tailCellTrack;

            CM2Model* source = this->m_model30;

            if (source && (source->m_particleEmitters[i]->m_flags & 0x10)) {
                CImVector color0;
                CImVector color1;
                CImVector color2;
                source->m_particleEmitters[i]->GetReplacementColors(color0, color1, color2);
                emitter->SetParticleColors(color0, color1, color2);
            }

            emitter->m_twinkleFPS = particleData->m_twinkleFPS;
            emitter->m_twinkleOnOff = particleData->m_twinkleOnOff;
            emitter->SetTwinkleScale(particleData->m_twinkleScale);
            emitter->m_ivelScale = particleData->m_ivelScale;
            emitter->m_tumbleBaseX = particleData->m_tumble.b.x;
            emitter->m_tumbleSpanX = particleData->m_tumble.t.x - particleData->m_tumble.b.x;
            emitter->m_tumbleBaseY = particleData->m_tumble.b.y;
            emitter->m_tumbleSpanY = particleData->m_tumble.t.y - particleData->m_tumble.b.y;
            emitter->m_tumbleBaseZ = particleData->m_tumble.b.z;
            emitter->m_tumbleSpanZ = particleData->m_tumble.t.z - particleData->m_tumble.b.z;
            emitter->m_drag = particleData->m_drag;
            emitter->m_params.m_initialSpin = particleData->m_initialSpin;
            emitter->m_params.m_initialSpinVariation = particleData->m_initialSpinVariation;
            emitter->m_params.m_spin = particleData->m_spinVariation;
            emitter->m_params.m_spinVariation = particleData->m_spin;
            emitter->m_windVectorX = particleData->m_windVector.x;
            emitter->m_windVectorY = particleData->m_windVector.y;
            emitter->m_windTime = particleData->m_windTime;
            emitter->m_windVectorZ = particleData->m_windVector.z;
            emitter->SetFollowParams(particleData->m_followSpeed1, particleData->m_followScale1, particleData->m_followSpeed2, particleData->m_followScale2);
            emitter->m_priorityPlane = particleData->m_priorityPlane;

            if (particleData->m_emitterType == 3) {
                static_cast<CSplineParticleEmitter*>(emitter)->SetSpline(particleData->m_spline.Data(), particleData->m_spline.m_count);
            }

            if (particleData->m_geometryMdl.m_offset && *particleData->m_geometryMdl.Data()) {
                emitter->SetModel(this->m_scene, particleData->m_geometryMdl.Data());
            }

            if (particleData->m_recursionMdl.m_offset && *particleData->m_recursionMdl.Data()) {
                emitter->CreateChildEmittersFromModel(this->m_scene, particleData->m_recursionMdl.Data());
            }

            emitter->DetermineIfSimple();
            particle->m_visibilityTrack.m_currentValue = 1;
        }
    }

    uint32_t optimize = 0;

    if (this->m_shared->m_maxInstances > 1 && (!this->m_model30 || (this->m_model30->m_fFlags & 0x10))) {
        optimize = 1;
    }

    CM2Model* source = this->m_model30;
    this->m_fFlags = (this->m_fFlags & ~0x10u) | (optimize << 4);

    if (source) {
        if (source->m_refCount-- == 1) {
            g_modelPool->Delete(source);
        }

        this->m_model30 = nullptr;
    }

    CM2Model** link = &this->m_attachList;

    while (*link) {
        CM2Model* child = *link;
        uint32_t id = child->m_attachmentId;
        M2Data* current = this->m_shared->m_data;
        uint16_t index = id < current->m_attachmentIndicesById.m_count ? current->m_attachmentIndicesById[id] : 0xFFFF;

        if (index != 0xFFFF || (child->m_fFlags & 0x40000)) {
            child->m_attachmentIndex = index;
            link = &(*link)->m_attachNext;
        } else {
            child->DetachFromParent();
        }
    }

    CM2Shared* currentShared = this->m_shared;
    M2Data* currentData = currentShared->m_data;

    if (currentData->m_bones.m_count == 1 && currentData->m_sequences.m_count == 1) {
        uint32_t boneFlags = currentData->m_bones[0].m_flags;

        if (!(boneFlags & 0x280) && !(boneFlags & 0x78) && !currentData->m_lights.m_count && !currentData->m_cameras.m_count && !currentData->m_ribbons.m_count && !currentData->m_particles.m_count && !currentData->m_colors.m_count) {
            this->m_fFlags |= 0x1000;
        }
    }

    this->m_fFlags = (this->m_fFlags & 0xFFFFF7FC) | 0x801;

    M2SequenceFallback fallback;
    uint16_t standId = this->SequenceFallbackById(&fallback, 0)->m_id;

    if (!(this->m_fFlags & 1)) {
        this->WaitForLoad(nullptr);
    }

    uint32_t sequenceId = this->HasSequence(this->m_shared->m_data, standId) ? 0 : data->m_sequences[0].m_id;
    this->SetBoneSequence(0xFFFFFFFF, sequenceId, 0xFFFFFFFF, 0, 1.0f, 0, 1);

    if (this->m_sequenceCallback || data->m_sequences.m_count > 1 || (this->m_eventCallback && data->m_events.m_count)) {
        this->m_fFlags |= 0x400000;
    }

    uint32_t time = this->m_scene->m_time;

    while (this->m_modelCallList) {
        CM2ModelCall* call = this->m_modelCallList;
        this->m_scene->m_time = call->m_time;

        switch (call->m_type) {
            case MODEL_REPLACE_TEXTURE:
                this->ReplaceTexture(call->m_args[0], reinterpret_cast<CTexture*>(call->m_args[1]));
                break;

            case MODEL_SET_GEOMETRY_VISIBLE:
                this->SetGeometryVisible(call->m_args[0], call->m_args[1], call->m_args[2]);
                break;

            case MODEL_SET_GEOSET_VISIBILITY_RANGE:
                this->SetGeosetVisibilityRange(call->m_args[0], call->m_args[1], call->m_args[2]);
                break;

            case MODEL_OPTIMIZE_VISIBLE_GEOSETS:
                this->OptimizeVisibleGeometry();
                break;

            case MODEL_SET_BONE_FLAGS:
                this->SetBoneFlags(call->m_args[0], call->m_args[1], call->m_args[2]);
                break;

            case MODEL_SET_BONE_SEQUENCE:
                this->SetBoneSequence(call->m_args[0], call->m_args[1], call->m_args[2], call->m_args[3], *reinterpret_cast<float*>(&call->m_args[4]), call->m_args[5], call->m_args[6]);
                break;

            case MODEL_UNSET_BONE_SEQUENCE:
                this->UnsetBoneSequence(call->m_args[0], call->m_args[1], call->m_args[2]);
                break;

            case MODEL_SET_BONE_SEQUENCE_TIME:
                this->SetBoneSequenceTime(call->m_args[0], call->m_args[1]);
                break;

            case MODEL_SET_BONE_SEQUENCE_SPEED:
                this->SetBoneSequenceSpeed(call->m_args[0], *reinterpret_cast<float*>(&call->m_args[1]));
                break;

            case MODEL_SET_BONE_PROCEDURAL_TRANSFORM:
                this->SetBoneProceduralTransform(call->m_args[0], *reinterpret_cast<C44Matrix*>(&call->m_args[1]));
                break;

            case MODEL_SET_LIGHT_ENABLED:
                this->SetLightEnabled(call->m_args[0], call->m_args[1]);
                break;

            case MODEL_REPLACE_PARTICLE_COLOR:
                this->ReplaceParticleColor(call->m_args[0], *reinterpret_cast<CImVector*>(&call->m_args[1]), *reinterpret_cast<CImVector*>(&call->m_args[2]), *reinterpret_cast<CImVector*>(&call->m_args[3]));
                break;

            case MODEL_SET_EMITTERS_ENABLED:
                this->SetEmittersEnabled(call->m_args[0]);
                break;

            case MODEL_SET_RIBBONS_ENABLED:
                this->SetRibbonsEnabled(call->m_args[0]);
                break;

            case MODEL_LOAD_SEQUENCE:
                this->LoadSequence(call->m_args[0]);
                break;
        }

        this->m_modelCallList = call->m_next;

        if (!call->m_type && call->m_args[1]) {
            HandleClose(reinterpret_cast<HOBJECT>(call->m_args[1]));
        }

        delete call;
    }

    this->m_scene->m_time = time;
    this->UpdateLoaded();
    this->m_fFlags &= ~0x800u;
    return 1;
}

// VERIFY: 320c6ca5 manual
// OFFSET: 0x00834540
void CM2Model::AttachToScene(CM2Scene* scene) {
    this->DetachFromScene();

    this->m_scenePrev = &scene->m_modelList;
    this->m_scene = scene;
    this->m_sceneNext = scene->m_modelList;
    scene->m_modelList = this;

    if (this->m_sceneNext) {
        this->m_sceneNext->m_scenePrev = &this->m_sceneNext;
    }

    if (this->m_fFlags & 1) {
        M2Data* data = this->m_shared->m_data;

        for (uint32_t i = 0; i < data->m_lights.m_count; i++) {
            this->m_lights[i].m_light.Initialize(scene);
        }

        M2SequenceFallback fallback;
        uint32_t sequenceId = this->HasSequence(data, this->SequenceFallbackById(&fallback, 0)->m_id) ? 0 : data->m_sequences[0].m_id;
        this->SetBoneSequence(0xFFFFFFFF, sequenceId, 0xFFFFFFFF, 0, 1.0f, 0, 1);
    } else {
        for (CM2ModelCall* call = this->m_modelCallList; call; call = call->m_next) {
            call->m_time += this->m_scene->m_time;
        }
    }
}

// VERIFY: d3d33312 auto
// OFFSET: 0x00834630
void CM2Model::AttachToSceneRecursive(CM2Scene* scene) {
    this->AttachToScene(scene);

    for (CM2Model* child = this->m_attachList; child; child = child->m_attachNext) {
        child->AttachToSceneRecursive(scene);
    }
}

// VERIFY: 3596777a manual
// OFFSET: 0x00834660
void CM2Model::GatherModelBatchesShadowMap(CModelBatchList& untextured, CModelBatchList& textured) {
    if (!(this->m_fFlags & 1)) {
        if (!(this->m_flags & 0x20)) {
            return;
        }

        uint32_t sharedFlags = this->m_shared->m_flags;

        if (!(sharedFlags & 1) || !(sharedFlags & 2)) {
            return;
        }

        this->InitializeLoaded();
    }

    this->AnimateSM();

    M2SkinProfile* skin = this->m_shared->m_skinData;
    bool optimized = this->m_optGeo != nullptr;
    uint32_t count = optimized ? this->m_optGeo->m_batchCount : skin->m_batches.m_count;

    for (uint32_t i = 0; i < count; i++) {
        M2Batch* batch;

        if (optimized) {
            batch = &this->m_optGeo->m_batches[i];
        } else {
            batch = &skin->m_batches[i];

            if (!this->m_skinSections[batch->m_skinSectionIndex]) {
                continue;
            }
        }

        if (batch->m_shader == 0x8000 || (batch->m_flags & 4) || batch->m_materialLayer) {
            continue;
        }

        M2Data* data = this->m_shared->m_data;
        M2Material* material = &data->m_materials[batch->m_materialIndex];

        if ((material->m_flags & 0x40) || (!(material->m_flags & 0x80) && material->m_blendMode > M2BLEND_ALPHA_KEY)) {
            continue;
        }

        float alpha = this->m_alpha;

        if (batch->m_colorIndex < data->m_colors.m_count) {
            alpha *= this->m_colors[batch->m_colorIndex].m_alphaTrack.m_currentValue;
        }

        if (batch->m_textureCount) {
            alpha *= this->m_textureWeights[data->m_textureWeightCombos[batch->m_textureWeightComboIndex]].m_currentValue;
        }

        if (alpha >= 0.55000001f) {
            (material->m_blendMode ? textured : untextured).Add(this, i);
        }
    }

    for (CM2Model* child = this->m_attachList; child; child = child->m_attachNext) {
        child->GatherModelBatchesShadowMap(untextured, textured);
    }
}

// VERIFY: ad015f89 manual
// OFFSET: 0x00834810
int32_t CM2Model::Initialize(CM2Scene* scene, CM2Shared* shared, CM2Model* source, uint32_t flags) {
    this->AttachToScene(scene);
    this->m_lastEmitterTime = this->m_scene->m_time;
    this->m_shared = shared;
    shared->AddRef();
    this->m_model30 = source;

    if (source) {
        source->m_refCount++;
    }

    this->m_flags = flags;
    this->m_modelCallTail = &this->m_modelCallList;
    this->m_loopOrigin = this->m_scene->m_time;

    if (this->m_model30) {
        for (CM2Model* child = this->m_model30->m_attachList; child; child = child->m_attachNext) {
            CM2Model* copy = this->m_scene->DuplicateModel(child, 0);

            if (copy) {
                copy->AttachToParent(this, child->m_attachmentId, nullptr, 0);

                if (copy->m_refCount-- == 1) {
                    g_modelPool->Delete(copy);
                }
            }
        }
    }

    return this->m_shared->CallbackWhenLoaded(this);
}

// VERIFY: 767a1eb1 auto
// OFFSET: 0x004D8630
void CM2Model::SetWorldTransform(const C44Matrix& matrix) {
    this->m_fFlags |= 0x8000;
    this->m_worldTransform = matrix;
}

// VERIFY: 3fa6010f auto
// OFFSET: 0x004E2730
void CM2Lighting::Reset() {
    this->m_sunAmbient = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDiffuse = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunSpecular = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDir = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector18 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector24 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector30 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector3C = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector48 = C3Vector(0.0f, 0.0f, 0.0f);
}

// VERIFY: 56d2a807 auto
// OFFSET: 0x004F5E20
CAaBox CM2Model::GetBoundingBox() {
    if (!(this->m_shared->m_flags & 1)) {
        this->WaitForLoad(nullptr);
    }

    return this->m_shared->m_data->m_bounds.m_extent;
}

// VERIFY: 542067ba manual
// OFFSET: 0x004F5E80
CAaSphere CM2Model::GetBoundingSphere() {
    if (!(this->m_shared->m_flags & 1)) {
        this->WaitForLoad(nullptr);
    }

    const M2Bounds& bounds = this->m_shared->m_data->m_bounds;
    CAaSphere sphere;
    sphere.c = C3Vector((bounds.m_extent.b.x + bounds.m_extent.t.x) * 0.5f, (bounds.m_extent.b.y + bounds.m_extent.t.y) * 0.5f, (bounds.m_extent.b.z + bounds.m_extent.t.z) * 0.5f);
    sphere.r = bounds.m_radius;

    return sphere;
}

// VERIFY: f3350a9a auto
// OFFSET: 0x006F1D20
C44Matrix CM2Model::GetAnimatedWorldTransform() {
    this->Animate();

    return this->m_matrixF4 * this->m_scene->m_viewInv;
}

// VERIFY: bf5b6c6a manual
// OFFSET: 0x00790440
CM2Lighting::CM2Lighting(const CAaSphere& sphere) {
    this->m_sphere4.c.x = 0.0f;
    this->m_sphere4.r = 0.0f;
    this->m_sphere4.c.y = 0.0f;
    this->m_sphere4.c.z = 0.0f;
    this->m_vector18 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector24 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector30 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector3C = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector48 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunAmbient = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDiffuse = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunSpecular = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDir = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_fogColor = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_liquidPlane.n.x = 0.0f;
    this->m_liquidPlane.n.y = 0.0f;
    this->m_liquidPlane.n.z = 1.0f;
    this->m_liquidPlane.d = 0.0f;
    this->Initialize(nullptr, sphere);
}

// VERIFY: 8535c10a manual
// OFFSET: 0x007B0F10
CM2Lighting::CM2Lighting(const C3Vector& position) {
    this->m_sphere4.c.x = 0.0f;
    this->m_sphere4.r = 0.0f;
    this->m_sphere4.c.y = 0.0f;
    this->m_sphere4.c.z = 0.0f;
    this->m_vector18 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector24 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector30 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector3C = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_vector48 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunAmbient = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDiffuse = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunSpecular = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sunDir = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_fogColor = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_liquidPlane.n.x = 0.0f;
    this->m_liquidPlane.n.y = 0.0f;
    this->m_liquidPlane.n.z = 1.0f;
    this->m_liquidPlane.d = 0.0f;

    CAaSphere sphere;
    sphere.c = position;
    sphere.r = 0.0f;
    this->Initialize(nullptr, sphere);
}

// VERIFY: 4329ef5d manual
// OFFSET: 0x0052E570
void M2SequenceInfo::Clear() {
    this->m_bounds.b.x = 0.0f;
    this->m_bounds.b.y = 0.0f;
    this->m_bounds.b.z = 0.0f;
    this->m_bounds.t.x = 0.0f;
    this->m_bounds.t.y = 0.0f;
    this->m_bounds.t.z = 0.0f;
    this->m_center = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_radius = 0.0f;
}

// VERIFY: 7202abde auto
// OFFSET: 0x007BABA0
uint32_t CM2Model::GetBillboardBoneCount() const {
    if (this->m_fFlags & 0x1) {
        return this->m_shared->m_billboardBoneCount;
    }

    return 0;
}
