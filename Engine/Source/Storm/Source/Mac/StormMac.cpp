#if defined(__APPLE__)

#include "Engine/Source/Platform/Compat/ThreadStack.h"
#include <crt_externs.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Source/Storm/H/SCritSect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SCmd.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Storm/Source/SOption.h"
#include "Engine/Source/Storm/Source/SThread.h"

extern char** environ;

namespace {

constexpr uint32_t kWaitObject0 = 0;
constexpr uint32_t kWaitTimeout = 258;
constexpr uint32_t kWaitFailed = 0xFFFFFFFFu;
constexpr uint32_t kInfinite = 0xFFFFFFFFu;
constexpr uint32_t kErrorInvalidParameter = 87;
constexpr uint32_t kErrorOpenFailed = 110;

uint64_t CurrentThreadId64() {
    uint64_t threadId = 0;
    pthread_threadid_np(nullptr, &threadId);
    return threadId;
}

uint32_t CurrentThreadId32() {
    return static_cast<uint32_t>(CurrentThreadId64());
}

std::chrono::steady_clock::time_point DeadlineFromTimeout(uint32_t timeout) {
    if (timeout == kInfinite) {
        return std::chrono::steady_clock::time_point::max();
    }

    return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
}

struct StormHandle {
    enum class Kind {
        Event,
        Mutex,
        Semaphore,
        Thread
    };

    explicit StormHandle(Kind kind) : kind(kind) {
    }

    virtual ~StormHandle() = default;
    virtual bool TryAcquire() = 0;
    virtual void UndoAcquire() = 0;
    virtual uint32_t Wait(uint32_t timeout) = 0;

    Kind kind;
};

struct EventHandle final : StormHandle {
    EventHandle(bool manualReset, bool initialState)
        : StormHandle(Kind::Event), manualReset(manualReset), signaled(initialState) {
    }

    bool TryAcquire() override {
        std::lock_guard<std::mutex> lock(mutex);

        if (!signaled) {
            return false;
        }

        if (!manualReset) {
            signaled = false;
        }

        return true;
    }

    void UndoAcquire() override {
        if (!manualReset) {
            Set();
        }
    }

    uint32_t Wait(uint32_t timeout) override {
        std::unique_lock<std::mutex> lock(mutex);

        if (timeout == 0 && !signaled) {
            return kWaitTimeout;
        }

        if (timeout == kInfinite) {
            cond.wait(lock, [this] { return signaled; });
        } else if (!cond.wait_until(lock, DeadlineFromTimeout(timeout), [this] { return signaled; })) {
            return kWaitTimeout;
        }

        if (!manualReset) {
            signaled = false;
        }

        return kWaitObject0;
    }

    int32_t Set() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            signaled = true;
        }

        cond.notify_all();
        return 1;
    }

    int32_t Reset() {
        std::lock_guard<std::mutex> lock(mutex);
        signaled = false;
        return 1;
    }

    std::mutex mutex;
    std::condition_variable cond;
    bool manualReset = false;
    bool signaled = false;
};

struct MutexHandle final : StormHandle {
    explicit MutexHandle(bool initialOwner) : StormHandle(Kind::Mutex) {
        if (initialOwner) {
            owner = CurrentThreadId64();
            recursion = 1;
        }
    }

    bool TryAcquire() override {
        std::lock_guard<std::mutex> lock(mutex);
        uint64_t current = CurrentThreadId64();

        if (!owner || owner == current) {
            owner = current;
            recursion++;
            return true;
        }

        return false;
    }

    void UndoAcquire() override {
        Release();
    }

    uint32_t Wait(uint32_t timeout) override {
        std::unique_lock<std::mutex> lock(mutex);
        uint64_t current = CurrentThreadId64();

        if (timeout == 0 && owner && owner != current) {
            return kWaitTimeout;
        }

        auto ready = [this, current] { return !owner || owner == current; };

        if (timeout == kInfinite) {
            cond.wait(lock, ready);
        } else if (!cond.wait_until(lock, DeadlineFromTimeout(timeout), ready)) {
            return kWaitTimeout;
        }

        owner = current;
        recursion++;
        return kWaitObject0;
    }

    int32_t Release() {
        std::lock_guard<std::mutex> lock(mutex);

        if (owner != CurrentThreadId64() || recursion == 0) {
            return 0;
        }

        if (--recursion == 0) {
            owner = 0;
            cond.notify_all();
        }

        return 1;
    }

    std::mutex mutex;
    std::condition_variable cond;
    uint64_t owner = 0;
    uint32_t recursion = 0;
};

struct SemaphoreHandle final : StormHandle {
    SemaphoreHandle(int32_t initialCount, int32_t maximumCount)
        : StormHandle(Kind::Semaphore), count(std::max<int32_t>(0, initialCount)), maximum(std::max<int32_t>(1, maximumCount)) {
        count = std::min(count, maximum);
    }

    bool TryAcquire() override {
        std::lock_guard<std::mutex> lock(mutex);

        if (count <= 0) {
            return false;
        }

        count--;
        return true;
    }

    void UndoAcquire() override {
        Signal(1);
    }

    uint32_t Wait(uint32_t timeout) override {
        std::unique_lock<std::mutex> lock(mutex);

        if (timeout == 0 && count <= 0) {
            return kWaitTimeout;
        }

        if (timeout == kInfinite) {
            cond.wait(lock, [this] { return count > 0; });
        } else if (!cond.wait_until(lock, DeadlineFromTimeout(timeout), [this] { return count > 0; })) {
            return kWaitTimeout;
        }

        count--;
        return kWaitObject0;
    }

    int32_t Signal(uint32_t releaseCount) {
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (releaseCount > static_cast<uint32_t>(maximum - count)) {
                return 0;
            }

            count += static_cast<int32_t>(releaseCount);
        }

        cond.notify_all();
        return 1;
    }

    std::mutex mutex;
    std::condition_variable cond;
    int32_t count = 0;
    int32_t maximum = 1;
};

struct ThreadHandle final : StormHandle {
    ThreadHandle() : StormHandle(Kind::Thread) {
    }

    ~ThreadHandle() override = default;

    bool TryAcquire() override {
        std::lock_guard<std::mutex> lock(mutex);
        return done;
    }

