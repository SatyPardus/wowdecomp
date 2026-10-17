#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"

namespace {

// Logs the first WMO roots and groups as they are parsed, then counts.
class HelloWorldMapObjectScript : public MapObjectScript {
    public:
    HelloWorldMapObjectScript() : MapObjectScript("HelloWorldMapObjectScript") {}

    void OnMapObjLoaded(CMapObj& mapObj) override {
        m_roots++;

        if (m_roots <= 3) {
            sExtensionMgr->Log("HelloWorld: OnMapObjLoaded %s (%u group(s))", mapObj.m_wmoName, mapObj.m_mapObjGroupCount);
        }
    }

    void OnMapObjGroupLoaded(CMapObjGroup& group) override {
        m_groups++;

        if (m_groups <= 3) {
            sExtensionMgr->Log("HelloWorld: OnMapObjGroupLoaded %s of %s", group.m_groupName ? group.m_groupName : "", group.m_parent->m_wmoName);
        } else if (m_groups % 100 == 0) {
            sExtensionMgr->Log("HelloWorld: %u wmo root(s), %u group(s) loaded", m_roots, m_groups);
        }
    }

    private:
    uint32_t m_roots = 0;
    uint32_t m_groups = 0;
};

}

void AddHelloWorldMapObjectScripts() {
    new HelloWorldMapObjectScript();
}
