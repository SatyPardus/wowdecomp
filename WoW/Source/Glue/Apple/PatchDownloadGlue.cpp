#if defined(__APPLE__)
#include "WoW/Source/Glue/CGlueMgr.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cerrno>
#include <cstdio>

bool CGlueMgr::PatchDownloadExecuteCommand(const char*, const char*) {
    // The downloaded patch's Windows runas/executable contract has no native
    // replacement yet. Report failure to the original patch state machine.
    SErrSetLastError(ENOTSUP);
    std::fputs("Native client patch execution is not implemented\n", stderr);
    return false;
}
#endif
