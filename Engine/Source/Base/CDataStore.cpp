#include <cstdint>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: f8214604 auto
// OFFSET: 0x00401050
CDataStore::CDataStore() {
    this->m_data = nullptr;
    this->m_base = 0;
    this->m_alloc = 0;
    this->m_size = 0;
    this->m_read = -1;
}

// VERIFY: 4e5e5344 auto
// OFFSET: 0x004669A0
CDataStore::CDataStore(uint8_t* data, uint32_t size) {
    this->m_data = data;
    this->m_base = 0;
    this->m_alloc = -1;
    this->m_size = size;
    this->m_read = 0;
}

// VERIFY: 750f93c6 manual
// OFFSET: 0x00401070
int32_t CDataStore::FetchWrite(uint32_t pos, uint32_t bytes, const char* fileName, int32_t lineNumber) {
    if (pos >= this->m_base && pos + bytes <= this->m_base + this->m_alloc) {
        return 1;
    }

    return this->InternalFetchWrite(pos, bytes, this->m_data, this->m_base, this->m_alloc, fileName, lineNumber) ? 1 : 0;
}

// VERIFY: 4e4a9229 auto
// OFFSET: 0x004010D0
int32_t CDataStore::IsRead() {
    return this->m_read == this->m_size;
}

// VERIFY: fb52a736 manual
// OFFSET: 0x004010E0
void CDataStore::Reset() {
    if (this->m_alloc == -1) {
        this->m_data = nullptr;
        this->m_alloc = 0;
    }

    this->FetchWrite(0, 0, nullptr, 0);
    this->m_read = -1;
    this->m_size = 0;
}

// VERIFY: 3e14d491 auto
// OFFSET: 0x00401130
void CDataStore::Finalize() {
    this->m_read = 0;
}

// VERIFY: edfdbe12 auto
// OFFSET: [0x00403880, 0x004038A0]
CDataStore::~CDataStore() {
    if (this->m_alloc != -1) {
        this->InternalDestroy(this->m_data, this->m_base, this->m_alloc);
    }
}

// INLINED
void CDataStore::InternalInitialize(uint8_t*& data, uint32_t& base, uint32_t& alloc) {
}

// VERIFY: 1b9351fd auto
// OFFSET: 0x0047ADE0
void CDataStore::GetBufferParams(const void** data, uint32_t* size, uint32_t* alloc) {
    if (data) {
        *data = this->m_data;
    }

    if (size) {
        *size = this->m_size;
    }

    if (alloc) {
        *alloc = this->m_alloc;
    }
}

// VERIFY: cb500213 manual
// OFFSET: 0x0047AE10
void CDataStore::DetachBuffer(void** data, uint32_t* size, uint32_t* alloc) {
    if (data) {
        *data = this->m_data;
    }

    if (size) {
        *size = this->m_size;
    }

    if (alloc) {
        *alloc = this->m_alloc;
    }

    this->m_data = nullptr;
    this->m_alloc = 0;
    this->Reset();
}

// VERIFY: 25292a15 manual
// OFFSET: 0x0047AE50
void CDataStore::InternalDestroy(uint8_t*& data, uint32_t& base, uint32_t& alloc) {
    if (alloc && data) {
        SMemFree(data, __FILE__, __LINE__, 0);
    }

    data = nullptr;
    base = 0;
    alloc = 0;
}

// VERIFY: 6fabe025 manual
// OFFSET: 0x0047AEA0
int32_t CDataStore::InternalFetchWrite(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc, const char* fileName, int32_t lineNumber) {
    alloc = (pos + bytes + 255) & ~255;

    if (!fileName) {
        fileName = __FILE__;
        lineNumber = __LINE__;
    }

    data = static_cast<uint8_t*>(SMemReAlloc(data, alloc, fileName, lineNumber, 0));

    return 1;
}

// VERIFY: 54ecbc33 manual
// OFFSET: 0x0047AEF0
CDataStore& CDataStore::Set(uint32_t pos, uint8_t val) {
    this->FetchWrite(pos, sizeof(val), nullptr, 0);
    this->m_data[pos - this->m_base] = val;

    return *this;
}

