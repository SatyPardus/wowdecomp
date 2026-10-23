#include "Object_C.h"
#include "WoW/Source/UIUtil/Types.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/ObjectEffect.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "Engine/Source/Services/SysMessage.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include <cmath>
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Magic/MagicClient/SpellVisuals.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/MovementShared.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitModelAttachRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00ADAC98
CGObject_C::InteractIcon CGObject_C::s_interactIcons[12] = {
    { nullptr, 1.0f, nullptr },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMe.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeQuestion_LTBlue.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeBlue.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMe.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeQuestion_Grey.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeGrey.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeGreen.mdx" },
    { nullptr, 1.0f, nullptr },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeQuestionMark.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeQuestion_LTBlue.mdx" },
    { nullptr, 1.0f, "Interface\\Buttons\\TalkToMeBlue.mdx" },
};

// STATIC: 0x00A34F5C
const uint32_t CGObject_C::s_questStatusIcons[11] = { 0, 6, 1, 2, 3, 5, 10, 11, 4, 9, 9 };

// STATIC: 0x00CA12C4
CTexture* CGObject_C::s_selectionTexture;

// VERIFY: fe38de3f manual
// OFFSET: 0x007435E0
static void LoadInteractIcons(int32_t load) {
    for (uint32_t i = 0; i < sizeof(CGObject_C::s_interactIcons) / sizeof(CGObject_C::s_interactIcons[0]); i++) {
        CGObject_C::InteractIcon& icon = CGObject_C::s_interactIcons[i];

        if (icon.m_model) {
            icon.m_model->Release();
            icon.m_model = nullptr;
        }

        if (!load) {
            icon.m_model = nullptr;
        } else if (icon.m_fileName && *icon.m_fileName) {
            icon.m_model = World::s_m2Scene->CreateModel(icon.m_fileName, 0);
        }
    }
}

// VERIFY: 22da9b4a manual
// OFFSET: 0x007440B0
static int32_t UpdateAllWorldObjectsCallback(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        object->UpdateWorldObject(0);
    }

    return 1;
}

// VERIFY: 8897bbb7 manual
// OFFSET: 0x00744100
static int32_t UpdateAllObjectEffectManagersCallback(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && object->m_effectManager) {
        object->m_effectManager->UpdateAllPlayingEffects();
    }

    return 1;
}

// VERIFY: 75bf5d26 manual
// OFFSET: 0x00744E10
static int32_t ScaleUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        object->HandleScaleUpdate(*static_cast<const float*>(oldValue));
    }

    return 1;
}

// VERIFY: 76f8c467 auto
// OFFSET: 0x007442E0
void CGObject_C::ModelLoaded(CM2Model* model) {
    if (model != this->GetObjectModel()) {
        return;
    }

    this->ModelChanged();

    CEffect* effect = this->m_effectList;

    while (effect) {
        CEffect* next = effect->m_next;
        effect->UpdateAttachment();
        effect = next;
    }
}

// VERIFY: f03ef6af manual
// OFFSET: 0x00743760
void CGObject_C::AddWorldObject() {
    if (!this->m_worldModel) {
        const char* fileName;

        if (this->GetModelFileName(&fileName)) {
            CM2Model* model = World::s_m2Scene->CreateModel(fileName, 0);

            if (model != this->m_worldModel) {
                this->SetModelFinish(this->SetModelStart(model));
            }

            model->Release();
        }

        if (!this->m_worldModel) {
            return;
        }
    }

    if (ClntObjMgrGetPlayerType()) {
        return;
    }

    if (this->m_worldObject) {
        SysMsgPrintf(SYSMSG_WARNING, 2, "OBJECTALREADYACTIVE|0x%016I64X", this->m_obj->m_guid.m_guid);
        return;
    }

    uint32_t flags = 0;

    if (this->m_obj->m_type & TYPE_GAMEOBJECT) {
        flags = 11;
    } else if (this->m_obj->m_type & TYPE_DYNAMICOBJECT) {
        flags = 10;
    } else if (this->m_obj->m_type & TYPE_CORPSE) {
        if (static_cast<CGCorpse_C*>(this)->m_corpse->m_corpseFieldFlags & 0x1) {
            flags = 2;
        }
    } else if (this->m_obj->m_type & TYPE_UNIT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(this);

        if (unit->IsLinkAll()) {
            flags = 8;
        }

        if (unit->HasNoShadowBlob()) {
            flags |= 0x2;
        }

        flags |= 0x10;

        if (this->m_obj->m_type & TYPE_PLAYER) {
            flags |= 0x20;
        }
    }

    this->m_worldObject = CMap::ObjectCreate(this->GetObjectModel(), CGWorldFrame::ObjectEnumProc, nullptr, this->m_obj->m_guid.m_guid, 0, flags);

    if ((this->m_modelFlags & 0x40000) && !(this->m_modelFlags & 0x20000)) {
        this->UpdateWorldObject(0);
    }
}

// VERIFY: 8e991786 manual
// OFFSET: 0x00745DA0
void CGObject_C::UpdateInteractIcon() {
    if (this->m_interactIcon) {
        if (this->m_interactIcon->m_attachParent) {
            this->m_interactIcon->DetachFromParent();
        }

        this->m_interactIcon->Release();
        this->m_interactIcon = nullptr;
    }

    uint32_t icon;
    uint32_t status = this->m_questStatus;

    if (status && (status - 2 > 2 || CGMinimapFrame::IsTrackingTrivialQuests())) {
        icon = CGObject_C::s_questStatusIcons[this->m_questStatus];
    } else {
        icon = this->m_interactIconType;

        if (!icon) {
            return;
        }
    }

    if (!icon) {
        return;
    }

    CM2Model* model = World::s_m2Scene->DuplicateModel(CGObject_C::s_interactIcons[icon].m_model, 0);
    this->m_interactIcon = model;

    if (model) {
        model->m_fadeAlpha = CGObject_C::s_interactIcons[icon].m_scale;
    }

    this->UpdateInteractIconAttach();
    this->SetInteractIconVisible(CGGameUI::m_visible);
}

