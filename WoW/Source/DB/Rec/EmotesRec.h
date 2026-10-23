#ifndef WOW_SOURCE_DB_REC_EMOTESREC_H
#define WOW_SOURCE_DB_REC_EMOTESREC_H

#include <cstddef>
#include <cstdint>

enum EMOTESPECPROCS : int32_t {
    EMOTESPECPROC_NONE = 0,
    EMOTESPECPROC_STANDSTATEHANDLER = 1,
    EMOTESPECPROC_EMOTESTATEHANDLER = 2,
    // EMOTESPECPROC_NUMSPECPROCS = 3,
};

// Bitset indexed by EMOTESPECPROCS, separate from EmotesRec::m_emoteFlags.
enum EMOTESPECPROC_MASK : uint32_t {
    allEmoteFlags = (1u << EMOTESPECPROC_NONE)
        | (1u << EMOTESPECPROC_STANDSTATEHANDLER)
        | (1u << EMOTESPECPROC_EMOTESTATEHANDLER),
};

class SFileImpl;

class EmotesRec {
    public:
    static const int32_t s_numColumns = 7;
    static const int32_t s_rowSize = 28;
    static const bool s_generatedID = false;

    /* 0000 */ int32_t m_id;
    /* 0004 */ const char* m_emoteSlashCommand;
    /* 0008 */ int32_t m_animID;
    /* 000C */ int32_t m_emoteFlags;
    /* 0010 */ EMOTESPECPROCS m_emoteSpecProc;
    /* 0014 */ int32_t m_emoteSpecProcParam;
    /* 0018 */ int32_t m_eventSoundID;

    EmotesRec();
    ~EmotesRec();
    static const char* GetFilename();
    bool Read(SFileImpl* file, const char* stringBuffer);
};

#endif
