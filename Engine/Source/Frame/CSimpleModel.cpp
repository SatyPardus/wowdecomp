#include <cmath>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/CameraInt.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// STATIC: 0x00B2CB0C
const char* CSimpleModel::s_objectTypeName = "Model";

// STATIC: 0x00DCE428
int32_t CSimpleModel::s_objectType;

// STATIC: 0x00DCE42C
CDataAllocator CSimpleModel::s_allocator(sizeof(CSimpleModel), 3);

// VERIFY: e99a2a7c auto
// OFFSET: 0x0095F370
static void AnimFinishedCallback(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (!interrupted) {
        CSimpleModel* simpleModel = reinterpret_cast<CSimpleModel*>(static_cast<uintptr_t>(param));

        if (simpleModel->m_onAnimFinished.m_luaRef) {
            simpleModel->RunScript(simpleModel->m_onAnimFinished, 0, nullptr);
        }
    }
}

// INLINED
int32_t CSimpleModel::GetObjectType() {
    if (!CSimpleModel::s_objectType) {
        CSimpleModel::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleModel::s_objectType;
}

// INLINED
void CSimpleModel::operator delete(void* ptr) {
    if (ptr) {
        CSimpleModel::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 56a4df90 auto
// OFFSET: 0x004E60B0
int32_t CSimpleModel::IsDrawable() {
    return this->m_model && this->m_model->IsDrawable(0, 1);
}

// VERIFY: 9bb9dd69 auto
// OFFSET: 0x004E60D0
bool CSimpleModel::IsA(int32_t type) {
    return type == CSimpleModel::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 3d38f994 auto
// OFFSET: 0x004E6110
bool CSimpleModel::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleModel::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 2b17add5 auto
// OFFSET: 0x004E6150
const char* CSimpleModel::GetObjectTypeName() {
    return CSimpleModel::s_objectTypeName;
}

// VERIFY: 504a298c auto
// OFFSET: 0x00632050
void CSimpleModel::SetGlow(float glow) {
}

// VERIFY: 13ff18fe auto
// OFFSET: 0x008C8DE0
int32_t CSimpleModel::AdvanceTime() {
    return 1;
}

// VERIFY: d4b291b5 auto
// OFFSET: [0x0095F3A0, 0x0095F670, 0x0095F7D0]
CSimpleModel::~CSimpleModel() {
    if (this->m_camera) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_camera));
    }

    if (this->m_model) {
        this->m_model->Release();
    }

    if (this->m_scene) {
        this->m_scene->Release();
    }
}

// VERIFY: 8bffeeff auto
// OFFSET: 0x0095F410
int32_t CSimpleModel::GetScriptMetaTable() {
    return CSimpleModel::s_metatable;
}

// VERIFY: 79a3582b auto
// OFFSET: 0x0095F420
FrameScript_Object::ScriptIx* CSimpleModel::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnUpdateModel", STORM_MAX_STR)) {
        return &this->m_onUpdateModel;
    }

    if (!SStrCmpI(name, "OnAnimFinished", STORM_MAX_STR)) {
        return &this->m_onAnimFinished;
    }

    return nullptr;
}

// VERIFY: 4c330b1e auto
// OFFSET: 0x0095F480
void CSimpleModel::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onUpdateModel, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onAnimFinished, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: a46fe975 auto
// OFFSET: 0x0095F4D0
void CSimpleModel::ModelLoaded(CM2Model* model, void* arg) {
    static_cast<CSimpleModel*>(arg)->OnModelLoaded(model);
}

// VERIFY: 21715380 auto
// OFFSET: 0x0095F4F0
void CSimpleModel::SetCamera(CCamera* camera) {
    CCamera* newCamera = camera;

    if (camera) {
        newCamera = reinterpret_cast<CCamera*>(HandleDuplicate(reinterpret_cast<HOBJECT>(camera)));
    }

    if (this->m_camera) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_camera));
    }

    this->m_camera = newCamera;
    this->m_pendingCameraIndex = 0xFFFFFFFF;
    this->m_pendingCameraId = 0xFFFFFFFF;
}

// VERIFY: 26f68f9c auto
// OFFSET: 0x0095F540
void CSimpleModel::SetCameraByID(uint32_t id) {
    if (!this->m_model || !this->m_model->IsLoaded(0, 0)) {
        this->m_pendingCameraId = id;
        return;
    }

    if (id < NUM_OBJECT_CAMERAS) {
        this->SetCamera(this->m_model->GetCameraById(id));
        return;
    }

    if (this->m_camera) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_camera));
    }

    this->m_camera = nullptr;
    this->m_pendingCameraIndex = 0xFFFFFFFF;
    this->m_pendingCameraId = 0xFFFFFFFF;
}

