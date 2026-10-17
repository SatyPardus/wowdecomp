#if defined(__APPLE__)
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/System/Log/Log.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

namespace System_Log {
void GetDefaultDirectory(char* buffer, uint32_t capacity) {
    const char* configured = std::getenv("WOW_LOG_ROOT");
    std::error_code error;
    std::string directory = configured && *configured ? configured :
        (std::filesystem::temp_directory_path(error) / "wowdecomp").string();
    directory += '/';
    Blizzard::String::Copy(buffer, directory.c_str(), capacity);
}
static char* Header(Blizzard::Time::Timestamp timestamp) {
    Blizzard::Time::TimeRec date;
    Blizzard::Time::BreakTime(timestamp, date);
    char hostname[256]{};
    if (gethostname(hostname, sizeof(hostname) - 1)) { Blizzard::String::Copy(hostname, "<unknown>", sizeof(hostname)); }
    auto* result = static_cast<char*>(malloc(1024));
    Blizzard::String::Format(result, 1024, "\n# %04d-%02d-%02d %02d:%02d:%02d UTC\n# system: %s\n", date.year, date.month,
        date.day, date.hour, date.minute, date.second, hostname);
    return result;
}
char* DefaultPreamble() { return Header(s_startTime); }
char* DefaultDayHeader() { return Header(Blizzard::Time::GetTimestamp()); }
uint32_t LogThread(void*) {
    int64_t lastFlush = Blizzard::Time::Milliseconds();
    while (s_state > 0) {
        Blizzard::Thread::Sleep(100);
        const int64_t now = Blizzard::Time::Milliseconds();
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
}
#endif
