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

// STATIC: 0x00ADBE24
static CpuHardware s_cpuHwSettings[] = {
    { 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1 },
};

// STATIC: 0x00ADBE54
static SoundHardware s_soundHwSettings = { 1, false };

// STATIC: 0x00ADBE5C
static uint32_t s_detailDoodadDensity[] = { 8, 12, 16, 24 };

// STATIC: 0x00ADBE6C
static bool s_animatingDoodads[] = { false, false, false, true };

// STATIC: 0x00ADBE70
static uint32_t s_waterLOD[] = { 0, 0, 0, 1 };

// STATIC: 0x00ADBE80
static float s_particleDensity[] = { 0.3f, 0.4f, 0.5f, 0.6f, 0.9f, 1.0f, 1.0f, 1.0f };

// STATIC: 0x00ADBEA0
static float s_unitDrawDist[] = { 35.0f, 50.0f, 50.0f, 75.0f, 75.0f, 300.0f, 300.0f, 300.0f };

// STATIC: 0x00ADBEC0
static float s_smallCull[] = { 0.08f, 0.08f, 0.07f, 0.07f, 0.07f, 0.07f, 0.04f, 0.04f };

// STATIC: 0x00ADBEE0
static float s_distCull[] = { 350.0f, 350.0f, 400.0f, 400.0f, 450.0f, 450.0f, 500.0f, 500.0f };

// STATIC: 0x00ADBF00
static float s_farClip[] = { 200.0f, 277.0f, 300.0f, 350.0f, 350.0f, 400.0f, 450.0f, 550.0f, 450.0f, 777.0f };

// STATIC: 0x00ADBF28
static const char* s_hwRegNames[] = { "HWCpuIdx", "HWMemIdx", "HWVideoID", "HWSoundIdx" };

// STATIC: 0x00CABE98
static CGxFormat s_formats[] = {
    CGxFormat(false, { 640, 480 }, CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_Ds160, 60, 1, true, false, true, true, 0),
    CGxFormat(false, { 800, 600 }, CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_Ds160, 60, 1, true, false, true, true, 0),
    CGxFormat(false, { 640, 480 }, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, 1, true, false, true, true, 0),
    CGxFormat(false, { 800, 600 }, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, 1, true, false, true, true, 0),
    CGxFormat(false, { 1024, 768 }, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, 1, true, false, true, true, 0),
    CGxFormat(false, { 1280, 1024 }, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, 1, true, false, true, true, 0),
    CGxFormat(false, { 1600, 1200 }, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, 1, true, false, true, true, 0),
};

// VERIFY: 419fe4e8 auto
// OFFSET: 0x0076B3F0
void ConsoleDetectSetDefaults(DefaultSettings& defaults, const Hardware& hardware) {
    const VideoHardwareRec* video = hardware.m_videoHw;
    const CpuHardware* cpu = hardware.m_cpuHw;

    defaults.m_farClip = s_farClip[2 * video->m_farclipIdx + cpu->m_farclipIdx];
    defaults.m_terrainShadowLOD = video->m_terrainShadowLod;
    defaults.m_detailDoodadDensity = s_detailDoodadDensity[video->m_detailDoodadDensityIdx];
    defaults.m_detailDoodadAlpha = video->m_detailDoodadAlpha;
    defaults.m_animatingDoodads = s_animatingDoodads[2 * video->m_animatingDoodadIdx + cpu->m_animatingDoodadIdx];
    defaults.m_trilinear = video->m_trilinear != 0;
    defaults.m_numLights = video->m_numLights;
    defaults.m_specular = video->m_specularity != 0;
    defaults.m_specularity = video->m_specularity != 0;
    defaults.m_waterLOD = s_waterLOD[2 * video->m_waterLodidx + cpu->m_waterLODIdx];
    defaults.m_particleDensity = s_particleDensity[2 * video->m_particleDensityIdx + cpu->m_particleDensityIdx];
    defaults.m_unitDrawDist = s_unitDrawDist[2 * video->m_unitDrawDistIdx + cpu->m_unitDrawDistIdx];
    defaults.m_smallCull = s_smallCull[2 * video->m_smallCullDistIdx + cpu->m_smallCullDistIdx];
    defaults.m_distCull = s_distCull[2 * video->m_smallCullDistIdx + cpu->m_smallCullDistIdx];
    defaults.m_format = &s_formats[video->m_resolutionIdx];
    defaults.m_baseMipLevel = video->m_baseMipLevel;
    defaults.unk_0019 = true;
    defaults.m_numChannels = 0;
    defaults.m_fivePointOne = false;
}

