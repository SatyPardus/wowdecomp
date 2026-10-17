#if defined(__APPLE__)
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include "Engine/Source/BlizzardCore/Source/System/Debug/Debug.h"

void Blizzard::Debug::Breakpoint() { raise(SIGTRAP); }
void Blizzard::Debug::DefaultCallback(const char* expression, const char* message, const char* file, uint32_t line) {
    fprintf(stderr, "%s:%u: Assertion failed: %s (%s)\n", file ? file : "?", line,
        expression ? expression : "", message ? message : "");
    std::abort();
}
void System_Debug::ShutdownProc() {}
#endif
