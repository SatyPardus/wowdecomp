#include "WoW/Source/WorldClient/WorldConstants.h"
#include "MinimapFrame.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/AreaPOIRec.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include <cmath>
#include <cfloat>
#include <cstring>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Gx/Gx.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/Ui/WorldMap.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/WMOAreaTableRec.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include <ctime>
#include <malloc.h>
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Gx/CGxMatrixStack.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/ShaderEffect/Types.h"

static const float MAX_POI_DISTANCE = 694.44446f;
static const float ICON_SIZE = 0.0125f;

// STATIC: 0x00ACEBAC
int32_t CGMinimapFrame::s_scaleChanged = 1;

// STATIC: 0x00ACEBB0
const char* CGMinimapFrame::s_objectTypeName = "Minimap";

// STATIC: 0x00ACEBB4
int32_t CGMinimapFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00BEBA44
float CGMinimapFrame::s_blipSize4;

// STATIC: 0x00BEBA48
float CGMinimapFrame::s_blipHalfSize3;

// STATIC: 0x00BEBA4C
float CGMinimapFrame::s_blipSize3;

// STATIC: 0x00BEBA50
float CGMinimapFrame::s_blipHalfSize2;

// STATIC: 0x00BEBA54
float CGMinimapFrame::s_blipSize2;

// STATIC: 0x00BEBA58
float CGMinimapFrame::s_blipHalfSize1;

// STATIC: 0x00BEBA5C
float CGMinimapFrame::s_blipSize1;

// STATIC: 0x00BEBA60
int32_t CGMinimapFrame::s_objectType;

// STATIC: 0x00BEBA6C
int32_t CGMinimapFrame::s_rotateMinimap;

// STATIC: 0x00BEBA70
float CGMinimapFrame::s_facing;

// STATIC: 0x00BEBA8C
C2Vector CGMinimapFrame::s_pingPosition;

// STATIC: 0x00BEBA88
CGMinimapFrame* CGMinimapFrame::s_minimapFrame;

// STATIC: 0x00BEBA9C
CDataAllocator CGMinimapFrame::s_minimapHeap(sizeof(CGMinimapFrame), 1);

// STATIC: 0x00BF7FC8
C3Vector CGMinimapFrame::s_blipQuads[3][4];

// STATIC: 0x00BEBA64
const TrackingTypeInfo* CGMinimapFrame::s_trackingInfo;

// STATIC: 0x00BEBA68
int32_t CGMinimapFrame::s_trackingSpell;

// STATIC: 0x00BEBA04
static CTexture* s_vehicleTextures[7];

// STATIC: 0x00BEBA20
static CTexture* s_groupArrowTexture;

// STATIC: 0x00BEBA24
CTexture* s_staticPOIArrowTexture;

// STATIC: 0x00BEBA28
CTexture* s_corpseArrowTexture;

// STATIC: 0x00BEBA2C
CTexture* s_poiArrowTexture;

// STATIC: 0x00BEBA30
CTexture* s_maskTexture;

// STATIC: 0x00BEBA34
CTexture* s_classBlipTexture;

// STATIC: 0x00BEBA38
CTexture* s_blipTexture;

// STATIC: 0x00BEBA3C
CTexture* s_iconTexture;

// STATIC: 0x00BF6148
static C2Vector s_iconTexCoords[196][4];

// STATIC: 0x00BF79C8
static C2Vector s_classBlipTexCoords[16][4];

// STATIC: 0x00BF7BC8
static C2Vector s_blipTexCoords[32][4];

// STATIC: 0x00BEBA40
static int32_t s_inWorld;

enum MINIMAP_OBJECT_TYPES : uint32_t {
    // Current native subdivisions of the five-type reference domain.
    MINIMAP_RAID = 0,
    MINIMAP_PARTY = 1,
    MINIMAP_PLAYER_HOSTILE = 2,
    MINIMAP_PLAYER_OTHER_FACTION = 3,
    MINIMAP_PLAYER_SAME_FACTION = 4,
    MINIMAP_MONSTER_HOSTILE = 5,
    MINIMAP_MONSTER_NEUTRAL = 6,
    MINIMAP_MONSTER_FRIENDLY = 7,
    MINIMAP_OBJECT = 8,
    MINIMAP_QUEST_AVAILABLE = 9,
    MINIMAP_QUEST_COMPLETE = 10,
    MINIMAP_QUEST_REPEATABLE = 11,
    MINIMAP_UNUSED_12 = 12, // No producer in the current native paths.
    MINIMAP_FLIGHTMASTER = 13,
    MINIMAP_UNUSED_14 = 14, // No producer in the current native paths.
    MINIMAP_PET = 15,
    MINIMAP_VEHICLE_GROUND_UNOCCUPIED = 16,
    MINIMAP_VEHICLE_GROUND_OCCUPIED = 17,
    MINIMAP_VEHICLE_AIR_UNOCCUPIED = 18,
    MINIMAP_VEHICLE_AIR_OCCUPIED = 19,
    MINIMAP_VEHICLE_UNUSED = 20, // No producer in the current native paths.
    MINIMAP_VEHICLE_AIR_HORDE = 21,
    MINIMAP_VEHICLE_AIR_ALLIANCE = 22,
    MINIMAP_NUM_TYPES = 23,
};

// STATIC: 0x00BEB9A8
static uint32_t s_hoverObjects[MINIMAP_NUM_TYPES];

// STATIC: 0x00BEBA74
static uint32_t s_hoverPOIs;

// STATIC: 0x00BEBA78
static uint32_t s_hoverPOIDirections;

// STATIC: 0x00BEBA7C
static uint32_t s_hoverParty;

// STATIC: 0x00BEBA80
static uint64_t s_hoverRaid;

// STATIC: 0x00A11C50
const TrackingTypeInfo CGMinimapFrame::s_trackingTypes[15] = {
    { 1, 0x1000, "MINIMAP_TRACKING_REPAIR", "Repair", 0x0 },
    { 1, 0x200, "MINIMAP_TRACKING_VENDOR_FOOD", "Food", 0x0 },
    { 1, 0x400, "MINIMAP_TRACKING_VENDOR_POISON", "Poisons", 0x10 },
    { 1, 0x100, "MINIMAP_TRACKING_VENDOR_AMMO", "Ammunition", 0x1A },
    { 1, 0x800, "MINIMAP_TRACKING_VENDOR_REAGENT", "Reagents", 0x0 },
    { 1, 0x10000, "MINIMAP_TRACKING_INNKEEPER", "Innkeeper", 0x0 },
    { 1, 0x2000, "MINIMAP_TRACKING_FLIGHTMASTER", "FlightMaster", 0x0 },
    { 1, 0x400000, "MINIMAP_TRACKING_STABLEMASTER", "StableMaster", 0x8 },
    { 1, 0x100000, "MINIMAP_TRACKING_BATTLEMASTER", "BattleMaster", 0x0 },
    { 1, 0x20, "MINIMAP_TRACKING_TRAINER_CLASS", "Class", 0x0 },
    { 1, 0x40, "MINIMAP_TRACKING_TRAINER_PROFESSION", "Profession", 0x0 },
    { 1, 0x200000, "MINIMAP_TRACKING_AUCTIONEER", "Auctioneer", 0x0 },
    { 1, 0x20000, "MINIMAP_TRACKING_BANKER", "Banker", 0x0 },
    { 2, 0x13, "MINIMAP_TRACKING_MAILBOX", "Mailbox", 0x0 },
    { 3, 0x0, "MINIMAP_TRACKING_TRIVIAL_QUESTS", "TrivialQuests", 0x0 },
};

// STATIC: 0x00A41E04
static const uint32_t s_outdoorZoomTiles[6] = { 14, 12, 10, 8, 6, 4 };

// STATIC: 0x00A41E1C
static const float s_indoorZoomRadius[6] = { 150.0f, 120.0f, 90.0f, 60.0f, 40.0f, 25.0f };

// STATIC: 0x00AF4E48
static int32_t s_mapId = -1;

// STATIC: 0x00AF4E4C
static int32_t s_outdoorZoom = 3;

// STATIC: 0x00AF4E50
static int32_t s_indoorZoom = 3;

// STATIC: 0x00AF4E58
static const char* s_minimapTexturePath = "Textures\\Minimap";

// STATIC: 0x00D39210
static float s_closestPOIFacing[3];

// STATIC: 0x00D3921C
static int32_t s_closestPOIs[3];

// STATIC: 0x00D39228
static int32_t s_poisChanged;

// STATIC: 0x00D39230
static char s_dynamicPOINames[4][64];

// STATIC: 0x00D39434
static uint8_t s_isIndoors;

// STATIC: 0x00D39444
static uint32_t s_numPOIs;

// STATIC: 0x00D39448
static uint32_t s_numClosestPOIs;

// STATIC: 0x00D3944C
static int32_t s_closestPOIsChanged;

// STATIC: 0x00D39454
static C3Vector s_playerPosition;

// STATIC: 0x00D39490
static AreaPOIRec s_dynamicPOIs[4];

// STATIC: 0x00D395F0
static TSFixedArray<AreaPOIRec*> s_pois;

// STATIC: 0x00D395FC
static TSFixedArray<int32_t> s_poiVisible;

// STATIC: 0x00D39608
static TSFixedArray<int32_t> s_poiOnEdge;

// STATIC: 0x00D39614
static TSGrowableArray<AreaPOIRec*> s_visiblePOIs;

// STATIC: 0x00D39624
static TSHashTable<MINIMAPMD5NAME, HASHKEY_STRI> s_md5Names;

// STATIC: 0x00BEBAB0
static MINIMAPQUAD s_quads[4] = {
    { { 0.0f, 0.0f, 0.5f, 0.5f }, C2Vector(0.0f, 0.0f) },
    { { 0.5f, 0.0f, 1.0f, 0.5f }, C2Vector(1.0f, 0.0f) },
    { { 0.5f, 0.5f, 1.0f, 1.0f }, C2Vector(1.0f, 1.0f) },
    { { 0.0f, 0.5f, 0.5f, 1.0f }, C2Vector(0.0f, 1.0f) },
};

// STATIC: 0x00BEBB10
static MINIMAPTILE s_tiles[256];

// STATIC: 0x00BF5F10
static MINIMAPINDOOR s_indoor;

// STATIC: 0x00BF5FB8
static TSGrowableArray<POIINFO> s_poiInfo;

// STATIC: 0x00BF8058
static TSGrowableArray<C3Vector> s_blipVertices;

// STATIC: 0x00BF8068
static TSGrowableArray<C2Vector> s_blipTexCoords2;

// STATIC: 0x00BF8078
static TSGrowableArray<uint16_t> s_blipIndices;

// STATIC: 0x00BF8098
static TSGrowableArray<OBJINFO> s_objInfo[MINIMAP_NUM_TYPES];

// STATIC: 0x00BF5FC8
static C2Vector s_poiArrowPositions[3];

// STATIC: 0x00BF5FE0
static C2Vector s_partyArrowPositions[5];

// STATIC: 0x00BF6008
static C2Vector s_raidArrowPositions[40];

// STATIC: 0x00BF8088
static TSGrowableArray<POIDIRECTIONDATA> s_poiDirections;

// STATIC: 0x00A11F7C
static uint16_t s_arrowIndices[4] = { 0, 2, 1, 3 };

// STATIC: 0x00ACEC64
static float s_arrowRotation = 2.3561945f;

// STATIC: 0x00BF8208
static MEMBERINFO s_partyMembers[5];

// STATIC: 0x00BF8398
static MEMBERINFO s_raidMembers[40];

// STATIC: 0x00ACECE4
static uint16_t s_quadIndices[4] = { 0, 1, 2, 3 };

// STATIC: 0x00ACECEC
static uint16_t s_insideQuadIndices[4] = { 0, 1, 3, 2 };

// STATIC: 0x00ACECF4
static uint16_t s_insideIndices[4] = { 0, 1, 3, 2 };

// STATIC: 0x00ACECFC
static float s_indoorTexScale = 0.33333334f;

// STATIC: 0x00D3922C
static uint32_t s_updateFlags;

// STATIC: 0x00D39330
static char s_mapObjName[260];

// STATIC: 0x00D39438
static int32_t s_mapObjGroup;

// STATIC: 0x00D3943C
static int32_t s_mapObjId;

// STATIC: 0x00D39440
static uint32_t s_worldMapRefreshTime;

// STATIC: 0x00D39450
static uint8_t s_mapObjIndoorArea;

// STATIC: 0x00D39460
static CAaBox s_indoorBounds;

// STATIC: 0x00D3947C
static int32_t s_tileMinX = -1;

// STATIC: 0x00D39480
static int32_t s_tileMinY = -1;

// STATIC: 0x00D39484
static int32_t s_tileMaxX = -1;

// STATIC: 0x00D39488
static int32_t s_tileMaxY = -1;

// STATIC: 0x00D39518
static C2Vector s_worldMapRefreshPos;

// STATIC: 0x00AF4E54
static int32_t s_mapObjAreaGroup = -1;

// STATIC: 0x00AF4E5C
static const char* s_minimapTileFormat = "%s\\map%d_%02d.blp";

// STATIC: 0x00AF4E60
static const char* s_minimapQuadFormat = "%s_%03d_%02d_%02d.blp";

// STATIC: 0x00AF4E78
static C2Vector s_halfTile(266.66666f, 266.66666f);

// STATIC: 0x00AF4E84
static float s_mapOrigin = 17066.666f;

// STATIC: 0x00AF4E88
static C44Matrix s_mapObjMatrix;

// STATIC: 0x00AF4EC8
static C44Matrix s_mapObjInvMatrix;

// STATIC: 0x00AF4F64
static float s_mapObjRotation = 90.0f;

// STATIC: 0x00AF4F88
static C2Vector s_tileSize(1066.6666f, 1066.6666f);

