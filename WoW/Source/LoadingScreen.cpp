#include <cstdint>
#include <cstring>
#include <malloc.h>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/Tempest/C3Spline_CatmullRom.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/LoadingScreenTaxiSplinesRec.h"
#include "WoW/Source/DB/Rec/TaxiPathNodeRec.h"
#include "WoW/Source/Object/GameObjectDef.h"
#include "WoW/Source/Ui/WorldMap.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/Rec/LoadingScreensRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Event/CSizeEvent.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Scrn/Types.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Services/Types.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x009E2DFC
static const BackgroundImage s_textureInfo[2] = {
    { "Interface\\Glues\\LoadingBar\\Loading-BarFill", true, 0.5f, 0.075f, 0.525f, 0.025f },
    { "Interface\\Glues\\LoadingBar\\Loading-BarBorder", false, 0.5f, 0.075f, 0.6f, 0.05f },
};

// STATIC: 0x009E2E2C
static const BackgroundImage s_miniTextureInfo[2] = {
    { "Interface\\Glues\\LoadingBar\\Loading-BarFill", true, 0.5f, 0.975f, 0.525f, 0.025f },
    { "Interface\\Glues\\LoadingBar\\Loading-BarBorder", false, 0.5f, 0.975f, 0.6f, 0.05f },
};

// STATIC: 0x00AB63B4
static float s_normalAspect = 4.0f / 3.0f;

// STATIC: 0x00AB63B8
static float s_wideAspect = 1.6f;

// STATIC: 0x00AB63BC
static int32_t s_simpleMapID = -1;

// STATIC: 0x00AB63C0
static uint16_t s_indices[4] = { 0, 1, 2, 3 };

// STATIC: 0x00AB63C8
static const char* s_fontNames[9] = {
    "Fonts\\FRIZQT__.TTF",
    "Fonts\\2002.TTF",
    "Fonts\\FRIZQT__.TTF",
    "Fonts\\FRIZQT__.TTF",
    "Fonts\\ZYKai_T.TTF",
    "Fonts\\blei00d.TTF",
    "Fonts\\FRIZQT__.TTF",
    "Fonts\\FRIZQT__.TTF",
    "Fonts\\NIM_____.TTF",
};

// STATIC: 0x00AB63FC
static CImVector s_pathColor = CImVector(0xFF, 0xFF, 0xFF, 0xFF);

// STATIC: 0x00AB6400
static C2Vector s_texCoord[4] = {
    C2Vector(0.0f, 1.0f),
    C2Vector(1.0f, 1.0f),
    C2Vector(0.0f, 0.0f),
    C2Vector(1.0f, 0.0f),
};

// STATIC: 0x00AB6420
static C3Vector s_pathOffsets[4] = {
    C3Vector(-0.01f, -0.015f, 0.0f),
    C3Vector(0.01f, -0.015f, 0.0f),
    C3Vector(-0.01f, 0.015f, 0.0f),
    C3Vector(0.01f, 0.015f, 0.0f),
};

// STATIC: 0x00AB6450
static C2Vector s_pathTexCoords[4] = {
    C2Vector(0.0f, 1.0f),
    C2Vector(0.5f, 1.0f),
    C2Vector(0.0f, 0.5f),
    C2Vector(0.5f, 0.5f),
};

// STATIC: 0x00AB6470
static CSizeEvent s_sizeEvent;

// STATIC: 0x00B2FE60
static CGxShader* s_pixelShader[1];

// STATIC: 0x00B2FE64
static CGxShader* s_vertexShader[2];

// STATIC: 0x00B2FE6C
static uint32_t s_lastUpdateTime;

// STATIC: 0x00B2FE70
static uint32_t s_nextPingTime;

// STATIC: 0x00B2FE74
static CTexture* s_dynamicElementsTexture;

// STATIC: 0x00B2FE78
static CTexture* s_dynamicBackgroundTextures[12];

// STATIC: 0x00B2FEA8
static CTexture* s_simpleBackgroundTexture;

// STATIC: 0x00B2FEAC
static float s_progress;

// STATIC: 0x00B2FEB0
static float s_worldProgress;

// STATIC: 0x00B2FEB4
static float s_asyncProgress;

// STATIC: 0x00B2FEB8
static float s_xmlProgress;

// STATIC: 0x00B2FEBC
static bool s_loadingWorld;

// STATIC: 0x00B2FEC0
static HLAYER s_miniLoadingLayer;

// STATIC: 0x00B2FEC4
static CTexture* s_miniTextures[2];

// STATIC: 0x00B2FECC
static CTexture* s_textures[2];

// STATIC: 0x00B2FED4
static HLAYER s_loadingScreenLayer;

// STATIC: 0x00B2FED8
static bool s_usingWideScreen;

// STATIC: 0x00B2FED9
static bool s_raceDataTip;

// STATIC: 0x00B2FEDC
static float s_spellProgress;

// STATIC: 0x00B2FEE0
static HTEXTFONT s_textFont;

// STATIC: 0x00B2FEE4
static CGxString* s_tipStr;

// STATIC: 0x00B2FEE8
static const char* s_gameTip;

// STATIC: 0x00B2FEEC
static CGxStringBatch* s_batch;

// STATIC: 0x00B2FEF0
static int32_t s_sizeEventPosted;

// STATIC: 0x00B2FF00
static C3Vector s_tileVertices[12][4];

// STATIC: 0x00B30140
static C2Vector s_tileTexCoords[12][4];

// STATIC: 0x00B302C0
static TSGrowableArray<DYNAMICELEMENTVERT> s_dynamicElements;

// STATIC: 0x00B302D0
static CLoadingScreenShipInfo<TaxiPathNodeRec> s_shipInfo;

