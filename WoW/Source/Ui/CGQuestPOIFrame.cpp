#include "CGQuestPOIFrame.h"
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include <cfloat>
#include <cmath>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Tempest/C3Spline_CatmullRom.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/WorldMapAreaRec.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/WorldMap.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00ACF17C
const char* CGQuestPOIFrame::s_objectTypeName = "QuestPOIFrame";

// STATIC: 0x00ACF1F0
int32_t CGQuestPOIFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00C0D7C4
int32_t CGQuestPOIFrame::s_objectType;

// STATIC: 0x00C0D7C8
CDataAllocator CGQuestPOIFrame::s_questPOIHeap(sizeof(CGQuestPOIFrame), 5);

// INLINED
void CGQuestPOIFrame::operator delete(void* ptr) {
    if (ptr) {
        CGQuestPOIFrame::s_questPOIHeap.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CGQuestPOIFrame::GetObjectType() {
    if (!CGQuestPOIFrame::s_objectType) {
        CGQuestPOIFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CGQuestPOIFrame::s_objectType;
}

// VERIFY: f4d221d4 auto
// OFFSET: 0x00514260
CSimpleFrame* CGQuestPOIFrame::Create(CSimpleFrame* parent) {
    if (CGQuestPOIFrame::s_questPOIHeap.m_bytesPerData < sizeof(CGQuestPOIFrame)) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    void* data = CGQuestPOIFrame::s_questPOIHeap.GetData(0, typeid(CGQuestPOIFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CGQuestPOIFrame(parent) : nullptr;
}

// VERIFY: 766b947d auto
// OFFSET: 0x0058ECF0
CGQuestPOIFrame::QuestPOIRenderData::~QuestPOIRenderData() {
}

// VERIFY: b799b5fe manual
// OFFSET: [0x0058F9D0, 0x0058FAB0, 0x00590060]
CGQuestPOIFrame::~CGQuestPOIFrame() {
    if (this->m_fillTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_fillTexture));
        this->m_fillTexture = nullptr;
    }

    if (this->m_borderTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_borderTexture));
        this->m_borderTexture = nullptr;
    }
}

// VERIFY: 4905382c auto
// OFFSET: 0x0058FA90
const char* CGQuestPOIFrame::GetObjectTypeName() {
    return CGQuestPOIFrame::s_objectTypeName;
}

// VERIFY: d10ef621 auto
// OFFSET: 0x0058FAA0
int32_t CGQuestPOIFrame::GetScriptMetaTable() {
    return CGQuestPOIFrame::s_metatable;
}

// VERIFY: 50cca9d9 auto
// OFFSET: 0x0058FAC0
bool CGQuestPOIFrame::IsA(int32_t type) {
    return type == CGQuestPOIFrame::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 4f27cf4a auto
// OFFSET: 0x0058FB00
bool CGQuestPOIFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CGQuestPOIFrame::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 2f4eb365 manual
// OFFSET: 0x0058FD30
void CGQuestPOIFrame::SetFillTexture(const char* fileName) {
    if (this->m_fillTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_fillTexture));
        this->m_fillTexture = nullptr;
    }

    CStatus status;
    this->m_fillTexture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
}

// VERIFY: ca0d2a19 manual
// OFFSET: 0x0058FDC0
void CGQuestPOIFrame::SetBorderTexture(const char* fileName) {
    if (this->m_borderTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_borderTexture));
        this->m_borderTexture = nullptr;
    }

    CStatus status;
    this->m_borderTexture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
}

// VERIFY: 978cf0cb manual
// OFFSET: 0x0058FF50
CGQuestPOIFrame::CGQuestPOIFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->SetProtected();
    this->DisableEvent(SIMPLE_EVENT_MOUSE);
    this->m_borderScalar = 1.0f;
    this->m_fillAlpha = 0xFF;
    this->m_borderAlpha = 0xFF;
    this->m_mergeThreshold = 0.25f;
    this->m_fillTexture = nullptr;
    this->m_borderTexture = nullptr;
    this->m_mouseOverQuestId = 0;
    this->m_mouseOverCount = 0;
    this->m_smoothing = 1;
    this->m_merging = 1;
    this->m_numSplinePoints = 20;

    for (uint32_t i = 0; i < sizeof(this->m_groups) / sizeof(this->m_groups[0]); i++) {
        this->m_groups[i].m_info = nullptr;
    }
}

