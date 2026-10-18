#include "FileDownload.h"
#include <cstdint>
#include <cstring>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/IOUnit/IOFileUnit.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Storm/Source/SLock.h"

// STATIC: 0x00C7C000
static SCritSect s_downloadLock;

// INLINED
FileDownload::FileDownload() {
    this->m_partial = false;
    this->m_status = 0;
    this->m_reason = 0;
    this->m_accepted = false;
    this->m_bytesSinceSave = 0;
    this->m_reasonData = 0;
    memset(this->m_fileName, 0, sizeof(this->m_fileName));
}

// VERIFY: 0d9466ed manual
// OFFSET: 0x006BA620
FileDownload::~FileDownload() {
}

// VERIFY: 8e443d41 auto
// OFFSET: 0x006B9DF0
GruntFileDownload::GruntFileDownload() {
    this->m_unit = nullptr;
    this->m_header.m_received = 0;
    this->m_header.m_size = 0;
    memset(this->m_header.m_md5, 0, sizeof(this->m_header.m_md5));
    this->m_header.m_version = 0;
    this->m_header.m_magic = 'DInf';
}

// VERIFY: b8dde84c auto
// OFFSET: 0x006B9E80
int32_t GruntFileDownload::Delete() {
    char path[260];

    if (this->m_partial) {
        SStrPrintf(path, sizeof(path), "%s.partial", this->m_fileName);
    } else {
        SStrCopy(path, this->m_fileName, sizeof(path));
    }

    return OsDeleteFile(path);
}

// VERIFY: 65bb752e auto
// OFFSET: 0x006B9EF0
void GruntFileDownload::RenamePartial() {
    if (!this->m_partial) {
        return;
    }

    char path[260];
    SStrPrintf(path, sizeof(path), "%s.partial", this->m_fileName);

    if (OsFileExists(this->m_fileName)) {
        OsDeleteFile(this->m_fileName);
    }

    if (!OsMoveFile(path, this->m_fileName)) {
        this->m_status = 3;
        this->m_reason = 1;
    }
}

// VERIFY: 86328908 auto
// OFFSET: 0x006B9F70
void STORMAPI GruntFileDownload::CalculateMD5(Storm::IOUnit* unit, uint64_t offset, uint64_t size, uint8_t* digest) {
    uint8_t buffer[0x8000];
    MD5_CTX context;
    MD5Init(&context);

    uint32_t pos = static_cast<uint32_t>(offset);
    uint32_t end = static_cast<uint32_t>(size) - static_cast<uint32_t>(offset);

    while (pos < end) {
        uint32_t count = end - pos;

        if (count >= 0x8000) {
            count = 0x8000;
        }

        if (!unit->Read(buffer, pos, count, nullptr)) {
            memset(digest, 0, 16);
            return;
        }

        pos += count;
        MD5Update(&context, buffer, count);
    }

    MD5Final(digest, &context);
}

// VERIFY: 236b88e1 manual
// OFFSET: 0x006BA020
bool GruntFileDownload::VerifyMD5(uint64_t size) {
    uint8_t digest[16];
    GruntFileDownload::CalculateMD5(this->m_unit, 0, size, digest);
    return memcmp(digest, this->m_header.m_md5, sizeof(digest)) == 0;
}

// VERIFY: 4ef02168 manual
// OFFSET: 0x006BA0F0
bool GruntFileDownload::VerifyFile() {
    int64_t size;
    this->m_unit->GetFileSize(&size);
    return static_cast<uint64_t>(size) == this->m_header.m_size && this->VerifyMD5(size);
}

// VERIFY: 9010eb69 manual
// OFFSET: 0x006BA140
bool GruntFileDownload::ValidateForContinue(bool& complete) {
    complete = false;

    int64_t size;
    this->m_unit->GetFileSize(&size);

    if (static_cast<uint64_t>(size) == this->m_header.m_size + sizeof(Header)) {
        Header header;

        if (this->m_unit->Read(&header, size - sizeof(Header), sizeof(Header), nullptr) && header.m_magic == 'DInf' && !header.m_version && !memcmp(header.m_md5, this->m_header.m_md5, sizeof(header.m_md5)) && header.m_size == this->m_header.m_size) {
            if (header.m_size != header.m_received) {
                this->m_header = header;
                return true;
            }

            if (this->VerifyMD5(header.m_size)) {
                complete = true;
                this->m_header = header;
                return true;
            }
        }
    } else if (static_cast<uint64_t>(size) == this->m_header.m_size) {
        complete = this->VerifyFile();

        if (complete) {
            return true;
        }
    }

    return false;
}

// VERIFY: c4202440 auto
// OFFSET: 0x006BA2D0
int32_t GruntFileDownload::GetStatus() {
    return this->m_status;
}

// VERIFY: a39a9c4a manual
// OFFSET: 0x006BA2E0
void GruntFileDownload::WriteHeader() {
    if (!this->m_unit) {
        return;
    }

    Header header = this->m_header;

    if (!this->m_unit->Write(&header, this->m_header.m_size, sizeof(header))) {
        this->m_status = 3;
        this->m_reason = 1;
    }
}

