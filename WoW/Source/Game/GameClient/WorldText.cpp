#include "WorldText.h"
#include <cmath>
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/CGxVertexPT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Gxu/Types.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/Game/GameClient/WTOBJECT.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/SmartScreenRect.h"
#include "WoW/Source/Ui/WorldFrame.h"

// STATIC: 0x00A41618
static float s_critHeightCurve[3][4] = {
    { 0.0f, 0.1f, 0.1f, 2.0f },
    { 0.1f, 0.2f, 2.0f, 1.0f },
    { 0.2f, 1.0f, 1.0f, 1.0f },
};

// STATIC: 0x00AF4748
static WORLDTEXTTYPE s_worldTextTypes[NUM_WORLDTEXTTYPES] = {
    { 2.0f, 150, 760, 1500, 0.018333333f, 0.018333333f, CImVector(0xFF, 0xFF, 0xFF, 0xFF) },
    { 2.0f, 150, 90, 1500, 0.018333333f, 0.018333333f, CImVector(0xFF, 0xFF, 0xFF, 0xFF) },
    { 0.0f, 150, 1000, 1500, 0.0f, 0.027499998f, CImVector(0xFF, 0xFF, 0xFF, 0xFF) },
    { 2.0f, 150, 1000, 1500, 0.018333333f, 0.018333333f, CImVector(0xFF, 0xFF, 0xFF, 0xFF) },
    { 0.0f, 500, 2000, 4500, 0.018333333f, 0.018333333f, CImVector(0x8B, 0x00, 0x94, 0x80) },
    { 0.0f, 500, 2000, 4500, 0.018333333f, 0.018333333f, CImVector(0x0A, 0xCA, 0xE0, 0xFF) },
    { 2.0f, 150, 760, 1500, 0.018333333f, 0.018333333f, CImVector(0x00, 0xFF, 0x00, 0xFF) },
    { 0.0f, 150, 1000, 1500, 0.0f, 0.027499998f, CImVector(0x00, 0xFF, 0x00, 0xFF) },
    { 6.0f, 150, 760, 1500, 0.027499998f, 0.027499998f, CImVector(0xFF, 0xFF, 0xFF, 0xFF) },
    { 6.0f, 150, 760, 1500, 0.027499998f, 0.027499998f, CImVector(0x00, 0xFF, 0x00, 0xFF) },
    { 2.0f, 150, 760, 1500, 0.018333333f, 0.018333333f, CImVector(0x00, 0x99, 0xFF, 0xFF) },
};

// STATIC: 0x00AF487C
static C2Vector s_shadowOffset = C2Vector(0.002f, 0.002f);

// STATIC: 0x00AF4884
static C3Vector s_iconPositions[4] = {
    C3Vector(0.0f, 1.0f, 0.0f),
    C3Vector(1.0f, 1.0f, 0.0f),
    C3Vector(1.0f, 0.0f, 0.0f),
    C3Vector(0.0f, 0.0f, 0.0f),
};

// STATIC: 0x00AF48B4
static C2Vector s_iconTexCoords[4] = {
    C2Vector(0.0f, 0.0f),
    C2Vector(1.0f, 0.0f),
    C2Vector(1.0f, 1.0f),
    C2Vector(0.0f, 1.0f),
};

// STATIC: 0x00D380C8
static CGxStringBatch* s_worldTextBatch;

// STATIC: 0x00D380CC
static HTEXTFONT s_worldTextFont;

// STATIC: 0x00D380D0
static CTexture* s_pvpRankTextures[15];

// STATIC: 0x00D3810C
static CDataAllocator s_freeStrings(sizeof(WORLDTEXTSTRING), 30);

// STATIC: 0x00D38120
static TSGrowableArray<SWorldTextIcon> s_worldTextIcons;

// STATIC: 0x00AF4708
static const char* s_pvpRankTextureNames[15] = { "Interface\\PvPRankBadges\\PvPRank01.blp", "Interface\\PvPRankBadges\\PvPRank02.blp", "Interface\\PvPRankBadges\\PvPRank03.blp", "Interface\\PvPRankBadges\\PvPRank04.blp", "Interface\\PvPRankBadges\\PvPRank05.blp", "Interface\\PvPRankBadges\\PvPRank06.blp", "Interface\\PvPRankBadges\\PvPRank07.blp", "Interface\\PvPRankBadges\\PvPRank08.blp", "Interface\\PvPRankBadges\\PvPRank09.blp", "Interface\\PvPRankBadges\\PvPRank10.blp", "Interface\\PvPRankBadges\\PvPRank11.blp", "Interface\\PvPRankBadges\\PvPRank12.blp", "Interface\\PvPRankBadges\\PvPRank13.blp", "Interface\\PvPRankBadges\\PvPRank14.blp", "Interface\\PvPRankBadges\\PvPRank15.blp" };

