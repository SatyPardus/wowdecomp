#include "Engine/Source/Gx/Metal/UIShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
std::string Fingerprint(const std::vector<uint8_t>& code) {
    Blizzard::Util::MD5 md5;
    md5.Process(code.data(), uint32_t(code.size()));
    uint8_t digest[16];
    md5.Finish(digest);
    const char hex[] = "0123456789abcdef";
    std::string text;
    for (uint8_t byte : digest) { text += hex[byte >> 4]; text += hex[byte & 15]; }
    return text;
}
}

GxMetal::UIShader GxMetal::UIShader::FromBls(const std::vector<uint8_t>& vertex, const std::vector<uint8_t>& pixel) {
    const auto vs = GxReadShaderBinary(vertex, 1024 * 1024);
    const auto ps = GxReadShaderBinary(pixel, 1024 * 1024);
    if (vs.size() != 2 || ps.size() != 1) { throw std::runtime_error("Unsupported UI shader permutation count"); }
    // Inspected original 3.3.5a ps/vs_2_0 and ps/vs_3_0 code from patch.MPQ:
    // vertex 0: position[i] = dot(c[i], float4(position,1)); pass color/UV.
    // vertex 1: additionally multiply clip.xy by clip.w (screen position at depth).
    // pixel: texture0(uv) * vertex color. Sampler/blend/alpha test remain Gx state.
    const char* vertexCode[2][2] = {{"e2b866d671c21aa1f4b4254d44a3adf6", "887281f15a76cac2d85c217cb291ab13"},
        {"d04abbed3d9106fe79af45af72551280", "0e3330addff73280c136548d7618cc97"}};
    const char* pixelCode[2] = {"9b545adbc271c83e082fc250ed7e0891", "b8d5c368b584854fd6bb94f4890e4ca0"};
    uint32_t vertexMajor = 0, pixelMajor = 0;
    for (unsigned version = 0; version < 2; ++version) {
        if (Fingerprint(vs[0].code) == vertexCode[version][0] && Fingerprint(vs[1].code) == vertexCode[version][1]) { vertexMajor = version + 2; }
        if (Fingerprint(ps[0].code) == pixelCode[version]) { pixelMajor = version + 2; }
    }
    for (const auto& v : vs) {
        if (v.metadata0 != 0xA01 || v.metadata1 != 0xA01 || v.metadata2 || v.metadata3) { vertexMajor = 0; }
    }
    if (ps[0].metadata0 != 0xA00 || ps[0].metadata1 != 0x200 || ps[0].metadata2 != 1 || ps[0].metadata3) { pixelMajor = 0; }
    if (!vertexMajor || !pixelMajor) { throw std::runtime_error("UI shader in current Data has no verified native implementation"); }
    return UIShader(vertexMajor, pixelMajor);
}

GxMetal::UITransform GxMetal::UIShader::VertexTransform(const float* registers, uint32_t count, uint32_t permutation) const {
    if (!registers || count < 4 || permutation > 1) { throw std::runtime_error("Invalid UI shader constants or permutation"); }
    C44Matrix rows;
    static_assert(sizeof(rows) == 16 * sizeof(float), "Gx constant register layout");
    std::memcpy(&rows, registers, sizeof(rows));
    // Game upload supplies four dot-product rows, not a Tempest row-vector matrix.
    return {rows.Transpose(), permutation == 1};
}

void GxMetal::UIShader::ValidateDesaturate(const std::vector<uint8_t>& pixel) {
    const auto ps = GxReadShaderBinary(pixel, 1024 * 1024);
    if (ps.size() != 1 || ps[0].metadata0 != 0xA00 || ps[0].metadata1 != 0x200 || ps[0].metadata2 != 1 || ps[0].metadata3)
        throw std::runtime_error("Unsupported Desaturate BLS layout");
    const auto code = Fingerprint(ps[0].code);
    if (code != "baa7de4149d0c7eaf626d201a5476295" && code != "6064371e3176bad8aa6a4fcbe87f2e03")
        throw std::runtime_error("Desaturate shader in current Data has no verified native implementation");
}
