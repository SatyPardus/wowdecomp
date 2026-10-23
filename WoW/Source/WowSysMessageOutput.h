#ifndef WOW_SOURCE_WOWSYSMESSAGEOUTPUT_H
#define WOW_SOURCE_WOWSYSMESSAGEOUTPUT_H

#include <cstdint>
#include "Engine/Source/Base/Types.h"
#include "Engine/Source/Storm/H/Types.h"
#include "Engine/Source/Tempest/CImVector.h"

struct CGxString;

struct SYSMESSAGE {
    /* 0000 */ uint32_t m_time;
    /* 0004 */ char m_text[256];
    /* 0104 */ CGxString* m_string;
    /* 0108 */ CImVector m_color;
};

class WowSysMessageOutput {
    public:
    static void Disable();
    static void Enable();
    static void Hide();
    static void Initialize();
    static void Output(const char* text, SYSMSG_TYPE severity);
    static void Paint(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec);
    static void Show();
    static void Shutdown();
};

#endif
