#include "TaxiMapFrame.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/TaxiNodesRec.h"
#include "WoW/Source/DB/Rec/TaxiPathRec.h"
#include "WoW/Source/DB/Rec/TaxiPathNodeRec.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Base/Types.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/WorldMapContinentRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/WorldMap.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cmath>

// STATIC: 0x00C0D7E0
CTexture* CGTaxiMap::m_mapTexture;

// STATIC: 0x00C0D7E4
int32_t CGTaxiMap::m_numNodes;

// STATIC: 0x00C0DC30
TSGrowableArray<TaxiNodeInfo> CGTaxiMap::m_nodes;

// STATIC: 0x00C0DC80
TSFixedArray<TSGrowableArray<TAXIROUTELINE>> CGTaxiMap::m_routeLines;

// STATIC: 0x00C0DBF0
WOWGUID CGTaxiMap::m_unit;

// STATIC: 0x00C9EA98
uint32_t CGTaxiMap::m_maxNodeID;

// STATIC: 0x00C9EA9C
uint32_t CGTaxiMap::m_nodeMaskSize;

// STATIC: 0x00C0D7E8
static int32_t s_mapID;

// STATIC: 0x00C0D7EC
static TaxiNodesRec* s_currentNode;

// STATIC: 0x00C0DBF8
static float s_taxiMinY;

// STATIC: 0x00C0DBFC
static float s_taxiMinX;

// STATIC: 0x00C0DC00
static float s_taxiMaxY;

// STATIC: 0x00C0DC04
static float s_taxiMaxX;

// STATIC: 0x00ACF250
static int32_t s_selectedNode = -1;

// STATIC: 0x00C0DC08
static TSHashTable<TAXIPATHNODESCLIENT, HASHKEY_NONE> s_pathNodes;

// STATIC: 0x00C0DC40
static TSGrowableArray<uint64_t> s_nodeMask;

// STATIC: 0x00C0DC50
static TSFixedArray<TSGrowableArray<TaxiPathRec*>> s_paths;

// STATIC: 0x00C0DC60
static TSGrowableArray<TAXILINE> s_taxiLines;

// STATIC: 0x00C0DC70
static TSGrowableArray<uint8_t> s_unused_C0DC70;

// VERIFY: cc1d814f auto
// OFFSET: 0x00590690
static float GetNodeX(const TaxiNodesRec* node) {
    int32_t mapID;
    float x;
    float y;
    int32_t dungeonMapID;
    WorldMapTransformPosition(node->m_continentID, node->m_pos[0], node->m_pos[1], &mapID, &x, &y, &dungeonMapID);
    return x;
}

// VERIFY: 447c51c1 auto
// OFFSET: 0x005906D0
static float GetNodeY(const TaxiNodesRec* node) {
    int32_t mapID;
    float x;
    float y;
    int32_t dungeonMapID;
    WorldMapTransformPosition(node->m_continentID, node->m_pos[0], node->m_pos[1], &mapID, &x, &y, &dungeonMapID);
    return y;
}

// VERIFY: d44e9914 manual
// OFFSET: 0x00590B20
static TaxiNodesRec* GetNodeMaskBit(int32_t nodeID, uint32_t* word, uint32_t* bit) {
    TaxiNodesRec* node = g_taxiNodesDB.GetRecord(nodeID);

    if (!node) {
        return nullptr;
    }

    *word = static_cast<uint32_t>(nodeID - 1) >> 6;
    *bit = (nodeID - 1) - (*word << 6);
    return node;
}

// VERIFY: c6845cea manual
// OFFSET: 0x00590B60
bool TaxiRouteExists(uint32_t from, uint32_t to) {
    return from && from <= CGTaxiMap::m_maxNodeID && to && to <= CGTaxiMap::m_maxNodeID && s_paths.m_data[from - 1].m_data[to - 1];
}