    void UndoAcquire() override {
    }

    uint32_t Wait(uint32_t timeout) override {
        std::unique_lock<std::mutex> lock(mutex);

        if (timeout == 0 && !done) {
            return kWaitTimeout;
        }

        if (timeout == kInfinite) {
            cond.wait(lock, [this] { return done; });
        } else if (!cond.wait_until(lock, DeadlineFromTimeout(timeout), [this] { return done; })) {
            return kWaitTimeout;
        }

        return kWaitObject0;
    }

    void MarkDone(uint32_t value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            exitValue = value;
            done = true;
        }

        cond.notify_all();
    }

    std::mutex mutex;
    std::condition_variable cond;
    pthread_t thread = {};
    uint32_t threadId = 0;
    uint32_t exitValue = 0;
    bool done = false;
};

StormHandle* AsHandle(HANDLE handle) {
    return static_cast<StormHandle*>(handle);
}

std::string DirectoryName(const std::string& path) {
    size_t slash = path.find_last_of('/');

    if (slash == std::string::npos) {
        return {};
    }

    return path.substr(0, slash);
}

void CreateDirectories(const std::string& path) {
    if (path.empty()) {
        return;
    }

    std::string partial;
    size_t start = 0;

    if (path[0] == '/') {
        partial = "/";
        start = 1;
    }

    while (start <= path.size()) {
        size_t slash = path.find('/', start);
        std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);

        if (!part.empty()) {
            if (partial.size() > 1 && partial.back() != '/') {
                partial.push_back('/');
            }

            partial += part;
            mkdir(partial.c_str(), 0755);
        }

        if (slash == std::string::npos) {
            break;
        }

        start = slash + 1;
    }
}

std::string NormalizePath(const char* path) {
    std::string result = path ? path : "";

    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }

    return result;
}

void SetThreadNameForCurrentThread(const char* threadName) {
    if (threadName && *threadName) {
        pthread_setname_np(threadName);
    }
}

struct ThreadLaunch {
    uint32_t (STORMAPI* threadProc)(void*) = nullptr;
    void* param = nullptr;
    ThreadHandle* handle = nullptr;
};

struct ProcessCompletion {
    void (*callback)(void*) = nullptr;
    void* data = nullptr;
    pid_t pid = -1;
};

std::mutex& RwLockMapMutex() {
    static std::mutex& mutex = *new std::mutex;
    return mutex;
}

std::unordered_map<const void*, std::shared_mutex*>& RwLockMap() {
    static std::unordered_map<const void*, std::shared_mutex*> locks;
    return locks;
}

std::shared_mutex* FindRwLock(volatile SRWLock::SURWLOCK* lock) {
    std::lock_guard<std::mutex> guard(RwLockMapMutex());
    auto it = RwLockMap().find(const_cast<const SRWLock::SURWLOCK*>(lock));
    return it == RwLockMap().end() ? nullptr : it->second;
}

std::string EscapeIni(const std::string& value) {
    std::string out;

    for (char c : value) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            default:
                out.push_back(c);
                break;
        }
    }

    return out;
}

std::string UnescapeIni(const std::string& value) {
    std::string out;

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            out.push_back(value[i]);
            continue;
        }

        char next = value[++i];

        switch (next) {
            case 't':
                out.push_back('\t');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            default:
                out.push_back(next);
                break;
        }
    }

    return out;
}

std::string RegistryPath() {
    const char* home = getenv("HOME");
    std::string path = home && *home ? home : ".";
    path += "/Library/Preferences/WowDecomp/registry.ini";
    return path;
}

std::string BuildRegistryKey(const char* keyname, uint32_t flags) {
    std::string key;

    if (!(flags & SREG_ABSOLUTE)) {
        key = (flags & SREG_BATTLE_NET) ? "Software\\Battle.net\\" : "Software\\Blizzard Entertainment\\";
    }

    key += keyname ? keyname : "";
    return key;
}

struct RegistryEntry {
    std::string type;
    std::string value;
};

using RegistryMap = std::unordered_map<std::string, RegistryEntry>;

std::mutex& RegistryMutex() {
    static std::mutex& mutex = *new std::mutex;
    return mutex;
}

std::string RegistryMapKey(const std::string& key, const std::string& value) {
    return key + "\t" + value;
}

RegistryMap LoadRegistryUnlocked() {
    RegistryMap map;
    std::ifstream file(RegistryPath());
    std::string line;

    while (std::getline(file, line)) {
        std::vector<std::string> parts;
        size_t start = 0;

        for (int i = 0; i < 3; ++i) {
            size_t tab = line.find('\t', start);

            if (tab == std::string::npos) {
                break;
            }

            parts.push_back(UnescapeIni(line.substr(start, tab - start)));
            start = tab + 1;
        }

        if (parts.size() == 3) {
            map[RegistryMapKey(parts[0], parts[1])] = {parts[2], UnescapeIni(line.substr(start))};
        }
    }

    return map;
}

bool SaveRegistryUnlocked(const RegistryMap& map) {
    std::string path = RegistryPath();
    CreateDirectories(DirectoryName(path));
    std::ofstream file(path, std::ios::trunc);

    if (!file) {
        return false;
    }

    for (const auto& [compound, entry] : map) {
        size_t tab = compound.find('\t');
        std::string key = tab == std::string::npos ? compound : compound.substr(0, tab);
        std::string valueName = tab == std::string::npos ? "" : compound.substr(tab + 1);
        file << EscapeIni(key) << '\t' << EscapeIni(valueName) << '\t' << EscapeIni(entry.type) << '\t' << EscapeIni(entry.value) << '\n';
    }

    return true;
}

struct LogRecord {
    HSLOG handle = nullptr;
    FILE* file = nullptr;
    std::string filename;
    uint32_t flags = 0;
    int32_t indent = 0;
    bool timestamp = true;
};

std::mutex& LogMutex() {
    static std::mutex& mutex = *new std::mutex;
    return mutex;
}

std::unordered_map<uintptr_t, LogRecord>& Logs() {
    static std::unordered_map<uintptr_t, LogRecord> logs;
    return logs;
}

std::atomic<uintptr_t>& LogSequence() {
    static std::atomic<uintptr_t> sequence{0};
    return sequence;
}

