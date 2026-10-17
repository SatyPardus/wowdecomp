#if defined(__APPLE__)

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Manifests.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Storage.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/File/Filestack_Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"
#include "Engine/Source/BlizzardCore/Source/System/URL/URL.h"

namespace {
std::string CanonicalStreamPath(const std::string& path) {
    std::string out = path;
    std::replace(out.begin(), out.end(), '\\', '/');
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool DownloadToString(const char* url, std::string& data, uint32_t timeout, Blizzard::URL::DownloadCallback::Status* status);
}

Blizzard::Streaming::DownloadProgress Blizzard::Streaming::s_progress;
std::map<std::string, Blizzard::Streaming::StorageEntry>* Blizzard::Storage::s_storage;
Blizzard::File::StreamRecord* Blizzard::Storage::s_storageFile;
bool Blizzard::Storage::s_storageReady;
bool Blizzard::Storage::s_storageDirty;
volatile bool Blizzard::Storage::s_storageStop;
volatile bool Blizzard::Storage::s_storageStopped = true;
bool Blizzard::Storage::s_usePartFiles;
int32_t Blizzard::Storage::s_blockSize = 0x100000;
std::string Blizzard::Storage::s_storagePath;
std::deque<Blizzard::Streaming::WriteRequest> Blizzard::Storage::s_writeQueue;
Blizzard::Lock::Mutex Blizzard::Storage::s_storageLock;
void* Blizzard::Storage::s_storageEvent;

void Blizzard::Streaming::DownloadProgress::Sample(uint32_t time) {
    if (++this->m_index >= 51) this->m_index = 0;
    this->m_samples[this->m_index].m_bytes = this->m_streamed;
    this->m_samples[this->m_index].m_time = time;
}

int32_t Blizzard::Streaming::DownloadProgress::GetRate() {
    uint32_t newest = this->m_index;
    uint32_t oldest = (newest + 1) % 51;
    if (!this->m_samples[oldest].m_time) oldest = 0;
    uint32_t dt = this->m_samples[newest].m_time - this->m_samples[oldest].m_time;
    if (!dt) return 0;
    return static_cast<int32_t>((this->m_samples[newest].m_bytes - this->m_samples[oldest].m_bytes) * 1000 / dt);
}

void Blizzard::Streaming::DownloadProgress::FireCallback() {
    if (this->m_enabled && this->m_callback) {
        this->m_callback(static_cast<uint32_t>(this->m_total), static_cast<uint32_t>(this->m_downloaded), static_cast<uint32_t>(this->m_streamed), static_cast<uint32_t>(this->GetRate()), 0, 0, 0);
    }
}

void Blizzard::Streaming::AddDownloaded(int32_t size) { s_statBytesDownloaded += size; s_progress.m_downloaded += size; }
void Blizzard::Streaming::AddStreamed(int32_t size) { s_statBytesRead += size; s_progress.m_streamed += size; }
uint32_t Blizzard::Streaming::DownloadThread(void*) { return 0; }
int32_t Blizzard::Streaming::GetIdleThread() { return -1; }
int32_t Blizzard::Streaming::GetNextCommand(FILE_THREAD_CMD**, int32_t) { return 0; }
bool Blizzard::Streaming::PopCommand(FILE_THREAD_CMD*, int32_t) { return false; }
bool Blizzard::Streaming::SelectQueue(FILE_THREAD_CMD*, int32_t, int32_t*) { return false; }
void Blizzard::Streaming::WaitForLoads() {}
bool Blizzard::Streaming::QueueLoad(Blizzard::File::StreamRecord*, int32_t, LoadRange*, Completion*) { return false; }
bool Blizzard::Streaming::RequestRanges(Blizzard::File::StreamRecord*, LoadRange*, int32_t) { return false; }
bool Blizzard::Streaming::LoadFile(Blizzard::File::Filesystem* fs, Blizzard::File::StreamRecord* file, LoadRange* range) {
    int32_t amount = range ? range->m_size : 0;
    return range && Blizzard::File::Read(fs, file, nullptr, range->m_offset, &amount);
}

void Blizzard::Streaming::HashFileName(Blizzard::Util::Digest<16>* digest, const char* name) {
    std::string lower = CanonicalStreamPath(name ? name : "");
    Blizzard::Util::HashString(digest->m_data, lower.c_str());
}

bool Blizzard::Streaming::InitializeStreaming(const char* manifest, const char* storagePath, const char* variant, const char* dataPath, bool required) {
    s_streamingReady = false;
    s_isStreamingTrial = false;
    if (variant) {
        Blizzard::String::Copy(Blizzard::Storage::s_variantName, variant, sizeof(Blizzard::Storage::s_variantName));
    }
    if (storagePath) {
        Blizzard::Storage::Initialize(storagePath, false);
    }
    if (manifest && *manifest && ManifestInfo::Initialize(manifest)) {
        s_isStreamingTrial = FileManifest::GetIsStreamingTrial();
        System_File::Stacked::PushStreaming();
        s_streamingReady = true;
        return true;
    }
    return !required;
}

std::string Blizzard::Streaming::ResolveFilePath(const std::string& path) { return CanonicalStreamPath(path); }
bool Blizzard::Streaming::IsStreamedFile(const char* path) { return s_streamingReady && ManifestInfo::GetFileItem(ResolveFilePath(path ? path : "")); }
int32_t Blizzard::Streaming::CheckFileRange(const char* path, int64_t, int32_t, const char*, int32_t) { return IsStreamedFile(path) ? 0 : 1; }
int32_t Blizzard::Streaming::GetIsLocalAmount(const char* path, int64_t, int32_t size, const char*) { return IsStreamedFile(path) ? 0 : size; }
bool Blizzard::Streaming::MakeLocalAsync(Blizzard::File::StreamRecord*, int64_t, int32_t, int32_t, const char*) { return true; }
bool Blizzard::Streaming::MakeLocalBlocking(Blizzard::File::StreamRecord*, int64_t, int32_t, int32_t, const char*) { return true; }
bool Blizzard::Streaming::CancelQueue(int32_t) { return true; }
bool Blizzard::Streaming::QueueCompletion(Completion* completion, int32_t) { if (completion) { completion->m_done = true; completion->m_success = true; if (completion->m_event) Blizzard::Lock::EventSet(*completion->m_event); } return true; }
bool Blizzard::Streaming::RemoveCompletion(Completion*, int32_t) { return true; }

Blizzard::Storage::StorageEntryRef::StorageEntryRef(Blizzard::Streaming::StorageEntry* entry) { this->m_entry = entry; }
Blizzard::Streaming::StorageEntry* Blizzard::Storage::Acquire(Blizzard::File::Filesystem*, const std::string&, const char*, int64_t, const char*, int32_t) { return nullptr; }
int32_t Blizzard::Storage::BuildRanges(Blizzard::Streaming::StorageEntry*, int64_t, int32_t, Blizzard::Streaming::LoadRange*) { return 0; }
int32_t Blizzard::Storage::BuildRangesFromStore(Blizzard::Streaming::StorageEntry*, int64_t, int32_t, Blizzard::Streaming::LoadRange*) { return 0; }
int32_t Blizzard::Storage::BuildRangesWithHeader(Blizzard::Streaming::StorageEntry*, int64_t, int32_t, Blizzard::Streaming::LoadRange*) { return 0; }
bool Blizzard::Storage::Close(Blizzard::File::Filesystem*, Blizzard::File::StreamRecord*, Blizzard::Streaming::StorageEntry*) { return true; }
int32_t Blizzard::Storage::CountRanges(Blizzard::Streaming::StorageEntry*, int64_t, int64_t) { return 0; }
bool Blizzard::Storage::FlushFile(Blizzard::File::Filesystem*, Blizzard::File::StreamRecord*) { return true; }
int32_t Blizzard::Storage::GetBlockLength(Blizzard::Streaming::StorageEntry*, int32_t) { return 0; }
int64_t Blizzard::Storage::GetNextHeaderOffset() { return 0; }
Blizzard::Streaming::LoadRange* Blizzard::Storage::GetRange(Blizzard::Streaming::StorageEntry*, int64_t, int32_t, int32_t* count, bool) { if (count) *count = 0; return nullptr; }
bool Blizzard::Storage::Initialize() { s_storageReady = true; return true; }
bool Blizzard::Storage::Initialize(const char* path, bool required) { s_storagePath = path ? path : ""; s_storageReady = true; return true; }
bool Blizzard::Storage::IsComplete(Blizzard::Streaming::StorageEntry*) { return false; }
bool Blizzard::Storage::QueueWrite(Blizzard::File::Filesystem*, Blizzard::File::StreamRecord*, uint8_t* data, int64_t, int32_t, bool owned) { if (owned) Blizzard::Memory::Free(data); return true; }
bool Blizzard::Storage::ReadAt(Blizzard::File::Filesystem* fs, Blizzard::File::StreamRecord* file, void* buffer, int64_t offset, int32_t size) { return Blizzard::File::Read(fs, file, buffer, offset, &size); }
bool Blizzard::Storage::ReadLocal(Blizzard::File::Filesystem* fs, Blizzard::File::StreamRecord* file, Blizzard::Streaming::StorageEntry*, uint8_t* buffer, int64_t offset, int32_t size) { return ReadAt(fs, file, buffer, offset, size); }
bool Blizzard::Storage::ReadStorageFile(const uint8_t*, int32_t) { return true; }
bool Blizzard::Storage::Remove(const std::string&) { return true; }
bool Blizzard::Storage::RequestRange(Blizzard::Streaming::StorageEntry*, int64_t, int32_t, int32_t) { return false; }
void Blizzard::Storage::Shutdown() { s_storageReady = false; }
void Blizzard::Storage::SetRangeAs(Blizzard::Streaming::StorageEntry*, int64_t, int32_t, int32_t) {}
uint32_t Blizzard::Storage::StorageThread(void*) { return 0; }
bool Blizzard::Storage::WriteAt(Blizzard::File::StreamRecord* file, const void* buffer, int64_t offset, int32_t size) { return Blizzard::File::Write(file, buffer, offset, &size); }
bool Blizzard::Storage::WriteBlock(Blizzard::Streaming::WriteRequest*) { return true; }
bool Blizzard::Storage::WriteIndex() { return true; }
bool Blizzard::Storage::WriteIndexEntry(Blizzard::Streaming::StorageEntry*, int32_t, bool) { return true; }
bool Blizzard::Storage::WritePartBlockInfo(Blizzard::Streaming::StorageEntry*, Blizzard::File::StreamRecord*, int32_t) { return true; }
Blizzard::Streaming::StorageEntry* Blizzard::Storage::PartialFile::CreateShared(const std::string&, const char*, int64_t, const char*, int32_t) { return nullptr; }
bool Blizzard::Storage::PartialFile::ParseFile(Blizzard::File::Filesystem*, Blizzard::File::StreamRecord*) { return false; }
bool Blizzard::Storage::PartialFile::RebuildHeader(Blizzard::File::Filesystem*, Blizzard::File::StreamRecord*) { return true; }

bool System_File::Stacked::stream_flush(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return fs->m_next ? (*reinterpret_cast<FileFunc*>(reinterpret_cast<uintptr_t>(fs->m_next) + parms->m_verb))(fs->m_next, parms) : true;
}

void System_Streaming::HttpCallback::Completed(Blizzard::URL::DownloadCallback::Status status) { this->m_status = status; }
System_Streaming::HttpCallback::HttpCallback() { this->m_data.reserve(1024); this->m_received = 0; this->m_status = Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS; this->m_timeout = 3000; this->m_event = nullptr; }
System_Streaming::HttpCallback::~HttpCallback() {}

namespace {
bool DownloadToString(const char* url, std::string& data, uint32_t timeout, Blizzard::URL::DownloadCallback::Status* status) {
    System_Streaming::HttpCallback callback;
    callback.SetTimeOutValue(timeout);
    bool result = Blizzard::URL::DownloadURL(url, &callback);
    if (status) *status = callback.m_status;
    data = callback.m_data;
    return result && callback.m_status == Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS;
}

template <typename Reader>
bool ReadIndirect(const char* url, const char* label, Reader reader) {
    std::string data;
    Blizzard::URL::DownloadCallback::Status status = Blizzard::URL::DownloadCallback::DOWNLOAD_FAILED;
    uint32_t timeout = 3000;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (DownloadToString(url, data, timeout, &status)) {
            return reader(const_cast<char*>(data.c_str()), static_cast<int32_t>(data.size()));
        }
        if (status == Blizzard::URL::DownloadCallback::DOWNLOAD_TIMEOUT) timeout *= 2;
    }
    Blizzard::File::AddToLastErrorStack(status, Blizzard::String::QuickFormat<1024>("%s - DownloadURL failed - %s", label, url).m_buffer, 20);
    return false;
}
}

