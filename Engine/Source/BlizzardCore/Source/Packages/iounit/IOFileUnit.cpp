#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnitContainer.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Storage.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"

// VERIFY: dc549ade manual
// OFFSET: 0x0042FA40
IOFileUnit* IOFileUnit::Create(IOUnitContainer* container, const char* path, int32_t flags) {
    bool ownsMemory;
    void* memory = IOUnitContainer::Alloc(container, sizeof(IOFileUnit), &ownsMemory);

    Blizzard::File::StreamRecord* stream;

    if (!Blizzard::File::Open(path, flags, stream)) {
        IOUnitContainer::Free(memory, ownsMemory);
        return nullptr;
    }

    if (!memory) {
        return nullptr;
    }

    return new (memory) IOFileUnit(stream, ownsMemory);
}

// INLINED
IOFileUnit::IOFileUnit(Blizzard::File::StreamRecord* stream, bool ownsMemory) : IOUnit(nullptr, ownsMemory) {
    this->m_stream = stream;
}

// VERIFY: 6ccd3f61 auto
// OFFSET: 0x004295E0
uint32_t IOFileUnit::GetType() {
    return 'file';
}

// VERIFY: 50aaf91c manual
// OFFSET: 0x00453860
bool IOFileUnit::Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead) {
    if (static_cast<System_SFile::RawReadInfo&>(System_SFile::s_rawReadInfo).m_retryNative) {
        const char* path = Blizzard::File::GetFileInfo(this->m_stream)->m_path;
        Blizzard::File::StreamingInfo* streaming = this->m_stream->m_streaming;

        if (!streaming || !streaming->m_entry || Blizzard::Streaming::CheckFileRange(path, offset, count, path, 5) == 1) {
#if defined(__APPLE__)
            return this->ReadNative(offset, path, buffer, count, bytesRead);
#else
            return this->ReadNative(static_cast<uint32_t>(offset), path, buffer, count, bytesRead);
#endif
        }
    }

    if (bytesRead) {
        *bytesRead = count;
        Blizzard::File::Read(this->m_stream, buffer, offset, reinterpret_cast<int32_t*>(bytesRead));
        return *bytesRead != 0;
    }

    int32_t amount = count;
    return Blizzard::File::Read(this->m_stream, buffer, offset, &amount) && amount == static_cast<int32_t>(count);
}

// VERIFY: 681090df auto
// OFFSET: 0x00430B90
bool IOFileUnit::Write(const void* buffer, int64_t offset, uint32_t count) {
    return Blizzard::File::Write(this->m_stream, buffer, offset, reinterpret_cast<int32_t*>(&count));
}

// VERIFY: f7c96dc2 manual
// OFFSET: 0x0042B600
bool IOFileUnit::Flush() {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Blizzard::File::Filesystem, m_verbs.m_flush);
    parms.m_stream = this->m_stream;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_flush(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 5d30a37b auto
// OFFSET: 0x0042B640
bool IOFileUnit::GetFileSize(int64_t* size) {
    *size = Blizzard::File::GetFileInfo(this->m_stream)->m_size;
    return true;
}

// VERIFY: 7cf5cfb1 manual
// OFFSET: 0x0042B670
bool IOFileUnit::SetFileSize(int64_t size) {
    return Blizzard::File::SetEOF(this->m_stream, size, 0);
}

void* IOFileUnit::Win32Handle() {
    return this->m_stream;
}

// VERIFY: 4a2a2a32 manual
// OFFSET: 0x0042B5C0
bool IOFileUnit::Close() {
    return Blizzard::File::Close(this->m_stream);
}

// VERIFY: 0c03f57b manual
// OFFSET: 0x00457D00
bool IOUnbufferedFileUnit::Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead) {
    if (!(reinterpret_cast<uintptr_t>(buffer) & this->m_alignMask) && !(count & this->m_alignMask)) {
        return this->IOFileUnit::Read(buffer, offset, count, bytesRead);
    }

    if (!this->m_alignedBuffer) {
        return false;
    }

    uint32_t total = 0;
    bool result = true;

    while (count) {
        uint32_t chunk = count > 0x100000 ? 0x100000 : count;
        uint32_t request = chunk;

        if (chunk & this->m_alignMask) {
            request = ~this->m_alignMask & (this->m_alignMask + chunk);
        }

        uint32_t read;
        result = this->IOFileUnit::Read(this->m_alignedBuffer, offset, request, &read);

        if (!result) {
            break;
        }

        uint32_t amount = chunk < request ? chunk : request;
        memcpy(buffer, this->m_alignedBuffer, amount);
        buffer = static_cast<uint8_t*>(buffer) + amount;
        offset += amount;
        count -= amount;
        total += amount;

        if (amount < chunk || !count) {
            break;
        }
    }

    if (!bytesRead) {
        return count == 0;
    }

    *bytesRead = total;
    return result;
}

