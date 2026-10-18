#if defined(__APPLE__)
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include <cerrno>
#include <climits>
#include <cstring>

uint32_t GetModuleFileNameUTF8(void* module, char* path, uint32_t size) {
    if (!path || !size || size > INT_MAX) { errno = EINVAL; return 0; }
    path[0] = 0;
    // An OsLoadLibrary handle is opaque to dyld; it is not an image address.
    if (module) { errno = ENOTSUP; return 0; }
    return Blizzard::Process::GetProcessPath(path, int32_t(size)) ? uint32_t(std::strlen(path)) : 0;
}
uint32_t GetExecutablePathUTF8(char* path, uint32_t size) { return GetModuleFileNameUTF8(nullptr, path, size); }
void OsGetExePath(char* path, uint32_t size) {
    if (!path || !size || size > INT_MAX) { errno = EINVAL; return; }
    Blizzard::Process::GetProcessDirectory(path, int32_t(size));
}
uint32_t OsPathGetRootChars(const char* path) { return path && path[0] == '/' ? 1 : 0; }
void OsPathStripFilename(char* path) {
    if (!path) { errno = EINVAL; return; }
    char* filename = path;
    for (char* p = path; *p; ++p) if (*p == '/' || *p == '\\') filename = p + 1;
    if (filename != path) *filename = 0;
}
#endif
