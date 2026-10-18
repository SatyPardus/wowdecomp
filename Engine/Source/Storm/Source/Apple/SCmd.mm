#if defined(__APPLE__)
#include "Engine/Source/Storm/Source/SCmd.h"
#import <Foundation/Foundation.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr uint32_t OpenFailed = 110; // Original Storm/Win32 error namespace.
constexpr size_t FileBudget = 1024 * 1024, NestedBudget = 8 * FileBudget;
struct FileIdentity { dev_t device; ino_t inode; };
thread_local std::vector<FileIdentity> activeFiles;
thread_local size_t responseBytes;
}

void GenerateError(void (STORMAPI* callback)(_CMDERROR*), uint32_t code, const char* item) {
    const char* format = nullptr;
    switch (code) {
        case STORM_ERROR_BAD_ARGUMENT: format = "Invalid argument: %s\n"; break;
        case STORM_ERROR_NOT_ENOUGH_ARGUMENTS: format = "The syntax of the command is incorrect.\n"; break;
        case OpenFailed: format = "Unable to open response file: %s\n"; break;
        default: return;
    }
    char message[256];
    std::snprintf(message, sizeof(message), format, item ? item : "");
    SErrSetLastError(code);
    _CMDERROR error{code, item, message};
    if (callback) callback(&error);
}

int32_t ProcessFile(const char* filename, _PROCESSING* processing, CMDDEF** nextarg,
    int32_t (STORMAPI* extra)(const char*), void (STORMAPI* error)(_CMDERROR*)) {
    auto fail = [&] { GenerateError(error, OpenFailed, filename); return 0; };
    if (!filename || !*filename || activeFiles.size() >= 16) return fail();
    // A response file is local, finite input. Nonblocking open also prevents a
    // FIFO from hanging startup before fstat can reject non-regular files.
    const int descriptor = open(filename, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (descriptor < 0) return fail();
    struct Close { int descriptor; ~Close() { close(descriptor); } } closeFile{descriptor};
    struct stat info{};
    if (fstat(descriptor, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 || uint64_t(info.st_size) > FileBudget) return fail();
    if (std::any_of(activeFiles.begin(), activeFiles.end(), [&](const auto& file) {
        return file.device == info.st_dev && file.inode == info.st_ino;
    })) return fail();
    if (activeFiles.empty()) responseBytes = 0;
    const size_t size = size_t(info.st_size);
    if (size > NestedBudget - responseBytes) return fail();
    responseBytes += size;
    activeFiles.push_back({info.st_dev, info.st_ino});
    struct Pop { ~Pop() { activeFiles.pop_back(); } } pop;
    std::vector<char> data(size + 1);
    for (size_t offset = 0; offset < size;) {
        const ssize_t count = read(descriptor, data.data() + offset, size - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return fail();
        offset += size_t(count);
    }
    const char* current = data.data();
    return ProcessString(&current, processing, nextarg, extra, error);
}

int32_t STORMAPI SCmdProcessCommandLine(int32_t (STORMAPI* extra)(const char*), void (STORMAPI* error)(_CMDERROR*)) {
    @autoreleasepool {
        NSArray<NSString*>* arguments = NSProcessInfo.processInfo.arguments;
        if (arguments.count > 16384) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0; }
        std::vector<const char*> argv;
        argv.reserve(arguments.count);
        for (NSString* argument in arguments) argv.push_back(argument.UTF8String);
        return SCmdProcessArguments(int(argv.size()), argv.data(), extra, error);
    }
}
#endif