// VERIFY: f41da0ed auto
// OFFSET: 0x00590090
void CGQuestPOIFrame::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    const char* fillTexture = node->GetAttributeByName("filltexture");

    if (fillTexture && *fillTexture) {
        this->SetFillTexture(fillTexture);
    }

    const char* borderTexture = node->GetAttributeByName("bordertexture");

    if (borderTexture && *borderTexture) {
        this->SetBorderTexture(borderTexture);
    }
}

// VERIFY: d36106ae auto
// OFFSET: 0x005905D0
void CGQuestPOIFrame::RenderCallback(void* param) {
    if (param) {
        static_cast<CGQuestPOIFrame*>(param)->Render();
    }
}

// VERIFY: 4d0a681d auto
// OFFSET: 0x00590660
void CGQuestPOIFrame::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    CSimpleFrame::OnFrameRender(batch, layer);

    if (layer == 3) {
        batch->QueueCallback(CGQuestPOIFrame::RenderCallback, this);
    }
}

// STATIC: 0x00ACF24C
static int32_t s_highlightIndex = -1;

// VERIFY: 563f2f34 auto
// OFFSET: 0x0058E0D0
void CGQuestPOIFrame::UpdateMouseOverPOI(float x, float y) {
    uint32_t count = 0;
    this->m_mouseOverQuestId = 0;
    this->m_mouseOverCount = 0;
    this->m_mouseOverObjectives[0] = 0;
    this->m_mouseOverObjectives[1] = 0;
    this->m_mouseOverObjectives[2] = 0;
    this->m_mouseOverObjectives[3] = 0;

    if (x < 0.0f || y < 0.0f || x > 1.0f || y > 1.0f) {
        return;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());

    if (CGWorldMap::GetCurrentAreaID() == -1 || !area) {
        return;
    }

    int32_t questId = 0;
    C3Vector point;
    point.z = 0.0f;
    point.x = (this->m_rect.maxX - this->m_rect.minX) * x + this->m_rect.minX;
    point.y = (this->m_rect.maxY - this->m_rect.minY) * (1.0f - y) + this->m_rect.minY;

    for (uint32_t i = 0; i < 4; i++) {
        QuestPOIRenderGroup* group = &this->m_groups[i];

        if (!group->m_info || !group->m_info->m_visible) {
            continue;
        }

        for (uint32_t j = 0; j < group->m_renderData.Count(); j++) {
            int32_t mask = group->m_info->m_objectiveMask;

            if (mask == -1) {
                continue;
            }

            QuestPOIRenderData* data = &group->m_renderData.m_data[j];

            if (!((1 << data->m_blob->m_objectiveIndex) & mask) || !CGQuestLog::IsPOIOnCurrentMap(&data->m_blob->m_centerX, data->m_blob->m_mapId, area->m_mapID)) {
                continue;
            }

            if (data->m_fillVerts.Count() < 3 || !data->m_valid) {
                continue;
            }

            if (!NTempest::Intersect(point, data->m_fillVerts.m_data, data->m_fillVerts.Count() - 1, C3Vector::C3AXIS_Z)) {
                continue;
            }

            count = data->m_numObjectives;
            questId = group->m_info->m_questId;

            for (uint32_t k = 0; k < data->m_numObjectives; k++) {
                this->m_mouseOverObjectives[k] = data->m_objectives[k];
            }
        }

        if (count) {
            break;
        }
    }

    if (questId && count) {
        this->m_mouseOverQuestId = questId;
        this->m_mouseOverCount = count;
    }
}

