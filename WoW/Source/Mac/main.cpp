#if defined(__APPLE__)

#define SDL_MAIN_HANDLED

#include <cfenv>
#include <cstdint>
#include <SDL.h>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"

int main() {
    StormRtlInitialize();
    std::feclearexcept(FE_ALL_EXCEPT);

    SDL_SetMainReady();

    uint32_t flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;

    if (SDL_Init(flags) != 0) {
        return -1;
    }

    int32_t result = CommonMain();
    SDL_Quit();

    return result;
}

#endif
