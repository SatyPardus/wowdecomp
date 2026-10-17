# CMAKE_PROJECT_INCLUDE file: XvidDecoder must use the DLL runtime (/MD) because the prebuilt libxvidcore.lib names
# /DEFAULTLIB:MSVCRT. The target does not exist yet when project() runs, so defer until the root directory is done.
# The property is appended after CMAKE_CXX_FLAGS_RELEASE's /MT, and the last runtime flag wins in clang-cl.
# (This file also runs for try_compile projects, which have no XvidDecoder, hence the guard.)
if (EXISTS "${CMAKE_SOURCE_DIR}/Tools/XvidDecoder/CMakeLists.txt")
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL
        set_property TARGET XvidDecoder APPEND PROPERTY COMPILE_OPTIONS /MD)
endif ()