// VERIFY: 8438da2b manual
// OFFSET: 0x0058E310
void CGQuestPOIFrame::MergeBlobs(uint32_t index) {
    QuestPOIRenderGroup* group = &this->m_groups[index];
    int32_t mask = group->m_info->m_objectiveMask;

    if (mask == -1) {
        return;
    }

    for (uint32_t i = 0; i < group->m_renderData.Count(); i++) {
        QuestPOIRenderData* first = &group->m_renderData.m_data[i];

        if (!first->m_valid || !((1 << first->m_blob->m_objectiveIndex) & mask)) {
            continue;
        }

        for (uint32_t j = i + 1; j < group->m_renderData.Count(); j++) {
            QuestPOIRenderData* second = &group->m_renderData.m_data[j];

            if (!second->m_valid || !((1 << second->m_blob->m_objectiveIndex) & mask)) {
                continue;
            }

            CRect overlap = first->m_bounds & second->m_bounds;

            if (!(overlap.maxY > overlap.minY) || !(overlap.maxX > overlap.minX)) {
                continue;
            }

            QuestPOIRenderData* smaller;
            QuestPOIRenderData* larger;

            if ((second->m_bounds.maxX - second->m_bounds.minX) * (second->m_bounds.maxY - second->m_bounds.minY) <= (first->m_bounds.maxX - first->m_bounds.minX) * (first->m_bounds.maxY - first->m_bounds.minY)) {
                smaller = second;
                larger = first;
            } else {
                smaller = first;
                larger = second;
            }

            uint32_t inside = 0;

            for (uint32_t k = 0; k < smaller->m_fillVerts.Count() - 1; k++) {
                if (NTempest::Intersect(smaller->m_fillVerts.m_data[k], larger->m_fillVerts.m_data, larger->m_fillVerts.Count() - 1, C3Vector::C3AXIS_Z)) {
                    inside++;
                }
            }

            if (!(static_cast<float>(inside) / static_cast<float>(smaller->m_fillVerts.Count() - 1) > this->m_mergeThreshold)) {
                continue;
            }

            smaller->m_valid = 0;

            for (uint32_t k = 0; k < smaller->m_numObjectives; k++) {
                if (larger->m_numObjectives >= 4) {
                    continue;
                }

                uint32_t m = 0;

                while (m < larger->m_numObjectives && larger->m_objectives[m] != smaller->m_objectives[k]) {
                    m++;
                }

                if (m >= larger->m_numObjectives) {
                    larger->m_objectives[larger->m_numObjectives++] = smaller->m_objectives[k];
                }
            }
        }
    }
}

// VERIFY: b0ec5fe5 auto
// OFFSET: 0x0058E5C0
static bool __stdcall IntersectLines(const C3Vector& a0, const C3Vector& a1, const C3Vector& b0, const C3Vector& b1, C3Vector* result) {
    float ax = a0.x - a1.x;
    float bx = b0.x - b1.x;
    float denom = (b0.y - b1.y) * ax - (a0.y - a1.y) * bx;

    if (fabs(denom) < 0.00000023841858f) {
        return false;
    }

    float ca = a0.x * a1.y - a1.x * a0.y;
    float cb = b0.x * b1.y - b1.x * b0.y;
    result->x = (bx * ca - ax * cb) * (1.0f / denom);
    result->y = 1.0f / denom * ((b0.y - b1.y) * ca - (a0.y - a1.y) * cb);
    result->z = 0.0f;
    return true;
}

// VERIFY: cdc6318a manual
// OFFSET: 0x0058ED80
void CGQuestPOIFrame::BuildBlobOutline(QuestPOIBlob* blob, TSGrowableArray<QuestPOIPoint>* points) {
    if (!this->m_smoothing) {
        points->SetCount(blob->m_points.Count());

        for (uint32_t i = 0; i < blob->m_points.Count(); i++) {
            points->m_data[i] = blob->m_points.m_data[i];
        }

        return;
    }

    C3Spline_CatmullRom spline;
    C3Vector controls[15];
    uint32_t count = blob->m_points.Count();
    controls[0] = C3Vector(static_cast<float>(blob->m_points.m_data[count - 1].m_x), static_cast<float>(blob->m_points.m_data[count - 1].m_y), 0.0f);

    for (uint32_t i = 0; i < count; i++) {
        controls[i + 1] = C3Vector(static_cast<float>(blob->m_points.m_data[i].m_x), static_cast<float>(blob->m_points.m_data[i].m_y), 0.0f);
    }

    controls[count + 1] = C3Vector(static_cast<float>(blob->m_points.m_data[0].m_x), static_cast<float>(blob->m_points.m_data[0].m_y), 0.0f);
    controls[count + 2] = C3Vector(static_cast<float>(blob->m_points.m_data[1].m_x), static_cast<float>(blob->m_points.m_data[1].m_y), 0.0f);
    spline.SetPoints(controls, count + 3);

    float t = 0.01f;

    for (uint32_t i = 0; i < this->m_numSplinePoints; i++) {
        C3Vector position(0.0f, 0.0f, 0.0f);
        spline.Pos(t, position, C3Spline::EVAL_ARCLENGTH);
        int32_t x = static_cast<int32_t>(position.x);
        int32_t y = static_cast<int32_t>(position.y);
        QuestPOIPoint* point = points->New();

        if (point) {
            point->m_x = x;
            point->m_y = y;
        }

        t = 1.0f / this->m_numSplinePoints + t;
    }
}