// VERIFY: 88b381ab manual
// OFFSET: 0x00590BA0
void CGTaxiMap::CloseMap() {
    if (!CGTaxiMap::m_unit) {
        return;
    }

    CGGameUI::ClearInteractTarget(CGTaxiMap::m_unit, 0);
    CGTaxiMap::m_unit.m_guid = 0;

    for (int32_t i = 0; i < CGTaxiMap::m_numNodes; i++) {
        if (CGTaxiMap::m_nodes.m_data[i].m_path) {
            delete[] CGTaxiMap::m_nodes.m_data[i].m_path;
            CGTaxiMap::m_nodes.m_data[i].m_path = nullptr;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TAXIMAP_CLOSED, nullptr);
}

// VERIFY: 8f7e4a2e manual
// OFFSET: 0x00590C30
static uint32_t GetRouteSegmentCount(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGTaxiMap::m_numNodes)) {
        return 0;
    }

    TaxiNodeInfo& node = CGTaxiMap::m_nodes.m_data[index];

    if (node.m_noDirectPath) {
        return node.m_numHops - 1;
    }

    return 1;
}

// VERIFY: 4ceda8f7 auto
// OFFSET: 0x00590C70
const char* CGTaxiMap::TaxiNodeType(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGTaxiMap::m_numNodes)) {
        return "NONE";
    }

    int32_t nodeID = CGTaxiMap::m_nodes.m_data[index].m_rec->m_id;

    if (nodeID == s_currentNode->m_id) {
        return "CURRENT";
    }

    uint32_t word;
    uint32_t bit;

    if (!CGTaxiMap::m_nodes.m_data[index].m_reachable || !GetNodeMaskBit(nodeID, &word, &bit)) {
        return "NONE";
    }

    if (s_nodeMask.m_data[word] & (1ull << bit)) {
        return "REACHABLE";
    }

    return "DISTANT";
}

// VERIFY: dec4ac64 manual
// OFFSET: 0x00591350
int32_t TaxiNodeCost(uint32_t index) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    CGUnit_C* taxiUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGTaxiMap::m_unit, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!taxiUnit) {
        return 0;
    }

    TaxiNodeInfo& node = CGTaxiMap::m_nodes.m_data[index];
    uint32_t nodeID = node.m_rec->m_id;

    if (nodeID && nodeID <= CGTaxiMap::m_maxNodeID && s_paths.m_data[s_currentNode->m_id - 1].m_data[nodeID - 1]) {
        return static_cast<int32_t>((1.0f - taxiUnit->GetPlayerDiscount(player)) * s_paths.m_data[s_currentNode->m_id - 1].m_data[nodeID - 1]->m_cost);
    }

    if (!node.m_noDirectPath) {
        return 0;
    }

    int32_t reaction = taxiUnit->UnitReaction(player);
    FactionTemplateRec* factionTemplate = g_factionTemplateDB.GetRecord(taxiUnit->m_unit->m_unitFieldFactiontemplate);

    if (factionTemplate && !CGUnit_C::FactionHasReputation(factionTemplate->m_faction)) {
        reaction = 3;
    }

    if (reaction > 3) {
        return node.m_costs[reaction - 3];
    }

    return node.m_costs[0];
}

// VERIFY: 7511acec manual
// OFFSET: 0x005914B0
void CGTaxiMap::TakeTaxiNode(uint32_t index) {
    if (index >= static_cast<uint32_t>(CGTaxiMap::m_numNodes)) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player->m_mountDisplayId > 0 && !(player->m_clientFlags & 0x10000000)) {
        CGGameUI::DisplayError(ERR_TAXIPLAYERALREADYMOUNTED);
        return;
    }

    TaxiNodeInfo& node = CGTaxiMap::m_nodes.m_data[index];
    uint32_t destination = node.m_rec->m_id;
    uint32_t source = s_currentNode->m_id;

    if (source == destination) {
        CGGameUI::DisplayError(ERR_TAXISAMENODE);
    } else if (TaxiRouteExists(source, destination)) {
        player->StartTaxi(CGTaxiMap::m_unit, source, destination);
    } else if (node.m_noDirectPath && node.m_path) {
        if (static_cast<uint32_t>(TaxiNodeCost(index)) <= player->GetCoinage()) {
            player->StartTaxiExpress(CGTaxiMap::m_unit, node.m_path, node.m_numHops);
        } else {
            CGGameUI::DisplayError(ERR_TAXINOTENOUGHMONEY);
        }
    } else {
        CGGameUI::DisplayError(ERR_TAXINOSUCHPATH);
    }
}

