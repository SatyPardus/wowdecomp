#include "WoW/Source/Battlenet/Battlenet.h"
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00DCED2C
Battlenet::Allocator* Battlenet::g_allocator;

// INLINED
Battlenet::Allocator::~Allocator() {
}

// VERIFY: 90ab8d30 auto
// OFFSET: 0x009C0D10
bool Battlenet::Initialize(Allocator& allocator) {
    Battlenet::g_allocator = &allocator;
    return true;
}

// VERIFY: c34fc70e auto
// OFFSET: 0x009C0D20
void Battlenet::Teardown() {
    Battlenet::g_allocator = nullptr;
}

// VERIFY: 3576828e auto
// OFFSET: 0x009BFEA0
uint32_t Battlenet::StringCopy(void* dest, const char* source, uint32_t size) {
    if (!size) {
        return 0;
    }

    size--;
    uint32_t length = static_cast<uint32_t>(strlen(source));

    if (size > length) {
        size = length;
    }

    memcpy(dest, source, size);
    static_cast<char*>(dest)[size] = '\0';
    return size;
}

// INLINED
Battlenet::Event::Header::Header() {
    this->m_source.m_exists = false;
    this->m_source.m_adapter = 0;
    this->m_source.m_event = static_cast<uint32_t>(-1);
    this->m_target.m_type = 0;
}

// VERIFY: 4794d187 manual
// OFFSET: 0x009BFD80
Battlenet::SystemClient::~SystemClient() {
}

// VERIFY: c726b2a6 auto
// OFFSET: 0x008C50C0
void Battlenet::SystemServer::Log(const char* message) {
    this->LogLevel(message, 5);
}

// VERIFY: 6be4e15c manual
// OFFSET: 0x008C50E0
Battlenet::SystemServer::~SystemServer() {
}

// INLINED
Battlenet::EventSubscriber::EventSubscriber() {
    this->m_eventId.m_value = 0;
}

// VERIFY: 719d8eeb manual
// OFFSET: 0x008C5110
Battlenet::EventSubscriber::~EventSubscriber() {
}

// VERIFY: 991ad3f1 manual
// OFFSET: 0x009C0C40
Battlenet::EventInterface::~EventInterface() {
}

// VERIFY: 42a57769 manual
// OFFSET: 0x009C2880
Battlenet::Library::~Library() {
}

// VERIFY: f3ddd817 manual
// OFFSET: 0x009C29C0
Battlenet::Library* Battlenet::Library::LoadApplicationBundle(const char* name, Allocator& allocator) {
    uint32_t length = SStrLen(name);
#if defined(_WIN32)
    const char* extension = ".dll";
#elif defined(__APPLE__)
    const char* extension = ".dylib";
#else
    const char* extension = ".so";
#endif
    const uint32_t extensionLength = static_cast<uint32_t>(strlen(extension));
    if (length > UINT32_MAX - extensionLength - 1) {
        return nullptr;
    }

    char* path = static_cast<char*>(allocator.Alloc(length + extensionLength + 1));

    if (!path) {
        return nullptr;
    }

    memcpy(path, name, length);
    memcpy(path + length, extension, extensionLength + 1);

    Library* library = Battlenet::Library::Load(path, allocator);
    allocator.Free(path);
    return library;
}

// VERIFY: dc515e8e auto
// OFFSET: 0x009C2A70
#if defined(_WIN32)
void Battlenet::WindowsLibrary::Destroy() {
    Allocator* allocator = this->m_allocator;
    this->~WindowsLibrary();
    allocator->Free(this);
}
#endif

// VERIFY: 82c53b15 auto
// OFFSET: 0x009C2010
bool Battlenet::GetProc(Library* library, void*& proc, uint32_t ordinal) {
    proc = library->GetProc(ordinal);
    return proc != nullptr;
}
