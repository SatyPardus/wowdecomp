#include "UnitSound_C.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombat_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x00ADB5D8
static const uint32_t s_unitSoundChance[14] = { 70, 100, 60, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 0 };

// STATIC: 0x00CA12D0
static CVar* s_cvFootstepSounds;

// STATIC: 0x00CA12D4
static uint32_t s_lastStandSoundTime;

// STATIC: 0x00CA12D8
static TSFixedArray<DEATTHUDSOUNDINFO> s_deathThudSounds[5];

// STATIC: 0x00ADB630
static const float s_attachmentHeights[50] = { 1.0f, 1.0f, 1.0f, 1.5f, 1.5f, 1.8f, 1.8f, 0.5f, 0.5f, 1.0f, 1.0f, 2.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.5f, 0.0f, 2.0f, 1.5f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 3.0f, 1.5f, 1.5f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 2.5f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

// STATIC: 0x00ADB674
static float s_emoteSoundHeight = 2.0f;

// INLINED
inline DEATTHUDSOUNDINFO::DEATTHUDSOUNDINFO() : m_soundEntryId(0), m_soundEntryIdWater(0) {
}

// VERIFY: 1674f405 manual
// OFFSET: 0x00747760
static void UnitSoundLoadDeathThuds() {
    int32_t maxSoundId = g_terrainTypeSoundsDB.m_maxID;

    for (int32_t i = 0; i < 5; i++) {
        s_deathThudSounds[i].SetCount(maxSoundId + 1);
    }

    for (int32_t i = g_deathThudLookupsDB.m_numRecords; i; ) {
        i--;
        DeathThudLookupsRec* rec = i >= 0 && i < g_deathThudLookupsDB.m_numRecords ? &g_deathThudLookupsDB.m_records[i] : nullptr;

        if (static_cast<uint32_t>(rec->m_sizeClass) < 5 && rec->m_terrainTypeSoundID <= maxSoundId) {
            s_deathThudSounds[rec->m_sizeClass].m_data[rec->m_terrainTypeSoundID].m_soundEntryId = rec->m_soundEntryID;
            s_deathThudSounds[rec->m_sizeClass].m_data[rec->m_terrainTypeSoundID].m_soundEntryIdWater = rec->m_soundEntryIdwater;
        }
    }
}

// VERIFY: d5edf73b manual
// OFFSET: 0x00746540
void CGUnit_C::SetImpactKitEffect(bool offhand, const ATTACKROUNDINFO* info, const C3Vector* position) {
    CGUnit_C* attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info->m_attacker, TYPEMASK_UNIT, __FILE__, __LINE__));
    const CGUnitVirtualItem* weapon = attacker ? attacker->GetAttackingWeapon((info->m_hitInfo >> 2) & 1, false) : nullptr;
    const CGUnitVirtualItem* parry = this->GetParryingItem(offhand);

    if (parry) {
        int32_t self = s_activeMover == info->m_attacker || s_activeMover == info->m_target;
        SI2::PlayParrySound(weapon, parry, info->m_hitInfo & 0x200, position, self);
    }
}

// VERIFY: 37583731 manual
// OFFSET: 0x00747860
void UnitSoundInitialize() {
    UnitSoundLoadDeathThuds();

    s_cvFootstepSounds = CVar::Register("FootstepSounds", nullptr, 0, "1", nullptr, 5, false, nullptr, false);
    s_lastStandSoundTime = FrameTime::s_curTimeMs;

    for (int32_t i = 0; i < g_soundEntriesDB.m_numRecords; i++) {
        SoundEntriesRec* rec = i >= 0 && i < g_soundEntriesDB.m_numRecords ? &g_soundEntriesDB.m_records[i] : nullptr;

        if (!SStrCmp(rec->m_name, "SpiritWolf (DONOTRENAME)", STORM_MAX_STR)) {
            CGUnit_C::s_dismountSoundKitId = rec->m_id;
            return;
        }
    }
}

// VERIFY: e952682c manual
// OFFSET: 0x00747810
void UnitSoundShutdown() {
    for (int32_t i = 0; i < 5; i++) {
        s_deathThudSounds[i].Clear();
    }
}

