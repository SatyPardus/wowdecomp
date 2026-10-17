#include "WoW/Source/Extension/ScriptDefines/ClientDBScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnClientDBLoaded(WowClientDB_Base* db, const char* fileName) {
    CALL_HOOKS(ClientDBScript, script->OnClientDBLoaded(*db, fileName));
}

void ExtensionMgr::OnClientDBInitialize() {
    CALL_HOOKS(ClientDBScript, script->OnClientDBInitialize());
}

void ExtensionMgr::OnClientDBShutdown() {
    CALL_HOOKS(ClientDBScript, script->OnClientDBShutdown());
}

ClientDBScript::ClientDBScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<ClientDBScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered ClientDBScript %s", name);
}
