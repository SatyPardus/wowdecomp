#include <cstddef>
#include <cstdint>
#include "WoW/Source/ObjectAlloc/HeapUsage.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/ObjectAlloc/ObjectAlloc.h"

// VERIFY: ac858bec manual
// OFFSET: 0x004D27E0
static int32_t HeapUsage(const char* command, const char* arguments) {
    char buffer[50000];
    char line[2048];

    OBJALLOCGLOBALS* globals = GetObjAllocGlobals();
    uint32_t count = globals->objects.Count();
    uint32_t objectBytes = 0;
    uint32_t heapBytes = 0;
    char* str = buffer + SStrPrintf(buffer, sizeof(buffer), "%u Heaps in use:\n", count);

    for (uint32_t i = 0; i < count; i++) {
        CObjectHeapList& list = globals->objects[i];
        list.TotalHeaps();

        uint32_t numAllocs = list.m_numAllocs >= 0xFFFFFFFF ? 0xFFFFFFFF : static_cast<uint32_t>(list.m_numAllocs);
        uint32_t totalBytes = list.TotalHeaps() * list.m_objSize * list.m_objsPerBlock;
        uint32_t usedBytes = list.BlocksAllocated() * list.m_objSize;

        str += SStrPrintf(str, buffer + sizeof(buffer) - str, "    \"%24s\" %3u blocks, %5u objs per block (%6u bytes per obj): %6u objs allocated (%3d%%) - Total bytes: %8.3fMb (%8.3fMb), Total Allocs: %d\n", list.m_heapName, list.TotalHeaps(), list.m_objsPerBlock, list.m_objSize, list.BlocksAllocated(), list.PercentUsed(), usedBytes * 0.00000095367432f, totalBytes * 0.00000095367432f, numAllocs);

        objectBytes += globals->objects[i].m_objSize * globals->objects[i].BlocksAllocated();
        heapBytes += globals->objects[i].TotalHeaps() * globals->objects[i].m_objSize * globals->objects[i].m_objsPerBlock;
    }

    SStrPrintf(str, buffer + sizeof(buffer) - str, "%4.3fMb total object bytes used, %4.3fMb bytes allocated for heaps\n", objectBytes * 0.00000095367432f, heapBytes * 0.00000095367432f);
    ReleaseObjAllocGlobals();

    char* dest = line;

    for (const char* src = buffer; *src; src++) {
        if (*src == '\n') {
            *dest = '\0';
            ConsoleWrite(line, DEFAULT_COLOR);
            dest = line;
        } else if (dest < line + sizeof(line) - 1) {
            *dest++ = *src;
        }
    }

    if (dest != line) {
        *dest = '\0';
        ConsoleWrite(line, DEFAULT_COLOR);
    }

    return 1;
}

static int32_t MemUsage(const char* command, const char* arguments) {
    return 1;
}

static int32_t HeapUsage2(const char* command, const char* arguments) {
    return 1;
}

// VERIFY: 267b1512 auto
// OFFSET: 0x004D2A30
void HeapUsageInitialize() {
    ConsoleCommandRegister("HeapUsage", HeapUsage, CATEGORY_GAME, nullptr);
    ConsoleCommandRegister("MemUsage", MemUsage, CATEGORY_GAME, nullptr);
    ConsoleCommandRegister("HeapUsage2", HeapUsage2, CATEGORY_GAME, nullptr);
    s_cvHeapAllocTracking = CVar::Register("heapAllocTracking", "Enables/disables allocation tracking & dumping in SMemMalloc", 0, "1", ChangeHeapAllocTracking, CATEGORY_GAME, false, nullptr, false);
}
