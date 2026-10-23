#include <cmath>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gxu/GxuFontString.h"
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Gxu/GxuFontUtil.h"
#include "Engine/Source/Gxu/GxuFontMiscClasses.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRect.h"

// VERIFY: acac9576 auto
// OFFSET: 0x006C5E60
void CGxString::RemoveShadow() {
    this->m_flags &= ~1;
}

// VERIFY: d84bd657 auto
// OFFSET: 0x006C5E70
void CGxString::TexturePageEvicted(uint32_t texturePage) {
    if (this->m_texturePagesUsed & (1 << texturePage)) {
        this->m_textureEvicted = 1;
    }
}

// VERIFY: 491c16e5 manual
// OFFSET: 0x006C5E90
void CGxString::WriteGeometry(CGxVertexPCT* buf, int32_t texturePage, int32_t vertIndex, int32_t numVerts) const {
    TEXTLINETEXTURE* textLine = this->m_textLines[texturePage];

    if (textLine) {
        textLine->WriteGeometry(buf, this->m_fontColor, this->m_shadowOffset, this->m_shadowColor, this->m_viewTranslation, this->m_flags & 1, (this->m_flags >> 5) & 1, vertIndex, numVerts);
    }
}

// VERIFY: d917ad36 auto
// OFFSET: 0x006C0AF0
GXUEMBEDDEDTEXTUREINFO::GXUEMBEDDEDTEXTUREINFO() {
    this->m_lineHeight = 0.0f;
    this->m_left = 0.0f;
    this->m_path = nullptr;
    this->m_baseLineHeight = 0.0f;
    this->m_pathLength = 0;
    this->m_right = 0.0f;
    this->m_offsetX = 0.0f;
    this->m_offsetY = 0.0f;
    this->m_texCoords.minY = 0.0f;
    this->m_texCoords.minX = 0.0f;
    this->m_texCoords.maxY = 0.0f;
    this->m_texCoords.maxX = 0.0f;
}

// VERIFY: f12e437f auto
// OFFSET: 0x006C60C0
EMBEDDEDPARSEINFO::EMBEDDEDPARSEINFO() {
    this->m_state = HYPERLINKNONE;
    this->m_hyperlink.m_lineHeight = 0.0f;
    this->m_hyperlink.m_indentation = 0.0f;
    this->m_hyperlink.m_linkData = nullptr;
    this->m_hyperlink.m_baseLineHeight = 0.0f;
    this->m_hyperlink.m_linkDataLength = 0;
    this->m_hyperlink.m_right = 0.0f;
    this->m_hyperlink.m_linkText = nullptr;
    this->m_hyperlink.m_linkTextLength = 0;
    this->m_hyperlink.m_colorCode = nullptr;
    this->m_hyperlink.m_colorCodeLength = 0;
}

// VERIFY: b6cefdc6 manual
// OFFSET: 0x006C6110
void CGxString::SetGradientAlpha(const GRADIENTINFO& info, uint8_t alpha) {
    if (info.m_texturePage >= 8) {
        return;
    }

    TEXTLINETEXTURE* textLine = this->m_textLines[info.m_texturePage];

    if (textLine && info.m_vertIndex < textLine->m_colors.m_count) {
        textLine->m_colors.m_data[info.m_vertIndex].a = alpha;
    }
}

// VERIFY: 63604a46 manual
// OFFSET: 0x006C6150
uint8_t CGxString::GetGradientAlpha(const GRADIENTINFO& info) {
    if (info.m_texturePage >= 8) {
        return 0;
    }

    TEXTLINETEXTURE* textLine = this->m_textLines[info.m_texturePage];

    if (!textLine || info.m_vertIndex >= textLine->m_colors.m_count) {
        return 0;
    }

    return textLine->m_colors.m_data[info.m_vertIndex].a;
}

