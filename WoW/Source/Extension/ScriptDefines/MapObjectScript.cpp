#include "WoW/Source/Extension/ScriptDefines/MapObjectScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnMapObjLoaded(CMapObj* mapObj) {
    CALL_HOOKS(MapObjectScript, script->OnMapObjLoaded(*mapObj));
}

void ExtensionMgr::OnMapObjGroupLoaded(CMapObjGroup* group) {
    CALL_HOOKS(MapObjectScript, script->OnMapObjGroupLoaded(*group));
}

MapObjectScript::MapObjectScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<MapObjectScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered MapObjectScript %s", name);
}
