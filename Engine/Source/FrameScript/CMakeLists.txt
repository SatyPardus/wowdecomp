file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(FrameScript STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(FrameScript
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(FrameScript
    PRIVATE
        Extension
)
