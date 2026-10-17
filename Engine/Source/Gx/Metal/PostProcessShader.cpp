#include "Engine/Source/Gx/Metal/PostProcessShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <cstring>
#include <stdexcept>

GxMetal::PostProcessShader GxMetal::PostProcessShader::FromBls(const std::vector<uint8_t>& bytes, FragmentEffect effect) {
    if (effect != FragmentEffect::Box4 && effect != FragmentEffect::Gauss4 && effect != FragmentEffect::Glow) {
        throw std::runtime_error("Unknown native post-processing effect");
    }
    const auto permutations = GxReadShaderBinary(bytes, 1024 * 1024);
    if (permutations.size() != 2) { throw std::runtime_error("Unsupported FFX permutation count"); }
    // Inspected original ps_2_0 code in patch.MPQ. Box4 averages RGB and
    // preserves vertex alpha; Gauss4 filters RGBA with 1/8,3/8,3/8,1/8.
    const uint8_t box[] = {0xae,0x91,0xaa,0x2e,0x92,0x2b,0xaa,0xa9,0x91,0x22,0xb3,0x8e,0x41,0x45,0x9c,0xf5};
    const uint8_t gauss[] = {0x8e,0x9b,0x13,0xaa,0x98,0x67,0x88,0x89,0x23,0x8c,0x22,0x11,0x77,0xae,0x33,0x75};
    const uint8_t glow[] = {0x9e,0x14,0xba,0x38,0x77,0x89,0xb4,0x95,0x67,0x33,0x4b,0xb2,0x16,0x99,0xd1,0x16};
    const auto* expected = effect == FragmentEffect::Box4 ? box : effect == FragmentEffect::Gauss4 ? gauss : glow;
    const uint32_t metadata = effect == FragmentEffect::Box4 ? 0x7a00 : effect == FragmentEffect::Gauss4 ? 0x7800 : 0x1a00;
    for (const auto& permutation : permutations) {
        Blizzard::Util::MD5 hash;
        hash.Process(permutation.code.data(), uint32_t(permutation.code.size()));
        uint8_t digest[16]; hash.Finish(digest);
        if (permutation.metadata0 != metadata ||
            permutation.metadata1 != 0x200 || permutation.metadata2 != (effect == FragmentEffect::Glow ? 3 : 15) || permutation.metadata3 ||
            std::memcmp(digest, expected, sizeof(digest))) {
            throw std::runtime_error("FFX effect in current Data has no verified native implementation");
        }
    }
    return PostProcessShader(effect);
}
