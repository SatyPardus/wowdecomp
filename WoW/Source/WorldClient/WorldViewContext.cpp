#include "WorldViewContext.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include <stdexcept>

namespace {
thread_local const CSimpleCamera* s_viewCamera = nullptr;
}
World::ViewCameraScope::ViewCameraScope(const CSimpleCamera& camera) : m_previous(s_viewCamera) { s_viewCamera = &camera; }
World::ViewCameraScope::~ViewCameraScope() { s_viewCamera = m_previous; }
const CSimpleCamera& World::ViewCamera() {
    if (s_viewCamera) return *s_viewCamera;
    if (const auto* camera = CGWorldFrame::GetActiveCamera()) return *camera;
    throw std::logic_error("World operation requires a bound view camera");
}
