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
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/SRgn.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/SUni.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/FontPath.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/PathExtension.cpp
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
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/CRTRandom.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/CmdLine.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/DynamicString.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/SCmd.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/BitArray.cpp
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
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/NetAddress.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/Coordinate.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Base/ConvertUTF.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/OsFile.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/OsLibrary.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/OsMemory.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/System.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Storm/Source/Apple/SReg.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/TimeManager.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Processor.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/Processor.cpp
    ${CMAKE_SOURCE_DIR}/Engine/Source/Os/Apple/FunctionValidation.cpp
)
target_include_directories(AppleFoundation PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(AppleFoundation PUBLIC "-framework CoreFoundation")
# Full local installations are supported first. The remote streaming filesystem
# must not be installed until its transport/cache backend has been ported.
target_compile_definitions(AppleFoundation PUBLIC WOW_LOCAL_FILES_ONLY=1)

add_subdirectory(ThirdParty EXCLUDE_FROM_ALL)
add_library(AppleArchives STATIC
    WoW/Source/ClientLocale.cpp
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
    WoW/Source/WowServices/Apple/ClientSettings.mm
    WoW/Source/ClientRandom.cpp
    Engine/Source/Base/Status.cpp
    Engine/Source/Services/Texture.cpp
    Engine/Source/Services/TextureBlob.cpp
    Engine/Source/Services/SysMessage.cpp
    Engine/Source/Services/TextBlock.cpp
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
    WoW/Source/DB/DBClient/WorldEffectData.cpp
    WoW/Source/DB/Rec/FootprintTexturesRec.cpp
    WoW/Source/DB/Rec/GroundEffectDoodadRec.cpp
    WoW/Source/DB/Rec/GroundEffectTextureRec.cpp
    WoW/Source/DB/Rec/AreaTableRec.cpp
    WoW/Source/DB/Rec/WMOAreaTableRec.cpp
    WoW/Source/DB/Rec/LightRec.cpp
    WoW/Source/DB/DBClient/DayNightData.cpp
    WoW/Source/DB/DBClient/Lights.cpp
    WoW/Source/DB/Rec/LightParamsRec.cpp
    WoW/Source/DB/Rec/LightSkyboxRec.cpp
    WoW/Source/DB/Rec/LightIntBandRec.cpp
    WoW/Source/DB/Rec/LightFloatBandRec.cpp
    WoW/Source/DB/DBClient/MapData.cpp
    WoW/Source/DB/Rec/MapRec.cpp
    WoW/Source/DB/DBClient/LiquidData.cpp
    WoW/Source/DB/Rec/LiquidTypeRec.cpp
    WoW/Source/DB/Rec/LiquidMaterialRec.cpp
    WoW/Source/DB/DBClient/CreatureAppearance.cpp
    WoW/Source/DB/Rec/CreatureDisplayInfoRec.cpp
    WoW/Source/DB/Rec/CreatureModelDataRec.cpp
    WoW/Source/DB/Rec/ParticleColorRec.cpp
    Engine/Source/FFX/FFX.cpp
    Engine/Source/FFX/FFX_CVar.cpp
    Engine/Source/FFX/EffectGlow.cpp
    Engine/Source/FFX/PassBox4.cpp
    Engine/Source/FFX/PassGauss4.cpp
    Engine/Source/FFX/PassGlow.cpp
    WoW/Source/Game/GameClient/PassDeath.cpp
    WoW/Source/Game/GameClient/EffectDeath.cpp
)
target_link_libraries(AppleGameResources PUBLIC Gxu AppleArchives Images BLPFile ConsoleBase Event)

add_library(AppleGameNetwork STATIC
    WoW/Source/WowServices/WowConnection.cpp
    WoW/Source/WowServices/WowConnectionSocket.cpp
    WoW/Source/WowServices/WowConnectionNet.cpp
    WoW/Source/WowServices/Apple/WowConnectionNet.cpp
    WoW/Source/WowServices/WDataStore.cpp
    Engine/Source/Storm/Source/SArc4.cpp)
target_link_libraries(AppleGameNetwork PUBLIC AppleGameResources)
add_library(AppleGameLogin STATIC
    Engine/Source/Os/Apple/LoginPlatform.cpp
    Engine/Source/Storm/Source/SBig.cpp
    WoW/Source/WowServices/BigInteger.cpp
    WoW/Source/WowServices/SRP6_Client.cpp
    WoW/Source/WowServices/SRP6_Random.cpp
    WoW/Source/WowServices/Grunt.cpp
    WoW/Source/WowServices/Login.cpp
    WoW/Source/WowServices/GruntLogin.cpp)
target_link_libraries(AppleGameLogin PUBLIC AppleGameNetwork "-framework Security")
add_library(AppleLoginProbeCore STATIC Tools/AppleBootstrap/LoginProbe.cpp)
target_link_libraries(AppleLoginProbeCore PUBLIC AppleGameLogin)
add_library(AppleNetworkProbeCore STATIC Tools/AppleBootstrap/NetworkProbe.cpp)
target_link_libraries(AppleNetworkProbeCore PUBLIC AppleGameNetwork)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(AppleNetworkProbe Tools/AppleBootstrap/network_main.cpp)
    target_link_libraries(AppleNetworkProbe PRIVATE AppleNetworkProbeCore)
    add_executable(AppleLoginProbe Tools/AppleBootstrap/login_main.cpp)
    target_link_libraries(AppleLoginProbe PRIVATE AppleLoginProbeCore)
endif ()

add_subdirectory(Engine/Source/ShaderEffect)
target_link_libraries(ShaderEffect PUBLIC Gx AppleArchives)
add_subdirectory(Engine/Source/Model2)
target_link_libraries(Model2 PUBLIC AppleGameResources ShaderEffect)
add_library(CreatureAppearance STATIC WoW/Common/ComponentCore/MonsterAppearance.cpp)
target_link_libraries(CreatureAppearance PUBLIC Model2)
add_library(CharacterTextureCache STATIC WoW/Common/TextureCache.cpp)
target_link_libraries(CharacterTextureCache PUBLIC AppleGameResources)
add_library(CharacterAppearance STATIC
    WoW/Source/Glue/ComponentData.cpp
    WoW/Common/ComponentCore/CharacterComponent.cpp
    WoW/Common/ComponentCore/CharacterCompressionNeon.cpp
    WoW/Common/ComponentCore/CharacterComponent_CVar.cpp
    WoW/Common/ComponentCore/ComponentUtils.cpp
    WoW/Common/ComponentCore/PlayerComponentState.cpp
    WoW/Common/ComponentCore/CharacterIdentity.cpp
    WoW/Common/ComponentCore/CharacterStartingOutfit.cpp
    WoW/Source/Game/GameClient/CDrawBowString.cpp
    WoW/Source/DB/Rec/CharBaseInfoRec.cpp
    WoW/Source/DB/Rec/CharStartOutfitRec.cpp
    WoW/Source/DB/Rec/ChrClassesRec.cpp
    WoW/Source/DB/DBClient/CharacterAppearance.cpp
    WoW/Source/DB/Rec/CharHairGeosetsRec.cpp
    WoW/Source/DB/Rec/CharSectionsRec.cpp
    WoW/Source/DB/Rec/CharacterFacialHairStylesRec.cpp
    WoW/Source/DB/Rec/ChrRacesRec.cpp
    WoW/Source/DB/Rec/HelmetGeosetVisDataRec.cpp
    WoW/Source/DB/Rec/ItemDisplayInfoRec.cpp
    WoW/Source/DB/Rec/ItemVisualEffectsRec.cpp
    WoW/Source/DB/Rec/ItemVisualsRec.cpp)
target_link_libraries(CharacterAppearance PUBLIC CharacterTextureCache CreatureAppearance Model2)
add_subdirectory(Engine/Source/pcre)
add_library(CharacterNames STATIC
    WoW/Source/Game/ValidateName.cpp WoW/Source/Game/NameGen.cpp
    WoW/Source/DB/DBClient/CharacterNames.cpp
    WoW/Source/DB/Rec/ChatProfanityRec.cpp WoW/Source/DB/Rec/NameGenRec.cpp
    WoW/Source/DB/Rec/NamesProfanityRec.cpp WoW/Source/DB/Rec/NamesReservedRec.cpp
    WoW/Source/DB/Rec/SpamMessagesRec.cpp)
target_link_libraries(CharacterNames PUBLIC AppleGameResources ConsoleBase pcreposix)

add_subdirectory(Engine/Source/FrameScript)
target_link_libraries(FrameScript PUBLIC AppleGameResources lua)
add_subdirectory(Engine/Source/Frame)
add_subdirectory(Engine/Source/FrameXML)
add_subdirectory(Engine/Source/Scrn)
target_link_libraries(Frame PUBLIC FrameScript FrameXML Scrn Model2)
target_sources(Frame PRIVATE ${CMAKE_SOURCE_DIR}/WoW/Source/Ui/MouseButtonNames.cpp)
target_link_libraries(FrameXML PUBLIC Frame)
target_link_libraries(Scrn PUBLIC AppleGameResources)

# Shared sound policy/cache and original software mixer. Full game sound policy
# is not initialized by the current graphics host.
add_library(SoundEngine STATIC Engine/Source/SoundEngine/SoundEngine.cpp Engine/Source/SoundEngine/SoundCache.cpp)
target_link_libraries(SoundEngine PUBLIC AppleGameResources FrameScript)
add_subdirectory(WoW/Source/SoundInterface)
target_link_libraries(SoundInterface PUBLIC SoundEngine)
include(cmake/AppleAudio.cmake)

add_library(WorldSupport STATIC WoW/Common/AaBsp.cpp)
target_link_libraries(WorldSupport PUBLIC Tempest)
add_library(ClientWorldSupport STATIC WoW/Source/ObjectMgrClient/ObjectMgrClient.cpp
    WoW/Source/Object/ObjectClient/ObjectEffect.cpp WoW/Source/UIUtil/CSimpleCamera.cpp
    WoW/Source/Ui/SoundPlaybackState.cpp WoW/Source/Object/ObjectClient/UnitWorldQueries_C.cpp
    WoW/Common/WowTime.cpp WoW/Source/Game/GameTime.cpp
    WoW/Source/Object/Object.cpp WoW/Source/Object/Item.cpp WoW/Source/Object/Container.cpp
    WoW/Source/Object/Unit.cpp WoW/Source/Object/Player.cpp WoW/Source/Object/GameObject.cpp
    WoW/Source/Object/DynamicObject.cpp WoW/Source/Object/Corpse.cpp)
target_link_libraries(ClientWorldSupport PUBLIC Model2 SoundInterface)
add_subdirectory(WoW/Source/WorldClient)
add_subdirectory(WoW/Source/LiquidSystem)
target_link_libraries(LiquidSystem PUBLIC Model2 WorldSupport)
target_sources(WorldClient PRIVATE ${CMAKE_SOURCE_DIR}/WoW/Source/Ui/WorldFrameHitTest.cpp
    ${CMAKE_SOURCE_DIR}/WoW/Source/Game/GameClient/ProjectTex.cpp
    ${CMAKE_SOURCE_DIR}/WoW/Source/ConsoleDevice/ConsoleDevice.cpp
    ${CMAKE_SOURCE_DIR}/WoW/Source/ConsoleDevice/ConsoleDevice_CVar.cpp
    ${CMAKE_SOURCE_DIR}/WoW/Source/ConsoleDevice/ConsoleHardware.cpp
    ${CMAKE_SOURCE_DIR}/WoW/Source/Hardware.cpp
    ${CMAKE_SOURCE_DIR}/WoW/Source/DB/Rec/VideoHardwareRec.cpp)
add_library(WorldTerrain STATIC WoW/Common/TerrainGeometry.cpp WoW/Common/TerrainSurface.cpp WoW/Common/TerrainAlpha.cpp)
target_link_libraries(WorldTerrain PUBLIC Tempest)
target_link_libraries(WorldClient PUBLIC Model2 WorldTerrain WorldSupport SoundInterface LiquidSystem ClientWorldSupport)

add_subdirectory(WoW/Source/Net)
add_library(AppleVideoOptions STATIC WoW/Source/Ui/VideoOptions_Scripts.cpp WoW/Source/Ui/Screen.cpp)
target_link_libraries(AppleVideoOptions PUBLIC Frame ConsoleBase Gx)
add_library(AppleKnowledgeBase STATIC
    WoW/Source/Ui/KnowledgeBase.cpp WoW/Source/Ui/KnowledgeBaseApple.cpp
    WoW/Source/Ui/CKBArticle.cpp WoW/Source/Ui/CKBQuery.cpp WoW/Source/Ui/CKBIterator.cpp
    WoW/Source/Ui/CAccountMsgHeaders.cpp WoW/Source/Ui/CAccountMsgBody.cpp WoW/Source/Ui/CAccountMsgHeaders_Scripts.cpp
    WoW/Source/Ui/GameUIState.cpp)
target_link_libraries(AppleKnowledgeBase PUBLIC FrameXML FrameScript CharacterNames)
add_subdirectory(WoW/Source/Glue)
add_subdirectory(WoW/Source/WowSvcs)
target_link_libraries(Net PUBLIC ClientWorldSupport)
target_link_libraries(Glue PUBLIC Frame WorldClient AppleGameLogin CharacterAppearance CharacterNames AppleKnowledgeBase AppleVideoOptions)
target_link_libraries(WowSvcs PUBLIC Net AppleGameLogin Glue)
target_sources(AppleGameLogin PRIVATE WoW/Source/WowServices/RealmConnection.cpp)
add_library(AppleSessionServices STATIC
    WoW/Source/Game/GameClient/GuildTabard.cpp
    Engine/Source/Storm/Source/SCrc.cpp
    Engine/Source/Storm/Source/SSignature.cpp
    Engine/Source/Storm/Source/Sha1.cpp
    Engine/Source/Storm/Source/RSA.cpp
    WoW/Source/UIUtil/AddOns.cpp
    WoW/Source/UIUtil/BannedAddOns.cpp
    WoW/Source/WowServices/Apple/ClientInstall.cpp
    WoW/Source/WowServices/Apple/ClientCache.mm
    WoW/Source/WowSvcs/WowSvcsClient/AccountData.cpp
    WoW/Source/WowSvcs/WowSvcsClient/AccountData_CVar.cpp
    WoW/Source/Game/GameClient/WardenCache.cpp
    WoW/Source/Game/GameClient/NameCache.cpp
    WoW/Source/Game/GameClient/PetNameCache.cpp
    WoW/Source/Object/CGPetition.cpp
    WoW/Source/Object/DanceCache.cpp
    WoW/Source/Object/QuestCache.cpp
    WoW/Source/Object/ItemStats.cpp
    WoW/Source/Object/ItemName.cpp
    WoW/Source/Object/ItemTextCache.cpp
    WoW/Source/Object/PageTextCache.cpp
    WoW/Source/Object/GameObjectStats.cpp
    WoW/Source/Object/NPCText.cpp
    WoW/Source/DB/DBClient/CreatureStats_C.cpp
    WoW/Source/DB/DBClient/GameObjectStats_C.cpp
    WoW/Source/DB/DBClient/BannedAddOnsData.cpp
    WoW/Source/DB/Rec/BannedAddOnsRec.cpp
    WoW/Source/DB/DBClient/DBCache.cpp
    WoW/Source/DB/DBClient/DBCacheInstances.cpp)
target_link_libraries(AppleSessionServices PUBLIC Net Frame AppleGameLogin)
target_link_libraries(WowSvcs PUBLIC AppleSessionServices)
add_library(AppleRealmProbeCore STATIC Tools/AppleBootstrap/RealmProbe.cpp Tools/AppleBootstrap/CacheProbe.cpp Tools/AppleBootstrap/AccountDataProbe.cpp)
target_link_libraries(AppleRealmProbeCore PUBLIC AppleGameLogin AppleSessionServices AppleMetal)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(AppleRealmProbe Tools/AppleBootstrap/realm_main.cpp)
    target_link_libraries(AppleRealmProbe PRIVATE AppleRealmProbeCore)
endif ()
add_library(AppleSessionProbeCore STATIC Tools/AppleBootstrap/SessionProbe.cpp)
target_link_libraries(AppleSessionProbeCore PUBLIC Net)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(AppleSessionProbe Tools/AppleBootstrap/session_main.cpp)
    target_link_libraries(AppleSessionProbe PRIVATE AppleSessionProbeCore)
endif ()

add_library(AppleTextureData ALIAS BLPFile)

enable_language(OBJCXX)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
target_sources(AppleFoundation PRIVATE Engine/Source/Os/Apple/OsURLDownload.mm Engine/Source/Storm/Source/Apple/SCmd.mm)
target_compile_options(AppleFoundation PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
target_link_libraries(AppleFoundation PUBLIC "-framework Foundation")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS Engine/Source/Gx/Metal/TextureQuad.metal Engine/Source/Gx/Metal/Skinning.metal Engine/Source/Gx/Metal/ShadowMap.metal Engine/Source/Gx/Metal/Model.metal Engine/Source/Gx/Metal/Terrain.metal Engine/Source/Gx/Metal/Wmo.metal Engine/Source/Gx/Metal/Liquid.metal Engine/Source/Gx/Metal/DetailDoodad.metal Engine/Source/Gx/Metal/WaterRipple.metal Engine/Source/Gx/Metal/FixedUnlit.metal)
file(READ Engine/Source/Gx/Metal/TextureQuad.metal WOW_TEXTURE_QUAD_SOURCE)
file(READ Engine/Source/Gx/Metal/Skinning.metal WOW_SKINNING_SOURCE)
file(READ Engine/Source/Gx/Metal/ShadowMap.metal WOW_SHADOW_MAP_SOURCE)
file(READ Engine/Source/Gx/Metal/Model.metal WOW_MODEL_SOURCE)
file(READ Engine/Source/Gx/Metal/Terrain.metal WOW_TERRAIN_SOURCE)
file(READ Engine/Source/Gx/Metal/Wmo.metal WOW_WMO_SOURCE)
file(READ Engine/Source/Gx/Metal/Liquid.metal WOW_LIQUID_SOURCE)
file(READ Engine/Source/Gx/Metal/DetailDoodad.metal WOW_DETAIL_DOODAD_SOURCE)
file(READ Engine/Source/Gx/Metal/WaterRipple.metal WOW_WATER_RIPPLE_SOURCE)
file(READ Engine/Source/Gx/Metal/FixedUnlit.metal WOW_FIXED_UNLIT_SOURCE)
string(APPEND WOW_TEXTURE_QUAD_SOURCE "\n${WOW_SKINNING_SOURCE}\n${WOW_SHADOW_MAP_SOURCE}\n${WOW_MODEL_SOURCE}\n${WOW_TERRAIN_SOURCE}\n${WOW_WMO_SOURCE}\n${WOW_LIQUID_SOURCE}\n${WOW_DETAIL_DOODAD_SOURCE}\n${WOW_WATER_RIPPLE_SOURCE}\n${WOW_FIXED_UNLIT_SOURCE}")
configure_file(Engine/Source/Gx/Metal/ShaderSource.h.in ${CMAKE_BINARY_DIR}/generated/MetalShaderSource.h @ONLY)
add_library(AppleMetal STATIC Engine/Source/Gx/Metal/TextureDevice.mm Engine/Source/Gx/Metal/GeometryDraw.mm
    Engine/Source/Gx/Metal/CGxDeviceMetal.mm Engine/Source/Gx/Metal/ApplePlatform.mm
    Engine/Source/Os/Apple/OsClipboard.mm Engine/Source/Os/OsClipboard.cpp)
target_include_directories(AppleMetal PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_compile_options(AppleMetal PRIVATE -fobjc-arc)
target_link_libraries(AppleMetal PUBLIC AppleTextureData Gx "-framework Metal" "-framework Foundation")
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(AppleMetal PUBLIC "-framework UIKit")
else ()
    target_link_libraries(AppleMetal PUBLIC "-framework AppKit")
endif ()
add_library(AppleLiquidView STATIC Tools/AppleBootstrap/LiquidPreviewContext.cpp)
target_link_libraries(AppleLiquidView PUBLIC WorldClient AppleGameResources)
add_library(AppleTerrainView STATIC Tools/AppleBootstrap/TerrainPreview.cpp Tools/AppleBootstrap/TerrainLiquidPreview.cpp Tools/AppleBootstrap/DayNightPreview.cpp)
target_link_libraries(AppleTerrainView PUBLIC AppleGameResources WorldTerrain AppleLiquidView)
add_library(AppleWmoView STATIC Tools/AppleBootstrap/WmoPreview.cpp Tools/AppleBootstrap/WmoLiquidPreview.cpp)
target_link_libraries(AppleWmoView PUBLIC WorldClient AppleGameResources AppleLiquidView)
add_library(AppleUIComponents STATIC Tools/AppleBootstrap/UIComponents.cpp)
target_link_libraries(AppleUIComponents PUBLIC Frame)
add_library(AppleMetalProbeCore STATIC Tools/AppleBootstrap/MetalProbe.mm Tools/AppleBootstrap/GeometryProbe.mm Tools/AppleBootstrap/FixedUnlitProbe.mm Tools/AppleBootstrap/GxDeviceProbe.mm Tools/AppleBootstrap/DisplaySettingsProbe.mm Tools/AppleBootstrap/FontProbe.mm Tools/AppleBootstrap/UIRenderProbe.mm Tools/AppleBootstrap/RenderTargetProbe.mm Tools/AppleBootstrap/Filter4Probe.mm Tools/AppleBootstrap/FFXPassProbe.mm Tools/AppleBootstrap/EventProbe.cpp Tools/AppleBootstrap/AsyncFileProbe.cpp Tools/AppleBootstrap/ModelSceneProbe.mm Tools/AppleBootstrap/ModelShaderProbe.mm Tools/AppleBootstrap/ShadowMapProbe.mm Tools/AppleBootstrap/DetailDoodadProbe.mm Tools/AppleBootstrap/WaterRippleProbe.mm Tools/AppleBootstrap/TerrainShaderProbe.mm Tools/AppleBootstrap/CubeTextureProbe.mm Tools/AppleBootstrap/WmoShaderProbe.mm Tools/AppleBootstrap/LiquidShaderProbe.mm Tools/AppleBootstrap/WmoSceneProbe.mm Tools/AppleBootstrap/TerrainSceneProbe.mm Tools/AppleBootstrap/WorldProbe.mm Tools/AppleBootstrap/DayNightProbe.mm)
target_compile_options(AppleMetalProbeCore PRIVATE -fobjc-arc)
add_library(AppleWorldProbe STATIC Tools/AppleBootstrap/WmoProbe.cpp)
target_link_libraries(AppleWorldProbe PUBLIC WorldClient AppleMetal)
target_link_libraries(AppleMetalProbeCore PUBLIC AppleUIComponents AppleVideoOptions AppleMetal AppleGameResources Model2 CreatureAppearance AppleTerrainView AppleWorldProbe AppleWmoView)
target_sources(AppleMetalProbeCore PRIVATE Tools/AppleBootstrap/CharacterSceneProbe.mm)
target_link_libraries(AppleMetalProbeCore PUBLIC CharacterAppearance)
add_library(AppleMetalView STATIC Tools/AppleBootstrap/MetalView.mm Tools/AppleBootstrap/NativeView.mm Tools/AppleBootstrap/ModelPreview.cpp Tools/AppleBootstrap/WorldPreview.cpp)
target_compile_options(AppleMetalView PRIVATE -fobjc-arc)
target_link_libraries(AppleMetalView PUBLIC CharacterAppearance AppleMetal AppleGameResources Model2 CreatureAppearance AppleTerrainView AppleWmoView AppleGameAudio "-framework MetalKit" "-framework CoreGraphics")

add_library(AppleProbeCore STATIC Tools/AppleBootstrap/Probe.cpp Tools/AppleBootstrap/ModelProbe.cpp Tools/AppleBootstrap/TerrainProbe.cpp Tools/AppleBootstrap/WorldDataProbe.cpp Tools/AppleBootstrap/LiquidDataProbe.cpp)
target_link_libraries(AppleProbeCore PUBLIC Gx Tempest lua AppleArchives AppleFoundation AppleTextureData ConsoleBase Model2 WorldTerrain WorldSupport AppleAudioProbeCore AppleWorldProbe AppleNetworkProbeCore AppleLoginProbeCore AppleSessionProbeCore AppleRealmProbeCore)
target_sources(AppleProbeCore PRIVATE Tools/AppleBootstrap/CharacterTextureProbe.cpp)
target_link_libraries(AppleProbeCore PUBLIC CharacterTextureCache)
target_sources(AppleProbeCore PRIVATE Tools/AppleBootstrap/CharacterNameProbe.cpp)
target_sources(AppleProbeCore PRIVATE Tools/AppleBootstrap/URLDownloadProbe.cpp)
target_sources(AppleProbeCore PRIVATE Tools/AppleBootstrap/CommandLineProbe.cpp)
target_link_libraries(AppleProbeCore PUBLIC CharacterNames AppleKnowledgeBase)

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
    target_link_libraries(AppleMetalView PUBLIC "-framework Carbon")
    add_executable(AppleInputProbe Tools/AppleBootstrap/NativeInputProbe.mm)
    target_compile_options(AppleInputProbe PRIVATE -fobjc-arc)
    target_link_libraries(AppleInputProbe PRIVATE AppleMetalView)
    add_executable(AppleUIProbe Tools/AppleBootstrap/UIProbe.mm)
    target_compile_options(AppleUIProbe PRIVATE -fobjc-arc)
    target_link_libraries(AppleUIProbe PRIVATE AppleUIComponents AppleMetalView)
    add_executable(AppleCoreProbe Tools/AppleBootstrap/main.cpp)
    target_link_libraries(AppleCoreProbe PRIVATE AppleProbeCore)
    add_executable(AppleMetalProbe Tools/AppleBootstrap/metal_main.mm)
    target_compile_options(AppleMetalProbe PRIVATE -fobjc-arc)
    target_link_libraries(AppleMetalProbe PRIVATE AppleMetalProbeCore AppleMetalView AppleArchives "-framework AppKit")
endif ()