// VERIFY: 1e9b4846 manual
// OFFSET: 0x007444E0
void CGObject_C::UpdateInteractIconScale() {
    if (!this->m_interactIcon || !this->m_interactIcon->m_attachParent || !this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    C44Matrix attachment = this->m_worldModel->GetAttachmentWorldTransform(this->m_interactIcon->m_attachmentId);
    float lengthSq = attachment.a1 * attachment.a1 + attachment.a2 * attachment.a2 + attachment.a0 * attachment.a0;

    if (!(0.0f < lengthSq)) {
        return;
    }

    float scale = PlayerNameComputeScale(this) / sqrtf(lengthSq);

    if (fequal(scale, 1.0f) || !(0.0f < scale)) {
        return;
    }

    C44Matrix matrix;
    matrix = this->m_interactIcon->m_worldTransform;
    float current = sqrtf(matrix.a1 * matrix.a1 + matrix.a2 * matrix.a2 + matrix.a0 * matrix.a0);

    if (0.01f < current && fabsf(scale - current) > 0.01f) {
        matrix.Scale(scale / current);
        this->m_interactIcon->SetWorldTransform(matrix);
    }
}

// VERIFY: c70eec03 auto
// OFFSET: 0x00743490
void CGObject_C::ObjectNameVisibilityChanged() {
    if (!this->m_interactIcon) {
        return;
    }

    if (PlayerNameShowing(this->m_nameDesc)) {
        this->m_interactIcon->SetBoneSequence(0xFFFFFFFF, 190, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        this->m_interactIcon->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 5c131dd9 auto
// OFFSET: 0x00743FF0
void CGObject_C::PostReenable() {
    this->RefreshInteractIcon();
    this->m_modelFlags &= ~0x20000;

    if (!this->m_worldObject && this->GetObjectModel()) {
        this->AddWorldObject();
    }
}

// VERIFY: c5f08860 auto
// OFFSET: 0x00744DB0
void CGObject_C::Reenable() {
    this->m_modelFlags = (this->m_modelFlags & ~0x10000) | 0x20000;
    this->m_scale = this->GetScale();
    this->m_scaleChangeTime = 0;
    uint32_t time = this->ShouldFadeIn() ? 1000 : 0;
    this->DoFade(this->GetBaseAlpha(), time);
}

// VERIFY: 9f30bf9a auto
// OFFSET: 0x00744D20
void CGObject_C::Disable() {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera && camera->m_relativeTo == this->m_obj->m_guid) {
        camera->MakeRelativeTo(WOWGUID{ 0 });
    }

    this->Fadeout();

    if (this->m_interactIcon) {
        if (this->m_interactIcon->m_attachParent) {
            this->m_interactIcon->DetachFromParent();
        }

        this->m_interactIcon->Release();
        this->m_interactIcon = nullptr;
    }

    this->m_fadeDuration = 0;
    this->m_fadeEndAlpha = 0;
    this->m_alpha = 0;
    this->m_modelFlags = (this->m_modelFlags & 0xF8FFFFFF) | 0x10000;
    this->m_disableTime = FrameTime::s_curTimeMs;
}

// VERIFY: 890fb3ed auto
// OFFSET: 0x00743D50
void CGObject_C::Fadeout() {
    if (!this->m_worldObject) {
        return;
    }

    if (this->ShouldFadeout()) {
        if (this->GetObjectModel() && !this->GetObjectModel()->m_attachParent) {
            this->Animate(0.0f);
        }

        CGObject_C::PrepareToFadeInt(this->m_worldObject, this->m_alpha * 0.0039215689f, this->GetTransportGUID());
        this->m_worldObject = nullptr;
    } else {
        World::ObjectDestroy(this->m_worldObject);
        this->m_worldObject = nullptr;
    }
}

// VERIFY: fce76166 auto
// OFFSET: 0x00743AF0
void CGObject_C::UpdateEffectAttachments() {
    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        CEffect* effect = this->m_effectList;

        while (effect) {
            CEffect* next = effect->m_next;

            if (effect->m_flags & 0x800) {
                effect->UpdateAttachment();
            }

            effect = next;
        }
    }
}

// VERIFY: 8c88711b auto
// OFFSET: 0x00743EC0
void CGObject_C::PreAnimate(CGWorldFrame* frame) {
    if (CGGameUI::m_lockedTarget == this->m_obj->m_guid && ClntObjMgrGetActivePlayer() != this->m_obj->m_guid && CGObject_C::s_cvObjectSelectionCircle->m_intValue) {
        frame->m_trackedEffectGuidA = this->m_obj->m_guid;
    }

    if (CGPetInfo::GetTarget() == this->m_obj->m_guid && ClntObjMgrGetActivePlayer() != this->m_obj->m_guid) {
        frame->m_trackedEffectGuidB = this->m_obj->m_guid;
    }

    uint32_t time = FrameTime::s_curTimeMs;
    this->ApplyAlpha(time);

    if (this->m_scaleChangeTime && time > this->m_scaleChangeTime) {
        int32_t elapsed = time - this->m_scaleChangeTime;

        if (elapsed < 2000) {
            this->m_scale = (this->GetScale() - this->m_scaleChangeStart) * (-cosf(elapsed * 0.0005f * 3.1415927f) * 0.5f + 0.5f) + this->m_scaleChangeStart;
            this->ScaleChangeUpdate();
        } else {
            this->m_scale = this->m_obj->m_scale;
            this->m_scaleChangeTime = 0;
            this->ScaleChangeFinished();
        }
    }
}

// VERIFY: e068f442 manual
// OFFSET: 0x00743680
void CGObject_C::SetModelFinish(CM2Model* model) {
    CEffect* effect = this->m_effectList;

    while (effect) {
        CEffect* next = effect->m_next;
        effect->DetachFromParent();

        if (this->m_worldModel->IsLoaded(0, 0)) {
            effect->UpdateAttachment();
        }

        effect = next;
    }

    if (model) {
        model->SetLoadedCallback(nullptr, nullptr);

        if (model->m_attachParent) {
            model->DetachFromParent();
        }

        model->Release();
    }

    if (this->m_worldModel) {
        this->m_worldModel->SetLoadedCallback(CGObject_C::ModelLoadedCallback, this);
    }

    if (this->m_worldObject) {
        CM2Model* worldModel = this->m_worldModel;

        if (worldModel == this->GetObjectModel()) {
            World::ObjectSetModel(this->m_worldObject, worldModel);
        }
    }
}

// VERIFY: fcd6a1dd manual
// OFFSET: 0x007438E0
void CGObject_C::UpdateWorldObject(int32_t a2) {
    if (!this->m_worldObject) {
        return;
    }

    C44Matrix mat;
    mat.Translate(this->GetPosition());
    mat.RotateAroundZ(this->GetFacing());
    mat.Scale(this->GetTrueScale());

    C3Vector vec(0.0f, 0.0f, 0.0f);
    CAaBox box(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f));
    CAaSphere sphere;
    sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    sphere.r = 0.0f;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        CM2Model* model = this->m_worldModel;

        if (!(model->m_shared->m_flags & 0x1)) {
            model->WaitForLoad(nullptr);
        }

        CAaBox bounds = model->m_shared->m_data->m_collisionBounds.m_extent;
        vec = C3Vector((bounds.t.x + bounds.b.x) * 0.5f, (bounds.t.y + bounds.b.y) * 0.5f, (bounds.t.z + bounds.b.z) * 0.5f);
        box = this->m_worldModel->GetBoundingBox();
        sphere = this->m_worldModel->GetBoundingSphere();
    }

    CMap::ObjectUpdate(this->m_worldObject, mat, box, sphere, vec, a2, 0xFFFFFFFF);
}

// VERIFY: 6f30b42a auto
// OFFSET: 0x00743110
void CGObject_C::ModelLoadedCallback(CM2Model* model, void* arg) {
    CGObject_C* object = static_cast<CGObject_C*>(arg);

    if (object) {
        object->ModelLoaded(model);
    }
}