// STATIC: 0x00B302F0
static char s_raceDataTipText[1024];

// VERIFY: 30cfdd0d manual
// OFFSET: 0x00407C50
static void BuildTiles() {
    for (int32_t row = 0; row < 3; row++) {
        for (int32_t col = 0; col < 4; col++) {
            int32_t left = col * 256;
            int32_t top = row * 256;
            int32_t right = left + 256;
            int32_t bottom = top + 256;

            if (right > 1002) {
                right = 1002;
            }

            if (bottom > 668) {
                bottom = 668;
            }

            int32_t tile = row * 4 + col;
            s_tileVertices[tile][0] = C3Vector(left * (1.0f / 1002.0f), 1.0f - bottom * (1.0f / 668.0f), 0.0f);
            s_tileVertices[tile][1] = C3Vector(right * (1.0f / 1002.0f), 1.0f - bottom * (1.0f / 668.0f), 0.0f);
            s_tileVertices[tile][2] = C3Vector(left * (1.0f / 1002.0f), 1.0f - top * (1.0f / 668.0f), 0.0f);
            s_tileVertices[tile][3] = C3Vector(right * (1.0f / 1002.0f), 1.0f - top * (1.0f / 668.0f), 0.0f);

            float u = 1.0f;
            float v = 1.0f;

            if (col == 3) {
                u = 234.0f / 256.0f;
            }

            if (row == 2) {
                v = 156.0f / 256.0f;
            }

            s_tileTexCoords[tile][0] = C2Vector(0.0f, v);
            s_tileTexCoords[tile][1] = C2Vector(u, v);
            s_tileTexCoords[tile][2] = C2Vector(0.0f, 0.0f);
            s_tileTexCoords[tile][3] = C2Vector(u, 0.0f);
        }
    }
}

// VERIFY: b60ab174 auto
// OFFSET: 0x0040B2B0
void LoadingScreenInitialize() {
    g_theGxDevicePtr->ShaderCreate(s_vertexShader, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    g_theGxDevicePtr->ShaderCreate(s_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
    s_shipInfo.Initialize(&g_taxiPathNodeDB, g_taxiPathDB.m_maxID + 1);
    BuildTiles();
}

// VERIFY: 91ed5224 auto
// OFFSET: 0x00407AB0
static int32_t SizeEvent(const void* data, void* param) {
    memcpy(&s_sizeEvent, data, sizeof(s_sizeEvent));
    s_sizeEventPosted = 1;
    return 0;
}

// VERIFY: 089a3404 auto
// OFFSET: 0x008E5250
static int32_t SwallowEvent(const void* data, void* param) {
    return 0;
}

// VERIFY: 59ff528b manual
// OFFSET: 0x004079D0
static void UpdateProgressValue() {
    float xmlProgress = 0.0f;
    float worldScale = 1.0f;

    if (s_xmlProgress != 0.0f) {
        xmlProgress = s_xmlProgress;

        if (s_loadingWorld) {
            worldScale = 0.5f;
            xmlProgress = 0.5f * s_xmlProgress;
        }
    }

    s_progress = xmlProgress + s_asyncProgress * 0.0f + worldScale * s_worldProgress;

    if (s_progress < 0.0f) {
        s_progress = 0.0f;
    } else if (s_progress >= 1.0f) {
        s_progress = 1.0f;
    }

    if (SFile::IsTrial() && s_raceDataTip) {
        if (StreamingIsRaceDataReady(StreamingGetRace())) {
            s_progress = s_progress * 0.3f + 0.7f;
        } else {
            s_progress = s_spellProgress * 0.7f;
        }
    }
}

// VERIFY: 5d3b9940 auto
// OFFSET: 0x00407B00
static void InitializeProgressBarEvents() {
    EventRegisterEx(EVENT_ID_CHAR, SwallowEvent, nullptr, 8.0f);
    EventRegisterEx(EVENT_ID_IME, SwallowEvent, nullptr, 8.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN, SwallowEvent, nullptr, 8.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, SwallowEvent, nullptr, 8.0f);
    EventRegisterEx(EVENT_ID_MOUSEDOWN, SwallowEvent, nullptr, 8.0f);
    EventRegisterEx(EVENT_ID_MOUSEMOVE, SwallowEvent, nullptr, 8.0f);
    EventRegisterEx(EVENT_ID_SIZE, SizeEvent, nullptr, 8.0f);
    s_sizeEventPosted = 0;
}

// VERIFY: c05cbcfa auto
// OFFSET: 0x00407E30
void LoadingScreenSetTip(const char* tip) {
    s_gameTip = tip;
}

// VERIFY: 902a1fd5 auto
// OFFSET: 0x00407E40
void LoadingScreenSetScreenTipForRaceData(bool show) {
    const char* text = FrameScript_GetText("TRIAL_LOADING_MESSAGE", -1, GENDER_NOT_APPLICABLE);
    s_spellProgress = 0.0f;
    s_raceDataTip = show;

    if (show) {
        SStrPrintf(s_raceDataTipText, sizeof(s_raceDataTipText), text);
        s_gameTip = s_raceDataTipText;
    }
}

// VERIFY: f774cf5f manual
// OFFSET: 0x00407E90
bool LoadingScreenDrawing() {
    return s_loadingScreenLayer != nullptr;
}

// VERIFY: aa7cf797 auto
// OFFSET: 0x00407EA0
void LoadingScreenSetTransparent(bool transparent) {
    if (!s_loadingScreenLayer) {
        return;
    }

    uint32_t flags = ScrnLayerGetFlags(s_loadingScreenLayer);

    if (transparent) {
        ScrnLayerSetFlags(s_loadingScreenLayer, flags | 0x1);
    } else {
        ScrnLayerSetFlags(s_loadingScreenLayer, flags & ~0x1u);
    }
}

// VERIFY: 13132595 manual
// OFFSET: 0x00408520
static void ProgressBarSendKeepAlive(uint32_t time) {
    if (static_cast<int32_t>(time - s_nextPingTime) < 0) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_KEEP_ALIVE));
    msg.Finalize();
    ClientServices::Send2(&msg);
    s_nextPingTime = time + 30000;
}

