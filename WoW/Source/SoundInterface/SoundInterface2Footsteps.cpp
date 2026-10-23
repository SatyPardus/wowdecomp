#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/SoundInterface2Footsteps.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

static const float MaximumFoostepDistance = 20.0f;

// STATIC: 0x00B4AE04
static HASHKEY_NONE s_footstepKey;

// STATIC: 0x00B4AE10
static TSHashTable<FOOTSTEPSNDCACHE, HASHKEY_NONE> s_footstepSounds;

// VERIFY: a8ad1552 auto
// OFFSET: 0x004CEE50
bool CGUnit_C::IsActivePlayer() {
    return this->m_obj->m_guid == ClntObjMgrGetActivePlayer();
}

// VERIFY: 1432f0e0 manual
// OFFSET: 0x004CF060
void SI2::FreeFootstepSounds() {
    s_footstepSounds.Clear();
}

// VERIFY: 8151ef99 manual
// OFFSET: 0x004CF100
static uint32_t GetFootstepSound(int32_t terrainType, int32_t footstepId, int32_t splash) {
    TerrainTypeRec* terrain = g_terrainTypeDB.GetRecord(terrainType);

    if (!terrain) {
        return 0;
    }

    int32_t index = terrain->m_soundID;
    FOOTSTEPSNDCACHE* cache = s_footstepSounds.Ptr(footstepId, s_footstepKey);

    if (!cache) {
        return 0;
    }

    if (splash) {
        return cache->m_splashSounds[index];
    }

    return cache->m_sounds[index];
}

// VERIFY: 508a3422 manual
// OFFSET: 0x004CF170
void SI2::PlayFootstepSound(int32_t footstepId, const C3Vector* position, int32_t terrainType, int32_t splash, int32_t loud, CGUnit_C* unit) {
    if (!unit) {
        return;
    }

    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

    if (!s_listenerAtCharacter) {
        return;
    }

    uint32_t soundKitId = GetFootstepSound(terrainType, footstepId, splash);

    if (!soundKitId) {
        soundKitId = GetFootstepSound(0, footstepId, splash);

        if (!soundKitId) {
            return;
        }
    }

    SE2SoundKitProperties properties;

    if ((unit->m_obj->m_type & TYPE_PLAYER) && unit->IsActivePlayer() && s_listenerAtCharacter->m_intValue) {
        properties.ResetToDefaults();
        properties.m_maxDistance = loud ? 50.0f : MaximumFoostepDistance;
        properties.m_volumeScale = 0.65f;
        properties.m_group = 17;
        properties.m_priority = 115;
        SI2::PlaySoundKit(soundKitId, nullptr, nullptr, &properties, 0, nullptr, 1, 0);
    } else if ((unit->m_obj->m_type & TYPE_PLAYER) && unit->IsActivePlayer()) {
        properties.ResetToDefaults();
        properties.m_group = 17;
        properties.m_priority = 115;
        SI2::PlaySoundKit(soundKitId, position, nullptr, &properties, 0, nullptr, 1, 0);
    } else {
        properties.ResetToDefaults();
        properties.m_group = 13;
        properties.m_maxAudibleBehavior = 0;
        SI2::PlaySoundKit(soundKitId, position, nullptr, &properties, 0, nullptr, 1, 0);
    }
}

// VERIFY: 25b83170 auto
// OFFSET: 0x004CF490
FOOTSTEPSNDCACHE::FOOTSTEPSNDCACHE() {
    this->m_sounds.Clear();
    this->m_splashSounds.Clear();
}

// VERIFY: b9aab4ae manual
// OFFSET: 0x004CF990
void SI2::InitFootstepSounds() {
    s_footstepSounds.Clear();

    uint32_t count = g_terrainTypeSoundsDB.m_maxID + 1;

    for (int32_t i = g_footstepTerrainLookupDB.m_numRecords; i;) {
        i--;

        FootstepTerrainLookupRec* rec = (i < 0 || i >= g_footstepTerrainLookupDB.m_numRecords) ? nullptr : &g_footstepTerrainLookupDB.m_records[i];

        if (!rec || rec->m_terrainSoundID < 0) {
            continue;
        }

        FOOTSTEPSNDCACHE* cache = s_footstepSounds.Ptr(rec->m_creatureFootstepID, s_footstepKey);

        if (!cache) {
            cache = s_footstepSounds.New(rec->m_creatureFootstepID, s_footstepKey, 0, 0);
            cache->m_sounds.SetCount(count);
            cache->m_splashSounds.SetCount(count);

            for (uint32_t j = 0; j < count; j++) {
                cache->m_sounds[j] = 0;
                cache->m_splashSounds[j] = 0;
            }
        }

        cache->m_sounds[rec->m_terrainSoundID] = rec->m_soundID;
        cache->m_splashSounds[rec->m_terrainSoundID] = rec->m_soundIdsplash;
    }
}
