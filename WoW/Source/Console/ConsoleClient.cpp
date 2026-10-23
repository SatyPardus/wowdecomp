#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <malloc.h>
#include <new>
#include <typeinfo>
#include "ConsoleClient.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Os/OsClipboard.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"

// STATIC: 0x00ADBAB4
static RECTF s_rect = { 0.0f, 1.0f, 1.0f, 1.0f };

// STATIC: 0x00ADBAC4
static KEY s_consoleHotKey = KEY_TILDE;

// STATIC: 0x00ADBAC8
static int32_t s_historyIndex = -1;

// STATIC: 0x00ADBACC
float s_fontHeight = 0.02f;

// STATIC: 0x00ADBAD0
float s_consoleLines = 10.0f;

// STATIC: 0x00ADBAD4
static RECTF s_hRect = { 0.0f, 0.0f, 1.0f, 1.0f };

// STATIC: 0x00ADBAE4
uint32_t s_baseTextFlags = 8;

// STATIC: 0x00ADBAE8
static const char* s_consoleCommandNames[] = { "fontcolor", "bgcolor", "highlightcolor", "fontsize", "font", "consolelines", "clear", "proportionaltext", "spacing", "settings", "default", "closeconsole", "repeat", "AppendLogToFile" };

// STATIC: 0x00ADBB20
static const char* s_consoleCommandHelp[] = { "[ColorClassName] [Red 0-255] [Green 0-255] [Blue 0-255]", "[alpha 0-255] [Red 0-255] [Green 0-255] [Blue 0-255]", "[alpha 0-255] [Red 0-255] [Green 0-255] [Blue 0-255]", "[15-50] arbitrary font size", "[fontname] make sure to use the .ttf file name", "[number] number of lines to show in the console", "Clears the console buffer", "Toggles fixed-width text characters", "[float] specifies inter-character spacing, in pixels", "Shows current font and console settings", "Resets all the font and console settings", "Closes the Console window", "Repeats a command", "[filename = ConsoleLogs/Log<Timestamp>.txt] [numLines = all]" };

// STATIC: 0x00A38308
static const COMMANDHANDLER s_consoleCommandHandlers[] = { ConsoleCommand_FontColor, ConsoleCommand_BackGroundColor, ConsoleCommand_HighLightColor, ConsoleCommand_FontSize, ConsoleCommand_Font, ConsoleCommand_BufferSize, ConsoleCommand_ClearConsole, ConsoleCommand_Proportional, ConsoleCommand_CharSpacing, ConsoleCommand_CurrentSettings, ConsoleCommand_DefaultSettings, ConsoleCommand_CloseConsole, ConsoleCommand_RepeatHandler, ConsoleCommand_AppendLogToFile };

// STATIC: 0x00ADBBB0
TSList<CONSOLELINE, TSGetLink<CONSOLELINE> > s_linelist;

// STATIC: 0x00ADBBBC
uint32_t s_colorArray[NUM_COLORTYPES] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFF808080, 0xFFFF0000, 0xFFFFFF00, 0xFFFFFFFF, 0xFFFFFFFF, 0x80FFFFFF, 0xC0000000 };

// STATIC: 0x00CA1690
static float s_caretpixheight;

// STATIC: 0x00CA1694
static int32_t s_caret;

// STATIC: 0x00CA1698
static char s_copyText[128];

// STATIC: 0x00CA1718
static float s_caretpixwidth;

// STATIC: 0x00CA171C
static CGxString* s_inputString;

// STATIC: 0x00CA1720
static CONSOLELINE* s_currlineptr;

// STATIC: 0x00CA1724
static HLAYER s_layerBackground;

// STATIC: 0x00CA1728
char s_repeatBuffer[64];

// STATIC: 0x00CA1768
char s_fontName[260];

// STATIC: 0x00CA186C
uint32_t s_repeatCount;

// STATIC: 0x00CA1870
static HLAYER s_layerText;

// STATIC: 0x00CA1874
static CGxStringBatch* s_batch;

// STATIC: 0x00CA1878
static char s_partial[256];

// STATIC: 0x00CA1978
int32_t s_active;

// STATIC: 0x00CA197C
uint32_t s_NumLines;

// STATIC: 0x00CA1980
static int32_t s_completionMode;

// STATIC: 0x00CA1984
static const char* s_completedCmd;

// STATIC: 0x00CA198C
HTEXTFONT s_textFont;

enum CONSOLERESIZESTATE : int32_t {
    CS_NONE = 0,
    CS_STRETCH = 1,
    NUM_CONSOLERESIZESTATES = 2,
};

// STATIC: 0x00CA1990
static CONSOLERESIZESTATE s_consoleResizeState;

enum HIGHLIGHTSTATE : int32_t {
    HS_NONE = 0,
    HS_HIGHLIGHTING = 1,
    HS_ENDHIGHLIGHT = 2,
    NUM_HIGHLIGHTSTATES = 3,
};

// STATIC: 0x00CA1994
static HIGHLIGHTSTATE s_highlightState;

// STATIC: 0x00CA1998
static float s_highlightHStart;

// STATIC: 0x00CA199C
static float s_highlightHEnd;

// STATIC: 0x00CA19A0
static uint32_t s_highlightLeftCharIndex;

// STATIC: 0x00CA19A4
static uint32_t s_highlightRightCharIndex;

// STATIC: 0x00CA19A8
float s_charSpacing;

