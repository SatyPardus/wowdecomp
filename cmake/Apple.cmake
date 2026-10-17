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
message(STATUS "Apple foundation: shared core, archives and native Metal textures; full game is not yet linked")

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>)
add_link_options(-Wl,-dead_strip)

add_library(AppleFoundation STATIC
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/SMem.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/Apple/Diagnostics.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/Apple/Sync.cpp
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
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/Handle.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/RCString.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/RefCount.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/SIMD.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/Prop.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/CDataAllocator.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/CDataRecycler.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/Tls.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/OsCall.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/Input.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/Coordinate.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/ConvertUTF.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/OsFile.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/OsMemory.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/TimeManager.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Processor.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/Processor.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/FunctionValidation.cpp
)
target_include_directories(AppleFoundation PUBLIC ${CMAKE_SOURCE_DIR})
# Full local installations are supported first. The remote streaming filesystem
# must not be installed until its transport/cache backend has been ported.
target_compile_definitions(AppleFoundation PUBLIC WOW_LOCAL_FILES_ONLY=1)

add_subdirectory(ThirdParty EXCLUDE_FROM_ALL)
add_library(AppleArchives STATIC
    WoW/Source/WowServices/ClientData.cpp
    Engine/Source/BlizzardCore/SFile2-Core.cpp
    Engine/Source/BlizzardCore/Source/Packages/Mopaq/IOMpqUnit.cpp
    Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQGlobals.cpp
    Engine/Source/BlizzardCore/Source/Packages/Mopaq/SComp.cpp
    Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.cpp
    Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.cpp
    Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileSignatures.cpp
    Engine/Source/BlizzardCore/Source/Packages/iounit/IOAlignUnit.cpp
    Engine/Source/BlizzardCore/Source/Packages/iounit/IOChecksumUnit.cpp
    Engine/Source/BlizzardCore/Source/Packages/iounit/IOFileUnit.cpp
    Engine/Source/BlizzardCore/Source/Packages/iounit/Apple/IOFileUnit.cpp
    Engine/Source/BlizzardCore/Source/Packages/Streaming/Local/Streaming.cpp
    Engine/Source/BlizzardCore/Source/Packages/Util/Hash.cpp
    Engine/Source/BlizzardCore/Source/Packages/Util/MD5.cpp
    Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.cpp
    Engine/Source/BlizzardCore/Source/Packages/Util/SBig.cpp
    Engine/Source/BlizzardCore/Source/System/Process/Apple/Process.cpp
    Engine/Source/BlizzardCore/Source/System/Hardware/Apple/Hardware.cpp
    Engine/Source/BlizzardCore/Source/System/Debug/Debug.cpp
    Engine/Source/BlizzardCore/Source/System/Debug/Apple/Debug.cpp
    Engine/Source/BlizzardCore/Source/System/Log/Log.cpp
    Engine/Source/BlizzardCore/Source/System/Log/Apple/Log.cpp
    Engine/Source/Storm/Source/SComp.cpp
    Engine/Source/Storm/Source/SStr.cpp
    Engine/Source/Storm/Source/SOption.cpp
    Engine/Source/Storm/Source/MD5.cpp
    Engine/Source/Storm/Source/Apple/SLog.cpp
)
target_link_libraries(AppleArchives PUBLIC AppleFoundation blast bzip2 lzma zlib)

add_subdirectory(Engine/Source/Tempest)
target_link_libraries(Tempest PUBLIC AppleFoundation)
add_subdirectory(Engine/Source/lua-5.1)
target_link_libraries(lua PUBLIC AppleFoundation)

# Shared game graphics and the original procedural texture/font path.
# Full client startup and the resource/event services are not yet initialized.
add_subdirectory(Engine/Source/Gx)
target_link_libraries(Gx PUBLIC Tempest AppleArchives)
add_subdirectory(Engine/Source/Gxu)
target_link_libraries(Gxu PUBLIC Gx)
add_subdirectory(Engine/Source/jpeg)
set_target_properties(jpeg PROPERTIES CXX_STANDARD 14)
add_subdirectory(Engine/Source/Images)
add_subdirectory(Engine/Source/BLPFile)
target_link_libraries(BLPFile PUBLIC Images Gx)
add_subdirectory(Engine/Source/ConsoleBase)
target_link_libraries(ConsoleBase PUBLIC AppleArchives)
add_subdirectory(Engine/Source/Event)
target_link_libraries(Event PUBLIC AppleArchives Tempest)
add_library(AppleGameResources STATIC
    Engine/Source/Base/Status.cpp
    Engine/Source/Services/Texture.cpp
    Engine/Source/Services/TextureBlob.cpp
    Engine/Source/Services/SysMessage.cpp
    Engine/Source/Services/AsyncFile.cpp
    Engine/Source/Services/ModelBlob.cpp
    Engine/Source/Services/Camera.cpp
    Engine/Source/Services/GfxSingletonManager.cpp
    Engine/Source/Services/ParticleSystem.cpp
    Engine/Source/Services/ParticleSystem2.cpp
    Engine/Source/Services/RibbonEmitter.cpp
    WoW/Source/ObjectAlloc/ObjectAlloc.cpp
    WoW/Source/DB/WowClientDB.cpp
    WoW/Source/DB/DBClient/AnimationData.cpp
    WoW/Source/DB/Rec/AnimationDataRec.cpp
    Engine/Source/FFX/FFX.cpp
    Engine/Source/FFX/FFX_CVar.cpp
    Engine/Source/FFX/EffectGlow.cpp
    Engine/Source/FFX/PassBox4.cpp
    Engine/Source/FFX/PassGauss4.cpp
    Engine/Source/FFX/PassGlow.cpp
)
target_link_libraries(AppleGameResources PUBLIC Gxu AppleArchives Images BLPFile ConsoleBase Event)

