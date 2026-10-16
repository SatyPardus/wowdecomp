# macOS Native Porting Notes

This document records the process of porting wowdecomp (the decompiled WoW 3.3.5a client) to a native macOS
executable, the design decisions made, and the issues found during startup debugging and their root causes.

Current status: The native Apple Silicon (arm64) build, `bin/Mac/Wow`, loads WoW 3.3.5 zhCN game data, logs in to
a server, creates characters (with localized race and class names), and enters and stays in the world.

## 1. Architecture and Dependency Replacements

| Windows | macOS replacement |
|---|---|
| Win32 window / WGL | SDL2 window + legacy OpenGL 2.1 context (system OpenGL.framework) |
| DirectInput / WM_* messages | SDL2 events translated to `OSINPUT` events (`Engine/Source/Os/Mac/Input.cpp`) |
| Direct3D 9 / 9Ex | Not available; macOS uses only the OpenGL device (ARB vertex/fragment program path) |
| WinINet | libcurl |
| Winsock / IOCP | BSD sockets |
| Registry | `~/Library/Preferences/WowDecomp/registry.ini` |
| FMOD Ex Win32 output | FMOD output plugin backed by SDL2 audio (`fmodex/mac/src`) |
| CRITICAL_SECTION / Win32 threads | pthread (see section 4 for semantic compatibility) |

Conventions:

- Platform code lives in `Mac/` directories alongside `Win32/` and `Win64/` (FMOD uses `fmodex/mac/src/`). Each
  source file is wrapped in `#if defined(__APPLE__)`. Module CMake files exclude `Win32/Win64` on macOS and
  exclude `Mac/` on Windows.
- Port behavior function by function, using the Win64 implementation as the reference. Only features that
  genuinely do not exist on macOS (Logitech LCD, SteelSeries HID, Windows anti-cheat memory scanning, the patch
  downloader, and DivX decoding) use a "device/feature unavailable" path.
- Isolate platform differences in shared code with `#if` so Windows behavior remains unchanged.
- On non-Windows platforms, `Engine/Source/Platform/Compat/` is a global include directory for compatibility
  shims: `malloc.h`, `SSE.h` (x86 SSE → ARM NEON), `RecursiveMutex.h`, and `ThreadStack.h`.

SDL2 platform conventions:

- `main()` (`WoW/Source/Mac/main.cpp`) calls `SDL_Init` before `CommonMain()` and `SDL_Quit` afterward.
- The native window handle (`HWND` on Windows) is an `SDL_Window*` on macOS.
- The Gx OpenGL device creates the window and GL context, then calls `OsGuiSetGxWindow`. The device tracks window
  size changes itself through `SDL_AddEventWatch`.
- The Os layer pumps events on the main thread with `SDL_PollEvent` and translates them into `OSINPUT` events
  equivalent to those produced by Win64 `OsWindowProc`.

## 2. Building and Running

Use the one-click `./play.sh` script from the repository root:

| Option | Action |
|---|---|
| `1` Configure | Check for Xcode Command Line Tools and Homebrew; if CMake or SDL2 is missing, offer to install it with Homebrew; generate `build/mac` |
| `2` Build | Build the `Wow` target to `bin/Mac/Wow` (runs step 1 first if not configured) |
| `3` Launch | On first run, ask for the client directory and symlink its `Data` folder to `bin/Mac/Data`; launch the client in the background and write output to `bin/Mac/Wow.log` (runs step 2 first if not built) |

You can also run `./play.sh 2` or `./play.sh 3 --client <client-directory>` directly. The script is compatible
with the version of bash 3.2 included with macOS. Manual steps:

```sh
brew install cmake sdl2
cmake -S . -B build/mac -DCMAKE_BUILD_TYPE=Release
cmake --build build/mac --target Wow --parallel
```

The executable is `bin/Mac/Wow`. Run it from a WoW 3.3.5a client directory (next to `Data/`); the data
directory is located relative to the executable.

