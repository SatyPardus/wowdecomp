# The same Xvid codec used by the Win64 adapter, built from a pinned upstream
# source release for every Apple SDK. No installed/Homebrew codec is required.
include(FetchContent)
FetchContent_Declare(apple_xvid
    URL https://downloads.xvid.com/downloads/xvidcore-1.3.7.tar.gz
    URL_HASH SHA256=abbdcbd39555691dd1c9b4d08f0a031376a3b211652c0d8b3b8aa9be1303ce2d
)
FetchContent_MakeAvailable(apple_xvid)
# Use upstream's portable source list, including its unmodified codec math.
file(READ "${apple_xvid_SOURCE_DIR}/build/generic/sources.inc" xvid_sources)
string(REGEX MATCH "SRC_GENERIC =[^=]+SRC_IA32" xvid_sources "${xvid_sources}")
string(REGEX MATCHALL "[a-zA-Z0-9_/]+\\.c" xvid_sources "${xvid_sources}")
list(TRANSFORM xvid_sources PREPEND "${apple_xvid_SOURCE_DIR}/src/")
add_library(AppleXvid STATIC ${xvid_sources})
target_include_directories(AppleXvid PUBLIC "${apple_xvid_SOURCE_DIR}/src")
target_compile_definitions(AppleXvid PRIVATE ARCH_IS_GENERIC ARCH_IS_64BIT ARCH_IS_LITTLE_ENDIAN)
target_compile_options(AppleXvid PRIVATE -fwrapv)
set_target_properties(AppleXvid PROPERTIES C_STANDARD 99)

add_library(AppleMovie STATIC
    Engine/Source/Media/Apple/XvidDecoder.cpp
    Engine/Source/Media/Apple/MovieAVI.cpp
)
target_link_libraries(AppleMovie PUBLIC AppleArchives PRIVATE AppleXvid)
target_link_libraries(DivxDecoder INTERFACE AppleMovie)
