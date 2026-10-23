#include "ChatBubbleFrame.h"
#include <cmath>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CBackdropGenerator.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Game/ValidateName.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"

// STATIC: 0x00ACE5EC
static TSList<CGChatBubbleFrame> s_freeChatBubbles;

// STATIC: 0x00ACE5F8
static TSList<CGChatBubbleFrame> s_chatBubbles;

// STATIC: 0x00BEAFBC
static uint8_t s_cameraPositionValid;

// STATIC: 0x00BEAFC0
static C3Vector s_cameraPosition;

// STATIC: 0x00BEAFD4
CDataAllocator CGChatBubbleFrame::s_chatBubbleHeap(sizeof(CGChatBubbleFrame), 10);

// INLINED
void CGChatBubbleFrame::operator delete(void* ptr) {
    if (ptr) {
        CGChatBubbleFrame::s_chatBubbleHeap.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 20523c8e auto
// OFFSET: 0x005EEB70
void CGChatBubbleFrame::InitializeGame() {
}

// VERIFY: 8d8d616f manual
// OFFSET: 0x0056C240
void CGChatBubbleFrame::FadeIn() {
    if (this->m_bubbleFlags & 0x5) {
        return;
    }

    uint8_t alpha = this->m_alpha;
    this->m_bubbleFlags = (this->m_bubbleFlags & ~0x3) | 0x1;
    this->m_fadeStart = OsGetAsyncTimeMs() - static_cast<int32_t>(alpha * (1.0f / 255.0f) * 250.0f);
}

// VERIFY: 2c21f0ea manual
// OFFSET: 0x0056C2B0
void CGChatBubbleFrame::FadeOut(bool remove) {
    if (this->m_bubbleFlags & 0x4) {
        return;
    }

    if (remove) {
        this->m_bubbleFlags |= 0x4;
    }

    if (this->m_bubbleFlags & 0x2) {
        return;
    }

    uint8_t alpha = this->m_alpha;
    this->m_bubbleFlags = (this->m_bubbleFlags & ~0x3) | 0x2;
    this->m_fadeStart = OsGetAsyncTimeMs() - static_cast<int32_t>((255.0f - alpha) * (1.0f / 255.0f) * 250.0f);
}

// VERIFY: 30574dc1 auto
// OFFSET: 0x0056C1C0
static int32_t ComputeDuration(const char* text, bool self) {
    int32_t perWord = self ? 500 : 750;

    if (!text || !*text) {
        return 0;
    }

    int32_t duration = perWord + (self ? 1000 : 2000);
    bool lastWasSpace = true;

    for (const char* c = text; *c; c++) {
        bool space = *c == ' ' || *c == '\t';

        if (space && !lastWasSpace) {
            duration += perWord;
        }

        lastWasSpace = space;
    }

    return duration;
}

// VERIFY: 5706ad2a manual
// OFFSET: 0x0056C340
void CGChatBubbleFrame::Render(CGWorldFrame* worldFrame) {
    if (!ClntObjMgrObjectPtr(this->m_unit, TYPE_UNIT, __FILE__, __LINE__) || !this->m_positionValid) {
        return;
    }

    C3Vector screen(0.0f, 0.0f, 0.0f);
    float threshold = NDCToDDCWidth(1.0f / (CoordinateGetAspectCompensation() * 1024.0f));
    uint32_t onScreen;
    worldFrame->GetScreenCoordinates(this->m_worldPosition, &screen, &onScreen);

    if (!onScreen) {
        return;
    }

    screen.x = CSimpleTop::RoundToPixelWidth(screen.x);
    screen.y = CSimpleTop::RoundToPixelHeight(screen.y);
    screen.z -= worldFrame->m_layoutDepth;
    bool moveX = threshold <= fabs(screen.x - this->m_screenX);
    bool moveY = fabs(screen.y - this->m_screenY) >= threshold;
    bool moveDepth = fabs(screen.z - this->m_screenDepth) >= threshold;

    if (moveX || moveY) {
        if (moveX) {
            this->m_screenX = screen.x;
        }

        if (moveY) {
            this->m_screenY = screen.y;
        }

        this->m_fontString->SetPoint(FRAMEPOINT_BOTTOM, worldFrame, FRAMEPOINT_BOTTOMLEFT, this->m_screenX, this->m_screenY, 1);
    }

    if (moveDepth) {
        this->m_screenDepth = screen.z;
        this->SetFrameDepth(screen.z, false);
    }
}

// VERIFY: aa475361 manual
// OFFSET: 0x0056C500
bool CGChatBubbleFrame::Update(CGUnit_C* player, CGUnit_C* unit) {
    if (!player) {
        return false;
    }

    CMapEntity* entity = unit->m_worldObject;
    CM2Model* model = unit->m_worldModel;

    if (!entity || !model) {
        return false;
    }

    if (!model->IsLoaded(0, 0) || !(model->m_fFlags & 0x8000)) {
        return true;
    }

    if (CGGameUI::m_visible) {
        this->Show();
    } else {
        this->Hide();
    }

    C3Vector position = unit->IsInVehicle() ? unit->m_vehiclePassenger->ComputeInterpolatedUnitPosition() : unit->GetPosition();
    C3Vector delta = player->GetPosition() - position;

    if (World::QueryObjectVisible(entity) && delta.y * delta.y + delta.x * delta.x + delta.z * delta.z < 625.0f) {
        if (this->GetEffectiveAlpha() != 255) {
            this->FadeIn();
        }
    } else if (this->GetEffectiveAlpha()) {
        this->FadeOut(false);
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (!(this->m_bubbleFlags & 0x2) && static_cast<int32_t>(now - this->m_showTime) > this->m_duration + 250) {
        this->FadeOut(true);
    }

    if (this->m_bubbleFlags & 0x1) {
        uint32_t elapsed = now - this->m_fadeStart;

        if (elapsed >= 250) {
            this->SetFrameAlpha(255);
            this->m_bubbleFlags &= ~0x1;
        } else {
            uint32_t alpha = 255 * elapsed / 250;
            this->SetFrameAlpha(static_cast<uint8_t>(alpha > 255 ? 255 : alpha));
        }
    } else if (this->m_bubbleFlags & 0x2) {
        uint32_t elapsed = now - this->m_fadeStart;

        if (elapsed >= 250) {
            if (this->m_bubbleFlags & 0x4) {
                return false;
            }

            this->m_bubbleFlags &= ~0x2;
            this->SetFrameAlpha(0);
        } else {
            uint32_t alpha = 255 * elapsed / 250;

            if (alpha > 255) {
                alpha = 255;
            }

            this->SetFrameAlpha(static_cast<uint8_t>(255 - alpha));
        }
    }

    C3Vector attachment;

    if (unit->IsMounted()) {
        attachment = unit->GetAttachmentPosition(29, nullptr);
    } else {
        attachment = unit->GetAttachmentPosition(18, nullptr);
    }

    this->m_height = attachment.z - unit->GetModelPosition().z;
    this->m_worldPosition.x = position.x;
    this->m_worldPosition.y = position.y;
    this->m_positionValid = 1;
    this->m_worldPosition.z = position.z + this->m_height + 0.7f;
    return true;
}

// VERIFY: 1309b080 manual
// OFFSET: 0x0056C7F0
void CGChatBubbleFrame::Initialize(const WOWGUID& unit, const char* text, const CImVector& color) {
    char buffer[3000];
    StripHyperlinks(LanguageProcess(text), buffer, sizeof(buffer));
    this->m_fontString->SetVertexColor(color);
    this->m_fontString->SetText(buffer, 1);
    this->m_fontString->SetWidth(0.0f);
    this->m_fontString->SetHeight(0.0f);

    if (s_locale == 1) {
        this->m_fontString->SetJustifyH(1);
    }

    float minWidth = NDCToDDCWidth(16.0f / (CoordinateGetAspectCompensation() * 1024.0f));
    minWidth += minWidth;
    float width = this->m_fontString->GetWidth();

    if (0.2f < width) {
        width = 0.2f;
    } else if (minWidth > width) {
        width = minWidth;
    }

    this->m_fontString->SetWidth(width);
    this->m_positionValid = 0;
    this->m_height = 0.0f;
    this->m_worldPosition = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_showTime = OsGetAsyncTimeMs();
    this->m_duration = ComputeDuration(buffer, unit == ClntObjMgrGetActivePlayer());
    this->m_bubbleFlags = 0x8;
    this->m_unit = unit;
    this->SetFrameAlpha(0);
    this->FadeIn();
}

// VERIFY: 42eb0c6b auto
// OFFSET: 0x0056C7A0
void CGChatBubbleFrame::OnWorldRender(CGWorldFrame* worldFrame) {
    CGChatBubbleFrame* next;

    for (CGChatBubbleFrame* bubble = s_chatBubbles.Head(); bubble; bubble = next) {
        next = s_chatBubbles.Next(bubble);
        bubble->Render(worldFrame);
    }

    CLayoutFrame::ResizePending();
}

// VERIFY: be3c560f manual
// OFFSET: 0x0056CAD0
CGChatBubbleFrame::CGChatBubbleFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_screenX = 0.0f;
    this->m_screenY = 0.0f;
    this->m_screenDepth = 0.0f;
    this->m_worldPosition.x = 0.0f;
    this->m_worldPosition.y = 0.0f;
    this->m_worldPosition.z = 0.0f;
    this->SetFrameFlag(0x40000, 1);

    void* data = SMemAlloc(sizeof(CBackdropGenerator), __FILE__, __LINE__, 0);
    CBackdropGenerator* backdrop = data ? new (data) CBackdropGenerator() : nullptr;
    backdrop->m_bgFile.Copy("Interface\\Tooltips\\ChatBubble-Background");
    backdrop->m_pieces = 0xFF;
    backdrop->m_tile = 1;
    backdrop->m_edgeFile.Copy("Interface\\Tooltips\\ChatBubble-Backdrop");
    backdrop->m_alphaMode = GxBlend_Alpha;
    float inset = NDCToDDCWidth(16.0f / (CoordinateGetAspectCompensation() * 1024.0f));
    backdrop->m_insetLeft = inset;
    backdrop->m_insetRight = inset;
    backdrop->m_insetTop = inset;
    backdrop->m_insetBottom = inset;
    backdrop->m_edgeSize = NDCToDDCWidth(16.0f / (CoordinateGetAspectCompensation() * 1024.0f));
    backdrop->m_tileSize = NDCToDDCWidth(16.0f / (CoordinateGetAspectCompensation() * 1024.0f));
    this->SetBackdrop(backdrop);

    void* textureData = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    CSimpleTexture* tail = textureData ? new (textureData) CSimpleTexture(this, 2, 1) : nullptr;
    tail->SetTexture("Interface\\Tooltips\\ChatBubble-Tail", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
    tail->SetWidth(NDCToDDCWidth(16.0f / (CoordinateGetAspectCompensation() * 1024.0f)));
    tail->SetHeight(NDCToDDCWidth(16.0f / (CoordinateGetAspectCompensation() * 1024.0f)));
    tail->SetPoint(FRAMEPOINT_TOPRIGHT, this, FRAMEPOINT_BOTTOM, 0.0f, NDCToDDCWidth(4.0f / (CoordinateGetAspectCompensation() * 1024.0f)), 1);

    void* stringData = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
    this->m_fontString = stringData ? new (stringData) CSimpleFontString(this, 2, 1) : nullptr;
    this->m_fontString->SetFont(FrameScript_GetText("NAMEPLATE_FONT", -1, GENDER_NOT_APPLICABLE), 0.01f, 0, false);
    this->m_fontString->SetNonSpaceWrap(1);
    this->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOPLEFT, -0.01f, 0.01f, 1);
    this->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this->m_fontString, FRAMEPOINT_BOTTOMRIGHT, 0.01f, -0.01f, 1);
}

// VERIFY: 4a39f013 auto
// OFFSET: 0x0056CDC0
void CGChatBubbleFrame::ShutdownGame() {
    s_freeChatBubbles.Clear();
    s_cameraPositionValid = 0;
}

// VERIFY: f2dda14b manual
// OFFSET: 0x0056CDE0
static void InsertSorted(CGChatBubbleFrame* bubble) {
    for (CGChatBubbleFrame* node = s_chatBubbles.Head(); node; node = s_chatBubbles.Next(node)) {
        if (static_cast<int32_t>(node->m_showTime - bubble->m_showTime) >= 0) {
            s_chatBubbles.LinkNode(bubble, 2, node);
            return;
        }
    }

    s_chatBubbles.LinkToTail(bubble);
}

// VERIFY: d9dd76f6 auto
// OFFSET: 0x0056CEA0
CGChatBubbleFrame* CGChatBubbleFrame::GetNewChatBubble(CSimpleFrame* parent) {
    CGChatBubbleFrame* bubble = s_freeChatBubbles.Head();

    if (!bubble) {
        void* data = CGChatBubbleFrame::s_chatBubbleHeap.GetData(0, typeid(CGChatBubbleFrame).raw_name(), SLOG_OBJECT);
        bubble = data ? new (data) CGChatBubbleFrame(parent) : nullptr;
    }

    s_chatBubbles.LinkToTail(bubble);
    return bubble;
}

// VERIFY: ec937cd5 auto
// OFFSET: 0x0056CE80
void CGChatBubbleFrame::Remove(CGChatBubbleFrame* bubble) {
    bubble->Hide();
    s_freeChatBubbles.LinkToTail(bubble);
}

// VERIFY: a296eb99 manual
// OFFSET: 0x0056CF80
void CGChatBubbleFrame::ClearAll() {
    CGChatBubbleFrame* next;

    for (CGChatBubbleFrame* bubble = s_chatBubbles.Head(); bubble; bubble = next) {
        next = s_chatBubbles.Next(bubble);
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(bubble->m_unit, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->ChatBubbleRemove();
        } else {
            CGChatBubbleFrame::Remove(bubble);
        }
    }
}

// VERIFY: c2f0f81c manual
// OFFSET: 0x0056D050
void CGChatBubbleFrame::OnWorldLayerUpdate() {
    if (!s_chatBubbles.Head()) {
        return;
    }

    C3Vector camera = CGWorldFrame::GetActiveCamera()->m_position;
    bool cameraMoved = false;

    if (!s_cameraPositionValid || camera.x != s_cameraPosition.x || camera.y != s_cameraPosition.y || camera.z != s_cameraPosition.z) {
        cameraMoved = true;
        s_cameraPositionValid = 1;
        s_cameraPosition = camera;
    }

    TSList<CGChatBubbleFrame> moved;
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGChatBubbleFrame* next;

    for (CGChatBubbleFrame* bubble = s_chatBubbles.Head(); bubble; bubble = next) {
        next = s_chatBubbles.Next(bubble);
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(bubble->m_unit, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            CGChatBubbleFrame::Remove(bubble);
            continue;
        }

        if (!bubble->Update(player, unit)) {
            unit->ChatBubbleRemove();
            continue;
        }

        if ((bubble->m_bubbleFlags & 0x8) || cameraMoved) {
            moved.LinkToTail(bubble);
            bubble->m_bubbleFlags &= ~0x8;
        }
    }

    if (!moved.Head()) {
        return;
    }

    for (CGChatBubbleFrame* bubble = moved.Head(); bubble; bubble = next) {
        next = moved.Next(bubble);
        InsertSorted(bubble);
    }

    int32_t level = 2;

    for (CGChatBubbleFrame* bubble = s_chatBubbles.Head(); bubble; bubble = next) {
        next = s_chatBubbles.Next(bubble);
        bubble->SetFrameLevel(level++, 1);
    }
}

// VERIFY: 28f3deba manual
// OFFSET: [0x0098FFE0, 0x0056CF50]
CGChatBubbleFrame::~CGChatBubbleFrame() {
}
