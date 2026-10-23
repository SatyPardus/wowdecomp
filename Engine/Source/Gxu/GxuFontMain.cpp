#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <typeinfo>
#include <freetype/ftmodule.h>
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Gxu/GxuFontMiscClasses.h"
#include "Engine/Source/Gxu/GxuFontUtil.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRect.h"

// STATIC: 0x00A31560
const int32_t g_quotedCodeLengths[NUM_QUOTEDCODES] = { 0, 10, 2, 2, 2, 2, 2, 2, 2 };

// STATIC: 0x00AD9960
static FT_MemoryRec_ s_GxuMemoryRecord = { nullptr, FreeTypeAlloc, FreeTypeFree, FreeTypeRealloc };

// STATIC: 0x00AD9970
static TSList<CGxStringBatch, TSGetLink<CGxStringBatch> > s_freeBatches;

// STATIC: 0x00AD99C8
static QUOTEDCODEINFO s_quotedCodeInfo[NUM_QUOTEDCODES] = {
    { 0x0, '-', 1 },
    { 0x100, 'C', 1 },
    { 0x100, 'R', 1 },
    { 0x200, '\n', 0 },
    { 0x800, '|', 0 },
    { 0x400, 'H', 1 },
    { 0x400, 'h', 1 },
    { 0x1000, 'T', 1 },
    { 0x1000, 't', 1 },
};

// STATIC: 0x00AD9AAC
TSList<CGxFont, TSGetLink<CGxFont> > g_fonts;

// STATIC: 0x00AD9AB8
TSList<CGxString, TSGetLink<CGxString> > g_freeStrings;

// STATIC: 0x00AD9AC4
TSList<CGxString, TSGetLink<CGxString> > g_strings;

// STATIC: 0x00C7C0A8
static uint16_t s_graphIndices[30 * 6];

// STATIC: 0x00C7C210
static uint32_t s_lastFrameTime;

// STATIC: 0x00C7C218
static float s_frameTimes[30];

// STATIC: 0x00C7C290
static uint32_t s_frameTimeIndex;

// STATIC: 0x00C7C298
static CGxVertexPC s_graphTenths[10 * 4];

// STATIC: 0x00C7C518
static CGxVertexPC s_graphOnes[10 * 4];

// STATIC: 0x00C7C798
static CGxVertexPC s_graphTens[10 * 4];

// STATIC: 0x00C7CA18
static CGxVertexPC s_graphVerts[30 * 4];

// STATIC: 0x00C7D2B4
FT_Library g_FTLibrary;

// STATIC: 0x00C7D2B8
float g_indentNormWidth;

// STATIC: 0x00C7D2BC
float g_indentPixelWidth;

// STATIC: 0x00C7D2C0
uint8_t g_fontStatsEnabled;

// STATIC: 0x00C7D2C4
uint32_t g_heightPixels;

// STATIC: 0x00C7D2C8
uint32_t g_widthPixels;

// STATIC: 0x00C7D2CC
CGxShader* g_fontPixelShader;

// STATIC: 0x00C7D2D0
CGxShader* g_fontVertexShader[2];

// VERIFY: 4776da25 manual
// OFFSET: 0x006BCD60
static float GetAverageFrameRate() {
    float total = 0.0f;
    uint32_t index = s_frameTimeIndex;

    for (int32_t i = 0; i < 30; i++) {
        total += s_frameTimes[index++];

        if (index == 30) {
            index = 0;
        }
    }

    float average = total / 30.0f;

    if (average < 0.01f) {
        return 100.0f;
    }

    return 1.0f / average;
}

// VERIFY: 0aa83fdc manual
// OFFSET: 0x006BCDD0
const char* GxuFontGetFontName(CGxFont* face) {
    if (!face) {
        return nullptr;
    }

    return face->GetName();
}

// VERIFY: a0187b65 manual
// OFFSET: 0x006BCDF0
uint32_t GxuFontGetFontFlags(CGxFont* face) {
    if (!face) {
        return 0;
    }

    return face->m_fontFlags;
}

// VERIFY: 44a249e9 auto
// OFFSET: 0x006BCE10
int32_t GxuFontAddToBatch(CGxStringBatch* batch, CGxString* string) {
    if (!batch || !string) {
        return 0;
    }

    batch->AddString(string);

    return 1;
}

