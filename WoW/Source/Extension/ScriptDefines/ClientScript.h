#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_CLIENTSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_CLIENTSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"

// The process lifecycle: engine services, client systems, the game session and the per-frame tick.
// Every hook is fired from WoW/Source/Client.cpp except OnUpdate, which the core fans out from an
// EVENT_ID_UPDATE handler it registers itself.
class ClientScript : public ScriptObject {
    protected:
    explicit ClientScript(const char* name);

    public:
    // End of InitializeGlobal(): Config.wtf and the CVars are loaded, console commands can be
    // registered, the event system, the window and the Gx device exist. No client system is up yet
    // (no FrameScript, no world, no glue screen). Register CVars and console commands here.
    virtual void OnStartup() {}

    // End of InitializeClient(), inside the event context's initialize callback: FrameScript, the
    // world, shadows, lighting, input and the glue manager exist and the first glue screen is set.
    // The message loop has not run a frame yet. This is where the core registers its per-frame
    // update handler, so OnUpdate starts after this hook.
    virtual void OnClientInitialized() {}

    // Every message-loop iteration, from an EVENT_ID_UPDATE handler the core registered at the end
    // of InitializeClient(). It runs after FrameTime::Update (the client's own update handler was
    // registered first and handlers of one event run in registration order), so FrameTime is
    // current. elapsedSec is the frame time the event scheduler measured. Fires in the glue screens
    // as well as in the world; use WorldScript::OnWorldUpdate for world-only work.
    virtual void OnUpdate(float elapsedSec) {}

    // Start of WowClientDestroy(), inside the event context's destroy callback: the game session
    // is already gone (ClientDestroyGame ran), the glue, FrameScript state, world and client
    // services still exist. Mirror of OnClientInitialized; OnUpdate stops before this hook.
    virtual void OnClientDestroy() {}

    // End of ClientInitializeGame(): the player entered the world, the object manager, the game UI
    // and the map are loaded, s_gameInitialized is set. mapId is the Map.dbc id being played.
    virtual void OnEnterWorld(uint32_t mapId) {}

    // Start of ClientDestroyGame(), after its s_gameInitialized check: the player is leaving the
    // world (logout, character select, disconnect). Everything game-side is still valid.
    virtual void OnLeaveWorld() {}

    // Start of BeginCloseGame(): the message loop has ended, the event context and the client are
    // destroyed, the engine is about to go away. CVars, console commands and the event system
    // still exist, so state can be saved.
    virtual void OnShutdown() {}
};

#endif
