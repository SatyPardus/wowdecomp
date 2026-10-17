#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_MAPOBJECTSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_MAPOBJECTSCRIPT_H

#include "WoW/Source/Extension/ScriptObject.h"

class CMapObj;
class CMapObjGroup;

// World map objects, the WMOs (WoW/Source/WorldClient/MapObj*.cpp). CMapObj::Create looks a
// .wmo up in the shared cache or reads the root file asynchronously; its completion parses the
// root (CMapObj::Load: header, materials, group info, portals, lights, doodad sets, fogs) and
// allocates one CMapObjGroup per group. Groups (<name>_NNN.wmo) are read on demand by
// CMapObj::ReadGroup, asynchronously or synchronously during a preload, and CMapObjGroup::Create
// parses them (geometry, batches, BSP, liquid, lighting). Placements (CMapObjDef) reference the
// shared CMapObj.
class MapObjectScript : public ScriptObject {
    protected:
    explicit MapObjectScript(const char* name);

    public:
    // End of CMapObj::PostloadCallback: the root file is parsed, materials created, bounding box
    // and ambient colour set, the group array allocated (mapObj.m_mapObjGroupCount entries, none
    // loaded). mapObj.m_wmoName is the path. Runs from the async-read completion.
    virtual void OnMapObjLoaded(CMapObj& mapObj) {}

    // After CMapObjGroup::Create, in both paths that run it (CMapObjGroup::AsyncPostloadCallback
    // for an asynchronous read, CMapObj::ReadGroup for a preload): the group's geometry,
    // batches and optional data are parsed and it is linked into group.m_parent's group list.
    virtual void OnMapObjGroupLoaded(CMapObjGroup& group) {}
};

#endif
