#pragma once
// Called on the game thread. Reject the session without unwinding through the
// original network queue's explicit locks/reference ownership.
void RejectNativeIntegrityRequest(const char* reason);
