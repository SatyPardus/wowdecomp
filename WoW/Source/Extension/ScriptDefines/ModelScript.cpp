#include "WoW/Source/Extension/ScriptDefines/ModelScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnModelSharedLoaded(CM2Shared* shared) {
    CALL_HOOKS(ModelScript, script->OnModelSharedLoaded(*shared));
}

void ExtensionMgr::OnModelSkinLoaded(CM2Shared* shared) {
    CALL_HOOKS(ModelScript, script->OnModelSkinLoaded(*shared));
}

void ExtensionMgr::OnModelTexturesCreated(CM2Shared* shared) {
    CALL_HOOKS(ModelScript, script->OnModelTexturesCreated(*shared));
}

void ExtensionMgr::OnModelSequenceLoaded(CM2Shared* shared, uint32_t sequenceIndex) {
    CALL_HOOKS(ModelScript, script->OnModelSequenceLoaded(*shared, sequenceIndex));
}

void ExtensionMgr::OnModelAnimated(CM2Model* model) {
    CALL_HOOKS(ModelScript, script->OnModelAnimated(*model));
}

void ExtensionMgr::OnModelSharedDestroy(CM2Shared* shared) {
    CALL_HOOKS(ModelScript, script->OnModelSharedDestroy(*shared));
}

void ExtensionMgr::OnModelCreated(CM2Model* model) {
    CALL_HOOKS(ModelScript, script->OnModelCreated(*model));
}

void ExtensionMgr::OnModelLoaded(CM2Model* model) {
    CALL_HOOKS(ModelScript, script->OnModelLoaded(*model));
}

void ExtensionMgr::OnModelDestroy(CM2Model* model) {
    CALL_HOOKS(ModelScript, script->OnModelDestroy(*model));
}

ModelScript::ModelScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<ModelScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered ModelScript %s", name);
}
