#include "ChecksumExecutables.h"
#if defined(__APPLE__)
#include "WoW/Source/Apple/LocalServerPolicy.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowServices/Login.h"
#include <stdexcept>
#endif
#include <cstring>
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "Engine/Source/Os/OsVersionHash.h"
#include "WoW/Source/WowServices/HMAC.h"

struct ChecksumFile {
    /* 0000 */ const char* m_path;
    /* 0004 */ int32_t m_type;
};

// STATIC: 0x00AD96E8
static ChecksumFile s_files[3] = { { nullptr, 2 }, { "DivxDecoder.dll", 2 }, { "unicows.dll", 1 } };

// VERIFY: b726580e manual
// OFFSET: 0x006B9CF0
void ChecksumExecutables(const uint8_t* challenge, uint32_t challengeLength, uint8_t* checksum) {
#if defined(__APPLE__)
    if (NativeLocalServerPolicy()) {
        auto* response = ClientServices::CreateLoginResponse();
        auto* login = ClientServices::LoginConnection();
        if (!login || login->GetLoginServerType() != 0 || !response || !NativeServerEndpointAllowed(true, response->GetLoginServer()))
            throw std::runtime_error("Unchecked executable challenge is restricted to the explicitly selected local Grunt server");
        // The opt-in server's StrictVersionCheck=0 does not inspect this field.
        // This is not an executable digest or a fallback after verification fails.
        std::memset(checksum, 0, 20);
        return;
    }
#endif
    HMAC<SHA1> hmac;
    hmac.Init(challenge, challengeLength);

    for (uint32_t i = 0; i < sizeof(s_files) / sizeof(s_files[0]); i++) {
        if (s_files[i].m_type) {
            IOsVersionHashCallback2<HMAC<SHA1>> callback(&hmac);
            IOsVersionChecksum(callback, s_files[i].m_path, s_files[i].m_type == 2);
        }
    }

    SHA1::Digest digest;
    hmac.Finish(digest);
    memcpy(checksum, &digest, sizeof(digest));
}
