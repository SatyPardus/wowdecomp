#if defined(__APPLE__)

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "Engine/Source/BlizzardCore/Source/System/Debug/Debug.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

static bool s_continueAfterAssert;

void Blizzard::Debug::Breakpoint() {
#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
    __builtin_debugtrap();
#else
    raise(SIGTRAP);
#endif
#else
    raise(SIGTRAP);
#endif
}

void Blizzard::Debug::DefaultCallback(const char* expression, const char* message, const char* fileName, uint32_t lineNumber) {
    char text[2048];
    char canonical[256];
    char directory[512];
    Blizzard::Time::TimeRec rec;
    Blizzard::Time::BreakTime(Blizzard::Time::GetTimestamp(), rec);
    Blizzard::String::Format(text, sizeof(text), "%04d/%02d/%02d %02d:%02d:%02d  %s(%d) : Assertion failed: %s  \"%s\"\n", rec.year, rec.month, rec.day, rec.hour, rec.minute, rec.second, CanonicalStr(fileName, canonical, sizeof(canonical)), lineNumber, expression, message);
    fprintf(stderr, "%s", text);
    Blizzard::Process::GetProcessDirectory(directory, sizeof(directory));
    Blizzard::String::Append(directory, "Errors", sizeof(directory));
    Blizzard::File::CreateDirectory(directory, true);
    Blizzard::String::Append(directory, "/Assert.log", sizeof(directory));
    FILE* stream = fopen(directory, "a");
    if (stream) {
        fprintf(stream, "%s", text);
        fclose(stream);
    }
    if (!s_continueAfterAssert) {
        abort();
    }
}

void System_Debug::ShutdownProc() {}

#endif