bool OpenLogFile(LogRecord& record) {
    if (record.file) {
        return true;
    }

    if (record.filename.empty() || (record.flags & SLOG_FLAG_NOFILE)) {
        return false;
    }

    std::string path = NormalizePath(record.filename.c_str());
    CreateDirectories(DirectoryName(path));
    record.file = fopen(path.c_str(), (record.flags & SLOG_FLAG_APPEND) ? "ab" : "wb");
    return record.file != nullptr;
}

std::string CurrentLogTimestamp() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    tm local = {};
    localtime_r(&tv.tv_sec, &local);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%u/%u %02u:%02u:%02u.%03u  ",
        local.tm_mon + 1,
        local.tm_mday,
        local.tm_hour,
        local.tm_min,
        local.tm_sec,
        static_cast<unsigned>(tv.tv_usec / 1000));
    return buffer;
}

std::mutex& ErrorMutex() {
    static std::mutex& mutex = *new std::mutex;
    return mutex;
}

std::vector<int32_t (STORMAPI*)(uint32_t, const char*, const char*, int32_t, const char*)>& ErrorHandlers() {
    static std::vector<int32_t (STORMAPI*)(uint32_t, const char*, const char*, int32_t, const char*)> handlers;
    return handlers;
}

uint32_t& LastStormError() {
    static uint32_t lastError = 0;
    return lastError;
}

int32_t& DisplayingError() {
    static int32_t displaying = 0;
    return displaying;
}

char (&LogTitle())[128] {
    static char title[128] = {};
    return title;
}

char (&LogLastPath())[1024] {
    static char path[1024] = {};
    return path;
}

int32_t (STORMAPI*& LogCallback())(char*, uint32_t) {
    static int32_t (STORMAPI* callback)(char*, uint32_t) = nullptr;
    return callback;
}

struct AppFatalInfo {
    const char* filename = nullptr;
    int32_t line = 0;
    uint32_t threadId = 0;
};

AppFatalInfo& FatalInfo() {
    static AppFatalInfo info;
    return info;
}

const char* StormErrorText(uint32_t errorcode) {
    switch (errorcode) {
        case STORM_ERROR_ASSERTION:
            return "Assertion Failure";
        case STORM_ERROR_NOT_ARCHIVE:
            return "Not Archive";
        case STORM_ERROR_VERSION_MISMATCH:
            return "Version Mismatch";
        case STORM_ERROR_MEM_NOT_ALLOCATED:
            return "Memory Already Freed";
        case STORM_ERROR_MEM_CORRUPTED:
            return "Memory Corrupt";
        case STORM_ERROR_MEM_INVALID:
            return "Memory Invalid Block";
        case STORM_ERROR_MEM_NOT_FREED:
            return "Memory Never Released";
        case STORM_ERROR_RESOURCES_NOT_RELEASED:
            return "Handle Never Released";
        case STORM_ERROR_OUT_OF_BOUNDS:
            return "Access Out Of Bounds";
        case STORM_ERROR_FILE_CORRUPTED:
            return "File Corrupt";
        case STORM_ERROR_FATAL:
            return "Fatal Exception";
        case STORM_ERROR_FATAL_CONDITION:
            return "Fatal Condition";
        default:
            return "";
    }
}

void WriteErrorLog(const char* message) {
    std::string dir = "Errors";
    CreateDirectories(dir);

    time_t now = time(nullptr);
    tm local = {};
    localtime_r(&now, &local);

    char path[1024];
    snprintf(path, sizeof(path), "Errors/%04u-%02u-%02u %02u.%02u.%02u Error.txt",
        local.tm_year + 1900,
        local.tm_mon + 1,
        local.tm_mday,
        local.tm_hour,
        local.tm_min,
        local.tm_sec);

    FILE* file = fopen(path, "wb");

    if (!file) {
        return;
    }

    if (LogTitle()[0]) {
        fprintf(file, "%s\n\n", LogTitle());
    }

    fprintf(file, "%s\n", message ? message : "");

    if (LogCallback()) {
        char buffer[5200] = {};

        if (LogCallback()(buffer, sizeof(buffer)) && buffer[0]) {
            fprintf(file, "\n%s\n", buffer);
        }
    }

    fclose(file);
    SStrCopy(LogLastPath(), path, sizeof(LogLastPath()));
}

void FormatV(char* buffer, size_t size, const char* format, va_list args) {
    if (!buffer || !size) {
        return;
    }

    vsnprintf(buffer, size, format ? format : "", args);
    buffer[size - 1] = 0;
}

std::string QuoteCommandLineArg(const char* arg) {
    std::string value = arg ? arg : "";
    bool needsQuotes = value.empty() || value.find_first_of(" ,;\"\t\n\r\x1A") != std::string::npos;

    if (!needsQuotes) {
        return value;
    }

    std::string out = "\"";

    for (char c : value) {
        out.push_back(c == '"' ? '\'' : c);
    }

    out.push_back('"');
    return out;
}

} // namespace

namespace S_Thread {
SThreadTrack s_threads[s_maxthreads];
int32_t s_numthreads;
CInitCritSect s_threadCrit;
} // namespace S_Thread

CCritSect::CCritSect() = default;
CCritSect::~CCritSect() = default;

void CCritSect::Enter() {
    this->m_critsect.lock();
}

void CCritSect::Enter(int32_t) {
    this->m_critsect.lock();
}

void CCritSect::Leave() {
    this->m_critsect.unlock();
}

void CCritSect::Leave(int32_t) {
    this->m_critsect.unlock();
}

SCritSect::SCritSect() = default;
SCritSect::~SCritSect() = default;

void SCritSect::Enter() {
    this->m_critsect.lock();
}

void SCritSect::Leave() {
    this->m_critsect.unlock();
}

int32_t CInitCritSect::Enter() {
    int32_t result = 0;

    if (!this->m_critsect) {
        while (__atomic_exchange_n(&this->m_spinLock, 1, __ATOMIC_ACQUIRE)) {
            sched_yield();
        }

        if (!this->m_critsect) {
            this->m_critsect = new CCritSect();
            result = 1;
        }

        __atomic_store_n(&this->m_spinLock, 0, __ATOMIC_RELEASE);
    }

    this->m_critsect->Enter();
    return result;
}

