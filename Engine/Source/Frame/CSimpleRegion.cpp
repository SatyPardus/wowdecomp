#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/Frame/CSimpleRegion.h"
#include "Engine/Source/Frame/CSimpleAlphaAnim.h"
#include "Engine/Source/Frame/CSimpleAnimGroup.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include <cmath>
#include "Engine/Source/Tempest/CImVector.h"

// VERIFY: 8fb2cb5e auto
// OFFSET: 0x00482BD0
void CSimpleRegion::SetVertexAlpha(uint8_t alpha) {
    CImVector color;
    this->GetVertexColor(color);
    color.a = alpha;
    this->SetVertexColor(color);
}

// VERIFY: 282f4f29 auto
// OFFSET: 0x00482C00
uint8_t CSimpleRegion::GetVertexAlpha() {
    CImVector color;
    this->GetVertexColor(color);
    return color.a;
}

// VERIFY: e228d26d auto
// OFFSET: 0x00482C20
void CSimpleRegion::Hide() {
    this->m_shown = 0;
    this->HideThis();
}

// VERIFY: b827cd7a auto
// OFFSET: 0x00482C30
void CSimpleRegion::SetParent(CSimpleFrame* parent) {
    this->SetFrame(parent, this->m_drawLayer, this->m_shown);
}

// VERIFY: bb364772 auto
// OFFSET: 0x004879B0
int32_t CSimpleRegion::CanBeAnchorFor(CLayoutFrame* frame) {
    return frame->IsProtected() == 0;
}

// VERIFY: 9758b6a4 auto
// OFFSET: 0x004879D0
void CSimpleRegion::StoreAnimPosition() {
    this->Resize(1);
    this->m_animating = 1;
    this->m_animAlpha = 0;
}

// VERIFY: 47fc365d auto
// OFFSET: 0x004879F0
void CSimpleRegion::ResetAnimPosition() {
    this->m_animating = 0;
    this->m_animAlpha = 0;
    this->Resize(1);
}

// VERIFY: ed2f4b12 auto
// OFFSET: 0x00487A10
void CSimpleRegion::SetVertexColor(const CImVector& color) {
    if (!this->m_colorCount && color.a >= 0xFE && color.r == 0xFF && color.g == 0xFF && color.b == 0xFF) {
        return;
    }

    if (this->m_colorCount == 1 && this->m_alphas[0] == color.a && this->m_colors[0].r == color.r && this->m_colors[0].g == color.g && this->m_colors[0].b == color.b) {
        return;
    }

    bool changed = false;

    if (this->m_colorCount != 1) {
        this->m_alphaCount = 1;
        this->m_colorCount = 1;
        changed = true;
    }

    this->m_alphas[0] = color.a;
    this->m_colors[0] = color;

    this->OnColorChanged(changed);
}

// VERIFY: 32a4bff1 manual
// OFFSET: 0x00487AB0
void CSimpleRegion::GetVertexColor(CImVector& color) {
    if (this->m_colorCount == 1) {
        color.a = this->m_alphas[0];
        color.r = this->m_colors[0].r;
        color.g = this->m_colors[0].g;
        color.b = this->m_colors[0].b;
    } else {
        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;
    }
}

// VERIFY: b47ed56a auto
// OFFSET: 0x00487B00
void CSimpleRegion::SetVertexGradient(ORIENTATION orientation, const CImVector& minColor, const CImVector& maxColor) {
    bool changed = false;

    if (this->m_colorCount != 4) {
        this->m_alphaCount = 4;
        this->m_colorCount = 4;
        changed = true;
    }

    if (orientation == ORIENTATION_HORIZONTAL) {
        this->m_alphas[0] = minColor.a;
        this->m_colors[0] = minColor;
        this->m_alphas[1] = minColor.a;
        this->m_colors[1] = minColor;
        this->m_alphas[2] = maxColor.a;
        this->m_colors[2] = maxColor;
        this->m_alphas[3] = maxColor.a;
        this->m_colors[3] = maxColor;
    } else {
        this->m_alphas[1] = minColor.a;
        this->m_colors[1] = minColor;
        this->m_alphas[3] = minColor.a;
        this->m_colors[3] = minColor;
        this->m_alphas[0] = maxColor.a;
        this->m_colors[0] = maxColor;
        this->m_alphas[2] = maxColor.a;
        this->m_colors[2] = maxColor;
    }

    this->OnColorChanged(changed);
}