// INLINED
inline WORLDTEXTSTRING::WORLDTEXTSTRING() {
    this->m_type = NUM_WORLDTEXTTYPES;
    this->m_halfWidth = 0.0f;
    this->m_halfHeight = 0.0f;
    this->m_string = nullptr;
    this->m_fontHeight = -1.0f;
    this->m_pvpRank = 0;
    this->m_text[0] = '\0';
}

// VERIFY: bbed0aa5 manual
// OFFSET: 0x007E66B0
WORLDTEXTSTRING::~WORLDTEXTSTRING() {
    if (this->m_string) {
        GxuFontDestroyString(this->m_string);
    }
}

// VERIFY: 9f87a140 auto
// OFFSET: 0x0041D190
uint32_t WorldTextGetType(WORLDTEXTSTRING* text) {
    return text->m_type;
}

// VERIFY: fa6c65c2 manual
// OFFSET: 0x007E6590
float WORLDTEXTSTRING::CalculateAscendOffset(uint32_t elapsed) {
    return static_cast<float>(elapsed) / static_cast<float>(s_worldTextTypes[this->m_type].m_duration) * s_worldTextTypes[this->m_type].m_ascendDistance;
}

// VERIFY: 42cc95bb auto
// OFFSET: 0x007E65E0
float WorldTextGetAscendDistance(uint32_t type) {
    return s_worldTextTypes[type].m_ascendDistance;
}

// VERIFY: 271233c4 auto
// OFFSET: 0x007E66F0
void WORLDTEXTSTRING::RecreateString() {
    if (this->m_string) {
        GxuFontDestroyString(this->m_string);
    }

    this->m_string = nullptr;

    CGxFont* font = TextBlockGetFontPtr(s_worldTextFont);
    float fontHeight = DDCToNDCHeight(this->m_fontHeight);

    C2Vector shadow;
    shadow.x = 0.0f;
    shadow.y = 0.0f;
    DDCToNDC(s_shadowOffset.x, s_shadowOffset.y, &shadow.x, &shadow.y);

    float extent;
    GxuFontGetTextExtent(font, this->m_text, SStrLen(this->m_text), fontHeight, &extent, shadow.x, 1.0f, 0.0f, 0);
    float textHeight = GxuFontGetWrappedTextHeight(font, this->m_text, fontHeight, extent, shadow, 1.0f, 0.0f, 0);

    C3Vector position(0.0f, 0.0f, 1.0f);
    GxuFontCreateString(font, this->m_text, fontHeight, position, extent, textHeight, 0.0f, this->m_string, GxVJ_Bottom, GxHJ_Left, 0, this->m_color, 0.0f, 1.0f);
    GxuFontAddToBatch(s_worldTextBatch, this->m_string);

    this->m_halfWidth = extent * 0.5f;
    this->m_halfHeight = 0.5f * textHeight;
}

// VERIFY: d96a0c47 manual
// OFFSET: 0x007E6850
void WORLDTEXTSTRING::CalculateNewColor(uint32_t elapsed) {
    const WORLDTEXTTYPE& type = s_worldTextTypes[this->m_type];
    uint32_t duration = type.m_duration;
    float progress = static_cast<float>(elapsed) / static_cast<float>(duration);
    uint8_t alpha = 0xFF;
    uint8_t shadowAlpha = 0x7F;

    if (elapsed < type.m_fadeInTime) {
        alpha = 0xFF;

        uint32_t fadeAlpha = static_cast<uint32_t>(255.0f * progress);

        if (fadeAlpha <= 0xFF) {
            alpha = fadeAlpha;
        }

        uint32_t fadeShadow = static_cast<int32_t>(progress * 127.0f);
        shadowAlpha = fadeShadow > 0x7F ? 0x7F : fadeShadow;
    } else if (elapsed >= type.m_fadeOutStart) {
        uint32_t range = duration - type.m_fadeOutStart;
        float fade = range ? static_cast<float>(elapsed - type.m_fadeOutStart) / static_cast<float>(range) : 0.0f;

        float fadeAlpha = fade * 255.0f;

        if (fadeAlpha > 255.0f) {
            fadeAlpha = 255.0f;
        } else if (fadeAlpha < 0.0f) {
            fadeAlpha = 0.0f;
        }

        alpha = static_cast<int32_t>(255.0f - fadeAlpha);

        float fadeShadow = fade * 127.0f;

        if (fadeShadow > 127.0f) {
            fadeShadow = 127.0f;
        } else if (fadeShadow < 0.0f) {
            fadeShadow = 0.0f;
        }

        shadowAlpha = static_cast<int32_t>(255.0f - fadeShadow);
    }

    CImVector shadowColor;
    shadowColor.b = 0;
    shadowColor.g = 0;
    shadowColor.r = 0;
    this->m_color.a = alpha;
    shadowColor.a = shadowAlpha;
    GxuFontSetStringColor(this->m_string, this->m_color);
    GxuFontAddShadow(this->m_string, shadowColor, s_shadowOffset);
}

