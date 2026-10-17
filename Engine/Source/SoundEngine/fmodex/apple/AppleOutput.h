#pragma once
#include "fmod.hpp"
#include <cstdint>

// Called by the owning application thread before suspending/resuming its loop.
FMOD_RESULT FMODAppleSetActive(FMOD::System* system, bool active);
struct FMODAppleOutputStats {
    uint64_t frames;
    uint64_t callbacks;
    uint64_t nonzeroSamples;
    int sampleRate;
    int channels;
    bool running;
};
FMOD_RESULT FMODAppleGetOutputStats(FMOD::System* system, FMODAppleOutputStats* stats);
