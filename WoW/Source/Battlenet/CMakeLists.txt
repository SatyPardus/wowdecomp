file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")
if (APPLE)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Apple/")
endif ()

add_library(Battlenet STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Battlenet
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)