// VERIFY: 575b0c08 manual
// OFFSET: 0x004085A0
static bool PaintSimpleBackground() {
    if (!s_simpleBackgroundTexture) {
        return false;
    }

    static C3Vector vertices[4] = {
        C3Vector(0.0f, 0.0f, 0.0f),
        C3Vector(1.0f, 0.0f, 0.0f),
        C3Vector(0.0f, 1.0f, 0.0f),
        C3Vector(1.0f, 1.0f, 0.0f),
    };

    CImVector color;
    *reinterpret_cast<uint32_t*>(&color) = 0xFFFFFFFF;

    GxPrimLockVertexPtrs(4, vertices, sizeof(C3Vector), nullptr, 0, &color, 0, nullptr, 0, s_texCoord, sizeof(C2Vector), nullptr, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(s_simpleBackgroundTexture, 1, nullptr));
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_indices);
    GxPrimUnlockVertexPtrs();
    GxuFontRenderBatch(s_batch);

    return true;
}

// VERIFY: 0739e068 manual
// OFFSET: 0x00409010
static bool PaintBackgroundImage() {
    if (!s_dynamicElements.Count()) {
        return false;
    }

    CImVector color;
    *reinterpret_cast<uint32_t*>(&color) = 0xFFFFFFFF;

    for (uint32_t i = 0; i < 12; i++) {
        GxPrimLockVertexPtrs(4, s_tileVertices[i], sizeof(C3Vector), nullptr, 0, &color, 0, nullptr, 0, s_tileTexCoords[i], sizeof(C2Vector), nullptr, 0);
        g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(s_dynamicBackgroundTextures[i], 1, nullptr));
        GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_indices);
        GxPrimUnlockVertexPtrs();
    }

    PaintDynamicLoadingBar();
    return true;
}

// VERIFY: c28f5a68 manual
// OFFSET: 0x004090C0
static void PaintLoadingBar(const BackgroundImage* images, int32_t count, CTexture** textures) {
    for (int32_t i = 0; i < count; i++) {
        if (!textures[i]) {
            continue;
        }

        GxRsSet(GxRs_BlendingMode, 2);
        GxUpdateAlphaRef(GxRs_AlphaRef);

        const BackgroundImage& image = images[i];
        C3Vector vertices[4];
        vertices[0].x = image.m_x - image.m_width * 0.5f;
        vertices[2].x = vertices[0].x;
        vertices[0].y = image.m_y - image.m_height * 0.5f;
        vertices[1].y = vertices[0].y;
        vertices[1].x = image.m_width * 0.5f + image.m_x;
        vertices[3].x = vertices[1].x;
        vertices[2].y = image.m_height * 0.5f + image.m_y;
        vertices[3].y = vertices[2].y;

        if (image.m_fill) {
            vertices[1].x = vertices[0].x + image.m_width * s_progress;
            vertices[3].x = vertices[1].x;
        }

        vertices[0].z = 0.0f;
        vertices[1].z = 0.0f;
        vertices[2].z = 0.0f;
        vertices[3].z = 0.0f;

        CImVector color;
        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;

        GxPrimLockVertexPtrs(4, vertices, sizeof(C3Vector), nullptr, 0, &color, 0, nullptr, 0, s_texCoord, sizeof(C2Vector), nullptr, 0);
        g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(textures[i], 1, nullptr));
        GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_indices);
        GxPrimUnlockVertexPtrs();
    }
}

// VERIFY: e5620ae9 manual
// OFFSET: 0x00409ED0
static bool LoadSimpleBackgroundTexture() {
    CStatus status;
    MapRec* mapRec = g_mapDB.GetRecord(s_simpleMapID);

    if (mapRec) {
        LoadingScreensRec* screenRec = g_loadingScreensDB.GetRecord(mapRec->m_loadingScreenID);

        if (screenRec) {
            s_usingWideScreen = false;
            s_simpleBackgroundTexture = nullptr;

            if (CoordinateGetAspectRatio() >= s_normalAspect + 0.001f && screenRec->m_hasWideScreen) {
                char* path = static_cast<char*>(alloca(strlen(screenRec->m_fileName) + 32));
                const char* extension = strchr(screenRec->m_fileName, '.');
                strcpy(path, screenRec->m_fileName);

                if (extension) {
                    path[extension - screenRec->m_fileName] = '\0';
                }

                strcat(path, "Wide");

                if (extension) {
                    strcat(path, extension);
                }

                if (SFile::FileExistsInArchives(path)) {
                    if (!SFile::IsTrial() || SFile::FileIsLocal(path, 6) == 1) {
                        CGxTexFlags flags;
                        s_simpleBackgroundTexture = TextureCreate(path, flags, &status, 1);

                        if (s_simpleBackgroundTexture) {
                            s_usingWideScreen = true;
                        }
                    } else {
                        SFile::MakeFileLocal(path, 6, false);
                    }
                }
            }

            if (!s_simpleBackgroundTexture && SFile::FileExistsInArchives(screenRec->m_fileName)) {
                if (!SFile::IsTrial() || SFile::FileIsLocal(screenRec->m_fileName, 6) == 1) {
                    CGxTexFlags flags;
                    s_simpleBackgroundTexture = TextureCreate(screenRec->m_fileName, flags, &status, 1);
                } else {
                    SFile::MakeFileLocal(screenRec->m_fileName, 6, false);
                }
            }
        }
    }

    if (!s_simpleBackgroundTexture) {
        CGxTexFlags flags;
        s_simpleBackgroundTexture = TextureCreate("Interface\\Glues\\loading", flags, &status, 1);
    }

    return s_simpleBackgroundTexture != nullptr;
}

