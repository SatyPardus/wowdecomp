#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "Client.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/Hardware.h"
#include "WoW/Source/WowSysMessageOutput.h"

// STATIC: 0x00AB6004
static const char* s_filterNames[] = { "general", "world", "ui", "animation", "models", "objects", "sound", "movement", "network" };

// STATIC: 0x00B2F000
static CVar* s_M2ForceAdditiveParticleSortVar;

// STATIC: 0x00B2F004
static CVar* s_M2BatchParticlesVar;

// STATIC: 0x00B2F008
static CVar* s_M2BatchDoodadsVar;

// STATIC: 0x00B2F00C
static CVar* s_M2UseThreadsVar;

// STATIC: 0x00B2F010
static CVar* s_M2UseClipPlanesVar;

// STATIC: 0x00B2F014
static CVar* s_M2UseZFillVar;

// STATIC: 0x00B2F018
static CVar* s_M2FasterDebugVar;

// STATIC: 0x00B2F01C
static CVar* s_M2FasterVar;

// STATIC: 0x00B2F020
CVar* s_desktopGammaCvar;

// STATIC: 0x00B2F024
CVar* s_gammaCvar;

// STATIC: 0x00B2F028
static CVar* s_cvErrorFilter;

// STATIC: 0x00B2F02C
static CVar* s_cvErrorLevelMax;

// STATIC: 0x00B2F030
static CVar* s_cvErrorLevelMin;

// STATIC: 0x00B2F034
static CVar* s_cvShowErrors;

// STATIC: 0x00B2F038
static CVar* s_cvErrors;

// STATIC: 0x00B2F03C
static CVar* s_cvMouseSpeed;

// STATIC: 0x00B2F04C
static CVar* s_cvScreenshotQuality;

// STATIC: 0x00B2F85C
static CVar* s_cvScreenshotFormat;

// STATIC: 0x00B2F960
CVar* g_readContestVar;

// STATIC: 0x00B2F964
CVar* g_readScanningVar;

// STATIC: 0x00B2F968
CVar* g_readTerminationWithoutNoticeVar;

// STATIC: 0x00B2F96C
CVar* g_readEULAVar;

// STATIC: 0x00B2F970
CVar* g_readTOSVar;

// STATIC: 0x00B2F974
CVar* g_lastCharacterIndexVar;

// STATIC: 0x00B2F978
CVar* g_checkAddonVersionVar;

// STATIC: 0x00B2F97C
static CVar* s_cvMovieSubtitle;

// STATIC: 0x00B2F980
CVar* s_cvExpansionMovie;

// STATIC: 0x00B2F984
CVar* s_cvMovie;

// STATIC: 0x00B2F988
CVar* g_accountUsesTokenVar;

// STATIC: 0x00B2F98C
CVar* g_accountListVar;

// STATIC: 0x00B2F990
CVar* g_accountNameVar;

// STATIC: 0x00B2F9F8
CVar* s_cvarUIFaster;

// STATIC: 0x00B2F9FC
CVar* s_cvTextureCacheSize;

// STATIC: 0x00B2FA00
CVar* s_cvTextureFilteringMode;

// STATIC: 0x00B2FA04
static CVar* s_cvAsyncThreadSleep;

// STATIC: 0x00B2FA08
static CVar* s_cvAsyncHandlerTimeout;

// VERIFY: 1c2781fc auto
// OFFSET: 0x004015E0
static bool MouseSpeedCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    OsInputSetMouseSpeed(SStrToFloat(value));
    return true;
}

// VERIFY: e32e19ff auto
// OFFSET: 0x00401600
static bool ErrorDisplayCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t enable = SStrToInt(value);

    if (enable) {
        WowSysMessageOutput::Enable();
        ConsoleWrite("Error display enabled", DEFAULT_COLOR);
    } else {
        WowSysMessageOutput::Disable();
        ConsoleWrite("Error display disabled", DEFAULT_COLOR);
    }

    SysMsgEnable(enable);

    return true;
}