## 3. Link Stage

The first attempt to link `Wow` had 352 undefined symbols, all from platform implementations excluded on
non-Windows systems. Implementations were added in parallel by module: Storm, BlizzardCore, Os, FMOD, Gx
(OpenGL/SDL2), and WoW client platform code. The following link issues were then fixed:

- `CGxDeviceOpenGl::DeviceSetBaseMipLevel` and `IShaderConstantsClear` had accidentally been placed inside a
  Windows-only `#if` block; they were moved out.
- The `s_mapId` in `MinimapFrame.cpp` and the WorldClient `s_mapId` are distinct variables in the original
  binary (0x00AF4E48 vs. 0x00ADFBC4). MSVC accepts redeclaring a name as `static` after an `extern` declaration,
  but Clang does not. The minimap's file-local variable was renamed to `s_minimapMapId` without changing its
  semantics.
- The empty `SetZone` in Mac `Streaming.cpp` shadowed the real implementation in `StreamingManager.cpp`; the
  stub was removed.
- Added the macOS implementation of `StringConversion_CodepageToUTF8::ToNative` (macOS file paths are natively
  UTF-8).

## 4. Issues Found When Starting with Real Game Data

| Symptom | Root cause | Fix |
|---|---|---|
| "Failed to find a suitable display device" | The Mac `DeviceAdapterInfer` always returned failure, so no row could be selected from `VideoHardware.dbc`. | Use a temporary OpenGL context to classify the adapter using the equivalent rules from the Windows D3D caps (T&L, texture-unit count, and pixel shader version). Apple Silicon has no PCI ID here, so it falls back to the generic 0xFFFF row. |
| SIGABRT on exit (exception thrown by `recursive_mutex::lock`) | On Mac, the lock type was `std::recursive_mutex`; `exit()` destroyed static objects while threads were still using them. Windows `CRITICAL_SECTION` has no destructor. | Added `WowRecursiveMutex`, a recursively lockable pthread mutex with constant initialization and a trivial destructor. On non-Windows platforms, both `CRITICAL_SECTION` and `Blizzard::Lock::Mutex` use it. Other static `std::mutex` instances are no longer destroyed at exit. |
| `Fonts\FRIZQT__.TTF` read failure (`LoadSectorTable failed`) | The block table is expanded in place from 16-byte entries to 49-byte `BLOCKENTRY` objects. The entry 0 `int64 m_offset` write overlaps the `uint32 compressedSize` read. Clang reordered accesses under strict aliasing and read 0; MSVC does not use type-based alias analysis. | Add `-fno-strict-aliasing -fwrapv` globally for non-MSVC compilers in the root `CMakeLists.txt`, matching the MSVC semantics the decompiled code relies on. |
| FMOD `FMOD_NONBLOCKING` thread stack overflow (`EXC_BAD_ACCESS`) | The stack size passed to Windows `CreateThread` is only the initial commit size; the reserved stack is at least 1 MB. A pthread stack size is a hard limit (the macOS default is 512 KB). | `WowThreadStackSize` in `ThreadStack.h` enforces a minimum of 1 MB. FMOD, BlizzardCore, and Storm use it at all three thread-creation sites. |
| Text spacing is too wide | On Retina fullscreen, the display switches to 3024×1964 while the device format remains 1512×982, so font rasterization and layout use inconsistent dimensions. | After entering fullscreen, `CreateGameWindow` adopts the actual backbuffer size, consistent with the existing maximized-window path. |
| Every Chinese glyph has the same full-width advance | The font has `post.isFixedPitch = 1` but is proportional. The bundled FreeType 2.0 forces `advanceWidthMax` for fonts marked this way. | `ThirdParty/freetype/src/truetype/ttgload.c` now uses the per-glyph `hmtx` advance, matching the original client's rendering. |
| The login 3D scene appears only in a small rectangle in the center of the screen | Decompiled-code error: `IXformSetProjection` tested `int32 >= 0` (bit 31), while the original tests the high bit of the low byte (bit 7, `GxMasterEnable_NormalProjection`). This caused the fixed-function perspective projection to always be scaled by a 0.2 debug factor. | Test the same bit as the two D3D devices. |
| Warning: "This system will no longer be supported in future versions (SSE)" | The Mac CPU feature flags did not match the Windows bit layout, so SSE (bit `0x4`) was not reported. | Use the Windows bit definitions: arm64 reports TSC/SSE/SSE2 (the relevant code is pure C++ or uses the NEON compatibility layer); x86_64 reports TSC/MMX/SSE/SSE2. |
| Cannot type in the password field | When focus left the account field, `DisableIME()` reduced the IME reference count to zero. Mac `OsIMEEnable(0)` / `OsIMESync()` then called `SDL_StopTextInput()`, disabling all text events. On Windows, disabling the IME only detaches the input-method context; `WM_CHAR` continues to arrive. The password field (flag `0x8`) does not enable the IME again. | IME toggles now manage only composition state; SDL text input remains enabled for the entire session. `OsInputEnableSecureMode` (called in balanced pairs on password-field focus gain/loss) now uses macOS Secure Event Input (`EnableSecureEventInput`), bypassing the input method and preventing other processes from observing keystrokes, corresponding to disabling the IME on Windows. |
| Crash after clicking Login while "Connecting" (`EXC_ARM_DA_ALIGN` on the Network thread) | `WowConnectionNet::WORKER` used `#pragma pack(push, 1)` to reproduce the 32-bit `Wow.exe` layout, placing `SCritSect m_lock` at odd offset 0x15. x86 tolerates an unaligned `CRITICAL_SECTION`; on arm64, a pthread mutex must be 8-byte aligned, so locking it caused a bus error. | Preserve byte packing only for 32-bit builds (`WOWDECOMP_ARCH_32`) used to compare layouts with the original binary. The 64-bit layout already differs from the comments' offsets, so use natural alignment there. A scan of all `pack(1)` regions found no other embedded locks. |
| Entering the world crashed with a different null dereference each time (`CMovement_C::UpdateStatusInternal`, `MovementUpdateCameraYaw`, `CMap::ObjectCreate`, `CGCamera::SetTelescope`, `Fatal Condition` in `CGGameUI::UpdateActivePlayer`) | The Mac `OsInputGet` called `s_eventPollProc` (`EventForceIdleProcessing`) on every poll with an empty queue. Win64 calls it only from a timer during modal non-client loops (title-bar menu/drag). While loading, `FrameXML_CreateFrames` → `UpdateProgressBar` pumps input, so the Mac build re-entered idle processing, including `PollNet`, which handled `SMSG_UPDATE_OBJECT` before movement globals, the camera, map light, and world frame existed. | Do not call the poll proc from `OsInputGet` (`Engine/Source/Os/Mac/Input.cpp`), matching Win64. Do not add null checks for the individual symptoms; they all disappear once packets are no longer processed during loading. |
| Crash after a few minutes in the world in `CM2Model::ProcessEventCallbacks` (invalid address in M2 event timestamp data) | `M2Array` stores a self-relative signed 32-bit offset. On 64-bit builds, external `.anim` sequence data and some vertex buffers are separate allocations that must be within ±2 GB of the M2 data (`OsAllocNear`). The Mac `OsAllocNear` ignored its anchor and used `mmap(nullptr, …)`. On arm64, large and small allocations can be more than 2 GB apart, so offsets were truncated. | Port the Win64 search to Mach VM (`Engine/Source/Os/Mac/OsMemory.cpp`): walk gaps between mapped regions within 0x60000000 of the anchor using `mach_vm_region`, then claim one with `mach_vm_allocate(VM_FLAGS_FIXED)`, which never replaces existing mappings, unlike `MAP_FIXED`. |
| On character creation, race and class tooltips and the race/class panel titles are empty (gender tooltips work) | Race and class names are localized DBC strings selected by `s_locale`, which comes from `Wow.ini` inside `locale-zhCN.MPQ` (`Language=zh`, `Country=CN`) through `SStrPrintf(s_iniLocale, 5, "%s%s", ...)`. Storm's ordered printf (`vsnoprintf`) mapped MSVC `_snprintf` to C `snprintf`. MSVC can fill all `n` characters, while C reserves one for the terminator, so on Mac each specifier that exactly filled the remaining space lost its last character. The result was `"zhC"`, which made `s_locale` fall back to 0 (enUS, an empty column in zhCN data). | `Engine/Source/Storm/Source/soprintf.cpp` passes `avail + 1` to `snprintf` on non-Windows platforms. The extra byte is `end`, which is already reserved for the final terminator. This fixes every truncated `SStrPrintf`/`SStrVPrintf` exact-fit case, not just locale handling. |

