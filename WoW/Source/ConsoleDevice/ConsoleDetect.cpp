#include <cstdint>
#include <cstdlib>
#include "ConsoleDetect.h"
#include "Engine/Source/Gx/CGxFormat.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Hardware.h"

// STATIC: 0x00ADBF28
static const char* s_hwRegNames[] = { "HWCpuIdx", "HWMemIdx", "HWVideoID", "HWSoundIdx" };

// VERIFY: b9267b57 manual
// OFFSET: 0x0076B5A0
static void ShowFatalMessageBox(int32_t stringID, const char* message) {
    Startup_StringsRec* titleRec = g_startup_StringsDB.GetRecord(1);
    const char* caption = titleRec ? titleRec->m_message : "World of Warcraft";
    Startup_StringsRec* rec = g_startup_StringsDB.GetRecord(stringID);

    OsGuiMessageBox(OsGuiGetWindow(2), 0, rec ? rec->m_message : message, caption);
    exit(0);
}

// VERIFY: ddc03ad3 auto
// OFFSET: 0x0076B620
void ConsoleDetectSaveHardware(const Hardware& hardware, bool& changed) {
    uint32_t cpuIdx;
    uint32_t memIdx;
    uint32_t videoID;
    uint32_t soundIdx;

    if (!SRegLoadValue("World of Warcraft\\Client", s_hwRegNames[0], 0, &cpuIdx)) {
        cpuIdx = hardware.m_cpuIdx;
    }

    if (!SRegLoadValue("World of Warcraft\\Client", s_hwRegNames[1], 0, &memIdx)) {
        memIdx = hardware.m_memIdx;
    }

    if (!SRegLoadValue("World of Warcraft\\Client", s_hwRegNames[2], 0, &videoID)) {
        videoID = hardware.m_videoID;
    }

    if (!SRegLoadValue("World of Warcraft\\Client", s_hwRegNames[3], 0, &soundIdx)) {
        soundIdx = hardware.m_soundIdx;
    }

    bool genericVideo = videoID == 1 || videoID == 168 || videoID == 169 || videoID == 170;

    if (hardware.m_cpuIdx == cpuIdx && hardware.m_memIdx == memIdx && (genericVideo || hardware.m_videoID == videoID) && hardware.m_soundIdx == soundIdx) {
        changed = false;
    } else {
        Startup_StringsRec* titleRec = g_startup_StringsDB.GetRecord(1);
        const char* caption = titleRec ? titleRec->m_message : "World of Warcraft";
        Startup_StringsRec* rec = g_startup_StringsDB.GetRecord(5);
        const char* message = rec ? rec->m_message : "Hardware changed.  Reload default settings?";

        changed = OsGuiMessageBox(OsGuiGetWindow(2), 2, message, caption) == 0;
    }

    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[0], 0, hardware.m_cpuIdx);
    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[1], 0, hardware.m_memIdx);
    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[2], 0, hardware.m_videoID);
    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[3], 0, hardware.m_soundIdx);
}

// VERIFY: 7f8168dc manual
// OFFSET: 0x0076BA30
void ConsoleDetectDetectHardware(Hardware& hardware, bool& changed) {
#if defined(__APPLE__)
    // Supported M1/A14 and newer devices use the original upper CPU tier.
    // mach timebase ticks measure elapsed time, not processor MHz.
    const uint32_t cpuIdx = 1;
#else
    if (OsIsRemoteSession()) {
        ShowFatalMessageBox(12, "Running World of Warcraft through a Remote Desktop connection is not supported.  Exiting program.");
    }

    const uint32_t cpuIdx = static_cast<int64_t>(OsGetProcessorTicksPerSecond()) * 0.000001f > 1500.0f;
#endif
    if (!ConsoleDetectSelectHardware(hardware, cpuIdx)) {
        ShowFatalMessageBox(11, "Failed to find a suitable display device.  Exiting program.");
    }

    ConsoleDetectSaveHardware(hardware, changed);

    char line[1024];
    GxLog("ConsoleDetectDetectHardware():");
    SStrPrintf(line, sizeof(line), "\tcpuIdx: %d", hardware.m_cpuIdx);
    GxLog(line);
    SStrPrintf(line, sizeof(line), "\tvideoID: %d", hardware.m_videoID);
    GxLog(line);
    SStrPrintf(line, sizeof(line), "\tsoundIdx: %d", hardware.m_soundIdx);
    GxLog(line);
    SStrPrintf(line, sizeof(line), "\tmemIdx: %d", hardware.m_memIdx);
    GxLog(line);
}