void CInitCritSect::Leave() {
    this->m_critsect->Leave();
}

void SRWLock::IInitialize() {
    SServerInitialize();
}

void SRWLock::IDestroy() {
    SServerDestroy();
}

void SRWLock::IOsRWLockIncRef() {
}

void SRWLock::IOsRWLockDecRef() {
}

uint32_t SRWLock::IAllocEvent(uint32_t) {
    return 0;
}

void SRWLock::IFreeEvent(uint32_t, uint32_t, int32_t) {
}

uint32_t SRWLock::IAddWaiter(volatile LONG*, uint32_t, uint32_t) {
    return 0;
}

int32_t SRWLock::IRemoveWaiter(volatile LONG*, uint32_t, uint32_t, uint32_t) {
    return 1;
}

void SRWLock::SUNNLockEnter(volatile SUNNLOCK*) {
}

void SRWLock::SUNNLockLeave(volatile SUNNLOCK*) {
}

void SRWLock::SURWLockInitialize(volatile SURWLOCK* lock) {
    lock->m_mutex.m_state = 1;
    lock->m_mutex.m_event = 0;
    lock->m_readerEvent = 0;

    std::lock_guard<std::mutex> guard(RwLockMapMutex());
    RwLockMap()[const_cast<const SURWLOCK*>(lock)] = new std::shared_mutex();
}

void SRWLock::SURWLockDelete(volatile SURWLOCK* lock) {
    std::shared_mutex* mutex = nullptr;

    {
        std::lock_guard<std::mutex> guard(RwLockMapMutex());
        auto it = RwLockMap().find(const_cast<const SURWLOCK*>(lock));

        if (it != RwLockMap().end()) {
            mutex = it->second;
            RwLockMap().erase(it);
        }
    }

    delete mutex;
}

void SRWLock::SURWLockEnter(volatile SURWLOCK* lock, int32_t forwriting) {
    std::shared_mutex* mutex = FindRwLock(lock);

    if (!mutex) {
        return;
    }

    if (forwriting) {
        mutex->lock();
    } else {
        mutex->lock_shared();
    }
}

void SRWLock::SURWLockLeave(volatile SURWLOCK* lock, int32_t fromwriting) {
    std::shared_mutex* mutex = FindRwLock(lock);

    if (!mutex) {
        return;
    }

    if (fromwriting) {
        mutex->unlock();
    } else {
        mutex->unlock_shared();
    }
}

CSRWLock::CSRWLock() {
    SRWLock::SURWLockInitialize(&this->m_opaqueData);
}

CSRWLock::~CSRWLock() {
    SRWLock::SURWLockDelete(&this->m_opaqueData);
}

void CSRWLock::Enter(int32_t forwriting) {
    SRWLock::SURWLockEnter(&this->m_opaqueData, forwriting);
}

void CSRWLock::Leave(int32_t fromwriting) {
    SRWLock::SURWLockLeave(&this->m_opaqueData, fromwriting);
}

SSyncObject::SSyncObject() : m_opaqueData(nullptr) {
}

SSyncObject::~SSyncObject() {
    Close();
}

void SSyncObject::Close() {
    delete AsHandle(this->m_opaqueData);
    this->m_opaqueData = nullptr;
}

int32_t SSyncObject::Valid() {
    return this->m_opaqueData != nullptr;
}

uint32_t SSyncObject::Wait(uint32_t timeout) {
    StormHandle* handle = AsHandle(this->m_opaqueData);
    return handle ? handle->Wait(timeout) : kWaitFailed;
}

uint32_t STORMAPI SSyncObject::WaitMultiplePtr(uint32_t count, SSyncObject** objects, int32_t waitAll, uint32_t timeout) {
    std::vector<StormHandle*> handles;
    handles.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        if (objects[i] && objects[i]->m_opaqueData) {
            handles.push_back(AsHandle(objects[i]->m_opaqueData));
        }
    }

    if (handles.empty()) {
        return kWaitFailed;
    }

    auto deadline = DeadlineFromTimeout(timeout);

    while (true) {
        if (waitAll) {
            std::vector<StormHandle*> acquired;
            bool all = true;

            for (StormHandle* handle : handles) {
                if (handle->TryAcquire()) {
                    acquired.push_back(handle);
                } else {
                    all = false;
                    break;
                }
            }

            if (all) {
                return kWaitObject0;
            }

            for (auto it = acquired.rbegin(); it != acquired.rend(); ++it) {
                (*it)->UndoAcquire();
            }
        } else {
            for (size_t i = 0; i < handles.size(); ++i) {
                if (handles[i]->TryAcquire()) {
                    return kWaitObject0 + static_cast<uint32_t>(i);
                }
            }
        }

        if (timeout == 0) {
            return kWaitTimeout;
        }

        if (timeout != kInfinite && std::chrono::steady_clock::now() >= deadline) {
            return kWaitTimeout;
        }

        usleep(1000);
    }
}

SEvent::SEvent(int32_t manualReset, int32_t initialState) {
    this->m_opaqueData = new EventHandle(manualReset != 0, initialState != 0);
}

int32_t SEvent::Set() {
    EventHandle* event = dynamic_cast<EventHandle*>(AsHandle(this->m_opaqueData));
    return event ? event->Set() : 0;
}

int32_t SEvent::Reset() {
    EventHandle* event = dynamic_cast<EventHandle*>(AsHandle(this->m_opaqueData));
    return event ? event->Reset() : 0;
}

SMutex::SMutex() {
    this->Create(0, nullptr);
}

void SMutex::Create(int32_t initialOwner, const char*) {
    Close();
    this->m_opaqueData = new MutexHandle(initialOwner != 0);
}

int32_t SMutex::Release() {
    MutexHandle* mutex = dynamic_cast<MutexHandle*>(AsHandle(this->m_opaqueData));
    return mutex ? mutex->Release() : 0;
}

SSemaphore::SSemaphore(int32_t initialCount, int32_t maximumCount) {
    this->m_opaqueData = new SemaphoreHandle(initialCount, maximumCount);
}

int32_t SSemaphore::Signal(uint32_t count) {
    SemaphoreHandle* sem = dynamic_cast<SemaphoreHandle*>(AsHandle(this->m_opaqueData));
    return sem ? sem->Signal(count) : 0;
}

