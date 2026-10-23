#include "CharacterModelBase.h"
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Common/ComponentCore/Types.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"

// STATIC: 0x00ACF510
const char* CGCharacterModelBase::s_objectTypeName = "PlayerModel";

// STATIC: 0x00ACF534
int32_t CGCharacterModelBase::s_metatable = LUA_NOREF;

// STATIC: 0x00C0E4D4
int32_t CGCharacterModelBase::s_objectType;

// STATIC: 0x00C0E4E0
CDataAllocator CGCharacterModelBase::s_characterModelHeap(sizeof(CGCharacterModelBase), 5);

// VERIFY: 80f18f2c auto
// OFFSET: 0x00597290
static int32_t IsUnitDrawable(CGUnit_C* unit) {
    if (unit->m_clientFlags & 0x400000) {
        return 0;
    }

    if (unit->m_characterComponent && !unit->m_characterComponent->RenderPrep(0)) {
        return 0;
    }

    if (!unit->m_worldModel) {
        return 0;
    }

    return unit->m_worldModel->IsDrawable(0, 1);
}

// INLINED
void CGCharacterModelBase::operator delete(void* ptr) {
    if (ptr) {
        CGCharacterModelBase::s_characterModelHeap.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CGCharacterModelBase::GetObjectType() {
    if (!CGCharacterModelBase::s_objectType) {
        CGCharacterModelBase::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CGCharacterModelBase::s_objectType;
}

// VERIFY: 8c517162 auto
// OFFSET: 0x005142B0
CSimpleFrame* CGCharacterModelBase::Create(CSimpleFrame* parent) {
    if (CGCharacterModelBase::s_characterModelHeap.m_bytesPerData < sizeof(CGCharacterModelBase)) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    void* data = CGCharacterModelBase::s_characterModelHeap.GetData(0, typeid(CGCharacterModelBase).raw_name(), SLOG_OBJECT);
    return data ? new (data) CGCharacterModelBase(parent) : nullptr;
}

// VERIFY: abeaac36 auto
// OFFSET: [0x005970B0, 0x00597280, 0x00597460]
CGCharacterModelBase::~CGCharacterModelBase() {
}

// VERIFY: e353d723 auto
// OFFSET: 0x005970D0
const char* CGCharacterModelBase::GetObjectTypeName() {
    return CGCharacterModelBase::s_objectTypeName;
}

// VERIFY: 71fe68c2 auto
// OFFSET: 0x005970E0
int32_t CGCharacterModelBase::GetScriptMetaTable() {
    return CGCharacterModelBase::s_metatable;
}

// VERIFY: 8f4f2545 auto
// OFFSET: 0x005971B0
void CGCharacterModelBase::UpdateModel() {
    CSimpleModel::UpdateModel();

    if (!this->m_sequenceSet && OsGetAsyncTimeMs() > this->m_sequenceResetTime && this->m_model->GetBoneSequenceId(-1)) {
        this->m_model->SetBoneSequence(-1, 0, -1, 0, 1.0f, 1, 1);
        this->m_sequenceResetTime = 0;
    }

    this->m_sequenceSet = 0;
}

// VERIFY: 156c6525 auto
// OFFSET: 0x00597210
int32_t CGCharacterModelBase::HideThis() {
    this->SetModel(static_cast<CM2Model*>(nullptr));
    return CSimpleFrame::HideThis();
}

// VERIFY: 301b377c auto
// OFFSET: 0x005972D0
bool CGCharacterModelBase::IsA(int32_t type) {
    return type == CGCharacterModelBase::s_objectType || type == CSimpleModel::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 97fbce4f auto
// OFFSET: 0x00597310
bool CGCharacterModelBase::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CGCharacterModelBase::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CSimpleModel::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: db00ef5c auto
// OFFSET: 0x00597370
CGCharacterModelBase::CGCharacterModelBase(CSimpleFrame* parent) : CSimpleModel(parent) {
    this->m_unitGuid = WOWGUID();
    this->m_sequenceSet = 0;
    this->m_sequenceResetTime = 0;
    this->m_creatureStats = nullptr;
    this->m_light.SetLightType(M2LIGHT_DIRECT);
    this->m_light.SetDirection(C3Vector(0.0f, -0.70710599f, -0.70710599f));
    this->m_light.m_dirColor = C3Vector(0.8f, 0.8f, 0.64f);
    this->m_light.m_ambColor = C3Vector(0.7f, 0.7f, 0.7f);
    this->m_light.SetDirection(C3Vector(0.0f, 1.0f, 0.0f));
    this->m_light.SetVisible(1);
}

// VERIFY: 282b399d manual
// OFFSET: 0x005975B0
CM2Model* CGCharacterModelBase::GetDuplicateUnitCharacterModel(CGUnit_C* unit) {
    if (!unit) {
        return nullptr;
    }

    return this->GetScene()->DuplicateModel(unit->m_worldModel, 0);
}

// VERIFY: 4219f365 auto
// OFFSET: 0x005977C0
void CGCharacterModelBase::OnModelLoaded(CM2Model* model) {
    CSimpleModel::OnModelLoaded(model);
    this->ConfigureCamera();
}

// VERIFY: ab38d930 manual
// OFFSET: 0x00597840
void CGCharacterModelBase::SetCreature(CreatureStats_C* stats) {
    this->m_unitGuid = WOWGUID();
    this->m_creatureStats = stats;

    if (this->m_isVisible && stats) {
        this->SetUpModel(stats);
    }
}

// VERIFY: c0c76e1e manual
// OFFSET: 0x00597870
int32_t CGCharacterModelBase::ShowThis() {
    if (!this->m_isVisible) {
        if (this->m_unitGuid != WOWGUID()) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_unitGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                this->SetUpModel(unit);
                return CSimpleFrame::ShowThis();
            }
        } else if (this->m_creatureStats) {
            this->SetUpModel(this->m_creatureStats);
        }
    }

    return CSimpleFrame::ShowThis();
}

// VERIFY: 07392823 manual
// OFFSET: 0x00597AA0
int32_t CGCharacterModelBase::IsDrawable() {
    if (!this->m_model || !this->m_model->IsDrawable(0, 1)) {
        return 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_unitGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && !IsUnitDrawable(unit)) {
        return 0;
    }

    return 1;
}

// VERIFY: deda935b auto
// OFFSET: 0x005970F0
void CGCharacterModelBase::RotateModel(float facing) {
    if (this->m_model && this->m_model->IsLoaded(0, 0)) {
        uint32_t sequence = 0;

        if (facing > this->m_facing) {
            sequence = 12;
        } else if (facing < this->m_facing) {
            sequence = 11;
        }

        if (this->m_model->HasSequence(sequence) && sequence != this->m_model->GetBoneSequenceId(-1)) {
            this->m_model->SetBoneSequence(-1, sequence, -1, 0, 1.0f, 1, 1);
        }
    }

    this->m_sequenceSet = 1;
    this->m_sequenceResetTime = OsGetAsyncTimeMs() + 100;
    this->m_facing = facing;
}

// VERIFY: 7b36912a manual
// OFFSET: 0x005977E0
void CGCharacterModelBase::RefreshUnit(WOWGUID guid) {
    this->m_unitGuid = guid;
    this->m_creatureStats = nullptr;

    if (this->m_isVisible) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            this->SetUpModel(unit);
        }
    }
}

