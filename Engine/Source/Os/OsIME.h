#ifndef ENGINE_SOURCE_OS_OSIME_H
#define ENGINE_SOURCE_OS_OSIME_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

enum OS_IME_LANGUAGEMODE : int32_t {
    OS_IME_MODE_ROMAN = 0,
    OS_IME_MODE_JAPANESE = 1,
    OS_IME_MODE_KOREAN = 2,
    OS_IME_MODE_CHINESE = 3,
};

struct OsIMECandidate {
    /* 0000 */ char m_text[1024];
};

OS_IME_LANGUAGEMODE OsIMEGetLanguageMode();
int32_t OsIMEGetCompositionString(char* buffer, uint32_t bufferSize);
int32_t OsIMEGetCompositionResult(char* buffer, uint32_t bufferSize);
int32_t OsIMEGetClauseInfo(uint32_t& clauseStart, uint32_t& clauseEnd, uint32_t& cursor);
void OsIMEEnable(int32_t enable);
void OsIMESync();
void OsIMESetLanguageMode(int32_t native);
void OsIMEInitialize();
void OsIMEDestroy();
int32_t OsIMEGetCandidates(uint32_t listMask, uint32_t& pageStart, uint32_t& pageSize, uint32_t& count, uint32_t& selection, TSGrowableArray<OsIMECandidate>& candidates);

#endif
