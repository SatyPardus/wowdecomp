#include <cstddef>
#include <cstdint>
#include <cstring>
#if defined(__APPLE__)
#include <algorithm>
#include <vector>
#endif
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Gxu/GxuFontMiscClasses.h"
#include "Engine/Source/Gxu/GxuFontUtil.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00C7D328
static uint16_t s_textureData[256 * 256];

#if defined(__APPLE__)
namespace {
// BGRA8 uses the existing UI shader and straight-alpha blending. Keep full
// FreeType coverage for both the glyph and its outline instead of UNORM4.
static std::vector<uint32_t> s_fontPixels;

void PasteGlyph8(const GLYPHBITMAPDATA& data, uint32_t* dest, uint32_t stride,
                 uint32_t cellHeight, uint32_t flags) {
    const bool mono = (flags & 2) != 0;
    const uint32_t radius = (flags & 1) ? ((flags & 8) ? 2 : 1) : 0;
    const int cellWidth = int(data.m_glyphCellWidth);
    auto coverage = [&](int x, int y) -> uint32_t {
        x -= int(radius); y -= int(data.m_yStart + (radius ? 1 : 0));
        if (x < 0 || y < 0 || x >= int(data.m_glyphWidth) || y >= int(data.m_glyphHeight)) return 0;
        const auto* row = static_cast<const uint8_t*>(data.m_data) + y * data.m_glyphPitch;
        return mono ? ((row[x / 8] & (0x80 >> (x % 8))) ? 255 : 0) : row[x];
    };
    for (int y = 0; y < int(cellHeight); ++y) {
        for (int x = 0; x < cellWidth; ++x) {
            const uint32_t fill = coverage(x, y);
            uint32_t alpha = fill;
            for (int dy = -int(radius); dy <= int(radius); ++dy) {
                for (int dx = -int(radius); dx <= int(radius); ++dx) {
                    uint32_t edge = coverage(x + dx, y + dy);
                    if (std::max(std::abs(dx), std::abs(dy)) == 2) edge = (edge * 119 + 127) / 255;
                    alpha = std::max(alpha, edge);
                }
            }
            const uint32_t gray = alpha ? (fill * 255 + alpha / 2) / alpha : 255;
            dest[y * stride + x] = (alpha << 24) | (gray << 16) | (gray << 8) | gray;
        }
    }
}
}
#endif

