#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/NativeHost.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Gxu/GxuFontFace.h"
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Gxu/GxuFontUtil.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Services/Texture.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <array>
#include <cmath>
#include <vector>

namespace {
struct FontScope {
    CGxFont* font = nullptr;
    CGxString* string = nullptr;
    CGxStringBatch* batch = nullptr;
    FontScope() { GxuFontInitialize(); }
    ~FontScope() {
        if (batch) { GxuFontDestroyBatch(batch); }
        if (string) { GxuFontDestroyString(string); }
        if (font) { GxuFontDestroyFont(font); }
        GxuFontShutdown();
    }
};

void CheckLocalizedAdvances() {
    // Authored ZYKai_T hmtx values, unitsPerEm=256. The font sets isFixedPitch
    // despite proportional Latin metrics; hhea.advanceWidthMax=524 must not
    // replace them. Both original glyph modes use the same advances.
    for (uint32_t flags : {0u, 2u}) {
        FontScope sample;
        if (!GxuFontCreateFont("Fonts\\ZYKai_T.TTF", .1f, sample.font, flags)) {
            throw std::runtime_error("Cannot load the original localized UI font");
        }
        auto* face = FontFaceGetFace(sample.font->m_fontFaceHandle);
        if (face->units_per_EM != 256 || !(face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)) {
            throw std::runtime_error("Localized font metric reference does not match mounted Data");
        }
        for (unsigned size : {12u, 24u, 32u, 48u, 64u, 96u, 128u}) {
            sample.font->ClearGlyphs(); sample.font->m_pixelSize = size;
            for (auto metric : {std::pair<uint32_t, unsigned>{'A', 176}, {'p', 133}, {0x4f60, 256}}) {
                const float expected = (metric.second * size / 256) + 1;
                GLYPHBITMAPDATA glyph{};
                if (!sample.font->GetGlyphData(&glyph, metric.first)) {
                    throw std::runtime_error("Cannot render original localized glyph");
                }
                const float advance = glyph.m_glyphAdvance;
                if (advance != expected || face->glyph->linearHoriAdvance != metric.second) {
                    std::fprintf(stderr, "Localized metrics: flags=%u size=%u code=%x advance=%g expected=%g linear=%ld expected=%u\n",
                        flags, size, metric.first, advance, expected,
                        face->glyph->linearHoriAdvance, metric.second);
                    throw std::runtime_error("Original localized per-glyph advance was replaced by a global width");
                }
            }
        }
    }
}

void CheckCoveragePrecision() {
    FontScope sample;
    if (!GxuFontCreateFont("Fonts\\ARIALN.TTF", 24.0f / g_heightPixels, sample.font, 0))
        throw std::runtime_error("Cannot load coverage reference font");
    std::array<bool, 256> levels{};
    for (uint32_t code : {'S', 'a', 'W', 'g'}) {
        CHARCODEDESC* glyph = sample.font->NewCodeDesc(code);
        if (!glyph) throw std::runtime_error("Missing coverage reference glyph");
        auto& cache = sample.font->m_textureCache[glyph->m_textureNumber];
        const uint32_t size = sample.font->GetTextureSize();
        uint32_t stride = 0; const void* texels = nullptr;
        TEXTURECACHE::TextureCallback(GxTex_Latch, size, size, 0, 0, &cache, stride, texels);
        if (stride != size * 4 || TextureGetGxTex(cache.m_texture, 1, nullptr)->m_format != GxTex_Argb8888)
            throw std::runtime_error("Font atlas did not retain 8-bit coverage");
        const auto& bitmap = glyph->m_bitmapData;
        const auto* source = static_cast<const uint8_t*>(bitmap.m_data);
        const auto* pixels = static_cast<const uint32_t*>(texels);
        for (uint32_t y = 0; y < bitmap.m_glyphHeight; ++y) {
            for (uint32_t x = 0; x < bitmap.m_glyphWidth; ++x) {
                const uint8_t alpha = source[y * bitmap.m_glyphPitch + x];
                const uint32_t pixel = pixels[(glyph->m_rowNumber * sample.font->m_cellHeight + bitmap.m_yStart + y) * size + glyph->m_glyphStartPixel + x];
                if (pixel != (uint32_t(alpha) << 24 | 0xFFFFFF))
                    throw std::runtime_error("Font atlas quantized or moved FreeType coverage");
                levels[alpha] = true;
            }
        }
    }
    size_t count = 0; for (bool present : levels) count += present;
    if (count <= 16) throw std::runtime_error("Coverage reference did not exercise more than 16 alpha levels");
    std::fprintf(stderr, "Font precision: %zu distinct FreeType coverage values retained exactly\n", count);
}

void CheckDistanceLayout() {
    for (const char* name : {"Fonts\\ARIALN.TTF", "Fonts\\ZYKai_T.TTF"}) {
        FontScope original;
        CGxFont* distanceFont = nullptr;
        if (!GxuFontCreateFont(name, .99f, original.font, 4) ||
            !GxuFontCreateFont(name, .99f, distanceFont, 4 | CGxFont::DISTANCE_FIELD))
            throw std::runtime_error("Cannot create world layout reference fonts");
        const float ratio = float(distanceFont->GetPixelSize()) / original.font->GetPixelSize();
        if (distanceFont->m_baseline != original.font->m_baseline * ratio)
            throw std::runtime_error("MSDF changed the world font baseline");
        for (uint32_t code : {'A', 'V', 'p', 'g', ' ', '?'}) {
            if (!original.font->NewCodeDesc(code) || !distanceFont->NewCodeDesc(code))
                throw std::runtime_error("Missing world layout reference glyph");
            for (uint32_t next : {'A', 'V', 'g'}) {
                if (std::abs(distanceFont->ComputeStep(code, next) / ratio -
                    original.font->ComputeStep(code, next)) > .0001f)
                    throw std::runtime_error("MSDF changed the original world font advance/kerning");
            }
        }
        // Terminal ink bounds determine centered/right-aligned name origins.
        // Atlas padding must not enter either extent or line-wrap measurement.
        for (const char* text : {"A", "AV pg", "A ", "Stormwind Guard", "未知目标", "魔兽世界\nAV"}) {
            for (float height : {.2f, 12.0f, 32.0f}) {
                float originalExtent = 0, distanceExtent = 0;
                GxuFontGetTextExtent(original.font, text, std::strlen(text), height, &originalExtent, 0, 1, 0, 0xc8);
                GxuFontGetTextExtent(distanceFont, text, std::strlen(text), height, &distanceExtent, 0, 1, 0, 0xc8);
                uint32_t originalBytes = 0, distanceBytes = 0;
                float originalWidth = 0, distanceWidth = 0;
                const char *originalNext = nullptr, *distanceNext = nullptr;
                bool originalBreak = false, distanceBreak = false;
                CalcWrapPoint(original.font, text, height, 100000, &originalBytes, &originalWidth,
                    &originalNext, 0, 0xc8, &originalBreak, nullptr, 1);
                CalcWrapPoint(distanceFont, text, height, 100000, &distanceBytes, &distanceWidth,
                    &distanceNext, 0, 0xc8, &distanceBreak, nullptr, 1);
                if (std::abs(originalExtent - distanceExtent) > .0001f ||
                    std::abs(originalWidth - distanceWidth) > .0001f ||
                    originalBytes != distanceBytes || originalNext != distanceNext)
                    throw std::runtime_error("MSDF changed the original name extent/centering");
            }
        }
        if (std::strstr(name, "ZYKai")) {
            // These real startup/name glyphs include positive bearings larger
            // than the padding; atlas translation must remain signed.
            for (uint32_t code : {0x672au, 0x77e5u, 0x76eeu, 0x6807u, 0x4e16u})
                if (!distanceFont->NewCodeDesc(code))
                    throw std::runtime_error("Cannot generate localized world-name MSDF");
        }
        auto* glyph = distanceFont->NewCodeDesc('A');
        auto& atlas = distanceFont->m_textureCache[glyph->m_textureNumber];
        const auto size = distanceFont->GetTextureSize();
        uint32_t stride; const void* pixels;
        TEXTURECACHE::TextureCallback(GxTex_Latch, size, size, 0, 0, &atlas, stride, pixels);
        if (static_cast<const uint32_t*>(pixels)[size * size - 1] != 0)
            throw std::runtime_error("Unallocated MSDF atlas texels must represent exterior distance");
        GxuFontCreateString(distanceFont, "A V pg", .2f, C3Vector(0, 0, 1), 1000, 1000, 0,
            original.string, GxVJ_Bottom, GxHJ_Center, 0x88, CImVector(255, 255, 255, 255), 0, 1);
        distanceFont->HandleScreenSizeChange();
        if (distanceFont->m_activeCharacters.Ptr('A', HASHKEY_NONE()) != glyph)
            throw std::runtime_error("Output resize needlessly regenerated the MSDF atlas");
        GxuFontDestroyString(original.string);
        GxuFontDestroyFont(distanceFont);
    }
    std::fprintf(stderr, "MSDF retains original world advances, kerning, baseline and atlas across resize\n");
}

void CheckGlyphPixelMapping(CGxDeviceMetal& device) {
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:256 height:128 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    for (const char* name : {"Fonts\\ARIALN.TTF", "Fonts\\ZYKai_T.TTF"}) {
        for (unsigned size : {14u, 18u, 24u, 36u, 48u}) {
            device.BeginFrame(pass);
            GxXformSetViewport(0, 1, 0, 1, 0, 1);
            GxRsSet(GxRs_AlphaRef, 0);
            CoordinateSetAspectRatio(2);
            FontScope sample;
            const float height = size / 128.0f;
            if (!GxuFontCreateFont(name, height, sample.font, 0)) throw std::runtime_error("Cannot create pixel mapping font");
            const bool chinese = std::strstr(name, "ZYKai") != nullptr;
            auto* glyph = sample.font->NewCodeDesc(chinese ? 0x4e16 : 'S');
            const auto& bitmap = glyph->m_bitmapData;
            std::array<unsigned, 256> expected{}, actual{};
            for (unsigned y = 0; y < bitmap.m_glyphHeight; ++y) {
                for (unsigned x = 0; x < bitmap.m_glyphWidth; ++x) {
                    const double coverage = static_cast<const uint8_t*>(bitmap.m_data)[y * bitmap.m_glyphPitch + x] / 255.0;
                    const double encoded = coverage <= .0031308 ? coverage * 12.92 : 1.055 * std::pow(coverage, 1.0 / 2.4) - .055;
                    ++expected[std::lround(encoded * 255)];
                }
            }
            sample.batch = GxuFontCreateBatch(false, false);
            if (!GxuFontCreateString(sample.font, chinese ? "世" : "S", height, C3Vector(.1f, .1f, 0), .8f, .8f, 0,
                sample.string, GxVJ_Top, GxHJ_Left, 0, CImVector(255, 255, 255, 255), 0, 1))
                throw std::runtime_error("Cannot create pixel mapping string");
            GxuFontAddToBatch(sample.batch, sample.string);
            GxuFontRenderBatch(sample.batch);
            TSGrowableArray<CImVector> pixels;
            CiRect rect(0, 0, 128, 256);
            GxReadPixels(rect, pixels);
            GxScenePresent();
            for (unsigned i = 0; i < pixels.Count(); ++i) ++actual[pixels[i].r];
            // Independent sRGB reference for white text over black. Translating
            // a one-to-one glyph may not resample it; only the documented
            // linear-coverage composition changes its encoded edge values.
            for (unsigned a = 1; a < 256; ++a) {
                if (expected[a] != actual[a]) {
                    std::fprintf(stderr, "Font pixel mapping: %s %u px, alpha %u: expected %u, got %u\n", name, size, a, expected[a], actual[a]);
                    throw std::runtime_error("UI glyph coverage changed between its bitmap and final pixels");
                }
            }
        }
    }
    std::fprintf(stderr, "UI font pixel mapping: ten actual-size glyphs match linear-light coverage composition\n");
}

void CheckFixedLayoutSize() {
    FontScope sample;
    if (!GxuFontCreateFont("Fonts\\ZYKai_T.TTF", 112.0f / g_heightPixels, sample.font,
        9 | CGxFont::UI_LAYOUT_SIZE))
        throw std::runtime_error("Cannot create original zone-title font");
    const uint32_t expectedSize = uint32_t(32 * GxMetal::HostPixelScale());
    if (sample.font->GetPixelSize() != expectedSize)
        throw std::runtime_error("FrameXML's 112px zone title lost its legacy 32px layout cap");
    if (!GxuFontCreateString(sample.font, "艾尔文森林", 0, C3Vector(.1f, .1f, 0), .9f, .5f, 0,
        sample.string, GxVJ_Top, GxHJ_Left, 4, CImVector(255, 255, 255, 255), 0, 1) ||
        std::abs(sample.string->m_currentFontHeight * g_heightPixels - expectedSize) > .001f)
        throw std::runtime_error("Fixed-size zone title changed its displayed line height");
    std::fprintf(stderr, "Original zone-title resource height 112 retains a 32-point displayed line\n");
}
uint64_t DrawFontSample(CGxDeviceMetal& device, uint32_t flags, float height = .1f, const char* fontName = "Fonts\\ARIALN.TTF") {
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:512 height:256 mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    device.BeginFrame(pass);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    GxRsSet(GxRs_AlphaRef, 0);
    CoordinateSetAspectRatio(2);
    FontScope sample;
    if (!GxuFontCreateFont(fontName, height, sample.font, flags)) {
        throw std::runtime_error("Cannot initialize the original game font from mounted Data");
    }
    sample.batch = GxuFontCreateBatch(false, false);
    const bool large = height > .1f;
    if (large && sample.font->GetPixelSize() <= 32) throw std::runtime_error("High-resolution UI glyph was capped at 32 pixels");
    const char* text = large ? "魔兽世界 Ag\n|cffffcc00清晰文字|r" : "World of Warcraft\nMetal |cffffcc00native UI|r";
    if (!GxuFontCreateString(sample.font, text, height,
        C3Vector(0.05f, 0.1f, 0), 0.9f, 0.75f, 0, sample.string, GxVJ_Top, GxHJ_Left, 0, CImVector(255, 255, 255, 255), 0, 1)) {
        throw std::runtime_error("Cannot create original Gxu text geometry");
    }
    if (flags & 8) { GxuFontAddShadow(sample.string, CImVector(255, 128, 64, 255), C2Vector(2.0f / 512, -2.0f / 256)); }
    GxuFontAddToBatch(sample.batch, sample.string);
    GxuFontRenderBatch(sample.batch);
    CiRect rect(0, 0, 256, 512);
    TSGrowableArray<CImVector> pixels;
    GxReadPixels(rect, pixels);
    GxScenePresent();
    size_t ink = 0, colored = 0;
    uint64_t hash = 14695981039346656037ull;
    for (uint32_t i = 0; i < pixels.Count(); ++i) {
        ink += pixels[i].r != 0 || pixels[i].g != 0 || pixels[i].b != 0;
        colored += pixels[i].r > pixels[i].g && pixels[i].g > pixels[i].b;
        for (uint8_t component : {pixels[i].r, pixels[i].g, pixels[i].b, pixels[i].a}) { hash = (hash ^ component) * 1099511628211ull; }
    }
    if (const char* output = flags == 0 ? std::getenv(large ? "WOW_FONT_LARGE_OUTPUT" : "WOW_FONT_OUTPUT") : nullptr) {
        FILE* file = std::fopen(output, "wb");
        if (!file) { throw std::runtime_error("Cannot write font probe image"); }
        std::fprintf(file, "P6\n512 256\n255\n");
        for (uint32_t i = 0; i < pixels.Count(); ++i) {
            uint8_t rgb[] = { pixels[i].r, pixels[i].g, pixels[i].b };
            std::fwrite(rgb, 1, 3, file);
        }
        std::fclose(file);
    }
    if (ink < 200 || ink > (large ? 70000 : 20000) || colored < 50) { throw std::runtime_error("Original Gxu font did not produce bounded visible colored glyphs"); }
    if (flags == 0) {
        // Changing only 3D resolution must not rebuild, move or resample UI.
        // Reuse the same font/string and compare every final output pixel.
        const auto originalFormat = device.m_format;
        for (unsigned samples : {1u, 4u}) {
            auto format = originalFormat; format.m_multisampleCount = samples;
            if (!device.DeviceSetFormat(format)) throw std::runtime_error("Cannot set scene-resolution check MSAA");
            for (float scale : {.5f, .75f, 1.0f}) {
                device.BeginFrame(pass);
                GxXformSetViewport(0, 1, 0, 1, 0, 1);
                device.BeginSceneResolution(scale);
                CRect scene; device.CapsWindowSize(scene);
                if (scene.maxX != 512 * scale || scene.maxY != 256 * scale)
                    throw std::runtime_error("Scene render dimensions did not follow its independent scale");
                GxSceneClear(1, CImVector(0, 0, 0, 255));
                device.EndSceneResolution();
                CRect output; device.CapsWindowSize(output);
                if (output.maxX != 512 || output.maxY != 256 || g_heightPixels != 256)
                    throw std::runtime_error("Scene scale changed UI coordinate dimensions");
                GxRsSet(GxRs_Multisample, 0);
                GxuFontRenderBatch(sample.batch);
                TSGrowableArray<CImVector> scaled;
                GxReadPixels(rect, scaled); GxScenePresent();
                if (scaled.Count() != pixels.Count() || std::memcmp(scaled.Ptr(), pixels.Ptr(), pixels.Count() * sizeof(CImVector)))
                    throw std::runtime_error("Scene render scale softened or moved output-resolution UI");
            }
        }
        if (!device.DeviceSetFormat(originalFormat)) throw std::runtime_error("Cannot restore scene-resolution check format");
        // Keep the font and string alive while the host changes drawable size.
        // Restoring the original size must reproduce the original glyphs.
        for (unsigned width : {768u, 512u}) {
            descriptor.width = width; descriptor.height = width / 2;
            pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
            device.BeginFrame(pass);
            GxuFontWindowSizeChanged();
            GxuFontRenderBatch(sample.batch);
            CiRect resizedRect(0, 0, width / 2, width);
            TSGrowableArray<CImVector> resized;
            GxReadPixels(resizedRect, resized); GxScenePresent();
            size_t resizedInk = 0;
            for (uint32_t i = 0; i < resized.Count(); ++i) { resizedInk += resized[i].r != 0; }
            if (resizedInk < 200 || (width == 512 && (resized.Count() != pixels.Count() ||
                std::memcmp(resized.Ptr(), pixels.Ptr(), pixels.Count() * sizeof(CImVector))))) {
                throw std::runtime_error("Gxu font drawable resize/restore changed glyph output");
            }
        }
        // A texture-quality change invalidates resident glyph textures, while
        // the original font/string/cache objects remain alive and regenerate them.
        for (uint32_t base : {1u, 0u}) {
            device.DeviceSetBaseMipLevel(base);
            device.BeginFrame(pass); GxuFontRenderBatch(sample.batch);
            CiRect restoredRect(0, 0, 256, 512); TSGrowableArray<CImVector> restored;
            GxReadPixels(restoredRect, restored); GxScenePresent();
            if (restored.Count() != pixels.Count() || std::memcmp(restored.Ptr(), pixels.Ptr(), pixels.Count() * sizeof(CImVector))) {
                throw std::runtime_error("Gxu glyph textures changed after base-mip recreation");
            }
        }
        // The quest log uses a zero-length gradient during resize. Its empty
        // fade interval must preserve the existing opaque glyphs exactly.
        for (int start : {0, 3}) {
            GxuFontStringSetGradient(sample.string, start, 0);
            device.BeginFrame(pass); GxuFontRenderBatch(sample.batch);
            TSGrowableArray<CImVector> restored;
            GxReadPixels(rect, restored); GxScenePresent();
            if (restored.Count() != pixels.Count() || std::memcmp(restored.Ptr(), pixels.Ptr(), pixels.Count() * sizeof(CImVector)))
                throw std::runtime_error("Zero-length font gradient changed the existing glyph output");
        }
        // Zero alpha is the other undefined division (0/0); it must also be
        // accepted without changing the original gradient traversal.
        GxuFontSetStringColor(sample.string, CImVector(255, 255, 255, 0));
        GxuFontStringSetGradient(sample.string, 0, 0);
    }
    return hash;
}
}

