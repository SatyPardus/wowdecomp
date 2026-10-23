#include "Effect_C.h"
#include <cstring>
#include <cmath>
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/SpellVisuals.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitModelAttachRec.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Light.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/Object/CreatureStats.h"

// STATIC: 0x00ADA764
static const char* s_specialUnitEffectNames[] = {
    "HARDCODED Footstep Water Run Spray",
    "HARDCODED Footstep Water Walk Spray",
    "HARDCODED Breath Underwater",
    "HARDCODED Breath Cold",
    "HARDCODED Loot Art",
    "HARDCODED Unit Level Up",
    "HARDCODED Mount Poof",
    "HARDCODED Inebriated Bubbles",
    "HARDCODED Meeting Stone Join",
    "HARDCODED Reputation",
    "HARDCODED Resist Spell",
    "HARDCODED Achievement Base",
};

// STATIC: 0x00CA0504
static CEffect* s_orphanedEffects;

// STATIC: 0x00CA0508
static CEffect* s_destroyedEffects;

// STATIC: 0x00CA050C
static int32_t s_specialUnitEffectIDs[sizeof(s_specialUnitEffectNames) / sizeof(s_specialUnitEffectNames[0])];

// VERIFY: 8db32f34 auto
// OFFSET: 0x006F7520
static int32_t BuildSpecialUnitEffectTable() {
    memset(s_specialUnitEffectIDs, 0, sizeof(s_specialUnitEffectIDs));

    for (int32_t i = 0; i < g_spellVisualEffectNameDB.m_numRecords; i++) {
        SpellVisualEffectNameRec* rec = (i < 0 || i >= g_spellVisualEffectNameDB.m_numRecords) ? nullptr : &g_spellVisualEffectNameDB.m_records[i];

        for (uint32_t effect = 0; effect < sizeof(s_specialUnitEffectNames) / sizeof(s_specialUnitEffectNames[0]); effect++) {
            if (!SStrCmp(s_specialUnitEffectNames[effect], rec->m_name, STORM_MAX_STR)) {
                s_specialUnitEffectIDs[effect] = rec->m_id;
                break;
            }
        }
    }

    return 1;
}

// VERIFY: d8c1c6a0 auto
// OFFSET: 0x006F75E0
int32_t SpecialUnitEffectsInitialize() {
    return BuildSpecialUnitEffectTable();
}

// VERIFY: 00dde46e manual
// OFFSET: 0x006FA3C0
void CEffect::Destroy() {
    while (s_destroyedEffects) {
        CEffect* effect = s_destroyedEffects;

        if (--effect->m_refCount == 0) {
            delete effect;
        }
    }

    while (s_orphanedEffects) {
        CEffect* effect = s_orphanedEffects;

        if (--effect->m_refCount == 0) {
            delete effect;
        }
    }
}

// VERIFY: 2ef1b8ec manual
// OFFSET: 0x006F74B0
void CEffect::UnLinkSelf() {
    if (this->m_prevLink) {
        *this->m_prevLink = this->m_next;
    }

    if (this->m_next) {
        this->m_next->m_prevLink = this->m_prevLink;
    }

    this->m_prevLink = nullptr;
    this->m_next = nullptr;
}

// VERIFY: 694be16d auto
// OFFSET: 0x006F75B0
const SpellVisualEffectNameRec* GetSpecialSpellVisualEffectNameRec(int32_t effect) {
    return g_spellVisualEffectNameDB.GetRecord(s_specialUnitEffectIDs[effect]);
}

// VERIFY: 582bd84a auto
// OFFSET: 0x006F75F0
void CEffect::SetEmissionRateIgnoresDistance(int32_t ignore) {
    if (!this->m_model || !this->m_model->IsLoaded(0, 0)) {
        return;
    }

    for (uint32_t i = 0; ; i++) {
        CM2Model* model = this->m_model;

        if (!(model->m_fFlags & 0x1)) {
            model->WaitForLoad(nullptr);
        }

        if (i >= model->m_shared->m_data->m_particles.m_count) {
            break;
        }

        model = this->m_model;

        if (!(model->m_fFlags & 0x1)) {
            model->WaitForLoad(nullptr);
        }

        CParticleEmitter2* emitter = model->m_particleEmitters[i];

        if (ignore) {
            emitter->m_flags |= 0x400000;
        } else {
            emitter->m_flags &= ~0x400000;
        }
    }
}

// VERIFY: be80d20e manual
// OFFSET: 0x006F76C0
void CEffect::LinkTo(CEffect** head) {
    if (this->m_prevLink) {
        *this->m_prevLink = this->m_next;
    }

    if (this->m_next) {
        this->m_next->m_prevLink = this->m_prevLink;
    }

    this->m_next = nullptr;
    this->m_prevLink = head;
    this->m_next = *head;
    *head = this;

    if (this->m_next) {
        this->m_next->m_prevLink = &this->m_next;
    }
}

// VERIFY: 166255dc manual
// OFFSET: 0x006F7850
void CEffect::DetachFromParent() {
    if (this->m_model && this->m_model->m_attachParent) {
        this->m_model->DetachFromParent();
    }
}

// VERIFY: 552c4366 auto
// OFFSET: 0x006F7870
void CEffect::SetEnabled(int32_t enabled) {
    if (!this->m_model) {
        return;
    }

    this->m_model->m_baseAlpha = enabled ? 1.0f : 0.0f;
    this->m_model->SetEmittersEnabled(enabled);
    this->m_model->SetRibbonsEnabled(enabled);
}

// VERIFY: af80ebbc auto
// OFFSET: 0x006F78B0
void CEffect::InitLightningEndpoint() {
    this->m_lightningState = 0;

    if (!this->m_model) {
        return;
    }

    uint32_t time = this->m_model->m_scene->m_time;

    if (!time) {
        time = 1;
    }

    this->m_model->m_lastAnimTime = time;
    this->SetEnabled(0);
}

// VERIFY: 71fcd0d5 auto
// OFFSET: 0x006F7900
void CEffect::ConnectToLightningThisFrame() {
    if (!this->m_lightningState && this->m_model) {
        this->m_model->m_lastAnimTime = 0;
        this->SetEnabled(1);
    }

    this->m_lightningState = 2;
}

// VERIFY: 43ed07f0 auto
// OFFSET: 0x006F97D0
void CEffect::AddFishingLine(CGObject_C* object, const WOWGUID& target) {
    CM2Model* model = object->m_worldModel;

    if (!model) {
        return;
    }

    CM2Model* line = model->m_attachList;

    while (line && line->m_attachmentId != 1) {
        line = line->m_attachNext;
    }

    this->m_lineModel = line;
    line->m_refCount++;
    this->m_lineModel->m_fFlags |= 0x20;
    this->m_lineModel->m_drawCallback = &CEffect::DrawFishingString;
    this->m_lineModel->m_drawArg = this;
    this->m_sourceGuid = object->m_obj->m_guid;
    this->m_targetGuid = target;
}

