#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <malloc.h>
#define STORM_MALLOC_SIZE _msize
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#define STORM_MALLOC_SIZE malloc_size
#else
#include <malloc.h>
#define STORM_MALLOC_SIZE malloc_usable_size
#endif
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00ADE850
static int32_t s_exitonfailure = 1;

// STATIC: 0x00ADE854
static int32_t s_heaptracking = 1;

// VERIFY: 56e6df56 auto
// OFFSET: 0x0076E4A0
void STORMAPI SMemSetDebugFlags(uint32_t flags, uint8_t changeMask) {
    if (changeMask & SMEM_DEBUG_NOEXITONFAILURE) {
        s_exitonfailure = (flags & SMEM_DEBUG_NOEXITONFAILURE) == 0;
    }

    if (changeMask & SMEM_DEBUG_HEAPTRACKING) {
        s_heaptracking = (flags & SMEM_DEBUG_HEAPTRACKING) != 0;
    }
}

// VERIFY: cd1f46b7 manual
// OFFSET: 0x0076E4E0
static void FatalError(size_t bytes, const char* filename, int32_t linenumber) {
    char buffer[128];
    sprintf(buffer, "Requested %u bytes of memory", static_cast<uint32_t>(bytes));

    SErrDisplayError(STORM_ERROR_NOT_ENOUGH_MEMORY, filename, linenumber, buffer, s_exitonfailure == 0, 1, 0x11111111);

    if (s_exitonfailure) {
        StormExitProcess(1);
    }
}

// VERIFY: a5777d4c auto
// OFFSET: 0x0076E540
void* STORMAPI SMemAlloc(size_t bytes, const char* filename, int32_t linenumber, uint32_t flags) {
    size_t size = (bytes + 7) & ~static_cast<size_t>(7);
    void* ptr = (flags & SMEM_FLAG_ZEROMEMORY) ? calloc(1, size) : malloc(size);

    if (!ptr) {
        FatalError(bytes, filename, linenumber);
        return nullptr;
    }

    STORM_MALLOC_SIZE(ptr);

    return ptr;
}

// VERIFY: fab6c035 auto
// OFFSET: 0x0076E5A0
int32_t STORMAPI SMemFree(void* ptr, const char* filename, int32_t linenumber, uint32_t flags) {
    if (ptr) {
        STORM_MALLOC_SIZE(ptr);
        free(ptr);
    }

    return 1;
}

// VERIFY: 19b70aac auto
// OFFSET: 0x0076E5D0
int32_t STORMAPI SMemIsValidPointer(void* ptr, size_t bytes, uint32_t flags) {
    return 1;
}

// VERIFY: 26d3182e manual
// OFFSET: 0x0076E5E0
void* STORMAPI SMemReAlloc(void* ptr, size_t bytes, const char* filename, int32_t linenumber, uint32_t flags) {
    if (flags == SMEM_REALLOC_NULL) {
        return nullptr;
    }

    if (!ptr) {
        return SMemAlloc(bytes, filename, linenumber, flags);
    }

    size_t oldsize = STORM_MALLOC_SIZE(ptr);

    if (flags & SMEM_FLAG_NOREALLOC) {
        return nullptr;
    }

    size_t size = (bytes + 7) & ~static_cast<size_t>(7);
    void* newptr = realloc(ptr, size);

    if (!newptr) {
        if (size) {
            FatalError(bytes, filename, linenumber);
        }

        return nullptr;
    }

    size_t newsize = STORM_MALLOC_SIZE(newptr);

    if (s_heaptracking && newptr != ptr) {
        DebugLog(-1, "[%08lx] REALLOC %10d bytes <-- [%08lx]/%d, requested=%d (%s: %d)\n", newptr, newsize, ptr, oldsize, bytes, filename, linenumber);
    }

    if (oldsize < bytes && (flags & SMEM_FLAG_ZEROMEMORY)) {
        memset(static_cast<uint8_t*>(newptr) + oldsize, 0, bytes - oldsize);
    }

    return newptr;
}

// VERIFY: a7926bc3 auto
// OFFSET: 0x00401010
void* operator new(size_t bytes) {
    return SMemAlloc(bytes, "new", SLOG_FUNCTION, 0);
}

// VERIFY: 79127843 auto
// OFFSET: 0x0047CC90
void operator delete(void* ptr) noexcept {
    if (ptr) {
        SMemFree(ptr, "delete", SLOG_FUNCTION, 0);
    }
}

// VERIFY: 5c89935c auto
// OFFSET: 0x00401030
void operator delete[](void* ptr) noexcept {
    if (ptr) {
        SMemFree(ptr, "delete[]", SLOG_FUNCTION, 0);
    }
}

int32_t STORMAPI SMemDestroy() {
    return 1;
}

void STORMAPI SMemInitialize() {
}
