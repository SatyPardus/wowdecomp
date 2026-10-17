#include "Engine/Source/Gx/Metal/ModelShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

const char* GxMetal::ModelVertexFamily(ModelVertexEffect effect) {
    switch (effect) {
        case ModelVertexEffect::Diffuse: return "Diffuse_T1";
        case ModelVertexEffect::Color: return "Color_T1";
        case ModelVertexEffect::ColorDiffuse: return "CDiffuse_T1";
    }
    throw std::runtime_error("Invalid native model vertex family");
}
bool GxMetal::SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect) {
    return target == GxSh_Vertex ? permutation < (vertexEffect == ModelVertexEffect::Diffuse ? 30u : 10u) : permutation < 16 && (permutation & 3) == 0;
}
void GxMetal::ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect, ModelVertexEffect vertexEffect) {
    if (!IsModelEffect(effect)) throw std::runtime_error("Invalid model shader family");
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
    static const char* coloredVertex[2][10] = {
        {
            "3e86c7bd92207b6eb87db98dbef85234",
            "b4215c33c04ed75779d25e8b5884a19d",
            "3e86c7bd92207b6eb87db98dbef85234",
            "b4215c33c04ed75779d25e8b5884a19d",
            "3e86c7bd92207b6eb87db98dbef85234",
            "b4215c33c04ed75779d25e8b5884a19d",
            "3e86c7bd92207b6eb87db98dbef85234",
            "b4215c33c04ed75779d25e8b5884a19d",
            "3e86c7bd92207b6eb87db98dbef85234",
            "b4215c33c04ed75779d25e8b5884a19d",
        },
        {
            "3e86c7bd92207b6eb87db98dbef85234",
            "2889ffba6227e08bff1ef82b9ef83968",
            "3e86c7bd92207b6eb87db98dbef85234",
            "319a3528a3638e297f185691619aa643",
            "3e86c7bd92207b6eb87db98dbef85234",
            "643d5a2656432376cd77ac536e390208",
            "3e86c7bd92207b6eb87db98dbef85234",
            "045c27469adfee6b8f5843ddbeb9d3fc",
            "3e86c7bd92207b6eb87db98dbef85234",
            "99d04c70b44aa87dd41647907ab99c8b",
        }
    };
    static const char* pixel[][2] = {
        {"de2319c04400e512097a435bbf2977d1", "f1976a55ee52915aabcdec86b8c18a28"},
        {"94998988c219f84b165ba6b13e021739", "da4b42c3e6ad53adaabc34bbbe240aba"}
    };
    for (uint32_t i = 0; i < shaders.size(); ++i) {
        if (!SupportsModelPermutation(target, i, vertexEffect)) continue;
        const auto& shader = shaders[i];
        Blizzard::Util::MD5 hash;
        hash.Process(shader.code.data(), uint32_t(shader.code.size()));
        uint8_t digest[16]; hash.Finish(digest);
        const char hex[] = "0123456789abcdef";
        std::string fingerprint;
        for (auto byte : digest) { fingerprint += hex[byte >> 4]; fingerprint += hex[byte & 15]; }
        const char* expected = target == GxSh_Vertex ?
            (vertexEffect == ModelVertexEffect::Diffuse ? vertex[i] : coloredVertex[unsigned(vertexEffect) - 1][i]) :
            pixel[effect == FragmentEffect::ModelMod][i >> 3];
        if (fingerprint != expected) {
            throw std::runtime_error("Model shader in current Data has no verified native implementation");
        }
    }
}
