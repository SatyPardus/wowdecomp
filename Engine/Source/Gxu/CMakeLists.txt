file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(Gxu STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Gxu
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Gxu
    PUBLIC
        freetype
)
if (APPLE)
    target_link_libraries(Gxu PRIVATE msdfgen::msdfgen-core)
endif ()