// STATIC: 0x00A41EF0
static const int32_t s_tileOffsets[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

// INLINED
void CGMinimapFrame::operator delete(void* ptr) {
    if (ptr) {
        CGMinimapFrame::s_minimapHeap.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CGMinimapFrame::GetObjectType() {
    if (!CGMinimapFrame::s_objectType) {
        CGMinimapFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CGMinimapFrame::s_objectType;
}

// VERIFY: 185b6554 auto
// OFFSET: 0x00514230
CSimpleFrame* CGMinimapFrame::Create(CSimpleFrame* parent) {
    void* data = CGMinimapFrame::s_minimapHeap.GetData(0, typeid(CGMinimapFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CGMinimapFrame(parent) : nullptr;
}

// VERIFY: 9c7a5302 auto
// OFFSET: 0x0057BC90
bool CGMinimapFrame::UpdateScale(bool force) {
    if (!CSimpleFrame::UpdateScale(force)) {
        return false;
    }

    CGMinimapFrame::s_scaleChanged = 1;
    return true;
}

// VERIFY: e963994e auto
// OFFSET: 0x0057BCC0
void CGMinimapFrame::UpdateRotation(float facing) {
    this->m_facing = facing;
    CGMinimapFrame::s_facing = facing;

    if (this->m_playerTexture) {
        if (g_rotateMinimapCVar->m_intValue) {
            facing = 0.0f;
        }

        this->m_playerTexture->SetRotation(facing, 0.5f, 0.5f);
    }
}

// VERIFY: 2d4505b3 auto
// OFFSET: 0x0057BE80
const char* CGMinimapFrame::GetObjectTypeName() {
    return CGMinimapFrame::s_objectTypeName;
}

// VERIFY: 54d9ac95 auto
// OFFSET: 0x0057BE90
int32_t CGMinimapFrame::GetScriptMetaTable() {
    return CGMinimapFrame::s_metatable;
}

// VERIFY: b0c85a44 auto
// OFFSET: 0x0057BEA0
void CGMinimapFrame::PostLoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::PostLoadXML(node, status);

    const char* playerTexture = node->GetAttributeByName("minimapPlayerTexture");

    if (!playerTexture) {
        playerTexture = "Interface\\Minimap\\MinimapArrow.tga";
    }

    if (!this->m_playerTexture->SetTexture(playerTexture, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        status->Add(STATUS_WARNING, "Invalid minimapPlayerTexture in Minimap.xml");
    }

    CLayoutFrame* compass = this->GetLayoutFrameByName("MinimapCompassTexture");
    this->m_compassTexture = compass ? static_cast<CScriptRegion*>(compass) : nullptr;
}

// VERIFY: c9d382fd auto
// OFFSET: 0x0057C540
bool CGMinimapFrame::IsA(int32_t type) {
    return type == CGMinimapFrame::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: af38ff6d auto
// OFFSET: 0x0057C580
bool CGMinimapFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CGMinimapFrame::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 812aa182 manual
// OFFSET: 0x0057C6A0
void CGMinimapFrame::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    CGObject_C* target = ClntObjMgrObjectPtr(camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        this->UpdateRotation(target->GetFacing());
    } else {
        this->UpdateRotation(camera->m_yaw);
    }
}

// VERIFY: 44ce878d auto
// OFFSET: 0x0057D580
void CGMinimapFrame::OnLayerCursorExit(int32_t motion, int32_t a3) {
    CGGameUI::m_gameTooltip->Hide();
    CSimpleFrame::OnLayerCursorExit(motion, a3);
}

// VERIFY: b41af30f auto
// OFFSET: 0x0057DCA0
CGMinimapFrame::CGMinimapFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_facing = -10000.0f;
    this->m_lastUpdateTime = 0;
    CGMinimapFrame::s_rotateMinimap = g_rotateMinimapCVar->m_intValue;
    this->m_playerTexture = nullptr;

    if (!CGMinimapFrame::s_minimapFrame) {
        CGMinimapFrame::s_minimapFrame = this;
    }

    this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);

    float scale = NDCToDDCHeight(1.0f) * 1.6666666f;
    CGMinimapFrame::s_blipSize1 = ICON_SIZE * scale;
    CGMinimapFrame::s_blipHalfSize1 = CGMinimapFrame::s_blipSize1 * 0.5f;
    CGMinimapFrame::s_blipSize2 = scale * 0.0375f;
    CGMinimapFrame::s_blipHalfSize2 = CGMinimapFrame::s_blipSize2 * 0.5f;
    CGMinimapFrame::s_blipSize3 = scale * 0.045f;
    CGMinimapFrame::s_blipHalfSize3 = 0.5f * CGMinimapFrame::s_blipSize3;
    CGMinimapFrame::s_blipSize4 = scale * 0.055f;

    float halfSizes[3] = { CGMinimapFrame::s_blipHalfSize1, CGMinimapFrame::s_blipHalfSize2, CGMinimapFrame::s_blipHalfSize3 };

    for (uint32_t i = 0; i < 3; i++) {
        float half = halfSizes[i];
        CGMinimapFrame::s_blipQuads[i][0] = C3Vector(-half, -half, 0.0f);
        CGMinimapFrame::s_blipQuads[i][1] = C3Vector(half, -half, 0.0f);
        CGMinimapFrame::s_blipQuads[i][2] = C3Vector(-half, half, 0.0f);
        CGMinimapFrame::s_blipQuads[i][3] = C3Vector(half, half, 0.0f);
    }

    this->m_blipFrameLevel = this->m_level + 3;

    void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(this, 2, 1) : nullptr;
    this->m_playerTexture = texture;
    texture->SetPoint(FRAMEPOINT_CENTER, this, FRAMEPOINT_CENTER, 0.001f, 0.001f, 1);
    this->m_compassTexture = nullptr;
}

// VERIFY: eb33fe04 auto
// OFFSET: [0x0057C250, 0x0057DFD0]
CGMinimapFrame::~CGMinimapFrame() {
    if (CGMinimapFrame::s_minimapFrame == this) {
        CGMinimapFrame::s_minimapFrame = nullptr;
    }
}

// VERIFY: 6329f6cb auto
// OFFSET: 0x005832A0
void CGMinimapFrame::RenderCallback(void* param) {
    if (param) {
        static_cast<CGMinimapFrame*>(param)->Render();
    }
}

// VERIFY: a761a756 auto
// OFFSET: 0x005832C0
void CGMinimapFrame::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    CSimpleFrame::OnFrameRender(batch, layer);

    if (!layer) {
        batch->QueueCallback(CGMinimapFrame::RenderCallback, this);
    }
}

// VERIFY: 855d28c7 auto
// OFFSET: 0x005831C0
void CGMinimapFrame::EnterWorld() {
    s_inWorld = 1;
    MinimapInitialize(CGPlayer_C::GetNewContinentID());
    s_indoor.m_radius = 10.0f;
    EGxTexFormat format = GxTex_Unknown;

    if (g_theGxDevicePtr->Caps().m_rttFormat[GxTex_Argb8888]) {
        format = GxTex_Argb8888;
    } else if (g_theGxDevicePtr->Caps().m_rttFormat[GxTex_Abgr8888]) {
        format = GxTex_Abgr8888;
    } else if (g_theGxDevicePtr->Caps().m_rttFormat[GxTex_Rgb565]) {
        format = GxTex_Rgb565;
    }

    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 1, 1, 0, 0, 0);
    s_indoor.m_texture = TextureCreate(256, 256, format, format, flags, nullptr, CGMinimapFrame::MinimapTextureCallback, "MinimapRenderTarget", 0);
    memset(s_hoverObjects, 0, sizeof(s_hoverObjects));
}

// VERIFY: 7d4e4155 manual
// OFFSET: 0x005832F0
void CGMinimapFrame::InitializeGame() {
    CStatus status;

    s_maskTexture = TextureCreate("Textures\\MinimapMask", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_iconTexture = TextureCreate("Interface\\Minimap\\POIIcons", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_blipTexture = TextureCreate("Interface\\Minimap\\ObjectIcons", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_classBlipTexture = TextureCreate("Interface\\Minimap\\PartyRaidBlips", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[0] = TextureCreate("Interface\\Minimap\\Vehicle-Ground-Unoccupied", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[1] = TextureCreate("Interface\\Minimap\\Vehicle-Ground-Occupied", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[2] = TextureCreate("Interface\\Minimap\\Vehicle-Air-Unoccupied", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[3] = TextureCreate("Interface\\Minimap\\Vehicle-Air-Occupied", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[4] = TextureCreate("Interface\\Minimap\\Vehicle-Ground-Unoccupied", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[5] = TextureCreate("Interface\\Minimap\\Vehicle-Air-Horde", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_vehicleTextures[6] = TextureCreate("Interface\\Minimap\\Vehicle-Air-Alliance", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_staticPOIArrowTexture = TextureCreate("Interface\\Minimap\\Rotating-MinimapArrow", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_poiArrowTexture = TextureCreate("Interface\\Minimap\\Rotating-MinimapGuideArrow", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_corpseArrowTexture = TextureCreate("Interface\\Minimap\\Rotating-MinimapCorpseArrow", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    s_groupArrowTexture = TextureCreate("Interface\\Minimap\\Rotating-MinimapGroupArrow", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);

    uint32_t width = 256;
    uint32_t height = 256;
    TextureGetDimensions(s_iconTexture, &width, &height, 1);

    uint32_t columns = CMath::sqrt_(196);
    float texel = 1.0f / width;
    float size = texel * 18.0f;

    for (uint32_t i = 0; i < 196; i++) {
        float left = (i % columns) * size + texel;
        float right = left + size - texel;
        float top = (i / columns) * size + texel;
        float bottom = top + size - texel;

        s_iconTexCoords[i][0].x = left;
        s_iconTexCoords[i][0].y = bottom;
        s_iconTexCoords[i][1].x = right;
        s_iconTexCoords[i][1].y = bottom;
        s_iconTexCoords[i][2].x = left;
        s_iconTexCoords[i][2].y = top;
        s_iconTexCoords[i][3].x = right;
        s_iconTexCoords[i][3].y = top;
    }

    for (uint32_t i = 0; i < 16; i++) {
        float left = (i & 7) * 0.125f;
        float bottom = ((i >> 3) + 1) * 0.5f;
        float right = ((i & 7) + 1) * 0.125f;
        float top = (i >> 3) * 0.5f;

        s_classBlipTexCoords[i][0].x = left;
        s_classBlipTexCoords[i][0].y = bottom;
        s_classBlipTexCoords[i][1].x = right;
        s_classBlipTexCoords[i][1].y = bottom;
        s_classBlipTexCoords[i][2].x = left;
        s_classBlipTexCoords[i][2].y = top;
        s_classBlipTexCoords[i][3].x = right;
        s_classBlipTexCoords[i][3].y = top;
    }

    for (uint32_t i = 0; i < 32; i++) {
        float left = (i & 7) * 0.125f;
        float bottom = ((i >> 3) + 1) * 0.25f;
        float right = ((i & 7) + 1) * 0.125f;
        float top = (i >> 3) * 0.25f;

        s_blipTexCoords[i][0].x = left;
        s_blipTexCoords[i][0].y = bottom;
        s_blipTexCoords[i][1].x = right;
        s_blipTexCoords[i][1].y = bottom;
        s_blipTexCoords[i][2].x = left;
        s_blipTexCoords[i][2].y = top;
        s_blipTexCoords[i][3].x = right;
        s_blipTexCoords[i][3].y = top;
    }

    CGMinimapFrame::s_trackingSpell = 0;
    CGMinimapFrame::s_trackingInfo = nullptr;
}

// VERIFY: 78b3bb60 manual
// OFFSET: 0x0057E980
int32_t CGMinimapFrame::CountAvailableTrackingTypes() {
    int32_t count = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t classMask = player ? 1 << player->m_unit->m_unitFieldBytes0[1] : 0;

    for (uint32_t i = 0; i < sizeof(CGMinimapFrame::s_trackingTypes) / sizeof(CGMinimapFrame::s_trackingTypes[0]); i++) {
        uint32_t mask = CGMinimapFrame::s_trackingTypes[i].m_classMask;

        if (!mask || (mask & classMask)) {
            count++;
        }
    }

    return count;
}

// VERIFY: 9e30c5ed manual
// OFFSET: 0x0057EB00
const TrackingTypeInfo* CGMinimapFrame::GetTrackingTypeByIndex(int32_t index) {
    int32_t count = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t classMask = player ? 1 << player->m_unit->m_unitFieldBytes0[1] : 0;

    for (uint32_t i = 0; i < sizeof(CGMinimapFrame::s_trackingTypes) / sizeof(CGMinimapFrame::s_trackingTypes[0]); i++) {
        uint32_t mask = CGMinimapFrame::s_trackingTypes[i].m_classMask;

        if (mask && !(mask & classMask)) {
            continue;
        }

        if (count == index) {
            return &CGMinimapFrame::s_trackingTypes[i];
        }

        count++;
    }

    return nullptr;
}

// VERIFY: aba21c39 auto
// OFFSET: 0x007F3BE0
AreaPOIRec* MinimapGetGossipPoi() {
    return &s_dynamicPOIs[1];
}

// VERIFY: 23614ed9 manual
// OFFSET: 0x0057EA30
void CGMinimapFrame::SetTrackingSpell(int32_t spellId) {
    CGMinimapFrame::s_trackingSpell = spellId;

    if (spellId) {
        CGMinimapFrame::LoadTrackingInfo(0);
    } else {
        Script_SendUniqueSignal(SCRIPT_EVENT_MINIMAP_UPDATE_TRACKING);
    }
}

// VERIFY: 08401ea1 auto
// OFFSET: 0x007F3B90
float MinimapGetViewRadius() {
    if (s_isIndoors) {
        return s_indoorZoomRadius[s_indoorZoom];
    }

    return s_outdoorZoomTiles[s_outdoorZoom] * 0.5f * 33.333332f;
}

// VERIFY: 10a7b5c4 manual
// OFFSET: 0x007F44A0
void UpdatePointsOfInterest() {
    if (s_mapId == -1) {
        return;
    }

    s_visiblePOIs.SetCount(0);
    float viewRadius = MinimapGetViewRadius();

    for (uint32_t i = 0; i < s_numPOIs; i++) {
        AreaPOIRec* poi = s_pois.m_data[i];

        if ((poi->m_pos[0] == 0.0f && poi->m_pos[1] == 0.0f) || (poi->m_worldStateID && !GetWorldState(poi->m_worldStateID))) {
            if (!s_poiVisible.m_data[i] && !s_poiOnEdge.m_data[i]) {
                continue;
            }

            s_poiVisible.m_data[i] = 0;
            s_poiOnEdge.m_data[i] = 0;
            s_poisChanged = 1;
            continue;
        }

        float dy = s_playerPosition.y - poi->m_pos[1];
        float dx = s_playerPosition.x - poi->m_pos[0];

        if (sqrt(dx * dx + dy * dy) / viewRadius > 0.8f) {
            if (!s_poiVisible.m_data[i] && s_poiOnEdge.m_data[i]) {
                continue;
            }

            s_poiVisible.m_data[i] = 0;
            s_poiOnEdge.m_data[i] = 1;
            s_poisChanged = 1;
            continue;
        }

        if (!(poi->m_flags & 2)) {
            if (!s_poiVisible.m_data[i] && !s_poiOnEdge.m_data[i]) {
                continue;
            }

            s_poiVisible.m_data[i] = 0;
            s_poiOnEdge.m_data[i] = 0;
            s_poisChanged = 1;
            continue;
        }

        *s_visiblePOIs.New() = poi;

        if (s_poiVisible.m_data[i] && !s_poiOnEdge.m_data[i]) {
            continue;
        }

        s_poiVisible.m_data[i] = 1;
        s_poiOnEdge.m_data[i] = 0;
        s_poisChanged = 1;
    }

    int32_t worst = -1;
    uint32_t count = 0;
    float distances[3] = { MAX_POI_DISTANCE, MAX_POI_DISTANCE, MAX_POI_DISTANCE };
    int32_t indices[3] = { -1, -1, -1 };
    int32_t importance[3];

    for (uint32_t i = 0; i < s_numPOIs; i++) {
        if (!s_poiOnEdge.m_data[i]) {
            continue;
        }

        AreaPOIRec* poi = s_pois.m_data[i];
        float dy = poi->m_pos[1] - s_playerPosition.y;
        float dx = poi->m_pos[0] - s_playerPosition.x;
        float distance = sqrt(dx * dx + dy * dy);

        if (distance > MAX_POI_DISTANCE && poi != &s_dynamicPOIs[2]) {
            continue;
        }

        if (worst == -1) {
            distances[count] = distance;
            indices[count] = i;
            importance[count] = poi->m_importance;
            count++;
        } else if (poi->m_importance < importance[worst] || (poi->m_importance == importance[worst] && distance < distances[worst])) {
            distances[worst] = distance;
            indices[worst] = i;
            importance[worst] = poi->m_importance;
        }

        if (count == 3) {
            worst = 0;

            for (int32_t j = 1; j < 3; j++) {
                if (importance[j] > importance[worst] || (importance[j] == importance[worst] && distances[worst] < distances[j])) {
                    worst = j;
                }
            }
        }
    }

    if (s_numClosestPOIs != count) {
        s_numClosestPOIs = count;
        s_closestPOIsChanged = 1;
    } else {
        for (uint32_t j = 0; j < count; j++) {
            if (indices[j] != s_closestPOIs[j]) {
                s_closestPOIsChanged = 1;
                break;
            }
        }
    }

    for (uint32_t j = 0; j < count; j++) {
        s_closestPOIs[j] = indices[j];
        AreaPOIRec* poi = s_pois.m_data[indices[j]];
        C3Vector position(poi->m_pos[0], poi->m_pos[1], s_playerPosition.z);
        s_closestPOIFacing[j] = CalculateFacingTo(&s_playerPosition, &position);
    }
}

// VERIFY: 52931454 auto
// OFFSET: 0x007F4930
void DisplayQuestPoint(float x, float y, int32_t importance, const char* name) {
    s_dynamicPOIs[0].m_pos[0] = x;
    s_dynamicPOIs[0].m_pos[1] = y;
    s_dynamicPOIs[0].m_flags = 0;
    s_dynamicPOIs[0].m_continentID = s_mapId;
    s_dynamicPOIs[0].m_icon[0] = 6;
    s_dynamicPOIs[0].m_importance = importance;
    SStrCopy(s_dynamicPOINames[0], name, sizeof(s_dynamicPOINames[0]));
    UpdatePointsOfInterest();
    s_closestPOIsChanged = 1;
}

// VERIFY: e9f5e6ef auto
// OFFSET: 0x007F4870
void MinimapSetDynamicPOI(int32_t index, int32_t flags, float x, float y, int32_t icon, int32_t importance, const char* name) {
    s_dynamicPOIs[index].m_flags = flags;
    s_dynamicPOIs[index].m_pos[0] = x;
    s_dynamicPOIs[index].m_icon[0] = icon;
    s_dynamicPOIs[index].m_continentID = s_mapId;
    s_dynamicPOIs[index].m_pos[1] = y;
    s_dynamicPOIs[index].m_importance = importance;
    SStrCopy(s_dynamicPOINames[index], name, sizeof(s_dynamicPOINames[index]));

    if (index == 1) {
        s_worldMapRefreshTime = static_cast<uint32_t>(time(nullptr)) + 480;

        if (CGWorldMap::m_currentOrphanZone == -1) {
            CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentZone, CGWorldMap::m_currentOrphanZone);
        } else {
            CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentOrphanZone, CGWorldMap::m_currentDungeonMap);
        }
    }

    UpdatePointsOfInterest();
    s_closestPOIsChanged = 1;
}

// VERIFY: 3a8b6454 auto
// OFFSET: 0x007F4990
void MinimapSetCorpsePOI(float x, float y) {
    const char* name = FrameScript_GetText("CORPSE_RED", -1, GENDER_NOT_APPLICABLE);
    s_dynamicPOIs[2].m_pos[0] = x;
    s_dynamicPOIs[2].m_pos[1] = y;
    s_dynamicPOIs[2].m_flags = 2;
    s_dynamicPOIs[2].m_continentID = s_mapId;
    s_dynamicPOIs[2].m_icon[0] = 8;
    s_dynamicPOIs[2].m_importance = -1;
    SStrCopy(s_dynamicPOINames[2], name, sizeof(s_dynamicPOINames[2]));
    UpdatePointsOfInterest();
    s_closestPOIsChanged = 1;
}

// VERIFY: 7ee85ac8 auto
// OFFSET: 0x007F4A00
void MinimapSetSpiritHealerPOI(float x, float y) {
    const char* name = FrameScript_GetText("SPIRIT_HEALER_RELEASE_RED", -1, GENDER_NOT_APPLICABLE);
    s_dynamicPOIs[3].m_pos[0] = x;
    s_dynamicPOIs[3].m_pos[1] = y;
    s_dynamicPOIs[3].m_flags = 2;
    s_dynamicPOIs[3].m_continentID = s_mapId;
    s_dynamicPOIs[3].m_icon[0] = 8;
    s_dynamicPOIs[3].m_importance = -1;
    SStrCopy(s_dynamicPOINames[3], name, sizeof(s_dynamicPOINames[3]));
    UpdatePointsOfInterest();
    s_closestPOIsChanged = 1;
}

// VERIFY: 0eda32e5 manual
// OFFSET: 0x007F6540
static void LoadMD5Names() {
    char path[260];
    SStrPrintf(path, sizeof(path), "%s\\md5translate.trs", s_minimapTexturePath);
    void* data;

    if (!SFile::LoadFile(path, &data, nullptr, 1, nullptr)) {
        return;
    }

    const char* cursor = static_cast<const char*>(data);
    char line[260];
    memset(line, 0, sizeof(line));

    do {
        SStrTokenize(&cursor, line, sizeof(line), "\r\n", nullptr);

        if (!*cursor || !line[0]) {
            break;
        }

        if (!SStrCmp(line, "dir:", SStrLen("dir:"))) {
            continue;
        }

        char* tab = SStrChr(line, '\t');

        if (!tab) {
            continue;
        }

        *tab = '\0';
        MINIMAPMD5NAME* entry = s_md5Names.Ptr(line);

        if (!entry) {
            entry = s_md5Names.New(line, 0, 0);
        }

        SStrCopy(entry->m_md5Name, tab + 1, sizeof(entry->m_md5Name));
    } while (line[0] && *cursor);

    SFile::Unload(data);
}

// VERIFY: 446a7368 auto
// OFFSET: 0x007F4E50
void MinimapShutdown() {
    s_playerPosition.x = FLT_MAX;
    s_playerPosition.z = FLT_MAX;
    s_playerPosition.y = FLT_MAX;
    s_tileMaxY = -1;
    s_tileMinY = -1;
    s_mapId = -1;
    s_tileMaxX = -1;
    s_tileMinX = -1;
    s_mapObjId = 0;
    s_mapObjGroup = 0;
    s_mapObjAreaGroup = -1;
    s_updateFlags = 0;
    s_md5Names.InternalClear(0);
}

// VERIFY: 885c45ba manual
// OFFSET: 0x007F6730
void MinimapInitialize(int32_t mapId) {
    s_numPOIs = 0;

    for (int32_t pass = 0; pass < 2; pass++) {
        if (pass > 0) {
            s_pois.SetCount(s_numPOIs);
            s_poiVisible.SetCount(s_numPOIs);
            s_poiOnEdge.SetCount(s_numPOIs);
            memset(s_poiVisible.m_data, 0, sizeof(int32_t) * s_poiVisible.m_count);
            memset(s_poiOnEdge.m_data, 0, sizeof(int32_t) * s_poiOnEdge.m_count);
        }

        uint32_t count = 0;

        for (int32_t i = g_areaPOIDB.m_numRecords - 1; i >= 0; i--) {
            AreaPOIRec* rec = g_areaPOIDB.GetRecordByIndex(i);

            if (rec->m_continentID == mapId && (rec->m_flags & 1)) {
                if (pass > 0) {
                    s_pois.m_data[count] = rec;
                }

                count++;
            }
        }

        s_numPOIs = count + 4;
    }

    for (uint32_t i = 0; i < 4; i++) {
        s_pois.m_data[s_numPOIs - 4 + i] = &s_dynamicPOIs[i];
        memset(&s_dynamicPOIs[i], 0, sizeof(s_dynamicPOIs[i]));
        s_dynamicPOIs[i].m_name = s_dynamicPOINames[i];
    }

    s_closestPOIFacing[0] = -1.0f;
    s_closestPOIFacing[1] = -1.0f;
    s_closestPOIFacing[2] = -1.0f;
    s_closestPOIs[0] = -1;
    s_closestPOIs[1] = -1;
    s_closestPOIs[2] = -1;
    s_outdoorZoom = g_minimapZoomCVar->m_intValue;
    s_indoorZoom = g_minimapInsideZoomCVar->m_intValue;
    FrameScript_SignalEvent(SCRIPT_EVENT_MINIMAP_UPDATE_ZOOM, nullptr);
    LoadMD5Names();
}

// VERIFY: 6111fd37 manual
// OFFSET: 0x0057C260
static uint32_t ColorToLuminance(CImVector color) {
    return (77 * color.r + 151 * color.g + 28 * color.b) >> 8;
}

// VERIFY: f5253e26 auto
// OFFSET: 0x0057D5B0
void CGMinimapFrame::SetCompassRotation(float rotation) {
    CSimpleTexture* compass = static_cast<CSimpleTexture*>(this->m_compassTexture);

    if (compass && (compass->m_visible)) {
        compass->SetRotation(rotation, 0.5f, 0.5f);
    }
}

// VERIFY: 7d85819d manual
// OFFSET: 0x0057D5F0
void MINIMAPTILE::BuildQuad(const CRect& rect, uint32_t index, const C2Vector* center, float scale, const CRect& bounds, float layoutScale, float width) {
    float size = scale * 2.0f;
    float inv = 1.0f / size;
    this->m_maskCoords[0].x = (rect.minX - bounds.minX) * inv;
    this->m_maskCoords[0].y = (rect.maxY - bounds.minY) * inv;
    this->m_maskCoords[1].x = (rect.maxX - bounds.minX) * inv;
    this->m_maskCoords[1].y = (rect.maxY - bounds.minY) * inv;
    this->m_maskCoords[2].x = (rect.minX - bounds.minX) * inv;
    this->m_maskCoords[2].y = (rect.minY - bounds.minY) * inv;
    this->m_maskCoords[3].x = (rect.maxX - bounds.minX) * inv;
    this->m_maskCoords[3].y = (rect.minY - bounds.minY) * inv;

    const CRect& quad = s_quads[index].m_rect;
    this->m_texCoords[0].x = (rect.minX - quad.minY) * 2.0f;
    this->m_texCoords[0].y = (rect.maxY - quad.minX) * 2.0f;
    this->m_texCoords[1].x = (rect.maxX - quad.minY) * 2.0f;
    this->m_texCoords[1].y = (rect.maxY - quad.minX) * 2.0f;
    this->m_texCoords[2].x = (rect.minX - quad.minY) * 2.0f;
    this->m_texCoords[2].y = (rect.minY - quad.minX) * 2.0f;
    this->m_texCoords[3].x = (rect.maxX - quad.minY) * 2.0f;
    this->m_texCoords[3].y = (rect.minY - quad.minX) * 2.0f;

    float vertexInv = 1.0f / size;
    float extent = layoutScale * width;
    float left = (rect.minX - bounds.minX) * vertexInv * extent;
    float right = vertexInv * (rect.maxX - bounds.minX) * extent;
    float top = extent - vertexInv * (rect.maxY - bounds.minY) * extent;
    float bottom = extent - (rect.minY - bounds.minY) * vertexInv * extent;
    this->m_vertices[0].x = left;
    this->m_vertices[0].y = top;
    this->m_vertices[0].z = 0.0f;
    this->m_vertices[1].x = right;
    this->m_vertices[1].y = top;
    this->m_vertices[1].z = 0.0f;
    this->m_vertices[2].x = left;
    this->m_vertices[2].y = bottom;
    this->m_vertices[2].z = 0.0f;
    this->m_vertices[3].x = right;
    this->m_vertices[3].y = bottom;
    this->m_vertices[3].z = 0.0f;
}

// VERIFY: 92f7692a auto
// OFFSET: 0x0057E540
void MINIMAPTILE::Render(uint32_t index, const CImVector* color) {
    if (!(this->m_flags & 1) || !this->m_texture) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(s_maskTexture, 1, nullptr));
    GxRsSet(GxRs_ColorOp1, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp1, GxTexOp_Mod);
    C3Vector normal(0.0f, 0.0f, 1.0f);
    GxPrimLockVertexPtrs(4, this->m_vertices, sizeof(C3Vector), &normal, 0, color, 0, nullptr, 0, this->m_texCoords, sizeof(C2Vector), this->m_maskCoords, sizeof(C2Vector));
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_quadIndices);
    GxPrimUnlockVertexPtrs();
    GxRsSet(GxRs_Texture1, 0);
}

// VERIFY: e8182a5d manual
// OFFSET: 0x0057E630
void MINIMAPTILE::UpdateQuad(uint32_t index, C2Vector center, float scale, float layoutScale, float width) {
    this->m_flags &= ~1u;

    if (index >= 4) {
        return;
    }

    CRect bounds;
    bounds.minY = center.y - scale;
    bounds.minX = center.x - scale;
    bounds.maxY = center.y + scale;
    bounds.maxX = center.x + scale;

    float minY = bounds.minY;
    float minX = bounds.minX;
    float maxY = bounds.maxY;
    float maxX = bounds.maxX;
    const CRect& quad = s_quads[index].m_rect;

    if (maxX >= quad.maxY) {
        maxX = quad.maxY;
    }

    if (quad.maxX <= maxY) {
        maxY = quad.maxX;
    }

    if (quad.minY >= minX) {
        minX = quad.minY;
    }

    if (quad.minX >= minY) {
        minY = quad.minX;
    }

    CRect rect;
    rect.minY = minY;
    rect.minX = minX;
    rect.maxY = maxY;
    rect.maxX = maxX;

    if (quad.minX < maxY && quad.maxX > minY && maxX > quad.minY && quad.maxY > minX) {
        this->BuildQuad(rect, index, &center, scale, bounds, layoutScale, width);
        this->m_flags |= 1;
    }
}

// VERIFY: 2aace7b1 auto
// OFFSET: 0x0057E780
void CGMinimapFrame::UpdateQuads(const C2Vector& center, float scale) {
    for (uint32_t i = 0; i < 256; i++) {
        s_tiles[i].UpdateQuad(i, center, scale, this->m_layoutScale, this->GetWidth());
    }
}

// VERIFY: 9260db0a auto
// OFFSET: 0x007F3B70
TSGrowableArray<AreaPOIRec*>* MinimapGetPOI(int32_t* changed) {
    *changed = s_poisChanged;
    s_poisChanged = 0;
    return &s_visiblePOIs;
}

// VERIFY: 6880c908 manual
// OFFSET: 0x007F3C80
static void MinimapWorldToTileCoords(const C3Vector* position, int32_t* tile) {
    float y = (17066.666f - position->y) * 0.0018750001f;
    float x = 0.0018750001f * (17066.666f - position->x);

    if (y < 0.0f) {
        y = y - 1.0f;
    }

    tile[0] = static_cast<int32_t>(y);

    if (x >= 0.0f) {
        tile[1] = static_cast<int32_t>(x);
    } else {
        tile[1] = static_cast<int32_t>(x - 1.0f);
    }
}

// VERIFY: 630be33b auto
// OFFSET: 0x007F5240
static int32_t MinimapBuildSource(const int32_t* tile, char* path, uint32_t size) {
    *path = 0;
    MapRec* map = g_mapDB.GetRecord(s_mapId);

    if (!map || !*map->m_directory) {
        SysMsgPrintf(SYSMSG_ERROR, 2, "NOCONTINENTNAME|%d", s_mapId);
        return 0;
    }

    SStrPrintf(path, size, s_minimapTileFormat, map->m_directory, tile[0], tile[1]);
    MINIMAPMD5NAME* name = s_md5Names.Ptr(path);

    if (!name) {
        SysMsgPrintf(SYSMSG_ERROR, 2, "No minimap texture: \"%s\"", path);
        *path = 0;
        return 0;
    }

    SStrPrintf(path, size, "%s\\%s", s_minimapTexturePath, name->m_md5Name);
    return 1;
}

// VERIFY: dd828e8c manual
// OFFSET: 0x007F5760
static void SetupTextureHandles(const int32_t* tile, int32_t reset, MINIMAPTILE* tiles) {
    tiles[0].m_flags &= ~2u;
    tiles[1].m_flags &= ~2u;
    tiles[2].m_flags &= ~2u;
    tiles[3].m_flags &= ~2u;

    if (!reset) {
        for (uint32_t i = 0; i < 4; i++) {
            int32_t x = tile[0] + s_tileOffsets[i][0];
            int32_t y = tile[1] + s_tileOffsets[i][1];

            for (uint32_t j = 0; j < 4; j++) {
                if (tiles[j].m_x == x && tiles[j].m_y == y) {
                    if (j != i) {
                        if (tiles[i].m_texture) {
                            HandleClose(reinterpret_cast<HOBJECT>(tiles[i].m_texture));
                        }

                        tiles[i].m_texture = tiles[j].m_texture;
                        tiles[i].m_x = x;
                        tiles[i].m_y = y;
                        tiles[j].m_texture = nullptr;
                    }

                    tiles[i].m_flags |= 2;
                }
            }
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (tiles[i].m_flags & 2) {
            continue;
        }

        int32_t source[2];
        source[0] = tile[0] + s_tileOffsets[i][0];
        source[1] = tile[1] + s_tileOffsets[i][1];
        char path[260];
        MinimapBuildSource(source, path, sizeof(path));

        if (path[0]) {
            if (tiles[i].m_texture) {
                HandleClose(reinterpret_cast<HOBJECT>(tiles[i].m_texture));
                tiles[i].m_texture = nullptr;
            }

            CStatus status;
            CTexture* texture = TextureCreate(path, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
            tiles[i].m_flags |= 2;
            tiles[i].m_texture = texture;
            tiles[i].m_x = source[0];
            tiles[i].m_y = source[1];
        } else {
            SysMsgPrintf(SYSMSG_ERROR, 4, "MINIMAPCHUNKNOTFOUND|%d|%d", source[0], source[1]);
        }
    }
}

// VERIFY: db65bbd1 manual
// OFFSET: 0x007F5930
static int32_t MinimapUpdatePosition(int32_t mapId, const C3Vector* position, C2Vector* center, float* scale, MINIMAPTILE* tiles) {
    int32_t mapChanged = mapId != s_mapId;
    s_playerPosition = *position;
    s_mapId = mapId;
    UpdatePointsOfInterest();

    C3Vector maxPosition(position->x + s_halfTile.x, position->y + s_halfTile.y, 0.0f);
    int32_t baseTile[2];
    MinimapWorldToTileCoords(&maxPosition, baseTile);
    C3Vector minPosition(position->x - s_halfTile.x, position->y - s_halfTile.y, 0.0f);
    int32_t endTile[2];
    MinimapWorldToTileCoords(&minPosition, endTile);

    int32_t endX = endTile[0];

    if (baseTile[0] == endX) {
        if (endX + 1 >= 64) {
            baseTile[0]--;
        } else {
            endX = endX + 1;
        }
    }

    int32_t endY = endTile[1];

    if (baseTile[1] == endY) {
        if (endY + 1 >= 64) {
            baseTile[1]--;
        } else {
            endY = endY + 1;
        }
    }

    if ((s_updateFlags & 1) || s_tileMinX != baseTile[0] || s_tileMinY != baseTile[1] || s_tileMaxX != endX || s_tileMaxY != endY) {
        SetupTextureHandles(baseTile, mapChanged, tiles);
        s_tileMinX = baseTile[0];
        s_tileMinY = baseTile[1];
        s_tileMaxX = endX;
        s_tileMaxY = endY;
    }

    s_updateFlags &= ~1u;
    C3Vector origin(HALF_WORLD_SIZE - baseTile[1] * MD_AREA_POLY_SIZE, s_mapOrigin - MD_AREA_POLY_SIZE * baseTile[0], 0.0f);
    C2Vector origin2(origin);
    float inv = 1.0f / s_tileSize.y;
    uint32_t halfTiles = s_outdoorZoomTiles[s_outdoorZoom] >> 1;
    float centerX = (origin2.y - position->y) * inv;
    float centerY = (origin2.x - position->x) / s_tileSize.x;
    *scale = inv * (halfTiles * 33.333332f);
    center->x = centerX;
    center->y = centerY;
    AdditionalCheck1();
    return 1;
}

// VERIFY: 283b17d6 auto
// OFFSET: 0x007F3E00
static void SetupMapObjMatrices(CMapEntity* entity) {
    World::QueryMapObjMatrix(entity, &s_mapObjMatrix, &s_mapObjInvMatrix);
    float a0 = s_mapObjMatrix.a0;
    float a1 = s_mapObjMatrix.a1;
    float a2 = s_mapObjMatrix.a2;
    float lengthSq = a1 * a1 + a2 * a2 + a0 * a0;

    if (std::fabs(lengthSq - 1.0f) >= 0.00000023841858f) {
        float inv = 1.0f / std::sqrt(lengthSq);
        float scale = inv / std::sqrt(lengthSq);
        s_mapObjMatrix.a0 = a0 * scale;
        s_mapObjMatrix.a1 = a1 * scale;
        s_mapObjMatrix.a2 = scale * a2;
        float b = inv / std::sqrt(s_mapObjMatrix.b2 * s_mapObjMatrix.b2 + s_mapObjMatrix.b1 * s_mapObjMatrix.b1 + s_mapObjMatrix.b0 * s_mapObjMatrix.b0);
        s_mapObjMatrix.b0 = s_mapObjMatrix.b0 * b;
        s_mapObjMatrix.b1 = s_mapObjMatrix.b1 * b;
        s_mapObjMatrix.b2 = s_mapObjMatrix.b2 * b;
        float c = std::sqrt(s_mapObjMatrix.c2 * s_mapObjMatrix.c2 + s_mapObjMatrix.c1 * s_mapObjMatrix.c1 + s_mapObjMatrix.c0 * s_mapObjMatrix.c0);
        s_mapObjMatrix.c0 = s_mapObjMatrix.c0 * (inv / c);
        s_mapObjMatrix.c1 = s_mapObjMatrix.c1 * (inv / c);
        s_mapObjMatrix.c2 = inv / c * s_mapObjMatrix.c2;
    }

    s_mapObjMatrix.d0 = 0.0f;
    s_mapObjMatrix.d1 = 0.0f;
    s_mapObjMatrix.d2 = 0.0f;
    s_mapObjMatrix.RotateAroundZ(s_mapObjRotation * 0.017453292f);
}

// VERIFY: 8100c6e5 auto
// OFFSET: 0x007F3A70
static void MinimapSetMapObjName(CMapEntity* entity) {
    const char* fileName = nullptr;
    World::QueryMapObjFileName(entity, &fileName);

    if (fileName) {
        fileName += SStrLen("World\\");
        SStrCopy(s_mapObjName, fileName, sizeof(s_mapObjName));
    } else {
        SStrCopy(s_mapObjName, "Dont Crash!", sizeof(s_mapObjName));
    }

    char* extension = SStrChrR(s_mapObjName, '.');

    if (extension) {
        *extension = 0;
    }
}

// VERIFY: a165470a manual
// OFFSET: 0x007F3CE0
static MINIMAPTILE* MinimapFindTile(MINIMAPTILE* tiles, const MINIMAPQUADINFO* quad, int32_t currentGroup, float height) {
    for (uint32_t i = 0; i < 256; i++) {
        MINIMAPTILE* tile = &tiles[i];

        if (tile->m_groupId == quad->m_groupId && tile->m_x == quad->m_x && tile->m_y == quad->m_y) {
            if (tile->m_groupId == currentGroup) {
                tile->m_flags |= 2;
                tile->m_depth = 3.4028235e38f;
            } else {
                tile->m_flags |= 2;
                tile->m_depth = (quad->m_bounds.t.z + quad->m_bounds.b.z) * 0.5f - height;
            }

            return tile;
        }
    }

    return nullptr;
}

// VERIFY: 2cc06210 manual
// OFFSET: 0x007F5070
static void SetupQuad(MINIMAPTILE* tiles, const MINIMAPQUADINFO* quad, int32_t currentGroup, float height, const char* mapObjName) {
    uint32_t index = 0;

    while (tiles->m_flags & 2) {
        index++;
        tiles++;

        if (index >= 256) {
            return;
        }
    }

    tiles->m_flags |= 2;
    char path[260];
    SStrPrintf(path, sizeof(path), s_minimapQuadFormat, mapObjName, quad->m_groupId, quad->m_x, quad->m_y);
    MINIMAPMD5NAME* name = s_md5Names.Ptr(path);

    if (name) {
        SStrPrintf(path, sizeof(path), "%s\\%s", s_minimapTexturePath, name->m_md5Name);
    } else {
        SysMsgPrintf(SYSMSG_ERROR, 2, "No minimap texture: \"%s\"", path);
        path[0] = 0;
    }

    if (tiles->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(tiles->m_texture));
    }

    if (path[0]) {
        CStatus status;
        CTexture* texture = TextureCreate(path, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
        tiles->m_flags |= 4;
        tiles->m_texture = texture;
    } else {
        tiles->m_flags &= ~2u;
        tiles->m_texture = nullptr;
    }

    tiles->m_x = quad->m_x;
    tiles->m_y = quad->m_y;
    tiles->m_bounds = quad->m_bounds;
    tiles->m_groupId = quad->m_groupId;

    if (tiles->m_groupId == currentGroup) {
        tiles->m_depth = 3.4028235e38f;
    } else {
        tiles->m_depth = (tiles->m_bounds.t.z + tiles->m_bounds.b.z) * 0.5f - height;
    }
}

// VERIFY: e07bae80 manual
// OFFSET: 0x007F5BA0
int32_t MinimapUpdate(CMapEntity* entity, int32_t mapId, const C3Vector* position, C2Vector* center, float* scale, MINIMAPTILE* tiles, MINIMAPINDOOR* indoor, int32_t force) {
    uint8_t reload = s_updateFlags & 1;
    bool changed = (s_updateFlags & 3) != 0;
    bool result = changed;

    if (s_worldMapRefreshTime && time(nullptr) >= static_cast<time_t>(s_worldMapRefreshTime)) {
        s_worldMapRefreshTime = 0;
        s_worldMapRefreshPos.x = 0.0f;
        result = true;
        s_worldMapRefreshPos.y = 0.0f;
        CGWorldMap::RefreshCurrentMap();
        changed = result;
    }

    C3Vector delta(0.0f, 0.0f, 0.0f);
    int32_t inside;

    if (s_playerPosition.x != position->x || s_playerPosition.y != position->y || mapId != s_mapId) {
        result = true;
        delta.x = position->x - s_playerPosition.x;
        delta.y = position->y - s_playerPosition.y;
        delta.z = position->z - s_playerPosition.z;
        s_playerPosition = *position;
        s_mapId = mapId;

        WMOAreaTableRec* groupRec = nullptr;
        WMOAreaTableRec* rootRec = nullptr;
        int32_t outdoorArea = 0;
        inside = World::QueryObjectInside(entity);
        int32_t groupId;
        World::QueryMapObjAreaTable(entity, &groupRec, &rootRec, &groupId);
        uint8_t indoorArea = 0;

        if (rootRec) {
            if (rootRec->m_flags & 1) {
                outdoorArea = 1;
            }

            if (rootRec->m_flags & 0x18) {
                indoorArea = 1;
            }
        }

        s_mapObjIndoorArea = indoorArea;
        int32_t wmoId;
        int32_t wmoHash;

        if (groupRec && (groupRec->m_flags & 0x20)) {
            inside = 0;
            s_mapObjIndoorArea = 0;
        } else if ((outdoorArea || indoorArea || inside) && World::QueryMapObjIDs(entity, &wmoId, &wmoHash, &groupId)) {
            indoor->m_active = 1;

            if (wmoId != s_mapObjId || wmoHash != s_mapObjGroup) {
                reload = 1;
                SetupMapObjMatrices(entity);
                MinimapSetMapObjName(entity);

                for (uint32_t i = 0; i < 256; i++) {
                    tiles[i].m_groupId = -1;
                }

                s_mapObjId = wmoId;
                s_mapObjGroup = wmoHash;
            }

            if (groupId != s_mapObjAreaGroup) {
                reload = 1;
                s_mapObjAreaGroup = groupId;
            }

            if (s_indoorBounds.b.x >= position->x || s_indoorBounds.b.y >= position->y || s_indoorBounds.t.x <= position->x) {
                reload = 1;
            } else if (position->y >= s_indoorBounds.t.y) {
                reload = 1;
            }

            goto indoorChecked;
        }

        indoor->m_active = 0;
        s_mapObjAreaGroup = -1;
        s_mapObjGroup = -1;
        s_mapObjId = -1;

    indoorChecked:
        if (s_isIndoors != indoor->m_active) {
            s_updateFlags |= 1;
            s_isIndoors = indoor->m_active;
            FrameScript_SignalEvent(SCRIPT_EVENT_MINIMAP_UPDATE_ZOOM, nullptr);
        }
    } else {
        if (!changed) {
            goto done;
        }

        inside = World::QueryObjectInside(entity);
    }

    if (indoor->m_active) {
        indoor->m_position = *position;
        SetupMapObjMatrices(entity);

        if (reload) {
            float radius = s_indoorZoomRadius[s_indoorZoom];
            float inv = 1.0f / radius;
            int32_t tileY = static_cast<int32_t>(std::floor(inv * position->y));
            int32_t tileX = static_cast<int32_t>(std::floor(position->x * inv));
            float minX = tileX * radius;
            float minY = tileY * radius;
            float half = radius * 0.5f;
            s_indoorBounds.b.y = minY;
            s_indoorBounds.b.x = minX;
            s_indoorBounds.b.z = position->z - half;
            s_indoorBounds.t.x = minX + radius;
            s_indoorBounds.t.y = radius + minY;
            s_indoorBounds.t.z = s_indoorBounds.b.z + half;

            MINIMAPQUADINFO* buffer = static_cast<MINIMAPQUADINFO*>(alloca(256 * sizeof(MINIMAPQUADINFO)));
            TSStackArray<MINIMAPQUADINFO> quads(buffer, 0, 0);
            CAaBox box = s_indoorBounds;
            box.Expand(radius);
            World::QueryMapObjMinimap(entity, &box, &quads, 256, inside != 0, s_mapObjIndoorArea);
            C3Vector local = *position * s_mapObjInvMatrix;

            for (uint32_t i = 0; i < 256; i++) {
                tiles[i].m_flags &= ~2u;
            }

            if (quads.m_count) {
                int32_t currentGroup = quads.m_data[0].m_groupId;

                for (uint32_t i = 0; i < quads.m_count; i++) {
                    if (MinimapFindTile(tiles, &quads.m_data[i], currentGroup, local.z)) {
                        quads.m_data[i].m_groupId = -1;
                    }
                }

                for (uint32_t i = 0; i < quads.m_count; i++) {
                    if (quads.m_data[i].m_groupId != -1) {
                        SetupQuad(tiles, &quads.m_data[i], currentGroup, local.z, s_mapObjName);
                    }
                }
            }

            s_updateFlags = (s_updateFlags & ~1u) | 2;
        }

        if (s_updateFlags & 2) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t flags = tiles[i].m_flags;

                if ((flags & 2) && (flags & 4)) {
                    if (tiles[i].m_texture) {
                        if (!TextureGetGxTex(tiles[i].m_texture, 0, nullptr)) {
                            goto texturesPending;
                        }

                        tiles[i].m_flags &= ~4u;
                    } else {
                        tiles[i].m_flags = flags & ~2u;
                    }
                }
            }

            s_updateFlags &= ~2u;
            C3Vector boxCenter;
            boxCenter.x = (s_indoorBounds.t.x + s_indoorBounds.b.x) * 0.5f;
            indoor->m_center.x = boxCenter.x;
            boxCenter.y = (s_indoorBounds.t.y + s_indoorBounds.b.y) * 0.5f;
            indoor->m_center.y = boxCenter.y;
            boxCenter.z = 0.5f * (s_indoorBounds.t.z + s_indoorBounds.b.z);
            indoor->m_center.z = boxCenter.z;
            indoor->m_matrix = s_mapObjMatrix;
            indoor->m_invMatrix = s_mapObjInvMatrix;
            indoor->m_dirty = 1;
            indoor->m_radius = s_indoorZoomRadius[s_indoorZoom];
        }

    texturesPending:
        if (!reload && indoor->m_invMatrix != s_mapObjInvMatrix) {
            bool notPending = !(s_updateFlags & 2);
            indoor->m_center.x = indoor->m_center.x + delta.x;
            indoor->m_center.y = delta.y + indoor->m_center.y;
            indoor->m_center.z = delta.z + indoor->m_center.z;
            indoor->m_matrix = s_mapObjMatrix;
            indoor->m_invMatrix = s_mapObjInvMatrix;

            if (notPending) {
                indoor->m_dirty = 1;
            }
        }

        UpdatePointsOfInterest();
    } else {
        result = MinimapUpdatePosition(mapId, position, center, scale, tiles) != 0;
    }

    if (s_worldMapRefreshPos.x != 0.0f || s_worldMapRefreshPos.y != 0.0f) {
        C2Vector current(*position);
        float dy = current.y - s_worldMapRefreshPos.y;
        float dx = current.x - s_worldMapRefreshPos.x;

        if (dx * dx + dy * dy < 100.0f) {
            s_worldMapRefreshPos.x = 0.0f;
            s_worldMapRefreshPos.y = 0.0f;
            s_worldMapRefreshTime = 0;

            if (CGWorldMap::m_currentOrphanZone == -1) {
                CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentZone, CGWorldMap::m_currentOrphanZone);
            } else {
                CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentOrphanZone, CGWorldMap::m_currentDungeonMap);
            }
        }
    }

    if (result) {
        return result;
    }

    changed = false;

done:
    if (force && !indoor->m_active) {
        MinimapUpdatePosition(mapId, position, center, scale, tiles);
    }

    return changed;
}

// VERIFY: 49439d70 manual
// OFFSET: 0x00581E80
void CGMinimapFrame::Render() {
    if (!s_maskTexture || !(this->m_layoutFlags & 0x100) || !s_inWorld) {
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    CGObject_C* target = ClntObjMgrObjectPtr(camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);
    C3Vector position = target ? target->GetPosition() : camera->m_position;
    CMapEntity* worldObject;

    if (target) {
        worldObject = target->m_worldObject;
    } else {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (!player) {
            return;
        }

        worldObject = player->m_worldObject;
    }

    CRect rect;
    rect.minY = 0.0f;
    rect.minX = 0.0f;
    rect.maxY = 0.0f;
    rect.maxX = 0.0f;

    if (!this->GetRect(&rect)) {
        return;
    }

    C44Matrix savedProjection;
    C44Matrix savedView;
    savedProjection = g_theGxDevicePtr->m_projection;
    GxXformView(savedView);
    float viewportMinX;
    float viewportMaxX;
    float viewportMinY;
    float viewportMaxY;
    float viewportMinZ;
    float viewportMaxZ;
    GxXformViewport(viewportMinX, viewportMaxX, viewportMinY, viewportMaxY, viewportMinZ, viewportMaxZ);
    C2Vector offset;
    offset.x = rect.minX;
    offset.y = rect.minY;
    CameraSetupScreenProjection(rect, offset, 0.0f, 1);
    DDCToNDC(rect.minX, rect.minY, &rect.minX, &rect.minY);
    DDCToNDC(rect.maxX, rect.maxY, &rect.maxX, &rect.maxY);
    GxXformSetViewport(rect.minX, rect.maxX, rect.minY, rect.maxY, 0.0f, 1.0f);

    C44Matrix view;
    GxXformView(view);
    C3Vector translation(view.d0, view.d1, view.d2);
    C44Matrix rotatedView;

    if (g_rotateMinimapCVar->m_intValue) {
        this->SetCompassRotation(-this->m_facing);
        rotatedView.Translate(translation);
        C3Vector axis(0.0f, 0.0f, 1.0f);
        rotatedView *= C44Matrix::Rotation(-this->m_facing, axis, true);
        C44Matrix untranslate;
        untranslate.Translate(C3Vector(-translation.x, -translation.y, -translation.z));
        rotatedView *= untranslate;
        rotatedView *= view;
        GxXformSetView(rotatedView);
    } else {
        this->SetCompassRotation(0.0f);
    }

    C2Vector center(0.0f, 0.0f);
    float scale;
    int32_t updated = MinimapUpdate(worldObject, CGPlayer_C::GetNewContinentID(), &position, &center, &scale, s_tiles, &s_indoor, s_scaleChanged);

    if ((updated || s_scaleChanged) && !s_indoor.m_active) {
        this->UpdateQuads(center, scale);
        s_scaleChanged = 0;
    }

    static C3Vector s_normal(0.0f, 0.0f, 1.0f);

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    DayNight::DNInfo* info = DayNight::GetInfo();
    CImVector diffuse = info->m_light0.m_diffuse;
    CImVector ambient = info->m_light0.m_ambient;
    CImVector white;
    white.b = 0xFF;
    white.g = 0xFF;
    white.r = 0xFF;
    white.a = 0xFF;
    uint32_t luminance = static_cast<uint8_t>(ColorToLuminance(ambient)) + 96;

    if (luminance > 0xFF) {
        luminance = 0xFF;
    }

    CImVector target2 = white;
    ambient.InterpColorRGB(luminance, &target2);
    target2 = ambient;
    diffuse.InterpColorRGB(192, &target2);
    diffuse.a = this->GetEffectiveAlpha();

    if (s_indoor.m_active) {
        C44Matrix savedIndoorView;
        GxXformView(savedIndoorView);
        C44Matrix identity;
        GxXformSetView(identity);

        if (GxIsWindowVisible() && GxHasContext()) {
            if (s_indoor.m_dirty) {
                GxTexUpdate(TextureGetGxTex(s_indoor.m_texture, 1, nullptr), 0, 0, 0, 0, 0);
                s_indoor.m_dirty = 0;
            }

            C3Vector direction(s_indoor.m_position.x - s_indoor.m_center.x, s_indoor.m_position.y - s_indoor.m_center.y, s_indoor.m_position.z - s_indoor.m_center.z);
            C3Vector local = TransformDirection(direction, s_indoor.m_invMatrix);
            direction = TransformDirection(local, s_indoor.m_matrix);
            float inv = 1.0f / s_indoor.m_radius;
            CImVector color;
            color.b = 0xFF;
            color.g = 0xFF;
            color.r = 0xFF;
            color.a = diffuse.a;
            C2Vector indoorCenter;
            indoorCenter.x = direction.x * inv * s_indoorTexScale + 0.5f;
            indoorCenter.y = inv * direction.y * s_indoorTexScale + 0.5f;
            this->RenderInside(s_indoor.m_radius, &indoorCenter, &color);
            GxXformSetView(savedIndoorView);
        }
    } else {
        for (uint32_t i = 0; i < 4; i++) {
            s_tiles[i].Render(i, &diffuse);
        }
    }

    if (g_rotateMinimapCVar->m_intValue) {
        g_theGxDevicePtr->XformSetView(view);
    }

    int32_t poisChanged;
    TSGrowableArray<AreaPOIRec*>* pois = MinimapGetPOI(&poisChanged);

    if (poisChanged) {
        s_poiInfo.SetCount(pois->m_count);

        for (uint32_t i = 0; i < s_poiInfo.m_count; i++) {
            AreaPOIRec* poi = pois->m_data[i];
            uint32_t state = 0;

            if (poi->m_worldStateID) {
                state = GetWorldState(poi->m_worldStateID) - 1;

                if (static_cast<int32_t>(state) < 0 || state >= 9) {
                    state = 0;
                }
            }

            s_poiInfo.m_data[i].m_icon = pois->m_data[i]->m_icon[state];
            s_poiInfo.m_data[i].m_name = pois->m_data[i]->m_name;
        }
    }

    CImVector iconColor;
    iconColor.b = 0xFF;
    iconColor.g = 0xFF;
    iconColor.r = 0xFF;
    iconColor.a = 0xFF;
    float viewRadius = MinimapGetViewRadius();
    C44Matrix rotation;

    if (g_rotateMinimapCVar->m_intValue) {
        rotation.Translate(translation);
        C3Vector axis(0.0f, 0.0f, 1.0f);
        rotation *= C44Matrix::Rotation(-this->m_facing, axis, true);
        C44Matrix untranslate;
        untranslate.Translate(C3Vector(-translation.x, -translation.y, -translation.z));
        rotation *= untranslate;
    }

    for (uint32_t i = 0; i < s_poiInfo.m_count; i++) {
        if (static_cast<uint32_t>(s_poiInfo.m_data[i].m_icon) > 195) {
            continue;
        }

        C3Vector origin = position;
        AreaPOIRec* poi = pois->m_data[i];
        float poiX = poi->m_pos[0];
        float poiY = poi->m_pos[1];
        float half = this->GetWidth() * this->m_layoutScale * 0.5f;
        float inv = 1.0f / viewRadius;
        C3Vector point;
        point.x = half - (poiY - origin.y) * half * inv;
        point.y = (poiX - origin.x) * half * inv + half;
        point.z = 0.0f;

        if (g_rotateMinimapCVar->m_intValue) {
            point *= rotation;
        }

        s_poiInfo.m_data[i].m_position.x = point.x;
        s_poiInfo.m_data[i].m_position.y = point.y;

        C3Vector vertices[4];
        uint16_t base = static_cast<uint16_t>(s_blipVertices.m_count);

        for (uint32_t j = 0; j < 4; j++) {
            vertices[j].x = CGMinimapFrame::s_blipQuads[0][j].x * this->m_layoutScale + point.x;
            vertices[j].y = CGMinimapFrame::s_blipQuads[0][j].y * this->m_layoutScale + point.y;
            vertices[j].z = CGMinimapFrame::s_blipQuads[0][j].z * this->m_layoutScale + point.z;
        }

        s_blipVertices.Add(4, vertices);
        uint16_t indices[6];
        indices[2] = base + 2;
        indices[3] = base + 2;
        indices[0] = base;
        indices[1] = base + 1;
        indices[4] = base + 1;
        indices[5] = base + 3;
        s_blipIndices.Add(6, indices);

        C2Vector texCoords[4];

        for (uint32_t j = 0; j < 4; j++) {
            texCoords[j] = s_iconTexCoords[s_poiInfo.m_data[i].m_icon][j];
        }

        s_blipTexCoords2.Add(4, texCoords);
    }

    if (s_iconTexture) {
        CGxTex* gxTex = TextureGetGxTex(s_iconTexture, 0, nullptr);

        if (gxTex && s_blipVertices.m_count) {
            g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
            GxPrimLockVertexPtrs(s_blipVertices.m_count, s_blipVertices.m_data, sizeof(C3Vector), &s_normal, 0, &iconColor, 0, nullptr, 0, s_blipTexCoords2.m_data, sizeof(C2Vector), nullptr, 0);
            GxPrimLockIndexPtr(GxPrim_Triangles, s_blipIndices.m_count, s_blipIndices.m_data);
            GxDrawLockedElements();
            GxPrimUnlockVertexPtrs();
            s_blipVertices.SetCount(0);
            s_blipTexCoords2.SetCount(0);
            s_blipIndices.m_count = 0;
        }
    }

    uint32_t lastUpdate = this->m_lastUpdateTime;
    bool expired = OsGetAsyncTimeMs() - lastUpdate >= 1000;

    if (updated || s_rotateMinimap != g_rotateMinimapCVar->m_intValue || !lastUpdate || expired) {
        uint32_t now = OsGetAsyncTimeMs();
        this->m_lastUpdateTime = now;

        if (!now) {
            this->m_lastUpdateTime = 1;
        }

        for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
            s_objInfo[i].SetCount(0);
        }

        int32_t wmoId = 0;
        int32_t wmoHash = 0;
        int32_t groupNum;

        if (World::QueryObjectInside(worldObject)) {
            World::QueryMapObjIDs(worldObject, &wmoId, &wmoHash, &groupNum);
        }

        MINIMAPENUMPARAM param;
        param.m_position = C3Vector(0.0f, 0.0f, 0.0f);
        param.m_player = ClntObjMgrGetActivePlayerObj();
        param.m_radius = viewRadius;
        param.m_scale = this->m_layoutScale;
        param.m_position = position;
        param.m_wmoId = wmoId;
        param.m_wmoHash = wmoHash;
        param.m_frame = this;
        ClntObjMgrEnumVisibleObjects(CGMinimapFrame::ObjectEnumProc, &param);

        CImVector vehicleColor;
        vehicleColor.b = 0xFF;
        vehicleColor.g = 0xFF;
        vehicleColor.r = 0xFF;
        vehicleColor.a = 0xAA;
        CImVector* playerColor;

        if (param.m_player->m_vehiclePassenger && param.m_player->m_vehiclePassenger->m_state) {
            playerColor = &vehicleColor;
        } else {
            playerColor = &iconColor;
        }

        for (uint32_t i = 0; i < CGBattlefieldInfo::m_numVehicles; i++) {
            WOWGUID vehicleGuid = i < CGBattlefieldInfo::m_numVehicles ? CGBattlefieldInfo::m_vehicleGuids[i] : WOWGUID();
            CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (!vehicle || vehicle->IsDeadOrGhost()) {
                continue;
            }

            C3Vector vehiclePosition = vehicle->GetPosition();
            float dx = vehiclePosition.x - position.x;
            float dy = vehiclePosition.y - position.y;
            float dz = vehiclePosition.z - position.z;
            float limit = viewRadius * 0.8f;

            if (dy * dy + dz * dz + dx * dx > limit * limit) {
                continue;
            }

            WOWGUID playerGuid = param.m_player->m_obj->m_guid;
            CGUnitData* unitData = vehicle->m_unit;

            if (unitData->m_unitFieldFlags & 0x1000000) {
                const WOWGUID& controller = unitData->m_unitFieldCharmedby ? unitData->m_unitFieldCharmedby : unitData->m_unitFieldCreatedby;

                if (playerGuid == controller) {
                    playerColor = &iconColor;
                    continue;
                }
            }

            VehicleRec* vehicleRec = vehicle->m_vehicle ? vehicle->m_vehicle->m_vehicleRec : nullptr;

            if (!vehicleRec || !(vehicleRec->m_flags & 0x10000000)) {
                continue;
            }

            TSGrowableArray<OBJINFO>* list;

            switch (vehicleRec->m_uiLocomotionType) {
                case 0:
                    list = (vehicle->m_unit->m_unitFieldFlags & 0x1000000) ? &s_objInfo[MINIMAP_VEHICLE_GROUND_OCCUPIED] : &s_objInfo[MINIMAP_VEHICLE_GROUND_UNOCCUPIED];
                    break;

                case 1:
                    list = (vehicle->m_unit->m_unitFieldFlags & 0x1000000) ? &s_objInfo[MINIMAP_VEHICLE_AIR_OCCUPIED] : &s_objInfo[MINIMAP_VEHICLE_AIR_UNOCCUPIED];
                    break;

                case 3:
                    list = &s_objInfo[MINIMAP_VEHICLE_AIR_HORDE];
                    break;

                case 4:
                    list = &s_objInfo[MINIMAP_VEHICLE_AIR_ALLIANCE];
                    break;

                default:
                    continue;
            }

            OBJINFO* info2 = list->New();
            info2->m_guid = vehicleGuid;
            info2->m_otherFloor = 0;
            C3Vector blipPosition = vehicle->GetPosition();
            float radius = param.m_radius;
            float half = this->GetWidth() * param.m_scale * 0.5f;
            float inv = 1.0f / radius;
            float x = half - (blipPosition.y - param.m_position.y) * half * inv;
            info2->m_position.x = x;
            info2->m_rotated.x = x;
            float y = half + (blipPosition.x - param.m_position.x) * half * inv;
            info2->m_position.y = y;
            info2->m_rotated.y = y;
            info2->m_facing = vehicle->GetFacing();
        }

        this->m_playerTexture->SetVertexColor(*playerColor);

        for (uint32_t i = 0; i < 5; i++) {
            if (!s_partyMembers[i].m_onMap) {
                continue;
            }

            OBJINFO* info2 = s_objInfo[MINIMAP_PARTY].New();
            info2->m_guid = s_partyMembers[i].m_guid;
            info2->m_otherFloor = 0;
            float radius = param.m_radius;
            float half = this->GetWidth() * param.m_scale * 0.5f;
            float inv = 1.0f / radius;
            float x = half - (s_partyMembers[i].m_position.y - param.m_position.y) * half * inv;
            info2->m_position.x = x;
            info2->m_rotated.x = x;
            float y = half + (s_partyMembers[i].m_position.x - param.m_position.x) * half * inv;
            info2->m_position.y = y;
            info2->m_rotated.y = y;
            info2->m_icon = s_partyMembers[i].m_class;
        }

        for (uint32_t i = 0; i < 40; i++) {
            if (!s_raidMembers[i].m_onMap) {
                continue;
            }

            OBJINFO* info2 = s_objInfo[MINIMAP_RAID].New();
            info2->m_guid = s_raidMembers[i].m_guid;
            info2->m_otherFloor = 0;
            float radius = param.m_radius;
            float half = this->GetWidth() * param.m_scale * 0.5f;
            float inv = 1.0f / radius;
            float x = half - (s_raidMembers[i].m_position.y - param.m_position.y) * half * inv;
            info2->m_position.x = x;
            info2->m_rotated.x = x;
            float y = half + (s_raidMembers[i].m_position.x - param.m_position.x) * half * inv;
            info2->m_position.y = y;
            info2->m_rotated.y = y;
            info2->m_icon = s_raidMembers[i].m_class;
        }
    }

    this->RenderGroupBlips(rotation);
    this->RenderVehicleBlips(rotation);
    this->RenderObjectBlips(rotation, false);
    this->RenderObjectBlips(rotation, true);
    this->RenderOverlay(rotation);
    this->RenderPartyBlips(rotation);
    this->RenderRaidArrows(rotation);
    g_theGxDevicePtr->RsPop();
    g_theGxDevicePtr->XformSetProjection(savedProjection);
    g_theGxDevicePtr->XformSetView(savedView);
    GxXformSetViewport(viewportMinX, viewportMaxX, viewportMinY, viewportMaxY, viewportMinZ, viewportMaxZ);
    CShaderEffect::UpdateProjMatrix();
    s_rotateMinimap = g_rotateMinimapCVar->m_intValue;
}

// VERIFY: cabffc62 auto
// OFFSET: 0x0057BF30
bool CGMinimapFrame::IsTrackingTrivialQuests() {
    return CGMinimapFrame::s_trackingInfo && CGMinimapFrame::s_trackingInfo->m_type == 3;
}

// VERIFY: 6d24deb9 auto
// OFFSET: 0x0057C660
C2Vector CGMinimapFrame::WorldPosToMinimapFrameCoords(C3Vector center, float radius, float x, float y, float scale, float width) {
    float inv = 1.0f / radius;
    float half = scale * width * 0.5f;
    C2Vector coords;
    coords.x = half - (y - center.y) * half * inv;
    coords.y = (x - center.x) * half * inv + half;
    return coords;
}

// VERIFY: 43050227 manual
// OFFSET: 0x0057EB80
void CGMinimapFrame::SetPingPosition(const WOWGUID& guid, const C2Vector& position) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CGMinimapFrame::s_pingPosition = position;
    char unit[32] = "player";

    if (guid != player->m_obj->m_guid) {
        bool found = false;

        for (uint32_t i = 0; i < CGPartyInfo::GetNumPartyMembers(); i++) {
            if (guid == CGGameUI::GetPartyMember(i)) {
                SStrPrintf(unit, sizeof(unit), "party%d", i + 1);
                found = true;
                break;
            }
        }

        if (!found) {
            for (uint32_t j = 0; j < CGRaidInfo::m_numMembers; j++) {
                if (guid == GetRaidMemberGUID(j)) {
                    SStrPrintf(unit, sizeof(unit), "raid%d", j + 1);
                    break;
                }
            }
        }
    }

    C2Vector playerPosition(player->GetPosition());
    C2Vector offset;
    offset.x = position.x - playerPosition.x;
    offset.y = position.y - playerPosition.y;

    if (g_rotateMinimapCVar->m_intValue) {
        C44Matrix rotation = C44Matrix::Rotation(-CGMinimapFrame::s_facing, C3Vector(0.0f, 0.0f, 1.0f), true);
        C2Vector rotated;
        rotated.x = rotation.c0 * 0.0f + rotation.a0 * offset.x + rotation.b0 * offset.y;
        rotated.y = offset.y * rotation.b1 + offset.x * rotation.a1 + 0.0f * rotation.c1;
        offset = rotated;
    }

    float radius = MinimapGetViewRadius();
    FrameScript_SignalEvent(SCRIPT_EVENT_MINIMAP_PING, "%s%f%f", unit, -(1.0 / (radius + radius) * offset.y), offset.x * (1.0 / (radius + radius)));
}

// VERIFY: 7e3ce000 manual
// OFFSET: 0x0057F7F0
int32_t CGMinimapFrame::ObjectEnumProc(WOWGUID guid, void* param) {
    if (guid == ClntObjMgrGetActivePlayer()) {
        return 1;
    }

    MINIMAPENUMPARAM* data = static_cast<MINIMAPENUMPARAM*>(param);
    CGPlayer_C* player = data->m_player;
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return 1;
    }

    C3Vector position = object->GetPosition();
    float centerX = data->m_position.x;
    float centerY = data->m_position.y;
    float radius = data->m_radius;

    if (radius * 0.8f * (radius * 0.8f) < (position.x - centerX) * (position.x - centerX) + (position.y - centerY) * (position.y - centerY)) {
        return 1;
    }

    uint8_t tracked = 0;
    uint8_t questGiver = 0;
    uint32_t type;
    uint32_t objectType = object->m_obj->m_type;

    if (objectType == HEIR_TYPE_GAMEOBJECT) {
        CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(object);

        if (gameObject->IsTracked() && static_cast<CGUnit_C*>(player)->CanInteract(gameObject)) {
            type = MINIMAP_OBJECT;
        } else {
            uint32_t status = gameObject->m_questStatus;

            if (status == 10) {
                type = MINIMAP_QUEST_COMPLETE;
            } else if (status == 8 || (status == 2 && CGMinimapFrame::IsTrackingTrivialQuests())) {
                type = MINIMAP_QUEST_AVAILABLE;
            } else if (status == 7 || (status == 4 && CGMinimapFrame::IsTrackingTrivialQuests())) {
                type = MINIMAP_QUEST_REPEATABLE;
            } else {
                if (!player->CanTrackObject(gameObject)) {
                    return 1;
                }

                type = MINIMAP_OBJECT;
            }
        }
    } else {
        if (objectType == (TYPE_PLAYER | TYPE_UNIT | TYPE_OBJECT)) {
            if (CGGameUI::IsInParty(guid) || CGGameUI::IsPartyMember(guid)) {
                return 1;
            }
        } else if (objectType != (TYPE_UNIT | TYPE_OBJECT)) {
            return 1;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(object);
        const WOWGUID& controller = unit->m_unit->m_unitFieldCharmedby ? unit->m_unit->m_unitFieldCharmedby : unit->m_unit->m_unitFieldSummonedby;

        if (controller == ClntObjMgrGetActivePlayer()) {
            type = MINIMAP_PET;
        } else {
            if (unit->m_unit->m_unitFieldHealth <= 0 && !unit->CanInteractWhileDead()) {
                return 1;
            }

            if (unit->IsTracked() && player->CanInteract(unit)) {
                if (CGMinimapFrame::s_trackingInfo->m_npcFlags & 0x40) {
                    tracked = 1;
                }

                type = MINIMAP_OBJECT;
            } else {
                uint32_t status = unit->m_questStatus;

                if (status == 10) {
                    type = MINIMAP_QUEST_COMPLETE;
                } else if (status == 8 || (status == 2 && CGMinimapFrame::IsTrackingTrivialQuests())) {
                    type = MINIMAP_QUEST_AVAILABLE;
                } else if (status == 7 || (status == 4 && CGMinimapFrame::IsTrackingTrivialQuests())) {
                    type = MINIMAP_QUEST_REPEATABLE;
                } else if (unit->m_interactIconType) {
                    type = MINIMAP_FLIGHTMASTER;
                    questGiver = 1; // The native tooltip flag identifies a flight master.
                } else {
                    if (!player->CanTrackUnit(unit)) {
                        return 1;
                    }

                    if (unit->m_obj->m_type & TYPE_PLAYER) {
                        if (player->CanAttack(unit)) {
                            type = MINIMAP_PLAYER_HOSTILE;
                        } else {
                            int32_t faction = static_cast<CGPlayer_C*>(unit)->GetPVPFactionIndex();
                            type = MINIMAP_PLAYER_SAME_FACTION - (faction != player->GetPVPFactionIndex());
                        }
                    } else {
                        UNIT_REACTION reaction = unit->UnitReaction(player);

                        if (reaction < UNIT_REACTION_FRIENDLY) {
                            type = (reaction > UNIT_REACTION_HOSTILE) + MINIMAP_MONSTER_HOSTILE;
                        } else {
                            type = MINIMAP_MONSTER_FRIENDLY;
                        }
                    }
                }
            }
        }
    }

    int32_t wmoId = 0;
    int32_t wmoHash = 0;
    int32_t groupNum;

    if (World::QueryObjectInside(object->m_worldObject)) {
        World::QueryMapObjIDs(object->m_worldObject, &wmoId, &wmoHash, &groupNum);
    }

    if (!data->m_wmoId || (wmoId == data->m_wmoId && wmoHash == data->m_wmoHash)) {
        OBJINFO* info = s_objInfo[type].New();
        C3Vector objectPosition = object->GetPosition();
        float width = data->m_frame->GetWidth();
        C2Vector coords = CGMinimapFrame::WorldPosToMinimapFrameCoords(data->m_position, data->m_radius, objectPosition.x, objectPosition.y, data->m_scale, width);
        info->m_guid = guid;
        info->m_rotated.x = coords.x;
        info->m_rotated.y = coords.y;
        info->m_position.y = coords.y;
        info->m_position.x = coords.x;
        info->m_otherFloor = wmoId != data->m_wmoId;
        info->m_tracked = tracked;
        info->m_questGiver = questGiver;
    }

    return 1;
}

// VERIFY: 1ba06c1a manual
// OFFSET: 0x007F3F40
static void SetMemberData(WOWGUID guid, MEMBERINFO* member, CGPlayer_C* player, uint32_t index, bool raid) {
    float dx = s_playerPosition.x;
    float dy = s_playerPosition.y;
    C3Vector position(0.0f, 0.0f, 0.0f);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (unit->m_vehiclePassenger && unit->m_vehiclePassenger->m_state) {
            for (uint32_t i = 0; i < CGBattlefieldInfo::m_numVehicles; i++) {
                WOWGUID vehicleGuid = i < CGBattlefieldInfo::m_numVehicles ? CGBattlefieldInfo::m_vehicleGuids[i] : WOWGUID();
                CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(vehicleGuid, TYPE_UNIT, __FILE__, __LINE__));

                if (!vehicle) {
                    continue;
                }

                VehicleRec* vehicleRec = vehicle->m_vehicle ? vehicle->m_vehicle->m_vehicleRec : nullptr;
                bool shown = vehicleRec && (vehicleRec->m_flags & 0x10000000);
                CGUnitData* unitData = vehicle->m_unit;

                if (unitData->m_unitFieldFlags & 0x1000000) {
                    const WOWGUID& controller = unitData->m_unitFieldCharmedby ? unitData->m_unitFieldCharmedby : unitData->m_unitFieldCreatedby;

                    if (guid == controller && shown) {
                        member->m_offMap = 0;
                        member->m_onMap = 0;
                        return;
                    }
                }
            }
        }

        dx = dx - unit->GetPosition().x;
        dy = dy - unit->GetPosition().y;
        position = unit->GetPosition();
        member->m_class = unit->m_unit->m_unitFieldBytes0[1];
    } else {
        if (guid == player->m_obj->m_guid) {
            member->m_offMap = 0;
            member->m_onMap = 0;
            return;
        }

        CGPartyMemberState* state = raid ? CGRaidInfo::GetRemoteStats(guid) : (index < 4 ? CGPartyInfo::GetMemberState(index) : nullptr);
        int32_t continentId = -1;

        if (!state) {
            member->m_offMap = 0;
            member->m_onMap = 0;
            return;
        }

        AreaTableRec* area = g_areaTableDB.GetRecord(state->m_zoneID);

        if (area) {
            continentId = area->m_continentID;
        }

        if (continentId != CGPlayer_C::GetNewContinentID() || !(state->m_flags & 1) || (state->m_flags & 0x20)) {
            member->m_offMap = 0;
            member->m_onMap = 0;
            return;
        }

        dx = dx - state->m_posX;
        dy = dy - state->m_posY;
        position.x = state->m_posX;
        position.y = state->m_posY;

        for (uint32_t i = 0; i < CGBattlefieldInfo::m_numVehicles; i++) {
            CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGBattlefieldInfo::GetVehicleGuid(i), TYPE_UNIT, __FILE__, __LINE__));

            if (!vehicle) {
                continue;
            }

            VehicleRec* vehicleRec = vehicle->m_vehicle ? vehicle->m_vehicle->m_vehicleRec : nullptr;
            bool shown = vehicleRec && (vehicleRec->m_flags & 0x10000000);

            if (vehicle->IsPossessedBy(guid) && shown) {
                member->m_offMap = 0;
                member->m_onMap = 0;
                return;
            }
        }
    }

    member->m_guid = guid;

    if (guid == player->m_obj->m_guid) {
        SStrCopy(member->m_name, unit->GetUnitName(nullptr, 1), sizeof(member->m_name));
    } else {
        WOWGUID requester = WOWGUID();
        NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);

        if (name) {
            SStrCopy(member->m_name, name->m_name, sizeof(member->m_name));
            member->m_class = name->m_class;
        } else {
            member->m_name[0] = 0;
        }
    }

    float range = s_isIndoors ? s_indoorZoomRadius[s_indoorZoom] : static_cast<float>(s_outdoorZoomTiles[s_outdoorZoom]) * 0.5f * 33.333332f;

    if (std::sqrt(dy * dy + dx * dx) / range <= 0.8f) {
        member->m_offMap = 0;
        member->m_position.x = position.x;
        member->m_onMap = 1;
        member->m_position.y = position.y;
    } else {
        member->m_onMap = 0;
        member->m_offMap = 1;
        member->m_facing = CalculateFacingTo(&s_playerPosition, &position);
    }
}

// VERIFY: 9e41d744 manual
// OFFSET: 0x007F4320
static void MinimapGetPartyMembers(MEMBERINFO* members) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    for (uint32_t i = 0; i < 4; i++) {
        WOWGUID guid = GetPartyMemberGUID(i);

        if (guid) {
            SetMemberData(guid, &members[i], player, i, false);
        } else {
            members[i].m_onMap = 0;
            members[i].m_offMap = 0;
        }
    }
}

// VERIFY: 18766529 manual
// OFFSET: 0x007F4390
static void MinimapGetRaidMembers(MEMBERINFO* members) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    CGRaidMember* self = CGRaidInfo::FindMemberByGuid(player->m_obj->m_guid);

    for (uint32_t i = 0; i < 40; i++) {
        CGRaidMember* raidMember;

        if (i < CGRaidInfo::m_numMembers && (raidMember = CGRaidInfo::m_members[i]) != nullptr && self && raidMember->m_guid && raidMember->m_subgroup != self->m_subgroup) {
            SetMemberData(raidMember->m_guid, &members[i], player, i, true);
        } else {
            members[i].m_onMap = 0;
            members[i].m_offMap = 0;
        }
    }
}

// VERIFY: f617bf0f manual
// OFFSET: 0x007F4B60
static int32_t MinimapBuildPOIDirectionData(TSGrowableArray<POIDIRECTIONDATA>* directions) {
    if (!s_closestPOIsChanged) {
        for (uint32_t i = 0; i < s_numClosestPOIs; i++) {
            directions->m_data[i].m_facing = s_closestPOIFacing[i];
        }

        return 0;
    }

    directions->SetCount(s_numClosestPOIs);
    s_closestPOIsChanged = 0;

    for (uint32_t i = 0; i < s_numClosestPOIs; i++) {
        POIDIRECTIONDATA* direction = &directions->m_data[i];
        AreaPOIRec* poi = s_pois.m_data[s_closestPOIs[i]];
        SStrCopy(direction->m_name, poi->m_name, sizeof(direction->m_name));
        direction->m_facing = s_closestPOIFacing[i];
        poi = s_pois.m_data[s_closestPOIs[i]];
        direction->m_alwaysShow = poi == &s_dynamicPOIs[1] || poi == &s_dynamicPOIs[2] || poi == &s_dynamicPOIs[0] || (poi->m_flags & 0x100);
        direction->m_dynamicIndex = -2;

        for (uint32_t j = 0; j < 4; j++) {
            if (s_pois.m_data[s_closestPOIs[i]] == &s_dynamicPOIs[j]) {
                direction->m_dynamicIndex = j;
                break;
            }
        }
    }

    return 1;
}

// VERIFY: ede40460 manual
// OFFSET: 0x0057D860
void CGMinimapFrame::RenderOverlay(const C44Matrix& rotation) {
    C2Vector texCoords[4];
    C3Vector vertices[4];
    static C3Vector s_normal(0.0f, 0.0f, 1.0f);

    MinimapBuildPOIDirectionData(&s_poiDirections);

    for (uint32_t i = 0; i < s_poiDirections.m_count; i++) {
        POIDIRECTIONDATA* direction = &s_poiDirections.m_data[i];

        if (s_indoor.m_active && !direction->m_alwaysShow) {
            continue;
        }

        CTexture* texture;

        if (direction->m_dynamicIndex == 2) {
            texture = s_corpseArrowTexture;
        } else if (direction->m_dynamicIndex == -2) {
            texture = s_staticPOIArrowTexture;
        } else {
            texture = s_poiArrowTexture;
        }

        if (!texture) {
            continue;
        }

        CGxTex* gxTex = TextureGetGxTex(texture, 0, nullptr);

        if (!gxTex) {
            continue;
        }

        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
        CImVector color;
        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;
        float facing = direction->m_facing;
        float cosine = std::cos(facing);
        float sine = std::sin(facing);
        float width = this->GetWidth();
        float height = this->GetHeight();
        C3Vector point;
        point.x = width * this->m_layoutScale * 0.5f + CGMinimapFrame::s_blipSize4 * sine * -0.8f * this->m_layoutScale;
        point.y = height * this->m_layoutScale * 0.5f + CGMinimapFrame::s_blipSize4 * cosine * 0.8f * this->m_layoutScale;
        point.z = 0.0f;

        if (g_rotateMinimapCVar->m_intValue) {
            facing = facing - this->m_facing;
            point *= rotation;
        }

        facing = facing - s_arrowRotation;
        cosine = std::cos(facing);
        sine = std::sin(facing);
        texCoords[0].x = sine + 0.5f;
        texCoords[0].y = 0.5f - cosine;
        texCoords[1].x = 0.5f - cosine;
        texCoords[1].y = 0.5f - sine;
        texCoords[2].x = cosine + 0.5f;
        texCoords[2].y = sine + 0.5f;
        texCoords[3].x = 0.5f - sine;
        texCoords[3].y = cosine + 0.5f;
        s_poiArrowPositions[i].x = point.x;
        s_poiArrowPositions[i].y = point.y;

        for (uint32_t j = 0; j < 4; j++) {
            vertices[j].x = CGMinimapFrame::s_blipQuads[2][j].x * this->m_layoutScale + point.x;
            vertices[j].y = CGMinimapFrame::s_blipQuads[2][j].y * this->m_layoutScale + point.y;
            vertices[j].z = CGMinimapFrame::s_blipQuads[2][j].z * this->m_layoutScale + point.z;
        }

        GxPrimLockVertexPtrs(4, vertices, sizeof(C3Vector), &s_normal, 0, &color, 0, nullptr, 0, texCoords, sizeof(C2Vector), nullptr, 0);
        GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_arrowIndices);
        GxPrimUnlockVertexPtrs();
    }
}

// VERIFY: 596d6a8a manual
// OFFSET: 0x0057FBD0
void CGMinimapFrame::LeaveWorld() {
    MinimapShutdown();

    for (uint32_t i = 0; i < 256; i++) {
        if (s_tiles[i].m_texture) {
            HandleClose(reinterpret_cast<HOBJECT>(s_tiles[i].m_texture));
            s_tiles[i].m_texture = nullptr;
        }

        s_tiles[i].m_flags = 0;
        s_tiles[i].m_x = -1;
        s_tiles[i].m_y = -1;
    }

    if (s_indoor.m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_indoor.m_texture));
    }

    s_indoor.m_texture = nullptr;
    s_poiInfo.Clear();

    for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
        s_objInfo[i].Clear();
    }

    s_inWorld = 0;
}

