#if defined(WOWDECOMP_ARCH_64)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <intrin.h>
#include <windows.h>
#include <shellapi.h>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B1D2AC
static int32_t s_sleepInBackground = 1;

// STATIC: 0x00B1D2B0
static const char* s_osTypeNames[] = { "Unknown", "Win95", "Win95OSR2", "Win98", "Win98SE", "WinME", "WinNT4", "Win2000", "WinXP", "Win2003", "Win9X_Other", "WinNT_Other", "MacOS9", "MacOSX", "Linux", "WinVista" };

// STATIC: 0x00A4C220
static const uint8_t s_hexDigitValues[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// STATIC: 0x00D415A0
static int32_t s_haveProcessorFeatures;

// STATIC: 0x00D415A4
static int32_t s_processorVendor;

// STATIC: 0x00D415A8
static uint32_t s_processorFeatures;

// STATIC: 0x00D415AC
static int32_t s_processorSockets;

// STATIC: 0x00D415B0
static int32_t s_processorCores;

// STATIC: 0x00D415B4
static int32_t s_processorCount;

// STATIC: 0x00D415B8
static uintptr_t s_textStart;

// STATIC: 0x00D415BC
static uintptr_t s_textEnd;

// STATIC: 0x00D415C0
static int32_t s_cpuLogEnabled;

// STATIC: 0x00D415C4
static uint32_t s_backgroundSleepMs;

// STATIC: 0x00D415C8
static uint64_t s_cpuTicksPerSecond;

// STATIC: 0x00D415D0
static int32_t s_gotPowerProfFrequency;

// STATIC: 0x00D415D4
static int32_t s_checkedPowerProfInfo;

static const char s_vendorIds[] = "GenuineIntel" "AuthenticAMD" "CyrixInstead" "CentaurHauls";

// OFFSET: 0x0086AF90
int32_t IOsGetProcessorFeatures(CpuIdInfo* info) {
    memset(info, 0, sizeof(CpuIdInfo));

    int32_t result = 0;
    uint32_t maxStandard = 0;
    uint32_t maxExtended = 0;

    uintptr_t flags = __readeflags();
    __writeeflags(flags ^ 0x200000);

    if (__readeflags() != flags) {
        int32_t regs[4];
        __cpuid(regs, 0);

        if (regs[0]) {
            maxStandard = regs[0];
            info->m_maxStandard = regs[0];
            memcpy(&info->m_vendor[0], &regs[1], 4);
            memcpy(&info->m_vendor[4], &regs[3], 4);
            memcpy(&info->m_vendor[8], &regs[2], 4);
            result = 1;

            if (maxStandard >= 4) {
                __cpuidex(regs, 4, 0);
                info->m_standard4a = regs[0];
            }

            __cpuid(regs, 1);
            info->m_standard1d = regs[3];
            info->m_standard1b = regs[1];

            __cpuid(regs, 0x80000000);

            if (static_cast<uint32_t>(regs[0]) > 0x80000000) {
                result = 2;
                maxExtended = regs[0];
                info->m_maxExtended = regs[0];

                if (maxExtended >= 0x80000008) {
                    __cpuid(regs, 0x80000008);
                    info->m_extended8c = regs[2];
                }

                if (maxExtended >= 0x80000002) {
                    __cpuid(regs, 0x80000002);
                    memcpy(&info->m_brand[0], regs, 16);
                }

                if (maxExtended >= 0x80000003) {
                    __cpuid(regs, 0x80000003);
                    memcpy(&info->m_brand[16], regs, 16);
                }

                if (maxExtended >= 0x80000004) {
                    __cpuid(regs, 0x80000004);
                    memcpy(&info->m_brand[32], regs, 16);
                }

                __cpuid(regs, 0x80000001);
                info->m_extended1d = regs[3];
                info->m_extended1c = regs[2];
            }
        }
    }

    return result;
}

// OFFSET: 0x0086B0C0
void OsSystemEnableCpuLog() {
    s_cpuLogEnabled = 1;
}

// OFFSET: 0x0086B0D0
void IOsSystemCpuLog(const CpuIdInfo* info) {
    if (!s_cpuLogEnabled) {
        return;
    }

    OsCreateDirectory("Logs", 0);

    HSLOG log;
    SLogCreate("Logs\\cpu.log", 0, &log);

    if (s_processorVendor == OS_PROCESSOR_VENDOR_CPUID_FAILED) {
        SLogWrite(log, "UNABLE TO IDENTIFY CPU");
        SLogClose(log);
        return;
    }

    SLogWrite(log, "vendor: %d", s_processorVendor);
    SLogWrite(log, "features: %08X", s_processorFeatures & 0x7FFFFFFF);
    SLogWrite(log, "sockets: %d", s_processorSockets);
    SLogWrite(log, "cores: %d", s_processorCores);
    SLogWrite(log, "processors: %d", s_processorCount);

    char vendor[16];
    strncpy(vendor, info->m_vendor, 12);
    vendor[12] = '\0';
    SLogWrite(log, "vendor id string= %s", vendor);

    SLogWrite(log, "standard (%d): 1b=%08X 1d=%08x 4a=%08X", info->m_maxStandard, info->m_standard1b, info->m_standard1d, info->m_standard4a);
    SLogWrite(log, "extended (%d): 1c=%08X 1d=%08x 8c=%08X", info->m_maxExtended & 0x7FFFFFFF, info->m_extended1c, info->m_extended1d, info->m_extended8c);

    const char* brand = info->m_brand;

    while (*brand == ' ') {
        brand++;
    }

    SLogWrite(log, "processor brand string= %s", brand);
    SLogClose(log);
}

// OFFSET: 0x0086B240
uint32_t OsGetNumberOfProcessors() {
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);

    if (!info.dwNumberOfProcessors) {
        return 1;
    }

    return info.dwNumberOfProcessors;
}