// VERIFY: 0b7415e2 auto
// OFFSET: 0x00401650
static bool ErrorDisplayShowCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t show = SStrToInt(value);

    if (show) {
        WowSysMessageOutput::Show();
        ConsoleWrite("Error display shown", DEFAULT_COLOR);
    } else {
        WowSysMessageOutput::Hide();
        ConsoleWrite("Error display hidden", DEFAULT_COLOR);
    }

    SysMsgEnable(show);

    return true;
}

// VERIFY: c4aceae3 auto
// OFFSET: 0x004016A0
static void DisplayErrorLevelStatus() {
    uint32_t minLevel = SysMsgGetMinDisplayLevel();
    uint32_t maxLevel = SysMsgGetMaxDisplayLevel();

    if (!minLevel && maxLevel == 3) {
        ConsoleWrite("Displaying all system messages", DEFAULT_COLOR);
        return;
    }

    char status[80];
    SStrCopy(status, "Displaying ", sizeof(status));

    if (minLevel == maxLevel) {
        SStrPack(status, "only ", STORM_MAX_STR);
    }

    switch (minLevel) {
        case 0:
            SStrPack(status, "informational messages", STORM_MAX_STR);
            break;

        case 1:
            SStrPack(status, "warnings", STORM_MAX_STR);
            break;

        case 2:
            SStrPack(status, "errors", STORM_MAX_STR);
            break;

        case 3:
            SStrPack(status, "fatal errors", STORM_MAX_STR);
            break;

        default:
            break;
    }

    if (minLevel != maxLevel) {
        switch (maxLevel) {
            case 1:
                SStrPack(status, " through warnings", STORM_MAX_STR);
                break;

            case 2:
                SStrPack(status, " through errors", STORM_MAX_STR);
                break;

            case 3:
                SStrPack(status, " through fatal errors", STORM_MAX_STR);
                break;
        }
    }

    ConsoleWrite(status, DEFAULT_COLOR);
}

// VERIFY: 1760105a manual
// OFFSET: 0x004017C0
static bool ErrorDisplayMinLevelCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t level = SStrToInt(value);

    if (level > 3) {
        ConsoleWriteA("%i is not valid, valid values are 0 - %i", DEFAULT_COLOR, level, 3);
        return false;
    }

    SysMsgSetMinDisplayLevel(level);
    DisplayErrorLevelStatus();

    return true;
}

// VERIFY: 25f454eb manual
// OFFSET: 0x00401800
static bool ErrorDisplayMaxLevelCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t level = SStrToInt(value);

    if (level > 3) {
        ConsoleWriteA("%i is not valid, valid values are 0 - %i", DEFAULT_COLOR, level, 3);
        return false;
    }

    SysMsgSetMaxDisplayLevel(level);
    DisplayErrorLevelStatus();

    return true;
}

// VERIFY: 63bbf850 manual
// OFFSET: 0x00401840
static void PrintFilterMask() {
    uint32_t filter = SysMsgGetFilter();

    if (filter == 0xFFFFFFFF) {
        ConsoleWrite("Now filtering: all messages", DEFAULT_COLOR);
        return;
    }

    char names[80] = {};

    for (uint32_t i = 0; i < sizeof(s_filterNames) / sizeof(s_filterNames[0]); i++) {
        if ((1 << i) & filter) {
            SStrPack(names, s_filterNames[i], sizeof(names));
            SStrPack(names, " ", sizeof(names));
        }
    }

    ConsolePrintf("Now filtering: %s", names);
}

