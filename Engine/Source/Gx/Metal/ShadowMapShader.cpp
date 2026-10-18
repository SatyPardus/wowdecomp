#include "ShadowMapShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

void GxMetal::ValidateShadowMapShader(const std::vector<uint8_t>& bytes, EGxShTarget target) {
    const auto programs = GxReadShaderBinary(bytes, 1024 * 1024);
    if ((target != GxSh_Vertex && target != GxSh_Pixel) || programs.size() != (target == GxSh_Vertex ? 90u : 16u))
        throw std::runtime_error("Unsupported shadow-map shader target or permutation count");
    static const char* vertex[] = {"505213ac7a6a058cc60c75546f371fb2", "8c83d68c509bc3f3a67dcd90bf87b544",
                                   "d0e0a44f8da92660bd31a3cb0dcf7dc5"};
    static const char* pixel[] = {"8eb7632bd84d472c34c4ff62b4bbf147", "5b3759283f9e09e6adfd08c67ac90b58"};
    for (unsigned i = 0; i < programs.size(); ++i) {
        Blizzard::Util::MD5 hash;
        hash.Process(programs[i].code.data(), uint32_t(programs[i].code.size()));
        uint8_t digest[16];
        hash.Finish(digest);
        std::string fingerprint;
        for (auto byte : digest) {
            fingerprint += "0123456789abcdef"[byte >> 4];
            fingerprint += "0123456789abcdef"[byte & 15];
        }
        if (fingerprint != (target == GxSh_Vertex ? vertex[ShadowMapSkinning(i)] : pixel[i >> 3]))
            throw std::runtime_error("Shadow-map shader in current Data has no verified native implementation");
    }
}
