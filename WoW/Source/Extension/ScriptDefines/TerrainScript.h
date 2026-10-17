#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_TERRAINSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_TERRAINSCRIPT_H

#include "WoW/Source/Extension/ScriptObject.h"

class CMapArea;
class CMapChunk;

// Terrain tiles (WoW/Source/WorldClient/MapArea.cpp). A CMapArea is one ADT tile of the current
// map: CMap::PrepareUpdate creates the tiles of the area of interest and CMapArea::Load issues
// the asynchronous read of World\Maps\<map>\<map>_<x>_<y>.adt; the completion callback parses
// the file (ProcessIffChunks: header, chunk table, texture names, doodad and WMO placements,
// liquid data) and the 16x16 chunks are then built on demand (CMapArea::PrepareChunk). Tiles
// that leave the area of interest are purged. The map itself is a WorldScript matter
// (OnMapLoaded / OnMapUnload).
class TerrainScript : public ScriptObject {
    protected:
    explicit TerrainScript(const char* name);

    public:
    // CMapArea::AsyncCallback after ProcessIffChunks: the ADT is parsed, area.m_index is the
    // tile coordinate, m_doodadDef / m_mapObjDef with their counts list the placements, the
    // terrain textures are requested, m_liquid holds the MH2O data when present. No chunk is
    // built yet. Runs on the main thread from the async-read completion.
    virtual void OnTerrainTileLoaded(CMapArea& area) {}

    // Start of CMapArea::Purge: the tile is leaving the area of interest (or the map is being
    // unloaded); its chunks and file buffer are about to be released. A pending read is
    // cancelled, so a tile may be purged without ever having fired OnTerrainTileLoaded.
    virtual void OnTerrainTileUnload(CMapArea& area) {}

    // CMapArea::PrepareChunk after CMapChunk::Create: one of the tile's 16x16 chunks is built
    // from its MCNK data (heights, normals, layers, shadow map, liquid; the detail doodads are
    // loaded later on demand). chunk.m_aIndex is the chunk's position in the tile, m_cOffset its
    // position on the map. Chunks are built as the area of interest reaches them.
    virtual void OnTerrainChunkCreated(CMapChunk& chunk) {}

    // Start of CMapChunk::Purge: the chunk's render data, detail doodads and liquid are about to
    // be released (tile purge or chunk rebuild).
    virtual void OnTerrainChunkPurge(CMapChunk& chunk) {}
};

#endif