// VERIFY: 9b0c9bfd auto
// OFFSET: 0x007E6A30
void WorldTextShutdown() {
    if (s_worldTextFont) {
        HandleClose(reinterpret_cast<HOBJECT>(s_worldTextFont));
    }

    s_freeStrings.Clear(typeid(WORLDTEXTSTRING).raw_name(), SLOG_OBJECT, 1);
    GxuFontDestroyBatch(s_worldTextBatch);

    for (uint32_t i = 0; i < 15; i++) {
        if (s_pvpRankTextures[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(s_pvpRankTextures[i]));
            s_pvpRankTextures[i] = nullptr;
        }
    }
}

// VERIFY: 265bbcf3 auto
// OFFSET: 0x007E6A90
void WorldTextDelete(WORLDTEXTSTRING* text) {
    text->~WORLDTEXTSTRING();
    s_freeStrings.PutData(text, nullptr, 0);
}

// VERIFY: 439b797d manual
// OFFSET: 0x007E6AC0
static void WorldTextSetupOrthoProjection() {
    CBoundingBox viewport = g_theGxDevicePtr->m_viewport;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    if (!g_theGxDevicePtr->Caps().m_pixelCenterOnEdge) {
        offsetX = -0.5f;
        offsetY = 0.5f;
    }

    CRect window;
    window.minY = 0.0f;
    window.minX = 0.0f;
    window.maxY = 0.0f;
    window.maxX = 0.0f;
    g_theGxDevicePtr->CapsWindowSize(window);

    uint32_t width = static_cast<uint32_t>(window.maxX - window.minX);
    uint32_t height = static_cast<uint32_t>(window.maxY - window.minY);

    C44Matrix ortho;
    float h = static_cast<float>(height);
    float w = static_cast<float>(width);
    GxuXformCreateOrtho(floor(w * viewport.m_x.m_l) + offsetX, floor(w * viewport.m_x.m_h) + offsetX, floor(h * viewport.m_y.m_l) + offsetY, floor(h * viewport.m_y.m_h) + offsetY, -1.0f, 1.0f, ortho);
    g_theGxDevicePtr->XformSetProjection(ortho);
}

// VERIFY: d685ec68 manual
// OFFSET: 0x007E6CC0
void WORLDTEXTSTRING::CalculateTextHeight(uint32_t elapsed) {
    const WORLDTEXTTYPE& type = s_worldTextTypes[this->m_type];
    float progress = static_cast<float>(elapsed) / static_cast<float>(type.m_duration);
    float height;

    if (this->m_type == WT_CRIT || this->m_type == WT_CRIT_HEAL) {
        float scale = 1.0f;

        for (uint32_t i = 0; i < 3; i++) {
            if (s_critHeightCurve[i][0] <= progress && s_critHeightCurve[i][1] > progress) {
                scale = (progress - s_critHeightCurve[i][0]) / (s_critHeightCurve[i][1] - s_critHeightCurve[i][0]) * (s_critHeightCurve[i][3] - s_critHeightCurve[i][2]) + s_critHeightCurve[i][2];
                break;
            }
        }

        height = scale * type.m_endHeight;
    } else {
        height = progress * (type.m_endHeight - type.m_startHeight) + type.m_startHeight;
    }

    if (height <= 0.001f) {
        height = 0.001f;
    }

    if (this->m_fontHeight != height) {
        this->m_fontHeight = height;
        this->RecreateString();
    }
}

// VERIFY: 9e3ba832 auto
// OFFSET: 0x007E6DC0
WORLDTEXTSTRING* WorldTextCreate(uint32_t type, const C3Vector& position, const char* text, const CImVector* color, const int32_t* pvpRank) {
    if (!text || !*text) {
        return nullptr;
    }

    WORLDTEXTSTRING* string = s_freeStrings.New<WORLDTEXTSTRING>(0);

    if (color) {
        string->m_color = *color;
    } else {
        string->m_color = s_worldTextTypes[type].m_color;
    }

    string->m_position = position;
    string->m_type = type;
    string->m_startTime = OsGetAsyncTimeMs();
    SStrCopy(string->m_text, text, sizeof(string->m_text));

    if (type == WT_HONORGAIN && pvpRank) {
        string->m_pvpRank = *pvpRank;
    }

    return string;
}

