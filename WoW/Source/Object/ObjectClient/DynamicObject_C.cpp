#include "DynamicObject_C.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Services/SysMessage.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellEffectCameraShakesRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellVisuals.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"

// VERIFY: 711b9f81 manual
// OFFSET: 0x00704F20
static void AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param) {
    CGDynamicObject_C* object = static_cast<CGDynamicObject_C*>(ClntObjMgrObjectPtr(WOWGUID{ param }, TYPEMASK_DYNAMICOBJECT, __FILE__, __LINE__));

    if (object) {
        object->HandleAnimEvent(eventId, data, position, time);
    }
}

// VERIFY: e474faa1 manual
// OFFSET: 0x00704AF0
static void AnimFinishedCallback(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (interrupted) {
        return;
    }

    CGDynamicObject_C* object = static_cast<CGDynamicObject_C*>(ClntObjMgrObjectPtr(WOWGUID{ param }, TYPEMASK_DYNAMICOBJECT, __FILE__, __LINE__));

    if (!object) {
        return;
    }

    if (object->m_stateFlags & 0x1) {
        object->GetObjectModel()->SetBoneSequence(0xFFFFFFFF, 158, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        object->GetObjectModel()->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 5dd58488 manual
// OFFSET: 0x007053A0
CGDynamicObject_C::CGDynamicObject_C(uint32_t time, const CClientObjCreate& create) : CGObject_C(time, create), m_passenger(create.m_move.m_status.m_transportGuid, create.m_move.m_status.m_transportPosition, this->m_obj->m_guid), m_scale(1.0f), m_blizzardVisual(nullptr) {
    this->m_stateFlags &= ~0x3;
    this->m_passenger.m_facing = create.m_move.m_status.m_transportFacing;
}

// VERIFY: b6793ae9 manual
// OFFSET: [0x007054E0, 0x007055E0]
CGDynamicObject_C::~CGDynamicObject_C() {
    if (this->m_blizzardVisual) {
        SpellVisualsBlizzardStop(&this->m_blizzardVisual);
    }

    SI2::StopSound(&this->m_soundKit, 0, 3.0f, 1);
}

// VERIFY: 2fd08b4c manual
// OFFSET: 0x00705230
void CGDynamicObject_C::PostInit(uint32_t time, const CClientObjCreate& create, bool a3) {
    this->m_passenger.PostInit(create.m_move.m_status.m_position);

    if (!this->GetTransportGUID()) {
        this->m_passenger.m_facing = create.m_move.m_status.m_facing;
    }

    this->CGObject_C::PostInit(time, create, a3);

    int32_t inTransit = Spell_C_IsSpellInTransit(this->m_dynamicObject->m_caster, this->m_dynamicObject->m_spellID, this->m_dynamicObject->m_castTime);
    this->m_stateFlags ^= (this->m_stateFlags ^ (inTransit * 2)) & 0x2;

    if (this->GetObjectModel()) {
        this->GetObjectModel()->SetEventCallback(reinterpret_cast<void*>(&AnimEventCallback), this->m_obj->m_guid.m_guid);
        this->GetObjectModel()->SetSequenceCallback(reinterpret_cast<void*>(&AnimFinishedCallback), this->m_obj->m_guid.m_guid);

        if (this->m_stateFlags & 0x2) {
            this->GetObjectModel()->SetEmittersEnabled(0);
            this->GetObjectModel()->SetRibbonsEnabled(0);
        }
    }

    this->ObjectVisKitProc();

    if (this->m_dynamicObject->m_bytes[0] != 2 || this->m_dynamicObject->m_caster != ClntObjMgrGetActivePlayer()) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && this->m_obj->m_guid.m_guid == player->GetFarSightGuid()) {
        player->SetFarSightFocus(this);
    }
}

// OFFSET: 0x00706560
uint64_t CGDynamicObject_C::GetTransportGUID() const {
    return this->m_passenger.m_transportGuid.m_guid;
}

// VERIFY: fe375946 manual
// OFFSET: 0x00704CC0
C44Matrix CGDynamicObject_C::GetMatrix() const {
    C44Matrix matrix;
    C3Vector position = this->GetRawPosition();
    matrix.d0 = position.x;
    matrix.d1 = position.y;
    matrix.d2 = position.z;
    matrix.RotateAroundZ(this->GetRawFacing());

    if (this->GetTransportGUID()) {
        CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPEMASK_OBJECT, __FILE__, __LINE__);

        if (transport) {
            matrix *= transport->GetMatrix();
        }
    }

    return matrix;
}

// VERIFY: 35b0255d auto
// OFFSET: 0x00706500
C3Vector CGDynamicObject_C::GetRawPosition() const {
    return this->m_passenger.m_position;
}

// VERIFY: 74221d0d auto
// OFFSET: 0x007049D0
void CGDynamicObject_C::ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden) {
    CGObject_C::ShouldRender(flags, render, hidden);

    if (this->m_stateFlags & 2) {
        hidden = 1;
    }
}