// VERIFY: 7e015e11 auto
// OFFSET: 0x007461E0
int32_t GetSoundID(const CreatureSoundDataRec* soundData, uint32_t soundType) {
    if (!soundData) {
        return 0;
    }

    switch (soundType) {
        case 0:
            return soundData->m_soundExertionID;
        case 1:
            return soundData->m_soundExertionCriticalID;
        case 2:
        case 13:
            return soundData->m_soundInjuryID;
        case 3:
            return soundData->m_soundInjuryCriticalID;
        case 4:
            return soundData->m_soundStunID;
        case 5:
            return soundData->m_soundStandID;
        case 7:
            return soundData->m_soundWingFlapID;
        case 8:
            return soundData->m_soundAlertID;
        case 9:
            return soundData->m_soundInjuryCrushingBlowID;
        case 10:
            return soundData->m_soundWingGlideID;
        case 11:
            return soundData->m_soundJumpStartID;
        case 12:
            return soundData->m_soundJumpEndID;
        default:
            return 0;
    }
}

// VERIFY: dab78e5a auto
// OFFSET: 0x007464A0
int32_t CheckUnitSoundTimer(uint32_t soundType) {
    if (soundType == 5) {
        if (static_cast<int32_t>(FrameTime::s_curTimeMs - s_lastStandSoundTime - 10000) < 0) {
            return 0;
        }

        s_lastStandSoundTime = FrameTime::s_curTimeMs;
    }

    return 1;
}

// VERIFY: fcb45083 auto
// OFFSET: 0x007462E0
void CGUnit_C::PlayFidgetSound(uint32_t index) {
    CreatureSoundDataRec* soundData = this->GetSoundData();

    if (!soundData || index >= 5) {
        return;
    }

    int32_t soundKitId = soundData->m_soundFidget[index];

    if (!soundKitId) {
        return;
    }

    C3Vector position = this->GetPosition();
    position.z += 2.0f;
    SI2::PlaySoundKit(soundKitId, &position, nullptr, nullptr, 0, nullptr, 1, 0);
}

// VERIFY: 339e65d9 auto
// OFFSET: 0x00746610
void CGUnit_C::PlayDeathThud() {
    C3Vector position = this->GetPosition();
    int32_t liquidType;
    float liquidLevel;
    int32_t liquidFlags;
    int32_t inLiquid = World::QueryObjectLiquid(this->m_worldObject, &liquidType, &liquidLevel, &liquidFlags);

    if (inLiquid && liquidLevel - position.z > 2.0f) {
        return;
    }

    uint32_t size = this->GetUnitSize();

    if (size >= 5) {
        return;
    }

    TerrainTypeRec* terrainType = g_terrainTypeDB.GetRecord(this->m_terrainTypeId);

    if (!terrainType || terrainType->m_soundID >= static_cast<int32_t>(s_deathThudSounds[size].m_count)) {
        return;
    }

    DEATTHUDSOUNDINFO* info = &s_deathThudSounds[size].m_data[terrainType->m_soundID];
    int32_t soundKitId = inLiquid ? info->m_soundEntryIdWater : info->m_soundEntryId;

    if (!soundKitId) {
        int32_t defaults[5] = { 907, 912, 917, 922, 927 };
        soundKitId = defaults[size];
        SysMsgPrintf(STATUS_WARNING, 2, "INVALID DEATH THUD SOUND: DEFAULTING TO ID %d", soundKitId);
    }

    SI2::PlaySoundKit(soundKitId, &position, nullptr, nullptr, 0, nullptr, 1, 0);
}

// VERIFY: 8792f0f0 auto
// OFFSET: 0x007471A0
void CGUnit_C::HandlePlayStandSound(uint32_t eventId, int32_t unused_a3) {
    switch (eventId) {
        case 0x31444624:
            this->PlayFidgetSound(0);
            break;
        case 0x32444624:
            this->PlayFidgetSound(1);
            break;
        case 0x33444624:
            this->PlayFidgetSound(2);
            break;
        case 0x34444624:
            this->PlayFidgetSound(3);
            break;
        case 0x35444624:
            this->PlayFidgetSound(4);
            break;
        case 0x36444624:
        case 0x37444624:
        case 0x38444624:
        case 0x39444624:
            this->GetSoundData();
            break;
        case 0x58444624:
            this->PlayStandSound();
            break;
    }
}

