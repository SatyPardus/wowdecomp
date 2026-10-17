#if defined(__APPLE__)
#include <cstdio>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Storm/H/storm.h"

namespace {
struct TextBounds {
    uintptr_t begin;
    uintptr_t end;
    TextBounds() {
        unsigned long size = 0;
        begin = reinterpret_cast<uintptr_t>(getsectiondata(&_mh_execute_header, "__TEXT", "__text", &size));
        end = begin + size;
    }
};
const TextBounds& Bounds() { static const TextBounds bounds; return bounds; }
}

void OsFindTextSectionBounds() { (void)Bounds(); }
void OsValidateFunctionPointer(const void* ptr) {
    const auto& bounds = Bounds();
    const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    if (!bounds.begin || address < bounds.begin || address >= bounds.end) {
        char message[128];
        snprintf(message, sizeof(message), "Invalid function pointer: %p", ptr);
        SErrDisplayError(STORM_ERROR_FATAL, __FILE__, __LINE__, message, 0, 1, 0);
    }
}
#endif
