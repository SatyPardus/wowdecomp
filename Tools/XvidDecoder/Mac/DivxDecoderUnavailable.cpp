#if defined(__APPLE__)

#include "ThirdParty/DivxDecoder/DivxDecoder.h"

extern "C" {

int32_t InitializeDivxDecoder(int32_t decoder, int32_t width, int32_t height) {
    static_cast<void>(decoder);
    static_cast<void>(width);
    static_cast<void>(height);
    return 1;
}

int32_t SetOutputFormat(int32_t decoder, int32_t format, int32_t width, int32_t height) {
    static_cast<void>(decoder);
    static_cast<void>(format);
    static_cast<void>(width);
    static_cast<void>(height);
    return 1;
}

int32_t DivxDecode(int32_t decoder, DivxDecodeFrame* frame, int32_t flags) {
    static_cast<void>(decoder);
    static_cast<void>(frame);
    static_cast<void>(flags);
    return 1;
}

int32_t UnInitializeDivxDecoder(int32_t decoder) {
    static_cast<void>(decoder);
    return 1;
}

}

#endif
