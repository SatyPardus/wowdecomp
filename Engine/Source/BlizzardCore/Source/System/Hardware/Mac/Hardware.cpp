#if defined(__APPLE__)

#include <cstdint>
#include <cstring>
#include <sys/sysctl.h>
#include "Engine/Source/BlizzardCore/Source/System/Hardware/Hardware.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"

Blizzard::Hardware::ProcessorFeatures System_Hardware::s_processorFeatures;
bool System_Hardware::s_initialized;
Blizzard::Lock::Atomic System_Hardware::s_initCount;

bool System_Hardware::ReadCPUID(CPUIDInfo* info) {
    if (info) {
        memset(info, 0, sizeof(*info));
    }
    return false;
}

void System_Hardware::InitProcessorFeatures() {
    uint32_t count = 0;
    size_t size = sizeof(count);
    if (sysctlbyname("hw.logicalcpu", &count, &size, nullptr, 0) != 0 || !count) {
        count = 1;
    }
    s_processorFeatures.m_processorCount = count;
    uint32_t cores = 0;
    size = sizeof(cores);
    if (sysctlbyname("hw.physicalcpu", &cores, &size, nullptr, 0) != 0 || !cores) {
        cores = count;
    }
    s_processorFeatures.m_coreCount = cores;
#if defined(__x86_64__)
    s_processorFeatures.m_vendor = 1;
    s_processorFeatures.m_features = 0x1 | 0x2 | 0x4 | 0x10 | 0x40;
#else
    s_processorFeatures.m_vendor = 0;
    s_processorFeatures.m_features = 0;
#endif
}

uint32_t Blizzard::Hardware::GetProcessorFeatures() {
    if (!System_Hardware::s_initialized) {
        if (System_Hardware::s_initCount.Increment() == 1) {
            System_Hardware::InitProcessorFeatures();
            System_Hardware::s_initialized = true;
        } else {
            while (!System_Hardware::s_initialized) {
                Blizzard::Process::Sleep(1);
            }
        }
    }
    return System_Hardware::s_processorFeatures.m_features;
}

#endif
