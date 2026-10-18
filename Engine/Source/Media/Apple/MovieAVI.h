#pragma once
#include <cstdint>
#include <vector>

namespace AppleMovie {
struct AVI {
    uint32_t width = 0, height = 0, rate = 0, scale = 0, frames = 0;
    // Original MovieFrame representation: little-endian length + each video
    // packet, and unaltered concatenated MP3 packets in idx1 order.
    std::vector<uint8_t> video, audio;
};
bool ReadAVI(const char* path, AVI& output);
}
