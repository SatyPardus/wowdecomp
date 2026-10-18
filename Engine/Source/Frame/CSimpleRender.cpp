#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleRender.h"
#if defined(__APPLE__)
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include <cctype>
#include <cstring>
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CSimpleEmbeddedTexture.h"
#include "Engine/Source/Gxu/GxuFontString.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include <cmath>
#include <stdexcept>
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CSimpleFont.h"
#include "Engine/Source/Frame/CSimpleFontStringAttributes.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameScript/Types.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Frame/CFramePoint.h"
#include "Engine/Source/Frame/CSimpleRegion.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"

// STATIC: 0x009EA254
static const uint8_t s_letterHasFinalConsonant[26] = { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// STATIC: 0x009EA270
static const uint8_t s_digitHasFinalConsonant[10] = { 1, 1, 0, 1, 0, 0, 1, 1, 1, 0 };

// STATIC: 0x00B41928
static uint16_t s_languageWideBuffer[0x2000];

// STATIC: 0x00B45928
static char s_languageBuffer[0x2000];

// STATIC: 0x00AC0F14
static TSExplicitList<CSimpleEmbeddedTexture, offsetof(CSimpleEmbeddedTexture, m_link)> s_embeddedTextureStorage;

// STATIC: 0x00B47940
CGxShader* CSimpleRender::s_vertexShader[2];

// STATIC: 0x00B47948
int32_t CSimpleRender::s_mergeEnable;
#if defined(__APPLE__)
int32_t (*CSimpleRender::s_declinedWord)(const char*, int32_t, char*, uint32_t) = nullptr;
#else
int32_t (*CSimpleRender::s_declinedWord)(const char*, int32_t, char*, uint32_t) = GetDeclinedWord;
#endif

// STATIC: 0x00B47978
static char s_displayText[0x2000];

// STATIC: 0x00AC0EF4
const char* CSimpleTexture::s_objectTypeName = "Texture";

// STATIC: 0x00AC0EF8
const char* CSimpleTexture::s_imageModePixelShaderNames[2] = { "UI", "Desaturate" };

// STATIC: 0x00AC0F00
EGxTexFilter CSimpleTexture::s_textureFilterMode = GxTex_Linear;

// STATIC: 0x00B47930
int32_t CSimpleTexture::s_count;

// STATIC: 0x00B47934
CGxShader* CSimpleTexture::s_imageModePixelShaders[2];

// STATIC: 0x00B4793C
int32_t CSimpleTexture::s_objectType;

// STATIC: 0x00B4794C
CDataAllocator CSimpleTexture::s_allocator(sizeof(CSimpleTexture), 0x2E0);

// STATIC: 0x00AC1114
int32_t CSimpleFontString::s_metatable = LUA_NOREF;

// STATIC: 0x00B4792C
int32_t CSimpleFontString::s_objectType;

// STATIC: 0x00AC0F10
const char* CSimpleFontString::s_objectTypeName = "FontString";

// STATIC: 0x00B47960
CDataAllocator CSimpleFontString::s_allocator(sizeof(CSimpleFontString), 809);

// STATIC: 0x00B47928
static int32_t s_fontStringCount;

// VERIFY: 0e98fd61 auto
// OFFSET: 0x004826D0
static int32_t IsRectOffscreen(const CRect& rect) {
    return rect.maxY < 0.0f || rect.minY > 1.0f || rect.maxX < 0.0f || rect.minX > 1.0f;
}

// VERIFY: d066960a manual
// OFFSET: 0x00481960
static const uint16_t* LanguageRule1ChooseParticle(uint32_t lastChar, const uint16_t* consonantForm, const uint16_t* vowelForm) {
    if (lastChar & 0xFF00) {
        if (static_cast<uint16_t>(lastChar + 0x5400) > 0x2BA3) {
            return consonantForm;
        }

        int32_t syllable = static_cast<uint16_t>(lastChar);

        if ((syllable - 0xAC08) % 28 == 0 && *consonantForm == 0xC73C) {
            return vowelForm;
        }

        return (syllable - 0xAC00) % 28 ? consonantForm : vowelForm;
    }

    if (static_cast<uint16_t>(lastChar - '0') <= 9) {
        return s_digitHasFinalConsonant[static_cast<uint16_t>(lastChar) - '0'] ? consonantForm : vowelForm;
    }

    if ((lastChar >= 'a' && lastChar <= 'z') || static_cast<uint16_t>(lastChar - 'A') <= 25) {
        return s_letterHasFinalConsonant[tolower(static_cast<uint16_t>(lastChar)) - 'a'] ? consonantForm : vowelForm;
    }

    return consonantForm;
}

// VERIFY: f67eabcd manual
// OFFSET: 0x00481A00
static char* LanguageRule1(const char* text) {
    uint16_t buffer[0x2000];
    uint32_t lastChar = 0;

    SUniConvertUTF8to16(buffer, sizeof(buffer) / sizeof(buffer[0]), text, STORM_MAX_STR, nullptr, nullptr);

    uint16_t* dest = s_languageWideBuffer;
    const uint16_t* src = buffer;

    if (buffer[0]) {
        do {
            uint16_t ch = *src;

            if (ch == '|') {
                src++;
                ch = *src;

                if (ch == '1' && (src[1] > '9' || src[1] < '0')) {
                    const uint16_t* consonantForm = src + 1;
                    const uint16_t* vowelForm = consonantForm;

                    if (*vowelForm) {
                        uint16_t c;

                        do {
                            c = *vowelForm;
                            vowelForm++;
                        } while (c != ';' && *vowelForm);
                    }

                    const uint16_t* end = vowelForm;

                    if (*end) {
                        uint16_t c;

                        do {
                            c = *end;
                            end++;
                        } while (c != ';' && *end);
                    }

                    src = end - 1;

                    const uint16_t* particle = LanguageRule1ChooseParticle(lastChar, consonantForm, vowelForm);

                    for (uint16_t c = *particle; c && c != ';'; c = *particle) {
                        particle++;
                        *dest = c;
                        dest++;
                    }
                } else {
                    *dest = '|';
                    dest++;
                    *dest = ch;
                    dest++;

                    if (ch == 'c' || ch == 'C') {
                        for (uint32_t i = 0; i < 8 && src[1]; i++) {
                            src++;
                            *dest = *src;
                            dest++;
                        }
                    }
                }
            } else {
                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || static_cast<uint16_t>(ch + 0x5400) <= 0x2BA3) {
                    lastChar = ch;
                }

                *dest = ch;
                dest++;
            }

            src++;
        } while (*src);
    }

    *dest = 0;
    SUniConvertUTF16to8(s_languageBuffer, sizeof(s_languageBuffer), s_languageWideBuffer, STORM_MAX_STR, nullptr, nullptr);

    return s_languageBuffer;
}

// VERIFY: 8cd26535 manual
// OFFSET: 0x00481BC0
static bool LanguageRule2IsVowel(uint16_t ch) {
    switch (ch) {
        case 'A':
        case 'E':
        case 'I':
        case 'O':
        case 'U':
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
            return true;

        default:
            return (ch >= 0xC0 && ch <= 0xC6) || (ch >= 0xC8 && ch <= 0xCF) || (ch >= 0xD2 && ch <= 0xD6) || (ch >= 0xD8 && ch <= 0xDC) || (ch >= 0xE0 && ch <= 0xE6) || (ch >= 0xE8 && ch <= 0xEF) || (ch >= 0xF2 && ch <= 0xF6) || static_cast<uint16_t>(ch - 0xF8) <= 4;
    }
}

// VERIFY: 7f584b59 manual
// OFFSET: 0x00481C80
static char* LanguageRule2(const char* text) {
    uint16_t buffer[0x2000];

    SUniConvertUTF8to16(buffer, sizeof(buffer) / sizeof(buffer[0]), text, STORM_MAX_STR, nullptr, nullptr);

    uint16_t* dest = s_languageWideBuffer;
    const uint16_t* src = buffer;

    if (buffer[0]) {
        do {
            if (*src == '|') {
                src++;

                if (*src == '2' && (src[1] > '9' || src[1] < '0')) {
                    src++;

                    while (*src == ' ') {
                        src++;
                    }

                    uint16_t ch = *src;

                    if (ch == 'L' && src[1] == '\'') {
                        *dest++ = 'd';
                        *dest++ = 'e';
                        *dest++ = ' ';
                        *dest++ = 'l';
                        *dest = '\'';
                        src += 2;
                    } else if (ch == 'L' && src[1] == 'e' && src[2] == ' ') {
                        *dest++ = 'd';
                        *dest++ = 'u';
                        *dest = ' ';
                        src += 3;
                    } else if (ch == 'L' && src[1] == 'a' && src[2] == ' ') {
                        *dest++ = 'd';
                        *dest++ = 'e';
                        *dest++ = ' ';
                        *dest++ = 'l';
                        *dest++ = 'a';
                        *dest = ' ';
                        src += 3;
                    } else if (ch == 'L' && src[1] == 'e' && src[2] == 's' && src[3] == ' ') {
                        *dest++ = 'd';
                        *dest++ = 'e';
                        *dest++ = 's';
                        *dest = ' ';
                        src += 4;
                    } else if (LanguageRule2IsVowel(ch) || ch == 'h' || ch == 'H' || (ch == 'y' && LanguageRule2IsVowel(src[1])) || (ch == 'Y' && LanguageRule2IsVowel(src[1]))) {
                        *dest++ = 'd';
                        *dest = '\'';
                    } else {
                        *dest++ = 'd';
                        *dest++ = 'e';
                        *dest = ' ';
                    }
                } else {
                    *dest = '|';
                }

                dest++;
            }

            *dest = *src;
            src++;
            dest++;
        } while (*src);
    }

    *dest = 0;
    SUniConvertUTF16to8(s_languageBuffer, sizeof(s_languageBuffer), s_languageWideBuffer, STORM_MAX_STR, nullptr, nullptr);

    return s_languageBuffer;
}

// VERIFY: 1b50c04d manual
// OFFSET: 0x00481E90
static char* LanguageRule3(const char* text) {
    char copy[0x2000];
    char word[0x400];

    if (text == s_languageBuffer) {
        SStrCopy(copy, text, sizeof(copy));
        text = copy;
    }

    const char* src = text;
    char* dest = s_languageBuffer;

    while (*src) {
        if (*src == '|') {
            src++;

            if (*src == '3' && (src[1] > '9' || src[1] < '0')) {
                if (src[1] == '-' && src[2] >= '0' && src[2] <= '9') {
                    src += 2;
                }

                uint32_t declension = SStrToUnsigned(&src);

                if (*src == '(') {
                    src++;
                }

                char* wordEnd = word;

                while (*src && *src != ')') {
                    if (wordEnd < &word[sizeof(word) - 1]) {
                        *wordEnd++ = *src;
                    }

                    src++;
                }

                if (!*src) {
                    src--;
                }

                *wordEnd = 0;
                if (!CSimpleRender::s_declinedWord) throw std::runtime_error("Game name-declension service is not connected to the native UI");
                dest += CSimpleRender::s_declinedWord(word, declension, dest, &s_languageBuffer[sizeof(s_languageBuffer)] - dest);
            } else {
                *dest = '|';
                dest++;
                *dest = *src;
                dest++;
            }
        } else {
            *dest = *src;
            dest++;
        }

        src++;
    }

    *dest = 0;

    return s_languageBuffer;
}

