#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleCheckbox.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// STATIC: 0x00B2D414
int32_t CSimpleCheckbox::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE458
int32_t CSimpleCheckbox::s_objectType;

// STATIC: 0x00B2CBD8
const char* CSimpleCheckbox::s_objectTypeName = "CheckButton";

// STATIC: 0x00DCE45C
CDataAllocator CSimpleCheckbox::s_allocator(sizeof(CSimpleCheckbox), 50);

// INLINED
void CSimpleCheckbox::operator delete(void* ptr) {
    if (ptr) {
        CSimpleCheckbox::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleCheckbox::GetObjectType() {
    if (!CSimpleCheckbox::s_objectType) {
        CSimpleCheckbox::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleCheckbox::s_objectType;
}

// VERIFY: abee3662 auto
// OFFSET: 0x009620E0
CSimpleCheckbox::CSimpleCheckbox(CSimpleFrame* parent) : CSimpleButton(parent) {
    this->m_checked = 0;
    this->m_checkedTexture = nullptr;
    this->m_disabledCheckedTexture = nullptr;
}

// VERIFY: bb4c9a26 auto
// OFFSET: 0x00962180
const char* CSimpleCheckbox::GetObjectTypeName() {
    return CSimpleCheckbox::s_objectTypeName;
}

// VERIFY: d075bc3c auto
// OFFSET: 0x00962190
int32_t CSimpleCheckbox::GetScriptMetaTable() {
    return CSimpleCheckbox::s_metatable;
}

// VERIFY: 4b06d372 auto
// OFFSET: 0x009621A0
void CSimpleCheckbox::UnregisterRegion(CSimpleRegion* region) {
    if (region == this->m_checkedTexture) {
        this->m_checkedTexture = nullptr;
    }

    if (region == this->m_disabledCheckedTexture) {
        this->m_disabledCheckedTexture = nullptr;
    }

    CSimpleButton::UnregisterRegion(region);
}

// VERIFY: cef0c454 auto
// OFFSET: [0x009621D0, 0x00962300]
CSimpleCheckbox::~CSimpleCheckbox() {
}

// VERIFY: 335846d3 auto
// OFFSET: 0x00962260
bool CSimpleCheckbox::IsA(int32_t type) {
    return type == CSimpleCheckbox::s_objectType || type == CSimpleButton::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: d502bdb9 auto
// OFFSET: 0x009622A0
bool CSimpleCheckbox::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleCheckbox::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CSimpleButton::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 0e5dc769 manual
// OFFSET: 0x00962340
void CSimpleCheckbox::SetChecked(int32_t checked, int32_t force) {
    if (!force && checked == this->m_checked) {
        return;
    }

    this->m_checked = checked;

    if (this->m_checkedTexture) {
        this->m_checkedTexture->m_shown = 0;
        this->m_checkedTexture->HideThis();
    }

    if (this->m_disabledCheckedTexture) {
        this->m_disabledCheckedTexture->m_shown = 0;
        this->m_disabledCheckedTexture->HideThis();
    }

    if (!this->m_checked) {
        return;
    }

    CSimpleTexture* texture;

    if (this->m_disabledCheckedTexture && this->m_state == BUTTONSTATE_DISABLED) {
        texture = this->m_disabledCheckedTexture;
    } else {
        texture = this->m_checkedTexture;

        if (!texture) {
            return;
        }
    }

    texture->m_shown = 1;
    texture->ShowThis();
}

// VERIFY: 2558c229 auto
// OFFSET: 0x009623C0
void CSimpleCheckbox::OnClick(const char* btn, int32_t down) {
    this->SetChecked(this->m_checked == 0, 0);
    CSimpleButton::OnClick(btn, down);
}

// VERIFY: a04f7c25 auto
// OFFSET: 0x009623F0
void CSimpleCheckbox::SetCheckedTexture(CSimpleTexture* texture) {
    if (texture == this->m_checkedTexture) {
        return;
    }

    if (this->m_checkedTexture) {
        delete this->m_checkedTexture;
    }

    if (texture) {
        texture->SetFrame(this, 3, 0);
    }

    this->m_checkedTexture = texture;
    this->SetChecked(this->m_checked, 1);
}

// VERIFY: 7036b472 auto
// OFFSET: 0x00962440
void CSimpleCheckbox::SetDisabledCheckedTexture(CSimpleTexture* texture) {
    if (texture == this->m_disabledCheckedTexture) {
        return;
    }

    if (this->m_disabledCheckedTexture) {
        delete this->m_disabledCheckedTexture;
    }

    if (texture) {
        texture->SetFrame(this, 3, 0);
    }

    this->m_disabledCheckedTexture = texture;
    this->SetChecked(this->m_checked, 1);
}

// VERIFY: 1affe968 auto
// OFFSET: 0x00962490
void CSimpleCheckbox::Enable(int32_t enabled) {
    CSimpleButton::Enable(enabled);
    this->SetChecked(this->m_checked, 1);
}

// VERIFY: 89a43e3d manual
// OFFSET: 0x009624C0
void CSimpleCheckbox::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleButton::LoadXML(node, status);

    const char* checked = node->GetAttributeByName("checked");

    if (checked && *checked) {
        this->SetChecked(StringToBOOL(checked), 0);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "CheckedTexture", STORM_MAX_STR)) {
            this->SetCheckedTexture(LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "DisabledCheckedTexture", STORM_MAX_STR)) {
            this->SetDisabledCheckedTexture(LoadXML_Texture(child, this, status));
        }
    }
}

// VERIFY: 8bf7cd0a manual
// OFFSET: 0x00962660
int32_t CSimpleCheckbox::SetCheckedTexture(const char* fileName) {
    if (this->m_checkedTexture) {
        this->m_checkedTexture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        return 1;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(nullptr, 2, 1) : nullptr;

    if (!texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        if (texture) {
            delete texture;
        }

        return 0;
    }

    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(GxBlend_Add);
    this->SetCheckedTexture(texture);
    return 1;
}

// VERIFY: 8b91e462 manual
// OFFSET: 0x00962720
int32_t CSimpleCheckbox::SetDisabledCheckedTexture(const char* fileName) {
    if (this->m_disabledCheckedTexture) {
        this->m_disabledCheckedTexture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        return 1;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(nullptr, 2, 1) : nullptr;

    if (!texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        if (texture) {
            delete texture;
        }

        return 0;
    }

    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(GxBlend_Add);
    this->SetDisabledCheckedTexture(texture);
    return 1;
}
