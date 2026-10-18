#include "Engine/Source/Gx/Metal/PostProcessShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <cstring>
#include <stdexcept>

GxMetal::PostProcessShader GxMetal::PostProcessShader::FromBls(const std::vector<uint8_t>& bytes, FragmentEffect effect) {
    if (effect != FragmentEffect::Box4 && effect != FragmentEffect::Gauss4 && effect != FragmentEffect::Glow && effect != FragmentEffect::GlowWave && effect != FragmentEffect::Death) {
        throw std::runtime_error("Unknown native post-processing effect");
    }
    const auto permutations = GxReadShaderBinary(bytes, 1024 * 1024);
    if (permutations.size() != (effect == FragmentEffect::GlowWave ? 4 : 2)) { throw std::runtime_error("Unsupported FFX permutation count"); }
    // Inspected original ps_2_0 code in patch.MPQ. Box4 averages RGB and
    // preserves vertex alpha; Gauss4 filters RGBA with 1/8,3/8,3/8,1/8.
    const uint8_t box[] = {0xae,0x91,0xaa,0x2e,0x92,0x2b,0xaa,0xa9,0x91,0x22,0xb3,0x8e,0x41,0x45,0x9c,0xf5};
    const uint8_t gauss[] = {0x8e,0x9b,0x13,0xaa,0x98,0x67,0x88,0x89,0x23,0x8c,0x22,0x11,0x77,0xae,0x33,0x75};
    const uint8_t glow[] = {0x9e,0x14,0xba,0x38,0x77,0x89,0xb4,0x95,0x67,0x33,0x4b,0xb2,0x16,0x99,0xd1,0x16};
    const uint8_t death[] = {0xed,0x59,0x8c,0x38,0xff,0x96,0x1f,0x55,0xa4,0x38,0xbb,0x9a,0x31,0x0e,0xd9,0x85};
    const uint8_t wave[2][16] = {
        {0x0d,0xa4,0xcb,0xa3,0x29,0x8f,0xee,0x3c,0x97,0x27,0x63,0xab,0x17,0x5f,0xc0,0xfa},
        {0xa8,0x0a,0xfb,0x31,0x71,0x95,0x8a,0x56,0x86,0x90,0xd6,0x0a,0xa6,0xf6,0xb1,0x58}
    };
    const auto* expected = effect == FragmentEffect::Box4 ? box : effect == FragmentEffect::Gauss4 ? gauss : effect == FragmentEffect::Death ? death : glow;
    const bool sceneBlur = effect == FragmentEffect::Glow || effect == FragmentEffect::Death;
    const uint32_t metadata = effect == FragmentEffect::Box4 ? 0x7a00 : effect == FragmentEffect::Gauss4 ? 0x7800 : sceneBlur ? 0x1a00 : 0x3a00;
    for (size_t i = 0; i < permutations.size(); ++i) {
        const auto& permutation = permutations[i];
        Blizzard::Util::MD5 hash;
        hash.Process(permutation.code.data(), uint32_t(permutation.code.size()));
        uint8_t digest[16]; hash.Finish(digest);
        if (permutation.metadata0 != metadata ||
            permutation.metadata1 != 0x200 || permutation.metadata2 != (sceneBlur ? 3 : effect == FragmentEffect::GlowWave ? 7 : 15) || permutation.metadata3 ||
            std::memcmp(digest, effect == FragmentEffect::GlowWave ? wave[i / 2] : expected, sizeof(digest))) {
            throw std::runtime_error("FFX effect in current Data has no verified native implementation");
        }
    }
    return PostProcessShader(effect);
}
