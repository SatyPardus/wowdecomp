#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_HTTPDOWNLOAD_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_HTTPDOWNLOAD_H

#include <cstdint>
#include <string>
#if defined(__APPLE__)
#include <mutex>
#endif
#include "Engine/Source/BlizzardCore/Source/System/URL/URL.h"

struct HttpDownloadState {
    /* 0000 */ volatile bool m_receiving;
    /* 0004 */ std::string m_targetName;
    /* 0020 */ std::string m_url;
    /* 003C */ bool m_done;
    /* 0040 */ int64_t m_size;
    /* 0048 */ uint8_t* m_data;
    /* 004C */ int32_t m_capacity;
    /* 0050 */ int32_t m_received;
#if defined(__APPLE__)
    std::mutex m_nativeMutex;
    Blizzard::URL::DownloadCallback::Status m_nativeStatus = Blizzard::URL::DownloadCallback::DOWNLOAD_UNKNOWN_ERROR;
#endif
};

class DownloadFileCallback : public Blizzard::URL::DownloadCallback {
    public:
    /* 0004 */ bool m_stop;
    /* 0008 */ HttpDownloadState* m_state;

    bool WantData() override;
    void GetContentLength(int64_t length) override;
    bool Stop() override;
    bool ShouldBlock() override;
    bool ReceivedData(const void* data, int32_t size) override;
    void Completed(Blizzard::URL::DownloadCallback::Status status) override;

    DownloadFileCallback();
};

class HttpDownload {
    public:
    /* 0000 */ DownloadFileCallback* m_callback;
    /* 0004 */ HttpDownloadState* m_state;
    /* 0008 */ bool m_requested;

    HttpDownload(const char* url, const char* targetName);
    ~HttpDownload();
    bool Cancel();
    bool Done();
#if defined(__APPLE__)
    bool Succeeded();
#endif
    uint8_t* GetBytes();
    int64_t Progress();
    int64_t Size();
    const char* TargetName();
};

#endif
