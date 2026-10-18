#if defined(__APPLE__)
#include "WoW/Source/Battlenet/Battlenet.h"
#include "Engine/Source/Os/OsLibrary.h"
#include <cerrno>
#include <new>

namespace {
class NativeLibrary final : public Battlenet::Library {
public:
    NativeLibrary(void* module, Battlenet::Allocator& allocator) : module_(module), allocator_(allocator) {}
    ~NativeLibrary() override { OsFreeLibrary(module_); }
    void Destroy() override {
        auto& allocator = allocator_;
        this->~NativeLibrary();
        allocator.Free(this);
    }
    void* GetProc(const char* name) override { return OsGetLibraryEntryPoint(module_, name); }
    void* GetProc(uint32_t) override { errno = ENOTSUP; return nullptr; }
private:
    void* module_;
    Battlenet::Allocator& allocator_;
};
}

Battlenet::Library* Battlenet::Library::Load(const char* path, Allocator& allocator) {
    void* module = OsLoadLibrary(path);
    if (!module) return nullptr;
    void* storage = allocator.Alloc(sizeof(NativeLibrary));
    if (!storage) { OsFreeLibrary(module); return nullptr; }
    return new (storage) NativeLibrary(module, allocator);
}
#endif
