#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"

namespace {

// Logs the first few objects the server creates (kind and low guid), counts the per-frame
// updates, and reports the create/remove/destroy balance when objects go away.
class HelloWorldObjectScript : public ObjectScript {
    public:
    HelloWorldObjectScript() : ObjectScript("HelloWorldObjectScript") {}

    void OnObjectCreated(CGObject_C& object) override {
        m_created++;

        if (m_created <= 5) {
            sExtensionMgr->Log("HelloWorld: OnObjectCreated type 0x%02X guid 0x%08X%08X", object.m_obj->m_type, static_cast<uint32_t>(object.m_obj->m_guid.m_guid >> 32), static_cast<uint32_t>(object.m_obj->m_guid.m_guid));
        }
    }

    void OnObjectUpdate(CGObject_C& object, uint32_t time) override {
        m_updates++;

        if (m_updates == 1) {
            sExtensionMgr->Log("HelloWorld: OnObjectUpdate (first, t=%u)", time);
        }
    }

    void OnObjectRemoved(CGObject_C& object) override {
        m_removed++;
    }

    void OnObjectDestroy(CGObject_C& object) override {
        m_destroyed++;

        if (m_destroyed % 100 == 0) {
            sExtensionMgr->Log("HelloWorld: objects %u created, %u removed, %u destroyed, %u update(s), %u spell visual kit(s)", m_created, m_removed, m_destroyed, m_updates, m_visualKits);
        }
    }

    void OnObjectSpellVisualKit(CGObject_C& object, const PlaySpellVisuaKitData& data) override {
        m_visualKits++;

        if (m_visualKits <= 3) {
            sExtensionMgr->Log("HelloWorld: OnObjectSpellVisualKit (%u so far)", m_visualKits);
        }
    }

    private:
    uint32_t m_created = 0;
    uint32_t m_updates = 0;
    uint32_t m_removed = 0;
    uint32_t m_destroyed = 0;
    uint32_t m_visualKits = 0;
};

}

void AddHelloWorldObjectScripts() {
    new HelloWorldObjectScript();
}
