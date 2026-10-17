#include <cstdio>
#include <cstring>
#include <filesystem>
#include "Tools/AppleBootstrap/Probe.h"

int main(int argc, char** argv) {
    if (argc != 3 || strcmp(argv[1], "--data-root") != 0) {
        fprintf(stderr, "Usage: AppleCoreProbe --data-root /path/to/client335a/Data\n");
        return 2;
    }
    std::string report;
    try {
        bool success = AppleBootstrap::CheckCore(std::filesystem::temp_directory_path().string(), report);
        fputs(report.c_str(), stdout);
        if (!success) { return 1; }
        success = AppleBootstrap::InspectData(argv[2], report);
        fputs(report.c_str(), stdout);
        return success ? 0 : 1;
    } catch (const std::exception& error) {
        fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
