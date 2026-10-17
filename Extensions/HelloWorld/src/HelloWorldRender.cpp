#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

// Logs the first call of each render hook and, every 1000 frames, the running counts, so the
// relative frequencies (frames painted vs world renders) can be read off the log.
class HelloWorldRenderScript : public RenderScript {
    public:
    HelloWorldRenderScript() : RenderScript("HelloWorldRenderScript") {}

    void OnFrameBegin(float elapsedSec) override {
        m_frames++;

        if (m_frames == 1) {
            sExtensionMgr->Log("HelloWorld: OnFrameBegin (first, %.4f s)", elapsedSec);
        }
    }

    void OnWorldRendered() override {
        m_worldRenders++;

        if (m_worldRenders == 1) {
            sExtensionMgr->Log("HelloWorld: OnWorldRendered (first)");
        }
    }

    void OnFrameEnd() override {
        m_frameEnds++;

        if (m_frameEnds == 1) {
            sExtensionMgr->Log("HelloWorld: OnFrameEnd (first)");
        } else if (m_frameEnds % 1000 == 0) {
            sExtensionMgr->Log("HelloWorld: %u frame(s) begun, %u ended, %u world render(s)", m_frames, m_frameEnds, m_worldRenders);
        }
    }

    private:
    uint32_t m_frames = 0;
    uint32_t m_frameEnds = 0;
    uint32_t m_worldRenders = 0;
};

}

void AddHelloWorldRenderScripts() {
    new HelloWorldRenderScript();
}