// VERIFY: 9ce5579d manual
// OFFSET: 0x006F9D70
CEffect::CEffect() {
    this->m_attachmentId = -1;
    this->m_model = nullptr;
    this->m_lineModel = nullptr;
    this->m_sourceGuid = WOWGUID();
    this->m_targetGuid = WOWGUID();
    this->m_spellId = 0;
    this->m_kitRec = nullptr;
    this->m_effectNameRec = nullptr;
    this->m_flags = 0;
    this->m_lightning[0] = nullptr;
    this->m_lightning[1] = nullptr;
    this->m_lightning[2] = nullptr;
    this->m_lightning[3] = nullptr;
    this->m_lightning[4] = nullptr;
    this->m_lightning[5] = nullptr;
    this->m_lightning[6] = nullptr;
    this->m_lightning[7] = nullptr;
    this->m_lightning[8] = nullptr;
    this->m_lightning[9] = nullptr;
    this->m_lightning[10] = nullptr;
    this->m_lightning[11] = nullptr;
    this->unused_0080 = 0;
    this->unused_0084 = 0;
    this->m_facing = 0.0f;
    this->m_lightningAttachment = -1;
    this->m_mountTransition = nullptr;
    this->m_pendingCacheId = 0;
    this->m_displayId = 0;
    this->m_itemSlot = 12;
    this->m_fadeTime = 0;
    this->m_transportGuid = WOWGUID();
    this->m_startTime = 0;
    this->m_expireTime = 0;
    this->unk_00D0 = 0;
    this->m_attachToModel = 0;
    this->m_attachToMount = 0;
    this->unused_00DC = 0;
    this->unused_00E0 = 0.0f;
    this->unused_00E4 = 0.0f;
    this->m_alpha = 0.0f;
    this->m_mapEntity = nullptr;
    this->m_refCount = 1;
    this->m_lightningRefCount = 0;
    this->m_lightningState = 0;
    this->m_attachRec = nullptr;
    this->m_prevLink = nullptr;
    this->m_next = nullptr;
}

// VERIFY: c30a71c8 auto
// OFFSET: 0x006F7A00
void CEffect::MoveToOrphanList() {
    this->LinkTo(&s_orphanedEffects);
}

// VERIFY: 1b5e24a5 manual
// OFFSET: 0x006FA390
void CEffect::Release() {
    if (this->m_refCount-- == 1) {
        delete this;
    }
}

// VERIFY: 07c58fd0 manual
// OFFSET: 0x006F7C40
void CEffect::InitFromUnit(C3Vector position, uint32_t time, int32_t spellId, const SpellVisualKitRec* kit, const SpellVisualEffectNameRec* effectName, const WOWGUID* guid, uint32_t flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), uint32_t a9, const SpellVisualKitModelAttachRec* attach, WOWGUID transport) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    this->m_effectNameRec = effectName;
    this->m_sourceGuid = *guid;
    this->m_position = position;
    this->m_flags = flags | 0x8002;
    this->m_transportGuid = transport;
    this->m_kitRec = kit;
    this->m_spellId = spellId;
    this->m_startTime = time;
    this->m_fadeTime = a9;
    this->m_facing = unit->GetSmoothFacing();
    this->m_attachRec = attach;
    this->m_model = World::s_m2Scene->CreateModel(this->m_effectNameRec->m_fileName, 0);

    if (this->m_model) {
        this->m_refCount++;
        this->m_model->SetLoadedCallback(&CEffect::CreateModelFromEffect, this);
        this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), reinterpret_cast<uintptr_t>(this));
        this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));

        if (this->m_flags & 0x400000) {
            this->SetEmissionRateIgnoresDistance(1);
        }
    }

    this->LinkTo(&unit->m_effectList);
}

// VERIFY: 2e373253 auto
// OFFSET: 0x006F7680
void CEffect::CreateModelFromEffect(CM2Model* model, void* arg) {
    CEffect* effect = static_cast<CEffect*>(arg);

    if (effect->m_flags & 0x400000) {
        effect->SetEmissionRateIgnoresDistance(1);
    }

    model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
}

// VERIFY: 392d7fa2 manual
// OFFSET: 0x006F7B00
void CEffect::EffectEvent(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param) {
    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));
    CGUnit_C* object = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(effect->m_targetGuid, TYPE_OBJECT, __FILE__, __LINE__));

    if (!object) {
        return;
    }

    if (eventId == 0x444E5324) {
        if ((object->m_obj->m_type & TYPE_UNIT) && object->IsActiveMover()) {
            SE2SoundKitProperties properties;
            properties.ResetToDefaults();

            static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

            int32_t listenerAtCharacter = s_listenerAtCharacter->m_intValue;

            if (listenerAtCharacter) {
                properties.m_volumeScale = 0.64999998f;
            }

            properties.m_priority = 110;
            SI2::PlaySoundKit(data, listenerAtCharacter ? nullptr : position, &effect->m_soundKitObject, &properties, 0, nullptr, 1, 0);
        } else {
            SI2::PlaySoundKit(data, position, &effect->m_soundKitObject, nullptr, 0, nullptr, 1, 0);
        }

        if (!(effect->m_flags & 0x200000)) {
            SI2::AttachToObject(&effect->m_soundKitObject, object->m_obj->m_guid);
        }
    } else if (eventId == 0x54494824 && (object->m_obj->m_type & TYPE_UNIT)) {
        object->PlayWoundAnimKit(0);
    }
}

// VERIFY: ac84d6f6 manual
// OFFSET: 0x006F94C0
void CEffect::AddEffect(int32_t attachType, int32_t spellId, const SpellVisualKitRec* kit, const SpellVisualEffectNameRec* effectName, const WOWGUID* target, const WOWGUID* source, uint32_t flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), const C3Vector* position, uint32_t a10, const SpellVisualKitModelAttachRec* attach) {
    CGObject_C* object = static_cast<CGObject_C*>(ClntObjMgrObjectPtr(*source, TYPE_OBJECT, __FILE__, __LINE__));

    if (!object) {
        return;
    }

    this->m_effectNameRec = effectName;
    CEffect::DestroyMatchingEffects(object, effectName, attachType, spellId, kit ? kit->m_id : 0, attach);

    if (attachType == -1) {
        DebugLog("**** CEffect::AddEffect called with ATTACH_NONE\n");
    }

    this->m_attachmentId = attachType;
    this->m_sourceGuid = *target;
    this->m_targetGuid = *source;
    this->m_kitRec = kit;
    this->m_fadeTime = a10;
    this->m_attachRec = attach;
    this->m_spellId = spellId;
    this->m_effectNameRec = effectName;
    this->m_flags = flags;

    if (position) {
        this->m_offset = *position;
        this->m_flags = flags | 0x10000;
    }

    this->m_model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0);

    if (this->m_model) {
        this->m_refCount++;
        this->m_model->SetLoadedCallback(&CEffect::CreateModelFromEffect, this);
        this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), reinterpret_cast<uintptr_t>(this));
        this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));

        if (this->m_flags & 0x400000) {
            this->SetEmissionRateIgnoresDistance(1);
        }
    }

    this->LinkTo(&object->m_effectList);
}

