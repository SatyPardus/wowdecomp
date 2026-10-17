#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

// STATIC: 0x00AB93F0
static System_Thread::UsingThreadLocalStorage s_usingThreadLocalStorage;

// STATIC: 0x00B33730
Blizzard::Thread::TLSSlot* System_Thread::s_slotList[128];

// STATIC: 0x00B33948
uint32_t System_Thread::s_slotListUsed;

// INLINED
System_Thread::UsingThreadLocalStorage::UsingThreadLocalStorage() : Blizzard::Using::Use("Blizzard::ThreadLocalStorage") {
    System_Using::Intern(this->m_name)->shutdownproc = System_Thread::ShutdownThreadLocalStorage;
}