// VERIFY: 1e0898ef manual
// OFFSET: 0x00481FC0
static char* LanguageRule4(const char* text) {
    bool wasDigit = false;
    int32_t number = 0;
    const char* src = text;
    char* dest = s_languageBuffer;

    while (*src) {
        char ch = *src;
        bool isDigit;

        if (ch >= '0' && ch <= '9') {
            number = wasDigit ? number * 10 + ch - '0' : ch - '0';
            isDigit = true;
        } else {
            isDigit = false;
        }

        if (ch == '|') {
            src++;

            if (*src == '4' && (src[1] > '9' || src[1] < '0')) {
                src++;

                uint32_t index = FrameScript_GetPluralIndex(number);

                for (uint32_t i = 0; i < index && *src; i++) {
                    while (*src != ':' && *src != ';') {
                        src++;

                        if (!*src) {
                            break;
                        }
                    }

                    if (!*src || *src == ';') {
                        break;
                    }

                    src++;
                }

                while (*src == ' ') {
                    src++;
                }

                if (!*src) {
                    break;
                }

                while (*src != ':' && *src != ';') {
                    *dest = *src;
                    src++;
                    dest++;

                    if (!*src) {
                        *dest = 0;
                        return s_languageBuffer;
                    }
                }

                while (*src != ';') {
                    src++;

                    if (!*src) {
                        *dest = 0;
                        return s_languageBuffer;
                    }
                }
            } else {
                *dest = '|';
                dest++;
                *dest = *src;
                dest++;
            }
        } else {
            *dest = ch;
            dest++;
        }

        src++;
        wasDigit = isDigit;
    }

    *dest = 0;

    return s_languageBuffer;
}

// VERIFY: 44de3423 manual
// OFFSET: 0x00482110
const char* LanguageProcess(const char* text) {
    if (SStrLen(text) > sizeof(s_languageBuffer)) {
        memcpy(s_languageBuffer, text, sizeof(s_languageBuffer));
        s_languageBuffer[sizeof(s_languageBuffer) - 3] = 0;
        s_languageBuffer[sizeof(s_languageBuffer) - 2] = 0;
        s_languageBuffer[sizeof(s_languageBuffer) - 1] = 0;
        text = s_languageBuffer;
    }

    for (;;) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(text);
        int32_t chars;
        uint32_t ch;
        int32_t rule = 0;

        while ((ch = sgetu8(src, &chars)) != 0xFFFFFFFF) {
            src += chars;

            if (ch != '|') {
                continue;
            }

            if (*src == '|') {
                src++;
                continue;
            }

            if (*src >= '0' && *src <= '9') {
                rule = SStrToInt(reinterpret_cast<const char*>(src));

                if (rule >= 1 && rule <= 4) {
                    break;
                }
            }
        }

        if (ch == 0xFFFFFFFF) {
            return text;
        }

        switch (rule) {
            case 1:
                text = LanguageRule1(text);
                break;

            case 2:
                text = LanguageRule2(text);
                break;

            case 3:
                text = LanguageRule3(text);
                break;

            case 4:
                text = LanguageRule4(text);
                break;
        }
    }
}

// INLINED
int32_t CSimpleFontString::GetObjectType() {
    if (!CSimpleFontString::s_objectType) {
        CSimpleFontString::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleFontString::s_objectType;
}

// VERIFY: a08564ec auto
// OFFSET: 0x00485390
bool CSimpleFontString::IsA(int32_t type) {
    return type == CSimpleFontString::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 9da7e913 auto
// OFFSET: 0x004853C0
bool CSimpleFontString::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleFontString::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptRegion::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptObject::s_objectTypeName, STORM_MAX_STR);
}

// VERIFY: 3a930695 auto
// OFFSET: 0x00485420
const char* CSimpleFontString::GetObjectTypeName() {
    return CSimpleFontString::s_objectTypeName;
}

// VERIFY: 210fa960 auto
// OFFSET: 0x00485430
int32_t CSimpleFontString::GetScriptMetaTable() {
    return CSimpleFontString::s_metatable;
}

// VERIFY: c3b240f1 auto
// OFFSET: 0x00482270
const char* CSimpleFontString::GetFontName() {
    return this->m_fontHandle ? TextBlockGetFontName(this->m_fontHandle) : nullptr;
}

// VERIFY: b8a061b3 auto
// OFFSET: 0x00482300
uint32_t CSimpleFontString::GetFontFlags() {
    return this->m_fontHandle ? TextBlockGetFontFlags(this->m_fontHandle) : 0;
}

// VERIFY: 86381625 auto
// OFFSET: 0x00482E90
void CSimpleRender::Init() {
    g_theGxDevicePtr->ShaderCreate(CSimpleRender::s_vertexShader, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
}

// VERIFY: ea0ca375 manual
// OFFSET: 0x00482EC0
void CSimpleRender::Destroy() {
    for (int32_t i = 0; i < 2; i++) {
        if (CSimpleRender::s_vertexShader[i]) {
            g_theGxDevicePtr->ShaderDestroy(CSimpleRender::s_vertexShader[i]);
        }
    }
}

// VERIFY: d4f7147d manual
// OFFSET: 0x00482F00
int32_t CSimpleRender::CalcBatchCounts(CRenderBatch* batch, uint32_t* vertCounts, uint32_t* idxCounts) {
    CGxTex* textureID = reinterpret_cast<CGxTex*>(-1);
    int32_t alphaMode = GxBlends_Last;
    CGxShader* shader = nullptr;
    int32_t batchIndex = -1;
    uint32_t vertTotal = 0;
    uint32_t idxTotal = 0;

    for (uint32_t i = 0; i < batch->m_texturelist.Count(); i++) {
        CSimpleBatchedMesh* mesh = &batch->m_texturelist[i];
        int32_t changed = 0;

        if (mesh->m_textureID != textureID) {
            textureID = mesh->m_textureID;
            changed = 1;
        }

        int32_t meshAlphaMode = (mesh->m_color && mesh->m_alphaMode < GxBlend_Alpha) ? GxBlend_Alpha : mesh->m_alphaMode;

        if (meshAlphaMode != alphaMode) {
            alphaMode = meshAlphaMode;
            changed = 1;
        }

        if (mesh->m_shader != shader) {
            shader = mesh->m_shader;
            changed = 1;
        }

        if (!CSimpleRender::s_mergeEnable || changed || idxTotal + mesh->m_idxCount >= 0x20000 || vertTotal + mesh->m_posCount >= 0x10000) {
            if (batchIndex > -1) {
                vertCounts[batchIndex] = vertTotal;
                idxCounts[batchIndex] = idxTotal;
            }

            batchIndex++;
            vertTotal = 0;
            idxTotal = 0;
        }

        vertTotal += mesh->m_posCount;
        idxTotal += mesh->m_idxCount;
    }

    if (idxTotal) {
        vertCounts[batchIndex] = vertTotal;
        idxCounts[batchIndex] = idxTotal;
        batchIndex++;
    }

    return batchIndex;
}

// VERIFY: 9b5bcef4 manual
// OFFSET: 0x00484B00
void CSimpleRender::DrawBatch(CRenderBatch* batch) {
    uint32_t meshCount = batch->m_texturelist.Count();

    if (meshCount) {
        CGxTex* textureID = reinterpret_cast<CGxTex*>(-1);
        int32_t alphaMode = GxBlends_Last;
        CGxShader* shader = nullptr;

        g_theGxDevicePtr->RsPush();

        CGxShader* vertexShader = g_theGxDevicePtr->StereoEnabled() ? CSimpleRender::s_vertexShader[1] : CSimpleRender::s_vertexShader[0];

        if (vertexShader && vertexShader->Valid()) {
            C44Matrix viewProj;
            GxRsSet(GxRs_VertexShader, vertexShader);
            GxXformViewProjNativeTranspose(viewProj);
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 0, reinterpret_cast<const float*>(&viewProj), 4);
        } else {
            GxRsSet(GxRs_Lighting, 0);
        }

        GxRsSet(GxRs_Fog, 0);
        GxRsSet(GxRs_DepthTest, 0);
        GxRsSet(GxRs_DepthWrite, 0);

        uint32_t* vertCounts = static_cast<uint32_t*>(alloca(sizeof(uint32_t) * meshCount));
        uint32_t* idxCounts = static_cast<uint32_t*>(alloca(sizeof(uint32_t) * meshCount));
        CSimpleRender::CalcBatchCounts(batch, vertCounts, idxCounts);

        CGxBuf* vertexBuf = nullptr;
        CGxBuf* indexBuf = nullptr;
        CGxVertexPCT* vertices = nullptr;
        uint16_t* indices = nullptr;
        uint32_t vertTotal = 0;
        int32_t idxTotal = 0;
        int32_t batchIndex = -1;

        for (uint32_t i = 0; i < meshCount; i++) {
            CSimpleBatchedMesh* mesh = &batch->m_texturelist[i];
            int32_t changed = 0;

            if (mesh->m_textureID != textureID) {
                textureID = mesh->m_textureID;
                changed = 1;
            }

            int32_t meshAlphaMode = (mesh->m_color && mesh->m_alphaMode < GxBlend_Alpha) ? GxBlend_Alpha : mesh->m_alphaMode;

            if (meshAlphaMode != alphaMode) {
                alphaMode = meshAlphaMode;
                changed = 1;
            }

            if (mesh->m_shader != shader) {
                shader = mesh->m_shader;
                changed = 1;
            }

            bool newBatch = !CSimpleRender::s_mergeEnable || changed || idxTotal + mesh->m_idxCount >= 0x20000 || vertTotal + mesh->m_posCount >= 0x10000;

            if (newBatch) {
                if (idxTotal > 0) {
                    GxBufUnlock(vertexBuf, 0);
                    GxBufUnlock(indexBuf, 0);
                    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);
                    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

                    CGxBatch gxBatch;
                    gxBatch.m_primType = GxPrim_Triangles;
                    gxBatch.m_start = 0;
                    gxBatch.m_count = idxCounts[batchIndex];
                    gxBatch.m_minIndex = 0;
                    gxBatch.m_maxIndex = vertCounts[batchIndex] - 1;
                    GxDraw(&gxBatch, 1);
                }

                batchIndex++;
                idxTotal = 0;
                vertTotal = 0;

                GxRsSet(GxRs_Texture0, TextureGetGxTex(mesh->m_texture, 1, nullptr));
                GxRsSet(GxRs_BlendingMode, alphaMode);
                GxUpdateAlphaRef(GxRs_AlphaRef);
                GxRsSet(GxRs_PixelShader, shader);
            }

            if (newBatch || !vertices) {
                vertexBuf = GxBufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), vertCounts[batchIndex]);
                indexBuf = GxBufStream(GxPoolTarget_Index, sizeof(uint16_t), idxCounts[batchIndex]);
                vertices = reinterpret_cast<CGxVertexPCT*>(GxBufLock(vertexBuf));
                indices = reinterpret_cast<uint16_t*>(GxBufLock(indexBuf));
            }

            if (!mesh->m_indices) {
                continue;
            }

            for (uint32_t v = 0; v < mesh->m_posCount; v++) {
                vertices->p = mesh->m_position[v];

                if (mesh->m_color) {
                    CImVector color = mesh->m_color[(v * mesh->m_colorCount) >> 2];
                    GxFormatColor(color);
                    vertices->c = color;
                } else {
                    CImVector white;
                    white.b = 0xFF;
                    white.g = 0xFF;
                    white.r = 0xFF;
                    white.a = 0xFF;
                    vertices->c = white;
                }

                if (mesh->m_onAtlas) {
                    vertices->tc.x = mesh->m_atlasScale * mesh->m_texCoord[v].x + mesh->m_atlasOffset.x;
                    vertices->tc.y = mesh->m_atlasScale * mesh->m_texCoord[v].y + mesh->m_atlasOffset.y;
                } else {
                    vertices->tc = mesh->m_texCoord[v];
                }

                vertices++;
            }

            for (uint32_t j = 0; j < mesh->m_idxCount; j++) {
                *indices = vertTotal + mesh->m_indices[j];
                indices++;
            }

            vertTotal += mesh->m_posCount;
            idxTotal += mesh->m_idxCount;
        }

        if (idxTotal > 0) {
            GxBufUnlock(vertexBuf, sizeof(CGxVertexPCT) * vertTotal);
            GxBufUnlock(indexBuf, sizeof(uint16_t) * idxTotal);
            GxPrimVertexPtr(vertexBuf, GxVBF_PCT);
            g_theGxDevicePtr->PrimIndexPtr(indexBuf);

            CGxBatch gxBatch;
            gxBatch.m_primType = GxPrim_Triangles;
            gxBatch.m_start = 0;
            gxBatch.m_count = idxCounts[batchIndex];
            gxBatch.m_minIndex = 0;
            gxBatch.m_maxIndex = vertCounts[batchIndex] - 1;
            GxDraw(&gxBatch, 1);
        }

        g_theGxDevicePtr->RsPop();
    }

    if (batch->m_stringbatch) {
        GxuFontRenderBatch(batch->m_stringbatch);
    }

    for (RENDERCALLBACKNODE* node = batch->m_callbacks.Head(); !TSList<RENDERCALLBACKNODE, TSGetLink<RENDERCALLBACKNODE> >::IsEnd(node); node = node->RawNext()) {
        node->m_callback(node->m_param);
    }
}

