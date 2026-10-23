#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <typeinfo>
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CSimpleAnimGroup.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00A43F38
static const XMLENUMVALUE s_frameStrata[] = {
    { FRAME_STRATA_BACKGROUND, "BACKGROUND" },
    { FRAME_STRATA_LOW, "LOW" },
    { FRAME_STRATA_MEDIUM, "MEDIUM" },
    { FRAME_STRATA_HIGH, "HIGH" },
    { FRAME_STRATA_DIALOG, "DIALOG" },
    { FRAME_STRATA_FULLSCREEN, "FULLSCREEN" },
    { FRAME_STRATA_FULLSCREEN_DIALOG, "FULLSCREEN_DIALOG" },
    { FRAME_STRATA_TOOLTIP, "TOOLTIP" },
};

// STATIC: 0x00A43F78
static const XMLENUMVALUE s_framePoints[] = {
    { FRAMEPOINT_BOTTOM, "BOTTOM" },
    { FRAMEPOINT_BOTTOMLEFT, "BOTTOMLEFT" },
    { FRAMEPOINT_BOTTOMRIGHT, "BOTTOMRIGHT" },
    { FRAMEPOINT_CENTER, "CENTER" },
    { FRAMEPOINT_TOP, "TOP" },
    { FRAMEPOINT_TOPRIGHT, "TOPRIGHT" },
    { FRAMEPOINT_TOPLEFT, "TOPLEFT" },
    { FRAMEPOINT_LEFT, "LEFT" },
    { FRAMEPOINT_RIGHT, "RIGHT" },
};

// STATIC: 0x00A43FC0
static const XMLENUMVALUE s_drawLayers[] = {
    { DRAWLAYER_BACKGROUND, "BACKGROUND" },
    { DRAWLAYER_BACKGROUND_BORDER, "BORDER" },
    { DRAWLAYER_ARTWORK, "ARTWORK" },
    { DRAWLAYER_ARTWORK_OVERLAY, "OVERLAY" },
    { DRAWLAYER_HIGHLIGHT, "HIGHLIGHT" },
};

// STATIC: 0x00A43FE8
static const XMLENUMVALUE s_blendModes[] = {
    { GxBlend_Opaque, "DISABLE" },
    { GxBlend_Alpha, "BLEND" },
    { GxBlend_AlphaKey, "ALPHAKEY" },
    { GxBlend_Add, "ADD" },
    { GxBlend_Mod, "MOD" },
};

// STATIC: 0x00A44010
static const XMLENUMVALUE s_justify[] = {
    { 0x1, "LEFT" },
    { 0x2, "CENTER" },
    { 0x4, "RIGHT" },
    { 0x8, "TOP" },
    { 0x10, "MIDDLE" },
    { 0x20, "BOTTOM" },
};

// STATIC: 0x00A44040
static const XMLENUMVALUE s_orientations[] = {
    { ORIENTATION_HORIZONTAL, "HORIZONTAL" },
    { ORIENTATION_VERTICAL, "VERTICAL" },
};

// STATIC: 0x00A44050
static const XMLENUMVALUE s_fontFlags[] = {
    { 0x1, "OUTLINE" },
    { 0x4, "THICKOUTLINE" },
    { 0x2, "MONOCHROME" },
};

// STATIC: 0x00A44068
static const SMOOTHWEIGHTS s_smoothWeights[] = {
    { 0.0f, 0.0f, "NONE" },
    { 1.0f, 0.0f, "IN" },
    { 0.0f, 1.0f, "OUT" },
    { 1.0f, 1.0f, "IN_OUT" },
    { 1.0f, 1.0f, "OUT_IN" },
};

// STATIC: 0x00A440A4
static const XMLENUMVALUE s_loopTypes[] = {
    { ANIM_LOOPTYPE_NONE, "NONE" },
    { ANIM_LOOPTYPE_REPEAT, "REPEAT" },
    { ANIM_LOOPTYPE_BOUNCE, "BOUNCE" },
};

// STATIC: 0x00A440BC
static const XMLENUMVALUE s_loopStates[] = {
    { ANIM_LOOPSTATE_NONE, "NONE" },
    { ANIM_LOOPSTATE_FORWARD, "FORWARD" },
    { ANIM_LOOPSTATE_REVERSE, "REVERSE" },
};