// VERIFY: c5694029 auto
// OFFSET: 0x006BCE40
void GxuFontRenderBatch(CGxStringBatch* batch) {
    if (batch) {
        batch->RenderBatch();
    }
}

// VERIFY: 3a236d08 auto
// OFFSET: 0x006BCE60
void GxuFontGetTextExtent(CGxFont* face, const char* text, uint32_t numBytes, float fontHeight, float* extent, float shadowWidth, float scale, float unused, uint32_t flags) {
    InternalGetTextExtent(face, text, numBytes, fontHeight, extent, flags, shadowWidth, scale);
}

// VERIFY: b1cd8666 auto
// OFFSET: 0x006BCEA0
uint32_t GxuFontGetMaxCharsWithinWidth(CGxFont* face, const char* text, float fontHeight, float maxWidth, uint32_t numBytes, float* extent, float shadowWidth, float scale, float unused, uint32_t flags) {
    return InternalGetMaxCharsWithinWidth(face, text, fontHeight, maxWidth, numBytes, extent, flags, shadowWidth, scale, nullptr, nullptr);
}

// VERIFY: 92aa6c25 manual
// OFFSET: 0x006BCEF0
uint32_t GxuFontGetMaxCharsWithinWidthFromEnd(CGxFont* face, const char* text, float fontHeight, float maxWidth, uint32_t numBytes, float* extent, float shadowWidth, float scale, float unused, uint32_t flags) {
    if (!text || !*text || !numBytes || maxWidth == 0.0f) {
        return 0;
    }

    float* widthArray = static_cast<float*>(alloca(sizeof(float) * numBytes));
    float totalWidth;
    uint32_t bytesInString;
    uint32_t numChars = InternalGetMaxCharsWithinWidth(face, text, fontHeight, 10000.0f, numBytes, &totalWidth, flags, shadowWidth, scale, &bytesInString, widthArray);

    if (maxWidth >= totalWidth) {
        if (extent) {
            *extent = totalWidth;
        }

        return numChars;
    }

    float excess = totalWidth - maxWidth;

    for (uint32_t i = 0; i < numChars; i++) {
        if (widthArray[i] > excess) {
            if (extent) {
                *extent = totalWidth - widthArray[i];
            }

            return numChars - i;
        }
    }

    if (extent) {
        *extent = 0.0f;
    }

    return 0;
}

// VERIFY: f1a25b15 auto
// OFFSET: 0x006BD020
float GxuFontGetOneToOneHeight(CGxFont* face) {
    if (!face) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0.0f;
    }

    return static_cast<float>(face->GetPixelSize()) / static_cast<float>(g_heightPixels);
}

// VERIFY: 1ebc6f2b auto
// OFFSET: 0x006BD070
int32_t GxuFontSetStringColor(CGxString* string, const CImVector& color) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    string->SetColor(color);

    return 1;
}

// VERIFY: 4d7d4971 auto
// OFFSET: 0x006BD0A0
void GxuFontSetStringPosition(CGxString* string, const C3Vector& position) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    string->SetStringPosition(position);
}

// VERIFY: 96cf026e auto
// OFFSET: 0x006BD0C0
void GxuFontAddShadow(CGxString* string, const CImVector& color, const C2Vector& offset) {
    if (string && !(string->m_flags & 0x80)) {
        string->AddShadow(offset, color);
    }
}

// VERIFY: bfef3566 auto
// OFFSET: 0x006BD0E0
void GxuFontRemoveShadow(CGxString* string) {
    if (string) {
        string->RemoveShadow();
    }
}

// VERIFY: 6fa02ff0 auto
// OFFSET: 0x006BD100
uint32_t GxuFontStringHyperLinkInfo(CGxString* string, const GXUFONTHYPERLINKINFO*& info) {
    if (!string) {
        return 0;
    }

    return string->GetHyperLinkInfo(info);
}

// VERIFY: 210ab3c4 auto
// OFFSET: 0x006BD120
uint32_t GxuFontStringEmbeddedTextureInfo(CGxString* string, const GXUEMBEDDEDTEXTUREINFO*& info) {
    if (!string) {
        return 0;
    }

    return string->GetEmbeddedTextureInfo(info);
}

