#include <cstddef>
#include <cstdint>
#include <new>
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Images/ImageWriter.h"
#include "Engine/Source/Images/tga.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Scrn/Layer.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

// STATIC: 0x009EDCB8
static const RECTF s_defaultRect = { 0.0f, 0.0f, 1.0f, 1.0f };

// STATIC: 0x00AC1B88
static char s_screenshotFormat[16] = "jpeg";

// STATIC: 0x00AC1B98
static uint32_t s_screenshotQuality = 60;

// STATIC: 0x00AC1B9C
static TSExplicitList<CILayer, offsetof(CILayer, m_zorderlink)> s_zorderlist;

// STATIC: 0x00B49A10
static float s_elapsedSec;

// STATIC: 0x00B49A14
static void (*s_screenshotCallback)(int32_t success);

// STATIC: 0x00B49A18
static int32_t s_noPresent;

// STATIC: 0x00B49A1C
static uint8_t* s_screenshotData;

// STATIC: 0x00B49A20
static uint32_t s_screenshotDataSize;

// VERIFY: 87ac0fd7 manual
// OFFSET: 0x004A8420
static void* LAYER_MemAlloc(size_t bytes) {
    return SMemAlloc(bytes, __FILE__, __LINE__, 0);
}

// VERIFY: fb9e21ad manual
// OFFSET: 0x004A8440
static void LAYER_MemFree(void* ptr) {
    SMemFree(ptr, __FILE__, __LINE__, 0);
}

// VERIFY: cbcfc981 auto
// OFFSET: 0x004A8460
static int32_t OnIdle(const void* data, void* param) {
    s_elapsedSec += static_cast<const EVENT_DATA_IDLE*>(data)->m_elapsedSec;

    return 1;
}

// VERIFY: f03a908c auto
// OFFSET: 0x004A8480
void ScrnSetScreenshotFormat(const char* format) {
    SStrCopy(s_screenshotFormat, format, sizeof(s_screenshotFormat));
}

// VERIFY: 228fd240 manual
// OFFSET: 0x004A84A0
void ScrnSetScreenshotQuality(uint32_t quality) {
    if (quality > 10) {
        quality = 10;
    } else if (quality < 1) {
        quality = 1;
    }

    s_screenshotQuality = static_cast<uint32_t>(quality * 5.5f + 45.0f);
}

// VERIFY: 8f3185ff auto
// OFFSET: 0x004A8500
void ScrnLayerSetRect(HLAYER layer, const RECTF* rect) {
    reinterpret_cast<CILayer*>(layer)->m_rect = *rect;
}

// VERIFY: e30052be auto
// OFFSET: 0x004A8530
uint32_t ScrnLayerGetFlags(HLAYER layer) {
    return reinterpret_cast<CILayer*>(layer)->m_flags;
}

// VERIFY: 4573bf37 auto
// OFFSET: 0x004A8540
void ScrnLayerSetFlags(HLAYER layer, uint32_t flags) {
    reinterpret_cast<CILayer*>(layer)->m_flags = flags;
}

// VERIFY: 2c71a8fb auto
// OFFSET: 0x004A8550
void ScrnScreenshot(void (*callback)(int32_t success), uint8_t* data, uint32_t size) {
    s_screenshotCallback = callback;
    s_screenshotData = data;
    s_screenshotDataSize = size;
}

// VERIFY: 90b31936 auto
// OFFSET: 0x004A8570
static int32_t CaptureScreen() {
    uint32_t width;
    uint32_t height;
    const CImVector* pixels;

    g_theGxDevicePtr->DeviceReadScreenShot(width, height, pixels);

    if (pixels && ImageWriterInitialize(s_screenshotDataSize, LAYER_MemAlloc, LAYER_MemFree, 0)) {
        OsCreateDirectory("Screenshots", 0);

        char timeStamp[260];
        OsGetTimeStamp(timeStamp, sizeof(timeStamp));

        if (SStrCmpI(s_screenshotFormat, "tga", STORM_MAX_STR) && SStrCmpI(s_screenshotFormat, "png", STORM_MAX_STR) && s_screenshotQuality < 100) {
            ImageWriterEmbedData(pixels, width, height, 4, s_screenshotData, s_screenshotDataSize);
        }

        char fileName[260];

        if (!SStrCmpI(s_screenshotFormat, "tga", STORM_MAX_STR)) {
            SStrPrintf(fileName, sizeof(fileName), "%s\\WoWScrnShot_%s.tga", "Screenshots", timeStamp);

            CTgaFile tga;
            tga.SetImage(pixels, width, height, 32, 0, 1, 0);
            tga.RemoveAlphaChannels();
            tga.Compress();
            tga.Write(fileName);
            tga.Close();
        } else {
            SStrPrintf(fileName, sizeof(fileName), "%s\\WoWScrnShot_%s.jpg", "Screenshots", timeStamp);
            ImageWriterWriteJpeg(fileName, pixels, width, height, 4, s_screenshotQuality);
        }

        ImageWriterShutdown();
    }

    return pixels != nullptr;
}

