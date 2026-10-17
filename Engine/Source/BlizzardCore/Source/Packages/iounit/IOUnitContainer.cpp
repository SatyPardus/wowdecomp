#include <cstdarg>
#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnitContainer.h"
#include "Engine/Source/Storm/H/storm.h"

// IO units contain pointers and int64_t members. Four-byte packing is only
// valid for the original x86 build, not the native 64-bit object layout.
static constexpr size_t s_unitAlignment = sizeof(void*) == 8 ? alignof(std::max_align_t) : 4;

// VERIFY: 8efaa12c manual
// OFFSET: 0x0042B9C0
IOUnitContainer::IOUnitContainer(uint32_t size, ...) {
    size_t total = (size + s_unitAlignment - 1) & ~(s_unitAlignment - 1);

    va_list args;
    va_start(args, size);

    for (uint32_t next = va_arg(args, uint32_t); next; next = va_arg(args, uint32_t)) {
        total = (next + total + s_unitAlignment - 1) & ~(s_unitAlignment - 1);
    }

    va_end(args);

    this->m_next = static_cast<uint8_t*>(SMemAlloc(total, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY));
    this->m_owner = true;
}

// VERIFY: b37dd59c manual
// OFFSET: 0x0042BA10
void* IOUnitContainer::Alloc(IOUnitContainer* container, uint32_t size, bool* ownsMemory) {
    if (!container) {
        *ownsMemory = true;
        return SMemAlloc(size, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY);
    }

    uint8_t* memory = container->m_next;
    container->m_next = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(memory) + size + s_unitAlignment - 1) & ~(s_unitAlignment - 1));
    *ownsMemory = container->m_owner;
    container->m_owner = false;

    return memory;
}

// INLINED
void IOUnitContainer::Free(void* memory, bool ownsMemory) {
    if (ownsMemory) {
        SMemFree(memory, __FILE__, __LINE__, 0);
    }
}
