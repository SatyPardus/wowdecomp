#include "ScanDLLGlue.h"
#include <new>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/OsURLDownload.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/CheckExecutableSignature.h"

// STATIC: 0x00B6B268
char CGlueMgr::m_scandllHackName[512];

// STATIC: 0x00B6B468
static GlueScanStatus s_downloadComplete;

// STATIC: 0x00B6B46C
GlueScanStatus CGlueMgr::m_scandllStatus;

#if defined(__APPLE__)
static int32_t InterlockedExchange(GlueScanStatus* value, int32_t replacement) {
    return value->exchange(replacement);
}
#endif

// STATIC: 0x00B6B470
const char* CGlueMgr::m_scandllResult;

// STATIC: 0x00B6B474
uint8_t CGlueMgr::m_scandllOkayToLogIn;

// STATIC: 0x00B6B475
uint8_t CGlueMgr::m_scandllShouldBlockPlaying;

// STATIC: 0x00B6B478
static CDataStore* s_versionData;

// STATIC: 0x00B6B47C
static char* s_dllURL;

// STATIC: 0x00B6B480
static char* s_localVersion;

// STATIC: 0x00B6B484
SThread CGlueMgr::m_scandllThread;

// STATIC: 0x009F6628
static ScanDLLThreadParams s_scanThreadParams = { ".\\Scan.dll", &CGlueMgr::m_scandllStatus, &CGlueMgr::m_scandllResult, CGlueMgr::m_scandllHackName, sizeof(CGlueMgr::m_scandllHackName), &CGlueMgr::m_scandllOkayToLogIn, &CGlueMgr::m_scandllShouldBlockPlaying };

// VERIFY: aa3b6cbd manual
// OFFSET: 0x004E5860
void CGlueMgr::ScanDLLContinueAnyway() {
    if (CGlueMgr::m_scandllShouldBlockPlaying) {
        return;
    }

    if (CGlueMgr::m_scandllStatus == 2 || CGlueMgr::m_scandllStatus == 3) {
        CGlueMgr::m_scandllOkayToLogIn = 1;
    }
}

// VERIFY: 25435ec1 manual
// OFFSET: 0x004E5880
bool CGlueMgr::IsScanDLLFinished() {
    return CGlueMgr::m_scandllOkayToLogIn;
}

// VERIFY: 69834dc2 auto
// OFFSET: 0x004E58D0
static uint32_t STORMAPI ScanThreadProc(void* param) {
    ScanDLLThreadParams* params = static_cast<ScanDLLThreadParams*>(param);

    uint8_t hackFound;
    uint8_t shouldBlockPlaying;
    const char* result;

    if (!ScanFileForHack(params->m_dllName, &hackFound, &shouldBlockPlaying, &result, params->m_hackName, params->m_hackNameSize)) {
        InterlockedExchange(params->m_status, 2);
        return 0;
    }

    *params->m_result = result;
    *params->m_okayToLogIn = hackFound == 0;
    *params->m_shouldBlockPlaying = shouldBlockPlaying;
    InterlockedExchange(params->m_status, 3);
    return 0;
}

// VERIFY: 335f1670 auto
// OFFSET: 0x004E5940
void CGlueMgr::ScanDLLFinished() {
    CGlueMgr::m_idleState = IDLE_NONE;
    CGlueMgr::m_showedDisconnect = 0;

    if (CGlueMgr::m_scandllStatus == 2) {
        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;
        InterlockedExchange(&CGlueMgr::m_scandllStatus, 2);
        FrameScript_SignalEvent(GLUE_EVENT_SCANDLL_ERROR, "%s", FrameScript_GetText("SCANDLL_MESSAGE_ERROR", -1, GENDER_NOT_APPLICABLE));
        return;
    }

    FrameScript_SignalEvent(GLUE_EVENT_SCANDLL_FINISHED, "%s%s%d", CGlueMgr::m_scandllResult, CGlueMgr::m_scandllHackName, CGlueMgr::m_scandllShouldBlockPlaying != 0);
}

// VERIFY: 64e52444 auto
// OFFSET: 0x004E59C0
static bool DownloadDllCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
    HOSFILE file = static_cast<HOSFILE>(param);

    if (data && size) {
        uint32_t written;

        if (!OsWriteFile(file, data, size, &written) || written != size) {
            OsCloseFile(file);
            OsDeleteFile(".\\Scan.dll");
            InterlockedExchange(&s_downloadComplete, 1);
            return false;
        }
    }

    if (done) {
        OsCloseFile(file);

        if (error) {
            OsDeleteFile(".\\Scan.dll");
        }

        InterlockedExchange(&s_downloadComplete, 1);
    }

    return true;
}