// VERIFY: 6e5dc5b3 auto
// OFFSET: 0x00487BF0
void CSimpleRegion::HideThis() {
    if (!this->m_visible || !this->m_parent) {
        return;
    }

    if (!this->m_parent->m_isLoading) {
        this->SetDeferredResize(1);
    }

    this->m_parent->RemoveFrameRegion(this, this->m_drawLayer);
    this->m_visible = 0;
}

// VERIFY: b0e6ff7b auto
// OFFSET: 0x00487C40
void CSimpleRegion::ShowThis() {
    if (!this->m_shown || !this->m_parent || !this->m_parent->m_isVisible || this->m_visible) {
        return;
    }

    if (!this->m_parent->m_isLoading) {
        this->SetDeferredResize(0);
    }

    this->m_parent->AddFrameRegion(this, this->m_drawLayer);
    this->m_visible = 1;
}

// VERIFY: fbf040a8 auto
// OFFSET: 0x00487CA0
void CSimpleRegion::OnRegionChanged() {
    if (this->m_visible && this->m_parent) {
        this->m_parent->NotifyDrawLayerChanged(this->m_drawLayer);
    }
}

// VERIFY: 51a4dfd1 manual
// OFFSET: 0x00487CC0
void CSimpleRegion::NotifyDelayedLoad() {
    if (this->m_visible && this->m_parent) {
        this->m_parent->NotifyDelayedLoad();
    }
}

// VERIFY: 63ab363f manual
// OFFSET: 0x00487CE0
void CSimpleRegion::AddAnimAlpha(int16_t alpha) {
    CImVector color;
    this->GetVertexColor(color);

    int16_t value = alpha + color.a;

    if (value > 255) {
        color.a = 255;
    } else {
        color.a = value < 0 ? 0 : value;
    }

    this->SetVertexColor(color);
}

// VERIFY: 87dca2c6 manual
// OFFSET: 0x00487D70
void CSimpleRegion::OnColorChanged(bool changed) {
    if (this->m_parent) {
        if ((uint8_t)(this->m_parent->m_alpha * this->m_parent->m_parentAlpha / 255) < 0xFE) {
            if (!this->m_colorCount) {
                this->m_alphaCount = 1;
                this->m_colorCount = 1;
                this->m_alphas[0] = 0xFF;
                this->m_colors[0].r = 0xFF;
                this->m_colors[0].g = 0xFF;
                this->m_colors[0].b = 0xFF;
                changed = true;
            }
        } else {
            uint32_t i = this->m_colorCount;

            while (i) {
                i--;

                if (this->m_alphas[i] < 0xFE || this->m_colors[i].r != 0xFF || this->m_colors[i].g != 0xFF || this->m_colors[i].b != 0xFF) {
                    break;
                }

                if (!i) {
                    this->m_alphaCount = 0;
                    this->m_colorCount = 0;
                    changed = true;
                }
            }
        }

        for (uint32_t i = this->m_colorCount; i; ) {
            i--;
            this->m_colors[i].a = this->m_alphas[i] * (uint8_t)(this->m_parent->m_alpha * this->m_parent->m_parentAlpha / 255) / 255;
        }
    }

    if (changed) {
        this->OnRegionChanged();
    }
}