add_subdirectory(Engine/Source/ShaderEffect)
target_link_libraries(ShaderEffect PUBLIC Gx AppleArchives)
add_subdirectory(Engine/Source/Model2)
target_link_libraries(Model2 PUBLIC AppleGameResources ShaderEffect)

add_library(AppleTextureData ALIAS BLPFile)

enable_language(OBJCXX)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS Engine/Source/Gx/Metal/TextureQuad.metal Engine/Source/Gx/Metal/Model.metal)
file(READ Engine/Source/Gx/Metal/TextureQuad.metal WOW_TEXTURE_QUAD_SOURCE)
file(READ Engine/Source/Gx/Metal/Model.metal WOW_MODEL_SOURCE)
string(APPEND WOW_TEXTURE_QUAD_SOURCE "\n${WOW_MODEL_SOURCE}")
configure_file(Engine/Source/Gx/Metal/ShaderSource.h.in ${CMAKE_BINARY_DIR}/generated/MetalShaderSource.h @ONLY)
add_library(AppleMetal STATIC Engine/Source/Gx/Metal/TextureDevice.mm Engine/Source/Gx/Metal/GeometryDraw.mm
    Engine/Source/Gx/Metal/CGxDeviceMetal.mm Engine/Source/Gx/Metal/ApplePlatform.mm)
target_include_directories(AppleMetal PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_compile_options(AppleMetal PRIVATE -fobjc-arc)
target_link_libraries(AppleMetal PUBLIC AppleTextureData Gx "-framework Metal" "-framework Foundation")
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(AppleMetal PUBLIC "-framework UIKit")
else ()
    target_link_libraries(AppleMetal PUBLIC "-framework AppKit")
endif ()
add_library(AppleMetalProbeCore STATIC Tools/AppleBootstrap/MetalProbe.mm Tools/AppleBootstrap/GeometryProbe.mm Tools/AppleBootstrap/GxDeviceProbe.mm Tools/AppleBootstrap/FontProbe.mm Tools/AppleBootstrap/RenderTargetProbe.mm Tools/AppleBootstrap/Filter4Probe.mm Tools/AppleBootstrap/FFXPassProbe.mm Tools/AppleBootstrap/EventProbe.cpp Tools/AppleBootstrap/AsyncFileProbe.cpp Tools/AppleBootstrap/ModelSceneProbe.mm Tools/AppleBootstrap/ModelShaderProbe.mm)
target_compile_options(AppleMetalProbeCore PRIVATE -fobjc-arc)
target_link_libraries(AppleMetalProbeCore PUBLIC AppleMetal AppleGameResources Model2)
add_library(AppleMetalView STATIC Tools/AppleBootstrap/MetalView.mm Tools/AppleBootstrap/ModelPreview.cpp)
target_compile_options(AppleMetalView PRIVATE -fobjc-arc)
target_link_libraries(AppleMetalView PUBLIC AppleMetal AppleGameResources Model2 "-framework MetalKit" "-framework CoreGraphics")

add_library(AppleProbeCore STATIC Tools/AppleBootstrap/Probe.cpp Tools/AppleBootstrap/ModelProbe.cpp)
target_link_libraries(AppleProbeCore PUBLIC Gx Tempest lua AppleArchives AppleFoundation AppleTextureData ConsoleBase Model2)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(WowAppleBootstrap MACOSX_BUNDLE Tools/AppleBootstrap/iOSMain.mm)
    target_compile_options(WowAppleBootstrap PRIVATE -fobjc-arc)
    target_link_libraries(WowAppleBootstrap PRIVATE AppleProbeCore AppleMetalProbeCore AppleMetalView "-framework UIKit" "-framework Foundation")
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
    add_executable(AppleMetalProbe Tools/AppleBootstrap/metal_main.mm)
    target_compile_options(AppleMetalProbe PRIVATE -fobjc-arc)
    target_link_libraries(AppleMetalProbe PRIVATE AppleMetalProbeCore AppleMetalView AppleArchives "-framework AppKit")
endif ()
