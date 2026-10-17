#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

class HelloWorldClientScript : public ClientScript {
    public:
    HelloWorldClientScript() : ClientScript("HelloWorldClientScript") {}

    void OnStartup() override {
        sExtensionMgr->Log("HelloWorld: OnStartup");
    }

    void OnClientInitialized() override {
        sExtensionMgr->Log("HelloWorld: OnClientInitialized");
    }

    void OnUpdate(float elapsedSec) override {
        m_updates++;

        if (m_updates == 1) {
            sExtensionMgr->Log("HelloWorld: OnUpdate (first, %.4f s)", elapsedSec);
        }
    }

    void OnClientDestroy() override {
        sExtensionMgr->Log("HelloWorld: OnClientDestroy after %u update(s)", m_updates);
    }

    void OnEnterWorld(uint32_t mapId) override {
        sExtensionMgr->Log("HelloWorld: OnEnterWorld (map %u)", mapId);
    }

    void OnLeaveWorld() override {
        sExtensionMgr->Log("HelloWorld: OnLeaveWorld");
    }

    void OnShutdown() override {
        sExtensionMgr->Log("HelloWorld: OnShutdown");
    }

    private:
    uint32_t m_updates = 0;
};

}

void AddHelloWorldClientScripts() {
    new HelloWorldClientScript();
}