// VERIFY: c2cd8ba9 auto
// OFFSET: 0x006BD140
int32_t GxuFontStringSetGradient(CGxString* string, int32_t start, int32_t length) {
    if (!string) {
        return -1;
    }

    return string->SetGradient(start, length);
}

// VERIFY: f811e579 manual
// OFFSET: 0x006BD160
static void InitializeFrameGraph() {
    CImVector red;
    red.b = 0x00;
    red.g = 0x00;
    red.r = 0xFF;
    red.a = 0xFF;

    float x = 0.01f;

    for (CGxVertexPC* vert = s_graphTens; vert < s_graphTens + 10 * 4; vert += 4) {
        vert[0].p = C3Vector(x, 0.005f, 1.0f);
        vert[0].c = red;
        float right = x + 0.01f;
        vert[1].p = C3Vector(right, 0.005f, 1.0f);
        vert[1].c = red;
        vert[2].p = C3Vector(x, 0.025f, 1.0f);
        vert[2].c = red;
        vert[3].p = C3Vector(right, 0.025f, 1.0f);
        vert[3].c = red;
        x = x + 0.02f;
    }

    CImVector yellow;
    yellow.b = 0x00;
    yellow.g = 0xFF;
    yellow.r = 0xFF;
    yellow.a = 0xFF;

    x = 0.01f;

    for (CGxVertexPC* vert = s_graphOnes; vert < s_graphOnes + 10 * 4; vert += 4) {
        vert[0].p = C3Vector(x, 0.035f, 1.0f);
        vert[0].c = yellow;
        float right = x + 0.01f;
        vert[1].p = C3Vector(right, 0.035f, 1.0f);
        vert[1].c = yellow;
        vert[2].p = C3Vector(x, 0.055f, 1.0f);
        vert[2].c = yellow;
        vert[3].p = C3Vector(right, 0.055f, 1.0f);
        vert[3].c = yellow;
        x = x + 0.02f;
    }

    CImVector green;
    green.b = 0x00;
    green.g = 0xFF;
    green.r = 0x00;
    green.a = 0xFF;

    x = 0.01f;

    for (CGxVertexPC* vert = s_graphTenths; vert < s_graphTenths + 10 * 4; vert += 4) {
        vert[0].p = C3Vector(x, 0.065f, 1.0f);
        vert[0].c = green;
        float right = x + 0.01f;
        vert[1].p = C3Vector(right, 0.065f, 1.0f);
        vert[1].c = green;
        vert[2].p = C3Vector(x, 0.085f, 1.0f);
        vert[2].c = green;
        vert[3].p = C3Vector(right, 0.085f, 1.0f);
        vert[3].c = green;
        x = x + 0.02f;
    }

    uint16_t* index = s_graphIndices;

    for (int32_t quad = 0; index < s_graphIndices + 30 * 6; quad++) {
        index[0] = 4 * quad;
        index[1] = 4 * quad + 1;
        index[2] = 4 * quad + 2;
        index[3] = 4 * quad + 1;
        index[4] = 4 * quad + 3;
        index[5] = 4 * quad + 2;
        index += 6;
    }
}

// VERIFY: 1ffd4bc1 manual
// OFFSET: 0x006BD390
uint32_t GxuFontGetMaxCharsWithinWidthAndHeight(CGxFont* face, const char* text, float fontHeight, float blockWidth, float blockHeight, uint32_t numBytes, float shadowWidth, float scale, float spacing, uint32_t flags) {
    uint32_t numChars = 0;

    if (!face) {
        return 0;
    }

    if (!text || !*text || fontHeight == 0.0f || blockWidth == 0.0f || blockHeight == 0.0f || !numBytes || !g_heightPixels) {
        return 0;
    }

    if (flags & 4) {
        fontHeight = GxuFontGetOneToOneHeight(face);
    }

    float heightPixels = static_cast<float>(g_heightPixels);
    float lineSpacing = static_cast<float>(CeilToInt64(heightPixels * spacing)) / heightPixels;
    uint32_t numLines = 0;
    bool wordBreak = true;
    float height = 0.0f;
    const char* currentText = text;

    while (*currentText) {
        if ((flags & 0x2000) && numLines == 1) {
            blockWidth -= g_indentNormWidth;
        }

        uint32_t lineBytes;
        float lineWidth;
        const char* nextText;
        CalcWrapPoint(face, currentText, fontHeight, blockWidth, &lineBytes, &lineWidth, &nextText, shadowWidth, flags, &wordBreak, nullptr, scale);

        if (nextText == currentText) {
            break;
        }

        numLines++;

        if (blockHeight + numLines * 0.00000095367431640625 < height + fontHeight) {
            break;
        }

        numChars += nextText - currentText;
        currentText = nextText;
        height = height + fontHeight + lineSpacing;

        if (!wordBreak || !nextText) {
            break;
        }
    }

    return numChars;
}