// VERIFY: 99c16903 auto
// OFFSET: 0x0095F5C0
void CSimpleModel::SetLight(const CM2Light& light) {
    this->m_light = light;
}

// VERIFY: bd3e4cc4 auto
// OFFSET: 0x0095F5E0
void CSimpleModel::SetSequence(uint32_t sequence) {
    if (this->m_model) {
        this->m_model->SetBoneSequence(0xFFFFFFFF, sequence, 0xFFFFFFFF, 0, 1.0f, 0, 1);
    }
}

// VERIFY: 5e59b8f4 auto
// OFFSET: 0x0095F610
int32_t CSimpleModel::SetSequenceTime(uint32_t sequence, int32_t time) {
    if (this->m_model) {
        this->m_model->SetBoneSequence(0xFFFFFFFF, sequence, 0xFFFFFFFF, time, 1.0f, 0, 1);
    }

    return 1;
}

// VERIFY: 209c9b4c auto
// OFFSET: 0x0095F650
CM2Scene* CSimpleModel::GetScene() {
    if (!this->m_scene) {
        this->m_scene = M2CreateScene();
    }

    return this->m_scene;
}

// VERIFY: 2fd138ba manual
// OFFSET: 0x0095F680
CSimpleModel::CSimpleModel(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_scene = nullptr;
    this->m_model = nullptr;
    this->m_camera = nullptr;
    this->m_pendingCameraIndex = 0;
    this->m_pendingCameraId = 0;
    this->m_facing = 0.0f;
    this->m_flags = 0;
    this->m_modelScale = 1.0f;
    this->m_fogNear = 0.0f;
    this->m_fogFar = 1.0f;
    this->m_bounds.b = C3Vector();
    this->m_bounds.t = C3Vector();
    this->m_light.SetVisible(0);
    this->m_light.SetLightType(M2LIGHT_POINT);
    this->m_light.m_ambColor = C3Vector(1.0f, 1.0f, 1.0f);
    this->m_light.m_dirColor = C3Vector(1.0f, 1.0f, 1.0f);
    this->m_fogColor.b = 0xFF;
    this->m_fogColor.g = 0xFF;
    this->m_fogColor.r = 0xFF;
    this->m_fogColor.a = 0xFF;
}

// VERIFY: ac71f78a auto
// OFFSET: 0x0095F800
void CSimpleModel::LoadXML(XMLNode* node, CStatus* status) {
    this->CSimpleFrame::LoadXML(node, status);

    const char* file = node->GetAttributeByName("file");

    if (file && *file) {
        this->SetModel(file);

        if (!this->m_model) {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Bad model file: %s", name ? name : "<unnamed>", file);
        }
    }

    const char* scale = node->GetAttributeByName("scale");

    if (scale && *scale) {
        float value = SStrToFloat(scale);

        if (value > 0.0f) {
            this->m_modelScale = value;
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Invalid model scale: %s", name ? name : "<unnamed>", scale);
        }
    }

    const char* fogNear = node->GetAttributeByName("fogNear");

    if (fogNear && *fogNear) {
        float value = SStrToFloat(fogNear);
        this->m_fogNear = value < 0.0f ? 0.0f : value;
    }

    const char* fogFar = node->GetAttributeByName("fogFar");

    if (fogFar && *fogFar) {
        float value = SStrToFloat(fogFar);
        this->m_fogFar = value < 0.0f ? 0.0f : value;
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "FogColor", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->m_fogColor = color;
            this->m_flags |= CSimpleModel::MODEL_FOGGED;
        }
    }
}

// VERIFY: 4963ebe3 auto
// OFFSET: 0x0095F990
void CSimpleModel::SetModel(const char* fileName) {
    CM2Model* model = nullptr;

    if (fileName && *fileName) {
        model = this->GetScene()->CreateModel(fileName, 1);
    }

    this->SetModel(model);

    if (model) {
        model->Release();
    }
}

