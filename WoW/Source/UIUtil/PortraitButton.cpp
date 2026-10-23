#include "PortraitButton.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/BLPFile/blp.h"
#include "Engine/Source/Images/tga.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include <cstring>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00C5CDF0
static CTexture* s_portraitMaskTexture;

// STATIC: 0x00C5CDF4
static CGxTex* s_portraitMaskGxTex;

// STATIC: 0x00C5CDF8
static CTexture* s_portraitDepthTexture;

// STATIC: 0x00C5CDFC
static int32_t s_rtAlphaSupported;

// STATIC: 0x00C5CE00
static PORTRAITCACHE* s_playerPortraitCache;

// STATIC: 0x00C5CE04
static PORTRAITCACHE* s_unitPortraitCache;

// STATIC: 0x00C5CE08
static TSHashTable<PLAYERPORTRAIT, CHashKeyGUID> s_playerPortraits;

// STATIC: 0x00C5CE30
static TSHashTable<UNITPORTRAIT, HASHKEY_DWORD> s_unitPortraits;

// STATIC: 0x00C5CE58
static TSHashTable<ITEMPORTRAIT, HASHKEY_STRI> s_itemPortraits;

// STATIC: 0x00C5CE80
static TSGrowableArray<CTexture*> s_freePortraitTextures;

// STATIC: 0x00AD28E4
static TSList<PORTRAITREQUEST, TSGetLink<PORTRAITREQUEST>> s_pendingPortraits;

// STATIC: 0x00AD28F0
static TSList<PORTRAITREQUEST, TSGetLink<PORTRAITREQUEST>> s_freePortraits;

static void PortraitDepthTextureCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
}

// VERIFY: 9cb5c23b auto
// OFFSET: 0x00616DC0
int32_t RTAlphaSupported() {
    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl && !g_theGxDevicePtr->Caps().m_oglRtAlpha) {
        return 0;
    }

    int32_t supported = 0;
    GxSceneClear(GxBufferBit_Color | GxBufferBit_Depth, CImVector());

    CiRect rect;
    rect.minY = 0;
    rect.minX = 0;
    rect.maxY = 64;
    rect.maxX = 64;

    TSGrowableArray<CImVector> pixels;
    g_theGxDevicePtr->DeviceReadPixels(rect, pixels);

    for (uint32_t i = 0; i < pixels.Count(); i++) {
        if (pixels[i].a != 0xFF) {
            supported = 1;
            break;
        }
    }

    return supported;
}

// VERIFY: 22491e4b manual
// OFFSET: 0x00617070
void __fastcall UpdateAllPortraits() {
    s_rtAlphaSupported = RTAlphaSupported();

    for (PLAYERPORTRAIT* portrait = s_playerPortraits.m_fulllist.Head(); portrait; portrait = s_playerPortraits.Next(portrait)) {
        portrait->m_cache.m_dirty = 1;
    }

    for (UNITPORTRAIT* portrait = s_unitPortraits.m_fulllist.Head(); portrait; portrait = s_unitPortraits.Next(portrait)) {
        portrait->m_cache.m_dirty = 1;
    }

    Script_SendUniqueSignalToAllUnits(SCRIPT_EVENT_UNIT_PORTRAIT_UPDATE);
}

// VERIFY: 8d2cec8a manual
// OFFSET: 0x00618010
void PortraitInitialize() {
    CStatus status;

    s_portraitMaskTexture = TextureCreate("Interface\\CharacterFrame\\TempPortraitAlphaMaskSmall.blp", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);

    if (s_portraitMaskTexture) {
        s_portraitMaskGxTex = TextureGetGxTex(s_portraitMaskTexture, 1, nullptr);
    } else {
        s_portraitMaskGxTex = nullptr;
    }

    s_rtAlphaSupported = RTAlphaSupported();

    if (s_rtAlphaSupported && (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl || g_theGxDevicePtr->DeviceApi() == GxApi_GLL)) {
        CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        flags.m_renderTarget = 1;
        s_portraitDepthTexture = TextureCreate(GxTex_2d, 64, 64, 24, GxTex_D24X8, GxTex_D24X8, flags, nullptr, PortraitDepthTextureCallback, "PortraitDepth", 1);
    }

    s_playerPortraitCache = nullptr;
    s_unitPortraitCache = nullptr;
}

