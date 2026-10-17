#if defined(__APPLE__)

#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Log/Log.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

namespace {
void HostName(char* buffer, size_t size) {
    if (gethostname(buffer, size) != 0) {
        Blizzard::String::Copy(buffer, "<unknown>", static_cast<uint32_t>(size));
    }
}
}

char* System_Log::DefaultPreamble() {
    Blizzard::Time::TimeRec rec;
    Blizzard::Time::BreakTime(s_startTime, rec);
    char computerName[256];
    HostName(computerName, sizeof(computerName));
    char* preamble = static_cast<char*>(malloc(4096));
    Blizzard::String::Format(preamble, 4096, "\n#-----------------------------------------------------------\n# System started at %04d-%02d-%02d %02d:%02d:%02d.%04d\n# system: %s\n#-----------------------------------------------------------\n", rec.year, rec.month, rec.day, rec.hour, rec.minute, rec.second, rec.nanosecond / 100000, computerName);
    return preamble;
}

char* System_Log::DefaultDayHeader() {
    Blizzard::Time::TimeRec rec;
    Blizzard::Time::BreakTime(Blizzard::Time::GetTimestamp(), rec);
    char computerName[256];
    HostName(computerName, sizeof(computerName));
    char* header = static_cast<char*>(malloc(4096));
    Blizzard::String::Format(header, 4096, "\n#-----------------------------------------------------------\n# %04d-%02d-%02d\n# system: %s\n#-----------------------------------------------------------\n", rec.year, rec.month, rec.day, computerName);
    return header;
}

uint32_t System_Log::LogThread(void*) {
    uint32_t lastFlush = static_cast<uint32_t>(Blizzard::Time::Milliseconds());
    while (s_state > 0) {
        Blizzard::Process::Sleep(100);
        uint32_t now = static_cast<uint32_t>(Blizzard::Time::Milliseconds());
        if (now - lastFlush > 5000) {
            lastFlush = now;
            if (Acquire(0)) {
                FlushLogs(false);
                Blizzard::Lock::MutexLeave(s_mutex);
            }
        }
    }
    s_threadDone = true;
    return 0;
}

#endif