void CheckTextCompositing(GxMetal::TextureDevice& gpu) {
    const std::array<uint8_t, 8> coverage{0, 1, 16, 64, 127, 192, 254, 255};
    auto linear = [](double x) { return x <= .04045 ? x / 12.92 : std::pow((x + .055) / 1.055, 2.4); };
    auto encoded = [](double x) { return x <= .0031308 ? x * 12.92 : 1.055 * std::pow(x, 1.0 / 2.4) - .055; };
    for (unsigned samples : {1u, 4u}) for (unsigned fill : {0u, 160u, 255u}) for (float opacity : {.4f, 1.0f}) {
        BLP::Image image; image.format = BLP::Format::BGRA8;
        BLP::Mip mip{8, 1, 32, {}};
        for (uint8_t a : coverage) mip.bytes.insert(mip.bytes.end(), {uint8_t(fill), uint8_t(fill), uint8_t(fill), a});
        image.mips.push_back(std::move(mip));
        auto atlas = gpu.Upload(image);
        auto desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:8 height:1 mipmapped:NO];
        desc.storageMode = MTLStorageModePrivate; desc.usage = MTLTextureUsageRenderTarget;
        auto target = [gpu.device newTextureWithDescriptor:desc];
        auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = target;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        if (samples > 1) {
            desc.textureType = MTLTextureType2DMultisample; desc.sampleCount = samples;
            pass.colorAttachments[0].texture = [gpu.device newTextureWithDescriptor:desc];
            pass.colorAttachments[0].resolveTexture = target;
            pass.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        }
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(32.0 / 255, 96.0 / 255, 224.0 / 255, .8);
        GxMetal::TextureDrawState state;
        state.filter = GxTex_Nearest; state.blend = GxBlend_Alpha; state.linearText = true;
        GxMetal::QuadParameters parameters; parameters.tint = {1, .6f, .2f, opacity};
        auto command = [gpu.queue commandBuffer];
        gpu.Draw(command, pass, atlas, parameters, state);
        auto buffer = [gpu.device newBufferWithLength:256 options:MTLResourceStorageModeShared];
        auto blit = [command blitCommandEncoder];
        [blit copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
            sourceSize:MTLSizeMake(8, 1, 1) toBuffer:buffer destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256];
        [blit endEncoding]; [command commit]; [command waitUntilCompleted];
        if (command.status != MTLCommandBufferStatusCompleted) throw std::runtime_error("Native text composition readback failed");
        const auto* actual = static_cast<const uint8_t*>(buffer.contents);
        const double background[] = {224.0 / 255, 96.0 / 255, 32.0 / 255};
        const double color[] = {.2, .6, 1};
        for (unsigned x = 0; x < coverage.size(); ++x) {
            const double a = coverage[x] / 255.0 * opacity;
            for (unsigned c = 0; c < 4; ++c) {
                const double result = c == 3 ? a * a + .8 * (1 - a) :
                    encoded(linear(color[c]) * (fill / 255.0) * a + linear(background[c]) * (1 - a));
                if (std::abs(int(actual[x * 4 + c]) - int(std::lround(result * 255))) > 1)
                    throw std::runtime_error("Text fill/outline, colored background, opacity or MSAA composition mismatch");
            }
        }
    }
    std::fprintf(stderr, "Native text fill/outline, colored backgrounds, opacity and 1x/4x MSAA match linear-light references\n");
}

