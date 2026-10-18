#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/BLPFile/blp.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
using Vec = std::array<float, 4>;
struct Fixture {
    CGxDeviceMetal& device;
    CGxShader* vertex[128]{};
    CGxShader* pixel = nullptr;
    CGxTex* blend = nullptr;
    uint32_t base;
    explicit Fixture(CGxDeviceMetal& d) : device(d), base(d.DeviceBaseMipLevel()) {
        device.RsPush();
        device.ShaderCreate(vertex, GxSh_Vertex, "Shaders\\Vertex", "Terrain", 128);
        device.ShaderCreate(&pixel, GxSh_Pixel, "Shaders\\Pixel", "Terrain0_env", 1);
        CGxTexFlags flags(GxTex_Nearest);
        device.TexCreate(
            GxTex_2d, 1, 1, 0, GxTex_Argb8888, GxTex_Argb8888, flags, nullptr,
            [](EGxTexCommand c, uint32_t, uint32_t, uint32_t, uint32_t, void*, uint32_t& stride, const void*& data) {
                static const uint32_t value = 0xFF000000;
                if (c == GxTex_Latch) {
                    stride = 4;
                    data = &value;
                }
            },
            "Cube blend reference", blend);
        for (auto state : {GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_Fog, GxRs_ScissorTest, GxRs_ClipPlaneMask,
                           GxRs_PolygonOffset, GxRs_PointSprite, GxRs_AlphaRef})
            device.RsSet(state, 0);
        device.RsSet(GxRs_BlendingMode, GxBlend_Opaque);
        device.RsSet(GxRs_ColorWrite, 15);
        GxXformSetViewport(0, 1, 0, 1, 0, 1);
    }
    ~Fixture() {
        if (device.FrameCommand())
            device.ScenePresent();
        device.DeviceSetBaseMipLevel(base);
        device.RsPop();
        device.TexDestroy(blend);
        for (auto& shader : vertex)
            device.ShaderDestroy(shader);
        device.ShaderDestroy(pixel);
    }
    MTLRenderPassDescriptor* Pass(unsigned width, unsigned height) {
        auto desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                       width:width
                                                                      height:height
                                                                   mipmapped:NO];
        desc.storageMode = MTLStorageModePrivate;
        desc.usage = MTLTextureUsageRenderTarget;
        auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:desc];
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        return pass;
    }
    void Draw(CGxTex* cube, unsigned face, float u0 = 0, float u1 = 1, float v0 = 0, float v1 = 1) {
        struct Vertex {
            float x, y, z, nx, ny, nz;
        };
        const Vertex points[] = {{-1, 1, .5f, 0, 0, 0}, {-1, -1, .5f, 0, 0, 0}, {1, 1, .5f, 0, 0, 0}, {1, -1, .5f, 0, 0, 0}};
        auto* buffer = GxBufStream(GxPoolTarget_Vertex, sizeof(Vertex), 4);
        std::memcpy(device.BufLock(buffer), points, sizeof(points));
        device.BufUnlock(buffer, 0);
        GxPrimVertexPtr(buffer, GxVBF_PN);
        std::array<Vec, 49> c{};
        for (unsigned i = 0; i < 4; ++i)
            c[i][i] = c[4 + i][i] = 1;
        // Inverse cube face transforms, in D3D's +X,-X,+Y,-Y,+Z,-Z order.
        const Vec horizontal[] = {{0, 0, -1, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {-1, 0, 0, 0}};
        const Vec vertical[] = {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, -1, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}};
        const Vec major[] = {{1, 0, 0, 0}, {-1, 0, 0, 0}, {0, 1, 0, 0}, {0, -1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1, 0}};
        for (unsigned j = 0; j < 3; ++j) {
            const unsigned source = j == 1 ? 2 : j == 2 ? 1 : 0;
            c[8][j] = horizontal[face][source] * (u1 - u0);
            c[9][j] = vertical[face][source] * (v1 - v0);
            c[10][j] = 2 * (major[face][source] + horizontal[face][source] * (u0 + u1 - 1) + vertical[face][source] * (1 - v0 - v1));
        }
        c[12] = {0, 1, 1, 0};
        c[25] = {1, 1, 1, 0};
        GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 49);
        const Vec zero{};
        GxShaderConstantsSet(GxSh_Pixel, 2, zero.data(), 1);
        device.RsSet(GxRs_VertexShader, vertex[2]);
        device.RsSet(GxRs_PixelShader, pixel);
        device.RsSet(GxRs_Texture0, cube);
        device.RsSet(GxRs_Texture1, blend);
        CGxBatch batch{GxPrim_TriangleStrip, 0, 4, 0, 3};
        device.Draw(&batch, 0);
    }
    std::vector<uint32_t> Read(unsigned width, unsigned height) {
        CiRect rect(0, 0, height, width);
        TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        std::vector<uint32_t> result(pixels.Count());
        std::memcpy(result.data(), pixels.Ptr(), result.size() * 4);
        return result;
    }
};
struct CubeData {
    uint32_t texels[6][4][64]{};
    unsigned locks = 0, unlocks = 0, latches[6][4]{};
    static void Upload(EGxTexCommand command, uint32_t w, uint32_t h, uint32_t face, uint32_t level, void* arg, uint32_t& stride,
                       const void*& data) {
        auto& self = *static_cast<CubeData*>(arg);
        if (command == GxTex_Lock)
            ++self.locks;
        if (command == GxTex_Unlock)
            ++self.unlocks;
        if (command == GxTex_Latch) {
            Require(face < 6 && level < 4 && w == (8u >> level) && h == w, "Cube callback face/level/extent mismatch");
            ++self.latches[face][level];
            stride = w * 4;
            data = self.texels[face][level];
        }
    }
};
void CheckSynthetic(Fixture& f) {
    auto& d = f.device;
    CubeData data;
    for (unsigned face = 0; face < 6; ++face)
        for (unsigned mip = 0; mip < 4; ++mip) {
            const unsigned size = 8 >> mip;
            for (unsigned y = 0; y < size; ++y)
                for (unsigned x = 0; x < size; ++x)
                    data.texels[face][mip][y * size + x] =
                        0xFF000000u | ((20 + face * 30 + x * 4) << 16) | ((30 + mip * 40 + y * 5) << 8) | (40 + x * 3 + y * 2);
        }
    CGxTex* texture = nullptr;
    CGxTexFlags flags(GxTex_NearestMipNearest);
    flags.m_wrapU = flags.m_wrapV = GxTex_Clamp;
    d.TexCreate(GxTex_CubeMap, 8, 8, 0, GxTex_Argb8888, GxTex_Argb8888, flags, &data, CubeData::Upload, "Cube update reference", texture);
    struct Close {
        CGxDeviceMetal& d;
        CGxTex* t;
        ~Close() { d.TexDestroy(t); }
    } close{d, texture};
    d.DeviceSetBaseMipLevel(0);
    for (unsigned face = 0; face < 6; ++face) {
        d.BeginFrame(f.Pass(8, 8));
        f.Draw(texture, face);
        const auto pixels = f.Read(8, 8);
        d.ScenePresent();
        for (unsigned i = 0; i < 64; ++i)
            Require(pixels[i] == data.texels[face][0][i], "Cube face orientation mismatch");
    }
    // Both draws use the same Gx handle. Replacing its pixels must not change
    // an earlier encoded draw, and the dirty rectangle applies to all faces.
    d.BeginFrame(f.Pass(16, 8));
    GxXformSetViewport(0, .5f, 0, 1, 0, 1);
    f.Draw(texture, 0);
    std::array<uint32_t, 64> old;
    std::copy_n(data.texels[0][0], 64, old.begin());
    for (unsigned face = 0; face < 6; ++face)
        for (unsigned y = 2; y < 6; ++y)
            for (unsigned x = 1; x < 5; ++x)
                data.texels[face][0][y * 8 + x] = 0xFFA01080;
    d.TexMarkForUpdate(texture, CiRect(2, 1, 6, 5), 0);
    GxXformSetViewport(.5f, 1, 0, 1, 0, 1);
    f.Draw(texture, 0);
    auto pixels = f.Read(16, 8);
    d.ScenePresent();
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; ++x) {
            Require(pixels[y * 16 + x] == old[y * 8 + x], "Cube update changed earlier draw");
            Require(pixels[y * 16 + 8 + x] == data.texels[0][0][y * 8 + x], "Cube dirty rectangle mismatch");
        }
    for (unsigned mip = 1; mip < 4; ++mip)
        for (unsigned face = 0; face < 6; ++face) {
            d.DeviceSetBaseMipLevel(mip);
            const unsigned size = 8 >> mip;
            d.BeginFrame(f.Pass(size, size));
            f.Draw(texture, face);
            pixels = f.Read(size, size);
            d.ScenePresent();
            for (unsigned i = 0; i < size * size; ++i)
                Require(pixels[i] == data.texels[face][mip][i], "Cube authored base mip mismatch");
        }
    d.DeviceSetBaseMipLevel(0);
    // Distinct authored mip colors expose incorrect gradients or level selection.
    for (unsigned face = 0; face < 6; ++face)
        for (unsigned filter = 0; filter < 6; ++filter)
            for (unsigned target : {2u, 3u}) {
                texture->m_flags.m_filter = filter;
                texture->m_flags.m_maxAnisotropy = 4;
                const bool nearest = filter == GxTex_Nearest || filter == GxTex_NearestMipNearest;
                const float lod = filter <= GxTex_Linear ? 0 : std::log2(8.0f / target);
                const bool trilinear = filter >= GxTex_LinearMipLinear;
                const unsigned low = unsigned(trilinear ? std::floor(lod) : std::round(lod));
                const unsigned high = trilinear ? unsigned(std::ceil(lod)) : low;
                const float blend = trilinear ? lod - low : 0;
                d.BeginFrame(f.Pass(target, target));
                f.Draw(texture, face);
                pixels = f.Read(target, target);
                d.ScenePresent();
                for (unsigned y = 0; y < target; ++y)
                    for (unsigned x = 0; x < target; ++x) {
                        float expected[3]{};
                        for (unsigned level : {low, high}) {
                            const unsigned size = 8 >> level;
                            const float u = (x + 0.5f) / target, v = (y + 0.5f) / target;
                            const float px = u * size - 0.5f, py = v * size - 0.5f;
                            const int ix = int(std::floor(nearest ? u * size : px)), iy = int(std::floor(nearest ? v * size : py));
                            const float fx = nearest ? 0 : px - std::floor(px), fy = nearest ? 0 : py - std::floor(py);
                            const float weight = low == high ? 0.5f : level == low ? 1 - blend : blend;
                            for (unsigned sy = 0; sy < 2; ++sy)
                                for (unsigned sx = 0; sx < 2; ++sx) {
                                    const auto value = data.texels[face][level][std::clamp(iy + int(sy), 0, int(size) - 1) * size +
                                                                                std::clamp(ix + int(sx), 0, int(size) - 1)];
                                    const float w = weight * (sx ? fx : 1 - fx) * (sy ? fy : 1 - fy);
                                    for (unsigned c = 0; c < 3; ++c)
                                        expected[c] += ((value >> (c * 8)) & 255) * w;
                                }
                        }
                        for (unsigned c = 0; c < 3; ++c)
                            if (std::abs(int((pixels[y * target + x] >> (c * 8)) & 255) - int(std::lround(expected[c]))) > 2)
                                throw std::runtime_error("Cube mip filter mismatch face " + std::to_string(face) + " filter " +
                                                         std::to_string(filter) + " target " + std::to_string(target));
                    }
            }
    Require(data.locks == data.unlocks, "Cube callback lock imbalance");
    for (unsigned face = 0; face < 6; ++face)
        for (unsigned mip = 0; mip < 4; ++mip)
            Require(data.latches[face][mip] >= 2, "Missing cube face/mip callback");
    d.DeviceSetBaseMipLevel(0);
    // Constant directions exactly on/close to each face edge. Face-local wrap
    // and clamp are visibly distinct from seamless neighbor-face filtering.
    for (unsigned face = 0; face < 6; ++face)
        for (unsigned wrap = 0; wrap < 2; ++wrap)
            for (unsigned filter = 0; filter < 6; ++filter) {
                texture->m_flags.m_wrapU = texture->m_flags.m_wrapV = wrap;
                texture->m_flags.m_filter = filter;
                texture->m_flags.m_maxAnisotropy = 4;
                for (float u : {0.0001f, 0.9999f}) {
                    const float v = .3125f, px = u * 8 - .5f, py = v * 8 - .5f;
                    const bool nearest = filter == GxTex_Nearest || filter == GxTex_NearestMipNearest;
                    const int x0 = int(std::floor(nearest ? u * 8 : px)), y0 = int(std::floor(nearest ? v * 8 : py));
                    const float fx = nearest ? 0 : px - std::floor(px), fy = nearest ? 0 : py - std::floor(py);
                    float expected[3]{};
                    for (unsigned y = 0; y < 2; ++y)
                        for (unsigned x = 0; x < 2; ++x) {
                            const int sx = wrap ? (x0 + int(x) + 8) % 8 : std::clamp(x0 + int(x), 0, 7),
                                      sy = wrap ? (y0 + int(y) + 8) % 8 : std::clamp(y0 + int(y), 0, 7);
                            const auto value = data.texels[face][0][sy * 8 + sx];
                            const float weight = (x ? fx : 1 - fx) * (y ? fy : 1 - fy);
                            for (unsigned c = 0; c < 3; ++c)
                                expected[c] += ((value >> (c * 8)) & 255) * weight;
                        }
                    d.BeginFrame(f.Pass(4, 4));
                    f.Draw(texture, face, u, u, v, v);
                    pixels = f.Read(4, 4);
                    d.ScenePresent();
                    for (auto value : pixels)
                        for (unsigned c = 0; c < 3; ++c)
                            Require(std::abs(int((value >> (c * 8)) & 255) - int(std::lround(expected[c]))) <= 1,
                                    "Cube face-local edge filtering mismatch");
                }
            }
}
std::array<BLP::Image, 6> ReadFaces(const char* name) {
    SFileImpl* file = nullptr;
    Require(SFile::OpenEx(nullptr, name, 0, &file), "Cannot open cube reference BLP");
    struct Close {
        SFileImpl* p;
        ~Close() { SFileCloseFile(p); }
    } close{file};
    std::vector<uint8_t> bytes(SFileGetFileSize(file, nullptr));
    Require(bytes.size() >= 148 && SFileReadFile(file, bytes.data(), uint32_t(bytes.size()), nullptr, nullptr, 0), "Cannot read cube BLP");
    auto word = [&](unsigned offset) {
        uint32_t value;
        std::memcpy(&value, bytes.data() + offset, 4);
        return value;
    };
    Require(word(0) == 0x32504C42 && word(12) == 6 * word(16) && (bytes[8] == 2 || bytes[8] == 3), "Unexpected reference cube encoding");
    const bool bc = bytes[8] == 2;
    Require(!bc || (bytes[9] == 0 && bytes[10] == 0), "Expected original BC1 cube");
    CBLPFile source;
    Require(source.Source(bytes.data(), bytes.size()), "Original BLP loader rejected cube payload");
    Require(source.m_numLevels == (bc ? 6u : 8u), "Cube usable mip chain mismatch");
    if (bc) {
        auto truncated = bytes;
        const uint32_t shortSize = 47;
        std::memcpy(truncated.data() + 84 + 5 * 4, &shortSize, 4);
        CBLPFile invalid;
        Require(!invalid.Source(truncated.data(), truncated.size()), "Truncated usable cube mip accepted");
    }
    const unsigned order[] = {0, 2, 4, 5, 3, 1};
    std::array<BLP::Image, 6> faces;
    for (unsigned face = 0; face < 6; ++face) {
        auto& image = faces[face];
        image.format = bc ? BLP::Format::BC1 : BLP::Format::BGRA8;
        for (unsigned level = 0, size = word(16);; ++level, size = std::max(1u, size / 2)) {
            const unsigned offset = word(20 + level * 4), length = word(84 + level * 4), rows = bc ? (size + 3) / 4 : size,
                           rowBytes = bc ? ((size + 3) / 4) * 8 : size * 4;
            Require(level < 16 && offset <= bytes.size() && length <= bytes.size() - offset && length >= rows * rowBytes * 6,
                    "Invalid cube reference mip range");
            BLP::Mip mip{size, size, rowBytes, std::vector<uint8_t>(rows * rowBytes)};
            for (unsigned row = 0; row < rows; ++row)
                std::memcpy(mip.bytes.data() + row * rowBytes, bytes.data() + offset + (row * 6 + order[face]) * rowBytes, rowBytes);
            image.mips.push_back(std::move(mip));
            if (size == (bc ? 4u : 1u))
                break;
        }
    }
    return faces;
}
} // namespace
void CheckGameCubeTextures() {
    Fixture f(*static_cast<CGxDeviceMetal*>(g_theGxDevicePtr));
    CheckSynthetic(f);
    unsigned checked = 0;
    for (const char* name : {"TILESET\\Terrain Cube Maps\\TCB_CrystalSong_A.blp", "TILESET\\Terrain Cube Maps\\TCB_CrystalSong_B.blp",
                             "TILESET\\Terrain Cube Maps\\TCB_SkyBlue_A.blp", "TILESET\\Terrain Cube Maps\\oilslickenvA.blp"}) {
        auto reference = ReadFaces(name);
        CStatus status;
        CGxTexFlags flags(GxTex_NearestMipNearest);
        flags.m_wrapU = flags.m_wrapV = GxTex_Clamp;
        auto* handle = TextureCreate(name, flags, &status, 1);
        Require(handle != nullptr, "Services cube creation failed");
        struct Close {
            CTexture* p;
            ~Close() { HandleClose(reinterpret_cast<HOBJECT>(p)); }
        } close{handle};
        auto* texture = TextureGetGxTex(handle, 1, &status);
        Require(texture && status.IsEmpty() && texture->m_target == GxTex_CubeMap && texture->m_width == 128,
                "Services cube target/extent mismatch");
        for (unsigned level = 0; level < reference[0].mips.size(); ++level) {
            f.device.DeviceSetBaseMipLevel(level);
            for (unsigned face = 0; face < 6; ++face) {
                auto expected = reference[face].DecodeBGRA(level);
                const auto size = expected.width;
                f.device.BeginFrame(f.Pass(size, size));
                f.Draw(texture, face);
                const auto pixels = f.Read(size, size);
                f.device.ScenePresent();
                for (unsigned i = 0; i < size * size; ++i)
                    for (unsigned c = 0; c < 3; ++c) {
                        const int tolerance = reference[face].format == BLP::Format::BC1 ? 2 : 0;
                        if (std::abs(int((pixels[i] >> (c * 8)) & 255) - int(expected.bytes[i * 4 + c])) > tolerance)
                            throw std::runtime_error(std::string("Services cube pixels mismatch ") + name + " face " +
                                                     std::to_string(face) + " mip " + std::to_string(level) + " pixel " +
                                                     std::to_string(i));
                    }
                ++checked;
            }
        }
        f.device.DeviceSetBaseMipLevel(0);
    }
    std::fprintf(stderr,
                 "Cube textures: six-face orientation, immutable local updates, base mips, six filters and face-edge wrap/clamp; %u real "
                 "Services BLP face/mip images verified\n",
                 checked);
}