// VERIFY: b25daa57 manual
// OFFSET: 0x0047AF40
CDataStore& CDataStore::Set(uint32_t pos, uint16_t val) {
    this->FetchWrite(pos, sizeof(val), nullptr, 0);
    *reinterpret_cast<uint16_t*>(&this->m_data[pos - this->m_base]) = val;

    return *this;
}

// VERIFY: 15a69477 manual
// OFFSET: 0x0047AF90
CDataStore& CDataStore::Set(uint32_t pos, uint32_t val) {
    this->FetchWrite(pos, sizeof(val), nullptr, 0);
    *reinterpret_cast<uint32_t*>(&this->m_data[pos - this->m_base]) = val;

    return *this;
}

// VERIFY: 1c0da3b2 manual
// OFFSET: 0x0047AFE0
CDataStore& CDataStore::Put(uint8_t val) {
    this->FetchWrite(this->m_size, sizeof(val), nullptr, 0);
    this->m_data[this->m_size - this->m_base] = val;
    this->m_size += sizeof(val);

    return *this;
}

// VERIFY: f2e75ec1 manual
// OFFSET: 0x0047B040
CDataStore& CDataStore::Put(uint16_t val) {
    this->FetchWrite(this->m_size, sizeof(val), nullptr, 0);
    *reinterpret_cast<uint16_t*>(&this->m_data[this->m_size - this->m_base]) = val;
    this->m_size += sizeof(val);

    return *this;
}

// VERIFY: 3667bd7a manual
// OFFSET: 0x0047B0A0
CDataStore& CDataStore::Put(uint32_t val) {
    this->FetchWrite(this->m_size, sizeof(val), nullptr, 0);
    *reinterpret_cast<uint32_t*>(&this->m_data[this->m_size - this->m_base]) = val;
    this->m_size += sizeof(val);

    return *this;
}

// VERIFY: d0461454 manual
// OFFSET: 0x0047B100
CDataStore& CDataStore::Put(uint64_t val) {
    this->FetchWrite(this->m_size, sizeof(val), nullptr, 0);
    *reinterpret_cast<uint64_t*>(&this->m_data[this->m_size - this->m_base]) = val;
    this->m_size += sizeof(val);

    return *this;
}

// VERIFY: fb92f4e9 manual
// OFFSET: 0x0047B160
CDataStore& CDataStore::Put(float val) {
    this->FetchWrite(this->m_size, sizeof(val), nullptr, 0);
    *reinterpret_cast<float*>(&this->m_data[this->m_size - this->m_base]) = val;
    this->m_size += sizeof(val);

    return *this;
}

// VERIFY: 16d8b8ae manual
// OFFSET: 0x0047B1C0
CDataStore& CDataStore::PutArray(const uint8_t* pval, uint32_t count) {
    if (!pval) {
        if (count) {
            SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        }

        return *this;
    }

    this->FetchWrite(this->m_size, count, nullptr, 0);

    while (count) {
        uint32_t bytes = count < this->m_alloc ? count : this->m_alloc;

        if (bytes <= 1) {
            bytes = 1;
        }

        this->FetchWrite(this->m_size, bytes, nullptr, 0);
        uint8_t* dest = &this->m_data[this->m_size - this->m_base];

        if (dest != pval) {
            memcpy(dest, pval, bytes);
        }

        pval += bytes;
        this->m_size += bytes;
        count -= bytes;
    }

    return *this;
}

// VERIFY: ebebf2be manual
// OFFSET: 0x0047B280
CDataStore& CDataStore::PutData(const void* data, uint32_t bytes) {
    return this->PutArray(static_cast<const uint8_t*>(data), bytes);
}

