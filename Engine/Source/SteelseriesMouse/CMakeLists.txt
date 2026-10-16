file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Mac/")
endif ()

add_library(SteelseriesMouse STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(SteelseriesMouse
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

if (WIN32)
    target_link_libraries(SteelseriesMouse
        PUBLIC
            hid
            setupapi
    )
elseif (APPLE)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(SteelseriesMouse
        PUBLIC
            ${IOKIT_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
    )
endif ()
