#if defined(__APPLE__)
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Os/OsFile.h"
#include "WoW/Source/WowServices/ArchiveLayout.h"
#include <algorithm>
#include <string>

bool CheckDataFilesExist() {
    // ClientData selects these paths from the configured resource root. The
    // native app's working directory is not the installation directory.
    std::string path = SFile::m_basePath;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += System_SFile2::s_dataPath;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += ClientArchiveLayout[17].m_name;
    std::replace(path.begin(), path.end(), '\\', '/');
    return OsFileExists(path.c_str()) != 0;
}
#endif
