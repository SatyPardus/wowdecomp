#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <mutex>
static int fopen_s(FILE** result, const char* path, const char* mode) {
    *result = std::fopen(path, mode);
    return *result ? 0 : errno;
}
#endif
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Log/Log.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

// STATIC: 0x00AB93EC
static System_Log::UsingLog s_usingLog;

// STATIC: 0x00B33C59
bool System_Log::s_threadRunning;

// STATIC: 0x00B33C5A
bool System_Log::s_threadDone;

// STATIC: 0x00B33C5C
Blizzard::Lock::Mutex System_Log::s_mutex;

// STATIC: 0x00B33C74
#if defined(__APPLE__)
std::atomic<int32_t> System_Log::s_state{};
#else
int32_t System_Log::s_state;
#endif

// STATIC: 0x00B33C78
int32_t System_Log::s_prefCount;

// STATIC: 0x00B33C7C
uint32_t System_Log::s_stringPoolSize;

// STATIC: 0x00B33C80
Blizzard::Time::Timestamp System_Log::s_startTime;

// STATIC: 0x00B33C88
Blizzard::Thread::ThreadRecord* System_Log::s_thread;

// STATIC: 0x00B33C8C
uint32_t System_Log::s_stringPoolUsed;

// STATIC: 0x00B33C90
System_Log::Pref* System_Log::s_Prefs;

// STATIC: 0x00B33C94
char* System_Log::s_stringPool;

// STATIC: 0x00B33C98
System_Log::LogFile System_Log::s_logs[16];

// VERIFY: ecc5fd89 auto
// OFFSET: 0x004283F0
char* System_Log::GetPreamble(int32_t logClass) {
    char* (*preamble)() = s_Prefs[logClass].m_preamble;

    return preamble ? preamble() : s_Prefs[0].m_preamble();
}

// VERIFY: 4955f092 auto
// OFFSET: 0x00428420
char* System_Log::GetDayHeader(int32_t logClass) {
    char* (*dayHeader)() = s_Prefs[logClass].m_dayHeader;

    return dayHeader ? dayHeader() : s_Prefs[0].m_dayHeader();
}

// VERIFY: dd4b1e41 auto
// OFFSET: 0x0042E200
void System_Log::PrintTimestamp(FILE* file, Blizzard::Time::Timestamp time) {
    Blizzard::Time::TimeRec rec;
    Blizzard::Time::BreakTime(time, rec);
    fprintf(file, "%02d:%02d:%02d.%04d ", rec.hour, rec.minute, rec.second, rec.nanosecond / 100000);
}

// VERIFY: 7f6c133f manual
// OFFSET: 0x0042E250
int32_t System_Log::FindBest(const char* name, uint32_t now) {
    int32_t freeIndex = -1;
    int32_t oldestOpen = -1;
    int32_t oldestClosed = -1;

    for (int32_t i = 0; i < 16; i++) {
        LogFile& log = s_logs[i];

        if (!log.m_name) {
            if (freeIndex < 0) {
                freeIndex = i;
            }

            continue;
        }

        if (name && !strcmp(log.m_name, name)) {
            if (log.m_file || i >= 0) {
                return i;
            }

            break;
        }

        if (log.m_file) {
            if (oldestOpen < 0 || now - log.m_lastWrite > now - s_logs[oldestOpen].m_lastWrite) {
                oldestOpen = i;
            }
        } else if (oldestClosed < 0 || now - log.m_lastWrite > now - s_logs[oldestClosed].m_lastWrite) {
            oldestClosed = i;
        }
    }

    if (freeIndex >= 0) {
        return freeIndex;
    }

    int32_t index = oldestClosed >= 0 ? oldestClosed : oldestOpen;

    if (s_logs[index].m_file) {
        fclose(s_logs[index].m_file);
        s_logs[index].m_file = nullptr;
    }

    return index;
}