uint32_t STORMAPI SGetCurrentThreadId() {
    return CurrentThreadId32();
}

uint32_t STORMAPI SGetCurrentProcessId() {
    return static_cast<uint32_t>(getpid());
}

int32_t STORMAPI SGetCurrentThreadPriority() {
    int policy = 0;
    sched_param param = {};

    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0) {
        return 0;
    }

    return param.sched_priority;
}

int32_t STORMAPI SSetCurrentThreadPriority(int32_t priority) {
    int policy = 0;
    sched_param param = {};

    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0) {
        return 0;
    }

    int minPriority = sched_get_priority_min(policy);
    int maxPriority = sched_get_priority_max(policy);

    if (minPriority != -1 && maxPriority != -1) {
        param.sched_priority = std::max(minPriority, std::min(maxPriority, priority));
    }

    return pthread_setschedparam(pthread_self(), policy, &param) == 0;
}

int32_t STORMAPI SSetCurrentProcessAffinityMask(size_t) {
    return 1;
}

static void EnsureMainThreadTracked() {
    S_Thread::s_threadCrit.Enter();

    if (!S_Thread::s_numthreads) {
        S_Thread::SThreadTrack* track = &S_Thread::s_threads[0];
        track->threadId = SGetCurrentThreadId();
        track->threadH = nullptr;
        track->live = 1;
        track->suspended = 0;
        SStrCopy(track->name, "main", sizeof(track->name));
        S_Thread::s_numthreads = 1;
    }

    S_Thread::s_threadCrit.Leave();
}

uint32_t STORMAPI S_Thread::s_SLaunchThread(void* threadParam) {
    ThreadLaunch* launch = static_cast<ThreadLaunch*>(threadParam);
    uint32_t (STORMAPI* threadProc)(void*) = launch->threadProc;
    void* param = launch->param;
    ThreadHandle* handle = launch->handle;
    uint32_t threadId = SGetCurrentThreadId();
    char threadName[16] = {};

    s_threadCrit.Enter();

    for (int32_t i = 0; i < s_numthreads; ++i) {
        if (s_threads[i].threadH == handle) {
            s_threads[i].threadId = threadId;
            SStrCopy(threadName, s_threads[i].name, sizeof(threadName));
            break;
        }
    }

    s_threadCrit.Leave();
    SetThreadNameForCurrentThread(threadName);
    delete launch;

    uint32_t result = threadProc ? threadProc(param) : 0;

    s_threadCrit.Enter();

    for (int32_t i = 0; i < s_numthreads; ++i) {
        if (s_threads[i].threadH == handle) {
            memmove(&s_threads[i], &s_threads[i + 1], (s_numthreads - i - 1) * sizeof(SThreadTrack));
            --s_numthreads;
            break;
        }
    }

    s_threadCrit.Leave();
    handle->MarkDone(result);
    return result;
}

static void* ThreadTrampoline(void* param) {
    uintptr_t result = S_Thread::s_SLaunchThread(param);
    return reinterpret_cast<void*>(result);
}

void* SCreateThread(uint32_t stackSize, uint32_t (STORMAPI* startAddress)(void*), void* parameter, uint32_t, uint32_t* threadId, char* threadName) {
    EnsureMainThreadTracked();

    ThreadHandle* handle = new ThreadHandle();
    ThreadLaunch* launch = new ThreadLaunch();
    launch->threadProc = startAddress;
    launch->param = parameter;
    launch->handle = handle;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_attr_setstacksize(&attr, WowThreadStackSize(stackSize));

    S_Thread::s_threadCrit.Enter();

    if (S_Thread::s_numthreads < S_Thread::s_maxthreads) {
        S_Thread::SThreadTrack* track = &S_Thread::s_threads[S_Thread::s_numthreads++];
        track->threadId = 0;
        track->threadH = handle;
        track->live = 1;
        track->suspended = 0;
        SStrCopy(track->name, threadName ? threadName : "", sizeof(track->name));
    }

    S_Thread::s_threadCrit.Leave();

    int err = pthread_create(&handle->thread, &attr, ThreadTrampoline, launch);
    pthread_attr_destroy(&attr);

    if (err != 0) {
        S_Thread::s_threadCrit.Enter();

        for (int32_t i = 0; i < S_Thread::s_numthreads; ++i) {
            if (S_Thread::s_threads[i].threadH == handle) {
                memmove(&S_Thread::s_threads[i], &S_Thread::s_threads[i + 1], (S_Thread::s_numthreads - i - 1) * sizeof(S_Thread::SThreadTrack));
                --S_Thread::s_numthreads;
                break;
            }
        }

        S_Thread::s_threadCrit.Leave();
        delete launch;
        delete handle;
        SErrSetLastError(err);
        return nullptr;
    }

    pthread_detach(handle->thread);

    while (!handle->threadId) {
        S_Thread::s_threadCrit.Enter();

        for (int32_t i = 0; i < S_Thread::s_numthreads; ++i) {
            if (S_Thread::s_threads[i].threadH == handle && S_Thread::s_threads[i].threadId) {
                handle->threadId = S_Thread::s_threads[i].threadId;
                break;
            }
        }

        S_Thread::s_threadCrit.Leave();

        if (!handle->threadId) {
            sched_yield();
        }
    }

    if (threadId) {
        *threadId = handle->threadId;
    }

    return handle;
}

void* STORMAPI SCreateThread(uint32_t (STORMAPI* startAddress)(void*), void* parameter, uint32_t* threadId, void*, char* threadName, uint32_t stackSize) {
    return SCreateThread(stackSize, startAddress, parameter, 0, threadId, threadName);
}

int32_t STORMAPI SThread::Create(uint32_t (STORMAPI* threadProc)(void*), void* param, SThread& thread, char* threadName, uint32_t stackSize) {
    uint32_t threadId = 0;
    thread.m_opaqueData = SCreateThread(threadProc, param, &threadId, nullptr, threadName, stackSize);
    return thread.m_opaqueData != nullptr;
}

