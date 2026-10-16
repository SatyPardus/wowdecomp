#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FILEDOWNLOAD_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FILEDOWNLOAD_H

#include <cstdint>
#include "Engine/Source/Storm/H/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

class HttpDownload;

namespace Storm {
class IOUnit;
}

class FileDownload {
    public:
    /* 0004 */ char m_fileName[260];
    /* 0108 */ bool m_partial;
    /* 010C */ int32_t m_status;
    /* 0110 */ int32_t m_reason;
    /* 0114 */ bool m_accepted;
    /* 0118 */ uint32_t m_bytesSinceSave;
    /* 011C */ uint32_t m_reasonData;

    virtual ~FileDownload();
    virtual bool Initialize(const char* fileName, const uint8_t* md5, uint64_t size, uint64_t& offset) = 0;
    virtual void Destroy(bool deleteFile) = 0;
    virtual int32_t GetStatus() = 0;
    virtual int32_t GetReason(uint32_t* data) = 0;
    virtual void Proceed(const uint8_t* data, uint32_t size) = 0;
    virtual float GetProgress() = 0;
    virtual void StopDownload() = 0;
    virtual void CompleteDownload() = 0;
    virtual bool Accepted() = 0;

    FileDownload();
};

class GruntFileDownload : public FileDownload {
    public:
    struct Header {
        /* 0000 */ uint32_t m_magic;
        /* 0004 */ uint32_t m_version;
        /* 0008 */ uint8_t m_md5[16];
        /* 0018 */ uint64_t m_received;
        /* 0020 */ uint64_t m_size;
    };

    static void STORMAPI CalculateMD5(Storm::IOUnit* unit, uint64_t offset, uint64_t size, uint8_t* digest);

    /* 0120 */ Storm::IOUnit* m_unit;
    /* 0124 */ uint32_t unused_0124;
    /* 0128 */ Header m_header;

    ~GruntFileDownload() override;
    bool Initialize(const char* fileName, const uint8_t* md5, uint64_t size, uint64_t& offset) override;
    void Destroy(bool deleteFile) override;
    int32_t GetStatus() override;
    int32_t GetReason(uint32_t* data) override;
    void Proceed(const uint8_t* data, uint32_t size) override;
    float GetProgress() override;
    void StopDownload() override;
    void CompleteDownload() override;
    bool Accepted() override;

    GruntFileDownload();
    void Close(bool deleteFile, bool rename);
    int32_t Delete();
    void RenamePartial();
    bool ValidateForContinue(bool& complete);
    bool VerifyFile();
    bool VerifyMD5(uint64_t size);
    void WriteHeader();
};

class BattlenetFileDownload : public FileDownload {
    public:
    /* 0120 */ int32_t m_numDownloads;
    /* 0124 */ TSGrowableArray<HttpDownload*> m_downloads;

    ~BattlenetFileDownload() override;
    bool Initialize(const char* fileName, const uint8_t* md5, uint64_t size, uint64_t& offset) override;
    void Destroy(bool deleteFile) override;
    int32_t GetStatus() override;
    int32_t GetReason(uint32_t* data) override;
    void Proceed(const uint8_t* data, uint32_t size) override;
    float GetProgress() override;
    void StopDownload() override;
    void CompleteDownload() override;
    bool Accepted() override;

    BattlenetFileDownload();
};

#endif
