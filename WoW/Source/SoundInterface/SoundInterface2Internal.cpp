#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include "WoW/Source/SoundInterface/SoundInterface2Internal.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/SoundInterface/SoundInterface2ZoneSounds.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"

// STATIC: 0x00B4AD3C
static TSGrowableArray<VSOUNDKITDEF*> s_soundKitDefs;

// STATIC: 0x00B4AD4C
static TSHashTable<SOUNDKITLOOKUP, HASHKEY_STRI> s_soundKitLookup;

// STATIC: 0x00B4AD74
static TSGrowableArray<WorldStateZoneSoundOverride*> s_worldStateZoneSoundOverrides;

// STATIC: 0x00B4AD84
static TSHashTable<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI> s_zoneSoundOverrides;

// VERIFY: 14413abb auto
// OFFSET: 0x004CB8C0
VSOUNDKITDEF::~VSOUNDKITDEF() {
    for (int32_t i = 0; i < 10; i++) {
        if (this->m_files[i]) {
            free(this->m_files[i]);
            this->m_files[i] = nullptr;
        }
    }
}

// VERIFY: cca97730 auto
// OFFSET: 0x004CB8F0
void VSOUNDKITDEF::ResetWeights() {
    for (int32_t i = 0; i < 10; i++) {
        this->m_weights[i] = 0;
    }
}

// VERIFY: eb26f6c8 auto
// OFFSET: 0x004CBA50
VSOUNDKITDEF::VSOUNDKITDEF() {
    this->m_numFiles = 0;
    this->m_advancedId = 0;

    for (int32_t i = 0; i < 10; i++) {
        this->m_files[i] = nullptr;
    }

    for (int32_t i = 0; i < 10; i++) {
        this->m_weights[i] = 0;
    }

    this->m_lastIndex = -1;
    this->m_pendingIndex = -1;
    this->m_downloadPending = 0;
}

// VERIFY: 229de6de manual
// OFFSET: 0x004CBAC0
const char* VSOUNDKITDEF::SelectFile(uint32_t mode, int32_t* index, int32_t fileIndex, int32_t priority) {
    if (mode > 2) {
        return nullptr;
    }

    if (this->m_numFiles <= 0) {
        return nullptr;
    }

    if (this->m_downloadPending) {
        if (this->m_lastIndex < 0) {
            this->m_lastIndex = 0;
        }

        return this->m_files[this->m_lastIndex];
    }

    if (fileIndex > -1) {
        int32_t selected = this->m_numFiles - 1;

        if (fileIndex < selected) {
            selected = fileIndex;
        }

        this->m_lastIndex = selected;

        if (index) {
            *index = selected;
        }

        return this->m_files[selected];
    }

    if (this->m_numFiles == 1) {
        this->m_lastIndex = 0;

        if (index) {
            *index = 0;
        }

        return this->m_files[0];
    }

    int32_t total;

    for (int32_t pass = 0;; pass++) {
        total = 0;

        if (pass == 2) {
            this->m_lastIndex = -1;
        } else if (pass >= 3) {
            return nullptr;
        }

        for (int32_t i = 0; i < this->m_numFiles; i++) {
            if (i != this->m_lastIndex) {
                total += this->m_weights[i];
            }
        }

        if (total) {
            break;
        }

        if (mode == 0) {
            for (int32_t i = 0; i < 10; i++) {
                this->m_weights[i] = 1;
            }
        } else if (mode == 1 || mode == 2) {
            for (int32_t i = 0; i < this->m_numFiles; i++) {
                this->m_weights[i] = this->m_frequencies[i];
            }
        } else {
            break;
        }
    }

    int32_t roll = mode ? static_cast<int32_t>((static_cast<uint64_t>(static_cast<uint32_t>(total)) * CRandom::uint32_(g_rndSeed)) >> 32) : 0;

    int32_t state = 1;

    if (this->m_pendingIndex >= 0) {
        state = SFile::FileIsLocal(this->m_files[this->m_pendingIndex], priority);

        if (state && state != 2) {
            this->m_pendingIndex = -1;
        }
    }

    int32_t selected = -1;
    int32_t i;

    for (i = 0; i < this->m_numFiles; i++) {
        if (i != this->m_lastIndex) {
            roll -= this->m_weights[i];
        }

        if (roll < 0) {
            break;
        }
    }

    if (i < this->m_numFiles) {
        if (SFile::IsStreamingMode()) {
            state = SFile::FileIsLocal(this->m_files[i], priority);
        }

        if (state && state != 2) {
            this->m_weights[i]--;
            selected = i;
        } else {
            if (this->m_pendingIndex < 0) {
                SFile::MakeFileLocal(this->m_files[i], priority, false);
                this->m_pendingIndex = i;
            }

            int32_t fallback = i;

            if (SFile::IsStreamingMode()) {
                for (int32_t j = i - 1; j >= 0; j--) {
                    int32_t local = SFile::FileIsLocal(this->m_files[j], priority);

                    if (local && local != 2) {
                        fallback = j;
                        break;
                    }
                }
            }

            selected = fallback;
        }
    } else {
        selected = 0;
    }

    if (mode == 1) {
        for (int32_t k = 0; k < 10; k++) {
            this->m_weights[k] = 0;
        }
    }

    this->m_lastIndex = selected;

    if (index) {
        *index = selected;
    }

    return this->m_files[selected];
}

