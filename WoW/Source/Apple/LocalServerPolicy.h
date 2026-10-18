#pragma once
// Opt in once before starting network workers. This is a local development
// server policy, not native executable verification or a fallback on failure.
void EnableNativeLocalServerPolicy();
bool NativeLocalServerPolicy();
bool NativeServerEndpointAllowed(bool authentication, const char* address);