// VERIFY: 65e79cec manual
// OFFSET: 0x006C6190
void CGxString::InitializeViewTranslation() {
    this->m_viewTranslation = this->m_position;

    if (!this->m_numLines || (this->m_flags & 0x80)) {
        return;
    }

    if (this->m_horzJust == GxHJ_Right) {
        this->m_viewTranslation.x = this->m_blockWidth + this->m_viewTranslation.x;
    } else if (this->m_horzJust == GxHJ_Center) {
        this->m_viewTranslation.x = this->m_blockWidth * 0.5 + this->m_viewTranslation.x;
    }

    float heightPixels = g_heightPixels;
    float spacingPixels = this->m_spacing * heightPixels;
    float spacing = static_cast<uint32_t>(spacingPixels + 0.99995) / heightPixels;
    float textHeight = spacing * (this->m_numLines - 1) + this->m_numLines * this->m_currentFontHeight;

    for (uint32_t i = 0; i < this->m_embeddedTextureInfo.m_count; i++) {
        if (this->m_currentFontHeight < this->m_embeddedTextureInfo.m_data[i].m_height) {
            textHeight = this->m_embeddedTextureInfo.m_data[i].m_height - this->m_currentFontHeight + textHeight;
        }
    }

    if (this->m_vertJust == GxVJ_Top) {
        this->m_viewTranslation.y = this->m_blockHeight + this->m_viewTranslation.y;
    } else if (this->m_vertJust == GxVJ_Middle) {
        this->m_viewTranslation.y = 0.5 * (this->m_blockHeight - textHeight) + textHeight + this->m_viewTranslation.y;
    } else {
        this->m_viewTranslation.y = this->m_viewTranslation.y + textHeight;
    }

    float x = g_widthPixels * this->m_viewTranslation.x;
    this->m_viewTranslation.x = floor(x);

    float y = g_heightPixels * this->m_viewTranslation.y;
    this->m_viewTranslation.y = floor(y);
}

// VERIFY: 4d328ee6 auto
// OFFSET: 0x006C63E0
int32_t CGxString::CalculateVertsNeeded(int32_t texturePage) {
    if (this->m_flags & 1) {
        return 2 * this->m_textLines[texturePage]->m_verts.m_count;
    }

    return this->m_textLines[texturePage]->m_verts.m_count;
}

// VERIFY: 9ad1b804 auto
// OFFSET: 0x006C6410
void CGxString::SetStringPosition(const C3Vector& position) {
    this->m_position = position;
    this->InitializeViewTranslation();
}

// VERIFY: c663729d manual
// OFFSET: 0x006C6A70
void CGxString::AddHyperlinkParseInfo(GXUFONTHYPERLINKINFO info) {
    float heightPixels = g_heightPixels;
    info.m_lineHeight = info.m_lineHeight / heightPixels;
    info.m_baseLineHeight = info.m_baseLineHeight / heightPixels;

    float widthPixels = g_widthPixels;
    info.m_indentation = info.m_indentation / widthPixels;
    info.m_right = info.m_right / widthPixels;

    this->m_hyperlinkInfo.Add(1, &info);
}

// VERIFY: c6a61c76 manual
// OFFSET: 0x006C6AF0
void CGxString::AddEmbeddedTextureParseInfo(GXUEMBEDDEDTEXTUREINFO info) {
    float heightPixels = g_heightPixels;
    info.m_lineHeight = info.m_lineHeight / heightPixels;
    info.m_baseLineHeight = info.m_baseLineHeight / heightPixels;

    float widthPixels = g_widthPixels;
    info.m_left = info.m_left / widthPixels;
    info.m_right = info.m_right / widthPixels;
    info.m_height = info.m_height / heightPixels;
    info.m_width = info.m_width / widthPixels;
    info.m_offsetX = info.m_offsetX / widthPixels;
    info.m_offsetY = info.m_offsetY / heightPixels;

    this->m_embeddedTextureInfo.Add(1, &info);
}

// VERIFY: 40444632 manual
// OFFSET: 0x006C6B90
void CGxString::ClearInstanceData() {
    this->m_hyperlinkInfo.SetCount(0);
    this->m_embeddedTextureInfo.SetCount(0);
    this->m_gradientInfo.SetCount(0);
    this->m_lastGradientStart = -1;
    this->m_lastGradientLength = -1;
    this->m_numLines = 0;

    for (uint32_t i = 0; i < 8; i++) {
        if (this->m_textLines[i]) {
            TEXTLINETEXTURE::Recycle(this->m_textLines[i]);
            this->m_textLines[i] = nullptr;
        }
    }
}

// VERIFY: 50ba3e99 manual
// OFFSET: 0x006C6C00
void CGxString::Tick() {
    if (this->m_numLines && this->m_idleTicks++ > 30) {
        this->ClearInstanceData();
    }
}

// VERIFY: 80d68fb6 manual
// OFFSET: 0x006C6C30
void CGxString::SetColor(const CImVector& color) {
    if (*reinterpret_cast<const uint32_t*>(&color) == *reinterpret_cast<const uint32_t*>(&this->m_fontColor)) {
        return;
    }

    this->m_fontColor = color;
    this->m_shadowColor.a = color.a < this->m_shadowColor.a ? color.a : this->m_shadowColor.a;

    if (!(this->m_flags & 8) || (this->m_flags & 0x20) || this->m_gradientInfo.m_count) {
        this->ClearInstanceData();
    }
}