// VERIFY: 52c6418c manual
// OFFSET: 0x004A8720
static int32_t OnPaint(const void* data, void* param) {
    if (GxHasContext() && GxIsWindowVisible()) {
        // Not in the original: nothing of this frame is drawn yet.
        sExtensionMgr->OnFrameBegin(s_elapsedSec);

        HSRGN rgn;
        SRgnCreate(&rgn, 0);

        RECTF screen = { 0.0f, 0.0f, 1.0f, 1.0f };
        SRgnCombineRectf(rgn, &screen, nullptr, 2);

        for (CILayer* layer = s_zorderlist.Tail(); layer; layer = s_zorderlist.Prev(layer)) {
            SRgnGetBoundingRectf(rgn, &layer->m_visible);

            layer->m_visible.left = layer->m_rect.left >= layer->m_visible.left ? layer->m_rect.left : layer->m_visible.left;
            layer->m_visible.bottom = layer->m_rect.bottom >= layer->m_visible.bottom ? layer->m_rect.bottom : layer->m_visible.bottom;
            layer->m_visible.right = layer->m_rect.right <= layer->m_visible.right ? layer->m_rect.right : layer->m_visible.right;
            layer->m_visible.top = layer->m_rect.top <= layer->m_visible.top ? layer->m_rect.top : layer->m_visible.top;

            if (!(layer->m_flags & 1)) {
                SRgnCombineRectf(rgn, &layer->m_rect, nullptr, 4);
            }
        }

        SRgnDelete(rgn);

        float minX;
        float maxX;
        float minY;
        float maxY;
        float minZ;
        float maxZ;
        GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

        CILayer* next;

        for (CILayer* layer = s_zorderlist.Head(); !s_zorderlist.IsEnd(layer); layer = next) {
            next = s_zorderlist.RawNext(layer);

            if (layer->m_visible.left < layer->m_visible.right && layer->m_visible.bottom < layer->m_visible.top) {
                if (layer->m_flags & 4) {
                    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
                } else {
                    GxXformSetViewport(layer->m_visible.left, layer->m_visible.right, layer->m_visible.bottom, layer->m_visible.top, 0.0f, 1.0f);
                }

                if (layer->m_flags & 2) {
                    C44Matrix view;
                    C44Matrix proj;
                    GxuXformCreateOrtho(layer->m_visible.left, layer->m_visible.right, layer->m_visible.bottom, layer->m_visible.top, 0.0f, 500.0f, proj);
                    GxXformSetView(view);
                    GxXformSetProjection(proj);
                }

                layer->m_paintFunc(layer->m_param, &layer->m_rect, &layer->m_visible, s_elapsedSec);
            }
        }

        GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
        GxuFontUpdate();

        // Not in the original: every layer has painted, the scene is not presented yet.
        sExtensionMgr->OnFrameEnd();

        if (!s_noPresent) {
            if (s_screenshotCallback) {
                g_theGxDevicePtr->DeviceTakeScreenShot();
                GxScenePresent();
                s_screenshotCallback(CaptureScreen());
                g_theGxDevicePtr->DeviceClearScreenShot();
                s_elapsedSec = 0.0f;
                s_screenshotCallback = nullptr;

                return 1;
            }

            GxScenePresent();
        }

        s_elapsedSec = 0.0f;
    } else {
        GxScenePresent();

        if (OsSleepInBackground()) {
            uint32_t count = OsGetBackgroundSleepMs() / 25;

            for (uint32_t i = 0; i < count; i++) {
                OsSleep(25);
                SE2::HeartBeat(nullptr, nullptr);
            }

            uint32_t remainder = OsGetBackgroundSleepMs() - count * 25;

            if (remainder) {
                OsSleep(remainder);
            }
        }
    }

    return 1;
}

// VERIFY: 04c6e74c auto
// OFFSET: 0x004A8B80
void ILayerInitialize() {
    EventRegister(EVENT_ID_IDLE, OnIdle);
    EventRegister(EVENT_ID_PAINT, OnPaint);
}

// VERIFY: 7c08fb34 auto
// OFFSET: 0x004A8BA0
static void ILayerPaint() {
    OnPaint(nullptr, nullptr);
}

// VERIFY: 38fdd351 manual
// OFFSET: 0x004A8BB0
void ScrnLayerCreate(const RECTF* rect, float zorder, uint32_t flags, void* param, void (*paintFunc)(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec), HLAYER* layer) {
    const RECTF* layerRect = rect ? rect : &s_defaultRect;

    void* mem = SMemAlloc(sizeof(CILayer), "HLAYER", SLOG_OBJECT, 0);
    CILayer* ilayer = mem ? new (mem) CILayer : nullptr;

    ilayer->m_rect = *layerRect;
    ilayer->m_zorder = zorder;
    ilayer->m_flags = flags;
    ilayer->m_param = param;
    ilayer->m_paintFunc = paintFunc;

    CILayer* existing;

    for (existing = s_zorderlist.Tail(); existing && zorder < existing->m_zorder; existing = s_zorderlist.Prev(existing)) {
    }

    s_zorderlist.LinkNode(ilayer, 1, existing);

    *layer = reinterpret_cast<HLAYER>(HandleCreate(ilayer, "HLAYER"));
}

// VERIFY: fc689bb0 manual
// OFFSET: 0x004A8C90
CILayer::~CILayer() {
}

// VERIFY: 393d0327 auto
// OFFSET: 0x004A8B60
void ILayerDestroy() {
    EventUnregister(EVENT_ID_IDLE, OnIdle);
    EventUnregister(EVENT_ID_PAINT, OnPaint);
}

// VERIFY: 867daeb1 auto
// OFFSET: 0x004A8D00
void ScrnLayerPaint() {
    ILayerPaint();
}
