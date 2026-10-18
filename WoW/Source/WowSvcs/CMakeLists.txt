file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")
if (APPLE)
    # Native session services own these shared implementations.
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/AccountData(_CVar)?\\.cpp$")
endif ()

add_library(WowSvcs STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(WowSvcs
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)