// VERIFY: 985bcc6b auto
// OFFSET: 0x006BD550
C3Vector GxuFontGetStringPosition(CGxString* string) {
    C3Vector position = string ? string->m_position : C3Vector(0.0f, 0.0f, 0.0f);

    return position;
}

// VERIFY: 4c66c41f manual
// OFFSET: 0x006BD5A0
QUOTEDCODE GxuDetermineQuotedCode(const char* text, int32_t& advance, CImVector* color, uint32_t flags, uint32_t& wide) {
    uint32_t ignoreColor = flags & 0x100;
    uint32_t ignoreNewLine = flags & 0x200;
    uint32_t ignoreHyperlink = flags & 0x400;
    uint32_t ignorePipe = flags & 0x800;
    uint32_t ignoreTexture = flags & 0x1000;

    uint32_t code = sgetu8(reinterpret_cast<const uint8_t*>(text), &advance);
    wide = code;
    int32_t codeAdvance = advance;

    switch (code) {
        case 0:
        case 0xFFFFFFFF:
            return CODE_INVALIDCODE;

        case '\r':
            advance = sgetu8(reinterpret_cast<const uint8_t*>(text + 1), &advance) == '\n' ? 2 : 1;
            return CODE_NEWLINE;

        case '\n':
            advance = 1;
            return CODE_NEWLINE;
    }

    if (code != '|' || ignorePipe) {
        return CODE_INVALIDCODE;
    }

    const char* next = text + codeAdvance;

    switch (*next) {
        case 'C':
        case 'c': {
            if (ignoreColor) {
                break;
            }

            const char* hex = next + 1;
            uint8_t components[4];

            for (int32_t i = 0; i < 4; i++) {
                if (!hex[0] || !hex[1]) {
                    advance = codeAdvance;
                    wide = code;
                    return CODE_INVALIDCODE;
                }

                char buffer[3];
                buffer[0] = hex[0];
                buffer[1] = hex[1];
                buffer[2] = '\0';
                hex += 2;

                char* end = nullptr;
                components[i] = static_cast<uint8_t>(strtol(buffer, &end, 16));

                if (end && *end) {
                    advance = codeAdvance;
                    wide = code;
                    return CODE_INVALIDCODE;
                }
            }

            if (color) {
                CImVector value;
                value.b = components[3];
                value.g = components[2];
                value.r = components[1];
                value.a = 0xFF;
                *color = value;
            }

            advance = 10;
            return CODE_COLORON;
        }

        case 'H': {
            if (ignoreHyperlink) {
                break;
            }

            const char* current = next + codeAdvance;

            if (!*current) {
                advance = codeAdvance;
                break;
            }

            while (true) {
                uint32_t wideChar = sgetu8(reinterpret_cast<const uint8_t*>(current), &advance);
                current += advance;

                if (wideChar == '|') {
                    break;
                }

                if (!*current) {
                    advance = codeAdvance;
                    wide = code;
                    return CODE_INVALIDCODE;
                }
            }

            if (*current) {
                uint32_t wideChar = sgetu8(reinterpret_cast<const uint8_t*>(current), &advance);
                current += advance;

                if (*current && wideChar == 'h' && current - text != 4 && !(current[0] == '|' && current[1] == 'h')) {
                    advance = current - text;
                    return CODE_HYPERLINKSTART;
                }
            }

            advance = codeAdvance;
            wide = code;
            return CODE_INVALIDCODE;
        }

        case 'N':
        case 'n':
            if (ignoreNewLine) {
                break;
            }

            advance = 2;
            return CODE_NEWLINE;

        case 'R':
        case 'r':
            if (ignoreColor) {
                break;
            }

            advance = 2;
            return CODE_COLORRESTORE;

        case 'T': {
            if (ignoreTexture) {
                break;
            }

            const char* current = next + codeAdvance;

            if (!*current) {
                advance = codeAdvance;
                break;
            }

            while (true) {
                uint32_t wideChar = sgetu8(reinterpret_cast<const uint8_t*>(current), &advance);
                current += advance;

                if (wideChar == '|') {
                    break;
                }

                if (!*current) {
                    advance = codeAdvance;
                    wide = code;
                    return CODE_INVALIDCODE;
                }
            }

            if (*current) {
                uint32_t wideChar = sgetu8(reinterpret_cast<const uint8_t*>(current), &advance);
                current += advance;

                if (wideChar == 't' && current - text != 4) {
                    advance = current - text;
                    return CODE_TEXTURESTART;
                }
            }

            advance = codeAdvance;
            wide = code;
            return CODE_INVALIDCODE;
        }

        case 'h':
            if (ignoreHyperlink) {
                break;
            }

            advance = 2;
            return CODE_HYPERLINKSTOP;

        case 't':
            if (ignoreTexture) {
                break;
            }

            advance = 2;
            return CODE_TEXTURESTOP;

        case '|':
            advance = 2;
            return CODE_PIPE;
    }

    wide = code;
    return CODE_INVALIDCODE;
}