// VERIFY: 81843143 auto
// OFFSET: 0x007435A0
void CGObject_C::LoopEffectHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));

    if (!interrupted && effect && effect->m_model) {
        effect->m_model->SetBoneSequence(0xFFFFFFFF, sequenceId, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: d408d6ed manual
// OFFSET: 0x004D5E00
CGObject_C::CGObject_C() {
}

// VERIFY: 7adf9979 auto
// OFFSET: 0x00632050
void CGObject_C::HandleOutOfRange(OUT_OF_RANGE_TYPE type) {
}

// VERIFY: 25fd42ac auto
// OFFSET: 0x008C8DE0
int32_t CGObject_C::ShouldFadeout() {
    return 1;
}

// VERIFY: b5a67297 auto
// OFFSET: 0x00632050
void CGObject_C::UpdateDisplayInfo(int32_t force) {
}

// VERIFY: 6b122e4b auto
// OFFSET: 0x007451B0
C3Vector CGObject_C::GetNamePosition() const {
    if (this->m_worldModel->HasAttachment(18)) {
        return this->m_worldModel->GetAttachmentPosition(18);
    }

    C3Vector position = this->GetPosition();
    position.z += this->m_height * this->m_scale * 1.25f;
    return position;
}

// VERIFY: 02fb2286 auto
// OFFSET: 0x008E5250
const CGBag_C* CGObject_C::GetBag() const {
    return nullptr;
}

// VERIFY: f85b6c68 auto
// OFFSET: 0x008E5250
CGBag_C* CGObject_C::GetBag() {
    return nullptr;
}

// VERIFY: 0dee944e auto
// OFFSET: 0x004D5EA0
C3Vector CGObject_C::GetPosition() const {
    return C3Vector(0.0f, 0.0f, 0.0f);
}

// VERIFY: e69e03cb auto
// OFFSET: 0x004D5EC0
C3Vector CGObject_C::GetRawPosition() const {
    return this->GetPosition();
}

// VERIFY: f1112526 auto
// OFFSET: 0x004D5EE0
float CGObject_C::GetFacing() const {
    return 0.0f;
}

// VERIFY: 7541f436 auto
// OFFSET: 0x004D5EF0
float CGObject_C::GetRawFacing() const {
    return this->GetFacing();
}

// VERIFY: 363225f4 auto
// OFFSET: 0x004D5F00
float CGObject_C::GetScale() const {
    return this->m_obj->m_scale;
}

// VERIFY: d520d802 auto
// OFFSET: 0x004D5F10
uint64_t CGObject_C::GetTransportGUID() const {
    return 0;
}

// VERIFY: 3905a697 auto
// OFFSET: 0x004D5F20
C4Quaternion CGObject_C::GetRotation() const {
    C4Quaternion rotation;
    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;
    rotation.w = 1.0f;
    rotation.FromAngleAxis(this->GetFacing(), C3Vector(0.0f, 0.0f, 1.0f));
    return rotation;
}

// VERIFY: d685a06a auto
// OFFSET: 0x00744380
void CGObject_C::SetFrameOfReference(const C44Matrix* matrix) {
    CM2Model* model = this->m_worldModel;

    if (!model || !(model->m_fFlags & 0x1)) {
        return;
    }

    if (matrix) {
        model->m_frameOfReferenceMatrix = *matrix;
        model->ChangeFrameOfReference(&model->m_frameOfReferenceMatrix);
    } else {
        model->ChangeFrameOfReference(nullptr);
    }
}

// VERIFY: b70df993 auto
// OFFSET: 0x008E5250
int32_t CGObject_C::IsQuestGiver() const {
    return 0;
}

// VERIFY: 5fa3d028 auto
// OFFSET: 0x005EEB70
void CGObject_C::RefreshInteractIcon() {
}

// VERIFY: 362a375d manual
// OFFSET: 0x00744460
void CGObject_C::UpdateInteractIconAttach() {
    if (!this->m_interactIcon || !this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    if (this->m_interactIcon->m_attachParent) {
        this->m_interactIcon->DetachFromParent();
    }

    if (this->m_worldModel->HasAttachment(18)) {
        this->m_interactIcon->AttachToParent(this->m_worldModel, 18, nullptr, 0);
        this->UpdateInteractIconScale();
        this->ObjectNameVisibilityChanged();
    }
}

// VERIFY: 5d394d8e auto
// OFFSET: 0x004899F0
int32_t CGObject_C::GetModelFileName(const char** name) const {
    return 0;
}

// VERIFY: a13063c0 auto
// OFFSET: 0x005EEB70
void CGObject_C::ScaleChangeUpdate() {
}

// VERIFY: 93da7744 auto
// OFFSET: 0x005EEB70
void CGObject_C::ScaleChangeFinished() {
}

// VERIFY: 67768697 auto
// OFFSET: 0x005EEB70
void CGObject_C::RenderTargetSelection() const {
}

// VERIFY: adf0f9a2 auto
// OFFSET: 0x005EEB70
void CGObject_C::RenderPetTargetSelection() const {
}

// VERIFY: 2932844c auto
// OFFSET: 0x005EEB70
void CGObject_C::Render() {
}

// VERIFY: edc3c913 auto
// OFFSET: 0x004D5F70
int32_t CGObject_C::GetSelectionHighlightColor(CImVector* color) const {
    *reinterpret_cast<uint32_t*>(color) = 0xFFFFFFFF;
    return 1;
}

// VERIFY: 986e4ed9 auto
// OFFSET: 0x004D5F90
float CGObject_C::GetTrueScale() const {
    return this->m_scaleModifier * this->m_scale;
}

// VERIFY: eb640da9 manual
// OFFSET: 0x00743E10
void CGObject_C::ApplyAlpha(uint32_t time) {
    CM2Model* model = this->GetObjectModel();

    if (!model) {
        return;
    }

    if (this->m_fadeDuration) {
        if (static_cast<int32_t>(time - this->m_fadeStartTime - this->m_fadeDuration) < 0) {
            this->m_alpha = this->m_fadeStartAlpha + static_cast<int32_t>((this->m_fadeEndAlpha - this->m_fadeStartAlpha) * (time - this->m_fadeStartTime)) / static_cast<int32_t>(this->m_fadeDuration);
        } else {
            this->m_fadeDuration = 0;
            this->m_alpha = this->m_fadeEndAlpha;
        }
    }

    model->m_fadeAlpha = static_cast<float>(this->m_alphaScale) * this->m_alpha * (1.0f / 65025.0f);
}

// VERIFY: bf65a937 auto
// OFFSET: 0x00743330
int32_t CGObject_C::Animate(float elapsed) {
    CM2Model* model = this->GetObjectModel();

    if (model) {
        float scale = this->GetTrueScale();
        float facing = this->GetRenderFacing();
        model->SetWorldTransform(this->GetPosition(), facing, scale);
    }

    return 1;
}

// VERIFY: 4939e645 auto
// OFFSET: 0x00743300
void CGObject_C::ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden) {
    if (!(flags & 0x1)) {
        render = 1;
    }
}

// VERIFY: 490db0ab auto
// OFFSET: 0x004D5EF0
float CGObject_C::GetRenderFacing() const {
    return this->GetFacing();
}

// VERIFY: 5c1e2d9f auto
// OFFSET: 0x005EEB70
void CGObject_C::OnSpecialMountAnim() {
}

// VERIFY: f815c558 auto
// OFFSET: 0x008C8DE0
int32_t CGObject_C::IsSolidSelectable() const {
    return 1;
}

// VERIFY: 76524fa5 auto
// OFFSET: 0x00959DE0
int32_t CGObject_C::IsCollidable(int32_t flags) const {
    return 1;
}

// VERIFY: b4c0da8d auto
// OFFSET: 0x008E5250
int32_t CGObject_C::CanHighlight() const {
    return 0;
}

// VERIFY: df7e2e8f auto
// OFFSET: 0x008E5250
int32_t CGObject_C::CanBeTargetted() const {
    return 0;
}

// VERIFY: 23761254 auto
// OFFSET: 0x008E5250
int32_t CGObject_C::FloatingTooltip() const {
    return 0;
}

// VERIFY: 6d451c6e auto
// OFFSET: 0x005EEB70
void CGObject_C::OnRightClick() {
}

// VERIFY: 896f70ae auto
// OFFSET: 0x008E5250
int32_t CGObject_C::IsHighlightSuppressed() const {
    return 0;
}

// VERIFY: f51330d1 manual
// OFFSET: 0x004D5FA0
C44Matrix CGObject_C::GetMatrix() const {
    return C44Matrix();
}

// VERIFY: cae05adc auto
// OFFSET: 0x007434E0
int32_t CGObject_C::UpdateObjectNameString(int32_t flags, char* buffer, uint32_t size) {
    if (this->GetObjectName()) {
        SStrPrintf(buffer, size, "%s", this->GetObjectName());
    } else {
        *buffer = '\0';
    }

    return 1;
}

// VERIFY: 9000471f auto
// OFFSET: 0x00743530
int32_t CGObject_C::ShouldRenderObjectName(uint32_t flags) const {
    if (this->m_interactIcon) {
        return 0;
    }

    return (flags >> 11) & 0x1;
}

// VERIFY: 0bd02555 auto
// OFFSET: 0x004D5FE0
CM2Model* CGObject_C::GetObjectModel() const {
    return this->m_worldModel;
}

// VERIFY: 6155de17 auto
// OFFSET: 0x008E5250
const char* CGObject_C::GetObjectName() const {
    return nullptr;
}

// VERIFY: 2578a536 auto
// OFFSET: 0x004899F0
int32_t CGObject_C::GetPageTextID(void (*callback)(int32_t, const WOWGUID&, void*, bool)) const {
    return 0;
}

// VERIFY: 98c12193 auto
// OFFSET: 0x005EEB70
void CGObject_C::CleanUpVehicleBoneAnimsBeforeObjectModelChange() {
}

// VERIFY: daa8fb99 auto
// OFFSET: 0x008C8DE0
int32_t CGObject_C::ShouldFadeIn() const {
    return 1;
}

// VERIFY: 671f9670 auto
// OFFSET: 0x004D5FF0
float CGObject_C::GetBaseAlpha() const {
    return 1.0f;
}

// VERIFY: 4236057f auto
// OFFSET: 0x00427A90
bool CGObject_C::IsTransport() const {
    return false;
}

// VERIFY: f7102178 auto
// OFFSET: 0x004899F0
int32_t CGObject_C::IsPointInside(const C3Vector& point) const {
    return 0;
}

// VERIFY: 49a82b24 auto
// OFFSET: 0x004D6000
int32_t CGObject_C::AddPassenger(CPassenger* passenger, int32_t a2) {
    return 1;
}

// VERIFY: 3d0df1fc auto
// OFFSET: 0x006F1700
float CGObject_C::GetSpeed() const {
    return -1.0f;
}

// VERIFY: fb1bf54a auto
// OFFSET: 0x00743550
void CGObject_C::PlaySpellVisualKit_PlayAnims(const SpellRec* spell, const SpellVisualKitRec* kit, SPELL_VISUAL_KIT_TYPE type, const C3Vector* position, int32_t a5, int32_t a6, WOWGUID guid, uint32_t a8, const SpellVisualRec* visual, uint32_t a10, int32_t a11) {
}

// VERIFY: 6ea82269 auto
// OFFSET: 0x00743560
void CGObject_C::PlaySpellVisualKit_HandleWeapons(const SpellRec* spell, const SpellVisualKitRec* kit, const C3Vector* position, WOWGUID guid, uint32_t a5, uint32_t flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), int32_t* hasEffect) {
}

// VERIFY: acfa0e6e auto
// OFFSET: 0x00743570
int32_t CGObject_C::PlaySpellVisualKit_DelayLightningEffects(const SpellRec* spell, const SpellVisualKitRec* kit, SPELL_VISUAL_KIT_TYPE type, const C3Vector* position, WOWGUID guid, int32_t a6, int32_t a7, int32_t a8, int32_t a9, uint32_t a10, const SpellVisualRec* visual) {
    return 0;
}

// VERIFY: c530e49a auto
// OFFSET: 0x00743B40
void CGObject_C::ClearSpellEffect(uint32_t spellId, int32_t all) {
    if (!spellId) {
        return;
    }

    for (CEffect* effect = this->m_effectList; effect;) {
        CEffect* next = effect->m_next;

        if (effect->m_spellId == spellId && (all || !(effect->m_flags & 0x20000)) && !(effect->m_flags & 0x20)) {
            this->OnSpellEffectClear(effect);
        }

        effect = next;
    }
}

// VERIFY: d4b02396 auto
// OFFSET: 0x00744030
void CGObject_C::DoFade(float alpha, uint32_t time) {
    uint8_t target = static_cast<int32_t>(alpha * 255.0f);
    uint8_t current = this->m_alpha;

    if (target == current) {
        this->m_fadeEndAlpha = target;
        this->m_fadeDuration = 0;
        this->m_alpha = target;
        return;
    }

    this->m_fadeStartTime = FrameTime::s_curTimeMs;
    this->m_fadeDuration = time;
    this->m_fadeStartAlpha = current;
    this->m_fadeEndAlpha = target;

    if (!time) {
        this->m_alpha = target;
    }
}

// VERIFY: b7241e5c auto
// OFFSET: 0x00744720
C44Matrix CGObject_C::GetModelMatrix() {
    CM2Model* model = this->GetObjectModel();

    if (model && (model->m_fFlags & 0x8000)) {
        model->Animate();
        return model->m_matrixF4 * model->m_scene->m_viewInv;
    }

    return this->GetMatrix();
}

// VERIFY: 08a31e06 auto
// OFFSET: 0x007446C0
C3Vector CGObject_C::GetModelPosition() {
    CM2Model* model = this->GetObjectModel();

    if (model && (model->m_fFlags & 0x8000)) {
        model->Animate();
        return *reinterpret_cast<const C3Vector*>(&model->m_matrixF4.d0) * model->m_scene->m_viewInv;
    }

    return this->GetPosition();
}

// VERIFY: 44a0be86 manual
// OFFSET: 0x00744640
int32_t CGObject_C::GetQuestGiverCursor(int32_t highlighted) {
    switch (this->m_questStatus) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 7:
        case 8:
            return highlighted ? QUEST_ERROR_CURSOR : QUEST_CURSOR;

        case 6:
            return highlighted ? QUEST_REPEATABLE_ERROR_CURSOR : QUEST_REPEATABLE_CURSOR;

        case 9:
        case 10:
            return highlighted ? QUEST_TURNIN_ERROR_CURSOR : QUEST_TURNIN_CURSOR;

        default:
            return NO_CURSOR;
    }
}

