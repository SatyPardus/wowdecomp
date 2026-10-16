#if defined(__APPLE__)

#include "ThreadStack.h"  // Engine/Source/Platform/Compat (global include dir on non-Windows)
#include "fmod_settings.h"

#include "fmod.h"
#include "fmod_memory.h"
#include "fmod_os_misc.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct FMOD_OS_CRITICALSECTION
{
    pthread_mutex_t mutex;
};

struct FMOD_OS_SEMAPHORE
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    unsigned int    count;
};

struct FMOD_OS_LIBRARY
{
    void *module;
};

struct FMOD_OS_THREAD_HANDLE
{
    pthread_t thread;
};

struct FMOD_OS_THREAD_START
{
    THREAD_RETURNTYPE (THREAD_CALLCONV *func)(void *param);
    void *param;
    char name[64];
};

static FMOD_OS_CRITICALSECTION gMemoryCrit;

int gSizeofCriticalSection = sizeof(FMOD_OS_CRITICALSECTION);
int gSizeofSemaphore       = sizeof(FMOD_OS_SEMAPHORE);

static int FMOD_OS_FileHandleToFD(void *handle)
{
    return (int)((intptr_t)handle - 1);
}

static void *FMOD_OS_FDToFileHandle(int fd)
{
    return (void *)(intptr_t)(fd + 1);
}

static bool FMOD_OS_UnicodeToUTF8(const char *name, char *buffer, size_t bufferlen)
{
    if (!name || !buffer || !bufferlen)
    {
        return false;
    }

    const uint16_t *in = (const uint16_t *)name;
    size_t out = 0;

    while (*in)
    {
        uint32_t ch = *in++;
        if (ch >= 0xD800 && ch <= 0xDBFF && *in >= 0xDC00 && *in <= 0xDFFF)
        {
            ch = 0x10000 + ((ch - 0xD800) << 10) + (*in++ - 0xDC00);
        }

        if (ch < 0x80)
        {
            if (out + 1 >= bufferlen) return false;
            buffer[out++] = (char)ch;
        }
        else if (ch < 0x800)
        {
            if (out + 2 >= bufferlen) return false;
            buffer[out++] = (char)(0xC0 | (ch >> 6));
            buffer[out++] = (char)(0x80 | (ch & 0x3F));
        }
        else if (ch < 0x10000)
        {
            if (out + 3 >= bufferlen) return false;
            buffer[out++] = (char)(0xE0 | (ch >> 12));
            buffer[out++] = (char)(0x80 | ((ch >> 6) & 0x3F));
            buffer[out++] = (char)(0x80 | (ch & 0x3F));
        }
        else
        {
            if (out + 4 >= bufferlen) return false;
            buffer[out++] = (char)(0xF0 | (ch >> 18));
            buffer[out++] = (char)(0x80 | ((ch >> 12) & 0x3F));
            buffer[out++] = (char)(0x80 | ((ch >> 6) & 0x3F));
            buffer[out++] = (char)(0x80 | (ch & 0x3F));
        }
    }

    buffer[out] = 0;
    return true;
}

void *FMOD_OS_Memory_Alloc(int size, FMOD_MEMORY_TYPE type)
{
    (void)type;
    return malloc(size);
}

void *FMOD_OS_Memory_Realloc(void *ptr, int size, FMOD_MEMORY_TYPE type)
{
    (void)type;
    return realloc(ptr, size);
}

void FMOD_OS_Memory_Free(void *ptr, FMOD_MEMORY_TYPE type)
{
    (void)type;
    free(ptr);
}

int FMOD_OS_GetMemoryUsed()
{
    return 0;
}

