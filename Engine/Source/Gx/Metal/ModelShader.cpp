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
        case ModelVertexEffect::DiffuseReflection: return "Diffuse_Env";
        case ModelVertexEffect::DiffuseEnvironment: return "Diffuse_T1_Env";
        case ModelVertexEffect::DiffuseDualUV: return "Diffuse_T1_T2";
        case ModelVertexEffect::DiffuseProjected: return "Diffuse_Proj_Proj";
        case ModelVertexEffect::ColorProjected: return "Color_Proj_Proj";
    }
    throw std::runtime_error("Invalid native model vertex family");
}
bool GxMetal::SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect, FragmentEffect effect) {
    if (target == GxSh_Vertex)
        return permutation < (vertexEffect == ModelVertexEffect::Diffuse || vertexEffect == ModelVertexEffect::DiffuseEnvironment ? 90u : UsesModelMaterial(vertexEffect) ? 30u : 10u);
    return permutation < 16 && ((permutation & 3) == 0 ||
        (ModelShadowMaterial(effect) && (permutation & 4) == 0));
}
void GxMetal::ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect, ModelVertexEffect vertexEffect) {
    if (!IsModelEffect(effect)) throw std::runtime_error("Invalid model shader family");
    const auto shaders = GxReadShaderBinary(bytes, 1024 * 1024);
    if (shaders.size() != (target == GxSh_Vertex ? 90u : 16u)) {
        throw std::runtime_error("Unsupported model shader permutation count");
    }
    // Bits: lighting + 2*(point lights + 5*(skinning + 3*shadow)).
    // These 30 vertex variants cover 0/1/4 bone influences and 0..4 local lights.
    // Pixel 0/4 share code, as do 8/12 (shader alpha test). The dynamic
    // receivers use R32F depth sampling; hardware PCF remains distinct.
    // Diffuse_T1 dynamic-shadow view/normal/projection outputs (SM3).
    static const char* dynamicShadowVertex[] = {
        "e58d136456637bf01db9db092c89b7f1",
        "95ed459f6acd963af469cd3f445ff4f6",
        "e58d136456637bf01db9db092c89b7f1",
        "5065e3340b9383e0e2d39f30e969c08f",
        "e58d136456637bf01db9db092c89b7f1",
        "cdf6c68318611ea574a203d8d82b9811",
        "e58d136456637bf01db9db092c89b7f1",
        "d80494194d051438d60e2ffee6f2aa1a",
        "e58d136456637bf01db9db092c89b7f1",
        "58a4d789277bdc09c0d012f8f512d899",
        "0c58fc2718cee41e5bfc3e9da6b71108",
        "5afd894a91d58c1b4f7ca79a33f9c3af",
        "0c58fc2718cee41e5bfc3e9da6b71108",
        "5b6afb561e18c3c93b7025bceab1aead",
        "0c58fc2718cee41e5bfc3e9da6b71108",
        "1f55b3a65a2118eb16a7bcf472fdc8f1",
        "0c58fc2718cee41e5bfc3e9da6b71108",
        "3d466d1bbb70088007ba3a83f8f17f4d",
        "0c58fc2718cee41e5bfc3e9da6b71108",
        "f9cf70b9107bf1904376e9584b7404ab",
        "f26d86f644719381bd72b518e89383ba",
        "c132e972518f060c36f312c2402265a0",
        "f26d86f644719381bd72b518e89383ba",
        "2128b1936f05dff8f5668d4fce722b2b",
        "f26d86f644719381bd72b518e89383ba",
        "52018ff7ad1afc4e91bbf8579f3f39c6",
        "f26d86f644719381bd72b518e89383ba",
        "a9cdf18c596b82020244d9998167e233",
        "f26d86f644719381bd72b518e89383ba",
        "c2a55400df8ce24151c9cf907024204e",
    };
    static const char* dynamicShadowPixel[2][2] = {
        {"705a7856449d672da7d8ed3bb011fa84", "c9b9c2d74181fc60bafba4d6ef2321a5"},
        {"d3761940c97672c9351c987201d0f70c", "fbec23c409af74b2db35438405483845"},
    };
    // Four affine projections for the three distant bands and dynamic map.
    static const char* bandedShadowVertex[] = {
        "274d35e4afc5491859d70e7f965ccc11",
        "5622b9710a31c7e3c076587979c3c81f",
        "274d35e4afc5491859d70e7f965ccc11",
        "e954af37ade52242e8597a4d42e1e358",
        "274d35e4afc5491859d70e7f965ccc11",
        "cea3344a97cea1b39424ee6161f2f964",
        "274d35e4afc5491859d70e7f965ccc11",
        "6d02310ecdedcb5eceaf7c13f8e48efb",
        "274d35e4afc5491859d70e7f965ccc11",
        "78fca6c244f1d42def6af06a02e51aa9",
        "7051d60db9b13b7cef4fe060cf5807f6",
        "f7fc9074bc4a2b673e0f64bb48a79c2b",
        "7051d60db9b13b7cef4fe060cf5807f6",
        "1926ec29e1fa2978da7cc3b8460a9b48",
        "7051d60db9b13b7cef4fe060cf5807f6",
        "db4eb2016ebd445aa029141b9dd78867",
        "7051d60db9b13b7cef4fe060cf5807f6",
        "f3a7eb54325b65098032584ea37b0846",
        "7051d60db9b13b7cef4fe060cf5807f6",
        "97e2aab9a9afeeeb49d6a8a4fbc2f324",
        "2b6b3fa4c0a1c613ee628b27a54ed768",
        "70968748fb77e37918cee210c9b1b0c9",
        "2b6b3fa4c0a1c613ee628b27a54ed768",
        "f907b4cb3e55491974a2f9c4928c1386",
        "2b6b3fa4c0a1c613ee628b27a54ed768",
        "f07306965987dfaabae1b2bc29a7237c",
        "2b6b3fa4c0a1c613ee628b27a54ed768",
        "050cb618632eb1b6230b2fe762cfea43",
        "2b6b3fa4c0a1c613ee628b27a54ed768",
        "a551582a2d402df80ba3a0f35c9f3812",
    };
    static const char* bandedShadowPixel[2][2][2] = {
        {{"f384f937ae18203c8f7f7802b7e29a12", "94a9cc00ef6fe43a4d3c93a3c42b5089"}, {"00226b5faf6e5a946bc05b8d1ef2ae9f", "e5d4cb1e35a84f17a0b352c3012edf60"}},
        {{"d83149168880ffd054c521ac54a360cd", "91ce8ef8e1b8be8467f9f306d67a3121"}, {"ea2f6e7059485a470ee85a30dbc92353", "43a8c4381a9b478464df5a7fcbd743ef"}},
    };
    // Environment coordinates and shadow coordinates remain separate outputs.
    static const char* environmentShadowVertex[] = {
        "f2c6860f0b6021cec5e9798e2e1ee754",
        "2a45d2e0b4b97d686aecc53733b087c3",
        "f2c6860f0b6021cec5e9798e2e1ee754",
        "5bf2a26b6d343e20481ff44904d8ba8f",
        "f2c6860f0b6021cec5e9798e2e1ee754",
        "77c731ce7dad550cd3007b46b15615fd",
        "f2c6860f0b6021cec5e9798e2e1ee754",
        "de6947392a1323c6c3bc7abf3d89fda5",
        "f2c6860f0b6021cec5e9798e2e1ee754",
        "eeaafffee44b939e6dcc703f04dd5755",
        "7651c573c45abb216cfc6053a211d4a0",
        "aa0ed9eaf319bd712a968379f1ea8cde",
        "7651c573c45abb216cfc6053a211d4a0",
        "02c0a1b9fcde81f29dbc9e62c1744249",
        "7651c573c45abb216cfc6053a211d4a0",
        "3d1c3aad7530070ca7b220ae01f3fed8",
        "7651c573c45abb216cfc6053a211d4a0",
        "d1a9270f4de2ecc7efc784c2ccc6ea80",
        "7651c573c45abb216cfc6053a211d4a0",
        "952a93569db5a964a858af18bc080306",
        "8c32f2d258b681e658d4681b5c2fdfee",
        "532d4b3ad3663180bab40b50eb0dd754",
        "8c32f2d258b681e658d4681b5c2fdfee",
        "8b0644bac39f7fdc9a5da1e4a616fdfa",
        "8c32f2d258b681e658d4681b5c2fdfee",
        "bf53649d5de3f70c6534d0256cf0ffce",
        "8c32f2d258b681e658d4681b5c2fdfee",
        "11e6046e7f4cf7bba0050bbe4a878631",
        "8c32f2d258b681e658d4681b5c2fdfee",
        "42c3342f4054f495450e1e781af99268",
        "38f51e88c10407978c97601928145477",
        "646c4855bcf9300613b6de5c676be9ec",
        "38f51e88c10407978c97601928145477",
        "47bfdc2dce37ab688d7ae2434a6d8420",
        "38f51e88c10407978c97601928145477",
        "2c36c62fc66f3b14804cca774fa25860",
        "38f51e88c10407978c97601928145477",
        "73799f36b006f041a43007b019c3cef4",
        "38f51e88c10407978c97601928145477",
        "889441666ccecf0373d15d8fdc5af067",
        "ab90f3ea7d870c08c6c455ac1df5fba4",
        "6f3f2bf64dc6b8c16691169f5b4abc95",
        "ab90f3ea7d870c08c6c455ac1df5fba4",
        "6e468f37e91651ea949d3eb5f36697a8",
        "ab90f3ea7d870c08c6c455ac1df5fba4",
        "3af8f066950cf9eae23f7b27ce90dc9b",
        "ab90f3ea7d870c08c6c455ac1df5fba4",
        "7c94b74cdb13a54303b2c9008f89aa14",
        "ab90f3ea7d870c08c6c455ac1df5fba4",
        "5bbec1405d3a2467123a936c7eeb7546",
        "4fc76a868ce3e5281b9fc4ebe5c9aa3e",
        "9ccdad7023327ffe443b109f2bac1e75",
        "4fc76a868ce3e5281b9fc4ebe5c9aa3e",
        "fdd47a5757a9dd37a25dcc2c5936e11a",
        "4fc76a868ce3e5281b9fc4ebe5c9aa3e",
        "8b4899e8b646209b5feaa52ca1197920",
        "4fc76a868ce3e5281b9fc4ebe5c9aa3e",
        "fe6e2cd36a96addf6ad3296f2853cd05",
        "4fc76a868ce3e5281b9fc4ebe5c9aa3e",
        "905bef32c4342b7fd9eceb928528f9f1",
    };
    static const char* environmentShadowPixel[3][2] = {
        {"56cdb1c03a2db9c4bd947f68abce8092", "f6c1e31ed6e331ed2e39dd05ecf44d42"},
        {"b80f4ee76977e6070915e28134d96a1d", "c0711557729aba22332f2e7b75a329cf"},
        {"1c812f7b0d4055e13a4db598fa189c68", "ea3863b36ac3c6df46264c78530b519b"},
    };
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
    static const char* reflectionVertex[] = {
        "b4e40871e5207a2cb6f88d38c4a5ed7e",
        "291d29189f56de0f92df24d19549b797",
        "b4e40871e5207a2cb6f88d38c4a5ed7e",
        "d1fac14054853cea6337b47fcbc14c36",
        "b4e40871e5207a2cb6f88d38c4a5ed7e",
        "11c08d93c780d9791d4e74d66fbce04a",
        "b4e40871e5207a2cb6f88d38c4a5ed7e",
        "330d8ddb133de72b0912f6f44a5b654b",
        "b4e40871e5207a2cb6f88d38c4a5ed7e",
        "b26e613995cc8e9a3ea099b424983d8b",
        "0c4d16fb042b910bf3c268ce5c568ab7",
        "5f9f7bac03fb2004d0d397b42c4c19ba",
        "0c4d16fb042b910bf3c268ce5c568ab7",
        "42f9ef7b4e38d7a132db7c05e833c9d9",
        "0c4d16fb042b910bf3c268ce5c568ab7",
        "dfde7e6d436c706054bd6c1eff5a17cc",
        "0c4d16fb042b910bf3c268ce5c568ab7",
        "d382382b92cad4859bb2aa58539c8691",
        "0c4d16fb042b910bf3c268ce5c568ab7",
        "479aa2ee3f05352fa214a8b24cd2a116",
        "8a4a81c8d61f64cfa59f8f70aadf03af",
        "d7e93da0cd00c0fd8d52ac22d26730a0",
        "8a4a81c8d61f64cfa59f8f70aadf03af",
        "28b6f9714ceafa35cb4f8b28b1e4ee92",
        "8a4a81c8d61f64cfa59f8f70aadf03af",
        "8c9402381fceeeeab33655f01e4e9b95",
        "8a4a81c8d61f64cfa59f8f70aadf03af",
        "5161886e4b1d4586d8d68ebeefd44f71",
        "8a4a81c8d61f64cfa59f8f70aadf03af",
        "17a645135790ba4b32dabdee268c135b",
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
        {"104f8e57582256fa22cbe5e0837c178e", "071e4b850ad8e6e2b0c824572c87f7e2"},
        {"d5b196dae035baf44d83fc4e303183f8", "3e1297741de4b4505d11e5f0635fac18"},
        {"8a6f9019a0d8eefbaafc6cb2a4297504", "edd96442ff48c808c5699802f0ddb183"},
        {"14c860d5d1dc515d2494909da7259cba", "7bfb833f935b83e2834bcd6edcc04b2f"},
        {"8a64d4b05e2398a2ba4c62a5fe4aa997", "fd945970147ffd0cff289204ef658ab1"}
    };
    for (uint32_t i = 0; i < shaders.size(); ++i) {
        if (!SupportsModelPermutation(target, i, vertexEffect, effect)) continue;
        const auto& shader = shaders[i];
        Blizzard::Util::MD5 hash;
        hash.Process(shader.code.data(), uint32_t(shader.code.size()));
        uint8_t digest[16]; hash.Finish(digest);
        const char hex[] = "0123456789abcdef";
        std::string fingerprint;
        for (auto byte : digest) { fingerprint += hex[byte >> 4]; fingerprint += hex[byte & 15]; }
        const char* expected = target == GxSh_Vertex ?
            (i >= 30 && vertexEffect == ModelVertexEffect::DiffuseEnvironment ? environmentShadowVertex[i-30] : i >= 60 ? bandedShadowVertex[i-60] : i >= 30 ? dynamicShadowVertex[i-30] : vertexEffect == ModelVertexEffect::DiffuseReflection ? reflectionVertex[i] : vertexEffect == ModelVertexEffect::DiffuseProjected ? projectedVertex[i] : vertexEffect == ModelVertexEffect::ColorProjected ? projectedColor[i] : vertexEffect == ModelVertexEffect::Diffuse ? vertex[i] : vertexEffect == ModelVertexEffect::DiffuseEnvironment ? environmentVertex[i] : vertexEffect == ModelVertexEffect::DiffuseDualUV ? dualUVVertex[i] : coloredVertex[unsigned(vertexEffect) - 1][i]) :
            ((i & 3) && effect == FragmentEffect::ModelOpaqueAddAlpha ? environmentShadowPixel[(i & 3)-1][i >> 3] : (i & 3) >= 2 ? bandedShadowPixel[unsigned(effect) - unsigned(FragmentEffect::ModelOpaque)][(i & 3)-2][i >> 3] : (i & 3) ? dynamicShadowPixel[unsigned(effect) - unsigned(FragmentEffect::ModelOpaque)][i >> 3] :
                pixel[unsigned(effect) - unsigned(FragmentEffect::ModelOpaque)][i >> 3]);
        if (fingerprint != expected) {
            throw std::runtime_error("Model shader in current Data has no verified native implementation");
        }
    }
}
