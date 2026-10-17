#include "Engine/Source/Tempest/CRandom.h"

// Shared game random state; seeded by the client startup path.
// STATIC: 0x00B2FA30
CRndSeed g_rndSeed(0);