// STATIC: 0x00CA19AC
static const char* s_consoleMessage;

// STATIC: 0x00CA19B0
float s_consoleHeight = s_consoleLines * s_fontHeight;

// STATIC: 0x00CA19B4
static SCritSect s_critsect;

// VERIFY: 4d7c6861 auto
// OFFSET: 0x00763680
void GenerateNodeString(CONSOLELINE* node) {
    CGxFont* font = TextBlockGetFontPtr(s_textFont);

    if (!font || !node || !node->m_buffer || !node->m_buffer[0]) {
        return;
    }

    if (node->m_fontPointer) {
        GxuFontDestroyString(node->m_fontPointer);
    }

    C3Vector position(0.0f, 0.0f, 1.0f);
    GxuFontCreateString(font, reinterpret_cast<const char*>(node->m_buffer), s_fontHeight, position, 1.0f, s_fontHeight, 0.0f, node->m_fontPointer, GxVJ_Middle, GxHJ_Left, s_baseTextFlags, *reinterpret_cast<const CImVector*>(&s_colorArray[node->m_colorType]), s_charSpacing, 1.0f);
}

// VERIFY: bc0f8159 auto
// OFFSET: 0x00763730
static void SetInputString(const char* text) {
    if (s_inputString) {
        GxuFontDestroyString(s_inputString);
    }

    s_inputString = nullptr;

    if (text && *text) {
        C3Vector position(0.0f, 0.0f, 1.0f);
        GxuFontCreateString(TextBlockGetFontPtr(s_textFont), text, s_fontHeight, position, 1.0f, s_fontHeight, 0.0f, s_inputString, GxVJ_Middle, GxHJ_Left, s_baseTextFlags, *reinterpret_cast<const CImVector*>(&s_colorArray[INPUT_COLOR]), s_charSpacing, 1.0f);
    }
}

// VERIFY: 23241548 manual
// OFFSET: 0x007637D0
static void ReserveInputSpace(CONSOLELINE* line, uint32_t chars) {
    if (line->m_chars + chars < line->m_charsalloc) {
        return;
    }

    do {
        line->m_charsalloc += 16;
    } while (line->m_chars + chars >= line->m_charsalloc);

    uint8_t* buffer = static_cast<uint8_t*>(SMemAlloc(line->m_charsalloc, __FILE__, __LINE__, 0));
    SStrCopy(reinterpret_cast<char*>(buffer), reinterpret_cast<const char*>(line->m_buffer), line->m_charsalloc);
    SMemFree(line->m_buffer, __FILE__, __LINE__, 0);
    line->m_buffer = buffer;
}

// VERIFY: 8e2941b8 auto
// OFFSET: 0x00763830
static void ResetHighlight() {
    s_highlightState = HS_NONE;
    s_hRect.right = 0.0f;
    s_hRect.left = 0.0f;
    s_hRect.top = 0.0f;
    s_hRect.bottom = 0.0f;
}

// VERIFY: 2bf504c0 manual
// OFFSET: 0x00763860
static void UpdateHighlight() {
    CGxFont* font = TextBlockGetFontPtr(s_textFont);
    uint32_t length = SStrLen(s_copyText);

    float left = s_highlightHStart >= s_highlightHEnd ? s_highlightHEnd : s_highlightHStart;
    float right = s_highlightHStart > s_highlightHEnd ? s_highlightHStart : s_highlightHEnd;

    if (left < 0.0f) {
        left = 0.0f;
    }

    if (right > 1.0f) {
        right = 1.0f;
    }

    s_highlightLeftCharIndex = GxuFontGetMaxCharsWithinWidth(font, s_copyText, s_fontHeight, left, length, &s_hRect.left, 0.0f, 1.0f, s_charSpacing, s_baseTextFlags);

    if (s_highlightLeftCharIndex) {
        s_highlightLeftCharIndex--;
    }

    if (s_hRect.left < 0.015) {
        s_hRect.left = 0.0f;
    }

    s_highlightRightCharIndex = GxuFontGetMaxCharsWithinWidth(font, s_copyText, s_fontHeight, right, length, &s_hRect.right, 0.0f, 1.0f, s_charSpacing, s_baseTextFlags);
}

// VERIFY: 28f17469 auto
// OFFSET: 0x007639A0
static int32_t OnMouseUp(const void* data, void* param) {
    if (!EventIsKeyDown(s_consoleHotKey) && s_active) {
        s_highlightState = HS_ENDHIGHLIGHT;
        s_consoleResizeState = CS_NONE;
    }

    return 1;
}

// VERIFY: 74333a1f auto
// OFFSET: 0x007639D0
static int32_t OnMouseMove(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouse = static_cast<const EVENT_DATA_MOUSE*>(data);

    if (EventIsKeyDown(s_consoleHotKey) || !s_active) {
        return 1;
    }

    if (s_consoleResizeState == CS_STRETCH) {
        s_consoleHeight = 1.0f - mouse->m_y;

        if (s_consoleHeight < s_fontHeight) {
            s_consoleHeight = s_fontHeight;
        }
    } else if (1.0f - s_consoleHeight > mouse->m_y) {
        return 1;
    }

    s_highlightHEnd = mouse->m_x;

    if (s_highlightState == HS_HIGHLIGHTING) {
        UpdateHighlight();
    }

    return 1;
}

