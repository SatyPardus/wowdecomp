#ifndef ENGINE_SOURCE_FRAME_CSIMPLEREGION_H
#define ENGINE_SOURCE_FRAME_CSIMPLEREGION_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CImVector.h"

class C2Vector;
class C3Vector;
class CRenderBatch;
class CSimpleAnimGroup;
class CSimpleFrame;

class CSimpleRegion : public CScriptRegion {
    public:
    /* 00A0 */ uint32_t m_alphaCount;
    /* 00A4 */ uint8_t m_alphas[4];
    /* 00A8 */ uint32_t m_colorCount;
    /* 00AC */ CImVector m_colors[4];
    /* 00BC */ TSLink<CSimpleRegion> m_regionLink;
    /* 00C4 */ TSLink<CSimpleRegion> m_layerLink;
    /* 00CC */ uint8_t m_drawLayer : 4;
    /* 00CC */ uint8_t m_shown : 1;
    /* 00CC */ uint8_t m_visible : 1;
    /* 00CC */ uint8_t m_animating : 1;
    /* 00CC */ uint8_t m_animAlpha : 1;
    /* 00CD */ uint8_t m_animSavedAlpha;
    /* 00CE */ uint8_t unused_00CE[2];
    /* 00D0 */ uint32_t unused_00D0;

    virtual ~CSimpleRegion();
    virtual void SetParent(CSimpleFrame* parent);
    virtual void AnimActivated(CSimpleAnimGroup* group, int32_t first, int32_t hasAlpha);
    virtual void AnimDeactivated(CSimpleAnimGroup* group, int32_t last);
    virtual void AddAnimAlpha(int16_t alpha);
    virtual int32_t CanBeAnchorFor(CLayoutFrame* frame);
    virtual void OnColorChanged(bool changed);
    virtual void OnScreenSizeChanged();
    virtual void Draw(CRenderBatch* batch) = 0;
    virtual void StoreAnimPosition();
    virtual void ResetAnimPosition();

    CSimpleRegion(CSimpleFrame* frame, uint32_t drawLayer, int32_t show);
    uint8_t GetVertexAlpha();
    void GetVertexColor(CImVector& color);
    void Hide();
    void HideThis();
    void NotifyDelayedLoad();
    void OnRegionChanged();
    void PreLoadXML(XMLNode* node, CStatus* status);
    void SetFrame(CSimpleFrame* frame, uint32_t drawLayer, int32_t show);
    void SetVertexAlpha(uint8_t alpha);
    void SetVertexColor(const CImVector& color);
    void SetVertexColorRGBA(float r, float g, float b, float a);
    void SetVertexGradient(ORIENTATION orientation, const CImVector& minColor, const CImVector& maxColor);
    void ShowThis();
};

void ApplyRotationAnim(C3Vector* positions, FRAMEPOINT origin, const C2Vector& offset, float radians);
void ApplyScaleAnim(C3Vector* positions, FRAMEPOINT origin, const C2Vector& offset, const C2Vector& scale);
void ApplyTranslationAnim(C3Vector* positions, const C2Vector& translation);
C3Vector& ApplyTranslationAnim(C3Vector& position, const C2Vector& translation);

#endif
