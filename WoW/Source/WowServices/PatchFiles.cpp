#include <cstdint>
#include "WoW/Source/WowServices/PatchFiles.h"
#include "Engine/Source/Base/Sort.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"

// VERIFY: 204ba559 auto
// OFFSET: 0x00401200
static int32_t CompareStrings(const void* a, const void* b, void* context) {
    return -SStrCmpI(*static_cast<char* const*>(a), *static_cast<char* const*>(b), STORM_MAX_STR);
}

// VERIFY: b136b0d2 manual
// OFFSET: 0x00405A10
int32_t PatchFiles::EnumPatchArchives(OS_FILE_DATA& data, void* param) {
    PatchFiles* files = static_cast<PatchFiles*>(param);

    char path[260];
    SStrPrintf(path, sizeof(path), "%s%s", files->m_directory, data.fileName);

    char** slot = files->m_files.New();
    *slot = SStrDupA(path, __FILE__, __LINE__);

    return 0;
}

// VERIFY: 4fd60074 manual
// OFFSET: 0x00405AB0
PatchFiles::PatchFiles(const char* locale, bool flag) {
    const auto& specs = ClientPatchArchiveSpecs;

    char directory[256];
    char pattern[256];
    char installed[256];

    this->m_directory = directory;
    const char* installPath = GetInstallLocationString();
    int32_t wanted = flag;

    for (int32_t pass = 0; pass <= 1; pass++) {
        for (uint32_t i = 0; i < specs.size(); i++) {
            const PATCHARCHIVESPEC& spec = specs[i];

            if (spec.m_pass != pass || spec.m_flag != wanted) {
                continue;
            }

            SStrPrintf(directory, sizeof(directory), spec.m_directory, locale);
            SStrPrintf(pattern, sizeof(pattern), spec.m_pattern, locale);

            if (spec.m_useInstallPath && *installPath) {
                SStrPrintf(installed, sizeof(installed), "%s%s", installPath, directory);
                SStrCopy(directory, installed, sizeof(directory));
            }

            OsFileList(this->m_directory, pattern, PatchFiles::EnumPatchArchives, this, 0);
        }

        if (pass == 0) {
            quicksort(this->m_files.Ptr(), this->m_files.Count(), sizeof(char*), CompareStrings, nullptr);
        }
    }
}

// VERIFY: ff9d1c1e manual
// OFFSET: 0x00405D70
PatchFiles::~PatchFiles() {
    for (uint32_t i = 0; i < this->m_files.Count(); i++) {
        if (this->m_files[i]) {
            SMemFree(this->m_files[i], __FILE__, __LINE__, 0);
        }
    }
}
