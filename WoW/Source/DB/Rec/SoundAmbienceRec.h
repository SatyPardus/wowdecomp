#ifndef WOW_SOURCE_DB_REC_SOUNDAMBIENCEREC_H
#define WOW_SOURCE_DB_REC_SOUNDAMBIENCEREC_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/Types.h"

class SFileImpl;

class SoundAmbienceRec {
    public:
    static const int32_t s_numColumns = 3;
    static const int32_t s_rowSize = 12;
    static const bool s_generatedID = false;

    /* 0000 */ int32_t m_id;
    /* 0004 */ int32_t m_ambienceID [NUM_AMBIENCES];

    SoundAmbienceRec();
    ~SoundAmbienceRec();
    static const char* GetFilename();
    bool Read(SFileImpl* file, const char* stringBuffer);
};

#endif