// VERIFY: 02ef1834 auto
// OFFSET: 0x00743BC0
void CGObject_C::HideHighlightType(HIGHLIGHTTYPE type) {
    this->m_modelFlags &= ~(1 << (type + 24));

    if (s_highlightedObjectGuid == this->m_obj->m_guid) {
        s_highlightedObjectGuid = WOWGUID();
    }

    if (this->IsHighlightSuppressed()) {
        return;
    }

    CM2Model* model = this->GetObjectModel();

    if (model && !(this->m_modelFlags & 0x7000000)) {
        model->m_emissiveTintR = 0.0f;
        model->m_emissiveTintG = 0.0f;
        model->m_emissiveTintB = 0.0f;
        model->unk_01B8 = 1.0f;
    }
}

// VERIFY: 9da10cdf manual
// OFFSET: 0x007433D0
int32_t CGObject_C::IsObjectLocked() {
    return (this->m_modelFlags & 0xFFFF) != 0;
}

// VERIFY: b07b7284 auto
// OFFSET: 0x00743390
int32_t CGObject_C::IsObjectModelLoaded() {
    if (this->GetObjectModel() && this->GetObjectModel()->IsLoaded(0, 0)) {
        return 1;
    }

    return 0;
}

// VERIFY: 72e63e2b auto
// OFFSET: 0x00743450
int32_t CGObject_C::IsReadyToDraw() {
    if (this->GetObjectModel() && this->GetObjectModel()->IsDrawable(0, 0)) {
        return 1;
    }

    return 0;
}

// VERIFY: 35ab56d8 manual
// OFFSET: 0x00744230
void CGObject_C::ModelChanged() {
    uint32_t loaded = this->GetObjectModel()->IsLoaded(0, 1);
    CAaBox box = this->GetObjectModel()->GetBoundingBox();

    if (box.t.x > box.b.x && box.t.y > box.b.y && box.t.z > box.b.z) {
        this->m_height = box.t.z - box.b.z;
    } else {
        this->m_height = 0.0f;
    }

    if (!(this->m_modelFlags & 0x10000)) {
        this->UpdateWorldObject(0);
    }

    if (loaded) {
        this->m_modelFlags &= ~0x200000;
    } else {
        this->m_modelFlags |= 0x200000;
    }
}

