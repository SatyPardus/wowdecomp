file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(DB STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(DB
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(DB
    PUBLIC
        Extension
)