// VERIFY: 41e3f515 manual
// OFFSET: 0x0058F1A0
void CGQuestPOIFrame::BuildBlobRenderData(QuestPOIRenderData* data, QuestPOIBlob* blob) {
    if (!blob) {
        if (data) {
            data->m_blob = nullptr;
            data->m_valid = 0;
        }

        return;
    }

    data->m_blob = blob;
    WorldMapAreaRec* area;

    if (blob->m_points.Count() < 3 || !(area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID()))) {
        data->m_valid = 0;
        return;
    }

    TSGrowableArray<QuestPOIPoint> points;
    this->BuildBlobOutline(blob, &points);
    uint32_t count = points.Count();
    data->m_fillVerts.SetCount(count + 1);
    data->m_fillTexCoords.SetCount(count + 1);
    data->m_fillIndices.SetCount(3 * count);

    C3Vector center(0.0f, 0.0f, 0.0f);
    data->m_objectives[0] = 0;
    data->m_objectives[1] = 0;
    data->m_objectives[2] = 0;
    data->m_objectives[3] = 0;
    data->m_objectives[0] = blob->m_objectiveIndex;
    data->m_numObjectives = 1;
    bool failed = false;

    for (uint32_t i = 0; i < count; i++) {
        C3Vector position(static_cast<float>(points.m_data[i].m_x), static_cast<float>(points.m_data[i].m_y), 0.0f);
        float x;
        float y;
        bool outland = false;

        if (!CGWorldMap::GetWorldPosition(position, area->m_mapID, &x, &y, &outland, false, 0) || outland || fabs(fabs(x * y)) < 0.001f) {
            failed = true;
            break;
        }

        C3Vector& vertex = data->m_fillVerts.m_data[i];
        vertex.x = x * (this->m_rect.maxX - this->m_rect.minX) + this->m_rect.minX;
        vertex.y = (1.0f - y) * (this->m_rect.maxY - this->m_rect.minY) + this->m_rect.minY;
        vertex.z = 1.0f;
        data->m_fillTexCoords.m_data[i] = C2Vector(1.0f, 1.0f);
        center.x += vertex.x;
        center.y += vertex.y;
        center.z += vertex.z;
        data->m_fillIndices.m_data[3 * i] = static_cast<uint16_t>(i);
        data->m_fillIndices.m_data[3 * i + 1] = static_cast<uint16_t>((i + 1) % count);
        data->m_fillIndices.m_data[3 * i + 2] = static_cast<uint16_t>(count);
    }

    data->m_valid = !failed;

    if (failed) {
        data->m_fillVerts.Clear();
        data->m_fillTexCoords.Clear();
        data->m_fillIndices.Clear();
        data->m_borderVerts.Clear();
        data->m_borderTexCoords.Clear();
        data->m_borderIndices.Clear();
        return;
    }

    data->m_bounds.minX = FLT_MAX;
    data->m_bounds.maxX = FLT_MIN;
    data->m_bounds.maxY = FLT_MIN;
    data->m_bounds.minY = FLT_MAX;

    for (uint32_t i = 0; i < data->m_fillVerts.Count() - 1; i++) {
        const C3Vector& vertex = data->m_fillVerts.m_data[i];
        data->m_bounds.minX = vertex.x <= data->m_bounds.minX ? vertex.x : data->m_bounds.minX;
        data->m_bounds.maxX = vertex.x >= data->m_bounds.maxX ? vertex.x : data->m_bounds.maxX;
        data->m_bounds.minY = vertex.y <= data->m_bounds.minY ? vertex.y : data->m_bounds.minY;
        data->m_bounds.maxY = vertex.y >= data->m_bounds.maxY ? vertex.y : data->m_bounds.maxY;
    }

    float scale = 1.0f / count;
    center.x *= scale;
    center.y *= scale;
    center.z *= scale;
    data->m_fillVerts.m_data[count] = center;
    data->m_fillTexCoords.m_data[count] = C2Vector(0.0f, 0.0f);

    data->m_borderVerts.SetCount(2 * count);
    data->m_borderTexCoords.SetCount(2 * count);
    data->m_borderIndices.SetCount(2 * count + 2);

    TSGrowableArray<C3Vector> starts;
    TSGrowableArray<C3Vector> ends;
    starts.SetCount(count);
    ends.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        const C3Vector& current = data->m_fillVerts.m_data[i];
        data->m_borderVerts.m_data[2 * i] = current;
        data->m_borderTexCoords.m_data[2 * i] = C2Vector(0.0f, 0.0f);
        data->m_borderTexCoords.m_data[2 * i + 1] = C2Vector(1.0f, 1.0f);
        data->m_borderIndices.m_data[2 * i] = static_cast<uint16_t>(2 * i);
        data->m_borderIndices.m_data[2 * i + 1] = static_cast<uint16_t>((2 * i + 1) % data->m_borderVerts.Count());

        uint32_t next = (i + 1) % count;
        const C3Vector& following = data->m_fillVerts.m_data[next];
        float nx = -(following.y - current.y);
        float ny = following.x - current.x;
        float nz = 0.0f;
        float length = ny * ny + nx * nx;

        if (length > 0.00000023841858f) {
            float inverse = 1.0f / sqrt(length);
            nx *= inverse;
            ny *= inverse;
            nz = inverse * 0.0f;
        }

        float border = this->m_borderScalar * 0.0099999998f;
        nx *= border;
        ny *= border;
        nz *= border;

        if ((center.y - current.y) * ny + (center.z - current.z) * nz + (center.x - current.x) * nx > 0.0f) {
            nx *= -1.0f;
            ny *= -1.0f;
            nz *= -1.0f;
        }

        starts.m_data[i] = C3Vector(current.x + nx, current.y + ny, current.z + nz);
        ends.m_data[i] = C3Vector(nx + following.x, ny + following.y, nz + following.z);
    }

    data->m_borderIndices.m_data[2 * count] = 0;
    data->m_borderIndices.m_data[2 * count + 1] = 1;

    for (uint32_t i = 0; i < count; i++) {
        C3Vector hit;
        hit.x = 0.0f;
        hit.y = 0.0f;
        uint32_t next = (i + 1) % count;

        if (IntersectLines(starts.m_data[i], ends.m_data[i], starts.m_data[next], ends.m_data[next], &hit)) {
            data->m_borderVerts.m_data[2 * i + 1] = C3Vector(hit.x, hit.y, 1.0f);
        } else {
            data->m_borderVerts.m_data[2 * i + 1] = starts.m_data[i];
        }
    }
}