// VERIFY: 2eb5c934 auto
// OFFSET: 0x006F8A60
void __stdcall CEffect::DestroyMatchingEffects(CGObject_C* object, const SpellVisualEffectNameRec* effectName, int32_t attachType, int32_t spellId, int32_t kitId, const SpellVisualKitModelAttachRec* attach) {
    if (attachType == -1) {
        return;
    }

    CEffect* effect = object->m_effectList;

    while (effect) {
        CEffect* next = effect->m_next;

        if (effect->m_effectNameRec && effect->m_effectNameRec->m_id == effectName->m_id && (!effect->m_kitRec || effect->m_kitRec->m_id == kitId) && (!effect->m_attachRec || effect->m_attachRec == attach) && (!spellId || effect->m_spellId == static_cast<uint32_t>(spellId)) && effect->m_attachmentId == attachType) {
            effect->MoveToDestroyList();
        }

        effect = next;
    }
}

// VERIFY: 25c89a39 manual
// OFFSET: 0x006F7E40
void CEffect::AddSoundEffect(int32_t soundId, int32_t expireTime, int32_t spellId, const C3Vector* position, CGObject_C* object, uint32_t fadeTime) {
    if (!object) {
        return;
    }

    this->LinkTo(&object->m_effectList);
    this->m_refCount++;
    this->m_expireTime = expireTime;
    this->m_spellId = spellId;
    this->m_sourceGuid = object->m_obj->m_guid;
    this->m_fadeTime = fadeTime;

    const C3Vector* soundPosition = position;

    if (position) {
        this->m_position = *position;
    } else {
        this->m_position = object->GetPosition();
        soundPosition = &this->m_position;
    }

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (this->m_flags & 0x1) {
        properties.m_loopMode = 1;
        properties.m_noRandomStart = 1;
    } else {
        properties.m_maxAudibleBehavior = 0;
        properties.m_loopMode = 2;

        if (CGUnit_C::s_activeMover == this->m_sourceGuid) {
            properties.m_priority = 110;
        }
    }

    SI2::PlaySoundKit(soundId, soundPosition, &this->m_soundKitObject, &properties, 0, nullptr, 1, 0);

    if (!(this->m_flags & 0x200000)) {
        SI2::AttachToObject(&this->m_soundKitObject, object->m_obj->m_guid);
    }

    this->m_flags |= 0x100800;
}

// VERIFY: 561572d5 manual
// OFFSET: 0x006F7FD0
void CEffect::AddAnimKitEffect(int32_t spellId, const SpellVisualKitRec* kit, const WOWGUID* guid, uint32_t flags, uint32_t fadeTime) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    this->LinkTo(&unit->m_effectList);
    this->m_refCount++;
    this->m_spellId = spellId;
    this->m_sourceGuid = *guid;
    this->m_flags |= flags | 0x800;
    this->m_kitRec = kit;
    this->m_fadeTime = fadeTime;
}

// STATIC: 0x00A32AF4
static const int32_t s_specialUnitEffectAttachments[] = { 50, 50, 17, 17, 19, 19, 19, 17, 19, 19, 34, 19 };

// VERIFY: f48d6796 auto
// OFFSET: 0x006F8F40
void CEffect::ReleaseLightningRef() {
    if (--this->m_lightningRefCount == 0) {
        this->MoveToDestroyList();
    }
}

// VERIFY: e6c5c900 manual
// OFFSET: 0x006F9260
void CEffect::AddSpecialEffect(int32_t type, const WOWGUID& target, void (*callback)(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param)) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit || static_cast<uint32_t>(type) >= NUM_UNITEFFECTSPECIALS) {
        return;
    }

    int32_t attachment = s_specialUnitEffectAttachments[type];

    if (attachment == 50 || !unit->m_worldModel) {
        return;
    }

    this->m_sourceGuid = target;
    this->m_targetGuid = target;
    this->m_flags = 0;
    this->m_attachmentId = attachment;

    if (unit->m_worldModel->IsLoaded(0, 0) && !unit->m_worldModel->HasAttachment(this->m_attachmentId)) {
        this->m_attachmentId = 19;
    }

    this->m_effectNameRec = g_spellVisualEffectNameDB.GetRecord(s_specialUnitEffectIDs[type]);
    CEffect::DestroyMatchingEffects(unit, this->m_effectNameRec, attachment, 0, 0, nullptr);
    this->m_model = World::s_m2Scene->CreateModel(this->m_effectNameRec->m_fileName, 0);

    if (this->m_model) {
        this->m_refCount++;
        this->m_model->SetLoadedCallback(&CEffect::CreateModelFromEffect, this);
        this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), reinterpret_cast<uintptr_t>(this));
        this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));
    }

    this->LinkTo(&unit->m_effectList);
}

// VERIFY: 3f5d1803 manual
// OFFSET: 0x006F93A0
void CEffect::AddSpecialByPosition(int32_t type, C3Vector position, const WOWGUID* guid, uint32_t flags, void (*callback)(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param)) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    this->m_sourceGuid = *guid;
    this->m_position.x = position.x;
    this->m_flags = flags | 2;
    this->m_position.y = position.y;
    this->m_position.z = position.z;
    this->m_transportGuid = WOWGUID();
    this->m_effectNameRec = g_spellVisualEffectNameDB.GetRecord(s_specialUnitEffectIDs[type]);
    this->m_model = World::s_m2Scene->CreateModel(this->m_effectNameRec->m_fileName, 0);

    if (this->m_model) {
        this->m_refCount++;
        this->m_model->SetLoadedCallback(&CEffect::CreateModelFromEffect, this);
        this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), reinterpret_cast<uintptr_t>(this));
        this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));

        if (this->m_flags & 0x400000) {
            this->SetEmissionRateIgnoresDistance(1);
        }
    }

    this->LinkTo(&unit->m_effectList);
}

// VERIFY: 097c23de auto
// OFFSET: 0x006F74F0
void CEffect::ClearLightning() {
    for (uint32_t i = 0; i < sizeof(this->m_lightning) / sizeof(this->m_lightning[0]); i++) {
        if (this->m_lightning[i]) {
            LightningObjectReleaseRef(this->m_lightning[i]);
        }

        this->m_lightning[i] = nullptr;
    }
}

// VERIFY: 4c42526f auto
// OFFSET: 0x006F8600
void CEffect::ApplyItemVisualByItemId(int32_t id, const WOWGUID& guid, void* param, bool success) {
    ItemRec* item = g_itemDB.GetRecord(id);

    if (!item) {
        return;
    }

    CEffect* effect = static_cast<CEffect*>(param);
    effect->m_itemSlot = CCharacterComponent::GetItemSlotForInvType(static_cast<INVENTORY_TYPES>(item->m_inventoryType));
    effect->m_displayId = item->m_displayInfoID;
    effect->ApplyItemVisual();
}

// VERIFY: 57136c38 manual
// OFFSET: 0x006F8700
void CEffect::ClearItemVisualEffect() {
    if (!(this->m_flags & 0x80000)) {
        return;
    }

    if (this->m_itemSlot < 12) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            if (unit->m_characterComponent) {
                unit->m_characterComponent->RemoveItem(static_cast<ITEM_SLOT>(this->m_itemSlot));
                unit->RefreshComponentItem(this->m_itemSlot);
                CGGameUI::UnitModelUpdate(this->m_sourceGuid, 3);
            }

            this->m_itemSlot = 12;
        }
    }

    if (this->m_pendingCacheId) {
        g_itemDBCache.CancelCallback(this->m_pendingCacheId, CEffect::ApplyItemVisualByItemId, this);
        this->m_pendingCacheId = 0;
    }

    this->m_flags &= 0x80000;
    this->m_itemSlot = 12;
}