FMOD_RESULT FMOD_OS_File_DriveStatus()
{
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Open(const char *name, char *mode, int unicode, unsigned int *filesize, void **handle)
{
    (void)mode;

    if (!name || !filesize || !handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    char path[4096];
    const char *openname = name;
    if (unicode)
    {
        if (!FMOD_OS_UnicodeToUTF8(name, path, sizeof(path)))
        {
            return FMOD_ERR_FILE_BAD;
        }
        openname = path;
    }

    int fd = open(openname, O_RDONLY);
    if (fd < 0)
    {
        if (errno == ENOENT || errno == ENOTDIR || errno == EINVAL)
        {
            return FMOD_ERR_FILE_NOTFOUND;
        }
        return FMOD_ERR_FILE_BAD;
    }

    struct stat st;
    if (fstat(fd, &st) != 0)
    {
        close(fd);
        return FMOD_ERR_FILE_BAD;
    }

    *filesize = (unsigned int)st.st_size;
    *handle = FMOD_OS_FDToFileHandle(fd);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Close(void *handle)
{
    int fd = FMOD_OS_FileHandleToFD(handle);
    if (fd < 0)
    {
        return FMOD_ERR_FILE_BAD;
    }

    if (close(fd) != 0)
    {
        return FMOD_ERR_FILE_BAD;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Cancel(void *handle)
{
    (void)handle;
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Read(void *handle, void *buf, unsigned int count, unsigned int *read)
{
    int fd = FMOD_OS_FileHandleToFD(handle);
    if (fd < 0 || !buf || !read)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    ssize_t rd = ::read(fd, buf, count);
    if (rd < 0)
    {
        return FMOD_ERR_FILE_BAD;
    }

    *read = (unsigned int)rd;
    if ((unsigned int)rd != count)
    {
        return FMOD_ERR_FILE_EOF;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Seek(void *handle, unsigned int offset)
{
    int fd = FMOD_OS_FileHandleToFD(handle);
    if (fd < 0)
    {
        return FMOD_ERR_FILE_BAD;
    }

    if (lseek(fd, offset, SEEK_SET) < 0)
    {
        return FMOD_ERR_FILE_BAD;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Debug_OutputStr(const char *s)
{
    if (s)
    {
        fputs(s, stderr);
    }
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_GetNs(unsigned int *ns)
{
    if (!ns)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t us = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    *ns = (unsigned int)us;

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_GetMs(unsigned int *ms)
{
    if (!ms)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t millis = (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
    *ms = (unsigned int)millis;

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_Sleep(unsigned int ms)
{
    usleep(ms * 1000);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Thread_GetCurrentID(FMOD_UINT_NATIVE *id)
{
    if (!id)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    uint64_t tid = 0;
    pthread_threadid_np(NULL, &tid);
    *id = (FMOD_UINT_NATIVE)tid;

    return FMOD_OK;
}

static void *FMOD_OS_Thread_Trampoline(void *param)
{
    FMOD_OS_THREAD_START *start = (FMOD_OS_THREAD_START *)param;

    if (start->name[0])
    {
        pthread_setname_np(start->name);
    }

    THREAD_RETURNTYPE (THREAD_CALLCONV *func)(void *) = start->func;
    void *userparam = start->param;
    free(start);

    uintptr_t result = func(userparam);
    return (void *)result;
}

FMOD_RESULT FMOD_OS_Thread_Create(const char *name, THREAD_RETURNTYPE (THREAD_CALLCONV *func)(void *param), void *param, FMOD_THREAD_PRIORITY priority, void *stack, int stacksize, void **handle)
{
    (void)priority;
    (void)stack;

    if (!func || !handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_OS_THREAD_HANDLE *thread = (FMOD_OS_THREAD_HANDLE *)malloc(sizeof(FMOD_OS_THREAD_HANDLE));
    FMOD_OS_THREAD_START *start = (FMOD_OS_THREAD_START *)malloc(sizeof(FMOD_OS_THREAD_START));
    if (!thread || !start)
    {
        free(thread);
        free(start);
        return FMOD_ERR_MEMORY;
    }

    start->func = func;
    start->param = param;
    if (name)
    {
        strncpy(start->name, name, sizeof(start->name) - 1);
        start->name[sizeof(start->name) - 1] = 0;
    }
    else
    {
        start->name[0] = 0;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, WowThreadStackSize(stacksize > 0 ? (size_t)stacksize : 0));

    int rc = pthread_create(&thread->thread, &attr, FMOD_OS_Thread_Trampoline, start);
    pthread_attr_destroy(&attr);
    if (rc != 0)
    {
        free(thread);
        free(start);
        return FMOD_ERR_MEMORY;
    }

    *handle = thread;
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Thread_Destroy(void *handle)
{
    if (!handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_OS_THREAD_HANDLE *thread = (FMOD_OS_THREAD_HANDLE *)handle;
    pthread_join(thread->thread, NULL);
    free(thread);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Create(FMOD_OS_CRITICALSECTION **crit, bool memorycrit)
{
    if (!crit)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (memorycrit)
    {
        *crit = &gMemoryCrit;
    }
    else
    {
        *crit = (FMOD_OS_CRITICALSECTION *)FMOD_Memory_Alloc(sizeof(FMOD_OS_CRITICALSECTION));
        if (!*crit)
        {
            return FMOD_ERR_MEMORY;
        }
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int rc = pthread_mutex_init(&(*crit)->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    if (rc != 0)
    {
        if (!memorycrit)
        {
            FMOD_Memory_Free(*crit);
        }
        *crit = 0;
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Free(FMOD_OS_CRITICALSECTION *crit)
{
    if (!crit)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    pthread_mutex_destroy(&crit->mutex);
    if (crit != &gMemoryCrit)
    {
        FMOD_Memory_Free(crit);
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Enter(FMOD_OS_CRITICALSECTION *crit)
{
    if (!crit)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&crit->mutex);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Leave(FMOD_OS_CRITICALSECTION *crit)
{
    if (!crit)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    pthread_mutex_unlock(&crit->mutex);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Create(FMOD_OS_SEMAPHORE **sema)
{
    if (!sema)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *sema = (FMOD_OS_SEMAPHORE *)FMOD_Memory_Alloc(sizeof(FMOD_OS_SEMAPHORE));
    if (!*sema)
    {
        return FMOD_ERR_MEMORY;
    }

    pthread_mutex_init(&(*sema)->mutex, NULL);
    pthread_cond_init(&(*sema)->cond, NULL);
    (*sema)->count = 0;

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Free(FMOD_OS_SEMAPHORE *sema)
{
    if (!sema)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    pthread_cond_destroy(&sema->cond);
    pthread_mutex_destroy(&sema->mutex);
    FMOD_Memory_Free(sema);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Wait(FMOD_OS_SEMAPHORE *sema)
{
    if (!sema)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&sema->mutex);
    while (sema->count == 0)
    {
        pthread_cond_wait(&sema->cond, &sema->mutex);
    }
    sema->count--;
    pthread_mutex_unlock(&sema->mutex);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Signal(FMOD_OS_SEMAPHORE *sema, bool interrupt)
{
    (void)interrupt;

    if (!sema)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&sema->mutex);
    sema->count++;
    pthread_cond_signal(&sema->cond);
    pthread_mutex_unlock(&sema->mutex);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_Load(const char *dllname, FMOD_OS_LIBRARY **handle)
{
    if (!dllname || !handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_OS_LIBRARY *library = (FMOD_OS_LIBRARY *)FMOD_Memory_Alloc(sizeof(FMOD_OS_LIBRARY));
    if (!library)
    {
        return FMOD_ERR_MEMORY;
    }

    library->module = dlopen(dllname, RTLD_NOW | RTLD_LOCAL);
    if (!library->module)
    {
        FMOD_Memory_Free(library);
        return FMOD_ERR_FILE_NOTFOUND;
    }

    *handle = library;
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_GetProcAddress(FMOD_OS_LIBRARY *handle, const char *procname, void **address)
{
    if (!handle || !procname || !address)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *address = dlsym(handle->module, procname);
    if (!*address)
    {
        return FMOD_ERR_FORMAT;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_Free(FMOD_OS_LIBRARY *handle)
{
    if (!handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (dlclose(handle->module) != 0)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_Memory_Free(handle);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CheckDriverList(bool *devicelistchanged)
{
    if (!devicelistchanged)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *devicelistchanged = false;
    return FMOD_OK;
}

#endif