// VERIFY: c2f184c9 auto
// OFFSET: 0x00763A60
static void MakeCommandCurrent(CONSOLELINE* line, const char* command) {
    line->m_inputpos = line->m_inputstart;
    line->m_chars = line->m_inputstart;
    line->m_buffer[line->m_inputstart] = 0;

    uint32_t length = SStrLen(command);
    ReserveInputSpace(line, length);
    SStrCopy(reinterpret_cast<char*>(line->m_buffer) + line->m_inputpos, command, STORM_MAX_STR);

    line->m_inputpos += length;
    line->m_chars = line->m_inputpos;
}

// VERIFY: db580041 manual
// OFFSET: 0x00763AB0
static void Backspace(CONSOLELINE* line) {
    if (line->m_inputpos <= line->m_inputstart) {
        return;
    }

    if (line->m_inputpos < line->m_chars) {
        memmove(&line->m_buffer[line->m_inputpos - 1], &line->m_buffer[line->m_inputpos], line->m_chars - line->m_inputpos + 1);
    } else {
        line->m_buffer[line->m_inputpos - 1] = 0;
    }

    line->m_inputpos--;
    line->m_chars--;
    SetInputString(reinterpret_cast<const char*>(line->m_buffer));
}

// VERIFY: 4bfc942f auto
// OFFSET: 0x00763B10
static void Delete(CONSOLELINE* line) {
    if (line->m_inputpos > line->m_chars) {
        return;
    }

    memmove(&line->m_buffer[line->m_inputpos], &line->m_buffer[line->m_inputpos + 1], line->m_chars - line->m_inputpos);
    line->m_chars--;
    SetInputString(reinterpret_cast<const char*>(line->m_buffer));
}

// VERIFY: d762156a auto
// OFFSET: 0x00763B40
static void Up(CONSOLELINE* line) {
    if (s_historyIndex == ConsoleCommandHistoryDepth() - 1) {
        return;
    }

    int32_t index = s_historyIndex + 1;
    const char* command = ConsoleCommandHistory(index);

    if (command) {
        MakeCommandCurrent(line, command);
        s_historyIndex = index;
        SetInputString(reinterpret_cast<const char*>(line->m_buffer));
    }
}

// VERIFY: e756c19b auto
// OFFSET: 0x00763B90
static void Down(CONSOLELINE* line) {
    if (s_historyIndex == -1) {
        return;
    }

    int32_t index = s_historyIndex - 1;
    const char* command;

    if (index == -1) {
        command = "";
    } else {
        command = ConsoleCommandHistory(index);

        if (!command) {
            return;
        }
    }

    MakeCommandCurrent(line, command);
    s_historyIndex = index;
    SetInputString(reinterpret_cast<const char*>(line->m_buffer));
}

// VERIFY: e9e50349 auto
// OFFSET: 0x00763BE0
static int32_t OnKeyUp(const void* data, void* param) {
    if (static_cast<const EVENT_DATA_KEY*>(data)->m_key == s_consoleHotKey && IsConsoleEnabled()) {
        return 0;
    }

    return !s_active;
}

// VERIFY: aff0c6bd auto
// OFFSET: 0x00763C20
const char* GetConsoleMessage() {
    return s_consoleMessage;
}

// VERIFY: 474f76f9 auto
// OFFSET: 0x00763C30
void SetConsoleMessage(const char* message) {
    s_consoleMessage = message;
}

// VERIFY: e6af0f35 auto
// OFFSET: 0x00763D60
static int32_t EventCloseCallback(void* param) {
    EventPostCloseEx(EventGetCurrentContext());

    return 0;
}

// VERIFY: f5ab2b88 manual
// OFFSET: 0x00763D80
CONSOLELINE::~CONSOLELINE() {
    if (this->m_buffer) {
        SMemFree(this->m_buffer, __FILE__, __LINE__, 0);
    }

    if (this->m_fontPointer) {
        GxuFontDestroyString(this->m_fontPointer);
    }
}

// VERIFY: 6f682d92 manual
// OFFSET: 0x00763DF0
static void DrawBackground() {
    C3Vector position[4] = {
        C3Vector(s_rect.left, s_rect.bottom, 0.0f),
        C3Vector(s_rect.right, s_rect.bottom, 0.0f),
        C3Vector(s_rect.left, s_rect.top, 0.0f),
        C3Vector(s_rect.right, s_rect.top, 0.0f)
    };

    uint16_t indices[4] = { 0, 1, 2, 3 };

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_PolygonOffset, 0.0f);
    GxRsSet(GxRs_BlendingMode, GxBlend_Alpha);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    GxPrimLockVertexPtrs(4, position, sizeof(C3Vector), nullptr, 0, reinterpret_cast<const CImVector*>(&s_colorArray[BACKGROUND_COLOR]), 0, nullptr, 0, nullptr, 0, nullptr, 0);
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, indices);
    GxPrimUnlockVertexPtrs();

    g_theGxDevicePtr->RsPop();
}

// VERIFY: cbd1e215 manual
// OFFSET: 0x00764040
static void DrawHighLight() {
    C3Vector position[4] = {
        C3Vector(s_hRect.left, s_hRect.bottom, 0.0f),
        C3Vector(s_hRect.right, s_hRect.bottom, 0.0f),
        C3Vector(s_hRect.left, s_hRect.top, 0.0f),
        C3Vector(s_hRect.right, s_hRect.top, 0.0f)
    };

    uint16_t indices[4] = { 0, 1, 2, 3 };

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_BlendingMode, GxBlend_Alpha);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    GxPrimLockVertexPtrs(4, position, sizeof(C3Vector), nullptr, 0, reinterpret_cast<const CImVector*>(&s_colorArray[HIGHLIGHT_COLOR]), 0, nullptr, 0, nullptr, 0, nullptr, 0);
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, indices);
    GxPrimUnlockVertexPtrs();

    g_theGxDevicePtr->RsPop();
}