// VERIFY: 9f9c444f manual
// OFFSET: 0x006F87C0
void CEffect::MoveToDestroyList() {
    this->LinkTo(&s_destroyedEffects);
    this->m_flags |= 0x900;

    if (this->m_flags & 0x4) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            if (this->m_attachToMount && unit->m_mountModel) {
                unit->m_mountModel->m_lastAnimTime = 0;
                this->m_attachToMount = 0;
            }

            if (this->m_attachToModel && unit->m_worldModel) {
                unit->m_worldModel->m_lastAnimTime = 0;
                this->m_attachToModel = 0;
            }

            unit->UpdateBaseAnimation(0, -1);
        }
    }

    if (SI2::IsLooping(&this->m_soundKitObject)) {
        SI2::StopSound(&this->m_soundKitObject, 0, 0.15000001f, 1);
    } else {
        SI2::Detach(&this->m_soundKitObject);
    }

    if (this->m_mountTransition) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            if (SpellVisualsIsMountTransitionFinished(this->m_mountTransition)) {
                unit->CompleteMountTransition();
            } else {
                unit->StopMountTransition();
            }

            unit->UpdateBaseAnimation(0, -1);
        }

        SpellVisualsCancelMountTransition(this->m_mountTransition);
        this->m_mountTransition = nullptr;
    }

    if (this->m_model) {
        this->m_model->SetSequenceCallback(nullptr, 0);
        this->m_model->SetEventCallback(nullptr, 0);
        this->m_model->SetLoadedCallback(nullptr, nullptr);
        CM2Model* model = this->m_model;

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x80u;
        } else {
            model->m_fFlags &= ~0x8u;
        }

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x20000u;
        } else {
            model->m_fFlags &= ~0x10000u;
        }

        if (this->m_model->m_attachParent) {
            this->m_model->m_fFlags |= 0x20000;
        } else {
            this->m_model->m_fFlags |= 0x10000;
        }

        this->m_model->SetEmittersEnabled(0);
    }

    this->ClearItemVisualEffect();
    this->ClearLightning();

    if (!this->m_prevLink) {
        this->m_refCount++;
    }
}

// VERIFY: 7b7099ce manual
// OFFSET: 0x006F9EC0
CEffect::~CEffect() {
    if (this->m_model) {
        if (this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        this->m_model->SetSequenceCallback(nullptr, 0);
        this->m_model->SetEventCallback(nullptr, 0);
        this->m_model->SetLoadedCallback(nullptr, nullptr);
        CM2Model* model = this->m_model;

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x80u;
        } else {
            model->m_fFlags &= ~0x8u;
        }

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x20000u;
        } else {
            model->m_fFlags &= ~0x10000u;
        }

        this->m_model->Release();
        this->m_model = nullptr;
    }

    if (this->m_lineModel) {
        this->m_lineModel->m_fFlags |= 0x20;
        this->m_lineModel->m_drawCallback = nullptr;
        this->m_lineModel->m_drawArg = nullptr;
        this->m_lineModel->Release();
    }

    this->UnLinkSelf();

    if (this->m_mapEntity) {
        World::ObjectDestroy(this->m_mapEntity);
    }

    if (SI2::IsLooping(&this->m_soundKitObject)) {
        SI2::StopSound(&this->m_soundKitObject, 0, 0.15000001f, 1);
    } else {
        SI2::Detach(&this->m_soundKitObject);
    }

    if (this->m_mountTransition) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->StopMountTransition();
            unit->UpdateBaseAnimation(0, -1);
        }

        SpellVisualsCancelMountTransition(this->m_mountTransition);
        this->m_mountTransition = nullptr;

        if (this->m_pendingCacheId) {
            g_creatureDBCache.CancelCallback(this->m_pendingCacheId, CEffect::MountTransitionCacheCB, this);
            this->m_pendingCacheId = 0;
        }
    }

    this->ClearItemVisualEffect();
    this->ClearLightning();
}

// VERIFY: bf507d37 manual
// OFFSET: 0x006FA450
void CEffect::CleanUpStaticLists() {
    uint32_t now = FrameTime::s_curTimeMs;
    CEffect* effect = s_orphanedEffects;

    while (effect) {
        CEffect* next = effect->m_next;

        if (!effect->m_startTime || static_cast<int32_t>(now - effect->m_startTime) > 0) {
            effect->EffectUpdate();
        }

        if (effect->m_expireTime && static_cast<int32_t>(now - effect->m_expireTime) > 0) {
            effect->UnLinkSelf();

            if (effect->m_refCount-- == 1) {
                delete effect;
            }
        }

        effect = next;
    }

    effect = s_destroyedEffects;

    while (effect) {
        CEffect* next = effect->m_next;

        if (!effect->m_model || !(effect->m_model->m_fFlags & 0x400)) {
            effect->UnLinkSelf();

            if (effect->m_refCount-- == 1) {
                delete effect;
            }
        }

        effect = next;
    }
}

// VERIFY: f8d5c444 manual
// OFFSET: 0x006FA5B0
void CEffect::AddLootEffect(CGUnit_C* unit, void (*callback)(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param)) {
    this->m_sourceGuid = unit->m_obj->m_guid;
    this->m_flags = 0x8;
    this->m_attachmentId = 19;
    const SpellVisualEffectNameRec* effectName = GetSpecialSpellVisualEffectNameRec(SPECIALEFFECT_LOOTART);
    this->m_effectNameRec = effectName;
    this->m_model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0);

    if (!this->m_model) {
        delete this;
        return;
    }

    if (this->m_flags & 0x400000) {
        this->SetEmissionRateIgnoresDistance(1);
    }

    this->m_model->SetLoadedCallback(&CEffect::CreateModelFromEffect, this);
    this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), reinterpret_cast<uintptr_t>(this));
    this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));
    this->LinkTo(&unit->m_effectList);
    this->m_refCount++;
}

// VERIFY: 7e81c241 manual
// OFFSET: 0x006F7720
C3Vector CEffect::TransformPositionByTransport() {
    if (!this->m_transportGuid) {
        return this->m_position;
    }

    C44Matrix transport;

    if (MovementGetTransportMtxSafe(this->m_transportGuid, &transport)) {
        this->m_worldPosition = this->m_position * transport;
        return this->m_worldPosition;
    }

    this->m_position = this->m_worldPosition;
    this->m_transportGuid = WOWGUID();
    return this->m_worldPosition;
}

