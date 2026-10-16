# wowdecomp

A C++ decompilation of the World of Warcraft 3.3.5a client (`Wow.exe`, build 12340, June 2010).

Join the discord for other clients and to show off some cool stuff you build.

https://discord.gg/DsZMGUD8aT

Every function is reconstructed from the original binary and keeps the address it came from in an
`// OFFSET:` tag, so each one can be compared against the original. The source tree follows the layout
of the original client (`Engine/Source/<Module>`, `WoW/Source/<Module>`, original file names).

x64 is the main target. Win32 still builds and is used to check struct layouts against the 32-bit
original.

## Progress

The percentages below measure how much of the original binary has a counterpart in this source tree. They do
**not** say how much of the game works, and the missing part is not missing gameplay: the client builds into a
complete executable and the game is 100% playable. Bugs could still be hiding, though; a ported function can behave differently
from the original.

Measured against the 12340 binary on 2026-10-15. A function counts as ported when its address is
tagged in the source.

| | Ported | Total | Share of the original |
|---|---:|---:|---:|
| Functions | 24,515 | 27,157 | 90.3% |
| Code bytes | 5,270,186 | 5,656,573 | 93.2% |

The total leaves out 658 functions that IDA identifies as compiler library code and 132 import thunks.
Most of the untagged remainder is code the build gets elsewhere: compiler-generated code (static
initializers, template instances) that our compiler emits on its own, C runtime helpers, and third-party
libraries that are linked from their upstream sources instead of being decompiled (FreeType, zlib, PCRE,
parts of FMOD Ex).

## Requirements

- Windows 10 or 11
- Visual Studio 2022 with the **Desktop development with C++** workload (MSVC v143 and a Windows SDK)
- CMake 3.10 or newer (the one that ships with Visual Studio works)
- For Win32 builds only: [NASM](https://www.nasm.us/), on `PATH` or passed as `-DNASM_EXECUTABLE=<path>`, and
  `DivxDecoder.dll` from your 3.3.5a client copied to `ThirdParty/DivxDecoder/` (the build copies it next to the
  executables; the x64 build uses its own decoder instead)
- A World of Warcraft **3.3.5a (12340)** client. No game data is included in this repository.

## Quick start (Windows)

Double-click **`play.cmd`** in the repository root and select a number:

| Key | Action |
|---|---|
| `1` | Set up the environment, build, then launch |
| `2` | Check/install build tools |
| `3` | Configure and build only |
| `4` | Launch an existing build |
| `0` | Exit |

The launcher uses Windows PowerShell 5.1 (included in Windows 10/11), targets **x64 Release**,
and works regardless of the shell's current directory. Missing tools can be installed through
`winget` after confirmation: Visual Studio 2022 C++ Build Tools, a Windows SDK, and bundled
CMake. An existing VS 2022 installation is updated through Visual Studio Installer if needed.
Installation can require several GB of downloads, administrator approval, or a restart. If
`winget` is unavailable, install the requirements above manually and retry.

On first launch, if `bin/Win/Data` is missing, enter the path to your own **3.3.5a (12340)**
client or its `Data` folder. The script creates a directory junction rather than copying the
game data. Keep the source folder available; the client uses that data directly. Existing
`bin/Win/Data` directories are left untouched. No game files are downloaded, and
`Data/<locale>/realmlist.wtf` is not modified: configure it for your intended server yourself.

Individual actions can also be invoked from PowerShell (failures return a nonzero exit code):

```powershell
.\play.cmd -Action Setup
.\play.cmd -Action Build
.\play.cmd -Action Run -ClientPath "D:\Games\World of Warcraft"
.\play.cmd -Action All -ClientPath "D:\Games\World of Warcraft"
```

The launcher keeps signature checks enabled and the crash logger on, and uses the default
`WowClient64.exe` output name. For other CMake options or Win32, use the manual commands below.
This launcher is Windows-only; for the native macOS client see [Building on macOS](#building-on-macos).

Launcher regression checks (no game data, installation, or compilation required):

```powershell
powershell -NoProfile -File .\Tools\tests\play.Tests.ps1
```

## Building

From a Developer PowerShell or any shell with `cmake` on `PATH`:

```powershell
git clone https://github.com/SatyPardus/wowdecomp.git
cd wowdecomp

# x64 (main target)
cmake -S . -B build/x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/x64 --config Release --target WowClient --parallel

# Win32 (optional, needs NASM)
cmake -S . -B build/win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build/win32 --config Release --target WowClient --parallel
```

In-source builds are refused, so keep the build directory separate (`build/` above).

The client is written to `bin/Win/`:

| Target | x64 | Win32 |
|---|---|---|
| `WowClient` | `WowClient64.exe` | `WowClient32.exe` |
| `Wow` | `Wow64.exe` | `Wow32.exe` |

`WowClient` is the client with a crash logger: it opens a console window and writes
`WowClient_crash.log` when the client crashes. `Wow` is the same client without it. You can also turn the
logger off in `WowClient` with `-DWOWCLIENT_CRASH_LOG=OFF`.

### Options

| CMake option | Default | Effect |
|---|---|---|
| `WOWDECOMP_DISABLE_SIGNATURE_CHECKS` | `OFF` | Skip the MPQ file and `.toc.sig` signature checks, so modified game data and interface files load |
| `WOWCLIENT_CRASH_LOG` | `ON` | Build `WowClient` with the crash logger |
| `WOWCLIENT_OUTPUT_OVERRIDE` | empty | Name the `WowClient` executable something other than `WowClient64` / `WowClient32` |

### Building on macOS

The native macOS client (Apple Silicon or Intel) replaces the Win32 platform layer with SDL2 (window, input,
audio device), the system OpenGL framework (legacy 2.1 context, ARB program path) and libcurl. Platform code
lives in `Mac/` directories next to the `Win32/`/`Win64/` ones.

```sh
brew install cmake sdl2
cmake -S . -B build/mac -DCMAKE_BUILD_TYPE=Release
cmake --build build/mac --target Wow --parallel
```

The client is written to `bin/Mac/Wow`. Copy your 3.3.5a `Data` folder to `bin/Mac/Data/` and start
`bin/Mac/Wow` (data is located relative to the executable). Not available on macOS: Direct3D (OpenGL only), Logitech LCD,
SteelSeries HID, DivX cinematics (skipped), and the patch-download launcher.

## Running

1. Copy the `Data` folder of your 3.3.5a client into `bin/Win/`, so you have `bin/Win/Data/`.
2. Point `Data/<locale>/realmlist.wtf` at the server you want to connect to.
3. Start `bin/Win/WowClient64.exe` (or `Wow64.exe`).

## Layout

| Path | Contents |
|---|---|
| `Engine/Source/` | Engine modules: Storm, Gx (Direct3D 9 / OpenGL), Model2, Frame/FrameXML UI, SoundEngine, Lua, ... |
| `WoW/Source/` | Game modules: objects, world rendering, UI, networking, database, glue screens |
| `WoW/Common/` | Code shared by the game modules |
| `ThirdParty/` | Libraries linked from their upstream sources |
| `Tools/WowClient/` | The `WowClient` executable target |
| `Tools/XvidDecoder/` | x64 movie decoder that replaces the original 32-bit `DivxDecoder.dll` |

## Special thanks

- [binana](https://github.com/thunderbrewhq/binana)
- [thunderbrew](https://github.com/thunderbrewhq/thunderbrew)
- [whoa](https://github.com/whoahq/whoa)
- TOMRUS, for the initial IDB
- Everyone who has reported bugs so far
- The WoW modding community
