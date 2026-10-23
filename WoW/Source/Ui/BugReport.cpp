#include "WoW/Source/Ui/BugReport.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQGlobals.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/AddOns.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "GameVersion.h"
#include "WaystoneVersion.h"

static const char* s_bugReportFile = "Logs\\BugReport.txt";

struct StockArchive {
    const char* m_name;
    uint64_t m_enUSSize;
};

struct StockFile {
    const char* m_name;
    const char* m_enUSMd5;
};

static const StockArchive s_stockBaseArchives[] = {
    { "common.mpq", 2881154862ull },
    { "common-2.mpq", 1810430636ull },
    { "expansion.mpq", 1921219911ull },
    { "lichking.mpq", 2553948549ull },
    { "patch.mpq", 4004713057ull },
    { "patch-2.mpq", 1401729059ull },
    { "patch-3.mpq", 605089137ull },
};

static const StockArchive s_stockLocaleArchives[] = {
    { "backup-%s.mpq", 167245856ull },
    { "base-%s.mpq", 29176975ull },
    { "expansion-locale-%s.mpq", 17389181ull },
    { "expansion-speech-%s.mpq", 241033297ull },
    { "lichking-locale-%s.mpq", 12354378ull },
    { "lichking-speech-%s.mpq", 354008839ull },
    { "locale-%s.mpq", 204291376ull },
    { "patch-%s.mpq", 296616080ull },
    { "patch-%s-2.mpq", 225570171ull },
    { "patch-%s-3.mpq", 100373935ull },
    { "speech-%s.mpq", 438430439ull },
};

static const char* s_assetExtensions[] = {
    ".anim", ".adt", ".blp", ".bls", ".dbc", ".lit", ".lua", ".m2", ".mdx", ".mp3", ".ogg", ".sig",
    ".skin", ".tga", ".toc", ".wav", ".wdl", ".wdt", ".wfx", ".wmo", ".xml", ".zmp",
};

static const char* s_skippedRootDirs[] = {
    "cache",
    "data",
    "errors",
    "logs",
    "screenshots",
    "wtf",
};

static const StockFile s_checkedFiles[] = {
    { "Interface\\FrameXML\\FrameXML.toc", "8255c5467a6be97038508b6b6a611fc4" },
    { "DBFilesClient\\CreatureDisplayInfo.dbc", "37553f408746a2cd10dd42f3d1cfb2db" },
    { "DBFilesClient\\CreatureDisplayInfoExtra.dbc", "74e270393ccbf0c897c067eb086ed78d" },
    { "DBFilesClient\\CreatureModelData.dbc", "c9e15948fe442b7b9d502f1243006ad1" },
    { "DBFilesClient\\ItemDisplayInfo.dbc", "ea544682682d4ec83a918577f505a41d" },
    { "DBFilesClient\\Spell.dbc", "543b9fe61355b6a77a01714d52fea2e5" },
    { "DBFilesClient\\SpellChainEffects.dbc", "3c9b6da5bf6ace07e68b7ae1efcd4a22" },
    { "DBFilesClient\\SpellVisual.dbc", "b0786536a38b51497a520542e3f0b36e" },
    { "DBFilesClient\\SpellVisualKit.dbc", "95d9618601da2aca6457053ba9e430dd" },
    { "DBFilesClient\\SpellVisualEffectName.dbc", "8b1c2c043645808fc17c49f312192897" },
    { "DBFilesClient\\Map.dbc", "2b824e66afaa43c017832ca0e87ed999" },
};

static std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return text;
}

static std::string FormatFileTime(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::file_time_type fileTime = std::filesystem::last_write_time(path, error);

    if (error) {
        return "?";
    }

    std::chrono::system_clock::time_point systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    time_t seconds = std::chrono::system_clock::to_time_t(systemTime);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&seconds));
    return buffer;
}

static std::string Md5Hex(const void* data, uint32_t size) {
    MD5_CTX context;
    MD5Init(&context);
    MD5Update(&context, static_cast<const uint8_t*>(data), size);
    uint8_t digest[16];
    MD5Final(digest, &context);

    char buffer[33];

    for (uint32_t i = 0; i < 16; i++) {
        snprintf(buffer + i * 2, 3, "%02x", digest[i]);
    }

    return buffer;
}

static const char* CVarString(const char* name) {
    CVar* var = CVar::Lookup(name);
    return var ? var->m_stringValue.m_str : "(unset)";
}

