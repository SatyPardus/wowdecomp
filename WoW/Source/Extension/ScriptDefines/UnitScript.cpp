#include "WoW/Source/Extension/ScriptDefines/UnitScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

void ExtensionMgr::OnUnitMoveUpdate(CGUnit_C* unit, uint32_t time) {
    CALL_HOOKS(UnitScript, script->OnUnitMoveUpdate(*unit, time));
}

void ExtensionMgr::OnUnitDisplayInfoUpdated(CGUnit_C* unit) {
    CALL_HOOKS(UnitScript, script->OnUnitDisplayInfoUpdated(*unit));
}

void ExtensionMgr::OnUnitDeath(CGUnit_C* unit) {
    CALL_HOOKS(UnitScript, script->OnUnitDeath(*unit));
}

void ExtensionMgr::OnUnitAuraAdded(CGUnit_C* unit, uint32_t index, AuraData* aura, int32_t spellId) {
    CALL_HOOKS(UnitScript, script->OnUnitAuraAdded(*unit, index, *aura, spellId));
}

void ExtensionMgr::OnUnitAuraRemoved(CGUnit_C* unit, uint32_t index, AuraData* aura, int32_t spellId) {
    CALL_HOOKS(UnitScript, script->OnUnitAuraRemoved(*unit, index, *aura, spellId));
}

void ExtensionMgr::OnUnitAuraEffectMaskUpdated(CGUnit_C* unit) {
    CALL_HOOKS(UnitScript, script->OnUnitAuraEffectMaskUpdated(*unit));
}

void ExtensionMgr::OnUnitWorldTransformUpdated(CGUnit_C* unit, float elapsed) {
    CALL_HOOKS(UnitScript, script->OnUnitWorldTransformUpdated(*unit, elapsed));
}

void ExtensionMgr::OnUnitMovementMessage(CGUnit_C* unit, uint32_t msgId, CDataStore* msg) {
    CALL_HOOKS(UnitScript, script->OnUnitMovementMessage(*unit, msgId, *msg));
}

UnitScript::UnitScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<UnitScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered UnitScript %s", name);
}
