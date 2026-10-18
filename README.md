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

The native port currently builds **foundation, graphics and audio probes**, not a playable client. They
link the existing Tempest mathematics, CDataStore, modified Lua 5.1 (including its
pool allocator and profiler), MPQ cryptography, and IOUnit chains to native Apple
memory, synchronization, pthread threads/TLS, timing and the original Blizzard file
API backed by native positional file IO. No Windows emulation is
used. Original SFile archive/resource IO, decompression and the game's SFile2
open/read/seek/preload layer are linked. The native
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

The shared Event scheduler now has a nonblocking native-host step. Mac/iOS
previews render continuously from the original paint event, preserving update,
timer, input, poll, queued-message, idle and paint ordering. Focus/resize events
come from the host; native keyboard, pointer and composed-text delivery remain
to be connected. Foreground resume resets frame elapsed time while retaining
wall-clock timers. Context shutdown and recreation release property TLS, call
contexts, queues and timers. Native stepping currently requires the client's
single scheduler thread; server network pumping is explicitly unsupported.

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
Original SM3 bytecode fingerprints guard supported variants; selecting a shadow
variant fails explicitly. GPU checks cover those variants, both real wolf material
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
Other model materials, colored skinning, shadows and full world rendering remain
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

The macOS preview and iOS Metal view run that original M2 scene beside the texture
and font preview. Model/skin/texture loading is asynchronous; the game Event update
drives animation time. The default is CreatureDisplayInfo 165 (timber wolf), playing
its original walk animation. Set `WOW_CREATURE_DISPLAY` to another positive DBC ID,
or `WOW_MODEL` to an explicit archive path without creature appearance replacement.
These options are mutually exclusive. Character composition is not integrated;
an unsupported shader reports its name and stops rendering.
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
currently stops explicitly at the unimplemented `Diffuse_Proj_Proj` effect.
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
supplies explicit camera/light/fog inputs. Full world placement, portal traversal,
doodads and WMO shadows remain to be connected.

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
The native Terrain/Terrain1 path now supports 32 vertex and 16 pixel variants:
one to four layers plus their independent blend/shadow map, PN/PNC input,
three point lights, specular, vertex fog and per-layer lighting masks. Old-alpha
interpolation and big-alpha weighted sums retain their distinct semantics.
256 GPU draws compare independent position/UV/light/fog/material references,
including packed and RGBA blend maps. macOS ASan/UBSan and Metal validation pass;
both iOS targets build and the simulator passes the checks and presents 120 frames.
The native host can now draw a complete authored ADT surface with
`WOW_TERRAIN=Azeroth:32:48` (Elwynn) or `WOW_TERRAIN=Northrend:32:32`.
This selection is mutually exclusive with the model/creature variables. It uses
the original Services texture lifecycle and specular `_s.blp` selection, all
material layers, WDT-selected vertex colors, hole indices and packed/RGBA blend
maps. UV animation and per-layer lighting flags are preserved; cube-reflection
materials fail explicitly. The preview supplies an orbiting camera and explicit
lighting/fog; automatic World DayNight and WMO/doodad placement are not integrated.
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
world loading. Terrain0/2/3, projected shadows, cube reflection and
fixed-function terrain paths remain unsupported.

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
of a larger shared depth surface. Other render-target formats, explicit depth textures, queries, gamma ramps and stereo remain
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
retryable saves; native console output currently goes to startup diagnostics.
Interactive console UI, persistent host configuration and other shader families
remain to be connected.

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
