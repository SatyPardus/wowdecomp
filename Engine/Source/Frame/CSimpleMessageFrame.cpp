#include "Engine/Source/Frame/MessageFrameConstants.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleMessageFrame.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// STATIC: 0x00DCE490
CDataAllocator CSimpleMessageFrame::s_allocator(sizeof(CSimpleMessageFrame), 5);

// STATIC: 0x00B2D148
int32_t CSimpleMessageFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE48C
int32_t CSimpleMessageFrame::s_objectType;

// STATIC: 0x00B2CC1C
const char* CSimpleMessageFrame::s_objectTypeName = "MessageFrame";

// INLINED
void CSimpleMessageFrame::operator delete(void* ptr) {
    if (ptr) {
        CSimpleMessageFrame::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleMessageFrame::GetObjectType() {
    if (!CSimpleMessageFrame::s_objectType) {
        CSimpleMessageFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleMessageFrame::s_objectType;
}

// VERIFY: 81e718f6 manual
// OFFSET: 0x00967DB0
void CSimpleMessageFrame::SetTimeVisible(float seconds) {
    for (uint32_t i = 0; i < this->m_rows; i++) {
        CSimpleMessageFrameLineNode* node = this->m_lines[i].m_node;

        if (node->m_timeLeft != 0.0f) {
            node->m_timeLeft = seconds;
        }
    }

    this->m_timeVisible = seconds;
}

// VERIFY: 7de4c0d6 manual
// OFFSET: 0x00967E00
void CSimpleMessageFrame::SetFadeDuration(float duration) {
    for (uint32_t i = 0; i < this->m_rows; i++) {
        CSimpleMessageFrameLineNode* node = this->m_lines[i].m_node;

        if (node->m_fadeLeft != 0.0f) {
            node->m_fadeLeft = duration;
        }
    }

    this->m_fadeDuration = duration;
}

// INLINED
void CSimpleMessageFrame::HideLineNode(CSimpleMessageFrameLineNode* node) {
    if (node->m_isVisible) {
        CSimpleFontString* string = node->m_string;
        string->m_shown = 0;
        string->HideThis();
        node->m_timeLeft = 0.0f;
        node->m_fadeLeft = 0.0f;
        node->m_isVisible = false;
        this->m_numVisible--;
    }
}

// INLINED
void CSimpleMessageFrame::ShowLineNode(CSimpleMessageFrameLineNode* node, float timeVisible, float fadeDuration) {
    CSimpleFontString* string = node->m_string;
    string->m_shown = 1;
    string->ShowThis();
    node->m_isVisible = true;
    node->m_timeLeft = timeVisible;
    node->m_fadeLeft = fadeDuration;
    this->m_numVisible++;
}

// VERIFY: 043a2fa3 manual
// OFFSET: 0x00967FE0
void CSimpleMessageFrame::ScrollMessages(uint32_t index) {
    CSimpleMessageFrameLineNode* last = this->m_lines[this->m_rows - 1].m_node;
    uint32_t i = this->m_rows - 1;

    for (; i > index; i--) {
        this->m_lines[i].m_node = this->m_lines[i - 1].m_node;
        CSimpleMessageFrameLine& line = this->m_lines[i];
        line.m_node->m_string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, line.m_offsetX, -line.m_offsetY, 1);
    }

    this->m_lines[i].m_node = last;
    CSimpleMessageFrameLine& line = this->m_lines[i];
    line.m_node->m_string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, line.m_offsetX, -line.m_offsetY, 1);

    this->HideLineNode(last);
}

// VERIFY: e46e5584 manual
// OFFSET: 0x009680F0
CSimpleMessageFrameLineNode::CSimpleMessageFrameLineNode() {
    this->m_timeLeft = 0.0f;
    this->m_fadeLeft = 0.0f;
    this->m_isVisible = false;

    void* data = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
    this->m_string = data ? new (data) CSimpleFontString(nullptr, 2, true) : nullptr;

    CSimpleFontString* string = this->m_string;
    uint32_t styleFlags = string->m_styleFlags;
    string->m_settableStyleFlags &= ~0x4000u;

    if ((styleFlags | 0x4000) != styleFlags) {
        string->m_styleFlags = styleFlags | 0x4000;

        if (string->m_string) {
            string->UpdateString();
        }
    }
}

// VERIFY: 7fe78925 manual
// OFFSET: 0x00968180
CSimpleMessageFrameLineNode::~CSimpleMessageFrameLineNode() {
    delete this->m_string;
}

// INLINED
CSimpleMessageFrameLine::CSimpleMessageFrameLine() {
    this->m_node = new (SMemAlloc(sizeof(CSimpleMessageFrameLineNode), __FILE__, __LINE__, 0)) CSimpleMessageFrameLineNode();
    this->m_node->m_refcnt++;
    this->m_offsetX = 0.0f;
    this->m_offsetY = 0.0f;
}

// INLINED
CSimpleMessageFrameLine::CSimpleMessageFrameLine(const CSimpleMessageFrameLine& source) {
    this->m_node = source.m_node;
    this->m_node->m_refcnt++;
    this->m_offsetX = source.m_offsetX;
    this->m_offsetY = source.m_offsetY;
}

// INLINED
CSimpleMessageFrameLine::~CSimpleMessageFrameLine() {
    if (--this->m_node->m_refcnt == 0) {
        this->m_node->DeleteSelf();
    }
}

// VERIFY: eedfdb44 manual
// OFFSET: 0x00968210
void CSimpleMessageFrame::AddPendingMessage(const char* text, const CImVector& color) {
    if (!text) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (!this->m_rows) {
        return;
    }

    this->ScrollMessages(0);

    CSimpleMessageFrameLineNode* node = this->m_lines[0].m_node;
    node->m_color = color;
    node->m_string->SetText(text, 1);
    node->m_string->SetVertexColor(color);

    this->ShowLineNode(node, this->m_timeVisible, this->m_fadeDuration);

    float height = node->m_string->GetHeight();
    uint32_t lines = static_cast<uint32_t>(floor(height / node->m_string->GetFontHeight(true) + 0.5));

    if (lines > 1 && this->m_rows > lines) {
        uint32_t top = this->m_insertMode == INSERT_AT_TOP;
        uint32_t count = lines - 1;

        do {
            this->ScrollMessages(top);
        } while (--count);
    }
}

// VERIFY: a94a901f manual
// OFFSET: 0x009684F0
CSimpleMessageFrame::CSimpleMessageFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_messageFrameArea.minY = 0.0f;
    this->m_messageFrameArea.minX = 0.0f;
    this->m_messageFrameArea.maxY = 0.0f;
    this->m_messageFrameArea.maxX = 0.0f;
    this->m_messageFrameInset.minY = 0.0f;
    this->m_messageFrameInset.minX = 0.0f;
    this->m_messageFrameInset.maxY = 0.0f;
    this->m_messageFrameInset.maxX = 0.0f;
    this->m_rows = 0;
    this->m_numVisible = 0;
    this->m_textMaxSize = 0;
    this->m_font = new (SMemAlloc(sizeof(CSimpleFontedFrameFont), __FILE__, __LINE__, 0)) CSimpleFontedFrameFont(this);
    this->m_timeVisible = DEFAULT_TIME_VISIBLE;
    this->m_fading = 1;
    this->m_insertMode = INSERT_AT_BOTTOM;
    this->m_fadeDuration = DEFAULT_FADE_DURATION;
}