// VERIFY: 75d88bb3 auto
// OFFSET: 0x0095F9F0
void CSimpleModel::SetCameraByIndex(uint32_t index) {
    if (!this->m_model || !this->m_model->IsLoaded(0, 0)) {
        this->m_pendingCameraIndex = index;
        return;
    }

    CM2Model* model = this->m_model;

    if (!(model->m_fFlags & 0x1)) {
        model->WaitForLoad(nullptr);
    }

    if (index < model->m_shared->m_data->m_cameras.m_count) {
        this->SetCamera(static_cast<CCamera*>(this->m_model->GetCameraByIndex(index)));
        return;
    }

    if (this->m_camera) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_camera));
    }

    this->m_camera = nullptr;
    this->m_pendingCameraIndex = 0xFFFFFFFF;
    this->m_pendingCameraId = 0xFFFFFFFF;
}

// VERIFY: 2982168f auto
// OFFSET: 0x0095FAA0
float CSimpleModel::GetWidth() {
    float width = this->CLayoutFrame::GetWidth();

    if (width == 0.0f && this->m_model && this->m_model->IsLoaded(0, 0)) {
        return this->m_bounds.t.x - this->m_bounds.b.x;
    }

    return width;
}

// VERIFY: f3d8262a auto
// OFFSET: 0x0095FAF0
float CSimpleModel::GetHeight() {
    float height = this->CLayoutFrame::GetHeight();

    if (height == 0.0f && this->m_model && this->m_model->IsLoaded(0, 0)) {
        return this->m_bounds.t.y - this->m_bounds.b.y;
    }

    return height;
}

// VERIFY: 554b25c2 manual
// OFFSET: 0x0095FB40
void CSimpleModel::UpdateAlpha() {
    this->CSimpleFrame::UpdateAlpha();

    CM2Model* model = this->m_model;

    if (model) {
        model->m_fadeAlpha = static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255) * 0.0039215689f;
    }
}

// VERIFY: ff61c16f auto
// OFFSET: 0x0095FBA0
void CSimpleModel::UpdateModel() {
    if (this->m_onUpdateModel.m_luaRef) {
        this->RunScript(this->m_onUpdateModel, 0, nullptr);
    }

    float height = NDCToDDCHeight(1.0f) * 1.6666666f;
    float layoutScale = this->m_layoutScale;

    C3Vector position;
    position.x = layoutScale * this->m_position.x;
    position.y = this->m_position.y * layoutScale;
    position.z = layoutScale * this->m_position.z;

    this->m_model->SetWorldTransform(position, this->m_facing, this->m_modelScale * this->m_layoutScale * height);
}

// VERIFY: 722f8373 manual
// OFFSET: 0x0095FC30
void CSimpleModel::RenderModel(void* arg) {
    CSimpleModel* simpleModel = static_cast<CSimpleModel*>(arg);

    if (!simpleModel->m_model) {
        return;
    }

    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (!simpleModel->GetRect(&rect)) {
        return;
    }

    CGxDevice* device = g_theGxDevicePtr;
    C44Matrix projection;
    C44Matrix view;
    projection = device->m_projection;
    view = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];

    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

    CCamera* camera = simpleModel->m_camera;
    C3Vector cameraPos;
    C3Vector cameraTarget;

    if (camera) {
        DataMgrGetCoord(camera, 7, &cameraPos);
        DataMgrGetCoord(camera, 8, &cameraTarget);
        CameraSetupWorldProjection(camera, rect, 0);
    } else {
        C2Vector offset;
        offset.x = rect.minX;
        offset.y = rect.minY;
        CameraSetupScreenProjection(rect, offset, 0.0f, 1);
    }

    DDCToNDC(rect.minX, rect.minY, &rect.minX, &rect.minY);
    DDCToNDC(rect.maxX, rect.maxY, &rect.maxX, &rect.maxY);

    rect.minX = rect.minX >= 1.0f ? 1.0f : (rect.minX <= 0.0f ? 0.0f : rect.minX);
    rect.maxX = rect.maxX >= 1.0f ? 1.0f : (rect.maxX <= 0.0f ? 0.0f : rect.maxX);
    rect.minY = rect.minY >= 1.0f ? 1.0f : (rect.minY <= 0.0f ? 0.0f : rect.minY);
    rect.maxY = rect.maxY >= 1.0f ? 1.0f : (rect.maxY <= 0.0f ? 0.0f : rect.maxY);

    if (fabs(rect.minX - rect.maxX) < 0.00000023841858f || fabs(rect.minY - rect.maxY) < 0.00000023841858f) {
        g_theGxDevicePtr->XformSetProjection(projection);
        g_theGxDevicePtr->XformSetView(view);
        GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
        return;
    }

    GxXformSetViewport(rect.minX * maxX, rect.maxX * maxX, rect.minY * maxY, rect.maxY * maxY, 0.0f, 1.0f);

    CImVector clearColor;
    clearColor.b = 0x00;
    clearColor.g = 0x00;
    clearColor.r = 0x00;
    clearColor.a = 0xFF;
    GxSceneClear(GxBufferBit_Depth, clearColor);

    CShaderEffect::UpdateProjMatrix();
    simpleModel->UpdateModel();

    if (simpleModel->m_pendingCameraIndex == 0xFFFFFFFF) {
        simpleModel->GetScene()->Animate(cameraPos);
        simpleModel->GetScene()->Draw(M2PASS_0);
        simpleModel->GetScene()->Draw(M2PASS_1);
    }

    g_theGxDevicePtr->XformSetProjection(projection);
    g_theGxDevicePtr->XformSetView(view);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
    CShaderEffect::UpdateProjMatrix();
}