// VERIFY: 91dabb04 manual
// OFFSET: 0x00409800
int32_t IsStillLoading() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !player->GetTransportGUID()) {
        return 1;
    }

    CGGameObject_C* transport = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(WOWGUID{ player->GetTransportGUID() }, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if (transport) {
        CMapBaseObj* object = transport->m_typeData->GetMapObjectTransportID();

        if (!object || !World::ObjectIsDrawable(object)) {
            return 1;
        }
    }

    LoadingScreenDisable();
    return 0;
}

// VERIFY: ebee3807 manual
// OFFSET: 0x0040A270
static void LoadingScreenPaint(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec) {
    if (!GxHasContext() || !GxIsWindowVisible() || (s_progress > 0.99f && !IsStillLoading())) {
        return;
    }

    if (!s_simpleBackgroundTexture && s_simpleMapID != -1 && !LoadSimpleBackgroundTexture()) {
        s_simpleMapID = -1;
    }

    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    CImVector black;
    *reinterpret_cast<uint32_t*>(&black) = 0xFF000000;
    GxSceneClear(GxBufferBit_Color, black);

    float aspect = CoordinateGetAspectCompensation();
    float ratio = s_usingWideScreen ? s_wideAspect / s_normalAspect : 1.0f;
    float scale = aspect / ratio;

    float left = 0.0f;
    float width = 1.0f;
    float bottom = 0.0f;
    float height = 1.0f;

    if (scale > 1.0f) {
        width = 1.0f / scale;
        left = (1.0f - width) * 0.5f;
    } else if (scale < 1.0f) {
        height = scale;
        bottom = (1.0f - scale) * 0.5f;
    }

    GxXformSetViewport(left, width + left, bottom, height + bottom, 0.0f, 1.0f);

    C44Matrix view;
    C44Matrix projection;
    GxuXformCreateOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 500.0f, projection);
    g_theGxDevicePtr->XformSetView(view);
    g_theGxDevicePtr->XformSetProjection(projection);

    g_theGxDevicePtr->RsPush();

    C44Matrix transform;

    if (s_vertexShader[0]->Valid() && s_pixelShader[0]->Valid()) {
        g_theGxDevicePtr->RsSet(GxRs_VertexShader, s_vertexShader[0]);
        g_theGxDevicePtr->RsSet(GxRs_PixelShader, s_pixelShader[0]);
        GxXformViewProjNativeTranspose(transform);
        GxShaderConstantsSet(GxSh_Vertex, 0, &transform.a0, 4);
    } else {
        GxRsSet(GxRs_Lighting, 0);
    }

    GxRsSet(GxRs_Fog, 0);

    if (!PaintBackgroundImage()) {
        PaintSimpleBackground();

        if (s_vertexShader[0]->Valid() && s_pixelShader[0]->Valid()) {
            GxShaderConstantsSet(GxSh_Vertex, 0, &transform.a0, 4);
        }

        PaintLoadingBar(s_textureInfo, 2, s_textures);
    }

    g_theGxDevicePtr->RsPop();
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

// VERIFY: 0ced5aa1 auto
// OFFSET: 0x0040A920
static void UpdateProgressBar(int32_t force) {
    uint32_t time = OsGetAsyncTimeMs();
    EventInputProcess(EventGetCurrentContext());

    if (!force && time - s_lastUpdateTime < 250) {
        return;
    }

    s_lastUpdateTime = time;
    SE2::HeartBeat(nullptr, nullptr);
    ProgressBarSendKeepAlive(time);
    UpdateProgressValue();
    LoadingScreenPaint(nullptr, nullptr, nullptr, 0.0f);
    GxScenePresent();
}

// VERIFY: 805d84ce manual
// OFFSET: 0x0040A990
static void InitializeProgressBar(bool loadingWorld) {
    if (s_loadingScreenLayer) {
        ScrnLayerSetFlags(s_loadingScreenLayer, ScrnLayerGetFlags(s_loadingScreenLayer) & ~0x1u);
    }

    s_xmlProgress = 0.0f;
    s_asyncProgress = 0.0f;
    s_worldProgress = 0.0f;
    s_loadingWorld = loadingWorld;

    if (!s_loadingScreenLayer) {
        CStatus status;

        for (uint32_t i = 0; i < sizeof(s_textures) / sizeof(s_textures[0]); i++) {
            if (!s_textures[i]) {
                CGxTexFlags flags;
                s_textures[i] = TextureCreate(s_textureInfo[i].m_path, flags, &status, 0);
            }
        }

        RECTF rect;
        rect.bottom = 0.0f;
        rect.left = 0.0f;
        rect.top = 1.0f;
        rect.right = 1.0f;
        ScrnLayerCreate(&rect, 9.0f, 6, nullptr, LoadingScreenPaint, &s_loadingScreenLayer);
        InitializeProgressBarEvents();
    }

    s_nextPingTime = OsGetAsyncTimeMs() + 30000;

    uint32_t time = OsGetAsyncTimeMs();
    EventInputProcess(EventGetCurrentContext());

    if (time - s_lastUpdateTime >= 250) {
        s_lastUpdateTime = time;
        SE2::HeartBeat(nullptr, nullptr);
        ProgressBarSendKeepAlive(time);
        UpdateProgressValue();
        LoadingScreenPaint(nullptr, nullptr, nullptr, 0.0f);
        GxScenePresent();
    }
}

