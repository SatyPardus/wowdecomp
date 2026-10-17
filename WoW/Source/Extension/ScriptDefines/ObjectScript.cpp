#include "WoW/Source/Extension/ScriptDefines/ObjectScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnObjectCreated(CGObject_C* object) {
    CALL_HOOKS(ObjectScript, script->OnObjectCreated(*object));
}

void ExtensionMgr::OnObjectUpdate(CGObject_C* object, uint32_t time) {
    CALL_HOOKS(ObjectScript, script->OnObjectUpdate(*object, time));
}

void ExtensionMgr::OnObjectRemoved(CGObject_C* object) {
    CALL_HOOKS(ObjectScript, script->OnObjectRemoved(*object));
}

void ExtensionMgr::OnObjectDestroy(CGObject_C* object) {
    CALL_HOOKS(ObjectScript, script->OnObjectDestroy(*object));
}

void ExtensionMgr::OnObjectSpellVisualKit(CGObject_C* object, const PlaySpellVisuaKitData* data) {
    CALL_HOOKS(ObjectScript, script->OnObjectSpellVisualKit(*object, *data));
}

ObjectScript::ObjectScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<ObjectScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered ObjectScript %s", name);
}
