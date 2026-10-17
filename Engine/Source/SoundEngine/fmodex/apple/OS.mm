#include "fmod_settings.h"
#include "fmod_os_misc.h"
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct FMOD_OS_CRITICALSECTION { pthread_mutex_t mutex; };
struct FMOD_OS_SEMAPHORE { pthread_mutex_t mutex; pthread_cond_t condition; unsigned count; };
int gSizeofCriticalSection = sizeof(FMOD_OS_CRITICALSECTION);
int gSizeofSemaphore = sizeof(FMOD_OS_SEMAPHORE);
namespace {
std::atomic<size_t> allocated{0};
struct Thread {
    pthread_t id;
    THREAD_RETURNTYPE (*callback)(void*);
    void* parameter;
    char name[64];
};
void* ThreadEntry(void* opaque) {
    auto* thread = static_cast<Thread*>(opaque);
    pthread_setname_np(thread->name);
    thread->callback(thread->parameter);
    return nullptr;
}
uint64_t MonotonicMicroseconds() {
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return uint64_t(now.tv_sec) * 1000000 + now.tv_nsec / 1000;
}
}
void* FMOD_OS_Memory_Alloc(int size, FMOD_MEMORY_TYPE) {
    if (size < 0) return nullptr;
    void* result = malloc(size);
    if (result) allocated.fetch_add(malloc_size(result), std::memory_order_relaxed);
    return result;
}
void* FMOD_OS_Memory_Realloc(void* pointer, int size, FMOD_MEMORY_TYPE type) {
    if (!pointer) return FMOD_OS_Memory_Alloc(size, type);
    if (size < 0) return nullptr;
    if (!size) { FMOD_OS_Memory_Free(pointer, type); return nullptr; }
    size_t oldSize = malloc_size(pointer);
    void* result = realloc(pointer, size);
    if (result) {
        allocated.fetch_sub(oldSize, std::memory_order_relaxed);
        allocated.fetch_add(malloc_size(result), std::memory_order_relaxed);
    }
    return result;
}
void FMOD_OS_Memory_Free(void* pointer, FMOD_MEMORY_TYPE) {
    if (pointer) allocated.fetch_sub(malloc_size(pointer), std::memory_order_relaxed);
    free(pointer);
}
int FMOD_OS_GetMemoryUsed() {
    size_t size = allocated.load(std::memory_order_relaxed);
    return size > INT_MAX ? INT_MAX : int(size);
}
FMOD_RESULT FMOD_OS_File_Open(const char* name, char*, int unicode, unsigned* size, void** handle) {
    if (!name || !size || !handle) return FMOD_ERR_INVALID_PARAM;
    *handle = nullptr;
    *size = 0;
    @autoreleasepool {
        NSString* path;
        if (unicode) {
            const auto* utf16 = reinterpret_cast<const unichar*>(name);
            size_t length = 0;
            while (utf16[length]) ++length;
            path = [[NSString alloc] initWithCharacters:utf16 length:length];
        } else path = [NSString stringWithUTF8String:name];
        if (!path) return FMOD_ERR_FILE_BAD;
        FILE* file = fopen(path.fileSystemRepresentation, "rb");
        if (!file) return errno == ENOENT || errno == ENOTDIR ? FMOD_ERR_FILE_NOTFOUND : FMOD_ERR_FILE_BAD;
        struct stat info{};
        if (fstat(fileno(file), &info) || info.st_size < 0 || uint64_t(info.st_size) > UINT_MAX || !S_ISREG(info.st_mode)) {
            fclose(file);
            return FMOD_ERR_FILE_BAD; // This FMOD file interface has a 32-bit length contract.
        }
        *size = static_cast<unsigned>(info.st_size);
        *handle = file;
    }
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_File_Close(void* handle) {
    return handle && !fclose(static_cast<FILE*>(handle)) ? FMOD_OK : FMOD_ERR_FILE_BAD;
}
FMOD_RESULT FMOD_OS_File_Read(void* handle, void* buffer, unsigned count, unsigned* read) {
    if (!handle || !read || (!buffer && count)) return FMOD_ERR_INVALID_PARAM;
    FILE* file = static_cast<FILE*>(handle);
    *read = static_cast<unsigned>(fread(buffer, 1, count, file));
    return *read == count ? FMOD_OK : ferror(file) ? FMOD_ERR_FILE_BAD : FMOD_ERR_FILE_EOF;
}
FMOD_RESULT FMOD_OS_File_Seek(void* handle, unsigned offset) {
    return handle && !fseeko(static_cast<FILE*>(handle), offset, SEEK_SET) ? FMOD_OK : FMOD_ERR_FILE_COULDNOTSEEK;
}
FMOD_RESULT FMOD_OS_File_Cancel(void*) { return FMOD_OK; } // Reads are synchronous.
FMOD_RESULT FMOD_OS_File_DriveStatus() { return FMOD_OK; }
FMOD_RESULT FMOD_OS_Debug_OutputStr(const char* message) {
    if (!message) return FMOD_ERR_INVALID_PARAM;
    fputs(message, stderr);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Time_GetNs(unsigned* value) {
    if (!value) return FMOD_ERR_INVALID_PARAM;
    // Despite its historical name, the Windows implementation returns microseconds.
    *value = static_cast<unsigned>(MonotonicMicroseconds());
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Time_GetMs(unsigned* value) {
    if (!value) return FMOD_ERR_INVALID_PARAM;
    *value = static_cast<unsigned>(MonotonicMicroseconds() / 1000);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Time_Sleep(unsigned ms) {
    timespec delay{time_t(ms / 1000), long(ms % 1000) * 1000000};
    while (nanosleep(&delay, &delay) && errno == EINTR) {}
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Thread_GetCurrentID(FMOD_UINT_NATIVE* id) {
    if (!id) return FMOD_ERR_INVALID_PARAM;
    uint64_t value = 0;
    if (pthread_threadid_np(nullptr, &value)) return FMOD_ERR_INTERNAL;
    *id = static_cast<FMOD_UINT_NATIVE>(value);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Thread_Create(const char* name, THREAD_RETURNTYPE (*callback)(void*), void* parameter,
                                FMOD_THREAD_PRIORITY priority, void* stack, int stackSize, void** handle) {
    if (!callback || !handle || stackSize < 0) return FMOD_ERR_INVALID_PARAM;
    *handle = nullptr;
    auto* thread = static_cast<Thread*>(calloc(1, sizeof(Thread)));
    if (!thread) return FMOD_ERR_MEMORY;
    thread->callback = callback;
    thread->parameter = parameter;
    snprintf(thread->name, sizeof(thread->name), "%s", name ? name : "FMOD");
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int error = 0;
    if (stack) error = pthread_attr_setstack(&attr, stack, stackSize);
    else {
        // Win32 ignored the legacy 64 KiB hints and used its 1 MiB default.
        // Decoder callbacks enter the 64-bit MPQ/file stack as well as FMOD.
        size_t nativeSize = stackSize > 1024 * 1024 ? size_t(stackSize) : 1024 * 1024;
        error = pthread_attr_setstacksize(&attr, (nativeSize + 16383) & ~size_t(16383));
    }
    if (!error) error = pthread_attr_set_qos_class_np(&attr, priority >= FMOD_THREAD_PRIORITY_HIGH ? QOS_CLASS_USER_INITIATED :
                                                    priority < FMOD_THREAD_PRIORITY_NORMAL ? QOS_CLASS_UTILITY : QOS_CLASS_DEFAULT, 0);
    if (!error) error = pthread_create(&thread->id, &attr, ThreadEntry, thread);
    pthread_attr_destroy(&attr);
    if (error) { free(thread); return FMOD_ERR_INTERNAL; }
    *handle = thread;
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Thread_Destroy(void* handle) {
    if (!handle) return FMOD_ERR_INVALID_PARAM;
    auto* thread = static_cast<Thread*>(handle);
    if (pthread_join(thread->id, nullptr)) return FMOD_ERR_INTERNAL;
    free(thread);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_CriticalSection_Create(FMOD_OS_CRITICALSECTION** out, bool) {
    if (!out) return FMOD_ERR_INVALID_PARAM;
    *out = nullptr;
    // Allocate outside FMOD: its memory pool itself needs this lock.
    auto* crit = static_cast<FMOD_OS_CRITICALSECTION*>(malloc(sizeof(FMOD_OS_CRITICALSECTION)));
    if (!crit) return FMOD_ERR_MEMORY;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int error = pthread_mutex_init(&crit->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (error) { free(crit); return FMOD_ERR_INTERNAL; }
    *out = crit;
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_CriticalSection_Free(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) return FMOD_ERR_INVALID_PARAM;
    if (pthread_mutex_destroy(&crit->mutex)) return FMOD_ERR_INTERNAL;
    free(crit);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_CriticalSection_Enter(FMOD_OS_CRITICALSECTION* crit) {
    return crit && !pthread_mutex_lock(&crit->mutex) ? FMOD_OK : FMOD_ERR_INVALID_PARAM;
}
FMOD_RESULT FMOD_OS_CriticalSection_Leave(FMOD_OS_CRITICALSECTION* crit) {
    return crit && !pthread_mutex_unlock(&crit->mutex) ? FMOD_OK : FMOD_ERR_INVALID_PARAM;
}
FMOD_RESULT FMOD_OS_Semaphore_Create(FMOD_OS_SEMAPHORE** out) {
    if (!out) return FMOD_ERR_INVALID_PARAM;
    *out = nullptr;
    auto* sema = static_cast<FMOD_OS_SEMAPHORE*>(calloc(1, sizeof(FMOD_OS_SEMAPHORE)));
    if (!sema) return FMOD_ERR_MEMORY;
    if (pthread_mutex_init(&sema->mutex, nullptr)) { free(sema); return FMOD_ERR_INTERNAL; }
    if (pthread_cond_init(&sema->condition, nullptr)) { pthread_mutex_destroy(&sema->mutex); free(sema); return FMOD_ERR_INTERNAL; }
    *out = sema;
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Semaphore_Free(FMOD_OS_SEMAPHORE* sema) {
    if (!sema) return FMOD_ERR_INVALID_PARAM;
    if (pthread_cond_destroy(&sema->condition)) return FMOD_ERR_INTERNAL;
    pthread_mutex_destroy(&sema->mutex);
    free(sema);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Semaphore_Wait(FMOD_OS_SEMAPHORE* sema) {
    if (!sema) return FMOD_ERR_INVALID_PARAM;
    pthread_mutex_lock(&sema->mutex);
    int error = 0;
    while (!sema->count && !error) error = pthread_cond_wait(&sema->condition, &sema->mutex);
    if (!error) --sema->count;
    pthread_mutex_unlock(&sema->mutex);
    return error ? FMOD_ERR_INTERNAL : FMOD_OK;
}
FMOD_RESULT FMOD_OS_Semaphore_Signal(FMOD_OS_SEMAPHORE* sema, bool) {
    if (!sema) return FMOD_ERR_INVALID_PARAM;
    pthread_mutex_lock(&sema->mutex);
    if (sema->count == UINT_MAX) { pthread_mutex_unlock(&sema->mutex); return FMOD_ERR_INTERNAL; }
    ++sema->count;
    pthread_cond_signal(&sema->condition);
    pthread_mutex_unlock(&sema->mutex);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Library_Load(const char* path, FMOD_OS_LIBRARY** handle) {
    if (!path || !handle) return FMOD_ERR_INVALID_PARAM;
    *handle = nullptr;
#if TARGET_OS_IPHONE
    return FMOD_ERR_UNSUPPORTED;
#else
    *handle = reinterpret_cast<FMOD_OS_LIBRARY*>(dlopen(path, RTLD_NOW | RTLD_LOCAL));
    return *handle ? FMOD_OK : FMOD_ERR_PLUGIN_MISSING;
#endif
}
FMOD_RESULT FMOD_OS_Library_GetProcAddress(FMOD_OS_LIBRARY* handle, const char* name, void** address) {
    if (!handle || !name || !address) return FMOD_ERR_INVALID_PARAM;
    *address = dlsym(handle, name);
    return *address ? FMOD_OK : FMOD_ERR_PLUGIN_MISSING;
}
FMOD_RESULT FMOD_OS_Library_Free(FMOD_OS_LIBRARY* handle) {
    return handle && !dlclose(handle) ? FMOD_OK : FMOD_ERR_PLUGIN;
}
