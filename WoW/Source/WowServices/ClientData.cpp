#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include "WoW/Source/WowServices/ArchiveLayout.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/IOMpqUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/Storm/H/storm.h"

namespace {
std::string Substitute(std::string value, const char* token, const std::string& locale) {
    size_t position = 0;
    while ((position = value.find(token, position)) != std::string::npos) {
        value.replace(position, std::strlen(token), locale);
        position += locale.size();
    }
    return value;
}
bool ChineseLocale(const std::string& locale) {
    return locale == "zhCN" || locale == "enCN" || locale == "zhTW" || locale == "enTW";
}
}

void ClientData::CloseArchive::operator()(System_SFile::ARCHIVEREC* archive) const {
    if (archive) { Blizzard::Mopaq::SFileCloseArchive(archive); }
}
ClientData::~ClientData() {
    while (!m_mounts.empty()) { m_mounts.pop_back(); }
    if (m_filePathsSelected) {
        SFile::SetBasePath(m_previousBasePath.c_str());
        SFile::SetDataPath(m_previousDataPath.c_str());
        SFile::SetDataPathAlternate(m_previousAlternatePath.c_str());
        SFile::RebuildHash();
    }
}

std::string ClientData::Resolve(const std::string& relative) const {
    std::string normalized = relative;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    std::filesystem::path path = m_root;
    // Original archive names are case insensitive. Resolve each component to
    // its actual spelling, including on case-sensitive iOS filesystems.
    for (const auto& component : std::filesystem::path(normalized)) {
        if (component == "." || component.empty()) { continue; }
        if (component == ".." || component.is_absolute()) { throw std::runtime_error("Archive path escapes Data root"); }
        std::error_code error;
        const auto exact = path / component;
        if (std::filesystem::exists(exact, error)) { path = exact; continue; }
        if (error) { throw std::system_error(error, "Inspect Data path"); }
        std::filesystem::path found;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (Blizzard::String::EqualI(entry.path().filename().string().c_str(), component.string().c_str())) {
                if (!found.empty()) { throw std::runtime_error("Ambiguous archive filename casing: " + relative); }
                found = entry.path();
            }
        }
        if (found.empty()) { return {}; }
        path = found;
    }
    return path.string();
}

bool ClientData::Open(const std::string& relative, int32_t priority, bool required) {
    const std::string path = Resolve(Substitute(relative, "****", m_locale));
    if (path.empty()) {
        if (required) { throw std::runtime_error("Required archive is missing: " + relative); }
        return false;
    }
    System_SFile::ARCHIVEREC* archive = nullptr;
    if (!Blizzard::Mopaq::SFileOpenArchive(path.c_str(), priority, 0xC00, &archive)) {
        throw std::runtime_error("Cannot mount archive: " + path + " (SFile " +
            std::to_string(Blizzard::Mopaq::SFileGetLastError()) + ")");
    }
    Mount mount{path, {}, priority, std::unique_ptr<System_SFile::ARCHIVEREC, CloseArchive>(archive)};
    m_mounts.push_back(std::move(mount));
    return true;
}

