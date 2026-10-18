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

### Apple foundation and Metal Gx builds

Apple configuration downloads the pinned Xvid 1.3.7 source release from Xvid's
official server and verifies its SHA-256. It builds the portable codec for the
selected SDK; Homebrew and the Windows decoder DLL are not used. The source
and build cache live under `build/`. For offline configuration, supply an
already extracted release with `-DFETCHCONTENT_SOURCE_DIR_APPLE_XVID=/path/to/xvidcore`.
Xvid's GPLv2-or-later license remains in the fetched source and
`ThirdParty/xvidcore/licenses/xvid.txt`. The movie reader keeps the original
AVI video packets, MP3 bytes and stream timing; it does not transcode Data.
The original MovieFrame now uses this decoder, its existing texture layouts,
MP3 sound clock, subtitle events and frame skipping. Set
`WOW_MOVIE='Interface\Cinematics\WOW_Intro_1024'` when launching the Mac
`AppleUIProbe /path/to/Data --preview`, the Metal preview, or the iOS app to play
one original movie. Paths omit `.avi`; the selected locale's loose directory
and MPQ subtitles follow original SFile precedence. The viewer shows subtitle
callbacks using the original font renderer; full Glue movie sequencing and
subtitle fades remain part of the pending complete Glue integration. Escape or
Space stops the Mac viewer's movie. iOS touch controls remain deferred.

The native port currently builds **foundation, graphics and audio probes**, not a playable client. They
link the existing Tempest mathematics, CDataStore, modified Lua 5.1 (including its
pool allocator and profiler), MPQ cryptography, and IOUnit chains to native Apple
memory, synchronization, pthread threads/TLS, timing and the original Blizzard file
API backed by native positional file IO. No Windows emulation is
used. Original SFile archive/resource IO, decompression and the game's SFile2
open/read/seek/preload layer are linked. The native
loader shares the client's archive definitions and applies patch priorities,
locale fallback and regional alternate views. Signature policy, game startup, the full Gx/Metal backend, audio and
login/session integration remain to be connected. Apple currently selects a full local
filesystem (`WOW_LOCAL_FILES_ONLY`); the remote streaming filesystem is not enabled.

Use CMake 3.21+ and Xcode. The presets target arm64, macOS 11+ and iOS 15+; the
installed Xcode 27 SDK requires iOS 15 as its minimum deployment target.

