#include "PlayerName.h"
#include <typeinfo>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Game/GameClient/WorldText.h"
#include <cstring>
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gxu/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Os/TimeManager.h"

// STATIC: 0x00D380A0
uint32_t s_nameMask;

// STATIC: 0x00D380A4
static uint32_t s_lastRenderFrame;

// STATIC: 0x00D380A8
static CGxStringBatch* s_playerNameBatch;

// STATIC: 0x00D380AC
static CGxFont* s_playerNameFont;

// STATIC: 0x00D380B0
static CTexture* s_playerNameIcons;

// STATIC: 0x00D380B4
static CDataAllocator s_freePlayerNames(sizeof(PLAYERNAMEDESC), 256);

// STATIC: 0x00AF46AC
static C3Vector s_iconPositions[4] = {
    C3Vector(-0.5f, 1.0f, 0.0f),
    C3Vector(0.5f, 1.0f, 0.0f),
    C3Vector(0.5f, 0.0f, 0.0f),
    C3Vector(-0.5f, 0.0f, 0.0f),
};

// STATIC: 0x00AF46DC
static C2Vector s_iconTexCoords[4] = {
    C2Vector(0.0f, 0.0f),
    C2Vector(0.25f, 0.0f),
    C2Vector(0.25f, 0.25f),
    C2Vector(0.0f, 0.25f),
};

// STATIC: 0x00AF46FC
TSExplicitList<PLAYERNAMEDESC, offsetof(PLAYERNAMEDESC, m_link)> s_playerNames;

// VERIFY: e90bacd6 auto
// OFFSET: 0x007E53A0
void PlayerNameShutdown() {
    if (s_playerNameFont) {
        GxuFontDestroyFont(s_playerNameFont);
    }

    s_playerNameFont = nullptr;
    GxuFontDestroyBatch(s_playerNameBatch);
    s_playerNameBatch = nullptr;
    s_freePlayerNames.Clear(typeid(PLAYERNAMEDESC).raw_name(), SLOG_OBJECT, 1);
    ConsoleCommandUnregister("PlayerNames");

    if (s_playerNameIcons) {
        HandleClose(reinterpret_cast<HOBJECT>(s_playerNameIcons));
        s_playerNameIcons = nullptr;
    }
}

// VERIFY: 2a265008 manual
// OFFSET: 0x007E64D0
void PlayerNameInitialize() {
    PlayerNameShutdown();

    const char* fontName;

    if (FrameScript_GetVariable("UNIT_NAME_FONT", fontName)) {
        GxuFontCreateFont(fontName, 0.99f, s_playerNameFont, 4);
    }

    s_playerNameBatch = GxuFontCreateBatch(true, true);

    CStatus status;
    s_playerNameIcons = TextureCreate("Interface\\TargetingFrame\\UI-RaidTargetingIcons", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 0);
}

// VERIFY: 6b580d5d auto
// OFFSET: 0x007E50F0
void PlayerNameTriggerColorUpdate(PLAYERNAMEDESC* desc) {
    if (desc) {
        desc->m_flags |= 0x2;
    }
}