// VERIFY: e5b220d7 auto
// OFFSET: 0x00482200
void CSimpleFontString::RemoveShadow() {
    if (this->m_styleFlags & 0x100) {
        this->m_styleFlags &= ~0x100u;

        if (this->m_string) {
            TextBlockRemoveShadow(this->m_string);
        }
    }
}

// VERIFY: e7bf854c auto
// OFFSET: 0x00482290
float CSimpleFontString::GetFontHeight(bool recalculate) {
    float fontHeight = this->m_fontHeight;

    if (recalculate && this->m_fontHandle && (this->m_styleFlags & 0x200)) {
        float ndcHeight = GxuFontGetOneToOneHeight(TextBlockGetFontPtr(this->m_fontHandle)) / this->m_layoutScale;
        NDCToDDC(0.0f, ndcHeight, nullptr, &fontHeight);
    }

    return fontHeight;
}

// VERIFY: 323fc4f3 auto
// OFFSET: 0x004834C0
void CSimpleTexture::StoreAnimPosition() {
    this->CSimpleRegion::StoreAnimPosition();
    this->SetPosition(this->m_rect, this->m_storedPositions);
    memcpy(this->m_positions, this->m_storedPositions, sizeof(this->m_positions));
}

// VERIFY: c6bab0dc auto
// OFFSET: 0x004834F0
void CSimpleTexture::ResetAnimPosition() {
    this->CSimpleRegion::ResetAnimPosition();
    this->SetPosition(this->m_rect, this->m_positions);
}

// VERIFY: 496d4923 auto
// OFFSET: 0x00483A40
const char* CSimpleFontString::GetDisplayText(float width, float height) {
    const char* text = this->m_text && *this->m_text ? this->m_text : nullptr;

    if (!this->m_fontHandle || !text || !*text || width <= 0.0f) {
        return text;
    }

    if (!(this->m_maxLines & 0xFFFFFF) && height <= 0.0f) {
        return text;
    }

    float fontHeight = this->GetFontHeight(true);
    float lineHeight = (CSimpleTop::RoundToPixelHeight(this->m_spacing) + fontHeight) * this->m_layoutScale;

    if (this->m_maxLines & 0xFFFFFF) {
        height = lineHeight * (this->m_maxLines & 0xFFFFFF);
    } else if (lineHeight >= height) {
        height = lineHeight;
    }

    uint32_t numBytes = SStrLen(text);

    if (TextBlockGetMaxCharsWithinWidthAndHeight(this->m_fontHandle, text, this->m_layoutScale * fontHeight, width, height, numBytes, this->m_layoutScale * this->m_shadowOffset.x, this->m_layoutScale, this->m_layoutScale * this->m_spacing, this->m_styleFlags) >= numBytes) {
        return text;
    }

    if (numBytes >= sizeof(s_displayText) - 4) {
        numBytes = sizeof(s_displayText) - 4;
    }

    SStrCopy(s_displayText, text, sizeof(s_displayText));

    uint32_t count;

    do {
        if (numBytes) {
            char ch;

            do {
                ch = s_displayText[numBytes - 1];
                numBytes--;
            } while ((ch & 0xC0) == 0x80 && numBytes);
        }

        SStrCopy(&s_displayText[numBytes], "...", STORM_MAX_STR);
        count = TextBlockGetMaxCharsWithinWidthAndHeight(this->m_fontHandle, s_displayText, this->m_layoutScale * fontHeight, width, height, numBytes + 3, this->m_layoutScale * this->m_shadowOffset.x, this->m_layoutScale, this->m_layoutScale * this->m_spacing, this->m_styleFlags);
    } while (numBytes && count < numBytes + 3);

    return s_displayText;
}

// VERIFY: 586014e2 auto
// OFFSET: 0x004840F0
bool CSimpleFontString::SetLayoutScale(float scale, bool force) {
    if (!this->CLayoutFrame::SetLayoutScale(scale, force)) {
        return false;
    }

    if (this->m_fontHandle) {
        char fontName[1024];
        SStrCopy(fontName, this->GetFontName(), sizeof(fontName));

        uint32_t fontFlags = this->GetFontFlags();
        float fontHeight = this->m_fontHeight;
        HTEXTFONT font = nullptr;

        if (fontHeight == 0.0f || (font = TextBlockGenerateFont(fontName, fontFlags, fontHeight * this->m_layoutScale)) != nullptr) {
            this->m_fontHeight = fontHeight;

            if (this->m_string) {
                HandleClose(reinterpret_cast<HOBJECT>(this->m_string));
                this->m_string = nullptr;
            }

            if (this->m_fontHandle) {
                HandleClose(reinterpret_cast<HOBJECT>(this->m_fontHandle));
            }

            this->m_fontHandle = font;

            if (font) {
                this->m_textWidth = 0.0f;
                this->m_textHeight = 0.0f;

                if (this->m_string) {
                    HandleClose(reinterpret_cast<HOBJECT>(this->m_string));
                    this->m_string = nullptr;
                }

                this->m_layoutFlags &= ~0x100u;
                this->Resize(0);
            }
        }
    }

    return true;
}

// VERIFY: 88bc13dc auto
// OFFSET: 0x00485140
void CSimpleTexture::Draw(CRenderBatch* batch) {
    if (this->m_texture) {
        batch->QueueTexture(this);
    }
}

// VERIFY: 45a4d11b auto
// OFFSET: 0x00485850
void CSimpleFontString::QueueTextures(CRenderBatch* batch) {
    for (CSimpleEmbeddedTexture* embedded = this->m_textures.Head(); !TSList<CSimpleEmbeddedTexture, TSGetExplicitLink<CSimpleEmbeddedTexture> >::IsEnd(embedded); embedded = this->m_textures.RawNext(embedded)) {
        if (embedded->m_texture && embedded->m_texture->m_texture) {
            batch->QueueTexture(embedded->m_texture);
        }
    }
}

// VERIFY: 502e17cc auto
// OFFSET: 0x00485D60
void CSimpleFontString::Draw(CRenderBatch* batch) {
    if (this->m_fontHandle && this->m_text && *this->m_text) {
        batch->QueueFontString(this);
        this->QueueTextures(batch);
    }
}

// VERIFY: 70fe81b7 auto
// OFFSET: 0x00485440
void CSimpleFontString::FreeEmbeddedTextures() {
    CSimpleEmbeddedTexture* embedded = this->m_textures.Head();

    while (!TSList<CSimpleEmbeddedTexture, TSGetExplicitLink<CSimpleEmbeddedTexture> >::IsEnd(embedded)) {
        CSimpleEmbeddedTexture* next = this->m_textures.RawNext(embedded);
        this->m_textures.UnlinkNode(embedded);
        embedded->m_texture->m_shown = 0;
        embedded->m_texture->HideThis();
        s_embeddedTextureStorage.LinkToTail(embedded);
        embedded = next;
    }
}

// VERIFY: 23bee593 manual
// OFFSET: 0x004858B0
void CSimpleFontString::FreeEmbededTextureStorage() {
    CSimpleEmbeddedTexture* embedded = s_embeddedTextureStorage.Head();

    while (!TSList<CSimpleEmbeddedTexture, TSGetExplicitLink<CSimpleEmbeddedTexture> >::IsEnd(embedded)) {
        embedded = s_embeddedTextureStorage.DeleteNode(embedded);
    }
}

// INLINED
CSimpleEmbeddedTexture* CSimpleFontString::GetEmbeddedTextureFromStorage() {
    CSimpleEmbeddedTexture* embedded = s_embeddedTextureStorage.Head();

    if (embedded) {
        s_embeddedTextureStorage.UnlinkNode(embedded);
        return embedded;
    }

    return new (SMemAlloc(sizeof(CSimpleEmbeddedTexture), __FILE__, __LINE__, 0)) CSimpleEmbeddedTexture;
}

// VERIFY: 947663f7 manual
// OFFSET: 0x00485DA0
void CSimpleFontString::RefreshEmbeddedTextures() {
    const GXUEMBEDDEDTEXTUREINFO* info;
    uint32_t count = GxuFontStringEmbeddedTextureInfo(this->m_string ? TextBlockGetStringPtr(this->m_string) : nullptr, info);

    for (uint32_t i = 0; i < count; i++) {
        CSimpleEmbeddedTexture* embedded = CSimpleFontString::GetEmbeddedTextureFromStorage();
        this->m_textures.LinkToTail(embedded);
        embedded->InitEmbeddedTexture(this, &info[i]);
    }
}