```sh
cmake --preset macos-arm64
cmake --build --preset macos-arm64 --parallel
build/macos-arm64/bin/AppleCoreProbe --data-root /path/to/client335a/Data
# Optionally start locale fallback at a particular language.
build/macos-arm64/bin/AppleCoreProbe --data-root /path/to/client335a/Data --locale zhCN

# Check original packet/stream networking using local loopback only.
build/macos-arm64/bin/AppleNetworkProbe
# Check original SRP and Grunt login against synthetic loopback credentials.
build/macos-arm64/bin/AppleLoginProbe
# Check original NetClient game-thread delivery and session teardown.
build/macos-arm64/bin/AppleSessionProbe
# Check original realm authentication, character flow and stream signatures.
build/macos-arm64/bin/AppleRealmProbe

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
search, reporting each supplying archive. SFile2 reads are compared against the
same resources, including preloaded reads, EOF, seeks and zero-padded whole-file
loads; a temporary loose file checks case-insensitive lookup through the original
file index. SFile2 compressed loose-file branches and asynchronous overlapped
reads remain incomplete in the reconstructed implementation. The configured root replaces the
client's installation-directory search; archive filenames resolve without regard
to case. Data is always opened read-only; the large-offset IO
check uses a temporary sparse file outside Data and removes it afterwards. A
failed check, missing Data, or an unsupported archive returns a nonzero exit code.
Native log output defaults to a `wowdecomp` directory under the app's temporary
directory; `WOW_LOG_ROOT` can select a writable log root separately from Data.
The game `OsFile` API now uses the same native file stack, including config-file
read/write/replacement, 64-bit sizes, directory and hidden/read-only attributes,
glob enumeration, copy/move and cleanup. Its directory attributes use the bits
expected by CVar/AddOns callers; unsupported attribute flags fail explicitly.
Windows cross-process sharing restrictions remain unsupported on Apple.
Startup path queries now return the actual native executable and its directory;
opaque dynamic-library handles are explicitly unsupported for image-path queries.
Computer/account names and OS versions come from the platform, with the original
buffer-length conventions. A missing iOS account database entry remains unavailable
(the original optional bug-report field is skipped), without a fabricated username.
The original float-seconds wrapper uses the native millisecond clock. Mac sanitizer
and iOS simulator checks cover these queries and their buffer boundaries.

`SCreateProcess` can launch native macOS helpers with the original command-line
quoting, an optional worker completion callback, and child reaping even without
a callback. Only standard descriptors are inherited. PATH lookup uses direct
native spawn; invalid executables are never retried as shell commands. There is
no Win32 GUI input-idle handshake. iOS reports `ENOTSUP`, including Simulator.
This does not provide native replacements for Launcher.exe or WowError.exe.

The separate Storm file interface used by patch downloads now uses native
positional IO, preserving creation/access modes, short/exact reads, logical
resize, refcount cleanup and offsets beyond 4 GiB. Uncached and write-through
requests use native flags; overlapped IO and Windows handles fail explicitly.
Windows share-denial remains unsupported. The original Grunt file-download
methods are shared unchanged with Windows in their own compilation unit.
Mac sanitizer and iOS simulator checks cover stop/resume from a saved header,
independent MD5 accept/reject, exact completed bytes, header removal/rename and
cancellation in temporary storage. This does not run a patch executable or make
the full updater available; the original download hash routine still has 32-bit
range arithmetic even though its native IO backend supports larger offsets.

The original `WowConnection` framing, directional header/opcode encryption,
send queues, stream callbacks and worker dispatch now run over native Apple
nonblocking IPv4 sockets and a waking `poll` loop. `AppleNetworkProbe` uses only
local loopback: it checks split 2/3-byte headers, queued sends, independent cipher
reference bytes, immediate reconnect, refusal, active shutdown and repeated
initialization. The iOS core check runs the same cases. Network shutdown joins
workers before closing retained sockets; queued message buffers can outlive it.
The original Grunt login and realm-list protocol also run on this transport.
`AppleLoginProbe` checks SRP against independently computed public/session keys
and proofs, then exchanges the original hello, authentication/version proofs and
realm-list messages with a synthetic loopback peer. Split and combined replies,
account rejection, invalid SRP keys/proofs and teardown are checked. The probe
supplies an explicit test version checksum; the application's real checksum,
signature policy and full Glue session integration remain to be connected.
No external login service is contacted by these checks. Apple uses its system
secure random source and retains the reference 12340 Windows/x86 protocol IDs;
these legacy wire IDs do not describe the native arm64 hardware.

The original `NetClient` now queues connection, authentication challenge, opcode
and disconnect events for the game thread. `AppleSessionProbe` exercises those
events through a real loopback socket, including the first unencrypted packet,
subsequent directional cipher headers, cancellation before dispatch, repeated
initialization and deferred deletion. Apple Glue binds its original disconnect
handler for its own lifetime; invoking the native disconnect path without a host
is an explicit error. The original event payload release was restored from the
reference executable. Native teardown releases the queue mutex before deleting
its owner, and canceled events release their references. Receive statistics and
the ping clock are synchronized between the worker and game thread.
Net, Glue and WowSvcs compile on Apple arm64; full Glue startup remains pending.
Windows DLL scanning, patch execution and the remaining platform services are
not made available merely by compiling their shared callers.

`AppleRealmProbe` runs the original RealmConnection through the original Event
POLL callback. Synthetic loopback exchanges check the 12340 session proof against
system SHA1, encrypted queue/authentication replies, the original empty AddOns
exchange and cache-version invalidation, two character records with equipment
and race restrictions, selected-character login, failure/logout replies, rejected
authentication and truncated character lists. This checks protocol behavior;
it does not create a playable player or connect to an external realm.

`AppleClient` is the macOS host for the original engine/client initialization,
GlueXML login interface, Event loop and shutdown. It mounts the original Data,
loads the authenticated file-signature manifest and preserves GlueXML signature
checks. The original Glue login has authenticated to local AzerothCore, selected the
realm and entered an existing character's world. The original player/equipment,
Stormwind scene, indoor minimap, chat, quest tracker and action bars now render.
Movement, turning, camera wheel zoom and jumping/landing have run through native
AppKit events and the character's original bindings. Complete gameplay/rendering
coverage and native executable challenge verification remain unfinished.

```sh
cmake --build --preset macos-arm64 --target AppleClient
build/macos-arm64/bin/AppleClient /absolute/path/to/client335a/Data /absolute/path/to/native-runtime
```

The `WowAppleClient` target builds `build/macos-arm64/bin/WowAppleClient.app`,
with a local development signature and high-resolution display declaration.
Opening the app without arguments restores its saved Data/runtime paths or asks
for the original Data directory. The picker has an explicit local AzerothCore
development option; hold Option while launching to reopen it. A fresh setup
keeps writable files in `~/Library/Application Support/WoWDecompClient`.
The app also accepts the command-line options below and remembers explicitly
supplied paths and local mode after successfully mounting Data. It does not save
the stdin password. macOS may request removable-volume access for external Data.

The explicit runtime directory must be outside the original installation. It
contains `WTF/Config.wtf`, cache, logs and other relative client output. Existing
configuration is loaded and saved through the original CVar system; credentials
are not supplied by this command. `WOW_CLIENT_FRAMES=120` runs a bounded startup
and shutdown; `WOW_CLIENT_CAPTURE=/absolute/path/to/login.png` saves the final
GPU image. With no frame limit the window remains interactive. The original
login interface and agreement display have run for 120 frames under Mac
ASan/UBSan and Metal validation, followed by the original client teardown.

`WowAppleClient` also builds the original client for iOS devices and the arm64
simulator; `WowAppleBootstrap` remains the separate component preview/check app.

```sh
cmake --build --preset ios-arm64 --target WowAppleClient
cmake --build --preset ios-simulator-arm64 --target WowAppleClient
```

The iOS app expects the original `Data` directory under its `Documents`, exposed
through Finder file sharing and Files. Import a copy, then return to the app.
Configuration, cache and logs go to `Library/Application Support/WoWDecompClient`;
the app never writes to Data. `WOW_DATA_ROOT` and `WOW_RUNTIME_ROOT` can override
these locations for development. Signed device deployment requires a development
team. The host runs the original Glue, networking, world and rendering code, in
landscape at native display resolution. It pauses frame submission and native
audio output while inactive, drains submitted GPU work, saves configuration on
backgrounding and resets the first resumed frame's elapsed time. Touch controls
and a mobile login UI remain outside the current scope.

The iOS host accepts `--local-unchecked`, `--login-stdin`, `--enter-world NAME`,
`WOW_CLIENT_FRAMES` and `WOW_CLIENT_CAPTURE` as described below. Simulator local
mode connects to the Mac's loopback AzerothCore. Since `simctl launch` does not
forward stdin, the simulator-only `--login-pipe PATH` can read two credential
lines from an owner-only FIFO inside the app's data container, then removes the
FIFO. Use an interactive writer with password echo disabled; never use a regular
file, command-line arguments or environment variables for credentials. This
development path still requires existing agreement preferences and does not
accept agreements automatically. It is not a device login or touch-input adapter.
The iOS 18.5 arm64 simulator has authenticated to local AzerothCore, entered an
existing character's Stormwind world at 2622x1206, resumed after backgrounding,
and completed 2400 frames and original shutdown with Metal validation enabled.
Its nine synchronous loading frames use separate layer drawables. Both iOS
targets build; signed physical-device deployment remains unverified.

For a local development server, `--local-unchecked` explicitly omits executable
challenge verification and restricts Grunt authentication to `127.0.0.1:3724`
and realm endpoints to literal `127.0.0.1` with a valid port. The server must
already use `StrictVersionCheck=0`; the client does not change that setting.
Resource/GlueXML signatures, SRP proofs and encrypted world authentication remain
active. Without this flag no unchecked challenge is supplied. Unsupported server
integrity modules still reject the session. Apple uses the Mac Glue platform
branch and has no Windows Scan.dll startup phase.

`--login-stdin` is an optional local development input path. It reads account and
password with terminal password echo disabled, submits them through the original
Glue login after the UI starts, and clears the password. It requires
`--local-unchecked`, an idle login screen and existing agreement preferences;
it does not accept agreements or create/delete characters. A 600-frame session
has reached the original character-selection screen and shut down normally under
Mac ASan/UBSan and Metal validation.

`--enter-world CHARACTER` additionally requests original world entry once that
exact character is selected and ready. It requires `--local-unchecked` and does
not change the selected character. Map loading acquires subsequent native
drawables for the original synchronous progress paints. Unsupported world
operations still report failure explicitly.

`--check-world-input` with `--enter-world` is an explicit local development check
that briefly moves the character, turns it, zooms the camera and jumps through
native events. It first checks the existing W/S, arrow, wheel and Space bindings;
it does not remap them or set player coordinates. Allow at least 900 frames for
the full sequence. World rendering, these inputs and normal shutdown have passed
Mac ASan/UBSan and Metal validation in a 900-frame local session. Full combat,
area transitions and sustained play are still being integrated and verified.

`--check-world-actions` verifies self-targeting, an existing Mark of the Wild
action, server aura/mana updates, backpack open/close and cancellation through
the original buff icon. It requires a druid with that spell on the current
regular action bar and the existing F1/B/number bindings; it applies or refreshes
and then cancels the buff. It does not change bindings or action slots. Allow
1200 frames, or 1800 when combined with `--check-world-input`.

`--check-world-casting` uses the existing Healing Touch action and F1/Escape/number
bindings to cancel one cast, wait beyond its cast time without spending server
mana, then complete a second cast. The original two-second cast bar and server
mana change (449 to 401) passed with spell 5186 in a 1500-frame local session,
including normal shutdown, Mac ASan/UBSan and Metal validation. This check heals
the selected player; it does not validate hostile combat or damage.

`--check-world-reentry` additionally opens the original game menu, clicks Logout,
waits for the server and character-selection screen, then clicks Enter World for
the same selected character. It never forces a logout or resets game objects.
Allow 3000 frames to include both world loads and a possible logout countdown.

macOS host frames explicitly track the window's native backing pixels before
acquiring Metal attachments, including Retina scaling and interactive resizing.
The original login now renders at the requested 1024x768 pixels on a 2x display;
window resizing, color/depth readback, input coordinates and original display
restart/rollback pass the native UI check under ASan/UBSan and Metal validation.

The native `FFXNetherBlur`/`FFXNetherCombine` paths preserve the original noisy
four-tap RGBA blur, fixed UV bias, two-pass intermediate surface, tint and fade.
`WOW_METAL_CHECK=ffx` focuses the existing Metal probe on post-processing
equations and original game passes. These checks pass on Mac with ASan/UBSan
and Metal validation and on the iOS simulator; the device target builds too.
This does not establish Windows GPU equivalence or complete gameplay coverage.

`FFXPropagateFog` and `FFXFogCombine` now preserve four-tap RGBA propagation,
alpha decay, component-wise density/minimum saturation and fog-alpha composition.
The original propagation pass with separate input/output and persistent 33x33
polar composition grid pass on Mac and the iOS simulator. The complete swirling
fog effect still needs its same-texture read/write behavior checked against the
original D3D9 path; active render-target feedback remains explicitly rejected.

World loading now also accepts the original `Diffuse_Env` reflection-only
vertex family and `Combiners_Opaque_Mod2xNA` material. The former uses sphere
reflection for texture 0 without authored UVs; the latter doubles layered RGB
while keeping vertex alpha. `WOW_METAL_CHECK=model-shaders` checks these with
the existing model variants/materials on Mac and the iOS simulator. Real local
world entry has reached server object updates; player creation is still being
integrated.

`AppleRealmClient` is a separate macOS command-line entry for a local AzerothCore
server. It performs real SRP authentication, realm selection, encrypted world
authentication and character enumeration using the original client code and
the mounted game's AddOn metadata, then disconnects. Run it interactively:

```sh
cmake --build --preset macos-arm64 --target AppleRealmClient
build/macos-arm64/bin/AppleRealmClient --local-unchecked /path/to/client335a/Data
```

Account and password are read from standard input; terminal password echo is
disabled. Credentials are not saved. The explicit development option connects
only to `127.0.0.1:3724` and an online realm advertised at `127.0.0.1`. It requires
the server's existing `StrictVersionCheck=0` setting: executable verification is
not performed, while SRP proofs and world-session authentication remain required.
It does not change server settings or substitute for the pending native client
verification policy. A real local session has authenticated and enumerated an
existing character under Mac ASan/UBSan. This entry has no login UI or world
entry and does not create/delete characters.

The original AddOns/cache implementations and Storm CRC/RSA/SHA1 signature stream
are linked. An independent synthetic signature fixture checks seven streaming
boundaries and rejects modified payloads, markers, signatures and truncation;
its key is confined to the probe. Existing asset keys and policy are preserved.
The resource check also loads the original BannedAddOns DBC byte for byte.
ClientData selects and restores the cache locale alongside the DBC locale;
native installation detection uses the selected Data root. Native WDB and addon
policy caches use the system Caches directory under `wowdecomp/WDB/<locale>`;
`WOW_CACHE_ROOT` can select another absolute writable root outside the read-only
Data directory. The original record formats, temporary-record policy and server
version invalidation are retained. Invalid or overlong paths fail instead of
falling back to the working directory. The cache probe saves and reloads original
records in an isolated temporary root, compares bytes, checks language isolation
and version invalidation, and rejects truncated addon policy records.
Full AddOns/UI startup and real executable version checks remain pending.
Warden cache serialization is shared; native execution of Windows
Warden modules is not implemented.
The combined native client now links the original Glue initialization. Native
URL requests go through AppKit/UIKit; remote streaming scheduling hints are
inactive in a full local installation. Windows patch execution reports failure.
Executable challenges raise an explicit unsupported error. Encrypted integrity
modules and armed scans log the unsupported request and disconnect without
emitting a digest or fabricated result, or unwinding through the original network
queue's locks. Warden initialization reports unavailable and installs a rejecting
message handler; its idle processing owns no module runtime. These boundaries
enable startup integration without claiming those services are implemented.
The separate local realm command above remains the explicit development policy.

The original account-settings synchronization now runs on the native Realm
session too. `AccountDataHost` supplies the session and selected identity; the
normal Glue entry still binds the original ClientServices/CGlueMgr. Downloads,
compressed uploads, MD5/time caches, old-file backups, delayed saves, and the
original synchronization switches are retained. Native account/character files
live under Application Support's `wowdecomp/Account/<account>[/<realm>/<character>]`.
`WOW_SETTINGS_ROOT` selects an absolute persistent settings root for these files
and the preview's existing `Config.wtf`; this is separate from disposable WDB
caches and read-only Data. Invalid path components fail. Native downloads validate
the complete compressed payload before atomic replacement, preserve existing
files/checksums on failure, and use an explicit 16 MiB per-file budget. Shutdown
cancels outstanding save timers and load callbacks. Two encrypted loopback
lifetimes check transfers, local-only layout, disabled synchronization, unchanged
saves, backups/reload, invalid types/lengths and recovery. Native platform bot
checks remain explicitly unsupported; no success response is fabricated.

The shared Event scheduler now has a nonblocking native-host step. Mac/iOS
previews render continuously from the original paint event, preserving update,
timer, input, poll, queued-message, idle and paint ordering. Focus/resize events
come from the host. The Mac view now delivers physical keys/repeat, independent
left/right modifiers, drawable-pixel pointer coordinates, buttons, wheel and
relative motion through Os/Event. AppKit text commits become Unicode scalar
events; IME updates own their UTF-8 snapshots and use the original OsIME getters.
Candidate presentation stays with AppKit; native UI hosts can supply the original
edit-box caret. Secure text entry and focus loss balance system input capture;
interactive IME-provider testing remains. `AppleInputProbe` checks this boundary with local AppKit events
and the original Event dispatcher. iOS touch interaction remains deferred.
Foreground resume resets frame elapsed time while retaining
wall-clock timers. Context shutdown and recreation release property TLS, call
contexts, queues and timers. Native stepping currently requires the client's
single scheduler thread; server network pumping is explicitly unsupported.

Original Frame/FrameXML/Scrn now build on all Apple targets. On Mac,
`build/macos-asan/bin/AppleUIProbe /path/to/Data --preview` opens an interactive
component window using original Glue fonts/button templates, the original color
wheel/value selector, original CGCooldown, Lua callbacks, CSimpleTop layout, Services textures and
Scrn painting. The original console overlay is available with the tilde key in
this Mac component host. It retains command history/completion, colors and the
256-line scrollback. Native Unicode scalars are encoded as UTF-8, with character
boundaries preserved by cursor movement and deletion; worker diagnostics are queued for game-thread font creation.
Its access flag is explicit in the component host; full client startup will
continue to obtain it from the original `-console` option. The color selector uses its original generated textures, HSV/RGB
methods and mouse tracking; the tabard/color texture callback is shared with
the game UI. Clicking the spell icon starts the original three-second cooldown,
including its eight-octant sweep, rotating edge and completion flash. Reverse
sweeps and the original Lua time conversion are also exercised by the GPU probe.
Its original edit box
accepts native Unicode/composition and supplies the AppKit candidate rectangle.
Without `--preview`, it checks two full lifecycles, native button hit testing,
composition replacement, caret movement and GPU output. `WOW_UI_FRAMES=120`
closes the visible check after 120 frames. Set `WOW_WORLD=Azeroth:-9465:64:85`
to compose the original world below the UI through Scrn. The viewer's movement
and right-drag are suspended while the console is open or the edit box has keyboard focus; clicking
the button clears that focus. `WOW_UI_CAPTURE=/absolute/path/frame.ppm` reads
the composed GPU frame at frame 120. This is an integration entry point;
full Glue/game UI, name-cache declension and login remain pending. Original
MovieFrame playback is available through `WOW_MOVIE` as described above.
Clipboard has native UTF-8 platform bindings; these checks do not access the
user's clipboard. UI Desaturate uses the original BLS luminance and alpha rules.
Mac and iOS GPU checks now share this component initialization and exercise
Unicode/composition delivery, original Lua button hit testing, two complete
lifetimes and exact output after changing/restoring drawable dimensions and
aspect ratio. Headless/iOS pointer queries use the last native absolute event
or the initial drawable origin; iOS touch integration remains pending.

The original Services asynchronous file queue now runs on native workers and
returns completion through Event poll handlers on the game thread. Idle Apple
workers wait for queued work or shutdown. Failed/short reads select cleanup
instead of successful loading, retries restore the starting file offset, and
cancellation/destruction synchronize with active readers. Checks compare real
DBC, script, M2, ADT and WMO bytes with synchronous reads, then exercise failure,
cancellation and worker recreation. The visible native preview now uses the
original `TextureCreate` handle, filename/flags cache, async BLP decoding and Gx
upload/recreation callbacks. Original BLP reads validate file/mip extents and
memory budgets; decode scratch grows for actual asset dimensions. An oversized
texture can run alone above the normal 4 MiB queue budget, and cancellation of
unallocated requests preserves the budget. Native absolute SFile2 paths allow
resources in app-owned directories. Real cached texture mip reloads and a 2048
square palette upload are checked through GPU readback. Remaining texture formats,
atlases, complete model rendering and full client startup are still being connected.

The original character-composition source texture cache now runs through the
native Services worker. It preserves authored palette/mip bytes and shared
ownership, validates the complete BLP before publishing it, and handles worker
failure, cancellation and retry. Native entries retain the full file size instead
of the original 20-bit field, accept client-length paths, and reuse their empty
ObjectAlloc heap across lifetimes. Files have an explicit 64 MiB input budget.
Two lifetimes check a >1 MiB source, all palette alpha formats, long paths and
failure/cleanup; twelve real CharSections skins retain all reachable mip and
palette bytes. This is the source cache needed by character composition; the
complete character assembly and texture-composition pipeline remain pending.

The original Model2 and ShaderEffect sources now compile for all Apple targets.
The data probe relocates the original camera and NorthrendSkeleton M2 arrays,
loads a real external animation through `CM2Shared`, and checks bounded nested
tracks and retry after a truncated animation. Apple model roots use separate VM
allocations so external animation and expanded vertex data remain within the
32-bit relative-pointer range, including under ASan. Native NEON matrix blending
and CPU skinning retain the original weighted/one-bone calculations and both UV
sets; the same kernels retain an SSE path for x86. The original scene/model
instance path also loads the real FlyByDwarf cinematic model and skin through
Services workers, plays its camera animation, and recreates the scene/cache.
It uses the original AnimationData DBC and bounded `World/model.blob` index.
The native instance arena aligns pointer-bearing arrays for 64-bit hosts.
The original `CM2SceneRender` now draws a real wolf with its original packed
vertices, indices, walk sequence and DBC-selected skins. Shared monster appearance
code applies the original texture slots, sampler flags, geosets and particle color
replacements. The native `Diffuse_T1` / `Combiners_Opaque` / `Combiners_Mod`
materials preserve 30 unshadowed vertex variants: rigid, one-bone and four-weight
skinning, directional and up to four local lights, material colors, UV matrices
and vertex fog. Four pixel variants per material preserve opaque versus textured
alpha and independent shader/state alpha tests, including equality boundaries.
Runtime function constants specialize the original choices; Gx register uploads
become typed native data, and only the referenced palette range is uploaded.
Original SM3 bytecode fingerprints guard supported variants; unsupported shadow
variants fail explicitly. GPU checks cover those variants, both real wolf material
batches and changing bone/pixel output as its original animation advances, including
scene/cache recreation. The original `Particle.wfx` now loads both particle effects;
`Color_T1` and `CDiffuse_T1` cover their ten rigid vertex variants each, preserving
vertex color, the original white-output branch, directional/local lighting and fog.
The original fireball emits and renders four particle batches through its own
billboard geometry and blending. Rejuvenation's five original ribbon emitters draw
their streamed triangle strips; its sphere/plane particles also reach the renderer.
The scene probe checks ribbons separately before enabling the particle emitters.
Effect loading checks complete bounded reads,
argument lengths, scopes and the original pass/stage capacities. Model and particle
constructors initialize reserved flag bits instead of retaining allocator contents.
Other model materials, colored skinning, remaining shadows and full world rendering remain
open; these checks are not a Windows visual-equivalence claim.

`Diffuse_T1_Env` adds 30 unshadowed model vertex variants with original view-space
sphere reflection. `Combiners_Opaque_AddNA` and `Combiners_Mod_AddNA` add a separately
sampled environment RGB contribution before fog, retaining their distinct base
alpha and alpha tests. Function constants remove unused reflection work, and the
pipeline key has separate space for the growing effect set. Numeric GPU checks
cover both texture samplers, reflected normal directions, skinning, lighting and
alpha boundaries; the real water elemental uses its original DBC-selected skins,
three environment batches and remaining single-texture material passes.

`Diffuse_T1_T2` adds 30 unshadowed variants with two authored UV sets and independent
texture matrices. `Combiners_Mod_Mod`, `Combiners_Opaque_Mod2x`, and
`Combiners_Opaque_Mod2xNA_Alpha` preserve multiplied layers, double modulation,
and the original combined opaque/reflection/alpha material. Their RGB and alpha
rules are separate, including the original double-modulated alpha and shader-test
operation order. Numeric GPU checks cover both UV matrices, samplers and alpha
boundaries. The original motorcycle (display 25870) draws two combined material
batches; its dual-UV death effect is exercised at 1137 ms, following the authored
color-alpha track instead of forcing visibility.

`Combiners_Mod_Mod2x`, used by authored Northrend sky models, now preserves
double-modulated RGB and alpha from both textures and vertex color. Its four
unshadowed BLS variants are verified; 246 additional GPU checks cover layered
coordinates, lighting/fog, and independent shader/state alpha boundaries on Mac
and iOS simulator. Mac ASan/UBSan and Metal validation pass; both iOS targets build.

The original map load also reaches `Combiners_Opaque_Add`,
`Combiners_Opaque_AddAlpha`, and `Combiners_Opaque_AddAlpha_Alpha`. Metal now
preserves their distinct additive alpha, alpha-weighted reflection, and combined
base-alpha interpolation, including the authored lighting/fog order. Their twelve
unshadowed variants add 738 GPU references across both UV paths, skinning,
lighting and shader/state alpha boundaries.

Native `ShadowMap` / `ShadowMapSL` generation preserves rigid, single-bone and
four-weight transforms, separate light-view transforms, linear depth and alpha
cutouts. All 90 vertex and 16 pixel handles are validated against the mounted
original shader data. R32F targets preserve full float depth, exact copies,
regional clears and D3D9 absent-channel sampling values; unsupported float
filtering and blending fail explicitly. The original wolf shadow gather/draw
path exercises both opaque and cutout batches. Complete world shadow queries
and PCF depth textures are still incomplete.

`Diffuse_T1` also supports its 60 R32F dynamic/banded-shadow vertex variants, paired
with `Combiners_Opaque` / `Combiners_Mod` pixel variants 1/2/3 and 9/10/11.
Native typed constants preserve the original projected coordinates, five/nine LOD-zero samples, lit depth
equality, border/distance/normal fades, RGB modulation before texturing/fog, and
unchanged alpha. Three distant bands retain their separate projections/samplers,
strict near/middle footprint boundaries and far-border fade. Model mode 3 keeps
its original five-tap dynamic sample without border fading; both banded modes
take the minimum after dynamic distance fading, then apply normal fading.
These rules differ from the terrain receiver. `Diffuse_T1_Env` also supports its
60 shadow vertex variants with `Combiners_Opaque_AddAlpha` receivers. Its reflection
coordinates remain independent; reflection RGB is weighted by its own alpha and
added after the shadowed base, before fog. The numeric fixture checks 19,710
independent outputs including both vertex families, skinning, sampling/band
transitions, fade order, independent reflection and shader alpha boundaries. The original wolf scene generates its R32F map from
both opaque/cutout batches, selects both receiver materials through the original
scene/cache code, and produces 125/130/130 darker pixels in the three modes with
unchanged alpha coverage across two resource lifetimes on Mac and iOS simulator.
Numeric and scene checks pass with Metal validation (Mac also under
ASan/UBSan); the Mac application and both iOS client targets build. These checks
use explicit shadow/light inputs; they do not establish full world shadow behavior
or Windows visual equivalence. Other model families/materials and hardware PCF
remain unsupported.

Native `DetailDoodad` rendering preserves sun and three-point-light paths,
unnormalized terrain normals, vertex tint, distance fading, fog and independent
state alpha testing. Sixty numeric GPU draws cover both non-shadow vertex and
pixel variants. Original grass and bone resources pass through the world loader
and `CDetailDoodadInst` vertex/index generation and texture batching, exercising
both DBC orientation flags across resource recreation. These checks use explicit
instance placement and lighting; world density/placement and shadow receivers
remain incomplete.

Native `WaterRipples` preserves texture RGB, vertex-controlled alpha and additive
blending. Its normalized D3D9 polygon offset is applied after clipping and
viewport mapping, with a separate zero-bias pipeline. Thirty-nine numeric draws
check color, alpha, perspective transforms, positive/negative bias, depth-write
independence and clip boundaries. Original ripple/wake creation queries real
Elwynn MH2O triangles and renders both original textures through the world batch
routine, including growth, fading, expiry and recreation. Local projection
matrices no longer read the unused UI camera. This is a focused world integration;
unit-driven ripple spawning and polygon offset for programmable effects other than water ripples remain pending.

Original `Model2.wfx` projected materials now use native view-space projection
for both texture coordinates, with the authored ModMod/ModAdd color and alpha
rules. Forty additional vertex permutations cover rigid and skinned lit
receivers and rigid colored receivers; 320 numeric GPU draws exercise their
projection, lighting, sampling, fog and alpha boundaries. Original acid, blizzard
and consecration models also pass through `CM2SceneRender::DrawBatchProj`, using
the original procedural depth-fade textures. Those checks use an explicit planar
receiver; complete world receiver queries and occlusion remain unintegrated.

The macOS preview and iOS Metal view run that original M2 scene beside the texture
and font preview. Model/skin/texture loading is asynchronous; the game Event update
drives animation time. The default is CreatureDisplayInfo 165 (timber wolf), playing
its original walk animation. Set `WOW_CREATURE_DISPLAY` to another positive DBC ID,
or `WOW_MODEL` to an explicit archive path without creature appearance replacement.
`WOW_CHARACTER=1:0` selects a human male through the original character component,
including asynchronous skin/face/hair composition and the configured compression.
The syntax is `race:sex[:skin:face:hairStyle:hairColor:facialHair:chestDisplay]`,
with omitted appearance fields defaulting to zero and validated by the original
creation rules; sex is 0 or 1. For example, `WOW_CHARACTER=1:1:0:0:0:0:0:220`
shows a human female wearing the real chest display 220. This scene selection is
exclusive with the other scene environment options. It is a character preview,
not a player session or a replacement for the unfinished Glue/character UI.
Set `WOW_CHARACTER_OUTFIT` to an original class ID to apply its starting outfit
through the shared character-creation dress routine. For example,
`WOW_CHARACTER=4:0 WOW_CHARACTER_OUTFIT=3` selects a night elf male hunter,
including its bow and quiver; `WOW_CHARACTER=1:1 WOW_CHARACTER_OUTFIT=1` selects
a human female warrior. Race/class combinations come from CharBaseInfo.dbc;
the optional chest display overrides the outfit's chest afterward.
For focused local iteration, `WOW_METAL_CHECK=models` runs the original model and
character checks twice through the Services/Event lifecycle; the result explicitly
identifies this subset. Omit it for the full Metal probe.
The arm64 compressor uses native NEON while retaining the original SIMD path's
integer mean, covariance order, power iterations and ties-to-even DXT1 quantization.
Native division/square root differ from x86 approximate reciprocals at some
quantization boundaries; full original Windows visual equivalence remains pending.
For a local compressor comparison, the Metal probe can append composed four-by-four
BGRA blocks to a derived file selected by `WOW_CHARACTER_BLOCK_DUMP`. This optional
diagnostic includes the real bare/equipped character mip inputs and does not write
to the original Data directory.
An unsupported shader reports its name and stops rendering.
`WOW_MODEL='Spells\Fireball_Missile_Low.m2'` exercises original particle emission
and drawing; `WOW_MODEL='Spells\Rejuvenation_Impact_Base.m2'` also exercises ribbons
and sphere emitters. The view owns depth,
scene/cache lifetime and foreground recreation. This remains a native rendering
bootstrap, not a playable client or a replacement for the game's UI.
`WOW_CREATURE_DISPLAY=110` selects the original water elemental and its environment
materials; `WOW_CREATURE_DISPLAY=25870` selects the motorcycle and its layered
materials.

The shared `WorldClient` static library now builds for all three Apple targets.
The original world-parameter and startup entry points now link on macOS; startup
currently stops explicitly at the unimplemented `DetailDoodad` effect.
Apple Storm settings use the app-owned `org.wowdecomp.client` Core Foundation
preferences domain, preserving user/system lookup flags and 32-bit values.
The normal game CVars continue to use their configured WTF file.
Its platform fixes retain Windows headers on Windows, use the shared allocator
type-name helper and portable stack allocation/finite checks, and keep anisotropic
filtering's flag at its original 32-bit width. This is a compilation milestone;
full world startup is not yet connected to the native host.
The shared terrain surface reader now copies unaligned ADT numeric data into
aligned arrays, validates chunk/index extents, texture references and compressed
alpha rows, and leaves archive bytes unchanged. Original world-space PN/PNC vertex
generation and hole-aware indices are shared with `CMapChunk`, retaining its
arithmetic and triangle ordering. Core checks read complete Ahn'Qiraj, Elwynn and
Northrend tiles, including vertex colors, holes and compressed alpha maps. This
surface view does not replace the WMO, doodad, liquid or sound loaders.
The shared BSP digest/cache and bit-array implementations now build natively;
checks use 961 actual WMO nodes and 465 leaves, preserving face flags/order and
geometry and clearing cached node pointers on release. LiquidType/LiquidMaterial
DBCs also use their original loaders; all 26 liquid and three material records
are checked against archive bytes on macOS and the iOS simulator. Original world
objects now link through the shared sound-object lifecycle and extracted world
UI model hit-testing implementation.

The original WMO root loader now owns aligned, bounded chunk payloads. Disk
material records remain 64 bytes; native texture pointers no longer occupy an
unaligned archive address. Altar of Storms, Stormwind and Dalaran retain all 417
materials, 378 groups and 10,831 doodads, including the original behavior of using
chunk lengths when patched files contain stale header counts. Original async
postload allocates the world groups and loads the altar's material textures through
Services/Gx. Truncation, failed-read cleanup/retry, cancellation and recreation
pass on macOS with ASan/UBSan and Metal validation, and in the iOS simulator;
both iOS targets build. The original group loader also owns aligned geometry,
BSP, colors and optional payloads; packed MLIQ vertices have separate aligned
storage. Fifteen actual groups (including twelve liquid grids) preserve authored
geometry/batches, the original vertex-color correction, liquid heights and tile
flags. Async and synchronous reads, failed group reads/retry and cancellation pass
with macOS ASan/UBSan and Metal validation and in the iOS simulator; both iOS
targets build.

Seven classic/unified WMO vertex families and seven pixel families now have
native Metal implementations: baked/dynamic color, zero to four local lights,
direct reflection coordinates, composite layers, fog and shader/state alpha
tests. The 210 vertex and 28 pixel permutations without shadows retain the
mounted SM3 program semantics, checked with 390 independent GPU reference draws.
The original `MapObjU.wfx` selects materials, and original `CMapObj::UnifiedRender`
uploads geometry and submits the material states and transition passes.
`WOW_WMO='World\wmo\Northrend\Dalaran\ND_Dalaran.wmo'` selects a root's group zero;
append `:groupIndex` for another group. This is mutually exclusive with terrain
and model selections. Altar, Stormwind and Dalaran groups produce 10, 5 and 30
original draws respectively, including Dalaran's nine two-pass transition batches.
Each uses one resident render pass and reproduces static pixels after recreation.
macOS ASan/UBSan and Metal validation pass; both iOS targets build. Mac and iOS
present 120 WMO frames alongside the original archive audio stream; the simulator
repeats both after foreground resource/device recreation. The preview
supplies explicit camera/light/fog inputs. Full placement/portal behavior and
remaining shadow modes still need complete world validation.

All seven WMO families also support their 420 dynamic/banded-shadow vertex aliases
and 42 R32F receiver pixel variants. Shared native M2/WMO sampling retains five/nine
LOD-zero taps, depth equality and border/distance/normal fades; WMO keeps its
separate baked color, doubled diffuse contribution, environment layer and alpha
semantics. Four independent projection/sampler bindings retain near/middle/far
selection, distance/normal fade order and the highest mode's distinct close-map
rules. `WOW_METAL_CHECK=wmo-shaders` runs 9,360 WMO numeric draws plus the
19,710 shared M2 shadow references. Mac ASan/UBSan and Metal validation pass;
the same references and subsequent 120 preview frames pass on iOS simulator.
The Mac CLI/application and both iOS client targets build.
The full Mac client entered Feral's Stormwind world with the original
`extShadowQuality=1` option in a separate runtime, rendered 1,200 frames, captured
the world and shut down normally. This removes the WMO receiver failure found
on that real path; it does not establish every zone or Windows visual equivalence.
With `extShadowQuality=3`, the full Mac client also completes 1,200 frames, world
readback and normal shutdown after adding the actual environment-reflection M2
receiver (vertex 71, pixel 2). This is one Stormwind view, not complete world or
quality-switch validation. WMO hardware PCF remains explicitly unsupported.

Original LiquidSystem now builds on Apple arm64. Native water, water without
specular and magma preserve all nine vertex/three pixel variants, their distinct
UV sources, unnormalized normals, zero to three local lights, specular/alpha
rules and fog. Independent GPU references cover both normal directions and
alpha-state boundaries. WMO groups with liquid automatically use the original
material/settings banks, 30-frame MPQ textures, depth-coordinate tables,
procedural water-color textures, geometry factory and renderer. The preview
supplies explicit water-color bands and lighting. Use the WMO selectors
`World\wmo\Azeroth\Buildings\Stormwind\Stormwind.wmo:224`,
`World\wmo\Northrend\Dalaran\ND_Dalaran.wmo:71`, or
`World\wmo\Dungeon\AZ_Blackrock\Blackrock.wmo:43` for exterior water, interior water
and magma. Their original 5x5, 14x46 and 29x31 grids render with visible texture
animation; resident geometry is reused through repeated draws and recreation.
The rebuilt liquid queue now dereferences its pointer-array entries correctly,
and a mesh factory retains one buffer reference across frames, releasing it on
purge/destruction. Signed color-gradient arithmetic preserves descending bands
without undefined shifts. macOS ASan/UBSan and Metal validation pass, including
Blackrock geometry/magma and archive audio running together for 120 frames;
both iOS targets build. The simulator passes these checks and presents 120 frames
of Dalaran geometry/water with audio, then repeats them after foreground resource
recreation. Terrain MH2O now uses a bounded, owned payload with naturally aligned
chunk/layer descriptors; rejected replacements retain the previous data. Authored
heights, depths, UVs and existence/fishable/deep bitmaps retain their values,
including implicit ocean defaults. Original MapArea uses the same bounded reader;
legacy MCLQ records now have bounded aligned ownership too. The original MCNK
length remains authoritative when the MCLQ size word is zero. Original chunk
creation and previews share the height/depth/UV initialization, while tile flags,
strip holes and layer geometry remain in CChunkLiquid. Tile height queries
reject the exclusive upper edge before indexing neighboring vertices. Original
Map.dbc selects Outland's green magma type; its localized fields use the selected
archive locale (zhCN slot 4 on the supplied installation), scoped to ClientData's
lifetime. Actor ripples, procedural-wave material 3 and fixed-function liquid
paths remain to be connected.
Original terrain alpha/shadow unpacking is now shared as well: four-bit maps stay
packed, eight-bit maps retain RLE values, and the original edge replication and
half-resolution source selection are preserved. Checks compare all texels against
the three tiles' raw alpha/shadow data and verify that disabling shadows leaves
material weights unchanged.
The native Terrain/Terrain1 surface path supports 32 vertex and 16 pixel variants:
one to four layers plus their independent blend/shadow map, PN/PNC input,
three point lights, specular, vertex fog and per-layer lighting masks. Old-alpha
interpolation and big-alpha weighted sums retain their distinct semantics.
256 GPU draws compare independent position/UV/light/fog/material references,
including packed and RGBA blend maps. macOS ASan/UBSan and Metal validation pass;
both iOS targets build and the simulator passes the checks and presents 120 frames.
Terrain2/Terrain3 add 32 projected-shadow vertex and 64 pixel variants, with
five-tap dynamic shadows, three distant bands, and the original nine-tap close
map selection. Affine view-space coordinates, explicit LOD zero, equality-lit
comparisons, baked-shadow combination and distinct edge fades are preserved.
Sparse binding reaches texture slot 8; absent/unused blend maps remain optional.
The shared CMap shader initialization/selection and ShadowCache matrix/offset/
texture binding run with Services-owned R32F targets. The expanded probe makes
2,816 independent reference draws, including band boundaries, out-of-range depth,
packed/RGBA layer maps, and the existing surface permutations. Pipeline cache
keys now use separate material fields instead of a nearly full packed bit word.
At the original close-map edge near 0.99, float multiply/add rounding can change
a nearly zero border test and select a different band across GPUs. The shader's
threshold is preserved; original Windows behavior at this edge remains unverified.
macOS ASan/UBSan and Metal validation pass; both iOS targets build and the
simulator passes the GPU checks and presents 120 terrain frames. Full world
shadow query/update integration and PCF depth textures remain pending.
Terrain0 adds the three original non-environment multi-pass materials: layer
alpha for old-alpha blending, remaining-weight base color, and selected-channel
additive color. Weighting stays after lighting and before fog. Original
GetPixelShader selection, packed/RGBA inputs and alpha equality/discard behavior
pass 192 additional reference draws on Mac and iOS simulator; both iOS targets
build and the simulator presents 120 terrain frames. Full world multi-pass
submission is still pending.
Terrain0_env and all Terrain1/2/3 environment variants now use the original
unnormalized view-space reflection and x/z/y rotation. All 128 terrain vertex
and 164 surface pixel programs are fingerprint-checked. Cube resources preserve
six faces, authored mip payloads, callback face order, local updates, base mip
recreation and earlier encoded draws. Native Metal cube storage has a 2D-array
sampling view to retain D3D9 face-local wrap/clamp filtering; projected gradients
retain mip and anisotropic filtering without derivative jumps at face changes.
Six filter modes, integer/fractional LODs and face-edge samples are checked.
Four real Services BLP cubes provide 156 face/mip reference images per resource
cycle. Three BC1 assets have single-block placeholders below 4x4 per face;
the original Gx compressed path stops at 4x4. The bounded BLP reader now accepts
that unused tail without exposing it as six-face data, while rejecting truncated
usable levels. No mip generation or lossy conversion is added. macOS ASan/UBSan
and Metal validation pass; both iOS targets build, and the simulator passes
the complete GPU checks and presents 120 terrain frames. Its lack of BC hardware
also exercises six-face lossless expansion. Original Windows cube LOD/tie
behavior and physical iOS hardware remain unverified. Cube render targets/copies are still explicit
unsupported operations.
TerrainSM now emits radial distance and squared distance from the original
TEXCOORD5 affine coordinates, with scaling in the fragment stage and no texture
inputs. Its 64 compatible vertex variants pass 512 float/RGBA reference draws
plus varying-quad interpolation checks. Mac ASan/UBSan and Metal validation,
both iOS builds, and simulator GPU checks with 120 terrain frames pass. The
original CWorld initialization and destruction now complete twice on Mac and
iOS simulator, with real shader, footprint, doodad and liquid resources. The
shutdown dependency on LightRec is linked. Weather and particulate globals are
detached before freeing their instances, and footprint texture/batch caches are
cleared so reinitialization reloads valid handles. Live and between-world CVar
callbacks, portrait notification detach, Mac ASan/UBSan and Metal validation pass;
both iOS targets build and the simulator subsequently presents 120 terrain frames.
The lifecycle check now also enters the original `CWorld::LoadMap` / `CMap::Load`
with an explicitly supplied `CSimpleCamera`. Azeroth loads 16 areas, 1,600 chunks,
24 WMO placements, 4,856 doodad model instances and 687 WDL horizon tiles through
the original asynchronous resource paths. Both world lifetimes reproduce that
population and clear the map tables on teardown. WDL chunk headers use byte copies
because 545 authored 16-bit heights leave the next header only two-byte aligned;
height, hole and geometry algorithms are unchanged. Original object-manager lookup,
object-effect state and the UI's chat/movie sound state now link on Apple; no active
player or network session is synthesized. Mac ASan/UBSan, Metal validation and
iOS simulator checks pass, and both iOS targets build.
The native host now owns that world and enters the original `CWorld::Update` and
`CWorldScene::Render` with an explicit camera. The Windows entry retains its
WorldFrame camera, loading-screen, cursor and targeting callbacks. No fake player
or loading-screen UI is inserted into the native core. The Elwynn fixture renders
61 terrain chunks and prepares 23 doodad models, with 16,382 varied pixels and
7,096 main-scene depth pixels in a 128x128 readback, through two complete world
lifetimes. The original WorldFrame continuation is shared too: M2 pass order,
detail doodads, liquid, weather, barriers, particulate and glare retain their
original ordering, including the submerged-camera branch. Windows UI callbacks
remain at their original positions. The main scene retains WorldFrame's depth
range ending at 0.94, before the separate horizon and sky bands.

Use `WOW_WORLD=Azeroth:-9465:64:85` with the existing Mac `--preview` command or
as an iOS environment variable. The format is `Map:worldX:worldY:worldZ`; the
viewer uses an explicit camera (near 0.2, far 500, FOV 1 radian).
On Mac, WASD moves the viewer, Q/E moves down/up, Shift moves faster, and holding
the right mouse button rotates it. Escape releases the mouse and Home restores
the initial viewpoint. This is a free viewer with no player or collision response;
the game camera/player controls still require the original session and UI.
`WOW_TIME_MINUTES` sets the initial time, then advances in real time; authored
map time overrides are retained. This selection uses the full view for the world
and is mutually exclusive with terrain/WMO/model previews. Mac and iOS simulator
both present 120 world frames; the 900x600 Goldshire-area frame selects 125 terrain
chunks and 182 doodads, with trees, roads, fences and lights visible in GPU
readback. Mac ASan/UBSan and Metal validation pass, and both iOS targets build.
The native host still has no active player, login/network
session or complete game UI. All-map materials, interior
portal traversal, dynamic shadows and the complete WorldFrame/FFX/audio policy
remain to be integrated or validated.

Original DayNight selection/interpolation now links on all Apple targets with
715 lights, 850 parameter sets, 124 skyboxes, and 15,300 color / 5,100 float bands.
Both original RLE storage modes retain the DBC records. Coordinate conversion,
36,821 ordered color knots and midnight interpolation are checked; the shipped
duplicate-time band retains the original executable's interval selection.
Three map-light sets are exercised at five times of day. Original `DrawSky`
renders Hellfire's sky (34 batches) and Northrend's yellow/green aurora (21 batches),
including authored animation and stationary-frame readback through two complete
world/model-cache lifetimes. Bone-sequence queries now use the original signed
wrapping clock delta, preserving future starts, reverse playback and integer
precision; unsigned float conversion previously triggered UB for these skies.
The model cache follows the application's teardown order before a new world
starts, since world destruction also frees shader effects. The original procedural sky/cloud/planet and world camera-fog calls are now
reached by the native world scene; broader map/time coverage remains pending.
Mac ASan/UBSan and Metal validation, both iOS target builds, and simulator GPU
checks followed by 120 visible terrain frames pass.
Fog calculation now also accepts explicit camera-volume query results; the
original world entry retains its player/WMO query and evaluation order. Outdoor
distance limits, portal blends, all 26 liquid types with WMO flags, density mode,
and override restoration pass on Mac ASan/UBSan and iOS simulator. The original
executable confirms that portal blending keeps the blended color in the interior
record while updating the outdoor ranges/density; that distinction is preserved.
The native host can now draw a complete authored ADT surface with
`WOW_TERRAIN=Azeroth:32:48` (Elwynn) or `WOW_TERRAIN=Northrend:32:32`.
This selection is mutually exclusive with the model/creature variables. It uses
the original Services texture lifecycle and specular `_s.blp` selection, all
material layers, WDT-selected vertex colors, hole indices and packed/RGBA blend
maps. UV animation and per-layer lighting flags are preserved. Authored base-layer
cube reflections use the original view-transpose constants and shader selection;
`WOW_TERRAIN=Northrend:32:21` renders Crystalsong Forest with 51 reflective chunks
among 256 surface draws in one resident pass. Two resource lifetimes and stationary
frames are verified by GPU readback; camera rotation changes the output without
geometry reuploads. Mac ASan/UBSan, Metal validation, both iOS builds, and visible
120-frame runs on Mac and iOS simulator pass. The visible terrain host now uses the
original DayNight map-light selection, color/float bands, sunlight, fog and water
colors, referenced at the tile center. Its clock starts at noon and advances in
real time; `WOW_TIME_MINUTES=0` selects midnight, `720` selects noon (range 0 to
less than 1440). The overview camera moves closer to keep terrain detail inside
the authored fog range, without changing fog parameters. Elwynn and Crystalsong
night/noon outputs and stationary frames pass GPU readback through two resource
lifetimes, retaining one resident render pass. Numeric shader/geometry probes
keep their explicit lighting inputs. This remains an outdoor ADT overview:
world zone/interior/submersion queries, WMO/doodad placement and the visible
procedural sky are not integrated. The host owns the original CVar lifecycle and
uses `wowdecomp/Config.wtf` under Application Support (the app container on iOS),
separate from Data; pending settings save on suspension and teardown. Configuration
read failures stop the host, and suspension-time save failures are logged. Mac
ASan/UBSan and Metal validation, both iOS builds and simulator checks pass;
both visible hosts present 120 frames, including simulator foreground recreation.
MH2O and MCLQ liquids now use original CChunkLiquid vertices/triangle strips, adjacent 2x2
chunk batching, depth coordinates, original material/settings banks and animated
MPQ textures. Elwynn's 42 layers become 19 draws; Northrend's 256 ocean layers
become 64 draws. Their liquid-only views cover 1585/12195 pixels and show animation.
Liquid materials now prepare geometry before drawing, and Metal batches their
pending immutable private-buffer copies into one upload encoder. Original queue
sorting, material states and same-frame update order are preserved; eager upload
backends retain their lock/unlock path. Elwynn, Northrend and the two Outland
fixtures now use one render pass from their first frame (previously 20/65/63/17),
with three buffer upload passes total for surface plus liquid geometry. Resident
frames do not reupload geometry. Upload queues reuse storage; repeated preparation
is a no-op, and an updated buffer preserves earlier draws in the same frame.
macOS ASan/UBSan and Metal validation pass through two resource/context cycles;
Elwynn terrain/water and original audio run together for 120 frames. Both iOS
targets build; the simulator runs Northrend terrain/water and audio for 120 frames,
then repeats them after foreground resource/device recreation.
For legacy water use `WOW_TERRAIN=Expansion01:19:31`; for a water/green-magma
mixture use `WOW_TERRAIN=Expansion01:28:37`. These contain 208 and 31 original
MCLQ layers, including two chunks with both liquid types. Their 9292 visible
tiles retain exact heights/UVs and original hole topology; liquid-only views draw
62/16 batches and cover 12155/559 pixels with animation. Mac ASan/UBSan and Metal
checks pass through resource recreation, and the mixed tile presents 120 frames
with original streamed audio. Both iOS targets build; the simulator passes the
checks and runs the mixed tile/audio for 120 frames, repeating both after
foreground resource/device recreation. These previews still do not perform full
world loading. Non-base cube layers remain explicitly unsupported. Lit fixed-function terrain and complete world effect/state coverage remain
pending; the native world scene entry is described above.

The original FMOD software mixer, decoders, resamplers and DSP sources now build
on Apple arm64, together with SoundEngine and SoundInterface. The new OS boundary
uses pthread synchronization, monotonic timing, native files and POSIX sockets.
Audio Unit output preserves the selected PCM16 or float samples and fixed mixer blocks, with
native callback sizes bridged without changing the original mixer clock. It
currently exposes the system default mono/stereo route; surround channel layouts,
recording and full SoundInterface world/music policy are not integrated.
iOS uses an ambient audio session and handles interruptions; media-service reset
is reported as an error requiring recreation. Physical audio-CD devices are not
supported. The socket boundary has not yet been exercised by game networking.

`AppleAudioProbe --data-root /path/to/Data` checks original button WAV PCM bytes,
reproducible title MP3 decoding, looping, spline resampling and stereo pan. Add
`--output` to play a short title-music sample and verify native callback suspension
and resumption. The iOS app runs the resource checks during startup; set
`WOW_AUDIO_CHECK=1` for the same output check. macOS ASan/UBSan and both iOS builds
pass. Windows audio equivalence, surround output and device-route changes still
need validation. The port fixes negative resampler look-behind offsets on LP64,
MP3 Huffman bit shifts and overlapping ID3 tag copies without re-encoding assets.

The native host now runs the original SE2/SESound engine, FrameScript Lua setup,
and asynchronous SFile open/read/seek callbacks. Set
`WOW_AUDIO='Sound\Music\GlueScreenMusic\Wotlk_main_title.mp3'` to stream the
original MPQ music alongside the model or terrain preview. The explicit preview
uses looping 2D playback at 10% volume; it does not select world music or ambience.
The host's Event heartbeat completes loading and advances sound state. Audio
output stops on application inactivity and resumes with the host; destruction
releases sounds before Event and archive teardown. `AppleAudioProbe --data-root
/path/to/Data --game` checks two engine recreation cycles, archive streaming and
output suspension/resumption under ASan/UBSan on macOS. Apple enumeration no
longer opens a second output device. The port also repairs the master channel
group's negative parent index, Lua's null open-upvalue conversion and the MP3
decoder's multidimensional end pointer; worker stacks accommodate the original
archive callbacks on arm64.

The native texture path reads BLP2 palettes (0/1/4/8-bit alpha), BGRA8 and
BC1/BC2/BC3, checks mip extents and memory budgets, and preserves authored mips.
It uploads original BC blocks when the device supports them; otherwise it expands
to BGRA8 without resizing, generating mips or recompressing to another lossy format.
Private GPU textures use one shared staging buffer per upload and ordered commands.
The native Metal primitive accepts Gx blend/filter/wrap enums, supports all 12
blend equations (including alpha), independent alpha testing, the Gx channel-write
mask, blend constants, six filter modes and independent U/V wrap/clamp. Pipeline
and sampler objects are cached; alpha testing uses an MSL function specialization.
A partial `CGxDeviceMetal` now connects the shared game device interface, Apple
factory, UI shader creation/binding, Gx buffer pools, texture callbacks, render
states and frame submission. Mac/iOS previews use this device. RGB565, ARGB1555
and ARGB4444 use native packed UNORM textures on Apple devices, including BGRA8
source conversion with the original D3D9 channel truncation. Font-cache ARGB4444
preserves all four-bit values, with only a bit rotation for Metal's channel order.
The simulator rejects packed formats even on an Apple GPU; there, UNORM4 expands
exactly to BGRA8; UNORM5/6 stores its integer codes in filterable BGRA8 and uses
per-texture shader normalization. This avoids rounding to an approximate 8-bit
color before filtering. A function constant removes normalization from native
packed/ordinary draws. Upload memory budgets include the fallback representation. Packed format,
conversion, linear sampling, partial-update and Services mip-reload checks run
through GPU readback. The original Gxu/FreeType font layout and rasterizer draw
the preview text.
The font probe reads ARIALN from the installation, covers multiline/color,
AA/monochrome, outline/shadow and drawable-size changes, and checks identical
pixels after restoring the size or recreating the font cache. Static/dynamic pool
uploads precede their consuming draws in the same command stream. Transient geometry
uses append-only, write-combined shared pages owned by each of three in-flight
frames. Completion of that specific frame gates page reuse; logical pool wrapping
cannot overwrite earlier draws. Unchanged streaming ranges reuse their snapshot
within the frame, and unused pages are released after a smaller frame. Texture updates replace immutable
GPU images, including partial rectangles and callbacks whose data starts at the
updated region. Original Gx base-mip/filter selection is retained (the standalone
BLP probe separately checks every authored mip). Frames are limited to three in
flight and drained on suspension. Native hosts still own window/view creation;
full client startup and input/event integration remain pending.
The native fixed-function unlit path supports zero to four contiguous texture
stages, all six original Gx RGB/alpha operations independently, per-stage
saturation, UV set selection, affine/projective texture transforms, vertex or
material color, linear vertex fog and independent alpha testing. It uses typed
Metal constants and function specialization. Polygon offset keeps the original
normalized depth adjustment after clipping and viewport mapping, using a separate
fragment-depth variant only when enabled. One hundred thirteen numerical draws cover stage
order, disabled cascades, all RGB/alpha pairs, transforms, fog interpolation,
alpha boundaries, signed offsets, depth write/compare, clamping, clip boundaries
and consecutive state restoration. `WOW_METAL_CHECK=fixed` runs these checks and
the existing water-ripple regression. Linear generated coordinates preserve the
original object/world/camera sources, transform flags and Gx state update timing;
324 independent pixel references cover transforms, projection, missing authored
UVs and changing cameras. These checks pass on Mac with ASan/UBSan and Metal
validation and on the iOS simulator. SM3 terrain/model/world shaders
keep their own fog even when the retained Gx fog-enable state is set, as required
by the original profile. Lighting, normal/reflection fixed-function coordinates and
mixed programmable/fixed-function pairs still fail explicitly.

The native unlit geometry primitive now links the shared Gx vertex-layout tables
and consumes packed vertex buffers and 16-bit indexed/nonindexed batches directly.
It maps Tempest matrices, BGRA vertex colors, Gx depth comparisons/write control,
culling and scissor rectangles. The preview uses immutable PCT vertices and an
index buffer. This path currently uses BGRA8 color and Depth32Float attachments;
it does not implement lighting, fog, skinning, triangle fans, MSAA or arbitrary
game shaders, and is not a D3D9 depth-precision or rasterization-equivalence result.
The first native programmable family is the original UI shader: texture times
vertex color, four transform constant registers, and both regular and screen-at-depth
vertex permutations. Preview startup reads the mounted `vs_3_0/UI.bls` and
`ps_3_0/UI.bls`; bounded BLS parsing and recognized code fingerprints prevent a
modified shader from silently selecting the wrong native implementation. The
original 2.0 family is recognized too. This is shader identification, not the
client's signature policy. The game shader API accepts verified UI, FFX filter and glow
families and reports unsupported effects explicitly. It retains the original
profile lookup order for absent assets and rejects unknown code in present assets.
The device supports synchronous BGRA pixel and Depth32Float depth readback,
clipped top-left rectangles, continuation of drawing after a readback, and the
original deferred screenshot interface. Only explicit readback waits for GPU
completion. Shared UI immediate geometry now reaches the device, including
`BufData` writes to streaming index buffers. The reconstructed D3D9 pixel reader
ignores nonzero rectangle origins and its depth reader fills zero; native Metal
returns the requested rectangle/actual depth. Current game pixel callers use
origin zero, and no game depth-reader call sites were found. The discrepancy still
needs original Windows verification. BGRA8 color render targets now support
offscreen drawing/sampling, switching back to the current default surface, and
reusing a scratch Depth32Float surface across smaller targets. Binding and texture
contents survive frame boundaries; host resizing and bound-target destruction
restore the correct default surface. BGRA8 surface copies preserve partial rectangles
and target mips; framebuffer copy/stretch reads the default color surface even
while an offscreen target is bound. Exact copies use GPU blits, while nearest/linear
scaling uses native sampling on the same command stream without a CPU round trip.
Color/depth clears honor viewport bounds and the last applied scissor state,
including the original offscreen Y convention. Full-surface clears use attachment
load actions; regional clears preserve surrounding pixels and the unused portion
of a larger shared depth surface. Other render-target formats, explicit depth textures, gamma ramps and stereo remain
explicit unsupported operations. Geometry draws
share an encoder while attachments and resource ordering permit. Buffer uploads,
copies, synchronous readback and target switches end the pass; full clears can be
consumed by the next draw. A focused check encodes 130 geometry draws and a regional
clear in one pass, with exact pixel output after viewport/scissor changes. Actual
particle/ribbon draws use the shared pages without interrupting the pass. The
original Rejuvenation's eight draws use one render pass and no buffer-upload pass
(previously eight and thirteen respectively). Nine queued frames exercise wrap and
all three reusable slots, preserve exact pixels and allocate no new pages after
warmup. These counts demonstrate reduced encoding work; sustained frame timing,
private-pool upload allocation reuse and texture-update bandwidth still need work.

Changing the base mip invalidates resident resources and delivers the original
producer and texture-cache recreation notifications. Reuploads select the authored
mips; earlier draws and the currently bound attachment retain their GPU images.
The probe also keeps original Gxu font objects alive across mip changes and checks
that their glyph textures reproduce the same pixels after reupload.

Native primitives for the original `FFXBox4` and `FFXGauss4` assets now preserve
four independent coordinates, textures and samplers. Box4 averages RGB with
vertex alpha; Gauss4 filters RGBA using the original tap weights. GPU checks cover
both effects and two-pass intermediate sampling. The original `PassBox4` and
`PassGauss4` now execute through Services textures and the Gx shader factory,
using their custom PCT4 stream and fixed vertex matrices. Metal quad coordinates
preserve D3D9 sampling at integer screen centers, including fractional downsample
ratios. The pass checks cover padded targets, intermediate pixels, shader caching
and restoration of caller matrices/state. Original `PassGlow` now combines scene
and blur textures through the game PCT2 layout, retaining its blue-channel mix,
squared blur weighted by vertex alpha, and opaque output alpha. The probe executes
Box4, both Gauss4 passes and Glow without an intermediate CPU wait, then checks
composition parameters, saturation and texture-coordinate scaling. `PassGlowWave`
also runs its original procedural wave texture and time-dependent PCT3 vertices,
with independent scene/blur displacement scales from pixel constants. Its GPU
output is checked against the actual generated RG data and submitted coordinates.
The reachable RG-in-BGRA8 branch is verified; UV88 texture support remains pending.
The original `FFX::Init/Start/Finish` and `EffectGlow` now run through shared
CVar registration and command dispatch, including default-framebuffer capture,
both effect modes, enable/disable and target reallocation after resize. GPU
checks exercise the original effect object across these transitions. Shared CVar
configuration supports an explicit writable absolute path, bounded loading and
retryable saves. Native console output goes to startup diagnostics and, while
the original console screen is alive, its game-thread display queue.
`WOW_METAL_CHECK=ui` runs only the shared UI/console input and MovieFrame GPU/audio checks;
its result is explicitly labelled as a focused run. The native UI/model hosts
now use the original ConsoleDevice initialization and destruction, hardware DBC
selection, display CVars and `gxRestart`. Native M1/A14 minimum hardware selects
the original upper CPU tier; the monotonic timer is not interpreted as CPU MHz.
The host supplies windowed BGRA8/Depth32Float defaults without modifying the DBC
or claiming support for its legacy D24 display format. Saved unsupported modes
fail explicitly. Stereo preferences are retained while stereo output is unavailable.
At an idle frame boundary, Metal `DeviceSetFormat` applies windowed resolution
and vsync requests to the actual macOS view. AppKit point rounding is published
as the active pixel size rather than stretching a differently sized drawable.
On iOS it applies a fixed rendering resolution inside the UIKit scene. Hosts
must acquire the next drawable after a size change; an already acquired
MetalKit color drawable retains its old dimensions until the current view draw
callback returns. Active Gx frames and non-main-thread native requests fail
without changing the format. Fullscreen/maximized windows, physical display mode
changes and disabling iOS presentation synchronization remain unsupported.
Native hosts now bind their view before the shared Gx factory runs, and the first
format request updates the real window/drawable. Failed host creation preserves
the previous binding and active format; a callback-based startup without a host
cannot silently create a headless display. Headless component probes remain
available when no callback or native view is supplied.
`WOW_DISPLAY_CHECK=1` checks real drawable/depth reallocation, GPU readback,
native pixel/input sizes, unsupported requests and continued rendering across
successive view callbacks in either native preview.
It also exercises the original restart path, including last-good rollback and
two requests in one active frame. The complete restart (world fade/portrait
invalidation, format application, CVars and atlas notification) is deferred until
submission; the latest request takes effect before the next drawable. Rounded
window sizes are reflected in both the CVars and last-good format. Metal portrait
alpha support is known from its BGRA8 attachment contract, so restart does not
clear/read the framebuffer outside a host frame. Full Glue/player startup and
remaining shader families are still pending.
The original shared system Lua registration is also linked into Glue and the
component host: `GetTime`, `GetGameTime`, `ConsoleExec`, the three original
file-access-denied functions and `GetAccountExpansionLevel`. Their implementations
are shared unchanged with Windows. Account expansion still comes from the real
ClientServices connection; the component viewer does not establish a login.
The shared in-world `Ui` module now compiles as an arm64 static library on all
three Apple targets. The optional Windows Logitech LCD backend is excluded.
The original `Object` and shared `Game` modules also compile on all three,
including player/unit objects, movement, world text and game callbacks. The
unfinished Win64 Warden platform source is excluded from Apple. Guild money
messages use a portable 64-bit hexadecimal scan with the original 16-digit
width; malformed input is rejected before changing the stored balance.
Original `UIUtil`, `DB`, loading screens and `AppleClientCore` now compile on all
three targets as well. This includes the original client startup/game transition,
client commands/CVars, camera/input control, tooltips and full DBC registration.
Achievement tooltip links retain their 64-bit GUID through a portable scan.
The original `Magic`, `WoWCommon`, `ObjectMgrClient` and `Minigame` libraries
also compile for all three targets, retaining spell logic/visual dispatch and
object-create/mirror messages. A combined Glue/client link still has missing
platform, Battle.net, external LCD and verification/scan dependencies.
The original Battle.net shared protocol, callback/argument layer, module manager
and login class now compile for Apple. Module loading uses native `.dylib` images
and named exports with balanced ownership; Windows ordinal exports fail explicitly.
Mac sanitizer and simulator checks exercise exact BSN bit/address bytes, native
protocol field/array/UTF-8/optional adapters and 64-bit arguments/reference counts.
The external Battle.net authentication component is still unavailable; this does
not establish Battle.net login or replace its security checks.
Its native IPv4 resolver now preserves the original single-address callback and
network byte order, with numeric address:port parsing before DNS (DNS proxies can
otherwise synthesize an address for this invalid hostname form). Localhost,
numeric addresses and failure clearing run on Mac and iOS Simulator. The shared
Blizzard file-version query uses the bounded PE/native bundle metadata reader;
PE32/PE32+ and the original client components are checked without executing them.
The separate `Blizzard::URL::DownloadURL` interface now uses native URLSession
with the original asynchronous callback contract, GET/HEAD/POST, inclusive byte
ranges, conditional requests, cookies, metadata and error statuses. A serial
worker queue delivers bounded data chunks and one completion; cancellation and
shutdown can drain callbacks before their owners are released. TLS validation
uses the system defaults. Loopback checks cover exact request/body bytes, 64-bit
range/length metadata, redirects, failures, truncation, timeout and restart. This
does not enable the unfinished streaming filesystem or complete patch handling.
The original `HttpDownload` consumer now runs on this transport with synchronized
state and an owner-held callback that is drained before destruction. Failed,
cancelled and truncated responses cannot be exposed as successful patch bytes;
the Battle.net patch caller checks success before writing any output. Buffer
growth follows received data, with an explicit rejection of responses exceeding
the consumer's existing signed 32-bit storage limit. Mac sanitizer and simulator
checks exercise fixed/chunked bodies, live progress, errors and active teardown.
Native Storm errors now write flushed, private text reports beneath
`WOW_LOG_ROOT/Errors` (the normal writable log root when unset), using the original
title/header callbacks, handler order and prepared fatal source position. The
last report is available before handlers run, recursive reporting is bounded,
and shutdown clears registered callbacks. Mac checks use actual child processes
to verify fatal reports and exit codes; iOS exercises recoverable errors and
concurrent handlers. OS-fault interception, minidumps and modal error UI remain
unimplemented.
This compile coverage includes the original game UI and scripts; it does not
establish full client startup or runtime coverage of those interfaces.
The original `Wow.ini` profile reader now runs through SFile on Apple, retaining
case-insensitive section/key lookup, repeated and quoted multi-values, original
whitespace and EOF behavior. Native path trimming handles UTF-8 and empty paths;
zero-size outputs fail without writing, and profile files are limited to 4 MiB
before allocation. Mac sanitizer and simulator checks cover locale fields,
FourCC values, failed-read retry/append and repeated create/destroy lifetimes.
The native scene host now calls the original Base global/context startup and
shutdown. Event and CVar resources are destroyed before Base releases its
properties, file-cache tables and string pool. Two visible Mac host lifetimes
each render 120 frames with sanitizer/Metal validation; iOS foreground rebuilds
also re-enter the original lifecycle. The existing prefetch implementation is
unchanged and this does not add the unfinished asynchronous prefetch behavior.
Storm now encloses that native host lifetime using the original initialization
and region, command, file and string cleanup. Native diagnostics and open logs
are closed immediately at shutdown so UIKit can rebuild in the same process.
Three core lifetimes verify owned-region cleanup, command strings and flushed
logs; two visible Mac lifetimes and iOS foreground recreation each render 120
frames with sanitizer/Metal validation where available. Windows retains its
original CRT exit handling and window-message registry.
The original GUI window queries/title calls now use weak native view ownership;
device shutdown clears the binding while the host view may still be alive.
Mac prompts use actual AppKit alerts with the original OK/Yes/No/Cancel results.
`AppleUIProbe --gui-check` exercises nine button choices, UTF-8 titles, invalid
inputs and thread/lifetime boundaries. `WOW_DISPLAY_CHECK=1` also checks this
binding during live Mac/iOS drawable resizing. UIKit synchronous prompts return
the original failure/cancel result with `ENOTSUP`: asynchronous startup prompt
continuations remain unimplemented and are never treated as user acceptance.
The original `PatchFiles` enumerator and quicksort now compile on Apple. Patch
directories follow the configured SFile base/Data paths instead of process CWD,
resolve locale directory casing and preserve full combined filenames. Both
original enumeration modes retain their passes and ordering. Mac sanitizer and
iOS checks cover long paths, case/locale and parent-directory patterns; all
seven real patch archives match the existing mount order and priorities. This
does not replace full client startup or add trial/remote patch downloads.
Voice communication and microphone recording are intentionally excluded from
Apple targets. The original UI reports voice chat unavailable, attempts to enable
voice/microphone CVars are rejected, and startup creates no ComSat interface or
worker. Earlier isolated ComSat network/codec sources remain in the repository,
but are no longer built, linked or exercised by the Apple apps and core probe.
Windows voice behavior is unchanged. Game sound, music and movie playback remain
in scope; Apple native frameworks may decode audio and pure video without
reproducing the original decoder internals or bit-identical samples. Resource
compatibility, timing, volume, looping, A/V sync and lifecycle remain required.

Native Metal counting visibility queries now serve the original sun/moon glare
consumer. Overlapping brackets and pass/frame splits accumulate sample counts;
clear/copy implementation draws are excluded. Availability polls never submit or
wait, while explicit result reads may flush and wait as in the original Gx API.
Shared result pages remain owned until both their query and GPU work finish and
then recycle. `WOW_METAL_CHECK=queries` checks 48 counts/lifetimes, including the
original planet geometry and glare fractions at full, half and zero visibility,
on Mac with ASan/UBSan and Metal validation. The real client also enters the local
world and exits with this capability enabled. Simulator does not support counting
queries and retains the original point-occlusion fallback; its capability check
and subsequent 120-frame preview pass. The iOS device target builds; physical
iPhone/iPad execution and original Windows visual comparison remain pending.

The Metal probe also checks the actual Gx factory, shader refcounts, streaming wrap,
texture replacement, partial updates, immediate geometry, mid-frame pixel/depth
readback and screenshots through game APIs. It verifies known alpha
cases, filtering and blending, then draws and
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
scenes stop the native Event frame callback and drain submitted work. Full reports remain in the view and bounded status messages go to unified
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
