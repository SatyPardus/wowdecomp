#ifndef ENGINE_SOURCE_BLPFILE_BLPIMAGE_H
#define ENGINE_SOURCE_BLPFILE_BLPIMAGE_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace BLP {
enum class Format { BGRA8, BC1, BC2, BC3 };
struct Mip {
    uint32_t width, height, rowBytes;
    std::vector<uint8_t> bytes;
};

// Bounded, owning image data for native graphics upload. Palette expansion
// preserves all color/alpha bits; BC blocks and authored mip levels are retained.
class Image {
public:
    static Image Load(const uint8_t* data, size_t size, size_t memoryBudget);
    Format format = Format::BGRA8;
    std::vector<Mip> mips;
    Mip DecodeBGRA(size_t level) const;
};
const char* FormatName(Format format);
}
#endif
