file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
endif ()

add_library(Battlenet STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Battlenet
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

if (CMAKE_DL_LIBS)
    target_link_libraries(Battlenet PRIVATE ${CMAKE_DL_LIBS})
endif ()
