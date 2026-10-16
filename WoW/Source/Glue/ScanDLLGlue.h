#ifndef WOW_SOURCE_GLUE_SCANDLLGLUE_H
#define WOW_SOURCE_GLUE_SCANDLLGLUE_H

#include <cstdint>

struct ScanDLLThreadParams {
    /* 0000 */ const char* m_dllName;
    /* 0004 */ volatile int32_t* m_status;
    /* 0008 */ const char** m_result;
    /* 000C */ char* m_hackName;
    /* 0010 */ uint32_t m_hackNameSize;
    /* 0014 */ uint8_t* m_okayToLogIn;
    /* 0018 */ uint8_t* m_shouldBlockPlaying;
};

#endif