int32_t SThread::SetPriority(int32_t priority) {
    static const int32_t priorities[] = {-20, -10, 0, 10, 20, 31};

    if (priority >= static_cast<int32_t>(sizeof(priorities) / sizeof(priorities[0]))) {
        return 0;
    }

    return priorities[priority] >= 0 ? 1 : 1;
}

int32_t STORMAPI SGetThreadName(uint32_t threadId, char* buffer, int32_t bufferchars) {
    if (!buffer || bufferchars < 1) {
        return 0;
    }

    EnsureMainThreadTracked();
    *buffer = 0;
    bool found = false;

    S_Thread::s_threadCrit.Enter();

    for (int32_t i = 0; i < S_Thread::s_numthreads; ++i) {
        if (S_Thread::s_threads[i].threadId == threadId) {
            SStrCopy(buffer, S_Thread::s_threads[i].name, bufferchars);
            found = true;
            break;
        }
    }

    S_Thread::s_threadCrit.Leave();
    return found;
}

static uint32_t STORMAPI ProcessCompletionThread(void* data) {
    ProcessCompletion* completion = static_cast<ProcessCompletion*>(data);
    int status = 0;
    waitpid(completion->pid, &status, 0);

    if (completion->callback) {
        completion->callback(completion->data);
    }

    delete completion;
    return 0;
}

int32_t STORMAPI SCreateProcess(const char* appName, char* commandLine, void (*callbackWhenProcessCompletes)(void*), void* callbackData) {
    const char* command = commandLine && *commandLine ? commandLine : appName;

    if (!command || !*command) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    pid_t pid = -1;
    char* const argv[] = {const_cast<char*>("sh"), const_cast<char*>("-c"), const_cast<char*>(command), nullptr};
    int err = posix_spawn(&pid, "/bin/sh", nullptr, nullptr, argv, environ);

    if (err != 0) {
        SErrSetLastError(err);
        return 0;
    }

    if (callbackWhenProcessCompletes) {
        ProcessCompletion* completion = new ProcessCompletion{callbackWhenProcessCompletes, callbackData, pid};
        uint32_t threadId = 0;
        SCreateThread(ProcessCompletionThread, completion, &threadId, nullptr, const_cast<char*>("process"), 0);
    }

    return 1;
}

void GenerateError(void (STORMAPI* errorcallback)(_CMDERROR*), uint32_t errorcode, const char* itemstring) {
    if (!errorcallback) {
        return;
    }

    const char* format = nullptr;

    switch (errorcode) {
        case STORM_ERROR_BAD_ARGUMENT:
            format = "Invalid argument: %s";
            break;
        case STORM_ERROR_NOT_ENOUGH_ARGUMENTS:
            format = "The syntax of the command is incorrect.";
            break;
        case kErrorOpenFailed:
            format = "Unable to open response file: %s";
            break;
        default:
            return;
    }

    char errorstr[256];

    if (strstr(format, "%s")) {
        SStrPrintf(errorstr, sizeof(errorstr), format, itemstring ? itemstring : "");
    } else {
        SStrCopy(errorstr, format, sizeof(errorstr));
    }

    SStrPack(errorstr, "\n", sizeof(errorstr));
    SErrSetLastError(errorcode);

    _CMDERROR data;
    data.errorcode = errorcode;
    data.itemstr = itemstring;
    data.errorstr = errorstr;
    errorcallback(&data);
}

int32_t ProcessFile(const char* filename, _PROCESSING* processing, CMDDEF** nextarg, int32_t (STORMAPI* extracallback)(const char*), void (STORMAPI* errorcallback)(_CMDERROR*)) {
    FILE* file = fopen(filename, "rb");

    if (!file) {
        if (errorcallback) {
            GenerateError(errorcallback, kErrorOpenFailed, filename);
        }

        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        fclose(file);
        return 0;
    }

    char* data = static_cast<char*>(SMemAlloc(static_cast<size_t>(size) + 1, __FILE__, __LINE__, 0));
    size_t bytesRead = fread(data, 1, static_cast<size_t>(size), file);
    fclose(file);
    data[bytesRead] = 0;

    const char* curr = data;
    int32_t result = ProcessString(&curr, processing, nextarg, extracallback, errorcallback);
    SMemFree(data, __FILE__, __LINE__, 0);
    return result;
}

int32_t STORMAPI SCmdProcessCommandLine(int32_t (STORMAPI* extracallback)(const char*), void (STORMAPI* errorcallback)(_CMDERROR*)) {
    int argc = *_NSGetArgc();
    char** argv = *_NSGetArgv();
    std::string commandLine;

    for (int i = 0; i < argc; ++i) {
        if (i) {
            commandLine.push_back(' ');
        }

        commandLine += QuoteCommandLineArg(argv[i]);
    }

    return SCmdProcess(commandLine.c_str(), 1, extracallback, errorcallback);
}

int32_t STORMAPI SRegGetBaseKey(uint32_t flags, char* buffer, uint32_t bufferchars) {
    if (!buffer || !bufferchars) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    SStrCopy(buffer, (flags & SREG_BATTLE_NET) ? "Software\\Battle.net\\" : "Software\\Blizzard Entertainment\\", bufferchars);
    return 1;
}

int32_t STORMAPI SRegLoadString(const char* keyname, const char* valuename, uint32_t flags, char* buffer, uint32_t buffersize) {
    if (!keyname || !*keyname || !valuename || !*valuename || !buffer || !buffersize) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    std::lock_guard<std::mutex> lock(RegistryMutex());
    RegistryMap map = LoadRegistryUnlocked();
    auto it = map.find(RegistryMapKey(BuildRegistryKey(keyname, flags), valuename));

    if (it == map.end()) {
        SErrSetLastError(STORM_ERROR_FILE_NOT_FOUND);
        return 0;
    }

    if (it->second.type == "dword") {
        SStrPrintf(buffer, buffersize, "%u", static_cast<uint32_t>(strtoul(it->second.value.c_str(), nullptr, 0)));
    } else {
        SStrCopy(buffer, it->second.value.c_str(), buffersize);
    }

    return 1;
}