// VERIFY: 59c574b9 auto
// OFFSET: 0x00616B40
static int32_t IsPortraitDrawable(CGUnit_C* unit) {
    if (unit->m_clientFlags & 0x400000) {
        return 0;
    }

    if (unit->m_characterComponent && (!unit->m_characterComponent->RenderPrep(0) || (unit->m_characterComponent->m_flags & 4))) {
        return 0;
    }

    if (!unit->m_worldModel) {
        return 0;
    }

    return unit->m_worldModel->IsDrawable(0, 1);
}

// VERIFY: 66b97025 manual
// OFFSET: 0x00616E80
void UpdatePendingPortraits() {
    if (!g_theGxDevicePtr || !g_theGxDevicePtr->CapsIsWindowVisible(-1) || !g_theGxDevicePtr || !g_theGxDevicePtr->CapsHasContext(-1)) {
        return;
    }

    PORTRAITREQUEST* next;

    for (PORTRAITREQUEST* request = s_pendingPortraits.Head(); !s_pendingPortraits.IsEnd(request); request = next) {
        next = s_pendingPortraits.RawNext(request);
        WOWGUID guid = request->m_guid;
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            s_freePortraits.LinkToTail(request);
            continue;
        }

        if ((unit->m_clientFlags & 0x400000) && !unit->InitializeComponent()) {
            continue;
        }

        if (!IsPortraitDrawable(unit)) {
            continue;
        }

        uint32_t flags = request->m_flags;
        s_freePortraits.LinkToTail(request);

        if (flags & 2) {
            Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_PORTRAIT_UPDATE);
        }

        if (flags & 1) {
            Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_MODEL_CHANGED);
        }
    }
}

// VERIFY: e142378a auto
// OFFSET: 0x00618110
void UpdateModelForUI(const WOWGUID& guid, uint32_t flags) {
    if (flags & 2) {
        PLAYERPORTRAIT* portrait = s_playerPortraits.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (portrait) {
            portrait->m_cache.m_dirty = 1;
        }
    }

    if (flags & 2) {
        Script_SendUniqueSignal(guid, SCRIPT_EVENT_UNIT_PORTRAIT_UPDATE);
    }

    if (flags & 1) {
        Script_SendUniqueSignal(guid, SCRIPT_EVENT_UNIT_MODEL_CHANGED);
    }
}

// STATIC: 0x00AD6628
const char* s_sexNames[3] = { "Male", "Female", "NOSEX" };

// INLINED
PORTRAITCACHE::PORTRAITCACHE() : m_dirty(1), m_texture(nullptr), m_prev(nullptr), m_next(nullptr) {
}

// VERIFY: 3458775d auto
// OFFSET: 0x00616950
static void PortraitCacheTouch(PORTRAITCACHE** head, PORTRAITCACHE* entry) {
    if (entry == *head) {
        return;
    }

    if (entry->m_prev) {
        entry->m_prev->m_next = entry->m_next;
    }

    if (entry->m_next) {
        entry->m_next->m_prev = entry->m_prev;
    }

    entry->m_prev = nullptr;
    entry->m_next = *head;

    if (*head) {
        (*head)->m_prev = entry;
    }

    *head = entry;
}

// VERIFY: 1bb6cff0 auto
// OFFSET: 0x00616B90
static void TextureUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command != GxTex_Latch || !userArg) {
        return;
    }

    texelStrideInBytes = 4 * width;
    texels = static_cast<TSGrowableArray<CImVector>*>(userArg)->m_data;
}

// VERIFY: ff723d81 auto
// OFFSET: 0x00616BC0
static void PortraitLightingCallback(CM2Model* model, CM2Lighting* lighting, void* arg) {
    C3Vector ambient(0.45f, 0.45f, 0.45f);
    lighting->AddAmbient(ambient);
    C3Vector direction(-1.0f, 0.0f, -1.0f);
    C3Vector diffuse(1.0f, 1.0f, 1.0f);
    lighting->AddDiffuse(diffuse, direction);
}

