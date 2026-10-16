#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_LOG_LOG_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_LOG_LOG_H

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Types.h"
#include "Engine/Source/BlizzardCore/Source/System/Using/Using.h"

namespace Blizzard {
namespace Log {

bool Expand(int32_t logClass, const char* name, char* dst, int32_t count, bool createDirectory);
void Flush(int32_t logClass);
void Printf(int32_t logClass, const char* name, const char* format, ...);

}
}

namespace System_Log {

class UsingLog : public Blizzard::Using::Use {
    public:
    UsingLog();
};

struct Pref {
    /* 0000 */ const char* m_mode;
    /* 0004 */ char* (*m_preamble)();
    /* 0008 */ char* (*m_dayHeader)();
    /* 000C */ bool m_disabled;
    /* 000D */ bool m_noTimestamp;
    /* 000E */ uint8_t unused_000E[2];
    /* 0010 */ uint32_t m_maxOpenTime;
    /* 0014 */ uint32_t m_maxIdleTime;
    /* 0018 */ char* m_directory;
};

struct LogFile {
    /* 0000 */ const char* m_name;
    /* 0004 */ const char* m_path;
    /* 0008 */ FILE* m_file;
    /* 000C */ int32_t m_logClass;
    /* 0010 */ uint32_t m_openTime;
    /* 0014 */ uint32_t m_lastWrite;
    /* 0018 */ bool m_headerWritten;
    /* 0019 */ bool m_binary;
    /* 001A */ uint8_t unused_001A[2];
};

extern bool s_threadRunning;
extern bool s_threadDone;
extern Blizzard::Lock::Mutex s_mutex;
extern int32_t s_state;
extern int32_t s_prefCount;
extern uint32_t s_stringPoolSize;
extern Blizzard::Time::Timestamp s_startTime;
extern Blizzard::Thread::ThreadRecord* s_thread;
extern uint32_t s_stringPoolUsed;
extern Pref* s_Prefs;
extern char* s_stringPool;
extern LogFile s_logs[16];

bool Acquire(int32_t logClass);
char* DefaultDayHeader();
char* DefaultPreamble();
int32_t FindBest(const char* name, uint32_t now);
void FlushLogs(bool force);
char* GetDayHeader(int32_t logClass);
char* GetPreamble(int32_t logClass);
void Init(int32_t logClass);
const char* Intern(const char* str);
void IVPrintf(bool locked, int32_t logClass, const char* name, const char* format, va_list args);
uint32_t LogThread(void* param);
void PrintTimestamp(FILE* file, Blizzard::Time::Timestamp time);
bool Reopen(int32_t logClass, const char* name, FILE*& file, int32_t& index, bool binary);
void ShutdownProc();

}

#endif