// VERIFY: 7a9fdc75 manual
// OFFSET: 0x006C8E70
void TEXTURECACHE::PasteGlyphOutlinedMonochrome(const GLYPHBITMAPDATA& data, uint16_t* dest) {
    const uint8_t* source = static_cast<const uint8_t*>(data.m_data);
    uint32_t thick = this->m_theFace->m_fontFlags & 8;

    uint16_t buffer[256 * 36];
    memset(buffer, 0, sizeof(buffer));

    uint32_t yStart = data.m_yStart;
    uint16_t* line = &buffer[256 * yStart + 257];

    if (thick) {
        line++;
    }

    for (uint32_t y = data.m_glyphHeight; y; y--) {
        for (uint32_t x = 0; x < data.m_glyphWidth; x++) {
            line[x] = (source[x >> 3] >> (7 - (x & 7))) & 1;
        }

        source += data.m_glyphPitch;
        line += 256;
    }

    uint32_t cellHeight = this->m_theFace->m_cellHeight;
    uint32_t cellWidth = data.m_glyphCellWidth;
    uint32_t passes = (thick != 0) + 1;

    for (uint32_t pass = 0; pass < passes; pass++) {
        uint32_t y = yStart;

        if (pass == 1 && yStart) {
            y = yStart - 1;
        }

        for (; y < cellHeight; y++) {
            uint32_t row = y << 8;

            for (uint32_t x = 0; x < cellWidth; x++) {
                uint16_t mask = 0;
                uint16_t value = 0;

                if (pass == 1) {
                    if (buffer[row + x]) {
                        continue;
                    }

                    mask = 4;
                    value = 2;
                } else if (pass == 0) {
                    mask = 1;
                    value = 4;
                }

                uint16_t* cell = &buffer[row + x];
                int32_t found;

                if (y == 0) {
                    if (x == 0) {
                        found = (cell[1] & mask) || (cell[257] & mask) || (cell[256] & mask);
                    } else if (x == cellWidth - 1) {
                        found = (cell[-1] & mask) || (cell[255] & mask) || (cell[256] & mask);
                    } else {
                        found = (cell[-1] & mask) || (cell[1] & mask) || (cell[255] & mask) || (cell[256] & mask) || (cell[257] & mask);
                    }
                } else if (y == cellHeight - 1) {
                    if (x == 0) {
                        found = (cell[-256] & mask) || (cell[-255] & mask) || (cell[1] & mask);
                    } else if (x == cellWidth - 1) {
                        found = (cell[-1] & mask) || (cell[-257] & mask) || (cell[-256] & mask);
                    } else {
                        found = (cell[-257] & mask) || (cell[-256] & mask) || (cell[-255] & mask) || (cell[-1] & mask) || (cell[1] & mask);
                    }
                } else if (x == 0) {
                    found = (cell[-256] & mask) || (cell[-255] & mask) || (cell[1] & mask) || (cell[257] & mask) || (cell[256] & mask);
                } else if (x == cellWidth - 1) {
                    found = (cell[-256] & mask) || (cell[-257] & mask) || (cell[-1] & mask) || (cell[255] & mask) || (cell[256] & mask);
                } else {
                    found = (cell[-257] & mask) || (cell[-256] & mask) || (cell[-255] & mask) || (cell[-1] & mask) || (cell[1] & mask) || (cell[255] & mask) || (cell[256] & mask) || (cell[257] & mask);
                }

                if (found && *cell != 1) {
                    *cell = value;
                }
            }
        }
    }

    uint16_t* bufferRow = buffer;

    for (uint32_t y = 0; y < this->m_theFace->m_cellHeight; y++) {
        for (uint32_t x = 0; x < data.m_glyphCellWidth; x++) {
            uint16_t value = bufferRow[x];

            if (value == 1) {
                dest[x] = 0xFFFF;
            } else if (value == 2) {
                dest[x] = 0x7000;
            } else {
                dest[x] = value != 4 ? 0 : 0xF000;
            }
        }

        bufferRow += 256;
        dest += 256;
    }
}

// VERIFY: 891087d8 manual
// OFFSET: 0x006C9330
void TEXTURECACHE::PasteGlyphNonOutlinedMonochrome(const GLYPHBITMAPDATA& data, uint16_t* dest) {
    const uint8_t* source = static_cast<const uint8_t*>(data.m_data);
    uint32_t pitch = data.m_glyphPitch;
    size_t size = 2 * data.m_glyphCellWidth;

    for (uint32_t y = 0; y < data.m_yStart; y++) {
        memset(dest, 0, size);
        dest += 256;
    }

    for (uint32_t y = 0; y < data.m_glyphHeight; y++) {
        for (uint32_t x = 0; x < data.m_glyphWidth; x++) {
            dest[x] = -(((source[x >> 3] >> (7 - (x & 7))) & 1) != 0);
        }

        source += pitch;
        dest += 256;
    }

    int32_t remaining = this->m_theFace->m_cellHeight - data.m_yStart - data.m_glyphHeight;

    if (remaining > 0) {
        for (uint32_t y = 0; y < static_cast<uint32_t>(remaining); y++) {
            memset(dest, 0, size);
            dest += 256;
        }
    }
}

