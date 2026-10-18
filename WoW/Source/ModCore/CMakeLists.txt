file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(ModCore STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(ModCore
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ModCore
    PUBLIC
        lua
        Storm
        Base
        WowSvcs
)
