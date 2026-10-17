#include "Engine/Source/Gx/Metal/ModelShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

bool GxMetal::SupportsModelPermutation(EGxShTarget target, uint32_t permutation) {
    return target == GxSh_Vertex ? permutation < 30 : permutation < 16 && (permutation & 3) == 0;
}
void GxMetal::ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target) {
    const auto shaders = GxReadShaderBinary(bytes, 1024 * 1024);
    if (shaders.size() != (target == GxSh_Vertex ? 90u : 16u)) {
        throw std::runtime_error("Unsupported model shader permutation count");
    }
    // Bits: lighting + 2*(point lights + 5*(skinning + 3*shadow)).
    // These 30 vertex variants cover 0/1/4 bone influences and 0..4 local lights.
    // Pixel 0/4 share code, as do 8/12 (shader alpha test). Shadows are not mapped
    // to these functions: selecting one fails explicitly at the draw call.
    static const char* vertex[] = {
        "089abea1c8a391a0f0f946dbca2d42bc",
        "40d1a248ecea6289d7f1b55863232ceb",
        "089abea1c8a391a0f0f946dbca2d42bc",
        "62d1da28d7eab7dd23b4f480f36823b5",
        "089abea1c8a391a0f0f946dbca2d42bc",
        "0d3b747450b06bdf0e79247ea2fb6d5c",
        "089abea1c8a391a0f0f946dbca2d42bc",
        "8ec4da4170d9fb3df33ee65ed673f65f",
        "089abea1c8a391a0f0f946dbca2d42bc",
        "3ce97e8829f26fa788d3f93fc392fbe1",
        "e19c0507bd414653fe165ee0d439a148",
        "d7b5a993c4426096134f2e785cd48386",
        "e19c0507bd414653fe165ee0d439a148",
        "93c33771d9eb202f2edf1bcde26641fe",
        "e19c0507bd414653fe165ee0d439a148",
        "d05b0758dbabeb97b4b5dbd071778bc0",
        "e19c0507bd414653fe165ee0d439a148",
        "bc6b1403bc121080cfe2c1d0ba6555a7",
        "e19c0507bd414653fe165ee0d439a148",
        "b2ec868e126d2442de4bc1d823fec7a6",
        "548b9cd8094d5a518266ca5d13fce67b",
        "12618e3e3d982af4a3787c4488ae4e34",
        "548b9cd8094d5a518266ca5d13fce67b",
        "b47006e1933065da0e2325ddb1f5428d",
        "548b9cd8094d5a518266ca5d13fce67b",
        "7dc140245323bc45cfc3b675b76ab1dd",
        "548b9cd8094d5a518266ca5d13fce67b",
        "4f70942293ec9ab8303fea498290ac22",
        "548b9cd8094d5a518266ca5d13fce67b",
        "9226c755d99d09b6f86eeefbcf62d3b5",
    };
    static const char* pixel[] = {"de2319c04400e512097a435bbf2977d1", "f1976a55ee52915aabcdec86b8c18a28"};
    for (uint32_t i = 0; i < shaders.size(); ++i) {
        if (!SupportsModelPermutation(target, i)) continue;
        const auto& shader = shaders[i];
        Blizzard::Util::MD5 hash;
        hash.Process(shader.code.data(), uint32_t(shader.code.size()));
        uint8_t digest[16]; hash.Finish(digest);
        const char hex[] = "0123456789abcdef";
        std::string fingerprint;
        for (auto byte : digest) { fingerprint += hex[byte >> 4]; fingerprint += hex[byte & 15]; }
        if (fingerprint != (target == GxSh_Vertex ? vertex[i] : pixel[i >> 3])) {
            throw std::runtime_error("Model shader in current Data has no verified native implementation");
        }
    }
}