// VERIFY: 515229bb manual
// OFFSET: 0x00432F00
bool Blizzard::Log::Expand(int32_t logClass, const char* name, char* dst, int32_t count, bool createDirectory) {
    Blizzard::Time::TimeRec rec;
    Blizzard::Time::BreakTime(System_Log::s_startTime, rec);

    char path[1024];
    Blizzard::String::MakeUnivPath(name, path, sizeof(path));

    char* directory = path;
    char* filename;
    char* separator = strrchr(path, '/');

    if (separator) {
        *separator = '\0';
        filename = separator + 1;
    } else {
        filename = path;
        directory = nullptr;
    }

    const char* defaultDirectory = System_Log::s_Prefs[0].m_directory;
    const char* classDirectory = nullptr;

    if (logClass) {
        classDirectory = System_Log::s_Prefs[logClass].m_directory ? System_Log::s_Prefs[logClass].m_directory : System_Log::s_Prefs[0].m_directory;
    }

    char base[512];

    if (directory && Blizzard::File::IsAbsolutePath(directory)) {
        base[0] = '\0';
    } else if (!defaultDirectory) {
        if (classDirectory) {
            Blizzard::String::Copy(base, classDirectory, sizeof(base));
        } else {
            base[0] = '\0';
        }
    } else if (classDirectory) {
        if (Blizzard::File::IsAbsolutePath(defaultDirectory) && !Blizzard::File::IsAbsolutePath(classDirectory)) {
            Blizzard::String::Copy(base, defaultDirectory, sizeof(base));
            Blizzard::String::Append(base, classDirectory, sizeof(base));
        } else {
            Blizzard::String::Copy(base, classDirectory, sizeof(base));
        }
    } else {
        Blizzard::String::Copy(base, defaultDirectory, sizeof(base));
    }

    char* out = dst + Blizzard::String::Copy(dst, base, count);

    if (directory) {
        out += Blizzard::String::Copy(out, directory, static_cast<uint32_t>(count + dst - out));
        *out++ = '/';
    }

    *out = '\0';

    if (createDirectory) {
        Blizzard::File::CreateDirectory(dst, true);
    }

    const char* source = filename;

    if (*source) {
        int32_t year = rec.year;

        do {
            if (out >= dst + count - 20) {
                break;
            }

            char c = *source;

            if (c != '%' || (c = *++source) == '%') {
                *out++ = c;
            } else if (c == 'T') {
                Blizzard::String::Format(out, static_cast<uint32_t>(count + dst - out), "%02d%02d%02d-%02d%02d%02d", year % 100, rec.month, rec.day, rec.hour, rec.minute, rec.second);
                out += out ? strlen(out) : 0;
            } else if (c == 'Y') {
                Blizzard::String::Format(out, static_cast<uint32_t>(count + dst - out), "%02d%02d%02d", year % 100, rec.month, rec.day);
                out += out ? strlen(out) : 0;
            }

            source++;
        } while (*source);
    }

    *out = '\0';
    Blizzard::String::MakeBackslashPath(dst, dst, count);

    return true;
}

// VERIFY: 3899484a manual
// OFFSET: 0x00433220
const char* System_Log::Intern(const char* str) {
    char* pool = s_stringPool;
    char* newPool = nullptr;

    if (!pool) {
        pool = static_cast<char*>(malloc(4096));
        s_stringPool = pool;
        s_stringPoolUsed = 0;
        s_stringPoolSize = 4096;
    }

    uint32_t length = str ? static_cast<uint32_t>(strlen(str)) : 0;

    if (s_stringPoolUsed + length + 1 > 4096) {
        for (int32_t pass = 2; pass; pass--) {
            s_stringPoolUsed = 0;

            for (int32_t i = 0; i < 16; i++) {
                const char* name = s_logs[i].m_name;
                const char* moved = nullptr;

                if (name) {
                    moved = name;
                    uint32_t bytes = static_cast<uint32_t>(strlen(name)) + 1;

                    if (newPool) {
                        moved = newPool + s_stringPoolUsed;
                        Blizzard::String::Copy(newPool + s_stringPoolUsed, name, bytes);
                    }

                    s_stringPoolUsed += bytes;
                }

                s_logs[i].m_name = moved;

                const char* path = s_logs[i].m_path;
                moved = nullptr;

                if (path) {
                    moved = path;
                    uint32_t bytes = static_cast<uint32_t>(strlen(path)) + 1;

                    if (newPool) {
                        moved = newPool + s_stringPoolUsed;
                        Blizzard::String::Copy(newPool + s_stringPoolUsed, path, bytes);
                    }

                    s_stringPoolUsed += bytes;
                }

                s_logs[i].m_path = moved;
            }

            uint32_t size = s_stringPoolSize;

            if (s_stringPoolUsed > size) {
                do {
                    size *= 2;
                } while (s_stringPoolUsed > size);

                s_stringPoolSize = size;
            }

            if (!newPool) {
                newPool = static_cast<char*>(malloc(size));
            }
        }

        free(s_stringPool);
        pool = newPool;
        s_stringPool = newPool;
    }

    if (!str) {
        return nullptr;
    }

    const char* result = str;
    uint32_t bytes = static_cast<uint32_t>(strlen(str)) + 1;

    if (pool) {
        result = pool + s_stringPoolUsed;
        Blizzard::String::Copy(pool + s_stringPoolUsed, str, bytes);
    }

    s_stringPoolUsed += bytes;

    return result;
}

