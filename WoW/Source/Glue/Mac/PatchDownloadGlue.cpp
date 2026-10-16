#if defined(__APPLE__)

#include "WoW/Source/Glue/CGlueMgr.h"

bool CGlueMgr::PatchDownloadExecuteCommand(const char* path, const char* arguments) {
    static_cast<void>(path);
    static_cast<void>(arguments);

    return false;
}

#endif
