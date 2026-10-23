#ifndef ENGINE_SOURCE_OS_PROCESSOR_H
#define ENGINE_SOURCE_OS_PROCESSOR_H

#include <cstdint>

// PPC retains its report-table slot; Windows detection emits 0, 1, 2 or 4.
enum : int32_t {
    OS_PROCESSOR_VENDOR_UNKNOWN = 0,
    OS_PROCESSOR_VENDOR_INTEL = 1,
    OS_PROCESSOR_VENDOR_AMD = 2,
    OS_PROCESSOR_VENDOR_PPC = 3,
    OS_PROCESSOR_VENDOR_CPUID_FAILED = 4,
};

uint32_t OsGetProcessorFeatures();
uint32_t OsGetProcessorFeaturesEx(int32_t& vendor);
uint64_t OsGetProcessorTicksPerSecond();

#endif
