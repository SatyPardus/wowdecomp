#include "Engine/Source/ComSat/ComSatInterface.h"
#include "Engine/Source/ComSat/ComSatManager.h"
#include "Engine/Source/ComSat/ComSatNetwork.h"
#include "Engine/Source/ComSat/ComSatSoundManager.h"
#include <new>

// STATIC: 0x00D43988
static ComSat::Sound::Manager* s_soundManager;

// STATIC: 0x00D4398C
static ComSat::Network::Manager* s_networkManager;

// VERIFY: 452c7806 auto
// OFFSET: 0x00880B30
void ComSat::Interface::Destroy(ComSat::Interface* comsat, ComSat::Allocator& allocator) {
    if (comsat) {
        comsat->~Interface();
        allocator.Free(comsat);
    }

    if (s_soundManager) {
        ComSat::Sound::Manager* soundManager = s_soundManager;
        soundManager->~Manager();
        allocator.Free(soundManager);
        s_soundManager = nullptr;
    }

    if (s_networkManager) {
        ComSat::Network::Manager* networkManager = s_networkManager;
        networkManager->~Manager();
        allocator.Free(networkManager);
        s_networkManager = nullptr;
    }
}

// VERIFY: 5430b1ec manual
// OFFSET: 0x00880F30
ComSat::Interface* ComSat::Interface::Create(const ComSat::Initialization& init, ComSat::Allocator& allocator) {
    s_soundManager = new (allocator.Alloc(sizeof(ComSat::Sound::Manager))) ComSat::Sound::Manager(init, allocator);
    s_networkManager = new (allocator.Alloc(sizeof(ComSat::Network::Manager))) ComSat::Network::Manager(init, allocator);
    return new (allocator.Alloc(sizeof(ComSat::Manager))) ComSat::Manager(*s_soundManager, *s_networkManager, init, allocator);
}
