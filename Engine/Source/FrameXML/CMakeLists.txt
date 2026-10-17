file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(FrameXML STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(FrameXML
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(FrameXML
    PUBLIC
        expat
        Extension
)
