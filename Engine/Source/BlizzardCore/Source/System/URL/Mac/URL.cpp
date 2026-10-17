#if defined(__APPLE__)

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <curl/curl.h>
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/URL/URL.h"

Blizzard::URL::HttpRequest::HttpRequest() {
    this->m_session = nullptr;
    this->m_connect = nullptr;
    this->m_request = nullptr;
    this->m_connectEvent = nullptr;
    this->m_requestEvent = nullptr;
    this->m_completeEvent = nullptr;
    this->m_dataEvent = nullptr;
    this->m_state = 0;
    this->m_done = false;
    this->m_connected = false;
    this->m_callback = nullptr;
    this->m_error = 0;
    this->m_status = Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS;
}

Blizzard::URL::HttpRequest::~HttpRequest() {}

namespace {

size_t WriteData(char* ptr, size_t size, size_t nmemb, void* userdata) {
    Blizzard::URL::DownloadCallback* callback = static_cast<Blizzard::URL::DownloadCallback*>(userdata);
    size_t bytes = size * nmemb;
    if (callback->Stop()) {
        return 0;
    }
    if (!callback->WantData()) {
        return bytes;
    }
    return callback->ReceivedData(ptr, static_cast<int32_t>(bytes)) ? bytes : 0;
}

size_t HeaderData(char* ptr, size_t size, size_t nmemb, void* userdata) {
    Blizzard::URL::DownloadCallback* callback = static_cast<Blizzard::URL::DownloadCallback*>(userdata);
    std::string header(ptr, size * nmemb);
    const char lastModified[] = "Last-Modified:";
    const char contentLength[] = "Content-Length:";
    if (strncasecmp(header.c_str(), lastModified, sizeof(lastModified) - 1) == 0) {
        std::string value = header.substr(sizeof(lastModified) - 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of("\r\n") + 1);
        callback->GetLastModified(value.c_str());
    } else if (strncasecmp(header.c_str(), contentLength, sizeof(contentLength) - 1) == 0) {
        callback->GetContentLength(strtoll(header.c_str() + sizeof(contentLength) - 1, nullptr, 10));
    }
    return size * nmemb;
}

Blizzard::URL::DownloadCallback::Status StatusFromHttp(long code, CURLcode result) {
    if (result == CURLE_OPERATION_TIMEDOUT) return Blizzard::URL::DownloadCallback::DOWNLOAD_TIMEOUT;
    if (result == CURLE_COULDNT_RESOLVE_HOST) return Blizzard::URL::DownloadCallback::DOWNLOAD_NAME_NOT_RESOLVED;
    if (result == CURLE_COULDNT_CONNECT) return Blizzard::URL::DownloadCallback::DOWNLOAD_CANNOT_CONNECT;
    if (result != CURLE_OK) return Blizzard::URL::DownloadCallback::DOWNLOAD_FAILED;
    switch (code) {
        case 304: return Blizzard::URL::DownloadCallback::DOWNLOAD_NOT_MODIFIED;
        case 400: return Blizzard::URL::DownloadCallback::DOWNLOAD_BAD_REQUEST;
        case 403: return Blizzard::URL::DownloadCallback::DOWNLOAD_FORBIDDEN;
        case 404: return Blizzard::URL::DownloadCallback::DOWNLOAD_NOT_FOUND;
        default: break;
    }
    if (code >= 200 && code < 300) return Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS;
    return Blizzard::URL::DownloadCallback::DOWNLOAD_FAILED;
}

} // namespace

bool Blizzard::URL::DownloadURL(const char* url, DownloadCallback* callback) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, callback->GetTimeout());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, callback->GetTimeout());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, callback);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderData);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, callback);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    struct curl_slist* headers = nullptr;
    const char* ifModifiedSince = callback->WantIfModifiedSince();
    if (ifModifiedSince) {
        std::string h = std::string("If-Modified-Since: ") + ifModifiedSince;
        headers = curl_slist_append(headers, h.c_str());
    }
    DownloadCallback::ByteRange range = callback->GetByteRange();
    if (range.m_start != -1 || range.m_end != -1) {
        char rangeBuf[128];
        if (range.m_start != -1 && range.m_end != -1) {
            Blizzard::String::Format(rangeBuf, sizeof(rangeBuf), "%lld-%lld", range.m_start, range.m_end);
        } else if (range.m_start != -1) {
            Blizzard::String::Format(rangeBuf, sizeof(rangeBuf), "%lld-", range.m_start);
        } else {
            Blizzard::String::Format(rangeBuf, sizeof(rangeBuf), "-%lld", range.m_end);
        }
        curl_easy_setopt(curl, CURLOPT_RANGE, rangeBuf);
    }
    DownloadCallback::Cookie cookie = callback->GetCookie();
    if (cookie.m_name && *cookie.m_name && cookie.m_value) {
        std::string c = std::string(cookie.m_name) + "=" + cookie.m_value;
        curl_easy_setopt(curl, CURLOPT_COOKIE, c.c_str());
    }
    const char* postData = callback->GetPostData();
    if (postData) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
        const char* contentType = callback->GetPostContentType();
        headers = curl_slist_append(headers, (std::string("Content-Type: ") + (contentType ? contentType : "text/html")).c_str());
    } else if (!callback->WantData()) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    callback->ConnectedOK();
    CURLcode result = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    callback->GetHTTPStatusCode(static_cast<int32_t>(code));
    DownloadCallback::Status status = callback->Stop() ? DownloadCallback::DOWNLOAD_STOPPED : StatusFromHttp(code, result);
    callback->Completed(status);
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return status == DownloadCallback::DOWNLOAD_SUCCESS || status == DownloadCallback::DOWNLOAD_NOT_MODIFIED;
}

#endif
