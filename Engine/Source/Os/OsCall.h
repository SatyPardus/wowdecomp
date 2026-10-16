#ifndef ENGINE_SOURCE_OS_OSCALL_H
#define ENGINE_SOURCE_OS_OSCALL_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class OsCallContext;

class OsCallThread : public TSLinkedNode<OsCallThread> {
    public:
    /* 0008 */ uint32_t m_threadId;
    /* 000C */ void* m_threadHandle;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ OsCallContext* m_context;
    /* 0018 */ uint8_t unused_0018[0x1804];
    /* 181C */ char m_name[128];
};

struct OsCallContextEntry {
    /* 0000 */ TSLink<OsCallContextEntry> m_link;
    /* 0008 */ uint8_t unused_0008[8];
};

class OsCallContext : public TSLinkedNode<OsCallContext> {
    public:
    /* 0008 */ OsCallThread* m_thread;
    /* 000C */ int32_t unused_000C;
    /* 0010 */ uint8_t unused_0010[8];
    /* 0018 */ OsCallContextEntry m_entries[1024];
    /* 4018 */ uint8_t unused_4018[0x510];
    /* 4528 */ char m_name[128];

    ~OsCallContext();
};

void OsCallDestroy();
void OsCallDestroyContext(void* context);
void OsCallInitialize(const char* name);
void* OsCallInitializeContext(const char* name);
void OsCallResetContext(void* context);
void OsCallSetContext(void* context);

#endif
