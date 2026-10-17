#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

class HelloWorldWorldScript : public WorldScript {
    public:
    HelloWorldWorldScript() : WorldScript("HelloWorldWorldScript") {}

    void OnLoginVerifyWorld(uint32_t mapId, const C3Vector& position, float facing) override {
        sExtensionMgr->Log("HelloWorld: OnLoginVerifyWorld map %u at %.1f %.1f %.1f facing %.2f", mapId, position.x, position.y, position.z, facing);
    }

    void OnLogoutComplete() override {
        sExtensionMgr->Log("HelloWorld: OnLogoutComplete");
    }

    void OnMapLoaded(uint32_t mapId, const char* mapName) override {
        m_worldUpdates = 0;
        sExtensionMgr->Log("HelloWorld: OnMapLoaded (map %u, %s)", mapId, mapName);
    }

    void OnMapUnload() override {
        sExtensionMgr->Log("HelloWorld: OnMapUnload after %u world update(s)", m_worldUpdates);
    }

    void OnWorldUpdate(float elapsedSec) override {
        m_worldUpdates++;

        if (m_worldUpdates == 1) {
            sExtensionMgr->Log("HelloWorld: OnWorldUpdate (first, %.4f s)", elapsedSec);
        }
    }

    private:
    uint32_t m_worldUpdates = 0;
};

}

void AddHelloWorldWorldScripts() {
    new HelloWorldWorldScript();
}