// INLINED
void CSimpleFontString::operator delete(void* ptr) {
    if (ptr) {
        CSimpleFontString::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: ce8bf3cc manual
// OFFSET: 0x00485240
CSimpleFontString::CSimpleFontString(CSimpleFrame* frame, uint32_t drawLayer, bool show) : CSimpleRegion(frame, drawLayer, show) {
    this->m_spacing = 0.0f;
    this->m_textWidth = 0.0f;
    this->m_textHeight = 0.0f;
    this->m_fontHandle = nullptr;
    this->m_textMaxSize = 0;
    this->m_textCurSize = 0;
    this->m_text = nullptr;
    this->m_string = nullptr;
    this->m_shadowColor.a = 0xFF;
    this->m_alphaGradientStart = -1;
    this->m_styleFlags = 0x212;
    this->m_settableStyleFlags = 0x2107F;
    this->m_maxLines = (this->m_maxLines & 0xFE000000) | 0x1000000;
    s_fontStringCount++;
}

// VERIFY: fcafba60 auto
// OFFSET: [0x00485370, 0x00485380, 0x00485600, 0x00485D30]
CSimpleFontString::~CSimpleFontString() {
    this->FreeEmbeddedTextures();

    if (this->m_text) {
        SMemFree(this->m_text, __FILE__, __LINE__, 0);
        this->m_text = nullptr;
    }

    this->SetFont(nullptr, 0.0f, 0, false);

    s_fontStringCount--;
}

// VERIFY: f646e84a manual
// OFFSET: 0x004818F0
void CSimpleFontString::SetTextLength(uint32_t length) {
    if (length == this->m_textMaxSize) {
        return;
    }

    if (length) {
        char* text = static_cast<char*>(SMemAlloc(length + 1, __FILE__, __LINE__, 0));
        *text = 0;

        if (this->m_text) {
            SMemFree(this->m_text, __FILE__, __LINE__, 0);
        }

        this->m_text = text;
        this->m_textCurSize = length;
    }

    this->m_textMaxSize = length;
}

// VERIFY: df02ba6a manual
// OFFSET: 0x00482230
bool CSimpleFontString::SetAlphaGradient(int32_t start, int32_t length) {
    this->m_alphaGradientStart = start;
    this->m_alphaGradientLength = length;

    return !this->m_string || TextBlockSetGradient(this->m_string, start, length) < 2;
}

// VERIFY: 8dc263f9 auto
// OFFSET: 0x00482320
void CSimpleFontString::SetWidth(float width) {
    this->CLayoutFrame::SetWidth(width);
    this->m_textWidth = 0.0f;
    this->m_textHeight = 0.0f;
}

// VERIFY: 0b7750c4 auto
// OFFSET: 0x00482350
void CSimpleFontString::SetHeight(float height) {
    this->CLayoutFrame::SetHeight(height);
    this->m_textWidth = 0.0f;
    this->m_textHeight = 0.0f;
}

// VERIFY: fd665b85 auto
// OFFSET: 0x00482380
float CSimpleFontString::GetTextWidth(const char* text, uint32_t numBytes) {
    float extent = 0.0f;

    if (!this->m_fontHandle) {
        return extent;
    }

    if (!numBytes) {
        numBytes = SStrLen(text);
    }

    TextBlockGetTextExtent(this->m_fontHandle, text, numBytes, this->m_layoutScale * this->GetFontHeight(true), &extent, this->m_shadowOffset.x * this->m_layoutScale, this->m_layoutScale, 0.0f, this->m_styleFlags);

    return extent / this->m_layoutScale;
}

// VERIFY: 7b82c1df manual
// OFFSET: 0x00482450
uint32_t CSimpleFontString::WrapText(const char* text, float width, uint32_t* lineStarts, uint32_t maxLines) {
    if (!this->m_fontHandle) {
        return 0;
    }

    return TextBlockWrapText(this->m_fontHandle, text, this->GetFontHeight(true) * this->m_layoutScale, width * this->m_layoutScale, lineStarts, maxLines, this->m_shadowOffset.x * this->m_layoutScale, this->m_layoutScale, 0.0f, this->m_styleFlags);
}

// VERIFY: 92162b43 manual
// OFFSET: 0x00482510
uint32_t CSimpleFontString::GetNumCharsWithinWidth(const char* text, uint32_t numBytes, float maxWidth) {
    if (!this->m_fontHandle) {
        return 0;
    }

    if (!numBytes) {
        numBytes = SStrLen(text);
    }

    float extent;

    return TextBlockGetMaxCharsWithinWidth(this->m_fontHandle, text, this->m_layoutScale * this->GetFontHeight(true), this->m_layoutScale * maxWidth, numBytes, &extent, this->m_shadowOffset.x * this->m_layoutScale, this->m_layoutScale, 0.0f, this->m_styleFlags);
}

// VERIFY: 211ff357 manual
// OFFSET: 0x004825E0
uint32_t CSimpleFontString::GetNumCharsWithinWidthFromEnd(const char* text, uint32_t numBytes, float maxWidth) {
    if (!this->m_fontHandle) {
        return 0;
    }

    if (!numBytes) {
        numBytes = SStrLen(text);
    }

    float extent;

    return TextBlockGetMaxCharsWithinWidthFromEnd(this->m_fontHandle, text, this->m_layoutScale * this->GetFontHeight(true), this->m_layoutScale * maxWidth, numBytes, &extent, this->m_shadowOffset.x * this->m_layoutScale, this->m_layoutScale, 0.0f, this->m_styleFlags);
}

// VERIFY: 5d78882a auto
// OFFSET: 0x004826B0
void CSimpleFontString::OnScreenSizeChanged() {
    this->m_textWidth = 0.0f;
    this->m_textHeight = 0.0f;
    this->Resize(0);
}

// VERIFY: 5cc3b167 auto
// OFFSET: 0x00482720
void CSimpleFontString::AddAnimTranslation(const C2Vector& translation) {
    if (this->m_string) {
        C3Vector position = TextBlockGetStringPos(this->m_string);
        ApplyTranslationAnim(position, translation);
        TextBlockSetStringPos(this->m_string, position);
    }
}

// VERIFY: ebf55995 auto
// OFFSET: 0x00482E10
void CSimpleFontString::ClearString() {
    this->m_textWidth = 0.0f;
    this->m_textHeight = 0.0f;

    if (this->m_string) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_string));
        this->m_string = nullptr;
    }

    this->m_layoutFlags &= ~0x100u;
    this->Resize(0);
}

// VERIFY: 45b31c86 manual
// OFFSET: 0x004836D0
void CSimpleFontString::PostLoadXML(XMLNode* node, CStatus* status) {
    if (!this->m_parent || this->HasAnyFramepoints()) {
        return;
    }

    if (this->m_styleFlags & 0x1) {
        this->SetPoint(FRAMEPOINT_LEFT, this->m_parent, FRAMEPOINT_LEFT, 0.0f, 0.0f, 1);
    } else if (this->m_styleFlags & 0x4) {
        this->SetPoint(FRAMEPOINT_RIGHT, this->m_parent, FRAMEPOINT_RIGHT, 0.0f, 0.0f, 1);
    } else {
        this->SetPoint(FRAMEPOINT_CENTER, this->m_parent, FRAMEPOINT_CENTER, 0.0f, 0.0f, 1);
    }
}

// VERIFY: 96073ff2 auto
// OFFSET: 0x00483750
int32_t CSimpleFontString::SetFont(const char* fontName, float fontHeight, uint32_t fontFlags, bool force) {
    if (!force) {
        const char* currentName = this->GetFontName();

        if (fontName && currentName && !SStrCmpI(fontName, currentName, STORM_MAX_STR) && fequal(fontHeight, this->m_fontHeight) && fontFlags == this->GetFontFlags()) {
            return 1;
        }
    }

    HTEXTFONT font = nullptr;

    if (fontName && fontHeight != 0.0f) {
        font = TextBlockGenerateFont(fontName, fontFlags, fontHeight * this->m_layoutScale);

        if (!font) {
            return 0;
        }
    }

    this->m_fontHeight = fontHeight;

    if (this->m_string) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_string));
        this->m_string = nullptr;
    }

    if (this->m_fontHandle) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_fontHandle));
    }

    this->m_fontHandle = font;

    if (font) {
        this->ClearString();
    }

    return 1;
}

// VERIFY: 3d8d8078 auto
// OFFSET: 0x00483890
void CSimpleFontString::SetTextHeight(float height) {
    if (fabsf(height - this->m_fontHeight) >= 0.00000023841858f) {
        this->m_fontHeight = height;
        this->m_styleFlags &= ~0x200u;
        this->ClearString();
    }
}

// VERIFY: 5aa8ddc3 manual
// OFFSET: 0x00483910
void CSimpleFontString::SetText(const char* text, int32_t process) {
    if (text && process) {
        text = LanguageProcess(text);
    }

    if ((!text || !*text) && (!this->m_text || !*this->m_text)) {
        return;
    }

    if (text) {
        if (this->m_text && !SStrCmp(text, this->m_text, STORM_MAX_STR)) {
            return;
        }

        if (*text) {
            if (this->m_textMaxSize) {
                SStrCopy(this->m_text, text, this->m_textMaxSize);
            } else {
                uint32_t length = SStrLen(text);

                if (length > this->m_textCurSize) {
                    if (this->m_text) {
                        SMemFree(this->m_text, __FILE__, __LINE__, 0);
                    }

                    this->m_text = SStrDupA(text, __FILE__, __LINE__);
                    this->m_textCurSize = length;
                } else {
                    SStrCopy(this->m_text, text, STORM_MAX_STR);
                }
            }

            this->ClearString();
            return;
        }
    }

    if (this->m_text) {
        *this->m_text = 0;
    }

    this->ClearString();
}

// VERIFY: 63874679 auto
// OFFSET: 0x00483C40
void CSimpleFontString::SetJustificationOffset(float x, float y) {
    if (x == this->m_justificationOffset.x && this->m_justificationOffset.y == y) {
        return;
    }

    this->m_justificationOffset.x = x;
    this->m_justificationOffset.y = y;

    if (this->m_string) {
        C3Vector position;
        position.x = this->m_layoutScale * this->m_justificationOffset.x + this->m_rect.minX;
        position.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
        position.z = this->m_layoutDepth;
        TextBlockSetStringPos(this->m_string, position);
    }
}

// VERIFY: 273d1f7a manual
// OFFSET: 0x00483CD0
void CSimpleFontString::AddShadow(const CImVector& color, const C2Vector& offset) {
    if ((this->m_styleFlags & 0x100) && *reinterpret_cast<const uint32_t*>(&color) == *reinterpret_cast<uint32_t*>(&this->m_shadowColor) && this->m_shadowOffset.x == offset.x && this->m_shadowOffset.y == offset.y) {
        return;
    }

    this->m_styleFlags |= 0x100;
    this->m_shadowColor = color;
    this->m_shadowOffset = offset;

    if (this->m_string) {
        CImVector shadowColor = this->m_shadowColor;
        C2Vector shadowOffset;
        shadowOffset.x = this->m_shadowOffset.x * this->m_layoutScale;
        shadowOffset.y = this->m_shadowOffset.y * this->m_layoutScale;
        uint8_t alpha = this->m_parent->m_alpha * this->m_parent->m_parentAlpha / 255;
        shadowColor.a = this->m_shadowColor.a * alpha / 255;
        TextBlockAddShadow(this->m_string, shadowColor, shadowOffset);
    }
}

// VERIFY: e9c58d61 manual
// OFFSET: 0x00483DD0
void CSimpleFontString::SetShadowOffset(const C2Vector& offset) {
    if (this->m_shadowOffset.x == offset.x && this->m_shadowOffset.y == offset.y) {
        return;
    }

    this->m_shadowOffset = offset;

    if (this->m_shadowOffset.x == 0.0f && this->m_shadowOffset.y == 0.0f) {
        this->RemoveShadow();
    } else {
        this->AddShadow(this->m_shadowColor, this->m_shadowOffset);
    }

    this->ClearString();
}

