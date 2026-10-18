file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/UILCD/Windows/")
endif ()
if (APPLE)
    # The optional Logitech LCD integration still requires its Windows SDK.
    # Keep the game UI independent of that peripheral backend.
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/UILCD/")
endif ()

add_library(Ui STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(Ui
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

if (WIN32)
    target_link_libraries(Ui
        PRIVATE
            lgLcd
    )
endif ()
