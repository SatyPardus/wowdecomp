#include <cstdint>
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 191ce23a manual
// OFFSET: 0x0086BC30
uint32_t OsPathGetRootChars(const char* path) {
    uint32_t length = SStrLen(path);

    if (length < 2) {
        return 0;
    }

    if (path[1] == ':') {
        return (path[2] == '\\') + 2;
    }

    if (path[0] != '\\' || path[1] != '\\') {
        return 0;
    }

    const char* p = path + 2;

    for (int32_t i = 2; i; i--) {
        if (p) {
            p = SStrChr(p, '\\') + 1;
        }
    }

    if (p) {
        return static_cast<uint32_t>(p - path);
    }

    return length;
}

// VERIFY: 1756afaa auto
// OFFSET: 0x0086BCA0
uint32_t GetExecutablePathUTF8(char* path, uint32_t size) {
    return GetModuleFileNameUTF8(nullptr, path, size);
}

// VERIFY: 8de68232 manual
// OFFSET: 0x0086BE50
void OsPathStripFilename(char* path) {
    char* filename = path;

    for (char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') {
            filename = p + 1;
        }
    }

    if (filename != path) {
        char* root = path + OsPathGetRootChars(path);

        if (filename >= root) {
            *filename = '\0';
        } else {
            *root = '\0';
        }
    }
}

// VERIFY: 6ea4d387 auto
// OFFSET: 0x0086BEA0
void OsGetExePath(char* path, uint32_t size) {
    GetModuleFileNameUTF8(nullptr, path, size);
    OsPathStripFilename(path);
}
