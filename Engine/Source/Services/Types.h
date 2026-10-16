#ifndef ENGINE_SOURCE_SERVICES_TYPES_H
#define ENGINE_SOURCE_SERVICES_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class CAsyncObject;
class CAsyncQueue;
class SFileImpl;

class CAsyncObject {
    public:
    /* 0000 */ SFileImpl* m_file;
    /* 0004 */ void* m_buffer;
    /* 0008 */ uint32_t m_size;
    /* 000C */ void* m_userArg;
    /* 0010 */ void* m_userPostloadCallback;
    /* 0014 */ void* m_userPostUnloadCallback;
    /* 0018 */ CAsyncQueue* m_queue;
    /* 001C */ uint32_t m_queuedFrame;
    /* 0020 */ uint8_t m_priority;
    /* 0021 */ int8_t m_isProcessed;
    /* 0022 */ int8_t m_isRead;
    /* 0023 */ int8_t m_isCurrent;
    /* 0024 */ int8_t m_char24;
    /* 0025 */ int8_t m_char25;
    /* 0026 */ uint16_t m_padding;
    /* 0028 */ TSLink<CAsyncObject> m_link;

    int32_t IsPending();
};

struct HTEXTBLOCK__ {
    /* 0000 */ int32_t m_unused;
};

typedef HTEXTBLOCK__* HTEXTBLOCK;

struct HTEXTFONT__ {
    /* 0000 */ int32_t m_unused;
};

typedef HTEXTFONT__* HTEXTFONT;

enum PIXEL_FORMAT {
    PIXEL_DXT1 = 0,
    PIXEL_DXT3 = 1,
    PIXEL_ARGB8888 = 2,
    PIXEL_ARGB1555 = 3,
    PIXEL_ARGB4444 = 4,
    PIXEL_RGB565 = 5,
    PIXEL_A8 = 6,
    PIXEL_DXT5 = 7,
    PIXEL_UNSPECIFIED = 8,
    PIXEL_ARGB2565 = 9,
    NUM_PIXEL_FORMATS = 10,
};

#endif
