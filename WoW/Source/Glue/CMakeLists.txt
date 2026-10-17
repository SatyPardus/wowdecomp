file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(Glue STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Glue
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Glue
    PRIVATE
        Extension
)