// VERIFY: 230de876 auto
// OFFSET: 0x00483E80
float CSimpleFontString::GetStringWidth() {
    if (this->m_textWidth == 0.0f && this->m_fontHandle) {
        const char* text = this->GetDisplayText(this->CLayoutFrame::GetWidth() * this->m_layoutScale, this->CLayoutFrame::GetHeight() * this->m_layoutScale);

        if (text && *text) {
            TextBlockGetTextExtent(this->m_fontHandle, text, SStrLen(text), this->GetFontHeight(true) * this->m_layoutScale, &this->m_textWidth, this->m_shadowOffset.x * this->m_layoutScale, this->m_layoutScale, 0.0f, this->m_styleFlags);
        }
    }

    return this->m_textWidth / this->m_layoutScale;
}

// VERIFY: 9a3e23aa auto
// OFFSET: 0x00483F30
float CSimpleFontString::GetWidth() {
    float width = this->CLayoutFrame::GetWidth();

    if (width == 0.0f) {
        width = this->GetStringWidth();
    }

    return width > NDCToDDCWidth(1.0f / (CoordinateGetAspectCompensation() * 1024.0f)) ? width : NDCToDDCWidth(1.0f / (CoordinateGetAspectCompensation() * 1024.0f));
}

// VERIFY: 5c920bfb auto
// OFFSET: 0x00483FA0
float CSimpleFontString::GetStringHeight() {
    if (this->m_textHeight == 0.0f && this->m_fontHandle) {
        const char* text = this->GetDisplayText(this->CLayoutFrame::GetWidth() * this->m_layoutScale, this->CLayoutFrame::GetHeight() * this->m_layoutScale);

        if (text && *text) {
            C2Vector shadowOffset;
            shadowOffset.x = this->m_shadowOffset.x * this->m_layoutScale;
            shadowOffset.y = this->m_layoutScale * this->m_shadowOffset.y;
            this->m_textHeight = TextBlockGetWrappedTextHeight(this->m_fontHandle, text, this->GetFontHeight(true) * this->m_layoutScale, this->GetWidth() * this->m_layoutScale, shadowOffset, this->m_layoutScale, this->m_spacing * this->m_layoutScale, this->m_styleFlags);
        }
    }

    return this->m_textHeight / this->m_layoutScale;
}

// VERIFY: 34e78a2e auto
// OFFSET: 0x00484080
float CSimpleFontString::GetHeight() {
    float height = this->CLayoutFrame::GetHeight();

    if (height == 0.0f) {
        height = this->GetStringHeight();
    }

    return height > NDCToDDCWidth(1.0f / (CoordinateGetAspectCompensation() * 1024.0f)) ? height : NDCToDDCWidth(1.0f / (CoordinateGetAspectCompensation() * 1024.0f));
}

// VERIFY: 4a6b69f4 auto
// OFFSET: 0x00484230
bool CSimpleFontString::SetLayoutDepth(float depth, bool force) {
    if (!this->CLayoutFrame::SetLayoutDepth(depth, force)) {
        return false;
    }

    if (this->m_string) {
        C3Vector position;
        position.x = this->m_justificationOffset.x * this->m_layoutScale + this->m_rect.minX;
        position.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
        position.z = this->m_layoutDepth;
        TextBlockSetStringPos(this->m_string, position);
    }

    return true;
}

// VERIFY: 026ce15c manual
// OFFSET: 0x004842A0
void CSimpleFontString::PreOnAnimUpdate() {
    if (!this->m_animating) {
        return;
    }

    if (this->m_string) {
        TextBlockSetStringPos(this->m_string, this->m_animPosition);
    }

    if (this->m_animAlpha) {
        uint8_t alpha = this->m_animSavedAlpha;
        CImVector color;
        this->GetVertexColor(color);
        color.a = alpha;
        this->SetVertexColor(color);
    }
}

// VERIFY: db02a088 manual
// OFFSET: 0x00484300
void CSimpleFontString::StoreAnimPosition() {
    this->CSimpleRegion::StoreAnimPosition();
    this->m_animPosition.x = this->m_justificationOffset.x * this->m_layoutScale + this->m_rect.minX;
    this->m_animPosition.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
    this->m_animPosition.z = this->m_layoutDepth;

    if (this->m_string) {
        TextBlockSetStringPos(this->m_string, this->m_animPosition);
    }
}

// VERIFY: c02c2d45 manual
// OFFSET: 0x00484350
void CSimpleFontString::ResetAnimPosition() {
    this->CSimpleRegion::ResetAnimPosition();
    this->m_animPosition.x = this->m_justificationOffset.x * this->m_layoutScale + this->m_rect.minX;
    this->m_animPosition.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
    this->m_animPosition.z = this->m_layoutDepth;

    if (this->m_string) {
        TextBlockSetStringPos(this->m_string, this->m_animPosition);
    }
}

// VERIFY: 50c246ae manual
// OFFSET: 0x004856F0
void CSimpleFontString::OnColorChanged(bool changed) {
    this->CSimpleRegion::OnColorChanged(changed);

    if (this->m_string) {
        CImVector color;
        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;

        if (this->m_colorCount == 1) {
            color = this->m_colors[0];
        }

        TextBlockUpdateColor(this->m_string, color);

        if (this->m_styleFlags & 0x100) {
            CImVector shadowColor = this->m_shadowColor;
            C2Vector shadowOffset;
            shadowOffset.x = this->m_shadowOffset.x * this->m_layoutScale;
            shadowOffset.y = this->m_shadowOffset.y * this->m_layoutScale;
            uint8_t alpha = this->m_parent->m_alpha * this->m_parent->m_parentAlpha / 255;
            shadowColor.a = this->m_shadowColor.a * alpha / 255;
            TextBlockAddShadow(this->m_string, shadowColor, shadowOffset);
        }
    }

    for (CSimpleEmbeddedTexture* embedded = this->m_textures.Head(); !TSList<CSimpleEmbeddedTexture, TSGetExplicitLink<CSimpleEmbeddedTexture> >::IsEnd(embedded); embedded = this->m_textures.RawNext(embedded)) {
        if (embedded->m_texture && embedded->m_texture->m_texture) {
            CImVector color;
            this->GetVertexColor(color);
            CSimpleTexture* texture = embedded->m_texture;
            uint8_t alpha = color.a;
            CImVector textureColor;
            texture->GetVertexColor(textureColor);
            textureColor.a = alpha;
            texture->SetVertexColor(textureColor);
        }
    }
}

// VERIFY: 1148256f manual
// OFFSET: 0x00486B20
void CSimpleFontString::UpdateString() {
    if (!(this->m_layoutFlags & 0x100)) {
        return;
    }

    if (this->m_string) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_string));
        this->m_string = nullptr;
    }

    this->FreeEmbeddedTextures();

    if (!(this->m_rect.maxY > this->m_rect.minY) || !(this->m_rect.maxX > this->m_rect.minX)) {
        return;
    }

    if (this->m_fontHandle && this->m_text && *this->m_text) {
        C3Vector position;
        position.x = this->m_justificationOffset.x * this->m_layoutScale + this->m_rect.minX;
        position.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
        position.z = this->m_layoutDepth;

        float width = this->CLayoutFrame::GetWidth();
        width = width != 0.0f ? width * this->m_layoutScale : this->m_textWidth;

        if (fnotequalz(width, this->m_rect.maxX - this->m_rect.minX, 0.00001f)) {
            width = this->m_rect.maxX - this->m_rect.minX;
        }

        float height = this->CLayoutFrame::GetHeight();
        height = height != 0.0f ? height * this->m_layoutScale : this->m_textHeight;

        if (fnotequalz(height, this->m_rect.maxY - this->m_rect.minY, 0.00001f)) {
            height = this->m_rect.maxY - this->m_rect.minY;
        }

        const char* text = this->GetDisplayText(width, height);
        uint32_t styleFlags = this->m_styleFlags;

        CImVector color;
        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;

        if (!(styleFlags & 0x400)) {
            bool hasColorCode = false;

            for (int32_t i = 0; text[i]; i++) {
                if (text[i] == '|') {
                    if (text[i + 1] == '|') {
                        i++;
                    } else if (text[i + 1] == 'C' || text[i + 1] == 'c') {
                        hasColorCode = true;
                        break;
                    }
                }
            }

            if (!hasColorCode) {
                styleFlags |= 0x400;
            }
        }

        if (this->m_colorCount) {
            color = this->m_colors[0];
        }

        this->m_string = TextBlockCreate(this->m_fontHandle, text, color, position, this->GetFontHeight(true) * this->m_layoutScale, width, height, styleFlags, 0.0f, this->m_spacing * this->m_layoutScale, this->m_layoutScale);

        if (this->m_styleFlags & 0x100) {
            CImVector shadowColor = this->m_shadowColor;
            uint8_t alpha = this->m_parent->GetEffectiveAlpha();
            C2Vector shadowOffset;
            shadowOffset.x = this->m_shadowOffset.x * this->m_layoutScale;
            shadowOffset.y = this->m_shadowOffset.y * this->m_layoutScale;
            shadowColor.a = this->m_shadowColor.a * alpha / 255;
            TextBlockAddShadow(this->m_string, shadowColor, shadowOffset);
        }

        if (this->m_alphaGradientStart >= 0) {
            TextBlockSetGradient(this->m_string, this->m_alphaGradientStart, this->m_alphaGradientLength);
        }

        this->RefreshEmbeddedTextures();

        if (this->m_animating) {
            this->m_animPosition = position;
            TextBlockSetStringPos(this->m_string, position);
        }
    }

    this->OnRegionChanged();
}

// VERIFY: 39fc54f9 manual
// OFFSET: 0x00486E30
void CSimpleFontString::OnFrameSizeChanged(const CRect& rect) {
    this->CLayoutFrame::OnFrameSizeChanged(rect);

    if (this->m_string && fabsf((rect.maxX - rect.minX) - (this->m_rect.maxX - this->m_rect.minX)) < 0.00001f && fabsf((rect.maxY - rect.minY) - (this->m_rect.maxY - this->m_rect.minY)) < 0.00001f) {
        C3Vector position;
        position.x = this->m_justificationOffset.x * this->m_layoutScale + this->m_rect.minX;
        position.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
        position.z = this->m_layoutDepth;
        TextBlockSetStringPos(this->m_string, position);

        if (IsRectOffscreen(this->m_rect) != IsRectOffscreen(rect)) {
            this->OnRegionChanged();
        }
    } else {
        this->UpdateString();
    }

    if (this->m_animating) {
        C3Vector previous = this->m_animPosition;
        this->m_animPosition.x = this->m_justificationOffset.x * this->m_layoutScale + this->m_rect.minX;
        this->m_animPosition.y = this->m_justificationOffset.y * this->m_layoutScale + this->m_rect.minY;
        this->m_animPosition.z = this->m_layoutDepth;

        if (this->m_string) {
            C3Vector delta = this->m_animPosition - previous;
            C3Vector position = TextBlockGetStringPos(this->m_string) + delta;
            TextBlockSetStringPos(this->m_string, position);
        }
    }

    if (this->m_parent) {
        this->m_parent->NotifyScrollParent();
    }
}

