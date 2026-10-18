#include <cstdint>
#include "WoW/Source/WowServices/PatchFiles.h"
#include "Engine/Source/Base/Sort.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"
#if defined(__APPLE__)
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {
std::string PatchDirectory(const std::filesystem::path& input) {
    std::filesystem::path path = input.root_path();
    for (const auto& component : input.relative_path()) {
        if (component.empty() || component == ".") continue;
        std::error_code error;
        const auto exact = path / component;
        if (std::filesystem::is_directory(exact, error)) { path = exact; continue; }
        if (error && error != std::errc::no_such_file_or_directory) throw std::system_error(error, "Inspect patch directory");
        std::filesystem::path found;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!SStrCmpI(entry.path().filename().string().c_str(), component.string().c_str(), STORM_MAX_STR) && entry.is_directory()) {
                if (!found.empty()) throw std::runtime_error("Ambiguous patch directory casing: " + input.string());
                found = entry.path();
            }
        }
        if (found.empty()) return {};
        path = found;
    }
    return path.string();
}
}
#endif

// VERIFY: 204ba559 auto
// OFFSET: 0x00401200
static int32_t CompareStrings(const void* a, const void* b, void* context) {
    return -SStrCmpI(*static_cast<char* const*>(a), *static_cast<char* const*>(b), STORM_MAX_STR);
}

// VERIFY: b136b0d2 manual
// OFFSET: 0x00405A10
int32_t PatchFiles::EnumPatchArchives(OS_FILE_DATA& data, void* param) {
    PatchFiles* files = static_cast<PatchFiles*>(param);

#if defined(__APPLE__)
    // Native base and Data paths each fit the original SFile fields, but their
    // combined archive filename can exceed the old temporary 260-byte buffer.
    const std::string path = std::string(files->m_directory) + data.fileName;
    char** slot = files->m_files.New();
    *slot = nullptr;
    *slot = SStrDupA(path.c_str(), __FILE__, __LINE__);
#else
    char path[260];
    SStrPrintf(path, sizeof(path), "%s%s", files->m_directory, data.fileName);
    char** slot = files->m_files.New();
    *slot = SStrDupA(path, __FILE__, __LINE__);
#endif

    return 0;
}

// VERIFY: 4fd60074 manual
// OFFSET: 0x00405AB0
PatchFiles::PatchFiles(const char* locale, bool flag) {
    const auto& specs = ClientPatchArchiveSpecs;

    char directory[256];
    char pattern[256];
    this->m_directory = directory;
#if defined(__APPLE__)
    // A native application's working directory is unrelated to its selected
    // installation. Use the same scoped paths as loose-file/MPQ access.
    auto normalize = [](const char* value) {
        std::string path(value); std::replace(path.begin(), path.end(), '\\', '/');
        return std::filesystem::path(path);
    };
    if (!locale || std::char_traits<char>::length(locale) != 4 ||
        (std::string(locale) != "----" && !std::all_of(locale, locale + 4, [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); })))
        throw std::invalid_argument("Patch archive locale must be a four-letter locale or ----");
    const auto base = normalize(SFile::m_basePath);
    const auto data = normalize(SFile::GetDataPath());
    auto root = (data.is_absolute() ? data : base / data).lexically_normal();
    while (root.has_relative_path() && root.filename().empty()) root = root.parent_path();
    if (data.empty() || !root.is_absolute()) throw std::invalid_argument("Patch archives require an absolute configured Data root");
    std::string nativeDirectory;
    try {
#else
    char installed[256];
    const char* installPath = GetInstallLocationString();
#endif
    int32_t wanted = flag;

    for (int32_t pass = 0; pass <= 1; pass++) {
        for (uint32_t i = 0; i < specs.size(); i++) {
            const PATCHARCHIVESPEC& spec = specs[i];

            if (spec.m_pass != pass || spec.m_flag != wanted) {
                continue;
            }

            SStrPrintf(directory, sizeof(directory), spec.m_directory, locale);
            SStrPrintf(pattern, sizeof(pattern), spec.m_pattern, locale);

#if defined(__APPLE__)
            if (spec.m_useInstallPath) throw std::runtime_error("Trial retail-install patch lookup is not supported on Apple");
            const std::string relative(directory);
            if (relative.compare(0, 5, "Data\\") == 0) nativeDirectory = PatchDirectory((root / normalize(relative.c_str() + 5)).lexically_normal());
            else nativeDirectory = PatchDirectory((root.parent_path() / normalize(directory)).lexically_normal());
            if (nativeDirectory.empty()) continue;
            if (nativeDirectory.back() != '/') nativeDirectory += '/';
            this->m_directory = nativeDirectory.c_str();
#else
            if (spec.m_useInstallPath && *installPath) {
                SStrPrintf(installed, sizeof(installed), "%s%s", installPath, directory);
                SStrCopy(directory, installed, sizeof(directory));
            }
#endif

            OsFileList(this->m_directory, pattern, PatchFiles::EnumPatchArchives, this, 0);
        }

        if (pass == 0) {
            quicksort(this->m_files.Ptr(), this->m_files.Count(), sizeof(char*), CompareStrings, nullptr);
        }
    }
#if defined(__APPLE__)
    } catch (...) {
        for (uint32_t i = 0; i < m_files.Count(); ++i) SMemFree(m_files[i], __FILE__, __LINE__, 0);
        throw;
    }
    this->m_directory = nullptr; // Used only by the synchronous enumeration.
#endif
}

// VERIFY: ff9d1c1e manual
// OFFSET: 0x00405D70
PatchFiles::~PatchFiles() {
    for (uint32_t i = 0; i < this->m_files.Count(); i++) {
        if (this->m_files[i]) {
            SMemFree(this->m_files[i], __FILE__, __LINE__, 0);
        }
    }
}