// VERIFY: 2cd9b1ae auto
// OFFSET: 0x00616F90
static void PortraitCachePurge(PORTRAITCACHE** head, uint32_t maxEntries, int32_t force) {
    uint32_t index = 1;
    PORTRAITCACHE* next;

    for (PORTRAITCACHE* entry = *head; entry; entry = next) {
        next = entry->m_next;

        if (index > maxEntries && (TextureGetRefCnt(entry->m_texture) == 1 || force)) {
            CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
            TextureGetGxTexFlags(entry->m_texture, &flags);

            if (flags.m_renderTarget) {
                s_freePortraitTextures.New(entry->m_texture);
            } else {
                HandleClose(reinterpret_cast<HOBJECT>(entry->m_texture));
            }

            entry->m_texture = nullptr;
            entry->m_dirty = 1;

            if (entry->m_next) {
                entry->m_next->m_prev = entry->m_prev;
            }

            if (entry->m_prev) {
                entry->m_prev->m_next = entry->m_next;
            }

            if (*head == entry) {
                *head = entry->m_next;
            }

            entry->m_prev = nullptr;
            entry->m_next = nullptr;
        }

        index++;
    }
}

// VERIFY: b77b6e91 manual
// OFFSET: 0x00617100
static int32_t QueuePortraitUpdate(CGUnit_C* unit, uint32_t flags) {
    if (g_theGxDevicePtr && g_theGxDevicePtr->CapsIsWindowVisible(-1) && g_theGxDevicePtr && g_theGxDevicePtr->CapsHasContext(-1) && IsPortraitDrawable(unit)) {
        return 0;
    }

    PORTRAITREQUEST* request = s_freePortraits.Head();

    if (!request) {
        void* memory = SMemAlloc(sizeof(PORTRAITREQUEST), __FILE__, __LINE__, 0);
        request = memory ? new (memory) PORTRAITREQUEST() : nullptr;
    }

    s_pendingPortraits.LinkToTail(request);
    request->m_guid = unit->m_obj->m_guid;
    request->m_flags = flags;
    return 1;
}

// VERIFY: 0ce6a9ac manual
// OFFSET: 0x006176A0
static TSFixedArray<uint8_t>* GetAlphaMask(uint32_t size) {
    static TSFixedArray<uint8_t> s_alphaMasks[2];
    TSFixedArray<uint8_t>* mask = &s_alphaMasks[size == 64];

    if (mask->m_count) {
        return mask;
    }

    mask->SetCount(size * size);
    int32_t loaded = 0;
    CTgaFile tga;
    const char* tgaName = nullptr;

    if (size == 128) {
        tgaName = "Interface\\CharacterFrame\\TempPortraitAlphaMask.tga";
    } else if (size == 64) {
        tgaName = "Interface\\CharacterFrame\\TempPortraitAlphaMaskSmall.tga";
    }

    if (tga.Open(tgaName, 1) && tga.LoadImageData(0)) {
        memcpy(mask->m_data, tga.Image(), mask->m_count);
        tga.Close();
        return mask;
    }

    CBLPFile blp;
    const char* blpName = nullptr;

    if (size == 128) {
        blpName = "Interface\\CharacterFrame\\TempPortraitAlphaMask.blp";
    } else if (size == 64) {
        blpName = "Interface\\CharacterFrame\\TempPortraitAlphaMaskSmall.blp";
    }

    if (blp.Open(blpName, 1)) {
        uint8_t* pixels;
        uint32_t stride;

        if (blp.Lock(PIXEL_ARGB8888, 0, pixels, stride)) {
            for (uint32_t i = 0; i < mask->m_count; i++) {
                mask->m_data[i] = pixels[4 * i];
            }

            loaded = 1;
        }

        blp.Unlock(0);
        blp.Close();
    }

    blp.Close();

    if (!loaded) {
        memset(mask->m_data, 0xFF, mask->m_count);
    }

    tga.Close();
    return mask;
}

// VERIFY: 06d7ef51 manual
// OFFSET: 0x00617A10
static int32_t AddPendingPortraitFlags(CGUnit_C* unit, uint32_t flags) {
    for (PORTRAITREQUEST* request = s_pendingPortraits.Head(); request; request = s_pendingPortraits.Next(request)) {
        if (request->m_guid == unit->m_obj->m_guid) {
            request->m_flags |= flags;
            return 1;
        }
    }

    return 0;
}

