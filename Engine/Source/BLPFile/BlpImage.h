#ifndef ENGINE_SOURCE_BLPFILE_BLPIMAGE_H
#define ENGINE_SOURCE_BLPFILE_BLPIMAGE_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace BLP {
// Packed formats use the original Gx/D3D little-endian bit layout. They are
// upload payloads produced by the texture service, not new BLP encodings.
// UV88 stores signed U then V bytes and must not be expanded into UNORM color.
enum class Format { BGRA8, BC1, BC2, BC3, RGB565, ARGB1555, ARGB4444, UV88 };
inline bool IsBlockCompressed(Format format) {
    return format == Format::BC1 || format == Format::BC2 || format == Format::BC3;
}
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