// VERIFY: 61e9c22c manual
// OFFSET: 0x00591AD0
void CGTaxiMap::TaxiNodeSetCurrent(uint32_t index) {
    s_selectedNode = -1;

    if (index >= static_cast<uint32_t>(CGTaxiMap::m_numNodes) || !CGTaxiMap::m_nodes.m_data[index].m_reachable) {
        return;
    }

    s_selectedNode = index;
    s_taxiLines.Clear();
    CGTaxiMap::m_routeLines.m_data[index].Clear();

    if (!CGTaxiMap::m_nodes.m_data[index].m_noDirectPath) {
        TAXILINE* line = s_taxiLines.New();
        line->m_src = s_currentNode;
        line->m_dest = CGTaxiMap::m_nodes.m_data[s_selectedNode].m_rec;
        TAXIROUTELINE* route = CGTaxiMap::m_routeLines.m_data[index].New();
        float srcX = GetNodeX(line->m_src);
        route->m_src.x = (s_taxiMaxY - GetNodeY(line->m_src)) / (s_taxiMaxY - s_taxiMinY);
        route->m_src.y = (srcX - s_taxiMinX) / (s_taxiMaxX - s_taxiMinX);
        route->m_src.z = 0.0f;
        float destX = GetNodeX(line->m_dest);
        route->m_dest.x = (s_taxiMaxY - GetNodeY(line->m_dest)) / (s_taxiMaxY - s_taxiMinY);
        route->m_dest.y = (destX - s_taxiMinX) / (s_taxiMaxX - s_taxiMinX);
        route->m_dest.z = 0.0f;
        return;
    }

    for (uint32_t i = 0; i < GetRouteSegmentCount(index); i++) {
        TAXILINE* line = s_taxiLines.New();
        line->m_src = g_taxiNodesDB.GetRecord(CGTaxiMap::m_nodes.m_data[s_selectedNode].m_path[i]);
        line->m_dest = g_taxiNodesDB.GetRecord(CGTaxiMap::m_nodes.m_data[s_selectedNode].m_path[i + 1]);
        TAXIROUTELINE* route = CGTaxiMap::m_routeLines.m_data[index].New();
        TaxiNodesRec* src = s_taxiLines.m_data[i].m_src;
        int32_t mapID;
        float x;
        float y;
        int32_t dungeonMapID;
        WorldMapTransformPosition(src->m_continentID, src->m_pos[0], src->m_pos[1], &mapID, &x, &y, &dungeonMapID);
        float srcY = y;
        WorldMapTransformPosition(src->m_continentID, src->m_pos[0], src->m_pos[1], &mapID, &x, &y, &dungeonMapID);
        route->m_src.x = (s_taxiMaxY - srcY) / (s_taxiMaxY - s_taxiMinY);
        route->m_src.y = (x - s_taxiMinX) / (s_taxiMaxX - s_taxiMinX);
        route->m_src.z = 0.0f;
        TaxiNodesRec* dest = s_taxiLines.m_data[i].m_dest;
        WorldMapTransformPosition(dest->m_continentID, dest->m_pos[0], dest->m_pos[1], &mapID, &x, &y, &dungeonMapID);
        float destY = y;
        WorldMapTransformPosition(dest->m_continentID, dest->m_pos[0], dest->m_pos[1], &mapID, &x, &y, &dungeonMapID);
        route->m_dest.x = (s_taxiMaxY - destY) / (s_taxiMaxY - s_taxiMinY);
        route->m_dest.y = (x - s_taxiMinX) / (s_taxiMaxX - s_taxiMinX);
        route->m_dest.z = 0.0f;
    }
}