// VERIFY: f7bbf413 manual
// OFFSET: 0x00438560
bool System_Log::Reopen(int32_t logClass, const char* name, FILE*& file, int32_t& index, bool binary) {
    uint32_t now = static_cast<uint32_t>(Blizzard::Time::Milliseconds());
    index = FindBest(name, now);
    FILE* stream = s_logs[index].m_file;

    if (stream) {
        if (s_logs[index].m_binary == binary) {
            file = s_logs[index].m_file;
            return true;
        }

        fclose(stream);
        stream = nullptr;
        s_logs[index].m_binary = binary;
    }

    char path[1024];

    if (!Blizzard::Log::Expand(logClass, name, path, sizeof(path), true)) {
        return false;
    }

    s_logs[index].m_name = Intern(name);
    s_logs[index].m_path = Intern(path);
    s_logs[index].m_openTime = now;
    s_logs[index].m_lastWrite = now;
    s_logs[index].m_logClass = logClass;

    const char* mode = s_Prefs[logClass].m_mode;

    if (!mode) {
        mode = s_Prefs[0].m_mode;
    }

    if (binary) {
        mode = "ab";
    }

    bool created = true;

    if (*mode == 'a' && !fopen_s(&stream, s_logs[index].m_path, "r")) {
        fclose(stream);
        created = false;
    }

    fopen_s(&s_logs[index].m_file, s_logs[index].m_path, mode);

    if (!s_logs[index].m_file) {
        file = nullptr;
        return false;
    }

    if (s_logs[index].m_headerWritten) {
        if (created) {
            char* header = GetDayHeader(logClass);
            fprintf(s_logs[index].m_file, "%s", header);
            free(header);
        }
    } else {
        char* header = GetPreamble(logClass);
        fprintf(s_logs[index].m_file, "%s", header);
        s_logs[index].m_headerWritten = true;
        free(header);
    }

    file = s_logs[index].m_file;

    return true;
}

// VERIFY: 66a2d90b manual
// OFFSET: 0x00438800
void System_Log::FlushLogs(bool force) {
    uint32_t now = static_cast<uint32_t>(Blizzard::Time::Milliseconds());

    if (force) {
        now -= 86400000;
    }

    for (int32_t i = 0; i < 16; i++) {
        LogFile& log = s_logs[i];

        if (!log.m_file) {
            continue;
        }

        uint32_t maxOpenTime = s_Prefs[log.m_logClass].m_maxOpenTime;

        if (!maxOpenTime) {
            maxOpenTime = s_Prefs[0].m_maxOpenTime;
        }

        if (now - log.m_openTime <= maxOpenTime) {
            uint32_t maxIdleTime = s_Prefs[log.m_logClass].m_maxIdleTime;

            if (!maxIdleTime) {
                maxIdleTime = s_Prefs[0].m_maxIdleTime;
            }

            if (now - log.m_lastWrite <= maxIdleTime) {
                continue;
            }
        }

        fclose(log.m_file);
        log.m_file = nullptr;
    }
}

// VERIFY: 95fb2713 manual
// OFFSET: 0x00439FE0
void System_Log::IVPrintf(bool locked, int32_t logClass, const char* name, const char* format, va_list args) {
    FILE* file;
    int32_t index;

    if (!Reopen(logClass, name, file, index, false)) {
        return;
    }

    Blizzard::Time::Timestamp now = Blizzard::Time::GetTimestamp();

    if (!s_Prefs[logClass].m_noTimestamp && !s_Prefs[0].m_noTimestamp) {
        PrintTimestamp(file, now);
    }

    vfprintf(file, format, args);
    s_logs[index].m_lastWrite = static_cast<uint32_t>(Blizzard::Time::Milliseconds());
}

