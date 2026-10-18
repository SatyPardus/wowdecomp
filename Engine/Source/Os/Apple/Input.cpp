#if defined(__APPLE__)
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Os/OsIME.h"
#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>

namespace {
std::mutex mutex;
struct QueuedInput { OSEVENT event; std::optional<OsInputComposition> composition; };
std::deque<QueuedInput> queue;
OsInputComposition composition;
std::set<KEY> keys;
uint32_t buttons;
int32_t pointerX, pointerY;
uint32_t imeEnableCount;
int32_t language;
bool nativeLanguage = true, secure;
void (*textStateCallback)(void*);
void* textStateOwner;
tagRECT bounds{0, 0, 1, 1};
void (*pollProc)();
}
void OsInputInitialize() {
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear(); keys.clear(); buttons = 0; composition = {}; pointerX = pointerY = 0;
}
void OsInputDestroy() { OsInputSetMouseMode(OS_MOUSE_MODE_NORMAL); OsIMEDestroy(); OsInputEnableSecureMode(0); OsInputInitialize(); pollProc = nullptr; }
void OsInputSetEventPollProc(void (*proc)()) { pollProc = proc; }
void OsInputSetDrawableSize(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(mutex);
    bounds.right = int32_t(width ? width : 1); bounds.bottom = int32_t(height ? height : 1);
}
int32_t OsGetDefaultWindowRect(tagRECT* rect) {
    if (!rect) { return 0; }
    std::lock_guard<std::mutex> lock(mutex);
    *rect = bounds;
    return 1;
}
void OsInputGetQueuedPointer(int32_t* x, int32_t* y) {
    std::lock_guard<std::mutex> lock(mutex);
    if (x) *x = pointerX;
    if (y) *y = pointerY;
}
void OsQueuePut(OSINPUT id, int32_t a, int32_t b, int32_t c, int32_t) {
    if (id == OS_INPUT_STRING) {
        throw std::invalid_argument("Use OsInputQueueText for owned native text, not a 32-bit pointer");
    }
    std::lock_guard<std::mutex> lock(mutex);
    if (id == OS_INPUT_FOCUS) { keys.clear(); buttons = 0; }
    if (id == OS_INPUT_KEY_DOWN) { keys.insert(KEY(a)); }
    if (id == OS_INPUT_KEY_UP) { keys.erase(KEY(a)); }
    if (id == OS_INPUT_MOUSE_DOWN) { buttons |= uint32_t(a); }
    if (id == OS_INPUT_MOUSE_UP) { buttons &= ~uint32_t(a); }
    if (id == OS_INPUT_MOUSE_MOVE || id == OS_INPUT_MOUSE_DOWN || id == OS_INPUT_MOUSE_UP || id == OS_INPUT_MOUSE_WHEEL) {
        pointerX = b; pointerY = c;
    }
    // Preserve transitions even when a native event burst exceeds the original
    // 15-event Win32 ring. A dropped release can leave a game action stuck.
    queue.push_back({{id, {a, b, c, int32_t(OsGetAsyncTimeMs())}}, {}});
}
int32_t OsQueueGet(OSINPUT* id, int32_t* a, int32_t* b, int32_t* c, int32_t* time) {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) { return 0; }
    auto item = std::move(queue.front()); queue.pop_front();
    if (item.composition) composition = std::move(*item.composition);
    const auto& event = item.event;
    *id = event.id; *a = event.param[0]; *b = event.param[1]; *c = event.param[2]; *time = event.param[3];
    return 1;
}
int32_t OsInputGet(OSINPUT* id, int32_t* a, int32_t* b, int32_t* c, int32_t* time) {
    return OsQueueGet(id, a, b, c, time);
}
int32_t OsInputIsKeyDown(KEY key) {
    std::lock_guard<std::mutex> lock(mutex); return keys.count(key) != 0;
}
int32_t OsInputIsMouseButtonDown(MOUSEBUTTON button) {
    std::lock_guard<std::mutex> lock(mutex); return (buttons & button) != 0;
}
void OsInputQueueText(const uint16_t* text, size_t length) {
    if (!text && length) throw std::invalid_argument("Null native text");
    std::lock_guard<std::mutex> lock(mutex);
    const int32_t time = int32_t(OsGetAsyncTimeMs());
    for (size_t i = 0; i < length; ++i) {
        uint32_t ch = text[i];
        if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < length && text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF)
            ch = 0x10000 + ((ch - 0xD800) << 10) + (text[++i] - 0xDC00);
        else if (ch >= 0xD800 && ch <= 0xDFFF) ch = 0xFFFD;
        queue.push_back({{OS_INPUT_CHAR, {int32_t(ch), 1, 0, time}}, {}});
    }
}
void OsInputQueueComposition(int32_t action, const OsInputComposition& value, bool result) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push_back({{OS_INPUT_IME, {action, 0, action == 2 ? (result ? 4 : 3) : 0, int32_t(OsGetAsyncTimeMs())}}, value});
}
static void TextStateChanged() { if (textStateCallback) textStateCallback(textStateOwner); }
void OsInputSetTextStateCallback(void (*callback)(void*), void* owner) { textStateCallback = callback; textStateOwner = owner; }
void OsInputSetLanguage(int32_t value) {
    if (value != language) { language = value; OsQueuePut(OS_INPUT_IME, 0, 0, 0, 0); }
}
bool OsInputAllowsComposition() { return imeEnableCount && nativeLanguage && !secure; }
bool OsInputIsSecure() { return secure; }
void OsInputEnableSecureMode(int32_t enable) { secure = enable != 0; TextStateChanged(); }
void OsIMEEnable(int32_t enable) {
    if (enable) ++imeEnableCount;
    else if (imeEnableCount) --imeEnableCount;
    TextStateChanged();
}
void OsIMESync() { TextStateChanged(); }
void OsIMESetLanguageMode(int32_t native) { nativeLanguage = native != 0; TextStateChanged(); }
void OsIMEInitialize() { imeEnableCount = 0; nativeLanguage = true; TextStateChanged(); }
void OsIMEDestroy() { imeEnableCount = 0; TextStateChanged(); }
int32_t OsIMEGetLanguageMode() { return OsInputAllowsComposition() ? language : 0; }
static int32_t CopyComposition(char* buffer, uint32_t size) {
    if (!buffer || !size) return 0;
    std::lock_guard<std::mutex> lock(mutex);
    size_t count = std::min(composition.text.size(), size_t(size - 1));
    // The original edit box has a bounded UTF-8 buffer; never cut a code point.
    while (count < composition.text.size() && count && (uint8_t(composition.text[count]) & 0xC0) == 0x80) --count;
    std::memcpy(buffer, composition.text.data(), count); buffer[count] = 0;
    return 1;
}
int32_t OsIMEGetCompositionString(char* buffer, uint32_t size) { return CopyComposition(buffer, size); }
int32_t OsIMEGetCompositionResult(char* buffer, uint32_t size) { return CopyComposition(buffer, size); }
int32_t OsIMEGetClauseInfo(uint32_t& start, uint32_t& end, uint32_t& cursor) {
    std::lock_guard<std::mutex> lock(mutex);
    start = composition.selectionStart; end = composition.selectionEnd; cursor = start;
    return 1;
}
int32_t OsIMEGetCandidates(uint32_t, uint32_t& start, uint32_t& size, uint32_t& count, uint32_t& selected, TSGrowableArray<OsIMECandidate>& candidates) {
    // AppKit owns the candidate panel. Do not also create the Win32 IME panel.
    start = size = count = selected = 0; candidates.Clear(); return 0;
}
#endif