// VERIFY: 150e7df6 manual
// OFFSET: 0x0058FB40
void CGQuestPOIFrame::RemoveQuest(int32_t questId) {
    if (!questId) {
        return;
    }

    uint32_t index = 0;

    while (!this->m_groups[index].m_info || this->m_groups[index].m_info->m_questId != questId) {
        if (++index >= 4) {
            return;
        }
    }

    QuestPOIRenderGroup* group = &this->m_groups[index];
    group->m_info = nullptr;

    for (uint32_t i = 0; i < group->m_renderData.Count(); i++) {
        QuestPOIRenderData* data = &group->m_renderData.m_data[i];
        data->m_fillVerts.Clear();
        data->m_fillTexCoords.Clear();
        data->m_fillIndices.Clear();
        data->m_borderVerts.Clear();
        data->m_borderTexCoords.Clear();
        data->m_borderIndices.Clear();
    }

    group->m_renderData.Clear();
}

// VERIFY: 5d482c46 manual
// OFFSET: 0x005900F0
bool CGQuestPOIFrame::BuildRenderData(uint32_t index) {
    if (index > 3) {
        return true;
    }

    QuestPOIRenderGroup* group = &this->m_groups[index];
    QuestPOIInfo* info = group->m_info;

    if (!info) {
        return true;
    }

    if (fabs(this->m_rect.maxX - this->m_rect.minX) < 0.001f && fabs(this->m_rect.maxY - this->m_rect.minY) < 0.001f) {
        return false;
    }

    for (uint32_t i = 0; i < info->m_blobs.Count(); i++) {
        QuestPOIRenderData* data = this->m_groups[index].m_renderData.New();

        if (data) {
            this->BuildBlobRenderData(data, &group->m_info->m_blobs.m_data[i]);
        }
    }

    if (this->m_merging) {
        this->MergeBlobs(index);
    }

    return true;
}

// VERIFY: 389c7a57 manual
// OFFSET: 0x00590560
void CGQuestPOIFrame::AddQuest(QuestPOIInfo* info) {
    if (!info) {
        return;
    }

    int32_t slot = -1;

    for (uint32_t i = 0; i < 4; i++) {
        if (this->m_groups[i].m_info) {
            if (this->m_groups[i].m_info->m_questId == info->m_questId) {
                return;
            }
        } else if (slot == -1) {
            slot = i;
        }
    }

    if (slot > -1) {
        this->m_groups[slot].m_info = info;
        this->m_groups[slot].m_dirty = !this->BuildRenderData(slot);
    }
}

