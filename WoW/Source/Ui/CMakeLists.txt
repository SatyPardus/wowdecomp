file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/UILCD/(Windows/|LCD\\.cpp$)")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Mac/")
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
