#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_UNITSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_UNITSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"

class AuraData;
class CDataStore;
class CGUnit_C;

// Units and players (CGUnit_C, WoW/Source/Object/ObjectClient/Unit_C.cpp): the method-level
// seams of a unit's life that ObjectScript's create/update/remove/destroy does not see. Every hook
// passes the native unit by reference; unit.m_obj->m_type & TYPEMASK_PLAYER tells a player, and
// unit.m_obj->m_guid == ClntObjMgrGetActivePlayer() the local player.
class UnitScript : public ScriptObject {
    protected:
    explicit UnitScript(const char* name);

    public:
    // End of CGUnit_C::OnMoveUpdate: the unit's world object, vehicle matrix, ground terrain type
    // (m_terrainTypeId), flight and swimming status are updated for a movement step. Fires for
    // every unit that moved (server movement and the local player's own movement alike). time is
    // the movement timestamp.
    virtual void OnUnitMoveUpdate(CGUnit_C& unit, uint32_t time) {}

    // End of CGUnit_C::UpdateDisplayInfo when it did rebuild the display (an early return skips
    // the rebuild and this hook when nothing changed and force was 0): model, mount attachment,
    // geosets, base animation, creature sounds and the object effect manager reflect the current
    // display id, items and shapeshift. The right place to adjust or decorate a unit's model.
    virtual void OnUnitDisplayInfoUpdated(CGUnit_C& unit) {}

    // Start of CGUnit_C::OnDeath: the unit's health reached zero; its combat state is about to be
    // cleared and the death animation started. Everything about the unit is still as alive.
    virtual void OnUnitDeath(CGUnit_C& unit) {}

    // End of CGUnit_C::OnAuraAdded: the aura's visual effect is applied (AddAuraEffect), the
    // combat log line written and, for the local player, the action bar / tracking / shapeshift
    // side effects run. index is the aura slot, spellId the Spell.dbc id.
    virtual void OnUnitAuraAdded(CGUnit_C& unit, uint32_t index, AuraData& aura, int32_t spellId) {}

    // Start of CGUnit_C::OnAuraRemoved: the aura is still applied; RemoveAuraEffect and the local
    // player side effects follow.
    virtual void OnUnitAuraRemoved(CGUnit_C& unit, uint32_t index, AuraData& aura, int32_t spellId) {}

    // End of CGUnit_C::UpdateAuraEffectMask: unit.m_auraEffectMask is rebuilt from the unit's
    // auras (one bit per SpellAura effect type present). Fires whenever the aura set changes.
    virtual void OnUnitAuraEffectMaskUpdated(CGUnit_C& unit) {}

    // End of CGUnit_C::UpdateWorldTransform: the smoothed ground normal is updated and the unit's
    // model received its world transform (position, render facing, true scale, ground normal).
    // Per frame per displayed unit: keep it cheap.
    virtual void OnUnitWorldTransformUpdated(CGUnit_C& unit, float elapsed) {}

    // CGUnit_C::SendMovementUpdate right before the message goes to the connection: the local
    // player's (or controlled unit's) movement message is complete and finalized (cursor at the
    // 32-bit opcode: MSG_MOVE_*, CMSG_MOVE_*_ACK, CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE). Fires
    // only for messages the unit is allowed to send. Observe only; NetScript::OnMessageSent sees
    // the same message a step later.
    virtual void OnUnitMovementMessage(CGUnit_C& unit, uint32_t msgId, CDataStore& msg) {}
};

#endif
