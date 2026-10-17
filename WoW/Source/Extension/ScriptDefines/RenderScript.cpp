#include "WoW/Source/Extension/ScriptDefines/RenderScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnFrameBegin(float elapsedSec) {
    CALL_HOOKS(RenderScript, script->OnFrameBegin(elapsedSec));
}

void ExtensionMgr::OnWorldRendered() {
    CALL_HOOKS(RenderScript, script->OnWorldRendered());
}

void ExtensionMgr::OnFrameEnd() {
    CALL_HOOKS(RenderScript, script->OnFrameEnd());
}

RenderScript::RenderScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<RenderScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered RenderScript %s", name);
}
