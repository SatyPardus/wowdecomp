#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleColorSelect.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Frame/ColorTexture.h"

// STATIC: 0x00B2CDB0
int32_t CSimpleColorSelect::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE51C
int32_t CSimpleColorSelect::s_objectType;

// STATIC: 0x00B2CD18
const char* CSimpleColorSelect::s_objectTypeName = "ColorSelect";

// STATIC: 0x00DCE520
CDataAllocator CSimpleColorSelect::s_allocator(sizeof(CSimpleColorSelect), 5);

// VERIFY: 1071f582 manual
// OFFSET: 0x0096DEC0
static void ValueTextureUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    // STATIC: 0x00DCE538
    static CImVector s_texels[64];

    switch (command) {
        case GxTex_Lock: {
            for (uint32_t i = 0; i < 64; i++) {
                s_texels[i] = *static_cast<CImVector*>(userArg);
            }

            break;
        }

        case GxTex_Latch: {
            texelStrideInBytes = 4 * width;
            texels = s_texels;
            break;
        }

        default:
            break;
    }
}

// INLINED
void CSimpleColorSelect::operator delete(void* ptr) {
    if (ptr) {
        CSimpleColorSelect::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleColorSelect::GetObjectType() {
    if (!CSimpleColorSelect::s_objectType) {
        CSimpleColorSelect::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleColorSelect::s_objectType;
}

// VERIFY: 5165e069 auto
// OFFSET: 0x0096DC30
const char* CSimpleColorSelect::GetObjectTypeName() {
    return CSimpleColorSelect::s_objectTypeName;
}

// VERIFY: 2cca8bdf auto
// OFFSET: 0x0096DC40
int32_t CSimpleColorSelect::GetScriptMetaTable() {
    return CSimpleColorSelect::s_metatable;
}

// VERIFY: 46918083 auto
// OFFSET: 0x0096DC50
FrameScript_Object::ScriptIx* CSimpleColorSelect::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnColorSelect", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,r,g,b) %s end";
        return &this->m_onColorSelect;
    }

    return nullptr;
}

// VERIFY: 7e8a9662 auto
// OFFSET: 0x0096DCA0
void CSimpleColorSelect::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onColorSelect, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: 3273159f auto
// OFFSET: 0x0096DCE0
void CSimpleColorSelect::UnregisterRegion(CSimpleRegion* region) {
    if (region == this->m_wheelTexture) {
        this->m_wheelTexture = nullptr;
    }

    if (region == this->m_wheelThumbTexture) {
        this->m_wheelThumbTexture = nullptr;
    }

    if (region == this->m_valueTexture) {
        this->m_valueTexture = nullptr;
    }

    if (region == this->m_valueThumbTexture) {
        this->m_valueThumbTexture = nullptr;
    }

    CSimpleFrame::UnregisterRegion(region);
}

// VERIFY: a4825dc5 manual
// OFFSET: 0x0096DD30
void CSimpleColorSelect::OnLayerHide() {
    this->m_wheelTracking = false;
    this->m_valueTracking = false;
    CSimpleFrame::OnLayerHide();
}

// VERIFY: ff010810 manual
// OFFSET: 0x0096DD50
int32_t CSimpleColorSelect::OnLayerMouseUp(const CMouseEvent& evt, const char* btn) {
    this->m_wheelTracking = false;
    this->m_valueTracking = false;
    return CSimpleFrame::OnLayerMouseUp(evt, btn);
}

// VERIFY: 26cba43c auto
// OFFSET: [0x0096DD70, 0x0096DFE0]
CSimpleColorSelect::~CSimpleColorSelect() {
}

// VERIFY: 73ae1417 auto
// OFFSET: 0x0096DD80
void CSimpleColorSelect::RunOnColorSelectScript() {
    if (!this->m_onColorSelect.m_luaRef) {
        return;
    }

    lua_State* L = FrameScript_GetContext();
    C3Vector rgb;
    CImVector color;
    HSVtoRGB(this->m_hsv, rgb);
    color = rgb;
    lua_pushnumber(L, color.r * (1.0 / 255.0));
    lua_pushnumber(L, color.g * (1.0 / 255.0));
    lua_pushnumber(L, color.b * (1.0 / 255.0));
    this->RunScript(this->m_onColorSelect, 3, nullptr);
}

// VERIFY: 33027447 auto
// OFFSET: 0x0096DE40
bool CSimpleColorSelect::IsA(int32_t type) {
    return type == CSimpleColorSelect::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 6a35e770 auto
// OFFSET: 0x0096DE80
bool CSimpleColorSelect::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleColorSelect::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 2fd3ddbf manual
// OFFSET: 0x0096DF30
CSimpleColorSelect::CSimpleColorSelect(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_wheelTracking = false;
    this->m_valueTracking = false;
    this->m_wheelTexture = nullptr;
    this->m_wheelThumbTexture = nullptr;
    this->m_valueTexture = nullptr;
    this->m_valueThumbTexture = nullptr;
    this->m_hsv = C3Vector(0.0f, 0.0f, 1.0f);
    this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
}

// VERIFY: 3d990beb auto
// OFFSET: 0x0096E030
void CSimpleColorSelect::UpdateValueTexture() {
    if (!this->m_valueTexture) {
        return;
    }

    C3Vector rgb;
    C3Vector hsv(this->m_hsv.x, this->m_hsv.y, 1.0f);
    HSVtoRGB(hsv, rgb);
    this->m_valueColor = rgb;
    GxTexUpdate(this->m_valueTexture->GetGxTex(), 0, 0, 0, 0, 0);
}

// VERIFY: 83c19f99 manual
// OFFSET: 0x0096E0B0
void CSimpleColorSelect::RepositionWheelThumb() {
    if (!this->m_wheelTexture || !this->m_wheelThumbTexture) {
        return;
    }

    C2Vector offset;
    float angle = this->m_hsv.x * 0.017453292f;
    offset.x = cos(angle);
    offset.y = sin(angle);

    float radius = this->m_wheelTexture->GetWidth() * 0.5f * this->m_hsv.y;
    offset.x *= radius;
    offset.y *= radius;

    this->m_wheelThumbTexture->SetPoint(FRAMEPOINT_CENTER, this->m_wheelTexture, FRAMEPOINT_CENTER, -offset.x, -offset.y, 1);
}

// VERIFY: 0e854627 auto
// OFFSET: 0x0096E170
void CSimpleColorSelect::RepositionValueThumb() {
    if (!this->m_valueTexture || !this->m_valueThumbTexture) {
        return;
    }

    float offsetY = this->m_wheelTexture->GetHeight() * this->m_hsv.z;
    this->m_valueThumbTexture->SetPoint(FRAMEPOINT_CENTER, this->m_valueTexture, FRAMEPOINT_BOTTOM, 0.0f, offsetY, 1);
}

// VERIFY: 2805e2d6 auto
// OFFSET: 0x0096E1D0
int32_t CSimpleColorSelect::OnLayerMouseDown(const CMouseEvent& evt, const char* btn) {
    C2Vector point = evt.m_data.m_mousePosition;

    if (this->m_wheelTexture && this->m_wheelTexture->PtInFrameRect(point)) {
        this->m_wheelTracking = true;
    }

    if (this->m_valueTexture && this->m_valueTexture->PtInFrameRect(point)) {
        this->m_valueTracking = true;
    }

    this->OnLayerTrackUpdate(evt);
    return CSimpleFrame::OnLayerMouseDown(evt, btn);
}

// VERIFY: 73b72b76 auto
// OFFSET: 0x0096E250
void CSimpleColorSelect::SetColorWheelThumbTexture(CSimpleTexture* texture) {
    if (texture == this->m_wheelThumbTexture) {
        return;
    }

    if (this->m_wheelThumbTexture) {
        delete this->m_wheelThumbTexture;
    }

    if (texture) {
        texture->SetFrame(this, 3, 1);
        texture->MarkFramePointsUnused();
    }

    this->m_wheelThumbTexture = texture;
    this->RepositionWheelThumb();
}

// VERIFY: c247fc29 manual
// OFFSET: 0x0096E2A0
void CSimpleColorSelect::SetColorValueTexture(CSimpleTexture* texture) {
    if (texture == this->m_valueTexture) {
        return;
    }

    if (this->m_valueTexture) {
        delete this->m_valueTexture;
    }

    if (texture) {
        CTexture* valueTexture = TextureCreate(8, 8, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &this->m_valueColor, ValueTextureUpdate, "ValueTexture", 0);
        texture->SetTexture(valueTexture);

        CImVector minColor;
        minColor.b = 0;
        minColor.g = 0;
        minColor.r = 0;
        minColor.a = 0xFF;

        CImVector maxColor;
        maxColor.b = 0xFF;
        maxColor.g = 0xFF;
        maxColor.r = 0xFF;
        maxColor.a = 0xFF;

        texture->SetVertexGradient(ORIENTATION_VERTICAL, minColor, maxColor);
        texture->SetFrame(this, 2, 1);

        if (valueTexture) {
            HandleClose(reinterpret_cast<HOBJECT>(valueTexture));
        }
    }

    this->m_valueTexture = texture;
    this->UpdateValueTexture();
}

// VERIFY: 7a2afebd auto
// OFFSET: 0x0096E380
void CSimpleColorSelect::SetColorValueThumbTexture(CSimpleTexture* texture) {
    if (texture == this->m_valueThumbTexture) {
        return;
    }

    if (this->m_valueThumbTexture) {
        delete this->m_valueThumbTexture;
    }

    if (texture) {
        texture->SetFrame(this, 3, 1);
        texture->MarkFramePointsUnused();
    }

    this->m_valueThumbTexture = texture;
    this->RepositionValueThumb();
}

// VERIFY: dea5f881 auto
// OFFSET: 0x0096E3D0
void CSimpleColorSelect::UpdateColor() {
    this->UpdateValueTexture();
    this->RepositionWheelThumb();
    this->RepositionValueThumb();
    this->RunOnColorSelectScript();
}

// VERIFY: 9302786b manual
// OFFSET: 0x0096E3F0
int32_t CSimpleColorSelect::OnLayerTrackUpdate(const CMouseEvent& evt) {
    if (this->m_wheelTracking || this->m_valueTracking) {
        if (this->m_wheelTracking) {
            C2Vector point = evt.m_data.m_mousePosition;
            CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
            this->m_wheelTexture->GetRect(&rect);

            float x = (point.x - (rect.maxX + rect.minX) * 0.5f) / ((rect.maxX - rect.minX) * 0.5f);
            float y = (point.y - (rect.maxY + rect.minY) * 0.5f) / ((rect.maxY - rect.minY) * 0.5f);
            float distance = sqrt(y * y + x * x);

            this->m_hsv.x = (atan2(y, x) + 3.1415927f) * 57.29578f;

            if (distance >= 1.0f) {
                distance = 1.0f;
            }

            this->m_hsv.y = distance;
        }

        if (this->m_valueTracking) {
            float y = evt.m_data.m_mousePosition.y;
            CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
            this->m_valueTexture->GetRect(&rect);

            float value = (y - rect.minY) / (rect.maxY - rect.minY);

            if (value < 0.0f) {
                this->m_hsv.z = 0.0f;
            } else if (value >= 1.0f) {
                this->m_hsv.z = 1.0f;
            } else {
                this->m_hsv.z = value;
            }
        }

        this->UpdateColor();
    }

    return CSimpleFrame::OnLayerTrackUpdate(evt);
}

// VERIFY: 21d0b3fb auto
// OFFSET: 0x0096E560
int32_t CSimpleColorSelect::SetColorWheelThumbTexture(const char* fileName) {
    if (this->m_wheelThumbTexture) {
        this->m_wheelThumbTexture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
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

    this->SetColorWheelThumbTexture(texture);
    return 1;
}

// VERIFY: df3359d1 auto
// OFFSET: 0x0096E610
int32_t CSimpleColorSelect::SetColorValueThumbTexture(const char* fileName) {
    if (this->m_valueThumbTexture) {
        this->m_valueThumbTexture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
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

    this->SetColorValueThumbTexture(texture);
    return 1;
}

// VERIFY: cd3346e3 manual
// OFFSET: 0x0096E6C0
void CSimpleColorSelect::SetColorWheelTexture(CSimpleTexture* texture) {
    if (texture == this->m_wheelTexture) {
        return;
    }

    if (this->m_wheelTexture) {
        delete this->m_wheelTexture;
    }

    if (texture) {
        // STATIC: 0x00DCE63C
        static TSGrowableArray<CImVector> s_wheelTexels;

        if (!s_wheelTexels.Count()) {
            s_wheelTexels.SetCount(128 * 128);

            uint32_t index = 0;

            for (int32_t y = 63; y > -65; y--) {
                for (int32_t x = -64; x < 64; x++) {
                    CImVector color;
                    int32_t distanceSq = y * y + x * x;

                    if (distanceSq <= 64 * 64) {
                        C3Vector rgb;
                        C3Vector hsv((atan2(static_cast<double>(y), static_cast<double>(x)) + 3.1415927f) * 57.29578f, sqrt(static_cast<double>(distanceSq)) * 0.015625f, 1.0f);
                        HSVtoRGB(hsv, rgb);
                        color = rgb;
                    }

                    s_wheelTexels.m_data[index++] = color;
                }
            }
        }

        CTexture* wheelTexture = TextureCreate(128, 128, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &s_wheelTexels, EmblemTextureUpdate, "WheelTexture", 0);
        texture->SetTexture(wheelTexture);
        texture->SetFrame(this, 2, 1);

        if (wheelTexture) {
            HandleClose(reinterpret_cast<HOBJECT>(wheelTexture));
        }
    }

    this->m_wheelTexture = texture;
}

// VERIFY: b24271d1 manual
// OFFSET: 0x0096E880
void CSimpleColorSelect::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "ColorWheelTexture", STORM_MAX_STR)) {
            this->SetColorWheelTexture(LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "ColorWheelThumbTexture", STORM_MAX_STR)) {
            this->SetColorWheelThumbTexture(LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "ColorValueTexture", STORM_MAX_STR)) {
            this->SetColorValueTexture(LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "ColorValueThumbTexture", STORM_MAX_STR)) {
            this->SetColorValueThumbTexture(LoadXML_Texture(child, this, status));
        }
    }
}