// VERIFY: 6c8073c0 auto
// OFFSET: 0x00968610
const char* CSimpleMessageFrame::GetObjectTypeName() {
    return CSimpleMessageFrame::s_objectTypeName;
}

// VERIFY: e51cd05e auto
// OFFSET: 0x00968620
int32_t CSimpleMessageFrame::GetScriptMetaTable() {
    return CSimpleMessageFrame::s_metatable;
}

// VERIFY: 2b059924 auto
// OFFSET: 0x00968640
bool CSimpleMessageFrame::IsA(int32_t type) {
    return type == CSimpleMessageFrame::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: e420b1cc auto
// OFFSET: 0x00968680
bool CSimpleMessageFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleMessageFrame::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 9bdafef4 manual
// OFFSET: 0x009686C0
void CSimpleMessageFrame::AddMessage(const char* text, const CImVector& color) {
    if (!text) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    MessageData* message = this->m_pendingMessages.New();
    message->m_text = SStrDupA(text, __FILE__, __LINE__);
    message->m_color = color;
}

// VERIFY: aa6d7e1c manual
// OFFSET: 0x00968710
void CSimpleMessageFrame::ClearPending() {
    for (uint32_t i = this->m_pendingMessages.Count(); i;) {
        char* text = this->m_pendingMessages[--i].m_text;

        if (text) {
            SMemFree(text, "delete", SLOG_FUNCTION, 0);
        }

        this->m_pendingMessages[i].m_text = nullptr;
    }

    this->m_pendingMessages.Clear();
}

// VERIFY: ca437c53 manual
// OFFSET: 0x00968790
void CSimpleMessageFrame::UpdateRect() {
    if (!(this->m_layoutFlags & 0x100)) {
        return;
    }

    this->m_messageFrameArea.minX = this->m_messageFrameInset.minX * this->m_layoutScale + this->m_rect.minX;
    this->m_messageFrameArea.maxX = this->m_rect.maxX - this->m_messageFrameInset.maxX * this->m_layoutScale;
    this->m_messageFrameArea.minY = this->m_messageFrameInset.minY * this->m_layoutScale + this->m_rect.minY;
    this->m_messageFrameArea.maxY = this->m_rect.maxY - this->m_messageFrameInset.maxY * this->m_layoutScale;

    float fontHeight = this->m_font->m_attributes.m_fontHeight;
    float spacing = CSimpleTop::RoundToPixelHeight(this->m_font->m_attributes.m_spacing * this->m_layoutScale);
    float lineHeight = CSimpleTop::RoundToPixelHeight(this->m_layoutScale * fontHeight) + spacing;

    if (lineHeight <= 0.0f) {
        this->m_rows = 0;
    } else {
        uint32_t rows = static_cast<uint32_t>((this->m_messageFrameArea.maxY - this->m_messageFrameArea.minY) / lineHeight);
        this->m_rows = rows;

        if (fabs(lineHeight * static_cast<float>(rows + 1) - (this->m_messageFrameArea.maxY - this->m_messageFrameArea.minY)) < 0.00000023841858f) {
            this->m_rows = rows + 1;
        }
    }

    for (uint32_t i = this->m_rows; i < this->m_lines.Count(); i++) {
        this->HideLineNode(this->m_lines[i].m_node);
    }

    this->m_lines.SetCount(this->m_rows);

    float invScale = 1.0f / this->m_layoutScale;
    float width = (this->m_messageFrameArea.maxX - this->m_messageFrameArea.minX) * invScale;
    float offsetX = this->m_messageFrameInset.minX;
    float offsetY = this->m_messageFrameInset.maxY;

    uint32_t i;
    int32_t step;

    if (this->m_insertMode != INSERT_AT_TOP) {
        i = this->m_rows - 1;
        step = -1;
    } else {
        i = 0;
        step = 1;
    }

    float lineStep = invScale * lineHeight;

    for (; i < this->m_rows; i += step) {
        CSimpleMessageFrameLine& line = this->m_lines[i];

        if (line.m_node && line.m_node->m_string) {
            CSimpleFontString* string = line.m_node->m_string;
            line.m_offsetX = offsetX / this->m_layoutScale;
            this->m_lines[i].m_offsetY = offsetY / this->m_layoutScale;
            string->SetFrame(this, 2, this->m_lines[i].m_node->m_isVisible);
            string->SetWidth(width);
            string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, this->m_lines[i].m_offsetX, -this->m_lines[i].m_offsetY, 1);
            string->SetFontObject(this->m_font);
            string->SetTextLength(this->m_textMaxSize);
            offsetY = lineStep + offsetY;
        }
    }
}

