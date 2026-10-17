#ifndef WOW_SOURCE_EXTENSION_EXTENSIONLOADER_H
#define WOW_SOURCE_EXTENSION_EXTENSIONLOADER_H

#include <cstdint>

// The table of compiled-in extensions. It is defined by Extensions/ExtensionsLoader.cpp, which
// Extensions/CMakeLists.txt generates from Extensions/ExtensionsLoader.cpp.in at configure time:
// one entry per Extensions/<Name>/src/ directory, in sorted directory order, naming the extension's
// entry point Add<Name>Extension(). The table ends with a { nullptr, nullptr } sentinel so it is
// never empty.
struct EXTENSIONENTRY {
    const char* m_name;
    void (*m_add)();
};

extern const EXTENSIONENTRY g_extensionEntries[];
extern const uint32_t g_extensionCount;

#endif
