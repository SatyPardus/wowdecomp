#ifndef WOW_SOURCE_UI_NAMEPLATEFRAME_H
#define WOW_SOURCE_UI_NAMEPLATEFRAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Object/WGUID.h"

class CGSimpleHealthBar;
class CGUnit_C;
class CSimpleFontString;
class CSimpleStatusBar;
class CSimpleTexture;

class CGNamePlateFrame : public TSLinkedNode<CGNamePlateFrame>, public CSimpleFrame {
    public:
    static CDataAllocator s_namePlateHeap;

    static void operator delete(void* ptr);
    static CGNamePlateFrame* GetNamePlateFocus();
    static CGNamePlateFrame* GetNewNameplateFrame(CSimpleFrame* parent);

    /* 02A4 */ uint32_t unused_02A4;
    /* 02A8 */ WOWGUID m_guid;
    /* 02B0 */ CSimpleTexture* m_namePlateGlow;
    /* 02B4 */ CSimpleTexture* m_targetingFrameSkull;
    /* 02B8 */ CSimpleTexture* m_raidTargetingIcon;
    /* 02BC */ CSimpleTexture* m_castBarBorder;
    /* 02C0 */ CSimpleTexture* m_castBarShield;
    /* 02C4 */ CSimpleTexture* m_targetingFrameFlash;
    /* 02C8 */ CSimpleTexture* m_eliteNameplateIcon;
    /* 02CC */ CSimpleTexture* m_castBarIcon;
    /* 02D0 */ CSimpleFontString* m_unitNameText;
    /* 02D4 */ CSimpleFontString* m_levelText;
    /* 02D8 */ CGSimpleHealthBar* m_healthBar;
    /* 02DC */ CSimpleStatusBar* m_castBar;
    /* 02E0 */ C3Vector m_position;
    /* 02EC */ CImVector m_healthBarColor;
    /* 02F0 */ int32_t m_damageFeedbackTime;
    /* 02F4 */ uint8_t m_casting;
    /* 02F5 */ uint8_t m_channeling;
    /* 02F8 */ float m_castHoldTime;
    /* 02FC */ float m_castFadeTime;

    ~CGNamePlateFrame() override;
    void OnLayerUpdate(float elapsedSec) override;

    CGNamePlateFrame(CSimpleFrame* parent);
    void HideCastBar();
    void Initialize(CGUnit_C* unit);
    void OnGainFocus();
    void OnLoseFocus();
    void SetCastBarValues(float minValue, float maxValue, float value, int32_t spellId, bool channel, bool notInterruptible);
    void TriggerDamageFeedback();
    void UpdateColorDisplay(CGUnit_C* unit);
    void UpdateEliteIcon(CGUnit_C* unit);
    void UpdateLevelDisplay(CGUnit_C* unit);
    void UpdateNameDisplay(const CGUnit_C* unit);
    void UpdateRaidTargetDisplay(uint32_t raidTarget);
};

#endif
