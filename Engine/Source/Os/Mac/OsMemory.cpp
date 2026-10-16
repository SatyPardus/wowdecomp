#if defined(__APPLE__)

#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <sys/mman.h>
#include <unistd.h>
#include "Engine/Source/Os/OsMemory.h"

static std::mutex& s_nearMutex = *new std::mutex;  // never destroyed: still used by threads during exit()
static std::unordered_map<void*, size_t> s_nearSizes;

void* OsAllocNear(const void* anchor, size_t size) {
    (void)anchor;
    long page = sysconf(_SC_PAGESIZE);
    size_t bytes = (size + static_cast<size_t>(page - 1)) & ~static_cast<size_t>(page - 1);
    void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (ptr == MAP_FAILED) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(s_nearMutex);
    s_nearSizes[ptr] = bytes;
    return ptr;
}

void OsFreeNear(void* ptr) {
    if (!ptr) {
        return;
    }
    size_t bytes = 0;
    {
        std::lock_guard<std::mutex> lock(s_nearMutex);
        auto it = s_nearSizes.find(ptr);
        if (it == s_nearSizes.end()) {
            return;
        }
        bytes = it->second;
        s_nearSizes.erase(it);
    }
    munmap(ptr, bytes);
}

#endif