// OFFSET: 0x0086B290
int32_t OsSleepInBackground() {
    return s_sleepInBackground;
}

// OFFSET: 0x0086B2A0
void OsSetSleepInBackground(int32_t sleep) {
    s_sleepInBackground = sleep;
}

// OFFSET: 0x0086B2B0
uint32_t OsGetBackgroundSleepMs() {
    return s_backgroundSleepMs;
}

// OFFSET: 0x0086B2C0
void OsSetBackgroundSleepMs(uint32_t milliseconds) {
    s_backgroundSleepMs = milliseconds;
}

// OFFSET: 0x0086B2D0
OsType OsGetVersion() {
    OsType type = OsType_Unknown;

    OSVERSIONINFOEXA info;
    memset(&info, 0, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

    if (!GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&info))) {
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

        if (!GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&info))) {
            return type;
        }
    }

    switch (info.dwPlatformId) {
        case VER_PLATFORM_WIN32_WINDOWS:
            if (info.dwMajorVersion != 4) {
                type = OsType_Win9X_Other;
            } else if (info.dwMinorVersion == 0) {
                type = OsType_Win95;

                if (info.szCSDVersion[1] == 'C' || info.szCSDVersion[1] == 'B') {
                    type = OsType_Win95OSR2;
                }
            } else if (info.dwMinorVersion == 10) {
                type = OsType_Win98;

                if (info.szCSDVersion[1] == 'A') {
                    type = OsType_Win98SE;
                }
            } else if (info.dwMinorVersion == 90) {
                type = OsType_WinME;
            } else {
                type = OsType_Win9X_Other;
            }

            break;

        case VER_PLATFORM_WIN32_NT:
            if (info.dwMajorVersion == 4) {
                type = OsType_WinNT4;
            } else if (info.dwMajorVersion == 5) {
                if (info.dwMinorVersion == 0) {
                    type = OsType_Win2000;
                } else if (info.dwMinorVersion == 1) {
                    type = OsType_WinXP;
                } else if (info.dwMinorVersion == 2) {
                    type = OsType_Win2003;
                } else {
                    type = OsType_WinNT_Other;
                }
            } else if (info.dwMajorVersion == 6 && info.dwMinorVersion == 0) {
                type = OsType_WinVista;
            } else {
                type = OsType_WinNT_Other;
            }

            break;
    }

    return type;
}

// OFFSET: 0x0086B430
void OsGetVersionString(char* buffer, int32_t size) {
    const char* name = s_osTypeNames[OsGetVersion()];

    if (!name) {
        name = s_osTypeNames[0];
    }

    SStrCopy(buffer, name, size);
}

// OFFSET: 0x0086B460
uint32_t OsGetProcessorId() {
    return 0x783836;
}

// OFFSET: 0x0086B470
uint32_t OsGetOsId() {
    return 0x57696E;
}

// OFFSET: 0x0086B480
int32_t OsGetComputerName(char* buffer, uint32_t* size) {
    return GetComputerNameA(buffer, reinterpret_cast<DWORD*>(size));
}