static const char* CheckArchive(const std::string& relative, const char* locale, uint64_t size) {
    std::string name = ToLower(relative);
    std::replace(name.begin(), name.end(), '/', '\\');

    for (const StockArchive& stock : s_stockBaseArchives) {
        if (name == stock.m_name) {
            return size == stock.m_enUSSize ? "stock" : "MODIFIED";
        }
    }

    std::string prefix = ToLower(locale) + "\\";

    if (name.compare(0, prefix.size(), prefix)) {
        return "EXTRA";
    }

    bool enUS = ToLower(locale) == "enus";

    for (const StockArchive& stock : s_stockLocaleArchives) {
        char expected[64];
        snprintf(expected, sizeof(expected), stock.m_name, locale);

        if (name.substr(prefix.size()) == ToLower(expected)) {
            if (!enUS) {
                return "stock?";
            }

            return size == stock.m_enUSSize ? "stock" : "MODIFIED";
        }
    }

    return "EXTRA";
}

static bool IsAssetFile(const std::filesystem::path& path) {
    std::string extension = ToLower(path.extension().string());

    for (const char* asset : s_assetExtensions) {
        if (extension == asset) {
            return true;
        }
    }

    return false;
}

static std::vector<System_SFile::ARCHIVEREC*> GetArchives() {
    std::vector<System_SFile::ARCHIVEREC*> archives;
    Blizzard::Lock::ScopedMutex lock(System_MPQGlobals::GetArchiveLock()->m_mutex);

    for (System_SFile::ARCHIVEREC* archive : *System_MPQGlobals::GetArchiveList()) {
        archives.push_back(archive);
    }

    return archives;
}

static void ReportBuild(FILE* file) {
    char os[64];
    OsGetVersionString(os, sizeof(os));

    fprintf(file, "[build]\n");
    fprintf(file, "  client          " CLIENT_NAME " " WAYSTONE_BUILD_STRING " (" WAYSTONE_COMMIT "), " GAME_VERSION " (" GAME_BUILD_STRING "), %s\n", sizeof(void*) == 8 ? "x64" : "x86");
    fprintf(file, "  compiled        %s %s\n", __DATE__, __TIME__);
#ifdef DISABLE_SIGNATURE_CHECKS
    fprintf(file, "  signature check OFF (built with WOWDECOMP_DISABLE_SIGNATURE_CHECKS: modified MPQ files and interface files load)\n");
#else
    fprintf(file, "  signature check ON (stock behaviour: modified signed files are rejected)\n");
#endif

    uint8_t digest[16];
    int32_t signature = FrameXML_CheckSignature("Interface\\FrameXML\\FrameXML.toc", "Interface\\FrameXML\\Bindings.xml", InterfaceKey, digest);
    const char* signatureText = signature == 3 ? "valid" : signature == 2 ? "MODIFIED or corrupt" : signature == 1 ? "corrupt signature" : signature == 0 ? "MISSING signature" : "unknown";
    fprintf(file, "  FrameXML sig    %s (%d)\n", signatureText, signature);
    fprintf(file, "  os              %s\n", os);
    fprintf(file, "  locale          %s\n", CVarString("locale"));
    fprintf(file, "  gxApi           %s\n", CVarString("gxApi"));
    fprintf(file, "  gxResolution    %s  gxWindow %s\n", CVarString("gxResolution"), CVarString("gxWindow"));
    fprintf(file, "  realmlist       %s\n", CVarString("realmList"));
    fprintf(file, "  realm           %s\n", ClientServices::GetSelectedRealmName() ? ClientServices::GetSelectedRealmName() : "");
    fprintf(file, "  character       %s\n\n", ClientServices::GetCharacterName() ? ClientServices::GetCharacterName() : "");
}

static void ReportArchives(FILE* file) {
    fprintf(file, "[opened archives, highest priority first]\n");

    for (System_SFile::ARCHIVEREC* archive : GetArchives()) {
        if (archive->m_parent || !archive->m_name[0]) {
            continue;
        }

        fprintf(file, "  %4d  %s\n", archive->m_priority, archive->m_name);
    }

    fprintf(file, "\n");
}

