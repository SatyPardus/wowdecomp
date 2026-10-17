# Linux cross-compile

Builds the Windows x64 client (`WowClient64.exe`) and `XvidDecoder.dll` on Linux with clang-cl, lld-link, and the
Microsoft CRT and SDK from `xwin`.

- **Tested on upstream commit:** `d36a92c08630c3dd219528337e4c0ef9c0d5745b`

## Pins

- `xwin` 0.10.0, manifest version `17`
- CRT `14.44.17.14`, SDK `10.0.26100`
- Sysroot: `~/.local/share/xwin`, made with `xwin splat --use-winsysroot-style --preserve-ms-arch-notation`

## Build command

```sh
cmake -S . -B build/linux-x64 -G Ninja --toolchain cmake/clang-cl-xwin.cmake \
    -DCMAKE_BUILD_TYPE=Release -DWOWDECOMP_DISABLE_SIGNATURE_CHECKS=ON
```

Override the sysroot with `-DXWIN_DIR=<path>`.

## Toolchain file (`cmake/clang-cl-xwin.cmake`)

- Upstream's `cmake_minimum_required(VERSION 3.10)` leaves policy CMP0091 at OLD, and it runs before the toolchain file is
  read, so `CMAKE_MSVC_RUNTIME_LIBRARY` is ignored and CMake adds `/MD`. The toolchain file therefore forces the Release
  flags to `/MT /O2 /Ob2 /DNDEBUG /Z7` (static runtime, CodeView debug info) and the Release link flags to `/DEBUG`.
  A target that needs the DLL runtime must add `/MD` to its own compile options.

## Runtime exception: `XvidDecoder` uses `/MD`

The prebuilt `ThirdParty/xvidcore/lib/x64/libxvidcore.lib` names `/DEFAULTLIB:"MSVCRT"`, so `XvidDecoder` keeps the DLL
runtime while everything else uses `/MT`. `cmake/xvid-runtime.cmake` (set as `CMAKE_PROJECT_INCLUDE` by the toolchain
file) defers a `set_property(TARGET XvidDecoder APPEND PROPERTY COMPILE_OPTIONS /MD)` until the root directory is
configured, so no upstream file changes. `XvidDecoder.dll` imports `VCRUNTIME140.dll` and `api-ms-win-crt-*`, which
Proton provides. Note that `Frame` links `DivxDecoder`, which builds `XvidDecoder.dll`, so this is needed for any build
that includes `Frame`.

## GL header shim

The source has `#include <GL/gl.h>`, but the xwin SDK only has `um/gl/GL.h`. `cmake/include/GL/gl.h` forwards to
`<gl/GL.h>`, and `cmake/include` is on the system include path (`-imsvc`) in the toolchain file.

## Build script (`tools/linux/build.sh`)

```sh
tools/linux/build.sh [--clean] [--configure-only] [--signature-checks] [target...]
```

- Checks that `clang-cl`, `lld-link`, `llvm-rc`, `cmake`, and `ninja` are on `PATH`, and that `XWIN_DIR` (default
  `~/.local/share/xwin`, override with the `XWIN_DIR` environment variable) has the pinned CRT and SDK folders. It exits
  non-zero and names each missing tool or folder.
- Configures `build/linux-x64` with the toolchain file and `WOWDECOMP_DISABLE_SIGNATURE_CHECKS=ON`.
  `--signature-checks` turns the checks back on.
- `--clean` deletes `build/linux-x64` first. `--configure-only` stops after configuring.
- With no targets it builds `WowClient` and `XvidDecoder`; otherwise only the named targets.

## Check script (`tools/linux/check.sh`)

```sh
tools/linux/check.sh [--no-build]
```

