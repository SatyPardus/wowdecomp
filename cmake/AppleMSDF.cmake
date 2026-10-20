# CPU outline generation only; Metal owns sampling, antialiasing and compositing.
# Use the game's already-loaded FreeType faces, with no additional font loader.
include(FetchContent)
set(MSDFGEN_CORE_ONLY ON CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_OPENMP OFF CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(apple_msdf
    URL https://codeload.github.com/Chlumsky/msdfgen/tar.gz/refs/tags/v1.12.1
    URL_HASH SHA256=24541c9fbb459e8c62adeb3eb22cb21fa07a4b5091e2ae1f041d144a4baf08cd
)
FetchContent_MakeAvailable(apple_msdf)
set(WOW_MSDF_LICENSE "${apple_msdf_SOURCE_DIR}/LICENSE.txt" CACHE INTERNAL "")