// VERIFY: 4e1858a9 auto
// OFFSET: 0x00706530
float CGDynamicObject_C::GetFacing() const {
    return this->m_passenger.GetFacing(this->m_passenger.m_facing);
}

// VERIFY: 909c3474 auto
// OFFSET: 0x007054D0
float CGDynamicObject_C::GetScale() const {
    return this->m_scale;
}

// VERIFY: 71b7b31f auto
// OFFSET: 0x00706550
float CGDynamicObject_C::GetRawFacing() const {
    return this->m_passenger.m_facing;
}

// VERIFY: 7501382d manual
// OFFSET: 0x007064E0
C3Vector CGDynamicObject_C::GetPosition() const {
    return this->m_passenger.GetPosition(this->m_passenger.m_position);
}

// VERIFY: 57b1635b auto
// OFFSET: 0x0070CBA0
void CGDynamicObject_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->CGObject_C::SetStorage(storage, saved);
    this->m_dynamicObject = reinterpret_cast<CGDynamicObjectData*>(storage + OBJECT_END);
    this->m_dynamicObjectMirror = saved + CGObject::TotalFieldsSaved();
}

// VERIFY: 549672d7 auto
// OFFSET: 0x005EEB70
void CGDynamicObject_C::Initialize() {
}

// VERIFY: ffb0c80d auto
// OFFSET: 0x00704910
void CGDynamicObject_C::SetFrameOfReference(const C44Matrix* matrix) {
    CGObject_C::SetFrameOfReference(matrix);

    if (this->m_blizzardVisual) {
        SpellVisualsBlizzardSetFrameOfReference(this->m_blizzardVisual, matrix);
    }
}

// VERIFY: 3f12c5a4 auto
// OFFSET: 0x00704940
void CGDynamicObject_C::Disable() {
    CGObject_C::Disable();
    this->m_passenger.m_link.Unlink();

    if (this->m_blizzardVisual) {
        SpellVisualsBlizzardStop(&this->m_blizzardVisual);
    }

    SI2::StopSound(&this->m_soundKit, 0, 3.0f, 1);
}

// VERIFY: 120cec34 manual
// OFFSET: 0x00704990
void CGDynamicObject_C::Reenable() {
    CGObject_C::Reenable();
    int32_t inTransit = Spell_C_IsSpellInTransit(this->m_dynamicObject->m_caster, this->m_dynamicObject->m_spellID, this->m_dynamicObject->m_castTime);
    this->m_stateFlags ^= (this->m_stateFlags ^ (inTransit * 2)) & 0x2;
}

// VERIFY: 34c768d1 auto
// OFFSET: 0x00704A40
static void DynamicObjectDecayHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CGDynamicObject_C* object = reinterpret_cast<CGDynamicObject_C*>(static_cast<uintptr_t>(param));

    if (object) {
        ClntObjMgrUnlockObject(object->m_obj->m_guid, __FILE__, __LINE__);
    }
}

