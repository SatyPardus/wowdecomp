file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")
if (APPLE)
    # The reconstructed Win64 Warden implementation does not implement its
    # checks. Do not select that platform path just because arm64 is 64-bit.
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
endif ()

add_library(Game STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Game
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Game
    PUBLIC
        pcreposix
        freetype
)