// VERIFY: 03f9f8cf manual
// OFFSET: 0x004E5A50
static bool VersionDownloadCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
    CDataStore* msg = static_cast<CDataStore*>(param);
    bool failed = true;

    if (data && size) {
        msg->PutArray(reinterpret_cast<const uint8_t*>(data), size);
    }

    if (!done) {
        return true;
    }

    if (!error) {
        msg->Finalize();

        uint8_t ch;
        uint32_t lineStart = 0;
        msg->Get(ch);

        if (msg->m_read <= msg->m_size) {
            bool found = false;

            while (true) {
                if (ch == '\n' || ch == '\r') {
                    uint32_t pos = msg->m_read;

                    if (pos - lineStart == 5) {
                        char tag[5];
                        msg->m_read = lineStart;
                        msg->GetArray(reinterpret_cast<uint8_t*>(tag), 4);
                        bool valid = msg->m_read <= msg->m_size;
                        msg->m_read = pos;
                        tag[4] = '\0';

                        if (valid && !SStrCmp(tag, "Scan", STORM_MAX_STR)) {
                            found = true;
                            break;
                        }
                    }

                    lineStart = pos;
                }

                msg->Get(ch);

                if (msg->m_read > msg->m_size) {
                    break;
                }
            }

            if (found) {
                uint32_t versionStart = msg->m_read;
                bool crlf = ch == '\r';
                uint32_t versionEnd = msg->m_read;
                bool first = true;
                msg->Get(ch);

                while (msg->m_read <= msg->m_size) {
                    if (first && crlf && ch == '\n') {
                        versionStart++;
                    } else if (ch == '\n' || ch == '\r') {
                        break;
                    }

                    versionEnd = msg->m_read;
                    first = false;
                    msg->Get(ch);
                }

                uint32_t length = versionEnd - versionStart;
                msg->m_read = versionEnd;

                if (length < 24) {
                    char version[24];
                    msg->m_read = versionStart;
                    msg->GetArray(reinterpret_cast<uint8_t*>(version), length);

                    if (msg->m_read <= msg->m_size) {
                        version[length] = '\0';

                        if (s_localVersion && !SStrCmp(s_localVersion, version, STORM_MAX_STR)) {
                            InterlockedExchange(&s_downloadComplete, 1);
                            failed = false;
                        } else {
                            HOSFILE file = OsCreateFile(".\\Scan.dll.new", OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

                            if (file != OS_INVALID_FILE) {
                                if (OsURLDownload(s_dllURL, DownloadDllCallback, file)) {
                                    failed = false;
                                } else {
                                    OsCloseFile(file);
                                    OsDeleteFile(".\\Scan.dll");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    delete s_versionData;
    s_versionData = nullptr;

    if (s_dllURL) {
        SMemFree(s_dllURL, __FILE__, __LINE__, 0);
    }

    s_dllURL = nullptr;
    delete[] s_localVersion;
    s_localVersion = nullptr;

    if (failed) {
        InterlockedExchange(&s_downloadComplete, 1);
    }

    return true;
}

// VERIFY: 451fb6dc manual
// OFFSET: 0x004E5CB0
void CGlueMgr::ScanDLLStart(const char* versionURL, const char* dllURL) {
    if (CGlueMgr::m_scandllStatus || s_downloadComplete) {
        return;
    }

    InterlockedExchange(&s_downloadComplete, 0);
    s_localVersion = static_cast<char*>(SMemAlloc(24, __FILE__, __LINE__, 0));

    bool updated = false;

    if (OsFileExists(".\\Scan.dll.new")) {
        if (GetDllVersionString(".\\Scan.dll.new", s_localVersion, 24)) {
            OsDeleteFile(".\\Scan.dll");
            updated = OsMoveFile(".\\Scan.dll.new", ".\\Scan.dll") != 0;
        }

        if (!updated) {
            OsDeleteFile(".\\Scan.dll.new");
        }
    }

    if (!updated && !GetDllVersionString(".\\Scan.dll", s_localVersion, 24)) {
        delete[] s_localVersion;
        s_localVersion = nullptr;
    }

    s_versionData = new (SMemAlloc(sizeof(CDataStore), __FILE__, __LINE__, 0)) CDataStore();
    s_dllURL = SStrDupA(dllURL, __FILE__, __LINE__);

    if (!OsURLDownload(versionURL, VersionDownloadCallback, s_versionData)) {
        delete s_versionData;
        s_versionData = nullptr;

        if (s_dllURL) {
            SMemFree(s_dllURL, __FILE__, __LINE__, 0);
        }

        s_dllURL = nullptr;
        delete[] s_localVersion;
        s_localVersion = nullptr;
        InterlockedExchange(&s_downloadComplete, 1);
    }

    InterlockedExchange(&CGlueMgr::m_scandllStatus, 1);

    if (!SThread::Create(ScanThreadProc, &s_scanThreadParams, CGlueMgr::m_scandllThread, const_cast<char*>("ScanThread"), 0)) {
        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;
        InterlockedExchange(&CGlueMgr::m_scandllStatus, 2);
        FrameScript_SignalEvent(GLUE_EVENT_SCANDLL_ERROR, "%s", FrameScript_GetText("SCANDLL_MESSAGE_ERROR", -1, GENDER_NOT_APPLICABLE));
        return;
    }

    FrameScript_SignalEvent(GLUE_EVENT_SCANDLL_DOWNLOADING, "%s", FrameScript_GetText("SCANDLL_MESSAGE_SCANNING", -1, GENDER_NOT_APPLICABLE));
    CGlueMgr::m_idleState = IDLE_14;
    CGlueMgr::m_showedDisconnect = 0;
}
