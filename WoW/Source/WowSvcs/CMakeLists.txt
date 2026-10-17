file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(WowSvcs STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(WowSvcs
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(WowSvcs
    PRIVATE
        Extension
)
