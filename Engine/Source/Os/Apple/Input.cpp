#if defined(__APPLE__)
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/TimeManager.h"
#include <deque>
#include <mutex>
#include <set>
#include <stdexcept>

namespace {
std::mutex mutex;
std::deque<OSEVENT> queue;
std::set<KEY> keys;
uint32_t buttons;
tagRECT bounds{0, 0, 1, 1};
void (*pollProc)();
}
void OsInputInitialize() {
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear(); keys.clear(); buttons = 0;
}
void OsInputDestroy() { OsInputSetMouseMode(OS_MOUSE_MODE_NORMAL); OsInputInitialize(); pollProc = nullptr; }
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
void OsQueuePut(OSINPUT id, int32_t a, int32_t b, int32_t c, int32_t) {
    if (id == OS_INPUT_STRING || id == OS_INPUT_IME) {
        throw std::runtime_error("Native composed text/IME events are not connected");
    }
    std::lock_guard<std::mutex> lock(mutex);
    if (id == OS_INPUT_FOCUS) { keys.clear(); buttons = 0; }
    if (id == OS_INPUT_KEY_DOWN) { keys.insert(KEY(a)); }
    if (id == OS_INPUT_KEY_UP) { keys.erase(KEY(a)); }
    if (id == OS_INPUT_MOUSE_DOWN) { buttons |= uint32_t(a); }
    if (id == OS_INPUT_MOUSE_UP) { buttons &= ~uint32_t(a); }
    // Preserve transitions even when a native event burst exceeds the original
    // 15-event Win32 ring. A dropped release can leave a game action stuck.
    queue.push_back({id, {a, b, c, int32_t(OsGetAsyncTimeMs())}});
}
int32_t OsQueueGet(OSINPUT* id, int32_t* a, int32_t* b, int32_t* c, int32_t* time) {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) { return 0; }
    const auto event = queue.front(); queue.pop_front();
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
#endif