// VERIFY: b9dea2b7 auto
// OFFSET: 0x004CBDB0
void SI2::CreateSoundGroups() {
    for (uint32_t i = 0; i < s_soundKitDefs.Count(); i++) {
        VSOUNDKITDEF* def = s_soundKitDefs[i];

        if (def && def->m_name) {
            def->m_loadSoundGroup = SE2::CreateSoundGroup(def->m_name, 3);
            def->m_soundGroup = SE2::CreateSoundGroup(def->m_name, 3);
        }
    }
}

// VERIFY: c9a76bad manual
// OFFSET: 0x004CBE00
void SI2::ReleaseSoundGroups() {
    for (uint32_t i = 0; i < s_soundKitDefs.Count(); i++) {
        VSOUNDKITDEF* def = s_soundKitDefs[i];

        if (def) {
            if (def->m_loadSoundGroup) {
                SE2::ReleaseSoundGroup(def->m_loadSoundGroup);
                def->m_loadSoundGroup = nullptr;
            }

            if (def->m_soundGroup) {
                SE2::ReleaseSoundGroup(def->m_soundGroup);
                def->m_soundGroup = nullptr;
            }
        }
    }
}

// VERIFY: b21cd78a manual
// OFFSET: 0x004CBE70
int32_t SI2::UpdateZoneSoundsForWorldState(int32_t areaId, int32_t zoneId, int32_t wmoAreaId, int32_t wmoGroupAreaId, int32_t inside) {
    int32_t count = s_worldStateZoneSoundOverrides.Count();
    int32_t found = 0;

    if (inside) {
        for (int32_t i = 0; i < count; i++) {
            WorldStateZoneSoundOverride* entry = s_worldStateZoneSoundOverrides[i];

            if ((wmoAreaId > 0 && wmoAreaId == entry->m_wmoAreaId) || (wmoGroupAreaId > 0 && wmoGroupAreaId == entry->m_wmoAreaId)) {
                if (GetWorldState(entry->m_worldStateId) == entry->m_worldStateValue) {
                    SI2::SetZoneReverb(1, s_worldStateZoneSoundOverrides[i]->m_soundProviderPreferencesId);
                    SI2::SetZoneAmbienceID(1, s_worldStateZoneSoundOverrides[i]->m_soundAmbienceId);
                    SI2::SetZoneIntroMusicID(6, s_worldStateZoneSoundOverrides[i]->m_zoneIntroMusicId);
                    SI2::SetZoneMusicID(1, s_worldStateZoneSoundOverrides[i]->m_zoneMusicId);
                    found = 1;

                    if (wmoGroupAreaId == s_worldStateZoneSoundOverrides[i]->m_wmoAreaId) {
                        return 1;
                    }
                }
            }
        }

        if (found) {
            return 1;
        }
    } else if (wmoAreaId > 0 || wmoGroupAreaId > 0) {
        int32_t best = 4;

        for (int32_t i = 0; i < count; i++) {
            WorldStateZoneSoundOverride* entry = s_worldStateZoneSoundOverrides[i];
            int32_t rank = 4;

            if (areaId > 0 && areaId == entry->m_areaId) {
                rank = 3;
            }

            if (zoneId > 0 && zoneId == entry->m_areaId) {
                rank = 2;
            }

            if (wmoGroupAreaId > 0 && wmoGroupAreaId == entry->m_wmoAreaId) {
                rank = 0;
            }

            if (rank < best && GetWorldState(entry->m_worldStateId) == entry->m_worldStateValue) {
                SI2::SetZoneReverb(1, s_worldStateZoneSoundOverrides[i]->m_soundProviderPreferencesId);
                SI2::SetZoneAmbienceID(1, s_worldStateZoneSoundOverrides[i]->m_soundAmbienceId);
                SI2::SetZoneIntroMusicID(6, s_worldStateZoneSoundOverrides[i]->m_zoneIntroMusicId);
                SI2::SetZoneMusicID(1, s_worldStateZoneSoundOverrides[i]->m_zoneMusicId);
                found = 1;
                best = rank;

                if (!rank) {
                    return 1;
                }
            }
        }

        if (found) {
            return 1;
        }
    } else {
        for (int32_t i = 0; i < count; i++) {
            WorldStateZoneSoundOverride* entry = s_worldStateZoneSoundOverrides[i];

            if ((areaId > 0 && areaId == entry->m_areaId) || (zoneId > 0 && zoneId == entry->m_areaId)) {
                if (GetWorldState(entry->m_worldStateId) == entry->m_worldStateValue) {
                    SI2::SetZoneReverb(1, s_worldStateZoneSoundOverrides[i]->m_soundProviderPreferencesId);
                    SI2::SetZoneAmbienceID(1, s_worldStateZoneSoundOverrides[i]->m_soundAmbienceId);
                    SI2::SetZoneIntroMusicID(6, s_worldStateZoneSoundOverrides[i]->m_zoneIntroMusicId);
                    SI2::SetZoneMusicID(1, s_worldStateZoneSoundOverrides[i]->m_zoneMusicId);
                    found = 1;

                    if (zoneId == s_worldStateZoneSoundOverrides[i]->m_areaId) {
                        return 1;
                    }
                }
            }
        }

        if (found) {
            return 1;
        }
    }

    SI2::SetZoneReverb(1, -1);
    SI2::SetZoneAmbienceID(1, -1);
    SI2::SetZoneIntroMusicID(6, -1);
    SI2::SetZoneMusicID(1, -1);
    return 0;
}