// VERIFY: cef12e51 manual
// OFFSET: 0x0047B290
int32_t CDataStore::FetchRead(uint32_t pos, uint32_t bytes) {
    if (pos + bytes > this->m_size) {
        this->m_read = this->m_size + 1;
        return 0;
    }

    if (pos >= this->m_base && pos + bytes <= this->m_base + this->m_alloc) {
        return 1;
    }

    if (this->InternalFetchRead(pos, bytes, this->m_data, this->m_base, this->m_alloc)) {
        return 1;
    }

    this->m_read = this->m_size + 1;
    return 0;
}

// VERIFY: 05c2b42b manual
// OFFSET: 0x0047B300
CDataStore& CDataStore::PutString(const char* string) {
    if (string) {
        this->PutArray(reinterpret_cast<const uint8_t*>(string), static_cast<uint32_t>(strlen(string)) + 1);
    } else {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    }

    return *this;
}

// VERIFY: c61d4ce4 manual
// OFFSET: 0x0047B340
CDataStore& CDataStore::Get(uint8_t& val) {
    if (this->FetchRead(this->m_read, sizeof(val))) {
        val = this->m_data[this->m_read - this->m_base];
        this->m_read += sizeof(val);
    }

    return *this;
}

// VERIFY: 2b9a91aa auto
// OFFSET: 0x0047B380
CDataStore& CDataStore::Get(uint16_t& val) {
    if (this->FetchRead(this->m_read, sizeof(val))) {
        val = *reinterpret_cast<uint16_t*>(&this->m_data[this->m_read - this->m_base]);
        this->m_read += sizeof(val);
    }

    return *this;
}

// VERIFY: 9852bc19 auto
// OFFSET: 0x0047B3C0
CDataStore& CDataStore::Get(uint32_t& val) {
    if (this->FetchRead(this->m_read, sizeof(val))) {
        val = *reinterpret_cast<uint32_t*>(&this->m_data[this->m_read - this->m_base]);
        this->m_read += sizeof(val);
    }

    return *this;
}

// VERIFY: 1e1f3874 auto
// OFFSET: 0x0047B400
CDataStore& CDataStore::Get(uint64_t& val) {
    if (this->FetchRead(this->m_read, sizeof(val))) {
        val = *reinterpret_cast<uint64_t*>(&this->m_data[this->m_read - this->m_base]);
        this->m_read += sizeof(val);
    }

    return *this;
}

// VERIFY: d6408fc8 manual
// OFFSET: 0x0047B440
CDataStore& CDataStore::Get(float& val) {
    if (this->FetchRead(this->m_read, sizeof(val))) {
        val = *reinterpret_cast<float*>(&this->m_data[this->m_read - this->m_base]);
        this->m_read += sizeof(val);
    }

    return *this;
}

// VERIFY: e39fa63b manual
// OFFSET: 0x0047B480
CDataStore& CDataStore::GetString(char* string, uint32_t maxChars) {
    if (!string) {
        if (maxChars) {
            SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        }

        return *this;
    }

    if (!maxChars) {
        return *this;
    }

    if (this->m_read <= this->m_size) {
        uint32_t copied = 0;

        if (this->FetchRead(this->m_read, 1)) {
            do {
                uint32_t end = this->m_base + this->m_alloc < this->m_size ? this->m_base + this->m_alloc : this->m_size;
                uint32_t chars = end - this->m_read < maxChars - copied ? end - this->m_read : maxChars - copied;
                uint8_t* src = &this->m_data[this->m_read - this->m_base];
                uint32_t read = 0;

                for (; chars; chars--) {
                    uint8_t c = src[read];
                    string[copied++] = c;
                    read++;

                    if (!c) {
                        break;
                    }
                }

                this->m_read += read;

                if (chars) {
                    break;
                }

                if (copied >= maxChars) {
                    this->m_read = this->m_size + 1;
                    break;
                }
            } while (this->FetchRead(this->m_read, 1));
        }
    }

    if (this->m_read > this->m_size) {
        string[0] = '\0';
    }

    return *this;
}