// VERIFY: c5efdcbb manual
// OFFSET: 0x006C9420
void TEXTURECACHE::PasteGlyphOutlinedAA(const GLYPHBITMAPDATA& data, uint16_t* dest) {
    static const uint8_t s_outlineAlpha[] = { 0, 1, 1, 3, 5, 7, 9, 11, 13, 15 };

    const uint8_t* source = static_cast<const uint8_t*>(data.m_data);
    uint32_t thick = this->m_theFace->m_fontFlags & 8;

    uint16_t alpha[256 * 36];
    uint16_t coverage[256 * 36];
    uint16_t state[256 * 36];

    memset(state, 0, sizeof(state));
    memset(coverage, 0, sizeof(coverage));

    uint16_t* stateLine = &state[256 * data.m_yStart + 257];

    if (thick) {
        stateLine++;
    }

    uint16_t* coverageLine = &coverage[stateLine - state];

    for (uint32_t y = data.m_glyphHeight; y; y--) {
        for (uint32_t x = 0; x < data.m_glyphWidth; x++) {
            uint8_t value = source[x];

            if (value) {
                stateLine[x] = 1;
                coverageLine[x] = value;
            }
        }

        coverageLine += 256;
        source += data.m_glyphPitch;
        stateLine += 256;
    }

    uint32_t cellHeight = this->m_theFace->m_cellHeight;
    uint32_t passes = (thick != 0) + 1;

    for (uint32_t pass = 0; pass < passes; pass++) {
        uint16_t value = 2 * (pass != 0) + 2;
        uint16_t mask = 2 * (pass != 0) + 1;
        int32_t firstRow = static_cast<int32_t>(data.m_yStart) - 1;

        for (uint32_t y = firstRow < 0 ? 0 : firstRow; y < cellHeight; y++) {
            uint32_t cellWidth = data.m_glyphCellWidth;
            uint32_t row = y << 8;

            for (uint32_t x = 0; x < cellWidth; x++) {
                uint16_t* cell = &state[row + x];

                if (*cell & mask) {
                    continue;
                }

                int32_t found;

                if (y == 0) {
                    if (x == 0) {
                        found = (cell[1] & mask) || (cell[257] & mask) || (cell[256] & mask);
                    } else if (x == cellWidth - 1) {
                        found = (cell[-1] & mask) || (cell[255] & mask) || (cell[256] & mask);
                    } else {
                        found = (cell[-1] & mask) || (cell[1] & mask) || (cell[255] & mask) || (cell[256] & mask) || (cell[257] & mask);
                    }
                } else if (y == cellHeight - 1) {
                    if (x == 0) {
                        found = (cell[-256] & mask) || (cell[-255] & mask) || (cell[1] & mask);
                    } else if (x == cellWidth - 1) {
                        found = (cell[-1] & mask) || (cell[-257] & mask) || (cell[-256] & mask);
                    } else {
                        found = (cell[-257] & mask) || (cell[-256] & mask) || (cell[-255] & mask) || (cell[-1] & mask) || (cell[1] & mask);
                    }
                } else if (x == 0) {
                    found = (cell[-256] & mask) || (cell[-255] & mask) || (cell[1] & mask) || (cell[257] & mask) || (cell[256] & mask);
                } else if (x == cellWidth - 1) {
                    found = (cell[-256] & mask) || (cell[-257] & mask) || (cell[-1] & mask) || (cell[255] & mask) || (cell[256] & mask);
                } else {
                    found = (cell[-257] & mask) || (cell[-256] & mask) || (cell[-255] & mask) || (cell[-1] & mask) || (cell[1] & mask) || (cell[255] & mask) || (cell[256] & mask) || (cell[257] & mask);
                }

                if (found) {
                    *cell = value;
                }
            }
        }
    }

    memset(alpha, 0, sizeof(alpha));

    for (uint32_t y = 0; y < cellHeight; y++) {
        uint32_t row = y << 8;

        for (uint32_t x = 0; x < data.m_glyphCellWidth; x++) {
            uint16_t* cell = &state[row + x];
            uint32_t cellWidth = data.m_glyphCellWidth;
            int32_t count;

            if (y == 0) {
                if (x == 0) {
                    count = (cell[0] != 0) + (cell[1] != 0) + (cell[257] != 0) + (cell[256] != 0);
                } else if (x == cellWidth - 1) {
                    count = (cell[0] != 0) + (cell[256] != 0) + (cell[255] != 0) + (cell[-1] != 0);
                } else {
                    count = (cell[0] != 0) + (cell[1] != 0) + (cell[257] != 0) + (cell[256] != 0) + (cell[255] != 0) + (cell[-1] != 0);
                }
            } else if (y == cellHeight - 1) {
                if (x == 0) {
                    count = (cell[0] != 0) + (cell[1] != 0) + (cell[-255] != 0) + (cell[-256] != 0);
                } else if (x == cellWidth - 1) {
                    count = (cell[-256] != 0) + (cell[-257] != 0) + (cell[0] != 0) + (cell[-1] != 0);
                } else {
                    count = (cell[-256] != 0) + (cell[-257] != 0) + (cell[-255] != 0) + (cell[1] != 0) + (cell[0] != 0) + (cell[-1] != 0);
                }
            } else if (x == 0) {
                count = (cell[0] != 0) + (cell[256] != 0) + (cell[1] != 0) + (cell[257] != 0) + (cell[-255] != 0) + (cell[-256] != 0);
            } else if (x == cellWidth - 1) {
                count = (cell[256] != 0) + (cell[255] != 0) + (cell[-256] != 0) + (cell[-257] != 0) + (cell[0] != 0) + (cell[-1] != 0);
            } else {
                count = (cell[256] != 0) + (cell[255] != 0) + (cell[-255] != 0) + (cell[-256] != 0) + (cell[-257] != 0) + (cell[257] != 0) + (cell[1] != 0) + (cell[0] != 0) + (cell[-1] != 0);
            }

            alpha[row + x] = s_outlineAlpha[count];
        }
    }

    uint32_t offset = 0;

    for (uint32_t y = 0; y < this->m_theFace->m_cellHeight; y++) {
        uint32_t index = offset;

        for (uint32_t x = 0; x < data.m_glyphCellWidth; x++) {
            if (!state[index]) {
                dest[index] = 0;
            } else if (!coverage[index]) {
                dest[index] = alpha[index] << 12;
            } else {
                uint8_t gray = (255 * static_cast<uint8_t>(coverage[index])) >> 12;
                dest[index] = ((((alpha[index] << 4) | gray) << 4 | gray) << 4) | gray;
            }

            index++;
        }

        offset += 256;
    }
}

