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
        case ModelVertexEffect::DiffuseEnvironment: return "Diffuse_T1_Env";
        case ModelVertexEffect::DiffuseDualUV: return "Diffuse_T1_T2";
        case ModelVertexEffect::DiffuseProjected: return "Diffuse_Proj_Proj";
        case ModelVertexEffect::ColorProjected: return "Color_Proj_Proj";
    }
    throw std::runtime_error("Invalid native model vertex family");
}
bool GxMetal::SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect) {
    return target == GxSh_Vertex ? permutation < (UsesModelMaterial(vertexEffect) ? 30u : 10u) : permutation < 16 && (permutation & 3) == 0;
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
    static const char* environmentVertex[] = {
        "3f3cadc99a29016bfd8e991941d6766a",
        "6900828bca505cae2812746ac208431f",
        "3f3cadc99a29016bfd8e991941d6766a",
        "4157d46fb8d910cf0bc154fcadf2a42d",
        "3f3cadc99a29016bfd8e991941d6766a",
        "55b6a7348edf9c25b3f84bc729f23357",
        "3f3cadc99a29016bfd8e991941d6766a",
        "ff398f00fbcf9dcbd83b98a76e1e6e80",
        "3f3cadc99a29016bfd8e991941d6766a",
        "73ef5a5c3cc1a97501407fb5608fd933",
        "5b5c38b4908b9e9c4fe07718fc8d850a",
        "4fa54f49a6e4c7599f3c004d657c8e38",
        "5b5c38b4908b9e9c4fe07718fc8d850a",
        "7129d62ea4130ec9ada489e03ac8225c",
        "5b5c38b4908b9e9c4fe07718fc8d850a",
        "04a8c94c851aacd462f6a1c08c5650c7",
        "5b5c38b4908b9e9c4fe07718fc8d850a",
        "f71fc90ddb5cc35946f559cd9187b4ef",
        "5b5c38b4908b9e9c4fe07718fc8d850a",
        "8bd86be6209d9737cf0715bb0fe99cbd",
        "a14eeef1e4f268078b0ba9a86692c4a0",
        "35ffc9468b600487edf489dc93090874",
        "a14eeef1e4f268078b0ba9a86692c4a0",
        "b7a8fb4df133aaee6cf74e691305e2dd",
        "a14eeef1e4f268078b0ba9a86692c4a0",
        "6649616cd48dada880abc2c04c64a35f",
        "a14eeef1e4f268078b0ba9a86692c4a0",
        "70c70bba80a6b12dd2b232b096b3fb29",
        "a14eeef1e4f268078b0ba9a86692c4a0",
        "37ffa88b730684fb2f74101f916b3adc",
    };
    static const char* dualUVVertex[] = {
        "9c9e897a4d3d43801ba8ed36c99366d0",
        "ab27e3f11a2caca161f329b64a9ad8ec",
        "9c9e897a4d3d43801ba8ed36c99366d0",
        "1a3bc9124f03aeae8b755dd42da7f165",
        "9c9e897a4d3d43801ba8ed36c99366d0",
        "4f39088aad18af2f3261856b76de7f3b",
        "9c9e897a4d3d43801ba8ed36c99366d0",
        "05ea8fca55dfab08426e73bc458cfa2b",
        "9c9e897a4d3d43801ba8ed36c99366d0",
        "c728587ed7f2b85da944ba2fdec7d572",
        "f76d4dc0ce4be64a5d4a103b7e58ad11",
        "0b8a1d2436e7af096481b895029062cb",
        "f76d4dc0ce4be64a5d4a103b7e58ad11",
        "2af712f7a2f8453b77a1efbdb4640c2f",
        "f76d4dc0ce4be64a5d4a103b7e58ad11",
        "cd6d1100f3beb8cb8f4a33fc45d40dfb",
        "f76d4dc0ce4be64a5d4a103b7e58ad11",
        "8d55432cac6a875828511231b65b2d45",
        "f76d4dc0ce4be64a5d4a103b7e58ad11",
        "4732b6655748f20ca6db0197d3858ab9",
        "687923f6a1848e9fd4af0f530f6ccde8",
        "a58a37f47f89864922cf813cb9e86c83",
        "687923f6a1848e9fd4af0f530f6ccde8",
        "8b86537cc0e2981152c16773f7b476b6",
        "687923f6a1848e9fd4af0f530f6ccde8",
        "596a269714148435e32360d53f7cab8a",
        "687923f6a1848e9fd4af0f530f6ccde8",
        "95b8162b63c19b04e76b0dd93ad8394c",
        "687923f6a1848e9fd4af0f530f6ccde8",
        "30826eb2113fe6875825afbca774f10c",
    };
    static const char* projectedVertex[] = {
        "e61be162d52df63987efbb70ad7939e0",
        "25ad77a79835323086e53c65d9dceac1",
        "e61be162d52df63987efbb70ad7939e0",
        "7a20c390754eefd928cf9f218442c1c3",
        "e61be162d52df63987efbb70ad7939e0",
        "0491e465648587bfb0079a0005018104",
        "e61be162d52df63987efbb70ad7939e0",
        "fb28f6231bac715dc90a8900c9b917b2",
        "e61be162d52df63987efbb70ad7939e0",
        "764fe329c5c28f0f1be769ede97219b7",
        "29a31675e194e89eff9519a78f02e971",
        "e8bb802a32c0818bc07a8ab4ed683fa5",
        "29a31675e194e89eff9519a78f02e971",
        "3604bf43667010e8284565c7647285bb",
        "29a31675e194e89eff9519a78f02e971",
        "a54117e92e0d26af08debc5397d71cb1",
        "29a31675e194e89eff9519a78f02e971",
        "0b8da80c3405182bdec93e76a6ee5121",
        "29a31675e194e89eff9519a78f02e971",
        "8ae33e61a473ed2ac92e4bd74eb42c9f",
        "9bb27855b3402efb82ef7e0968a74df0",
        "44d622b5140f6fc963015b83d90ce710",
        "9bb27855b3402efb82ef7e0968a74df0",
        "a2020e2ebd5796d900f13d2853513cb8",
        "9bb27855b3402efb82ef7e0968a74df0",
        "16fee985d588cc8c6e1cf96478497595",
        "9bb27855b3402efb82ef7e0968a74df0",
        "1f1ab36df2d7f76aa57d67815f50affd",
        "9bb27855b3402efb82ef7e0968a74df0",
        "019ecb8d12142f5ae5beccdf102196fc",
    };
    static const char* projectedColor[] = {
        "8ebba96931c1072b46a9c6aeb0b2c26f",
        "edaba469f2df9db576f0e43724b68006",
        "8ebba96931c1072b46a9c6aeb0b2c26f",
        "edaba469f2df9db576f0e43724b68006",
        "8ebba96931c1072b46a9c6aeb0b2c26f",
        "edaba469f2df9db576f0e43724b68006",
        "8ebba96931c1072b46a9c6aeb0b2c26f",
        "edaba469f2df9db576f0e43724b68006",
        "8ebba96931c1072b46a9c6aeb0b2c26f",
        "edaba469f2df9db576f0e43724b68006",
    };
    static const char* pixel[][2] = {
        {"de2319c04400e512097a435bbf2977d1", "f1976a55ee52915aabcdec86b8c18a28"},
        {"94998988c219f84b165ba6b13e021739", "da4b42c3e6ad53adaabc34bbbe240aba"},
        {"7751ab618fb321ce2818312a85dab5f6", "9e6e050436798f2d9e2070bc0f0add60"},
        {"80dec97eb1bddae15fcf8bee0bc09445", "ada818ef0b3cdca6a8f59f971d74fc98"},
        {"5ae2fdbfbdecb3864fa1a4f665ab4069", "753a6bc77766be7de0240f1456909607"},
        {"a700448d7b2acfa0090cc20e403f20a0", "aadd717cd5eb4835808976befaae5c37"},
        {"a0d3210786631ecd99e5f217fe76153e", "08d2d53838ad3d5a70573d5f76d0fba3"},
        {"cab1b81fa97d3f61e4a57fd1fad15f8f", "992c4b1bd94c87b9e0e07045471d12b9"},
        {"104f8e57582256fa22cbe5e0837c178e", "071e4b850ad8e6e2b0c824572c87f7e2"}
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
            (vertexEffect == ModelVertexEffect::DiffuseProjected ? projectedVertex[i] : vertexEffect == ModelVertexEffect::ColorProjected ? projectedColor[i] : vertexEffect == ModelVertexEffect::Diffuse ? vertex[i] : vertexEffect == ModelVertexEffect::DiffuseEnvironment ? environmentVertex[i] : vertexEffect == ModelVertexEffect::DiffuseDualUV ? dualUVVertex[i] : coloredVertex[unsigned(vertexEffect) - 1][i]) :
            pixel[unsigned(effect) - unsigned(FragmentEffect::ModelOpaque)][i >> 3];
        if (fingerprint != expected) {
            throw std::runtime_error("Model shader in current Data has no verified native implementation");
        }
    }
}