ClientData::ClientData(const std::string& dataRoot, const std::string& requestedLocale) {
    m_root = std::filesystem::canonical(dataRoot).string();
    if (!std::filesystem::is_directory(m_root)) { throw std::runtime_error("Data root is not a directory"); }
    const int availability = Resolve("common.MPQ").empty() ? 1 : 2;
    if (availability == 2) {
        size_t first = 2; // original default: enUS, then cyclic locale fallback
        if (!requestedLocale.empty()) {
            const auto it = std::find_if(ClientLocaleNames.begin(), ClientLocaleNames.end(), [&](const char* name) {
                return Blizzard::String::EqualI(name, requestedLocale.c_str());
            });
            if (it == ClientLocaleNames.end()) { throw std::runtime_error("Unsupported locale: " + requestedLocale); }
            first = size_t(it - ClientLocaleNames.begin());
        }
        for (size_t i = 0; i < ClientLocaleNames.size(); ++i) {
            const char* locale = ClientLocaleNames[(first + i) % ClientLocaleNames.size()];
            if (!Resolve(std::string(locale) + "/locale-" + locale + ".MPQ").empty()) { m_locale = locale; break; }
        }
        if (m_locale.empty()) { throw std::runtime_error("No supported locale archive in Data"); }
    } else {
        m_locale = "----"; // legacy split archive layout
    }

    // Preserve PatchFiles(false)'s two passes and descending lexical sort,
    // followed by Client.cpp's reverse open with increasing priorities.
    std::vector<std::string> patches;
    for (int pass = 0; pass < 2; ++pass) {
        for (const auto& spec : ClientPatchArchiveSpecs) {
            if (spec.m_flag || spec.m_pass != pass) { continue; }
            std::string directory = Substitute(spec.m_directory, "%s", m_locale).substr(5); // configured root replaces the Data directory
            const std::string physical = directory.empty() ? m_root : Resolve(directory);
            if (physical.empty()) { continue; }
            const std::string pattern = Substitute(spec.m_pattern, "%s", m_locale);
            for (const auto& entry : std::filesystem::directory_iterator(physical)) {
                const std::string filename = entry.path().filename().string();
                if (entry.is_regular_file() && Blizzard::File::SimpleGlob(filename.c_str(), pattern.c_str())) {
                    patches.push_back(directory + filename);
                }
            }
        }
        if (pass == 0) {
            std::sort(patches.begin(), patches.end(), [](const std::string& a, const std::string& b) {
                return SStrCmpI(a.c_str(), b.c_str(), STORM_MAX_STR) > 0;
            });
        }
    }
    int32_t priority = 64;
    for (auto it = patches.rbegin(); it != patches.rend(); ++it) { Open(*it, priority++, true); }
    const size_t patchCount = m_mounts.size();
    auto layout = ClientArchiveLayout;
    for (auto& definition : layout) {
        if (definition.m_unk04 == 3 && Open(definition.m_name, priority, false)) {
            definition.m_opened = true;
            ++priority;
        }
    }
    for (size_t patch = 0; patch < patchCount; ++patch) {
        for (const auto& definition : layout) {
            if (definition.m_unk04 != 3 || (!definition.m_opened && !ChineseLocale(m_locale))) { continue; }
            const std::string prefix = std::filesystem::path(definition.m_name).stem().string();
            System_SFile::ARCHIVEREC* archive = nullptr;
            if (!Blizzard::Mopaq::SFileOpenArchiveView(m_mounts[patch].archive.get(), prefix.c_str(), priority, &archive)) {
                throw std::runtime_error("Cannot mount patch view: " + prefix);
            }
            Mount mount{m_mounts[patch].path, prefix, priority++, std::unique_ptr<System_SFile::ARCHIVEREC, CloseArchive>(archive)};
            m_mounts.push_back(std::move(mount));
        }
    }
    int32_t basePriority = 63;
    for (const auto& definition : layout) {
        if (definition.m_unk04 == 3 || definition.m_unk04 == 4) { continue; }
        if (availability & definition.m_unk08) { Open(definition.m_name, basePriority, definition.m_unk04 != 2); }
        --basePriority;
    }

    // Game callers (notably fonts) explicitly request loose-file overrides.
    // Scope that search to this installation instead of the process working
    // directory, which can be / in a native application or iOS simulator.
    const auto root = std::filesystem::path(m_root);
    const auto base = root.parent_path().string();
    const auto data = root.filename().string();
    if (base.size() + 2 > sizeof(SFile::m_basePath) || data.size() + 2 > sizeof(System_SFile2::s_dataPath)) {
        throw std::runtime_error("Configured resource root exceeds the game file API path limit");
    }
    m_previousBasePath = SFile::m_basePath;
    m_previousDataPath = System_SFile2::s_dataPath;
    m_previousAlternatePath = System_SFile2::s_dataPath2;
    SFile::SetBasePath(base.c_str());
    SFile::SetDataPath(data.c_str());
    SFile::SetDataPathAlternate("");
    SFile::RebuildHash();
    m_filePathsSelected = true;
}

ClientData::Asset ClientData::Read(const std::string& name, uint32_t maximumBytes, bool verifyDigest) const {
    IOMpqUnit* opened = nullptr;
    if (!Blizzard::Mopaq::SFileOpenFileEx(nullptr, name.c_str(), 0, &opened)) { throw std::runtime_error("Resource not found: " + name); }
    struct Release { void operator()(IOMpqUnit* file) const { file->Release(); } };
    std::unique_ptr<IOMpqUnit, Release> file(opened);
    const uint32_t size = Blizzard::Mopaq::SFileGetFileSize(opened, nullptr);
    if (size > maximumBytes) { throw std::runtime_error("Resource exceeds requested memory budget: " + name); }
    Asset asset;
    asset.bytes.resize(size);
    uint32_t bytes = 0;
    if (!Blizzard::Mopaq::SFileReadFile(opened, asset.bytes.data(), size, &bytes) || bytes != size) {
        throw std::runtime_error("Resource read failed: " + name);
    }
    char archiveName[260]{};
    Blizzard::Mopaq::SFileGetArchiveName(opened->GetArchive(), archiveName, sizeof(archiveName));
    asset.archive = archiveName;
    const auto* expected = opened->GetMd5();
    if (verifyDigest && expected && *expected != System_SFile::s_emptyMd5) {
        Blizzard::Util::MD5 md5;
        md5.Process(asset.bytes.data(), size);
        uint8_t digest[16];
        md5.Finish(digest);
        if (std::memcmp(digest, expected->m_data, sizeof(digest))) { throw std::runtime_error("Archive MD5 mismatch: " + name); }
        asset.digestVerified = true;
    }
    return asset;
}