// VERIFY: bc697e9d manual
// OFFSET: 0x005924B0
static bool LoadMapTexture() {
    char fileName[260];
    SStrPrintf(fileName, sizeof(fileName), "Interface\\TaxiFrame\\TAXIMAP%d.blp", s_mapID);

    if (CGTaxiMap::m_mapTexture) {
        if (TextureIsSame(CGTaxiMap::m_mapTexture, fileName)) {
            return true;
        }

        HandleClose(reinterpret_cast<HOBJECT>(CGTaxiMap::m_mapTexture));
    }

    CStatus status;
    CGTaxiMap::m_mapTexture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 0);
    return CGTaxiMap::m_mapTexture != nullptr;
}

// VERIFY: 87846e76 manual
// OFFSET: 0x00592580
float CGTaxiMap::CalculateAccurateDistance(uint32_t index, uint32_t parent) {
    float distance = 0.0f;
    TaxiPathRec* path = s_paths.m_data[CGTaxiMap::m_nodes.m_data[parent].m_rec->m_id - 1].m_data[CGTaxiMap::m_nodes.m_data[index].m_rec->m_id - 1];

    if (!path) {
        return 9.9999998e10f;
    }

    TAXIPATHNODESCLIENT* pathNodes = s_pathNodes.Ptr(path->m_id, HASHKEY_NONE());

    if (!pathNodes) {
        return 9.9999998e10f;
    }

    for (int32_t i = 0; i < pathNodes->m_numNodes; i++) {
        if (!i) {
            continue;
        }

        TaxiPathNodeRec* prev = &pathNodes->m_firstNode[i - 1];
        TaxiPathNodeRec* node = &pathNodes->m_firstNode[i];
        int32_t mapID;
        int32_t dungeonMapID;
        float prevX;
        float prevY;
        WorldMapTransformPosition(prev->m_continentID, prev->m_loc[0], prev->m_loc[1], &mapID, &prevX, &prevY, &dungeonMapID);
        float prevZ = prev->m_loc[2];
        float x;
        float y;
        WorldMapTransformPosition(node->m_continentID, node->m_loc[0], node->m_loc[1], &mapID, &x, &y, &dungeonMapID);
        float z = node->m_loc[2];
        distance = sqrt((y - prevY) * (y - prevY) + (z - prevZ) * (z - prevZ) + (x - prevX) * (x - prevX)) + distance;
    }

    return distance;
}