int32_t STORMAPI SRegLoadValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t* value) {
    if (!keyname || !*keyname || !valuename || !*valuename || !value) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    std::lock_guard<std::mutex> lock(RegistryMutex());
    RegistryMap map = LoadRegistryUnlocked();
    auto it = map.find(RegistryMapKey(BuildRegistryKey(keyname, flags), valuename));

    if (it == map.end()) {
        SErrSetLastError(STORM_ERROR_FILE_NOT_FOUND);
        return 0;
    }

    *value = static_cast<uint32_t>(strtoul(it->second.value.c_str(), nullptr, 0));
    return 1;
}

int32_t STORMAPI SRegSaveString(const char* keyname, const char* valuename, uint32_t flags, const char* string) {
    if (!keyname || !*keyname || !valuename || !*valuename || !string) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    std::lock_guard<std::mutex> lock(RegistryMutex());
    RegistryMap map = LoadRegistryUnlocked();
    map[RegistryMapKey(BuildRegistryKey(keyname, flags), valuename)] = {"string", string};
    bool ok = SaveRegistryUnlocked(map);

    if (!ok) {
        SErrSetLastError(errno);
    }

    return ok;
}

int32_t STORMAPI SRegSaveValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t value) {
    if (!keyname || !*keyname || !valuename || !*valuename) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    std::lock_guard<std::mutex> lock(RegistryMutex());
    RegistryMap map = LoadRegistryUnlocked();
    map[RegistryMapKey(BuildRegistryKey(keyname, flags), valuename)] = {"dword", std::to_string(value)};
    bool ok = SaveRegistryUnlocked(map);

    if (!ok) {
        SErrSetLastError(errno);
    }

    return ok;
}

void STORMAPI SLogInitialize() {
}

void STORMAPI SLogDestroy() {
    std::lock_guard<std::mutex> lock(LogMutex());

    for (auto& [_, record] : Logs()) {
        if (record.file) {
            fflush(record.file);
            fclose(record.file);
            record.file = nullptr;
        }
    }

    Logs().clear();
}

int32_t STORMAPI SLogCreate(const char* filename, uint32_t flags, HSLOG* log) {
    if (!filename || !*filename || !log) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    *log = nullptr;
    uintptr_t id = ++LogSequence();

    if ((id & 3u) == 0) {
        ++id;
        LogSequence().store(id);
    }

    LogRecord record;
    record.handle = reinterpret_cast<HSLOG>(id);
    record.flags = flags;

    if (flags & SLOG_FLAG_NOFILE) {
        record.filename.clear();
    } else {
        record.filename = filename;

        if ((flags & SLOG_FLAG_OPENFILE) && !OpenLogFile(record)) {
            SErrSetLastError(errno);
            return 0;
        }
    }

    std::lock_guard<std::mutex> lock(LogMutex());
    Logs()[id] = record;
    *log = record.handle;
    return 1;
}

void STORMAPI SLogClose(HSLOG log) {
    std::lock_guard<std::mutex> lock(LogMutex());
    auto it = Logs().find(reinterpret_cast<uintptr_t>(log));

    if (it == Logs().end()) {
        return;
    }

    if (it->second.file) {
        fflush(it->second.file);
        fclose(it->second.file);
    }

    Logs().erase(it);
}

void STORMAPI SLogFlush(HSLOG log) {
    std::lock_guard<std::mutex> lock(LogMutex());
    auto it = Logs().find(reinterpret_cast<uintptr_t>(log));

    if (it != Logs().end() && it->second.file) {
        fflush(it->second.file);
    }
}

void STORMAPI SLogFlushAll() {
    std::lock_guard<std::mutex> lock(LogMutex());

    for (auto& [_, record] : Logs()) {
        if (record.file) {
            fflush(record.file);
        }
    }
}

void STORMAPI SLogVWrite(HSLOG log, const char* format, va_list arglist) {
    std::lock_guard<std::mutex> lock(LogMutex());
    auto it = Logs().find(reinterpret_cast<uintptr_t>(log));

    if (it == Logs().end()) {
        return;
    }

    LogRecord& record = it->second;

    if (!record.file && !OpenLogFile(record)) {
        return;
    }

    std::string prefix = record.timestamp ? CurrentLogTimestamp() : std::string();
    std::string indent(record.indent > 0 ? std::min(record.indent, 128) : 0, ' ');

    if (!prefix.empty()) {
        fwrite(prefix.data(), 1, prefix.size(), record.file);
    }

    if (!indent.empty()) {
        fwrite(indent.data(), 1, indent.size(), record.file);
    }

    if (g_opt.echotooutputdebugstring) {
        va_list copy;
        va_copy(copy, arglist);
        vfprintf(stderr, format ? format : "", copy);
        va_end(copy);
        fputc('\n', stderr);
    }

    vfprintf(record.file, format ? format : "", arglist);
    fwrite("\r\n", 1, 2, record.file);
}

void SLogWrite(HSLOG log, const char* format, ...) {
    va_list args;
    va_start(args, format);
    SLogVWrite(log, format, args);
    va_end(args);
}

void SErrInitialize() {
    g_opt.serrleaksilentwarning = 1;
    uint32_t noMiniDumps = 0;
    SRegLoadValue("Internal", "No Minidumps", 0, &noMiniDumps);
}

int32_t STORMAPI SErrDestroy() {
    std::lock_guard<std::mutex> lock(ErrorMutex());
    ErrorHandlers().clear();
    DisplayingError() = 0;
    return 1;
}

void STORMAPI SErrSetLastError(uint32_t errorcode) {
    LastStormError() = errorcode;
    errno = static_cast<int>(errorcode);
}

uint32_t STORMAPI SErrGetLastError() {
    return LastStormError();
}

int32_t STORMAPI SErrIsDisplayingError() {
    return DisplayingError();
}

void STORMAPI SErrPrepareAppFatal(const char* filename, int32_t linenumber) {
    std::lock_guard<std::mutex> lock(ErrorMutex());
    FatalInfo().filename = filename;
    FatalInfo().line = linenumber;
    FatalInfo().threadId = SGetCurrentThreadId();
}

void STORMAPI SErrSetLogTitleString(const char* title) {
    std::lock_guard<std::mutex> lock(ErrorMutex());
    SStrCopy(LogTitle(), title ? title : "", sizeof(LogTitle()));
}

void STORMAPI SErrSetLogCallback(int32_t (STORMAPI* callback)(char*, uint32_t)) {
    std::lock_guard<std::mutex> lock(ErrorMutex());
    LogCallback() = callback;
}

