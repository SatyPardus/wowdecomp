#include "Engine/Source/Os/OsClipboard.h"
#include "Engine/Source/Storm/H/storm.h"
#include <TargetConditionals.h>
#include <cstring>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

char* IOsClipboardGetString(void*) {
#if TARGET_OS_OSX
    NSString* text = [NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
#else
    NSString* text = UIPasteboard.generalPasteboard.string;
#endif
    const char* bytes = text.UTF8String;
    if (!bytes) return nullptr;
    const size_t size = std::strlen(bytes) + 1;
    auto* result = static_cast<char*>(SMemAlloc(size, __FILE__, __LINE__, 0));
    if (result) std::memcpy(result, bytes, size);
    return result;
}
void OsClipboardFreeString(char* text) { if (text) SMemFree(text, __FILE__, __LINE__, 0); }
int32_t IOsClipboardPutString(const char* bytes, void*) {
    if (!bytes) return 0;
    NSString* text = [[NSString alloc] initWithUTF8String:bytes];
    if (!text) return 0;
#if TARGET_OS_OSX
    [NSPasteboard.generalPasteboard clearContents];
    return [NSPasteboard.generalPasteboard writeObjects:@[text]];
#else
    UIPasteboard.generalPasteboard.string = text;
    return 1;
#endif
}
