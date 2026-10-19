#include "MapLowDetailData.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace {
void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(std::string("Invalid WDL: ") + message);
}
struct Bytes {
    const uint8_t* data;
    size_t size;
    Bytes Slice(size_t offset, size_t count) const {
        Require(offset <= size && count <= size - offset, "chunk exceeds file");
        return {data + offset, count};
    }
    template<class T> T Value(size_t offset) const {
        T value; std::memcpy(&value, Slice(offset, sizeof(T)).data, sizeof(T)); return value;
    }
    template<class T> std::vector<T> Records() const {
        Require(size % sizeof(T) == 0, "partial table record");
        std::vector<T> result(size / sizeof(T));
        if (size) std::memcpy(result.data(), data, size);
        return result;
    }
};
bool Finite(const C3Vector& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
}

MapLowDetailData::MapLowDetailData(const void* bytes, size_t size) {
    static_assert(sizeof(SMMapObjDef) == 64, "WDL object disk layout");
    Require(bytes && size >= 12 && size <= MaxBytes, "invalid file size or budget exceeded");
    const Bytes file{static_cast<const uint8_t*>(bytes), size};
    std::unordered_map<size_t, Bytes> chunks;
    for (size_t at = 0; at < size;) {
        Require(chunks.size() < 16384, "too many chunks");
        const auto payload = file.Slice(at + 8, file.Value<uint32_t>(at + 4));
        chunks.emplace(at, payload);
        at += payload.size + 8;
    }
    auto block = [&](size_t offset, uint32_t token) {
        const auto found = chunks.find(offset);
        Require(found != chunks.end() && file.Value<uint32_t>(offset) == token, "invalid chunk reference");
        return found->second;
    };
    const auto version = block(0, 0x4D564552);
    Require(version.size == 4 && version.Value<uint32_t>(0) == 18, "unsupported version");
    size_t at = 12;
    if (file.Value<uint32_t>(at) == 0x4D574D4F) {
        const auto mwmo = block(at, 0x4D574D4F); names = mwmo.Records<char>(); at += mwmo.size + 8;
        const auto mwid = block(at, 0x4D574944); nameOffsets = mwid.Records<uint32_t>(); at += mwid.size + 8;
        const auto modf = block(at, 0x4D4F4446); objects = modf.Records<SMMapObjDef>(); at += modf.size + 8;
        for (const auto offset : nameOffsets)
            Require(offset < names.size() && names[offset] && std::memchr(names.data() + offset, 0, names.size() - offset),
                    "invalid filename offset or missing terminator");
        for (const auto& object : objects)
            Require(object.m_nameId < nameOffsets.size() && Finite(object.m_position) && Finite(object.m_rotation) &&
                    Finite(object.m_extents.b) && Finite(object.m_extents.t), "invalid WMO placement");
    }
    const auto offsets = block(at, 0x4D414F46);
    Require(offsets.size == sizeof(areaOffsets), "incomplete area index");
    std::memcpy(areaOffsets.data(), offsets.data, offsets.size);
    areaIndices.fill(-1);
    size_t count = 0;
    for (const auto offset : areaOffsets) count += offset != 0;
    areas.reserve(count);
    for (size_t i = 0; i < areaOffsets.size(); ++i) {
        const auto offset = areaOffsets[i];
        if (!offset) continue;
        const auto mare = block(offset, 0x4D415245);
        Area area;
        Require(mare.size == sizeof(area.heights), "invalid height grid");
        std::memcpy(area.heights.data(), mare.data, mare.size);
        const size_t next = size_t(offset) + 8 + mare.size;
        if (next < size && file.Value<uint32_t>(next) == 0x4D41484F) {
            const auto maho = block(next, 0x4D41484F);
            Require(maho.size == sizeof(area.holes), "invalid hole grid");
            std::memcpy(area.holes.data(), maho.data, maho.size);
            area.hasHoles = true;
        }
        areaIndices[i] = int32_t(areas.size());
        areas.push_back(area);
    }
}