// VERIFY: 73f75c40 manual
// OFFSET: 0x00747240
void CGUnit_C::HandleFootfallAnimEvent(const C3Vector& position) {
    if ((this->m_unit->m_unitFieldBytes1[2] & 0x2) || (this->m_movementData.m_flags & MOVEMENTFLAG_HOVER)) {
        return;
    }

    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state) {
        return;
    }

    if ((this->m_obj->m_type & TYPEMASK_PLAYER) && (static_cast<CGPlayer_C*>(this)->m_player->m_playerFlags & 0x10)) {
        return;
    }

    this->PlayFoleySound();

    CreatureSoundDataRec* soundData = this->GetSoundData();

    if (soundData && soundData->m_soundFootstepID && s_cvFootstepSounds && s_cvFootstepSounds->m_intValue) {
        SI2::PlayFootstepSound(soundData->m_soundFootstepID, &position, this->m_terrainTypeId, this->IsSplashing(position), this->GetUnitSize() >= 3, this);
    }
}

// VERIFY: 63312a72 manual
// OFFSET: 0x00747310
void CGUnit_C::PlayUnitSound(uint32_t soundType, int32_t force) {
    if (!force && static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 101) >> 32) > s_unitSoundChance[soundType]) {
        return;
    }

    if (soundType == 5) {
        if (static_cast<int32_t>(FrameTime::s_curTimeMs - s_lastStandSoundTime - 10000) < 0) {
            return;
        }

        s_lastStandSoundTime = FrameTime::s_curTimeMs;
    }

    int32_t soundKitId = GetSoundID(this->GetSoundData(), soundType);

    if (!soundKitId) {
        return;
    }

    C3Vector position = this->GetPosition();

    if (CGUnit_C::s_activeMover == this->m_obj->m_guid || this->IsActivePlayer()) {
        static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

        int32_t listenerAtCharacter = s_listenerAtCharacter->m_intValue;
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_group = this->GetUnitSoundKitIndex(soundType);

        if (listenerAtCharacter) {
            properties.m_volumeScale = 0.64999998f;
        }

        properties.m_priority = 110;
        SI2::PlaySoundKit(soundKitId, listenerAtCharacter ? nullptr : &position, nullptr, &properties, 0, nullptr, 1, 0);
    } else {
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();

        if (soundType == 7) {
            properties.m_maxAudibleBehavior = 0;
        }

        properties.m_group = this->GetUnitSoundKitIndex(soundType);
        SI2::PlaySoundKit(soundKitId, &position, nullptr, &properties, 0, nullptr, 1, 0);
    }
}

// VERIFY: 12476da9 auto
// OFFSET: 0x00746340
void CGUnit_C::KillCreatureLoopSound() {
    SI2::StopSound(&this->m_loopSound, 0, 0.5f, 1);
}

// VERIFY: f678b1e1 auto
// OFFSET: 0x00746360
bool CGUnit_C::GetWeaponSwingType(bool offhand, int32_t* swingType) const {
    if (!this->GetVirtualItemDisplayID(offhand == false)) {
        *swingType = 0;
        return true;
    }

    const CGUnitVirtualItem* item = this->GetVirtualItem(offhand == false, false);

    if (!item || item->m_classID != 2) {
        return false;
    }

    const ItemSubClassRec* subClass = SDBItemSubclassGetSubClassRec(2, item->m_subclassID);

    if (subClass) {
        *swingType = subClass->m_weaponSwingSize;
    }

    return subClass != nullptr;
}

// VERIFY: f4e40cf7 auto
// OFFSET: 0x007465E0
void CGUnit_C::PlayStandSound() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        this->PlayUnitSound(5, 0);
    }
}

