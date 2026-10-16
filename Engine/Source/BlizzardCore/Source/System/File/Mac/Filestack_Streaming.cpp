#if defined(__APPLE__)

#include <cstddef>
#include <cstdint>
#include <string>
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Manifests.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Storage.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/File/Filestack_Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"

static bool s_stackedClose = true;

Blizzard::File::StreamRecord* FileStack_Streaming::StreamingOpen(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms) {
    m_key = Blizzard::Streaming::ResolveFilePath(std::string(parms->m_path));
    m_file = Blizzard::Streaming::ManifestInfo::GetFileItem(m_key);
    m_entry = nullptr;
    if (!m_file) {
        return nullptr;
    }
    Blizzard::File::FileInfo info;
    if (System_File::Stacked::ExistsAt(fs->m_next, parms->m_path) && Blizzard::File::GetFileInfo(fs->m_next, parms->m_path, &info) && info.m_size == m_file->m_size) {
        return nullptr;
    }
    if (m_file->m_flags & 4) {
        Blizzard::File::StreamRecord* record = System_File::Stacked::AllocStreamingRecord(parms->m_path);
        record->m_fileHandle = nullptr;
        record->m_mode = parms->m_mode;
        return InitRecord(record);
    }
    m_entry = Blizzard::Storage::Acquire(fs->m_next, m_key, parms->m_path, m_file->m_size, m_file->m_fileVersion.c_str(), m_file->m_flags);
    if (!m_entry) {
        return nullptr;
    }
    return nullptr;
}

bool System_File::Stacked::stream_close(Blizzard::File::Filesystem* fs, FileParms* parms) {
    Blizzard::File::StreamRecord* file = parms->m_stream;
    if (s_stackedClose) {
        FileParms flush;
        flush.m_verb = offsetof(Blizzard::File::Filesystem, m_verbs.m_flush);
        flush.m_stream = file;
        if (s_manager) {
            s_manager->m_verbs.m_flush(s_manager, &flush);
        }
    }
    bool result = !file->m_streaming || Blizzard::Storage::Close(fs->m_next, file, file->m_streaming->m_entry);
    if (file->m_streaming) {
        file->m_streaming->~StreamingInfo();
    }
    bool passed = fs->m_next ? (*reinterpret_cast<FileFunc*>(reinterpret_cast<uintptr_t>(fs->m_next) + parms->m_verb))(fs->m_next, parms) : false;
    return result && passed;
}

#endif