// VERIFY: ef8566ca manual
// OFFSET: 0x006C6C70
void CGxString::HandleScreenSizeChange() {
    if (this->m_flags & 4) {
        this->m_requestedFontHeight = GxuFontGetOneToOneHeight(this->m_currentFace);
    }

    double minHeight = 2.0 / g_heightPixels;
    this->m_currentFontHeight = this->m_requestedFontHeight > minHeight ? this->m_requestedFontHeight : minHeight;

    this->ClearInstanceData();
}

// VERIFY: 6a878b65 manual
// OFFSET: 0x006C6CD0
void CGxString::InitializeTextLine(const char* text, uint32_t numBytes, CImVector& color, const C3Vector& position, uint32_t* texturePagesUsed, EMBEDDEDPARSEINFO& parseInfo) {
    if (this->m_flags & 8) {
        for (uint32_t i = 0; i < 8; i++) {
            if (this->m_textLines[i]) {
                this->m_textLines[i]->m_colors.SetCount(0);
            }
        }
    }

    C3Vector pos = position;
    uint32_t lastCode = 0;

    float fontPixelHeight = ScreenToPixelHeight((this->m_flags >> 7) & 1, this->m_currentFontHeight);
    double pixelHeight = fontPixelHeight;
    float scale = pixelHeight / this->m_currentFace->GetPixelSize();
    float cellHeight = ScreenToPixelHeight((this->m_flags >> 7) & 1, this->m_currentFontHeight);

    if (this->m_currentFace->m_fontFlags & 8) {
        cellHeight = cellHeight + 4.0;
    } else if (this->m_currentFace->m_fontFlags & 1) {
        cellHeight = cellHeight + 2.0;
    }

    if (parseInfo.m_state == HYPERLINKDISPLAY) {
        parseInfo.m_hyperlink.m_right = pos.x;
    }

    static CRect pixelAdjustments = { 1.0f / 512.0f, 1.0f / 512.0f, -1.0f / 512.0f, -1.0f / 512.0f };

    float step = 0.0f;
    float x = 0.0f;

    if (*text) {
        while (numBytes) {
            CImVector codeColor;
            *reinterpret_cast<uint32_t*>(&codeColor) = 0;

            int32_t advance;
            uint32_t wide;
            QUOTEDCODE code = GxuDetermineQuotedCode(text, advance, &codeColor, this->m_flags, wide);

            text += advance;
            numBytes -= advance;

            if (lastCode) {
                if (this->m_flags & 0x10) {
                    step = this->m_currentFace->ComputeStepFixedWidth(lastCode, wide);
                } else {
                    step = this->m_currentFace->ComputeStep(lastCode, wide);
                }
            }

            x = step * scale;

            switch (code) {
                case CODE_COLORON: {
                    if (!(this->m_flags & 8)) {
                        codeColor.a = this->m_fontColor.a;
                        color = codeColor;
                    }

                    parseInfo.m_hyperlink.m_colorCode = text - advance;
                    parseInfo.m_hyperlink.m_colorCodeLength = advance;
                    break;
                }

                case CODE_COLORRESTORE: {
                    color = this->m_fontColor;
                    break;
                }

                case CODE_NEWLINE:
                case CODE_TEXTURESTOP: {
                    break;
                }

                case CODE_HYPERLINKSTART: {
                    if (parseInfo.m_state != HYPERLINKNONE) {
                        break;
                    }

                    parseInfo.m_state = HYPERLINKDISPLAY;
                    parseInfo.m_hyperlink.m_linkData = text - advance + g_quotedCodeLengths[CODE_HYPERLINKSTART];
                    parseInfo.m_hyperlink.m_linkDataLength = advance - g_quotedCodeLengths[CODE_HYPERLINKSTOP] - g_quotedCodeLengths[CODE_HYPERLINKSTART];
                    parseInfo.m_hyperlink.m_linkText = text - advance;

                    const char* end = SStrStr(text, "|h");

                    if (end) {
                        end += g_quotedCodeLengths[CODE_HYPERLINKSTOP];

                        if (end && !SStrCmp(end, "|r", 2) && parseInfo.m_hyperlink.m_colorCode) {
                            end += 2;
                            parseInfo.m_hyperlink.m_linkText = parseInfo.m_hyperlink.m_colorCode;
                            parseInfo.m_hyperlink.m_colorCode = nullptr;
                            parseInfo.m_hyperlink.m_colorCodeLength = 0;
                        }

                        parseInfo.m_hyperlink.m_linkTextLength = end - parseInfo.m_hyperlink.m_linkText;
                        parseInfo.m_hyperlink.m_indentation = x + pos.x;
                    } else {
                        parseInfo.m_hyperlink.m_linkTextLength = 0;
                        parseInfo.m_hyperlink.m_indentation = x + pos.x;
                    }

                    break;
                }

                case CODE_HYPERLINKSTOP: {
                    if (parseInfo.m_state == HYPERLINKDISPLAY) {
                        parseInfo.m_hyperlink.m_right = x + pos.x;
                    } else if (parseInfo.m_state != HYPERLINKTEXTURE) {
                        break;
                    }

                    parseInfo.m_state = HYPERLINKNONE;
                    this->AddHyperlinkParseInfo(parseInfo.m_hyperlink);
                    break;
                }

                case CODE_TEXTURESTART: {
                    if (parseInfo.m_state == HYPERLINKTEXTURE) {
                        break;
                    }

                    GXUEMBEDDEDTEXTUREINFO& texture = parseInfo.m_texture;

                    if (!ParseEmbeddedTexture(text - advance, &texture, fontPixelHeight, this->m_scale, this->m_currentFontHeight)) {
                        break;
                    }

                    float savedLineHeight = texture.m_lineHeight;
                    float savedLeft = texture.m_left;
                    float savedBaseLineHeight = texture.m_baseLineHeight;
                    float savedRight = texture.m_right;

                    if (lastCode) {
                        CHARCODEDESC* desc = this->m_currentFace->NewCodeDesc(lastCode);

                        if (desc) {
                            step = this->m_currentFace->GetGlyphBearing(desc, (this->m_flags >> 7) & 1, this->m_currentFontHeight) + desc->m_bitmapData.m_glyphCellWidth;
                        }

                        x = step * scale;

                        if (x < 0.0) {
                            x = 0.0;
                        }

                        if (this->m_currentFace->m_fontFlags & 8) {
                            x = 4.0 * scale + x;
                        } else if (this->m_currentFace->m_fontFlags & 1) {
                            x = scale + scale + x;
                        }
                    }

                    float left = x + pos.x;
                    texture.m_left = left;

                    if (this->m_flags & 1) {
                        texture.m_left = this->m_shadowOffset.x * scale + left;
                    }

                    float right = texture.m_width * scale + texture.m_left;
                    texture.m_right = right;

                    if (this->m_vertJust == GxVJ_Top) {
                        texture.m_baseLineHeight = texture.m_lineHeight - scale * texture.m_height;
                    } else if (this->m_vertJust == GxVJ_Bottom) {
                        texture.m_lineHeight = scale * texture.m_height + texture.m_baseLineHeight;
                    } else if (this->m_vertJust == GxVJ_Middle) {
                        float height = scale * texture.m_height;
                        float top = (height - fontPixelHeight) * 0.5 + texture.m_lineHeight;
                        texture.m_lineHeight = top;
                        texture.m_baseLineHeight = top - height;
                    }

                    pos.x = right;
                    lastCode = 0;
                    step = 0.0f;

                    if (parseInfo.m_state == HYPERLINKDISPLAY) {
                        parseInfo.m_hyperlink.m_lineHeight = texture.m_lineHeight;
                        parseInfo.m_hyperlink.m_indentation = texture.m_left;
                        parseInfo.m_hyperlink.m_baseLineHeight = texture.m_baseLineHeight;
                        parseInfo.m_hyperlink.m_right = texture.m_right;
                        parseInfo.m_state = HYPERLINKTEXTURE;
                    }

                    this->AddEmbeddedTextureParseInfo(texture);

                    parseInfo.m_texture.m_lineHeight = savedLineHeight;
                    parseInfo.m_texture.m_left = savedLeft;
                    parseInfo.m_texture.m_baseLineHeight = savedBaseLineHeight;
                    parseInfo.m_texture.m_right = savedRight;
                    break;
                }

                default: {
                    CHARCODEDESC* desc = this->m_currentFace->NewCodeDesc(wide);

                    if (!desc) {
                        desc = this->m_currentFace->NewCodeDesc('?');

                        if (!desc) {
                            break;
                        }
                    }

                    uint32_t texturePage = desc->m_textureNumber;
                    *texturePagesUsed |= 1 << texturePage;

                    if (!this->m_textLines[texturePage]) {
                        this->m_textLines[texturePage] = TEXTLINETEXTURE::NewTextLineTexture();
                    }

                    TEXTLINETEXTURE* textLine = this->m_textLines[texturePage];

                    if (!(this->m_flags & 8)) {
                        uint32_t colorIndex = textLine->m_colors.Add(4, 0, &color);

                        if (this->m_flags & 0x20) {
                            GRADIENTINFO gradient[4];
                            gradient[0].m_texturePage = texturePage;
                            gradient[0].m_vertIndex = colorIndex;
                            gradient[1].m_texturePage = texturePage;
                            gradient[1].m_vertIndex = colorIndex + 1;
                            gradient[2].m_texturePage = texturePage;
                            gradient[2].m_vertIndex = colorIndex + 2;
                            gradient[3].m_texturePage = texturePage;
                            gradient[3].m_vertIndex = colorIndex + 3;
                            this->m_gradientInfo.Add(4, 8, gradient);
                        }
                    }

                    uint32_t flags = this->m_flags;

                    if (!(flags & 0x80)) {
                        x = x + 0.5;
                        x = floor(x);
                    }

                    pos.x = x + pos.x;

                    VERT vert;
                    vert.m_vc.x = pos.x;
                    vert.m_vc.y = pos.y;
                    vert.m_vc.z = pos.z;
                    vert.m_tc.x = 0.0f;
                    vert.m_tc.y = 0.0f;
                    vert.m_vc.x = this->m_currentFace->GetGlyphBearing(desc, (flags >> 7) & 1, this->m_currentFontHeight) + pos.x;

                    float width = desc->m_bitmapData.m_glyphCellWidth * scale;

                    if (!(this->m_flags & 0x80)) {
                        width = floor(width);
                    }

                    if (this->m_currentFace->m_fontFlags & 8) {
                        vert.m_vc.y = vert.m_vc.y - 2.0;
                    } else if (this->m_currentFace->m_fontFlags & 1) {
                        vert.m_vc.y = vert.m_vc.y - 1.0;
                    }

                    vert.m_vc.y = desc->m_bitmapData.m_yOffset * scale + vert.m_vc.y;

                    uint32_t vertIndex = textLine->m_verts.Add(4, 0, &vert);
                    VERT* verts = &textLine->m_verts.m_data[vertIndex];

                    verts[0].m_vc.x = verts[1].m_vc.x;

                    width = width + verts[3].m_vc.x;
                    verts[3].m_vc.x = width;
                    verts[2].m_vc.x = width;

                    float top = (this->m_flags & 0x80 ? fontPixelHeight : cellHeight) + verts[3].m_vc.y;
                    verts[3].m_vc.y = top;
                    verts[1].m_vc.y = top;
                    verts[0].m_vc.y = verts[2].m_vc.y;

                    CRect coords = desc->m_bitmapData.m_textureCoords;

                    if (this->m_flags & 0x80) {
                        coords.minY = coords.minY + pixelAdjustments.minY;
                        coords.minX = coords.minX + pixelAdjustments.minX;
                        coords.maxY = coords.maxY + pixelAdjustments.maxY;
                        coords.maxX = coords.maxX + pixelAdjustments.maxX;
                    }

                    verts[2].m_tc.y = coords.maxY;
                    verts[0].m_tc.y = coords.maxY;
                    verts[3].m_tc.y = coords.minY;
                    verts[1].m_tc.y = coords.minY;
                    verts[1].m_tc.x = coords.minX;
                    verts[0].m_tc.x = coords.minX;
                    verts[3].m_tc.x = coords.maxX;
                    verts[2].m_tc.x = coords.maxX;

                    lastCode = wide;
                    break;
                }
            }

            if (!*text) {
                break;
            }
        }
    }

    parseInfo.m_hyperlink.m_right = x + pos.x;
}