// VERIFY: ac3b73a1 manual
// OFFSET: 0x0040AB20
void LoadingScreenSpellUpdateProgress(float progress) {
    if (progress <= s_spellProgress) {
        return;
    }

    s_spellProgress = progress;
    UpdateProgressBar(1.0f == progress);
}

// VERIFY: 2d94348d manual
// OFFSET: 0x0040AB70
void LoadingScreenEnable(int32_t mapID, bool loadingWorld) {
    CGGameUI::SetAspect();

    if (s_gameTip) {
        s_textFont = TextBlockGenerateFont(s_fontNames[s_locale], 1, NDCToDDCHeight(0.018f));
        CGxFont* font = TextBlockGetFontPtr(s_textFont);

        float width = DDCToNDCWidth(NDCToDDCWidth(515.0f / (CoordinateGetAspectCompensation() * 1024.0f)));
        float compensation = CoordinateGetAspectCompensation();
        float offset = compensation >= 1.0f ? 0.0f : (1.0f - compensation) * 0.5f;

        C3Vector position;
        position.x = 0.5f - width * 0.5f;
        position.y = offset + 0.1f;
        position.z = 1.0f;

        char* tip = static_cast<char*>(alloca(SStrLen(s_gameTip) + 1));
        memcpy(tip, s_gameTip, SStrLen(s_gameTip) + 1);

        for (char* end = &tip[SStrLen(tip) - 1]; *end == '\r' || *end == '\n'; end--) {
            *end = '\0';
        }

        CImVector color;
        *reinterpret_cast<uint32_t*>(&color) = 0xD7C8C8C8;
        GxuFontCreateString(font, tip, 0.018f, position, width, 1.0f, 0.005f, s_tipStr, GxVJ_Bottom, GxHJ_Left, 0, color, 0.0f, 1.0f);

        C2Vector shadowOffset;
        shadowOffset.x = 0.001f;
        shadowOffset.y = -0.001f;

        CImVector shadowColor;
        shadowColor.b = 0;
        shadowColor.g = 0;
        shadowColor.r = 0;
        shadowColor.a = 0xFF;
        GxuFontAddShadow(s_tipStr, shadowColor, shadowOffset);

        s_batch = GxuFontCreateBatch(false, false);
        GxuFontAddToBatch(s_batch, s_tipStr);
    }

    s_simpleMapID = mapID;
    InitializeProgressBar(loadingWorld);
}

// VERIFY: a0b9f60d auto
// OFFSET: 0x0040AEF0
void LoadingScreenAsyncCallback(float progress, void* param) {
    if (progress <= s_asyncProgress) {
        return;
    }

    s_asyncProgress = progress;
    UpdateProgressBar(1.0f == progress);
}

// VERIFY: 1a8e81c0 auto
// OFFSET: 0x0040AF40
void LoadingScreenWorldCallback(float progress, void* param) {
    if (progress <= s_worldProgress) {
        return;
    }

    s_worldProgress = progress;
    UpdateProgressBar(1.0f == progress);
}

// VERIFY: 58aa7b39 auto
// OFFSET: 0x0040AF90
void LoadingScreenXMLCallback(float progress, void* param) {
    if (progress <= s_xmlProgress) {
        return;
    }

    s_xmlProgress = progress;
    UpdateProgressBar(1.0f == progress);
}

// VERIFY: 2414bf03 auto
// OFFSET: 0x00407BD0
static void LoadingScreenDisableEvents() {
    EventUnregister(EVENT_ID_CHAR, SwallowEvent);
    EventUnregister(EVENT_ID_IME, SwallowEvent);
    EventUnregister(EVENT_ID_KEYDOWN, SwallowEvent);
    EventUnregister(EVENT_ID_KEYDOWN_REPEATING, SwallowEvent);
    EventUnregister(EVENT_ID_MOUSEDOWN, SwallowEvent);
    EventUnregister(EVENT_ID_MOUSEMOVE, SwallowEvent);
    EventUnregister(EVENT_ID_SIZE, SizeEvent);

    if (s_sizeEventPosted) {
        EventQueuePost(EventGetCurrentContext(), EVENT_ID_SIZE, &s_sizeEvent, sizeof(s_sizeEvent));
    }
}

// VERIFY: 69f3b93a manual
// OFFSET: 0x004094E0
static void ClearDynamicData() {
    if (s_dynamicElementsTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_dynamicElementsTexture));
    }

    s_dynamicElementsTexture = nullptr;

    for (uint32_t i = 0; i < sizeof(s_dynamicBackgroundTextures) / sizeof(s_dynamicBackgroundTextures[0]); i++) {
        if (s_dynamicBackgroundTextures[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(s_dynamicBackgroundTextures[i]));
        }

        s_dynamicBackgroundTextures[i] = nullptr;
    }

    s_dynamicElements.Clear();
}

