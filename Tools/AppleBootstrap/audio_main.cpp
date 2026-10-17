#include "Tools/AppleBootstrap/AudioProbe.h"
#include "Tools/AppleBootstrap/GameAudioPreview.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "WoW/Source/WowServices/ClientData.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void CheckGameAudio() {
    static HPROPCONTEXT soundContext = nullptr;
    PropInitialize();
    struct Events {
        Events() {
            EventInitialize(1, 0);
            if (!EventCreateContextEx(1, [](const void*, void*) -> int32_t {
                    soundContext = PropGetSelectedContext(); return 1;
                }, nullptr, 1, 0))
                throw std::runtime_error("Cannot create sound Event context");
            EventProcessPending();
        }
        ~Events() { EventDestroy(); PropDestroy(); }
    } events;
    auto advance = [](unsigned count) {
        for (unsigned i = 0; i < count; ++i) {
            EventProcessPending();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        std::unique_ptr<GameAudioPreview> owner;
        {
            struct RestoreContext { HPROPCONTEXT previous = PropGetSelectedContext(); ~RestoreContext() { PropSelectContext(previous); } } restore;
            PropSelectContext(soundContext);
            owner = std::make_unique<GameAudioPreview>("Sound\\Music\\GlueScreenMusic\\Wotlk_main_title.mp3");
        }
        auto& sound = *owner;
        advance(200);
        auto playing = sound.Stats();
        if (!sound.IsPlaying() || !playing.nonzeroSamples) throw std::runtime_error("SESound stream did not become audible");
        sound.SetActive(false);
        auto before = sound.Stats();
        advance(25);
        auto suspended = sound.Stats();
        if (suspended.running || before.frames != suspended.frames) throw std::runtime_error("SESound output continued while suspended");
        sound.SetActive(true);
        advance(100);
        auto resumed = sound.Stats();
        if (!resumed.running || resumed.nonzeroSamples <= suspended.nonzeroSamples)
            throw std::runtime_error("SESound output did not resume");
        std::cout << "Original SESound cycle " << cycle + 1 << ": MPQ streaming, Event heartbeat, "
                  << resumed.sampleRate << " Hz, " << resumed.frames << " frames; suspend/resume verified.\n";
    }
}
}
int main(int argc, char** argv) {
    if ((argc != 3 && argc != 4) || strcmp(argv[1], "--data-root") ||
        (argc == 4 && strcmp(argv[3], "--output") && strcmp(argv[3], "--game"))) {
        std::cerr << "Usage: AppleAudioProbe --data-root /path/to/Data [--output|--game]\n";
        return 2;
    }
    try {
        ClientData client(argv[2], "");
        CheckAudioResources(client, std::cout);
        if (argc == 4 && !strcmp(argv[3], "--output")) CheckAudioOutput(client, std::cout);
        if (argc == 4 && !strcmp(argv[3], "--game")) CheckGameAudio();
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
