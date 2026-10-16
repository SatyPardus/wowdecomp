#if defined(WOWDECOMP_ARCH_64)

#include <cstdint>
#include <windows.h>
#include "Engine/Source/Os/OsCall.h"
#include "Engine/Source/Os/Tls.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SThread.h"

// STATIC: 0x00B1D49C
static TSList<OsCallThread, TSGetLink<OsCallThread> > s_threadList;

// STATIC: 0x00B1D4A8
static TSList<OsCallContext, TSGetLink<OsCallContext> > s_contextList;

// STATIC: 0x00D41868
static uint32_t s_tlsIndex;

// STATIC: 0x00D4186C
static int32_t s_threadCount;

// STATIC: 0x00D41870
static CInitCritSect s_critsect;

// OFFSET: 0x0086F5A0
void OsCallSetContext(void* context) {
    if (!s_threadCount || !context) {
        return;
    }

    OsCallThread* thread = static_cast<OsCallThread*>(OsTlsGetValue(s_tlsIndex));

    if (!thread) {
        return;
    }

    s_critsect.Enter();
    static_cast<OsCallContext*>(context)->m_thread = thread;
    thread->m_context = static_cast<OsCallContext*>(context);
    s_critsect.Leave();
}

// OFFSET: 0x0086F5F0
void OsCallResetContext(void* context) {
    if (!s_threadCount || !context) {
        return;
    }

    OsCallThread* thread = static_cast<OsCallThread*>(OsTlsGetValue(s_tlsIndex));

    if (!thread) {
        return;
    }

    if (thread->m_context || static_cast<OsCallContext*>(context)->m_thread) {
        s_critsect.Enter();
        thread->m_context = nullptr;
        static_cast<OsCallContext*>(context)->m_thread = nullptr;
        s_critsect.Leave();
    }
}

// OFFSET: 0x0086F650
OsCallContext::~OsCallContext() {
}

// OFFSET: 0x0086F6E0
void OsCallDestroy() {
    s_critsect.Enter();

    if (s_threadCount) {
        OsCallThread* thread = static_cast<OsCallThread*>(OsTlsGetValue(s_tlsIndex));

        if (thread) {
            OsTlsSetValue(s_tlsIndex, nullptr);

            if (thread->m_context) {
                thread->m_context->m_thread = nullptr;
            }

            CloseHandle(thread->m_threadHandle);
            delete thread;
        }
    }

    if (--s_threadCount == 0) {
        OsTlsFree(s_tlsIndex);
    }

    s_critsect.Leave();
}

// OFFSET: 0x0086F760
void OsCallInitialize(const char* name) {
    s_critsect.Enter();

    if (s_threadCount++ == 0) {
        s_tlsIndex = OsTlsAlloc();
    }

    OsCallThread* oldThread = static_cast<OsCallThread*>(OsTlsGetValue(s_tlsIndex));

    if (oldThread) {
        delete oldThread;
    }

    OsCallThread* thread = new (SMemAlloc(sizeof(OsCallThread), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)) OsCallThread;
    thread->m_threadId = GetCurrentThreadId();
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &thread->m_threadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    SStrPrintf(thread->m_name, sizeof(thread->m_name), "%s [%u]", name, thread->m_threadId);

    OsTlsSetValue(s_tlsIndex, thread);
    s_threadList.LinkToTail(thread);

    s_critsect.Leave();
}

// OFFSET: 0x0086F880
void* OsCallInitializeContext(const char* name) {
    OsCallContext* context = new (SMemAlloc(sizeof(OsCallContext), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)) OsCallContext;
    SStrCopy(context->m_name, name, sizeof(context->m_name));
    context->unused_000C = -1;

    s_critsect.Enter();
    s_contextList.LinkToTail(context);
    s_critsect.Leave();

    return context;
}

// OFFSET: 0x0086F910
void OsCallDestroyContext(void* context) {
    if (!context) {
        return;
    }

    s_critsect.Enter();

    OsCallThread* thread = static_cast<OsCallContext*>(context)->m_thread;

    if (thread) {
        thread->m_context = nullptr;
    }

    delete static_cast<OsCallContext*>(context);

    s_critsect.Leave();
}

#endif