// VERIFY: 3b879ab8 auto
// OFFSET: 0x006C7440
void CGxString::AddShadow(const C2Vector& offset, const CImVector& color) {
    this->m_shadowColor = color;
    this->m_shadowColor.a = color.a < this->m_fontColor.a ? color.a : this->m_fontColor.a;
    this->m_shadowOffset = offset;
    this->m_flags |= 1;

    if (this->m_flags & 0x20) {
        this->ClearInstanceData();
    }
}

// VERIFY: 926a0475 auto
// OFFSET: 0x006C7480
int32_t CGxString::CheckGeometry() {
    if (this->m_textureEvicted) {
        if (!this->m_currentFace->CheckStringGlyphs(this->m_text)) {
            this->ClearInstanceData();
        }

        this->m_textureEvicted = 0;
    }

    this->CreateGeometry();
    this->m_idleTicks = 0;

    return this->m_numLines != 0;
}

// VERIFY: 10d868ef manual
// OFFSET: 0x006C74D0
int32_t CGxString::Initialize(float fontHeight, const C3Vector& position, float blockWidth, float blockHeight, CGxFont* face, const char* text, EGxFontVJusts vertJust, EGxFontHJusts horzJust, float spacing, uint32_t flags, const CImVector& color, float scale) {
    uint32_t textLen = SStrLen(text) + 1;

    if (textLen > this->m_textLen) {
        if (this->m_text) {
            SMemFree(this->m_text, __FILE__, __LINE__, 0);
        }

        this->m_textLen = textLen;
        this->m_text = static_cast<char*>(SMemAlloc(textLen, __FILE__, __LINE__, 0));
    }

    SStrCopy(this->m_text, text, this->m_textLen);

    this->m_blockWidth = blockWidth;
    this->m_blockHeight = blockHeight;
    this->m_spacing = spacing;
    this->m_position = position;
    this->m_horzJust = horzJust;
    this->m_vertJust = vertJust;
    this->m_flags = flags;
    this->m_fontColor = color;
    this->m_scale = scale;
    this->m_currentFace = face;

    face->m_strings.LinkToTail(this);

    if ((flags & 4) && !(this->m_flags & 0x80)) {
        this->m_requestedFontHeight = GxuFontGetOneToOneHeight(face);
    } else {
        this->m_requestedFontHeight = fontHeight;
    }

    double minHeight = 2.0 / g_heightPixels;
    this->m_currentFontHeight = this->m_requestedFontHeight > minHeight ? this->m_requestedFontHeight : minHeight;

    return 1;
}

