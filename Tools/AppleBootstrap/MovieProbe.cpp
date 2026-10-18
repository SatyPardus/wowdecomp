#include "Engine/Source/Media/Apple/MovieAVI.h"
#include "ThirdParty/DivxDecoder/DivxDecoder.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <unistd.h>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
uint32_t U32(const uint8_t* p) { uint32_t n; std::memcpy(&n, p, 4); return n; }
struct Decoder {
    int id;
    Decoder(int id, unsigned w, unsigned h) : id(id) { Require(!InitializeDivxDecoder(id, int(w), int(h)), "Movie decoder create failed"); }
    ~Decoder() { UnInitializeDivxDecoder(id); }
};
void Malformed(const std::vector<uint8_t>& original, size_t index) {
    auto pattern = (std::filesystem::temp_directory_path() / "wow-movie-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end()); name.push_back(0);
    const int fd = mkstemp(name.data()); Require(fd >= 0, "Cannot create AVI check file"); close(fd);
    struct Cleanup { const char* path; ~Cleanup() { unlink(path); } } cleanup{name.data()};
    auto check = [&](const std::vector<uint8_t>& bytes, bool valid) {
        { std::ofstream file(name.data(), std::ios::binary | std::ios::trunc);
          file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); Require(bool(file), "Cannot write AVI check file"); }
        AppleMovie::AVI output; output.width = 123; output.video = {1, 2, 3};
        Require(AppleMovie::ReadAVI(name.data(), output) == valid, "Malformed AVI acceptance changed");
        if (!valid) Require(output.width == 123 && output.video == std::vector<uint8_t>({1, 2, 3}), "Failed AVI read published partial data");
    };
    check({original.begin(), original.begin() + 11}, false);
    auto bad = original; bad[4] = bad[5] = bad[6] = bad[7] = 0xff; check(bad, false);
    bad = original; bad[16] = bad[17] = bad[18] = bad[19] = 0xff; check(bad, false);
    bad = original; std::fill_n(bad.data() + index + 8, 4, 0xff); check(bad, false);
    bad = original; bad[index + 12] ^= 1; check(bad, false);
    bad = original; bad.resize(index + 7); check(bad, false);
    // The valid retry includes the original trailing JUNK outside RIFF's size.
    check(original, true);
}
}
void CheckMovieResources(const std::string& root, std::ostream& report) {
    namespace fs = std::filesystem;
    unsigned movies = 0, decoded = 0;
    uint64_t videoBytes = 0, audioBytes = 0;
    bool malformed = false;
    for (const auto& locale : fs::directory_iterator(root)) {
        const auto directory = locale.path() / "Interface/Cinematics";
        if (!fs::is_directory(directory)) continue;
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.path().extension() != ".avi") continue;
            AppleMovie::AVI avi;
            Require(AppleMovie::ReadAVI(entry.path().c_str(), avi), "Original AVI parse failed");
            std::ifstream source(entry.path(), std::ios::binary);
            std::vector<uint8_t> original((std::istreambuf_iterator<char>(source)), {});
            // Independently compare the actual idx1 bytes and stream headers.
            size_t movie = 0, index = 0, count = 0;
            for (size_t at = 12; at + 8 <= uint64_t(U32(original.data() + 4)) + 8;) {
                const uint32_t n = U32(original.data() + at + 4);
                if (!std::memcmp(original.data() + at, "LIST", 4) && !std::memcmp(original.data() + at + 8, "movi", 4)) movie = at + 8;
                if (!std::memcmp(original.data() + at, "idx1", 4)) { index = at + 8; count = n; }
                at += 8 + n + (n & 1);
            }
            Require(movie && index, "Movie reference lacks index");
            if (!malformed && entry.path().filename() == "Logo_1024.avi") { Malformed(original, index); malformed = true; }
            size_t v = 0, a = 0;
            for (size_t at = index; at < index + count; at += 16) {
                const uint8_t* e = original.data() + at;
                const uint32_t size = U32(e + 12), offset = U32(e + 8);
                if (e[2] == 'd') {
                    Require(U32(avi.video.data() + v) == size, "AVI packet length changed"); v += 4;
                    Require(!std::memcmp(avi.video.data() + v, original.data() + movie + offset + 8, size), "AVI video packet changed"); v += size;
                } else {
                    Require(!std::memcmp(avi.audio.data() + a, original.data() + movie + offset + 8, size), "AVI audio packet changed"); a += size;
                }
            }
            Require(v == avi.video.size() && a == avi.audio.size(), "AVI concatenated data length changed");
            videoBytes += v; audioBytes += a;
            const unsigned frames = std::min(avi.frames, entry.path().filename().string().find("Logo") == 0 ? 96u : 12u);
            const size_t imageSize = size_t(avi.width) * avi.height * 4;
            std::vector<uint8_t> full(imageSize + 32, 0xcd), skipped(imageSize + 32, 0xcd);
            Decoder one(1, avi.width, avi.height), two(2, avi.width, avi.height);
            Require(!SetOutputFormat(1, 1, avi.width, avi.height) &&
                SetOutputFormat(1, 2, avi.width, avi.height) &&
                SetOutputFormat(1, 1, avi.width + 1, avi.height), "Movie output format validation failed");
            v = 0;
            for (unsigned frame = 0; frame < frames; ++frame) {
                const uint32_t size = U32(avi.video.data() + v); v += 4;
                DivxDecodeFrame input{};
                input.m_input = avi.video.data() + v; input.m_inputSize = size;
                input.m_render = 1; input.m_output = full.data() + 16;
                Require(!DivxDecode(1, &input, 0), "Original AVI frame decode failed");
                input.m_render = frame + 1 == frames; input.m_output = skipped.data() + 16;
                Require(!DivxDecode(2, &input, 0), "Movie skipped-frame decode failed");
                v += size; ++decoded;
            }
            Require(full == skipped, "Movie reference frames changed when display was skipped");
            Require(std::all_of(full.begin(), full.begin() + 16, [](uint8_t c) { return c == 0xcd; }) &&
                std::all_of(full.end() - 16, full.end(), [](uint8_t c) { return c == 0xcd; }), "Movie decoder wrote outside image");
            Require(std::any_of(full.begin() + 16, full.end() - 16, [](uint8_t c) { return c != 0xcd; }), "Movie decoder produced no picture");
            ++movies;
        }
    }
    Require(movies != 0, "No original AVI movies found");
    Require(InitializeDivxDecoder(0, 1024, 512) && InitializeDivxDecoder(1, -1, 512) &&
        InitializeDivxDecoder(16, 1024, 512) && UnInitializeDivxDecoder(1), "Invalid movie decoder lifetime accepted");
    report << "Original movies: " << movies << " AVI files, " << videoBytes << " video / " << audioBytes
           << " MP3 bytes preserved; " << decoded << " frames decoded with identical skipped-frame references.\n";
}