// VERIFY: 48c16c0c manual
// OFFSET: 0x005926C0
void CGTaxiMap::RecursePath(uint32_t index, uint32_t parent, int32_t cost) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    TaxiNodesRec* node = CGTaxiMap::m_nodes.m_data[index].m_rec;

    if (node && player) {
        FactionTemplateRec* factionTemplate = g_factionTemplateDB.GetRecord(player->m_unit->m_unitFieldFactiontemplate);

        if (factionTemplate) {
            int32_t mount0 = node->m_mountCreatureID[0];
            int32_t mount1 = node->m_mountCreatureID[1];

            if (factionTemplate->m_factionGroup == 3) {
                bool hordeMount = mount0 == 2224 || mount0 == 3574 || mount0 == 19917 || mount0 == 34709 || mount1 == 2224 || mount1 == 3574 || mount1 == 19917 || mount1 == 34709;
                bool allianceMount = mount0 == 3837 || mount0 == 541 || mount0 == 28360 || mount0 == 28361 || mount0 == 31315 || mount1 == 3837 || mount1 == 541 || mount1 == 28360 || mount1 == 28361 || mount1 == 31315;

                if (hordeMount && !allianceMount) {
                    return;
                }
            } else if (factionTemplate->m_factionGroup == 5) {
                bool allianceMount = mount0 == 3837 || mount0 == 541 || mount0 == 28360 || mount0 == 28361 || mount0 == 31315 || mount1 == 3837 || mount1 == 541 || mount1 == 28360 || mount1 == 28361 || mount1 == 31315;
                bool hordeMount = mount0 == 2224 || mount0 == 3574 || mount0 == 19917 || mount0 == 34709 || mount1 == 2224 || mount1 == 3574 || mount1 == 19917 || mount1 == 34709;

                if (allianceMount && !hordeMount) {
                    return;
                }
            }
        }
    }

    TaxiNodeInfo& parentNode = CGTaxiMap::m_nodes.m_data[parent];
    uint32_t hops = parentNode.m_numHops + 1;
    float distance = CGTaxiMap::CalculateAccurateDistance(index, parent) + parentNode.m_distance;
    TaxiNodeInfo& info = CGTaxiMap::m_nodes.m_data[index];

    if (info.m_path) {
        if (distance >= info.m_distance) {
            return;
        }

        delete[] info.m_path;
        info.m_path = new (__FILE__, __LINE__) uint32_t[hops];
    } else {
        info.m_path = new (__FILE__, __LINE__) uint32_t[hops];
        info.m_reachable = 1;
    }

    uint32_t i = 0;

    for (; i < hops - 1; i++) {
        info.m_path[i] = parentNode.m_path[i];
    }

    info.m_path[i] = info.m_rec->m_id;
    info.m_numHops = hops;
    float scale = 1.0f;

    for (uint32_t j = 0; j < 5; j++) {
        info.m_costs[j] = static_cast<int32_t>(scale * cost + parentNode.m_costs[j]);
        scale = scale - 0.050000001f;
    }

    info.m_distance = distance;

    for (int32_t j = 0; j < CGTaxiMap::m_numNodes; j++) {
        uint32_t from = info.m_rec->m_id;
        uint32_t to = CGTaxiMap::m_nodes.m_data[j].m_rec->m_id;

        if (TaxiRouteExists(from, to)) {
            CGTaxiMap::RecursePath(j, index, s_paths.m_data[from - 1].m_data[to - 1]->m_cost);
        }
    }
}

// VERIFY: cb0f9231 manual
// OFFSET: 0x00592BB0
void CGTaxiMap::GeneratePaths() {
    uint32_t start = 0;

    for (int32_t i = 0; i < CGTaxiMap::m_numNodes; i++) {
        if (CGTaxiMap::m_nodes.m_data[i].m_rec->m_id == s_currentNode->m_id) {
            start = i;
            break;
        }
    }

    TaxiNodeInfo& node = CGTaxiMap::m_nodes.m_data[start];
    node.m_costs[0] = 0;
    node.m_costs[1] = 0;
    node.m_costs[2] = 0;
    node.m_costs[3] = 0;
    node.m_costs[4] = 0;
    node.m_numHops = 1;
    node.m_path = new (__FILE__, __LINE__) uint32_t[1];
    node.m_path[0] = s_currentNode->m_id;
    node.m_reachable = 1;
    node.m_distance = 0.0f;

    for (int32_t i = 0; i < CGTaxiMap::m_numNodes; i++) {
        uint32_t from = s_currentNode->m_id;
        uint32_t to = CGTaxiMap::m_nodes.m_data[i].m_rec->m_id;

        if (TaxiRouteExists(from, to)) {
            CGTaxiMap::RecursePath(i, start, s_paths.m_data[from - 1].m_data[to - 1]->m_cost);
        }
    }
}