// VERIFY: 45504676 manual
// OFFSET: 0x006C75E0
void CGxString::Recycle() {
    this->m_batchedStringLink.Unlink();
    g_freeStrings.LinkToTail(this);
    this->ClearInstanceData();
}

// VERIFY: 4d2beec7 manual
// OFFSET: 0x006C7640
CGxString::CGxString() {
    this->m_requestedFontHeight = 0.02f;
    this->m_currentFontHeight = 0.02f;
    this->m_position.x = 0.0f;
    this->m_position.y = 0.0f;
    this->m_position.z = 0.0f;
    this->m_fontColor.b = 0xFF;
    this->m_fontColor.g = 0xFF;
    this->m_fontColor.r = 0xFF;
    this->m_fontColor.a = 0xFF;
    this->m_shadowColor.a = 0xFF;
    this->m_shadowColor.b = 0;
    this->m_shadowColor.g = 0;
    this->m_shadowColor.r = 0;
    this->m_shadowOffset.x = 0.00125f;
    this->m_shadowOffset.y = -0.00125f;
    this->m_currentFace = nullptr;
    this->m_text = nullptr;
    this->m_blockWidth = 1.0f;
    this->m_textLen = 0;
    this->m_blockHeight = 1.0f;
    this->m_vertJust = GxVJ_Top;
    this->m_horzJust = GxHJ_Left;
    this->m_spacing = 0.0f;
    this->m_flags = 0;
    this->m_texturePagesUsed = 0;
    this->m_textureEvicted = 0;
    this->m_lastGradientStart = -1;
    this->m_lastGradientLength = -1;
    this->m_viewTranslation.x = 0.0f;
    this->m_viewTranslation.y = 0.0f;
    this->m_viewTranslation.z = 0.0f;
    this->m_scale = 1.0f;
    this->m_numLines = 0;
    this->m_idleTicks = 0;
    this->m_gradientInfo.m_chunk = 128;

    for (uint32_t i = 0; i < 8; i++) {
        this->m_textLines[i] = nullptr;
    }
}

