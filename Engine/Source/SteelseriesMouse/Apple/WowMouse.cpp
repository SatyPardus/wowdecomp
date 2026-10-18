#if defined(__APPLE__)
#include "Engine/Source/SteelseriesMouse/WowMouse.h"

// No native vendor HID backend is installed. Keep the original no-device
// branch available without treating an ordinary system mouse as this device.
SteelseriesMouse::~SteelseriesMouse() = default;
WowMouse::WowMouse() : m_hidManager(nullptr) {}
WowMouse::~WowMouse() = default;
int32_t WowMouse::IsConnected() { return 0; }
#endif