// VERIFY: 45fd06c3 manual
// OFFSET: 0x006C9C70
void TEXTURECACHE::PasteGlyphNonOutlinedAA(const GLYPHBITMAPDATA& data, uint16_t* dest) {
    const uint8_t* source = static_cast<const uint8_t*>(data.m_data);
    uint32_t pitch = data.m_glyphPitch;
    size_t size = 2 * data.m_glyphCellWidth;

    for (uint32_t y = 0; y < data.m_yStart; y++) {
        memset(dest, 0, size);
        dest += 256;
    }

    for (uint32_t y = 0; y < data.m_glyphHeight; y++) {
        for (uint32_t x = 0; x < data.m_glyphWidth; x++) {
            dest[x] = (source[x] << 8) | 0xFFF;
        }

        source += pitch;
        dest += 256;
    }

    int32_t remaining = this->m_theFace->m_cellHeight - data.m_yStart - data.m_glyphHeight;

    if (remaining > 0) {
        for (uint32_t y = 0; y < static_cast<uint32_t>(remaining); y++) {
            memset(dest, 0, size);
            dest += 256;
        }
    }
}

// VERIFY: d9c18e4c auto
// OFFSET: 0x006C9D50
void TEXTURECACHE::UpdateDirty() {
    if (this->m_anyDirtyGlyphs && this->m_texture) {
        CGxTex* gxTex = TextureGetGxTex(this->m_texture, 1, nullptr);
        const uint32_t size = this->m_theFace->GetTextureSize();
        GxTexUpdate(gxTex, 0, 0, size, size, 1);
        this->m_anyDirtyGlyphs = 0;
    }
}

// VERIFY: 0d8fc085 manual
// OFFSET: 0x006C9D90
CHARCODEDESC* TEXTURECACHE::AllocateNewGlyph(GLYPHBITMAPDATA* data) {
    uint32_t count = this->m_textureRows.m_count;

    for (uint32_t i = 0; i < count; i++) {
        CHARCODEDESC* desc = this->m_textureRows.m_data[i].CreateNewDesc(data, i, this->m_theFace->m_cellHeight);

        if (desc) {
            return desc;
        }
    }

    return nullptr;
}

