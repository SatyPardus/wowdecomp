#include "Engine/Source/Os/OsMemory.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <mach/mach.h>

static_assert(sizeof(vm_address_t) == 8, "Model relative allocation requires a 64-bit Apple target");

namespace {
constexpr vm_size_t NearRange = 0x60000000;
struct alignas(16) Allocation { vm_size_t bytes; };

// Search holes, then claim with FIXED alone. Unlike mmap(MAP_FIXED), this never
// replaces a mapping, including one created by another thread during the scan.
vm_address_t AllocateBetween(vm_address_t begin, vm_address_t end, vm_size_t bytes) {
    const vm_size_t page = vm_page_size;
    auto candidate = (begin + page - 1) & ~(page - 1);
    while (candidate < end && bytes <= end - candidate) {
        vm_address_t region = candidate;
        vm_size_t regionSize = 0;
        vm_region_basic_info_data_64_t info{};
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object = MACH_PORT_NULL;
        const auto result = vm_region_64(mach_task_self(), &region, &regionSize,
            VM_REGION_BASIC_INFO_64, reinterpret_cast<vm_region_info_t>(&info), &count, &object);
        if (object != MACH_PORT_NULL) mach_port_deallocate(mach_task_self(), object);
        if (result != KERN_SUCCESS && result != KERN_INVALID_ADDRESS) return 0;
        const auto holeEnd = result == KERN_INVALID_ADDRESS ? end : std::min(region, end);
        if (holeEnd >= candidate && bytes <= holeEnd - candidate) {
            auto address = candidate;
            if (vm_allocate(mach_task_self(), &address, bytes, VM_FLAGS_FIXED) == KERN_SUCCESS) return address;
            // A concurrent mapping can win the hole. Query the next page again.
            candidate += page;
        } else {
            if (result != KERN_SUCCESS || regionSize > end - region) return 0;
            candidate = region + regionSize;
        }
    }
    return 0;
}
}

void* OsAllocNear(const void* anchor, size_t size) {
    const vm_size_t page = vm_page_size;
    if (!size || size > NearRange - sizeof(Allocation) - page) return nullptr;
    const vm_size_t bytes = (size + sizeof(Allocation) + page - 1) & ~(page - 1);
    vm_address_t address = 0;
    if (!anchor) {
        if (vm_allocate(mach_task_self(), &address, bytes, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) return nullptr;
    } else {
        const auto center = reinterpret_cast<uintptr_t>(anchor);
        const auto lower = center > NearRange ? center - NearRange : page;
        if (center > std::numeric_limits<uintptr_t>::max() - NearRange) return nullptr;
        address = AllocateBetween(center, center + NearRange, bytes);
        if (!address) address = AllocateBetween(lower, center, bytes);
        if (!address) return nullptr;
    }
    auto* allocation = reinterpret_cast<Allocation*>(address);
    allocation->bytes = bytes;
    return allocation + 1;
}

void OsFreeNear(void* ptr) {
    if (!ptr) return;
    auto* allocation = static_cast<Allocation*>(ptr) - 1;
    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(allocation), allocation->bytes);
}