int32_t STORMAPI SErrGetLogLastPath(char* buf, uint32_t size) {
    if (!buf || !size) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    std::lock_guard<std::mutex> lock(ErrorMutex());

    if (!LogLastPath()[0]) {
        *buf = 0;
        return 0;
    }

    SStrCopy(buf, LogLastPath(), size);
    return 1;
}

int32_t STORMAPI SErrGetErrorStr(uint32_t errorcode, char* buffer, uint32_t bufferchars) {
    if (!buffer || !bufferchars) {
        SErrSetLastError(kErrorInvalidParameter);
        return 0;
    }

    if (errorcode == 108) {
        SStrCopy(buffer, "Invalid or corrupt archive.\n", bufferchars);
        return 1;
    }

    const char* stormText = StormErrorText(errorcode);

    if (stormText[0]) {
        SStrCopy(buffer, stormText, bufferchars);
        return 1;
    }

    const char* systemText = strerror(static_cast<int>(errorcode));

    if (systemText && *systemText) {
        SStrCopy(buffer, systemText, bufferchars);
        return 1;
    }

    *buffer = 0;
    return 0;
}

void STORMAPI SErrRegisterHandler(int32_t (STORMAPI* handler)(uint32_t, const char*, const char*, int32_t, const char*)) {
    std::lock_guard<std::mutex> lock(ErrorMutex());
    ErrorHandlers().push_back(handler);
}

int32_t STORMAPI SErrDisplayError(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable, uint32_t exitcode, uint32_t) {
    if (DisplayingError()) {
        return 0;
    }

    DisplayingError() = 1;

    char errorstr[256] = {};
    SErrGetErrorStr(errorcode, errorstr, sizeof(errorstr));

    if (!errorstr[0]) {
        SStrPrintf(errorstr, sizeof(errorstr), "ERROR #%u (0x%08x) %s", errorcode, errorcode, StormErrorText(errorcode));
    }

    char outstr[4096];

    if (filename && *filename && linenumber > 0) {
        SStrPrintf(outstr, sizeof(outstr), "%s(%d) : error %u: %s\n%s", filename, linenumber, errorcode, errorstr, description ? description : "");
    } else if (filename && *filename) {
        SStrPrintf(outstr, sizeof(outstr), "%s : error %u: %s\n%s", filename, errorcode, errorstr, description ? description : "");
    } else {
        SStrPrintf(outstr, sizeof(outstr), "error %u: %s\n%s", errorcode, errorstr, description ? description : "");
    }

    fputs(outstr, stderr);
    fputc('\n', stderr);

    bool handled = false;

    {
        std::lock_guard<std::mutex> lock(ErrorMutex());

        for (auto handler : ErrorHandlers()) {
            if (handler && !handler(errorcode, errorstr, filename, linenumber, description ? description : "")) {
                handled = true;
                break;
            }
        }
    }

    if (!g_opt.serrsuppresslogs) {
        WriteErrorLog(outstr);
    }

    if (!recoverable) {
        DisplayingError() = 0;
        StormExitProcess(exitcode ? static_cast<int32_t>(exitcode) : 1);
    }

    DisplayingError() = 0;
    return handled ? 0 : 1;
}

int32_t SErrDisplayErrorFmt(uint32_t errorcode, const char* filename, int32_t linenumber, int32_t recoverable, uint32_t exitcode, const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    FormatV(buffer, sizeof(buffer), format, args);
    va_end(args);
    return SErrDisplayError(errorcode, filename, linenumber, buffer, recoverable, exitcode, 0x11111111);
}

void SErrDisplayAppFatalCustomV(uint32_t errorcode, const char* format, va_list args) {
    const char* filename = nullptr;
    int32_t line = 0;

    {
        std::lock_guard<std::mutex> lock(ErrorMutex());

        if (FatalInfo().threadId == SGetCurrentThreadId()) {
            filename = FatalInfo().filename;
            line = FatalInfo().line;
            FatalInfo() = {};
        }
    }

    char buffer[2048];
    FormatV(buffer, sizeof(buffer), format, args);
    SErrDisplayError(errorcode, filename, line, buffer, 0, 1, 0x11111111);
    StormExitProcess(1);
}

void SErrDisplayAppFatalCustom(uint32_t errorcode, const char* format, ...) {
    va_list args;
    va_start(args, format);
    SErrDisplayAppFatalCustomV(errorcode, format, args);
    va_end(args);
}

void SErrDisplayAppFatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    SErrDisplayAppFatalCustomV(STORM_ERROR_FATAL_CONDITION, format, args);
    va_end(args);
}

void SErrDisplayAppFatalMessage(const char* message, const char* filename, int32_t linenumber) {
    SErrPrepareAppFatal(filename, linenumber);
    SErrDisplayAppFatal("%s", message ? message : "");
    __builtin_unreachable();
}

void STORMAPI SErrCatchUnhandledExceptions() {
    signal(SIGPIPE, SIG_IGN);
}

void STORMAPI SErrReportNamedResourceLeak(const char* handlename, const char* resourcename) {
    if (g_opt.serrleaksilentwarning) {
        fprintf(stderr, "Storm Error : handle never released -- %s%s%s%s\n", handlename ? handlename : "", resourcename && *resourcename ? " (" : "", resourcename && *resourcename ? resourcename : "", resourcename && *resourcename ? ")" : "");
    } else {
        char message[256];
        SStrPrintf(message, sizeof(message), "%s%s%s%s", handlename ? handlename : "", resourcename && *resourcename ? " (" : "", resourcename && *resourcename ? resourcename : "", resourcename && *resourcename ? ")" : "");
        SErrDisplayError(STORM_ERROR_RESOURCES_NOT_RELEASED, message, SLOG_HANDLE, nullptr, 1, 1, 0x11111111);
    }
}

int32_t STORMAPI SMsgDestroy() {
    return 1;
}

HWND__* STORMAPI SMsgGetDefaultWindow() {
    return nullptr;
}

void SDebugPrintf(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    FormatV(buffer, sizeof(buffer), format, args);
    va_end(args);
    fputs(buffer, stderr);
}

void SServerInitialize() {
}

void SServerDestroy() {
}

#endif