// VERIFY: dd3a5a44 manual
// OFFSET: 0x007641A0
static void DrawCaret(const C3Vector& caretpos) {
    C3Vector position[4] = {
        C3Vector(caretpos.x, caretpos.y, 0.0f),
        C3Vector(caretpos.x + s_caretpixwidth * 2.0f, caretpos.y, 0.0f),
        C3Vector(caretpos.x, caretpos.y + s_fontHeight, 0.0f),
        C3Vector(caretpos.x + s_caretpixwidth * 2.0f, caretpos.y + s_fontHeight, 0.0f)
    };

    uint16_t indices[4] = { 0, 1, 2, 3 };

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_PolygonOffset, 0.0f);
    GxRsSet(GxRs_BlendingMode, GxBlend_Alpha);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    GxPrimLockVertexPtrs(4, position, sizeof(C3Vector), nullptr, 0, reinterpret_cast<const CImVector*>(&s_colorArray[INPUT_COLOR]), 0, nullptr, 0, nullptr, 0, nullptr, 0);
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, indices);
    GxPrimUnlockVertexPtrs();

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 265f30fc manual
// OFFSET: 0x007643F0
static void PaintBackground(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec) {
    if (s_rect.bottom < 1.0f) {
        DrawBackground();

        if (s_highlightState) {
            DrawHighLight();
        }
    }
}

// VERIFY: 15c91155 manual
// OFFSET: 0x007644B0
static void EnforceMaxLines() {
    if (s_NumLines <= 256) {
        return;
    }

    CONSOLELINE* node = s_linelist.Tail();

    if (node == s_currlineptr) {
        s_currlineptr = s_linelist.Prev(node);
    }

    node->~CONSOLELINE();
    SMemFree(node, typeid(CONSOLELINE).raw_name(), SLOG_OBJECT, 0);
    s_NumLines--;
}

// VERIFY: 9d457e81 manual
// OFFSET: 0x00764520
static CONSOLELINE* GetInputLine() {
    CONSOLELINE* head = s_linelist.Head();

    if (head && head->m_inputpos) {
        return head;
    }

    CONSOLELINE* line = s_linelist.NewNode(1, 0, 0);

    line->m_charsalloc = 16;
    line->m_buffer = static_cast<uint8_t*>(SMemAlloc(line->m_charsalloc, __FILE__, __LINE__, 0));
    SStrCopy(reinterpret_cast<char*>(line->m_buffer), "> ", line->m_charsalloc);
    SetInputString(reinterpret_cast<const char*>(line->m_buffer));

    uint32_t length = SStrLen(reinterpret_cast<const char*>(line->m_buffer));
    s_NumLines++;
    line->m_chars = line->m_inputpos = line->m_inputstart = length;
    line->m_colorType = INPUT_COLOR;

    s_currlineptr = line;
    EnforceMaxLines();

    return line;
}

// VERIFY: c9dc3834 manual
// OFFSET: 0x007645E0
static void PaintText(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec) {
    // STATIC: 0x00CA19F0
    static float carettime;

    if (s_rect.bottom >= 1.0f) {
        return;
    }

    carettime += elapsedSec;

    if ((s_caret && carettime >= 0.3f) || (!s_caret && carettime >= 0.2f)) {
        carettime = 0.0f;
        s_caret = !s_caret;
    }

    CONSOLELINE* lineptr = GetInputLine();
    C3Vector position(s_rect.left, s_fontHeight * 0.75f + s_rect.bottom, 1.0f);

    GxuFontClearBatch(s_batch);

    if (s_inputString) {
        GxuFontSetStringPosition(s_inputString, position);
        GxuFontAddToBatch(s_batch, s_inputString);
    }

    if (lineptr->m_inputpos) {
        C3Vector caretpos = position;
        GxuFontGetTextExtent(TextBlockGetFontPtr(s_textFont), reinterpret_cast<const char*>(lineptr->m_buffer), lineptr->m_inputpos, s_fontHeight, &caretpos.x, 0.0f, 1.0f, s_charSpacing, s_baseTextFlags);
        DrawCaret(caretpos);
    }

    position.y += s_fontHeight;

    for (CONSOLELINE* ptr = s_currlineptr; !s_linelist.IsEnd(ptr) && position.y < 1.0f; ptr = ptr->RawNext()) {
        if (ptr->m_fontPointer && ptr != lineptr) {
            GxuFontSetStringPosition(ptr->m_fontPointer, position);
            GxuFontAddToBatch(s_batch, ptr->m_fontPointer);
            position.y += s_fontHeight;
        }
    }

    GxuFontRenderBatch(s_batch);
}

// VERIFY: c5bd90a0 manual
// OFFSET: 0x00764790
static CONSOLELINE* GetLineAtMousePosition(float y) {
    int32_t lineIndex = static_cast<int32_t>((s_consoleHeight - (1.0f - y)) / s_fontHeight);

    if (lineIndex == 1) {
        return s_linelist.Head();
    }

    if (s_currlineptr != s_linelist.Head()) {
        lineIndex--;
    }

    for (CONSOLELINE* line = s_currlineptr; !s_linelist.IsEnd(line); line = line->RawNext()) {
        if (lineIndex <= 1) {
            return line;
        }

        lineIndex--;
    }

    return nullptr;
}

