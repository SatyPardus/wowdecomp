#if defined(__APPLE__)
#include "Engine/Source/BlizzardCore/Source/System/URL/URL.h"
#import <Foundation/Foundation.h>
#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>

using Callback = Blizzard::URL::DownloadCallback;

@interface BlizzardURLRequest : NSObject <NSURLSessionDataDelegate> {
@public
    Callback* owner;
    Callback::Status status;
    bool decided;
    NSURLSession* session;
    NSURLSessionDataTask* task;
}
@end

namespace {
struct Downloads {
    std::mutex mutex;
    std::condition_variable changed;
    NSMutableSet<BlizzardURLRequest*>* active = [NSMutableSet new];
    NSHTTPCookieStorage* cookies = NSURLSessionConfiguration.ephemeralSessionConfiguration.HTTPCookieStorage;
    bool stopping = false;
};
Downloads& State() { static Downloads state; return state; }
thread_local bool inCallback;
struct CallbackScope {
    bool previous = inCallback;
    CallbackScope() { inCallback = true; }
    ~CallbackScope() { inCallback = previous; }
};
Callback::Status HTTPStatus(NSInteger code) {
    if (code >= 200 && code < 300) return Callback::DOWNLOAD_SUCCESS;
    switch (code) {
        case 304: return Callback::DOWNLOAD_NOT_MODIFIED;
        case 400: return Callback::DOWNLOAD_BAD_REQUEST;
        case 403: return Callback::DOWNLOAD_FORBIDDEN;
        case 404: return Callback::DOWNLOAD_NOT_FOUND;
        default: return Callback::DOWNLOAD_FAILED;
    }
}
Callback::Status ErrorStatus(NSError* error) {
    if (![error.domain isEqualToString:NSURLErrorDomain]) return Callback::DOWNLOAD_FAILED;
    switch (error.code) {
        case NSURLErrorTimedOut: return Callback::DOWNLOAD_TIMEOUT;
        case NSURLErrorCannotFindHost:
        case NSURLErrorDNSLookupFailed: return Callback::DOWNLOAD_NAME_NOT_RESOLVED;
        case NSURLErrorCannotConnectToHost: return Callback::DOWNLOAD_CANNOT_CONNECT;
        case NSURLErrorCancelled: return Callback::DOWNLOAD_STOPPED;
        default: return Callback::DOWNLOAD_FAILED;
    }
}
bool SetHeader(NSMutableURLRequest* request, NSString* name, const char* value) {
    if (!value) return true;
    if (strnlen(value, 16384) == 16384 || std::strpbrk(value, "\r\n")) return false;
    NSString* text = [NSString stringWithUTF8String:value];
    if (!text) return false;
    [request setValue:text forHTTPHeaderField:name];
    return true;
}
}

@implementation BlizzardURLRequest
- (void)URLSession:(NSURLSession*)urlSession dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveResponse:(NSURLResponse*)response completionHandler:(void (^)(NSURLSessionResponseDisposition))completion {
    CallbackScope scope;
    try {
        NSHTTPURLResponse* http = [response isKindOfClass:NSHTTPURLResponse.class] ? (NSHTTPURLResponse*)response : nil;
        owner->ConnectedOK();
        owner->GetHTTPStatusCode(int32_t(http.statusCode));
        status = HTTPStatus(http.statusCode);
        if (status != Callback::DOWNLOAD_SUCCESS) {
            decided = true;
            completion(NSURLSessionResponseCancel);
            return;
        }
        NSString* modified = [http valueForHTTPHeaderField:@"Last-Modified"];
        if (modified) owner->GetLastModified(modified.UTF8String);
        // Use 64-bit metadata; the public callback and streaming offsets are
        // 64-bit, even though the reconstructed WinINet wrapper narrows atol.
        if (response.expectedContentLength >= 0) owner->GetContentLength(response.expectedContentLength);
        if (!owner->WantData()) {
            decided = true;
            completion(NSURLSessionResponseCancel);
            return;
        }
        completion(NSURLSessionResponseAllow);
    } catch (...) {
        status = Callback::DOWNLOAD_FAILED; decided = true;
        completion(NSURLSessionResponseCancel);
    }
}
- (void)URLSession:(NSURLSession*)urlSession dataTask:(NSURLSessionDataTask*)dataTask didReceiveData:(NSData*)data {
    if (decided) return;
    CallbackScope scope;
    try {
        const auto* bytes = static_cast<const uint8_t*>(data.bytes);
        size_t remaining = data.length;
        while (remaining) {
            const int32_t count = int32_t(std::min<size_t>(remaining, 16384));
            const bool accepted = owner->ReceivedData(bytes, count);
            const bool stopped = owner->Stop();
            if (!accepted || stopped) {
                status = stopped ? Callback::DOWNLOAD_STOPPED : Callback::DOWNLOAD_ABORTED;
                decided = true; [dataTask cancel]; return;
            }
            bytes += count; remaining -= count;
        }
    } catch (...) {
        status = Callback::DOWNLOAD_FAILED; decided = true; [dataTask cancel];
    }
}
- (void)URLSession:(NSURLSession*)urlSession task:(NSURLSessionTask*)completedTask didCompleteWithError:(NSError*)error {
    if (error && !decided) status = ErrorStatus(error);
    // Completed may delete owner or start another download. No registry lock
    // is held and the callback is never dereferenced after this call.
    { CallbackScope scope;
      try { owner->Completed(status); }
      catch (...) { std::fputs("Blizzard URL completion threw an exception\n", stderr); }
    }
    [urlSession finishTasksAndInvalidate];
}
- (void)URLSession:(NSURLSession*)urlSession didBecomeInvalidWithError:(NSError*)error {
    auto& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    session = nil; task = nil;
    [state.active removeObject:self];
    state.changed.notify_all();
}
@end

