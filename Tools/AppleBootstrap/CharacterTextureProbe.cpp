#include "WoW/Common/TextureCache.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/BLPFile/blp.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Event/Event.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int32_t Initialize(const void*, void*) { AsyncFileReadInitialize(0, 20); return 1; }
int32_t Destroy(const void*, void*) { AsyncFileReadDestroy(); return 1; }
struct Context {
    Context() { PropInitialize(); EventInitialize(1, 0); EventCreateContextEx(0, Initialize, Destroy, 1, 0); EventProcessPending(); }
    ~Context() { EventDestroy(); PropDestroy(); }
};
struct Texture {
    void* handle = nullptr;
    explicit Texture(const std::string& name) : handle(TextureCacheCreateTexture(name.c_str())) { Require(handle, "Character texture cache could not allocate an entry"); }
    ~Texture() { TextureCacheDestroyTexture(handle); }
};
struct Scratch {
    std::filesystem::path root;
    Scratch() {
        std::string pattern = (std::filesystem::temp_directory_path() / "wow-chartex-XXXXXX").string();
        Require(mkdtemp(pattern.data()), "Cannot create character texture scratch directory"); root = pattern;
    }
    ~Scratch() { std::error_code error; std::filesystem::remove_all(root, error); }
};
void Write(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc); file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    Require(bool(file), "Cannot write character texture fixture");
}
uint32_t Read32(const uint8_t* bytes) { uint32_t value; std::memcpy(&value, bytes, 4); return value; }
std::vector<uint8_t> Palette(unsigned width, unsigned alphaBits) {
    BLPHeader header{}; header.m_magic = 0x32504c42; header.m_formatVersion = 1; header.m_colorEncoding = 1;
    header.m_preferredFormat = PIXEL_ARGB8888; header.m_alphaSize = alphaBits; header.m_hasMips = 1; header.m_width = header.m_height = width;
    for (unsigned i = 0; i < 256; ++i) header.m_extended.m_palette[i] = {uint8_t(i), uint8_t(255 - i), uint8_t(i * 17), 0};
    size_t size = sizeof(header); unsigned levels = 0;
    for (unsigned side = width; side; side >>= 1, ++levels) {
        header.m_mipOffsets[levels] = size;
        header.m_mipSizes[levels] = side * side + (side * side * alphaBits + 7) / 8;
        size += header.m_mipSizes[levels];
    }
    std::vector<uint8_t> bytes(size); std::memcpy(bytes.data(), &header, sizeof(header));
    for (size_t i = sizeof(header); i < size; ++i) bytes[i] = uint8_t(i * 13 + 7);
    return bytes;
}
void Check(const std::string& name, const std::vector<uint8_t>& bytes, bool wait) {
    CBLPFile reference; Require(reference.Source(const_cast<uint8_t*>(bytes.data()), bytes.size()), "Invalid character texture reference");
    Texture texture(name); auto* entry = static_cast<CACHEENTRY*>(texture.handle);
    Texture alias(name); Require(alias.handle == texture.handle && entry->m_refCount == 2, "Character texture filename cache did not share ownership");
    Require(TextureCacheCopyTexture(texture.handle) == texture.handle && entry->m_refCount == 3, "Character texture copy reference changed");
    TextureCacheDestroyTexture(texture.handle);
    TCTEXTUREINFO info{};
    Require(!TextureCacheGetInfo(texture.handle, info, 0) && !TextureCacheHasMips(texture.handle) && !TextureCacheGetPal(texture.handle) && !TextureCacheGetMip(texture.handle, 0),
        "Character texture published its buffer before validation");
    if (wait) Require(TextureCacheGetInfo(texture.handle, info, 1), "Character texture synchronous wait failed");
    else { AsyncFileReadWaitAll(); Require(TextureCacheGetInfo(texture.handle, info, 0), "Character texture poll did not finish"); }
    Require(TextureCacheHasMips(texture.handle) && !entry->m_asyncObject && !entry->m_loadFailed && entry->m_size == bytes.size(), "Character texture did not finish its complete file");
    Require(info.m_width == reference.m_header.m_width && info.m_height == reference.m_header.m_height && info.m_alphaSize == reference.m_header.m_alphaSize && info.m_mipCount == reference.m_numLevels,
        "Character texture metadata changed");
    for (unsigned i = 0; i < reference.m_numLevels; ++i) {
        Require(!std::memcmp(TextureCacheGetMip(texture.handle, i), bytes.data() + reference.m_header.m_mipOffsets[i], reference.m_header.m_mipSizes[i]), "Character texture authored mip changed");
    }
    auto* palette = TextureCacheGetPal(texture.handle);
    if (reference.m_header.m_colorEncoding == 1) Require(palette && !std::memcmp(palette, reference.m_header.m_extended.m_palette, sizeof(reference.m_header.m_extended.m_palette)), "Character texture palette changed");
    else Require(!palette, "Non-palette character texture exposed a palette");
    Require(!TextureCacheGetMip(texture.handle, reference.m_numLevels), "Character texture exposed a nonexistent mip");
}
}