// VERIFY: d0b7eaae manual
// OFFSET: 0x00746720
void CGUnit_C::PlaySplashSound(const C3Vector* position) {
    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

    int32_t activeMover;
    int32_t listenerAtCharacter;

    if (s_activeMover != this->m_obj->m_guid) {
        activeMover = 0;
        listenerAtCharacter = 0;
    } else {
        activeMover = 1;
        listenerAtCharacter = s_listenerAtCharacter->m_intValue ? 1 : 0;
    }

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (activeMover) {
        properties.m_priority = 110;

        if (listenerAtCharacter) {
            properties.m_volumeScale = 0.64999998f;
        }
    }

    properties.m_group = 8;
    SI2::PlaySoundKit(this->m_splashSoundKitId, listenerAtCharacter ? nullptr : position, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: 5cdf8669 auto
// OFFSET: 0x00746900
int32_t CGUnit_C::GetNewNPCSoundCount(int32_t count, int32_t* soundKitId, int32_t* soundIndex) const {
    if (!this->m_displayInfo) {
        return -1;
    }

    NPCSoundsRec* sounds = g_npcSoundsDB.GetRecord(this->m_displayInfo->m_npcsoundID);

    if (!sounds || !sounds->m_soundID[0]) {
        return -1;
    }

    if (count < 5) {
        *soundKitId = sounds->m_soundID[0];
        *soundIndex = -1;
        return count + 1;
    }

    if (count - 5 < SI2::GetNumSoundsInSoundKit(sounds->m_soundID[2])) {
        *soundIndex = count - 5;
        *soundKitId = sounds->m_soundID[2];
        return count + 1;
    }

    *soundKitId = sounds->m_soundID[0];
    *soundIndex = -1;
    return 0;
}

// VERIFY: e49e815c auto
// OFFSET: 0x007469C0
void CGUnit_C::PlayAISound(int32_t soundKitId) {
    SI2::StopSound(&this->m_npcSound, 1, -1.0f, 1);

    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

    if (s_activeMover == this->m_obj->m_guid && s_listenerAtCharacter->m_intValue) {
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_volumeScale = 0.64999998f;
        properties.m_priority = 110;
        SI2::PlaySoundKit(soundKitId, nullptr, &this->m_aiSound, &properties, 0, nullptr, 1, 0);
    } else {
        C3Vector position = this->GetPosition();
        SI2::PlaySoundKit(soundKitId, &position, &this->m_aiSound, nullptr, 0, nullptr, 1, 0);
        SI2::AttachToObject(&this->m_aiSound, this->m_obj->m_guid);
    }
}

// VERIFY: 055965ad auto
// OFFSET: 0x00746AD0
void CGUnit_C::PlayEmoteStateSound(const C3Vector* position) {
    EmotesRec* emote = g_emotesDB.GetRecord(this->m_unit->m_unitNpcEmotestate);

    if (emote && emote->m_emoteSpecProc == 2) {
        SI2::PlaySoundKit(emote->m_eventSoundID, position, nullptr, nullptr, 0, nullptr, 1, 0);
    }
}

// VERIFY: 13e3e5f1 auto
// OFFSET: 0x00746BD0
C3Vector CGUnit_C::GetAttachmentPosition(uint32_t attachId, const C3Vector* offset) {
    CM2Model* model = this->m_worldModel;

    if (model && model->IsLoaded(0, 0) && model->HasAttachment(attachId)) {
        if (!offset || (offset->x == 0.0f && offset->y == 0.0f && offset->z == 0.0f)) {
            return model->GetAttachmentPosition(attachId);
        }

        return *offset * model->GetAttachmentWorldTransform(attachId);
    }

    C3Vector position(0.0f, 0.0f, 0.0f);

    if (offset) {
        position = *offset;
    }

    if (attachId <= 49) {
        position.z = s_attachmentHeights[attachId] + position.z;
    }

    if (!offset || (offset->x == 0.0f && offset->y == 0.0f && offset->z == 0.0f)) {
        C3Vector base = this->m_vehiclePassenger ? this->GetModelPosition() : this->GetPosition();
        position.x = base.x + position.x;
        position.y = base.y + position.y;
        position.z = base.z + position.z;
    } else {
        position *= this->m_vehiclePassenger ? this->GetModelMatrix() : this->GetMatrix();
    }

    return position;
}

// VERIFY: 972d06e0 auto
// OFFSET: 0x00747000
void CGUnit_C::PlayTextEmoteSound(uint32_t emote, uint32_t emoteNum) {
    CVar* cvar = CVar::Lookup("Sound_EnableEmoteSounds");

    if (!cvar || !cvar->m_intValue) {
        return;
    }

    if (!this->IsActivePlayer()) {
        WOWGUID guid = this->m_obj->m_guid;

        if (!CGGameUI::IsInParty(guid)) {
            return;
        }
    }

    int32_t soundKitId = SI2::GetTextEmoteSoundID(emote, this->GetDisplayRace(), this->GetVoiceSex());

    if (this->IsShapeShifted()) {
        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(this->GetShapeshiftForm());

        if (form && (form->m_flags & 0x1000)) {
            return;
        }
    }

    this->PlayEmoteSound(soundKitId, false, emoteNum);
}

// VERIFY: 9c20bca8 auto
// OFFSET: 0x00746280
int32_t CreatureSoundDataGetFidgetSound(const CreatureSoundDataRec* rec, uint32_t index) {
    if (rec && index < 5) {
        return rec->m_soundFidget[index];
    }

    return 0;
}

// VERIFY: 9a579a40 auto
// OFFSET: 0x007462A0
float CreatureSoundDataGetFidgetDelayMin(const CreatureSoundDataRec* rec) {
    if (rec) {
        return rec->m_fidgetDelaySecondsMin;
    }

    return 0.0f;
}

// VERIFY: e79c80f1 auto
// OFFSET: 0x007462C0
float CreatureSoundDataGetFidgetDelayMax(const CreatureSoundDataRec* rec) {
    if (rec) {
        return rec->m_fidgetDelaySecondsMax;
    }

    return 0.0f;
}

// VERIFY: b66560c6 auto
// OFFSET: 0x007463E0
uint32_t CGUnit_C::GetImpactType() const {
    if (this->m_soundData && this->m_soundData->m_creatureImpactType < 4) {
        return s_impactTypes[this->m_soundData->m_creatureImpactType];
    }

    return 0;
}

// VERIFY: 22593312 auto
// OFFSET: 0x007464D0
void CGUnit_C::PlayFoleySound() {
    static CVar* s_enableArmorFoley = CVar::Lookup("Sound_EnableArmorFoleySoundForOthers");

    if (s_enableArmorFoley && s_enableArmorFoley->m_intValue) {
        C3Vector position = this->GetPosition();
        SI2::PlayFoleySound(this->m_modelData->m_foleyMaterialID, &position, 0);
    }
}

// VERIFY: 034a2c24 auto
// OFFSET: 0x007467F0
void CGUnit_C::CheckLoopSound() {
    CreatureSoundDataRec* soundData = this->GetSoundData();

    if (this->m_unit->m_unitFieldHealth > 0 && !(this->m_unit->m_unitFieldFlags2 & 0x1) && soundData && soundData->m_loopSoundID && !SI2::IsPlaying(&this->m_loopSound)) {
        C3Vector position = this->GetPosition();

        if (SI2::IsPlaying(&this->m_loopSound) || SI2::IsTooCloseToExistingSoundKit(soundData->m_loopSoundID, &position)) {
            return;
        }

        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_loopMode = 1;
        properties.m_maxAudibleBehavior = 0;
        properties.m_streaming = true;
        SI2::PlaySoundKit(soundData->m_loopSoundID, &position, &this->m_loopSound, &properties, 0, nullptr, 1, 0);
        SI2::AttachToObject(&this->m_loopSound, this->m_obj->m_guid);
        return;
    }

    SI2::StopSound(&this->m_loopSound, 0, 0.5f, 1);
}

// VERIFY: c9c84a2c manual
// OFFSET: 0x00746B30
int32_t CGUnit_C::GetUnitSoundKitIndex(uint32_t soundType) const {
    switch (soundType) {
        case 0:
        case 1:
            return (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x20)) ? 15 : 9;

        case 2:
        case 3:
        case 9:
            return (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x20)) ? 16 : 11;

        default:
            return 0;
    }
}

