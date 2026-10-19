#include <cstdlib>
#include "Tools/AppleBootstrap/MetalProbe.h"
#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>

void CheckGxGeometry(GxMetal::TextureDevice& gpu);
void CheckGxDevice(const char* focus = nullptr);
void CheckGxFilter4(GxMetal::TextureDevice& gpu);
void CheckUIShader(GxMetal::TextureDevice& gpu, const GxMetal::UIShader& ui);

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

BLP::Image Block(BLP::Format format, std::initializer_list<uint8_t> bytes) {
    BLP::Image image;
    image.format = format;
    image.mips.push_back({4, 4, uint32_t(bytes.size()), bytes});
    return image;
}

GxMetal::TextureDrawState NearestState() {
    GxMetal::TextureDrawState state;
    state.filter = GxTex_NearestMipNearest;
    return state;
}

std::vector<uint8_t> Render(GxMetal::TextureDevice& gpu, id<MTLTexture> texture, float level,
    const GxMetal::TextureDrawState& state = NearestState(), uint32_t widthOverride = 0, uint32_t heightOverride = 0,
    GxMetal::QuadParameters parameters = {}) {
    const uint32_t width = widthOverride ? widthOverride : std::max(1u, uint32_t(texture.width) >> uint32_t(level));
    const uint32_t height = heightOverride ? heightOverride : std::max(1u, uint32_t(texture.height) >> uint32_t(level));
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
        width:width height:height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    id<MTLTexture> target = [gpu.device newTextureWithDescriptor:descriptor];
    const size_t stride = (size_t(width) * 4 + 255) & ~size_t(255);
    id<MTLBuffer> readback = [gpu.device newBufferWithLength:stride * height options:MTLResourceStorageModeShared];
    Require(target && readback, "Metal readback allocation failed");
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(32.0 / 255, 64.0 / 255, 96.0 / 255, 1);
    id<MTLCommandBuffer> command = [gpu.queue commandBuffer];
    parameters.explicitMip = 1;
    parameters.mipLevel = level;
    gpu.Draw(command, pass, texture, parameters, state);
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    [blit copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
        sourceSize:MTLSizeMake(width, height, 1) toBuffer:readback destinationOffset:0 destinationBytesPerRow:stride destinationBytesPerImage:stride * height];
    [blit endEncoding];
    [command commit];
    // A diagnostic readback deliberately waits; the runtime draw/upload path does not.
    [command waitUntilCompleted];
    if (command.status != MTLCommandBufferStatusCompleted) {
        throw std::runtime_error(command.error.localizedDescription.UTF8String ?: "Metal command failed");
    }
    std::vector<uint8_t> pixels(size_t(width) * height * 4);
    for (uint32_t y = 0; y < height; ++y) {
        std::memcpy(pixels.data() + size_t(y) * width * 4, static_cast<const uint8_t*>(readback.contents) + y * stride, width * 4);
    }
    return pixels;
}

