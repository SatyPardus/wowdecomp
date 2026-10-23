#include "Engine/Source/SteelseriesMouse/WowMouse.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/SteelseriesMouse/HidManager.h"

// VERIFY: f6868da0 manual
// OFFSET: 0x00635DD0
SteelseriesMouse::~SteelseriesMouse() {
}

// VERIFY: c632886d manual
// OFFSET: 0x008C2F50
WowMouse::WowMouse() {
    void* window = OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX);

    if (window) {
        this->m_hidManager = HidManager::Create();
        this->m_hidManager->SetWindow(window);
        this->m_hidManager->EnumerateDevices();
    }
}

// VERIFY: 39a94fcf manual
// OFFSET: [0x008C2ED0, 0x008C2FC0]
WowMouse::~WowMouse() {
    if (this->m_hidManager) {
        delete this->m_hidManager;
        this->m_hidManager = nullptr;
    }
}

// VERIFY: c463f2f6 auto
// OFFSET: 0x008C2F30
int32_t WowMouse::IsConnected() {
    return this->m_hidManager->GetDeviceCount() != 0;
}