// VERIFY: bae0a2ba manual
// OFFSET: 0x00746D60
void CGUnit_C::PlayEmoteSound(int32_t soundKitId, bool checkCVars, int32_t fileIndex) {
    if ((this->m_obj->m_type & TYPEMASK_PLAYER) && this->m_unit->m_unitFieldHealth <= 0) {
        return;
    }

    if (checkCVars) {
        static CVar* s_enableEmoteSounds = CVar::Lookup("Sound_EnableEmoteSounds");

        if (!s_enableEmoteSounds || !s_enableEmoteSounds->m_intValue) {
            return;
        }

        static CVar* s_enablePetSounds = CVar::Lookup("Sound_EnablePetSounds");

        if (s_enablePetSounds && !s_enablePetSounds->m_intValue && this->m_unit->m_unitFieldPetnumber) {
            return;
        }
    }

    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");
    int32_t listenerAtCharacter = s_listenerAtCharacter->m_intValue;
    int32_t local;
    int32_t atCharacter;

    if (CGUnit_C::s_activeMover != this->m_obj->m_guid && !this->IsActivePlayer()) {
        local = 0;
        atCharacter = 0;
    } else {
        local = 1;
        atCharacter = listenerAtCharacter ? 1 : 0;
    }

    if ((this->m_obj->m_type & TYPEMASK_PLAYER) && static_cast<CGPlayer_C*>(this)->m_dance) {
        return;
    }

    C3Vector position;
    CM2Model* model = this->m_worldModel;

    if (model && model->IsLoaded(0, 0) && model->HasAttachment(17)) {
        position = model->GetAttachmentPosition(17);
    } else {
        C3Vector offset(0.0f, 0.0f, s_emoteSoundHeight);
        C3Vector base = this->m_vehiclePassenger ? this->GetModelPosition() : this->GetPosition();
        position = base + offset;
    }

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_fileIndex = fileIndex;
    properties.m_suppressorMode = 2;

    if (local) {
        properties.m_priority = 110;

        if (atCharacter) {
            properties.m_volumeScale = 0.64999998f;
        }
    }

    if (soundKitId != 6576 && this->m_unit->m_unitFieldBytes0[1] == 6) {
        properties.m_useDspChain = 1;
        SI2::DK_GetPlayerFilterName(this, properties.m_dspChainName, sizeof(properties.m_dspChainName));
    }

    SI2::PlaySoundKit(soundKitId, atCharacter ? nullptr : &position, &this->m_reactionSound, &properties, 0, nullptr, 1, 0);

    if (!atCharacter) {
        SI2::AttachToObject(&this->m_reactionSound, this->m_obj->m_guid);
    }
}