// VERIFY: deafcbfa manual
// OFFSET: 0x00764800
static void PasteInInputLine(const char* text) {
    int32_t length = SStrLen(text);

    if (!length) {
        return;
    }

    CONSOLELINE* line = GetInputLine();
    ReserveInputSpace(line, length);

    if (line->m_inputpos >= line->m_chars) {
        for (int32_t i = 0; i < length; i++) {
            line->m_buffer[line->m_inputpos] = text[i];
            line->m_inputpos++;
        }

        line->m_buffer[line->m_inputpos] = 0;
        line->m_chars = line->m_inputpos;
    } else if (length > 1) {
        char* tail = static_cast<char*>(SMemAlloc(line->m_charsalloc, __FILE__, __LINE__, 0));
        SStrCopy(tail, reinterpret_cast<const char*>(line->m_buffer) + line->m_inputpos, STORM_MAX_STR);

        char* head = static_cast<char*>(SMemAlloc(line->m_charsalloc, __FILE__, __LINE__, 0));
        SStrCopy(head, reinterpret_cast<const char*>(line->m_buffer), STORM_MAX_STR);
        head[line->m_inputpos] = 0;

        SStrPack(head, text, line->m_charsalloc);
        line->m_inputpos = SStrLen(head);
        SStrPack(head, tail, line->m_charsalloc);

        SStrCopy(reinterpret_cast<char*>(line->m_buffer), head, STORM_MAX_STR);
        line->m_chars = SStrLen(reinterpret_cast<const char*>(line->m_buffer));

        if (tail) {
            SMemFree(tail, __FILE__, __LINE__, 0);
        }

        if (head) {
            SMemFree(head, __FILE__, __LINE__, 0);
        }
    } else {
        memmove(&line->m_buffer[line->m_inputpos + 1], &line->m_buffer[line->m_inputpos], line->m_chars - line->m_inputpos + 1);
        line->m_buffer[line->m_inputpos] = *text;
        line->m_inputpos++;
        line->m_chars++;
    }

    SetInputString(reinterpret_cast<const char*>(line->m_buffer));
}

// VERIFY: d9f1e771 auto
// OFFSET: 0x00764990
static int32_t OnChar(const void* data, void* param) {
    if (EventIsKeyDown(s_consoleHotKey) && IsConsoleEnabled()) {
        return 0;
    }

    if (!s_active) {
        return 1;
    }

    char character[2];
    character[0] = static_cast<char>(static_cast<const EVENT_DATA_CHAR*>(data)->m_ch);
    character[1] = 0;

    PasteInInputLine(character);
    ResetHighlight();

    return 0;
}

// VERIFY: 8301bc53 manual
// OFFSET: 0x00764A10
static int32_t OnMouseDown(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouse = static_cast<const EVENT_DATA_MOUSE*>(data);

    if (EventIsKeyDown(s_consoleHotKey) || !s_active) {
        return 1;
    }

    if (1.0f - s_consoleHeight > mouse->m_y) {
        return 1;
    }

    float height = s_consoleHeight >= 1.0f ? 1.0f : s_consoleHeight;
    float y = 1.0f - mouse->m_y;

    if (y >= height - s_fontHeight * 0.75f && s_consoleHeight >= y) {
        ResetHighlight();
        s_consoleResizeState = CS_STRETCH;

        return 0;
    }

    ResetHighlight();

    CONSOLELINE* line = GetLineAtMousePosition(mouse->m_y);

    if (!line) {
        s_copyText[0] = 0;

        return 0;
    }

    SStrCopy(s_copyText, reinterpret_cast<const char*>(line->m_buffer), sizeof(s_copyText));
    s_highlightState = HS_HIGHLIGHTING;

    int32_t lineIndex = static_cast<int32_t>((s_consoleHeight - y) / s_fontHeight - 1.0f);
    s_hRect.top = 1.0f - (s_consoleHeight - s_fontHeight * 0.75f - s_fontHeight - lineIndex * s_fontHeight);
    s_hRect.bottom = s_hRect.top - s_fontHeight;

    s_highlightHStart = mouse->m_x;
    s_highlightHEnd = mouse->m_x;
    UpdateHighlight();

    return 0;
}