bool Blizzard::Streaming::FileManifest::ReadIndirectFile(const char* url) { return ReadIndirect(url, "FileManifest::ReadIndirectFile", ReadDirectFileFromBuffer); }
bool Blizzard::Streaming::Transport::ReadIndirectFile(const char* url) { return ReadIndirect(url, "Transport::ReadIndirectFile", ReadDirectFileFromBuffer); }
bool Blizzard::Streaming::SourceManifest::ReadIndirectFile(const char* url) { return ReadIndirect(url, "SourceManifest::ReadIndirectFile", ReadDirectFileFromBuffer); }
bool Blizzard::Streaming::ManifestInfo::ReadIndirectFile(const char* url) { return ReadIndirect(url, "ManifestInfo::ReadIndirectFile", [this](char* b, int32_t s) { return this->ReadDirectFileFromBuffer(b, s); }); }

int32_t Blizzard::Streaming::Transport::AdjustRange(int64_t* start, int64_t* end, int32_t md5Size, int64_t fileSize) {
    if (md5Size <= 0) return 0;
    int64_t alignedStart = *start - *start % md5Size;
    int64_t alignedEnd = md5Size + *end - (md5Size + *end) % md5Size - 1;
    if (alignedEnd > fileSize) alignedEnd = fileSize - 1;
    int32_t skip = static_cast<int32_t>(32 * (alignedStart / md5Size));
    int64_t original = *start;
    *start = skip + alignedStart;
    *end = alignedEnd + 32 * ((alignedEnd + 1) / md5Size);
    return static_cast<int32_t>(skip + original - *start);
}
void Blizzard::Streaming::Transport::THROTTLE::Wait() {}
Blizzard::Streaming::TransportHttpCallback::~TransportHttpCallback() {}
int32_t Blizzard::Streaming::TransportHttpCallback::GetTimeout() { return this->m_timeout; }
Blizzard::URL::DownloadCallback::ByteRange Blizzard::Streaming::TransportHttpCallback::GetByteRange() { ByteRange r; r.m_start = m_rangeStart; r.m_end = m_rangeEnd; return r; }
bool Blizzard::Streaming::TransportHttpCallback::ShouldBlock() { return true; }
void Blizzard::Streaming::TransportHttpCallback::SetByteRange(int64_t start, int64_t end) { m_rangeStart = start; m_rangeEnd = end; }
void Blizzard::Streaming::TransportHttpCallback::SetTimeOutValue(uint32_t timeout) { m_timeout = timeout; }
bool Blizzard::Streaming::TransportHttpCallback::Stop() { return m_stop; }
Blizzard::URL::DownloadCallback::Cookie Blizzard::Streaming::TransportHttpCallback::GetCookie() { Cookie c; c.m_name = m_cookieName.c_str(); c.m_value = m_cookieValue.c_str(); return c; }
Blizzard::Streaming::TransportHttpCallback::TransportHttpCallback(uint32_t size) { m_data.reserve(size); m_received = 0; m_rangeStart = 0; m_rangeEnd = size ? size - 1 : 0; m_timeout = 10000; m_event = nullptr; m_stop = false; m_status = Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS; m_success = false; }
void Blizzard::Streaming::TransportHttpCallback::SetCookie(const char* name, const char* value) { m_cookieName = name ? name : ""; m_cookieValue = value ? value : ""; }
bool Blizzard::Streaming::TransportHttpCallback::ReceivedData(const void* data, int32_t size) { if (size > 0) { m_data.append(static_cast<const char*>(data), size); m_received += size; } return true; }
void Blizzard::Streaming::TransportHttpCallback::Completed(Blizzard::URL::DownloadCallback::Status status) { m_status = status; m_success = (status == Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS || status == Blizzard::URL::DownloadCallback::DOWNLOAD_STOPPED); }
Blizzard::Streaming::URL_INFO::URL_INFO(std::string url, int64_t start, int64_t end, int32_t size) { m_url = url; m_start = start; m_end = end; m_size = size; }
void Blizzard::Streaming::Transport::GetSplitFileURL(const std::string& url, int64_t start, int64_t end, int32_t, int32_t, int32_t, int32_t size, std::vector<URL_INFO>* urls) { urls->push_back(URL_INFO(url, start, end, size)); }
bool Blizzard::Streaming::Transport::MD5_Data(std::string* out, std::string* data, int32_t, int64_t, int64_t) { if (out && data) *out = *data; return true; }
bool Blizzard::Streaming::Transport::Read(Manifests::TransportItem* transport, std::string url, int32_t maxRetry, void* data, int64_t fileSize, int64_t offset, int32_t size) { return !_strnicmp(url.c_str(), s_filePrefix, strlen(s_filePrefix)) ? ReadFile(transport, url, maxRetry, data, fileSize, offset, size) : ReadHttp(transport, url, maxRetry, data, fileSize, offset, size); }
bool Blizzard::Streaming::Transport::ReadFile(Manifests::TransportItem*, std::string url, int32_t, void* data, int64_t, int64_t offset, int32_t size) { Blizzard::File::StreamRecord* file; if (!Blizzard::File::Open(url.c_str() + strlen(s_filePrefix), 0x1005, file)) return false; int32_t count = size; bool ok = Blizzard::File::Read(file, data, offset, &count) && count == size; Blizzard::File::Close(file); return ok; }
bool Blizzard::Streaming::Transport::ReadHttp(Manifests::TransportItem*, std::string url, int32_t, void* data, int64_t, int64_t offset, int32_t size) { TransportHttpCallback callback(size); callback.SetByteRange(offset, offset + size - 1); if (!Blizzard::URL::DownloadURL(url.c_str(), &callback) || !callback.m_success || callback.m_data.size() < static_cast<size_t>(size)) return false; memcpy(data, callback.m_data.data(), size); return true; }

void StopStreaming() { Blizzard::Streaming::s_streamingReady = false; Blizzard::Storage::Shutdown(); }
char* GetComponentVersionData(uint32_t* size) { if (size) *size = 0; return nullptr; }

#endif