// VERIFY: b5510c04 auto
// OFFSET: 0x00618180
void SetPortraitTexture(CSimpleTexture* texture, uint32_t race, uint32_t sex, WOWGUID guid) {
    if (guid) {
        PLAYERPORTRAIT* portrait = s_playerPortraits.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (portrait && portrait->m_cache.m_texture && !portrait->m_cache.m_dirty) {
            texture->SetTexture(portrait->m_cache.m_texture);
            return;
        }
    }

    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(race);
    char fileName[64];
    SStrPrintf(fileName, sizeof(fileName), "Interface\\CharacterFrame\\TemporaryPortrait-%s-%s", s_sexNames[sex], raceRec->m_clientFileString);
    texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
}

// VERIFY: 84c2bbbc auto
// OFFSET: 0x00618230
void SetPortraitTexture(CSimpleTexture* texture, uint32_t displayId, CGPartyPetState* petState) {
    UNITPORTRAIT* portrait = s_unitPortraits.Ptr(displayId, HASHKEY_DWORD{ displayId });

    if (portrait && !portrait->m_cache.m_dirty) {
        texture->SetTexture(portrait->m_cache.m_texture);
        return;
    }

    if (petState->m_guid) {
        CGPartyMemberState* state = CGGameUI::GetPartyMemberStateFromPet(petState->m_guid);
        uint32_t seatId = state->m_vehicleSeatId;

        if (seatId) {
            VehicleSeatRec* seat = g_vehicleSeatDB.GetRecord(seatId);

            if (seat->m_uiSkin == 0) {
                texture->SetTexture("Interface\\CharacterFrame\\TemporaryPortrait-Vehicle-Organic", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
                return;
            }

            if (seat->m_uiSkin == 1) {
                texture->SetTexture("Interface\\CharacterFrame\\TemporaryPortrait-Vehicle-Mechanical", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
                return;
            }
        }
    }

    texture->SetTexture("Interface\\CharacterFrame\\TemporaryPortrait-Pet", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
}

// VERIFY: 1ca97177 manual
// OFFSET: 0x00619330
void SetPortraitTexture(CSimpleTexture* texture, const char* fileName) {
    if (!fileName || !*fileName) {
        return;
    }

    ITEMPORTRAIT* portrait = s_itemPortraits.Ptr(fileName);

    if (portrait) {
        texture->SetTexture(portrait->m_texture);
        return;
    }

    portrait = s_itemPortraits.New(fileName, 0, 0);
    CBLPFile blp;
    char blpName[260];
    SStrPrintf(blpName, sizeof(blpName), "%s.blp", fileName);

    if (blp.Open(blpName, 1)) {
        if (!portrait->m_texels.Count()) {
            portrait->m_texels.SetCount(4096);
        }

        uint8_t* pixels;
        uint32_t stride;

        if (blp.Lock(PIXEL_ARGB8888, 0, pixels, stride)) {
            for (uint32_t i = 0; i < portrait->m_texels.Count(); i++) {
                portrait->m_texels.m_data[i] = *reinterpret_cast<CImVector*>(&pixels[4 * i]);
            }
        }

        blp.Unlock(0);
        blp.Close();
        TSFixedArray<uint8_t>* mask = GetAlphaMask(64);

        for (uint32_t i = 0; i < portrait->m_texels.Count(); i++) {
            portrait->m_texels.m_data[i].a = mask->m_data[i];
        }

        CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
        CTexture* portraitTexture = TextureCreate(64, 64, GxTex_Argb8888, GxTex_Argb8888, flags, &portrait->m_texels, TextureUpdate, "Portrait2", 0);
        texture->SetTexture(portraitTexture);
        portrait->m_texture = portraitTexture;
    }

    blp.Close();
}

// VERIFY: f3347f07 manual
// OFFSET: 0x00619580
void RenderPortraitTexture(CSimpleTexture* texture, CGUnit_C* unit) {
    if (!texture || !unit || AddPendingPortraitFlags(unit, 2)) {
        return;
    }

    if (unit->GetPortraitTextureName() && *unit->GetPortraitTextureName()) {
        char fileName[260];
        SStrPrintf(fileName, sizeof(fileName), "interface\\icons\\%s", unit->GetPortraitTextureName());
        SetPortraitTexture(texture, fileName);
        return;
    }

    if (unit->m_obj->m_type & TYPE_PLAYER) {
        WOWGUID guid = unit->m_obj->m_guid;
        PLAYERPORTRAIT* portrait = s_playerPortraits.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (portrait && !portrait->m_cache.m_dirty) {
            texture->SetTexture(portrait->m_cache.m_texture);
            PortraitCacheTouch(&s_playerPortraitCache, &portrait->m_cache);
            return;
        }
    } else {
        uint32_t displayId = unit->GetPortraitDisplayId();
        UNITPORTRAIT* portrait = s_unitPortraits.Ptr(displayId, HASHKEY_DWORD{ displayId });

        if (portrait && !portrait->m_cache.m_dirty) {
            texture->SetTexture(portrait->m_cache.m_texture);
            PortraitCacheTouch(&s_unitPortraitCache, &portrait->m_cache);
            return;
        }
    }

    if (QueuePortraitUpdate(unit, 2)) {
        return;
    }

    CM2Scene* scene = M2CreateScene();

    if (!scene) {
        return;
    }

    CM2Model* model = scene->DuplicateModel(unit->m_worldModel, 0);

    if (!model) {
        scene->Release();
        return;
    }

    CCamera* createdCamera = nullptr;
    CCamera* camera;

    if (model->HasCamera(CAMERA_PORTRAIT)) {
        camera = model->GetCameraById(CAMERA_PORTRAIT);
    } else {
        camera = CameraCreate();
        createdCamera = camera;

        if (!camera) {
            model->Release();
            scene->Release();
            return;
        }

        C3Vector target(0.0f, 0.0f, 0.0f);
        M2SequenceInfo info;
        info.Clear();
        model->GetSequenceInfo(0, 0, info);
        C3Vector position;
        position.x = info.m_radius * 1.7f + info.m_center.x;
        target = info.m_center;
        position.y = info.m_center.y;
        position.z = info.m_center.z;
        CameraSetFloat(camera, 4, 1.57f);
        CameraSetCoord(camera, 8, target, 0);
        CameraSetCoord(camera, 7, position, 0);
        CameraSetFloat(camera, 3, 0.027777778f);
    }

    CCharacterComponent::RemoveVisuals(model);
    model->SetBoneSequence(0xFFFFFFFF, 0, 0, 0, 0.0f, 0, 1);

    float viewportMinX;
    float viewportMaxX;
    float viewportMinY;
    float viewportMaxY;
    float viewportMinZ;
    float viewportMaxZ;
    GxXformViewport(viewportMinX, viewportMaxX, viewportMinY, viewportMaxY, viewportMinZ, viewportMaxZ);
    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    C44Matrix savedProjection;
    C44Matrix savedView;
    GxXformProjection(savedProjection);
    GxXformView(savedView);
    int32_t fogEnabled = GxMasterEnable(GxMasterEnable_Fog);
    GxMasterEnableSet(GxMasterEnable_Fog, 0);

    static C44Matrix s_identity;

    C3Vector cameraPosition(0.0f, 0.0f, 0.0f);
    C3Vector cameraTarget(0.0f, 0.0f, 0.0f);
    model->IsLoaded(1, 1);
    model->SetAnimating(1);
    C3Vector origin(0.0f, 0.0f, 0.0f);
    scene->Animate(origin);
    DataMgrGetCoord(reinterpret_cast<CDataMgr*>(camera), 7, &cameraPosition);
    DataMgrGetCoord(reinterpret_cast<CDataMgr*>(camera), 8, &cameraTarget);

    CRect window;
    window.minY = 0.0f;
    window.minX = 0.0f;
    window.maxY = 0.0f;
    window.maxX = 0.0f;
    GxCapsWindowSizeInScreenCoords(window);
    float width = window.maxX - window.minX;
    float height = window.maxY - window.minY;
    float ddcHeight = NDCToDDCHeight(1.0f);
    float top = ddcHeight - NDCToDDCHeight(1.0f) / height * 64.0f;
    ddcHeight = NDCToDDCHeight(1.0f);
    float right = NDCToDDCWidth(1.0f) / width * 64.0f;

    CRect viewport;
    viewport.minY = top;
    viewport.minX = 0.0f;
    viewport.maxY = ddcHeight;
    viewport.maxX = right;

    CRect projection;
    projection.minY = 0.0f;
    projection.minX = 0.0f;
    projection.maxY = 1.0f;
    projection.maxX = right / (ddcHeight - top);
    CameraSetupWorldProjection(camera, projection, 0);

    PORTRAITCACHE* cache;

    if (unit->m_obj->m_type & TYPE_PLAYER) {
        WOWGUID guid = unit->m_obj->m_guid;
        PLAYERPORTRAIT* portrait = s_playerPortraits.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (!portrait) {
            portrait = s_playerPortraits.New(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid), 0, 0);
        }

        cache = &portrait->m_cache;
        PortraitCacheTouch(&s_playerPortraitCache, cache);
    } else {
        uint32_t displayId = unit->GetPortraitDisplayId();
        UNITPORTRAIT* portrait = s_unitPortraits.Ptr(displayId, HASHKEY_DWORD{ displayId });

        if (!portrait) {
            portrait = s_unitPortraits.InternalNewNode(displayId, 0, 0);
        }

        cache = &portrait->m_cache;
        PortraitCacheTouch(&s_unitPortraitCache, cache);
    }

    if (!cache->m_texture) {
        if (s_freePortraitTextures.Count()) {
            uint32_t count = s_freePortraitTextures.Count() - 1;
            cache->m_texture = s_freePortraitTextures.m_data[count];
            s_freePortraitTextures.SetCount(count);
        }

        if (!cache->m_texture) {
            CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
            flags.m_filter = GxTex_Linear;

            if (s_rtAlphaSupported) {
                flags.m_renderTarget = 1;
            }

            cache->m_texture = TextureCreate(64, 64, GxTex_Argb8888, GxTex_Argb8888, flags, s_rtAlphaSupported ? nullptr : &cache->m_texels, TextureUpdate, "Portrait1", 0);
        }
    }

    texture->SetTexture(cache->m_texture);
    CGxTex* gxTex = cache->m_texture ? TextureGetGxTex(cache->m_texture, 1, nullptr) : nullptr;
    CGxTex* savedColorTarget = nullptr;
    CGxTex* savedDepthTarget = nullptr;

    if (!gxTex) {
        model->Release();
        scene->Release();

        if (s_rtAlphaSupported) {
            s_rtAlphaSupported = 0;
            RenderPortraitTexture(texture, unit);
        }

        return;
    }

    uint32_t renderTarget = gxTex->m_flags.m_renderTarget;

    if (renderTarget) {
        g_theGxDevicePtr->DeviceGetRenderTarget(GxBuffers_Color, savedColorTarget);
        GxRenderTargetSet(GxBuffers_Color, gxTex, 0);

        if (s_portraitDepthTexture) {
            g_theGxDevicePtr->DeviceGetRenderTarget(GxBuffers_Depth, savedDepthTarget);
            GxRenderTargetSet(GxBuffers_Depth, TextureGetGxTex(s_portraitDepthTexture, 1, nullptr), 0);
        }
    } else {
        DDCToNDC(viewport.minX, viewport.minY, &viewport.minX, &viewport.minY);
        DDCToNDC(viewport.maxX, viewport.maxY, &viewport.maxX, &viewport.maxY);
        GxXformSetViewport(viewport.minX, viewport.maxX, viewport.minY, viewport.maxY, 0.0f, 1.0f);
    }

    GxSceneClear(GxBufferBit_Color | GxBufferBit_Depth, CImVector());
    model->IsDrawable(1, 1);
    model->SetAnimating(1);

    if (model->m_attachParent) {
        model->m_fFlags |= 0x80;
    } else {
        model->m_fFlags |= 0x8;
    }

    if (model->m_attachParent) {
        model->m_fFlags |= 0x20000;
    } else {
        model->m_fFlags |= 0x10000;
    }

    model->m_lightingCallback = PortraitLightingCallback;
    model->m_lightingArg = nullptr;
    scene->Animate(cameraPosition);
    scene->Draw(M2PASS_0);
    scene->Draw(M2PASS_1);

    if (renderTarget) {
        C44Matrix view;
        GxXformSetView(view);
        C44Matrix ortho;
        GxuXformCreateOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 500.0f, ortho);
        GxXformSetProjection(ortho);
        g_theGxDevicePtr->RsPush();
        GxRsSet(GxRs_Texture0, s_portraitMaskGxTex);
        GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
        GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
        GxRsSet(GxRs_ColorWrite, 8);
        GxRsSet(GxRs_BlendingMode, 0);
        GxRsSet(GxRs_Lighting, 0);
        GxRsSet(GxRs_Culling, 0);
        GxRsSet(GxRs_DepthTest, 0);
        GxRsSet(GxRs_AlphaRef, 0);
        GxRsSet(GxRs_Multisample, 0);
        g_theGxDevicePtr->XformPushAndSet(GxXform_World, view);

        static C3Vector s_positions[] = { C3Vector(-1.0f, 1.0f, 0.0f), C3Vector(-1.0f, -1.0f, 0.0f), C3Vector(1.0f, 1.0f, 0.0f), C3Vector(1.0f, -1.0f, 0.0f) };
        static C2Vector s_texCoords[] = { C2Vector(0.0f, 0.0f), C2Vector(0.0f, 1.0f), C2Vector(1.0f, 0.0f), C2Vector(1.0f, 1.0f) };
        static uint16_t s_indices[] = { 0, 1, 2, 3 };

        GxPrimLockVertexPtrs(4, s_positions, sizeof(C3Vector), nullptr, 0, nullptr, 0, nullptr, 0, s_texCoords, sizeof(C2Vector), nullptr, 0);
        GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_indices);
        GxPrimUnlockVertexPtrs();
        GxXformPop(GxXform_World);
        g_theGxDevicePtr->RsPop();
        GxRenderTargetSet(GxBuffers_Color, savedColorTarget, 0);

        if (s_portraitDepthTexture) {
            GxRenderTargetSet(GxBuffers_Depth, savedDepthTarget, 0);
        }

        CImVector black;
        black.b = 0;
        black.g = 0;
        black.r = 0;
        black.a = 0xFF;
        GxSceneClear(GxBufferBit_Depth, black);
    } else if (g_theGxDevicePtr->DeviceApi() != GxApi_D3d10 && g_theGxDevicePtr->DeviceApi() != GxApi_D3d11) {
        CiRect rect;
        rect.minY = 0;
        rect.minX = 0;
        rect.maxY = 64;
        rect.maxX = 64;
        GxReadPixels(rect, cache->m_texels);
        TSFixedArray<uint8_t>* mask = GetAlphaMask(64);

        for (uint32_t i = 0; i < cache->m_texels.Count(); i++) {
            cache->m_texels.m_data[i].a = mask->m_data[i];
        }

        GxTexUpdate(gxTex, rect, 1);
        GxSceneClear(GxBufferBit_Color | GxBufferBit_Depth, CImVector());
    }

    GxXformSetProjection(savedProjection);
    GxXformSetView(savedView);
    GxMasterEnableSet(GxMasterEnable_Fog, fogEnabled);
    GxXformSetViewport(viewportMinX, viewportMaxX, viewportMinY, viewportMaxY, viewportMinZ, viewportMaxZ);

    if (createdCamera) {
        HandleClose(reinterpret_cast<HOBJECT>(createdCamera));
    }

    model->Release();
    scene->Release();

    if (unit->m_obj->m_type & TYPE_PLAYER) {
        PortraitCachePurge(&s_playerPortraitCache, 40, 0);
    } else {
        PortraitCachePurge(&s_unitPortraitCache, 8, 0);
    }

    if (cache) {
        cache->m_dirty = 0;
    }
}

