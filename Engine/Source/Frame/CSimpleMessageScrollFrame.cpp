#include "Engine/Source/Frame/MessageFrameConstants.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleMessageScrollFrame.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

// STATIC: 0x00DCE4A8
CDataAllocator CSimpleMessageScrollFrame::s_allocator(sizeof(CSimpleMessageScrollFrame), 5);

// STATIC: 0x00B2D060
int32_t CSimpleMessageScrollFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE4A4
int32_t CSimpleMessageScrollFrame::s_objectType;

// STATIC: 0x00B2CC64
const char* CSimpleMessageScrollFrame::s_objectTypeName = "ScrollingMessageFrame";

// INLINED
void CSimpleMessageScrollFrame::operator delete(void* ptr) {
    if (ptr) {
        CSimpleMessageScrollFrame::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleMessageScrollFrame::GetObjectType() {
    if (!CSimpleMessageScrollFrame::s_objectType) {
        CSimpleMessageScrollFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleMessageScrollFrame::s_objectType;
}

// INLINED
CSimpleMessageScrollFrameLine::CSimpleMessageScrollFrameLine() {
    this->m_text = nullptr;
    this->m_color.b = 0xFF;
    this->m_color.g = 0xFF;
    this->m_color.r = 0xFF;
    this->m_color.a = 0xFF;
    this->m_timeLeft = 0.0f;
    this->m_fadeLeft = 0.0f;
    this->m_isVisible = 0;
    this->m_typeID = 0;
    this->m_accessID = 0;
    this->m_extraData = 0;
}

// INLINED
CSimpleMessageScrollFrameLine::~CSimpleMessageScrollFrameLine() {
    if (this->m_text) {
        SMemFree(this->m_text, __FILE__, __LINE__, 0);
    }
}

// VERIFY: 96f08a24 manual
// OFFSET: 0x00969040
FrameScript_Object::ScriptIx* CSimpleMessageScrollFrame::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleHyperlinkedFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnMessageScrollChanged", STORM_MAX_STR)) {
        return &this->m_onMessageScrollChanged;
    }

    return nullptr;
}

// VERIFY: 7b81b9b4 auto
// OFFSET: 0x00969080
void CSimpleMessageScrollFrame::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onMessageScrollChanged, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: 64524f1c auto
// OFFSET: 0x009690C0
int32_t CSimpleMessageScrollFrame::GetCurrentScroll() {
    if (!this->m_numMessages || this->m_currentLine == -1) {
        return 0;
    }

    return (this->m_currentLine + this->m_numMessages - this->m_currentScroll) % this->m_numMessages;
}

// VERIFY: c1dfae24 manual
// OFFSET: 0x009691F0
CSimpleFontStringRecord::CSimpleFontStringRecord(CSimpleFrame* frame, uint32_t drawLayer, bool show) : CSimpleFontString(frame, drawLayer, show) {
}

// INLINED
void CSimpleFontStringRecord::operator delete(void* ptr) {
    if (ptr) {
        SMemFree(ptr, "delete", SLOG_FUNCTION, 0);
    }
}

// VERIFY: 81cf3843 manual
// OFFSET: [0x00969250, 0x00969260, 0x00969270, 0x00969630]
CSimpleFontStringRecord::~CSimpleFontStringRecord() {
}

// VERIFY: 662285ce auto
// OFFSET: 0x00969280
void CSimpleMessageScrollFrame::SetTimeVisible(float seconds) {
    for (int32_t i = 0; i < this->m_numDisplayed; i++) {
        CSimpleMessageScrollFrameLine* line = this->m_displayNodes[i].m_line;

        if (line->m_isVisible) {
            line->m_timeLeft = seconds;
        }
    }

    this->m_timeVisible = seconds;
}

// VERIFY: 2a94563e auto
// OFFSET: 0x009692C0
void CSimpleMessageScrollFrame::SetFadeDuration(float duration) {
    for (int32_t i = 0; i < this->m_numDisplayed; i++) {
        CSimpleMessageScrollFrameLine* line = this->m_displayNodes[i].m_line;

        if (line->m_isVisible) {
            line->m_fadeLeft = duration;
        }
    }

    this->m_fadeDuration = duration;
}

// VERIFY: 2b141796 manual
// OFFSET: 0x00969300
int32_t CSimpleMessageScrollFrame::GetNumMessages(int32_t accessID) {
    if (!accessID) {
        return this->m_numMessages;
    }

    int32_t count = 0;
    int32_t index = this->m_currentLine - this->m_numMessages + 1;

    for (int32_t i = 0; i < this->m_numMessages; i++, index++) {
        int32_t line = index < 0 ? index + this->m_maxMessages : index;

        if (this->m_lines[line].m_accessID == accessID) {
            count++;
        }
    }

    return count;
}

// VERIFY: 45ca47a3 auto
// OFFSET: 0x00969370
void CSimpleMessageScrollFrame::UpdateNode(CSimpleMessageScrollFrameDisplayNode* node, CSimpleMessageScrollFrameLine* line, int32_t reset) {
    if (reset || !this->m_atBottom) {
        line->m_color.a = 0xFF;
        line->m_isVisible = 1;
        line->m_timeLeft = this->m_timeVisible;
        line->m_fadeLeft = this->m_fadeDuration;
    }

    float height = node->m_string->GetStringHeight();

    if (line->m_text) {
        node->m_line = line;
        node->m_string->SetText(line->m_text, 1);
        node->m_string->SetVertexColor(line->m_color);
    } else {
        node->m_string->SetText("", 1);
    }

    if (fabs(node->m_string->GetStringHeight() - height) >= 0.001f) {
        node->m_string->Resize(1);
    }
}

// VERIFY: 7b3c6ac3 manual
// OFFSET: 0x00969410
void CSimpleMessageScrollFrame::RefreshMessages() {
    for (int32_t i = this->m_numDisplayed; i; i--) {
        CSimpleMessageScrollFrameDisplayNode& node = this->m_displayNodes[i - 1];
        CSimpleMessageScrollFrameLine* line = node.m_line;
        line->m_color.a = 0xFF;
        line->m_isVisible = 1;
        line->m_timeLeft = this->m_timeVisible;
        line->m_fadeLeft = this->m_fadeDuration;
        node.m_string->SetVertexColor(line->m_color);

        CSimpleFontStringRecord* string = node.m_string;
        string->m_shown = 1;
        string->ShowThis();
    }

    this->m_hyperlinksDirty = 1;
}

// VERIFY: 5a33a611 manual
// OFFSET: 0x00969670
CSimpleMessageScrollFrameLine* CSimpleMessageScrollFrame::GetMessageInfoByAccessID(uint32_t index, int32_t accessID) {
    if (accessID) {
        int32_t count = this->m_numMessages;

        if (count <= 0) {
            return nullptr;
        }

        int32_t checked = 0;

        for (int32_t i = this->m_currentLine - count + 1;; i++) {
            int32_t lineIndex = i < 0 ? i + this->m_maxMessages : i;
            CSimpleMessageScrollFrameLine* line = &this->m_lines[lineIndex];

            if (line->m_accessID == accessID && !index--) {
                return line;
            }

            if (++checked >= count) {
                return nullptr;
            }
        }
    }

    if (index >= static_cast<uint32_t>(this->m_numMessages)) {
        return nullptr;
    }

    int32_t lineIndex = this->m_currentLine - this->m_numMessages + index + 1;

    if (lineIndex < 0) {
        lineIndex += this->m_maxMessages;
    }

    return &this->m_lines[lineIndex];
}

// VERIFY: 9d5f48d4 manual
// OFFSET: 0x00969720
CSimpleMessageScrollFrameDisplayNode::CSimpleMessageScrollFrameDisplayNode() {
    this->m_line = nullptr;
    this->m_string = new (SMemAlloc(sizeof(CSimpleFontStringRecord), __FILE__, __LINE__, 0)) CSimpleFontStringRecord(nullptr, 1, true);
    this->m_string->m_refcnt++;

    CSimpleFontStringRecord* string = this->m_string;
    uint32_t styleFlags = string->m_styleFlags;
    string->m_settableStyleFlags &= ~0x4000u;

    if ((styleFlags | 0x4000) != styleFlags) {
        string->m_styleFlags = styleFlags | 0x4000;

        if (string->m_string) {
            string->UpdateString();
        }
    }
}

// INLINED
CSimpleMessageScrollFrameDisplayNode::CSimpleMessageScrollFrameDisplayNode(const CSimpleMessageScrollFrameDisplayNode& source) {
    this->m_line = source.m_line;
    this->m_string = source.m_string;
    this->m_string->m_refcnt++;
}

// VERIFY: 787c52e6 manual
// OFFSET: 0x009697B0
CSimpleMessageScrollFrameDisplayNode::~CSimpleMessageScrollFrameDisplayNode() {
    if (--this->m_string->m_refcnt == 0) {
        this->m_string->DeleteSelf();
    }
}

// VERIFY: 3886d5f4 manual
// OFFSET: 0x009698E0
void CSimpleMessageScrollFrame::SetMaxLines(int32_t maxLines) {
    this->m_lines.Clear();
    this->m_displayNodes.Clear();
    this->m_maxMessages = maxLines;
    this->m_numMessages = 0;
    this->m_numDisplayed = 0;
    this->m_atBottom = 1;
    this->m_atTop = 0;
    this->m_lines.SetCount(maxLines);
    this->m_currentLine = -1;
    this->m_currentScroll = 0;
}

// VERIFY: a3114978 manual
// OFFSET: 0x00969960
void CSimpleMessageScrollFrame::Clear() {
    this->m_displayNodes.Clear();
    this->m_numDisplayed = 0;
    this->m_numMessages = 0;
    this->m_hyperlinksDirty = 1;
    this->m_atBottom = 1;
    this->m_atTop = 0;
    this->m_currentLine = -1;
    this->m_currentScroll = 0;
}

// VERIFY: 756a4320 manual
// OFFSET: 0x009699C0
void CSimpleMessageScrollFrame::RefreshHyperlinks() {
    CSimpleHyperlinkButton* button = this->m_hyperlinks.Head();

    while (!TSList<CSimpleHyperlinkButton, TSGetExplicitLink<CSimpleHyperlinkButton> >::IsEnd(button)) {
        CSimpleHyperlinkButton* next = this->m_hyperlinks.RawNext(button);
        this->m_hyperlinks.UnlinkNode(button);
        delete button;
        button = next;
    }

    if (!this->m_hyperlinksEnabled) {
        return;
    }

    for (int32_t i = this->m_numDisplayed - 1; i != -1; i--) {
        CSimpleMessageScrollFrameDisplayNode& node = this->m_displayNodes[i];

        if (!node.m_line->m_isVisible) {
            continue;
        }

        const GXUFONTHYPERLINKINFO* info;
        uint32_t count = GxuFontStringHyperLinkInfo(node.m_string->m_string ? TextBlockGetStringPtr(node.m_string->m_string) : nullptr, info);

        for (uint32_t j = 0; j < count; j++) {
            void* data = CSimpleHyperlinkButton::s_allocator.GetData(0, typeid(CSimpleHyperlinkButton).raw_name(), SLOG_OBJECT);
            CSimpleHyperlinkButton* link = data ? new (data) CSimpleHyperlinkButton(this) : nullptr;
            this->m_hyperlinks.LinkToTail(link);
            link->SetHyperlink(node.m_string, &info[j]);
        }
    }
}

// VERIFY: 8f6eeca2 auto
// OFFSET: [0x00969C30, 0x00969DD0, 0x0096A480]
CSimpleMessageScrollFrame::~CSimpleMessageScrollFrame() {
    this->m_lines.Clear();
    this->m_displayNodes.Clear();
    this->m_numDisplayed = 0;
    delete this->m_font;

    CSimpleHyperlinkButton* button = this->m_hyperlinks.Head();

    while (!TSList<CSimpleHyperlinkButton, TSGetExplicitLink<CSimpleHyperlinkButton> >::IsEnd(button)) {
        CSimpleHyperlinkButton* next = this->m_hyperlinks.RawNext(button);
        this->m_hyperlinks.UnlinkNode(button);
        delete button;
        button = next;
    }
}

// VERIFY: f536be9b auto
// OFFSET: 0x00969DB0
const char* CSimpleMessageScrollFrame::GetObjectTypeName() {
    return CSimpleMessageScrollFrame::s_objectTypeName;
}

// VERIFY: 8d535b01 auto
// OFFSET: 0x00969DC0
int32_t CSimpleMessageScrollFrame::GetScriptMetaTable() {
    return CSimpleMessageScrollFrame::s_metatable;
}

// VERIFY: f7a01043 auto
// OFFSET: 0x00969DE0
bool CSimpleMessageScrollFrame::IsA(int32_t type) {
    return type == CSimpleMessageScrollFrame::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 651a3515 auto
// OFFSET: 0x00969E20
bool CSimpleMessageScrollFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleMessageScrollFrame::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: a1e39c5a manual
// OFFSET: 0x00969E60
void CSimpleMessageScrollFrame::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    if (this->m_fading && this->m_atBottom) {
        for (int32_t i = 0; i < this->m_numDisplayed; i++) {
            CSimpleMessageScrollFrameDisplayNode& node = this->m_displayNodes[i];
            CSimpleMessageScrollFrameLine* line = node.m_line;

            if (!line->m_isVisible) {
                continue;
            }

            bool hide = false;

            if (line->m_timeLeft != 0.0f) {
                line->m_timeLeft -= elapsedSec;

                if (line->m_timeLeft < 0.0f) {
                    if (line->m_fadeLeft == 0.0f) {
                        hide = true;
                    } else {
                        line->m_timeLeft = 0.0f;
                    }
                }
            } else if (line->m_fadeLeft != 0.0f) {
                float fadeLeft = line->m_fadeLeft - elapsedSec;

                if (fadeLeft < 0.0f) {
                    hide = true;
                } else {
                    line->m_fadeLeft = fadeLeft;
                    line->m_color.a = static_cast<uint8_t>(fadeLeft / this->m_fadeDuration * 255.0f);
                    node.m_string->SetVertexColor(line->m_color);
                }
            }

            if (hide) {
                line->m_isVisible = 0;
                CSimpleFontStringRecord* string = node.m_string;
                string->m_shown = 0;
                string->HideThis();
                this->m_hyperlinksDirty = 1;
            }
        }
    }

    if (this->m_hyperlinksDirty) {
        this->RefreshHyperlinks();
        this->m_hyperlinksDirty = 0;
    }
}

// VERIFY: 7336b7a9 manual
// OFFSET: 0x00969FA0
void CSimpleMessageScrollFrame::ScrollMessages(int32_t scroll) {
    if (this->m_currentLine == -1) {
        return;
    }

    bool changed = scroll != this->m_currentScroll;
    this->m_currentScroll = scroll;
    this->m_numDisplayed = 0;

    if (this->m_isVisible && !this->IsResizePending()) {
        int32_t reset = 0;

        if (this->m_currentScroll == this->m_currentLine) {
            if (!this->m_atBottom) {
                reset = 1;
            }

            this->m_atBottom = 1;
        } else {
            this->m_atBottom = 0;
        }

        float height = (this->m_messageFrameArea.maxY - this->m_messageFrameArea.minY) / this->m_layoutScale;
        int32_t count = this->m_numMessages;

        if (this->m_currentScroll != this->m_currentLine) {
            int32_t current = this->m_currentScroll;

            if (current >= this->m_currentLine) {
                current -= this->m_maxMessages;
            }

            count += current - this->m_currentLine;
        }

        int32_t index = scroll;
        int32_t i = 0;

        if (count > 0) {
            while (true) {
                bool created = false;

                if (i == this->m_displayNodes.Count()) {
                    created = true;
                    this->m_displayNodes.SetCount(i + 1);
                }

                CSimpleMessageScrollFrameLine* line = &this->m_lines[index];
                this->UpdateNode(&this->m_displayNodes[i], line, reset);
                CSimpleFontStringRecord* string = this->m_displayNodes[i].m_string;

                if (created) {
                    string->SetFrame(this, 2, 1);
                    string->m_fontableFlags &= ~0x4u;
                    string->SetFontObject(this->m_font);
                    string->SetWidth((this->m_messageFrameArea.maxX - this->m_messageFrameArea.minX) / this->m_layoutScale);

                    if (this->m_insertMode == INSERT_AT_BOTTOM) {
                        if (i == 0) {
                            string->SetPoint(FRAMEPOINT_BOTTOMLEFT, this, FRAMEPOINT_BOTTOMLEFT, this->m_messageFrameInset.minX, this->m_messageFrameInset.minY, 1);
                        } else {
                            string->SetPoint(FRAMEPOINT_BOTTOMLEFT, this->m_displayNodes[i - 1].m_string, FRAMEPOINT_TOPLEFT, 0.0f, CSimpleTop::RoundToPixelHeight(string->m_spacing), 1);
                        }
                    } else {
                        if (i == 0) {
                            string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, this->m_messageFrameInset.minX, this->m_messageFrameInset.minY, 1);
                        } else {
                            string->SetPoint(FRAMEPOINT_TOPLEFT, this->m_displayNodes[i - 1].m_string, FRAMEPOINT_BOTTOMLEFT, 0.0f, -CSimpleTop::RoundToPixelHeight(string->m_spacing), 1);
                        }
                    }

                    string->SetTextLength(this->m_textMaxSize);
                }

                float spacing = CSimpleTop::RoundToPixelHeight(string->m_spacing);
                float lineHeight = string->GetStringHeight() + spacing;

                if (lineHeight > height && i > 0) {
                    break;
                }

                if (line->m_isVisible) {
                    string->m_shown = 1;
                    string->ShowThis();
                } else {
                    string->m_shown = 0;
                    string->HideThis();
                }

                height = height - lineHeight;

                if (--index < 0) {
                    index += this->m_maxMessages;
                }

                this->m_numDisplayed++;

                if (++i >= count) {
                    break;
                }
            }
        }

        this->m_atTop = i == count;

        for (uint32_t j = i; j < this->m_displayNodes.Count(); j++) {
            CSimpleFontStringRecord* string = this->m_displayNodes[j].m_string;
            string->m_shown = 0;
            string->HideThis();
        }
    }

    this->m_hyperlinksDirty = 1;

    if (changed && this->m_onMessageScrollChanged.m_luaRef) {
        this->RunScript(this->m_onMessageScrollChanged, 0, nullptr);
    }
}