// VERIFY: f492d0d9 manual
// OFFSET: 0x006BD900
float GxuFontGetWrappedTextHeight(CGxFont* face, const char* text, float fontHeight, float blockWidth, const C2Vector& shadowOffset, float scale, float spacing, uint32_t flags) {
    uint32_t numLines = 0;

    if (!face) {
        DebugValidate("face");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0.0f;
    }

    if (!text) {
        DebugValidate("text");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0.0f;
    }

    if (flags & 4) {
        fontHeight = GxuFontGetOneToOneHeight(face);
    }

    float lineWidth = 0.0f;
    const char* nextText = nullptr;
    float extraHeight = 0.0f;
    int32_t advance = 0;
    float lineHeight = 0.0f;
    uint32_t wide = 0;
    bool wordBreak = true;
    const char* currentText = text;

    do {
        if (!*currentText) {
            break;
        }

        if ((flags & 0x2000) && numLines == 1) {
            blockWidth -= g_indentNormWidth;
        }

        if (GxuDetermineQuotedCode(currentText, advance, nullptr, flags, wide) == CODE_NEWLINE) {
            nextText = currentText + advance;
        } else {
            uint32_t lineBytes;
            CalcWrapPoint(face, currentText, fontHeight, blockWidth, &lineBytes, &lineWidth, &nextText, shadowOffset.x, flags, &wordBreak, &lineHeight, scale);

            if (currentText == nextText) {
                break;
            }
        }

        numLines++;
        currentText = nextText;

        float fontPixelHeight = static_cast<float>(g_heightPixels) * fontHeight;

        if (fontPixelHeight < lineHeight) {
            extraHeight = lineHeight - fontPixelHeight + extraHeight;
        }

        if (flags & 2) {
            break;
        }
    } while (nextText);

    if (!(flags & 2) && currentText > text && GxuDetermineQuotedCode(currentText - 1, advance, nullptr, flags, wide) == CODE_NEWLINE) {
        numLines++;
    }

    if (!numLines) {
        return 0.0f;
    }

    float heightPixels = static_cast<float>(g_heightPixels);
    float lineSpacing = static_cast<float>(CeilToInt64(heightPixels * spacing)) / heightPixels;

    return extraHeight / heightPixels + lineSpacing * static_cast<float>(numLines - 1) + static_cast<float>(numLines) * fontHeight;
}

