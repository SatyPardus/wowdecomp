#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

// STATIC: 0x00DCEAF8
static int32_t s_vocalErrorSounds[NUM_VOCALUISOUNDS];

// STATIC: 0x00DCEC08
static SE2SoundKitObject s_vocalErrorObject;

// VERIFY: 2b3e2ff2 manual
// OFFSET: 0x00986C90
void SI2::InitVocalErrorSounds(int32_t race, uint32_t sex) {
    memset(s_vocalErrorSounds, 0, sizeof(s_vocalErrorSounds));

    if (sex > 1) {
        return;
    }

    for (int32_t i = 0; i < g_vocalUISoundsDB.m_numRecords; i++) {
        VocalUISoundsRec* rec = (i < 0 || i >= g_vocalUISoundsDB.m_numRecords) ? nullptr : &g_vocalUISoundsDB.m_records[i];

        if (rec && rec->m_vocalUienum < NUM_VOCALUISOUNDS && rec->m_raceID == race) {
            s_vocalErrorSounds[rec->m_vocalUienum] = rec->m_normalSoundID[sex];
        }
    }
}

// VERIFY: 535f9207 auto
// OFFSET: 0x00986D00
void SI2::PlayVocalErrorSound(int32_t error) {
    static CVar* s_enableErrorSpeech = CVar::Lookup("Sound_EnableErrorSpeech");

    if (!s_enableErrorSpeech || !s_enableErrorSpeech->m_intValue || error >= static_cast<int32_t>(sizeof(s_vocalErrorSounds) / sizeof(s_vocalErrorSounds[0]))) {
        return;
    }

    if (SI2::IsPlaying(&s_vocalErrorObject)) {
        return;
    }

    int32_t soundKitId = s_vocalErrorSounds[error];
    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_group = 7;

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->m_unit->m_unitFieldBytes0[1] == 6) {
        properties.m_useDspChain = 1;
        SI2::DK_GetPlayerFilterName(player, properties.m_dspChainName, sizeof(properties.m_dspChainName));
    }

    SI2::PlaySoundKit(soundKitId, nullptr, &s_vocalErrorObject, &properties, 0, nullptr, 1, 0);
}