static void ReportDataFolder(FILE* file, const std::filesystem::path& root) {
    const char* locale = CVarString("locale");
    std::filesystem::path data = root / "Data";
    uint32_t extra = 0;
    uint32_t modified = 0;
    std::vector<std::string> loose;

    fprintf(file, "[Data folder: %s]\n", data.string().c_str());

    std::error_code error;

    for (std::filesystem::recursive_directory_iterator it(data, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) {
            continue;
        }

        std::string relative = std::filesystem::relative(it->path(), data, error).string();

        if (ToLower(it->path().extension().string()) == ".mpq") {
            uint64_t size = it->file_size(error);
            const char* state = CheckArchive(relative, locale, size);
            fprintf(file, "  %-8s %-40s %12llu  %s\n", state, relative.c_str(), static_cast<unsigned long long>(size), FormatFileTime(it->path()).c_str());

            if (!strcmp(state, "EXTRA")) {
                extra++;
            } else if (!strcmp(state, "MODIFIED")) {
                modified++;
            }
        } else if (IsAssetFile(it->path())) {
            loose.push_back(relative);
        }
    }

    for (const std::string& name : loose) {
        fprintf(file, "  LOOSE    %s\n", name.c_str());
    }

    fprintf(file, "  summary: %u modified archive(s) (size differs from stock enUS), %u non-stock archive(s), %u loose asset file(s) in Data\n\n", modified, extra, static_cast<uint32_t>(loose.size()));
}

static void ReportLooseOverrides(FILE* file, const std::filesystem::path& root) {
    fprintf(file, "[loose game files next to the exe (override MPQ contents)]\n");

    uint32_t count = 0;
    std::error_code error;

    for (std::filesystem::recursive_directory_iterator it(root, error), end; !error && it != end; it.increment(error)) {
        std::string relative = ToLower(std::filesystem::relative(it->path(), root, error).string());

        if (it->is_directory(error)) {
            bool skip = it.depth() == 0 && std::find_if(std::begin(s_skippedRootDirs), std::end(s_skippedRootDirs), [&](const char* dir) { return relative == dir; }) != std::end(s_skippedRootDirs);

            if (skip || relative == "interface\\addons") {
                it.disable_recursion_pending();
            }

            continue;
        }

        if (it->is_regular_file(error) && IsAssetFile(it->path())) {
            fprintf(file, "  LOOSE    %s  %s\n", std::filesystem::relative(it->path(), root, error).string().c_str(), FormatFileTime(it->path()).c_str());
            count++;
        }
    }

    fprintf(file, "  summary: %u loose file(s)\n\n", count);
}

static void ReportFile(FILE* file, const char* name, const char* stockMd5 = nullptr) {
    if (!name || !name[0]) {
        return;
    }

    std::string lookup = name;
    std::string lower = ToLower(lookup);

    if (lower.size() > 4 && (lower.compare(lower.size() - 4, 4, ".mdx") == 0 || lower.compare(lower.size() - 4, 4, ".mdl") == 0)) {
        lookup = lookup.substr(0, lookup.size() - 4) + ".m2";
    }

    fprintf(file, "  %s\n", lookup.c_str());

    char path[260];
    SFILE_TYPE type = SFILE_PLAIN;
    System_SFile::ARCHIVEREC* winner = nullptr;

    if (!System_SFile2::FindFile(lookup.c_str(), path, sizeof(path), 0, &type, &winner)) {
        fprintf(file, "    used: NOT FOUND\n");
        return;
    }

    std::string usedMd5 = "?";
    void* usedData = nullptr;
    uint32_t usedSize = 0;

    if (SFile::LoadFile(lookup.c_str(), &usedData, &usedSize, 0, nullptr) && usedData) {
        usedMd5 = Md5Hex(usedData, usedSize);
        SFile::Unload(usedData);
    }

    const char* state = "";

    if (stockMd5 && ToLower(CVarString("locale")) == "enus") {
        state = usedMd5 == stockMd5 ? "  stock" : "  MODIFIED";
    }

    if (type == SFILE_PLAIN) {
        fprintf(file, "    used: LOOSE FILE %s  md5 %s%s\n", path, usedMd5.c_str(), state);
    } else {
        fprintf(file, "    used: %s  md5 %s%s\n", winner ? winner->m_name : "(archive)", usedMd5.c_str(), state);
    }

    for (System_SFile::ARCHIVEREC* archive : GetArchives()) {
        if (archive->m_parent || !archive->m_name[0]) {
            continue;
        }

        void* data = nullptr;
        uint32_t size = 0;

        if (!Blizzard::Mopaq::SFileLoadFile(archive, lookup.c_str(), &data, &size, 0, 0) || !data) {
            continue;
        }

        fprintf(file, "    in %-48s size %10u  md5 %s\n", archive->m_name, size, Md5Hex(data, size).c_str());
        Blizzard::Mopaq::SFileUnloadFile(data);
    }
}

