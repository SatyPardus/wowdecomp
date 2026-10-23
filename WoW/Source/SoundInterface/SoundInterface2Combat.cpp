#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/SoundInterface2Combat.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

// STATIC: 0x00B4AE7C
static TSFixedArray<IMPACTSOUNDARRAY> s_impactSounds;

// STATIC: 0x00B4AE88
static int32_t s_swingSounds[NUM_WEAPONSWINGSOUNDTYPES][NUM_SWINGTYPES];

// VERIFY: 647f0380 auto
// OFFSET: 0x007DA1F0
IMPACTSOUND::IMPACTSOUND() {
    this->m_soundId = 0;
    this->m_critSoundId = 0;
}

// VERIFY: 2250fb03 manual
// OFFSET: 0x004D0380
static void InitWeaponImpactSound(WeaponImpactSoundsRec* rec) {
    if (!s_impactSounds.m_count) {
        return;
    }

    IMPACTSOUNDARRAY* sounds = &s_impactSounds.m_data[rec->m_weaponSubClassID];

    for (int32_t i = 0; i < 10; i++) {
        sounds->m_sounds[i][rec->m_parrySoundType].m_soundId = rec->m_impactSoundID[i];
        sounds->m_sounds[i][rec->m_parrySoundType].m_critSoundId = rec->m_critImpactSoundID[i];
    }
}

// VERIFY: d720637a auto
// OFFSET: 0x004D04E0
void SI2::InitWeaponSwingCombatSounds() {
    for (int32_t i = g_weaponSwingSounds2DB.m_numRecords; i;) {
        i--;

        WeaponSwingSounds2Rec* rec = (i < 0 || i >= g_weaponSwingSounds2DB.m_numRecords) ? nullptr : &g_weaponSwingSounds2DB.m_records[i];

        if (rec && rec->m_swingType < NUM_WEAPONSWINGSOUNDTYPES && rec->m_crit < NUM_SWINGTYPES) {
            s_swingSounds[rec->m_swingType][rec->m_crit != SWING_NORMAL] = rec->m_soundID;
        }
    }
}

// VERIFY: df290633 manual
// OFFSET: 0x004D0540
void SI2::PlayParrySound(const CGUnitVirtualItem* weapon, const CGUnitVirtualItem* defender, int32_t crit, const C3Vector* position, int32_t self) {
    if (!s_impactSounds.m_count) {
        return;
    }

    int32_t subclass;
    PARRYMATERIALS metal;

    if (weapon) {
        subclass = weapon->m_soundOverrideSubclassID;

        if (subclass == 0xFF) {
            subclass = weapon->m_subclassID;
        }

        metal = static_cast<PARRYMATERIALS>(CGItem_C::IsMetal(weapon->m_material));
    } else {
        metal = PARRYMATERIAL_WOOD;
        subclass = ClientDBGetUnarmedWeapon();
    }

    int32_t type = 6;

    if (defender) {
        if (defender->m_classID == 2) {
            type = 6 - CGItem_C::IsMetal(defender->m_material);
        } else if (defender->m_classID == 4) {
            type = 4 - CGItem_C::IsMetal(defender->m_material);
        }
    }

    IMPACTSOUND* sound = &s_impactSounds.m_data[subclass].m_sounds[type][metal];

    C3Vector soundPosition;
    soundPosition.x = position->x;
    soundPosition.y = position->y;
    soundPosition.z = position->z + 2.0f;

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_group = 14;

    if (self) {
        properties.m_priority = 110;
    }

    SI2::PlaySoundKit(crit != 0 ? sound->m_critSoundId : sound->m_soundId, &soundPosition, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: 4ec94dcc manual
// OFFSET: 0x004D0660
void SI2::PlayHitSound(const CGUnitVirtualItem* weapon, int32_t impactType, int32_t crit, const C3Vector* position, int32_t self) {
    if (!s_impactSounds.m_count) {
        return;
    }

    int32_t subclass;
    PARRYMATERIALS metal;

    if (weapon) {
        subclass = weapon->m_soundOverrideSubclassID;

        if (subclass == 0xFF) {
            subclass = weapon->m_subclassID;
        }

        metal = static_cast<PARRYMATERIALS>(CGItem_C::IsMetal(weapon->m_material));
    } else {
        metal = PARRYMATERIAL_WOOD;
        subclass = ClientDBGetUnarmedWeapon();
    }

    IMPACTSOUND* sound = &s_impactSounds.m_data[subclass].m_sounds[impactType][metal];

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_group = 14;

    if (self) {
        properties.m_priority = 110;
    }

    SI2::PlaySoundKit(crit != 0 ? sound->m_critSoundId : sound->m_soundId, position, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: 1275dcb8 manual
// OFFSET: 0x004D0720
int32_t SI2::PlayWeaponSwooshSound(WEAPONSWING_SOUNDTYPES type, int32_t crit, const C3Vector* position, int32_t quiet, int32_t self) {
    if (type >= NUM_WEAPONSWINGSOUNDTYPES) {
        return 0;
    }

    float volume = quiet ? 0.5f : 1.0f;

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_volume = volume;
    properties.m_group = 10;

    if (self) {
        properties.m_priority = 110;
    }

    return SI2::PlaySoundKit(s_swingSounds[type][crit != SWING_NORMAL], position, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: 4281e82d manual
// OFFSET: 0x004D07B0
int32_t SI2::PlaySheatheSound(const CGUnitVirtualItem* item, int32_t sheathe, const C3Vector* position, int32_t self) {
    if (!item) {
        return 0;
    }

    MaterialRec* material = g_materialDB.GetRecord(item->m_material);

    if (!material) {
        return 0;
    }

    C3Vector soundPosition;
    soundPosition.x = position->x;
    soundPosition.y = position->y;
    soundPosition.z = position->z + 2.0f;

    int32_t soundKitId = sheathe ? material->m_sheatheSoundID : material->m_unsheatheSoundID;

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (self) {
        properties.m_priority = 110;
    }

    return SI2::PlaySoundKit(soundKitId, &soundPosition, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: c557bfee auto
// OFFSET: 0x004D0850
int32_t SI2::PlayMissedSound(bool oneHand, const C3Vector* position, int32_t self) {
    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (self) {
        properties.m_priority = 110;
    }

    return SI2::PlaySoundKit(oneHand ? "(DONOTRENAME)Combat Miss 1H" : "(DONOTRENAME)Combat Miss 2H", position, nullptr, &properties);
}

// VERIFY: 3aee2ee4 manual
// OFFSET: 0x004D0B70
void SI2::InitWeaponImpactCombatSounds() {
    s_impactSounds.SetCount(ClientDBGetNumWeaponSubClasses());

    for (int32_t i = g_weaponImpactSoundsDB.m_numRecords; i;) {
        i--;

        WeaponImpactSoundsRec* rec = (i < 0 || i >= g_weaponImpactSoundsDB.m_numRecords) ? nullptr : &g_weaponImpactSoundsDB.m_records[i];
        InitWeaponImpactSound(rec);
    }
}
