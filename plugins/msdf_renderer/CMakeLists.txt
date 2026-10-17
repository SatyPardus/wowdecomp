cmake_minimum_required(VERSION 3.10)
project(msdf_renderer CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB MSDFGEN_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/ThirdParty/msdfgen/core/*.cpp
)

file(GLOB MSDFGEN_EXT_SOURCES
    ${CMAKE_SOURCE_DIR}/ThirdParty/msdfgen/ext/*.cpp
)

add_library(msdf_renderer SHARED
    MsdfRenderer.cpp
    ${MSDFGEN_CORE_SOURCES}
    ${MSDFGEN_EXT_SOURCES}
)

target_include_directories(msdf_renderer
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/ThirdParty
        ${CMAKE_SOURCE_DIR}/ThirdParty/msdfgen
        ${CMAKE_SOURCE_DIR}/ThirdParty/freetype/include
)

target_compile_definitions(msdf_renderer
    PRIVATE
        MSDFGEN_PUBLIC=
        MSDFGEN_DISABLE_VARIABLE_FONTS=1
        MSDFGEN_DISABLE_SVG=1
        MSDFGEN_DISABLE_PNG=1
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

target_link_libraries(msdf_renderer
    PRIVATE
        freetype
)

set_target_properties(msdf_renderer PROPERTIES
    OUTPUT_NAME msdf_renderer
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Win/plugins
)

foreach (config ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${config} config)
    set_target_properties(msdf_renderer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_SOURCE_DIR}/bin/Win/plugins
    )
endforeach ()