### Issues Not Specific to macOS

The following three issues also affect Windows builds:

- Fonts: a Windows build using zhCN/zhTW data will have the same glyph-spacing issue.
- The 0.2× scene scale also occurs on Windows when using the OpenGL renderer.
- The `-fno-strict-aliasing -fwrapv` flags apply to all non-MSVC compilers.

## 5. Debugging Methods

- The original zhCN `Wow.exe` can be used for verification (its layout differs from enUS 12340, so offsets in
  the decompilation comments do not directly apply). Search by instruction pattern or constant, then disassemble
  with `objdump -d`. For example, the `FT_Load_Glyph` flags are `0x208A` (monochrome adds `0x38`, yielding
  `0x20C2`); this confirmed that the decompiled code matched the original and helped isolate the issue to the
  bundled FreeType.
- Independently parse MPQ files with Python (decrypt hashes/block tables and decompress sectors) to compare
  ground-truth data with what the engine reads.
- Compare an x86_64 build running under Rosetta with the arm64 build to distinguish ARM-specific numeric issues
  from shared-code issues.
- If the process hangs, use `sample <pid>` to inspect the main-thread stack. For crashes, capture a backtrace
  with `lldb -b -o run -o bt` or inspect `~/Library/Logs/DiagnosticReports/Wow-*.ips`.