static const char* DisplayModelName(int32_t displayId) {
    CreatureDisplayInfoRec* display = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!display) {
        return nullptr;
    }

    CreatureModelDataRec* model = g_creatureModelDataDB.GetRecord(display->m_modelID);
    return model ? model->m_modelName : nullptr;
}

static void ReportUnit(FILE* file, const char* label, WOWGUID guid) {
    if (guid == WOWGUID()) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    const char* model = nullptr;
    unit->GetModelFileName(&model);

    fprintf(file, "[%s]\n", label);
    fprintf(file, "  guid %016llx  display %u  native %u  mount display %u\n", static_cast<unsigned long long>(guid.m_guid), unit->m_unit->m_unitFieldDisplayid, unit->m_unit->m_unitFieldNativedisplayid, unit->m_unit->m_unitFieldMountdisplayid);
    ReportFile(file, model);

    if (unit->m_unit->m_unitFieldMountdisplayid) {
        const char* mount = DisplayModelName(unit->m_unit->m_unitFieldMountdisplayid);

        if (mount) {
            ReportFile(file, mount);
        } else {
            fprintf(file, "  mount display %u has no CreatureDisplayInfo/CreatureModelData record\n", unit->m_unit->m_unitFieldMountdisplayid);
        }
    }

    fprintf(file, "\n");
}

static void ReportAddOns(FILE* file) {
    uint32_t count = GetNumAddOns();
    fprintf(file, "[AddOns: %u]\n", count);

    for (uint32_t i = 0; i < count; i++) {
        const char* name = GetAddOnByIndex(i);

        if (!name) {
            continue;
        }

        fprintf(file, "  %-8s %s\n", IsAddOnLoaded(name) ? "loaded" : "-", name);
    }

    fprintf(file, "\n");
}

static bool WriteBugReport(const char* arg) {
    FILE* file = fopen(s_bugReportFile, "w");

    if (!file) {
        return false;
    }

    char exePath[260];
    OsGetExePath(exePath, sizeof(exePath));
    std::filesystem::path root(exePath);

    time_t now = time(nullptr);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(file, "wowdecomp bug report %s\n", stamp);
    fprintf(file, "note: %s\n\n", arg && arg[0] ? arg : "");

    ReportBuild(file);
    ReportArchives(file);
    ReportDataFolder(file, root);
    ReportLooseOverrides(file, root);

    fprintf(file, "[checked files: which source the client uses, and every archive holding a copy]\n");

    for (const StockFile& checked : s_checkedFiles) {
        ReportFile(file, checked.m_name, checked.m_enUSMd5);
    }

    if (arg && arg[0] && (strchr(arg, '\\') || strchr(arg, '/')) && !strchr(arg, ' ')) {
        ReportFile(file, arg);
    }

    fprintf(file, "\n");

    ReportUnit(file, "player", ClntObjMgrGetActivePlayer());
    ReportUnit(file, "target", CGGameUI::m_lockedTarget);
    ReportAddOns(file);

    fclose(file);
    return true;
}

static int32_t Script_BugReport(lua_State* L) {
    const char* arg = lua_isstring(L, 1) ? lua_tolstring(L, 1, nullptr) : nullptr;

    if (!WriteBugReport(arg)) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, s_bugReportFile);
    return 1;
}

void BugReportRegister() {
    FrameScript_RegisterFunction("BugReport", Script_BugReport);
    FrameScript_Execute(
        "SLASH_BUGREPORT1 = \"/bugreport\"\n"
        "SlashCmdList[\"BUGREPORT\"] = function(msg)\n"
        "    local path = BugReport(msg)\n"
        "    if path then\n"
        "        DEFAULT_CHAT_FRAME:AddMessage(\"Bug report written to \" .. path .. \". Please attach it to your report.\", 1.0, 1.0, 0.0)\n"
        "    else\n"
        "        DEFAULT_CHAT_FRAME:AddMessage(\"Bug report could not be written.\", 1.0, 0.1, 0.1)\n"
        "    end\n"
        "end\n",
        "BugReport", nullptr);
}
