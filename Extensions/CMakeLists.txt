file(GLOB EXTENSION_CANDIDATES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")

set(EXTENSION_NAMES "")
foreach (candidate ${EXTENSION_CANDIDATES})
    if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${candidate}/src")
        list(APPEND EXTENSION_NAMES ${candidate})
    endif ()
endforeach ()
list(SORT EXTENSION_NAMES)

set(EXTENSION_SOURCES "")
set(EXTENSION_FORWARD_DECL "")
set(EXTENSION_ENTRIES "")
set(EXTENSION_COUNT 0)

message(STATUS "Extensions:")

foreach (name ${EXTENSION_NAMES})
    string(TOUPPER ${name} upper)
    option(WOWDECOMP_EXTENSION_${upper} "Compile the ${name} extension into the client" ON)

    if (NOT WOWDECOMP_EXTENSION_${upper})
        message(STATUS "  - ${name} (disabled)")
        continue()
    endif ()

    string(MAKE_C_IDENTIFIER ${name} ident)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}/src/*.cpp")
    list(APPEND EXTENSION_SOURCES ${sources})

    string(APPEND EXTENSION_FORWARD_DECL "void Add${ident}Extension();\n")
    string(APPEND EXTENSION_ENTRIES "    { \"${name}\", Add${ident}Extension },\n")
    math(EXPR EXTENSION_COUNT "${EXTENSION_COUNT} + 1")

    # An extension may ship <Name>/<Name>.cmake for its own build needs (defines, third-party code).
    include("${CMAKE_CURRENT_SOURCE_DIR}/${name}/${name}.cmake" OPTIONAL)

    message(STATUS "  + ${name}")
endforeach ()

set(EXTENSION_LOADER ${CMAKE_CURRENT_BINARY_DIR}/ExtensionsLoader.cpp)
configure_file(ExtensionsLoader.cpp.in ${EXTENSION_LOADER} @ONLY)

add_library(WowExtensions STATIC
    ${EXTENSION_LOADER}
    ${EXTENSION_SOURCES}
)

target_include_directories(WowExtensions
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(WowExtensions
    PUBLIC
        Extension
)
