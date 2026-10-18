## OnGlueResume

source: `CGlueMgr` script registration (`CGlueMgr.cpp`)

Mods are mounted just before this hook, so their listeners are already in place.

## OnGlueIdle

source: `CGlueMgr` idle loop (`CGlueMgr.cpp`)

## OnGlueScreenChange

source: `CGlueMgr` (`CGlueMgr.cpp`)

`screenName` is the name the Glue manager uses for the screen, for example login, charselect, or charcreate.

## IsLoginBlocked

source: `DefaultServerLogin` script (`GlueScriptEvents_Scripts.cpp`)

Return `true` and `DefaultServerLogin(account, password)` returns without doing anything. It does not show a message, so show your own.

## OnCharacterSelect

source: `CharacterSelection_Scripts.cpp`

`charIndex` is the index passed to the select script.

## OnCharacterCreate

source: `CharacterCreation.cpp`

`race`, `classId`, and `gender` are the numeric ids chosen in the creation UI.