// VERIFY: 6e7f45e2 manual
// OFFSET: 0x007E6390
void PlayerNameUpdateDrawCallback(PLAYERNAMEDESC* desc) {
    if (!desc) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(desc->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    CM2Model* model = object->m_worldModel;

    if (!model) {
        return;
    }

    desc->m_renderFrame = s_lastRenderFrame;
    uint32_t raidTarget = GetRaidTargetIndexFromGuid(desc->m_guid);

    if (object->ShouldRenderObjectName(s_nameMask) || PLAYERNAMEDESC::ShouldRender(object, raidTarget)) {
        if (!(desc->m_flags & 0x8)) {
            desc->m_flags |= 0x8;
            object->ObjectNameVisibilityChanged();
        }

        model->m_fFlags &= ~0x20;
        model->m_drawCallback = &PLAYERNAMEDESC::RenderIfVisible;
        model->m_drawArg = desc;
    } else {
        model->m_fFlags |= 0x20;
        model->m_drawCallback = nullptr;
        model->m_drawArg = nullptr;

        if (desc->m_flags & 0x8) {
            desc->m_flags &= ~0x8;
            object->ObjectNameVisibilityChanged();
        }
    }
}

// VERIFY: 6a232d70 auto
// OFFSET: 0x007E5120
void PlayerNameUpdateEarly() {
    s_lastRenderFrame++;
}

// VERIFY: fb153d0c auto
// OFFSET: 0x007E5130
void PlayerNameTriggerNameRegenerate(PLAYERNAMEDESC* desc) {
    if (desc) {
        desc->m_flags |= 0x1;
    }
}

// VERIFY: 04249170 auto
// OFFSET: 0x007E5150
bool PlayerNameShowing(PLAYERNAMEDESC* desc) {
    return desc && (desc->m_flags & 0x8);
}

// VERIFY: d71173c9 manual
// OFFSET: 0x007E5580
void PlayerNameUpdateLate() {
    for (PLAYERNAMEDESC* desc = s_playerNames.Head(); desc; desc = s_playerNames.Next(desc)) {
        if (desc->m_renderFrame == s_lastRenderFrame || (desc->m_flags & 4)) {
            continue;
        }

        for (int32_t i = 3; i >= 0; i--) {
            if (desc->m_worldText[i]) {
                WorldTextDelete(desc->m_worldText[i]);
                desc->m_worldText[i] = nullptr;
            }
        }
    }
}

// INLINED
inline PLAYERNAMEDESC::PLAYERNAMEDESC() {
    this->m_string = nullptr;
    this->m_zOffset = 0.0f;
    this->m_guid.m_guid = 0;
    this->m_flags = 3;
    this->m_renderFrame = s_lastRenderFrame;
    this->m_worldText[0] = nullptr;
    this->m_worldText[1] = nullptr;
    this->m_worldText[2] = nullptr;
    this->m_worldText[3] = nullptr;
}

// VERIFY: 118079b2 manual
// OFFSET: 0x007E54D0
PLAYERNAMEDESC::~PLAYERNAMEDESC() {
    if (this->m_string) {
        GxuFontDestroyString(this->m_string);
    }

    for (int32_t i = 3; i >= 0; i--) {
        if (this->m_worldText[i]) {
            WorldTextDelete(this->m_worldText[i]);
        }
    }
}

// VERIFY: 561a729f manual
// OFFSET: 0x007E5F60
PLAYERNAMEDESC* PlayerNameCreate(const WOWGUID& guid) {
    if (!guid) {
        return nullptr;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object || !object->m_worldModel) {
        return nullptr;
    }

    PLAYERNAMEDESC* desc = s_freePlayerNames.New<PLAYERNAMEDESC>(0);
    desc->m_guid = guid;
    s_playerNames.LinkToTail(desc);

    return desc;
}

// VERIFY: c871fae9 manual
// OFFSET: 0x007E6320
void PlayerNameDelete(PLAYERNAMEDESC* desc) {
    if (!desc) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(desc->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        CM2Model* model = object->m_worldModel;

        if (model) {
            model->m_fFlags |= 0x20;
            model->m_drawCallback = nullptr;
            model->m_drawArg = nullptr;
        }
    }

    desc->~PLAYERNAMEDESC();
    s_freePlayerNames.PutData(desc, nullptr, 0);
}

// VERIFY: f8567035 manual
// OFFSET: 0x007E5340
int32_t PLAYERNAMEDESC::ShouldRender(CGObject_C* object, uint32_t raidTarget) {
    int32_t show = 1;

    if (object && (object->m_obj->m_type & TYPE_UNIT)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(object);
        show = !unit->IsDeadOrGhost() && !unit->m_namePlateFrame;
    }

    return CGGameUI::m_visible && raidTarget < 8 && show;
}

// VERIFY: 64e128ac manual
// OFFSET: 0x007E5420
float PlayerNameComputeScale(CGObject_C* object) {
    float height;

    if (object->m_obj->m_type & TYPE_UNIT) {
        height = static_cast<CGUnit_C*>(object)->GetStandHeight();
    } else if (object->m_worldModel && object->m_worldModel->IsLoaded(0, 0) && object->m_worldModel->HasAttachment(18)) {
        CM2Model* model = object->m_worldModel;
        float attachZ = model->GetAttachmentPosition(18).z;
        height = attachZ - model->GetWorldPosition().z;
    } else {
        height = object->m_height * object->m_scale * 1.25f;
    }

    if (height <= 4.0f) {
        return 1.0f;
    }

    return height * 0.25f * 1.5f;
}

// VERIFY: 90af9757 manual
// OFFSET: 0x007E52A0
static uint8_t PlayerNameIconAlpha(const C3Vector& position) {
    CGCamera* camera = CGWorldFrame::s_currentWorldFrame->m_camera;

    if (!camera) {
        return 0xFF;
    }

    float dy = position.y - camera->m_position.y;
    float dz = position.z - camera->m_position.z;
    float dx = position.x - camera->m_position.x;
    float distance = sqrtf(dx * dx + dz * dz + dy * dy);

    if (distance >= 8.0f) {
        return 0xFF;
    }

    if (distance > 4.0f) {
        return static_cast<uint8_t>((distance - 4.0f) * 55.25f + 34.0f);
    }

    return 34;
}

// VERIFY: 64e7e257 manual
// OFFSET: 0x007E5640
void PLAYERNAMEDESC::Render() {
    if (!this->m_guid) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(this->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    int32_t showName = object->ShouldRenderObjectName(s_nameMask);
    float lineHeight = PlayerNameComputeScale(object) * 0.2f;
    C3Vector position = object->GetNamePosition();

    if (showName) {
        if (this->m_flags & 0x2) {
            this->m_flags &= ~0x2;
            object->GetSelectionHighlightColor(&this->m_color);

            if (this->m_string && !(this->m_flags & 0x1)) {
                GxuFontSetStringColor(this->m_string, this->m_color);
            }
        }

        if (this->m_string && (this->m_flags & 0x1)) {
            GxuFontDestroyString(this->m_string);
            this->m_zOffset = 0.0f;
            this->m_string = nullptr;
        }

        if (!this->m_string) {
            char text[1024] = {};
            int32_t lines = object->UpdateObjectNameString(s_nameMask, text, sizeof(text));
            this->m_zOffset = lines * lineHeight;

            if (s_playerNameFont && text[0]) {
                C3Vector origin(0.0f, 0.0f, 1.0f);
                GxuFontCreateString(s_playerNameFont, LanguageProcess(text), lineHeight, origin, 100000.0f, 100000.0f, 0.0f, this->m_string, GxVJ_Bottom, GxHJ_Center, 200, this->m_color, 0.0f, 1.0f);
            }

            this->m_flags &= ~0x1;
        }

        if (this->m_string) {
            GxuFontAddToBatch(s_playerNameBatch, this->m_string);
            position.z = this->m_zOffset + position.z;
            GxuFontSetStringPosition(this->m_string, position);
            GxuFontRenderBatch(s_playerNameBatch);
        }
    } else if (this->m_string) {
        GxuFontDestroyString(this->m_string);
    }

    uint32_t raidTarget = GetRaidTargetIndexFromGuid(this->m_guid);

    if (!PLAYERNAMEDESC::ShouldRender(object, raidTarget)) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(s_playerNameIcons, 0, nullptr);

    if (!gxTex) {
        return;
    }

    if (showName) {
        position.z = position.z + lineHeight;
    }

    C44Matrix view;
    GxXformView(view);
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);

    uint8_t alpha = PlayerNameIconAlpha(position);

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), 4);
    CGxVertexPCT* vertices = reinterpret_cast<CGxVertexPCT*>(g_theGxDevicePtr->BufLock(vertexBuf));
    float u = static_cast<int32_t>(raidTarget & 3) * 0.25f;
    float v = 0.25f * static_cast<int32_t>(raidTarget >> 2);

    vertices[0].p = s_iconPositions[0];
    vertices[0].tc.x = s_iconTexCoords[0].x + u;
    vertices[0].tc.y = s_iconTexCoords[0].y + v;
    vertices[0].c = CImVector(0xFF, 0xFF, 0xFF, alpha);
    vertices[1].p = s_iconPositions[1];
    vertices[1].tc.x = s_iconTexCoords[1].x + u;
    vertices[1].tc.y = s_iconTexCoords[1].y + v;
    vertices[1].c = CImVector(0xFF, 0xFF, 0xFF, alpha);
    vertices[2].p = s_iconPositions[2];
    vertices[2].tc.x = s_iconTexCoords[2].x + u;
    vertices[2].tc.y = s_iconTexCoords[2].y + v;
    vertices[2].c = CImVector(0xFF, 0xFF, 0xFF, alpha);
    vertices[3].p = s_iconPositions[3];
    vertices[3].tc.x = s_iconTexCoords[3].x + u;
    vertices[3].tc.y = s_iconTexCoords[3].y + v;
    vertices[3].c = CImVector(0xFF, 0xFF, 0xFF, alpha);

    g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
    vertexBuf->m_hasData = 1;
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);

    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), 6);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 3;
    indices[3] = 3;
    indices[4] = 1;
    indices[5] = 2;
    g_theGxDevicePtr->BufUnlock(indexBuf, 0);
    indexBuf->m_hasData = 1;
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);

    C44Matrix world;
    world.Translate(position * view);
    world *= view.Inverse(view.Determinant());

    CGxMatrixStack& worldStack = g_theGxDevicePtr->m_xforms[GxXform_World];
    worldStack.m_dirty = 1;
    worldStack.m_flags[worldStack.m_level] &= ~CGxMatrixStack::F_Identity;
    worldStack.m_mtx[worldStack.m_level] = world;

    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = 6;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 3;
    g_theGxDevicePtr->Draw(&batch, 1);

    CGxMatrixStack& popStack = g_theGxDevicePtr->m_xforms[GxXform_World];

    if (popStack.m_level) {
        popStack.m_level--;
    }

    popStack.m_dirty = 1;
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 090254ae manual
// OFFSET: 0x007E6090
void PLAYERNAMEDESC::RenderIfVisible(CM2Model* model, const CM2Lighting* lighting, void* arg) {
    PLAYERNAMEDESC* desc = static_cast<PLAYERNAMEDESC*>(arg);

    if (!desc) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(desc->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && object->m_worldModel->IsDrawable(0, 0)) {
        desc->Render();
    }
}