// VERIFY: 042eb4d2 manual
// OFFSET: 0x006BA340
void GruntFileDownload::Proceed(const uint8_t* data, uint32_t size) {
    s_downloadLock.Enter();

    bool success = true;

    if (this->m_unit && this->m_unit->Write(data, this->m_header.m_received, size)) {
        this->m_header.m_received += size;
        this->m_bytesSinceSave += size;

        if (this->m_header.m_received <= this->m_header.m_size) {
            if (this->m_bytesSinceSave >= 0x100000 || this->m_header.m_received == this->m_header.m_size) {
                this->WriteHeader();
            }
        } else {
            this->m_status = 3;
            this->m_reason = 1;
        }

        if (this->m_header.m_received == this->m_header.m_size) {
            uint8_t digest[16];
            GruntFileDownload::CalculateMD5(this->m_unit, 0, this->m_header.m_size, digest);
            this->m_status = (memcmp(digest, this->m_header.m_md5, sizeof(digest)) != 0) + 2;
        }
    } else {
        success = false;
    }

    s_downloadLock.Leave();

    if (!success) {
        this->m_status = 3;
    }
}

// VERIFY: 613103d4 manual
// OFFSET: 0x006BA500
float GruntFileDownload::GetProgress() {
    if (!this->m_header.m_size) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(this->m_header.m_received) / static_cast<double>(this->m_header.m_size));
}

// INLINED
inline void GruntFileDownload::Close(bool deleteFile, bool rename) {
    s_downloadLock.Enter();

    if (this->m_unit) {
        this->m_unit->Release();
        this->m_unit = nullptr;
    }

    if (deleteFile) {
        this->Delete();
    }

    if (rename) {
        this->RenamePartial();
    }

    s_downloadLock.Leave();
}

// VERIFY: 46bdcb82 auto
// OFFSET: 0x006BA650
void GruntFileDownload::Destroy(bool deleteFile) {
    this->Close(deleteFile, false);
}

// VERIFY: 3530cd68 auto
// OFFSET: 0x006BA6B0
bool GruntFileDownload::Initialize(const char* fileName, const uint8_t* md5, uint64_t size, uint64_t& offset) {
    this->Destroy(false);

    this->m_header.m_size = size;
    memcpy(this->m_header.m_md5, md5, sizeof(this->m_header.m_md5));

    bool result = true;
    offset = 0;

    SStrCopy(this->m_fileName, fileName, sizeof(this->m_fileName));

    char path[260];
    SStrPrintf(path, sizeof(path), "%s.partial", fileName);

    if (OsFileExists(path)) {
        Storm::IOFileParam param = { path, 0xC0000000, 0, 3, 0 };
        this->m_unit = Storm::IOFileUnit::Create(param, nullptr);

        if (this->m_unit) {
            bool complete;

            if (this->ValidateForContinue(complete)) {
                this->m_partial = true;

                if (complete) {
                    result = false;
                    this->m_status = 2;
                }

                offset = this->m_header.m_received;
                return result;
            }

            this->m_unit->Release();
            this->m_unit = nullptr;
        }
    }

    if (OsFileExists(this->m_fileName)) {
        bool done = false;
        Storm::IOFileParam param = { this->m_fileName, 0xC0000000, 0, 3, 0 };
        this->m_unit = Storm::IOFileUnit::Create(param, nullptr);

        if (this->m_unit) {
            if (this->VerifyFile()) {
                this->m_status = 2;
                offset = this->m_header.m_received;
                result = false;
                done = true;
            }

            this->m_unit->Release();
            this->m_unit = nullptr;

            if (done) {
                return result;
            }
        }
    }

    Storm::IOFileParam param = { path, 0xC0000000, 0, 2, 0 };
    this->m_unit = Storm::IOFileUnit::Create(param, nullptr);
    this->m_partial = true;

    if (!this->m_unit) {
        this->m_status = 3;
        this->m_reason = 1;
        return false;
    }

    if (!this->m_unit->SetFileSize(this->m_header.m_size + sizeof(Header))) {
        this->m_reason = 2;
        this->m_status = 3;
        this->m_reasonData = static_cast<uint32_t>((this->m_header.m_size + sizeof(Header)) >> 20) + 1;
        return false;
    }

    return result;
}

// VERIFY: 4c531141 manual
// OFFSET: 0x006BA930
void GruntFileDownload::CompleteDownload() {
    if (!this->m_unit) {
        return;
    }

    bool rename = false;

    if (this->m_unit->SetFileSize(this->m_header.m_size)) {
        rename = this->m_partial;
    } else {
        this->m_status = 3;
        this->m_reason = 1;
    }

    this->Close(false, rename);
}

// VERIFY: 60e97aba auto
// OFFSET: 0x006BA9D0
void GruntFileDownload::StopDownload() {
    s_downloadLock.Enter();

    this->m_status = 3;
    this->m_reason = 3;

    if (this->m_unit) {
        this->WriteHeader();
        this->Close(false, false);
    }

    s_downloadLock.Leave();
}

// VERIFY: 73e8deb2 auto
// OFFSET: [0x006BAD70, 0x006BAE50]
GruntFileDownload::~GruntFileDownload() {
    this->Close(false, false);
}

// VERIFY: aef01f8b auto
// OFFSET: 0x006BAEE0
int32_t GruntFileDownload::GetReason(uint32_t* data) {
    if (data) {
        *data = this->m_reasonData;
    }

    return this->m_reason;
}

// VERIFY: d5f8e470 auto
// OFFSET: 0x006BAF00
bool GruntFileDownload::Accepted() {
    return this->m_accepted;
}
