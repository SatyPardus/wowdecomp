#include "Engine/Source/Os/OsURLDownload.h"
#import <Foundation/Foundation.h>
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <mutex>

@interface WowURLDownload : NSObject <NSURLSessionDataDelegate> {
@public
    OsURLDownloadCallback callback;
    void* parameter;
    HTTPError::ErrorCode result;
    bool stoppedByCallback;
    NSURLSession* session;
    NSURLSessionDataTask* task;
}
@end

namespace {
struct Downloads {
    std::mutex mutex;
    std::condition_variable changed;
    NSMutableSet<WowURLDownload*>* active = [NSMutableSet new];
    bool stopping = false;
};
Downloads& State() {
    static Downloads state;
    return state;
}
bool Deliver(WowURLDownload* download, const char* data, uint32_t size, bool done) {
    if (!download->callback) return true;
    // As with WinINet, callbacks are serialized per request on a worker. The
    // byte view is valid only for this call; the client decides when to stop.
    try {
        return download->callback(download->parameter, data, size, done, download->result);
    } catch (...) {
        download->result = HTTPError::Failed;
        return false;
    }
}
}

@implementation WowURLDownload
- (void)URLSession:(NSURLSession*)urlSession dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveResponse:(NSURLResponse*)response completionHandler:(void (^)(NSURLSessionResponseDisposition))completion {
    const NSInteger status = [response isKindOfClass:[NSHTTPURLResponse class]] ? [(NSHTTPURLResponse*)response statusCode] : 0;
    if (status >= 200 && status < 300) {
        completion(NSURLSessionResponseAllow);
    } else {
        result = status == 404 ? HTTPError::NotFound : HTTPError::Failed;
        completion(NSURLSessionResponseCancel);
    }
}
- (void)URLSession:(NSURLSession*)urlSession dataTask:(NSURLSessionDataTask*)dataTask didReceiveData:(NSData*)data {
    const char* bytes = static_cast<const char*>(data.bytes);
    size_t remaining = data.length;
    while (remaining && !stoppedByCallback) {
        const auto count = static_cast<uint32_t>(std::min<size_t>(remaining, 16384));
        if (!Deliver(self, bytes, count, false)) {
            stoppedByCallback = true;
            [dataTask cancel];
            return;
        }
        bytes += count;
        remaining -= count;
    }
}
- (void)URLSession:(NSURLSession*)urlSession task:(NSURLSessionTask*)completedTask didCompleteWithError:(NSError*)error {
    if (error && result == HTTPError::NoError && !stoppedByCallback) {
        result = [error.domain isEqualToString:NSURLErrorDomain] && error.code == NSURLErrorTimedOut
            ? HTTPError::Timeout : HTTPError::Failed;
    }
    if (result == HTTPError::NoError && !stoppedByCallback) Deliver(self, nullptr, 0, false);
    Deliver(self, nullptr, 0, true);
    [urlSession finishTasksAndInvalidate];
}
- (void)URLSession:(NSURLSession*)urlSession didBecomeInvalidWithError:(NSError*)error {
    auto& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    session = nil;
    task = nil;
    [state.active removeObject:self];
    state.changed.notify_all();
}
@end

OsUrlDownload::OsUrlDownload()
    : m_internet(nullptr), m_connect(nullptr), m_request(nullptr), m_connectEvent(nullptr), m_requestEvent(nullptr),
      m_completeEvent(nullptr), m_dataEvent(nullptr), m_state(0), m_done(false), m_statusCode(0), m_error(0),
      m_errorCode(HTTPError::NoError), m_callback(nullptr), m_param(nullptr) {}

bool OsUrlDownload::DownloadString(const char* address, OsURLDownloadCallback callback, void* parameter, int32_t timeout) {
    @autoreleasepool {
        if (!address || std::strlen(address) > 16384) return false;
        NSString* text = [NSString stringWithUTF8String:address];
        NSURL* url = text ? [NSURL URLWithString:text] : nil;
        NSString* scheme = url.scheme.lowercaseString;
        if (!url.host.length || (![scheme isEqualToString:@"http"] && ![scheme isEqualToString:@"https"])) return false;
        auto& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.stopping) return false;
        WowURLDownload* download = [WowURLDownload new];
        download->callback = callback;
        download->parameter = parameter;
        download->result = HTTPError::NoError;
        NSURLSessionConfiguration* configuration = [NSURLSessionConfiguration ephemeralSessionConfiguration];
        configuration.URLCache = nil;
        configuration.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
        configuration.waitsForConnectivity = NO;
        if (timeout > 0) configuration.timeoutIntervalForRequest = double(timeout) / 1000;
        NSOperationQueue* queue = [NSOperationQueue new];
        queue.maxConcurrentOperationCount = 1;
        queue.name = @"WoW URL download";
        download->session = [NSURLSession sessionWithConfiguration:configuration delegate:download delegateQueue:queue];
        NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
        if (timeout > 0) request.timeoutInterval = double(timeout) / 1000;
        [request setValue:@"Blizzard Web Client" forHTTPHeaderField:@"User-Agent"];
        download->task = [download->session dataTaskWithRequest:request];
        [state.active addObject:download];
        [download->task resume];
        return true;
    }
}

bool OsURLDownload(const char* url, OsURLDownloadCallback callback, void* parameter) {
    return OsUrlDownload::DownloadString(url, callback, parameter, 5000);
}

void OsURLDownloadShutdown() {
    @autoreleasepool {
        auto& state = State();
        std::unique_lock<std::mutex> lock(state.mutex);
        state.stopping = true;
        // Cancellation is asynchronous; keep request owners alive until their
        // final callback has returned and NSURLSession releases its delegate.
        for (WowURLDownload* download in state.active) [download->task cancel];
        state.changed.wait(lock, [&] { return !state.active.count; });
        state.stopping = false;
    }
}

void OsURLDownloadCancel(OsURLDownloadCallback callback, void* parameter) {
    @autoreleasepool {
        auto& state = State();
        std::unique_lock<std::mutex> lock(state.mutex);
        for (WowURLDownload* download in state.active)
            if (download->callback == callback && download->parameter == parameter) [download->task cancel];
        state.changed.wait(lock, [&] {
            for (WowURLDownload* download in state.active)
                if (download->callback == callback && download->parameter == parameter) return false;
            return true;
        });
    }
}
