#if defined(__APPLE__)

#include <cstdint>
#include <Carbon/Carbon.h>
#include "Engine/Source/Os/Input.h"

// Called in balanced pairs while a password edit box has focus. Secure Event Input is the macOS password-field
// mode: other processes cannot observe keystrokes and the system switches to a Roman input source, so the
// password bypasses the input method just as it bypasses the disabled IME on Windows.
void OsInputEnableSecureMode(int32_t enable) {
    if (enable) {
        EnableSecureEventInput();
    } else {
        DisableSecureEventInput();
    }
}

#endif