// VERIFY: 18db0a1d manual
// OFFSET: 0x00764B90
static int32_t OnKeyDownRepeat(const void* data, void* param) {
    const EVENT_DATA_KEY* key = static_cast<const EVENT_DATA_KEY*>(data);

    if (key->m_key == s_consoleHotKey && IsConsoleEnabled()) {
        s_active = !s_active;

        return 0;
    }

    if (EventIsKeyDown(s_consoleHotKey) || !s_active) {
        return 1;
    }

    CONSOLELINE* line = GetInputLine();

    switch (key->m_key) {
        case KEY_BACKSPACE:
            Backspace(line);
            break;

        case KEY_LEFT:
            if (line->m_inputpos > line->m_inputstart) {
                line->m_inputpos--;
            }

            break;

        case KEY_UP:
            Up(line);
            break;

        case KEY_RIGHT:
            if (line->m_inputpos < line->m_chars) {
                line->m_inputpos++;
            }

            break;

        case KEY_DOWN:
            Down(line);
            break;

        case KEY_DELETE:
            Delete(line);
            break;

        case KEY_PAGEUP:
            if (key->m_metaKeyState & 3) {
                CONSOLELINE* ptr = s_currlineptr;

                for (int32_t i = 0; i < 10 && ptr; i++) {
                    CONSOLELINE* next = ptr->Next();

                    if (next) {
                        ptr = next;
                    }
                }

                if (ptr) {
                    s_currlineptr = ptr;
                }
            } else {
                if (s_currlineptr == s_linelist.Head()) {
                    s_currlineptr = s_currlineptr->Next();
                }

                if (s_currlineptr) {
                    CONSOLELINE* next = s_currlineptr->Next();

                    if (next) {
                        s_currlineptr = next;
                    }
                }
            }

            break;

        case KEY_PAGEDOWN:
            if (key->m_metaKeyState & 3) {
                CONSOLELINE* ptr = s_currlineptr;

                for (int32_t i = 0; i < 10 && ptr; i++) {
                    CONSOLELINE* prev = ptr->m_link.Prev();

                    if (prev) {
                        ptr = prev;
                    }
                }

                if (ptr) {
                    s_currlineptr = ptr;
                }
            } else {
                if (s_currlineptr == s_linelist.Head()) {
                    s_currlineptr = s_currlineptr->Next();
                }

                if (s_currlineptr) {
                    CONSOLELINE* prev = s_currlineptr->m_link.Prev();

                    if (prev) {
                        s_currlineptr = prev;
                    }
                }
            }

            break;

        default:
            break;
    }

    if (key->m_key != KEY_TAB && key->m_key != KEY_LSHIFT && key->m_key != KEY_RSHIFT && key->m_key != KEY_LALT && key->m_key != KEY_RALT && !(key->m_metaKeyState & 0xC)) {
        s_completionMode = 0;
        ResetHighlight();
    }

    return 0;
}

// VERIFY: ed71f29f manual
// OFFSET: 0x00765270
void ConsoleWrite(const char* str, COLOR_T color) {
    if (!str || !*str || !ConsoleDeviceExists() || !s_textFont) {
        return;
    }

    s_critsect.Enter();

    CONSOLELINE* node = s_linelist.NewNode(0, 0, 0);
    CONSOLELINE* head = s_linelist.Head();

    if (head && head->m_inputpos) {
        s_linelist.LinkNode(node, 1, head);
    } else {
        s_linelist.LinkToHead(node);
    }

    size_t length = SStrLen(str) + 1;
    node->m_chars = length;
    node->m_charsalloc = length;
    node->m_buffer = static_cast<uint8_t*>(SMemAlloc(length, __FILE__, __LINE__, 0));
    SStrCopy(reinterpret_cast<char*>(node->m_buffer), str, STORM_MAX_STR);
    node->m_colorType = color;

    GenerateNodeString(node);
    s_NumLines++;
    EnforceMaxLines();

    s_critsect.Leave();
}

// VERIFY: 7dcf8eb5 auto
// OFFSET: 0x00765360
void ConsoleWriteA(const char* format, COLOR_T color, ...) {
    if (!format || !*format || !ConsoleDeviceExists()) {
        return;
    }

    char buffer[1024];
    va_list args;
    va_start(args, color);
    SStrVPrintf(buffer, sizeof(buffer), format, args);
    ConsoleWrite(buffer, color);
    va_end(args);
}

// VERIFY: da6ae03f auto
// OFFSET: 0x007653B0
void ConsolePrintf(const char* format, ...) {
    if (!format || !*format || !ConsoleDeviceExists()) {
        return;
    }

    char buffer[4096];
    va_list args;
    va_start(args, format);
    SStrVPrintf(buffer, sizeof(buffer), format, args);
    ConsoleWrite(buffer, DEFAULT_COLOR);
    va_end(args);
}

// VERIFY: d43d21f6 manual
// OFFSET: 0x007658A0
void ConsoleCommandExecute(const char* commandLine, int32_t addToHistory) {
    if (s_execState == EM_RECORDING || s_execState == EM_PROMPTOVERWRITE || s_execState == EM_APPEND) {
        if (!AddLineToExecFile(commandLine)) {
            return;
        }
    }

    while (*commandLine == ' ') {
        commandLine++;
    }

    if (addToHistory) {
        const char* last = ConsoleCommandHistory(0);

        if (!last || SStrCmp(commandLine, last, STORM_MAX_STR)) {
            AddToHistory(commandLine);
        }
    }

    char* arguments = static_cast<char*>(SMemAlloc(1024, __FILE__, __LINE__, 0));
    const char* command;
    CONSOLECOMMAND* entry = ParseCommand(commandLine, &command, arguments, 1024);

    if (!entry) {
        if (g_defaultCommand) {
            if (g_defaultCommand(command, arguments)) {
                goto done;
            }
        } else {
            entry = g_consoleCommandHash.Ptr("run");

            if (!entry) {
                ConsoleWrite("Unknown command", DEFAULT_COLOR);
                goto done;
            }

            command = "";
            SStrCopy(arguments, commandLine, STORM_MAX_STR);
        }
    }

    OsValidateFunctionPointer(reinterpret_cast<const void*>(entry->m_handler));
    entry->m_handler(command, arguments);

done:
    if (arguments) {
        delete[] arguments;
    }
}

