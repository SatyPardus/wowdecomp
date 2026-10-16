#if defined(__APPLE__)

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include "Engine/Source/BlizzardCore/Source/System/Memory/TempBuffer.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"

extern char** environ;

bool Blizzard::Process::GetProcessPath(char* buffer, int32_t buffersize) {
    if (!buffer || buffersize <= 0) {
        return false;
    }
    uint32_t size = static_cast<uint32_t>(buffersize);
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        if (buffersize > 0) {
            *buffer = '\0';
        }
        return false;
    }
    char resolved[4096];
    if (realpath(buffer, resolved)) {
        Blizzard::String::Copy(buffer, resolved, buffersize);
    }
    return true;
}

bool Blizzard::Process::GetProcessDirectory(char* buffer, int32_t buffersize) {
    Blizzard::Memory::TempBuffer<char, 1024> path(buffersize);
    if (!GetProcessPath(path.m_data, buffersize)) {
        return false;
    }
    int32_t length = static_cast<int32_t>(Blizzard::String::FindFilename(path.m_data) - path.m_data);
    if (buffer && buffersize > 0) {
        Blizzard::String::Copy(buffer, path.m_data, std::min(length + 1, buffersize));
    }
    return true;
}

bool Blizzard::Process::LaunchChildProcessWithArguments(const char* path, const char* arguments, void (*callback)(void*), void* param) {
    std::string command = std::string(path ? path : "") + " " + (arguments ? arguments : "");
    pid_t pid;
    const char* argv[] = { "/bin/sh", "-c", command.c_str(), nullptr };
    if (posix_spawn(&pid, "/bin/sh", nullptr, nullptr, const_cast<char* const*>(argv), environ) != 0) {
        return false;
    }
    if (callback) {
        int status;
        waitpid(pid, &status, 0);
        callback(param);
    }
    return true;
}

void Blizzard::Process::Sleep(uint32_t milliseconds) {
    usleep(static_cast<useconds_t>(milliseconds) * 1000);
}

#endif
