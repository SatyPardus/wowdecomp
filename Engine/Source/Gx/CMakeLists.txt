file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Mac/")
else ()
    # Direct3D devices and the Win32 adapter queries are Windows-only. The OpenGL device is shared; its
    # window/context code is selected per platform (WGL on Windows, SDL2 on macOS).
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/CGxDevice(D3d|D3d9Ex)/")
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
endif ()

add_library(Gx STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Gx
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

if (WIN32)
    target_link_libraries(Gx
        PRIVATE
            nvapi
            opengl32.lib
            d3d9.lib
    )
elseif (APPLE)
    target_compile_definitions(Gx PRIVATE GL_SILENCE_DEPRECATION)
    target_link_libraries(Gx PUBLIC SDL2::SDL2 OpenGL::GL)
endif ()