// VERIFY: 2116833b manual
// OFFSET: 0x00409550
void LoadingScreenDisable() {
    if (s_loadingScreenLayer) {
        HandleClose(reinterpret_cast<HOBJECT>(s_loadingScreenLayer));
    }

    s_loadingScreenLayer = nullptr;

    for (uint32_t i = 0; i < sizeof(s_textures) / sizeof(s_textures[0]); i++) {
        if (s_textures[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(s_textures[i]));
        }

        s_textures[i] = nullptr;
    }

    if (s_simpleBackgroundTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_simpleBackgroundTexture));
    }

    s_simpleBackgroundTexture = nullptr;

    LoadingScreenDisableEvents();
    ClearDynamicData();

    s_gameTip = nullptr;

    if (s_batch) {
        GxuFontDestroyBatch(s_batch);
    }

    s_batch = nullptr;

    if (s_tipStr) {
        GxuFontDestroyString(s_tipStr);
    }

    s_tipStr = nullptr;

    if (s_textFont) {
        HandleClose(reinterpret_cast<HOBJECT>(s_textFont));
    }

    s_textFont = nullptr;

    SI2::StopAllMusic();
    SI2::StopZoneAmbience();

    if (SFile::IsStreamingMode()) {
        CGSpellBook::MakeKnownSpellModelsLocal();
    }
}

// VERIFY: b183edd8 auto
// OFFSET: 0x00409620
void LoadingScreenShutdown() {
    LoadingScreenDisable();

    for (uint32_t i = 0; i < sizeof(s_vertexShader) / sizeof(s_vertexShader[0]); i++) {
        if (s_vertexShader[i]) {
            g_theGxDevicePtr->ShaderDestroy(s_vertexShader[i]);
        }
    }

    if (s_pixelShader[0]) {
        g_theGxDevicePtr->ShaderDestroy(s_pixelShader[0]);
    }
}

// VERIFY: 0d44dabd auto
// OFFSET: 0x00407EF0
void LoadingScreenMiniDisable() {
    if (s_miniLoadingLayer) {
        HandleClose(reinterpret_cast<HOBJECT>(s_miniLoadingLayer));
    }

    s_miniLoadingLayer = nullptr;

    for (uint32_t i = 0; i < sizeof(s_miniTextures) / sizeof(s_miniTextures[0]); i++) {
        if (s_miniTextures[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(s_miniTextures[i]));
        }

        s_miniTextures[i] = nullptr;
    }
}

// VERIFY: 109a7518 auto
// OFFSET: 0x0040A770
void LoadingScreenMiniUpdate(float progress) {
    LoadingScreenMiniEnable();
    s_progress = progress;
}

// VERIFY: c6318812 manual
// OFFSET: 0x0040A160
bool LoadDynamicBackground() {
    CStatus status;

    if (!s_dynamicElementsTexture) {
        CGxTexFlags flags;
        s_dynamicElementsTexture = TextureCreate("Interface\\Glues\\LoadingScreens\\DynamicElements", flags, &status, 1);

        if (!s_dynamicElementsTexture) {
            return false;
        }
    }

    for (uint32_t i = 0; i < 12; i++) {
        char fileName[260];
        SStrPrintf(fileName, sizeof(fileName), "Interface\\WorldMap\\World\\World%d", i + 1);

        CGxTexFlags flags;
        s_dynamicBackgroundTextures[i] = TextureCreate(fileName, flags, &status, 1);

        if (!s_dynamicBackgroundTextures[i]) {
            return false;
        }
    }

    return true;
}

// VERIFY: 4a7d39e7 manual
// OFFSET: 0x00409B10
static void LoadingScreenMiniPaint(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec) {
    if (!GxHasContext() || !GxIsWindowVisible()) {
        return;
    }

    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    float aspect = CoordinateGetAspectCompensation();
    float ratio = s_usingWideScreen ? s_wideAspect / s_normalAspect : 1.0f;
    float scale = aspect / ratio;

    float left = 0.0f;
    float width = 1.0f;
    float bottom = 0.0f;
    float height = 1.0f;

    if (scale > 1.0f) {
        width = 1.0f / scale;
        left = (1.0f - width) * 0.5f;
    } else if (scale < 1.0f) {
        height = scale;
        bottom = (1.0f - scale) * 0.5f;
    }

    GxXformSetViewport(left, width + left, bottom, height + bottom, 0.0f, 1.0f);

    C44Matrix view;
    C44Matrix projection;
    GxuXformCreateOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 500.0f, projection);
    g_theGxDevicePtr->XformSetView(view);
    g_theGxDevicePtr->XformSetProjection(projection);

    g_theGxDevicePtr->RsPush();

    C44Matrix transform;

    if (s_vertexShader[0]->Valid() && s_pixelShader[0]->Valid()) {
        g_theGxDevicePtr->RsSet(GxRs_VertexShader, s_vertexShader[0]);
        g_theGxDevicePtr->RsSet(GxRs_PixelShader, s_pixelShader[0]);
        GxXformViewProjNativeTranspose(transform);
        GxShaderConstantsSet(GxSh_Vertex, 0, &transform.a0, 4);
    } else {
        GxRsSet(GxRs_Lighting, 0);
    }

    GxRsSet(GxRs_Fog, 0);
    PaintLoadingBar(s_miniTextureInfo, 2, s_miniTextures);
    g_theGxDevicePtr->RsPop();
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

// VERIFY: 62c9d5dd manual
// OFFSET: 0x0040A690
void LoadingScreenMiniEnable() {
    if (s_miniLoadingLayer) {
        return;
    }

    CStatus status;

    for (uint32_t i = 0; i < sizeof(s_miniTextures) / sizeof(s_miniTextures[0]); i++) {
        if (!s_miniTextures[i]) {
            CGxTexFlags flags;
            s_miniTextures[i] = TextureCreate(s_miniTextureInfo[i].m_path, flags, &status, 0);
        }
    }

    RECTF rect;
    rect.bottom = 0.0f;
    rect.left = 0.0f;
    rect.top = 1.0f;
    rect.right = 1.0f;
    ScrnLayerCreate(&rect, 9.0f, 3, nullptr, LoadingScreenMiniPaint, &s_miniLoadingLayer);
    s_progress = 0.5f;
}