// VERIFY: d509222d manual
// OFFSET: 0x006C7760
CGxString::~CGxString() {
    for (uint32_t i = 0; i < 8; i++) {
        if (this->m_textLines[i]) {
            TEXTLINETEXTURE::Recycle(this->m_textLines[i]);
        }
    }

    if (this->m_text) {
        SMemFree(this->m_text, __FILE__, __LINE__, 0);
    }

    this->m_hyperlinkInfo.Clear();
    this->m_embeddedTextureInfo.Clear();
}

// VERIFY: 19f236ba manual
// OFFSET: 0x006C78F0
int32_t CGxString::SetGradient(int32_t start, int32_t length) {
    if (start < 0 || length < 0) {
        return 2;
    }

    if (start == this->m_lastGradientStart && length == this->m_lastGradientLength) {
        return 0;
    }

    this->m_lastGradientLength = length;
    this->m_lastGradientStart = start;
    this->m_flags = (this->m_flags & ~8) | 0x20;

    if (!this->m_gradientInfo.m_count) {
        this->ClearInstanceData();
        this->CheckGeometry();
    }

    if (!this->m_numLines) {
        return -1;
    }

    uint8_t alpha = this->m_fontColor.a;
    int32_t first = 4 * start;
    int32_t count = this->m_gradientInfo.m_count;
    uint8_t fade = static_cast<uint8_t>(alpha / static_cast<float>(length));

    for (int32_t i = 0; i < count; i++) {
        if (i < first) {
            this->SetGradientAlpha(this->m_gradientInfo.m_data[i], alpha);
        } else if (alpha && i < first + 4 * length) {
            this->SetGradientAlpha(this->m_gradientInfo.m_data[i], alpha);

            if (i & 1) {
                alpha = alpha > fade ? alpha - fade : 0;
            }
        } else {
            if (!this->GetGradientAlpha(this->m_gradientInfo.m_data[i])) {
                break;
            }

            this->SetGradientAlpha(this->m_gradientInfo.m_data[i], alpha);
        }
    }

    if (first < count) {
        return first + 4 * length > count;
    }

    return 2;
}