- Runs `tools/linux/build.sh --clean`, then prints one `PASS` or `FAIL` line per check and exits non-zero if any failed:
  `WowClient64.exe` is PE32+ GUI x86-64, it has no `vcruntime140`/`msvcp140`/`ucrtbase`/`api-ms-win-crt-` imports,
  `WowClient64.pdb` exists, `XvidDecoder.dll` is a PE32+ x86-64 DLL, and `WowClient64.exe` was built with signature
  checks off (it contains the `signature check OFF` string that `BugReport.cpp` compiles in only under
  `DISABLE_SIGNATURE_CHECKS`), and no section of `WowClient64.exe` has characteristics `0x0`.
- `--no-build` skips the clean build and runs only the checks on the existing `bin/Win` files.

## Install script (`tools/linux/install.sh`)

```sh
tools/linux/install.sh <client dir>
```

- Copies `WowClient64.exe`, `WowClient64.pdb`, and `XvidDecoder.dll` from `bin/Win` into `<client dir>`.
- Exits non-zero without copying when `<client dir>` has no `wow.exe` (any case), or when any of the three files is not built.
- Writes only those three names (overwriting earlier copies). `wow.exe` and every other file are untouched.
- **Back up first:** before the first install, copy `WTF/` and `Interface/AddOns` out of the client folder.
  To roll back, delete the three installed files and restore those two folders from the backup.

## Source files changed

| File | Error | Fix |
| --- | --- | --- |
| `WoW/Source/Battlenet/BSN/BSNEncodeInterface.cpp` | `reinterpret_cast from 'const void *' to 'AdapterGet…' casts away qualifiers` (30 sites; a hard error in clang, no warning flag) | Added a local `BSN_ADAPTER_CAST(type, ptr)` macro. Under `__clang__` it adds `const_cast<void*>`; otherwise it expands to the original `reinterpret_cast`, so the MSVC path is unchanged. |
| `WoW/Source/Ui/MinimapFrame.cpp` | Link: `duplicate symbol: int s_mapId` (with `WorldClient.lib(Map.cpp.obj)`). `Map.h` declares `extern int32_t s_mapId`; clang gives the later `static int32_t s_mapId` external linkage, MSVC keeps it file-local. | Under `__clang__`, `#define s_mapId s_mapId_MinimapFrame` before the static, so it stays file-local. |

Every other file compiles under clang-cl with no source edits (warnings only), given the flags below. `nasm` is not
needed: its step is Win32-only and does not run for x64.

## Added flags

In `cmake/clang-cl-xwin.cmake`, besides `/winsysroot`, `/MT`, and `/Z7`:

| Flag | Reason |
| --- | --- |
| `-Wno-register` | The bundled `jpeg` C sources use `register`, which clang rejects as an error when compiled as C++17 (`-Wregister`). |
| `-Wno-c++11-narrowing` | `Engine/Source/Event/Types.h` has `enum : uint32_t` values of `0xFFFFFFFFFFFFFFFF`. MSVC truncates to `0xFFFFFFFF`; clang errors. The flag keeps the MSVC value. |

These are `_INIT` flags, so they only reach the cache on a fresh configure. Use `--clean` after changing them.

`WowClient` links to `bin/Win/WowClient64.exe` (PE32+ GUI x86-64) and `.pdb`, with no CRT DLL imports.

## Linker flag: `/MERGE:memcpy_=.rdata`

The static CRT's memcpy objects (CRT `14.44.17.14`) contain COMDAT sections `memcpy_$fo_bdd$` and `memcpy_$fo_rvas$`
with no memory flags (characteristics `0x1000`, COMDAT only). lld-link groups them into an output section `memcpy_` with
characteristics `0x0`, which Windows maps without read access. The client's executable checksum
(`Engine/Source/Os/Win64/OsVersionHash.cpp`, `ComputeChecksum`) hashes every non-writable section, so it faulted on that
page; the `__try` in `IOsVersionChecksum` caught it and the crash logger recorded it. The toolchain file merges the
section into `.rdata`, and `check.sh` fails if any section has characteristics `0x0`.