// VERIFY: 5cc131b0 manual
// OFFSET: 0x00617A60
int32_t ReadyModelForUI(CGUnit_C* unit, uint32_t flags) {
    if (!unit || AddPendingPortraitFlags(unit, flags)) {
        return 0;
    }

    return QueuePortraitUpdate(unit, flags) == 0;
}

// VERIFY: 7490c1d9 auto
// OFFSET: 0x00617860
void PortraitShutdown() {
    s_portraitMaskGxTex = nullptr;
    HandleClose(reinterpret_cast<HOBJECT>(s_portraitMaskTexture));
    s_portraitMaskTexture = nullptr;

    if (s_portraitDepthTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_portraitDepthTexture));
        s_portraitDepthTexture = nullptr;
    }

    PortraitCachePurge(&s_playerPortraitCache, 0, 1);
    PortraitCachePurge(&s_unitPortraitCache, 0, 1);

    for (uint32_t i = 0; i < s_freePortraitTextures.m_count; i++) {
        HandleClose(reinterpret_cast<HOBJECT>(s_freePortraitTextures.m_data[i]));
    }

    s_freePortraitTextures.Clear();
    s_playerPortraits.Clear();
    s_unitPortraits.Clear();
    s_itemPortraits.Clear();
    s_pendingPortraits.Clear();
    s_freePortraits.Clear();
}
