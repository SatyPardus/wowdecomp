#include "WorldFrame.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x00B7436C
CGWorldFrame* CGWorldFrame::s_currentWorldFrame;

// VERIFY: d69961a6 auto
// OFFSET: 0x004F6370
void CGWorldFrame::AddModelToHitTest(CM2Model* model, uint32_t group, void* owner, uint32_t mode) {
    if (model->m_fFlags & 0x1) {
        if (!model->m_hitTestPrev) {
            CM2Model** head = &model->m_scene->m_hitTestList;
            model->m_hitTestPrev = head;
            model->m_hitTestNext = *head;
            *head = model;

            if (model->m_hitTestNext) {
                model->m_hitTestNext->m_hitTestPrev = &model->m_hitTestNext;
            }
        }

        model->m_hitTestMode = mode;
        model->m_hitTestGroup = group;
        model->m_hitTestOwner = owner;
    }

    for (CM2Model* attached = model->m_attachList; attached; attached = attached->m_attachNext) {
        if (!attached->m_hitTestPrev) {
            AddModelToHitTest(attached, group, reinterpret_cast<void*>(-1), mode);
        }
    }
}

// VERIFY: abfda7eb manual
// OFFSET: 0x004F9410
bool CGWorldFrame::HitTestModelList(const C3Vector& start, const C3Vector& end) {
    C44Matrix view;
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];

    C3Vector camera = this->m_camera->m_position;
    C3Vector viewStart = (start - camera) * view;
    C3Vector viewEnd = (end - camera) * view;

    World::s_m2Scene->BeginHitTest();

    for (CModelRecord* record = this->m_modelList.Head(); record; record = this->m_modelList.Next(record)) {
        AddModelToHitTest(record->m_model, 0, record, 0);
    }

    float t = 1.0f;

    return World::s_m2Scene->EndHitTest(viewStart, viewEnd, t, 0) != nullptr;
}