// OFFSET: 0x0086B4A0
int32_t OsGetUserName(char* buffer, uint32_t* size) {
    return GetUserNameA(buffer, reinterpret_cast<DWORD*>(size));
}

// OFFSET: 0x0086B4C0
uint64_t OsGetPhysicalMemory() {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);

    return status.ullTotalPhys;
}

// OFFSET: 0x0086B4F0
void* CreateNamedEventA(const char* name, int32_t manualReset) {
    return CreateEventA(nullptr, manualReset, FALSE, name);
}

// OFFSET: 0x0086B510
void OsFindTextSectionBounds() {
    uint8_t* image = reinterpret_cast<uint8_t*>(StormGetInstance());
    IMAGE_NT_HEADERS* header = reinterpret_cast<IMAGE_NT_HEADERS*>(image + reinterpret_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew);
    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(header);
    uint16_t i;

    for (i = 0; i < header->FileHeader.NumberOfSections; i++) {
        if (!strncmp(reinterpret_cast<const char*>(sections[i].Name), ".text", 8)) {
            break;
        }
    }

    if (i >= header->FileHeader.NumberOfSections) {
        SErrDisplayAppFatal("Unable to find .text section");
    }

    s_textStart = header->OptionalHeader.ImageBase + sections[i].VirtualAddress;
    s_textEnd = header->OptionalHeader.ImageBase + sections[i].VirtualAddress + sections[i].Misc.VirtualSize;
}

// OFFSET: 0x0086B5A0
void OsValidateFunctionPointer(const void* ptr) {
    if (!s_textStart || !s_textEnd) {
        OsFindTextSectionBounds();
    }

    if (reinterpret_cast<uintptr_t>(ptr) < s_textStart || reinterpret_cast<uintptr_t>(ptr) >= s_textEnd) {
        char buffer[64];
        SStrPrintf(buffer, sizeof(buffer), "Invalid function pointer: %p", ptr);
        SErrDisplayAppFatal(buffer);
    }
}

// OFFSET: 0x0086B600
int32_t IOsParseProcessorFrequency(const char* brand) {
    if (!brand) {
        return 0;
    }

    const char* p = brand + strlen(brand) - 1;

    if (p <= brand || !*p || *p != 'z') {
        return 0;
    }

    p--;

    if (p <= brand || !*p || *p != 'H') {
        return 0;
    }

    p--;

    if (p <= brand) {
        return 0;
    }

    double scale;

    if (*p == 'G') {
        scale = 1000000000.0;
    } else if (*p == 'M') {
        scale = 1000000.0;
    } else if (*p == 'T') {
        scale = 1000000000000.0;
    } else {
        return 0;
    }

    char number[48];
    memset(number, ' ', sizeof(number));
    number[47] = '\0';

    p--;
    char* dst = &number[46];

    while (p > brand && *p && ((*p >= '0' && *p <= '9') || *p == '.')) {
        *dst = *p;
        p--;
        dst--;
    }

    double value = atof(number);

    if (value == 0.0) {
        return 0;
    }

    s_cpuTicksPerSecond = static_cast<int64_t>(value * scale);

    return 1;
}

// OFFSET: 0x0086B710
int32_t IOsGetPowerProfFrequency() {
    typedef LONG (WINAPI* CallNtPowerInformationFunc)(int32_t level, void* inputBuffer, ULONG inputBufferLength, void* outputBuffer, ULONG outputBufferLength);

    int32_t result = 0;
    HMODULE library = LoadLibraryA("powrprof.dll");

    if (library) {
        CallNtPowerInformationFunc callNtPowerInformation = reinterpret_cast<CallNtPowerInformationFunc>(GetProcAddress(library, "CallNtPowerInformation"));
        uint32_t information[24];

        if (callNtPowerInformation && !callNtPowerInformation(11, nullptr, 0, information, sizeof(information))) {
            s_cpuTicksPerSecond = static_cast<uint64_t>(information[1]) * 1000000;
            result = 1;
        }

        FreeLibrary(library);
    }

    return result;
}

// OFFSET: 0x0086B780
int32_t OsIsRemoteSession() {
    return GetSystemMetrics(SM_REMOTESESSION);
}

