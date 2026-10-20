# Keep the original Windows dependency available for binary comparisons.
# Apple uses an unmodified, pinned upstream release; no Homebrew runtime is needed.
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_ENABLE_ERROR_STRINGS ON CACHE BOOL "" FORCE)
FetchContent_Declare(apple_freetype
    URL https://download-mirror.savannah.gnu.org/releases/freetype/freetype-2.14.3.tar.xz
    URL_HASH SHA256=36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f
)
FetchContent_MakeAvailable(apple_freetype)
set(WOW_FREETYPE_LICENSE "${apple_freetype_SOURCE_DIR}/docs/FTL.TXT" CACHE INTERNAL "")
