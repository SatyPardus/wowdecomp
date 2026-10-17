#if defined(__APPLE__)

#include "Engine/Source/BlizzardCore/Source/System/Version/Version.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"

bool Blizzard::Version::GetFileVersion(const char* path, FileVersion& version) {
    version.m_major = 0;
    version.m_minor = 0;
    version.m_build = 0;
    version.m_revision = 0;
    return false;
}

#endif
