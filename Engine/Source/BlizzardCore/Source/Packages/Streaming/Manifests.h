#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_STREAMING_MANIFESTS_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_STREAMING_MANIFESTS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Engine/Source/BlizzardCore/Source/System/Time/Types.h"
#include "Engine/Source/BlizzardCore/Source/System/URL/URL.h"
#include "Engine/Source/Storm/H/Types.h"

namespace Blizzard {
namespace Streaming {
namespace Manifests {

class ManifestItem {
    public:
    /* 0004 */ std::string m_name;

    virtual bool HandleTag(const char* tag, const char* value);
    virtual ~ManifestItem();
};

class TransportItem : public ManifestItem {
    public:
    /* 0020 */ int32_t m_md5Size;
    /* 0024 */ int32_t m_splitSize;

    bool HandleTag(const char* tag, const char* value) override;
};

class TimeslotItem : public ManifestItem {
    public:
    static Blizzard::Time::Timestamp ParseTimestamp(const char* time);

    /* 0020 */ Blizzard::Time::Timestamp m_startTime;
    /* 0028 */ Blizzard::Time::Timestamp m_endTime;
    /* 0030 */ int32_t m_usage;

    bool HandleTag(const char* tag, const char* value) override;
};

class ServerPathItem : public ManifestItem {
    public:
    /* 0020 */ std::string m_path;

    bool HandleTag(const char* tag, const char* value) override;
};

class ParameterItem : public ManifestItem {
    public:
    /* 0020 */ std::string m_value;

    bool HandleTag(const char* tag, const char* value) override;
    ~ParameterItem() override;

    ParameterItem();
};

class ServerItem : public ManifestItem {
    public:
    struct UsageSlot {
        /* 0000 */ Blizzard::Time::Timestamp m_time;
        /* 0008 */ int32_t m_usage;
    };

    /* 0020 */ std::string m_location;
    /* 003C */ std::vector<std::string> m_timeslotIds;
    /* 004C */ std::vector<UsageSlot> m_usageSlots;
    /* 005C */ int32_t m_uses;
    /* 0060 */ int32_t m_canDeactivate;
    /* 0064 */ int32_t m_maxRetry;
    /* 0068 */ bool m_active;

    bool HandleTag(const char* tag, const char* value) override;
    ~ServerItem() override;

    ServerItem();
    int32_t GetUsage(Blizzard::Time::Timestamp time);
    void InsertTimeslot(TimeslotItem* timeslot);
    void InsertUsageSlot(UsageSlot& slot, Blizzard::Time::Timestamp end);
};

class FileItem : public ManifestItem {
    public:
    /* 0020 */ std::string m_fileName;
    /* 003C */ std::string m_path;
    /* 0058 */ std::string m_localPath;
    /* 0074 */ std::string m_transportItem;
    /* 0090 */ int64_t m_size;
    /* 0098 */ TransportItem* m_transport;
    /* 009C */ int32_t m_flags;
    /* 00A0 */ std::string m_fileVersion;
    /* 00BC */ std::vector<std::string> m_validServers;

    bool HandleTag(const char* tag, const char* value) override;
    ~FileItem() override;

    FileItem();
    std::string GetRemotePath();
};

}

class ManifestInfo {
    public:
    static ManifestInfo* s_instance;

    static ManifestInfo* Get();
    static bool STORMAPI GetItemTag(char* line, char** value, char** tag, int32_t* length);
    static void DeactivateServer(std::string url, const std::string& name);
    static Manifests::FileItem* GetFileItem(const std::string& name);
    static std::string GetParameter(std::string name);
    static std::string GetRemotePath(const std::string& name);
    static int32_t GetServerItemMaxRetry(std::string url, const std::string& name);
    static Manifests::TransportItem* GetTransportItem(const std::string& name);
    static std::vector<std::string> GetFileList(const char* path);
    static bool Initialize(const char* path);
    static void SetFileItem(std::string name, const Manifests::FileItem& item);
    static void SetParameter(std::string name, int32_t value);
    static void SetParameter(std::string name, std::string value);
    static void SetServerItem(std::string name, const Manifests::ServerItem& item);
    static void SetServerPathItem(std::string name, std::string path);
    static void SetTransportItem(std::string name, const Manifests::TransportItem& item);

    /* 0000 */ int32_t m_version;
    /* 0004 */ std::map<std::string, Manifests::ServerItem> m_servers;
    /* 0010 */ std::map<std::string, Manifests::TimeslotItem> m_timeslots;
    /* 001C */ std::map<std::string, Manifests::ServerPathItem> m_serverPaths;
    /* 0028 */ std::map<std::string, Manifests::ParameterItem> m_parameters;
    /* 0034 */ std::map<std::string, Manifests::TransportItem> m_transports;
    /* 0040 */ std::map<std::string, Manifests::FileItem> m_files;
    /* 004C */ uint32_t unused_004C;

    ManifestInfo();
    ~ManifestInfo();
    bool FinalizeManifestItems();
    std::string GetRandomServerLocation(const std::vector<std::string>& validServers);
    Manifests::ManifestItem* HandleBaseTag(const char* tag, const char* value);
    bool ReadDirectFile(const char* path);
    bool ReadDirectFileFromBuffer(char* buffer, int32_t size);
    bool ReadIndirectFile(const char* url);
};

class FileManifest {
    public:
    struct MANIFEST_ITEM {
        /* 0000 */ std::string m_path;
        /* 001C */ std::string m_absolutePath;
        /* 0038 */ std::string m_fileVersion;
        /* 0058 */ int64_t m_size;
        /* 0060 */ int32_t m_flags;
    };

