#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleSlider.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00B2CE88
int32_t CSimpleSlider::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE4D4
int32_t CSimpleSlider::s_objectType;

// STATIC: 0x00B2CCF4
const char* CSimpleSlider::s_objectTypeName = "Slider";

// STATIC: 0x00DCE4D8
CDataAllocator CSimpleSlider::s_allocator(sizeof(CSimpleSlider), 5);

// INLINED
void CSimpleSlider::operator delete(void* ptr) {
    if (ptr) {
        CSimpleSlider::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleSlider::GetObjectType() {
    if (!CSimpleSlider::s_objectType) {
        CSimpleSlider::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleSlider::s_objectType;
}

// VERIFY: a3f64b02 auto
// OFFSET: 0x0096BBB0
void CSimpleSlider::RunOnMinMaxChangedScript() {
    if (this->m_onMinMaxChanged.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, this->m_baseValue);
        lua_pushnumber(L, this->m_range + this->m_baseValue);
        this->RunScript(this->m_onMinMaxChanged, 2, nullptr);
    }
}

// VERIFY: e424afb5 manual
// OFFSET: 0x0096BC10
float CSimpleSlider::SnapValueToStep(float value) {
    if (0.0f == this->m_valueStep) {
        return value;
    }

    float offset = value - this->m_baseValue;
    float half = this->m_valueStep * 0.5f;

    if (offset > 0.0f) {
        return static_cast<float>(static_cast<int32_t>((offset + half) / this->m_valueStep)) * this->m_valueStep + this->m_baseValue;
    }

    return static_cast<float>(static_cast<int32_t>((offset - half) / this->m_valueStep)) * this->m_valueStep + this->m_baseValue;
}

// VERIFY: c521bada auto
// OFFSET: 0x0096BCA0
FrameScript_Object::ScriptIx* CSimpleSlider::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
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

// VERIFY: 3ccee8f8 auto
// OFFSET: 0x0096BD10
void CSimpleSlider::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onValueChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMinMaxChanged, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: 71516971 auto
// OFFSET: 0x0096BD60
void CSimpleSlider::UnregisterRegion(CSimpleRegion* region) {
    if (region == this->m_thumbTexture) {
        this->m_thumbTexture = nullptr;
    }

    CSimpleFrame::UnregisterRegion(region);
}

// VERIFY: d0eb7641 auto
// OFFSET: 0x0096BD90
void CSimpleSlider::SetThumbTexture(CSimpleTexture* texture, uint32_t drawLayer) {
    if (texture == this->m_thumbTexture) {
        return;
    }

    if (this->m_thumbTexture) {
        delete this->m_thumbTexture;
    }

    if (texture) {
        texture->SetFrame(this, drawLayer, 1);
        texture->MarkFramePointsUnused();
    }

    this->m_changed = 1;
    this->m_thumbTexture = texture;
}

// VERIFY: 01caecb5 auto
// OFFSET: 0x0096BDE0
void CSimpleSlider::SetOrientation(ORIENTATION orientation) {
    this->m_orientation = orientation;

    if (this->m_thumbTexture) {
        this->m_thumbTexture->MarkFramePointsUnused();
    }

    this->m_changed = 1;
}

// VERIFY: 1110a436 auto
// OFFSET: 0x0096BE10
void CSimpleSlider::OnFrameSizeChanged(const CRect& rect) {
    CSimpleFrame::OnFrameSizeChanged(rect);
    this->m_changed = 1;
}

// VERIFY: 157f4ca0 auto
// OFFSET: 0x0096BE30
void CSimpleSlider::OnLayerHide() {
    this->m_buttonDown = 0;
    CSimpleFrame::OnLayerHide();
}

// VERIFY: 2d5c1950 auto
// OFFSET: 0x0096BE40
int32_t CSimpleSlider::OnLayerMouseUp(const CMouseEvent& evt, const char* btn) {
    this->m_buttonDown = 0;
    return CSimpleFrame::OnLayerMouseUp(evt, btn);
}

// VERIFY: b10eac73 auto
// OFFSET: 0x0096BE50
CSimpleSlider::CSimpleSlider(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_changed = 0;
    this->m_rangeSet = 0;
    this->m_valueSet = 0;
    this->m_buttonDown = 0;
    this->m_baseValue = 0.0f;
    this->m_range = 0.0f;
    this->m_value = 0.0f;
    this->m_valueStep = 0.0f;
    this->m_thumbTexture = nullptr;
    this->m_orientation = ORIENTATION_VERTICAL;
    this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
}

// VERIFY: a4db6b4a auto
// OFFSET: 0x0096BED0
const char* CSimpleSlider::GetObjectTypeName() {
    return CSimpleSlider::s_objectTypeName;
}

// VERIFY: b56e9ccb auto
// OFFSET: 0x0096BEE0
int32_t CSimpleSlider::GetScriptMetaTable() {
    return CSimpleSlider::s_metatable;
}

// VERIFY: d8c76476 auto
// OFFSET: 0x0096BF00
bool CSimpleSlider::IsA(int32_t type) {
    return type == CSimpleSlider::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 6fe163ff auto
// OFFSET: 0x0096BF40
bool CSimpleSlider::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleSlider::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 53cf468c auto
// OFFSET: [0x0096BEF0, 0x0096BF80, 0x0096C440]
CSimpleSlider::~CSimpleSlider() {
    if (this->m_thumbTexture) {
        delete this->m_thumbTexture;
        this->m_changed = 1;
        this->m_thumbTexture = nullptr;
    }
}

// VERIFY: a1983b83 auto
// OFFSET: 0x0096BFE0
int32_t CSimpleSlider::SetThumbTexture(const char* fileName) {
    if (this->m_thumbTexture) {
        this->m_thumbTexture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        return 1;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, STORM_TYPE_NAME(CSimpleTexture), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(nullptr, 2, 1) : nullptr;

    if (texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        this->SetThumbTexture(texture, 3);
        return 1;
    }

    if (texture) {
        delete texture;
    }

    return 0;
}

// VERIFY: cd12c178 manual
// OFFSET: 0x0096C090
void CSimpleSlider::SetValue(float value) {
    if (!this->m_rangeSet) {
        return;
    }

    float maxValue = this->m_range + this->m_baseValue;
    value = std::min(std::max(value, this->m_baseValue), maxValue);
    value = this->SnapValueToStep(value);
    value = maxValue < value + this->m_valueStep ? maxValue : value;
    value = value - this->m_valueStep < this->m_baseValue ? this->m_baseValue : value;

    if (this->m_valueSet && fabsf(value - this->m_value) < 0.00000023841858f) {
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

// VERIFY: 3106fae6 auto
// OFFSET: 0x0096C1B0
void CSimpleSlider::SetValueStep(float step) {
    step = std::max(step, FLT_EPSILON);

    if (fabsf(step - this->m_valueStep) < 0.00000023841858f) {
        return;
    }

    this->m_valueStep = step;

    if (this->m_valueSet) {
        this->SetValue(this->m_value);
    }
}

// VERIFY: e27eccf5 manual
// OFFSET: 0x0096C210
void CSimpleSlider::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    if (!this->m_changed) {
        return;
    }

    CSimpleTexture* thumb = this->m_thumbTexture;

    if (!thumb || !this->m_rangeSet || !this->m_valueSet) {
        return;
    }

    float range = this->m_range + this->m_baseValue - this->m_baseValue;
    float position = fequal(range, 0.0f) ? 0.0f : (this->m_value - this->m_baseValue) / range;

    if (this->m_orientation != ORIENTATION_HORIZONTAL) {
        float height = (this->m_rect.maxY - this->m_rect.minY) / this->m_layoutScale;
        thumb->SetPoint(FRAMEPOINT_TOP, this, FRAMEPOINT_TOP, 0.0f, -((height - thumb->GetHeight()) * position), 1);
    } else {
        float width = (this->m_rect.maxX - this->m_rect.minX) / this->m_layoutScale;
        thumb->SetPoint(FRAMEPOINT_LEFT, this, FRAMEPOINT_LEFT, (width - thumb->GetWidth()) * position, 0.0f, 1);
    }

    this->m_changed = 0;
}

// VERIFY: 0d3f1c40 auto
// OFFSET: 0x0096C320
int32_t CSimpleSlider::OnLayerTrackUpdate(const CMouseEvent& evt) {
    if (this->m_buttonDown && this->m_thumbTexture) {
        float length;
        float offset;

        if (this->m_orientation != ORIENTATION_HORIZONTAL) {
            length = this->m_rect.maxY - this->m_rect.minY - this->m_thumbTexture->GetHeight();
            offset = this->m_rect.maxY - this->m_thumbTexture->GetHeight() * 0.5f - evt.m_data.m_mousePosition.y;
        } else {
            length = this->m_rect.maxX - this->m_rect.minX - this->m_thumbTexture->GetWidth();
            offset = evt.m_data.m_mousePosition.x - (this->m_thumbTexture->GetWidth() * 0.5f + this->m_rect.minX);
        }

        this->SetValue((this->m_range + this->m_baseValue - this->m_baseValue) * (offset / length) + this->m_baseValue);
    }

    return CSimpleFrame::OnLayerTrackUpdate(evt);
}

// VERIFY: 4a17f296 auto
// OFFSET: 0x0096C400
int32_t CSimpleSlider::OnLayerMouseDown(const CMouseEvent& evt, const char* btn) {
    if (!(this->m_frameFlags & 0x400)) {
        this->m_buttonDown = 1;
        this->OnLayerTrackUpdate(evt);
    }

    return CSimpleFrame::OnLayerMouseDown(evt, btn);
}

// VERIFY: f2e35f6a auto
// OFFSET: 0x0096C470
void CSimpleSlider::SetMinMaxValues(float min, float max) {
    float range = max - min;

    if (this->m_rangeSet && fabsf(range - this->m_range) < 0.00000023841858f && fabsf(this->m_baseValue - min) < 0.00000023841858f) {
        return;
    }

    this->m_baseValue = min;
    this->m_changed = 1;
    this->m_rangeSet = 1;
    this->m_range = range;
    this->RunOnMinMaxChangedScript();

    if (this->m_valueSet) {
        this->SetValue(this->m_value);
    }
}

// VERIFY: 13a5443a manual
// OFFSET: 0x0096C500
void CSimpleSlider::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    uint32_t drawLayer = 3;
    const char* drawLayerAttr = node->GetAttributeByName("drawLayer");

    if (drawLayerAttr && *drawLayerAttr) {
        StringToDrawLayer(drawLayerAttr, drawLayer);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "ThumbTexture", STORM_MAX_STR)) {
            this->SetThumbTexture(LoadXML_Texture(child, this, status), drawLayer);
        }
    }

    const char* valueStep = node->GetAttributeByName("valueStep");
    this->SetValueStep(valueStep && *valueStep ? SStrToFloat(valueStep) : 0.001f);

    const char* minValue = node->GetAttributeByName("minValue");

    if (minValue && *minValue) {
        float min = SStrToFloat(minValue);
        const char* maxValue = node->GetAttributeByName("maxValue");

        if (maxValue && *maxValue) {
            this->SetMinMaxValues(min, SStrToFloat(maxValue));

            const char* defaultValue = node->GetAttributeByName("defaultValue");

            if (defaultValue && *defaultValue) {
                this->SetValue(SStrToFloat(defaultValue));
            }
        }
    }

    const char* orientationAttr = node->GetAttributeByName("orientation");

    if (orientationAttr && *orientationAttr) {
        ORIENTATION orientation;

        if (StringToOrientation(orientationAttr, orientation)) {
            this->SetOrientation(orientation);
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown orientation %s in element %s", name ? name : "<unnamed>", orientationAttr, node->m_name.m_str);
        }
    }
}
