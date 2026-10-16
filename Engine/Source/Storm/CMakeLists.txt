file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Mac/")
endif ()

add_library(Storm STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Storm
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Storm
    PUBLIC
        blast
        bzip2
        zlib
)
