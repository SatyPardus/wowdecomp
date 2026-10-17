#ifndef ENGINE_SOURCE_PLATFORM_CALLINGCONVENTION_H
#define ENGINE_SOURCE_PLATFORM_CALLINGCONVENTION_H

// The original x86 Windows ABI needs stdcall. Native Apple targets use their
// platform ABI; pretending to be Win32 also selects the wrong system APIs.
#if defined(_WIN32)
#define WOW_STDCALL __stdcall
#else
#define WOW_STDCALL
#endif

#endif
