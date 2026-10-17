#if defined(__APPLE__)

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include "Engine/Source/Os/OsMemory.h"

// Model data references these buffers through self-relative 32-bit offsets (M2Array), so they
// must stay within +/-2 GB of the anchor. Matches the Win64 search range.
static const mach_vm_address_t s_nearRange = 0x60000000;

static std::mutex& s_nearMutex = *new std::mutex;  // never destroyed: still used by threads during exit()
static std::unordered_map<void*, mach_vm_size_t>& s_nearSizes = *new std::unordered_map<void*, mach_vm_size_t>;

static bool TryAllocateAt(mach_vm_address_t address, mach_vm_size_t bytes) {
    mach_vm_address_t target = address;
    return mach_vm_allocate(mach_task_self(), &target, bytes, VM_FLAGS_FIXED) == KERN_SUCCESS && target == address;
}

// Walks the gaps between mapped regions in [minimum, maximum) and claims the first one large enough.
static void* AllocateInRange(mach_vm_address_t minimum, mach_vm_address_t maximum, mach_vm_size_t bytes) {
    mach_vm_address_t cursor = minimum;

    while (cursor < maximum && maximum - cursor >= bytes) {
        mach_vm_address_t regionAddress = cursor;
        mach_vm_size_t regionSize = 0;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object = MACH_PORT_NULL;
        kern_return_t result = mach_vm_region(mach_task_self(), &regionAddress, &regionSize, VM_REGION_BASIC_INFO_64, reinterpret_cast<vm_region_info_t>(&info), &count, &object);

        mach_vm_address_t gapEnd = result == KERN_SUCCESS ? regionAddress : maximum;

        if (gapEnd > maximum) {
            gapEnd = maximum;
        }

        if (gapEnd > cursor && gapEnd - cursor >= bytes && TryAllocateAt(cursor, bytes)) {
            return reinterpret_cast<void*>(cursor);
        }

        if (result != KERN_SUCCESS) {
            break;
        }

        cursor = regionAddress + regionSize;
    }

    return nullptr;
}

void* OsAllocNear(const void* anchor, size_t size) {
    mach_vm_size_t bytes = mach_vm_round_page(size ? size : 1);
    mach_vm_address_t center = mach_vm_trunc_page(reinterpret_cast<uintptr_t>(anchor));
    mach_vm_address_t minimum = center > s_nearRange + vm_page_size ? center - s_nearRange : vm_page_size;
    mach_vm_address_t maximum = center + s_nearRange;

    // Prefer space above the anchor, then fall back to the whole window.
    void* ptr = AllocateInRange(center, maximum, bytes);

    if (!ptr) {
        ptr = AllocateInRange(minimum, maximum, bytes);
    }

    if (!ptr) {
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

    mach_vm_size_t bytes = 0;

    {
        std::lock_guard<std::mutex> lock(s_nearMutex);
        auto it = s_nearSizes.find(ptr);

        if (it == s_nearSizes.end()) {
            return;
        }

        bytes = it->second;
        s_nearSizes.erase(it);
    }

    mach_vm_deallocate(mach_task_self(), reinterpret_cast<mach_vm_address_t>(ptr), bytes);
}

#endif