// VERIFY: 6f1b3998 auto
// OFFSET: 0x004CC180
VSOUNDKITDEF* SI2::GetSoundKitDef(int32_t soundKitId) {
    if (soundKitId >= static_cast<int32_t>(s_soundKitDefs.Count()) || soundKitId <= 0) {
        return nullptr;
    }

    return s_soundKitDefs[soundKitId];
}

// VERIFY: 76ad8733 manual
// OFFSET: 0x004CC1B0
void SI2::InitWorldStateZoneSoundOverrides() {
    uint32_t count = g_worldStateZoneSoundsDB.m_numRecords;
    s_worldStateZoneSoundOverrides.SetCount(count);
    memset(s_worldStateZoneSoundOverrides.Ptr(), 0, sizeof(WorldStateZoneSoundOverride*) * count);

    for (uint32_t i = 0; i < s_worldStateZoneSoundOverrides.Count(); i++) {
        s_worldStateZoneSoundOverrides[i] = static_cast<WorldStateZoneSoundOverride*>(SMemAlloc(sizeof(WorldStateZoneSoundOverride), __FILE__, __LINE__, 0));

        WorldStateZoneSoundsRec* rec = &g_worldStateZoneSoundsDB.m_records[i];

        if (!rec) {
            continue;
        }

        s_worldStateZoneSoundOverrides[i]->m_areaId = rec->m_areaID;
        s_worldStateZoneSoundOverrides[i]->m_wmoAreaId = rec->m_wmoareaID;
        s_worldStateZoneSoundOverrides[i]->m_worldStateId = rec->m_worldStateID;
        s_worldStateZoneSoundOverrides[i]->m_worldStateValue = rec->m_worldStateValue;
        s_worldStateZoneSoundOverrides[i]->m_zoneIntroMusicId = rec->m_zoneIntroMusicID;
        s_worldStateZoneSoundOverrides[i]->m_zoneMusicId = rec->m_zoneMusicID;
        s_worldStateZoneSoundOverrides[i]->m_soundAmbienceId = rec->m_soundAmbienceID;
        s_worldStateZoneSoundOverrides[i]->m_soundProviderPreferencesId = rec->m_soundProviderPreferencesID;
    }
}

// VERIFY: 21e9b526 manual
// OFFSET: 0x004CC2F0
void SI2::FreeWorldStateZoneSoundOverrides() {
    for (uint32_t i = 0; i < s_worldStateZoneSoundOverrides.Count(); i++) {
        if (s_worldStateZoneSoundOverrides[i]) {
            delete s_worldStateZoneSoundOverrides[i];
        }
    }

    s_worldStateZoneSoundOverrides.Clear();
}

// VERIFY: 6baa66d7 manual
// OFFSET: 0x004CC620
void SI2::FreeSoundKitDefs() {
    for (uint32_t i = 0; i < s_soundKitDefs.Count(); i++) {
        if (s_soundKitDefs[i]) {
            delete s_soundKitDefs[i];
        }
    }

    s_soundKitDefs.Clear();
    s_soundKitLookup.Clear();
}

// VERIFY: b1c83b1a manual
// OFFSET: 0x004CC6E0
void SI2::FreeZoneSoundOverrides() {
    s_zoneSoundOverrides.Clear();
}

