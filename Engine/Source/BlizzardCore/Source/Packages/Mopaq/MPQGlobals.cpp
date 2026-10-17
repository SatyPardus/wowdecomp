#include <cstdint>
#if !defined(_WIN32)
#include <strings.h>
#define _stricmp strcasecmp
#endif
#include <cstring>
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQGlobals.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"

// STATIC: 0x00B33E58
Blizzard::Lock::ConstructOnce<System_MPQGlobals::Globals> System_MPQGlobals::s_globals;

// STATIC: 0x00B382C8
static System_MPQGlobals::RegisterGlobalStorage s_registerGlobalStorage;

// INLINED
bool System_MPQGlobals::ArchiveLess::operator()(const System_SFile::ARCHIVEREC* a, const System_SFile::ARCHIVEREC* b) const {
    if (a->m_priority == b->m_priority) {
        return a->m_id > b->m_id;
    }

    return a->m_priority > b->m_priority;
}

// VERIFY: 4886183c manual
// OFFSET: 0x0044FFF0
System_MPQGlobals::Globals::Globals() {
    this->m_mopaq.m_directAccess = 0;

    if (!Blizzard::Process::GetProcessDirectory(this->m_mopaq.m_basePath, sizeof(this->m_mopaq.m_basePath))) {
        this->m_mopaq.m_basePath[0] = '\0';
    }

    this->m_mopaq.m_openCallback = nullptr;
    this->m_mopaq.m_openCallbackParam = nullptr;
    this->m_mopaq.m_md5Mode = 0;
}

// VERIFY: e5b01062 manual
// OFFSET: 0x00451D00
System_MPQGlobals::ArchiveList* System_MPQGlobals::GetArchiveList() {
    return &s_globals.Get()->m_archives;
}

// VERIFY: 3d7e5cb3 manual
// OFFSET: 0x00451D60
Blizzard::Lock::RMutex* System_MPQGlobals::GetArchiveLock() {
    return &s_globals.Get()->m_archiveLock;
}

// VERIFY: 82f39fdc manual
// OFFSET: 0x00451DC0
System_MPQGlobals::MopaqGlobals* System_MPQGlobals::GetMopaqGlobals() {
    return &s_globals.Get()->m_mopaq;
}

// VERIFY: b9034813 manual
// OFFSET: 0x00451E20
Blizzard::Lock::RMutex* System_MPQGlobals::GetMopaqGlobalsLock() {
    return &s_globals.Get()->m_mopaqLock;
}

// VERIFY: 2488e5d1 manual
// OFFSET: 0x00451E80
System_MPQGlobals::WorkerPool* System_MPQGlobals::GetWorkerPool() {
    return &s_globals.Get()->m_workerPool;
}

// VERIFY: af26db8a manual
// OFFSET: 0x0042BA60
uint32_t System_MPQGlobals::Worker::ThreadProc(void* param) {
    Worker* worker = static_cast<Worker*>(param);

    Blizzard::Lock::EventWaitForever(worker->m_start.m_event);

    while (worker->m_proc) {
        worker->m_proc(worker->m_param);
        worker->m_proc = nullptr;
        worker->m_param = nullptr;
        Blizzard::Lock::EventSet(worker->m_done.m_event);
        Blizzard::Lock::EventWaitForever(worker->m_start.m_event);
    }

    return 0;
}

// VERIFY: ab9a819e manual
// OFFSET: 0x00451200
System_MPQGlobals::Worker::Worker() {
    this->m_proc = nullptr;
    this->m_param = nullptr;
    Blizzard::Thread::Create(ThreadProc, this, &this->m_thread, "", 0, 0x40000);
}

// VERIFY: 7d61b910 manual
// OFFSET: 0x00451A30
void System_MPQGlobals::WorkerPool::Acquire(uint32_t count, Worker** workers) {
    uint32_t i = 0;

    if (!count) {
        return;
    }

    Blizzard::Lock::MutexEnter(this->m_mutex.m_mutex);

    for (; i < count && !this->m_workers.empty(); i++) {
        workers[i] = this->m_workers.back();
        this->m_workers.pop_back();
    }

    for (; i < count; i++) {
        workers[i] = new Worker();
    }

    Blizzard::Lock::MutexLeave(this->m_mutex.m_mutex);
}

// VERIFY: d764e194 manual
// OFFSET: 0x00446060
void System_MPQGlobals::WorkerPool::Release(uint32_t count, Worker** workers) {
    if (!count) {
        return;
    }

    Blizzard::Lock::MutexEnter(this->m_mutex.m_mutex);

    do {
        this->m_workers.push_back(*workers++);
    } while (--count);

    Blizzard::Lock::MutexLeave(this->m_mutex.m_mutex);
}

// VERIFY: 99eaeda8 manual
// OFFSET: 0x00453FC0
System_SFile::ARCHIVEREC* System_MPQGlobals::FindArchiveByName(const char* name) {
    ArchiveList* archives = GetArchiveList();

    for (ArchiveList::iterator it = archives->begin(); it != archives->end(); ++it) {
        if (name && *it && !_stricmp(name, (*it)->m_name)) {
            return *it;
        }
    }

    return nullptr;
}

// VERIFY: 5ed0190a manual
// OFFSET: 0x0044FDB0
System_MPQGlobals::Globals::~Globals() {
}

// VERIFY: 7f085614 manual
// OFFSET: 0x0044E4C0
System_MPQGlobals::WorkerPool::~WorkerPool() {
    for (std::vector<Worker*>::iterator it = this->m_workers.begin(); it != this->m_workers.end(); ++it) {
        delete *it;
    }
}

// VERIFY: a6b7e9bd manual
// OFFSET: 0x0044C4D0
System_MPQGlobals::Worker::~Worker() {
    this->m_proc = nullptr;
    Blizzard::Lock::EventSet(this->m_start.m_event);
    Blizzard::Thread::WaitForever(this->m_thread);
    Blizzard::Thread::Close(this->m_thread);
}

// VERIFY: 599afcc5 auto
// OFFSET: 0x00451B40
void System_MPQGlobals::RegisterGlobalStorage::Shutdown() {
    if (s_globals.m_constructed) {
        reinterpret_cast<Globals*>(s_globals.m_storage)->~Globals();
        s_globals.m_constructed = false;
        s_globals.m_count.m_value = 0;
    }
}

// VERIFY: 6b56e30f manual
// OFFSET: 0x00451C90
System_MPQGlobals::RegisterGlobalStorage::RegisterGlobalStorage() : Blizzard::Using::Use("Blizzard::Mopaq") {
    System_Using::Intern(this->m_name)->shutdownproc = System_MPQGlobals::RegisterGlobalStorage::Shutdown;
    Blizzard::Using::DependsOn("Blizzard::Mopaq", "Blizzard::Memory");
    Blizzard::Using::DependsOn("Blizzard::Mopaq", "Blizzard::File");
    Blizzard::Using::DependsOn("Blizzard::Mopaq", "Blizzard::ThreadLocalStorage");
    Blizzard::Using::DependsOn("Blizzard::ThreadLocalStorage", "Blizzard::Memory");
}
