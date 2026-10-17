#include "WmoData.h"
#include <algorithm>
#include <cstring>

WmoData::WmoData(const void* data, size_t size, Stream stream) {
    if (!data || (stream == Stream::File && size < 12) || size > MaxBytes) throw std::runtime_error("Invalid WMO byte size or resource budget exceeded");
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t offset = 0; offset < size;) {
        if (size - offset < 8 || m_chunks.size() >= 4096) throw std::runtime_error("Invalid WMO chunk table");
        Chunk chunk{};
        std::memcpy(&chunk.token, bytes + offset, 4);
        std::memcpy(&chunk.size, bytes + offset + 4, 4);
        offset += 8;
        if (chunk.size > size - offset) throw std::runtime_error("Truncated WMO chunk payload");
        // Empty chunks still have a valid, zero-initialized address; their record
        // count remains zero and string users must validate before dereferencing.
        chunk.storage.resize(std::max(size_t(1), (chunk.size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t)));
        std::memcpy(chunk.Data(), bytes + offset, chunk.size);
        offset += chunk.size;
        m_chunks.push_back(std::move(chunk));
    }
    if (stream == Stream::File) {
        auto& version = Require(0x4D564552);
        if (m_chunks.front().token != version.token || version.size != 4 || *version.Records<uint32_t>() != 17)
            throw std::runtime_error("Unsupported WMO version");
    }
}
WmoData::Chunk& WmoData::At(size_t index, uint32_t token) {
    if (index >= m_chunks.size() || (token && m_chunks[index].token != token))
        throw std::runtime_error("Invalid WMO group chunk sequence");
    return m_chunks[index];
}
WmoData::Chunk* WmoData::Find(uint32_t token) {
    Chunk* found = nullptr;
    for (auto& chunk : m_chunks) if (chunk.token == token) {
        if (found) throw std::runtime_error("Duplicate WMO root chunk");
        found = &chunk;
    }
    return found;
}
WmoData::Chunk& WmoData::Require(uint32_t token) {
    auto* chunk = Find(token);
    if (!chunk) throw std::runtime_error("Missing WMO root chunk");
    return *chunk;
}
void WmoData::Chunk::CheckString(uint32_t offset) {
    if (offset >= size || !std::memchr(static_cast<const char*>(Data()) + offset, 0, size - offset))
        throw std::runtime_error("Invalid WMO string reference");
}