// VERIFY: 254821be auto
// OFFSET: 0x00704A70
void CGDynamicObject_C::HandleOutOfRange(OUT_OF_RANGE_TYPE type) {
    if (type != 1) {
        return;
    }

    CM2Model* model = this->GetObjectModel();

    if (model && model->IsLoaded(0, 0) && model->HasSequence(159)) {
        ClntObjMgrLockObject(this->m_obj->m_guid);
        model->SetSequenceCallback(reinterpret_cast<void*>(&DynamicObjectDecayHandler), reinterpret_cast<uintptr_t>(this));
        model->SetBoneSequence(0xFFFFFFFF, 159, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 56897800 auto
// OFFSET: 0x00704B30
SpellVisualEffectNameRec* CGDynamicObject_C::GetVisualEffectNameRec(const SpellRec* spell) const {
    if (!spell) {
        SysMsgPrintf(STATUS_WARNING, 2, "NOSPELLIDFOUND|%d", this->m_dynamicObject->m_spellID);
        return nullptr;
    }

    const SpellVisualRec* visual = GetSpellVisual(spell);

    if (!visual) {
        SysMsgPrintf(STATUS_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d|%d", spell->m_spellVisualID[0], this->m_dynamicObject->m_spellID);
        return nullptr;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_persistentAreaKit);

    if (!kit) {
        SysMsgPrintf(STATUS_WARNING, 2, "SPELLVISUALKITIDNOTFOUND|%d", visual->m_persistentAreaKit);
        return nullptr;
    }

    SpellVisualEffectNameRec* effect = g_spellVisualEffectNameDB.GetRecord(kit->m_worldEffect);

    if (!effect) {
        effect = g_spellVisualEffectNameDB.GetRecord(kit->m_baseEffect);

        if (!effect) {
            SysMsgPrintf(STATUS_WARNING, 2, "SPELLEFFECTIDNOTFOUND|%d", kit->m_worldEffect);
        }
    }

    return effect;
}

// VERIFY: 5f42bd79 auto
// OFFSET: 0x00704C30
void CGDynamicObject_C::HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time) {
    SpellEffectCameraShakesRec* shakes = g_spellEffectCameraShakesDB.GetRecord(data);

    if (eventId == 0x444E5324) {
        SI2::PlaySoundKit(data, position, nullptr, nullptr, 0, nullptr, 1, 0);
        return;
    }

    if (eventId == 0x4B485324 && shakes) {
        for (uint32_t i = 0; i < 3; i++) {
            CGWorldFrame::GetActiveCamera()->AddShakeById(shakes->m_cameraShake[i], position);
        }
    }
}

// VERIFY: 2910d78f manual
// OFFSET: 0x00704D90
void CGDynamicObject_C::ModelLoaded(CM2Model* model) {
    CGObject_C::ModelLoaded(model);
    this->m_stateFlags ^= (this->GetObjectModel()->HasSequence(158) ^ this->m_stateFlags) & 0x1;
    SpellRec spell;
    g_spellDB.GetRecord(this->m_dynamicObject->m_spellID, &spell);
    SpellVisualEffectNameRec* effect = this->GetVisualEffectNameRec(&spell);

    if (spell.m_id && (spell.m_attributesExE & 0x40000000)) {
        model->m_fFlags |= 0x4;
    }

    uint8_t type = this->m_dynamicObject->m_bytes[0];

    if (type != 2 && type != 1) {
        CAaSphere sphere = this->GetObjectModel()->GetBoundingSphere();

        if (sphere.r > 0.001f) {
            this->m_scale = this->m_dynamicObject->m_radius / sphere.r;
        } else if (effect && effect->m_areaEffectSize > 0.0f) {
            this->m_scale = this->m_dynamicObject->m_radius / effect->m_areaEffectSize;
        }
    }

    if (effect && effect->m_scale > 0.0f) {
        this->m_scale = effect->m_scale * this->m_scale;
    }

    this->m_scaleModifier = this->m_scale * this->m_scaleModifier;

    if (effect) {
        this->DoFade(1.0f, 0);
    }

    if (this->GetObjectModel()->HasSequence(127)) {
        this->GetObjectModel()->SetBoneSequence(0xFFFFFFFF, 127, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 09497bd0 auto
// OFFSET: 0x00704F60
void CGDynamicObject_C::ObjectVisKitProc() {
    if (this->m_stateFlags & 0x2) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_dynamicObject->m_spellID, &spell)) {
        return;
    }

    const SpellVisualRec* visual = GetSpellVisual(&spell);

    if (!visual) {
        return;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_persistentAreaKit);

    if (!kit) {
        return;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (kit->m_charProc[i] == 9) {
            uint8_t density = static_cast<uint8_t>(static_cast<int32_t>(kit->m_charParamZero[i]));
            C3Vector position = this->GetRawPosition();
            this->m_blizzardVisual = SpellVisualsBlizzardCreate(&position, this->m_dynamicObject->m_radius, this->m_dynamicObject->m_spellID, density, kit->m_charParamOne[i]);
            break;
        }
    }

    if (kit->m_soundID) {
        SI2::StopSound(&this->m_soundKit, 0, 3.0f, 1);
        C3Vector position = this->GetPosition();
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_fadeInTime = 2.0f;
        properties.m_loopMode = 1;
        SI2::PlaySoundKit(kit->m_soundID, &position, &this->m_soundKit, &properties, 0, nullptr, 1, 0);
    }
}

// VERIFY: 5775200f auto
// OFFSET: 0x007050E0
void CGDynamicObject_C::PostReenable() {
    this->ObjectVisKitProc();
    this->m_passenger.Reenable();
    CGObject_C::PostReenable();
}

// VERIFY: dc132a97 manual
// OFFSET: 0x00705100
int32_t CGDynamicObject_C::GetModelFileName(const char** name) const {
    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_dynamicObject->m_spellID, &spell)) {
        SysMsgPrintf(STATUS_WARNING, 2, "NOSPELLIDFOUND|%d", this->m_dynamicObject->m_spellID);
        return 0;
    }

    SpellVisualEffectNameRec* effect = this->GetVisualEffectNameRec(&spell);

    if (!effect) {
        SysMsgPrintf(STATUS_FATAL, 2, "NOOBJECTFILENAME|%d|Dynamic", this->m_obj->m_entryID);
        return 0;
    }

    *name = effect->m_fileName;
    return *name != nullptr;
}

// VERIFY: 82c1b18c auto
// OFFSET: 0x007051B0
void CGDynamicObject_C::UpdateDisplay(uint32_t time) {
    if (!(this->m_stateFlags & 0x2) || Spell_C_IsSpellInTransit(this->m_dynamicObject->m_caster, this->m_dynamicObject->m_spellID, this->m_dynamicObject->m_castTime)) {
        return;
    }

    this->m_stateFlags &= ~0x2;
    CM2Model* model = this->GetObjectModel();

    if (model) {
        model->SetEmittersEnabled(1);
        model->SetRibbonsEnabled(1);
        model->SetBoneSequence(0xFFFFFFFF, 127, 0xFFFFFFFF, 0, 1.0f, 1, 1);
        this->ObjectVisKitProc();
    }
}
