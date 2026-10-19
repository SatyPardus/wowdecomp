#include "MapLayoutData.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(std::string("Invalid WDT: ") + message);
}
bool Finite(const C3Vector& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
}

MapLayoutData::MapLayoutData(const void* bytes, size_t size) {
    static_assert(sizeof(SMMapHeader) == 32 && sizeof(SMAreaInfo) == 8 && sizeof(SMMapObjDef) == 64,
                  "WDT disk layout must not depend on host pointers");
    Require(bytes && size >= 12 && size <= MaxBytes, "invalid file size or budget exceeded");
    const auto* data = static_cast<const uint8_t*>(bytes);
    size_t at = 0;
    auto block = [&](uint32_t expected, size_t minimum, size_t maximum) {
        Require(at <= size && size - at >= 8, "truncated chunk header");
        uint32_t token, count;
        std::memcpy(&token, data + at, 4); std::memcpy(&count, data + at + 4, 4);
        Require(token == expected && count >= minimum && count <= maximum, "unexpected chunk or size");
        Require(count <= size - at - 8, "chunk exceeds file");
        const auto* result = data + at + 8; at += 8 + count;
        return std::pair<const uint8_t*, size_t>(result, count);
    };
    std::memcpy(&version, block(0x4D564552, 4, 4).first, 4);
    Require(version == 18, "unsupported version");
    std::memcpy(&header, block(0x4D504844, sizeof(header), sizeof(header)).first, sizeof(header));
    std::memcpy(areas.data(), block(0x4D41494E, sizeof(areas), sizeof(areas)).first, sizeof(areas));
    if (header.m_flags & 1) {
        const auto names = block(0x4D574D4F, 1, MaxBytes);
        const auto* end = static_cast<const uint8_t*>(std::memchr(names.first, 0, names.second));
        Require(end && end != names.first, "missing global WMO filename or terminator");
        // Preserve the original loader's 256-byte filename capacity without
        // overflowing it; extra names in the chunk are ignored as originally.
        Require(size_t(end - names.first) < 256, "global WMO filename is too long");
        globalName.assign(reinterpret_cast<const char*>(names.first), size_t(end - names.first));
        const auto objects = block(0x4D4F4446, sizeof(globalObject), MaxBytes);
        Require(objects.second % sizeof(globalObject) == 0, "partial global WMO placement");
        std::memcpy(&globalObject, objects.first, sizeof(globalObject));
        Require(Finite(globalObject.m_position) && Finite(globalObject.m_rotation) &&
                Finite(globalObject.m_extents.b) && Finite(globalObject.m_extents.t), "non-finite global WMO placement");
    }
    // The original loader does not use trailing chunks (including an empty
    // MWMO on ADT maps). Still reject truncated data rather than reading it.
    while (at < size) {
        Require(size - at >= 8, "truncated trailing chunk header");
        uint32_t count; std::memcpy(&count, data + at + 4, 4);
        Require(count <= size - at - 8, "trailing chunk exceeds file");
        at += size_t(count) + 8;
    }
}
