#if defined(__APPLE__)
#include "Engine/Source/Os/OsSecureRandom.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/Time.h"
#include <CoreFoundation/CoreFoundation.h>
#include <Security/SecRandom.h>
#include <stdexcept>

void OsSecureRandom(uint8_t* buffer, uint32_t size) {
    if (size && SecRandomCopyBytes(kSecRandomDefault, size, buffer) != errSecSuccess)
        throw std::runtime_error("Apple secure random generation failed");
}
void IOsSecureRandom(uint8_t* buffer, uint32_t size) { OsSecureRandom(buffer, size); }

// These two legacy IDs select the 12340 login wire/version-proof dialect;
// they are not hardware capability queries. Preserve the reference Windows
// client's dialect (the existing Windows x64 build also advertises x86).
uint32_t OsGetProcessorId() { return 0x783836; }
uint32_t OsGetOsId() { return 0x57696e; }

int32_t OsGetTimeZone() {
    CFTimeZoneRef zone = CFTimeZoneCopyDefault();
    const CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
    const double seconds = CFTimeZoneGetSecondsFromGMT(zone, now) - CFTimeZoneGetDaylightSavingTimeOffset(zone, now);
    CFRelease(zone);
    return static_cast<int32_t>(seconds / 60); // Original Windows Bias excludes DST.
}
#endif