// VERIFY: b56823a7 manual
// OFFSET: 0x007470D0
void CGUnit_C::PlayDismountSound() {
    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

    int32_t activeMover;
    int32_t listenerAtCharacter;

    if (s_activeMover != this->m_obj->m_guid) {
        activeMover = 0;
        listenerAtCharacter = 0;
    } else {
        activeMover = 1;
        listenerAtCharacter = s_listenerAtCharacter->m_intValue ? 1 : 0;
    }

    C3Vector position = this->GetPosition();

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (activeMover) {
        properties.m_priority = 110;

        if (listenerAtCharacter) {
            properties.m_volumeScale = 0.64999998f;
        }
    }

    SI2::PlaySoundKit(s_dismountSoundKitId, listenerAtCharacter ? nullptr : &position, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: 82755190 auto
// OFFSET: 0x007474B0
void CGUnit_C::OnAiReaction(int32_t reaction) {
    if (reaction == 4 && this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x1000000)) {
        return;
    }

    if (!this->m_soundData || SI2::IsPlaying(&this->m_aiSound)) {
        return;
    }

    SE2SoundKitObject* object = &this->m_npcSound;

    if (SI2::IsPlaying(object) && reaction <= static_cast<int32_t>(this->m_reactionSoundPriority)) {
        return;
    }

    this->m_reactionSoundPriority = reaction;
    static CVar* s_enablePetSounds = CVar::Lookup("Sound_EnablePetSounds");
    int32_t soundKitId;

    switch (this->m_reactionSoundPriority) {
    case 0:
        if (s_enablePetSounds && !s_enablePetSounds->m_intValue && this->m_unit->m_unitFieldPetnumber) {
            return;
        }

        soundKitId = this->m_soundData->m_soundAggroID;
        break;

    case 1:
        if (s_enablePetSounds && !s_enablePetSounds->m_intValue && this->m_unit->m_unitFieldPetnumber) {
            return;
        }

        soundKitId = this->m_soundData->m_soundPetOrderID;
        break;

    case 2:
        if (s_enablePetSounds && !s_enablePetSounds->m_intValue && this->m_unit->m_unitFieldPetnumber) {
            return;
        }

        soundKitId = this->m_soundData->m_soundPetAttackID;
        break;

    case 4:
        soundKitId = this->m_soundData->m_soundDeathID;
        break;

    default:
        return;
    }

    C3Vector position = this->GetAttachmentPosition(17, nullptr);
    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");
    int32_t activeMover;
    int32_t listenerAtCharacter;

    if (CGUnit_C::s_activeMover == this->m_obj->m_guid) {
        activeMover = 1;
        listenerAtCharacter = s_listenerAtCharacter->m_intValue ? 1 : 0;
    } else {
        activeMover = 0;
        listenerAtCharacter = 0;
    }

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (activeMover) {
        properties.m_priority = 110;
        CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

        if (player && player->m_unit->m_unitFieldBytes0[1] == 6) {
            properties.m_useDspChain = 1;
            SI2::DK_GetPlayerFilterName(player, properties.m_dspChainName, sizeof(properties.m_dspChainName));
        }

        if (listenerAtCharacter) {
            properties.m_volumeScale = 0.64999998f;
        }
    }

    SI2::PlaySoundKit(soundKitId, listenerAtCharacter ? nullptr : &position, object, &properties, 0, nullptr, 1, 0);
}