void CheckCharacterTextureCache(const ClientData* client) {
    Scratch scratch;
    const auto large = Palette(1024, 8);
    Require(large.size() > 1024 * 1024, "Character texture fixture did not cross the original 20-bit size field");
    const auto largePath = scratch.root / "large.blp"; Write(largePath, large);
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        Context context;
        Check(largePath.string(), large, cycle == 0);
        for (unsigned alpha : {0u, 1u, 4u, 8u}) {
            const auto bytes = Palette(32, alpha); const auto path = scratch.root / ("palette-" + std::to_string(alpha) + ".blp");
            Write(path, bytes); Check(path.string(), bytes, cycle == 0);
        }
        const auto longPath = scratch.root / (std::string(220 - scratch.root.string().size() - 5, 'x') + ".blp");
        auto small = Palette(16, 4); Write(longPath, small); Check(longPath.string(), small, true);
        auto invalid = small; invalid.resize(invalid.size() - 1); const auto invalidPath = scratch.root / "invalid.blp"; Write(invalidPath, invalid);
        {
            Texture texture(invalidPath.string()); TCTEXTUREINFO info{};
            TextureCacheGetInfo(texture.handle, info, 1);
            Require(static_cast<CACHEENTRY*>(texture.handle)->m_loadFailed && !info.m_width && !TextureCacheHasMips(texture.handle), "Truncated character texture became ready");
        }
        Write(invalidPath, small); Check(invalidPath.string(), small, true);
        // Hold the original queue lock until the already-opened file is made
        // shorter, so the real worker deterministically takes its failure path.
        const auto failurePath = scratch.root / "short-read.blp"; Write(failurePath, large);
        {
            Texture texture(failurePath.string()); TCTEXTUREINFO info{};
            AsyncFile::EnterQueueLock(); TextureCacheGetInfo(texture.handle, info, 0);
            std::error_code truncateError;
            std::filesystem::resize_file(failurePath, sizeof(BLPHeader), truncateError); AsyncFile::LeaveQueueLock();
            Require(!truncateError, "Cannot create the short-read character texture fixture");
            TextureCacheGetInfo(texture.handle, info, 1);
            auto* entry = static_cast<CACHEENTRY*>(texture.handle);
            Require(entry->m_loadFailed && !entry->m_asyncObject && !entry->m_data && !TextureCacheGetPal(texture.handle), "Character texture worker failure did not release its source");
        }
        Write(failurePath, large); Check(failurePath.string(), large, false);
        for (unsigned i = 0; i < 24; ++i) {
            Texture texture(largePath.string()); TCTEXTUREINFO info{}; TextureCacheGetInfo(texture.handle, info, 0);
        }
        AsyncFileReadWaitAll(); TextureCacheDestroy();
    }
    std::fprintf(stderr, "Original character texture cache: >1 MiB source, four alpha formats, authored mips/palette, long paths, pending ownership, worker failure/retry, cancellation and two lifetimes passed\n");
    if (!client) return;
    const auto dbc = client->Read("DBFilesClient\\CharSections.dbc", 16 * 1024 * 1024).bytes;
    Require(dbc.size() >= 20 && !std::memcmp(dbc.data(), "WDBC", 4) && Read32(dbc.data() + 12) == 40, "Invalid original character sections table");
    const auto rows = Read32(dbc.data() + 4), strings = Read32(dbc.data() + 16);
    const size_t stringStart = 20 + uint64_t(rows) * 40;
    Require(stringStart <= dbc.size() && strings <= dbc.size() - stringStart, "Invalid character section string range");
    std::vector<std::string> names;
    for (unsigned i = 0; i < rows && names.size() < 12; ++i) {
        for (unsigned slot = 0; slot < 3 && names.size() < 12; ++slot) {
            uint32_t offset = Read32(dbc.data() + 20 + i * 40 + 16 + slot * 4);
            Require(offset < strings, "Invalid character skin filename offset");
            const char* text = reinterpret_cast<const char*>(dbc.data() + stringStart + offset);
            Require(std::memchr(text, 0, strings - offset), "Unterminated character skin filename");
            if (*text && std::find(names.begin(), names.end(), text) == names.end()) names.emplace_back(text);
        }
    }
    Require(names.size() == 12, "Original character skin fixtures are missing");
    Context context;
    for (const auto& name : names) Check(name, client->Read(name, 64 * 1024 * 1024).bytes, true);
    TextureCacheDestroy();
    std::fprintf(stderr, "Original character texture cache: %zu real CharSections skin textures retained every reachable mip and palette\n", names.size());
}
