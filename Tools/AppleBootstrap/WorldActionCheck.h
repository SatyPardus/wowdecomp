#pragma once
#include <stdexcept>
// Host check failures occur between completed game frames and can close normally.
class NativeWorldCheckFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
@class WowNativeView;
// Explicit local development check using the character's existing bindings
// and action bar. Returns true after the original UI/server round trip.
bool CheckNativeWorldActions(WowNativeView* view);
// Original cast bar, native Escape cancellation and a completed healing cast.
bool CheckNativeWorldCasting(WowNativeView* view);
// Original live shadow-quality CVar, resource recreation and restoration.
bool CheckNativeWorldShadows();
// Original gxMultisample/gxRestart through live world frames and restoration.
bool CheckNativeWorldMultisample();
// Native picking and an existing nearby NPC's original server gossip dialog.
bool CheckNativeWorldNPC(WowNativeView* view, bool browseMerchant = false);
// An already learned Teleport: Moonglade through the original spellbook entry.
bool CheckNativeWorldTravel();
// Existing backpack Hearthstone, native item click and server-directed travel.
bool CheckNativeWorldHearthstone(WowNativeView* view);
// Original game-menu logout and character-selection reentry, using native UI
// events and the same character. No forced logout or object-manager reset.
bool CheckNativeWorldReentry(WowNativeView* view);
// Native character, spellbook, quest, talent and map toggles with live drawing.
bool CheckNativeWorldPanels(WowNativeView* view);
// Read a nearby game-object quest offer through native picking; no acceptance.
bool CheckNativeWorldQuest(WowNativeView* view);
// Move/swap the existing backpack Hearthstone and restore the full layout using
// the original item cursor, native left clicks and server inventory updates.
bool CheckNativeWorldInventory(WowNativeView* view);
// Explicit local route through the existing W/arrow bindings and collision.
// WOW_WORLD_WALK contains x:y waypoints separated by semicolons.
bool CheckNativeWorldWalk(WowNativeView* view, bool recoverCorpse = false);
// Follow the explicit route as a ghost, stop in original corpse range and accept
// the original recovery popup through native mouse input.
bool CheckNativeWorldCorpse(WowNativeView* view);

// Native picking/action-bar Moonfire, or the existing T attack binding, against
// a nearby non-player critter. Approach uses original movement and collision.
// Encounter mode instead fights a low-level non-critter to death and acquires
// available loot through the original loot buttons and server updates.
bool CheckNativeWorldCombat(WowNativeView* view, bool melee = false, bool encounter = false, bool lootOnly = false);
