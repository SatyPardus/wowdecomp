#if defined(__APPLE__)
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <mach-o/dyld.h>
#include <string>
#include <vector>
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"

namespace Blizzard::Process {
static bool Path(char* buffer, int32_t capacity, bool directory) {
    if (!buffer || capacity <= 0) { errno = EINVAL; return false; }
    buffer[0] = 0;
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> name(size);
    if (_NSGetExecutablePath(name.data(), &size)) { return false; }
    std::error_code error;
    auto path = std::filesystem::canonical(name.data(), error);
    if (error) { errno = error.value(); return false; }
    const std::string result = directory ? path.parent_path().string() + "/" : path.string();
    if (result.size() >= size_t(capacity)) { errno = ENAMETOOLONG; return false; }
    std::memcpy(buffer, result.c_str(), result.size() + 1);
    return true;
}
bool GetProcessPath(char* buffer, int32_t capacity) { return Path(buffer, capacity, false); }
bool GetProcessDirectory(char* buffer, int32_t capacity) { return Path(buffer, capacity, true); }
}
#endif