bool Blizzard::URL::DownloadURL(const char* address, DownloadCallback* callback) {
    @autoreleasepool {
        if (!address || !callback || strnlen(address, 16384) == 16384) return false;
        NSString* text = [NSString stringWithUTF8String:address];
        NSURL* url = text ? [NSURL URLWithString:text] : nil;
        NSString* scheme = url.scheme.lowercaseString;
        if (!url.host.length || (![scheme isEqualToString:@"http"] && ![scheme isEqualToString:@"https"])) return false;
        NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
        const char* post = callback->GetPostData();
        request.HTTPMethod = post ? @"POST" : callback->WantData() ? @"GET" : @"HEAD";
        if (post) {
            const char* type = callback->GetPostContentType();
            if (!SetHeader(request, @"Content-Type", type ? type : "text/html")) return false;
            request.HTTPBody = [NSData dataWithBytes:post length:std::strlen(post)];
        }
        if (!SetHeader(request, @"If-Modified-Since", callback->WantIfModifiedSince())) return false;
        const auto range = callback->GetByteRange();
        if (range.m_start != -1 || range.m_end != -1) {
            // Preserve the original request's inclusive range and signed fields.
            [request setValue:[NSString stringWithFormat:@"bytes=%lld-%lld", (long long)range.m_start, (long long)range.m_end]
                forHTTPHeaderField:@"Range"];
        }
        NSURLSessionConfiguration* configuration = NSURLSessionConfiguration.ephemeralSessionConfiguration;
        configuration.URLCache = nil;
        configuration.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
        configuration.HTTPAdditionalHeaders = @{@"User-Agent": @"Blizzard Web Client", @"Accept-Encoding": @"identity"};
        const int32_t milliseconds = callback->GetTimeout();
        if (milliseconds <= 0) return false;
        configuration.timeoutIntervalForRequest = milliseconds / 1000.0;
        // GetTimeout is a connection/receive inactivity timeout, not a limit
        // on the duration of a large transfer that continues to make progress.
        configuration.HTTPCookieStorage = State().cookies;
        const auto cookie = callback->GetCookie();
        if (cookie.m_name && *cookie.m_name && cookie.m_value) {
            NSString* name = [NSString stringWithUTF8String:cookie.m_name];
            NSString* value = [NSString stringWithUTF8String:cookie.m_value];
            if (!name || !value) return false;
            NSHTTPCookie* nativeCookie = [NSHTTPCookie cookieWithProperties:@{NSHTTPCookieName: name,
                NSHTTPCookieValue: value, NSHTTPCookieDomain: url.host, NSHTTPCookiePath: @"/"}];
            if (!nativeCookie) return false;
            [configuration.HTTPCookieStorage setCookie:nativeCookie];
        }
        auto& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.stopping) return false;
        BlizzardURLRequest* download = [BlizzardURLRequest new];
        download->owner = callback; download->status = Callback::DOWNLOAD_SUCCESS;
        // The original entry is asynchronous; ShouldBlock is not called by its
        // WinINet implementation. A serial delegate queue preserves byte order.
        NSOperationQueue* queue = [NSOperationQueue new]; queue.maxConcurrentOperationCount = 1;
        download->session = [NSURLSession sessionWithConfiguration:configuration delegate:download delegateQueue:queue];
        download->task = [download->session dataTaskWithRequest:request];
        if (!download->task) { [download->session invalidateAndCancel]; return false; }
        [state.active addObject:download];
        [download->task resume];
        return true;
    }
}

void Blizzard::URL::CancelNativeDownloads(DownloadCallback* callback, bool wait) {
    if (inCallback) throw std::logic_error("Cancel native downloads outside their callbacks");
    @autoreleasepool {
        auto& state = State(); std::unique_lock<std::mutex> lock(state.mutex);
        for (BlizzardURLRequest* item in state.active) if (item->owner == callback) [item->task cancel];
        if (!wait) return;
        state.changed.wait(lock, [&] {
            for (BlizzardURLRequest* item in state.active) if (item->owner == callback) return false;
            return true;
        });
    }
}
void Blizzard::URL::ShutdownNativeDownloads() {
    if (inCallback) throw std::logic_error("Shut down native downloads outside their callbacks");
    @autoreleasepool {
        auto& state = State(); std::unique_lock<std::mutex> lock(state.mutex);
        state.stopping = true;
        for (BlizzardURLRequest* item in state.active) [item->task cancel];
        state.changed.wait(lock, [&] { return !state.active.count; });
        state.stopping = false;
    }
}
#endif