// VERIFY: 859dfddb manual
// OFFSET: 0x00968A60
void CSimpleMessageFrame::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    uint32_t count = this->m_pendingMessages.Count();

    if (count) {
        CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (this->GetRect(&rect)) {
            for (uint32_t i = 0; i < count; i++) {
                MessageData& message = this->m_pendingMessages[i];
                this->AddPendingMessage(message.m_text, message.m_color);

                if (message.m_text) {
                    SMemFree(message.m_text, "delete", SLOG_FUNCTION, 0);
                }

                message.m_text = nullptr;
            }

            this->m_pendingMessages.SetCount(0);
        }
    }

    if (!this->m_numVisible || !this->m_fading) {
        return;
    }

    for (uint32_t i = 0; i < this->m_rows; i++) {
        CSimpleMessageFrameLineNode* node = this->m_lines[i].m_node;

        if (node->m_timeLeft != 0.0f) {
            node->m_timeLeft -= elapsedSec;

            if (node->m_timeLeft < 0.0f) {
                if (node->m_fadeLeft == 0.0f) {
                    this->HideLineNode(node);
                } else {
                    node->m_timeLeft = 0.0f;
                }
            }
        } else if (node->m_fadeLeft != 0.0f) {
            float fadeLeft = node->m_fadeLeft - elapsedSec;

            if (fadeLeft < 0.0f) {
                this->HideLineNode(node);
            } else {
                node->m_fadeLeft = fadeLeft;
                node->m_color.a = static_cast<uint8_t>(fadeLeft / this->m_fadeDuration * 255.0f);
                node->m_string->SetVertexColor(node->m_color);
            }
        }
    }
}