// VERIFY: cf58c7c8 manual
// OFFSET: 0x007E70D0
bool WORLDTEXTSTRING::Update(uint32_t time) {
    if (static_cast<int32_t>(time - s_worldTextTypes[this->m_type].m_duration - this->m_startTime) >= 0) {
        return false;
    }

    uint32_t elapsed = time - this->m_startTime;
    this->CalculateTextHeight(elapsed);

    if (!this->m_string) {
        return true;
    }

    this->CalculateNewColor(elapsed);

    C4Vector position;
    position.x = this->m_position.x;
    position.y = this->m_position.y;
    position.z = this->m_position.z;
    position.w = 1.0f;

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        CGObject_C* transport = ClntObjMgrObjectPtr(camera->m_relativeTo, TYPE_GAMEOBJECT, __FILE__, __LINE__);

        if (transport) {
            position = position * transport->GetMatrix();
        }
    }

    position.z = this->CalculateAscendOffset(elapsed) + position.z;

    CGWorldFrame* worldFrame = CGWorldFrame::s_currentWorldFrame;
    CRect frameRect;
    frameRect.minY = 0.0f;
    frameRect.minX = 0.0f;
    frameRect.maxY = 0.0f;
    frameRect.maxX = 0.0f;

    if (!worldFrame->GetRect(&frameRect)) {
        return false;
    }

    C3Vector screen;
    uint32_t result = 0;
    worldFrame->GetScreenCoordinates(C3Vector(position), &screen, &result);

    if (!(result & 0x1) || !(result & 0x4)) {
        return false;
    }

    screen.x = screen.x + frameRect.minX;
    screen.y = screen.y + frameRect.minY;

    float halfWidth = this->m_halfWidth;
    float halfHeight = this->m_halfHeight;

    float maxX = halfWidth < screen.x ? screen.x : halfWidth;
    float x;

    if (NDCToDDCWidth(1.0f) - halfWidth <= maxX) {
        x = NDCToDDCWidth(1.0f) - halfWidth;
    } else {
        x = screen.x > halfWidth ? screen.x : halfWidth;
    }

    float maxY = screen.y > halfHeight ? screen.y : halfHeight;
    float y;

    if (NDCToDDCHeight(1.0f) - halfHeight <= maxY) {
        y = NDCToDDCHeight(1.0f) - halfHeight;
    } else {
        y = screen.y > halfHeight ? screen.y : halfHeight;
    }

    CRect rect;
    rect.minY = this->m_halfHeight + y;
    rect.minX = x - this->m_halfWidth;
    rect.maxY = y - this->m_halfHeight;
    rect.maxX = x + this->m_halfWidth;
    SmartScreenRectPlace(SRECTGRID_WORLDTEXT, rect);

    C3Vector stringPosition;
    stringPosition.x = DDCToNDCWidth((rect.maxX + rect.minX) * 0.5f) - this->m_halfWidth;
    stringPosition.y = DDCToNDCHeight((rect.maxY + rect.minY) * 0.5f);
    stringPosition.z = position.z;
    this->m_screenPosition = stringPosition;
    GxuFontSetStringPosition(this->m_string, stringPosition);

    return true;
}

// VERIFY: 59b2c99d auto
// OFFSET: 0x007E7390
void WORLDTEXTSTRING::AddIcon() {
    if (this->m_pvpRank < 5) {
        return;
    }

    uint32_t rank = this->m_pvpRank - 5;

    if (rank >= 15) {
        return;
    }

    CRect window;
    window.minY = 0.0f;
    window.minX = 0.0f;
    window.maxY = 0.0f;
    window.maxX = 0.0f;
    g_theGxDevicePtr->CapsWindowSize(window);

    float width = window.maxX - window.minX;
    float height = window.maxY - window.minY;

    SWorldTextIcon* icon = s_worldTextIcons.New();
    icon->m_size = this->m_halfHeight * height + this->m_halfHeight * height;
    icon->m_texture = s_pvpRankTextures[rank];
    icon->m_position = this->m_screenPosition;
    icon->m_position.x = icon->m_position.x * width - 40.0f;
    icon->m_position.y = height * icon->m_position.y - (icon->m_size - 32.0f) * 0.5f;
    icon->m_color = this->m_color;
}

