#ifndef ENGINE_SOURCE_OS_APPLE_INPUT_H
#define ENGINE_SOURCE_OS_APPLE_INPUT_H
#include <cstdint>

// Native hosts deliver drawable pixel dimensions and input through OsQueuePut.
void OsInputSetDrawableSize(uint32_t width, uint32_t height);
#endif
