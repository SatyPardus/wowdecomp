#if defined(__APPLE__)
#include <mutex>
#include <sys/sysctl.h>
#include "Engine/Source/BlizzardCore/Source/System/Hardware/Hardware.h"

Blizzard::Hardware::ProcessorFeatures System_Hardware::s_processorFeatures{};
uint32_t Blizzard::Hardware::GetProcessorFeatures() {
    static std::once_flag once;
    std::call_once(once, [] {
        auto& features = System_Hardware::s_processorFeatures;
        features.m_coreCount = features.m_processorCount = 1;
        size_t size = sizeof(uint32_t);
        sysctlbyname("hw.physicalcpu", &features.m_coreCount, &size, nullptr, 0);
        size = sizeof(uint32_t);
        sysctlbyname("hw.logicalcpu", &features.m_processorCount, &size, nullptr, 0);
        // These feature bits select x86-specific implementations. ARM SIMD
        // specialization must have its own capability bits and native code.
        features.m_features = features.m_vendor = 0;
    });
    return System_Hardware::s_processorFeatures.m_features;
}
#endif