// VERIFY: f3d1c304 manual
// OFFSET: 0x006F7950
float __stdcall CEffect::ComputeModelScaleFromTarget(CGObject_C* object) {
    CM2Model* model = object->GetObjectModel();

    if (model && !model->IsLoaded(0, 1)) {
        return 1.0f;
    }

    float scale = 1.0f;

    if (object->m_obj->m_type & TYPE_UNIT) {
        CreatureModelDataRec* modelData = static_cast<CGUnit_C*>(object)->GetModelData();

        if (modelData) {
            scale = modelData->m_worldEffectScale;
        }
    }

    CAaBox box = model->GetBoundingBox();
    float size = box.t.x - box.b.x;
    float depth = box.t.y - box.b.y;

    if (depth <= size) {
        size = depth;
    }

    size *= 0.30000001f;

    if (size <= 1.0f) {
        return 1.0f * scale;
    }

    return size * scale;
}

// VERIFY: cd786896 auto
// OFFSET: 0x006F8040
void CEffect::AddLightning(int32_t expireTime, uint32_t spellId, const SpellVisualKitRec* kit, const WOWGUID* guid, uint32_t flags) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    this->LinkTo(&unit->m_effectList);
    this->m_refCount++;
    this->m_kitRec = kit;
    this->m_expireTime = expireTime;
    this->m_spellId = spellId;
    this->m_sourceGuid = *guid;
    this->m_flags = flags;
}

// VERIFY: bf2f00fc manual
// OFFSET: 0x006F84F0
void CEffect::ApplyModelAttachRecToMatrix(const SpellVisualKitModelAttachRec* attach, C44Matrix& matrix, float facing) {
    float x = matrix.d0;
    float y = matrix.d1;
    float z = matrix.d2;
    matrix.d0 = 0.0f;
    matrix.d1 = 0.0f;
    matrix.d2 = 0.0f;

    if (0.0f != attach->m_yaw) {
        matrix.RotateAroundZ(-attach->m_yaw);
    }

    if (0.0f != attach->m_pitch) {
        matrix.RotateAroundY(attach->m_pitch);
    }

    if (0.0f != attach->m_roll) {
        matrix.RotateAroundX(attach->m_roll);
    }

    if (facing == 0.0f) {
        matrix.d0 = attach->m_offset[0] + x;
        matrix.d1 = y - attach->m_offset[1];
    } else {
        float s;
        float c;
        CMath::sincos_(facing - 1.5707964f, &s, &c);
        matrix.d0 = c * attach->m_offset[1] + x - attach->m_offset[0] * s;
        matrix.d1 = c * attach->m_offset[0] + s * attach->m_offset[1] + y;
    }

    matrix.d2 = attach->m_offset[2] + z;
}

// VERIFY: 9308087b manual
// OFFSET: 0x006F8AE0
C44Matrix* CEffect::TransformPosition(C44Matrix* matrix, CGObject_C* object) {
    *matrix = C44Matrix();
    C3Vector position = this->TransformPositionByTransport();
    matrix->d0 = position.x;
    matrix->d1 = position.y;
    matrix->d2 = position.z;
    float facing = 0.0f;

    if (object) {
        if (object->m_obj->m_type & TYPE_UNIT) {
            facing = static_cast<CGUnit_C*>(object)->GetSmoothFacing();
        } else {
            facing = object->GetFacing();
        }
    }

    if (this->m_flags & 0x8000) {
        matrix->RotateAroundZ(this->m_facing);
    } else if (object) {
        matrix->RotateAroundZ(facing);
    }

    float scale = 1.0f;

    if ((this->m_flags & 0x200) && object) {
        float targetScale = CEffect::ComputeModelScaleFromTarget(object);
        scale = object->GetTrueScale() * targetScale;
    }

    if (this->m_effectNameRec) {
        float minScale = this->m_effectNameRec->m_minAllowedScale;
        float effectScale = scale * this->m_effectNameRec->m_scale;
        scale = this->m_effectNameRec->m_maxAllowedScale;

        if (effectScale < minScale) {
            scale = minScale;
        } else if (effectScale < scale) {
            scale = effectScale;
        }
    }

    if (scale <= 0.0f) {
        scale = 1.0f;
    }

    matrix->Scale(scale);

    if (this->m_attachRec) {
        CEffect::ApplyModelAttachRecToMatrix(this->m_attachRec, *matrix, (this->m_flags & 0x8000) ? this->m_facing : facing);
    }

    return matrix;
}

