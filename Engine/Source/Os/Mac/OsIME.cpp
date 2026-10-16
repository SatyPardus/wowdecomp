#if defined(__APPLE__)

#include <cstdint>
#include <cstring>
#include <SDL.h>
#include "Engine/Source/Os/OsIME.h"

static uint32_t s_enableCount;
static char s_composition[1024];

void IOsIMEUpdateComposition(const char* text) {
    strncpy(s_composition, text ? text : "", sizeof(s_composition) - 1);
    s_composition[sizeof(s_composition) - 1] = '\0';
}

int32_t OsIMEGetLanguageMode() {
    return 0;
}

int32_t OsIMEGetCompositionString(char* buffer, uint32_t bufferSize) {
    if (!buffer || !bufferSize) {
        return 0;
    }
    strncpy(buffer, s_composition, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    return s_composition[0] != '\0';
}

int32_t OsIMEGetCompositionResult(char* buffer, uint32_t bufferSize) {
    if (buffer && bufferSize) {
        buffer[0] = '\0';
    }
    return 0;
}

int32_t OsIMEGetClauseInfo(uint32_t& clauseStart, uint32_t& clauseEnd, uint32_t& cursor) {
    clauseStart = 0;
    clauseEnd = static_cast<uint32_t>(strlen(s_composition));
    cursor = clauseEnd;
    return s_composition[0] != '\0';
}

void OsIMEEnable(int32_t enable) {
    if (enable) {
        if (++s_enableCount == 1) {
            SDL_StartTextInput();
        }
    } else if (s_enableCount && --s_enableCount == 0) {
        SDL_StopTextInput();
    }
}

void OsIMESync() {
    if (s_enableCount) {
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }
}

void OsIMESetLanguageMode(int32_t native) {
    (void)native;
}

void OsIMEInitialize() {
    s_enableCount = 0;
    s_composition[0] = '\0';
}

void OsIMEDestroy() {
    SDL_StopTextInput();
    s_enableCount = 0;
    s_composition[0] = '\0';
}

int32_t OsIMEGetCandidates(uint32_t listMask, uint32_t& pageStart, uint32_t& pageSize, uint32_t& count, uint32_t& selection, TSGrowableArray<OsIMECandidate>& candidates) {
    (void)listMask;
    candidates.Clear();
    pageStart = 0;
    pageSize = 0;
    count = 0;
    selection = 0;
    return 0;
}

#endif