void CheckDecodeCases(GxMetal::TextureDevice& gpu) {
    // Endpoint extremes, transparent BC1, explicit BC2 alpha and both BC3
    // interpolation modes. Expected values are independent known block results.
    auto bc1 = Block(BLP::Format::BC1, {0, 0, 255, 255, 0xE4, 0xE4, 0xE4, 0xE4});
    const auto colors = bc1.DecodeBGRA(0).bytes;
    const uint8_t expected[] = {0, 0, 0, 255, 255, 255, 255, 255, 127, 127, 127, 255, 0, 0, 0, 0};
    Require(!std::memcmp(colors.data(), expected, sizeof(expected)), "BC1 transparent block decode mismatch");
    auto bc2 = Block(BLP::Format::BC2, {0x10, 0xF8, 0x10, 0xF8, 0x10, 0xF8, 0x10, 0xF8, 255, 255, 0, 0, 0, 0, 0, 0});
    const auto explicitAlpha = bc2.DecodeBGRA(0).bytes;
    Require(explicitAlpha[3] == 0 && explicitAlpha[7] == 17 && explicitAlpha[11] == 136 && explicitAlpha[15] == 255 && explicitAlpha[0] == 255,
        "BC2 explicit alpha/endpoint decode mismatch");
    auto bc3 = Block(BLP::Format::BC3, {0, 255, 0x88, 0xC6, 0xFA, 0x88, 0xC6, 0xFA, 255, 255, 0, 0, 0, 0, 0, 0});
    const auto sixAlpha = bc3.DecodeBGRA(0).bytes;
    const uint8_t expectedAlpha[] = {0, 255, 51, 102, 153, 204, 0, 255};
    for (unsigned i = 0; i < 8; ++i) { Require(sixAlpha[i * 4 + 3] == expectedAlpha[i], "BC3 six-alpha mode mismatch"); }
    bc3.mips[0].bytes[0] = 255; bc3.mips[0].bytes[1] = 0;
    const auto eightAlpha = bc3.DecodeBGRA(0).bytes;
    const uint8_t expectedEight[] = {255, 0, 219, 182, 146, 109, 73, 36};
    for (unsigned i = 0; i < 8; ++i) { Require(eightAlpha[i * 4 + 3] == expectedEight[i], "BC3 eight-alpha mode mismatch"); }
    for (const auto* image : {&bc1, &bc2, &bc3}) {
        const auto pixels = Render(gpu, gpu.Upload(*image, true), 0);
        Require(pixels == image->DecodeBGRA(0).bytes, "Expanded texture differs after Metal draw");
    }
    BLP::Image blendImage;
    blendImage.mips.push_back({1, 1, 4, {200, 100, 50, 128}});
    auto state = NearestState();
    state.blend = GxBlend_Alpha;
    const auto blended = Render(gpu, gpu.Upload(blendImage), 0, state);
    const unsigned background[] = {96, 64, 32, 255};
    for (unsigned c = 0; c < 4; ++c) {
        const int expected = int(std::lround((blendImage.mips[0].bytes[c] * 128.0 + background[c] * 127.0) / 255));
        Require(std::abs(int(blended[c]) - expected) <= 1, "Straight alpha blend factor mismatch");
    }
    BLP::Image filtered;
    filtered.mips.push_back({2, 2, 8, {0, 0, 0, 255, 0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255}});
    state.blend = GxBlend_Opaque; state.filter = GxTex_Linear;
    const auto average = Render(gpu, gpu.Upload(filtered), 0, state, 1, 1);
    Require(average[0] == 64 && average[1] == 64 && average[2] == 64 && average[3] == 255, "Bilinear center sampling mismatch");
}

