file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(ObjectMgrClient STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(ObjectMgrClient
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(ObjectMgrClient
    PRIVATE
        Extension
)
