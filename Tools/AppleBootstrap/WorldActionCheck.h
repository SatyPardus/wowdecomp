#pragma once
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

// Native picking/action-bar Moonfire, or the existing T attack binding, against
// a nearby non-player critter. Approach uses original movement and collision.
bool CheckNativeWorldCombat(WowNativeView* view, bool melee = false);
