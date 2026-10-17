#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_OBJECTSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_OBJECTSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"

class CGObject_C;
class PlaySpellVisuaKitData;

// The client object manager (WoW/Source/ObjectMgrClient) and its objects: every unit, player,
// game object, dynamic object, corpse, item and container the server describes through
// SMSG_UPDATE_OBJECT is a CGObject_C subclass (CGUnit_C, CGPlayer_C, CGGameObject_C, ...). An
// object is created from the first full update, lives in the active set (and in the visible list
// while in range), is unlinked to a lazy-cleanup list when the server removes it or it goes out
// of range, and is destroyed when that list expires it or the manager shuts down. Every hook
// passes the native object by reference; object->m_obj->m_type tells the kind, and the usual
// casts (static_cast<CGUnit_C*> after a TYPEMASK_UNIT check) apply.
class ObjectScript : public ScriptObject {
    protected:
    explicit ObjectScript(const char* name);

    public:
    // CreateObject (ObjectMgrClient.cpp) once a new object is allocated, its descriptor storage
    // set up, its fields filled from the update block, InitObject run (movement, position, type
    // specific setup) and the object linked into the visible list. Does not fire for an object
    // the server re-sends while it is still known (that path refreshes the fields and re-enables
    // it). The active player's own object comes through here too.
    virtual void OnObjectCreated(CGObject_C& object) {}

    // UnitUpdateProc (WoW/Source/Ui/WorldFrame.cpp), the per-frame pass over the visible
    // objects during CGWorldFrame::OnWorldUpdate, after the object's own UpdateDisplay (units,
    // game objects and dynamic objects have one; other kinds pass straight through). time is
    // FrameTime::s_curTimeMs. Once per visible object per world frame: keep it cheap.
    virtual void OnObjectUpdate(CGObject_C& object, uint32_t time) {}

    // ObjDelete (ObjectMgrClient.cpp): the object leaves the active set (out of range or
    // destroyed by the server): its mirror lists are cleared and it moves to the lazy-cleanup
    // list, still allocated and readable. OnObjectDestroy follows later, or OnObjectCreated
    // again if the server re-sends it first (the reenable path recovers it from that list).
    virtual void OnObjectRemoved(CGObject_C& object) {}

    // DestroyRemovedObject (ObjectMgrClient.cpp) before the destructor runs: the object is
    // about to be freed, from lazy-cleanup expiry or from the manager's own shutdown. Release
    // anything keyed by this object's address here.
    virtual void OnObjectDestroy(CGObject_C& object) {}

    // CGObject_C::PlaySpellVisualKit once the request has a kit to play (requests without an
    // impact kit return before this): data carries the spell record, the SpellVisualKit record,
    // the kit type, an optional world position (transformed to the transport's frame by the
    // engine afterwards) and the caster/target guids. The effects, animations, sounds and
    // weapon trails of the kit are started after the hook.
    virtual void OnObjectSpellVisualKit(CGObject_C& object, const PlaySpellVisuaKitData& data) {}
};

#endif
