#include "Engine/Source/Os/OsURLDownload.h"
#include "Engine/Source/BlizzardCore/Source/System/URL/URL.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
#include "WoW/Source/Ui/KnowledgeBase.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <functional>
#include <memory>
#include <thread>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Socket {
    int fd;
    explicit Socket(int value) : fd(value) { Require(fd >= 0, "Cannot create HTTP fixture socket"); }
    Socket(Socket&& other) : fd(std::exchange(other.fd, -1)) {}
    ~Socket() { if (fd >= 0) close(fd); }
};
void Ready(int fd, short events) {
    pollfd item{fd, events, 0};
    int status;
    do { status = poll(&item, 1, 5000); } while (status < 0 && errno == EINTR);
    Require(status > 0, "HTTP fixture socket timed out");
}
struct Server {
    Socket listener{socket(AF_INET, SOCK_STREAM, 0)};
    std::string root;
    Server() {
        sockaddr_in address{};
        address.sin_len = sizeof(address); address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(!bind(listener.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) && !listen(listener.fd, 8),
                "Cannot bind HTTP fixture listener");
        socklen_t size = sizeof(address);
        Require(!getsockname(listener.fd, reinterpret_cast<sockaddr*>(&address), &size), "Cannot query HTTP fixture port");
        root = "http://127.0.0.1:" + std::to_string(ntohs(address.sin_port));
    }
    Socket Accept(const char* path, const char* method = "GET", std::string* captured = nullptr) {
        Ready(listener.fd, POLLIN);
        Socket socket(accept(listener.fd, nullptr, nullptr));
        Require(WowSocketNonBlocking(socket.fd), "Cannot configure HTTP fixture peer");
        std::string request;
        while (request.find("\r\n\r\n") == std::string::npos) {
            Ready(socket.fd, POLLIN);
            char bytes[1024];
            const auto size = recv(socket.fd, bytes, sizeof(bytes), 0);
            if (size < 0 && (errno == EINTR || errno == EAGAIN)) continue;
            Require(size > 0 && request.size() + size <= 16384, "Invalid HTTP fixture request");
            request.append(bytes, size);
        }
        Require(request.find(std::string(method) + " " + path + " HTTP/1.1\r\n") == 0 &&
                    request.find("User-Agent: Blizzard Web Client\r\n") != std::string::npos,
                "Native HTTP request lost its path/query or original user agent");
        if (captured) *captured = request;
        return socket;
    }
};
void Send(const Socket& socket, const std::string& bytes) {
    size_t offset = 0;
    while (offset < bytes.size()) {
        Ready(socket.fd, POLLOUT);
        const auto count = send(socket.fd, bytes.data() + offset, bytes.size() - offset, 0);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        Require(count > 0, "HTTP fixture response write failed");
        offset += count;
    }
}
struct Result {
    std::mutex mutex;
    std::condition_variable changed;
    std::string bytes;
    unsigned completions = 0, chunks = 0, empty = 0;
    bool valid = true, stop = false;
    Result* next = nullptr;
    std::string nextURL;
    bool nextStarted = false;
    HTTPError::ErrorCode error = HTTPError::NoError;
    ~Result() { OsURLDownloadShutdown(); }
    static bool Callback(void* argument, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
        auto& result = *static_cast<Result*>(argument);
        std::lock_guard<std::mutex> lock(result.mutex);
        result.valid &= !result.completions && size <= 16384;
        if (done) {
            ++result.completions;
            result.error = error;
            result.valid &= !data && !size;
            if (result.next) result.nextStarted = OsURLDownload(result.nextURL.c_str(), Callback, result.next);
        } else if (size) {
            ++result.chunks;
            result.valid &= data && error == HTTPError::NoError;
            result.bytes.append(data, size);
        } else {
            ++result.empty;
        }
        result.changed.notify_all();
        return !result.stop;
    }
    void Wait() {
        std::unique_lock<std::mutex> lock(mutex);
        Require(changed.wait_for(lock, std::chrono::seconds(5), [&] { return completions != 0; }), "HTTP completion timed out");
        Require(valid && completions == 1, "HTTP chunk bounds or completion ordering changed");
    }
};
}

