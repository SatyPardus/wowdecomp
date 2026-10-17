#include "WoW/Source/Extension/ScriptDefines/TerrainScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnTerrainTileLoaded(CMapArea* area) {
    CALL_HOOKS(TerrainScript, script->OnTerrainTileLoaded(*area));
}

void ExtensionMgr::OnTerrainTileUnload(CMapArea* area) {
    CALL_HOOKS(TerrainScript, script->OnTerrainTileUnload(*area));
}

void ExtensionMgr::OnTerrainChunkCreated(CMapChunk* chunk) {
    CALL_HOOKS(TerrainScript, script->OnTerrainChunkCreated(*chunk));
}

void ExtensionMgr::OnTerrainChunkPurge(CMapChunk* chunk) {
    CALL_HOOKS(TerrainScript, script->OnTerrainChunkPurge(*chunk));
}

TerrainScript::TerrainScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<TerrainScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered TerrainScript %s", name);
}
