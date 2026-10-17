#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

// Logs the first few stores as they load with their record counts, then the total when the
// stock set is complete. An extension store would be a WowClientDB<MyRec> global loaded from
// OnClientDBInitialize (see ClientDBScript.h for the record contract).
class HelloWorldClientDBScript : public ClientDBScript {
    public:
    HelloWorldClientDBScript() : ClientDBScript("HelloWorldClientDBScript") {}

    void OnClientDBLoaded(WowClientDB_Base& db, const char* fileName) override {
        m_loaded++;

        if (m_loaded <= 3) {
            sExtensionMgr->Log("HelloWorld: OnClientDBLoaded %s (%d record(s), ids %d..%d)", fileName, db.m_numRecords, db.m_minID, db.m_maxID);
        }
    }

    void OnClientDBInitialize() override {
        sExtensionMgr->Log("HelloWorld: OnClientDBInitialize, %u store(s) loaded so far", m_loaded);
    }

    void OnClientDBShutdown() override {
        sExtensionMgr->Log("HelloWorld: OnClientDBShutdown, %u store(s) loaded in total", m_loaded);
    }

    private:
    uint32_t m_loaded = 0;
};

}

void AddHelloWorldClientDBScripts() {
    new HelloWorldClientDBScript();
}
