file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Mac/")
endif ()

add_library(Glue STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Glue
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)
