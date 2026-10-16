#include <cstdint>
#include <cstdlib>
#if defined(_WIN32)
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00CAE948
static int32_t s_exitcode;

// STATIC: 0x00CAE94C
static int32_t s_exitcount;

// VERIFY: cbf144ec auto
// OFFSET: 0x007739B0
static void StormAtExit() {
    if (s_exitcount++ == 0) {
#if defined(_WIN32)
        _cexit();
#endif
        StormRtlDestroy();
        _exit(s_exitcode);
    }
}

// VERIFY: 3c11c3f7 manual
// OFFSET: 0x007739E0
void STORMAPI StormInitialize() {
    SStrInitialize();
    SErrInitialize();
    SServerInitialize();
    SFile::Initialize();
}

// VERIFY: eb281f58 auto
// OFFSET: 0x00773A00
void STORMAPI StormDestroy() {
    SRgnDestroy();
    SMsgDestroy();
    SEvtDestroy();
    SCmdDestroy();
    SFile::Destroy();
    SCompDestroy();
    SStrDestroy();
    atexit(StormAtExit);
}

// VERIFY: 75275781 auto
// OFFSET: 0x00773A30
HINSTANCE__* STORMAPI StormGetInstance() {
#if defined(_WIN32)
    return GetModuleHandleA(nullptr);
#else
    return nullptr;
#endif
}

// VERIFY: 8c025427 auto
// OFFSET: 0x00773A40
void StormExit(int32_t code) {
    s_exitcode = code;
    exit(code);
}

// VERIFY: a0012f97 auto
// OFFSET: 0x00773A60
void StormExitProcess(int32_t code) {
    s_exitcode = code;
#if defined(_WIN32)
    ExitProcess(code);
#else
    _exit(code);
#endif
}