// VERIFY: 3eb5beca manual
// OFFSET: 0x00408690
static void BuildEndpointVertices(const C3Vector& start, const C3Vector& end) {
    s_dynamicElements.m_data[0].m_position = C3Vector(start.x - 0.02f, start.y - 0.0265f, 0.0f);
    s_dynamicElements.m_data[0].m_texCoord.x = 0.5f;
    s_dynamicElements.m_data[0].m_texCoord.y = 0.5f;
    s_dynamicElements.m_data[0].m_color = s_pathColor;

    s_dynamicElements.m_data[1].m_position = C3Vector(start.x + 0.02f, start.y - 0.0265f, 0.0f);
    s_dynamicElements.m_data[1].m_texCoord.x = 1.0f;
    s_dynamicElements.m_data[1].m_texCoord.y = 0.5f;
    s_dynamicElements.m_data[1].m_color = s_pathColor;

    s_dynamicElements.m_data[2].m_position = C3Vector(start.x - 0.02f, start.y + 0.0265f, 0.0f);
    s_dynamicElements.m_data[2].m_texCoord.x = 0.5f;
    s_dynamicElements.m_data[2].m_texCoord.y = 0.0f;
    s_dynamicElements.m_data[2].m_color = s_pathColor;

    s_dynamicElements.m_data[3].m_position = C3Vector(start.x + 0.02f, start.y + 0.0265f, 0.0f);
    s_dynamicElements.m_data[3].m_texCoord.x = 1.0f;
    s_dynamicElements.m_data[3].m_texCoord.y = 0.0f;
    s_dynamicElements.m_data[3].m_color = s_pathColor;

    s_dynamicElements.m_data[4].m_position = C3Vector(end.x - 0.02f, end.y - 0.0265f, 0.0f);
    s_dynamicElements.m_data[4].m_texCoord.x = 0.0f;
    s_dynamicElements.m_data[4].m_texCoord.y = 0.5f;
    s_dynamicElements.m_data[4].m_color = s_pathColor;

    s_dynamicElements.m_data[5].m_position = C3Vector(end.x + 0.02f, end.y - 0.0265f, 0.0f);
    s_dynamicElements.m_data[5].m_texCoord.x = 0.5f;
    s_dynamicElements.m_data[5].m_texCoord.y = 0.5f;
    s_dynamicElements.m_data[5].m_color = s_pathColor;

    s_dynamicElements.m_data[6].m_position = C3Vector(end.x - 0.02f, end.y + 0.0265f, 0.0f);
    s_dynamicElements.m_data[6].m_texCoord.x = 0.0f;
    s_dynamicElements.m_data[6].m_texCoord.y = 0.0f;
    s_dynamicElements.m_data[6].m_color = s_pathColor;

    s_dynamicElements.m_data[7].m_position = C3Vector(end.x + 0.02f, end.y + 0.0265f, 0.0f);
    s_dynamicElements.m_data[7].m_texCoord.x = 0.5f;
    s_dynamicElements.m_data[7].m_texCoord.y = 0.0f;
    s_dynamicElements.m_data[7].m_color = s_pathColor;
}

// VERIFY: 0d76d9d0 manual
// OFFSET: 0x004089C0
static void BuildPathVertices(C3Spline* spline, int32_t segments) {
    float t = 0.0f;
    float step = 1.0f / segments;
    DYNAMICELEMENTVERT* vertex = &s_dynamicElements.m_data[8];

    for (int32_t i = 0; i < segments + 1; i++) {
        t = t < 0.0f ? 0.0f : (1.0f <= t ? 1.0f : t);

        C3Vector position(t, t, t);
        spline->Pos(t, position, C3Spline::EVAL_ARCLENGTH);

        for (uint32_t j = 0; j < 4; j++) {
            vertex[j].m_position = s_pathOffsets[j] + position;
            vertex[j].m_texCoord = s_pathTexCoords[j];
        }

        vertex += 4;
        t += step;
    }
}

// VERIFY: 8b8c8fa2 manual
// OFFSET: 0x00408BB0
static LoadingScreenTaxiSplinesRec* FindTaxiSpline(int32_t pathID, int32_t legIndex) {
    for (int32_t i = 0; i < g_loadingScreenTaxiSplinesDB.m_numRecords; i++) {
        LoadingScreenTaxiSplinesRec* rec = g_loadingScreenTaxiSplinesDB.GetRecordByIndex(i);

        if (rec->m_pathID == pathID && rec->m_legIndex == legIndex) {
            return rec;
        }
    }

    return nullptr;
}

// VERIFY: 99e42577 manual
// OFFSET: 0x00408E10
void PaintDynamicLoadingBar() {
    if (!s_dynamicElements.Count()) {
        return;
    }

    int32_t segments = (static_cast<int32_t>(s_dynamicElements.Count()) - 8) / 4;
    int32_t lit = 0;

    while (lit < segments) {
        DYNAMICELEMENTVERT* vertex = &s_dynamicElements.m_data[8 + lit * 4];
        vertex[0].m_color = s_pathColor;
        vertex[1].m_color = s_pathColor;
        vertex[2].m_color = s_pathColor;
        vertex[3].m_color = s_pathColor;

        if (static_cast<float>(lit) / static_cast<float>(segments - 1) > s_progress) {
            break;
        }

        lit++;
    }

    uint32_t indexCount = 6 * lit + 12;
    uint32_t vertexCount = 4 * lit + 8;
    uint16_t* indices = static_cast<uint16_t*>(alloca(indexCount * sizeof(uint16_t)));
    uint16_t* index = indices;

    for (int32_t i = 0; i < lit; i++) {
        uint16_t base = static_cast<uint16_t>(8 + i * 4);
        index[0] = base;
        index[1] = base + 1;
        index[2] = base + 2;
        index[3] = base + 1;
        index[4] = base + 3;
        index[5] = base + 2;
        index += 6;
    }

    index[0] = 0;
    index[1] = 1;
    index[2] = 2;
    index[3] = 1;
    index[4] = 3;
    index[5] = 2;
    index[6] = 4;
    index[7] = 5;
    index[8] = 6;
    index[9] = 5;
    index[10] = 7;
    index[11] = 6;

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(s_dynamicElementsTexture, 1, nullptr));
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    DYNAMICELEMENTVERT* vertices = s_dynamicElements.m_data;
    GxPrimLockVertexPtrs(vertexCount, &vertices->m_position, sizeof(DYNAMICELEMENTVERT), nullptr, 0, &vertices->m_color, sizeof(DYNAMICELEMENTVERT), nullptr, 0, &vertices->m_texCoord, sizeof(DYNAMICELEMENTVERT), nullptr, 0);
    GxDrawLockedElements(GxPrim_Triangles, indexCount, indices);
    GxPrimUnlockVertexPtrs();
}