// VERIFY: c0427aaf auto
// OFFSET: 0x006BDB10
uint32_t GxuFontWrapText(CGxFont* face, const char* text, uint32_t textLength, float fontHeight, float blockWidth, uint32_t* lineStarts, uint32_t maxLines, float shadowWidth, float scale, float unused, uint32_t flags) {
    if (!text || !*text || !textLength || fontHeight == 0.0f || blockWidth == 0.0f) {
        return 0;
    }

    if (flags & 4) {
        fontHeight = GxuFontGetOneToOneHeight(face);
    }

    const char* currentText = text;
    uint32_t numLines = 0;
    bool wordBreak = true;
    const char* end = text + textLength;

    do {
        if (currentText >= end) {
            break;
        }

        if ((flags & 0x2000) && numLines == 1) {
            blockWidth -= g_indentNormWidth;
        }

        if (numLines < maxLines) {
            lineStarts[numLines] = currentText - text;
        }

        numLines++;

        int32_t advance;
        uint32_t wide;

        if (GxuDetermineQuotedCode(currentText, advance, nullptr, flags, wide) == CODE_NEWLINE) {
            currentText += advance;
        } else {
            uint32_t lineBytes;
            float lineWidth;
            const char* nextText;
            CalcWrapPoint(face, currentText, fontHeight, blockWidth, &lineBytes, &lineWidth, &nextText, shadowWidth, flags, &wordBreak, nullptr, scale);

            if (currentText == nextText) {
                break;
            }

            currentText = nextText;
        }
    } while (currentText);

    if (end[-1] == '\n') {
        if (numLines < maxLines) {
            lineStarts[numLines] = textLength;
        }

        numLines++;
    }

    return numLines;
}

// VERIFY: cdaa59a6 manual
// OFFSET: 0x006BDC50
const char* GxuFontStripEscapeCodes(const char* text, uint32_t textLength, uint32_t flags, char* buffer, uint32_t bufferSize) {
    *buffer = '\0';

    if (!text || !*text || !textLength) {
        SStrPrintf(buffer, bufferSize, "%s", text);
        return text;
    }

    uint32_t length = 0;
    uint32_t remaining = textLength;

    while (*text && remaining) {
        const char* start = text;
        int32_t advance;
        uint32_t wide;
        QUOTEDCODE code = GxuDetermineQuotedCode(text, advance, nullptr, 0, wide);

        switch (code) {
            case CODE_COLORON:
            case CODE_COLORRESTORE:
            case CODE_HYPERLINKSTART:
            case CODE_TEXTURESTART:
                if (flags & s_quotedCodeInfo[code].m_ignoreFlag) {
                    text += advance;
                    remaining -= advance;
                    continue;
                }

                break;

            case CODE_NEWLINE:
            case CODE_PIPE:
            case CODE_HYPERLINKSTOP:
            case CODE_TEXTURESTOP:
                remaining -= advance;
                text += advance;

                if (flags & s_quotedCodeInfo[code].m_ignoreFlag) {
                    continue;
                }

                if (length < bufferSize - advance) {
                    if (s_quotedCodeInfo[code].m_escaped) {
                        buffer[length++] = '|';
                    }

                    buffer[length++] = s_quotedCodeInfo[code].m_code;
                    continue;
                }

                break;
        }

        if (length >= bufferSize - advance) {
            break;
        }

        for (int32_t i = 0; i < advance; i++) {
            buffer[length++] = start[i];
        }

        text = start + advance;
        remaining -= advance;
    }

    buffer[length] = '\0';

    return buffer;
}