// VERIFY: 04724996 manual
// OFFSET: 0x0096A2F0
CSimpleMessageScrollFrame::CSimpleMessageScrollFrame(CSimpleFrame* parent, int32_t maxLines) : CSimpleHyperlinkedFrame(parent) {
    this->m_messageFrameArea.minY = 0.0f;
    this->m_messageFrameArea.minX = 0.0f;
    this->m_messageFrameArea.maxY = 0.0f;
    this->m_messageFrameArea.maxX = 0.0f;
    this->m_messageFrameInset.minY = 0.0f;
    this->m_messageFrameInset.minX = 0.0f;
    this->m_messageFrameInset.maxY = 0.0f;
    this->m_messageFrameInset.maxX = 0.0f;
    this->m_numMessages = 0;
    this->m_maxMessages = maxLines;
    this->m_numDisplayed = 0;
    this->m_atBottom = 1;
    this->m_atTop = 0;
    this->m_textMaxSize = 0;
    this->m_font = new (SMemAlloc(sizeof(CSimpleFontedFrameFont), __FILE__, __LINE__, 0)) CSimpleFontedFrameFont(this);
    this->m_timeVisible = DEFAULT_TIME_VISIBLE;
    this->m_fadeDuration = DEFAULT_FADE_DURATION;
    this->m_fading = 1;
    this->m_lines.SetCount(this->m_maxMessages);
    this->m_currentScroll = 0;
    this->m_hyperlinksDirty = 0;
    this->m_currentLine = -1;
    this->m_hyperlinksEnabled = true;
    this->m_insertMode = INSERT_AT_BOTTOM;
}

