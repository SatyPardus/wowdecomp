#ifndef ENGINE_SOURCE_FRAME_CSIMPLERENDER_H
#define ENGINE_SOURCE_FRAME_CSIMPLERENDER_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleEmbeddedTexture.h"
#include "Engine/Source/Frame/CSimpleFontable.h"
#include "Engine/Source/Frame/CSimpleRegion.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/Services/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/stpl.h"

class CGxShader;
class CRect;
class CRenderBatch;
class CSimpleFontStringAttributes;
class CSimpleFrame;
class CSimpleTexture;
class CStatus;
struct FONTHASHOBJ;
class TEXTBLOCK;
class XMLNode;

class CSimpleRender {
    public:
    static CGxShader* s_vertexShader[2];
    static int32_t s_mergeEnable;
    // The language renderer asks the game layer for declined player/pet names.
    static int32_t (*s_declinedWord)(const char*, int32_t, char*, uint32_t);

    static int32_t CalcBatchCounts(CRenderBatch* batch, uint32_t* vertCounts, uint32_t* idxCounts);
    static void Destroy();
    static void DrawBatch(CRenderBatch* batch);
    static void Init();
};

class CSimpleFontString : public CSimpleRegion, public CSimpleFontable {
    public:
    static CDataAllocator s_allocator;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr);
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static void FreeEmbededTextureStorage();
    static CSimpleEmbeddedTexture* GetEmbeddedTextureFromStorage();
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 00E8 */ HTEXTFONT m_fontHandle;
    /* 00EC */ float m_fontHeight;
    /* 00F0 */ uint16_t m_textMaxSize;
    /* 00F2 */ uint16_t m_textCurSize;
    /* 00F4 */ char* m_text;
    /* 00F8 */ float m_spacing;
    /* 00FC */ HTEXTBLOCK m_string;
    /* 0100 */ float m_textWidth;
    /* 0104 */ float m_textHeight;
    /* 0108 */ CImVector m_shadowColor;
    /* 010C */ C2Vector m_shadowOffset;
    /* 0114 */ C2Vector m_justificationOffset;
    /* 011C */ int16_t m_alphaGradientStart;
    /* 011E */ int16_t m_alphaGradientLength;
    /* 0120 */ uint32_t m_styleFlags;
    /* 0124 */ uint32_t m_settableStyleFlags;
    /* 0128 */ uint32_t m_maxLines;
    /* 012C */ C3Vector m_animPosition;
    /* 0138 */ TSExplicitList<CSimpleEmbeddedTexture, offsetof(CSimpleEmbeddedTexture, m_link)> m_textures;

    virtual ~CSimpleFontString();
    virtual int32_t GetScriptMetaTable();
    virtual bool IsA(int32_t type);
    virtual bool IsA(const char* typeName);
    virtual const char* GetObjectTypeName();
    virtual void PreOnAnimUpdate();
    virtual void AddAnimTranslation(const C2Vector& translation);
    virtual void OnColorChanged(bool changed);
    virtual void OnScreenSizeChanged();
    virtual void Draw(CRenderBatch* batch);
    virtual void StoreAnimPosition();
    virtual void ResetAnimPosition();
    virtual void LoadXML(XMLNode* node, CStatus* status);
    virtual bool SetLayoutScale(float scale, bool force);
    virtual bool SetLayoutDepth(float depth, bool force);
    virtual void SetWidth(float width);
    virtual void SetHeight(float height);
    virtual float GetWidth();
    virtual float GetHeight();
    virtual void OnFrameSizeChanged(const CRect& rect);
    virtual void FontObjectUpdated(CSimpleFontStringAttributes& attributes);

    CSimpleFontString(CSimpleFrame* frame, uint32_t drawLayer, bool show);
    void AddShadow(const CImVector& color, const C2Vector& offset);
    void ClearString();
    void FreeEmbeddedTextures();
    const char* GetDisplayText(float width, float height);
    uint32_t GetFontFlags();
    float GetFontHeight(bool recalculate);
    const char* GetFontName();
    uint32_t GetNumCharsWithinWidth(const char* text, uint32_t numBytes, float maxWidth);
    uint32_t GetNumCharsWithinWidthFromEnd(const char* text, uint32_t numBytes, float maxWidth);
    float GetStringHeight();
    float GetStringWidth();
    float GetTextWidth(const char* text, uint32_t numBytes);
    void PostLoadXML(XMLNode* node, CStatus* status);
    void QueueTextures(CRenderBatch* batch);
    void RefreshEmbeddedTextures();
    void RemoveShadow();
    bool SetAlphaGradient(int32_t start, int32_t length);
    int32_t SetFont(const char* fontName, float fontHeight, uint32_t fontFlags, bool force);
    void SetIndented(int32_t indented);
    void SetJustificationOffset(float x, float y);
    void SetJustifyH(uint8_t justify);
    void SetJustifyV(uint8_t justify);
    void SetNonSpaceWrap(int32_t wrap);
    void SetNonWordWrap(int32_t noWrap);
    void SetShadowColor(const CImVector& color);
    void SetShadowOffset(const C2Vector& offset);
    void SetSpacing(float spacing);
    void SetStyleFlags(uint32_t styleFlags);
    void SetText(const char* text, int32_t process);
    void SetTextHeight(float height);
    void SetTextLength(uint32_t length);
    void UpdateString();
    uint32_t WrapText(const char* text, float width, uint32_t* lineStarts, uint32_t maxLines);
};

int32_t GetDeclinedWord(const char* word, int32_t declension, char* buffer, uint32_t bufferSize);
const char* LanguageProcess(const char* text);

#endif