// VERIFY: 838456da manual
// OFFSET: 0x005975E0
void CGCharacterModelBase::SetUpModel(CGUnit_C* unit) {
    if (!ReadyModelForUI(unit, 3)) {
        return;
    }

    CM2Model* model = this->GetDuplicateUnitCharacterModel(unit);

    if (!model) {
        return;
    }

    if (!(model->m_fFlags & 0x1)) {
        model->WaitForLoad(nullptr);
    }

    CCharacterComponent::RemoveVisuals(model);
    this->SetModel(model);
    this->SetFrameAlpha(static_cast<uint8_t>(unit->GetBaseAlpha() * 255.0f));
    model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);

    for (CM2Model* attached = model->m_attachList; attached; attached = attached->m_attachNext) {
        if (attached->m_attachmentId == 2) {
            CCharacterComponent::ComponentCloseFingers(model, COMP_HAND_LEFT);
            break;
        }
    }

    for (CM2Model* attached = model->m_attachList; attached; attached = attached->m_attachNext) {
        if (attached->m_attachmentId == 1) {
            CCharacterComponent::ComponentCloseFingers(model, COMP_HAND_RIGHT);
            break;
        }
    }

    model->Release();
}

// VERIFY: 5476abab auto
// OFFSET: 0x00597700
void CGCharacterModelBase::SetUpModel(CreatureStats_C* stats) {
    if (!stats) {
        return;
    }

    CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(stats->m_displayId[0]);

    if (!displayRec) {
        return;
    }

    CreatureModelDataRec* modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

    if (!modelRec) {
        return;
    }

    CM2Model* model = this->GetScene()->CreateModel(modelRec->m_modelName, 0);

    if (!model) {
        return;
    }

    CCharacterComponent::ReplaceMonsterSkin(model, displayRec, modelRec);
    this->SetModel(model);
    model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    model->Release();
}

// VERIFY: 24702114 auto
// OFFSET: 0x005974A0
void CGCharacterModelBase::ConfigureCamera() {
    this->SetCameraByID(CAMERA_PAPERDOLL);
    CCamera* camera;

    if (this->m_camera) {
        float facing = this->m_facing;
        this->m_facing = 0.0f;
        this->UpdateModel();
        this->m_model->SetAnimating(1);
        this->GetScene()->Animate(C3Vector(0.0f, 0.0f, 0.0f));
        this->m_facing = facing;
        camera = CameraDuplicate(this->m_camera);
    } else {
        CAaSphere sphere = this->m_model->GetBoundingSphere();
        camera = CameraCreate();
        CameraSetFloat(camera, 4, 0.5f);
        CameraSetCoord(camera, 8, sphere.c, 0);
        CameraSetCoord(camera, 7, C3Vector(5.5555558f, 0.0f, 2.4166667f), 0);
        CameraSetFloat(camera, 3, 0.027777778f);
    }

    this->SetCamera(camera);
    HandleClose(reinterpret_cast<HOBJECT>(camera));
}