// VERIFY: 86a4fd93 auto
// OFFSET: 0x00487090
void CSimpleFontString::SetStyleFlags(uint32_t styleFlags) {
    uint32_t flags = (styleFlags & this->m_settableStyleFlags) | (this->m_styleFlags & ~this->m_settableStyleFlags);

    if (flags != this->m_styleFlags) {
        this->m_styleFlags = flags;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: ee7180a8 auto
// OFFSET: 0x004870D0
void CSimpleFontString::SetSpacing(float spacing) {
    if (spacing < 0.0f) {
        spacing = 0.0f;
    }

    if (fabsf(spacing - this->m_spacing) >= 0.00000023841858f) {
        this->m_spacing = spacing;
        this->m_textHeight = 0.0f;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: adb869c7 auto
// OFFSET: 0x00487130
void CSimpleFontString::SetShadowColor(const CImVector& color) {
    if (*reinterpret_cast<const uint32_t*>(&color) != *reinterpret_cast<uint32_t*>(&this->m_shadowColor)) {
        this->m_shadowColor = color;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: 283d01b1 auto
// OFFSET: 0x00487190
void CSimpleFontString::SetJustifyH(uint8_t justify) {
    this->m_settableStyleFlags &= ~0x7u;
    uint32_t styleFlags = (this->m_styleFlags & ~0x7u) | (justify & 0x7);

    if (styleFlags != this->m_styleFlags) {
        this->m_styleFlags = styleFlags;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: d976f70d auto
// OFFSET: 0x004871D0
void CSimpleFontString::SetJustifyV(uint8_t justify) {
    this->m_settableStyleFlags &= ~0x38u;
    uint32_t styleFlags = (this->m_styleFlags & ~0x38u) | (justify & 0x38);

    if (styleFlags != this->m_styleFlags) {
        this->m_styleFlags = styleFlags;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: e9a340b1 auto
// OFFSET: 0x00487210
void CSimpleFontString::SetNonWordWrap(int32_t noWrap) {
    this->m_settableStyleFlags &= ~0x40u;
    uint32_t styleFlags = noWrap ? this->m_styleFlags | 0x40 : this->m_styleFlags & ~0x40u;

    if (styleFlags != this->m_styleFlags) {
        this->m_styleFlags = styleFlags;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: eb8e3134 auto
// OFFSET: 0x00487250
void CSimpleFontString::SetNonSpaceWrap(int32_t wrap) {
    this->m_settableStyleFlags &= ~0x1000u;
    uint32_t styleFlags = wrap ? this->m_styleFlags | 0x1000 : this->m_styleFlags & ~0x1000u;

    if (styleFlags != this->m_styleFlags) {
        this->m_styleFlags = styleFlags;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: 8f4f99c8 auto
// OFFSET: 0x004872A0
void CSimpleFontString::SetIndented(int32_t indented) {
    this->m_settableStyleFlags &= ~0x20000u;
    uint32_t styleFlags = indented ? this->m_styleFlags | 0x20000 : this->m_styleFlags & ~0x20000u;

    if (styleFlags != this->m_styleFlags) {
        this->m_styleFlags = styleFlags;

        if (this->m_string) {
            this->UpdateString();
        }
    }
}

// VERIFY: 83e312ff manual
// OFFSET: 0x004873E0
void CSimpleFontString::LoadXML(XMLNode* node, CStatus* status) {
    const char* inherits = node->GetAttributeByName("inherits");

    if (inherits && *inherits) {
        CSimpleFont* font = CSimpleFont::GetFont(inherits, 0);

        if (font) {
            if (this->m_maxLines & 0x1000000) {
                this->SetFontObject(font);
            }
        } else {
            const char* taint;
            bool locked;
            XMLNode* inheritNode = FrameXML_AcquireHashNode(inherits, taint, locked);

            if (!inheritNode) {
                status->Add(STATUS_WARNING, "Couldn't find inherited node: %s", inherits);
            } else if (locked) {
                status->Add(STATUS_WARNING, "Recursively inherited node: %s", inherits);
            } else {
                this->LoadXML(inheritNode, status);
                FrameXML_ReleaseHashNode(inherits);
            }
        }
    }

    this->CScriptRegion::LoadXML(node, status);

    const char* hidden = node->GetAttributeByName("hidden");

    if (hidden && *hidden) {
        if (StringToBOOL(hidden)) {
            this->m_shown = 0;
            this->HideThis();
        } else {
            this->m_shown = 1;
            this->ShowThis();
        }
    }

    const char* textAttr = node->GetAttributeByName("text");

    if (textAttr && *textAttr) {
        const char* text = FrameScript_GetText(textAttr, -1, GENDER_NOT_APPLICABLE);

        if (!text || !*text) {
            text = textAttr;
        }

        this->SetText(text, 1);
    }

    const char* nonSpaceWrap = node->GetAttributeByName("nonspacewrap");

    if (nonSpaceWrap && *nonSpaceWrap) {
        this->SetNonSpaceWrap(StringToBOOL(nonSpaceWrap));
    }

    const char* wordWrap = node->GetAttributeByName("wordwrap");

    if (wordWrap && *wordWrap) {
        this->SetNonWordWrap(StringToBOOL(wordWrap) == 0);
    }

    const char* bytes = node->GetAttributeByName("bytes");

    if (bytes && *bytes) {
        this->SetTextLength(SStrToInt(bytes));
    }

    const char* maxLines = node->GetAttributeByName("maxLines");

    if (maxLines && *maxLines) {
        this->m_maxLines = (this->m_maxLines & ~0xFFFFFFu) | (SStrToUnsigned(maxLines) & 0xFFFFFF);
    }

    if (!(this->m_maxLines & 0x1000000)) {
        return;
    }

    const char* fontName = node->GetAttributeByName("font");

    if (fontName && *fontName) {
        CSimpleFont* font = CSimpleFont::GetFont(fontName, 0);

        if (font) {
            this->SetFontObject(font);
        } else {
            float fontHeight = 0.0f;
            XMLNode* heightNode = node->GetChildByName("FontHeight");

            if (heightNode) {
                LoadXML_Value(heightNode, fontHeight, status);
            }

            if (fontHeight <= 0.00000011920929f) {
                status->Add(STATUS_WARNING, "FontString %s: invalid font height %f in %s element", this->GetDisplayName(), fontHeight, node->m_name.m_str);
                return;
            }

            uint32_t fontFlags = 0;
            const char* outline = node->GetAttributeByName("outline");

            if (outline && *outline) {
                if (!SStrCmpI(outline, "NORMAL", STORM_MAX_STR)) {
                    fontFlags = 1;
                } else if (!SStrCmpI(outline, "THICK", STORM_MAX_STR)) {
                    fontFlags = 5;
                }
            }

            const char* monochrome = node->GetAttributeByName("monochrome");

            if (monochrome && *monochrome && StringToBOOL(monochrome)) {
                fontFlags |= 2;
            }

            if (this->SetFont(fontName, fontHeight, fontFlags, false)) {
                this->m_fontableFlags &= ~0x1u;
            } else {
                status->Add(STATUS_WARNING, "FontString %s: Unable to load font file %s", this->GetDisplayName(), fontName);
            }
        }
    }

    const char* spacing = node->GetAttributeByName("spacing");

    if (spacing && *spacing) {
        float value = SStrToFloat(spacing);
        this->SetSpacing(NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f)));
        this->m_fontableFlags &= ~0x10u;
    }

    uint32_t justify;
    const char* justifyV = node->GetAttributeByName("justifyV");

    if (justifyV && *justifyV && StringToJustify(justifyV, justify)) {
        this->SetJustifyV(justify);
        this->m_fontableFlags &= ~0x2u;
    }

    const char* justifyH = node->GetAttributeByName("justifyH");

    if (justifyH && *justifyH && StringToJustify(justifyH, justify)) {
        this->SetJustifyH(justify);
        this->m_fontableFlags &= ~0x2u;
    }

    const char* indented = node->GetAttributeByName("indented");

    if (indented && *indented) {
        this->SetIndented(StringToBOOL(indented));
    }

    const char* alphaAttr = node->GetAttributeByName("alpha");

    if (alphaAttr && *alphaAttr) {
        float alpha = SStrToFloat(alphaAttr);

        if (alpha < 0.0f) {
            alpha = 0.0f;
        } else if (alpha >= 1.0f) {
            alpha = 1.0f;
        }

        this->SetVertexAlpha(static_cast<uint8_t>(alpha * 255.0f));
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Color", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->SetVertexColor(color);
            this->m_fontableFlags &= ~0x4u;
        } else if (!SStrCmpI(child->m_name.m_str, "Shadow", STORM_MAX_STR)) {
            float offsetX = 0.001f;
            float offsetY = -0.001f;
            CImVector color;
            color.a = 0xFF;

            XMLNode* colorNode = child->GetChildByName("Color");

            if (colorNode) {
                LoadXML_Color(colorNode, color, status);
            }

            XMLNode* offsetNode = child->GetChildByName("Offset");

            if (offsetNode) {
                LoadXML_Dimensions(offsetNode, offsetX, offsetY, status);
            }

            C2Vector offset;
            offset.x = offsetX;
            offset.y = offsetY;
            this->AddShadow(color, offset);
            this->m_fontableFlags &= ~0x8u;
        }
    }
}

// VERIFY: 744f3b3c auto
// OFFSET: 0x00487990
void CSimpleFontString::FontObjectUpdated(CSimpleFontStringAttributes& attributes) {
    attributes.Update(this, this->m_fontableFlags);
}

// INLINED
int32_t CSimpleTexture::GetObjectType() {
    if (!CSimpleTexture::s_objectType) {
        CSimpleTexture::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleTexture::s_objectType;
}

// INLINED
void CSimpleTexture::operator delete(void* ptr) {
    if (ptr) {
        CSimpleTexture::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 0a77af56 auto
// OFFSET: 0x00481520
int32_t CSimpleTexture::SetTexture(CTexture* texture) {
    if (!this->m_texture || !texture || HandleCompare(reinterpret_cast<HOBJECT>(this->m_texture), reinterpret_cast<HOBJECT>(texture))) {
        CTexture* newTexture = nullptr;

        if (texture) {
            newTexture = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture)));
        }

        if (this->m_texture) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
        }

        this->m_texture = newTexture;
        this->OnRegionChanged();
    }

    return 1;
}

// VERIFY: 66dd3b98 auto
// OFFSET: 0x00481590
int32_t CSimpleTexture::SetTexture(const CImVector& color) {
    CTexture* texture = TextureCreateSolid(color);

    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    this->m_texture = texture;
    this->OnRegionChanged();

    return 1;
}

// VERIFY: a001c950 auto
// OFFSET: 0x004815E0
int32_t CSimpleTexture::GetTexture(char* buffer, uint32_t bufferLen) {
    if (!this->m_texture) {
        return 0;
    }

    const char* filename = TextureGetFilename(this->m_texture);

    if (!filename || !*filename) {
        return 0;
    }

    SStrCopy(buffer, filename, bufferLen);
    return 1;
}

// VERIFY: 289faf79 auto
// OFFSET: 0x00481620
void CSimpleTexture::SetBlendMode(EGxBlend blend) {
    if (blend != this->m_alphaMode) {
        this->m_alphaMode = blend;
        this->OnRegionChanged();
    }
}

// VERIFY: ced9d67c auto
// OFFSET: 0x00481640
void CSimpleTexture::SetTexCoord(const C2Vector* texCoords) {
    this->m_texCoords[0] = texCoords[0];
    this->m_texCoords[1] = texCoords[1];
    this->m_texCoords[2] = texCoords[2];
    this->m_texCoords[3] = texCoords[3];
}

// VERIFY: d3659815 auto
// OFFSET: 0x004816A0
void CSimpleTexture::GetTexCoord(C2Vector* texCoords) {
    texCoords[0] = this->m_texCoords[0];
    texCoords[1] = this->m_texCoords[1];
    texCoords[2] = this->m_texCoords[2];
    texCoords[3] = this->m_texCoords[3];
}

// VERIFY: 7b313c73 auto
// OFFSET: 0x00481700
void CSimpleTexture::SetShader(CGxShader* shader) {
    if (shader != this->m_shader) {
        this->m_shader = shader;
        this->OnRegionChanged();
    }
}

// VERIFY: 1db02c26 auto
// OFFSET: 0x00481720
CGxTex* CSimpleTexture::GetGxTex() {
    return this->m_texture ? TextureGetGxTex(this->m_texture, 1, nullptr) : nullptr;
}

// VERIFY: 09cfc6ac auto
// OFFSET: 0x00481740
void CSimpleTexture::AddAnimTranslation(const C2Vector& translation) {
    ApplyTranslationAnim(this->m_positions, translation);
    this->OnRegionChanged();
}

// VERIFY: 0283fe37 auto
// OFFSET: 0x00481770
void CSimpleTexture::AddAnimRotation(FRAMEPOINT origin, const C2Vector& offset, float radians) {
    ApplyRotationAnim(this->m_positions, origin, offset, radians);
    this->OnRegionChanged();
}

// VERIFY: 2b060cdb auto
// OFFSET: 0x004817A0
void CSimpleTexture::AddAnimScale(FRAMEPOINT origin, const C2Vector& offset, const C2Vector& scale) {
    ApplyScaleAnim(this->m_positions, origin, offset, scale);
    this->OnRegionChanged();
}

// VERIFY: efe353c3 auto
// OFFSET: 0x004817D0
float CSimpleTexture::GetWidth() {
    float width = this->CLayoutFrame::GetWidth();

    if (width == 0.0f && this->m_texture) {
        uint32_t textureWidth;

        if (TextureGetDimensions(this->m_texture, &textureWidth, nullptr, 0)) {
            return NDCToDDCWidth(static_cast<float>(textureWidth) / (CoordinateGetAspectCompensation() * 1024.0f));
        }
    }

    return width;
}

// VERIFY: 9aa1bf92 auto
// OFFSET: 0x00481850
float CSimpleTexture::GetHeight() {
    float height = this->CLayoutFrame::GetHeight();

    if (height == 0.0f && this->m_texture) {
        uint32_t textureHeight;

        if (TextureGetDimensions(this->m_texture, nullptr, &textureHeight, 0)) {
            return NDCToDDCWidth(static_cast<float>(textureHeight) / (CoordinateGetAspectCompensation() * 1024.0f));
        }
    }

    return height;
}

// VERIFY: d5f393df auto
// OFFSET: 0x004818D0
int32_t CSimpleTexture::IsObjectLoaded() {
    if (this->m_texture && !TextureIsLoaded(this->m_texture)) {
        return 0;
    }

    return 1;
}

// VERIFY: bced6855 auto
// OFFSET: 0x00482C60
bool CSimpleTexture::IsA(int32_t type) {
    return type == CSimpleTexture::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 3165fe90 auto
// OFFSET: 0x00482C90
bool CSimpleTexture::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleTexture::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptRegion::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptObject::s_objectTypeName, STORM_MAX_STR);
}

// VERIFY: 465f6c7c auto
// OFFSET: 0x00482CF0
const char* CSimpleTexture::GetObjectTypeName() {
    return CSimpleTexture::s_objectTypeName;
}

// VERIFY: 80d103be auto
// OFFSET: 0x00482D00
int32_t CSimpleTexture::GetScriptMetaTable() {
    return CSimpleTexture::s_metatable;
}

// VERIFY: ec4a9c07 auto
// OFFSET: [0x00482860, 0x00483010]
CSimpleTexture::~CSimpleTexture() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    CSimpleTexture::s_count--;
}

// VERIFY: d7f68080 auto
// OFFSET: 0x00483060
void CSimpleTexture::Init() {
    for (uint32_t i = 0; i < 2; i++) {
        if (CSimpleTexture::s_imageModePixelShaderNames[i]) {
            g_theGxDevicePtr->ShaderCreate(&CSimpleTexture::s_imageModePixelShaders[i], GxSh_Pixel, "Shaders\\Pixel", CSimpleTexture::s_imageModePixelShaderNames[i], 1);
        }
    }
}

// VERIFY: 61684332 manual
// OFFSET: 0x004830A0
void CSimpleTexture::Destroy() {
    for (int32_t i = 0; i < 2; i++) {
        if (CSimpleTexture::s_imageModePixelShaders[i]) {
            g_theGxDevicePtr->ShaderDestroy(CSimpleTexture::s_imageModePixelShaders[i]);
        }
    }
}

// VERIFY: 3a72f2dc manual
// OFFSET: 0x004830E0
void CSimpleTexture::PostLoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame* parent = this->m_parent;

    if (!parent) {
        return;
    }

    for (uint32_t i = 0; i < FRAMEPOINT_NUMPOINTS; i++) {
        if (this->m_framePoints[i] && !(this->m_framePoints[i]->m_flags & 0x8)) {
            return;
        }
    }

    this->SetAllPoints(parent, 1);
}

// VERIFY: 4323d6db manual
// OFFSET: 0x00483120
void CSimpleTexture::SetRotation(float radians, float centerX, float centerY) {
    float angle = radians - 0.78539819f;
    float s = sin(angle);
    float c = cos(angle);

    this->m_texCoords[0].x = s + centerX;
    this->m_texCoords[0].y = centerY - c;
    this->m_texCoords[1].x = centerX - c;
    this->m_texCoords[1].y = centerY - s;
    this->m_texCoords[2].x = c + centerX;
    this->m_texCoords[2].y = s + centerY;
    this->m_texCoords[3].x = centerX - s;
    this->m_texCoords[3].y = centerY + c;
}

// VERIFY: 8094d28a auto
// OFFSET: 0x004831D0
void CSimpleTexture::SetTexCoord(const CRect& texRect) {
    this->m_texCoords[0].x = texRect.minX;
    this->m_texCoords[0].y = texRect.minY;
    this->m_texCoords[1].x = texRect.minX;
    this->m_texCoords[1].y = texRect.maxY;
    this->m_texCoords[2].x = texRect.maxX;
    this->m_texCoords[2].y = texRect.minY;
    this->m_texCoords[3].x = texRect.maxX;
    this->m_texCoords[3].y = texRect.maxY;
}

// VERIFY: 5a6638c2 auto
// OFFSET: 0x00483220
void CSimpleTexture::SetPosition(const CRect& rect, C3Vector* position) {
    position[0].x = rect.minX;
    position[0].y = rect.maxY;
    position[0].z = this->m_layoutDepth;
    position[1].x = rect.minX;
    position[1].y = rect.minY;
    position[1].z = this->m_layoutDepth;
    position[2].x = rect.maxX;
    position[2].y = rect.maxY;
    position[2].z = this->m_layoutDepth;
    position[3].x = rect.maxX;
    position[3].y = rect.minY;
    position[3].z = this->m_layoutDepth;
}

// VERIFY: da5e4d9e auto
// OFFSET: 0x00483290
CGxShader* CSimpleTexture::GetImageModePixelShader(TextureImageMode mode) {
    if (CSimpleTexture::s_imageModePixelShaders[mode] && CSimpleTexture::s_imageModePixelShaders[mode]->Valid()) {
        return CSimpleTexture::s_imageModePixelShaders[mode];
    }

    return nullptr;
}

// VERIFY: 99978bcf manual
// OFFSET: 0x004832C0
void CSimpleTexture::OnFrameSizeChanged(const CRect& rect) {
    this->CLayoutFrame::OnFrameSizeChanged(rect);

    if (this->m_animating) {
        C3Vector positions[4];
        memcpy(positions, this->m_storedPositions, sizeof(positions));
        this->SetPosition(this->m_rect, this->m_storedPositions);

        this->m_positions[0].x += this->m_storedPositions[0].x - positions[0].x;
        this->m_positions[0].y += this->m_storedPositions[0].y - positions[0].y;
        this->m_positions[0].z += this->m_storedPositions[0].z - positions[0].z;
        this->m_positions[1].x += this->m_storedPositions[1].x - positions[1].x;
        this->m_positions[1].y += this->m_storedPositions[1].y - positions[1].y;
        this->m_positions[1].z += this->m_storedPositions[1].z - positions[1].z;
        this->m_positions[2].x += this->m_storedPositions[2].x - positions[2].x;
        this->m_positions[2].y += this->m_storedPositions[2].y - positions[2].y;
        this->m_positions[2].z += this->m_storedPositions[2].z - positions[2].z;
        this->m_positions[3].x += this->m_storedPositions[3].x - positions[3].x;
        this->m_positions[3].y += this->m_storedPositions[3].y - positions[3].y;
        this->m_positions[3].z += this->m_storedPositions[3].z - positions[3].z;
    } else {
        this->SetPosition(this->m_rect, this->m_positions);
    }

    if ((this->m_textureFlags & 0x4) || (this->m_textureFlags & 0x8)) {
        this->m_textureFlags |= 0x2;
        this->OnRegionChanged();
    }

    if (this->m_parent) {
        this->m_parent->NotifyScrollParent();
    }
}

// VERIFY: a9647663 auto
// OFFSET: 0x00483460
void CSimpleTexture::PreOnAnimUpdate() {
    if (this->m_animating) {
        memcpy(this->m_positions, this->m_storedPositions, sizeof(this->m_positions));

        if (this->m_animAlpha) {
            uint8_t alpha = this->m_animSavedAlpha;
            CImVector color;
            this->GetVertexColor(color);
            color.a = alpha;
            this->SetVertexColor(color);
        }
    }
}

// VERIFY: e87a4320 manual
// OFFSET: 0x00484470
CSimpleTexture::CSimpleTexture(CSimpleFrame* frame, uint32_t drawLayer, int32_t show) : CSimpleRegion(frame, drawLayer, show) {
    this->m_texture = nullptr;
    this->m_alphaMode = GxBlend_Alpha;
    this->m_shader = CSimpleTexture::s_imageModePixelShaders[ImageMode_Normal];
    this->m_textureFlags = 0;
    this->m_shader = CSimpleTexture::GetImageModePixelShader(ImageMode_Normal);
    CSimpleTexture::s_count++;
    this->m_texCoords[0].x = 0.0f;
    this->m_texCoords[0].y = 0.0f;
    this->m_texCoords[1].x = 0.0f;
    this->m_texCoords[1].y = 1.0f;
    this->m_texCoords[2].x = 1.0f;
    this->m_texCoords[2].y = 0.0f;
    this->m_texCoords[3].x = 1.0f;
    this->m_texCoords[3].y = 1.0f;
}

// VERIFY: 9d63630a manual
// OFFSET: 0x004859E0
int32_t CSimpleTexture::SetTexture(const char* fileName, int32_t wrapU, int32_t wrapV, EGxTexFilter filter, TextureImageMode mode) {
    if (this->m_texture && fileName && TextureIsSame(this->m_texture, fileName)) {
        return 1;
    }

    CTexture* texture = nullptr;

    if (fileName && *fileName) {
        CStatus status;
        texture = TextureCreate(fileName, CGxTexFlags(filter, wrapU, wrapV, 0, 0, 0, 1), &status, 0x17);

        if (status.m_highestSeverity >= STATUS_ERROR) {
            HandleClose(reinterpret_cast<HOBJECT>(texture));
            SysMsgAdd(&status);
            return 0;
        }

        this->m_shader = CSimpleTexture::GetImageModePixelShader(mode);
    }

    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    this->m_texture = texture;
    this->OnRegionChanged();

    return 1;
}

// VERIFY: f1967c67 auto
// OFFSET: 0x00485AF0
void CSimpleEmbeddedTexture::InitEmbeddedTexture(CSimpleFontString* string, const GXUEMBEDDEDTEXTUREINFO* info) {
    if (string && info) {
        if (!this->m_texture) {
            return;
        }

        char path[256];
        SStrCopy(path, info->m_path, info->m_pathLength + 1);

        if (this->m_texture->SetTexture(path, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
            float top = 0.0f;
            float left = 0.0f;
            float baseLine = 0.0f;
            float right = 0.0f;
            NDCToDDC(info->m_left, info->m_baseLineHeight, &left, &baseLine);
            NDCToDDC(info->m_right, info->m_lineHeight, &right, &top);

            float offsetX = NDCToDDCWidth(info->m_offsetX);
            float offsetY = NDCToDDCHeight(info->m_offsetY);
            float width = NDCToDDCWidth(info->m_width);
            float height = NDCToDDCHeight(info->m_height);

            uint32_t styleFlags = string->m_styleFlags;
            float x = 0.0f;
            float y = 0.0f;
            uint32_t point = FRAMEPOINT_TOPLEFT;

            switch (styleFlags & 0x7) {
                case 0x1:
                    x = left + offsetX;
                    break;

                case 0x2:
                    point = FRAMEPOINT_TOP;
                    x = (right + left) * 0.5f + offsetX;
                    break;

                case 0x4:
                    point = FRAMEPOINT_TOPRIGHT;
                    x = right + offsetX;
                    break;
            }

            switch (styleFlags & 0x38) {
                case 0x8:
                    y = top + offsetY;
                    break;

                case 0x10:
                    point += 3;
                    y = (string->GetStringHeight() * string->m_layoutScale + height) * 0.5f + baseLine + offsetY;
                    break;

                case 0x20:
                    point += 6;
                    y = string->GetStringHeight() + baseLine + offsetY;
                    break;
            }

            this->m_texture->MarkFramePointsUnused();
            this->m_texture->SetPoint(static_cast<FRAMEPOINT>(point), string, static_cast<FRAMEPOINT>(point), x, y, 1);
            this->m_texture->SetWidth(width);
            this->m_texture->SetHeight(height);
            this->m_texture->SetVertexAlpha(string->GetVertexAlpha());

            if (info->m_hasTexCoords) {
                this->m_texture->SetTexCoord(info->m_texCoords);
            } else {
                CRect texRect = { 0.0f, 0.0f, 1.0f, 1.0f };
                this->m_texture->SetTexCoord(texRect);
            }

            this->m_texture->m_shown = 1;
            this->m_texture->ShowThis();
            return;
        }
    }

    if (this->m_texture) {
        this->m_texture->m_shown = 0;
        this->m_texture->HideThis();
    }
}

// VERIFY: 8453c584 manual
// OFFSET: 0x00485F40
void CSimpleTexture::LoadXML(XMLNode* node, CStatus* status) {
    const char* inherits = node->GetAttributeByName("inherits");

    if (inherits && *inherits) {
        const char* taint;
        bool locked;
        XMLNode* inheritNode = FrameXML_AcquireHashNode(inherits, taint, locked);

        if (!inheritNode) {
            status->Add(STATUS_WARNING, "Couldn't find inherited node: %s", inherits);
        } else if (locked) {
            status->Add(STATUS_WARNING, "Recursively inherited node: %s", inherits);
        } else {
            this->LoadXML(inheritNode, status);
            FrameXML_ReleaseHashNode(inherits);
        }
    }

    this->CScriptRegion::LoadXML(node, status);

    CImVector color;
    int32_t wrapU = 0;
    int32_t wrapV = 0;

    const char* hidden = node->GetAttributeByName("hidden");

    if (hidden && *hidden) {
        if (StringToBOOL(hidden)) {
            this->m_shown = 0;
            this->HideThis();
        } else {
            this->m_shown = 1;
            this->ShowThis();
        }
    }

    const char* horizTile = node->GetAttributeByName("horizTile");

    if (horizTile && *horizTile) {
        this->m_textureFlags ^= (((StringToBOOL(horizTile) != 0) << 2) ^ this->m_textureFlags) & 0x4;
        wrapU = 1;
    }

    const char* vertTile = node->GetAttributeByName("vertTile");

    if (vertTile && *vertTile) {
        this->m_textureFlags ^= (((StringToBOOL(vertTile) != 0) << 3) ^ this->m_textureFlags) & 0x8;
        wrapV = 1;
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "TexCoords", STORM_MAX_STR)) {
            float ulx = 0.0f;
            float uly = 0.0f;
            float llx = 0.0f;
            float lly = 1.0f;
            float urx = 1.0f;
            float ury = 0.0f;
            float lrx = 1.0f;
            float lry = 1.0f;
            int32_t valid = 1;

            XMLNode* rectNode = child->GetChildByName("Rect");

            if (rectNode) {
                const char* value = rectNode->GetAttributeByName("ULx");

                if (value && *value) {
                    ulx = SStrToFloat(value);

                    if (ulx < -10000.0f || ulx > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("ULy");

                if (value && *value) {
                    uly = SStrToFloat(value);

                    if (uly < -10000.0f || uly > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("LLx");

                if (value && *value) {
                    llx = SStrToFloat(value);

                    if (llx < -10000.0f || llx > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("LLy");

                if (value && *value) {
                    lly = SStrToFloat(value);

                    if (lly < -10000.0f || lly > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("URx");

                if (value && *value) {
                    urx = SStrToFloat(value);

                    if (urx < -10000.0f || urx > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("URy");

                if (value && *value) {
                    ury = SStrToFloat(value);

                    if (ury < -10000.0f || ury > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("LRx");

                if (value && *value) {
                    lrx = SStrToFloat(value);

                    if (lrx < -10000.0f || lrx > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }

                value = rectNode->GetAttributeByName("LRy");

                if (value && *value) {
                    lry = SStrToFloat(value);

                    if (lry < -10000.0f || lry > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid rect value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }
            }

            const char* left = child->GetAttributeByName("left");

            if (left && *left) {
                if (this->m_textureFlags & 0x4) {
                    const char* name = this->GetName();
                    status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (horizTile is on)", name ? name : "<unnamed>");
                } else {
                    llx = SStrToFloat(left);
                    ulx = llx;

                    if (llx < -10000.0f || llx > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }
            }

            const char* right = child->GetAttributeByName("right");

            if (right && *right) {
                if (this->m_textureFlags & 0x4) {
                    const char* name = this->GetName();
                    status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (horizTile is on)", name ? name : "<unnamed>");
                } else {
                    lrx = SStrToFloat(right);
                    urx = lrx;

                    if (lrx < -10000.0f || lrx > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }
            }

            const char* top = child->GetAttributeByName("top");

            if (top && *top) {
                if (this->m_textureFlags & 0x8) {
                    const char* name = this->GetName();
                    status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (vertTile is on)", name ? name : "<unnamed>");
                } else {
                    ury = SStrToFloat(top);
                    uly = ury;

                    if (ury < -10000.0f || ury > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (out of range)", name ? name : "<unnamed>");
                        valid = 0;
                    }
                }
            }

            const char* bottom = child->GetAttributeByName("bottom");

            if (bottom && *bottom) {
                if (this->m_textureFlags & 0x8) {
                    const char* name = this->GetName();
                    status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (vertTile is on)", name ? name : "<unnamed>");
                } else {
                    lly = SStrToFloat(bottom);
                    lry = lly;

                    if (lly < -10000.0f || lly > 10000.0f) {
                        const char* name = this->GetName();
                        status->Add(STATUS_ERROR, "Texture %s: Invalid TexCoords value (out of range)", name ? name : "<unnamed>");
                        continue;
                    }
                }
            }

            if (valid) {
                this->m_texCoords[0].x = ulx;
                this->m_texCoords[0].y = uly;
                this->m_texCoords[1].x = llx;
                this->m_texCoords[1].y = lly;
                this->m_texCoords[2].x = urx;
                this->m_texCoords[2].y = ury;
                this->m_texCoords[3].x = lrx;
                this->m_texCoords[3].y = lry;

                if (ulx < 0.0f || ulx > 1.0f || llx < 0.0f || llx > 1.0f || urx < 0.0f || urx > 1.0f || lrx < 0.0f || lrx > 1.0f) {
                    wrapU = 1;
                }

                if (uly < 0.0f || uly > 1.0f || lly < 0.0f || lly > 1.0f || ury < 0.0f || ury > 1.0f || lry < 0.0f || lry > 1.0f) {
                    wrapV = 1;
                }
            }
        } else if (!SStrCmpI(child->m_name.m_str, "Color", STORM_MAX_STR)) {
            LoadXML_Color(child, color, status);
            this->SetTexture(color);
        } else if (!SStrCmpI(child->m_name.m_str, "Gradient", STORM_MAX_STR)) {
            ORIENTATION orientation;
            CImVector minColor;
            CImVector maxColor;
            LoadXML_Gradient(child, orientation, minColor, maxColor, status);
            this->SetVertexGradient(orientation, minColor, maxColor);
        }
    }

    const char* file = node->GetAttributeByName("file");

    if (file && *file) {
        if (this->SetTexture(file, wrapU, wrapV, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
            if (*reinterpret_cast<uint32_t*>(&color)) {
                this->SetVertexColor(color);
            }
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Texture %s: Unable to load texture file %s", name ? name : "<unnamed>", file);

            CImVector missingColor;
            missingColor.b = 0x00;
            missingColor.g = 0xFF;
            missingColor.r = 0x00;
            missingColor.a = 0xFF;
            this->SetTexture(missingColor);
        }
    }

    const char* alphaMode = node->GetAttributeByName("alphaMode");
    EGxBlend blend;

    if (alphaMode && *alphaMode && StringToBlendMode(alphaMode, blend)) {
        this->SetBlendMode(blend);
    }

    const char* alpha = node->GetAttributeByName("alpha");

    if (alpha && *alpha) {
        float value = SStrToFloat(alpha);
        value = value < 0.0f ? 0.0f : (value >= 1.0f ? 1.0f : value);

        CImVector vertexColor;
        this->GetVertexColor(vertexColor);
        vertexColor.a = static_cast<uint8_t>(value * 255.0f);
        this->SetVertexColor(vertexColor);
    }

    const char* nonBlocking = node->GetAttributeByName("nonBlocking");

    if (nonBlocking && *nonBlocking) {
        this->m_textureFlags ^= ((StringToBOOL(nonBlocking) != 0) ^ this->m_textureFlags) & 0x1;
    }
}