// VERIFY: 5b02e890 manual
// OFFSET: 0x0057FF70
void CGMinimapFrame::RenderGroupBlips(const C44Matrix& rotation) {
    if (!s_classBlipTexture) {
        return;
    }

    for (uint32_t list = MINIMAP_RAID; list < MINIMAP_PLAYER_HOSTILE; list++) {
        for (uint32_t i = 0; i < s_objInfo[list].m_count; i++) {
            OBJINFO* info = &s_objInfo[list].m_data[i];

            if (info->m_otherFloor) {
                continue;
            }

            C3Vector point(info->m_position.x, info->m_position.y, 0.0f);

            if (g_rotateMinimapCVar->m_intValue) {
                point = C3Vector(info->m_rotated.x, info->m_rotated.y, 0.0f);
                point *= rotation;
                info->m_position.x = point.x;
                info->m_position.y = point.y;
            }

            uint16_t base = static_cast<uint16_t>(s_blipVertices.m_count);
            C3Vector vertices[4];

            for (uint32_t j = 0; j < 4; j++) {
                vertices[j].x = this->m_layoutScale * CGMinimapFrame::s_blipQuads[0][j].x + point.x;
                vertices[j].y = this->m_layoutScale * CGMinimapFrame::s_blipQuads[0][j].y + point.y;
                vertices[j].z = 0.0f;
            }

            s_blipVertices.Add(4, vertices);
            uint16_t indices[6];
            indices[2] = base + 2;
            indices[3] = base + 2;
            indices[0] = base;
            indices[1] = base + 1;
            indices[4] = base + 1;
            indices[5] = base + 3;
            s_blipIndices.Add(6, indices);

            int32_t icon = info->m_icon - 1;

            if (!list) { // Raid classes use the second half of the class atlas.
                icon += 16;
            }

            s_blipTexCoords2.Add(4, s_blipTexCoords[icon]);
        }
    }

    if (!s_blipVertices.m_count || !s_blipIndices.m_count) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(s_classBlipTexture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    static CImVector s_color(0xFF, 0xFF, 0xFF, 0xFF);
    static C3Vector s_normal(0.0f, 0.0f, 1.0f);
    GxPrimLockVertexPtrs(s_blipVertices.m_count, s_blipVertices.m_data, sizeof(C3Vector), &s_normal, 0, &s_color, 0, nullptr, 0, s_blipTexCoords2.m_data, sizeof(C2Vector), nullptr, 0);
    GxPrimLockIndexPtr(GxPrim_Triangles, s_blipIndices.m_count, s_blipIndices.m_data);
    GxDrawLockedElements();
    GxPrimUnlockVertexPtrs();
    s_blipVertices.SetCount(0);
    s_blipTexCoords2.SetCount(0);
    s_blipIndices.SetCount(0);
}

// VERIFY: a2aea681 manual
// OFFSET: 0x00580380
void CGMinimapFrame::RenderObjectBlips(const C44Matrix& rotation, bool otherFloor) {
    if (!s_blipTexture) {
        return;
    }

    for (uint32_t list = MINIMAP_PLAYER_HOSTILE; list < MINIMAP_VEHICLE_GROUND_UNOCCUPIED; list++) {
        for (uint32_t i = 0; i < s_objInfo[list].m_count; i++) {
            OBJINFO* info = &s_objInfo[list].m_data[i];

            if (info->m_otherFloor != otherFloor) {
                continue;
            }

            C3Vector point(info->m_position.x, info->m_position.y, 0.0f);

            if (g_rotateMinimapCVar->m_intValue) {
                point = C3Vector(info->m_rotated.x, info->m_rotated.y, 0.0f);
                point *= rotation;
                info->m_position.x = point.x;
                info->m_position.y = point.y;
            }

            uint16_t base = static_cast<uint16_t>(s_blipVertices.m_count);
            C3Vector vertices[4];

            for (uint32_t j = 0; j < 4; j++) {
                vertices[j].x = CGMinimapFrame::s_blipQuads[0][j].x * this->m_layoutScale + point.x;
                vertices[j].y = CGMinimapFrame::s_blipQuads[0][j].y * this->m_layoutScale + point.y;
                vertices[j].z = 0.0f;
            }

            s_blipVertices.Add(4, vertices);
            uint16_t indices[6];
            indices[2] = base + 2;
            indices[3] = base + 2;
            indices[0] = base;
            indices[1] = base + 1;
            indices[4] = base + 1;
            indices[5] = base + 3;
            s_blipIndices.Add(6, indices);
            s_blipTexCoords2.Add(4, s_classBlipTexCoords[list]);
        }
    }

    if (!s_blipVertices.m_count) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(s_blipTexture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    static CImVector s_color(0xFF, 0xFF, 0xFF, 0xFF);
    static CImVector s_otherFloorColor(0xB0, 0xB0, 0xB0, 0xFF);
    static C3Vector s_normal(0.0f, 0.0f, 1.0f);
    GxPrimLockVertexPtrs(s_blipVertices.m_count, s_blipVertices.m_data, sizeof(C3Vector), &s_normal, 0, otherFloor ? &s_otherFloorColor : &s_color, 0, nullptr, 0, s_blipTexCoords2.m_data, sizeof(C2Vector), nullptr, 0);
    GxPrimLockIndexPtr(GxPrim_Triangles, s_blipIndices.m_count, s_blipIndices.m_data);
    GxDrawLockedElements();
    GxPrimUnlockVertexPtrs();
    s_blipVertices.SetCount(0);
    s_blipTexCoords2.SetCount(0);
    s_blipIndices.m_count = 0;
}

// VERIFY: a016d4c5 manual
// OFFSET: 0x00580720
void CGMinimapFrame::RenderVehicleBlips(const C44Matrix& rotation) {
    for (uint32_t list = MINIMAP_VEHICLE_GROUND_UNOCCUPIED; list < MINIMAP_NUM_TYPES; list++) {
        CTexture* texture = s_vehicleTextures[list - MINIMAP_VEHICLE_GROUND_UNOCCUPIED];

        if (!s_objInfo[list].m_count || !texture) {
            continue;
        }

        for (uint32_t i = 0; i < s_objInfo[list].m_count; i++) {
            OBJINFO* info = &s_objInfo[list].m_data[i];

            if (info->m_otherFloor) {
                continue;
            }

            C3Vector point(info->m_position.x, info->m_position.y, 0.0f);

            if (g_rotateMinimapCVar->m_intValue) {
                point = C3Vector(info->m_rotated.x, info->m_rotated.y, 0.0f);
                point *= rotation;
                info->m_position.x = point.x;
                info->m_position.y = point.y;
            }

            uint16_t base = static_cast<uint16_t>(s_blipVertices.m_count);
            C3Vector vertices[4];

            for (uint32_t j = 0; j < 4; j++) {
                vertices[j].x = CGMinimapFrame::s_blipQuads[1][j].x * this->m_layoutScale + point.x;
                vertices[j].y = CGMinimapFrame::s_blipQuads[1][j].y * this->m_layoutScale + point.y;
                vertices[j].z = 0.0f;
            }

            s_blipVertices.Add(4, vertices);
            uint16_t indices[6];
            indices[2] = base + 2;
            indices[3] = base + 2;
            indices[0] = base;
            indices[1] = base + 1;
            indices[4] = base + 1;
            indices[5] = base + 3;
            s_blipIndices.Add(6, indices);

            float facing = info->m_facing - s_arrowRotation;
            float cosine = std::cos(facing);
            float sine = std::sin(facing);
            C2Vector texCoords[4];
            texCoords[0].x = sine + 0.5f;
            texCoords[0].y = 0.5f - cosine;
            texCoords[1].x = 0.5f - cosine;
            texCoords[1].y = 0.5f - sine;
            texCoords[2].x = cosine + 0.5f;
            texCoords[2].y = sine + 0.5f;
            texCoords[3].x = 0.5f - sine;
            texCoords[3].y = cosine + 0.5f;
            s_blipTexCoords2.Add(4, texCoords);
        }

        if (!s_blipVertices.m_count) {
            return;
        }

        CGxTex* gxTex = TextureGetGxTex(texture, 0, nullptr);

        if (!gxTex) {
            return;
        }

        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
        static CImVector s_color(0xFF, 0xFF, 0xFF, 0xFF);
        static C3Vector s_normal(0.0f, 0.0f, 1.0f);
        GxPrimLockVertexPtrs(s_blipVertices.m_count, s_blipVertices.m_data, sizeof(C3Vector), &s_normal, 0, &s_color, 0, nullptr, 0, s_blipTexCoords2.m_data, sizeof(C2Vector), nullptr, 0);
        GxPrimLockIndexPtr(GxPrim_Triangles, s_blipIndices.m_count, s_blipIndices.m_data);
        GxDrawLockedElements();
        GxPrimUnlockVertexPtrs();
        s_blipVertices.SetCount(0);
        s_blipTexCoords2.SetCount(0);
        s_blipIndices.m_count = 0;
    }
}

// VERIFY: 6699a7c8 manual
// OFFSET: 0x00580AE0
void CGMinimapFrame::RenderPartyBlips(const C44Matrix& rotation) {
    static C3Vector s_normal(0.0f, 0.0f, 1.0f);

    if (!s_groupArrowTexture) {
        return;
    }

    MinimapGetPartyMembers(s_partyMembers);

    for (uint32_t i = 0; i < 5; i++) {
        if (!s_partyMembers[i].m_offMap) {
            continue;
        }

        float facing = s_partyMembers[i].m_facing;
        float cosine = std::cos(facing);
        float sine = std::sin(facing);
        C3Vector point;
        point.x = this->GetWidth() * this->m_layoutScale * 0.5f + this->m_layoutScale * CGMinimapFrame::s_blipSize4 * sine * -0.8f;
        point.y = this->GetWidth() * this->m_layoutScale * 0.5f + this->m_layoutScale * CGMinimapFrame::s_blipSize4 * cosine * 0.8f;
        point.z = 0.0f;

        if (g_rotateMinimapCVar->m_intValue) {
            facing = facing - this->m_facing;
            point *= rotation;
        }

        s_partyArrowPositions[i].x = point.x;
        s_partyArrowPositions[i].y = point.y;
        uint16_t base = static_cast<uint16_t>(s_blipVertices.m_count);
        C3Vector vertices[4];

        for (uint32_t j = 0; j < 4; j++) {
            vertices[j].x = CGMinimapFrame::s_blipQuads[2][j].x * this->m_layoutScale + point.x;
            vertices[j].y = CGMinimapFrame::s_blipQuads[2][j].y * this->m_layoutScale + point.y;
            vertices[j].z = CGMinimapFrame::s_blipQuads[2][j].z * this->m_layoutScale + point.z;
        }

        s_blipVertices.Add(4, vertices);
        uint16_t indices[6];
        indices[2] = base + 2;
        indices[3] = base + 2;
        indices[0] = base;
        indices[1] = base + 1;
        indices[4] = base + 1;
        indices[5] = base + 3;
        s_blipIndices.Add(6, indices);

        facing = facing - s_arrowRotation;
        cosine = std::cos(facing);
        sine = std::sin(facing);
        C2Vector texCoords[4];
        texCoords[0].x = sine + 0.5f;
        texCoords[0].y = 0.5f - cosine;
        texCoords[1].x = 0.5f - cosine;
        texCoords[1].y = 0.5f - sine;
        texCoords[2].x = cosine + 0.5f;
        texCoords[2].y = sine + 0.5f;
        texCoords[3].x = 0.5f - sine;
        texCoords[3].y = cosine + 0.5f;
        s_blipTexCoords2.Add(4, texCoords);
    }

    if (!s_blipVertices.m_count) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(s_groupArrowTexture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    static CImVector s_color(0xFF, 0xFF, 0xFF, 0xFF);
    GxPrimLockVertexPtrs(s_blipVertices.m_count, s_blipVertices.m_data, sizeof(C3Vector), &s_normal, 0, &s_color, 0, nullptr, 0, s_blipTexCoords2.m_data, sizeof(C2Vector), nullptr, 0);
    GxPrimLockIndexPtr(GxPrim_Triangles, s_blipIndices.m_count, s_blipIndices.m_data);
    GxDrawLockedElements();
    GxPrimUnlockVertexPtrs();
    s_blipVertices.SetCount(0);
    s_blipTexCoords2.SetCount(0);
    s_blipIndices.m_count = 0;
}

// VERIFY: d41dccb8 manual
// OFFSET: 0x00580EC0
void CGMinimapFrame::RenderRaidArrows(const C44Matrix& rotation) {
    if (!s_groupArrowTexture) {
        return;
    }

    MinimapGetRaidMembers(s_raidMembers);

    for (uint32_t i = 0; i < 40; i++) {
        if (!s_raidMembers[i].m_offMap) {
            continue;
        }

        float facing = s_raidMembers[i].m_facing;
        float cosine = std::cos(facing);
        float sine = std::sin(facing);
        C3Vector point;
        point.x = this->GetWidth() * this->m_layoutScale * 0.5f + CGMinimapFrame::s_blipSize4 * sine * this->m_layoutScale * -0.8f;
        point.y = this->GetHeight() * this->m_layoutScale * 0.5f + CGMinimapFrame::s_blipSize4 * cosine * this->m_layoutScale * 0.8f;
        point.z = 0.0f;

        if (g_rotateMinimapCVar->m_intValue) {
            facing = facing - this->m_facing;
            point *= rotation;
        }

        s_raidArrowPositions[i].x = point.x;
        s_raidArrowPositions[i].y = point.y;
        uint16_t base = static_cast<uint16_t>(s_blipVertices.m_count);
        C3Vector vertices[4];

        for (uint32_t j = 0; j < 4; j++) {
            vertices[j].x = CGMinimapFrame::s_blipQuads[2][j].x * this->m_layoutScale + point.x;
            vertices[j].y = CGMinimapFrame::s_blipQuads[2][j].y * this->m_layoutScale + point.y;
            vertices[j].z = CGMinimapFrame::s_blipQuads[2][j].z * this->m_layoutScale + point.z;
        }

        s_blipVertices.Add(4, vertices);
        uint16_t indices[6];
        indices[2] = base + 2;
        indices[3] = base + 2;
        indices[0] = base;
        indices[1] = base + 1;
        indices[4] = base + 1;
        indices[5] = base + 3;
        s_blipIndices.Add(6, indices);

        facing = facing - s_arrowRotation;
        cosine = std::cos(facing);
        sine = std::sin(facing);
        C2Vector texCoords[4];
        texCoords[0].x = sine + 0.5f;
        texCoords[0].y = 0.5f - cosine;
        texCoords[1].x = 0.5f - cosine;
        texCoords[1].y = 0.5f - sine;
        texCoords[2].x = cosine + 0.5f;
        texCoords[2].y = sine + 0.5f;
        texCoords[3].x = 0.5f - sine;
        texCoords[3].y = cosine + 0.5f;
        s_blipTexCoords2.Add(4, texCoords);
    }

    if (!s_blipVertices.m_count) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(s_groupArrowTexture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    static C3Vector s_normal(0.0f, 0.0f, 1.0f);
    static CImVector s_color(0xFF, 0xFF, 0xFF, 0xFF);
    GxPrimLockVertexPtrs(s_blipVertices.m_count, s_blipVertices.m_data, sizeof(C3Vector), &s_normal, 0, &s_color, 0, nullptr, 0, s_blipTexCoords2.m_data, sizeof(C2Vector), nullptr, 0);
    GxPrimLockIndexPtr(GxPrim_Triangles, s_blipIndices.m_count, s_blipIndices.m_data);
    GxDrawLockedElements();
    GxPrimUnlockVertexPtrs();
    s_blipVertices.SetCount(0);
    s_blipTexCoords2.SetCount(0);
    s_blipIndices.m_count = 0;
}

// VERIFY: b168ac00 manual
// OFFSET: 0x0057BD10
void CGMinimapFrame::RenderInsideSortQuads(MINIMAPTILE** head) {
    for (uint32_t i = 0; i < 256; i++) {
        MINIMAPTILE* tile = &s_tiles[i];

        if (!(tile->m_flags & 2) || !tile->m_texture) {
            continue;
        }

        MINIMAPTILE** link = head;

        for (MINIMAPTILE* current = *head; current; current = current->m_nextSorted) {
            if (current->m_depth >= tile->m_depth) {
                break;
            }

            link = &current->m_nextSorted;
        }

        tile->m_nextSorted = *link;
        *link = tile;
    }
}

// VERIFY: 3e4f29c8 manual
// OFFSET: 0x0057E7F0
void CGMinimapFrame::RenderInsideQuad(MINIMAPTILE* tile, const CImVector* color) {
    C3Vector vertices[4];
    vertices[0] = C3Vector(tile->m_bounds.b.x, tile->m_bounds.b.y, 0.0f);
    vertices[1] = C3Vector(tile->m_bounds.t.x, tile->m_bounds.b.y, 0.0f);
    vertices[2] = C3Vector(tile->m_bounds.t.x, tile->m_bounds.t.y, 0.0f);
    vertices[3] = C3Vector(tile->m_bounds.b.x, tile->m_bounds.t.y, 0.0f);
    CGxTex* gxTex = TextureGetGxTex(tile->m_texture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    CGxTexParms parms;
    GxTexParameters(gxTex, parms);
    float halfX = 0.5f / parms.m_width;
    float halfY = 0.5f / parms.m_height;
    C2Vector texCoords[4];
    texCoords[0] = C2Vector(halfX, 1.0f - halfY);
    texCoords[1] = C2Vector(1.0f - halfX, 1.0f - halfY);
    texCoords[2] = C2Vector(1.0f - halfX, halfY);
    texCoords[3] = C2Vector(halfX, halfY);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    GxRsSet(GxRs_DepthTest, 0);
    GxPrimLockVertexPtrs(4, vertices, sizeof(C3Vector), nullptr, 0, color, 0, nullptr, 0, texCoords, sizeof(C2Vector), nullptr, 0);
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_insideQuadIndices);
    GxPrimUnlockVertexPtrs();
}

// VERIFY: 9d3272fd manual
// OFFSET: 0x00581290
void CGMinimapFrame::RenderInsideTexture(const CImVector* color) {
    C44Matrix savedProjection;
    savedProjection = g_theGxDevicePtr->m_projection;
    C44Matrix ortho;
    float extent = s_indoor.m_radius * 1.5f;
    GxuXformCreateOrtho(-extent, extent, -extent, extent, -2500.0f, 2500.0f, ortho);
    g_theGxDevicePtr->XformSetProjection(ortho);

    C44Matrix savedView;
    CGxMatrixStack& viewStack = g_theGxDevicePtr->m_xforms[GxXform_View];
    savedView = viewStack.m_mtx[viewStack.m_level];
    C44Matrix identity;
    g_theGxDevicePtr->XformSetView(identity);

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsInit();
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_BlendingMode, 1);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    g_theGxDevicePtr->XformPush(GxXform_World);
    CGxMatrixStack& world = g_theGxDevicePtr->m_xforms[GxXform_World];
    world.Identity();
    world.m_dirty = 1;
    world.m_flags[world.m_level] &= ~1u;
    world.m_mtx[world.m_level] *= s_indoor.m_matrix;
    C3Vector center = s_indoor.m_center * s_indoor.m_invMatrix;
    GxXformTranslate(GxXform_World, C3Vector(-center.x, -center.y, -center.z));

    MINIMAPTILE* head = nullptr;
    CGMinimapFrame::RenderInsideSortQuads(&head);

    for (MINIMAPTILE* tile = head; tile; tile = tile->m_nextSorted) {
        CGMinimapFrame::RenderInsideQuad(tile, color);
    }

    GxXformPop(GxXform_World);
    g_theGxDevicePtr->XformSetView(savedView);
    g_theGxDevicePtr->XformSetProjection(savedProjection);
    CShaderEffect::UpdateProjMatrix();
    g_theGxDevicePtr->RsPop();
}

// VERIFY: e449b7d7 manual
// OFFSET: 0x00581CD0
void CGMinimapFrame::MinimapTextureCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command != GxTex_Latch) {
        return;
    }

    float minX = g_theGxDevicePtr->m_viewport.m_x.m_l;
    float maxX = g_theGxDevicePtr->m_viewport.m_x.m_h;
    float minY = g_theGxDevicePtr->m_viewport.m_y.m_l;
    float maxY = g_theGxDevicePtr->m_viewport.m_y.m_h;
    float minZ = g_theGxDevicePtr->m_viewport.m_z.m_l;
    float maxZ = g_theGxDevicePtr->m_viewport.m_z.m_h;
    CGxTex* target = TextureGetGxTex(s_indoor.m_texture, 1, nullptr);
    GxRsSet(GxRs_ScissorTest, 0);
    g_theGxDevicePtr->DeviceSetRenderTarget(GxBuffers_Color, target, 0);
    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    uint8_t alpha;

    if (CGMinimapFrame::s_minimapFrame) {
        alpha = CGMinimapFrame::s_minimapFrame->m_alpha * CGMinimapFrame::s_minimapFrame->m_parentAlpha / 255;
    } else {
        alpha = 0xFF;
    }

    GxSceneClear(GxBufferBit_Color, CImVector(0, 0, 0, alpha));
    CImVector color(0xFF, 0xFF, 0xFF, alpha);
    CGMinimapFrame::RenderInsideTexture(&color);
    GxRsSet(GxRs_ScissorTest, 0);
    g_theGxDevicePtr->DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

// VERIFY: c51a3f7b manual
// OFFSET: 0x00581740
void CGMinimapFrame::RenderInside(float scale, const C2Vector* center, const CImVector* color) {
    g_theGxDevicePtr->RsSet(GxRs_VertexShader, static_cast<void*>(nullptr));
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, static_cast<void*>(nullptr));
    C44Matrix savedProjection;
    savedProjection = g_theGxDevicePtr->m_projection;
    C44Matrix ortho;
    float half = scale * 0.5f;
    GxuXformCreateOrtho(-half, half, -half, half, -1000.0f, 1000.0f, ortho);
    g_theGxDevicePtr->XformSetProjection(ortho);
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_TextureShader0, 1);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(s_indoor.m_texture, 1, nullptr));
    g_theGxDevicePtr->XformPush(GxXform_Tex0);

    C44Matrix texMatrix;
    texMatrix.Translate(C3Vector(0.0f, 1.0f, 0.0f));
    texMatrix.Scale(C3Vector(1.0f, -1.0f, 1.0f));
    texMatrix.Translate(C3Vector(center->x, center->y, 0.0f));
    texMatrix.Scale(0.33333334f);

    if (g_rotateMinimapCVar->m_intValue) {
        texMatrix.RotateAroundZ(this->m_facing);
    }

    GxXformSet(GxXform_Tex0, texMatrix);
    GxRsSet(GxRs_TextureShader1, 1);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(s_maskTexture, 1, nullptr));
    g_theGxDevicePtr->XformPush(GxXform_Tex1);
    GxXformTranslate(GxXform_Tex1, C3Vector(0.0f, 1.0f, 0.0f));
    GxXformScale(GxXform_Tex1, C3Vector(1.0f, -1.0f, 1.0f));
    GxXformTranslate(GxXform_Tex1, C3Vector(0.5f, 0.5f, 0.0f));
    GxXformScale(GxXform_Tex1, C3Vector(0.5f, 0.5f, 0.5f));
    g_theGxDevicePtr->XformPush(GxXform_World);
    GxXformScale(GxXform_World, C3Vector(half, half, half));

    static C3Vector s_positions[4] = { C3Vector(-1.0f, -1.0f, 0.0f), C3Vector(-1.0f, 1.0f, 0.0f), C3Vector(1.0f, 1.0f, 0.0f), C3Vector(1.0f, -1.0f, 0.0f) };
    static C2Vector s_texCoords[4] = { C2Vector(-1.0f, -1.0f), C2Vector(-1.0f, 1.0f), C2Vector(1.0f, 1.0f), C2Vector(1.0f, -1.0f) };

    GxPrimLockVertexPtrs(4, s_positions, sizeof(C3Vector), nullptr, 0, color, 0, nullptr, 0, s_texCoords, sizeof(C2Vector), s_texCoords, sizeof(C2Vector));
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_insideIndices);
    GxPrimUnlockVertexPtrs();
    g_theGxDevicePtr->XformSetProjection(savedProjection);
    CShaderEffect::UpdateProjMatrix();
    GxXformPop(GxXform_World);
    GxXformPop(GxXform_Tex1);
    GxXformPop(GxXform_Tex0);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 2325fb71 manual