// VERIFY: 5b3fe8fd auto
// OFFSET: 0x0096A4B0
void CSimpleMessageScrollFrame::FontUpdated(CSimpleFontedFrameFont* font, int32_t flags) {
    if (flags & 0x11) {
        for (uint32_t i = 0; i < this->m_displayNodes.Count(); i++) {
            this->m_displayNodes[i].m_string->SetFontObject(this->m_font);
        }

        this->ScrollMessages(this->m_currentScroll);
    }
}

// VERIFY: aa69ad5d manual
// OFFSET: 0x0096A510
void CSimpleMessageScrollFrame::RemoveLinesByAccessID(int32_t accessID) {
    int32_t start = this->m_currentLine - this->m_numMessages + 1;

    if (start < 0) {
        start += this->m_maxMessages;
    }

    int32_t src = start;
    int32_t dst = start;
    int32_t kept = 0;

    for (int32_t i = 0; i < this->m_numMessages; i++) {
        if (this->m_lines[src].m_accessID != accessID) {
            if (src != dst) {
                if (this->m_lines[dst].m_text) {
                    SMemFree(this->m_lines[dst].m_text, __FILE__, __LINE__, 0);
                }

                this->m_lines[dst] = this->m_lines[src];
                this->m_lines[src].m_text = nullptr;
            }

            kept++;
            dst = (dst + 1) % this->m_maxMessages;
        }

        src = (src + 1) % this->m_maxMessages;
    }

    if (kept) {
        this->m_currentLine += kept - this->m_numMessages;

        if (this->m_currentLine < 0) {
            this->m_currentLine += this->m_maxMessages;
        }
    } else {
        this->m_currentLine = -1;
    }

    this->m_numMessages = kept;

    if (kept) {
        this->ScrollMessages(this->m_currentLine);
        this->RefreshMessages();
    } else {
        this->Clear();
    }
}

