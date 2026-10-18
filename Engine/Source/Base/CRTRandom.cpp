#include "CRTRandom.h"

#if defined(__APPLE__)
#include <cstdint>

// WoW 3.3.5a: rand 0x0088B867, srand 0x0088B85A; the CRT thread
// initializer at 0x0040DE57 sets this state to 1 (offset 0x14).
// Unsigned arithmetic preserves the original 32-bit wrap.
static thread_local uint32_t s_randomState = 1;

int ClientRand(void) {
    s_randomState = s_randomState * 214013u + 2531011u;
    return static_cast<int>((s_randomState >> 16) & CLIENT_RAND_MAX);
}

void ClientSrand(unsigned int seed) {
    s_randomState = seed;
}
#endif
