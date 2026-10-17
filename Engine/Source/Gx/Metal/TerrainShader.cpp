#include "Engine/Source/Gx/Metal/TerrainShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

void GxMetal::ValidateTerrainShader(const std::vector<uint8_t>& bytes, EGxShTarget target) {
    const auto shaders = GxReadShaderBinary(bytes, 1024 * 1024);
    if (shaders.size() != (target == GxSh_Vertex ? 128u : 32u))
        throw std::runtime_error("Unsupported terrain shader permutation count");
    static const char* vertex[] = {
        "81ac7af9ae710577608cbcb8ad77470b",
        "054b63866bcf1651e6e76f5e36e53d3d",
        "6d943ed204a8900ae7a63dd626d17be9",
        "1246378e4b05ef73114c5bc5409607e2",
        "a4dd24270d495e353e5d8458b08c9464",
        "2194ec39067f8b5cbc379fd6e4b89e74",
        "5e705e01525391ba0b91da9dec1dae31",
        "7c6f28760d5c34c5dc808071e3966fb0",
        "85f88f62c50c988d68a3701c50df2236",
        "7b806e0a1d4b09ceb00376ed2fc0c392",
        "8a47990c33d62d4d839db5d7f4f23d15",
        "31d634f3645fc18bcb9a23e84a35ed16",
        "baf147e10ab128f72927411885cb2992",
        "d8e703cc9dee36b607cdb15ee813b2a6",
        "1c6367c5bb9b739aa6e67f86e0f5af19",
        "a5f4e4ee8730741245c0f913477e4781",
        "f26134f30b95f193b249e7cf654f1e68",
        "7ccd667f7e66c4048610e2cbfaaa7828",
        "6d85278de4d945639a17eead9f706b19",
        "a6abc6e2fc4a55a1159ba02be94f2f85",
        "2631ff70fb14699b5adbdbb47505d7b1",
        "51ee29caeb7011bbed6dcabf78f8317d",
        "f51f098221c233de5451221bfdb2332b",
        "f43ba95ca3eb9f7ac1849d1bcd049306",
        "3d8935cf781b00693cfa10eea46cce90",
        "eb0fc4c986771c4e172cc6bd5c48c5d9",
        "7fd1299333912582db9b6e35db3c3853",
        "887646e3826e2f5273d614a41badb8b8",
        "e1a31b9687b7fa03bd203a76fefc7343",
        "7473de83b8abca272e531eca0a5fb580",
        "d2b17fa15d9c3e865b29e911fd90602d",
        "02a231ce4cf5f8de31b23f75b3a90d8d",
    };
    static const char* pixel[] = {
        "7c43dd4074108affaf683aada6ddb5fb",
        "90c5413ac98968f6fe4923d6a1c83705",
        "81596a8ec24e95a5570674a8654a6332",
        "6bbfaebacae5f8806dc73ad9451aa3c9",
        "a3a015f52ac4119af22e53c30937575e",
        "f3a58f9792f6431f9c63cadbb76ed5bf",
        "a19ddc630f1b795a9d0401be8fa3f14f",
        "663588fdfb6c9123600c1a63874d949f",
        "0578108775ed2b3156d45b30649a913d",
        "da6cead50d7330e54bb8def6792804d0",
        "69e2c539ad38bd03c4a74c45c056fd27",
        "65de9c5dd23d577e9856b91de210713f",
        "5f3a5c6ba289c7e1507833d6d5885807",
        "677bf4bcd19d1d69d01f33ba86b9743e",
        "cdb7d96523b47c412d7f036923b8bbe7",
        "0eafec508a99ae16a260a8f8a6c8751c",
    };
    for (uint32_t i = 0; i < shaders.size(); ++i) {
        if (!SupportsTerrainPermutation(target, i)) continue;
        Blizzard::Util::MD5 hash;
        hash.Process(shaders[i].code.data(), uint32_t(shaders[i].code.size()));
        uint8_t digest[16]; hash.Finish(digest);
        const char hex[] = "0123456789abcdef";
        std::string fingerprint;
        for (auto byte : digest) { fingerprint += hex[byte >> 4]; fingerprint += hex[byte & 15]; }
        if (fingerprint != (target == GxSh_Vertex ? vertex[i / 4] : pixel[i / 2]))
            throw std::runtime_error("Terrain shader in current Data has no verified native implementation");
    }
}
