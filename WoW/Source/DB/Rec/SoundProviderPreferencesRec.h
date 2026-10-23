#ifndef WOW_SOURCE_DB_REC_SOUNDPROVIDERPREFERENCESREC_H
#define WOW_SOURCE_DB_REC_SOUNDPROVIDERPREFERENCESREC_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/Types.h"

class SFileImpl;

class SoundProviderPreferencesRec {
    public:
    static const int32_t s_numColumns = 24;
    static const int32_t s_rowSize = 96;
    static const bool s_generatedID = false;

    /* 0000 */ int32_t m_id;
    /* 0004 */ const char* m_description;
    /* 0008 */ int32_t m_flags;
    /* 000C */ SNDROOMTYPE m_eaxenvironmentSelection;
    /* 0010 */ float m_eaxdecayTime;
    /* 0014 */ float m_eax2EnvironmentSize;
    /* 0018 */ float m_eax2EnvironmentDiffusion;
    /* 001C */ int32_t m_eax2Room;
    /* 0020 */ int32_t m_eax2RoomHf;
    /* 0024 */ float m_eax2DecayHfratio;
    /* 0028 */ int32_t m_eax2Reflections;
    /* 002C */ float m_eax2ReflectionsDelay;
    /* 0030 */ int32_t m_eax2Reverb;
    /* 0034 */ float m_eax2ReverbDelay;
    /* 0038 */ float m_eax2RoomRolloff;
    /* 003C */ float m_eax2AirAbsorption;
    /* 0040 */ int32_t m_eax3RoomLf;
    /* 0044 */ float m_eax3DecayLfratio;
    /* 0048 */ float m_eax3EchoTime;
    /* 004C */ float m_eax3EchoDepth;
    /* 0050 */ float m_eax3ModulationTime;
    /* 0054 */ float m_eax3ModulationDepth;
    /* 0058 */ float m_eax3Hfreference;
    /* 005C */ float m_eax3Lfreference;

    SoundProviderPreferencesRec();
    ~SoundProviderPreferencesRec();
    static const char* GetFilename();
    bool Read(SFileImpl* file, const char* stringBuffer);
};

#endif
