#if defined(__APPLE__)

#include <cstdint>
#include <cstring>
#include <SDL.h>
#include "Engine/Source/Os/OsClipboard.h"
#include "Engine/Source/Storm/H/storm.h"

char* IOsClipboardGetString(void* window) {
    (void)window;
    char* sdlText = SDL_GetClipboardText();
    if (!sdlText) {
        return nullptr;
    }
    size_t len = strlen(sdlText) + 1;
    char* result = static_cast<char*>(SMemAlloc(len, __FILE__, __LINE__, 0));
    if (result) {
        memcpy(result, sdlText, len);
    }
    SDL_free(sdlText);
    return result;
}

void OsClipboardFreeString(char* string) {
    if (string) {
        SMemFree(string, __FILE__, __LINE__, 0);
    }
}

int32_t IOsClipboardPutString(const char* string, void* window) {
    (void)window;
    return SDL_SetClipboardText(string ? string : "") == 0;
}

#endif
