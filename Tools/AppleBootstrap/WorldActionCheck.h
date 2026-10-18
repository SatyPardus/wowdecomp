#pragma once
@class WowNativeView;
// Explicit local development check using the character's existing bindings
// and action bar. Returns true after the original UI/server round trip.
bool CheckNativeWorldActions(WowNativeView* view);