void CheckNativeWorldText(CGxDeviceMetal& device) {
    const auto original = device.m_format;
    device.RsPush();
    C44Matrix projection(1);
    projection.a0 = 2.0f / 512; projection.b1 = 2.0f / 256;
    projection.c2 = 2; projection.d0 = projection.d1 = projection.d2 = -1;
    auto desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:512 height:256 mipmapped:NO];
    desc.storageMode = MTLStorageModePrivate; desc.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:desc];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    desc.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:desc];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    unsigned checks = 0;
    for (uint32_t raster : {12u, 32u}) {
        FontScope sample;
        const uint32_t flags = 4 | CGxFont::DISTANCE_FIELD;
        GxuFontCreateFont("Fonts\\ZYKai_T.TTF", float(raster) / 256, sample.font, flags);
        sample.batch = GxuFontCreateBatch(true, false);
        GxuFontCreateString(sample.font, "Stormwind Guard 魔兽世界", float(raster),
            C3Vector(40.25f, 80.25f, .5f), 100000, 100000, 0, sample.string,
            GxVJ_Bottom, GxHJ_Left, 0x88, CImVector(30, 255, 60, 255), 0, 1);
        GxuFontAddToBatch(sample.batch, sample.string);
        for (unsigned samples : {1u, 4u}) {
            auto format = original; format.m_multisampleCount = samples;
            if (!device.DeviceSetFormat(format)) throw std::runtime_error("Cannot set native text MSAA");
            std::vector<CImVector> reference;
            for (float scale : {1.0f, .75f, .5f}) for (bool occluder : {false, true}) {
                device.BeginFrame(pass); GxXformSetViewport(0, 1, 0, 1, 0, .94f);
                GxXformSetView(C44Matrix(1)); GxXformSetProjection(projection);
                GxRsSet(GxRs_DepthFunc, 0); GxRsSet(GxRs_Multisample, 1);
                GxRsSet(GxRs_ScissorTest, 0);
                device.BeginSceneResolution(scale);
                GxSceneClear(3, CImVector(0, 0, 0, 255));
                GxuFontRenderBatch(sample.batch);
                if (occluder) {
                    // Geometry submitted AFTER the name must still hide its ink.
                    for (unsigned i = 0; i < 4; ++i) GxRsSet(EGxRenderState(GxRs_Texture0 + i), static_cast<CGxTex*>(nullptr));
                    GxRsSet(GxRs_VertexShader, static_cast<CGxShader*>(nullptr));
                    GxRsSet(GxRs_PixelShader, static_cast<CGxShader*>(nullptr));
                    for (auto state : {GxRs_Lighting, GxRs_Fog, GxRs_Culling, GxRs_AlphaRef}) GxRsSet(state, 0);
                    GxRsSet(GxRs_DepthTest, 1); GxRsSet(GxRs_DepthWrite, 1);
                    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
                    const C3Vector positions[]{{-10,-10,.25f},{150,-10,.25f},{-10,300,.25f},{150,300,.25f}};
                    CImVector color(0, 0, 0, 255);
                    GxPrimVertexPtr(4, positions, sizeof(C3Vector), nullptr, 0, &color, 0, nullptr, 0, nullptr, 0);
                    CGxBatch batch{GxPrim_TriangleStrip, 0, 4, 0, 3}; device.Draw(&batch, 0);
                }
                device.SealWorldTextDepth();
                // Model FFX replacing both scene color and scratch depth.
                GxSceneClear(3, CImVector(0, 0, 0, 255));
                device.EndSceneResolution();
                TSGrowableArray<CImVector> pixels; CiRect rect(0, 0, 256, 512);
                GxReadPixels(rect, pixels); GxScenePresent();
                size_t ink = 0;
                for (uint32_t i = 0; i < pixels.Count(); ++i) {
                    const auto p = pixels[i]; const unsigned x = i % 512;
                    ink += p.g != 0;
                    if (occluder && x < 148 && p.g) throw std::runtime_error("Native text leaked through foreground depth");
                    if (reference.empty()) continue;
                    if ((!occluder || x > 152) && std::memcmp(&p, &reference[i], sizeof(p)))
                        throw std::runtime_error("World text changed with scene scale or post-processing");
                }
                if (ink < 50) throw std::runtime_error("Native text was missing or completely occluded");
                if (reference.empty()) reference.assign(pixels.Ptr(), pixels.Ptr() + pixels.Count());
                ++checks;
            }
        }
    }
    device.DeviceSetFormat(original); device.RsPop();
    std::fprintf(stderr, "Native world text: %u small/large MSDF, scale, post-process and foreground-depth cases passed\n", checks);
}

