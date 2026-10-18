#ifndef ENGINE_SOURCE_OS_APPLE_INPUT_H
#define ENGINE_SOURCE_OS_APPLE_INPUT_H
#include <cstdint>
#include <string>
#include "Engine/Source/Os/Input.h"

// Native hosts deliver drawable pixel dimensions and input through OsQueuePut.
void OsInputSetDrawableSize(uint32_t width, uint32_t height);
// Last absolute native event position, initially the virtual drawable origin.
// Apply the saved resize policy when a native host view attaches/detaches.
void OsInputApplyWindowResizeLock(void* nativeView);
void OsInputGetQueuedPointer(int32_t* x, int32_t* y);

// Copy native text into the queue. Never put a pointer in OSEVENT's 32-bit words.
void OsInputQueueText(const uint16_t* text, size_t length);
struct OsInputComposition {
    std::string text;
    uint32_t selectionStart = 0, selectionEnd = 0; // Unicode scalar indices.
};
void OsInputQueueComposition(int32_t action, const OsInputComposition& composition, bool result = false);
void OsInputSetLanguage(int32_t language); // Original OsIME language codes.
bool OsInputAllowsComposition();
bool OsInputIsSecure();
void OsInputSetTextStateCallback(void (*callback)(void*), void* owner);
OS_MOUSE_MODE OsInputGetMouseMode();
#endif
