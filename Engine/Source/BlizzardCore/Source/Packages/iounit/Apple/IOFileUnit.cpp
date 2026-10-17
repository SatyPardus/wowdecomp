#if defined(__APPLE__)
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"

bool IOFileUnit::ReadNative(int64_t offset, const char* path, void* buffer, uint32_t count, uint32_t* bytesRead) {
    Blizzard::File::StreamRecord* stream = nullptr;
    if (bytesRead) { *bytesRead = 0; }
    // F_NOCACHE provides uncached IO without Windows sector alignment rules.
    if (!Blizzard::File::Open(path, 0x1045, stream)) { return false; }
    int32_t amount = count;
    const bool result = Blizzard::File::Read(stream, buffer, offset, &amount);
    Blizzard::File::Close(stream);
    if (bytesRead) { *bytesRead = uint32_t(amount); }
    return result && (bytesRead || amount == count);
}

IOUnbufferedFileUnit::IOUnbufferedFileUnit(Blizzard::File::StreamRecord* stream, bool ownsMemory)
    : IOFileUnit(stream, ownsMemory), m_buffer(nullptr), m_alignedBuffer(nullptr), m_alignMask(0),
      m_size(Blizzard::File::GetFileInfo(stream)->m_size) {}
#endif
