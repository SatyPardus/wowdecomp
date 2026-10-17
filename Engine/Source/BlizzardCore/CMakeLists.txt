file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (NOT WIN32)
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Win(32|64)/")
else ()
    list(FILTER PRIVATE_SOURCES EXCLUDE REGEX "/Mac/")
endif ()

add_library(BlizzardCore STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(BlizzardCore
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(BlizzardCore
    PUBLIC
        blast
        lzma
        zlib
)

if(WIN32)
    target_link_libraries(BlizzardCore
        PUBLIC
            wininet
    )
elseif(APPLE)
    target_link_libraries(BlizzardCore
        PUBLIC
            CURL::libcurl
    )
endif()

target_include_directories(BlizzardCore
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine/Source/SoundEngine/fmodex/lib/ogg_vorbis/ogg/include
        ${CMAKE_SOURCE_DIR}/Engine/Source/SoundEngine/fmodex/lib/ogg_vorbis/vorbis/include
)
