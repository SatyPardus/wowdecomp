#include "Engine/Source/Gx/Metal/TerrainShader.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include <stdexcept>
#include <string>

void GxMetal::ValidateTerrainShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect) {
    const auto shaders = GxReadShaderBinary(bytes, 1024 * 1024);
    if (shaders.size() != TerrainPermutationCount(target, effect))
        throw std::runtime_error("Unsupported terrain shader permutation count");
    static const char* vertex[] = {
        "81ac7af9ae710577608cbcb8ad77470b", "054b63866bcf1651e6e76f5e36e53d3d", "6d943ed204a8900ae7a63dd626d17be9",
        "1246378e4b05ef73114c5bc5409607e2", "a4dd24270d495e353e5d8458b08c9464", "2194ec39067f8b5cbc379fd6e4b89e74",
        "5e705e01525391ba0b91da9dec1dae31", "7c6f28760d5c34c5dc808071e3966fb0", "85f88f62c50c988d68a3701c50df2236",
        "7b806e0a1d4b09ceb00376ed2fc0c392", "8a47990c33d62d4d839db5d7f4f23d15", "31d634f3645fc18bcb9a23e84a35ed16",
        "baf147e10ab128f72927411885cb2992", "d8e703cc9dee36b607cdb15ee813b2a6", "1c6367c5bb9b739aa6e67f86e0f5af19",
        "a5f4e4ee8730741245c0f913477e4781", "f26134f30b95f193b249e7cf654f1e68", "7ccd667f7e66c4048610e2cbfaaa7828",
        "6d85278de4d945639a17eead9f706b19", "a6abc6e2fc4a55a1159ba02be94f2f85", "2631ff70fb14699b5adbdbb47505d7b1",
        "51ee29caeb7011bbed6dcabf78f8317d", "f51f098221c233de5451221bfdb2332b", "f43ba95ca3eb9f7ac1849d1bcd049306",
        "3d8935cf781b00693cfa10eea46cce90", "eb0fc4c986771c4e172cc6bd5c48c5d9", "7fd1299333912582db9b6e35db3c3853",
        "887646e3826e2f5273d614a41badb8b8", "e1a31b9687b7fa03bd203a76fefc7343", "7473de83b8abca272e531eca0a5fb580",
        "d2b17fa15d9c3e865b29e911fd90602d", "02a231ce4cf5f8de31b23f75b3a90d8d",
    };
    static const char* pixel[] = {
        "7c43dd4074108affaf683aada6ddb5fb", "90c5413ac98968f6fe4923d6a1c83705", "81596a8ec24e95a5570674a8654a6332",
        "6bbfaebacae5f8806dc73ad9451aa3c9", "a3a015f52ac4119af22e53c30937575e", "f3a58f9792f6431f9c63cadbb76ed5bf",
        "a19ddc630f1b795a9d0401be8fa3f14f", "663588fdfb6c9123600c1a63874d949f", "0578108775ed2b3156d45b30649a913d",
        "da6cead50d7330e54bb8def6792804d0", "69e2c539ad38bd03c4a74c45c056fd27", "65de9c5dd23d577e9856b91de210713f",
        "5f3a5c6ba289c7e1507833d6d5885807", "677bf4bcd19d1d69d01f33ba86b9743e", "cdb7d96523b47c412d7f036923b8bbe7",
        "0eafec508a99ae16a260a8f8a6c8751c",
    };
    static const char* vertexShadow[] = {
        "c5243542257afd80d2b3fa0e1f537f55", "82dec36204efa6b003332b6ce6ad0bd8", "34ede5dfcbd66520eef214ccb5019688",
        "af25b48eda160fdd736c710474115d14", "3d9026e74e08017b0c2a8cde8fda735d", "c63e932cee5ef0fc6fac81a7eb05e21c",
        "dfeb898fa59331f705c36138dd916fc6", "a09ab22fdda2c065619eb84444db4c90", "78b90cc2f504f17f2e5388d5de408a61",
        "478b009001bc9fff3e6ef4946da1d8f8", "9f8b58eec7ac77bb517a1582863b8e41", "9566e57985421e2cd6224387267dfa37",
        "959bfeaea9d72512b7e70d54875ccd7d", "c37dc1bd807a4e6b042890a8a1992f91", "ab5288b50938e1ed853298ffd3b4896a",
        "94083583104f75690633f36fba819829", "644e54f609d32675499f6ffe350262a5", "6befb33bea43a9cdeef3aa3669b9e477",
        "17f0c897c7a55728c5cc73ba2df01e3a", "2e76e1270558e11b3a90fd75530c2555", "584b0337635e67df41a4b74aa4a1c047",
        "03e03603c90d07b5fbf86c24e66dad66", "2ffd902cf0fa8f5065b4181556ce253b", "217b504f8068ff50115f1543158c73fc",
        "ab29cda5ac7420182dff43c3d54519e7", "f5b8662c44ebac62ece4d3a5d1ae86ba", "c36f0725473ce03257b15f71be1d1c2a",
        "f211588dbd800433a8ca90b6c19b4476", "ccf1fd2b2b4ec3a4fb3eba17820de48a", "36bcb485993c5de7b94d3be28a3478c6",
        "d8b55aa8df3ab18edcb95cb4181c9479", "84fba15e9490f3f4ae171b706b7a48ae",
    };
    static const char* pixel2[] = {
        "25df7bd05aee3bf2960b0148545c86e7", "86124181a44cc9ba06a1a4a2727ea1a5", "16bccd84cf8c509c46cf12d291423838",
        "3b58dc0775ac163158ac47b0072d145f", "56ede719cbda3168e3b697f80475b582", "3467c55944114d0af40ceb62423ccc13",
        "a20b834fa0578ecf410972ee60612f01", "e67d2cbb15ff86ab8d92f6b942018377", "6fceb8a9f4aab5573c2914febca175f7",
        "88a15eafaea242795cc2285460741c8b", "41675315377d533abff0b5da90e3de3f", "391b0102fc60b60e365d769455bede3d",
        "b5cb8c32bde1d35a1cdb107288ee5cba", "cae9d6f221e0b4fdf24676fb5eed6be4", "2b29a3912dfbed831ae1ae14f9abfb1e",
        "c51fd2978e656ca7163b156fa12f1a9b",
    };
    static const char* pixel3[] = {
        "a79314c5dfbdff92e0e02e573a0d9bff", "9dcc095b3faf1ff1a2865bd4314c0039", "4f3b958c200ed98d0f7bf2de8969bd90",
        "1d82e4b3fdb1e8f6bc2ef01680374c18", "6645114f014e8300c98a709d3b1b836a", "72aef2611a73930481b8705cdb247da3",
        "458e551dea75aed25f6db7114b661a5d", "7cb05f0698af0823ade0745227b812dc", "6b37586fb6fc39749b9d68f57d1994e2",
        "499822c6ab02ffa53caad4bfbd51cc32", "9414c11cee8b298f4f5e9cf9ce13a9e0", "3edff734020c6b84cc4ffbd1283a3a7d",
        "1250ffb15ae0b7d29b17334f1e1b0732", "6220daf8f40211e8257c07c0986d7286", "9e53f315ed398fda90cd8a62486007f3",
        "58bb2f5f8bc70c632573efa323d59768", "3c1c260d67950cb358e88410ef47c5a2", "f7c33a5c68122be5d59169dbe4d713f5",
        "23f8a8291c95ea420faea9b29bf02192", "16ed39e6803dbd9d262854635a84e916", "38ca6a657592bc1a36f47bd6ef32e79b",
        "ccc3dbca3ffc7e478f621e356dc7ac8e", "c4eccab6fbdcc1a90919dd9eff390b98", "4bf4c674de117b86b63299514b343c66",
        "dcd4253d386870fbc83ebee810a4f7b8", "b57190a4172e3714b38da3dcb219f5ec", "80bf3b307ba6b4f59edfba6451020b37",
        "8c5f884b7f0295496dc499b24ae35cde", "a44f9500bb5b8bb8836ffcc667e1abe6", "7656b0cf0f00f983829bba242e3ae3b3",
        "2c56bcca2f7ed43101b6b9b2f2990dbb", "ea4fde6dca6055a0ce9fe9212a073165", "a1b433a0c8f544e33ec3d29d705fd7c0",
        "4081a69a56da611c103b544137de522a", "2e82a535461ec499184de0bb0ebd026f", "3382525c8a286d2f755094b61eecd831",
        "8554f0925f7eab6fb614c6807d4da370", "3afa1a3f65642fd2a3d6114afba2acb3", "53937818498b97ea533eeee216edd663",
        "a625600cc039d523a2be3097fd7060f9", "5f470adf47d421950fa7c0864cfb66a5", "b080d2b6727d8a0b3d7bee7640ed885e",
        "96190512c5149678a2cfc7cd792d8515", "858b611a2c3fde9ef3c2c30150c5a766", "4acccbb1e6c8ff27292c986d27807fb0",
        "3bd18ec871ccd16c959cdfedfbb01104", "70d8041b4091416e5a99cbebf546bb8d", "4e266bc8dac813e6d4d4e5dee73f22a2",
    };
    for (uint32_t i = 0; i < shaders.size(); ++i) {
        if (!SupportsTerrainPermutation(target, i, effect))
            continue;
        Blizzard::Util::MD5 hash;
        hash.Process(shaders[i].code.data(), uint32_t(shaders[i].code.size()));
        uint8_t digest[16];
        hash.Finish(digest);
        const char hex[] = "0123456789abcdef";
        std::string fingerprint;
        for (auto byte : digest) {
            fingerprint += hex[byte >> 4];
            fingerprint += hex[byte & 15];
        }
        if (fingerprint != (target == GxSh_Vertex                ? ((i & 1) ? vertexShadow[i / 4] : vertex[i / 4])
                            : effect == FragmentEffect::Terrain2 ? pixel2[i / 2]
                            : effect == FragmentEffect::Terrain3 ? pixel3[i / 2]
                                                                 : pixel[i / 2]))
            throw std::runtime_error("Terrain shader in current Data has no verified native implementation");
    }
}
