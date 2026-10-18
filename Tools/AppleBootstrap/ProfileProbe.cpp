#include "Engine/Source/Services/Profile.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
using Profile = std::unique_ptr<void, decltype(&ProfileClose)>;
std::string Value(void* profile, const char* key, uint32_t index = 0) {
    char value[256];
    Require(ProfileGetValue(profile, "wow config", key, value, sizeof(value), index), "Original profile value lookup failed");
    return value;
}
}

void CheckProfiles(const std::string& scratchDirectory) {
    std::string pattern = scratchDirectory + "/wow-profile-XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    Require(mkdtemp(temporary.data()), "Cannot create profile scratch directory");
    struct Cleanup {
        const char* path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{temporary.data()};
    const std::string path = std::string(temporary.data()) + "/配置.ini";
    const std::string text = "// startup locale\r\n[WoW Config]\r\nRegion=3\r\nLanguage=en\r\nCountry=US\r\n"
        "Quoted=\"alpha,beta\",gamma,\"测试\"\nNumbers=-42,'enUS'\nRepeated=first\nRepeated=second\n"
        "Empty=\"\"\nSpaced = keep \nNoFinal=tail";
    { std::ofstream file(path, std::ios::binary); file << text; }
    for (unsigned cycle = 0; cycle != 2; ++cycle) {
        Profile profile(ProfileCreate(), ProfileClose);
        Require(bool(profile), "Cannot create original profile");
        Require(ProfileReadFile(profile.get(), (path + " \t").c_str()), "Original profile did not load a native UTF-8 path");
        int32_t region = 0;
        Require(ProfileGetValue(profile.get(), "WOW CONFIG", "region", &region, 0) && region == 3 &&
            Value(profile.get(), "LANGUAGE") == "en" && Value(profile.get(), "Country") == "US",
            "Original locale profile values or case-insensitive lookup changed");
        Require(Value(profile.get(), "Quoted", 0) == "alpha,beta" && Value(profile.get(), "Quoted", 1) == "gamma" &&
            Value(profile.get(), "Quoted", 2) == "测试" && Value(profile.get(), "Empty").empty(),
            "Original profile quote/comma/UTF-8/empty-value rules changed");
        Require(Value(profile.get(), "Repeated", 0) == "first" && Value(profile.get(), "Repeated", 1) == "second" &&
            Value(profile.get(), "NoFinal") == "tail" && Value(profile.get(), "Spaced ") == " keep ",
            "Original profile repeated key, EOF or untrimmed value rules changed");
        int32_t number = 0;
        Require(ProfileGetValue(profile.get(), "WoW Config", "Numbers", &number, 0) && number == -42 &&
            ProfileGetValue(profile.get(), "WoW Config", "Numbers", &number, 1) && uint32_t(number) == 0x656E5553,
            "Original profile decimal/FourCC rules changed");
        Require(uint32_t(ProfileInternal::PrfStrToInt("'\xff" "ABC'")) == 0xFF414243u,
            "Native profile FourCC conversion lost original signed-byte/low-32-bit behavior");
        char boundary[] = {'x', 'g'};
        Require(ProfileGetValue(profile.get(), "WoW Config", "Country", boundary, 1, 0) && boundary[0] == 0 && boundary[1] == 'g',
            "Original profile short string boundary changed");
        boundary[0] = 'x';
        Require(!ProfileGetValue(profile.get(), "WoW Config", "Country", boundary, 0, 0) && boundary[0] == 'x',
            "Native profile wrote into a zero-size string buffer");
        Require(!ProfileReadFile(profile.get(), "") && !ProfileReadFile(profile.get(), " \t\r\n") &&
            !ProfileReadFile(profile.get(), (path + ".missing").c_str()) &&
            !ProfileReadFile(profile.get(), std::string(260, 'x').c_str()), "Native profile accepted an unavailable path");
        const char appended[] = "[WoW Config]\nRepeated=third\n";
        Require(ProfileInternal::IReadBuffer(static_cast<ProfileInternal::PROFILE*>(profile.get()), appended, sizeof(appended) - 1) &&
            Value(profile.get(), "Repeated", 2) == "third", "Original profile append behavior changed after failed read");
        Require(!ProfileInternal::IReadBuffer(static_cast<ProfileInternal::PROFILE*>(profile.get()), text.data(), 4 * 1024 * 1024 + 1) &&
            errno == EFBIG && Value(profile.get(), "Country") == "US", "Native profile budget failure changed existing values");
    }
    const std::string large = path + ".large";
    { std::ofstream file(large); }
    std::filesystem::resize_file(large, 4 * 1024 * 1024 + 1);
    Profile profile(ProfileCreate(), ProfileClose);
    Require(!ProfileReadFile(profile.get(), large.c_str()) && errno == EFBIG, "Native profile allocated an oversized file");
    const std::string empty = path + ".empty";
    { std::ofstream file(empty); }
    Require(ProfileReadFile(profile.get(), empty.c_str()), "Original empty profile should be valid");
    std::fputs("Original startup profile: locale/case lookup, quoted multi-values/UTF-8/FourCC, preserved whitespace/EOF, append/retry, buffer/file budget boundaries and two lifetimes passed\n", stderr);
}
