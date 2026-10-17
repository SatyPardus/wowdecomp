#ifndef WOWDECOMP_PLATFORM_COMPAT_THREADSTACK_H
#define WOWDECOMP_PLATFORM_COMPAT_THREADSTACK_H

#include <cstddef>
#include <unistd.h>

// Win32 CreateThread treats a requested stack size as the initial commit; the reservation is never smaller
// than the executable's default reserve (1 MB). pthread stack sizes are hard limits (and the macOS default for
// secondary threads is only 512 KB), so callers that asked Windows for a few KB would overflow. Map a request
// to what Windows would actually reserve.
inline size_t WowThreadStackSize(size_t requested) {
    const size_t kWindowsDefaultReserve = 1024 * 1024;
    size_t size = requested > kWindowsDefaultReserve ? requested : kWindowsDefaultReserve;
    size_t page = static_cast<size_t>(getpagesize());
    return (size + page - 1) & ~(page - 1);
}

#endif
