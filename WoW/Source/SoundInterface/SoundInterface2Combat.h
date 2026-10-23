#ifndef WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2COMBAT_H
#define WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2COMBAT_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/Types.h"

struct IMPACTSOUND {
    /* 0000 */ int32_t m_soundId;
    /* 0004 */ int32_t m_critSoundId;

    IMPACTSOUND();
};

struct IMPACTSOUNDARRAY {
    /* 0000 */ IMPACTSOUND m_sounds[10][NUM_PARRYMATERIALS];
};

#endif
