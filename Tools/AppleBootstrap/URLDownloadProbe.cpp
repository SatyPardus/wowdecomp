#include "Engine/Source/Os/OsURLDownload.h"
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
