file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(Model2 STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Model2
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Model2
    PRIVATE
        Extension
)