// VERIFY: ae8cc858 auto
// OFFSET: 0x00960070
void CSimpleModel::LightingCallback(CM2Model* model, CM2Lighting* lighting, void* arg) {
    CSimpleModel* simpleModel = static_cast<CSimpleModel*>(arg);

    if (simpleModel->m_flags & CSimpleModel::MODEL_FOGGED) {
        lighting->SetFog(C3Vector(simpleModel->m_fogColor), simpleModel->m_fogNear, simpleModel->m_fogFar);
    }

    if (simpleModel->m_light.m_visible) {
        lighting->AddLight(&simpleModel->m_light);
    }
}

// VERIFY: 45e34e76 manual
// OFFSET: 0x009600E0
void CSimpleModel::SetModel(CM2Model* model) {
    if (model) {
        model->m_refCount++;
    }

    if (this->m_model) {
        this->m_model->Release();
    }

    this->m_model = model;

    if (model) {
        model->m_fadeAlpha = static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255) * 0.0039215689f;
        this->m_model->SetSequenceCallback(reinterpret_cast<void*>(&AnimFinishedCallback), reinterpret_cast<uintptr_t>(this));
        this->m_model->m_lightingCallback = &CSimpleModel::LightingCallback;
        this->m_model->m_lightingArg = this;
        this->m_model->SetLoadedCallback(&CSimpleModel::ModelLoaded, this);
    }

    this->NotifyDrawLayerChanged(DRAWLAYER_ARTWORK);
}

// VERIFY: b15a7a55 auto
// OFFSET: 0x00960190
void CSimpleModel::OnModelLoaded(CM2Model* model) {
    model->SetAnimating(1);
    model->m_scene->Animate(C3Vector());

    if (this->m_pendingCameraIndex != 0xFFFFFFFF) {
        this->SetCameraByIndex(this->m_pendingCameraIndex);
    } else if (this->m_pendingCameraId != 0xFFFFFFFF) {
        this->SetCameraByID(this->m_pendingCameraId);
    }

    this->m_bounds = model->GetBoundingBox();
    this->Resize(0);
}

// VERIFY: 268cc7ee auto
// OFFSET: 0x00960240
void CSimpleModel::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    this->CSimpleFrame::OnFrameRender(batch, layer);

    if (this->m_model && layer == 2) {
        batch->QueueCallback(&CSimpleModel::RenderModel, this);
    }
}

// VERIFY: 3e188ae3 manual
// OFFSET: 0x00960280
void CSimpleModel::OnLayerUpdate(float elapsedSec) {
    this->CSimpleFrame::OnLayerUpdate(elapsedSec);

    if (this->m_model && this->m_isVisible && this->m_pendingCameraIndex == 0xFFFFFFFF) {
        this->m_model->SetAnimating(1);

        CM2Model* model = this->m_model;

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
    }

    if (!this->m_scene) {
        this->m_scene = M2CreateScene();
    }

    this->m_scene->AdvanceTime(FloatToInt64(elapsedSec * 1000.0f));
}

// VERIFY: d7661726 manual
// OFFSET: 0x00960320
void CSimpleModel::ReplaceTexture(uint32_t type, const char* fileName) {
    if (this->m_model) {
        CStatus status;
        CTexture* texture = TextureCreate(fileName, CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 2);
        SysMsgAdd(&status);
        this->m_model->ReplaceTexture(type, texture);
        HandleClose(reinterpret_cast<HOBJECT>(texture));
    }
}

// VERIFY: 06d1129f auto
// OFFSET: 0x00542F80
void CSimpleModel::SetPosition(const C3Vector& position) {
    this->m_position = position;
}
