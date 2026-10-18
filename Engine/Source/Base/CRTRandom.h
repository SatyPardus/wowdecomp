#pragma once

// The game's CRT random stream is distinct from Tempest CRandom and secure
// authentication randomness. Apple libc has a different range and sequence.
#if defined(__APPLE__)
enum { CLIENT_RAND_MAX = 0x7FFF };
#ifdef __cplusplus
extern "C" {
#endif
int ClientRand(void);
void ClientSrand(unsigned int seed);
#ifdef __cplusplus
}
#endif
#else
#include <stdlib.h>
enum { CLIENT_RAND_MAX = RAND_MAX };
static inline int ClientRand(void) { return rand(); }
static inline void ClientSrand(unsigned int seed) { srand(seed); }
#endif
