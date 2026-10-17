#ifndef ENGINE_SOURCE_GXU_GXUFONTMISCCLASSES_H
#define ENGINE_SOURCE_GXU_GXUFONTMISCCLASSES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gxu/GxuFontString.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"

class CGxVertexPCT;
class CTexture;
struct CGxBuf;
struct CGxFont;
struct CGxPool;

struct GLYPHBITMAPDATA {
    /* 0000 */ void* m_data;
    /* 0004 */ uint32_t m_dataSize;
    /* 0008 */ uint32_t m_glyphWidth;
    /* 000C */ uint32_t m_glyphHeight;
    /* 0010 */ uint32_t m_glyphCellWidth;
    /* 0014 */ float m_glyphAdvance;
    /* 0018 */ float m_glyphBearing;
    /* 001C */ uint32_t m_glyphPitch;
    /* 0020 */ int32_t m_yOffset;
    /* 0024 */ uint32_t m_yStart;
    /* 0028 */ uint8_t m_isMsdf;
    /* 002C */ CRect m_textureCoords;

    GLYPHBITMAPDATA();
    ~GLYPHBITMAPDATA();
    void CopyFrom(GLYPHBITMAPDATA* source);
};

struct CHARCODEDESC : public TSHashObject<CHARCODEDESC, HASHKEY_NONE> {
    /* 0018 */ TSLink<CHARCODEDESC> m_textureRowLink;
    /* 0020 */ TSLink<CHARCODEDESC> m_fontGlyphLink;
    /* 0028 */ uint32_t m_textureNumber;
    /* 002C */ uint32_t m_rowNumber;
    /* 0030 */ uint32_t m_glyphStartPixel;
    /* 0034 */ uint32_t m_glyphEndPixel;
    /* 0038 */ GLYPHBITMAPDATA m_bitmapData;

    CHARCODEDESC();
    void GenerateTextureCoords(uint32_t rowNumber, uint32_t cellHeight, uint32_t atlasSize = 256);
    uint32_t GapToPrevious();
};

struct TEXTLINETEXTURE : public TSLinkedNode<TEXTLINETEXTURE> {
    static int32_t s_recycledBytes;
    static TSList<TEXTLINETEXTURE, TSGetLink<TEXTLINETEXTURE> > s_freeTextLineTextures;

    static TEXTLINETEXTURE* NewTextLineTexture();
    static void Recycle(TEXTLINETEXTURE* ptr);
    static void Shutdown();

    /* 0008 */ TSGrowableArray<VERT> m_verts;
    /* 0018 */ TSGrowableArray<CImVector> m_colors;

    TEXTLINETEXTURE();
    ~TEXTLINETEXTURE();
    void WriteGeometry(CGxVertexPCT* buf, const CImVector& fontColor, const C2Vector& shadowOffset, const CImVector& shadowColor, const C3Vector& viewTranslation, bool hasShadow, bool hasGradient, int32_t vertIndex, int32_t numVerts) const;
};

struct BATCHEDRENDERFONTDESC : public TSHashObject<BATCHEDRENDERFONTDESC, HASHKEY_PTR> {
    static CGxPool* s_indexPool;
    static CGxBuf* s_indexBuf;
    static uint8_t s_billboarded;

    static void Initialize();
    static void InitializeIndexBuff();
    static void Shutdown();
    static CGxVertexPCT* UnlockVertexPtrAndRender(CGxBuf*& buf, int32_t count);

    /* 0018 */ CGxFont* m_face;
    /* 001C */ TSExplicitList<CGxString, offsetof(CGxString, m_batchedStringLink)> m_strings;

    ~BATCHEDRENDERFONTDESC();
    void RenderBatch();
};

struct CGxStringBatch : public TSLinkedNode<CGxStringBatch> {
    /* 0008 */ uint32_t m_flags;
    /* 000C */ TSHashTable<BATCHEDRENDERFONTDESC, HASHKEY_PTR> m_fontBatch;

    CGxStringBatch();
    void AddString(CGxString* string);
    void RenderBatch();
};

struct TEXTURECACHEROW {
    /* 0000 */ uint32_t m_widestFreeSlot;
    /* 0004 */ TSExplicitList<CHARCODEDESC, offsetof(CHARCODEDESC, m_textureRowLink)> m_glyphlist;

    TEXTURECACHEROW();
    CHARCODEDESC* CreateNewDesc(GLYPHBITMAPDATA* data, uint32_t rowNumber, uint32_t cellHeight, uint32_t atlasSize = 256);
    void EvictGlyph(CHARCODEDESC* desc, uint32_t atlasSize = 256);
};

struct TEXTURECACHE {
    static void TextureCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);

    /* 0000 */ CTexture* m_texture;
    /* 0004 */ CGxFont* m_theFace;
    /* 0008 */ uint8_t m_anyDirtyGlyphs;
    /* 0009 */ uint8_t m_isMsdfCache;
    /* 000C */ uint32_t m_atlasSize;
    /* 0010 */ TSFixedArray<TEXTURECACHEROW> m_textureRows;

    TEXTURECACHE();
    ~TEXTURECACHE();
    CHARCODEDESC* AllocateNewGlyph(GLYPHBITMAPDATA* data);
    void CreateTexture(int32_t filter);
    void Destroy();
    void Initialize(CGxFont* face, uint32_t cellHeight);
    void PasteGlyphNonOutlinedAA(const GLYPHBITMAPDATA& data, uint16_t* dest);
    void PasteGlyphNonOutlinedMonochrome(const GLYPHBITMAPDATA& data, uint16_t* dest);
    void PasteGlyphOutlinedAA(const GLYPHBITMAPDATA& data, uint16_t* dest);
    void PasteGlyphOutlinedMonochrome(const GLYPHBITMAPDATA& data, uint16_t* dest);
    void UpdateDirty();
    void PasteGlyphMsdf(const GLYPHBITMAPDATA& data, uint32_t* dest);
    static void MsdfTextureCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
};

#endif
