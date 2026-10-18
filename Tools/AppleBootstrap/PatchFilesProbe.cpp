#include "WoW/Source/WowServices/PatchFiles.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::string Normalize(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return std::filesystem::path(value).lexically_normal().string();
}
void Equal(PatchFiles& files, const std::vector<std::string>& expected) {
    Require(files.m_files.Count() == expected.size(), "Original patch enumeration changed its file count");
    for (size_t i = 0; i < expected.size(); ++i)
        Require(!SStrCmpI(Normalize(files.m_files[i]).c_str(), Normalize(expected[i]).c_str(), STORM_MAX_STR),
            "Original patch enumeration changed its path/order or truncated an archive name");
    Require(!files.m_directory, "Patch enumeration retained an expired directory pointer");
}
void Touch(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path); file << "enumeration only";
    Require(bool(file), "Cannot create patch enumeration fixture");
}
}

void CheckPatchFiles(const std::string& scratchDirectory) {
    std::string pattern = scratchDirectory + "/wow-patches-XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    Require(mkdtemp(temporary.data()), "Cannot create patch enumeration scratch directory");
    struct Scope {
        const char* path;
        std::string base = SFile::m_basePath, data = SFile::GetDataPath();
        ~Scope() { SFile::SetBasePath(base.c_str()); SFile::SetDataPath(data.c_str()); std::error_code error; std::filesystem::remove_all(path, error); }
    } scope{temporary.data()};
    auto base = std::filesystem::path(temporary.data());
    Require(base.string().size() < 237, "Patch scratch root exceeds the original SFile base-path budget");
    base /= std::string(238 - base.string().size() - 1, 'b');
    const auto root = base / "MyAssets";
    const auto parentData = base.parent_path() / "Data";
    const std::string locale = "enUS";
    std::vector<std::string> sorted;
    for (const char* suffix : {"2", "3", "4", "A"}) {
        const auto name = root / (std::string("PaTcH-") + suffix + ".MpQ"); Touch(name); sorted.push_back(name.string());
        const auto localized = root / "eNuS" / (std::string("patch-enUS-") + suffix + ".MPQ"); Touch(localized); sorted.push_back(localized.string());
    }
    Touch(root / "patch.MPQ"); Touch(root / "eNuS" / "patch-enUS.MPQ");
    Touch(root / "patch-10.MPQ"); Touch(root / "eNuS" / "patch-enUS-10.MPQ"); // '?' must match exactly one character.
    Touch(parentData / "patch-2.MPQ"); Touch(parentData / "enUS" / "patch-enUS-2.MPQ");
    Touch(parentData / "patch.MPQ"); Touch(parentData / "enUS" / "patch-enUS.MPQ");
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return SStrCmpI(a.c_str(), b.c_str(), STORM_MAX_STR) > 0; });
    sorted.push_back((root / "patch.MPQ").string()); sorted.push_back((root / "eNuS" / "patch-enUS.MPQ").string());
    Require(std::any_of(sorted.begin(), sorted.end(), [](const auto& name) { return name.size() >= 260; }), "Patch fixture did not exercise a combined long filename");
    std::vector<std::string> special;
    for (const char* suffix : {"4", "3"}) {
        special.push_back((root / (std::string("patch-") + suffix + ".MPQ")).string());
        special.push_back((root / "enUS" / (std::string("patch-enUS-") + suffix + ".MPQ")).string());
    }
    special.push_back((parentData / "patch-2.MPQ").string()); special.push_back((parentData / "enUS" / "patch-enUS-2.MPQ").string());
    special.push_back((parentData / "patch.MPQ").string()); special.push_back((parentData / "enUS" / "patch-enUS.MPQ").string());
    const auto cwd = std::filesystem::current_path();
    for (unsigned cycle = 0; cycle != 2; ++cycle) {
        SFile::SetBasePath(base.string().c_str());
        SFile::SetDataPath(cycle ? root.string().c_str() : "MyAssets");
        { PatchFiles files(locale.c_str(), false); Equal(files, sorted); }
        { PatchFiles files(locale.c_str(), true); Equal(files, special); }
        for (const char* invalid : {"", "../x", "en/US", "enUSmore"}) {
            bool rejected = false; try { PatchFiles files(invalid, false); } catch (const std::invalid_argument&) { rejected = true; }
            Require(rejected, "Native patch locale allowed malformed paths");
        }
        { PatchFiles missing("----", false); Require(missing.m_files.Count() == 5, "Missing locale changed base patch enumeration"); }
    }
    Require(std::filesystem::current_path() == cwd, "Native patch enumeration changed the process directory");
    SFile::SetBasePath(""); SFile::SetDataPath("relative");
    bool rejected = false; try { PatchFiles files("enUS", false); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "Native patch enumeration silently fell back to the process working directory");
    std::fputs("Original PatchFiles: two-pass priority, locale/case, single-character glob, special parent patches, relative/absolute configured Data, long combined paths and two lifetimes passed\n", stderr);
}

void CheckMountedPatchFiles(const ClientData& client) {
    PatchFiles files(client.Locale().c_str(), false);
    Require(files.m_files.Count() <= client.Mounts().size(), "Original patch enumeration exceeds mounted resources");
    for (uint32_t i = 0; i < files.m_files.Count(); ++i) {
        const auto& mount = client.Mounts()[i];
        Require(mount.prefix.empty() && mount.priority == int32_t(64 + i) &&
            !SStrCmpI(Normalize(mount.path).c_str(), Normalize(files.m_files[files.m_files.Count() - i - 1]).c_str(), STORM_MAX_STR),
            "Original patch reverse-open priority differs from the native mounted installation");
    }
    std::fprintf(stderr, "Original PatchFiles: %u real patch archives exactly match the mounted installation's reverse-open order\n", files.m_files.Count());
}