// OFFSET: 0x0057E020
static int32_t UpdateObjectInteractIcon(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && (object->m_questStatus == 2 || object->m_questStatus == 3 || object->m_questStatus == 4)) {
        object->UpdateInteractIcon();
    }

    return 1;
}

// VERIFY: b1b39bd3 auto
// OFFSET: 0x0057E070
void CGMinimapFrame::LoadTrackingInfo(const TrackingTypeInfo* info) {
    bool wasQuestTracking = CGMinimapFrame::s_trackingInfo ? CGMinimapFrame::s_trackingInfo->m_type == 3 : false;
    CGMinimapFrame::s_trackingInfo = info;
    CVar* var = CVar::Lookup("minimapTrackedInfo");

    if (var) {
        if (info) {
            var->Set(info->m_name, true, false, false, true);
        } else {
            var->Set("", true, false, false, true);
        }
    }

    bool isQuestTracking = CGMinimapFrame::s_trackingInfo ? CGMinimapFrame::s_trackingInfo->m_type == 3 : false;

    if (wasQuestTracking != isQuestTracking) {
        ClntObjMgrEnumVisibleObjects(UpdateObjectInteractIcon, nullptr);
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_MINIMAP_UPDATE_TRACKING);
}

// VERIFY: 0d1f704a manual
// OFFSET: 0x0057C720
int32_t CGMinimapFrame::OnLayerTrackUpdate(const CMouseEvent& evt) {
    if (!CSimpleFrame::OnLayerTrackUpdate(evt)) {
        bool anyObject = false;

        for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
            if (s_hoverObjects[i]) {
                anyObject = true;
                break;
            }
        }

        if (s_hoverPOIs || s_hoverPOIDirections || s_hoverParty || s_hoverRaid || anyObject) {
            s_hoverPOIs = 0;
            memset(s_hoverObjects, 0, sizeof(s_hoverObjects));
            s_hoverPOIDirections = 0;
            s_hoverParty = 0;
            s_hoverRaid = 0;
            CGGameUI::m_gameTooltip->Hide();
        }

        return 0;
    }

    float mouseX = evt.m_data.m_mousePosition.x;
    float mouseY = evt.m_data.m_mousePosition.y;
    CRect rect;
    this->GetRect(&rect);
    float half = CGMinimapFrame::s_blipHalfSize1;
    uint32_t pois = 0;

    for (uint32_t i = 0; i < s_poiInfo.Count(); i++) {
        float x = s_poiInfo[i].m_position.x + rect.minX;
        float y = s_poiInfo[i].m_position.y + rect.minY;

        if (x - half <= mouseX && x + half >= mouseX && half + y >= mouseY && y - half <= mouseY) {
            pois |= 1 << i;
        }
    }

    uint32_t objects[MINIMAP_NUM_TYPES];
    memset(objects, 0, sizeof(objects));

    for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
        for (uint32_t j = 0; j < s_objInfo[i].Count(); j++) {
            OBJINFO& info = s_objInfo[i][j];
            float x = info.m_position.x + rect.minX;
            float y = info.m_position.y + rect.minY;

            if (x - half > mouseX || x + half < mouseX || half + y < mouseY || y - half > mouseY) {
                continue;
            }

            const char* name;
            CGObject_C* object = ClntObjMgrObjectPtr(info.m_guid, TYPE_OBJECT, __FILE__, __LINE__);

            if (object) {
                name = object->GetObjectName();
            } else {
                WOWGUID requester = WOWGUID();
                NameCache* record = g_nameDBCache.GetRecord(info.m_guid.m_guid, requester, nullptr, nullptr, false);
                name = record ? record->m_name : nullptr;
            }

            if (name) {
                objects[i] |= 1 << j;
            }
        }
    }

    float tolerance = this->m_layoutScale * 0.005f;
    uint32_t directions = 0;

    for (uint32_t i = 0; i < s_poiDirections.Count(); i++) {
        if ((!s_indoor.m_active || s_poiDirections[i].m_alwaysShow) && std::fabs(mouseX - (s_poiArrowPositions[i].x + rect.minX)) < tolerance && std::fabs(mouseY - (s_poiArrowPositions[i].y + rect.minY)) < tolerance) {
            directions |= 1 << i;
        }
    }

    uint32_t party = 0;

    for (uint32_t i = 0; i < 5; i++) {
        if (s_partyMembers[i].m_offMap && std::fabs(mouseX - (s_partyArrowPositions[i].x + rect.minX)) < tolerance && std::fabs(mouseY - (s_partyArrowPositions[i].y + rect.minY)) < tolerance) {
            party |= 1 << i;
        }
    }

    uint64_t raid = 0;

    for (uint32_t i = 0; i < 40; i++) {
        if (s_raidMembers[i].m_offMap && std::fabs(mouseX - (s_raidArrowPositions[i].x + rect.minX)) < tolerance && std::fabs(mouseY - (s_raidArrowPositions[i].y + rect.minY)) < tolerance) {
            raid |= 1ull << i;
        }
    }

    bool objectsChanged = false;

    for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
        if (s_hoverObjects[i] != objects[i]) {
            objectsChanged = true;
            break;
        }
    }

    if (s_hoverPOIs == pois && !objectsChanged && s_hoverPOIDirections == directions && s_hoverParty == party && s_hoverRaid == raid) {
        return 1;
    }

    s_hoverPOIs = pois;
    s_hoverPOIDirections = directions;
    s_hoverParty = party;
    s_hoverRaid = raid;
    bool anyObject = false;

    for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
        s_hoverObjects[i] = objects[i];

        if (objects[i]) {
            anyObject = true;
        }
    }

    int32_t colorblind = 0;
    CVar* colorblindVar = CVar::Lookup("colorblindMode");

    if (colorblindVar && colorblindVar->m_intValue) {
        colorblind = 1;
    }

    CGTooltip* tooltip = CGGameUI::m_gameTooltip;

    if (!tooltip) {
        return 1;
    }

    if (!s_hoverPOIs && !anyObject && !s_hoverPOIDirections && !s_hoverParty && !s_hoverRaid) {
        tooltip->Hide();
        return 1;
    }

    tooltip->SetOwner(this, ANCHOR_CURSOR, 0.0f, 0.0f);
    bool first = true;
    char text[4096];
    text[0] = 0;

    if (s_hoverPOIDirections) {
        for (uint32_t i = 0; i < 3; i++) {
            if (s_hoverPOIDirections & (1 << i)) {
                if (!first) {
                    SStrPack(text, "\n", sizeof(text));
                }

                first = false;
                SStrPack(text, s_poiDirections.m_data[i].m_name, sizeof(text));
            }
        }
    }

    if (s_hoverParty) {
        for (uint32_t i = 0; i < 5; i++) {
            if (s_hoverParty & (1 << i)) {
                if (!first) {
                    SStrPack(text, "\n", sizeof(text));
                }

                first = false;
                SStrPack(text, s_partyMembers[i].m_name, sizeof(text));
            }
        }
    }

    if (s_hoverRaid) {
        for (uint32_t i = 0; i < 40; i++) {
            if (s_hoverRaid & (1ull << i)) {
                if (!first) {
                    SStrPack(text, "\n", sizeof(text));
                }

                first = false;
                SStrPack(text, s_raidMembers[i].m_name, sizeof(text));
            }
        }
    }

    if (s_hoverPOIs) {
        for (uint32_t i = 0; i < s_poiInfo.Count(); i++) {
            if (s_hoverPOIs & (1 << i)) {
                if (!first) {
                    SStrPack(text, "\n", sizeof(text));
                }

                first = false;
                SStrPack(text, s_poiInfo[i].m_name, sizeof(text));
            }
        }
    }

    for (uint32_t i = 0; i < MINIMAP_NUM_TYPES; i++) {
        for (uint32_t j = 0; j < s_objInfo[i].Count(); j++) {
            if (!(s_hoverObjects[i] & (1 << j))) {
                continue;
            }

            const char* name;
            CGObject_C* object = ClntObjMgrObjectPtr(s_objInfo[i][j].m_guid, TYPE_OBJECT, __FILE__, __LINE__);

            if (object) {
                name = object->GetObjectName();
            } else {
                WOWGUID requester = WOWGUID();
                NameCache* record = g_nameDBCache.GetRecord(s_objInfo[i][j].m_guid.m_guid, requester, nullptr, nullptr, false);
                name = record ? record->m_name : nullptr;
            }

            if (!name) {
                continue;
            }

            if (!first) {
                SStrPack(text, "\n", sizeof(text));
            }

            first = false;

            if (s_objInfo[i][j].m_otherFloor) {
                SStrPack(text, "|cffb0b0b0", sizeof(text));
            }

            SStrPack(text, name, sizeof(text));

            if (s_objInfo[i][j].m_questGiver && colorblind) {
                uint32_t length = SStrLen(text);
                SStrPrintf(&text[length], sizeof(text) - length, "\n <%s>", FrameScript_GetText("MINIMAP_TRACKING_FLIGHTMASTER", -1, GENDER_NOT_APPLICABLE));
            }

            if (s_objInfo[i][j].m_tracked && object) {
                const char* title = static_cast<CGUnit_C*>(object)->GetUnitTitle();

                if (title) {
                    uint32_t length = SStrLen(text);
                    SStrPrintf(&text[length], sizeof(text) - length, "\n <%s>", title);
                }
            }

            if (s_objInfo[i][j].m_otherFloor) {
                SStrPack(text, "|r", sizeof(text));
            }
        }
    }

    tooltip->AddLine(text, nullptr, 0);
    tooltip->Show();
    return 1;
}