// VERIFY: 8904645e auto
// OFFSET: [0x00968630, 0x00968C10, 0x00968D70]
CSimpleMessageFrame::~CSimpleMessageFrame() {
    this->m_lines.Clear();
    this->ClearPending();
    delete this->m_font;
}

// VERIFY: 3758e948 auto
// OFFSET: 0x00968C90
void CSimpleMessageFrame::FontUpdated(CSimpleFontedFrameFont* font, int32_t flags) {
    if (flags & 0x11) {
        this->UpdateRect();
    }
}

// VERIFY: ad40c94c auto
// OFFSET: 0x00968CB0
void CSimpleMessageFrame::SetInsertMode(SimpleMessageFrameInsertMode mode) {
    this->m_insertMode = mode;
    this->UpdateRect();
}

// VERIFY: d4cfd810 auto
// OFFSET: 0x00968CD0
void CSimpleMessageFrame::Clear() {
    for (uint32_t i = 0; i < this->m_rows; i++) {
        this->HideLineNode(this->m_lines[i].m_node);
    }

    this->ClearPending();
}

// VERIFY: a8bce361 auto
// OFFSET: 0x00968D50
void CSimpleMessageFrame::OnFrameSizeChanged(const CRect& rect) {
    CSimpleFrame::OnFrameSizeChanged(rect);
    this->UpdateRect();
}

// VERIFY: f7468239 manual
// OFFSET: 0x00968DA0
void CSimpleMessageFrame::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    const char* fontName = node->GetAttributeByName("font");

    if (fontName && *fontName) {
        CSimpleFont* font = CSimpleFont::GetFont(fontName, 0);

        if (font) {
            this->m_font->SetFontObject(font);
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "%s %s: Couldn't find font object named %s", this->GetObjectTypeName(), name ? name : "<unnamed>", fontName);
        }
    }

    const char* fade = node->GetAttributeByName("fade");

    if (fade && *fade) {
        this->m_fading = StringToBOOL(fade);
    }

    const char* displayDuration = node->GetAttributeByName("displayDuration");

    if (displayDuration && *displayDuration) {
        float seconds = SStrToFloat(displayDuration);

        if (seconds > 0.0f) {
            this->SetTimeVisible(seconds);
        }
    }

    const char* fadeDuration = node->GetAttributeByName("fadeDuration");

    if (fadeDuration && *fadeDuration) {
        float duration = SStrToFloat(fadeDuration);

        if (duration > 0.0f) {
            this->SetFadeDuration(duration);
        }
    }

    const char* insertMode = node->GetAttributeByName("insertMode");

    if (insertMode && *insertMode) {
        if (!SStrCmpI(insertMode, "BOTTOM", STORM_MAX_STR)) {
            this->m_insertMode = INSERT_AT_BOTTOM;
        } else {
            this->m_insertMode = INSERT_AT_TOP;
        }

        this->UpdateRect();
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "FontString", STORM_MAX_STR)) {
            if (this->m_font->m_fontObject) {
                CSimpleFont* fontObject = this->m_font->m_fontObject;
                const char* name = this->GetName();
                status->Add(STATUS_WARNING, "%s: FontString element overriding font %s", name ? name : "<unnamed>", fontObject->GetName());
            }

            this->m_font->LoadXML(child, status);

            const char* bytes = child->GetAttributeByName("bytes");

            if (bytes && *bytes) {
                this->m_textMaxSize = SStrToInt(bytes);
            }
        } else if (!SStrCmpI(child->m_name.m_str, "TextInsets", STORM_MAX_STR)) {
            float left;
            float right;
            float top;
            float bottom;

            if (LoadXML_Insets(child, left, right, top, bottom, status)) {
                this->m_messageFrameInset.minX = left;
                this->m_messageFrameInset.maxX = right;
                this->m_messageFrameInset.maxY = top;
                this->m_messageFrameInset.minY = bottom;
                this->UpdateRect();
            }
        }
    }
}
