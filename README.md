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

### Apple foundation and Metal texture builds

The native port currently builds **foundation and texture probes**, not a playable client. They
link the existing Tempest mathematics, CDataStore, modified Lua 5.1 (including its
pool allocator and profiler), MPQ cryptography, and IOUnit chains to native Apple
memory, synchronization, pthread threads/TLS, timing and the original Blizzard file
API backed by native positional file IO. No Windows emulation is
used. Original SFile archive/resource IO and decompression are linked. The native
loader shares the client's archive definitions and applies patch priorities,
locale fallback and regional alternate views. Signature policy, game startup, the full Gx/Metal backend, audio and
network integration remain to be connected. Apple currently selects a full local
filesystem (`WOW_LOCAL_FILES_ONLY`); the remote streaming filesystem is not enabled.

Use CMake 3.21+ and Xcode. The presets target arm64, macOS 11+ and iOS 15+; the
installed Xcode 27 SDK requires iOS 15 as its minimum deployment target.

```sh
cmake --preset macos-arm64
cmake --build --preset macos-arm64 --parallel
build/macos-arm64/bin/AppleCoreProbe --data-root /path/to/client335a/Data
# Optionally start locale fallback at a particular language.
build/macos-arm64/bin/AppleCoreProbe --data-root /path/to/client335a/Data --locale zhCN

# Draw and read back every authored mip; add --preview for a native window.
build/macos-arm64/bin/AppleMetalProbe --data-root /path/to/client335a/Data \
  --texture 'Interface\GLUES\COMMON\GLUES-WOW-BCLOGO.BLP' --preview

# Compile the device app without a development identity (not installable yet).
cmake --preset ios-arm64 -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build --preset ios-arm64 --parallel

# Build for an Apple Silicon iOS simulator.
cmake --preset ios-simulator-arm64 -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build --preset ios-simulator-arm64 --parallel
```

The macOS probe checks native core behavior and reads MPQ v0/v1 headers and hash
tables using the original MPQ decryption. It opens archives through SFile, reads
their listfiles and representative model, terrain, WMO, texture, DBC, script and
audio resources, checking embedded MD5 digests when present. It then mounts the
full local archive layout and resolves known resources through the global SFile
search, reporting each supplying archive. The configured root replaces the
client's installation-directory search; archive filenames resolve without regard
to case. Data is always opened read-only; the large-offset IO
check uses a temporary sparse file outside Data and removes it afterwards. A
failed check, missing Data, or an unsupported archive returns a nonzero exit code.
Native log output defaults to a `wowdecomp` directory under the app's temporary
directory; `WOW_LOG_ROOT` can select a writable log root separately from Data.

The native texture path reads BLP2 palettes (0/1/4/8-bit alpha), BGRA8 and
BC1/BC2/BC3, checks mip extents and memory budgets, and preserves authored mips.
It uploads original BC blocks when the device supports them; otherwise it expands
to BGRA8 without resizing, generating mips or recompressing to another lossy format.
Private GPU textures use one shared staging buffer per upload and ordered commands.
The native Metal primitive accepts Gx blend/filter/wrap enums, supports all 12
blend equations (including alpha), independent alpha testing, the Gx channel-write
mask, blend constants, six filter modes and independent U/V wrap/clamp. Pipeline
and sampler objects are cached; alpha testing uses an MSL function specialization.
It is not yet connected to the full CGxDevice interface.
The original shared Gx sources also compile as a native static library; Apple
device factories, platform hooks and full client linkage remain pending.

The Metal probe verifies known alpha cases, filtering and blending, then draws and
reads back each mip through both native and forced-expanded upload paths. Expanded
samples must match exactly. The tested M2 Pro's native BC interpolation differs by
up to 2/255 from the D3D9 reference integer equations; the probe reports those
differences. This is not a Windows GPU visual-equivalence result. Legacy JPEG BLP
is explicitly unsupported (also disabled in the existing CBLPFile loader).
Shaders are currently compiled once per device context from embedded native MSL;
offline metallib packaging and pipeline caching across game effects are pending.

The iOS app is `build/ios-arm64/bin/RelWithDebInfo/WowAppleBootstrap.app`. For device
deployment, reconfigure with
`-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=YES` and
`-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID`, open
`build/ios-arm64/wowdecomp.xcodeproj`, select the `WowAppleBootstrap` scheme and your
device, and run with Xcode. Device signing/deployment requires your own Apple
development identity. Copy Data into the app's `Documents/Data` using Files or
Finder. The probe reruns when its scene enters the foreground. For simulator
development, `WOW_DATA_ROOT` in the scheme environment can point at an existing
host Data directory; real devices need a path accessible inside their sandbox.
`WOW_LOCALE` selects the starting locale for the iOS app. With no selection the
loader starts at enUS and follows the original cyclic locale fallback order.
The iOS app shows the same Metal texture and runs the GPU checks only while its
scene is active. `WOW_TEXTURE` selects a different archive resource. Inactive
scenes stop submitting draws and drain submitted work; this preview draws on
demand. Full reports remain in the view and bounded status messages go to unified
logging, avoiding blocked simulator console pipes.
Missing files and failed checks appear in the app and console. iOS does not allow
spawning a shell, so Lua `os.execute` explicitly reports an unsupported-operation
error there; it is not registered by the game's FrameScript initialization.

### Windows options

| CMake option | Default | Effect |
|---|---|---|
| `WOWDECOMP_DISABLE_SIGNATURE_CHECKS` | `OFF` | Skip the MPQ file and `.toc.sig` signature checks, so modified game data and interface files load |
| `WOWCLIENT_CRASH_LOG` | `ON` | Build `WowClient` with the crash logger |
| `WOWCLIENT_OUTPUT_OVERRIDE` | empty | Name the `WowClient` executable something other than `WowClient64` / `WowClient32` |

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