void CheckGxStates(GxMetal::TextureDevice& gpu) {
    BLP::Image source;
    source.mips.push_back({1, 1, 4, {40, 80, 160, 128}});
    id<MTLTexture> texture = gpu.Upload(source);
    const double destination[] = {96, 64, 32, 255};
    auto state = NearestState();
    state.blendFactor = 0.375f;
    // Independent scalar forms of the Gx blend equations, including alpha.
    for (int mode = 0; mode < GxBlends_Last; ++mode) {
        state.blend = static_cast<EGxBlend>(mode);
        const auto pixels = Render(gpu, texture, 0, state);
        for (unsigned c = 0; c < 4; ++c) {
            const double s = source.mips[0].bytes[c], d = destination[c], a = 128.0 / 255;
            double expected;
            switch (state.blend) {
                case GxBlend_Opaque: case GxBlend_AlphaKey: expected = s; break;
                case GxBlend_Alpha: expected = s * a + d * (1 - a); break;
                case GxBlend_Add: expected = s * a + d; break;
                case GxBlend_Mod: expected = s * d / 255; break;
                case GxBlend_Mod2x: expected = 2 * s * d / 255; break;
                case GxBlend_ModAdd: expected = s * d / 255 + d; break;
                case GxBlend_InvSrcAlphaAdd: expected = s * (1 - a) + d; break;
                case GxBlend_InvSrcAlphaOpaque: expected = s * (1 - a); break;
                case GxBlend_SrcAlphaOpaque: expected = s * a; break;
                case GxBlend_NoAlphaAdd: expected = s + d; break;
                case GxBlend_ConstantAlpha: expected = s * (95.0 / 255) + d * (160.0 / 255); break;
                default: throw std::runtime_error("Unhandled Gx blend test");
            }
            Require(std::abs(int(pixels[c]) - int(std::lround(std::min(255.0, expected)))) <= 1, "Gx blend RGBA result mismatch");
        }
    }
    state = NearestState();
    state.blend = GxBlend_AlphaKey;
    for (uint32_t threshold : {127u, 128u, 129u, 224u}) {
        state.alphaRef = threshold;
        const auto pixels = Render(gpu, texture, 0, state);
        for (unsigned c = 0; c < 4; ++c) {
            Require(pixels[c] == (threshold <= 128 ? source.mips[0].bytes[c] : uint8_t(destination[c])), "Gx alpha-test threshold mismatch");
        }
    }
    state = NearestState();
    const unsigned masks[] = {2, 4, 1, 8}; // BGRA byte order vs Gx mask bits
    for (uint32_t mask : {0u, 1u, 2u, 4u, 8u, 15u}) {
        state.colorWrite = mask;
        const auto pixels = Render(gpu, texture, 0, state);
        for (unsigned c = 0; c < 4; ++c) {
            Require(pixels[c] == (mask & masks[c] ? source.mips[0].bytes[c] : uint8_t(destination[c])), "Gx channel-write mask mismatch");
        }
    }
    BLP::Image mips;
    mips.mips.push_back({2, 2, 8, {0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255}});
    mips.mips.push_back({1, 1, 4, {255, 0, 0, 255}});
    texture = gpu.Upload(mips);
    state = NearestState();
    for (int filter = GxTex_Nearest; filter < GxTexFilters_Last; ++filter) {
        state.filter = static_cast<EGxTexFilter>(filter);
        state.anisotropy = filter == GxTex_Anisotropic ? 16 : 1;
        const auto pixels = Render(gpu, texture, 0.75f, state, 1, 1);
        const uint8_t blue = filter <= GxTex_Linear ? 0 : (filter <= GxTex_LinearMipNearest ? 255 : 191);
        const uint8_t red = filter <= GxTex_Linear ? 255 : (filter <= GxTex_LinearMipNearest ? 0 : 64);
        if (pixels[0] != blue || pixels[2] != red || pixels[3] != 255) {
            throw std::runtime_error("Gx mip filter " + std::to_string(filter) + " mismatch: B/R " +
                std::to_string(pixels[0]) + "/" + std::to_string(pixels[2]));
        }
    }
    BLP::Image wrap;
    wrap.mips.push_back({2, 2, 8, {1, 2, 3, 255, 11, 12, 13, 255, 21, 22, 23, 255, 31, 32, 33, 255}});
    texture = gpu.Upload(wrap);
    GxMetal::QuadParameters coordinates;
    coordinates.uvScale = {0, 0}; coordinates.uvOffset = {1.25f, -0.25f};
    state = NearestState();
    for (unsigned u = 0; u < 2; ++u) {
        for (unsigned v = 0; v < 2; ++v) {
            state.wrapU = static_cast<EGxTexWrapMode>(u); state.wrapV = static_cast<EGxTexWrapMode>(v);
            const auto pixels = Render(gpu, texture, 0, state, 1, 1, coordinates);
            const size_t offset = (v * 2 + (u ? 0 : 1)) * 4;
            Require(!std::memcmp(pixels.data(), wrap.mips[0].bytes.data() + offset, 4), "Gx wrap/clamp addressing mismatch");
        }
    }
}
}

