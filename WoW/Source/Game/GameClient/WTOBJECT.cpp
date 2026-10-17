#include "WTOBJECT.h"
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include <typeinfo>

// STATIC: 0x00D38074
static uint32_t s_count;

// STATIC: 0x00D38078
static CDataAllocator s_freeSwings(sizeof(SWING), 8);

// STATIC: 0x00D3808C
static CDataAllocator s_freeTrails(sizeof(WTOBJECT), 128);

// INLINED
WTOBJECT::~WTOBJECT() {
    if (this->m_model) {
        CM2Model* model = this->m_model;
        model->m_fFlags |= 0x20;
        model->m_drawCallback = nullptr;
        model->m_drawArg = nullptr;
        this->m_model->Release();
    }

    if (this->m_swing) {
        s_freeSwings.PutData(this->m_swing, nullptr, 0);
    }
}

// VERIFY: c3c7b136 auto
// OFFSET: 0x007E4BC0
void WeaponTrailsShutdown() {
    s_freeTrails.Clear(STORM_TYPE_NAME(WTOBJECT), SLOG_OBJECT, 1);
    s_freeSwings.Clear(STORM_TYPE_NAME(SWING), SLOG_OBJECT, 1);
}

// VERIFY: 38b83c0d auto
// OFFSET: 0x007E4BF0
WTOBJECT* WeaponTrailCreate(CM2Model* model) {
    s_count++;
    WTOBJECT* trail = static_cast<WTOBJECT*>(s_freeTrails.GetData(0, STORM_TYPE_NAME(WTOBJECT), SLOG_OBJECT));

    if (trail) {
        trail->m_swing = nullptr;
        trail->m_model = nullptr;
    }

    trail->m_model = model;
    model->m_refCount++;
    return trail;
}

// VERIFY: 254754a7 auto
// OFFSET: 0x007E4CC0
void WeaponTrailClose(WTOBJECT* trail) {
    s_count--;
    s_freeTrails.Delete(trail);
}

// VERIFY: 918160ea manual
// OFFSET: 0x007E4B70
void SWING::FillVertices(CImVector color, uint8_t fade, CGxVertexPC* vertices) {
    for (int32_t i = this->m_tail; i < this->m_head; i++, vertices++) {
        vertices->p = this->m_points[i & 0x7F];

        if (!(i & 0x1)) {
            color.a -= fade;
        }

        vertices->c = color;
    }
}

// VERIFY: b7752821 manual
// OFFSET: 0x007E4C30
SWING::SWING() {
    this->m_tail = 0;
    this->m_head = 0;
    *reinterpret_cast<uint32_t*>(&this->m_color) = 0;
}

// VERIFY: 76207940 manual
// OFFSET: 0x007E4CE0
bool SWING::BuildVertexBuffer(uint32_t time, const C3Vector& top, const C3Vector& bottom) {
    uint32_t elapsed = time - this->m_startTime;

    if (elapsed < this->m_duration) {
        int32_t index = this->m_head++;

        if (this->m_tail < this->m_head - 128) {
            this->m_tail = this->m_head - 128;
        }

        this->m_points[index & 0x7F] = bottom;
        index = this->m_head++;

        if (this->m_tail < this->m_head - 128) {
            this->m_tail = this->m_head - 128;
        }

        this->m_points[index & 0x7F] = top;
    }

    uint8_t alpha = this->m_color.a;
    int32_t delta = time - this->m_lastTime;
    this->m_lastTime = time;
    uint8_t fade = static_cast<uint8_t>(FloatToInt64(delta * 0.0033333334f * alpha));

    if (!fade) {
        fade = 1;
    }

    int32_t segments = alpha / fade;

    if (segments <= 1) {
        return false;
    }

    int32_t head = this->m_head;

    if (this->m_tail < head - 2 * segments) {
        this->m_tail = head - 2 * segments;
    }

    int32_t count = head - this->m_tail;

    if (!count) {
        return false;
    }

    if (count >= 3) {
        g_theGxDevicePtr->RsPush();
        GxRsSet(GxRs_Culling, 0);
        GxRsSet(GxRs_Fog, 0);
        GxRsSet(GxRs_Lighting, 0);
        GxRsSet(GxRs_BlendingMode, GxBlend_Alpha);
        GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
        GxRsSet(GxRs_DepthWrite, 0);
        GxRsSet(GxRs_DepthTest, 1);
        GxRsSet(GxRs_Texture0, 0);
        GxRsSet(GxRs_Texture1, 0);

        CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPC), count);
        CGxVertexPC* vertices = reinterpret_cast<CGxVertexPC*>(g_theGxDevicePtr->BufLock(buf));
        this->FillVertices(GxSwizzleColor(this->m_color), fade, vertices);

        if (static_cast<int32_t>(elapsed) > static_cast<int32_t>(this->m_duration >> 1)) {
            this->m_color.a -= fade;
        }

        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;
        GxPrimVertexPtr(buf, GxVBF_PC);

        CGxBatch batch;
        batch.m_primType = GxPrim_TriangleStrip;
        batch.m_start = 0;
        batch.m_count = count;
        batch.m_minIndex = 0;
        batch.m_maxIndex = count - 1;
        g_theGxDevicePtr->Draw(&batch, 0);
        g_theGxDevicePtr->RsPop();
    }

    return true;
}

// VERIFY: 1e058dc3 auto
// OFFSET: 0x007E4F50
static void WeaponTrailCallback(CM2Model* model, const CM2Lighting* lighting, void* arg) {
    if (!model->HasEvent('BTW$') || !model->HasEvent('TTW$')) {
        return;
    }

    uint32_t time = FrameTime::s_curTimeMs;
    C3Vector bottom = model->GetEventPosition('BTW$');
    C3Vector top = model->GetEventPosition('TTW$');
    WTOBJECT* trail = static_cast<WTOBJECT*>(arg);

    if (!trail->m_swing->BuildVertexBuffer(time, top, bottom)) {
        s_freeSwings.PutData(trail->m_swing, nullptr, 0);
        trail->m_swing = nullptr;
        model->m_fFlags |= 0x20;
        model->m_drawCallback = nullptr;
        model->m_drawArg = nullptr;
    }
}

// VERIFY: 2687134f auto
// OFFSET: 0x007E4FF0
void WTOBJECT::SetDrawTrail(const uint32_t* color, uint32_t duration) {
    if (this->m_swing) {
        this->m_swing->m_tail = 0;
        this->m_swing->m_head = 0;
    } else {
        SWING* swing = static_cast<SWING*>(s_freeSwings.GetData(0, STORM_TYPE_NAME(SWING), SLOG_OBJECT));

        if (swing) {
            new (swing) SWING();
        }

        CM2Model* model = this->m_model;
        this->m_swing = swing;
        model->m_fFlags |= 0x20;
        model->m_drawCallback = WeaponTrailCallback;
        model->m_drawArg = this;
    }

    *reinterpret_cast<uint32_t*>(&this->m_swing->m_color) = *color;
    this->m_swing->m_duration = duration;
    this->m_swing->m_startTime = FrameTime::s_curTimeMs;
    this->m_swing->m_lastTime = this->m_swing->m_startTime;
}

// VERIFY: b37df885 auto
// OFFSET: 0x007E5080
void WeaponTrailSetDrawTrail(WTOBJECT* trail, const uint32_t* a2, uint32_t a3) {
    if (a3 && trail) {
        trail->SetDrawTrail(a2, a3);
    }
}
