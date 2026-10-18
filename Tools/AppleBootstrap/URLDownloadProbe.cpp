#include "Engine/Source/Os/OsURLDownload.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
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
    Socket Accept(const char* path) {
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
        Require(request.find(std::string("GET ") + path + " HTTP/1.1\r\n") == 0 &&
                    request.find("User-Agent: Blizzard Web Client\r\n") != std::string::npos,
                "Native HTTP request lost its path/query or original user agent");
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
