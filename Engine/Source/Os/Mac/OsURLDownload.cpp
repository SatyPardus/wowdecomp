#if defined(__APPLE__)

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <curl/curl.h>
#include "Engine/Source/Os/OsURLDownload.h"

struct CurlDownloadState {
    OsURLDownloadCallback callback;
    void* param;
    std::atomic<bool> cancelled;
};

static size_t CurlWrite(char* ptr, size_t size, size_t nmemb, void* userdata) {
    CurlDownloadState* state = static_cast<CurlDownloadState*>(userdata);
    uint32_t bytes = static_cast<uint32_t>(size * nmemb);
    if (!bytes) {
        return 0;
    }
    if (state->callback && !state->callback(state->param, ptr, bytes, false, HTTPError::NoError)) {
        state->cancelled = true;
        return 0;
    }
    return bytes;
}

OsUrlDownload::OsUrlDownload() {
    this->m_internet = nullptr;
    this->m_connect = nullptr;
    this->m_request = nullptr;
    this->m_connectEvent = nullptr;
    this->m_requestEvent = nullptr;
    this->m_completeEvent = nullptr;
    this->m_dataEvent = nullptr;
    this->m_state = 0;
    this->m_done = false;
    this->m_statusCode = 0;
    this->m_error = 0;
    this->m_errorCode = HTTPError::NoError;
    this->m_callback = nullptr;
    this->m_param = nullptr;
}

static HTTPError::ErrorCode MapCurlError(CURLcode code, long status) {
    if (code == CURLE_OPERATION_TIMEDOUT) {
        return HTTPError::Timeout;
    }
    if (status == 404) {
        return HTTPError::NotFound;
    }
    if (code == CURLE_OK && status >= 200 && status < 300) {
        return HTTPError::NoError;
    }
    return HTTPError::Failed;
}

bool OsUrlDownload::DownloadString(const char* url, OsURLDownloadCallback callback, void* param, int32_t timeout) {
    if (!url || !*url) {
        return false;
    }

    std::string urlCopy(url);
    std::thread([urlCopy, callback, param, timeout]() {
        CurlDownloadState state = {callback, param, false};
        CURL* curl = curl_easy_init();
        HTTPError::ErrorCode error = HTTPError::Failed;
        long status = 0;

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, urlCopy.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Blizzard Web Client");
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            if (timeout > 0) {
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
            }
            CURLcode code = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
            error = state.cancelled ? HTTPError::NoError : MapCurlError(code, status);
            curl_easy_cleanup(curl);
        }

        if (callback) {
            callback(param, nullptr, 0, true, error);
        }
    }).detach();

    return true;
}

bool OsURLDownload(const char* url, OsURLDownloadCallback callback, void* param) {
    return OsUrlDownload::DownloadString(url, callback, param, 30000);
}

#endif