// VERIFY: 2f86cffe auto
// OFFSET: 0x0076B520
void ConsoleDetectSetDefaultsFormat(DefaultSettings& defaults, const Hardware& hardware) {
    defaults.m_format = &s_formats[hardware.m_videoHw->m_resolutionIdx];
}

// VERIFY: d4b42eb1 auto
// OFFSET: 0x0076B540
static void SetVideoIdx(Hardware& hardware) {
    for (int32_t i = 1; i < g_videoHardwareDB.m_numRecords; i++) {
        const VideoHardwareRec* rec = i < 0 || i >= g_videoHardwareDB.m_numRecords ? nullptr : &g_videoHardwareDB.m_records[i];

        if (hardware.m_videoDevice.m_vendorID == rec->m_vendorID && hardware.m_videoDevice.m_deviceID == rec->m_deviceID) {
            hardware.m_videoID = rec->m_id;
            return;
        }
    }
}

// VERIFY: b9267b57 manual
// OFFSET: 0x0076B5A0
static void ShowFatalMessageBox(int32_t stringID, const char* message) {
    Startup_StringsRec* titleRec = g_startup_StringsDB.GetRecord(1);
    const char* caption = titleRec ? titleRec->m_message : "World of Warcraft";
    Startup_StringsRec* rec = g_startup_StringsDB.GetRecord(stringID);

    OsGuiMessageBox(OsGuiGetWindow(OSGUI_WINDOW_TYPE_FRONT), OSGUI_MESSAGE_STYLE_OK, rec ? rec->m_message : message, caption);
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

        changed = OsGuiMessageBox(OsGuiGetWindow(OSGUI_WINDOW_TYPE_FRONT), OSGUI_MESSAGE_STYLE_YESNO, message, caption) == OSGUI_MESSAGE_RESULT_YESOK;
    }

    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[0], 0, hardware.m_cpuIdx);
    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[1], 0, hardware.m_memIdx);
    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[2], 0, hardware.m_videoID);
    SRegSaveValue("World of Warcraft\\Client", s_hwRegNames[3], 0, hardware.m_soundIdx);
}

// VERIFY: 7f8168dc manual
// OFFSET: 0x0076BA30
void ConsoleDetectDetectHardware(Hardware& hardware, bool& changed) {
    if (OsIsRemoteSession()) {
        ShowFatalMessageBox(12, "Running World of Warcraft through a Remote Desktop connection is not supported.  Exiting program.");
    }

    g_videoHardwareDB.Load(__FILE__, __LINE__);

    hardware.m_cpuIdx = static_cast<int64_t>(OsGetProcessorTicksPerSecond()) * 0.000001f > 1500.0f;
    hardware.m_memIdx = 0;
    hardware.m_videoID = 0;

    if (GxAdapterID(hardware.m_videoDevice.m_vendorID, hardware.m_videoDevice.m_deviceID, hardware.m_videoDevice.m_driverVersionHi, hardware.m_videoDevice.m_driverVersionLo)) {
        SetVideoIdx(hardware);
    }

    if (!hardware.m_videoID) {
        hardware.m_videoDevice.m_vendorID = 0xFFFF;

        if (GxAdapterInfer(hardware.m_videoDevice.m_deviceID)) {
            SetVideoIdx(hardware);
        }

        if (!hardware.m_videoID) {
            ShowFatalMessageBox(11, "Failed to find a suitable display device.  Exiting program.");
        }
    }

    hardware.m_soundIdx = 0;
    hardware.m_cpuHw = &s_cpuHwSettings[hardware.m_cpuIdx];
    hardware.m_videoHw = g_videoHardwareDB.GetRecord(hardware.m_videoID);
    hardware.m_soundHw = &s_soundHwSettings;

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