// VERIFY: 248ca280 manual
// OFFSET: 0x006C9F50
void TEXTURECACHE::TextureCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command != GxTex_Latch) {
        return;
    }

    TEXTURECACHE* cache = static_cast<TEXTURECACHE*>(userArg);

#if defined(__APPLE__)
    // Rebuild from resident descriptors. Clearing prevents an evicted, wider
    // glyph from leaking into a new glyph's filtering footprint.
    s_fontPixels.assign(size_t(width) * height, 0x00FFFFFF);
#endif
    for (uint32_t row = cache->m_textureRows.m_count; row > 0; row--) {
        TEXTURECACHEROW& cacheRow = cache->m_textureRows.m_data[row - 1];

        for (CHARCODEDESC* glyph = cacheRow.m_glyphlist.Head(); glyph; glyph = cacheRow.m_glyphlist.Next(glyph)) {
            if (!cache->m_texture || !cache->m_theFace || !cache->m_theFace->m_cellHeight) {
                continue;
            }

            CGxFont* font = cache->m_theFace;
#if defined(__APPLE__)
            uint32_t* dest = &s_fontPixels[glyph->m_glyphStartPixel + glyph->m_rowNumber * font->m_cellHeight * width];
            PasteGlyph8(glyph->m_bitmapData, dest, width, font->m_cellHeight, font->m_fontFlags);
#else
            uint16_t* dest = &s_textureData[glyph->m_glyphStartPixel + ((glyph->m_rowNumber * font->m_cellHeight) << 8)];

            if (font->m_fontFlags & 1) {
                if (font->m_fontFlags & 2) {
                    cache->PasteGlyphOutlinedMonochrome(glyph->m_bitmapData, dest);
                } else {
                    cache->PasteGlyphOutlinedAA(glyph->m_bitmapData, dest);
                }
            } else if (font->m_fontFlags & 2) {
                cache->PasteGlyphNonOutlinedMonochrome(glyph->m_bitmapData, dest);
            } else {
                cache->PasteGlyphNonOutlinedAA(glyph->m_bitmapData, dest);
            }
#endif
        }
    }

#if defined(__APPLE__)
    texelStrideInBytes = width * sizeof(uint32_t);
    texels = s_fontPixels.data();
#else
    texelStrideInBytes = 512;
    texels = s_textureData;
#endif
}

// VERIFY: 0f656a97 auto
// OFFSET: 0x006CA050
void TEXTURECACHE::CreateTexture(int32_t filter) {
    CGxTexFlags flags(static_cast<EGxTexFilter>(filter != 0), 0, 0, 0, 0, 0, 1, 0, 0, 0);
#if defined(__APPLE__)
    const uint32_t size = this->m_theFace->GetTextureSize();
    this->m_texture = TextureCreate(size, size, GxTex_Argb8888, GxTex_Argb8888, flags, this, TEXTURECACHE::TextureCallback, "GxuFont", 0);
#else
    this->m_texture = TextureCreate(256, 256, GxTex_Argb4444, GxTex_Argb4444, flags, this, TEXTURECACHE::TextureCallback, "GxuFont", 0);
#endif
}

// VERIFY: ba005b39 manual
// OFFSET: 0x006CA130
void TEXTURECACHE::Initialize(CGxFont* face, uint32_t cellHeight) {
    this->m_theFace = face;
    const uint32_t size = face->GetTextureSize();
    uint32_t rows = size / cellHeight;
    this->m_textureRows.SetCount(rows);

    for (uint32_t i = 0; i < rows; i++) {
        this->m_textureRows.m_data[i].m_widestFreeSlot = size;
#if defined(__APPLE__)
        this->m_textureRows.m_data[i].m_textureSize = size;
#endif
    }
}

// INLINED
TEXTURECACHE::TEXTURECACHE() {
    this->m_texture = nullptr;
    this->m_theFace = nullptr;
    this->m_anyDirtyGlyphs = 0;
}

// INLINED
TEXTURECACHE::~TEXTURECACHE() {
    this->Destroy();
}

// INLINED
void TEXTURECACHE::Destroy() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    this->m_texture = nullptr;
    this->m_textureRows.Clear();
}
