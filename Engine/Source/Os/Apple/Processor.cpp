#include "Engine/Source/Os/Processor.h"
#include <thread>
#include "Engine/Source/Os/TimeManager.h"

uint32_t OsGetProcessorFeaturesEx(int32_t& vendor) {
    // These flags describe x86 instruction sets, not generic SIMD support.
    vendor = 4;
    return 0x80000000u;
}
uint64_t OsGetProcessorTicksPerSecond() { return OsGetAsyncClocksPerSecond(); }

uint32_t OsGetNumberOfProcessors() { const auto count = std::thread::hardware_concurrency(); return count ? count : 1; }