// VERIFY: 03a1abb4 manual
// OFFSET: 0x006F8C50
int32_t CEffect::UpdateAttachment() {
    if (this->m_attachmentId == -1) {
        if (this->m_model && this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        return 1;
    }

    CGObject_C* object;

    if (this->m_targetGuid) {
        object = ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_OBJECT, __FILE__, __LINE__);
    } else if (this->m_sourceGuid) {
        object = ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_OBJECT, __FILE__, __LINE__);
    } else {
        object = nullptr;
    }

    if (!object) {
        if (this->m_model && this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        return 0;
    }

    CM2Model* parent = object->m_worldModel;

    if (parent && !parent->IsLoaded(0, 0)) {
        DebugLog("**** CEffect::UpdateAttachment() called before %s finished loading\n", parent->m_shared->m_fileNameWithoutPath);

        if (this->m_model && this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        this->MoveToDestroyList();
        return 0;
    }

    CreatureModelDataRec* modelData = (object->m_obj->m_type & TYPE_UNIT) ? static_cast<CGUnit_C*>(object)->GetModelData() : nullptr;

    if ((this->m_attachmentId == 2 || this->m_attachmentId == 1) && modelData) {
        CreatureModelDataRec* data = static_cast<CGUnit_C*>(object)->GetModelData();

        if (data && (data->m_flags & 0x10)) {
            this->DetachFromParent();
            return 0;
        }
    }

    if (!parent || !parent->HasAttachment(this->m_attachmentId)) {
        if (this->m_model && this->m_model->m_attachParent) {
            this->m_model->DetachFromParent();
        }

        this->MoveToDestroyList();
        return 0;
    }

    float scale = 1.0f;

    if (modelData) {
        scale = modelData->m_attachedEffectScale;
    }

    if (this->m_effectNameRec) {
        scale *= this->m_effectNameRec->m_scale;
        float effectScale = scale;
        float worldScale = parent->GetAttachmentWorldScale(this->m_attachmentId) * scale;

        if (worldScale > 0.000001f) {
            if (this->m_effectNameRec->m_maxAllowedScale < worldScale) {
                scale = effectScale / worldScale * this->m_effectNameRec->m_maxAllowedScale;
            } else if (this->m_effectNameRec->m_minAllowedScale > worldScale) {
                scale = this->m_effectNameRec->m_minAllowedScale / worldScale * effectScale;
            }
        }
    }

    if (this->m_model && (1.0f != scale || this->m_attachRec)) {
        C44Matrix transform;

        if (this->m_attachRec) {
            CEffect::ApplyModelAttachRecToMatrix(this->m_attachRec, transform, 0.0f);
        }

        transform.Scale(scale);
        this->m_model->SetWorldTransform(transform);
    }

    if (this->m_flags & 0x10000) {
        if (this->m_model && (this->m_model->m_attachParent != parent || this->m_model->m_attachmentId != static_cast<uint32_t>(this->m_attachmentId))) {
            this->DetachFromParent();
            this->m_model->AttachToParent(parent, this->m_attachmentId, &this->m_offset, 0);
        }

        this->m_position = this->m_offset;
    } else {
        if (this->m_model && (this->m_model->m_attachParent != parent || this->m_model->m_attachmentId != static_cast<uint32_t>(this->m_attachmentId))) {
            this->DetachFromParent();
            this->m_model->AttachToParent(parent, this->m_attachmentId, nullptr, 0);
        }

        this->m_position = parent->GetAttachmentPosition(this->m_attachmentId);
    }

    this->m_transportGuid = WOWGUID();

    if (this->m_flags & 0x2000000) {
        this->m_model->m_fFlags |= 0x100000;
    }

    object->SetBoundsDirty();
    return 1;
}

// VERIFY: a7cc6925 manual
// OFFSET: 0x006FA050
void CEffect::EffectUpdate() {
    if (!(this->m_flags & 0x900)) {
        this->PostInitialize();
    }

    if ((this->m_flags & 0x40000) && this->m_lightningState > 0 && !--this->m_lightningState && this->m_model) {
        this->m_model->m_baseAlpha = 0.0f;
        this->m_model->SetEmittersEnabled(0);
        this->m_model->SetRibbonsEnabled(0);
    }

    if ((this->m_flags & 0x4000) || ((this->m_flags & 0x2) && this->m_transportGuid)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && this->m_mapEntity && this->m_model) {
            if (this->m_flags & 0x4000) {
                this->m_position = unit->GetRawPosition();

                if (unit->m_movement->m_flags & 0x40000000) {
                    this->m_position.z -= unit->m_unit->m_unitFieldHoverheight;
                }

                this->m_transportGuid.m_guid = unit->GetTransportGUID();
            }

            C44Matrix transform;
            this->TransformPosition(&transform, unit);
            CAaBox box;
            CAaSphere sphere;
            C3Vector light;

            if (this->m_model->IsLoaded(0, 0)) {
                box = this->m_model->GetBoundingBox();
                sphere = this->m_model->GetBoundingSphere();
                light = sphere.c;
            }

            light.z = 1.0f;
            this->m_model->m_fFlags |= 0x8000;
            this->m_model->m_worldTransform = transform;
            C44Matrix world = this->m_model->m_worldTransform;
            CMap::ObjectUpdate(this->m_mapEntity, world, box, sphere, light, false, 0xFFFFFFFF);
        }
    }

    if ((this->m_flags & 0x4) && this->m_expireTime && static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_expireTime) > 0) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return;
        }

        if (this->m_attachToMount && unit->m_mountModel) {
            unit->m_mountModel->m_lastAnimTime = 0;
            this->m_attachToMount = 0;
        }

        if (this->m_attachToModel && unit->m_worldModel) {
            unit->m_worldModel->m_lastAnimTime = 0;
            this->m_attachToModel = 0;
        }

        unit->UpdateBaseAnimation(0, -1);
        this->MoveToDestroyList();
    } else if (this->unk_00D0 && static_cast<int32_t>(FrameTime::s_curTimeMs - this->unk_00D0) > 0) {
        CGObject_C* object = ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_OBJECT, __FILE__, __LINE__);

        if (!object) {
            return;
        }

        object->m_scaleChangeTime = 0;
        object->m_scaleModifier = 1.0f;
        this->MoveToDestroyList();
    } else if (!(this->m_flags & 0x20) && this->m_expireTime && static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_expireTime) > 0) {
        this->MoveToDestroyList();
    }

    if ((this->m_flags & 0x100000) && !SI2::IsPlaying(&this->m_soundKitObject)) {
        this->MoveToDestroyList();
    }
}

// VERIFY: eb25cf59 manual
// OFFSET: 0x006F8970
void CEffect::UpdateWorldObject(float elapsed, uint32_t flags) {
    if ((!this->m_startTime || static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_startTime) > 0) && this->m_model) {
        this->m_model->SetAnimating(1);

        if (!(this->m_flags & 0x100) && this->m_model->IsLoaded(0, 0)) {
            CM2Model* model = this->m_model;
            CM2Model* parent = model->m_attachParent;
            uint32_t visible = flags & 1;

            if (parent) {
                model->m_fFlags = (model->m_fFlags & ~0x80u) | (visible << 7);
            } else {
                model->m_fFlags = (model->m_fFlags & ~0x8u) | (visible << 3);
            }

            if (parent) {
                model->m_fFlags = (model->m_fFlags & ~0x20000u) | (visible << 17);
            } else {
                model->m_fFlags = (model->m_fFlags & ~0x10000u) | (visible << 16);
            }

            this->m_startTime = 0;
        }
    }

    if (this->m_expireTime && static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_expireTime) > 0) {
        this->MoveToDestroyList();
    }
}

