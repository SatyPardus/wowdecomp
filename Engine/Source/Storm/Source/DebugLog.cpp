#include <cstdarg>
#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "Engine/Source/Storm/H/storm.h"

#ifndef NDEBUG
void DebugLog(const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

#if defined(_WIN32)
    OutputDebugStringA(buffer);
#else
    fputs(buffer, stderr);
#endif

    size_t length = strlen(buffer);

    if (length == 0 || buffer[length - 1] != '\n') {
#if defined(_WIN32)
        OutputDebugStringA("\n");
#else
        fputc('\n', stderr);
#endif
    }
}

void DebugLog(int32_t target, const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DebugLog("%s", buffer);
}

void DebugValidate(const char* expression) {
    DebugLog("Validation failed: %s", expression);
}
#endif
