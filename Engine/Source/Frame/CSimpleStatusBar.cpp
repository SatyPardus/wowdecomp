#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleStatusBar.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00B2CE18
int32_t CSimpleStatusBar::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE440
int32_t CSimpleStatusBar::s_objectType;

// STATIC: 0x00B2CBD4
const char* CSimpleStatusBar::s_objectTypeName = "StatusBar";

// STATIC: 0x00DCE444
CDataAllocator CSimpleStatusBar::s_allocator(sizeof(CSimpleStatusBar), 5);

// INLINED
void CSimpleStatusBar::operator delete(void* ptr) {
    if (ptr) {
        CSimpleStatusBar::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleStatusBar::GetObjectType() {
    if (!CSimpleStatusBar::s_objectType) {
        CSimpleStatusBar::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleStatusBar::s_objectType;
}

// VERIFY: 8f81c45b auto
// OFFSET: 0x00961560
void CSimpleStatusBar::RunOnMinMaxChangedScript() {
    if (this->m_onMinMaxChanged.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, this->m_minValue);
        lua_pushnumber(L, this->m_maxValue);
        this->RunScript(this->m_onMinMaxChanged, 2, nullptr);
    }
}

// VERIFY: cd515e0f auto
// OFFSET: [0x009615C0, 0x00961970, 0x00961A70]
CSimpleStatusBar::~CSimpleStatusBar() {
}

// VERIFY: 9f838f6a auto
// OFFSET: 0x009615F0
void CSimpleStatusBar::SetStatusBarColor(const CImVector& color) {
    if (this->m_barTexture) {
        this->m_barTexture->SetVertexColor(color);
    }
}

// VERIFY: 1dccb1ab auto
// OFFSET: 0x00961610
const char* CSimpleStatusBar::GetObjectTypeName() {
    return CSimpleStatusBar::s_objectTypeName;
}

// VERIFY: 84bf541f auto
// OFFSET: 0x00961620
int32_t CSimpleStatusBar::GetScriptMetaTable() {
    return CSimpleStatusBar::s_metatable;
}

// VERIFY: 8578fc21 auto
// OFFSET: 0x00961630
FrameScript_Object::ScriptIx* CSimpleStatusBar::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnValueChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,value) %s end";
        return &this->m_onValueChanged;
    }

    if (!SStrCmpI(name, "OnMinMaxChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,min,max) %s end";
        return &this->m_onMinMaxChanged;
    }

    return nullptr;
}

// VERIFY: 705d4a97 auto
// OFFSET: 0x009616A0
void CSimpleStatusBar::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onValueChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMinMaxChanged, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: 0b85180c auto
// OFFSET: 0x009616F0
void CSimpleStatusBar::UnregisterRegion(CSimpleRegion* region) {
    if (region == this->m_barTexture) {
        this->m_barTexture = nullptr;
    }

    CSimpleFrame::UnregisterRegion(region);
}

// VERIFY: 19c7ecf8 auto
// OFFSET: 0x00961720
void CSimpleStatusBar::SetBarTexture(CSimpleTexture* texture, uint32_t drawLayer) {
    if (texture == this->m_barTexture) {
        return;
    }

    if (this->m_barTexture) {
        delete this->m_barTexture;
    }

    if (texture) {
        texture->SetFrame(this, drawLayer, 1);
        texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, this, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
        texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
        texture->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
        texture->SetPoint(FRAMEPOINT_TOPRIGHT, this, FRAMEPOINT_TOPRIGHT, 0.0f, 0.0f, 1);
    }

    this->m_changed = 1;
    this->m_barTexture = texture;
}

// VERIFY: ac23d1fa auto
// OFFSET: 0x009617E0
void CSimpleStatusBar::SetOrientation(ORIENTATION orientation) {
    this->m_changed = 1;
    this->m_orientation = orientation;
}

