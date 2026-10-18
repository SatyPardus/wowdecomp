#include "WaterRippleShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

void GxMetal::ValidateWaterRippleShader(const std::vector<uint8_t>& bytes, EGxShTarget target) {
    const auto programs = GxReadShaderBinary(bytes, 1024 * 1024);
    if ((target != GxSh_Vertex && target != GxSh_Pixel) || programs.size() != 1)
        throw std::runtime_error("Unsupported water-ripple target or permutation count");
    Blizzard::Util::MD5 hash;
    hash.Process(programs[0].code.data(), uint32_t(programs[0].code.size()));
    uint8_t digest[16];
    hash.Finish(digest);
    std::string fingerprint;
    for (auto byte : digest) {
        fingerprint += "0123456789abcdef"[byte >> 4];
        fingerprint += "0123456789abcdef"[byte & 15];
    }
    if (fingerprint != (target == GxSh_Vertex ? "05d64d32df616f22c6c681928486b798" : "35cd53c7e6f4228c4a9261d2b8f5d78b"))
        throw std::runtime_error("Water-ripple shader in current Data has no verified native implementation");
}
