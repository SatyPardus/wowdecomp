#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleFont.h"
#include <cmath>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// STATIC: 0x00B499B4
static TSHashTable<SIMPLEFONT, HASHKEY_STRI> s_fontList;

// STATIC: 0x00B499AC
static int32_t s_fontCount;

// STATIC: 0x00AC18D8
int32_t CSimpleFont::s_metatable = LUA_NOREF;

// STATIC: 0x00B499B0
int32_t CSimpleFont::s_objectType;

// STATIC: 0x00AC1330
const char* CSimpleFont::s_objectTypeName = "Font";

// INLINED
SIMPLEFONT::SIMPLEFONT() {
    this->m_font = new (SMemAlloc(sizeof(CSimpleFont), __FILE__, __LINE__, 0)) CSimpleFont;
}

// INLINED
SIMPLEFONT::~SIMPLEFONT() {
    delete this->m_font;
}

// INLINED
int32_t CSimpleFont::GetObjectType() {
    if (!CSimpleFont::s_objectType) {
        CSimpleFont::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleFont::s_objectType;
}

// VERIFY: a2632992 auto
// OFFSET: 0x004965C0
const char* CSimpleFont::GetName() {
    return this->m_name;
}

// VERIFY: eddc0268 auto
// OFFSET: 0x00496610
bool CSimpleFont::IsA(int32_t type) {
    return type == CSimpleFont::s_objectType;
}

// VERIFY: c1008cf1 manual
// OFFSET: 0x00496630
bool CSimpleFont::IsA(const char* typeName) {
    return SStrCmpI(typeName, CSimpleFont::s_objectTypeName, STORM_MAX_STR) == 0;
}

// VERIFY: 3d0bc1be auto
// OFFSET: 0x00496660
const char* CSimpleFont::GetObjectTypeName() {
    return CSimpleFont::s_objectTypeName;
}

// VERIFY: 4925c45f auto
// OFFSET: 0x00496670
int32_t CSimpleFont::GetScriptMetaTable() {
    return CSimpleFont::s_metatable;
}

// VERIFY: dd9273cb manual
// OFFSET: 0x00496F70
CSimpleFont* CSimpleFont::GetFont(const char* name, int32_t create) {
    SIMPLEFONT* node = s_fontList.Ptr(name);

    if (!node) {
        if (!create) {
            return nullptr;
        }

        node = s_fontList.New(name, 0, 0);

        const char* fontName = node->m_key.m_str;
        CSimpleFont* font = node->m_font;

        if (font->m_name) {
            font->UnregisterScriptObject(font->m_name);
        }

        font->m_name = fontName;

        if (fontName) {
            font->RegisterScriptObject(fontName);
        }
    }

    return node->m_font;
}

// VERIFY: 4e49d100 manual
// OFFSET: 0x00496870
CSimpleFont::CSimpleFont() {
    this->m_name = nullptr;
    this->m_attributeMask = 0;
    s_fontCount++;
}

// VERIFY: afaa75af manual
// OFFSET: [0x004964D0, 0x00963560, 0x009652B0]
CSimpleFont::~CSimpleFont() {
    if (this->m_name) {
        this->UnregisterScriptObject(this->m_name);
    }

    this->m_name = nullptr;

    CSimpleFontable* fontable;

    while ((fontable = this->m_fontableList.Head()) != nullptr) {
        fontable->SetFontObject(nullptr);
    }

    s_fontCount--;
}

// VERIFY: 6f8f4fa0 manual
// OFFSET: 0x004964C0
void CSimpleFont::Destroy() {
    s_fontList.Clear();
}

// VERIFY: a26db561 auto
// OFFSET: 0x00484AB0
void CSimpleFont::RemoveFontable(CSimpleFontable* fontable) {
    this->m_fontableList.UnlinkNode(fontable);
}

// VERIFY: 687fcec6 auto
// OFFSET: 0x004963F0
void CSimpleFont::UpdateObjects() {
    if (!this->m_attributes.m_flags) {
        return;
    }

    for (CSimpleFontable* fontable = this->m_fontableList.Head(); !TSList<CSimpleFontable, TSGetExplicitLink<CSimpleFontable> >::IsEnd(fontable); fontable = this->m_fontableList.RawNext(fontable)) {
        fontable->FontObjectUpdated(this->m_attributes);
    }

    this->m_attributes.m_flags &= ~FLAG_COMPLETE_UPDATE;
}

// VERIFY: de3bab57 manual
// OFFSET: 0x00496440
CSimpleFont& CSimpleFont::operator=(const CSimpleFont& source) {
    this->m_attributeMask = source.m_attributeMask;
    this->m_attributes = source.m_attributes;
    this->m_attributes.m_flags &= this->m_attributeMask;
    this->m_fontableFlags = source.m_fontableFlags;

    CSimpleFont* font = source.m_fontObject;

    for (CSimpleFont* parent = font; parent; parent = parent->m_fontObject) {
        if (parent == this) {
            font = nullptr;
            break;
        }
    }

    if (font != this->m_fontObject) {
        if (this->m_fontObject) {
            this->m_fontObject->RemoveFontable(this);
        }

        this->m_fontObject = font;

        if (font) {
            font->m_fontableList.LinkToTail(this);
        }
    }

    this->UpdateObjects();

    return *this;
}

// VERIFY: 5c6e0d80 auto
// OFFSET: 0x004965D0
void CSimpleFont::FontObjectUpdated(CSimpleFontStringAttributes& attributes) {
    attributes.Update(this->m_attributes, this->m_fontableFlags);
    this->UpdateObjects();
}

// VERIFY: 4695fb39 manual
// OFFSET: 0x00497070
void CSimpleFont::LoadXML(XMLNode* node, CStatus* status) {
    const char* inherits = node->GetAttributeByName("inherits");

    if (inherits && *inherits) {
        SIMPLEFONT* inherited = s_fontList.Ptr(inherits);

        if (inherited) {
            this->SetFontObject(inherited->m_font);
        } else {
            status->Add(STATUS_WARNING, "Couldn't find inherited font: %s", inherits);
        }
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

            if (!(fontHeight > 0.0f)) {
                const char* name = this->GetName();
                status->Add(STATUS_WARNING, "Font %s: Missing font height in %s element", name ? name : "<unnamed>", node->m_name.m_str);
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

            if (this->m_attributes.SetFont(fontName, fontHeight, fontFlags)) {
                this->m_fontableFlags &= ~0x1u;
            } else {
                status->Add(STATUS_WARNING, "Font %s: Unable to load font file %s", this->GetDisplayName(), fontName);
            }
        }
    }

    const char* spacingAttr = node->GetAttributeByName("spacing");

    if (spacingAttr && *spacingAttr) {
        float spacing = SStrToFloat(spacingAttr);
        float ddcSpacing = NDCToDDCWidth(spacing / (CoordinateGetAspectCompensation() * 1024.0f));

        if (fabsf(ddcSpacing - this->m_attributes.m_spacing) >= 0.00000023841858f || !(this->m_attributes.m_flags & 0x1000)) {
            this->m_attributes.m_flags |= 0x1010;
            this->m_attributes.m_spacing = ddcSpacing;
        }

        this->m_fontableFlags &= ~0x10u;
    }

    uint32_t justify;
    const char* justifyV = node->GetAttributeByName("justifyV");

    if (justifyV && *justifyV && StringToJustify(justifyV, justify)) {
        this->m_attributes.SetJustifyV(justify);
        this->m_fontableFlags &= ~0x2u;
    }

    const char* justifyH = node->GetAttributeByName("justifyH");

    if (justifyH && *justifyH && StringToJustify(justifyH, justify)) {
        this->m_attributes.SetJustifyH(justify);
        this->m_fontableFlags &= ~0x2u;
    }

    const char* nonSpaceWrap = node->GetAttributeByName("nonspacewrap");

    if (nonSpaceWrap && *nonSpaceWrap) {
        uint32_t styleFlags = StringToBOOL(nonSpaceWrap) ? this->m_attributes.m_styleFlags | 0x1000 : this->m_attributes.m_styleFlags & ~0x1000u;

        if (styleFlags != this->m_attributes.m_styleFlags || !(this->m_attributes.m_flags & 0x200)) {
            this->m_attributes.m_flags |= 0x202;
            this->m_attributes.m_styleFlags = styleFlags;
        }
    }

    const char* indented = node->GetAttributeByName("indented");

    if (indented && *indented) {
        uint32_t styleFlags = StringToBOOL(indented) ? this->m_attributes.m_styleFlags | 0x20000 : this->m_attributes.m_styleFlags & ~0x20000u;

        if (styleFlags != this->m_attributes.m_styleFlags || !(this->m_attributes.m_flags & 0x200)) {
            this->m_attributes.m_flags |= 0x202;
            this->m_attributes.m_styleFlags = styleFlags;
        }
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Color", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);

            if (*reinterpret_cast<uint32_t*>(&color) != *reinterpret_cast<uint32_t*>(&this->m_attributes.m_color) || !(this->m_attributes.m_flags & 0x400)) {
                this->m_attributes.m_color = color;
                this->m_attributes.m_flags |= 0x404;
            }

            this->m_fontableFlags &= ~0x4u;
        } else if (!SStrCmpI(child->m_name.m_str, "Shadow", STORM_MAX_STR)) {
            CImVector color;
            color.a = 0xFF;
            C2Vector offset;
            offset.x = 0.001f;
            offset.y = -0.001f;

            XMLNode* colorNode = child->GetChildByName("Color");

            if (colorNode) {
                LoadXML_Color(colorNode, color, status);
            }

            XMLNode* offsetNode = child->GetChildByName("Offset");

            if (offsetNode) {
                LoadXML_Dimensions(offsetNode, offset.x, offset.y, status);
            }

            if (*reinterpret_cast<uint32_t*>(&color) != *reinterpret_cast<uint32_t*>(&this->m_attributes.m_shadowColor) || offset.x != this->m_attributes.m_shadowOffset.x || offset.y != this->m_attributes.m_shadowOffset.y || !(this->m_attributes.m_flags & 0x800)) {
                this->m_attributes.m_shadowColor = color;
                this->m_attributes.m_flags |= 0x808;
                this->m_attributes.m_shadowOffset = offset;
            }

            this->m_fontableFlags &= ~0x8u;
        }
    }

    this->UpdateObjects();
}
