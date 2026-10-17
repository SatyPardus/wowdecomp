#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/WorldClient/MapArea.h"

namespace {

// Logs the first tiles as their ADT is parsed, with the placement counts the tile carries, and
// keeps the loaded/unloaded balance.
class HelloWorldTerrainScript : public TerrainScript {
    public:
    HelloWorldTerrainScript() : TerrainScript("HelloWorldTerrainScript") {}

    void OnTerrainTileLoaded(CMapArea& area) override {
        m_loaded++;

        if (m_loaded <= 5) {
            sExtensionMgr->Log("HelloWorld: OnTerrainTileLoaded tile %d,%d (%u doodad(s), %u wmo(s))", area.m_index.x, area.m_index.y, area.m_doodadDefCount, area.m_mapObjDefCount);
        }
    }

    void OnTerrainTileUnload(CMapArea& area) override {
        m_unloaded++;

        if (m_unloaded <= 3 || m_unloaded % 50 == 0) {
            sExtensionMgr->Log("HelloWorld: OnTerrainTileUnload tile %d,%d (%u loaded, %u unloaded so far)", area.m_index.x, area.m_index.y, m_loaded, m_unloaded);
        }
    }

    void OnTerrainChunkCreated(CMapChunk& chunk) override {
        m_chunks++;

        if (m_chunks == 1) {
            sExtensionMgr->Log("HelloWorld: OnTerrainChunkCreated (first)");
        }
    }

    void OnTerrainChunkPurge(CMapChunk& chunk) override {
        m_chunksPurged++;

        if (m_chunksPurged % 256 == 0) {
            sExtensionMgr->Log("HelloWorld: %u chunk(s) created, %u purged", m_chunks, m_chunksPurged);
        }
    }

    private:
    uint32_t m_loaded = 0;
    uint32_t m_unloaded = 0;
    uint32_t m_chunks = 0;
    uint32_t m_chunksPurged = 0;
};

}

void AddHelloWorldTerrainScripts() {
    new HelloWorldTerrainScript();
}
