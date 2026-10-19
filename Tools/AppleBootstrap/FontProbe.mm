#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Gxu/GxuFontFace.h"
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Services/Texture.h"
#include <freetype/internal/ftstream.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
void CheckFontIntegerReads() {
    for (uint32_t expected : {0u, 0x01020304u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
        const int64_t signedExpected = expected & 0x80000000u ? int64_t(expected) - 0x100000000ll : expected;
        FT_Byte bytes[4];
        for (bool little : {false, true}) {
            for (unsigned i = 0; i < 4; ++i) { bytes[i] = uint8_t(expected >> (8 * (little ? i : 3 - i))); }
            FT_StreamRec_ record{};
            record.base = record.cursor = bytes;
            record.size = 4; record.limit = bytes + 4;
            FT_Stream stream = &record;
            auto* pointer = bytes;
            FT_ULong next = little ? NEXT_ULongLE(pointer) : NEXT_ULong(pointer);
            FT_ULong get = little ? GET_ULongLE() : GET_ULong();
            record.cursor = bytes;
            FT_Long signedGet = little ? GET_LongLE() : GET_Long();
            FT_ULong read = 0;
            FT_Error error = 0;
            bool failed = little ? READ_ULongLE(read) : READ_ULong(read);
            if (next != expected || get != expected || read != expected || signedGet != signedExpected || failed) {
                throw std::runtime_error("FreeType font integer width or sign-extension mismatch");
            }
        }
    }
}

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
        for (unsigned size : {12u, 24u, 32u}) {
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

uint64_t DrawFontSample(CGxDeviceMetal& device, uint32_t flags) {
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
    if (!GxuFontCreateFont("Fonts\\ARIALN.TTF", 0.1f, sample.font, flags)) {
        throw std::runtime_error("Cannot initialize the original game font from mounted Data");
    }
    sample.batch = GxuFontCreateBatch(false, false);
    if (!GxuFontCreateString(sample.font, "World of Warcraft\nMetal |cffffcc00native UI|r", 0.1f,
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
    if (const char* output = flags == 0 ? std::getenv("WOW_FONT_OUTPUT") : nullptr) {
        FILE* file = std::fopen(output, "wb");
        if (!file) { throw std::runtime_error("Cannot write font probe image"); }
        std::fprintf(file, "P6\n512 256\n255\n");
        for (uint32_t i = 0; i < pixels.Count(); ++i) {
            uint8_t rgb[] = { pixels[i].r, pixels[i].g, pixels[i].b };
            std::fwrite(rgb, 1, 3, file);
        }
        std::fclose(file);
    }
    if (ink < 200 || ink > 20000 || colored < 50) { throw std::runtime_error("Original Gxu font did not produce bounded visible colored glyphs"); }
    if (flags == 0) {
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

void CheckGxFont(CGxDeviceMetal& device) {
    CheckFontIntegerReads();
    CheckLocalizedAdvances();
    struct CacheScope { ~CacheScope() { TextureGxCacheFlush(); } } cache;
    device.PoolSizeSet(device.m_vertexPool, 1024 * 1024);
    device.PoolSizeSet(device.m_indexPool, 64 * 1024);
    // Exercise the original AA/monochrome and normal/thick outline paths,
    // including texture reuse after destroying and recreating the font system.
    uint64_t original = DrawFontSample(device, 0);
    for (uint32_t flags : {1u, 2u, 3u, 8u, 11u}) { DrawFontSample(device, flags); }
    if (DrawFontSample(device, 0) != original) { throw std::runtime_error("Gxu font recreation changed the glyph output"); }
}