// VERIFY: 47a145e0 auto
// OFFSET: 0x004CCA30
int32_t SI2::GetZoneSoundOverride(int32_t mapId, int32_t chunkX, int32_t chunkY, int32_t subchunkX, int32_t subchunkY, int32_t* zoneIntroMusicId, int32_t* zoneMusicId, int32_t* soundAmbienceId, int32_t* soundProviderPreferencesId) {
    char key[64];
    SStrPrintf(key, sizeof(key), "%010d_%010d_%010d_%010d_%010d", mapId, chunkX, chunkY, subchunkX, subchunkY);

    ZONESOUNDOVERRIDELOOKUP* lookup = s_zoneSoundOverrides.Ptr(key);

    if (!lookup) {
        return 0;
    }

    *zoneIntroMusicId = lookup->m_zoneIntroMusicId;
    *zoneMusicId = lookup->m_zoneMusicId;
    *soundAmbienceId = lookup->m_soundAmbienceId;
    *soundProviderPreferencesId = lookup->m_soundProviderPreferencesId;
    return 1;
}

// VERIFY: 6d857c47 auto
// OFFSET: 0x004CCAA0
int32_t SI2::LookupSoundKitId(const char* name) {
    SOUNDKITLOOKUP* lookup = s_soundKitLookup.Ptr(name);

    if (!lookup) {
        return 0;
    }

    return lookup->m_soundKitId;
}

// VERIFY: 9c167174 manual
// OFFSET: 0x004CD4A0
void SI2::InitSoundKitLookup() {
    uint32_t count = g_soundEntriesDB.m_maxID + 1;
    s_soundKitDefs.SetCount(count);
    memset(s_soundKitDefs.Ptr(), 0, sizeof(VSOUNDKITDEF*) * count);

    for (int32_t i = 0; i < g_soundEntriesDB.m_numRecords; i++) {
        SoundEntriesRec* rec = &g_soundEntriesDB.m_records[i];

        if (!rec) {
            continue;
        }

        VSOUNDKITDEF* def = new (SMemAlloc(sizeof(VSOUNDKITDEF), __FILE__, __LINE__, 0)) VSOUNDKITDEF();

        for (int32_t j = 0; j < 10; j++) {
            const char* file = rec->m_file[j];

            if (!file || !*file) {
                break;
            }

            size_t length = strlen(file) + 1;

            if (*rec->m_directoryBase) {
                length += strlen(rec->m_directoryBase) + 1;
            }

            def->m_files[j] = static_cast<char*>(calloc(1, length));

            if (*rec->m_directoryBase) {
                bool slash = rec->m_directoryBase[strlen(rec->m_directoryBase) - 1] == '\\';
                sprintf(def->m_files[j], "%s", rec->m_directoryBase);

                if (!slash) {
                    strcat(def->m_files[j], "\\");
                }
            }

            strcat(def->m_files[j], rec->m_file[j]);

            def->m_numFiles++;
            def->m_frequencies[j] = rec->m_freq[j];
        }

        def->m_flags = rec->m_flags;
        def->m_name = rec->m_name;
        def->m_minDistance = rec->m_minDistance;
        def->m_distanceCutoff = rec->m_distanceCutoff;
        def->m_volume = rec->m_volumeFloat;
        def->m_id = rec->m_id;
        def->m_advancedId = rec->m_soundEntriesAdvancedID;
        def->m_advanced = nullptr;

        if (def->m_advancedId) {
            def->m_advanced = g_soundEntriesAdvancedDB.GetRecord(def->m_advancedId);
        }

        s_soundKitDefs[rec->m_id] = def;

        SOUNDKITLOOKUP* lookup = s_soundKitLookup.Ptr(rec->m_name);

        if (!lookup) {
            lookup = s_soundKitLookup.New(rec->m_name, 0, 0);
        }

        lookup->m_soundKitId = rec->m_id;
    }
}

// VERIFY: 17a0acc4 manual
// OFFSET: 0x004CD870
void SI2::InitZoneSoundOverrides() {
    char key[64];

    for (int32_t i = 0; i < g_worldChunkSoundsDB.m_numRecords; i++) {
        WorldChunkSoundsRec* rec = &g_worldChunkSoundsDB.m_records[i];

        if (!key) {
            continue;
        }

        SStrPrintf(key, sizeof(key), "%010d_%010d_%010d_%010d_%010d", rec->m_id, rec->m_chunkX, rec->m_chunkY, rec->m_subchunkX, rec->m_subchunkY);

        ZONESOUNDOVERRIDELOOKUP* lookup = s_zoneSoundOverrides.Ptr(key);

        if (!lookup) {
            lookup = s_zoneSoundOverrides.New(key, 0, 0);
        }

        lookup->m_zoneIntroMusicId = rec->m_zoneIntroMusicID;
        lookup->m_zoneMusicId = rec->m_zoneMusicID;
        lookup->m_soundAmbienceId = rec->m_soundAmbienceID;
        lookup->m_soundProviderPreferencesId = rec->m_soundProviderPreferencesID;
    }
}
