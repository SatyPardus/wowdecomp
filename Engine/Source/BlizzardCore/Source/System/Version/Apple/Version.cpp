#if defined(__APPLE__)
#include "Engine/Source/BlizzardCore/Source/System/Version/Version.h"
#include "Engine/Source/Os/OsVersionHash.h"

bool Blizzard::Version::GetFileVersion(const char* path, FileVersion& version) {
    OsFileVersionInfo info(path);
    if (!info.m_data.Count()) return false;
    version = {info.m_fileVersion[0], info.m_fileVersion[1], info.m_fileVersion[2], info.m_fileVersion[3]};
    return true;
}
#endif
