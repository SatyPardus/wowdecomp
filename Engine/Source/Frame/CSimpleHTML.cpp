#include <cmath>
#include <cstddef>
#include <cstdint>
#if defined(__APPLE__)
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleHTML.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFont.h"
#include "Engine/Source/Frame/CSimpleFontedFrame.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// STATIC: 0x00B2D208
int32_t CSimpleHTML::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE4EC
int32_t CSimpleHTML::s_objectType;

// STATIC: 0x00B2CCF8
const char* CSimpleHTML::s_objectTypeName = "SimpleHTML";

// STATIC: 0x00DCE4F0
CDataAllocator CSimpleHTML::s_allocator(sizeof(CSimpleHTML), 5);

// INLINED
void CSimpleHTML::operator delete(void* ptr) {
    if (ptr) {
        CSimpleHTML::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleHTML::GetObjectType() {
    if (!CSimpleHTML::s_objectType) {
        CSimpleHTML::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleHTML::s_objectType;
}

// VERIFY: adeee239 auto
// OFFSET: 0x0096C750
void CSimpleHTML::FontUpdated(CSimpleFontedFrameFont* font, int32_t flags) {
    if (flags & 0x11) {
        this->m_hyperlinksDirty = 1;
    }
}

// VERIFY: d15ae246 manual
// OFFSET: 0x0096C770
void CSimpleHTML::SetHyperlinkFormat(const char* format) {
    if (this->m_hyperlinkFormat) {
        SMemFree(this->m_hyperlinkFormat, __FILE__, __LINE__, 0);
    }

    this->m_hyperlinkFormat = SStrDupA(format, __FILE__, __LINE__);
}

// VERIFY: b1ee01c6 auto
// OFFSET: 0x0096C7B0
void CSimpleHTML::OnFrameSizeChanged(const CRect& rect) {
    CSimpleFrame::OnFrameSizeChanged(rect);

    if (!(fabsf((rect.maxX - rect.minX) - (this->m_rect.maxX - this->m_rect.minX)) < 0.0000099999997f) || !(fabsf((rect.maxY - rect.minY) - (this->m_rect.maxY - this->m_rect.minY)) < 0.0000099999997f)) {
        this->m_hyperlinksDirty = 1;
    }
}

// VERIFY: 012bb8ae manual
// OFFSET: 0x0096C820
CSimpleHTML::CSimpleHTML(CSimpleFrame* parent) : CSimpleHyperlinkedFrame(parent) {
    for (int32_t i = 0; i < NUM_HTML_TEXT_TYPES; i++) {
        void* data = SMemAlloc(sizeof(CSimpleFontedFrameFont), __FILE__, __LINE__, 0);
        this->m_fonts[i] = data ? new (data) CSimpleFontedFrameFont(this) : nullptr;
    }

    this->m_hyperlinkFormat = nullptr;
    this->SetHyperlinkFormat("|H%s|h%s|h");
    this->m_hyperlinksDirty = 0;
    this->m_hyperlinksEnabled = true;
}

// VERIFY: 0a33d401 auto
// OFFSET: 0x0096C930
const char* CSimpleHTML::GetObjectTypeName() {
    return CSimpleHTML::s_objectTypeName;
}

// VERIFY: 8ccd312e auto
// OFFSET: 0x0096C940
int32_t CSimpleHTML::GetScriptMetaTable() {
    return CSimpleHTML::s_metatable;
}

// VERIFY: 7cbd791b auto
// OFFSET: 0x0096C960
bool CSimpleHTML::IsA(int32_t type) {
    return type == CSimpleHTML::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 5016a1a3 auto
// OFFSET: 0x0096C9A0
bool CSimpleHTML::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleHTML::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: ce60c879 manual
// OFFSET: 0x0096C9E0
void CSimpleHTML::ParseIMG(XMLNode* node, CStatus* status) {
    bool floating = false;
    uint32_t justify = 1;
    const char* align = node->GetAttributeByName("align");

    if (align && *align) {
        StringToJustify(align, justify);

        if (justify == 1 || justify == 4) {
            floating = true;
        }
    }

    float width = 0.0f;
    float height = 0.0f;

    const char* widthAttr = node->GetAttributeByName("width");

    if (widthAttr && *widthAttr) {
        width = NDCToDDCWidth(SStrToFloat(widthAttr) / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    const char* heightAttr = node->GetAttributeByName("height");

    if (heightAttr && *heightAttr) {
        height = NDCToDDCWidth(SStrToFloat(heightAttr) / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(this, 2, 1) : nullptr;

    texture->SetWidth(width);
    texture->SetHeight(height);

    if (justify == 1) {
        if (this->m_layoutAnchor) {
            texture->SetPoint(FRAMEPOINT_TOPLEFT, this->m_layoutAnchor, FRAMEPOINT_BOTTOMLEFT, 0.0f, this->m_layoutOffset, 1);
        } else {
            texture->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
        }
    } else if (justify == 2) {
        if (this->m_layoutAnchor) {
            texture->SetPoint(FRAMEPOINT_TOP, this->m_layoutAnchor, FRAMEPOINT_BOTTOM, 0.0f, this->m_layoutOffset, 1);
        } else {
            texture->SetPoint(FRAMEPOINT_TOP, this, FRAMEPOINT_TOP, 0.0f, 0.0f, 1);
        }
    } else if (justify == 4) {
        if (this->m_layoutAnchor) {
            texture->SetPoint(FRAMEPOINT_TOPRIGHT, this->m_layoutAnchor, FRAMEPOINT_BOTTOMRIGHT, 0.0f, this->m_layoutOffset, 1);
        } else {
            texture->SetPoint(FRAMEPOINT_TOPRIGHT, this, FRAMEPOINT_TOPRIGHT, 0.0f, 0.0f, 1);
        }
    }

    texture->SetTexture(node->GetAttributeByName("src"), 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);

    if (!floating) {
        this->m_layoutOffset -= texture->GetHeight();
    }

    CONTENTNODE* content = this->m_content.NewNode(2, 0, 0);
    content->m_type = 2;
    content->m_region = texture;
}

// VERIFY: bbbb23ac manual
// OFFSET: 0x0096CC90
void CSimpleHTML::AddText(const char* text, HTML_TEXT_TYPE type, uint32_t justify) {
    void* data = CSimpleFontString::s_allocator.GetData(0, StormTypeName(typeid(CSimpleFontString)), SLOG_OBJECT);
    CSimpleFontString* string = data ? new (data) CSimpleFontString(this, 2, 1) : nullptr;

    if (this->m_layoutAnchor) {
        string->SetPoint(FRAMEPOINT_TOPLEFT, this->m_layoutAnchor, FRAMEPOINT_BOTTOMLEFT, 0.0f, this->m_layoutOffset, 1);
    } else {
        string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
    }

    string->SetWidth(this->GetWidth());

    CSimpleFont* font = this->m_fonts[type];

    if (!font->m_attributes.m_font.m_str) {
        font = this->m_fonts[HTML_TEXT_NORMAL];
    }

    string->SetFontObject(font);
    string->SetJustifyH(justify);
    string->SetNonSpaceWrap(1);
    string->m_fontableFlags &= ~0x2u;
    string->SetText(text, 1);

    this->m_layoutAnchor = string;
    this->m_layoutOffset = -CSimpleTop::RoundToPixelHeight(string->m_spacing);

    CONTENTNODE* content = this->m_content.NewNode(2, 0, 0);
    content->m_region = string;
    content->m_type = SStrStr(text, "|H") != nullptr;
}

// VERIFY: 31268dcd auto
// OFFSET: [0x0096C950, 0x0096CF00, 0x0096D6D0]
CSimpleHTML::~CSimpleHTML() {
    for (int32_t i = 0; i < NUM_HTML_TEXT_TYPES; i++) {
        if (this->m_fonts[i]) {
            delete this->m_fonts[i];
        }
    }

    CONTENTNODE* content = this->m_content.Head();

    while (!TSList<CONTENTNODE, TSGetLink<CONTENTNODE> >::IsEnd(content)) {
        content = this->m_content.DeleteNode(content);
    }

    CSimpleHyperlinkButton* button = this->m_hyperlinks.Head();

    while (!TSList<CSimpleHyperlinkButton, TSGetExplicitLink<CSimpleHyperlinkButton> >::IsEnd(button)) {
        CSimpleHyperlinkButton* next = this->m_hyperlinks.RawNext(button);
        this->m_hyperlinks.UnlinkNode(button);
        delete button;
        button = next;
    }

    SMemFree(this->m_hyperlinkFormat, __FILE__, __LINE__, 0);
}

// VERIFY: 684e4381 manual
// OFFSET: 0x0096D090
void CSimpleHTML::ClearContent() {
    CONTENTNODE* content = this->m_content.Head();

    while (!TSList<CONTENTNODE, TSGetLink<CONTENTNODE> >::IsEnd(content)) {
        if (content->m_region) {
            delete content->m_region;
        }

        content = this->m_content.DeleteNode(content);
    }

    this->m_layoutAnchor = nullptr;
    this->m_layoutOffset = 0.0f;

    CSimpleHyperlinkButton* button = this->m_hyperlinks.Head();

    while (!TSList<CSimpleHyperlinkButton, TSGetExplicitLink<CSimpleHyperlinkButton> >::IsEnd(button)) {
        CSimpleHyperlinkButton* next = this->m_hyperlinks.RawNext(button);
        this->m_hyperlinks.UnlinkNode(button);
        delete button;
        button = next;
    }
}

// VERIFY: 6ff12c29 manual
// OFFSET: 0x0096D170
void CSimpleHTML::ParseP(XMLNode* node, HTML_TEXT_TYPE type, CStatus* status) {
    uint32_t justify = 1;
    const char* align = node->GetAttributeByName("align");

    if (align && *align) {
        StringToJustify(align, justify);
    }

    char* text = SStrDupA(node->m_body ? node->m_body : "", __FILE__, __LINE__);
    uint32_t offset = 0;

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "BR", STORM_MAX_STR)) {
            char* newText = static_cast<char*>(SMemAlloc(SStrLen(text) + 3, __FILE__, __LINE__, 0));
            SStrCopy(newText, text, offset + child->m_offset + 1);
            SStrPack(newText, "|n", STORM_MAX_STR);
            SStrPack(newText, &text[offset + child->m_offset], STORM_MAX_STR);
            SMemFree(text, __FILE__, __LINE__, 0);
            offset += 2;
            text = newText;
        } else if (!SStrCmpI(child->m_name.m_str, "A", STORM_MAX_STR)) {
            const char* linkText = child->m_body;
            const char* href = child->GetAttributeByName("href");

            if (linkText && *linkText && href && *href) {
                uint32_t size = SStrLen(href) + SStrLen(this->m_hyperlinkFormat) + SStrLen(linkText) + 1;
                char* link = static_cast<char*>(alloca(size));
                SStrPrintf(link, size, this->m_hyperlinkFormat, href, linkText);

                char* newText = static_cast<char*>(SMemAlloc(SStrLen(text) + size, __FILE__, __LINE__, 0));
                SStrCopy(newText, text, offset + child->m_offset + 1);
                SStrPack(newText, link, STORM_MAX_STR);
                SStrPack(newText, &text[offset + child->m_offset], STORM_MAX_STR);
                SMemFree(text, __FILE__, __LINE__, 0);
                text = newText;
                offset += SStrLen(link);
            }
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown element type: %s", name ? name : "<unnamed>", child->m_name.m_str);
        }
    }

    bool space = false;
    bool lineStart = true;
    char* src = text;
    char* dst = text;

    while (*src) {
        switch (*src) {
            case '\t':
            case '\n':
            case '\r':
            case ' ':
                if (!lineStart && !space) {
                    space = true;
                    *dst++ = ' ';
                }

                break;

            case '|':
                if (src[1] == 'n') {
                    if (dst > text && space) {
                        dst--;
                    }

                    *dst++ = '|';
                    *dst++ = 'n';
                    space = false;
                    lineStart = true;
                    src++;
                    break;
                }

            default:
                lineStart = false;
                space = false;
                *dst++ = *src;
                break;
        }

        src++;
    }

    if (dst > text && space) {
        dst--;
    }

    *dst = '\0';

    this->AddText(text, type, justify);
    SMemFree(text, __FILE__, __LINE__, 0);
}

// VERIFY: a10a3e21 manual
// OFFSET: 0x0096D500
void CSimpleHTML::RefreshHyperlinks() {
    if (!(this->m_layoutFlags & 0x100)) {
        return;
    }

    CSimpleHyperlinkButton* button = this->m_hyperlinks.Head();

    while (!TSList<CSimpleHyperlinkButton, TSGetExplicitLink<CSimpleHyperlinkButton> >::IsEnd(button)) {
        CSimpleHyperlinkButton* next = this->m_hyperlinks.RawNext(button);
        this->m_hyperlinks.UnlinkNode(button);
        delete button;
        button = next;
    }

    if (!this->m_hyperlinksEnabled) {
        return;
    }

    CONTENTNODE* content = this->m_content.Head();

    while (!TSList<CONTENTNODE, TSGetLink<CONTENTNODE> >::IsEnd(content)) {
        if (content->m_type == 1) {
            CSimpleFontString* string = static_cast<CSimpleFontString*>(content->m_region);
            const GXUFONTHYPERLINKINFO* info;
            uint32_t count = GxuFontStringHyperLinkInfo(string->m_string ? TextBlockGetStringPtr(string->m_string) : nullptr, info);

            for (uint32_t i = 0; i < count; i++) {
                void* data = CSimpleHyperlinkButton::s_allocator.GetData(0, StormTypeName(typeid(CSimpleHyperlinkButton)), SLOG_OBJECT);
                CSimpleHyperlinkButton* link = data ? new (data) CSimpleHyperlinkButton(this) : nullptr;
                this->m_hyperlinks.LinkToTail(link);
                link->SetHyperlink(string, &info[i]);
            }
        }

        content = this->m_content.RawNext(content);
    }
}

// VERIFY: 5bea9ccf auto
// OFFSET: 0x0096D700
void CSimpleHTML::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    if (this->m_hyperlinksDirty) {
        this->RefreshHyperlinks();
        this->m_hyperlinksDirty = 0;
    }
}

// VERIFY: 0a739ef6 auto
// OFFSET: 0x0096D740
void CSimpleHTML::ParseBODY(XMLNode* node, CStatus* status) {
    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "H1", STORM_MAX_STR)) {
            this->ParseP(child, HTML_TEXT_HEADER1, status);
        } else if (!SStrCmpI(child->m_name.m_str, "H2", STORM_MAX_STR)) {
            this->ParseP(child, HTML_TEXT_HEADER2, status);
        } else if (!SStrCmpI(child->m_name.m_str, "H3", STORM_MAX_STR)) {
            this->ParseP(child, HTML_TEXT_HEADER3, status);
        } else if (!SStrCmpI(child->m_name.m_str, "P", STORM_MAX_STR)) {
            this->ParseP(child, HTML_TEXT_NORMAL, status);
        } else if (!SStrCmpI(child->m_name.m_str, "BR", STORM_MAX_STR)) {
            this->AddText("\n", HTML_TEXT_NORMAL, 1);
        } else if (!SStrCmpI(child->m_name.m_str, "IMG", STORM_MAX_STR)) {
            this->ParseIMG(child, status);
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown element type: %s", name ? name : "<unnamed>", child->m_name.m_str);
        }
    }
}

// VERIFY: 6e4dfcbd manual
// OFFSET: 0x0096D890
bool CSimpleHTML::SetText(const char* text, CStatus* status) {
    static CStatus s_nullStatus;

    if (!status) {
        status = &s_nullStatus;
    }

    if (!text) {
        text = "";
    }

    if (this->m_inHyperlinkScript) {
        return false;
    }

    this->ClearContent();

    bool parsed = false;
    XMLTree* tree = XMLTree_Load(text, SStrLen(text));

    if (tree) {
        XMLNode* root = XMLTree_GetRoot(tree);

        if (SStrCmpI(root->m_name.m_str, "HTML", STORM_MAX_STR)) {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown element type: %s (expected HTML)", name ? name : "<unnamed>", root->m_name.m_str);
        } else {
            for (XMLNode* child = root->m_child; child; child = child->m_next) {
                if (!SStrCmpI(child->m_name.m_str, "BODY", STORM_MAX_STR)) {
                    parsed = true;
                    this->ParseBODY(child, status);
                    break;
                }

                const char* name = this->GetName();
                status->Add(STATUS_WARNING, "Frame %s: Unknown element type: %s (expected BODY)", name ? name : "<unnamed>", child->m_name.m_str);
            }
        }

        XMLTree_Free(tree);
    }

    if (!parsed) {
        this->AddText(text, HTML_TEXT_NORMAL, 1);
    }

    this->m_hyperlinksDirty = 1;
    return parsed;
}

// VERIFY: d2405739 manual
// OFFSET: 0x0096DA40
void CSimpleHTML::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    const char* fontName = node->GetAttributeByName("font");

    if (fontName && *fontName) {
        CSimpleFont* font = CSimpleFont::GetFont(fontName, 0);

        if (font) {
            for (int32_t i = 0; i < NUM_HTML_TEXT_TYPES; i++) {
                this->m_fonts[i]->SetFontObject(font);
            }
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "%s %s: Couldn't find font object named %s", this->GetObjectTypeName(), name ? name : "<unnamed>", fontName);
        }
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "FontString", STORM_MAX_STR)) {
            this->m_fonts[HTML_TEXT_NORMAL]->LoadXML(child, status);
        } else if (!SStrCmpI(child->m_name.m_str, "FontStringHeader1", STORM_MAX_STR)) {
            this->m_fonts[HTML_TEXT_HEADER1]->LoadXML(child, status);
        } else if (!SStrCmpI(child->m_name.m_str, "FontStringHeader2", STORM_MAX_STR)) {
            this->m_fonts[HTML_TEXT_HEADER2]->LoadXML(child, status);
        } else if (!SStrCmpI(child->m_name.m_str, "FontStringHeader3", STORM_MAX_STR)) {
            this->m_fonts[HTML_TEXT_HEADER3]->LoadXML(child, status);
        }
    }

    const char* file = node->GetAttributeByName("file");
    void* data;

    if (file && SFile::ReadFile(nullptr, file, &data, nullptr, 1, 1, nullptr)) {
        this->SetText(static_cast<const char*>(data), nullptr);
        SFile::Unload(data);
    }

    const char* hyperlinkFormat = node->GetAttributeByName("hyperlinkFormat");

    if (hyperlinkFormat) {
        this->SetHyperlinkFormat(hyperlinkFormat);
    }
}
