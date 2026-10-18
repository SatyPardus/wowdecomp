## OnEngineInit

status: not-wired

Intended for low-level engine initialization. `ModManager::OnEngineInit()` exists, but no engine code calls it.

## OnEngineShutdown

status: not-wired

`ModManager::Shutdown()` runs from `WowClientDestroy` and stops the file watcher, but it does not dispatch this hook.

## OnGameInitialize

source: `ClientInitializeGame` (`Client.cpp`)

Runs after the game clock is initialized and before the command manager is set up.

## OnGameDestroy

source: `ClientDestroyGame` (`Client.cpp`)

Runs before `CGGameUI::ShutdownGame`.

## OnFrameTick

status: not-wired

Intended for every main-loop iteration. Use [`OnWorldUpdate`](/reference/hooks/lifecycle-world/onworldupdate/) (in the world) or [`OnGlueIdle`](/reference/hooks/glue/onglueidle/) (on Glue screens) instead.

## OnEnterWorld

source: `CGGameUI` (`GameUI.cpp`)

## OnLeaveWorld

source: `CGGameUI` (`GameUI.cpp`)

## OnMapChange

source: `CWorld` (`World.cpp`)

## OnWorldUpdate

status: partial
source: `CGWorldFrame` update (`WorldFrame.cpp`)

:::caution[dt is always 0]
The engine calls `ModManager::OnWorldUpdate()` with its default argument, so `dt` is `0.0`. Measure time yourself, for example with `GetTime()`.
:::

This tick also applies [hot reload](/guides/hot-reload/): modified scripts are re-executed just before the hook is dispatched.

## OnWorldRender

source: `CGWorldFrame` render pass (`WorldFrame.cpp`)

`worldFrame` is an opaque pointer to the `CGWorldFrame`.

## OnWorldHitTest

source: `CGWorldFrame` hit test (`WorldFrame.cpp`)

- Also updates the values returned by `ModCore.GetLastHitGuid()` and `ModCore.GetLastHitPoint()`.
- `point` is `nil` when there is no hit point.
- `hitObject` is true when the hit kind is an object rather than terrain.

## OnTerrainTrack

source: `CGWorldFrame::OnLayerTrackTerrain` (`WorldFrame.cpp`)

Returning `true` stops the default tracking handler (the ground-target shadow and cursor reset).

When [`ModCore.SetCastAtCursor(true)`](/reference/api/modcore/#setcastatcursor) is armed, the engine disarms it and issues a terrain click **before** this hook runs.

## OnTerrainClick

source: `CGGameUI::OnTerrainClick` (`GameUI.cpp`)

Returning `true` stops default handling, including ground-targeted spell casts.

`ModCore.ClickTerrain` calls `CGGameUI::HandleTerrainClick`, which can re-trigger this hook. Guard against re-entrancy if you call it from a listener.

## OnZoneChange

source: `CGGameUI` zone text update (`GameUI.cpp`)

## OnWeatherChange

source: `MapWeather.cpp`

## OnCameraUpdate

source: `CGWorldFrame` (`WorldFrame.cpp`)

Fires every frame the world camera is updated. `fov` is the camera's field of view.
