#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"

namespace {

// Logs the local player's unit events (auras, display rebuilds, death) and counts the rest.
class HelloWorldUnitScript : public UnitScript {
    public:
    HelloWorldUnitScript() : UnitScript("HelloWorldUnitScript") {}

    void OnUnitMoveUpdate(CGUnit_C& unit, uint32_t time) override {
        m_moves++;

        if (m_moves == 1) {
            sExtensionMgr->Log("HelloWorld: OnUnitMoveUpdate (first, t=%u)", time);
        }
    }

    void OnUnitDisplayInfoUpdated(CGUnit_C& unit) override {
        m_displays++;

        if (IsLocalPlayer(unit)) {
            sExtensionMgr->Log("HelloWorld: OnUnitDisplayInfoUpdated (local player, %u rebuild(s) in total)", m_displays);
        }
    }

    void OnUnitDeath(CGUnit_C& unit) override {
        m_deaths++;
        sExtensionMgr->Log("HelloWorld: OnUnitDeath (%s, %u death(s))", IsLocalPlayer(unit) ? "local player" : "unit", m_deaths);
    }

    void OnUnitAuraAdded(CGUnit_C& unit, uint32_t index, AuraData& aura, int32_t spellId) override {
        if (IsLocalPlayer(unit)) {
            sExtensionMgr->Log("HelloWorld: OnUnitAuraAdded slot %u spell %d", index, spellId);
        }
    }

    void OnUnitAuraRemoved(CGUnit_C& unit, uint32_t index, AuraData& aura, int32_t spellId) override {
        if (IsLocalPlayer(unit)) {
            sExtensionMgr->Log("HelloWorld: OnUnitAuraRemoved slot %u spell %d", index, spellId);
        }
    }

    void OnUnitAuraEffectMaskUpdated(CGUnit_C& unit) override {
        m_masks++;
    }

    void OnUnitWorldTransformUpdated(CGUnit_C& unit, float elapsed) override {
        m_transforms++;

        if (m_transforms == 1) {
            sExtensionMgr->Log("HelloWorld: OnUnitWorldTransformUpdated (first, %.4f s)", elapsed);
        }
    }

    void OnUnitMovementMessage(CGUnit_C& unit, uint32_t msgId, CDataStore& msg) override {
        m_movementMessages++;

        if (m_movementMessages <= 3) {
            sExtensionMgr->Log("HelloWorld: OnUnitMovementMessage opcode 0x%03X (%u aura mask update(s), %u transform(s) so far)", msgId, m_masks, m_transforms);
        }
    }

    private:
    static bool IsLocalPlayer(CGUnit_C& unit) {
        return unit.m_obj->m_guid == ClntObjMgrGetActivePlayer();
    }

    uint32_t m_moves = 0;
    uint32_t m_displays = 0;
    uint32_t m_deaths = 0;
    uint32_t m_masks = 0;
    uint32_t m_transforms = 0;
    uint32_t m_movementMessages = 0;
};

}

void AddHelloWorldUnitScripts() {
    new HelloWorldUnitScript();
}
