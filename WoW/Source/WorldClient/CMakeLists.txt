file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(WorldClient STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(WorldClient
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(WorldClient
    PRIVATE
        Extension
)