// VERIFY: 15022de8 auto
// OFFSET: 0x006BDE80
int32_t GxuFontCreateFont(const char* name, float fontHeight, CGxFont*& face, uint32_t flags) {
    if (!name) {
        DebugValidate("name");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!*name) {
        DebugValidate("*name");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!(fontHeight <= 1.0f && fontHeight > 0.0f)) {
        DebugValidate("( fontHeight <= 1.0f ) && ( fontHeight > 0.0f )");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    CGxFont* newFace = g_fonts.NewNode(2, 0, 0);

    if (flags & 8) {
        flags |= 1;
    }

    int32_t result = newFace->Initialize(name, flags, fontHeight);

    if (!result) {
        g_fonts.DeleteNode(newFace);
        newFace = nullptr;
    }

    face = newFace;

    return result;
}

// VERIFY: 96f05107 auto
// OFFSET: 0x006BDF90
void GxuFontDestroyFont(CGxFont*& face) {
    if (face) {
        CGxFont* ptr = face;
        ptr->~CGxFont();
        SMemFree(ptr, typeid(CGxFont).raw_name(), SLOG_OBJECT, 0);
    }

    face = nullptr;
}

// VERIFY: ddb1d650 auto
// OFFSET: 0x006BDFC0
void GxuFontDestroyString(CGxString*& string) {
    if (string) {
        string->m_link.Unlink();
        string->Recycle();
        string = nullptr;
    }
}

// VERIFY: d017d2a4 manual
// OFFSET: 0x006BE020
void GxuFontWindowSizeChanged() {
    static CRect s_currentRect = { 0.0f, 0.0f, 0.0f, 0.0f };

    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSize(rect);

    if (rect.maxX - rect.minX == 0.0f || rect.maxY - rect.minY == 0.0f) {
        CRect defaultRect = { 0.0f, 0.0f, 480.0f, 640.0f };
        rect = defaultRect;
    }

    if (s_currentRect.minY != rect.minY || s_currentRect.minX != rect.minX || s_currentRect.maxY != rect.maxY || s_currentRect.maxX != rect.maxX) {
        s_currentRect = rect;

        g_widthPixels = static_cast<uint32_t>(rect.maxX - rect.minX);
        g_heightPixels = static_cast<uint32_t>(rect.maxY - rect.minY);
        g_indentPixelWidth = 15.0f;
        g_indentNormWidth = 15.0f / static_cast<float>(g_widthPixels);

        for (CGxFont* face = g_fonts.Head(); face; face = g_fonts.Next(face)) {
            face->HandleScreenSizeChange();
        }
    }
}

// VERIFY: c5d92c78 auto
// OFFSET: 0x006BE230
void GxuFontInitialize() {
    g_theGxDevicePtr->ShaderCreate(g_fontVertexShader, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    g_theGxDevicePtr->ShaderCreate(&g_fontPixelShader, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);

    BATCHEDRENDERFONTDESC::Initialize();

    FT_New_Library(&s_GxuMemoryRecord, &g_FTLibrary);
    FT_Add_Default_Modules(g_FTLibrary);

    GxuFontWindowSizeChanged();
    InitializeFrameGraph();
}

// VERIFY: 57f9dc07 manual
// OFFSET: 0x006BE2B0
int32_t GxuFontCreateString(CGxFont* face, const char* text, float fontHeight, const C3Vector& position, float blockWidth, float blockHeight, float spacing, CGxString*& string, EGxFontVJusts vertJust, EGxFontHJusts horzJust, uint32_t flags, const CImVector& color, float unused, float scale) {
    if (!face) {
        DebugValidate("face");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!text) {
        DebugValidate("text");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (fontHeight == 0.0f && !(flags & 4)) {
        DebugValidate("fontHeight || ( flags & EGxStringFlags_FixedSize )");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (vertJust >= GxVJ_Last) {
        DebugValidate("vertJustification < GxVJ_Last");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (horzJust >= GxHJ_Last) {
        DebugValidate("horzJustification < GxHJ_Last");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    CGxString* newString = CGxString::GetNewString(1);
    int32_t result = newString->Initialize(fontHeight, position, blockWidth, blockHeight, face, text, vertJust, horzJust, spacing, flags & ~1, color, scale);

    if (!result) {
        GxuFontDestroyString(newString);
    }

    string = newString;

    return result;
}

// VERIFY: 2dfebcd7 auto
// OFFSET: 0x006BE390
int32_t GxuFontClearBatch(CGxStringBatch* batch) {
    if (!batch) {
        return 0;
    }

    batch->m_fontBatch.InternalClear(0);

    return 1;
}

// VERIFY: c7f3f7d3 auto
// OFFSET: 0x006BE3B0
int32_t GxuFontDestroyBatch(CGxStringBatch* batch) {
    if (!batch) {
        return 0;
    }

    batch->m_fontBatch.InternalClear(0);
    s_freeBatches.LinkToTail(batch);

    return 1;
}

// VERIFY: 86ae417d manual
// OFFSET: 0x006BE3E0
void GxuFontUpdate() {
    for (CGxString* string = g_strings.Head(); string; string = g_strings.Next(string)) {
        string->Tick();
    }

    if (!g_fontStatsEnabled) {
        return;
    }

    uint32_t time = OsGetAsyncTimeMs();
    s_frameTimes[s_frameTimeIndex] = (time - s_lastFrameTime) / 1000.0;

    if (++s_frameTimeIndex == 30) {
        s_frameTimeIndex = 0;
    }

    s_lastFrameTime = time;

    float frameRate = GetAverageFrameRate();
    int32_t whole = static_cast<int32_t>(frameRate);
    int32_t numVerts = 0;

    for (int32_t i = 0; i < whole / 10 * 4; i++) {
        s_graphVerts[numVerts++] = s_graphTens[i];
    }

    for (int32_t i = 0; i < whole % 10 * 4; i++) {
        s_graphVerts[numVerts++] = s_graphOnes[i];
    }

    int32_t tenths = static_cast<int32_t>(frameRate * 10.0f) % 10;

    for (int32_t i = 0; i < tenths * 4; i++) {
        s_graphVerts[numVerts++] = s_graphTenths[i];
    }

    if (!numVerts) {
        return;
    }

    C44Matrix view;
    GxXformView(view);

    C44Matrix projection;
    GxXformProjection(projection);

    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

    C44Matrix ortho;
    GxuXformCreateOrtho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, ortho);
    GxXformSetProjection(ortho);

    C44Matrix identity;
    GxXformSetView(identity);

    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    CGxBuf* vertexBuf = GxBufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPC), numVerts);
    memcpy(GxBufLock(vertexBuf), s_graphVerts, sizeof(CGxVertexPC) * numVerts);
    GxBufUnlock(vertexBuf, 0);

    int32_t numIndices = 6 * (numVerts / 4);
    CGxBuf* indexBuf = GxBufStream(GxPoolTarget_Index, sizeof(uint16_t), numIndices);
    memcpy(GxBufLock(indexBuf), s_graphIndices, sizeof(uint16_t) * numIndices);
    GxBufUnlock(indexBuf, 0);

    GxPrimVertexPtr(vertexBuf, GxVBF_PC);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_BlendingMode, 0);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_ColorOp0, 0);
    GxRsSet(GxRs_AlphaOp0, 3);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_ColorWrite, 15);

    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = numIndices;
    batch.m_minIndex = 0;
    batch.m_maxIndex = numIndices - 1;
    GxDraw(&batch, 1);

    g_theGxDevicePtr->RsPop();

    GxXformSetView(view);
    GxXformSetProjection(projection);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

// VERIFY: 4345db69 manual
// OFFSET: 0x006BF160
CGxStringBatch* GxuFontCreateBatch(bool billboarded, bool worldSpace) {
    CGxStringBatch* batch = s_freeBatches.Head();

    if (batch) {
        s_freeBatches.UnlinkNode(batch);
    } else {
        batch = new (SMemAlloc(sizeof(CGxStringBatch), typeid(CGxStringBatch).raw_name(), SLOG_OBJECT, SMEM_FLAG_ZEROMEMORY)) CGxStringBatch;
    }

    if (billboarded) {
        batch->m_flags |= 1;
    } else {
        batch->m_flags &= ~1;
    }

    if (worldSpace) {
        batch->m_flags |= 2;
    } else {
        batch->m_flags &= ~2;
    }

    return batch;
}

// VERIFY: 4ee8bc9c manual
// OFFSET: 0x006BF2D0
void GxuFontShutdown() {
    g_strings.Clear();
    g_fonts.Clear();
    s_freeBatches.Clear();
    g_freeStrings.Clear();

    TEXTLINETEXTURE::Shutdown();
    BATCHEDRENDERFONTDESC::Shutdown();

    if (g_FTLibrary) {
        FT_Done_Library(g_FTLibrary);
    }

    g_FTLibrary = nullptr;
    g_fontStatsEnabled = 0;

    for (int32_t i = 0; i < 2; i++) {
        if (g_fontVertexShader[i]) {
            g_theGxDevicePtr->ShaderDestroy(g_fontVertexShader[i]);
        }
    }

    if (g_fontPixelShader) {
        g_theGxDevicePtr->ShaderDestroy(g_fontPixelShader);
    }
}
