#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

// WMO chunks are byte-packed on disk. Keep each payload naturally aligned and
// independently bounded before the original world core creates runtime views.
class WmoData {
public:
    enum class Stream { File, Children };
    static constexpr size_t MaxBytes = 64 * 1024 * 1024;
    struct Chunk {
        uint32_t token, size;
        std::vector<std::max_align_t> storage;
        void* Data() { return storage.data(); }
        template<class T> T* Records() {
            if (size % sizeof(T)) throw std::runtime_error("WMO chunk has a partial record");
            return static_cast<T*>(Data());
        }
        template<class T> uint32_t Count() { Records<T>(); return size / sizeof(T); }
        void CheckString(uint32_t offset);
    };
    explicit WmoData(const void* bytes, size_t size, Stream stream = Stream::File);
    Chunk& At(size_t index, uint32_t token = 0);
    Chunk* Find(uint32_t token);
    Chunk& Require(uint32_t token);
private:
    std::vector<Chunk> m_chunks;
};
