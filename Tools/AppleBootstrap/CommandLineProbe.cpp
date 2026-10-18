#include "Engine/Source/Base/CmdLine.h"
#include "Engine/Source/Storm/H/storm.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>
#include <unistd.h>

namespace {
void Require(bool valid, const char* message) { if (!valid) throw std::runtime_error(message); }
uint32_t lastError;
std::string lastItem, lastMessage;
std::vector<std::string> extras;
void STORMAPI Error(_CMDERROR* error) {
    lastError = error->errorcode; lastItem = error->itemstr; lastMessage = error->errorstr;
}
int32_t STORMAPI Extra(const char* value) { extras.emplace_back(value); return 1; }
void Write(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary); file.write(contents.data(), contents.size());
    Require(bool(file), "Cannot write command-line fixture");
}
int Parse(const std::vector<std::string>& values) {
    std::vector<const char*> argv; for (const auto& value : values) argv.push_back(value.c_str());
    lastError = 0; lastItem.clear(); lastMessage.clear();
    return SCmdProcessArguments(int(argv.size()), argv.data(), Extra, Error);
}
std::string String(uint32_t id) {
    char buffer[4096]; Require(SCmdGetString(id, buffer, sizeof(buffer)), "Missing command string"); return buffer;
}
}

void CheckCommandLine(const std::string& scratch) {
    std::string pattern = (std::filesystem::path(scratch) / "wow-cmd-XXXXXX").string();
    std::vector<char> directory(pattern.begin(), pattern.end()); directory.push_back(0);
    Require(mkdtemp(directory.data()), "Cannot create command-line scratch directory");
    struct Scope {
        std::filesystem::path path;
        ~Scope() { SCmdDestroy(); std::error_code error; std::filesystem::remove_all(path, error); }
    } scope{directory.data()};
    for (int cycle = 0; cycle < 2; ++cycle) {
        const char* clientArgs[] = {"WoW", "-NoWarnings", "-fullscreen+", "-fullscreen-", "-datadir", "/Volumes/Game Data;one,two/\"literal\"", "-loadfile", "@literal.lua"};
        Require(CmdLineProcess(sizeof(clientArgs) / sizeof(clientArgs[0]), clientArgs), "Original client options failed with native argv");
        Require(CmdLineGetBool(CMD_NO_WARNINGS) && !CmdLineGetBool(CMD_FULL_SCREEN) && !CmdLineGetBool(CMD_D3D), "Original client boolean option semantics changed");
        Require(std::string(CmdLineGetString(CMD_DATA_DIR)) == clientArgs[5] && std::string(CmdLineGetString(CMD_LOADFILE)) == "@literal.lua", "Native argv lost a path, separator, quote or literal @ value");
        SCmdDestroy();

        const _ARGLIST options[] = {{SCMD_TYPE_BOOL, 1, "toggle", nullptr}, {SCMD_TYPE_NUMBER, 2, "unsigned", nullptr},
            {SCMD_TYPE_NUMBER | SCMD_NUM_SIGNED, 3, "signed", nullptr}, {SCMD_TYPE_STRING, 4, "text", nullptr},
            {SCMD_TYPE_STRING, 4, "alias", nullptr}, {SCMD_TYPE_BOOL | SCMD_BOOL_CLEAR, 5, "clear", nullptr}};
        Require(SCmdRegisterArgList(options, 6), "Cannot register original Storm option table");
        Require(SCmdGetBool(5), "Original clear-flag default changed");
        const char* legacy = "\"C:\\Program Files\\WoW.exe\" /toggle /unsigned:0xF00D /signed=-2147483648 /text \"path with spaces\" /clear";
        Require(SCmdProcess(legacy, 1, Extra, Error) && SCmdGetBool(1) && SCmdGetNum(2) == 0xF00D && SCmdGetNum(3) == 0x80000000 && !SCmdGetBool(5) && String(4) == "path with spaces", "Original command-string grammar changed");
        Require(Parse({"WoW", "-alias", "", "-signed", "-2", "-unsigned", "-2"}) && String(4).empty() && SCmdGetNum(2) == 0xFFFFFFFE && SCmdGetNum(3) == 0xFFFFFFFE, "Native empty string/negative numeric argument failed");
        Require(Parse({"WoW", "-unsigned", "4294967296", "-signed", "2147483648"}) && SCmdGetNum(2) == 0xFFFFFFFF && SCmdGetNum(3) == 0x7FFFFFFF, "Apple long width changed original 32-bit option saturation");
        Require(Parse({"WoW", "-signed", "-2147483649", "-unsigned", "-4294967296"}) && SCmdGetNum(3) == 0x80000000 && SCmdGetNum(2) == 0xFFFFFFFF, "Negative out-of-range option saturation changed");
        Require(Parse({"WoW", "-unsigned", "-4294967295"}) && SCmdGetNum(2) == 1, "Unsigned 32-bit negation changed");
        Require(Parse({"WoW", "-unsigned", "-18446744073709551616"}) && SCmdGetNum(2) == 0xFFFFFFFF, "Native long overflow was accepted as a small negative number");
        Require(!Parse({"WoW", "-text", std::string(1024 * 1024, 'x')}), "Native argument budget was ignored");

        const auto leaf = scope.path / "leaf", root = scope.path / "root";
        Write(leaf, "-unsigned 0x42 -text \"from response\"\n");
        Write(root, "@" + leaf.string() + "\r\n-toggle- -clear+\n");
        Require(Parse({"WoW", "@" + root.string()}) && SCmdGetNum(2) == 0x42 && !SCmdGetBool(1) && SCmdGetBool(5) && String(4) == "from response", "Nested original response-file processing changed");
        std::filesystem::create_symlink(leaf, scope.path / "link");
        Require(Parse({"WoW", "@" + (scope.path / "link").string()}), "Normal response-file symlink was rejected");
        Require(!Parse({"WoW", "@" + (scope.path / "absent").string()}) && lastError == 110 && !lastMessage.empty(), "Missing response file did not report original error");
        Write(root, "@" + root.string());
        Require(!Parse({"WoW", "@" + root.string()}) && lastError == 110, "Recursive response file was accepted");
        Write(root, "@" + (scope.path / "hard").string()); std::filesystem::create_hard_link(root, scope.path / "hard");
        Require(!Parse({"WoW", "@" + root.string()}), "Hard-linked response cycle was accepted");
        Write(root, std::string(1024 * 1024 + 1, ' '));
        Require(!Parse({"WoW", "@" + root.string()}), "Over-budget response file was accepted");
        for (int i = 0; i < 17; ++i) Write(scope.path / ("depth" + std::to_string(i)), i == 16 ? "-toggle" : "@" + (scope.path / ("depth" + std::to_string(i + 1))).string());
        Require(!Parse({"WoW", "@" + (scope.path / "depth0").string()}), "Over-deep response chain was accepted");
        Write(leaf, std::string(1024 * 1024, ' '));
        std::string repeated; for (int i = 0; i < 9; ++i) repeated += "@" + leaf.string() + "\n";
        Write(root, repeated);
        Require(!Parse({"WoW", "@" + root.string()}), "Cumulative response budget was ignored");
        Write(root, "-toggle+ -text \"retry succeeded\"");
        Require(Parse({"WoW", "@" + root.string()}) && SCmdGetBool(1) && String(4) == "retry succeeded", "Response failure poisoned later startup attempts");
        Require(!Parse({"WoW", "-unknown"}) && lastError == STORM_ERROR_BAD_ARGUMENT && lastItem == "unknown", "Unknown option did not retain original diagnostic");
        SCmdDestroy();
        const _ARGLIST required[] = {{SCMD_TYPE_STRING | SCMD_ARG_REQUIRED, 7, "input", nullptr}};
        SCmdRegisterArgList(required, 1);
        Require(!Parse({"WoW"}) && lastError == STORM_ERROR_NOT_ENOUGH_ARGUMENTS && Parse({"WoW", "input value"}) && String(7) == "input value", "Required original positional argument failed");
        SCmdDestroy();
        // CoreProbe uses these two options; the iOS host has no CLI options.
        const _ARGLIST probeArgs[] = {{SCMD_TYPE_STRING, 8, "-data-root", nullptr}, {SCMD_TYPE_STRING, 9, "-locale", nullptr}};
        SCmdRegisterArgList(probeArgs, 2);
        Require(SCmdProcessCommandLine(Extra, Error), "Native process argv collection failed");
        SCmdDestroy();
        std::filesystem::remove(scope.path / "link"); std::filesystem::remove(scope.path / "hard");
    }
    std::fprintf(stderr, "Original command options: native argv/absolute and quoted paths, 32-bit numbers, aliases, nested response files, bounded cycle/size/depth failures and retry passed twice\n");
}