void CheckGxFont(CGxDeviceMetal& device) {
    CheckTextCompositing(device.GPU());
    CheckGlyphPixelMapping(device);
    CheckFixedLayoutSize();
    CheckLocalizedAdvances();
    CheckDistanceLayout();
    struct CacheScope { ~CacheScope() { TextureGxCacheFlush(); } } cache;
    device.PoolSizeSet(device.m_vertexPool, 1024 * 1024);
    device.PoolSizeSet(device.m_indexPool, 64 * 1024);
    // Exercise the original AA/monochrome and normal/thick outline paths,
    // including texture reuse after destroying and recreating the font system.
    uint64_t original = DrawFontSample(device, 0);
    CheckCoveragePrecision();
    for (uint32_t flags : {1u, 2u, 3u, 8u, 11u}) { DrawFontSample(device, flags); }
    for (uint32_t flags : {0u, 1u, 2u, 3u, 9u, 11u}) { DrawFontSample(device, flags, .25f, "Fonts\\ZYKai_T.TTF"); }
    for (uint32_t flags : {4u, 5u, 13u})
        DrawFontSample(device, flags | CGxFont::DISTANCE_FIELD, .25f, "Fonts\\ZYKai_T.TTF");
    CheckNativeWorldText(device);
    if (DrawFontSample(device, 0) != original) { throw std::runtime_error("Gxu font recreation changed the glyph output"); }
}
