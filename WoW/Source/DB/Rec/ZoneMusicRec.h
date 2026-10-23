#ifndef WOW_SOURCE_DB_REC_ZONEMUSICREC_H
#define WOW_SOURCE_DB_REC_ZONEMUSICREC_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/Types.h"

class SFileImpl;

class ZoneMusicRec {
    public:
    static const int32_t s_numColumns = 8;
    static const int32_t s_rowSize = 32;
    static const bool s_generatedID = false;

    /* 0000 */ int32_t m_id;
    /* 0004 */ const char* m_setName;
    /* 0008 */ int32_t m_silenceIntervalMin [NUM_AMBIENCES];
    /* 0010 */ int32_t m_silenceIntervalMax [NUM_AMBIENCES];
    /* 0018 */ int32_t m_sounds [NUM_AMBIENCES];

    ZoneMusicRec();
    ~ZoneMusicRec();
    static const char* GetFilename();
    bool Read(SFileImpl* file, const char* stringBuffer);
};

#endif
