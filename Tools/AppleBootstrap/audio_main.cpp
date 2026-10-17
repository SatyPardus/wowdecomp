#include "Tools/AppleBootstrap/AudioProbe.h"
#include "WoW/Source/WowServices/ClientData.h"
#include <cstring>
#include <iostream>
int main(int argc, char** argv) {
    if ((argc != 3 && argc != 4) || strcmp(argv[1], "--data-root") || (argc == 4 && strcmp(argv[3], "--output"))) {
        std::cerr << "Usage: AppleAudioProbe --data-root /path/to/Data [--output]\n";
        return 2;
    }
    try {
        ClientData client(argv[2], "");
        CheckAudioResources(client, std::cout);
        if (argc == 4) CheckAudioOutput(client, std::cout);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
