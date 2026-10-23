#ifndef ENGINE_SOURCE_OS_JOYSTICK_H
#define ENGINE_SOURCE_OS_JOYSTICK_H

#include <cstdint>

// Keep the current unsigned joystick index and invalid-ID representation.
typedef uint32_t OsJoystickID;

void OsJoystickClose(OsJoystickID index);
void OsJoystickConfigureMouseMode(OsJoystickID index, int32_t stick, int32_t leftButton, int32_t rightButton, float speed);
uint32_t OsJoystickCount();
char* OsJoystickGetName(OsJoystickID index, char* buffer, uint32_t size);
void OsJoystickMapHat(OsJoystickID index, int32_t hat, int32_t up, int32_t right, int32_t down, int32_t left);
void OsJoystickMapSlider(OsJoystickID index, int32_t axis);
void OsJoystickMapStick(OsJoystickID index, int32_t stick, int32_t xAxis, int32_t yAxis);
OsJoystickID OsJoystickOpen(OsJoystickID index);
void OsJoystickSetMouseMode(OsJoystickID index, int32_t enable);
void OsPollJoysticks(int32_t active);

#endif
