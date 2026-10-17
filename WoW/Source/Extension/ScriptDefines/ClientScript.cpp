#include "WoW/Source/Extension/ScriptDefines/ClientScript.h"
#include "Engine/Source/Event/Event.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

// The core's EVENT_ID_UPDATE handler. One handler for all scripts: the client registers it at the
// end of InitializeClient (after its own FrameTime update handler, so FrameTime is current when the
// scripts run) and unregisters it at the start of WowClientDestroy. Returning 1 lets the remaining
// handlers of the event run.
static int32_t OnUpdateHandler(const void* data, void* param) {
    const EVENT_DATA_UPDATE* update = static_cast<const EVENT_DATA_UPDATE*>(data);
    sExtensionMgr->OnUpdate(update->m_elapsedSec);

    return 1;
}

void ExtensionMgr::OnStartup() {
    CALL_HOOKS(ClientScript, script->OnStartup());
}

void ExtensionMgr::OnClientInitialized() {
    EventRegister(EVENT_ID_UPDATE, OnUpdateHandler);

    CALL_HOOKS(ClientScript, script->OnClientInitialized());
}

void ExtensionMgr::OnUpdate(float elapsedSec) {
    CALL_HOOKS(ClientScript, script->OnUpdate(elapsedSec));
}

void ExtensionMgr::OnClientDestroy() {
    EventUnregister(EVENT_ID_UPDATE, OnUpdateHandler);

    CALL_HOOKS(ClientScript, script->OnClientDestroy());
}

void ExtensionMgr::OnEnterWorld(uint32_t mapId) {
    CALL_HOOKS(ClientScript, script->OnEnterWorld(mapId));
}

void ExtensionMgr::OnLeaveWorld() {
    CALL_HOOKS(ClientScript, script->OnLeaveWorld());
}

void ExtensionMgr::OnShutdown() {
    CALL_HOOKS(ClientScript, script->OnShutdown());
}

ClientScript::ClientScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<ClientScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered ClientScript %s", name);
}
