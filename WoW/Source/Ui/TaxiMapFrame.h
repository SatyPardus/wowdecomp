#ifndef WOW_SOURCE_UI_TAXIMAPFRAME_H
#define WOW_SOURCE_UI_TAXIMAPFRAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Object/WGUID.h"

class CTexture;
class TaxiNodesRec;
class TaxiPathNodeRec;
class TaxiPathRec;
struct lua_State;

struct TaxiNodeInfo {
    /* 0000 */ TaxiNodesRec* m_rec;
    /* 0004 */ float m_x;
    /* 0008 */ float m_y;
    /* 000C */ uint8_t m_noDirectPath;
    /* 000D */ uint8_t m_reachable;
    /* 000E */ uint8_t unused_000E[2];
    /* 0010 */ int32_t m_costs[5];
    /* 0024 */ uint32_t* m_path;
    /* 0028 */ uint32_t m_numHops;
    /* 002C */ float m_distance;
};

struct TAXIPATHNODESCLIENT : public TSHashObject<TAXIPATHNODESCLIENT, HASHKEY_NONE> {
    /* 0018 */ TaxiPathNodeRec* m_firstNode;
    /* 001C */ int32_t m_numNodes;
};

struct TAXILINE {
    /* 0000 */ TaxiNodesRec* m_src;
    /* 0004 */ TaxiNodesRec* m_dest;
};

struct TAXIROUTELINE {
    /* 0000 */ C3Vector m_src;
    /* 000C */ C3Vector m_dest;
};

class CGTaxiMap {
    public:
    static void CloseMap();
    static void LeaveWorld();
    static CTexture* m_mapTexture;
    static int32_t m_numNodes;
    static TSGrowableArray<TaxiNodeInfo> m_nodes;
    static TSFixedArray<TSGrowableArray<TAXIROUTELINE>> m_routeLines;
    static WOWGUID m_unit;
    static uint32_t m_maxNodeID;
    static uint32_t m_nodeMaskSize;
    static void InitializeGame();
    static void ShutdownGame();
    static void FreeRouteLines();
    static void RegisterScriptFunctions();
    static void UnregisterScriptFunctions();
    static void SetupMap(const WOWGUID& unit, int32_t currentNode, const uint64_t* nodeMask);
    static void GeneratePaths();
    static void RecursePath(uint32_t index, uint32_t parent, int32_t cost);
    static float CalculateAccurateDistance(uint32_t index, uint32_t parent);
    static void TaxiNodeSetCurrent(uint32_t index);
    static void TakeTaxiNode(uint32_t index);
    static const char* TaxiNodeType(uint32_t index);
};

bool TaxiRouteExists(uint32_t from, uint32_t to);
int32_t TaxiNodeCost(uint32_t index);


#endif