// VERIFY: 26cf1459 auto
// OFFSET: 0x0096A6E0
void CSimpleMessageScrollFrame::UpdateColorByID(int32_t typeID, const CImVector& color) {
    if (!typeID) {
        return;
    }

    for (int32_t i = 0; i < this->m_numMessages; i++) {
        CSimpleMessageScrollFrameLine& line = this->m_lines[i];

        if (line.m_typeID == typeID) {
            line.m_color = color;
        }
    }

    this->ScrollMessages(this->m_currentScroll);
}

// VERIFY: 33e97eaa auto
// OFFSET: 0x0096A740
void CSimpleMessageScrollFrame::OnScreenSizeChanged() {
    CSimpleFrame::OnScreenSizeChanged();
    this->ScrollMessages(this->m_currentScroll);
}

// VERIFY: 3f38cf68 manual
// OFFSET: 0x0096A760
void CSimpleMessageScrollFrame::UpdateRect() {
    if (!(this->m_layoutFlags & 0x100)) {
        return;
    }

    float oldWidth = this->m_messageFrameArea.maxX - this->m_messageFrameArea.minX;
    float oldHeight = this->m_messageFrameArea.maxY - this->m_messageFrameArea.minY;

    this->m_messageFrameArea.minX = this->m_messageFrameInset.minX * this->m_layoutScale + this->m_rect.minX;
    this->m_messageFrameArea.maxX = this->m_rect.maxX - this->m_messageFrameInset.maxX * this->m_layoutScale;
    this->m_messageFrameArea.minY = this->m_messageFrameInset.minY * this->m_layoutScale + this->m_rect.minY;
    this->m_messageFrameArea.maxY = this->m_rect.maxY - this->m_messageFrameInset.maxY * this->m_layoutScale;

    if (fabs(oldWidth - (this->m_messageFrameArea.maxX - this->m_messageFrameArea.minX)) >= 0.0000099999997f || fabs(oldHeight - (this->m_messageFrameArea.maxY - this->m_messageFrameArea.minY)) >= 0.0000099999997f) {
        float width = (this->m_messageFrameArea.maxX - this->m_messageFrameArea.minX) / this->m_layoutScale;

        for (uint32_t i = 0; i < this->m_displayNodes.Count(); i++) {
            this->m_displayNodes[i].m_string->SetWidth(width);
        }

        this->ScrollMessages(this->m_currentScroll);
    }
}

