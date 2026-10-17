file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(Scrn STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Scrn
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Scrn
    PRIVATE
        freetype
        Extension
)
