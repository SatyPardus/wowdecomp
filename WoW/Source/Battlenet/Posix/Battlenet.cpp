#include "WoW/Source/Battlenet/Battlenet.h"
#include <dlfcn.h>
#include <new>

Battlenet::Library* Battlenet::Library::Load(const char* path, Allocator& allocator) {
    void* module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        return nullptr;
    }

    void* memory = allocator.Alloc(sizeof(PosixLibrary));
    if (!memory) {
        dlclose(module);
        return nullptr;
    }

    PosixLibrary* library = new (memory) PosixLibrary();
    library->m_module = module;
    library->m_allocator = &allocator;
    return library;
}

void* Battlenet::PosixLibrary::GetProc(const char* name) {
    return dlsym(this->m_module, name);
}

void* Battlenet::PosixLibrary::GetProc(uint32_t ordinal) {
    (void)ordinal;
    return nullptr;
}

Battlenet::PosixLibrary::~PosixLibrary() {
    dlclose(this->m_module);
}

void Battlenet::PosixLibrary::Destroy() {
    Allocator* allocator = this->m_allocator;
    this->~PosixLibrary();
    allocator->Free(this);
}