// VERIFY: d4130505 manual
// OFFSET: 0x006F9840
void CEffect::PostInitialize() {
    if (this->m_model && !this->m_model->IsLoaded(0, 0)) {
        return;
    }

    this->m_flags |= 0x800;
    int32_t hasSound = !(this->m_flags & 0x400) && this->m_kitRec && this->m_kitRec->m_soundID && this->m_kitRec->m_soundID != -1;

    if ((this->m_flags & 0x2) && !this->m_mapEntity && this->m_model) {
        CGObject_C* source = ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_OBJECT, __FILE__, __LINE__);
        C44Matrix transform;
        C44Matrix* matrix = this->TransformPosition(&transform, source);
        this->m_model->m_fFlags |= 0x8000;
        this->m_model->m_worldTransform = *matrix;
        C44Matrix world = this->m_model->m_worldTransform;
        this->m_mapEntity = CMap::ObjectCreateStatic(this->m_model, world, &CGWorldFrame::SpellEffectEnumProc, nullptr, reinterpret_cast<uintptr_t>(this));
        CAaBox box;
        CAaSphere sphere;
        C3Vector light;

        if (this->m_model->IsLoaded(0, 0)) {
            box = this->m_model->GetBoundingBox();
            sphere = this->m_model->GetBoundingSphere();
            light = sphere.c;
        }

        light.z = 1.0f;
        world = this->m_model->m_worldTransform;
        CMap::ObjectUpdate(this->m_mapEntity, world, box, sphere, light, false, 0xFFFFFFFF);
        int32_t animating = !this->m_startTime || static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_startTime) > 0;
        this->m_model->SetAnimating(animating);
        CM2Model* model = this->m_model;
        CM2Model* parent = model->m_attachParent;
        uint32_t visible = animating & 1;

        if (parent) {
            model->m_fFlags = (model->m_fFlags & ~0x80u) | (visible << 7);
        } else {
            model->m_fFlags = (model->m_fFlags & ~0x8u) | (visible << 3);
        }

        if (parent) {
            model->m_fFlags = (model->m_fFlags & ~0x20000u) | (visible << 17);
        } else {
            model->m_fFlags = (model->m_fFlags & ~0x10000u) | (visible << 16);
        }
    }

    SpellRec spell;

    if (this->m_spellId && g_spellDB.GetRecord(this->m_spellId, &spell) && (spell.m_attributesExE & 0x40000000)) {
        this->m_model->m_fFlags |= 0x4;
    }

    if (!this->UpdateAttachment()) {
        return;
    }

    if ((this->m_flags & 0x10) && this->m_kitRec && this->m_kitRec->m_shakeID > 0) {
        C3Vector position = this->TransformPositionByTransport();
        SpellVisualsPlayCameraShakeID(this->m_kitRec->m_shakeID, &position);
    }

    if (!hasSound) {
        return;
    }

    CGObject_C* object = nullptr;

    if (this->m_targetGuid) {
        object = ClntObjMgrObjectPtr(this->m_targetGuid, TYPE_OBJECT, __FILE__, __LINE__);
    } else if (this->m_sourceGuid && !(this->m_flags & 0x2000)) {
        object = ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_OBJECT, __FILE__, __LINE__);
    }

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    int32_t focus = CGObject_C::GetFocusObject() == object;
    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");
    int32_t listenerAtCharacter = 0;

    if (focus && s_listenerAtCharacter->m_intValue && !(this->m_flags & 0x200000)) {
        listenerAtCharacter = 1;
    }

    if (focus || this->m_sourceGuid == CGUnit_C::s_activeMover) {
        properties.m_priority = 110;
    }

    if (CGGameUI::SequenceMusicIsPlaying()) {
        return;
    }

    if (this->m_flags & 0x1) {
        if (!object) {
            C3Vector position = this->TransformPositionByTransport();
            SI2::PlaySoundKit(this->m_kitRec->m_soundID, &position, &this->m_soundKitObject, nullptr, 0, nullptr, 1, 0);
            return;
        }

        C3Vector position = object->GetPosition();

        if (this->m_flags & 0x1000) {
            properties.m_streaming = true;
            properties.m_maxAudibleBehavior = 0;
        }

        properties.m_noRandomStart = 1;
        SI2::PlaySoundKit(this->m_kitRec->m_soundID, &position, &this->m_soundKitObject, &properties, 0, nullptr, 1, 0);

        if (!(this->m_flags & 0x200000)) {
            SI2::AttachToObject(&this->m_soundKitObject, object->m_obj->m_guid);
        }

        return;
    }

    C3Vector position = C3Vector();
    const C3Vector* soundPosition = nullptr;
    properties.m_loopMode = 2;
    properties.m_maxAudibleBehavior = 0;

    if (listenerAtCharacter) {
        properties.m_volumeScale = 0.64999998f;
    } else {
        position = this->TransformPositionByTransport();
        soundPosition = &position;
    }

    SI2::PlaySoundKit(this->m_kitRec->m_soundID, soundPosition, &this->m_soundKitObject, &properties, 0, nullptr, 1, 0);

    if (!listenerAtCharacter && !(this->m_flags & 0x200000) && object) {
        SI2::AttachToObject(&this->m_soundKitObject, object->m_obj->m_guid);
    }
}

// VERIFY: 5e1b1086 auto
// OFFSET: 0x006F7480
static void MountTransitionSequenceCB(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (sequenceId != 127) {
        return;
    }

    CEffect* effect = reinterpret_cast<CEffect*>(static_cast<uintptr_t>(param));

    if (effect->m_mountTransition) {
        SpellVisualsFlagMountTransitionFinished(effect->m_mountTransition);
    }
}

// VERIFY: 84fadc9f auto
// OFFSET: 0x006F7D60
void CEffect::InitAtPosition(int32_t spellId, const SpellVisualKitRec* kit, const SpellVisualEffectNameRec* effectName, uint32_t flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), const C3Vector* position, uint32_t a8, WOWGUID transport) {
    this->m_effectNameRec = effectName;
    this->m_attachmentId = -1;
    this->m_sourceGuid = WOWGUID();
    this->m_targetGuid = WOWGUID();
    this->m_position = *position;
    this->m_transportGuid = transport;
    this->m_spellId = spellId;
    this->m_kitRec = kit;
    this->m_effectNameRec = effectName;
    this->m_flags = flags | 0x2;
    this->m_fadeTime = a8;
    this->m_model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0);

    if (this->m_model) {
        this->m_refCount++;
        this->m_model->SetLoadedCallback(&CEffect::CreateModelFromEffect, this);
        this->m_model->SetSequenceCallback(reinterpret_cast<void*>(callback), reinterpret_cast<uintptr_t>(this));
        this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));

        if (this->m_flags & 0x400000) {
            this->SetEmissionRateIgnoresDistance(1);
        }
    }

    this->LinkTo(&s_orphanedEffects);
}

// VERIFY: a5956734 auto
// OFFSET: 0x006F80B0
void CEffect::CreateProceduralEffectImpl(int32_t expireTime, float param, uint32_t spellId, const SpellVisualKitRec* kit, const WOWGUID* guid, uint32_t flags) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    this->LinkTo(&unit->m_effectList);
    this->m_refCount++;

    if (unit->m_mountModel && unit->m_mountModel->IsLoaded(0, 0)) {
        this->m_attachToMount = unit->m_mountModel->m_lastAnimTime == 0;

        if (param > 0.0f) {
            uint32_t duration = static_cast<uint32_t>(param * 1000.0f);
            uint32_t animId = unit->GetCurrentTorsoAnimId();
            M2SequenceInfo info;
            info.Clear();
            unit->m_mountModel->GetSequenceInfo(animId, 0, info);

            if (info.m_duration < duration) {
                duration = info.m_duration;
            }

            unit->SetBoneSequenceTimeRecursive(unit->m_mountModel, 0xFFFFFFFF, duration, 0);
        }

        unit->UpdateBoneSequenceFlags(unit->m_mountModel, 1, 0);
    } else {
        this->m_attachToMount = 1;
    }

    if (unit->m_worldModel && unit->m_worldModel->IsLoaded(0, 0)) {
        this->m_attachToModel = unit->m_worldModel->m_lastAnimTime == 0;

        if (param > 0.0f) {
            uint32_t duration = static_cast<uint32_t>(param * 1000.0f);
            uint32_t animId = unit->GetCurrentTorsoAnimId();
            M2SequenceInfo info;
            info.Clear();
            unit->m_worldModel->GetSequenceInfo(animId, 0, info);

            if (info.m_duration < duration) {
                duration = info.m_duration;
            }

            unit->SetBoneSequenceTimeRecursive(unit->m_worldModel, 0xFFFFFFFF, duration, 0);
        }

        unit->UpdateBoneSequenceFlags(unit->m_worldModel, 1, 0);
    } else {
        this->m_attachToModel = 1;
    }

    this->m_kitRec = kit;
    this->m_expireTime = expireTime;
    this->m_spellId = spellId;
    this->m_sourceGuid = *guid;
    this->m_flags |= flags | 0x4;
}

// VERIFY: fc96753b manual
// OFFSET: 0x006F82D0
void CEffect::ApplyItemVisual() {
    if (!(this->m_flags & 0x80000) || this->m_itemSlot >= 12 || !this->m_displayId) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    ItemDisplayInfoRec rec;
    bool firstSlot = this->m_itemSlot == 0;
    bool canEquip = unit->CanEquipItemsInThisForm() && (unit->m_obj->m_type & TYPE_PLAYER);

    if ((firstSlot || canEquip) && g_itemDisplayInfoDB.GetRecord(this->m_displayId, &rec)) {
        CCharacterComponent* component = unit->m_characterComponent;

        if (component) {
            component->RemoveItem(static_cast<ITEM_SLOT>(this->m_itemSlot));
            component->AddItem(static_cast<ITEM_SLOT>(this->m_itemSlot), &rec, 0);
            CGGameUI::UnitModelUpdate(this->m_sourceGuid, 3);
        }
    }
}