// VERIFY: 1324e85e auto
// OFFSET: 0x0058EB20
static void GxRsSet(EGxRenderState which, CImVector value) {
    if (!g_theGxDevicePtr->m_context) {
        return;
    }

    CGxAppRenderState& state = g_theGxDevicePtr->m_appRenderStates[which];
    int32_t color = *reinterpret_cast<int32_t*>(&value);

    if (state.m_value.m_data.p != reinterpret_cast<void*>(static_cast<intptr_t>(color))) {
        g_theGxDevicePtr->IRsDirty(which);
        state.m_value.m_data.p = reinterpret_cast<void*>(static_cast<intptr_t>(color));
    }
}

// VERIFY: 89d46308 manual
// OFFSET: 0x005901C0
void CGQuestPOIFrame::Render() {
    if (!g_questPOICVar->m_intValue || !this->m_fillTexture || !this->m_borderTexture) {
        return;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());

    if (CGWorldMap::GetCurrentAreaID() == -1 || !area) {
        return;
    }

    CGxTex* fillTexture = TextureGetGxTex(this->m_fillTexture, 0, nullptr);

    if (!fillTexture) {
        return;
    }

    CGxTex* borderTexture = TextureGetGxTex(this->m_borderTexture, 0, nullptr);

    if (!borderTexture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_Multisample, 0);
    GxRsSet(GxRs_Texture0, fillTexture);
    GxRsSet(GxRs_MatDiffuse, CImVector(0xFF, 0xFF, 0xFF, this->m_fillAlpha));

    for (uint32_t i = 0; i < 4; i++) {
        QuestPOIRenderGroup* group = &this->m_groups[i];

        if (!group->m_info) {
            continue;
        }

        if (group->m_dirty) {
            if (!this->BuildRenderData(i)) {
                continue;
            }

            group->m_dirty = 0;
        }

        for (uint32_t j = 0; j < group->m_renderData.Count(); j++) {
            QuestPOIRenderData* data = &group->m_renderData.m_data[j];

            if (!data->m_blob || !data->m_valid || !CGQuestLog::IsPOIOnCurrentMap(&data->m_blob->m_centerX, data->m_blob->m_mapId, area->m_mapID)) {
                continue;
            }

            int32_t mask = group->m_info->m_objectiveMask;

            if ((mask == -1 || ((1 << data->m_blob->m_objectiveIndex) & mask)) && (s_highlightIndex == -1 || s_highlightIndex == static_cast<int32_t>(j))) {
                GxPrimLockVertexPtrs(data->m_fillVerts.Count(), data->m_fillVerts.m_data, sizeof(C3Vector), nullptr, 0, nullptr, 0, nullptr, 0, data->m_fillTexCoords.m_data, sizeof(C2Vector), nullptr, 0);
                GxDrawLockedElements(GxPrim_Triangles, data->m_fillIndices.Count(), data->m_fillIndices.m_data);
                GxPrimUnlockVertexPtrs();
            }
        }
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, borderTexture);
    GxRsSet(GxRs_MatDiffuse, CImVector(0xFF, 0xFF, 0xFF, this->m_borderAlpha));

    for (uint32_t i = 0; i < 4; i++) {
        QuestPOIRenderGroup* group = &this->m_groups[i];

        if (!group->m_info) {
            continue;
        }

        for (uint32_t j = 0; j < group->m_renderData.Count(); j++) {
            QuestPOIRenderData* data = &group->m_renderData.m_data[j];

            if (!data->m_blob || !data->m_valid || !CGQuestLog::IsPOIOnCurrentMap(&data->m_blob->m_centerX, data->m_blob->m_mapId, area->m_mapID)) {
                continue;
            }

            int32_t mask = group->m_info->m_objectiveMask;

            if ((mask == -1 || ((1 << data->m_blob->m_objectiveIndex) & mask)) && (s_highlightIndex == -1 || s_highlightIndex == static_cast<int32_t>(j))) {
                GxPrimLockVertexPtrs(data->m_borderVerts.Count(), data->m_borderVerts.m_data, sizeof(C3Vector), nullptr, 0, nullptr, 0, nullptr, 0, data->m_borderTexCoords.m_data, sizeof(C2Vector), nullptr, 0);
                GxDrawLockedElements(GxPrim_TriangleStrip, data->m_borderIndices.Count(), data->m_borderIndices.m_data);
                GxPrimUnlockVertexPtrs();
            }
        }
    }

    g_theGxDevicePtr->RsPop();
}
