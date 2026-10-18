#if defined(__APPLE__)
#include "Engine/Source/Os/Joystick.h"
// No native joystick backend is enabled. Preserve the original absent-device
// contract so optional controller configuration cannot block keyboard/mouse UI.
uint32_t OsJoystickCount() { return 0; }
uint32_t OsJoystickOpen(uint32_t) { return UINT32_MAX; }
char* OsJoystickGetName(uint32_t, char*, uint32_t) { return nullptr; }
void OsJoystickClose(uint32_t) {}
void OsJoystickConfigureMouseMode(uint32_t, int32_t, int32_t, int32_t, float) {}
void OsJoystickMapHat(uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t) {}
void OsJoystickMapSlider(uint32_t, int32_t) {}
void OsJoystickMapStick(uint32_t, int32_t, int32_t, int32_t) {}
void OsJoystickSetMouseMode(uint32_t, int32_t) {}
void OsPollJoysticks(int32_t) {}
#endif