// OFFSET: 0x0086B790
int32_t OsLaunchURL(const char* url) {
    if (!url || !*url) {
        return 0;
    }

    HWND window = GetActiveWindow();

    if (!window) {
        return 0;
    }

    if (SStrLen(url) >= 1024) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    char file[1024];
    char* dst = file;
    const char* src = url + 1;

    for (char c = *url; c; c = *src++, dst++) {
        if (c == '%') {
            char high = *src++;

            if (!high) {
                break;
            }

            char low = *src++;

            if (!low) {
                break;
            }

            *dst = (s_hexDigitValues[high] << 4) | s_hexDigitValues[low];
        } else {
            *dst = c;
        }
    }

    *dst = '\0';

    HINSTANCE result = ShellExecuteA(window, "open", file, nullptr, nullptr, SW_SHOWNORMAL);

    if (reinterpret_cast<uintptr_t>(result) > 32) {
        return 1;
    }

    if (memcmp(url, "file://", 7)) {
        char tempPath[MAX_PATH];

        if (GetTempPathA(MAX_PATH, tempPath)) {
            char tempFile[MAX_PATH];
            SStrPrintf(tempFile, MAX_PATH, "%sTempHTMLFile%x.html", tempPath, OsGetAsyncTimeMs());

            FILE* stream = fopen(tempFile, "wb");

            if (stream) {
                fclose(stream);

                char executable[MAX_PATH];
                result = FindExecutableA(tempFile, nullptr, executable);
                DeleteFileA(tempFile);

                if (reinterpret_cast<uintptr_t>(result) > 32) {
                    result = ShellExecuteA(window, "open", executable, file, nullptr, SW_SHOWNORMAL);
                }
            }
        }
    }

    return reinterpret_cast<uintptr_t>(result) > 32;
}

// OFFSET: 0x0086B9A0
uint32_t OsGetProcessorFeaturesEx(int32_t& vendor) {
    if (s_haveProcessorFeatures) {
        vendor = s_processorVendor;
        return s_processorFeatures;
    }

    s_haveProcessorFeatures = 1;

    uint32_t features = 0;

    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);

    s_processorCount = systemInfo.dwNumberOfProcessors;

    if (!systemInfo.dwNumberOfProcessors) {
        s_processorCount = 1;
    }

    s_processorCores = 1;
    s_processorSockets = 1;

    CpuIdInfo info;

    if (IOsGetProcessorFeatures(&info)) {
        if (info.m_standard1d & 0x10) {
            features = 1;
        }

        if (info.m_standard1d & 0x800000) {
            features |= 2;
        }

        if (info.m_standard1d & 0x2000000) {
            features |= 4;
        }

        if (info.m_extended1d & 0x80000000) {
            features |= 8;
        }

        if (info.m_standard1d & 0x4000000) {
            features |= 0x10;
        }

        if (!strncmp(info.m_vendor, s_vendorIds, 12)) {
            s_processorVendor = OS_PROCESSOR_VENDOR_INTEL;

            if (info.m_maxStandard >= 4 && (info.m_standard4a >> 26)) {
                features |= 0x40;
                s_processorCores = static_cast<uint8_t>(info.m_standard4a >> 26) + 1;
            }
        } else if (!strncmp(info.m_vendor, s_vendorIds + 12, 12)) {
            s_processorVendor = OS_PROCESSOR_VENDOR_AMD;

            uint8_t cores = static_cast<uint8_t>(info.m_extended8c);

            if (cores) {
                features |= 0x40;
                s_processorCores = cores + 1;
            }
        } else if (strncmp(info.m_vendor, s_vendorIds + 24, 12)) {
            strncmp(info.m_vendor, s_vendorIds + 36, 12);
        }
    } else {
        s_processorVendor = OS_PROCESSOR_VENDOR_CPUID_FAILED;
    }

    s_processorFeatures = features | 0x80000000;

    IOsSystemCpuLog(&info);

    if (!IOsParseProcessorFrequency(info.m_brand)) {
        if (!s_checkedPowerProfInfo) {
            s_gotPowerProfFrequency = IOsGetPowerProfFrequency();
            s_checkedPowerProfInfo = 1;
        }

        if (!s_gotPowerProfFrequency) {
            s_cpuTicksPerSecond = OsGetAsyncClocksPerSecond();
        }
    }

    vendor = s_processorVendor;
    return s_processorFeatures;
}

// OFFSET: 0x0086BBA0
uint64_t OsGetProcessorTicksPerSecond() {
    if (!s_cpuTicksPerSecond) {
        int32_t vendor;
        OsGetProcessorFeaturesEx(vendor);
    }

    return s_cpuTicksPerSecond;
}

#endif