// VERIFY: 5d06da76 auto
// OFFSET: 0x0096A880
void CSimpleMessageScrollFrame::OnLayerShow() {
    CSimpleFrame::OnLayerShow();
    this->ScrollMessages(this->m_currentScroll);
}

// VERIFY: c52848e3 auto
// OFFSET: 0x0096A8A0
int32_t CSimpleMessageScrollFrame::ScrollUp() {
    if (this->m_currentLine == -1) {
        return 0;
    }

    if (this->m_atTop) {
        this->RefreshMessages();
        return 0;
    }

    int32_t scroll = this->m_currentScroll - 1;

    if (scroll < 0) {
        scroll += this->m_maxMessages;
    }

    this->ScrollMessages(scroll);
    return 1;
}

// VERIFY: d8fd3b3e auto
// OFFSET: 0x0096A8E0
int32_t CSimpleMessageScrollFrame::ScrollDown() {
    if (this->m_currentLine == -1) {
        return 0;
    }

    if (this->m_atBottom) {
        this->RefreshMessages();
        return 0;
    }

    int32_t scroll = this->m_currentScroll + 1;

    if (scroll >= this->m_maxMessages) {
        scroll -= this->m_maxMessages;
    }

    this->ScrollMessages(scroll);
    return 1;
}

// VERIFY: 0d06bfca auto
// OFFSET: 0x0096A920
void CSimpleMessageScrollFrame::ScrollToTop() {
    if (this->m_currentLine == -1) {
        return;
    }

    int32_t scroll = 0;

    if (this->m_numMessages == this->m_maxMessages) {
        scroll = this->m_currentLine + 1;

        if (scroll == this->m_maxMessages) {
            scroll = 0;
        }
    }

    this->ScrollMessages(scroll);
}

