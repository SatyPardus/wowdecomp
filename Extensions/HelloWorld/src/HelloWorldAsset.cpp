#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

// The CVar that turns the demonstration redirect on. It is read on every file open, from any
// thread that opens files, so the pointer is cached once and only m_intValue is read.
CVar* s_redirectCursor;

// Registers the CVar (ClientScript::OnStartup is where CVars belong) and, when it is set,
// redirects the pointer cursor texture to the attack cursor: with
//   SET helloWorldRedirectCursor "1"
// in Config.wtf the login screen shows the attack cursor, which proves the redirect took effect
// at the SFile level without any other change. Every open is counted and the first few logged.
class HelloWorldAssetStartup : public ClientScript {
    public:
    HelloWorldAssetStartup() : ClientScript("HelloWorldAssetStartup") {}

    void OnStartup() override {
        s_redirectCursor = sExtensionMgr->RegisterCVar("helloWorldRedirectCursor", "HelloWorld: open Interface\\Cursor\\Attack.blp in place of Point.blp", 0, "0", nullptr, CATEGORY_DEBUG);
    }
};

class HelloWorldAssetScript : public AssetScript {
    public:
    HelloWorldAssetScript() : AssetScript("HelloWorldAssetScript") {}

    bool OnFileOpen(const char* name, char* redirect, uint32_t redirectSize) override {
        m_opens++;

        if (m_opens <= 5) {
            sExtensionMgr->Log("HelloWorld: OnFileOpen %s", name);
        }

        if (s_redirectCursor && s_redirectCursor->m_intValue && !SStrCmpI(name, "Interface\\Cursor\\Point.blp", STORM_MAX_STR)) {
            SStrCopy(redirect, "Interface\\Cursor\\Attack.blp", redirectSize);
            sExtensionMgr->Log("HelloWorld: OnFileOpen redirected %s -> %s", name, redirect);
            return true;
        }

        return false;
    }

    void OnFileOpened(const char* name, SFileImpl& file) override {
        m_opened++;

        if (m_opened <= 3) {
            sExtensionMgr->Log("HelloWorld: OnFileOpened %s (source type %u)", name, file.m_type);
        }
    }

    private:
    uint32_t m_opens = 0;
    uint32_t m_opened = 0;
};

}

void AddHelloWorldAssetScripts() {
    new HelloWorldAssetStartup();
    new HelloWorldAssetScript();
}