// VERIFY: d7bec52d manual
// OFFSET: 0x0043A070
void System_Log::Init(int32_t logClass) {
    if (s_prefCount > logClass) {
        return;
    }

    if (logClass < 4) {
        logClass = 4;
    }

    Pref* old = s_Prefs;
    uint32_t bytes = sizeof(Pref) * (logClass + 1);
    s_Prefs = static_cast<Pref*>(malloc(bytes));
    memset(s_Prefs, 0, bytes);

    if (old) {
        Blizzard::String::MemCopy(s_Prefs, old, sizeof(Pref) * s_prefCount);
        free(old);
    }

    if (!s_prefCount) {
        s_Prefs[0].m_mode = "a";
        s_Prefs[0].m_preamble = DefaultPreamble;
        s_Prefs[0].m_dayHeader = DefaultDayHeader;
        s_Prefs[0].m_disabled = false;
        s_Prefs[0].m_noTimestamp = false;
        s_Prefs[0].m_maxOpenTime = 30000;
        s_Prefs[0].m_maxIdleTime = 15000;

        char directory[1024];
#if defined(__APPLE__)
        GetDefaultDirectory(directory, sizeof(directory));
#else
        Blizzard::Process::GetProcessDirectory(directory, sizeof(directory));
#endif
        s_Prefs[0].m_directory = static_cast<char*>(malloc(strlen(directory) + 1));
        Blizzard::String::MakeUnivPath(directory, s_Prefs[0].m_directory, static_cast<int32_t>(strlen(directory) + 1));
        s_prefCount++;
    }

    if (s_prefCount == 1) {
        s_Prefs[1] = s_Prefs[0];
        s_Prefs[1].m_maxOpenTime = 1;
        s_prefCount++;
    }

    if (s_prefCount == 2) {
        s_Prefs[2] = s_Prefs[0];
        s_Prefs[2].m_maxOpenTime = 1;
        s_prefCount++;
    }

    if (s_prefCount == 3) {
        s_Prefs[3] = s_Prefs[0];
        s_Prefs[3].m_disabled = true;
    }

    s_prefCount = logClass + 1;
}

// VERIFY: 88e9144c manual
// OFFSET: 0x0044CC50
void System_Log::ShutdownProc() {
    if (s_state <= 0) {
        return;
    }

    s_state = -1;

    if (s_thread && s_threadRunning) {
        Blizzard::Thread::WaitForever(s_thread);
        Blizzard::Thread::Close(s_thread);
    }

    Blizzard::Lock::MutexEnter(s_mutex);
    Blizzard::Lock::MutexLeave(s_mutex);
    FlushLogs(true);
    Blizzard::Lock::MutexDestroy(s_mutex);
    free(s_Prefs);
    free(s_stringPool);
}

// VERIFY: 78f12fb8 auto
// OFFSET: 0x004519B0
bool System_Log::Acquire(int32_t logClass) {
    if (s_state < 0) {
        return false;
    }

    if (!s_state) {
#if defined(__APPLE__)
        static std::once_flag once;
        std::call_once(once, [] {
#endif
        s_startTime = Blizzard::Time::GetTimestamp();
        Blizzard::Lock::MutexCreate(s_mutex);
        s_state = 1;
        s_threadRunning = true;
        s_threadRunning = Blizzard::Thread::Create(LogThread, nullptr, &s_thread, "", 0, 0x10000);
#if defined(__APPLE__)
        });
#endif
    }

    Blizzard::Lock::MutexEnter(s_mutex);
    Init(logClass);

    return true;
}

// VERIFY: a3c2e8bc manual
// OFFSET: 0x00451BB0
void Blizzard::Log::Printf(int32_t logClass, const char* name, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (System_Log::Acquire(logClass)) {
        if (!System_Log::s_Prefs[logClass].m_disabled && !System_Log::s_Prefs[0].m_disabled) {
            System_Log::IVPrintf(true, logClass, name, format, args);
        }

        Blizzard::Lock::MutexLeave(System_Log::s_mutex);
    }

    va_end(args);
}

// VERIFY: e571c9ce manual
// OFFSET: 0x00451C10
void Blizzard::Log::Flush(int32_t logClass) {
    if (System_Log::Acquire(logClass)) {
        System_Log::FlushLogs(true);
        Blizzard::Lock::MutexLeave(System_Log::s_mutex);
    }
}

// INLINED
System_Log::UsingLog::UsingLog() : Blizzard::Using::Use("Blizzard::Log") {
    System_Using::Intern(this->m_name)->shutdownproc = System_Log::ShutdownProc;
    Blizzard::Using::DependsOn("Blizzard::Log", "Blizzard::File");
}