// VERIFY: c2e11c04 manual
// OFFSET: 0x007E50A0
void PLAYERNAMEDESC::DeleteWorldText(uint32_t type) {
    for (int32_t i = 3; i >= 0; i--) {
        if (this->m_worldText[i] && (type == WT_ALL || WorldTextGetType(this->m_worldText[i]) == type)) {
            WorldTextDelete(this->m_worldText[i]);
            this->m_worldText[i] = nullptr;
        }
    }
}

// VERIFY: 047b8567 auto
// OFFSET: 0x007E5100
void PlayerNameDeleteText(PLAYERNAMEDESC* desc, uint32_t type) {
    if (desc) {
        desc->DeleteWorldText(type);
    }
}

// VERIFY: e9a4b925 manual
// OFFSET: 0x007E5170
void PLAYERNAMEDESC::UpdateWorldText(uint32_t time) {
    CGObject_C* object = ClntObjMgrObjectPtr(this->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    if (!(this->m_flags & 0x4) && !object->m_worldModel->IsDrawable(0, 0)) {
        return;
    }

    for (int32_t i = 3; i >= 0; i--) {
        if (!this->m_worldText[i]) {
            continue;
        }

        CGCamera* camera = CGWorldFrame::s_currentWorldFrame->m_camera;

        if ((this->m_guid == camera->m_targetGUID && camera->IsFirstPersonView()) || !WorldTextUpdate(this->m_worldText[i], time)) {
            WorldTextDelete(this->m_worldText[i]);
            this->m_worldText[i] = nullptr;
        }
    }
}

// VERIFY: 8aeccf6a manual
// OFFSET: 0x007E5230
void PLAYERNAMEDESC::UpdateWorldTextIcons() {
    CGObject_C* object = ClntObjMgrObjectPtr(this->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    if (!(this->m_flags & 0x4) && !object->m_worldModel->IsDrawable(0, 0)) {
        return;
    }

    for (int32_t i = 3; i >= 0; i--) {
        if (this->m_worldText[i]) {
            WorldTextAddIcon(this->m_worldText[i]);
        }
    }
}

// VERIFY: 9e42a91f manual
// OFFSET: 0x007E5550
void PlayerNameTriggerColorUpdateAll() {
    for (PLAYERNAMEDESC* desc = s_playerNames.Head(); desc; desc = s_playerNames.Next(desc)) {
        desc->m_flags |= 0x2;
    }
}

// VERIFY: f804daea manual
// OFFSET: 0x007E5C30
void PLAYERNAMEDESC::CreateWorldText(uint32_t type, const char* text, const CImVector* color, const int32_t* pvpRank) {
    uint32_t slot = 0;

    do {
        if (!this->m_worldText[slot]) {
            break;
        }

        slot++;
    } while (slot < 4);

    if (slot == 4) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(this->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    C3Vector position = object->GetNamePosition();
    position.z = position.z - 0.33333334f;

    C3Vector corners[8];
    World::GetFrustumCorners(corners);

    if (corners[1].x == corners[0].x && corners[1].y == corners[0].y && corners[1].z == corners[0].z) {
        return;
    }

    C3Vector corner;
    corner.x = corners[4].x * 0.1f + corners[5].x * 0.9f;
    corner.y = corners[4].y * 0.1f + corners[5].y * 0.9f;
    corner.z = corners[5].z * 0.9f + corners[4].z * 0.1f;
    corners[5] = corner;
    corner.x = corners[7].x * 0.1f + corners[6].x * 0.9f;
    corner.y = corners[7].y * 0.1f + corners[6].y * 0.9f;
    corner.z = corners[6].z * 0.9f + corners[7].z * 0.1f;
    corners[6] = corner;
    corner.x = corners[0].x * 0.1f + corners[1].x * 0.9f;
    corner.y = corners[0].y * 0.1f + corners[1].y * 0.9f;
    corner.z = corners[1].z * 0.9f + corners[0].z * 0.1f;
    corners[1] = corner;
    corner.x = corners[3].x * 0.1f + corners[2].x * 0.9f;
    corner.y = corners[2].y * 0.9f + corners[3].y * 0.1f;
    corner.z = 0.9f * corners[2].z + 0.1f * corners[3].z;
    corners[2] = corner;

    CWFrustum frustum(corners);
    float topZ = (frustum.planes[0].n.x * position.x + frustum.planes[0].n.y * position.y + frustum.planes[0].d) * (-1.0f / frustum.planes[0].n.z);

    if (position.z > topZ) {
        position.z = topZ;
        position.z = position.z - WorldTextGetAscendDistance(type);
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        CGObject_C* transport = ClntObjMgrObjectPtr(camera->m_relativeTo, TYPE_GAMEOBJECT, __FILE__, __LINE__);

        if (transport) {
            position *= transport->GetMatrix().AffineInverse();
        }
    }

    this->m_worldText[slot] = WorldTextCreate(type, position, text, color, pvpRank);
}

// VERIFY: a695ce6e manual
// OFFSET: 0x007E5FD0
PLAYERNAMEDESC* ObjectNameCreate(const WOWGUID& guid) {
    if (!guid || !ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__)) {
        return nullptr;
    }

    PLAYERNAMEDESC* desc = s_freePlayerNames.New<PLAYERNAMEDESC>(0);
    desc->m_guid = guid;
    desc->m_flags |= 0x4;
    s_playerNames.LinkToTail(desc);

    return desc;
}

// VERIFY: 037184fa auto
// OFFSET: 0x007E6030
void PlayerNameCreateText(PLAYERNAMEDESC* desc, uint32_t type, const char* text, const CImVector* color, const int32_t* pvpRank) {
    if (!desc) {
        return;
    }

    CGCamera* camera = CGWorldFrame::s_currentWorldFrame->m_camera;

    if ((desc->m_guid != camera->m_targetGUID || !camera->IsFirstPersonView()) && !ClntObjMgrGetPlayerType()) {
        desc->CreateWorldText(type, text, color, pvpRank);
    }
}

// VERIFY: 1c9fba67 manual
// OFFSET: 0x007E6480
void PlayerNameUpdateWorldText() {
    uint32_t time = OsGetAsyncTimeMs();

    for (PLAYERNAMEDESC* desc = s_playerNames.Head(); desc; desc = s_playerNames.Next(desc)) {
        desc->UpdateWorldText(time);
        desc->UpdateWorldTextIcons();
    }
}