// VERIFY: 4f2300fa auto
// OFFSET: 0x00961800
void CSimpleStatusBar::SetMinMaxValues(float minValue, float maxValue) {
    if (minValue > maxValue) {
        minValue = maxValue;
    }

    if (this->m_rangeSet && this->m_minValue == minValue && this->m_maxValue == maxValue) {
        return;
    }

    this->m_minValue = minValue;
    this->m_changed = 1;
    this->m_rangeSet = 1;
    this->m_maxValue = maxValue;
    this->RunOnMinMaxChangedScript();

    if (this->m_valueSet) {
        this->SetValue(this->m_value);
    }
}

// VERIFY: 64665966 manual
// OFFSET: 0x00961890
void CSimpleStatusBar::SetValue(float value) {
    if (!this->m_rangeSet) {
        return;
    }

    if (value > this->m_maxValue) {
        value = this->m_maxValue;
    } else if (value < this->m_minValue) {
        value = this->m_minValue;
    }

    if (this->m_valueSet && this->m_value == value) {
        return;
    }

    this->m_value = value;
    this->m_changed = 1;
    this->m_valueSet = 1;

    if (this->m_onValueChanged.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, this->m_value);
        this->RunScript(this->m_onValueChanged, 1, nullptr);
    }
}

// VERIFY: 38b829a1 auto
// OFFSET: 0x00961940
float CSimpleStatusBar::GetAnimValue() const {
    float range = this->m_maxValue - this->m_minValue;

    if (range > 0.0f) {
        return (this->m_value - this->m_minValue) / range;
    }

    return 0.0f;
}