// VERIFY: f8b5b030 manual
// OFFSET: 0x006C7AA0
CGxString* CGxString::GetNewString(int32_t linkOnTop) {
    CGxString* string = g_freeStrings.Head();

    if (string) {
        g_strings.LinkToTail(string);
        return string;
    }

    return g_strings.NewNode(linkOnTop ? 2 : 0, 0, 0);
}

// VERIFY: 21bb2d06 manual
// OFFSET: 0x006C7B10
void CGxString::CreateGeometry() {
    if (this->m_numLines || !this->m_text || !*this->m_text) {
        return;
    }

    this->m_gradientInfo.SetCount(0);

    int32_t lastGradientStart = this->m_lastGradientStart;
    int32_t lastGradientLength = this->m_lastGradientLength;

    float top = ScreenToPixelHeight((this->m_flags >> 7) & 1, -this->m_currentFontHeight);

    C3Vector position;
    position.x = 0.0f;
    position.y = this->m_flags & 0x80 ? 0.0f : top;
    position.z = 0.0f;

    const char* text = this->m_text;
    CImVector color = this->m_fontColor;

    EMBEDDEDPARSEINFO parseInfo;

    float width = 0.0f;
    float spacing = this->m_spacing;
    const char* nextText = nullptr;
    uint32_t numBytes = 0;

    float spacingPixels = g_heightPixels * spacing;
    spacingPixels = static_cast<uint32_t>(spacingPixels + 0.99995);

    float fontPixelHeight = ScreenToPixelHeight(0, this->m_currentFontHeight);
    float lineHeight;

    if (this->m_flags & 0x80) {
        lineHeight = this->m_currentFontHeight + spacing;
    } else {
        spacing = spacingPixels / g_heightPixels;
        lineHeight = spacingPixels + fontPixelHeight;
    }

    bool wordBreak = true;

    parseInfo.m_hyperlink.m_baseLineHeight = position.y;
    float lineTop = lineHeight + position.y;
    parseInfo.m_hyperlink.m_lineHeight = lineTop;
    parseInfo.m_texture.m_lineHeight = lineTop;
    parseInfo.m_texture.m_baseLineHeight = position.y;

    float heightUsed = 0.0f;
    float blockWidth = this->m_blockWidth;

    while (*text) {
        if (!(this->m_blockHeight > heightUsed)) {
            break;
        }

        if ((this->m_flags & 0x2000) && this->m_numLines == 1) {
            blockWidth = blockWidth - g_indentNormWidth;
        }

        heightUsed = this->m_currentFontHeight + spacing + heightUsed;

        int32_t advance;
        uint32_t wide;
        QUOTEDCODE code = GxuDetermineQuotedCode(text, advance, nullptr, this->m_flags, wide);

        width = 0.0f;

        if (code == CODE_NEWLINE) {
            text += advance;
            this->m_numLines++;
            position.y = position.y - lineHeight;
            parseInfo.m_hyperlink.m_baseLineHeight = parseInfo.m_hyperlink.m_baseLineHeight - lineHeight;
            parseInfo.m_hyperlink.m_lineHeight = parseInfo.m_hyperlink.m_lineHeight - lineHeight;
            parseInfo.m_texture.m_baseLineHeight = parseInfo.m_texture.m_baseLineHeight - lineHeight;
            parseInfo.m_texture.m_lineHeight = parseInfo.m_texture.m_lineHeight - lineHeight;
            continue;
        }

        float lineHeightUsed = 0.0f;
        float shadowWidth = this->m_flags & 1 ? this->m_shadowOffset.x : 0.0f;

        CalcWrapPoint(this->m_currentFace, text, this->m_currentFontHeight, blockWidth, &numBytes, &width, &nextText, shadowWidth, this->m_flags, &wordBreak, &lineHeightUsed, this->m_scale);

        if (nextText == text || !nextText || (!numBytes && !*nextText)) {
            break;
        }

        if (this->m_horzJust == GxHJ_Right) {
            float indent = (this->m_flags & 0x2000) && this->m_numLines ? g_indentPixelWidth : 0.0f;
            position.x = ScreenToPixelWidth((this->m_flags >> 7) & 1, -width) - indent;
        } else if (this->m_horzJust == GxHJ_Center) {
            float halfWidth = -(width * 0.5);
            position.x = ScreenToPixelWidth((this->m_flags >> 7) & 1, halfWidth);
        } else {
            position.x = (this->m_flags & 0x2000) && this->m_numLines ? g_indentPixelWidth : 0.0f;
        }

        float offset = 0.0f;

        if (fontPixelHeight < lineHeightUsed) {
            if (this->m_vertJust == GxVJ_Bottom) {
                offset = lineHeightUsed - fontPixelHeight;
            } else if (this->m_vertJust == GxVJ_Middle) {
                float halfOffset = (lineHeightUsed - fontPixelHeight) * 0.5;
                offset = floor(halfOffset);
            }
        }

        uint32_t texturePagesUsed = 0;

        position.y = position.y - offset;
        parseInfo.m_hyperlink.m_baseLineHeight = parseInfo.m_hyperlink.m_baseLineHeight - offset;
        parseInfo.m_hyperlink.m_lineHeight = parseInfo.m_hyperlink.m_lineHeight - offset;
        parseInfo.m_texture.m_baseLineHeight = position.y;
        parseInfo.m_texture.m_lineHeight = position.y + lineHeight;

        if (parseInfo.m_state) {
            parseInfo.m_hyperlink.m_indentation = position.x;
        }

        this->InitializeTextLine(text, numBytes, color, position, &texturePagesUsed, parseInfo);
        this->m_numLines++;

        offset = 0.0f;

        if (fontPixelHeight < lineHeightUsed) {
            if (this->m_vertJust == GxVJ_Top) {
                offset = lineHeightUsed - fontPixelHeight;
            } else if (this->m_vertJust == GxVJ_Middle) {
                offset = (lineHeightUsed - fontPixelHeight) * 0.5;
            }
        }

        if (parseInfo.m_state == HYPERLINKDISPLAY) {
            this->AddHyperlinkParseInfo(parseInfo.m_hyperlink);
        }

        this->m_texturePagesUsed |= texturePagesUsed;
        text = nextText;

        float advanceY = offset + lineHeight;
        position.y = position.y - advanceY;

        if (this->m_flags & 2) {
            break;
        }

        parseInfo.m_hyperlink.m_baseLineHeight = parseInfo.m_hyperlink.m_baseLineHeight - advanceY;
        parseInfo.m_hyperlink.m_lineHeight = parseInfo.m_hyperlink.m_lineHeight - advanceY;
        parseInfo.m_texture.m_baseLineHeight = position.y;
        parseInfo.m_texture.m_lineHeight = lineHeight + position.y;
    }

    this->InitializeViewTranslation();

    if ((this->m_flags & 0x20) && (lastGradientStart != -1 || lastGradientLength != -1)) {
        this->SetGradient(lastGradientStart, lastGradientLength);
    }
}

