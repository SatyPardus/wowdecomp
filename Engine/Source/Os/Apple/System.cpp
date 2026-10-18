#if defined(__APPLE__)
#include "Engine/Source/Os/System.h"
#include <sys/sysctl.h>
#include <stdexcept>

uint64_t OsGetPhysicalMemory() {
    uint64_t bytes = 0;
    size_t size = sizeof(bytes);
    if (sysctlbyname("hw.memsize", &bytes, &size, nullptr, 0) || size != sizeof(bytes) || !bytes)
        throw std::runtime_error("Cannot query Apple physical memory");
    return bytes;
}
// Native hosts suspend frame callbacks when inactive. Sleeping inside Scrn's
// paint callback would block AppKit/UIKit and delay lifecycle delivery.
int32_t OsSleepInBackground() { return 0; }
uint32_t OsGetBackgroundSleepMs() { return 0; }
#endif
