# Toolchain file: cross-compile the Windows x64 client on Linux with clang-cl, lld-link, and the
# Microsoft CRT and SDK laid out by `xwin splat --use-winsysroot-style`.
# Override the sysroot with -DXWIN_DIR=<path>.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if (NOT DEFINED XWIN_DIR)
    set(XWIN_DIR "$ENV{HOME}/.local/share/xwin")
endif ()

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)

# Upstream's cmake_minimum_required(3.10) leaves CMP0091 at OLD, and that call runs before this file is read, so
# CMAKE_MSVC_RUNTIME_LIBRARY is ignored and CMake adds /MD itself. Set the static runtime in the Release flags instead.
# (A per-target runtime exception must therefore add /MD to its own compile options.)
# Try-compile projects must not link an executable (the sysroot is only seen at the real link step).
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -Wno-register: old C code (jpeg) uses `register`, an error in C++17.
# -Wno-c++11-narrowing: Event/Types.h has `enum : uint32_t` values of 0xFFFFFFFFFFFFFFFF (MSVC truncates to 0xFFFFFFFF).
# The source includes <GL/gl.h>, but the SDK only has um/gl/GL.h; cmake/include/GL/gl.h forwards to it.
set(WOWDECOMP_SHIM_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/include")

set(CMAKE_C_FLAGS_INIT "/winsysroot ${XWIN_DIR} -imsvc ${WOWDECOMP_SHIM_INCLUDE} -Wno-register -Wno-c++11-narrowing")
set(CMAKE_CXX_FLAGS_INIT "/winsysroot ${XWIN_DIR} -imsvc ${WOWDECOMP_SHIM_INCLUDE} -Wno-register -Wno-c++11-narrowing")
# /MERGE:memcpy_=.rdata: the static CRT's memcpy objects (CRT 14.44) carry COMDAT sections memcpy_$fo_bdd$ and
# memcpy_$fo_rvas$ with no memory flags. lld-link turns them into an output section `memcpy_` with characteristics 0,
# which Windows maps unreadable, and the client's exe checksum (OsVersionHash.cpp ComputeChecksum) faults reading it.
set(CMAKE_EXE_LINKER_FLAGS_INIT "/winsysroot:${XWIN_DIR} /MERGE:memcpy_=.rdata")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "/winsysroot:${XWIN_DIR} /MERGE:memcpy_=.rdata")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "/winsysroot:${XWIN_DIR}")

# Release builds: static runtime, CodeView debug info in the objects, and a PDB from the linker.
set(CMAKE_C_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG /Z7" CACHE STRING "Release C flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG /Z7" CACHE STRING "Release C++ flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG" CACHE STRING "Release exe link flags" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG" CACHE STRING "Release DLL link flags" FORCE)

# XvidDecoder needs the DLL runtime; see cmake/xvid-runtime.cmake.
set(CMAKE_PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/xvid-runtime.cmake")
