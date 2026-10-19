#include "DetailDoodadShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

void GxMetal::ValidateDetailDoodadShader(const std::vector<uint8_t>& bytes, EGxShTarget target) {
    const auto programs = GxReadShaderBinary(bytes, 1024 * 1024);
    if ((target != GxSh_Vertex && target != GxSh_Pixel) || programs.size() != (target == GxSh_Vertex ? 6u : 8u))
        throw std::runtime_error("Unsupported detail-doodad target or permutation count");
    static const char* vertex[] = {"8b9536587a6cbc2c57967bd6ae43efaa", "defac6040460757d8400b109e9c5ce8a", "726289cf74f51d7cda1f2e4691ba0d5a",
        "fd6343f93250ed57ffd7cc5fa3881c85", "206f25c794234d1dd8f2cd960966d3fe", "ff73a6eaa5db40a48479af67fbd42eec"};
    static const char* pixel[] = {"a1574295099b340982c863d96039d70e", "2366039751be45f79c46f67698b6f39d", "1bf025e7524e324c7e20d36c878695dc",
        "8f7c38f3da425c36125649c091fb512a", "a1574295099b340982c863d96039d70e",
        "d1fddae2a32b288449f56bc6541f50f1", "bd30d35dbb08fbce274baeb95fe132da", "b5d5cedb772fce9f1ebc51d191009a9b"};
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
        if (fingerprint != (target == GxSh_Vertex ? vertex[i] : pixel[i]))
            throw std::runtime_error("Detail-doodad shader in current Data has no verified native implementation");
    }
}