// VERIFY: d7a17e0d auto
// OFFSET: 0x007431E0
void CGObject_C::ReleaseModel() {
    if (!this->m_worldModel) {
        return;
    }

    this->m_worldModel->SetLoadedCallback(nullptr, nullptr);
    this->m_worldModel->SetSequenceCallback(nullptr, 0);
    this->m_worldModel->SetEventCallback(nullptr, 0);
    this->m_worldModel->Release();
    this->m_worldModel = nullptr;
}

// VERIFY: 2d1503ab auto
// OFFSET: 0x007443D0
void CGObject_C::RemoveInteractIcon() {
    if (!this->m_interactIcon) {
        return;
    }

    if (this->m_interactIcon->m_attachParent) {
        this->m_interactIcon->DetachFromParent();
    }

    this->m_interactIcon->Release();
    this->m_interactIcon = nullptr;
}

// VERIFY: a2fd3997 auto
// OFFSET: 0x00743BA0
void CGObject_C::SetBlock(int32_t index, uint32_t value) {
    reinterpret_cast<uint32_t*>(this->m_obj)[index] = value;
}

// VERIFY: 9a9e12cb auto
// OFFSET: 0x00743320
void CGObject_C::SetBoundsDirty() {
    this->m_modelFlags |= 0x200000;
}

// VERIFY: a177f7a1 auto
// OFFSET: 0x00743420
void CGObject_C::SetDisablePending(int32_t pending) {
    if (pending) {
        this->m_modelFlags |= 0x100000;
    } else {
        this->m_modelFlags &= ~0x100000;
    }
}

// VERIFY: 63d58122 manual
// OFFSET: 0x00745140
void CGObject_C::SetInteractIconVisible(int32_t visible) {
    CM2Model* icon = this->m_interactIcon;

    if (!icon || !icon->m_attachParent) {
        return;
    }

    uint32_t flag = visible & 0x1;
    CM2Model* parent = icon->m_attachParent;

    if (parent) {
        icon->m_fFlags = (icon->m_fFlags & ~0x80) | (flag << 7);
    } else {
        icon->m_fFlags = (icon->m_fFlags & ~0x8) | (flag << 3);
    }

    if (parent) {
        icon->m_fFlags = (icon->m_fFlags & ~0x20000) | (flag << 17);
    } else {
        icon->m_fFlags = (icon->m_fFlags & ~0x10000) | (flag << 16);
    }
}

// VERIFY: 832dec86 auto
// OFFSET: 0x00743660
CM2Model* CGObject_C::SetModelStart(CM2Model* model) {
    if (model) {
        model->m_refCount++;
    }

    CM2Model* previous = this->m_worldModel;
    this->m_worldModel = model;
    return previous;
}

// VERIFY: 49434f02 auto
// OFFSET: 0x007433E0
void CGObject_C::SetObjectLocked(int32_t locked) {
    uint32_t count = this->m_modelFlags;

    if (locked && count != 0xFFFF) {
        this->m_modelFlags = count + 1;
    } else if (count) {
        this->m_modelFlags = count - 1;
    }
}

// VERIFY: 1393f91e manual
// OFFSET: 0x00744400
void CGObject_C::SetQuestStatus(uint32_t status) {
    uint32_t previous = this->m_questStatus;

    if (status == previous && (!CGObject_C::s_questStatusIcons[status] || this->m_interactIcon)) {
        return;
    }

    uint32_t icon = CGObject_C::s_questStatusIcons[status];
    int32_t changed = icon != CGObject_C::s_questStatusIcons[previous] || (icon && !this->m_interactIcon);
    this->m_questStatus = status;

    if (changed) {
        this->UpdateInteractIcon();
    }
}

// VERIFY: 19f06550 auto
// OFFSET: 0x00743640
void CGObject_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->m_obj = reinterpret_cast<CGObjectData*>(storage);
    this->m_objMirror = saved;
}

// VERIFY: 0db698ff auto
// OFFSET: 0x00743130
void CGObject_C::SetTypeID(OBJECT_TYPE_ID typeId) {
    this->m_typeId = typeId;

    switch (typeId) {
        case ID_OBJECT:
            this->m_obj->m_type = HEIR_TYPE_OBJECT;
            break;

        case ID_ITEM:
            this->m_obj->m_type = HEIR_TYPE_ITEM;
            break;

        case ID_CONTAINER:
            this->m_obj->m_type = HEIR_TYPE_CONTAINER;
            break;

        case ID_UNIT:
            this->m_obj->m_type = HEIR_TYPE_UNIT;
            break;

        case ID_PLAYER:
            this->m_obj->m_type = HEIR_TYPE_PLAYER;
            break;

        case ID_GAMEOBJECT:
            this->m_obj->m_type = HEIR_TYPE_GAMEOBJECT;
            break;

        case ID_DYNAMICOBJECT:
            this->m_obj->m_type = HEIR_TYPE_DYNAMICOBJECT;
            break;

        case ID_CORPSE:
            this->m_obj->m_type = HEIR_TYPE_CORPSE;
            break;

        default:
            break;
    }
}

// VERIFY: 18198d8d auto
// OFFSET: 0x00743C70
void CGObject_C::ShowHighlightType(HIGHLIGHTTYPE type) {
    this->m_modelFlags |= 1 << (type + 24);

    if (this->IsHighlightSuppressed()) {
        return;
    }

    s_highlightedObjectGuid = this->m_obj->m_guid;

    CM2Model* model = this->GetObjectModel();

    if (!model) {
        return;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    model->m_emissiveTintR = info->m_light1.m_ambient.r * (1.0f / 255.0f);
    model->m_emissiveTintG = info->m_light1.m_ambient.g * (1.0f / 255.0f);
    model->m_emissiveTintB = info->m_light1.m_ambient.b * (1.0f / 255.0f);

    if ((this->m_modelFlags & 0x7000000) != 0x4000000) {
        model->unk_01B8 = 1.0f;
    }
}

// VERIFY: ce4a803c manual
// OFFSET: 0x00744AC0
void CGObject_C::ClearEffectList(int32_t all) {
    if (all) {
        for (CEffect* effect = this->m_effectList; effect; effect = this->m_effectList) {
            effect->UnLinkSelf();

            if (effect->m_attachmentId == -1 && effect->m_expireTime) {
                effect->MoveToOrphanList();
            } else {
                effect->Release();
            }
        }

        return;
    }

    SpellRec spell;
    CEffect* effect = this->m_effectList;

    while (effect) {
        CEffect* next = effect->m_next;

        if (!g_spellDB.GetRecord(effect->m_spellId, &spell) || !(spell.m_attributesExC & 0x100000)) {
            effect->MoveToDestroyList();
        }

        effect = next;
    }
}

// VERIFY: 82510f34 manual
// OFFSET: [0x004D6010, 0x00745F90]
CGObject_C::~CGObject_C() {
    if (this->m_worldObject) {
        World::ObjectDestroy(this->m_worldObject);
        this->m_worldObject = nullptr;
    }

    this->ReleaseModel();
    this->ClearEffectList(1);
    this->RemoveInteractIcon();

    if (this->m_effectManager) {
        delete this->m_effectManager;
        this->m_effectManager = nullptr;
    }
}

// VERIFY: a5bf1653 auto
// OFFSET: 0x007440F0
int32_t CGObject_C::UpdateAllWorldObjects() {
    return ClntObjMgrEnumVisibleObjects(&UpdateAllWorldObjectsCallback, nullptr);
}

// VERIFY: 12ea82d2 auto
// OFFSET: 0x00744140
int32_t CGObject_C::UpdateAllObjectEffectManagers() {
    return ClntObjMgrEnumVisibleObjects(&UpdateAllObjectEffectManagersCallback, nullptr);
}

// VERIFY: 1f7da9e1 auto
// OFFSET: 0x007441D0
void CGObject_C::HandleScaleUpdate(float scale) {
    this->m_scaleChangeTime = FrameTime::s_curTimeMs;
    this->m_scaleChangeStart = scale;
    this->m_scale = scale;

    WOWGUID guid = this->m_obj->m_guid;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (guid == (camera->m_targetGUID)) {
        CGGameUI::ResetCamera(this->m_obj->m_guid);
    }
}

// VERIFY: 13895fa5 manual
// OFFSET: 0x00744E50
void CGObject_C::Shutdown() {
    if (CGObject_C::s_selectionTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(CGObject_C::s_selectionTexture));
    }

    CGObject_C::s_selectionTexture = nullptr;
    LoadInteractIcons(0);
    ClntObjMgrUnsetTypeMirrorHandler(ID_OBJECT, OBJECT_FIELD_SCALE_X * sizeof(uint32_t), &ScaleUpdateHandler, nullptr);
}