bool AppleBootstrap::CheckMetal(const BLP::Image& image, const GxMetal::UIShader& ui, std::string& report) {
    std::ostringstream out;
    @autoreleasepool {
        try {
            GxMetal::TextureDevice gpu(MTLCreateSystemDefaultDevice());
            out << "Metal device: " << gpu.device.name.UTF8String << "; BC support: " << gpu.SupportsBC() << '\n';
            if (const char* selection = std::getenv("WOW_METAL_CHECK")) {
                if (!std::strcmp(selection, "gamma")) { CheckGxDevice(selection); report = "Original presentation gamma checks passed.\n"; return true; }
                if (!std::strcmp(selection, "cursor")) { CheckGxDevice(selection); report = "Original software cursor rendering checks passed.\n"; return true; }
                if (!std::strcmp(selection, "memory")) { CheckGxDevice(selection); report = "Metal cache eviction preserved active and in-flight draws.\n"; return true; }
                if (!std::strcmp(selection, "weather-world")) { CheckGxDevice(selection); report = "Original weather/world integration checks passed.\n"; return true; }
                if (!std::strcmp(selection, "weather")) { CheckGxDevice(selection); report = "Native weather shader checks passed.\n"; return true; }
                if (!std::strcmp(selection, "msaa")) {
                    CheckGxDevice(selection);
                    report = "Native MSAA capability and supported paths checked; per-device coverage and skipped paths are listed above.\n";
                    return true;
                }
                if (!std::strcmp(selection, "queries")) {
                    CheckGxDevice(selection);
                    report = "Metal occlusion query capability and supported-device counts/lifetimes checked.\n";
                    return true;
                }
                if (!std::strcmp(selection, "render-target")) {
                    CheckGxDevice(selection);
                    report = "Original render-target producers, copies, clears and recreation passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "atlas")) {
                    CheckGxDevice(selection);
                    report = "Original Services icon atlas creation, updates and recreation passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "fixed")) {
                    CheckGxDevice(selection);
                    report = "Original fixed-function color/depth and water-ripple GPU checks passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "detail-shaders")) {
                    CheckGxDevice(selection);
                    report = "Original detail-doodad lighting, baked colors and dynamic/banded R32F/PCF shadows passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "terrain-shaders")) {
                    CheckGxDevice(selection);
                    report = "Original terrain layers, lighting, R32F/PCF receivers and shadow generation passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "model-depth")) {
                    CheckGxDevice(selection);
                    report = "Original model normalized polygon offset passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "model-clipping")) {
                    CheckGxDevice(selection);
                    report = "Original model homogeneous clip planes and clipped geometry passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "model-shaders")) {
                    CheckGxDevice(selection);
                    report = "Original model shader variants/materials and reflection-only coordinates passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "model-projected-shadows")) {
                    CheckGxDevice(selection);
                    report = "Original projected model materials and R32F shadow receivers passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "model-colored-shadows")) {
                    CheckGxDevice(selection);
                    report = "Original colored model materials and R32F shadow receivers passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "wmo-shaders")) {
                    CheckGxDevice(selection);
                    report = "Original WMO materials/dynamic-banded shadows and shared M2 receiver regression passed (focused run).\n";
                    return true;
                }
                if (!std::strcmp(selection, "ffx")) {
                    CheckGxFilter4(gpu); CheckGxDevice(selection);
                    report = "Original FFX shaders/passes and nether/fog shader GPU checks passed (focused run).\n";
                    return true;
                }
                Require(std::strcmp(selection, "models") == 0 || std::strcmp(selection, "ui") == 0, "Unknown WOW_METAL_CHECK selection");
                CheckGxDevice(selection);
                report = std::strcmp(selection, "ui") == 0 ? "Original UI/console resource, input and GPU checks passed (focused run).\n" : "Original model/character resource and GPU checks passed (focused run).\n";
                return true;
            }
            CheckDecodeCases(gpu);
            CheckGxStates(gpu);
            CheckGxGeometry(gpu);
            CheckUIShader(gpu, ui);
            CheckGxDevice();
            CheckGxFilter4(gpu);
            out << "Original FFXBox4/FFXGauss4 native primitives: four independent textures/coordinates/samplers, RGB versus RGBA/vertex-alpha semantics, alpha test and two-pass intermediate sampling passed.\n";
            out << "Original FFX PassBox4/PassGauss4 through Services/Gx: custom PCT4 streams, four texture bindings, fixed vertex matrices, downsampling pixel centers, padded targets, shader cache and caller state restoration passed.\n";
            out << "Original FFX Glow composition: scene/blur coordinates, blue/alpha weights, opaque alpha, saturation and ordered Box4/Gauss4/Glow pass chain passed.\n";
            out << "Original FFX GlowWave: generated RG wave texture, time-dependent PCT3 coordinates, wrap sampling, pixel-constant scales and displaced scene/blur composition passed.\n";
            out << "Original FFX Init/Start/Finish and EffectGlow: CVar selection, default-framebuffer capture, both effect modes, disable/re-enable and target resizing passed.\n";
            out << "Original FFXDeath: independent scene/blur sampling, original luminance and midtone tint, opaque alpha, packed inputs, DayNight glow, Box4/Gauss4/Death chain, CVar toggles, effect switching and target resizing passed.\n";
            out << "Native shadow generation: all 90 vertex / 16 pixel handles, 360 skin/light-view/depth/UV/alpha checks, R32F precision, regional clears, exact copies and D3D9 sampling channels passed.\n";
            out << "Native Terrain/Terrain1: 32 vertex and 16 pixel variants, independent five-texture sampling, old/big alpha, layer lighting, specular and fog references passed.\n";
            out << "Real Elwynn/Northrend/Outland terrain: 256 chunks per tile, original hole indices, specular BLP mips, packed/RGBA blend maps, vertex colors, 51 Crystalsong cube-reflection chunks, original MH2O/MCLQ geometry/materials, animated water/ocean/green magma, resident geometry reuse and one render pass after residency passed.\n";
            out << "Native LiquidSystem: nine vertex and three pixel variants for water, water without specular and magma, original independent UVs, non-normalized normals, three local lights, fog and state alpha references passed.\n";
            out << "Native WMO materials: seven classic/unified vertex families and seven pixel families, 0..4 local lights, baked/dynamic color, direct reflection coordinates, composite blending, fog, alpha and R32F/PCF dynamic/banded shadow references passed.\n";
            out << "Original WMO UnifiedRender: actual altar/Stormwind/Dalaran groups, original geometry upload, material states and two-pass transition batches, one resident render pass, repeated pixels and recreation passed.\n";
            out << "Original WMO liquids: authored Stormwind/Dalaran/Blackrock grids, original material banks, depth-color textures, animated MPQ frames and geometry factory; visible animation, resident geometry reuse and recreation passed.\n";
            out << "Original WMO roots/groups: Altar of Storms, Stormwind and Dalaran, 15 real groups including 12 liquid grids, aligned geometry/BSP, authored batches/colors/liquid heights, material textures, async and preload reads, failed-read cleanup/retry and cancellation passed.\n";
            out << "Original Services async file queue: MPQ DBC/script/model/terrain/WMO reads, game-thread completion, failed reads, cancellation and worker recreation passed.\n";
            out << "Original M2 scene: AnimationData DBC, bounded model bounds index, async camera/skin loading, cinematic animation, DBC-selected wolf skins/geosets, water elemental reflections, motorcycle layered materials, original fireball particles, Rejuvenation ribbons/sphere emitters, authored projected spells on an explicit receiver, and scene/cache recreation passed.\n";
            out << "Native model/particle/projected materials: 150 vertex variants, 0/1/4-bone skinning, 0..4 local lights, two UV matrices, view-space projection, sphere reflection, independent samplers, additive/multiplied/combined layer materials, fog and both alpha-test paths passed; shadow receiver permutations remain unsupported.\n";
            out << "Original Services TextureCreate: cached handles, bounded BLP mips, oversized asynchronous palette uploads, cancellation and mip recreation passed.\n";
            out << "Gx packed UNORM: RGB565/ARGB1555/ARGB4444, original BGRA truncation, point/linear filtering, partial uploads and Services palette mips passed.\n";
            out << "Original Event scheduler: native nonblocking steps, dispatch order, timers, queued messages, input state and context recreation passed.\n";
            out << "Original CWorld lifecycle: Azeroth map loading/destruction twice, terrain/WMO/doodad/horizon population, original world update/scene rendering and complete teardown, live/detached weather callbacks and portrait notifications passed; player/network/UI integration remains pending.\n";
            out << "Native unlit fixed-function: six independent RGB/alpha operations, up to four textures, UV selection/transforms, diffuse color, vertex fog interpolation and alpha boundaries passed.\n";
            out << "Native Gx factory, UI shader creation/binding, streaming-buffer wrap, immutable texture updates, partial-origin callbacks, pixel/depth readback, screenshots and exact ARGB4444 font textures passed.\n";
            out << "Gx color render targets: offscreen draw/sample ordering, default clear preservation, scratch depth reuse, target destruction and host resize passed.\n";
            out << "Gx surface copies: default versus explicit source, exact partial RGBA copies, nearest/linear scaling, offset destination, invalid bounds and same-frame mip sampling passed.\n";
            out << "Gx regional clears: independent color/depth masks, viewport/scissor rounding and deferral, offscreen Y origin and preservation outside shared depth bounds passed.\n";
            out << "Gx mip recreation: producer/cache notifications, authored mip selection, retained in-flight images/attachments and original glyph reupload passed.\n";
            out << "Original MPQ font rendered through Gxu/FreeType: multiline text, embedded color, AA/monochrome, normal/thick outlines, shadow, resize/restore and font/texture-cache recreation passed.\n";
            out << "BC endpoint/alpha cases, BGRA channel order, straight alpha blending and bilinear sampling passed.\n";
            out << "Gx: 12 RGBA blend modes, alpha-test equality/clip, channel masks, six mip filters and independent U/V addressing passed.\n";
            out << "Gx geometry: 13 unskinned packed layouts, indexed/nonindexed draws, Tempest transforms, BGRA vertex colors, depth comparisons/write masks, opposite cull modes and scissors passed.\n";
            out << "Original UI BLS recognized (vs_" << ui.VertexMajorVersion() << "_0 / ps_" << ui.PixelMajorVersion()
                << "_0): both native vertex permutations and constant-register mapping passed.\n";
            for (bool expanded : {false, true}) {
                id<MTLTexture> texture = gpu.Upload(image, expanded);
                unsigned maxRGB = 0, maxAlpha = 0;
                size_t differences = 0;
                for (size_t level = 0; level < image.mips.size(); ++level) {
                    const auto actual = Render(gpu, texture, uint32_t(level));
                    const auto expected = image.DecodeBGRA(level).bytes;
                    Require(actual.size() == expected.size(), "Metal mip readback extent mismatch");
                    for (size_t i = 0; i < actual.size(); ++i) {
                        const unsigned delta = std::abs(int(actual[i]) - expected[i]);
                        if (i % 4 == 3) { maxAlpha = std::max(maxAlpha, delta); } else { maxRGB = std::max(maxRGB, delta); }
                        differences += delta != 0;
                    }
                }
                out << (expanded ? "Forced BGRA expansion" : "Device-native upload") << ": " << image.mips.size()
                    << " authored mips drawn/read back; max RGB/alpha delta " << maxRGB << '/' << maxAlpha
                    << ", differing components " << differences << '\n';
                const bool hardwareBC = !expanded && gpu.SupportsBC() && image.format != BLP::Format::BGRA8;
                // Observed M2 Pro BC3 interpolation uses quantized weights:
                // endpoints 253/8, index 5 gives 111, vs 113 from (3a+4b)/7.
                // Rounding a weight to 1/64 can move an 8-bit result by 2.
                // This bounds this upload check, not Windows effect equivalence.
                // Expansion has no hardware BC rounding and must be exact.
                Require(maxRGB <= 2u * hardwareBC && maxAlpha <= 2u * hardwareBC, "Metal texture samples differ beyond observed BC rounding");
            }
            out << "Hardware BC interpolation differences are reported; original Windows GPU comparison remains pending.\n";
            report = out.str();
            return true;
        } catch (const std::exception& error) {
            report = out.str() + "Metal check failed: " + error.what() + "\n";
            return false;
        }
    }
}
