file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(Net STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Net
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

if (APPLE)
    target_link_libraries(Net PUBLIC AppleGameNetwork)
else ()
    target_link_libraries(Net PUBLIC ws2_32 wsock32)
endif ()
