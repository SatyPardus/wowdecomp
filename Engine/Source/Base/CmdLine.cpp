#include <cstdint>
#include "Engine/Source/Base/CmdLine.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x009E9CB0
static const _ARGLIST s_argList[] = {
    { SCMD_TYPE_BOOL, CMD_D3D, "d3d", nullptr },
    { SCMD_TYPE_BOOL, CMD_D3D9EX, "d3d9ex", nullptr },
    { SCMD_TYPE_STRING, CMD_DATA_DIR, "datadir", nullptr },
    { SCMD_TYPE_BOOL, CMD_NO_LAG_FIX, "nolagfix", nullptr },
    { SCMD_TYPE_STRING, CMD_LOADFILE, "loadfile", nullptr },
    { SCMD_TYPE_STRING, CMD_GAMETYPE, "gametype", nullptr },
    { SCMD_TYPE_BOOL, CMD_OPENGL, "opengl", nullptr },
    { SCMD_TYPE_BOOL, CMD_SW_TNL, "swtnl", nullptr },
    { SCMD_TYPE_BOOL, CMD_TIMEDEMO, "timedemo", nullptr },
    { SCMD_TYPE_STRING, CMD_DEMOREZ, "rez", nullptr },
    { SCMD_TYPE_STRING, CMD_DEMODEPTH, "depth", nullptr },
    { SCMD_TYPE_STRING, CMD_DEMODETAIL, "detail", nullptr },
    { SCMD_TYPE_STRING, CMD_DEMOSOUND, "sound", nullptr },
    { SCMD_TYPE_BOOL, CMD_FULL_SCREEN, "fullscreen", nullptr },
    { SCMD_TYPE_BOOL, CMD_22050HZ, "22050", nullptr },
    { SCMD_TYPE_BOOL, CMD_NO_WARNINGS, "nowarnings", nullptr },
};

// VERIFY: 51748a1e auto
// OFFSET: 0x0047B6F0
int32_t CmdLineGetBool(CMDOPT opt) {
    return SCmdGetBool(opt);
}

// VERIFY: aa7e8705 auto
// OFFSET: 0x0047B700
const char* CmdLineGetString(CMDOPT opt) {
    // STATIC: 0x00B414E8
    static char buffer[260];

    SCmdGetString(opt, buffer, sizeof(buffer));
    return buffer;
}

// VERIFY: 1b140400 auto
// OFFSET: 0x0047B720
int32_t CmdLineProcess() {
    SCmdRegisterArgList(s_argList, sizeof(s_argList) / sizeof(s_argList[0]));
    return SCmdProcessCommandLine(nullptr, nullptr);
}

#if defined(__APPLE__)
int32_t CmdLineProcess(int argc, const char* const* argv) {
    if (!SCmdRegisterArgList(s_argList, sizeof(s_argList) / sizeof(s_argList[0]))) return 0;
    return SCmdProcessArguments(argc, argv, nullptr, nullptr);
}
#endif
