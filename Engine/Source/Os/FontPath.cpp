#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 57238f7e auto
// OFFSET: 0x0086BD80
void OsBuildFontFilePath(const char* fontName, char* path, uint32_t size) {
    SStrPrintf(path, size, "%s\\%s", "Fonts", fontName);
}
