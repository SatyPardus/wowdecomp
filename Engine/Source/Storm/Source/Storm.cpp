#include <cstdint>
#include <cstdlib>
#if !defined(__APPLE__)
#include <process.h>
#include <windows.h>
#endif
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"

#if !defined(__APPLE__)
// STATIC: 0x00CAE948
static int32_t s_exitcode;

// STATIC: 0x00CAE94C
static int32_t s_exitcount;

// VERIFY: cbf144ec auto
// OFFSET: 0x007739B0
static void StormAtExit() {
    if (s_exitcount++ == 0) {
        _cexit();
        StormRtlDestroy();
        _exit(s_exitcode);
    }
}
#endif

// VERIFY: 3c11c3f7 manual
// OFFSET: 0x007739E0
void STORMAPI StormInitialize() {
    SStrInitialize();
    SErrInitialize();
#if !defined(__APPLE__)
    // The native synchronization layer does not load kernel32 exports.
    SServerInitialize();
#endif
    SFile::Initialize();
}

// VERIFY: eb281f58 auto
// OFFSET: 0x00773A00
void STORMAPI StormDestroy() {
    SRgnDestroy();
#if !defined(__APPLE__)
    // Native view/event owners release their windows before Storm teardown;
    // there is no Win32 SMsg window registry on Apple.
    SMsgDestroy();
#endif
    SEvtDestroy();
    SCmdDestroy();
    SFile::Destroy();
    SCompDestroy();
    SStrDestroy();
#if defined(__APPLE__)
    // UIKit can return to the same process and build another client lifetime.
    // Finish owned diagnostics/logs now, without a Windows CRT _cexit hook.
    SErrDestroy();
    SLogDestroy();
#else
    atexit(StormAtExit);
#endif
}

#if !defined(__APPLE__)
// VERIFY: 75275781 auto
// OFFSET: 0x00773A30
HINSTANCE STORMAPI StormGetInstance() {
    return GetModuleHandleA(nullptr);
}
#endif

// VERIFY: 8c025427 auto
// OFFSET: 0x00773A40
void StormExit(int32_t code) {
#if !defined(__APPLE__)
    s_exitcode = code;
#endif
    exit(code);
}

#if !defined(__APPLE__)
// VERIFY: a0012f97 auto
// OFFSET: 0x00773A60
void StormExitProcess(int32_t code) {
    s_exitcode = code;
    ExitProcess(code);
}
#endif
