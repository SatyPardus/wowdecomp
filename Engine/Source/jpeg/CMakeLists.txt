file(GLOB PRIVATE_SOURCES CONFIGURE_DEPENDS "*.c" "*.cpp")

set_source_files_properties(${PRIVATE_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(jpeg STATIC
    ${PRIVATE_SOURCES}
)

set_target_properties(jpeg PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(jpeg
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