// VERIFY: b4f3e5a1 auto
// OFFSET: 0x007F3AE0
void MinimapSetZoom(uint32_t zoom) {
    uint8_t indoors = s_isIndoors;
    int32_t* current = indoors ? &s_indoorZoom : &s_outdoorZoom;
    int32_t previous = *current;
    *current = zoom >= 5 ? 5 : zoom;

    if (previous != *current) {
        s_updateFlags |= 0x1;
        (indoors ? g_minimapInsideZoomCVar : g_minimapZoomCVar)->Set(*current, true, false, false, true);
    }
}

// VERIFY: a04b4793 auto
// OFFSET: 0x007F3B40
int32_t MinimapGetZoom() {
    return s_isIndoors ? s_indoorZoom : s_outdoorZoom;
}

// VERIFY: 07a4c72b auto
// OFFSET: 0x007F3B60
int32_t MinimapGetZoomLevels() {
    return 6;
}

// VERIFY: 0f410ab6 auto
// OFFSET: 0x007F4810
void RefreshWorldStatePOIs(uint32_t state) {
    for (uint32_t i = 0; i < s_numPOIs; i++) {
        AreaPOIRec* poi = s_pois.m_data[i];

        if (0.0f != poi->m_pos[0] && 0.0f != poi->m_pos[1] && poi->m_worldStateID == static_cast<int32_t>(state)) {
            UpdatePointsOfInterest();
            s_poisChanged = 1;
            return;
        }
    }
}

// VERIFY: ac080634 manual
// OFFSET: 0x0057BD90
void CGMinimapFrame::ShutdownGame() {
    if (s_maskTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_maskTexture));
    }

    s_maskTexture = nullptr;

    if (s_iconTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_iconTexture));
    }

    s_iconTexture = nullptr;

    if (s_blipTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_blipTexture));
    }

    s_blipTexture = nullptr;

    if (s_classBlipTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_classBlipTexture));
    }

    s_classBlipTexture = nullptr;

    for (uint32_t i = 0; i < 7; i++) {
        HandleClose(reinterpret_cast<HOBJECT>(s_vehicleTextures[i]));
        s_vehicleTextures[i] = nullptr;
    }

    if (s_staticPOIArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_staticPOIArrowTexture));
    }

    s_staticPOIArrowTexture = nullptr;

    if (s_poiArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_poiArrowTexture));
    }

    s_poiArrowTexture = nullptr;

    if (s_corpseArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_corpseArrowTexture));
    }

    s_corpseArrowTexture = nullptr;

    if (s_groupArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_groupArrowTexture));
    }

    s_groupArrowTexture = nullptr;
}