// VERIFY: d8b73485 manual
// OFFSET: 0x004018D0
static int32_t ErrorDisplaySetFilter(const char* filter) {
    int32_t except = 0;
    uint32_t mask = 0;
    char separators[] = "\t\r\n\" ";
    char token[64];

    SStrTokenize(&filter, token, sizeof(token), separators, nullptr);

    while (token[0]) {
        uint32_t i;

        for (i = 0; i < sizeof(s_filterNames) / sizeof(s_filterNames[0]); i++) {
            if (!SStrCmpI(token, s_filterNames[i], STORM_MAX_STR)) {
                uint32_t bit = 1 << i;

                if (except) {
                    mask &= ~bit;
                } else {
                    mask |= bit;
                }

                break;
            }
        }

        if (i == sizeof(s_filterNames) / sizeof(s_filterNames[0])) {
            if (!SStrCmpI(token, "all", STORM_MAX_STR)) {
                mask = (except != 0) - 1;
            } else {
                if (!SStrCmpI(token, "except", STORM_MAX_STR)) {
                    ConsolePrintf("Unknown filter %s", token);
                    ConsoleWrite("Filters: general world ui animation models objects sound movement all", DEFAULT_COLOR);
                    ConsoleWrite("         use \"except\" to invert mask", DEFAULT_COLOR);
                    ConsoleWrite("         i.e.: all except objects", DEFAULT_COLOR);
                    return 0;
                }

                except = 1;
            }
        }

        SStrTokenize(&filter, token, sizeof(token), separators, nullptr);
    }

    SysMsgSetFilter(mask);

    return 1;
}

// VERIFY: 911d9ddb auto
// OFFSET: 0x00401A10
static bool ErrorDisplayFilterCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (!ErrorDisplaySetFilter(value)) {
        return false;
    }

    PrintFilterMask();

    return true;
}

// VERIFY: 66dde7ed auto
// OFFSET: 0x00401A30
static bool GammaCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    float gamma = SStrToFloat(value);

    if (gamma < 0.001f) {
        gamma = 1.0f;
    }

    if (!s_desktopGammaCvar || s_desktopGammaCvar->m_intValue) {
        return true;
    }

    g_theGxDevicePtr->DeviceSetGamma(gamma);

    return true;
}

// VERIFY: 134f8295 manual
// OFFSET: 0x00401A80
static bool DesktopGammaCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    // WoWFix335 windowed-mode-gamma-fix: desktopGamma ignored, original applies the desktop ramp when set
    SStrToInt(value);

    float gamma = s_gammaCvar ? s_gammaCvar->m_floatValue : 1.0f;

    if (gamma < 0.001f) {
        gamma = 1.0f;
    }

    g_theGxDevicePtr->DeviceSetGamma(gamma);

    return true;
}

// VERIFY: d329ddc4 auto
// OFFSET: 0x00401B20
static bool ScreenshotFormatCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    ScrnSetScreenshotFormat(value);
    return true;
}

// VERIFY: bcaf1dde auto
// OFFSET: 0x00401B40
static bool ScreenshotQualityCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    ScrnSetScreenshotQuality(SStrToInt(value));
    return true;
}

// VERIFY: 85990911 auto
// OFFSET: 0x00402100
static uint32_t GetM2FasterModifierMask(int32_t faster, int32_t debug) {
    uint32_t mask = 0;

    switch (faster) {
        case 0:
            if (!debug) {
                return 0;
            }

            switch (debug % 10) {
                case 1:
                    mask = 0x2000;
                    break;

                case 2:
                    mask = 0x6000;
                    break;

                case 3:
                    mask = 0xE000;
                    break;
            }

            if (debug / 100 % 10 && EventIsKeyDown(KEY_LCONTROL)) {
                return 0;
            }

            return mask;

        case 1:
            return 0xE000;

        case 2:
        case 3:
            return 0x2000;

        default:
            return mask;
    }
}

// VERIFY: c044ca58 auto
// OFFSET: 0x004021C0
static bool M2FasterChanged(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t faster = SStrToInt(value);

    if (s_M2FasterDebugVar) {
        M2SetGlobalOptFlags(GetM2FasterModifierMask(faster, s_M2FasterDebugVar->m_intValue));
    } else {
        M2SetGlobalOptFlags(GetM2FasterModifierMask(faster, 0));
    }

    return true;
}

// VERIFY: 7763bcf0 auto
// OFFSET: 0x00402210
static bool M2DebugFasterChanged(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t faster = s_M2FasterVar ? s_M2FasterVar->m_intValue : 0;
    M2SetGlobalOptFlags(GetM2FasterModifierMask(faster, SStrToInt(value)));
    return true;
}