// VERIFY: da792d7a manual
// OFFSET: 0x00409880
static bool InitializeSpline(uint32_t path, int32_t leg) {
    LoadingScreenTaxiSplinesRec* splineRec = FindTaxiSpline(path, leg);

    if (!splineRec) {
        return false;
    }

    int32_t legCount = path < s_shipInfo.m_paths.Count() ? s_shipInfo.m_paths.m_data[path].m_legs.Count() : 0;

    if (!legCount) {
        return false;
    }

    int32_t startNodeID;
    int32_t endNodeID = 0;
    int32_t nextStartNodeID = 0;

    if (!s_shipInfo.GetLeg(path, leg, &startNodeID, &endNodeID)) {
        return false;
    }

    if (!s_shipInfo.GetLeg(path, (leg + 1) % legCount, &nextStartNodeID, &startNodeID)) {
        return false;
    }

    TaxiPathNodeRec* startNode = g_taxiPathNodeDB.GetRecord(endNodeID);
    TaxiPathNodeRec* endNode = g_taxiPathNodeDB.GetRecord(nextStartNodeID);

    if (!startNode || !endNode) {
        return false;
    }

    C3Vector start;
    start.x = startNode->m_loc[0];
    start.y = startNode->m_loc[1];
    start.z = 0.0f;

    C3Vector end;
    end.x = endNode->m_loc[0];
    end.y = endNode->m_loc[1];
    end.z = 0.0f;

    if (!CGWorldMap::GetContinentPosition(start.x, start.y, startNode->m_continentID, &start.x, &start.y) || !CGWorldMap::GetContinentPosition(end.x, end.y, endNode->m_continentID, &end.x, &end.y)) {
        return false;
    }

    TSStackArray<C3Vector> points(static_cast<C3Vector*>(alloca(sizeof(C3Vector) * 12)), 12, 1);
    points.m_data[1] = start;
    points.m_data[0] = points.m_data[1];

    uint32_t count = 0;

    while (count < 8 && (splineRec->m_locx[count] > 0.0f || splineRec->m_locy[count] > 0.0f)) {
        points.m_data[count + 2].x = splineRec->m_locx[count];
        points.m_data[count + 2].y = splineRec->m_locy[count];
        points.m_data[count + 2].z = 0.0f;
        count++;
    }

    points.m_data[count + 3] = end;
    points.m_data[count + 2] = points.m_data[count + 3];

    C3Spline_CatmullRom spline;
    spline.SetPoints(points.m_data, count + 4);

    int32_t segments = FloatToInt64(spline.cachedLength * 66.666672f);

    if (segments < 1) {
        segments = 1;
    }

    s_dynamicElements.SetCount(4 * segments + 12);
    BuildEndpointVertices(start, end);
    BuildPathVertices(&spline, segments);

    return true;
}

// VERIFY: 6a24a8ca manual
// OFFSET: 0x0040AD50
void LoadingScreenEnableShip(uint32_t transportEntry, int32_t transportMap, int32_t mapID) {
    s_simpleMapID = mapID;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        CGGameObject_C* transport = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(WOWGUID{ player->GetTransportGUID() }, TYPE_GAMEOBJECT, __FILE__, __LINE__));

        if (transport && transport->m_obj->m_entryID == transportEntry) {
            uint32_t path = transport->GetPropertyValue(CGameObjectDef::GetPropNum(transport->m_gameObject->m_typeID, 35));
            uint32_t leg = s_shipInfo.FindLeg(&g_taxiPathNodeDB, path, transportMap);

            if (!InitializeSpline(path, leg) || !LoadDynamicBackground()) {
                ClearDynamicData();
                InitializeProgressBar(true);
                return;
            }

            s_simpleMapID = -1;
        }
    }

    InitializeProgressBar(true);
}

// VERIFY: 06b933e0 manual
// OFFSET: 0x0040AE30
void LoadingScreenEnableShipLeg(int32_t entryId, uint32_t leg) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        CGGameObject_C* transport = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(WOWGUID{ player->GetTransportGUID() }, TYPE_GAMEOBJECT, __FILE__, __LINE__));

        if (transport && transport->m_obj->m_entryID == entryId) {
            uint32_t path = transport->GetPropertyValue(CGameObjectDef::GetPropNum(transport->m_gameObject->m_typeID, 35));

            if (!InitializeSpline(path, leg) || !LoadDynamicBackground()) {
                ClearDynamicData();
                InitializeProgressBar(true);
                return;
            }

            s_simpleMapID = -1;
        }
    }

    InitializeProgressBar(true);
}
