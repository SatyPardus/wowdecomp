#ifndef ENGINE_SOURCE_PLATFORM_CALLINGCONVENTION_H
#define ENGINE_SOURCE_PLATFORM_CALLINGCONVENTION_H

// The original x86 Windows ABI needs explicit calling conventions. Apple uses its
// platform ABI; pretending to be Win32 also selects the wrong system APIs.
#if defined(_WIN32)
#define WOW_STDCALL __stdcall
#define WOW_FASTCALL __fastcall
#else
#define WOW_STDCALL
#define WOW_FASTCALL
#endif

#endif
