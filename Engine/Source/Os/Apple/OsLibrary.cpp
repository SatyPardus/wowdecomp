#if defined(__APPLE__)
#include "Engine/Source/Os/OsLibrary.h"
#include <algorithm>
#include <cstdint>
#include <dlfcn.h>
#include <string>

namespace {
bool Module(void* module) {
    // These dyld search scopes are not owned modules returned by this API.
    return module && module != RTLD_DEFAULT && module != RTLD_NEXT && module != RTLD_SELF && module != RTLD_MAIN_ONLY;
}
}

void* OsLoadLibrary(const char* fileName) {
    if (!fileName || !*fileName) return nullptr;
    std::string path(fileName);
    std::replace(path.begin(), path.end(), '\\', '/');
    // dyld checks the image architecture/signature. This does not load PE DLLs
    // or change the platform's code-signing and executable-memory policy.
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_FIRST);
}

void OsFreeLibrary(void* module) {
    if (Module(module)) dlclose(module);
}

void* OsGetLibraryEntryPoint(void* module, const char* name) {
    // The original signature checker also requests Windows ordinal exports by
    // passing small integer pointers. Mach-O has named exports only.
    if (!Module(module) || reinterpret_cast<uintptr_t>(name) <= UINT16_MAX || !*name) return nullptr;
    return dlsym(module, name);
}
#endif