// VERIFY: e0b60d78 manual
// OFFSET: 0x00745E60
CGObject_C::CGObject_C(uint32_t time, const CClientObjCreate& create) {
    this->m_scale = 1.0f;
    this->m_scaleModifier = 1.0f;
    this->m_scaleChangeStart = 1.0f;
    this->m_interactIcon = nullptr;
    this->m_height = 1.0f;
    this->m_questStatus = 0;
    this->m_interactIconType = 0;
    this->m_scaleChangeTime = 0;
    this->m_effectList = nullptr;
    this->m_nameDesc = nullptr;
    this->m_worldModel = nullptr;
    this->m_worldObject = nullptr;
    this->m_modelFlags = 0;
    this->m_fadeStartTime = 0;
    this->m_fadeDuration = 0;
    this->m_alpha = 0;
    this->m_fadeStartAlpha = 0;
    this->m_fadeEndAlpha = 0;
    this->m_alphaScale = 0xFF;
    this->m_effectManager = nullptr;

    ClntObjMgrLinkInNewObject(this);
    this->m_scale = this->m_obj->m_scale;
}

// VERIFY: d6fbb1d4 manual
// OFFSET: 0x007460C0
void CGObject_C::Initialize() {
    CGObject_C::s_cvObjectSelectionCircle = CVar::Register("ObjectSelectionCircle", nullptr, 0, "1", nullptr, 0, false, nullptr, false);

    if (CGObject_C::s_selectionTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(CGObject_C::s_selectionTexture));
    }

    CStatus status;
    CGObject_C::s_selectionTexture = TextureCreate("Textures\\UnitSelectTexture.blp", CGxTexFlags(), &status, 1);
    LoadInteractIcons(1);
    ClntObjMgrSetTypeMirrorHandler(ID_OBJECT, OBJECT_FIELD_SCALE_X * sizeof(uint32_t), sizeof(float), &ScaleUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
}

// VERIFY: 64b6f506 auto
// OFFSET: 0x00744A50
void CGObject_C::PostInit(uint32_t time, const CClientObjCreate& create, bool a3) {
    this->m_modelFlags |= 0x40000;
    this->DoFade(this->GetBaseAlpha(), this->ShouldFadeIn() ? 1000 : 0);
    this->RefreshInteractIcon();
    this->m_scaleChangeTime = 0;

    if (this->m_worldObject) {
        this->UpdateWorldObject(0);
    }
}

// VERIFY: 2b7b33bd auto
// OFFSET: 0x007432E0
const SpellVisualRec* CGObject_C::GetAppropriateSpellVisual(const SpellRec* spell, SpellVisualRec& visual, int32_t visualId, int32_t lowViolenceVisualId) const {
    return GetSpellVisual(spell);
}

// VERIFY: 52331444 auto
// OFFSET: 0x00744330
void CGObject_C::ConnectToLightningThisFrame(uint32_t spellId, int32_t attachment) {
    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        if (effect->m_spellId == spellId && (effect->m_flags & 0x40000) && effect->m_lightningAttachment == attachment) {
            effect->ConnectToLightningThisFrame();
        }
    }
}

// VERIFY: abf45c20 manual
// OFFSET: 0x007450B0
CGObject_C* CGObject_C::GetFocusObject() {
    CGObject_C* object = ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__);

    if (!object) {
        object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__);

        if (!object) {
            return nullptr;
        }
    }

    if (!(object->m_obj->m_type & TYPE_PLAYER) || !(static_cast<CGPlayer_C*>(object)->m_combatModeFlags & 1)) {
        return object;
    }

    uint64_t farSight = static_cast<CGPlayer_C*>(object)->GetFarSightGuid();

    if (!farSight) {
        return object;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(WOWGUID{ farSight }, TYPE_OBJECT, __FILE__, __LINE__);
    return target ? target : object;
}

// VERIFY: bdeb4d6c auto
// OFFSET: 0x00743230
void CGObject_C::AuraDecayHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (!interrupted) {
        reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param))->MoveToDestroyList();
    }
}

// VERIFY: 8bec8c39 auto
// OFFSET: 0x00743250
void CGObject_C::OnSpellEffectClear(CEffect* effect) {
    if (effect->m_model && effect->m_model->IsLoaded(0, 0) && effect->m_model->HasSequence(159)) {
        effect->m_flags &= ~0x1000;
        effect->m_model->SetBoneSequence(0xFFFFFFFF, 159, 0xFFFFFFFF, 0, 1.0f, 1, 1);
        effect->m_model->SetSequenceCallback(reinterpret_cast<void*>(&CGObject_C::AuraDecayHandler), reinterpret_cast<uintptr_t>(effect));
        return;
    }

    if (effect->m_flags & 0x40) {
        SI2::StopSound(&effect->m_soundKitObject, 0, 0.3f, 1);
    }

    effect->MoveToDestroyList();
}

// VERIFY: 5c5abd1d auto
// OFFSET: 0x00743580
void CGObject_C::EffectDecayHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));

    if (!interrupted && effect) {
        effect->MoveToDestroyList();
    }
}

