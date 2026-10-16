#if defined(__APPLE__)

#include <cstdint>
#include <climits>
#include <cstring>
#include <mach-o/dyld.h>
#include <unistd.h>
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Storm/H/storm.h"

static CommandLineUTF8 s_commandLineUTF8;

uint32_t GetModuleFileNameUTF8(void* module, char* path, uint32_t size) {
    (void)module;
    if (!path || !size) {
        return 0;
    }

    uint32_t required = size;
    if (_NSGetExecutablePath(path, &required) != 0) {
        path[0] = '\0';
        return required;
    }

    char resolved[PATH_MAX];
    if (realpath(path, resolved)) {
        SStrCopy(path, resolved, size);
    }

    return static_cast<uint32_t>(strlen(path));
}

CommandLineUTF8::CommandLineUTF8() {
    const char* empty = "";
    m_commandLine = new char[1];
    m_commandLine[0] = empty[0];
}

CommandLineUTF8::~CommandLineUTF8() {
    delete[] m_commandLine;
}

#endif