// VERIFY: 74ee586d auto
// OFFSET: 0x00961980
bool CSimpleStatusBar::IsA(int32_t type) {
    return type == CSimpleStatusBar::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: eb1c4628 auto
// OFFSET: 0x009619C0
bool CSimpleStatusBar::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleStatusBar::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: d599a721 auto
// OFFSET: 0x00961A00
CSimpleStatusBar::CSimpleStatusBar(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_changed = 0;
    this->m_rangeSet = 0;
    this->m_valueSet = 0;
    this->m_rotatesTexture = 0;
    this->m_minValue = 0.0f;
    this->m_maxValue = 0.0f;
    this->m_barTexture = nullptr;
    this->m_value = 0.0f;
    this->m_orientation = ORIENTATION_HORIZONTAL;
}

// VERIFY: d7ce03a7 auto
// OFFSET: 0x00961AC0
int32_t CSimpleStatusBar::SetBarTexture(const char* fileName, uint32_t drawLayer) {
    CSimpleTexture* texture = this->m_barTexture;

    if (texture) {
        texture->SetTexture(fileName, (texture->m_textureFlags >> 2) & 1, (texture->m_textureFlags >> 3) & 1, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        this->SetBarTexture(texture, drawLayer);
        return 1;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    texture = data ? new (data) CSimpleTexture(nullptr, 2, 1) : nullptr;

    if (!texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        if (texture) {
            delete texture;
        }

        return 0;
    }

    this->SetBarTexture(texture, drawLayer);
    return 1;
}

// VERIFY: b5c4efa8 manual
// OFFSET: 0x00961B80
void CSimpleStatusBar::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    if (!this->m_changed || !this->m_barTexture || !this->m_rangeSet || !this->m_valueSet) {
        return;
    }

    float value = this->GetAnimValue();

    if (value > 0.0f) {
        float width;
        float height;
        this->GetSize(width, height, 0);

        this->m_barTexture->m_shown = 1;
        this->m_barTexture->ShowThis();

        if (this->m_orientation == ORIENTATION_VERTICAL) {
            this->m_barTexture->SetPoint(FRAMEPOINT_BOTTOMLEFT, this, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
            this->m_barTexture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
            value = 1.0f - value;
            this->m_barTexture->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, -(value * height), 1);
            this->m_barTexture->SetPoint(FRAMEPOINT_TOPRIGHT, this, FRAMEPOINT_TOPRIGHT, 0.0f, -(value * height), 1);
        } else {
            this->m_barTexture->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
            this->m_barTexture->SetPoint(FRAMEPOINT_BOTTOMLEFT, this, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
            value = 1.0f - value;
            this->m_barTexture->SetPoint(FRAMEPOINT_TOPRIGHT, this, FRAMEPOINT_TOPRIGHT, -(value * width), 0.0f, 1);
            this->m_barTexture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this, FRAMEPOINT_BOTTOMRIGHT, -(value * width), 0.0f, 1);
        }

        this->m_changed = 0;
    } else {
        this->m_barTexture->m_shown = 0;
        this->m_barTexture->HideThis();
        this->m_changed = 0;
    }
}

// VERIFY: 7a8b77e2 manual
// OFFSET: 0x00961D70
void CSimpleStatusBar::SetRotatesTexture(int32_t rotates) {
    if (rotates) {
        C2Vector texCoords[4];
        texCoords[0].x = 0.0f;
        texCoords[0].y = 1.0f;
        texCoords[1].x = 1.0f;
        texCoords[1].y = 1.0f;
        texCoords[2].x = 0.0f;
        texCoords[2].y = 0.0f;
        texCoords[3].x = 1.0f;
        texCoords[3].y = 0.0f;
        this->m_barTexture->SetTexCoord(texCoords);
    } else {
        CRect texRect = { 0.0f, 0.0f, 1.0f, 1.0f };
        this->m_barTexture->SetTexCoord(texRect);
    }

    this->m_rotatesTexture = rotates;
    this->m_changed = 1;
}

// VERIFY: f9c4a1fd manual
// OFFSET: 0x00961E30
void CSimpleStatusBar::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    uint32_t drawLayer = 2;
    const char* drawLayerAttr = node->GetAttributeByName("drawLayer");

    if (drawLayerAttr && *drawLayerAttr) {
        StringToDrawLayer(drawLayerAttr, drawLayer);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "BarTexture", STORM_MAX_STR)) {
            this->SetBarTexture(LoadXML_Texture(child, this, status), drawLayer);
        } else if (!SStrCmpI(child->m_name.m_str, "BarColor", STORM_MAX_STR)) {
            CImVector color;

            if (LoadXML_Color(child, color, status)) {
                this->SetStatusBarColor(color);
            }
        }
    }

    const char* minValueAttr = node->GetAttributeByName("minValue");

    if (minValueAttr && *minValueAttr) {
        float minValue = SStrToFloat(minValueAttr);
        const char* maxValueAttr = node->GetAttributeByName("maxValue");

        if (maxValueAttr && *maxValueAttr) {
            float maxValue = SStrToFloat(maxValueAttr);

            if (minValue < -1.0e12 || minValue > 1.0e12 || maxValue < -1.0e12 || maxValue > 1.0e12) {
                const char* name = this->GetName();
                status->Add(STATUS_ERROR, "Frame %s: Min or Max out of range", name ? name : "<unnamed>");
            } else if (maxValue - minValue > 1.0e12) {
                status->Add(STATUS_ERROR, "Frame %s: Min and Max too far apart", this->GetDisplayName());
            } else {
                this->SetMinMaxValues(minValue, maxValue);
            }

            const char* defaultValueAttr = node->GetAttributeByName("defaultValue");

            if (defaultValueAttr && *defaultValueAttr) {
                this->SetValue(SStrToFloat(defaultValueAttr));
            }
        }
    }

    const char* orientationAttr = node->GetAttributeByName("orientation");

    if (orientationAttr && *orientationAttr) {
        ORIENTATION orientation;

        if (StringToOrientation(orientationAttr, orientation)) {
            this->m_changed = 1;
            this->m_orientation = orientation;
        } else {
            const char* nodeName = node->m_name.m_str;
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown orientation %s in element %s", name ? name : "<unnamed>", orientationAttr, nodeName);
        }
    }

    const char* rotatesTextureAttr = node->GetAttributeByName("rotatesTexture");

    if (rotatesTextureAttr && *rotatesTextureAttr) {
        this->SetRotatesTexture(StringToBOOL(rotatesTextureAttr));
    }
}
