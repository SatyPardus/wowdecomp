file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

# The extension core: script types, the registry and the hook dispatch. The extensions themselves are
# collected by Extensions/CMakeLists.txt into the WowExtensions library.
add_library(Extension STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Extension
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)
