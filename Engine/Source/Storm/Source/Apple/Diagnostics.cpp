#if defined(__APPLE__)
#include <cstdio>
#include <cstdlib>
#include "Engine/Source/Storm/H/storm.h"

// Foundation diagnostics. The client crash reporter and UI are separate work.
static thread_local uint32_t s_lastError;

uint32_t STORMAPI SErrGetLastError() { return s_lastError; }
void STORMAPI SErrSetLastError(uint32_t error) { s_lastError = error; }

int32_t STORMAPI SErrGetErrorStr(uint32_t error, char* buffer, uint32_t size) {
    if (!buffer || !size) { SErrSetLastError(87); return 0; }
    // Storm retains Windows error numbers. Do not interpret them as POSIX errno.
    const char* message = nullptr;
    switch (error) {
        case 0: message = "Success"; break;
        case 2: message = "File not found"; break;
        case 5: message = "Access denied"; break;
        case 38: message = "End of file"; break;
        case 87: message = "Invalid parameter"; break;
        case 108: message = "Invalid or corrupt archive"; break;
        case 120: message = "Function not implemented"; break;
    }
    if (message) { std::snprintf(buffer, size, "%s", message); }
    else { std::snprintf(buffer, size, "Storm error 0x%08x", error); }
    return 1;
}

void SErrDisplayAppFatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
    StormExitProcess(1);
}

int32_t STORMAPI SErrDisplayError(uint32_t error, const char* file, int32_t line,
    const char* description, int32_t recoverable, uint32_t exitCode, uint32_t) {
    SErrSetLastError(error);
    fprintf(stderr, "Storm error 0x%08x at %s:%d: %s\n", error,
        file ? file : "?", line, description ? description : "");
    if (!recoverable) {
        StormExitProcess(static_cast<int32_t>(exitCode));
    }
    return 1;
}

[[noreturn]] void StormExitProcess(int32_t code) { std::exit(code); }

#ifndef NDEBUG
void DebugLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void DebugLog(int32_t, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void DebugValidate(const char* expression) {
    DebugLog("Validation failed: %s\n", expression);
}
#endif
#endif