void CheckURLDownloads() {
    for (const char* url : {"", "file:///tmp/unused", "ftp://localhost/test", "http://", "bad-url"})
        Require(!OsURLDownload(url, nullptr, nullptr), "Unsupported/invalid download URL was accepted");
    Require(!OsURLDownload(nullptr, nullptr, nullptr), "Null download URL was accepted");
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        Server server;
        std::string payload(100003, '\0');
        for (size_t i = 0; i < payload.size(); ++i) payload[i] = char(i * 17 + cycle);
        {
            Result result;
            Require(OsURLDownload((server.root + "/redirect").c_str(), Result::Callback, &result), "Cannot start native HTTP download");
            {
                auto peer = server.Accept("/redirect");
                Send(peer, "HTTP/1.1 302 Found\r\nLocation: /bytes?language=zhCN&build=12340\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            }
            {
                auto peer = server.Accept("/bytes?language=zhCN&build=12340");
                Send(peer, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n");
                for (size_t offset = 0; offset < payload.size();) {
                    const size_t count = std::min<size_t>(19001, payload.size() - offset);
                    char prefix[32]; std::snprintf(prefix, sizeof(prefix), "%zx\r\n", count);
                    Send(peer, std::string(prefix) + payload.substr(offset, count) + "\r\n");
                    offset += count;
                }
                Send(peer, "0\r\n\r\n");
            }
            result.Wait();
            Require(result.error == HTTPError::NoError && result.bytes == payload && result.chunks >= 7 && result.empty == 1,
                    "Native chunked/redirected HTTP changed payload bytes or EOF delivery");
        }
        for (int code : {204, 404, 500}) {
            Result result;
            Require(OsURLDownload((server.root + "/status").c_str(), Result::Callback, &result), "Cannot start HTTP status check");
            auto peer = server.Accept("/status");
            Send(peer, "HTTP/1.1 " + std::to_string(code) + " Status\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            result.Wait();
            const auto expected = code == 204 ? HTTPError::NoError : code == 404 ? HTTPError::NotFound : HTTPError::Failed;
            Require(result.error == expected && result.bytes.empty(), "HTTP status did not preserve original error mapping");
        }
        {
            Result result; result.stop = true;
            Require(OsURLDownload((server.root + "/stop").c_str(), Result::Callback, &result), "Cannot start stoppable HTTP download");
            auto peer = server.Accept("/stop");
            Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 100003\r\n\r\n" + payload.substr(0, 20000));
            result.Wait();
            Require(result.error == HTTPError::NoError && result.chunks == 1 && !result.bytes.empty() &&
                        payload.compare(0, result.bytes.size(), result.bytes) == 0,
                    "HTTP callback stop lost bytes or emitted a false transfer failure");
        }
        {
            Result result;
            Require(OsURLDownload((server.root + "/truncated").c_str(), Result::Callback, &result), "Cannot start truncated HTTP download");
            {
                auto peer = server.Accept("/truncated");
                Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 100003\r\nConnection: close\r\n\r\n" + payload.substr(0, 100));
            }
            result.Wait();
            Require(result.error == HTTPError::Failed && result.bytes.size() <= 100,
                    "Truncated HTTP body was reported as a complete download");
        }
        {
            Result next, first;
            first.next = &next;
            first.nextURL = server.root + "/next";
            Require(OsURLDownload((server.root + "/first").c_str(), Result::Callback, &first), "Cannot start chained HTTP download");
            {
                auto peer = server.Accept("/first");
                Send(peer, "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");
            }
            first.Wait();
            Require(first.nextStarted, "HTTP completion could not start the next download");
            auto peer = server.Accept("/next");
            Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: close\r\n\r\nnext");
            next.Wait();
            Require(next.error == HTTPError::NoError && next.bytes == "next", "Chained HTTP request lost its callback owner");
        }
        {
            Result result;
            Require(OsUrlDownload::DownloadString((server.root + "/timeout").c_str(), Result::Callback, &result, 200),
                    "Cannot start HTTP timeout check");
            auto peer = server.Accept("/timeout");
            result.Wait();
            Require(result.error == HTTPError::Timeout, "Native HTTP timeout did not report Timeout");
        }
        {
            Result first, second;
            Require(OsURLDownload((server.root + "/shutdown1").c_str(), Result::Callback, &first), "Cannot start first pending HTTP request");
            auto peer1 = server.Accept("/shutdown1");
            Require(OsURLDownload((server.root + "/shutdown2").c_str(), Result::Callback, &second), "Cannot start second pending HTTP request");
            auto peer2 = server.Accept("/shutdown2");
            OsURLDownloadShutdown();
            Require(first.completions == 1 && second.completions == 1 && first.error == HTTPError::Failed && second.error == HTTPError::Failed,
                    "HTTP shutdown did not drain all cancellation callbacks before returning");
        }
    }
    std::fprintf(stderr, "Native HTTP: exact streamed bytes, query/redirect, EOF/status, truncation, chained callback, stop, timeout and shutdown/restart passed twice\n");
}

namespace {
struct BlizzardResult : Blizzard::URL::DownloadCallback {
    std::mutex mutex;
    std::condition_variable changed;
    std::string bytes, modified, order, post, nextURL;
    bool want = true, abort = false, stop = false, fail = false, headers = false;
    unsigned completions = 0, chunks = 0;
    bool valid = true, nextStarted = false;
    int32_t code = 0, timeout = 2000;
    int64_t length = -1;
    ByteRange range{-1, -1};
    Status status = DOWNLOAD_UNKNOWN_ERROR;
    BlizzardResult* next = nullptr;
    ~BlizzardResult() override { Blizzard::URL::CancelNativeDownloads(this); }
    bool WantData() override { return want; }
    int32_t GetTimeout() override { return timeout; }
    ByteRange GetByteRange() override { return range; }
    const char* GetPostData() override { return post.empty() ? nullptr : post.c_str(); }
    const char* GetPostContentType() override { return "text/plain; charset=utf-8"; }
    const char* WantIfModifiedSince() override { return headers ? "Wed, 21 Oct 2015 07:28:00 GMT" : nullptr; }
    Cookie GetCookie() override { return headers ? Cookie{"wow_native_probe", "12340"} : Cookie{nullptr, nullptr}; }
    void ConnectedOK() override { order += 'c'; }
    void GetHTTPStatusCode(int32_t value) override { code = value; order += 's'; }
    void GetLastModified(const char* value) override { modified = value; order += 'm'; }
    void GetContentLength(int64_t value) override { length = value; order += 'l'; }
    bool Stop() override { return stop; }
    bool ReceivedData(const void* data, int32_t size) override {
        if (fail) throw std::runtime_error("deliberate download callback failure");
        valid &= !completions && data && size > 0 && size <= 16384;
        bytes.append(static_cast<const char*>(data), size); ++chunks; order += 'd';
        return !abort;
    }
    void Completed(Status value) override {
        std::lock_guard<std::mutex> lock(mutex);
        status = value; ++completions; order += 'f';
        if (next) nextStarted = Blizzard::URL::DownloadURL(nextURL.c_str(), next);
        changed.notify_all();
    }
    void Wait() {
        std::unique_lock<std::mutex> lock(mutex);
        Require(changed.wait_for(lock, std::chrono::seconds(6), [&] { return completions != 0; }), "Blizzard URL completion timed out");
        Require(valid && completions == 1, "Blizzard URL delivered invalid chunks or duplicate completion");
    }
};
}

void CheckBlizzardURLDownloads() {
    using Callback = Blizzard::URL::DownloadCallback;
    using Blizzard::URL::DownloadURL;
    {
        BlizzardResult result;
        for (const char* url : {"", "file:///tmp/unused", "ftp://localhost/test", "http://", "bad-url"})
            Require(!DownloadURL(url, &result), "Blizzard URL accepted invalid/unsupported address");
        Require(!DownloadURL(nullptr, &result) && !DownloadURL("http://localhost/", nullptr) && !result.completions,
            "Invalid Blizzard URL start produced a completion or accepted null input");
    }
    for (unsigned cycle = 0; cycle != 2; ++cycle) {
        Server server;
        const std::string payload(100003, char('A' + cycle));
        {
            BlizzardResult result;
            Require(DownloadURL((server.root + "/redirect").c_str(), &result), "Cannot start Blizzard URL");
            {
                auto peer = server.Accept("/redirect");
                Send(peer, "HTTP/1.1 302 Found\r\nLocation: /bytes?locale=zhCN&build=12340\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            }
            {
                auto peer = server.Accept("/bytes?locale=zhCN&build=12340");
                Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 100003\r\nLast-Modified: Wed, 21 Oct 2015 07:28:00 GMT\r\nConnection: close\r\n\r\n" + payload);
            }
            result.Wait();
            Require(result.status == Callback::DOWNLOAD_SUCCESS && result.bytes == payload && result.length == 100003 &&
                result.chunks >= 7 && result.order.find("csml") == 0 && result.order.back() == 'f' && !result.modified.empty(),
                "Blizzard URL changed redirected bytes, callback order or response metadata");
        }
        {
            BlizzardResult result; result.want = false;
            Require(DownloadURL((server.root + "/head").c_str(), &result), "Cannot start Blizzard HEAD");
            auto peer = server.Accept("/head", "HEAD");
            Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 4294967313\r\nConnection: close\r\n\r\n");
            result.Wait();
            Require(result.status == Callback::DOWNLOAD_SUCCESS && result.length == INT64_C(4294967313) && !result.chunks,
                "Blizzard HEAD truncated a 64-bit length or delivered body data");
        }
        {
            BlizzardResult result; result.range = {INT64_C(4294967300), INT64_C(4294967303)};
            Require(DownloadURL((server.root + "/range").c_str(), &result), "Cannot start Blizzard range");
            std::string request;
            auto peer = server.Accept("/range", "GET", &request);
            Require(request.find("Range: bytes=4294967300-4294967303\r\n") != std::string::npos, "Blizzard range lost inclusive 64-bit offsets");
            Send(peer, "HTTP/1.1 206 Partial Content\r\nContent-Length: 4\r\nContent-Range: bytes 4294967300-4294967303/4294967400\r\nConnection: close\r\n\r\npart");
            result.Wait();
            Require(result.status == Callback::DOWNLOAD_SUCCESS && result.code == 206 && result.bytes == "part", "Blizzard range response changed");
        }
        {
            BlizzardResult result; result.post = "locale=zhCN&text=\xe4\xb8\xad\xe6\x96\x87"; result.headers = true;
            Require(DownloadURL((server.root + "/post?build=12340").c_str(), &result), "Cannot start Blizzard POST");
            std::string request;
            auto peer = server.Accept("/post?build=12340", "POST", &request);
            const size_t body = request.find("\r\n\r\n") + 4;
            while (request.size() - body < result.post.size()) {
                Ready(peer.fd, POLLIN); char buffer[1024]; const auto count = recv(peer.fd, buffer, sizeof(buffer), 0);
                if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
                Require(count > 0, "Blizzard POST body truncated"); request.append(buffer, count);
            }
            Require(request.substr(body) == result.post && request.find("Content-Type: text/plain; charset=utf-8\r\n") != std::string::npos &&
                request.find("If-Modified-Since: Wed, 21 Oct 2015 07:28:00 GMT\r\n") != std::string::npos &&
                request.find("wow_native_probe=12340") != std::string::npos, "Blizzard POST/header/cookie bytes changed");
            Send(peer, "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");
            result.Wait(); Require(result.status == Callback::DOWNLOAD_SUCCESS, "Blizzard POST failed");
        }
        for (int code : {304, 400, 403, 404, 500}) {
            BlizzardResult result;
            Require(DownloadURL((server.root + "/status").c_str(), &result), "Cannot start Blizzard status check");
            auto peer = server.Accept("/status");
            Send(peer, "HTTP/1.1 " + std::to_string(code) + " Status\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            result.Wait();
            const auto expected = code == 304 ? Callback::DOWNLOAD_NOT_MODIFIED : code == 400 ? Callback::DOWNLOAD_BAD_REQUEST :
                code == 403 ? Callback::DOWNLOAD_FORBIDDEN : code == 404 ? Callback::DOWNLOAD_NOT_FOUND : Callback::DOWNLOAD_FAILED;
            Require(result.status == expected && result.code == code && !result.chunks, "Blizzard URL changed HTTP failure mapping");
        }
        for (int choice : {0, 1, 2}) {
            BlizzardResult result; result.abort = choice == 0; result.stop = choice == 1; result.fail = choice == 2;
            Require(DownloadURL((server.root + "/stop").c_str(), &result), "Cannot start Blizzard stop check");
            auto peer = server.Accept("/stop");
            Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 100003\r\n\r\n" + payload.substr(0, 20000));
            result.Wait();
            Require(result.status == (choice == 0 ? Callback::DOWNLOAD_ABORTED : choice == 1 ? Callback::DOWNLOAD_STOPPED : Callback::DOWNLOAD_FAILED) &&
                result.chunks == (choice == 2 ? 0u : 1u), "Blizzard URL lost callback abort/stop/failure semantics");
        }
        {
            BlizzardResult result;
            Require(DownloadURL((server.root + "/truncated").c_str(), &result), "Cannot start Blizzard truncated download");
            { auto peer = server.Accept("/truncated"); Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 100003\r\nConnection: close\r\n\r\nshort"); }
            result.Wait(); Require(result.status == Callback::DOWNLOAD_FAILED, "Blizzard URL reported a truncated body as complete");
        }
        {
            BlizzardResult result; result.timeout = 200;
            Require(DownloadURL((server.root + "/timeout").c_str(), &result), "Cannot start Blizzard timeout");
            auto peer = server.Accept("/timeout"); result.Wait();
            Require(result.status == Callback::DOWNLOAD_TIMEOUT, "Blizzard URL lost timeout status");
        }
        {
            BlizzardResult next, first; first.next = &next; first.nextURL = server.root + "/next";
            Require(DownloadURL((server.root + "/first").c_str(), &first), "Cannot start chained Blizzard download");
            { auto peer = server.Accept("/first"); Send(peer, "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n"); }
            first.Wait(); Require(first.nextStarted, "Blizzard completion could not start a request");
            auto peer = server.Accept("/next"); Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: close\r\n\r\nnext");
            next.Wait(); Require(next.status == Callback::DOWNLOAD_SUCCESS && next.bytes == "next", "Chained Blizzard download failed");
        }
        {
            BlizzardResult first, second;
            Require(DownloadURL((server.root + "/cancel").c_str(), &first), "Cannot start scoped Blizzard cancellation");
            auto peer1 = server.Accept("/cancel");
            Require(DownloadURL((server.root + "/survive").c_str(), &second), "Cannot start concurrent Blizzard request");
            auto peer2 = server.Accept("/survive");
            Blizzard::URL::CancelNativeDownloads(&first); first.Wait();
            Send(peer2, "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n"); second.Wait();
            Require(first.status == Callback::DOWNLOAD_STOPPED && second.status == Callback::DOWNLOAD_SUCCESS,
                "Scoped Blizzard cancellation affected another request or returned before completion");
        }
        {
            BlizzardResult result;
            Require(DownloadURL((server.root + "/shutdown").c_str(), &result), "Cannot start Blizzard shutdown check");
            auto peer = server.Accept("/shutdown"); Blizzard::URL::ShutdownNativeDownloads(); result.Wait();
            Require(result.status == Callback::DOWNLOAD_STOPPED, "Blizzard shutdown lost its completion");
        }
    }
    std::fputs("Blizzard URL: original callback order, exact GET/POST/HEAD/range/UTF-8/cookie metadata, HTTP errors, truncation, abort/stop/timeout, chained requests and cancellation/shutdown passed twice\n", stderr);
}

void CheckKnowledgeBaseDownloads() {
    struct Page : CKBPage {
        unsigned successes = 0, failures = 0, computed = 0;
        std::thread::id owner = std::this_thread::get_id();
        std::function<void()> completed;
        void _ComputeCachedData() override {
            Require(std::this_thread::get_id() == owner, "Page parsed XML outside its game thread"); ++computed;
        }
        void _SendSuccessEvent() override {
            Require(std::this_thread::get_id() == owner && !m_inQueue, "Page event arrived on wrong thread or before queue removal");
            ++successes; auto action = completed; if (action) action();
        }
        void _SendFailureEvent() override {
            Require(std::this_thread::get_id() == owner && !m_inQueue, "Page failure arrived on wrong thread or before queue removal"); ++failures;
        }
    };
    auto pump = [](const auto& done) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!done()) {
            CKBPage::UpdateLoadingQueue();
            Require(std::chrono::steady_clock::now() < deadline, "Downloaded page did not complete");
            // Exercise FrameXML's same shared parser while workers receive data.
            auto* tree = XMLTree_Load("<Ui><Frame/></Ui>", 17);
            Require(tree, "FrameXML parse was corrupted by a concurrent download"); XMLTree_Free(tree);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };
    auto response = [](const Socket& peer, const std::string& data) {
        Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(data.size()) + "\r\nConnection: close\r\n\r\n" + data);
    };
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        Server server; Page first, second;
        const std::string xml = "<root number=\"42\" enabled=\"true\"><body>account &amp; support</body></root>";
        Require(first._StartNativeDownload((server.root + "/first").c_str()), "Cannot start native page");
        auto firstPeer = server.Accept("/first");
        Require(second._StartNativeDownload((server.root + "/second").c_str()), "Cannot start concurrent native page");
        auto secondPeer = server.Accept("/second");
        Send(firstPeer, "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(xml.size()) + "\r\nConnection: close\r\n\r\n" + xml.substr(0, 17));
        CKBPage::UpdateLoadingQueue(); Require(first.m_state == 1 && !first.m_tree, "Partial download published XML early");
        Send(firstPeer, xml.substr(17)); response(secondPeer, xml);
        pump([&] { return first.successes && second.successes; });
        CKBIterator root{}; first._GetRoot(root);
        Require(first.computed == 1 && second.computed == 1 && first._GetInt(root.m_node, "number") == 42 &&
            first._GetBool(root.m_node, "enabled") && std::string(first._GetBodyString(root.m_node->GetChildByName("body"))) == "account & support",
            "Downloaded original XML fields or body changed");
        for (unsigned state : {6u, 8u, 4u}) {
            Require(first._StartNativeDownload((server.root + "/failure").c_str()), "Cannot restart page");
            {
                auto peer = server.Accept("/failure");
                if (state == 6) Send(peer, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                else if (state == 8) response(peer, "<root>");
                else Send(peer, "HTTP/1.1 200 OK\r\nContent-Length: 100\r\nConnection: close\r\n\r\n<root>");
            }
            pump([&] { return !first.m_inQueue; });
            Require(first.m_state == state && !first.m_data, "Page failure state/partial-buffer cleanup changed");
        }
        Require(first._BeginLoading() && !CKBPage::LoadingCallback(&first, "x", 8 * 1024 * 1024 + 1, false, HTTPError::NoError), "Page byte budget failed");
        CKBPage::UpdateLoadingQueue(); Require(first.m_state == 1, "Rejected chunk published before its final callback");
        CKBPage::LoadingCallback(&first, nullptr, 0, true, HTTPError::NoError); CKBPage::UpdateLoadingQueue();
        Require(first.m_state == 7 && first.failures == 4, "Page budget error was overwritten by normal callback termination");
        Require(first._BeginLoading() && OsUrlDownload::DownloadString((server.root + "/timeout").c_str(), CKBPage::LoadingCallback, &first, 200), "Cannot start page timeout");
        auto timed = server.Accept("/timeout"); pump([&] { return !first.m_inQueue; }); Require(first.m_state == 5, "Page timeout state changed");
        Require(!first._StartNativeDownload("bad-url") && first.m_state == 3 && !first.m_inQueue, "Invalid page URL left a queued request");
        first.completed = [&] { first.completed = {}; Require(first._StartNativeDownload((server.root + "/reloaded").c_str()), "Event could not reload its page"); };
        Require(first._StartNativeDownload((server.root + "/retry").c_str()), "Page retry failed");
        auto retry = server.Accept("/retry"); response(retry, xml); pump([&] { return first.successes == 2; });
        auto reloaded = server.Accept("/reloaded"); response(reloaded, xml); pump([&] { return first.successes == 3; });

        auto deleted = std::make_unique<Page>();
        Require(deleted->_StartNativeDownload((server.root + "/delete").c_str()), "Cannot start deletion check");
        auto deletedPeer = server.Accept("/delete");
        Require(second._StartNativeDownload((server.root + "/survivor").c_str()), "Cannot start surviving page");
        auto survivor = server.Accept("/survivor"); deleted.reset();
        response(survivor, xml); pump([&] { return second.successes == 2; });
        deleted = std::make_unique<Page>(); deleted->completed = [&] { deleted.reset(); };
        Require(deleted->_StartNativeDownload((server.root + "/event-delete").c_str()), "Cannot start event deletion check");
        auto eventPeer = server.Accept("/event-delete"); response(eventPeer, xml); pump([&] { return !deleted; });

        CKBSetup setup;
        Require(setup._StartNativeDownload((server.root + "/setup").c_str()), "Cannot load original knowledge-base setup");
        auto setupPeer = server.Accept("/setup");
        response(setupPeer, "<root><Languages><language id=\"1\">English</language><language id=\"2\">Chinese</language></Languages><Categories><category id=\"7\"><SubCategories><category id=\"9\"/></SubCategories></category></Categories><ArticleHeaders><article id=\"11\"/><article id=\"12\"/></ArticleHeaders></root>");
        pump([&] { return !setup.m_inQueue; });
        Require(setup.m_state == 2 && setup.m_numLanguages == 2 && setup.m_numCategories == 1 && setup.m_numArticleHeaders == 2 &&
            setup._GetInt(setup.GetLanguage(1), "id") == 2 && setup._GetInt(setup.GetCategoryNode(0), "id") == 7 &&
            setup._GetInt(setup.GetSubCategory(0, 0), "id") == 9 && setup._GetInt(setup.GetArticleHeader(1), "id") == 12,
            "Original knowledge-base cached metadata or iterators changed");
        Require(second._StartNativeDownload((server.root + "/shutdown").c_str()), "Cannot start shutdown page");
        auto shutdown = server.Accept("/shutdown"); CKBPage::Shutdown();
        Require(second.m_state == 4 && !CKBPage::s_pLoadingQueue_Head, "Page shutdown did not cancel/drain callbacks and dispatch failure");
        OsURLDownloadShutdown();
    }
    std::fprintf(stderr, "Original web pages: concurrent downloads/game-thread XML, fields/iterators, HTTP/XML/size/timeout failure and retry, event reload/deletion, scoped cancellation and shutdown passed twice\n");
}
