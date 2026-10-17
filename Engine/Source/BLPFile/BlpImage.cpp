#include "Engine/Source/BLPFile/BlpImage.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace {
uint16_t U16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
uint32_t U32(const uint8_t* p) { return uint32_t(U16(p)) | uint32_t(U16(p + 2)) << 16; }
void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}
using Color = std::array<uint8_t, 4>; // BGRA, straight alpha
Color RGB565(uint16_t value) {
    const uint8_t b = value & 31, g = (value >> 5) & 63, r = value >> 11;
    return {uint8_t((b << 3) | (b >> 2)), uint8_t((g << 2) | (g >> 4)), uint8_t((r << 3) | (r >> 2)), 255};
}
}

const char* BLP::FormatName(Format format) {
    switch (format) {
        case Format::BGRA8: return "BGRA8";
        case Format::BC1: return "BC1";
        case Format::BC2: return "BC2";
        case Format::BC3: return "BC3";
        case Format::RGB565: return "RGB565";
        case Format::ARGB1555: return "ARGB1555";
        case Format::ARGB4444: return "ARGB4444";
    }
    return "unknown";
}

BLP::Image BLP::Image::Load(const uint8_t* data, size_t size, size_t memoryBudget) {
    Require(data && size >= 148, "Truncated BLP header");
    Require(U32(data) == 0x32504C42 && U32(data + 4) == 1, "Unsupported BLP signature/version");
    const uint8_t encoding = data[8], alphaBits = data[9], preferred = data[10];
    uint32_t width = U32(data + 12), height = U32(data + 16);
    Require(width && height && width <= 16384 && height <= 16384, "BLP dimensions exceed native texture limits");
    Require(alphaBits == 0 || alphaBits == 1 || alphaBits == 4 || alphaBits == 8, "Unsupported BLP alpha precision");
    Image image;
    size_t headerBytes = 148;
    if (encoding == 1) {
        headerBytes += 1024;
        Require(size >= headerBytes, "Truncated BLP palette");
    } else if (encoding == 2) {
        if (preferred == 0) { image.format = Format::BC1; }
        else if (preferred == 1) { image.format = Format::BC2; }
        else if (preferred == 7) { image.format = Format::BC3; }
        else { throw std::runtime_error("Unsupported BLP block format"); }
    } else {
        Require(encoding == 3 && preferred == 2, "Unsupported BLP color encoding (including legacy JPEG)");
    }
    const bool hasMips = (data[11] & 15) != 0;
    size_t used = 0;
    for (uint32_t level = 0; level < 16; ++level) {
        const uint32_t offset = U32(data + 20 + level * 4), storedBytes = U32(data + 84 + level * 4);
        const size_t pixels = size_t(width) * height;
        const uint32_t blockBytes = image.format == Format::BC1 ? 8 : 16;
        const uint32_t rowBytes = image.format == Format::BGRA8 ? width * 4 : ((width + 3) / 4) * blockBytes;
        const size_t outputBytes = image.format == Format::BGRA8 ? pixels * 4 : size_t(rowBytes) * ((height + 3) / 4);
        const size_t inputBytes = encoding == 1 ? pixels + (pixels * alphaBits + 7) / 8 : outputBytes;
        Require(offset >= headerBytes && offset <= size && storedBytes <= size - offset && storedBytes >= inputBytes,
            "Missing or truncated BLP mip level");
        // Budget also covers full-resolution expansion on devices without BC.
        const size_t budgetBytes = std::max(outputBytes, pixels * 4);
        Require(budgetBytes <= memoryBudget - used, "BLP exceeds texture memory budget");
        used += budgetBytes;
        const uint8_t* source = data + offset;
        Mip mip{width, height, rowBytes, std::vector<uint8_t>(outputBytes)};
        if (encoding == 1) {
            for (size_t i = 0; i < pixels; ++i) {
                const uint8_t* color = data + 148 + size_t(source[i]) * 4;
                std::memcpy(mip.bytes.data() + i * 4, color, 3);
                uint8_t alpha = 255;
                if (alphaBits) {
                    const unsigned mask = (1u << alphaBits) - 1;
                    alpha = uint8_t(((source[pixels + i * alphaBits / 8] >> (i * alphaBits % 8)) & mask) * 255 / mask);
                }
                mip.bytes[i * 4 + 3] = alpha;
            }
        } else { std::memcpy(mip.bytes.data(), source, outputBytes); }
        image.mips.push_back(std::move(mip));
        if (!hasMips || (width == 1 && height == 1)) { break; }
        width = std::max(1u, width >> 1);
        height = std::max(1u, height >> 1);
    }
    return image;
}