- lldb runs the client with ASLR enabled, so `b -a <static address>` breakpoints silently fail to resolve. Use
  symbol breakpoints, and resolve globals through `SBTarget.FindSymbols(...).GetLoadAddress()` in a Python script.
  Only one client instance can run at a time; a stale debugged instance makes a new launch exit immediately.
- SDL converts SIGTERM into a quit event. If the process is stuck in a modal dialog, SIGTERM will not terminate
  it; SIGKILL is required.

## 6. Known Limitations and Follow-up Work

- Verified with admin/admin: server login, character creation, entering the world, and several minutes in the
  world without crashing. Not yet verified: sound output, IME composition, windowed mode, and resolution
  switching.
- Upstream renames (`unk_*` → `unused_*`) update only the Win32/Win64 platform files. After merging upstream,
  rebuild on macOS and apply the same rename to `Mac/` sources (for example, `OsCallContext::unused_000C`).
- A temporary x86_64 test build (using a locally built SDL 2.30) showed a Yes/No dialog with no text. This was
  not seen in the official arm64 build using Homebrew SDL, but its dialog text has not been confirmed.
- Audio uses SDL2 output; OpenAL Soft is not integrated.
- Streaming implements only a minimal local-first path; a complete client installation does not depend on it.
- Unavailable on macOS: Direct3D, Logitech LCD, SteelSeries HID, DivX cinematics (skipped), and the patch
  downloader.
- Windows could not be built on this machine. Shared-file changes are isolated with `#if`; review found no
  changes to Windows behavior.