// VERIFY: f39b7102 manual
// OFFSET: 0x007659C0
static int32_t OnIdle(const void* data, void* param) {
    float finalPos = s_active ? 1.0f - s_consoleHeight : 1.0f;

    if (finalPos > 1.0f) {
        finalPos = 1.0f;
    } else if (!(finalPos > 0.0f)) {
        finalPos = 0.0f;
    }

    if (s_repeatCount && s_repeatBuffer[0]) {
        ConsoleCommandExecute(s_repeatBuffer, 0);
        s_repeatCount--;
    }

    if (s_rect.bottom == finalPos) {
        return 1;
    }

    if (s_consoleResizeState) {
        s_rect.bottom = finalPos;
    } else {
        float direction = s_rect.bottom > finalPos ? -1.0f : 1.0f;
        float elapsedSec = static_cast<const EVENT_DATA_IDLE*>(data)->m_elapsedSec;

        if (s_active) {
            float bottom = s_rect.bottom + direction * elapsedSec * 5.0f;
            s_rect.bottom = finalPos > bottom ? finalPos : bottom;
        } else {
            float bottom = s_rect.bottom + direction * elapsedSec * 5.0f;
            s_rect.bottom = finalPos < bottom ? finalPos : bottom;
        }
    }

    ScrnLayerSetRect(s_layerBackground, &s_rect);
    ScrnLayerSetRect(s_layerText, &s_rect);

    return 1;
}

// VERIFY: d6707bb6 manual
// OFFSET: 0x00765AF0
static int32_t OnKeyDown(const void* data, void* param) {
    const EVENT_DATA_KEY* key = static_cast<const EVENT_DATA_KEY*>(data);

    if (key->m_key == s_consoleHotKey && IsConsoleEnabled()) {
        s_active = !s_active;

        if (!s_active) {
            ResetHighlight();
        }

        return 0;
    }

    if (EventIsKeyDown(s_consoleHotKey) || !s_active) {
        return 1;
    }

    CONSOLELINE* line = GetInputLine();

    switch (key->m_key) {
        case KEY_ESCAPE:
            if (line->m_inputpos > line->m_inputstart) {
                line->m_inputpos = line->m_inputstart;
                line->m_chars = line->m_inputstart;
                line->m_buffer[line->m_inputstart] = 0;
                SetInputString(reinterpret_cast<const char*>(line->m_buffer));
            } else {
                s_active = 0;
            }

            break;

        case KEY_ENTER:
            if (line->m_inputpos > line->m_inputstart) {
                line->m_inputpos = 0;
                GenerateNodeString(line);
                ConsoleCommandExecute(reinterpret_cast<const char*>(line->m_buffer) + line->m_inputstart, 1);
                s_historyIndex = -1;
            }

            break;

        case KEY_BACKSPACE:
            Backspace(line);
            break;

        case KEY_TAB:
            if (!s_completionMode) {
                s_completedCmd = nullptr;
                s_completionMode = 1;
                SStrCopy(s_partial, reinterpret_cast<const char*>(line->m_buffer) + line->m_inputstart, STORM_MAX_STR);
            }

            if (ConsoleCommandComplete(s_partial, &s_completedCmd, (key->m_metaKeyState & 3) != 0)) {
                MakeCommandCurrent(line, s_completedCmd);
            }

            SetInputString(reinterpret_cast<const char*>(line->m_buffer));
            break;

        case KEY_LEFT:
            if (line->m_inputpos > line->m_inputstart) {
                line->m_inputpos--;
            }

            break;

        case KEY_UP:
            Up(line);
            break;

        case KEY_RIGHT:
            if (line->m_inputpos < line->m_chars) {
                line->m_inputpos++;
            }

            break;

        case KEY_DOWN:
            Down(line);
            break;

        case KEY_DELETE:
            Delete(line);
            break;

        case KEY_HOME:
            if (key->m_metaKeyState & 0xC) {
                s_currlineptr = s_linelist.Tail();
            } else if (line->m_inputpos > line->m_inputstart) {
                line->m_inputpos = line->m_inputstart;
            }

            break;

        case KEY_END:
            if (key->m_metaKeyState & 0xC) {
                s_currlineptr = s_linelist.Head();
            } else if (line->m_inputpos < line->m_chars) {
                line->m_inputpos = line->m_chars;
            }

            break;

        case KEY_PAGEUP:
            if (key->m_metaKeyState & 3) {
                CONSOLELINE* ptr = s_currlineptr;

                for (int32_t i = 0; i < 10 && ptr; i++) {
                    CONSOLELINE* next = ptr->Next();

                    if (next) {
                        ptr = next;
                    }
                }

                if (ptr) {
                    s_currlineptr = ptr;
                }
            } else {
                if (s_currlineptr == s_linelist.Head()) {
                    s_currlineptr = s_currlineptr->Next();
                }

                if (s_currlineptr) {
                    CONSOLELINE* next = s_currlineptr->Next();

                    if (next) {
                        s_currlineptr = next;
                    }
                }
            }

            break;

        case KEY_PAGEDOWN:
            if (key->m_metaKeyState & 3) {
                CONSOLELINE* ptr = s_currlineptr;

                for (int32_t i = 0; i < 10 && ptr; i++) {
                    CONSOLELINE* prev = ptr->m_link.Prev();

                    if (prev) {
                        ptr = prev;
                    }
                }

                if (ptr) {
                    s_currlineptr = ptr;
                }
            } else {
                if (s_currlineptr == s_linelist.Head()) {
                    s_currlineptr = s_currlineptr->Next();
                }

                if (s_currlineptr) {
                    CONSOLELINE* prev = s_currlineptr->m_link.Prev();

                    if (prev) {
                        s_currlineptr = prev;
                    }
                }
            }

            break;

        case 'C':
            if (key->m_metaKeyState & 0xC) {
                if (s_copyText[0]) {
                    char buffer[128];
                    uint32_t length = s_highlightRightCharIndex - s_highlightLeftCharIndex;

                    if (length >= sizeof(buffer) - 1) {
                        length = sizeof(buffer) - 1;
                    }

                    memcpy(buffer, &s_copyText[s_highlightLeftCharIndex], length);
                    buffer[length] = 0;
                    OsClipboardPutString(buffer);
                }

                ResetHighlight();
            }

            break;

        case 'V':
            if (key->m_metaKeyState & 0xC) {
                char* text = OsClipboardGetString();

                if (text) {
                    PasteInInputLine(text);
                    SMemFree(text, __FILE__, __LINE__, 0);
                    ResetHighlight();
                }
            }

            break;

        default:
            break;
    }

    if (key->m_key != KEY_TAB && key->m_key != KEY_LSHIFT && key->m_key != KEY_RSHIFT && key->m_key != KEY_LALT && key->m_key != KEY_RALT && !(key->m_metaKeyState & 0xC)) {
        s_completionMode = 0;
        ResetHighlight();
    }

    return 0;
}