// VERIFY: e0971ab1 auto
// OFFSET: 0x00402250
bool UIFasterCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t faster = SStrToInt(value);

    if (faster > 3) {
        return false;
    }

    if (faster & 1) {
        Hardware* hardware = ConsoleGetHardware();

        if (hardware) {
            if (!hardware->m_videoHw && hardware->m_videoDevice.m_vendorID == 0x8086) {
                faster &= ~1u;
                ConsoleWrite("Texture atlas disabled.", DEFAULT_COLOR);
            }

            VideoHardwareRec* video = hardware->m_videoHw;

            if (video && video->m_atlasdisable) {
                faster &= ~1u;
                ConsoleWrite("Texture atlas disabled.", DEFAULT_COLOR);
            }
        }
    }

    TextureEnableAtlas(faster & 1);
    CSimpleRender::s_mergeEnable = (faster >> 1) & 1;

    return true;
}

// VERIFY: 1cb25927 auto
// OFFSET: 0x004022E0
bool ProcessAffinityMaskCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t maxMask = 0xFFFFFFFF >> (32 - OsGetNumberOfProcessors());
    uint32_t mask = atol(value);

    if (mask > maxMask) {
        ConsolePrintf("Specified mask %08x is greater than the maximum allowable value of %08x", mask, maxMask);
        return false;
    }

    return true;
}

// VERIFY: 46b0bbdc auto
// OFFSET: 0x00402320
bool TextureCacheSizeCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t size = SStrToInt(value) << 20;
    int32_t maxSize = TextureCacheGetMaxSize();
    char message[256];

    if (size < 0 || size > maxSize) {
        SStrPrintf(message, sizeof(message), "Texture cache size (%d meg) greater than maximum allowed for your system (%d meg).", size >> 20, maxSize >> 20);
        ConsoleWrite(message, DEFAULT_COLOR);
        return false;
    }

    SStrPrintf(message, sizeof(message), "Texture cache size set to %d meg.", size >> 20);
    ConsoleWrite(message, DEFAULT_COLOR);
    TextureSetCacheSize(size);

    return true;
}

// VERIFY: d4af3a76 manual
// OFFSET: 0x00402410
static bool BatchDoodadsCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t enable = SStrToInt(value);
    uint32_t flags = M2GetCacheFlags();

    if (enable) {
        M2SetCacheFlags(flags | 0x20);
        ConsoleWrite("Doodad batching enabled.", DEFAULT_COLOR);
    } else {
        M2SetCacheFlags(flags & ~0x20);
        ConsoleWrite("Doodad batching disabled.", DEFAULT_COLOR);
    }

    return true;
}

// VERIFY: 1553be3a manual
// OFFSET: 0x00402470
static bool BatchParticlesCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t enable = SStrToInt(value);
    uint32_t flags = M2GetCacheFlags();

    if (enable) {
        M2SetCacheFlags(flags | 0x80);
        ConsoleWrite("Particle batching enabled.", DEFAULT_COLOR);
    } else {
        M2SetCacheFlags(flags & ~0x80);
        ConsoleWrite("Particle batching disabled.", DEFAULT_COLOR);
    }

    return true;
}

// VERIFY: 5cbd54c5 manual
// OFFSET: 0x004024D0
static bool ForceAdditiveParticleSortCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t enable = SStrToInt(value);
    uint32_t flags = M2GetCacheFlags();

    if (enable) {
        M2SetCacheFlags(flags | 0x100);
        ConsoleWrite("Sorting all particles as though they were additive.", DEFAULT_COLOR);
    } else {
        M2SetCacheFlags(flags & ~0x100);
        ConsoleWrite("Sorting particles normally.", DEFAULT_COLOR);
    }

    return true;
}

// VERIFY: 138a79f8 auto
// OFFSET: 0x00402670
static bool AsyncFileReadThreadSleepCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    return static_cast<uint32_t>(SStrToInt(value)) <= 100;
}