// VERIFY: 890de1f7 manual
// OFFSET: 0x00434E00
bool IOUnbufferedFileUnit::Write(const void* buffer, int64_t offset, uint32_t count) {
    if (!(reinterpret_cast<uintptr_t>(buffer) & this->m_alignMask) && !(count & this->m_alignMask)) {
        return Blizzard::File::Write(this->m_stream, buffer, offset, reinterpret_cast<int32_t*>(&count));
    }

    if (!this->m_alignedBuffer) {
        return false;
    }

    bool result = true;

    while (count) {
        uint32_t chunk = count > 0x100000 ? 0x100000 : count;
        memcpy(this->m_alignedBuffer, buffer, chunk);
        buffer = static_cast<const uint8_t*>(buffer) + chunk;

        int32_t request = chunk;

        if (chunk & this->m_alignMask) {
            request = ~this->m_alignMask & (this->m_alignMask + chunk);
        }

        result = Blizzard::File::Write(this->m_stream, this->m_alignedBuffer, offset, &request);

        if (!result) {
            break;
        }

        offset += chunk;
        count -= chunk;
    }

    if (offset > this->m_size) {
        this->m_size = offset;
    }

    return result;
}

// VERIFY: 710d0107 auto
// OFFSET: 0x00426340
bool IOUnbufferedFileUnit::GetFileSize(int64_t* size) {
    *size = this->m_size;
    return true;
}

// VERIFY: 5d7cec3a auto
// OFFSET: 0x00430B50
bool IOUnbufferedFileUnit::SetFileSize(int64_t size) {
    this->m_size = size;
    return this->IOFileUnit::SetFileSize((size + this->m_alignMask) & ~static_cast<int64_t>(this->m_alignMask));
}

// VERIFY: a6aec7d5 manual
// OFFSET: 0x00430A50
bool IOUnbufferedFileUnit::Close() {
    Blizzard::Memory::Free(this->m_buffer);

    Blizzard::File::FileInfo* info = Blizzard::File::GetFileInfo(this->m_stream);
    int64_t fileSize = info->m_size;
    char path[1024];
    Blizzard::String::Copy(path, info->m_path, sizeof(path));

    if (!Blizzard::File::Close(this->m_stream)) {
        return false;
    }

    if (fileSize <= this->m_size) {
        return true;
    }

    if (!Blizzard::File::Open(path, 0x100E, this->m_stream)) {
        return false;
    }

    bool result = Blizzard::File::SetEOF(this->m_stream, this->m_size, 0);
    return Blizzard::File::Close(this->m_stream) && result;
}

// VERIFY: b673bf5e manual
// OFFSET: 0x00436860
IOUnbufferedFileUnit* System_Mopaq::OpenDiskFile(IOUnitContainer* container, const char* path, int32_t flags) {
    bool ownsMemory;
    void* memory = IOUnitContainer::Alloc(container, sizeof(IOUnbufferedFileUnit), &ownsMemory);

    Blizzard::File::StreamRecord* stream;

    if (!Blizzard::File::Open(path, flags, stream)) {
        IOUnitContainer::Free(memory, ownsMemory);
        return nullptr;
    }

    if (!memory) {
        return nullptr;
    }

    return new (memory) IOUnbufferedFileUnit(stream, ownsMemory);
}
