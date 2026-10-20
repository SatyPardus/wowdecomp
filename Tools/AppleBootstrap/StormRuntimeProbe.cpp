#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SOption.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void CheckChatFormatting() {
    const auto ordered = g_opt.orderedprintfenabled;
    struct Restore { decltype(ordered) saved; ~Restore() { g_opt.orderedprintfenabled = saved; } } restore{ordered};
    char text[256];
    // Two full-width GUIDs followed by a pointer reproduce the live chat crash.
    for (int mode : {0, 1}) {
        g_opt.orderedprintfenabled = mode;
        const auto size = SStrPrintf(text, sizeof(text), "%s:%d:%016I64X:%016I64X:%s", "Whisper", 7,
            uint64_t(0xF130123456789ABC), uint64_t(0x1F8), "party ready");
        Require(std::strcmp(text, "Whisper:7:F130123456789ABC:00000000000001F8:party ready") == 0 && size == std::strlen(text),
            "Chat GUID formatting changed the following string argument");
        SStrPrintf(text, sizeof(text), "I64 literal %%I64X %I64d %I64u", int64_t(-4294967297LL), uint64_t(18446744073709551615ULL));
        Require(!std::strcmp(text, "I64 literal %I64X -4294967297 18446744073709551615"), "64-bit formatting changed literals or integer range");
        Require(SStrPrintf(text, 5, "%016I64X:%s", uint64_t(0x123456789ABCDEF0), "tail") == 4 && !std::strcmp(text, "1234"),
            "64-bit bounded formatting changed truncation");
        SStrPrintf(text, STORM_MAX_STR, "%016I64X:%s", uint64_t(0x123456789ABCDEF0), "tail");
        Require(!std::strcmp(text, "123456789ABCDEF0:tail"), "Unbounded formatting lost the native length modifier");
    }
    g_opt.orderedprintfenabled = 0;
    SStrPrintf(text, sizeof(text), "%*.*I64x:%s", 10, 6, uint64_t(0x1234), "tail");
    Require(!std::strcmp(text, "    001234:tail"), "Native formatting lost dynamic width/precision arguments");
}
}

void CheckStormRuntime(const std::string& scratchDirectory) {
    CheckChatFormatting();
    std::string pattern = scratchDirectory + "/wow-storm-XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    Require(mkdtemp(temporary.data()), "Cannot create Storm runtime check directory");
    struct Scope {
        const char* path;
        int32_t silent = g_opt.serrleaksilentwarning;
        ~Scope() { StormDestroy(); g_opt.serrleaksilentwarning = silent; std::error_code error; std::filesystem::remove_all(path, error); }
    } scope{temporary.data()};
    const auto logPath = std::string(temporary.data()) + "/runtime.log";
    for (unsigned cycle = 0; cycle != 3; ++cycle) {
        StormInitialize();
        Require(SStrToFloat("3.125") == 3.125f, "Storm initialization lost original float conversion");
        HSRGN region = nullptr; SRgnCreate(&region, 0);
        Require(region, "Storm runtime could not create a region after restart");
        RECTF rect{1, 2, 7, 9};
        SRgnCombineRectf(region, &rect, nullptr, SRGN_OR);
        RECTF bounds{}; SRgnGetBoundingRectf(region, &bounds);
        Require(!std::memcmp(&rect, &bounds, sizeof(rect)), "Original region geometry changed across native initialization");
        const _ARGLIST arguments[] = {{SCMD_TYPE_STRING, 7, "runtime-text", nullptr}};
        Require(SCmdRegisterArgList(arguments, 1), "Cannot register Storm runtime option");
        const char* argv[] = {"WoW", "-runtime-text", "native text"};
        Require(SCmdProcessArguments(3, argv, nullptr, nullptr), "Cannot parse Storm runtime option");
        char value[32]{}; Require(SCmdGetString(7, value, sizeof(value)) && !std::strcmp(value, "native text"), "Storm option value changed");
        HSLOG log = nullptr;
        Require(SLogCreate(logPath.c_str(), SLOG_FLAG_APPEND, &log), "Cannot create runtime-owned log");
        SLogWrite(log, "runtime cycle %u", cycle);
        // Deliberately leave the region, option string and log owned by Storm.
        StormDestroy();
        Require(!SCmdGetString(7, value, sizeof(value)), "Storm shutdown retained an option allocation");
        char path[8]{}; Require(!SErrGetLogLastPath(path, sizeof(path)), "Storm shutdown retained error callbacks/path state");
        std::ifstream input(logPath); const std::string contents((std::istreambuf_iterator<char>(input)), {});
        Require(contents.find("runtime cycle " + std::to_string(cycle)) != std::string::npos, "Storm shutdown did not flush its outstanding log");
    }
    std::fputs("Original Storm runtime: three startup/teardown lifetimes released regions, command strings and live logs; repeated shutdown passed\n", stderr);
}