// VERIFY: 93774946 auto
// OFFSET: 0x00744920
void CGObject_C::AuraEffectHoldHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));

    if (interrupted || !effect || !effect->m_model) {
        return;
    }

    uint32_t endTime = effect->m_expireTime;

    if (endTime && static_cast<int32_t>(OsGetAsyncTimeMs() - endTime) > 0) {
        if (effect->m_model->HasSequence(159)) {
            effect->m_model->SetSequenceCallback(reinterpret_cast<void*>(&CGObject_C::EffectDecayHandler), reinterpret_cast<uintptr_t>(effect));
            effect->m_model->SetBoneSequence(0xFFFFFFFF, 159, 0xFFFFFFFF, 0, 1.0f, 1, 1);
        } else {
            effect->MoveToDestroyList();
        }
    } else {
        effect->m_model->SetBoneSequence(0xFFFFFFFF, sequenceId, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 4c989a64 auto
// OFFSET: 0x007449C0
void CGObject_C::AuraEffectHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));

    if (interrupted || !effect || !effect->m_model) {
        return;
    }

    if (effect->m_kitRec && (effect->m_kitRec->m_flags & 0x20)) {
        effect->m_model->SetSequenceCallback(reinterpret_cast<void*>(&CGObject_C::AuraEffectHoldHandler), reinterpret_cast<uintptr_t>(effect));
        effect->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else if (effect->m_model->HasSequence(158)) {
        effect->m_model->SetSequenceCallback(reinterpret_cast<void*>(&CGObject_C::AuraEffectHoldHandler), reinterpret_cast<uintptr_t>(effect));
        effect->m_model->SetBoneSequence(0xFFFFFFFF, 158, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 0fe69e0d auto
// OFFSET: 0x00744790
void CGObject_C::PlayEffectName(int32_t attachType, uint32_t a3, int32_t spellId, const SpellVisualKitRec* kit, const SpellVisualEffectNameRec* effectName, uint32_t* flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), const C3Vector* position, WOWGUID target, uint32_t a11, const SpellVisualKitModelAttachRec* attach) {
    CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
    effect->m_expireTime = a3;

    WOWGUID source = this->m_obj->m_guid;

    if (!target.m_guid) {
        target = this->m_obj->m_guid;
    }

    effect->AddEffect(attachType, spellId, kit, effectName, &target, &source, *flags, callback, position, a11, attach);

    if (*flags & 0x40000) {
        effect->SetEnabled(0);
    }

    if (kit->m_flags & 0x200) {
        effect->m_flags |= 0x2000000;
    }

    effect->Release();
    *flags = (*flags & ~0x411) | 0x400;
}

// VERIFY: 3b7cacb6 manual
// OFFSET: 0x00745230
void CGObject_C::PlaySpellVisualKit(const PlaySpellVisuaKitData& data) {
    uint32_t flags = 0;
    int32_t spellId = 0;
    int32_t hasEffect = 0;

    if (data.m_spellRec) {
        spellId = data.m_spellRec->m_id;
    }

    if (!data.m_impactVisualKit) {
        return;
    }

    C3Vector transformed = { 0.0f, 0.0f, 0.0f };
    const C3Vector* position = nullptr;

    if (data.m_position) {
        if (data.m_transportGUID.m_guid) {
            position = &transformed;
            C44Matrix transport;
            MovementGetTransportMtxX(data.m_transportGUID, &transport, WOWGUID{ 0 }, __FILE__, __LINE__);
            transformed = *data.m_position * transport;
        } else {
            position = data.m_position;
        }
    }

    void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t) = nullptr;

    if (data.m_impactVisualKit->m_shakeID) {
        flags = 0x10;
    }

    if (SI2::IsSoundKitLooping(data.m_impactVisualKit->m_soundID)) {
        flags |= 0x1;
    }

    const SpellRec* spell = data.m_spellRec;

    if (spell && spell->m_spellMissileID > 0) {
        flags |= 0x400000;
    }

    const SpellVisualRec* visual = spell ? g_spellVisualDB.GetRecord(spell->m_spellVisualID[0]) : nullptr;

    if (this->PlaySpellVisualKit_DelayLightningEffects(spell, data.m_impactVisualKit, static_cast<SPELL_VISUAL_KIT_TYPE>(data.m_kitType), data.m_position, data.m_transportGUID, data.m_playAnim, data.m_harmful, data.unused_0020, data.unused_0024, flags, visual)) {
        return;
    }

    if (visual && (visual->m_flags & 0x10)) {
        flags |= 0x200000;
    }

    const SpellVisualKitRec* kit = data.m_impactVisualKit;

    if (kit->m_flags & 0xC) {
        flags |= 0x1000000;
    }

    int32_t animParam = data.m_playAnim;

    switch (data.m_kitType) {
        case 0:
            flags |= 0x80;
            callback = &CGObject_C::EffectOnceHandler;
            flags = (flags & ~0x21) | 0x20;
            break;

        case 1:
        case 5:
        case 6:
        case 8:
            callback = &CGObject_C::EffectOnceHandler;
            flags = (flags & ~0x21) | 0x20;
            break;

        case 2:
            flags |= 0x21000;
            callback = &CGObject_C::AuraEffectHandler;

            if (spell) {
                if (spell->m_effectAura[0] == SPELL_AURA_MOUNTED) {
                    flags |= 0x800000;
                }

                if (spell->m_effectAura[1] == SPELL_AURA_MOUNTED) {
                    flags |= 0x800000;
                }

                if (spell->m_effectAura[2] == SPELL_AURA_MOUNTED) {
                    flags |= 0x800000;
                }
            }

            break;

        case 3:
            if (spell) {
                callback = &CGObject_C::EffectOnceHandler;
                flags = (flags & ~0x21) | 0x20;
            }

            animParam = 0;
            flags |= 0x22000;
            break;

        case 4:
            callback = &CGObject_C::LoopEffectHandler;
            flags |= 0x40;
            break;

        case 7:
            if (spell) {
                callback = &CGObject_C::LoopEffectHandler;
            }

            flags |= 0x22000;
            break;

        default:
            break;
    }

    SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(kit->m_headEffect);

    if (effectName) {
        this->PlayEffectName(20, 0, spellId, kit, effectName, &flags, callback, position, data.m_someGUID, 0, nullptr);
        hasEffect = 1;
    }

    effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_worldEffect);

    if (effectName) {
        CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
        effect->m_expireTime = 0;

        if (data.m_spellRec && !data.m_position) {
            for (uint32_t i = 0; i < 3; i++) {
                if (data.m_spellRec->m_effectAura[i] == SPELL_AURA_MOD_ROOT) {
                    flags |= 0x4000;
                    break;
                }
            }
        }

        WOWGUID transport = data.m_transportGUID;
        C3Vector objectPosition = { 0.0f, 0.0f, 0.0f };

        if (!data.m_position) {
            flags |= 0x200;
            objectPosition = this->GetRawPosition();
            transport.m_guid = this->GetTransportGUID();

            if ((this->m_obj->m_type & TYPE_UNIT) && (static_cast<CGUnit_C*>(this)->m_movement->m_flags & MOVEMENTFLAG_HOVER)) {
                objectPosition.z -= static_cast<CGUnit_C*>(this)->m_unit->m_unitFieldHoverheight;
            }
        }

        WOWGUID guid = this->m_obj->m_guid;
        const C3Vector* effectPosition = data.m_position ? data.m_position : &objectPosition;
        effect->InitFromUnit(*effectPosition, OsGetAsyncTimeMs(), spellId, data.m_impactVisualKit, effectName, &guid, flags, callback, data.m_effectFadeTime, nullptr, transport);
        effect->Release();
        flags = (flags & ~0x411) | 0x400;
        hasEffect = 1;
    }

    if (flags & 0x2000) {
        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_baseEffect);

        if (!effectName || !data.m_position) {
            goto attachments;
        }

        CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
        effect->m_expireTime = 0;
        WOWGUID guid = this->m_obj->m_guid;
        effect->InitFromUnit(*data.m_position, OsGetAsyncTimeMs(), spellId, data.m_impactVisualKit, effectName, &guid, flags, callback, data.m_effectFadeTime, nullptr, data.m_transportGUID);
        effect->Release();
        flags = (flags & ~0x411) | 0x400;
    } else {
        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_baseEffect);

        if (effectName) {
            this->PlayEffectName(19, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_leftHandEffect);

        if (effectName) {
            this->PlayEffectName(21, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_rightHandEffect);

        if (effectName) {
            this->PlayEffectName(22, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_breathEffect);

        if (effectName) {
            this->PlayEffectName(17, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_chestEffect);

        if (effectName) {
            this->PlayEffectName(34, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        this->PlaySpellVisualKit_HandleWeapons(data.m_spellRec, data.m_impactVisualKit, position, data.m_someGUID, data.m_effectFadeTime, flags, callback, &hasEffect);

        const SpellVisualKitRec* special = data.m_impactVisualKit;
        effectName = g_spellVisualEffectNameDB.GetRecord(special->m_specialEffect[0]);

        if (effectName) {
            this->PlayEffectName(23, 0, spellId, special, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_specialEffect[1]);

        if (effectName) {
            this->PlayEffectName(24, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, nullptr);
            hasEffect = 1;
        }

        effectName = g_spellVisualEffectNameDB.GetRecord(data.m_impactVisualKit->m_specialEffect[2]);

        if (!effectName) {
            goto attachments;
        }

        this->PlayEffectName(25, 0, spellId, data.m_impactVisualKit, effectName, &flags, callback, data.m_position, data.m_someGUID, data.m_effectFadeTime, nullptr);
    }

    hasEffect = 1;

attachments:
    int32_t attachCount = SpellVisualsGetModelAttachRecCount(data.m_impactVisualKit->m_id);

    for (int32_t i = 0; i < attachCount; i++) {
        const SpellVisualKitModelAttachRec* attach = SpellVisualsGetModelAttachRec(data.m_impactVisualKit->m_id, i);
        SpellVisualEffectNameRec* attachEffect = attach ? g_spellVisualEffectNameDB.GetRecord(attach->m_spellVisualEffectNameID) : nullptr;

        if (!attachEffect) {
            continue;
        }

        if (flags & 0x2000) {
            if (!data.m_position) {
                continue;
            }

            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->m_expireTime = 0;
            WOWGUID guid = this->m_obj->m_guid;
            effect->InitFromUnit(*data.m_position, OsGetAsyncTimeMs(), spellId, data.m_impactVisualKit, attachEffect, &guid, flags, callback, data.m_effectFadeTime, attach, data.m_transportGUID);
            effect->Release();
            flags = (flags & ~0x411) | 0x400;
        } else if (attach->m_attachmentID == -1) {
            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->m_expireTime = 0;

            if (data.m_spellRec && !data.m_position) {
                for (uint32_t j = 0; j < 3; j++) {
                    if (data.m_spellRec->m_effectAura[j] == SPELL_AURA_MOD_ROOT) {
                        flags |= 0x4000;
                        break;
                    }
                }
            }

            WOWGUID transport = data.m_transportGUID;
            C3Vector objectPosition = { 0.0f, 0.0f, 0.0f };

            if (!data.m_position) {
                flags |= 0x200;
                objectPosition = this->GetRawPosition();
                transport.m_guid = this->GetTransportGUID();

                if ((this->m_obj->m_type & TYPE_UNIT) && (static_cast<CGUnit_C*>(this)->m_movement->m_flags & MOVEMENTFLAG_HOVER)) {
                    objectPosition.z -= static_cast<CGUnit_C*>(this)->m_unit->m_unitFieldHoverheight;
                }
            }

            WOWGUID guid = this->m_obj->m_guid;
            const C3Vector* effectPosition = data.m_position ? data.m_position : &objectPosition;
            effect->InitFromUnit(*effectPosition, OsGetAsyncTimeMs(), spellId, data.m_impactVisualKit, attachEffect, &guid, flags, callback, data.m_effectFadeTime, attach, transport);
            effect->Release();
            flags = (flags & ~0x411) | 0x400;
        } else {
            this->PlayEffectName(attach->m_attachmentID, 0, spellId, data.m_impactVisualKit, attachEffect, &flags, callback, position, data.m_someGUID, data.m_effectFadeTime, attach);
        }

        hasEffect = 1;
    }

    this->PlaySpellVisualKit_PlayAnims(data.m_spellRec, data.m_impactVisualKit, static_cast<SPELL_VISUAL_KIT_TYPE>(data.m_kitType), position, animParam, data.m_harmful, data.m_someGUID, data.m_effectFadeTime, visual, flags, hasEffect);
}

// VERIFY: 1d3e05cf auto
// OFFSET: 0x00744870
void CGObject_C::EffectOnceHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));

    if (interrupted || !effect) {
        return;
    }

    if (effect->m_model && effect->m_model->HasSequence(159)) {
        effect->m_model->SetSequenceCallback(reinterpret_cast<void*>(&CGObject_C::EffectDecayHandler), reinterpret_cast<uintptr_t>(effect));
        effect->m_model->SetBoneSequence(0xFFFFFFFF, 159, 0xFFFFFFFF, 0, 1.0f, 1, 1);
        return;
    }

    M2BoneSequenceInfo info;
    effect->m_model->GetBoneSequenceInfo(boneId, info);
    effect->m_model->SetBoneSequenceTime(boneId, info.m_endTime - info.m_startTime - 1);
    uint32_t sceneTime = effect->m_model->m_scene->m_time;

    if (!sceneTime) {
        sceneTime = 1;
    }

    effect->m_model->m_lastAnimTime = sceneTime;
    effect->MoveToDestroyList();
}

// VERIFY: 211f8e2d auto
// OFFSET: 0x00744BD0
void CGObject_C::ClearEffectKit(int32_t spellId, int32_t kitType, int32_t a4, int32_t a5, int32_t a6) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    SpellVisualRec visualRec;
    const SpellVisualRec* visual = this->GetAppropriateSpellVisual(&spell, visualRec, 0, 0);

    if (!visual) {
        return;
    }

    SpellVisualKitRec* kit;

    switch (kitType) {
    case 0:
        kit = g_spellVisualKitDB.GetRecord(visual->m_castKit);
        break;
    case 1:
        kit = g_spellVisualKitDB.GetRecord(visual->m_impactKit);
        break;
    case 2:
        kit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);
        break;
    case 4:
        kit = g_spellVisualKitDB.GetRecord(visual->m_precastKit);
        break;
    case 5:
        kit = g_spellVisualKitDB.GetRecord(visual->m_casterImpactKit);
        break;
    case 6:
        kit = g_spellVisualKitDB.GetRecord(visual->m_targetImpactKit);
        break;
    case 7:
        kit = g_spellVisualKitDB.GetRecord(visual->m_missileTargetingKit);
        break;
    case 8:
        kit = g_spellVisualKitDB.GetRecord(visual->m_stateDoneKit);
        break;
    default:
        return;
    }

    if (!kit) {
        return;
    }

    CEffect* effect = this->m_effectList;

    while (effect) {
        CEffect* next = effect->m_next;

        if (effect->m_kitRec && effect->m_kitRec->m_id == kit->m_id && (!a5 || effect->m_spellId == static_cast<uint32_t>(spellId)) && (!a6 || effect->m_fadeTime == static_cast<uint32_t>(a4))) {
            effect->MoveToDestroyList();
        }

        effect = next;
    }
}

