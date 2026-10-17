#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_WORLDSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_WORLDSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"

class C3Vector;

// The world scene: the server's world-session messages, map boundaries and the world tick. Map
// hooks fire from WoW/Source/WorldClient/World.cpp, the tick from WoW/Source/Ui/WorldFrame.cpp,
// the session ones from WoW/Source/Client.cpp and WowSvcs/WowSvcsClient/ClientServices.cpp. Map
// boundaries differ from ClientScript::OnEnterWorld/OnLeaveWorld, which are session boundaries:
// a server transfer (SMSG_NEW_WORLD) unloads and loads a map without leaving the game.
class WorldScript : public ScriptObject {
    protected:
    explicit WorldScript(const char* name);

    public:
    // Packet_SMSG_LOGIN_VERIFY_WORLD (Client.cpp) once the fields are read and before the
    // client decides whether the map changed: the server states which map and where the
    // character stands. Fires for the initial world login and for every transfer confirmation;
    // when mapId is the current map the client does nothing further, otherwise it loads the
    // new world (LoadNewWorld -> OnMapUnload / OnMapLoaded).
    virtual void OnLoginVerifyWorld(uint32_t mapId, const C3Vector& position, float facing) {}

    // Start of ClientConnection::HandleLogoutComplete (ClientServices.cpp): SMSG_LOGOUT_COMPLETE
    // arrived; the character is still in the world for this call. For the current connection
    // ClientDestroyGame follows immediately (OnLeaveWorld, OnMapUnload), then the glue resumes.
    virtual void OnLogoutComplete() {}

    // End of CWorld::LoadMap(): the map (CMap::Load), its area of interest, the liquid material
    // settings and the clip planes are set up. mapId is the Map.dbc id, mapName its directory
    // name (Map.dbc m_directory). Fires on world entry and on every map transfer.
    virtual void OnMapLoaded(uint32_t mapId, const char* mapName) {}

    // World::UnloadMap(), before CMap::Unload(): the fade-out list is already cleared, the map and
    // its objects are still loaded. Fires only when a map was active (the call from a session that
    // never loaded one does not reach it).
    virtual void OnMapUnload() {}

    // End of CGWorldFrame::OnWorldUpdate(): the camera, the active mover, the visible objects,
    // CWorld::Update (area of interest, day/night, fog) and the sound listener are updated for
    // this frame and the world is about to be rendered. elapsedSec is the world frame's time.
    // Fires only while the world frame is active (in the world, not in the glue).
    virtual void OnWorldUpdate(float elapsedSec) {}
};

#endif