// VERIFY: 6e322ad9 manual
// OFFSET: 0x005932E0
void CGTaxiMap::SetupMap(const WOWGUID& unit, int32_t currentNode, const uint64_t* nodeMask) {
    if (CGTaxiMap::m_unit == unit) {
        return;
    }

    if (CGTaxiMap::m_unit) {
        CGTaxiMap::CloseMap();
    }

    CGTaxiMap::m_numNodes = 0;

    for (uint32_t i = 0; i < s_nodeMask.Count(); i++) {
        s_nodeMask.m_data[i] = nodeMask[i];
    }

    TaxiNodesRec* current = g_taxiNodesDB.GetRecord(currentNode);
    s_currentNode = current;
    int32_t dungeonMapID;
    float x;
    float y;
    WorldMapTransformPosition(current->m_continentID, current->m_pos[0], current->m_pos[1], &s_mapID, &x, &y, &dungeonMapID);

    for (uint32_t i = 0; i < CGTaxiMap::m_maxNodeID; i++) {
        CGTaxiMap::m_routeLines.m_data[i].SetCount(0);
    }

    WorldMapContinentRec* continent = nullptr;
    int32_t index = 0;

    for (; index < g_worldMapContinentDB.m_numRecords; index++) {
        continent = g_worldMapContinentDB.GetRecordByIndex(index);

        if (continent->m_mapID == s_mapID) {
            break;
        }
    }

    if (index == g_worldMapContinentDB.m_numRecords) {
        return;
    }

    s_taxiMaxY = continent->m_taxiMax[1];
    s_taxiMinY = continent->m_taxiMin[1];
    s_taxiMinX = continent->m_taxiMin[0];
    s_taxiMaxX = continent->m_taxiMax[0];

    if (0.0f == continent->m_taxiMax[0] - continent->m_taxiMin[0] || 0.0f == continent->m_taxiMax[1] - continent->m_taxiMin[1]) {
        return;
    }

    int32_t nodeID = 1;

    for (uint32_t word = 0; word < s_nodeMask.Count(); word++) {
        for (uint32_t bit = 0; bit < 64; bit++) {
            uint64_t mask = 1ull << bit;

            if (!(mask & s_nodeMask.m_data[word])) {
                continue;
            }

            TaxiNodesRec* node = g_taxiNodesDB.GetRecord(nodeID + bit);

            if (!node) {
                continue;
            }

            MapRec* map = g_mapDB.GetRecord(node->m_continentID);
            int32_t mapID;
            WorldMapTransformPosition(node->m_continentID, node->m_pos[0], node->m_pos[1], &mapID, &x, &y, &dungeonMapID);

            if (mapID != s_mapID || (map && map->m_expansionID > ClientServices::Connection()->m_accountExpansion)) {
                s_nodeMask.m_data[word] &= ~mask;
                continue;
            }

            TaxiNodeInfo& info = CGTaxiMap::m_nodes.m_data[CGTaxiMap::m_numNodes];
            info.m_rec = node;
            WorldMapTransformPosition(node->m_continentID, node->m_pos[0], node->m_pos[1], &mapID, &x, &y, &dungeonMapID);
            info.m_x = (s_taxiMaxY - y) / (s_taxiMaxX - s_taxiMinX);
            WorldMapTransformPosition(node->m_continentID, node->m_pos[0], node->m_pos[1], &mapID, &x, &y, &dungeonMapID);
            info.m_y = (x - s_taxiMinX) / (s_taxiMaxY - s_taxiMinY);
            uint32_t from = s_currentNode->m_id;
            uint32_t to = node->m_id;
            info.m_noDirectPath = !from || from > CGTaxiMap::m_maxNodeID || !to || to > CGTaxiMap::m_maxNodeID || !s_paths.m_data[from - 1].m_data[to - 1];
            info.m_path = nullptr;

            for (uint32_t j = 0; j < 5; j++) {
                info.m_costs[j] = 999999;
            }

            info.m_numHops = 999999;
            info.m_reachable = 0;
            info.m_distance = 100000000.0f;
            CGTaxiMap::m_numNodes++;
        }

        nodeID += 64;
    }

    if (!CGTaxiMap::m_numNodes || !LoadMapTexture()) {
        CGTaxiMap::m_numNodes = 0;
        return;
    }

    CGTaxiMap::GeneratePaths();
    CGGameUI::SetInteractTarget(unit);
    CGTaxiMap::m_unit = unit;

    for (int32_t i = 0; i < CGTaxiMap::m_numNodes; i++) {
        CGTaxiMap::TaxiNodeSetCurrent(i);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TAXIMAP_OPENED, nullptr);
    CGTutorial::TriggerTutorial(34);

    if (CGTaxiMap::m_numNodes <= 1) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    if (player->GetClientStandState()) {
        player->ChangeStandState(0);
    }

    if (player->m_mountDisplayId > 0 && !(player->m_clientFlags & 0x10000000)) {
        player->Dismount();
    }

    if (player->IsCancelShapeshiftAtFlightMaster()) {
        player->CancelShapeshift();
    }
}