// VERIFY: b35e56bd auto
// OFFSET: 0x00743730
void CGObject_C::SetModel(CM2Model* model) {
    if (model == this->m_worldModel) {
        return;
    }

    if (model) {
        model->m_refCount++;
    }

    CM2Model* old = this->m_worldModel;
    this->m_worldModel = model;
    this->SetModelFinish(old);
}

// VERIFY: 51db1246 auto
// OFFSET: 0x00744150
void __stdcall CGObject_C::BuildSelectionRotMatrix(C44Matrix* matrix, const C3Vector* position) {
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

    C3Vector camera(0.0f, 0.0f, 0.0f);
    CGWorldFrame::GetCameraPosition(&camera);
    matrix->RotateAroundZ(-CalculateFacingTo(&camera, position));
}

// VERIFY: ab1865e5 manual
// OFFSET: 0x00744EB0
int32_t CGObject_C::SetCircleRenderStates() {
    CGxTex* texture = TextureGetGxTex(CGObject_C::s_selectionTexture, 0, nullptr);

    if (!texture) {
        return 0;
    }

    GxRsSet(GxRs_BlendingMode, GxBlend_Add);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, ProjectTex2dGetAddFadeTex());
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_ColorOp1, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp1, GxTexOp_Mod);
    return 1;
}