// VERIFY: 0b274ac7 manual
// OFFSET: 0x006F83D0
int32_t CEffect::LoadMountTransitionModel() {
    if (!this->m_displayId) {
        return 0;
    }

    CreatureDisplayInfoRec* display = g_creatureDisplayInfoDB.GetRecord(this->m_displayId);

    if (!display) {
        return 0;
    }

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(display->m_modelID);

    if (!modelData) {
        return 0;
    }

    this->m_model = World::s_m2Scene->CreateModel(modelData->m_modelName, 0);
    this->m_model->SetSequenceCallback(reinterpret_cast<void*>(&MountTransitionSequenceCB), reinterpret_cast<uintptr_t>(this));
    this->m_model->SetEventCallback(reinterpret_cast<void*>(&CEffect::EffectEvent), reinterpret_cast<uintptr_t>(this));
    CCharacterComponent::ReplaceMonsterSkin(this->m_model, display, modelData);
    this->m_model->SetBoneSequence(0xFFFFFFFF, 127, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    CM2Model* model = this->m_model;

    if (model->m_attachParent) {
        model->m_fFlags &= ~0x80;
    } else {
        model->m_fFlags &= ~0x8;
    }

    if (model->m_attachParent) {
        model->m_fFlags &= ~0x20000;
    } else {
        model->m_fFlags &= ~0x10000;
    }

    this->m_model->SetAnimating(1);
    return 1;
}

// VERIFY: b4754e47 auto
// OFFSET: 0x006F8650
void CEffect::InitProceduralEffect(int32_t expireTime, int32_t param, uint32_t spellId, const SpellVisualKitRec* kit, const WOWGUID* guid, uint32_t flags) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    this->m_expireTime = expireTime;
    this->m_spellId = spellId;
    this->m_kitRec = kit;
    this->m_sourceGuid = *guid;
    this->m_flags |= flags | 0x80000;
    this->m_pendingCacheId = param;
    this->LinkTo(&unit->m_effectList);
    this->m_refCount++;

    if (g_itemDBCache.GetRecord(this->m_pendingCacheId, *guid, &CEffect::ApplyItemVisualByItemId, this, false)) {
        CEffect::ApplyItemVisualByItemId(param, *guid, this, true);
    }
}

// VERIFY: 78e5d527 manual
// OFFSET: 0x006F8F50
void CEffect::DrawFishingString(CM2Model* model, const CM2Lighting* lighting, void* arg) {
    if (!model->HasEvent('HCC$')) {
        return;
    }

    CEffect* effect = static_cast<CEffect*>(arg);
    CGObject_C* bobber = ClntObjMgrObjectPtr(effect->m_targetGuid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (!bobber) {
        return;
    }

    C3Vector start = model->GetEventPosition('HCC$');
    C3Vector end = bobber->GetPosition();
    end.z = bobber->GetScale() * bobber->m_height * 0.2f + end.z;
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float dz = end.z - start.z;
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPC), 65);
    CGxVertexPC* vertices = reinterpret_cast<CGxVertexPC*>(g_theGxDevicePtr->BufLock(buf));
    const C3Vector& ambient = lighting->m_sunAmbient;
    CImVector color;
    color.b = static_cast<uint8_t>(static_cast<int32_t>(ambient.z > 0.0f ? (ambient.z < 1.0f ? ambient.z * 255.0f + 0.5f : 255.0f) : 0.0f));
    color.g = static_cast<uint8_t>(static_cast<int32_t>(ambient.y > 0.0f ? (ambient.y < 1.0f ? ambient.y * 255.0f + 0.5f : 255.0f) : 0.0f));
    color.r = static_cast<uint8_t>(static_cast<int32_t>(ambient.x > 0.0f ? (ambient.x < 1.0f ? ambient.x * 255.0f + 0.5f : 255.0f) : 0.0f));
    color.a = 0xFF;
    GxFormatColor(color);

    for (int32_t i = 0; i < 65; i++) {
        float t = i * 0.015625f;
        vertices[i].p.x = dx * t + start.x;
        vertices[i].p.y = dy * t + start.y;
        vertices[i].p.z = dz * t + start.z;
        vertices[i].p.z = vertices[i].p.z - sinf(t * 3.1415927f) * 0.5f;
        vertices[i].c = color;
    }

    GxBufUnlock(buf, 0);
    GxPrimVertexPtr(buf, GxVBF_PC);
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Texture0, 0);
    CGxBatch batch;
    batch.m_primType = GxPrim_LineStrip;
    batch.m_start = 0;
    batch.m_count = 65;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 64;
    g_theGxDevicePtr->Draw(&batch, 0);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 891c7da6 auto
// OFFSET: 0x006F9610
void CEffect::MountTransitionCacheCB(int32_t id, const WOWGUID& guid, void* param, bool success) {
    CEffect* effect = static_cast<CEffect*>(param);
    WOWGUID key = { static_cast<uint64_t>(effect->m_spellId) | 0x1FE0000000000000ull };
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(id, key, nullptr, nullptr, false);

    if (stats) {
        effect->m_displayId = stats->m_displayId[0];
        effect->m_pendingCacheId = 0;
        effect->LoadMountTransitionModel();
        effect->UpdateAttachment();
    }
}

// VERIFY: c6d8c4e1 auto
// OFFSET: 0x006F9670
void CEffect::AddMountTransition(uint32_t spellId, const SpellVisualKitRec* kit, const WOWGUID* guid) {
    this->m_sourceGuid = *guid;
    this->m_spellId = spellId;
    this->m_kitRec = kit;
    this->m_attachmentId = 19;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_sourceGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    this->m_pendingCacheId = 0;
    this->m_displayId = 0;

    for (uint32_t i = 0; i < 3; i++) {
        if (spell.m_effect[i] == SPELL_EFFECT_APPLY_AURA && spell.m_effectAura[i] == SPELL_AURA_MOUNTED) {
            int32_t creatureId = spell.m_effectMiscValue[i];
            this->m_pendingCacheId = creatureId;
            WOWGUID key = { static_cast<uint64_t>(spellId) | 0x1FE0000000000000ull };
            CreatureStats_C* stats = g_creatureDBCache.GetRecord(creatureId, key, &CEffect::MountTransitionCacheCB, this, false);

            if (stats) {
                this->m_displayId = stats->m_displayId[0];
                this->m_pendingCacheId = 0;
                break;
            }
        }
    }

    if (this->m_displayId ? this->LoadMountTransitionModel() : this->m_pendingCacheId) {
        this->LinkTo(&unit->m_effectList);
        this->m_refCount++;
        this->m_mountTransition = SpellVisualsStartMountTransition(unit);
        unit->StartMountTransition(this->m_mountTransition, this);
    }
}