// VERIFY: b519be3d manual
// OFFSET: 0x00487ED0
void CSimpleRegion::SetFrame(CSimpleFrame* frame, uint32_t drawLayer, int32_t show) {
    if (this->m_parent == frame) {
        if (this->m_drawLayer == drawLayer) {
            if (show == this->m_shown) {
                return;
            }

            if (show) {
                this->m_shown = 1;
                this->ShowThis();
            } else {
                this->Hide();
            }

            return;
        }

        if (this->m_shown) {
            this->Hide();
        }

        this->m_drawLayer = drawLayer;

        if (show) {
            this->m_shown = 1;
            this->ShowThis();
        }

        return;
    }

    if (this->m_parent) {
        this->HideThis();
        this->m_parent->UnregisterRegion(this);
    }

    this->m_parent = frame;
    this->m_drawLayer = drawLayer;

    if (!frame) {
        return;
    }

    frame->RegisterRegion(this);
    this->SetDeferredResize((this->m_parent->m_layoutFlags >> 8) & 2);
    this->OnColorChanged(false);

    if (show) {
        this->m_shown = 1;
        this->ShowThis();
    } else {
        this->Hide();
    }
}

// VERIFY: 3115eee5 manual
// OFFSET: 0x00488000
void CSimpleRegion::AnimActivated(CSimpleAnimGroup* group, int32_t first, int32_t hasAlpha) {
    if (first && (!this->m_playingAnimGroups || !this->m_playingAnimGroups->Head())) {
        this->StoreAnimPosition();
    }

    if (!this->m_animAlpha && hasAlpha) {
        this->m_animAlpha = 1;
        this->m_animSavedAlpha = this->GetVertexAlpha();
    }
}

// VERIFY: 09d5e65c manual
// OFFSET: 0x00488060
void CSimpleRegion::AnimDeactivated(CSimpleAnimGroup* group, int32_t last) {
    if (this->m_animAlpha) {
        this->m_animAlpha = this->HasActiveAnimOfType(CSimpleAlphaAnim::GetObjectType());

        if (!this->m_animAlpha) {
            this->SetVertexAlpha(this->m_animSavedAlpha);
        }
    }

    if (last && (!this->m_playingAnimGroups || !this->m_playingAnimGroups->Head())) {
        this->ResetAnimPosition();
    }
}

// VERIFY: 48ca4759 manual
// OFFSET: 0x004880E0
CSimpleRegion::CSimpleRegion(CSimpleFrame* frame, uint32_t drawLayer, int32_t show) {
    this->m_alphaCount = 4;
    this->m_colorCount = 4;
    this->m_drawLayer = DRAWLAYER_BACKGROUND;
    this->m_shown = 0;
    this->m_visible = 0;
    this->m_animating = 0;
    this->m_animAlpha = 0;
    this->m_animSavedAlpha = 0xFF;

    memset(this->m_colors, 0, this->m_colorCount * sizeof(CImVector));
    this->m_colorCount = 0;
    memset(this->m_alphas, 0, this->m_alphaCount);
    this->m_alphaCount = 0;

    if (frame) {
        this->SetFrame(frame, drawLayer, show);
    }
}

// VERIFY: d3e924a1 manual
// OFFSET: [0x004881B0, 0x004881D0, 0x004882E0]
CSimpleRegion::~CSimpleRegion() {
    this->SetFrame(nullptr, 1, 0);
}

// VERIFY: a61fca48 manual
// OFFSET: 0x005EEB70
void CSimpleRegion::OnScreenSizeChanged() {
}

// VERIFY: 42d3a2b3 auto
// OFFSET: 0x0098E850
void CSimpleRegion::SetVertexColorRGBA(float r, float g, float b, float a) {
    CImVector color;
    color.Set(a, r, g, b);
    this->SetVertexColor(color);
}

// VERIFY: 21e61798 auto
// OFFSET: 0x0048B800
C3Vector& ApplyTranslationAnim(C3Vector& position, const C2Vector& translation) {
    position.x = position.x + translation.x;
    position.y = translation.y + position.y;

    return position;
}

// VERIFY: ea040a81 auto
// OFFSET: 0x0048B7B0
void ApplyTranslationAnim(C3Vector* positions, const C2Vector& translation) {
    for (uint32_t i = 0; i < 4; i++) {
        positions[i].x = positions[i].x + translation.x;
        positions[i].y = translation.y + positions[i].y;
    }
}