// VERIFY: 679247cd auto
// OFFSET: 0x0096A950
void CSimpleMessageScrollFrame::ScrollToBottom() {
    if (this->m_currentLine == -1) {
        return;
    }

    if (this->m_atBottom) {
        this->RefreshMessages();
    } else {
        this->ScrollMessages(this->m_currentLine);
    }
}

// VERIFY: 0805dce5 auto
// OFFSET: 0x0096A970
void CSimpleMessageScrollFrame::SetScrollOffset(int32_t offset) {
    int32_t count = this->m_numMessages;

    if (!count || this->m_currentLine == -1) {
        return;
    }

    int32_t scroll = (this->m_currentLine + count - offset) % count;

    if (scroll <= 0) {
        scroll = 0;
    }

    if (count < scroll) {
        scroll = count;
    }

    if (scroll == this->m_currentScroll) {
        this->RefreshMessages();
    } else {
        this->ScrollMessages(scroll);
    }
}

// VERIFY: 14e2d433 auto
// OFFSET: 0x0096A9D0
void CSimpleMessageScrollFrame::SetInsertMode(SimpleMessageFrameInsertMode mode) {
    this->m_insertMode = mode;
    this->UpdateRect();
}

// VERIFY: b857c22a manual
// OFFSET: 0x0096A9F0
void CSimpleMessageScrollFrame::AddMessage(const char* text, const CImVector& color, int32_t typeID, int32_t addToTop, int32_t accessID, int32_t extraData) {
    if (!text) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    int32_t maxMessages = this->m_maxMessages;

    if (this->m_numMessages < maxMessages) {
        this->m_numMessages++;
    }

    int32_t scrollDown = 0;
    CSimpleMessageScrollFrameLine* line;

    if (addToTop) {
        if (this->m_currentLine == -1) {
            this->m_currentLine = 0;
        }

        int32_t index = this->m_currentLine - this->m_numMessages + 1;

        if (index < 0) {
            index += maxMessages;
        }

        line = &this->m_lines[index];
    } else {
        this->m_currentLine = (this->m_currentLine + 1) % maxMessages;
        int32_t bottom = this->m_currentScroll - this->m_numDisplayed + 1;

        if (bottom < 0) {
            bottom += maxMessages;
        }

        if (this->m_currentLine == bottom) {
            scrollDown = 1;
        }

        line = &this->m_lines[this->m_currentLine];
    }

    if (!line) {
        return;
    }

    if (line->m_text) {
        SMemFree(line->m_text, __FILE__, __LINE__, 0);
    }

    line->m_text = SStrDupA(text, __FILE__, __LINE__);
    line->m_color = color;
    line->m_isVisible = 1;
    line->m_timeLeft = this->m_timeVisible;
    line->m_typeID = typeID;
    line->m_fadeLeft = this->m_fadeDuration;
    line->m_accessID = accessID;
    line->m_extraData = extraData;

    if (this->m_atBottom) {
        this->ScrollMessages(this->m_currentLine);
    } else if (scrollDown) {
        this->ScrollDown();
    }
}