BLP::Mip BLP::Image::DecodeBGRA(size_t level) const {
    const Mip& source = mips.at(level);
    if (format == Format::BGRA8) { return source; }
    Mip result{source.width, source.height, source.width * 4, std::vector<uint8_t>(size_t(source.width) * source.height * 4)};
    if (!IsBlockCompressed(format)) {
        // Diagnostic readback only: packed UNORM5/6 stays packed on the GPU,
        // since an 8-bit expansion would change values before filtering.
        for (uint32_t y = 0; y < source.height; ++y) {
            for (uint32_t x = 0; x < source.width; ++x) {
                const uint16_t packed = U16(source.bytes.data() + size_t(y) * source.rowBytes + x * 2);
                auto* pixel = result.bytes.data() + size_t(y) * result.rowBytes + x * 4;
                if (format == Format::ARGB4444) {
                    for (unsigned c = 0; c < 4; ++c) { pixel[c] = ((packed >> (c * 4)) & 15) * 17; }
                } else {
                    const unsigned greenBits = format == Format::RGB565 ? 6 : 5;
                    const unsigned greenMask = (1u << greenBits) - 1;
                    pixel[0] = ((packed & 31) * 255 + 15) / 31;
                    pixel[1] = (((packed >> 5) & greenMask) * 255 + greenMask / 2) / greenMask;
                    pixel[2] = (((packed >> (5 + greenBits)) & 31) * 255 + 15) / 31;
                    pixel[3] = format == Format::RGB565 || (packed & 0x8000) ? 255 : 0;
                }
            }
        }
        return result;
    }
    const uint32_t blockBytes = format == Format::BC1 ? 8 : 16;
    for (uint32_t by = 0; by < source.height; by += 4) {
        for (uint32_t bx = 0; bx < source.width; bx += 4) {
            const uint8_t* block = source.bytes.data() + size_t(by / 4) * source.rowBytes + (bx / 4) * blockBytes;
            const uint8_t* colorBlock = block + (format == Format::BC1 ? 0 : 8);
            const uint16_t c0 = U16(colorBlock), c1 = U16(colorBlock + 2);
            std::array<Color, 4> colors{RGB565(c0), RGB565(c1), Color{0, 0, 0, 255}, Color{0, 0, 0, 255}};
            // D3D9 BC1 transparent mode; BC2/3 always use four colors.
            // See Microsoft "Opaque and 1-Bit Alpha Textures (Direct3D 9)".
            if (format == Format::BC1 && c0 <= c1) {
                for (size_t c = 0; c < 3; ++c) { colors[2][c] = (unsigned(colors[0][c]) + colors[1][c]) / 2; }
                colors[3] = {0, 0, 0, 0};
            } else {
                for (size_t c = 0; c < 3; ++c) {
                    colors[2][c] = (2 * unsigned(colors[0][c]) + colors[1][c] + 1) / 3;
                    colors[3][c] = (unsigned(colors[0][c]) + 2 * colors[1][c] + 1) / 3;
                }
            }
            std::array<uint8_t, 8> alpha{};
            uint64_t alphaIndices = 0;
            if (format == Format::BC3) {
                alpha[0] = block[0]; alpha[1] = block[1];
                const unsigned denominator = alpha[0] > alpha[1] ? 7 : 5;
                for (unsigned i = 1; i < denominator; ++i) {
                    alpha[i + 1] = ((denominator - i) * alpha[0] + i * alpha[1] + denominator / 2) / denominator;
                }
                if (denominator == 5) { alpha[6] = 0; alpha[7] = 255; }
                for (unsigned i = 0; i < 6; ++i) { alphaIndices |= uint64_t(block[2 + i]) << (i * 8); }
            }
            const uint32_t indices = U32(colorBlock + 4);
            for (uint32_t y = 0; y < std::min(4u, source.height - by); ++y) {
                for (uint32_t x = 0; x < std::min(4u, source.width - bx); ++x) {
                    const unsigned pixel = y * 4 + x;
                    Color color = colors[(indices >> (pixel * 2)) & 3];
                    if (format == Format::BC2) { color[3] = ((block[pixel / 2] >> ((pixel % 2) * 4)) & 15) * 17; }
                    if (format == Format::BC3) { color[3] = alpha[(alphaIndices >> (pixel * 3)) & 7]; }
                    std::memcpy(result.bytes.data() + size_t(by + y) * result.rowBytes + (bx + x) * 4, color.data(), 4);
                }
            }
        }
    }
    return result;
}
