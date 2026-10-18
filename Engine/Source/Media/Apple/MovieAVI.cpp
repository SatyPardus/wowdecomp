#include "MovieAVI.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include <array>
#include <cstring>
#include <limits>
#include <utility>

namespace AppleMovie {
namespace {
constexpr uint64_t budget = 256ull * 1024 * 1024;
uint32_t U32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
bool Tag(const uint8_t* p, const char* tag) { return std::memcmp(p, tag, 4) == 0; }
struct File {
    SFileImpl* file = nullptr;
    uint64_t size = 0;
    ~File() { if (file) SFileCloseFile(file); }
    bool Read(uint64_t at, void* data, uint32_t count) {
        if (at > size || count > size - at) return false;
        int32_t high = int32_t(at >> 32);
        if (uint32_t(SFile::SetFilePointer(file, int32_t(at), &high, 0)) != uint32_t(at) ||
            high != int32_t(at >> 32)) return false;
        uint32_t got = 0;
        return !count || (SFileReadFile(file, data, count, &got, nullptr, 0) && got == count);
    }
};
struct Chunk { uint64_t start = 0, data = 0, end = 0, next = 0; uint32_t size = 0; std::array<uint8_t, 4> id{}; };
bool Next(File& file, uint64_t at, uint64_t end, Chunk& chunk) {
    uint8_t header[8];
    if (at > end || end - at < 8 || !file.Read(at, header, 8)) return false;
    chunk.start = at; chunk.data = at + 8; chunk.size = U32(header + 4);
    std::memcpy(chunk.id.data(), header, 4);
    if (chunk.size > end - chunk.data) return false;
    chunk.end = chunk.data + chunk.size; chunk.next = chunk.end + (chunk.size & 1);
    return chunk.next <= end;
}
bool Stream(File& file, const Chunk& list, unsigned number, AVI& avi, int& video, int& audio) {
    std::vector<uint8_t> header, format;
    for (uint64_t at = list.data + 4; at < list.end;) {
        Chunk chunk; if (!Next(file, at, list.end, chunk)) return false;
        if (Tag(chunk.id.data(), "strh") || Tag(chunk.id.data(), "strf")) {
            auto& data = Tag(chunk.id.data(), "strh") ? header : format;
            if (!data.empty() || chunk.size > 65536) return false;
            data.resize(chunk.size);
            if (!file.Read(chunk.data, data.data(), chunk.size)) return false;
        }
        at = chunk.next;
    }
    if (header.size() < 56) return false;
    if (Tag(header.data(), "vids")) {
        if (video >= 0 || format.size() < 40 || U32(format.data()) < 40) return false;
        const auto* codec = format.data() + 16;
        if (!Tag(codec, "DXGM") && !Tag(codec, "DIVX") && !Tag(codec, "DX50") &&
            !Tag(codec, "XVID") && !Tag(codec, "FMP4")) return false;
        avi.scale = U32(header.data() + 20); avi.rate = U32(header.data() + 24);
        avi.frames = U32(header.data() + 32);
        avi.width = U32(format.data() + 4); avi.height = U32(format.data() + 8);
        if (!avi.scale || !avi.rate || uint64_t(avi.rate) > uint64_t(avi.scale) * 1000 || !avi.frames || avi.frames > 1000000 ||
            !avi.width || !avi.height || avi.width > 4096 || avi.height > 4096) return false;
        video = int(number);
    } else if (Tag(header.data(), "auds")) {
        if (audio >= 0 || format.size() < 16 || format[0] != 0x55 || format[1]) return false;
        audio = int(number);
    }
    return true;
}
bool Header(File& file, const Chunk& list, AVI& avi, int& video, int& audio) {
    unsigned stream = 0;
    for (uint64_t at = list.data + 4; at < list.end;) {
        Chunk chunk; if (!Next(file, at, list.end, chunk)) return false;
        if (Tag(chunk.id.data(), "LIST")) {
            uint8_t kind[4];
            if (chunk.size < 4 || !file.Read(chunk.data, kind, 4)) return false;
            if (Tag(kind, "strl") && (stream >= 100 || !Stream(file, chunk, stream++, avi, video, audio))) return false;
        }
        at = chunk.next;
    }
    return video >= 0;
}
}

bool ReadAVI(const char* path, AVI& output) {
    // Publish only a complete, validated result. Failed/retried opens never
    // expose partially read packets or overwrite an existing movie.
    try {
        File file;
        if (!path || !SFile::OpenEx(nullptr, path, 1, &file.file)) return false;
        uint32_t high = 0; const uint32_t low = SFileGetFileSize(file.file, &high);
        file.size = uint64_t(high) << 32 | low;
        if (file.size > budget) return false;
        uint8_t riff[12];
        if (!file.Read(0, riff, 12) || !Tag(riff, "RIFF") || !Tag(riff + 8, "AVI ")) return false;
        const uint64_t end = uint64_t(U32(riff + 4)) + 8;
        if (end < 12 || end > file.size) return false;
        AVI avi; Chunk movie, index; int video = -1, audio = -1; bool header = false;
        for (uint64_t at = 12; at < end;) {
            Chunk chunk; if (!Next(file, at, end, chunk)) return false;
            if (Tag(chunk.id.data(), "LIST")) {
                uint8_t kind[4];
                if (chunk.size < 4 || !file.Read(chunk.data, kind, 4)) return false;
                if (Tag(kind, "hdrl")) {
                    if (header || !Header(file, chunk, avi, video, audio)) return false;
                    header = true;
                } else if (Tag(kind, "movi")) {
                    if (movie.size) return false;
                    movie = chunk;
                }
            } else if (Tag(chunk.id.data(), "idx1")) {
                if (index.size) return false;
                index = chunk;
            }
            at = chunk.next;
        }
        if (!header || !movie.size || !index.size || index.size % 16 || index.size > 32 * 1024 * 1024) return false;
        std::vector<uint8_t> entries(index.size);
        if (!file.Read(index.data, entries.data(), index.size)) return false;
        uint64_t videoSize = 0, audioSize = 0; uint32_t frames = 0;
        for (size_t i = 0; i < entries.size(); i += 16) {
            const auto* entry = entries.data() + i;
            if (entry[0] < '0' || entry[0] > '9' || entry[1] < '0' || entry[1] > '9') return false;
            const int stream = 10 * (entry[0] - '0') + entry[1] - '0';
            const uint32_t size = U32(entry + 12);
            const uint64_t at = movie.data + U32(entry + 8);
            Chunk packet;
            if (at < movie.data + 4 || !Next(file, at, movie.end, packet) ||
                packet.size != size || std::memcmp(packet.id.data(), entry, 4)) return false;
            if (stream == video && entry[2] == 'd' && (entry[3] == 'c' || entry[3] == 'b')) {
                if (size > 16 * 1024 * 1024) return false;
                videoSize += uint64_t(size) + 4; ++frames;
            } else if (stream == audio && entry[2] == 'w' && entry[3] == 'b') {
                audioSize += size;
            } else return false;
            if (videoSize + audioSize > budget) return false;
        }
        if (frames != avi.frames) return false;
        avi.video.resize(size_t(videoSize)); avi.audio.resize(size_t(audioSize));
        size_t v = 0, a = 0;
        for (size_t i = 0; i < entries.size(); i += 16) {
            const auto* entry = entries.data() + i;
            const uint32_t size = U32(entry + 12);
            const uint64_t at = movie.data + U32(entry + 8) + 8;
            if (entry[2] == 'd') {
                std::memcpy(avi.video.data() + v, entry + 12, 4); v += 4;
                if (!file.Read(at, avi.video.data() + v, size)) return false;
                v += size;
            } else {
                if (!file.Read(at, avi.audio.data() + a, size)) return false;
                a += size;
            }
        }
        output = std::move(avi);
        return true;
    } catch (...) { return false; }
}
}