// VERIFY: d4891602 manual
// OFFSET: 0x005938D0
void CGTaxiMap::InitializeGame() {
    uint32_t maxNodeID = g_taxiNodesDB.m_maxID;
    uint32_t maskSize = (maxNodeID >> 6) + 1;

    CGTaxiMap::m_unit.m_guid = 0;
    CGTaxiMap::m_numNodes = 0;
    CGTaxiMap::m_maxNodeID = maxNodeID;
    CGTaxiMap::m_nodeMaskSize = maskSize;

    CGTaxiMap::m_nodes.SetCount(maxNodeID);
    s_nodeMask.SetCount(CGTaxiMap::m_nodeMaskSize);
    s_paths.SetCount(CGTaxiMap::m_maxNodeID);

    for (uint32_t i = 0; i < CGTaxiMap::m_maxNodeID; i++) {
        s_paths.m_data[i].SetCount(CGTaxiMap::m_maxNodeID);

        for (uint32_t j = 0; j < CGTaxiMap::m_maxNodeID; j++) {
            s_paths.m_data[i].m_data[j] = nullptr;
        }
    }

    s_unused_C0DC70.SetCount(CGTaxiMap::m_maxNodeID);
    CGTaxiMap::m_routeLines.SetCount(CGTaxiMap::m_maxNodeID);

    for (int32_t i = g_taxiPathDB.m_numRecords - 1; i >= 0; i--) {
        TaxiPathRec* path = i < 0 || i >= g_taxiPathDB.m_numRecords ? nullptr : &g_taxiPathDB.m_records[i];
        TaxiNodesRec* from = g_taxiNodesDB.GetRecord(path->m_fromTaxiNode);
        TaxiNodesRec* to = g_taxiNodesDB.GetRecord(path->m_toTaxiNode);

        if (from && to) {
            s_paths.m_data[from->m_id - 1].m_data[to->m_id - 1] = path;
        }
    }

    int32_t lastPathID = -1;
    TAXIPATHNODESCLIENT* pathNodes = nullptr;

    for (int32_t i = 0; i < g_taxiPathNodeDB.m_numRecords; i++) {
        TaxiPathNodeRec* node = i < 0 || i >= g_taxiPathNodeDB.m_numRecords ? nullptr : &g_taxiPathNodeDB.m_records[i];
        int32_t pathID = node->m_pathID;

        if (pathID == lastPathID) {
            pathNodes->m_numNodes++;
        } else {
            pathNodes = s_pathNodes.New(pathID, HASHKEY_NONE(), 0, 0);
            pathNodes->m_firstNode = node;
            pathNodes->m_numNodes = 1;
            lastPathID = node->m_pathID;
        }
    }
}

// VERIFY: 7b70e52d manual
// OFFSET: 0x00591A10
void CGTaxiMap::FreeRouteLines() {
    if (CGTaxiMap::m_mapTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(CGTaxiMap::m_mapTexture));
        CGTaxiMap::m_mapTexture = nullptr;
    }

    s_taxiLines.Clear();

    for (uint32_t i = 0; i < CGTaxiMap::m_maxNodeID; i++) {
        CGTaxiMap::m_routeLines.m_data[i].Clear();
    }

    CGTaxiMap::m_numNodes = 0;
}

// VERIFY: 7911d34a auto
// OFFSET: 0x005920D0
void CGTaxiMap::ShutdownGame() {
    s_pathNodes.InternalClear(0);
    CGTaxiMap::FreeRouteLines();
}

// VERIFY: 93b0ba02 auto
// OFFSET: 0x00591AC0
void CGTaxiMap::LeaveWorld() {
    CGTaxiMap::FreeRouteLines();
    CGTaxiMap::CloseMap();
}
