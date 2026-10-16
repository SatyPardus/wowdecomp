#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOUNITCONTAINER_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOUNITCONTAINER_H

#include <cstddef>
#include <cstdint>

class IOUnitContainer {
    public:
    static void* Alloc(IOUnitContainer* container, uint32_t size, bool* ownsMemory);
    static void Free(void* memory, bool ownsMemory);

    /* 0000 */ uint8_t* m_next;
    /* 0004 */ bool m_owner;
    /* 0005 */ uint8_t unused_0005[3];

    IOUnitContainer(uint32_t size, ...);
};

#endif
