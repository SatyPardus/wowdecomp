#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "Engine/Source/BlizzardCore/Source/System/Debug/Debug.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Using/Using.h"

// STATIC: 0x00AB9358
static System_Debug::UsingDebug s_usingDebug;

// STATIC: 0x00AB90A0
Blizzard::Debug::AssertCallback System_Debug::s_AssertCallback = Blizzard::Debug::DefaultCallback;

// STATIC: 0x00B33310
char System_Debug::s_CanonBuffer[1024];

// STATIC: 0x00AB9348
char* System_Debug::s_CanonHead = System_Debug::s_CanonBuffer;

// STATIC: 0x00AB934C
char* System_Debug::s_CanonTail = System_Debug::s_CanonBuffer;

// VERIFY: abd76e5c auto
// OFFSET: 0x00426FE0
void Blizzard::Debug::SetAssertHandler(AssertCallback handler) {
    System_Debug::s_AssertCallback = handler;
}

// VERIFY: d388a718 manual
// OFFSET: 0x00433420
char* Blizzard::Debug::CanonicalStr(const char* src, char* dst, uint32_t maxchars) {
    char buffer[512];
    Blizzard::String::MakeUnivPath(src, buffer, sizeof(buffer));
    Blizzard::String::Copy(dst, buffer, maxchars);

    for (const char* prefix = System_Debug::s_CanonHead; prefix < System_Debug::s_CanonTail;) {
        const char* current = prefix;
        uint32_t length = prefix ? static_cast<uint32_t>(strlen(prefix)) : 0;
        prefix += length + 1;

        if (current && Blizzard::String::EqualI(buffer, current, length)) {
            Blizzard::String::Copy(dst, buffer + length, maxchars);
            break;
        }
    }

    return dst;
}

// VERIFY: db22e266 manual
// OFFSET: 0x00426FF0
void Blizzard::Debug::StderrOutput(const char* text) {
    if (text) {
        fputs(text, stderr);
    }
}

// VERIFY: 409e37fb manual
// OFFSET: 0x0042CE40
void Blizzard::Debug::Printf(void (*output)(const char*), const char* format, ...) {
    char buffer[4096];

    if (format) {
        va_list args;
        va_start(args, format);
        vsnoprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        buffer[sizeof(buffer) - 1] = '\0';
    } else {
        buffer[0] = '\0';
    }

    output(buffer);
}

// INLINED
System_Debug::UsingDebug::UsingDebug() : Blizzard::Using::Use("Blizzard::Debug") {
    System_Using::Intern(this->m_name)->shutdownproc = System_Debug::ShutdownProc;
    Blizzard::Using::DependsOn("Blizzard::Debug", "Blizzard::Memory");
}
