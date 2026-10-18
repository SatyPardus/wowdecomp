file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")
if (APPLE)
    # The reconstructed Win64 Warden implementation does not implement its
    # checks. Do not select that platform path just because arm64 is 64-bit.
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
endif ()

add_library(Game STATIC
    ${PRIVATE_SOURCES}
)

if (APPLE)
    # Shared callback destructors only; Apple intentionally excludes voice chat,
    # its network/codec libraries and microphone capture.
    target_sources(Game PRIVATE ${CMAKE_SOURCE_DIR}/Engine/Source/ComSat/ComSatCallbacks.cpp)
endif ()

target_include_directories(Game
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Game
    PUBLIC
        pcreposix
        freetype
)
