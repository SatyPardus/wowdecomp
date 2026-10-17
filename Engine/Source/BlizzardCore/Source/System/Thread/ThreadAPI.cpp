#include <cstdint>
#include <cstdlib>
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"

// STATIC: 0x00B38334
static System_Thread::UsingThread s_usingThread;

// VERIFY: 0c90ad19 manual
// OFFSET: 0x00450B80
bool Blizzard::Thread::CreateDetached(uint32_t (*function)(void*), void* param, const char* name) {
    ThreadRecord* thread;
    bool result = Create(function, param, &thread, name, 0, 0);

    if (result) {
        Close(thread);
    }

    return result;
}

// VERIFY: 5a19ef77 manual
// OFFSET: 0x004509F0
System_Thread::UsingThread::UsingThread() : Blizzard::Using::Use("Blizzard::Thread") {
    System_Using::Intern(this->m_name)->shutdownproc = System_Thread::ShutdownThreadSystem;
    Blizzard::Using::DependsOn("Blizzard::Thread", "Blizzard::Memory");
    Blizzard::Using::DependsOn("Blizzard::ThreadLocalStorage", "Blizzard::Thread");
    System_Thread::InitThreadSystem();
}
