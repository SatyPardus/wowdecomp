#if defined(__APPLE__)

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include "Engine/Source/Os/Error.h"

char* OsGetLastErrorStr() {
    const char* text = strerror(errno);
    if (!text) {
        text = "Unknown error";
    }
    size_t len = strlen(text) + 1;
    char* out = static_cast<char*>(malloc(len));
    if (out) {
        memcpy(out, text, len);
    }
    return out;
}

void OsFreeLastErrorStr(char* str) {
    free(str);
}

#endif
