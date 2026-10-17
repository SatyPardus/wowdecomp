#if defined(__APPLE__)

#include <cstdint>
#include <cstdlib>
#include "Engine/Source/Os/OsSecureRandom.h"

void IOsSecureRandom(uint8_t* buffer, uint32_t size) {
    if (buffer && size) {
        arc4random_buf(buffer, size);
    }
}

void OsSecureRandom(uint8_t* buffer, uint32_t size) {
    IOsSecureRandom(buffer, size);
}

#endif