// VERIFY: 35d578c9 manual
// OFFSET: 0x0047B560
CDataStore& CDataStore::GetArray(uint8_t* pval, uint32_t count) {
    if (!pval && count) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return *this;
    }

    if (this->m_read <= this->m_size) {
        while (count) {
            uint32_t bytes = this->m_size - this->m_read < count ? this->m_size - this->m_read : count;
            bytes = bytes < this->m_alloc ? bytes : this->m_alloc;

            if (bytes <= 1) {
                bytes = 1;
            }

            if (!this->FetchRead(this->m_read, bytes)) {
                break;
            }

            uint8_t* src = &this->m_data[this->m_read - this->m_base];

            if (pval != src) {
                memcpy(pval, src, bytes);
            }

            pval += bytes;
            this->m_read += bytes;
            count -= bytes;
        }
    }

    return *this;
}

// VERIFY: 97a1b952 auto
// OFFSET: 0x0047B5F0
CDataStore& CDataStore::GetArray(uint32_t* pval, uint32_t count) {
    if (!pval && count) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return *this;
    }

    if (this->m_read <= this->m_size) {
        uint32_t remaining = count * sizeof(uint32_t);

        while (remaining) {
            uint32_t bytes = this->m_size - this->m_read < remaining ? this->m_size - this->m_read : remaining;
            bytes = bytes < this->m_alloc ? bytes : this->m_alloc;

            if (bytes <= sizeof(uint32_t)) {
                bytes = sizeof(uint32_t);
            }

            bytes &= ~(sizeof(uint32_t) - 1);

            if (!this->FetchRead(this->m_read, bytes)) {
                break;
            }

            memcpy(pval, &this->m_data[this->m_read - this->m_base], bytes / sizeof(uint32_t) * sizeof(uint32_t));
            pval += bytes / sizeof(uint32_t);
            this->m_read += bytes;
            remaining -= bytes;
        }
    }

    return *this;
}

// VERIFY: f83befa5 manual
// OFFSET: 0x0047B6A0
CDataStore& CDataStore::GetData(void* data, uint32_t bytes) {
    return this->GetArray(static_cast<uint8_t*>(data), bytes);
}

// VERIFY: fee4b0f5 auto
// OFFSET: 0x0047B6B0
CDataStore& CDataStore::GetDataInSitu(void*& data, uint32_t bytes) {
    data = nullptr;

    if (this->FetchRead(this->m_read, bytes)) {
        data = &this->m_data[this->m_read - this->m_base];
        this->m_read += bytes;
    }

    return *this;
}

// INLINED
int32_t CDataStore::InternalFetchRead(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc) {
    return 0;
}

// INLINED
uint8_t* CDataStore::GetHeaderSpace(uint32_t bytes) {
    return nullptr;
}

// VERIFY: 97bd27e2 manual
// OFFSET: 0x00465FD0
void* CDataStore::Alloc(uint32_t bytes, const char* fileName, int32_t lineNumber) {
    if (!bytes) {
        return nullptr;
    }

    if (fileName) {
        return SMemAlloc(bytes, fileName, lineNumber, 0);
    }

    return SMemAlloc(bytes, __FILE__, __LINE__, 0);
}

// VERIFY: 5f3c641c manual
// OFFSET: 0x00466010
void* CDataStore::ReAlloc(void* ptr, uint32_t bytes, const char* fileName, int32_t lineNumber) {
    if (fileName) {
        return SMemReAlloc(ptr, bytes, fileName, lineNumber, 0);
    }

    return SMemReAlloc(ptr, bytes, __FILE__, __LINE__, 0);
}

// VERIFY: 8b1ed662 auto
// OFFSET: 0x00401140
CDataStore& CDataStore::GetInt8(uint32_t& val) {
    uint8_t value = 0;
    this->Get(value);
    val = value;
    return *this;
}

// VERIFY: 8c30e384 auto
// OFFSET: [0x00464560, 0x004D3870]
uint8_t CDataStore::GetByte() {
    uint8_t val = 0;
    this->Get(val);

    return val;
}

// VERIFY: 76f41d37 auto
// OFFSET: 0x0050F420
CDataStore& CDataStore::Get(int32_t& val) {
    uint32_t value = 0;
    this->Get(value);
    val = value;

    return *this;
}