// VERIFY: 7d364ca0 auto
// OFFSET: 0x00402690
static bool AsyncFileReadHandlerTimeoutCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    return static_cast<uint32_t>(SStrToInt(value) - 20) <= 230;
}

// VERIFY: aabf0f07 manual
// OFFSET: 0x00403200
bool TimingMethodCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t method = atol(value);

    if (method > 2) {
        ConsolePrintf("'%s' is not a valid timing method. Valid methods are:", value);

        for (int32_t i = 0; i <= 2; i++) {
            ConsolePrintf("  %d - %s", i, GetTimingMethodName(static_cast<OsTimeManager::TimingMethod>(i)));
        }

        return false;
    }

    if (oldValue && method != static_cast<uint32_t>(atol(oldValue))) {
        CVar* testError = static_cast<CVar*>(arg);

        if (testError->m_intValue) {
            testError->SetReadOnly(false);
            testError->Set("0", true, false, false, true);
            testError->SetReadOnly(true);
        }
    }

    return true;
}

// VERIFY: c28b9e2a auto
// OFFSET: 0x00403320
bool LocaleCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    SStrCopy(s_localeCode, value, sizeof(s_localeCode));

    return true;
}

// VERIFY: 9951b118 manual
// OFFSET: 0x00401B60
void ClientRegisterConsoleCommands() {
    ConsoleCommandRegister("reloadUI", CCommand_ReloadUI, CATEGORY_GRAPHICS, nullptr);
    ConsoleCommandRegister("perf", CCommand_Perf, CATEGORY_DEBUG, nullptr);

    g_accountNameVar = CVar::Register("accountName", "Saved account name", 0x40, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_accountListVar = CVar::Register("accountList", "List of wow accounts for saved Blizzard account", 0, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_accountUsesTokenVar = CVar::Register("g_accountUsesToken", "Saved whether uses authenticator", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvMovie = CVar::Register("movie", "Show movie on startup", 0, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvExpansionMovie = CVar::Register("expansionMovie", "Show expansion movie on startup", 0, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvMovieSubtitle = CVar::Register("movieSubtitle", "Show movie subtitles", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_checkAddonVersionVar = CVar::Register("checkAddonVersion", "Check interface addon version number", 0, "1", nullptr, CATEGORY_GAME, false, nullptr, false);

    char mouseSpeed[32];
    SStrPrintf(mouseSpeed, sizeof(mouseSpeed), "%1.1f", OsInputGetMouseSpeed());
    s_cvMouseSpeed = CVar::Register("mouseSpeed", nullptr, 0, mouseSpeed, MouseSpeedCallback, CATEGORY_GAME, false, nullptr, false);

    s_cvErrors = CVar::Register("Errors", nullptr, 0, "0", ErrorDisplayCallback, CATEGORY_DEBUG, false, nullptr, false);
    s_cvShowErrors = CVar::Register("ShowErrors", nullptr, 0, "1", ErrorDisplayShowCallback, CATEGORY_DEBUG, false, nullptr, false);
    s_cvErrorLevelMin = CVar::Register("ErrorLevelMin", nullptr, 0, "2", ErrorDisplayMinLevelCallback, CATEGORY_DEBUG, false, nullptr, false);
    s_cvErrorLevelMax = CVar::Register("ErrorLevelMax", nullptr, 0, "3", ErrorDisplayMaxLevelCallback, CATEGORY_DEBUG, false, nullptr, false);
    s_cvErrorFilter = CVar::Register("ErrorFilter", nullptr, 0, "all", ErrorDisplayFilterCallback, CATEGORY_DEBUG, false, nullptr, false);
    s_desktopGammaCvar = CVar::Register("DesktopGamma", nullptr, 0, "0", DesktopGammaCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_gammaCvar = CVar::Register("Gamma", nullptr, 0, "1.0", GammaCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    g_lastCharacterIndexVar = CVar::Register("lastCharacterIndex", "Last character selected", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_readTOSVar = CVar::Register("readTOS", "Status of the TOS", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_readEULAVar = CVar::Register("readEULA", "Status of the EULA", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_readTerminationWithoutNoticeVar = CVar::Register("readTerminationWithoutNotice", "Status of the Termination without Notice notice", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_readScanningVar = CVar::Register("readScanning", "Status of the Scanning notice", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_readContestVar = CVar::Register("readContest", "Status of the Contest notice", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvScreenshotFormat = CVar::Register("screenshotFormat", "Set the format of screenshots", 1, "jpeg", ScreenshotFormatCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvScreenshotQuality = CVar::Register("screenshotQuality", "Set the quality of screenshots (1 - 10)", 1, "3", ScreenshotQualityCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    CVar* showToolsUI = CVar::Register("showToolsUI", "Display the launcher when starting the game", 0, "-1", nullptr, CATEGORY_GAME, false, nullptr, false);

    if (static_cast<uint32_t>(showToolsUI->m_intValue) >= 2) {
        showToolsUI->Set("1", true, false, false, true);
    }

    if (SFile::IsTrial()) {
        CVar* converted = CVar::Register("converted", "Trial to Retail", 0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
        StreamingSetEnabled(converted->m_intValue != 0);
    }

    CVar::Register("accounttype", "Account Type", 0, "", nullptr, CATEGORY_GAME, false, nullptr, false);
}

// VERIFY: dd31cf34 auto
// OFFSET: 0x004026B0
void AsyncFileReadRegisterCVars() {
    s_cvAsyncThreadSleep = CVar::Register("asyncThreadSleep", "Engine option: Async read thread sleep", 1, "0", AsyncFileReadThreadSleepCallback, CATEGORY_DEBUG, false, nullptr, false);
    s_cvAsyncHandlerTimeout = CVar::Register("asyncHandlerTimeout", "Engine option: Async read main thread timeout", 1, "100", AsyncFileReadHandlerTimeoutCallback, CATEGORY_DEBUG, false, nullptr, false);
    AsyncFileReadInitialize(s_cvAsyncThreadSleep->m_intValue, s_cvAsyncHandlerTimeout->m_intValue);
}

// VERIFY: 8ac33977 auto
// OFFSET: 0x00402760
uint32_t GetM2CvarFlags() {
    s_M2UseZFillVar = CVar::Register("M2UseZFill", "z-fill transparent objects", 0, "1", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2UseClipPlanesVar = CVar::Register("M2UseClipPlanes", "use clip planes for sorting transparent objects", 0, "1", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2UseThreadsVar = CVar::Register("M2UseThreads", "multithread model animations", 0, "1", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2BatchDoodadsVar = CVar::Register("M2BatchDoodads", "combine doodads to reduce batch count", 0, "1", BatchDoodadsCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2BatchParticlesVar = CVar::Register("M2BatchParticles", "combine particle emitters to reduce batch count", 0, "1", BatchParticlesCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2ForceAdditiveParticleSortVar = CVar::Register("M2ForceAdditiveParticleSort", "force all particles to sort as though they were additive", 0, "0", ForceAdditiveParticleSortCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2FasterVar = CVar::Register("M2Faster", "end user control of scene optimization mode - (0-3)", 0, "1", M2FasterChanged, CATEGORY_GRAPHICS, false, nullptr, false);
    s_M2FasterDebugVar = CVar::Register("M2FasterDebug", "programmer control of scene optimization mode", 0, "0", M2DebugFasterChanged, CATEGORY_GRAPHICS, false, nullptr, false);

    uint32_t flags = s_M2UseZFillVar->m_intValue != 0;

    if (s_M2UseClipPlanesVar->m_intValue) {
        flags |= 0x2;
    }

    if (s_M2UseThreadsVar->m_intValue) {
        flags |= 0x4;
    }

    if (s_M2BatchDoodadsVar->m_intValue) {
        flags |= 0x20;
    }

    if (s_M2BatchParticlesVar->m_intValue) {
        flags |= 0x80;
    }

    if (s_M2ForceAdditiveParticleSortVar->m_intValue) {
        flags |= 0x100;
    }

    return flags | 0x8;
}