// STATIC: 0x00A440D4
static const XMLENUMVALUE s_curveTypes[] = {
    { ANIM_CURVE_NONE, "NONE" },
    { ANIM_CURVE_SMOOTH, "SMOOTH" },
};

// STATIC: 0x00A4421C
static const uint8_t s_signaturePublicExponent[4] = { 0x01, 0x00, 0x01, 0x00 };

// STATIC: 0x00D3F730
static char s_fontFlagsString[64];

// VERIFY: d37bc6e0 auto
// OFFSET: 0x00814E40
int32_t StringToFrameStrata(const char* string, FRAME_STRATA& strata) {
    for (uint32_t i = 0; i < sizeof(s_frameStrata) / sizeof(s_frameStrata[0]); i++) {
        if (!SStrCmpI(s_frameStrata[i].m_name, string, STORM_MAX_STR)) {
            strata = static_cast<FRAME_STRATA>(s_frameStrata[i].m_value);
            return 1;
        }
    }

    return 0;
}

// VERIFY: c7635fb9 auto
// OFFSET: 0x00814E90
const char* FrameStrataToString(FRAME_STRATA strata) {
    for (uint32_t i = 0; i < sizeof(s_frameStrata) / sizeof(s_frameStrata[0]); i++) {
        if (strata == s_frameStrata[i].m_value) {
            return s_frameStrata[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 96ee6114 auto
// OFFSET: 0x00814ED0
int32_t StringToFramePoint(const char* string, FRAMEPOINT& point) {
    for (uint32_t i = 0; i < sizeof(s_framePoints) / sizeof(s_framePoints[0]); i++) {
        if (!SStrCmpI(s_framePoints[i].m_name, string, STORM_MAX_STR)) {
            point = static_cast<FRAMEPOINT>(s_framePoints[i].m_value);
            return 1;
        }
    }

    return 0;
}

// VERIFY: 305511b6 auto
// OFFSET: 0x00814F20
const char* FramePointToString(FRAMEPOINT point) {
    for (uint32_t i = 0; i < sizeof(s_framePoints) / sizeof(s_framePoints[0]); i++) {
        if (point == s_framePoints[i].m_value) {
            return s_framePoints[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 8a25e9ea auto
// OFFSET: 0x00814F60
int32_t StringToDrawLayer(const char* string, uint32_t& layer) {
    for (uint32_t i = 0; i < sizeof(s_drawLayers) / sizeof(s_drawLayers[0]); i++) {
        if (!SStrCmpI(s_drawLayers[i].m_name, string, STORM_MAX_STR)) {
            layer = s_drawLayers[i].m_value;
            return 1;
        }
    }

    return 0;
}

// VERIFY: ab06cece auto
// OFFSET: 0x00814FB0
const char* DrawLayerToString(uint32_t layer) {
    for (uint32_t i = 0; i < sizeof(s_drawLayers) / sizeof(s_drawLayers[0]); i++) {
        if (layer == s_drawLayers[i].m_value) {
            return s_drawLayers[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: d7a0825b auto
// OFFSET: 0x00814FF0
int32_t StringToBlendMode(const char* string, EGxBlend& blend) {
    for (uint32_t i = 0; i < sizeof(s_blendModes) / sizeof(s_blendModes[0]); i++) {
        if (!SStrCmpI(s_blendModes[i].m_name, string, STORM_MAX_STR)) {
            blend = static_cast<EGxBlend>(s_blendModes[i].m_value);
            return 1;
        }
    }

    return 0;
}

// VERIFY: 06e3b9e5 auto
// OFFSET: 0x00815040
const char* BlendModeToString(EGxBlend blend) {
    for (uint32_t i = 0; i < sizeof(s_blendModes) / sizeof(s_blendModes[0]); i++) {
        if (blend == s_blendModes[i].m_value) {
            return s_blendModes[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 987b1717 auto
// OFFSET: 0x00815080
int32_t StringToJustify(const char* string, uint32_t& justify) {
    for (uint32_t i = 0; i < sizeof(s_justify) / sizeof(s_justify[0]); i++) {
        if (!SStrCmpI(s_justify[i].m_name, string, STORM_MAX_STR)) {
            justify = s_justify[i].m_value;
            return 1;
        }
    }

    return 0;
}

// VERIFY: f9e9e7b3 auto
// OFFSET: 0x008150D0
const char* JustifyToString(uint32_t justify) {
    for (uint32_t i = 0; i < sizeof(s_justify) / sizeof(s_justify[0]); i++) {
        if (justify & s_justify[i].m_value) {
            return s_justify[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: c9627a19 auto
// OFFSET: 0x00815110
int32_t StringToOrientation(const char* string, ORIENTATION& orientation) {
    for (uint32_t i = 0; i < sizeof(s_orientations) / sizeof(s_orientations[0]); i++) {
        if (!SStrCmpI(s_orientations[i].m_name, string, STORM_MAX_STR)) {
            orientation = static_cast<ORIENTATION>(s_orientations[i].m_value);
            return 1;
        }
    }

    return 0;
}

// VERIFY: 37b9ac8c auto
// OFFSET: 0x00815160
const char* OrientationToString(ORIENTATION orientation) {
    for (uint32_t i = 0; i < sizeof(s_orientations) / sizeof(s_orientations[0]); i++) {
        if (orientation == s_orientations[i].m_value) {
            return s_orientations[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 45fda266 auto
// OFFSET: 0x008151A0
uint32_t StringToFontFlags(const char* string) {
    uint32_t flags = 0;

    for (uint32_t i = 0; i < sizeof(s_fontFlags) / sizeof(s_fontFlags[0]); i++) {
        if (SStrStrI(string, s_fontFlags[i].m_name)) {
            flags |= s_fontFlags[i].m_value;
        }
    }

    return flags;
}

// VERIFY: 9e674fdc auto
// OFFSET: 0x008151E0
const char* FontFlagsToString(uint32_t flags) {
    s_fontFlagsString[0] = '\0';

    for (uint32_t i = 0; i < sizeof(s_fontFlags) / sizeof(s_fontFlags[0]); i++) {
        if (flags & s_fontFlags[i].m_value) {
            if (s_fontFlagsString[0]) {
                SStrPack(s_fontFlagsString, ", ", sizeof(s_fontFlagsString));
            }

            SStrPack(s_fontFlagsString, s_fontFlags[i].m_name, sizeof(s_fontFlagsString));
        }
    }

    return s_fontFlagsString;
}

// VERIFY: 580b9450 manual
// OFFSET: 0x00815240
int32_t StringToSmoothWeights(const char* string, float& in, float& out) {
    for (uint32_t i = 0; i < sizeof(s_smoothWeights) / sizeof(s_smoothWeights[0]); i++) {
        if (!SStrCmpI(s_smoothWeights[i].m_name, string, STORM_MAX_STR)) {
            in = s_smoothWeights[i].m_in;
            out = s_smoothWeights[i].m_out;
            return 1;
        }
    }

    return 0;
}

// VERIFY: bd8cb02c auto
// OFFSET: 0x008152A0
int32_t StringToLoopType(const char* string, SimpleAnimLoopType& loop) {
    for (uint32_t i = 0; i < sizeof(s_loopTypes) / sizeof(s_loopTypes[0]); i++) {
        if (!SStrCmpI(s_loopTypes[i].m_name, string, STORM_MAX_STR)) {
            loop = static_cast<SimpleAnimLoopType>(s_loopTypes[i].m_value);
            return 1;
        }
    }

    return 0;
}

// VERIFY: bb276483 auto
// OFFSET: 0x008152F0
const char* LoopTypeToString(SimpleAnimLoopType loop) {
    for (uint32_t i = 0; i < sizeof(s_loopTypes) / sizeof(s_loopTypes[0]); i++) {
        if (loop == s_loopTypes[i].m_value) {
            return s_loopTypes[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 0e8c8acf auto
// OFFSET: 0x00815330
const char* LoopStateToString(SimpleAnimLoopState state) {
    for (uint32_t i = 0; i < sizeof(s_loopStates) / sizeof(s_loopStates[0]); i++) {
        if (state == s_loopStates[i].m_value) {
            return s_loopStates[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 5a457d8a auto
// OFFSET: 0x00815370
int32_t StringToCurveType(const char* string, SimpleAnimCurveType& curve) {
    for (uint32_t i = 0; i < sizeof(s_curveTypes) / sizeof(s_curveTypes[0]); i++) {
        if (!SStrCmpI(s_curveTypes[i].m_name, string, STORM_MAX_STR)) {
            curve = static_cast<SimpleAnimCurveType>(s_curveTypes[i].m_value);
            return 1;
        }
    }

    return 0;
}

// VERIFY: 27176859 auto
// OFFSET: 0x008153C0
const char* CurveTypeToString(SimpleAnimCurveType curve) {
    for (uint32_t i = 0; i < sizeof(s_curveTypes) / sizeof(s_curveTypes[0]); i++) {
        if (curve == s_curveTypes[i].m_value) {
            return s_curveTypes[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 89581c53 auto
// OFFSET: 0x00815400
static int32_t StringToBOOL(const char* string, int32_t defaultValue) {
    if (!string) {
        return defaultValue;
    }

    switch (*string) {
        case '0':
        case 'F':
        case 'N':
        case 'f':
        case 'n':
            return 0;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'T':
        case 'Y':
        case 't':
        case 'y':
            return 1;
    }

    if (!SStrCmpI(string, "off", STORM_MAX_STR) || !SStrCmpI(string, "disabled", STORM_MAX_STR)) {
        return 0;
    }

    if (!SStrCmpI(string, "on", STORM_MAX_STR) || !SStrCmpI(string, "enabled", STORM_MAX_STR)) {
        return 1;
    }

    return defaultValue;
}

// VERIFY: e0b63df6 auto
// OFFSET: 0x008154E0
int32_t StringToBOOL(const char* string) {
    return StringToBOOL(string, 0);
}

// VERIFY: ed82a3ea auto
// OFFSET: 0x00815500
int32_t StringToBOOL(lua_State* L, int32_t index, int32_t defaultValue) {
    switch (lua_type(L, index)) {
        case LUA_TNIL:
            return 0;

        case LUA_TBOOLEAN:
            return lua_toboolean(L, index);

        case LUA_TNUMBER:
            return static_cast<int32_t>(lua_tonumber(L, index)) != 0;

        case LUA_TSTRING:
            return StringToBOOL(lua_tolstring(L, index, nullptr), defaultValue);

        default:
            return defaultValue;
    }
}

// VERIFY: 41e6d008 auto
// OFFSET: 0x00815590
const char* SmoothWeightsToString(float in, float out) {
    for (uint32_t i = 0; i < sizeof(s_smoothWeights) / sizeof(s_smoothWeights[0]); i++) {
        if (fabs(in - s_smoothWeights[i].m_in) < 0.001f && fabs(out - s_smoothWeights[i].m_out) < 0.001f) {
            return s_smoothWeights[i].m_name;
        }
    }

    return "UNKNOWN";
}

// VERIFY: 785739a1 auto
// OFFSET: 0x00815600
int32_t LoadXML_Value(XMLNode* node, float& value, CStatus* status) {
    value = 0.0f;

    const char* valueString = node->GetAttributeByName("val");

    if (valueString && *valueString) {
        float ndc = SStrToFloat(valueString) / (CoordinateGetAspectCompensation() * 1024.0f);
        value = NDCToDDCWidth(ndc);
    }

    XMLNode* child = node->m_child;

    if (!child) {
        if (!valueString) {
            status->Add(STATUS_WARNING, "No \"val\" attribute in element: %s", node->m_name.m_str);
            return 0;
        }

        return 1;
    }

    if (!SStrCmpI(child->m_name.m_str, "AbsValue", STORM_MAX_STR)) {
        const char* absString = child->GetAttributeByName("val");

        if (absString && *absString) {
            float ndc = SStrToFloat(absString) / (CoordinateGetAspectCompensation() * 1024.0f);
            value = NDCToDDCWidth(ndc);
        }

        return 1;
    }

    if (!SStrCmpI(child->m_name.m_str, "RelValue", STORM_MAX_STR)) {
        const char* relString = child->GetAttributeByName("val");

        if (relString && *relString) {
            value = SStrToFloat(relString);
        }

        return 1;
    }

    status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->m_name.m_str, child->m_name.m_str);

    return 0;
}

// VERIFY: 4b656605 auto
// OFFSET: 0x00815740
int32_t LoadXML_Dimensions(XMLNode* node, float& x, float& y, CStatus* status) {
    x = 0.0f;
    y = 0.0f;

    const char* xString = node->GetAttributeByName("x");

    if (xString && *xString) {
        float ndc = SStrToFloat(xString) / (CoordinateGetAspectCompensation() * 1024.0f);
        x = NDCToDDCWidth(ndc);
    }

    const char* yString = node->GetAttributeByName("y");

    if (yString && *yString) {
        float ndc = SStrToFloat(yString) / (CoordinateGetAspectCompensation() * 1024.0f);
        y = NDCToDDCWidth(ndc);
    }

    XMLNode* child = node->m_child;

    if (!child) {
        return yString != nullptr;
    }

    if (!SStrCmpI(child->m_name.m_str, "AbsDimension", STORM_MAX_STR)) {
        const char* absX = child->GetAttributeByName("x");

        if (absX && *absX) {
            float ndc = SStrToFloat(absX) / (CoordinateGetAspectCompensation() * 1024.0f);
            x = NDCToDDCWidth(ndc);
        }

        const char* absY = child->GetAttributeByName("y");

        if (absY && *absY) {
            float ndc = SStrToFloat(absY) / (CoordinateGetAspectCompensation() * 1024.0f);
            y = NDCToDDCWidth(ndc);
        }

        return 1;
    }

    if (!SStrCmpI(child->m_name.m_str, "RelDimension", STORM_MAX_STR)) {
        const char* relX = child->GetAttributeByName("x");

        if (relX && *relX) {
            x = SStrToFloat(relX);
        }

        const char* relY = child->GetAttributeByName("y");

        if (relY && *relY) {
            y = SStrToFloat(relY);
        }

        return 1;
    }

    status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->m_name.m_str, child->m_name.m_str);

    return 0;
}

// VERIFY: 60497307 auto
// OFFSET: 0x00815910
int32_t LoadXML_Insets(XMLNode* node, float& left, float& right, float& top, float& bottom, CStatus* status) {
    left = 0.0f;
    right = 0.0f;
    top = 0.0f;
    bottom = 0.0f;

    const char* leftString = node->GetAttributeByName("left");

    if (leftString && *leftString) {
        float ndc = SStrToFloat(leftString) / (CoordinateGetAspectCompensation() * 1024.0f);
        left = NDCToDDCWidth(ndc);
    }

    const char* rightString = node->GetAttributeByName("right");

    if (rightString && *rightString) {
        float ndc = SStrToFloat(rightString) / (CoordinateGetAspectCompensation() * 1024.0f);
        right = NDCToDDCWidth(ndc);
    }

    const char* topString = node->GetAttributeByName("top");

    if (topString && *topString) {
        float ndc = SStrToFloat(topString) / (CoordinateGetAspectCompensation() * 1024.0f);
        top = NDCToDDCWidth(ndc);
    }

    const char* bottomString = node->GetAttributeByName("bottom");

    if (bottomString && *bottomString) {
        float ndc = SStrToFloat(bottomString) / (CoordinateGetAspectCompensation() * 1024.0f);
        bottom = NDCToDDCWidth(ndc);
    }

    XMLNode* child = node->m_child;

    if (!child) {
        if (!bottomString) {
            status->Add(STATUS_WARNING, "No \"right\", \"left\", \"top\", or \"bottom\" attributes in element: %s", node->m_name.m_str);
            return 0;
        }

        return 1;
    }

    if (!SStrCmpI(child->m_name.m_str, "AbsInset", STORM_MAX_STR)) {
        const char* absRight = child->GetAttributeByName("right");

        if (absRight && *absRight) {
            float ndc = SStrToFloat(absRight) / (CoordinateGetAspectCompensation() * 1024.0f);
            right = NDCToDDCWidth(ndc);
        }

        const char* absLeft = child->GetAttributeByName("left");

        if (absLeft && *absLeft) {
            float ndc = SStrToFloat(absLeft) / (CoordinateGetAspectCompensation() * 1024.0f);
            left = NDCToDDCWidth(ndc);
        }

        const char* absTop = child->GetAttributeByName("top");

        if (absTop && *absTop) {
            float ndc = SStrToFloat(absTop) / (CoordinateGetAspectCompensation() * 1024.0f);
            top = NDCToDDCWidth(ndc);
        }

        const char* absBottom = child->GetAttributeByName("bottom");

        if (absBottom && *absBottom) {
            float ndc = SStrToFloat(absBottom) / (CoordinateGetAspectCompensation() * 1024.0f);
            bottom = NDCToDDCWidth(ndc);
        }

        return 1;
    }

    if (!SStrCmpI(child->m_name.m_str, "RelInset", STORM_MAX_STR)) {
        const char* relRight = child->GetAttributeByName("right");

        if (relRight && *relRight) {
            right = SStrToFloat(relRight);
        }

        const char* relLeft = child->GetAttributeByName("left");

        if (relLeft && *relLeft) {
            left = SStrToFloat(relLeft);
        }

        const char* relTop = child->GetAttributeByName("top");

        if (relTop && *relTop) {
            top = SStrToFloat(relTop);
        }

        const char* relBottom = child->GetAttributeByName("bottom");

        if (relBottom && *relBottom) {
            bottom = SStrToFloat(relBottom);
        }

        return 1;
    }

    status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->m_name.m_str, child->m_name.m_str);

    return 0;
}

// VERIFY: e06fe5f5 auto
// OFFSET: 0x00815C30
int32_t LoadXML_Color(XMLNode* node, CImVector& color, CStatus* status) {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    const char* rString = node->GetAttributeByName("r");

    if (rString && *rString) {
        r = (SStrToFloat(rString) < 1.0f && !(SStrToFloat(rString) > 0.0f)) ? 0.0f : (SStrToFloat(rString) >= 1.0f ? 1.0f : SStrToFloat(rString));
    }

    const char* gString = node->GetAttributeByName("g");

    if (gString && *gString) {
        g = (SStrToFloat(gString) < 1.0f && !(SStrToFloat(gString) > 0.0f)) ? 0.0f : (SStrToFloat(gString) >= 1.0f ? 1.0f : SStrToFloat(gString));
    }

    const char* bString = node->GetAttributeByName("b");

    if (bString && *bString) {
        b = (SStrToFloat(bString) < 1.0f && !(SStrToFloat(bString) > 0.0f)) ? 0.0f : (SStrToFloat(bString) >= 1.0f ? 1.0f : SStrToFloat(bString));
    }

    const char* aString = node->GetAttributeByName("a");

    if (aString && *aString) {
        a = (SStrToFloat(aString) < 1.0f && !(SStrToFloat(aString) > 0.0f)) ? 0.0f : (SStrToFloat(aString) >= 1.0f ? 1.0f : SStrToFloat(aString));
    }

    color.Set(a, r, g, b);

    return 1;
}

// VERIFY: 91664f60 auto
// OFFSET: 0x00815DF0
int32_t LoadXML_Gradient(XMLNode* node, ORIENTATION& orientation, CImVector& minColor, CImVector& maxColor, CStatus* status) {
    orientation = ORIENTATION_HORIZONTAL;

    const char* orientationString = node->GetAttributeByName("orientation");

    if (orientationString && *orientationString && !StringToOrientation(orientationString, orientation)) {
        status->Add(STATUS_WARNING, "Unknown orientation %s in element %s", orientationString, node->m_name.m_str);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "MinColor", STORM_MAX_STR)) {
            LoadXML_Color(child, minColor, status);
        }

        if (!SStrCmpI(child->m_name.m_str, "MaxColor", STORM_MAX_STR)) {
            LoadXML_Color(child, maxColor, status);
        }
    }

    return 1;
}

// VERIFY: 4bd552ef manual
// OFFSET: 0x00815EB0
int32_t LoadXML_AnimOrigin(XMLNode* node, FRAMEPOINT& point, C2Vector& offset, CStatus* status) {
    point = FRAMEPOINT_CENTER;
    offset.x = 0.0f;
    offset.y = 0.0f;

    const char* pointString = node->GetAttributeByName("point");

    if (pointString && *pointString) {
        if (!StringToFramePoint(pointString, point)) {
            status->Add(STATUS_WARNING, "Invalid origin point %s in element %s", pointString, node->m_name.m_str);
        }

        XMLNode* offsetNode = node->GetChildByName("Offset");

        if (offsetNode) {
            LoadXML_Dimensions(offsetNode, offset.x, offset.y, status);
        }
    }

    return 1;
}

// VERIFY: dea072e9 auto
// OFFSET: 0x00815F40
CSimpleTexture* LoadXML_Texture(XMLNode* node, CSimpleFrame* frame, CStatus* status) {
    void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 2, 1) : nullptr;

    texture->PreLoadXML(node, status);
    texture->LoadXML(node, status);
    texture->PostLoadXML(node, status);

    return texture;
}

// VERIFY: 437bdf6e auto
// OFFSET: 0x00815FA0
CSimpleFontString* LoadXML_String(XMLNode* node, CSimpleFrame* frame, CStatus* status) {
    void* data = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
    CSimpleFontString* string = data ? new (data) CSimpleFontString(frame, 2, true) : nullptr;

    string->PreLoadXML(node, status);
    string->LoadXML(node, status);
    string->PostLoadXML(node, status);

    return string;
}

// VERIFY: 8282815a manual
// OFFSET: 0x00816000
CSimpleAnimGroup* LoadXML_AnimGroup(XMLNode* node, CScriptRegion* region, CStatus* status) {
    CSimpleAnimGroup* group = new CSimpleAnimGroup(region);

    group->PreLoadXML(node, status);
    group->LoadXML(node, status);
    group->PostLoadXML(node, status);

    return group;
}

// VERIFY: 644cec26 auto
// OFFSET: 0x00816060
static void ProcessTocForMakeSign(MD5_CTX* md5, const char* text, const char* basePath) {
    const char* current = text;
    char line[1024];

    do {
        SStrTokenize(&current, line, sizeof(line), "\r\n", nullptr);

        if (!line[0]) {
            break;
        }

        if (line[0] != '#') {
            ProcessFileForMakeSign(md5, line, basePath);
        }
    } while (line[0] && *current);
}

// VERIFY: 0877ed9a manual
// OFFSET: 0x008160D0
static void ParseManifestTag(char* tag, int32_t length, MD5_CTX* md5, const char* basePath) {
    char* end = tag + length;
    char* name = tag;

    while ((*name == '\r' || *name == '\n' || *name == ' ' || *name == '\t') && name < end) {
        name++;
    }

    if (name == end) {
        return;
    }

    char* nameEnd = name;

    while (*nameEnd != '\r' && *nameEnd != '\n' && *nameEnd != ' ' && *nameEnd != '\t' && nameEnd < end) {
        nameEnd++;
    }

    if (nameEnd == end) {
        return;
    }

    *nameEnd = '\0';

    if (SStrCmpI(name, "Script", STORM_MAX_STR) && SStrCmpI(name, "Include", STORM_MAX_STR)) {
        return;
    }

    char* attribute = name;
    char* attributeEnd = nameEnd;

    while (attribute < end) {
        attribute = attributeEnd + 1;

        while ((*attribute == '\r' || *attribute == '\n' || *attribute == ' ' || *attribute == '\t') && attribute < end) {
            attribute++;
        }

        if (attribute == end) {
            return;
        }

        attributeEnd = attribute;

        while (*attributeEnd != '\r' && *attributeEnd != '\n' && *attributeEnd != ' ' && *attributeEnd != '\t' && *attributeEnd != '=' && attributeEnd < end) {
            attributeEnd++;
        }

        if (attributeEnd == end) {
            return;
        }

        *attributeEnd = '\0';

        if (!SStrCmpI(attribute, "file", STORM_MAX_STR)) {
            break;
        }
    }

    if (attribute == end) {
        return;
    }

    char* value = attributeEnd + 1;

    while ((*value == '\r' || *value == '\n' || *value == ' ' || *value == '\t' || *value == '=' || *value == '"') && value < end) {
        value++;
    }

    if (value == end) {
        return;
    }

    char* valueEnd = value;

    while (*valueEnd != '\r' && *valueEnd != '\n' && *valueEnd != '\t' && *valueEnd != '"' && *valueEnd != '/' && valueEnd < end) {
        valueEnd++;
    }

    *valueEnd = '\0';

    ProcessFileForMakeSign(md5, value, basePath);
}

// VERIFY: 65239f1e auto
// OFFSET: 0x00816240
static void ProcessXMLForMakeSign(char* text, uint32_t size, MD5_CTX* md5, const char* basePath) {
    char* end = text + size;
    char* current = text;

    while (*current != '<' && current < end) {
        current++;
    }

    char* tag = current + 1;
    char* cursor = tag;

    while (cursor < end) {
        if (*cursor == '>') {
            *cursor = '\0';
            ParseManifestTag(tag, cursor - tag, md5, basePath);

            char* next = cursor + 1;

            while (*next != '<' && next < end) {
                next++;
            }

            tag = next + 1;
            cursor = tag;
        } else {
            cursor++;
        }
    }
}

// VERIFY: 5341ffcf manual
// OFFSET: 0x008162B0
int32_t ProcessFileForMakeSign(MD5_CTX* md5, const char* filename, const char* basePath) {
    char path[260];
    SStrPrintf(path, sizeof(path), "%s%s", basePath, filename);

    char* end = &path[SStrLen(path)];

    while (end[-1] == ' ') {
        if (end <= path) {
            break;
        }

        end--;
    }

    *end = '\0';

    if (SStrStr(path, "..")) {
        for (char* dots = SStrStr(path, ".."); dots; dots = SStrStr(path, "..")) {
            char before = dots[-1];

            if (before != '\\' && before != '/') {
                break;
            }

            char after = dots[2];

            if (after != '\\' && after != '/') {
                break;
            }

            char* dir = dots - 2;

            while (dir >= path && *dir != '\\' && *dir != '/') {
                dir--;
            }

            SStrCopy(dir + 1, dots + 3, STORM_MAX_STR);
        }
    }

    char directory[260];
    memset(directory, 0, sizeof(directory));

    char* backslash = SStrChrR(path, '\\');
    char* slash = SStrChrR(path, '/');

    if (backslash > slash) {
        slash = backslash;
    }

    if (slash) {
        uint32_t length = slash - path + 2;

        if (length < sizeof(directory)) {
            SStrCopy(directory, path, slash - path + 2);
            directory[length] = '\0';
        }
    }

    void* data;
    uint32_t size;
    int32_t result = SFile::ReadFile(nullptr, path, &data, &size, 1, 1, nullptr);

    if (!result) {
        return result;
    }

    MD5Update(md5, static_cast<const uint8_t*>(data), size);

    const char* extension = SStrChrR(const_cast<char*>(filename), '.');

    if (extension) {
        if (!SStrCmpI(extension, ".toc", STORM_MAX_STR)) {
            ProcessTocForMakeSign(md5, static_cast<const char*>(data), directory);
        } else if (!SStrCmpI(extension, ".xml", STORM_MAX_STR)) {
            ProcessXMLForMakeSign(static_cast<char*>(data), size, md5, directory);
        }
    }

    SFile::Unload(data);

    return 1;
}

// VERIFY: e9b24e51 auto
// OFFSET: 0x008164D0
static void CalculateSignatureMD5(const char* tocFile, const char* extraFile, uint8_t* digest) {
    MD5_CTX context;
    MD5Init(&context);
    ProcessFileForMakeSign(&context, tocFile, "");

    void* data;
    uint32_t size;

    if (extraFile && SFile::ReadFile(nullptr, extraFile, &data, &size, 1, 1, nullptr)) {
        MD5Update(&context, static_cast<const uint8_t*>(data), size);
        SFile::Unload(data);
    }

    MD5Final(digest, &context);
}

// VERIFY: 65235038 auto
// OFFSET: 0x00816550
static int32_t VerifySignature(const uint8_t* data, uint32_t size, const uint8_t* modulus, const char* name) {
    if (!modulus) {
        return 0;
    }

    SSignatureData* token;
    SSignatureVerifyStream_Begin(&token, 256, 4);

    uint32_t signatureLength = SSignatureVerifyStream_GetSignatureLength(token);
    SSignatureVerifyStream_ProvideData(token, data, size - signatureLength);
    SSignatureVerifyStream_ProvideData(token, reinterpret_cast<const uint8_t*>(name), SStrLen(name));
    SSignatureVerifyStream_ProvideData(token, &data[size - signatureLength], signatureLength);

    return SSignatureVerifyStream_Finish(token, modulus, s_signaturePublicExponent);
}

// VERIFY: 29422f35 manual
// OFFSET: 0x008165E0
int32_t FrameXML_CheckSignature(const char* tocFile, const char* extraFile, const uint8_t* modulus, uint8_t* digest) {
#ifdef DISABLE_SIGNATURE_CHECKS
    CalculateSignatureMD5(tocFile, extraFile, digest);
    return 3;
#endif

    char sigFile[260];
    SStrPrintf(sigFile, sizeof(sigFile), "%s.sig", tocFile);

    void* data;
    uint32_t size;

    if (!SFile::ReadFile(nullptr, sigFile, &data, &size, 0, 1, nullptr)) {
        return 0;
    }

    if (size != 276) {
        SFile::Unload(data);
        return 1;
    }

    char* name = SStrChrR(sigFile, '\\');
    name = name ? name + 1 : sigFile;
    SStrUpper(name);

    if (!VerifySignature(static_cast<const uint8_t*>(data), size, modulus, name)) {
        SFile::Unload(data);
        return 1;
    }

    uint8_t signedDigest[16];
    memcpy(signedDigest, data, sizeof(signedDigest));
    SFile::Unload(data);

    CalculateSignatureMD5(tocFile, extraFile, digest);

    return memcmp(signedDigest, digest, sizeof(signedDigest)) ? 2 : 3;
}