// VERIFY: 3fe5b285 manual
// OFFSET: 0x007E7450
bool WorldTextUpdate(WORLDTEXTSTRING* text, uint32_t time) {
    if (!text) {
        return false;
    }

    return text->Update(time);
}

// VERIFY: 85045602 auto
// OFFSET: 0x007E7470
void WorldTextAddIcon(WORLDTEXTSTRING* text) {
    if (text) {
        text->AddIcon();
    }
}

// VERIFY: 4635ab7e manual
// OFFSET: 0x007E7490
void WorldTextRenderBatch() {
    GxuFontRenderBatch(s_worldTextBatch);

    if (!s_worldTextIcons.Count()) {
        return;
    }

    C44Matrix projection;
    C44Matrix view;
    projection = g_theGxDevicePtr->m_projection;
    CGxMatrixStack& viewStack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = viewStack.m_mtx[viewStack.m_level];

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsSet(GxRs_Lighting, 0);
    g_theGxDevicePtr->RsSet(GxRs_Fog, 0);
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    C44Matrix identity;
    g_theGxDevicePtr->XformSetView(identity);
    WorldTextSetupOrthoProjection();
    g_theGxDevicePtr->RsSet(GxRs_DepthTest, 0);
    g_theGxDevicePtr->RsSet(GxRs_DepthWrite, 0);

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPT), 4);
    CGxVertexPT* vertices = reinterpret_cast<CGxVertexPT*>(g_theGxDevicePtr->BufLock(vertexBuf));

    for (uint32_t i = 0; i < 4; i++) {
        vertices[i].p.x = s_iconPositions[i].x * 32.0f;
        vertices[i].p.y = s_iconPositions[i].y * 32.0f;
        vertices[i].p.z = s_iconPositions[i].z * 32.0f;
        vertices[i].tc = s_iconTexCoords[i];
    }

    g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
    vertexBuf->m_hasData = 1;
    GxPrimVertexPtr(vertexBuf, GxVBF_PT);

    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), 6);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 3;
    indices[3] = 3;
    indices[4] = 1;
    indices[5] = 2;
    g_theGxDevicePtr->BufUnlock(indexBuf, 0);
    indexBuf->m_hasData = 1;
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

    g_theGxDevicePtr->XformPush(GxXform_World);

    for (uint32_t i = 0; i < s_worldTextIcons.Count(); i++) {
        SWorldTextIcon& icon = s_worldTextIcons.m_data[i];

        if (TextureGetGxTex(icon.m_texture, 0, nullptr)) {
            continue;
        }

        CImVector color(0xFF, 0xFF, 0xFF, icon.m_color.a);
        g_theGxDevicePtr->RsSet(GxRs_MatDiffuse, *reinterpret_cast<int32_t*>(&color));
        g_theGxDevicePtr->RsSet(GxRs_Texture0, static_cast<void*>(nullptr));

        C44Matrix world;
        world.Translate(icon.m_position);

        CGxMatrixStack& worldStack = g_theGxDevicePtr->m_xforms[GxXform_World];
        worldStack.m_dirty = 1;
        worldStack.m_flags[worldStack.m_level] &= ~CGxMatrixStack::F_Identity;
        worldStack.m_mtx[worldStack.m_level] = world;

        CGxBatch batch;
        batch.m_primType = GxPrim_Triangles;
        batch.m_start = 0;
        batch.m_count = 6;
        batch.m_minIndex = 0;
        batch.m_maxIndex = 3;
        g_theGxDevicePtr->Draw(&batch, 1);
    }

    s_worldTextIcons.SetCount(0);

    CGxMatrixStack& popStack = g_theGxDevicePtr->m_xforms[GxXform_World];

    if (popStack.m_level) {
        popStack.m_level--;
    }

    popStack.m_dirty = 1;
    g_theGxDevicePtr->RsPop();
    g_theGxDevicePtr->XformSetView(view);
    g_theGxDevicePtr->XformSetProjection(projection);
}

// VERIFY: 816c5db7 manual
// OFFSET: 0x007E7BB0
void WorldTextInitialize() {
    const char* fontName = FrameScript_GetText("DAMAGE_TEXT_FONT", -1, GENDER_NOT_APPLICABLE);
    s_worldTextFont = TextBlockGenerateFont(fontName, 0, DDCToNDCHeight(0.018333333f));
    s_worldTextBatch = GxuFontCreateBatch(false, false);

    CStatus status;

    for (uint32_t i = 0; i < 15; i++) {
        s_pvpRankTextures[i] = TextureCreate(s_pvpRankTextureNames[i], CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 0);
    }
}
