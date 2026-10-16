#ifndef WOW_SOURCE_OBJECT_CGPETITION_H
#define WOW_SOURCE_OBJECT_CGPETITION_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

class CDataStore;

class CGPetition {
    public:
    static const uint32_t s_cacheVersion = 1;

    /* 0000 */ int32_t m_id;
    /* 0004 */ uint8_t unused_0004 [4];
    /* 0008 */ WOWGUID m_ownerGuid;
    /* 0010 */ char m_title [256];
    /* 0110 */ char m_bodyText [4096];
    /* 1110 */ uint32_t m_minSignatures;
    /* 1114 */ uint32_t m_maxSignatures;
    /* 1118 */ uint32_t m_deadline;
    /* 111C */ uint32_t m_issueDate;
    /* 1120 */ uint32_t m_allowedGuildId;
    /* 1124 */ uint32_t m_allowedClasses;
    /* 1128 */ uint32_t m_allowedRaces;
    /* 112C */ uint16_t m_allowedGenders;
    /* 112E */ uint8_t unused_112E [2];
    /* 1130 */ uint32_t m_allowedMinLevel;
    /* 1134 */ uint32_t m_allowedMaxLevel;
    /* 1138 */ char m_choiceText [10][64];
    /* 13B8 */ uint32_t m_numChoices;
    /* 13BC */ uint32_t m_staticType;
    /* 13C0 */ uint32_t m_muid;

    CGPetition();
    CGPetition& operator=(const CGPetition& source);
    void Pack(CDataStore* msg);
    void Unpack(CDataStore* msg);
};

#endif
