file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Win32/Win64 network sources use Winsock; other platforms use the BSD socket sources in Posix/.
if (WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/ComSat/Posix/")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/ComSat/Win(32|64)/")
endif ()

add_library(ComSat STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(ComSat
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)
