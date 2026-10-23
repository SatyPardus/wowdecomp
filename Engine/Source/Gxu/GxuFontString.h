#ifndef ENGINE_SOURCE_GXU_GXUFONTSTRING_H
#define ENGINE_SOURCE_GXU_GXUFONTSTRING_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gxu/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"

class CGxVertexPCT;
struct CGxFont;
struct CGxString;
struct TEXTLINETEXTURE;

struct GXUEMBEDDEDTEXTUREINFO {
    /* 0000 */ float m_lineHeight;
    /* 0004 */ float m_left;
    /* 0008 */ float m_baseLineHeight;
    /* 000C */ float m_right;
    /* 0010 */ const char* m_path;
    /* 0014 */ uint32_t m_pathLength;
    /* 0018 */ float m_height;
    /* 001C */ float m_width;
    /* 0020 */ float m_offsetX;
    /* 0024 */ float m_offsetY;
    /* 0028 */ CRect m_texCoords;
    /* 0038 */ uint8_t m_hasTexCoords;

    GXUEMBEDDEDTEXTUREINFO();
};

enum HYPERLINKPARSEMODE : int32_t {
    HYPERLINKNONE = 0,
    HYPERLINKDISPLAY = 1,
    HYPERLINKTEXTURE = 2,
};

struct EMBEDDEDPARSEINFO {
    /* 0000 */ HYPERLINKPARSEMODE m_state;
    /* 0004 */ GXUFONTHYPERLINKINFO m_hyperlink;
    /* 002C */ GXUEMBEDDEDTEXTUREINFO m_texture;

    EMBEDDEDPARSEINFO();
};

struct CGxString : public TSLinkedNode<CGxString> {
    static CGxString* GetNewString(int32_t linkOnTop);

    /* 0008 */ TSLink<CGxString> m_fontStringLink;
    /* 0010 */ TSLink<CGxString> m_batchedStringLink;
    /* 0018 */ float m_requestedFontHeight;
    /* 001C */ float m_currentFontHeight;
    /* 0020 */ C3Vector m_position;
    /* 002C */ CImVector m_fontColor;
    /* 0030 */ CImVector m_shadowColor;
    /* 0034 */ C2Vector m_shadowOffset;
    /* 003C */ float m_blockWidth;
    /* 0040 */ float m_blockHeight;
    /* 0044 */ CGxFont* m_currentFace;
    /* 0048 */ char* m_text;
    /* 004C */ uint32_t m_textLen;
    /* 0050 */ EGxFontVJusts m_vertJust;
    /* 0054 */ EGxFontHJusts m_horzJust;
    /* 0058 */ float m_spacing;
    /* 005C */ uint32_t m_flags;
    /* 0060 */ uint32_t m_texturePagesUsed;
    /* 0064 */ int32_t m_textureEvicted;
    /* 0068 */ int32_t m_lastGradientStart;
    /* 006C */ int32_t m_lastGradientLength;
    /* 0070 */ C3Vector m_viewTranslation;
    /* 007C */ float m_scale;
    /* 0080 */ TSGrowableArray<GXUFONTHYPERLINKINFO> m_hyperlinkInfo;
    /* 0090 */ TSGrowableArray<GXUEMBEDDEDTEXTUREINFO> m_embeddedTextureInfo;
    /* 00A0 */ TSGrowableArray<GRADIENTINFO> m_gradientInfo;
    /* 00B0 */ int32_t m_numLines;
    /* 00B4 */ TEXTLINETEXTURE* m_textLines[8];
    /* 00D4 */ int32_t m_idleTicks;

    CGxString();
    ~CGxString();
    void AddEmbeddedTextureParseInfo(GXUEMBEDDEDTEXTUREINFO info);
    void AddHyperlinkParseInfo(GXUFONTHYPERLINKINFO info);
    void AddShadow(const C2Vector& offset, const CImVector& color);
    int32_t CalculateVertsNeeded(int32_t texturePage);
    int32_t CheckGeometry();
    void ClearInstanceData();
    void CreateGeometry();
    uint8_t GetGradientAlpha(const GRADIENTINFO& info);
    uint32_t GetEmbeddedTextureInfo(const GXUEMBEDDEDTEXTUREINFO*& info);
    uint32_t GetHyperLinkInfo(const GXUFONTHYPERLINKINFO*& info);
    void HandleScreenSizeChange();
    int32_t Initialize(float fontHeight, const C3Vector& position, float blockWidth, float blockHeight, CGxFont* face, const char* text, EGxFontVJusts vertJust, EGxFontHJusts horzJust, float spacing, uint32_t flags, const CImVector& color, float scale);
    void InitializeTextLine(const char* text, uint32_t numBytes, CImVector& color, const C3Vector& position, uint32_t* texturePagesUsed, EMBEDDEDPARSEINFO& parseInfo);
    void InitializeViewTranslation();
    void Recycle();
    void RemoveShadow();
    void SetColor(const CImVector& color);
    void SetGradientAlpha(const GRADIENTINFO& info, uint8_t alpha);
    int32_t SetGradient(int32_t start, int32_t length);
    void SetStringPosition(const C3Vector& position);
    void TexturePageEvicted(uint32_t texturePage);
    void Tick();
    void WriteGeometry(CGxVertexPCT* buf, int32_t texturePage, int32_t vertIndex, int32_t numVerts) const;
};

struct VERT {
    /* 0000 */ C3Vector m_vc;
    /* 000C */ C2Vector m_tc;
};

#endif