// VERIFY: 805c06a6 auto
// OFFSET: 0x0096AB30
void CSimpleMessageScrollFrame::OnFrameSizeChanged(const CRect& rect) {
    CSimpleFrame::OnFrameSizeChanged(rect);
    this->UpdateRect();
}

// VERIFY: ad7c78d5 auto
// OFFSET: 0x0096AB50
void CSimpleMessageScrollFrame::PageUp() {
    for (int32_t i = this->m_numDisplayed; i; i--) {
        if (!this->ScrollUp()) {
            return;
        }
    }

    this->ScrollDown();
}

// VERIFY: 48cf9ab4 auto
// OFFSET: 0x0096ABD0
void CSimpleMessageScrollFrame::PageDown() {
    for (int32_t i = this->m_numDisplayed; i; i--) {
        if (!this->ScrollDown()) {
            return;
        }
    }

    this->ScrollUp();
}

// VERIFY: 35c70e06 manual
// OFFSET: 0x0096AC50
void CSimpleMessageScrollFrame::LoadXML(XMLNode* node, CStatus* status) {
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

    const char* maxLines = node->GetAttributeByName("maxLines");

    if (maxLines && *maxLines && SStrToInt(maxLines) > 0) {
        this->SetMaxLines(SStrToInt(maxLines));
    }

    const char* insertMode = node->GetAttributeByName("insertMode");

    if (insertMode && *insertMode) {
        if (!SStrCmpI(insertMode, "TOP", STORM_MAX_STR)) {
            this->m_insertMode = INSERT_AT_TOP;
        } else {
            this->m_insertMode = INSERT_AT_BOTTOM;
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
