# Writes WaystoneVersion.h from git: the build number is the commit count of HEAD, so every new commit gets a higher
# number and the same commit always gets the same number. The file is only rewritten when a value changes.
# Usage: cmake -DSOURCE_DIR=<repo> -DOUTPUT=<header> -P GenerateWaystoneVersion.cmake

set(WAYSTONE_BUILD 0)
set(WAYSTONE_COMMIT "unknown")
set(WAYSTONE_DATE "unknown")

find_package(Git QUIET)

if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE count
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if (result EQUAL 0 AND count MATCHES "^[0-9]+$")
        set(WAYSTONE_BUILD ${count})
    endif ()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE commit
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if (result EQUAL 0 AND commit)
        set(WAYSTONE_COMMIT ${commit})
    endif ()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd "--date=format:%b %d %Y"
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE date
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if (result EQUAL 0 AND date)
        set(WAYSTONE_DATE ${date})
    endif ()
endif ()

set(content "#ifndef WAYSTONE_VERSION_H
#define WAYSTONE_VERSION_H

#define WAYSTONE_BUILD ${WAYSTONE_BUILD}
#define WAYSTONE_BUILD_STRING \"${WAYSTONE_BUILD}\"
#define WAYSTONE_COMMIT \"${WAYSTONE_COMMIT}\"
#define WAYSTONE_DATE \"${WAYSTONE_DATE}\"

#endif
")

set(existing "")

if (EXISTS ${OUTPUT})
    file(READ ${OUTPUT} existing)
endif ()

if (NOT existing STREQUAL content)
    file(WRITE ${OUTPUT} "${content}")
endif ()
