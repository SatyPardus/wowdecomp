# Incremental native port. This intentionally does not expose a WowClient
# target until the game, graphics, sound and platform modules can link.
if (CMAKE_VERSION VERSION_LESS 3.21)
    message(FATAL_ERROR "Apple builds require CMake 3.21 or newer.")
endif ()
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0119 NEW)

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The Apple port requires a 64-bit target.")
endif ()
message(STATUS "Apple foundation: shared core and archive probe; game/Metal are not yet linked")

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>)

add_library(AppleFoundation STATIC
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/SMem.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/Apple/Diagnostics.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/soprintf.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Lock/Apple/Lock.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Thread/Apple/Thread.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Thread/Apple/ThreadLocalStorage.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Using/Using.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Debug/ErrorStack.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/String/String.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/String/Path.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/File/File.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/File/Apple/Filestack.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/System/Time/Apple/Time.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnitContainer.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/Packages/iounit/Posix/IOPosixFileUnit.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQEncrypt.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/CDataStore.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/ConvertUTF.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/TimeManager.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/FunctionValidation.cpp
)
target_include_directories(AppleFoundation PUBLIC ${CMAKE_SOURCE_DIR})
# Full local installations are supported first. The remote streaming filesystem
# must not be installed until its transport/cache backend has been ported.
target_compile_definitions(AppleFoundation PUBLIC WOW_LOCAL_FILES_ONLY=1)

add_subdirectory(Engine/Source/Tempest)
target_link_libraries(Tempest PUBLIC AppleFoundation)
add_subdirectory(Engine/Source/lua-5.1)
target_link_libraries(lua PUBLIC AppleFoundation)

add_library(AppleProbeCore STATIC Tools/AppleBootstrap/Probe.cpp)
target_link_libraries(AppleProbeCore PUBLIC Tempest lua AppleFoundation)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_language(OBJCXX)
    add_executable(WowAppleBootstrap MACOSX_BUNDLE Tools/AppleBootstrap/iOSMain.mm)
    target_compile_options(WowAppleBootstrap PRIVATE -fobjc-arc)
    target_link_libraries(WowAppleBootstrap PRIVATE AppleProbeCore "-framework UIKit" "-framework Foundation")
    set_target_properties(WowAppleBootstrap PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Tools/AppleBootstrap/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER org.wowdecomp.AppleBootstrap
        MACOSX_BUNDLE_BUNDLE_NAME WowAppleBootstrap
        MACOSX_BUNDLE_BUNDLE_VERSION 1
        MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER org.wowdecomp.AppleBootstrap
        XCODE_GENERATE_SCHEME TRUE
    )
else ()
    add_executable(AppleCoreProbe Tools/AppleBootstrap/main.cpp)
    target_link_libraries(AppleCoreProbe PRIVATE AppleProbeCore)
endif ()