// VERIFY: 9f3bd928 auto
// OFFSET: 0x006C8000
uint32_t CGxString::GetHyperLinkInfo(const GXUFONTHYPERLINKINFO*& info) {
    this->CreateGeometry();

    uint32_t count = this->m_hyperlinkInfo.m_count;

    if (!count || !g_widthPixels || !g_heightPixels) {
        return 0;
    }

    info = this->m_hyperlinkInfo.m_data;

    return count;
}

// VERIFY: edea8278 auto
// OFFSET: 0x006C8040
uint32_t CGxString::GetEmbeddedTextureInfo(const GXUEMBEDDEDTEXTUREINFO*& info) {
    this->CreateGeometry();

    uint32_t count = this->m_embeddedTextureInfo.m_count;

    if (!count || !g_widthPixels || !g_heightPixels) {
        return 0;
    }

    info = this->m_embeddedTextureInfo.m_data;

    return count;
}

// INLINED
GXUFONTHYPERLINKINFO::GXUFONTHYPERLINKINFO() {
    this->m_lineHeight = 0.0f;
    this->m_indentation = 0.0f;
    this->m_baseLineHeight = 0.0f;
    this->m_right = 0.0f;
    this->m_linkData = nullptr;
    this->m_linkDataLength = 0;
    this->m_linkText = nullptr;
    this->m_linkTextLength = 0;
    this->m_colorCode = nullptr;
    this->m_colorCodeLength = 0;
}
