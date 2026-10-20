#include "LiquidShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

namespace GxMetal {
const char* LiquidShaderFamily(FragmentEffect effect, EGxShTarget target) {
    static const char* names[2][4] = {{"vsLiquidWater", "vsLiquidWaterNoSpec", "vsLiquidMagma", "vsLiquidProcWater"},
        {"psLiquidWater", "psLiquidWaterNoSpec", "psLiquidMagma", "psLiquidProcWater"}};
    if (!IsLiquidEffect(effect)) throw std::runtime_error("Invalid liquid shader family");
    return names[target == GxSh_Pixel][unsigned(effect) - unsigned(FragmentEffect::LiquidWater)];
}
void ValidateLiquidShader(const std::vector<uint8_t>& bytes, FragmentEffect effect, EGxShTarget target) {
    if (!IsLiquidEffect(effect)) throw std::runtime_error("Invalid liquid shader family");
    const auto programs = GxReadShaderBinary(bytes, 1024 * 1024);
    if (programs.size() != LiquidPermutationCount(effect, target)) throw std::runtime_error("Unsupported liquid shader permutation count");
    static const char* hashes[2][4] = {
        {"156486fdac3bd33cf2de0652655816ff", "df36da92f158f87f06b5c7b9b296b138", "b434ecb654f7ae3525008018d62d1d4f", "0eb6a3d78c5f276aa2a5f572d4d431a6"},
        {"67f5d0116a8c4ecd9f0d9f28bf789532", "fb10739404ab88f23b097e244c4de8f7", "083788a8505f90ad0b3b792ff1d6a257", "556376220be5cee37b6409513f158a24"},
    };
    Blizzard::Util::MD5 hash;
    for (const auto& program : programs) hash.Process(program.code.data(), uint32_t(program.code.size()));
    uint8_t digest[16]; hash.Finish(digest);
    std::string fingerprint; const char hex[] = "0123456789abcdef";
    for (auto b : digest) { fingerprint += hex[b >> 4]; fingerprint += hex[b & 15]; }
    if (fingerprint != hashes[target == GxSh_Pixel][unsigned(effect) - unsigned(FragmentEffect::LiquidWater)])
        throw std::runtime_error("Liquid shader in current Data has no verified native implementation");
}
}
