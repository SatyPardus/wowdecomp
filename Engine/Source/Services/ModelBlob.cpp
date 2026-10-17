#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B4A270
static CModelBlob* s_modelBlob;

// STATIC: 0x00B4A274
static TSHashTableReuse<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE> s_modelBlobHT;

// INLINED
HASHKEY_MODELBLOBFILE::HASHKEY_MODELBLOBFILE() {
    this->m_filename = nullptr;
}

// INLINED
bool HASHKEY_MODELBLOBFILE::operator==(const HASHKEY_MODELBLOBFILE& key) const {
    return !SStrCmpI(this->m_filename, key.m_filename, 260);
}

// INLINED
CModelBlob::CHashEntry::CHashEntry() {
    this->m_blob = nullptr;
    this->m_entry = nullptr;
}

// INLINED
CModelBlob::CHashEntry::~CHashEntry() {
    this->m_blob = nullptr;
    this->m_entry = nullptr;
}

// INLINED
CModelBlob::CModelBlob(const char* fileName) {
    this->m_data = nullptr;
    this->m_entries = nullptr;
    this->m_entryCount = 0;
    this->m_stringTable = nullptr;
    this->Load(fileName);
}

// VERIFY: e1be90a5 manual
// OFFSET: 0x004BAF60
CModelBlob::~CModelBlob() {
    if (this->m_data) {
        SMemFree(this->m_data, __FILE__, __LINE__, 0);
    }

    this->m_entries = nullptr;
    this->m_entryCount = 0;
    this->m_stringTable = nullptr;
    s_modelBlobHT.Clear();
}

// VERIFY: a7881e40 auto
// OFFSET: 0x004BB1C0
void ModelBlobDestroy() {
    if (s_modelBlob) {
        delete s_modelBlob;
        s_modelBlob = nullptr;
    }
}

// VERIFY: 74063afe manual
// OFFSET: 0x004BB370
int32_t CModelBlob::Query(const char* modelName, C3Vector& min, C3Vector& max) {
    uint32_t hashval = SStrHashHT(modelName);
    HASHKEY_MODELBLOBFILE key;
    key.m_filename = modelName;
    CHashEntry* entry = s_modelBlobHT.Ptr(hashval, key);

    if (!entry) {
        return 0;
    }

    min = entry->m_entry->m_boundingBox.b;
    max = entry->m_entry->m_boundingBox.t;

    return 1;
}

// VERIFY: 7fa98d10 auto
// OFFSET: 0x004BB3E0
int32_t ModelBlobQuery(const char* modelName, C3Vector& min, C3Vector& max) {
    if (!s_modelBlob) {
        return 0;
    }

    return s_modelBlob->Query(modelName, min, max);
}

// VERIFY: 8ef8f2aa manual
// OFFSET: 0x004BBA70
void CModelBlob::HashBlobEntry(SEntry* entry) {
    const char* name = this->m_stringTable + entry->m_filenameOffset;
    uint32_t hashval = SStrHashHT(name);
    HASHKEY_MODELBLOBFILE key;
    key.m_filename = name;

    if (s_modelBlobHT.Ptr(hashval, key)) {
        return;
    }

    CHashEntry* hashEntry = s_modelBlobHT.New(hashval, key, 0, 0);
    hashEntry->m_blob = this;
    hashEntry->m_entry = entry;
}

// VERIFY: 3c3bf082 manual
// OFFSET: 0x004BBB20
void CModelBlob::Load(const char* fileName) {
    SFileImpl* file = nullptr;
    SErrSetLastError(0);

    if (!SFile::OpenEx(nullptr, fileName, 0, &file)) {
        if (!SErrGetLastError()) {
            SErrSetLastError(STORM_ERROR_FILE_NOT_FOUND);
        }

        return;
    }

    SStrCopy(this->m_filename, fileName, sizeof(this->m_filename));
    uint32_t high = 0;
    const uint32_t size = SFileGetFileSize(file, &high);
    if (high || size < 28 || size > 64 * 1024 * 1024) {
        SFileCloseFile(file);
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }
    auto* bytes = static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0));
    uint32_t read = 0;
    const bool complete = bytes && SFileReadFile(file, bytes, size, &read, nullptr, 0) && read == size;
    SFileCloseFile(file);
    auto reject = [&] { SMemFree(bytes, __FILE__, __LINE__, 0); SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); };
    if (!complete) { reject(); return; }
    auto word = [&](uint32_t offset) { uint32_t value; std::memcpy(&value, bytes + offset, 4); return value; };
    // The original format is three consecutive chunks: version, entries, names.
    const uint32_t versionBytes = word(4);
    if (versionBytes < 4 || versionBytes > size - 24 || versionBytes % 4) { reject(); return; }
    const uint32_t entryChunk = 8 + versionBytes;
    const uint32_t entryBytes = word(entryChunk + 4);
    if (entryBytes > size - entryChunk - 16 || entryBytes % sizeof(SEntry)) { reject(); return; }
    const uint32_t stringChunk = entryChunk + 8 + entryBytes;
    const uint32_t stringBytes = word(stringChunk + 4);
    if (stringBytes > size - stringChunk - 8) { reject(); return; }
    auto* entries = reinterpret_cast<SEntry*>(bytes + entryChunk + 8);
    const char* names = reinterpret_cast<const char*>(bytes + stringChunk + 8);
    const uint32_t count = entryBytes / sizeof(SEntry);
    for (uint32_t i = 0; i < count; ++i) {
        const auto offset = entries[i].m_filenameOffset;
        if (offset >= stringBytes || !std::memchr(names + offset, 0, stringBytes - offset)) { reject(); return; }
    }
    this->m_version = word(8);
    this->m_data = bytes;
    this->m_entries = entries;
    this->m_entryCount = count;
    this->m_stringTable = names;
    for (uint32_t i = 0; i < count; ++i) this->HashBlobEntry(&entries[i]);
}

// VERIFY: b703c86f manual
// OFFSET: 0x004BBC20
int32_t ModelBlobLoad(const char* fileName) {
    if (s_modelBlob) {
        delete s_modelBlob;
        s_modelBlob = nullptr;
    }

    s_modelBlob = new CModelBlob(fileName);

    if (s_modelBlob->m_data) {
        return 1;
    }

    delete s_modelBlob;
    s_modelBlob = nullptr;

    return 0;
}
