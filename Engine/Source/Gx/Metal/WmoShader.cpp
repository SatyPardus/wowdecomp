#include "WmoShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

namespace GxMetal {
const char* WmoVertexFamily(WmoVertexEffect effect) {
    static const char* names[] = {"MapObjDiffuse_T1", "MapObjSpecular_T1", "MapObjDiffuse_T1_Refl", "MapObjUDiffuse_T1", "MapObjUSpecular_T1", "MapObjUDiffuse_T1_Refl", "MapObjUDiffuse_Comp"};
    const auto index = unsigned(effect);
    if (index >= 7) throw std::runtime_error("Invalid WMO shader family");
    return names[index];
}
const char* WmoPixelFamily(FragmentEffect effect) {
    static const char* names[] = {"MapObjDiffuse", "MapObjOpaque", "MapObjSpecular", "MapObjMetal", "MapObjEnv", "MapObjEnvMetal", "MapObjComposite"};
    const auto index = unsigned(effect) - unsigned(FragmentEffect::WmoDiffuse);
    if (index >= 7) throw std::runtime_error("Invalid WMO shader family");
    return names[index];
}
void ValidateWmoShader(const std::vector<uint8_t>& bytes, EGxShTarget target, WmoVertexEffect vertex, FragmentEffect pixel) {
    const auto programs = GxReadShaderBinary(bytes, 1024 * 1024);
    if (programs.size() != (target == GxSh_Vertex ? 90u : 16u)) throw std::runtime_error("Unsupported WMO shader permutation count");
    static const char* hashes[2][7] = {
        {"451e8ac092f4f4e85af0dbf9ccaaf1ca", "e8c5f398c2d60898195f531f6812b993", "ded8c3cb77eab515c171823da261ba5a", "43319d5f26657af4f4b6402b2552a2f7", "c645ad3640f5a6ed73adfd5aa135fdc4", "b64a727fa04d9462c0b99b064ec528b0", "3cb1e98f43b93819b55c716a66f38dc3"},
        {"1d10e39fb56afa97496012618543cb39", "3f908e6a6e3a9bc4a05b3070d8d6199c", "3f908e6a6e3a9bc4a05b3070d8d6199c", "3f908e6a6e3a9bc4a05b3070d8d6199c", "c574f97365080556e8f900e6d17ad6c8", "57faaef713edefff7372c63477361252", "e7a28b051c4c1ff36775ee17f434bdbe"},
    };
    static const char* shadowHashes[2][7] = {
        {"4d1200a60c11911806e236cd25bdbaa8", "7740bde1c6abb608f489d2e570b70b37", "9cb4ef2956b2b7da719bd3a3f94f9434", "a41ff28f952dea6efe60850d1f0f04df", "d7000d853fd6c7c01a3ef13f1792cc00", "240765e063de83043eff127809436034", "04b2f169dc6d3c4406eba773661c2444"},
        {"9d8444f8b72e71c7a4ae93fe928090b4", "af885b62412f5c615f5466fe0e6e23e4", "af885b62412f5c615f5466fe0e6e23e4", "a186ff114177caa93fbae89b7b31ccbe", "211deb4fc59b8b2f8d2a5292d7c6fc9f", "14ef22b3466e4517efe44d97bc6a4078", "e41d2c8cf4e21f771e6d20955cae167e"},
    };
    const unsigned family = target == GxSh_Vertex ? unsigned(vertex) : unsigned(pixel) - unsigned(FragmentEffect::WmoDiffuse);
    if (family >= 7) throw std::runtime_error("Invalid WMO shader family");
    for (unsigned shadow = 0; shadow < 2; ++shadow) {
        Blizzard::Util::MD5 hash;
        for (unsigned i = 0; i < programs.size(); ++i) {
            const bool receiver = target == GxSh_Vertex ? i >= 30 : (i & 3) != 0;
            if (SupportsWmoPermutation(target, i) && receiver == bool(shadow))
                hash.Process(programs[i].code.data(), uint32_t(programs[i].code.size()));
        }
        uint8_t digest[16]; hash.Finish(digest);
        std::string fingerprint; const char hex[] = "0123456789abcdef";
        for (auto b : digest) { fingerprint += hex[b >> 4]; fingerprint += hex[b & 15]; }
        if (fingerprint != (shadow ? shadowHashes : hashes)[target == GxSh_Pixel][family])
            throw std::runtime_error("WMO shader in current Data has no verified native implementation");
    }
}
}
