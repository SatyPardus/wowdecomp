#include "ClientSettings.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#import <Foundation/Foundation.h>
#include <cstdlib>
#include <cstring>

namespace {
bool ValidPathComponent(const char* value) {
    return value && *value && !std::strchr(value, '/') && !std::strchr(value, '\\') &&
        std::strcmp(value, ".") && std::strcmp(value, "..");
}
}
bool NativeSettingsFile(std::string& path, const char* name, bool createParents,
                        const char* account, const char* realm, const char* character) {
    path.clear();
    if (!ValidPathComponent(name)) return false;
    if ((account && !ValidPathComponent(account)) || (realm && !ValidPathComponent(realm)) || (character && !ValidPathComponent(character)) ||
        (bool(realm) != bool(character)) || (realm && !account)) return false;
    @autoreleasepool {
        NSString* root = nil;
        if (const char* override = std::getenv("WOW_SETTINGS_ROOT")) {
            root = [NSString stringWithUTF8String:override];
            if (!root || !root.isAbsolutePath || std::strchr(override, '\\')) {
                DebugLog("WOW_SETTINGS_ROOT must be an absolute native UTF-8 path\n");
                return false;
            }
        } else {
            NSURL* url = [NSFileManager.defaultManager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask
                                                    appropriateForURL:nil create:NO error:nil];
            root = [url.path stringByAppendingPathComponent:@"wowdecomp"];
        }
        if (!root) return false;
        root = root.stringByStandardizingPath;
        if (account) root = [root stringByAppendingPathComponent:@"Account"];
        for (const char* value : {account, realm, character}) {
            if (!value) continue;
            NSString* component = [NSString stringWithUTF8String:value];
            if (!component) return false;
            root = [root stringByAppendingPathComponent:component];
        }
        NSString* file = [NSString stringWithUTF8String:name];
        if (!file) return false;
        NSString* full = [root stringByAppendingPathComponent:file];
        const char* nativePath = full.fileSystemRepresentation;
        if (!nativePath || std::strlen(nativePath) >= 260) {
            DebugLog("Native settings file exceeds the client path capacity\n");
            return false;
        }
        NSError* error = nil;
        if (createParents && ![NSFileManager.defaultManager createDirectoryAtPath:root withIntermediateDirectories:YES attributes:nil error:&error]) {
            DebugLog("Cannot create native settings directory: %s\n", error.localizedDescription.UTF8String);
            return false;
        }
        path = nativePath;
        return true;
    }
}

bool NativeSettingsReplace(const std::string& path, const void* bytes, size_t size) {
    @autoreleasepool {
        NSString* file = [NSString stringWithUTF8String:path.c_str()];
        if (!file || !file.isAbsolutePath) return false;
        NSData* data = [NSData dataWithBytesNoCopy:const_cast<void*>(bytes) length:size freeWhenDone:NO];
        NSError* error = nil;
        if (![data writeToFile:file options:NSDataWritingAtomic error:&error]) {
            DebugLog("Cannot replace native settings file: %s\n", error.localizedDescription.UTF8String);
            return false;
        }
        return true;
    }
}
