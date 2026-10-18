#include "WoW/Source/Client.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#import <Foundation/Foundation.h>
#include <cstdlib>
#include <cstring>

bool GetCacheDirectory(char* path) {
    // The installation and its archives can be read-only. Cache placement is
    // independent of both the installation and the process working directory.
    path[0] = '\0';
    @autoreleasepool {
        NSString* root = nil;
        const char* override = std::getenv("WOW_CACHE_ROOT");
        if (override) {
            root = [NSString stringWithUTF8String:override];
            if (!root || !root.isAbsolutePath) {
                DebugLog("WOW_CACHE_ROOT must be an absolute UTF-8 path\n");
                return false;
            }
        } else {
            NSURL* url = [NSFileManager.defaultManager URLForDirectory:NSCachesDirectory inDomain:NSUserDomainMask
                                                    appropriateForURL:nil create:NO error:nil];
            root = [url.path stringByAppendingPathComponent:@"wowdecomp"];
        }
        if (!root) {
            DebugLog("Cannot locate the native application cache directory\n");
            return false;
        }
        // The mounted ClientData selects this locale. Do not let startup before
        // resource selection mix records into an unpartitioned cache.
        bool validLocale = s_localeCode[4] == '\0';
        for (unsigned i = 0; i < 4; ++i) {
            const char c = s_localeCode[i];
            validLocale &= (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }
        if (!validLocale) {
            DebugLog("Cannot select a cache directory before the resource locale\n");
            return false;
        }
        NSString* directory = [[root.stringByStandardizingPath stringByAppendingPathComponent:@"WDB"]
                              stringByAppendingPathComponent:[NSString stringWithUTF8String:s_localeCode]];
        const char* nativePath = directory.fileSystemRepresentation;
        if (!nativePath || std::strlen(nativePath) >= 260) {
            DebugLog("Native cache directory exceeds the client path capacity\n");
            return false;
        }
        NSError* error = nil;
        if (![NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:&error]) {
            DebugLog("Cannot create native cache directory %s: %s\n", nativePath, error.localizedDescription.UTF8String);
            return false;
        }
        SStrCopy(path, nativePath, 260);
        return true;
    }
}