// VERIFY: 8f2c32a0 auto
// OFFSET: 0x00765FF0
static void RegisterHandlers() {
    EventRegisterEx(EVENT_ID_CHAR, OnChar, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_IDLE, OnIdle, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN, OnKeyDown, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_KEYUP, OnKeyUp, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, OnKeyDownRepeat, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_MOUSEDOWN, OnMouseDown, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_MOUSEUP, OnMouseUp, nullptr, 7.0f);
    EventRegisterEx(EVENT_ID_MOUSEMOVE, OnMouseMove, nullptr, 7.0f);
}

// VERIFY: 2345be12 auto
// OFFSET: 0x007660D0
static void UnregisterHandlers() {
    EventUnregister(EVENT_ID_CHAR, OnChar);
    EventUnregister(EVENT_ID_IDLE, OnIdle);
    EventUnregister(EVENT_ID_KEYDOWN, OnKeyDown);
    EventUnregister(EVENT_ID_KEYDOWN_REPEATING, OnKeyDownRepeat);
    EventUnregister(EVENT_ID_KEYUP, OnKeyUp);
    EventUnregister(EVENT_ID_MOUSEDOWN, OnMouseDown);
    EventUnregister(EVENT_ID_MOUSEUP, OnMouseUp);
    EventUnregister(EVENT_ID_MOUSEMOVE, OnMouseMove);
}

// VERIFY: f77e438c auto
// OFFSET: 0x00766140
void ConsoleScreenInitialize(const char* title) {
    CRect windowSize;
    g_theGxDevicePtr->CapsWindowSize(windowSize);

    float width = windowSize.maxX - windowSize.minX;
    s_caretpixwidth = width == 0.0f ? 1.0f : 1.0f / width;

    float height = windowSize.maxY - windowSize.minY;
    s_caretpixheight = height == 0.0f ? 1.0f : 1.0f / height;

    SStrCopy(s_fontName, "Fonts\\ARIALN.ttf", sizeof(s_fontName));
    s_textFont = TextBlockGenerateFont(s_fontName, 0, NDCToDDCHeight(s_fontHeight));

    ScrnLayerCreate(&s_rect, 6.0f, 3, nullptr, PaintBackground, &s_layerBackground);
    ScrnLayerCreate(&s_rect, 7.0f, 3, nullptr, PaintText, &s_layerText);

    RegisterHandlers();

    for (uint32_t i = 0; i < sizeof(s_consoleCommandNames) / sizeof(s_consoleCommandNames[0]); i++) {
        ConsoleCommandRegister(s_consoleCommandNames[i], s_consoleCommandHandlers[i], CATEGORY_CONSOLE, s_consoleCommandHelp[i]);
    }

    EventSetConfirmCloseCallback(EventCloseCallback, nullptr);
    ConsoleCommandExecute("ver", 1);
    s_batch = GxuFontCreateBatch(false, false);
}

// VERIFY: ef444a6a auto
// OFFSET: 0x007662A0
void ConsoleScreenDestroy() {
    EventSetConfirmCloseCallback(nullptr, nullptr);
    UnregisterHandlers();
    HandleClose(reinterpret_cast<HOBJECT>(s_textFont));
    HandleClose(reinterpret_cast<HOBJECT>(s_layerBackground));
    HandleClose(reinterpret_cast<HOBJECT>(s_layerText));
    s_linelist.Clear();

    if (s_batch) {
        GxuFontDestroyBatch(s_batch);
    }

    s_batch = nullptr;

    if (s_inputString) {
        GxuFontDestroyString(s_inputString);
    }

    s_inputString = nullptr;
}

// VERIFY: 0aea2766 auto
// OFFSET: 0x00763C40
void SetConsoleKey(KEY key) {
    s_consoleHotKey = key;
}

// VERIFY: d98debac auto
// OFFSET: 0x00763C50
void ConsolePostClose() {
    EventPostCloseEx(EventGetCurrentContext());
}

// VERIFY: b605948e auto
// OFFSET: 0x00763C60
int32_t ConsoleGetActive() {
    return s_active;
}
