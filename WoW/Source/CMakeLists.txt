if (NOT WIN32 AND NOT APPLE)
    message(STATUS "Skipping the Wow client target: no platform layer for this OS yet.")
    return()
endif ()

file(GLOB PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

if (APPLE)
    file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS "Mac/*.cpp")
    add_executable(Wow ${PRIVATE_SOURCES} ${PLATFORM_SOURCES})
    set(WOW_OUTPUT_NAME Wow)
    set(WOW_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Mac)
else ()
    if (WOWDECOMP_ARCH_64)
        file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS "Win64/*.cpp")
        set(WOW_OUTPUT_NAME Wow64)
    else ()
        file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS "Win32/*.cpp")
        set(WOW_OUTPUT_NAME Wow32)
    endif ()
    add_executable(Wow WIN32 ${PRIVATE_SOURCES} ${PLATFORM_SOURCES})
    set(WOW_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Win)
endif ()

set_target_properties(Wow PROPERTIES
    OUTPUT_NAME ${WOW_OUTPUT_NAME}
    RUNTIME_OUTPUT_DIRECTORY ${WOW_OUTPUT_DIRECTORY}
)

foreach (config ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${config} config)
    set_target_properties(Wow PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} ${WOW_OUTPUT_DIRECTORY})
endforeach ()

target_include_directories(Wow
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

if (MSVC)
    target_link_options(Wow PRIVATE "/INCREMENTAL:NO" "/ENTRY:start")
endif ()

if (APPLE)
    target_link_libraries(Wow PRIVATE SDL2::SDL2 OpenGL::GL CURL::libcurl)
endif ()

target_link_libraries(Wow
    PRIVATE
        BLPFile
        Base
        BlizzardCore
        ComSat
        ConsoleBase
        Event
        FFX
        Frame
        FrameScript
        lua
        FrameXML
        Gx
        Gxu
        Images
        Model2
        Os
        Scrn
        Services
        ShaderEffect
        Tempest
        SoundEngine
        SteelseriesMouse
        Storm
        WoWCommon
        Battlenet
        Console
        ConsoleDevice
        DB
        Game
        Glue
        LiquidSystem
        Magic
        Minigame
        Net
        Object
        ObjectAlloc
        ObjectMgrClient
        SoundInterface
        UIUtil
        Ui
        WorldClient
        WowServices
        WowSvcs
)
