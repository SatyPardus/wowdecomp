#ifndef WOW_SOURCE_WOWSERVICES_PATCHFILES_H
#define WOW_SOURCE_WOWSERVICES_PATCHFILES_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/WowServices/ArchiveLayout.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/stpl.h"



class PatchFiles {
    public:
    /* 0000 */ const char* m_directory;
    /* 0004 */ TSGrowableArray<char*> m_files;

    static int32_t EnumPatchArchives(OS_FILE_DATA& data, void* param);

    PatchFiles(const char* locale, bool flag);
    ~PatchFiles();
};

#endif
