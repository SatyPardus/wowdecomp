#if defined(__APPLE__)
#include "Engine/Source/Storm/H/storm.h"
#include <CoreFoundation/CoreFoundation.h>
#include <cstdlib>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <string>

namespace {
// App-owned native preferences. Registry hive flags retain their user/system
// lookup order; no Windows registry or other application's preferences are read.
const CFStringRef Application = CFSTR("org.wowdecomp.client");
struct Owned {
    CFTypeRef value = nullptr;
    explicit Owned(CFTypeRef value = nullptr) : value(value) {}
    ~Owned() {
        if (value)
            CFRelease(value);
    }
    Owned(const Owned &) = delete;
    Owned &operator=(const Owned &) = delete;
};
CFStringRef MakeKey(const char *key, const char *name, uint32_t flags) {
    if (!key || !*key || !name || !*name) {
        SErrSetLastError(87);
        return nullptr;
    }
    std::string full;
    if (!(flags & SREG_ABSOLUTE))
        full = flags & SREG_BATTLE_NET ? "Software\\Battle.net\\" : "Software\\Blizzard Entertainment\\";
    auto append = [&](const char *text) {
        for (; *text; ++text) {
            if (*text == '%')
                full += "%25";
            else if (*text == '\n')
                full += "%0A";
            else
                full += *text;
        }
    };
    append(key);
    full += '\n';
    append(name);
    Owned source(CFStringCreateWithCString(kCFAllocatorDefault, full.c_str(), kCFStringEncodingUTF8));
    if (!source.value) {
        SErrSetLastError(87);
        return nullptr;
    }
    auto result = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, static_cast<CFStringRef>(source.value));
    if (!result) {
        SErrSetLastError(8);
        return nullptr;
    }
    CFStringLowercase(result, nullptr);
    return result;
}
CFPropertyListRef Load(CFStringRef key, uint32_t flags) {
    if (!key)
        return nullptr;
    CFPropertyListRef result = nullptr;
    if (!(flags & SREG_EXCLUDE_CURRENT_USER))
        result = CFPreferencesCopyValue(key, Application, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    if (!result && !(flags & SREG_EXCLUDE_LOCAL_MACHINE))
        result = CFPreferencesCopyValue(key, Application, kCFPreferencesAnyUser, kCFPreferencesAnyHost);
    if (!result)
        SErrSetLastError(2);
    return result;
}
bool AsString(CFTypeRef value, std::string &result) {
    if (CFGetTypeID(value) == CFStringGetTypeID()) {
        auto string = static_cast<CFStringRef>(value);
        const auto bytes = CFStringGetMaximumSizeForEncoding(CFStringGetLength(string), kCFStringEncodingUTF8);
        result.resize(size_t(bytes) + 1);
        if (!CFStringGetCString(string, result.data(), result.size(), kCFStringEncodingUTF8))
            return false;
        result.resize(std::strlen(result.c_str()));
        return true;
    }
    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        int64_t number;
        if (!CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberSInt64Type, &number) || number < 0 || uint64_t(number) > UINT32_MAX)
            return false;
        result = std::to_string(number);
        return true;
    }
    return false;
}
int32_t Save(CFStringRef key, uint32_t flags, CFTypeRef value) {
    if (!key || !value)
        return 0;
    const auto user = flags & SREG_EXCLUDE_CURRENT_USER ? kCFPreferencesAnyUser : kCFPreferencesCurrentUser;
    CFPreferencesSetValue(key, value, Application, user, kCFPreferencesAnyHost);
    // Synchronize even without FLUSH_KEY: the legacy API reports persistence
    // errors before success, and callers do not have a native close operation.
    if (!CFPreferencesSynchronize(Application, user, kCFPreferencesAnyHost)) {
        SErrSetLastError(5);
        return 0;
    }
    return 1;
}
} // namespace

int32_t STORMAPI SRegLoadValue(const char *keyname, const char *valuename, uint32_t flags, uint32_t *value) {
    if (!value) {
        SErrSetLastError(87);
        return 0;
    }
    Owned key(MakeKey(keyname, valuename, flags));
    Owned stored(Load(static_cast<CFStringRef>(key.value), flags));
    if (!stored.value)
        return 0;
    std::string text;
    if (!AsString(stored.value, text)) {
        SErrSetLastError(13);
        return 0;
    }
    // Match the original string-to-DWORD conversion, including base prefixes.
    const char *first = text.c_str();
    while (std::isspace(static_cast<unsigned char>(*first)))
        ++first;
    const bool negative = *first == '-';
    if (*first == '+' || *first == '-')
        ++first;
    if (*first == '+' || *first == '-' || std::isspace(static_cast<unsigned char>(*first))) {
        *value = 0;
        return 1;
    }
    errno = 0;
    const auto magnitude = std::strtoull(first, nullptr, 0);
    *value = errno == ERANGE || magnitude > UINT32_MAX ? UINT32_MAX : negative ? uint32_t(0) - uint32_t(magnitude) : uint32_t(magnitude);
    return 1;
}
int32_t STORMAPI SRegLoadString(const char *keyname, const char *valuename, uint32_t flags, char *buffer, uint32_t size) {
    if (!buffer || !size) {
        SErrSetLastError(87);
        return 0;
    }
    Owned key(MakeKey(keyname, valuename, flags));
    Owned stored(Load(static_cast<CFStringRef>(key.value), flags));
    if (!stored.value)
        return 0;
    std::string text;
    if (!AsString(stored.value, text)) {
        SErrSetLastError(13);
        return 0;
    }
    const auto count = std::min(text.size(), size_t(size - 1));
    std::memcpy(buffer, text.data(), count);
    buffer[count] = 0;
    return 1;
}
int32_t STORMAPI SRegSaveValue(const char *keyname, const char *valuename, uint32_t flags, uint32_t value) {
    Owned key(MakeKey(keyname, valuename, flags));
    const int64_t number = value;
    Owned stored(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &number));
    return Save(static_cast<CFStringRef>(key.value), flags, stored.value);
}
int32_t STORMAPI SRegSaveString(const char *keyname, const char *valuename, uint32_t flags, const char *value) {
    if (!value) {
        SErrSetLastError(87);
        return 0;
    }
    Owned key(MakeKey(keyname, valuename, flags));
    Owned stored(CFStringCreateWithCString(kCFAllocatorDefault, value, kCFStringEncodingUTF8));
    if (!stored.value) {
        SErrSetLastError(87);
        return 0;
    }
    return Save(static_cast<CFStringRef>(key.value), flags, stored.value);
}
#endif
