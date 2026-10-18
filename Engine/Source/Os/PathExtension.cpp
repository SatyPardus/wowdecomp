#include "Engine/Source/Os/Path.h"

// VERIFY: b2787fd1 auto
// OFFSET: 0x0086BCC0
char* IOsPathFindExtension(char* path, int32_t offset) {
    char* extension = nullptr;

    while (true) {
        switch (*path) {
            case '\0':
                return extension ? extension : path;

            case '.':
                extension = path + offset;
                path++;
                break;

            case '/':
            case '\\':
                extension = nullptr;
                path++;
                break;

            default:
                path++;
                break;
        }
    }
}

// VERIFY: 0e83d15a auto
// OFFSET: 0x0086BD60
char* OsPathFindExtensionWithoutDot(char* path) {
    return IOsPathFindExtension(path, 1);
}
