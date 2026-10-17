#include "WoW/Source/Extension/ScriptDefines/AssetScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

bool ExtensionMgr::OnFileOpen(const char* name, char* redirect, uint32_t redirectSize) {
    // First script that redirects wins; the remaining scripts do not see the request.
    CALL_HOOKS(AssetScript, if (script->OnFileOpen(name, redirect, redirectSize)) { return true; });

    return false;
}

void ExtensionMgr::OnFileOpened(const char* name, SFileImpl* file) {
    CALL_HOOKS(AssetScript, script->OnFileOpened(name, *file));
}

AssetScript::AssetScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<AssetScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered AssetScript %s", name);
}
