#include "DetailDoodadShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

void GxMetal::ValidateDetailDoodadShader(const std::vector<uint8_t>& bytes, EGxShTarget target) {
    const auto programs = GxReadShaderBinary(bytes, 1024 * 1024);
    if ((target != GxSh_Vertex && target != GxSh_Pixel) || programs.size() != (target == GxSh_Vertex ? 6u : 8u))
        throw std::runtime_error("Unsupported detail-doodad target or permutation count");
    static const char* vertex[] = {"8b9536587a6cbc2c57967bd6ae43efaa", "fd6343f93250ed57ffd7cc5fa3881c85"};
    for (unsigned i = 0; i < programs.size(); ++i) {
        if (!SupportsDetailDoodadPermutation(target, i))
            continue;
        Blizzard::Util::MD5 hash;
        hash.Process(programs[i].code.data(), uint32_t(programs[i].code.size()));
        uint8_t digest[16];
        hash.Finish(digest);
        std::string fingerprint;
        for (auto byte : digest) {
            fingerprint += "0123456789abcdef"[byte >> 4];
            fingerprint += "0123456789abcdef"[byte & 15];
        }
        if (fingerprint != (target == GxSh_Vertex ? vertex[i / 3] : "a1574295099b340982c863d96039d70e"))
            throw std::runtime_error("Detail-doodad shader in current Data has no verified native implementation");
    }
}