    static int32_t GetBGPreloadSleep();
    static std::vector<std::string> GetFileList(const char* path);
    static bool GetIsStreamingTrial();
    static bool ReadDirectFile(const char* path);
    static bool ReadDirectFileFromBuffer(char* buffer, int32_t size);
    static bool ReadIndirectFile(const char* url);
    static void SetDefaultBgPreloadSleep(int32_t sleep);
    static bool SetSpeedTest(int32_t speedTest);
    static bool SplitItem(char* line, MANIFEST_ITEM* item);
    static bool SplitServerItem(char* line, std::string& base, std::string& path);
};

class SourceManifest {
    public:
    struct MANIFEST_ITEM {
        /* 0000 */ std::string m_name;
        /* 001C */ std::string m_path;
    };

    static bool Initialize(const char* path);
    static bool ReadDirectFile(const char* path);
    static bool ReadDirectFileFromBuffer(char* buffer, int32_t size);
    static bool ReadIndirectFile(const char* url);
    static bool SplitItem(char* line, MANIFEST_ITEM* item);
};

struct URL_INFO {
    /* 0000 */ std::string m_url;
    /* 0020 */ int64_t m_start;
    /* 0028 */ int64_t m_end;
    /* 0030 */ int32_t m_size;

    URL_INFO(std::string url, int64_t start, int64_t end, int32_t size);
};

class TransportHttpCallback : public Blizzard::URL::DownloadCallback {
    public:
    /* 0004 */ std::string m_cookieName;
    /* 0020 */ std::string m_cookieValue;
    /* 003C */ std::string m_data;
    /* 0058 */ int64_t m_received;
    /* 0060 */ int64_t m_rangeStart;
    /* 0068 */ int64_t m_rangeEnd;
    /* 0070 */ uint32_t m_timeout;
    /* 0074 */ void* m_event;
    /* 0078 */ bool m_stop;
    /* 007C */ Blizzard::URL::DownloadCallback::Status m_status;
    /* 0080 */ bool m_success;

    ~TransportHttpCallback() override;
    int32_t GetTimeout() override;
    Blizzard::URL::DownloadCallback::ByteRange GetByteRange() override;
    bool Stop() override;
    bool ShouldBlock() override;
    Blizzard::URL::DownloadCallback::Cookie GetCookie() override;
    bool ReceivedData(const void* data, int32_t size) override;
    void Completed(Blizzard::URL::DownloadCallback::Status status) override;
    virtual void SetByteRange(int64_t start, int64_t end);
    virtual void SetCookie(const char* name, const char* value);
    virtual void SetTimeOutValue(uint32_t timeout);

    TransportHttpCallback(uint32_t size);
};

class Transport {
    public:
    struct THROTTLE {
        /* 0000 */ Blizzard::Time::Timestamp m_start;
        /* 0008 */ int64_t m_bytes;

        void Wait();
    };

    struct TRANSPORT_ITEM {
        /* 0000 */ std::string m_name;
        /* 001C */ int32_t m_md5Size;
        /* 0020 */ int32_t m_splitSize;
        /* 0024 */ std::vector<std::string> m_servers;
    };

    static int32_t AdjustRange(int64_t* start, int64_t* end, int32_t md5Size, int64_t fileSize);
    static void GetSplitFileURL(const std::string& url, int64_t start, int64_t end, int32_t skip, int32_t splitSize, int32_t md5Size, int32_t size, std::vector<URL_INFO>* urls);
    static bool MD5_Data(std::string* out, std::string* data, int32_t md5Size, int64_t start, int64_t end);
    static bool Read(Manifests::TransportItem* transport, std::string url, int32_t maxRetry, void* data, int64_t fileSize, int64_t offset, int32_t size);
    static bool ReadDirectFile(const char* path);
    static bool ReadDirectFileFromBuffer(char* buffer, int32_t size);
    static bool ReadFile(Manifests::TransportItem* transport, std::string url, int32_t maxRetry, void* data, int64_t fileSize, int64_t offset, int32_t size);
    static bool ReadHttp(Manifests::TransportItem* transport, std::string url, int32_t maxRetry, void* data, int64_t fileSize, int64_t offset, int32_t size);
    static bool ReadIndirectFile(const char* url);
    static bool SplitItem(char* line, TRANSPORT_ITEM* item);
};

extern const char* s_httpPrefix;
extern const char* s_filePrefix;

bool SplitLine(char* line, char** value, char** tag, int32_t* length);
char* STORMAPI TrimLine(char* line);

}
}

namespace System_Streaming {

class HttpCallback : public Blizzard::URL::DownloadCallback {
    public:
    /* 0004 */ std::string m_data;
    /* 0020 */ uint32_t m_received;
    /* 0024 */ Blizzard::URL::DownloadCallback::Status m_status;
    /* 0028 */ uint32_t m_timeout;
    /* 002C */ void* m_event;

    ~HttpCallback() override;
    int32_t GetTimeout() override;
    bool ShouldBlock() override;
    bool ReceivedData(const void* data, int32_t size) override;
    void Completed(Blizzard::URL::DownloadCallback::Status status) override;
    virtual void SetTimeOutValue(uint32_t timeout);

    HttpCallback();
};

}

#endif