// VERIFY: 2ba548b1 auto
// OFFSET: 0x0048B860
static C2Vector LerpPoint(float t, const C2Vector& a, const C2Vector& b) {
    C2Vector result;
    result.x = (b.x - a.x) * t + a.x;
    result.y = (b.y - a.y) * t + a.y;

    return result;
}

// VERIFY: f5466730 auto
// OFFSET: 0x0048B890
static void GetOriginOffset(FRAMEPOINT origin, const C3Vector* positions, const C2Vector& offset, C2Vector& result) {
    switch (origin) {
        case FRAMEPOINT_TOPLEFT:
            result.x = positions[0].x;
            result.y = positions[0].y;
            break;

        case FRAMEPOINT_TOP:
            result = LerpPoint(0.5f, C2Vector(positions[0]), C2Vector(positions[2]));
            break;

        case FRAMEPOINT_TOPRIGHT:
            result.x = positions[2].x;
            result.y = positions[2].y;
            break;

        case FRAMEPOINT_LEFT:
            result = LerpPoint(0.5f, C2Vector(positions[0]), C2Vector(positions[1]));
            break;

        case FRAMEPOINT_CENTER:
            result = LerpPoint(0.5f, C2Vector(positions[0]), C2Vector(positions[3]));
            break;

        case FRAMEPOINT_RIGHT:
            result = LerpPoint(0.5f, C2Vector(positions[2]), C2Vector(positions[3]));
            break;

        case FRAMEPOINT_BOTTOMLEFT:
            result.x = positions[1].x;
            result.y = positions[1].y;
            break;

        case FRAMEPOINT_BOTTOM:
            result = LerpPoint(0.5f, C2Vector(positions[1]), C2Vector(positions[3]));
            break;

        case FRAMEPOINT_BOTTOMRIGHT:
            result.x = positions[3].x;
            result.y = positions[3].y;
            break;

        default:
            result.x = 0.0f;
            result.y = 0.0f;
            break;
    }

    if (offset.x * offset.x + offset.y * offset.y > 0.00000011920929f) {
        C3Vector edge;
        edge.x = positions[3].x - positions[1].x;
        edge.y = positions[3].y - positions[1].y;
        edge.z = positions[3].z - positions[1].z;
        C2Vector direction(edge);
        float length = direction.y * direction.y + direction.x * direction.x;

        if (length > 0.00000023841858f) {
            float scale = 1.0f / sqrt(length);
            direction.x = direction.x * scale;
            direction.y = direction.y * scale;
        }

        result.x = offset.x * direction.x - offset.y * direction.y + result.x;
        result.y = offset.y * direction.x + offset.x * direction.y + result.y;
    }
}

// VERIFY: db497aa8 auto
// OFFSET: 0x0048BA80
void ApplyRotationAnim(C3Vector* positions, FRAMEPOINT origin, const C2Vector& offset, float radians) {
    C2Vector center;
    GetOriginOffset(origin, positions, offset, center);

    float c = cos(radians);
    float s = sin(radians);

    for (uint32_t i = 0; i < 4; i++) {
        float x = positions[i].x - center.x;
        float y = positions[i].y - center.y;
        positions[i].x = x * c - y * s + center.x;
        positions[i].y = x * s + y * c + center.y;
    }
}

// VERIFY: 3aedd906 auto
// OFFSET: 0x0048BB80
void ApplyScaleAnim(C3Vector* positions, FRAMEPOINT origin, const C2Vector& offset, const C2Vector& scale) {
    C2Vector center;
    GetOriginOffset(origin, positions, offset, center);

    for (uint32_t i = 0; i < 4; i++) {
        positions[i].x = (center.x - positions[i].x) * scale.x + positions[i].x;
        positions[i].y = (center.y - positions[i].y) * scale.y + positions[i].y;
    }
}

// VERIFY: 14ba8493 auto
// OFFSET: 0x004814F0
void CSimpleRegion::PreLoadXML(XMLNode* node, CStatus* status) {
    const char* name = node->GetAttributeByName("name");

    if (name && *name) {
        this->SetName(name);
    }
}
